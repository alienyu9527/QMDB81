/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbMonitor.cpp
*@Description��
*@Author:			jin.shaohua
*@Date��	    2012.10
*@History:
******************************************************************************************/
#include "App/mdbMonitor.h"
#include "Helper/TThreadLog.h"
#include "Control/mdbMgrShm.h"
#include "Control/mdbSysTableThread.h"
#include "Control/mdbTimeThread.h"
#include "Helper/mdbDateTime.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Control/mdbAutoUnlockThread.h"


//namespace QuickMDB{

TMdbMonitor::TMdbMonitor():
m_pShmDSN(NULL),
m_pMdbLink(NULL),
m_pQueryMem(NULL),
m_pQuerySession(NULL),
m_pQueryTS(NULL)
{

}
TMdbMonitor::~TMdbMonitor()
{
    SAFE_DELETE(m_pMdbLink);
    SAFE_DELETE(m_pQueryMem);
    SAFE_DELETE(m_pQuerySession);
    SAFE_DELETE(m_pQueryTS);
}

/******************************************************************************
* ��������	:  Init
* ��������	:  ��ʼ��
* ����		:  sDsn - ��ʼ����dsn��
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbMonitor::Init(const char * sDsn)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(sDsn);
    m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
    CHECK_OBJ(m_pShmDSN);
    m_pDsn   = m_pShmDSN->GetInfo();
    CHECK_OBJ(m_pDsn);
    SAFE_DELETE(m_pMdbLink);
    m_pMdbLink = new(std::nothrow) TMdbDatabase();
    CHECK_OBJ(m_pMdbLink);
    try
    {
        m_pMdbLink->ConnectAsMgr(sDsn);
        m_pQueryMem = m_pMdbLink->CreateDBQuery();
        m_pQueryMem->SetSQL("select mem_type,mem_size,mem_left from dba_resource");
        CHECK_OBJ(m_pQueryMem);
        m_pQuerySession = m_pMdbLink->CreateDBQuery();
        m_pQuerySession->SetSQL("select ip,start_time,state from dba_session");
        CHECK_OBJ(m_pQuerySession);
        m_pQueryTS = m_pMdbLink->CreateDBQuery();
        m_pQueryTS->SetSQL("select table_space_name,page_size,ask_pages,total_pages,free_pages from dba_table_space");
        CHECK_OBJ(m_pQueryTS);
        
    }
    catch(TMdbException &e)
    {
        CHECK_RET(-1,"Link to mdb[%s] error msg=[%s].",sDsn,e.GetErrMsg());
    }
    catch(...)
    {
        CHECK_RET(-1,"Link to mdb[%s] error.",sDsn);
    }
    //ReConnectOracle();//������oracle������
    CHECK_RET(m_tProcCtrl.Init(sDsn),"m_tProcCtrl.Init error.");
    TADD_FUNC("Finish.");
    return iRet;

}
/******************************************************************************
* ��������	:  Start
* ��������	:  ��ʼ���
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbMonitor::Start()
{
    int iRet = 0;
    //ʱ�����߳�
    TMdbTimeThread timeThread;
    timeThread.Init(m_pShmDSN);
    timeThread.Start();
    //ϵͳ�����߳�
    TMdbSysTableThread sysTableThread;
    sysTableThread.Init(m_pShmDSN);
    sysTableThread.Start();
    //�Զ���������߳�
    TMdbAutoUnlockThread autoUnlockThread;
    autoUnlockThread.Init(m_pShmDSN);
    autoUnlockThread.Start();    
    while(1)
    {
        TADD_FLOW("QuickMDB Main Process Auto-Checking......");
        TMdbDateTime::MSleep(1500);
        //�ж��Ƿ����
        if(m_tProcCtrl.IsDBStop())
        {
            timeThread.Stop();sysTableThread.Stop();autoUnlockThread.Stop();//ֹͣ�����߳�
            TMdbDateTime::MSleep(500);
            while(timeThread.IsRun() || sysTableThread.IsRun() || autoUnlockThread.IsRun())
            {
                TADD_NORMAL("Some thread is still running....");
                TMdbDateTime::MSleep(1000);
            }
            TADD_NORMAL("QmdbMonitor stop");
            break;
        }
        //��������
        m_tProcCtrl.UpdateProcHeart(0);
        //��ظ������̵�״̬
        m_tProcCtrl.ScanAndClear();
        //��ر�ռ�,��ʱ�������ݵ���
        CHECK_RET_NONE(SpaceCtrl(),"SpaceCtrl failed.");
         //ˢ������,ͬ����oracle
       // CHECK_RET_NONE(FlushSequence(),"FlushSequence failed");
         //д��Ϣ���ļ�
        //CHECK_RET_NONE(WritePerformanceToFile(),"WritePerformanceToFile failed");
    }
    return iRet;
}


bool TMdbMonitor::IsDBStop()
{
    return m_pShmDSN->GetInfo()->cState == DB_stop;
}

/******************************************************************************
* ��������	:  SpaceCtrl()
* ��������	:  ���Ƶ�����ռ�Ĵ�С
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �ɹ����ض�Ӧ0�����򷵻�-1
* ����		:  li.shugang
*******************************************************************************/
int TMdbMonitor::SpaceCtrl()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
    TMdbTableSpaceCtrl tableSpaceCtrl;
    for(;itor != m_pShmDSN->m_TSList.end();++itor)
    {
        TMdbTableSpace *pTmp = &(*itor);
        if(pTmp->sName[0] == 0)
        {
            continue;
        }
        TADD_DETAIL("Table-Space[%s] : iEmptyPages=%d, iRequestCounts=%d.", 
            pTmp->sName, pTmp->iEmptyPages, pTmp->iRequestCounts);
        //�����ռ��㹻��ֱ������
        if(pTmp->iEmptyPages >= pTmp->iRequestCounts)
        {
            continue;
        }
        //�����ռ䲻�㣬���ű�ռ�
        CHECK_RET(tableSpaceCtrl.Init(m_pShmDSN->GetInfo()->sName,pTmp->sName),"tableSpace.Init failed");
        TADD_NORMAL("tablespce[%s] ask newPages[%d]",pTmp->sName,pTmp->iRequestCounts);
        CHECK_RET(pTmp->tEmptyMutex.Lock(true, &m_pDsn->tCurTime),"tEmptyMutex.Lock() failed.");
        do{
            CHECK_RET_BREAK(tableSpaceCtrl.AskNewPages(pTmp->iRequestCounts),"AskNewPage(%d) failed.",pTmp->iRequestCounts);
        }while(0);
        CHECK_RET(pTmp->tEmptyMutex.UnLock(true),"unlock failed.");
        TADD_NORMAL("tablespce[%s] ask newPages[%d]",pTmp->sName,pTmp->iRequestCounts);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

#if 0

/******************************************************************************
* ��������	:  FlushSequence()
* ��������	:  ˢ������,ͬ����oracle
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
int TMdbMonitor::FlushSequence()
{
    TADD_FUNC("Start.");
    TMemSeq *pTmp = NULL;
    //ˢ��Sequence
    TShmList<TMemSeq>::iterator itor = m_pShmDSN->m_MemSeqList.begin();
    for(;itor != m_pShmDSN->m_MemSeqList.end();++itor)
    {
        pTmp = &(*itor);
        //���û�б�ռ�
        if(pTmp->sSeqName[0] == 0)
        {
            continue;
        }
        TADD_DETAIL("sSeqName=%s, iCur=%lld, Step=%lld.", pTmp->sSeqName,pTmp->iCur, pTmp->iStep);
        if(NULL == m_pOraQuery)
        {
            ReConnectOracle();
        }
        else
        {
            try
            {
                char sUpperDsn[64];
                memset(sUpperDsn,0,sizeof(sUpperDsn));
                TMdbStrFunc::ToUpper(m_pDsn->sName,sUpperDsn);
                char sQuerySeqTabSQL[MAX_SQL_LEN] = {0};
                sprintf(sQuerySeqTabSQL,"select count(*) seqcount from user_tables where table_name = '%s_MDB_SEQUENCE'",sUpperDsn);
                m_pOraQuery->Close();
                m_pOraQuery->SetSQL(sQuerySeqTabSQL);
                m_pOraQuery->Open();
                int iSeqTable = -1;
                if(m_pOraQuery->Next())
                {
                    iSeqTable = m_pOraQuery->Field("seqcount").AsInteger();
                }
                char sSQL[MAX_SQL_LEN] = {0};
                if(iSeqTable >= 1)//���%sDSN%s_MDB_SEQUENCE����
                {
                    sprintf(sSQL,"update %s_MDB_SEQUENCE set CUR_NUMBER=:cur where upper(SEQ_NAME)=upper(:name) and upper(DSN_NAME)=upper(:dsn_name) and LOCAL_IP=:local_ip ",sUpperDsn);
                }
                else//���%sDSN%s_MDB_SEQUENCE������ MDB_SEQUENCE ����ڻ��� MDB_SEQUANCE�����
                {
                    char sQueryMdbSequance[MAX_SQL_LEN];
                    memset(sQueryMdbSequance,0,sizeof(sQueryMdbSequance));
                    sprintf(sQueryMdbSequance,"select table_name from user_tables where table_name = 'MDB_SEQUENCE' OR table_name = 'MDB_SEQUANCE' ");
                    m_pOraQuery->Close();
                    m_pOraQuery->SetSQL(sQueryMdbSequance);
                    m_pOraQuery->Open();
                    int iTabCounts = 0;
                    char sTabName[MAX_NAME_LEN];
                    memset(sTabName,0,sizeof(sTabName));
                    while(m_pOraQuery->Next())
                    {
                        iTabCounts++;
                        SAFESTRCPY(sTabName,sizeof(sTabName),m_pOraQuery->Field("table_name").AsString());
                        if(TMdbStrFunc::StrNocaseCmp(sTabName,"MDB_SEQUENCE") == 0)
                        {
                            memset(sSQL,0,sizeof(sSQL));
                            SAFESTRCPY(sSQL,sizeof(sSQL),"update MDB_SEQUENCE set CUR_NUMBER=:cur where upper(SEQ_NAME)=upper(:name) and upper(DSN_NAME)=upper(:dsn_name) and LOCAL_IP=:local_ip ");
                        }
                        else if(TMdbStrFunc::StrNocaseCmp(sTabName,"MDB_SEQUANCE") == 0)
                        {
                            memset(sSQL,0,sizeof(sSQL));
                            SAFESTRCPY(sSQL,sizeof(sSQL),"update MDB_SEQUANCE set CUR_NUMBER=:cur where upper(SEQ_NAME)=upper(:name) and upper(DSN_NAME)=upper(:dsn_name) and LOCAL_IP=:local_ip ");
                        }
                    }
                    if(iTabCounts > 1)//����ͬʱ���ڣ�����
                    {
                        TADD_ERROR(ERROR_UNKNOWN,"both table MDB_SEQUENCE and MDB_SEQUANCE exist,please check,drop one.");
                        return ERR_TAB_TABLE_NUM_EXCEED_MAX;
                    }
                }
                m_pOraQuery->Close();
                m_pOraQuery->SetSQL(sSQL);
                m_pOraQuery->SetParameter("cur", pTmp->iCur);
                m_pOraQuery->SetParameter("name", pTmp->sSeqName);
                m_pOraQuery->SetParameter("dsn_name", m_pDsn->sName);
                m_pOraQuery->SetParameter("local_ip", m_pDsn->sLocalIP);
                m_pOraQuery->Execute();
                int iRowsAffected = m_pOraQuery->RowsAffected();  
                if(iRowsAffected > 0)
                {
                    TADD_DETAIL("iRowsAffected=[%d], CUR_NUMBER=[%lld],sSeqName=[%s].",iRowsAffected,pTmp->iCur,pTmp->sSeqName);
                }
                m_pOraQuery->Commit();
            }
            catch(TOraDBException &e)
            {
                TADD_ERROR(ERROR_UNKNOWN,"SQL=[%s], error_msg=[%s]", e.GetErrSql(), e.GetErrMsg());
                ReConnectOracle();
            }
        }
    }
    TADD_FUNC(" Finish.");
    return 0;
}


//��������Oracle
int TMdbMonitor::ReConnectOracle()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    try
    {
        SAFE_DELETE(m_pOraQuery);
        if(NULL != m_pOraLink)
        {
            m_pOraLink->Disconnect();
            SAFE_DELETE(m_pOraLink);
        }
        m_pOraLink = new(std::nothrow) TOraDBDatabase();
        if(NULL == m_pOraLink)
        {
            CHECK_RET(ERR_OS_NO_MEMROY,"Not Have Enough Mem.");
        }
        m_pOraLink->SetLogin(m_pDsn->sOracleUID, m_pDsn->sOraclePWD, m_pDsn->sOracleID);
        if(m_pOraLink->Connect())
        {
            m_pOraQuery = m_pOraLink->CreateDBQuery();
            CHECK_OBJ(m_pOraQuery);
        }
        else
        {
            CHECK_RET(ERR_DB_NOT_CONNECTED,"Connect to Oracle[%s/%s@%s] Failed.",
                    m_pDsn->sOracleUID, m_pDsn->sOraclePWD, m_pDsn->sOracleID);
        }
    }
    catch(TOraDBException &e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Can't connect to Oracle[%s/%s@%s].\n\t%s",
            m_pDsn->sOracleUID, m_pDsn->sOraclePWD, m_pDsn->sOracleID, e.GetErrMsg());
        return ERROR_UNKNOWN;
     }
    TADD_FUNC("Finish.");
    return iRet;
}
#endif
/******************************************************************************
* ��������	:  WritePerformanceToFile()
* ��������	:  д����ͳ����Ϣ���ļ���IManager����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �ɹ����ض�Ӧ0�����򷵻�-1
* ����		:  jiang.mingjun
*******************************************************************************/
int TMdbMonitor::WritePerformanceToFile()
{
    //�ж��Ƿ�����һ��,�������һ����½���һ���ļ�
    static char strFirstDate[15];
    static char strCurrentDate[15];
    static char strCreateFileDate[9];

    static char strFirstDateTime[15];
    static char strCurrentDateTime[15];
    static bool bNewFlush = false;
    static bool bNewFile = false;
    static char szSessionFileName[256];
    static char szPagesInfoFileName[256];
    static char szProcessInfoFileName[256];
    static char szMemInfoFileName[256];
    static char TempFileName[256];

    static FILE *Sessionfp = NULL;
    static FILE *Processfp = NULL;
    static FILE *Pagesfp = NULL;
    static FILE *Memfp = NULL;
    //int  filehandle = -1 ;

    char sPath[MAX_PATH_NAME_LEN];
    memset(sPath, 0, sizeof(sPath));
    sprintf(sPath, "%s/info/", getenv("QuickMDB_HOME"));

    if(!TMdbNtcDirOper::IsExist(sPath))
    {
        TMdbNtcDirOper::MakeFullDir(sPath);
    }

    if(bNewFile == false)
    {
        bNewFile = true;
        TMdbDateTime::GetNowDateTime(strFirstDate,0);
        strFirstDate[14]='\0';

        TMdbDateTime::GetNowDateTime(strCreateFileDate,1);
        strCreateFileDate[8] = '\0';

        //�ļ���
        memset(szSessionFileName,0,256);
        memset(szPagesInfoFileName,0,256);
        memset(szProcessInfoFileName,0,256);
        memset(szMemInfoFileName,0,256);

        memset(TempFileName,0,256);
        sprintf(TempFileName,"info_qmdb_session_%s",strCreateFileDate);
        sprintf(szSessionFileName, "%s/%s", sPath,TempFileName);
        SAFE_CLOSE(Sessionfp);
        Sessionfp = fopen(szSessionFileName, "a+");
        if(Sessionfp == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Open [%s] Failed.",szSessionFileName);
        }
        memset(TempFileName,0,256);
        sprintf(TempFileName,"info_qmdb_tablespace_%s",strCreateFileDate);
        sprintf(szPagesInfoFileName, "%s/%s", sPath,TempFileName);
        
        SAFE_CLOSE(Pagesfp);
        Pagesfp = fopen(szPagesInfoFileName, "a+");
        if(Pagesfp == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Open [%s] Failed.", szPagesInfoFileName);
        }
        memset(TempFileName,0,256);
        sprintf(TempFileName,"info_qmdb_process_%s",strCreateFileDate);
        sprintf(szProcessInfoFileName, "%s/%s", sPath,TempFileName);
        
        SAFE_CLOSE(Processfp);
        Processfp = fopen(szProcessInfoFileName, "a+");
        if(Processfp == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Open [%s] Failed.",  szProcessInfoFileName);
        }
        memset(TempFileName,0,256);
        sprintf(TempFileName,"info_qmdb_memory_%s",strCreateFileDate);
        sprintf(szMemInfoFileName, "%s/%s", sPath,TempFileName);
        
        SAFE_CLOSE(Memfp);
        Memfp = fopen(szMemInfoFileName, "a+");
        if(Memfp == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Open [%s] Failed.", szMemInfoFileName);
        }
        //дsession��Ϣ
        WriteSession(Sessionfp);
        //дprocess��Ϣ
        WriteProcess(Processfp);
        //дmemery��Ϣ
        WriteMemory(Memfp);
        //д��ռ���Ϣ
        WriteTableSpace(Pagesfp);
    }
    memset(strCurrentDate,0,sizeof(strCurrentDate));
    TMdbDateTime::GetNowDateTime(strCurrentDate,0);
    strCurrentDate[14]='\0';
    if(TMdbDateTime::GetDiffDay(strCurrentDate,strFirstDate) >0)
    {
        bNewFile = false;
        bNewFlush = false;
        //�½��ļ�
    }

    if(bNewFlush == false)
    {
        bNewFlush = true;
        TMdbDateTime::GetNowDateTime(strFirstDateTime,0);
        strFirstDateTime[14]='\0';
    }
    memset(strCurrentDateTime,0,15);
    TMdbDateTime::GetNowDateTime(strCurrentDateTime,0);
    strCurrentDateTime[14]='\0';
    if(TMdbDateTime::GetDiffSeconds(strCurrentDateTime,strFirstDateTime) >5*60 )
    {
        bNewFlush = false;
        //дsession��Ϣ
        WriteSession(Sessionfp);
        //дprocess��Ϣ
        WriteProcess(Processfp);
        //дmemery��Ϣ
        WriteMemory(Memfp);
        //д��ռ���Ϣ
        WriteTableSpace(Pagesfp);
    }
    return 0;
}

void TMdbMonitor::WriteSession(FILE *fp)
{
    if(IsDBStop() == true)
    {
        return ;
    }
    m_pQuerySession->Open();
    while(m_pQuerySession->Next())
    {
        fprintf(fp,"ip=%s|start_time=%s|state=%s\n",m_pQuerySession->Field("ip").AsString(),
            m_pQuerySession->Field("start_time").AsString(),m_pQuerySession->Field("state").AsString());
        fflush(fp);
    }
}


void TMdbMonitor::WriteProcess(FILE *fp)
{
    if(IsDBStop() == true)
    {
        return ;
    }
    TShmList<TMdbProc>::iterator itor = m_pShmDSN->m_ProcList.begin();
    for(;itor != m_pShmDSN->m_ProcList.end();++itor)
    {
        TMdbProc * pProc = &(*itor);
        if(pProc->sName[0] != 0)
        {
            fprintf(fp,"pid=%d|Name=%s|StartTime=%s|State=%c|Restart=%s\n",
                pProc->iPid,pProc->sName,pProc->sStartTime,pProc->cState,pProc->bIsMonitor?"TRUE":"FALSE");
            fflush(fp);
        }
    }
}

void TMdbMonitor::WriteMemory(FILE *fp)
{
    if(IsDBStop() == true)
    {
        return ;
    }
    char strCurrentDateTime[15] = {0};
    TMdbDateTime::GetCurrentTimeStr(strCurrentDateTime);
    m_pQueryMem->Open();

    while(m_pQueryMem->Next())
    {
        fprintf(fp,"cur_time=%s|mem_type=%s|mem_size=%lld|mem_left=%lld\n",
            strCurrentDateTime,m_pQueryMem->Field("mem_type").AsString(),
                m_pQueryMem->Field("mem_size").AsInteger(),
                m_pQueryMem->Field("mem_left").AsInteger());
        fflush(fp);
    }
}

void TMdbMonitor::WriteTableSpace(FILE *fp)
{
    if(IsDBStop() == true)
    {
        return ;
    }
    char strCurrentDateTime[15] = {0};
    TMdbDateTime::GetCurrentTimeStr(strCurrentDateTime);
    m_pQueryTS->Open();
    while(m_pQueryTS->Next())
    {
        fprintf(fp,"cur_time=%s|table_space_name=%s|page_size=%lld|ask_pages=%lld|total_pages=%lld|free_pages=%lld\n",
            strCurrentDateTime,m_pQueryTS->Field("table_space_name").AsString(),
            m_pQueryTS->Field("page_size").AsInteger(),
            m_pQueryTS->Field("ask_pages").AsInteger(),
            m_pQueryTS->Field("total_pages").AsInteger(),
            m_pQueryTS->Field("free_pages").AsInteger());
        fflush(fp);
    }
}


//}
