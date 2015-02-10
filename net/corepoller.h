#ifndef COREPOLLER_H
#define COREPOLLER_H


#include "typedef.h"

#ifdef _cpluscplus
extern "C" {
#endif //_cpluscplus

	typedef struct core_msg
	{
		OVERLAPPED op;
		int sender;
		int recver;
		int session;
		size_t len;
		void* msg;
	} core_msg;

	typedef void(*core_handler)(struct core_poller* io,void* ctx,core_msg* msg,size_t bytes,int err);

	struct core_poller;
	struct core_poller* corepoller_new(void);
	void corepoller_delete(struct core_poller* io);
	void corepoller_start_thread(struct core_poller* io, uint8_t n);
	void corepoller_stop_thread(struct core_poller* io);
	int corepoller_post(struct core_poller* io, void* ctx, core_msg* ev, size_t bytes, int e);
	int corepoller_append_socket(struct core_poller* io, SOCKET s, void* context);
	core_handler corepoller_handler(struct core_poller* io, core_handler h);
	//uint32_t corepoller_start_timer(struct core_poller* io, void *(call)(), void* u, uint32_t wait = 0);
	//void corepoller_stop_timer(struct core_poller* io, uint32_t idx);

#ifdef _cpluscplus
}
#endif //_cpluscplus

#endif //COREPOLLER_H
