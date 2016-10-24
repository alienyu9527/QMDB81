/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbRepLoadCtrl.h		
*@Description: 分片备份的数据上载模块
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
//#include "BillingSDK.h"
#include "Replication/mdbRepLoadData.h"
#include "Replication/mdbServerConfig.h"
#include "Control/mdbRepCommon.h"
#include "Common/mdbSysThreads.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB
//{
    TMdbRepLoadThread::TMdbRepLoadThread(TMdbRepDataClient *pClient): m_pClient(pClient)
    {
        
    }

    TMdbRepLoadThread::~TMdbRepLoadThread()
    {

    }

    int TMdbRepLoadThread::AddRoutingID(int iRoutingID)
    {
        m_aiRoutingID.push_back(iRoutingID);
        return 0;
    }

    int TMdbRepLoadThread::GetHostID()
    {
        if(NULL == m_pClient)
        {
            return MDB_REP_EMPTY_HOST_ID;
        }
        else
        {
            return m_pClient->GetHostID();
        }
    }

    int TMdbRepLoadThread::SetRuningInfo(TMdbConfig* pMdbCfg)
    {
        CHECK_OBJ(pMdbCfg);
        m_pMdbCfg = pMdbCfg;
        return 0;
    }


    int TMdbRepLoadThread::Execute()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbShmRepMgr *pRepShm = new(std::nothrow) TMdbShmRepMgr(m_pMdbCfg->GetDSN()->sName);
		if(pRepShm ==  NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new pRepShm");
			return ERR_OS_NO_MEMROY;
		}
        CHECK_RET(pRepShm->Attach(), "Attach TMdbShmRep failed.");

        //拼接routing_list串
        char sRoutingList[MAX_REP_ROUTING_LIST_LEN];
        sRoutingList[0] = '\0';
        for (unsigned int i = 0; i<m_aiRoutingID.size(); i++)
        {
            if (i == 0)
            {
                snprintf(sRoutingList, MAX_REP_ROUTING_LIST_LEN, "%d", m_aiRoutingID[i]);
            }
            else
            {
                snprintf(sRoutingList+strlen(sRoutingList), MAX_REP_ROUTING_LIST_LEN-strlen(sRoutingList), ",%d", m_aiRoutingID[i]);
            }
        }
        
        if (m_pClient!=NULL)//从备机加载
        {
            CHECK_RET(m_pClient->Init(m_pMdbCfg), "TMdbRepDataClient init failed");
            //CHECK_RET(LoadDataFromRepHost(sRoutingList), "LoadDataFromRepHost failed.");
            iRet = LoadDataFromRepHost(sRoutingList);
            if (iRet != ERROR_SUCCESS)
            {
                TADD_ERROR(iRet, "Routing ID [%s] load from standby host failed.", sRoutingList);
                //增加路由加载失败标识
                CHECK_RET(pRepShm->AddFailedRoutingList(sRoutingList), "Add failed routing list error.");               
            }
        }
        else////增加路由加载失败标识
        {
            //CHECK_RET(LoadDataFromStorage(strRoutingList), "LoadDataFromStorage failed.");
            TADD_ERROR(iRet, "Routing ID [%s] load from standby host failed.", sRoutingList);
            CHECK_RET(pRepShm->AddFailedRoutingList(sRoutingList), "Add failed routing list error."); 

        }       

        SAFE_DELETE(pRepShm);
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbRepLoadThread::LoadDataFromRepHost(const char*sRoutinglist)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //通知备机删除相应的同步文件
        TADD_NORMAL("Test: Send clean rep file cmd.");
        CHECK_RET(m_pClient->SendCleanCmd(), "Send clean cmd failed.");

        //从备机加载数据
        TADD_NORMAL("Test: Start to load data(DSN = [%s], routing_id = [%s]) from host(%s:%d)...", m_pMdbCfg->GetDSN()->sName, sRoutinglist, 
            m_pClient->GetIP(), m_pClient->GetPort());
        iRet = m_pClient->LoadData(sRoutinglist);
        if (iRet != ERROR_SUCCESS)
        {
            m_pClient->Disconnect();//断开与服务器的连接
            TADD_WARNING("Load data from rep host failed, routing_list = %s.", sRoutinglist);
            //CHECK_RET(LoadDataFromStorage(sRoutinglist), "Load data from storage failed, routing_list = %s", sRoutinglist);
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbRepLoadThread::LoadDataFromStorage(const char*sRoutinglist)
    {
        TADD_NORMAL("Start.");
        int iRet = 0;

        TADD_NORMAL("Finish.");
        return iRet;
    }

    
    TMdbRepLoadDataCtrl::TMdbRepLoadDataCtrl():
        m_pRepConfig(NULL), 
        m_pShmMgr(NULL),
        m_pMonitorClient(NULL), 
        m_pShmRep(NULL),
        m_pMdbCfg(NULL),
        m_bSvrConnFlag(false)
    {

    }

    TMdbRepLoadDataCtrl::~TMdbRepLoadDataCtrl()
    {
        SAFE_DELETE(m_pRepConfig);
        SAFE_DELETE(m_pShmMgr);
        SAFE_DELETE(m_pMonitorClient);
        for (unsigned int i = 0; i<m_arThread.GetSize(); i++)
        {
            SAFE_DELETE(m_arThread[i]);
        }

        m_pShmRep = NULL;
        m_pMdbCfg = NULL;
    }

       /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	:  初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TMdbRepLoadDataCtrl::Init(const char* sDsn, TMdbConfig* pMdbCfg)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(sDsn);
        CHECK_OBJ(pMdbCfg);

        m_strDsn = sDsn;
        m_pRepConfig = new(std::nothrow)TMdbRepConfig();
        CHECK_OBJ(m_pRepConfig);
        CHECK_RET(m_pRepConfig->Init(sDsn),"TMdbRepConifg init failed");

        m_bSvrConnFlag = false;
        m_pMonitorClient = new(std::nothrow) TMdbRepMonitorClient();
        CHECK_OBJ(m_pMonitorClient);
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
            //CHECK_RET(-1, "Connect to Config Server failed.");
        }

        m_pShmMgr = new(std::nothrow)TMdbShmRepMgr(m_strDsn.c_str());
        CHECK_OBJ(m_pShmMgr);
        //CHECK_RET(m_pShmMgr->Create(), "TMdbShmRepMgr Create failed.");
        CHECK_RET(m_pShmMgr->Attach(), "TMdbShmRepMgr Attach failed.");
        m_pShmRep = m_pShmMgr->GetRoutingRep();
        m_pMdbCfg = pMdbCfg;

        m_iHostID = MDB_REP_EMPTY_HOST_ID;
        TADD_FUNC("Finish.");
        return iRet;
    }

	   int TMdbRepLoadDataCtrl::LoadDataForReLoadTool(char* sTblName)
		 {
			 TADD_FUNC("Start.");
			 int iRet = 0;
			
			 //(1)向配置服务注册和上报状态 create
			// int iHostID = 0;
			 //int iHeartbeat = 0;
	   
			// CHECK_RET(m_pMonitorClient->Register(m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort, iHostID,iHeartbeat), "Register Failed.");
			// TADD_NORMAL("Local_IP = %s, Local_Port = %d,Host_ID = %d", m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort ,iHostID);
			// m_pShmMgr->SetHostID(iHostID);
			// m_pShmMgr->SetHeartbeat(iHeartbeat);
			// m_pShmMgr->SetMdbState(E_MDB_STATE_CREATING);
			// TADD_NORMAL("Report state [%d]", E_MDB_STATE_CREATING);
			 //CHECK_RET(m_pMonitorClient->ReportState(iHostID, E_MDB_STATE_CREATING), "ReportState failed.");	  
	   
			 //(2)向配置服务请求路由信息，写入共享内存
			// CHECK_RET(m_pMonitorClient->RoutingRequest(iHostID, m_pShmMgr), "RoutingRequest failed.");
	   
			 //(3)创建与所有备机(包括容灾机）连接
			 CHECK_RET(ConnectRepHosts(), "CreateLoadThreads failed.")
	   
			
	   
			 //(5)从所有备机（或容灾机）加载数据，加载失败的从容灾机加载
			 CHECK_RET(LoadDataFromRep(true,sTblName), "LoadDataFromRep failed.");
	   
			 //(6) 上报状态，加载成功
			 //m_pShmMgr->SetMdbState(E_MDB_STATE_CREATED);
			 //TADD_NORMAL("Report state [%d]", E_MDB_STATE_CREATED);
			 //CHECK_RET(m_pMonitorClient->ReportState(iHostID, E_MDB_STATE_CREATED), "ReportState failed.");	 
			 ///*QuickMDB::*/TMdbNtcDateTime::Sleep(100);//如果不sleep， 对方可能收不到数据
	   
			 //(7)断开与备机的连接
			 CHECK_RET(DisConnectRepHosts(), "DisConnectRepHosts failed.")
	   
			
			 TADD_FUNC("Finish.");
			 return iRet;
		 }
	   


       /******************************************************************************
        * 函数名称	:  LoadData
        * 函数描述	:  加载数据
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/

	   
    int TMdbRepLoadDataCtrl::LoadData()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        /*if(!m_bSvrConnFlag)
        {
            TADD_NORMAL("config server not connected .load routing data based by config");
            return LoadDataNoSvr();
        }*/
           
        /*//(1)向配置服务注册和上报状态 create
        int iHostID = 0;
        int iHeartbeat = 0;
 
        CHECK_RET(m_pMonitorClient->Register(m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort, iHostID,iHeartbeat), "Register Failed.");
        TADD_NORMAL("Local_IP = %s, Local_Port = %d,Host_ID = %d", m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort ,iHostID);
        m_pShmMgr->SetHostID(iHostID);
        m_pShmMgr->SetHeartbeat(iHeartbeat);
        m_pShmMgr->SetMdbState(E_MDB_STATE_CREATING);
        TADD_NORMAL("Report state [%d]", E_MDB_STATE_CREATING);
        CHECK_RET(m_pMonitorClient->ReportState(iHostID, E_MDB_STATE_CREATING), "ReportState failed.");      

        //(2)向配置服务请求路由信息，写入共享内存
        CHECK_RET(m_pMonitorClient->RoutingRequest(iHostID, m_pShmMgr), "RoutingRequest failed.");*/

        //(3)创建与所有备机(包括容灾机）连接
        CHECK_RET(ConnectRepHosts(), "CreateLoadThreads failed.");

        //(4)处理遗留数据
        CHECK_RET(DealLeftFile(), "DealLeftFile failed.");

        //(5)从所有备机（或容灾机）加载数据，加载失败的从容灾机加载
        CHECK_RET(LoadDataFromRep(), "LoadDataFromRep failed.");

        //(6) 上报状态，加载成功
        //m_pShmMgr->SetMdbState(E_MDB_STATE_CREATED);
        //TADD_NORMAL("Report state [%d]", E_MDB_STATE_CREATED);
        //CHECK_RET(m_pMonitorClient->ReportState(iHostID, E_MDB_STATE_CREATED), "ReportState failed.");    
        //TMdbNtcDateTime::Sleep(100);//如果不sleep， 对方可能收不到数据

        //(7)断开与备机的连接
        CHECK_RET(DisConnectRepHosts(), "DisConnectRepHosts failed.")

        //(8) 同步分片备份配置到本地配置文件
        //CHECK_RET(SyncLocalConfig(), "Save config info to local config file failed.");     
        TADD_FUNC("Finish.");
        return iRet;
    }


	   
    int TMdbRepLoadDataCtrl::LoadDataNoSvr()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        /*//(1) 从本地配置文件获取路由信息，保存到共享内存
        CHECK_RET(WriteLocalRoutingInfo(), "WriteLocalRoutingInfo failed.");

        //(2)保存当前状态
        TADD_NORMAL("Local_IP = %s, Local_Port = %d,Host_ID=%d", m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort, m_iHostID);
        m_pShmMgr->SetMdbState(E_MDB_STATE_CREATING);*/

         //(3)创建与所有备机(包括容灾机）连接
        CHECK_RET(ConnectRepHosts(), "CreateLoadThreads failed.");

        //(4)处理遗留数据
        CHECK_RET(DealLeftFile(), "DealLeftFile failed.");

        //(5)从所有备机（或容灾机）加载数据，加载失败的从容灾机加载
        CHECK_RET(LoadDataFromRep(), "LoadDataFromRep failed.");

        //(6) 加载成功，保存状态
        //m_pShmMgr->SetMdbState(E_MDB_STATE_CREATED);

        TADD_FUNC("Finish.");
        return iRet;
    }
       /******************************************************************************
        * 函数名称	:  ConnectRepHosts
        * 函数描述	:  创建与所有备机的连接
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TMdbRepLoadDataCtrl::ConnectRepHosts()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        const TMdbShmRepHost *pRepHost = NULL;
        m_arpLoadClient = new (std::nothrow) TMdbRepDataClient* [m_pShmRep->m_iRepHostCount];
		if(m_arpLoadClient == NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new m_arpLoadClient");
			return ERR_OS_NO_MEMROY;
		}
        TMdbRepDataClient *pClient = NULL;

        //连接所有备机
        for (int i = 0; i < m_pShmRep->m_iRepHostCount; i++)
        {
            m_arpLoadClient[i] = NULL;
            pClient = new(std::nothrow) TMdbRepDataClient();
            CHECK_OBJ(pClient);
            pRepHost = &m_pShmRep->m_arHosts[i];
            if (pClient->Connect(pRepHost->m_sIP, pRepHost->m_iPort))
            {
                pClient->SetHostID(m_pShmRep->m_iHostID, pRepHost->m_iHostID);
                m_arpLoadClient[i] = pClient;
            }
            else
            {
                TADD_ERROR(ERR_NET_PEER_INVALID, "Connect to Stand by host[%s:%d] error.", pRepHost->m_sIP, pRepHost->m_iPort);
                SAFE_DELETE(pClient);
            }
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbRepLoadDataCtrl::DisConnectRepHosts()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        for (int i = 0; i<m_pShmRep->m_iRepHostCount; i++)
        {
            SAFE_DELETE(m_arpLoadClient[i]);
        }
        SAFE_DELETE_ARRAY(m_arpLoadClient);      

        TADD_FUNC("Finish.");
        return iRet;
    }

       /******************************************************************************
        * 函数名称	:  DealLeftFile
        * 函数描述	:  处理遗留文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TMdbRepLoadDataCtrl::DealLeftFile()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        if(!TMdbNtcDirOper::IsExist(m_pRepConfig->m_sRepPath.c_str()))
        {
            TADD_NORMAL("Path[%s] not exist,create it!",m_pRepConfig->m_sRepPath.c_str());
            if(!TMdbNtcDirOper::MakeDir(m_pRepConfig->m_sRepPath.c_str()))
            {
                CHECK_RET(ERR_OS_CREATE_DIR,"create path[%s] failed.", m_pRepConfig->m_sRepPath.c_str());
            }
        }
        
        //遍历目录下的文件
        TMdbNtcFileScanner tFileScanner;
        tFileScanner.AddFilter(MDB_REP_FILE_PATTEN);
        if (!tFileScanner.ScanFile(m_pRepConfig->m_sRepPath.c_str()))
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "ScanFile in path[%s] failed.", m_pRepConfig->m_sRepPath.c_str());
        }
        const char* pFileName = NULL;
        TMdbNtcSplit tSplit;
        int iHostID;//主机ID
        TMdbRepDataClient *pClient;
       // TMdbNtcString sFileFullName;
        
        while((pFileName=tFileScanner.GetNext())!=NULL)
        {
            tSplit.SplitString(pFileName, '.');
            //文件是否过期
            if (IsFileOutOfDate((time_t)atol(tSplit[2])))
            {
     /*           sFileFullName.Clear();
                sFileFullName.Append(m_pRepConfig->m_sRepPath.c_str());
                if (sFileFullName[sFileFullName.GetLength()-1]!='/')
                {
                    sFileFullName.Append("/");
                }
                sFileFullName.Append(pFileName);*/

                TMdbNtcFileOper::Remove(pFileName);
            }
            else//文件未过期，发送至对应备机（或者容灾机）
            {
                iHostID = atoi(tSplit[1]);
                pClient = GetClient(iHostID);
                if (pClient !=NULL)
                {
                    CHECK_RET(pClient->SendData(pFileName),"Send file[%s] failed.", pFileName);
                }
                else
                {
                    // TODO:TADD_WARNING("Host [%d] can not connect, and file [%s] is left.", iHostID, pFileName);
                }
            }           
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

        /******************************************************************************
        * 函数名称	:  LoadDataFromRep
        * 函数描述	:  从备机或者其他存储加载数据
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TMdbRepLoadDataCtrl::LoadDataFromRep(bool bTool,char * sTblName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        //（1）为每个路由创建或者分配加载的线程
        CHECK_RET(CreateLoadThreads(bTool,sTblName), "CreateLoadThreads failed.");

        //（2）启动加载线程
        TMdbRepLoadThread *pThread = NULL;
        for (unsigned int i = 0; i<m_arThread.GetSize(); i++)
        {
            pThread =(TMdbRepLoadThread*)m_arThread[i];
            CHECK_OBJ(pThread);
            pThread->Run();
        }

        //（3）等待所有线程加载完成
        bool bAllExit = false;
        while(!bAllExit)
        {
            bAllExit = true;
            for (unsigned int i = 0; i<m_arThread.GetSize(); i++)
            {
                pThread = (TMdbRepLoadThread*)m_arThread[i];
                CHECK_OBJ(pThread);
                if (pThread->GetThreadState() != MDB_NTC_THREAD_EXIT)
                {
                    bAllExit = false;
                    break;
                }
            }
            TMdbNtcDateTime::Sleep(100);
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

         /******************************************************************************
        * 函数名称	:  LoadDataFromRep
        * 函数描述	:  为每个路由创建或者分配加载的线程
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TMdbRepLoadDataCtrl::CreateLoadThreads(bool bTool,char* sTblName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iHostID = MDB_REP_EMPTY_HOST_ID;
        TMdbRepLoadThread *pThread = NULL;
        const TMdbShmRepRouting *pRouting = NULL;
        TMdbRepDataClient *pClient = NULL;
        for (int i = 0; i<m_pShmRep->m_iRoutingIDCount; i++)
        {
            pRouting = &m_pShmRep->m_arRouting[i];
            pThread = NULL;
            pClient = NULL;
            for (int j = 0; j<MAX_REP_HOST_COUNT; j++)//遍历该路由对应的所有备机
            {
                if (MDB_REP_EMPTY_HOST_ID == pRouting->m_aiHostID[j])
                {
                    break;
                }
                pClient = GetClient(pRouting->m_aiHostID[j]);
                if (pClient!=NULL)
                {
                    iHostID = pRouting->m_aiHostID[j];
                    break;
                }
            }
			if (NULL == pClient)
			{
	            for (int j = 0; j<MAX_REP_HOST_COUNT; j++)//遍历该路由对应的所有备机
	            {
	                if (MDB_REP_EMPTY_HOST_ID == pRouting->m_iRecoveryHostID[j])
	                {
	                    break;
	                }
	                pClient = GetClient(pRouting->m_iRecoveryHostID[j]);
	                if (pClient!=NULL)
	                {
	                    iHostID = pRouting->m_iRecoveryHostID[j];
	                    break;
	                }
	            }
			}

            if(NULL == pClient)//路由找不到对应的加载主机
            {
            	for(int j = 0; j<MAX_REP_ROUTING_ID_COUTN; j++)
        		{
					if(pRouting->m_iRouteValue[j] != EMPTY_ROUTE_ID)
					{
                		TADD_NORMAL("Routing value [%d] load from standby host failed.", pRouting->m_iRouteValue[j]);
                		m_pShmMgr->AddFailedRoutingID(pRouting->m_iRouteValue[j]);
					}
					else
					{
						break;
					}
        		}
                continue;
            }

			//设置同步的表名
			if(bTool && sTblName != NULL)
			{
				SAFESTRCPY(pClient->sTblName,sizeof(pClient->sTblName), sTblName);
				pClient->bTool = true;
             }
			else
			{
				pClient->sTblName[0] = 0;
				pClient->bTool = false;
			}
					

            pThread = GetThread(iHostID);//获取主机ID对应的线程，如果主机ID为MDB_REP_EMPTY_HOST_ID,则对应的线程从存储加载
            if (pThread != NULL)//线程已经存在
            {
            	for(int j = 0; j<MAX_REP_ROUTING_ID_COUTN; j++)
        		{
					if(pRouting->m_iRouteValue[j] != EMPTY_ROUTE_ID)
					{
						pThread->AddRoutingID(pRouting->m_iRouteValue[j]);//把该路由的加载放入该线程
					}
					else
					{
						break;
					}
        		}
            }
            else //线程不存在
            {
                pThread = new(std::nothrow)TMdbRepLoadThread(pClient);
                CHECK_OBJ(pThread);
                pThread->SetRuningInfo(m_pMdbCfg);
				for(int j = 0; j<MAX_REP_ROUTING_ID_COUTN; j++)
        		{
					if(pRouting->m_iRouteValue[j] != EMPTY_ROUTE_ID)
					{
						pThread->AddRoutingID(pRouting->m_iRouteValue[j]);//把该路由的加载放入该线程
					}
					else
					{
						break;
					}
        		}
                m_arThread.Add(pThread);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

        /******************************************************************************
        * 函数名称	:  GetClient
        * 函数描述	:  根据hostID获取对应的连接
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    TMdbRepDataClient* TMdbRepLoadDataCtrl::GetClient(int iHostID)
    {
        for (int i = 0; i<m_pShmRep->m_iRepHostCount; i++)
        {
            if (m_arpLoadClient[i] !=NULL && iHostID == m_arpLoadClient[i]->GetHostID())
            {
                return m_arpLoadClient[i];
            }
        }
        return NULL;
    }

    TMdbRepLoadThread* TMdbRepLoadDataCtrl::GetThread(int iHostID)
    {
        TMdbRepLoadThread* pThread = NULL;
        for (unsigned int i = 0; i< m_arThread.GetSize(); i++)
        {
            pThread = (TMdbRepLoadThread*)m_arThread[i];
            if (iHostID == pThread->GetHostID())
            {
                return pThread;
            }
        }
        return NULL;
    }

        /******************************************************************************
        * 函数名称	:  IsFileOutOfDate
        * 函数描述	:  文件是否过期
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    bool TMdbRepLoadDataCtrl::IsFileOutOfDate(time_t tTime)
    {
        return TMdbNtcDateTime::GetDiffSeconds(tTime, TMdbNtcDateTime::GetCurTime())>m_pRepConfig->m_iFileInvalidTime;
    }

    int TMdbRepLoadDataCtrl::WriteLocalRoutingInfo()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        char sConfigStr[MAX_REP_SEND_BUF_LEN] = {0};

        TShbRepLocalConfig tLocalConfig;
        CHECK_RET(tLocalConfig.ReadLocalConfig(m_pMdbCfg->GetDSN()->sName,sConfigStr, sizeof(sConfigStr))
            ,"Get Local config content failed");
        
        CHECK_RET(m_pShmMgr->WriteRoutingInfo(sConfigStr, sizeof(sConfigStr)),"WriteLocalRoutingInfo failed");
        m_pShmMgr->SetHostID(tLocalConfig.m_iHostID);
        m_pShmMgr->SetHeartbeat(tLocalConfig.m_iHeartbeat);
        m_iHostID = tLocalConfig.m_iHostID;
        return iRet;
    }
    
    int TMdbRepLoadDataCtrl::SyncLocalConfig()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        TShbRepLocalConfig tLocalConfig;
        CHECK_RET(tLocalConfig.SyncLocalConfig(m_pMdbCfg->GetDSN()->sName,m_pMonitorClient->GetSvrConfigMsg()),"sync Local config failed");
        
        return iRet;
    }
//}
