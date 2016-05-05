/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbDDLExecuteEngine.h		
*@Description�� DDL SQLִ������
*@Author:	    cao.peng
*@Date��	    
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

//mdb ִ������
class TMdbDDLExecuteEngine
{
public:
	TMdbDDLExecuteEngine();
	~TMdbDDLExecuteEngine();
	int Init(TMdbSqlParser * pMdbSqlParser);
	int SetDB(TMdbShmDSN * pShmDsn,TMdbConfig *pConfig);//����DSN
	int Execute();
	int LoadDataFromRepOrOracle(const char *pTableName);
	int SetCurOperateUser(const char* pUserName);//���õ�ǰ�������û���Ϣ
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
	* ��������	:  AddSequenceToOracle
	* ��������	:  ����oracle��mdb_sequence���м�¼
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int AddSequenceToOracle();
	/******************************************************************************
	* ��������	:  DelSequenceToOracle
	* ��������	:  ɾ��oracle��sequence����ָ������
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int DelSequenceToOracle();
	/******************************************************************************
	* ��������	:  AlterSequence
	* ��������	:  �޸Ĺ����ڴ�������(oracle��������ͬ�����̴���)
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int AlterSequence();
	/******************************************************************************
	* ��������	:  ConnectOracle
	* ��������	:  ����ORACLE
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int ConnectOracle();
	/******************************************************************************
	* ��������	:  CreateSyncShm
	* ��������	:  ���ݱ��ͬ������������������oracleͬ����
	* ����		:  iRepType ���ͬ������
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int CreateSyncShm(int iRepType, bool bSharedBackUp);
	/******************************************************************************
	* ��������	:  StartOraRepProcess
	* ��������	:  ����ORacleͬ����ؽ���
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int StartOraRepProcess();
	/******************************************************************************
	* ��������	:  StartRepProcess
	* ��������	:  ��������ͬ������
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int StartRepProcess();
	/******************************************************************************
	* ��������	:  StartCaptureProcess
	* ��������	:  ����Capture����ˢ������
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int StartCaptureProcess();
	/******************************************************************************
	* ��������	:  ExecuteCreateJob
	* ��������	:  ����job
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int ExecuteCreateJob();
	/******************************************************************************
	* ��������	:  ExecuteAlterJob
	* ��������	:  �޸�job����
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int ExecuteAlterJob();
	/******************************************************************************
	* ��������	:  ExecuteRemoveJob
	* ��������	:  ɾ��job
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  cao.peng
	*******************************************************************************/
	int ExecuteRemoveJob();
	int AddFlushSQLOrLoadSQLParam();//����flush-sql or load-sql��̬����
	int DropFlushSQLOrLoadSQLParam();//ɾ��flush-sql or load-sql��̬����
	int ModifyFlushSQLOrLoadSQLParam();//�޸�flush-sql or load-sql��̬����
	int ExecuteAlterOpt(const int iSqlType);//��װ���е�alter����
	int ExecuteDropDsn();//���ݿ�ɾ���ӿ�
	int RestartOraRepProcess();//����oracle���ݽ���
	//int ModifColumnRepType(TMdbTable * pTable);//�޸��е�ͬ����������ͬ������һ��
	int DeleteDBARecord(const char* psTableName);
    int ExecuteRenameTable(const char *pNewTableName,const bool bIsGenXML);//rename table
    int ChangePageTableInfo(TMdbTable * pTable,const char *sNewTableName);//ҳ��Ϣ�޸�
    
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
	TMdbJob            *m_pMdbJob;//job�������
	TMdbDatabase m_tMDB;
	TMdbScript   *m_pScript;
	TMDBDBInterface *m_pDBLink; //Oracle����
	TMdbProcCtrl m_tProcCtrl; //���̿��ƹ���
	TMDbUser *m_pCurOperUser;
public:
	TMdbErrorHelper m_tError;
};

//}

#endif
