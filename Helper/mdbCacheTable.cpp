/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbCacheTable.h		
*@Description�� mdb ��ʱ�����
*@Author:			jin.shaohua
*@Date��	    2013.12
*@History:
******************************************************************************************/
//#include "BillingSDK.h"
#include "Helper/mdbCacheTable.h"
#include "Helper/mdbSQLParser.h"
#include "Helper/mdbMemValue.h"
#include "Helper/mdbMalloc.h"


//using namespace ZSmart::BillingSDK;
//namespace QuickMDB{
		TMdbCacheRow::TMdbCacheRow():m_pDataAddr(NULL)
	{

	}
	TMdbCacheRow::~TMdbCacheRow()
	{
		Clean(false);
	}
	/******************************************************************************
	* ��������	:  Clean
	* ��������	:  ��������
	* ����		:  bRef - �Ƿ�������
	* ���		:  
	* ����ֵ	:  
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheRow::Clean(bool bRef)
	{
		m_pDataAddr = NULL;
		if(false == bRef)
		{//ֻ������
			unsigned int i = 0;
			for(i = 0;i< m_vAggValue.size();++i)
			{
				QMDB_MALLOC->ReleaseMemValue(m_vAggValue[i]);
			}
			for(i = 0;i< m_vGroupBy.size();++i)
			{
				QMDB_MALLOC->ReleaseMemValue(m_vGroupBy[i]);
			}
			for(i = 0;i< m_vHaving.size();++i)
			{
				QMDB_MALLOC->ReleaseMemValue(m_vHaving[i]);
			}
			for(i = 0;i< m_vOrderby.size();++i)
			{
				QMDB_MALLOC->ReleaseMemValue(m_vOrderby[i]);
			}
		}
		m_vAggValue.clear();
		m_vGroupBy.clear();
		m_vHaving.clear();
		m_vOrderby.clear();
		return 0;
	}
	/******************************************************************************
	* ��������	:  CompareGroupBy
	* ��������	:  �Ƚ�group byֵ
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheRow::CompareGroupBy(std::vector<ST_MEM_VALUE * > & vGroupBy)
	{
		int iRet = 0;
		if(0 == vGroupBy.size()){return -1;} //û��group by����ʾ����һ��
		if(vGroupBy.size() != m_vGroupBy.size())
		{
			TADD_WARNING("vGroupBy.size() != m_vGroupBy.size()");
			return vGroupBy.size() - m_vGroupBy.size();
		}
		unsigned int i = 0;
		for(i = 0;i < vGroupBy.size();++i)
		{
			if(TMdbMemValue::CompareExprValue(vGroupBy[i],m_vGroupBy[i]) != 0)
			{//��һ��
				return -1;
			}
		}
		return iRet;
	}
	/******************************************************************************
	* ��������	:  SetUnused
	* ��������	:  ����Ϊδʹ��
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheRow::SetUnused()
	{
		m_pDataAddr = NULL;
		return 0;
	}


	/******************************************************************************
	* ��������	:  TMdbTemporaryTable
	* ��������	:  ��ʱ��
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  
	* ����		:  jin.shaohua
	*******************************************************************************/
	TMdbCacheTable::TMdbCacheTable():
	m_pCacheRowDef(NULL),
		m_pvAggExpr(NULL),
		m_iRowCached(0),
		m_bSingleAgg(false),
		m_hashTable(NULL)
		
	{
		m_stOrderbyInfo.clear();
	}
	/******************************************************************************
	* ��������	:  ~TMdbTemporaryTable
	* ��������	:  ��ʱ��
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  
	* ����		:  jin.shaohua
	*******************************************************************************/
	TMdbCacheTable::~TMdbCacheTable()
	{
		Destroy();
	}
	/******************************************************************************
	* ��������	:  Clear
	* ��������	:  ������ʱ��
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheTable::Clear()
	{
		int iRet = 0;
		m_llNextPos = 0;
		unsigned int i = 0;
		for(i = 0;i< m_vData.size();++i)
		{
			m_vData[i]->SetUnused();
		}
		m_iRowCached = 0;
		if(NULL != m_pvAggExpr)
		{
			for(i = 0;i < m_pvAggExpr->size();++i)
			{
				ExprCalcClearProperty((*m_pvAggExpr)[i],CALC_PER_Exec|CALC_NO_NEED);
				ExprCalcSetProperty((*m_pvAggExpr)[i],CALC_PER_Row);
			}
		}
		return iRet;
	}

	/******************************************************************************
	* ��������	:  Create
	* ��������	:  ����,group by ,order by , �ۺϺ���������Ҫ�����
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheTable::Create(TMdbSqlParser * pSqlParser)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		Destroy();
		CHECK_OBJ(pSqlParser);
		if(TK_SELECT != pSqlParser->m_stSqlStruct.iSqlType){return iRet;}//ֻ�в�ѯ��Ҫ�����
		if( 0 != pSqlParser->m_listInputAggValue.iItemNum || 
			0 != pSqlParser->m_listOutputGroupBy.iItemNum ||
			0 != pSqlParser->m_listOutputOrderby.iItemNum)
		{
			m_pCacheRowDef = new TMdbCacheRow();
			//ֻ�ǿ�����ָ��
			m_pCacheRowDef->m_vOrderby  = pSqlParser->m_listOutputOrderby.vMemValue;
			m_pCacheRowDef->m_vGroupBy = pSqlParser->m_listOutputGroupBy.vMemValue;
			m_pCacheRowDef->m_vHaving    = pSqlParser->m_listOutputHaving.vMemValue;
			m_pCacheRowDef->m_vAggValue = pSqlParser->m_listInputAggValue.vMemValue;
			m_pvAggExpr = &(pSqlParser->m_vAggExpr);
			if(pSqlParser->m_stSqlStruct.bIsSingleGroupFunc)
			{
				m_bSingleAgg = (0 == m_pCacheRowDef->m_vGroupBy.size());
			}
			else
			{
				m_bSingleAgg = (0 == m_pCacheRowDef->m_vGroupBy.size() && 0 == m_pCacheRowDef->m_vOrderby.size());
			}
			m_stOrderbyInfo = pSqlParser->m_stSqlStruct.stOrderbyInfo;
		}
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* ��������	:  Destroy
	* ��������	:  ����
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheTable::Destroy()
	{
		if(NULL != m_pCacheRowDef)
		{
			m_pCacheRowDef->Clean(true);
		}
		SAFE_DELETE(m_pCacheRowDef);
		unsigned int i = 0;
		for(i = 0;i< m_vData.size();++i)
		{
			SAFE_DELETE(m_vData[i]);
		}
		m_vData.clear();
		m_pvAggExpr = NULL;
		m_iRowCached = 0;
		m_bSingleAgg = false;
		m_stOrderbyInfo.clear();
		return 0;
	}

	/******************************************************************************
	* ��������	:  GetAggValue
	* ��������	:  ��ȡ�ۺ�ֵ
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheTable::GetAggValue(char * pDataAddr,TMdbCacheRow * & pCacheRow)
	{
		int iRet = 0;
		if(m_bSingleAgg){return iRet;}
		pCacheRow = NULL;

		if(m_pCacheRowDef->m_vGroupBy.size() != 0)
			//pCacheRow = GetRowByGroupValue(m_pCacheRowDef->m_vGroupBy);
		{
			iRet = GetRowByHashTable(pCacheRow);
			CHECK_RET(iRet,"GetAggValue:get record from hash table failed");
		}
		
		
		if(NULL == pCacheRow)
		{//��ֵ����һ��
			TADD_FLOW("need to insert");
			Insert(pCacheRow);
		}
		else
		{//���¾ۺ�ֵ
			TADD_FLOW("need to update");
		}
		CopyMemValueList(m_pCacheRowDef->m_vAggValue,pCacheRow->m_vAggValue,false);
		CopyMemValueList(m_pCacheRowDef->m_vHaving,pCacheRow->m_vHaving,false);
		pCacheRow->m_pDataAddr = pDataAddr;
		return iRet;
	}

	/******************************************************************************
	* ��������	:  SetAggValue
	* ��������	: ���þۺ�ֵ
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheTable::SetAggValue(char * pDataAddr,TMdbCacheRow * & pCacheRow)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		if(m_bSingleAgg || NULL == pCacheRow){return iRet;}
		CHECK_OBJ(pCacheRow);
		CopyMemValueList(pCacheRow->m_vAggValue,m_pCacheRowDef->m_vAggValue,false);
		CopyMemValueList(pCacheRow->m_vHaving,m_pCacheRowDef->m_vHaving,false);
		pCacheRow->m_pDataAddr = pDataAddr;
		TADD_FUNC("End.");
		return iRet;
	}

	/******************************************************************************
	* ��������	:  GetRowByGroupValue
	* ��������	:  ����Group by ��ȡ��ֵ
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	TMdbCacheRow * TMdbCacheTable::GetRowByGroupValue(std::vector<ST_MEM_VALUE * > & vGroupBy)
	{
		TADD_FUNC("Start.");
		//TODO;��ȫ���������Ժ�Ҫ��������ʽ����
		unsigned int i = 0;
		TMdbCacheRow * pRetRow = NULL;
		for(i = 0;i < m_iRowCached;++i)
		{
			if(false == m_vData[i]->IsUnused() && m_vData[i]->CompareGroupBy(vGroupBy) == 0 )
			{//�ҵ�
				pRetRow = m_vData[i];
				break;
			}
		}
		TADD_FUNC("Finish.");
		return pRetRow;
	}
	/******************************************************************************
	* ��������	:  CreateHashTable
	* ��������	:  ��������group by��hash��
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  miao.jianxin
	*******************************************************************************/
	int TMdbCacheTable::CreateHashTable()
	{
		m_hashTable =  new(std::nothrow) HTAB();
		if(m_hashTable == NULL)
			return -1;
		
		int ret = m_hashTable->hash_create();
		
		if(ret != 0)
			return ret;
		
		m_hashTable->set_cache_table(this);
		return 0;

	}
	/******************************************************************************
	* ��������	:  DestroyHashTable
	* ��������	:  ����hash��
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  miao.jianxin
	*******************************************************************************/
	int TMdbCacheTable::DestroyHashTable()
	{
		if(m_hashTable != NULL)
		{
			m_hashTable->hash_destroy();
			SAFE_DELETE(m_hashTable);
		}

		return 0;

	}
	/******************************************************************************
	* ��������	:  GetHashValue
	* ��������	:  ����group by��hashֵ
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  miao.jianxin
	*******************************************************************************/
	unsigned int TMdbCacheTable::GetHashValue(std::vector<ST_MEM_VALUE*>  vGroup)
	{
		int i;
		char     m_sTempValue[MAX_BLOB_LEN];
		m_sTempValue[0] = 0;
		for(i=0; i<vGroup.size(); i++)
		{
			if(vGroup[i] == NULL)
				return 0;
			else if(vGroup[i]->IsNull())
				snprintf(m_sTempValue+strlen(m_sTempValue),sizeof(m_sTempValue)-strlen(m_sTempValue),";0");
			else
			{
				if (MemValueHasProperty(vGroup[i],MEM_Int))
				{
					snprintf(m_sTempValue+strlen(m_sTempValue),sizeof(m_sTempValue)-strlen(m_sTempValue),";%lld",vGroup[i]->lValue);
					
      
				}

				else
				{
					snprintf(m_sTempValue+strlen(m_sTempValue),sizeof(m_sTempValue)-strlen(m_sTempValue),";%s",vGroup[i]->sValue);

				}
			}
		}

		unsigned int  hash = 0; 
		char* ptr= m_sTempValue;
		i = 0;
		 while(ptr[i] != 0)
       	 {
       		 hash = hash *33 + ptr[i]; 
			 i++;
		  }
		 
        return hash; 

	}

	/******************************************************************************
	* ��������	:  GetRowByHashTable
	* ��������	:  ��ȡgroup by�ֶ���ͬ�ļ�¼
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  miao.jianxin
	*******************************************************************************/
	int TMdbCacheTable::GetRowByHashTable(TMdbCacheRow * &pRetRow)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
        int findNode = -1;
		unsigned int hashValue;
		pRetRow = NULL;
		//compute hash value;
		hashValue = GetHashValue(m_pCacheRowDef->m_vGroupBy);
		
		iRet = m_hashTable->hash_search_with_hash_value(hashValue,m_iRowCached, findNode);
		CHECK_RET(iRet,"GetRowByHashTable: do operation with hash table failed");
		
		if(findNode != -1)
		{
			pRetRow = m_vData[findNode];
		}
		
		TADD_FUNC("Finish.");
		return 0;
	}
	
	/******************************************************************************
	* ��������	:  bNeedCached
	* ��������	:  �Ƿ���Ҫ����
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	bool TMdbCacheTable::bNeedCached()
	{
		return (NULL != m_pCacheRowDef);
	}

	/******************************************************************************
	* ��������	:  Insert
	* ��������	:  ����ֵ
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheTable::Insert(TMdbCacheRow *  & pNewRow)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		//TODO:����������Ƿ����
		if(m_iRowCached >= m_vData.size())
		{
			pNewRow = new TMdbCacheRow();
			m_vData.push_back(pNewRow);
		}
		else
		{
			pNewRow = m_vData[m_iRowCached];
		}
		m_iRowCached ++;
		TADD_FLOW("Insert one row to cache table,m_iRowCached = [%d],vData.size=[%d]",m_iRowCached,m_vData.size());
		CHECK_OBJ(pNewRow);
		CopyMemValueList(pNewRow->m_vOrderby,m_pCacheRowDef->m_vOrderby,false);
		CopyMemValueList(pNewRow->m_vGroupBy,m_pCacheRowDef->m_vGroupBy,false);
		CopyMemValueList(pNewRow->m_vHaving,m_pCacheRowDef->m_vHaving,false);
		CopyMemValueList(pNewRow->m_vAggValue,m_pCacheRowDef->m_vAggValue,true);
		TADD_FUNC("Finish.");
		return iRet;
	}



	/******************************************************************************
	* ��������	:  CopyMemValueList
	* ��������	:  ����memlist;
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheTable::CopyMemValueList(std::vector<ST_MEM_VALUE * > & vDest,std::vector<ST_MEM_VALUE * > & vSource,bool bClearValue)
	{
		unsigned int i = 0;
		if(0 == vDest.size())
		{
			for(i = 0;i < vSource.size();++i)
			{
				ST_MEM_VALUE * pNew = vSource[i]->Dup();
				vDest.push_back(pNew);
			}
		}
		else
		{
			for(i = 0;i < vSource.size();++i)
			{
				vSource[i]->CopyToMemValue(vDest[i]);
			}
		}
		if(bClearValue)
		{
			for(i = 0;i < vDest.size();++i)
			{
				vDest[i]->ClearValue();
			}
		}
		return 0;
	}
	/******************************************************************************
	* ��������	:  FinishInsert
	* ��������	:  ��������
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheTable::FinishInsert(ST_MEM_VALUE_LIST & stLimitMemValueList)
	{
		TADD_FUNC("Start.");
		int iRet  = 0;
    	unsigned int i = 0;
		if(m_pCacheRowDef->m_vHaving.size() != 0)
		{//
			for(i = 0;i < m_iRowCached;)
			{
				if(0 == m_vData[i]->m_vHaving[0]->lValue)
				{//having����������
					m_vData[i]->SetUnused();//����Ϊδʹ����
					TADD_FLOW("Row[%d] not satisfy having condition.",i);
					TMdbCacheRow * pTemp = m_vData[i];
					m_vData[i] = m_vData[m_iRowCached - 1];
					m_vData[m_iRowCached - 1] = pTemp;
					m_iRowCached --;
				}
				else
				{
					++i;
				}
			}
		}
		if(m_pCacheRowDef->m_vOrderby.size() != 0)
		{
			//��Ҫorder by ����
            //����limit��offset��ֵ���õ���Ҫ����ļ�¼����
            long long iSortNum = -1;
			if(stLimitMemValueList.iItemNum == 1)
    		{
				iSortNum = stLimitMemValueList.vMemValue[0]->lValue;
     		}
            else if(stLimitMemValueList.iItemNum  == 2)
            {
				iSortNum = stLimitMemValueList.vMemValue[0]->lValue; //limit
				iSortNum += stLimitMemValueList.vMemValue[1]->lValue;//offset
            }

			
			if(iSortNum > 0 && iSortNum < m_iRowCached)//ͨ������������
			{
				HeapSortForKNum(m_iRowCached,iSortNum);

				
			}
			else if(iSortNum == 0)
			{
				
			}
			else
			{
				QuickSort(0,m_iRowCached-1);
			
			}
			
		}
		//�Ծۺϱ��ʽ���ټ���
		CHECK_OBJ(m_pvAggExpr);
		for(i = 0;i < m_pvAggExpr->size();++i)
		{
			ExprCalcClearProperty((*m_pvAggExpr)[i],CALC_PER_Row|CALC_NO_NEED);
			ExprCalcSetProperty((*m_pvAggExpr)[i],CALC_PER_Exec|CALC_NO_NEED);
		}
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* ��������	:  Open
	* ��������	: �򿪻�����α�ָ��ͷ
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int   TMdbCacheTable::Open()
	{
		m_llNextPos = 0;
		return 0;
	}

	/******************************************************************************
	* ��������	:  Next
	* ��������	: ���������
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	bool TMdbCacheTable::Next(char * & pDataAddr)
	{
		TADD_FUNC("Start.");
		if(m_bSingleAgg)
		{
			m_llNextPos ++;
			return (1 ==  m_llNextPos);
		}
		else
		{
			while(m_llNextPos < m_iRowCached)
			{
				if(m_vData[m_llNextPos]->IsUnused()){m_llNextPos ++;continue;}//����ֵû�У�����
				pDataAddr = m_vData[m_llNextPos]->m_pDataAddr;//��ȡ���ݵ�ַ
				CopyMemValueList(m_pCacheRowDef->m_vAggValue,m_vData[m_llNextPos]->m_vAggValue,false);//���ֵ
				m_llNextPos ++;
				return true;
			}
		}
		TADD_FUNC("Finish.");
		return false;
	}
	/******************************************************************************
	* ��������	:  CompareOrderby
	* ��������	: order by �Ƚ�
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheTable::CompareOrderby(TMdbCacheRow * pLeft,TMdbCacheRow * pRight)
	{
		long long llRet = 0;
		unsigned int i = 0;
		for(i = 0; i < pLeft->m_vOrderby.size();++i)
		{
			llRet = TMdbMemValue::CompareExprValue(pLeft->m_vOrderby[i],pRight->m_vOrderby[i]);
			if(0 != llRet)
			{
				llRet = (MDB_SO_DESC == m_stOrderbyInfo.iSortType[i])?0-llRet:llRet;
				break;
			}
		}
		if(0 == llRet){return 0;}
		if(0 > llRet){return -1;}
		if(0 < llRet){return 1;}
	}
	
	/******************************************************************************
	* ��������	:  MinHeap
	* ��������	:  ����������С��
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  miao.jianxin

	*******************************************************************************/
	int TMdbCacheTable::MinHeap(int iParent, int iLen)
	{
		int iSmallInx = -1;  
		int iLeft = iParent*2+1;  
		int iRight = iParent*2+2;
		if(iParent < iLen/2)
		{
			if (iLeft < iLen && CompareOrderby(m_vData[iLeft],m_vData[iParent]) < 0)  
				iSmallInx = iLeft;  
			else  
				iSmallInx = iParent;      
			if (iRight < iLen && CompareOrderby(m_vData[iRight],m_vData[iSmallInx]) < 0)  
				iSmallInx = iRight;   
			
			if (iSmallInx != iParent) 
			{  
				//swap(m_vData[iParent], m_vData[iLargeInx]);
				TMdbCacheRow * pTempRow;
				pTempRow = m_vData[iParent];
				m_vData[iParent] = m_vData[iSmallInx];
				m_vData[iSmallInx] = pTempRow;
				MinHeap(iSmallInx, iLen);  
			}  
		}
        return 0;
	}

	
	/******************************************************************************
	* ��������	:  BuildHeap
	* ��������	:  ����������Ԫ�ؽ�����С��
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  miao.jianxin

	*******************************************************************************/
	int TMdbCacheTable::BuildHeap(int iLen)
	{
		int index = iLen/2-1;  
		int i;
		for (i = index; i >= 0; i--)  
			MinHeap(i, iLen); 
		return 0;

	}

	/******************************************************************************
	* ��������	:  HeapSortForKNum
	* ��������	:  ����������ͨ���������ҳ�ǰK��Ԫ��
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  miao.jianxin

	*******************************************************************************/
	int TMdbCacheTable::HeapSortForKNum(int iTotalNum, int iKNum)
	{
		int i;
		TMdbCacheRow * pTempRow = NULL;
		BuildHeap(iTotalNum);
		
		if(iKNum>0 && iTotalNum>=1)
		{
			pTempRow = m_vData[iTotalNum-1];
			m_vData[iTotalNum-1] = m_vData[0];
			m_vData[0] = pTempRow;
		}
			
			
		
		for(i=1; i<iKNum; i++)
		{
			MinHeap(0,iTotalNum-i);

			pTempRow = m_vData[iTotalNum-i-1];
			m_vData[iTotalNum-i-1] = m_vData[0];
			m_vData[0] = pTempRow;
		}


		if(iKNum<iTotalNum/2)
		{
			for(i=0; i<iKNum; i++)
			{
				pTempRow = m_vData[iTotalNum-i-1];
				m_vData[iTotalNum-i-1] = m_vData[i];
				m_vData[i] = pTempRow;
			}
		}
		else
		{
			for(i=0; i<iTotalNum/2; i++)
			{
				pTempRow = m_vData[iTotalNum-i-1];
				m_vData[iTotalNum-i-1] = m_vData[i];
				m_vData[i] = pTempRow;
			}
		}
		return 0;
	}
	
	/******************************************************************************
	* ��������	:  QuickSort
	* ��������	: ��������,orderby
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbCacheTable::QuickSort(int iLeft,int iRight)
	{
		if(iLeft >= iRight){return 0;}
		int i,j;
		TMdbCacheRow * pTempRow = NULL; 

		for(i=iLeft; i<iRight; i++)
			if(CompareOrderby(m_vData[i],m_vData[i+1]) > 0 )
				break;
			
		if(i == iRight)
		{
			return 0;
		}
		
		for(i=iLeft; i<iRight; i++)
			if(CompareOrderby(m_vData[i],m_vData[i+1]) < 0 )
				break;	

		if(i == iRight)
		{
			for(int iPos = 0; iPos<(iRight-iLeft+1)/2;iPos++)
			{
				pTempRow = m_vData[iRight-iPos];
				m_vData[iRight-iPos] = m_vData[iLeft+iPos];
				m_vData[iLeft+iPos] = pTempRow;

			}
			return 0;
		}

		i = iLeft;j = iRight;
		pTempRow = m_vData[i];
		m_vData[i] = m_vData[(j+i)/2];
		m_vData[(j+i)/2] = pTempRow;
		pTempRow = m_vData[i];
		
		while(i<j)
		{
			while( i<j && (CompareOrderby(m_vData[j],pTempRow) > 0))
			{
				j--;
			}
			if(i < j) 
			{
				m_vData[i] = m_vData[j];
				i++;
			}
			while( i<j && (CompareOrderby(m_vData[i],pTempRow) < 0)) 
			{
				i++;
			}
			if(i < j)
			{
				m_vData[j] = m_vData[i];
				j--;
			}
		}
		
		m_vData[i] = pTempRow;
		if(iLeft < i-1)
			QuickSort(iLeft,i - 1);
		if(i+1 < iRight)
			QuickSort(i+1,iRight);
		return 0;
	}

	/******************************************************************************
	* ��������	:  CompareOrderbyForGroupBy
	* ��������	: group by �Ƚ�
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - ��ͬ!0 -��ͬ
	* ����		:  miao.jianxin
	*******************************************************************************/
	int TMdbCacheTable::CompareOrderbyForGroupBy(TMdbCacheRow * pLeft,TMdbCacheRow * pRight)
	{
		long long llRet = 0;
		unsigned int i = 0;
		for(i = 0; i < pLeft->m_vGroupBy.size();++i)
		{
			llRet = TMdbMemValue::CompareExprValue(pLeft->m_vGroupBy[i],pRight->m_vGroupBy[i]);
			if(0 != llRet)
			{
				break;
			}
		}
			
		if(0 == llRet){return 0;}
		if(0 > llRet){return -1;}
		if(0 < llRet){return 1;}
	}

	/******************************************************************************
	* ��������	:  CompareGroupBy
	* ��������	: group by �Ƚ�
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - ��ͬ!0 -��ͬ
	* ����		:  miao.jianxin
	*******************************************************************************/
	int TMdbCacheTable::CompareGroupBy(int iPos)
	{
		TMdbCacheRow  *pLeft = NULL, *pRight = NULL;
		pLeft = m_pCacheRowDef;
		CHECK_OBJ(pLeft);
		pRight = m_vData[iPos];
		CHECK_OBJ(pRight);
		return CompareOrderbyForGroupBy(pLeft,pRight);
		
	}

//}



