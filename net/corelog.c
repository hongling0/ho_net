#define _CRT_SECURE_NO_WARNINGS
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "corelog.h"
#include "corelogger.h"

#define BUF_SIZE 512
#define BUF_TIME_SIZE 32
#define BUF_HEAD_SIZE 64

static unsigned int hash_string(const char* buf, int len)
{
	unsigned int hash = 5318;
	while (len--) {
		hash = ((hash << 5) + hash) + (*buf++);
	}
	return hash;
}

static long tonumber(const char* str, long def)
{
	char* endptr = NULL;
	long n = strtol(str, &endptr, 10);
	if (*endptr != '\0') {
		fprintf(stderr, "can not convert to number. %s\n", str);
		return def;
	}
	return n;
}

static char* ltrim(char* str)
{
	int i = 0;
	for (; str[i] != '\0'; i++) {
		if (!isspace(str[i])) break;
	}
	return str + i;
}
static char* rtrim(char* str)
{
	int i, e = -1;
	for (i = 0; str[i] != '\0'; i++) {
		if (isspace(str[i])) {
			if (e < 0) e = i;
		} else {
			if (e >= 0) e = -1;
		}
	}
	if (e >= 0)	 str[e] = '\0';
	return str;
}
static char* trim(char* str)
{
	return rtrim(ltrim(str));
}


static const char* LOG_LEVEL[] =
{
	"FATAL", "ERROR", "WARN", "INFO", "DEBUG"
};

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
	node->name = _strdup(name);
	node->level_max = corelog_debug;
	node->level_min = corelog_fatal;
	node->next = store->slot_name[h];
	store->slot_name[h] = node;
	return node;
}

static void logger_write(struct logger_store* store, struct core_logger_ctx * ctx, const char* key, struct tm* tm, const char* head, const char* msg)
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
					logger->log(logger, ctx, tm, head, msg);
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
	time_t now = time(NULL);
	localtime_s(&tm, &now);
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
		logger_impl(&ctx, data);
		free(data);
	}
}

struct corelogger_conf
{
	char* name;
	char* type;
	char* keylist;
	int level_max;
	int level_min;
	char* args;
};

static int corelogger_create(struct corelogger_conf * conf)
{
	struct logger_node * node = hash_rawadd(the_corelogger_store.cur, conf->name);
	logger_delete(node->log);
	node->log = NULL;
	node->keylist = conf->keylist;
	struct logger* log = do_logger_new(conf->type, conf->args);
	if (log) {
		node->log = log;
	}
	return 0;
}


static const char* init =
"{"
"	name=console"
"	type=console"
"	level_min=0"
"	level_max=4"
"}"
"";

void corelog_init(char* args)
{
	char* newstr = NULL;
	if (!args) {
		newstr = _strdup(init);
		args = newstr;
	}
	char errbuf[128];
	struct logconf_analyze_ctx analyze = logconf_analyze_ctx_init(args, 1);
	struct logerr_ctx err = logerr_ctx_init(errbuf, sizeof(errbuf));
	struct logconf_section_list list = { NULL };
	if (logconf_section(struct logconf_analyze_ctx* ctx, struct logconf_section_list* list, struct logerr_ctx * err)) {
		fprintf(stderr, "corelog_init failire! %s\n", errbuf : "unkonw");
	}
	struct logconf_context thectx = { args, 1, 0, { 0, 0 } };
	struct logconf_context *ctx = &thectx;
	struct core_err_ctx err;
	for (;;) {
		if (logconf_next_section(ctx, &err)) {
			if (ctx->err) {
				fprintf(stderr, "corelog_init failire! %s\n", ctx->errstr ? ctx->errstr : "unkonw");
			}
			break;
		}
		struct logconf_context newctx = { ctx->out[elogconf_section], ctx->line };
		if (corelogger_init(&newctx)) {

		}
	}
	free(newstr);
}

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

static struct logger* do_logger_new(const char* type, const char* args)
{
	struct logger_new_node* node, *head = &newfunc_list;
	for (node = head->next; node; node = node->next) {
		if (strcmp(type, node->type) == 0) {
			return node->func(args);
		}
	}
	return NULL;
}
