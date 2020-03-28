
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>    
#include <errno.h>     
#include <sys/wait.h>  

#include "gsx_global.h"
#include "gsx_macro.h"
#include "gsx_func.h" 


typedef struct 
{
    int           signo;       
    const  char   *signame;    
    void  (*handler)(int signo, siginfo_t *siginfo, void *ucontext); 
} gsx_signal_t;


static void gsx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext); 
static void gsx_process_get_status(void);                                      



gsx_signal_t  signals[] = {
    
    { SIGHUP,    "SIGHUP",           gsx_signal_handler },        
    { SIGINT,    "SIGINT",           gsx_signal_handler },        
	{ SIGTERM,   "SIGTERM",          gsx_signal_handler },        
    { SIGCHLD,   "SIGCHLD",          gsx_signal_handler },        
    { SIGQUIT,   "SIGQUIT",          gsx_signal_handler },        
    { SIGIO,     "SIGIO",            gsx_signal_handler },        
    { SIGSYS,    "SIGSYS, SIG_IGN",  NULL               },        
                                                                  
    
    { 0,         NULL,               NULL               }         
};



int gsx_init_signals()
{
    gsx_signal_t      *sig;  
    struct sigaction   sa;   

    for (sig = signals; sig->signo != 0; sig++)  
    {        
        
        memset(&sa,0,sizeof(struct sigaction));

        if (sig->handler)  
        {
            sa.sa_sigaction = sig->handler;  
            sa.sa_flags = SA_SIGINFO;                                                     
        }
        else
        {
            sa.sa_handler = SIG_IGN;                                 
        } 

        sigemptyset(&sa.sa_mask);   
                                    
                                    
        
        
        if (sigaction(sig->signo, &sa, NULL) == -1)                                               
        {   
            gsx_log_error_core(GSX_LOG_EMERG,errno,"sigaction(%s) failed",sig->signame); 
            return -1; 
        }	
        else
        {            
            
            
        }
    } 
    return 0; 
}



static void gsx_signal_handler(int signo, siginfo_t *siginfo, void *ucontext)
{    
    
    gsx_signal_t    *sig;    
    char            *action; 
    
    for (sig = signals; sig->signo != 0; sig++) 
    {         
        
        if (sig->signo == signo) 
        { 
            break;
        }
    } 

    action = (char *)"";  

    if(gsx_process == GSX_PROCESS_MASTER)      
    {
        
        switch (signo)
        {
        case SIGCHLD:  
            gsx_reap = 1;  
            break;

        

        default:
            break;
        } 
    }
    else if(gsx_process == GSX_PROCESS_WORKER) 
    {
        
        
        
    }
    else
    {
        
        
    } 

    
    
    if(siginfo && siginfo->si_pid)  
    {
        gsx_log_error_core(GSX_LOG_NOTICE,0,"signal %d (%s) received from %P%s", signo, sig->signame, siginfo->si_pid, action); 
    }
    else
    {
        gsx_log_error_core(GSX_LOG_NOTICE,0,"signal %d (%s) received %s",signo, sig->signame, action);
    }

    

    
    if (signo == SIGCHLD) 
    {
        gsx_process_get_status(); 
    } 

    return;
}


static void gsx_process_get_status(void)
{
    pid_t            pid;
    int              status;
    int              err;
    int              one=0; 

    
    for ( ;; ) 
    {
        
        
        
        pid = waitpid(-1, &status, WNOHANG); 
                                              
                                               

        if(pid == 0) 
        {
            return;
        } 
        
        if(pid == -1)
        {
            
            err = errno;
            if(err == EINTR)           
            {
                continue;
            }

            if(err == ECHILD  && one)  
            {
                return;
            }

            if (err == ECHILD)         
            {
                gsx_log_error_core(GSX_LOG_INFO,err,"waitpid() failed!");
                return;
            }
            gsx_log_error_core(GSX_LOG_ALERT,err,"waitpid() failed!");
            return;
        }  
        
        
        one = 1;  
        if(WTERMSIG(status))  
        {
            gsx_log_error_core(GSX_LOG_ALERT,0,"pid = %P exited on signal %d!",pid,WTERMSIG(status)); 
        }
        else
        {
            gsx_log_error_core(GSX_LOG_NOTICE,0,"pid = %P exited with code %d!",pid,WEXITSTATUS(status)); 
        }
    } 
    return;
}
