#include "connector.h"



namespace frame
{
	static void msg_connect(logic* self, logic_msg* msg)
	{
		logic_connect* m = (logic_connect*)msg;
		//fprintf(stdout, "|logic_connect|%d| [%s]\n", m->id, errno_str(m->err));
		((connector*)self)->on_connect(m->id,m->err);
	}
	static void msg_recv(logic* self, logic_msg* msg)
	{
		logic_recv* m = (logic_recv*)msg;
		((connector*)self)->on_recv(m->id, m->buffer);
	}
	static void msg_err(logic* self, logic_msg* msg)
	{
		logic_socketerr* m = (logic_socketerr*)msg;
		//fprintf(stdout, "|logic_socketerr|%d|%s\n", m->id, errno_str(m->err));
		((connector*)self)->on_socketerr(m->id, m->err);
		start_close(m->id);
	}

	connector::connector(iocp& e, const char* _ip, int _port) :logic(e), ip(_ip), port(_port), socket(0)
	{
		addhandler(logic_msg_id<logic_connect>::id, &msg_connect);
		addhandler(logic_msg_id<logic_recv>::id, &msg_recv);
		addhandler(logic_msg_id<logic_socketerr>::id, &msg_err);
	}
	bool connector::start(protocol_call call/* = default_protocol*/)
	{
		if (socket) 
			return true;

		socket_opt opt;
		opt.recv = call;

		errno_type err = start_connet(logic_id, ip.c_str(), port, opt, socket);
		//fprintf(stdout, "|start_connet|%d|%s:%d|%d\n", logic_id, ip.c_str(), port, socket);
		if (!err && socket > 0)
			return true;
		//fprintf(stderr, "|start_connet|%d failure|%s:%d|%d %s\n", logic_id, ip.c_str(), port, errno_str(err));
		return false;
	}
	void connector::close()
	{
		if (!socket) return;
		start_close(socket);
		socket = 0;
	}

	void connector::on_connect(int id,errno_type err)
	{
		if (id != socket)
		{
			fprintf(stdout, "|on_connect|%d id error|%d %d\n", logic_id,socket, id);
			fflush(stdout);
			assert(false);
		}
			
		//fprintf(stdout, "%d on_connect %s\n", logic_id, errno_str(err));
#define MSG "hellow world\nhellow world\nhellow world\nhellow world\nhellow world\nhellow world\nhellow world\n"
		//if(err==NO_ERROR)
			//send((char*)MSG, sizeof(MSG));
	}
	void connector::on_recv(int id, ring_buffer*buffer)
	{
		char * buf=NULL;
		size_t len=0;
		buffer->readbuffer(&buf, &len);
		std::string msg(buf, len);
		//fprintf(stdout, "%d on_recv %s\n", id, msg.c_str());

		start_send(id, buf, len);
	}
	void connector::on_socketerr(int id, errno_type err)
	{

	}

	void  connector::send(char* data, size_t sz)
	{
		errno_type err = start_send(socket, data, sz);
		if (err)
			fprintf(stderr, "connector::send failure [%s]\n", errno_str(err));
	}
}
