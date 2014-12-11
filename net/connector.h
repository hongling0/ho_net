#pragma once

#include <string>
#include "socketinterface.h"
#include "logic.h"

namespace frame
{
	class connector : public logic
	{
	public:
		connector(iocp& e, protocol_call call = default_protocol);
		bool start(const char* _ip, int _port);
		void close();

		void send(char* data, size_t sz);
		
		virtual void on_connect(int id, errno_type err);
		virtual void on_recv(int id, ring_buffer*buffer);
		virtual void on_socketerr(int id, errno_type err);
	private:
		std::string ip;
		int port;
		int socket;
		protocol_call call;
	};
}