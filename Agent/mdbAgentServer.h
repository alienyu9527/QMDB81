/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbAgentServer.h
*@Description�� �������QMDB��Զ�̷���(CSģʽ)����ӿ�
*@Author:       li.shugang jiang.mingjun
*@Date��        2009��05��23��
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
    //һ�����prepare�ĸ���
    #define MAX_PREPARE_NUM 128 
    #define MAX_CS_THREAD_COUNTS 5000
    #define MAX_DUMP_PACKAGE_BUFFER (MAX_CSP_LEN+1024) //���ݰ����ػ����Сȷ������MAX_CSP_LEN

    //�����������
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
    
    class TMdbAgentServer;
    //clientSocket��Ϣ
    class TClientSocket
    {
    public:
        TClientSocket();
        ~TClientSocket();
    public:
        int m_clientFd;//�ͻ���socket����ID
        Socket m_oSocket;//�׽��ַ�װ
        TMdbAgentServer * m_pAgentServer;
    };

    class ClientQuery
    {
    public:
        ClientQuery();
        ~ClientQuery();
        int CalcMaxNextNum();//�������next����
    public:
        TMdbQuery *m_pQuery;
        int m_iSqlLabel;//SQL��ʶ
        int m_iMaxNextNum;//select ÿ������next�����
        TMdbParamArray m_tParamArray;//�����󶨲������飬�м�ת��������
    };
    //agent����Ŵ���
    class TAgentClientSequence
    {
    public:
        TAgentClientSequence();
         ~TAgentClientSequence();
        int Clear();
        int m_iLastSequence;//��¼��һ��sequence;
        unsigned char m_sLastSendPackage[MAX_CSP_LEN];//�ϴεİ���Ϣ
        int m_iLastSendLen;//�ϴη��͵ĳ���
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
        ClientQuery *  GetClientQuery(int iSqlLabel);//��ȡClientQuery
        int RemoveClientQuery(int iSqlLabel);
        TMdbSequenceMgr * FindSeqMgrByName(const char * sSeqName,TMdbConfig * pConfig);
        TMdbSequenceMgr * GetSeqMgrByName(const char * sSeqName,TMdbConfig * pConfig);
    	
        unsigned int m_iSessionID;//�ỰID
        int m_iPno;//billing NTC Pno
        int m_iPort;//�Զ˶˿�
        
        //Socket m_ClientSocket;//�ͻ����׽��ַ�װ
        TMdbCspParserMgr      m_tCspParserMgr;//csp����������
        TMdbCSLink m_tTMdbCSLink;//CS ע����Ϣ
    	int m_cProcState;//��¼���񵥵Ľ���״̬�仯

        TMdbDatabase *m_pDB;  //��������QDB
        std::vector<ClientQuery *> m_vClientQuery;//sql����ָ������
        unsigned long m_iThreadId;    //���ڵ��߳�ID
        TMdbAvpHead m_tHead;  //avp head ����
        unsigned char m_sSendPackage[MAX_CSP_LEN];
        unsigned char m_sRecvPackage[MAX_CSP_LEN];
        char m_sPackageBuffer[MAX_DUMP_PACKAGE_BUFFER];//��������ץ��

        TMdbSequenceMgr *m_pSequenceMgr[MAX_SEQUENCE_NUM]; //Ĭ��255��Sequence����
        TAgentClientSequence m_tLastSequence;//��¼ǰһ��sequence
		TMdbRecvMsgEvent * m_pEventInfo;//�������ݰ��¼�        
        unsigned int m_iRecvPos;//�ϴζ�ȡ����λ��
    };

    //���ݰ�д�����ļ�
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
    	int checkAndBackup();//�ļ�����뱸��
    	
    private:	
    	FILE* m_fp;
    	char  m_sFileName[MAX_PATH_NAME_LEN];
    	
    };
	
	//�����NTC����
    class TMdbAgentServer:public TMdbNtcEngine
    {
    public:
        TMdbAgentServer();
        virtual ~TMdbAgentServer();
        int Init(const char* pszDSN);//��ʼ����Attach�����ڴ棬ȡ��������Ϣ����λ
        int PreSocket(int iAgentPort);//��ʼ��Socket�������м���
        int StartServer(const char* pszName);// ������������������ӽ��д���
    public:
		/******************************************************************************
		* ��������	:  OnRecvMsg
		* ��������	:  ���ͻ�������Ϣ����ʱ�򣬴����˺���
		* ����		:  �¼���Ϣ\�¼���
		* ���		:
		* ����ֵ	:
		* ����		:
		*******************************************************************************/
		virtual bool OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump);
		
		/******************************************************************************
		* ��������	:  OnConnect
		* ��������	:  ����socket���ӻ���socket���ӳɹ�ʱ�������˺���
		* ����		:  �¼���Ϣ\�¼���
		* ���		:
		* ����ֵ	:
		* ����		:
		*******************************************************************************/
	    virtual bool OnConnect(TMdbConnectEvent* pEventInfo, TMdbEventPump* pEventPump);
		
		/******************************************************************************
		* ��������	:  OnDisconnect
		* ��������	:  �������ӶϿ�ʱ�������˺���
		* ����		:  �¼���Ϣ\�¼���
		* ���		:
		* ����ֵ	:
		* ����		:
		*******************************************************************************/
		virtual bool OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump);

		/******************************************************************************
		* ��������	:  OnError
		* ��������	:  ����������ʱ�������˺������������Ϣ�������
		* ����		:  �¼���Ϣ\�¼���
		* ���		:
		* ����ֵ	:
		* ����		:
		*******************************************************************************/
	    virtual bool OnError(TMdbErrorEvent* pEventInfo, TMdbEventPump* pEventPump);
		
		/******************************************************************************
		* ��������	:  OnTimeOut
		* ��������	:  �����г�ʱʱ�������˺���
		* ����		:  �¼���Ϣ\�¼���
		* ���		:
		* ����ֵ	:
		* ����		:
		*******************************************************************************/
		virtual bool OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump);
		
		/******************************************************************************
		* ��������	:  RecvMsg
		* ��������	:  �������ݰ�
		* ����		:  TAgentClient * 
		* ���		:
		* ����ֵ	:
		* ����		:
		*******************************************************************************/
		int RecvMsg(TAgentClient * ptClient);
		
		/******************************************************************************
		* ��������	:  CheckNewClient
		* ��������	:  ����������Ч��
		* ����		:  
		* ���		:
		* ����ֵ	:
		* ����		:
		*******************************************************************************/
		virtual bool CheckNewClient(QuickMDB_SOCKET new_fd, TMdbServerInfo* pServerInfo);
        
		/******************************************************************************
		* ��������	:  IPExist
		* ��������	:  ���ip�Ƿ��������б���
		* ����		:  
		* ���		:
		* ����ֵ	:
		* ����		:
		*******************************************************************************/
        bool IsIPExist(std::vector<string> &vIPListCfg, TMdbNtcSplit &tSplitCfg, TMdbNtcSplit &tSplitPeer);
        void ParseCSIPCfg();//�������Ƶ�ip�б�
        
        /******************************************************************************
        * ��������  :  AddWorkThread
        * ��������  :  ���ӹ����߳�
        * ����      :
        * ���      :
        * ����ֵ    :
        * ����      :
        *******************************************************************************/
        int AddWorkThread();
		
        int ParseMsgHead(TAgentClient * ptClient);//�������հ�ͷ
    private:
        int Login(TAgentClient *ptClient) throw (TMdbException);//��½, ������Ȩ����֤
        void ClientExit(TAgentClient *&ptClient);//�߳̽�������β����
        int  PrintRecvMsg(TAgentClient *ptClient);//��ӡ��������
        void PrintMsg(TAgentClient *ptClient,unsigned char *caTemp,int iMsgLen);//��ӡ16��������
        int  PrintSendMsg(TAgentClient *ptClient);//��ӡ��������
        int DoOperation(TAgentClient *ptClient);//�������Ӧ��
        int AppErrorAnswer(TAgentClient *ptClient,int iAnsCode,const char *fmt,...);//������ϢӦ��
        int AppSendSQL(TAgentClient *ptClient);//SQLע�ᣬ���ֶ�̬��̬SQL
        int AppSendParam(TAgentClient *ptClient);//���������Ĳ���
        int AppSendParamArray(TAgentClient *ptClient);//���������Ĳ�������
        int AppNextSQLResult(TAgentClient *ptClient);//��Ӧnext����
        int AppTransAction(TAgentClient *ptClient);//������
        int AppGetSequence(TAgentClient *ptClient);//��ȡ����
        int FillNextResult(TAgentClient *ptClient,TMdbQuery * pQuery,TMdbCspParser * pSendSQLAns)throw(TMdbException,TMdbCSPException);//����ѯ��next ����ֵ
        int AppGetQmdbInfo(TAgentClient *ptClient);//��ȡqmdb ��Ϣ��
        
        int SendPackageBody(TAgentClient *ptClient,int iSendLen);// ���Ͱ���
        TAgentClient * GetFreeAgentClient(TMdbConnectEvent * pEventInfo);//��ȡ����agent client
        int Authentication(TAgentClient *ptClient);//��֤
        int SendAnswer(TMdbCspParser * pCspParser,TAgentClient *ptClient,int AnsCode,const char * sMsg);//���ظ���Ϣ
        int CheckClientSequence(TAgentClient * ptClient);//���ͻ������
    	void DumpRecvPackage(TAgentClient *ptClient);//��־����-1ʱץ���ݰ�-���հ�
    	void DumpSendPackage(TAgentClient *ptClient);//��־����-1ʱץ���ݰ�-���Ͱ�
    	void DumpPackage(TAgentClient *ptClient, bool bSendFlag);  //  ������ݰ�
    private:
        TMdbShmDSN* m_pShmDSN;
        TMdbConfig *m_pConfig;
        TMdbDSN    *m_pDsn;
        TAgentClient *m_ptClient[MAX_CS_THREAD_COUNTS];
        int m_iClientThreadNum;//���տͻ������ӵ��߳���Ŀ
        static int m_iSessionStep;//�ỰID
        TMutex  tMutex;
        TMutex  tMutexConnect;
        TMdbProcCtrl m_tProcCtrl;//���̿���
        TMdbLinkCtrl m_tLinkCtrl;//���ӹ���
    	TAgentFile m_tAgentFile;//ץ���ݰ�д�����ļ�
        std::vector<string> m_vValidIP;//��Чip
        std::vector<string> m_vInValidIP;//��Чip
    	bool m_bRecvFlag;//�������ݰ����
        int m_iPumpMaxCount;//�����߳����ֵ
        int m_iPeerCountPerPump;//ÿ���߳�������
    };


//}

#endif //__MINI_DATABASE_AGENT_SERVER_H__

