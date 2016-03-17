
/**
 * Copyright (C) Anny.
 * Copyright (C) Disvr, Inc.
 */

#include "SvrQos.h"

SvrQos::SvrQos()
{
	Initialize();
}

SvrQos::~SvrQos() {}

void SvrQos::Initialize() {}

/** 添加节点&路由 | 修改节点&路由权重 */
int SvrQos::SaveNode(struct SvrNet_t& stSvr)
{
	//访问量加1
	//...
	
	map<struct SvrNet_t, struct SvrStat_t*>::iterator mapReqIt = mMapReqSvr.find(stSvr);
	if (mapReqIt == mMapReqSvr.end())
	{
		if(AllocNode(stSvr) < 0)
		{
			//log
			return -1;
		}
	}
	else
	{
		//修改weight
		struct SvrNet_t &stKey = const_cast<struct SvrNet_t&> (mapReqIt->first);
		if (stSvr.mWeight != stKey.mWeight)
		{
			//log
            stKey.mWeight = stSvr.mWeight;
            ModRouteNode(stSvr);
		}
	}
	return 0;
}

/** 删除 节点&路由 */
int SvrQos::DelNode(struct SvrNet_t& stSvr)
{
	//访问量加1
	//...
	
    map<struct SvrNet_t, struct SvrStat_t*>::iterator mapReqIt = mMapReqSvr.find(stSvr);
    if (mapReqIt == mMapReqSvr.end())
	{
		//log
        return -1;
    }
    SvrStat_t* pSvrStat = mapReqIt->second;
    mMapReqSvr.erase(mapReqIt);
    
    DelRouteNode(stSvr);
    SAFE_DELETE(pSvrStat);
    return 0;
}

/** 修改 节点&路由 权重（权重为0删除节点） */
int SvrQos::ModNode(struct SvrNet_t& stSvr)
{
	//访问量加1
	//...
	
	if (stSvr.mWeight < 0)
	{
		//log
		return -1;
	}

	int iRet = 0;
	if (stSvr.mWeight == 0)		//权重为0删除节点
	{
		//log
		iRet = DelNode(stSvr);
	}
	else
	{
		//log
		if(stSvr.mWeight > MAX_WEIGHT)
		{
			//log
			stSvr.mWeight = MAX_WEIGHT;
		}
		iRet = SaveNode(stSvr);
	}
	return iRet;
}

/** 调用数上报 */
int NotifyNode(struct SvrNet_t& stSvr)
{
	//访问量加1
	//...
	
	//log
	
    map<struct SvrNet_t, struct SvrStat_t*>::iterator mapReqIt = mMapReqSvr.find(stSvr);
    if (mapReqIt == mMapReqSvr.end())
	{
		//log
        return -1;
    }

	SvrStat_t* pSvrStat = mapReqIt->second;
	pSvrStat->mInfo.mReqAll++;
	pSvrStat->mInfo.mSReqAll++;
	
	//log
	
	return 0;
}

/** 调用结果上报 */
int CallerNode(struct SvrNet_t& stSvr, struct SvrCaller_t& stCaller)
{
	//访问量加1
	//...
	
	//log
	
	if (stCaller.mReqUsetimeUsec <= 0)
	{
		stCaller.mReqUsetimeUsec = 1;
	}

    map<struct SvrNet_t, struct SvrStat_t*>::iterator mapReqIt = mMapReqSvr.find(stSvr);
    if (mapReqIt == mMapReqSvr.end())
	{
		//log
        return -1;
    }
   
    SvrStat_t* pSvrStat = mapReqIt->second;
    if (stCaller.mReqRet >= 0)
    {
    	pSvrStat->mInfo.mReqSuc += stCaller.mReqCount;
    	pSvrStat->mInfo.mSReqSuc += stCaller.mReqCount;
    	pSvrStat->mInfo.mTotalUsec += stCaller.mReqUsetimeUsec;
    	pSvrStat->mInfo.mContErrCount = 0;
    }
    else
    {
    	pSvrStat->mInfo.mReqErrRet += stCaller.mReqCount;
    	pSvrStat->mInfo.mSReqErrRet += stCaller.mReqCount;
    	pSvrStat->mInfo.mContErrCount += stCaller.mReqCount;
    }

    SvrKind_t stNode(stSvr);
    RebuildRoute(stNode);
    return 0;
}

/** 添加 节点&路由 */
int AllocNode(struct SvrNet_t& stSvr)
{
	struct SvrStat_t* pSvrStat = new SvrStat_t();
	if (pSvrStat == NULL)
	{
		//log
		SAFE_DELETE(pSvrStat);
		return -1;
	}

	gettimeofday(&pSvrStat->mInfo.mBuildTm, NULL);	//重建时间
	if(LoadStatCfg(stSvr, pSvrStat) < 0)
	{
		//log
		SAFE_DELETE(pSvrStat);
		return -1;
	}

	mMapReqSvr[stSvr] = pSvrStat;
	pSvrStat->mRebuildTm = mRebuildTm;	//设置节点重建时间间隔
	AddRouteNode(stSvr, pSvrStat);		//将新Req节点加入到节点路由map
	//log
	return 0;
}

/** 单次获取路由 */
int QueryNode(struct SvrNet_t& stSvr)
{
	//统计信息
	
	int iAck = GetRouteNode(stSvr);

	//log
	return iAck;
}

/** 加载阈值配置 */
int LoadStatCfg(struct SvrNet_t& stSvr, struct SvrStat_t* pSvrStat)
{
	pSvrStat->mInfo.InitInfo(stSvr);
	
	{
		pSvrStat->mReqCfg = mReqCfg;
	}

	{
		pSvrStat->mListCfg = mListCfg;
	}
	return 0;
}

/** 添加新路由 */
int AddRouteNode(struct SvrNet_t& stSvr, struct SvrStat_t* pSvrStat)
{
    SvrKind_t stNode(stSvr);
    stNode.mRebuildTm = pSvrStat->mReqCfg.mRebuildTm;

    multimap<float, struct SvrNode_t>* &pTable = mRouteTable[stSvr];
    if(pTable == NULL)
	{
        pTable = new multimap<float, SvrNode_t>;
    }
    if(pTable == NULL)
    {
    	//log
    	return -1;
    }

    //路由表中已有相关节点，取优先级最低的那个作为新节点的默认值
    multimap<float, struct SvrNode_t>::reverse_iterator it = pTable->rbegin();
    for(; it != pTable->rend(); ++it)
	{
        struct SvrNode_t& stNode = it->second;

        pSvrStat->mReqCfg.mReqLimit = stNode.mStat->mReqCfg.mReqLimit;
        pSvrStat->mReqCfg.mReqMin = stNode.mStat->mReqCfg.mReqMin;
        pSvrStat->mReqCfg.mReqMax = stNode.mStat->mReqCfg.mReqMax;
        pSvrStat->mReqCfg.mReqErrMin = stNode.mStat->mReqCfg.mReqErrMin;
        pSvrStat->mReqCfg.mReqExtendRate = stNode.mStat->mReqCfg.mReqExtendRate;

        //正在运行时，突然新加一个服务器，如果mPreAll=0。GetRoute函数分配服务器时，会连续新加的服务器
        //导致：被调扩容时，新加的服务器流量会瞬间爆增，然后在一个周期内恢复正常
        pSvrStat->mInfo = stNode.mStat->mInfo;	//统计信息
        break;
    }

    struct SvrNode_t stRouteInfo(stSvr, pSvrStat);
    pTable->insert(make_pair(stRouteInfo.mKey, stRouteInfo));
    return 0;
}

/** 单次获取路由 */
int GetRouteNode(struct SvrNet_t& stSvr)
{
	//log
	
	if(stSvr.mGid <= 0 || stSvr.mXid <= 0)
	{
		return -1;
	}

	SvrKind_t stNode(stSvr);
	
	/** 重建路由 */
	RebuildRoute(stNode);

	map<struct SvrKind_t,  multimap<float, struct SvrNode_t>* >::iterator rtIt = mRouteTable.find(stNode);
	//找不到相关路由
	if(rtIt == mRouteTable.end())
	{
        return -1;
    }

    struct SvrKind_t &stKind = (struct SvrKind_t &) (rtIt->first);
    multimap<float, struct SvrNode_t>* pTable = rtIt->second;
	if(pTable == NULL || pTable->empty())
	{
		return QOS_RTN_OVERLOAD;
	}

	int iIndex = stKind.mPindex;
	if (iIndex < 0 || iIndex >= pTable->size())
	{
		iIndex = 0;
		stKind.mPindex = 0;
	}
	
	multimap<float, struct SvrNode_t>::iterator it = pTable->begin() , it1 = pTable->begin();
	
	return 0;
}

int RebuildErrRoute(struct SvrKind_t& stItem)
{
	//
}

/** 重建路由 */
int RebuildRoute(struct SvrKind_t& stItem, int bForce)
{
	map<struct SvrKind_t,  multimap<float, struct SvrNode_t>* >::iterator rtIt = mRouteTable.find(stItem);
	if (rtIt == mRouteTable.end())
	{
		mAllReqMin = true;
        multimap<float, struct SvrNode_t>* pNewTable = new multimap<float, struct SvrNode_t>;
        RebuildErrRoute(stItem, pNewTable, 1, 1, 1);

        if(!pNewTable->empty())
		{
            stItem.inner_change++;
            stItem.pindex = 0;
            mRouteTable.insert(make_pair(stItem, pNewTable));
        }
        else
        {
            SAFE_DELETE(pNewTable);
        }
		return -1;
	}
	
	struct SvrKind_t &stKind = (struct SvrKind_t &) (rtIt->first);
	multimap<float, struct SvrNode_t>* pTable = rtIt->second;
	if(pTable == NULL || pTable->empty())
	{
		return -1;
	}

	int iCurTm = time(NULL);
	//如果还没到路由rebuild时间直接返回，默认一分钟rebuild一次
	if((iCurTm - stKind.mPtm) > 2 * stKind.mRebuildTm || (stKind.mPtm - iCurTm) > 2 * stKind.mRebuildTm) 
	{
		//防时间跳变
		stKind.mPtm = iCurTm;
	}
    if(!bForce && iCurTm - stKind.mPtm < stKind.mRebuildTm)
	{
        return -1;
    }
    //rebuild时间
    stKind.mPtm = iCurTm;
    stItem.mPtm = iCurTm;

    //当前时间
    struct timeval nowTm;
    gettimeofday(&nowTm, NULL);

    unsigned int iMaxDelay = DELAY_MAX;
    bool bErrRateBig = true;  //所有被调都过载标志

    //int iReqLimit = 0;
    int iReqAll = 0, iReqSuc = 0, iReqRej = 0, iReqErrRet = 0 , iReqErrTm = 0;
    float fAvgErrRate = 0;

    int iRouteTotalReq = 0;
    float fHighWeight = 0, fCfgErrRate = 0, fBestLowPri = 1; /*, avg_err_rate = 0, total_err_rate = 0, high_suc_rage = 0, node_err_rate = 0;*/

    multimap<float, struct SvrNode_t>::iterator it = pTable->begin();
    for(; it != pTable.end(); it++)
    {
    	struct SvrInfo_t &si = it->second.mStat->mInfo;

    	iReqAll = si.mReqAll;
    	iReqSuc = si.mReqSuc;
    	iReqRej = si.mReqRej;
    	iReqErrRet = si.mReqErrRet;
    	iReqErrTm = si.mReqErrTm;
    	fAvgErrRate = si.mAvgErrRate;

    	iRouteTotalReq += iReqAll - iReqRej;	//总数
    	if(iReqAll < iReqSuc + iReqErrTm + mReqErrRet) 
    	{
    		iReqAll = iReqSuc + mReqErrRet + iReqErrTm;	//总请求数
    	}

    	//fCfgErrRate = iter->second.m_node_info->_qos_req_cfg._req_err_min;
    	//node_err_rate = 1 - q.ok_rate;
    	//if(node_err_rate < cfg_err_rate + q.avg_err_rate) err_rate_big = false;

    }


    return 0;
}

/** 修改路由权重 */
int ModRouteNode(struct SvrNet_t& stSvr)
{
	SvrKind_t stNode(stSvr);
    map<struct SvrKind_t,  multimap<float, struct SvrNode_t>* >::iterator rtIt = mRouteTable.find(stNode);
    map<struct SvrKind_t, list<SvrNode_t>* >::iterator reIt = mErrTable.find(stNode);

	if(rtIt == mRouteTable.end() && reIt == mErrTable.end())
	{
		//log
        return -1;
    }

	if(rtIt != mRouteTable.end())
	{
		multimap<float, struct SvrNode_t>* pTable = rtIt->second;
		multimap<float, struct SvrNode_t>::iterator it = pTable->begin();
		for(; it != pTable->end(); it++)
		{
			if(it->second.mNet == stSvr)	//请求节点
			{
                if (it->second.mNet.mWeight != stSvr.mWeight)
				{
                    it->second.mNet.mWeight = stSvr.mWeight;	//设置为请求权重
                }
				return 0;
			}
		}
	}
	if(reIt != mErrTable.end())
	{
		list<SvrNode_t>* pNode = reIt->second;
		list<SvrNode_t>::iterator it = pNode->begin();
		for(; it != pNode->end(); it++)
		{
			if(it->mNet == stSvr)
			{
                if(it->mNet.mWeight != stSvr.mWeight)
				{
                    it->mNet.mWeight = stSvr.mWeight;
                }
				return 0;
			}
		}
	}

	//log
	return 0;
}

/** 删除路由节点 */
int DelRouteNode(struct SvrNet_t& stSvr)
{
    SvrKind_t stNode(stSvr);
    map<struct SvrKind_t,  multimap<float, struct SvrNode_t>* >::iterator rtIt = mRouteTable.find(stNode);
    map<struct SvrKind_t, list<SvrNode_t>* >::iterator reIt = mErrTable.find(stNode);

	if(rtIt == mRouteTable.end() && reIt == mErrTable.end())
	{
        return -1;
    }

    unsigned int iDelCount = 0;
	if(rtIt != mRouteTable.end())
	{
		multimap<float, struct SvrNode_t>* pTable = rtIt->second;
        if (pTable != NULL)
		{
            multimap<float, struct SvrNode_t>::iterator it = pTable->begin();

            while(it != pTable->end())
			{
                if(it->second.mNet == stSvr)
				{
                    pTable->erase(it++);
                    if(pTable->empty())
					{
                        mRouteTable.erase(rtIt);
                        SAFE_DELETE(pTable);
                    }
                    iDelCount++;
                    break;
                }
				else
				{
                    it++;
                }
            }
        }
        else
        {
        	//log
        }
	}
	if (reIt != mErrTable.end())
	{
		list<SvrNode_t>* pNode = reIt->second;
		if (pNode != NULL)
		{
			list<SvrNode_t>::iterator it = pNode->begin();
			while(it != pNode.end())
			{
				if (it->mNet == stSvr)
				{
					pNode->erase(it++);
					if (pNode->empty())
					{
                        mErrTable.erase(reIt);
                        SAFE_DELETE(it);
					}
					iDelCount++;
					break;
				}
				else
				{
					it++;
				}
			}
		}
		else
		{
			//log
		}
	}

	if(iDelCount > 0)
	{
		return 0;
	}

	//log
	return -1;
}












int SvrQos::InitSvr(SvrNet_t* pSvr, int iNum)
{
	if (iNum <= 0)
	{
		return -1;
	}
	Final();

	Svr_t *pTmpSvr;
	for(int i = 0; i < iNum ; i++)
	{
		if (pSvr[i] == NULL)
		{
			continue;
		}

		pTmpSvr = new Svr_t(pSvr[i]);	//SvrNet -> Svr
		if (pTmpSvr == NULL)
		{
			LOG_ERROR(ELOG_KEY, "[runtime] init id(%d) svr failed", pSvr[i]->mId);
			continue;
		}
		mSvr.push_back(pTmpSvr);
	}

	SvrRebuild();
	return 0;
}

int SvrQos::ReloadSvr(SvrNet_t *pSvr, int iNum)
{
	return InitSvr(pSvr, iNum);
}

//更新不同配置 & 添加新配置
int SvrQos::SyncSvr(SvrNet_t *pSvr, int iNum)
{
	if (iNum <= 0)
	{
		return -1;
	}

	int j = 0;
	Svr_t *pTmpSvr;
	for(int i = 0; i < iNum ; i++)
	{
		vector<Svr_t*>::iterator it = GetItById((pSvr+i)->mId);
		if (it != mSvr.end())
		{
			if(IsChangeSvr(*it, pSvr+i))
			{
				//更新配置
				SAFE_DELETE(*it);
				mSvr.erase(it);
				pTmpSvr = new Svr_t(pSvr[i]);	//SvrNet -> Svr
				if (pTmpSvr == NULL)
				{
					LOG_ERROR(ELOG_KEY, "[runtime] init id(%d) svr failed", pSvr[i]->mId);
					continue;
				}
				mSvr.push_back(pTmpSvr);
				j++;
			}
		}
		else
		{
			//添加新配置
			pTmpSvr = new Svr_t(pSvr[i]);
			if (pTmpSvr == NULL)
			{
				LOG_ERROR(ELOG_KEY, "[runtime] init id(%d) svr failed", pSvr[i]->mId);
				continue;
			}
			mSvr.push_back(pTmpSvr);
			j++;
		}
	}
	SvrRebuild();	
	return j;
}

int SvrQos::GetSvrAll(SvrNet_t* pBuffer)
{
	vector<Svr_t*>::iterator it = mSvr.begin();
	if (mSvr.size() < 1)
	{
		return 0;
	}

	for(int i = 0; it != mSvr.end(); i++, it++)
	{
		pBuffer[i] = ((struct SvrNet_t)**it);   //Svr_t => SvrNet_t
	}
	return mSvr.size();
}

int SvrQos::GetAllSvrByGXid(SvrNet_t* pBuffer, int iGid, int iXid)
{
	string sGid = Itos(iGid);
	string sXid = Itos(iXid);
	string sGXid = sGid + "-" + sXid;
	
	vector<Svr_t*> vSvr;
	int iNum = 0;
	map<string, vector<Svr_t*> >::iterator mn = mSvrByGXid.find(sGXid);
	if(mn != mSvrByGXid.end())
	{
		vSvr = mn->second;	//已排序
		iNum = vSvr.size();

		vector<Svr_t*>::iterator it = vSvr.begin();
		for(int i = 0; i < iNum && it != vSvr.end(); i++, it++)
		{
			pBuffer[i] = ((struct SvrNet_t)**it);   //Svr_t => SvrNet_t
		}
	}
	else
	{
		iNum = 0;
	}
	return iNum;
}

//获取gid、xid下的某一svr
int SvrQos::GetSvrByGXid(SvrNet_t* pBuffer, int iGid, int iXid)
{
	string sGid = Itos(iGid);
	string sXid = Itos(iXid);
	string sGXid = sGid + "-" + sXid;
	
	int iId = 0;
	map<string, vector<Svr_t*> >::iterator mn = mSvrByGXid.find(sGXid);
	if(mn != mSvrByGXid.end())
	{
		iId = GXidWRRSvr(pBuffer, sGXid, mn->second);
	}
	return iId;
}

//加权轮询
int SvrQos::GXidWRRSvr(SvrNet_t* pBuffer, string sKey, vector<Svr_t*> vSvr)
{
	map<string, StatcsGXid_t*>::iterator mn = mStatcsGXid.find(sKey);
	int iLen = vSvr.size();
	if (iLen <= 0 || mn == mStatcsGXid.end())
	{
		return -1;	//讲道理的话，这里不会被执行
	}
	int iLoop = iLen;
	StatcsGXid_t *pStatcs = mn->second;

	LOG_DEBUG(ELOG_KEY, "[runtime] WRR init id,%d", pStatcs->mIdx);
	LOG_DEBUG(ELOG_KEY, "[runtime] WRR init cur,%d", pStatcs->mCur);
	LOG_DEBUG(ELOG_KEY, "[runtime] WRR init gcd,%d", pStatcs->mGcd);

	while(iLoop-- >= 0)
	{
		pStatcs->mIdx = (pStatcs->mIdx + 1) % iLen;
		if (pStatcs->mIdx == 0)
		{
			pStatcs->mCur = pStatcs->mCur - pStatcs->mGcd;
			if (pStatcs->mCur <= 0)
			{
				pStatcs->mCur = vSvr[0]->mWeight; //最大值
				if (pStatcs->mCur <= 0)
				{
					return -1;
				}
			}
		}
		//权重为0，禁用
		if (vSvr[pStatcs->mIdx]->mWeight == 0)
		{
			continue;
		}
		else if (vSvr[pStatcs->mIdx]->mShutdown == 1)	//是否故障
		{
			vSvr[pStatcs->mIdx]->mRfuNum++;
		}
		else if (vSvr[pStatcs->mIdx]->mWeight >= pStatcs->mCur)
		{
			if (vSvr[pStatcs->mIdx]->mPreNum <= 0 && (vSvr[pStatcs->mIdx]->mOverLoad == 1 || CalcOverLoad(vSvr[pStatcs->mIdx]) == 1))	//过载
			{
				vSvr[pStatcs->mIdx]->mRfuNum++;
			} 
			else
			{
				if(vSvr[pStatcs->mIdx]->mOverLoad == 1 || CalcOverLoad(vSvr[pStatcs->mIdx]) == 1)
				{
					vSvr[pStatcs->mIdx]->mPreNum--;
				}
				pStatcs->mSumConn++;
				vSvr[pStatcs->mIdx]->mUsedNum++;
				
				*pBuffer = (struct SvrNet_t) *vSvr[pStatcs->mIdx];	//Svr_t => SvrNet_t
				return vSvr[pStatcs->mIdx]->mId;
			}
		}
	}
	return -1;
}

//接受上报数据
void SvrQos::ReportSvr(SvrReportReqId_t *pReportSvr)
{
	vector<Svr_t*>::iterator it = GetItById(pReportSvr->mId);
	if (it != mSvr.end() && pReportSvr->mDelay != 0 && pReportSvr->mStu != SVR_UNKNOWN)
	{
		SetGXDirty((*it), 1);	//设置同组所有Svr更新位(重新设置对应权值、错误率)
		(*it)->mDelay = pReportSvr->mDelay;
		if (pReportSvr->mStu == SVR_SUC)	//成功
		{
			//(*it)->mOkNum++;
		}
		else if(pReportSvr->mStu == SVR_ERR)	//失败
		{
			//(*it)->mErrNum++;
		}

		//释放连接
		ReleaseConn(*it);
		LOG_DEBUG(ELOG_KEY,"[runtime] recvive a report message(shm), Ok(%d),Err(%d)", (*it)->mOkNum, (*it)->mErrNum);
	}
}

void SvrQos::Statistics()
{
	if(mSvr.size() <= 0) return;

	int iSunConn = -1;
	vector<Svr_t*>::iterator it;
	for (it = mSvr.begin(); it != mSvr.end(); it++)
	{
		if ((*it)->mDirty == 1)
		{
			iSunConn = GetSumConn(*it);
			(*it)->mOkRate = (float)(*it)->mOkNum / iSunConn;
			(*it)->mErrRate = (float)(*it)->mErrNum / iSunConn;
			(*it)->mRfuRate = (float)(*it)->mRfuNum / iSunConn;

			(*it)->mPreNum = CalcPre(*it);	//每一周期预取数重置为1 （后期实现扩张、收缩）
			(*it)->mShutdown = CalcShutdown(*it);	//非压力故障
			(*it)->mWeight = CalcWeight(*it);
			
			(*it)->ClearStatistics();	//清除统计数据
		}
	}
	SvrRebuild();
	LOG_DEBUG(ELOG_KEY,"[runtime] statistics svr success");
}

short SvrQos::CalcPre(Svr_t* stSvr)
{
	return 1;
}

short SvrQos::CalcShutdown(Svr_t* stSvr)
{
	return 0;
}

//计算动态权重
int SvrQos::CalcWeight(Svr_t* stSvr)
{
	if (stSvr->mDelay == 0 && stSvr->mOkRate == 0)	//从未访问过
	{
		return stSvr->mWeight;
	}

	Svr_t pSvr[MAX_SVR_NUM];
	int iNum = GetAllSvrByGXid(pSvr, stSvr->mGid, stSvr->mXid);
	if (iNum <= 0)
	{
		return stSvr->mWeight;	//讲道理的话，这里不可能执行到（至少有其自身）
	}

	int iDelay = stSvr->mDelay;
	float fOkRate = stSvr->mOkRate;
	for (int i = 0; i < iNum; ++i)
	{
		if (iDelay > pSvr[i].mDelay && pSvr[i].mDelay != 0)
		{
			iDelay = pSvr[i].mDelay;	//最小延时
		}
		if (fOkRate < pSvr[i].mOkRate && pSvr[i].mOkRate != 0)
		{
			fOkRate = pSvr[i].mOkRate;	//最大成功率
		}
	}

	float fLoadWeight = 1/(((float)stSvr->mDelay/iDelay) * (fOkRate/stSvr->mOkRate));
	return (int)(fLoadWeight * ZOOM_WEIGHT * stSvr->mSWeight);	//放大
}

//检测是否过载
//TODO
short SvrQos::CalcOverLoad(Svr_t* stSvr)
{
	if (stSvr->mErrNum > 0 && stSvr->mPreNum <= 0)	//本周期
	{
		stSvr->mOverLoad = 1;	//过载
	}
	else if(stSvr->mErrRate > ERR_RATE_THRESHOLD)	//上一周期错误率
	{
		stSvr->mOverLoad = 1; //过载(存在故障可能性)
		//stSvr->mShutdown = 1;
	}
	else
	{
		stSvr->mOverLoad = 0;
	}
	return stSvr->mOverLoad;
}

//设置同组所有svr更新位
void SvrQos::SetGXDirty(Svr_t* stSvr, int iDirty)
{
	Svr_t pSvr[MAX_SVR_NUM];
	int iNum = GetAllSvrByGXid(pSvr, stSvr->mGid, stSvr->mXid);
	if (iNum <= 0)
	{
		return;	//讲道理的话，这里不可能执行到（至少有其自身）
	}
	for (int i = 0; i < iNum; ++i)
	{
		 pSvr[i].mDirty = iDirty;
	}
}

vector<Svr_t*>::iterator SvrQos::GetItFromV(Svr_t* pSvr)
{
	vector<Svr_t*>::iterator it;
	for (it = mSvr.begin(); it != mSvr.end(); it++)
	{
		if (**it == *pSvr)
		{
			break;
		}
	}
	return it;
}

vector<Svr_t*>::iterator SvrQos::GetItById(int iId)
{
	vector<Svr_t*>::iterator it;
	for (it = mSvr.begin(); it != mSvr.end(); it++)
	{
		if ((*it)->mId == iId)
		{
			break;
		}
	}
	return it;
}

bool SvrQos::IsChangeSvr(const SvrNet_t* pR1, const SvrNet_t* pR2)
{
	if (pR1->mVersion!=pR2->mVersion || pR1->mGid!=pR2->mGid || pR1->mXid!=pR2->mXid || pR1->mSWeight!=pR2->mSWeight || pR1->mPort!=pR2->mPort || strncmp(pR1->mIp,pR2->mIp,MAX_SVR_IP_LEN)!=0)
	{
		return true;
	}
	return false;
}

void SvrQos::DelContainer()
{
	mSvrByGXid.clear();
	mStatcsGXid.clear();
}

void SvrQos::Final()
{
	DelContainer();
	for(vector<Svr_t*>::iterator it = mSvr.begin(); it != mSvr.end(); it++)
	{
		SAFE_DELETE(*it);
	}
	for(map<string, StatcsGXid_t*>::iterator it = mStatcsGXid.begin(); it != mStatcsGXid.end(); it++)
	{
		SAFE_DELETE(it->second);
	}
	mSvr.clear();
}

//整理容器 & 同步其他容器
void SvrQos::SvrRebuild()
{
	if(mSvr.size() <= 0) return;
	sort(mSvr.begin(), mSvr.end(), GreaterSvr);	//降序排序

	DelContainer();
	string sGid, sXid, sGXid;
	vector<Svr_t*> vSvr;
	for(vector<Svr_t*>::iterator it = mSvr.begin(); it != mSvr.end(); it++)
	{
		sGid = Itos((*it)->mGid); 
		sXid = Itos((*it)->mXid);
		sGXid = sGid + "-" + sXid;

		//mSvrByGXid
		map<string, vector<Svr_t*> >::iterator mg = mSvrByGXid.find(sGXid);
		vSvr.clear();
		if(mg != mSvrByGXid.end())
		{
			vSvr = mg->second;
			mSvrByGXid.erase(mg);
		}
		vSvr.push_back(*it);
		mSvrByGXid.insert(pair<string, vector<Svr_t*> >(sGXid, vSvr));		
	}

	StatcsGXid_t *pStatcsGXid = 0;
	map<string, vector<Svr_t*> >::iterator it = mSvrByGXid.begin();
	for(; it != mSvrByGXid.end(); it++)
	{
		map<string, StatcsGXid_t*>::iterator mgx = mStatcsGXid.find(it->first);
		if(mgx == mStatcsGXid.end())
		{
			pStatcsGXid = new StatcsGXid_t();
		}
		else
		{
			pStatcsGXid = mgx->second;
			//mStatcsGXid.erase(mgx);
		}
		pStatcsGXid->mGcd = GcdWeight(it->second, it->second.size());
		mStatcsGXid.insert(pair<string, StatcsGXid_t*>(it->first, pStatcsGXid));
	}
}

void SvrQos::ReleaseConn(Svr_t* stSvr)
{
	string sGid, sXid, sGXid;
	sGid = Itos(stSvr->mGid); 
	sXid = Itos(stSvr->mXid);
	sGXid = sGid + "-" + sXid;
	map<string, StatcsGXid_t*>::iterator mgx = mStatcsGXid.find(sGXid);
	StatcsGXid_t *pStatcsGXid = 0;
	if(mgx == mStatcsGXid.end())
	{
		pStatcsGXid = mgx->second;
		pStatcsGXid->mSumConn--;
	}
}

int SvrQos::GetSumConn(Svr_t* stSvr)
{
	string sGid, sXid, sGXid;
	sGid = Itos(stSvr->mGid); 
	sXid = Itos(stSvr->mXid);
	sGXid = sGid + "-" + sXid;

	map<string, StatcsGXid_t*>::iterator mgx = mStatcsGXid.find(sGXid);
	StatcsGXid_t *pStatcsGXid = 0;
	if(mgx == mStatcsGXid.end())
	{
		pStatcsGXid = mgx->second;
		return pStatcsGXid->mSumConn;
	}
	return -1;
}

int SvrQos::GcdWeight(vector<Svr_t*> vSvr, int n)
{
	if (n == 1) 
	{
		return vSvr[0]->mWeight;
	}
	return Gcd(vSvr[n-1]->mWeight, GcdWeight(vSvr, n - 1));
}