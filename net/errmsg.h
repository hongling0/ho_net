#pragma once

#include "typedef.h"

#define FRAME_POLLER_NOT_RUN			2000000000
#define FRAME_POLLER_NOT_FOUND		2000000001
#define FRAME_INVALID_SOCKET_TYPE	2000000002
#define FRAME_CLOSED_SOCKET_TYPE	2000000003
#define FRAME_BUFF_EMPTY					2000000004
#define FRAME_IO_PENDING					2000000005
#define FRAME_IO_PARSEERR					2000000006
#define FRAME_INVALID_SOCKET			2000000007

namespace frame
{

	bool errno_append(errno_type e, char* msg);

	char * errno_str(errno_type e);
}