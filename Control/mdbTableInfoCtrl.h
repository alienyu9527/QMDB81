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
    //QMDB�б���Ϣ������
    //�Ա���Ϣdrop create�Ȳ���
    class TMdbTableInfoCtrl
    {
    public:
    	TMdbTableInfoCtrl();
    	~TMdbTableInfoCtrl();
    	/******************************************************************************
        * ��������	:  SetDSN()
        * ��������	:  ����DSN��Ϣ    
        * ����		:  pszDSN, ������������DSN 
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1
        * ����		:  jin.shaohua
        *******************************************************************************/
    	int SetDSN(const char* pszDSN);
    	/******************************************************************************
        * ��������	:  CreateNewTable()
        * ��������	:  ������table;
        * ����		:  pTableName, ����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ��ش�����
        * ����		:  jin.shaohua
        *******************************************************************************/
    	int CreateNewTable(const char * pTableName);
    	/******************************************************************************
        * ��������	:  DropTable()
        * ��������	:  ɾ����
        * ����		:  pTableName, ����
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ��ش�����
        * ����		:  jin.shaohua
        *******************************************************************************/
    	int DropTable(const char * pTableName, bool bCheckpoint = true);
		/******************************************************************************
	    * ��������	:  TruncateTable()
	    * ��������	:  Truncate��
	    * ����		:  pTableName, ����
	    * ���		:  ��
	    * ����ֵ	:  �ɹ�����0, ʧ�ܷ��ش�����
	    * ����		:  jin.shaohua
	    *******************************************************************************/
		int TruncateTable(const char * pTableName);
    private:
    	//�Ƿ�Ϸ�:��δ����,���Ҳ�����dba_��ͷ��ϵͳ��
    	bool IsValidNewTable(const char * pTableName);
    	//У���Ƿ���Ҫ��oracle ����
    	bool IsNeedLoadFromOra(TMdbTable * pTable);
    private:
          int DeleteData(TMdbTable * pTable);//ɾ������
    	//ɾ��������ҳ��,�黹��ռ�
    	int DeletePage(TMdbTable * pTable);
    	//ɾ�����ݶ�Ӧ����������
    	int DeleteIndex(TMdbTable *pTable);
		int TruncateIndex(TMdbTable * pTable);
		int TruncateData(TMdbTable * pTable);
    private:
    	//������ʧ�ܵ�����¿��Իع�
    	int CreateToRollBack();
    	
    private:
        TMdbConfig *m_pConfig;
        TMdbShmDSN *m_pShmDSN;   
        TMdbDSN    *m_pDsn;

        TMdbTable*     m_pTable;                   //���������Ϣ����������Ϣ��Ӧ����
        TMdbTableSpaceCtrl m_mdbTSCtrl;            //��ռ������Ϣ
        TMdbDatabase m_tMDB;
    	
    };


//}
#endif
