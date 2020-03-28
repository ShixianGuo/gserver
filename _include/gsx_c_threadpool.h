
#ifndef __GSX_THREADPOOL_H__
#define __GSX_THREADPOOL_H__

#include <vector>
#include <pthread.h>
#include <atomic>
class CThreadPool
{
public:
    CThreadPool();

    ~CThreadPool();

public:
    bool Create(int threadNum);
    void StopAll();
    void inMsgRecvQueueAndSignal(char *buf);
    void Call();

private:
    static void *ThreadFunc(void *threadData);
    void clearMsgRecvQueue();

private:
    struct ThreadItem
    {
        pthread_t _Handle;
        CThreadPool *_pThis;
        bool ifrunning;
        ThreadItem(CThreadPool *pthis) : _pThis(pthis), ifrunning(false) {}
        ~ThreadItem() {}
    };

private:
    static pthread_mutex_t m_pthreadMutex;
    static pthread_cond_t m_pthreadCond;
    static bool m_shutdown;
    int m_iThreadNum;
    std::atomic<int> m_iRunningThreadNum;
    time_t m_iLastEmgTime;
    std::vector<ThreadItem *> m_threadVector;
    std::list<char *> m_MsgRecvQueue;
    int m_iRecvMsgQueueCount;
};

#endif
