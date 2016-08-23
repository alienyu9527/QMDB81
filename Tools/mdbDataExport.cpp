/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbDataExport.cpp   
*@Description�� �ڴ������ݵ����ķ��ʽӿ�
*@Author:       li.shugang
*@Date��        2009��10��30��
*@History:
******************************************************************************************/

#ifndef DB_NODB

#include "Tools/mdbDataExport.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

#ifdef WIN32
#pragma warning(disable:4101)
#endif

    TMdbDataExport::TMdbDataExport()
    {
        m_pConfig = NULL;
        m_pDBLink = NULL;
        m_pTable  = NULL;
        m_pCountSQL = NULL;
    }


    TMdbDataExport::~TMdbDataExport()
    {  
        if(m_pDBLink != NULL)
            delete m_pDBLink;

        if(m_pCountSQL)
        {
            delete []m_pCountSQL;
            m_pCountSQL = NULL;
        }
    }


    /******************************************************************************
    * ��������  :  Login()
    * ��������  :  ���ݵ�¼��Ϣ����¼���ݿ�  
    * ����      :  pszDSN, �ڴ����ݿ��DSN����
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻�-1
    * ����      :  li.shugang
    *******************************************************************************/
    int TMdbDataExport::Login(const char* pszDSN)
    {
        TADD_FUNC("TMdbDataExport::Login(%s) : Begin.", pszDSN); 
        int iRet = 0;
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        CHECK_OBJ(m_pConfig);
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
        }
        catch(TMDBDBExcpInterface &e)
        {        
            TADD_ERROR(-1,"[%s : %d] : TMdbDataExport::Login() : Can't connect Oracle. DSN=%s, UID=%s, PWD=%s.", 
                __FILE__, __LINE__, sDSN, sUID, sPWD);
            return e.GetErrCode();    
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
                TADD_ERROR(-1,"[%s : %d] : TMdbDataExport::Login() Can't connect to QuickMDB.", __FILE__, __LINE__);
                return ERR_DB_LOCAL_CONNECT;
            }
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(-1,"[%s : %d] : TMdbDataExport::Login() Can't connect to QuickMDB.", __FILE__, __LINE__);
            return e.GetErrCode();
        }
            
        TADD_FUNC("TMdbDataExport::Login(%s) : Finish.", pszDSN);
        
        return iRet;
    }


    /******************************************************************************
    * ��������  :  Export()
    * ��������  :  ��������  
    * ����      :  pszTableName, ��Ҫ�����ı�   
    * ����      :  pszObjTable,  ������Ŀ���   
    * ���      :  ��
    * ����ֵ    :  �ɹ����ص����ļ�¼�������򷵻�-1
    * ����      :  li.shugang
    *******************************************************************************/
    int TMdbDataExport::Export(const char* pszTableName, const char* pszObjTable,const char* pszWhere)
    {
        TADD_FUNC("TMdbDataExport::Export(%s==>%s) : Begin.", pszTableName, pszObjTable);
        if(strlen(pszWhere) > 0)
        {
            sWhere = pszWhere;
            TADD_NORMAL("where condition: %s.",pszWhere);
        }
        
        int iRet = 0;
        
        //����Ƿ���ڶ�Ӧ���ڴ��, ��ƴд��Ӧ��SQL
        iRet = CheckMDBTable(pszTableName);
        if(iRet < 0)
        {
            return iRet; 
        }
        
        //����Ƿ���ڶ�Ӧ��Oracle��, ��ƴд��Ӧ��SQL
        iRet = CheckOraTable(pszObjTable);
        if(iRet < 0)
        {
            TADD_ERROR(-1,"[%s : %d] : TMdbDataExport::Export() : Can't find Oracle-Table=[%s].", __FILE__, __LINE__, pszObjTable);
            return iRet; 
        }
        
        //����DAO����
        TMdbDAOBase* pDAOBase = new(std::nothrow) TMdbDAOBase();
        if(pDAOBase == NULL)
        {
            TADD_ERROR(-1,"[%s : %d] : TMdbDataExport::Export() : Can't c CreateDAO().", __FILE__, __LINE__);
            return iRet; 
        }
        pDAOBase->SetSQL(m_sORaSQL);
                
        try
        {
            TMdbQuery *pQuery = m_tDB.CreateDBQuery();
            if(pQuery == NULL)
            {
                TADD_ERROR(-1,"Can't CreateDBQuery.");
                return ERR_OS_NO_MEMROY;
            }
            pQuery->CloseSQL();
            pQuery->SetSQL(m_pCountSQL);
            pQuery->Open();
            int iCounts = 0;
            if(pQuery->Next())
            {
                iCounts = pQuery->Field(0).AsInteger();
            }
            TADD_NORMAL("[%d] data counts in qmdb table [%s].",iCounts,pszTableName);
            
            iCounts = 0;
            pQuery->CloseSQL();
            pQuery->SetSQL(m_sMDBSQL);
            pQuery->Open();
            
            TMdbData tData;
            while(pQuery->Next())
            { 
                //�����ݶ��Ž�ȥ
                pDAOBase->StartData();
                for(int i=0; i<m_pTable->iColumnCounts; ++i)
                {
                    tData.Clear();
                    //������
                    SAFESTRCPY(tData.sName,sizeof(tData.sName),m_pTable->tColumn[i].sName);
                    //��������
                    SAFESTRCPY(tData.sPName,sizeof(tData.sPName),m_pTable->tColumn[i].sName);
                    tData.iType = m_pTable->tColumn[i].iDataType;
                    tData.iLen = m_pTable->tColumn[i].iColumnLen;

                    //�Ƿ�Ϊ��
                    if(pQuery->Field(m_pTable->tColumn[i].sName).isNULL())
                    {
                        tData.isNull = -1;
                    }
                    else
                    {
                        if(tData.iType == DT_Int)
                        {
                            sprintf(tData.sValue, "%lld", pQuery->Field(m_pTable->tColumn[i].sName).AsInteger());
                        }
                        else
                        {
                            SAFESTRCPY(tData.sValue,sizeof(tData.sValue),pQuery->Field(m_pTable->tColumn[i].sName).AsString());
                        }
                    }
                    //��������
                    tData.cOperType = '=';
                    //�������
                    pDAOBase->AddData(&tData);
                }
                pDAOBase->EndData();
                
                //����������ﵽ��������ֵ�����ύ���
                TADD_DETAIL("Real-Counts=%d, MAX_DATA_COUNTS=%d.", pDAOBase->GetCounts(), MAX_DATA_COUNTS);
                if(pDAOBase->GetCounts() >= MAX_DATA_COUNTS)
                {
                    iRet = pDAOBase->Execute(m_pDBLink); 
                    if(iRet < 0)
                    {
                         pDAOBase->WriteError();   
                    }
                    pDAOBase->ClearArrayData();  
                }
        
                ++iCounts;
                if(iCounts%10000 == 0)
                {
                    TADD_NORMAL("[%s]==>[%s] iCounts=%d.", pszTableName, pszObjTable, iCounts);  
                }
            }
            
            //��ʣ��������ύ
            iRet = pDAOBase->Execute(m_pDBLink); 
           	 m_pDBLink->Commit();
            
            TADD_NORMAL("[%s]==>[%s] Total-iCounts=%d.", pszTableName, pszObjTable, iCounts);    
            
            delete pQuery;
            pQuery = NULL;
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(-1,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());  
            return e.GetErrCode();
        }
        SAFE_DELETE(pDAOBase);
        TADD_FUNC("TMdbDataExport::Export(%s==>%s) : Finish.", pszTableName, pszObjTable);
        return iRet;
    }


    /******************************************************************************
    * ��������  :  ForceExport()
    * ��������  :  ��������  
    * ����      :  pszTableName, ��Ҫ�����ı�   
    * ����      :  pszObjTable,  ������Ŀ���   
    * ���      :  ��
    * ����ֵ    :  �ɹ����ص����ļ�¼�������򷵻�-1
    * ����      :  li.shugang
    *******************************************************************************/
    int TMdbDataExport::ForceExport(const char* pszTableName, const char* pszObjTable,const char* pszWhere)
    {
        TADD_FUNC("TMdbDataExport::Export(%s==>%s) : Begin.", pszTableName, pszObjTable);
        if(strlen(pszWhere) > 0)
        {
            sWhere = pszWhere;
            TADD_NORMAL("where condition: %s.",pszWhere);
        }
        
        int iRet = 0;
        
        //����Ƿ���ڶ�Ӧ���ڴ��, ��ƴд��Ӧ��SQL
        iRet = CheckMDBTable(pszTableName);
        if(iRet < 0)
        {
            return iRet; 
        }
        
        //����Ƿ���ڶ�Ӧ��Oracle��, ��ƴд��Ӧ��SQL
        iRet = ForceCheckOraTable(pszObjTable);
        if(iRet < 0)
        {
            TADD_ERROR(-1,"[%s : %d] : TMdbDataExport::Export() : Can't find Oracle-Table=[%s].", __FILE__, __LINE__, pszObjTable);
            return iRet; 
        }
        
        //����DAO����
        TMdbDAOBase* pDAOBase = new(std::nothrow) TMdbDAOBase();
        if(pDAOBase == NULL)
        {
            TADD_ERROR(-1,"[%s : %d] : TMdbDataExport::Export() : Can't CreateDAO().", __FILE__, __LINE__);
            return iRet; 
        }
        pDAOBase->SetSQL(m_sORaSQL);
                
        try
        {
            TMdbQuery *pQuery = m_tDB.CreateDBQuery();
            if(pQuery == NULL)
            {
                TADD_ERROR(-1,"Can't CreateDBQuery.");
                return ERR_OS_NO_MEMROY;
            }
            pQuery->CloseSQL();
            pQuery->SetSQL(m_pCountSQL);
            pQuery->Open();
            int iCounts = 0;
            if(pQuery->Next())
            {
                iCounts = pQuery->Field(0).AsInteger();
            }
            TADD_NORMAL("[%d] data counts in qmdb table [%s].",iCounts,pszTableName);
            
            iCounts = 0;
            pQuery->CloseSQL();
            pQuery->SetSQL(m_sMDBSQL);
            pQuery->Open();
            
            TMdbData tData;
            while(pQuery->Next())
            { 
                //�����ݶ��Ž�ȥ
                pDAOBase->StartData();
				#ifdef DB_ORACLE
				int addDataRound = 2;
				#elif DB_MYSQL
				int addDataRound = 1;
				#endif
				for(int k = 0; k < addDataRound; k++)
				{
	                for(int i=0; i<m_pTable->iColumnCounts; ++i)
	                {
	                    tData.Clear();
	                    //������
	                    SAFESTRCPY(tData.sName,sizeof(tData.sName),m_pTable->tColumn[i].sName);
	                    //��������
	                    SAFESTRCPY(tData.sPName,sizeof(tData.sPName),m_pTable->tColumn[i].sName);
	                    tData.iType = m_pTable->tColumn[i].iDataType;
	                    tData.iLen = m_pTable->tColumn[i].iColumnLen;

	                    //�Ƿ�Ϊ��
	                    if(pQuery->Field(m_pTable->tColumn[i].sName).isNULL())
	                    {
	                        tData.isNull = -1;
	                    }
	                    else
	                    {
	                        if(tData.iType == DT_Int)
	                        {
	                            snprintf(tData.sValue, sizeof(tData.sValue), "%lld", pQuery->Field(m_pTable->tColumn[i].sName).AsInteger());
	                        }
	                        else
	                        {
	                            SAFESTRCPY(tData.sValue,sizeof(tData.sValue),pQuery->Field(m_pTable->tColumn[i].sName).AsString());
	                        }
	                    }
	                    //��������
	                    tData.cOperType = '=';
	                    //�������
	                    pDAOBase->AddData(&tData);
	                }
				}
                pDAOBase->EndData();
                
                //����������ﵽ��������ֵ�����ύ���
                TADD_DETAIL("Real-Counts=%d, MAX_DATA_COUNTS=%d.", pDAOBase->GetCounts(), MAX_DATA_COUNTS);
                if(pDAOBase->GetCounts() >= MAX_DATA_COUNTS)
                {
                    iRet = pDAOBase->Execute(m_pDBLink); 
                    if(iRet < 0)
                    {
                         pDAOBase->WriteError();   
                    }
                    pDAOBase->ClearArrayData();  
                }
        
                ++iCounts;
                if(iCounts%10000 == 0)
                {
                    TADD_NORMAL("[%s]==>[%s] iCounts=%d.", pszTableName, pszObjTable, iCounts);  
                }
            }
            
            //��ʣ��������ύ
            iRet = pDAOBase->Execute(m_pDBLink); 
            m_pDBLink->Commit();
            
            TADD_NORMAL("[%s]==>[%s] Total-iCounts=%d.", pszTableName, pszObjTable, iCounts);    
            
            delete pQuery;
            pQuery = NULL;
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(-1,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());  
            return e.GetErrCode();
        }
        SAFE_DELETE(pDAOBase);
        TADD_FUNC("TMdbDataExport::Export(%s==>%s) : Finish.", pszTableName, pszObjTable);
        return iRet;
    }
	
    int TMdbDataExport::GetMDBUser(const char* pszDSN, char* pszUID, char* pszPWD)
    {
        TADD_FUNC("TMdbDataExport::GetMDBUser(%s) : Begin.", pszDSN); 
        int iRet = 0;
        int iUserCounts = m_pConfig->GetUserCounts();
        TMDbUser* pUser = NULL;
        for(int i=0;i<iUserCounts;i++)
        {
            pUser = m_pConfig->GetUser(i);
            if(pUser == NULL)
            {
                TADD_ERROR(-1,"TMdbDataExport::GetMDBUser() : Can't GetUser().");
                return ERR_DB_USER_INVALID;  
            }
            if(TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess,"Administrator") == 0 ||  TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess,"Read-Write") == 0 )
            {
                break;
            }
        }
        
        SAFESTRCPY(pszUID,32,pUser->sUser);
        SAFESTRCPY(pszPWD,32,pUser->sPwd);
        TADD_FUNC("TMdbDataExport::GetMDBUser(%s/%s@%s) : Finish.", pszUID, pszPWD, pszDSN);    
        return iRet;
    }


    int TMdbDataExport::GetOraUser(char* pszDSN, char* pszUID, char* pszPWD)
    {
        TADD_FUNC("TMdbDataExport::GetOraUser() : Begin.");
        
        int iRet = 0;
        TMdbCfgDSN* pDSN = m_pConfig->GetDSN();
        if(pDSN == NULL)
        {
            TADD_ERROR(-1,"TMdbDataExport::GetOraUser() : Can't GetDSN().");
            return ERR_DB_DSN_INVALID;  
        }
        
        strcpy(pszUID, pDSN->sOracleUID);
        strcpy(pszDSN, pDSN->sOracleID);
        strcpy(pszPWD, pDSN->sOraclePWD);

        
        TADD_FUNC("TMdbDataExport::GetOraUser(%s/%s@%s) : Finish.", pszUID, pszPWD, pszDSN);    
        return iRet;
    }


    int TMdbDataExport::CheckMDBTable(const char* pszTableName)
    {
        TADD_FUNC("TMdbDataExport::CheckMDBTable() : Begin.");
        
        int iRet = 0;
        
        //�ҵ���Ӧ�ı���Ϣ
        m_pTable = m_pConfig->GetTable(pszTableName);
        if(m_pTable == NULL)
        {
            TADD_ERROR(-1,"TMdbDataExport::CheckMDBTable() : Can't find table=[%s].", pszTableName);
            return ERR_TAB_NO_TABLE;      
        }
        //199199 ȥ��mdbExPortͬ���������ƣ��������Ŀ��԰�QMDB�еı��뵽Oracle
        /*
        //mdb->oracleͬ�����в�����
        bool bOraRep = false;
        for(int i = 0;i<m_pTable->iColumnCounts;i++)
        {
            if(m_pTable->tColumn[i].iRepAttr == Column_To_Ora || m_pTable->tColumn[i].iRepAttr == Column_Ora_Rep)
            {
                bOraRep = true;
                break;
            }
        }
        if(!bOraRep)
        {
            TADD_ERROR(-1,"TMdbDataExport::CheckMDBTable() : no Ora-Rep column in table=[%s].", pszTableName);
            return ERROR_INVALID_REP_TYPE;  
        }
        */
        
        //ƴװ��ѯ���
        memset(m_sMDBSQL, 0, sizeof(m_sMDBSQL));
        sprintf(m_sMDBSQL, "select * from %s", pszTableName);

        m_pCountSQL = new(std::nothrow) char[512];
		if(m_pCountSQL == NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new m_pCountSQL");
			return ERR_OS_NO_MEMROY;
		}
        memset(m_pCountSQL, 0, sizeof(m_pCountSQL));
        sprintf(m_pCountSQL, "select count(*) from %s", pszTableName);
        
        if(strlen(sWhere.c_str()) > 0)
        {
            if(TMdbNtcStrFunc::FindString(sWhere.c_str(),"where") >= 0)
            {
                sprintf(m_sMDBSQL+strlen(m_sMDBSQL), " %s", sWhere.c_str());
                sprintf(m_pCountSQL+strlen(m_pCountSQL), " %s", sWhere.c_str());
            }
            else
            {
                sprintf(m_sMDBSQL+strlen(m_sMDBSQL), " where %s", sWhere.c_str());
                sprintf(m_pCountSQL+strlen(m_pCountSQL), " where %s", sWhere.c_str());
            }
        }
        TADD_NORMAL("QuickMDB-SQL=[%s]", m_sMDBSQL);
        TADD_NORMAL("QuickMDB-COUNT-SQL=[%s]", m_pCountSQL);
        
        TADD_FUNC("TMdbDataExport::CheckMDBTable() : Finish."); 
        return iRet;    
    }


    int TMdbDataExport::CheckOraTable(const char* pszTableName)
    {
        TADD_FUNC("TMdbDataExport::CheckOraTable() : Begin.");
        
        int iRet = 0;

        //����Ƿ���ڶ�Ӧ��Oracle��
        memset(m_sORaSQL, 0, sizeof(m_sORaSQL));
        sprintf(m_sORaSQL, "select count(*) from %s", pszTableName);
        
        try
        {
            TMDBDBQueryInterface* pTDBQuery = m_pDBLink->CreateDBQuery();
            pTDBQuery->Close();
            pTDBQuery->SetSQL(m_sORaSQL);
            pTDBQuery->Open();
            pTDBQuery->Next();
            delete pTDBQuery;
            pTDBQuery = NULL;
        }
        catch(TMDBDBExcpInterface &e)
        {        
            TADD_ERROR(-1,"[%s : %d] : TMdbDataExport::CheckOraTable() : ERROR-MSG=%s\nERROR-SQL=%s.", 
                __FILE__, __LINE__, e.GetErrMsg(), e.GetErrSql());
            return e.GetErrCode();    
        }
        
        memset(m_sORaSQL, 0, sizeof(m_sORaSQL));
        sprintf(m_sORaSQL, "insert into %s(", pszTableName);
        
        char sTemp[1024];
        memset(sTemp, 0, sizeof(sTemp));
        
        bool bFlag = false;
        for(int i=0; i<m_pTable->iColumnCounts; ++i)
        {
            if(bFlag)
            {
                sprintf(&m_sORaSQL[strlen(m_sORaSQL)], ", %s",  m_pTable->tColumn[i].sName);

                if(m_pTable->tColumn[i].iDataType != DT_DateStamp)
                {
                    sprintf(&sTemp[strlen(sTemp)], ", :%s",     m_pTable->tColumn[i].sName);
                }
                else
                {
                    sprintf(&sTemp[strlen(sTemp)], ", to_date(:%s, 'YYYYMMDDHH24MISS')",m_pTable->tColumn[i].sName);
                }
            }
            else
            {
                sprintf(&m_sORaSQL[strlen(m_sORaSQL)], "%s", m_pTable->tColumn[i].sName);
                
                if(m_pTable->tColumn[i].iDataType != DT_DateStamp)
                {
                    sprintf(&sTemp[strlen(sTemp)], ":%s",m_pTable->tColumn[i].sName);
                }
                else
                {
                    sprintf(&sTemp[strlen(sTemp)], "to_date(:%s, 'YYYYMMDDHH24MISS')", m_pTable->tColumn[i].sName);
                }
                bFlag = true;
            }
        }
        
        sprintf(&m_sORaSQL[strlen(m_sORaSQL)], ") values(%s)",  sTemp);
        TADD_NORMAL("TMdbDataExport::CheckOraTable() : Oracle-SQL=[%s]", m_sORaSQL);
        
        TADD_FUNC("TMdbDataExport::CheckOraTable() : Finish."); 
        return iRet;    
    }

    int TMdbDataExport::ForceCheckOraTable(const char* pszTableName)
    {
        TADD_FUNC("TMdbDataExport::CheckOraTable() : Begin.");
        
        int iRet = 0;

        //����Ƿ���ڶ�Ӧ��Oracle��
        memset(m_sORaSQL, 0, sizeof(m_sORaSQL));
        snprintf(m_sORaSQL, sizeof(m_sORaSQL) - strlen(m_sORaSQL), "select count(*) from %s", pszTableName);
        
        try
        {
            TMDBDBQueryInterface* pTDBQuery = m_pDBLink->CreateDBQuery();
            pTDBQuery->Close();
            pTDBQuery->SetSQL(m_sORaSQL);
            pTDBQuery->Open();
            pTDBQuery->Next();
            delete pTDBQuery;
            pTDBQuery = NULL;
        }
        catch(TMDBDBExcpInterface &e)
        {        
            TADD_ERROR(-1,"[%s : %d] : TMdbDataExport::CheckOraTable() : ERROR-MSG=%s\nERROR-SQL=%s.", 
                __FILE__, __LINE__, e.GetErrMsg(), e.GetErrSql());
            return e.GetErrCode();    
        }
        
        memset(m_sORaSQL, 0, sizeof(m_sORaSQL));
		char sColm[1024];
        memset(sColm, 0, sizeof(sColm));
		char sValue[1024];
        memset(sValue, 0, sizeof(sValue));
		char sPriCondition[1024];
        memset(sPriCondition, 0, sizeof(sPriCondition));
		char sUpdateValue[1024];
        memset(sUpdateValue, 0, sizeof(sUpdateValue));
		char sUpdateCondition[1024];
        memset(sUpdateCondition, 0, sizeof(sUpdateCondition));
		
        TMdbPrimaryKey * pPriKey = &(m_pTable->m_tPriKey);
		for( int i = 0; i < pPriKey->iColumnCounts; i++)
		{
			if(m_pTable->tColumn[pPriKey->iColumnNo[i]].iDataType != DT_DateStamp)
            {
                snprintf(sPriCondition, sizeof(sPriCondition)-strlen(sPriCondition), " A.%s = :%s and", m_pTable->tColumn[pPriKey->iColumnNo[i]].sName, m_pTable->tColumn[pPriKey->iColumnNo[i]].sName);
				snprintf(sValue+strlen(sValue), sizeof(sValue) - strlen(sValue), " :%s,", m_pTable->tColumn[pPriKey->iColumnNo[i]].sName);
            }
            else
            {
                snprintf(sPriCondition, sizeof(sPriCondition)-strlen(sPriCondition), " A.%s = to_date(:%s, 'YYYYMMDDHH24MISS') and", m_pTable->tColumn[pPriKey->iColumnNo[i]].sName, m_pTable->tColumn[pPriKey->iColumnNo[i]].sName);
				snprintf(sValue+strlen(sValue), sizeof(sValue) - strlen(sValue), " to_date(:%s, 'YYYYMMDDHH24MISS'),",m_pTable->tColumn[pPriKey->iColumnNo[i]].sName);
            }
			snprintf(sColm, sizeof(sColm)-strlen(sColm), " %s,", m_pTable->tColumn[pPriKey->iColumnNo[i]].sName);
		}
		sPriCondition[strlen(sPriCondition) - 4] = '\0';

        for(int i=0; i<m_pTable->iColumnCounts; ++i)
        {
        	bool isPrimaryKey = false;
			for(int j = 0; j < pPriKey->iColumnCounts; j++)
			{
				if(pPriKey->iColumnNo[j] == i)
				{
					isPrimaryKey = true;
					break;
				}
			}
			if(isPrimaryKey) continue;
			snprintf(sColm+strlen(sColm), sizeof(sColm)-strlen(sColm), " %s,", m_pTable->tColumn[i].sName);
			snprintf(sUpdateCondition+strlen(sUpdateCondition),sizeof(sUpdateCondition)-strlen(sUpdateCondition)," %s=values(%s),",m_pTable->tColumn[i].sName,m_pTable->tColumn[i].sName);
            if(m_pTable->tColumn[i].iDataType != DT_DateStamp)
            {
                snprintf(sValue+strlen(sValue), sizeof(sValue) - strlen(sValue), " :%s,", m_pTable->tColumn[i].sName);
                snprintf(sUpdateValue+strlen(sUpdateValue), sizeof(sUpdateValue) - strlen(sUpdateValue), " %s = :%s,", m_pTable->tColumn[i].sName, m_pTable->tColumn[i].sName);
            }
            else
            {
                snprintf(sValue+strlen(sValue), sizeof(sValue) - strlen(sValue), " to_date(:%s, 'YYYYMMDDHH24MISS'),",m_pTable->tColumn[i].sName);
                snprintf(sUpdateValue+strlen(sUpdateValue), sizeof(sUpdateValue) - strlen(sUpdateValue), " %s = to_date(:%s, 'YYYYMMDDHH24MISS'),",m_pTable->tColumn[i].sName, m_pTable->tColumn[i].sName);
            }
        }
		sColm[strlen(sColm) - 1] = '\0';
		sValue[strlen(sValue) - 1] = '\0';
		sUpdateValue[strlen(sUpdateValue) - 1] = '\0';
		sUpdateCondition[strlen(sUpdateCondition) - 1] = '\0';

		#ifdef DB_ORACLE
		snprintf(m_sORaSQL, sizeof(m_sORaSQL), "merge into %s A using (select * from dual) B on (%s) when matched then update set %s when not matched then insert (%s) values (%s)", pszTableName, sPriCondition, sUpdateValue, sColm, sValue);
		#elif DB_MYSQL
		snprintf(m_sORaSQL, sizeof(m_sORaSQL), "insert into %s (%s) values (%s) on duplicate key update (%s)", pszTableName, sColm, sValue, sUpdateCondition);
		#endif
		
        TADD_NORMAL("TMdbDataExport::CheckOraTable() : Oracle-SQL=[%s]", m_sORaSQL);
        
        TADD_FUNC("TMdbDataExport::CheckOraTable() : Finish."); 
        return iRet;    
    }

#endif   //  DB_NODB
	
//}    

