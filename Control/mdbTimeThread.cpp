/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbTimeThread.h
*@Description： 负责管理时间信息
*@Author:		li.shugang
*@Date：	    2009年06月12日
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


    //开始启动
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

    //开始停止
    void TMdbTimeThread::Stop()
    {
        m_bRunFlag = false;
    }
    //检测剩余磁盘空间
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
        
        //检查磁盘空间是否小于32M
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
            ullDiskSpace /= 1024;//billingSDK 中linux单位是字节
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
    * 函数名称	:  CollectTableOperInfo
    * 函数描述	:  获取表的操作信息
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
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
            //pMdbTable->tTableMutex.Lock(true,&(m_pDsn->tCurTime));//加锁
            
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
    * 函数名称  :  PrintAllSeqCurValue
    * 函数描述  :  打印所有序列的详细信息
    * 输入		:  
    * 输出		:  
    * 返回值    :  无
    * 作者		:  cao.peng
    *******************************************************************************/
    void TMdbTimeThread::PrintAllSeqCurValue()
    {    
        TMemSeq *pTmpSeq = NULL;
        TShmList<TMemSeq>::iterator itor = m_pShmDSN->m_MemSeqList.begin();
        for(; itor != m_pShmDSN->m_MemSeqList.end(); ++itor)
        {
            pTmpSeq = &(*itor);
            //如果没有序列
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
            //如果有命令需要停止
            if(m_pDsn->cState == DB_stop || m_bRunFlag==false)
            {
                TADD_FLOW("TMdbTimeThread=[%d] Catch Stop Signal,begin to stop....",pthread_self());
                m_bIsRun = false;
                break;
            }
            //设置当前时间
            TMdbDateTime::GetCurrentTimeStr(m_pDsn->sCurTime);
            gettimeofday(&m_pDsn->tCurTime, NULL);
            TADD_DETAIL("m_pDsn->sCurTime=[%s].", m_pDsn->sCurTime);
            
            //每10秒检测一次
            long iSec = TMdbDateTime::GetDiffSeconds(m_pDsn->sCurTime, sOldTime);
            if(iSec >= 10 || iSec <= -10)
            {
                TMdbDateTime::GetCurrentTimeStr(sOldTime);
                CheckDiskSpace();//检测磁盘空间
                CollectTableOperInfo();//采集表操作信息
            }
            //每10分钟打印一下序列信息
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


