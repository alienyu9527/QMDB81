/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbDDLExecuteEngine.h		
*@Description： DDL SQL执行引擎
*@Author:	    cao.peng
*@Date：	    
*@History:
******************************************************************************************/
#ifndef _MDB_DDLEXECUTE_ENGINE_H_
#define _MDB_DDLEXECUTE_ENGINE_H_

#include "Helper/mdbSQLParser.h"
#include "Helper/TDBFactory.h"
#include "Control/mdbPageCtrl.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Control/mdbFlush.h"
#include "Helper/mdbErrorHelper.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbScript.h"



//namespace QuickMDB{

//mdb 执行引擎
class TMdbDDLExecuteEngine
{
public:
	TMdbDDLExecuteEngine();
	~TMdbDDLExecuteEngine();
	int Init(TMdbSqlParser * pMdbSqlParser);
	int SetDB(TMdbShmDSN * pShmDsn,TMdbConfig *pConfig);//设置DSN
	int Execute();
	int LoadDataFromRepOrOracle(const char *pTableName);
	int SetCurOperateUser(const char* pUserName);//设置当前操作的用户信息
protected:
	int ExecuteCreateOpt(const int iSqlType);
	int ExecuteDropOpt(const int iSqlType);	
	int ExecuteTruncateOpt(const int iSqlType);	
	int ExecuteCreateTable(const bool bIsGenXML=true);
	int ExecuteDropTable(const char *pTableName,const bool bIsGenXML=true);
	int ExecuteTruncateTable(const char *pTableName,const bool bIsGenXML=true);
    int ExecuteAlterDsn();
	
	int ExecuteAddIndex(const char * pTableName);
	int ExecuteDropIndex(const char * pTableName,const char *pIndxName);
	int ExecuteAddPrimary(const char * pTableName);

	int ExecuteCreateUser();
	int ExecuteAlterUser();
	int ExecuteDropUser();

	int ExecuteCreateTablespace();
	int ExecuteAlterTablespace();
	int ExecuteDropTablespace();

	int ExecuteModifyTableAttr();
	int ExecuteModifyTable();
	
	int CreateTmpTable(const char* psTableName);
	bool CheckColumnInTable(const char* pClomName,const TMdbTable * pTable);
	int MigDataFromOldTONewTable(TMdbTable * pOldTable,TMdbTable * pNewTable,bool bIsRep);
	void InitSelectSQL(const TMdbTable * pTable);
	void InitInsertSQL(const TMdbTable * pTable);

	bool IsValidNewTable(const char * pTableName);
	bool IsSysTable(const char * pTableName);
	int ConnectMDB();
	int SyncUser(int iOptType,TMDbUser* pUser);
	bool IsLoadFromOracle(TMdbTable * pTable);
	bool IsLoadFromRep(TMdbTable * pTable);
	int CorrectionColumn(TMdbColumn *pColumn,TMdbTable * pTable,const char *pColumnAttr=NULL);
	/******************************************************************************
	* 函数名称	:  AddSequenceToOracle
	* 函数描述	:  增加oracle侧mdb_sequence表中记录
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int AddSequenceToOracle();
	/******************************************************************************
	* 函数名称	:  DelSequenceToOracle
	* 函数描述	:  删除oracle侧sequence表中指定序列
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int DelSequenceToOracle();
	/******************************************************************************
	* 函数名称	:  AlterSequence
	* 函数描述	:  修改共享内存中序列(oracle侧序列由同步进程处理)
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int AlterSequence();
	/******************************************************************************
	* 函数名称	:  ConnectOracle
	* 函数描述	:  连接ORACLE
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int ConnectOracle();
	/******************************************************************************
	* 函数名称	:  CreateSyncShm
	* 函数描述	:  根据表的同步属性来创建主备或oracle同步区
	* 输入		:  iRepType 表的同步属性
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int CreateSyncShm(int iRepType, bool bSharedBackUp);
	/******************************************************************************
	* 函数名称	:  StartOraRepProcess
	* 函数描述	:  启动ORacle同步相关进程
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int StartOraRepProcess();
	/******************************************************************************
	* 函数名称	:  StartRepProcess
	* 函数描述	:  启动主备同步进程
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int StartRepProcess();
	/******************************************************************************
	* 函数名称	:  StartCaptureProcess
	* 函数描述	:  启动Capture队列刷出进程
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int StartCaptureProcess();
	/******************************************************************************
	* 函数名称	:  ExecuteCreateJob
	* 函数描述	:  创建job
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int ExecuteCreateJob();
	/******************************************************************************
	* 函数名称	:  ExecuteAlterJob
	* 函数描述	:  修改job属性
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int ExecuteAlterJob();
	/******************************************************************************
	* 函数名称	:  ExecuteRemoveJob
	* 函数描述	:  删除job
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  cao.peng
	*******************************************************************************/
	int ExecuteRemoveJob();
	int AddFlushSQLOrLoadSQLParam();//增加flush-sql or load-sql动态参数
	int DropFlushSQLOrLoadSQLParam();//删除flush-sql or load-sql动态参数
	int ModifyFlushSQLOrLoadSQLParam();//修改flush-sql or load-sql动态参数
	int ExecuteAlterOpt(const int iSqlType);//封装所有的alter操作
	int ExecuteDropDsn();//数据库删除接口
	int RestartOraRepProcess();//重启oracle备份进程
	//int ModifColumnRepType(TMdbTable * pTable);//修改列的同步属性与表的同步属性一致
	int DeleteDBARecord(const char* psTableName);
    int ExecuteRenameTable(const char *pNewTableName,const bool bIsGenXML);//rename table
    int ChangePageTableInfo(TMdbTable * pTable,const char *sNewTableName);//页信息修改
    
private:
        
	char sTableSpaceName[MAX_NAME_LEN];
	char m_sSSQL[MAX_SQL_LEN];
	char m_sISQL[MAX_SQL_LEN];
	TMdbSqlParser * m_pMdbSqlParser;
	TMdbTable * m_pTable;
	TMdbDSN   * m_pDsn;
	TMdbShmDSN * m_pShmDSN;
	TMdbConfig         *m_pConfig;
	TMDbUser           *m_pUser;
	TMdbTableSpaceCtrl m_mdbTSCtrl;
	TMdbIndexCtrl      m_mdbIndexCtrl;
	TMdbJob            *m_pMdbJob;//job管理对象
	TMdbDatabase m_tMDB;
	TMdbScript   *m_pScript;
	TMDBDBInterface *m_pDBLink; //Oracle链接
	TMdbProcCtrl m_tProcCtrl; //进程控制管理
	TMDbUser *m_pCurOperUser;
public:
	TMdbErrorHelper m_tError;
};

//}

#endif
