
#ifndef __GSX_C_SLOGIC_H__
#define __GSX_C_SLOGIC_H__

#include <sys/socket.h>
#include "gsx_c_socket.h"

class CLogicSocket : public CSocekt
{
public:
	CLogicSocket();
	virtual ~CLogicSocket();
	virtual bool Initialize();

public:
	void SendNoBodyPkgToClient(LPSTRUC_MSG_HEADER pMsgHeader, unsigned short iMsgCode);

	bool _HandleRegister(lpgsx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char *pPkgBody, unsigned short iBodyLength);
	bool _HandleLogIn(lpgsx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char *pPkgBody, unsigned short iBodyLength);
	bool _HandlePing(lpgsx_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char *pPkgBody, unsigned short iBodyLength);

	virtual void procPingTimeOutChecking(LPSTRUC_MSG_HEADER tmpmsg, time_t cur_time);

public:
	virtual void threadRecvProcFunc(char *pMsgBuf);
};

#endif
