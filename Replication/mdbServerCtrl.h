/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbServerCtrl.h		
*@Description�� ���÷��������
*@Author:		jiang.lili
*@Date��	    2014/03/20
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_SERVER_H__
#define __ZTE_MINI_DATABASE_SERVER_H__
#include "Replication/mdbServerConfig.h"
#include "Replication/mdbRepNTC.h"
#include "Control/mdbRepCommon.h"

//namespace QuickMDB
//{
    class TMdbStatMonitor;

#define  MDB_SERVER_SHM_NAME "mdbServerShm"
    struct TMDBServerShm
    {
    public:
        bool bExit;//�Ƿ���Ҫ�˳�
        bool bIsRepExist;//�����Ƿ����
        char sStartTime[MAX_TIME_LEN];//����ʱ��
        char sRepCnnTime[MAX_TIME_LEN];//���������ϵ�ʱ��
        int iPid;//���̵�PID
    };
    /**
    * @brief mdb����ͬ������
    * 
    */	
    class TMdbServerClient:public TMdbRepNTCClient
    {
    public: 
        TMdbServerClient();
        ~TMdbServerClient();
    public:
        /******************************************************************************
        * ��������	:  Init
        * ��������	: ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Init(TMdbServerConfig * pConfig, TMdbStatMonitor *pMonitor);

        /******************************************************************************
        * ��������	:  Connect
        * ��������	: ���ӱ���
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool Connect();

        /******************************************************************************
        * ��������	:  IsConnected
        * ��������	: �Ƿ��뱸�����ӳɹ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool IsConnected()
        {
            return m_eState == E_REP_NTC_OK;
        }

         /******************************************************************************
        * ��������	:  IsRepExist
        * ��������	: �����Ƿ����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        bool IsRepExist()
        {
            return m_bRepExist;
        }

        /******************************************************************************
        * ��������	:  SyncCfgFile
        * ��������	: ͬ�������ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SyncLocalCfgFile();

        /******************************************************************************
        * ��������	:  SyncLocalMdbState
        * ��������	: ͬ������MDB״̬
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SyncLocalMdbState();

        /******************************************************************************
        * ��������	:  SendRouterChange
        * ��������	: ��·�ɱ�����͸�����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SendRouterChange(const char* sMsg);

        /******************************************************************************
        * ��������	:  SendStatChange
        * ��������	: ����״̬�����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SendStatChange(const char* sMsg);

        /******************************************************************************
        * ��������	:  SendStatChange
        * ��������	: ����״̬�����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SendStatChange(int iHostID, EMdbRepState eState);

        /******************************************************************************
        * ��������	:  DealRepServerCmd
        * ��������	: �������÷��񱸻�������ͬ����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int DealRepServerCmd(TRepNTCMsg& tCmd);
    private:
        std::string m_sRepServerIP;//���÷��񱸻�IP
        int m_iRepPort;//���÷��񱸻��˿ں�
        bool m_bRepExist;//���÷��񱸻��Ƿ����
        TMdbServerConfig * m_pConfig;//�����ļ�
        TMdbStatMonitor *m_PMonitor;//״̬���

    };

    /**
    * @brief mdb״̬�����
    * 
    */	
    class TMdbStatMonitor
    {
    public: 
        TMdbStatMonitor();
        ~TMdbStatMonitor();
    public:
        /******************************************************************************
        * ��������	:  Init
        * ��������	: ��ʼ��״̬�����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Init(TMdbServerConfig *pConfig);

        /******************************************************************************
        * ��������	:  SyncState
        * ��������	:  ͬ��MDB״̬
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SyncState(const char* pData);

        /******************************************************************************
        * ��������	:  GetStateData
        * ��������	:  ��ȡ����MDB״̬
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetStateData(char*pDataBuf, int iBufLen);

        /******************************************************************************
        * ��������	:  AddMDB
        * ��������	: ����һ��QuickMDB��Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int AddMDB(TMdbRepState &tMdb);

        /******************************************************************************
        * ��������	:  RemoveMDB
        * ��������	: ɾ��һ��QuickMDB��Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int RemoveMDB(int iHostID);

        /******************************************************************************
        * ��������	:  UpdateState
        * ��������	: ����QuickMDB״̬
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int UpdateState(int iHostID, EMdbRepState eState);

        /******************************************************************************
        * ��������	:  GetRoutingInfo
        * ��������	: ��ȡ·����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetRoutingInfo(int iRoutingID, char* sData);

        /******************************************************************************
        * ��������	:  GetHostInfo
        * ��������	: ��ȡ������Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetHostInfo(int iHostID, char* sData);

         /******************************************************************************
        * ��������	:  GetGroupInfo
        * ��������	: ��ȡ����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetGroupInfo(int iGroupID, char* sData);

         /******************************************************************************
        * ��������	:  GetHostState
        * ��������	: ��ȡ����״̬
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        EMdbRepState GetHostState(int iHostID);//��ȡ����״̬

    private:
        int GetOneGroup(TMdbGroup& tGroup, char *sData);
        int GetHostState(int iHostID, char* sState);//��������ID����ȡ����״̬
        
    private:
        TMdbServerConfig *m_pServerConfig;
        std::map<int, TMdbRepState> m_tMdbState;
    };

    /**
    * @brief ���÷�������ͨ�ŷ����
    * 
    */	
    class TMdbServerEngine:public TMdbRepNTCServer
    {
    public:
        TMdbServerEngine();
        ~TMdbServerEngine();
    public:
        int Init(TMdbStatMonitor *pMonitor, TMdbServerClient* pClient, TMdbServerConfig *pConfig);

    protected:        
        virtual bool OnConnect(TMdbConnectEvent* pEventInfo, TMdbEventPump* pEventPump);
        virtual bool OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump);
        virtual bool OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump);
        //virtual bool OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump);

    protected:
        int SendPacket(/*QuickMDB::*/TMdbPeerInfo* pPeerInfo, const char* pcBodyBuffer);
    private:
        int DealEvent(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg);

        int DealRegist(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg);//����ע����Ϣ

        int DealStateChange(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg, bool IsRep = true);//����״̬�����Ϣ

        int DealRoutingRequest(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg);//����·��������Ϣ

        int DealRoutingChange(TMdbWinntTcpMsg* pWinntTcpMsg, bool IsRep);//����·�ɱ����Ϣ

        int DealRoutingQuery(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg);//����·����Ϣ��ѯ

        int DealRepConfigSync(TMdbWinntTcpHelper* pWinntcpHelper);//���� ��������ʱ��ͬ�������ļ���Ϣ����

        int DealRepStateSync(TMdbWinntTcpHelper* pWinntcpHelper);//���� ��������ʱ��ͬ��״̬��Ϣ����

        int SetHostID(const char* sIP, int iPort, int iHostID);//���ø����Ӷ�Ӧ��������ʶ
        int GetHostID(const char* sIP, int iPort);//����IP��ȡ��Ӧ��������ʶ
        int RemoveClient(const char* sIP, int iPort);

    private:
        TMdbServerClient * m_pClient;
        TMdbStatMonitor * m_pMonitor;
        TMdbServerConfig* m_pConfig;
        std::vector<TMdbRepHost> m_arClientHost;//������mdbServer���ӵĿͻ��ˣ��洢�ͻ���������ID�Ķ�Ӧ��ϵ�������쳣ʱ���ҵ���Ӧ����
    };

    /**
    * @brief ���÷��������
    * 
    */	
    class TMdbServerCenter
    {
    public:
        TMdbServerCenter();
        ~TMdbServerCenter();
    public:
        /******************************************************************************
        * ��������	:  Init
        * ��������	: ��ʼ������ȡ·����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Init();

        /******************************************************************************
        * ��������	:  Start
        * ��������	: �����������ȴ��ʹ�������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Start();

        /******************************************************************************
        * ��������	:  Stop
        * ��������	:  ֹͣMDBServer����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Stop();

    private:

    private:
        TMdbServerConfig *m_pConfig;//�����ļ�ָ��
        TMdbServerEngine * m_pCfgServer; //·�ɼ�״̬��Ϣ��ط������߳�
        TMdbServerClient * m_pRepClient; //���÷�������ͬ���ͻ���
        TMdbStatMonitor* m_pStatMonitor;//״̬����࣬ά�����ӵĸ���QuickMDB״̬
        TMDBServerShm *m_pServerShm;//mdbServer�����Ϣ�����ڴ�
        TMdbNtcShareMem *m_pShmMgr;
        
        
    };

//}
#endif


