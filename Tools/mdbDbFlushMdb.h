/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbOraFlushMdb.h     
*@Description��oracle->mdb ������ˢ��
*@Author:       zhang.lin
*@Date��        2012��2��13��
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
        * ��������  :  Init()
        * ��������  :  ��ʼ��,����ͬ�����ԡ�����Oracle��minidb  ��  
        * ����      :  pszDSN, �ڴ����ݿ��DSN����
        * ����      :  pszTableName, ����or all
        * ���      :  ��
        * ����ֵ    :  �ɹ�����0�����򷵻�-1
        * ����      :  zhang.lin
        *******************************************************************************/
        int Init(const char* pszDSN , const char* pszTableName);
        

    /******************************************************************************
    * ��������  :  FlushData()
    * ��������  :  У�������,����oracleˢ��
    * ����      :  pszTable,��������all
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻�-1
    * ����      :  zhang.lin
    *******************************************************************************/
       int FlushData(const char* pszTable);
        
    private:
        //����DSNȡ����Ӧ��MDB���û���������
        int GetMDBUser(const char* pszDSN, char* pszUID, char* pszPWD);
        //����DSNȡ����Ӧ��oracle���û���������
        int GetOraUser(char* pszDSN, char* pszUID, char* pszPWD);

    	/******************************************************************************
        * ��������  :  InitLoadCfg()
        * ��������  :  ��ʼ�������������ļ�
        * ����      :  pszDSN,  dsn��  
        * ���      :  ��
        * ����ֵ    :  �ɹ�����0�����򷵻ش�����
        * ����      :  zhang.lin
        *******************************************************************************/
    	int InitLoadCfg(const char* pszDSN);
       
       /******************************************************************************
        * ��������  :  CheckTableRepAttr()
        * ��������  :  ��������
        * ����      :  pszTableName,  ����  
        * ���      :  ��
        * ����ֵ    :  �ɹ�����0�����򷵻ش�����
        * ����      :  zhang.lin
        *******************************************************************************/
       int CheckTableRepAttr(const char* pszTableName);

    /******************************************************************************
        * ��������  :  RecordData()
        * ��������  :  ��¼��һ�µ�����
        * ����      :  pTable,  ��ָ�� 
        * ���      :  ��
        * ����ֵ    :  
        * ����      :  zhang.lin
        *******************************************************************************/
       void RecordData(TMdbTable* pTable);
       
       /******************************************************************************
        * ��������  :  Restore()
        * ��������  :  �ָ�����  
        * ����      :  pszTable ����
        * ���      :  ��
        * ����ֵ    :  �ɹ�����0�����򷵻�-1
        * ����      :  zhang.lin
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
    	//����sqlͶ��
    	void SetComplexParam(TMdbTable* pTable);

    	//��update��insert�������ȥ
        void SetUpInParam(TMdbTable* pTable,bool bisAllowUpdate);
    private:
        TMdbDatabase m_tDB;     
        TMDBDBInterface* m_pDBLink;   //����
        TMdbConfig *m_pConfig;  
        char m_sMDBSQL[MAX_SQL_LEN]; //��Ӧ��mdb SQL
        char m_sORaSQL[MAX_SQL_LEN]; //��Ӧ��Oracle SQL    
        TMdbDAOBase* m_pDAO;
        TMdbTable* m_pTable;

        char  m_sFileName[MAX_PATH_NAME_LEN];//�Ա������ļ���
        int   m_iDiffRecords;//�쳣���ݼ�¼��

        TMDBDBQueryInterface     *m_pOraQuery;
    	TMdbQuery     *m_pMQuery;
        TMdbQuery     *m_pMInsert;
        TMdbQuery     *m_pMSelect;
    	
    };
//}    




#endif //__QUICK_MEMORY_DATABASE_ORA_FLUSH_MDB_H__




