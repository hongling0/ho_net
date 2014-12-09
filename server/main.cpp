#include <string>
#include <iostream>
#include "listener.h"
#include "socketinterface.h"

#define MSG "hello client\n"


using namespace frame;

class ls : public listener
{
public:
	ls(iocp& io) :listener(io, "", 1000)
	{

	}
	virtual void on_recv(int id, ring_buffer*buffer)
	{
		count++;
		if (count / 100 != cnt_10)
		{
			cnt_10 = count / 100;
			printf("recv msg %d\n", count);
		}
		listener::on_recv(id, buffer);
	}

	int cnt_10;
	int count;
};

int main()
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	frame::start_socketserver(1);

	frame::iocp io;
	io.start_thread(1);

	ls l(io);
	l.start();

	std::string buf;
	std::cin >> buf;
	
	io.stop_thread();
	frame::stop_socketserver();
	
	_CrtDumpMemoryLeaks();

	return 1;
}