
#ifndef __GSX_SOCKET_H__
#define __GSX_SOCKET_H__

#include <vector>       
#include <list>         
#include <sys/epoll.h>  
#include <sys/socket.h>
#include <pthread.h>    
#include <semaphore.h>  
#include <atomic>       
#include <map>
#include "gsx_comm.h"

#define GSX_LISTEN_BACKLOG 511 
#define GSX_MAX_EVENTS 512

typedef struct gsx_listening_s gsx_listening_t, *lpgsx_listening_t;
typedef struct gsx_connection_s gsx_connection_t, *lpgsx_connection_t;

typedef class CSocekt CSocekt;
typedef void (CSocekt::*gsx_event_handler_pt)(lpgsx_connection_t c);

struct gsx_listening_s
{
	int port;
	int fd;
	lpgsx_connection_t connection;
};

struct gsx_connection_s
{
	gsx_connection_s();
	virtual ~gsx_connection_s();
	void GetOneToUse();
	void PutOneToFree();

	int fd;
	lpgsx_listening_t listening;
	uint64_t iCurrsequence;
	struct sockaddr s_sockaddr;

	gsx_event_handler_pt rhandler;
	gsx_event_handler_pt whandler;

	uint32_t events;
	unsigned char curStat;
	char dataHeadInfo[_DATA_BUFSIZE_];

	char *precvbuf;
	unsigned int irecvlen;
	char *precvMemPointer;

	pthread_mutex_t logicPorcMutex;
	std::atomic<int> iThrowsendCount;

	char *psendMemPointer;
	char *psendbuf;

	unsigned int isendlen;
	time_t inRecyTime;
	time_t lastPingTime;
	
	lpgsx_connection_t next;
};

typedef struct _STRUC_MSG_HEADER
{
	lpgsx_connection_t pConn;
	uint64_t iCurrsequence;
} STRUC_MSG_HEADER, *LPSTRUC_MSG_HEADER;

class CSocekt
{
public:
	CSocekt();
	virtual ~CSocekt();
	virtual bool Initialize();
	virtual bool Initialize_subproc();
	virtual void Shutdown_subproc();

public:
	virtual void threadRecvProcFunc(char *pMsgBuf);
	virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t cur_time);

public:
	int gsx_epoll_init();
	int gsx_epoll_process_events(int timer);
	int gsx_epoll_oper_event(int fd, uint32_t eventtype, uint32_t flag, int bcaction, lpgsx_connection_t pConn);

protected:
	void msgSend(char *psendbuf);
	void zdClosesocketProc(lpgsx_connection_t p_Conn);

private:
	void ReadConf();

	//端口处理相关
	bool gsx_open_listening_sockets();
	void gsx_close_listening_sockets();
	bool setnonblocking(int sockfd);

	//连接池 或 连接 相关
	lpgsx_connection_t gsx_get_connection(int isock);
	void gsx_free_connection(lpgsx_connection_t pConn);

    
	void gsx_event_accept(lpgsx_connection_t oldc);
	void gsx_read_request_handler(lpgsx_connection_t pConn);
	void gsx_write_request_handler(lpgsx_connection_t pConn);
	void gsx_close_connection(lpgsx_connection_t pConn);

	ssize_t recvproc(lpgsx_connection_t pConn, char *buff, ssize_t buflen);
	void gsx_wait_request_handler_proc_p1(lpgsx_connection_t pConn);
	void gsx_wait_request_handler_proc_plast(lpgsx_connection_t pConn);
	void clearMsgSendQueue();
	ssize_t sendproc(lpgsx_connection_t c, char *buff, ssize_t size);


	size_t gsx_sock_ntop(struct sockaddr *sa, int port, u_char *text, size_t len);
	void initconnection();
	void clearconnection();




	void inRecyConnectQueue(lpgsx_connection_t pConn);
	void AddToTimerQueue(lpgsx_connection_t pConn);
	time_t GetEarliestTime();
	LPSTRUC_MSG_HEADER RemoveFirstTimer();
	LPSTRUC_MSG_HEADER GetOverTimeTimer(time_t cur_time);
	void DeleteFromTimerQueue(lpgsx_connection_t pConn);
	void clearAllFromTimerQueue();

	static void *ServerSendQueueThread(void *threadData);
	static void *ServerRecyConnectionThread(void *threadData);
	static void *ServerTimerQueueMonitorThread(void *threadData);

protected:
	size_t m_iLenPkgHeader;
	size_t m_iLenMsgHeader;
	int m_iWaitTime;

private:
	struct ThreadItem
	{
		pthread_t _Handle;
		CSocekt *_pThis;
		bool ifrunning;
		ThreadItem(CSocekt *pthis) : _pThis(pthis), ifrunning(false) {}
		~ThreadItem() {}
	};

	int m_worker_connections;
	int m_ListenPortCount;
	int m_epollhandle;
	std::list<lpgsx_connection_t> m_connectionList;
	std::list<lpgsx_connection_t> m_freeconnectionList;
	std::atomic<int> m_total_connection_n;
	std::atomic<int> m_free_connection_n;
	pthread_mutex_t m_connectionMutex;
	pthread_mutex_t m_recyconnqueueMutex;
	std::list<lpgsx_connection_t> m_recyconnectionList;
	std::atomic<int> m_totol_recyconnection_n;
	int m_RecyConnectionWaitTime;

	std::vector<lpgsx_listening_t> m_ListenSocketList;
	struct epoll_event m_events[GSX_MAX_EVENTS];
	std::list<char *> m_MsgSendQueue;
	std::atomic<int> m_iSendMsgQueueCount;
	std::vector<ThreadItem *> m_threadVector;
	pthread_mutex_t m_sendMessageQueueMutex;
	sem_t m_semEventSendQueue;
	int m_ifkickTimeCount;
	pthread_mutex_t m_timequeueMutex;
	std::multimap<time_t, LPSTRUC_MSG_HEADER> m_timerQueuemap;
	size_t m_cur_size_;
	time_t m_timer_value_;
};

#endif
