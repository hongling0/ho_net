#pragma once

//#include <Windows.h>
//#include <winsock.h>

#include "typedef.h"
#include "buffer.h"
#include "logic.h"


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

	class socket;
	class socket_server;

	struct io_event : public event_head
	{
		void * u;
		WSABUF wsa;
		char buf[2*(sizeof(sockaddr_in) + 16)];
		atomic_type ready;
	};

	typedef bool(*protocol_recv)(socket* s,errno_type e);
	typedef bool(*protocol_accept)(socket* s, socket* n, errno_type e);
	typedef bool(*protocol_connect)(socket* s, errno_type e);

	struct socket_opt
	{
		protocol_recv recv;
		union
		{
			protocol_accept accept;
			protocol_connect connect;
		};
	};

	struct message
	{
		uint8_t type;
		void* data;
	};

	class socket
	{
	public:
		socket()
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
		ring_buffer wb;
		ring_buffer rb;
		socket_opt opt;
	};

}