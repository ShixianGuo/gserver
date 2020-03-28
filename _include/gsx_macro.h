
#ifndef __GSX_MACRO_H__
#define __GSX_MACRO_H__

#define GSX_MAX_ERROR_STR 2048
#define gsx_cpymem(dst, src, n) (((u_char *)memcpy(dst, src, n)) + (n)) #define gsx_min(val1, val2)((val1 > val2) ? (val2) : (val1))
#define GSX_MAX_UINT32_VALUE (uint32_t)0xffffffff #define GSX_INT64_LEN(sizeof("-9223372036854775808") - 1)

#define GSX_LOG_STDERR 0 #define GSX_LOG_EMERG 1 #define GSX_LOG_ALERT 2 #define GSX_LOG_CRIT 3 #define GSX_LOG_ERR 4 #define GSX_LOG_WARN 5 #define GSX_LOG_NOTICE 6 #define GSX_LOG_INFO 7 #define GSX_LOG_DEBUG 8
#define GSX_ERROR_LOG_PATH "error.log"
#define GSX_PROCESS_MASTER 0 #define GSX_PROCESS_WORKER 1

#endif
