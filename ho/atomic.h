#pragma once

#ifdef _WIN32

#include <Windows.h>

#define sync_add_and_fetch(l,v) InterlockedAdd(l,v)
#define sync_bool_compare_and_swap(l,v,c) InterlockedCompareExchange(l,v,c)


#else



#endif
