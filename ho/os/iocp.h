#pragma once

#include <Windows.h>
#include <winsock.h>

#include "../typedef.h"
#include "socket.h"


namespace net
{
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

		int start_listen(const char * addr, int port, int backlog, errno_t& e);
		int start_connet(const char * addr, int port, errno_t& e);
		int start_send(int id, void* data, size_t sz);
//		int async_send(int id,const buf_t* bufs, size_t count, errno_t& e);
		int post_recv(socket*);
		int post_send(socket*);
		int post_close(socket*);
	private:
		socket* getsocket(int id)
		{

		}
		bool relisten(socket* s, io_event* ev);
		bool resend(socket* s, io_event* ev);
		int reserve_id();
		int force_close(socket* s);
		socket * new_fd(int id, int fd, bool add);
	private:
		atomic_type alloc_id;
		int event_n;
		int event_index;
		socket* slot;
		handle_t event_fd;
	};
}