#pragma once

//#include <Windows.h>
//#include <winsock.h>

#include "typedef.h"
#include "buffer.h"



namespace net
{

	enum socket_type_enum
	{
		SOCKET_TYPE_INVALID = 0,
		SOCKET_TYPE_RESERVE,
		SOCKET_TYPE_PLISTEN,
		SOCKET_TYPE_LISTEN,
		SOCKET_TYPE_CONNECTING,
		SOCKET_TYPE_CONNECTED,
		SOCKET_TYPE_HALFCLOSE,
		SOCKET_TYPE_PACCEPT,
		SOCKET_TYPE_BIND,
	};

	enum ioevent_type
	{
		ioevent_read,
		ioevent_write,
		ioevent_max,
	};

	class iocp;
	class socket;
	struct io_event;
	typedef void(*ioevent_call)(iocp*, io_event*,socket*, errno_type, size_t);
	struct io_event 
	{
		OVERLAPPED op;
		ioevent_call call;
		void * u;
		WSABUF wsa;
		char* buf;
		atomic_type ready;
	};

	class socket
	{
	public:
		socket()
		{
			memset(this, 0, sizeof(*this));
			type = SOCKET_TYPE_INVALID;
			fd = INVALID_SOCKET;
		}
		void reset()
		{
			if (fd != INVALID_SOCKET)
				closesocket(fd);
			memset(this, 0, sizeof(*this));
			type = SOCKET_TYPE_INVALID;
			fd = INVALID_SOCKET;
		}
		int id;
		SOCKET fd;
		atomic_type type;
		io_event op[ioevent_max];
		atomic_type pending;
		ring_buffer wb;
		ring_buffer rb;
	};

}