#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>
#include "corelogic.h"

#define WLOCK(w);
#define WUNLOCK(w)
#define RLOCK(w)
#define RUNLOCK(w)
#define DEFAULT_SLOT_SIZE 64

struct corelogic_store
{
	struct core_logic** slot;
	uint32_t allocid;
	int slot_size;
	volatile long lock;
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

struct core_logic* corelogic_new(struct core_poller* io)
{
	struct core_logic* ret = (struct core_logic*)malloc(sizeof(*ret));
	ret->ref = 1;
	ret->io = io;
	corelogic_store_reg(ret);
	return ret;
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
		if (thelgc == lgc) {
			store->slot[hash] = NULL;
			free(lgc);
		}
		WUNLOCK(&store->lock);
	}
}