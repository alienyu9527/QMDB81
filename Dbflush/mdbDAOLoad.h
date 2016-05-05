/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbDAOLoad.h		
*@Description： 负责把表中的数据上载上来
*@Author:		li.shugang
*@Date：	    2008年12月16日
*@History:
******************************************************************************************/
#ifndef __QUICK_MEMORY_DATABASE_DAO_LOAD_H__
#define __QUICK_MEMORY_DATABASE_DAO_LOAD_H__

#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Interface/mdbQuery.h"

//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{


#define _TRY_BEGIN_ try{
#define _TRY_END_ }\
catch(TMDBDBExcpInterface &oe)\
{\
    TADD_ERROR(ERROR_UNKNOWN," TDBException:%s\n", oe.GetErrMsg());\
    TADD_ERROR(ERROR_UNKNOWN,"QuerySQL:\n%s\n", oe.GetErrSql());\
    return ERROR_UNKNOWN;\
}\
catch(TMdbException& e)\
{\
    TADD_ERROR(ERROR_UNKNOWN,"TMDBException:%s\n", e.GetErrMsg());\
    TADD_ERROR(ERROR_UNKNOWN," QuerySQL:\n%s\n", e.GetErrSql());\
    return ERROR_UNKNOWN;\
}

#define _TRY_END_NO_RETURN_ }\
catch(TMDBDBExcpInterface &oe)\
{\
    TADD_ERROR(ERROR_UNKNOWN," TDBException:%s\n", oe.GetErrMsg());\
    TADD_ERROR(ERROR_UNKNOWN,"QuerySQL:\n%s\n", oe.GetErrSql());\
}\
catch(TMdbException& e)\
{\
    TADD_ERROR(ERROR_UNKNOWN,"TMDBException:%s\n", e.GetErrMsg());\
    TADD_ERROR(ERROR_UNKNOWN," QuerySQL:\n%s\n", e.GetErrSql());\
}\
catch(...)\
{\
	TADD_ERROR(ERROR_UNKNOWN,"ERROR\n");\
}


class TMdbDAOLoad
{
public:
    /******************************************************************************
    * 函数名称	:  TMdbDAOLoad()
    * 函数描述	:  获取Oracle链接项
    * 输入		:  pConfig，配置参数 
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    TMdbDAOLoad(TMdbConfig *pConfig);

    ~TMdbDAOLoad();

    /******************************************************************************
    * 函数名称	:  Connect()
    * 函数描述	:  链接Oracle数据库 
    * 输入		:  无 
    * 输出		:  无
    * 返回值	:  成功返回true，否则返回false
    * 作者		:  li.shugang
    *******************************************************************************/
    bool Connect();

    /******************************************************************************
    * 函数名称	:  DisConnect()
    * 函数描述	:  断开Oracle链接 
    * 输入		:  无 
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void DisConnect();

    /******************************************************************************
    * 函数名称	:  Init()
    * 函数描述	:  初始化某个表，拼写出SQL语句    
    * 输入		:  pTable, 待处理的表 
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int Init(TMdbShmDSN* pShmDSN, TMdbTable* pTable, const char* sRoutingList = NULL,const char* sFilerSql = NULL);

    /******************************************************************************
    * 函数名称	:  Load()
    * 函数描述	:  上载某个表 
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int Load(TMdbDatabase* pMdb);
    /******************************************************************************
    * 函数名称	:  LoadSequence()
    * 函数描述	:  上载序列
    * 输入		:  pMdb,pSeq,pDsn
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int LoadSequence(TMdbDatabase* pMdb,TMdbDSN* pDsn);

    /******************************************************************************
    * 函数名称	:  ReLoad()
    * 函数描述	:  重新上载某个表
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  zhang.lin
    *******************************************************************************/
    int ReLoad(TMdbDatabase* pMdb);

    int SetSequenceName(char* sSeqenceName,char* sDsnName);
	
	

private:
    int InitLoadSql(const char* sRoutingList = NULL,const char* sFilterSql = NULL);
    int InitInsertSql();
    /******************************************************************************
    * 函数名称	:  InitQueryInQMDB()
    * 函数描述	:  拼写校验是否已经在QMDB中存在的SQL
    * 输入		:
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  cao.peng
    *******************************************************************************/
    int InitQueryInQMDB();

    int InitCheckFieldSql();
    //检查列是否在oracle表中存在
    int CheckFieldExist();
    int LoadAsString(TMdbDatabase* pMdb);
    int LoadAsNormal(TMdbDatabase* pMdb);

    int ReLoadAsNormal(TMdbDatabase* pMdb);
    int ReLoadAsString(TMdbDatabase* pMdb);

    bool CheckRecord(TMdbQuery * pQMDBQuery,TMDBDBQueryInterface* pOraQuery);
    bool CheckAsStringRecord(TMdbQuery * pQMDBQuery,TMDBDBQueryInterface* pOraQuery);
    void SetLoadSQLParameter();
    bool IsLoadColumn(TMdbColumn &tColumn);
    int GetLoadAsStringSQL(char sSQL[],const int iSize,const char* sFilterSql = NULL);
    int GetLoadAsNormalSQL(char sSQL[],const int iSize,const char* sFilterSql = NULL);
    bool isKey(int iColumnNo);//判断某列是否为主键列
    int UpdateMDBStringData(const TMdbTable* pTable,TMdbNtcSplit &tSplit,int &iCount);//更新数据
    int UpdateMDBNormalData(const TMdbTable* pTable,int &iCount);//更新数据
    int InitQueryForMDB(TMdbDatabase* pMdb);//初始化各个QUERY
    int InitUpdateSQLInMDB();//初始化更新SQL

    /******************************************************************************
	* 函数名称	:  AddRouteIDForLoadSQL()
	* 函数描述	:  为表刷新SQL添加路由
	* 输入		:  pTable 表
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  cao.peng
	*******************************************************************************/
	void AddRouteIDForLoadSQL(const char* sRoutingList);

    /******************************************************************************
    * 函数名称	:  HasWhereCond()
    * 函数描述	:  判断一张表的Load SQL语句中是否包含where条件
    * 输入		:  sTableName 表名
    * 输入		:  sLoadSQL sql语句
    * 输出		:  无
    * 返回值	:  存在返回true,否则返回false
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool HasWhereCond(const char* sTableName, const char * sLoadSQL);
private:
    int m_iSeqCacheSize; //序列Cache大小
    char m_sSQL[MAX_SQL_LEN];  //加载SQL(select)，有load-sql就用配置加载SQL。
    char m_sMSQL[MAX_SQL_LEN];  //存储拼写出来的SQL,insert sql

    char m_sCkFieldSQL[MAX_SQL_LEN];

    char m_sQMSQL[MAX_SQL_LEN]; //存储查询SQL，校验是否已经存在 select %s from %s where %s=:%s and %s=:%s
    char m_sUpdateSQL[MAX_SQL_LEN];//用于更新QMDB侧表数据SQL
    char m_sDSN[MAX_NAME_LEN]; //Oracle DSN
    char m_sUID[MAX_NAME_LEN]; //Oracle UID
    char m_sPWD[MAX_NAME_LEN]; //Oracle PWD

    TMDBDBInterface* m_pDBLink;   //链接
    TMDBDBQueryInterface*    m_pOraQuery;    //动作
    TMdbTable* m_pTable;
    TMdbShmDSN* m_pShmDSN;
    TMdbQuery *m_pInsertQuery;
    TMdbQuery *m_pCheckQuery;
    TMdbQuery *m_pUpdateQuery;
    std::map<string,bool> m_mapFieldExist;
};


//}

#endif //__QUICK_MEMORY_DATABASE_DAO_LOAD_H__

