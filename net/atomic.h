#pragma once
#include <stdint.h>
#if defined(_MSC_VER)
#include <Windows.h>
#endif

#if defined(_MSC_VER)

typedef volatile long atomic_type;
#define sync_add_and_fetch(l,v) InterlockedAdd(l,v)
#define sync_dec_and_fetch(l,v) InterlockedAdd(l,v)
#define sync_bool_compare_and_swap(l,v,c) InterlockedCompareExchange(l,v,c)

 
#define atomic_cmp_set(lock, old, set)	((uint32_t) InterlockedCompareExchange((long *) lock, set, old) == old)
#define atomic_fetch_add(value, add) InterlockedExchangeAdd((long *)value, add)
#define atomic_add_fetch(value, add) InterlockedAdd(value, add)
#define atomic_dec_fetch(value, add) InterlockedAdd(value, 0-add)
#define memory_barrier()

#else

typedef volatile uint32_t atomic_type;

#define atomic_cmp_set(lock,old,set)  __sync_bool_compare_and_swap(lock, old, set)
#define atomic_fetch_add(value, add) __sync_fetch_and_add(value, add)
#define atomic_add_fetch(value, add) __sync_add_and_fetch(value, add)
#define memory_barrier()        __sync_synchronize()

#endif
