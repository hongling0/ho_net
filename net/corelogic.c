#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "corelock.h"
#include "corelogic.h"
#include "coremodule.h"

#define WLOCK(w) rwlock_wlock((w))
#define WUNLOCK(w) rwlock_wunlock((w))
#define RLOCK(w) rwlock_rlock((w))
#define RUNLOCK(w) rwlock_runlock((w))
#define DEFAULT_SLOT_SIZE 64

struct corelogic_store
{
	struct core_logic** slot;
	uint32_t allocid;
	int slot_size;
	struct rwlock lock;
};

static struct corelogic_store STORE;

static void corelogic_store_resize()
{
	int i,handle,hash;
	struct core_logic** newlist;
	struct corelogic_store * store = &STORE;
	unsigned long new_sz = store->slot_size;
	new_sz = (new_sz > 0 ? new_sz * 2 : DEFAULT_SLOT_SIZE);
	newlist = (struct core_logic**)malloc(sizeof(struct core_logic*)*new_sz);
	memset(newlist, 0, sizeof(struct core_logic*)*new_sz);
	for (i = 0; i < store->slot_size; i++) {
		handle = store->slot[i]->logic_id;
		hash = handle & ((store->slot_size * 2) - 1);
		newlist[hash] = store->slot[i];
	}
	free(store->slot);
	store->slot = newlist;
	store->slot_size = new_sz;
}

static uint32_t  corelogic_store_reg(struct core_logic* lgc)
{
	int i,handle,hash;
	struct corelogic_store * store = &STORE;
	assert(lgc);
	WLOCK(&store->lock);
	for (;;) {
		for (i = 0; i < store->slot_size; i++) {
			handle = i + store->allocid;
			hash = handle & (store->slot_size - 1);
			if (store->slot[hash] == NULL) {
				lgc->logic_id = handle;
				store->slot[hash] = lgc;
				store->allocid = handle + 1;
				WUNLOCK(&store->lock);
				return handle;
			}
		}
		corelogic_store_resize();
	}
}

static void default_logic_handler(struct core_poller* io, struct core_logic* lgc, int sender, int session, void* data, size_t sz)
{
	assert(0);
}

struct core_logic* corelogic_new(struct core_poller* io,const char* name, void* param)
{
	struct core_logic* ret;
	void* inst;
	struct core_module* mod = coremodule_query(name);
	if (!mod)
		return NULL;

	assert(mod->create);
	inst = mod->create();
	if (!inst)
		return NULL;
	
	ret = (struct core_logic*)malloc(sizeof(*ret));
	ret->ref = 1;
	ret->io = io;
	ret->call = NULL;
	ret->cmd = NULL;
	ret->inst = inst;
	corelogic_store_reg(ret);
	mod->init(inst, ret, param);
	return ret;
}

int corelogic_cmd(struct core_logic* lgc, int cmd, void* param)
{
	return lgc->cmd(lgc->ub, cmd, param);
}

struct core_logic* corelogic_grub(int id)
{
	int hash;
	struct core_logic* lgc,*ret;
	struct corelogic_store * store = &STORE;

	RLOCK(&store->lock);
	hash = id&(store->slot_size - 1);
	lgc = store->slot[hash];
	if (lgc&& lgc->logic_id == id) {
		InterlockedIncrement(&lgc->ref);
		ret = lgc;
	}
	RUNLOCK(&store->lock);
	return ret;
}

void corelogic_release(struct core_logic* lgc)
{
	int hash;
	struct core_logic* thelgc;
	struct corelogic_store * store = &STORE;

	if (InterlockedDecrement(&lgc->ref) == 0) {
		WLOCK(&store->lock);
		hash = lgc->logic_id&(store->slot_size - 1);
		thelgc = store->slot[hash];
		assert(thelgc == lgc);
		store->slot[hash] = NULL;
		WUNLOCK(&store->lock);
		lgc->mod->release(lgc->inst);
		free(lgc);
	}
}

logic_handler corelogic_handler(struct core_logic* lgc, logic_handler call)
{
	logic_handler oldcall = lgc->call;
	lgc->call = call ? call : default_logic_handler;
	return oldcall;
}

static void default_logicmsg_handler(struct core_poller* io, void* data, struct msghead* msg, size_t bytes, int err)
{
	struct logic_msg* ctx = (struct logic_msg*)msg;
	struct core_logic* lgc = corelogic_grub(ctx->recver);
	if (!lgc) {
		fprintf(stderr, "logicmsg_handler dead core_logic %d\n", ctx->recver);
	} else {
		lgc->call(io, lgc, ctx->sender, ctx->session, ctx->data, ctx->sz);
	}
	logicmsg_delete(ctx);
}

struct logic_msg* logicmsg_new()
{
	struct logic_msg* ctx = (struct logic_msg*)malloc(sizeof(*ctx));
	memset(ctx, 0, sizeof(*ctx));
	ctx->call = default_logicmsg_handler;
	return ctx;
}

void logicmsg_delete(struct logic_msg* msg)
{
	free(msg->data);
	free(msg);
}