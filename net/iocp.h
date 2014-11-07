#pragma once

#include "socket.h"

namespace net
{
	class socket;
	struct io_event;

	class iocp
	{
	public:
		iocp();
		~iocp();
		void poll();

		bool start_run(size_t t);

		int start_listen(const char * addr, int port, int backlog, const socket_opt& opt, errno_type& e);
		int start_connet(const char * addr, int port, const socket_opt& opt, errno_type& e);
		int send(int id, char* data, size_t sz, errno_type& e);
//		int async_send(int id,const buf_t* bufs, size_t count, errno_type& e);
		bool post_recv(socket* s, errno_type& err);
		bool post_send(socket*, errno_type& err);
		int post_close(int id);

		bool relisten(socket* s, io_event* ev, errno_type& e);
	private:
		socket* getsocket(int id);
		int reserve_id();
		void force_close(socket* s);
		socket * new_fd(int id, socket_type fd, const socket_opt& opt, bool add);
	private:
		atomic_type alloc_id;
		int event_n;
		int event_index;
		socket* slot;
		handle_type event_fd;
	};
}