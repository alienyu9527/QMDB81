/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepServerCtrl.h
*@Description�� ����ͬ�����ݽ��գ������ݼ���ʱͬ�����ݵķ���
*@Author:		jiang.lili
*@Date��	    2014/03/20
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_REP_SERVER_CTRL_H__
#define __ZX_MINI_DATABASE_REP_SERVER_CTRL_H__


#include "Interface/mdbQuery.h"
#include "Replication/mdbRepNTC.h"
#include "Control/mdbRepCommon.h"
#include "Replication/mdbRepFlushDao.h"
#include "Control/mdbProcCtrl.h"

//namespace QuickMDB
//{
    /**
    * @brief ���ձ���ͬ��������
    * 
    */
    class TRepServerDataRcv:public TMdbNtcBaseObject
    {
    public:
        TRepServerDataRcv();
        ~TRepServerDataRcv();
    public:
        int Init(const char* sDsn, const char* strIP, int iPort);//��ʼ��
        int DealRcvData(const char* sDataBuf, int iBufLen);//������յ�������
        bool IsSame(const char* strIP, int iPort);//�Ƿ�����ͬ�ı���
    private:
        int CombineData(const char* sDataBuf, int iBufLen);
        int GetRecdLen();
        int CheckDataCompletion();
        void SaveRecord();
       
    private:
        TMdbNtcString m_strIP;//����IP
        int m_iPort;//�����˿ں�
        char m_sCurTime[MAX_TIME_LEN]; // ��ǰʱ��
        char m_sLinkTime[MAX_TIME_LEN]; //  ����ʱ��
        bool m_bCheckTime; // �Ƿ�У��ʱ���       
        int m_iMsgBufLen;
        char* m_psMsgBuf;
        char m_sLeftBuf[MAX_REP_SEND_BUF_LEN];//ʣ��buf
        char m_sOneRecord[MAX_VALUE_LEN];//һ����¼

        int m_iDealDataLen;//�Ѿ������Msg����
        int m_iLeftDataLen;//ʣ���Msg����
        int m_iTotalDataLen;//���ݵ��ܳ���

        TRepFlushDAOCtrl * m_ptFlushDaoCtrl;
    };

    /**
    * @brief �򱸻���������ʱ�ļ�������
    * 
    */
    class TRepServerDataSend
    {
    public:
        TRepServerDataSend();
        ~TRepServerDataSend();
    public:
        int Init(const char* sDsn);
        int SendData(int iHostID, const char* sRoutinglist, TMdbPeerInfo* pPeerInfo, bool bIsMemLoad, const char * sRemoteTableInfo);//��������������
        int SendData(const char* sTableName, const char* sRoutinglist, TMdbPeerInfo* pPeerInfo, bool bIsMemLoad, const char * sRemoteTableInfo);//��������������
        int CleanRepFile(int iHostID=MDB_REP_EMPTY_HOST_ID);//ɾ����Ӧ������ͬ���ļ�
    private:
        int DealOneTable(const char* sTableName, const char* sRoutinglist);
		int GetChainSnapshot(TMdbTable * pTable, int * &iPageIDList, int & iPageCounts);
		int ParseRoutingList(const char * sRoutinglist);
		int CheckRemoteTableInfo(TMdbTable * pTable, const char * sRemoteTableInfo);
		int SendMemData(TMdbTable * pTable,  int * iPageIDList, int iPageCount);
        int DealOneTableMem(const char* sTableName, const char* sRoutinglist, const char * sRemoteTableInfo);
        int SendBufData(const char* sBuf, int iLength);//���ͻ������е�����
        bool CheckRecordRoutingId(TMdbTable * pTable, const char * pDataAddr);
		int CalcNullSize(int iColCounts);
		int AdjustNullArea(char * sRecord,int & iLen,TMdbTable * pTable);
		int AdjustMemRecord(char * sRecord, int & iLen, TMdbTable * pTable);
        void GetOneRecord();//����һ����¼
        int GetOneRecordFromMem(TMdbTable * pTable);
        
    private:
		TMdbDSN * m_tDsn;
		
		TMdbTableSpaceCtrl m_tTSCtrl;
		TMdbPageCtrl m_tPageCtrl;
		TMdbVarCharCtrl m_tVarcharCtrl;
		TMdbRowCtrl m_tRowCtrl;
		
		int m_iVarColPos[MAX_COLUMN_COUNTS];
		int m_iVarColCount;
		
		TMdbPage * m_pCurPage;
		char * m_pCurDataAddr;	
		char* m_pNextDataAddr;
		int m_iDataOffset;
		
		int m_iWhichPos;
		unsigned int m_iRowId;
		char m_sVarcharBuf[MAX_BLOB_LEN];
		int m_iValueSize;

		int m_iRouteList[MAX_ROUTER_LIST_LEN];
		int m_iRouteCount;

		TMdbTableChangeOper m_tTableChangeOper[MAX_LOAD_TABLE_COLUMN_DIFF_COUNT];
		int m_iChangeCount;
		TMdbLoadTableRemoteStruct m_tRemoteStruct;
        TMdbDatabase *m_pDataBase;
        TMdbQuery *m_pCurQuery;//��ǰ������ָ��
        TMdbConfig* m_pConfig;
        TMdbRepConfig *m_pRepConfig;
        
       TRepLoadDao *m_pLoadDao;//��ȡ����
       //std::string m_strRoutingList;//��Ҫ���ص�·���б�
       //int m_iHostID;//��Ӧ������ID
       TMdbPeerInfo* m_pPeerInfo;
       char m_sSendBuf[MAX_REP_SEND_BUF_LEN];//���ͻ�����
       char m_sOneRecord[MAX_VALUE_LEN];//һ����¼
       int m_iSendBufLen;
	   int m_iOneRecordLen;
	   char m_sNullFlag[(MAX_COLUMN_COUNTS+MDB_CHAR_SIZE-1)/MDB_CHAR_SIZE];
	   bool m_bColMissAdd;
	   bool m_bNullSizeChange;
	   int m_iNullSizeAdd;
    };

    /**
    * @brief mdbRepServer���̹�����
    * 
    */
    class TRepServer
    {
    public:
        TRepServer();
        ~TRepServer();
        
    public:
        /******************************************************************************
        * ��������	:  Init
        * ��������	:  ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Init(const char *pszDSN); //��ʼ��
        int Start();//����
    private:
        TMdbRepConfig *m_pRepConfig;//�����ļ�
        TMdbRepDataServer *m_pRepDataServer;//���������
        TMdbShmRepMgr *m_pShmMgr;//�����ڴ������
        const TMdbShmRep *m_pShmRep;//�����ڴ�

        TMdbProcCtrl m_tProcCtrl;
        
    };
//}


#endif //__ZX_MINI_DATABASE_SOCKET_REPLICATION_SERVER_H__

