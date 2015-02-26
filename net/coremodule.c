#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "coremodule.h"
#include "modules.h"


struct core_module* coremodule_query(const char * name)
{
	int i;
	for (i = 0; all_modules[i]; i++) {
		if (strcmp(name, all_modules[i]->name) == 0) {
			return all_modules[i];
		}
	}
	return NULL;
}