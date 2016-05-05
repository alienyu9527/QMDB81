/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbOraFlushMdb.cpp     
*@Description��oracle->mdb ������ˢ��
*@Author:       zhang.lin
*@Date��        2012��2��13��
*@History:
******************************************************************************************/
#include "Tools/mdbDbFlushMdb.h"
#include "Helper/mdbDateTime.h"

//namespace QuickMDB{


    TMdbDbFlushMdb::TMdbDbFlushMdb()
    {
        m_pConfig = NULL;
        m_pDBLink = NULL;
        m_pTable  = NULL;
        m_pOraQuery  = NULL;
        m_pMQuery  = NULL;
        m_pMInsert  = NULL;
        m_pMSelect  = NULL;
        m_iDiffRecords  = 0;
    }


    TMdbDbFlushMdb::~TMdbDbFlushMdb()
    {   
        SAFE_DELETE(m_pDBLink);
        SAFE_DELETE(m_pOraQuery);
    }
    /******************************************************************************
    * ��������  :  InitLoadCfg()
    * ��������  :  ��ʼ�������������ļ�
    * ����      :  pszDSN,  dsn��  
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻�-1
    * ����      :  zhang.lin
    *******************************************************************************/
    int TMdbDbFlushMdb::InitLoadCfg(const char* pszDSN)
    {
        TADD_FUNC("TMdbDbFlushMdb::InitLoadCfg(%s) : Begin.", pszDSN);
        int iRet =0;
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        CHECK_OBJ(m_pConfig);
        TADD_FUNC("TMdbDbFlushMdb::InitLoadCfg(%s) : Finish.", pszDSN);
        return iRet;
    }
    /******************************************************************************
    * ��������  :  CheckTableRepAttr()
    * ��������  :  ��������
    * ����      :  pszTableName,  ����  
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻ش�����
    * ����      :  zhang.lin
    *******************************************************************************/
    int TMdbDbFlushMdb::CheckTableRepAttr(const char* pszTableName)
    {
        TADD_FUNC("TMdbDbFlushMdb::CheckTableRepAttr(%s) : Begin.", pszTableName);
        int iRet = 0;
        if(m_pConfig == NULL)
        {
            TADD_ERROR(-1,"Can't find Config-file."); 
            return ERR_APP_CONFIG_NOT_EXIST;    
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pszTableName,"all") == 0 )
        {
            return iRet;
        }
        else if(TMdbNtcStrFunc::FindString(pszTableName,"dba_") >= 0 )
        {
            TADD_ERROR(-1,"Table [%s] : ERR_TAB_TABLEid_INVALID.",pszTableName);
            return ERR_TAB_TABLEID_INVALID;
        }
        else
        {
            TMdbTable* pTable = m_pConfig->GetTable(pszTableName);
            if(NULL == pTable)
            {
                TADD_ERROR(-1,"Table [%s] does not exist in config.",pszTableName);
                return ERR_TAB_CONFIG_TABLENAME_NOT_EXIST;
            } 
            else//�����ͬ������
            {
                
                if(pTable->iRepAttr != REP_FROM_DB)
                {
                    TADD_ERROR(-1,"Table RepAttr error(not From_Ora) in table[%s].",pszTableName);
                    return ERR_APP_DATA_TYPE_INVALID;
                }
            }
        }
        TADD_FUNC("TMdbDbFlushMdb::CheckTableRepAttr(%s) : Finish.", pszTableName); 
        return iRet;
    }

    /******************************************************************************
    * ��������  :  Init()
    * ��������  :  ��ʼ��,����ͬ�����ԡ�����Oracle��minidb  ��
    * ����      :  pszDSN, �ڴ����ݿ��DSN����
    * ����      :  pszTableName, ����or all
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻�-1
    * ����      :  zhang.lin
    *******************************************************************************/
    int TMdbDbFlushMdb::Init(const char* pszDSN ,const char* pszTableName)
    {
        TADD_FUNC("Begin.");
        
        int iRet  = -1;
        iRet = InitLoadCfg(pszDSN);
        if(iRet < 0)
        {
            TADD_ERROR(-1,"Init(%s) failed.",pszDSN); 
            return iRet;
        }
        //����ͬ������
        iRet = CheckTableRepAttr(pszTableName);
        if(iRet < 0)
        {
            return iRet;
        }
        memset(m_sFileName, 0, sizeof(m_sFileName));
        //��ȡ�ȶ��ļ�·��
        strncpy(m_sFileName, m_pConfig->GetDSN()->checkDataByOra, MAX_PATH_NAME_LEN-1);
        if(m_sFileName[0] == '\0' )
        {
            TADD_ERROR(-1,"Config file Not find CHECK_DATA_BY_ORA in [%s].", pszDSN);  
            return ERR_APP_CONFIG_ITEM_NOT_EXIST;  
        }

        char sUID[32], sPWD[32], sDSN[32];
        memset(sUID, 0, sizeof(sUID));
        memset(sPWD, 0, sizeof(sPWD));
        memset(sDSN, 0, sizeof(sDSN)); 
        //����DSNȡ����Ӧ��Oracle���û���������
        iRet = GetOraUser(sDSN, sUID, sPWD);
        if(iRet < 0)
        {        
            return iRet; 
        }     
        //��¼Oracle
        try
        {
            m_pDBLink = TMDBDBFactory::CeatDB();
            m_pDBLink->SetLogin(sUID, sPWD, sDSN);
            if(m_pDBLink->Connect() == false)
                iRet = ERR_APP_CONNCET_ORACLE_FAILED;

            m_pOraQuery = m_pDBLink->CreateDBQuery();
        }
        catch(TMDBDBExcpInterface &e)
        {        
            TADD_ERROR(-1,"Can't connect Oracle. DSN=%s, UID=%s, PWD=%s.",sDSN, sUID, sPWD);
            return ERROR_UNKNOWN;    
        }
        
        //����DSNȡ����Ӧ��MDB���û���������
        iRet = GetMDBUser(pszDSN, sUID, sPWD);
        if(iRet < 0)
        {        
            return iRet; 
        }       
        try
        {
            //��¼MDB
            m_tDB.SetLogin(sUID, sPWD, pszDSN);
            if(m_tDB.Connect() == false)
            {
                TADD_ERROR(-1,"Can't connect to QuickMDB.");
                return ERR_DB_LOCAL_CONNECT;
            }
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(-1,"Can't connect to QuickMDB.");
            return ERROR_UNKNOWN;
        }
            
        TADD_FUNC("Finish.");
        
        return iRet;
    }

    int TMdbDbFlushMdb::GetMDBUser(const char* pszDSN, char* pszUID, char* pszPWD)
    {
        TADD_FUNC("Begin."); 
        int iRet = 0;
        int iUserCounts = m_pConfig->GetUserCounts();
        TMDbUser* pUser = NULL;
        bool bFind = false;
        for(int i=0;i<iUserCounts;i++)
        {
            pUser = m_pConfig->GetUser(i);
            if(pUser == NULL)
            {
                TADD_ERROR(-1,"Can't GetUser().");
                return ERR_DB_USER_INVALID;  
            }
            if(TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess,"Administrator") == 0 ||  TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess,"Read-Write") == 0 )
            {
                bFind = true;
                break;
            }
        }
        if(!bFind)
        {
            TADD_ERROR(-1,"GetMDBUser Error.");
            return ERR_DB_USER_INVALID;  
        }
        SAFESTRCPY(pszUID,sizeof(pszUID),pUser->sUser);
        SAFESTRCPY(pszPWD,sizeof(pszPWD),pUser->sPwd);
        
        TADD_FUNC("GetMDBUser(%s/%s@%s) : Finish.", pszUID, pszPWD,pszDSN);    
        return iRet;
    }


    int TMdbDbFlushMdb::GetOraUser(char* pszDSN, char* pszUID, char* pszPWD)
    {
        TADD_FUNC("Begin.");
        
        int iRet = 0;
        TMdbCfgDSN* pDSN = m_pConfig->GetDSN();
        if(pDSN == NULL)
        {
            TADD_ERROR(-1,"Can't GetDSN().");
            return ERR_DB_DSN_INVALID;  
        }
        
        strcpy(pszUID, pDSN->sOracleUID);
        strcpy(pszDSN, pDSN->sOracleID);
        strcpy(pszPWD, pDSN->sOraclePWD);

        
        TADD_FUNC("GetOraUser(%s/%s@%s) : Finish.", pszUID, pszPWD, pszDSN);    
        return iRet;
    }
    int  TMdbDbFlushMdb::GenFlushDataSQL(TMdbTable* pTable,char* sMSQL,char* sOraSelectSQL)
    {
        strcpy(sOraSelectSQL,"select ");
        bool bFlag = false;
        for(int j=0; j<pTable->iColumnCounts; ++j)
        {
            if(bFlag == false)
            {
                sprintf(sOraSelectSQL+strlen(sOraSelectSQL),"%s", pTable->tColumn[j].sName);
                bFlag = true;
            }
            else
            {
                sprintf(sOraSelectSQL+strlen(sOraSelectSQL),", %s",pTable->tColumn[j].sName) ; 
            }
        }//end for(int j=0; j<pTable->iColumnCounts; ++j)
        if(bFlag == false)
        {
            TADD_FLOW("Table=[%s], No-Column need to check", pTable->sTableName);
            return -1;
        }
        sprintf(sOraSelectSQL+strlen(sOraSelectSQL)," from %s", pTable->sTableName);
        strcpy(sMSQL,sOraSelectSQL);

        //Oracle��SQL�Ѿ�ȷ����Ȼ��Ҫȷ��MDB��SQL��where �� ������Ϣ
        for(int j=0; j<(pTable->m_tPriKey).iColumnCounts; ++j)
        {
            int iNo = pTable->m_tPriKey.iColumnNo[j];
            if(j == 0)
            {
                sprintf(sMSQL+strlen(sMSQL)," where %s=:%s", pTable->tColumn[iNo].sName, pTable->tColumn[iNo].sName);
            }
            else
            {
                sprintf(sMSQL+strlen(sMSQL)," and %s=:%s", pTable->tColumn[iNo].sName, pTable->tColumn[iNo].sName);
            }
        }//end for(int j=0; j<(pTable->m_tPriKey).iColumnCounts; ++j)
        //�����load-sql��������load-sql
        if (strlen(pTable->m_sLoadSQL)>0 && pTable->iLoadType == 1)
        {
            TADD_NORMAL("Load SQL:\n%s",pTable->m_sLoadSQL);
            memset(sOraSelectSQL,0,sizeof(sOraSelectSQL));
            strcpy(sOraSelectSQL,pTable->m_sLoadSQL);
        }
        return 0;
    }
    int  TMdbDbFlushMdb::FlushAllTab()
    {
        int iRet = 0;
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            iRet = -1;
            //ȡ������Ϣ
            TMdbTable* pTable = m_pConfig->GetTableByPos(i);
            if(pTable == NULL)
            {
                continue;	
            }
            if(pTable->iRepAttr != REP_FROM_DB)
            {
                continue;
            }
            iRet = 0;
            TADD_NORMAL("Flushing table=[%s] start.", pTable->sTableName);
            FlushData(pTable->sTableName);
        }   
        return iRet;
    }
    TMdbTable*  TMdbDbFlushMdb::GetTable(const char* pszTable)
    {
        TMdbTable* pTable = NULL;
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            //ȡ������Ϣ
            pTable = m_pConfig->GetTableByPos(i);
            if(pTable == NULL)
            {
                continue;
            }
            if(pszTable != NULL)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pszTable, pTable->sTableName) == 0)
                {
                    break;
                }
            }
        }
        return pTable;
    }
    void TMdbDbFlushMdb::SetComplexParam(TMdbTable* pTable)
    {   
        if(pTable->iParameterCount != 0  && pTable->iLoadType == 1)
        {
            for(int i = 0; i<pTable->iParameterCount; i++)
            {
                if(pTable->tParameter[i].iDataType == DT_Int && pTable->tParameter[i].iParameterType == 0)
                {
                    m_pOraQuery->SetParameter(pTable->tParameter[i].sName,TMdbNtcStrFunc::StrToInt(pTable->tParameter[i].sValue));
                }
                else if(pTable->tParameter[i].iParameterType == 0)
                {
                    m_pOraQuery->SetParameter(pTable->tParameter[i].sName,pTable->tParameter[i].sValue);
                }
            }
        }
    }

    /******************************************************************************
    * ��������  :  FlushData()
    * ��������  :  У�������,����oracleˢ��
    * ����      :  pszTable,����
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻�-1
    * ����      :  zhang.lin
    *******************************************************************************/
    int TMdbDbFlushMdb::FlushData(const char* pszTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_iDiffRecords = 0;
        if(TMdbNtcStrFunc::StrNoCaseCmp(pszTable,"all") == 0 )
        {
            if(FlushAllTab() != 0)
            {
                return iRet;
            }
        }
        TMdbTable* pTable = GetTable(pszTable);
        CHECK_OBJ(pTable);
        //�Ա��ļ���ʽ /ztesoft/ocsocsv70f/data/r12/check_data_ora/tabletest
        memset(m_sFileName, 0, sizeof(m_sFileName));
        //��ȡ�ȶ��ļ�·��
        snprintf(m_sFileName,sizeof(m_sFileName),"%s/",m_pConfig->GetDSN()->checkDataByOra);
        if(m_sFileName[0] == '\0' )
        {
            TADD_ERROR(-1,"Config file Not find CHECK_DATA_BY_ORA.");  
            return ERR_APP_CONFIG_ITEM_NOT_EXIST;  
        }
        bool bExist = TMdbNtcDirOper::IsExist(m_sFileName);
        if(!bExist)
        {
            if(!TMdbNtcDirOper::MakeFullDir(m_sFileName))
            {
                TADD_ERROR(-1,"Can't create dir=[%s].",m_sFileName);
                return ERR_OS_CREATE_DIR;
            }
        }
        snprintf(m_sFileName+strlen(m_sFileName),sizeof(m_sFileName)-strlen(m_sFileName),"/%s",pTable->sTableName); 
        TADD_FLOW("Checking table=[%s]......", pTable->sTableName);
        //����ҪƴдSQL���Ա��ѯ�Ա� sOraSelectSQL: ORACLE select SQL sMSQL:QMDB SQL,������������sql
        char sOraSelectSQL[MAX_SQL_LEN];
        char sMSQL[MAX_SQL_LEN];
        memset(sOraSelectSQL, 0, sizeof(sOraSelectSQL));
        memset(sMSQL, 0, sizeof(sMSQL));
        CHECK_RET(GenFlushDataSQL(pTable,sMSQL,sOraSelectSQL),"GenFlushDataSQL ERROR.");    
        TADD_FLOW("TMdbDbFlushMdb::CheckData() : \n Oracle-SQL=%s.\n QMDB-SQL=%s.", sOraSelectSQL, sMSQL);
        try
        {
            TMdbQuery* pQuery = m_tDB.CreateDBQuery();
            CHECK_OBJ(pQuery);
            //qmdbִ�в�ѯ����
            pQuery->CloseSQL();
            pQuery->SetSQL(sMSQL);	
            //ORACLEִ�в�ѯ����	
            m_pOraQuery->Close();
            m_pOraQuery->SetSQL(sOraSelectSQL);	
            //����Ǹ���SQL������Ҫ���ò���,load-type 0�ݲ�֧�֡�
            SetComplexParam(pTable);
            m_pOraQuery->Open();
            while(m_pOraQuery->Next())
            {
                //�Ѳ������ȥ
                for(int j=0; j<(pTable->m_tPriKey).iColumnCounts; ++j)
                {
                    int iNo = pTable->m_tPriKey.iColumnNo[j];
                    if(pTable->tColumn[iNo].iDataType == DT_Int)
                    {
                        pQuery->SetParameter(pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsInteger());
                    }
                    else if(pTable->tColumn[iNo].iDataType == DT_DateStamp)
                    {
                        pQuery->SetParameter(pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsDateTimeString());
                    }
                    else if(pTable->tColumn[iNo].iDataType == DT_Char)
                    {
                        char sValue[2048];
                        memset(sValue, 0, sizeof(sValue));
                        strcpy(sValue,m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                        TMdbNtcStrFunc::TrimRight(sValue);
                        pQuery->SetParameter(pTable->tColumn[iNo].sName,sValue);
                    }
                    else
                    {
                        pQuery->SetParameter(pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                    }
                }//end for(int j=0; j<(pTable->m_tPriKey).iColumnCounts; ++j)
                pQuery->Open();//qmdb��ѯ����open
                if(pQuery->Next())
                {
                    bool bSame = CheckSame(pTable,pQuery)	;
                    if(bSame == false)
                    {
                        //��¼��һ�µ�����
                        RecordData(pTable);
                    }   
                }//end if(pQuery->Next())
                else
                {
                    //��¼��һ�µ�����
                    RecordData(pTable);
                }
            }
            if(m_iDiffRecords > 0)
            {
                TADD_NORMAL("Find %d different records in table[%s].",m_iDiffRecords,pszTable);
                //�ָ�����
                Restore(pszTable);
                //ɾ����ʱ�ļ�
                TMdbNtcFileOper::Remove(m_sFileName);
            }
            else
            {
                TADD_NORMAL("Find %d different records in table[%s],no data need to flush.",m_iDiffRecords,pszTable);
            }
        }
        catch(TMdbException &e)
        {        
            TADD_ERROR(-1,"QuickMDB-SQL=[%s], error_msg=[%s].", e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;    
        }
        catch(TMDBDBExcpInterface &e)
        {        
            TADD_ERROR(-1,"Oracle-SQL=[%s], error_msg=[%s].", e.GetErrSql(), e.GetErrMsg());
            return ERR_SQL_INVALID;    
        }
        TADD_FUNC("Finish.");
        return 0;
    }
    bool  TMdbDbFlushMdb::CheckSame(TMdbTable* pTable,TMdbQuery* pQuery)
    {
        bool bSame = true;
        for(int j=0; j<pTable->iColumnCounts; ++j)
        {
            if(pTable->tColumn[j].iDataType == DT_Int)
            {
                if(m_pOraQuery->Field(pTable->tColumn[j].sName).AsInteger() != pQuery->Field(pTable->tColumn[j].sName).AsInteger())
                {
                    bSame = false;
                    break;
                }
            }
            else if(pTable->tColumn[j].iDataType == DT_Char )
            {
                int iOraLen = strlen(m_pOraQuery->Field(pTable->tColumn[j].sName).AsString());
                int iMdbLen = strlen(pQuery->Field(pTable->tColumn[j].sName).AsString());
                char sNullSt[1024];
                memset(sNullSt,0,sizeof(sNullSt));
                strcpy(sNullSt," ");
                for(int i=0;i<iOraLen-iMdbLen-1;i++)
                {
                    strcat(sNullSt," ");
                }
                if(iMdbLen == iOraLen)
                {
                    if(strcmp(m_pOraQuery->Field(pTable->tColumn[j].sName).AsString(), pQuery->Field(pTable->tColumn[j].sName).AsString()) != 0)
                    {
                        bSame = false;
                        break;
                    }
                }
                else if(iMdbLen < iOraLen)
                {
                    if(TMdbNtcStrFunc::FindString(m_pOraQuery->Field(pTable->tColumn[j].sName).AsString(),pQuery->Field(pTable->tColumn[j].sName).AsString()) == -1 || TMdbNtcStrFunc::FindString(m_pOraQuery->Field(pTable->tColumn[j].sName).AsString(),sNullSt) == -1)
                    {
                        bSame = false;
                        break;
                    }
                }
                else //iMdbLen > iOraLen
                {
                    bSame = false;
                    break;
                }
            }
            else 
            {
                if(strcmp(m_pOraQuery->Field(pTable->tColumn[j].sName).AsString(), pQuery->Field(pTable->tColumn[j].sName).AsString()) != 0)
                {
                    bSame = false;
                    break;
                }
            }
        }
        return bSame;   
    }
    //��¼��һ�µ�����
    void TMdbDbFlushMdb::RecordData(TMdbTable* pTable)
    {
        TADD_FUNC("Start.");	
        FILE* fp = fopen(m_sFileName, "a+");
        if(fp == NULL)
        {
            TADD_ERROR(-1,"Can't open file=[%s].", m_sFileName);
            return;
        }

        char sData[2048];
        char sDataTemp[2048];
        memset(sData, 0, sizeof(sData));
        memset(sDataTemp,0,sizeof(sDataTemp));
        sprintf(sData, "%s,", pTable->sTableName);
        sprintf(sDataTemp, "%s", "|");

        //�����м�¼
        for(int j=0; j<(pTable->m_tPriKey).iColumnCounts; ++j)
        {
            int iNo = pTable->m_tPriKey.iColumnNo[j];
            if(j == 0)
            {
                if(pTable->tColumn[iNo].iDataType == DT_Int)
                {
                    sprintf(sData+strlen(sData)," %s=%lld", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsInteger());
                    sprintf(sDataTemp+strlen(sDataTemp)," %s=%lld", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsInteger());
                }
                else
                {
                    if(pTable->tColumn[iNo].iDataType == DT_DateStamp)
                    {
                        sprintf(sData+strlen(sData), " %s='%s'", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                        sprintf(sDataTemp+strlen(sDataTemp), " %s=to_date('%s','yyyy-mm-dd hh24:mi:ss')", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                    }
                    else if(pTable->tColumn[iNo].iDataType == DT_Char)
                    {
                        char sValue[4096];
                        memset(sValue, 0, sizeof(sValue));
                        strcpy(sValue,m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                        TMdbNtcStrFunc::TrimRight(sValue);
                        sprintf(sData+strlen(sData), " %s='%s'", pTable->tColumn[iNo].sName, sValue);
                        sprintf(sDataTemp+strlen(sDataTemp), " %s='%s'", pTable->tColumn[iNo].sName, sValue);
                    }
                    else
                    {
                        sprintf(sData+strlen(sData), " %s='%s'", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                        sprintf(sDataTemp+strlen(sDataTemp), " %s='%s'", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                    }
                }
            }
            else
            {
                if(pTable->tColumn[iNo].iDataType == DT_Int)
                {
                    sprintf(sData+strlen(sData), " and %s=%lld", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsInteger());
                    sprintf(sDataTemp+strlen(sDataTemp), " and %s=%lld", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsInteger());
                }
                else
                {
                    if(pTable->tColumn[iNo].iDataType == DT_DateStamp)
                    {
                        sprintf(sData+strlen(sData), " and %s='%s'", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                        sprintf(sDataTemp+strlen(sDataTemp), " and %s=to_date('%s','yyyy-mm-dd hh24:mi:ss')", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                    }
                    else if(pTable->tColumn[iNo].iDataType == DT_Char)
                    {
                        char sValue[2048];
                        memset(sValue, 0, sizeof(sValue));
                        strcpy(sValue,m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                        TMdbNtcStrFunc::TrimRight(sValue);
                        sprintf(sData+strlen(sData), " and %s='%s'", pTable->tColumn[iNo].sName, sValue);
                        sprintf(sDataTemp+strlen(sDataTemp), " and %s='%s'", pTable->tColumn[iNo].sName, sValue);
                    }
                    else
                    {
                        sprintf(sData+strlen(sData), " and %s='%s'", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                        sprintf(sDataTemp+strlen(sDataTemp), " and %s='%s'", pTable->tColumn[iNo].sName, m_pOraQuery->Field(pTable->tColumn[iNo].sName).AsString());
                    }      

                }
            }

        }
        strcat(sData," ");
        strcat(sData,sDataTemp);
        //���ɻس�
        strcat(sData,"\n");
        sData[strlen(sData)] = '\0';
        fputs(sData,fp);
        SAFE_CLOSE(fp);
        ++m_iDiffRecords;
        TADD_FUNC("Finish.");
    }

    /******************************************************************************
    * ��������  :  Restore()
    * ��������  :  �ָ�����  
    * ����      :  pszTable ����
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻�-1
    * ����      :  zhang.lin
    *******************************************************************************/
    int TMdbDbFlushMdb::Restore(const char* pszTable)
    {
        TADD_FUNC("TMdbDbFlushMdb::Restore() : Start.");
        char sLogTime[32];
        memset(sLogTime,0,sizeof(sLogTime));
        TMdbDateTime::GetCurrentTimeStr(sLogTime);
        TADD_NORMAL("Flush start time:[%s]",sLogTime) ;
        //int iRet = 0;
        m_iDiffRecords = 0;
        //���ָ��ļ��Ƿ����
        bool bExist = TMdbNtcFileOper::IsExist(m_sFileName);
        if(bExist == true)
        {
            FILE* fp = fopen(m_sFileName, "r");
            if(fp == NULL)
            {
                TADD_ERROR(-1,"Can't open file=%s, errno=%d, errmsg=%s.", m_sFileName, errno, strerror(errno));
                return ERR_OS_NO_MEMROY;
            }
            //һ�����ļ�¼������
            char sMsg[1024];
            bool bFlag = false;
            while(fgets(sMsg, sizeof(sMsg), fp) != NULL)
            {
                TADD_FLOW("TMdbDbFlushMdb::Restore() : records=[%s].", sMsg);
                if(pszTable == NULL)
                {
                    bFlag = RestoreData(sMsg);
                }
                else
                {
                    bFlag = RestoreData(sMsg,pszTable);
                }
                if(bFlag == true)
                {
                    ++m_iDiffRecords;
                }
            }
            SAFE_CLOSE(fp);
        }
        else
        {
            TADD_ERROR(-1,"No file=[%s].",m_sFileName);
        }

        TADD_NORMAL("   Flush %d different records, in table[%s].",m_iDiffRecords, pszTable);
        TADD_FUNC("TMdbDbFlushMdb::Restore() : Finish.");
        memset(sLogTime,0,sizeof(sLogTime));
        TMdbDateTime::GetCurrentTimeStr(sLogTime);
        TADD_NORMAL("Flush end time:[%s]",sLogTime) ;
        return 0;
    }

    bool TMdbDbFlushMdb::RestoreData(const char* pszMsg,const char* pszTable)
    {
        TADD_FUNC("Start.");
        char sTableName[MAX_NAME_LEN];
        memset(sTableName, 0, sizeof(sTableName));
        int iPos = 0;
        int iPosOra = 0;
        iPos = GetPosTabName(sTableName,pszMsg);
        iPosOra = GetPosOra(pszMsg);
        if(pszTable != NULL)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(sTableName, pszTable) !=0)
            {
                return false;   //Ĭ�ϲ���Ҫ�ָ�
            }
        }
        TADD_DETAIL("pszMsg=[%s], sTableName=[%s].", pszMsg, sTableName);
        TMdbTable* pTable = NULL;
        pTable = GetMdbTable(sTableName);
        //���û���ҵ�����,���¼����
        if(pTable == NULL)
        {
            TADD_ERROR(-1,"No-Table=[%s], pszMsg=[%s]",sTableName, pszMsg);
            return false;
        }
        TADD_DETAIL("Restore table=[%s]......", pTable->sTableName);
        //����ҪƴдSQL��Oracle-��ѯ, QMDB-����
        char sSQL[MAX_SQL_LEN];
        char sMSQL[MAX_SQL_LEN];
        char sMInsertSql[MAX_SQL_LEN];
        char sMSelectSql[MAX_SQL_LEN];	
        memset(sSQL, 0, sizeof(sSQL));
        memset(sMSQL, 0, sizeof(sMSQL));
        memset(sMInsertSql, 0, sizeof(sMInsertSql));
        memset(sMSelectSql, 0, sizeof(sMSelectSql));
        bool bisAllowUpdate = true;
        int iRet = GetSQL(pTable,sSQL,sMSQL,sMSelectSql,bisAllowUpdate);
        if(iRet != 0)
        {
            TADD_ERROR(-1,"GetSQL ERROR.");
            return false;
        }
        CHECK_RET(GetInsertSQL(pTable,sMInsertSql,sizeof(sMInsertSql)),"GetInsertSQL failed.");
        char caTemp[512];
        memset(caTemp,0,512);
        strncpy(caTemp,&pszMsg[iPos],iPosOra-iPos-1);
        //mdb update sql : sMSQL
        sprintf(sMSQL+strlen(sMSQL), " where %s",caTemp);
        //mdb select sql : sMSelectSql
        sprintf(sMSelectSql+strlen(sMSelectSql), " from %s where %s", pTable->sTableName,caTemp);
        sprintf(sSQL+strlen(sSQL)," from %s where %s",pTable->sTableName,&pszMsg[iPosOra]);// oracle selct sql
        TADD_FLOW("\n\tOracle-SQL=%s.\n\tQMDB-UP-SQL=%s.", sSQL, sMSQL);
        TADD_FLOW("\n\tMDB_SElECT-SQL=%s.\n\tMDB_INSERT-SQL=%s.", sMSelectSql, sMInsertSql);
        try
        {
            m_pMQuery = m_tDB.CreateDBQuery();
            m_pMInsert = m_tDB.CreateDBQuery();
            m_pMSelect = m_tDB.CreateDBQuery();
            if(m_pMQuery == NULL || m_pMSelect== NULL || m_pMInsert == NULL)
            {
                TADD_ERROR(-1,"Can't CreateDBQuery().");
                TADD_ERROR(-1,"pszMsg=[%s]",pszMsg);
                return false;     
            }
            //ִ�в�ѯ����
            m_pMQuery->CloseSQL();
            m_pMInsert->CloseSQL();
            m_pMSelect->CloseSQL();
            if(bisAllowUpdate)
            {
                m_pMQuery->SetSQL(sMSQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH,0);
            }
            m_pMInsert->SetSQL(sMInsertSql,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH,0);
            m_pMSelect->SetSQL(sMSelectSql,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH,0);
            m_pOraQuery->Close();
            m_pOraQuery->SetSQL(sSQL);
            m_pOraQuery->Open();
            m_pMSelect->Open();
            while(m_pOraQuery->Next())
            {
                //��update��insert�������ȥ
                SetUpInParam(pTable,bisAllowUpdate);
                //����oracle��ֵ��MDB��ȥ��ѯ��,���Ƿ����,���������,��������insert
                if(!m_pMSelect->Next())
                {
                    //����
                    m_pMInsert->Execute();
                    m_pMInsert->Commit();
                }
                else
                {
                    if(bisAllowUpdate)
                    {
                        m_pMQuery->Execute();
                        m_pMQuery->Commit();
                    }
                }
            }
        }
        catch(TMDBDBExcpInterface &e)
        {        
            TADD_ERROR(-1,"Oracle-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
            delete m_pMQuery;
            TADD_ERROR(-1,"pszMsg=[%s]",pszMsg);
            return false;    
        }
        catch(TMdbException &e)
        { 
            TADD_ERROR(-1,"QMDB-SQL=[%s], error_msg=[%s].", e.GetErrSql(), e.GetErrMsg());
            delete m_pMQuery;
            TADD_ERROR(-1,"pszMsg=[%s]",pszMsg);
            return false;    
        }
        delete m_pMQuery;
        delete m_pMInsert;
        delete m_pMSelect;

        TADD_FUNC("Finish.");
        return true;
    }

    void TMdbDbFlushMdb::SetUpInParam(TMdbTable* pTable,bool bisAllowUpdate)
    {
        //��update��insert�������ȥ
        for(int j=0; j<pTable->iColumnCounts; ++j)
        {
            //insert sql
            //�ж��Ƿ�ΪNULL��
            if(m_pOraQuery->Field(pTable->tColumn[j].sName).isNULL())
            {
                m_pMInsert->SetParameterNULL(j);
            }
            else
            {
                if(pTable->tColumn[j].iDataType == DT_Int)
                {
                    m_pMInsert->SetParameter(pTable->tColumn[j].sName, \
                        (long long)m_pOraQuery->Field(pTable->tColumn[j].sName).AsInteger());
                }
                else
                {
                    m_pMInsert->SetParameter(pTable->tColumn[j].sName,\
                        m_pOraQuery->Field(pTable->tColumn[j].sName).AsString());
                }
            }
            //end insert sql
            //update sql
            if(bisAllowUpdate)
            {
                bool bPkCol = false;
                for(int n=0; n<(pTable->m_tPriKey).iColumnCounts; ++n)
                {
                    int iNo =  pTable->m_tPriKey.iColumnNo[n];//������λ��
                    if(iNo == j)
                    {
                        bPkCol = true;
                        break;
                    }      
                }
                if(!bPkCol)
                {
                    if(m_pOraQuery->Field(pTable->tColumn[j].sName).isNULL())
                    {
                        m_pMQuery->SetParameterNULL(pTable->tColumn[j].sName);
                        continue;
                    }
                    if(pTable->tColumn[j].iDataType == DT_Int)
                    {
                        m_pMQuery->SetParameter(pTable->tColumn[j].sName, \
                            m_pOraQuery->Field(pTable->tColumn[j].sName).AsInteger());
                    }
                    else
                    {
                        m_pMQuery->SetParameter(pTable->tColumn[j].sName,\
                            m_pOraQuery->Field(pTable->tColumn[j].sName).AsString());  
                    }
                }
            }
            //end update sql
        }
    }

    int TMdbDbFlushMdb::GetInsertSQL(TMdbTable* pTable,char* sMInsertSql,const int iLen)
    {
        //ƴqmdb :insert sql
        bool bFlag  = false;
        snprintf(sMInsertSql,iLen,"insert into %s ",pTable->sTableName);
        for(int j=0; j<pTable->iColumnCounts; ++j)
        {
            if(bFlag == false)
            {
                snprintf(sMInsertSql+strlen(sMInsertSql),iLen-strlen(sMInsertSql),\
                    " (%s",pTable->tColumn[j].sName);
                bFlag = true;
            }
            else
            {
                snprintf(sMInsertSql+strlen(sMInsertSql),iLen-strlen(sMInsertSql),\
                    ",%s", pTable->tColumn[j].sName);
            }
        }
        snprintf(sMInsertSql+strlen(sMInsertSql),iLen-strlen(sMInsertSql),"%s",") values");
        bFlag = false;
        for(int j=0; j<pTable->iColumnCounts; ++j)
        {
            if(!bFlag)
            {
                snprintf(sMInsertSql+strlen(sMInsertSql),iLen-strlen(sMInsertSql),\
                    " (:%s",pTable->tColumn[j].sName);
                bFlag = true;
            }
            else
            {
                snprintf(sMInsertSql+strlen(sMInsertSql),iLen-strlen(sMInsertSql),\
                    ",:%s", pTable->tColumn[j].sName);
            }
        }
        snprintf(sMInsertSql+strlen(sMInsertSql),iLen-strlen(sMInsertSql),"%s",")");
        return 0;
    }

    int TMdbDbFlushMdb::GetSQL(TMdbTable* pTable,char* sSQL,char* sMSQL,char* sMSelectSql,bool &bisAllowUpdate)
    {
        strcpy(sSQL,"select ");
        sprintf(sMSQL, "update %s set ", pTable->sTableName);
        sprintf(sMSelectSql,"select ");
        bool bFlag  = false;
        bool bMFlag = false;
        bisAllowUpdate = true;
        //oracleͬ������
        int iOraRepCounts = 0;
        iOraRepCounts = GetOraRepCounts(pTable);
        for(int j=0; j<pTable->iColumnCounts; ++j)
        {
            if(bFlag == false)
            {
                if(pTable->tColumn[j].bIsDefault)
                {
                    sprintf(sSQL+strlen(sSQL), " nvl(%s,'%s') %s", pTable->tColumn[j].sName,pTable->tColumn[j].iDefaultValue,pTable->tColumn[j].sName);
                }
                else
                {
                    sprintf(sSQL+strlen(sSQL), " %s", pTable->tColumn[j].sName);
                }
                sprintf(sMSelectSql+strlen(sMSelectSql), " %s", pTable->tColumn[j].sName);
                bFlag = true;
            }
            else
            {
                if(pTable->tColumn[j].bIsDefault)
                {
                    sprintf(sSQL+strlen(sSQL), ", nvl(%s,'%s') %s", pTable->tColumn[j].sName,pTable->tColumn[j].iDefaultValue,pTable->tColumn[j].sName);
                }
                else
                {
                    sprintf(sSQL+strlen(sSQL), ", %s", pTable->tColumn[j].sName);
                }
                sprintf(sMSelectSql+strlen(sMSelectSql), ", %s", pTable->tColumn[j].sName);
            }

            //�жϱ����Ƿ�������,ȷ��Ψһ��
            bool bIsIndex = false;
            for(int n=0; n<(pTable->m_tPriKey).iColumnCounts; ++n)
            {
                int iNo =  pTable->m_tPriKey.iColumnNo[n];
                if(iNo == j)
                {
                    bIsIndex = true;
                    break;
                }
            }
            //����в���������,�����
            if(bIsIndex == false)
            {
                if(bMFlag == false)
                {
                    sprintf(sMSQL+strlen(sMSQL), " %s=:%s", pTable->tColumn[j].sName, pTable->tColumn[j].sName);
                    bMFlag = true;
                }
                else
                {
                    sprintf(sMSQL+strlen(sMSQL), ", %s=:%s", pTable->tColumn[j].sName, pTable->tColumn[j].sName);
                }
            }

        }
        //������е��ж�������
        if((pTable->m_tPriKey).iColumnCounts == iOraRepCounts)
        {
            bisAllowUpdate = false;
        }
        if(bFlag == false)
        {
            TADD_FLOW("Table=[%s], No-Column need to Restore", pTable->sTableName);
            return -1;
        }
        return 0;
    }

    TMdbTable* TMdbDbFlushMdb::GetMdbTable(const char* pszTable)
    {
        TMdbTable* pTable = NULL;
        pTable = m_pConfig->GetTable(pszTable);
        if(NULL == pTable)
        {
            TADD_ERROR(-1,"Table [%s] does not exist in config.",pszTable);
            return NULL;
        } 
        return pTable;
    }
    int  TMdbDbFlushMdb::GetPosOra(const char* pszMsg)
    {
        int iPosOra = -1;
        for(size_t i=0; i<strlen(pszMsg); ++i)
        {
            if(pszMsg[i] != '|')
            {
                continue;
            }
            else
            {
                iPosOra = i+1;
                break;
            }
        }
        return iPosOra;
    }

    int  TMdbDbFlushMdb::GetPosTabName(char* pszTabName,const char* pszMsg)
    {
        int iPos = -1;
        for(size_t i=0; i<strlen(pszMsg); ++i)
        {
            if(pszMsg[i] != ',')
            {
                pszTabName[i] = pszMsg[i];
                continue;
            }
            else
            {
                iPos = i+1;
                break;
            }
        }
        return iPos;
    }    
    int TMdbDbFlushMdb::GetOraRepCounts(TMdbTable* pTable)
    {
        int iOraRepCounts = 0;
        for(int j=0; j<pTable->iColumnCounts; ++j)
        {
           iOraRepCounts ++; 
        }
        return iOraRepCounts;
    }

//}
