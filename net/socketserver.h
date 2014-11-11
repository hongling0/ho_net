#pragma once

#include "socket.h"



namespace net
{
	class socket;
	struct io_event;

	namespace socketserver
	{
		void start(size_t t);
		void stop();
		int start_listen(int logic,const char * addr, int port, int backlog, const socket_opt& opt, errno_type& e);
		int start_connet(int logic, const char * addr, int port, const socket_opt& opt, errno_type& e);
		errno_type start_send(int fd, char* data, size_t sz);
		errno_type start_close(int fd);
	}

}