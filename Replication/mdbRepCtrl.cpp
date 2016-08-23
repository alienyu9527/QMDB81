/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepCtrl.cpp	
*@Description: ��Ƭ���������̹����࣬���������÷���ͨ�ź���������
*@Author:		jiang.lili
*@Date��	    2014/05/4
*@History:
******************************************************************************************/
//#include "BillingSDK.h"
#include "Replication/mdbRepCtrl.h"
#include "Replication/mdbRepLoadData.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbDateTime.h"
#include "Replication/mdbServerConfig.h"

//using namespace ZSmart::BillingSDK;
//namespace QuickMDB
//{
    TMdbRepCtrl::TMdbRepCtrl():m_pMDB(NULL), m_pRepConfig(NULL), m_pMonitorClient(NULL), m_pShmMgr(NULL),m_bSvrConn(false)
    {

    }

    TMdbRepCtrl::~TMdbRepCtrl()
    {
        SAFE_DELETE(m_pMDB);
        SAFE_DELETE(m_pRepConfig);
        SAFE_DELETE(m_pShmMgr);
    }

    /******************************************************************************
    * ��������	:  Init()
    * ��������	:  ��ʼ��
    * ����		:  sDsn mdb��Dsn����
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbRepCtrl::Init(const char* sDsn)
    {
        int iRet = 0;
        TADD_FUNC("Start.");

        CHECK_OBJ(sDsn);
        SAFESTRCPY(m_sDsn, sizeof(m_sDsn), sDsn);
        TMdbNtcStrFunc::ToUpper(m_sDsn);

        //��ȡ�����ļ�
        m_pRepConfig = new (std::nothrow)TMdbRepConfig();
		CHECK_OBJ(m_pRepConfig);
        CHECK_RET(m_pRepConfig->Init(sDsn), "RepConfig init failed.");
 
        //���ӹ����ڴ�
        m_pShmMgr = new (std::nothrow)TMdbShmRepMgr(sDsn);
        CHECK_OBJ(m_pShmMgr);
        //CHECK_RET(m_pShmMgr->Create(),"Create shm failed.");
        CHECK_RET(m_pShmMgr->Attach(),"Attach shm failed.");
        
        //����MDB
        m_pMDB = new(std::nothrow)TMdbDatabase();
        CHECK_OBJ(m_pMDB);
        CHECK_RET(ConnectMDB(), "Connect to MDB [%s] failed.", m_sDsn);

        //�������÷���
        m_pMonitorClient = new(std::nothrow) TMdbRepMonitorClient();
        CHECK_OBJ(m_pMonitorClient);
        CHECK_RET(ConnectServer(), "Connect to configuration server failed.");

        m_tProcCtrl.Init(sDsn);

        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  LoadData()
    * ��������	:  ���ط�Ƭ���ݵ�������QuickMDB��  ��������, �����Գ���ʹ�ã�ͨ���趨��ͬ��port��ͬһ̨���������������qmdb
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  jiang.lili
    *******************************************************************************/
    #if 0
    int TMdbRepCtrl::LoadData(const char* sDsn, int iPort)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TMdbRepLoadDataCtrl *pLoadData = new(std::nothrow) TMdbRepLoadDataCtrl();
        CHECK_RET(pLoadData->Init(sDsn, iPort),"TMdbRepLoadDataCtrl init failed.");
        pLoadData->LoadData();

        TADD_FUNC("Finish");
        return iRet;
    }
    #endif

        /******************************************************************************
    * ��������	:  LoadData()
    * ��������	:  ���ط�Ƭ���ݵ�������QuickMDB
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  jiang.lili
    *******************************************************************************/
    #if 0
    int TMdbRepCtrl::LoadData(const char* sDsn, TMdbConfig* pMdbCfg)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TMdbRepLoadDataCtrl *pLoadData = new(std::nothrow) TMdbRepLoadDataCtrl();
        CHECK_RET(pLoadData->Init(sDsn,pMdbCfg),"TMdbRepLoadDataCtrl init failed.");
        pLoadData->LoadData();

        TADD_FUNC("Finish");
        return iRet;
    }
    #endif

    /******************************************************************************
    * ��������	:  Start()
    * ��������	:  ������Ƭ���ݸ����ӽ��̣���������ͬ����״̬�ϱ���·�ɱ������
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbRepCtrl::Start()
    {
        int iRet = 0;
        TADD_FUNC("Start.");

        //�ϱ�״̬
        CHECK_RET(m_pShmMgr->SetRunningFlag(true), "Update running state failed.");//����mdbRep����״̬
        m_pShmMgr->SetMdbState(E_MDB_STATE_RUNNING);
        if(m_bSvrConn)
        {
            TADD_NORMAL("Report state [%d]", E_MDB_STATE_RUNNING);
            CHECK_RET(m_pMonitorClient->ReportState(m_pShmMgr->GetRoutingRep()->m_iHostID, E_MDB_STATE_RUNNING), "Report state failed.");
        }
        
        //���ͬ�����̵����У�����·�ɱ������
        TRepNTCMsg tMsg;//����mdbServer���յ�����Ϣ
        while(true)
        {
            if(m_tProcCtrl.IsCurProcStop())
            {
                
                TADD_NORMAL("mdbRep stop.");
                break;
            }
            m_tProcCtrl.UpdateProcHeart(0);

            if (m_pMonitorClient->NeedReconnect())
            {
                TADD_NORMAL("Reconnect to mdbServer.");
                CHECK_RET(ConnectServer(), "Connect to configuration server failed.");
                if(m_bSvrConn) // �����ɹ����ϱ�һ�ε�ǰ״̬
                {
                    //  �����ϱ�һ�ε�ǰ״̬
                    TADD_NORMAL("Report state [%d]", m_pShmMgr->GetRoutingRep()->m_eState);
                    CHECK_RET(m_pMonitorClient->ReportState(m_pShmMgr->GetRoutingRep()->m_iHostID, m_pShmMgr->GetRoutingRep()->m_eState), "Report state failed.");

                    // ͬ��һ�η�Ƭ����������Ϣ(�ݲ����ǶϿ��ڼ�·�ɱ�����⣩
                    CHECK_RET(m_pMonitorClient->RoutingRequestNoShm(m_pShmMgr->GetRoutingRep()->m_iHostID), "Report state failed.");
                    TShbRepLocalConfig tLocalConfig;
                    CHECK_RET(tLocalConfig.SyncLocalConfig(m_sDsn,m_pMonitorClient->GetSvrConfigMsg()),"sync Local config failed");
                }
                else
                {
                    TMdbNtcDateTime::Sleep(100);
                }
            }
         
            //ִ��mdbServer���͹���������
            iRet = m_pMonitorClient->GetMsg(tMsg);
            if (iRet == 0)
            {
                DealServerCmd(tMsg);
            }
        }

        //�ϱ�״̬��ͬ�����̱�ֹͣ
        m_pShmMgr->SetMdbState(E_MDB_STATE_CREATED);
        if(m_bSvrConn)
        {
            TADD_NORMAL("Report state [%d]", E_MDB_STATE_CREATED);
            CHECK_RET(m_pMonitorClient->ReportState(m_pShmMgr->GetRoutingRep()->m_iHostID, E_MDB_STATE_CREATED), "Report state failed");    
            TMdbNtcDateTime::Sleep(100);//������sleep��mdbServer�ղ���״̬��Ϣ
            m_pMonitorClient->Disconnect();
        }
        
        CHECK_RET(m_pShmMgr->SetRunningFlag(false), "Update running state failed.");//����mdbRep����״̬
        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  ConnectMDB()
    * ��������	:  ������Ƭ���ݸ����ӽ��̣���������ͬ����״̬�ϱ���·�ɱ������
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbRepCtrl::ConnectMDB()
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        try
        {
            if (!m_pMDB->ConnectAsMgr(m_sDsn))
            {
                CHECK_RET(ERR_APP_INVALID_PARAM,"Connect to DSN[%s] failed.", m_sDsn);
            }
             
        }
        catch (TMdbException& e)
        {
            iRet =ERR_APP_INVALID_PARAM;
            TADD_ERROR(ERR_APP_INVALID_PARAM, "Connect(%s) failed. \nERROR_SQL=%s.\nERROR_MSG=%s\n", m_sDsn, e.GetErrSql(), e.GetErrMsg());
        }   
        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  ConnectServer()
    * ��������	:  �������÷���
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbRepCtrl::ConnectServer()
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        m_bSvrConn = false;
        if (m_pMonitorClient->Connect(m_pRepConfig->m_sServerIP.c_str(), m_pRepConfig->m_iServerPort))
        {
            TADD_NORMAL("Connect to config server[%s:%d] OK.", m_pRepConfig->m_sServerIP.c_str(), m_pRepConfig->m_iServerPort);
            m_bSvrConn = true;
        }
        else if (m_pMonitorClient->Connect(m_pRepConfig->m_sRepServerIP.c_str(), m_pRepConfig->m_iRepServerPort))
        {
            TADD_NORMAL("Connect to config server[%s:%d] OK.", m_pRepConfig->m_sRepServerIP.c_str(), m_pRepConfig->m_iRepServerPort);
            m_bSvrConn = true;
        }
        else
        {
            //֧�ֲ������÷����
            TADD_NORMAL("Start not connecting to any mdbServer.");
            m_bSvrConn = false;
        }

        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  DealServerCmd()
    * ��������	:  �����������÷���������·�ɱ����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbRepCtrl::DealServerCmd(TRepNTCMsg &tCmd)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        switch(tCmd.eEvent)
        {
        case 0:
            {
                TADD_FUNC("Get heartbeat check from server.");
                m_pMonitorClient->SendHearbeat();
                break;
            }
        case E_SVR_ROUTING_CHANGE:
            {
                //ͨ��m_pShmMgr���޸Ĺ����ڴ���·�ɶ�Ӧ��ϵ
                break;
            }
        default:
            {
                CHECK_RET(ERROR_UNKNOWN, "Unknown cmd.");
            }
        }

        TADD_FUNC("Finish");
        return iRet;
    }

    int TMdbRepCtrl::ReportState(const char*sDsn, int iHostID, EMdbRepState eState)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
       TMdbRepConfig *pConfig = new (std::nothrow)TMdbRepConfig();
	   if(pConfig ==  NULL)
	   	{
			TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new TMdbRepConfig");
			return ERR_OS_NO_MEMROY;
	   	}
        CHECK_RET(pConfig->Init(sDsn), "RepConfig init failed.");
        TMdbRepMonitorClient *pMonitorClient = new(std::nothrow) TMdbRepMonitorClient();
        CHECK_OBJ(pMonitorClient);
        bool bConnect = false;
        //����
        if (pMonitorClient->Connect(pConfig->m_sServerIP.c_str(), pConfig->m_iServerPort))
        {
            bConnect = true;
            TADD_NORMAL("Connect to config server[%s:%d] OK.", pConfig->m_sServerIP.c_str(), pConfig->m_iServerPort);
        }
        else if (pMonitorClient->Connect(pConfig->m_sRepServerIP.c_str(), pConfig->m_iRepServerPort))
        {
            bConnect = true;
            TADD_NORMAL("Connect to config server[%s:%d] OK.", pConfig->m_sRepServerIP.c_str(), pConfig->m_iRepServerPort);
        }
        if(!bConnect)
        {
            TADD_ERROR(ERR_NET_PEER_INVALID, "Connect to config server failed.");
            return ERR_NET_PEER_INVALID;
        }
        if (bConnect)
        {
            CHECK_RET(pMonitorClient->ReportState(iHostID, eState), "Report state to mdbServer failed.");

            TMdbNtcDateTime::Sleep(100);//������sleep��mdbServer�ղ���״̬��Ϣ
        }

        SAFE_DELETE(pConfig);
        SAFE_DELETE(pMonitorClient);
        TADD_FUNC("Finish.");
        return iRet;
    }

    #if 0

    /******************************************************************************
    * ��������	:  StartRepProcess()
    * ��������	:  ����ͬ������
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbRepCtrl::StartRepProcess()
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        char strRepServerCmd[MAX_NAME_LEN];
        snprintf(strRepServerCmd, MAX_NAME_LEN, "%s %s", "mdbRepServer", m_sDsn);

        char strRepClientCmd[MAX_NAME_LEN];
        snprintf(strRepClientCmd, MAX_NAME_LEN, "%s %s", "mdbRepClient", m_sDsn);

        CHECK_RET(StartProcess(strRepServerCmd), "Start Process[%s] failed.", strRepServerCmd);
        CHECK_RET(StartProcess(strRepClientCmd), "Start Process[%s] failed.", strRepClientCmd);
        TADD_FUNC("Finish");
        return iRet;
    }

    int TMdbRepCtrl::StartProcess(const char* sProcName)
    {
        TADD_FUNC("Start.[%s]", sProcName);
        if(TMdbOS::IsProcExist(sProcName) ==  true)
        {
            //���̴���
            TADD_WARNING("Process=[%s] is running, can't restart.", sProcName);
            return 0;
        }
        else
        {
            //���̲�����
            system(sProcName);//��������
            TADD_DETAIL("system(%s) OK.",sProcName);
            TMdbNtcDateTime::Sleep(1000);
            //�ȴ���������
            int iCounts = 0;
            TMdbProc * pProc = NULL;
            while(true)
            {
                if(TMdbOS::IsProcExist(sProcName) ==  false || NULL == pProc)
                {
                    TMdbNtcDateTime::Sleep(1000);
                    if(iCounts%5 == 0)
                    {
                        TADD_NORMAL("Waiting for process=[%s] to start.", sProcName);
                    }
                    if(iCounts%31 == 30)
                    {
                        TADD_ERROR(ERR_SQL_EXECUTE_TIMEOUT,"Can't start process=[%s].", sProcName);
                        return ERR_SQL_EXECUTE_TIMEOUT;
                    }
                    ++iCounts;
                }
                else
                {
                    TADD_NORMAL("Start Process=[%s][%d].",pProc->sName,pProc->iPid);
                    break;
                }
            }
        }
        return 0;
    }
    
    /******************************************************************************
    * ��������	:  MonitorProcess()
    * ��������	:  ���ͬ�����̵�����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbRepCtrl::MonitorProcess()
    {
        int iRet = 0;
        TADD_FUNC("Start.");

        TADD_FUNC("Finish");
        return iRet;
    }
    #endif

	TMdbShardBuckupCfgCtrl::TMdbShardBuckupCfgCtrl()
	{
		m_sDsn = NULL;
		m_pRepConfig = NULL;
		m_bSvrConnFlag = false;
		m_pMonitorClient = NULL;
		m_pShmMgr = NULL;
	}

	TMdbShardBuckupCfgCtrl::~TMdbShardBuckupCfgCtrl()
	{
		SAFE_DELETE(m_pShmMgr);
		SAFE_DELETE(m_pMonitorClient); 
        SAFE_DELETE(m_pRepConfig); 
	}

	int TMdbShardBuckupCfgCtrl::Init(const char * pszDSN)
	{
		TADD_FUNC("START");
		int iRet = 0;
		m_sDsn = pszDSN;
		m_pRepConfig = new(std::nothrow)TMdbRepConfig();
        CHECK_OBJ(m_pRepConfig);
        CHECK_RET(m_pRepConfig->Init(m_sDsn),"TMdbRepConifg init failed");
		m_bSvrConnFlag = false;
		m_pMonitorClient = new(std::nothrow) TMdbRepMonitorClient();
	    CHECK_OBJ(m_pMonitorClient);
		m_pShmMgr = new (std::nothrow)TMdbShmRepMgr(m_sDsn);
	    CHECK_OBJ(m_pShmMgr);
		CHECK_RET(m_pShmMgr->Attach(), "MdbShmRepMgr Attach Failed.");
		TADD_FUNC("END");
		return iRet;
	}
	
	int TMdbShardBuckupCfgCtrl::GetShardBuckupInfo()
	{
		TADD_FUNC("START.");
		int iRet = 0;
		//���·�Ƭ����������Ϣ

        m_bSvrConnFlag = false;
	    if (m_pMonitorClient->Connect(m_pRepConfig->m_sServerIP.c_str(), m_pRepConfig->m_iServerPort))
	    {
	        m_bSvrConnFlag = true;
	        TADD_NORMAL("Connect to Config Server[%s:%d] OK.", m_pRepConfig->m_sServerIP.c_str(), m_pRepConfig->m_iServerPort);
	    }
	    else if(m_pMonitorClient->Connect(m_pRepConfig->m_sRepServerIP.c_str(), m_pRepConfig->m_iRepServerPort))
	    {
	        m_bSvrConnFlag = true;
	        TADD_NORMAL("Connect to Config Server[%s:%d] OK.", m_pRepConfig->m_sRepServerIP.c_str(), m_pRepConfig->m_iRepServerPort);
	    }
	    else
	    {
	        m_bSvrConnFlag = false;
	        //CHECK_RET(-1, "Connect to Config Server failed.");
	    }
		
		if(m_bSvrConnFlag)
		{
	        int iHostID = 0;
	        int iHeartbeat = 0;
			CHECK_RET(m_pMonitorClient->Register(m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort, iHostID,iHeartbeat), "Register Failed.");
			m_pShmMgr->SetHostID(iHostID);
	        m_pShmMgr->SetHeartbeat(iHeartbeat);
	        m_pShmMgr->SetMdbState(E_MDB_STATE_CREATING);
			CHECK_RET(m_pMonitorClient->ReportState(iHostID, E_MDB_STATE_CREATING), "ReportState failed.");      

	        //(2)�����÷�������·����Ϣ��д�빲���ڴ�
	        CHECK_RET(m_pMonitorClient->RoutingRequest(iHostID, m_pShmMgr), "RoutingRequest failed.");
	        //(8) ͬ����Ƭ�������õ����������ļ�
	        TShbRepLocalConfig tLocalConfig;
	        CHECK_RET(tLocalConfig.SyncLocalConfig(m_sDsn,m_pMonitorClient->GetSvrConfigMsg()),"sync Local config failed");
		}
		else
		{
			char sConfigStr[MAX_REP_SEND_BUF_LEN] = {0};

	        TShbRepLocalConfig tLocalConfig;
	        CHECK_RET(tLocalConfig.ReadLocalConfig(m_sDsn,sConfigStr, sizeof(sConfigStr)),"Get Local config content failed");
	        
	        CHECK_RET(m_pShmMgr->WriteRoutingInfo(sConfigStr, sizeof(sConfigStr)),"WriteLocalRoutingInfo failed");
	        m_pShmMgr->SetHostID(tLocalConfig.m_iHostID);
	        m_pShmMgr->SetHeartbeat(tLocalConfig.m_iHeartbeat);
		}
		TADD_FUNC("END.");
		return iRet;
	}
//}
