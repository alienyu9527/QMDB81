/****************************************************************************************
*@Copyrights  2011，中兴软创（南京）计算机有限公司 开发部 CCB项目--QMDB团队
*@                   All rights reserved.
*@Name：        mdbShadowFlushCtrl.cpp
*@Description： 基于影子表模式的ORCLE2MDB同步
*@Author:       li.ming
*@Date：        2013年12月20日
*@History:
******************************************************************************************/
#include "Dbflush/mdbShadowFlushCtrl.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"

//namespace QuickMDB{


    TShadowRecord::TShadowRecord()
    {
        Clear();
    }

    TShadowRecord::~TShadowRecord()
    {
    }


    void TShadowRecord::Clear()
    {
        memset(m_sTabName, 0, sizeof(m_sTabName));
        m_iColmCnt = 0;
        for(int i = 0; i < MAX_COLUMN_COUNTS; i++)
        {
            m_aData[i].Clear();
        }
        m_tDelData.Clear();
    }
    void TShadowRecord::AddData( TMDBDBFieldInterface& tField,int iLen,const MDB_INT64& iSeq)
    {
        TADD_FUNC("Start.");
        m_aData[m_iColmCnt].iLen  = iLen;
        m_aData[m_iColmCnt].iType = tField.GetFieldType();
        m_aData[m_iColmCnt].isNull = tField.isNULL()?-1:0;
        SAFESTRCPY(m_aData[m_iColmCnt].sName,sizeof(m_aData[m_iColmCnt].sName),tField.GetFieldName());
        SAFESTRCPY(m_aData[m_iColmCnt].sPName,sizeof(m_aData[m_iColmCnt].sPName),tField.GetFieldName());
        if(tField.isNULL()== true)
        {
            m_aData[m_iColmCnt].isNull = -1;
        }
        else
        {
            if(iSeq != -1)
            {
                snprintf(m_aData[m_iColmCnt].sValue,sizeof(m_aData[m_iColmCnt].sValue),"%lld", iSeq);
            }
            else
            {
                SAFESTRCPY(m_aData[m_iColmCnt].sValue,sizeof(m_aData[m_iColmCnt].sValue),tField.AsString());
            }       
        }

        m_iColmCnt++;
    }

    void TShadowRecord::AddDelData( TMDBDBFieldInterface& tField,int iLen)
    {
        TADD_FUNC("Start.");
        m_tDelData.iLen  =    iLen;
        m_tDelData.iType = tField.GetFieldType();
        m_tDelData.isNull = tField.isNULL()?-1:0;
        SAFESTRCPY(m_tDelData.sName,sizeof(m_tDelData.sName),tField.GetFieldName());
        SAFESTRCPY(m_tDelData.sPName,sizeof(m_tDelData.sPName),tField.GetFieldName());
        SAFESTRCPY(m_tDelData.sValue,sizeof(m_tDelData.sValue),tField.AsString());
    }

    TTableDAO::TTableDAO()
    {
        memset(m_sTabName, 0, sizeof(m_sTabName));
        m_pInsertDAO = NULL;
        m_pDelDAO = NULL;
    }

    TTableDAO::~TTableDAO()
    {
        memset(m_sTabName, 0, sizeof(m_sTabName));
        SAFE_DELETE(m_pInsertDAO);
        SAFE_DELETE(m_pDelDAO);
    }

    void TTableDAO::Clear()
    {
        memset(m_sTabName, 0, sizeof(m_sTabName));
        SAFE_DELETE(m_pInsertDAO);
        SAFE_DELETE(m_pDelDAO);
    }

    TShadowDAO::TShadowDAO()
    {
        memset(m_sDSN, 0, sizeof(m_sDSN));
        memset(m_sUID, 0, sizeof(m_sUID));
        memset(m_sPWD, 0, sizeof(m_sPWD));
        memset(m_sShadowTabName, 0, sizeof(m_sShadowTabName));
        memset(m_sNotifyTabName, 0, sizeof(m_sNotifyTabName));

        m_pDBLink = NULL;
        m_pQuery = NULL;

        m_pCurRecd = NULL;
    }

    TShadowDAO::~TShadowDAO()
    {
        if(NULL != m_pQuery)
        {
            m_pQuery->Close();
            SAFE_DELETE(m_pQuery);
        }

        if(NULL != m_pDBLink)
        {
            m_pDBLink->Disconnect();
            SAFE_DELETE(m_pDBLink);
        }
    }

    int TShadowDAO::Init(const char* psDsn,const char* psUid, const char* psPwd, const char* psShadowName,const char* psNotifyTabName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        CHECK_OBJ(psDsn);
        CHECK_OBJ(psUid);
        CHECK_OBJ(psPwd);
        CHECK_OBJ(psNotifyTabName);

        SAFESTRCPY(m_sDSN, sizeof(m_sDSN), psDsn);
        SAFESTRCPY(m_sUID, sizeof(m_sUID), psUid);
        SAFESTRCPY(m_sPWD, sizeof(m_sPWD), psPwd);
        SAFESTRCPY(m_sShadowTabName, sizeof(m_sShadowTabName), psShadowName);
        SAFESTRCPY(m_sNotifyTabName, sizeof(m_sNotifyTabName), psNotifyTabName);

        try
        {
            if(m_pDBLink != NULL)
            {
                SAFE_DELETE(m_pDBLink);
            }
            
            m_pDBLink = TMDBDBFactory::CeatDB();
            CHECK_OBJ(m_pDBLink);
            
            m_pDBLink->SetLogin(m_sUID, m_sPWD, m_sDSN);
            if(m_pDBLink->Connect() == false)
            {
                m_pDBLink->Disconnect();
                SAFE_DELETE(m_pDBLink);
                iRet = ERR_APP_CONNCET_ORACLE_FAILED;
            }

        }
        catch(TMDBDBExcpInterface &e)
        {        
            TADD_ERROR(ERR_APP_CONNCET_ORACLE_FAILED,"Can't connect Oracle:%s/***@%s.\n ERR-MSG=%s, ERR_SQL=%s\n", 
                                    m_sUID,m_sDSN, e.GetErrMsg(), e.GetErrSql());
            m_pDBLink->Disconnect();
            if(m_pDBLink != NULL)
            {		      
                SAFE_DELETE(m_pDBLink);
            }
            iRet = ERROR_UNKNOWN;    
        }

        TADD_NORMAL("Connect Oracle:[%s/***@%s] success.", m_sUID,m_sDSN);

        try
        {
            for(int i=0; i<MAX_TABLE_COUNTS; ++i)
            {
                m_vTabDAO[i].Clear();	
            }
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Clear Failed\n");
        }

        TADD_FUNC("Finish.");
        return iRet;    
    }

    int TShadowDAO::Execute( TShadowRecord& tOneRecord)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        m_iCurDaoPos = 0;
        m_iCurDaoPos = FindDAO(tOneRecord.m_sTabName);
        if(m_iCurDaoPos < 0)
        {
            m_iCurDaoPos = CreateDAO(tOneRecord.m_sTabName);
            if(m_iCurDaoPos < 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Create Dao for Table[%s] failed.", tOneRecord.m_sTabName);
                return -1;
            }
        }

        TADD_DETAIL("excute.current pos =%d",m_iCurDaoPos);

        //  insert shadow dao
        m_vTabDAO[m_iCurDaoPos].m_pInsertDAO->StartData();
        for(int i = 0; i < tOneRecord.m_iColmCnt; i++)
        {
            m_vTabDAO[m_iCurDaoPos].m_pInsertDAO->AddData(&(tOneRecord.m_aData[i]));
        }
        m_vTabDAO[m_iCurDaoPos].m_pInsertDAO->EndData();

        TADD_DETAIL("TShadowDAO::Execute() insert: Real-Counts=%d, MAX_DATA_COUNTS=%d.", m_vTabDAO[m_iCurDaoPos].m_pInsertDAO->GetCounts(), MAX_DATA_COUNTS);

        if(m_vTabDAO[m_iCurDaoPos].m_pInsertDAO->GetCounts() >= MAX_DATA_COUNTS)
        {
            iRet = m_vTabDAO[m_iCurDaoPos].m_pInsertDAO->Execute(m_pDBLink); 
            if(iRet < 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"insert dao excute failed.");
                m_pDBLink->Rollback();
                m_vTabDAO[m_iCurDaoPos].m_pInsertDAO->ClearArrayData();  
                return iRet;
            }
            m_vTabDAO[m_iCurDaoPos].m_pInsertDAO->ClearArrayData();  
        }

        // delete change_notify dao
        m_vTabDAO[m_iCurDaoPos].m_pDelDAO->StartData();
        m_vTabDAO[m_iCurDaoPos].m_pDelDAO->AddData(&(tOneRecord.m_tDelData));
        m_vTabDAO[m_iCurDaoPos].m_pDelDAO->EndData();

        TADD_DETAIL("TShadowDAO::Execute() delete: Real-Counts=%d, MAX_DATA_COUNTS=%d.", m_vTabDAO[m_iCurDaoPos].m_pDelDAO->GetCounts(), MAX_DATA_COUNTS);

        if(m_vTabDAO[m_iCurDaoPos].m_pDelDAO->GetCounts() >= MAX_DATA_COUNTS)
        {
            iRet = m_vTabDAO[m_iCurDaoPos].m_pDelDAO->Execute(m_pDBLink); 
            if(iRet < 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"delete dao excute failed.");
                m_pDBLink->Rollback();
                m_vTabDAO[m_iCurDaoPos].m_pDelDAO->ClearArrayData();  
                return iRet;
            }
            m_vTabDAO[m_iCurDaoPos].m_pDelDAO->ClearArrayData();  
        }
        
        return iRet;    
    }

    int TShadowDAO::Commit()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        for(int i = 0; i < MAX_TABLE_COUNTS; i++)
        {
            if(m_vTabDAO[i].m_pInsertDAO == NULL || m_vTabDAO[i].m_pDelDAO == NULL)
            {
                continue;
            }
            
            iRet = m_vTabDAO[i].m_pInsertDAO->Execute(m_pDBLink); 
            if(iRet < 0)
            {
                m_pDBLink->Rollback();
                TADD_ERROR(ERROR_UNKNOWN,"Commit,but excute failed. rollback");
                m_vTabDAO[i].m_pInsertDAO->ClearArrayData();  
                break;
            }
            m_vTabDAO[i].m_pInsertDAO->ClearArrayData();  

            iRet = m_vTabDAO[i].m_pDelDAO->Execute(m_pDBLink); 
            if(iRet < 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"delete dao excute failed.");
                m_pDBLink->Rollback();
                m_vTabDAO[i].m_pDelDAO->ClearArrayData(); 
                break;
            }
            m_vTabDAO[i].m_pDelDAO->ClearArrayData(); 
        }
      
        if(iRet == 0)
        {
        	m_pDBLink->Commit();
        }  
        
        return iRet;
    }

    void TShadowDAO::Clear()
    {
        TADD_FUNC("Start");
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            if(m_vTabDAO[i].m_pInsertDAO == NULL || m_vTabDAO[i].m_pDelDAO == NULL)
            {
                continue;
            }

            m_vTabDAO[i].m_pInsertDAO->ClearArrayData();     
            m_vTabDAO[i].m_pDelDAO->ClearArrayData();   
        }    	

        m_iCurDaoPos = -1;
    }

    void TShadowDAO::ClearDAOByTableName(const char* psTabName)
    {
        TADD_FUNC("Start");
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(m_vTabDAO[i].m_sTabName, psTabName) )
            {
                if(m_vTabDAO[i].m_pInsertDAO == NULL || m_vTabDAO[i].m_pDelDAO == NULL)
                {
                    break;
                }
                
                m_vTabDAO[i].m_pInsertDAO->ClearArrayData();     
                m_vTabDAO[i].m_pDelDAO->ClearArrayData();   
            }
        }
    }

    int TShadowDAO::CreateDAO(const char* psTabName)
    {
        TADD_FUNC("Start.");
        int iRet = -1;

        TADD_DETAIL("Create Dao for Table=[%s]", psTabName);
        char sSql[MAX_SQL_LEN] = {0};

        int iPos = -1;
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            if(0 == strlen(m_vTabDAO[i].m_sTabName))
            {
                m_vTabDAO[i].m_pInsertDAO = new TMdbDAOBase();
                if(NULL == m_vTabDAO[i].m_pInsertDAO)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                    break;
                }
                GenInsertSql(psTabName, sSql, sizeof(sSql));
                m_vTabDAO[i].m_pInsertDAO->SetSQL(sSql);

                m_vTabDAO[i].m_pDelDAO= new TMdbDAOBase();
                if(NULL == m_vTabDAO[i].m_pDelDAO)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                    break;
                }
                GenDeleteSql(psTabName, sSql, sizeof(sSql));
                m_vTabDAO[i].m_pDelDAO->SetSQL(sSql);

                SAFESTRCPY(m_vTabDAO[i].m_sTabName, sizeof(m_vTabDAO[i].m_sTabName), psTabName);
                iPos = i;
                TADD_DETAIL("Create Dao Success, dao pos = %d", iPos);
                break;
            }        
        }
        
        if(iPos == -1)
        {
            TADD_ERROR(ERROR_UNKNOWN,"too many Tables, Max=%d.", MAX_TABLE_COUNTS);
        }

        iRet = iPos;    
        TADD_DETAIL("Ret = [%d]",iRet);
        
        return iRet;
    }

    int TShadowDAO::FindDAO(const char* psTabName)
    {
        TADD_FUNC("Start.");
        int iRet = -1;

        TADD_DETAIL("FindDAO Dao for Table=[%s]", psTabName);

        int iPos = -1;
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            TADD_DETAIL("m_vTabDAO[%d].m_sTabName=[%s]", i ,m_vTabDAO[i].m_sTabName);
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(m_vTabDAO[i].m_sTabName, psTabName))
            {
                iPos = i;
                TADD_DETAIL("Find Dao Success, dao pos = %d", iPos);
                break;
            }        
        }
        
        if(iPos == -1)
        {
            TADD_DETAIL("[%s : %d] : find dao failed, table name=[%s].", __FILE__, __LINE__, psTabName);
        }

        iRet = iPos;    
        TADD_DETAIL("Ret = [%d]",iRet);
        
        return iRet;
    }

    void TShadowDAO::GenInsertSql(const char* psTabName, char*  psSql, int isqllen)
    {
        TADD_FUNC("Start");
        TADD_DETAIL("Table=[%s], Shadow=[%s]",psTabName,m_sShadowTabName);
        char sSQL[MAX_SQL_LEN] = {0};
        snprintf(sSQL, sizeof(sSQL), 
             "insert into %s "
             "(CHANGE_NOTIFY_ID, TABLE_NAME, ACTION_TYPE, UPDATE_TIME, CHILD_TABLE_NAME, KEY1, KEY2, KEY3, KEY4, KEY5,TABLE_SEQUENCE) "
             "values(:CHANGE_NOTIFY_ID, :TABLE_NAME, :ACTION_TYPE, to_date(:UPDATE_TIME,'yyyy-mm-dd hh24:mi:ss'), :CHILD_TABLE_NAME, :KEY1, :KEY2, :KEY3, :KEY4, :KEY5,:TABLE_SEQUENCE)", m_sShadowTabName);

        strncpy(psSql, sSQL, isqllen);
        
    }

    void TShadowDAO::GenDeleteSql(const char* psTabName, char*  psSql, int isqllen)
    {
        TADD_FUNC("Start");
        
        TADD_DETAIL("Table=[%s], NotifyName=[%s]",psTabName,m_sNotifyTabName);
        char sSQL[MAX_SQL_LEN] = {0};
        snprintf(sSQL, sizeof(sSQL), 
             "delete from %s where CHANGE_NOTIFY_ID=:CHANGE_NOTIFY_ID", m_sNotifyTabName);

        strncpy(psSql, sSQL, isqllen);
    }



    TShadowFlushCtrl::TShadowFlushCtrl()
    {
        m_pOraLink = NULL;
        m_pOraQry = NULL;
        m_pCurTabInfo = NULL;
        m_pSelectQry = NULL;
        memset(m_sNotifyTabName, 0, sizeof(m_sNotifyTabName) );
        memset(m_sShadowTabName, 0, sizeof(m_sShadowTabName) );
        memset(m_sNotifySeqTabName, 0, sizeof(m_sNotifySeqTabName));

        
    }

    TShadowFlushCtrl::~TShadowFlushCtrl()
    {
        if(m_pOraLink != NULL)
        {
            m_pOraLink->Disconnect();
            SAFE_DELETE(m_pOraLink);
            SAFE_DELETE(m_pSelectQry);
        }

        TTableNotifyInfo* pInfo = NULL;
        std::vector<TTableNotifyInfo*>::iterator itor = m_vpTabNotifyInfo.begin();
        for(; itor != m_vpTabNotifyInfo.end(); ++itor)
        {
            pInfo = *(itor);
            SAFE_DELETE(pInfo);
        }    
    }

    int TShadowFlushCtrl::Init(const char* psDsnName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(psDsnName);

        TADD_DETAIL("DSN=[%s]", psDsnName);

        iRet = m_tSvrCtrl.CheckDsnCfg(psDsnName, m_tCfgInfo);
        if(iRet < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"no dsn[%s] info found in config file.", psDsnName);
            return -1;
        }
        // connect oracle 
        CHECK_RET(ConnectOracle(),"connect oracle failed.");
        
        // init mdb_change_notif name
        CHECK_RET(GetNotifyTabName(psDsnName),"Get change notify table name failed.DSN:[%s]", psDsnName);

        // init shadow table name
        char sUpperName[MAX_NAME_LEN] = {0};
        SAFESTRCPY(sUpperName, sizeof(sUpperName), psDsnName);
        TMdbNtcStrFunc::ToUpper(sUpperName);
        snprintf(m_sShadowTabName, sizeof(m_sShadowTabName), "QMDB_SHADOW_%s", sUpperName);
        TADD_NORMAL("shadow table name=[%s]", m_sShadowTabName);

        // init dao info
        CHECK_RET(InitDao(),"Init Dao Failed.");

        // attach server shm
        m_pSvrMgr = m_tSvrCtrl.AttachSvrMgr(m_tCfgInfo.m_llMdbDsnId);
        if(NULL == m_pSvrMgr)
        {
            TADD_ERROR(ERROR_UNKNOWN,"attach server shm failed.");
            iRet = -1;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }


    int TShadowFlushCtrl::ConnectOracle()    
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        //如果链接不为空，则判断链接是否正常，如果正常则直接返回，如果不正常，则清空表的query，并清空链接，重新创建链接
        if(m_pOraLink != NULL)
        {
            if(m_pOraLink->IsConnect() == true){return iRet;}
            m_pOraLink->Disconnect();
            SAFE_DELETE(m_pOraLink);
        }
        
        try
        {
            m_pOraLink = TMDBDBFactory::CeatDB();
            CHECK_OBJ(m_pOraLink);
            m_pOraLink->SetLogin(m_tCfgInfo.m_sDBUid, m_tCfgInfo.m_sDBPwd,m_tCfgInfo.m_sDBTns);
            if(m_pOraLink->Connect() == false)
            {
                TADD_ERROR(ERR_DB_NOT_CONNECTED,"Connect Oracle Faild,user=[%s],dsn=[%s].",m_tCfgInfo.m_sDBUid,m_tCfgInfo.m_sDBTns);
                return ERR_DB_NOT_CONNECTED;
            }

            if(NULL != m_pOraQry)
            {
                SAFE_DELETE(m_pOraQry);
            }
            m_pOraQry = m_pOraLink->CreateDBQuery();
            CHECK_OBJ(m_pOraQry);
            if(NULL != m_pSelectQry)
            {
                SAFE_DELETE(m_pSelectQry);
            }
            m_pSelectQry = m_pOraLink->CreateDBQuery();
            CHECK_OBJ(m_pSelectQry);
        }
        catch(TMDBDBExcpInterface &e)
        {
            TADD_ERROR(ERR_DB_NOT_CONNECTED,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            return ERR_DB_NOT_CONNECTED;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  Run()
    * 函数描述	:  将%DSN%_MDB_CHANGE_NOTIF中记录移入影子表QMDB_SHADOW_%DSN%
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0, 失败返回-1;
    * 作者		:  li.ming
    *******************************************************************************/
    int TShadowFlushCtrl::Run()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        char sGetTabSQL[MAX_SQL_LEN] = {0};
        snprintf(sGetTabSQL, sizeof(sGetTabSQL), "select table_name from %s group by table_name",m_sNotifyTabName);
        TADD_DETAIL("sGetTabSQL=[%s]",sGetTabSQL);
        
        char sTabName[MAX_NAME_LEN] = {0};
        m_pSvrMgr->m_bRunFlag = true;
        m_pSvrMgr->m_iProcId = TMdbOS::GetPID();
        
        while(true)
        {
            if(false == m_pSvrMgr->m_bRunFlag)
            {
                TADD_NORMAL("Stop.");
                return 0;
            }

            // oracle重连
            if(0 != ConnectOracle()){TMdbDateTime::Sleep(1);continue;}

            if(GetChangeNotifyCnt() > 0)
            {
                try
                {
                    m_pOraQry->Close();
                    m_pOraQry->SetSQL(sGetTabSQL);
                    m_pOraQry->Open();
                    while(m_pOraQry->Next())
                    {
                        memset(sTabName, 0, sizeof(sTabName));
                        SAFESTRCPY(sTabName, sizeof(sTabName), m_pOraQry->Field("table_name").AsString());
                        TADD_DETAIL("sTabName=[%s]", sTabName);

                        TTableNotifyInfo* pCurTabInfo = GetTableNotifyInfo(sTabName);
                        if(NULL == pCurTabInfo)
                        {
                            iRet = AddNewNotfyTab(sTabName);
                            pCurTabInfo = GetTableNotifyInfo(sTabName);
                            if(NULL == pCurTabInfo )
                            {
                                TADD_ERROR(ERROR_UNKNOWN,"init TTableNotifyInfo failed.table=[%s]", sTabName);
                                continue;
                            }
                            
                        }
                        
                        iRet = DealTable(pCurTabInfo);
                        if(iRet < 0)
                        {
                            iRet = InitDao();
                            if(iRet < 0)
                            {
                                TADD_ERROR(ERROR_UNKNOWN,"rebuild Dao Failed.");
                            }
                            break;
                        }
                    }
                }
                catch(TMDBDBExcpInterface& e)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
                    //snprintf(m_sNotifyTabName,sizeof(m_sNotifyTabName),"MDB_CHANGE_NOTIF");
                }
                catch(...)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"failed : Unknown error!\n");
                    TADD_FUNC("Finish.");
                    return -1;
                }
            }
            else
            {
                TMdbDateTime::Sleep(5);     
            }  
        }

        TADD_FUNC("Finish.");
        return iRet;
    }


    int TShadowFlushCtrl::DealTable(TTableNotifyInfo* pTableInfo)
    {
        TADD_FUNC("Start.");    
        int iRet = 0;
        
        MDB_INT64 iFlushCount = 0;
        m_pCurTabInfo = pTableInfo;
        
        MDB_INT64 iNewSeq = m_pCurTabInfo->m_iLastSeq;
        TADD_NORMAL("Table=[%s],New seq begin with[%lld]", pTableInfo->m_sTabName,iNewSeq);

        TShadowRecord tTmpRecd;

        m_tDao.ClearDAOByTableName(m_pCurTabInfo->m_sTabName);

        try
        {
            m_pSelectQry->Close();
            m_pSelectQry->SetSQL(m_pCurTabInfo->m_sSql);
            m_pSelectQry->Open(MAX_REP_PREFETCH_ROWS);
            while(m_pSelectQry->Next())
            {
                tTmpRecd.Clear();

                tTmpRecd.AddData(m_pSelectQry->Field("CHANGE_NOTIFY_ID"),11);
                tTmpRecd.AddDelData(m_pSelectQry->Field("CHANGE_NOTIFY_ID"),11);
                
                tTmpRecd.AddData(m_pSelectQry->Field("ACTION_TYPE"),2);
                
                tTmpRecd.AddData(m_pSelectQry->Field("TABLE_NAME"),32);
                
                tTmpRecd.AddData(m_pSelectQry->Field("CHILD_TABLE_NAME"),32);
                
                tTmpRecd.AddData(m_pSelectQry->Field("UPDATE_TIME"),15);
                
                tTmpRecd.AddData(m_pSelectQry->Field("KEY1"),128);
                
                tTmpRecd.AddData(m_pSelectQry->Field("KEY2"),128);
                
                tTmpRecd.AddData(m_pSelectQry->Field("KEY3"),128);
                
                tTmpRecd.AddData(m_pSelectQry->Field("KEY4"),128);
                
                tTmpRecd.AddData(m_pSelectQry->Field("KEY5"),128);

                // recaculate table_sequence
                iNewSeq++;
                TADD_DETAIL("New SEQ=[%lld]", iNewSeq);
                tTmpRecd.AddData(m_pSelectQry->Field("TABLE_SEQUENCE"), 11,iNewSeq);

                SAFESTRCPY(tTmpRecd.m_sTabName, sizeof(tTmpRecd.m_sTabName), m_pCurTabInfo->m_sTabName);
                
                iRet = m_tDao.Execute(tTmpRecd);
                
                if(iRet == 0 )
                {
                    iFlushCount++;
                }
                else
                {
                    TADD_ERROR(ERROR_UNKNOWN,"DAO Excute failed.");
                    break;
                }
                
            }

            if(iRet == 0)
            {
                iRet = DaoCommit(m_pCurTabInfo, iNewSeq);
                if(iRet == 0)
                {
                    TADD_FLOW("deal finished  successfully.process count=[%lld] ,save last seq = [%lld]", iFlushCount,m_pCurTabInfo->m_iLastSeq);
                }            
            }
            
        }
        catch(TMDBDBExcpInterface &oe)
        {
            TADD_ERROR(ERROR_UNKNOWN,"TDBException :%s\nSQL=[%s].",  oe.GetErrMsg(), oe.GetErrSql());
            iRet  = -1;
        }
        catch(TBaseException &oe)
        {        
            TADD_ERROR(ERROR_UNKNOWN,"TException :%s.", oe.GetErrMsg());
            iRet  = -1;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Unknow Exception.");
            iRet  = -1;
        }

        TADD_FUNC("Finish.");
        return iRet;
    }


    int TShadowFlushCtrl::GetNotifyTabName(const char* psMdbDsn)
    {
        TADD_FUNC("Start.");    
        int iRet = 0;

        CHECK_OBJ(psMdbDsn);

        char sChangeNotifySQL[MAX_SQL_LEN] = {0};
        snprintf(sChangeNotifySQL, MAX_SQL_LEN,"select TABLE_SEQUENCE from %s_MDB_CHANGE_NOTIF",psMdbDsn);

        char sNotifySeqSQL[MAX_SQL_LEN] = {0};
        snprintf(sNotifySeqSQL, MAX_SQL_LEN,"select TABLE_SEQUENCE from %s_MDB_CHANGE_NOTIFY_SEQ",psMdbDsn);

        try
        {
            m_pOraQry->Close();
            m_pOraQry->SetSQL(sChangeNotifySQL);
            m_pOraQry->Open();
            if(m_pOraQry->Next())
            {
            }
            snprintf(m_sNotifyTabName,sizeof(m_sNotifyTabName),"%s_MDB_CHANGE_NOTIF",psMdbDsn);
        }
        catch(TMDBDBExcpInterface& e)
        {
            TADD_DETAIL("set name as MDB_CHANGE_NOTIF ");
            snprintf(m_sNotifyTabName,sizeof(m_sNotifyTabName),"MDB_CHANGE_NOTIF");
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"failed : Unknown error!\n");
            TADD_FUNC("Finish.");
            return -1;
        }

        TADD_NORMAL("CHANGE_NOTIF table name=[%s]", m_sNotifyTabName);

        try
        {
            m_pOraQry->Close();
            m_pOraQry->SetSQL(sNotifySeqSQL);
            m_pOraQry->Open();
            if(m_pOraQry->Next())
            {
            }
            snprintf(m_sNotifySeqTabName,sizeof(m_sNotifySeqTabName),"%s_MDB_CHANGE_NOTIFY_SEQ",psMdbDsn);
        }
        catch(TMDBDBExcpInterface& e)
        {
            TADD_DETAIL("set name as MDB_CHANGE_NOTIFY_SEQ ");
            snprintf(m_sNotifySeqTabName,sizeof(m_sNotifySeqTabName),"MDB_CHANGE_NOTIFY_SEQ");
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"failed : Unknown error!\n");
            TADD_FUNC("Finish.");
            return -1;
        }

        TADD_NORMAL("CHANGE_NOTIFY_SEQ table name=[%s]", m_sNotifySeqTabName);

        return iRet;
    }

    MDB_INT64 TShadowFlushCtrl::GetChangeNotifyCnt()
    {
        TADD_FUNC("Start.");    
        MDB_INT64 iCnt = 0;

        char sSQL[MAX_SQL_LEN] = {0};
        snprintf(sSQL, MAX_SQL_LEN,"select count(1) counts from %s",m_sNotifyTabName);

        try
        {
            m_pOraQry->Close();
            m_pOraQry->SetSQL(sSQL);
            m_pOraQry->Open();
            if(m_pOraQry->Next())
            {
                iCnt = m_pOraQry->Field("counts").AsInteger();
            }
        }
        catch(TMDBDBExcpInterface& e)
        {
            TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());
            return -1;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"failed : Unknown error!\n");
            return -1;
        }

        TADD_DETAIL("[%s] counts = [%lld]", m_sNotifyTabName, iCnt);

        return iCnt;
    }

    TTableNotifyInfo* TShadowFlushCtrl::GetTableNotifyInfo(const char* psTabName)
    {
        TADD_FUNC("Start.");
        if(NULL == psTabName)
        {
            TADD_ERROR(ERROR_UNKNOWN,"table name is null!");
            return NULL;
        }

        TADD_DETAIL("Table Name=[%s]", psTabName);
        
        std::vector<TTableNotifyInfo*>::iterator itor = m_vpTabNotifyInfo.begin();
        for(; itor != m_vpTabNotifyInfo.end(); ++itor)
        {
            TTableNotifyInfo* pInfo = *(itor);

            if(NULL != pInfo && 0 == TMdbNtcStrFunc::StrNoCaseCmp(pInfo->m_sTabName, psTabName))
            {
                TADD_DETAIL("Get table[%s]'s notify info", psTabName);
                return *(itor);
            }
            else
            {
                continue;
            }
        }

        return NULL;
    }

    int TShadowFlushCtrl::AddNewNotfyTab(const char* psTableName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        if(NULL == psTableName)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Table Name is null!");
            return -1;
        }

        TTableNotifyInfo* pCurTabInfo = new TTableNotifyInfo();
        CHECK_OBJ(pCurTabInfo);

        iRet = pCurTabInfo->Init(m_sNotifyTabName, m_sNotifySeqTabName, m_sShadowTabName
                                                , psTableName, m_pOraQry);
        if(iRet < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"init TTableNotifyInfo failed.table=[%s]", psTableName);
            return -1;
        }

        m_vpTabNotifyInfo.push_back(pCurTabInfo);

        return iRet;
    }

    int TShadowFlushCtrl::InitDao()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        while(true)
        {
            iRet = m_tDao.Init(m_tCfgInfo.m_sDBTns
                                   , m_tCfgInfo.m_sDBUid
                                   , m_tCfgInfo.m_sDBPwd
                                   , m_sShadowTabName
                                   , m_sNotifyTabName);
            
            if(iRet == ERR_APP_CONNCET_ORACLE_FAILED)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Can't Connect to Oracle,Init Dao Faild.");
                TMdbDateTime::Sleep(1);
            }
            else
            {
                break;
            }
        }
        
        return iRet;
    }

    int TShadowFlushCtrl::DaoCommit(TTableNotifyInfo* pTableInfo, const MDB_INT64 iLastSeq)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        iRet = m_tDao.Commit();
        if(iRet < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Dao commit failed.");
            return -1;
        }

        pTableInfo->m_iLastSeq = iLastSeq;
        TADD_FLOW("commit success. save last seq = [%lld]", pTableInfo->m_iLastSeq);
        
        return iRet;
    }

    TTableNotifyInfo::TTableNotifyInfo()
    {
        m_iLastSeq = -1;
        memset(m_sTabName, 0, sizeof(m_sTabName));
        memset(m_sSql, 0, sizeof(m_sSql));
    }

    TTableNotifyInfo::~TTableNotifyInfo()
    {
    }

    int TTableNotifyInfo::Init(const char* psNotifyName
                                         ,const char* psNotifySeqName
                                         ,const char* psShadowTabName
                                         ,const char* psTableName
                                         ,TMDBDBQueryInterface* pOraQry)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(psNotifyName);
        CHECK_OBJ(psNotifySeqName);
        CHECK_OBJ(psTableName);
        CHECK_OBJ(psShadowTabName);
        CHECK_OBJ(pOraQry);
        
        TADD_DETAIL("Table Name=[%s], notify table name=[%s], notify_seq table name=[%s], ShadowTab=[%s]"
                                ,psTableName, psNotifyName, psNotifySeqName, psShadowTabName);

        snprintf(m_sSql, sizeof(m_sSql), "SELECT CHANGE_NOTIFY_ID, ACTION_TYPE, TABLE_NAME, CHILD_TABLE_NAME, UPDATE_TIME,TABLE_SEQUENCE, KEY1, KEY2, KEY3, KEY4,KEY5 "
                                                        "FROM %s WHERE upper(table_name) = upper('%s') ORDER BY  TABLE_SEQUENCE",psNotifyName,psTableName);

        TADD_FLOW("SQL:[%s]", m_sSql);

        SAFESTRCPY(m_sTabName, sizeof(m_sTabName), psTableName);

        m_iLastSeq = GetLastSeq(psNotifyName, psNotifySeqName,psShadowTabName,psTableName, pOraQry);
        TADD_NORMAL("last seq=%lld", m_iLastSeq);
        
        return iRet;
    }

    /*
    获取逻辑:
    1 先取影子表，若影子表有记录，则取影子表中最大的table_sequence，并返回；
    2 若影子表无记录，则取change_notify_seq表的该表的最大table_sequence
    3 若change_notify_seq表无记录，则取change_notif表中最小的table_sequence
    4 若change_notif表无记录，则返回0
    */
    MDB_INT64 TTableNotifyInfo::GetLastSeq(const char* psNotifyName 
                                        ,const char* psNotifySeqName 
                                        ,const char* psShadowTabName
                                        ,const char* psTableName
                                        ,TMDBDBQueryInterface* pOraQry)
    {
        TADD_FUNC("Start.");
        MDB_INT64 iLastSeq = 0;
        MDB_INT64 iCurSeq = 0;

        iLastSeq = GetMaxSeqFromShadowTab(psTableName, psShadowTabName,pOraQry);

        if(iLastSeq < 0)
        {
            iLastSeq = GetSeqFromNotifySeqTab(psTableName,psNotifySeqName, pOraQry);
            if(iLastSeq < 0)
            {
                iCurSeq = GetMinSeqFromNotifyTab(psTableName, psNotifyName, pOraQry);
                if(iCurSeq <= 0)
                {
                    iLastSeq = 0;
                }
                else
                {
                    iLastSeq =  iCurSeq -1;
                }
            }
        }
        return iLastSeq;
    }

    MDB_INT64 TTableNotifyInfo::GetMaxSeqFromShadowTab(const char* psTableName,const char* psShadowTabName,TMDBDBQueryInterface* pOraQry)
    {
        TADD_FUNC("Start.");
        MDB_INT64 iLastSeq = -1;

        char sSQL[MAX_SQL_LEN] = {0};
        snprintf(sSQL, MAX_SQL_LEN,"select nvl(max(TABLE_SEQUENCE),-1) MAX_TABLE_SEQUENCE from %s where lower(table_name) = lower('%s') ",psShadowTabName,psTableName);

        try
        {
            pOraQry->Close();
            pOraQry->SetSQL(sSQL);
            pOraQry->Open();
            if(pOraQry->Next())
            {
                iLastSeq = pOraQry->Field("MAX_TABLE_SEQUENCE").AsInteger();
            }
        }
        catch(TMDBDBExcpInterface& e)
        {
            TADD_ERROR(ERROR_UNKNOWN,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"failed : Unknown error!\n");
        }

        TADD_NORMAL("Table:[%s] max table_sequence from [%s] = [%lld]", psTableName, psShadowTabName, iLastSeq);

        return iLastSeq;
    }

    MDB_INT64 TTableNotifyInfo::GetSeqFromNotifySeqTab(const char* psTableName,const char* psNotifySeqName, TMDBDBQueryInterface* pOraQry)
    {
        TADD_FUNC("Start.");
        MDB_INT64 iLastSeq = -1;

        char sSQL[MAX_SQL_LEN] = {0};
        snprintf(sSQL, MAX_SQL_LEN,"select nvl(max(TABLE_SEQUENCE),-1) MAX_TABLE_SEQUENCE  from %s where  lower(table_name) = lower('%s') ",psNotifySeqName,psTableName);

        try
        {
            pOraQry->Close();
            pOraQry->SetSQL(sSQL);
            pOraQry->Open();
            if(pOraQry->Next())
            {
                iLastSeq = pOraQry->Field("MAX_TABLE_SEQUENCE").AsInteger();
            }
        }
        catch(TMDBDBExcpInterface& e)
        {
            TADD_ERROR(ERROR_UNKNOWN,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"failed : Unknown error!\n");
        }

        TADD_NORMAL("Table:[%s] , table_sequence from [%s] = [%lld]", psTableName, psNotifySeqName, iLastSeq);

        return iLastSeq;
    }

    MDB_INT64 TTableNotifyInfo::GetMinSeqFromNotifyTab(const char* psTableName,const char* psNotifyName,TMDBDBQueryInterface* pOraQry)
    {
        TADD_FUNC("Start.");
        MDB_INT64 iMinSeq = -1;

        char sSQL[MAX_SQL_LEN] = {0};
        snprintf(sSQL, MAX_SQL_LEN,"select nvl(min(TABLE_SEQUENCE),-1) MIN_TABLE_SEQUENCE from %s where lower(table_name) = lower('%s') ",psNotifyName,psTableName);

        try
        {
            pOraQry->Close();
            pOraQry->SetSQL(sSQL);
            pOraQry->Open();
            if(pOraQry->Next())
            {
                iMinSeq = pOraQry->Field("MIN_TABLE_SEQUENCE").AsInteger();
            }
        }
        catch(TMDBDBExcpInterface& e)
        {
            TADD_ERROR(ERROR_UNKNOWN,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"failed : Unknown error!\n");
        }

        TADD_NORMAL("Table:[%s]  ,min table_sequence from [%s] = [%lld]", psTableName, psNotifyName, iMinSeq);

        return iMinSeq;
    }

//}
