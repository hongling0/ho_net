#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <Windows.h>
#include <winsock.h>
#include <WinSock2.h>

typedef volatile long atomic_type;
typedef HANDLE handle_type;
typedef SOCKET socket_type;
typedef unsigned long errno_type;
typedef WSABUF buf_type;