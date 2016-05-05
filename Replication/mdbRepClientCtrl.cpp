/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbRepClientCtrl.cpp		
*@Description�� �����Ƭ����ͬ�����ݵķ���
*@Author:		jiang.lili
*@Date��	    2014/03/20
*@History:
******************************************************************************************/
#include "Replication/mdbRepClientCtrl.h"
//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;
//namespace QuickMDB
//{
    TRepClientEngine::TRepClientEngine():m_pClient(NULL)
    {
        m_pShmRep = NULL;
        m_pShmMgr = NULL;
    }

    TRepClientEngine::~TRepClientEngine()
    {

    }
       /******************************************************************************
        * ��������	:  Init
        * ��������	:  ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TRepClientEngine::Init(const TMdbShmRepHost &tHost, const char* sFilePath, time_t tFileInvalidTime, const char* sDsn)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        m_iHostID = tHost.m_iHostID;
        m_strIP.Assign(tHost.m_sIP);
        m_iPort = tHost.m_iPort;
        m_iFileInvalidTime = tFileInvalidTime;
        CHECK_OBJ(sFilePath);
        m_strPath = sFilePath;
        if (m_strPath[m_strPath.GetLength()-1]!='/')
        {
            m_strPath.Append("/");
        }

        m_pShmMgr = new (std::nothrow)TMdbShmRepMgr(sDsn);
         CHECK_OBJ(m_pShmMgr);
         CHECK_RET(m_pShmMgr->Attach(),"Create shm failed."); 

         m_pShmRep = (TMdbShmRep*)m_pShmMgr->GetRoutingRep();
         CHECK_OBJ(m_pShmRep);

        char sFilePatten[MAX_NAME_LEN];//�ļ����Ƶĸ�ʽ Rep.HostID.*.OK
        snprintf(sFilePatten, MAX_NAME_LEN, "Rep.%d.*.OK", m_iHostID);        
        m_tFileScanner.AddFilter(sFilePatten);

        TADD_FUNC("Finish.");
        return iRet;
    }
       /******************************************************************************
        * ��������	:  Execute
        * ��������	:  �߳�ִ�к���
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TRepClientEngine::Execute()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_pClient = new(std::nothrow) TMdbRepDataClient();
        CHECK_OBJ(m_pClient);

       TADD_NORMAL("thread[%d] Start.", GetThreadId());
        while(true)
        {
        	time(&m_tThreadUpdateTime);
            if(this->TestCancel())
            {
                TADD_NORMAL("thread[%d] cancel", GetThreadId());
                break;
            }            
            
            if (m_pClient->NeedReconnect())//�Ƿ���Ҫ���������
            {
                if (!m_pClient->Connect(m_strIP.c_str(), m_iPort))//���Ӷ�Ӧ�ı���
                {
                    //����ʧ�ܣ�ɾ�������ļ�
                    iRet = CheckRepFile();
                    if (iRet != ERROR_SUCCESS)
                    {
                        TADD_ERROR(iRet,"CheckRepFile failed.");
                    }
                }  
            }
            
            //����ͬ���ļ�����ͬ���ļ���sleep 1s
            if (E_REP_NTC_OK == m_pClient->m_eState)
            {
                iRet = SendRepFile();
                if (iRet!=ERROR_SUCCESS)
                {
                    TADD_ERROR(ERR_NET_SEND_FAILED, "SendRepFile failed.");
                }
                //��Ӧ���������
                /*QuickMDB::*/TMdbMsgInfo* pMsg=NULL;
                iRet = m_pClient->GetMsg(pMsg);
                if (iRet == 0)
                {
                    DoServerCmd(pMsg);
                }
            }
            else
            {
                TMdbNtcDateTime::Sleep(1000);
            }
            
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }

        /******************************************************************************
        * ��������	:  CheckRepFile
        * ��������	:  ���ͬ���ļ��Ƿ���ڣ�ɾ������ͬ���ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TRepClientEngine::CheckRepFile()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //����Ŀ¼�µ��ļ�
        m_tFileScanner.ClearResult();
        if (!m_tFileScanner.ScanFile(m_strPath.c_str()))
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "ScanFile in path[%s] failed.", m_strPath.c_str());
        }
        const char* pFileName = NULL;
        TMdbNtcSplit tSplit;

        while((pFileName=m_tFileScanner.GetNext())!=NULL)
        {
            tSplit.SplitString(pFileName, '.');
            //�ļ��Ƿ����
            if (/*QuickMDB::*/TMdbNtcDateTime::GetDiffSeconds(atoi(tSplit[2]), /*QuickMDB::*/TMdbNtcDateTime::GetCurTime())>m_iFileInvalidTime)
            {
                TADD_NORMAL("Remove file=[%s]",pFileName);
                /*QuickMDB::*/TMdbNtcFileOper::Remove(pFileName);
            }
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

        /******************************************************************************
        * ��������	:  SendRepFile
        * ��������	:  ����ͬ���ļ�����ͬ���ļ�����sleep 1s
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TRepClientEngine::SendRepFile()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        //����Ŀ¼�µ��ļ�
        m_tFileScanner.ClearResult();
        if (!m_tFileScanner.ScanFile(m_strPath.c_str()))
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "ScanFile in path[%s] failed.", m_strPath.c_str());
        }
        //û���ļ�����sleep 1s
        if (m_tFileScanner.GetCount() == 0)
        {
            /*QuickMDB::*/TMdbNtcDateTime::Sleep(1000);
            m_pClient->SendHearbeat();//��������������1.2��
            return iRet;
        }
        m_tFileScanner.Sort();
        
        //�����ļ�
        const char* pFileName = NULL;
        /*QuickMDB::*/TMdbNtcString sFileFullName;

        while((pFileName=m_tFileScanner.GetNext())!=NULL)
        {
            TADD_NORMAL("Remove file=[%s]",pFileName);
            CHECK_RET(m_pClient->SendData(pFileName),"Send file[%s] failed.", pFileName);
            /*QuickMDB::*/TMdbNtcFileOper::Remove(pFileName);
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

         /******************************************************************************
        * ��������	:  DealRepServerCmd
        * ��������	: �������÷��񱸻�������ͬ����Ϣ
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TRepClientEngine::DoServerCmd(/*QuickMDB::*/TMdbMsgInfo* pMsg)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        CHECK_OBJ(pMsg);
        CHECK_OBJ(pMsg->GetBuffer());
        
        if (strlen(pMsg->GetBuffer())>=strlen(REP_HEART_BEAT) && strncmp(pMsg->GetBuffer(), REP_HEART_BEAT, strlen(REP_HEART_BEAT)) == 0)
        {
            TADD_NORMAL("Get heartbeat check from server.");
            m_pClient->SendHearbeat();
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "Unknown command.");
        }

        TADD_FUNC("Finish");
        return iRet;
    }


    int TRepClientEngine::KillRepDataClient()
    {
        int iRet = 0;
        if(!m_pClient) return iRet;		
		m_pClient->Kill();
		return iRet;
	}
		 

    TRepClient::TRepClient():m_pShmMgr(NULL),m_pRepConfig(NULL)
    {
        memset(m_sDsn, 0, sizeof(m_sDsn));

    }

    TRepClient::~TRepClient()
    {
        m_pShmMgr->Detach();
        SAFE_DELETE(m_pShmMgr);
        SAFE_DELETE(m_pRepConfig);
    }
       /******************************************************************************
        * ��������	:  Init
        * ��������	:  ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TRepClient::Init(const char *sDsn)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_pShmMgr = new(std::nothrow) TMdbShmRepMgr(sDsn);
        CHECK_OBJ(m_pShmMgr);
        CHECK_RET(m_pShmMgr->Attach(),"Attach to shared memory failed.");

        m_pRepConfig = new(std::nothrow) TMdbRepConfig();
        CHECK_OBJ(m_pRepConfig);
        CHECK_RET(m_pRepConfig->Init(sDsn), "TMdbRepConfig failed.");

        m_tProcCtrl.Init(sDsn);

        SAFESTRCPY(m_sDsn, sizeof(m_sDsn), sDsn);

        TADD_FUNC("Finish.");
        return iRet;
    }
       /******************************************************************************
        * ��������	:  Start
        * ��������	:  ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TRepClient::Start()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //�������б�����ÿ������Ϊһ���߳�
        TADD_NORMAL("Rep host count = [%d]", m_pShmMgr->GetRoutingRep()->m_iRepHostCount);
        for (int i = 0; i<m_pShmMgr->GetRoutingRep()->m_iRepHostCount; i++)
        {
            TRepClientEngine *pClientEngine = new(std::nothrow)TRepClientEngine();
            CHECK_OBJ(pClientEngine);
            CHECK_RET(pClientEngine->Init(m_pShmMgr->GetRoutingRep()->m_arHosts[i], m_pRepConfig->m_sRepPath.c_str(), m_pRepConfig->m_iFileInvalidTime,m_sDsn), "TRepClientEngine init failed.");
            if(!pClientEngine->Run())
            {
                CHECK_RET(ERR_OS_CREATE_THREAD, "TRepClientEngine Run failed.");
            }
            m_arThreads.Add(pClientEngine);
        }
        
        //���̼߳���Ƿ���·�ɱ��,������߳�,�ȴ��˳�����
        while(true)
        {
            if(m_tProcCtrl.IsCurProcStop())
            {
                TADD_NORMAL("mdbRepClient stop.");
                WaitAllThreadQuit();
                break;
            }
            m_tProcCtrl.UpdateProcHeart(0);
            
            if (m_pShmMgr->GetRoutingRep()->m_bRoutingChange)
            {
                CHECK_RET(DealRoutingChange(), "DealRoutingChange failed.");
            }

			CheckThreadHeart();
            TMdbNtcDateTime::Sleep(1000);
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

       /******************************************************************************
        * ��������	:  DealRoutingChange
        * ��������	:  ����·�ɱ������, ��ֹ������Ҫ�ı������Ӵ����̣߳�Ϊ�µ����������߳�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TRepClient::DealRoutingChange()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        TADD_FUNC("Finish.");
        return iRet;
    }

        /******************************************************************************
        * ��������	:  WaitAllThreadQuit
        * ��������	:  �ȴ������߳̽���
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TRepClient::WaitAllThreadQuit()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TRepClientEngine *pThread = NULL;
        
        for(unsigned int i=0; i<m_arThreads.GetSize(); i++)
        {
            pThread = dynamic_cast<TRepClientEngine*>(m_arThreads[i]);
            pThread->Cancel();
            
            while(false == pThread->Wait())
            {
                TADD_NORMAL("waiting for thread[%d] to cancel", pThread->GetThreadId());
                /*QuickMDB::*/TMdbNtcDateTime::Sleep(5);
            }
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

	int TRepClient::CheckThreadHeart()
	{
		TADD_FUNC("Start.");
        int iRet = 0;
		int iHearBeatFatal = 30; // ������ʱʱ�䣬��λ:��
		
        TRepClientEngine *pThread = NULL;
		time_t tCurTime;
   		time(&tCurTime);
        
        for(unsigned int i=0; i<m_arThreads.GetSize(); i++)
        {
            pThread = dynamic_cast<TRepClientEngine*>(m_arThreads[i]);
			if (0 == pThread->m_tThreadUpdateTime) continue;
			if (tCurTime - pThread->m_tThreadUpdateTime > iHearBeatFatal)
			{
				TADD_WARNING("Thread %d  lost heartbeat,difftime= %d s,will restart.",
					pThread->GetThreadId(), tCurTime - pThread->m_tThreadUpdateTime);
				
				//��Ҫ��ֹͣ���Ϳͻ���
				pThread->KillRepDataClient();
				//���������߳�
				pThread->Kill();

				//�������߳�
				if(!pThread->Run())
	            {
	                CHECK_RET(ERR_OS_CREATE_THREAD, "TRepClientEngine Run failed When Restart.");
	            }
				else
				{
					TADD_WARNING("Restart thread success,new thread id = %d.", pThread->GetThreadId());
				}
			}           
        }

        TADD_FUNC("Finish.");
        return iRet;
	}

//}
