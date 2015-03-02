#ifndef CORE_LOCK_H
#define CORE_LOCK_H

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
		void(*log)(struct logger * log,const char* key, int level, const char* file, const char *func, int line, int thr, const char* fmt, ...);
		void(*release)(struct logger * log);
	};

	void logger_register(struct logger * log,const char* key);
	void logger_log(struct core_logger_ctx *ctx, const char* fmt,...);

	enum{
		corelog_fatal,
		corelog_error,
		corelog_warnning,
		corelog_info,
		corelog_debug,
	};

#define LOG_IMPL(key,level,...) do{struct core_logger_ctx ctx={key,level,__FILE__,__FUNCTION__,__LINE__,0}; logger_log(&ctx,__VA_ARGS__)  } while(0)

#define LOG_FATAL(key,...) LOG_IMPL(key,corelog_fatal,__VA_ARGS__)
#define LOG_ERROR(key,...) LOG_IMPL(key,corelog_error,__VA_ARGS__)
#define LOG_WARN(key,...) LOG_IMPL(key,corelog_warnning,__VA_ARGS__)
#define LOG_INFO(key,...) LOG_IMPL(key,corelog_info,__VA_ARGS__)
#define LOG_DEBUG(key,...) LOG_IMPL(key,corelog_debug,__VA_ARGS__)

#ifdef _cpluscplus
}
#endif

#endif //CORE_LOCK_H