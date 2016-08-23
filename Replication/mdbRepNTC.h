/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbRepNTC.h		
*@Description: 封装使用NTC通信的相关类
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
#ifndef __ZTE_MINI_DATABASE_REP_NTC_H__
#define __ZTE_MINI_DATABASE_REP_NTC_H__
//#include "BillingNTC.h"
#include "Control/mdbRepCommon.h"
#include "Replication/mdbRepFileParser.h"
#include "Replication/mdbRepFlushDao.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Control/mdbVarcharMgr.h"
#include "Control/mdbPageCtrl.h"
#include "Control/mdbRowCtrl.h"
#include "Control/mdbExecuteEngine.h"

using std::endl;
//using namespace QuickMDB;

//namespace QuickMDB
//{
    class TMdbLoadDataTool;

#define HEARTBEAT_TIME_OUT 30//心跳超时默认时间
    
    enum eRepConnState
    {
        E_REP_NTC_CLOSED = 0,  ///连接不成功
        E_REP_NTC_OK           ///连接成功
    };

    enum eRepNtcEvent
    {
        E_NEW_CONNECTION=1000, //新的连接
        E_CONNECTION_CLOSE, //连接关闭
        E_MDB_REGIST = 2000, //QuickMDB注册信息
        E_MDB_REGIST_ANS,
        E_MDB_ROUTING_REQUEST, //路由请求
        E_MDB_ROUTING_REQUEST_ANS,
        E_MDB_HEARTBEAT, //QuickMDB心跳
        E_MDB_STAT_CHANGE,  //状态变化
        E_REP_STAT_CHANGE=3000, //备机通知mdb状态变化
        E_REP_SYNC_CONFIG_REQUEST, //备机请求同步信息
        E_REP_SYNC_CONFIG_REQUEST_ANS,
        E_REP_SYNC_STATE_REQUEST,//备机请求同步状态信息
        E_REP_SYNC_STATE_REQUEST_ANS,
        E_REP_ROUTING_CHANGE,//备机通知路由变更
        E_SVR_ROUTING_CHANGE=4000, //路由变更
        E_SVR_ROUTING_INFO, //路由信息
        E_SVR_ROUTING_INOF_ANS,
        E_LOAD_FROM_REP = 5000,//从备机加载数据请求
        E_LOAD_FROM_REP_ANS,
        E_SEND_REP_DATA,//发送同步数据
        E_CLEAN_REP_FILE,//清除同步文件（兼容1.2版本）
        E_HEART_BEAT = 6000,//心跳包
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
    * @brief NTC同步客户端封装
    * 
    */
    class TMdbRepNTCClient: public TMdbSyncEngine
    {
    public:
        TMdbRepNTCClient();
        ~TMdbRepNTCClient();
    public:
        /******************************************************************************
        * 函数名称	:  Connect
        * 函数描述	: 连接服务端
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool Connect(const char* pszIP, int iPort, int iTimeOut = 3000);

        /******************************************************************************
        * 函数名称	:  SendPacket
        * 函数描述	: 向服务端发送消息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool SendPacket(eRepNtcEvent eEvent, const char* psBodyBuffer = "", int iLength=-1);

        /******************************************************************************
        * 函数名称	:  SendHearbeat
        * 函数描述	: 向服务端发送心跳包
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool SendHearbeat();

        /******************************************************************************
        * 函数名称	:  GetMsg
        * 函数描述	: 从服务端获取消息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetMsg(TRepNTCMsg& tMsg);

        /******************************************************************************
        * 函数名称	:  Disconnect
        * 函数描述	: 断开与服务端的连接
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        void Disconnect();

        /******************************************************************************
        * 函数名称	:  NeedReconnect
        * 函数描述	: 是否需要重新连接
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool NeedReconnect();
    protected:
        //virtual bool OnConnect(TMdbConnectEvent* pEventInfo, TMdbEventPump* pEventPump);
        //virtual bool OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump);
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
    * @brief 配置服务客户端
    * 
    */
    class TMdbRepMonitorClient: public TMdbRepNTCClient
    {
    public: 
        TMdbRepMonitorClient();
        ~TMdbRepMonitorClient();
    public:
        /******************************************************************************
        * 函数名称	:  Register
        * 函数描述	:  向配置服务注册
        * 输入		:  
        * 输出		:  iHostID 本机在配置服务端对应的host_ID
        * 输出		:  iHeartbeat 心跳间隔
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Register(const char* sIP, int iPort, int& iHostID, int& iHeartbeat);

        /******************************************************************************
        * 函数名称	:  RoutingRequest
        * 函数描述	:  向配置服务请求路由信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int RoutingRequest(int iHostID, TMdbShmRepMgr *pShmRepMgr);

        // 只向配置服务请求路由信息，不写入共享内存
        int RoutingRequestNoShm(int iHostID);

        /******************************************************************************
        * 函数名称	:  ReportState
        * 函数描述	:  向配置服务请求路由信息
        * 输入		:  iHostID 本机在配置服务端对应的Host_ID
        * 输入		:  eState 本机的状态
        * 输出		:  
        * 返回值	:  0 - 成功! 非0-失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int ReportState(int iHostID, EMdbRepState eState);

        const char* GetSvrConfigMsg();

    private:
        char m_sSvrMsg[MAX_REP_SEND_BUF_LEN];
    };


    /**
    * @brief 分片备份同步数据发送端client
    * 
    */
    class TMdbRepDataClient:public TMdbSyncEngine
    {
    public:
        TMdbRepDataClient();
        ~TMdbRepDataClient();

    public:
        /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	:  初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int Init(TMdbConfig* pMdbCfg);

        /******************************************************************************
        * 函数名称	:  Connect
        * 函数描述	: 连接服务端
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool Connect(const char* pszIP, int iPort, int iTimeOut = 3000);

        /******************************************************************************
        * 函数名称	:  SendPacket
        * 函数描述	: 向服务端发送消息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool SendPacket(eRepNtcEvent eEvent, const char* psBodyBuffer = "", int iLength=-1);

        /******************************************************************************
        * 函数名称	:  SendHearbeat
        * 函数描述	: 向服务端发送心跳包
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool SendHearbeat();

        /******************************************************************************
        * 函数名称	:  GetMsg
        * 函数描述	: 从服务端获取消息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetMsg(TMdbMsgInfo* &pMsg);

        /******************************************************************************
        * 函数名称	:  Disconnect
        * 函数描述	: 断开与服务端的连接
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        void Disconnect();

        /******************************************************************************
        * 函数名称	:  NeedReconnect
        * 函数描述	: 是否需要重新连接
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        bool NeedReconnect();

        /******************************************************************************
        * 函数名称	:  SetHostID
        * 函数描述	:  设置客户端对应的hostID
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SetHostID(int iLocHostID, int iRepHostID);
        
        /* 函数名称	:  GetHostID
        * 函数描述	:  获取客户端对应的hostID
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetHostID();

        /******************************************************************************
        * 函数名称	:  SendData
        * 函数描述	: 将文件数据发送至对端
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功 !0 -失败
        * 作者		:  jiang.xiaolong
        *******************************************************************************/ 
        int SendData(const char* sFileName);

		/******************************************************************************
        * 函数名称	:  SendData
        * 函数描述	: 将一条同步记录数据发送至对端
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功 !0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/ 
        int SendData(const char* sOneRecord, int iLen);

		/******************************************************************************
		* 函数名称	:  SendData
		* 函数描述	: 将缓存中残留数据发送至对端
		* 输入		:  
		* 输出		:  
		* 返回值	:  0 - 成功 !0 -失败
		* 作者		:  jiang.xiaolong
		*******************************************************************************/ 
		int SendData();

        /******************************************************************************
        * 函数名称	:  LoadData
        * 函数描述	: 从对端加载数据
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败 
        * 作者		:  jiang.lili
        *******************************************************************************/
        int LoadData(const char* sRoutinglist);

        /******************************************************************************
        * 函数名称	:  SendCleanCmd
        * 函数描述	: 通知备机清除同步文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败 
        * 作者		:  jiang.lili
        *******************************************************************************/
        int SendCleanCmd();

        /******************************************************************************
        * 函数名称	:  GetIP
        * 函数描述	:  获取对端IP
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        const char* GetIP()
        {
            return m_strIP.c_str();
        }
        /******************************************************************************
        * 函数名称	:  GetPort
        * 函数描述	:  获取对端端口号
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
        int GetPort()
        {
            return m_iPort;
        }

    private:
		int SetLoadMsg(const char* sTableName, const char* sRoutinglist, bool bIsMemLoad, char * sMsgBuf);
        int LoadOneTable(const char* sTableName, const char* sRoutinglist, bool bIsMemLoad = false);

    public:
        eRepConnState m_eState;
		bool bTool;
		char sTblName[MAX_NAME_LEN];

    protected:
        int m_iHeartbeat;
        TMdbSharedPtr<TMdbPeerInfo> m_pPeerInfo; 
        TMdbRecvMsgEvent* m_pMsgEvent;

        int m_iTimeOut;        
        time_t m_tConnStateTime;

    private:
        std::string m_strDsn;
        TMdbLoadDataTool * m_pLoadDataTool;
        int m_iRepHostID;
        int m_iLocHostID;
        char m_sSendBuf[MAX_REP_SEND_BUF_LEN];
        int m_iBufLen;
		time_t m_tFlushTime;
        TMdbRepFileParser *m_ptFileParser;//文件解析
        TMdbRepFileStat *m_ptRepFileStat;//文件记录操作统计
        std::string m_strIP;//对端IP
        int m_iPort;//对端端口号
       
    };

    /**
    * @brief server端封装
    * 
    */	
    class TMdbRepNTCServer:public TMdbNtcEngine
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
        int m_iHeartbeatWarning;//客户端心跳超时时间
        int m_iWorkThreadNum;//工作线程数量

    };

    class TRepServerDataRcv;

    /**
    * @brief 分片备份同步数据接收端server
    * 
    */
    class TMdbRepDataServer:public TMdbNtcEngine
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
        int DealLoadRequest(const char* pData, TMdbPeerInfo* pPeerInfo);//数据加载请求
        int DealCleanFile(const char* pData);//清除同步文件
        int DealRcvData(const char* pData, int iLength, TMdbPeerInfo* pPeerInfo);//接收同步数据

        TRepServerDataRcv *GetRcvEngine(const char* strIP, int iPort);
    protected:
        int m_iHeartbeatWarning;//客户端心跳超时时间
        int m_iWorkThreadNum;//工作线程数量
    private:
        TMdbNtcAutoArray m_arRcvEngine;//数据接收引擎
        TMdbNtcString m_strDsn;//DSN名称
    };

    class TMdbLoadDataTool
    {
    public:
        TMdbLoadDataTool();
        ~TMdbLoadDataTool();
    public:
        int Init(TMdbConfig* pMdbCfg);
		int SetUploadTable(const char* sTableName);
        int UploadData(const char* pDataBuf, int iLen, bool &bOver);
		int UploadMemData(const char* sDataBuf, int iBufLen, bool &bOver);
		int SetbTool(bool bToolFlag);
    private:
        void CombineData(const char* sDataBuf, int iBufLen);
        int DealWithMsgBuf();
		int DealWithMemMsgBuf();
        void SaveRecord(int iCurPos);
        int InitSQL(TMdbTable* pTable);
        int GetOneRecord(int iNextPos);
		int  ExecuteForMdbReLoadUpdate(size_t iColCount,int* iDropIndex);
		int  ExecuteForMdbReLoadInsert(size_t iColCount,int* iDropIndex);
		int  ExecuteForMdbReLoad(size_t iColCount,int* iDropIndex);
        int Execute();
		int ExecuteMemData();
        bool IsNULL(const char* sSrc);
    private:
        TMdbConfig* m_pMdbCfg;
		TMdbShmDSN * m_pShmDsn;
		char sDsnName[MAX_NAME_LEN];
        TMdbDatabase *m_pDataBase;
        TMdbQuery *m_pCurQuery;  //insert
		TMdbQuery *m_pMdbSelQuery; //select for mdb
		TMdbQuery *m_pMdbUptQuery; //select for mdb

        bool m_bDataOver;

        char* m_psMesgBuf;
        int m_iMsgBufLen;
        char m_sResidueBuf[MAX_REP_SEND_BUF_LEN];
        int m_iResidueLen;

        int m_iMsgLen;
        int m_iCurPos;

		TMdbTableSpaceCtrl m_tTSCtrl;
		TMdbVarCharCtrl m_tVarcharCtrl;
		TMdbTable * m_pCurTable;
		TMdbTableSpace * m_pCurTS;
		char m_cStorage;
		TMdbPage * m_pCurFreePage;
		TMdbPageCtrl m_tPageCtrl;
		TMdbRowCtrl m_tRowCtrl;
		int m_iVarColPos[MAX_COLUMN_COUNTS];
		int m_iVarColCount;

        char m_sTempRecord[MAX_VALUE_LEN];
		int m_iTempLen;
        //int  m_iTempPos;

        std::vector<std::string> m_vParam;
        char m_paramValue[MAX_VALUE_LEN];
        TRepLoadDao * m_pLoadDao;
		std::string strTableName;
		std::vector<int> m_vKeyNo;
		bool bTool;
    };

//}
#endif
