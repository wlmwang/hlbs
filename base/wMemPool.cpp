
/**
 * Copyright (C) Anny.
 * Copyright (C) Disvr, Inc.
 */

#include "wMemPool.h"

wMemPool::wMemPool()
{
	Initialize();
}

wMemPool::~wMemPool() 
{
	Destroy();
}

void wMemPool::Initialize()
{
	*mStart = NULL;
	*mLast = NULL;
	*mEnd = NULL;
	mSize = 0;
	mExtra = NULL;
}

char *wMemPool::Create(size_t size)
{
    mStart = memalign(POOL_ALIGNMENT, size);	//ҳ����
    if(mStart == NULL)
	{
        return NULL;
    }
	
	mSize = size;
    mLast = mStart;
    mEnd  = mStart + size;
	
    return mStart;
}

char *wMemPool::Alloc(size_t size)
{
	char *m = NULL;
	if(m + size <= mEnd)
	{
		m = ALIGN_PTR(mLast, ALIGNMENT);
		mLast = m + size;
	}
	else
	{
		struct extra_t *l = memalign(POOL_ALIGNMENT, sizeof(struct extra_t));
		m = memalign(POOL_ALIGNMENT, size);
		
		l->mAddr = m;
		l->mNext = mExtra;
		mExtra = l;
	}
	return m;
}


void wMemPool::Destroy()
{
    for (struct extra_t* l = mExtra; l; l = l->mNext) 
	{
        if (l->mAddr) 
		{
            free(l->mAddr);
        }
    }
	
	if(mStart)
	{
		free(mStart);
	}
}

void wMemPool::Reset()
{
	mLast = mStart;
    
	for (struct extra_t* l = mExtra; l; l = l->mNext) 
	{
        if (l->mAddr) 
		{
            free(l->mAddr);
        }
    }
}