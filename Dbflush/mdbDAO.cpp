/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbDAO.cpp		
*@Description： 负责管理miniDB的动态DAO的控制
*@Author:		li.shugang
*@Date：	    2009年03月23日
*@History:
******************************************************************************************/

#include "Dbflush/mdbDAO.h"
#include "Helper/mdbBase.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

    #ifdef WIN32
    #pragma warning(disable:4101)
    #endif


    TMdbNodeDAO::TMdbNodeDAO()
    {    
        iOper    = -1;
        sSQL = NULL;
		for(int i = 0; i < MAX_COLUMN_COUNTS; i++)
		{
			m_tData[i] = NULL;
		}
        pDAO     = NULL;
		iCount = 0;
		dataColCount = 0;
    }


    TMdbNodeDAO::~TMdbNodeDAO()
    {
		
		for(int i = 0; i < MAX_COLUMN_COUNTS; i++)
		{
			SAFE_DELETE(m_tData[i]);
		}
		SAFE_DELETE(sSQL);
		if(pDAO != NULL)
        {
            delete pDAO;
            pDAO = NULL;
        }        
    }


    TMdbTableDAO::TMdbTableDAO()
    {
        //iTableID = -1;
        for(int i=0; i<MAX_DAO_COUNTS; ++i)
        {
        	for(int j=0; j<MAX_MYSQL_ARRAY_SIZE; j++)
    		{
				pNodeDAO[i][j] = NULL;
    		}            
        }
        iLastOperType = -1;
        iTabPos = -1;
    }


    TMdbTableDAO::~TMdbTableDAO()
    {
        for(int i=0; i<MAX_DAO_COUNTS; ++i)
        {
        	for(int j=0; j<MAX_MYSQL_ARRAY_SIZE; j++)
    		{          
	            if(pNodeDAO[i][j] != NULL)
	            {
	                delete pNodeDAO[i][j];
	                pNodeDAO[i][j] = NULL;
	            }            
    		}  
        }
    }


    void TMdbTableDAO::Clear()
    {
        for(int i=0; i<MAX_DAO_COUNTS; ++i)
        {
        	for(int j=0; j<MAX_MYSQL_ARRAY_SIZE; j++)
    		{          
	            if(pNodeDAO[i][j] != NULL)
	            {
	                delete pNodeDAO[i][j];
	                pNodeDAO[i][j] = NULL;
	            }            
    		}  
        }
        
        iLastOperType = -1;
        sTableName.clear();
        iTabPos = -1;
    }


    TMdbDAO::TMdbDAO()
    {
        TADD_FUNC("TMdbDAO::TMdbDAO() : Start.");

        memset(m_sDSN, 0, sizeof(m_sDSN));
        memset(m_sUID, 0, sizeof(m_sUID));
        memset(m_sPWD, 0, sizeof(m_sPWD));

        m_pDBLink = NULL;   //链接    
        m_pQuery = NULL;
		m_pSelDBLink = NULL;
		m_pSelQuery = NULL;
        TADD_FUNC("TMdbDAO::TMdbDAO() : Finish.");
    }


    TMdbDAO::~TMdbDAO()
    {
        TADD_FUNC("TMdbDAO::~TMdbDAO() : Start.");

        if(m_pQuery != NULL)
        {
            delete m_pQuery;
            m_pQuery = NULL;
        }

        if(m_pDBLink != NULL)
        {
            m_pDBLink->Disconnect();
            delete m_pDBLink;    
            m_pDBLink = NULL;
        }

		SAFE_DELETE(m_pSelQuery);
		
		if(m_pSelDBLink != NULL)
        {
            m_pSelDBLink->Disconnect();
            delete m_pSelDBLink;    
            m_pSelDBLink = NULL;
        }
        TADD_FUNC("TMdbDAO::~TMdbDAO() : Finish.");
    }


    /******************************************************************************
    * 函数名称	:  Init()
    * 函数描述	:  初始化：连接Oracle  
    * 输入		:  pConfig, 配置文件，带有Oracle的相关信息  
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbDAO::Init(TMdbConfig* pConfig,TMdbShmDSN *pShmDSN)
    {
        TADD_FUNC("TMdbDAO::Init() : Start.");
        CHECK_OBJ(pConfig);
        m_pMdbConfig = pConfig;
        m_pShmDSN = pShmDSN;

        //开始链接
        int iRet = 0;
		#ifdef DB_MYSQL
		do
		{
			try
			{
				if(m_pSelDBLink != NULL)
				{
					delete m_pSelDBLink;
					m_pSelDBLink = NULL;
				}
				m_pSelDBLink = new(std::nothrow) TMdbDatabase();
				if(pShmDSN == NULL) break;
				TMdbDSN * pDsn = pShmDSN->GetInfo();
				if(m_pSelDBLink->ConnectAsMgr(pDsn->sName) == false)
				{
					CHECK_RET(ERR_DB_NOT_CONNECTED,"connect mdb[%s] failed.",pShmDSN->GetInfo()->sName);
				}
				//m_pSelQuery = m_pSelDBLink->CreateDBQuery();
			}
			catch(TMdbException &e)
			{
				TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());
	    		iRet = ERROR_UNKNOWN;
			}
			catch(...)
			{
				TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
	    		iRet = ERROR_UNKNOWN;
			}
		}while(0);
		#endif
        SAFESTRCPY(m_sDSN,sizeof(m_sDSN),pConfig->GetDSN()->sOracleID);
        SAFESTRCPY(m_sUID,sizeof(m_sUID),pConfig->GetDSN()->sOracleUID);
        SAFESTRCPY(m_sPWD,sizeof(m_sPWD),pConfig->GetDSN()->sOraclePWD);

        //检测参数的合法性
        if(m_sDSN[0]==0 || m_sUID[0]==0 || m_sPWD[0]==0)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Paramter invalid. DSN=%s, UID=%s, PWD=%s.", 
                m_sDSN, m_sUID, m_sPWD);
            return ERR_APP_INVALID_PARAM;    
        }

        try
        {
            if(m_pDBLink != NULL)
            {		      
                delete m_pDBLink;    
                m_pDBLink = NULL;
            }                
            m_pDBLink = TMDBDBFactory::CeatDB(); 
            m_pDBLink->SetLogin(m_sUID, m_sPWD, m_sDSN);
            if(m_pDBLink->Connect() == false)
            {
                m_pDBLink->Disconnect();
                if(m_pDBLink != NULL)
                {		      
                    delete m_pDBLink;    
                    m_pDBLink= NULL;
                }
                iRet = ERR_APP_CONNCET_ORACLE_FAILED;
            }
        }
        catch(TMDBDBExcpInterface &e)
        {        
            TADD_ERROR(ERROR_UNKNOWN,"Can't connect Oracle. DSN=%s, UID=%s, PWD=%s.\n ERR-MSG=%s, ERR_SQL=%s\n", 
                m_sDSN, m_sUID, m_sPWD, e.GetErrMsg(), e.GetErrSql());
            m_pDBLink->Disconnect();
            if(m_pDBLink != NULL)
            {		      
                delete m_pDBLink;    
                m_pDBLink= NULL;
            }
            iRet = ERROR_UNKNOWN;    
        }

        //清除数据
        try
        {
            for(int i=0; i<MAX_TABLE_COUNTS; ++i)
            {
                m_tTableDAO[i].Clear();	
            }
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Clear Failed\n");
        }

        TADD_FUNC("TMdbDAO::Init() : Finish(OK).");
        return iRet;


        TADD_ERROR(ERR_APP_CONFIG_NOT_EXIST,"Finish(pConfig=NULL).");
        return ERR_APP_CONFIG_NOT_EXIST;
    }

	int TMdbDAO::CheckMdbData(TMdbLCR& tLCR)
	{
		int iRet = 0;
		
		if(m_pShmDSN == NULL)
		{
			iRet = 1;
			return iRet;
		}
		if(tLCR.m_iSqlType == TK_INSERT || tLCR.m_iSqlType == TK_UPDATE)
		{
			try
			{
				CHECK_RET(GetSelectSQL(tLCR), "GetSelectSQL failed");
				if(m_pSelQuery == NULL)
				{
					m_pSelQuery = m_pSelDBLink->CreateDBQuery();
				}
				m_pSelQuery->Close();
				m_pSelQuery->SetSQL(tLCR.m_sSelSQL);
				TMdbTable* pTable = NULL;
				pTable = m_pShmDSN->GetTableByName(tLCR.m_sTableName.c_str());
				CHECK_OBJ(pTable);
				for(int i=0; i<pTable->m_tPriKey.iColumnCounts; ++i)
		        {
		            int iNo = pTable->m_tPriKey.iColumnNo[i];
					if(tLCR.m_iSqlType == TK_INSERT)
					{
						std::vector<TLCRColm>::iterator itor = tLCR.m_vColms.begin();
		                for (; itor != tLCR.m_vColms.end(); ++itor)
		                {
		                	TADD_DETAIL("%s->tColumn[%d].sName = [%s], itor->m_sColmName = [%s]", pTable->sTableName, iNo, pTable->tColumn[iNo].sName, itor->m_sColmName.c_str());
		                    if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tColumn[iNo].sName,itor->m_sColmName.c_str()) == 0)
		                	{
								if(itor->m_bNull)
								{
									TADD_DETAIL("setParameter [%s] = [null]", pTable->tColumn[iNo].sName);
									m_pSelQuery->SetParameterNULL(pTable->tColumn[iNo].sName);
								}
								else if(pTable->tColumn[iNo].iDataType == DT_Int)
					            {
									TADD_DETAIL("setParameter [%s] = [%d]", pTable->tColumn[iNo].sName, atoi(itor->m_sColmValue.c_str()));
					                m_pSelQuery->SetParameter(pTable->tColumn[iNo].sName, atoi(itor->m_sColmValue.c_str()));
					            }
					            else
					            {
									TADD_DETAIL("setParameter [%s] = [%s]", pTable->tColumn[iNo].sName, itor->m_sColmValue.c_str());
					                m_pSelQuery->SetParameter(pTable->tColumn[iNo].sName, itor->m_sColmValue.c_str());
					            }
								break;
		                	}
		                }
					}
					else
					{
						std::vector<TLCRColm>::iterator itor = tLCR.m_vWColms.begin();
		                for (; itor != tLCR.m_vWColms.end(); ++itor)
		                {
		                	TADD_DETAIL("%s->tColumn[%d].sName = [%s], itor->m_sColmName = [%s]", pTable->sTableName, iNo, pTable->tColumn[iNo].sName, itor->m_sColmName.c_str());
		                    if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tColumn[iNo].sName,itor->m_sColmName.c_str()) == 0)
		                	{
								if(itor->m_bNull)
								{
									TADD_DETAIL("setParameter [%s] = [null]", pTable->tColumn[iNo].sName);
									m_pSelQuery->SetParameterNULL(pTable->tColumn[iNo].sName);
								}
								else if(pTable->tColumn[iNo].iDataType == DT_Int)
					            {
									TADD_DETAIL("setParameter [%s] = [%d]", pTable->tColumn[iNo].sName, atoi(itor->m_sColmValue.c_str()));
					                m_pSelQuery->SetParameter(pTable->tColumn[iNo].sName, atoi(itor->m_sColmValue.c_str()));
					            }
					            else
					            {
									TADD_DETAIL("setParameter [%s] = [%s]", pTable->tColumn[iNo].sName, itor->m_sColmValue.c_str());
					                m_pSelQuery->SetParameter(pTable->tColumn[iNo].sName, itor->m_sColmValue.c_str());
					            }
								break;
		                	}
		                }
					}
		        }
				m_pSelQuery->Open();
				if(m_pSelQuery->Next() == false)//插入或者更新的数据不存在，直接返回，继续下一条记录
				{
					iRet = -1;
					return iRet;
				}
			}
			catch(TMdbException &e)
			{
				TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());
	    		iRet = ERROR_UNKNOWN;
			}
			catch(...)
			{
				TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
	    		iRet = ERROR_UNKNOWN;
			}
		}
		return iRet;
	}

    /******************************************************************************
    * 函数名称	:  Execute()
    * 函数描述	:  执行操作数据, 寻找到匹配的DAO, 向Oracle提交数据  
    * 输入		:  pOneRecord, 数据结果  
    * 输出		:  无
    * 返回值	:  成功返回0，数据问题返回-1, Oracle断开返回1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbDAO::Execute(TMdbLCR& tLCR,bool commitFlag)
    {
        TADD_FUNC("TMdbDAO::Execute() : Start.");

        int iRet     = 0;
		#ifdef DB_MYSQL
		if(CheckMdbData(tLCR) < 0) return 0;
		#endif
        CHECK_RET(GetSQL(tLCR),"GetSQL Failed");
        //m_pOneRecord = pOneRecord;

        //找到对应的DAO，如果没有DAO，则创建一个
        TMdbNodeDAO** pNodeDao = FindDAO(tLCR);
        if(pNodeDao == NULL)
        {
            pNodeDao = CreateDAO(tLCR);
            CHECK_OBJ(pNodeDao);
        }
        //这里判断一下数据，如果当前操作时insert或者delete操作，看前面插入的数据是否有insert和delete，如果有
        //则先执行之前的操作
        int iPos=0;
        for(iPos=0; iPos<MAX_TABLE_COUNTS; ++iPos)
        {
            if(m_tTableDAO[iPos].sTableName == tLCR.m_sTableName)
            {
                break;
            }
        }

        if(m_tTableDAO[iPos].iLastOperType != -1)
        {
            if((tLCR.m_iSqlType == OP_Insert || tLCR.m_iSqlType == OP_Delete)\
            && tLCR.m_iSqlType != m_tTableDAO[iPos].iLastOperType)
            {
                iRet = Commit(false);
                if(iRet < 0)
                {
                    return iRet;
                }
            }
        }

        if(tLCR.m_iSqlType == OP_Insert || tLCR.m_iSqlType == OP_Delete)
        {
            m_tTableDAO[iPos].iLastOperType = tLCR.m_iSqlType;
        }
		

        TMdbDAOBase* pDAOBase = NULL;
		//对于BLOB表需要单条提交，不支持批量提交
		std::vector<TLCRColm>::iterator itor = tLCR.m_vColms.begin();
        for (; itor != tLCR.m_vColms.end(); ++itor)
        {
			if(itor->m_iType == DT_Blob)
			{
		        if(pNodeDao == NULL)
		        {
		            pNodeDao = CreateDAO(tLCR);
		            CHECK_OBJ(pNodeDao);
		        }
				pDAOBase = pNodeDao[0]->pDAO;
				CHECK_OBJ(pDAOBase);
				iRet = pDAOBase->ExecuteOne(m_pDBLink,tLCR);
		        if(iRet < 0)
		        {
		            m_pDBLink->Rollback();
		        }
		        return iRet;
			}
        }

		#ifdef DB_MYSQL
		CHECK_RET(PushData(tLCR, pNodeDao[pNodeDao[0]->iCount], pNodeDao[0]->iCount),"PushData failed");
		pNodeDao[0]->iCount++;
    	pDAOBase = pNodeDao[pNodeDao[0]->iCount-1]->pDAO;
        CHECK_OBJ(pDAOBase);
		if(pNodeDao[0]->iCount >= MAX_MYSQL_ARRAY_SIZE)
		{
			//数据提交到daobase中
			pDAOBase->StartData();
			for(int i = 0; i<pNodeDao[0]->iCount; i++)
			{
		        for(int j=0; j<tLCR.m_vColms.size()+tLCR.m_vWColms.size(); ++j)
		        {
		            if(pNodeDao[i]->m_tData[j] != NULL) pDAOBase->AddData(pNodeDao[i]->m_tData[j]);
		        }
			}
			pDAOBase->EndData();
			//
			pNodeDao[0]->iCount = 0;
		}
		#elif DB_ORACLE
		CHECK_OBJ(pNodeDao[0]);
    	pDAOBase = pNodeDao[0]->pDAO;
        CHECK_OBJ(pDAOBase);
		//把数据都放进去
        pDAOBase->StartData();
	PushData(tLCR);
        for(int i=0; i<tLCR.m_vColms.size()+tLCR.m_vWColms.size(); ++i)
        {
            pDAOBase->AddData(&m_tData[i]);
        }
        pDAOBase->EndData();
		#endif
        //如果数据量达到批量的数值，则提交入库
        TADD_DETAIL("TMdbDAO::Execute() : Real-Counts=%d, MAX_DATA_COUNTS=%d.", pDAOBase->GetCounts(), MAX_DATA_COUNTS);
        if (commitFlag == false)
        {
            if(pDAOBase->GetCounts() >= MAX_DATA_COUNTS)
            {
                iRet = pDAOBase->Execute(m_pDBLink); 
                if(iRet < 0)
                {
                    pDAOBase->WriteError();   
                    m_pDBLink->Rollback();
                }
                pDAOBase->ClearArrayData();  
            }
        }
        else
        {
			#ifdef DB_MYSQL
			//数据提交到daobase中
			pDAOBase->StartData();
			for(int i = 0; i<pNodeDao[0]->iCount; i++)
			{
		        for(int j=0; j<tLCR.m_vColms.size()+tLCR.m_vWColms.size(); ++j)
		        {
		            if(pNodeDao[i]->m_tData[j] != NULL) pDAOBase->AddData(pNodeDao[i]->m_tData[j]);
		        }
			}
			pDAOBase->EndData();
			//
			pNodeDao[0]->iCount = 0;
			#endif
            iRet = pDAOBase->Execute(m_pDBLink); 
            if(iRet < 0)
            {
                pDAOBase->WriteError();   
            }
            pDAOBase->ClearArrayData();  
        }
        TADD_FUNC("TMdbDAO::Execute() : Finish.");
        return iRet;
    }


    //生成DAO
    TMdbNodeDAO** TMdbDAO::CreateDAO(TMdbLCR& tLCR)
    {
        TADD_FUNC("TMdbDAO::CreateDAO() : Start.");

        int iPos = -1;
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            //如果有这个表
            //TADD_DETAIL("TMdbDAO::CreateDAO() : m_pOneRecord->m_iTableID=%d, m_tTableDAO[%d].iTableID=%d.", 
            //m_pOneRecord->m_iTableID, i, m_tTableDAO[i].iTableID);
            if(m_tTableDAO[i].sTableName == tLCR.m_sTableName)
            {
                for(int j=0; j<MAX_DAO_COUNTS; ++j)
                {
                    if(m_tTableDAO[i].pNodeDAO[j][0] == NULL)
                    {
						char sSQL[MAX_SQL_LEN] = {0};
						#ifdef DB_MYSQL
                    	for(int k = 0; k < MAX_MYSQL_ARRAY_SIZE; k++)
                		{
							m_tTableDAO[i].pNodeDAO[j][k] = new(std::nothrow) TMdbNodeDAO();
	                        if(m_tTableDAO[i].pNodeDAO[j][k] == NULL)
	                        {
	                            TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
	                            return NULL;
	                        }

	                        m_tTableDAO[i].pNodeDAO[j][k]->iOper = tLCR.m_iSqlType;
							if(m_tTableDAO[i].pNodeDAO[j][k]->sSQL == NULL)
							{
								m_tTableDAO[i].pNodeDAO[j][k]->sSQL = new(std::nothrow) char[MAX_SQL_LEN];
								if(m_tTableDAO[i].pNodeDAO[j][k]->sSQL == NULL)
								{
									TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                            		return NULL;
								}
							}
							//memset(m_tTableDAO[i].pNodeDAO[j][k]->sSQL, 0, MAX_SQL_LEN);
							//
							if(k == 0)
							{
								SAFESTRCPY(m_tTableDAO[i].pNodeDAO[j][k]->sSQL,MAX_SQL_LEN,tLCR.m_sSQL);
								m_tTableDAO[i].pNodeDAO[j][k]->dataColCount = tLCR.m_vColms.size()+tLCR.m_vWColms.size();
		                        m_tTableDAO[i].pNodeDAO[j][k]->pDAO = new(std::nothrow) TMdbDAOBase();
								if(m_tTableDAO[i].pNodeDAO[j][k]->pDAO == NULL)
								{
									TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new TMdbDAOBase");
									return NULL;

								}
		                        m_tTableDAO[i].pNodeDAO[j][k]->pDAO->SetSQL(tLCR.m_sSQL);
							}
							else
							{
	                    		memset(sSQL, 0, sizeof(sSQL));
								GetSQL(tLCR, k, sSQL);
								//SAFESTRCPY(m_tTableDAO[i].pNodeDAO[j][k]->sSQL,sizeof(m_tTableDAO[i].pNodeDAO[j][k]->sSQL),sSQL);
								m_tTableDAO[i].pNodeDAO[j][k]->dataColCount = m_tTableDAO[i].pNodeDAO[j][0]->dataColCount*k;
		                        m_tTableDAO[i].pNodeDAO[j][k]->pDAO = new(std::nothrow) TMdbDAOBase();
								if(m_tTableDAO[i].pNodeDAO[j][k]->pDAO == NULL)
								{
									TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new TMdbDAOBase");
									return NULL;
								}
		                        m_tTableDAO[i].pNodeDAO[j][k]->pDAO->SetSQL(sSQL);
							}
                		}
						#elif DB_ORACLE
						m_tTableDAO[i].pNodeDAO[j][0] = new(std::nothrow) TMdbNodeDAO();
                        if(m_tTableDAO[i].pNodeDAO[j][0] == NULL)
                        {
                            TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                            return NULL;
                        }

                        m_tTableDAO[i].pNodeDAO[j][0]->iOper = tLCR.m_iSqlType;
						if(m_tTableDAO[i].pNodeDAO[j][0]->sSQL == NULL)
						{
							m_tTableDAO[i].pNodeDAO[j][0]->sSQL = new(std::nothrow) char[MAX_SQL_LEN];
							if(m_tTableDAO[i].pNodeDAO[j][0]->sSQL == NULL)
							{
								TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                        		return NULL;
							}
						}
						//memset(m_tTableDAO[i].pNodeDAO[j][0]->sSQL, 0, MAX_SQL_LEN);
                        SAFESTRCPY(m_tTableDAO[i].pNodeDAO[j][0]->sSQL,MAX_SQL_LEN,tLCR.m_sSQL);
                        m_tTableDAO[i].pNodeDAO[j][0]->pDAO = new(std::nothrow) TMdbDAOBase();
						if(m_tTableDAO[i].pNodeDAO[j][0]->pDAO == NULL)
						{
							TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                            return NULL;
						}
                        m_tTableDAO[i].pNodeDAO[j][0]->pDAO->SetSQL(tLCR.m_sSQL);
						#endif
                        TADD_FUNC("TMdbDAO::CreateDAO() : Finish(Create[%d,%d] OK).", i, j);
                        return m_tTableDAO[i].pNodeDAO[j];
                    }
                    else
                    {
                        TADD_DETAIL("TMdbDAO::CreateDAO() :[%d,%d]==NULL.", i, j);
                    }   
                }

                TADD_ERROR(ERROR_UNKNOWN," too many DAO, Max=%d.",  MAX_DAO_COUNTS);
                return NULL;
            }
            else if(m_tTableDAO[i].sTableName.length()== 0)
            {
                m_tTableDAO[i].sTableName = tLCR.m_sTableName;
                char sSQL[MAX_SQL_LEN] = {0};
				#ifdef DB_MYSQL
            	for(int k = 0; k < MAX_MYSQL_ARRAY_SIZE; k++)
        		{
					m_tTableDAO[i].pNodeDAO[0][k] = new(std::nothrow) TMdbNodeDAO();
                    if(m_tTableDAO[i].pNodeDAO[0][k] == NULL)
                    {
                        TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                        return NULL;
                    }

                    m_tTableDAO[i].pNodeDAO[0][k]->iOper = tLCR.m_iSqlType;
					if(m_tTableDAO[i].pNodeDAO[0][k]->sSQL == NULL)
					{
						m_tTableDAO[i].pNodeDAO[0][k]->sSQL = new(std::nothrow) char[MAX_SQL_LEN];
						if(m_tTableDAO[i].pNodeDAO[0][k]->sSQL == NULL)
						{
							TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                    		return NULL;
						}
					}
					//memset(m_tTableDAO[i].pNodeDAO[0][k]->sSQL, 0, MAX_SQL_LEN);
					//
					if(k == 0)
					{
						SAFESTRCPY(m_tTableDAO[i].pNodeDAO[0][k]->sSQL,MAX_SQL_LEN,tLCR.m_sSQL);
						m_tTableDAO[i].pNodeDAO[0][k]->dataColCount = tLCR.m_vColms.size()+tLCR.m_vWColms.size();
                        m_tTableDAO[i].pNodeDAO[0][k]->pDAO = new(std::nothrow) TMdbDAOBase();
						if(m_tTableDAO[i].pNodeDAO[0][k]->pDAO == NULL)
						{
							TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                    		return NULL;
						}
                        m_tTableDAO[i].pNodeDAO[0][k]->pDAO->SetSQL(tLCR.m_sSQL);
					}
                    else
                	{
                		memset(sSQL, 0, sizeof(sSQL));
						GetSQL(tLCR, k, sSQL);
						//SAFESTRCPY(m_tTableDAO[i].pNodeDAO[0][k]->sSQL,sizeof(m_tTableDAO[i].pNodeDAO[0][k]->sSQL),sSQL);
						m_tTableDAO[i].pNodeDAO[0][k]->dataColCount = m_tTableDAO[i].pNodeDAO[0][0]->dataColCount*k;
                        m_tTableDAO[i].pNodeDAO[0][k]->pDAO = new(std::nothrow) TMdbDAOBase();
						if(m_tTableDAO[i].pNodeDAO[0][k]->pDAO == NULL)
						{
							TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                    		return NULL;
						}
                        m_tTableDAO[i].pNodeDAO[0][k]->pDAO->SetSQL(sSQL);
                	}
        		}
				#elif DB_ORACLE
				m_tTableDAO[i].pNodeDAO[0][0] = new(std::nothrow) TMdbNodeDAO();
                if(m_tTableDAO[i].pNodeDAO[0][0] == NULL)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                    return NULL;
                }

                m_tTableDAO[i].pNodeDAO[0][0]->iOper = tLCR.m_iSqlType;
				if(m_tTableDAO[i].pNodeDAO[0][0]->sSQL == NULL)
				{
					m_tTableDAO[i].pNodeDAO[0][0]->sSQL = new(std::nothrow) char[MAX_SQL_LEN];
					if(m_tTableDAO[i].pNodeDAO[0][0]->sSQL == NULL)
					{
						TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                		return NULL;
					}
				}
				//memset(m_tTableDAO[i].pNodeDAO[0][0]->sSQL, 0, MAX_SQL_LEN);
                SAFESTRCPY(m_tTableDAO[i].pNodeDAO[0][0]->sSQL,MAX_SQL_LEN,tLCR.m_sSQL);
                m_tTableDAO[i].pNodeDAO[0][0]->pDAO = new(std::nothrow) TMdbDAOBase();
				if(m_tTableDAO[i].pNodeDAO[0][0]->pDAO == NULL)
				{
					TADD_ERROR(ERROR_UNKNOWN,"Out of memory.");
                    return NULL;
				}
                m_tTableDAO[i].pNodeDAO[0][0]->pDAO->SetSQL(tLCR.m_sSQL);
				#endif
                TADD_FUNC("TMdbDAO::CreateDAO() : Finish(Create[%d,%d] OK).", i, 0);
                return m_tTableDAO[i].pNodeDAO[0]; 
            }
        }

        if(iPos == -1)
        {
            TADD_ERROR(ERROR_UNKNOWN,"too many Tables, Max=%d.", MAX_TABLE_COUNTS);
            return NULL;    
        }

        TADD_FUNC("TMdbDAO::CreateDAO() : Finish.");

        return NULL;
    }


    //查找DAO
    TMdbNodeDAO** TMdbDAO::FindDAO(TMdbLCR& tLCR)
    {
        TADD_FUNC("TMdbDAO::FindDAO() : Start.");
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            //首先找到操作的表
            TADD_DETAIL("TMdbDAO::FindDAO() : m_pOneRecord->m_iTableName=%s, m_tTableDAO[%d].iTableID=%s.", 
            tLCR.m_sTableName.c_str(), i, m_tTableDAO[i].sTableName.c_str());
            if(m_tTableDAO[i].sTableName == tLCR.m_sTableName)
            {
                for(int j=0; j<MAX_DAO_COUNTS; ++j)
                {
                    if(m_tTableDAO[i].pNodeDAO[j][0] != NULL)
                    {
                        //然后找到对应的操作类型
                        TADD_DETAIL("TMdbDAO::FindDAO([%d,%d]) : iOper=[%d].", i, j, m_tTableDAO[i].pNodeDAO[j][0]->iOper); 
                        if(m_tTableDAO[i].pNodeDAO[j][0]->iOper == tLCR.m_iSqlType)
                        {
                            TADD_DETAIL("TMdbDAO::FindDAO() : \n\tThis-SQL=[%s]\n\tObj-SQL=[%s].", m_tTableDAO[i].pNodeDAO[j][0]->sSQL, tLCR.m_sSQL); 
                            if(TMdbNtcStrFunc::StrNoCaseCmp(m_tTableDAO[i].pNodeDAO[j][0]->sSQL, tLCR.m_sSQL) == 0)
                            {
                                TADD_FUNC("TMdbDAO::FindDAO() : Finish(Find[%d,%d]).", i, j);   
                                return m_tTableDAO[i].pNodeDAO[j];    
                            }
                        }   
                    }
                }
                break;
            }
        }
        TADD_FUNC("TMdbDAO::FindDAO() : Finish(Not find).");    
        return NULL;
    }

	int TMdbDAO::PushData(TMdbLCR & tLcr)
    {
        int iRet = 0;
        int iPos = 0;
        TMdbTable* pTable = NULL;
        if (m_pShmDSN != NULL)
        {
            pTable = m_pShmDSN->GetTableByName(tLcr.m_sTableName.c_str());
        }
        else
        {
            pTable = m_pMdbConfig->GetTableByName(tLcr.m_sTableName.c_str());
        }
        CHECK_OBJ(pTable);
        std::vector<TLCRColm>::iterator itor = tLcr.m_vColms.begin();
        for (; itor != tLcr.m_vColms.end(); ++itor)
        {
            TMdbColumn * pColumn = pTable->GetColumnByName(itor->m_sColmName.c_str());
            m_tData[iPos].iLen  = pColumn->iColumnLen;
            m_tData[iPos].iType = pColumn->iDataType;
            m_tData[iPos].isNull = itor->m_bNull?-1:0;
            SAFESTRCPY(m_tData[iPos].sName,sizeof(m_tData[iPos].sName),itor->m_sColmName.c_str());
            SAFESTRCPY(m_tData[iPos].sPName,sizeof(m_tData[iPos].sPName),itor->m_sColmName.c_str());
            SAFESTRCPY(m_tData[iPos].sValue,sizeof(m_tData[iPos].sValue),itor->m_sColmValue.c_str());
            iPos++;
        }

        itor = tLcr.m_vWColms.begin();
        for (; itor != tLcr.m_vWColms.end(); ++itor)
        {
            TMdbColumn * pColumn = pTable->GetColumnByName(itor->m_sColmName.c_str());
            m_tData[iPos].iLen  = pColumn->iColumnLen;
            m_tData[iPos].iType = pColumn->iDataType;
            m_tData[iPos].isNull = itor->m_bNull?-1:0;
            SAFESTRCPY(m_tData[iPos].sName,sizeof(m_tData[iPos].sName),itor->m_sColmName.c_str());
            SAFESTRCPY(m_tData[iPos].sPName,sizeof(m_tData[iPos].sPName),itor->m_sColmName.c_str());
            SAFESTRCPY(m_tData[iPos].sValue,sizeof(m_tData[iPos].sValue),itor->m_sColmValue.c_str());
            iPos++;
        }
        return iRet;
    }

    int TMdbDAO::PushData(TMdbLCR & tLcr, TMdbNodeDAO* pNodeDao, int iCount)
    {
        int iRet = 0;
        int iPos = 0;
        TMdbTable* pTable = NULL;
        if (m_pShmDSN != NULL)
        {
            pTable = m_pShmDSN->GetTableByName(tLcr.m_sTableName.c_str());
        }
        else
        {
            pTable = m_pMdbConfig->GetTableByName(tLcr.m_sTableName.c_str());
        }
        CHECK_OBJ(pTable);
		
        std::vector<TLCRColm>::iterator itor;
		
		if(tLcr.m_iSqlType == TK_UPDATE)
		{
			itor = tLcr.m_vWColms.begin();
	        for (; itor != tLcr.m_vWColms.end(); ++itor)
	        {
	            TMdbColumn * pColumn = pTable->GetColumnByName(itor->m_sColmName.c_str());
				if(pNodeDao->m_tData[iPos] == NULL)
				{
					pNodeDao->m_tData[iPos] = new(std::nothrow) TMdbData();
					CHECK_OBJ(pNodeDao->m_tData[iPos]);
				}
	            pNodeDao->m_tData[iPos]->iLen  = pColumn->iColumnLen;
	            pNodeDao->m_tData[iPos]->iType = pColumn->iDataType;
	            pNodeDao->m_tData[iPos]->isNull = itor->m_bNull?-1:0;
	            SAFESTRCPY(pNodeDao->m_tData[iPos]->sName,sizeof(pNodeDao->m_tData[iPos]->sName),itor->m_sColmName.c_str());
				if(iCount > 0)
				{
					snprintf(pNodeDao->m_tData[iPos]->sPName,sizeof(pNodeDao->m_tData[iPos]->sPName),"%s%d",itor->m_sColmName.c_str(),iCount);
				}
	            else
            	{
        		    SAFESTRCPY(pNodeDao->m_tData[iPos]->sPName,sizeof(pNodeDao->m_tData[iPos]->sPName),itor->m_sColmName.c_str());
            	}
	            SAFESTRCPY(pNodeDao->m_tData[iPos]->sValue,sizeof(pNodeDao->m_tData[iPos]->sValue),itor->m_sColmValue.c_str());
	            iPos++;
	        }
		}
		itor = tLcr.m_vColms.begin();
        for (; itor != tLcr.m_vColms.end(); ++itor)
        {
            TMdbColumn * pColumn = pTable->GetColumnByName(itor->m_sColmName.c_str());
			if(pNodeDao->m_tData[iPos] == NULL)
			{
				pNodeDao->m_tData[iPos] = new(std::nothrow) TMdbData();
				CHECK_OBJ(pNodeDao->m_tData[iPos]);
			}
            pNodeDao->m_tData[iPos]->iLen  = pColumn->iColumnLen;
            pNodeDao->m_tData[iPos]->iType = pColumn->iDataType;
            pNodeDao->m_tData[iPos]->isNull = itor->m_bNull?-1:0;
            SAFESTRCPY(pNodeDao->m_tData[iPos]->sName,sizeof(pNodeDao->m_tData[iPos]->sName),itor->m_sColmName.c_str());
			if(iCount > 0)
			{
				snprintf(pNodeDao->m_tData[iPos]->sPName,sizeof(pNodeDao->m_tData[iPos]->sPName),"%s%d",itor->m_sColmName.c_str(),iCount);
			}
            else
        	{
    		    SAFESTRCPY(pNodeDao->m_tData[iPos]->sPName,sizeof(pNodeDao->m_tData[iPos]->sPName),itor->m_sColmName.c_str());
        	}
            SAFESTRCPY(pNodeDao->m_tData[iPos]->sValue,sizeof(pNodeDao->m_tData[iPos]->sValue),itor->m_sColmValue.c_str());
            iPos++;
        }
		if(tLcr.m_iSqlType != TK_UPDATE)
		{
	        itor = tLcr.m_vWColms.begin();
	        for (; itor != tLcr.m_vWColms.end(); ++itor)
	        {
	            TMdbColumn * pColumn = pTable->GetColumnByName(itor->m_sColmName.c_str());
				if(pNodeDao->m_tData[iPos] == NULL)
				{
					pNodeDao->m_tData[iPos] = new(std::nothrow) TMdbData();
					CHECK_OBJ(pNodeDao->m_tData[iPos]);
				}
	            pNodeDao->m_tData[iPos]->iLen  = pColumn->iColumnLen;
	            pNodeDao->m_tData[iPos]->iType = pColumn->iDataType;
	            pNodeDao->m_tData[iPos]->isNull = itor->m_bNull?-1:0;
	            SAFESTRCPY(pNodeDao->m_tData[iPos]->sName,sizeof(pNodeDao->m_tData[iPos]->sName),itor->m_sColmName.c_str());
				if(iCount > 0)
				{
					snprintf(pNodeDao->m_tData[iPos]->sPName,sizeof(pNodeDao->m_tData[iPos]->sPName),"%s%d",itor->m_sColmName.c_str(),iCount);
				}
	            else
            	{
        		    SAFESTRCPY(pNodeDao->m_tData[iPos]->sPName,sizeof(pNodeDao->m_tData[iPos]->sPName),itor->m_sColmName.c_str());
            	}
	            SAFESTRCPY(pNodeDao->m_tData[iPos]->sValue,sizeof(pNodeDao->m_tData[iPos]->sValue),itor->m_sColmValue.c_str());
	            iPos++;
	        }
		}
        return iRet;
    }

	int TMdbDAO::GetSelectSQL(TMdbLCR & tLcr)
	{
		int iRet = 0;
		TMdbTable* pTable = NULL;
		if (m_pShmDSN != NULL)
		{
			pTable = m_pShmDSN->GetTableByName(tLcr.m_sTableName.c_str());
		}
		else
		{
			pTable = m_pMdbConfig->GetTableByName(tLcr.m_sTableName.c_str());
		}
		
		CHECK_OBJ(pTable);
		tLcr.GetSelectSQL(pTable);
		return iRet;
	}


    int TMdbDAO::GetSQL(TMdbLCR & tLcr)
    {
        int iRet = 0;
        TMdbTable* pTable = NULL;
        if (m_pShmDSN != NULL)
        {
            pTable = m_pShmDSN->GetTableByName(tLcr.m_sTableName.c_str());
        }
        else
        {
            pTable = m_pMdbConfig->GetTableByName(tLcr.m_sTableName.c_str());
        }
        
        CHECK_OBJ(pTable);
        tLcr.GetSQL(pTable);
        return iRet;
    }
	int TMdbDAO::GetSQL(TMdbLCR & tLcr, int reptNum, char* sSQL)
	{
		int iRet = 0;
		TMdbTable* pTable = NULL;
		if (m_pShmDSN != NULL)
		{
			pTable = m_pShmDSN->GetTableByName(tLcr.m_sTableName.c_str());
		}
		else
		{
			pTable = m_pMdbConfig->GetTableByName(tLcr.m_sTableName.c_str());
		}
		
		CHECK_OBJ(pTable);
		tLcr.GetSQL(pTable, reptNum, sSQL);
		return iRet;
	}


    /******************************************************************************
    * 函数名称	:  Commit()
    * 函数描述	:  向Oracle提交数据  
    * 输入		:  无 
    * 输出		:  无
    * 返回值	:  成功返回0，数据问题返回-1, Oracle断开返回1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbDAO::Commit(bool bCommitFlag)
    {
        TADD_FUNC("TMdbDAO::Commit() : Start.");
        int iRet = 0;
        int arrOperOrder[] = {OP_Delete,OP_Insert,OP_Update};//先delete 再insert最后再update
        TMdbDAOBase * pDAOBase = NULL;
        for(int k = 0;k < 3 ;++k)
        {
            for(int i=0; i<MAX_TABLE_COUNTS; ++i)
            {
                for(int j=0; j<MAX_DAO_COUNTS; ++j)
                {
                	if(m_tTableDAO[i].pNodeDAO[j][0] !=NULL \
						&& m_tTableDAO[i].pNodeDAO[j][0]->iOper == arrOperOrder[k])
            		{
					#ifdef DB_MYSQL
            			if(m_tTableDAO[i].pNodeDAO[j][0]->iCount > 0 )
            			{
							pDAOBase = m_tTableDAO[i].pNodeDAO[j][m_tTableDAO[i].pNodeDAO[j][0]->iCount-1]->pDAO;
		            		
							pDAOBase->StartData();
							for(int m = 0; m<m_tTableDAO[i].pNodeDAO[j][0]->iCount; m++)
							{
						        for(int n=0; n<m_tTableDAO[i].pNodeDAO[j][0]->dataColCount; ++n)
						        {
						            if(m_tTableDAO[i].pNodeDAO[j][m]->m_tData[n] != NULL) pDAOBase->AddData(m_tTableDAO[i].pNodeDAO[j][m]->m_tData[n]);
						        }
							}
							pDAOBase->EndData();
							//
							m_tTableDAO[i].pNodeDAO[j][0]->iCount = 0;
            			}
						for(int m = 0; m < MAX_MYSQL_ARRAY_SIZE; m++)
						{
							pDAOBase = m_tTableDAO[i].pNodeDAO[j][m]->pDAO;
		                    iRet = pDAOBase->Execute(m_pDBLink);      
		                    if(iRet < 0)
		                    {
		                        m_pDBLink->Rollback();
		                        break;
		                    } 
		                    else if(iRet > 0)
		                    {
		                        iRet = 1;
		                        break;
		                    }
						}
					#elif DB_ORACLE
                        //然后找到对应的操作类型
						pDAOBase = m_tTableDAO[i].pNodeDAO[j][0]->pDAO;
	                    iRet = pDAOBase->Execute(m_pDBLink);      
	                    if(iRet < 0)
	                    {
	                        m_pDBLink->Rollback();
	                        break;
	                    } 
	                    else if(iRet > 0)
	                    {
	                        iRet = 1;
	                        break;
	                    }
					#endif
                    }
                }    
                if(iRet != 0)
                break;
            }
            if(iRet != 0)
            break;
        }    
        if(iRet == 0 && bCommitFlag == true)
        {
            m_pDBLink->Commit();
        }
        //清空数据
        Clear();
        TADD_DETAIL("[%s : %d] : TMdbDAO::Commit() : iRet=%d.", __FILE__, __LINE__, iRet);
        TADD_FUNC("TMdbDAO::Commit() : Finish.");
        return iRet;
    }

    void TMdbDAO::Clear()
    {
        TADD_DETAIL("TMdbDAO::Clear() : Start.");
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            for(int j=0; j<MAX_DAO_COUNTS; ++j)
            {
            	for(int k = 0; k < MAX_MYSQL_ARRAY_SIZE; k++)
            	{
	                if(m_tTableDAO[i].pNodeDAO[j][k] != NULL)
	                {
                    	m_tTableDAO[i].pNodeDAO[j][k]->pDAO->ClearArrayData();    
                	}
                }
            }
            m_tTableDAO[i].iLastOperType = -1;
        }    	
        TADD_DETAIL("TMdbDAO::Clear() : Finish.");
    }
    
    /******************************************************************************
    * 函数名称	:  ClearDAOByTableId
    * 函数描述	:  根据tableid清理dao
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbDAO::ClearDAOByTableId(const int iTableId)
    {
        /*
        int i = 0;
        for(i = 0; i< MAX_TABLE_COUNTS;++i)
        {
            if(iTableId == m_tTableDAO[i].iTableID)
            {//find
                m_tTableDAO[i].Clear();
            }
        }
        */
        return 0;
    }


//}
