#pragma once

#include "event.h"

namespace frame
{
	typedef bool(*protocol_call)(int id, buffer_head* bufferlist, logic_recv** out);

	static bool default_protocol(int id, buffer_head* bufferlist, logic_recv** out)
	{
		ring_buffer * buffer = bufferlist->pop_front();
		logic_recv * logic_ev = new logic_recv;
		logic_ev->id = id;
		logic_ev->buffer = buffer;
		*out = logic_ev;
		return true;
	}

	struct socket_opt
	{
		socket_opt()
		{
			recv = default_protocol;
		}
		protocol_call recv;
	};
	int start_socketserver(int thr);
	int stop_socketserver();
	int start_listen(int logic, const char * addr, int port, int backlog, const socket_opt& opt, errno_type& e);
	int start_connet(int logic, const char * addr, int port, const socket_opt& opt, errno_type& e);
	errno_type start_send(int fd, char* data, size_t sz);
	errno_type start_close(int fd);
}

