
/**
 * Copyright (C) Anny.
 * Copyright (C) Disvr, Inc.
 */

#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wLog.h"
#include "wMisc.h"
#include "AgentServer.h"
#include "AgentConfig.h"
#include "BaseCommand.h"

AgentServer::AgentServer():wTcpServer<AgentServer>("路由服务器")
{
	if(mStatus == SERVER_STATUS_INIT)
	{
		Initialize();
	}
}

AgentServer::~AgentServer() 
{
	SAFE_DELETE(mRouterConn);
}

/**
 * 初始化
 */
void AgentServer::Initialize()
{
	mRouterConn = new wMTcpClient<AgentServerTask>();
}

wTcpTask* AgentServer::NewTcpTask(wSocket *pSocket)
{
	wTcpTask *pTcpTask = new AgentServerTask(pSocket);
	return pTcpTask;
}

void AgentServer::ConnectRouter()
{
	AgentConfig *pConfig = AgentConfig::Instance();
	
	//mRouterConn
	bool bRet = mRouterConn->GenerateClient(ROUTER_SERVER_TYPE, "RouterFromAgent", pConfig->mRouterIPAddr, pConfig->mRouterPort);
	if (!bRet)
	{
		cout << "Connect to RouterServer failed! Please start it" <<endl;
		LOG_ERROR("default", "Connect to RouterServer failed");
		exit(1);
	}
	
	//发送初始化rtbl配置请求
	InitRtblReq();
}

int AgentServer::InitRtblReq()
{
	wMTcpClient<AgentServerTask>* pRouterConn = RouterConn();
	if(pRouterConn == NULL)
	{
		return -1;
	}
	wTcpClient<AgentServerTask>* pClient = pRouterConn->OneTcpClient(ROUTER_SERVER_TYPE);
	if(pClient != NULL && pClient->TcpTask())
	{
		RtblReqInit_t vRtl;
		return pClient->TcpTask()->SyncSend((char *)&vRtl, sizeof(vRtl));
	}
	return -1;
}

int AgentServer::ReloadRtblReq()
{
	wMTcpClient<AgentServerTask>* pRouterConn = RouterConn();
	if(pRouterConn == NULL)
	{
		return -1;
	}
	wTcpClient<AgentServerTask>* pClient = pRouterConn->OneTcpClient(ROUTER_SERVER_TYPE);
	if(pClient != NULL && pClient->TcpTask())
	{
		RtblReqReload_t vRtl;
		return pClient->TcpTask()->SyncSend((char *)&vRtl, sizeof(vRtl));
	}
	return -1;
}

//准备工作
void AgentServer::PrepareRun()
{
	mRouterConn->PrepareStart();
	ConnectRouter(); //连接Router服务
}

void AgentServer::Run()
{
	mRouterConn->Start(false);
}
