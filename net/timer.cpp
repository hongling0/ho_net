#include <limits.h>
#include "timer.h"

#define HASH(id) id % TIMER_HASH_SIZE

#define TIME_NEAR_MASK (TIME_NEAR-1)
#define TIME_LEVEL_MASK (TIME_LEVEL-1)
#define TIME_SLOT_LEN 100

namespace frame
{
	static uint64_t gettime()
	{
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		ULARGE_INTEGER u_int;
		u_int.HighPart = ft.dwHighDateTime;
		u_int.LowPart = ft.dwLowDateTime;
		return u_int.QuadPart / 10 / TIME_SLOT_LEN;
	}

	timer::timer()
	{
		memset(this, 0, sizeof(*this));
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j <TIME_LEVEL; j++)
			{
				timer_node* r = &n[i][j].head;
				r->list_next = r;
				r->list_prev = r;
				r->hash_next = NULL;
				r->hash_prev = NULL;
			}
		}
		for (int i = 0; i < TIME_NEAR; i++)
		{
			timer_node* r = &t[i].head;
			r->list_next = r;
			r->list_prev = r;
			r->hash_next = NULL;
			r->hash_prev = NULL;
		}
		for (int i = 0; i < TIMER_HASH_SIZE; i++)
		{
			timer_node* r = &h[i];
			r->hash_next = r;
			r->hash_prev = r;
		}
		timer_tick = gettime();
		time = 0;
		index = 0;
	}

	void timer::addto_list(timer_node * r)
	{
		uint32_t expire = r->expire;
		uint32_t current_time = time;
		if ((expire | TIME_NEAR_MASK) == (current_time | TIME_NEAR_MASK))
			list_add(&t[expire &TIME_NEAR_MASK],r);
		else
		{
			int i = 0;
			uint32_t mask = TIME_NEAR;
			for (; i < 3; i++)
			{
				mask <<= 6;
				if ((expire | (mask - 1)) == (current_time | (mask - 1)))
					break;
			}
			list_add(&n[i][(expire & (8 + i * 6))&TIME_LEVEL_MASK], r);
		}
	}
	uint32_t timer::add(timer_call call, timer_context ctx, uint32_t wait)
	{
		timer_node * r =alloc();
		r->call = call;
		r->u = ctx;
		r->id = ++index;
		wait = wait / TIME_SLOT_LEN;
		wait = wait ? wait : 1;
		r->expire = time + wait/TIME_SLOT_LEN;
		r->hash_next = NULL;
		r->hash_prev = NULL;
		r->list_next = NULL;
		r->list_prev = NULL;

		addto_list(r);
		addto_hash(r);
		
		return r->id;
	}

	timer::timer_node* timer::alloc()
	{
		timer_node* r = freenode;
		if (r)
		{
			freenode = freenode->list_next;
			--free_cnt;
		}
		else
			r = (timer_node*) malloc (sizeof(timer_node));
		++use_cnt;
		return r;
	}
	void timer::dealloc(timer_node* n)
	{
		uint32_t use = (--use_cnt) / 10;
		use = use ? use : 1;
		if (free_cnt >= use)
			free(n);
		else
		{
			n->list_next = freenode;
			freenode = n;
			++free_cnt;
		}
	}

	void timer::del(uint32_t id)
	{
		int hash = HASH(id);
		timer_node *r, *head;
		for (head = &h[hash], r = head->hash_next; r != head; r = r->hash_next)
		{
			if (r->id == id)
			{
				delfrom_list(r);
				delfrom_hash(r);
				dealloc(r);
				break;
			}
		}
	}

	void timer::execute()
	{
		lock();
		int idx = time & TIME_NEAR_MASK;
		timer_node* r = list_clear(&t[idx]);
		unlock();
		while (r)
		{
			timer_node* n = r->list_next;
			lock();
			delfrom_hash(r);
			unlock();
			r->call(r->u);
			dealloc(r);
			r = n;
		}
	}

	void timer::list_move(int level, int idx)
	{
		timer_node* r = list_clear(&n[level][idx]);
		while (r)
		{
			timer_node* n = r->list_next;
			n->list_prev = NULL;
			r->list_next = NULL;
			addto_list(r);
			r = n;
		}
	}

	timer::timer_node* timer::list_clear(timer_list* list)
	{
		timer_node* node = list->head.list_next;
		if (node == &list->head) return NULL;
		node->list_prev = NULL;
		list->head.list_prev->list_next = NULL;
		list->head.list_prev = &list->head;
		list->head.list_next = &list->head;
		return node;
	}

	void timer::shift()
	{
		int mask = TIME_NEAR;
		lock();
		uint32_t ct = ++time;
		if (ct == 0)
		{
			list_move(3, 0);
		}
		else
		{
			uint32_t time = ct >> 8;
			int i = 0;

			while ((ct & (mask - 1)) == 0)
			{
				int idx = time & TIME_LEVEL_MASK;
				if (idx != 0) {
					list_move(i, idx);
					break;
				}
				mask <<= 6;
				time >>= 6;
				++i;
			}
		}
		unlock();
	}

	void timer::tick()
	{
		execute();
		shift();
		execute();
	}

	void timer::update()
	{
		uint32_t diff = 0;
		uint64_t now_tick = gettime();

		lock();

		if (now_tick < timer_tick)
		{
			diff = (uint32_t)(_UI64_MAX - timer_tick + now_tick);
			fprintf(stderr, "timer %p rewind from %lld to %lld\n", this, now_tick, timer_tick);
		}
		else if (now_tick != timer_tick)
		{
			diff = (uint32_t)(now_tick - timer_tick);
		}
		if (diff > 0)
		{
			timer_tick = now_tick;
			unlock();
			for (uint32_t i = 0; i<diff; i++)
			{
				tick();
			}
		}
		else
			unlock();
	}

	void timer::list_add(timer_list* list,timer_node* r)
	{
		ASSERT(!r->list_prev, "may be add repeated");
		ASSERT(!r->list_next, "may be add repeated");
		r->list_prev = list->head.list_prev;
		r->list_prev->list_next = r;
		r->list_next = &list->head;
		r->list_next->list_prev = r;
	}

	void timer::delfrom_list(timer_node* r)
	{
		ASSERT(r->hash_prev, "may be not in list");
		ASSERT(r->hash_next, "may be not in list");
		r->list_prev->list_next = r->list_next;
		r->list_next->list_prev = r->list_prev;
		r->list_prev = NULL;
		r->list_next = NULL;
	}

	void timer::addto_hash(timer_node* r)
	{
		ASSERT(!r->hash_prev, "may be add repeated");
		ASSERT(!r->hash_next, "may be add repeated");
		int hash = HASH(r->id);
		r->hash_next = h[hash].hash_next;
		r->hash_next->hash_prev = r;
		r->hash_prev = &h[hash];
		r->hash_prev->hash_next = r;
	}
	void timer::delfrom_hash(timer_node* r)
	{
		ASSERT(r->hash_prev, "may be not in hash");
		ASSERT(r->hash_next, "may be not in hash");
		r->hash_prev->hash_next = r->hash_next;
		r->hash_next->hash_prev = r->hash_prev;
		r->hash_prev = NULL;
		r->hash_next = NULL;
	}
}