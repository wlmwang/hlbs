
/**
 * Copyright (C) Anny.
 * Copyright (C) Disvr, Inc.
 */

#ifndef _W_MPROCESS_H_
#define _W_MPROCESS_H_

#include <map>
#include <vector>

#include "wType.h"
#include "wLog.h"
#include "wNoncopyable.h"
#include "wProcess.h"
#include "wShm.h"
#include "wMsgQueue.h"

#define MAX_PROCESSES         1024

class wMProcess : private wNoncopyable
{
	public:
		wMProcess();
		virtual ~wMProcess();

		int PrepareStart();
		int Start(int iNum = 8);

		virtual wProcess* NewProcess(int iIdx, string sTitle);
		pid_t GenerateWorker(int iIdx, string sTitle);
	protected:
		map<int, wProcess*> mProcList;
		string mName;
		pid_t mPid;

		wShm* mShm;
};

#endif