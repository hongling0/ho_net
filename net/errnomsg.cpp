#include "typedef.h"

#define ERRNO_HASH_SIZE (1<<8)
#define hash(e) ((e)%ERRNO_HASH_SIZE)

struct errnode
{
	struct errnode * next;
	errno_type err;
	char msg[1];
};

struct errhash
{
	struct errnode* hash[ERRNO_HASH_SIZE];
	atomic_type lock;
};

struct errhash HASH;

static struct errnode* sys_errmsg(errno_type e)
{
	TCHAR * lpMsgBuf;
	struct errnode* ret;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		e,
		MAKELONG(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL,
		);
	ret = (struct errnode*)malloc(sizeof(*n) + strlen(lpMsgBuf) + 1);
	ret->next = NULL;
	ret->err = e;
	strcpy(ret->msg, lpMsgBuf);
	LocalFree(lpMsgBuf);
	return ret;
}

char * errno_str(errno_type e)
{
	struct errhash* hash = &HASH;
	uint8_t h = hash(e);
	struct errnode* node = hash->hash[h];
	struct errnode* prev, *newnode;
	for (; node&&node->err < e; prev = node, node = node->next);
	if (node&&node->err == e) return node->msg;
	while (atomic_cmp_set(&hash->lock, 0, 1) != 0);

	node = hash->hash[h];
	for (; node&&node->err < e; prev = node, node = node->next);
	if (node&&node->err == e)
	{
		atomic_dec_fetch(&hash->lock, 1);
		return node->msg;
	}
	newnode = sys_errmsg(e);
	if (node)
	{
		prev->next = newnode;
		newnode->next = node;
	}
	else
		hash->hash[h] = newnode;
	return newnode->msg;
}