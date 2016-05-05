/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbChangeNotify.h		
*@Description�� �ڴ����ݿ��Oracleͬ���������
*@Author:		li.shugang
*@Date��	    2009��5��07��
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_CHANGE_NOTIFY_H__
#define __MINI_DATABASE_CHANGE_NOTIFY_H__

#include "Helper/mdbConfig.h"
#include "Helper/TDBFactory.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbMgrShm.h"
#include "Control/mdbProcCtrl.h"
#include <algorithm>
#include <string.h>
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

using namespace std;

//namespace QuickMDB{

    // ��ˢ�������д�ŵ����ڵ���
    #define MAX_FLUSH_ARRAY_CNT      10000

    struct LSNODE
    {
        MDB_INT64  iSeq;//©ˢsequence
        char sTabName[MAX_NAME_LEN];
        char sTime[MAX_TIME_LEN];//ʱ��
    };

    // �Ѵ����ˢ�½ڵ�
    struct SNODE
    {
        bool bFlush; // �Ƿ��ѱ�ˢ��
        MDB_INT64 lSeq; // table_sequence
    };

    class CNQueryList
    {
    public:
        CNQueryList();
        ~CNQueryList();
        int Init(TMdbDatabase *pMdbLink,TMDBDBInterface *pOraLink,TMdbTable* pTable,TMdbConfig  *pConfig,char* dsn_name,char* mdbChangeNotifyName,char* mdbChangeNotifySeqName);
        int InitMdbQuery();
        int InitOraSQL();
        TMdbDatabase *m_pMdbLink;
        TMDBDBInterface *m_pOraLink;
        TMdbConfig  *m_pConfig;
        TMdbQuery *m_pQueryQ;  //QMDB��ѯ���
        TMdbQuery *m_pQueryU;  //QMDB���¾��
        TMdbQuery *m_pQueryI;  //QMDB������
        TMdbQuery *m_pQueryD;  //QMDBɾ�����	
        TMdbTable* m_pTable;
        char m_sUpperDsn[MAX_NAME_LEN];
        char m_sOraSelectSQL[MAX_SQL_LEN];
        char m_sOraDeleteSQL[MAX_SQL_LEN];
        char m_sGetSeqFromNotifySeqSQL[MAX_SQL_LEN];
        char m_sGetSeqFromNotifySQL[MAX_SQL_LEN];
        char m_sInsertNotifySeqSQL[MAX_SQL_LEN];
        char m_sUpdateNotifySeqSQL[MAX_SQL_LEN];
        char m_sSQL[MAX_SQL_LEN];
        char m_sChangeNotifyName[MAX_NAME_LEN];
        char m_sChangeNotifySeqName[MAX_NAME_LEN];
        int m_iFlushCount;
        char m_sShadowName[MAX_NAME_LEN];

    public:
        //��ȡ��ѯ���
        int GetQuerySQL();

        //��ȡ���¾��
        int GetUpdateSQL();

        //��ȡ������
        int GetInsertSQL();

        //��ȡɾ�����
        int GetDeleteSQL(TMDBDBQueryInterface* ptOraQry); 	

        //��ȡOracleSQL����
        int GetOraSQL();
        //��ȡoracle delete sql
        int GetOraDSQL();

        int GetSeqFromNotifySeqSQL();
        int GetSeqFromNotifySQL();
        int GetInsertNotifySeqSQL();
        int GetUpdateNotifySeqSQL();
        bool isKey(int iColumnNo);
    };


    class TMdbChangeNotify
    {
    public:
        TMdbChangeNotify();
        ~TMdbChangeNotify();

        /******************************************************************************
        * ��������	:  Init()
        * ��������	:  ��ʼ��������Oracle  
        * ����		:  pszDSN, ��������������DSN  
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻ظ���
        * ����		:  li.shugang
        *******************************************************************************/
        int Init(const char* pszDSN, const char* pszName);

        /******************************************************************************
        * ��������	:  Start()
        * ��������	:  ͬ��Oracle����
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻ظ���
        * ����		:  li.shugang
        *******************************************************************************/
        int Start();

    private:
        int FlushTable(TMdbTable* pTable);
        int FlushRecord(MDB_INT64 &iStartSeq,MDB_INT64 &iEndSeq,MDB_INT64 &iUpdateSequence,char *sUpdateTime); //ˢ�·�Delete�������
        int FlushDRecord(MDB_INT64 &iStartSeq,MDB_INT64 &iEndSeq,MDB_INT64 &iUpdateSequence,char *sUpdateTime); //ˢ�·�Delete�������
        int UpdateNotifySeqTable(MDB_INT64 iUpdateSequence,char *sUpdateTime);
        int GetStartEndSeq(MDB_INT64 &iStartSeq, MDB_INT64 &iEndSeq);
        bool FiltTable(TMdbTable* pTable);
        int ClearAllTableQuery();
        int InitTable();
        //��©�������ݲ���"©ˢ����"
        void InsertDelayList(MDB_INT64 iStart, MDB_INT64 iEnd);

        //����"©ˢ����"�е�����
        int DealDelayData();

        //ɾ������
        int DeleteData();

        //��������
        int InsertData();

        //��������
        int UpdateData();

        //��ѯ����,��������ݷ���1, ���򷵻�0, ʧ�ܷ���<0
        int QueryData();		

        //����Oracle����
        int ConnectOracle();

        bool isKey(int iColumnNo);
        int GetAdministrator();		

        /******************************************************************************
        * ��������	:  InitOraQry()
        * ��������	:  ��ʼ�� %dsn%_mdb_change_notif���ѯ���
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻ظ���
        * ����		:  li.ming
        *******************************************************************************/
        int InitOraQry();

        /******************************************************************************
        * ��������	:  NeedToFlush()
        * ��������	:  ��ѯ %dsn%_mdb_change_notif��,�ж��Ƿ�����Ҫ��ˢ�µ�����
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��Ҫ����ˢ�·���true�����򷵻�false
        * ����		:  li.ming
        *******************************************************************************/
        bool NeedToFlush();

        int InitFlushList(MDB_INT64 lStartSeq, MDB_INT64 lEndSeq);
        int AddToFlushList(MDB_INT64 lStartSeq, MDB_INT64 lSeq);
        void ClearFlushList();
        int SetSqlName();

        CNQueryList* GetTabQuery(const char* psTabName);

        MDB_INT64 GetSmartStartSeq(MDB_INT64 lStartSeq);

        void PrintFlushLog();
        void LogFlushInfo(std::string&  sLogInfo);
        void LogRecdNotifyInfo(const MDB_INT64 iStartSeq, const MDB_INT64 iEndSeq, const MDB_INT64 iCurSeq, const char cActionType, const char* psTabName, std::string & sRecdStr);
        void LogRecdPkInfo(std::string & sRecdStr, const char* psPkName, const char* psPkValue);
        
    private: 
        TMDBDBInterface *m_pDBLink;   //Oracle����
        TMdbDatabase *m_mdbLink;     //QMDB����
        TMdbConfig  *m_pConfig;
        TMdbProcCtrl m_tProcCtrl;//���̿���
        TMdbShmDSN  *m_pShmDSN;
        TMdbDSN     *m_pDsn;     
        CNQueryList*  m_tQueryList[MAX_TABLE_COUNTS];
        TMdbTable* m_tCurTable;
        CNQueryList* m_tCurQuery;
        TMDBDBQueryInterface *m_pOraQry; // %dsn%_mdb_change_notif���ѯ�������ȡ�Ƿ�����Ҫ��ˢ�µ�����
        int m_iTotalFlushCount;
        int m_iTimeInterval; // ˢ��ʱ������
        int m_iDelayTime; // ��ˢ�ڵ�ĳ�ʱʱ�䣻
        char m_sOpenStartTime[MAX_TIME_LEN]; //ÿ��ȡˢ������open��ʼʱ�䣻
        char m_sOpenEndTime[MAX_TIME_LEN]; // ÿ��ȡˢ������open����ʱ�䣻
        char m_sQrySql[MAX_SQL_LEN]; // ��ȡ�Ƿ�����Ҫˢ�����ݵ�sql
        std::vector<LSNODE> m_vLsList;//©ˢ����

        bool m_bAlloc; // �Ƿ�������������ˢ�½ڵ������־
        MDB_INT64 m_lFlushCnt; // ��Ҫˢ�µĽڵ���
        MDB_INT64 m_lStartSeq; // ��ʼtable_sequence
        MDB_INT64 m_lEndSeq;// ����table_sequence
        char* m_pFlushList; // ��Ҫˢ�µļ�¼����1wʱ����ˢ�½ڵ����ڴ�������
        char m_cFlushList[MAX_FLUSH_ARRAY_CNT]; // ��Ҫˢ�µļ�¼����1wʱ����ˢ�½ڵ����ڴ�������
        char m_sChangeNotifyName[MAX_NAME_LEN];
        char m_sChangeNotifySeqName[MAX_NAME_LEN];
        char m_sShadowName[MAX_NAME_LEN];

        std::string m_sFlushLogStr; // ��¼ˢ�µļ�¼��Ϣ
        std::string m_sRecdLogStr; // ��ʱ��¼һ����¼��ˢ����Ϣ
    	bool m_bNeedFlush; //�Ƿ���Ҫˢ��
    };


//}
#endif //__MINI_DATABASE_CHANGE_NOTIFY_H__


