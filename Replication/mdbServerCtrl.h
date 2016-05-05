/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbServerCtrl.h		
*@Description： 配置服务管理类
*@Author:		jiang.lili
*@Date：	    2014/03/20
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
        bool bExit;//是否需要退出
        bool bIsRepExist;//备机是否存在
        char sStartTime[MAX_TIME_LEN];//启动时间
        char sRepCnnTime[MAX_TIME_LEN];//备机连接上的时间
        int iPid;//进程的PID
    };
    /**
    * @brief mdb主备同步引擎
    * 
    */	
    class TMdbServerClient:public TMdbRepNTCClient
    {
    public: 
        TMdbServerClient();
        ~TMdbServerClient();
    public:
        /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	: 初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Init(TMdbServerConfig * pConfig, TMdbStatMonitor *pMonitor);

        /******************************************************************************
        * 函数名称	:  Connect
        * 函数描述	: 连接备机
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool Connect();

        /******************************************************************************
        * 函数名称	:  IsConnected
        * 函数描述	: 是否与备机连接成功
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool IsConnected()
        {
            return m_eState == E_REP_NTC_OK;
        }

         /******************************************************************************
        * 函数名称	:  IsRepExist
        * 函数描述	: 备机是否存在
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool IsRepExist()
        {
            return m_bRepExist;
        }

        /******************************************************************************
        * 函数名称	:  SyncCfgFile
        * 函数描述	: 同步配置文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SyncLocalCfgFile();

        /******************************************************************************
        * 函数名称	:  SyncLocalMdbState
        * 函数描述	: 同步本地MDB状态
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SyncLocalMdbState();

        /******************************************************************************
        * 函数名称	:  SendRouterChange
        * 函数描述	: 将路由变更发送给备机
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SendRouterChange(const char* sMsg);

        /******************************************************************************
        * 函数名称	:  SendStatChange
        * 函数描述	: 发送状态变更信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SendStatChange(const char* sMsg);

        /******************************************************************************
        * 函数名称	:  SendStatChange
        * 函数描述	: 发送状态变更信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SendStatChange(int iHostID, EMdbRepState eState);

        /******************************************************************************
        * 函数名称	:  DealRepServerCmd
        * 函数描述	: 处理配置服务备机发来的同步信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int DealRepServerCmd(TRepNTCMsg& tCmd);
    private:
        std::string m_sRepServerIP;//配置服务备机IP
        int m_iRepPort;//配置服务备机端口号
        bool m_bRepExist;//配置服务备机是否存在
        TMdbServerConfig * m_pConfig;//配置文件
        TMdbStatMonitor *m_PMonitor;//状态监控

    };

    /**
    * @brief mdb状态监控类
    * 
    */	
    class TMdbStatMonitor
    {
    public: 
        TMdbStatMonitor();
        ~TMdbStatMonitor();
    public:
        /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	: 初始化状态监控信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Init(TMdbServerConfig *pConfig);

        /******************************************************************************
        * 函数名称	:  SyncState
        * 函数描述	:  同步MDB状态
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SyncState(const char* pData);

        /******************************************************************************
        * 函数名称	:  GetStateData
        * 函数描述	:  获取所有MDB状态
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetStateData(char*pDataBuf, int iBufLen);

        /******************************************************************************
        * 函数名称	:  AddMDB
        * 函数描述	: 增加一个QuickMDB信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int AddMDB(TMdbRepState &tMdb);

        /******************************************************************************
        * 函数名称	:  RemoveMDB
        * 函数描述	: 删除一个QuickMDB信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int RemoveMDB(int iHostID);

        /******************************************************************************
        * 函数名称	:  UpdateState
        * 函数描述	: 更新QuickMDB状态
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int UpdateState(int iHostID, EMdbRepState eState);

        /******************************************************************************
        * 函数名称	:  GetRoutingInfo
        * 函数描述	: 获取路由信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetRoutingInfo(int iRoutingID, char* sData);

        /******************************************************************************
        * 函数名称	:  GetHostInfo
        * 函数描述	: 获取主机信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetHostInfo(int iHostID, char* sData);

         /******************************************************************************
        * 函数名称	:  GetGroupInfo
        * 函数描述	: 获取组信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetGroupInfo(int iGroupID, char* sData);

         /******************************************************************************
        * 函数名称	:  GetHostState
        * 函数描述	: 获取主机状态
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        EMdbRepState GetHostState(int iHostID);//获取主机状态

    private:
        int GetOneGroup(TMdbGroup& tGroup, char *sData);
        int GetHostState(int iHostID, char* sState);//根据主机ID，获取主机状态
        
    private:
        TMdbServerConfig *m_pServerConfig;
        std::map<int, TMdbRepState> m_tMdbState;
    };

    /**
    * @brief 配置服务网络通信服务端
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

        int DealRegist(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg);//处理注册信息

        int DealStateChange(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg, bool IsRep = true);//处理状态变更信息

        int DealRoutingRequest(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg);//处理路由请求信息

        int DealRoutingChange(TMdbWinntTcpMsg* pWinntTcpMsg, bool IsRep);//处理路由变更信息

        int DealRoutingQuery(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg);//处理路由信息查询

        int DealRepConfigSync(TMdbWinntTcpHelper* pWinntcpHelper);//处理 备机启动时，同步配置文件信息请求

        int DealRepStateSync(TMdbWinntTcpHelper* pWinntcpHelper);//处理 备机启动时，同步状态信息请求

        int SetHostID(const char* sIP, int iPort, int iHostID);//设置该连接对应的主机标识
        int GetHostID(const char* sIP, int iPort);//根据IP获取对应的主机标识
        int RemoveClient(const char* sIP, int iPort);

    private:
        TMdbServerClient * m_pClient;
        TMdbStatMonitor * m_pMonitor;
        TMdbServerConfig* m_pConfig;
        std::vector<TMdbRepHost> m_arClientHost;//所有与mdbServer连接的客户端，存储客户端与主机ID的对应关系，用于异常时，找到对应主机
    };

    /**
    * @brief 配置服务控制类
    * 
    */	
    class TMdbServerCenter
    {
    public:
        TMdbServerCenter();
        ~TMdbServerCenter();
    public:
        /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	: 初始化，获取路由信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Init();

        /******************************************************************************
        * 函数名称	:  Start
        * 函数描述	: 启动监听，等待和处理连接
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Start();

        /******************************************************************************
        * 函数名称	:  Stop
        * 函数描述	:  停止MDBServer进程
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Stop();

    private:

    private:
        TMdbServerConfig *m_pConfig;//配置文件指针
        TMdbServerEngine * m_pCfgServer; //路由及状态信息监控服务器线程
        TMdbServerClient * m_pRepClient; //配置服务主备同步客户端
        TMdbStatMonitor* m_pStatMonitor;//状态监控类，维护连接的各个QuickMDB状态
        TMDBServerShm *m_pServerShm;//mdbServer相关信息共享内存
        TMdbNtcShareMem *m_pShmMgr;
        
        
    };

//}
#endif


