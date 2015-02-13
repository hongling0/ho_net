#ifndef CORE_LOGIC_H
#define CORE_LOGIC_H

#include <stdint.h>
#include "corepoller.h"

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

	struct core_poller;
	struct core_logic;
	struct logic_msg;
	typedef void(*logic_handler)(struct core_poller* io, void* ub, int sender, int session, void* data, size_t sz);
	typedef int(*cmd_handler)(struct core_poller* io, void* ub, int cmd, void* param);

	struct core_logic
	{
		volatile unsigned long ref;
		int logic_id;
		struct core_module* mod;
		void * ub;
		logic_handler call;
		cmd_handler cmd;
		struct core_poller* io;
	};

#define UB2LOGIC(address) ((struct core_logic *)((char*)(address) - (unsigned long)(&((struct core_logic *)0)->ub)))

	struct core_logic* corelogic_new(struct core_poller* io, logic_handler call);
	struct core_logic* corelogic_grub(int id);
	void corelogic_release(struct core_logic* lgc);
	logic_handler corelogic_handler(struct core_logic* lgc, logic_handler call);

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