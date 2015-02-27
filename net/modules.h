#include <stdio.h>

extern struct core_module connector_module;
extern struct core_module listener_module;

struct core_module* all_modules[]=
{
	&connector_module,
	&listener_module,
	NULL
};
