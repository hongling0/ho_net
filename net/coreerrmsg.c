#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <Windows.h>

#include "coreerrmsg.h"

#define ERRNO_HASH_SIZE (1<<8)
#define HASH(e) (e&(HASH.slot_sz-1))

#define LOCK(a)  while (InterlockedCompareExchange(a, 1, 0) != 0)
#define UNLOCK(a) InterlockedExchange(a,0)

struct errnode
{
	struct errnode * next;
	int err;
	char msg[1];
};

struct errhash
{
	struct errnode** slot;
	unsigned long slot_sz;// head[ERRNO_HASH_SIZE];
	unsigned long used;
	volatile long lock;
};

struct errhash HASH;

static struct errnode* sys_errmsg(int e, const char * def)
{
	TCHAR * lpMsgBuf;
	struct errnode* n;
	DWORD retval;
	const char * msg;
	retval = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		e,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
		);
	if (retval == 0) {
		if (def)
			msg = def;
		else {
			char buf[64];
			sprintf(buf, "Unkown error %d", e);
			msg = buf;
		}
	} else {
		msg = lpMsgBuf; // todo: widechar to mulitbyte words  
	}
	n = (struct errnode*)malloc(sizeof(*n) + strlen(msg) + 1);
	n->next = NULL;
	n->err = e;
	strcpy(n->msg, msg);
	if (retval)
		LocalFree(lpMsgBuf);
	return n;
}

static unsigned long thepower(unsigned long n)
{
	unsigned long i = 64;
	while (i < n) {
		i <<= 1;
	}
	return i;
}

static void core_errno_resize(struct errhash* hash);
static struct errnode * core_errno_add(struct errhash* hash, uint32_t h, struct errnode *node)
{
	struct errnode * cur, *prev;
	core_errno_resize(hash);
	prev = NULL;//;
	h = HASH(node->err);
	cur = hash->slot[h];
	for (; cur&&cur->err < node->err; prev = cur, cur = cur->next);
	if (cur&&cur->err == node->err) {
		free(node);
		return cur;
	}
	if (prev) {
		prev->next = node;
	} else {
		hash->slot[h] = node;
	}
	node->next = cur;
	hash->used++;
	return node;
}

static void core_errno_resize(struct errhash* hash)
{
	unsigned long oldsz, i;
	struct errnode **oldlist, *node, *next;
	if ((hash->slot_sz*0.75) >(hash->used + 1)) {
		return;
	}
	oldlist = hash->slot;
	oldsz = hash->slot_sz;
	hash->slot_sz = thepower(hash->slot_sz);
	hash->slot = (struct errnode**)malloc(sizeof(*hash->slot)*hash->slot_sz);
	hash->used = 0;
	memset(hash->slot, 0, sizeof(*hash->slot)*hash->slot_sz);
	for (i = 0; i < hash->slot_sz; ++i) {
		node = hash->slot[i];
		while (node) {
			next = node->next;
			core_errno_add(hash, HASH(node->err), node);
			node->next;
		}
	}
}

static const char * errno_str_inner(int e, const char* def)
{
	struct errhash* hash = &HASH;
	uint32_t h = HASH(e);
	struct errnode * node;

	node = hash->slot?hash->slot[h]:NULL;
	for (; node&&node->err < e; node = node->next);
	if (node&&node->err == e) return node->msg;

	LOCK(&hash->lock);
	node = sys_errmsg(e, def);
	node = core_errno_add(hash, h, node);
	UNLOCK(&hash->lock);
	return node->msg;
}

int errno_append(int e, const char* msg)
{
	return strcmp(errno_str_inner(e, msg), msg);
}

const char* errno_str(int e)
{
	return errno_str_inner(e, NULL);
}

void core_errmsg_release()
{
	struct errnode * node, *next;
	unsigned i;
	struct errhash* hash = &HASH;
	for (i = 0; i < hash->slot_sz; ++i) {
		node = hash->slot[i];
		while (node) {
			next = node->next;
			free(node);
			node = next;
		}
	}
	free(hash->slot);
	hash->slot = NULL;
	hash->used = 0;
	hash->slot_sz = 0;
}