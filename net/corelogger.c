#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "corelog.h"
#include "corelogger.h"

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

#define LOCK(a)
#define UNLOCK(a)

static void log_console(struct logger * log, struct core_logger_ctx *ctx, const char* head, const char* msg)
{
	FILE* io;
	if (ctx->level < corelog_info) {
		io = stderr;
	}
	else {
		io = stdout;
	}
	fputs(head, io);
	fputs(msg, io);
}
static struct logger* logger_new_console(const char* param, struct logerr_ctx * err)
{
	struct logger* log = (struct logger*)malloc(sizeof(*log));
	if (!log) {
		err->err = errno;
		return NULL;
	}
	log->log = log_console;
	log->release = NULL;
	return log;
}

static void log_daterolle_file(struct logger * log, struct core_logger_ctx *ctx,struct tm* tm, const char* head, const char* msg)
{
	const char* fmt=(const char *)(log + 1);
	char filename[128];
	strftime(filename, sizeof(filename), fmt, tm);
	FILE* io = fopen(filename, "a");
	if (io) {
		fputs(head, io);
		fputs(msg, io);
		fclose(io);
	}
}

static struct logger* logger_new_daterolle_file(const char* param, struct logerr_ctx * err)
{
	struct logger* log = (struct logger*)malloc(sizeof(*log));
	if (!log) {
		err->err = errno;
		return NULL;
	}
	log->log = log_console;
	log->release = NULL;
	return log;
}

void corelog_init()
{
	logger_new_register("console", logger_new_console);
}
struct logcfg_lex
{
	int current;
	int lastline;
	int line;
	char* ptr;
};

int logcfg_analyse(struct logcfg_lex* ctx, struct logcfg_section* sec, struct logcfg_err * err)
{
	enum {
		sec_start,
		sec_begin,
		sec_end,
	} state;
	int line = 1;
	char ch,* p, *start;
	state = sec_start;
	for (; *ctx->ptr&&!err->err; ctx->ptr++) {
		ch = *ctx->ptr;
		switch (state) {
		case sec_start:
			switch (ch) {
			case '{':
				sec->line_begin = ctx->line;
				state = sec_begin;
				start = ctx->ptr;
				break;
			case ' ':case '\f':case '\t':
				break;
			case '\r':case '\n':
				ctx->line++;
				break;
			default:
				err->err = -1;
				_snprintf(err->buf, err->sz, "unexpected char (at line %d) %s", ctx->line, ctx->ptr);
				break;
			}
		case sec_begin:
			{
				switch (ch) {
				case '\r':case '\n':
					ctx->line++;
					break;
				case '{':	{
						struct logcfg_section_list list = logcfg_section_list_init();
						logcfg_analyse(ctx, &list, err);
						break;
					}
				case '}':
					state = sec_end;
					sec->line_end = ctx->line;
					*ctx->ptr = NULL;
				}
			}
		}
	}

}