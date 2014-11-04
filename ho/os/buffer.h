#pragma once

//#include <Windows.h>
//#include <winsock.h>

#include "typedef.h"




namespace net
{

	class ring_buffer
	{
	public:
		ring_buffer()
		{
			memset(this, 0, sizeof(*this));
		}
		~ring_buffer()
		{
			init(0);
		}
		void init(size_t sz)
		{
			delete[] buffer;
			memset(this, 0, sizeof(*this));
			if (sz>0)
			{
				buffer = new char[sz];
				capacity = sz;
			}
		}
		bool empty() const
		{
			return _write == _read;
		}
		bool write(char* data, size_t sz)
		{
			size_t all = capacity - (_write - _read);
			if (all < sz) return false;
			_write += sz;
			size_t wp = _write%capacity;
			size_t rp = _read %capacity;
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
		bool writebuffer(char** ptr, size_t * sz) const
		{
			size_t all = capacity - (_write - _read);
			if (all = 0) return false;
			size_t wp = _write%capacity;
			size_t rp = _read %capacity;
			*ptr = &buffer[wp];
			if (wp>rp)
				*sz = capacity - wp;
			else
				*sz = rp - wp;
			return true;
		}
		bool write_ok(size_t sz)
		{
			assert(sz <= capacity - (_write - _read));
			_write += sz;
			return true;
		}
		bool read(char* data, size_t sz)
		{
			size_t all = _write - _read;
			if (all < sz) return false;
			_read += sz;
			size_t wp = _write%capacity;
			size_t rp = _read %capacity;
			if (rp>wp)
			{
				size_t r = capacity - rp;
				r = r >= sz ? sz : r;
				sz -= r;
				memcpy(data, &buffer[rp], r);
				if (sz>0)
					memcpy(data+sz, &buffer[0], sz);
			}
			else
				memcpy(data, &buffer[rp], sz);
			return true;
		}
		bool readbuffer(char** ptr, size_t* sz) const
		{
			size_t all = _write - _read;
			if (all ==0 ) return false;
			size_t wp = _write%capacity;
			size_t rp = _read %capacity;
			*ptr = &buffer[rp];
			if (wp > rp)
				*sz = wp - rp;
			else
				*sz = capacity - rp;
			return true;
		}
		bool read_ok(size_t sz)
		{
			assert(sz <= (_write - _read));
			_read += sz;
			return true;
		}
	private:
		char * buffer;
		size_t capacity;
		atomic_type _read;
		atomic_type _write;
	};
}