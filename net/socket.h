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
		io_event()
		{
			u = 0;
			b = 0;
			ready = 0;
		}
		~io_event()
		{
			delete b;
		}
		void * u;
		char buf[2*(sizeof(sockaddr_in) + 16)];
		ring_buffer* b;
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
		~socket()
		{
			reset();
		}
		void reset()
		{
			if (fd != INVALID_SOCKET)
				closesocket(fd);
			id = 0;
			fd = INVALID_SOCKET;
			type = SOCKET_TYPE_RESERVE;
			memset(&op, 0, sizeof(op));
			memset(&local, 0, sizeof(local));
			memset(&remote, 0, sizeof(remote));
			pending = 0;
			closing = 0;
			w_byte = 0;
			r_byte = 0;
			wb.clean();
			rb.clean();
		}
		int id;
		int logic;
		SOCKET fd;
		atomic_type type;
		io_event op[socket_ev_max];
		atomic_type pending;
		atomic_type closing;
		buffer_head wb;
		buffer_head rb;
		uint32_t w_byte;
		uint32_t r_byte;
		socket_opt opt;
		SOCKADDR_IN local;
		SOCKADDR_IN remote;
		socket_server& io;
	};

}