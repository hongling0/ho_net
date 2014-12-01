#pragma once

#include "typedef.h"

namespace frame
{

	bool errno_append(errno_type e, char* msg);

	char * errno_str(errno_type e);
}