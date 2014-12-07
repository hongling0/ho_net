#pragma once
#include "lock.h"
#include "ring_buffer.h"

namespace frame
{
	class buffer_head
	{
	protected:
		ring_buffer* head;
		ring_buffer* tail;
		sys::spinlock lock;
	public:
		buffer_head();
		~buffer_head();
		void clean();
		void push_front(ring_buffer* b);
		void push_back(ring_buffer* b);
		ring_buffer* pop_front();
	};
}