#include <stdio.h>

extern struct core_module connector_module;

struct core_module* all_modules[]=
{
	&connector_module,
	NULL
};
