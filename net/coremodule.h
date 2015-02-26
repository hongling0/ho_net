#ifndef CORE_MODULE_H
#define CORE_MODULE_H

#include <stdint.h>

#ifdef _cpluscplus
extern "C"{
#endif

	struct core_module
	{
		char name[16];
		void*(*create)(void);
		int(*init)(void* ins, struct core_logic* lgc, void* param);
		void(*release)(void* ins);
	};

	struct core_module* coremodule_query(const char* name);
	 
#ifdef _cpluscplus
}
#endif

#endif //CORE_MODULE_H