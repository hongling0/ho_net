#pragma once
#include "typedef.h"
#include "lock.h"

#define TIMER_HASH_SIZE 1024
#define TIME_NEAR (1<<8)
#define TIME_LEVEL (1 << 6)

namespace frame
{
	union timer_context
	{
		void * p;
		int i;
		double d;
	};
	typedef void(*timer_call)(timer_context u);
	class timer : public sys::spinlock
	{
	private:
		struct timer_node
		{
			timer_context u;
			timer_call call;
			timer_node * list_prev;
			timer_node * list_next;
			timer_node * hash_prev;
			timer_node * hash_next;
			uint32_t expire;
			uint32_t id;
		};
		struct timer_list
		{
			timer_node head;
		};
	public:
		timer();
		uint32_t add(timer_call call, timer_context ctx, uint32_t wait);
		void del(uint32_t id);
		void update();
	protected:
		void execute();
		void shift();
		void tick();
		
		void addto_list(timer_node* r);
		void delfrom_list(timer_node* r);
		void addto_hash(timer_node* r);
		void delfrom_hash(timer_node* r);

		timer_node* list_clear(timer_list* list);
		void list_move(int level, int idx);
		void list_add(timer_list* list, timer_node* r);

		timer_node* alloc();
		void dealloc(timer_node* n);

	private:
		timer_list n[4][TIME_LEVEL];
		timer_list t[TIME_NEAR];
		timer_node h[TIMER_HASH_SIZE];
		uint32_t index;
		uint64_t timer_tick;
		uint32_t time;
		timer_node* freenode;
		uint32_t free_cnt;
		uint32_t use_cnt;
	};
}