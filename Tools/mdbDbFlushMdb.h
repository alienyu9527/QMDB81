/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbOraFlushMdb.h     
*@Description：oracle->mdb 的数据刷新
*@Author:       zhang.lin
*@Date：        2012年2月13日
*@History:
******************************************************************************************/
#ifndef __QUICK_MEMORY_DATABASE_ORA_FLUSH_MDB_H__
#define __QUICK_MEMORY_DATABASE_ORA_FLUSH_MDB_H__

#include <string>
#include <iostream>

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Interface/mdbQuery.h"
#include "Dbflush/mdbDAOBase.h"


//namespace QuickMDB{


    class TMdbShmDSN;
    class TMdbTableCtrl;



    class TMdbDbFlushMdb
    {
    public:
        TMdbDbFlushMdb();
        ~TMdbDbFlushMdb();
        
    public:
        /******************************************************************************
        * 函数名称  :  Init()
        * 函数描述  :  初始化,检查表同步属性、连接Oracle和minidb  等  
        * 输入      :  pszDSN, 内存数据库的DSN名称
        * 输入      :  pszTableName, 表名or all
        * 输出      :  无
        * 返回值    :  成功返回0，否则返回-1
        * 作者      :  zhang.lin
        *******************************************************************************/
        int Init(const char* pszDSN , const char* pszTableName);
        

    /******************************************************************************
    * 函数名称  :  FlushData()
    * 函数描述  :  校验表数据,并从oracle刷新
    * 输入      :  pszTable,表名或者all
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  zhang.lin
    *******************************************************************************/
       int FlushData(const char* pszTable);
        
    private:
        //根据DSN取出对应的MDB的用户名和密码
        int GetMDBUser(const char* pszDSN, char* pszUID, char* pszPWD);
        //根据DSN取出对应的oracle的用户名和密码
        int GetOraUser(char* pszDSN, char* pszUID, char* pszPWD);

    	/******************************************************************************
        * 函数名称  :  InitLoadCfg()
        * 函数描述  :  初始化及加载配置文件
        * 输入      :  pszDSN,  dsn名  
        * 输出      :  无
        * 返回值    :  成功返回0，否则返回错误码
        * 作者      :  zhang.lin
        *******************************************************************************/
    	int InitLoadCfg(const char* pszDSN);
       
       /******************************************************************************
        * 函数名称  :  CheckTableRepAttr()
        * 函数描述  :  检查表属性
        * 输入      :  pszTableName,  表名  
        * 输出      :  无
        * 返回值    :  成功返回0，否则返回错误码
        * 作者      :  zhang.lin
        *******************************************************************************/
       int CheckTableRepAttr(const char* pszTableName);

    /******************************************************************************
        * 函数名称  :  RecordData()
        * 函数描述  :  记录不一致的数据
        * 输入      :  pTable,  表指针 
        * 输出      :  无
        * 返回值    :  
        * 作者      :  zhang.lin
        *******************************************************************************/
       void RecordData(TMdbTable* pTable);
       
       /******************************************************************************
        * 函数名称  :  Restore()
        * 函数描述  :  恢复数据  
        * 输入      :  pszTable 表名
        * 输出      :  无
        * 返回值    :  成功返回0，否则返回-1
        * 作者      :  zhang.lin
        *******************************************************************************/
        int Restore(const char* pszTable=NULL);

    	bool RestoreData(const char* pszMsg,const char* pszTable=NULL);

    	TMdbTable* GetMdbTable(const char* pszTable);

    	int  GetPosOra(const char* pszMsg);
    	int  GetPosTabName(char* pszTabName,const char* pszMsg);

    	int  GetSQL(TMdbTable* pTable,char* sSQL,char* sMSQL,char* sMSelectSql,bool &bisAllowUpdate);

    	int GetInsertSQL(TMdbTable* pTable,char* sMInsertSql,const int iLen);
    	int  GenFlushDataSQL(TMdbTable* pTable,char* sSQL,char* sMSQL);

    	int  FlushAllTab();

    	TMdbTable*  GetTable(const char* pszTable);

    	bool  CheckSame(TMdbTable* pTable,TMdbQuery* pQuery);

    	int GetOraRepCounts(TMdbTable* pTable);
    	//复杂sql投参
    	void SetComplexParam(TMdbTable* pTable);

    	//把update和insert参数设进去
        void SetUpInParam(TMdbTable* pTable,bool bisAllowUpdate);
    private:
        TMdbDatabase m_tDB;     
        TMDBDBInterface* m_pDBLink;   //链接
        TMdbConfig *m_pConfig;  
        char m_sMDBSQL[MAX_SQL_LEN]; //对应的mdb SQL
        char m_sORaSQL[MAX_SQL_LEN]; //对应的Oracle SQL    
        TMdbDAOBase* m_pDAO;
        TMdbTable* m_pTable;

        char  m_sFileName[MAX_PATH_NAME_LEN];//对比数据文件名
        int   m_iDiffRecords;//异常数据记录数

        TMDBDBQueryInterface     *m_pOraQuery;
    	TMdbQuery     *m_pMQuery;
        TMdbQuery     *m_pMInsert;
        TMdbQuery     *m_pMSelect;
    	
    };
//}    




#endif //__QUICK_MEMORY_DATABASE_ORA_FLUSH_MDB_H__




