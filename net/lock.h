#pragma once


#include "typedef.h"

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