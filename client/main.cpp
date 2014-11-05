
#include "iocp.h"


using namespace net;


int main()
{
	iocp poller;
	poller.start_run(4);

	errno_type err;
	poller.start_connet("127.0.0.1", 10000, err);
	printf("%s\n", errno_str(err));

	while (1) Sleep(1);
	return 1;
}