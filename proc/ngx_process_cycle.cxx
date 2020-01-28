
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>   //信号相关头文件 
#include <errno.h>    //errno
#include <unistd.h>

#include "ngx_func.h"
#include "ngx_macro.h"
#include "ngx_c_conf.h"

//函数声明
static void ngx_start_worker_processes(int threadnums);
static int ngx_spawn_process(int threadnums,const char *pprocname);
static void ngx_worker_process_cycle(int inum,const char *pprocname);
static void ngx_worker_process_init(int inum);

//变量声明
static u_char  master_process[] = "master process";

//描述：创建worker子进程
void ngx_master_process_cycle()
{    
    sigset_t set;        //信号集

    sigemptyset(&set);   //清空信号集

   
    sigaddset(&set, SIGCHLD);     //子进程状态改变
    sigaddset(&set, SIGALRM);     //定时器超时
    sigaddset(&set, SIGIO);       //异步I/O
    sigaddset(&set, SIGINT);      //终端中断符
    sigaddset(&set, SIGHUP);      //连接断开
    sigaddset(&set, SIGUSR1);     //用户定义信号
    sigaddset(&set, SIGUSR2);     //用户定义信号
    sigaddset(&set, SIGWINCH);    //终端窗口大小改变
    sigaddset(&set, SIGTERM);     //终止
    sigaddset(&set, SIGQUIT);     //终端退出符
  
    //设置，此时无法接受的信号；阻塞期间，你发过来的上述信号，多个会被合并为一个，暂存着，等你放开信号屏蔽后才能收到这些信号
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) 
    {        
        ngx_log_error_core(NGX_LOG_ALERT,errno,"ngx_master_process_cycle()中sigprocmask()失败!");
    }
   
    //设置主进程标题---------begin
    size_t size;
    int    i;
    size = sizeof(master_process);  
    size += g_argvneedmem;          
    if(size < 1000)
    {
        char title[1000] = {0};
        strcpy(title,(const char *)master_process); //"master process"
        strcat(title," ");  
        for (i = 0; i < g_os_argc; i++)        
        {
            strcat(title,g_os_argv[i]);
        }//end for
        ngx_setproctitle(title); //设置标题
        ngx_log_error_core(NGX_LOG_NOTICE,0,"%s %P 启动并开始运行......!",title,ngx_pid); 
    }    
    //设置主进程标题---------end
        
   
    CConfig *p_config = CConfig::GetInstance(); 
    int workprocess = p_config->GetIntDefault("WorkerProcesses",1); //从配置文件中得到要创建的worker进程数量
    ngx_start_worker_processes(workprocess);  //这里要创建worker子进程

    //创建子进程后，父进程的执行流程会返回到这里，子进程不会走进来    
    sigemptyset(&set); //信号屏蔽字为空，表示不屏蔽任何信号
    
    for ( ;; ) 
    {



        sigsuspend(&set); //阻塞在这里，等待一个信号，此时进程是挂起的，不占用cpu时间，只有收到信号才会被唤醒（返回）；
 
        sleep(1); 
     
    }// end for(;;)
    return;
}

//描述：根据给定的参数创建指定数量的子进程，因为以后可能要扩展功能，增加参数，所以单独写成一个函数
//threadnums:要创建的子进程数量
static void ngx_start_worker_processes(int threadnums)
{
    int i;
    for (i = 0; i < threadnums; i++)  
    {
        ngx_spawn_process(i,"worker process");
    } //end for
    return;
}

//描述：产生一个子进程
//inum：进程编号【0开始】
//pprocname：子进程名字"worker process"
static int ngx_spawn_process(int inum,const char *pprocname)
{
    pid_t  pid;

    pid = fork(); //fork()系统调用产生子进程
    switch (pid)  //pid判断父子进程，分支处理
    {  
    case -1: //产生子进程失败
        ngx_log_error_core(NGX_LOG_ALERT,errno,"ngx_spawn_process()fork()产生子进程num=%d,procname=\"%s\"失败!",inum,pprocname);
        return -1;

    case 0:  //子进程分支
        ngx_parent = ngx_pid;              //因为是子进程了，所有原来的pid变成了父pid
        ngx_pid = getpid();                //重新获取pid,即本子进程的pid
        ngx_worker_process_cycle(inum,pprocname);    //我希望所有worker子进程，在这个函数里不断循环着不出来，也就是说，子进程流程不往下边走;
        break;

    default: //这个应该是父进程分支，直接break;，流程往switch之后走            
        break;
    }//end switch

    return pid;
}

//描述：worker子进程的功能函数，每个woker子进程，就在这里循环着了（无限循环【处理网络事件和定时器事件以对外提供web服务】）
static void ngx_worker_process_cycle(int inum,const char *pprocname) 
{
    //设置一下变量
    ngx_process = NGX_PROCESS_WORKER;  //设置进程的类型，是worker进程

    //重新为子进程设置进程名，不要与父进程重复------
    ngx_worker_process_init(inum);
    ngx_setproctitle(pprocname); //设置标题   
    ngx_log_error_core(NGX_LOG_NOTICE,0,"%s %P 启动并开始运行......!",pprocname,ngx_pid); 
    for(;;)
    {
        sleep(1); //休息1秒       
   
    } //end for(;;)
    return;
}

//描述：子进程创建时调用本函数进行一些初始化工作
static void ngx_worker_process_init(int inum)
{
    sigset_t  set;      //信号集

    sigemptyset(&set);  //清空信号集
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)  //原来是屏蔽那10个信号【防止fork()期间收到信号导致混乱】，现在不再屏蔽任何信号【接收任何信号】
    {
        ngx_log_error_core(NGX_LOG_ALERT,errno,"ngx_worker_process_init()中sigprocmask()失败!");
    }

    return;
}
