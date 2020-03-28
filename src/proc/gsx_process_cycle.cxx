
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "gsx_func.h"
#include "gsx_macro.h"
#include "gsx_c_conf.h"

static void gsx_start_worker_processes(int threadnums);
static int gsx_spawn_process(int threadnums, const char *pprocname);
static void gsx_worker_process_cycle(int inum, const char *pprocname);
static void gsx_worker_process_init(int inum);

static u_char master_process[] = "master process";

void gsx_master_process_cycle()
{
    sigset_t set;

    sigemptyset(&set);

    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGWINCH);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGQUIT);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
    {
        gsx_log_error_core(GSX_LOG_ALERT, errno, "gsx_master_process_cycle()中sigprocmask()失败!");
    }

    size_t size;
    int i;
    size = sizeof(master_process);
    size += g_argvneedmem;
    if (size < 1000)
    {
        char title[1000] = {0};
        strcpy(title, (const char *)master_process);
        strcat(title, " ");
        for (i = 0; i < g_os_argc; i++)
        {
            strcat(title, g_os_argv[i]);
        }
        gsx_setproctitle(title);
        gsx_log_error_core(GSX_LOG_NOTICE, 0, "%s %P 【master进程】启动并开始运行......!", title, gsx_pid);
    }

    CConfig *p_config = CConfig::GetInstance();
    int workprocess = p_config->GetIntDefault("WorkerProcesses", 1);
    gsx_start_worker_processes(workprocess);

    sigemptyset(&set);

    for (;;)
    {
        sigsuspend(&set);
        sleep(1);
    }
    return;
}

static void gsx_start_worker_processes(int threadnums)
{
    int i;
    for (i = 0; i < threadnums; i++)
    {
        gsx_spawn_process(i, "worker process");
    }
    return;
}

static int gsx_spawn_process(int inum, const char *pprocname)
{
    pid_t pid;

    pid = fork();
    switch (pid)
    {
    case -1:
        gsx_log_error_core(GSX_LOG_ALERT, errno, "gsx_spawn_process()fork()产生子进程num=%d,procname=\"%s\"失败!", inum, pprocname);
        return -1;

    case 0:
        gsx_parent = gsx_pid;
        gsx_pid = getpid();
        gsx_worker_process_cycle(inum, pprocname);
        break;

    default:
        break;
    }

    return pid;
}

static void gsx_worker_process_cycle(int inum, const char *pprocname)
{

    gsx_process = GSX_PROCESS_WORKER;

    gsx_worker_process_init(inum);
    gsx_setproctitle(pprocname);
    gsx_log_error_core(GSX_LOG_NOTICE, 0, "%s %P 【worker进程】启动并开始运行......!", pprocname, gsx_pid);

    for (;;)
    {

        gsx_process_events_and_timers();

        /*if(false) 
        {
            g_stopEvent = 1;
            break;
        }*/
    }

    g_threadpool.StopAll();
    g_socket.Shutdown_subproc();
    return;
}

static void gsx_worker_process_init(int inum)
{
    sigset_t set;

    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
    {
        gsx_log_error_core(GSX_LOG_ALERT, errno, "gsx_worker_process_init()中sigprocmask()失败!");
    }

    CConfig *p_config = CConfig::GetInstance();
    int tmpthreadnums = p_config->GetIntDefault("ProcMsgRecvWorkThreadCount", 5);
    if (g_threadpool.Create(tmpthreadnums) == false)
    {
        exit(-2);
    }
    sleep(1);

    if (g_socket.Initialize_subproc() == false)
    {
        exit(-2);
    }

    g_socket.gsx_epoll_init();

    return;
}
