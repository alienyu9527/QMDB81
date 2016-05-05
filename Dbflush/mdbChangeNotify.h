/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbChangeNotify.h		
*@Description： 内存数据库的Oracle同步管理控制
*@Author:		li.shugang
*@Date：	    2009年5月07日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_CHANGE_NOTIFY_H__
#define __MINI_DATABASE_CHANGE_NOTIFY_H__

#include "Helper/mdbConfig.h"
#include "Helper/TDBFactory.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbMgrShm.h"
#include "Control/mdbProcCtrl.h"
#include <algorithm>
#include <string.h>
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

using namespace std;

//namespace QuickMDB{

    // 已刷新数组中存放的最大节点数
    #define MAX_FLUSH_ARRAY_CNT      10000

    struct LSNODE
    {
        MDB_INT64  iSeq;//漏刷sequence
        char sTabName[MAX_NAME_LEN];
        char sTime[MAX_TIME_LEN];//时间
    };

    // 已处理的刷新节点
    struct SNODE
    {
        bool bFlush; // 是否已被刷新
        MDB_INT64 lSeq; // table_sequence
    };

    class CNQueryList
    {
    public:
        CNQueryList();
        ~CNQueryList();
        int Init(TMdbDatabase *pMdbLink,TMDBDBInterface *pOraLink,TMdbTable* pTable,TMdbConfig  *pConfig,char* dsn_name,char* mdbChangeNotifyName,char* mdbChangeNotifySeqName);
        int InitMdbQuery();
        int InitOraSQL();
        TMdbDatabase *m_pMdbLink;
        TMDBDBInterface *m_pOraLink;
        TMdbConfig  *m_pConfig;
        TMdbQuery *m_pQueryQ;  //QMDB查询句柄
        TMdbQuery *m_pQueryU;  //QMDB更新句柄
        TMdbQuery *m_pQueryI;  //QMDB插入句柄
        TMdbQuery *m_pQueryD;  //QMDB删除句柄	
        TMdbTable* m_pTable;
        char m_sUpperDsn[MAX_NAME_LEN];
        char m_sOraSelectSQL[MAX_SQL_LEN];
        char m_sOraDeleteSQL[MAX_SQL_LEN];
        char m_sGetSeqFromNotifySeqSQL[MAX_SQL_LEN];
        char m_sGetSeqFromNotifySQL[MAX_SQL_LEN];
        char m_sInsertNotifySeqSQL[MAX_SQL_LEN];
        char m_sUpdateNotifySeqSQL[MAX_SQL_LEN];
        char m_sSQL[MAX_SQL_LEN];
        char m_sChangeNotifyName[MAX_NAME_LEN];
        char m_sChangeNotifySeqName[MAX_NAME_LEN];
        int m_iFlushCount;
        char m_sShadowName[MAX_NAME_LEN];

    public:
        //获取查询句柄
        int GetQuerySQL();

        //获取更新句柄
        int GetUpdateSQL();

        //获取插入句柄
        int GetInsertSQL();

        //获取删除句柄
        int GetDeleteSQL(TMDBDBQueryInterface* ptOraQry); 	

        //获取OracleSQL属性
        int GetOraSQL();
        //获取oracle delete sql
        int GetOraDSQL();

        int GetSeqFromNotifySeqSQL();
        int GetSeqFromNotifySQL();
        int GetInsertNotifySeqSQL();
        int GetUpdateNotifySeqSQL();
        bool isKey(int iColumnNo);
    };


    class TMdbChangeNotify
    {
    public:
        TMdbChangeNotify();
        ~TMdbChangeNotify();

        /******************************************************************************
        * 函数名称	:  Init()
        * 函数描述	:  初始化：链接Oracle  
        * 输入		:  pszDSN, 锁管理区所属的DSN  
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回负数
        * 作者		:  li.shugang
        *******************************************************************************/
        int Init(const char* pszDSN, const char* pszName);

        /******************************************************************************
        * 函数名称	:  Start()
        * 函数描述	:  同步Oracle数据
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回负数
        * 作者		:  li.shugang
        *******************************************************************************/
        int Start();

    private:
        int FlushTable(TMdbTable* pTable);
        int FlushRecord(MDB_INT64 &iStartSeq,MDB_INT64 &iEndSeq,MDB_INT64 &iUpdateSequence,char *sUpdateTime); //刷新非Delete操作语句
        int FlushDRecord(MDB_INT64 &iStartSeq,MDB_INT64 &iEndSeq,MDB_INT64 &iUpdateSequence,char *sUpdateTime); //刷新非Delete操作语句
        int UpdateNotifySeqTable(MDB_INT64 iUpdateSequence,char *sUpdateTime);
        int GetStartEndSeq(MDB_INT64 &iStartSeq, MDB_INT64 &iEndSeq);
        bool FiltTable(TMdbTable* pTable);
        int ClearAllTableQuery();
        int InitTable();
        //把漏掉的数据插入"漏刷链表"
        void InsertDelayList(MDB_INT64 iStart, MDB_INT64 iEnd);

        //处理"漏刷链表"中的数据
        int DealDelayData();

        //删除数据
        int DeleteData();

        //插入数据
        int InsertData();

        //更新数据
        int UpdateData();

        //查询数据,如果有数据返回1, 否则返回0, 失败返回<0
        int QueryData();		

        //创建Oracle连接
        int ConnectOracle();

        bool isKey(int iColumnNo);
        int GetAdministrator();		

        /******************************************************************************
        * 函数名称	:  InitOraQry()
        * 函数描述	:  初始化 %dsn%_mdb_change_notif表查询句柄
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回负数
        * 作者		:  li.ming
        *******************************************************************************/
        int InitOraQry();

        /******************************************************************************
        * 函数名称	:  NeedToFlush()
        * 函数描述	:  查询 %dsn%_mdb_change_notif表,判断是否有需要待刷新的数据
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  需要遍历刷新返回true，否则返回false
        * 作者		:  li.ming
        *******************************************************************************/
        bool NeedToFlush();

        int InitFlushList(MDB_INT64 lStartSeq, MDB_INT64 lEndSeq);
        int AddToFlushList(MDB_INT64 lStartSeq, MDB_INT64 lSeq);
        void ClearFlushList();
        int SetSqlName();

        CNQueryList* GetTabQuery(const char* psTabName);

        MDB_INT64 GetSmartStartSeq(MDB_INT64 lStartSeq);

        void PrintFlushLog();
        void LogFlushInfo(std::string&  sLogInfo);
        void LogRecdNotifyInfo(const MDB_INT64 iStartSeq, const MDB_INT64 iEndSeq, const MDB_INT64 iCurSeq, const char cActionType, const char* psTabName, std::string & sRecdStr);
        void LogRecdPkInfo(std::string & sRecdStr, const char* psPkName, const char* psPkValue);
        
    private: 
        TMDBDBInterface *m_pDBLink;   //Oracle链接
        TMdbDatabase *m_mdbLink;     //QMDB链接
        TMdbConfig  *m_pConfig;
        TMdbProcCtrl m_tProcCtrl;//进程控制
        TMdbShmDSN  *m_pShmDSN;
        TMdbDSN     *m_pDsn;     
        CNQueryList*  m_tQueryList[MAX_TABLE_COUNTS];
        TMdbTable* m_tCurTable;
        CNQueryList* m_tCurQuery;
        TMDBDBQueryInterface *m_pOraQry; // %dsn%_mdb_change_notif表查询句柄，获取是否有需要待刷新的数据
        int m_iTotalFlushCount;
        int m_iTimeInterval; // 刷新时间间隔；
        int m_iDelayTime; // 补刷节点的超时时间；
        char m_sOpenStartTime[MAX_TIME_LEN]; //每次取刷新数据open开始时间；
        char m_sOpenEndTime[MAX_TIME_LEN]; // 每次取刷新数据open结束时间；
        char m_sQrySql[MAX_SQL_LEN]; // 获取是否有需要刷新数据的sql
        std::vector<LSNODE> m_vLsList;//漏刷链表

        bool m_bAlloc; // 是否有重新申请已刷新节点链表标志
        MDB_INT64 m_lFlushCnt; // 需要刷新的节点数
        MDB_INT64 m_lStartSeq; // 开始table_sequence
        MDB_INT64 m_lEndSeq;// 结束table_sequence
        char* m_pFlushList; // 需要刷新的记录超过1w时，已刷新节点存放在此链表中
        char m_cFlushList[MAX_FLUSH_ARRAY_CNT]; // 需要刷新的记录少于1w时，已刷新节点存放在此链表中
        char m_sChangeNotifyName[MAX_NAME_LEN];
        char m_sChangeNotifySeqName[MAX_NAME_LEN];
        char m_sShadowName[MAX_NAME_LEN];

        std::string m_sFlushLogStr; // 记录刷新的记录信息
        std::string m_sRecdLogStr; // 临时记录一条记录的刷新信息
    	bool m_bNeedFlush; //是否需要刷新
    };


//}
#endif //__MINI_DATABASE_CHANGE_NOTIFY_H__


