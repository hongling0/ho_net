#pragma once
#define _CRTDBG_MAP_ALLOC
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <winsock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <winsock.h>
#include <process.h> 
#include <crtdbg.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"mswsock.lib")

#include "atomic.h"

typedef HANDLE handle_type; 
typedef SOCKET socket_type;
typedef unsigned long errno_type;




#define CHECKMEMORY()     do{ \
                            int nFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ); \
                            nFlag |= _CRTDBG_LEAK_CHECK_DF; \
                            _CrtSetDbgFlag( nFlag ); \
                        } while(0);

inline void * __cdecl operator new(unsigned int size,
	const char *file, int line)
{
	CHECKMEMORY()
	return ::operator new(size, _NORMAL_BLOCK, file, line);
};

#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW