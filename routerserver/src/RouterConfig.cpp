
/**
 * Copyright (C) Anny.
 * Copyright (C) Disvr, Inc.
 */

#include "RouterConfig.h"

RouterConfig::RouterConfig()
{
	Initialize();
}

RouterConfig::~RouterConfig()
{
	SAFE_DELETE(mDoc);
	SAFE_DELETE(mSvrQos);
}

void RouterConfig::Initialize()
{
	memset(mIPAddr, 0, sizeof(mIPAddr));
	mPort = 0;
	mBacklog = LISTEN_BACKLOG;
	mWorkers = 1;
	memcpy(mBaseConfFile, CONF_XML, strlen(CONF_XML) + 1);
	memcpy(mSvrConfFile, SVR_XML, strlen(SVR_XML) + 1);

	mDoc = new TiXmlDocument();
	mSvrQos = SvrQos::Instance();
	mMemPool = new wMemPool();
	mMemPool->Create(MEM_POOL_MAX);
}

void RouterConfig::GetBaseConf()
{
	bool bLoadOK = mDoc->LoadFile(mBaseConfFile);
	if (!bLoadOK)
	{
		LOG_ERROR(ELOG_KEY, "[startup] Load config file(conf.xml) failed");
		exit(2);
	}

	TiXmlElement *pRoot = mDoc->FirstChildElement();
	if (NULL == pRoot)
	{
		LOG_ERROR(ELOG_KEY, "[startup] Read root from config file(conf.xml) failed");
		exit(2);
	}
	
	TiXmlElement *pElement = NULL;
	TiXmlElement *pChildElm = NULL;

	//读取日志配置
	pElement = pRoot->FirstChildElement("LOG");
	if(NULL != pElement)
	{
		for(pChildElm = pElement->FirstChildElement(); pChildElm != NULL; pChildElm = pChildElm->NextSiblingElement())
		{
			const char *szKey = pChildElm->Attribute("KEY");
			const char *szFile = pChildElm->Attribute("FILE");
			const char *szLevel = pChildElm->Attribute("LEVEL");
			if (szKey != NULL && szFile != NULL && szLevel != NULL)
			{
				const char *szMaxFileSize = pChildElm->Attribute("MAX_FILE_SIZE");
				long long nMaxFileSize = szMaxFileSize == NULL ? atoi(szMaxFileSize) : 10*1024*1024;
				INIT_ROLLINGFILE_LOG(szKey, szFile, (LogLevel)atoi(szLevel), nMaxFileSize, 20);
			}
			else
			{
				LOG_ERROR(ELOG_KEY, "[startup] Get log config from conf.xml error");
			}
		}
	}
	else
	{
		LOG_ERROR(ELOG_KEY, "[startup] Get log config from conf.xml failed");
		exit(2);
	}

	pElement = pRoot->FirstChildElement("SERVER");
	if(NULL != pElement)
	{
		const char *szIPAddr = pElement->Attribute("IPADDRESS");
		const char *szPort = pElement->Attribute("PORT");
		const char *szBacklog = pElement->Attribute("BACKLOG");
		const char *szWorkers = pElement->Attribute("WORKERS");
		if (szIPAddr != NULL && szPort != NULL)
		{
			mPort = atoi(szPort);
			memcpy(mIPAddr, szIPAddr, MAX_IP_LEN);
		}
		else
		{
			LOG_ERROR(ELOG_KEY, "[startup] Get SERVER ip or port from conf.xml failed");
		}
		mBacklog = szBacklog != NULL ? atoi(szBacklog): mBacklog;
		mWorkers = szWorkers != NULL ? atoi(szWorkers): mWorkers;
	}
	else
	{
		LOG_ERROR(ELOG_KEY, "[startup] Get SERVER node from conf.xml failed");
		exit(2);
	}
}

void RouterConfig::GetSvrConf()
{
	bool bLoadOK = mDoc->LoadFile(mSvrConfFile);
	if (!bLoadOK)
	{
		LOG_ERROR(ELOG_KEY, "[startup] Load config file(svr.xml) failed");
		exit(2);
	}

	TiXmlElement *pElement = NULL;
	TiXmlElement *pChildElm = NULL;
	TiXmlElement *pRoot = mDoc->FirstChildElement();
	
	pElement = pRoot->FirstChildElement("SVRS");
	if(pElement != NULL)
	{
		int i = 0;
		for(pChildElm = pElement->FirstChildElement(); pChildElm != NULL ; pChildElm = pChildElm->NextSiblingElement())
		{
			i++;
			const char *szGid = pChildElm->Attribute("GID");
			const char *szXid = pChildElm->Attribute("XID");
			const char *szHost = pChildElm->Attribute("HOST");
			const char *szPort = pChildElm->Attribute("PORT");
			const char *szWeight = pChildElm->Attribute("WEIGHT");
			const char *szVersion = pChildElm->Attribute("VERSION");
			if (szGid != NULL && szXid != NULL && szHost != NULL && szPort != NULL)
			{
				struct SvrNet_t stSvr;

				stSvr.mGid = atoi(szGid);
				stSvr.mXid = atoi(szXid);
				stSvr.mPort = atoi(szPort);
				memcpy(stSvr.mHost, szHost, MAX_SVR_HOST);
				stSvr.mWeight = szWeight != NULL ? atoi(szWeight): stSvr.mWeight;
				stSvr.mVersion = szVersion != NULL ? atoi(szVersion): stSvr.mVersion;
				
				mSvrQos->SaveNode(stSvr);
			}
			else
			{
				LOG_ERROR("svr", "[startup] Parse svr config from svr.xml occur error: line(%d)!", i);
			}
		}
	}
	else
	{
		LOG_ERROR(ELOG_KEY, "[startup] Get SVRS node from svr.xml failed");
		exit(2);
	}

	SetModTime();
}

int RouterConfig::ReloadSvr(SvrNet_t* pBuffer)
{
	//TODO
	//mSvrQos->DelNode();
	return GetModSvr(pBuffer);
}

//不能删除节点（可修改WEIGHT=0属性，达到删除节点效果）
int RouterConfig::GetModSvr(SvrNet_t* pBuffer)
{
	bool bLoadOK = mDoc->LoadFile(mSvrConfFile);
	if (!bLoadOK)
	{
		LOG_ERROR(ELOG_KEY, "[modify svr] Load config file(svr.xml) failed");
		exit(2);
	}

	TiXmlElement *pElement = NULL;
	TiXmlElement *pChildElm = NULL;
	TiXmlElement *pRoot = mDoc->FirstChildElement();
	
	pElement = pRoot->FirstChildElement("SVRS");	
	
	int i = 0 , j = 0;
	if(pElement != NULL)
	{
		for(pChildElm = pElement->FirstChildElement(); pChildElm != NULL ; pChildElm = pChildElm->NextSiblingElement())
		{
			i++;
			const char *szGid = pChildElm->Attribute("GID");
			const char *szXid = pChildElm->Attribute("XID");
			const char *szHost = pChildElm->Attribute("HOST");
			const char *szPort = pChildElm->Attribute("PORT");
			const char *szWeight = pChildElm->Attribute("WEIGHT");
			const char *szVersion = pChildElm->Attribute("VERSION");
			if (szGid != NULL && szXid != NULL && szHost != NULL && szPort != NULL)
			{
				struct SvrNet_t stSvr;
				stSvr.mGid = atoi(szGid);
				stSvr.mXid = atoi(szXid);
				stSvr.mPort = atoi(szPort);
				memcpy(stSvr.mHost, szHost, MAX_SVR_HOST);
				stSvr.mWeight = szWeight != NULL ? atoi(szWeight): stSvr.mWeight;
				stSvr.mVersion = szVersion != NULL ? atoi(szVersion): stSvr.mVersion;
				
				/** 新配置始终下发，旧配置检测到version变化才下发*/
				if(mSvrQos->IsExistNode(stSvr))
				{
					if(mSvrQos->IsVerChange(stSvr))	//版本变化，确定下发配置
					{
						mSvrQos->SaveNode(stSvr);
						pBuffer[j++] = stSvr;
					}
				}
				else
				{
					mSvrQos->SaveNode(stSvr);	//添加新配置
					pBuffer[j++] = stSvr;
				}
			}
			else
			{
				LOG_ERROR("svr", "[modify svr] Parse svr config from svr.xml occur error: line(%d)!", i);
			}
		}

		SetModTime();
	}
	else
	{
		LOG_ERROR(ELOG_KEY, "[modify svr] Get SVRS node from svr.xml failed");
		return -1;
	}
	return j;
}

int RouterConfig::SetModTime()
{
	struct stat stBuf;
	int iRet = stat(mSvrConfFile, &stBuf);
	if (iRet == 0)
	{
		mMtime = stBuf.st_mtime;
		return mMtime;
	}
	return iRet; //-1
}

bool RouterConfig::IsModTime()
{
	struct stat stBuf;
	int iRet = stat(mSvrConfFile, &stBuf);
	if (iRet == 0 && stBuf.st_mtime > mMtime)
	{
		return true;
	}
	return false;
}

void RouterConfig::GetQosConf()
{
	mSvrQos->mRateWeight = 7;
	mSvrQos->mDelayWeight = 1;

    float rate = (float) mSvrQos->mRateWeight / (float) mSvrQos->mDelayWeight;
    
    /** rate 需在 0.01-100 之间*/
    if(rate > 100000)
    {
        mSvrQos->mRateWeight = 100000;
        mSvrQos->mDelayWeight = 1;
    }
    if(rate < 0.00001)
    {
        mSvrQos->mDelayWeight = 100000;
        mSvrQos->mRateWeight = 1;
    }

	/** 访问量配置 */
	mSvrQos->mReqCfg.mReqLimit = 10000;
	mSvrQos->mReqCfg.mReqMax = 10000;
	mSvrQos->mReqCfg.mReqMin = 10;
	mSvrQos->mReqCfg.mReqErrMin = 0.5;
	mSvrQos->mReqCfg.mReqExtendRate = 0.2;
	mSvrQos->mReqCfg.mRebuildTm = 60;	//5
	mSvrQos->mReqCfg.mPreTime = 4;

	mSvrQos->mRebuildTm = mSvrQos->mReqCfg.mRebuildTm;

	/** 并发量配置 */
	mSvrQos->mListCfg.mListLimit = 100;
	mSvrQos->mListCfg.mListMax = 400;
	mSvrQos->mListCfg.mListMin = 10;
	mSvrQos->mListCfg.mListExtendRate = 0.2;

	/** 宕机配置 */
	mSvrQos->mDownCfg.mReqCountTrigerProbe = 100000;
	mSvrQos->mDownCfg.mDownTimeTrigerProbe = 600;
	mSvrQos->mDownCfg.mProbeTimes = 3;
	mSvrQos->mDownCfg.mPossibleDownErrReq = 10;
	mSvrQos->mDownCfg.mPossbileDownErrRate = 0.5;
	mSvrQos->mDownCfg.mProbeBegin = 0;
	mSvrQos->mDownCfg.mProbeInterval = 3;
	mSvrQos->mDownCfg.mProbeNodeExpireTime = 600;

	if(!(mSvrQos->mReqCfg.mReqExtendRate > 0.001 && mSvrQos->mReqCfg.mReqExtendRate < 101))
	{
		LOG_ERROR(ELOG_KEY, "[startup] Init invalid req_extend_rate[%f]  !((ext > 0.001) && (ext < 101))", mSvrQos->mReqCfg.mReqExtendRate);
		exit(2);
	}

	if (mSvrQos->mReqCfg.mReqErrMin >= 1)
	{
		LOG_ERROR(ELOG_KEY, "[startup] Init invalid _req_err_min[%f]  _qos_req_cfg._req_err_min > 1", mSvrQos->mReqCfg.mReqErrMin);
		exit(2);
	}

	if (mSvrQos->mDownCfg.mPossbileDownErrRate > 1 || mSvrQos->mDownCfg.mPossbileDownErrRate < 0.01)
	{
		LOG_ERROR(ELOG_KEY, "[startup] Init invalid err_rate_to_def_possible_down[%f] > 1 or < _req_min[%f]", mSvrQos->mReqCfg.mReqErrMin, mSvrQos->mDownCfg.mPossbileDownErrRate);
		exit(2);
	}

	if (mSvrQos->mDownCfg.mProbeTimes < 3)
	{
		LOG_ERROR(ELOG_KEY, "[startup] Init invalid continuous_err_req_count_to_def_possible_down[%d] <3", mSvrQos->mDownCfg.mProbeTimes);
		exit(2);
	}

	if (mSvrQos->mReqCfg.mRebuildTm < 3)
	{
		LOG_ERROR(ELOG_KEY, "[startup] Init invalid rebuildtm[%d] < 3", mSvrQos->mReqCfg.mRebuildTm);
		exit(2);
	}
	
	if (mSvrQos->mReqCfg->mPreTime <= 0 || mSvrQos->mReqCfg->mPreTime > (mSvrQos->mRebuildTm / 2))
	{
		mSvrQos->mReqCfg->mPreTime = 2;
	}
}

