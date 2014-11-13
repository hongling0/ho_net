#pragma once

#include "socket.h"

namespace net
{
	namespace network
	{
		void start(size_t thr);
		void stop();
		int start_listen(int logic,const char * addr, int port, int backlog, errno_type& e);
		int start_connet(int logic, const char * addr, int port, errno_type& e);
		errno_type start_send(int fd, char* data, size_t sz);
		errno_type start_close(int fd);
	}
}