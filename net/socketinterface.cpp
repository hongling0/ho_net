#include "socketserver.h"

namespace frame
{

	static iocp io;
	static socket_server * s_server;

	int start_socketserver(int thr)
	{
		s_server = new socket_server(io);
		io.start_thread(thr);
		return 0;
	}
	int stop_socketserver()
	{
		io.stop_thread();
		delete s_server;
		s_server = NULL;
		return 0;
	}

	errno_type start_listen(int logic, const char * addr, int port, int backlog, const socket_opt& opt, int& e)
	{
		socket_server& server = *s_server;
		return server.start_listen(logic, addr, port, backlog, opt, e);
	}
	errno_type start_connet(int logic, const char * addr, int port, const socket_opt& opt, int& e)
	{
		socket_server& server = *s_server;
		return server.start_connet(logic, addr, port, opt, e);
	}
	errno_type start_send(int id, char* data, size_t sz)
	{
		socket_server& server = *s_server;
		return server.start_send(id, data, sz);
	}
	errno_type start_close(int id)
	{
		socket_server& server = *s_server;
		return server.start_close(id);
	}
}

