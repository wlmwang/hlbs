
/**
 * Copyright (C) Anny.
 * Copyright (C) Disvr, Inc.
 */

#ifndef _ROUTER_CLIENT_TASK_H_
#define _ROUTER_CLIENT_TASK_H_

#include <arpa/inet.h>

#include "wType.h"
#include "wHeadCmd.h"
#include "wTcpTask.h"
#include "wLog.h"

class RouterClientTask : public wTcpTask
{
	public:
		RouterClientTask(int iNewSocket, struct sockaddr_in *stSockAddr);
		
        ~RouterClientTask();
		
		virtual int HandleRecvMessage(char * pBuffer, int nLen);
		virtual void ProcessSendMessage();

		virtual int HandleSendMessage(char * pBuffer, int nLen);
		virtual void ProcessRecvMessage();
		virtual void ParseRecvMessage(struct wHeadCmd* pHeadCmd ,char *pBuffer,int iLen);
	
};

#endif
