/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbMutexCtrl.cpp
*@Description： 内存数据库的锁刷新程序
*@Author:		jiang.mingjun
*@Date：	    2010年05月04日
*@History:
******************************************************************************************/
#include "Tools/mdbMutexCtrl.h"
#include "Helper/mdbShm.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbOS.h"
#include "Helper/TThreadLog.h"
#include "Control/mdbMgrShm.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//namespace QuickMDB{



#ifdef WIN32
#pragma warning(disable: 4305)
#endif

    TMdbMutexCtrl::TMdbMutexCtrl()
    {
        m_pShmDsn = NULL;
    }


    TMdbMutexCtrl::~TMdbMutexCtrl()
    {

    }


    //初始化
    int TMdbMutexCtrl::Init(const char* pszDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pszDsn);
        m_pShmDsn = TMdbShmMgr::GetShmDSN(pszDsn);
        CHECK_OBJ(m_pShmDsn);
        return iRet;
    }

    int TMdbMutexCtrl::ReNewAllMutex()
    {
        int iRet = 0;
        CHECK_RET(RenewDsnMutex(),"RenewDsnMutex failed.");
        CHECK_RET(RenewSeqMutex(),"RenewSeqMutex failed.");
        CHECK_RET(RenewDataMutex(),"RenewDataMutex failed.");
        CHECK_RET(RenewTableMutex(),"RenewTableMutex failed.");
        CHECK_RET(RenewPageMutex(),"RenewPageMutex failed.");
        CHECK_RET(RenewTablespaceMutex(),"RenewTablespaceMutex failed.");
        int i = 0;
        for(i = 0;i < SA_MAX;++i)
        {
            CHECK_RET(RenewSyncAreaMutex(i),"RenewSyncAreaMutex failed.");
        }
    
    return iRet;
}

int TMdbMutexCtrl::RenewOneMutex(TMutex & tMutex)
{
    int iRet = 0;
    do{
    CHECK_RET_BREAK(tMutex.TryLock(),"TryLock failed.errno=[%d],errmsg=[%s]",errno,strerror(errno));
    }while(0);
    do{
    CHECK_RET_BREAK(tMutex.UnLock(true),"UnLock failed.errno=[%d],errmsg=[%s]",errno,strerror(errno));
    }while(0);
    return iRet;
}

int TMdbMutexCtrl::RenewDsnMutex()
{
    int iRet = 0;
    CHECK_OBJ(m_pShmDsn);
    CHECK_RET(RenewOneMutex(m_pShmDsn->GetInfo()->tMutex),"RenewOneMutex failed.");
    TADD_NORMAL("Renew dsn[%s] mutex ok",m_pShmDsn->GetInfo()->sName);
    return iRet;
}
int TMdbMutexCtrl::RenewDataMutex()
{
    int iRet = 0;
    CHECK_OBJ(m_pShmDsn);
    int i = 0;
    for(i = 0;i < m_pShmDsn->GetInfo()->iShmCounts;++i)
    {
        TMdbShmHead *pHead = (TMdbShmHead*)m_pShmDsn->GetDataShm(i);
        if(NULL != pHead)
        {
            CHECK_RET(RenewOneMutex(pHead->tMutex),"RenewOneMutex failed.");
            TADD_NORMAL("Renew DataShm[%d] mutex ok",i);
        }
    }
    return iRet;
}

int TMdbMutexCtrl::RenewSeqMutex(const char * sSeqname)
{
    int iRet = 0;
    CHECK_OBJ(m_pShmDsn);
    TShmList<TMemSeq>::iterator itor = m_pShmDsn->m_MemSeqList.begin();
    for(;itor != m_pShmDsn->m_MemSeqList.end();++itor)
    {
        if(NULL == sSeqname || 0 == sSeqname[0] || 
            TMdbNtcStrFunc::StrNoCaseCmp((*itor).sSeqName,sSeqname) == 0)
        {
            CHECK_RET(RenewOneMutex(itor->tMutex),"RenewOneMutex failed.");
            TADD_NORMAL("Renew Seq[%s] mutex ok",(*itor).sSeqName);
        }
    }
    return iRet;
}

int TMdbMutexCtrl::RenewTableMutex(const char * sTablename)
{
    int iRet = 0;
    //初始化表信息块
    CHECK_OBJ(m_pShmDsn);
    TShmList<TMdbTable>::iterator itor = m_pShmDsn->m_TableList.begin();
    for(;itor != m_pShmDsn->m_TableList.end();++itor)
    {
        TMdbTable *pTable = &(*itor);
         if(pTable != NULL && pTable->sTableName[0] != 0 )
        { 
            if(NULL == sTablename || 0 == sTablename[0] || 
            TMdbNtcStrFunc::StrNoCaseCmp((*itor).sTableName,sTablename) == 0)
            {
                CHECK_RET(RenewOneMutex(pTable->tFreeMutex),"RenewOneMutex failed.");
                CHECK_RET(RenewOneMutex(pTable->tFullMutex),"RenewOneMutex failed.");
                CHECK_RET(RenewOneMutex(pTable->tTableMutex),"RenewOneMutex failed.");
                TADD_NORMAL("Renew Table[%s] mutex ok",(*itor).sTableName);
            }
        }
    }
    return iRet;
}

int TMdbMutexCtrl::RenewIndexMutex()
{
    int iRet = 0;
    CHECK_OBJ(m_pShmDsn);
    int i = 0;
    for(i = 0;i< m_pShmDsn->GetInfo()->iBaseIndexShmCounts;++i)
    {
        TMdbBaseIndexMgrInfo* pInfo = (TMdbBaseIndexMgrInfo*)m_pShmDsn->GetBaseIndex(i);
        CHECK_RET(RenewOneMutex(pInfo->tMutex),"RenewOneMutex failed.");
        TADD_NORMAL("Renew BaseIndex[%d] mutex ok",i);
    }
    for(i = 0;i< m_pShmDsn->GetInfo()->iConflictIndexShmCounts;++i)
    {
        TMdbConflictIndexMgrInfo* pInfo = (TMdbConflictIndexMgrInfo*)m_pShmDsn->GetConflictIndex(i);
        CHECK_RET(RenewOneMutex(pInfo->tMutex),"RenewOneMutex failed.");
        TADD_NORMAL("Renew ConflictIndex[%d] mutex ok",i);
    }
    return iRet;
}

int TMdbMutexCtrl::RenewPageMutex()
{
    int iRet = 0;
    CHECK_OBJ(m_pShmDsn);
    TMutex* pMutex = (TMutex*)m_pShmDsn->GetPageMutexAddr();
    for(int i=0; i<MAX_MUTEX_COUNTS; ++i)
    {
        if(pMutex != NULL)
        {
            CHECK_RET(RenewOneMutex(*pMutex),"RenewOneMutex failed.");
            TADD_NORMAL("Renew Page[%d] mutex ok",i);
        }
        ++pMutex;
    }
    return iRet;
}
//恢复表空间锁
int TMdbMutexCtrl::RenewTablespaceMutex(const char * sTSname)
{
    int iRet = 0;
    CHECK_OBJ(m_pShmDsn);
    TShmList<TMdbTableSpace>::iterator itor = m_pShmDsn->m_TSList.begin();
    for(;itor != m_pShmDsn->m_TSList.end();++itor)
    {
        TMdbTableSpace *pTableSpace = &(*itor);
        if(pTableSpace->sName[0] != '\0')
        {
            if(NULL == sTSname || 0 == sTSname[0] || 
            TMdbNtcStrFunc::StrNoCaseCmp((*itor).sName,sTSname) == 0)
            {
                 CHECK_RET(RenewOneMutex(pTableSpace->tEmptyMutex),"RenewOneMutex(pTableSpace->tEmptyMutex) failed.");
                 //CHECK_RET(RenewOneMutex(pTableSpace->m_tFreeMutex),"RenewOneMutex(pTableSpace->m_tFreeMutex)falied.");
                 //CHECK_RET(RenewOneMutex(pTableSpace->m_tFullMutex),"RenewOneMutex(pTableSpace->m_tFullMutex)falied.");
                 TADD_NORMAL("Renew Tablespace[%s] mutex ok",pTableSpace->sName);
            }
        }
    }
    return iRet;
}

int TMdbMutexCtrl::RenewSyncAreaMutex(int iType)
{
     int iRet = 0;
    CHECK_OBJ(m_pShmDsn);
    TMdbMemQueue* pQueue = (TMdbMemQueue*)m_pShmDsn->GetSyncAreaShm();
    if(NULL != pQueue )
    {
        CHECK_RET(RenewOneMutex(pQueue->tPushMutex),"RenewOneMutex failed.");
        CHECK_RET(RenewOneMutex(pQueue->tPopMutex),"RenewOneMutex failed.");
        TADD_NORMAL("Renew [%s] mutex ok",m_pShmDsn->GetInfo()->m_arrSyncArea.m_sName);
    }
    return iRet;
}


int TMdbMutexCtrl::RenewShardBakBufAreaMutex(int iType)
{
    int iRet = 0;
    CHECK_OBJ(m_pShmDsn);
	for(int i = 0; i<MAX_SHM_ID; i++)
	{
		TMdbOnlineRepMemQueue* pQueue = (TMdbOnlineRepMemQueue*)m_pShmDsn->GetShardBakBufAreaShm(i);
	    if(NULL != pQueue )
	    {
	        CHECK_RET(RenewOneMutex(pQueue->tPushMutex),"RenewOneMutex failed.");
	        CHECK_RET(RenewOneMutex(pQueue->tPopMutex),"RenewOneMutex failed.");
	        TADD_NORMAL("Renew [%s] mutex ok",m_pShmDsn->GetInfo()->m_arrShardBakBufArea.m_sName);
	    }
	}
    
    return iRet;
}

//}
