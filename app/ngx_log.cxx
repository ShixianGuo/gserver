//和日志相关的函数放之类

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>    //uintptr_t
#include <stdarg.h>    //va_start....
#include <unistd.h>    //STDERR_FILENO等
#include <sys/time.h>  //gettimeofday
#include <time.h>      //localtime_r
#include <fcntl.h>     //open
#include <errno.h>     //errno

#include "ngx_global.h"
#include "ngx_macro.h"
#include "ngx_func.h"
#include "ngx_c_conf.h"

static u_char err_levels[][20]  = 
{
    {"stderr"},    //0：控制台错误
    {"emerg"},     //1：紧急
    {"alert"},     //2：警戒
    {"crit"},      //3：严重
    {"error"},     //4：错误
    {"warn"},      //5：警告
    {"notice"},    //6：注意
    {"info"},      //7：信息
    {"debug"}      //8：调试
};
ngx_log_t   ngx_log;

void ngx_log_stderr(int err, const char *fmt, ...)
{    
    va_list args;                        
    u_char  errstr[NGX_MAX_ERROR_STR+1]; 
    u_char  *p,*last;

    memset(errstr,0,sizeof(errstr));     

    last = errstr + NGX_MAX_ERROR_STR;        //last指向整个buffer最后
                                             
                                                
    p = ngx_cpymem(errstr, "nginx: ", 7);     //p指向"nginx: "之后    
    
    va_start(args, fmt); 
    p = ngx_vslprintf(p,last,fmt,args); 
    va_end(args);        

    if (err)  
    {
        p = ngx_log_errno(p, last, err);
    }
    
    //若位置不够，那换行也要硬插入到末尾，哪怕覆盖到其他内容    
    if (p >= (last - 1))
    {
        p = (last - 1) - 1; //把尾部空格留出来
                         
    }
    *p++ = '\n'; //增加个换行符    
    
    //往标准错误【一般是屏幕】输出信息    
    write(STDERR_FILENO,errstr,p - errstr);

    if(ngx_log.fd > STDERR_FILENO) 
    {
        ngx_log_error_core(NGX_LOG_STDERR,err,(const char *)errstr); 
    }    
    return;
}

u_char *ngx_log_errno(u_char *buf, u_char *last, int err)
{
    //以下代码是我自己改造，感觉作者的代码有些瑕疵
    char *perrorinfo = strerror(err); //根据资料不会返回NULL;
    size_t len = strlen(perrorinfo);

    //然后我还要插入一些字符串： (%d:)  
    char leftstr[10] = {0}; 
    sprintf(leftstr," (%d: ",err);
    size_t leftlen = strlen(leftstr);

    char rightstr[] = ") "; 
    size_t rightlen = strlen(rightstr);
    
    size_t extralen = leftlen + rightlen; //左右的额外宽度
    if ((buf + len + extralen) < last)
    {
        //保证整个我装得下，我就装，否则我全部抛弃 ,nginx的做法是 如果位置不够，就硬留出50个位置【哪怕覆盖掉以往的有效内容】，也要硬往后边塞，这样当然也可以；
        buf = ngx_cpymem(buf, leftstr, leftlen);
        buf = ngx_cpymem(buf, perrorinfo, len);
        buf = ngx_cpymem(buf, rightstr, rightlen);
    }
    return buf;
}

void ngx_log_error_core(int level,  int err, const char *fmt, ...)
{
    u_char  *last;
    u_char  errstr[NGX_MAX_ERROR_STR+1];   //这个+1也是我放入进来的，本函数可以参考ngx_log_stderr()函数的写法；

    memset(errstr,0,sizeof(errstr));  
    last = errstr + NGX_MAX_ERROR_STR;   
    
    struct timeval   tv;
    struct tm        tm;
    time_t           sec;   //秒
    u_char           *p;    //指向当前要拷贝数据到其中的内存位置
    va_list          args;

    memset(&tv,0,sizeof(struct timeval));    
    memset(&tm,0,sizeof(struct tm));

    gettimeofday(&tv, NULL);     //获取当前时间，返回自1970-01-01 00:00:00到现在经历的秒数【第二个参数是时区，一般不关心】        

    sec = tv.tv_sec;             //秒
    localtime_r(&sec, &tm);      //把参数1的time_t转换为本地时间，保存到参数2中去，带_r的是线程安全的版本，尽量使用
    tm.tm_mon++;                 //月份要调整下正常
    tm.tm_year += 1900;          //年份要调整下才正常
    
    u_char strcurrtime[40]={0};  //先组合出一个当前时间字符串，格式形如：2019/01/08 19:57:11
    ngx_slprintf(strcurrtime,  
                    (u_char *)-1,                       //若用一个u_char *接一个 (u_char *)-1,则 得到的结果是 0xffffffff....，这个值足够大
                    "%4d/%02d/%02d %02d:%02d:%02d",     //格式是 年/月/日 时:分:秒
                    tm.tm_year, tm.tm_mon,
                    tm.tm_mday, tm.tm_hour,
                    tm.tm_min, tm.tm_sec);
    p = ngx_cpymem(errstr,strcurrtime,strlen((const char *)strcurrtime));  //日期增加进来，得到形如：     2019/01/08 20:26:07
    p = ngx_slprintf(p, last, " [%s] ", err_levels[level]);                //日志级别增加进来，得到形如：  2019/01/08 20:26:07 [crit] 
    p = ngx_slprintf(p, last, "%P: ",ngx_pid);                             //支持%P格式，进程id增加进来，得到形如：   2019/01/08 20:50:15 [crit] 2037:

    va_start(args, fmt);                     //使args指向起始的参数
    p = ngx_vslprintf(p, last, fmt, args);   //把fmt和args参数弄进去，组合出来这个字符串
    va_end(args);                            //释放args 

    if (err)  //如果错误代码不是0，表示有错误发生
    {
        //错误代码和错误信息也要显示出来
        p = ngx_log_errno(p, last, err);
    }
    //若位置不够，那换行也要硬插入到末尾，哪怕覆盖到其他内容
    if (p >= (last - 1))
    {
        p = (last - 1) - 1; //把尾部空格留出来，这里感觉nginx处理的似乎就不对 
                             //我觉得，last-1，才是最后 一个而有效的内存，而这个位置要保存\0，所以我认为再减1，这个位置，才适合保存\n
    }
    *p++ = '\n'; //增加个换行符       

    //这么写代码是图方便：随时可以把流程弄到while后边去；大家可以借鉴一下这种写法
    ssize_t   n;
    while(1) 
    {        
        if (level > ngx_log.log_level) 
        {
            //要打印的这个日志的等级太落后（等级数字太大，比配置文件中的数字大)
            //这种日志就不打印了
            break;
        }
        //磁盘是否满了的判断，先算了吧，还是由管理员保证这个事情吧； 

        //写日志文件        
        n = write(ngx_log.fd,errstr,p - errstr);  //文件写入成功后，如果中途
        if (n == -1) 
        {
            //写失败有问题
            if(errno == ENOSPC) //写失败，且原因是磁盘没空间了
            {
                //磁盘没空间了
                //没空间还写个毛线啊
                //先do nothing吧；
            }
            else
            {
                //这是有其他错误，那么我考虑把这个错误显示到标准错误设备吧；
                if(ngx_log.fd != STDERR_FILENO) //当前是定位到文件的，则条件成立
                {
                    n = write(STDERR_FILENO,errstr,p - errstr);
                }
            }
        }
        break;
    } //end while    
    return;
}

void ngx_log_init()
{
    u_char *plogname = NULL;
    size_t nlen;

    //从配置文件中读取和日志相关的配置信息
    CConfig *p_config = CConfig::GetInstance();
    plogname = (u_char *)p_config->GetString("Log");
    if(plogname == NULL)
    {
        plogname = (u_char *) NGX_ERROR_LOG_PATH; //"logs/error.log" ,logs目录需要提前建立出来
    }

    ngx_log.log_level = p_config->GetIntDefault("LogLevel",NGX_LOG_NOTICE);//缺省日志等级为6
   
    ngx_log.fd = open((const char *)plogname,O_WRONLY|O_APPEND|O_CREAT,0644);  
    if (ngx_log.fd == -1)  //如果有错误，则直接定位到 标准错误上去 
    {
        ngx_log_stderr(errno,"[alert] could not open error log file: open() \"%s\" failed", plogname);
        ngx_log.fd = STDERR_FILENO; 
    } 
    return;
}
