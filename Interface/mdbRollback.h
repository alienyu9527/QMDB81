/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbRollback.h
*@Description： 负责管理QuickMDB的回滚控制接口
*@Author:		li.shugang
*@Date：	    2012年02月6日
*@History:
******************************************************************************************/
#ifndef __QUICK_MEMORY_DATABASE_ROLLBACK_H__
#define __QUICK_MEMORY_DATABASE_ROLLBACK_H__

#include <map>
#include <vector>
//#include "BillingSDK.h"
#include "Control/mdbVarcharMgr.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{
    
	#define MAX_ROLLBACK_COUNTS 10000

	class TMdbQuery;
	class TMdbDatabase;
	class TMdbSqlParser;
	class TMdbColumn;
	class TMdbVarcharMgr;
	class TMdbRowCtrl;
	
	//回滚单元
	class TRBUnit
	{
	public:
		TRBUnit();
		~TRBUnit();
	public:
		TMdbQuery* pQuery;
		char * sSQL;
	};

	//回滚段
	class TMdbRollback
	{
	public:
		TMdbRollback();
		~TMdbRollback();
		void SetDB(TMdbDatabase* pDB,char* sDsn);
		/******************************************************************************
		* 函数名称	:  CreateQuery()
		* 函数描述	:  根据SQL获取对应的Query
		* 输入		:  pMdbTable, 表结构指针
		* 输入		:  pszSQL, 原始SQL
		* 输出		:  无
		* 返回值	:  成功返回Query的位置, 失败返回-1
		* 作者		:  li.shugang
		*******************************************************************************/
		int CreateQuery(const char * sSql,TMdbSqlParser * pMdbSqlParser);
		/******************************************************************************
		* 函数名称	:  PushData()
		* 函数描述	:  根据Pos获取对应的SQL操作
		* 输入		:  iPos, Query的位置，0---999
		* 输入		:  pData, 实际记录的地址
		* 输入		:  iCounts, 已经执行的记录数
		* 输出		:  无
		* 返回值	:  成功返回Query的句柄, 失败返回NULL
		* 作者		:  li.shugang
		*******************************************************************************/
		int PushData(int iPos, const char* pData, int iCounts,const char * pExtraDataAddr,TMdbRowCtrl * pRowCtrl);
		/******************************************************************************
		* 函数名称	:  Commit()
		* 函数描述	:  提交所有的SQL操作
		* 输入		:  无
		* 输出		:  无
		* 返回值	:  成功返回0, 失败返回负数
		* 作者		:  li.shugang
		*******************************************************************************/
		int Commit();
		int CommitEx();
	#if 0
		/******************************************************************************
		* 函数名称	:  Rollback()
		* 函数描述	:  回滚所有的SQL操作
		* 输入		:  无
		* 输出		:  无
		* 返回值	:  成功返回0, 失败返回负数
		* 作者		:  li.shugang
		*******************************************************************************/
		int Rollback(bool bOneSql);
	#endif
		int RowsAffected();
		int SetCloseRBUnit(int iPos);
		int RollbackAll();//回滚所有
		int RollbackOneArrayExecute();//回滚一个批量执行
		int RollbackOneExecute();//回滚一个执行
		bool bIsRollbackEmpty(){return m_iOffSet < 0 || m_pszRollback == NULL || m_pBuffer == NULL;}//回滚段是否为空
		int SetArraryExecuteStart();//设置批量执行开始
	private:
		int GetFreePos();//获取一个空闲的回滚单元
		//从内存中获取数据，并写入回滚buffer
    	int GetDataFromAddr(TMdbColumn * pColumn,const char* pData,const char * pExtraDataAddr,TMdbRowCtrl * pRowCtrl, TMdbSqlParser * pMdbSqlParser);
		int ReAlloc();//根据当前回滚段，重新分配回滚段的内存，如果失败，返回-1
		int PushDataIntoRB();//把buffer数据写入回滚段中
		int RollbackOneRecord();//只回滚最后一条
	private:
		TMdbDatabase* m_pDB;
		TRBUnit* m_ptRBUnit[MAX_ROLLBACK_COUNTS];   //一个个的回滚SQL信息
		char  m_sSQL[4096];                         //临时SQL
		char* m_pBuffer;                            //存放临时的一个回滚数据
		char* m_pszRollback;                        //回滚段的指针
		int   m_iSize;                              //回滚段的大小，单位为M
		int   m_iOffSet;                            //回滚数据的偏移量，是一个栈，而非队列
		int   m_iRowsAffected;                      //提交、回滚影响的记录数
		std::vector<int>  m_vCloseRBUnit;			//已关闭的QUERY所对应的RBUnit
		TMdbVarCharCtrl m_pVarcharCtrl;
		//变长存储区管理
		//TMdbVarcharMgr * m_pVarcharMgr;//varchar管理
		TMdbNtcSplit * m_pRBValueSplit;//回滚数据分隔器
		//TMdbSplit * m_pRBValueSplit;//回滚数据分隔器
	};
//}


#endif //__QUICK_MEMORY_DATABASE_ROLLBACK_H__

