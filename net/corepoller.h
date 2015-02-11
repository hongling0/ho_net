#ifndef CORE_POLLER_H
#define CORE_POLLER_H


#include "typedef.h"

#ifdef _cpluscplus
extern "C" {
#endif //_cpluscplus

#define coremsg_head \
		OVERLAPPED op; \
		core_handler call

	struct core_poller;
	struct msghead;
	typedef void(*core_handler)(struct core_poller* io, void* ctx, struct msghead* msg, size_t bytes, int err);

	struct msghead
	{
		coremsg_head;
	} ;

	struct core_poller* corepoller_new(void);
	void corepoller_delete(struct core_poller* io);
	void corepoller_start_thread(struct core_poller* io, uint8_t n);
	void corepoller_stop_thread(struct core_poller* io);
	int corepoller_post(struct core_poller* io, void* ctx, struct msghead* ev, size_t bytes, int e);
	int corepoller_append_socket(struct core_poller* io, SOCKET s, void* context);
	//uint32_t corepoller_start_timer(struct core_poller* io, void *(call)(), void* u, uint32_t wait = 0);
	//void corepoller_stop_timer(struct core_poller* io, uint32_t idx);

#ifdef _cpluscplus
}
#endif //_cpluscplus

#endif //COREPOLLER_H
