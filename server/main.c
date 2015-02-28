#include <stdio.h>
#include "corelogic.h"
#include "coresocket.h"
#include "corelistener.h"


#define MSG "hello client\n"





int main()
{
	//_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	start_socketserver(1);

	struct core_poller * io = corepoller_new();
	corepoller_start_thread(io, 1);
	struct core_logic* connector = corelogic_new(io, "listener", "0.0.0.0 10000");
	corelogic_cmd(connector, listener_start, NULL);

	getc(stdin);

	corelogic_cmd(connector, listener_stop, NULL);
	corelogic_release(connector);

	corepoller_stop_thread(io);
	corepoller_delete(io);

	stop_socketserver();

	
	//_CrtDumpMemoryLeaks();

	return 1;
}