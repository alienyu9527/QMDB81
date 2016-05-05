/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbRepClientCtrl.cpp		
*@Description： 负责分片备份同步数据的发送
*@Author:		jiang.lili
*@Date：	    2014/03/20
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
        * 函数名称	:  Init
        * 函数描述	:  初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
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

        char sFilePatten[MAX_NAME_LEN];//文件名称的格式 Rep.HostID.*.OK
        snprintf(sFilePatten, MAX_NAME_LEN, "Rep.%d.*.OK", m_iHostID);        
        m_tFileScanner.AddFilter(sFilePatten);

        TADD_FUNC("Finish.");
        return iRet;
    }
       /******************************************************************************
        * 函数名称	:  Execute
        * 函数描述	:  线程执行函数
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
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
            
            if (m_pClient->NeedReconnect())//是否需要重连服务端
            {
                if (!m_pClient->Connect(m_strIP.c_str(), m_iPort))//连接对应的备机
                {
                    //连接失败，删除过期文件
                    iRet = CheckRepFile();
                    if (iRet != ERROR_SUCCESS)
                    {
                        TADD_ERROR(iRet,"CheckRepFile failed.");
                    }
                }  
            }
            
            //发送同步文件，无同步文件则sleep 1s
            if (E_REP_NTC_OK == m_pClient->m_eState)
            {
                iRet = SendRepFile();
                if (iRet!=ERROR_SUCCESS)
                {
                    TADD_ERROR(ERR_NET_SEND_FAILED, "SendRepFile failed.");
                }
                //响应服务端命令
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
        * 函数名称	:  CheckRepFile
        * 函数描述	:  检查同步文件是否过期，删除过期同步文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TRepClientEngine::CheckRepFile()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //遍历目录下的文件
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
            //文件是否过期
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
        * 函数名称	:  SendRepFile
        * 函数描述	:  发送同步文件，无同步文件，则sleep 1s
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TRepClientEngine::SendRepFile()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        //遍历目录下的文件
        m_tFileScanner.ClearResult();
        if (!m_tFileScanner.ScanFile(m_strPath.c_str()))
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "ScanFile in path[%s] failed.", m_strPath.c_str());
        }
        //没有文件，则sleep 1s
        if (m_tFileScanner.GetCount() == 0)
        {
            /*QuickMDB::*/TMdbNtcDateTime::Sleep(1000);
            m_pClient->SendHearbeat();//发送心跳（兼容1.2）
            return iRet;
        }
        m_tFileScanner.Sort();
        
        //发送文件
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
        * 函数名称	:  DealRepServerCmd
        * 函数描述	: 处理配置服务备机发来的同步信息
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
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
        * 函数名称	:  Init
        * 函数描述	:  初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
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
        * 函数名称	:  Start
        * 函数描述	:  初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TRepClient::Start()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //连接所有备机，每个连接为一个线程
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
        
        //主线程检查是否有路由变更,监控子线程,等待退出命令
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
        * 函数名称	:  DealRoutingChange
        * 函数描述	:  处理路由变更命令, 终止不再需要的备机连接处理线程，为新的主机创建线程
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TRepClient::DealRoutingChange()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        TADD_FUNC("Finish.");
        return iRet;
    }

        /******************************************************************************
        * 函数名称	:  WaitAllThreadQuit
        * 函数描述	:  等待所有线程结束
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
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
		int iHearBeatFatal = 30; // 心跳超时时间，单位:秒
		
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
				
				//需要先停止发送客户端
				pThread->KillRepDataClient();
				//再销毁主线程
				pThread->Kill();

				//重启主线程
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
