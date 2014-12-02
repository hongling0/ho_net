#include <string>

#include "connector.h"


int main()
{
	frame::start_socketserver(2);

	frame::iocp io;
	io.start_thread(1);

	frame::connector c(io, "127.0.0.1", 1000);
	c.start();

	while (1) Sleep(100);
	return 1;
}