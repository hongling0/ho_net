#include <string>

#include "iocp.h"

#define MSG "hello world\n"
using namespace net;

iocp poller;

bool on_connect(net::socket * s,errno_type e)
{
	printf("%d connect_ok %s\n", s->id,errno_str(e));
	errno_type dummy;
	poller.send(s->id, MSG, sizeof(MSG), dummy);
	return true;
}

bool on_recv(net::socket * s, errno_type e)
{
	char * ptr;
	size_t sz;
	if (s->rb.readbuffer(&ptr,&sz))
	{
		std::string str(ptr, sz);
		printf("%d recv %s\n", s->id, str.c_str());
		s->rb.read_ok(sz);
	}
	else
	{
		printf("%d recv %s\n", s->id, errno_str(e));
	}
	return true;
}

int main()
{

	poller.start_run(4);

	errno_type err;
	socket_opt opt;
	opt.recv = on_recv;
	opt.connect = on_connect;

	poller.start_connet("127.0.0.1", 10000, opt,err);
	printf("%s\n", errno_str(err));

	while (1) Sleep(100);
	return 1;
}