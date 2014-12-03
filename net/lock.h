#pragma once


#include "typedef.h"
#include "atomic.h"

namespace sys
{
	struct mutex
	{
		mutex()
		{
			InitializeCriticalSection(&cs);
		}
		void lock()
		{
			EnterCriticalSection(&cs);
		}
		void unlock()
		{
			LeaveCriticalSection(&cs);
		}
	private:
		RTL_CRITICAL_SECTION cs;
	};

	struct spinlock
	{
		spinlock()
		{
			_lock = 0;
		}
		void lock()
		{
			while (InterlockedCompareExchange(&_lock,1, 0));
		}
		void unlock()
		{
			InterlockedExchange(&_lock, 0);
		}
	private:
		LONG _lock;
	};

	struct rwlock
	{
		rwlock()
		{
			_read = 0;
			_write = 0;
		}
		void rlock()
		{
			for (;;)
			{
				while (_write) MemoryBarrier();
				InterlockedIncrement(&_read);
				if (_write) {
					InterlockedDecrement(&_read);
				}
				else {
					break;
				}
			}
		}
		void wlock()
		{
			while (InterlockedCompareExchange(&_write, 1, 0));
			while (_read) {
				MemoryBarrier();
			}
		}
		void runlock()
		{
			InterlockedDecrement(&_read);
		}
		void wunlock()
		{
			InterlockedExchange(&_write, 0);
		}
	private:
		LONG _read;
		LONG _write;
	};

	struct srwlock
	{
		srwlock()
		{
			InitializeSRWLock(&l);
		}
		void rlock()
		{
			AcquireSRWLockShared(&l);
		}
		void wlock()
		{
			AcquireSRWLockExclusive(&l);
		}
		void runlock()
		{
			ReleaseSRWLockShared(&l);
		}
		void wunlock()
		{
			ReleaseSRWLockExclusive(&l);
		}
	private:
		SRWLOCK l;
	};

	template<typename T>
	struct lock_guard
	{
		lock_guard(T& _t) :t(_t)
		{
			t.lock();
		}
		~lock_guard()
		{
			t.unlock();
		}
	private:
		T& t;
	};
}