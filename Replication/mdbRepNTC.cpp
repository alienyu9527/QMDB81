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
#include "Helper/mdbBase.h"
#include "Helper/mdbDateTime.h"
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

        if(!TMdbSyncEngine::IsStart()) 
        {
            TMdbSyncEngine::Start();
        }

        m_pPeerInfo = TMdbSyncEngine::Connect(pszIP, iPort, new TMdbWinntTcpProtocol(), m_iTimeOut);//三秒超时
        if (m_pPeerInfo != NULL)
        {
            TADD_NORMAL("Connect to Server(%s:%d) OK.", pszIP, iPort);
            m_pTcpHelper = new(std::nothrow)TMdbWinntTcpHelper(m_pPeerInfo);
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

        bRet = m_pTcpHelper->SendPacket(MDB_ASYN_EVENT, eEvent, psBodyBuffer, iLength);
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

        m_pMsgEvent = TMdbSyncEngine::GetMessage(m_iTimeOut);
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
            TMdbSyncEngine::Disconnect(m_pPeerInfo->pno);
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
                TMdbNtcSplit tSplit;
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
        m_iLocHostID(MDB_REP_EMPTY_HOST_ID), m_iBufLen(0), m_tFlushTime(0), m_ptFileParser(NULL), m_ptRepFileStat(NULL)         
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
        return iRet;
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

        if(!TMdbSyncEngine::IsStart()) 
        {
            TMdbSyncEngine::Start();
        }

        m_pPeerInfo = TMdbSyncEngine::Connect(pszIP, iPort, NULL, iTimeOut);//三秒超时
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
    int TMdbRepDataClient::GetMsg(TMdbMsgInfo* &pMsg)
    {
        TADD_FUNC("Start.");
        int iRet = ERROR_SUCCESS;
        if (m_pMsgEvent!=NULL)
        {
            m_pMsgEvent->Release();
        }

        m_pMsgEvent = TMdbSyncEngine::GetMessage(m_iTimeOut);
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
        TMdbSyncEngine::Disconnect(m_pPeerInfo->pno);
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
    * 函数名称	:  SendData
    * 函数描述	: 将文件数据发送至对端
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功 !0 -失败
    * 作者		:  jiang.xiaolong
    *******************************************************************************/ 
	int TMdbRepDataClient::SendData(const char* sOneRecord, int iLen)
	{
		TADD_FUNC("Start");
		int iRet = 0;
		CHECK_OBJ(sOneRecord);
		//m_iBufLen = 0;
		
		TADD_DETAIL("send:[%s]", sOneRecord);
		if(MAX_REP_SEND_BUF_LEN - m_iBufLen > iLen)//空间足够，继续将数据放入缓存
		{
			memcpy(&m_sSendBuf[m_iBufLen],sOneRecord, iLen);
		}
		else//send mSendBuffer
		{
			if (!SendPacket(E_SEND_REP_DATA, m_sSendBuf, m_iBufLen))
			{
				CHECK_RET(ERR_NET_SEND_FAILED, "Send data to peer failed.");
			}
			m_iBufLen = 0;
			memcpy(&m_sSendBuf[m_iBufLen],sOneRecord, iLen);
		}
		m_iBufLen+=iLen;
		m_sSendBuf[m_iBufLen] = '\0';

		if(TMdbNtcDateTime::GetDiffSeconds(TMdbNtcDateTime::GetCurTime(), m_tFlushTime)>= 1)//每隔1s清空缓冲区
		{
			if(m_iBufLen > 0)//缓冲区剩余数据
			{		 
				TADD_DETAIL("packet sending ...");
				TADD_DETAIL("TEST: m_sSendBuf=[%s], m_iBufLen=%d", m_sSendBuf, m_iBufLen);
				if (!SendPacket(E_SEND_REP_DATA, m_sSendBuf, m_iBufLen))
				{
					CHECK_RET(ERR_NET_SEND_FAILED, "Send data to peer failed.");
				}
				m_iBufLen = 0;
			}
			m_tFlushTime = TMdbNtcDateTime::GetCurTime();
		}
		TADD_FUNC("Finish");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  SendData
	* 函数描述	: 将缓存中残留数据发送至对端
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功 !0 -失败
	* 作者		:  jiang.xiaolong
	*******************************************************************************/ 
	int TMdbRepDataClient::SendData()
	{
		TADD_FUNC("Start");
		int iRet = 0;
		if(m_iBufLen == 0)
		{
			return 1;
		}
		if(m_iBufLen > 0)//缓冲区剩余数据
		{		 
			TADD_DETAIL("packet sending ...");
			TADD_DETAIL("TEST: m_sSendBuf=[%s], m_iBufLen=%d", m_sSendBuf, m_iBufLen);
			if (!SendPacket(E_SEND_REP_DATA, m_sSendBuf, m_iBufLen))
			{
				CHECK_RET(ERR_NET_SEND_FAILED, "Send data to peer failed.");
			}
			m_iBufLen = 0;
		}
		m_tFlushTime = TMdbNtcDateTime::GetCurTime();
		TADD_FUNC("Finish");
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

				
				if(sTblName[0] != 0
					&& TMdbNtcStrFunc::StrNoCaseCmp(sTblName,"all") != 0
					&& TMdbNtcStrFunc::StrNoCaseCmp(pTable->sTableName,sTblName) != 0)
					
					continue;
				
                iRet = LoadOneTable(pTable->sTableName, sRoutinglist, pShmDSN->GetInfo()->m_bIsMemLoad);
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

	int TMdbRepDataClient::SetLoadMsg(const char* sTableName, const char* sRoutinglist, bool bIsMemLoad, char * sMsgBuf)
	{
        TADD_FUNC("Start.");
		int iRet = 0;
		if (atoi(sRoutinglist) == DEFAULT_ROUTE_ID)
        {
             snprintf(sMsgBuf, MAX_REP_SEND_BUF_LEN, "%s%s", LOAD_DATA_START_FLAG, sTableName);//格式：Load:TableName|routinglist
        }
        else
        {
             snprintf(sMsgBuf, MAX_REP_SEND_BUF_LEN, "%s%s|%s", LOAD_DATA_START_FLAG, sTableName,  sRoutinglist);//格式：Load:TableName|routinglist
        }
		if(bIsMemLoad)
		{
			snprintf(sMsgBuf+strlen(sMsgBuf), MAX_REP_SEND_BUF_LEN-strlen(sMsgBuf), "#");
			TMdbShmDSN * pShmDSN = TMdbShmMgr::GetShmDSN(m_strDsn.c_str());
			TMdbTable * pTable = pShmDSN->GetTableByName(sTableName);
			CHECK_OBJ(pTable);
			for(int i = 0; i<pTable->iColumnCounts; i++)
	    	{
				snprintf(sMsgBuf+strlen(sMsgBuf), MAX_REP_SEND_BUF_LEN-strlen(sMsgBuf), "%s,%d,%d,", pTable->tColumn[i].sName
					, pTable->tColumn[i].iDataType, pTable->tColumn[i].iColumnLen);
				if(pTable->tColumn[i].bIsDefault)
				{
					if(pTable->tColumn[i].iDataType == DT_Blob)
					{
						std::string encoded;
	                    encoded = Base::base64_encode(reinterpret_cast<const unsigned char*>(pTable->tColumn[i].iDefaultValue),strlen(pTable->tColumn[i].iDefaultValue));
	                    snprintf(sMsgBuf+strlen(sMsgBuf), MAX_REP_SEND_BUF_LEN-strlen(sMsgBuf), "%s,%s", "Y", encoded.c_str());
					}
					else if(pTable->tColumn[i].iDataType == DT_DateStamp)
					{
						if(pTable->tColumn[i].iColumnLen == sizeof(int))
                        {
                            snprintf(sMsgBuf+strlen(sMsgBuf), MAX_REP_SEND_BUF_LEN-strlen(sMsgBuf), "%s,%d", "Y", \
								(int)TMdbDateTime::StringToTime(pTable->tColumn[i].iDefaultValue,pShmDSN->GetInfo()->m_iTimeDifferent));
                        }
                        else if(pTable->tColumn[i].iColumnLen == sizeof(long long))
                        {
                            snprintf(sMsgBuf+strlen(sMsgBuf), MAX_REP_SEND_BUF_LEN-strlen(sMsgBuf), "%s,%lld", "Y", \
								(long long)TMdbDateTime::StringToTime(pTable->tColumn[i].iDefaultValue,pShmDSN->GetInfo()->m_iTimeDifferent));
                        }
                        else if(pTable->tColumn[i].iColumnLen >= 14)
                        {
                            snprintf(sMsgBuf+strlen(sMsgBuf), MAX_REP_SEND_BUF_LEN-strlen(sMsgBuf), "%s,%s", "Y", pTable->tColumn[i].iDefaultValue);
                        }
					}
					else
					{
						snprintf(sMsgBuf+strlen(sMsgBuf), MAX_REP_SEND_BUF_LEN-strlen(sMsgBuf), "%s,%s", "Y", pTable->tColumn[i].iDefaultValue);
					}
				}
				else
				{
					snprintf(sMsgBuf+strlen(sMsgBuf), MAX_REP_SEND_BUF_LEN-strlen(sMsgBuf), "%s", "N");
				}
				if(i<pTable->iColumnCounts-1)
				{
					snprintf(sMsgBuf+strlen(sMsgBuf), MAX_REP_SEND_BUF_LEN-strlen(sMsgBuf), "@");
				}
	    	}
        	snprintf(sMsgBuf+strlen(sMsgBuf), MAX_REP_SEND_BUF_LEN-strlen(sMsgBuf), "%s%s%s", pTable->iOneRecordNullOffset<0?"$N":"$Y", MdbNtcIsBigEndian()?"%B":"%L", "&Y");//格式：Load:TableName|routinglist&bIsMemLoad
		}//Load:table_name|routing_id1,routing_id2...#col_name1,col_type,col_len,is_default,default_value@ col_name2,col_type,col_len,is_default,default_value@..$bEnableNull%bIsBigEndian&bIsMemLoad
        
        TADD_FUNC("Finish.");
        return iRet;
	}

    int TMdbRepDataClient::LoadOneTable(const char* sTableName, const char* sRoutinglist, bool bIsMemLoad)
    {
        TADD_FUNC("Start.");
        TADD_NORMAL("Load table[%s], routing_list = [%s], bIsMemLoad = [%s]", sTableName, sRoutinglist, bIsMemLoad?"&Y":"N");
        int iRet = 0;
        char sMsgBuf[MAX_REP_SEND_BUF_LEN] = {0};
		CHECK_RET(SetLoadMsg(sTableName,sRoutinglist,bIsMemLoad,sMsgBuf), "SetLoadMsg failed.");
        if (SendPacket(E_LOAD_FROM_REP, sMsgBuf))
        {
            TADD_FLOW("Send load data request [%s] OK.", sMsgBuf);
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send load data request [%s] failed.", sMsgBuf);
        }

        TMdbMsgInfo* pMsg = NULL;
		if(bIsMemLoad)
		{
			while(true)
			{
				if (GetMsg(pMsg) == ERROR_SUCCESS)
				{
					if(TMdbNtcStrFunc::StrNoCaseCmp(pMsg->GetBuffer(), HOST_ENDIAN_NOT_MATCH)==0)
					{
						TADD_DETAIL("Get endian not match msg.");
						bIsMemLoad = false;
						break;
					}
					else if(TMdbNtcStrFunc::StrNoCaseCmp(pMsg->GetBuffer(), HOST_ENDIAN_MATCH)==0)
					{
						TADD_DETAIL("Get endian match msg.");
						break;
					}
					else
					{
						continue;
					}
				}
			}
		}

		if(bIsMemLoad)
		{
			m_pLoadDataTool->SetUploadTable(sTableName);
		}
        pMsg = NULL;
        bool bOver = false;//是否收到上载结束标识
        while(!bOver)
        {
            if (GetMsg(pMsg) == ERROR_SUCCESS)
            {
            	CHECK_RET(m_pLoadDataTool->SetbTool(bTool),"SetbTool failed");
                if(bIsMemLoad)
        		{
					iRet = m_pLoadDataTool->UploadMemData(pMsg->GetBuffer(), pMsg->GetLength(), bOver);
        		}
				else
				{
            		iRet = m_pLoadDataTool->UploadData(pMsg->GetBuffer(), pMsg->GetLength(), bOver);
				}
                if (iRet != ERROR_SUCCESS)
                {
                    TADD_ERROR(iRet, "pLoadDataTool->UploadData failed, Msg = [%s]", pMsg->GetBuffer());
                }
            }
        }
        if (bOver)
        {
        	if(bIsMemLoad)
    		{
	        	//表数据处理完毕，重建索引
				TMdbExecuteEngine tExecEngine;
				CHECK_RET(tExecEngine.ReBuildTableFromPage(m_strDsn.c_str(),TMdbShmMgr::GetShmDSN(m_strDsn.c_str())->GetTableByName(sTableName)),"ReBuildTableFromPage failed.");
			    TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Rebuild Index.");
    		}
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
        bRet = TMdbNtcEngine::AddListen(sIP, iPort, new(std::nothrow) TMdbWinntTcpProtocol());
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
            bRet = TMdbNtcEngine::Start();
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

    bool TMdbRepNTCServer::OnConnect(TMdbConnectEvent *pEventInfo, TMdbEventPump *pEventPump)
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
        bRet = TMdbNtcEngine::AddListen(sIP, iPort, NULL);
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
            bRet = TMdbNtcEngine::Start();
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

    bool TMdbRepDataServer::OnConnect(TMdbConnectEvent *pEventInfo, TMdbEventPump *pEventPump)
    {
        TADD_NORMAL("Start.");
        bool bRet = true;
        TMdbNtcEngine::OnConnect(pEventInfo, pEventPump);

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

        TMdbNtcEngine::OnDisconnect(pEventInfo, pEventPump);
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

        if (TMdbNtcStrFunc::StrNoCaseCmp(pEventInfo->pMsgInfo->GetBuffer(), REP_HEART_BEAT)==0)//心跳包
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

    int TMdbRepDataServer::DealLoadRequest(const char* pData, TMdbPeerInfo* pPeerInfo)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
		bool bIsMemLoad = false;
        //数据格式Load:TableName|routinglist
        TADD_NORMAL("LOAD CMD: [%s]", pData);
		TMdbNtcSplit tSplitLoadType, tSplitEndian, tSplitTableInfo;
        TMdbNtcSplit tSplitRouteId;
		const char * psRemoteTableInfo = NULL;
		tSplitLoadType.SplitString(pData+strlen(LOAD_DATA_START_FLAG), '&');
        if (tSplitLoadType.GetFieldCount() == 2 && tSplitLoadType[1][0] == 'Y')
        {
        	tSplitEndian.SplitString(tSplitLoadType[0], '%');
			if(tSplitEndian[1][0] == MdbNtcIsBigEndian()?'Y':'N')
			{
				bIsMemLoad = true;
				if (!pPeerInfo->PostMessage(HOST_ENDIAN_MATCH, strlen(HOST_ENDIAN_MATCH)))
		        {
		            CHECK_RET(ERR_NET_SEND_FAILED, "Send endian match info to Rep failed..");
		        }
			}
			else
			{
				bIsMemLoad = false;
				if (!pPeerInfo->PostMessage(HOST_ENDIAN_NOT_MATCH, strlen(HOST_ENDIAN_NOT_MATCH)))
		        {
		            CHECK_RET(ERR_NET_SEND_FAILED, "Send endian NOT match info to Rep failed..");
		        }
			}
			tSplitTableInfo.SplitString(tSplitEndian[0], '#');
			if(tSplitTableInfo.GetFieldCount() != 2)
			{
				CHECK_RET(ERR_APP_INVALID_PARAM, "Invalid Msg format[%s]", pData);
			}
			psRemoteTableInfo = tSplitTableInfo[1];//用于校验表结构
			tSplitRouteId.SplitString(tSplitTableInfo[0], '|');//内存传输模式加载命令
        }
		else
		{
			tSplitRouteId.SplitString(tSplitLoadType[0], '|');//普通模式加载命令
		}
        
        const char *psRoutinglist = NULL;
        const char *psTableName = NULL;
        if (tSplitRouteId.GetFieldCount() == 2)
        {
            psTableName = tSplitRouteId[0];
            psRoutinglist = tSplitRouteId[1];
        }
        else if (tSplitRouteId.GetFieldCount() == 1)//兼容1.2版本，不匹配路由条件， 加载所有路由数据，请求加载数据格式为：Load:TableName
        {
            psTableName = tSplitRouteId[0];
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "Invalid Msg format[%s]", pData);
        }

        TRepServerDataSend *m_pDataSend = new(std::nothrow) TRepServerDataSend();
        CHECK_OBJ(m_pDataSend);
        CHECK_RET(m_pDataSend->Init(m_strDsn.c_str()), "TRepServerDataSend Init failed.");
        CHECK_RET(m_pDataSend->SendData(psTableName, psRoutinglist, pPeerInfo, bIsMemLoad, psRemoteTableInfo), "TRepServerDataSend SendData failed.");

        SAFE_DELETE(m_pDataSend);
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbRepDataServer::DealRcvData(const char* pData, int iLength, TMdbPeerInfo* pPeerInfo)
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
		m_pShmDsn = TMdbShmMgr::GetShmDSN(m_pMdbCfg->GetDSN()->sName);
		memset(sDsnName, 0, sizeof(sDsnName));
		SAFESTRCPY(sDsnName, sizeof(sDsnName), m_pMdbCfg->GetDSN()->sName);
        m_bDataOver = false;
        m_iMsgBufLen = MAX_REP_SEND_BUF_LEN*2;
        m_psMesgBuf = new(std::nothrow)char[m_iMsgBufLen];
		m_pCurTable = NULL;
		m_pCurTS = NULL;
		m_cStorage = 'N';
        CHECK_OBJ(m_psMesgBuf);
		CHECK_RET(m_tPageCtrl.SetDSN(sDsnName), "SetDsn failed.");
		CHECK_RET(m_tVarcharCtrl.Init(sDsnName), "Init failed.");
		m_iTempLen = 0;
		
		for(int i = 0; i<MAX_COLUMN_COUNTS; i++)
		{
			m_iVarColPos[i] = -1;
		}
		m_iVarColCount = 0;
		
        try
        {
            m_pDataBase = new(std::nothrow) TMdbDatabase();
            CHECK_OBJ(m_pDataBase);
            if(m_pDataBase->ConnectAsMgr(sDsnName) == false)
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

	int TMdbLoadDataTool::SetUploadTable(const char* sTableName)
	{
		int iRet  = 0;
		m_pCurTable = m_pShmDsn->GetTableByName(sTableName);
		CHECK_OBJ(m_pCurTable);
		m_pCurTS = m_pShmDsn->GetTableSpaceAddrByName(m_pCurTable->m_sTableSpace);
        m_cStorage = m_pCurTS->m_bFileStorage?'Y':'N';
		CHECK_RET(m_tTSCtrl.Init(sDsnName, m_pCurTable->m_sTableSpace),"m_tTSctrl.Init() failed.");
		CHECK_RET(m_tRowCtrl.Init(sDsnName, m_pCurTable), "m_tRowCtrl.Init() failed.");
		for(int i = 0; i<MAX_COLUMN_COUNTS; i++)
		{
			m_iVarColPos[i] = -1;
		}
		m_iVarColCount = 0;
		//获取表中varchar及blob字段位置信息
		for (int i = 0; i < m_pCurTable->iColumnCounts; i++)
		{
			if(DT_VarChar == m_pCurTable->tColumn[i].iDataType || DT_Blob == m_pCurTable->tColumn[i].iDataType)
			{
				m_iVarColPos[m_iVarColCount++] = i;
			}
		}
		return iRet;
	}
	
	int TMdbLoadDataTool::SetbTool(bool bToolFlag)
	{
		bTool =  bToolFlag;
		return 0;
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

	
	int TMdbLoadDataTool::UploadMemData(const char* sDataBuf, int iBufLen, bool &bOver)
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
		iRet = DealWithMemMsgBuf();
		if(iRet == 0 && m_bDataOver)//收到到备机数据结束标识或者出错，需要关闭链接
		{
			bOver = true;
		}
		else if(ERR_TAB_NO_TABLE == iRet)//表在对端不存在
		{
			TADD_WARNING("Table does not exist in peer MDB.");
			bOver = true;
		}
		else if(ERR_TAB_LOAD_FROM_PEER_FAILED == iRet)//对端发送数据错误
		{
			TADD_ERROR(ERR_TAB_LOAD_FROM_PEER_FAILED, "Table load from peer MDB failed.");
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
                int ipos = TMdbNtcStrFunc::FindString(&m_psMesgBuf[m_iCurPos], "!!");
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
				m_pMdbSelQuery = m_pLoadDao->GetMdbSelQuery(strTableName);
				m_pMdbUptQuery = m_pLoadDao->GetMdbUpdateQuery(strTableName);
				
				m_vKeyNo.clear();
				CHECK_RET(m_pLoadDao->GetPrimaryKey(m_vKeyNo),"GetPrimaryKey failed");;
				
                //CHECK_OBJ(m_pCurQuery);
                /*TMdbTable* pTable =m_pMdbCfg->GetTable(strTableName.c_str());
                pTable->bIsNeedLoadFromOra = false;*/
                
                TShmList<TMdbTable>::iterator itor = m_pShmDsn->m_TableList.begin();
                for(;itor != m_pShmDsn->m_TableList.end();++itor)
                {
                    if (TMdbNtcStrFunc::StrNoCaseCmp(itor->sTableName, strTableName.c_str()) == 0)
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
                int ipos = TMdbNtcStrFunc::FindString(&m_psMesgBuf[m_iCurPos], ".OK");
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
                int iNextPos = TMdbNtcStrFunc::FindString(&m_psMesgBuf[m_iCurPos], "!!");
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

	int TMdbLoadDataTool::DealWithMemMsgBuf()
    {
        TADD_FUNC(" Start.");
        int iRet = 0;

        while(true)
        {
        	TADD_DETAIL("m_psMesgBuf[m_iCurPos] = [%s]", &m_psMesgBuf[m_iCurPos]);
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

            if(strncmp(&(m_psMesgBuf[m_iCurPos]),REP_TABLE_ERROR,strlen(REP_TABLE_ERROR)) == 0)
            {
                TADD_FUNC("Table load from peer MDB failed.");
                return ERR_TAB_LOAD_FROM_PEER_FAILED;
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
                int ipos = TMdbNtcStrFunc::FindString(&m_psMesgBuf[m_iCurPos], "!!");
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
                
                TShmList<TMdbTable>::iterator itor = m_pShmDsn->m_TableList.begin();
                for(;itor != m_pShmDsn->m_TableList.end();++itor)
                {
                    if (TMdbNtcStrFunc::StrNoCaseCmp(itor->sTableName, strTableName.c_str()) == 0)
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
                int ipos = TMdbNtcStrFunc::FindString(&m_psMesgBuf[m_iCurPos], ".OK");
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
				m_iTempLen = 0;
				m_iTempLen += (int)(m_psMesgBuf[m_iCurPos+3] - '0')*100000;
				m_iTempLen += (int)(m_psMesgBuf[m_iCurPos+4] - '0')*10000;
				m_iTempLen += (int)(m_psMesgBuf[m_iCurPos+5] - '0')*1000;
				m_iTempLen += (int)(m_psMesgBuf[m_iCurPos+6] - '0')*100;
				m_iTempLen += (int)(m_psMesgBuf[m_iCurPos+7] - '0')*10;
				m_iTempLen += (int)(m_psMesgBuf[m_iCurPos+8] - '0');

				if(m_iCurPos + m_iTempLen >= m_iMsgLen)
		        {
		            SaveRecord(m_iCurPos);
		            TADD_DETAIL("m_iCurPos = %d, m_iMsgLen = %d", m_iCurPos, m_iMsgLen);
		            break;
		        }
				else
				{
			        memcpy(m_sTempRecord,&m_psMesgBuf[m_iCurPos],m_iTempLen);
			        TADD_DETAIL("Record:[%s]", m_sTempRecord);
			        m_sTempRecord[m_iTempLen] = 0;
			        m_iCurPos = m_iCurPos + m_iTempLen;
					ExecuteMemData();
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
            iPos = TMdbNtcStrFunc::FindString(&m_sTempRecord[i], ",@");
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

	int TMdbLoadDataTool::ExecuteForMdbReLoadUpdate(size_t iColCount,int* iDropIndex)
	{
		int iCount = 0;
		size_t i = 0,j = 0;
		//update

		//update里面其他的非主键列
		for(i=0; i<iColCount; i++)
		{
			if(iDropIndex[i] == 1)
			{
				continue;
			}
		
			for(j=0; j<m_vKeyNo.size(); j++)
				if(i ==  m_vKeyNo[j])
					break;
							
			if(j<m_vKeyNo.size())
				continue;
							
			if(IsNULL(m_vParam[i].c_str()))
			{
				m_pMdbUptQuery->SetParameterNULL(iCount);
			}
			else
			{
				m_pMdbUptQuery->SetParameter(iCount, m_vParam[i].c_str());
			}
				iCount++;
		}
		
		
		//update里面的主键列
		for(i=0; i<iColCount; i++)
		{
			if(iDropIndex[i] == 1)
			{
				continue;
			}
		
			for(j=0; j<m_vKeyNo.size(); j++)
				if(i ==  m_vKeyNo[j])
					break;
							
			if(j<m_vKeyNo.size())
			{
				m_pMdbUptQuery->SetParameter(iCount, m_vParam[i].c_str());
				iCount++;
			}
							
		}
		
		m_pMdbUptQuery->Execute();
		m_pMdbUptQuery->Commit();
		return 0;
		
	}

	
	int TMdbLoadDataTool::ExecuteForMdbReLoadInsert(size_t iColCount,int* iDropIndex)
	{
		int iCount = 0;
		size_t i = 0;
		
		for(i=0; i<iColCount; i++)
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
		return 0;
	}
	
	int TMdbLoadDataTool::ExecuteForMdbReLoad(size_t iColCount,int* iDropIndex)
	{
		
		size_t i = 0,j = 0;
		int iCnt = 0;
		int iRet = 0;
		//select
		for(i=0; i<iColCount; i++)
		{
			if(iDropIndex[i] == 1)
			{
				continue;
			}
			for(j=0; j<m_vKeyNo.size(); j++)
				if(i ==  m_vKeyNo[j])
					break;
					
			if(j< m_vKeyNo.size())
			{
					
				m_pMdbSelQuery->SetParameter(iCnt, m_vParam[i].c_str());
				iCnt++;
			}
		
		}
		
		m_pMdbSelQuery->Execute();
		m_pMdbSelQuery->Open();
			 
		if(m_pMdbSelQuery->Next())
		{
			//update
			CHECK_RET(ExecuteForMdbReLoadUpdate(iColCount,iDropIndex),"ExecuteForMdbReLoadInsert failed");
					
		
		}
		else
		{
			
			//insert
			CHECK_RET(ExecuteForMdbReLoadInsert(iColCount,iDropIndex),"ExecuteForMdbReLoadInsert failed");
					
		}

		return iRet;
	}

	
    int TMdbLoadDataTool::Execute()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
		int iDropIndex[128] = {0};
		int iCount = 0;
		size_t i = 0;
		
        try
        {
        	m_pMdbCfg->GetDropColumnIndex(strTableName.c_str(), iDropIndex);
            size_t iColCount = m_vParam.size();

			//来自于命令mdbReload的调用的处理
			if(bTool)
			{
				
				CHECK_RET(ExecuteForMdbReLoad(iColCount,iDropIndex),"ExecuteForMdbReLoadInsert failed");	
 
			}

			else
			{
				iCount = 0;
				//insert
			 		for(i=0; i<iColCount; i++)
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
			
            
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERROR_UNKNOWN, "ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbLoadDataTool::ExecuteMemData()
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		int iCurPos = 0;
		int iVarLen = 0;
	    int iWhichPos = -1;
	    unsigned int iRowId = 0;
		TMdbColumn * pVarColumn = NULL;
		
		//插入varchar及blob数据并更新临时记录中的地址
		iCurPos += m_pCurTable->iOneRecordSize + 9;
		for(int i = 0; i<m_iVarColCount; i++)
		{
			pVarColumn = &m_pCurTable->tColumn[m_iVarColPos[i]];
			if(!m_tRowCtrl.IsColumnNull(pVarColumn, m_sTempRecord+9))
			{
				iVarLen = 0;
				if(m_sTempRecord[iCurPos]==',' && m_sTempRecord[iCurPos+1]=='@')
				{
					iVarLen += (int)(m_sTempRecord[iCurPos+2] - '0')*1000;
					iVarLen += (int)(m_sTempRecord[iCurPos+3] - '0')*100;
					iVarLen += (int)(m_sTempRecord[iCurPos+4] - '0')*10;
					iVarLen += (int)(m_sTempRecord[iCurPos+5] - '0');
				}
				else
				{
					TADD_ERROR(ERR_APP_DATA_INVALID, "Varchar/blob column data position error.");
					return ERR_APP_DATA_INVALID;
				}
				do
	            {
	                iWhichPos = -1;
	                iRowId = 0;
					CHECK_RET(m_tVarcharCtrl.Insert(&m_sTempRecord[iCurPos+6], iVarLen, iWhichPos, iRowId,m_cStorage),"Insert Varchar Faild,ColoumName=[%s],iVarCharlen[%d]",pVarColumn->sName,iVarLen);
					TADD_FLOW("iWhichPos = [%d], iRowID = [%u], m_cSorage = [%c]", iWhichPos, iRowId, m_cStorage);
	                m_tVarcharCtrl.SetStorgePos(iWhichPos, iRowId, m_sTempRecord+9+pVarColumn->iOffSet);
					TADD_FLOW("m_sTempRecord+9+pVarColumn->iOffSet[0] = [%d], m_sTempRecord+9+pVarColumn->iOffSet[1] = [%d]", (m_sTempRecord+9+pVarColumn->iOffSet)[0]-'0', (int*)&((m_sTempRecord+9+pVarColumn->iOffSet)[1]));
					iCurPos += iVarLen+6;
	            }while(0);
			}
		}
		
		//将定长记录插入表空闲页中
		while(1)
        {   //申请一块新空间
        	m_pCurFreePage = NULL;
			CHECK_RET(m_tTSCtrl.GetFreePage(m_pCurTable, m_pCurFreePage), "m_tTSCtrl.GetFreePage() failed.");
			CHECK_RET(m_tPageCtrl.Attach((char *)m_pCurFreePage, m_pCurTable->bReadLock, true), "Can't Attach to page.");
	        CHECK_RET(m_tPageCtrl.WLock(),"tPageCtrl.WLock() failed.");
            int iDataOffset = 0;
			TMdbRowID tRowID;
            iRet = m_pCurFreePage->GetFreeRecord(iDataOffset,m_pCurTable->iOneRecordSize);
            if(iRet == ERR_PAGE_NO_MEMORY)
            {//page没有空间了，无需报错
            	TADD_DETAIL("Current page is Full.");
                CHECK_RET(m_tTSCtrl.TablePageFreeToFull(m_pCurTable,m_pCurFreePage),"FreeToFull() error.iRet=[%d]",iRet);
        		m_tPageCtrl.UnWLock();//解页锁
                continue;
            }
            else if(0 != iRet)
            {//其他错误
                CHECK_RET_BREAK(iRet,"GetFreeRecord failed,page=[%s]", m_pCurFreePage->ToString().c_str());
            }
            tRowID.SetDataOffset(m_pCurFreePage->DataOffsetToRecordPos(iDataOffset));//设置数据位置
            tRowID.SetPageID(m_pCurFreePage->m_iPageID);
            memcpy((char *)m_pCurFreePage+iDataOffset, m_sTempRecord+9, m_pCurTable->iOneRecordSize);
            //修改头信息:记录数、数据偏移量、剩余空间大小、时间戳
            SAFESTRCPY(m_pCurFreePage->m_sUpdateTime,sizeof(m_pCurFreePage->m_sUpdateTime),m_pShmDsn->GetInfo()->sCurTime);
            ++(m_pCurFreePage->m_iRecordCounts); 
            m_pCurFreePage->m_iPageLSN = m_pShmDsn->GetInfo()->GetLSN();
            TADD_DETAIL("page_id=[%d],offsetPos=[%d],rowid=[%d].",m_pCurFreePage->m_iPageID,iDataOffset,tRowID.m_iRowID);
			break;
        }
        if(iRet == 0)
        {
            m_tTSCtrl.SetPageDirtyFlag(m_pCurFreePage->m_iPageID);
        }
        m_tPageCtrl.UnWLock();//解页锁
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





