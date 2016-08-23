/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbFlushDao.h		
*@Description: 将同步数据刷新到MDB中
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
#include "Replication/mdbRepFlushDao.h"

//namespace QuickMDB
//{
    TFlushDAONode::TFlushDAONode()
    {
        memset(m_sSQL,0x00,sizeof(m_sSQL));
		memset(m_sMdbSelSQL,0x00,sizeof(m_sMdbSelSQL));
		memset(m_sMdbUptSQL,0x00,sizeof(m_sMdbUptSQL));
        m_pQuery = NULL;
		m_pMdbSelQuery =  NULL;
		m_pMdbUptQuery =  NULL;
		
    }
    TFlushDAONode::~TFlushDAONode()
    {
        Clear();
    }

    void TFlushDAONode::Clear()
    {
        memset(m_sSQL,0x00,sizeof(m_sSQL));
		memset(m_sMdbSelSQL,0x00,sizeof(m_sMdbSelSQL));
		memset(m_sMdbUptSQL,0x00,sizeof(m_sMdbUptSQL));
		
        SAFE_DELETE(m_pQuery);
		SAFE_DELETE(m_pMdbSelQuery);
		SAFE_DELETE(m_pMdbUptQuery);
		
    }

    TMdbQuery* TFlushDAONode::CreateQuery(const char * sSQL,int iNodePos,TMdbDatabase * ptDB)
    {
        if(m_sSQL[0] == 0 || TMdbNtcStrFunc::StrNoCaseCmp(m_sSQL,sSQL) != 0)
        {
            SAFE_DELETE(m_pQuery);//清理前一次的
            m_sSQL[0] = 0;
            try
            {
                m_pQuery = ptDB->CreateDBQuery();
                m_pQuery->SetSQL(sSQL,QUERY_NO_ORAFLUSH |QUERY_NO_REDOFLUSH|QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK, 0);
            }
            catch(TMdbException& e)
            {
                SAFE_DELETE(m_pQuery);

                TADD_ERROR(ERR_DB_NOT_CONNECTED,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());
                TADD_FUNC("Finish.");
                return NULL;
            }
            catch(...)
            {
                SAFE_DELETE(m_pQuery);

                TADD_ERROR(ERR_DB_NOT_CONNECTED,"Unknown error!\n");
                TADD_FUNC("Finish.");
                return NULL;
            }
            
            SAFESTRCPY(m_sSQL,sizeof(m_sSQL),sSQL);
        }

        TADD_FUNC("Finish.");
        return m_pQuery;
    }

    TRepFlushDao::TRepFlushDao():m_pDataBase(NULL)
    {
        for(int i = 0; i < MAX_FLUSN_DAO_COUNTS; i++)
        {
            m_arrDaoNode[i] = NULL;
        }
        memset(m_sSQL, 0, sizeof(m_sSQL));
        m_pRecdQry = NULL;
       
    }

    TRepFlushDao::~TRepFlushDao()
    {
        for (int i = 0; i<MAX_FLUSN_DAO_COUNTS; i++)
        {
            SAFE_DELETE(m_arrDaoNode[i]);
        }
        m_arPKs.clear();
        SAFE_DELETE(m_pRecdQry);
        
    }

    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	: 获取该表的主键名称列表
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepFlushDao::Init(std::string strTableName, TMdbDatabase* pDataBase)throw (TMdbException)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_strTableName = strTableName;
        CHECK_OBJ(pDataBase);
        m_pDataBase = pDataBase;
        CHECK_RET(InitTabProperty(), "get column info failed.");
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TRepFlushDao::InitTabProperty()throw (TMdbException)
    {
        TADD_FUNC("start.");
        int iRet = 0;
        static char sGetTableIDSQL[] = "SELECT  PRIMARY_KEY FROM DBA_TABLES WHERE TABLE_NAME = :TABLE_NAME";
        static char sGetPkPos[] = "SELECT COLUMN_NAME, DATA_POS ,data_type,nullable,data_len FROM DBA_COLUMN WHERE TABLE_NAME = :TABLE_NAME";
        TMdbQuery *pQuery = NULL;
        try
        {
             TMdbNtcSplit tSplit;
             
            pQuery = m_pDataBase->CreateDBQuery();
            CHECK_OBJ(pQuery);
            pQuery->SetSQL(sGetTableIDSQL);
            pQuery->SetParameter(0, m_strTableName.c_str());
            pQuery->Open();
            if (pQuery->Next())
            {
                std::string strPkPos = pQuery->Field(0).AsString();
                tSplit.SplitString(strPkPos.c_str(), ',');
            }
            else
            {
                CHECK_RET(ERR_TAB_NO_TABLE, "Cannot find table [%s]", m_strTableName.c_str());
            }
            pQuery->CloseSQL();

            bool bIsPk = false;
            TMdbColumn tColm;
            pQuery->SetSQL(sGetPkPos);
            pQuery->SetParameter(0, m_strTableName.c_str());
            pQuery->Open();
            while(pQuery->Next())
            {
                bIsPk = false;
                tColm.Clear();
                SAFESTRCPY(tColm.sName, sizeof(tColm.sName), pQuery->Field("column_name").AsString());
                
                char sDataType[MAX_NAME_LEN]={0};
                SAFESTRCPY(sDataType, sizeof(sDataType), pQuery->Field("data_type").AsString());
                tColm.iDataType = GetColumnDataType(sDataType);
                if(tColm.iDataType < 0)
                {
                    iRet = ERR_TAB_COLUMN_DATA_TYPE_INVALID;
                    TADD_ERROR(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"Column[%s]'s Data type[%s] is invalid ",tColm.sName,sDataType);
                    break;
                }
                
                tColm.iColumnLen = pQuery->Field("data_len").AsInteger();
                tColm.m_bNullable = (0 == TMdbNtcStrFunc::StrNoCaseCmp(pQuery->Field("nullable").AsString(), "Y"))?true:false;
                
                tColm.iPos  = pQuery->Field("data_pos").AsInteger();

                m_vColms.push_back(tColm);
                
                for (unsigned int i = 0; i<tSplit.GetFieldCount(); i++)
                {
                	if (tColm.iPos == atoi(tSplit[i]))
                	{
                		bIsPk = true;
                		m_arPKs.push_back(tColm.sName);
                		break;
                	}
                }
                
                if(!bIsPk)//非主键列
                {
                	m_arCols.push_back(tColm.sName);
                }
            }
        }    
        catch(TMdbException& e)
        {
            iRet = ERROR_UNKNOWN;
            TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());
        }
        catch(...)
        {
            iRet = ERROR_UNKNOWN;
            TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
        }
               
        SAFE_DELETE(pQuery);
        return iRet;
    }

    int TRepFlushDao::GetColumnDataType(const char* psDataType)
    {

        TADD_FUNC("Start.");
        int iRet = -1;
        CHECK_OBJ(psDataType);

        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psDataType, "NUMBER"))
        {
            iRet = DT_Int;
        }

        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psDataType, "CHAR"))
        {
            iRet = DT_Char;
        }

        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psDataType, "VARCHAR"))
        {
            iRet = DT_VarChar;
        }

        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psDataType, "DATESTAMP"))
        {
            iRet = DT_DateStamp;
        }

        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psDataType, "BLOB"))
        {
            iRet = DT_Blob;
        }

        TADD_DETAIL("DataType=[%s], return[%d]", psDataType, iRet);
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    
    TMdbColumn* TRepFlushDao::GetColumnInfo(const char* psName)
    {
        std::vector<TMdbColumn>::iterator itor = m_vColms.begin();
        for(; itor != m_vColms.end(); ++itor)
        {
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psName, itor->sName))
            {
                return &(*itor);
            }
        }

        return NULL;
    }
    


    /******************************************************************************
    * 函数名称	:  CreateQuery
    * 函数描述	:  创建query
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepFlushDao::GetQuery(TMdbQuery * &pQuery, TMdbRepRecd & tRecd, int * iDropIndex)throw (TMdbException)
    {
    	int iRet = 0;
        int iPos = 0;

        iRet = GetSQL(tRecd, tRecd.m_tHeadInfo.m_iSqlType, iDropIndex);
		if(iRet < 0)
		{
			TADD_ERROR(iRet, "Get SQL failed!");
			return iRet;
		}
		

        // pos = 0 为select query位置
        if(tRecd.m_tHeadInfo.m_iSqlType == TK_INSERT)
        {
            iPos = 0;
        }
        else if(tRecd.m_tHeadInfo.m_iSqlType  == TK_DELETE)
        {
            iPos = 1;
            
        }
        else if(TK_UPDATE == tRecd.m_tHeadInfo.m_iSqlType )
    	{//update 从2开始
    		int i = 0;
    		TFlushDAONode * pstNode = NULL;
    		for(i = 2;i<MAX_FLUSN_DAO_COUNTS;i++)
    		{
    			pstNode = m_arrDaoNode[i];
    			if(NULL != pstNode)
    			{
    				if(0 != pstNode->m_sSQL[0] && TMdbNtcStrFunc::StrNoCaseCmp(pstNode->m_sSQL,m_sSQL) == 0)
    				{//这条sql语句已被构造
    					break;
    				}
    			}
    			else
    			{//找不到，新建一个
    				break;
    			}
    		}
              if(i >= MAX_FLUSN_DAO_COUNTS)
              {//满了,随机取一个节点清空掉
                 srand((unsigned)time(0));
                 i = rand()%(MAX_FLUSN_DAO_COUNTS-3) +2;
                 SAFE_DELETE(m_arrDaoNode[i]);
              }
    		iPos = i;
    	}

        if(NULL == m_arrDaoNode[iPos])
        {
            m_arrDaoNode[iPos] = new(std::nothrow) TFlushDAONode();
            if(NULL == m_arrDaoNode[iPos])
            {
                TADD_ERROR(ERR_OS_NO_MEMROY,"new dao node failed.");
                pQuery = NULL;
            }
        }

        pQuery = m_arrDaoNode[iPos]->CreateQuery(m_sSQL,iPos,m_pDataBase);
        return iRet;
    }

    TMdbQuery* TRepFlushDao::GetRecdQuery()throw (TMdbException)
    {
        if(NULL != m_pRecdQry)
        {
            return m_pRecdQry;
        }
        
        char sWhere[MAX_SQL_LEN] = {0};

        std::vector<std::string>::iterator itor = m_arPKs.begin();
        for(; itor != m_arPKs.end(); ++itor)
        {
            sprintf(sWhere+strlen(sWhere),"  %s=:%s and",itor->c_str(),itor->c_str());
        }           

        sWhere[strlen(sWhere)-3] = '\0';//去除最后一个'and'
        
        sprintf(m_sSQL,"select * from %s where %s",m_strTableName.c_str(),sWhere);

        try
        {
            m_pRecdQry = m_pDataBase->CreateDBQuery();
            if(NULL == m_pRecdQry)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY, "new TMdbQuery failed.");
                return NULL;
            }

            m_pRecdQry->SetSQL(m_sSQL);
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(ERR_SQL_INVALID,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
            return NULL;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN, "Unknown error!\n");   
            return NULL;
        } 

        return m_pRecdQry;
    }
    TMdbQuery * TRepFlushDao::GetSelectQuery(TMdbRepRecd & tRecd)throw (TMdbException)
    {
        int iPos = 0;

        GetSQL(tRecd, TK_SELECT);

        int i = 0;
        TFlushDAONode * pstNode = NULL;
        for(i = 2;i<MAX_FLUSN_DAO_COUNTS;i++)
        {
            pstNode = m_arrDaoNode[i];
            if(NULL != pstNode)
            {
                if(0 != pstNode->m_sSQL[0] && TMdbNtcStrFunc::StrNoCaseCmp(pstNode->m_sSQL,m_sSQL) == 0)
                {//这条sql语句已被构造
                        break;
                }
            }
            else
            {//找不到，新建一个
                break;
            }
        }
        if(i >= MAX_FLUSN_DAO_COUNTS)
        {//满了,随机取一个节点清空掉
            srand((unsigned)time(0));
            i = rand()%(MAX_FLUSN_DAO_COUNTS-3) +2;
            SAFE_DELETE(m_arrDaoNode[i]);
        }
        iPos = i;

        if(NULL == m_arrDaoNode[iPos])
        {
            m_arrDaoNode[iPos] = new(std::nothrow) TFlushDAONode();
            if(NULL == m_arrDaoNode[iPos])
            {
                TADD_ERROR(ERR_OS_NO_MEMROY,"new dao node failed.");
                return  NULL;
            }
        }

        return m_arrDaoNode[iPos]->CreateQuery(m_sSQL,iPos,m_pDataBase);
        
    }

        /******************************************************************************
    * 函数名称	:  SetSQL
    * 函数描述	:  拼接sql，并设置指针pNode
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepFlushDao::SetSQL(TFlushDAONode *pNode, int iSqlType)throw (TMdbException)
    {
        int iRet = 0;
        CHECK_OBJ(pNode);
        CHECK_OBJ(pNode->m_pQuery);
        int iSqlLen = 0;
        
        switch(iSqlType)
        {
        case TK_INSERT:
            {
                snprintf(pNode->m_sSQL, MAX_SQL_LEN, "INSERT INTO %s(", m_strTableName.c_str());
                
                for (unsigned int i = 0; i< m_arPKs.size(); i++)
                {
                    iSqlLen = static_cast<int>(strlen(pNode->m_sSQL));
                    if (0 == i)
                    {
                        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, "%s", m_arPKs[i].c_str());
                    }
                    else
                    {
                        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ",%s", m_arPKs[i].c_str());
                    }          
                }
                for (unsigned int i= 0; i<m_arCols.size(); i++)
                {
                    iSqlLen = static_cast<int>(strlen(pNode->m_sSQL));
                    snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ",%s", m_arCols[i].c_str());
                }
                iSqlLen = static_cast<int>(strlen(pNode->m_sSQL));
                snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ") VALUES(");
                for (unsigned int i = 0; i< m_arPKs.size(); i++)
                {
                    iSqlLen = static_cast<int>(strlen(pNode->m_sSQL));
                    if (0 == i)
                    {
                        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ":%s", m_arPKs[i].c_str());
                    }
                    else
                    {
                        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ",:%s", m_arPKs[i].c_str());
                    }          
                }
                for (unsigned int i = 0; i<m_arCols.size(); i++)
                {
                    iSqlLen = static_cast<int>(strlen(pNode->m_sSQL));
                    snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ",:%s", m_arCols[i].c_str());
                }
                iSqlLen = static_cast<int>(strlen(pNode->m_sSQL));
                snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ")");
                break;
            }
        case TK_UPDATE:
            {
                snprintf(pNode->m_sSQL, MAX_SQL_LEN, "UPDATE %s SET ", m_strTableName.c_str());

                for (unsigned int i = 0; i<m_arCols.size(); i++)
                {
                    iSqlLen = static_cast<int>(strlen(pNode->m_sSQL));
                    if (0 == i)
                    {
                        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, "%s = :%s", m_arCols[i].c_str(), m_arCols[i].c_str());
                    }
                    else
                    {
                        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ", %s = :%s", m_arCols[i].c_str(), m_arCols[i].c_str());
                    }                    
                }
                iSqlLen = static_cast<int>(strlen(pNode->m_sSQL));
                snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, " WHERE ");
                for (unsigned int i = 0; i< m_arPKs.size(); i++)
                {
                    iSqlLen = static_cast<int>(strlen(pNode->m_sSQL));
                    if (0 == i)
                    {
                        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, "%s = :%s", m_arPKs[i].c_str(), m_arPKs[i].c_str());
                    }
                    else
                    {
                        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ",%s = :%s", m_arPKs[i].c_str(), m_arPKs[i].c_str());
                    }          
                }
               
                break;
            }
        case TK_DELETE:
            {
                snprintf(pNode->m_sSQL, MAX_SQL_LEN, "DELETE FROM %s WHERE", m_strTableName.c_str());
                for (unsigned int i = 0; i< m_arPKs.size(); i++)
                {
                    iSqlLen = static_cast<int>(strlen(pNode->m_sSQL));
                    if (0 == i)
                    {
                        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, "%s = :%s", (m_arPKs[i]).c_str(), (m_arPKs[i]).c_str());
                    }
                    else
                    {
                        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ",%s = :%s", m_arPKs[i].c_str(), m_arPKs[i].c_str());
                    }          
                }
                break;
            }
        default:
            {
                CHECK_RET(ERR_APP_INVALID_PARAM,"Unknown SQL type.");
            }  
        }

        pNode->m_pQuery->CloseSQL();
        pNode->m_pQuery->SetSQL(pNode->m_sSQL,QUERY_NO_ORAFLUSH |QUERY_NO_REDOFLUSH|QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK, 0);
        return iRet;
    }

    int TRepFlushDao::GetSQL(TMdbRepRecd& tRecd, int iSqlType, int * iDropIndex)
    {
    	int iRet = 0;
        TMdbColumn* pColm = NULL;
        
		int paramIndex = 0;
        switch(iSqlType)
        {
        case TK_INSERT:
            {
                char sColm[MAX_SQL_LEN] = {0};
                char sValue[MAX_VALUE_LEN]= {0};
                
                std::vector<TRepColm>::iterator itor = tRecd.m_vColms.begin();
                for (; itor != tRecd.m_vColms.end(); ++itor)
                {
                	if(iDropIndex != NULL && iDropIndex[paramIndex++] == 1)
            		{
						continue;
            		}
                    pColm = GetColumnInfo(itor->m_sColmName.c_str());
                    if(NULL == pColm)
                    {
                        continue;
                    }
                    
                   sprintf(sColm+strlen(sColm)," %s,",itor->m_sColmName.c_str());
                    if(DT_DateStamp == pColm->iDataType)
                    {
                        sprintf(sValue+strlen(sValue)," to_date(:%s, 'YYYYMMDDHH24MISS'),",itor->m_sColmName.c_str());
                    }
                    else
                    {
                        sprintf(sValue+strlen(sValue)," :%s,",itor->m_sColmName.c_str());
                    }
                }

                sColm[strlen(sColm) -1] = '\0';//去除最后一个，
                sValue[strlen(sValue) -1]   = '\0';
                sprintf(m_sSQL,"insert into %s (%s)values(%s)",m_strTableName.c_str(),sColm,sValue);

                break;
            }
        case TK_UPDATE:
            {
                char sWhere[MAX_SQL_LEN] = {0};
                GetWhereSql(tRecd,sWhere);

                char sSet[MAX_SQL_LEN] ={0};
                std::vector<TRepColm>::iterator itor = tRecd.m_vColms.begin();
                for(; itor != tRecd.m_vColms.end(); ++itor)
                {
                	if(iDropIndex != NULL && iDropIndex[paramIndex++] == 1)
            		{
						continue;
            		}
                    pColm = GetColumnInfo(itor->m_sColmName.c_str());
                    if(NULL == pColm)
                    {
                        continue;
                    }

                    if(DT_DateStamp == pColm->iDataType)
                    {
                        sprintf(sSet+strlen(sSet),"  %s=to_date(:%s, 'YYYYMMDDHH24MISS'),",pColm->sName,pColm->sName);
                    }
                    else
                    {
                        sprintf(sSet+strlen(sSet),"  %s=:%s,",pColm->sName,pColm->sName);
                    }
                }

                sSet[strlen(sSet) -1]  = '\0';
                sprintf(m_sSQL,"update %s set %s where %s",m_strTableName.c_str(),sSet,sWhere);
               
                break;
            }
        case TK_DELETE:
            {
                char sWhere[MAX_SQL_LEN] = {0};
                GetWhereSql(tRecd,sWhere);
                sprintf(m_sSQL,"delete from %s where %s",m_strTableName.c_str(),sWhere);
                
                break;
            }
        case TK_SELECT:
            {
                char sWhere[MAX_SQL_LEN] = {0};
                GetWhereSql(tRecd,sWhere);
                
                char sSet[1024] = {0};
                std::vector<TRepColm>::iterator itor = tRecd.m_vColms.begin();
                for(; itor != tRecd.m_vColms.end(); ++itor)
                {
                    pColm = GetColumnInfo(itor->m_sColmName.c_str());
                    if(NULL == pColm)
                    {
                        continue;
                    }
                    sprintf(sSet+strlen(sSet)," %s,",pColm->sName);
                }
                sSet[strlen(sSet) -1]  = '\0';
                sprintf(m_sSQL,"select %s from %s where %s",sSet,m_strTableName.c_str(),sWhere);
				break;
            }
        default:
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM,"Unknown SQL type[%d].",iSqlType);
				return -1;
            }  
        }

        return iRet;
    }

    void TRepFlushDao::GetWhereSql(TMdbRepRecd& tRecd, char* psWhere)
    {
        TMdbColumn* pColm = NULL;
        std::vector<TRepColm>::iterator itor = tRecd.m_vWColms.begin();
        for(; itor !=tRecd.m_vWColms.end(); ++itor )
        {
            pColm = GetColumnInfo(itor->m_sColmName.c_str());
            if(NULL == pColm)
            {
                continue;
            }

            if(DT_DateStamp == pColm->iDataType)
            {
                sprintf(psWhere+strlen(psWhere),"  %s=to_date(:%s, 'YYYYMMDDHH24MISS') and",pColm->sName,pColm->sName);
            }
            else
            {
                sprintf(psWhere+strlen(psWhere),"  %s=:%s and",pColm->sName,pColm->sName);
            }
        }

        psWhere[strlen(psWhere)-3] = '\0';//去除最后一个'and'
        return;
    }

    TRepLoadDao::TRepLoadDao():m_pNode(NULL), m_pDataBase(NULL),m_pMdbCfg(NULL)
    {
        
    }

    TRepLoadDao::~TRepLoadDao()
    {
        //SAFE_DELETE(m_pNode);
    }

    int TRepLoadDao::Init(TMdbDatabase *pDataBase,TMdbConfig* pMdbCfg)
    {
        int iRet = 0;
        CHECK_OBJ(pMdbCfg);
        m_pMdbCfg = pMdbCfg;
        CHECK_OBJ(pDataBase);
        m_pDataBase = pDataBase;
        m_pNode = new(std::nothrow)TFlushDAONode();
        CHECK_OBJ(m_pNode);
        m_pNode->m_pQuery = pDataBase->CreateDBQuery();
		
		m_pNode->m_pMdbSelQuery = pDataBase->CreateDBQuery();
		m_pNode->m_pMdbUptQuery = pDataBase->CreateDBQuery();
		
        CHECK_OBJ(m_pNode->m_pQuery);        
        return iRet;
    }

  	//获取主键的所在序号
  	int TRepLoadDao::GetPrimaryKey(std::vector<int > &  vKeyNo)
  	{
  		//prkey jizhu
  		TMdbTable* pTable =m_pMdbCfg->GetTable(m_strTableName.c_str());
		for(int i=0; i<(pTable->m_tPriKey).iColumnCounts; i++)
		{
			//m_priCols.push_back(pTable->tColumn[pTable->m_tPriKey->iColumnNo[i]].sName);
			vKeyNo.push_back((pTable->m_tPriKey).iColumnNo[i]);
		}
	
  		return 0;

  	}

    TMdbQuery* TRepLoadDao::GetQuery(std::string strTableName, bool bSelect, const char* sRoutinglist /* = NULL */)throw (TMdbException)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_strTableName = strTableName;
        m_arCols.clear();
        /*
        static char sGetTableIDSQL[] = "SELECT TABLE_ID FROM DBA_TABLES WHERE TABLE_NAME = :TABLE_NAME";
        static char sGetCols[] = "SELECT COLUMN_NAME  FROM DBA_COLUMN WHERE TABLE_ID = :TABLE_ID";
        TMdbQuery *pQuery = m_pDataBase->CreateDBQuery();
        if (NULL == pQuery)
        {
            return NULL;
        }
        pQuery->SetSQL(sGetTableIDSQL);
        pQuery->SetParameter("TABLE_NAME", strTableName.c_str());
        pQuery->Open();
        if (pQuery->Next())
        {
            int iTableID = pQuery->Field(0).AsInteger();
            pQuery->CloseSQL();
            pQuery->SetSQL(sGetCols);
            pQuery->SetParameter(0,iTableID);
            pQuery->Open();
            while (pQuery->Next())
            {
                m_arCols.push_back(pQuery->Field(0).AsString());
            }
        }
        else
        {
            TADD_ERROR(-1, "Cannot find table [%s]", strTableName.c_str());
            SAFE_DELETE(pQuery);
            return NULL;
        }
        SAFE_DELETE(pQuery);
        */
        TMdbTable* pTable =m_pMdbCfg->GetTable(m_strTableName.c_str());
        if (NULL == pTable)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM, "Table[%s] does not exist in Dsn[%s]", m_strTableName.c_str(), m_pMdbCfg->GetDSN()->sName);
            return NULL;
        }
        for(int i=0; i<pTable->iColumnCounts; ++i)
        {
            m_arCols.push_back(pTable->tColumn[i].sName);
        }

		
        if (bSelect)
        {
            iRet = SetSelectSQL(m_pNode, sRoutinglist);
            if (iRet != ERROR_SUCCESS)
            {
                TADD_ERROR(iRet, "SetSQL failed.");
                return NULL;
            }
        }
        else
        {
            iRet = SetInsertSQL(m_pNode);
            if (iRet != ERROR_SUCCESS)
            {
                TADD_ERROR(iRet, "SetSQL failed.");
                return NULL;
            }
        }
        
        TADD_FUNC("Finish.");
        return m_pNode->m_pQuery;
    }

	

    int TRepLoadDao::SetSelectSQL(TFlushDAONode *pNode, const char* sRoutinglist/* =NULL */)throw (TMdbException)
    {
        int iRet = 0;
       	size_t iSqlLen = 0;
        snprintf(pNode->m_sSQL, MAX_SQL_LEN, "SELECT ");

        for (unsigned int i = 0; i< m_arCols.size(); i++)
        {
            iSqlLen = strlen(pNode->m_sSQL);
            if (0 == i)
            {
                snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, "%s", m_arCols[i].c_str());
            }
            else
            {
                snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ",%s", m_arCols[i].c_str());
            }          
        }
      
        iSqlLen = strlen(pNode->m_sSQL);
        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, " FROM %s", m_strTableName.c_str());

        if (sRoutinglist!=NULL && atoi(sRoutinglist) != DEFALUT_ROUT_ID)//附加路由条件
        {
            TADD_NORMAL("TEST:  set where clause,routing_in (%s)", sRoutinglist);
            iSqlLen = strlen(pNode->m_sSQL);
            snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, " WHERE %s IN (%s)", m_pMdbCfg->GetDSN()->sRoutingName, sRoutinglist);
        }

        try
        {
            pNode->m_pQuery->CloseSQL();
            pNode->m_pQuery->SetSQL(pNode->m_sSQL);
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN, "Unknown error!\n");   
        } 
        return iRet;
    }


	
	TMdbQuery* TRepLoadDao::GetMdbSelQuery(std::string strTableName)throw (TMdbException)
	   {
		   TADD_FUNC("Start.");
		   int iRet = 0;
		   m_strTableName = strTableName;
		   m_arCols.clear();
		   m_priCols.clear();
		   TMdbTable* pTable =m_pMdbCfg->GetTable(m_strTableName.c_str());
		   if (NULL == pTable)
		   {
			   TADD_ERROR(ERR_APP_INVALID_PARAM, "Table[%s] does not exist in Dsn[%s]", m_strTableName.c_str(), m_pMdbCfg->GetDSN()->sName);
			   return NULL;
		   }
		   for(int i=0; i<pTable->iColumnCounts; ++i)
		   {
			   m_arCols.push_back(pTable->tColumn[i].sName);
		   }

		   //prkey jizhu
			for(int i=0; i<(pTable->m_tPriKey).iColumnCounts; i++)
			{
				m_priCols.push_back(pTable->tColumn[(pTable->m_tPriKey).iColumnNo[i]].sName);
			}
	
		  
			iRet = SetMdbSelectSQL(m_pNode);
			if (iRet != ERROR_SUCCESS)
			{
				TADD_ERROR(iRet, "SetSQL failed.");
				return NULL;
			}
		
		   
		   TADD_FUNC("Finish.");
		   return m_pNode->m_pMdbSelQuery;
	   }

	TMdbQuery* TRepLoadDao::GetMdbUpdateQuery(std::string strTableName)throw (TMdbException)
	   {
		   TADD_FUNC("Start.");
		   int iRet = 0;
		   m_strTableName = strTableName;
		   m_arCols.clear();
		   m_priCols.clear();
		   TMdbTable* pTable =m_pMdbCfg->GetTable(m_strTableName.c_str());
		   if (NULL == pTable)
		   {
			   TADD_ERROR(ERR_APP_INVALID_PARAM, "Table[%s] does not exist in Dsn[%s]", m_strTableName.c_str(), m_pMdbCfg->GetDSN()->sName);
			   return NULL;
		   }
		   for(int i=0; i<pTable->iColumnCounts; ++i)
		   {
			   m_arCols.push_back(pTable->tColumn[i].sName);
		   }
		   
		   //prkey jizhu
			for(int i=0; i<(pTable->m_tPriKey).iColumnCounts; i++)
			{
				m_priCols.push_back(pTable->tColumn[(pTable->m_tPriKey).iColumnNo[i]].sName);
			}
			
		 
			iRet = SetMdbUpdateSQL(m_pNode);
			if (iRet != ERROR_SUCCESS)
			{
				 TADD_ERROR(iRet, "SetSQL failed.");
				 return NULL;
			}
		
		   
		   TADD_FUNC("Finish.");
		   return m_pNode->m_pMdbUptQuery;
	   }
	

	 int TRepLoadDao::SetMdbSelectSQL(TFlushDAONode *pNode)throw (TMdbException)
	 {
	 	int iRet = 0;
        int iSqlLen = 0;
        snprintf(pNode->m_sMdbSelSQL, MAX_SQL_LEN, "SELECT ");

        for (unsigned int i = 0; i< m_arCols.size(); i++)
        {
            iSqlLen = strlen(pNode->m_sMdbSelSQL);
            if (0 == i)
            {
                snprintf(pNode->m_sMdbSelSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, "%s", m_arCols[i].c_str());
            }
            else
            {
                snprintf(pNode->m_sMdbSelSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ",%s", m_arCols[i].c_str());
            }          
        }
      
        iSqlLen = strlen(pNode->m_sMdbSelSQL);
        snprintf(pNode->m_sMdbSelSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, " FROM %s", m_strTableName.c_str());

       	if(m_priCols.size() != 0)
       	{
       		iSqlLen = strlen(pNode->m_sMdbSelSQL);
			snprintf(pNode->m_sMdbSelSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, " WHERE ");
			for(unsigned int i = 0; i< m_priCols.size(); i++)
				
            {
                 iSqlLen = strlen(pNode->m_sMdbSelSQL);
                 if (0 == i)
                 {
                        snprintf(pNode->m_sMdbSelSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, "%s = :%s", (m_priCols[i]).c_str(), (m_priCols[i]).c_str());
                 }
                 else
                 {
                        snprintf(pNode->m_sMdbSelSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, " And %s = :%s", m_priCols[i].c_str(), m_priCols[i].c_str());
                 }          
            }
       	}
        try
        {
            pNode->m_pMdbSelQuery->CloseSQL();
            pNode->m_pMdbSelQuery->SetSQL(pNode->m_sMdbSelSQL,QUERY_NO_ORAFLUSH |QUERY_NO_REDOFLUSH|QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN, "Unknown error!\n");   
        } 
        return iRet;
	 	
	 	

	 }

	 int TRepLoadDao::SetMdbUpdateSQL(TFlushDAONode *pNode)throw (TMdbException)
	 {
	 	int iRet = 0;
        int iSqlLen = 0;
	 	snprintf(pNode->m_sMdbUptSQL, MAX_SQL_LEN, "UPDATE %s SET ", m_strTableName.c_str());
		int iCnt = 0;
        for (unsigned int i = 0; i<m_arCols.size(); i++)
        {
        	unsigned int j = 0;
        	for(j = 0; j<m_priCols.size(); j++)
				if(TMdbNtcStrFunc::StrNoCaseCmp(m_arCols[i].c_str(),m_priCols[j].c_str())== 0)
					break;
				
			if(j<m_priCols.size())
				continue;
			
             iSqlLen = strlen(pNode->m_sMdbUptSQL);
             if (0 == iCnt)
             {
                  snprintf(pNode->m_sMdbUptSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, "%s = :%s", m_arCols[i].c_str(), m_arCols[i].c_str());
             }
              else
             {
                  snprintf(pNode->m_sMdbUptSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ", %s = :%s", m_arCols[i].c_str(), m_arCols[i].c_str());
             }  
			 iCnt++;
        }

		iSqlLen = strlen(pNode->m_sMdbUptSQL);
        snprintf(pNode->m_sMdbUptSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, " WHERE ");
        for (unsigned int i = 0; i< m_priCols.size(); i++)
        {
            iSqlLen = strlen(pNode->m_sMdbUptSQL);
            if (0 == i)
            {
                 snprintf(pNode->m_sMdbUptSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, "%s = :%s", m_priCols[i].c_str(), m_priCols[i].c_str());
            }
            else
            {
                 snprintf(pNode->m_sMdbUptSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, " And %s = :%s", m_priCols[i].c_str(), m_priCols[i].c_str());
            }          
         }

		try
        {
            pNode->m_pMdbUptQuery->CloseSQL();
            pNode->m_pMdbUptQuery->SetSQL(pNode->m_sMdbUptSQL,QUERY_NO_ORAFLUSH |QUERY_NO_REDOFLUSH|QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN, "Unknown error!\n");   
        } 
        return iRet;

	 }

    int TRepLoadDao::SetInsertSQL(TFlushDAONode *pNode)throw (TMdbException)
    {
        int iRet = 0;
        int iSqlLen = 0;
        snprintf(pNode->m_sSQL, MAX_SQL_LEN, "INSERT INTO %s(", m_strTableName.c_str());

        for (unsigned int i = 0; i< m_arCols.size(); i++)
        {
            iSqlLen = strlen(pNode->m_sSQL);
            if (0 == i)
            {
                snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, "%s", m_arCols[i].c_str());
            }
            else
            {
                snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ",%s", m_arCols[i].c_str());
            }          
        }
       
        iSqlLen = strlen(pNode->m_sSQL);
        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ") VALUES(");
        for (unsigned int i = 0; i< m_arCols.size(); i++)
        {
            iSqlLen = strlen(pNode->m_sSQL);
            if (0 == i)
            {
                snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ":%s", m_arCols[i].c_str());
            }
            else
            {
                snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ",:%s", m_arCols[i].c_str());
            }          
        }
        
        iSqlLen = strlen(pNode->m_sSQL);
        snprintf(pNode->m_sSQL+iSqlLen, MAX_SQL_LEN-iSqlLen, ")");

        try
        {
            pNode->m_pQuery->CloseSQL();
			pNode->m_pQuery->SetSQL(pNode->m_sSQL,QUERY_NO_ORAFLUSH |QUERY_NO_REDOFLUSH|QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN, "Unknown error!\n");   
        } 
        return iRet;
    }


	

    TRepFlushDAOCtrl::TRepFlushDAOCtrl():m_ptDatabase(NULL), m_pRcd12Parser(NULL),m_pRcdParser(NULL),m_pConfig(NULL)
    {
       m_tMapFlushDao.SetAutoRelease(true);
    }

    TRepFlushDAOCtrl::~TRepFlushDAOCtrl()
    {
        SAFE_DELETE(m_pRcd12Parser);
        SAFE_DELETE(m_pRcdParser);
        SAFE_DELETE(m_ptDatabase);
        m_tMapFlushDao.Clear();
    }

    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	: 初始化
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepFlushDAOCtrl::Init(const char * sDsn)
    {
        int iRet = 0;
        CHECK_OBJ(sDsn);
        m_pRcdParser = new(std::nothrow) TMdbRepRecdDecode();
        CHECK_OBJ(m_pRcdParser);

        try
        {
            m_ptDatabase = new(std::nothrow) TMdbDatabase();
            CHECK_OBJ(m_ptDatabase);
            if(m_ptDatabase->ConnectAsMgr(sDsn) == false)
            {
                CHECK_RET(ERR_APP_INVALID_PARAM,"ConnectAsMgr [%s] error.",sDsn);
            }

            m_pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
            CHECK_OBJ(m_pConfig);
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN,"Unknown error!\n");   
        } 
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  Execute
    * 函数描述	:  执行消息
    * 输入		:  sMsg -待解析的一条数据信息
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepFlushDAOCtrl::Execute(const char * sMsg, bool bCheckTime)
    {
        int iRet = 0;
       
        m_tCurRedoRecd.Clear();
        
        if(m_pRcdParser->GetVersion(sMsg) == VERSION_DATA_12)
        {
            // 兼容1.2 格式
            if(NULL == m_pRcd12Parser)
            {
                m_pRcd12Parser = new(std::nothrow) TMdbRep12Decode();
                CHECK_OBJ(m_pRcd12Parser);
            }
            CHECK_RET(m_pRcd12Parser->Analyse(sMsg,m_pConfig,m_tCurRedoRecd),"1.2:Parse[%s] failed.ret=[%d]",sMsg,iRet);
        }
        else
        {
            CHECK_RET(m_pRcdParser->DeSerialize(sMsg,m_tCurRedoRecd),"m_pRcdParser.Parse[%s] failed.ret=[%d]",sMsg,iRet);
        }
        
        try
        {

            m_pCurFlushDao = (TRepFlushDao *)m_tMapFlushDao.FindData(m_tCurRedoRecd.m_tHeadInfo.m_sTableName);
            if(m_pCurFlushDao == NULL )
            {
                m_pCurFlushDao= new(std::nothrow)TRepFlushDao();
                CHECK_OBJ(m_pCurFlushDao);
                CHECK_RET(m_pCurFlushDao->Init(m_tCurRedoRecd.m_tHeadInfo.m_sTableName, m_ptDatabase), "TRepFlushDao init failed.");
                m_tMapFlushDao.Add(m_tCurRedoRecd.m_tHeadInfo.m_sTableName, m_pCurFlushDao);
            }
            
            // 时间戳校验
            if(bCheckTime)
            {
                m_iTimeStmp = 0;
                CHECK_RET(QueryRecd(),"query mdb record failed.");
                if(m_tCurRedoRecd.m_tHeadInfo.m_iSqlType != 4)
                {
                    if(!m_bExist || !CheckTimeStamp(m_iTimeStmp, m_tCurRedoRecd.m_tHeadInfo.m_iTimeStamp))
                    {
                        TADD_DETAIL("NOT INSERT ,MAY NOT EXIST OR TIME EXPIRED,NOT EXECTUE");
                        return iRet;
                    }
                }
                else if(m_bExist)
                {
                    TADD_DETAIL("INSERT BUT EXIST,NOT EXECTUE");
                    return iRet;
                }
            }
			int iDropIndex[128] = {0};
			int paramIndex = 0;
			bool bIsInvalidUpdate = true;
			switch(m_tCurRedoRecd.m_tHeadInfo.m_iSqlType)
			{
			case TK_INSERT:
				{
					std::vector<TRepColm>::iterator itor = m_tCurRedoRecd.m_vColms.begin();
					for (; itor != m_tCurRedoRecd.m_vColms.end(); ++itor)
					{
						if(m_pConfig->IsDropColumn(m_tCurRedoRecd.m_tHeadInfo.m_sTableName, itor->m_sColmName.c_str()))
						{
							iDropIndex[paramIndex++] = 1;
							continue;
						}
						paramIndex++;
					}
					break;
				}
			case TK_UPDATE:
				{
					std::vector<TRepColm>::iterator itor = m_tCurRedoRecd.m_vColms.begin();
					for(; itor != m_tCurRedoRecd.m_vColms.end(); ++itor)
					{
						if(m_pConfig->IsDropColumn(m_tCurRedoRecd.m_tHeadInfo.m_sTableName, itor->m_sColmName.c_str()))
						{
							iDropIndex[paramIndex++] = 1;
							continue;
						}
						bIsInvalidUpdate = false;
						paramIndex++;
					}
					if(bIsInvalidUpdate)
					{
						return 0;
					}
					break;
				}
			}
            iRet = SetQuery(iDropIndex);
			if(iRet < 0)
			{
				TADD_ERROR(iRet, "SetQuery failed.");
				return iRet;
			}
            CHECK_OBJ(m_pCurQuery);

            //m_pCurQuery->SetDataSource(m_tCurRedoRecd.m_tHeadInfo.m_iSourceID);

            CHECK_RET(SetParam(iDropIndex),"SetParam failed.");
            m_pCurQuery->Execute();
            m_pCurQuery->Commit();
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN, "Unknown error!\n");   
        } 
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  GetFlushDao
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepFlushDAOCtrl::SetQuery(int * iDropIndex)throw (TMdbException)
    {
        int iRet = 0;
		
        iRet = m_pCurFlushDao->GetQuery(m_pCurQuery, m_tCurRedoRecd, iDropIndex);  
		if(iRet < 0)
		{
			TADD_ERROR(iRet, "GetQuery failed!");
		}
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  SetParam
    * 函数描述	:  设置参数
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepFlushDAOCtrl::SetParam(int * iDropIndex)throw (TMdbException)
    {
        int iRet = 0;
        int iCount = 0;
		int iParamIndex = 0;
        std::vector<TRepColm>::iterator itor = m_tCurRedoRecd.m_vColms.begin();
        for(; itor != m_tCurRedoRecd.m_vColms.end(); ++itor )
        {
        	if(iDropIndex[iParamIndex++] == 1)
    		{
				continue;
    		}
            if(itor->m_bNull)
			{
			  
				m_pCurQuery->SetParameterNULL(iCount);
			}
			else
			{
				m_pCurQuery->SetParameter(iCount,itor->m_sColmValue.c_str());
			}
			iCount ++;
        }

        itor = m_tCurRedoRecd.m_vWColms.begin();
        for(; itor != m_tCurRedoRecd.m_vWColms.end(); ++itor )
        {
            if(itor->m_bNull)
              {
                  
                  m_pCurQuery->SetParameterNULL(iCount);
              }
              else
              {
                  m_pCurQuery->SetParameter(iCount,itor->m_sColmValue.c_str());
              }
              iCount ++;
        }

        //  设置记录时间戳
        TADD_DETAIL("timestamp=[%lld]", m_tCurRedoRecd.m_tHeadInfo.m_iTimeStamp);
        m_pCurQuery->SetTimeStamp(m_tCurRedoRecd.m_tHeadInfo.m_iTimeStamp);
        
        return iRet;
    }

    int TRepFlushDAOCtrl::QueryRecd()
    {
        int iRet = 0;
        
        m_bExist = false;
        try
        {
            
            m_pCurQuery = m_pCurFlushDao->GetRecdQuery();
            CHECK_OBJ(m_pCurQuery);
            CHECK_RET(SetPkParam(),"SetPkParam failed.");
            m_pCurQuery->Open();
            if(m_pCurQuery->Next())
            {
                m_bExist = true;
                m_iTimeStmp = m_pCurQuery->GetTimeStamp();
                TADD_DETAIL("record exist, timestamp=[%lld]", m_iTimeStmp);
            }
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN, "Unknown error!\n");   
        } 

        return iRet;
    }

    bool TRepFlushDAOCtrl::CheckTimeStamp(long long iMdbTime, long long iRecdTIme)
    {
        TADD_DETAIL("mdb timestamp:[%lld], record timestamp:[%lld]",iMdbTime, iRecdTIme);
        return (iRecdTIme >= iMdbTime);
    }

    int TRepFlushDAOCtrl::SetPkParam() throw (TMdbException)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        if(TK_UPDATE == m_tCurRedoRecd.m_tHeadInfo.m_iSqlType || TK_DELETE == m_tCurRedoRecd.m_tHeadInfo.m_iSqlType)
        {
            std::vector<TRepColm>::iterator itor = m_tCurRedoRecd.m_vWColms.begin();
            for(; itor != m_tCurRedoRecd.m_vWColms.end(); ++itor )
            {
                if(itor->m_bNull)
                  {
                      
                      m_pCurQuery->SetParameterNULL(itor->m_sColmName.c_str());
                  }
                  else
                  {
                      m_pCurQuery->SetParameter(itor->m_sColmName.c_str(),itor->m_sColmValue.c_str());
                  }
            }
        }
        else // insert
        {
            TRepColm* pRepColm = NULL;
            std::vector<std::string>::iterator itor = m_pCurFlushDao->m_arPKs.begin();
            for(; itor != m_pCurFlushDao->m_arPKs.end(); ++itor)
            {
                pRepColm= GetPkColm(itor->c_str());
                if(NULL == pRepColm)
                {
                    CHECK_RET(ERR_APP_INVALID_PARAM, "invalid record ,not find pk column[%s]", itor->c_str());
                }
                if(pRepColm->m_bNull)
                {
                    m_pCurQuery->SetParameterNULL(pRepColm->m_sColmName.c_str());
                }
                else
                {
                    m_pCurQuery->SetParameter(pRepColm->m_sColmName.c_str(),pRepColm->m_sColmValue.c_str());
                }
            }
        }
        
        return iRet;
    }

    TRepColm* TRepFlushDAOCtrl::GetPkColm(const char* psColumName)
    {
        std::vector<TRepColm>::iterator itor = m_tCurRedoRecd.m_vColms.begin();
        for(; itor != m_tCurRedoRecd.m_vColms.end(); ++itor)
        {
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psColumName, itor->m_sColmName.c_str()) )
            {
                return &(*(itor));
            }
        }

        itor = m_tCurRedoRecd.m_vWColms.begin();
        for(; itor != m_tCurRedoRecd.m_vWColms.end(); ++itor)
        {
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psColumName, itor->m_sColmName.c_str()) )
            {
                return &(*(itor));
            }
        }

        return NULL;
    }
    


//}
