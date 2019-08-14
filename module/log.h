

/*
 *  realize four level log module
 *
 * 从小到大排序 error, warn, info, debug
 * 默认输出级别最大warn, 比warn低可以输出
 *
 * 
 */


#ifndef __MO_LOG_H__
#define __MO_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
};

typedef void (*log_FuncpfnCB_t)(void *ctx, int level, const char *format, ...);

typedef struct logger_s {
    int level;

    void *context;
    log_FuncpfnCB_t log;
} logger_t;

extern logger_t default_logger;

// for user 
#define MO_LOG_DEFAULTLOG_CB  ( log_FuncpfnCB_t )1

#define mo_log(LEVEL, format, args...)    \
        do {                                    \
            const int level = LOG_LEVEL_##LEVEL;   \
            if(default_logger.level >= level ){ \
                void *ctx = default_logger.context; \
                default_logger.log(ctx, (level),format,##args);  \
          }}while(0)

#define mo_logln(LEVEL, format, args...) \
         do {                                    \
             const int level = LOG_LEVEL_##LEVEL;   \
             if(default_logger.level >= level ){ \
                 void *ctx = default_logger.context; \
                 default_logger.log(ctx, (level),format,##args);  \
                 default_logger.log(ctx, (level),"\r\n");   \
           }}while(0)

#define mo_log_set_max_logger_level(LEVEL) (default_logger.level = (LEVEL))

void mo_log_set_logger_callback(log_FuncpfnCB_t log_func);


#ifdef __cplusplus
}  /* extern "C */
#endif

#endif

