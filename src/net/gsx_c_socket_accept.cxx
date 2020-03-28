
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

#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "gsx_c_conf.h"
#include "gsx_macro.h"
#include "gsx_global.h"
#include "gsx_func.h"
#include "gsx_c_socket.h"

void CSocekt::gsx_event_accept(lpgsx_connection_t oldc)
{

    struct sockaddr mysockaddr;
    socklen_t socklen;
    int err;
    int level;
    int s;
    static int use_accept4 = 1;
    lpgsx_connection_t newc;

    socklen = sizeof(mysockaddr);
    do
    {
        if (use_accept4)
        {

            s = accept4(oldc->fd, &mysockaddr, &socklen, SOCK_NONBLOCK);
        }
        else
        {

            s = accept(oldc->fd, &mysockaddr, &socklen);
        }

        if (s == -1)
        {
            err = errno;

            if (err == EAGAIN)
            {

                return;
            }
            level = GSX_LOG_ALERT;
            if (err == ECONNABORTED)
            {

                level = GSX_LOG_ERR;
            }
            else if (err == EMFILE || err == ENFILE)

            {
                level = GSX_LOG_CRIT;
            }
            gsx_log_error_core(level, errno, "CSocekt::gsx_event_accept()中accept4()失败!");

            if (use_accept4 && err == ENOSYS)
            {
                use_accept4 = 0;
                continue;
            }

            if (err == ECONNABORTED)
            {
            }

            if (err == EMFILE || err == ENFILE)
            {
            }
            return;
        }

        newc = gsx_get_connection(s);
        if (newc == NULL)
        {

            if (close(s) == -1)
            {
                gsx_log_error_core(GSX_LOG_ALERT, errno, "CSocekt::gsx_event_accept()中close(%d)失败!", s);
            }
            return;
        }

        memcpy(&newc->s_sockaddr, &mysockaddr, socklen);

        if (!use_accept4)
        {

            if (setnonblocking(s) == false)
            {

                gsx_close_connection(newc);
                return;
            }
        }

        newc->listening = oldc->listening;

        newc->rhandler = &CSocekt::gsx_read_request_handler;
        newc->whandler = &CSocekt::gsx_write_request_handler;

        if (gsx_epoll_oper_event(
                s,
                EPOLL_CTL_ADD,
                EPOLLIN | EPOLLRDHUP,
                0,
                newc) == -1)
        {

            gsx_close_connection(newc);
            return;
        }
        /*
        else
        {
            
            int           n;
            socklen_t     len;
            len = sizeof(int);
            getsockopt(s,SOL_SOCKET,SO_SNDBUF, &n, &len);
            gsx_log_stderr(0,"发送缓冲区的大小为%d!",n); 

            n = 0;
            getsockopt(s,SOL_SOCKET,SO_RCVBUF, &n, &len);
            gsx_log_stderr(0,"接收缓冲区的大小为%d!",n); 

            int sendbuf = 2048;
            if (setsockopt(s, SOL_SOCKET, SO_SNDBUF,(const void *) &sendbuf,n) == 0)
            {
                gsx_log_stderr(0,"发送缓冲区大小成功设置为%d!",sendbuf); 
            }

             getsockopt(s,SOL_SOCKET,SO_SNDBUF, &n, &len);
            gsx_log_stderr(0,"发送缓冲区的大小为%d!",n); 
        }
        */

        if (m_ifkickTimeCount == 1)
        {
            AddToTimerQueue(newc);
        }

        break;
    } while (1);

    return;
}
