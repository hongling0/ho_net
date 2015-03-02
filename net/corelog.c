#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "corelog.h"

#define BUF_SIZE 512
#define BUF_TIME_SIZE 32
#define BUF_HEAD_SIZE 64


const char* LOG_LEVEL[] =
{
	"FATAL", "ERROR", "WARN", "INFO", "DEBUG"
};

static unsigned int hash_string(const char* buf, int len)
{
	unsigned int hash = 5318;
	while (len--) {
		hash = ((hash << 5) + hash) + (*buf++);
	}
	return hash;
}

static void logger_delete(struct logger* log)
{
	if (log->release) {
		log->release(log);
	}
	else {
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
	int hash;
};

static struct logger_new_node
{
	struct logger_new_node * next;
	logger_new func;
	char type[1];
} newfunc_list;

int logger_new_func_reg(const char* type, logger_new func)
{
	struct logger_new_node* node, *head = &newfunc_list;

	for (node = head->next; node; node = node->next) {
		if (strcmp(type, node->type) == 0) {
			return -1;
		}
	}
	node = (struct logger_new_node*)malloc(sizeof(*node) + strlen(type) + 1);
	node->func = func;
	strcpy(node->type, type);
	node->next = head->next;
	head->next = head;
	return 0;
}

static struct logger* do_logger_new(const char* type,const char* args)
{
	struct logger_new_node* node, *head = &newfunc_list;
	for (node = head->next; node; node = node->next) {
		if (strcmp(type, node->type) == 0) {
			return node->func(args);
		}
	}
	return NULL;
}

struct logger_keyhead
{
	struct logger_keyhead * next;
	const char * key;
	int hash;
	struct logger_node * node;
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


static struct corelogger_stroe
{
	struct logger_store* cur;
	struct logger_store* history;
} the_corelogger_store;

static struct logger_node * hash_rawadd(struct logger_store * store, const char* name)
{
	struct logger_node * node, *next;
	int h, hashint;
	hashint = hash_string(name, strlen(name));
	h = hashint&(store->name_sz - 1);
	node = store->slot_name[h];
	while (node) {
		next = node->next;
		if (node->hash == hashint && (strcmp(name, node->name) == 0)) {
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

static void logger_write(struct logger_store* store, struct core_logger_ctx * ctx,const char* key,struct tm* tm, const char* head, const char* msg)
{
	struct logger_keyhead * head_node;
	struct logger_node* log_node;
	int hash = hash_string(key, strlen(key));
	int h = hash&(store->key_sz - 1);
	for (head_node = store->slot_key[h]; head_node; head_node = head_node->next) {
		if (head_node->hash == hash && (strcmp(key, head_node->key))) {
			for (log_node = head_node->node; log_node; log_node = log_node->next) {
				if (log_node->level_max >= ctx->level&&log_node->level_min <= ctx->level) {
					struct logger* logger = log_node->log;
					logger->log(logger, ctx,tm, head, msg);
				}
			}
			break;
		}
		head_node = head_node->next;
	}
}

static void logger_impl(struct core_logger_ctx * ctx, const char* msg)
{
	char buf[BUF_TIME_SIZE], head[BUF_HEAD_SIZE];
	struct tm tm;

	struct corelogger_stroe * store = &the_corelogger_store;
	struct logger_store* s = store->cur;
	if (!s) return;

	localtime_s(&tm, time(NULL));
	strftime(buf, BUF_SIZE, "%c", &tm);
	sprintf(head, "%s [%s][%s]", buf, ctx->key, LOG_LEVEL[ctx->level]);

	logger_write(s, ctx, ctx->key, &tm, head, msg);
	logger_write(s, ctx, "*", &tm, head, msg);
}

void logger_log(const char* key, int level, const char* file, const char* func, int line, int thr, const char* fmt, ...)
{
	va_list va;
	char tmp[BUF_SIZE];
	int len;
	struct core_logger_ctx ctx = { .key = key, .level = level, .file = file, .func = func, .line = line, .thr = thr };

	va_start(va, fmt);
	len = vsnprintf(tmp, BUF_SIZE, fmt, va);
	va_end(va);
	if (len < BUF_SIZE) {
		logger_impl(&ctx, tmp);
	}
	else {
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
		logger_impl(&ctx, data);
		free(data);
	}
}


static char* ltrim(char* str)
{
	int i = 0;
	for (; str[i] != '\0'; i++) {
		if (!isspace(str[i]))
			break;
	}
	return str + i;
}
static void rtrim(char* str)
{
	int i,e = -1;
	for (i = 0; str[i] != '\0'; i++) {
		if (isspace(str[i])) {
			if (e < 0)	e = i;
		}
		else {
			if (e >= 0)	e = -1;
		}
	}
	if (e >= 0)	str[e] = '\0';
}
static char* trim(char* str)
{
	char* ret = ltrim(str);
	rtrim(ret);
	return ret;
}

char* logconf_next_keyval(const char**key, const char**val, char* args)
{
	char *key_begin = NULL, *key_end = NULL, *val_begin = NULL, *val_end = NULL, *ptr = *args;
	int got = 0;;
	while (*ptr) {
		if (*ptr == '\n') {
			if (key_end) {
				val_end = ptr++;
				break;
			}else {
				key_begin = NULL;
			}
		}
		else if (*ptr == '0') {
			if (key_end) {
				val_end = ptr;
				break;
			} else {
				return NULL;
			}
		}
		else if (*ptr == '=') {
			if (!key_end) {
				key_end = ptr;
				val_begin = ptr + 1;
			}
		}
		else {
			if (!key_begin) {
				key_begin = *ptr;
			}
		}
	}
	*key_end = '0';
	if (!key_begin) {
		key_begin = key_end;
	}
	*key = trim(key_begin);
	*val = trim(val_begin);
	return ptr;
}
char* logconf_next_section(char** args)
{
	char*begin, *end, *ptr=*args;
	int left = 0, right = 0;
	if (!ptr) return NULL;

	while (*ptr) {
		if (*ptr == '{') {
			left++;
			if (left == 1) {
				*begin = ptr + 1;
			}
		}
		else if (*ptr == '}') {
			right++;
			if (left == right) {
				end = ptr;
				break;
			}
		}
	}
	if (left != right) {
		fprintf(stderr, "unmatch {}\n");
		return NULL;
	}
	if (*ptr == '0') {
		return NULL;
	}
	*args = begin;
	*ptr = '0';
	return ++ptr;
}

void corelog_init(char* args)
{
	char *ptr = args;
	for (;;) {
		char* sec = NULL;
		ptr = logconf_next_section(&sec);
		if (!sec) {
			break;
		}
		for (;;) {
			char *key = NULL, *val = NULL;
			sec = logconf_next_keyval(&key, &val, sec);
			if (!key&&!val) {
				break;
			}
			if (strcmp(key, "name") == 0) {

			}
			else if (strcmp(key, "type")==0) {

			}
			else if (strcmp(key, "levl_min") == 0) {

			}
			else if (strcmp(key, "levl_max") == 0) {

			}
			else if (strcmp(key, "args") == 0) {

			}
			if (!sec) {
				break;
			}
		}
		if (!ptr) {
			break;
		}
	}
}