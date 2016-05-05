/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbTimeThread.h
*@Description�� �������ʱ����Ϣ
*@Author:		li.shugang
*@Date��	    2009��06��12��
*@History:      jiang.mingjun
******************************************************************************************/
#include "Helper/mdbDateTime.h"
#include "Control/mdbTimeThread.h"
#include <errno.h>
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{

    TMdbTimeThread::TMdbTimeThread()
    {
        m_bIsRun  = false;
        m_pShmDSN = NULL;
        m_pDsn    = NULL;
    }
    TMdbTimeThread::~TMdbTimeThread()
    {

    }
    void TMdbTimeThread::Init(TMdbShmDSN* pShmDSN)
    {
        m_pShmDSN = pShmDSN;
        m_pDsn = m_pShmDSN->GetInfo();
        return;
    }
    void* TMdbTimeThread::agent(void* p)
    {
        TMdbTimeThread* pThread = (TMdbTimeThread*)p;
        pThread->svc();
        return 0;
    }


    //��ʼ����
    int TMdbTimeThread::Start()
    {
        pthread_t       tID;
        pthread_attr_t  thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);
        int iRet = pthread_attr_setstacksize(&thread_attr, 5*1024*1024);
        if(iRet != 0)
        {
            TADD_ERROR(ERR_OS_SET_THREAD_ATTR,"Can't pthread_attr_setstacksize(), errno=[%d], errmsg=[%s].", errno, strerror(errno));
            return ERR_OS_SET_THREAD_ATTR;
        }
        if(0 != pthread_create(&tID, &thread_attr, agent, this))
        {
            TADD_ERROR(ERR_OS_CREATE_THREAD,"Can't pthread_create(), errno=[%d], errmsg=[%s].",errno, strerror(errno));
            return ERR_OS_CREATE_THREAD;
        }
        return 0;
    }

    //��ʼֹͣ
    void TMdbTimeThread::Stop()
    {
        m_bRunFlag = false;
    }
    //���ʣ����̿ռ�
    int TMdbTimeThread::CheckDiskSpace()
    {
        char * sTempDis = NULL;
        CHECK_OBJ(m_pDsn);
        sTempDis = m_pDsn->m_arrSyncArea.m_sDir[0];
#if 0
        int i = 0;
        for(i = 0;i < SA_MAX; ++i)
        {
            if(m_pDsn->m_arrSyncArea[i].m_iShmID > 0)
            {
                sTempDis = m_pDsn->m_arrSyncArea[i].m_sDir;
            }
        }
       
        if(m_pDsn->iOraShmID != -1)
        {
            sTempDis = m_pDsn->sLogDir;
        }
        else if(m_pDsn->iRepShmID != -1)
        {
            sTempDis = m_pDsn->sRepDir;
        }
#endif
        
        //�����̿ռ��Ƿ�С��32M
        if(NULL != sTempDis)
        {
            //unsigned long long ullDiskSpace = TMdbNtcDirOper::GetDiskFreeSpace(sTempDis, ullDiskSpace);
            
            if(TMdbNtcDirOper::IsExist(sTempDis) == false)
            {
                char sFileName[MAX_PATH_NAME_LEN] = {0};
                SAFESTRCPY(sFileName, sizeof(sFileName), sTempDis);
                
                if(sFileName[strlen(sFileName)-1] != '/')
                {
                    sFileName[strlen(sFileName)] = '/';
                }
                
                if(TMdbNtcDirOper::MakeFullDir(sFileName) == false)
                {
                    TADD_ERROR(ERR_OS_CREATE_DIR,"Can't create dir=[%s].", sFileName);
                    return ERR_OS_CREATE_DIR;
                }
            }
    
            unsigned long long ullDiskSpace = 0;
            if(false == TMdbNtcDirOper::GetDiskFreeSpace(sTempDis, ullDiskSpace))  
            {
                TADD_WARNING("GetDiskFreeSpace failed,dir=[%s].\n", sTempDis);
                return 0;
            }
            #ifndef WIN32
            ullDiskSpace /= 1024;//billingSDK ��linux��λ���ֽ�
            #endif
            if(ullDiskSpace <=  MIN_DISK_SPACE_KB )  //32M
            {
                m_pDsn->bDiskSpaceStat = false;
                TADD_WARNING("DiskSpaceLeft=[%llu]KB < [%llu]KB, RepDir[%s] don't have space for flush data."\
					,ullDiskSpace, MIN_DISK_SPACE_KB, sTempDis);
            }
            else
            {
                m_pDsn->bDiskSpaceStat = true;
            }
        }
        return 0;
    }

    /******************************************************************************
    * ��������	:  CollectTableOperInfo
    * ��������	:  ��ȡ��Ĳ�����Ϣ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbTimeThread::CollectTableOperInfo()
    {    
        int arrOperCount[8] = {0};
        const char * sOperName[] = {"Query","Insert","Update","Delete",
                                            "QueryFail","InsertFail","UpdateFail","DeleteFail"};
        TShmList<TMdbTable >& tTableList = m_pShmDSN->m_TableList;
        TShmList<TMdbTable >::iterator itor = tTableList.begin();
        for(;itor != tTableList.end();++itor)
        {
            TMdbTable * pMdbTable  = &(*itor);
            if(pMdbTable->sTableName[0]== 0 || pMdbTable->bIsSysTab)
            {
                continue;
            }
            //pMdbTable->tTableMutex.Lock(true,&(m_pDsn->tCurTime));//����
            
            arrOperCount[0] = pMdbTable->iQueryCounts;
            arrOperCount[1] = pMdbTable->iInsertCounts;
            arrOperCount[2] = pMdbTable->iUpdateCounts;
            arrOperCount[3] = pMdbTable->iDeleteCounts;
            arrOperCount[4] = pMdbTable->iQueryFailCounts;
            arrOperCount[5] = pMdbTable->iInsertFailCounts;
            arrOperCount[6] = pMdbTable->iUpdateFailCounts;
            arrOperCount[7] = pMdbTable->iDeleteFailCounts;
            
            pMdbTable->iQueryCounts  = 0;
            pMdbTable->iInsertCounts  = 0;
            pMdbTable->iUpdateCounts = 0;
            pMdbTable->iDeleteCounts  = 0;
            pMdbTable->iQueryFailCounts = 0;
            pMdbTable->iInsertFailCounts = 0;
            pMdbTable->iUpdateFailCounts= 0;
            pMdbTable->iDeleteFailCounts = 0;
            //pMdbTable->tTableMutex.UnLock(true);
            int j = 0;
            for(j = 0;j < 8;++j)
            {
                if(arrOperCount[j] != 0)
                {
                    TADD_NORMAL("Table[%-24s]:%12s=[%-8d]",pMdbTable->sTableName,sOperName[j],arrOperCount[j]);
                }
            }
        }
        return 0;
    }

    /******************************************************************************
    * ��������  :  PrintAllSeqCurValue
    * ��������  :  ��ӡ�������е���ϸ��Ϣ
    * ����		:  
    * ���		:  
    * ����ֵ    :  ��
    * ����		:  cao.peng
    *******************************************************************************/
    void TMdbTimeThread::PrintAllSeqCurValue()
    {    
        TMemSeq *pTmpSeq = NULL;
        TShmList<TMemSeq>::iterator itor = m_pShmDSN->m_MemSeqList.begin();
        for(; itor != m_pShmDSN->m_MemSeqList.end(); ++itor)
        {
            pTmpSeq = &(*itor);
            //���û������
            if(pTmpSeq->sSeqName[0] == 0)
            {
                continue;
            }
            TADD_NORMAL("==============Sequence-Info=================");
            TADD_NORMAL("    sSeqName = %s.", pTmpSeq->sSeqName);
            TADD_NORMAL("    iStart   = %ld.", pTmpSeq->iStart);
            TADD_NORMAL("    iEnd     = %ld.", pTmpSeq->iEnd);
            TADD_NORMAL("    iCur     = %ld.", pTmpSeq->iCur);
            TADD_NORMAL("    iStep    = %ld.", pTmpSeq->iStep);    
            TADD_NORMAL("==============Sequence-Info=================");
        }
    }

    int TMdbTimeThread::svc()
    {
        TADD_FLOW("*************QmdbTimeThread start***************");
        m_bRunFlag = true;
        m_bIsRun = true;
        char sOldTime[MAX_TIME_LEN];
        char sSeqPrintLastTime[MAX_TIME_LEN] = {0};
        memset(sOldTime, 0, sizeof(sOldTime));
        TMdbDateTime::GetCurrentTimeStr(sOldTime);
        TMdbDateTime::GetCurrentTimeStr(sSeqPrintLastTime);
        while(true)
        {
            //�����������Ҫֹͣ
            if(m_pDsn->cState == DB_stop || m_bRunFlag==false)
            {
                TADD_FLOW("TMdbTimeThread=[%d] Catch Stop Signal,begin to stop....",pthread_self());
                m_bIsRun = false;
                break;
            }
            //���õ�ǰʱ��
            TMdbDateTime::GetCurrentTimeStr(m_pDsn->sCurTime);
            gettimeofday(&m_pDsn->tCurTime, NULL);
            TADD_DETAIL("m_pDsn->sCurTime=[%s].", m_pDsn->sCurTime);
            
            //ÿ10����һ��
            long iSec = TMdbDateTime::GetDiffSeconds(m_pDsn->sCurTime, sOldTime);
            if(iSec >= 10 || iSec <= -10)
            {
                TMdbDateTime::GetCurrentTimeStr(sOldTime);
                CheckDiskSpace();//�����̿ռ�
                CollectTableOperInfo();//�ɼ��������Ϣ
            }
            //ÿ10���Ӵ�ӡһ��������Ϣ
            iSec = TMdbDateTime::GetDiffSeconds(m_pDsn->sCurTime,sSeqPrintLastTime);
            if(iSec >= 600)
            {
                PrintAllSeqCurValue();
                TMdbDateTime::GetCurrentTimeStr(sSeqPrintLastTime);
            }
            TMdbDateTime::MSleep(20);
        }
        TADD_NORMAL("TMdbTimeThread Exit");
        return 0;
    }
//}


