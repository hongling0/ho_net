#include <string>
#include "listener.h"



#define MSG "hello client\n"


int main()
{
	frame::iocp io;
	io.start_thread(1);

	frame::listener l(io, "", 1000);
	l.start();

	while (1) Sleep(100);
	return 1;
}