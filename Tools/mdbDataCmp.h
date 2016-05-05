/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbDataCmp.h     
*@Description���Ƚ�����һ̨�����߶�̨���Զ������뱾���Ա������Ƿ���ͬ
*@Author:       jiang.lili
*@Date��        2014��4��
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
//�Ƚ����ݲ�ͬ������
#define  MDB_DIFF_DATA_SAME '0' //������ͬ
#define  MDB_DIFF_DATA_NOT_SAME '1' //���ݲ�ͬ
#define  MDB_DIFF_LOCAL_NOT_EXIST '2' //�Զ��У�����û��
#define  MDB_DIFF_PEER_NOT_EXIST '3' //�����У��Զ�û��
#define  MDB_DIFF_BOTH_NOT_EXIST '4' //�ظ����ʱ�����غͶԶ˶�û��

    //���Ƚϵ�DSN����
    enum MDB_CHECK_DSN_TYPE
    {
        E_DSN_UNKOWN = 0,
        E_DSN_MDB, //mdb
        E_DSN_ORACLE//oracle
    };

    //�߳�״̬
    enum MDB_CHECK_THREAD_STATE
    {
        E_CHECK_THREAD_FREE,
        E_CHECK_THREAD_BUSY,
        E_CHECK_THREAD_OVER,
        E_CHECK_THREAD_ERROR
    };

    //ͨ��stream���صı�
    struct TMdbCheckQdgTable
    {
        std::string m_strTableName;
        std::string m_strLoadSql;
        void Clear(){m_strLoadSql.clear(); m_strTableName.clear();}
    };

    //���Ƚϵ�DSN
    class TMdbDataCheckDSN
    {
    public:
        void Clear();
    public:
        MDB_CHECK_DSN_TYPE m_eType;//���ݿ�����
        std::string m_strDsn;//dsn����
        std::string m_strUser;//�û���
        std::string m_strPassword;//����
        std::string m_strIP;//IP
        int m_iPort;//�˿ں�
        bool m_bAll;//�Ƿ�Ƚ���������
        std::vector<std::string> m_vTables;//���Ƚϵı��б�
        std::vector<TMdbCheckQdgTable> m_vQdgTable;//qdg���б�
        std::string m_strFilterSql; //ƥ���where����
    };

    //������ı���Ϣ
    class TMdbDataCheckTable
    {
    public:
        void Clear();
    public:
        int m_iTableID;//��ID
        std::string m_strTableName;//����
        int m_iRecordNum;//��¼��
        int m_iPkLen;//��������
        bool m_bDealed;//�Ƿ��Ѵ���
        bool m_bIsQdgTable;//�Ƿ���Qdg��
        std::string m_strLoadSql;//qdg���load sql
    };

    //�Ƚ��߳�״̬
    class TMdbCheckThreadState
    {
    public:
        int m_iThreadNo;//�̺߳�
        MDB_CHECK_THREAD_STATE m_eState; //�̵߳�ǰ״̬
        int m_iCurTableID;//��ǰ����ı�ID
        int m_iCurRcdNum;//��ǰ���Ѵ���ļ�¼��
    };

    //�ȽϵĻ�����Ϣ
    class TMdbDataCheckInfo
    {
    public:
        void Clear();
    public:
        std::string m_strDsn;//����DSN����
        int m_iCmpTimes; //�Բ�ͬ���ݵıȽϴ���
        int m_iThreadNum; //�߳�����
        int m_iLogLevel; //��־����
        int m_iInterval;//�Ƚϼ��
        std::string m_strPath;//�ȽϽ���ļ��洢Ŀ¼
    };


    //�����ļ�
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

    //��ͬ�ļ�¼
    struct TMDBDiffRcd
    {
        char cDiffType;//��ͬ������
        char cRecheckSame;//���¼���Ƿ���ͬ
        std::string strPk;//����ֵ�������ŷָ�
    };

    //��ͬ��¼д�ļ���
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

    //������hashд�ļ���
    class TMdbHashFile
    {
    public:
        TMdbHashFile();
        ~TMdbHashFile();
    public:
        int Init(std::string strPath, std::string strTable, int iRcdNum, int iPkLen);//��ʼ��
        int WritePk(std::string strPK); //��¼һ������
        bool IsPkExist(std::string strPK);//���strPK��Ӧ���������ļ����Ƿ����

    private:
        int GetHashValue(std::string strValue);//��ȡ��ϣֵ

    private:
        FILE* m_pFile;//�ļ�ָ��
        int m_iRcdNum;//��¼����
        int m_iPkLen;//������󳤶�
        long long m_lCurMaxPos;//�Ѿ�ռ�õ����λ��
        char* m_pPkBuf;//����buffer
        char m_sPosBuf[MDB_DATA_CHECK_MAX_POS_LEN];//λ�û�����

        static const int m_iDefaultHash; //Ĭ�ϵĹ�ϣ�����������ؼ�¼ֵΪ0���Сʱ��ʹ�á���ֹ���ؼ�¼ȱʧ���Զ˼�¼�϶�ʱ����ͻ�϶�ĵ����
    };

    //���ݱȽ��߳�
    class TMdbCheckThread: public TMdbThreadBase
    {
    public:
        TMdbCheckThread();
        ~TMdbCheckThread();
        int Init(TMdbDataCheckConfig *pCheckConfig, TMdbConfig *pMdbConfig, std::string strPath);
        
        int Start();//�����߳�
        void Stop();//ֹͣ�߳�
        int SetTableToCmp(TMdbDataCheckTable * pTableToCmp);//���ø��̴߳��Ƚϵı�
        int SetTableToRestore(TMdbDataCheckTable * pTableToRestore);//���ø��̴߳��ָ��ı�
    private:
        virtual int svc();
        int CompareOneTable();//�Ƚ�һ�ű�����ݣ��Զ��뱾�رȽ�
        int RestoreOneTable();//�ָ�һ�ű�
        int ConnectDB();//���ӶԶ˺ͱ�������Դ
       
        int CmpOraTable();//��oracle�еı�Ƚ�
        int CmpMdbTable();//��mdb�еı�Ƚ�
        int CheckLocalData();//��鱾�������Ƿ�������
        int ReCompare();//�Բ�ͬ�����ظ��Ƚ�
        void ClearCmpResult();//�����αȽϽ��

        //��������sql
        int SetInsertSQL();//���ûָ�����ʱ�Ĳ���SQL
        int SetUpdateSQL();//���ûָ�����ʱ�ĸ���SQL
        int SetDeleteSQL();//���ûָ�����ʱ�Ĳ���SQL
        int SetSelectByPKSQL();//���ð�������ѯsql
        int SetSelectAllSQL(); //���ò�ѯ��������sql
        int SetOraSelectAllSQL();//���ô�oracle��ѯ�������ݵ�sql
        int SetOraSelectByPKSQL();//����oracle2mdb����qdg���ĸ���������ȡ��¼sql

        int SetLocalKeyParam(TMdbClientQuery *pPeerQuery, TMdbQuery *pLocalQuery);//��һ�αȽ�ʱ������Queryָ��SQL����
        int SetLocalKeyParam(TMDBDBQueryInterface *pPeerQuery, TMdbQuery *pLocalQuery);//��һ�αȽ�ʱ������Queryָ��SQL����

        int SetLocalParam(TMdbClientQuery *pPeerQuery, TMdbQuery* pLocalQuery);
        int SetLocalParam(TMDBDBQueryInterface *pPeerQuery, TMdbQuery* pLocalQuery);

        int SaveRecord(TMdbClientQuery *pPeerQuery, int iDiffType = MDB_DIFF_DATA_SAME);// ����Զ�����һ����¼������������ͬ��¼
        int SaveRecord(TMDBDBQueryInterface *pOraQuery, int iDiffType = MDB_DIFF_DATA_SAME);//����oracle��һ����¼������������ͬ��¼

        int SaveDiffRcd(std::string strPk, int iDiffType);//����һ����ͬ��¼

        int CompareOneMdbRcd(TMdbClientQuery *pPeerQuery, TMdbQuery *pLocalQuery, bool &bSame);//�Ƚ�����queryָ���Ӧ�ļ�¼�Ƿ���ͬ
        int CompareOneOraRcd(TMDBDBQueryInterface *pOraQuery, TMdbQuery *pLocalQuery, bool &bSame);//��oracle�Ƚ�һ����¼�Ƿ���ͬ

        int ReCompareWithOra();
        int ReCompareWithPeer();

        ///////////////////���ݻָ�
        bool CheckColumIsPK(const int iColumnIdx);

        int RestoreDataFromPeer();
        int RestoreDataFromOra();
        int RestoreDataToOra();

        //�ӶԶ�mdb�ָ�
        int SetPeesPKParam(std::string strPks);
        int SetRestoreLocalParam(TMdbClientQuery *pClientQuery, TMdbQuery* pLocalQuery, E_QUERY_TYPE eSqlType);
        int SetDeleteLocalParam(TMdbQuery* pLocalQuery, std::string strPks);

        //��oracle�ָ�
        int SetOraPKParam(std::string strPks);
        int SetRestoreLocalParam(TMDBDBQueryInterface *pOraQuery, TMdbQuery* pLocalQuery, E_QUERY_TYPE eSqlType);

        //��oracle�ָ�
        int SetLocalPKParam(std::string strPks);
        int SetRestoreOraParam(TMdbQuery *pOraQuery, TMDBDBQueryInterface* pLocalQuery, E_QUERY_TYPE eSqlType);
        int SetDeleteOraParam(TMDBDBQueryInterface* pOraQuery, std::string strPks);

        //mdb���Load sql�������и��Ӳ���
        void SetCompexParam(TMDBDBQueryInterface *pOraQuery);

    public:
        int m_iThreadNO;//�̺߳�
        
        MDB_CHECK_THREAD_STATE m_eState;//�߳�״̬
    private:
        TMdbDataCheckTable * m_pTableToCmp;//���Ƚϵı�
        TMdbDataCheckTable* m_pTableToRestore;//���ָ��ı�
        bool m_bDiffExist;//�Ƿ���ڲ�ͬ
        TMdbDataCheckConfig * m_pCheckConfig;
        TMdbConfig * m_pMdbConfig;
        std::string m_strPath;//�ļ�·��

        TMdbDatabase *m_pLocalMdb;//����mdb
        TMdbClientDatabase *m_pPeerMdb;//�Զ�mdb
        TMDBDBInterface *m_pOraDB;//oracle

        TMdbQuery *m_pLocalQuery;
        TMdbClientQuery *m_pPeerQuery;
        TMDBDBQueryInterface *m_pOraQuery;
    private:
        TMdbHashFile *m_pPkFile;
        TMdbDiffFile * m_pDiffFile;

        //����sql
        char m_sMdbSelectAllSQL[MAX_SQL_LEN];
        char m_sOraSelectAllSQL[MAX_SQL_LEN*2];//��oracle�м������ݵ�sql
        char m_sMdbSelectByPkSQL[MAX_SQL_LEN];
        char m_sOraSelectByPkSQL[MAX_SQL_LEN*2];//��oracle�а�ס���������ݵ�sql
        bool m_bOraSelectNoBlob;//select����в�����blob
        char m_sUpdateSQL[MAX_SQL_LEN];
        char m_sInsertSQL[MAX_SQL_LEN];
        char m_sDeleteSQL[MAX_SQL_LEN];

        std::string m_strPK;
        TMdbTable *m_pTable;
        int m_iDiffRcd;

        bool m_bExit;//�Ƿ��˳�

    };

    //���ݱȽϹ�����
    class TMdbCheckDataMgr
    {
    public: 
        TMdbCheckDataMgr();
        ~TMdbCheckDataMgr();
    public:
        int Init(TMdbDataCheckConfig *pConfig);//��ʼ��
        int Start();//��ʼ�Ƚ�
    private: 
        int StatTables();//ͳ�Ʊ���Ϣ������m_vTables
        int CreateThreads();//�����Ƚ����߳�
        int CompareTables();//��̬������������߳�
        int RestoreData();//�ָ�����
        int StopThreads();//ֹͣ�������߳�

        int GenTableList(std::vector<TMdbDataCheckTable>& vTmpTable);//���������������m_vTables
        bool CheckRepAttr(TMdbTable * pTable, TMdbCheckQdgTable* &pQdg);//�����ͬ������
        int StatOneTable(TMdbTable * pTable, std::vector<TMdbDataCheckTable>& vTmpTable, TMdbCheckQdgTable* pQdg=NULL);//ͳ��һ�ű�ļ�¼���������浽vTmpTable��

        TMdbDataCheckTable* GetCmpTable(std::string strTableName);
public:
    static bool m_bDetail;//�Ƿ�����Ƚϵ���ϸ��Ϣ
    private:
        TMdbDataCheckConfig *m_pCheckConfig;
        TMdbConfig *m_pConfig;

        std::vector<TMdbDataCheckTable> m_vTables;//������ı�
        std::vector<TMdbCheckThread*>   m_vThreads;//���߳�
        std::vector<TMdbCheckThreadState> m_vStateTable;//���߳�״̬��

        int m_iDealedTable;//�ѷ���ı����
        int m_iThreadNum;

        TMdbDatabase * m_pDatabase;
        TMdbQuery * m_pQuery;
        std::string m_strPath;//�ļ���Ŀ¼
    };


//}
#endif