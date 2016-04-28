
/**
 * Copyright (C) Anny Wang.
 * Copyright (C) Hupu, Inc.
 */

#ifndef _ROUTER_MASTER_H_
#define _ROUTER_MASTER_H_

#include <map>
#include <vector>

#include "wCore.h"
#include "wMaster.h"
#include "RouterWorker.h"
#include "RouterConfig.h"
#include "RouterServer.h"

class RouterMaster : public wMaster<RouterMaster>
{
	public:
		RouterMaster();
		virtual ~RouterMaster();
		void Initialize();
		
		virtual void PrepareRun();
		virtual void Run();
		virtual void ReconfigMaster();
		
		virtual wWorker* NewWorker(int iSlot = 0);
	
	protected:
		char *mTitle;

		RouterConfig *mConfig;
		RouterServer *mServer;
};

#endif