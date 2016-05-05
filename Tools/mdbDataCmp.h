/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbDataCmp.h     
*@Description：比较任意一台（或者多台）对端主机与本机对比数据是否相同
*@Author:       jiang.lili
*@Date：        2014年4月
*@History:
******************************************************************************************/
#ifndef __QUICK_MEMORY_DATABASE_DATA_CMP_H__
#define __QUICK_MEMORY_DATABASE_DATA_CMP_H__

#include "Helper/mdbThreadBase.h"
#include "Helper/mdbConfig.h"
#include "Helper/TDBFactory.h"
#include "Interface/mdbQuery.h"
#include "Interface/mdbClientQuery.h"
#include "Control/mdbSysTableThread.h"
#include "Helper/mdbXML.h"



//namespace QuickMDB{


#define  CATCH_MDB_DATA_CHECK_EXEPTION     catch(TMdbException &e)\
    {\
    CHECK_RET(-1,"MdbException. \nERROR_MSG=%s\n ERROR_SQL=%s\n", e.GetErrMsg(), e.GetErrSql());\
    }\
    catch(TMDBDBExcpInterface &e)\
    {\
    CHECK_RET(-1,"OracleException. \nERROR_CODE=%d\nERROR_MSG=%s\n ERROR_SQL=%s\n", e.GetErrCode(), e.GetErrMsg(), e.GetErrSql());\
    }\
    catch(...)\
    {\
    CHECK_RET(-1,"Unknown exception. ");\
    }

#define  MDB_DATA_CHECK_MAX_POS_LEN 16
#define  MDB_DATA_CHECK_INT_BUF 32
//比较数据不同的类型
#define  MDB_DIFF_DATA_SAME '0' //数据相同
#define  MDB_DIFF_DATA_NOT_SAME '1' //数据不同
#define  MDB_DIFF_LOCAL_NOT_EXIST '2' //对端有，本地没有
#define  MDB_DIFF_PEER_NOT_EXIST '3' //本地有，对端没有
#define  MDB_DIFF_BOTH_NOT_EXIST '4' //重复检查时，本地和对端都没有

    //待比较的DSN类型
    enum MDB_CHECK_DSN_TYPE
    {
        E_DSN_UNKOWN = 0,
        E_DSN_MDB, //mdb
        E_DSN_ORACLE//oracle
    };

    //线程状态
    enum MDB_CHECK_THREAD_STATE
    {
        E_CHECK_THREAD_FREE,
        E_CHECK_THREAD_BUSY,
        E_CHECK_THREAD_OVER,
        E_CHECK_THREAD_ERROR
    };

    //通过stream上载的表
    struct TMdbCheckQdgTable
    {
        std::string m_strTableName;
        std::string m_strLoadSql;
        void Clear(){m_strLoadSql.clear(); m_strTableName.clear();}
    };

    //待比较的DSN
    class TMdbDataCheckDSN
    {
    public:
        void Clear();
    public:
        MDB_CHECK_DSN_TYPE m_eType;//数据库类型
        std::string m_strDsn;//dsn名称
        std::string m_strUser;//用户名
        std::string m_strPassword;//密码
        std::string m_strIP;//IP
        int m_iPort;//端口号
        bool m_bAll;//是否比较所有数据
        std::vector<std::string> m_vTables;//待比较的表列表
        std::vector<TMdbCheckQdgTable> m_vQdgTable;//qdg表列表
        std::string m_strFilterSql; //匹配的where条件
    };

    //待处理的表信息
    class TMdbDataCheckTable
    {
    public:
        void Clear();
    public:
        int m_iTableID;//表ID
        std::string m_strTableName;//表名
        int m_iRecordNum;//记录数
        int m_iPkLen;//主键长度
        bool m_bDealed;//是否已处理
        bool m_bIsQdgTable;//是否是Qdg表
        std::string m_strLoadSql;//qdg表的load sql
    };

    //比较线程状态
    class TMdbCheckThreadState
    {
    public:
        int m_iThreadNo;//线程号
        MDB_CHECK_THREAD_STATE m_eState; //线程当前状态
        int m_iCurTableID;//当前处理的表ID
        int m_iCurRcdNum;//当前表已处理的记录数
    };

    //比较的基本信息
    class TMdbDataCheckInfo
    {
    public:
        void Clear();
    public:
        std::string m_strDsn;//本地DSN名称
        int m_iCmpTimes; //对不同数据的比较次数
        int m_iThreadNum; //线程数量
        int m_iLogLevel; //日志级别
        int m_iInterval;//比较间隔
        std::string m_strPath;//比较结果文件存储目录
    };


    //配置文件
    class TMdbDataCheckConfig
    {
    public:
        TMdbDataCheckConfig();
        ~TMdbDataCheckConfig();
        int Init(std::string strCfgFile, std::string strDataSource);
    private:
        int LoadSys(MDBXMLElement* pRoot);
        int LoadDsn(MDBXMLElement* pRoot);
        int LoadQdgTable(MDBXMLElement* pESec);
        int LoadDbInfo(MDBXMLAttribute* pAttr);
        bool CheckQdgTable(std::string strTable);
    public:
        TMdbDataCheckDSN m_tDsn;
        TMdbDataCheckInfo m_tCheckInfo;
        std::string m_sDataSource;
    };

    //不同的记录
    struct TMDBDiffRcd
    {
        char cDiffType;//不同的类型
        char cRecheckSame;//重新检查是否相同
        std::string strPk;//主键值串，逗号分隔
    };

    //不同记录写文件类
    class TMdbDiffFile
    {
    public:
        TMdbDiffFile();
        ~TMdbDiffFile();
    public:
        int Init(std::string strPath, std::string strTable,  int iBufLen);
        int Open(std::string strPath, std::string strTable,  int iBufLen);
        int WriteDiffRcd(TMDBDiffRcd *pDiffRcd);
        int GetNextDiffRcd(TMDBDiffRcd* pDiffRcd);
        int UpdateDiffRcdSame();
        int UpdateDiffType(int iDiffType);
        void SeekSetBegin(){ fseek(m_pFile, 0, SEEK_SET); }
    private:
        FILE *m_pFile;
        int m_iBufLen;
        char *m_pTmpBuf;
    };

    //主键按hash写文件类
    class TMdbHashFile
    {
    public:
        TMdbHashFile();
        ~TMdbHashFile();
    public:
        int Init(std::string strPath, std::string strTable, int iRcdNum, int iPkLen);//初始化
        int WritePk(std::string strPK); //记录一条主键
        bool IsPkExist(std::string strPK);//检查strPK对应的主键在文件中是否存在

    private:
        int GetHashValue(std::string strValue);//获取哈希值

    private:
        FILE* m_pFile;//文件指针
        int m_iRcdNum;//记录总数
        int m_iPkLen;//主键最大长度
        long long m_lCurMaxPos;//已经占用的最大位置
        char* m_pPkBuf;//主键buffer
        char m_sPosBuf[MDB_DATA_CHECK_MAX_POS_LEN];//位置缓冲区

        static const int m_iDefaultHash; //默认的哈希基数，当本地记录值为0或较小时，使用。防止本地记录缺失而对端记录较多时，冲突较多的的情况
    };

    //数据比较线程
    class TMdbCheckThread: public TMdbThreadBase
    {
    public:
        TMdbCheckThread();
        ~TMdbCheckThread();
        int Init(TMdbDataCheckConfig *pCheckConfig, TMdbConfig *pMdbConfig, std::string strPath);
        
        int Start();//开启线程
        void Stop();//停止线程
        int SetTableToCmp(TMdbDataCheckTable * pTableToCmp);//设置该线程待比较的表
        int SetTableToRestore(TMdbDataCheckTable * pTableToRestore);//设置该线程待恢复的表
    private:
        virtual int svc();
        int CompareOneTable();//比较一张表的数据，对端与本地比较
        int RestoreOneTable();//恢复一张表
        int ConnectDB();//连接对端和本地数据源
       
        int CmpOraTable();//与oracle中的表比较
        int CmpMdbTable();//与mdb中的表比较
        int CheckLocalData();//检查本地数据是否有冗余
        int ReCompare();//对不同数据重复比较
        void ClearCmpResult();//清理本次比较结果

        //构建各种sql
        int SetInsertSQL();//设置恢复数据时的插入SQL
        int SetUpdateSQL();//设置恢复数据时的更新SQL
        int SetDeleteSQL();//设置恢复数据时的插入SQL
        int SetSelectByPKSQL();//设置按主键查询sql
        int SetSelectAllSQL(); //设置查询所有数据sql
        int SetOraSelectAllSQL();//设置从oracle查询所有数据的sql
        int SetOraSelectByPKSQL();//设置oracle2mdb表（或qdg）的根据主键获取记录sql

        int SetLocalKeyParam(TMdbClientQuery *pPeerQuery, TMdbQuery *pLocalQuery);//第一次比较时，本地Query指针SQL参数
        int SetLocalKeyParam(TMDBDBQueryInterface *pPeerQuery, TMdbQuery *pLocalQuery);//第一次比较时，本地Query指针SQL参数

        int SetLocalParam(TMdbClientQuery *pPeerQuery, TMdbQuery* pLocalQuery);
        int SetLocalParam(TMDBDBQueryInterface *pPeerQuery, TMdbQuery* pLocalQuery);

        int SaveRecord(TMdbClientQuery *pPeerQuery, int iDiffType = MDB_DIFF_DATA_SAME);// 保存对端主机一条记录的主键，及不同记录
        int SaveRecord(TMDBDBQueryInterface *pOraQuery, int iDiffType = MDB_DIFF_DATA_SAME);//保存oracle中一条记录的主键，及不同记录

        int SaveDiffRcd(std::string strPk, int iDiffType);//保存一条不同记录

        int CompareOneMdbRcd(TMdbClientQuery *pPeerQuery, TMdbQuery *pLocalQuery, bool &bSame);//比较两个query指针对应的记录是否相同
        int CompareOneOraRcd(TMDBDBQueryInterface *pOraQuery, TMdbQuery *pLocalQuery, bool &bSame);//与oracle比较一条记录是否相同

        int ReCompareWithOra();
        int ReCompareWithPeer();

        ///////////////////数据恢复
        bool CheckColumIsPK(const int iColumnIdx);

        int RestoreDataFromPeer();
        int RestoreDataFromOra();
        int RestoreDataToOra();

        //从对端mdb恢复
        int SetPeesPKParam(std::string strPks);
        int SetRestoreLocalParam(TMdbClientQuery *pClientQuery, TMdbQuery* pLocalQuery, E_QUERY_TYPE eSqlType);
        int SetDeleteLocalParam(TMdbQuery* pLocalQuery, std::string strPks);

        //从oracle恢复
        int SetOraPKParam(std::string strPks);
        int SetRestoreLocalParam(TMDBDBQueryInterface *pOraQuery, TMdbQuery* pLocalQuery, E_QUERY_TYPE eSqlType);

        //向oracle恢复
        int SetLocalPKParam(std::string strPks);
        int SetRestoreOraParam(TMdbQuery *pOraQuery, TMDBDBQueryInterface* pLocalQuery, E_QUERY_TYPE eSqlType);
        int SetDeleteOraParam(TMDBDBQueryInterface* pOraQuery, std::string strPks);

        //mdb表的Load sql中配置有复杂参数
        void SetCompexParam(TMDBDBQueryInterface *pOraQuery);

    public:
        int m_iThreadNO;//线程号
        
        MDB_CHECK_THREAD_STATE m_eState;//线程状态
    private:
        TMdbDataCheckTable * m_pTableToCmp;//待比较的表
        TMdbDataCheckTable* m_pTableToRestore;//待恢复的表
        bool m_bDiffExist;//是否存在不同
        TMdbDataCheckConfig * m_pCheckConfig;
        TMdbConfig * m_pMdbConfig;
        std::string m_strPath;//文件路径

        TMdbDatabase *m_pLocalMdb;//本地mdb
        TMdbClientDatabase *m_pPeerMdb;//对端mdb
        TMDBDBInterface *m_pOraDB;//oracle

        TMdbQuery *m_pLocalQuery;
        TMdbClientQuery *m_pPeerQuery;
        TMDBDBQueryInterface *m_pOraQuery;
    private:
        TMdbHashFile *m_pPkFile;
        TMdbDiffFile * m_pDiffFile;

        //各种sql
        char m_sMdbSelectAllSQL[MAX_SQL_LEN];
        char m_sOraSelectAllSQL[MAX_SQL_LEN*2];//从oracle中加载数据的sql
        char m_sMdbSelectByPkSQL[MAX_SQL_LEN];
        char m_sOraSelectByPkSQL[MAX_SQL_LEN*2];//从oracle中按住键加载数据的sql
        bool m_bOraSelectNoBlob;//select语句中不包含blob
        char m_sUpdateSQL[MAX_SQL_LEN];
        char m_sInsertSQL[MAX_SQL_LEN];
        char m_sDeleteSQL[MAX_SQL_LEN];

        std::string m_strPK;
        TMdbTable *m_pTable;
        int m_iDiffRcd;

        bool m_bExit;//是否退出

    };

    //数据比较管理类
    class TMdbCheckDataMgr
    {
    public: 
        TMdbCheckDataMgr();
        ~TMdbCheckDataMgr();
    public:
        int Init(TMdbDataCheckConfig *pConfig);//初始化
        int Start();//开始比较
    private: 
        int StatTables();//统计表信息，生成m_vTables
        int CreateThreads();//创建比较子线程
        int CompareTables();//动态分配表到各个子线程
        int RestoreData();//恢复数据
        int StopThreads();//停止各个子线程

        int GenTableList(std::vector<TMdbDataCheckTable>& vTmpTable);//产生待处理表链表m_vTables
        bool CheckRepAttr(TMdbTable * pTable, TMdbCheckQdgTable* &pQdg);//检查表的同步属性
        int StatOneTable(TMdbTable * pTable, std::vector<TMdbDataCheckTable>& vTmpTable, TMdbCheckQdgTable* pQdg=NULL);//统计一张表的记录数，并保存到vTmpTable中

        TMdbDataCheckTable* GetCmpTable(std::string strTableName);
public:
    static bool m_bDetail;//是否输出比较的详细信息
    private:
        TMdbDataCheckConfig *m_pCheckConfig;
        TMdbConfig *m_pConfig;

        std::vector<TMdbDataCheckTable> m_vTables;//待处理的表
        std::vector<TMdbCheckThread*>   m_vThreads;//子线程
        std::vector<TMdbCheckThreadState> m_vStateTable;//子线程状态表

        int m_iDealedTable;//已分配的表个数
        int m_iThreadNum;

        TMdbDatabase * m_pDatabase;
        TMdbQuery * m_pQuery;
        std::string m_strPath;//文件的目录
    };


//}
#endif