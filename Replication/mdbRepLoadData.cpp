/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepLoadCtrl.h		
*@Description: ��Ƭ���ݵ���������ģ��
*@Author:		jiang.lili
*@Date��	    2014/05/4
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

        //ƴ��routing_list��
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
        
        if (m_pClient!=NULL)//�ӱ�������
        {
            CHECK_RET(m_pClient->Init(m_pMdbCfg), "TMdbRepDataClient init failed");
            //CHECK_RET(LoadDataFromRepHost(sRoutingList), "LoadDataFromRepHost failed.");
            iRet = LoadDataFromRepHost(sRoutingList);
            if (iRet != ERROR_SUCCESS)
            {
                TADD_ERROR(iRet, "Routing ID [%s] load from standby host failed.", sRoutingList);
                //����·�ɼ���ʧ�ܱ�ʶ
                CHECK_RET(pRepShm->AddFailedRoutingList(sRoutingList), "Add failed routing list error.");               
            }
        }
        else////����·�ɼ���ʧ�ܱ�ʶ
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
        //֪ͨ����ɾ����Ӧ��ͬ���ļ�
        TADD_NORMAL("Test: Send clean rep file cmd.");
        CHECK_RET(m_pClient->SendCleanCmd(), "Send clean cmd failed.");

        //�ӱ�����������
        TADD_NORMAL("Test: Start to load data(DSN = [%s], routing_id = [%s]) from host(%s:%d)...", m_pMdbCfg->GetDSN()->sName, sRoutinglist, 
            m_pClient->GetIP(), m_pClient->GetPort());
        iRet = m_pClient->LoadData(sRoutinglist);
        if (iRet != ERROR_SUCCESS)
        {
            m_pClient->Disconnect();//�Ͽ��������������
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
        * ��������	:  Init
        * ��������	:  ��ʼ��
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
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
			
			 //(1)�����÷���ע����ϱ�״̬ create
			// int iHostID = 0;
			 //int iHeartbeat = 0;
	   
			// CHECK_RET(m_pMonitorClient->Register(m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort, iHostID,iHeartbeat), "Register Failed.");
			// TADD_NORMAL("Local_IP = %s, Local_Port = %d,Host_ID = %d", m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort ,iHostID);
			// m_pShmMgr->SetHostID(iHostID);
			// m_pShmMgr->SetHeartbeat(iHeartbeat);
			// m_pShmMgr->SetMdbState(E_MDB_STATE_CREATING);
			// TADD_NORMAL("Report state [%d]", E_MDB_STATE_CREATING);
			 //CHECK_RET(m_pMonitorClient->ReportState(iHostID, E_MDB_STATE_CREATING), "ReportState failed.");	  
	   
			 //(2)�����÷�������·����Ϣ��д�빲���ڴ�
			// CHECK_RET(m_pMonitorClient->RoutingRequest(iHostID, m_pShmMgr), "RoutingRequest failed.");
	   
			 //(3)���������б���(�������ֻ�������
			 CHECK_RET(ConnectRepHosts(), "CreateLoadThreads failed.")
	   
			
	   
			 //(5)�����б����������ֻ����������ݣ�����ʧ�ܵĴ����ֻ�����
			 CHECK_RET(LoadDataFromRep(true,sTblName), "LoadDataFromRep failed.");
	   
			 //(6) �ϱ�״̬�����سɹ�
			 //m_pShmMgr->SetMdbState(E_MDB_STATE_CREATED);
			 //TADD_NORMAL("Report state [%d]", E_MDB_STATE_CREATED);
			 //CHECK_RET(m_pMonitorClient->ReportState(iHostID, E_MDB_STATE_CREATED), "ReportState failed.");	 
			 ///*QuickMDB::*/TMdbNtcDateTime::Sleep(100);//�����sleep�� �Է������ղ�������
	   
			 //(7)�Ͽ��뱸��������
			 CHECK_RET(DisConnectRepHosts(), "DisConnectRepHosts failed.")
	   
			
			 TADD_FUNC("Finish.");
			 return iRet;
		 }
	   


       /******************************************************************************
        * ��������	:  LoadData
        * ��������	:  ��������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
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
           
        /*//(1)�����÷���ע����ϱ�״̬ create
        int iHostID = 0;
        int iHeartbeat = 0;
 
        CHECK_RET(m_pMonitorClient->Register(m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort, iHostID,iHeartbeat), "Register Failed.");
        TADD_NORMAL("Local_IP = %s, Local_Port = %d,Host_ID = %d", m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort ,iHostID);
        m_pShmMgr->SetHostID(iHostID);
        m_pShmMgr->SetHeartbeat(iHeartbeat);
        m_pShmMgr->SetMdbState(E_MDB_STATE_CREATING);
        TADD_NORMAL("Report state [%d]", E_MDB_STATE_CREATING);
        CHECK_RET(m_pMonitorClient->ReportState(iHostID, E_MDB_STATE_CREATING), "ReportState failed.");      

        //(2)�����÷�������·����Ϣ��д�빲���ڴ�
        CHECK_RET(m_pMonitorClient->RoutingRequest(iHostID, m_pShmMgr), "RoutingRequest failed.");*/

        //(3)���������б���(�������ֻ�������
        CHECK_RET(ConnectRepHosts(), "CreateLoadThreads failed.");

        //(4)������������
        CHECK_RET(DealLeftFile(), "DealLeftFile failed.");

        //(5)�����б����������ֻ����������ݣ�����ʧ�ܵĴ����ֻ�����
        CHECK_RET(LoadDataFromRep(), "LoadDataFromRep failed.");

        //(6) �ϱ�״̬�����سɹ�
        //m_pShmMgr->SetMdbState(E_MDB_STATE_CREATED);
        //TADD_NORMAL("Report state [%d]", E_MDB_STATE_CREATED);
        //CHECK_RET(m_pMonitorClient->ReportState(iHostID, E_MDB_STATE_CREATED), "ReportState failed.");    
        //TMdbNtcDateTime::Sleep(100);//�����sleep�� �Է������ղ�������

        //(7)�Ͽ��뱸��������
        CHECK_RET(DisConnectRepHosts(), "DisConnectRepHosts failed.")

        //(8) ͬ����Ƭ�������õ����������ļ�
        //CHECK_RET(SyncLocalConfig(), "Save config info to local config file failed.");     
        TADD_FUNC("Finish.");
        return iRet;
    }


	   
    int TMdbRepLoadDataCtrl::LoadDataNoSvr()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        /*//(1) �ӱ��������ļ���ȡ·����Ϣ�����浽�����ڴ�
        CHECK_RET(WriteLocalRoutingInfo(), "WriteLocalRoutingInfo failed.");

        //(2)���浱ǰ״̬
        TADD_NORMAL("Local_IP = %s, Local_Port = %d,Host_ID=%d", m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort, m_iHostID);
        m_pShmMgr->SetMdbState(E_MDB_STATE_CREATING);*/

         //(3)���������б���(�������ֻ�������
        CHECK_RET(ConnectRepHosts(), "CreateLoadThreads failed.");

        //(4)������������
        CHECK_RET(DealLeftFile(), "DealLeftFile failed.");

        //(5)�����б����������ֻ����������ݣ�����ʧ�ܵĴ����ֻ�����
        CHECK_RET(LoadDataFromRep(), "LoadDataFromRep failed.");

        //(6) ���سɹ�������״̬
        //m_pShmMgr->SetMdbState(E_MDB_STATE_CREATED);

        TADD_FUNC("Finish.");
        return iRet;
    }
       /******************************************************************************
        * ��������	:  ConnectRepHosts
        * ��������	:  ���������б���������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
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

        //�������б���
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
        * ��������	:  DealLeftFile
        * ��������	:  ���������ļ�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
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
        
        //����Ŀ¼�µ��ļ�
        TMdbNtcFileScanner tFileScanner;
        tFileScanner.AddFilter(MDB_REP_FILE_PATTEN);
        if (!tFileScanner.ScanFile(m_pRepConfig->m_sRepPath.c_str()))
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "ScanFile in path[%s] failed.", m_pRepConfig->m_sRepPath.c_str());
        }
        const char* pFileName = NULL;
        TMdbNtcSplit tSplit;
        int iHostID;//����ID
        TMdbRepDataClient *pClient;
       // TMdbNtcString sFileFullName;
        
        while((pFileName=tFileScanner.GetNext())!=NULL)
        {
            tSplit.SplitString(pFileName, '.');
            //�ļ��Ƿ����
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
            else//�ļ�δ���ڣ���������Ӧ�������������ֻ���
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
        * ��������	:  LoadDataFromRep
        * ��������	:  �ӱ������������洢��������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TMdbRepLoadDataCtrl::LoadDataFromRep(bool bTool,char * sTblName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        //��1��Ϊÿ��·�ɴ������߷�����ص��߳�
        CHECK_RET(CreateLoadThreads(bTool,sTblName), "CreateLoadThreads failed.");

        //��2�����������߳�
        TMdbRepLoadThread *pThread = NULL;
        for (unsigned int i = 0; i<m_arThread.GetSize(); i++)
        {
            pThread =(TMdbRepLoadThread*)m_arThread[i];
            CHECK_OBJ(pThread);
            pThread->Run();
        }

        //��3���ȴ������̼߳������
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
        * ��������	:  LoadDataFromRep
        * ��������	:  Ϊÿ��·�ɴ������߷�����ص��߳�
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
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
            for (int j = 0; j<MAX_REP_HOST_COUNT; j++)//������·�ɶ�Ӧ�����б���
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
	            for (int j = 0; j<MAX_REP_HOST_COUNT; j++)//������·�ɶ�Ӧ�����б���
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

            if(NULL == pClient)//·���Ҳ�����Ӧ�ļ�������
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

			//����ͬ���ı���
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
					

            pThread = GetThread(iHostID);//��ȡ����ID��Ӧ���̣߳��������IDΪMDB_REP_EMPTY_HOST_ID,���Ӧ���̴߳Ӵ洢����
            if (pThread != NULL)//�߳��Ѿ�����
            {
            	for(int j = 0; j<MAX_REP_ROUTING_ID_COUTN; j++)
        		{
					if(pRouting->m_iRouteValue[j] != EMPTY_ROUTE_ID)
					{
						pThread->AddRoutingID(pRouting->m_iRouteValue[j]);//�Ѹ�·�ɵļ��ط�����߳�
					}
					else
					{
						break;
					}
        		}
            }
            else //�̲߳�����
            {
                pThread = new(std::nothrow)TMdbRepLoadThread(pClient);
                CHECK_OBJ(pThread);
                pThread->SetRuningInfo(m_pMdbCfg);
				for(int j = 0; j<MAX_REP_ROUTING_ID_COUTN; j++)
        		{
					if(pRouting->m_iRouteValue[j] != EMPTY_ROUTE_ID)
					{
						pThread->AddRoutingID(pRouting->m_iRouteValue[j]);//�Ѹ�·�ɵļ��ط�����߳�
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
        * ��������	:  GetClient
        * ��������	:  ����hostID��ȡ��Ӧ������
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
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
        * ��������	:  IsFileOutOfDate
        * ��������	:  �ļ��Ƿ����
        * ����		:  
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
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
