/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbRepClientCtrl.h	
*@Description�� �����Ƭ����ͬ�����ݵķ���
*@Author:		jiang.lili
*@Date��	    2014/03/20
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_REP_CLIENT_CTRL_T_H__
#define __ZX_MINI_DATABASE_REP_CLIENT_CTRL_T_H__

#include "Control/mdbRepCommon.h"
#include "Replication/mdbRepNTC.h"
#include "Control/mdbProcCtrl.h"
#include "Helper/mdbQueue.h"
#include "Replication/mdbRepLog.h"
#include "Control/mdbStorageEngine.h"

//namespace QuickMDB
//{
    /**
    * @brief �ͻ��˴����߳��࣬��Ӧһ̨����
    * 
    */
    class TRepClientEngine: public TMdbNtcThread
    {
    public:
        TRepClientEngine();
        ~TRepClientEngine();
    public:
        /******************************************************************************
        * ��������	:  Init
        * ��������	:  ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Init(const TMdbShmRepHost &tHost, const char* sFilePath, time_t tFileInvalidTime, const char* sDsn);

        /******************************************************************************
        * ��������	:  GetHostID
        * ��������	:  ��ȡ���̶߳�Ӧ��������ʶ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int GetHostID()
        {
            return m_iHostID;
        }
		
		int KillRepDataClient();
    protected:
       /******************************************************************************
        * ��������	:  Execute
        * ��������	:  �߳�ִ�к���
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        virtual int  Execute();//�̵߳�ִ�к���
    private:
		int UpdateRepMode(bool bRollback = false);//���µ�ǰͬ��ģʽ
		int UpdateOnlineRepMode(bool bRollback = false);
        /******************************************************************************
        * ��������	:  CheckRepFile
        * ��������	:  ���ͬ���ļ��Ƿ����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int CheckRepFile();
		/******************************************************************************
		* ��������	:  CheckShardBakBuf
		* ��������	:  �����·����ʹ�����
		* ����		:  
		* ���		:  
		* ����ֵ	:  0 - �ɹ�!0 -ʧ��
		* ����		:  jiang.lili
		*******************************************************************************/
		int CheckShardBakBuf();
		/******************************************************************************
		* ��������	:  CheckOverdueRepFile
		* ��������	:  ���ͬ���ļ��Ƿ���ڣ�ɾ������ͬ���ļ�
		* ����		:  
		* ���		:  
		* ����ֵ	:  0 - �ɹ�!0 -ʧ��
		* ����		:  jiang.lili
		*******************************************************************************/
		int CheckOverdueRepFile();

        /******************************************************************************
        * ��������	:  SendRepFile
        * ��������	:  ����ͬ���ļ�����ͬ���ļ�����sleep 1s
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int SendRepFile();

		/******************************************************************************
		* ��������	:  SendRepData
		* ��������	:  ����ͬ����¼����ͬ���ļ�����sleep 10ms
		* ����		:  
		* ���		:  
		* ����ֵ	:  0 - �ɹ�!0 -ʧ��
		* ����		:  jiang.xiaolong
		*******************************************************************************/
		int SendRepData();
        /******************************************************************************
        * ��������	:  DoServerCmd
        * ��������	:  ��Ӧserver�����Ŀǰ����Ӧ��������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int DoServerCmd(TMdbMsgInfo* pMsg);
    private:
        int m_iHostID;//������ʶ
        TMdbNtcString m_strIP;//IP
        int m_iPort;//Port
		bool m_bSameEndian;//�ֽ����Ƿ�һ��
        TMdbNtcFileScanner m_tFileScanner;
		TMdbShmDSN * m_pShmDSN;
  
        TMdbNtcString m_strPath;//ͬ���ļ���Ŀ¼
        time_t m_iFileInvalidTime;//ͬ���ļ��Ĺ���ʱ��
        TMdbRepDataClient *m_pClient;//���Ϳͻ���     
        TMdbShmRepMgr* m_pShmMgr;
        TMdbShmRep* m_pShmRep;//�����ڴ�ָ��
        bool m_bRepFileExist;//�Ƿ��ж�Ӧ��ͬ���ļ�����
        bool m_bShardBakBufFree;//��·�������㹻����
        TMdbDSN * m_pMdbDSN;//�Ƿ����÷���ģʽͬ��
        TMdbOnlineRepQueue m_pOnlineRepQueueCtrl;
    };


	class TMdbWriteRepLog:public TMdbNtcThread
	{
	public:
		TMdbWriteRepLog();
		~TMdbWriteRepLog();
		int Init(int iHostID, const char* sDsn, const char* sFilePath);
		int UpdateRepMode(bool bRollback = false);//���µ�ǰͬ��ģʽ
		int UpdateOnlineRepMode(bool bRollback = false);
        /******************************************************************************
        * ��������	:  CheckRepFile
        * ��������	:  ���ͬ���ļ��Ƿ����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int CheckRepFile();
		/******************************************************************************
		* ��������	:  CheckShardBakBuf
		* ��������	:  �����·����ʹ�����
		* ����		:  
		* ���		:  
		* ����ֵ	:  0 - �ɹ�!0 -ʧ��
		* ����		:  jiang.lili
		*******************************************************************************/
		int CheckShardBakBuf();
        int  Execute();
        void Cleanup();
	private:
		int m_iHostID;
		TMdbOnlineRepQueue m_tOnlineRepQueueCtrl;//�ڴ滺�������   
        TMdbShmRepMgr* m_pShmMgr;
        TMdbShmRep* m_pShmRep;//�����ڴ�ָ��
        TMdbShmDSN * m_pShmDsn;
        TMdbDSN * m_pDsn;
        TRoutingRepLog m_mdbReplog;
        TMdbRedoLogParser m_LogParser;
        TMdbConfig  *m_pConfig;
        bool m_bRepFileExist;//�Ƿ��ж�Ӧ��ͬ���ļ�����
        bool m_bShardBakBufFree;//��·�������㹻����
        TMdbNtcFileScanner m_tFileScanner;
		TMdbNtcString m_strPath;//ͬ���ļ���Ŀ¼
	};


    /**
    * @brief ��Ƭ����ͬ���ͻ���
    * 
    */
    class TRepClient
    {
    public:
        TRepClient();
        ~TRepClient();
       /******************************************************************************
        * ��������	:  Init
        * ��������	:  ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Init(const char *sDsn, int iHostID);
       /******************************************************************************
        * ��������	:  Start
        * ��������	:  ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int Start();
       /******************************************************************************
        * ��������	:  DealRoutingChange
        * ��������	:  ����·�ɱ������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int DealRoutingChange();//����·�ɱ��
        /******************************************************************************
        * ��������	:  WaitAllThreadQuit
        * ��������	:  �ȴ������߳̽���
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
        int WaitAllThreadQuit();

		/******************************************************************************
        * ��������	:  CheckThreadsHeartBeat
        * ��������	:  ����߳�����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  yu.lianxiang
        *******************************************************************************/
        int CheckThreadHeart(); 

    private:
        TMdbShmRepMgr *m_pShmMgr;//�����ڴ������ָ��
        TMdbRepConfig *m_pRepConfig;//��Ƭ���������ļ�
        TMdbNtcAutoArray m_arThreads;//���з���ͬ�����ݵ����߳�
        char m_sDsn[MAX_NAME_LEN];
		int m_iHostID;
		TRepClientEngine * m_pClientEngine;
		TMdbWriteRepLog * m_pWriteRepLog;

        TMdbProcCtrl m_tProcCtrl;
    };

//}

#endif //__ZX_MINI_DATABASE_REP_CLIENT_CTRL_T_H__
