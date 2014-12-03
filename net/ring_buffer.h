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
}