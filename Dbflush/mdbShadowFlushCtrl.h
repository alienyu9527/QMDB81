/****************************************************************************************
*@Copyrights  2011�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QMDB�Ŷ�
*@                   All rights reserved.
*@Name��        mdbShadowFlushCtrl.h
*@Description�� ����Ӱ�ӱ�ģʽ��ORCLE2MDBͬ��
*@Author:       li.ming
*@Date��        2013��12��20��
*@History:
******************************************************************************************/
#ifndef _MDB_SHADOW_FLUSH_CTRL_H_
#define _MDB_SHADOW_FLUSH_CTRL_H_

#include "Dbflush/mdbDAOBase.h"
#include "Dbflush/mdbShaowServerCtrl.h"

//namespace QuickMDB{



#define MAX_KEY_LEN   128
#define MAX_REP_PREFETCH_ROWS 10000

    class TShadowRecord
    {
    public:
        TShadowRecord();
        ~TShadowRecord();

        void Clear();
        void AddData( TMDBDBFieldInterface& tField,int iLen, const MDB_INT64& iSeq = -1);
        void AddDelData( TMDBDBFieldInterface& tField,int iLen);

    public:
        char m_sTabName[MAX_NAME_LEN];
        TMdbData m_tDelData;
        TMdbData m_aData[MAX_COLUMN_COUNTS];
        int m_iColmCnt;   
        
    };


    class TTableDAO
    {
    public:
        TTableDAO();
        ~TTableDAO();
        
        void Clear();
       
        char m_sTabName[MAX_NAME_LEN];
        TMdbDAOBase* m_pInsertDAO; 
        TMdbDAOBase* m_pDelDAO;    
    };


    class TShadowDAO
    {
    public:
        TShadowDAO();
        ~TShadowDAO();
        
        int Init(const char* psDsn,const char* psUid, const char* psPwd, const char* psShadowName,const char* psNotifyTabName);
        int Execute( TShadowRecord& tOneRecord);
        int Commit();

        void Clear();
        
        void ClearDAOByTableName(const char* psTabName);

    private:   
        int CreateDAO(const char* psTabName);
        int FindDAO(const char* psTabName);
        
        void GenInsertSql(const char* psTabName, char*  psSql, int isqllen);
        void GenDeleteSql(const char* psTabName, char*  psSql, int isqllen);

    private:
        char m_sDSN[MAX_NAME_LEN]; //DSN
        char m_sUID[MAX_NAME_LEN]; //UID
        char m_sPWD[MAX_NAME_LEN]; //PWD
       
        TMDBDBInterface* m_pDBLink;   //����
        TMDBDBQueryInterface *m_pQuery;       //�����ύ
        TShadowRecord* m_pCurRecd; 

        char m_sShadowTabName[MAX_NAME_LEN];
        char m_sNotifyTabName[MAX_NAME_LEN];

        int m_iCurDaoPos; 
        TTableDAO m_vTabDAO[MAX_TABLE_COUNTS]; 
    };


    class TTableNotifyInfo
    {
    public:
        TTableNotifyInfo();
        ~TTableNotifyInfo();

        int Init(const char* psNotifyName  /*%dsn%_mdb_change_notif or mdb_change_notif*/
                     ,const char* psNotifySeqName  /*%dsn%_mdb_change_notify_seq or mdb_change_notify_seq*/
                     ,const char* psShadowTabName
                     ,const char* psTableName
                     ,TMDBDBQueryInterface* pOraQry);

    private:    
        
        MDB_INT64 GetLastSeq(const char* psNotifyName  /*%dsn%_mdb_change_notif or mdb_change_notif*/
                                        ,const char* psNotifySeqName /*%dsn%_mdb_change_notify_seq or mdb_change_notify_seq*/
                                        ,const char* psShadowTabName
                                        ,const char* psTableName
                                        ,TMDBDBQueryInterface* pOraQry);

        MDB_INT64 GetMaxSeqFromShadowTab(const char* psTableName,const char* psShadowTabName,TMDBDBQueryInterface* pOraQry);
        
        MDB_INT64 GetSeqFromNotifySeqTab(const char* psTableName, const char* psNotifySeqName,TMDBDBQueryInterface* pOraQry);
        
        MDB_INT64 GetMinSeqFromNotifyTab(const char* psTableName,const char* psNotifyName,TMDBDBQueryInterface* pOraQry);

    public:
        char m_sTabName[MAX_NAME_LEN];
        MDB_INT64 m_iLastSeq;
        char m_sSql[MAX_SQL_LEN];
    };


    class TShadowFlushCtrl
    {
    public:
        
        TShadowFlushCtrl();
        ~TShadowFlushCtrl();

        // ��ʼ��
        int Init(const char* psDsnName);

        /******************************************************************************
        * ��������	:  Run()
        * ��������	:  ��%DSN%_MDB_CHANGE_NOTIF�м�¼����Ӱ�ӱ�QMDB_SHADOW_%DSN%
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1;
        * ����		:  li.ming
        *******************************************************************************/
        int Run();

    private:

        //���� ����ÿ��
        int DealTable( TTableNotifyInfo* pTable);

        // ��ȡ��ǰchange_notif���м�¼��
        MDB_INT64 GetChangeNotifyCnt();
        
        int InitDao();

        // �������ݿ�
        int ConnectOracle();

        // ��ȡ���ˢ����Ϣ
        TTableNotifyInfo* GetTableNotifyInfo(const char* psTabName);

        // ��ȡchange_notif������(%dsn%_mdb_change_notif or mdb_change_notif)
        int GetNotifyTabName(const char* sMdbDsn);    

        // ������Ҫˢ�µı����Ϣ
        int AddNewNotfyTab(const char* psTabName);

        // �ύDAO
        int DaoCommit(TTableNotifyInfo* pTableInfo, const MDB_INT64 iLastSeq);
        
    private:
        std::vector<TTableNotifyInfo*> m_vpTabNotifyInfo; // ����ORACLE2MDB���ȡ %DSN%_MDB_CHANGE_NOTIF�м�¼SQL�Լ�Ӱ�ӱ����seq��Ϣ
        TMDBDBInterface* m_pOraLink; // oracle ����
        TMDBDBQueryInterface* m_pOraQry; // oracle �������
        TMDBDBQueryInterface* m_pSelectQry; // oracle �������

        TShadowDAO m_tDao; // DAO��ʽ����¼����Ӱ�ӱ�

        TTableNotifyInfo* m_pCurTabInfo; // ��ǰˢ�±��ˢ����Ϣ

        char m_sNotifyTabName[MAX_NAME_LEN]; //  change_notify ������(%dsn%_mdb_change_notif or mdb_change_notif)
        char m_sNotifySeqTabName[MAX_NAME_LEN]; //  change_notify_seq ������(%dsn%_mdb_change_notify_seq or mdb_change_notify_seq)
        char m_sShadowTabName[MAX_NAME_LEN];  // Ӱ�ӱ�����(QMDB_SHADOW_%DSN%)

        TMdbShadowSvrCtrl m_tSvrCtrl; 
        TShadowCfgNode m_tCfgInfo;
        TShmMonitorInfo* m_pSvrMgr;
    };


//}
#endif
