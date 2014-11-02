#pragma once

#include <Windows.h>
#include <winsock.h>

#include "../typedef.h"




namespace net
{
	struct write_buffer {
		struct write_buffer * next;
		char *ptr;
		int sz;
		void *buffer;
	};

	struct wb_list {
		struct write_buffer * head;
		struct write_buffer * tail;
	};

	enum socket_type
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
	};

	class iocp;
	class socket;
	struct io_event;
	typedef void(*ioevent_call)(iocp*, io_event*,socket*, errno_t, size_t);
	struct io_event 
	{
		OVERLAPPED op;
		ioevent_call call;
		void * u;
		WSABUF wsa;
		char* buf;
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
		int id;
		SOCKET fd;
		atomic_type type;
		uint64_t wb_size;
		io_event op[3];
		struct wb_list wb;
	};

}