/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    SqlParserHelper.cpp		
*@Description�� sql�������İ����࣬�ṩ����MDB��һЩ��Ϣ��
*				����MDB�ڲ���Ϣ�Ͻ��ܵ���Ϣ�������
*@Author:	   jin.shaohua
*@Date��	    2012.05
*@History:
******************************************************************************************/
#include "SqlParserHelper.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{


/******************************************************************************
* ��������	:  TSqlParserHelper
* ��������	:  ����
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
TSqlParserHelper::TSqlParserHelper():
m_pShmDsn(NULL),
m_pMdbDsn(NULL),
m_pMdbTable(NULL),
m_pMdbTablespace(NULL)
{

}

/******************************************************************************
* ��������	:  TSqlParserHelper
* ��������	:  ����
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
TSqlParserHelper::~TSqlParserHelper()
{

}

/******************************************************************************
* ��������	:  SetDB
* ��������	:  ����DB
* ����		:  pShmDsn - DSN��Ϣ  pMdb - mdb������Ϣ
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TSqlParserHelper::SetDB(TMdbShmDSN * pShmDsn)
{
	int iRet = 0;
	CHECK_OBJ(pShmDsn);
	m_pShmDsn = pShmDsn;
	CHECK_RET(m_pShmDsn->Attach(),"DSN Attach failed.");
	m_pMdbDsn = pShmDsn->GetInfo();
	return iRet;
}

/******************************************************************************
* ��������	:  GetMdbShmDSN
* ��������	:  ��ȡMDB shm DSN
* ����		:  ��
* ���		:  ��
* ����ֵ	:  DSN��Ϣ������
* ����		:  jin.shaohua
*******************************************************************************/
TMdbShmDSN * TSqlParserHelper::GetMdbShmDSN()
{
	return m_pShmDsn;
}

/******************************************************************************
* ��������	:  GetMdbTable
* ��������	:  ��ȡmdbtableָ��
* ����		:  sTablename ����
* ���		:  ��
* ����ֵ	:  dbtableָ��
* ����		:  jin.shaohua
*******************************************************************************/
TMdbTable * TSqlParserHelper::GetMdbTable(const char * sTablename)
{
	if(NULL == m_pMdbDsn)
	{
		return NULL;
	}
	m_pMdbTable = m_pShmDsn->GetTableByName(sTablename);
	return m_pMdbTable;
}

TMdbTableSpace* TSqlParserHelper::GetMdbTablespace(const char * sTablespaceName)
{
	if(NULL == m_pMdbDsn)
	{
		return NULL;
	}
	m_pMdbTablespace = m_pShmDsn->GetTableSpaceAddrByName(sTablespaceName);
	return m_pMdbTablespace;
}

/******************************************************************************
* ��������	:  GetMdbColumn
* ��������	:  ��ȡ��ָ��
* ����		:  pMdbTables ����Ϣָ�룬ColumnName ����
* ���		:  ��
* ����ֵ	:  NULL ����ʧ�ܣ�!NULL �ҵ���ָ��
* ����		:  jin.shaohua
*******************************************************************************/
TMdbColumn * TSqlParserHelper::GetMdbColumn(TMdbTable * pMdbTable,const char * sColumnName)
{
	if(NULL == pMdbTable)return NULL;

	TMdbColumn * pRetColumn = NULL;
	int iNum = pMdbTable->iColumnCounts;
	int i = 0;
	for(i = 0;i<iNum;i++)
	{
		pRetColumn = &pMdbTable->tColumn[i];
		if(TMdbNtcStrFunc::StrNoCaseCmp(pRetColumn->sName, sColumnName) == 0 && pRetColumn->sName[0] !=0)
		{//�鵽
			return pRetColumn;
		}
	}
	return NULL;
}
/******************************************************************************
* ��������	:  GetMdbColumn
* ��������	:  //��ȡ��λ��
* ����		:  pMdbTables ����Ϣָ�룬ColumnName ����
* ���		:  ��
* ����ֵ	:  NULL ����ʧ�ܣ�!NULL �ҵ���ָ��
* ����		:  jin.shaohua
*******************************************************************************/
int TSqlParserHelper::GetMdbColumnPos(TMdbTable * pMdbTable,const char * sColumnName)
{
	int iRet = -1;
	if(NULL == pMdbTable)return iRet;
	TMdbColumn * pRetColumn = NULL;
	int iNum = pMdbTable->iColumnCounts;
	int i = 0;
	for(i = 0;i<iNum;i++)
	{
		pRetColumn = &pMdbTable->tColumn[i];
		if(TMdbNtcStrFunc::StrNoCaseCmp(pRetColumn->sName, sColumnName) == 0 && pRetColumn->sName[0] !=0)
		{//�鵽
			return i;
		}
	}
	return iRet;
}
#if 0

/******************************************************************************
* ��������	:  BuildColumIndexMap
* ��������	:  ����column index map
* ����		:  pMdbTable ����Ϣָ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TSqlParserHelper::BuildColumIndexMap(TMdbTable * pMdbTable)
{
	int iRet = 0;
	if(NULL == pMdbTable)
	{
		return -1;
	}
	m_mapColumnToIndex.clear();
	int i = 0;
	for(i = 0;i<pMdbTable->iIndexCounts;i++)
	{
		for(int j = 0;j<MAX_INDEX_COLUMN_COUNTS;j++)
		{
			if(pMdbTable->tIndex[i].iColumnNo[j] >= 0)
			{
				m_mapColumnToIndex[pMdbTable->tIndex[i].iColumnNo[j]] = i;
			}
		}
	}
	return iRet;
}

/******************************************************************************
* ��������	:  GetIndexByColumnPos
* ��������	:  ��ȡiColumnPos����Ӧ��indexPos
* ����		:  iColummPos ������λ��
* ���		:  ��
* ����ֵ	:  ��λ�ö�Ӧ������λ��
* ����		:  jin.shaohua
*******************************************************************************/
int TSqlParserHelper::GetIndexByColumnPos(int iColummPos)
{
	if(m_mapColumnToIndex.find(iColummPos) != m_mapColumnToIndex.end())
	{
		return m_mapColumnToIndex[iColummPos];
	}
	else
	{
		return -1;
	}
}
#endif

/******************************************************************************
* ��������	:  IsPrimaryKey
* ��������	:  �ж����Ƿ�������
* ����		:  pMdbTable �����ָ��
* ����		:  pColumn   �ж���ָ��
* ���		:  ��
* ����ֵ	:  true �ǣ�false ��
* ����		:  jin.shaohua
*******************************************************************************/
bool TSqlParserHelper::IsPrimaryKey(TMdbTable * pMdbTable,TMdbColumn *pColumn)
{
	int i = 0;
	for(i = 0;i<pMdbTable->m_tPriKey.iColumnCounts;i++)
	{
		if(pColumn->iPos == pMdbTable->m_tPriKey.iColumnNo[i])
			return true;
	}
	return false;
}

/******************************************************************************
* ��������  :  GetSequenceByName
* ��������  :  ��ȡָ���ڴ�����
* ����		:  pSeqName ������
* ���		:  ��
* ����ֵ    :  �ڴ�����
* ����		:  cao.peng
*******************************************************************************/
TMemSeq * TSqlParserHelper::GetSequenceByName(const char* pSeqName)
{
    if(NULL == m_pShmDsn)
    {
        return NULL;
    }
    return m_pShmDsn->GetMemSeqByName(pSeqName);
}

/******************************************************************************
* ��������  :  GetSeqCurValue
* ��������  :  ��ȡ����NEXTֵ
* ����		:  pSeqName ������
* ���		:  ��
* ����ֵ    :  ����NEXTֵ
* ����		:  cao.peng
*******************************************************************************/
int TSqlParserHelper::GetSeqNextValue(const char* pSeqName)
{
    if(NULL == m_pShmDsn)
    {
        return 0;
    }
    TMemSeq * pMemSeq = m_pShmDsn->GetMemSeqByName(pSeqName);
    if(NULL == pMemSeq)
    {
        return ERR_DB_SEQUECE_NOT_EXIST;
    }
    pMemSeq->tMutex.Lock(true);
    MDB_INT64 iRet = pMemSeq->iCur;
    if(iRet+pMemSeq->iStep > pMemSeq->iEnd)
    {
        pMemSeq->iCur = pMemSeq->iStart;
    }
    else
    {
        pMemSeq->iCur += pMemSeq->iStep;
    }
    iRet = pMemSeq->iCur;
    pMemSeq->tMutex.UnLock(true);
    return (int)iRet;
}

/******************************************************************************
* ��������	:  CheckPrimaryKeyColumn
* ��������	:  У�����õ������Ƿ�Ϸ�
* ����		:  pTable У���
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TSqlParserHelper::CheckPrimaryKeyColumn(TMdbTable* pTable)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTable);
    //У�����������Ƿ�Ϸ�
    for(int j=0; j<pTable->m_tPriKey.iColumnCounts; ++j)
    {
        if(pTable->m_tPriKey.iColumnNo[j]<0){continue;}
        if(pTable->m_tPriKey.iColumnNo[j] >= pTable->iColumnCounts)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Table=[%s] has invalid primary-key=[column-pos = %d].",\
                pTable->sTableName, pTable->m_tPriKey.iColumnNo[j]);
            return ERR_TAB_PK_COLUMN_POS_INVALID;
        }
        //����������Ƿ�����ΪNULL����
        TMdbColumn &tColumn = pTable->tColumn[pTable->m_tPriKey.iColumnNo[j]];
        if(tColumn.m_bNullable)
        {
            TADD_ERROR(ERROR_UNKNOWN,"In table %s,the primary key column[%s] cannot be configured for NULL.",\
                  pTable->sTableName,tColumn.sName);
            return ERR_TAB_PR_COLUMN_IS_NOT_NULLABLE;
        }
        //����������Ƿ�����ȱʡֵ
        if(tColumn.bIsDefault)
        {
            TADD_ERROR(ERROR_UNKNOWN,"In table %s,the primary key column[%s] cannot have default values.",\
                pTable->sTableName,tColumn.sName);
            return ERR_TAB_PK_COLUMN_HAS_DEFAULT_VALUE;
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}

//}
