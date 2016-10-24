/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbDAOLoad.cpp
*@Description�� ����ѱ��е�������������
*@Author:		li.shugang
*@Date��	    2008��12��16��
*@History:
******************************************************************************************/
#include "Dbflush/mdbDAOLoad.h"

//namespace QuickMDB{


/******************************************************************************
* ��������	:  TMdbDAOLoad()
* ��������	:  ��ȡOracle������
* ����		:  pConfig�����ò���
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
TMdbDAOLoad::TMdbDAOLoad(TMdbConfig *pConfig)
{
    TADD_FUNC("Start.");
    if(NULL == pConfig)
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM,"pConfig is null");
    }
    memset(m_sSQL, 0, sizeof(m_sSQL));
    memset(m_sCkFieldSQL, 0, sizeof(m_sCkFieldSQL));
    memset(m_sDSN, 0, sizeof(m_sDSN));
    memset(m_sUID, 0, sizeof(m_sUID));
    memset(m_sPWD, 0, sizeof(m_sPWD));
    memset(m_sMSQL, 0, sizeof(m_sMSQL));
    memset(m_sQMSQL,0,sizeof(m_sQMSQL));
    memset(m_sUpdateSQL,0,sizeof(m_sUpdateSQL));
    m_pDBLink = NULL;   //����
    m_pOraQuery  = NULL;   //����
    m_pInsertQuery = NULL;
    m_pCheckQuery = NULL;
    m_pUpdateQuery = NULL;
    m_pTable = NULL;
    m_pShmDSN = NULL;
    m_iSeqCacheSize = pConfig->GetDSN()->m_iSeqCacheSize;
    SAFESTRCPY(m_sDSN,sizeof(m_sDSN),pConfig->GetDSN()->sOracleID);
    SAFESTRCPY(m_sUID,sizeof(m_sUID),pConfig->GetDSN()->sOracleUID);
    SAFESTRCPY(m_sPWD,sizeof(m_sPWD),pConfig->GetDSN()->sOraclePWD);
    //--HH--m_cType = pConfig->GetDSN()->cType;
    
    TADD_FUNC("Finish.");
}

TMdbDAOLoad::~TMdbDAOLoad()
{
    TADD_FUNC("Start.");
    SAFE_DELETE(m_pOraQuery);
    if(m_pDBLink != NULL)
    {
        m_pDBLink->Disconnect();
        SAFE_DELETE(m_pDBLink)
    }
    SAFE_DELETE(m_pInsertQuery);
    SAFE_DELETE(m_pCheckQuery);
    SAFE_DELETE(m_pUpdateQuery);
    TADD_FUNC("Finish.");
}

/******************************************************************************
* ��������	:  Connect()
* ��������	:  �������ݿ�
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �ɹ�����true�����򷵻�false
* ����		:  li.shugang
*******************************************************************************/
bool TMdbDAOLoad::Connect()
{
    TADD_FUNC("Start.");
    bool bFlag = true;
    if(NULL != m_pDBLink)
    {
        return true;
    }
    //�������ĺϷ���
    if(m_sDSN[0]==0 || m_sUID[0]==0 || m_sPWD[0]==0)
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM,"Paramter invalid. DSN=%s, UID=%s, PWD=%s.",m_sDSN, m_sUID, m_sPWD);
        return false;
    }
    TADD_FLOW("Connecting to DB [%s/%s@%s] ......", m_sUID, m_sPWD, m_sDSN);
    //��ʼ����
    try
    {
        m_pDBLink = TMDBDBFactory::CeatDB();
        m_pDBLink->SetLogin(m_sUID, m_sPWD, m_sDSN);
        bFlag = m_pDBLink->Connect();
    }
    catch(TMDBDBExcpInterface &e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"TDBException:%s\n", e.GetErrMsg());
        bFlag = false;
    }
    TADD_FLOW("Connect to DB [%s/%s@%s] %s.", m_sUID, m_sPWD, m_sDSN, bFlag?"OK":"FAILED");
    TADD_FUNC("Finish.");
    return bFlag;
}


/******************************************************************************
* ��������	:  DisConnect()
* ��������	:  �Ͽ�����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMdbDAOLoad::DisConnect()
{
    TADD_FUNC("Start.");
    if(m_pDBLink != NULL)
    {
        m_pDBLink->Disconnect();
    }
    TADD_FUNC("Finish.");
}


/******************************************************************************
* ��������	:  Init()
* ��������	:  ��ʼ��ĳ����ƴд��SQL���
* ����		:  pTable, ������ı�
* ���		:  ��
* ����ֵ	:  �ɹ�����0�����򷵻�-1
* ����		:  li.shugang
*******************************************************************************/
int TMdbDAOLoad::Init(TMdbShmDSN* pShmDSN, TMdbTable* pTable, const char* sRoutingList,const char* sFilterSql)
{
    TADD_FUNC("Start.");
    CHECK_OBJ(pShmDSN);
    CHECK_OBJ(pTable);
    m_pShmDSN = pShmDSN;
    m_pTable = pTable;
    int iRet = 0;
    if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->sTableName,"dba_sequence") != 0)
    {
        CHECK_RET(CheckFieldExist(),"CheckFieldExist error.");
        CHECK_RET(InitLoadSql(sRoutingList,sFilterSql),"init load sql error.");
    }
    
    CHECK_RET(InitInsertSql(),"init insert sql error.");
    CHECK_RET(InitQueryInQMDB(),"init query sql error.");
    CHECK_RET(InitUpdateSQLInMDB(),"Init update sql errror.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  InitLoadSql()
* ��������	:  ��ʼ������sql
* ����		:
* ���		:  ��
* ����ֵ	:  �ɹ�����0�����򷵻�-1
* ����		:  zhang.lin
*******************************************************************************/
int TMdbDAOLoad::InitLoadSql(const char* sRoutingList,const char* sFilterSql)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(m_pTable);
    memset(m_sSQL, 0, sizeof(m_sSQL));
    if(m_pTable->m_sLoadSQL[0] != 0)
    {
        //���������loadSQL����ֱ��ʹ��
        SAFESTRCPY(m_sSQL,sizeof(m_sSQL),m_pTable->m_sLoadSQL);
        //return 0;
    }
    else
    {
        //ƴд��ѯSQL,���ּ��ط�ʽ 0 or 1
        if(m_pTable->iLoadType == 0)
        {
            CHECK_RET(GetLoadAsStringSQL(m_sSQL,sizeof(m_sSQL),sFilterSql),"GetLoadAsStringSQL failed.");
        }
        else
        {
            CHECK_RET(GetLoadAsNormalSQL(m_sSQL,sizeof(m_sSQL),sFilterSql),"GetLoadAsStringSQL failed.");
        }
    }
    if (m_pTable->m_bShardBack)
    {
        AddRouteIDForLoadSQL(sRoutingList);
    }
    
    TADD_FUNC("Finish.");
    return 0;
}

/******************************************************************************
* ��������	:  InitInsertSql()
* ��������	:  ��ʼ��insert sql
* ����		:
* ���		:  ��
* ����ֵ	:  �ɹ�����0�����򷵻�-1
* ����		:  zhang.lin
*******************************************************************************/
int TMdbDAOLoad::InitInsertSql()
{
    TADD_FUNC("Start.");
    CHECK_OBJ(m_pTable);
    bool bFirstComlun = true;
    memset(m_sMSQL, 0, sizeof(m_sMSQL));
    //��ʼ��MDB-SQL
    for(int i=0; i<m_pTable->iColumnCounts; ++i)
    {
        if(!IsLoadColumn(m_pTable->tColumn[i]))
        {
            continue;   
        }
        if(bFirstComlun)
        {
            sprintf(m_sMSQL, "insert into %s(%s", m_pTable->sTableName, m_pTable->tColumn[i].sName);
            bFirstComlun = false;
        }
        else
        {
            sprintf(&m_sMSQL[strlen(m_sMSQL)], ", %s", m_pTable->tColumn[i].sName);
        }
    }

    sprintf(&m_sMSQL[strlen(m_sMSQL)], ") %s (", "values");

    for(int i=0; i<m_pTable->iColumnCounts; ++i)
    {
        if(!IsLoadColumn(m_pTable->tColumn[i]))
        {
            continue;   
        }
        if(i == 0)
        {
            sprintf(&m_sMSQL[strlen(m_sMSQL)], ":%s", m_pTable->tColumn[i].sName);
        }
        else
        {
            sprintf(&m_sMSQL[strlen(m_sMSQL)], ", :%s", m_pTable->tColumn[i].sName);
        }
    }
    m_sMSQL[strlen(m_sMSQL)] = ')';
    TADD_DETAIL("Insert SQL,m_sMSQL=[%s].",m_sMSQL);
    TADD_FUNC("Finish.");
    return 0;
}

/******************************************************************************
* ��������	:  InitQueryInQMDB()
* ��������	:  ƴдУ���Ƿ��Ѿ���QMDB�д��ڵ�SQL
* ����		:
* ���		:  ��
* ����ֵ	:  �ɹ�����0�����򷵻�-1
* ����		:  cao.peng
*******************************************************************************/
int TMdbDAOLoad::InitQueryInQMDB()
{
    int iRet = 0;
    //ƴдУ���Ƿ��Ѿ���QMDB�д��ڵ�SQL
    memset(m_sQMSQL,0,sizeof(m_sQMSQL));
    sprintf(m_sQMSQL, "select %s from %s where ", m_pTable->tColumn[0].sName,m_pTable->sTableName);
    for(int i =0; i< m_pTable->m_tPriKey.iColumnCounts; i++)
    {
        if(i == m_pTable->m_tPriKey.iColumnCounts -1)
        {
            sprintf(&m_sQMSQL[strlen(m_sQMSQL)], "%s=:%s", \
                m_pTable->tColumn[m_pTable->m_tPriKey.iColumnNo[i]].sName,\
                m_pTable->tColumn[m_pTable->m_tPriKey.iColumnNo[i]].sName);
        }
        else
        {
            sprintf(&m_sQMSQL[strlen(m_sQMSQL)], "%s=:%s and ", 
                m_pTable->tColumn[m_pTable->m_tPriKey.iColumnNo[i]].sName,\
                m_pTable->tColumn[m_pTable->m_tPriKey.iColumnNo[i]].sName);
        }
    }
    return iRet;
}

//��ʼ��update SQL
int TMdbDAOLoad::InitUpdateSQLInMDB()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //ƴдУ���Ƿ��Ѿ���QMDB�д��ڵ�SQL
    memset(m_sUpdateSQL,0,sizeof(m_sUpdateSQL));
    // �����о�Ϊ�����У���֧��update
    if(m_pTable->iColumnCounts == m_pTable->m_tPriKey.iColumnCounts)
    {
        TADD_NORMAL("Table[%s]:All Columns are Primary Keys.not support to update.", m_pTable->sTableName);
        return iRet;
    }
    //����ƴдupdate��set����
    snprintf(m_sUpdateSQL,sizeof(m_sUpdateSQL),"update %s set ",m_pTable->sTableName);
	bool bFirstFlag = false;
    for(int i=0; i<m_pTable->iColumnCounts; ++i)
    {
    	if(true == isKey(i)){continue;}
        //blob�в���Ҫ����
        if(m_pTable->tColumn[i].iDataType == DT_Blob){continue;}
        if(bFirstFlag == false)
        {
            snprintf(m_sUpdateSQL+strlen(m_sUpdateSQL),sizeof(m_sUpdateSQL)-strlen(m_sUpdateSQL),
                " %s=:%s",m_pTable->tColumn[i].sName,m_pTable->tColumn[i].sName);
			bFirstFlag = true;
        }
        else
        {
            snprintf(m_sUpdateSQL+strlen(m_sUpdateSQL),sizeof(m_sUpdateSQL)-strlen(m_sUpdateSQL),
                ",%s=:%s ",m_pTable->tColumn[i].sName,m_pTable->tColumn[i].sName);
        }
    }
    snprintf(m_sUpdateSQL+strlen(m_sUpdateSQL),sizeof(m_sUpdateSQL)-strlen(m_sUpdateSQL)," %s ","where");
    //���ƴд�����ֶ�
    for(int i=0; i<m_pTable->m_tPriKey.iColumnCounts; ++i)
    {
        int iColumnNo = m_pTable->m_tPriKey.iColumnNo[i];
        if(i == 0)
        {
            snprintf(m_sUpdateSQL+strlen(m_sUpdateSQL),sizeof(m_sUpdateSQL)-strlen(m_sUpdateSQL),
                "%s=:%s ",m_pTable->tColumn[iColumnNo].sName,m_pTable->tColumn[iColumnNo].sName);
        }
        else
        {
            snprintf(m_sUpdateSQL+strlen(m_sUpdateSQL),sizeof(m_sUpdateSQL)-strlen(m_sUpdateSQL),
                "and %s=:%s ",m_pTable->tColumn[iColumnNo].sName,m_pTable->tColumn[iColumnNo].sName);
        }
    }
    TADD_DETAIL("m_sUpdateSQL=[%s],OperType=[%d]", m_sUpdateSQL,TK_UPDATE);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  InitCheckFieldSql()
* ��������	:  ��ʼ��У�����Ƿ��������Ӧ��oracle����
* ����		:
* ���		:  ��
* ����ֵ	:  �ɹ�����0�����򷵻�-1
* ����		:  zhang.lin
*******************************************************************************/
int TMdbDAOLoad::InitCheckFieldSql()
{
    TADD_FUNC("Start.");
    CHECK_OBJ(m_pTable);
    memset(m_sCkFieldSQL, 0, sizeof(m_sCkFieldSQL));
    if(m_pTable->m_sLoadSQL[0] != 0)
    {
        //���������loadSQL����ֱ��ʹ��
        SAFESTRCPY(m_sCkFieldSQL,sizeof(m_sCkFieldSQL),m_pTable->m_sLoadSQL);
    }
    else
    {
        sprintf(m_sCkFieldSQL, "select * from %s ",m_pTable->sTableName);
    }
    TADD_DETAIL("check oracle filed SQL,m_sCkFieldSQL=[%s].",m_sCkFieldSQL);
    TADD_FUNC("Finish.");
    return 0;
}

//�ж�ĳ���Ƿ�Ϊ������
bool TMdbDAOLoad::isKey(int iColumnNo)
{
    for(int i = 0; i<m_pTable->m_tPriKey.iColumnCounts; i++)
    {
        int iCPos = m_pTable->m_tPriKey.iColumnNo[i];
        if(iCPos == iColumnNo){return true;}
    }
	return false;
}

/******************************************************************************
* ��������	:  Load()
* ��������	:  ����ĳ����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �ɹ�����0�����򷵻�-1
* ����		:  li.shugang
*******************************************************************************/
int TMdbDAOLoad::Load(TMdbDatabase* pMdb)
{
    TADD_FUNC("Start.");
    CHECK_OBJ(pMdb);
    CHECK_OBJ(m_pTable);
    int iRet = 0;

	 for(int i=0; i<m_pTable->iColumnCounts; ++i)
    {
        //�������oracle�в�����,�򱨴�
        std::map<string,bool>::iterator it = m_mapFieldExist.find(m_pTable->tColumn[i].sName);
        if (it!=m_mapFieldExist.end())
        {
            if(it->second == false)
            {

				TADD_ERROR(ERR_TAB_COLUMN_NOT_EXIST,"column %s of %s doesn't exist in oracle table",m_pTable->tColumn[i].sName,m_pTable->sTableName);
			    return ERR_TAB_COLUMN_NOT_EXIST;
			}
        }
	 }
	
    if(m_pTable->iLoadType == 0)//���ط�ʽΪ0��ƴ��
    {
        iRet = LoadAsString(pMdb);
    }
    else
    {
    	//struct timeval time1 = pMdb->GetShmDsn()->GetInfo()->tCurTime;
        iRet = LoadAsNormal(pMdb);
    	//struct timeval time2 = pMdb->GetShmDsn()->GetInfo()->tCurTime;
		//int interval = (1000000*time2.tv_sec +time2.tv_usec) - (1000000*time1.tv_sec + time1.tv_usec);
		//TADD_NORMAL("Table loaded in %d s %d ms. ", interval/1000000, (interval%1000000)/1000);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbDAOLoad::LoadAsString(TMdbDatabase* pMdb)
{
    TADD_FUNC("Start.");
    CHECK_OBJ(pMdb);

    if(m_sSQL[0] == 0)
    {
        TADD_WARNING("The table=[%s] needn't load.",m_pTable->sTableName);
        return 0;
    }
    
    _TRY_BEGIN_
    TADD_FLOW("Loading table=[%s].", m_pTable->sTableName);
    TMdbQuery *pQuery = pMdb->CreateDBQuery();
    CHECK_OBJ(pQuery);
    pQuery->CloseSQL();
    pQuery->SetSQL(m_sMSQL,QUERY_NO_ORAFLUSH |QUERY_NO_REDOFLUSH|QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);
    SAFE_DELETE(m_pOraQuery);
    CHECK_OBJ(m_pDBLink);
    m_pOraQuery = m_pDBLink->CreateDBQuery();
    m_pOraQuery->Close();
    m_pOraQuery->SetSQL(m_sSQL);
    //����Ǹ���SQL������Ҫ���ò���
    SetLoadSQLParameter();
    //ȡ������Ϣ
    CHECK_OBJ(m_pShmDSN);
    TMdbTable* pTable =  m_pShmDSN->GetTableByName(m_pTable->sTableName);//tableCtr.SetTable(m_pTable->sTableName, NULL);
    if(pTable == NULL)
    {
        TADD_ERROR(ERR_TAB_NO_TABLE,"Can't Find Table=[%s].",m_pTable->sTableName);
        SAFE_DELETE(pQuery);
        return ERR_TAB_NO_TABLE;
    }
    //����״̬:��������
    pTable->cState = Table_loading;
    m_pOraQuery->Open();
    int iCounts = 0;
    TMdbNtcSplit tSplit;
    char sBlobValue[MAX_BLOB_LEN] = {0};
    while(m_pOraQuery->Next())
    {
        _TRY_BEGIN_
        int m = 0;
        tSplit.SplitString(m_pOraQuery->Field(0).AsString(), '~');
        for(int i=0; i<pTable->iColumnCounts; ++i)
        {
            if(!IsLoadColumn(pTable->tColumn[i]))
            {
                continue;
            }
            if(strlen(tSplit[i]) == 0)
            {
                if(pTable->tColumn[i].iDataType == DT_Char || pTable->tColumn[i].iDataType == DT_VarChar)
                {
                    pQuery->SetParameter(m,"");
                }
                else
                {
                    pQuery->SetParameterNULL(m);
                }
            }
            else
            {
                switch(pTable->tColumn[i].iDataType)
                {
                case DT_Int:
                    {
                        pQuery->SetParameter(m, TMdbNtcStrFunc::StrToInt(tSplit[i]));
                        break;

                    }
                case DT_Blob:
                    {
                        memset(sBlobValue,0,MAX_BLOB_LEN);
                        int ilength = 0;
                        strncpy(sBlobValue,m_pOraQuery->Field(i).AsBlobBuffer(ilength),MAX_BLOB_LEN - 1);
                        pQuery->SetParameter(pTable->tColumn[i].sName, sBlobValue, ilength,false);
                        break;

                    }
                default:
                    {
                        pQuery->SetParameter(m, tSplit[i]);
                        break;
                    }
                }
            }

            m++;
        }
        pQuery->Execute(-2);
        pQuery->Commit();

        ++iCounts;
        if(iCounts%10000 == 0)
        {
            TADD_NORMAL("Load table=[%s] %d records.", m_pTable->sTableName, iCounts);
        }
        _TRY_END_NO_RETURN_
    }
    TADD_NORMAL("Load table=[%s] Finished, Total record counts=[%d].", m_pTable->sTableName, iCounts);
    SAFE_DELETE(pQuery);
    //����״̬:�����������
    pTable->cState = Table_running;
    _TRY_END_
    TADD_FUNC("Finish.");
    return 0;
}

int TMdbDAOLoad::LoadAsNormal(TMdbDatabase* pMdb)
{
    TADD_FUNC("Start.");
    CHECK_OBJ(pMdb);
    if(m_sSQL[0] == 0)
    {
        TADD_WARNING("The table=[%s] needn't load.",m_pTable->sTableName);
        return 0;
    }
    _TRY_BEGIN_
    TADD_FLOW("Loading table=[%s].", m_pTable->sTableName);
    TMdbQuery *pQuery = pMdb->CreateDBQuery();
    CHECK_OBJ(pQuery);
    pQuery->CloseSQL();
    pQuery->SetSQL(m_sMSQL,QUERY_NO_ORAFLUSH |QUERY_NO_REDOFLUSH|QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);
    SAFE_DELETE(m_pOraQuery);
    CHECK_OBJ(m_pDBLink);
    m_pOraQuery = m_pDBLink->CreateDBQuery();//���α�
    m_pOraQuery->Close();
    m_pOraQuery->SetSQL(m_sSQL);
    //����Ǹ���SQL������Ҫ���ò���
    SetLoadSQLParameter();
    //ȡ������Ϣ
    CHECK_OBJ(m_pShmDSN);
    TMdbTable* pTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);//tableCtr.SetTable(m_pTable->sTableName, NULL);
    if(pTable == NULL)
    {
        TADD_ERROR(ERR_TAB_NO_TABLE,"Can't Find Table=[%s].",m_pTable->sTableName);
        SAFE_DELETE(pQuery);
        return ERR_TAB_NO_TABLE;
    }
    //����״̬:��������
    pTable->cState = Table_loading;
    m_pOraQuery->Open();
    int iCounts = 0;
    char sBlobValue[MAX_BLOB_LEN] = {0};
    while(m_pOraQuery->Next())
    {
        _TRY_BEGIN_
        int m = 0;
        for(int i=0; i<pTable->iColumnCounts; ++i)
        {
            if(!IsLoadColumn(pTable->tColumn[i]))
            {
                continue;
            }
            if( m_pOraQuery->Field(i).isNULL())
            {
            	if(pTable->tColumn[i].iDataType == DT_Char || pTable->tColumn[i].iDataType == DT_VarChar)
        		{
        			pQuery->SetParameter(m,"");
        		}
				else
				{
	                pQuery->SetParameterNULL(m);
				}
            }
			else
			{
	            switch(pTable->tColumn[i].iDataType)
	            {
		            case DT_Int:
		            {
	                    pQuery->SetParameter(m, m_pOraQuery->Field(i).AsInteger());
		                break;

		            }
		            case DT_Blob:
		            {
		                memset(sBlobValue,0,MAX_BLOB_LEN);
		                int ilength = 0;
		                SAFESTRCPY(sBlobValue,sizeof(sBlobValue),m_pOraQuery->Field(i).AsBlobBuffer(ilength));
		                pQuery->SetParameter(pTable->tColumn[i].sName, sBlobValue, ilength,false);
		                break;
		            }
		            case DT_Char:
		            case DT_VarChar:
		            {
	                    pQuery->SetParameter(m, m_pOraQuery->Field(i).AsString());
		                break;
		            }
		            default:
		            {
	                    pQuery->SetParameter(m, m_pOraQuery->Field(i).AsString());
		                break;
		            }
	            }
			}
            m++;
        }
        pQuery->Execute(-2);
        pQuery->Commit();
        ++iCounts;
        if(iCounts%10000 == 0)
        {
            TADD_NORMAL("Load table=[%s] %d records.", m_pTable->sTableName, iCounts);
        }
        _TRY_END_NO_RETURN_
    }
    TADD_NORMAL("Load table=[%s] Finished, Total record counts=[%d].", m_pTable->sTableName, iCounts);
    SAFE_DELETE(pQuery);
    //����״̬:�����������
    pTable->cState = Table_running;
    _TRY_END_
    TADD_FUNC("Finish.");

    return 0;
}

int TMdbDAOLoad::LoadSequence(TMdbDatabase * pMdb,TMdbDSN* pDsn)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pMdb);
    CHECK_OBJ(pDsn);
	
	m_pShmDSN = TMdbShmMgr::GetShmDSN(pDsn->sName);
    CHECK_OBJ(m_pShmDSN);
	
    char sSQL[MAX_SQL_LEN];
    memset(sSQL,0,sizeof(sSQL));
	char sSequenceName[MAX_NAME_LEN];
	memset(sSequenceName,0,sizeof(sSequenceName));

    CHECK_RET(SetSequenceName(sSequenceName,pDsn->sName),"SetSequenceName Faild");

    sprintf(sSQL,"select SEQ_NAME,START_NUMBER,END_NUMBER,CUR_NUMBER,STEP_NUMBER "
            "from %s "
            "where lower(DSN_NAME) =lower(:DSN_NAME) and lower(LOCAL_IP)=lower(:LOCAL_IP) ",sSequenceName);
    TADD_NORMAL("Load sequence by SQL=\n%s",sSQL);
    try
    {
        SAFE_DELETE(m_pOraQuery);
        CHECK_OBJ(m_pDBLink);
        //���α�
        m_pOraQuery = m_pDBLink->CreateDBQuery();
        m_pOraQuery->Close();
		m_pOraQuery->SetSQL(sSQL);
        m_pOraQuery->SetParameter("DSN_NAME" ,pDsn->sName);
        m_pOraQuery->SetParameter("LOCAL_IP" ,pDsn->sLocalIP);
        m_pOraQuery->Open();
        TMemSeq*  pSeq = NULL;
        while(m_pOraQuery->Next())
        {
            _TRY_BEGIN_
            CHECK_RET(m_pShmDSN->AddNewMemSeq(pSeq),"AddNewMemSeq failed.");
            SAFESTRCPY(pSeq->sSeqName,sizeof(pSeq->sSeqName),m_pOraQuery->Field("SEQ_NAME").AsString());
            //ȥ���������������ߵĿո�,��ֹͨ����������ѯ�������е�����
            TMdbNtcStrFunc::Trim(pSeq->sSeqName);
            pSeq->iStart = m_pOraQuery->Field("START_NUMBER").AsInteger();
            pSeq->iEnd = m_pOraQuery->Field("END_NUMBER").AsInteger();
            pSeq->iStep = m_pOraQuery->Field("STEP_NUMBER").AsInteger();
            int iCurNumber = m_pOraQuery->Field("CUR_NUMBER").AsInteger() + pSeq->iStep * m_iSeqCacheSize;
            //��������ǰ����ֵ����oracle�и����еĵ�ǰֵ����ֱ��ʹ��oracle�е�ǰֵͬ��
            if(iCurNumber > pSeq->iEnd)
            {
                pSeq->iCur = m_pOraQuery->Field("CUR_NUMBER").AsInteger();
            }
            else
            {
                pSeq->iCur = iCurNumber;
            }
            _TRY_END_NO_RETURN_
        }
        TADD_NORMAL("Load table=[dba_sequence] Finished, Total record counts=[%d].", m_pOraQuery->RowsAffected());
    }
    catch(TMDBDBExcpInterface &oe)
    {
        TADD_ERROR(ERROR_UNKNOWN,"TDBException:%s\n",oe.GetErrMsg());
        TADD_ERROR(ERROR_UNKNOWN,"QuerySQL:\n%s\n",oe.GetErrSql());
        return ERROR_UNKNOWN;
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"TMDBException:%s\n",e.GetErrMsg());
        TADD_ERROR(ERROR_UNKNOWN,"QuerySQL:\n%s\n",e.GetErrSql());
        return ERROR_UNKNOWN;
    }
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* ��������	:  ReLoad()
* ��������	:  ��������ĳ����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �ɹ�����0�����򷵻�-1
* ����		:  zhang.lin
*******************************************************************************/
int TMdbDAOLoad::ReLoad(TMdbDatabase* pMdb)
{
    TADD_FUNC("Start.");
    CHECK_OBJ(pMdb);
    int iRet = 0;
	for(int i=0; i<m_pTable->iColumnCounts; ++i)
    {
        //�������oracle�в�����,�򱨴�
        std::map<string,bool>::iterator it = m_mapFieldExist.find(m_pTable->tColumn[i].sName);
        if (it!=m_mapFieldExist.end())
        {
            if(it->second == false)
            {

				TADD_ERROR(ERR_TAB_COLUMN_NOT_EXIST,"column %s of %s doesn't exist in oracle table",m_pTable->tColumn[i].sName,m_pTable->sTableName);
			    return ERR_TAB_COLUMN_NOT_EXIST;
			}
        }
	}
    if(m_pTable->iLoadType == 0)
    {
        iRet = ReLoadAsString(pMdb);
    }
    else
    {
        iRet = ReLoadAsNormal(pMdb);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbDAOLoad::SetSequenceName(char * sSeqenceName,char* sDsnName)
{
    try
    {
        SAFE_DELETE(m_pOraQuery);
        char sQuerySeqTabSQL[MAX_SQL_LEN];
        memset(sQuerySeqTabSQL,0,sizeof(sQuerySeqTabSQL));
        sprintf(sQuerySeqTabSQL,"select count(*)  from %s_MDB_SEQUENCE",sDsnName);
		CHECK_OBJ(m_pDBLink);
        m_pOraQuery = m_pDBLink->CreateDBQuery();
        m_pOraQuery->Close();
        m_pOraQuery->SetSQL(sQuerySeqTabSQL);
        m_pOraQuery->Open();
            if(m_pOraQuery->Next())
            {
            }
	    snprintf(sSeqenceName,MAX_NAME_LEN,"%s_MDB_SEQUENCE",sDsnName);
	}
    catch(TMDBDBExcpInterface &e)
    {
      TADD_NORMAL("Not find table=[%s_MDB_SEQUENCE],try load from [MDB_SEQUENCE]",sDsnName);
    	snprintf(sSeqenceName,MAX_NAME_LEN,"MDB_SEQUENCE");
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN, "SetSequenceName failed : Unknown error!\n");
        return -1;
    }
	return 0;
}

int TMdbDAOLoad::ReLoadAsString(TMdbDatabase* pMdb)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pMdb);
    if(m_sSQL[0] == 0)
    {
        TADD_WARNING("The table=[%s] needn't load.",m_pTable->sTableName);
        return 0;
    }
    _TRY_BEGIN_
    TADD_NORMAL("Loading table=[%s].", m_pTable->sTableName);
    CHECK_RET(InitQueryForMDB(pMdb),"InitQueryForMDB failed.");
    //����Ǹ���SQL������Ҫ���ò���
    SetLoadSQLParameter();
    //ȡ������Ϣ
    CHECK_OBJ(m_pShmDSN);
    TMdbTable* pTable =  m_pShmDSN->GetTableByName(m_pTable->sTableName);
    if(pTable == NULL)
    {
        TADD_ERROR(ERR_TAB_NO_TABLE,"Can't Find Table=[%s].",m_pTable->sTableName);
        return ERR_TAB_NO_TABLE;
    }
    int iCounts = 0;
    TMdbNtcSplit tSplit;
    char sBlobValue[MAX_BLOB_LEN] = {0};
    m_pOraQuery->Open();
    while(m_pOraQuery->Next())
    {
        _TRY_BEGIN_
        tSplit.SplitString(m_pOraQuery->Field(0).AsString(),'~');
        //�����¼�Ѿ����ڣ�����Ҫ��������
        if(CheckAsStringRecord(m_pCheckQuery,m_pOraQuery) == true)
        {
            UpdateMDBStringData(pTable,tSplit,iCounts);
            continue;
        }
        int m = 0;
        for(int i=0; i<pTable->iColumnCounts; ++i)
        {
            if(!IsLoadColumn(pTable->tColumn[i]))
            {
                continue;
            }
            
            if(pTable->tColumn[i].iDataType == DT_Int)
            {
                m_pInsertQuery->SetParameter(m, TMdbNtcStrFunc::StrToInt(tSplit[i]));
            }
            else if(pTable->tColumn[i].iDataType == DT_Blob)
            {
                memset(sBlobValue,0,MAX_BLOB_LEN);
                int ilength = 0;
                strncpy(sBlobValue,m_pOraQuery->Field(i).AsBlobBuffer(ilength),MAX_BLOB_LEN - 1);
                //���ﲻ��Ҫ���룬��setparmeter���Ѿ�������
                m_pInsertQuery->SetParameter(pTable->tColumn[i].sName, sBlobValue, ilength,false);
            }
            else
            {
                m_pInsertQuery->SetParameter(m, tSplit[i]);
            }
            m++;
        }
        m_pInsertQuery->Execute(-2);
        m_pInsertQuery->Commit();
        ++iCounts;
        if(iCounts%10000 == 0)
        {
            TADD_NORMAL("Load table=[%s] %d records.", m_pTable->sTableName, iCounts);
        }
        _TRY_END_NO_RETURN_
    }

    TADD_NORMAL("Load table=[%s] Finished, Total record counts=[%d].", m_pTable->sTableName, iCounts);
    //����״̬:�����������
    pTable->cState = Table_running;
    _TRY_END_
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbDAOLoad::ReLoadAsNormal(TMdbDatabase* pMdb)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pMdb);
    if(m_sSQL[0] == 0)
    {
        TADD_WARNING("The table=[%s] needn't load.",m_pTable->sTableName);
        return 0;
    }
    _TRY_BEGIN_
    TADD_NORMAL("Loading table=[%s].", m_pTable->sTableName);
    CHECK_RET(InitQueryForMDB(pMdb),"InitQueryForMDB failed.");
    //����Ǹ��Ӽ���SQL������Ҫ���ò���
    SetLoadSQLParameter();
    //ȡ������Ϣ
    CHECK_OBJ(m_pShmDSN);
    TMdbTable* pTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);
    if(pTable == NULL)
    {
        TADD_ERROR(ERR_TAB_NO_TABLE,"Can't Find Table[%s] in the shared memory.",m_pTable->sTableName);
        return ERR_TAB_NO_TABLE;
    }

    //����״̬:��������
    m_pOraQuery->Open();
    int iCounts = 0;
    char sBlobValue[MAX_BLOB_LEN] = {0};
    while(m_pOraQuery->Next())
    {
        _TRY_BEGIN_
        //�����¼�Ѿ����ڣ����������update����
        if(CheckRecord(m_pCheckQuery,m_pOraQuery) == true)
        {
            UpdateMDBNormalData(pTable,iCounts);//����ĳ����¼ʧ�ܣ���Ҫ����
            continue;
        }
        int m = 0;
        for(int i=0; i<pTable->iColumnCounts; ++i)
        {
            if(!IsLoadColumn(pTable->tColumn[i]))
            {
                continue;
            }
            switch(pTable->tColumn[i].iDataType)
            {
                case DT_Int:
                {
                    if( m_pOraQuery->Field(i).isNULL())
                    {
                        m_pInsertQuery->SetParameterNULL(m);
                    }
                    else
                    {
                        m_pInsertQuery->SetParameter(m, m_pOraQuery->Field(i).AsInteger());
                    }
                    break;
                }
                case DT_Blob:
                {
                    memset(sBlobValue,0,MAX_BLOB_LEN);
                    int ilength = 0;
                    SAFESTRCPY(sBlobValue,sizeof(sBlobValue),m_pOraQuery->Field(i).AsBlobBuffer(ilength));
                    m_pInsertQuery->SetParameter(pTable->tColumn[i].sName, sBlobValue, ilength,false);
                    break;
                }
                case DT_Char:
                case DT_VarChar:
                {
                    if( m_pOraQuery->Field(i).isNULL() )
                    {
                        m_pInsertQuery->SetParameter(m,"");
                    }
                    else
                    {
                        m_pInsertQuery->SetParameter(m, m_pOraQuery->Field(i).AsString());
                    }
                    break;
                }
                default:
                {
                    if( m_pOraQuery->Field(i).isNULL() )
                    {
                        m_pInsertQuery->SetParameterNULL(m);
                    }
                    else
                    {
                        m_pInsertQuery->SetParameter(m, m_pOraQuery->Field(i).AsString());
                    }
                    break;
                }
            }
            m++;
        }
        m_pInsertQuery->Execute(-2);
        m_pInsertQuery->Commit();
        ++iCounts;
        if(iCounts%10000 == 0)
        {
            TADD_NORMAL("Load table=[%s] %d records.", m_pTable->sTableName, iCounts);
        }
        _TRY_END_NO_RETURN_
    }
    TADD_NORMAL("Load table=[%s] Finished, Total record counts=[%d].", m_pTable->sTableName, iCounts);
    //����״̬:�����������
    pTable->cState = Table_running;
    _TRY_END_
    TADD_FUNC("Finish.");
    return iRet;
}

//��ʼ������QUERY
int TMdbDAOLoad::InitQueryForMDB(TMdbDatabase* pMdb)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //�����ȡ��ռ����ԣ��жϸñ��Ƿ�֧���ļ��洢�����֧���ļ������Ҫдredo
    CHECK_OBJ(m_pTable);
   TMdbTableSpace *pSpace = m_pShmDSN->GetTableSpaceAddrByName(m_pTable->m_sTableSpace);
   CHECK_OBJ(pSpace);
    
    CHECK_OBJ(pMdb);
    CHECK_OBJ(m_pDBLink);
    if(NULL == m_pInsertQuery)
    {
        m_pInsertQuery = pMdb->CreateDBQuery();
        CHECK_OBJ(m_pInsertQuery);
    }
    if(NULL == m_pCheckQuery)
    {
        m_pCheckQuery = pMdb->CreateDBQuery();
        CHECK_OBJ(m_pCheckQuery);
    }
    if(NULL == m_pUpdateQuery)
    {
        m_pUpdateQuery = pMdb->CreateDBQuery();
        CHECK_OBJ(m_pUpdateQuery);
    }
    if(NULL == m_pOraQuery)
    {
        m_pOraQuery = m_pDBLink->CreateDBQuery();
        CHECK_OBJ(m_pOraQuery);
    }
    if(pSpace->m_bFileStorage == true)
    {
        m_pInsertQuery->Close();
        m_pInsertQuery->SetSQL(m_sMSQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);
        
        m_pCheckQuery->Close();
        m_pCheckQuery->SetSQL(m_sQMSQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);
        
        m_pUpdateQuery->Close();
        m_pUpdateQuery->SetSQL(m_sUpdateSQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK,0);
    }
    else
    {
        m_pInsertQuery->Close();
        m_pInsertQuery->SetSQL(m_sMSQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH|QUERY_NO_REDOFLUSH|QUERY_NO_ROLLBACK,0);
        
        m_pCheckQuery->Close();
        m_pCheckQuery->SetSQL(m_sQMSQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH|QUERY_NO_REDOFLUSH|QUERY_NO_ROLLBACK,0);
        
        m_pUpdateQuery->Close();
        m_pUpdateQuery->SetSQL(m_sUpdateSQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH|QUERY_NO_REDOFLUSH|QUERY_NO_ROLLBACK,0);
    }
    //���α�
    m_pOraQuery->Close();
    m_pOraQuery->SetSQL(m_sSQL);
    TADD_FUNC("Finish.");
    return iRet;
}

//����QMDB������
int TMdbDAOLoad::UpdateMDBStringData(const TMdbTable* pTable,TMdbNtcSplit &tSplit,int &iCount)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTable);
    char sValue[1024];
    memset(sValue,0,sizeof(sValue));
    char sBlobValue[MAX_BLOB_LEN] = {0};
    _TRY_BEGIN_
    // �����о�Ϊ�����У���֧��update
    if(pTable->iColumnCounts == pTable->m_tPriKey.iColumnCounts)
    {
        TADD_WARNING("Table[%s]:All Columns are primary key, update not support", pTable->sTableName);
        return iRet;
    }
    
    for(int i=0; i<pTable->iColumnCounts; ++i)
    {
		if(isKey(i) == true)
		{
			continue;
		}
		
        if(pTable->tColumn[i].iDataType == DT_Int)
        {
            m_pUpdateQuery->SetParameter(pTable->tColumn[i].sName,TMdbNtcStrFunc::StrToInt(tSplit[i]));
        }
        else if(pTable->tColumn[i].iDataType != DT_Blob)
        {
            memset(sValue, 0, sizeof(sValue));
            SAFESTRCPY(sValue,sizeof(sValue),tSplit[i]);
            TMdbNtcStrFunc::Trim(sValue);
            m_pUpdateQuery->SetParameter(pTable->tColumn[i].sName, sValue);
        }
    }
    //���ƴд�����ֶ�
    for(int i =0; i< pTable->m_tPriKey.iColumnCounts; i++)
    {
        //�������ڵ���λ��
        int iPkPos = pTable->tColumn[pTable->m_tPriKey.iColumnNo[i]].iPos;
        int iDataType = pTable->tColumn[pTable->m_tPriKey.iColumnNo[i]].iDataType;
        if(iDataType == DT_Int)
        {
            m_pUpdateQuery->SetParameter(pTable->tColumn[iPkPos].sName,TMdbNtcStrFunc::StrToInt(tSplit[iPkPos]));
        }
        else if(iDataType == DT_Blob)
        {
            memset(sBlobValue,0,MAX_BLOB_LEN);
            int ilength = strlen(tSplit[iPkPos]);
            SAFESTRCPY(sBlobValue,sizeof(sBlobValue),tSplit[iPkPos]);
            m_pUpdateQuery->SetParameter(pTable->tColumn[iPkPos].sName,sBlobValue,ilength,false);
        }
        else
        {
            m_pUpdateQuery->SetParameter(pTable->tColumn[iPkPos].sName,tSplit[iPkPos]);
        }
    }
    m_pUpdateQuery->Execute();
    m_pUpdateQuery->Commit();
    iCount++;
    _TRY_END_NO_RETURN_
    TADD_FUNC("Finish.");
    return iRet;
}

//����QMDB������
int TMdbDAOLoad::UpdateMDBNormalData(const TMdbTable* pTable,int &iCount)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char sValue[1024];
    memset(sValue,0,sizeof(sValue));
    CHECK_OBJ(pTable);
    _TRY_BEGIN_
    // �����о�Ϊ�����У���֧��update
    if(pTable->iColumnCounts == pTable->m_tPriKey.iColumnCounts)
    {
        TADD_WARNING("Table[%s]:All Columns are primary key, update not support", pTable->sTableName);
        return iRet;
    }
    
    for(int i=0; i<pTable->iColumnCounts; ++i)
    {
        TADD_DETAIL("Column[%d]-Name=[%s].", 
            i, pTable->tColumn[i].sName);
		if(isKey(i) == true)
		{
			continue;
		}
		
        if(m_pOraQuery->Field(i).isNULL())
        {
             m_pUpdateQuery->SetParameterNULL(pTable->tColumn[i].sName);
        }
        else if(pTable->tColumn[i].iDataType == DT_Int)
        {
            m_pUpdateQuery->SetParameter(pTable->tColumn[i].sName,m_pOraQuery->Field(i).AsInteger());
        }
        else if(pTable->tColumn[i].iDataType != DT_Blob)
        {
            memset(sValue, 0, sizeof(sValue));
            SAFESTRCPY(sValue,sizeof(sValue),m_pOraQuery->Field(i).AsString());
            TMdbNtcStrFunc::Trim(sValue);
            m_pUpdateQuery->SetParameter(pTable->tColumn[i].sName, sValue);
        }
    }
    //���ƴд�����ֶ�
    for(int i=0; i<pTable->m_tPriKey.iColumnCounts; ++i)
    {
        int iColumnNo = pTable->m_tPriKey.iColumnNo[i];
        if(pTable->tColumn[iColumnNo].iDataType == DT_Int)
        {
            m_pUpdateQuery->SetParameter(pTable->tColumn[iColumnNo].sName,m_pOraQuery->Field(iColumnNo).AsInteger());
        }
        else if(pTable->tColumn[i].iDataType != DT_Blob)
        {
            memset(sValue, 0, sizeof(sValue));
            SAFESTRCPY(sValue,sizeof(sValue),m_pOraQuery->Field(iColumnNo).AsString());
            TMdbNtcStrFunc::Trim(sValue);
            m_pUpdateQuery->SetParameter(pTable->tColumn[iColumnNo].sName,sValue);
        }
    }
    m_pUpdateQuery->Execute();
    m_pUpdateQuery->Commit();
    iCount++;
    _TRY_END_NO_RETURN_
    TADD_FUNC("Finish.");
    return iRet;
}

//���ö�̬����sql����
void TMdbDAOLoad::SetLoadSQLParameter()
{
    if(m_pTable->iParameterCount != 0 && m_pTable->m_sLoadSQL[0] != 0)
    {
        for(int i = 0; i<m_pTable->iParameterCount; i++)
        {
            if(m_pTable->tParameter[i].iParameterType != 0)
            {
                continue;
            }
            
            if(m_pTable->tParameter[i].iDataType == DT_Int)
            {
                m_pOraQuery->SetParameter(m_pTable->tParameter[i].sName,\
                    TMdbNtcStrFunc::StrToInt(m_pTable->tParameter[i].sValue));
            }
            else
            {
                m_pOraQuery->SetParameter(m_pTable->tParameter[i].sName,\
                    m_pTable->tParameter[i].sValue);
            }
        }
    }
}

int TMdbDAOLoad::CheckFieldExist()
{
    TADD_FUNC("Start.");
    bool bFlag = false;
    int iRet = 0;
    CHECK_OBJ(m_pTable);
    try
    {
        SAFE_DELETE(m_pOraQuery);
        CHECK_OBJ(m_pDBLink);
        CHECK_RET(InitCheckFieldSql(),"InitCheckFieldSql error.");
        //���α�
        m_pOraQuery = m_pDBLink->CreateDBQuery();
        m_pOraQuery->Close();
        m_pOraQuery->SetSQL(m_sCkFieldSQL);

        //����Ǹ���SQL������Ҫ���ò���
        if(m_pTable->iParameterCount != 0 && m_pTable->m_sLoadSQL[0] != 0)
        {
            for(int i = 0; i<m_pTable->iParameterCount; i++)
            {
                if(m_pTable->tParameter[i].iDataType == DT_Int && m_pTable->tParameter[i].iParameterType == 0)
                {
                    m_pOraQuery->SetParameter(m_pTable->tParameter[i].sName,TMdbNtcStrFunc::StrToInt(m_pTable->tParameter[i].sValue));
                }
                else if(m_pTable->tParameter[i].iParameterType == 0)
                {
                    m_pOraQuery->SetParameter(m_pTable->tParameter[i].sName,m_pTable->tParameter[i].sValue);
                }
            }
        }
        m_pOraQuery->Open();

        for(int i=0; i<m_pTable->iColumnCounts; ++i)
        {
            //NoRep or mdb2mdb
            bFlag = m_pOraQuery->IsFieldExist(m_pTable->tColumn[i].sName);
            TADD_DETAIL("IsFieldExist(%s) in oracle table =[%s].",m_pTable->tColumn[i].sName,bFlag?"TRUE":"FALSE");
            m_mapFieldExist[m_pTable->tColumn[i].sName] = bFlag;
        }

    }
    catch(TMDBDBExcpInterface &oe)
    {
        TADD_ERROR(ERROR_UNKNOWN,"TDBException:%s\n",oe.GetErrMsg());
        TADD_ERROR(ERROR_UNKNOWN,"QuerySQL:\n%s\n",oe.GetErrSql());
        return ERROR_UNKNOWN;
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"TMDBException:%s\n",e.GetErrMsg());
        TADD_ERROR(ERROR_UNKNOWN,"QuerySQL:\n%s\n",e.GetErrSql());
        return ERROR_UNKNOWN;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unkown error ,SQL=[%s].",m_sCkFieldSQL);
        return ERROR_UNKNOWN;
    }	
    TADD_FUNC("Finish.");
    return iRet;
}
bool TMdbDAOLoad::CheckRecord(TMdbQuery * pQMDBQuery,TMDBDBQueryInterface* pOraQuery)
{
    TADD_FUNC("Start.");
    CHECK_OBJ(pQMDBQuery);
    CHECK_OBJ(pOraQuery);
    bool bFlag = false;
    for(int i =0; i< m_pTable->m_tPriKey.iColumnCounts; i++)
    {
        if(m_pTable->tColumn[m_pTable->m_tPriKey.iColumnNo[i]].iDataType == DT_Int)
        {
            pQMDBQuery->SetParameter(i, m_pOraQuery->Field(m_pTable->m_tPriKey.iColumnNo[i]).AsInteger());
        }
        else
        {
            pQMDBQuery->SetParameter(i, m_pOraQuery->Field(m_pTable->m_tPriKey.iColumnNo[i]).AsString());
        }
    }
    pQMDBQuery->Open();
    while(pQMDBQuery->Next())
    {
        bFlag = true;
        break;
    }
    TADD_FUNC("Finish.");
    return bFlag;
}

bool TMdbDAOLoad::CheckAsStringRecord(TMdbQuery * pQMDBQuery,TMDBDBQueryInterface* pOraQuery)
{
    TADD_FUNC("Start.");
    CHECK_OBJ(pQMDBQuery);
    CHECK_OBJ(pOraQuery);
    TMdbNtcSplit tSplit;
    tSplit.SplitString(m_pOraQuery->Field(0).AsString(),'~');
    // 1~test~2~test2~hello
    bool bFlag = false;
    char sBlobValue[MAX_BLOB_LEN] = {0};
    for(int i =0; i< m_pTable->m_tPriKey.iColumnCounts; i++)
    {
        //�������ڵ���λ��
        int iPkPos = m_pTable->tColumn[m_pTable->m_tPriKey.iColumnNo[i]].iPos;
        int iDataType = m_pTable->tColumn[m_pTable->m_tPriKey.iColumnNo[i]].iDataType;
        if(iDataType == DT_Int)
        {
            pQMDBQuery->SetParameter(i, TMdbNtcStrFunc::StrToInt(tSplit[iPkPos]));
        }
        else if(iDataType == DT_Blob)
        {
            memset(sBlobValue,0,MAX_BLOB_LEN);
            int ilength = strlen(tSplit[iPkPos]);;
            SAFESTRCPY(sBlobValue,sizeof(sBlobValue),tSplit[iPkPos]);
            //���ﲻ��Ҫ���룬��setparmeter���Ѿ�������
            pQMDBQuery->SetParameter(m_pTable->tColumn[m_pTable->m_tPriKey.iColumnNo[i]].sName, sBlobValue, ilength,false);
        }
        else
        {
            pQMDBQuery->SetParameter(i, tSplit[iPkPos]);
        }
    }
    pQMDBQuery->Open();
    while(pQMDBQuery->Next())
    {
        bFlag = true;
        break;
    }
    TADD_FUNC("Finish.");
    return bFlag;
}

//�ж�ĳ���Ƿ��oracle������
bool TMdbDAOLoad::IsLoadColumn(TMdbColumn &tColumn)
{
    // �в�����ͬ�����ԡ�
    /*if(tColumn.iRepAttr != Column_Ora_Rep 
            && tColumn.iRepAttr != Column_From_Ora 
            && tColumn.iRepAttr != Column_To_Ora)
    {
        return false;
    }*/
    return true;
}

//ƴ������SQL
int TMdbDAOLoad::GetLoadAsStringSQL(char sSQL[],const int iSize,const char* sFilterSql)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bFirstComlun = true;
    memset(sSQL, 0, iSize);
    //ƴд��ѯSQL
    for(int i=0; i<m_pTable->iColumnCounts; ++i)
    {
        //�������oracle�в�����,�򲻲�ѯ
        std::map<string,bool>::iterator it = m_mapFieldExist.find(m_pTable->tColumn[i].sName);
        if (it!=m_mapFieldExist.end())
        {
            if(!it->second){continue;}
        }
        if(bFirstComlun)
        {
            if(m_pTable->tColumn[i].iDataType != DT_DateStamp)
            {
                snprintf(sSQL, iSize,"select %s", m_pTable->tColumn[i].sName);
            }
            else
            {           
                #ifdef DB_ORACLE
                    snprintf(sSQL, iSize,"select to_char(%s, 'YYYYMMDDHH24MISS')",\
                        m_pTable->tColumn[i].sName);
                #elif DB_MYSQL  
                    snprintf(sSQL, iSize,"select date_format(%s, '%%Y%%m%%d%%H%%i%%s')",\
                        m_pTable->tColumn[i].sName);
                #else        
                    TADD_ERROR(ERROR_UNKNOWN, "[%s:%d]  DB_TYPE is wrong, check Makefile",__FILE__,__LINE__); 
                    iRet = -1;
                #endif 
            }
            bFirstComlun = false;
            continue;
        }
    
        if(m_pTable->tColumn[i].iDataType != DT_DateStamp)
        {
            snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL),\
                "||'~'||%s", m_pTable->tColumn[i].sName);
        }
        else
        {
            #ifdef DB_ORACLE
                snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL),\
                    "||'~'||to_char(%s, 'YYYYMMDDHH24MISS')", m_pTable->tColumn[i].sName);
            #elif DB_MYSQL  
                snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL),\
                    "||'~'||date_format(%s, '%%Y%%m%%d%%H%%i%%s')", m_pTable->tColumn[i].sName);
            #else        
                TADD_ERROR(ERROR_UNKNOWN, "[%s:%d]  DB_TYPE is wrong, check Makefile",__FILE__,__LINE__); 
                iRet = -1;
            #endif
        }
    }

	if(sFilterSql != NULL)
	{
    	snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL),\
            " as RetString from %s where %s ",m_pTable->sTableName, sFilterSql);
    
	}
    else
    {
    	if(m_pTable->m_sFilterSQL[0] == 0)
    	{
        	snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL),\
            	" as RetString from %s ", m_pTable->sTableName);
    	}
   		 else
    	{
        	snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL),\
            " as RetString from %s where %s ",m_pTable->sTableName, m_pTable->m_sFilterSQL);
    	}

    }
    TADD_DETAIL("Load SQL,sSQL = [%s].",sSQL);
    TADD_FUNC("Finish.");
    return iRet;
}

//ƴ������SQL
int TMdbDAOLoad::GetLoadAsNormalSQL(char sSQL[],const int iSize,const char* sFilterSql)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bFirstComlun = true;
    memset(sSQL, 0, iSize);
    //ƴд��ѯSQL
    for(int i=0; i<m_pTable->iColumnCounts; ++i)
    {
        //�������oracle�в�����,�򲻲�ѯ
        std::map<string,bool>::iterator it = m_mapFieldExist.find(m_pTable->tColumn[i].sName);
        if (it!=m_mapFieldExist.end())
        {
            if(!it->second)
            {
                continue;
            }
        }
        if(bFirstComlun)
        {
            if(m_pTable->tColumn[i].iDataType != DT_DateStamp)
            {
                snprintf(sSQL, iSize,"select %s", m_pTable->tColumn[i].sName);
            }
            else
            {
                #ifdef DB_ORACLE
                    snprintf(sSQL, iSize,"select to_char(%s, 'YYYYMMDDHH24MISS')",\
                        m_pTable->tColumn[i].sName);
                #elif DB_MYSQL
                    snprintf(sSQL, iSize,"select date_format(%s, '%%Y%%m%%d%%H%%i%%s')",\
                        m_pTable->tColumn[i].sName);
                #else        
                    TADD_ERROR(ERROR_UNKNOWN, "[%s:%d]  DB_TYPE is wrong, check Makefile",__FILE__,__LINE__); 
                    iRet = -1;
                #endif                    
            }
            bFirstComlun = false;
            continue;
        }
        
        if(m_pTable->tColumn[i].iDataType != DT_DateStamp)
        {
            snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL),", %s", \
                m_pTable->tColumn[i].sName);
        }
        else
        {
            #ifdef DB_ORACLE
                snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL),\
                    ", to_char(%s, 'YYYYMMDDHH24MISS') %s", \
                    m_pTable->tColumn[i].sName,m_pTable->tColumn[i].sName);
            #elif DB_MYSQL
                snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL),\
                    ", date_format(%s, '%%Y%%m%%d%%H%%i%%s') %s", \
                    m_pTable->tColumn[i].sName,m_pTable->tColumn[i].sName);
            #else        
                TADD_ERROR(ERROR_UNKNOWN, "[%s:%d]  DB_TYPE is wrong, check Makefile",__FILE__,__LINE__); 
                iRet = -1;
            #endif 
        }
    }

	if(sFilterSql != NULL)
	{
		snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL), " from %s where %s ", \
                m_pTable->sTableName, sFilterSql);
		
	}
	else
	{
		if(m_pTable->m_sFilterSQL[0] == 0)
    	{
        	snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL), " from %s ", \
                m_pTable->sTableName);
    	}
    	else
    	{
        	snprintf(sSQL+strlen(sSQL),iSize-strlen(sSQL), " from %s where %s ", \
                m_pTable->sTableName, m_pTable->m_sFilterSQL);
    	}

	}
    
    TADD_DETAIL("Load SQL,sSQL = [%s].",sSQL);
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* ��������	:  AddRouteIDForLoadSQL()
* ��������	:  Ϊ��ˢ��SQL���·��
* ����		:  pTable ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  cao.peng
*******************************************************************************/
void TMdbDAOLoad::AddRouteIDForLoadSQL(const char* sRoutingList)
{
    //TADD_NORMAL("TMdbDAOLoad::AddRouteIDForLoadSQL(sRoutingList = %s)", sRoutingList);
    if (NULL == sRoutingList || '\0' == sRoutingList[0] || TMdbNtcStrFunc::StrNoCaseCmp(sRoutingList, DEFAULT_ROUTE_ID_STRING) == 0)
    {
        return;
    }
    if (sRoutingList) 
    {
        if (HasWhereCond(m_pTable->sTableName, m_sSQL)) //LoadSQL���Ѿ�����where����
        {
            snprintf(m_sSQL+strlen(m_sSQL), MAX_SQL_LEN - strlen(m_sSQL), " AND ROUTING_ID IN (%s)", sRoutingList);
        }
        else
        {
            snprintf(m_sSQL+strlen(m_sSQL), MAX_SQL_LEN - strlen(m_sSQL), " WHERE ROUTING_ID IN (%s)", sRoutingList);
        }
    }  
    TADD_NORMAL("LoadSQL : %s\n", m_sSQL);
}

/******************************************************************************
* ��������	:  HasWhereCond()
* ��������	:  �ж�һ�ű��Load SQL������Ƿ����where����
* ����		:  sTableName ����
* ����		:  sLoadSQL sql���
* ���		:  ��
* ����ֵ	:  ���ڷ���true,���򷵻�false
* ����		:  jiang.lili
*******************************************************************************/
bool TMdbDAOLoad::HasWhereCond(const char* sTableName, const char * sLoadSQL)
{
    bool bRet = false;
    char sTmpSQL[MAX_SQL_LEN];
    memset(sTmpSQL, 0, sizeof(sTmpSQL));

    //ȥ��LoadSQL�������ո����
    int iLen = 0;
    for (int i = 0;  i<MAX_SQL_LEN && sLoadSQL[i] != '\0' ; i++)
    {
        if (iLen-1 >=0 && ' ' == sTmpSQL[iLen-1] &&  ' ' == sLoadSQL[i])
        {
            continue;
        }
        sTmpSQL[iLen] = sLoadSQL[i];
        iLen ++;
    }

    TMdbNtcSplit tSplit;
    tSplit.SplitString(sTmpSQL, ' ');
    for (int i = 0; i < tSplit.GetFieldCount()-2; i++)
    {
        if (TMdbNtcStrFunc::StrNoCaseCmp(tSplit[i], "from") == 0
            && TMdbNtcStrFunc::StrNoCaseCmp(tSplit[i+1], sTableName) == 0
            && TMdbNtcStrFunc::StrNoCaseCmp(tSplit[i+1], sTableName) == 0
            && (tSplit[i+2], "where") == 0)
        {
            bRet = true;
            break;
        }
    }

    return bRet;
}

//}

