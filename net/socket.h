#pragma once

//#include <Windows.h>
//#include <winsock.h>

#include "typedef.h"
#include "buffer.h"
#include "logic.h"
#include "socketinterface.h"


namespace frame
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

	enum socket_ev_type
	{
		socket_ev_read,
		socket_ev_write,
		socket_ev_max,
	};

	struct io_event : public event_head
	{
		void * u;
		char buf[2*(sizeof(sockaddr_in) + 16)];
		atomic_type ready;
	};

	class socket_server;
	class socket
	{
	public:
		socket(socket_server& _io) :io(_io)
		{
			fd = INVALID_SOCKET;
			reset();
		}
		void reset()
		{
			if (fd != INVALID_SOCKET)
				closesocket(fd);
			id = 0;
			fd = INVALID_SOCKET;
			type = SOCKET_TYPE_INVALID;
			memset(&op, 0, sizeof(op));
			pending = 0;
			wb.clean();
			rb.clean();
		}
		int id;
		int logic;
		SOCKET fd;
		atomic_type type;
		io_event op[socket_ev_max];
		atomic_type pending;
		buffer_head wb;
		buffer_head  rb;
		socket_opt opt;
		socket_server& io;
	};

}