#include "buffer.h"
#include "typedef.h"
#include "atomic.h"

namespace frame
{
	buffer_head::buffer_head()
	{
		head = NULL;
		tail = NULL;
	}
	buffer_head::~buffer_head()
	{
		clean();
	}
	void buffer_head::clean()
	{
		ring_buffer* b = head;
		while (b)
		{
			head = b->next;
			delete b;
		}
		tail = NULL;
	}
	void buffer_head::push_front(ring_buffer* b)
	{
		sys::lock_guard<sys::spinlock> guard(lock);
		b->next = head;
		if (head == NULL)
			tail = head = b;
		else
			head = b;
	}
	void buffer_head::push_back(ring_buffer* b)
	{
		sys::lock_guard<sys::spinlock> guard(lock);
		if (head == NULL)
			tail = head = b;
		else
		{
			tail->next = b;
			tail = b;
		}
	}
	ring_buffer* buffer_head::pop_front()
	{
		sys::lock_guard<sys::spinlock> guard(lock);
		ring_buffer* ret = head;
		if (head) head = head->next;
		if (head == NULL) tail = NULL;
		return ret;
	}
}
