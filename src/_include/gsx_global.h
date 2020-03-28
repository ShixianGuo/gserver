
#ifndef __GSX_GBLDEF_H__
#define __GSX_GBLDEF_H__

#include <signal.h>

#include "gsx_c_slogic.h"
#include "gsx_c_threadpool.h"

typedef struct _CConfItem
{
	char ItemName[50];
	char ItemContent[500];
} CConfItem, *LPCConfItem;

typedef struct
{
	int log_level;
	int fd;
} gsx_log_t;

extern size_t g_argvneedmem;
extern size_t g_envneedmem;
extern int g_os_argc;
extern char **g_os_argv;
extern char *gp_envmem;
extern int g_daemonized;
extern CLogicSocket g_socket;
extern CThreadPool g_threadpool;

extern pid_t gsx_pid;
extern pid_t gsx_parent;
extern gsx_log_t gsx_log;
extern int gsx_process;
extern sig_atomic_t gsx_reap;
extern int g_stopEvent;

#endif
