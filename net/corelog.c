#include <Windows.h>

#include "corelock.h"

void rwlock_init(struct rwlock* lock)
{
	lock->read = 0;
	lock->write = 0;
}

void rwlock_rlock(struct rwlock* lock)
{
	for (;;) {
		while (lock->write) {}
		InterlockedIncrement(&lock->read);
		if (lock->write) {
			InterlockedDecrement(&lock->read);
		}
		else {
			break;
		}
	}
}

void rwlock_wlock(struct rwlock* lock)
{
	while (InterlockedCompareExchange(&lock->write, 1, 0));
	while (lock->read) {}
}

void rwlock_runlock(struct rwlock* lock)
{
	InterlockedDecrement(&lock->read);
}

void rwlock_wunlock(struct rwlock* lock)
{
	InterlockedExchange(&lock->write, 0);
}
