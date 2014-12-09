#include "buffer.h"

namespace frame
{
	ring_buffer::ring_buffer()
	{
		memset(this, 0, sizeof(*this));
		init(4096);
	}
	ring_buffer::ring_buffer(size_t len)
	{
		size_t sz = 2;
		while (sz < len) sz *= 2;
		memset(this, 0, sizeof(*this));
		if (len > 0)
			init(sz);
	}
	ring_buffer::ring_buffer(char* data, size_t len)
	{
		size_t sz = 2;
		while (sz < len) sz *= 2;
		memset(this, 0, sizeof(*this));
		if (len > 0)
			init(sz);
		write(data, sz);
	}
	ring_buffer::~ring_buffer()
	{
		init(0);
	}
	void ring_buffer::init(size_t sz)
	{
		delete[] buffer;
		memset(this, 0, sizeof(*this));
		if (sz>0)
		{
			buffer = new char[sz];
			memset(buffer, 0, sz);
			capacity = sz;
		}
	}
	bool ring_buffer::empty() const
	{
		return _write == _read;
	}
	bool ring_buffer::write(char* data, size_t sz)
	{
		size_t all = capacity - (_write - _read);
		if (all < sz) return false;
		size_t wp = _write%capacity;
		size_t rp = _read %capacity;
		_write += sz;
		if (wp>rp)
		{
			size_t w = capacity - wp;
			w = w >= sz ? sz : w;
			sz -= w;
			memcpy(&buffer[wp], data, w);
			if (sz > 0)
				memcpy(&buffer[0], data + w, sz);
		}
		else
			memcpy(&buffer[wp], data, sz);
		return true;
	}
	bool ring_buffer::writebuffer(char** ptr, size_t * sz) const
	{
		size_t all = capacity - (_write - _read);
		if (all == 0) return false;
		size_t wp = _write%capacity;
		size_t rp = _read %capacity;
		*ptr = &buffer[wp];
		if (wp >= rp)
			*sz = capacity - wp - 1;
		else
			*sz = rp - wp;
		return true;
	}
	bool ring_buffer::write_ok(size_t sz)
	{
		assert(sz <= capacity - (_write - _read));
		_write += sz;
		return true;
	}
	bool ring_buffer::read(char* data, size_t sz)
	{
		size_t all = _write - _read;
		if (all < sz) return false;
		size_t wp = _write%capacity;
		size_t rp = _read %capacity;
		_read += sz;
		if (rp>wp)
		{
			size_t r = capacity - rp;
			r = r >= sz ? sz : r;
			sz -= r;
			memcpy(data, &buffer[rp], r);
			if (sz>0)
				memcpy(data + sz, &buffer[0], sz);
		}
		else
			memcpy(data, &buffer[rp], sz);
		return true;
	}
	bool ring_buffer::readbuffer(char** ptr, size_t* sz) const
	{
		size_t all = _write - _read;
		if (all == 0) return false;
		size_t wp = _write%capacity;
		size_t rp = _read %capacity;
		*ptr = &buffer[rp];
		if (wp > rp)
			*sz = wp - rp;
		else
			*sz = capacity - rp;
		return true;
	}
	bool ring_buffer::read_ok(size_t sz)
	{
		assert((int)sz <= (_write - _read));
		_read += sz;
		return true;
	}
	void ring_buffer::clean()
	{
		_read = 0;
		_write = 0;
	}

}