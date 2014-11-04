#pragma once

#include "typedef.h"

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
		
		void on_accept(int listenid, int newid);
		void on_connect(int connectid);
		void on_send(int s, size_t sz);
		void on_recv(int s, size_t sz);
		void on_close(int s);

		int start_listen(const char * addr, int port, int backlog, errno_type& e);
		int start_connet(const char * addr, int port, errno_type& e);
		int send(int id, char* data, size_t sz);
//		int async_send(int id,const buf_t* bufs, size_t count, errno_type& e);
		bool post_recv(socket* s, errno_type& err);
		int post_send(socket*);
		int post_close(int id);

		bool relisten(socket* s, io_event* ev, errno_type& e);
		bool resend(socket* s, io_event* ev, errno_type& e);
	private:
		socket* getsocket(int id)
		{
			return NULL;
		}

		int reserve_id();
		void force_close(socket* s);
		socket * new_fd(int id, socket_type fd, bool add);
	private:
		atomic_type alloc_id;
		int event_n;
		int event_index;
		socket* slot;
		handle_t event_fd;
	};
}