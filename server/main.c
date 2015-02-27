#include <Windows.h>
#include "corelogic.h"
#include "coresocket.h"
#include "corelistener.h"


#define MSG "hello client\n"





int main()
{
	//_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);

	start_socketserver(3);

	
	struct core_poller * io = corepoller_new();
	corepoller_start_thread(io, 1);
	struct core_logic* listener=corelogic_new(io,"listener","0.0.0.0 10000");
	corelogic_cmd(listener, listener_start, NULL);

	for (;;) {
		Sleep(1);
	}

	stop_socketserver();
	
	//_CrtDumpMemoryLeaks();

	return 1;
}