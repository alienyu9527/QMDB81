#ifndef _MDB_EXECUTE_ENGINE_H_
#define _MDB_EXECUTE_ENGINE_H_
#include "Helper/mdbSQLParser.h"
/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbExecuteEngine.h
*@Description： SQL执行引擎
*@Author:	     jin.shaohua
*@Date：	    2012.05
*@History:
******************************************************************************************/
#include "Control/mdbTableWalker.h"
#include "Control/mdbPageCtrl.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Control/mdbLimitCtrl.h"
#include "Control/mdbFlush.h"
#include "Helper/mdbErrorHelper.h"
#include "Control/mdbObserveCtrl.h"
#include "Control/mdbVarcharMgr.h"
#include "Control/mdbRowCtrl.h"
#include "Helper/mdbCacheTable.h"
#include "Interface/mdbQuery.h"

//namespace QuickMDB{

    class TMdbRollback;
    //mdb 执行引擎
    class TMdbExecuteEngine
    {
    public:
        TMdbExecuteEngine();
        ~TMdbExecuteEngine();
        int Init(TMdbSqlParser * pMdbSqlParser, MDB_INT32 iFlag, TMdbLocalLink* pLocalLink);//初始化工作
        int Execute();//执行
        int Next(bool & bResult);//获取下一条
        int FillCollist();//填充列值
        int GetRowsAffected()
        {
            return m_iRowsAffected;    //获取影响的记录数
        }
        int SetRollbackBlock(TMdbRollback * pRollback,int iRBUnitPos);//设置回滚段
        int GetOneRowData(void *pStruct,int* Column);//获取一列数据信息
		int GetOneRowData(TMdbColumnAddr* pTColumnAddr);
        long long GetRowTimeStamp();
        int CheckRowDataStruct(int* Column);//校验结构长度信息
        int ReBuildTableFromPage(const char * sDSN,TMdbTable * pMdbTable);//从内存页重新构建表
        void ClearLastExecute();
        protected:
        int ExecuteInsert();//执行插入
        int ExecuteUpdate();//执行更新
        int ExecuteDelete();//执行删除
        int FillSqlParserValue(ST_MEM_VALUE_LIST & stMemValueList);//向解析器填充值
        int CheckWhere(bool &bResult);//检测where条件是否满足
		int CheckDiskFree();
        
        int InsertDataFill(char* const &sTmp);//填充要插入的数据
        int InsertData(char* pAddr, int iSize);//将数据插入到内存中
        int UpdateData();//将数据更新到
        int ChangeInsertIndex( char* pAddr, TMdbRowID& rowID);
        int ChangeUpdateIndex(std::vector<ST_INDEX_VALUE > & vUpdateIndex);//对update 操作所造成的索引变更
        int GetNextIndex(ST_TABLE_INDEX_INFO * & pTableIndex,long long & llValue);//获取下一个索引
		int SetTrieWord();
		//int GetNextIndex(ST_TABLE_INDEX_INFO * & pTableIndex);//获取下一个索引
        //long long CalcMPIndexValue( char* pAddr, int iIndexPos, int& iError);
        //long long CalcOneIndexValue( char* pAddr, int iIndexPos, int& iError);
        //long long CalcIndexValue( char* pAddr, int iIndexPos, int& iError);
        int CalcMemValueHash(ST_INDEX_VALUE & stIndexValue,long long & llValue);
        int DeleteVarCharValue( char*  const &pAddr);//删除变长数据
        int ClearMemValue(ST_MEM_VALUE_LIST & stMemValueList);//清理数据
        int ExecuteCacheSelect();//执行缓存查询
        bool CacheNext();//缓存next
        //bool NormalNext();
        inline bool  IsNeedReadLock();//是否需要加读锁
        int NextWhere(bool & bResult);//符合where条件        
		int NextWhereByPage(bool & bResult);
		int NextWhereByIndex(bool & bResult);
		
        int PushRollbackData(const char *pDataAddr,const char * pExtraDataAddr);//搜集需要回滚的数据
        int IsPKExist();//检测主键是否已存在
        bool IsDataPosBefore();//该记录是否被定位过
        inline int * GetRowIndexArray(char * pDataAddr);//获取rowindex数组
        int GetUpdateDiff();//获取增量更新值
        int SetRowDataTimeStamp(char* pAddr, int iOffset,long long iTimeStamp = 0);
        int UpdateRowDataTimeStamp(char* const & pAddr, int iOffset, long long iTimeStamp = 0);
    private:

        TMdbSqlParser * m_pMdbSqlParser;//语法树结构
        int m_iCurIndex;//正在使用第几个索引
        std::vector<ST_INDEX_VALUE > * m_pVIndex;
        TMdbTableWalker m_MdbTableWalker;//mdb 表的遍历类
        char * m_pDataAddr;//数据地址
        TMdbRowID m_tCurRowIDData ;
        char* m_pPageAddr;    //数据所在页面的地址
        int   m_iPagePos;     //数据在页面中的位置

        bool m_bScanAll;//全量遍历
        long long m_llScanAllPos;//记录全量遍历的上次位置
        TMdbTable * m_pTable;//操作的某个表
        TMdbDSN   * m_pDsn;


        TMdbPageCtrl   m_mdbPageCtrl;              //页控制信息
        TMdbTableSpaceCtrl m_mdbTSCtrl;            //表空间控制信息
        int m_iRowsAffected;						//影响的记录数
        int m_iIsStop;    //中途停止
        TMdbIndexCtrl   m_mdbIndexCtrl;//索引管理
        int    m_iMoniNext; //模拟next
        int    m_iNextType;//next的类型
        TMDBLimitCtrl  m_tLimitCtrl;//limit
        char * m_pInsertBlock;//插入块
        char * m_pVarCharBlock;//变长char 块用来做索引计算用

        TMdbRollback * m_pRollback;//回滚段
        int            m_iRBUnitPos;//回滚unit
        TMdbFlush    m_tMdbFlush;//mdb 刷新模块

        char     m_sTempValue[MAX_BLOB_LEN];//存放临时字符串，为了性能
        TObserveTableExec m_tObserveTableExec;//监控表操作
        //TMdbVarcharMgr m_tVarcharMgr;
        TMdbVarCharCtrl m_tVarcharCtrl;
        char * m_pUpdateBlock;//更新块
        TMdbRowCtrl m_tRowCtrl;//记录控制
        int* m_aRowIndexPos; 
        TMdbCacheTable m_tCacheTable;//缓存表
        TMdbLocalLink* m_pLocalLink;
    public:
        TMdbErrorHelper m_tError;
    };
//}



#endif
