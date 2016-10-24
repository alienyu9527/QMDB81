/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbRepServerCtrl.cpp		
*@Description： 负责同步数据接收，和数据加载时同步数据的发送
*@Author:		jiang.lili
*@Date：	    2014/03/20
*@History:
******************************************************************************************/
#include "Replication/mdbRepServerCtrl.h"
#include "Helper/mdbDateTime.h"

//namespace QuickMDB
//{
    TRepServerDataRcv::TRepServerDataRcv():m_bCheckTime(true),m_ptFlushDaoCtrl(NULL)
    {	
        memset(m_sCurTime, 0, sizeof(m_sCurTime));
        memset(m_sLinkTime, 0, sizeof(m_sLinkTime));
        m_iMsgBufLen=0;
        m_psMsgBuf = NULL;
    }


    TRepServerDataRcv::~TRepServerDataRcv()
    {
        SAFE_DELETE(m_ptFlushDaoCtrl);
        SAFE_DELETE(m_psMsgBuf);
    }

    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	:  初始化
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepServerDataRcv::Init(const char* sDsn, const char* strIP, int iPort)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(strIP);
        m_strIP.Assign(strIP);
        m_iPort = iPort;

        TMdbDateTime::GetCurrentTimeStr(m_sLinkTime);
        
        m_sLeftBuf[0] = '\0';
        m_iLeftDataLen = 0;
        m_sOneRecord[0] = '\0';

        // 初始化时按(MAX_REP_SEND_BUF_LEN*2)申请好。如果接收的包比这大，再重新申请。
        m_iMsgBufLen = MAX_REP_SEND_BUF_LEN*2;
        m_psMsgBuf = new(std::nothrow) char[m_iMsgBufLen];
        if(NULL == m_psMsgBuf)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY, "initialize message buff failed.no memory.");
            return ERR_OS_NO_MEMROY;
        }
        memset(m_psMsgBuf, 0, m_iMsgBufLen);

        m_ptFlushDaoCtrl = new(std::nothrow) TRepFlushDAOCtrl();
        CHECK_OBJ(m_ptFlushDaoCtrl);
        CHECK_RET(m_ptFlushDaoCtrl->Init(sDsn), "TRepFlushDAOCtrl init failed.");

        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  IsSame
    * 函数描述	:  是否是处理同一个备机的引擎
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TRepServerDataRcv::IsSame(const char* strIP, int iPort)
    {
        if (NULL == strIP || TMdbNtcStrFunc::StrNoCaseCmp(strIP, m_strIP.c_str()) != 0 || iPort != m_iPort)
        {
            return false;
        }
        return true;
    }

    /******************************************************************************
    * 函数名称	:  RcvData
    * 函数描述	:  接收数据
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepServerDataRcv::DealRcvData(const char* sDataBuf, int iBufLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if (iBufLen <=0)
        {
            return iRet;
        }

        // 超过3小时，认为已经没有过期数据了。不再校验时间戳
        if(m_bCheckTime)
        {
            TMdbDateTime::GetCurrentTimeStr(m_sCurTime);
            if(TMdbDateTime::GetDiffHour(m_sCurTime, m_sLinkTime) > 3)
            {
                m_bCheckTime = false;
            }
        }

        iRet = CombineData(sDataBuf, iBufLen);
        if( iRet < 0)
        {
            return iRet;
        }
        
        m_iDealDataLen = 0;
        
        while(true)
        {
            iRet =CheckDataCompletion();
            if(iRet > 0){continue;}
            if(iRet < 0){break;}

            m_sOneRecord[0]='\0';
            int iLen = GetRecdLen();
            strncpy(m_sOneRecord, m_psMsgBuf+m_iDealDataLen, iLen);
            m_sOneRecord[iLen] = '\0';
            TADD_DETAIL("to proc[%d][%s]", iLen,m_sOneRecord);
            iRet = m_ptFlushDaoCtrl->Execute(m_sOneRecord,m_bCheckTime);
            if(iRet < 0)
            {
                CHECK_RET(iRet, "Execute failed:[%d][%s]",iLen,m_sOneRecord );
            }

            m_iDealDataLen+=iLen;
        }

        TADD_FUNC("Finish.");
        return ERROR_SUCCESS;
    }

    int TRepServerDataRcv::GetRecdLen()
    {
        int iLen   = (m_psMsgBuf[m_iDealDataLen+2]-'0')*1000 ;
        iLen +=  (m_psMsgBuf[m_iDealDataLen+3]-'0')*100 ;
        iLen+=(m_psMsgBuf[m_iDealDataLen+4]-'0')*10   ;
        iLen += (m_psMsgBuf[m_iDealDataLen+5]-'0');
        TADD_DETAIL("Record Length=[%d]",iLen);
        return iLen;
    }

    //拼接数据，上次遗留数据+本次新接收数据
    int TRepServerDataRcv::CombineData(const char* sDataBuf, int iBufLen)
    {
        int iRet = 0;
        TADD_DETAIL("recv data:[%d][%s]",iBufLen, sDataBuf);
        
        if(iBufLen + m_iLeftDataLen>= m_iMsgBufLen)
        {
            TADD_NORMAL("Msg buffer len[%d], m_iLeftDataLen[%d], m_iMsgBufLen[%d]", iBufLen, m_iLeftDataLen, m_iMsgBufLen);
            SAFE_DELETE(m_psMsgBuf);
            int iNewBuffLen = iBufLen+m_iLeftDataLen + MAX_REP_SEND_BUF_LEN;
            m_psMsgBuf = new(std::nothrow) char[iNewBuffLen];
            if(NULL == m_psMsgBuf)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY, "reallocate message buff failed. no memory.");
                return ERR_OS_NO_MEMROY;
            }
            m_iMsgBufLen = iNewBuffLen;
            memset(m_psMsgBuf, 0, m_iMsgBufLen);
        }
        else
        {
            memset(m_psMsgBuf, 0, m_iMsgBufLen);
        }


        if(m_iLeftDataLen > 0)
        {
            strncpy(m_psMsgBuf,m_sLeftBuf,m_iLeftDataLen);
            strncat(m_psMsgBuf, sDataBuf, iBufLen);
        }
        else
        {
            strncpy(m_psMsgBuf,sDataBuf,iBufLen);
        }
        
        m_iTotalDataLen = m_iLeftDataLen+iBufLen;
        m_psMsgBuf[m_iTotalDataLen] = '\0';
        
        memset(m_sLeftBuf, 0, sizeof(m_sLeftBuf));
        m_iLeftDataLen = 0;
        
        return iRet;
    }

    void TRepServerDataRcv::SaveRecord()
    {
        memset(m_sLeftBuf,0,MAX_REP_SEND_BUF_LEN);
        m_iLeftDataLen = m_iTotalDataLen - m_iDealDataLen; 
        if(m_iLeftDataLen > 0)
        {
            strncpy(m_sLeftBuf,m_psMsgBuf+m_iDealDataLen, m_iLeftDataLen);
            m_sLeftBuf[m_iLeftDataLen] = '\0';
        }    
        TADD_DETAIL("save data:[%d][%s].", m_iLeftDataLen,m_sLeftBuf);
        return;
    }

    int TRepServerDataRcv::CheckDataCompletion()
    {
        if(m_iTotalDataLen < m_iDealDataLen+10)
        {
            SaveRecord();
            TADD_DETAIL("length too short,  and save it. Left Buff=[%s].", m_sLeftBuf);
            return -1;
        }

        if(strlen(m_psMsgBuf+m_iDealDataLen) == 0)
        {
            memset(m_sLeftBuf,0,MAX_REP_SEND_BUF_LEN);
            m_iLeftDataLen = 0;
            return -1;
        }

        if(m_psMsgBuf[m_iDealDataLen] ==MDB_REP_RCD_BEGIN[0] && m_psMsgBuf[m_iDealDataLen+1] == MDB_REP_RCD_BEGIN[1])
        {
            int iLen = GetRecdLen();
            if(iLen+m_iDealDataLen > m_iTotalDataLen)
            {
                SaveRecord();
                TADD_DETAIL("Data not complete, and save it.  Left Buff=[%s].", m_sLeftBuf);
                return -1;
            }

            if(m_psMsgBuf[m_iDealDataLen+iLen-1] != '#' || m_psMsgBuf[m_iDealDataLen+iLen-2] != '#')
            {
                 TADD_ERROR(ERR_NET_RECV_DATA_FORMAT,"Record is invalid, Lost This Record !deal-len[%d],recd-len[%d],total-len[%d]"
                        ,m_iDealDataLen, iLen, m_iTotalDataLen);                           
                 m_iDealDataLen+=iLen;                
                 return 1;
            }

            if(iLen > MAX_VALUE_LEN)
            {
                TADD_ERROR(ERR_NET_RECV_DATA_FORMAT,"Record too Long Lost This Record !");
                m_iDealDataLen+=iLen;
                return 1;
            }

        }
        else
        {
            m_iDealDataLen++;
            return 1;
        }
        
        return 0;
    }

    TRepServerDataSend::TRepServerDataSend():m_pDataBase(NULL), m_pCurQuery(NULL), m_pConfig(NULL),m_pRepConfig(NULL),m_pLoadDao(NULL),
        m_pPeerInfo(NULL)
    {

    }

    TRepServerDataSend::~TRepServerDataSend()
    {
        SAFE_DELETE(m_pDataBase);
        SAFE_DELETE(m_pLoadDao);
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
    int TRepServerDataSend::Init(const char* sDsn)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(sDsn);

		TMdbShmDSN * tShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
		m_tDsn = tShmDsn->GetInfo();
		
		m_tPageCtrl.SetDSN(m_tDsn->sName);
		m_tVarcharCtrl.Init(m_tDsn->sName);

		for(int i = 0; i<MAX_COLUMN_COUNTS; i++)
		{
			m_iVarColPos[i] = 0;
		}
		m_iVarColCount = 0;
		
		m_pCurPage = NULL;
		m_pCurDataAddr = NULL;	
		m_pNextDataAddr = NULL;
		m_iDataOffset = 0;
		
		m_iWhichPos = 0;
		m_iRowId = 0;
		memset(m_sVarcharBuf, 0, sizeof(m_sVarcharBuf));
		m_iValueSize = 0;

		for(int i = 0; i<MAX_ROUTER_LIST_LEN; i++)
		{
			m_iRouteList[i] = EMPTY_ROUTE_ID;
		}
		m_iRouteCount = 0;

		for(int i = 0; i<MAX_LOAD_TABLE_COLUMN_DIFF_COUNT; i++)
		{
			m_tTableChangeOper[i].Clear();
		}
		m_iChangeCount = 0;
		m_tRemoteStruct.Clear();
		
		m_bColMissAdd = false;
		m_bNullSizeChange = false;
		m_iNullSizeAdd = 0;
		
        try
        {
            m_pDataBase = new(std::nothrow) TMdbDatabase();
            CHECK_OBJ(m_pDataBase);
            if(m_pDataBase->ConnectAsMgr(sDsn) == false)
            {
                CHECK_RET(ERR_APP_INVALID_PARAM,"ConnectAsMgr [%s] error.",sDsn);
            }
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN,"Unknown error!\n");   
        } 

        m_pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
        CHECK_OBJ(m_pConfig);

        m_pRepConfig = new(std::nothrow) TMdbRepConfig();
        CHECK_OBJ(m_pRepConfig);
        CHECK_RET(m_pRepConfig->Init(sDsn), "TMdbRepConfig failed.");

        m_pLoadDao = new(std::nothrow) TRepLoadDao();
        CHECK_OBJ(m_pLoadDao);      
        CHECK_RET(m_pLoadDao->Init(m_pDataBase,m_pConfig), "TRepFlushDao init failed.");

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SendData
    * 函数描述	:  发送同步数据
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepServerDataSend::SendData(int iHostID, const char* sRoutinglist, TMdbPeerInfo* pPeerInfo, bool bIsMemLoad, const char * sRemoteTableInfo)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pPeerInfo);
        m_pPeerInfo = pPeerInfo;
		
        //删除该主机ID对应的落地文件
        CHECK_RET(CleanRepFile(iHostID), "Clean Rep file failed.");
        //发送同步数据
        char sTableSql[] = "SELECT TABLE_NAME FROM DBA_TABLES WHERE shard_backup = 'Y' ";
        std::string strTableName;
        
        try
        {
            TMdbQuery *pTableQuery = m_pDataBase->CreateDBQuery();
            CHECK_OBJ(pTableQuery);
            pTableQuery->SetSQL(sTableSql);
            pTableQuery->Open();
            while(pTableQuery->Next())//逐一获取各个表的数据
            {
                strTableName = pTableQuery->Field(0).AsString();  
				if(bIsMemLoad)
				{
					CHECK_RET(DealOneTableMem(strTableName.c_str(), sRoutinglist, sRemoteTableInfo), "Deal table [%s] failed.", strTableName.c_str()); 
				}
				else
				{
					CHECK_RET(DealOneTable(strTableName.c_str(), sRoutinglist), "Deal table [%s] failed.", strTableName.c_str());   
				}
            }
            //所有的数据发送完，需要发送一个结束标示
            sprintf(m_sSendBuf,"%s",LOAD_DATA_END_FLAG); 
            if(SendBufData(m_sSendBuf, strlen(m_sSendBuf)) < 0)
            {
                return ERR_NET_SEND_FAILED;
            }	        
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN,"Unknown error!\n");   
        } 

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TRepServerDataSend::SendData(const char* sTableName, const char* sRoutinglist, TMdbPeerInfo* pPeerInfo, bool bIsMemLoad, const char * sRemoteTableInfo)
    {
        TADD_NORMAL("Start. Routinglist = [%s], TableName = [ %s ], bIsMemLoad = [%s]", sRoutinglist==NULL?"NULL":sRoutinglist, sTableName, bIsMemLoad?"TRUE":"FALSE");
        int iRet = 0;

        CHECK_OBJ(pPeerInfo);
        m_pPeerInfo = pPeerInfo;
		
		if(bIsMemLoad)
		{
			iRet = DealOneTableMem(sTableName, sRoutinglist, sRemoteTableInfo);
			if(iRet < 0)
			{
				TADD_ERROR(iRet, "Deal table [%s] failed.", sTableName);
				if (!pPeerInfo->PostMessage(REP_TABLE_ERROR, strlen(REP_TABLE_ERROR)))
		        {
		            CHECK_RET(ERR_NET_SEND_FAILED, "Send endian match info to Rep failed..");
		        }
				return iRet;
			}
		}
		else
		{
			CHECK_RET(DealOneTable(sTableName, sRoutinglist), "Deal table [%s] failed.", sTableName);   
		}
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  CleanRepFile
    * 函数描述	:  清除该主机对应的同步文件
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepServerDataSend::CleanRepFile(int iHostID)
    {
        TADD_NORMAL("Start. HostID[%d]", iHostID);
        int iRet = 0;

        TMdbNtcFileScanner tFileScanner;
        if (MDB_REP_EMPTY_HOST_ID == iHostID)//主备双机互备模式，删除所有同步文件（兼容1.2）
        {
            tFileScanner.AddFilter("Rep.*.OK");
        }
        else//分片备份，删除指定主机的同步文件
        {
            char sFilterName[MAX_NAME_LEN];
            snprintf(sFilterName, MAX_NAME_LEN, "Rep.%d.*.OK", iHostID);
            tFileScanner.AddFilter(sFilterName);
        }
        if (!tFileScanner.ScanFile(m_pRepConfig->m_sRepPath.c_str()))
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "ScanFile in path[%s] failed.", m_pRepConfig->m_sRepPath.c_str());
        }
        const char* pFileName = NULL;
        while((pFileName=tFileScanner.GetNext())!=NULL)
        {
            TADD_NORMAL("Remove file[%s]", pFileName);
            TMdbNtcFileOper::Remove(pFileName);
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  DealOneTable
    * 函数描述	:  发送一张表的同步数据
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepServerDataSend::DealOneTable(const char* sTableName, const char* sRoutinglist)
    {
        TADD_FUNC("Start[%s:%s].", sTableName, sRoutinglist==NULL?"NULL":sRoutinglist);
        int iRet = ERROR_SUCCESS;
        try
        {
            int iRecordCount = 0;
            int iSendBufLen = 0;

            // 判断请求的表是否存在
            TMdbTable* pTable =m_pConfig->GetTableByName(sTableName);
            if(NULL == pTable)
            {
                memset(m_sSendBuf,0,MAX_SEND_BUF);
                sprintf(m_sSendBuf,"%s",REP_TABLE_NO_EXIST); 
                if(SendBufData(m_sSendBuf, (strlen(m_sSendBuf))) < 0){return ERR_NET_SEND_FAILED;}
                TADD_WARNING("Uploaded on table[%s] does not exist.",sTableName);
                return iRet;
            }
            
            //拼接报文头
            //memset(m_sSendBuf,0,MAX_REP_SEND_BUF_LEN);
            snprintf(m_sSendBuf,MAX_REP_SEND_BUF_LEN, "@@!!%s", sTableName);
            iSendBufLen = strlen(m_sSendBuf);

            //表记录
            m_pCurQuery = m_pLoadDao->GetQuery(sTableName, true, sRoutinglist);
            CHECK_OBJ(m_pCurQuery);
            m_pCurQuery->Open();
            while(m_pCurQuery->Next())
            {
                iRecordCount++;
                memset(m_sOneRecord,0,MAX_VALUE_LEN);
                snprintf(m_sOneRecord, MAX_VALUE_LEN, "%s", "!!");//记录分割
                GetOneRecord();
                int iOneRecordLen = strlen(m_sOneRecord);

                if(MAX_REP_SEND_BUF_LEN - iSendBufLen >= iOneRecordLen)// //如果发送缓冲区空间还够，则拷贝到发送缓冲区
                {
                    snprintf(m_sSendBuf+iSendBufLen, MAX_REP_SEND_BUF_LEN-iSendBufLen,"%s", m_sOneRecord);
                    iSendBufLen += iOneRecordLen;
                    if(iRecordCount%100 == 0) //达到100条也必须要发送
                    {
                        if(SendBufData(m_sSendBuf, iSendBufLen) < 0){return ERR_NET_SEND_FAILED;}
                        iSendBufLen = 0;
                        m_sSendBuf[0] = '\0';
                    }                    
                }
                else //如果缓冲区满了，则先将缓冲区的数据发送出去，再把本次获取的记录放到缓冲区中
                {
                    if(SendBufData(m_sSendBuf, iSendBufLen) < 0){return ERR_NET_SEND_FAILED;}
                    snprintf(m_sSendBuf, MAX_REP_SEND_BUF_LEN, "%s", m_sOneRecord); 
                    iSendBufLen = strlen(m_sOneRecord);
                }
            }

            //发送报文尾
            if(MAX_REP_SEND_BUF_LEN - iSendBufLen < 20)
            {
                if(SendBufData(m_sSendBuf,iSendBufLen) < 0){return ERR_NET_SEND_FAILED;}
                m_sSendBuf[0] = '\0';
                iSendBufLen = 0;
            }
            snprintf(m_sSendBuf+iSendBufLen, MAX_REP_SEND_BUF_LEN-iSendBufLen, "%s%d%s%s","!!&&.",iRecordCount,".OK", LOAD_DATA_END_FLAG);	
            if(SendBufData(m_sSendBuf, (strlen(m_sSendBuf))) < 0)
            {
                return ERR_NET_SEND_FAILED;
            }
            m_sSendBuf[0] = '\0';
            iSendBufLen = 0;
        }
        catch(TMdbException &e)
        {        
            CHECK_RET(ERROR_UNKNOWN, "[%s : %d] : TRepServerEngine::FillAndSendTableData() : Minidb-SQL=[%s], error_msg=[%s].", 
                __FILE__, __LINE__, e.GetErrSql(), e.GetErrMsg());
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN,"Unknown error!\n");   
        } 
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TRepServerDataSend::GetChainSnapshot(TMdbTable * pTable, int * &iPageIDList, int & iPageCounts)
	{
		int iRet = 0;
		CHECK_OBJ(pTable);
		int iPageId = 0;
		int iFreeId = -999;
		iPageCounts = 0;
		TMdbPage * pPage = NULL;
		CHECK_RET(pTable->tFreeMutex.Lock(true, &m_tDsn->tCurTime),"[%s].tFreeMutex.Lock() failed.",pTable->sTableName);
		CHECK_RET(pTable->tFullMutex.Lock(true, &m_tDsn->tCurTime),"[%s].tFreeMutex.Lock() failed.",pTable->sTableName);

		iPageIDList = new(std::nothrow) int[pTable->iFullPages+pTable->iFreePages+50];
		CHECK_OBJ(iPageIDList);
		for(int i = 0; i<pTable->iFullPages+pTable->iFreePages; i++)
		{
			iPageIDList[i] = 0;
		}
		
		iPageId = pTable->iFullPageID;
		while(iPageId > 0)
		{
			pPage = (TMdbPage * )m_tTSCtrl.GetAddrByPageID(iPageId);
			CHECK_OBJ(pPage);
			iPageIDList[iPageCounts++] = iPageId;
			iPageId = pPage->m_iNextPageID;
		}
		
		iPageId = pTable->iFreePageID;
		while(iPageId > 0)
		{
			if(iFreeId == -999)
			{
				iFreeId = iPageId;
			}
			else if(iPageId == iFreeId)
			{
				break;//free链是环形链表，不重复处理
			}
			pPage = (TMdbPage * )m_tTSCtrl.GetAddrByPageID(iPageId);
			CHECK_OBJ(pPage);
			iPageIDList[iPageCounts++] = iPageId;
			iPageId = pPage->m_iNextPageID;
		}
		CHECK_RET(pTable->tFullMutex.UnLock(true, m_tDsn->sCurTime),"[%s].tFreeMutex.UnLock() failed.",pTable->sTableName);
		CHECK_RET(pTable->tFreeMutex.UnLock(true, m_tDsn->sCurTime),"[%s].tFreeMutex.UnLock() failed.",pTable->sTableName);
		return iRet;
	}

	int TRepServerDataSend::ParseRoutingList(const char * sRoutinglist)
	{
		int iRet = 0;
		for(int i = 0; i<MAX_ROUTER_LIST_LEN; i++)
		{
			m_iRouteList[i] = 0;
		}
		m_iRouteCount = 0;
		
		if(sRoutinglist!=NULL)
		{
			TMdbNtcSplit tSplit;
            tSplit.SplitString(sRoutinglist, ',');
			if(atoi(tSplit[0]) != DEFAULT_ROUTE_ID)
    		{
				for(int i = 0; i<tSplit.GetFieldCount(); i++)
				{
					m_iRouteList[m_iRouteCount++] = atoi(tSplit[i]);
				}
    		}
		}
		return iRet;
	}

	int TRepServerDataSend::CheckRemoteTableInfo(TMdbTable * pTable, const char * sTableInfo)
	{
		int iRet = 0;
		m_iChangeCount = 0;
		int iExtra = 0;//本地比对端多出的列数
		int iMiss = 0;//本地比对端少掉的列数
		m_bColMissAdd = false;
		m_bNullSizeChange = false;
		m_iNullSizeAdd = 0;
		CHECK_OBJ(sTableInfo);
		for(int i = 0; i<MAX_LOAD_TABLE_COLUMN_DIFF_COUNT; i++)
		{
			m_tTableChangeOper[i].Clear();
		}
		m_tRemoteStruct.Clear();

		for(int i = 0; i<MAX_COLUMN_COUNTS; i++)
		{
			m_iVarColPos[i] = -1;
		}
		m_iVarColCount = 0;
		
		TMdbNtcSplit tNullSplit;
		tNullSplit.SplitString(sTableInfo, '$');
		if(tNullSplit.GetFieldCount()!=2) return -1;
		if((tNullSplit[1][0] == 'Y' && pTable->iOneRecordNullOffset < 0)\
			|| (tNullSplit[1][0] == 'N' && pTable->iOneRecordNullOffset > 0))
		{
			TADD_WARNING("Table [%s] on local host has different null-able setting with rep host.", pTable->sTableName);
			return -1;
		}
		int iLMaxMatch = -1;
		int iRMaxMatch = -1;
		TMdbNtcSplit tColSplit;
		tColSplit.SplitString(tNullSplit[0], '@');
		for(int i = 0; i<pTable->iColumnCounts; i++)
		{
			for(int j = 0; j<tColSplit.GetFieldCount(); j++)
			{
				TMdbNtcSplit tColInfoSplit;
				tColInfoSplit.SplitString(tColSplit[j],',');
				if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tColumn[i].sName, tColInfoSplit[0]) == 0)
				{
					if(iLMaxMatch< i) iLMaxMatch = i;
					if(iRMaxMatch< j) iRMaxMatch = j;
					if(i - iExtra + iMiss < j)//对端有列被跳过，需确认差异
					{
						for(int k = i - iExtra + iMiss;k<j;k++)
						{
							TMdbNtcSplit tTemp;
							tTemp.SplitString(tColSplit[k], ',');
							for(int l = i; l<pTable->iColumnCounts; l++)
							{
								if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tColumn[l].sName, tTemp[0]) == 0)
								{
									TADD_ERROR(ERR_SQL_COLUMN_INDEX_INVALID,"Column postion is different.");
									iRet = ERR_SQL_COLUMN_INDEX_INVALID;
									return iRet;
								}
							}	
						}
						//对端新增列
						for(int k = i - iExtra + iMiss;k<j;k++)
						{
							TMdbNtcSplit tTemp;
							tTemp.SplitString(tColSplit[k], ',');
							if(tTemp[3][0] == 'Y' && tTemp.GetFieldCount() == 5)
							{
								m_tTableChangeOper[m_iChangeCount].m_iColPos = i;
								m_tTableChangeOper[m_iChangeCount].m_iColType = atoi(tTemp[1]);
								m_tTableChangeOper[m_iChangeCount].m_iColLen = atoi(tTemp[2]);
								m_tTableChangeOper[m_iChangeCount].m_iChangeType = TC_ColumnAdd;
								m_bColMissAdd = true;
								if(m_tTableChangeOper[m_iChangeCount].m_iColType == DT_Int)
								{
									m_tTableChangeOper[m_iChangeCount].m_llValue = atol(tTemp[4]);
								}
								else if(m_tTableChangeOper[m_iChangeCount].m_iColType == DT_DateStamp)
								{
									if(m_tTableChangeOper[m_iChangeCount].m_iColLen >= 14)
									{
										SAFESTRCPY(m_tTableChangeOper[m_iChangeCount].m_sValue, sizeof(m_tTableChangeOper[m_iChangeCount].m_sValue), tTemp[4]);
									}
									else if(m_tTableChangeOper[m_iChangeCount].m_iColLen == sizeof(long long))
									{
										m_tTableChangeOper[m_iChangeCount].m_llValue = atol(tTemp[4]);
									}
									else if(m_tTableChangeOper[m_iChangeCount].m_iColLen == sizeof(int))
									{
										m_tTableChangeOper[m_iChangeCount].m_iValue = atoi(tTemp[4]);
									}
									else
									{
										//error
										return -1;
									}
								}
								else
								{
									SAFESTRCPY(m_tTableChangeOper[m_iChangeCount].m_sValue, sizeof(m_tTableChangeOper[m_iChangeCount].m_sValue), tTemp[4]);
									if(m_tTableChangeOper[m_iChangeCount].m_iColType == DT_VarChar|| m_tTableChangeOper[m_iChangeCount].m_iColType == DT_Blob)
									{
										m_iVarColPos[m_iVarColCount++] = m_tRemoteStruct.m_iColCount;
									}
								}
								iMiss++;
								m_tRemoteStruct.m_iIsLocalCol[m_tRemoteStruct.m_iColCount] = 0;
								m_tRemoteStruct.m_iColPos[m_tRemoteStruct.m_iColCount] = i;
								m_tRemoteStruct.m_iChangePos[m_tRemoteStruct.m_iColCount] = m_iChangeCount;
								m_tRemoteStruct.m_iColCount++;
								
								if((m_iChangeCount++)>=MAX_LOAD_TABLE_COLUMN_DIFF_COUNT)
								{
									TADD_ERROR(-1, "Too many column changed.");
									iRet = -1;
									return iRet;
								}
							}
							else
							{
								TADD_ERROR(-1, "Can't find default value for added column.");
								iRet = -1;
								return iRet;
							}
						}
					}
					
					if(pTable->tColumn[i].iDataType == DT_Char && atoi(tColInfoSplit[1]) == DT_VarChar)
					{
						if(pTable->tColumn[i].iColumnLen <= atoi(tColInfoSplit[2]))
						{
							m_tTableChangeOper[m_iChangeCount].m_iColPos = i;
							m_tTableChangeOper[m_iChangeCount].m_iColType = atoi(tColInfoSplit[1]);
							m_tTableChangeOper[m_iChangeCount].m_iColLen = atoi(tColInfoSplit[2]);
							m_tTableChangeOper[m_iChangeCount].m_iChangeType = TC_ColCharToVar;
							m_iVarColPos[m_iVarColCount++] = m_tRemoteStruct.m_iColCount;
							m_tRemoteStruct.m_iIsLocalCol[m_tRemoteStruct.m_iColCount] = 1;//按照本地列处理NULL标志位
							m_tRemoteStruct.m_iColPos[m_tRemoteStruct.m_iColCount] = i;
							m_tRemoteStruct.m_iChangePos[m_tRemoteStruct.m_iColCount] = m_iChangeCount;
							m_tRemoteStruct.m_iColCount++;
							if((m_iChangeCount++)>=MAX_LOAD_TABLE_COLUMN_DIFF_COUNT)
							{
								TADD_ERROR(-1, "Too many column changed.");
								iRet = -1;
								return iRet;
							}
						}
						else
						{
							TADD_ERROR(-1, "Rep's column [%s] datalen is too short.", pTable->tColumn[i].sName);
							iRet = -1;
							return iRet;
						}
					}
					else if(pTable->tColumn[i].iDataType == DT_VarChar && atoi(tColInfoSplit[1]) == DT_Char)
					{
						m_tTableChangeOper[m_iChangeCount].m_iColPos = i;
						m_tTableChangeOper[m_iChangeCount].m_iColType = atoi(tColInfoSplit[1]);
						m_tTableChangeOper[m_iChangeCount].m_iColLen = atoi(tColInfoSplit[2]);
						m_tTableChangeOper[m_iChangeCount].m_iChangeType = TC_ColVarToChar;
						m_tRemoteStruct.m_iIsLocalCol[m_tRemoteStruct.m_iColCount] = 1;//按照本地列处理NULL标志位
						m_tRemoteStruct.m_iColPos[m_tRemoteStruct.m_iColCount] = i;
						m_tRemoteStruct.m_iChangePos[m_tRemoteStruct.m_iColCount] = m_iChangeCount;
						m_tRemoteStruct.m_iColCount++;
						if((m_iChangeCount++)>=MAX_LOAD_TABLE_COLUMN_DIFF_COUNT)
						{
							TADD_ERROR(-1, "Too many column changed.");
							iRet = -1;
							return iRet;
						}
					}
					else if(pTable->tColumn[i].iDataType != atoi(tColInfoSplit[1]))//列类型不一致
					{
						TADD_ERROR(-1, "Different datatype of column[%s].", pTable->tColumn[i].sName);
						iRet = -1;
						return iRet;
					}
					else if(pTable->tColumn[i].iDataType == DT_DateStamp && pTable->tColumn[i].iColumnLen != atoi(tColInfoSplit[2]))
					{
						if(atoi(tColInfoSplit[2]) >= 14)
						{
							if(pTable->tColumn[i].iColumnLen == sizeof(long long))
							{
								m_tTableChangeOper[m_iChangeCount].m_iChangeType = TC_ColDateLToN;
							}
							else if(pTable->tColumn[i].iColumnLen == sizeof(int))
							{
								m_tTableChangeOper[m_iChangeCount].m_iChangeType = TC_ColDateYToN;
							}
						}
						else if(atoi(tColInfoSplit[2]) == sizeof(long long))
						{
							if(pTable->tColumn[i].iColumnLen == sizeof(int))
							{
								m_tTableChangeOper[m_iChangeCount].m_iChangeType = TC_ColDateYToL;
							}
						}
						else
						{
							TADD_ERROR(-1, "Table [%s] column [%s]'s zip-level is higher.", pTable->sTableName, pTable->tColumn[i].sName);
							iRet = -1;
							return iRet;
						}
						m_tTableChangeOper[m_iChangeCount].m_iColPos = i;
						m_tTableChangeOper[m_iChangeCount].m_iColType = atoi(tColInfoSplit[1]);
						m_tTableChangeOper[m_iChangeCount].m_iColLen = atoi(tColInfoSplit[2]);
						m_tRemoteStruct.m_iIsLocalCol[m_tRemoteStruct.m_iColCount] = 1;//按照本地列处理NULL标志位
						m_tRemoteStruct.m_iColPos[m_tRemoteStruct.m_iColCount] = i;
						m_tRemoteStruct.m_iChangePos[m_tRemoteStruct.m_iColCount] = m_iChangeCount;
						m_tRemoteStruct.m_iColCount++;
						if((m_iChangeCount++)>=MAX_LOAD_TABLE_COLUMN_DIFF_COUNT)
						{
							TADD_ERROR(-1, "Too many column changed.");
							iRet = -1;
							return iRet;
						}
					}
					else if(pTable->tColumn[i].iColumnLen < atoi(tColInfoSplit[2]))//列长度增加
					{
						m_tTableChangeOper[m_iChangeCount].m_iColPos = i;
						m_tTableChangeOper[m_iChangeCount].m_iColType = atoi(tColInfoSplit[1]);
						m_tTableChangeOper[m_iChangeCount].m_iColLen = atoi(tColInfoSplit[2]);
						m_tTableChangeOper[m_iChangeCount].m_iChangeType = TC_ColLenIncrease;

						if(pTable->tColumn[i].iDataType == DT_VarChar || pTable->tColumn[i].iDataType == DT_Blob)
						{
							m_iVarColPos[m_iVarColCount++] = m_tRemoteStruct.m_iColCount;
						}
						
						m_tRemoteStruct.m_iIsLocalCol[m_tRemoteStruct.m_iColCount] = 1;
						m_tRemoteStruct.m_iColPos[m_tRemoteStruct.m_iColCount] = i;
						m_tRemoteStruct.m_iChangePos[m_tRemoteStruct.m_iColCount] = m_iChangeCount;
						m_tRemoteStruct.m_iColCount++;
						
						if((m_iChangeCount++)>=MAX_LOAD_TABLE_COLUMN_DIFF_COUNT)
						{
							TADD_ERROR(-1, "Too many column changed.");
							iRet = -1;
							return iRet;
						}
					}
					else if(pTable->tColumn[i].iColumnLen > atoi(tColInfoSplit[2]))//列长度减少，报错
					{
						TADD_ERROR(-1, "Rep's column [%s] datalen is too short.", pTable->tColumn[i].sName);
						iRet = -1;
						return iRet;
					}
					else
					{
						if(pTable->tColumn[i].iDataType == DT_VarChar || pTable->tColumn[i].iDataType == DT_Blob)
						{
							m_iVarColPos[m_iVarColCount++] = m_tRemoteStruct.m_iColCount;
						}
						
						m_tRemoteStruct.m_iIsLocalCol[m_tRemoteStruct.m_iColCount] = 1;
						m_tRemoteStruct.m_iColPos[m_tRemoteStruct.m_iColCount] = i;
						m_tRemoteStruct.m_iColCount++;
					}
					break;
				}
				//对端删除列
				if(j == tColSplit.GetFieldCount() - 1)
				{
					if(i > iLMaxMatch) iLMaxMatch = i;
					m_tTableChangeOper[m_iChangeCount].m_iColPos = i;
					m_tTableChangeOper[m_iChangeCount].m_iChangeType = TC_ColumnDrop;
					m_bColMissAdd = true;
					iExtra++;
					if((m_iChangeCount++)>=MAX_LOAD_TABLE_COLUMN_DIFF_COUNT)
					{
						TADD_ERROR(-1, "Too many column changed.");
						iRet = -1;
						return iRet;
					}
				}
			}
		}
		
		for(int i = iRMaxMatch+1; i<tColSplit.GetFieldCount(); i++)
		{
			TMdbNtcSplit tTemp;
			tTemp.SplitString(tColSplit[i], ',');
			if(tTemp[3][0] == 'Y' && tTemp.GetFieldCount() == 5)
			{
				m_tTableChangeOper[m_iChangeCount].m_iColPos = iLMaxMatch;
				m_tTableChangeOper[m_iChangeCount].m_iColType = atoi(tTemp[1]);
				m_tTableChangeOper[m_iChangeCount].m_iColLen = atoi(tTemp[2]);
				m_tTableChangeOper[m_iChangeCount].m_iChangeType = TC_ColumnAdd;
				m_bColMissAdd = true;
				if(m_tTableChangeOper[m_iChangeCount].m_iColType == DT_Int)
				{
					m_tTableChangeOper[m_iChangeCount].m_llValue = atol(tTemp[4]);
				}
				else if(m_tTableChangeOper[m_iChangeCount].m_iColType == DT_DateStamp)
				{
					if(m_tTableChangeOper[m_iChangeCount].m_iColLen >= 14)
					{
						SAFESTRCPY(m_tTableChangeOper[m_iChangeCount].m_sValue, sizeof(m_tTableChangeOper[m_iChangeCount].m_sValue), tTemp[4]);
					}
					else if(m_tTableChangeOper[m_iChangeCount].m_iColLen == sizeof(long long))
					{
						m_tTableChangeOper[m_iChangeCount].m_llValue = atol(tTemp[4]);
					}
					else if(m_tTableChangeOper[m_iChangeCount].m_iColLen == sizeof(int))
					{
						m_tTableChangeOper[m_iChangeCount].m_iValue = atoi(tTemp[4]);
					}
					else
					{
						//error
						return -1;
					}
				}
				else
				{
					SAFESTRCPY(m_tTableChangeOper[m_iChangeCount].m_sValue, sizeof(m_tTableChangeOper[m_iChangeCount].m_sValue), tTemp[4]);
					if(m_tTableChangeOper[m_iChangeCount].m_iColType == DT_VarChar|| m_tTableChangeOper[m_iChangeCount].m_iColType == DT_Blob)
					{
						m_iVarColPos[m_iVarColCount++] = m_tRemoteStruct.m_iColCount;
					}
				}
				iMiss++;
				m_tRemoteStruct.m_iIsLocalCol[m_tRemoteStruct.m_iColCount] = 0;
				m_tRemoteStruct.m_iColPos[m_tRemoteStruct.m_iColCount] = iLMaxMatch;
				m_tRemoteStruct.m_iChangePos[m_tRemoteStruct.m_iColCount] = m_iChangeCount;
				m_tRemoteStruct.m_iColCount++;
				
				if((m_iChangeCount++)>=5)
				{
					TADD_ERROR(-1, "Too many column changed.");
					iRet = -1;
					return iRet;
				}
			}
			else
			{
				TADD_ERROR(-1, "Can't find default value for added column.");
				iRet = -1;
				return iRet;
			}
		}
		if(pTable->iOneRecordNullOffset > 0 && m_bColMissAdd == true)
		{
			
			if(m_tRemoteStruct.m_iColCount != pTable->iColumnCounts)
			{
				int iNullSize = CalcNullSize(pTable->iColumnCounts);
		        int iNewNullSize = CalcNullSize(m_tRemoteStruct.m_iColCount);
				if(iNewNullSize != iNullSize)
				{
					m_bNullSizeChange = true;
					m_iNullSizeAdd = iNewNullSize - iNullSize;
				}
			}
		}
		return iRet;
	}

	int TRepServerDataSend::SendMemData(TMdbTable * pTable,  int * iPageIDList, int iPageCount)
	{
		TADD_FUNC("START");
		int iRet = 0;
		int i = 0;
		int iRecordCount = 0;
		while(iPageIDList[i] > 0 && i<iPageCount)
		{
			m_pCurPage = (TMdbPage * )m_tTSCtrl.GetAddrByPageID(iPageIDList[i]);
			if(m_pCurPage == NULL)
			{
				i++;
				continue;
			}
			if(m_pCurPage->m_iRecordCounts > 0)
			{
				//加页写锁，防止读取过程中发生操作
				CHECK_RET(m_tPageCtrl.Attach((char *)m_pCurPage, false, true),"tPageCtrl.Attach() failed.");
				CHECK_RET(m_tPageCtrl.WLock(),"tPageCtrl.WLock() failed.");
				while(true)
				{
					memset(m_sOneRecord, 0, sizeof(m_sOneRecord));
					m_iOneRecordLen = 0;
					iRet = GetOneRecordFromMem(pTable);
					if(iRet == 1)//取出成功
					{
						iRecordCount++;
						if(iRecordCount%10000 == 0)
						{
							TADD_NORMAL("Send table [%s] [%d] records.", pTable->sTableName, iRecordCount);
						}
						if(MAX_REP_SEND_BUF_LEN - m_iSendBufLen >= m_iOneRecordLen)// //如果发送缓冲区空间还够，则拷贝到发送缓冲区
						{
							memcpy(m_sSendBuf+m_iSendBufLen, m_sOneRecord, m_iOneRecordLen);
							m_iSendBufLen += m_iOneRecordLen;
							if(iRecordCount%100 == 0) //达到100条也必须要发送
							{
								if(SendBufData(m_sSendBuf, m_iSendBufLen) < 0){return ERR_NET_SEND_FAILED;}
								m_iSendBufLen = 0;
								m_sSendBuf[0] = '\0';
							}					 
						}
						else //如果缓冲区满了，则先将缓冲区的数据发送出去，再把本次获取的记录放到缓冲区中
						{
							if(SendBufData(m_sSendBuf, m_iSendBufLen) < 0){return ERR_NET_SEND_FAILED;}
							m_iSendBufLen = 0;
							m_sSendBuf[0] = '\0';
							memcpy(m_sSendBuf, m_sOneRecord, m_iOneRecordLen);
							m_iSendBufLen += m_iOneRecordLen;
						}
					}
					else if(iRet == 0)//当前页已处理结束
					{
						break;
					}
					else//取数据出错
					{
						m_tPageCtrl.UnWLock();
						SAFE_DELETE_ARRAY(iPageIDList);
						return iRet;
					}
				}
				m_tPageCtrl.UnWLock();
			}
			i++;
		}
		
		//发送报文尾
		if(MAX_REP_SEND_BUF_LEN - m_iSendBufLen < 20)
		{
			if(SendBufData(m_sSendBuf,m_iSendBufLen) < 0){return ERR_NET_SEND_FAILED;}
			m_sSendBuf[0] = '\0';
			m_iSendBufLen = 0;
		}
		char sEndStr[MAX_NAME_LEN] = {0};
		snprintf(sEndStr, MAX_NAME_LEN, "%s%d%s%s","!!&&.",iRecordCount,".OK", LOAD_DATA_END_FLAG);	
		memcpy(m_sSendBuf+m_iSendBufLen, sEndStr, strlen(sEndStr));
		m_iSendBufLen+=strlen(sEndStr);
		if(SendBufData(m_sSendBuf, m_iSendBufLen) < 0)
		{
			return ERR_NET_SEND_FAILED;
		}
		m_sSendBuf[0] = '\0';
		m_iSendBufLen = 0;
		TADD_FUNC("END");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  DealOneTableMem
	* 函数描述	:  发送一张表的内存数据
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jiang.xiaolong
	*******************************************************************************/
	int TRepServerDataSend::DealOneTableMem(const char* sTableName, const char* sRoutinglist, const char * sRemoteTableInfo)
	{
		TADD_FUNC("Start[%s].", sTableName);
		int iRet = ERROR_SUCCESS;

		// 判断请求的表是否存在
		TMdbTable* pTable =TMdbShmMgr::GetShmDSN(m_pConfig->GetDSN()->sName)->GetTableByName(sTableName);
		if(NULL == pTable)
		{
			memset(m_sSendBuf,0,MAX_SEND_BUF);
			snprintf(m_sSendBuf,sizeof(m_sSendBuf),"%s",REP_TABLE_NO_EXIST); 
			if(SendBufData(m_sSendBuf, (strlen(m_sSendBuf))) < 0){return ERR_NET_SEND_FAILED;}
			TADD_WARNING("Uploaded on table[%s] does not exist.",sTableName);
			return iRet;
		}

		CHECK_RET(ParseRoutingList(sRoutinglist), "CheckRoutingList [%s] failed.", sRoutinglist);//解析加载路由
		CHECK_RET(CheckRemoteTableInfo(pTable, sRemoteTableInfo), "CheckRemoteTableInfo [%s] failed.", sRemoteTableInfo);//校验两端表结构信息
		
		if(m_iRouteCount == 0) TADD_NORMAL("Loading table[%s] by default routing.", sTableName);
			
		CHECK_RET(m_tTSCtrl.Init(m_tDsn->sName, pTable->m_sTableSpace), "m_tTSCtrl.Init failed.");
		CHECK_RET(m_tRowCtrl.Init(m_tDsn->sName, pTable), "m_tRowCtrl.Init failed.");
		
		//拼接报文头
		memset(m_sSendBuf,0,MAX_REP_SEND_BUF_LEN);
		m_iSendBufLen = 0;
		snprintf(m_sSendBuf,MAX_REP_SEND_BUF_LEN, "@@!!%s", sTableName);
		m_iSendBufLen += strlen(m_sSendBuf);
		
		m_pCurPage = NULL;
		m_pCurDataAddr = NULL;	
		m_pNextDataAddr = NULL;
		m_iDataOffset = 0;
		int * iPageIDList = NULL;
		int iPageCount = 0;
		//截取当前free链和full链的快照
		CHECK_RET(GetChainSnapshot(pTable, iPageIDList, iPageCount), "GetChainSnapshot failed.");
		CHECK_RET(SendMemData(pTable, iPageIDList, iPageCount), "SendMemData failed.");
		SAFE_DELETE_ARRAY(iPageIDList);
		TADD_FUNC("Finish.");
		return iRet;
	}


    /******************************************************************************
    * 函数名称	:  GetOneRecord
    * 函数描述	:  获取一条记录
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    void TRepServerDataSend::GetOneRecord()
    {
        int iCount = m_pCurQuery->FieldCount();
        for(int j=0; j<iCount; j++)
        {
            if (m_pCurQuery->Field(j).isNULL())
            {
                sprintf(m_sOneRecord+strlen(m_sOneRecord),",@(nil)");
            }
            else
            {
                sprintf(m_sOneRecord+strlen(m_sOneRecord),",@%s",m_pCurQuery->Field(j).AsString());
            }
        }
    }

	bool TRepServerDataSend::CheckRecordIsCommit(const char * pDataAddr)
	{
		TMdbPageNode* pNode = (TMdbPageNode*)pDataAddr - 1;
		if(pNode->iSessionID == 0) return true;

		TADD_NORMAL("Data %p is not Commit.  Skip",pDataAddr);

		return false;
	}

	bool TRepServerDataSend::CheckRecordRoutingId(TMdbTable * pTable, const char * pDataAddr)
	{
		//暂时只支持整型路由
		if(m_iRouteCount == 0) return true;
		CHECK_OBJ(pTable);
		TMdbColumn * pColumn = pTable->GetColumnByName(m_pConfig->GetDSN()->sRoutingName);
		if(pColumn->iDataType != DT_Int)
		{
			return false;
		}
		else
		{
			long long iValue = *(long long *)(pDataAddr+pColumn->iOffSet);
			for(int i = 0; i<m_iRouteCount; i++)
			{
				if(m_iRouteList[i] == iValue) return true;
			}
		}
		
		return false;
	}

	int TRepServerDataSend::CalcNullSize(int iColCounts)
	{
		int iCount = iColCounts/MDB_CHAR_SIZE;
        if(0 != iColCounts % MDB_CHAR_SIZE )
        {
            iCount ++;
        }
        return iCount;
	}

	int TRepServerDataSend::AdjustNullArea(char * sRecord,int & iLen,TMdbTable * pTable)
	{
		TADD_FUNC("START");
		int iRet = 0;
		char sMask[] = {128,64,32,16,8,4,2,1};
		int i = 0;
		
		if(pTable->iOneRecordNullOffset > 0 && m_bColMissAdd)
		{
			for(i = 0; i<m_tRemoteStruct.m_iColCount; i++)
			{
				if(m_tRemoteStruct.m_iIsLocalCol[i] == 1)
				{
					if(m_tRowCtrl.IsColumnNull(&(pTable->tColumn[m_tRemoteStruct.m_iColPos[i]]), sRecord))
					{
				        char * sNull = m_sNullFlag + i/MDB_CHAR_SIZE;
				        *sNull |=sMask[i%MDB_CHAR_SIZE];
					}
					else
					{
						char * sNull = m_sNullFlag + i/MDB_CHAR_SIZE;
				        *sNull &=sMask[i%MDB_CHAR_SIZE];
					}
				}
				else if(m_tRemoteStruct.m_iIsLocalCol[i] == 0)
				{
					char * sNull = m_sNullFlag + i/MDB_CHAR_SIZE;
				    *sNull &=sMask[i%MDB_CHAR_SIZE];
				}
				else
				{
					//ERROR
					TADD_ERROR(-1, "Invalid column source.");
					return -1;
				}
			}
			memcpy(sRecord+pTable->iOneRecordNullOffset, m_sNullFlag, CalcNullSize(m_tRemoteStruct.m_iColCount));
			if(m_bNullSizeChange)
			{
				iLen += m_iNullSizeAdd;
			}
		}
		TADD_FUNC("END");
		return iRet;
	}

	int TRepServerDataSend::AdjustMemRecord(const char* pAddr, char * sRecord, int & iLen, TMdbTable * pTable)
	{
		TADD_FUNC("START");
		int iRet = 0;
				
		if(m_iChangeCount <= 0) return iRet;
		CHECK_RET(AdjustNullArea(sRecord,iLen,pTable), "AdjustNullArea failed.");
		for(int j = m_iChangeCount-1; j >= 0; j--)
		{
			if(m_tTableChangeOper[j].m_iChangeType == TC_ColumnDrop)
			{
				int iOffset1 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos].iOffSet;
				if(m_tTableChangeOper[j].m_iColPos == pTable->iColumnCounts-1)
				{
					iLen += iOffset1 - pTable->m_iTimeStampOffset;
				}
				else
				{
					int iOffset2 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos+1].iOffSet;
					memmove(sRecord+iOffset1, sRecord+iOffset2, iLen - iOffset2);
					iLen += (iOffset1 - iOffset2);
				}
			}
			else if(m_tTableChangeOper[j].m_iChangeType == TC_ColumnAdd)
			{
				int iOffset1 = 0;
				if(m_tTableChangeOper[j].m_iColPos == pTable->iColumnCounts)
				{
					iOffset1 = pTable->m_iTimeStampOffset;
				}
				else
				{
					iOffset1 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos].iOffSet;
				}
				
				if(m_tTableChangeOper[j].m_iColType == DT_VarChar || m_tTableChangeOper[j].m_iColType == DT_Blob)
				{
					memmove(sRecord+iOffset1+1+sizeof(long)*2, sRecord+iOffset1, iLen - iOffset1);
					iLen += sizeof(long)*2 + 1;
				}
				else if(m_tTableChangeOper[j].m_iColType == DT_DateStamp)
				{
					memmove(sRecord+iOffset1+m_tTableChangeOper[j].m_iColLen, sRecord+iOffset1, iLen - iOffset1);
					if(m_tTableChangeOper[j].m_iColLen == sizeof(int))
                    {
                        int * pInt = (int*)(sRecord+iOffset1);
		                *pInt = m_tTableChangeOper[j].m_iValue;
                    }
                    else if(m_tTableChangeOper[j].m_iColLen == sizeof(long long))
                    {
                        long long * pLonglong = (long long*)(sRecord+iOffset1);
		                * pLonglong = m_tTableChangeOper[j].m_llValue;
                    }
                    else if(m_tTableChangeOper[j].m_iColLen >= 14)
                    {
                        strncpy(sRecord+iOffset1, m_tTableChangeOper[j].m_sValue, m_tTableChangeOper[j].m_iColLen);
                    }
					iLen += m_tTableChangeOper[j].m_iColLen;
				}
				else if(m_tTableChangeOper[j].m_iColType == DT_Int)
				{
					memmove(sRecord+iOffset1+m_tTableChangeOper[j].m_iColLen, sRecord+iOffset1, iLen - iOffset1);
					long long * pLonglong = (long long*)(sRecord+iOffset1);
		            * pLonglong = m_tTableChangeOper[j].m_llValue;
					iLen += m_tTableChangeOper[j].m_iColLen;
				}
				else
				{
					memmove(sRecord+iOffset1+m_tTableChangeOper[j].m_iColLen, sRecord+iOffset1, iLen - iOffset1);
					strncpy(sRecord+iOffset1, m_tTableChangeOper[j].m_sValue, m_tTableChangeOper[j].m_iColLen);
					iLen += m_tTableChangeOper[j].m_iColLen;
				}
			}
			else if(m_tTableChangeOper[j].m_iChangeType == TC_ColLenIncrease)
			{
				int iOffset1 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos].iOffSet;
				int iOffset2 = 0;
				if(m_tTableChangeOper[j].m_iColPos != pTable->iColumnCounts-1)
				{
					iOffset2 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos+1].iOffSet;
				}
				else
				{
					iOffset2 = pTable->m_iTimeStampOffset;
				}
				memmove(sRecord+iOffset1+m_tTableChangeOper[j].m_iColLen, sRecord+iOffset2, iLen - iOffset2);
				iLen += iOffset1+m_tTableChangeOper[j].m_iColLen-iOffset2;
			}
			else if(m_tTableChangeOper[j].m_iChangeType == TC_ColCharToVar)
			{
				int iOffset1 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos].iOffSet;
				int iOffset2 = 0;
				if(m_tTableChangeOper[j].m_iColPos != pTable->iColumnCounts-1)
				{
					iOffset2 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos+1].iOffSet;
				}
				else
				{
					iOffset2 = pTable->m_iTimeStampOffset;
				}
				memmove(sRecord+iOffset1+1+sizeof(long)*2, sRecord+iOffset2, iLen - iOffset2);
				iLen += iOffset1+1+sizeof(long)*2-iOffset2;
			}
			else if(m_tTableChangeOper[j].m_iChangeType == TC_ColVarToChar)
			{
				int iOffset1 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos].iOffSet;
				int iOffset2 = 0;
				if(m_tTableChangeOper[j].m_iColPos != pTable->iColumnCounts-1)
				{
					iOffset2 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos+1].iOffSet;
				}
				else
				{
					iOffset2 = pTable->m_iTimeStampOffset;
				}
				memmove(sRecord+iOffset1+m_tTableChangeOper[j].m_iColLen, sRecord+iOffset2, iLen - iOffset2);
				iLen += iOffset1+m_tTableChangeOper[j].m_iColLen-iOffset2;
				if(m_tRowCtrl.IsColumnNull(&pTable->tColumn[m_tTableChangeOper[j].m_iColPos],m_pCurDataAddr) != true)
				{
					m_tVarcharCtrl.GetStoragePos(m_pCurDataAddr + iOffset1, m_iWhichPos, m_iRowId);
					if(m_iWhichPos < VC_16 || m_iWhichPos > VC_8192)
					{
						//TADD_ERROR();
						return -1;
					}
					CHECK_RET(m_tVarcharCtrl.GetVarcharValue(sRecord+iOffset1, m_pCurDataAddr + iOffset1, m_iValueSize),"Get Varchar value Faild");
					sRecord[iOffset1+m_iValueSize] = '\0';
				}
			}
			else if(m_tTableChangeOper[j].m_iChangeType == TC_ColDateLToN)
			{
				int iOffset1 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos].iOffSet;
				long long lLong = 0;
				bool bNull = m_tRowCtrl.IsColumnNull(&pTable->tColumn[m_tTableChangeOper[j].m_iColPos],m_pCurDataAddr);
				if(!bNull)
				{
					lLong = (long long)*(long long *)(sRecord+iOffset1);
				}
				int iOffset2 = 0;
				if(m_tTableChangeOper[j].m_iColPos != pTable->iColumnCounts-1)
				{
					iOffset2 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos+1].iOffSet;
				}
				else
				{
					iOffset2 = pTable->m_iTimeStampOffset;
				}
				memmove(sRecord+iOffset1+m_tTableChangeOper[j].m_iColLen, sRecord+iOffset2, iLen - iOffset2);
				iLen += iOffset1+m_tTableChangeOper[j].m_iColLen-iOffset2;
				if(!bNull)
				{
            		TMdbDateTime::TimeToString(lLong,sRecord+iOffset1);
				}
			}
			else if(m_tTableChangeOper[j].m_iChangeType == TC_ColDateYToL)
			{
				int iOffset1 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos].iOffSet;
				int iInt = 0;
				bool bNull = m_tRowCtrl.IsColumnNull(&pTable->tColumn[m_tTableChangeOper[j].m_iColPos],m_pCurDataAddr);
				if(!bNull)
				{
					iInt = (int)*(int*)(sRecord+iOffset1);
				}
				int iOffset2 = 0;
				if(m_tTableChangeOper[j].m_iColPos != pTable->iColumnCounts-1)
				{
					iOffset2 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos+1].iOffSet;
				}
				else
				{
					iOffset2 = pTable->m_iTimeStampOffset;
				}
				memmove(sRecord+iOffset1+m_tTableChangeOper[j].m_iColLen, sRecord+iOffset2, iLen - iOffset2);
				iLen += iOffset1+m_tTableChangeOper[j].m_iColLen-iOffset2;
				if(!bNull)
				{
            		long long * pLong = (long long*)(sRecord+iOffset1);
					*pLong = iInt;
				}
			}
			else if(m_tTableChangeOper[j].m_iChangeType == TC_ColDateYToN)
			{
				int iOffset1 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos].iOffSet;
				int iInt = 0;
				bool bNull = m_tRowCtrl.IsColumnNull(&pTable->tColumn[m_tTableChangeOper[j].m_iColPos],m_pCurDataAddr);
				if(!bNull)
				{
					iInt = (int)*(int*)(sRecord+iOffset1);
				}
				int iOffset2 = 0;
				if(m_tTableChangeOper[j].m_iColPos != pTable->iColumnCounts-1)
				{
					iOffset2 = pTable->tColumn[m_tTableChangeOper[j].m_iColPos+1].iOffSet;
				}
				else
				{
					iOffset2 = pTable->m_iTimeStampOffset;
				}
				memmove(sRecord+iOffset1+m_tTableChangeOper[j].m_iColLen, sRecord+iOffset2, iLen - iOffset2);
				iLen += iOffset1+m_tTableChangeOper[j].m_iColLen-iOffset2;
				if(!bNull)
				{
            		TMdbDateTime::TimeToString(iInt,sRecord+iOffset1);
				}
			}
			else
			{
				TADD_ERROR(ERROR_UNKNOWN, "Invalid table struct change.");
				return ERROR_UNKNOWN;
			}
		}
		TADD_FUNC("END");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  GetOneRecordFromMem
	* 函数描述	:  直接从表的内存页中获取一条记录
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 当前页处理完毕，1 - 当前记录获取成功，!0 -失败
	* 作者		:  jiang.xiaolong
	*******************************************************************************/

	int TRepServerDataSend::GetOneRecordFromMem(TMdbTable * pTable)
	{
		int iRet = 0;
		CHECK_OBJ(m_tDsn);
			
		if(NULL == m_pCurPage)
		{
			iRet = -1;
		}
		if(m_pCurPage->m_iRecordCounts == 0)
		{
			iRet = 0;
		}
		else
		{
			while(m_pCurPage->GetNextDataAddr(m_pCurDataAddr, m_iDataOffset, m_pNextDataAddr) != NULL)
			{
				if(!CheckRecordIsCommit(m_pCurDataAddr)) continue;
				if(!CheckRecordRoutingId(pTable,m_pCurDataAddr)) continue;//校验路由失败则取下一条继续
				snprintf(m_sOneRecord, MAX_VALUE_LEN, "%s", "!!,");//记录分割
				m_iOneRecordLen+=3;
				m_iOneRecordLen+=6;//用于存储记录总长度，对端接收到数据后进行校验
				char * sRecord = m_sOneRecord+m_iOneRecordLen;
				int iLen = pTable->iOneRecordSize;
				memcpy(m_sOneRecord+m_iOneRecordLen, m_pCurDataAddr, pTable->iOneRecordSize);
				m_iOneRecordLen += pTable->iOneRecordSize;
				// 根据表结构差异调整内存记录
				CHECK_RET(AdjustMemRecord(m_pCurDataAddr, sRecord, iLen, pTable), "MemRecordAdjust failed.");
				m_iOneRecordLen += iLen-pTable->iOneRecordSize;
				
				for(int i = 0; i < m_iVarColCount; i++)
				{
					m_iWhichPos = -1;
					m_iRowId = 0;
					m_iValueSize = 0;
					if(m_tRemoteStruct.m_iIsLocalCol[m_iVarColPos[i]] == 0)
					{
						m_sOneRecord[m_iOneRecordLen] = ',';
						m_sOneRecord[m_iOneRecordLen+1] = '@';
						char * pValue = m_tTableChangeOper[m_tRemoteStruct.m_iChangePos[m_iVarColPos[i]]].m_sValue;
						m_iValueSize = strlen(pValue);
						memcpy(m_sOneRecord+m_iOneRecordLen+6, pValue, m_iValueSize);
					}
					else
					{
						TMdbColumn * pCol = &pTable->tColumn[m_tRemoteStruct.m_iColPos[m_iVarColPos[i]]];
						if(m_tRowCtrl.IsColumnNull(pCol,m_pCurDataAddr) != true)
						{
							m_sOneRecord[m_iOneRecordLen] = ',';
							m_sOneRecord[m_iOneRecordLen+1] = '@';
							if(m_tTableChangeOper[m_tRemoteStruct.m_iChangePos[m_iVarColPos[i]]].m_iChangeType == TC_ColCharToVar)
							{
								m_iValueSize = strlen(m_pCurDataAddr+pCol->iOffSet);
								if(m_iValueSize > pCol->iColumnLen)
								{
									return -1;
								}
								SAFESTRCPY(m_sOneRecord+m_iOneRecordLen+6,m_iValueSize+1,m_pCurDataAddr+pCol->iOffSet);
							}
							else
							{
								m_tVarcharCtrl.GetStoragePos(m_pCurDataAddr + pCol->iOffSet, m_iWhichPos, m_iRowId);
								if(m_iWhichPos < VC_16 || m_iWhichPos > VC_8192)
								{
										return -1;
								}
								CHECK_RET(m_tVarcharCtrl.GetVarcharValue(m_sOneRecord+m_iOneRecordLen+6, m_pCurDataAddr + pCol->iOffSet, m_iValueSize),"Get Varchar value Faild");
							}
						}
						else
						{
							continue;//NULL就不进行设置了，等到了对端对NULL标志位进行解析判断
						}
					}
					m_sOneRecord[m_iOneRecordLen+2] = m_iValueSize/1000 + '0';
					m_sOneRecord[m_iOneRecordLen+3] = (m_iValueSize%1000)/100 + '0';
					m_sOneRecord[m_iOneRecordLen+4] = (m_iValueSize%100)/10 + '0';
					m_sOneRecord[m_iOneRecordLen+5] = m_iValueSize%10 + '0';
					m_iOneRecordLen += m_iValueSize+6;
				}

				m_sOneRecord[3] = m_iOneRecordLen/100000 + '0';
				m_sOneRecord[4] = (m_iOneRecordLen%100000)/10000 + '0';
				m_sOneRecord[5] = (m_iOneRecordLen%10000)/1000 + '0';
				m_sOneRecord[6] = (m_iOneRecordLen%1000)/100 + '0';
				m_sOneRecord[7] = (m_iOneRecordLen%100)/10 + '0';
				m_sOneRecord[8] = m_iOneRecordLen%10 + '0';
				iRet = 1;
				break;//每次处理一条
			}
			if(m_pCurDataAddr == NULL)	iRet = 0;//页内数据处理结束
		}

		return iRet;
	}

    /******************************************************************************
    * 函数名称	:  SendBufData
    * 函数描述	:  将缓冲区中的数据发送至对端
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepServerDataSend::SendBufData(const char* sBuf, int iLength)
    {
        int iRet = 0;
        TADD_FUNC("sBuf = %s", sBuf);
        if (!m_pPeerInfo->PostMessage(sBuf, iLength))
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send buffer data to Rep failed..");
        }
        return iRet;
    }

    TRepServer::TRepServer():m_pRepConfig(NULL), m_pRepDataServer(NULL), m_pShmMgr(NULL), m_pShmRep(NULL)
    {	

    }

    TRepServer::~TRepServer()
    {
        SAFE_DELETE(m_pShmMgr);
        SAFE_DELETE(m_pRepConfig);
        m_pShmRep = NULL;
        SAFE_DELETE(m_pRepDataServer);
    }

    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	:  初始化
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepServer::Init(const char *sDsn)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_pShmMgr = new(std::nothrow) TMdbShmRepMgr(sDsn);
        CHECK_OBJ(m_pShmMgr);
        CHECK_RET(m_pShmMgr->Attach(),"Attach to shared memory failed.");
        m_pShmRep = m_pShmMgr->GetRoutingRep();

        m_pRepConfig = new(std::nothrow) TMdbRepConfig();
        CHECK_OBJ(m_pRepConfig);
        CHECK_RET(m_pRepConfig->Init(sDsn), "TMdbRepConfig failed.");

        m_pRepDataServer = new(std::nothrow) TMdbRepDataServer(sDsn);
        CHECK_OBJ(m_pRepDataServer);

        m_tProcCtrl.Init(sDsn);

        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  Start
    * 函数描述	:  运行
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TRepServer::Start()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if (!m_pRepDataServer->Start(m_pRepConfig->m_sLocalIP.c_str(), m_pRepConfig->m_iLocalPort, m_pShmRep->m_iRepHostCount))
        {
            CHECK_RET(ERR_NET_NTC_START, "RepServer start failed.");
        }

        while(true)
        {
            if(m_tProcCtrl.IsCurProcStop())
            {
                TADD_NORMAL("mdbRepServer stop.");
                break;
            }
            m_tProcCtrl.UpdateProcHeart(0);

            TMdbNtcDateTime::Sleep(1000);
        }

        TADD_FUNC("Finish.");
        return iRet;
    }


//}
