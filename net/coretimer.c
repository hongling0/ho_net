#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include <Windows.h>
#include "coretimer.h"

#define TIME_SLOT_LEN 10
#define TIMER_HASH_SIZE (1<<10)
#define TIME_NEAR (1<<8)
#define TIME_LEVEL (1 << 6)

#define HASH(id) (id & (TIMER_HASH_SIZE-1))
#define TIME_NEAR_MASK (TIME_NEAR-1)
#define TIME_LEVEL_MASK (TIME_LEVEL-1)

static uint64_t gettime()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER u_int;
	u_int.HighPart = ft.dwHighDateTime;
	u_int.LowPart = ft.dwLowDateTime;
	return u_int.QuadPart / 10 / 1000 / TIME_SLOT_LEN;  // ns to ms
}


struct timer_node
{
	void* u;
	timer_call call;
	struct timer_node * list_prev;
	struct timer_node * list_next;
	struct timer_node * hash_prev;
	struct timer_node * hash_next;
	uint32_t expire;
	uint32_t id;
};

struct core_timer
{
	struct timer_node n[4][TIME_LEVEL];
	struct timer_node t[TIME_NEAR];
	uint32_t index;
	uint64_t timer_tick;
	uint32_t time;
	struct timer_node* freenode;
	uint32_t free_cnt;
	uint32_t use_cnt;
	struct timer_node h[TIMER_HASH_SIZE];
};

struct core_timer * coretimer_new(void)
{
	int i, j;
	struct timer_node* r;
	struct core_timer * ret = (struct core_timer *)malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));
	for (i = 0; i < 3; i++) {
		for (j = 0; j <TIME_LEVEL; j++) {
			r = &ret->n[i][j];
			r->list_next = r;
			r->list_prev = r;
			r->hash_next = NULL;
			r->hash_prev = NULL;
		}
	}
	for (i = 0; i < TIME_NEAR; i++) {
		r = &ret->t[i];
		r->list_next = r;
		r->list_prev = r;
		r->hash_next = NULL;
		r->hash_prev = NULL;
	}
	for (i = 0; i < TIMER_HASH_SIZE; i++) {
		r = &ret->h[i];
		r->hash_next = r;
		r->hash_prev = r;
	}
	ret->timer_tick = gettime();
	return ret;
}
void coretimer_delete(struct core_timer *t)
{
	int i;
	struct timer_node* node, *next;
	for (i = 0; i < TIMER_HASH_SIZE; i++) {
		node = t->h[i].hash_next;
		while (node) {
			next = node->hash_next;
			coretimer_del(t, node->id);
			node = next;
		}
	}
	node = t->freenode;
	while (node) {
		next = node->list_next;
		free(node);
		node = next;
	}
	free(t);
}

static struct timer_node* timernode_alloc(struct core_timer *timer)
{
	struct timer_node* node = timer->freenode;
	if (node) {
		timer->freenode = node->list_next;
		--timer->free_cnt;
	} else {
		node = (struct timer_node*)malloc(sizeof(*node));
	}
	++timer->use_cnt;
	return node;
}
static void timernode_dealloc(struct core_timer *timer, struct timer_node* node)
{
	uint32_t use = (--timer->use_cnt) >> 3;
	use = use ? use : 1;
	if (timer->free_cnt >= use)
		free(node);
	else {
		node->list_next = timer->freenode;
		timer->freenode = node;
		++timer->free_cnt;
	}
}


static void list_add(struct timer_node* list, struct timer_node* r)
{
	assert(!r->list_prev);//, "may be add repeated");
	assert(!r->list_next);//, "may be add repeated");
	r->list_prev = list->list_prev;
	r->list_prev->list_next = r;
	r->list_next = list;
	r->list_next->list_prev = r;
}

static void delfrom_list(struct timer_node* r)
{
	assert(r->hash_prev);//, "may be not in list");
	assert(r->hash_next);//, "may be not in list");
	r->list_prev->list_next = r->list_next;
	r->list_next->list_prev = r->list_prev;
	r->list_prev = NULL;
	r->list_next = NULL;
}

static void addto_hash(struct timer_node hash[], struct timer_node* r)
{
	int h;
	assert(!r->hash_prev);//, "may be add repeated");
	assert(!r->hash_next);//, "may be add repeated");
	h = HASH(r->id);
	r->hash_next = hash[h].hash_next;
	r->hash_next->hash_prev = r;
	r->hash_prev = &hash[h];
	r->hash_prev->hash_next = r;
}
static void delfrom_hash(struct timer_node* r)
{
	assert(r->hash_prev);//, "may be not in hash");
	assert(r->hash_next);//, "may be not in hash");
	r->hash_prev->hash_next = r->hash_next;
	r->hash_next->hash_prev = r->hash_prev;
	r->hash_prev = NULL;
	r->hash_next = NULL;
}

struct timer_node* list_clear(struct timer_node* list)
{
	struct timer_node* node = list->list_next;
	if (node == list) return NULL;
	node->list_prev = NULL;
	list->list_prev->list_next = NULL;
	list->list_prev = list;
	list->list_next = list;
	return node;
}

static void addto_list(struct core_timer *timer, struct timer_node * r)
{
	uint32_t expire = r->expire;
	uint32_t current_time = timer->time;
	if (expire < current_time) {
		assert(0);
	}
	if ((expire | TIME_NEAR_MASK) == (current_time | TIME_NEAR_MASK))
		list_add(&timer->t[expire &TIME_NEAR_MASK], r);
	else {
		int i = 0;
		uint32_t mask = TIME_NEAR << 6;
		for (; i < 3; i++) {
			if ((expire | (mask - 1)) == (current_time | (mask - 1)))
				break;
			mask <<= 6;
		}
		list_add(&timer->n[i][(expire >> (8 + i * 6))&TIME_LEVEL_MASK], r);
	}
}

uint32_t coretimer_add(struct core_timer *t, timer_call call, void* ctx, uint32_t wait)
{
	struct timer_node * r = timernode_alloc(t);
	r->call = call;
	r->u = ctx;
	r->id = ++t->index;
	wait = wait / TIME_SLOT_LEN;
	wait = wait ? wait : 1;
	r->expire = t->time + wait;
	r->hash_next = NULL;
	r->hash_prev = NULL;
	r->list_next = NULL;
	r->list_prev = NULL;

	addto_list(t, r);
	addto_hash(t->h, r);

	return r->id;
}

void coretimer_del(struct core_timer *t, uint32_t id)
{
	struct timer_node *r, *head;
	int hash = HASH(id);
	for (head = &t->h[hash], r = head->hash_next; r != head; r = r->hash_next) {
		if (r->id == id) {
			delfrom_list(r);
			delfrom_hash(r);
			timernode_dealloc(t, r);
			break;
		}
	}
}

static void timer_execute(struct core_timer *timer, struct iocp* io)
{
	//LOCK
	int idx = timer->time & TIME_NEAR_MASK;
	struct timer_node* r = list_clear(&timer->t[idx]);
	//UNLOCK
	while (r) {
		struct timer_node* n = r->list_next;
		//LOCK
		delfrom_hash(r);
		//UNLOCK
		r->call(io, r->u);
		timernode_dealloc(timer,r);
		r = n;
	}
}

static void list_move(struct core_timer* timer, int level, int idx)
{
	struct timer_node* r = list_clear(&timer->n[level][idx]);
	while (r) {
		struct timer_node* n = r->list_next;
		r->list_prev = NULL;
		r->list_next = NULL;
		addto_list(timer,r);
		r = n;
	}
}

static void timer_shift(struct core_timer* timer)
{
	int mask = TIME_NEAR;
	//LOCK
	uint32_t ct = ++timer->time;
	if (ct == 0) {
		list_move(timer, 3, 0);
	} else {
		uint32_t t = ct >> 8;
		int i = 0;

		while ((ct & (mask - 1)) == 0) {
			int idx = t & TIME_LEVEL_MASK;
			if (idx != 0) {
				list_move(timer, i, idx);
				break;
			}
			mask <<= 6;
			t >>= 6;
			++i;
		}
	}
	//UNLOCK
}

static void timer_tick(struct core_timer* timer, struct iocp* io)
{
	timer_execute(timer,io);
	timer_shift(timer);
	timer_execute(timer, io);
}

void coretimer_update(struct core_timer* timer, struct iocp* io)
{
	uint32_t diff, i;
	uint64_t now_tick = gettime();

	//LOCK(timer)

	if (now_tick <  timer->timer_tick) {
		diff = (uint32_t)(_UI64_MAX - timer->timer_tick + now_tick);
		fprintf(stderr, "timer %p rewind from %lld to %lld\n", timer, now_tick, timer->timer_tick);
	} else if (now_tick != timer->timer_tick) {
		diff = (uint32_t)(now_tick - timer->timer_tick);
	}
	if (diff > 0) {
		timer->timer_tick = now_tick;
		//LOCK(timer)
		for (i = 0; i<diff; i++) {
			timer_tick(timer, io);
		}
	} else {
		//LOCK(timer)
	}
}
