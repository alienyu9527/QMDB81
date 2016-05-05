/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    .cpp
*@Description��
*@Author:			jin.shaohua
*@Date��	    2013.4
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

    //��ʼ��
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


    //��ʼ����
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

    //��ʼֹͣ
    void TMdbAutoUnlockThread::Stop()
    {
        m_bRunFlag = false;
    }

    //����
    int TMdbAutoUnlockThread::svc()
    {
        TADD_FLOW("***************Start AutoUnlockThread****************");
        int iRet = 0;
        m_bRunFlag = true;
        m_bIsRun = true;
        time(&m_tLastTime); //ȡ�õ�ǰʱ���time_tֵ
        while(true)
        {
            //�����������Ҫֹͣ
            if(m_pDsn->cState == DB_stop || m_bRunFlag == false)
            {
                TADD_NORMAL("AutoUnlockThread stop.");
                m_bIsRun = false;
                break;
            }
            ReleaseMutex();//�ͷų�ʱ��
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


    //�ͷų�ʱ��
    void TMdbAutoUnlockThread::ReleaseMutex()
    {
        TADD_FUNC("Start.");
        struct timeval tCurTime;                 //��ǰʱ��
        gettimeofday(&tCurTime, NULL);
        //DSN��������
        if(0 != CheckAndRelease(m_pDsn->tMutex,tCurTime,5000,true))
        {
             TADD_ERROR(ERROR_UNKNOWN, "Mutex Dsn[%s] is block.", m_pDsn->sName);
        }
        //����Ƚϱ�����
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
        //varchar ��������
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
        
        //ɨ��ҳ��
        TMutex* pMutex = (TMutex*)(m_pShmDSN->GetPageMutexAddr());
        for(int i=0; i<MAX_MUTEX_COUNTS; ++i)
        {
            if(0 != CheckAndRelease(*pMutex,tCurTime,5000,true))
            {
                    TADD_ERROR(ERROR_UNKNOWN,"Page Mutex[%d] block", i);
            }
            ++pMutex;
        }
		//ɨ��varchar�洢��ҳ��
	pMutex = (TMutex*)(m_pShmDSN->GetVarcharPageMutexAddr());
        for(int i=0; i<MAX_VARCHAR_MUTEX_COUNTS; ++i)
        {
            if(0 != CheckAndRelease(*pMutex,tCurTime,5000,true))
            {
                    TADD_ERROR(ERROR_UNKNOWN,"Varchar Page Mutex[%d] block", i);
            }
            ++pMutex;
        }
        //ͬ�������ڴ��Pop/Push���ļ�أ������Զ�����
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
    * ��������	:  CheckAndRelease
    * ��������	:  ��Ⲣ�ͷ���
    * ����		:  iTimeOutMS - ��ʱ������
    * ���		:  
    * ����ֵ	:  
    * ����		:  jin.shaohua
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


