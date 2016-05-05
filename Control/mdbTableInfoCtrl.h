#ifndef _MDB_TABLE_INFO_CTRL_
#define _MDB_TABLE_INFO_CTRL_

#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Helper/mdbConfig.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbStorageEngine.h"

//namespace QuickMDB{

    class TMdbLoadFromDb;
    //QMDB中表信息管理类
    //对表信息drop create等操作
    class TMdbTableInfoCtrl
    {
    public:
    	TMdbTableInfoCtrl();
    	~TMdbTableInfoCtrl();
    	/******************************************************************************
        * 函数名称	:  SetDSN()
        * 函数描述	:  设置DSN信息    
        * 输入		:  pszDSN, 管理区所属的DSN 
        * 输出		:  无
        * 返回值	:  成功返回0, 失败返回-1
        * 作者		:  jin.shaohua
        *******************************************************************************/
    	int SetDSN(const char* pszDSN);
    	/******************************************************************************
        * 函数名称	:  CreateNewTable()
        * 函数描述	:  创建新table;
        * 输入		:  pTableName, 表名
        * 输出		:  无
        * 返回值	:  成功返回0, 失败返回错误码
        * 作者		:  jin.shaohua
        *******************************************************************************/
    	int CreateNewTable(const char * pTableName);
    	/******************************************************************************
        * 函数名称	:  DropTable()
        * 函数描述	:  删除表
        * 输入		:  pTableName, 表名
        * 输出		:  无
        * 返回值	:  成功返回0, 失败返回错误码
        * 作者		:  jin.shaohua
        *******************************************************************************/
    	int DropTable(const char * pTableName, bool bCheckpoint = true);
		/******************************************************************************
	    * 函数名称	:  TruncateTable()
	    * 函数描述	:  Truncate表
	    * 输入		:  pTableName, 表名
	    * 输出		:  无
	    * 返回值	:  成功返回0, 失败返回错误码
	    * 作者		:  jin.shaohua
	    *******************************************************************************/
		int TruncateTable(const char * pTableName);
    private:
    	//是否合法:表还未存在,而且不属于dba_开头的系统表
    	bool IsValidNewTable(const char * pTableName);
    	//校验是否需要从oracle 上载
    	bool IsNeedLoadFromOra(TMdbTable * pTable);
    private:
          int DeleteData(TMdbTable * pTable);//删除数据
    	//删除表管理的页面,归还表空间
    	int DeletePage(TMdbTable * pTable);
    	//删除数据对应的所有索引
    	int DeleteIndex(TMdbTable *pTable);
		int TruncateIndex(TMdbTable * pTable);
		int TruncateData(TMdbTable * pTable);
    private:
    	//创建表失败的情况下可以回滚
    	int CreateToRollBack();
    	
    private:
        TMdbConfig *m_pConfig;
        TMdbShmDSN *m_pShmDSN;   
        TMdbDSN    *m_pDsn;

        TMdbTable*     m_pTable;                   //表的整体信息，与索引信息对应起来
        TMdbTableSpaceCtrl m_mdbTSCtrl;            //表空间控制信息
        TMdbDatabase m_tMDB;
    	
    };


//}
#endif
