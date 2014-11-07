#include "typedef.h"

#define ERRNO_HASH_SIZE (1<<8)
#define hash(e) ((e)%ERRNO_HASH_SIZE)

#define LOCK(a)
#define UNLOCK(a)

struct errnode
{
	struct errnode * next;
	errno_type err;
	char msg[1];
};

struct errhash
{
	struct errnode head[ERRNO_HASH_SIZE];
	atomic_type lock;
};

struct errhash HASH;

static struct errnode* sys_errmsg(errno_type e)
{
	TCHAR * lpMsgBuf;
	struct errnode* n;
	DWORD retval;
	char * msg;
	retval=FormatMessage(
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
	if (retval == 0)
		msg = "Unkown error\n";
	else
	{
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

char * errno_str(errno_type e)
{
	struct errhash* hash = &HASH;
	uint8_t h = hash(e);
	struct errnode * node,*prev, *newnode;

	node = hash->head[h].next;
	for (; node&&node->err < e; prev = node, node = node->next);
	if (node&&node->err == e) return node->msg;
	while (atomic_cmp_set(&hash->lock, 0, 1) != 0);

	prev = &hash->head[h];
	node = prev->next;
	if (node)
	{
		for (; node&&node->err < e; prev = node, node = node->next);
		if (node->err = e) return node->msg;
	}

	LOCK(&hash->lock);

	prev = &hash->head[h];
	node = prev->next;
	if (node)
	{
		for (; node&&node->err < e; prev = node, node = node->next);
		if (node->err = e)
		{
			UNLOCK(&hash->lock);
			return node->msg;
		}
	}
	
	newnode = sys_errmsg(e);
	prev->next = newnode;
	newnode->next = node;
	UNLOCK(&hash->lock);
	return newnode->msg;
}