

#include <stdio.h>
#include <stdarg.h>
#include "log.h"

// 默认输出到stdout
static void mo_log_defaultlog_callback(void *ctx, int level, const char *format,...)
{
    va_list ap;
    
    (void)ctx;
    (void)level;

    va_start(ap,format);
	vprintf(format,ap);
	va_end(ap);
}

//将所以信息默认不输出
static void mo_log_nulllog_callback(void *ctx, int level, const char *format,...)
{
    (void)ctx;
    (void)level;
    (void)format;
}

logger_t default_logger = {
	LOG_LEVEL_WARN,
    NULL,
	mo_log_defaultlog_callback
};

/*********************************************************************
 * @fn          
 *
 * @brief       注册一个
 *
 * @param       log_Funcpfn_t - function pointer to read/write routine
 *
 * @return      
 */
void mo_log_set_logger_callback(log_FuncpfnCB_t log_func)
{
    if (NULL == log_func)
        log_func = mo_log_nulllog_callback;
    else if(log_func == MO_LOG_DEFAULTLOG_CB)
        log_func = mo_log_defaultlog_callback;

    default_logger.log = log_func;
}

