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
        m_psMsgBuf = new char[m_iMsgBufLen];
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
        if (NULL == strIP || /*QuickMDB::*/TMdbNtcStrFunc::StrNoCaseCmp(strIP, m_strIP.c_str()) != 0 || iPort != m_iPort)
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
    int TRepServerDataSend::SendData(int iHostID, const char* sRoutinglist, TMdbPeerInfo* pPeerInfo)
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
                CHECK_RET(DealOneTable(strTableName.c_str(),sRoutinglist), "Deal table [%s] failed.", strTableName.c_str());            
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

    int TRepServerDataSend::SendData(const char* sTableName, const char* sRoutinglist, TMdbPeerInfo* pPeerInfo)
    {
        TADD_NORMAL("Start. Routinglist = [%s], TableName = [ %s ]", sRoutinglist==NULL?"NULL":sRoutinglist, sTableName);
        int iRet = 0;

        CHECK_OBJ(pPeerInfo);
        m_pPeerInfo = pPeerInfo;
         CHECK_RET(DealOneTable(sTableName, sRoutinglist), "Deal table [%s] failed.", sTableName);    
         //sprintf(m_sSendBuf,"%s",LOAD_DATA_END_FLAG); 
         //if(SendBufData(m_sSendBuf, strlen(m_sSendBuf)) < 0)
         //{
         //    return ERR_NET_SEND_FAILED;
         //}	        

        //char sTableSql[] = "SELECT TABLE_NAME FROM DBA_TABLES WHERE shard_backup = 'Y' ";
        //std::string strTableName;

        //try
        //{
        //    TMdbQuery *pTableQuery = m_pDataBase->CreateDBQuery();
        //    CHECK_OBJ(pTableQuery);
        //    pTableQuery->SetSQL(sTableSql);
        //    pTableQuery->Open();
        //    while(pTableQuery->Next())//逐一获取各个表的数据
        //    {
        //        strTableName = pTableQuery->Field(0).AsString();           
        //        CHECK_RET(DealOneTable(strTableName.c_str(), sRoutinglist), "Deal table [%s] failed.", strTableName.c_str());            
        //    }
        //    //所有的数据发送完，需要发送一个结束标示
        //    //memset(m_sSendBuf,0, MAX_REP_SEND_BUF_LEN);
        //    sprintf(m_sSendBuf,"%s",LOAD_DATA_END_FLAG); 
        //    if(SendBufData(m_sSendBuf, strlen(m_sSendBuf)) < 0){return ERR_NET_SEND_FAILED;}	        
        //}
        //catch(TMdbException& e)
        //{
        //    TADD_ERROR(-1,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        //    iRet = -1;
        //}
        //catch(...)
        //{
        //    TADD_ERROR(-1,"Unknown error!\n");   
        //    iRet = -1;    
        //} 

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

        /*QuickMDB::*/TMdbNtcFileScanner tFileScanner;
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
            /*QuickMDB::*/TMdbNtcFileOper::Remove(pFileName);
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

            /*QuickMDB::*/TMdbNtcDateTime::Sleep(1000);
        }

        TADD_FUNC("Finish.");
        return iRet;
    }


//}
