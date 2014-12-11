#pragma once
#ifdef _DEBUG
//#define _CRTDBG_MAP_ALLOC
#endif
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

#ifdef _DEBUG
//#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"mswsock.lib")

#include "atomic.h"

typedef HANDLE handle_type; 
typedef SOCKET socket_type;
typedef unsigned long errno_type;

#ifdef _DEBUG
#define ASSERT(exp, ...) do{ if (!(exp)) { fprintf(stderr, __VA_ARGS__);assert((exp)) } } while (0);
#else
#define ASSERT(exp, ...) ((void) 0)
#endif

