#include <string>

#include "connector.h"


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
	frame::iocp io;
	io.start_thread(1);

	frame::connector c(io, "127.0.0.1", 10000);
	c.start();

	while (1) Sleep(100);
	return 1;
}