#pragma once

//#include <Windows.h>
//#include <winsock.h>

#include <string>
#include "socketserver.h"
#include "logic.h"



namespace frame
{
	class listener : public logic
	{
	public:
		listener(iocp& e, const char* _ip, int _port) :logic(e), ip(_ip), port(_port), socket(0)
		{
			addhandler(msg_id<logic_accept>::id, &msg_accept);
		}
		bool start()
		{
			if (socket) return true;
			errno_type err;
			socket_opt opt;
			opt.recv = protocol_recv;
			socket = start_listen(logic_id, ip.c_str(), port, 0, opt, err);
			if (!err) return true;

			fprintf(stderr, "start listen %s:%d failure ! %s\n", ip.c_str(), port, errno_str(err));
			return false;
		}
		void close()
		{
			if (!socket) return;
			start_close(socket);
		}
		
	protected:
		static bool protocol_recv(socket* s, errno_type e)
		{
			return true;
		}
		virtual void on_accept(int listenid, int newid, errno_type err)
		{

		}
		virtual void on_recv(int id, ring_buffer*buffer, errno_type err)
		{

		}
	protected:
		static void msg_accept(logic* self,logic_msg* msg)
		{
			logic_accept* m = (logic_accept*)msg;
			((listener*)self)->on_accept(m->listenid, m->id, m->err);
		}
		static void msg_recv(logic* self, logic_msg* msg)
		{

		}
	private:
		std::string ip;
		int port;
		int socket;
	};
}