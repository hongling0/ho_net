#include <string>
#include <iostream>
#include "connector.h"

using namespace frame;

class cnt : public connector
{
public:
	cnt(frame::iocp& io) :connector(io, "127.0.0.1", 1000)
	{
		cnt_10 = 0;
		count = 0;
	}

	virtual void on_recv(int id, ring_buffer*buffer)
	{
		count++;
		if (count / 10000 != cnt_10)
		{
			cnt_10 = count / 10000;
			printf("recv msg %d\n", count);
		}
		char * buf = NULL;
		size_t len = 0;
		buffer->readbuffer(&buf, &len);
		//std::string msg(buf, len);

		start_send(id, buf, len);
		//start_send(id, buf, len);
	}

	int cnt_10;
	int count;
};

int main()
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	frame::start_socketserver(4);

	frame::iocp io;
	io.start_thread(1);

	for (int i = 0; i < 100; ++i)
	{
		frame::connector * c = new cnt(io);
		Sleep(10);
		c->start();
	}
	std::string buf;
	std::cin >> buf;

	io.stop_thread();
	frame::stop_socketserver();

	_CrtDumpMemoryLeaks();
	return 1;
}