#include <string>
#include <iostream>
#include "connector.h"

using namespace frame;

struct context
{
	iocp* io;
	static uint32_t counter;
};
uint32_t context::counter = 0;

void test_callback(timer_context u)
{
	context* ctx = (context*)u.p;
	if (ctx->counter%100000==0)
		fprintf(stdout, "timeout:%p,%d\n", u.p, ctx->counter);
	ctx->counter++;
	ctx->io->start_timer(test_callback, u, 0);
}

void start_timer(iocp& io)
{
	context* ctx = new context();
	ctx->io = &io;
	timer_context u; u.p = ctx;
	ctx->io->start_timer(test_callback, u, 0);
}

int main()
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	frame::iocp io;
	io.start_thread(1);

	for (int i = 0; i < 9999; ++i)
	{
		start_timer(io);
	}

	std::string buf;
	std::cin >> buf;

	io.stop_thread();
	frame::stop_socketserver();

	_CrtDumpMemoryLeaks();
	return 1;
}