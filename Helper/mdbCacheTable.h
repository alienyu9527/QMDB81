/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbTemporary.cpp		
*@Description�� mdb ��ʱ�����
*@Author:			jin.shaohua
*@Date��	    2013.12
*@History:
******************************************************************************************/
#ifndef _MDB_CACHE_TABLE_H_
#define _MDB_CACHE_TABLE_H_

#include <vector>
#include "Helper/SqlParserStruct.h"
#include "Helper/mdbHashTable.h"

//namespace QuickMDB{
	class TMdbSqlParser;

	//��ʱ��
	class TMdbCacheRow
	{
	public:
		TMdbCacheRow();
		~TMdbCacheRow();
		int Clean(bool bRef);//����
		int CompareGroupBy(std::vector<ST_MEM_VALUE * > & vGroupBy);//�Ƚ�group byֵ
		int SetUnused();//����Ϊδʹ��
		bool IsUnused(){return NULL == m_pDataAddr;}//�Ƿ�δʹ��
	public:
		std::vector<ST_MEM_VALUE * > m_vGroupBy; //group by ��
		std::vector<ST_MEM_VALUE * > m_vOrderby;//order by��
		std::vector<ST_MEM_VALUE *>  m_vAggValue;//�ۺϺ�����
		std::vector<ST_MEM_VALUE * > m_vHaving;//having ��
		char * m_pDataAddr;//���ݵ�ַ
	};

	//��ʱ�����
	class TMdbCacheTable
	{
	public:
		TMdbCacheTable();
		~TMdbCacheTable();
		int Clear();//����
		int Create(TMdbSqlParser * pSqlParser);//����
		int Destroy();//����
		int GetAggValue(char * pDataAddr,TMdbCacheRow * & pCacheRow);//���ۺ�ֵ
		int SetAggValue(char * pDataAddr,TMdbCacheRow * & pCacheRow);//���ۺ�ֵ
		int Insert(TMdbCacheRow *  & pNewRow);//����
		int UpdateAggValue();//���¾ۺ�ֵ
		bool bNeedCached();//�Ƿ���Ҫ����
		int   FinishInsert(ST_MEM_VALUE_LIST & stLimitMemValueList);//��������
		int   Open();//�򿪻�����α�ָ��ͷ
		bool Next(char * & pDataAddr);//���������
		int CompareGroupBy(int iPos);//group by�Ƚ�
		int CreateHashTable();//��������group by��hash��
		int DestroyHashTable();//�����û�group by��hash��

		
	private:
		TMdbCacheRow * GetRowByGroupValue(std::vector<ST_MEM_VALUE * > & vGroupBy);//����Group by ��ȡ��ֵ
		int CopyMemValueList(std::vector<ST_MEM_VALUE * > & vDest,std::vector<ST_MEM_VALUE * > & vSource,bool bClearValue);//����memlist;
		int QuickSort(int iLeft,int iRight);//��������
		int CompareOrderby(TMdbCacheRow * pLeft,TMdbCacheRow * pRight);//orderby �Ƚ�
		int MinHeap(int iParent, int iLen);//������ʱ�����ѳ���С��
		int BuildHeap(int iLen);//������ʱ����
		int HeapSortForKNum(int iTotalNum, int iKNum);//order by��limit���ʹ��ʱ,ͨ�����������order byǰK����¼
		int CompareOrderbyForGroupBy(TMdbCacheRow * pLeft,TMdbCacheRow * pRight);//group by�Ƚ�

		int GetRowByHashTable(TMdbCacheRow * &pRetRow);//ͨ��hash���ȡgroup by�ֶ���ͬ�ļ�¼
		unsigned int  GetHashValue(std::vector<ST_MEM_VALUE*>  vGroup);//����group by�ֶε�hash
		
	private:
		TMdbCacheRow * m_pCacheRowDef;//�����ж���
		std::vector<TMdbCacheRow * > m_vData;//����
		long long m_llNextPos;//next��λ��
		std::vector<ST_EXPR *> * m_pvAggExpr;//�ۺϱ��ʽ��ַ
		unsigned int m_iRowCached;//�ѻ���ļ�¼��
		bool m_bSingleAgg;//ֻ��һ���ۺϣ��Ż�����
		ST_ORDERBY_INFO m_stOrderbyInfo;//������Ϣ
		HTAB* m_hashTable;//����ִ��group by��hash table
	};

//}

#endif

