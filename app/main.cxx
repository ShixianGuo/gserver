#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h> 



#include "ngx_macro.h"  //各种宏定义
#include "ngx_func.h"    //各种函数声明
#include "ngx_c_conf.h"  //和配置文件处理相关的类,名字带c_表示和类有关
#include "ngx_global.h"


//和设置标题有关的全局量
size_t  g_argvneedmem=0;        //保存下这些argv参数所需要的内存大小
size_t  g_envneedmem=0;         //环境变量所占内存大小
int     g_os_argc;              //参数个数 
char    **g_os_argv;            //原始命令行参数数组,在main中会被赋值
char    *gp_envmem=NULL;        //指向自己分配的env环境变量的内存
int     g_daemonized=0;         //守护进程标记 0：未启用，1：启用了


//和进程本身有关的全局量
pid_t   ngx_pid;                //当前进程的pid
pid_t   ngx_parent;             //父进程的pid
int     ngx_process;            //进程类型，比如master,worker进程等

sig_atomic_t  ngx_reap;         //标记子进程状态变化


void global_init(int argc, const char** argv){
    ngx_pid    = getpid();      //取得进程pid
    ngx_parent = getppid();     //取得父进程的id 

    //统计argv所占的内存
    g_argvneedmem = 0;
    for(int i = 0; i < argc; i++)  //argv =  ./nginx -a -b -c asdfas
    {
        g_argvneedmem += strlen(argv[i]) + 1; //+1是给\0留空间。
    } 

    //统计环境变量所占的内存
    for(int i = 0; environ[i]; i++) 
    {
        g_envneedmem += strlen(environ[i]) + 1; //+1是因为末尾有\0,是占实际内存位置的，要算进来
    } //end for

    g_os_argc = argc;           //保存参数个数
    g_os_argv = (char **) argv; //保存参数指针

    ngx_log.fd = -1;                  //-1：表示日志文件尚未打开
    ngx_process = NGX_PROCESS_MASTER; //先标记本进程是master进程
    ngx_reap = 0;                     //标记子进程没有发生变化

}



void freeresource()
{
    if(gp_envmem)
    {
        delete []gp_envmem;
        gp_envmem = NULL;
    }

    if(ngx_log.fd != STDERR_FILENO && ngx_log.fd != -1)  
    {        
        close(ngx_log.fd);
        ngx_log.fd = -1;    
    }
}


int main(int argc, const char** argv) {
    int exitcode = 0;    

    global_init(argc,argv);

    CConfig *p_config = CConfig::GetInstance(); 
    if(p_config->Load("nginx.conf") == false)    
    {   
        ngx_log_init();    
        ngx_log_stderr(0,"配置文件[%s]载入失败，退出!","nginx.conf");
        exitcode = 2;     
        goto lblexit;
    }

    ngx_log_init();  
   
    if(ngx_init_signals() != 0) //信号初始化
    {
        exitcode = 1;
        goto lblexit;
    }     

    ngx_init_setproctitle();  

    if(p_config->GetIntDefault("Daemon",0) == 1) {
        int cdaemonresult = ngx_daemon();
        if(cdaemonresult == -1) 
        {
            exitcode = 1;  
            goto lblexit;
        }
        if(cdaemonresult == 1)
        {
            //这是原始的父进程
            freeresource();

            exitcode = 0;
            return exitcode;  
        }

        g_daemonized = 1;    //守护进程标记，

    }

    ngx_master_process_cycle(); 

 lblexit:
    //(5)该释放的资源要释放掉
    ngx_log_stderr(0,"程序退出，再见了!");
    freeresource();  //一系列的main返回前的释放动作函数
    //printf("程序退出，再见!\n");    
    return exitcode;   
    
    return 0;
}


