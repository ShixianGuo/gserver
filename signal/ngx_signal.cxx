
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>    
#include <errno.h>     
#include <sys/wait.h>  

#include "ngx_global.h"
#include "ngx_macro.h"
#include "ngx_func.h" 

//一个信号有关的结构
typedef struct 
{
    int           signo;       //信号对应的数字编号 ，
    const  char   *signame;   

    //信号处理函数
    void  (*handler)(int signo, siginfo_t *siginfo, void *ucontext); //siginfo_t:系统定义的结构
} ngx_signal_t;


static void ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext); 
static void ngx_process_get_status(void);                                      //获取子进程的结束状态


ngx_signal_t  signals[] = {
    // signo      signame             handler
    { SIGHUP,    "SIGHUP",           ngx_signal_handler },        //终端断开信号，对于守护进程常用于reload重载配置文件通知--标识1
    { SIGINT,    "SIGINT",           ngx_signal_handler },        //标识2   
	{ SIGTERM,   "SIGTERM",          ngx_signal_handler },        //标识15
    { SIGCHLD,   "SIGCHLD",          ngx_signal_handler },        //子进程退出时，父进程会收到这个信号--标识17
    { SIGQUIT,   "SIGQUIT",          ngx_signal_handler },        //标识3
    { SIGIO,     "SIGIO",            ngx_signal_handler },        //指示一个异步I/O事件【通用异步I/O信号】
    { SIGSYS,    "SIGSYS, SIG_IGN",  NULL               },        //我们想忽略这个信号，SIGSYS表示收到了一个无效系统调用 --标识31
                                                               
    { 0,         NULL,               NULL               }         //信号对应的数字至少是1，所以可以用0作为一个特殊标记
};

//初始化信号的函数，用于注册信号处理程序
int ngx_init_signals()
{
    ngx_signal_t      *sig;  
    struct sigaction   sa;   

    for (sig = signals; sig->signo != 0; sig++)  
    {        
        memset(&sa,0,sizeof(struct sigaction));

        if (sig->handler) 
        {
            sa.sa_sigaction = sig->handler;  
            sa.sa_flags = SA_SIGINFO;      //想让sa.sa_sigaction指定的信号处理程序(函数)生效，你就把sa_flags设定为SA_SIGINFO
        }
        else
        {
            sa.sa_handler = SIG_IGN;      
        } //end if

        sigemptyset(&sa.sa_mask);   
                                    
        

        if (sigaction(sig->signo, &sa, NULL) == -1) //参数1：要操作的信号
                                                    //参数2：主要就是那个信号处理函数以及执行信号处理函数时候要屏蔽的信号等等内容
                                                    //参数3：返回以往的对信号的处理方式【跟sigprocmask()函数边的第三个参数是的】
        {   
            ngx_log_error_core(NGX_LOG_EMERG,errno,"sigaction(%s) failed",sig->signame); //显示到日志文件中去的 
            return -1; //有失败就直接返回
        }	
        else
        {            
            //ngx_log_error_core(NGX_LOG_EMERG,errno,"sigaction(%s) succed!",sig->signame);     
            //ngx_log_stderr(0,"sigaction(%s) succed!",sig->signame); 
        }
    } //end for
    return 0; 
}

//信号处理函数
//siginfo：这个系统定义的结构中包含了信号产生原因的有关信息
static void ngx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext)
{    
    //printf("来信号了\n");    
    ngx_signal_t    *sig;    //自定义结构
    char            *action; //用于记录一个动作字符串以往日志文件中写
    
    for (sig = signals; sig->signo != 0; sig++) //遍历信号数组    
    {         
        if (sig->signo == signo) 
        { 
            break;
        }
    } //end for

    action = (char *)"";  //目前还没有什么动作；

    if(ngx_process == NGX_PROCESS_MASTER)      //master进程
    {
        //master进程的往这里走
        switch (signo)
        {
        case SIGCHLD:      //一般子进程退出会收到该信号
            ngx_reap = 1;  
            break;

        //.....其他信号处理以后待增加

        default:
            break;
        } //end switch
    }
    else if(ngx_process == NGX_PROCESS_WORKER) //worker进程，具体干活的进程，处理的信号相对比较少
    {
    
    }
    else
    {
        //非master非worker进程，先啥也不干
        //do nothing
    } //end if(ngx_process == NGX_PROCESS_MASTER)

    if(siginfo && siginfo->si_pid)  //si_pid = sending process ID【发送该信号的进程id】
    {
        ngx_log_error_core(NGX_LOG_NOTICE,0,"signal %d (%s) received from %P%s", signo, sig->signame, siginfo->si_pid, action); 
    }
    else
    {
        ngx_log_error_core(NGX_LOG_NOTICE,0,"signal %d (%s) received %s",signo, sig->signame, action);//没有发送该信号的进程id，所以不显示发送该信号的进程id
    }

    if (signo == SIGCHLD) 
    {
        ngx_process_get_status(); //获取子进程的结束状态
    } //end if

    return;
}

//获取子进程的结束状态，防止单独kill子进程时子进程变成僵尸进程
static void ngx_process_get_status(void)
{
    pid_t            pid;
    int              status;
    int              err;
    int              one=0; //标记信号正常处理过一次

    //当你杀死一个子进程时，父进程会收到这个SIGCHLD信号。
    for ( ;; ) 
    {
        pid = waitpid(-1, &status, WNOHANG); //第一个参数为-1，表示等待任何子进程，
                                             //第二个参数：保存子进程的状态信息
                                             //第三个参数：提供额外选项，WNOHANG表示不要阻塞，让这个waitpid()立即返回        

        if(pid == 0) //子进程没结束，会立即返回这个数字
        {
            return;
        } //end if(pid == 0)
        //-------------------------------
        if(pid == -1)//这表示这个waitpid调用有错
        {
            err = errno;
            if(err == EINTR)           //调用被某个信号中断
            {
                continue;
            }

            if(err == ECHILD  && one)  //没有子进程
            {
                return;
            }

            if (err == ECHILD)         //没有子进程
            {
                ngx_log_error_core(NGX_LOG_INFO,err,"waitpid() failed!");
                return;
            }
            ngx_log_error_core(NGX_LOG_ALERT,err,"waitpid() failed!");
            return;
        }  //end if(pid == -1)
        //-------------------------------
        //走到这里，表示  成功【返回进程id】
        one = 1;  //标记waitpid()返回了正常的返回值
        if(WTERMSIG(status))  //获取使子进程终止的信号编号
        {
            ngx_log_error_core(NGX_LOG_ALERT,0,"pid = %P exited on signal %d!",pid,WTERMSIG(status)); //获取使子进程终止的信号编号
        }
        else
        {
            ngx_log_error_core(NGX_LOG_NOTICE,0,"pid = %P exited with code %d!",pid,WEXITSTATUS(status)); //WEXITSTATUS()获取子进程传递给exit或者_exit参数的低八位
        }
    } //end for
    return;
}
