
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

#include "gsx_global.h"
#include "gsx_macro.h"
#include "gsx_func.h"
#include "gsx_c_conf.h"

static u_char err_levels[][20] =
    {
        {"stderr"},
        {"emerg"},
        {"alert"},
        {"crit"},
        {"error"},
        {"warn"},
        {"notice"},
        {"info"},
        {"debug"}};
gsx_log_t gsx_log;

/* 
    gsx_log_stderr(0, "invalid option: \"%s\"", argv[0]);  
    gsx_log_stderr(0, "invalid option: %10d", 21);         
    gsx_log_stderr(0, "invalid option: %.6f", 21.378);     
    gsx_log_stderr(0, "invalid option: %.6f", 12.999);     
    gsx_log_stderr(0, "invalid option: %.2f", 12.999);     
    gsx_log_stderr(0, "invalid option: %xd", 1678);        
    gsx_log_stderr(0, "invalid option: %Xd", 1678);        
    gsx_log_stderr(15, "invalid option: %s , %d", "testInfo",326);        
    gsx_log_stderr(0, "invalid option: %d", 1678); 
    */
void gsx_log_stderr(int err, const char *fmt, ...)
{
    va_list args;
    u_char errstr[GSX_MAX_ERROR_STR + 1];
    u_char *p, *last;

    memset(errstr, 0, sizeof(errstr));

    last = errstr + GSX_MAX_ERROR_STR;

    p = gsx_cpymem(errstr, "nginx: ", 7);

    va_start(args, fmt);
    p = gsx_vslprintf(p, last, fmt, args);
    va_end(args);

    if (err)
    {

        p = gsx_log_errno(p, last, err);
    }

    if (p >= (last - 1))
    {
        p = (last - 1) - 1;
    }
    *p++ = '\n';

    write(STDERR_FILENO, errstr, p - errstr);

    if (gsx_log.fd > STDERR_FILENO)
    {

        err = 0;
        p--;
        *p = 0;
        gsx_log_error_core(GSX_LOG_STDERR, err, (const char *)errstr);
    }
    return;
}

u_char *gsx_log_errno(u_char *buf, u_char *last, int err)
{

    char *perrorinfo = strerror(err);
    size_t len = strlen(perrorinfo);

    char leftstr[10] = {0};
    sprintf(leftstr, " (%d: ", err);
    size_t leftlen = strlen(leftstr);

    char rightstr[] = ") ";
    size_t rightlen = strlen(rightstr);

    size_t extralen = leftlen + rightlen;
    if ((buf + len + extralen) < last)
    {

        buf = gsx_cpymem(buf, leftstr, leftlen);
        buf = gsx_cpymem(buf, perrorinfo, len);
        buf = gsx_cpymem(buf, rightstr, rightlen);
    }
    return buf;
}

void gsx_log_error_core(int level, int err, const char *fmt, ...)
{
    u_char *last;
    u_char errstr[GSX_MAX_ERROR_STR + 1];

    memset(errstr, 0, sizeof(errstr));
    last = errstr + GSX_MAX_ERROR_STR;

    struct timeval tv;
    struct tm tm;
    time_t sec;
    u_char *p;
    va_list args;

    memset(&tv, 0, sizeof(struct timeval));
    memset(&tm, 0, sizeof(struct tm));

    gettimeofday(&tv, NULL);

    sec = tv.tv_sec;
    localtime_r(&sec, &tm);
    tm.tm_mon++;
    tm.tm_year += 1900;

    u_char strcurrtime[40] = {0};
    gsx_slprintf(strcurrtime,
                 (u_char *)-1,
                 "%4d/%02d/%02d %02d:%02d:%02d",
                 tm.tm_year, tm.tm_mon,
                 tm.tm_mday, tm.tm_hour,
                 tm.tm_min, tm.tm_sec);
    p = gsx_cpymem(errstr, strcurrtime, strlen((const char *)strcurrtime));
    p = gsx_slprintf(p, last, " [%s] ", err_levels[level]);
    p = gsx_slprintf(p, last, "%P: ", gsx_pid);

    va_start(args, fmt);
    p = gsx_vslprintf(p, last, fmt, args);
    va_end(args);

    if (err)
    {

        p = gsx_log_errno(p, last, err);
    }

    if (p >= (last - 1))
    {
        p = (last - 1) - 1;
    }
    *p++ = '\n';

    ssize_t n;
    while (1)
    {
        if (level > gsx_log.log_level)
        {

            break;
        }

        n = write(gsx_log.fd, errstr, p - errstr);
        if (n == -1)
        {

            if (errno == ENOSPC)
            {
            }
            else
            {

                if (gsx_log.fd != STDERR_FILENO)
                {
                    n = write(STDERR_FILENO, errstr, p - errstr);
                }
            }
        }
        break;
    }
    return;
}

void gsx_log_init()
{
    u_char *plogname = NULL;
    size_t nlen;

    CConfig *p_config = CConfig::GetInstance();
    plogname = (u_char *)p_config->GetString("Log");
    if (plogname == NULL)
    {

        plogname = (u_char *)GSX_ERROR_LOG_PATH;
    }
    gsx_log.log_level = p_config->GetIntDefault("LogLevel", GSX_LOG_NOTICE);

    gsx_log.fd = open((const char *)plogname, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (gsx_log.fd == -1)
    {
        gsx_log_stderr(errno, "[alert] could not open error log file: open() \"%s\" failed", plogname);
        gsx_log.fd = STDERR_FILENO;
    }
    return;
}
