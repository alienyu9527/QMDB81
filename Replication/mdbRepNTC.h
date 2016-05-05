/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepNTC.h		
*@Description: ��װʹ��NTCͨ�ŵ������
*@Author:		jiang.lili
*@Date��	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_NTC_H__
#define __ZTE_MINI_DATABASE_REP_NTC_H__
//#include "BillingNTC.h"
#include "Control/mdbRepCommon.h"
#include "Replication/mdbRepFileParser.h"
#include "Replication/mdbRepFlushDao.h"
using std::endl;
//using namespace QuickMDB;

//namespace QuickMDB
//{
    class TMdbLoadDataTool;

#define HEARTBEAT_TIME_OUT 30//������ʱĬ��ʱ��
    
    enum eRepConnState
    {
        E_REP_NTC_CLOSED = 0,  ///���Ӳ��ɹ�
        E_REP_NTC_OK           ///���ӳɹ�
    };

    enum eRepNtcEvent
    {
        E_NEW_CONNECTION=1000, //�µ�����
        E_CONNECTION_CLOSE, //���ӹر�
        E_MDB_REGIST = 2000, //QuickMDBע����Ϣ
        E_MDB_REGIST_ANS,
        E_MDB_ROUTING_REQUEST, //·������
        E_MDB_ROUTING_REQUEST_ANS,
        E_MDB_HEARTBEAT, //QuickMDB����
        E_MDB_STAT_CHANGE,  //״̬�仯
        E_REP_STAT_CHANGE=3000, //����֪ͨmdb״̬�仯
        E_REP_SYNC_CONFIG_REQUEST, //��������ͬ����Ϣ
        E_REP_SYNC_CONFIG_REQUEST_ANS,
        E_REP_SYNC_STATE_REQUEST,//��������ͬ��״̬��Ϣ
        E_REP_SYNC_STATE_REQUEST_ANS,
        E_REP_ROUTING_CHANGE,//����֪ͨ·�ɱ��
        E_SVR_ROUTING_CHANGE=4000, //·�ɱ��
        E_SVR_ROUTING_INFO, //·����Ϣ
        E_SVR_ROUTING_INOF_ANS,
        E_LOAD_FROM_REP = 5000,//�ӱ���������������
        E_LOAD_FROM_REP_ANS,
        E_SEND_REP_DATA,//����ͬ������
        E_CLEAN_REP_FILE,//���ͬ���ļ�������1.2�汾��
        E_HEART_BEAT = 6000,//������
        E_REP_NTC_UNKNOWN
    };

    enum eQueryType
    {
        E_QUERY_HOST=1000,
        E_QUERY_ROUTING,
        E_QUERY_GROUP
    };

    struct TRepNTCMsg
    {
        eRepNtcEvent eEvent;
        int iMsgLen;  
        const char *psMsg;     
    public:
        TRepNTCMsg()
        {
            eEvent = E_REP_NTC_UNKNOWN;
            iMsgLen = 0;
            psMsg = NULL;
        }
        const char*ToString()
        {
            std::ostringstream   ostrOut;

            ostrOut 
                <<"Msg:"
                <<"Event  = "<<eEvent<<", "
                <<"MsgLen = "<<iMsgLen<<", "
                <<"Msg = "<<(psMsg==NULL? "NULL":psMsg)<<endl;
            return ostrOut.str().c_str();
        }
    };

    /**
    * @brief NTCͬ���ͻ��˷�װ
    * 
    */
    class TMdbRepNTCClient: public /*QuickMDB::*/TMdbSyncEngine
    {
    public:
        TMdbRepNTCClient();
        ~TMdbRepNTCClient();
    public:
        /******************************************************************************
        * ��������	:  Connect
        * ��������	: ���ӷ����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool Connect(const char* pszIP, int iPort, int iTimeOut = 3000);

        /******************************************************************************
        * ��������	:  SendPacket
        * ��������	: �����˷�����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool SendPacket(eRepNtcEvent eEvent, const char* psBodyBuffer = "", int iLength=-1);

        /******************************************************************************
        * ��������	:  SendHearbeat
        * ��������	: �����˷���������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool SendHearbeat();

        /******************************************************************************
        * ��������	:  GetMsg
        * ��������	: �ӷ���˻�ȡ��Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetMsg(TRepNTCMsg& tMsg);

        /******************************************************************************
        * ��������	:  Disconnect
        * ��������	: �Ͽ������˵�����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        void Disconnect();

        /******************************************************************************
        * ��������	:  NeedReconnect
        * ��������	: �Ƿ���Ҫ��������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool NeedReconnect();
    protected:
        //virtual bool OnConnect(/*QuickMDB::*/TMdbConnectEvent* pEventInfo, /*QuickMDB::*/TMdbEventPump* pEventPump);
        //virtual bool OnDisconnect(/*QuickMDB::*/TMdbDisconnectEvent* pEventInfo, /*QuickMDB::*/TMdbEventPump* pEventPump);
        //virtual bool OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump);

    public:
        eRepConnState m_eState;
    protected:
        int m_iHostID;
        int m_iHeartbeat;
        TMdbSharedPtr<TMdbPeerInfo>  m_pPeerInfo;
        TMdbWinntTcpHelper *m_pTcpHelper;        
        TMdbRecvMsgEvent* m_pMsgEvent;

        int m_iTimeOut;        
        time_t m_tConnStateTime;
    };

    /**
    * @brief ���÷���ͻ���
    * 
    */
    class TMdbRepMonitorClient: public TMdbRepNTCClient
    {
    public: 
        TMdbRepMonitorClient();
        ~TMdbRepMonitorClient();
    public:
        /******************************************************************************
        * ��������	:  Register
        * ��������	:  �����÷���ע��
        * ����		:  
        * ���		:  iHostID ���������÷���˶�Ӧ��host_ID
        * ���		:  iHeartbeat �������
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Register(const char* sIP, int iPort, int& iHostID, int& iHeartbeat);

        /******************************************************************************
        * ��������	:  RoutingRequest
        * ��������	:  �����÷�������·����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int RoutingRequest(int iHostID, TMdbShmRepMgr *pShmRepMgr);

        // ֻ�����÷�������·����Ϣ����д�빲���ڴ�
        int RoutingRequestNoShm(int iHostID);

        /******************************************************************************
        * ��������	:  ReportState
        * ��������	:  �����÷�������·����Ϣ
        * ����		:  iHostID ���������÷���˶�Ӧ��Host_ID
        * ����		:  eState ������״̬
        * ���		:  
        * ����ֵ	:  0 - �ɹ�! ��0-ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int ReportState(int iHostID, EMdbRepState eState);

        const char* GetSvrConfigMsg();

    private:
        char m_sSvrMsg[MAX_REP_SEND_BUF_LEN];
    };


    /**
    * @brief ��Ƭ����ͬ�����ݷ��Ͷ�client
    * 
    */
    class TMdbRepDataClient:public /*QuickMDB::*/TMdbSyncEngine
    {
    public:
        TMdbRepDataClient();
        ~TMdbRepDataClient();

    public:
        /******************************************************************************
        * ��������	:  Init
        * ��������	:  ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Init(TMdbConfig* pMdbCfg);

        /******************************************************************************
        * ��������	:  Connect
        * ��������	: ���ӷ����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool Connect(const char* pszIP, int iPort, int iTimeOut = 3000);

        /******************************************************************************
        * ��������	:  SendPacket
        * ��������	: �����˷�����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool SendPacket(eRepNtcEvent eEvent, const char* psBodyBuffer = "", int iLength=-1);

        /******************************************************************************
        * ��������	:  SendHearbeat
        * ��������	: �����˷���������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool SendHearbeat();

        /******************************************************************************
        * ��������	:  GetMsg
        * ��������	: �ӷ���˻�ȡ��Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetMsg(/*QuickMDB::*/TMdbMsgInfo* &pMsg);

        /******************************************************************************
        * ��������	:  Disconnect
        * ��������	: �Ͽ������˵�����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        void Disconnect();

        /******************************************************************************
        * ��������	:  NeedReconnect
        * ��������	: �Ƿ���Ҫ��������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool NeedReconnect();

        /******************************************************************************
        * ��������	:  SetHostID
        * ��������	:  ���ÿͻ��˶�Ӧ��hostID
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SetHostID(int iLocHostID, int iRepHostID);
        
        /* ��������	:  GetHostID
        * ��������	:  ��ȡ�ͻ��˶�Ӧ��hostID
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetHostID();

        /******************************************************************************
        * ��������	:  SendData
        * ��������	: ���ļ����ݷ������Զ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ� !0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/ 
        int SendData(const char* sFileName);

        /******************************************************************************
        * ��������	:  LoadData
        * ��������	: �ӶԶ˼�������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ�� 
        * ����		:  jiang.lili
        *******************************************************************************/
        int LoadData(const char* sRoutinglist);

        /******************************************************************************
        * ��������	:  SendCleanCmd
        * ��������	: ֪ͨ�������ͬ���ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ�� 
        * ����		:  jiang.lili
        *******************************************************************************/
        int SendCleanCmd();

        /******************************************************************************
        * ��������	:  GetIP
        * ��������	:  ��ȡ�Զ�IP
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        const char* GetIP()
        {
            return m_strIP.c_str();
        }
        /******************************************************************************
        * ��������	:  GetPort
        * ��������	:  ��ȡ�Զ˶˿ں�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetPort()
        {
            return m_iPort;
        }

    private:
        int LoadOneTable(const char* sTableName, const char* sRoutinglist);

    public:
        eRepConnState m_eState;

    protected:
        int m_iHeartbeat;
        TMdbSharedPtr<TMdbPeerInfo> m_pPeerInfo; 
        /*QuickMDB::*/TMdbRecvMsgEvent* m_pMsgEvent;

        int m_iTimeOut;        
        time_t m_tConnStateTime;

    private:
        std::string m_strDsn;
        TMdbLoadDataTool * m_pLoadDataTool;
        int m_iRepHostID;
        int m_iLocHostID;
        char m_sSendBuf[MAX_REP_SEND_BUF_LEN];
        int m_iBufLen;
        TMdbRepFileParser *m_ptFileParser;//�ļ�����
        TMdbRepFileStat *m_ptRepFileStat;//�ļ���¼����ͳ��
        std::string m_strIP;//�Զ�IP
        int m_iPort;//�Զ˶˿ں�
    };

    /**
    * @brief server�˷�װ
    * 
    */	
    class TMdbRepNTCServer:public /*QuickMDB::*/TMdbNtcEngine
    {
    public:
        TMdbRepNTCServer();
        ~TMdbRepNTCServer();
    public:
        bool Start(const char* sIP, int iPort, int iMaxThread, int iHeartbeatWarning = 30);

    protected:
        virtual bool OnConnect(TMdbConnectEvent* pEventInfo, TMdbEventPump* pEventPump);
        virtual bool OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump);
        virtual bool OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump);
        virtual bool OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump);

    protected:
        int m_iHeartbeatWarning;//�ͻ���������ʱʱ��
        int m_iWorkThreadNum;//�����߳�����

    };

    class TRepServerDataRcv;

    /**
    * @brief ��Ƭ����ͬ�����ݽ��ն�server
    * 
    */
    class TMdbRepDataServer:public /*QuickMDB::*/TMdbNtcEngine
    {
    public:
        TMdbRepDataServer(const char* sDsn);
        ~TMdbRepDataServer();
 
        bool Start(const char* sIP, int iPort, int iMaxThread, int iHeartbeatWarning = 30);
   protected:
        virtual bool OnConnect(TMdbConnectEvent* pEventInfo, TMdbEventPump* pEventPump);
        virtual bool OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump);
        virtual bool OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump);
        virtual bool OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump);
    public:

    private:
        //int DealEvent(TMdbRecvMsgEvent* pEventInfo);
        int DealLoadRequest(const char* pData, /*QuickMDB::*/TMdbPeerInfo* pPeerInfo);//���ݼ�������
        int DealCleanFile(const char* pData);//���ͬ���ļ�
        int DealRcvData(const char* pData, int iLength, /*QuickMDB::*/TMdbPeerInfo* pPeerInfo);//����ͬ������

        TRepServerDataRcv *GetRcvEngine(const char* strIP, int iPort);
    protected:
        int m_iHeartbeatWarning;//�ͻ���������ʱʱ��
        int m_iWorkThreadNum;//�����߳�����
    private:
        /*QuickMDB::*/TMdbNtcAutoArray m_arRcvEngine;//���ݽ�������
        /*QuickMDB::*/TMdbNtcString m_strDsn;//DSN����
    };

    class TMdbLoadDataTool
    {
    public:
        TMdbLoadDataTool();
        ~TMdbLoadDataTool();
    public:
        int Init(TMdbConfig* pMdbCfg);
        int UploadData(const char* pDataBuf, int iLen, bool &bOver);
    private:
        void CombineData(const char* sDataBuf, int iBufLen);
        int DealWithMsgBuf();
        void SaveRecord(int iCurPos);
        int InitSQL(TMdbTable* pTable);
        int GetOneRecord(int iNextPos);
        int Execute();
        bool IsNULL(const char* sSrc);
    private:
        TMdbConfig* m_pMdbCfg;
        TMdbDatabase *m_pDataBase;
        TMdbQuery *m_pCurQuery;

        bool m_bDataOver;

        char* m_psMesgBuf;
        int m_iMsgBufLen;
        char m_sResidueBuf[MAX_REP_SEND_BUF_LEN];
        int m_iResidueLen;

        int m_iMsgLen;
        int m_iCurPos;

        char m_sTempRecord[MAX_VALUE_LEN];
        //int  m_iTempPos;

        std::vector<std::string> m_vParam;
        char m_paramValue[MAX_VALUE_LEN];
        TRepLoadDao * m_pLoadDao;
		std::string strTableName;
    };

//}
#endif
