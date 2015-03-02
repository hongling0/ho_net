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
		void(*log)(struct logger * log, struct core_logger_ctx *ctx,struct tm* tm,const char* head,const char* msg);
		void(*release)(struct logger * log);
	};

	void logger_log(const char* key,int level,const char* file,const char* func,int line, int thr,const char* fmt,...);

	enum{
		corelog_fatal,
		corelog_error,
		corelog_warnning,
		corelog_info,
		corelog_debug,
	};

	/*
	{
		name=console
		type=console
		level_max=0
		level_min=3
		keylist=*
	}
	{
		name=size_rolle_file
		type=size_rolle_file
		level_max=0
		level_min=3
		keylist=*
		args={
			size=1G
		}
	}
	{
		name=date_rolle_file
		type=date_rolle_file
		level_max=0
		level_min=3
		keylist=*
		args={
			filename=%Y-%m-%d.log
		}
	}
	{
		name=mysql
		type=mysql
		level_max=0
		level_min=3
		keylist=*
		args={
			DBNAME=gamelog
		}
	}
	{
		name=syslog
		type=syslog
		level_max=0
		level_min=3
		keylist=*
		args={
		DBNAME=gamelog
		}
	}
	*/

	char* logconf_next_keyval(const char**key, const char**val, char* args);
	char* logconf_next_section(char** args);
	typedef struct logger* (*logger_new)(const char* param);
	int logger_new_register(const char* type, logger_new func);


#define LOG_IMPL(key,level,...) do{logger_log(key,level,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)

#define LOG_FATAL(key,...)	do{logger_log(key,corelog_fatal,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)
#define LOG_ERROR(key,...)	do{logger_log(key,corelog_error,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)
#define LOG_WARN(key,...)	do{logger_log(key,corelog_warnning,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)
#define LOG_INFO(key,...)	do{logger_log(key,corelog_info,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)
#define LOG_DEBUG(key,...)	do{logger_log(key,corelog_debug,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)


#ifdef _cpluscplus
}
#endif

#endif //CORE_LOCK_H