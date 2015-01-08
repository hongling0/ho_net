#include <string>
#include <iostream>
#include "connector.h"

using namespace frame;

struct context
{
	static uint32_t counter;
};
uint32_t context::counter = 0;

void test_callback(iocp& io,void* u)
{
	context* ctx = (context*)u;
	if (ctx->counter%100000==0)
		fprintf(stdout, "timeout:%p,%d\n", u, ctx->counter);
	ctx->counter++;
	io.start_timer(test_callback, u, 1000);
}

void start_timer(iocp& io)
{
	context* ctx = new context();
	io.start_timer(test_callback, ctx, 1000);
}

int main()
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	frame::iocp io;
	io.start_thread(8);

	for (int i = 0; i < 100000; ++i)
	{
		start_timer(io);
	}

	std::string buf;
	std::cin >> buf;

	io.stop_thread();

	_CrtDumpMemoryLeaks();
	return 1;
}