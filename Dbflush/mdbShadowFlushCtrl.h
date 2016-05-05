/****************************************************************************************
*@Copyrights  2011，中兴软创（南京）计算机有限公司 开发部 CCB项目--QMDB团队
*@                   All rights reserved.
*@Name：        mdbShadowFlushCtrl.h
*@Description： 基于影子表模式的ORCLE2MDB同步
*@Author:       li.ming
*@Date：        2013年12月20日
*@History:
******************************************************************************************/
#ifndef _MDB_SHADOW_FLUSH_CTRL_H_
#define _MDB_SHADOW_FLUSH_CTRL_H_

#include "Dbflush/mdbDAOBase.h"
#include "Dbflush/mdbShaowServerCtrl.h"

//namespace QuickMDB{



#define MAX_KEY_LEN   128
#define MAX_REP_PREFETCH_ROWS 10000

    class TShadowRecord
    {
    public:
        TShadowRecord();
        ~TShadowRecord();

        void Clear();
        void AddData( TMDBDBFieldInterface& tField,int iLen, const MDB_INT64& iSeq = -1);
        void AddDelData( TMDBDBFieldInterface& tField,int iLen);

    public:
        char m_sTabName[MAX_NAME_LEN];
        TMdbData m_tDelData;
        TMdbData m_aData[MAX_COLUMN_COUNTS];
        int m_iColmCnt;   
        
    };


    class TTableDAO
    {
    public:
        TTableDAO();
        ~TTableDAO();
        
        void Clear();
       
        char m_sTabName[MAX_NAME_LEN];
        TMdbDAOBase* m_pInsertDAO; 
        TMdbDAOBase* m_pDelDAO;    
    };


    class TShadowDAO
    {
    public:
        TShadowDAO();
        ~TShadowDAO();
        
        int Init(const char* psDsn,const char* psUid, const char* psPwd, const char* psShadowName,const char* psNotifyTabName);
        int Execute( TShadowRecord& tOneRecord);
        int Commit();

        void Clear();
        
        void ClearDAOByTableName(const char* psTabName);

    private:   
        int CreateDAO(const char* psTabName);
        int FindDAO(const char* psTabName);
        
        void GenInsertSql(const char* psTabName, char*  psSql, int isqllen);
        void GenDeleteSql(const char* psTabName, char*  psSql, int isqllen);

    private:
        char m_sDSN[MAX_NAME_LEN]; //DSN
        char m_sUID[MAX_NAME_LEN]; //UID
        char m_sPWD[MAX_NAME_LEN]; //PWD
       
        TMDBDBInterface* m_pDBLink;   //链接
        TMDBDBQueryInterface *m_pQuery;       //单次提交
        TShadowRecord* m_pCurRecd; 

        char m_sShadowTabName[MAX_NAME_LEN];
        char m_sNotifyTabName[MAX_NAME_LEN];

        int m_iCurDaoPos; 
        TTableDAO m_vTabDAO[MAX_TABLE_COUNTS]; 
    };


    class TTableNotifyInfo
    {
    public:
        TTableNotifyInfo();
        ~TTableNotifyInfo();

        int Init(const char* psNotifyName  /*%dsn%_mdb_change_notif or mdb_change_notif*/
                     ,const char* psNotifySeqName  /*%dsn%_mdb_change_notify_seq or mdb_change_notify_seq*/
                     ,const char* psShadowTabName
                     ,const char* psTableName
                     ,TMDBDBQueryInterface* pOraQry);

    private:    
        
        MDB_INT64 GetLastSeq(const char* psNotifyName  /*%dsn%_mdb_change_notif or mdb_change_notif*/
                                        ,const char* psNotifySeqName /*%dsn%_mdb_change_notify_seq or mdb_change_notify_seq*/
                                        ,const char* psShadowTabName
                                        ,const char* psTableName
                                        ,TMDBDBQueryInterface* pOraQry);

        MDB_INT64 GetMaxSeqFromShadowTab(const char* psTableName,const char* psShadowTabName,TMDBDBQueryInterface* pOraQry);
        
        MDB_INT64 GetSeqFromNotifySeqTab(const char* psTableName, const char* psNotifySeqName,TMDBDBQueryInterface* pOraQry);
        
        MDB_INT64 GetMinSeqFromNotifyTab(const char* psTableName,const char* psNotifyName,TMDBDBQueryInterface* pOraQry);

    public:
        char m_sTabName[MAX_NAME_LEN];
        MDB_INT64 m_iLastSeq;
        char m_sSql[MAX_SQL_LEN];
    };


    class TShadowFlushCtrl
    {
    public:
        
        TShadowFlushCtrl();
        ~TShadowFlushCtrl();

        // 初始化
        int Init(const char* psDsnName);

        /******************************************************************************
        * 函数名称	:  Run()
        * 函数描述	:  将%DSN%_MDB_CHANGE_NOTIF中记录移入影子表QMDB_SHADOW_%DSN%
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0, 失败返回-1;
        * 作者		:  li.ming
        *******************************************************************************/
        int Run();

    private:

        //具体 处理每表
        int DealTable( TTableNotifyInfo* pTable);

        // 获取当前change_notif表中记录数
        MDB_INT64 GetChangeNotifyCnt();
        
        int InitDao();

        // 连接数据库
        int ConnectOracle();

        // 获取表的刷新信息
        TTableNotifyInfo* GetTableNotifyInfo(const char* psTabName);

        // 获取change_notif表名称(%dsn%_mdb_change_notif or mdb_change_notif)
        int GetNotifyTabName(const char* sMdbDsn);    

        // 新增需要刷新的表的信息
        int AddNewNotfyTab(const char* psTabName);

        // 提交DAO
        int DaoCommit(TTableNotifyInfo* pTableInfo, const MDB_INT64 iLastSeq);
        
    private:
        std::vector<TTableNotifyInfo*> m_vpTabNotifyInfo; // 各个ORACLE2MDB表获取 %DSN%_MDB_CHANGE_NOTIF中记录SQL以及影子表起点seq信息
        TMDBDBInterface* m_pOraLink; // oracle 连接
        TMDBDBQueryInterface* m_pOraQry; // oracle 操作句柄
        TMDBDBQueryInterface* m_pSelectQry; // oracle 操作句柄

        TShadowDAO m_tDao; // DAO方式将记录插入影子表

        TTableNotifyInfo* m_pCurTabInfo; // 当前刷新表的刷新信息

        char m_sNotifyTabName[MAX_NAME_LEN]; //  change_notify 表名称(%dsn%_mdb_change_notif or mdb_change_notif)
        char m_sNotifySeqTabName[MAX_NAME_LEN]; //  change_notify_seq 表名称(%dsn%_mdb_change_notify_seq or mdb_change_notify_seq)
        char m_sShadowTabName[MAX_NAME_LEN];  // 影子表名称(QMDB_SHADOW_%DSN%)

        TMdbShadowSvrCtrl m_tSvrCtrl; 
        TShadowCfgNode m_tCfgInfo;
        TShmMonitorInfo* m_pSvrMgr;
    };


//}
#endif
