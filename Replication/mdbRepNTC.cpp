/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbRepNTC.cpp	
*@Description: 封装使用NTC通信的相关类
*@Author:		jiang.lili
*@Date：	    2014/05/4
*@History:
******************************************************************************************/
#include "Replication/mdbRepNTC.h"
#include "Replication/mdbRepServerCtrl.h"
//#include "Replication/mdbRepLoadData.h"
//namespace QuickMDB
//{
    TMdbRepNTCClient::TMdbRepNTCClient():m_eState(E_REP_NTC_CLOSED), m_iHeartbeat(10), m_pPeerInfo(NULL), m_pTcpHelper(NULL), m_pMsgEvent(NULL),  m_iTimeOut(3000)
    {

    }

    TMdbRepNTCClient::~TMdbRepNTCClient()
    {
        //SAFE_DELETE(m_pTcpHelper);

        if (m_pMsgEvent!=NULL)
        {
            m_pMsgEvent->Release();
        }
    }

    /******************************************************************************
    * 函数名称	:  Connect
    * 函数描述	: 连接服务端
    * 输入		:  pszIP 对端的IP
    * 输入		:  iPort 对端的IP
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbRepNTCClient::Connect(const char* pszIP, int iPort,  int iTimeOut /* = 3000 */)
    {
        TADD_FUNC("Start.");
        bool bRet = true;
        if (pszIP == NULL || pszIP[0] == '\0' || iPort <= 0)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM, "Invalid IP[%s] or Port[%d]", pszIP, iPort);
            return false;
        }
        m_iTimeOut = iTimeOut;

        if(!/*QuickMDB::*/TMdbSyncEngine::IsStart()) 
        {
            /*QuickMDB::*/TMdbSyncEngine::Start();
        }

        m_pPeerInfo = /*QuickMDB::*/TMdbSyncEngine::Connect(pszIP, iPort, new /*QuickMDB::*/TMdbWinntTcpProtocol(), m_iTimeOut);//三秒超时
        if (m_pPeerInfo != NULL)
        {
            TADD_NORMAL("Connect to Server(%s:%d) OK.", pszIP, iPort);
            m_pTcpHelper = new(std::nothrow)/*QuickMDB::*/TMdbWinntTcpHelper(m_pPeerInfo);
            if (m_pTcpHelper == NULL)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY, "Get TMdbWinntTcpHelper failed.");
                bRet = false;
            }    
            m_pPeerInfo->SetHelper(m_pTcpHelper);
            m_eState = E_REP_NTC_OK;
        }
        else
        {
            TADD_WARNING_NO_SCREEN("Connect to Server(%s:%d) failed.", pszIP, iPort);
            bRet = false;
        }

        TADD_FUNC("Finish");
        return bRet;
    }

    /******************************************************************************
    * 函数名称	:  SendPacket
    * 函数描述	: 向服务端发送消息
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbRepNTCClient::SendPacket(eRepNtcEvent eEvent, const char* psBodyBuffer, int iLength/*=-1*/)
    {
        TADD_FUNC("Start.");
        if (m_eState == E_REP_NTC_CLOSED || NULL == m_pTcpHelper)//未与服务端连接
        {
            return false;
        }

        bool bRet = true;
        if (iLength < 0)
        {
            iLength = psBodyBuffer!=NULL? strlen(psBodyBuffer): 0;
        }

        bRet = m_pTcpHelper->SendPacket(/*QuickMDB::*/MDB_ASYN_EVENT, eEvent, psBodyBuffer, iLength);
        TADD_FUNC("Finish");
        return bRet;
    }

    /******************************************************************************
    * 函数名称	:  SendHearbeat
    * 函数描述	: 向服务端发送心跳包
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/   
    bool TMdbRepNTCClient::SendHearbeat()
    {
        return m_pTcpHelper->SendCheckPacket();
    }

    /******************************************************************************
    * 函数名称	:  GetMsg
    * 函数描述	: 从服务端获取消息
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepNTCClient::GetMsg(TRepNTCMsg& tMsg)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if (m_pMsgEvent!=NULL)
        {
            m_pMsgEvent->Release();
        }

        m_pMsgEvent = /*QuickMDB::*/TMdbSyncEngine::GetMessage(m_iTimeOut);
        if (m_pMsgEvent !=NULL)
        {
            tMsg.eEvent = (eRepNtcEvent)static_cast<TMdbWinntTcpMsg*>(m_pMsgEvent->pMsgInfo)->oMsgHead.event;
            tMsg.iMsgLen = m_pMsgEvent->pMsgInfo->GetBodyLength();
            tMsg.psMsg = m_pMsgEvent->pMsgInfo->GetBody();         
            TADD_FUNC("Event = %d, Len = %d, Msg = %s", tMsg.eEvent, tMsg.iMsgLen, tMsg.psMsg);
        }
        else
        {
            iRet = ERR_NET_RECV_FAILED;
        }

        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  Disconnect
    * 函数描述	: 断开与服务端的连接
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    void TMdbRepNTCClient::Disconnect()
    {
        TADD_FUNC("Start.");
        if (m_pPeerInfo!=NULL)
        {
            /*QuickMDB::*/TMdbSyncEngine::Disconnect(m_pPeerInfo->pno);
        }
        
        m_eState = E_REP_NTC_CLOSED;
        TADD_FUNC("Finish");
    }

    /******************************************************************************
    * 函数名称	:  NeedReconnect
    * 函数描述	: 是否需要重新连接
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbRepNTCClient::NeedReconnect()
    {
        /*
        if(m_eState == E_REP_NTC_CLOSED)
        {
        time_t tCurTime;
        time(&tCurTime);
        if(tCurTime - m_tConnStateTime > 10)
        {
        time(&m_tConnStateTime);
        return true;
        }
        }
        return false;
        */

        if( m_pPeerInfo==NULL || m_pPeerInfo->IsShutdown() )
        {
            m_eState = E_REP_NTC_CLOSED;
            return true;
        }
        else
        {
            return false;
        }

    }

    TMdbRepMonitorClient::TMdbRepMonitorClient()
    {
        memset(m_sSvrMsg, 0, MAX_REP_SEND_MSG_LEN);
    }

    TMdbRepMonitorClient::~TMdbRepMonitorClient()
    {
    }
    /******************************************************************************
    * 函数名称	:  Register
    * 函数描述	:  向配置服务注册
    * 输入		:  
    * 输出		:  iHostID 本机在配置服务端对应的host_ID
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepMonitorClient::Register(const char* sIP, int iPort, int& iHostID, int& iHeartbeat)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sMsgBuf[MAX_REP_SEND_MSG_LEN] = {0};
        TADD_NORMAL("ip=[%s],port=[%d]", sIP, iPort);
        snprintf(sMsgBuf, MAX_REP_SEND_MSG_LEN, "%s|%d", sIP, iPort);
        if (SendPacket(E_MDB_REGIST, sMsgBuf))
        {
            TADD_NORMAL("Send register Msg OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED,"Send register Msg failed.");
        }
        TRepNTCMsg tMsg;
        if (GetMsg(tMsg) == ERROR_SUCCESS)
        {
            if (tMsg.eEvent != E_MDB_REGIST_ANS || tMsg.iMsgLen<=0)
            {
                CHECK_RET(ERR_NET_RECV_DATA_FORMAT,"Get invalid register answer from server, [%s]", tMsg.ToString());
            }
            else
            {
                /*QuickMDB::*/TMdbNtcSplit tSplit;
                tSplit.SplitString(tMsg.psMsg, '|');
                iHostID = atoi(tSplit[0]);
                iHeartbeat = atoi(tSplit[1]);
                if (iHostID < 0 || iHeartbeat <0)
                {
                    CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "Invalid HostID = %s or Heartbeat interval = %s", tSplit[0], tSplit[1]);
                }

                m_iHostID = iHostID;
                m_iHeartbeat = iHeartbeat;
                m_pPeerInfo->SetPeerTimeout(MDB_NTC_DATA_IDLE_TIMEOUT, m_iHeartbeat);//设置自动发送心跳包
            }
        }
        else
        {
            CHECK_RET(ERR_NET_RECV_FAILED,"Get register answer Msg failed.");
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  ReportState
    * 函数描述	:  向配置服务请求路由信息
    * 输入		:  iHostID 本机在配置服务端对应的Host_ID
    * 输入		:  eState 本机的状态
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int  TMdbRepMonitorClient::ReportState(int iHostID, EMdbRepState eState)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sMsg[MAX_REP_SEND_MSG_LEN];
        snprintf(sMsg, MAX_REP_SEND_MSG_LEN, "%d|%d",iHostID, eState);
        if (SendPacket(E_MDB_STAT_CHANGE, sMsg))
        {
            TADD_NORMAL("Report State to server OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Report State to server failed.");
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    const char* TMdbRepMonitorClient::GetSvrConfigMsg()
    {
        return m_sSvrMsg;
    }

    /******************************************************************************
    * 函数名称	:  RoutingRequest
    * 函数描述	:  向配置服务请求路由信息
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepMonitorClient::RoutingRequest(int iHostID, TMdbShmRepMgr *pShmRepMgr)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sMsg[MAX_REP_SEND_MSG_LEN];
        snprintf(sMsg, MAX_REP_SEND_MSG_LEN, "%d",iHostID);
        if (SendPacket(E_MDB_ROUTING_REQUEST, sMsg))
        {
            TADD_NORMAL("Send routing request to server OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send routing request to server failed.");
        }

        TRepNTCMsg tMsg;
        if (GetMsg(tMsg)==ERROR_SUCCESS)
        {
            if (tMsg.eEvent != E_MDB_ROUTING_REQUEST_ANS || tMsg.iMsgLen<=0)
            {
                CHECK_RET(ERR_NET_RECV_DATA_FORMAT,"Get invalid routing request answer from server, [%s]", tMsg.ToString());
            }
            else
            {
                CHECK_RET(pShmRepMgr->WriteRoutingInfo(tMsg.psMsg, tMsg.iMsgLen),"WriteRoutingInfo failed");
                snprintf(m_sSvrMsg, MAX_REP_SEND_BUF_LEN, "%s|%d|%d", tMsg.psMsg, m_iHostID, m_iHeartbeat);
            }
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbRepMonitorClient::RoutingRequestNoShm(int iHostID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sMsg[MAX_REP_SEND_MSG_LEN];
        snprintf(sMsg, MAX_REP_SEND_MSG_LEN, "%d",iHostID);
        if (SendPacket(E_MDB_ROUTING_REQUEST, sMsg))
        {
            TADD_NORMAL("Send routing request to server OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send routing request to server failed.");
        }

        TRepNTCMsg tMsg;
        if (GetMsg(tMsg)==ERROR_SUCCESS)
        {
            if (tMsg.eEvent != E_MDB_ROUTING_REQUEST_ANS || tMsg.iMsgLen<=0)
            {
                CHECK_RET(ERR_NET_RECV_DATA_FORMAT,"Get invalid routing request answer from server, [%s]", tMsg.ToString());
            }
            else
            {
                snprintf(m_sSvrMsg, MAX_REP_SEND_BUF_LEN, "%s|%d|%d", tMsg.psMsg, m_iHostID, m_iHeartbeat);
            }
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    
    TMdbRepDataClient::TMdbRepDataClient():m_pPeerInfo(NULL),m_pMsgEvent(NULL),m_pLoadDataTool(NULL), m_iRepHostID(MDB_REP_EMPTY_HOST_ID),
        m_iLocHostID(MDB_REP_EMPTY_HOST_ID), m_iBufLen(0), m_ptFileParser(NULL), m_ptRepFileStat(NULL)         
    {

    }
    TMdbRepDataClient::~TMdbRepDataClient()
    {
        SAFE_DELETE(m_pLoadDataTool);
    }

    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	:  初始化
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepDataClient::Init(TMdbConfig* pMdbCfg)
    {
        int iRet = 0;
        CHECK_OBJ(pMdbCfg);        
        m_pLoadDataTool = new(std::nothrow) TMdbLoadDataTool();
        CHECK_OBJ(m_pLoadDataTool);
        CHECK_RET(m_pLoadDataTool->Init(pMdbCfg), "TMdbLoadDataTool Init failed.");

        m_strDsn = pMdbCfg->GetDSN()->sName;
        return 0;
    }
    /******************************************************************************
    * 函数名称	:  Connect
    * 函数描述	: 连接服务端
    * 输入		:  pszIP 对端的IP
    * 输入		:  iPort 对端的IP
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbRepDataClient::Connect(const char* pszIP, int iPort,  int iTimeOut /* = 3000 */)
    {
        TADD_FUNC("Start.");
        bool bRet = true;
        
        if (pszIP == NULL || pszIP[0] == '\0' || iPort <= 0)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM, "Invalid IP[%s] or Port[%d]", pszIP, iPort);
            return false;
        }

        m_strIP.assign(pszIP);
        m_iPort = iPort;
        m_iTimeOut = iTimeOut;

        if(!/*QuickMDB::*/TMdbSyncEngine::IsStart()) 
        {
            /*QuickMDB::*/TMdbSyncEngine::Start();
        }

        m_pPeerInfo = /*QuickMDB::*/TMdbSyncEngine::Connect(pszIP, iPort, NULL, iTimeOut);//三秒超时
        if (m_pPeerInfo != NULL)
        {
            TADD_NORMAL("Connect to Server(%s:%d) OK.", pszIP, iPort);
            m_eState = E_REP_NTC_OK;
        }
        else
        {
            TADD_WARNING_NO_SCREEN("Connect to Server(%s:%d) failed.", pszIP, iPort);
            bRet = false;
        }

        TADD_FUNC("Finish");
        return bRet;
    }

    /******************************************************************************
    * 函数名称	:  SendPacket
    * 函数描述	: 向服务端发送消息
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbRepDataClient::SendPacket(eRepNtcEvent eEvent, const char* psBodyBuffer, int iLength/*=-1*/)
    {
        TADD_FUNC("Start.");
        if (m_eState == E_REP_NTC_CLOSED || NULL == m_pPeerInfo)//未与服务端连接
        {
            return false;
        }

        bool bRet = false;
        if (iLength < 0)
        {
            iLength = psBodyBuffer!=NULL? strlen(psBodyBuffer): 0;
        }

        switch(eEvent)
        {
        case E_LOAD_FROM_REP: //从对端加载数据
        case  E_CLEAN_REP_FILE: //通知对端清除数据
        case E_SEND_REP_DATA: //发送同步数据
            {
                bRet = m_pPeerInfo->PostMessage(psBodyBuffer, iLength);
                TADD_DETAIL("Send Packet[%s] OK", psBodyBuffer);
                break;
            }
        case E_HEART_BEAT:
            {
                bRet = m_pPeerInfo->PostMessage(REP_HEART_BEAT, strlen(REP_HEART_BEAT));
                TADD_DETAIL("Send Packet[%s] OK", REP_HEART_BEAT);
                break;
            }
        default:
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM, "Unknown Message type. Send message failed.");
                return false;
            }
        }
        
        TADD_FUNC("Finish");
        return bRet;
    }

    /******************************************************************************
    * 函数名称	:  SendHearbeat
    * 函数描述	: 向服务端发送心跳包
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/   
    bool TMdbRepDataClient::SendHearbeat()
    {
        return SendPacket(E_HEART_BEAT);
    }

    /******************************************************************************
    * 函数名称	:  GetMsg
    * 函数描述	: 从服务端获取消息
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepDataClient::GetMsg(/*QuickMDB::*/TMdbMsgInfo* &pMsg)
    {
        TADD_FUNC("Start.");
        int iRet = ERROR_SUCCESS;
        if (m_pMsgEvent!=NULL)
        {
            m_pMsgEvent->Release();
        }

        m_pMsgEvent = /*QuickMDB::*/TMdbSyncEngine::GetMessage(m_iTimeOut);
        if (m_pMsgEvent !=NULL && m_pMsgEvent->pMsgInfo->GetLength()>0)
        {
            pMsg = m_pMsgEvent->pMsgInfo;
            //TADD_NORMAL("Len = %d, Msg = %s", m_pMsgEvent->pMsgInfo->GetLength(), m_pMsgEvent->pMsgInfo->GetBuffer());
            /*static FILE *pFile = fopen("fprintf.out","w");
            fprintf(pFile, "Len = %d, Msg = %s", m_pMsgEvent->pMsgInfo->GetLength(), m_pMsgEvent->pMsgInfo->GetBuffer());*/
        }
        else
        {
            iRet = ERR_NET_RECV_FAILED;
        }

        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  Disconnect
    * 函数描述	: 断开与服务端的连接
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    void TMdbRepDataClient::Disconnect()
    {
        TADD_FUNC("Start.");
        /*QuickMDB::*/TMdbSyncEngine::Disconnect(m_pPeerInfo->pno);
        m_eState = E_REP_NTC_CLOSED;
        m_pPeerInfo = NULL;
        TADD_FUNC("Finish");
    }

    /******************************************************************************
    * 函数名称	:  NeedReconnect
    * 函数描述	: 是否需要重新连接
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbRepDataClient::NeedReconnect()
    {
        /*
        if(m_eState == E_REP_NTC_CLOSED)
        {
        time_t tCurTime;
        time(&tCurTime);
        if(tCurTime - m_tConnStateTime > 10)
        {
        time(&m_tConnStateTime);
        return true;
        }
        }
        return false;
        */

        if( NULL == m_pPeerInfo|| m_pPeerInfo->IsShutdown())
        {
            m_eState = E_REP_NTC_CLOSED;
            m_pPeerInfo = NULL;
            TADD_NORMAL("Need to reconnect to Peer[%s:%d].", m_strIP.c_str(), m_iPort);
            return true;
        }
        else
        {
            return false;
        }
    }
    /******************************************************************************
    * 函数名称	:  SendData
    * 函数描述	: 将文件数据发送至对端
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功 !0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepDataClient::SendData(const char* sFileName)
    {
        TADD_DETAIL("Start.");
        int iRet = 0;
        if (NULL == m_ptFileParser)
        {
            m_ptFileParser = new (std::nothrow)TMdbRepFileParser();
            m_ptRepFileStat = new(std::nothrow) TMdbRepFileStat();
        }
        int iOneRecodCounts = 0;
        m_iBufLen = 0;
        CHECK_RET(m_ptFileParser->Init(sFileName), "TMdbRepFileParser init failed.");//设置待解析的文件
        TMdbOneRepRecord* pOneRecord = m_ptFileParser->Next();
        m_ptRepFileStat->Clear();
        while(NULL != pOneRecord)
        { 
            TADD_DETAIL("send:[%s]", pOneRecord->m_sData);
            m_ptRepFileStat->Stat(pOneRecord);
            if(OP_Update == m_ptRepFileStat->GetSqlType())//update操作，重新获取最新的数据，是否需要？
            {
                //iActLen = 0;
                ////sData[0]=0;
                ////m_tFlushDaoCtrl.RenewUpdateColumn((char*)&(pOneRecord->m_sData[4]),sData,iActLen);
                //if(iActLen > 0)
                //{
                //    pOneRecord->m_iLen = iActLen + 4;
                //    memcpy(&pOneRecord->m_sData[4], sData,iActLen);
                //    pOneRecord->m_sData[iActLen+4]='\0';
                //}
            }
            if(MAX_REP_SEND_BUF_LEN - m_iBufLen > pOneRecord->m_iLen)//空间足够，继续将数据放入缓存
            {
                memcpy(&m_sSendBuf[m_iBufLen],(char *)pOneRecord->m_sData, pOneRecord->m_iLen);
            }
            else//send mSendBuffer
            {
                if (!SendPacket(E_SEND_REP_DATA, m_sSendBuf, m_iBufLen))
                {
                    CHECK_RET(ERR_NET_SEND_FAILED, "Send data to peer failed.");
                }
                m_iBufLen = 0;
                memcpy(&m_sSendBuf[m_iBufLen],(char *)pOneRecord->m_sData, pOneRecord->m_iLen);
            }
            ++iOneRecodCounts;
            m_iBufLen+=pOneRecord->m_iLen;
            m_sSendBuf[m_iBufLen] = '\0';

            pOneRecord = m_ptFileParser->Next();
        }

        if(m_iBufLen > 0)//缓冲区剩余数据
        {        
            TADD_DETAIL("packet sending ...");
            TADD_DETAIL("TEST: m_sSendBuf=[%s], m_iBufLen=%d", m_sSendBuf, m_iBufLen);
            if (!SendPacket(E_SEND_REP_DATA, m_sSendBuf, m_iBufLen))
            {
                CHECK_RET(ERR_NET_SEND_FAILED, "Send data to peer failed.");
            }
        }
        //处理完毕一个rep文件总条数
        TADD_NORMAL("FileName = [%s] ,Record Counts=[%d].", sFileName, iOneRecodCounts);
        m_ptRepFileStat->PrintStatInfo();

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  LoadData
    * 函数描述	: 从对端加载数据
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败 
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepDataClient::LoadData(const char* sRoutinglist)
    {
        TADD_NORMAL("Start. Routing list[%s]", sRoutinglist);
        int iRet = 0;
        //先断开连接，再重新重新连接（兼容1.2版本）
        TMdbNtcDateTime::Sleep(100);//保证之前发的信息可以成功发至服务端
        Disconnect();
        if (!Connect(m_strIP.c_str(), m_iPort, m_iTimeOut))
        {
            CHECK_RET(ERR_NET_SOCKET_INVALID, "Reconnect to peer mdbRepServer failed.");
        }
        TMdbShmDSN*pShmDSN = TMdbShmMgr::GetShmDSN(m_strDsn.c_str());
        TMdbTable *pTable = NULL;
        TShmList<TMdbTable>::iterator itor = pShmDSN->m_TableList.begin();
        for(;itor != pShmDSN->m_TableList.end();++itor)//兼容1.2，逐一加载每张表
        {
            pTable = &(*itor);

            if (pTable->m_bShardBack)
            {
                //文件存储加载成功的表，不再加载
                if ( pShmDSN->GetInfo()->m_bLoadFromDisk && pShmDSN->GetInfo()->m_bLoadFromDiskOK && pShmDSN->GetTableSpaceAddrByName(pTable->m_sTableSpace)->m_bFileStorage)
                {
                    continue;
                }

                iRet = LoadOneTable(pTable->sTableName, sRoutinglist);
                if (ERR_TAB_NO_TABLE == iRet)
                {
                    TADD_WARNING("Table[%s] does not exist in peer MDB.", pTable->sTableName);
                    iRet = 0;//继续加载其他表
                }
                else
                {
                    CHECK_RET(iRet, "Deal table [%s] failed.", pTable->sTableName);    
                }
            }     
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

        /******************************************************************************
        * 函数名称	:  SendCleanCmd
        * 函数描述	: 通知备机清除同步文件
        * 输入		:  
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败 
        * 作者		:  jiang.lili
        *******************************************************************************/
    int TMdbRepDataClient::SendCleanCmd()
    {
        int iRet = ERROR_SUCCESS;
        char sMsgBuf[MAX_REP_SEND_MSG_LEN];
        snprintf(sMsgBuf, MAX_REP_SEND_MSG_LEN, "%s%d", CLEAN_REP_FILE_FLAG,m_iLocHostID);
        if (!SendPacket(E_CLEAN_REP_FILE, sMsgBuf, strlen(sMsgBuf)))
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send CLEAN_REP_FILE_FLAG failed.");
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SetHostID
    * 函数描述	:  设置客户端对应的hostID
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepDataClient::SetHostID(int iLocHostID, int iRepHostID)
    {
        m_iRepHostID = iRepHostID;
        m_iLocHostID = iLocHostID;
        return 0;
    }
    /******************************************************************************
    * 函数名称	:  GetHostID
    * 函数描述	:  获取客户端对应的hostID
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbRepDataClient::GetHostID()
    {
        return m_iRepHostID;
    }


    int TMdbRepDataClient::LoadOneTable(const char* sTableName, const char* sRoutinglist)
    {
        TADD_FUNC("Start.");
        TADD_NORMAL("Load table[%s], routing_list = [%s]", sTableName, sRoutinglist);
        int iRet = 0;
        char sMsgBuf[MAX_REP_SEND_BUF_LEN];
        if (atoi(sRoutinglist) == DEFALUT_ROUT_ID)
        {
             snprintf(sMsgBuf, MAX_REP_SEND_BUF_LEN, "%s%s", LOAD_DATA_START_FLAG, sTableName);//格式：Load:TableName|routinglist
        }
        else
        {
             snprintf(sMsgBuf, MAX_REP_SEND_BUF_LEN, "%s%s|%s", LOAD_DATA_START_FLAG, sTableName,  sRoutinglist);//格式：Load:TableName|routinglist
        }
       
        if (SendPacket(E_LOAD_FROM_REP, sMsgBuf))
        {
            TADD_NORMAL("Send load data request OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send load data request failed.");
        }

        /*QuickMDB::*/TMdbMsgInfo* pMsg = NULL;
        bool bOver = false;//是否收到上载结束标识
        while(!bOver)
        {
            if (GetMsg(pMsg) == ERROR_SUCCESS)
            {
                iRet = m_pLoadDataTool->UploadData(pMsg->GetBuffer(), pMsg->GetLength(), bOver);
                if (iRet != ERROR_SUCCESS)
                {
                    TADD_ERROR(iRet, "pLoadDataTool->UploadData failed, Msg = [%s]", pMsg->GetBuffer());
                }
            }
        }
        if (bOver)
        {
            TADD_NORMAL("Load Table[%s] over.", sTableName);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    TMdbRepNTCServer::TMdbRepNTCServer():m_iHeartbeatWarning(HEARTBEAT_TIME_OUT), m_iWorkThreadNum(1)
    {

    }

    TMdbRepNTCServer::~TMdbRepNTCServer()
    {

    }

    /******************************************************************************
    * 函数名称	:  Start
    * 函数描述	: 开启监听和处理线程
    * 输入		:  iMaxThread 最大工作线程数量
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbRepNTCServer::Start(const char* sIP, int iPort, int iMaxThread, int iHeartbeatWarning)
    {
        TADD_FUNC("Start.");       
        bool bRet = false;
        m_iHeartbeatWarning = iHeartbeatWarning<=0? HEARTBEAT_TIME_OUT: iHeartbeatWarning;
        m_iWorkThreadNum = iMaxThread<=0 ? 1 : iMaxThread;

        if ( iPort < 0)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM, "Invalid Port[%d]",  iPort);
            return false;
        }

        TADD_NORMAL("Start to Add listen.");
        bRet = /*QuickMDB::*/TMdbNtcEngine::AddListen(sIP, iPort, new(std::nothrow) TMdbWinntTcpProtocol());
        if (bRet)
        {
            TADD_NORMAL("Start to listen(%d) OK", iPort);
            bRet = true;
        }
        else
        {
            TADD_ERROR(ERR_NET_LISTEN_PORT, "Start to listen(%d) failed", iPort);
            return bRet;
        }

        this->InitEventPump(m_iWorkThreadNum);//初始化工作线程数据量
        if (!this->IsStart())
        {
            bRet = /*QuickMDB::*/TMdbNtcEngine::Start();
            if (bRet)
            {
                TADD_NORMAL("Start NTC Engine OK");
                bRet = true;
            }
            else
            {
                TADD_ERROR(ERR_NET_NTC_START, "Start mdbServer failed");
                return false;
            }
        }

        TADD_FUNC("Finish.");
        return bRet;
    }

    bool TMdbRepNTCServer::OnConnect(/*QuickMDB::*/TMdbConnectEvent *pEventInfo, /*QuickMDB::*/TMdbEventPump *pEventPump)
    {
        TADD_FUNC("Start.");
        bool bRet = true;
        pEventInfo->pPeerInfo->SetHelper(new(std::nothrow) TMdbWinntTcpHelper(pEventInfo->pPeerInfo));
        TADD_NORMAL("Remote connection [%s:%u] is established!\n", pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(), pEventInfo->pPeerInfo->GetRemotePort());

        pEventInfo->pPeerInfo->SetPeerTimeout(MDB_NTC_DATA_IDLE_TIMEOUT, m_iHeartbeatWarning);//设置超时时间

        TADD_FUNC("Finish.");
        return bRet;
    }

    bool TMdbRepNTCServer::OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump)
    {
        TADD_FUNC("Start.");

        TADD_NORMAL("Connection[%s:%u] is disconnected!close by %s,reason:%s", pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(), 
            pEventInfo->pPeerInfo->GetRemotePort(), pEventInfo->IsPasvClose()?"remote":"self", pEventInfo->GetDisconnectReason().c_str());

        TADD_FUNC("Finish.");
        return true;
    }

    bool TMdbRepNTCServer::OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump)
    {
        TADD_NORMAL("Start.");

        TMdbNtcEngine::OnRecvMsg(pEventInfo, pEventPump);

        TADD_FUNC("Finish.");
        return true;
    }

    bool TMdbRepNTCServer::OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump)
    {
        TADD_FUNC("Start.");
        //检查对端是否存在，心跳超时处理
        if (!static_cast<TMdbWinntTcpHelper*>(pEventInfo->pPeerInfo->GetHelper())->SendCheckPacket())//发送心跳包
        {
            TADD_WARNING("Send check package to [%s:%d] failed\n", pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(), pEventInfo->pPeerInfo->GetRemotePort());        
        }
        //pEventInfo->pPeerInfo->SetPeerTimeout(MDB_NTC_DATA_IDLE_TIMEOUT, m_iHeartbeatWarning);//设置超时时间
        TADD_FUNC("Finish.");
        return true;
    }

    /******************************************************************************
    * 函数名称	:  Start
    * 函数描述	: 开启监听和处理线程
    * 输入		:  iMaxThread 最大工作线程数量
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbRepDataServer::Start(const char* sIP, int iPort, int iMaxThread, int iHeartbeatWarning)
    {
        TADD_FUNC("Start.");       
        bool bRet = false;
        m_iHeartbeatWarning = iHeartbeatWarning<=0? HEARTBEAT_TIME_OUT: iHeartbeatWarning;
        m_iWorkThreadNum = iMaxThread<=0 ? 1 : iMaxThread;

        if ( iPort < 0)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM, "Invalid Port[%d]",  iPort);
            return false;
        }

        TADD_NORMAL("Start to Add listen.");
        bRet = /*QuickMDB::*/TMdbNtcEngine::AddListen(sIP, iPort, NULL);
        if (bRet)
        {
            TADD_NORMAL("Start to listen(%d) OK", iPort);
            bRet = true;
        }
        else
        {
            TADD_ERROR(ERR_NET_LISTEN_PORT, "Start to listen(%d) failed", iPort);
            return bRet;
        }

        this->InitEventPump(m_iWorkThreadNum);//初始化工作线程数据量
        if (!this->IsStart())
        {
            bRet = /*QuickMDB::*/TMdbNtcEngine::Start();
            if (bRet)
            {
                TADD_NORMAL("Start mdbServer OK");
                bRet = true;
            }
            else
            {
                TADD_ERROR(ERR_NET_NTC_START, "Start mdbServer failed");
                return false;
            }
        }

        TADD_FUNC("Finish.");
        return bRet;
    }

    TMdbRepDataServer::TMdbRepDataServer(const char* sDsn)
    {
        m_arRcvEngine.Clear();
        m_arRcvEngine.SetAutoRelease(true);
        m_strDsn.Assign(sDsn);
    }

    TMdbRepDataServer::~TMdbRepDataServer()
    {
        for (unsigned int i = 0; i<m_arRcvEngine.GetSize(); i++)
        {
            SAFE_DELETE(m_arRcvEngine[i]);
        }
        m_arRcvEngine.Clear();
    }

    bool TMdbRepDataServer::OnConnect(/*QuickMDB::*/TMdbConnectEvent *pEventInfo, /*QuickMDB::*/TMdbEventPump *pEventPump)
    {
        TADD_NORMAL("Start.");
        bool bRet = true;
        /*QuickMDB::*/TMdbNtcEngine::OnConnect(pEventInfo, pEventPump);

        TADD_NORMAL("Finish.");
        return bRet;

    }

    bool TMdbRepDataServer::OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump)
    {
        TADD_NORMAL("Start.");
        bool bRet = true;
        
        //删除对应的备机接收引擎
        TRepServerDataRcv *pDataRcv = NULL;
        for (unsigned int i = 0; i<m_arRcvEngine.GetSize(); i++)
        {
            pDataRcv = (TRepServerDataRcv*)m_arRcvEngine[i];
            if (pDataRcv->IsSame(pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(), pEventInfo->pPeerInfo->GetRemotePort()))
            {
                SAFE_DELETE(m_arRcvEngine[i]);
                m_arRcvEngine.Remove(i);
            }
        }

        /*QuickMDB::*/TMdbNtcEngine::OnDisconnect(pEventInfo, pEventPump);
        TADD_FUNC("Finish.");
        return bRet;

    }

    bool TMdbRepDataServer::OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump)
    {
        TADD_FUNC("Start.");
        //检查对端是否存在，心跳超时处理
        if (!pEventInfo->pPeerInfo->PostMessage(REP_HEART_BEAT, strlen(REP_HEART_BEAT)))//发送心跳包
        {
            TADD_WARNING("Send check package to [%s:%d] failed\n", pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(), pEventInfo->pPeerInfo->GetRemotePort());        
        }
        //pEventInfo->pPeerInfo->SetPeerTimeout(MDB_NTC_DATA_IDLE_TIMEOUT, m_iHeartbeatWarning);//设置超时时间
        TADD_FUNC("Finish.");
        return true;
    }

    bool TMdbRepDataServer::OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump)
    {
        TADD_DETAIL("Start.");
        bool bRet = true;
        TADD_DETAIL("Length=[%d],Buffer=[%s]", pEventInfo->pMsgInfo->GetLength(),pEventInfo->pMsgInfo->GetBuffer());

        if (/*QuickMDB::*/TMdbNtcStrFunc::StrNoCaseCmp(pEventInfo->pMsgInfo->GetBuffer(), REP_HEART_BEAT)==0)//心跳包
        {
            TADD_DETAIL("Recv check package from [%s:%d]\n", pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(), pEventInfo->pPeerInfo->GetRemotePort());
        }
        else if (strncmp(pEventInfo->pMsgInfo->GetBuffer(), CLEAN_REP_FILE_FLAG, strlen(CLEAN_REP_FILE_FLAG))==0)//清除同步文件
        {
            //清除同步文件
            TADD_NORMAL("Get clean cmd.");
            DealCleanFile(pEventInfo->pMsgInfo->GetBuffer());
        }
        else if (strncmp(pEventInfo->pMsgInfo->GetBuffer(), LOAD_DATA_START_FLAG, strlen(LOAD_DATA_START_FLAG))==0)//加载数据请求
        {
            //根据加载请求，发送相应的表数据
            TADD_NORMAL("Get load cmd.");
            DealLoadRequest(pEventInfo->pMsgInfo->GetBuffer(), pEventInfo->pPeerInfo);
        }
        else//同步数据
        {
            //接收同步数据
            TADD_DETAIL("Get rep data.");
            DealRcvData(pEventInfo->pMsgInfo->GetBuffer(), pEventInfo->pMsgInfo->GetLength(), pEventInfo->pPeerInfo);
        }
        
        TADD_FUNC("Finish.");
        return bRet;
    }

    int TMdbRepDataServer::DealCleanFile(const char* pData)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //CleanRepFile:hostID
        int iHostID;
        if (strlen(pData) == strlen(CLEAN_REP_FILE_FLAG))
        {
            iHostID = MDB_REP_EMPTY_HOST_ID;
        }
        else if (strlen(pData) > strlen(CLEAN_REP_FILE_FLAG))
        {
             iHostID = atoi(pData+strlen(CLEAN_REP_FILE_FLAG));
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "Invalid command [%s] for clean rep file.", pData);
        }

        TRepServerDataSend *m_pDataSend = new(std::nothrow) TRepServerDataSend();
        CHECK_OBJ(m_pDataSend);
        CHECK_RET(m_pDataSend->Init(m_strDsn.c_str()), "TRepServerDataSend Init failed.");
        CHECK_RET(m_pDataSend->CleanRepFile(iHostID), "TRepServerDataSend SendData failed.");

        SAFE_DELETE(m_pDataSend);

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbRepDataServer::DealLoadRequest(const char* pData, /*QuickMDB::*/TMdbPeerInfo* pPeerInfo)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //数据格式Load:TableName|routinglist
        /*QuickMDB::*/TMdbNtcSplit tSplit;
        tSplit.SplitString(pData+strlen(LOAD_DATA_START_FLAG), '|');
        const char *psRoutinglist = NULL;
        const char *psTableName = NULL;
        if (tSplit.GetFieldCount() == 2)
        {
            psTableName = tSplit[0];
            psRoutinglist = tSplit[1];
        }
        else if (tSplit.GetFieldCount() == 1)//兼容1.2版本，不匹配路由条件， 加载所有路由数据，请求加载数据格式为：Load:TableName
        {
            psTableName = tSplit[0];
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "Invalid Msg format[%s]", pData);
        }

        TRepServerDataSend *m_pDataSend = new(std::nothrow) TRepServerDataSend();
        CHECK_OBJ(m_pDataSend);
        CHECK_RET(m_pDataSend->Init(m_strDsn.c_str()), "TRepServerDataSend Init failed.");
        CHECK_RET(m_pDataSend->SendData(psTableName, psRoutinglist, pPeerInfo), "TRepServerDataSend SendData failed.");

        SAFE_DELETE(m_pDataSend);
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbRepDataServer::DealRcvData(const char* pData, int iLength, /*QuickMDB::*/TMdbPeerInfo* pPeerInfo)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TRepServerDataRcv *pDataRcv = GetRcvEngine(pPeerInfo->GetRemoteAddrInfo().c_str(), pPeerInfo->GetRemotePort());
        CHECK_OBJ(pDataRcv);
        CHECK_RET(pDataRcv->DealRcvData(pData, iLength), "Rcv data failed.");    
        TADD_FUNC("Finish.");
        return iRet;
    }

    TRepServerDataRcv* TMdbRepDataServer::GetRcvEngine(const char *strIP, int iPort)
    {
        TRepServerDataRcv *pDataRcv = NULL;
        for (unsigned int i = 0; i<m_arRcvEngine.GetSize(); i++)
        {
            pDataRcv = (TRepServerDataRcv*)m_arRcvEngine[i];
            if (pDataRcv->IsSame(strIP, iPort))
            {
                return pDataRcv;
            }
        }
        pDataRcv = new(std::nothrow) TRepServerDataRcv();
        if (pDataRcv !=NULL)
        {
            pDataRcv->Init(m_strDsn.c_str(), strIP, iPort);
            m_arRcvEngine.Add(pDataRcv);
        }
        return pDataRcv;
    }
    TMdbLoadDataTool::TMdbLoadDataTool():m_pMdbCfg(NULL),m_pDataBase(NULL), m_pCurQuery(NULL), m_psMesgBuf(NULL)
    {
        m_iCurPos = 0;
        m_iMsgLen = 0;
        m_iResidueLen = 0;
        m_bDataOver = false;
    }

    TMdbLoadDataTool::~TMdbLoadDataTool()
    {
        SAFE_DELETE(m_pLoadDao);
        SAFE_DELETE(m_pDataBase);
        m_pMdbCfg = NULL;
        SAFE_DELETE(m_psMesgBuf);
    }

    int TMdbLoadDataTool::Init(TMdbConfig* pMdbCfg)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_pMdbCfg = pMdbCfg;
        m_bDataOver = false;
        m_iMsgBufLen = MAX_REP_SEND_BUF_LEN*2;
        m_psMesgBuf = new(std::nothrow)char[m_iMsgBufLen];
        CHECK_OBJ(m_psMesgBuf);

        try
        {
            m_pDataBase = new(std::nothrow) TMdbDatabase();
            CHECK_OBJ(m_pDataBase);
            if(m_pDataBase->ConnectAsMgr(pMdbCfg->GetDSN()->sName) == false)
            {
                CHECK_RET(ERR_APP_INVALID_PARAM,"ConnectAsMgr [%s] error.",pMdbCfg->GetDSN()->sName);
            }
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"ERROR_SQL=%s.\nERROR_MSG=%s\n",  e.GetErrSql(), e.GetErrMsg());   
        }
        catch(...)
        {
            CHECK_RET(ERROR_UNKNOWN,"Unknown error!\n");   
        } 

        m_pLoadDao = new(std::nothrow) TRepLoadDao();
        CHECK_OBJ(m_pLoadDao);      
        CHECK_RET(m_pLoadDao->Init(m_pDataBase,pMdbCfg), "TRepFlushDao init failed.");

		strTableName.clear();
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbLoadDataTool::UploadData(const char* sDataBuf, int iBufLen, bool &bOver)
    {
        TADD_FUNC("Start. %s", sDataBuf);
        int iRet = 0;
        m_bDataOver = false;
        bOver = false;
        if (iBufLen <=0)
        {
            return iRet;
        }

        //拼接数据，上次遗留数据+本次新接收数据
        CombineData(sDataBuf, iBufLen);
        iRet = DealWithMsgBuf();
        if(iRet == 0 && m_bDataOver)//收到到备机数据结束标识或者出错，需要关闭链接
        {
            bOver = true;
        }
        else if(ERR_TAB_NO_TABLE == iRet)//表在对端不存在
        {
            TADD_WARNING("Table does not exist in peer MDB.");
            bOver = true;
        }
        else if (iRet < 0)
        {
            TADD_ERROR(iRet, "Deal with receive data error.");
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbLoadDataTool::DealWithMsgBuf()
    {
        TADD_FUNC(" Start.");
        int iRet = 0;

        while(true)
        {
            if(m_iCurPos + 10 >= m_iMsgLen)
            {
                SaveRecord(m_iCurPos);
                TADD_DETAIL("m_iCurPos = %d, m_iMsgLen = %d", m_iCurPos, m_iMsgLen);
                break;
            }

            if(strncmp(&(m_psMesgBuf[m_iCurPos]),REP_TABLE_NO_EXIST,strlen(REP_TABLE_NO_EXIST)) == 0)
            {
                TADD_FUNC("Table does not exist in peer MDB.");
                return ERR_TAB_NO_TABLE;
            }

            if(m_psMesgBuf[m_iCurPos]== 'L' && m_psMesgBuf[m_iCurPos+1] == 'o' && m_psMesgBuf[m_iCurPos+2] == 'a'
                && strncmp(&m_psMesgBuf[m_iCurPos], LOAD_DATA_END_FLAG, strlen(LOAD_DATA_END_FLAG))==0)//收到结束标识
            {
                TADD_DETAIL("#####Load Data End.");
                m_bDataOver = true;
                return 0;
            }

            if(m_psMesgBuf[m_iCurPos] == '@' && m_psMesgBuf[m_iCurPos+1] == '@' && m_psMesgBuf[m_iCurPos+2] == '!')//表的开头
            {
                TADD_DETAIL("#####New Table.");
                m_iCurPos+=4;
                int ipos = /*QuickMDB::*/TMdbNtcStrFunc::FindString(&m_psMesgBuf[m_iCurPos], "!!");
				strTableName.clear();
                strTableName.assign(m_psMesgBuf, m_iCurPos, ipos);
                if(TMdbNtcStrFunc::IsDigital(strTableName.c_str()))
                {// 1.2 版本
                    int iTableId = TMdbNtcStrFunc::StrToInt(strTableName.c_str());
                    TMdbTable* pTable = m_pMdbCfg->GetTableByTableId(iTableId);
                    if(NULL == pTable)
                    {
                        CHECK_RET(ERR_TAB_NO_TABLE,"not find table[%s]",strTableName.c_str());
                    }
                    strTableName.assign(pTable->sTableName);
                }
				
                TADD_NORMAL("Table[%s] start.", strTableName.c_str());
                m_pCurQuery = m_pLoadDao->GetQuery(strTableName, false);		
                //CHECK_OBJ(m_pCurQuery);
                /*TMdbTable* pTable =m_pMdbCfg->GetTable(strTableName.c_str());
                pTable->bIsNeedLoadFromOra = false;*/
                
                TShmList<TMdbTable>::iterator itor = TMdbShmMgr::GetShmDSN(m_pMdbCfg->GetDSN()->sName)->m_TableList.begin();
                for(;itor != TMdbShmMgr::GetShmDSN(m_pMdbCfg->GetDSN()->sName)->m_TableList.end();++itor)
                {
                    if (/*QuickMDB::*/TMdbNtcStrFunc::StrNoCaseCmp(itor->sTableName, strTableName.c_str()) == 0)
                    {
                        itor->bIsNeedLoadFromOra = false;
                        break;
                    }
                }

                m_iCurPos+=ipos;
                continue;
            }

            if(m_psMesgBuf[m_iCurPos] == '!' && m_psMesgBuf[m_iCurPos+1] == '!' && m_psMesgBuf[m_iCurPos+2] == '&')
            {
                TADD_DETAIL("#####Table End.");
                int ipos = /*QuickMDB::*/TMdbNtcStrFunc::FindString(&m_psMesgBuf[m_iCurPos], ".OK");
                if(ipos != -1)
                {
                    m_iCurPos = m_iCurPos+ipos+3;
                }
                else
                {
                    SaveRecord(m_iCurPos);
                    break;
                }
                continue;
            }

            if(m_psMesgBuf[m_iCurPos] == '!' && m_psMesgBuf[m_iCurPos+1] == '!' && m_psMesgBuf[m_iCurPos+2] == ',')
            {
                TADD_DETAIL("#####New Record.");
                m_iCurPos+=2;
                int iNextPos = /*QuickMDB::*/TMdbNtcStrFunc::FindString(&m_psMesgBuf[m_iCurPos], "!!");
                if(iNextPos != -1)
                {
                    /*CHECK_RET(GetOneRecord(iNextPos),"GetOneRecord(%d) failed.",iNextPos);
                    CHECK_RET(Execute(),"Excute failed.");*/
                    GetOneRecord(iNextPos);
                    Execute();
                }
                else//剩余记录不完整
                {
                    m_iCurPos-=2;
                    TADD_DETAIL("Left record [%s]",&m_psMesgBuf[m_iCurPos]);
                    SaveRecord(m_iCurPos);
                    break;
                }
                continue;
            }
            else
            {
                m_iCurPos++;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    void TMdbLoadDataTool::SaveRecord(int iCurPos)
    {
        m_iResidueLen = m_iMsgLen-iCurPos;
        memcpy(m_sResidueBuf,&m_psMesgBuf[iCurPos],m_iResidueLen);
        m_sResidueBuf[m_iResidueLen] = 0;
        return;
    }

    int TMdbLoadDataTool::GetOneRecord(int iNextPos)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        memcpy(m_sTempRecord,&m_psMesgBuf[m_iCurPos],iNextPos);
        TADD_DETAIL("Record:[%s]", m_sTempRecord);
        m_sTempRecord[iNextPos] = 0;
        m_iCurPos = m_iCurPos + iNextPos;

        m_vParam.clear();
        int iPos = 0;
        for(int i = 2; i<=iNextPos; i=iPos+2+i)
        {
            m_paramValue[0] = 0;
            iPos = /*QuickMDB::*/TMdbNtcStrFunc::FindString(&m_sTempRecord[i], ",@");
            if(iPos != -1)
            {
                if(iPos >= MAX_VALUE_LEN)
                {
                    CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "data_len[%d] > buff_len[%d].", iPos, MAX_VALUE_LEN);
                }

                memcpy(m_paramValue,&m_sTempRecord[i],iPos);
                m_paramValue[iPos] = 0;
                m_vParam.push_back(m_paramValue);
            }
            else
            {
                if(iNextPos - i > MAX_VALUE_LEN)
                {
                    CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "data_len[%d] > buff_len[%d].", iNextPos - i, MAX_VALUE_LEN);
                }
                memcpy(m_paramValue,&m_sTempRecord[i],iNextPos-i);
                m_paramValue[iNextPos-i] = 0;
                m_vParam.push_back(m_paramValue);
                break;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbLoadDataTool::Execute()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
		int iDropIndex[128] = {0};
		int iCount = 0;
        try
        {
        	m_pMdbCfg->GetDropColumnIndex(strTableName.c_str(), iDropIndex);
            int iColCount = m_vParam.size();
            for(int i=0; i<iColCount; i++)
            {
            	if(iDropIndex[i] == 1)
        		{
					continue;
        		}
                if(IsNULL(m_vParam[i].c_str()))
                {
                    m_pCurQuery->SetParameterNULL(iCount);
                }
                else
                {
                    m_pCurQuery->SetParameter(iCount, m_vParam[i].c_str());
                }
				iCount++;
            }
            m_pCurQuery->Execute();
            m_pCurQuery->Commit();
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERROR_UNKNOWN, "ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    void TMdbLoadDataTool::CombineData(const char* pDataBuf, int iBufLen)
    {
        TADD_FUNC("Start.");

        if (m_iResidueLen+iBufLen > m_iMsgBufLen)//接收到消息包过大，调整消息缓冲区大小
        {
            TADD_DETAIL("Msg package size[%d], m_iResidueLen[%d], m_iMsgBufLen[%d]", iBufLen, m_iResidueLen, m_iMsgBufLen);

            SAFE_DELETE(m_psMesgBuf);
            m_iMsgBufLen = m_iResidueLen+iBufLen+MAX_REP_SEND_BUF_LEN;
            m_psMesgBuf = new(std::nothrow)char[m_iMsgBufLen];
            if (NULL == m_psMesgBuf)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY, "No memory.");
            }
        }

        if(m_iResidueLen != 0)
        {
            memcpy((char*)m_psMesgBuf,m_sResidueBuf,m_iResidueLen);
            memcpy((char*)m_psMesgBuf+m_iResidueLen,pDataBuf,iBufLen);
            m_psMesgBuf[m_iResidueLen+iBufLen] = 0;
        }
        else
        {
            memcpy((char*)m_psMesgBuf,pDataBuf,iBufLen);
            m_psMesgBuf[iBufLen] = 0;
        }
        m_iMsgLen = iBufLen+m_iResidueLen;
        m_sResidueBuf[0]=0;
        m_iResidueLen = 0;
        m_iCurPos = 0;
        TADD_FUNC("Finish.");
    }

    bool TMdbLoadDataTool::IsNULL(const char* sSrc)
    {
        return strncmp(sSrc, "(nil)", strlen(sSrc)) == 0;
    }

//}





