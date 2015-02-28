#include <stdio.h>
#include "corepoller.h"
#include "coreconnector.h"
#include "coresocket.h"


int main()
{
	start_socketserver(3);


	struct core_poller * io = corepoller_new();
	corepoller_start_thread(io, 1);
	struct core_logic* connector = corelogic_new(io, "connector", "127.0.0.1 10000");
	corelogic_cmd(connector, connector_start, NULL);

	getchar();

	corelogic_cmd(connector, connector_stop, NULL);
	corelogic_release(connector);

	corepoller_stop_thread(io);
	corepoller_delete(io);

	stop_socketserver();

	//_CrtDumpMemoryLeaks();
	return 1;
}