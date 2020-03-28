

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "gsx_func.h"
#include "gsx_macro.h"
#include "gsx_c_conf.h"

int gsx_daemon()
{

    switch (fork())
    {
    case -1:

        gsx_log_error_core(GSX_LOG_EMERG, errno, "gsx_daemon()中fork()失败!");
        return -1;
    case 0:

        break;
    default:

        return 1;
    }

    gsx_parent = gsx_pid;
    gsx_pid = getpid();

    if (setsid() == -1)
    {
        gsx_log_error_core(GSX_LOG_EMERG, errno, "gsx_daemon()中setsid()失败!");
        return -1;
    }

    umask(0);

    int fd = open("/dev/null", O_RDWR);
    if (fd == -1)
    {
        gsx_log_error_core(GSX_LOG_EMERG, errno, "gsx_daemon()中open(\"/dev/null\")失败!");
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1)
    {
        gsx_log_error_core(GSX_LOG_EMERG, errno, "gsx_daemon()中dup2(STDIN)失败!");
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1)
    {
        gsx_log_error_core(GSX_LOG_EMERG, errno, "gsx_daemon()中dup2(STDOUT)失败!");
        return -1;
    }
    if (fd > STDERR_FILENO)
    {
        if (close(fd) == -1)
        {
            gsx_log_error_core(GSX_LOG_EMERG, errno, "gsx_daemon()中close(fd)失败!");
            return -1;
        }
    }
    return 0;
}
