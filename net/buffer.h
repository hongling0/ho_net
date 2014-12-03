#pragma once
#include "typedef.h"
#include "atomic.h"

namespace frame
{
	template<typename T>
	class buffer
	{
	protected:
		uint32_t head;
		uint32_t tail;
		T ** queue;
		T *list;
		uint32_t cap
	public:
		buffer_head()
		{
			cap = 8;
			queue = malloc(cap * sizeof(T *));
			memset(queue, 0, sizeof(T *) * cap);
			head = 0;
			tail = 0;
			list = 0;
		}
		~buffer_head()
		{
			clean();
		}
		void clean()
		{
			T* b = head;
			while (b)
			{
				head = b->next;
				delete b;
			}
			tail = 0;
			head = 0;
		}
		void push(T* b)
		{
			while (b)	// reverse list hear
			{
				ring_buffer* n = b->next;
				if (tail)
				{
					head = tail = n;
				}
				else
				{
					tail->next = b;
					tail = b;
				}
				b = n;
			}
		}
		ring_buffer* get_head()
		{
			T* b = head;
			while (b&&b->empty())
			{
				head = b->next;
				delete b;
			}
			if (!b)
				tail = 0;
			return b;
		}
	};
}