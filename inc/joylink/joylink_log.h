#ifndef __JOYLINK_LOG_H__
#define __JOYLINK_LOG_H__

#include <stdio.h>

#if defined(__RT_THREAD__)
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <rtthread.h>
#elif defined(__MTK_7687__)
#else
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <time.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define JL_LOG_LEVEL_FATAL      (0)
#define JL_LOG_LEVEL_NOTICE     (1)
#define JL_LOG_LEVEL_INFO       (2)
#define JL_LOG_LEVEL_ERROR      (3)
#define JL_LOG_LEVEL_WARN       (4)
#define JL_LOG_LEVEL_DEBUG      (5)

#define Black   0;30
#define Red     0;31
#define Green   0;32
#define Brown   0;33
#define Blue    0;34
#define Purple  0;35
#define Cyan    0;36

#ifdef __RT_THREAD__
#define DBG_TAG              "jl"
#define DBG_LVL              DBG_LOG
#include <rtdbg.h>

#if defined(JOYLINK_OUTPUT_LEVEL_DEBUG)
#define JL_LOG_LEVEL         JL_LOG_LEVEL_DEBUG
#elif defined(JOYLINK_OUTPUT_LEVEL_WARN)
#define JL_LOG_LEVEL         JL_LOG_LEVEL_WARN
#elif defined(JOYLINK_OUTPUT_LEVEL_ERROR)
#define JL_LOG_LEVEL         JL_LOG_LEVEL_ERROR
#elif defined(JOYLINK_OUTPUT_LEVEL_INFO)
#define JL_LOG_LEVEL         JL_LOG_LEVEL_INFO
#elif defined(JOYLINK_OUTPUT_LEVEL_NOTICE)
#define JL_LOG_LEVEL         JL_LOG_LEVEL_NOTICE
#elif defined(JOYLINK_OUTPUT_LEVEL_FATAL)
#define JL_LOG_LEVEL         JL_LOG_LEVEL_FATAL
#endif 

#endif

#define log_fatal(format, ...) \
    do{\
        if(JL_LOG_LEVEL >= JL_LOG_LEVEL_FATAL){\
            LOG_I("[%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }\
    }while(0)

#define log_notice(format, ...) \
    do{\
        if(JL_LOG_LEVEL >= JL_LOG_LEVEL_NOTICE){\
            LOG_I("[%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }\
    }while(0)

#define log_info(format, ...) \
    do{\
        if(JL_LOG_LEVEL >= JL_LOG_LEVEL_INFO){\
            LOG_I("[%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }\
    }while(0)

#define log_error(format, ...) \
    do{\
        if(JL_LOG_LEVEL >= JL_LOG_LEVEL_ERROR){\
            LOG_E("[%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }\
    }while(0)

#define log_warn(format, ...) \
    do{\
        if(JL_LOG_LEVEL >= JL_LOG_LEVEL_WARN){\
            LOG_W("[%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }\
    }while(0)

#define log_debug(format, ...) \
    do{\
        if(JL_LOG_LEVEL >= JL_LOG_LEVEL_DEBUG){\
            LOG_D("[%s][%d] " format, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
        }\
    }while(0)

#ifdef __cplusplus
}
#endif

#endif /* __LOGGING_H__ */
