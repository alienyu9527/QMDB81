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
#include "Helper/mdbDateTime.h"
//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;
//namespace QuickMDB
//{
    TRepClientEngine::TRepClientEngine():m_pClient(NULL)
    {
        m_pShmRep = NULL;
        m_pShmMgr = NULL;
		m_bRepFileExist = true;
		m_bShardBakBufFree = false;
		m_pMdbDSN = NULL;
    }

    TRepClientEngine::~TRepClientEngine()
    {
		SAFE_DELETE(m_pShmMgr);
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
		m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
		CHECK_OBJ(m_pShmDSN);
		m_pMdbDSN = m_pShmDSN->GetInfo();
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
		CHECK_RET(m_pShmMgr->Attach(),"Attach shm failed."); 

		m_pShmRep = (TMdbShmRep*)m_pShmMgr->GetRoutingRep();
		CHECK_OBJ(m_pShmRep);
		TMdbOnlineRepMemQueue * pOnlineRepMemQueue = (TMdbOnlineRepMemQueue *)m_pShmDSN->GetShardBakBufAreaShm(m_iHostID);
		CHECK_OBJ(pOnlineRepMemQueue);
		CHECK_RET(m_pOnlineRepQueueCtrl.Init(pOnlineRepMemQueue, m_pMdbDSN, true), "Init failed.");
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
		if(m_pShmRep->m_iRepMode[m_iHostID] < 0)
		{
			CHECK_RET(UpdateRepMode(),"UpdateRepMode failed.");
		}
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
                	m_pShmRep->SetNetState(m_iHostID, false);
					CHECK_RET(UpdateRepMode(true),"UpdateRepMode failed.");
					TADD_DETAIL("m_pShmRep->m_iRepMode[%d] = [%d]", m_iHostID, m_pShmRep->m_iRepMode[m_iHostID]);
                    //连接失败，删除过期文件
                    iRet = CheckOverdueRepFile();
                    if (iRet != ERROR_SUCCESS)
                    {
                        TADD_ERROR(iRet,"CheckRepFile failed.");
                    }
                }  
				else
				{
					m_pShmRep->SetNetState(m_iHostID, true);
				}
            }
			//CHECK_RET(UpdateRepMode(),"UpdateRepMode failed.");
			TADD_DETAIL("m_pShmRep->m_iRepMode[%d] = [%d]", m_iHostID, m_pShmRep->m_iRepMode[m_iHostID]);
			if(m_pShmRep->m_iRepMode[m_iHostID] == 0)//文件同步模式
			{
				TADD_DETAIL("File-rep Mode.");
				//发送同步文件，无同步文件则sleep 1s
	            if (E_REP_NTC_OK == m_pClient->m_eState)
	            {
	                iRet = SendRepFile();
	                if (iRet!=ERROR_SUCCESS)
	                {
	                    TADD_ERROR(ERR_NET_SEND_FAILED, "SendRepFile failed.");
	                }
	                //响应服务端命令
	                TMdbMsgInfo* pMsg=NULL;
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
			else if(m_pShmRep->m_iRepMode[m_iHostID] == 1)//飞行同步模式
			{
				TADD_DETAIL("Online-rep Mode.");
				if (E_REP_NTC_OK == m_pClient->m_eState)
	            {
					iRet = SendRepData();
	                if (iRet!=ERROR_SUCCESS)
	                {
	                    TADD_ERROR(ERR_NET_SEND_FAILED, "SendRepData failed.");
	                }
	                //响应服务端命令
	                TMdbMsgInfo* pMsg=NULL;
	                iRet = m_pClient->GetMsg(pMsg);
	                if (iRet == 0)
	                {
	                    DoServerCmd(pMsg);
	                }
				}
	            else
	            {
                	m_pShmRep->SetNetState(m_iHostID, false);
	                CHECK_RET(UpdateRepMode(true),"UpdateRepMode failed.");
					TADD_DETAIL("m_pShmRep->m_iRepMode[%d] = [%d]", m_iHostID, m_pShmRep->m_iRepMode[m_iHostID]);
					continue;
	            }
			}
			else
			{
				TADD_ERROR(ERR_DB_INVALID_VALUE, "Invalid Rep Mode[%d].", m_pShmRep->m_iRepMode[m_iHostID]);
			}
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TRepClientEngine::UpdateRepMode(bool bRollback)
	{
		int iRet = 0;
		
		if(!m_pMdbDSN->m_bIsOnlineRep)
		{
			m_pShmRep->SetRepMode(m_pMdbDSN, m_iHostID, 0);
			return iRet;
		}
		if(m_pClient->m_eState != E_REP_NTC_OK || !m_pShmRep->m_bNetConnect[m_iHostID] || bRollback)
		{
			if(bRollback && m_pShmRep->m_iRepMode[m_iHostID] == 1)
			{
				//pop位置回退至clean
				m_pOnlineRepQueueCtrl.RollbackPopPos();
				TADD_NORMAL("Switch online-rep to file-rep, rollback pop to clean.");
			}
			m_pShmRep->SetRepMode(m_pMdbDSN, m_iHostID, 0);
			return iRet;
		}
		CHECK_RET(CheckShardBakBuf(), "CheckShardBakBuf failed.");
		if(!m_bShardBakBufFree)
		{
			if(m_pShmRep->m_iRepMode[m_iHostID] ==1)
			{
				TADD_NORMAL("Switch online-rep to file-rep.");
			}
			m_pShmRep->SetRepMode(m_pMdbDSN, m_iHostID, 0);
			return iRet;
		}
		CHECK_RET(CheckRepFile(), "CheckRepFile failed.");
		if(m_bRepFileExist)
		{
			if(m_pShmRep->m_iRepMode[m_iHostID] ==1)
			{
				TADD_NORMAL("Find rep file, use file-rep mode.");
			}
			m_pShmRep->SetRepMode(m_pMdbDSN, m_iHostID, 0);
			return iRet;
		}
		if(m_pShmRep->m_iRepMode[m_iHostID] ==0)
		{
			TADD_NORMAL("Switch file-rep to online-rep.");
		}
		m_pShmRep->SetRepMode(m_pMdbDSN, m_iHostID, 1);//若无不适合飞行模式条件，采用飞行模式同步
		return iRet;
	}

	int TRepClientEngine::UpdateOnlineRepMode(bool bRollback)
	{
		int iRet = 0;
		
		if(!m_pMdbDSN->m_bIsOnlineRep)
		{
			m_pShmRep->SetRepMode(m_pMdbDSN, m_iHostID, 0);
			return iRet;
		}
		if(m_pClient->m_eState != E_REP_NTC_OK || !m_pShmRep->m_bNetConnect[m_iHostID] || bRollback)
		{
			if(bRollback && m_pShmRep->m_iRepMode[m_iHostID] == 1)
			{
				//pop位置回退至clean
				m_pOnlineRepQueueCtrl.RollbackPopPos();
				TADD_NORMAL("Switch online-rep to file-rep, rollback pop to clean.");
			}
			m_pShmRep->SetRepMode(m_pMdbDSN, m_iHostID, 0);
			return iRet;
		}
		CHECK_RET(CheckShardBakBuf(), "CheckShardBakBuf failed.");
		if(!m_bShardBakBufFree)
		{
			if(m_pShmRep->m_iRepMode[m_iHostID] ==1)
			{
				TADD_NORMAL("Switch online-rep to file-rep.");
			}
			m_pShmRep->SetRepMode(m_pMdbDSN, m_iHostID, 0);
			return iRet;
		}
		
		if(m_pShmRep->m_iRepMode[m_iHostID] ==0)
		{
			TADD_NORMAL("Switch file-rep to online-rep.");
		}
		m_pShmRep->SetRepMode(m_pMdbDSN, m_iHostID, 1);//若无不适合飞行模式条件，采用飞行模式同步
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
        while((pFileName=m_tFileScanner.GetNext())!=NULL)
        {
			m_bRepFileExist = true;
			TADD_DETAIL("m_bRepFileExist = [%s], fileName = [%s]", m_bRepFileExist?"TRUE":"FALSE", pFileName);
			return iRet;
        }
		m_bRepFileExist = false;
		TADD_DETAIL("m_bRepFileExist = [%s]", m_bRepFileExist?"TRUE":"FALSE");
        TADD_FUNC("Finish.");
        return iRet;
    }

	/******************************************************************************
	* 函数名称	:  CheckShardBakBuf
	* 函数描述	:  检查链路缓存使用情况
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jiang.lili
	*******************************************************************************/
	int TRepClientEngine::CheckShardBakBuf()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		int iPercent = m_pOnlineRepQueueCtrl.GetUsedPercentage();
		TADD_DETAIL("[%d] buf used.", iPercent);
		m_bShardBakBufFree = iPercent<MAX_ONLINE_REP_BUF_USE_PERCENT?true:false;
		TADD_FUNC("Finish.");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  CheckOverdueRepFile
	* 函数描述	:  检查同步文件是否过期，删除过期同步文件
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jiang.lili
	*******************************************************************************/
	int TRepClientEngine::CheckOverdueRepFile()
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
            if (TMdbNtcDateTime::GetDiffSeconds(atoi(tSplit[2]), TMdbNtcDateTime::GetCurTime())>m_iFileInvalidTime)
            {
                TADD_NORMAL("Remove file=[%s]",pFileName);
                TMdbNtcFileOper::Remove(pFileName);
            }
        }
		m_bRepFileExist = true;
		TADD_FUNC("Finish.");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  SendRepData
	* 函数描述	:  发送同步记录，无同步文件，则sleep 10ms
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jiang.xiaolong
	*******************************************************************************/
	int TRepClientEngine::SendRepData()
	{
		TADD_FUNC("Start");
		int iRet = 0;
		char * sOneRecord = NULL;
		int iLen = 0;
		int iCount = 0;
		int iEmpty = 0; //连续链路缓存空次数
		while(true)
		{
			if(iCount != 0 && iCount%2000 == 0)
			{
				time(&m_tThreadUpdateTime);
			}
			if(m_pShmRep->m_iRepMode[m_iHostID] != 1)
			{
				TADD_NORMAL("Detect file rep mode.");
				break;
			}
			iRet = m_pOnlineRepQueueCtrl.Pop();
			switch(iRet)
	        {
	        case T_EMPTY://链路缓存空
	            {
					iEmpty++;
	                break;
	            }           
	        case T_SUCCESS://有数据
	            {
	                TADD_DETAIL("data=[%s]",m_pOnlineRepQueueCtrl.GetData());
			        sOneRecord = m_pOnlineRepQueueCtrl.GetData(); 
			        iLen = m_pOnlineRepQueueCtrl.GetRecordLen();
			        if(sOneRecord[0] != 0)//链路缓存有数据
			        {
						iRet = m_pClient->SendData(sOneRecord, iLen);
						if(iRet < 0)
						{
							TADD_WARNING("Send data to Host [%d] failed.", m_iHostID);
                			m_pShmRep->SetNetState(m_iHostID, false);
							CHECK_RET(UpdateRepMode(true), "UpdateRepMode failed.");
							TADD_DETAIL("m_pShmRep->m_iRepMode[%d] = [%d]", m_iHostID, m_pShmRep->m_iRepMode[m_iHostID]);
						}
						else
						{
							m_pShmRep->SetNetState(m_iHostID, true);
						}
						iEmpty = 0;
			        }
	                break;
	            }
	        default:
	            {
	                TADD_ERROR(iRet, "Queue iRet = [%d]",iRet);
	                break;
	            }
	        }
			if(iEmpty >= 100) //连续100次取不到数据就发送心跳包
			{
				iRet = m_pClient->SendData();//清空缓存
				if(iRet < 0)
				{
					TADD_WARNING("Send data to Host [%d] failed.", m_iHostID);
        			m_pShmRep->SetNetState(m_iHostID, false);
					CHECK_RET(UpdateRepMode(true), "UpdateRepMode failed.");
					TADD_DETAIL("m_pShmRep->m_iRepMode[%d] = [%d]", m_iHostID, m_pShmRep->m_iRepMode[m_iHostID]);
				}
				else if(iRet != 1)//有数据发送
				{
					m_pShmRep->SetNetState(m_iHostID, true);
				}
				m_pClient->SendHearbeat();//发送心跳（兼容1.2）
            	TMdbDateTime::MSleep(10);
				return ERROR_SUCCESS;
			}
			iCount++;
		}
		TADD_FUNC("Finish");
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
			CHECK_RET(UpdateOnlineRepMode(), "UpdateRepMode failed.");
            m_pClient->SendHearbeat();//发送心跳（兼容1.2）
            return iRet;
        }
        m_tFileScanner.Sort();
        
        //发送文件
        const char* pFileName = NULL;
        TMdbNtcString sFileFullName;

        while((pFileName=m_tFileScanner.GetNext())!=NULL)
        {
            TADD_NORMAL("Remove file=[%s]",pFileName);
            CHECK_RET(m_pClient->SendData(pFileName),"Send file[%s] failed.", pFileName);
			m_pShmRep->SetNetState(m_iHostID, true);
            TMdbNtcFileOper::Remove(pFileName);
        }
		//CHECK_RET(UpdateRepMode(), "UpdateRepMode failed.");
		TADD_DETAIL("m_pShmRep->m_iRepMode[%d] = [%d]", m_iHostID, m_pShmRep->m_iRepMode[m_iHostID]);
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
    int TRepClientEngine::DoServerCmd(TMdbMsgInfo* pMsg)
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





	TMdbWriteRepLog::TMdbWriteRepLog()
	{
		m_bRepFileExist = true;
		m_bShardBakBufFree = false;
	}
	
	TMdbWriteRepLog::~TMdbWriteRepLog()
	{
		SAFE_DELETE(m_pShmMgr);
	}
	
	int TMdbWriteRepLog::Init(int iHostID, const char* sDsn, const char* sFilePath)
	{
		TADD_FUNC("START");
		int iRet = 0;
		m_iHostID = iHostID;
		CHECK_OBJ(sDsn);
        m_pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDsn);
		m_pShmMgr = new (std::nothrow)TMdbShmRepMgr(sDsn);
		CHECK_OBJ(m_pShmMgr);
		CHECK_RET(m_pShmMgr->Attach(),"Attach shm failed."); 

		m_pShmRep = (TMdbShmRep*)m_pShmMgr->GetRoutingRep();
		CHECK_OBJ(m_pShmRep);
        m_pDsn = m_pShmDsn->GetInfo();
        CHECK_OBJ(m_pDsn);    
		
        CHECK_OBJ(sFilePath);
        m_strPath = sFilePath;
        if (m_strPath[m_strPath.GetLength()-1]!='/')
        {
            m_strPath.Append("/");
        }
        m_pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
        CHECK_OBJ(m_pConfig);
        TMdbOnlineRepMemQueue * pOnlineRepMemQueue = (TMdbOnlineRepMemQueue *)m_pShmDsn->GetShardBakBufAreaShm(m_iHostID);
		CHECK_OBJ(pOnlineRepMemQueue);
		CHECK_RET(m_tOnlineRepQueueCtrl.Init(pOnlineRepMemQueue, m_pDsn, true), "Init failed.");
		
        char sFilePatten[MAX_NAME_LEN];//文件名称的格式 Rep.HostID.*.OK
        snprintf(sFilePatten, MAX_NAME_LEN, "Rep.%d.*.OK", m_iHostID);        
        m_tFileScanner.AddFilter(sFilePatten);

        if(m_pConfig->GetIsStartShardBackupRep() == true)
        {
            CHECK_RET(m_mdbReplog.Init(sDsn, m_tOnlineRepQueueCtrl, m_iHostID),"mdbReplog init failed.");
        }
		TADD_FUNC("END");
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
	int TMdbWriteRepLog::CheckRepFile()
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
		while((pFileName=m_tFileScanner.GetNext())!=NULL)
		{
			m_bRepFileExist = true;
			TADD_DETAIL("m_bRepFileExist = [%s], fileName = [%s]", m_bRepFileExist?"TRUE":"FALSE", pFileName);
			return iRet;
		}
		m_bRepFileExist = false;
		TADD_DETAIL("m_bRepFileExist = [%s]", m_bRepFileExist?"TRUE":"FALSE");
		TADD_FUNC("Finish.");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  CheckShardBakBuf
	* 函数描述	:  检查链路缓存使用情况
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jiang.lili
	*******************************************************************************/
	int TMdbWriteRepLog::CheckShardBakBuf()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		int iPercent = m_tOnlineRepQueueCtrl.GetUsedPercentage();
		TADD_DETAIL("[%d] buf used.", iPercent);
		m_bShardBakBufFree = iPercent<MAX_ONLINE_REP_BUF_USE_PERCENT?true:false;
		TADD_FUNC("Finish.");
		return iRet;
	}

	int TMdbWriteRepLog::UpdateRepMode(bool bRollback)
	{
		int iRet = 0;
		
		if(!m_pDsn->m_bIsOnlineRep)
		{
			m_pShmRep->SetRepMode(m_pDsn, m_iHostID, 0);
			return iRet;
		}
		if(bRollback || !m_pShmRep->m_bNetConnect[m_iHostID])
		{
			if(bRollback && m_pShmRep->m_iRepMode[m_iHostID] == 1)
			{
				//pop位置回退至clean
				m_tOnlineRepQueueCtrl.RollbackPopPos();
				TADD_NORMAL("Switch online-rep to file-rep, rollback pop to clean.");
			}
			m_pShmRep->SetRepMode(m_pDsn, m_iHostID, 0);
			return iRet;
		}
		CHECK_RET(CheckRepFile(), "CheckRepFile failed.");
		if(m_bRepFileExist)
		{
			if(m_pShmRep->m_iRepMode[m_iHostID] == 1)
			{
				TADD_NORMAL("Find rep file, use file-rep mode.");
			}
			m_pShmRep->SetRepMode(m_pDsn, m_iHostID, 0);
			return iRet;
		}
		
		CHECK_RET(CheckShardBakBuf(), "CheckShardBakBuf failed.");
		if(!m_bShardBakBufFree)
		{
			if(m_pShmRep->m_iRepMode[m_iHostID] ==1)
			{
				TADD_NORMAL("Switch online-rep to file-rep.");
			}
			m_pShmRep->SetRepMode(m_pDsn, m_iHostID, 0);
			return iRet;
		}
		if(m_pShmRep->m_iRepMode[m_iHostID] ==0)
		{
			TADD_NORMAL("Switch file-rep to online-rep.");
		}
		m_pShmRep->SetRepMode(m_pDsn, m_iHostID, 1);//若无不适合飞行模式条件，采用飞行模式同步
		return iRet;
	}

	int TMdbWriteRepLog::UpdateOnlineRepMode(bool bRollback)
	{
		int iRet = 0;
		
		if(!m_pDsn->m_bIsOnlineRep)
		{
			m_pShmRep->SetRepMode(m_pDsn, m_iHostID, 0);
			return iRet;
		}
		if(bRollback || !m_pShmRep->m_bNetConnect[m_iHostID])
		{
			if(bRollback && m_pShmRep->m_iRepMode[m_iHostID] == 1)
			{
				//pop位置回退至clean
				m_tOnlineRepQueueCtrl.RollbackPopPos();
				TADD_NORMAL("Switch online-rep to file-rep, rollback pop to clean.");
			}
			m_pShmRep->SetRepMode(m_pDsn, m_iHostID, 0);
			return iRet;
		}
		CHECK_RET(CheckShardBakBuf(), "CheckShardBakBuf failed.");
		if(!m_bShardBakBufFree)
		{
			if(m_pShmRep->m_iRepMode[m_iHostID] ==1)
			{
				TADD_NORMAL("Switch online-rep to file-rep.");
			}
			m_pShmRep->SetRepMode(m_pDsn, m_iHostID, 0);
			return iRet;
		}
		if(m_pShmRep->m_iRepMode[m_iHostID] ==0)
		{
			TADD_NORMAL("Switch file-rep to online-rep.");
		}
		m_pShmRep->SetRepMode(m_pDsn, m_iHostID, 1);//若无不适合飞行模式条件，采用飞行模式同步
		return iRet;
	}

	
	int  TMdbWriteRepLog::Execute()
	{
		TADD_FUNC("Start.");
        int iRet = 0;
		char * sOneRecord = NULL;

        TADD_NORMAL("thread[%d] Start.", GetThreadId());
		if(m_pShmRep->m_iRepMode[m_iHostID] < 0)
		{
			CHECK_RET(UpdateRepMode(),"UpdateRepMode failed.");
		}
        while(true)
        {
        	time(&m_tThreadUpdateTime);
            if(this->TestCancel())
            {
                TADD_NORMAL("thread[%d] cancel", GetThreadId());
                break;
            }
			if(m_pShmRep->m_iRepMode[m_iHostID] == 0)//文件同步模式
			{
				iRet = m_tOnlineRepQueueCtrl.Pop();
				switch(iRet)
		        {
		        case T_EMPTY://链路缓存空
		            {
		                m_mdbReplog.Log(true);
		                TMdbDateTime::MSleep(10);
						CHECK_RET(UpdateRepMode(),"UpdateRepMode failed.");
		                break;
		            }           
		        case T_SUCCESS://有数据
		            {
						sOneRecord = NULL;
		                TADD_DETAIL("data=[%s]",m_tOnlineRepQueueCtrl.GetData());
				        sOneRecord = m_tOnlineRepQueueCtrl.GetData(); 
				        if(sOneRecord[0] != 0)//链路缓存有数据
				        {
				        	m_mdbReplog.Log(false);
				        }
						else
						{
							m_mdbReplog.Log(true);
						}
						CHECK_RET(UpdateRepMode(),"UpdateRepMode failed.");
		                break;
		            }
		        default:
		            {
		                TADD_ERROR(iRet, "Queue iRet = [%d]",iRet);
		                break;
		            }
		        }
			}
			else if(m_pShmRep->m_iRepMode[m_iHostID] == 1)//飞行同步模式
			{
				CHECK_RET(UpdateOnlineRepMode(),"UpdateRepMode failed.");
                TMdbDateTime::MSleep(10);
			}
			else
			{
				TADD_ERROR(ERR_DB_INVALID_VALUE, "Invalid Rep Mode[%d].", m_pShmRep->m_iRepMode[m_iHostID]);
				CHECK_RET(UpdateRepMode(),"UpdateRepMode failed.");
			}
        }
        
        TADD_FUNC("Finish.");
        return iRet;
	}
	
	void TMdbWriteRepLog::Cleanup()
	{
		
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
		SAFE_DELETE(m_pClientEngine);
		SAFE_DELETE(m_pWriteRepLog);
    }
       /******************************************************************************
        * 函数名称	:  Init
        * 函数描述	:  初始化
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TRepClient::Init(const char *sDsn, int iHostID)
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

		m_iHostID = iHostID;
		
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
        //连接指定备机
        TADD_NORMAL("Rep host ID = [%d]", m_iHostID);
        m_pClientEngine = new(std::nothrow)TRepClientEngine();
        CHECK_OBJ(m_pClientEngine);
        CHECK_RET(m_pClientEngine->Init(*(m_pShmMgr->GetRoutingRep()->GetRepHostByID(m_iHostID)), m_pRepConfig->m_sRepPath.c_str(), m_pRepConfig->m_iFileInvalidTime,m_sDsn), "TRepClientEngine init failed.");
        if(!m_pClientEngine->Run())
        {
            CHECK_RET(ERR_OS_CREATE_THREAD, "TRepClientEngine Run failed.");
        }

		m_pWriteRepLog = new(std::nothrow)TMdbWriteRepLog();
		CHECK_RET(m_pWriteRepLog->Init(m_iHostID, m_sDsn, m_pRepConfig->m_sRepPath.c_str()), "Init failed");
		if(!m_pWriteRepLog->Run())
		{
			CHECK_RET(ERR_OS_CREATE_THREAD, "TMdbWriteRepLog Run failed.");
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
		//等待落地线程退出
		m_pWriteRepLog->Cancel();
		while(false == m_pWriteRepLog->Wait())
        {
            TADD_NORMAL("waiting for thread[%d] to cancel", m_pWriteRepLog->GetThreadId());
            TMdbNtcDateTime::Sleep(5);
        }
		//等待发送线程退出
		m_pClientEngine->Cancel();
        while(false == m_pClientEngine->Wait())
        {
            TADD_NORMAL("waiting for thread[%d] to cancel", m_pClientEngine->GetThreadId());
            /*QuickMDB::*/TMdbNtcDateTime::Sleep(5);
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

	int TRepClient::CheckThreadHeart()
	{
		TADD_FUNC("Start.");
        int iRet = 0;
		int iHearBeatFatal = 30; // 心跳超时时间，单位:秒
		
		time_t tCurTime;
   		time(&tCurTime);

		if (0 != m_pWriteRepLog->m_tThreadUpdateTime)
		{
			if (tCurTime - m_pWriteRepLog->m_tThreadUpdateTime > iHearBeatFatal)
			{
				TADD_WARNING("Thread %d  lost heartbeat,difftime= %d s,will restart.",
					m_pWriteRepLog->GetThreadId(), tCurTime - m_pWriteRepLog->m_tThreadUpdateTime);
				
				//销毁主线程
				m_pWriteRepLog->Kill();

				//重启主线程
				if(!m_pWriteRepLog->Run())
	            {
	                CHECK_RET(ERR_OS_CREATE_THREAD, "TMdbWriteRepLog Run failed When Restart.");
	            }
				else
				{
					TADD_WARNING("Restart thread success,new thread id = %d.", m_pWriteRepLog->GetThreadId());
				}
			}  
		}
   		time(&tCurTime);
		if (0 != m_pClientEngine->m_tThreadUpdateTime)
		{
			if (tCurTime - m_pClientEngine->m_tThreadUpdateTime > iHearBeatFatal)
			{
				TADD_WARNING("Thread %d  lost heartbeat,difftime= %d s,will restart.",
					m_pClientEngine->GetThreadId(), tCurTime - m_pClientEngine->m_tThreadUpdateTime);
				
				//需要先停止发送客户端
				m_pClientEngine->KillRepDataClient();
				//再销毁主线程
				m_pClientEngine->Kill();

				//重启主线程
				if(!m_pClientEngine->Run())
	            {
	                CHECK_RET(ERR_OS_CREATE_THREAD, "TRepClientEngine Run failed When Restart.");
	            }
				else
				{
					TADD_WARNING("Restart thread success,new thread id = %d.", m_pClientEngine->GetThreadId());
				}
			}  
		}

        TADD_FUNC("Finish.");
        return iRet;
	}

//}
