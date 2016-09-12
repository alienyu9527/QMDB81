/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbAgentServer.h
*@Description： 负责管理QMDB的远程访问(CS模式)代理接口
*@Author:       li.shugang jiang.mingjun
*@Date：        2009年05月23日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_AGENT_SERVER_H__
#define __MINI_DATABASE_AGENT_SERVER_H__

#ifdef WIN32
#include <iostream>
#include <windows.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

#else
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#endif

#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Interface/mdbQuery.h"
#include "Interface/mdbSequance.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbSocket.h"
#include "Helper/mdbCspAvpMgr.h"
#include "Helper/mdbPerfInfo.h"
#include "Control/mdbProcCtrl.h"
#include "Control/mdbLinkCtrl.h"
#include <map>
#include "Common/mdbNtcEngine.h"

////#include "BillingSDK.h"
////#include "BillingNTC.h"
//using namespace QuickMDB;

using namespace std;

//namespace QuickMDB{

    #define MAX_SEQUENCE_NUM 255
    //一次最多prepare的个数
    #define MAX_PREPARE_NUM 128 
    #define MAX_CS_THREAD_COUNTS 5000
    #define MAX_DUMP_PACKAGE_BUFFER (MAX_CSP_LEN+1024) //数据包本地缓存大小确保大于MAX_CSP_LEN
    //no ocp begin
    #define NO_OCP_USERNAME_LEN 32
    #define NO_OCP_USERNAME_POS SIZE_MSG_AVP_HEAD
    #define NO_OCP_PASSWORD_POS (SIZE_MSG_AVP_HEAD + NO_OCP_USERNAME_LEN)
    #define NO_OCP_PASSWORD_LEN 32
    #define NO_OCP_SQLLABEL_POS SIZE_MSG_BIN_HEAD
    #define NO_OCP_SQL_POS (SIZE_MSG_BIN_HEAD+sizeof(int))
    #define NO_OCP_PARAMCNT_POS (SIZE_MSG_BIN_HEAD+sizeof(int))
    #define NO_OCP_BATCHCNT_POS (SIZE_MSG_BIN_HEAD+sizeof(int))
    
    //no ocp end

    //代码块锁功能
    class CodeBlock
    {
    public:
        CodeBlock() : n(0) { }

        class Within
        {
            CodeBlock& _s;
        public:
            Within(CodeBlock& s) : _s(s)
            {
                _s.enter();
            }
            ~Within()
            {
                _s.leave();
            }
        };

        inline void assertWithin()
        {
            assert( n == 1 );
        }

    private:
        volatile int n;
        unsigned tid;

        inline void enter()
        {
            if( ++n != 1 ) ;

        }
        inline void leave()
        {
            if( --n != 0 ) ;
        }
    };
    
    class TMdbNoNtcAgentServer;
    //clientSocket信息
    class TClientSocket
    {
    public:
        TClientSocket();
        ~TClientSocket();
    public:
        int m_clientFd;//客户端socket链接ID
        Socket m_oSocket;//套接字封装
        TMdbNoNtcAgentServer * m_pAgentServer;
    };

    class ClientQuery
    {
    public:
        ClientQuery();
        ~ClientQuery();
        int CalcMaxNextNum();//计算最大next次数
    public:
        TMdbQuery *m_pQuery;
        int m_iSqlLabel;//SQL标识
        int m_iMaxNextNum;//select 每次最大的next结果数
        bool m_bFirstNext;
        TMdbParamArray m_tParamArray;//批量绑定参数数组，中间转储过渡用
    };
    //agent的序号处理
    class TAgentClientSequence
    {
    public:
        TAgentClientSequence();
         ~TAgentClientSequence();
        int Clear();
        int m_iLastSequence;//记录上一个sequence;
        unsigned char m_sLastSendPackage[MAX_CSP_LEN];//上次的包信息
        int m_iLastSendLen;//上次发送的长度
    };
    //
    class TAgentClient
    {
    public:
        TAgentClient();
        ~TAgentClient();
        void Clear();
        int Init();
        void Print();
        ClientQuery *  GetClientQuery(int iSqlLabel,bool bOutWarn = false);//获取ClientQuery
        int RemoveClientQuery(int iSqlLabel);
        TMdbSequenceMgr * FindSeqMgrByName(const char * sSeqName,TMdbConfig * pConfig);
        TMdbSequenceMgr * GetSeqMgrByName(const char * sSeqName,TMdbConfig * pConfig);
    	
        unsigned int m_iSessionID;//会话ID
        int m_iPno;//billing NTC Pno
        int m_iPort;//对端端口
        
        Socket m_ClientSocket;//客户端套接字封装
        TMdbCspParserMgr      m_tCspParserMgr;//csp解析器管理
        TMdbCSLink m_tTMdbCSLink;//CS 注册信息
    	int m_cProcState;//记录服务单的进程状态变化

        TMdbDatabase *m_pDB;  //用于连接QDB
        std::vector<ClientQuery *> m_vClientQuery;//sql对象指针容器
        unsigned long m_iThreadId;    //所在的线程ID
        TMdbAvpHead m_tHead;  //avp head 解析
        unsigned char m_sSendPackage[MAX_CSP_LEN];
        unsigned char m_sRecvPackage[MAX_CSP_LEN];
        char m_sPackageBuffer[MAX_DUMP_PACKAGE_BUFFER];//缓存数据抓包
        bool m_bFirstNext;//第一次解析
        bool m_bRegister;
        char m_iSqlType;
        unsigned char m_iFieldCount;
        unsigned char m_iParamCount;
        TMdbSequenceMgr *m_pSequenceMgr[MAX_SEQUENCE_NUM]; //默认255个Sequence缓存
        TAgentClientSequence m_tLastSequence;//记录前一次sequence
		TMdbRecvMsgEvent * m_pEventInfo;//接收数据包事件        
        unsigned int m_iRecvPos;//上次读取到的位置
        NoOcpParse m_tRecvDataParse;
        NoOcpParse m_tSendDataParse;

		
    };

    //数据包写本地文件
    class TAgentFile
    {
    public:
    	TAgentFile();
    	~TAgentFile();
    	
    public:
    	int Init();
    	int Open(const char* sFileName);
    	int Write(const char *buffer, unsigned int iCount);
    	
    private:
    	int checkAndBackup();//文件检测与备份
    	
    private:	
    	FILE* m_fp;
    	char  m_sFileName[MAX_PATH_NAME_LEN];
    	
    };

	//非NTC方式的Server
	class TMdbNoNtcAgentServer
	{
public:
    TMdbNoNtcAgentServer();
    ~TMdbNoNtcAgentServer();
    int Init(const char* pszDSN,int iAgentPort);//初始化：Attach共享内存，取出进程信息所在位
    //int PreSocket();//初始化Socket，并进行监听
    int PreSocket(int iAgentPort);
    int Start(const char* pszName);// 建立监听，并针对链接进行处理
private:
  //  int PrepareRecvLogonPack(TAgentClient *ptClient);//接收登陆消息包
    int Login(TAgentClient *ptClient) throw (TMdbException);//登陆, 并进行权限验证
    void threadExit(TAgentClient *&ptClient,bool bCloseSocket);//线程结束的收尾工作
  //  int RegConnectMDB(TAgentClient *ptClient) throw (TMdbException);//链接qmdb
    int  PrintRecvMsg(TAgentClient *ptClient);//打印接收数据
    void PrintMsg(TAgentClient *ptClient,unsigned char *caTemp,int iMsgLen);//打印16进制数据
    int  PrintSendMsg(TAgentClient *ptClient);//打印发送数据
    int Run(TAgentClient *ptClient);//处理消息
   // int RecvAppMessage(TAgentClient *ptClient);//接收应用类消息
    int DoOperation(TAgentClient *ptClient);//处理各类应用
    int StartNewPthread() ;//开启新线程
    int AppErrorAnswer(TAgentClient *ptClient,int iAnsCode,const char *fmt,...);//错误消息应答
    int AppSendSQL(TAgentClient *ptClient);//SQL注册，区分动态静态SQL
   // int SendSQLAnswer(TAgentClient *ptClient,int iResult,int iSqlLabel,char *sMsg);//发送SQLAnswer应答
    int AppSendParam(TAgentClient *ptClient);//处理发送来的参数
    int AppSendParamArray(TAgentClient *ptClient);//处理发送来的参数数组
    int AppNextSQLResult(TAgentClient *ptClient);//响应next操作
    int AppTransAction(TAgentClient *ptClient);//事务处理
    int AppGetSequence(TAgentClient *ptClient);//获取序列
    int FillNextResult(TAgentClient *ptClient,TMdbQuery * pQuery,TMdbCspParser * pSendSQLAns)throw(TMdbException,TMdbCSPException);//填充查询的next 返回值
    int AppGetQmdbInfo(TAgentClient *ptClient);//获取qmdb 信息包
    
   // int RecvPackageBody(TAgentClient *ptClient,int iMsgLen);//接收包体
    int SendPackageBody(TAgentClient *ptClient,int iSendLen);// 发送包体
    void ThreadSleep(int milliSeconds) //睡眠毫秒时间
    {
        timeval timeout = { milliSeconds/1000, milliSeconds%1000};
        select(0, NULL, NULL, NULL, &timeout);
    }
    static void* agent(void* p);
    int svc(int iSockFD,sockaddr_in  tAddr);//数据交互处理
   // int SendLogOnAnswer(TAgentClient *ptClient,int AnsCode,char *AnsMsg);//发送登录应答消息
    TAgentClient * GetFreeAgentClient(int iSockFD,sockaddr_in  tAddr);//获取空闲agent client
    int  GetConnectAgentPort(TMdbCspParser * pParser);
    int Authentication(TAgentClient *ptClient);//认证
    int SendAnswer(TMdbCspParser * pCspParser,TAgentClient *ptClient,int AnsCode,const char * sMsg);//发回复信息
    int RecvPackage(TAgentClient * ptClient);//接收消息包
    int RecvPackageOnce(TAgentClient * ptClient);//接收消息包
    int CheckClientSequence(TAgentClient * ptClient);//检测客户端序号
	void DumpRecvPackage(TAgentClient *ptClient);//日志级别-1时抓数据包-接收包
	void DumpSendPackage(TAgentClient *ptClient);//日志级别-1时抓数据包-发送包
	void DumpPackage(TAgentClient *ptClient, bool bSendFlag);  //  落地数据包

//bin 不使用ocp协议
    int DoOperationBin(TAgentClient *ptClient);//处理各类应用
    int AppSendSQLBin(TAgentClient *ptClient);//SQL注册，区分动态静态SQL
    int AppSendParamBin(TAgentClient *ptClient);//处理发送来的参数
    int AppSendParamArrayBin(TAgentClient *ptClient);//处理发送来的参数数组
    int AppNextSQLResultBin(TAgentClient *ptClient);//响应next操作
    int AuthenticationBin(TAgentClient *ptClient);//认证
    int SendAnswer(TAgentClient *ptClient,int AnsCode,const char * sMsg);//发回复信息
    int FillNextResult(TAgentClient *ptClient,TMdbQuery * pQuery,bool &bFirstNext)throw(TMdbException,TMdbCSPException);//填充查询的next 返回值
    int AppTransActionBin(TAgentClient *ptClient);//事务处理
    int AppGetQmdbInfoBin(TAgentClient *ptClient);//获取qmdb 信息包
    int AppGetSequenceBin(TAgentClient *ptClient);//获取序列
    
private:
    TMdbShmDSN* m_pShmDSN;
    TMdbConfig *m_pConfig;
    TMdbDSN    *m_pDsn;
    int m_clientFd;//客户端socket链接ID
    TAgentClient *m_ptClient[MAX_CS_THREAD_COUNTS];
    int m_iClientThreadNum;//接收客户端链接的线程数目
    static int m_iSessionStep;//会话ID
    Socket m_oSocket;//套接字封装
    TMutex  tMutex;
    CodeBlock durThreadMain;
    //static bool m_bLoginFailed;
    //static time_t m_tLastTime;//上次登陆失败时间
    TMdbProcCtrl m_tProcCtrl;//进程控制
    TMdbLinkCtrl m_tLinkCtrl;//链接管理
	TAgentFile m_tAgentFile;//抓数据包写本地文件
	int iPort;
	
	};
	
	//服务端NTC改造
    class TMdbAgentServer:public TMdbNtcEngine
    {
    public:
        TMdbAgentServer();
        virtual ~TMdbAgentServer();
        int Init(const char* pszDSN);//初始化：Attach共享内存，取出进程信息所在位
        int PreSocket(int iAgentPort);//初始化Socket，并进行监听
        int StartServer(const char* pszName);// 建立监听，并针对链接进行处理
    public:
		/******************************************************************************
		* 函数名称	:  OnRecvMsg
		* 函数描述	:  当客户端有消息来的时候，触发此函数
		* 输入		:  事件信息\事件泵
		* 输出		:
		* 返回值	:
		* 作者		:
		*******************************************************************************/
		virtual bool OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump);
		
		/******************************************************************************
		* 函数名称	:  OnConnect
		* 函数描述	:  当有socket连接或者socket连接成功时，触发此函数
		* 输入		:  事件信息\事件泵
		* 输出		:
		* 返回值	:
		* 作者		:
		*******************************************************************************/
	    virtual bool OnConnect(TMdbConnectEvent* pEventInfo, TMdbEventPump* pEventPump);
		
		/******************************************************************************
		* 函数名称	:  OnDisconnect
		* 函数描述	:  当有连接断开时，触发此函数
		* 输入		:  事件信息\事件泵
		* 输出		:
		* 返回值	:
		* 作者		:
		*******************************************************************************/
		virtual bool OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump);

		/******************************************************************************
		* 函数名称	:  OnError
		* 函数描述	:  当发生错误时，触发此函数，如解析消息包出错等
		* 输入		:  事件信息\事件泵
		* 输出		:
		* 返回值	:
		* 作者		:
		*******************************************************************************/
	    virtual bool OnError(TMdbErrorEvent* pEventInfo, TMdbEventPump* pEventPump);
		
		/******************************************************************************
		* 函数名称	:  OnTimeOut
		* 函数描述	:  当空闲超时时，触发此函数
		* 输入		:  事件信息\事件泵
		* 输出		:
		* 返回值	:
		* 作者		:
		*******************************************************************************/
		virtual bool OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump);
		
		/******************************************************************************
		* 函数名称	:  RecvMsg
		* 函数描述	:  接收数据包
		* 输入		:  TAgentClient * 
		* 输出		:
		* 返回值	:
		* 作者		:
		*******************************************************************************/
		int RecvMsg(TAgentClient * ptClient);
		
		/******************************************************************************
		* 函数名称	:  CheckNewClient
		* 函数描述	:  检验连接有效性
		* 输入		:  
		* 输出		:
		* 返回值	:
		* 作者		:
		*******************************************************************************/
		virtual bool CheckNewClient(QuickMDB_SOCKET new_fd, TMdbServerInfo* pServerInfo);
        
		/******************************************************************************
		* 函数名称	:  IPExist
		* 函数描述	:  检查ip是否在配置列表中
		* 输入		:  
		* 输出		:
		* 返回值	:
		* 作者		:
		*******************************************************************************/
        bool IsIPExist(std::vector<string> &vIPListCfg, TMdbNtcSplit &tSplitCfg, TMdbNtcSplit &tSplitPeer);
        void ParseCSIPCfg();//解析限制的ip列表
        
        /******************************************************************************
        * 函数名称  :  AddWorkThread
        * 函数描述  :  增加工作线程
        * 输入      :
        * 输出      :
        * 返回值    :
        * 作者      :
        *******************************************************************************/
        int AddWorkThread();
		
        int ParseMsgHead(TAgentClient * ptClient);//解析接收包头
    private:
        int Login(TAgentClient *ptClient) throw (TMdbException);//登陆, 并进行权限验证
        void ClientExit(TAgentClient *&ptClient);//线程结束的收尾工作
        int  PrintRecvMsg(TAgentClient *ptClient);//打印接收数据
        void PrintMsg(TAgentClient *ptClient,unsigned char *caTemp,int iMsgLen);//打印16进制数据
        int  PrintSendMsg(TAgentClient *ptClient);//打印发送数据
        int DoOperation(TAgentClient *ptClient);//处理各类应用
        int AppErrorAnswer(TAgentClient *ptClient,int iAnsCode,const char *fmt,...);//错误消息应答
        int AppSendSQL(TAgentClient *ptClient);//SQL注册，区分动态静态SQL
        int AppSendParam(TAgentClient *ptClient);//处理发送来的参数
        int AppSendParamArray(TAgentClient *ptClient);//处理发送来的参数数组
        int AppNextSQLResult(TAgentClient *ptClient);//响应next操作
        int AppTransAction(TAgentClient *ptClient);//事务处理
        int AppGetSequence(TAgentClient *ptClient);//获取序列
        int FillNextResult(TAgentClient *ptClient,TMdbQuery * pQuery,TMdbCspParser * pSendSQLAns)throw(TMdbException,TMdbCSPException);//填充查询的next 返回值
        int AppGetQmdbInfo(TAgentClient *ptClient);//获取qmdb 信息包
        
        int SendPackageBody(TAgentClient *ptClient,int iSendLen);// 发送包体
        TAgentClient * GetFreeAgentClient(TMdbConnectEvent * pEventInfo);//获取空闲agent client
        int Authentication(TAgentClient *ptClient);//认证
        int SendAnswer(TMdbCspParser * pCspParser,TAgentClient *ptClient,int AnsCode,const char * sMsg);//发回复信息
        int CheckClientSequence(TAgentClient * ptClient);//检测客户端序号
    	void DumpRecvPackage(TAgentClient *ptClient);//日志级别-1时抓数据包-接收包
    	void DumpSendPackage(TAgentClient *ptClient);//日志级别-1时抓数据包-发送包
    	void DumpPackage(TAgentClient *ptClient, bool bSendFlag);  //  落地数据包
//bin 不使用ocp协议
        int DoOperationBin(TAgentClient *ptClient);//处理各类应用
        int AppSendSQLBin(TAgentClient *ptClient);//SQL注册，区分动态静态SQL
        int AppSendParamBin(TAgentClient *ptClient);//处理发送来的参数
        int AppSendParamArrayBin(TAgentClient *ptClient);//处理发送来的参数数组
        int AppNextSQLResultBin(TAgentClient *ptClient);//响应next操作
        int AuthenticationBin(TAgentClient *ptClient);//认证
        int SendAnswer(TAgentClient *ptClient,int AnsCode,const char * sMsg);//发回复信息
        int FillNextResult(TAgentClient *ptClient,TMdbQuery * pQuery,bool &bFirstNext)throw(TMdbException,TMdbCSPException);//填充查询的next 返回值
        int AppTransActionBin(TAgentClient *ptClient);//事务处理
        int AppGetQmdbInfoBin(TAgentClient *ptClient);//获取qmdb 信息包
        int AppGetSequenceBin(TAgentClient *ptClient);//获取序列
    private:
        TMdbShmDSN* m_pShmDSN;
        TMdbConfig *m_pConfig;
        TMdbDSN    *m_pDsn;
        TAgentClient *m_ptClient[MAX_CS_THREAD_COUNTS];
        int m_iClientThreadNum;//接收客户端链接的线程数目
        static int m_iSessionStep;//会话ID
        TMutex  tMutex;
        TMutex  tMutexConnect;
        TMdbProcCtrl m_tProcCtrl;//进程控制
        TMdbLinkCtrl m_tLinkCtrl;//链接管理
    	TAgentFile m_tAgentFile;//抓数据包写本地文件
        std::vector<string> m_vValidIP;//有效ip
        std::vector<string> m_vInValidIP;//无效ip
    	bool m_bRecvFlag;//处理数据包标记
        int m_iPumpMaxCount;//工作线程最大值
        int m_iPeerCountPerPump;//每个线程连接数
    };


//}

#endif //__MINI_DATABASE_AGENT_SERVER_H__

