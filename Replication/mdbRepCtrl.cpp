/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbRepCtrl.cpp	
*@Description: 分片备份主进程管理类，负责与配置服务通信和数据上载
*@Author:		jiang.lili
*@Date：	    2014/05/4
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
    * 函数名称	:  Init()
    * 函数描述	:  初始化
    * 输入		:  sDsn mdb的Dsn名称
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepCtrl::Init(const char* sDsn)
    {
        int iRet = 0;
        TADD_FUNC("Start.");

        CHECK_OBJ(sDsn);
        SAFESTRCPY(m_sDsn, sizeof(m_sDsn), sDsn);
        TMdbNtcStrFunc::ToUpper(m_sDsn);

        //读取配置文件
        m_pRepConfig = new (std::nothrow)TMdbRepConfig();
		CHECK_OBJ(m_pRepConfig);
        CHECK_RET(m_pRepConfig->Init(sDsn), "RepConfig init failed.");
 
        //连接共享内存
        m_pShmMgr = new (std::nothrow)TMdbShmRepMgr(sDsn);
        CHECK_OBJ(m_pShmMgr);
        //CHECK_RET(m_pShmMgr->Create(),"Create shm failed.");
        CHECK_RET(m_pShmMgr->Attach(),"Attach shm failed.");
        
        //连接MDB
        m_pMDB = new(std::nothrow)TMdbDatabase();
        CHECK_OBJ(m_pMDB);
        CHECK_RET(ConnectMDB(), "Connect to MDB [%s] failed.", m_sDsn);

        //连接配置服务
        m_pMonitorClient = new(std::nothrow) TMdbRepMonitorClient();
        CHECK_OBJ(m_pMonitorClient);
        CHECK_RET(ConnectServer(), "Connect to configuration server failed.");

        m_tProcCtrl.Init(sDsn);

        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  LoadData()
    * 函数描述	:  加载分片备份的数据至QuickMDB，  加载数据, 供测试程序使用，通过设定不同的port，同一台主机可以启动多个qmdb
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  jiang.lili
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
    * 函数名称	:  LoadData()
    * 函数描述	:  加载分片备份的数据至QuickMDB
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  jiang.lili
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
    * 函数名称	:  Start()
    * 函数描述	:  启动分片备份各个子进程，进行数据同步、状态上报、路由变更处理
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepCtrl::Start()
    {
        int iRet = 0;
        TADD_FUNC("Start.");

        //上报状态
        CHECK_RET(m_pShmMgr->SetRunningFlag(true), "Update running state failed.");//设置mdbRep自身状态
        m_pShmMgr->SetMdbState(E_MDB_STATE_RUNNING);
        if(m_bSvrConn)
        {
            TADD_NORMAL("Report state [%d]", E_MDB_STATE_RUNNING);
            CHECK_RET(m_pMonitorClient->ReportState(m_pShmMgr->GetRoutingRep()->m_iHostID, E_MDB_STATE_RUNNING), "Report state failed.");
        }
        
        //监控同步进程的运行，处理路由变更命令
        TRepNTCMsg tMsg;//保存mdbServer接收到的信息
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
                if(m_bSvrConn) // 重连成功后，上报一次当前状态
                {
                    //  重新上报一次当前状态
                    TADD_NORMAL("Report state [%d]", m_pShmMgr->GetRoutingRep()->m_eState);
                    CHECK_RET(m_pMonitorClient->ReportState(m_pShmMgr->GetRoutingRep()->m_iHostID, m_pShmMgr->GetRoutingRep()->m_eState), "Report state failed.");

                    // 同步一次分片备份配置信息(暂不考虑断开期间路由变更问题）
                    CHECK_RET(m_pMonitorClient->RoutingRequestNoShm(m_pShmMgr->GetRoutingRep()->m_iHostID), "Report state failed.");
                    TShbRepLocalConfig tLocalConfig;
                    CHECK_RET(tLocalConfig.SyncLocalConfig(m_sDsn,m_pMonitorClient->GetSvrConfigMsg()),"sync Local config failed");
                }
                else
                {
                    TMdbNtcDateTime::Sleep(100);
                }
            }
         
            //执行mdbServer发送过来的命令
            iRet = m_pMonitorClient->GetMsg(tMsg);
            if (iRet == 0)
            {
                DealServerCmd(tMsg);
            }
        }

        //上报状态，同步进程被停止
        m_pShmMgr->SetMdbState(E_MDB_STATE_CREATED);
        if(m_bSvrConn)
        {
            TADD_NORMAL("Report state [%d]", E_MDB_STATE_CREATED);
            CHECK_RET(m_pMonitorClient->ReportState(m_pShmMgr->GetRoutingRep()->m_iHostID, E_MDB_STATE_CREATED), "Report state failed");    
            TMdbNtcDateTime::Sleep(100);//不增加sleep，mdbServer收不到状态信息
            m_pMonitorClient->Disconnect();
        }
        
        CHECK_RET(m_pShmMgr->SetRunningFlag(false), "Update running state failed.");//设置mdbRep自身状态
        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  ConnectMDB()
    * 函数描述	:  启动分片备份各个子进程，进行数据同步、状态上报、路由变更处理
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  jiang.lili
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
    * 函数名称	:  ConnectServer()
    * 函数描述	:  连接配置服务
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  jiang.lili
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
            //支持不起配置服务端
            TADD_NORMAL("Start not connecting to any mdbServer.");
            m_bSvrConn = false;
        }

        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  DealServerCmd()
    * 函数描述	:  处理来自配置服务的命令，如路由变更等
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  jiang.lili
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
                //通过m_pShmMgr，修改共享内存中路由对应关系
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
        //连接
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

            TMdbNtcDateTime::Sleep(100);//不增加sleep，mdbServer收不到状态信息
        }

        SAFE_DELETE(pConfig);
        SAFE_DELETE(pMonitorClient);
        TADD_FUNC("Finish.");
        return iRet;
    }

    #if 0

    /******************************************************************************
    * 函数名称	:  StartRepProcess()
    * 函数描述	:  启动同步进程
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  jiang.lili
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
            //进程存在
            TADD_WARNING("Process=[%s] is running, can't restart.", sProcName);
            return 0;
        }
        else
        {
            //进程不存在
            system(sProcName);//启动进程
            TADD_DETAIL("system(%s) OK.",sProcName);
            TMdbNtcDateTime::Sleep(1000);
            //等待进程启动
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
    * 函数名称	:  MonitorProcess()
    * 函数描述	:  监控同步进程的运行
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  jiang.lili
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
		//更新分片备份配置信息

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

	        //(2)向配置服务请求路由信息，写入共享内存
	        CHECK_RET(m_pMonitorClient->RoutingRequest(iHostID, m_pShmMgr), "RoutingRequest failed.");
	        //(8) 同步分片备份配置到本地配置文件
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
