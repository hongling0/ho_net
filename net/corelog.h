#ifndef CORE_LOG_H
#define CORE_LOG_H

#ifdef _cpluscplus
extern "C" {
#endif

	enum corelog_level
	{
		corelog_fatal,
		corelog_error,
		corelog_warnning,
		corelog_info,
		corelog_debug,
	};
	void logger_log(const char* key, int level, const char* file, const char* func, int line, int thr, const char* fmt, ...);

#define LOG(key,level,...) do{logger_log(key,level,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)

#define LOG_FATAL(key,...)	do{logger_log(key,corelog_fatal,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)
#define LOG_ERROR(key,...)	do{logger_log(key,corelog_error,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)
#define LOG_WARN(key,...)	do{logger_log(key,corelog_warnning,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)
#define LOG_INFO(key,...)	do{logger_log(key,corelog_info,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)
#define LOG_DEBUG(key,...)	do{logger_log(key,corelog_debug,__FILE__,__FUNCTION__,__LINE__,0,__VA_ARGS__)  } while(0)

#ifdef _cpluscplus
}
#endif

#endif //CORE_LOG_H