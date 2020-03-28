
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

void CSocekt::AddToTimerQueue(lpgsx_connection_t pConn)
{
	CMemory *p_memory = CMemory::GetInstance();

	time_t futtime = time(NULL);
	futtime += m_iWaitTime;

	CLock lock(&m_timequeueMutex);
	LPSTRUC_MSG_HEADER tmpMsgHeader = (LPSTRUC_MSG_HEADER)p_memory->AllocMemory(m_iLenMsgHeader, false);
	tmpMsgHeader->pConn = pConn;
	tmpMsgHeader->iCurrsequence = pConn->iCurrsequence;
	m_timerQueuemap.insert(std::make_pair(futtime, tmpMsgHeader));
	m_cur_size_++;
	m_timer_value_ = GetEarliestTime();
	return;
}

time_t CSocekt::GetEarliestTime()
{
	std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos;
	pos = m_timerQueuemap.begin();
	return pos->first;
}

LPSTRUC_MSG_HEADER CSocekt::RemoveFirstTimer()
{
	std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos;
	LPSTRUC_MSG_HEADER p_tmp;
	if (m_cur_size_ <= 0)
	{
		return NULL;
	}
	pos = m_timerQueuemap.begin();
	p_tmp = pos->second;
	m_timerQueuemap.erase(pos);
	--m_cur_size_;
	return p_tmp;
}

LPSTRUC_MSG_HEADER CSocekt::GetOverTimeTimer(time_t cur_time)
{
	CMemory *p_memory = CMemory::GetInstance();
	LPSTRUC_MSG_HEADER ptmp;

	if (m_cur_size_ == 0 || m_timerQueuemap.empty())
		return NULL;

	time_t earliesttime = GetEarliestTime();
	if (earliesttime <= cur_time)
	{

		ptmp = RemoveFirstTimer();

		time_t newinqueutime = cur_time + (m_iWaitTime);
		LPSTRUC_MSG_HEADER tmpMsgHeader = (LPSTRUC_MSG_HEADER)p_memory->AllocMemory(sizeof(STRUC_MSG_HEADER), false);
		tmpMsgHeader->pConn = ptmp->pConn;
		tmpMsgHeader->iCurrsequence = ptmp->iCurrsequence;
		m_timerQueuemap.insert(std::make_pair(newinqueutime, tmpMsgHeader));
		m_cur_size_++;

		if (m_cur_size_ > 0)
		{
			m_timer_value_ = GetEarliestTime();
		}
		return ptmp;
	}
	return NULL;
}

void CSocekt::DeleteFromTimerQueue(lpgsx_connection_t pConn)
{
	std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos, posend;
	CMemory *p_memory = CMemory::GetInstance();

	CLock lock(&m_timequeueMutex);

lblMTQM:
	pos = m_timerQueuemap.begin();
	posend = m_timerQueuemap.end();
	for (; pos != posend; ++pos)
	{
		if (pos->second->pConn == pConn)
		{
			p_memory->FreeMemory(pos->second);
			m_timerQueuemap.erase(pos);
			--m_cur_size_;
			goto lblMTQM;
		}
	}
	if (m_cur_size_ > 0)
	{
		m_timer_value_ = GetEarliestTime();
	}
	return;
}

void CSocekt::clearAllFromTimerQueue()
{
	std::multimap<time_t, LPSTRUC_MSG_HEADER>::iterator pos, posend;

	CMemory *p_memory = CMemory::GetInstance();
	pos = m_timerQueuemap.begin();
	posend = m_timerQueuemap.end();
	for (; pos != posend; ++pos)
	{
		p_memory->FreeMemory(pos->second);
		--m_cur_size_;
	}
	m_timerQueuemap.clear();
}

void *CSocekt::ServerTimerQueueMonitorThread(void *threadData)
{
	ThreadItem *pThread = static_cast<ThreadItem *>(threadData);
	CSocekt *pSocketObj = pThread->_pThis;

	time_t absolute_time, cur_time;
	int err;

	while (g_stopEvent == 0)
	{

		if (pSocketObj->m_cur_size_ > 0)
		{

			absolute_time = pSocketObj->m_timer_value_;
			cur_time = time(NULL);
			if (absolute_time < cur_time)
			{

				std::list<LPSTRUC_MSG_HEADER> m_lsIdleList;
				LPSTRUC_MSG_HEADER result;

				err = pthread_mutex_lock(&pSocketObj->m_timequeueMutex);
				if (err != 0)
					gsx_log_stderr(err, "CSocekt::ServerTimerQueueMonitorThread()中pthread_mutex_lock()失败，返回的错误码为%d!", err);
				while ((result = pSocketObj->GetOverTimeTimer(cur_time)) != NULL)
				{
					m_lsIdleList.push_back(result);
				}
				err = pthread_mutex_unlock(&pSocketObj->m_timequeueMutex);
				if (err != 0)
					gsx_log_stderr(err, "CSocekt::ServerTimerQueueMonitorThread()pthread_mutex_unlock()失败，返回的错误码为%d!", err);
				LPSTRUC_MSG_HEADER tmpmsg;
				while (!m_lsIdleList.empty())
				{
					tmpmsg = m_lsIdleList.front();
					m_lsIdleList.pop_front();
					pSocketObj->procPingTimeOutChecking(tmpmsg, cur_time);
				}
			}
		}

		usleep(500 * 1000);
	}

	return (void *)0;
}

void CSocekt::procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t cur_time)
{
	CMemory *p_memory = CMemory::GetInstance();
	p_memory->FreeMemory(tmpmsg);
}
