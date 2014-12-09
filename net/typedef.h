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



#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)