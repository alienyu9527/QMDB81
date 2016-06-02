/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@                   All rights reserved.
*@Name��	    mdbRollback.cpp
*@Description�� �������QuickMDB�Ļع����ƽӿ�
*@Author:		li.shugang
*@Date��	    2012��02��6��
*@History:
******************************************************************************************/

#include "Interface/mdbRollback.h"
#include "Interface/mdbQuery.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbSQLParser.h"
#include "Control/mdbRowCtrl.h"

//namespace QuickMDB{
#define RB_COL_SPLIT '\a'  //�ع����ݵ��зָ��
#define RB_COL_NULL  '\b'  //�ع����ݵ�NULL��ʾ��ʽ
#define RB_FIRST_ROW_IN_ONE_EXEC '@'  //��һ����¼��ʶ(�����ύ��)
#define RB_START_IN_ARR_EXEC '$'    //��һ����¼��ʶ(�����ύ)
#define RB_DATA_HEAD  '^'  //����ͷ��ʶ
	/******************************************************************************
	* ��������	:  TRBUnit
	* ��������	:  �ع���Ԫ
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:
	* ����		:  jin.shaohua
	*******************************************************************************/
	TRBUnit::TRBUnit()
	{
		pQuery = NULL;
		sSQL = NULL;
	}

	TRBUnit::~TRBUnit()
	{
		SAFE_DELETE(pQuery);
		SAFE_DELETE(sSQL);
	}

	/******************************************************************************
	* ��������	:  TMdbRollback
	* ��������	:  �ع��� ����
	* ����		:
	* ���		:
	* ����ֵ	:
	* ����		:  jin.shaohua
	*******************************************************************************/
	TMdbRollback::TMdbRollback()
	{
		TADD_FUNC("Start.");
		for(int i=0; i<MAX_ROLLBACK_COUNTS; ++i)
		{
			m_ptRBUnit[i] = NULL;
		}
		m_pDB         = NULL;
		m_pszRollback = NULL;
		m_pBuffer     = NULL;
		m_iSize       = -1;
		m_iOffSet     = -1;
		//m_pVarcharMgr = NULL;
		m_pRBValueSplit = NULL;
		TADD_FUNC("Finish.");
	}

	/******************************************************************************
	* ��������	:  TMdbRollback
	* ��������	:  �ع��� ����
	* ����		:
	* ���		:
	* ����ֵ	:
	* ����		:  jin.shaohua
	*******************************************************************************/
	TMdbRollback::~TMdbRollback()
	{
		TADD_FUNC("Start.");
		SAFE_DELETE_ARRAY(m_pszRollback);
		SAFE_DELETE_ARRAY(m_pBuffer);
		for(int i=0; i<MAX_ROLLBACK_COUNTS; ++i)
		{
			SAFE_DELETE(m_ptRBUnit[i]);
		}
		//SAFE_DELETE(m_pVarcharMgr);
		SAFE_DELETE(m_pRBValueSplit);
		TADD_FUNC("Finish.");
	}

	/******************************************************************************
	* ��������	:  SetDB
	* ��������	:  ��������DB
	* ����		:  pDB - DB
	* ���		:
	* ����ֵ	:
	* ����		:  jin.shaohua
	*******************************************************************************/
	void TMdbRollback::SetDB(TMdbDatabase* pDB,char* sDsn)
	{
		m_pDB = pDB;
		//�䳤�洢������
		/*
		SAFE_DELETE(m_pVarcharMgr);
		m_pVarcharMgr = new TMdbVarcharMgr();
		if(NULL == m_pVarcharMgr)
		{
			TADD_ERROR(-1,"m_pVarcharMgr is NULL!!!");
		}
		else
		{
			m_pVarcharMgr->SetConfig(pDB->GetShmDsn());
		}
		*/
		m_pVarcharCtrl.Init(sDsn);
		
	}


	/******************************************************************************
	* ��������	:  CreateQuery()
	* ��������	:  ����SQL��ȡ��Ӧ��Query
	* ����		:  pMdbTable, ��ṹָ��
	* ����		:  pszSQL, ԭʼSQL
	* ���		:  ��
	* ����ֵ	:  �ɹ�����Query��λ��, ʧ�ܷ���-1
	* ����		:  li.shugang
	*******************************************************************************/
	int TMdbRollback::CreateQuery(const char* sSql,TMdbSqlParser * pMdbSqlParser)
	{
		TADD_FUNC("Start.SQL=[%s].",sSql);
		int iRet = 0;
		int iPos = -1;
		if(m_pDB->GetTransactionState() == TRANS_IN)
		{
			memset(m_sSQL,0x00, sizeof(m_sSQL));
			if(m_pBuffer == NULL)
			{
				m_pBuffer = new(std::nothrow) char[MAX_VALUE_LEN];
				CHECK_OBJ(m_pBuffer);
			}
			//�����ҵ�һ�����еĻع��ڵ㵥Ԫ
			iPos = GetFreePos();
			if(iPos < 0)
			{
				CHECK_RET(-1,"No free pos sql = [%s].",sSql);
			}
			TMdbQuery * pQuery = NULL;
			CHECK_RET(pMdbSqlParser->GenRollbackSql(m_sSQL),"GenRollbackSql failed.");
			pQuery = m_pDB->CreateDBQuery();
			pQuery->CloseSQL();
	        //pQuery->SetSQL(m_sSQL,QUERY_NO_ROLLBACK,0); //���Ϊ2������?
	        if(m_ptRBUnit[iPos]->sSQL == NULL)
	    	{
				m_ptRBUnit[iPos]->sSQL = new char[MAX_SQL_LEN];
	    	}
			memset(m_ptRBUnit[iPos]->sSQL, 0, MAX_SQL_LEN);
			SAFESTRCPY(m_ptRBUnit[iPos]->sSQL,MAX_SQL_LEN,m_sSQL);
			m_ptRBUnit[iPos]->pQuery = pQuery;
		}
		TADD_FUNC("Finish(iPos=%d).", iPos);
		return 0==iRet?iPos:iRet;
	}

	/******************************************************************************
	* ��������	:  GetFreePos
	* ��������	:  ��ȡһ�����еĻع���Ԫ
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  >0 - �ɹ�; <0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::GetFreePos()
	{
		int iRet = 0;
		int iPos = -1;
		for(int i=0; i<MAX_ROLLBACK_COUNTS; ++i)
		{
			if(m_ptRBUnit[i] == NULL)
			{
				iPos = i;
				m_ptRBUnit[i] = new(std::nothrow) TRBUnit();
				if(m_ptRBUnit[i] == NULL)
				{
					CHECK_RET(-1,"new(std::nothrow) TRBUnit() failed");
				}
				break;
			}
		}
		TADD_FUNC("Finish(iPos=%d).", iPos);
		return 0==iRet?iPos:iRet;
	}

	/******************************************************************************
	* ��������	:  PushData()
	* ��������	:  ����Pos��ȡ��Ӧ��SQL����
	* ����		:  iPos, Query��λ�ã�0---999
	* ����		:  pData, ʵ�ʼ�¼�ĵ�ַ
	* ����		:  iCounts, �Ѿ�ִ�еļ�¼��
	* ���		:  ��
	* ����ֵ	:  �ɹ�����Query�ľ��, ʧ�ܷ���NULL
	* ����		:  li.shugang
	*******************************************************************************/
	int TMdbRollback::PushData(int iPos, const char* pData, int iCounts,const char * pExtraDataAddr,TMdbRowCtrl * pRowCtrl)
	{
		TADD_FUNC("Start.iPos[%d],pData[%p],iCounts[%d].",iPos,pData,iCounts);
		int iRet = 0;
		if(iPos < 0 || iPos >= MAX_ROLLBACK_COUNTS)
		{
			CHECK_RET(ERR_APP_PARAM_INVALID,"iPos[%d] is invalid.",iPos);
		}
		if(m_ptRBUnit[iPos] == NULL)
		{
			CHECK_RET(ERR_APP_PARAM_INVALID,"(iPos=%d) Not exist.",iPos);
		}
		//�����һ��SQL�տ�ʼִ�У�Ҫ�����ǩ�������ڱ�SQLʧ��ʱ���Ա���˵�ǰ����
		if(iCounts == 0)
		{
			m_pBuffer[0] = RB_FIRST_ROW_IN_ONE_EXEC;
		}
		else
		{
			m_pBuffer[0] = '\0';
		}
		m_pBuffer[1] = '\0';
		sprintf(m_pBuffer, "%s^^%d", m_pBuffer, iPos);
	    TMdbSqlParser * pMdbSqlParser = NULL;
	    pMdbSqlParser = new(std::nothrow) TMdbSqlParser();
	    if(NULL == pMdbSqlParser)
	    {
	        CHECK_RET(ERR_OS_NO_MEMROY,"m_pMdbSqlParser is NULL");
	    }
		CHECK_RET(pMdbSqlParser->SetDB(m_pDB->GetShmDsn(),TMdbConfigMgr::GetMdbConfig(m_pDB->GetDSN())),"ERROR_DB_NOT_CONNECT [%s].",pMdbSqlParser->m_tError.GetErrMsg());
	    if(0 == m_ptRBUnit[iPos]->sSQL[0])
	    {
	        TADD_ERROR(ERR_SQL_INVALID,"sSQL is NULL.");
	    }
	    //��β���';' ��Ȼ����ʧ��
	    TMdbNtcStrFunc::Trim(m_ptRBUnit[iPos]->sSQL,' ');
	    int iLen = strlen(m_ptRBUnit[iPos]->sSQL);
	    if(';' != m_ptRBUnit[iPos]->sSQL[iLen-1])
	    {
	        m_ptRBUnit[iPos]->sSQL[iLen] = ';';
	        m_ptRBUnit[iPos]->sSQL[iLen + 1] = '\0';
	    }
		CHECK_RET(pMdbSqlParser->ParseSQL(m_ptRBUnit[iPos]->sSQL),"ERROR_SQL_INVALID [%s].",pMdbSqlParser->m_tError.GetErrMsg());
    	ST_MEM_VALUE_LIST * pMemValueList = &(pMdbSqlParser->m_listInputVariable);
		int i = 0;
		for(i = 0; i<pMemValueList->iItemNum; i++)
		{
        	iRet = GetDataFromAddr(pMemValueList->pValueArray[i]->pColumnToSet,pData,pExtraDataAddr,pRowCtrl, pMdbSqlParser);
			if(0 != iRet)
			{
				if(0 != iCounts)
				{//�ع����ִ��
					RollbackOneExecute();//����������ȷ��, �����������ʧ�ܣ���ع���ǰSQL����
				}
				CHECK_RET(ERR_APP_PARAM_INVALID,"PushXData(iPos=%d) failed.buffer = [%s]",iPos,m_pBuffer);
			}
		}
		TADD_DETAIL("m_pBuffer=[%s].",m_pBuffer);
		CHECK_RET(PushDataIntoRB(),"PushDataIntoRB failed.");//��buffer����д��ع�����
    	SAFE_DELETE(pMdbSqlParser);
		return iRet;
	}

	/******************************************************************************
	* ��������	:  GetDataFromAddr
	* ��������	:  ���ڴ��л�ȡ���ݣ���д��ع�buffer
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::GetDataFromAddr(TMdbColumn * pColumn,const char* pData,const char * pExtraDataAddr,TMdbRowCtrl * pRowCtrl, TMdbSqlParser * pMdbSqlParser)
	{
		int iRet = 0;
		CHECK_OBJ(pColumn);
		CHECK_OBJ(pData);
		CHECK_OBJ(pRowCtrl);
		TADD_FUNC("Start.Column[%s],pData[%p]",pColumn->sName,pData);
		if(pRowCtrl->IsColumnNull(pColumn,pData) == true)
		{
			//�ж��Ƿ�Ϊnull
			sprintf(m_pBuffer+strlen(m_pBuffer), "%c%c",RB_COL_SPLIT,RB_COL_NULL);
			return iRet;
		}
		switch(pColumn->iDataType)
		{
		case DT_Int:
			{
        		if(pMdbSqlParser->m_stSqlStruct.iSqlType == TK_UPDATE && pColumn->bIncrementalUpdate())
				{
					//����ʹ����������
					CHECK_OBJ(pExtraDataAddr);
					long long* pInt = (long long*)&pExtraDataAddr[pColumn->iOffSet];
					sprintf(m_pBuffer+strlen(m_pBuffer), "%c%lld",RB_COL_SPLIT, *pInt);
				}
				else
				{
					long long* pInt = (long long*)&pData[pColumn->iOffSet];
					sprintf(m_pBuffer+strlen(m_pBuffer), "%c%lld",RB_COL_SPLIT, *pInt);
				}
			}
			break;
		case DT_Char:
			{
				//ע���ַ����߽�����
				sprintf(m_pBuffer+strlen(m_pBuffer), "%c%s",RB_COL_SPLIT, (char*)&pData[pColumn->iOffSet]);
			}
			break;
		case DT_VarChar:
		case DT_Blob:
			{
				//�䳤���ݻ�ȡ

            sprintf(m_pBuffer+strlen(m_pBuffer), "%c",RB_COL_SPLIT);
            m_pVarcharCtrl.GetVarcharValue(&m_pBuffer[strlen(m_pBuffer)], (char*)&pData[pColumn->iOffSet]);
			}
			break;
		case DT_DateStamp:
			{
				if(sizeof(int) == pColumn->iColumnLen )
				{
					//ѹ��ʱ������
					char sTime[15] = {0};
					int *pInt = (int*)&pData[pColumn->iOffSet];
					TMdbDateTime::TimeToString(*pInt,sTime);
					sprintf(m_pBuffer+strlen(m_pBuffer),"%c%s",RB_COL_SPLIT,sTime);
				}
				else if(sizeof(long long) == pColumn->iColumnLen)
				{
					//��ѹ��ʱ������
					char sTime[15] = {0};
					long long *pLong = (long long*)&pData[pColumn->iOffSet];
					TMdbDateTime::TimeToString(*pLong,sTime);
					sprintf(m_pBuffer+strlen(m_pBuffer),"%c%s",RB_COL_SPLIT,sTime);
				}
				else
				{
					sprintf(m_pBuffer+strlen(m_pBuffer),"%c%s",RB_COL_SPLIT,(char*)&pData[pColumn->iOffSet]);
				}
			}
			break;
		default:
			CHECK_RET(-1,"iDataType[%d] is invalid.",pColumn->iDataType);
			break;
		}
		TADD_FUNC("Finish.iRet(%d).",iRet);
		return iRet;
	}
	/******************************************************************************
	* ��������	:  ReAlloc
	* ��������	:  ���ݵ�ǰ�ع��Σ����·���ع��ε��ڴ棬���ʧ�ܣ�����-1
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::ReAlloc()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		try
		{
			if(m_iSize < 0)
			{
				char* pTemp = new(std::nothrow) char[1024*1024];
				if(NULL == pTemp)
				{
					CHECK_RET(-1,"new 1M memory failed.");
				}
				m_pszRollback = pTemp;
				m_iSize       = 1;
				m_iOffSet     = m_iSize*1024*1024;
			}
			else if(m_iSize >= 1024)
			{
				CHECK_RET(-1,"rollback segment is too large > %dM",m_iSize);
			}
			else
			{
				char* pTemp = new(std::nothrow) char[m_iSize*2*1024*1024];
				if(pTemp == NULL)
				{
					CHECK_RET(-1,"new %dM memory failed.",m_iSize*2);
				}
				int iDataSize = m_iSize*1024*1024-m_iOffSet;
				TADD_DETAIL("m_iSize=%dM, m_iOffSet=%d, iDataSize=(%d).", m_iSize, m_iOffSet, iDataSize);
				memcpy(&pTemp[m_iSize*2*1024*1024-iDataSize], &m_pszRollback[m_iOffSet], iDataSize);
				m_iOffSet = m_iSize*2*1024*1024-iDataSize;
				m_iSize   = m_iSize*2;

				delete m_pszRollback;
				m_pszRollback = pTemp;
				TADD_DETAIL("m_iSize=%dM, m_iOffSet=%d.",m_iSize,m_iOffSet);
			}
		}
		catch(...)
		{
			CHECK_RET(-1,"memory not enough, ask %dM.",m_iSize*2);
		}
		TADD_FUNC("Finish(iRet=%d).", iRet);
		return iRet;
	}

	/******************************************************************************
	* ��������	:  PushDataIntoRB
	* ��������	:  ��buffer����д��ع�����
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::PushDataIntoRB()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		//����ǵ�һ�η���
		if(m_pszRollback == NULL)
		{
			CHECK_RET(ReAlloc(),"ReAlloc(1M) failed.");
		}
		TADD_DETAIL("m_pBuffer=[%s].", m_pBuffer);
		//����ռ䲻�㣬������������ع���
		int iLen = strlen(m_pBuffer)+1;
		if(m_iOffSet < iLen)
		{
			iRet =  ReAlloc();
			if(iRet < 0)
			{
				TADD_ERROR(ERR_OS_NO_MEMROY,"ReAlloc(%dM) failed.", m_iSize);
				if(m_pBuffer[0] == '^' && m_pBuffer[1] == '^')
					RollbackOneExecute();
				return iRet;
			}
		}
		TADD_DETAIL(" m_iOffSet=%d, iLen=%d.", m_iOffSet, iLen);
		//������ѹ��ع�����
		strcpy(&m_pszRollback[m_iOffSet-iLen], m_pBuffer);
		m_iOffSet -= iLen;
		TADD_FUNC("Finish.");
		return iRet;
	}


	/******************************************************************************
	* ��������	:  Commit()
	* ��������	:  �ύ���е�SQL����
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  �ɹ�����0, ʧ�ܷ��ظ���
	* ����		:  li.shugang
	*******************************************************************************/
	int TMdbRollback::Commit()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		m_iOffSet = m_iSize*1024*1024;
		if(m_pBuffer != NULL)
		{
			m_pBuffer[0] = 0;
		}
		m_iRowsAffected = 0;
		std::vector<int>::iterator itor = m_vCloseRBUnit.begin();
		for(; itor != m_vCloseRBUnit.end(); ++itor)
		{
			SAFE_DELETE(m_ptRBUnit[*itor]);
		}
		m_vCloseRBUnit.clear();
		TADD_FUNC("Finish(iRet=%d).", iRet);
		return iRet;
	}

	/******************************************************************************
	* ��������	:  CommitEx
	* ��������	:  �ύ����¼Ӱ�������
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::CommitEx()
	{
		TADD_FUNC("TMdbRollback::Commit() : Start.");
		int iRet = 0;
		m_iRowsAffected = 0;
		int iPos = m_iOffSet;
		while(iPos < m_iSize*1024*1024-5)
		{
			if(m_pszRollback != NULL)
			{
				if(m_pszRollback[iPos] == '^' && m_pszRollback[iPos+1] == '^')
				{
					++m_iRowsAffected;
					iPos += 2;
				}
				else
				{
					++iPos;
				}
			}
			else
			{
				break;
			}
		}
		m_iOffSet = m_iSize*1024*1024;
		if(m_pBuffer != NULL)
		{
			memset(m_pBuffer, 0, MAX_VALUE_LEN);
		}
		TADD_FUNC("Finish(iRet=%d).", iRet);
		return iRet;
	}



	/******************************************************************************
	* ��������	:  RowsAffected
	* ��������	:  ��ȡӰ��ļ�¼��
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::RowsAffected()
	{
		return m_iRowsAffected;
	}
	/******************************************************************************
	* ��������	:  SetCloseRBUnit
	* ��������	:  ��������һ���ύ����Ҫ�رյĻع���Ԫ
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::SetCloseRBUnit(int iPos)
	{
		if(iPos >=0 && iPos < MAX_ROLLBACK_COUNTS)
		{
			m_vCloseRBUnit.push_back(iPos);
		}
		return 0;
	}
	/******************************************************************************
	* ��������	:  RollbackAll
	* ��������	:  �ع�����
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::RollbackAll()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		m_iRowsAffected = 0;
		if(bIsRollbackEmpty())
		{//û����Ҫ�ع�
			TADD_DETAIL("Nothing to rollback.");
			return 0;
		}
		int iLen = m_iSize * 1024*1024;//�ع����ܴ�С
		while(m_iOffSet < iLen)
		{//��ջ���ع���
			if(RB_DATA_HEAD == m_pszRollback[m_iOffSet] && RB_DATA_HEAD == m_pszRollback[m_iOffSet+1] )
			{
				CHECK_RET(RollbackOneRecord(),"RollbackOneRecord faild,offset=[%d].",m_iOffSet);
			}
			m_iOffSet ++;
		}
		TADD_FUNC("Finish.");
		return iRet;
	}

	/******************************************************************************
	* ��������	:  RollbackOneArrayExecute
	* ��������	:  �ع�һ������ִ��
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::RollbackOneArrayExecute()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		m_iRowsAffected = 0;
		if(bIsRollbackEmpty())
		{//û����Ҫ�ع�
			TADD_DETAIL("Nothing to rollback.");
			return iRet;
		}
		int iLen = m_iSize * 1024*1024;//�ع����ܴ�С
		while(m_iOffSet < iLen)
		{//��ջ���ع���
			if(RB_START_IN_ARR_EXEC == m_pszRollback[m_iOffSet])
			{//ĳһ���εĿ�ʼ��ʶ
				m_iOffSet ++;
				break;
			}
			if(RB_DATA_HEAD == m_pszRollback[m_iOffSet] && RB_DATA_HEAD == m_pszRollback[m_iOffSet+1] )
			{
				CHECK_RET(RollbackOneRecord(),"RollbackOneRecord faild,offset=[%d].",m_iOffSet);
			}
			m_iOffSet ++;
		}
		TADD_FUNC("Finish.");
		return iRet;
	}

	/******************************************************************************
	* ��������	:  RollbackOneArrayExecute
	* ��������	:  �ع�һ��ִ��
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::RollbackOneExecute()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		m_iRowsAffected = 0;
		if(bIsRollbackEmpty())
		{//û����Ҫ�ع�
			TADD_DETAIL("Nothing to rollback.");
			return 0;
		}
		int iLen = m_iSize * 1024*1024;//�ع����ܴ�С
		bool bFirstRow = false;
		while(m_iOffSet < iLen)
		{//��ջ���ع���
			if(RB_FIRST_ROW_IN_ONE_EXEC == m_pszRollback[m_iOffSet])
			{//ĳһ���εĿ�ʼ��ʶ
				bFirstRow = true;
				m_iOffSet ++;
			}
			if(RB_DATA_HEAD == m_pszRollback[m_iOffSet] && RB_DATA_HEAD == m_pszRollback[m_iOffSet+1] )
			{
				CHECK_RET(RollbackOneRecord(),"RollbackOneRecord faild,offset=[%d].",m_iOffSet);
				if(bFirstRow){break;}
			}
			m_iOffSet ++;
		}
		TADD_FUNC("Finish.");
		return iRet;
	}

	/******************************************************************************
	* ��������	:  RollbackLastOne
	* ��������	:  ֻ�ع����һ��
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::RollbackOneRecord()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		if(bIsRollbackEmpty())
		{
			TADD_FUNC("Finish(No Data).");
			return iRet;
		}
		if(NULL == m_pRBValueSplit)
		{
			m_pRBValueSplit = new TMdbNtcSplit();
		}
		CHECK_OBJ(m_pRBValueSplit);
		TADD_DETAIL("m_iOffSet=%d", m_iOffSet);
		if(m_pszRollback[m_iOffSet] == '^' && m_pszRollback[m_iOffSet+1] == '^')
		{
			SAFESTRCPY(m_pBuffer, MAX_VALUE_LEN, &m_pszRollback[m_iOffSet+2]);
			TADD_DETAIL("m_pBuffer=%s.", m_pBuffer);
			//int iCounts = m_pRBValueSplit->SplitString(m_pBuffer,RB_COL_SPLIT);
			bool bRet=m_pRBValueSplit->SplitData(m_pBuffer,strlen(m_pBuffer), RB_COL_SPLIT);
                    if(bRet == false)
                    {
                        TADD_ERROR(ERR_APP_PARAM_INVALID,"Invalid Rollback-Data=[%s].",m_pBuffer);
                        m_iOffSet += strlen(m_pBuffer);
                        return 0;                        
                    }
                    int iCounts = m_pRBValueSplit->GetFieldCount();
			int iPos = atoi((*m_pRBValueSplit)[0]);
			if(iPos < 0)
			{
				TADD_ERROR(ERR_APP_PARAM_INVALID,"Invalid Rollback-Data=[%s].",m_pBuffer);
				m_iOffSet += strlen(m_pBuffer);
				return 0;
			}
			TADD_DETAIL("iPos=%d, iCounts=%d.", iPos, iCounts);
			if(m_ptRBUnit[iPos] == NULL)
			{
				TADD_ERROR(ERR_APP_PARAM_INVALID,"Invalid Rollback-Data=[%s], iPos=%d.",m_pBuffer, iPos);
				m_iOffSet += strlen(m_pBuffer);
				return 0;
			}
			try
			{
				if(m_ptRBUnit[iPos]->pQuery == NULL)
				{
					TADD_ERROR(ERR_APP_PARAM_INVALID,"pQuery=NULL, Invalid Rollback-Data=[%s], iPos=%d.",m_pBuffer, iPos);
					m_iOffSet += strlen(m_pBuffer);
					return 0;
				}
				m_ptRBUnit[iPos]->pQuery->SetSQL(m_ptRBUnit[iPos]->sSQL,QUERY_NO_ROLLBACK,0);
				for(int i=1; i<iCounts; ++i)
				{
					if(RB_COL_NULL == (*m_pRBValueSplit)[i][0])
					{
						// NULLֵ
						m_ptRBUnit[iPos]->pQuery->SetParameterNULL(i-1);
					}
					else
					{
						m_ptRBUnit[iPos]->pQuery->SetParameter(i-1, (*m_pRBValueSplit)[i]);
					}
				}
				m_ptRBUnit[iPos]->pQuery->Execute();
			}
			catch(TMdbException &e)
			{
				TADD_ERROR(ERR_APP_PARAM_INVALID,"Msg=%s.",e.GetErrMsg());
				TADD_ERROR(ERR_APP_PARAM_INVALID,"Invalid Rollback-Data=[%s], iPos=%d.",m_pBuffer, iPos);
				TADD_ERROR(ERR_APP_PARAM_INVALID,"SQL=%s.",m_ptRBUnit[iPos]->pQuery->GetSQL());
			}
			m_iOffSet += strlen(m_pBuffer);
			++m_iRowsAffected;
		}
		else
		{
			CHECK_RET(ERR_APP_PARAM_INVALID,"offset of rollback is [%d] ",m_iOffSet);
		}
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* ��������	:  SetArraryExecuteStart
	* ��������	:  ��������ִ�п�ʼ
	* ����		:
	* ����		:
	* ���		:
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbRollback::SetArraryExecuteStart()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		CHECK_OBJ(m_pBuffer);
		m_pBuffer[0] = RB_START_IN_ARR_EXEC;
		m_pBuffer[1] = '\0';
		CHECK_RET(PushDataIntoRB(),"PushDataIntoRB failed.");
		TADD_FUNC("Finish.");
		return iRet;
	}
//}




