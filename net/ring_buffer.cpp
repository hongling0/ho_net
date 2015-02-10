#include "buffer.h"

#define MIN(a,b) ((a)<(b))?(a):(b)

namespace frame
{
	ring_buffer::ring_buffer()
	{
		buffer = 0;
		next = 0;
		init(128);
	}
	ring_buffer::ring_buffer(size_t len)
	{
		buffer = 0;
		next = 0;
		init(len);
	}
	ring_buffer::ring_buffer(char* data, size_t len)
	{
		buffer = 0;
		next = 0;
		init(len);
		if (len > 0)
			write(data, len);
	}
	ring_buffer::~ring_buffer()
	{
		free(buffer);
	}
	void ring_buffer::init(size_t sz)
	{
		if (sz != 0 && ((sz&(sz - 1)) != 0)) {
			sz = ((sz >> 1) + 1) << 1;
		}
		free(buffer);
		buffer = 0;
		size = 0;
		out = 0;
		in = 0;
		if (sz>0) {
			buffer = (char*)malloc(sz);
			size = sz;
		}
	}
	bool ring_buffer::empty() const
	{
		return in == out;
	}
	bool ring_buffer::write(char* data, size_t sz)
	{
		size_t all = size - (in - out);
		if (all < sz) return false;

		size_t l = MIN(sz, size - (in&(size - 1)));
		memcpy(buffer + (in&(size - 1)), data, l);
		memcpy(buffer, data + l, sz - l);

		in += sz;
		return true;
	}
	bool ring_buffer::writebuffer(char** ptr, size_t * sz) const
	{
		size_t all = size - (in - out);
		if (all == 0) return false;
		size_t wp = in&(size - 1);
		size_t rp = out&(size - 1);
		*ptr = &buffer[wp];
		if (wp >= rp)
			*sz = size - wp - 1;
		else
			*sz = rp - wp;
		return true;
	}
	bool ring_buffer::write_ok(size_t sz)
	{
		assert(sz <= size - (in - out));
		in += sz;
		return true;
	}
	bool ring_buffer::read(char* data, size_t sz)
	{
		size_t all = in - out;
		if (all < sz) return false;

		size_t l = MIN(sz, size - (out&(size - 1)));
		memcpy(data, buffer + (out&(size - 1)), l);
		memcpy(data + l, buffer, sz - l);

		out += sz;
		return true;
	}
	bool ring_buffer::readbuffer(char** ptr, size_t* sz) const
	{
		size_t all = in - out;
		if (all == 0) return false;
		size_t wp = in%size;
		size_t rp = out %size;
		*ptr = &buffer[rp];
		if (wp > rp)
			*sz = wp - rp;
		else
			*sz = size - rp;
		return true;
	}
	bool ring_buffer::read_ok(size_t sz)
	{
		assert((int)sz <= (in - out));
		out += sz;
		return true;
	}
	void ring_buffer::clean()
	{
		out = 0;
		in = 0;
	}

}