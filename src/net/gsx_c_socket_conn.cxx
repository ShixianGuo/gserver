

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

gsx_connection_s::gsx_connection_s()
{
    iCurrsequence = 0;
    pthread_mutex_init(&logicPorcMutex, NULL);
}
gsx_connection_s::~gsx_connection_s()
{
    pthread_mutex_destroy(&logicPorcMutex);
}

void gsx_connection_s::GetOneToUse()
{
    ++iCurrsequence;

    fd = -1;
    curStat = _PKG_HD_INIT;
    precvbuf = dataHeadInfo;
    irecvlen = sizeof(COMM_PKG_HEADER);

    precvMemPointer = NULL;
    iThrowsendCount = 0;
    psendMemPointer = NULL;
    events = 0;
    lastPingTime = time(NULL);
}

void gsx_connection_s::PutOneToFree()
{
    ++iCurrsequence;
    if (precvMemPointer != NULL)
    {
        CMemory::GetInstance()->FreeMemory(precvMemPointer);
        precvMemPointer = NULL;
    }
    if (psendMemPointer != NULL)
    {
        CMemory::GetInstance()->FreeMemory(psendMemPointer);
        psendMemPointer = NULL;
    }

    iThrowsendCount = 0;
}

void CSocekt::initconnection()
{
    lpgsx_connection_t p_Conn;
    CMemory *p_memory = CMemory::GetInstance();

    int ilenconnpool = sizeof(gsx_connection_t);
    for (int i = 0; i < m_worker_connections; ++i)
    {
        p_Conn = (lpgsx_connection_t)p_memory->AllocMemory(ilenconnpool, true);

        p_Conn = new (p_Conn) gsx_connection_t();
        p_Conn->GetOneToUse();
        m_connectionList.push_back(p_Conn);
        m_freeconnectionList.push_back(p_Conn);
    }
    m_free_connection_n = m_total_connection_n = m_connectionList.size();
    return;
}

void CSocekt::clearconnection()
{
    lpgsx_connection_t p_Conn;
    CMemory *p_memory = CMemory::GetInstance();

    while (!m_connectionList.empty())
    {
        p_Conn = m_connectionList.front();
        m_connectionList.pop_front();
        p_Conn->~gsx_connection_t();
        p_memory->FreeMemory(p_Conn);
    }
}

lpgsx_connection_t CSocekt::gsx_get_connection(int isock)
{

    CLock lock(&m_connectionMutex);

    if (!m_freeconnectionList.empty())
    {

        lpgsx_connection_t p_Conn = m_freeconnectionList.front();
        m_freeconnectionList.pop_front();
        p_Conn->GetOneToUse();
        --m_free_connection_n;
        p_Conn->fd = isock;
        return p_Conn;
    }

    CMemory *p_memory = CMemory::GetInstance();
    lpgsx_connection_t p_Conn = (lpgsx_connection_t)p_memory->AllocMemory(sizeof(gsx_connection_t), true);
    p_Conn = new (p_Conn) gsx_connection_t();
    p_Conn->GetOneToUse();
    m_connectionList.push_back(p_Conn);
    ++m_total_connection_n;
    p_Conn->fd = isock;
    return p_Conn;

    /*
    lpgsx_connection_t  c = m_pfree_connections; 

    if(c == NULL)
    {
        
        gsx_log_stderr(0,"CSocekt::gsx_get_connection()中空闲链表为空,这不应该!");
        return NULL;
    }

    m_pfree_connections = c->next;                       
    m_free_connection_n--;                               
    
    
    uintptr_t  instance = c->instance;            
    uint64_t   iCurrsequence = c->iCurrsequence;  
    


    
    memset(c,0,sizeof(gsx_connection_t));                
    c->fd      = isock;                                  


    c->curStat = _PKG_HD_INIT;                           

    c->precvbuf = c->dataHeadInfo;                       
    c->irecvlen = sizeof(COMM_PKG_HEADER);               

    
    c->precvMemPointer = NULL;                            
    c->iThrowsendCount = 0;                              
    c->psendMemPointer = NULL;                          
    


    
    c->instance = !instance;                            
    c->iCurrsequence=iCurrsequence;++c->iCurrsequence;  

    
    return c;    
    */
}

void CSocekt::gsx_free_connection(lpgsx_connection_t pConn)
{

    CLock lock(&m_connectionMutex);

    pConn->PutOneToFree();

    m_freeconnectionList.push_back(pConn);

    ++m_free_connection_n;

    /*
    if(c->precvMemPointer != NULL)
    {
        
        CMemory::GetInstance()->FreeMemory(c->precvMemPointer);
        c->precvMemPointer = NULL;
        
    }
    if(c->psendMemPointer != NULL) 
    {
        CMemory::GetInstance()->FreeMemory(c->psendMemPointer);
        c->psendMemPointer = NULL;
    }

    c->next = m_pfree_connections;                       

    
    ++c->iCurrsequence;                                  
    c->iThrowsendCount = 0;                              

    m_pfree_connections = c;                             
    ++m_free_connection_n;                               
    */
    return;
}

void CSocekt::inRecyConnectQueue(lpgsx_connection_t pConn)
{

    std::list<lpgsx_connection_t>::iterator pos;
    bool iffind = false;

    CLock lock(&m_recyconnqueueMutex);

    for (pos = m_recyconnectionList.begin(); pos != m_recyconnectionList.end(); ++pos)
    {
        if ((*pos) == pConn)
        {
            iffind = true;
            break;
        }
    }
    if (iffind == true)
    {

        return;
    }

    pConn->inRecyTime = time(NULL);
    ++pConn->iCurrsequence;
    m_recyconnectionList.push_back(pConn);
    ++m_totol_recyconnection_n;
    return;
}

void *CSocekt::ServerRecyConnectionThread(void *threadData)
{
    ThreadItem *pThread = static_cast<ThreadItem *>(threadData);
    CSocekt *pSocketObj = pThread->_pThis;

    time_t currtime;
    int err;
    std::list<lpgsx_connection_t>::iterator pos, posend;
    lpgsx_connection_t p_Conn;

    while (1)
    {

        usleep(200 * 1000);

        if (pSocketObj->m_totol_recyconnection_n > 0)
        {
            currtime = time(NULL);
            err = pthread_mutex_lock(&pSocketObj->m_recyconnqueueMutex);
            if (err != 0)
                gsx_log_stderr(err, "CSocekt::ServerRecyConnectionThread()中pthread_mutex_lock()失败，返回的错误码为%d!", err);

        lblRRTD:
            pos = pSocketObj->m_recyconnectionList.begin();
            posend = pSocketObj->m_recyconnectionList.end();
            for (; pos != posend; ++pos)
            {
                p_Conn = (*pos);
                if (
                    ((p_Conn->inRecyTime + pSocketObj->m_RecyConnectionWaitTime) > currtime) && (g_stopEvent == 0))
                {
                    continue;
                }

                if (p_Conn->iThrowsendCount > 0)
                {

                    gsx_log_stderr(0, "CSocekt::ServerRecyConnectionThread()中到释放时间却发现p_Conn.iThrowsendCount!=0，这个不该发生");
                }

                --pSocketObj->m_totol_recyconnection_n;
                pSocketObj->m_recyconnectionList.erase(pos);

                pSocketObj->gsx_free_connection(p_Conn);
                goto lblRRTD;
            }
            err = pthread_mutex_unlock(&pSocketObj->m_recyconnqueueMutex);
            if (err != 0)
                gsx_log_stderr(err, "CSocekt::ServerRecyConnectionThread()pthread_mutex_unlock()失败，返回的错误码为%d!", err);
        }

        if (g_stopEvent == 1)
        {
            if (pSocketObj->m_totol_recyconnection_n > 0)
            {

                err = pthread_mutex_lock(&pSocketObj->m_recyconnqueueMutex);
                if (err != 0)
                    gsx_log_stderr(err, "CSocekt::ServerRecyConnectionThread()中pthread_mutex_lock2()失败，返回的错误码为%d!", err);

            lblRRTD2:
                pos = pSocketObj->m_recyconnectionList.begin();
                posend = pSocketObj->m_recyconnectionList.end();
                for (; pos != posend; ++pos)
                {
                    p_Conn = (*pos);
                    --pSocketObj->m_totol_recyconnection_n;
                    pSocketObj->m_recyconnectionList.erase(pos);
                    pSocketObj->gsx_free_connection(p_Conn);
                    goto lblRRTD2;
                }
                err = pthread_mutex_unlock(&pSocketObj->m_recyconnqueueMutex);
                if (err != 0)
                    gsx_log_stderr(err, "CSocekt::ServerRecyConnectionThread()pthread_mutex_unlock2()失败，返回的错误码为%d!", err);
            }
            break;
        }
    }

    return (void *)0;
}

void CSocekt::gsx_close_connection(lpgsx_connection_t pConn)
{

    gsx_free_connection(pConn);
    if (pConn->fd != -1)
    {
        close(pConn->fd);
        pConn->fd = -1;
    }
    return;
}
