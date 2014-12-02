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

		}
		void unlock()
		{

		}
	private:
		atomic_type _lock;
	};

	struct rwlock
	{
		rwlock()
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
		void unlock()
		{
			ReleaseSRWLockShared(&l);
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