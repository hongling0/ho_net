#pragma once

//#include <Windows.h>
//#include <winsock.h>

#include "typedef.h"
#include "buffer.h"
#include "logic.h"


namespace frame
{
	enum
	{
		io_event_type_sys,
		io_event_type_accept,
		io_event_type_connect,
		io_event_type_recv,
		io_event_type_error,

		io_event_type_max,
	};

	struct ev_accept : event_head
	{
		int id;
		int listenid;
		errno_type err;
	};

	struct ev_connect : public event_head
	{
		int id;
		errno_type err;
	};

	struct ev_recv : public event_head
	{
		int id;
		ring_buffer* buffer;
	};

	struct ev_socketerr : public event_head
	{
		int id;
		errno_type err;
	};
}