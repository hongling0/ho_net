#include "timer.h"

#define HASH(id) id % TIMER_HASH_SIZE

namespace frame
{
	timer::timer()
	{
		memset(this, 0, sizeof(*this));
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < (2 ^ 6); j++)
			{
				timer_node* r = &n[i][j].head;
				r->list_next = r;
				r->list_prev = r;
				r->hash_next = NULL;
				r->hash_prev = NULL;
			}
		}
		for (int i = 0; i < (2 ^ 8); i++)
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
	}
	uint32_t timer::add(timer_call call, timer_context ctx, uint32_t wait)
	{
		timer_node * r = (timer_node*)malloc(sizeof(timer_node));
		r->call = call;
		r->u = ctx;
		r->id = ++index;
		r->expire = timer_tick + wait;

		uint32_t time = r->expire;
		uint32_t current_time = timer_tick;
		if ((time | (2 ^ 8 - 1)) == (current_time | (2 ^ 8 - 1)))
			addto_list(r, &t[time & (2 ^ 8 - 1)]);
		else
		{
			int i = 0;
			uint32_t mask = 2 ^ 8;
			for (; i < 3; i++)
			{
				mask <<= 6;
				if ((time | (mask - 1)) == (current_time | (mask - 1)))
					break;
			}
			addto_list(r, &n[i][(time & (8 + i * 6))&(2 ^ 6 - 1)]);
		}
		addto_hash(r);
		return r->id;
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
				free(r);
				break;
			}
		}
	}

	void timer::execute()
	{
		int idx = timer_tick & (2 ^ 8);
		while (true)
		{
			timer_node* r = t[idx].head.list_next;
			if (r == &t[idx].head)
				break;
			r->call(r->u);
		}
	}

	void timer::shift()
	{
		int mask = 2 ^ 8;
		uint32_t ct = ++timer_tick;
		if (ct == 0)
		{
			// move_list(T, i, idx);
		}
		else
		{
			uint32_t time = ct >> 8;
			int i = 0;

			while ((ct & (mask - 1)) == 0)
			{
				int idx = time & (2 ^ 6 - 1);
				if (idx != 0) {
					//move_list(T, i, idx);
					break;
				}
				mask <<= 6;
				time >>= 6;
				++i;
			}
		}
	}

	void timer::tick()
	{
		execute();
		shift();
		execute();
	}

	void timer::update()
	{
		// todo
	}

	void timer::addto_list(timer_node* r, timer_list* list)
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
		r->hash_prev = NULL;
		r->hash_next = NULL;
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