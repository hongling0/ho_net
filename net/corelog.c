#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "corelog.h"

#define BUF_SIZE 512
#define LOCK(a)
#define UNLOCK(a)

static unsigned int hash_string(const char* buf, int len)
{
	unsigned int hash = 5318;
	while (len--) {
		hash = ((hash << 5) + hash) + (tolower(*buf++));
	}
	return hash;
}


static void logger_delete(struct logger* log)
{
	if (log->release) {
		log->release(log);
	} else {
		free(log);
	}
}

struct logger_node
{
	struct logger_node * next;
	struct logger * log;
	char* name;
	char* keylist;
	int level_max;
	int level_min;
};

struct logger_keyhead
{
	const char * key;
	struct logger_node head;
};


struct logger_store
{
	struct logger_store * next;
	struct logger_keyhead ** slot_key;	//key2logger
	int key_sz;
	int key_use;
	struct logger_node ** slot_name;	//name2logger
	int name_sz;
	int name_use;
	volatile long ref;
};

struct corelogger
{
	struct logger_store* cur;
	struct logger_store* history;
};

static struct logger_node * hash_rawadd(struct logger_store * store, const char* name)
{
	struct logger_node * node, *next;
	int h, hashint;
	h = hash_string(name, strlen(name));
	hashint = h&(store->name_sz - 1);
	node = store->slot_name[h];
	while (node) {
		next = node->next;
		if (strcmp(name, node->name) == 0) {
			return node;
		}
		node = next;
	}
	node = (struct logger_node *)malloc(sizeof(*node));
	memset(node, 0, sizeof(*node));
	node->name = strdup(name);
	node->level_max = corelog_debug;
	node->level_min = corelog_fatal;
	node->next = store->slot_name[h];
	store->slot_name[h] = node;
	return node;
}


static void logger_impl(struct core_logger_ctx *ctx, const char* msg)
{

}

int logger_register(struct logger* log, const char* key)
{

}

void logger_log(struct core_logger_ctx *ctx, const char* fmt,...)
{
	va_list va;
	char tmp[BUF_SIZE];
	va_start(va, fmt);
	int len = vsnprintf(tmp, BUF_SIZE, fmt, va);
	va_end(va);
	if (len < BUF_SIZE) {
		//todo
	} else {
		char *data = NULL;
		int max_sz = BUF_SIZE;
		for (;;) {
			max_sz <<= 1;
			data = (char*)malloc(max_sz);
			va_start(va, fmt);
			len = vsnprintf(data, max_sz, fmt, va);
			va_end(va);
			if (len < max_sz) {
				break;
			}
			free(data);
		}
		//todo
		free(data);
	}
}