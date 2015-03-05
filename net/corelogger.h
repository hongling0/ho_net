#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#ifdef _cpluscplus
extern "C" {
#endif

	struct core_logger_ctx
	{
		const char* key;
		int level;
		const char* file;
		const char* func;
		int line;
		int thr;
	};

	struct logger
	{
		void(*log)(struct logger * log, struct core_logger_ctx *ctx, struct tm* tm, const char* head, const char* msg);
		void(*release)(struct logger * log);
	};

	struct logcfg_err
	{
		/* out  err ocurr*/
		int err;
		char* buf;
		unsigned long sz;
	};
#define logerr_ctx_init(buf,sz) {0,buf,sz}

	struct logcfg_keyval
	{
		struct logcfg_keyval * next;
		int line_begin;
		int line_end;
		char* key;		
		char* val;
		struct logcfg_section* section;
	};
	
	struct logcfg_section
	{
		struct logcfg_section * next;
		int line_begin;
		int line_end;
		struct logcfg_keyval* keyval;
	};
	struct logcfg_section_list
	{
		struct logcfg_section* head;
	};
#define logcfg_section_list_init() {NULL}
	void logcfg_section_list_free(struct logconf_section_list* list);

	int logcfg_analyse(struct logconf_section_list* list, struct logerr_ctx * err,char* input);

	typedef struct logger* (*logger_new)(const char* param, struct logerr_ctx * err);
	int logger_new_register(const char* type, logger_new func);

#ifdef _cpluscplus
}
#endif

#endif //CORE_LOGGER_H