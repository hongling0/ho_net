#ifndef CORE_TIMER_H
#define CORE_TIMER_H

#include <stdint.h>

#ifdef _cpluscplus
extern "C" {
#endif //_cpluscplus

	typedef void(*timer_call)(struct iocp* io, void* u);

	struct core_timer * coretimer_new(void);
	void coretimer_delete(struct core_timer *t);
	uint32_t coretimer_add(struct core_timer *t, timer_call call, void* ctx, uint32_t wait);
	void coretimer_del(struct core_timer *t, uint32_t id);
	void coretimer_update(struct core_timer *t, struct iocp* io);

#ifdef _cpluscplus
}
#endif //_cpluscplus

#endif