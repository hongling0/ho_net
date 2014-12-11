#pragma once

//#include <Windows.h>
//#include <winsock.h>

#include <string>
#include "socketinterface.h"
#include "logic.h"



namespace frame
{
	class listener : public logic
	{
	public:
		listener(iocp& e, protocol_call call = default_protocol);
		bool start(const char* _ip, int _port);
		void close();
		
		virtual void on_accept(int listenid, int newid);
		virtual void on_recv(int id, ring_buffer*buffer);
		virtual void on_socketerr(int id, errno_type err);
	private:
		std::string ip;
		int port;
		int socket;
		protocol_call call;
	};
}