
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
#include "gsx_c_memory.h"
#include "gsx_c_lockmutex.h"

CSocekt::CSocekt()
{

    m_worker_connections = 1;
    m_ListenPortCount = 1;
    m_RecyConnectionWaitTime = 60;

    m_epollhandle = -1;

    m_iLenPkgHeader = sizeof(COMM_PKG_HEADER);
    m_iLenMsgHeader = sizeof(STRUC_MSG_HEADER);

    m_iSendMsgQueueCount = 0;
    m_totol_recyconnection_n = 0;
    m_cur_size_ = 0;
    m_timer_value_ = 0;
    return;
}

bool CSocekt::Initialize()
{
    ReadConf();
    if (gsx_open_listening_sockets() == false)
        return false;
    return true;
}

bool CSocekt::Initialize_subproc()
{

    if (pthread_mutex_init(&m_sendMessageQueueMutex, NULL) != 0)
    {
        gsx_log_stderr(0, "CSocekt::Initialize_subproc()中pthread_mutex_init(&m_sendMessageQueueMutex)失败.");
        return false;
    }

    if (pthread_mutex_init(&m_connectionMutex, NULL) != 0)
    {
        gsx_log_stderr(0, "CSocekt::Initialize_subproc()中pthread_mutex_init(&m_connectionMutex)失败.");
        return false;
    }

    if (pthread_mutex_init(&m_recyconnqueueMutex, NULL) != 0)
    {
        gsx_log_stderr(0, "CSocekt::Initialize_subproc()中pthread_mutex_init(&m_recyconnqueueMutex)失败.");
        return false;
    }

    if (pthread_mutex_init(&m_timequeueMutex, NULL) != 0)
    {
        gsx_log_stderr(0, "CSocekt::Initialize_subproc()中pthread_mutex_init(&m_timequeueMutex)失败.");
        return false;
    }

    if (sem_init(&m_semEventSendQueue, 0, 0) == -1)
    {
        gsx_log_stderr(0, "CSocekt::Initialize_subproc()中sem_init(&m_semEventSendQueue,0,0)失败.");
        return false;
    }

    int err;
    ThreadItem *pSendQueue;
    m_threadVector.push_back(pSendQueue = new ThreadItem(this));
    err = pthread_create(&pSendQueue->_Handle, NULL, ServerSendQueueThread, pSendQueue);
    if (err != 0)
    {
        gsx_log_stderr(0, "CSocekt::Initialize_subproc()中pthread_create(ServerSendQueueThread)失败.");
        return false;
    }

    ThreadItem *pRecyconn;
    m_threadVector.push_back(pRecyconn = new ThreadItem(this));
    err = pthread_create(&pRecyconn->_Handle, NULL, ServerRecyConnectionThread, pRecyconn);
    if (err != 0)
    {
        gsx_log_stderr(0, "CSocekt::Initialize_subproc()中pthread_create(ServerRecyConnectionThread)失败.");
        return false;
    }

    if (m_ifkickTimeCount == 1)
    {
        ThreadItem *pTimemonitor;
        m_threadVector.push_back(pTimemonitor = new ThreadItem(this));
        err = pthread_create(&pTimemonitor->_Handle, NULL, ServerTimerQueueMonitorThread, pTimemonitor);
        if (err != 0)
        {
            gsx_log_stderr(0, "CSocekt::Initialize_subproc()中pthread_create(ServerTimerQueueMonitorThread)失败.");
            return false;
        }
    }

    return true;
}

CSocekt::~CSocekt()
{

    std::vector<lpgsx_listening_t>::iterator pos;
    for (pos = m_ListenSocketList.begin(); pos != m_ListenSocketList.end(); ++pos)
    {
        delete (*pos);
    }
    m_ListenSocketList.clear();
    return;
}

void CSocekt::Shutdown_subproc()
{

    if (sem_post(&m_semEventSendQueue) == -1)
    {
        gsx_log_stderr(0, "CSocekt::Shutdown_subproc()中sem_post(&m_semEventSendQueue)失败.");
    }

    std::vector<ThreadItem *>::iterator iter;
    for (iter = m_threadVector.begin(); iter != m_threadVector.end(); iter++)
    {
        pthread_join((*iter)->_Handle, NULL);
    }

    for (iter = m_threadVector.begin(); iter != m_threadVector.end(); iter++)
    {
        if (*iter)
            delete *iter;
    }
    m_threadVector.clear();

    clearMsgSendQueue();
    clearconnection();
    clearAllFromTimerQueue();

    pthread_mutex_destroy(&m_connectionMutex);
    pthread_mutex_destroy(&m_sendMessageQueueMutex);
    pthread_mutex_destroy(&m_recyconnqueueMutex);
    pthread_mutex_destroy(&m_timequeueMutex);
    sem_destroy(&m_semEventSendQueue);
}

void CSocekt::clearMsgSendQueue()
{
    char *sTmpMempoint;
    CMemory *p_memory = CMemory::GetInstance();

    while (!m_MsgSendQueue.empty())
    {
        sTmpMempoint = m_MsgSendQueue.front();
        m_MsgSendQueue.pop_front();
        p_memory->FreeMemory(sTmpMempoint);
    }
}

void CSocekt::ReadConf()
{
    CConfig *p_config = CConfig::GetInstance();
    m_worker_connections = p_config->GetIntDefault("worker_connections", m_worker_connections);
    m_ListenPortCount = p_config->GetIntDefault("ListenPortCount", m_ListenPortCount);
    m_RecyConnectionWaitTime = p_config->GetIntDefault("Sock_RecyConnectionWaitTime", m_RecyConnectionWaitTime);

    m_ifkickTimeCount = p_config->GetIntDefault("Sock_WaitTimeEnable", 0);
    m_iWaitTime = p_config->GetIntDefault("Sock_MaxWaitTime", m_iWaitTime);
    m_iWaitTime = (m_iWaitTime > 5) ? m_iWaitTime : 5;

    return;
}

bool CSocekt::gsx_open_listening_sockets()
{
    int isock;
    struct sockaddr_in serv_addr;
    int iport;
    char strinfo[100];

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    CConfig *p_config = CConfig::GetInstance();
    for (int i = 0; i < m_ListenPortCount; i++)
    {

        isock = socket(AF_INET, SOCK_STREAM, 0);
        if (isock == -1)
        {
            gsx_log_stderr(errno, "CSocekt::Initialize()中socket()失败,i=%d.", i);

            return false;
        }

        int reuseaddr = 1;
        if (setsockopt(isock, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuseaddr, sizeof(reuseaddr)) == -1)
        {
            gsx_log_stderr(errno, "CSocekt::Initialize()中setsockopt(SO_REUSEADDR)失败,i=%d.", i);
            close(isock);
            return false;
        }

        if (setnonblocking(isock) == false)
        {
            gsx_log_stderr(errno, "CSocekt::Initialize()中setnonblocking()失败,i=%d.", i);
            close(isock);
            return false;
        }

        strinfo[0] = 0;
        sprintf(strinfo, "ListenPort%d", i);
        iport = p_config->GetIntDefault(strinfo, 10000);
        serv_addr.sin_port = htons((in_port_t)iport);

        if (bind(isock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        {
            gsx_log_stderr(errno, "CSocekt::Initialize()中bind()失败,i=%d.", i);
            close(isock);
            return false;
        }

        if (listen(isock, GSX_LISTEN_BACKLOG) == -1)
        {
            gsx_log_stderr(errno, "CSocekt::Initialize()中listen()失败,i=%d.", i);
            close(isock);
            return false;
        }

        lpgsx_listening_t p_listensocketitem = new gsx_listening_t;
        memset(p_listensocketitem, 0, sizeof(gsx_listening_t));
        p_listensocketitem->port = iport;
        p_listensocketitem->fd = isock;
        gsx_log_error_core(GSX_LOG_INFO, 0, "监听%d端口成功!", iport);
        m_ListenSocketList.push_back(p_listensocketitem);
    }
    if (m_ListenSocketList.size() <= 0)
        return false;
    return true;
}

bool CSocekt::setnonblocking(int sockfd)
{
    int nb = 1;
    if (ioctl(sockfd, FIONBIO, &nb) == -1)
    {
        return false;
    }
    return true;

    /* 
    
    
    int opts = fcntl(sockfd, F_GETFL);  
    if(opts < 0) 
    {
        gsx_log_stderr(errno,"CSocekt::setnonblocking()中fcntl(F_GETFL)失败.");
        return false;
    }
    opts |= O_NONBLOCK; 
    if(fcntl(sockfd, F_SETFL, opts) < 0) 
    {
        gsx_log_stderr(errno,"CSocekt::setnonblocking()中fcntl(F_SETFL)失败.");
        return false;
    }
    return true;
    */
}

void CSocekt::gsx_close_listening_sockets()
{
    for (int i = 0; i < m_ListenPortCount; i++)
    {

        close(m_ListenSocketList[i]->fd);
        gsx_log_error_core(GSX_LOG_INFO, 0, "关闭监听端口%d!", m_ListenSocketList[i]->port);
    }
    return;
}

void CSocekt::msgSend(char *psendbuf)
{
    CLock lock(&m_sendMessageQueueMutex);
    m_MsgSendQueue.push_back(psendbuf);
    ++m_iSendMsgQueueCount;

    if (sem_post(&m_semEventSendQueue) == -1)
    {
        gsx_log_stderr(0, "CSocekt::msgSend()中sem_post(&m_semEventSendQueue)失败.");
    }
    return;
}

void CSocekt::zdClosesocketProc(lpgsx_connection_t p_Conn)
{
    if (m_ifkickTimeCount == 1)
    {
        DeleteFromTimerQueue(p_Conn);
    }
    if (p_Conn->fd != -1)
    {
        close(p_Conn->fd);
        p_Conn->fd = -1;
    }

    if (p_Conn->iThrowsendCount > 0)
        --p_Conn->iThrowsendCount;

    inRecyConnectQueue(p_Conn);
    return;
}

int CSocekt::gsx_epoll_init()
{

    m_epollhandle = epoll_create(m_worker_connections);
    if (m_epollhandle == -1)
    {
        gsx_log_stderr(errno, "CSocekt::gsx_epoll_init()中epoll_create()失败.");
        exit(2);
    }

    initconnection();
    /*
    m_connection_n = m_worker_connections;      
    
    m_pconnections = new gsx_connection_t[m_connection_n]; 
   

    int i = m_connection_n;                
    lpgsx_connection_t next  = NULL;
    lpgsx_connection_t c = m_pconnections; 
    do 
    {
        i--;   

        
        c[i].data = next;         
        c[i].fd = -1;             
        c[i].instance = 1;        
        c[i].iCurrsequence = 0;   
        

        next = &c[i]; 
    } while (i); 
    m_pfree_connections = next;            
    m_free_connection_n = m_connection_n;  
    */

    std::vector<lpgsx_listening_t>::iterator pos;
    for (pos = m_ListenSocketList.begin(); pos != m_ListenSocketList.end(); ++pos)
    {
        lpgsx_connection_t p_Conn = gsx_get_connection((*pos)->fd);
        if (p_Conn == NULL)
        {

            gsx_log_stderr(errno, "CSocekt::gsx_epoll_init()中gsx_get_connection()失败.");
            exit(2);
        }
        p_Conn->listening = (*pos);
        (*pos)->connection = p_Conn;

        p_Conn->rhandler = &CSocekt::gsx_event_accept;

        /*if(gsx_epoll_add_event((*pos)->fd,       
                                1,0,             
                                0,               
                                EPOLL_CTL_ADD,   
                                p_Conn           
                                ) == -1) 
                                */
        if (gsx_epoll_oper_event(
                (*pos)->fd,
                EPOLL_CTL_ADD,
                EPOLLIN | EPOLLRDHUP,
                0,
                p_Conn) == -1)
        {
            exit(2);
        }
    }
    return 1;
}

/*








int CSocekt::gsx_epoll_add_event(int fd,
                                int readevent,int writeevent,
                                uint32_t otherflag, 
                                uint32_t eventtype, 
                                lpgsx_connection_t pConn
                                )
{
    struct epoll_event ev;
    
    memset(&ev, 0, sizeof(ev));

    if(readevent==1)
    {
        
        ev.events = EPOLLIN|EPOLLRDHUP; 
                                          

        
                    



        
        
        
        
        
                
        
        
        
                
    }
    if(writeevent==1)    
    {
        
        ev.events = EPOLLOUT;
        
        ev.events |= (EPOLLIN|EPOLLRDHUP); 
    }
    
    if(otherflag != 0)
    {
        ev.events |= otherflag; 
    }

    
    
    
                                                             

    if(eventtype == EPOLL_CTL_ADD)
    {        
        
        ev.data.ptr = (void *)pConn;
    }
    
    if(epoll_ctl(m_epollhandle,eventtype,fd,&ev) == -1)
    {
        gsx_log_stderr(errno,"CSocekt::gsx_epoll_add_event()中epoll_ctl(%d,%d,%d,%ud,%ud)失败.",fd,readevent,writeevent,otherflag,eventtype);
        
        return -1;
    }
    return 1;
}
*/

int CSocekt::gsx_epoll_oper_event(
    int fd,
    uint32_t eventtype,
    uint32_t flag,
    int bcaction,
    lpgsx_connection_t pConn)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));

    if (eventtype == EPOLL_CTL_ADD)
    {

        ev.events = flag;
        pConn->events = flag;
    }
    else if (eventtype == EPOLL_CTL_MOD)
    {

        ev.events = pConn->events;
        if (bcaction == 0)
        {

            ev.events |= flag;
        }
        else if (bcaction == 1)
        {

            ev.events &= ~flag;
        }
        else
        {

            ev.events = flag;
        }
        pConn->events = ev.events;
    }
    else
    {

        return 1;
    }

    ev.data.ptr = (void *)pConn;

    if (epoll_ctl(m_epollhandle, eventtype, fd, &ev) == -1)
    {
        gsx_log_stderr(errno, "CSocekt::gsx_epoll_oper_event()中epoll_ctl(%d,%ud,%ud,%d)失败.", fd, eventtype, flag, bcaction);
        return -1;
    }
    return 1;
}

int CSocekt::gsx_epoll_process_events(int timer)
{

    int events = epoll_wait(m_epollhandle, m_events, GSX_MAX_EVENTS, timer);

    if (events == -1)
    {

        if (errno == EINTR)
        {

            gsx_log_error_core(GSX_LOG_INFO, errno, "CSocekt::gsx_epoll_process_events()中epoll_wait()失败!");
            return 1;
        }
        else
        {

            gsx_log_error_core(GSX_LOG_ALERT, errno, "CSocekt::gsx_epoll_process_events()中epoll_wait()失败!");
            return 0;
        }
    }

    if (events == 0)
    {
        if (timer != -1)
        {

            return 1;
        }

        gsx_log_error_core(GSX_LOG_ALERT, 0, "CSocekt::gsx_epoll_process_events()中epoll_wait()没超时却没返回任何事件!");
        return 0;
    }

    lpgsx_connection_t p_Conn;

    uint32_t revents;
    for (int i = 0; i < events; ++i)
    {
        p_Conn = (lpgsx_connection_t)(m_events[i].data.ptr);

        /*
        instance = (uintptr_t) c & 1;                             
                                                                  
        p_Conn = (lpgsx_connection_t) ((uintptr_t)p_Conn & (uintptr_t) ~1); 

        
        
        if(c->fd == -1)  
                           
        {                 
            
            
            

            
            gsx_log_error_core(GSX_LOG_DEBUG,0,"CSocekt::gsx_epoll_process_events()中遇到了fd=-1的过期事件:%p.",c); 
            continue; 
        }

        
        if(c->instance != instance)
        {
            
            
            
            
            

            
            
            
            
                

            
            
                  

            gsx_log_error_core(GSX_LOG_DEBUG,0,"CSocekt::gsx_epoll_process_events()中遇到了instance值改变的过期事件:%p.",c); 
            continue; 
        }
        
        */

        revents = m_events[i].events;

        /*
        if(revents & (EPOLLERR|EPOLLHUP)) 
        {
            
            
            
            revents |= EPOLLIN|EPOLLOUT;   
                                           
        } */

        if (revents & EPOLLIN)
        {

            (this->*(p_Conn->rhandler))(p_Conn);
        }

        if (revents & EPOLLOUT)
        {

            if (revents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {

                --p_Conn->iThrowsendCount;
            }
            else
            {
                (this->*(p_Conn->whandler))(p_Conn);
            }
        }
    }
    return 1;
}

void *CSocekt::ServerSendQueueThread(void *threadData)
{
    ThreadItem *pThread = static_cast<ThreadItem *>(threadData);
    CSocekt *pSocketObj = pThread->_pThis;
    int err;
    std::list<char *>::iterator pos, pos2, posend;

    char *pMsgBuf;
    LPSTRUC_MSG_HEADER pMsgHeader;
    LPCOMM_PKG_HEADER pPkgHeader;
    lpgsx_connection_t p_Conn;
    unsigned short itmp;
    ssize_t sendsize;

    CMemory *p_memory = CMemory::GetInstance();

    while (g_stopEvent == 0)
    {

        if (sem_wait(&pSocketObj->m_semEventSendQueue) == -1)
        {

            if (errno != EINTR)
                gsx_log_stderr(errno, "CSocekt::ServerSendQueueThread()中sem_wait(&pSocketObj->m_semEventSendQueue)失败.");
        }

        if (g_stopEvent != 0)
            break;

        if (pSocketObj->m_iSendMsgQueueCount > 0)
        {
            err = pthread_mutex_lock(&pSocketObj->m_sendMessageQueueMutex);
            if (err != 0)
                gsx_log_stderr(err, "CSocekt::ServerSendQueueThread()中pthread_mutex_lock()失败，返回的错误码为%d!", err);

            pos = pSocketObj->m_MsgSendQueue.begin();
            posend = pSocketObj->m_MsgSendQueue.end();

            while (pos != posend)
            {
                pMsgBuf = (*pos);
                pMsgHeader = (LPSTRUC_MSG_HEADER)pMsgBuf;
                pPkgHeader = (LPCOMM_PKG_HEADER)(pMsgBuf + pSocketObj->m_iLenMsgHeader);
                p_Conn = pMsgHeader->pConn;

                if (p_Conn->iCurrsequence != pMsgHeader->iCurrsequence)
                {

                    pos2 = pos;
                    pos++;
                    pSocketObj->m_MsgSendQueue.erase(pos2);
                    --pSocketObj->m_iSendMsgQueueCount;
                    p_memory->FreeMemory(pMsgBuf);
                    continue;
                }

                if (p_Conn->iThrowsendCount > 0)
                {

                    pos++;
                    continue;
                }

                p_Conn->psendMemPointer = pMsgBuf;
                pos2 = pos;
                pos++;
                pSocketObj->m_MsgSendQueue.erase(pos2);
                --pSocketObj->m_iSendMsgQueueCount;
                p_Conn->psendbuf = (char *)pPkgHeader;
                itmp = ntohs(pPkgHeader->pkgLen);
                p_Conn->isendlen = itmp;

                sendsize = pSocketObj->sendproc(p_Conn, p_Conn->psendbuf, p_Conn->isendlen);
                if (sendsize > 0)
                {
                    if (sendsize == p_Conn->isendlen)
                    {

                        p_memory->FreeMemory(p_Conn->psendMemPointer);
                        p_Conn->psendMemPointer = NULL;
                        p_Conn->iThrowsendCount = 0;
                    }
                    else
                    {

                        p_Conn->psendbuf = p_Conn->psendbuf + sendsize;
                        p_Conn->isendlen = p_Conn->isendlen - sendsize;

                        ++p_Conn->iThrowsendCount;

                        if (pSocketObj->gsx_epoll_oper_event(
                                p_Conn->fd,
                                EPOLL_CTL_MOD,
                                EPOLLOUT,
                                0,
                                p_Conn) == -1)
                        {

                            gsx_log_stderr(errno, "CSocekt::ServerSendQueueThread()gsx_epoll_oper_event()失败.");
                        }
                    }
                    continue;
                }

                else if (sendsize == 0)
                {

                    p_memory->FreeMemory(p_Conn->psendMemPointer);
                    p_Conn->psendMemPointer = NULL;
                    p_Conn->iThrowsendCount = 0;
                    continue;
                }

                else if (sendsize == -1)
                {

                    ++p_Conn->iThrowsendCount;

                    if (pSocketObj->gsx_epoll_oper_event(
                            p_Conn->fd,
                            EPOLL_CTL_MOD,
                            EPOLLOUT,
                            0,
                            p_Conn) == -1)
                    {

                        gsx_log_stderr(errno, "CSocekt::ServerSendQueueThread()中gsx_epoll_add_event()_2失败.");
                    }
                    continue;
                }

                else
                {

                    p_memory->FreeMemory(p_Conn->psendMemPointer);
                    p_Conn->psendMemPointer = NULL;
                    p_Conn->iThrowsendCount = 0;
                    continue;
                }
            }

            err = pthread_mutex_unlock(&pSocketObj->m_sendMessageQueueMutex);
            if (err != 0)
                gsx_log_stderr(err, "CSocekt::ServerSendQueueThread()pthread_mutex_unlock()失败，返回的错误码为%d!", err);
        }
    }

    return (void *)0;
}
