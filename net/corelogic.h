#ifndef CORE_LOGIC_H
#define CORE_LOGIC_H

#include <stdint.h>
#include "corepoller.h"

#ifdef _cpluscplus
extern "C"{
#endif

	typedef void(*logic_handler)(struct core_poller* io, void* ub, int sender, int session, void* data, size_t sz);
	typedef int(*cmd_handler)(void* ub, int cmd, void* param);

	struct core_logic
	{
		volatile unsigned long ref;
		int logic_id;
		struct core_module* mod;
		void* inst;
		void * ub;
		logic_handler call;
		cmd_handler cmd;
		struct core_poller* io;
	};

#define UB2LOGIC(address) ((struct core_logic *)((char*)(address) - (unsigned long)(&((struct core_logic *)0)->ub)))

	struct core_logic* corelogic_new(struct core_poller* io, const char* name, void* param);
	struct core_logic* corelogic_grub(int id);
	void corelogic_release(struct core_logic* lgc);
	int corelogic_cmd(struct core_logic* lgc, int cmd, void* param);

	struct logic_msg
	{
		coremsg_head;
		int sender;
		int recver;
		int session;
		size_t sz;
		void* data;
	};
	struct logic_msg* logicmsg_new();
	void logicmsg_delete(struct logic_msg* msg);

#ifdef _cpluscplus
}
#endif

#endif //CORE_LOGIC_H