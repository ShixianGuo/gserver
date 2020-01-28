

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>  

#include "ngx_global.h"
#include "ngx_macro.h"
#include "ngx_func.h"


static u_char *ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64,u_char zero, uintptr_t hexadecimal, uintptr_t width);


u_char *ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...) 
{
    va_list   args;
    u_char   *p;

    va_start(args, fmt); //使args指向起始的参数
    p = ngx_vslprintf(buf, last, fmt, args);
    va_end(args);        //释放args   
    return p;
}


u_char *ngx_vslprintf(u_char *buf, u_char *last,const char *fmt,va_list args)
{
   
    u_char     zero;


    uintptr_t  width,sign,hex,frac_width,scale,n;  //临时用到的一些变量

    int64_t    i64;   //保存%d对应的可变参
    uint64_t   ui64;  //保存%ud对应的可变参，临时作为%f可变参的整数部分也是可以的 
    u_char     *p;    //保存%s对应的可变参
    double     f;     //保存%f对应的可变参
    uint64_t   frac;  //%f可变参数,根据%.2f等，取得小数部分的2位后的内容；
    

    while (*fmt && buf < last) 
    {
        if (*fmt == '%') 
        {
            //-----------------变量初始化工作开始-----------------
            zero  = (u_char) ((*++fmt == '0') ? '0' : ' ');  //ngx_log_stderr(0, "数字是%010d", 12); 
                                                                
            width = 0;                                       //格式字符% 后边如果是个数字，这个数字最终会弄到width里
            sign  = 1;                                       //显示的是否是有符号数，这里给1，表示是有符号数，除非你 用%u，这个u表示无符号数 
            hex   = 0;                                       //是否以16进制形式显示  0：不是，1：是，并以小写字母显示a-f，2：是，并以大写字母显示A-F
            frac_width = 0;                                  //小数点后位数字，一般需要和%.10f配合使用
            i64 = 0;                                         //一般用%d对应的可变参中的实际数字，会保存在这里
            ui64 = 0;                                        //一般用%ud对应的可变参中的实际数字，会保存在这里    
            
            //-----------------变量初始化工作结束-----------------

            while (*fmt >= '0' && *fmt <= '9')  //如果%后边接的字符是 '0' --'9'之间的内容   ，比如  %16这种；   
            {
                width = width * 10 + (*fmt++ - '0');
            }

            for ( ;; ) 
            {
                switch (*fmt)
                {
                case 'u':     
                    sign = 0; 
                    fmt++;    
                    continue; 

                case 'X':     
                    hex = 2;  
                    sign = 0;
                    fmt++;
                    continue;
                case 'x':      
                    hex = 1;   
                    sign = 0;
                    fmt++;
                    continue;

                case '.':      
                    fmt++;      
                    while(*fmt >= '0' && *fmt <= '9')  //如果是数字，一直循环，这个循环最终就能把诸如%.10f中的10提取出来
                    {
                        frac_width = frac_width * 10 + (*fmt++ - '0'); 
                    } //end while(*fmt >= '0' && *fmt <= '9') 
                    break;

                default:
                    break;                
                } //end switch (*fmt) 
                break;
            } //end for ( ;; )

            switch (*fmt) 
            {
            case '%': //只有%%时才会遇到这个情形，本意是打印一个%
                *buf++ = '%';
                fmt++;
                continue;

            case 'd': //显示整型数据，
                if (sign)  //如果是有符号数
                {
                    i64 = (int64_t) va_arg(args, int); 
                }
                else 
                {
                    ui64 = (uint64_t) va_arg(args, u_int);    
                }
                break; 

            case 's': //一般用于显示字符串
                p = va_arg(args, u_char *); 

                while (*p && buf < last)  
                {
                    *buf++ = *p++;  
                }
                
                fmt++;
                continue; 

            case 'P':  //转换一个pid_t类型
                i64 = (int64_t) va_arg(args, pid_t);
                sign = 1;
                break;

            case 'f': //一般 用于显示double类型数据，如果要显示小数部分，则要形如 %.5f  
                f = va_arg(args, double); 
                if (f < 0)  //负数的处理
                {
                    *buf++ = '-'; 
                    f = -f; 
                }
                //走到这里保证f肯定 >= 0【不为负数】
                ui64 = (int64_t) f; //正整数部分给到ui64里
                frac = 0;

                //如果要求小数点后显示多少位小数
                if (frac_width) //如果是%d.2f，那么frac_width就会是这里的2
                {
                    scale = 1;  //缩放从1开始
                    for (n = frac_width; n; n--) 
                    {
                        scale *= 10; 
                    }
                    
                    frac = (uint64_t) ((f - (double) ui64) * scale + 0.5);  
                                                                            

                    if (frac == scale)  
                                         
                    {
                        ui64++;    //正整数部分进位
                        frac = 0;  //小数部分归0
                    }
                } //end if (frac_width)

                //正整数部分，先显示出来
                buf = ngx_sprintf_num(buf, last, ui64, zero, 0, width); //把一个数字 比如“1234567”弄到buffer中显示

                if (frac_width) //指定了显示多少位小数
                {
                    if (buf < last) 
                    {
                        *buf++ = '.'; //因为指定显示多少位小数，先把小数点增加进来
                    }
                    buf = ngx_sprintf_num(buf, last, frac, '0', 0, frac_width); //frac这里是小数部分，显示出来，不够的，前边填充'0'字符
                }
                fmt++;
                continue;  //重新从while开始执行

            //..................................
            //................其他格式符，逐步完善
            //..................................

            default:
                *buf++ = *fmt++; //往下移动一个字符
                continue; 
            } //end switch (*fmt) 
            
            //统一把显示的数字都保存到 ui64 里去；
            if (sign) //显示的是有符号数
            {
                if (i64 < 0)  //这可能是和%d格式对应的要显示的数字
                {
                    *buf++ = '-'; 
                    ui64 = (uint64_t) -i64; //变成无符号数（正数）
                }
                else //显示正数
                {
                    ui64 = (uint64_t) i64;
                }
            } //end if (sign) 


            buf = ngx_sprintf_num(buf, last, ui64, zero, hex, width); 
            fmt++;
        }
        else  //当成正常字符，源【fmt】拷贝到目标【buf】里
        {
           
            *buf++ = *fmt++;  
        } //end if (*fmt == '%') 
    }  //end while (*fmt && buf < last) 
    
    return buf;
}

static u_char * ngx_sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero, uintptr_t hexadecimal, uintptr_t width)
{
    //temp[21]
    u_char      *p, temp[NGX_INT64_LEN + 1];  
    size_t      len;
    uint32_t    ui32;

    static u_char   hex[] = "0123456789abcdef"; 
    static u_char   HEX[] = "0123456789ABCDEF"; 

    p = temp + NGX_INT64_LEN; 

    if (hexadecimal == 0)  
    {
        if (ui64 <= (uint64_t) NGX_MAX_UINT32_VALUE)   
        {
            ui32 = (uint32_t) ui64; 
            do 
            {
                *--p = (u_char) (ui32 % 10 + '0'); 
            }
            while (ui32 /= 10); //每次缩小10倍等于去掉屁股后边这个数字
        }
        else
        {
            do 
            {
                *--p = (u_char) (ui64 % 10 + '0');
            } while (ui64 /= 10); //每次缩小10倍等于去掉屁股后边这个数字
        }
    }
    else if (hexadecimal == 1)  
    {
        do 
        {            
            *--p = hex[(uint32_t) (ui64 & 0xf)];    
        } while (ui64 >>= 4);   
        
    } 
    else // hexadecimal == 2   
    { 
        do 
        { 
            *--p = HEX[(uint32_t) (ui64 & 0xf)];
        } while (ui64 >>= 4);
    }

    len = (temp + NGX_INT64_LEN) - p;  

    while (len++ < width && buf < last)  
    {
        *buf++ = zero;  
                        
                        
    }
    
    len = (temp + NGX_INT64_LEN) - p; 

    if((buf + len) >= last)   
    {
        len = last - buf; 
    }

    return ngx_cpymem(buf, p, len); 
}

