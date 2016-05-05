/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbFlushDao.h		
*@Description: 将同步数据刷新到MDB中
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_FLUSH_DAO_H__
#define __ZTE_MINI_DATABASE_REP_FLUSH_DAO_H__

#include "Control/mdbRepCommon.h"
#include "Interface/mdbQuery.h"
#include "Helper/mdbRepRecd.h"
//#include "Interface/mdbRollback.h"
//namespace QuickMDB
//{
    #define MAX_FLUSN_DAO_COUNTS 30
    
    /**
    * @brief 节点
    * 
    */
    class TFlushDAONode
    {
    public:
        TFlushDAONode();
        ~TFlushDAONode();
        void Clear();
        TMdbQuery* CreateQuery(const char * sSQL,int iNodePos,TMdbDatabase* ptDB);
    public:
        char m_sSQL[MAX_SQL_LEN];
        TMdbQuery * m_pQuery;//执行sql

    };
    
    /**
    * @brief 每张表的sql和query指针信息
    * 
    */
    class TRepFlushDao:public TMdbNtcBaseObject
    {
    public:
        TRepFlushDao();
        ~TRepFlushDao();
        int Init(std::string strTableName, TMdbDatabase* pDataBase)throw (TMdbException);//初始化
        int GetQuery(TMdbQuery * &pQuery, TMdbRepRecd & tRecd, int * iDropIndex)throw (TMdbException);//创建query
        TMdbQuery * GetSelectQuery(TMdbRepRecd & tRecd)throw (TMdbException);
        TMdbQuery * GetLoadQuery(const char* sRoutinglist);
        TMdbQuery* GetRecdQuery()throw (TMdbException);
        
    private:
        int SetSQL(TFlushDAONode *pNode, int iSqlType)throw (TMdbException);
        int GetSQL(TMdbRepRecd& tRecd, int iSqlType, int * iDropIndex = NULL);
        int InitTabProperty()throw (TMdbException);
        int GetColumnDataType(const char* psDataType);
        TMdbColumn* GetColumnInfo(const char* psName);
        void GetWhereSql(TMdbRepRecd& tRecd, char* psWhere);
    public:
        char m_sSQL[MAX_SQL_LEN];
        TMdbDatabase *m_pDataBase;
        TFlushDAONode * m_arrDaoNode[MAX_FLUSN_DAO_COUNTS];
        TMdbQuery *  m_pRecdQry;
        std::string m_strTableName;//表名
        std::vector<std::string> m_arPKs;//主键集合
        std::vector<std::string> m_arCols;//非主键列集合
        std::vector<TMdbColumn> m_vColms;
    };
    /**
    * @brief 数据加载，取数据和插入数据控制
    * 
    */
    class TRepLoadDao
    {
    public:
        TRepLoadDao();
        ~TRepLoadDao();
    public:
        int Init(TMdbDatabase *pDataBase,TMdbConfig* pMdbCfg);
        TMdbQuery* GetQuery(std::string strTableName, bool bSelect, const char* sRoutinglist = NULL)throw (TMdbException);
    private: 
        int SetSelectSQL(TFlushDAONode *pNode, const char* sRoutinglist=NULL)throw (TMdbException);
        int SetInsertSQL(TFlushDAONode *pNode)throw (TMdbException);
    public:
        TFlushDAONode *m_pNode;
        TMdbDatabase *m_pDataBase;
        std::string m_strTableName;//表名
        std::vector<std::string> m_arCols;//非主键列集合
        
        TMdbConfig* m_pMdbCfg;
    };

    /**
    * @brief 刷新控制
    * 
    */
    class TRepFlushDAOCtrl
    {
    public:
        TRepFlushDAOCtrl();
        ~TRepFlushDAOCtrl();
        int Init(const char * sDsn);
        int Execute(const char * sMsg, bool bCheckTime);//执行
    private:
        int SetQuery(int * iDropIndex)throw (TMdbException);//设置query
        int SetParam(int * iDropIndex)throw (TMdbException);//设置参数
        int SetPkParam() throw (TMdbException);
        int QueryRecd();
        bool CheckTimeStamp(long long iMdbTime, long long iRecdTIme);
        TRepColm* GetPkColm(const char* psColumName);
    private:
        TMdbNtcStrMap m_tMapFlushDao;
        TMdbDatabase* m_ptDatabase;
        TMdbRep12Decode* m_pRcd12Parser;
        TMdbRepRecdDecode * m_pRcdParser;//数据解析器
        TMdbRepRecd m_tCurRedoRecd;//当前处理的记录
        TMdbQuery  * m_pCurQuery;//执行的query;
        TRepFlushDao *m_pCurFlushDao;//当前的DAO
        long long m_iTimeStmp;
        bool m_bExist;
        TMdbConfig* m_pConfig;
    };

//}

#endif
