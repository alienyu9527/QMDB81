/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    SqlParserHelper.cpp		
*@Description： sql解析器的帮助类，提供关于MDB的一些信息，
*				将与MDB内部信息较紧密的信息剥离出来
*@Author:	   jin.shaohua
*@Date：	    2012.05
*@History:
******************************************************************************************/
#include "SqlParserHelper.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{


/******************************************************************************
* 函数名称	:  TSqlParserHelper
* 函数描述	:  构造
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
TSqlParserHelper::TSqlParserHelper():
m_pShmDsn(NULL),
m_pMdbDsn(NULL),
m_pMdbTable(NULL),
m_pMdbTablespace(NULL)
{

}

/******************************************************************************
* 函数名称	:  TSqlParserHelper
* 函数描述	:  析构
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
TSqlParserHelper::~TSqlParserHelper()
{

}

/******************************************************************************
* 函数名称	:  SetDB
* 函数描述	:  设置DB
* 输入		:  pShmDsn - DSN信息  pMdb - mdb链接信息
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
* 函数名称	:  GetMdbShmDSN
* 函数描述	:  获取MDB shm DSN
* 输入		:  无
* 输出		:  无
* 返回值	:  DSN信息管理区
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbShmDSN * TSqlParserHelper::GetMdbShmDSN()
{
	return m_pShmDsn;
}

/******************************************************************************
* 函数名称	:  GetMdbTable
* 函数描述	:  获取mdbtable指针
* 输入		:  sTablename 表名
* 输出		:  无
* 返回值	:  dbtable指针
* 作者		:  jin.shaohua
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
* 函数名称	:  GetMdbColumn
* 函数描述	:  获取列指针
* 输入		:  pMdbTables 表信息指针，ColumnName 列名
* 输出		:  无
* 返回值	:  NULL 查找失败，!NULL 找到列指针
* 作者		:  jin.shaohua
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
		{//查到
			return pRetColumn;
		}
	}
	return NULL;
}
/******************************************************************************
* 函数名称	:  GetMdbColumn
* 函数描述	:  //获取列位置
* 输入		:  pMdbTables 表信息指针，ColumnName 列名
* 输出		:  无
* 返回值	:  NULL 查找失败，!NULL 找到列指针
* 作者		:  jin.shaohua
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
		{//查到
			return i;
		}
	}
	return iRet;
}
#if 0

/******************************************************************************
* 函数名称	:  BuildColumIndexMap
* 函数描述	:  构建column index map
* 输入		:  pMdbTable 表信息指针
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
* 函数名称	:  GetIndexByColumnPos
* 函数描述	:  获取iColumnPos所对应的indexPos
* 输入		:  iColummPos 列所在位置
* 输出		:  无
* 返回值	:  列位置对应的索引位置
* 作者		:  jin.shaohua
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
* 函数名称	:  IsPrimaryKey
* 函数描述	:  判断列是否是主键
* 输入		:  pMdbTable 表对象指针
* 输入		:  pColumn   列对象指针
* 输出		:  无
* 返回值	:  true 是，false 否
* 作者		:  jin.shaohua
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
* 函数名称  :  GetSequenceByName
* 函数描述  :  获取指定内存序列
* 输入		:  pSeqName 序列名
* 输出		:  无
* 返回值    :  内存序列
* 作者		:  cao.peng
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
* 函数名称  :  GetSeqCurValue
* 函数描述  :  获取序列NEXT值
* 输入		:  pSeqName 序列名
* 输出		:  无
* 返回值    :  序列NEXT值
* 作者		:  cao.peng
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
* 函数名称	:  CheckPrimaryKeyColumn
* 函数描述	:  校验配置的主键是否合法
* 输入		:  pTable 校验表
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TSqlParserHelper::CheckPrimaryKeyColumn(TMdbTable* pTable)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTable);
    //校验主键的列是否合法
    for(int j=0; j<pTable->m_tPriKey.iColumnCounts; ++j)
    {
        if(pTable->m_tPriKey.iColumnNo[j]<0){continue;}
        if(pTable->m_tPriKey.iColumnNo[j] >= pTable->iColumnCounts)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Table=[%s] has invalid primary-key=[column-pos = %d].",\
                pTable->sTableName, pTable->m_tPriKey.iColumnNo[j]);
            return ERR_TAB_PK_COLUMN_POS_INVALID;
        }
        //检查主键列是否配置为NULL属性
        TMdbColumn &tColumn = pTable->tColumn[pTable->m_tPriKey.iColumnNo[j]];
        if(tColumn.m_bNullable)
        {
            TADD_ERROR(ERROR_UNKNOWN,"In table %s,the primary key column[%s] cannot be configured for NULL.",\
                  pTable->sTableName,tColumn.sName);
            return ERR_TAB_PR_COLUMN_IS_NOT_NULLABLE;
        }
        //检查主键列是否配置缺省值
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
