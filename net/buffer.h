#pragma once
#include "typedef.h"
#include "atomic.h"

namespace frame
{
	class ring_buffer
	{
	public:
		ring_buffer();
		ring_buffer(size_t len);
		~ring_buffer();
		void init(size_t sz);
		bool empty() const;
		bool write(char* data, size_t sz);
		bool writebuffer(char** ptr, size_t * sz) const;
		bool write_ok(size_t sz);
		bool read(char* data, size_t sz);
		bool readbuffer(char** ptr, size_t* sz) const;
		bool read_ok(size_t sz);
		void clean();
		ring_buffer* next;
	private:
		char * buffer;
		size_t capacity;
		atomic_type _read;
		atomic_type _write;
	};

	class buffer_head
	{
	protected:
		ring_buffer* head;
		ring_buffer* tail;
	public:
		buffer_head()
		{
			head = 0;
			tail = 0;
		}
		~buffer_head()
		{
			clean();
		}
		void clean()
		{
			ring_buffer* b = head;
			while (b)
			{
				head = b->next;
				delete b;
			}
			tail = 0;
			head = 0;
		}
		void push(ring_buffer* b)
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
			ring_buffer* b = head;
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