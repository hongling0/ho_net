#include <stdio.h>
#include "corelog.h"

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

struct logger* logger_new_console(void)
{
	struct logger* log = (struct logger*)malloc(sizeof(*log));
	log->log = log_console;
	log->release = NULL;
	return log;
}
