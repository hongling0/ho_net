#pragma once
#include "typedef.h"

#define TIMER_HASH_SIZE 4096

namespace frame
{
	union timer_context
	{
		void * p;
		int i;
		double d;
	};
	typedef void(*timer_call)(timer_context u);
	class timer
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
		void addto_list(timer_node* r, timer_list* list);
		void delfrom_list(timer_node* r);
		void addto_hash(timer_node* r);
		void delfrom_hash(timer_node* r);
	private:
		timer_list n[4][2 ^ 6];
		timer_list t[2 ^ 8];
		timer_node h[TIMER_HASH_SIZE];
		uint32_t index;
		uint32_t timer_tick;
	};
}