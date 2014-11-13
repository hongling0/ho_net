#pragma once

//#include <Windows.h>
//#include <winsock.h>

#include "typedef.h"
#include "buffer.h"



namespace net
{
	struct event_head
	{
		OVERLAPPED op;
		uint8_t type;
	};
}