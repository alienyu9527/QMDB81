/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbTemporary.cpp		
*@Description： mdb 临时表机制
*@Author:			jin.shaohua
*@Date：	    2013.12
*@History:
******************************************************************************************/
#ifndef _MDB_CACHE_TABLE_H_
#define _MDB_CACHE_TABLE_H_

#include <vector>
#include "Helper/SqlParserStruct.h"
#include "Helper/mdbHashTable.h"

//namespace QuickMDB{
	class TMdbSqlParser;

	//临时列
	class TMdbCacheRow
	{
	public:
		TMdbCacheRow();
		~TMdbCacheRow();
		int Clean(bool bRef);//清理
		int CompareGroupBy(std::vector<ST_MEM_VALUE * > & vGroupBy);//比较group by值
		int SetUnused();//设置为未使用
		bool IsUnused(){return NULL == m_pDataAddr;}//是否未使用
	public:
		std::vector<ST_MEM_VALUE * > m_vGroupBy; //group by 列
		std::vector<ST_MEM_VALUE * > m_vOrderby;//order by列
		std::vector<ST_MEM_VALUE *>  m_vAggValue;//聚合函数列
		std::vector<ST_MEM_VALUE * > m_vHaving;//having 列
		char * m_pDataAddr;//数据地址
	};

	//临时表机制
	class TMdbCacheTable
	{
	public:
		TMdbCacheTable();
		~TMdbCacheTable();
		int Clear();//清理
		int Create(TMdbSqlParser * pSqlParser);//创建
		int Destroy();//销毁
		int GetAggValue(char * pDataAddr,TMdbCacheRow * & pCacheRow);//填充聚合值
		int SetAggValue(char * pDataAddr,TMdbCacheRow * & pCacheRow);//填充聚合值
		int Insert(TMdbCacheRow *  & pNewRow);//插入
		int UpdateAggValue();//更新聚合值
		bool bNeedCached();//是否需要缓存
		int   FinishInsert(ST_MEM_VALUE_LIST & stLimitMemValueList);//结束插入
		int   Open();//打开缓存表，游标指到头
		bool Next(char * & pDataAddr);//遍历缓存表
		int CompareGroupBy(int iPos);//group by比较
		int CreateHashTable();//创建用于group by的hash表
		int DestroyHashTable();//销毁用户group by的hash表

		
	private:
		TMdbCacheRow * GetRowByGroupValue(std::vector<ST_MEM_VALUE * > & vGroupBy);//根据Group by 获取列值
		int CopyMemValueList(std::vector<ST_MEM_VALUE * > & vDest,std::vector<ST_MEM_VALUE * > & vSource,bool bClearValue);//拷贝memlist;
		int QuickSort(long unsigned int iLeft,long unsigned int iRight);//快速排序
		int CompareOrderby(TMdbCacheRow * pLeft,TMdbCacheRow * pRight);//orderby 比较
		int MinHeap(long unsigned int iParent, long unsigned int iLen);//堆排序时调整堆成最小堆
		int BuildHeap(long unsigned int iLen);//堆排序时建堆
		int HeapSortForKNum(long unsigned int iTotalNum, long unsigned int iKNum);//order by和limit结合使用时,通过堆排序计算order by前K个记录
		int CompareOrderbyForGroupBy(TMdbCacheRow * pLeft,TMdbCacheRow * pRight);//group by比较

		int GetRowByHashTable(TMdbCacheRow * &pRetRow);//通过hash表获取group by字段相同的记录
		unsigned int  GetHashValue(std::vector<ST_MEM_VALUE*>  vGroup);//计算group by字段的hash
		
	private:
		TMdbCacheRow * m_pCacheRowDef;//缓存行定义
		std::vector<TMdbCacheRow * > m_vData;//数据
		long unsigned int  m_llNextPos;//next的位置
		std::vector<ST_EXPR *> * m_pvAggExpr;//聚合表达式地址
		long unsigned int m_iRowCached;//已缓存的记录数
		bool m_bSingleAgg;//只有一条聚合，优化处理
		ST_ORDERBY_INFO m_stOrderbyInfo;//排序信息
		HTAB* m_hashTable;//用来执行group by的hash table
	};

//}

#endif

