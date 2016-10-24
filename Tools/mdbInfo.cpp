/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbInfo.h
*@Description： 负责打印内存数据库的各种信息
*@Author:       li.shugang
*@Modify:       jiang.mingjun
*@Date：        2009年03月10日
*@History:      简化输出信息
******************************************************************************************/
#include "Tools/mdbInfo.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbIndexCtrl.h"
#include "Control/mdbLinkCtrl.h"
#include "Tools/mdbChangeLog.h"
#include "Helper/mdbDateTime.h"
#include "Control/mdbStorageEngine.h"
#include "Control/mdbVarcharMgr.h"
#include "Control/mdbRepCommon.h"

//namespace QuickMDB{


#define _TRY_CATCH_BEGIN_ try{

#define _TRY_CATCH_END_ }\
    catch(TMdbException& e)\
    {\
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());\
        iRet = ERROR_UNKNOWN;\
    }\
    catch(...)\
    {\
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");\
        iRet = ERROR_UNKNOWN;\
    }

#define PRINT_SYNC_LOCK_INFO(_SYNC_TYPE,LOCK_TYPE,MUTEX)\
    if(MUTEX.IsLock())\
    {\
        TMdbDateTime::TimevalToTimeStr(MUTEX.m_tCurTime,sCreateTime,false);\
        printf("    [%s]:%s lock is busy,the creation time is [%s],the Locked process-id is %d\n",\
        _SYNC_TYPE,LOCK_TYPE,sCreateTime,MUTEX.GetLockPID());\
    }\
    else\
    {\
        printf("    [%s]:%s lock is idle.\n",_SYNC_TYPE,LOCK_TYPE);\
    }\

    TMdbInfo::TMdbInfo()
    {
        m_pConfig = NULL;
        m_pShmDSN = NULL;
        m_pDsn    = NULL;
        m_pMdbTable = NULL;
        m_pMdbTableSpace = NULL;
        m_pMdbProc       = NULL;
        m_pMdbLocalLink  = NULL;
        //m_pMdbRemoteLink = NULL;
    }

    TMdbInfo::TMdbInfo(bool bDetail)
    {
        m_pConfig = NULL;
        m_pShmDSN = NULL;
        m_pDsn    = NULL;
        m_pMdbTable = NULL;
        m_pMdbTableSpace = NULL;
        m_pMdbProc       = NULL;
        m_pMdbLocalLink  = NULL;
        //m_pMdbRemoteLink = NULL;
        m_bMore = bDetail;
    }


    TMdbInfo::~TMdbInfo()
    {
        if(m_pShmDSN!= NULL)
        {
#ifndef WIN32
            //m_pShmDSN->Detach();
            delete m_pShmDSN;
#endif
            m_pShmDSN = NULL;
        }
    }


    /******************************************************************************
    * 函数名称  :  Connect()
    * 函数描述  :  链接某个DSN，但是不在管理区注册任何信息
    * 输入      :  pszDSN, 锁管理区所属的DSN
    * 输出      :  无
    * 返回值    :  成功返回0, 失败返回-1
    * 作者      :  li.shugang
    *******************************************************************************/
    int TMdbInfo::Connect(const char* pszDSN)
    {
        if(pszDSN == NULL)
        {
            return -1;
        }
        int iRet = 0;
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        CHECK_OBJ(m_pConfig);
        if (NULL == m_pShmDSN)
        {
            m_pShmDSN = new(std::nothrow) TMdbShmDSN();
            if(m_pShmDSN == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Mem Not Enough");
                return -1;
            }
        }
        //连接上共享内存
        CHECK_RET(m_pShmDSN->Attach(pszDSN, *m_pConfig),"Attach [%s] failed.",pszDSN);
        m_pDsn = m_pShmDSN->GetInfo();
        CHECK_OBJ(m_pDsn);
		m_tMgrShmAlloc.AttachByKey(m_pShmDSN->GetMgrKey(),m_pMgrAddr);
        return iRet;
    }


    /******************************************************************************
    * 函数名称  :  PrintLink()
    * 函数描述  :  打印链接信息
    * 输入      :  iFlag, 1-打印本地链接, 2-打印远程链接, 0-打印所有链接, lTid指定某一个线程
    * 输出      :  无
    * 返回值    :  无
    * 作者      :  li.shugang
    *******************************************************************************/
    void TMdbInfo::PrintLink(int iFlag, int iPid)
    {
        if(m_pDsn == NULL)
            return;
        if(iFlag == 0 || iFlag == 1)
        {
            TShmList<TMdbLocalLink>::iterator itor = m_pShmDSN->m_LocalLinkList.begin();
            for(; itor != m_pShmDSN->m_LocalLinkList.end(); ++itor)
            {
                TMdbLocalLink *pLocalLink = &(*itor);
				
                if(pLocalLink->iPID >= 0)
                {
                    pLocalLink->Show(m_bMore);
					
					if(iPid && iPid == pLocalLink->iPID)
					{				
						pLocalLink->ShowIndexInfo();
					}
                }
            }
        }

        if(iFlag == 0 || iFlag == 2)
        {
            TShmList<TMdbRemoteLink>::iterator itor = m_pShmDSN->m_RemoteLinkList.begin();
            for(; itor != m_pShmDSN->m_RemoteLinkList.end(); ++itor)
            {
                TMdbRemoteLink *pRemoteLink = &(*itor);
                if(pRemoteLink->sIP[0] != 0)
                {
                    pRemoteLink->Show(m_bMore);
                }
            }
        }
        TShmList<TMdbRepLink>::iterator itor = m_pShmDSN->m_RepLinkList.begin();
        for(; itor != m_pShmDSN->m_RepLinkList.end(); ++itor)
        {
            TMdbRepLink *pRepLink = &(*itor);
            if(pRepLink->iPID >= 0)
            {
                pRepLink->Show();
            }
        }
        printf("Note:\n");
        printf("State:(L-USE, D-DOWN)\n");
    }


    void TMdbInfo::PrintDSN(const char* pszDSN)
    {
        if(m_pDsn == NULL)
            return;

        if(TMdbNtcStrFunc::StrNoCaseCmp(pszDSN, m_pDsn->sName) == 0)
        {
            m_pDsn->Show(m_bMore);
            printf("MgrFreeLeft=[%zd]MB",m_pShmDSN->GetUsedSize()/(1024*1024));
        }
        if(m_bMore == false)
        {
            printf("Note:\n");
            printf("RepAttr: (0-DSN_No_Rep,1-DSN_Ora_Rep,2-DSN_Rep,3-DSN_Two_Rep)\n");
            printf("State:   (0-DB_unused, U-DB_running, L-DB_loading, R-DB_repping, S-DB_stop)\n");
            printf("AccAttr: (0-not complications,1-complications)\n");
            TMdbChangeLog tChangeLog;
            tChangeLog.Print();
        }
    }

    void TMdbInfo::PrintSeq(const char* pszSeq)
    {
        if(m_pDsn == NULL)
            return;
        TShmList<TMemSeq>::iterator itor = m_pShmDSN->m_MemSeqList.begin();
        for(; itor != m_pShmDSN->m_MemSeqList.end(); ++itor)
        {
            TMemSeq *pSeq  = &(*itor);
            if(pSeq->sSeqName[0] != 0)
            {
                if(pszSeq == NULL)
                {
                    pSeq->Show(m_bMore);
                }
                //else if(TMdbStrFunc::StrCmpNoCase(pszSeq, pSeq->sSeqName) == 0)
                else if(TMdbNtcStrFunc::FindString(pSeq->sSeqName,pszSeq) >= 0)//模糊匹配
                {
                    pSeq->Show(m_bMore);
                }
            }
        }
    }

    /******************************************************************************
    * 函数名称  :  PrintProc()
    * 函数描述  :  打印进程信息
    * 输入      :  pszProc, 进程名称
    * 输出      :  无
    * 返回值    :  无
    * 作者      :  li.shugang
    *******************************************************************************/
    void TMdbInfo::PrintProc(const char* pszProc)
    {
        if(m_pDsn == NULL)
            return;
        TShmList<TMdbProc>::iterator itor = m_pShmDSN->m_ProcList.begin();
        for(; itor != m_pShmDSN->m_ProcList.end(); ++itor)
        {
            TMdbProc *pProc  = &(*itor);
            if(pProc->sName[0] != 0)
            {
                if(pszProc == NULL)
                {
                    pProc->Show(m_bMore);
                }
                //else if(TMdbStrFunc::StrCmpNoCase(pszProc, pProc->sName) == 0)
                else if(TMdbNtcStrFunc::FindString(pProc->sName,pszProc) >= 0)
                {
                    pProc->Show(m_bMore);
                    //break;
                }
            }

        }
        printf("Note:\n");
        printf("State:(S-stop, F-free, B-busy, K-kill, D-dump package)\n");
    }


    /******************************************************************************
    * 函数名称  :  PrintMem()
    * 函数描述  :  打印内存块信息
    * 输入      :  pszProc, 进程名称
    * 输出      :  无
    * 返回值    :  无
    * 作者      :  li.shugang
    *******************************************************************************/
    int  TMdbInfo::PrintMem(const char* pszShmID)
    {
        int iRet = 0;
        CHECK_OBJ(m_pDsn);
        //首先是管理区信息--因为前面已经Attach上内存块了，所以下面只需要取出ShmID即可
        long iMgrKey = GET_MGR_KEY(m_pConfig->GetDSN()->llValue);
        SHAMEM_T iShmID = 0;
        if(false == TMdbShm::IsShmExist(iMgrKey,m_pConfig->GetDSN()->iManagerSize, iShmID))
        {
            TADD_ERROR(ERROR_UNKNOWN,"manager share memory is not exist, errno=%d, errmsg=[%s].",errno, strerror(errno));
            return 0;
        }
        printf("\nManager Share Memory:\n\tKey=0x%ld(%ld), Shm-ID=%d(0x%0x)\n\n", iMgrKey, iMgrKey, iShmID, iShmID);
        //基础索引区
        printf("Base Index Share Memory:\n");
        for(int i=0; i<MAX_SHM_ID; ++i)
        {
            if(m_pDsn->iBaseIndexShmID[i] != -1)
            {
                printf("\tKey=0x%08d(%d), Shm-ID=%d(0x%0x)\n", m_pDsn->iBaseIndexShmKey[i], m_pDsn->iBaseIndexShmKey[i], m_pDsn->iBaseIndexShmID[i], m_pDsn->iBaseIndexShmID[i]);
                TMdbBaseIndexMgrInfo* pInfo = (TMdbBaseIndexMgrInfo*)m_pShmDSN->GetBaseIndex(i);
                printf("\t\tiSeq         = %d\n", pInfo->iSeq);
                printf("\t\tTotalSize    = %lld\n",pInfo->iTotalSize);
                printf("\t\tFreeSpace:");
                for(int j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
                {
                    if(pInfo->tFreeSpace[j].iPosAdd == 0)
                    {
                        break;   //结束
                    }
                    printf("(%d)PosAdd=[%zd],Size[%zd] ",j,pInfo->tFreeSpace[j].iPosAdd,pInfo->tFreeSpace[j].iSize);
                }
                printf("\n");
                printf("\t\tiIndexCounts = %d\n", pInfo->iIndexCounts);
                for(int j=0; j<pInfo->iIndexCounts; ++j)
                {
                    if('1' == pInfo->tIndex[j].cState)
                    {
                        printf("\t\t\tIndex-Name = %s\n", pInfo->tIndex[j].sName);
                    }
                }
            }
        }
        //冲突索引区
        printf("\nConflict Index Share Memory:\n");
        for(int i=0; i<MAX_SHM_ID; ++i)
        {
            if(m_pDsn->iConflictIndexShmID[i] != -1)
            {
                printf("\tKey=0x%08x(%d), Shm-ID=%d(0x%0x)\n", m_pDsn->iConflictIndexShmKey[i], m_pDsn->iConflictIndexShmKey[i],
                       m_pDsn->iConflictIndexShmID[i], m_pDsn->iConflictIndexShmID[i]);
                TMdbConflictIndexMgrInfo* pInfo = (TMdbConflictIndexMgrInfo*)m_pShmDSN->GetConflictIndex(i);
                printf("\t\tiSeq         = %d\n", pInfo->iSeq);
                printf("\t\tTotalSize    = %lld\n",pInfo->iTotalSize);
                printf("\t\tFreeSpace:");
                for(int j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
                {
                    if(pInfo->tFreeSpace[j].iPosAdd == 0)
                    {
                        break;   //结束
                    }
                    printf("(%d)PosAdd=[%zd],Size[%zd] ",j,pInfo->tFreeSpace[j].iPosAdd,pInfo->tFreeSpace[j].iSize);
                }
                printf("\n");
                printf("\t\tiIndexCounts = %d\n", pInfo->iIndexCounts);
            }
        }
        printf("\n");
        //数据区
        printf("Data Share Memory:\n");
        for(int i=0; i<MAX_SHM_ID; ++i)
        {
            if(m_pDsn->iShmID[i] != -1)
            {
                printf("\tKey=0x%08llx(%lld), Shm-ID=%d(0x%0x)\n", m_pDsn->iShmKey[i], m_pDsn->iShmKey[i], m_pDsn->iShmID[i], m_pDsn->iShmID[i]);
                TMdbShmHead* pInfo = (TMdbShmHead*)m_pShmDSN->GetDataShm(i);
                printf("\t\tiTotalSize   = %lld\n", (MDB_INT64)pInfo->iTotalSize);
                printf("\t\tiLeftOffSet  = %lld\n", (MDB_INT64)pInfo->iLeftOffSet);
            }
        }
        printf("\n");
        //打印同步区
        TMdbSyncArea & tSA = m_pDsn->m_arrSyncArea;
        if(tSA.m_iShmID > 0)
        {
             printf("[%s] Share Memory:\n\tKey=0x%08llx(%lld), Shm-ID=%d(0x%0x)\n\n",
                tSA.m_sName,tSA.m_iShmKey,tSA.m_iShmKey,tSA.m_iShmID,tSA.m_iShmID);
            TMdbMemQueue* pQueue = (TMdbMemQueue*)m_pShmDSN->GetSyncAreaShm();
            CHECK_OBJ(pQueue);
            printf("        iPushPos = %d \n", pQueue->iPushPos);
            printf("        iPopPos = %d \n",pQueue->iPopPos );
            printf("        iStartPos = %d \n", pQueue->iStartPos);
            printf("        iEndPos = %d \n",pQueue->iEndPos );
            printf("        iTailPos = %d \n\n", pQueue->iTailPos);
        }
		//打印分片备份链路缓存区
		
        //变长存储区
        printf("VarChar and Blob Share Memory:\n");
        TMdbVarCharCtrl varcharCtrl;
        CHECK_RET(varcharCtrl.Init(m_pShmDSN->GetInfo()->sName),"Init failed.");
        TShmList<TMdbVarchar>::iterator itor = m_pShmDSN->m_VarCharList.begin();
        for(;itor != m_pShmDSN->m_VarCharList.end();++itor)
        {
            TMdbVarchar* pVarchar=&(*itor);
            if(pVarchar->iTotalPages == 0){continue;}
            TMdbTSNode* pNodeTmp = &pVarchar->tNode;
            while(true)
            {
                printf("\tShm-ID=%d(0x%0x)\n",pNodeTmp->iShmID,pNodeTmp->iShmID);
                printf("\t\tiVarcharID   		= %d\n", pVarchar->iVarcharID);
                printf("\t\tiTotalPages   		= %d\n", pVarchar->iTotalPages);
                printf("\t\tiStartPage  	= %d\n", pNodeTmp->iPageStart);
                printf("\t\tiEndPage     		= %d\n", pNodeTmp->iPageEnd);
                if(0 == pNodeTmp->iNext )
                {
                    break;
                }
                else
                {
                    pNodeTmp = varcharCtrl.GetNextNode(pNodeTmp);
                }
            }
        }
        return iRet;
    }


    /******************************************************************************
    * 函数名称  :  PrintTableSpace()
    * 函数描述  :  打印表空间信息
    * 输入      :  pszTableSpace, 表空间名称
    * 输出      :  无
    * 返回值    :  无
    * 作者      :  li.shugang
    *******************************************************************************/
    void TMdbInfo::PrintTableSpace(const char* pszTableSpace)
    {
        if(m_pDsn == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s : %d] : PrintTableSpace(%s) m_pDsn is NULL.", __FILE__, __LINE__, pszTableSpace);
            return;
        }
        TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(; itor != m_pShmDSN->m_TSList.end(); ++itor)
        {
            TMdbTableSpace *pTableSpace = &(*itor);
            TADD_DETAIL("TableSpacename = [%s].", pTableSpace->sName);
            if(pTableSpace->sName[0] != 0 )
            {
                if(pszTableSpace == NULL)
                {
                    pTableSpace->Show(m_bMore);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pszTableSpace, pTableSpace->sName) == 0)
                {
                    pTableSpace->Show(m_bMore);
                    break;
                }
            }
        }
        printf("Note:\n");
        printf("State:(0-unused, 1-using, 2-creating, 3-destroying)\n");
    }


    /******************************************************************************
    * 函数名称  :  PrintTable()
    * 函数描述  :  打印表信息
    * 输入      :  pszTable, 表名称
    * 输出      :  无
    * 返回值    :  无
    * 作者      :  li.shugang
    * 修改      :  jiang.mingjun
    *******************************************************************************/
    void TMdbInfo::PrintTable(const char* pszTable)
    {
        if(m_pDsn == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN," m_pDsn is NULL.",pszTable);
            return;
        }
        //int i = 0;
        TMdbIndexCtrl tIndexCtrl;
        TShmList<TMdbTable>::iterator itor= m_pShmDSN->m_TableList.begin();
        for(; itor != m_pShmDSN->m_TableList.end(); ++itor)
        {
            TMdbTable *pTable = &(*itor);
            if(pTable->sTableName[0] != 0)
            {
                if(pszTable == NULL)
                {
                    pTable->Show(m_bMore);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pszTable, pTable->sTableName) == 0)
                {
                    EstimateTableCostMemory(pTable);
                    pTable->Show(m_bMore);
                    if(0 == tIndexCtrl.AttachTable(m_pShmDSN,pTable))
                    {
                        tIndexCtrl.PrintIndexInfo(0,true);
                    }
                    break;
                }
            }
        }
        printf("Note:\n");
        printf("Stat :(0-unused, 1-using, 2-loading, 3-repping, 4-waitingRep)\n");
    }



    /******************************************************************************
    * 函数名称  :  PrintSQL()
    * 函数描述  :  打印系统SQL信息
    * 输入      :  iPos, SQL位置，如果为-1表示打印全部
    * 输出      :  无
    * 返回值    :  无
    * 作者      :  li.shugang
    *******************************************************************************/
    void TMdbInfo::PrintSQL(int iPos)
    {

        printf("Pos    SQL\n");
        printf("---------------------------------------------\n");
    }

    /******************************************************************************
    * 函数名称	: PrintJob 
    * 函数描述	:  打印job列表
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  jin.shaohua
    *******************************************************************************/
    void TMdbInfo::PrintJob(const char * sJobName)
    {
        if(m_pDsn == NULL)
            return;
        if(NULL != sJobName)
        {
            TMdbJob *pJob = m_pShmDSN->GetJobByName(sJobName);
            if(NULL == pJob)
            {
                printf("not find job[%s]\n",NULL == sJobName?"nil":sJobName);
            }
            else
            {
                printf("%s\n",pJob->ToString().c_str());
            }
        }
        else
        {
            TShmList<TMdbJob>::iterator itor = m_pShmDSN->m_JobList.begin();
            for(; itor != m_pShmDSN->m_JobList.end(); ++itor)
            {
                TMdbJob *pJob  = &(*itor);
                if(JOB_PER_NONE != pJob->m_iRateType)
                {
                    printf("%s\n",pJob->ToString().c_str());
                }
            }
        }

    }

    /******************************************************************************
    * 函数名称	: PrintUser 
    * 函数描述	: 打印用户列表(主要是为了查询用户密码明文)
    * 输入		:  sUserName 用户名或all
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  cao.peng
    *******************************************************************************/
    void TMdbInfo::PrintUser(const char * sUserName)
    {
        if(m_pConfig == NULL)
            return;
        char sHead[MAX_FILE_NAME] = {0};
        sprintf(sHead,"%-22s %-22s %-22s","user_name","user_pwd","access_attr");
        if(NULL != sUserName)
        {
            TMDbUser *pUser = m_pConfig->GetUser(sUserName);
            if(NULL == pUser)
            {
                printf("not find user[%s]\n",NULL == sUserName?"nil":sUserName);
            }
            else
            {
                printf("%s\n",sHead);
                printf("%-22s %-22s %-22s",pUser->sUser,pUser->sPwd,pUser->sAccess);
            }
        }
        else
        {
            int iUserCounts = m_pConfig->GetUserCounts();
            printf("%s\n",sHead);
            for(int i = 0; i < iUserCounts; i++)
            {
                TMDbUser *pUser = m_pConfig->GetUser(i);
                if(NULL != pUser)
                {
                    printf("%-22s %-22s %-22s\n",pUser->sUser,pUser->sPwd,pUser->sAccess);
                }
            }
        }
    }

    /******************************************************************************
    * 函数名称	: SetRouterToCapture
    * 函数描述	:  设置要捕获的路由
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbInfo::SetRouterToCapture(const char * sRouter)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        _TRY_CATCH_BEGIN_
        TMdbDatabase tDB;
        if(false == tDB.ConnectAsMgr(m_pDsn->sName))
        {
            CHECK_RET(ERR_DB_LOCAL_CONNECT,"connect [%s] faild.",m_pDsn->sName);
        }
        char sTemp[1024] = {0};
        tDB.GetCaptureRouter(sTemp);
        TADD_NORMAL("Now Capture router=[%s]",sTemp);
        if(NULL != sRouter && 0 != sRouter[0])
        {
            //需要变更
            CHECK_RET(tDB.SetCaptureRouter(sRouter),"SetCaptureRouter failed.");
            tDB.GetCaptureRouter(sTemp);
            TADD_NORMAL("New Capture router=[%s]",sTemp);
        }
        _TRY_CATCH_END_
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	: EstimateTableCostMemory
    * 函数描述	:  预估表内存消耗
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbInfo::EstimateTableCostMemory(TMdbTable * pTable)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        CHECK_OBJ(m_pShmDSN);
        printf("\n\n[Table-Memory-Cost]:\n");
        //预估索引段消耗
        printf("\n[Index cost]:\n[indexCount[%d] * record-count[%d]* indexnode-size[%d] * 2/(1024*1024)=[%.2fMB]]",
            pTable->iIndexCounts,pTable->iRecordCounts,sizeof(TMdbIndexNode),
            (pTable->iIndexCounts *pTable->iRecordCounts * sizeof(TMdbIndexNode)*2*1.0)/(1024*1024));    
        printf("\ntip:one index = base index + conflict index.\n");
        TMdbTableSpace * pTS = m_pShmDSN->GetTableSpaceAddrByName(pTable->m_sTableSpace);
        CHECK_OBJ(pTS);
        //预估数据页消耗
        printf("\n[Data cost]:\n"\
                    "[Theory: (record-size[%d]+page-node-size[%d])* record-count[%d]/(1024*1024)=[%.2fMB]]"\
                    "\n[Real:     current-page-count[%d]*page-size[%d]/(1024*1024) = %0.2fMB]\n",
                        pTable->iOneRecordSize,sizeof(TMdbPageNode),pTable->iRecordCounts,
                        (pTable->iOneRecordSize+sizeof(TMdbPageNode))*pTable->iRecordCounts*1.0/(1024*1024),
                        pTable->iFreePages+pTable->iFullPages,pTS->iPageSize,
                        ((pTable->iFreePages+pTable->iFullPages)*pTS->iPageSize*1.0)/(1024*1024));
        return iRet;
    }

    /******************************************************************************
    * 函数名称	: PrintUsageOfLock
    * 函数描述	: 打印QMDB中锁使用情况
    * 输入		:
    * 输出		:
    * 返回值	:  无
    * 作者		:  cao.peng
    *******************************************************************************/
    void TMdbInfo::PrintUsageOfLock()
    {
        char sCreateTime[MAX_TIME_LEN] = {0};
        printf("[DSN Shared Lock]:\n");
        //DSN管理区锁使用情况
        if(m_pDsn->tMutex.IsLock())
        {
            TMdbDateTime::TimevalToTimeStr(m_pDsn->tMutex.m_tCurTime,sCreateTime,false);
            printf("    DSN[%s] shared lock is busy,the creation time is [%s],the Locked process-id is %d.\n",\
               m_pDsn->sName,sCreateTime,m_pDsn->tMutex.GetLockPID());
        }
        else
        {
            printf("    DSN[%s] shared lock is idle.\n",m_pDsn->sName);
        }
        //varchar 管理区锁使用情况
        printf("[Varchar-Area Lock]:\n");
        TvarcharBlock * pVarcharBlock = (TvarcharBlock *)m_pShmDSN->GetVarcharMgrAddr();
        if(pVarcharBlock->tVarCharMutex.IsLock())
        {
            TMdbDateTime::TimevalToTimeStr(pVarcharBlock->tVarCharMutex.m_tCurTime,sCreateTime,false);
            printf("    Varchar management area lock is busy,the creation time is [%s],the Locked process-id is %d.\n",\
                sCreateTime,pVarcharBlock->tVarCharMutex.GetLockPID());
        }
        else
        {
            printf("    Varchar management area lock is idle.\n");
        }
        //表锁使用情况
        printf("[Table Lock]:\n");
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(; itor != m_pShmDSN->m_TableList.end(); ++itor)
        {
            TMdbTable* pTable = &(*itor);
            if(pTable->sTableName[0] == 0){continue;}
            //自由页的共享锁
            if(pTable->tFreeMutex.IsLock())
            {
                TMdbDateTime::TimevalToTimeStr(pTable->tFreeMutex.m_tCurTime,sCreateTime,false);
                printf("    Free page table[%s] lock is busy,the creation time is [%s],the Locked process-id is %d.\n",\
                    pTable->sTableName,sCreateTime,pTable->tFreeMutex.GetLockPID());
            }
            else
            {
                printf("    Free page table[%s] lock is idle.\n",pTable->sTableName);
            }
            //满页的共享锁
            if(pTable->tFullMutex.IsLock())
            {
                TMdbDateTime::TimevalToTimeStr(pTable->tFullMutex.m_tCurTime,sCreateTime,false);
                printf("    Full page table[%s] lock is busy,the creation time is [%s],the Locked process-id is %d.\n",\
                    pTable->sTableName,sCreateTime,pTable->tFullMutex.GetLockPID());
            }
            else
            {
                printf("    Full page table[%s] lock is idle.\n",pTable->sTableName);
            }
            //表锁
            if(pTable->tTableMutex.IsLock())
            {
                TMdbDateTime::TimevalToTimeStr(pTable->tTableMutex.m_tCurTime,sCreateTime,false);
                printf("    Table[%s] lock is busy,the creation time is [%s],the Locked process-id is %d.\n",\
                    pTable->sTableName,sCreateTime,pTable->tTableMutex.GetLockPID());
            }
            else
            {
                printf("    Table[%s] lock is idle.\n",pTable->sTableName);
            }
        }
        //扫描页锁使用情况
        printf("[Page Lock]:\n");
        TMutex* pMutex = (TMutex*)(m_pShmDSN->GetPageMutexAddr());
        for(int i=0; i<MAX_MUTEX_COUNTS; ++i)
        {
            if(pMutex->IsCreate()){continue;}
            if(pMutex->IsLock())
            {
                TMdbDateTime::TimevalToTimeStr(pMutex->m_tCurTime,sCreateTime,false);
                printf("    Page lock is busy,the creation time is [%s],the Locked process-id is %d.\n",\
                   sCreateTime,pMutex->GetLockPID());
            }
            else
            {
                printf("    Page lock is idle.\n");
            }
            ++pMutex;
        }
        //同步共享内存的Pop/Push锁使用情况
        printf("[Sync-Area Lock]:\n");
        TMdbMemQueue * pMemQueue = (TMdbMemQueue*)m_pShmDSN->GetSyncAreaShm();
        PRINT_SYNC_LOCK_INFO("REP","POP",pMemQueue->tPopMutex);
        PRINT_SYNC_LOCK_INFO("REP","PUSH",pMemQueue->tPushMutex); 
        /*
        int i = 0;
        for(i = 0;i < SA_MAX;++i)
        {
            
            if(NULL == pMemQueue){continue;}
            //pop锁/push锁
            switch(i)
            {
                case SA_ORACLE:
                {
                    PRINT_SYNC_LOCK_INFO("SA_ORACLE","POP",pMemQueue->tPopMutex);
                    PRINT_SYNC_LOCK_INFO("SA_ORACLE","PUSH",pMemQueue->tPushMutex); 
                    break;
                }
                case SA_REP:
                {
                    PRINT_SYNC_LOCK_INFO("SA_REP","POP",pMemQueue->tPopMutex);
                    PRINT_SYNC_LOCK_INFO("SA_REP","PUSH",pMemQueue->tPushMutex);
                    break;
                }
                case SA_CAPTURE:
                {
                    PRINT_SYNC_LOCK_INFO("SA_CAPTURE","POP",pMemQueue->tPopMutex);
                    PRINT_SYNC_LOCK_INFO("SA_CAPTURE","PUSH",pMemQueue->tPushMutex);
                    break;
                }
                case SA_REDO:
                {
                    PRINT_SYNC_LOCK_INFO("SA_REDO","POP",pMemQueue->tPopMutex);
                    PRINT_SYNC_LOCK_INFO("SA_REDO","PUSH",pMemQueue->tPushMutex);
                    break;
                }
                default:
                    break;
            }
        }  
        */
    }

    /******************************************************************************
    * 函数名称	: PrintRoutingRep
    * 函数描述	: 打印QuickMDB路由备份信息
    * 输入		:
    * 输出		:
    * 返回值	:  无
    * 作者		:  jiang.lili
    *******************************************************************************/
    void TMdbInfo::PrintRoutingRep()
    {
        TMdbShmRepMgr *pShmMgr = new(std::nothrow) TMdbShmRepMgr(m_pDsn->sName);
        if (NULL == pShmMgr)
        {
            printf("ERROR! \n");
            return;
        }

        if (pShmMgr->Attach() != ERROR_SUCCESS)
        {
            printf("ERROR! Attach shared memory failed. Check if QuickMDB is created.\n");
            SAFE_DELETE(pShmMgr);
            return;
        }
  
        const TMdbShmRep *pShmRep = pShmMgr->GetRoutingRep();
        //是否分片备份模式
        printf("\tIs-Shard-Rep Mode = [%s]\n", pShmRep->m_bNoRtMode?"NO":"YES");
        printf("\tLoad-failed Routing_list = [%s]\n", pShmRep->m_sFailedRoutingList);

        //路由信息
        printf("\tRouting information:\n");
        printf("\t\tRouting-ID Count = [%d]\n", pShmRep->m_iRoutingIDCount);
        //printf("\t\tRouting-List = [%s]\n", pShmRep->m_sRoutingList);

        //备机信息
        printf("\tStandby hosts information:\n");
        printf("\t\tStandby-Host Count = [%d]\n", pShmRep->m_iRepHostCount);

        for (int i = 0; i<pShmRep->m_iRepHostCount; i++)
        {
            printf("\t\tHost-ID = [%d] IP = [%s] Port = [%d] SameEndian = [%s]\n", pShmRep->m_arHosts[i].m_iHostID, pShmRep->m_arHosts[i].m_sIP, pShmRep->m_arHosts[i].m_iPort, pShmRep->m_arHosts[i].m_bSameEndian?"true":"false");
        }

        //映射关系
        printf("\tRouting Map:\n");
        int k = 0;
        for (int j = 0; j<pShmRep->m_iRoutingIDCount; j++)
        {
            printf("\t\tRouting-ID = [%d] Routing-Value = [", pShmRep->m_arRouting[j].m_iRoutingID);
			bool bFirst = true;
			for(k = 0; k<MAX_REP_ROUTING_ID_COUTN; k++)
			{
				if(pShmRep->m_arRouting[j].m_iRouteValue[k] == EMPTY_ROUTE_ID) break;
				if(bFirst)
				{
					printf("%d", pShmRep->m_arRouting[j].m_iRouteValue[k]);
				}
				else
				{
					printf(",%d", pShmRep->m_arRouting[j].m_iRouteValue[k]);
				}
			}
			printf("] Standby-Host ID = [");
            if (pShmRep->m_arRouting[j].m_aiHostID[0] ==MDB_REP_EMPTY_HOST_ID)
            {
                printf("-1");
            }
            else
            {
                k = 0;
                for (; k < MAX_REP_HOST_COUNT; k++)
                {
                    if (pShmRep->m_arRouting[j].m_aiHostID[k] == MDB_REP_EMPTY_HOST_ID)
                    {
                        break;
                    }
                    if (0 == k)
                    {
                        printf("%d", pShmRep->m_arRouting[j].m_aiHostID[k]);
                    }
                    else
                    {
                        printf(",%d", pShmRep->m_arRouting[j].m_aiHostID[k]);
                    }
                }
            }

			printf(" Recovery_Host ID = [");
            if (pShmRep->m_arRouting[j].m_iRecoveryHostID[0] ==MDB_REP_EMPTY_HOST_ID)
            {
                printf("-1");
            }
            else
            {
                k = 0;
                for (; k < MAX_REP_HOST_COUNT; k++)
                {
                    if (pShmRep->m_arRouting[j].m_iRecoveryHostID[k] == MDB_REP_EMPTY_HOST_ID)
                    {
                        break;
                    }
                    if (0 == k)
                    {
                        printf("%d", pShmRep->m_arRouting[j].m_iRecoveryHostID[k]);
                    }
                    else
                    {
                        printf(",%d", pShmRep->m_arRouting[j].m_iRecoveryHostID[k]);
                    }
                }
            }
            printf("]\n");
        }

        SAFE_DELETE(pShmMgr);
    }
//}

void TMdbInfo::PrintNotLoadFromDBInfo()//控制表从数据库加载的附加配置信息
{
    if(m_pConfig->GetDSN()->m_vNotLoadFromDBTab.empty()) 
    {
        printf("No table is set to not load from database.\n");
        return ;
    }
    
    printf("table not to load from database:\n");
    std::vector<std::string>::iterator itor = m_pConfig->GetDSN()->m_vNotLoadFromDBTab.begin();
    std::vector<std::string>::iterator itorBegin = itor;
    for(; itor != m_pConfig->GetDSN()->m_vNotLoadFromDBTab.end(); ++itor)
    {
        if(false == itor->empty())
        {
            if(itor == itorBegin)
            {
                printf("%s",itor->c_str());
            }
            else
            {
                printf(",%s",itor->c_str());
            }
        }
    }
    printf("\n");
    
}

void TMdbInfo::PrintVarcharPageList()
{
	TMdbVarCharCtrl tVarCharCtrl;
	tVarCharCtrl.Init(m_pDsn->sName);
	
	for(int i=0;i<10;i++)
	{
		TMdbVarchar* pVarChar = m_pShmDSN->GetVarchar(i);
		if(NULL == pVarChar)
		{
			printf("pVarChar is null,pos=%d.",i);
			return;
		}
		else
		{
			int iHeadID = pVarChar->iFreePageId;
	        TMdbPage * pPage = (TMdbPage *)tVarCharCtrl.GetAddrByPageID(pVarChar,iHeadID);
			if(NULL == pPage) 
			{
				printf("FreeList empty\n");continue;
			}
			printf("FreeList:\n");
			while(pPage->m_iNextPageID!=iHeadID)
			{
				printf("[%d - %d]",pPage->m_iPageID, pPage->m_iNextPageID);
				pPage = (TMdbPage *)tVarCharCtrl.GetAddrByPageID(pVarChar,pPage->m_iNextPageID);
			}


			iHeadID = pVarChar->iFullPageId;
	        pPage = (TMdbPage *)tVarCharCtrl.GetAddrByPageID(pVarChar,iHeadID);
			if(NULL == pPage) 
			{
				printf("FullList empty\n");continue;
			}
			printf("FullList:\n");
			while(pPage->m_iNextPageID!=-1)
			{
				printf("[%d - %d]",pPage->m_iPageID, pPage->m_iNextPageID);
				pPage = (TMdbPage *)tVarCharCtrl.GetAddrByPageID(pVarChar,pPage->m_iNextPageID);
			}

			

		}

	}

}

TMdbSizeInfo::TMdbSizeInfo()
{

    m_pConfig   = NULL;
    m_pShmDSN   = NULL;
    m_pDsn      = NULL;
	m_pShmMgr     = NULL;
}

TMdbSizeInfo::~TMdbSizeInfo()
{
	SAFE_DELETE(m_pShmMgr);
}

int TMdbSizeInfo::Init(const char * sDsn)
{
    int iRet    = 0;
    m_pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
    CHECK_OBJ(m_pConfig);
    m_pShmDSN   = TMdbShmMgr::GetShmDSN(sDsn);
    CHECK_OBJ(m_pShmDSN);
    m_pDsn      = m_pShmDSN->GetInfo();
    CHECK_OBJ(m_pDsn);
    if(m_pConfig->GetIsStartShardBackupRep())
    {
		m_pShmMgr = new (std::nothrow)TMdbShmRepMgr(m_pDsn->sName);
	    CHECK_OBJ(m_pShmMgr);
		CHECK_RET(m_pShmMgr->Attach(), "MdbShmRepMgr Attach Failed.");
    }
    return iRet;
}

void TMdbSizeInfo::PrintResourceInfo(bool bDetail)
{
    printf("[ ResourceSize ]\n");
    if(!bDetail)
    {
        TResourceSize data;
        data.Clear();
        GetMgrSize(data);
        GetDataBlockSize(data);
        GetIndexBlockSize(data);
        GetVarcharBlockSize(data);
        GetSyncSize(data);
		if(m_pConfig->GetIsStartShardBackupRep())
	    {
			const TMdbShmRep * pShmRep = m_pShmMgr->GetRoutingRep();
			for(int i = 0; i<pShmRep->m_iRepHostCount; i++)
			{
				int iHostID = pShmRep->m_arHosts[i].m_iHostID;
				if(iHostID != MDB_REP_EMPTY_HOST_ID)
				{
					GetSBSyncSize(iHostID, data);
				}
			}
		}
        printf("%-16s %-16s %-16s\n","TotalSize(M)","UsedSize(M)","FreeSize(M)");
        printf("%-16.1f %-16.1f %-16.1f\n", data.dTotalSize, data.dUsedSize,data.dTotalSize - data.dUsedSize);
    }
    else
    {
        printf("%-32s %16s %16s %16s\n","MemType","TotalSize","UsedSize","FreeSize");
        TResourceSize data;
        data.Clear();
        GetMgrSize(data);
        data.Print();
        
        data.Clear();
        GetDataBlockSize(data);
        data.Print();
        
        data.Clear();
        GetIndexBlockSize(data);
        data.Print();
        
        data.Clear();
        GetVarcharBlockSize(data);
        data.Print();
        
        data.Clear();
        GetSyncSize(data);
        data.Print();
		
		if(m_pConfig->GetIsStartShardBackupRep())
	    {
			const TMdbShmRep * pShmRep = m_pShmMgr->GetRoutingRep();
			for(int i = 0; i<pShmRep->m_iRepHostCount; i++)
			{
				int iHostID = pShmRep->m_arHosts[i].m_iHostID;
				if(iHostID != MDB_REP_EMPTY_HOST_ID)
				{
					data.Clear();
					GetSBSyncSize(iHostID, data);
	        		data.Print();
				}
			}
		}
    }
    printf("\n");
}

void TMdbSizeInfo::GetHashIndexSize(TResourceSize &data )
{
    int i   = 0;
    int j   = 0;
    size_t iMemLeft = 0;
    //size_t iMemTotal = 0;
    //索引的管理区很小，忽略
    //hash begin
    //base index
    for(; i < m_pDsn->iBaseIndexShmCounts; ++i)
    {
        if(m_pDsn->iBaseIndexShmID[i]<= 0) continue;
        TMdbBaseIndexMgrInfo* pInfo  = (TMdbBaseIndexMgrInfo*)m_pShmDSN->GetBaseIndex(i);
        if(NULL == pInfo) continue;
        for(; j<MAX_BASE_INDEX_COUNTS; j++)
        {
            if(pInfo->tFreeSpace[j].iPosAdd == 0) break;
            iMemLeft += pInfo->tFreeSpace[j].iPosAdd;
        }
        data.dTotalSize += (double)pInfo->iTotalSize*1.0/(1024*1024*1.0);
        data.dUsedSize  += (double)iMemLeft*1.0/(1024*1024*1.0);
    }
    
    //conflict index
    i = j = 0;
    for(; i < m_pDsn->iConflictIndexShmCounts; ++i)
    {
        if(m_pDsn->iConflictIndexShmID[i]<= 0) continue;
        TMdbConflictIndexMgrInfo* pInfo  = (TMdbConflictIndexMgrInfo*)m_pShmDSN->GetConflictIndex(i);
        if(NULL == pInfo) continue;
        iMemLeft = 0;
        for(; j<MAX_BASE_INDEX_COUNTS; j++)
        {
            if(pInfo->tFreeSpace[j].iPosAdd == 0) break;
            iMemLeft += pInfo->tFreeSpace[j].iPosAdd;
        }
        data.dTotalSize += (double)pInfo->iTotalSize*1.0/(1024*1024*1.0);
        data.dUsedSize  += (double)iMemLeft*1.0/(1024*1024*1.0);
    }
    
    //hash end
}

void TMdbSizeInfo::GetMHashIndexSize(TResourceSize &data )
{
    int i   = 0;
    int j   = 0;
    size_t iMemLeft = 0;
    size_t iMemTotal = 0;
    //索引的管理区很小，忽略
    //hash begin
    //base index
    //i = j = 0;
    for(; i < m_pDsn->iMHashBaseIdxShmCnt; ++i)
    {
        if(m_pDsn->iMHashBaseIdxShmID[i]<= 0) continue;
        TMdbMHashBaseIndexMgrInfo* pInfo  = (TMdbMHashBaseIndexMgrInfo*)m_pShmDSN->GetMHashBaseIndex(i);
        if(NULL == pInfo) continue;
        for(; j<MAX_MHASH_INDEX_COUNT; j++)
        {
            if(pInfo->tFreeSpace[j].iPosAdd == 0) break;
            iMemLeft += pInfo->tFreeSpace[j].iPosAdd;
        }
        data.dTotalSize += (double)pInfo->iTotalSize*1.0/(1024*1024*1.0);
        data.dUsedSize  += (double)iMemLeft*1.0/(1024*1024*1.0);
    }


    //confilict
    //iMemLeft    = 0;
    //iMemTotal   = 0;
    TShmList<TMdbMhashBlock>::iterator itor_conf = m_pShmDSN->m_MhashConfList.begin();
    for(;itor_conf != m_pShmDSN->m_MhashConfList.end();++itor_conf)
    {
        if(itor_conf->iShmID != INITVAl)
        {
            iMemTotal   += itor_conf->iSize;
            if(itor_conf->iStartNode > 0)
            {
                iMemLeft    += (size_t)(itor_conf->iEndNode - itor_conf->iStartNode)*sizeof(TMdbMHashConfIndexNode);
            }
        }
    }
    data.dTotalSize += (double)iMemTotal*1.0/(1024*1024*1.0);
    data.dUsedSize  += (double)iMemLeft*1.0/(1024*1024*1.0);

    //mhash end
    
}

void TMdbSizeInfo::GetMHashOtherSize(TResourceSize &data )
{
    int i   = 0;
    int j   = 0;
    size_t iMemLeft = 0;
    size_t iMemTotal = 0;
    //索引的管理区很小，忽略
    //hash begin
    //base index

    //Mutex
    //i = j = 0;
    for(; i < m_pDsn->iMHashMutexShmCnt; ++i)
    {
        if(m_pDsn->iMHashMutexShmID[i]<= 0) continue;
        TMdbMHashMutexMgrInfo* pInfo  = (TMdbMHashMutexMgrInfo*)m_pShmDSN->GetMHashMutex(i);
        if(NULL == pInfo) continue;
        iMemLeft = 0;
        for(; j<MAX_MHASH_INDEX_COUNT; j++)
        {
            if(pInfo->tFreeSpace[j].iPosAdd == 0) break;
            iMemLeft += pInfo->tFreeSpace[j].iPosAdd;
        }
        data.dTotalSize += (double)pInfo->iTotalSize*1.0/(1024*1024*1.0);
        data.dUsedSize  += (double)iMemLeft*1.0/(1024*1024*1.0);
    }

    //layer
    iMemLeft    = 0;
    iMemTotal   = 0;
    TShmList<TMdbMhashBlock>::iterator itor_layer = m_pShmDSN->m_MhashLayerList.begin();
    for(;itor_layer != m_pShmDSN->m_MhashLayerList.end();++itor_layer)
    {
        if(itor_layer->iShmID != INITVAl)
        {
            iMemTotal   += itor_layer->iSize;
            if(itor_layer->iStartNode > 0)
            {
                iMemLeft    += (size_t)(itor_layer->iEndNode - itor_layer->iStartNode)*sizeof(TMdbMHashConfIndexNode);
            }
        }
    }
    data.dTotalSize += (double)iMemTotal*1.0/(1024*1024*1.0);
    data.dUsedSize  += (double)iMemLeft*1.0/(1024*1024*1.0);
    //mhash end
    
}

void TMdbSizeInfo::GetIndexBlockSize(TResourceSize &data )
{
    SAFESTRCPY(data.sDataType,sizeof(data.sDataType),"IndexBlock");
    GetHashIndexSize(data);
    GetMHashIndexSize(data);
    GetMHashOtherSize(data);
    
}

void TMdbSizeInfo::GetDataBlockSize(TResourceSize &data )
{
    SAFESTRCPY(data.sDataType,sizeof(data.sDataType),"DataBlock");
    for(int i = 0; i < m_pDsn->iShmCounts; ++i)
    {
        if(m_pDsn->iShmID[i]<= 0) continue;
        TMdbShmHead *pHead = (TMdbShmHead*)m_pShmDSN->GetDataShm(i);
        if(NULL == pHead) continue;
        data.dTotalSize += (double)pHead->iTotalSize*1.0/(1024*1024*1.0);
        data.dUsedSize  += (double)pHead->iLeftOffSet*1.0/(1024*1024*1.0);
    }

    
}

void TMdbSizeInfo::GetVarcharBlockSize(TResourceSize &data )
{
    SAFESTRCPY(data.sDataType,sizeof(data.sDataType),"VarcharBlock");
    for(int i = 0; i < m_pDsn->iVarCharShmCounts; ++i)
    {
        if(m_pDsn->iVarCharShmID[i]<= 0) continue;
        char* pAddr  = NULL;
        m_pShmDSN->AttachvarCharBlockShm(i,&pAddr);
        TMdbShmHead *pHead = (TMdbShmHead*)pAddr;
        if(NULL == pHead) continue;
        data.dTotalSize += (double)pHead->iTotalSize*1.0/(1024*1024*1.0);
        data.dUsedSize  += (double)pHead->iLeftOffSet*1.0/(1024*1024*1.0);
    }
}

void TMdbSizeInfo::GetMgrSize(TResourceSize &data )
{
    SAFESTRCPY(data.sDataType,sizeof(data.sDataType),"ManagerBlock");
    data.dTotalSize = (double)m_pConfig->GetDSN()->iManagerSize*1.0/(1024*1024*1.0);
    data.dUsedSize  = (double)m_pShmDSN->GetUsedSize()*1.0/(1024*1024*1.0);
}

void TMdbSizeInfo::GetSyncSize(TResourceSize &data )
{
    SAFESTRCPY(data.sDataType,sizeof(data.sDataType),"SyncBlock");
    //int i = 0;
    //for(i = 0;i < SA_MAX;++i)
    {
        TMdbSyncArea &tSA = m_pDsn->m_arrSyncArea;
        if(tSA.m_iShmID <=0) return;
        TMdbMemQueue* pQueue = (TMdbMemQueue*)m_pShmDSN->GetSyncAreaShm();
        if(NULL == pQueue) return;
        long iLeftSize = 0;
        if(pQueue->iPushPos >= pQueue->iPopPos)
        {
            iLeftSize = (pQueue->iTailPos - pQueue->iStartPos) - (pQueue->iPushPos - pQueue->iPopPos);
        }
        else
        {
            iLeftSize = (pQueue->iPopPos - pQueue->iPushPos);
        }
        data.dTotalSize += (double)tSA.m_iShmSize; 
        data.dUsedSize  += (double)tSA.m_iShmSize-(double)iLeftSize*1.0/(1024*1024*1.0);
        
    }
    
}

void TMdbSizeInfo::GetSBSyncSize(int iHostID, TResourceSize &data)
{
    snprintf(data.sDataType,sizeof(data.sDataType),"ShardBuckupSyncBlock [%d]", iHostID);

	TMdbShardBakBufArea & tSBBA = m_pDsn->m_arrShardBakBufArea;
	TMdbOnlineRepMemQueue* pOLQueue = NULL;
	if(iHostID != MDB_REP_EMPTY_HOST_ID && tSBBA.m_iShmID[iHostID] > 0)
	{
        pOLQueue = (TMdbOnlineRepMemQueue*)m_pShmDSN->GetShardBakBufAreaShm(iHostID);
        if(NULL == pOLQueue) return;
	}
	else
	{
		return;
	}
	long iLeftSize = 0;
    if(pOLQueue->iPushPos >= pOLQueue->iCleanPos)
    {
        iLeftSize = (pOLQueue->iTailPos - pOLQueue->iStartPos) - (pOLQueue->iPushPos - pOLQueue->iCleanPos);
    }
    else
    {
        iLeftSize = (pOLQueue->iCleanPos - pOLQueue->iPushPos);
    }
    data.dTotalSize += (double)tSBBA.m_iShmSize; 
    data.dUsedSize  += (double)tSBBA.m_iShmSize-(double)iLeftSize*1.0/(1024*1024*1.0);
}


void TMdbSizeInfo::PrintTableInfo(const char *sTableName,int iCount)
{
    if(NULL == sTableName) return;
    
    printf("[ TableSize ]\n");
    printf("%-32s %16s %16s %16s %16s\n",
        "TableName",
        "DataSize",
        "IndexSize",
        "TotalRecordCount",
        "UsedRecordCount");
    TShmList<TMdbTable>::iterator itor= m_pShmDSN->m_TableList.begin();
    TTableSize tTableSize;
    for(; itor != m_pShmDSN->m_TableList.end(); ++itor)
    {
        TMdbTable *pTable = &(*itor);
        if(pTable->sTableName[0] != 0)
        {
            
            if(sTableName[0] == 0)
            {
                tTableSize.Clear();
                GetOneTableSize(pTable, tTableSize);
                tTableSize.Print();
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(sTableName, pTable->sTableName) == 0)
            {
                tTableSize.Clear();
                tTableSize.iTotalCount = iCount;
                GetOneTableSize(pTable, tTableSize);
                tTableSize.Print();
                break;
            }
        }
    }
    printf("\n");
    
}

void TMdbSizeInfo::GetOneTableSize(TMdbTable * pTable,TTableSize &tTableSize)
{
    if(NULL == pTable) return;
    SAFESTRCPY(tTableSize.sTableName,sizeof(tTableSize.sTableName),pTable->sTableName);
    if(tTableSize.iTotalCount <= 0)
    {//没有指定记录数，则使用内存中的记录数
        tTableSize.iTotalCount  = pTable->iRecordCounts;
		
        tTableSize.iUsedCount   = pTable->iCounts;
    }
    else
    {//输入的表记录数 需要 根据算法做改变
        TMdbTable table;
        table.iRecordCounts = tTableSize.iTotalCount;
        table.ResetRecordCounts();
        tTableSize.iTotalCount = table.iRecordCounts;
        tTableSize.iUsedCount  = 0;//如果指定了记录数，该项直接置0
    }
    tTableSize.dDataSize    = (double)(pTable->iOneRecordSize + (int)sizeof(TMdbPageNode))*tTableSize.iTotalCount*1.0/(1024*1024*1.0);
	
	tTableSize.dIndexSize   = (double)(pTable->iIndexCounts * 1.0 * tTableSize.iTotalCount * (int)sizeof(TMdbIndexNode))*2.0/(1024*1024*1.0);


}

