#ifndef CORE_LOGIC_H
#define CORE_LOGIC_H

#include <stdint.h>
#include "corelogic.h"

#ifdef _cpluscplus
extern "C"{
#endif
	
	typedef void(logic_handler)(struct core_poller* io, void* data, struct msghead* msg, size_t sz, int err);

	struct core_logic
	{
		volatile unsigned long ref;
		int logic_id;
		logic_handler handler;
		struct core_poller* io;
	};

	struct core_logic* corelogic_new(struct core_poller* io);
	struct core_logic* corelogic_grub(int id);
	void corelogic_release(struct core_logic* lgc);

#ifdef _cpluscplus
}
#endif

#endif //CORE_LOGIC_H