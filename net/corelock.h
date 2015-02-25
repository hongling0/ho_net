#ifndef CORE_LOCK_H
#define CORE_LOCK_H

#ifdef _cpluscplus
extern "C" {
#endif

	struct rwlock
	{
		volatile long read;
		volatile long write;
	};


	void rwlock_init(struct rwlock* lock);
	void rwlock_rlock(struct rwlock* lock);
	void rwlock_runlock(struct rwlock* lock);
	void rwlock_wlock(struct rwlock* lock);	
	void rwlock_wunlock(struct rwlock* lock);

#ifdef _cpluscplus
}
#endif

#endif //CORE_LOCK_H