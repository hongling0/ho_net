#include <string>
#include "iocp.h"


using namespace net;

iocp poller;

bool on_accept(net::socket * s, net::socket * n,errno_type e)
{
	printf("%d on_recv %d %s\n", s->id,n->id,errno_str(e));
	return true;
}

bool on_recv(net::socket * s,errno_type e)
{
	char * ptr;
	size_t sz;
	if (s->rb.readbuffer(&ptr, &sz))
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
	iocp poller;
	poller.start_run(4);

	errno_type err;
	socket_opt opt;
	opt.accept = on_accept;
	opt.recv = on_recv;
	poller.start_listen("", 10000, 0, opt,err);
	printf("%s\n", errno_str(err));

	while (1) Sleep(100);
	return 1;
}