/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    .cpp
*@Description：
*@Author:			jin.shaohua
*@Date：	    2013.4
*@History:
******************************************************************************************/
#include "Helper/mdbDateTime.h"
#include "Control/mdbAutoUnlockThread.h"
#include <errno.h>
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{
    TMdbAutoUnlockThread::TMdbAutoUnlockThread()
    {
        m_bIsRun  = false;
        m_pShmDSN = NULL;
        m_pDsn    = NULL;
    }


    TMdbAutoUnlockThread::~TMdbAutoUnlockThread()
    {

    }

    //初始化
    void TMdbAutoUnlockThread::Init(TMdbShmDSN* pShmDSN)
    {
        m_pShmDSN = pShmDSN;
        m_pDsn = m_pShmDSN->GetInfo();
    }

    void* TMdbAutoUnlockThread::agent(void* p)
    {
        TMdbAutoUnlockThread* pThread = (TMdbAutoUnlockThread*)p;
        pThread->svc();
        return 0;
    }


    //开始启动
    int TMdbAutoUnlockThread::Start()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        pthread_t       tID;
        pthread_attr_t  thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);

        CHECK_RET(pthread_attr_setstacksize(&thread_attr, 5*1024*1024),"Can't pthread_attr_setstacksize(), errno=[%d], errmsg=[%s].",
                  errno, strerror(errno));
        CHECK_RET(pthread_create(&tID, &thread_attr, agent, this),"Can't pthread_attr_setstacksize(), errno=[%d], errmsg=[%s].",
                  errno, strerror(errno));
        return iRet;
    }

    //开始停止
    void TMdbAutoUnlockThread::Stop()
    {
        m_bRunFlag = false;
    }

    //处理
    int TMdbAutoUnlockThread::svc()
    {
        TADD_FLOW("***************Start AutoUnlockThread****************");
        int iRet = 0;
        m_bRunFlag = true;
        m_bIsRun = true;
        time(&m_tLastTime); //取得当前时间的time_t值
        while(true)
        {
            //如果有命令需要停止
            if(m_pDsn->cState == DB_stop || m_bRunFlag == false)
            {
                TADD_NORMAL("AutoUnlockThread stop.");
                m_bIsRun = false;
                break;
            }
            ReleaseMutex();//释放超时锁
            TMdbDateTime::MSleep(1000);
        };
        TADD_NORMAL("AutoUnlockThread Exit");
        return iRet;
    }

    int TMdbAutoUnlockThread::DiffMSecond(struct timeval tTV1, struct timeval tTV2)
    {
        if(tTV1.tv_sec == 0 || tTV2.tv_sec == 0)
        {
            return 0;
        }

        int iSec  = tTV1.tv_sec - tTV2.tv_sec;
        int iUSec = tTV1.tv_usec - tTV2.tv_usec;

        int iMSec = iSec*1000 + iUSec/1000;
        return iMSec;
    }


    //释放超时锁
    void TMdbAutoUnlockThread::ReleaseMutex()
    {
        TADD_FUNC("Start.");
        struct timeval tCurTime;                 //当前时间
        gettimeofday(&tCurTime, NULL);
        //DSN管理区锁
        if(0 != CheckAndRelease(m_pDsn->tMutex,tCurTime,5000,true))
        {
             TADD_ERROR(ERROR_UNKNOWN, "Mutex Dsn[%s] is block.", m_pDsn->sName);
        }
        //逐个比较表名称
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(; itor != m_pShmDSN->m_TableList.end(); ++itor)
        {
            TMdbTable* pTable = &(*itor);
            if(pTable->sTableName[0] == 0)
            {
                continue;
            }
            if(0 != CheckAndRelease(pTable->tFreeMutex,tCurTime,20000,true))
            {
                 TADD_ERROR(ERROR_UNKNOWN, "Table[%s] tFreeMutex is block.", pTable->sTableName);
            }
            if(0 != CheckAndRelease(pTable->tFullMutex,tCurTime,20000,true))
            {
                 TADD_ERROR(ERROR_UNKNOWN, "Table[%s] tFullMutex is block.", pTable->sTableName);
            }
            if(0 != CheckAndRelease(pTable->tTableMutex,tCurTime,20000,true))
            {
                 TADD_ERROR(ERROR_UNKNOWN,"Table[%s] tTableMutex is block.", pTable->sTableName);
            }
        }
        //varchar 管理区锁
        /*
        TvarcharBlock * pVarcharBlock = (TvarcharBlock *)m_pShmDSN->GetVarcharMgrAddr();
        if(0 != CheckAndRelease(pVarcharBlock->tVarCharMutex,tCurTime,5000))
        {
                TADD_ERROR(ERROR_UNKNOWN,"tVarCharMutex is block.");
        }
        */

        TShmList<TMdbVarchar>::iterator itorVar = m_pShmDSN->m_VarCharList.begin();
        for(; itorVar != m_pShmDSN->m_VarCharList.end(); ++itorVar)
        {
            TMdbVarchar* pVarchar = &(*itorVar);
            if(0 != CheckAndRelease(pVarchar->tMutex,tCurTime,5000,true))
            {
                 TADD_ERROR(ERROR_UNKNOWN, "TMdbVarchar[%d] tMutex is block.", pVarchar->iVarcharID);
            }
        }
        
        //扫描页锁
        TMutex* pMutex = (TMutex*)(m_pShmDSN->GetPageMutexAddr());
        for(int i=0; i<MAX_MUTEX_COUNTS; ++i)
        {
            if(0 != CheckAndRelease(*pMutex,tCurTime,5000,true))
            {
                    TADD_ERROR(ERROR_UNKNOWN,"Page Mutex[%d] block", i);
            }
            ++pMutex;
        }
		//扫描varchar存储区页锁
	pMutex = (TMutex*)(m_pShmDSN->GetVarcharPageMutexAddr());
        for(int i=0; i<MAX_VARCHAR_MUTEX_COUNTS; ++i)
        {
            if(0 != CheckAndRelease(*pMutex,tCurTime,5000,true))
            {
                    TADD_ERROR(ERROR_UNKNOWN,"Varchar Page Mutex[%d] block", i);
            }
            ++pMutex;
        }
        //同步共享内存的Pop/Push锁的监控，进行自动解锁
        TMdbMemQueue * pMemQueue = (TMdbMemQueue*)m_pShmDSN->GetSyncAreaShm();
        if(pMemQueue != NULL)
        {
            if(0 != CheckAndRelease(pMemQueue->tPopMutex,tCurTime,5000,true))
            {
                    TADD_ERROR(ERROR_UNKNOWN, "[%s] PopMutex block",m_pDsn->m_arrSyncArea.m_sName);
            }
            if(0 != CheckAndRelease(pMemQueue->tPushMutex,tCurTime,5000,true))
            {
                    TADD_ERROR(ERROR_UNKNOWN,"[%s] tPushMutex block",m_pDsn->m_arrSyncArea.m_sName);
            }

        }   
        TADD_FUNC(" Finish.");
    }
    /******************************************************************************
    * 函数名称	:  CheckAndRelease
    * 函数描述	:  检测并释放锁
    * 输入		:  iTimeOutMS - 超时毫秒数
    * 输出		:  
    * 返回值	:  
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbAutoUnlockThread::CheckAndRelease(TMutex & tMutex,struct timeval tNow, int iTimeOutMS,bool bRelaseNow)
    {
        int iRet = 0;
        if(DiffMSecond(tNow, tMutex.m_tCurTime) >= iTimeOutMS)
        {
            if(tMutex.GetLockPID() <=0 || false == TMdbNtcSysUtils::IsProcessExist(tMutex.GetLockPID()))
            {
                TADD_ERROR(ERROR_UNKNOWN, "Proc[%d] is not exist,relase mutex",tMutex.GetLockPID());
                CHECK_RET(tMutex.UnLock(true),"UnLock Faild");
            }
            else if(bRelaseNow == true)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Proc[%d] exist,timeout is [%d]ms,relase mutex",tMutex.GetLockPID(),iTimeOutMS);
                CHECK_RET(tMutex.UnLock(true),"UnLock Faild");
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN, "Proc[%d] exist,but hold the mutex to looong,timeout is [%d]ms",tMutex.GetLockPID(),iTimeOutMS);
				return ERR_APP_INVALID_PARAM;
            }
            
        }
        return iRet;
    }
//}


