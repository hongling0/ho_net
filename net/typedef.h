#pragma once
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
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"mswsock.lib")

#include "atomic.h"

typedef HANDLE handle_type;
typedef SOCKET socket_type;
typedef unsigned long errno_type;

extern	char * errno_str(errno_type e);