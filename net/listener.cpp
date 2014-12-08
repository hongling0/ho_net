#pragma once

#include "listener.h"



namespace frame
{
	static void msg_accept(logic* self, logic_msg* msg)
	{
		logic_accept* m = (logic_accept*)msg;
		fprintf(stderr, "|logic_accept|%d| [%s]\n", m->id, errno_str(m->err));
		((listener*)self)->on_accept(m->listenid, m->id);
	}
	static void msg_recv(logic* self, logic_msg* msg)
	{
		logic_recv* m = (logic_recv*)msg;
		((listener*)self)->on_recv(m->id, m->buffer);
	}
	static void msg_err(logic* self, logic_msg* msg)
	{
		logic_socketerr* m = (logic_socketerr*)msg;
		fprintf(stdout, "|logic_socketerr|%d|%s\n", m->id, errno_str(m->err));
		((listener*)self)->on_socketerr(m->id, m->err);
		start_close(m->id);
	}

	listener::listener(iocp& e, const char* _ip, int _port) :logic(e), ip(_ip), port(_port), socket(0)
	{
		addhandler(logic_msg_id<logic_accept>::id, &msg_accept);
		addhandler(logic_msg_id<logic_recv>::id, &msg_recv);
		addhandler(logic_msg_id<logic_socketerr>::id, &msg_err);
	}
	bool listener::start(protocol_call call/* = default_protocol*/)
	{
		if (socket) return true;

		socket_opt opt;
		opt.recv = call;

		errno_type err;
		fprintf(stdout, "%d start_listen %s:%d\n", logic_id, ip.c_str(), port);
		socket = start_listen(logic_id, ip.c_str(), port, 0, opt, err);
		if (!err) return true;

		fprintf(stderr, "start listen %s:%d failure ! %s\n", ip.c_str(), port, errno_str(err));
		return false;
	}
	void listener::close()
	{
		if (!socket) return;
		start_close(socket);
	}

	void listener::on_accept(int listenid, int newid)
	{
		fprintf(stdout, "%d logic %d accept %d\n", logic_id, listenid,newid);
	}
	void listener::on_recv(int id, ring_buffer*buffer)
	{
		char * buf=NULL;
		size_t len=0;
		if (buffer->readbuffer(&buf, &len))
		{
			//std::string msg(buf, len);
			//fprintf(stdout, "%d on_recv %s\n", id, msg.c_str());
		}
		start_send(id, buf, len);
	}
	void listener::on_socketerr(int id, errno_type err)
	{
		
	}
}