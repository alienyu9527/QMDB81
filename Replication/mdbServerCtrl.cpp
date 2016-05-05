/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbServerCtrl.h		
*@Description�� ���÷��������
*@Author:		jiang.lili
*@Date��	    2014/03/20
*@History:
******************************************************************************************/
#include "Replication/mdbServerCtrl.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"

//namespace QuickMDB
//{
    TMdbServerClient::TMdbServerClient()
    {
        m_bRepExist = false;
    }

    TMdbServerClient::~TMdbServerClient()
    {

    }

    /******************************************************************************
    * ��������	:  Init
    * ��������	: ��ʼ��
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerClient::Init(TMdbServerConfig * pConfig, TMdbStatMonitor *pMonitor)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pConfig);
        m_pConfig = pConfig;
        CHECK_OBJ(pMonitor);
        m_PMonitor = pMonitor;

        TMdbRepHost*pRepServer = pConfig->GetRepServer();
        if (NULL == pRepServer)//����������
        {
            TADD_WARNING("MdbServer doesn't have a standby server.");
            m_bRepExist = false;
        }
        else
        {
            m_bRepExist = true;
            m_sRepServerIP = pRepServer->m_strIP;
            m_iRepPort = pRepServer->m_iPort;
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    bool TMdbServerClient::Connect()
    {
        TADD_FUNC("Start");

        bool bRet = TMdbRepNTCClient::Connect(m_sRepServerIP.c_str(), m_iRepPort);
        if (bRet)
        {
            TADD_NORMAL("Connect to standby server OK.");
        }
        else
        {
            TADD_WARNING_NO_SCREEN("Connect to standby server(IP[%s]:Port[%d]) failed.", m_sRepServerIP.c_str(), m_iRepPort);
        }

        TADD_FUNC("Finish.");
        return bRet;
    }

    ///******************************************************************************
    //* ��������	:  GetRouerInfo
    //* ��������	: �ӱ�����ȡ�͸���·��������Ϣ
    //* ����		:  
    //* ���		:  
    //* ����ֵ	:  0 - �ɹ�!0 -ʧ��
    //* ����		:  jiang.lili
    //*******************************************************************************/
    //int TMdbServerClient::GetRouerInfo()
    //{
    //    int iRet = 0;
    //    return iRet;
    //}

    /******************************************************************************
    * ��������	:  SyncCfgFile
    * ��������	: ͬ�����������ļ�
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerClient::SyncLocalCfgFile()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if (TMdbRepNTCClient::SendPacket(E_REP_SYNC_CONFIG_REQUEST))
        {
            TADD_NORMAL("Send sync request to standby server OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED,  "Send sync request to standby server failed.");
        }
        TRepNTCMsg tMsg;
        CHECK_RET(TMdbRepNTCClient::GetMsg(tMsg), "Get message from standby server failed.");//���մ����÷��񱸻����͹�����·����Ϣ
        if (tMsg.eEvent != E_REP_SYNC_CONFIG_REQUEST_ANS || tMsg.iMsgLen <=0)
        {
            CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "Get data from standby server failed. EVENT = [%d], MsgLen = [%d]", tMsg.eEvent, tMsg.iMsgLen);
        }
        else
        {
            TADD_NORMAL("Get message from standby server, [%s].", tMsg.ToString());
            CHECK_RET(m_pConfig->SyncLocalCfgFile(tMsg.psMsg), "Synchronize local configuration file failed." );//ͬ�������ļ�
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SyncLocalMdbState
    * ��������	: ͬ������MDB״̬
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerClient::SyncLocalMdbState()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if (TMdbRepNTCClient::SendPacket(E_REP_SYNC_STATE_REQUEST))
        {
            TADD_NORMAL("Send sync MDB state request to standby server OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED,  "Send sync MDB state request to standby server failed.");
        }
        TRepNTCMsg tMsg;
        CHECK_RET(TMdbRepNTCClient::GetMsg(tMsg), "Get message from standby server failed.");//���մ����÷��񱸻����͹�����·����Ϣ
        TADD_NORMAL("Get message from standby server, [%s]", tMsg.ToString());
        if (tMsg.eEvent != E_REP_SYNC_STATE_REQUEST_ANS || tMsg.iMsgLen <=0)
        {
            CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "Get data from standby server failed. EVENT = [%d], MsgLen = [%d]", tMsg.eEvent, tMsg.iMsgLen);
        }
        else
        {
            CHECK_RET(m_PMonitor->SyncState(tMsg.psMsg), "SyncState failed." );//ͬ��״̬��Ϣ
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SendRouterChange
    * ��������	: ��·�ɱ�����͸�����
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerClient::SendRouterChange(const char* sMsg)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if (SendPacket(E_REP_ROUTING_CHANGE, sMsg))
        {
            TADD_NORMAL("Send ROUTING_CHANGE Info to rep server OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send ROUTING_CHANGE Info to rep server failed.");
        }

        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SendStatChange
    * ��������	: ����״̬�����Ϣ
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerClient::SendStatChange(const char* sMsg)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if (SendPacket(E_REP_STAT_CHANGE, sMsg))
        {
            TADD_NORMAL("Send STAT_CHANGE Info to rep server OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send STAT_CHANGE Info to rep server failed.");
        }

        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SendStatChange
    * ��������	: ����״̬�����Ϣ
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerClient::SendStatChange(int iHostID, /*QuickMDB::*/EMdbRepState eState)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //ƴ����Ϣ
        char sMsg[MAX_REP_SEND_MSG_LEN];
        snprintf(sMsg, MAX_REP_SEND_MSG_LEN, "%d|%d", iHostID, eState);

        //������Ϣ
        if (SendPacket(E_REP_STAT_CHANGE, sMsg))
        {
            TADD_NORMAL("Send ROUTING_CHANGE Info to rep server OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send ROUTING_CHANGE Info to rep server failed.");
        }

        TADD_FUNC("Finish");
        return iRet;
    }

    int TMdbServerClient::DealRepServerCmd(/*QuickMDB::*/TRepNTCMsg &tMsg)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        switch(tMsg.eEvent)
        {
        case 0:
            {
                TADD_FUNC("Get heartbeat check from server.");
                SendHearbeat();
                break;
            }
        default:
            {
                CHECK_RET(ERR_APP_INVALID_PARAM, "Unknown command.");
            }
        }

        TADD_FUNC("Finish");
        return iRet;
    }

    TMdbStatMonitor::TMdbStatMonitor():m_pServerConfig(NULL)
    {

    }

    TMdbStatMonitor::~TMdbStatMonitor()
    {

    }

    /******************************************************************************
    * ��������	:  Init
    * ��������	: ��ʼ��״̬�����Ϣ
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbStatMonitor::Init(TMdbServerConfig *pConfig)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pConfig);
        m_pServerConfig = pConfig;
        //�������ļ���ÿ�����е�ÿ��host_id��Ӧ������״̬����Ϊunknown

        TMdbRepState tState;
        for (unsigned int i = 0; i<m_pServerConfig->m_arGroup.size(); i++)
        {
            for(unsigned int j = 0; j < m_pServerConfig->m_arGroup[i].m_arHostIDList.size(); j++)
            {
                tState.m_iHostID = m_pServerConfig->m_arGroup[i].m_arHostIDList[j];
                tState.m_eState = E_MDB_STATE_UNKNOWN;
                m_tMdbState.insert(std::pair<int, TMdbRepState>(tState.m_iHostID, tState));
            }
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  SyncState
    * ��������	:  ͬ��MDB״̬
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbStatMonitor::SyncState(const char* pData)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbNtcSplit tSplit;
        tSplit.SplitString(pData, '|', true);
        TMdbNtcSplit tSplitColon;
        TMdbRepState tState;
        m_tMdbState.clear();
        for (unsigned int i = 0; i<tSplit.GetFieldCount(); i++)
        {
            tSplitColon.SplitString(tSplit[i], ':');
            if (tSplitColon.GetFieldCount() != 2)
            {
                CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "Invalid state data format.");
            }
            tState.m_iHostID = atoi(tSplitColon[0]);
            tState.m_eState =  (EMdbRepState)atoi(tSplitColon[1]);
            m_tMdbState.insert(std::pair<int, TMdbRepState>(tState.m_iHostID, tState));
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GetStateData
    * ��������	:  ��ȡ����MDB״̬
    * ����		:  
    * ���		:  HostID:State|HostID:State
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��  
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbStatMonitor::GetStateData(char*pDataBuf, int iBufLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        std::map<int, TMdbRepState>::iterator itor = m_tMdbState.begin();
        int iStrLen;
        for (; itor!=m_tMdbState.end(); ++itor)
        {
            iStrLen = strlen(pDataBuf);
            snprintf(pDataBuf+iStrLen, iBufLen-iStrLen, "%d:%d|", (*itor).second.m_iHostID, (int)((*itor).second.m_eState));
        }

        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  AddMDB
    * ��������	: ����һ��QuickMDB��Ϣ
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbStatMonitor::AddMDB(TMdbRepState &tMdb)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_tMdbState.insert(std::pair<int, TMdbRepState>(tMdb.m_iHostID, tMdb));

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  UpdateState
    * ��������	: ����һ��QuickMDB״̬
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbStatMonitor::UpdateState(int iHostID, EMdbRepState eState)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbRepState *pState = &m_tMdbState.find(iHostID)->second;
        if (pState!=NULL)
        {
            pState->m_eState = eState;
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "Host ID [%d] not found.", iHostID);
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  RemoveMDB
    * ��������	: ɾ��һ��QuickMDB��Ϣ
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbStatMonitor::RemoveMDB(int iHostID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbRepState *pState = &m_tMdbState.find(iHostID)->second;
        if (pState!=NULL)
        {
            m_tMdbState.erase(iHostID);
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM, "Host ID [%d] not found.", iHostID);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GetRoutingInfo
    * ��������	: ��ȡ·����Ϣ
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbStatMonitor::GetRoutingInfo(int iRoutingID, char* sData)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        snprintf(sData, MAX_REP_SEND_BUF_LEN, "ROUTING INFOMATION: \n");
        if (-1 == iRoutingID)//��ӡ����·����Ϣ
        {
            std::map<int, TMdbRepRules>::iterator itorpRules = m_pServerConfig->m_AllRules.begin();
            for (; itorpRules != m_pServerConfig->m_AllRules.end(); ++itorpRules)
            {
                snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "\tRouing_ID %d : %s \n", itorpRules->second.m_iRuleID, itorpRules->second.m_strRule.c_str());
            }
        }
        else//��ӡָ��·����Ϣ
        {
            std::map<int, TMdbRepRules>::iterator itorpRules = m_pServerConfig->m_AllRules.find(iRoutingID);
            if (itorpRules != m_pServerConfig->m_AllRules.end())
            {
                snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "\tRouing_ID %d : %s \n", itorpRules->second.m_iRuleID, itorpRules->second.m_strRule.c_str()); 
            }
            else
            {
                snprintf(sData, MAX_REP_SEND_BUF_LEN-strlen(sData), "\tInvalid rule ID.\n"); 
            }
        }

        TADD_NORMAL("sData = %s", sData);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GetHostInfo
    * ��������	: ��ȡ������Ϣ
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbStatMonitor::GetHostInfo(int iHostID, char* sData)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TADD_NORMAL("Get host information, hostID = %d", iHostID);
        snprintf(sData, MAX_REP_SEND_BUF_LEN, "HOST INFORMATION:\n");
        char sState[MAX_NAME_LEN];
        sState[0] = '\0';

        if (-1 == iHostID)//��ӡ����������Ϣ
        {
            std::map<int, TMdbRepHost>::iterator itorpHost = m_pServerConfig->m_AllHosts.begin();
            for (; itorpHost != m_pServerConfig->m_AllHosts.end(); ++itorpHost)
            {
                CHECK_RET(GetHostState(itorpHost->second.m_iHostID, sState), "Get host[%d] state failed.", itorpHost->second.m_iHostID);
                snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "\tHost_ID %d : %s(%d)\t%s \n", 
                    itorpHost->second.m_iHostID, itorpHost->second.m_strIP.c_str(), itorpHost->second.m_iPort, sState);
            }
        }
        else//��ӡָ��������Ϣ
        {
            std::map<int, TMdbRepHost>::iterator itorpHost = m_pServerConfig->m_AllHosts.find(iHostID);
            if (itorpHost != m_pServerConfig->m_AllHosts.end())
            {
                CHECK_RET(GetHostState(itorpHost->second.m_iHostID, sState), "Get host[%d] state failed.", itorpHost->second.m_iHostID);
                snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "\tHost_ID %d : %s(%d)\t%s \n", 
                    itorpHost->second.m_iHostID, itorpHost->second.m_strIP.c_str(), itorpHost->second.m_iPort, sState);
            }
            else
            {
                snprintf(sData, MAX_REP_SEND_BUF_LEN-strlen(sData), "\tInvalid host ID.\n"); 
            }
        }

        TADD_NORMAL("sData = %s", sData);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GetGroupInfo
    * ��������	: ��ȡ������Ϣ
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbStatMonitor::GetGroupInfo(int iGroupID, char* sData)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TADD_NORMAL("Get host information, GroupID = %d", iGroupID);
        snprintf(sData, MAX_REP_SEND_BUF_LEN, "GROUP INFORMATION:\n");

        if (-1 == iGroupID)//��ӡ����·����Ϣ
        {
            std::vector<TMdbGroup>::iterator itorpGroup = m_pServerConfig->m_arGroup.begin();
            int iCount = 0;
            for (; itorpGroup != m_pServerConfig->m_arGroup.end(); ++itorpGroup)
            {
                snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "\tGroup_ID %d : \n \t\tRule_ID_List: \t", iCount);
                GetOneGroup((*itorpGroup), sData);
                iCount++;
            }
        }
        else//��ӡָ��·����Ϣ
        {
            if (iGroupID >= m_pServerConfig->m_arGroup.size() || iGroupID < 0)
            {
                snprintf(sData, MAX_REP_SEND_BUF_LEN, "\tInvalid group ID[%d].", iGroupID);
            }
            else
            {
                GetOneGroup(m_pServerConfig->m_arGroup[iGroupID], sData);
            }
        }

        TADD_NORMAL("sData = %s", sData);
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbStatMonitor::GetOneGroup(TMdbGroup& tGroup, char *sData)
    {
        //·��ID
        for (unsigned int i = 0; i < tGroup.m_arRuleIDList.size(); i++)
        {
            if (0 == i)
            {
                snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "%d", tGroup.m_arRuleIDList[i]);
            }
            else
            {
                snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), ", %d", tGroup.m_arRuleIDList[i]);
            }
        }
        snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "\n\t\tHost_ID_List:\t");
        //����ID
        for (unsigned int i = 0; i < tGroup.m_arHostIDList.size(); i++)
        {
            if (0 == i)
            {
                snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "%d", tGroup.m_arHostIDList[i]);
            }
            else
            {
                snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), ", %d", tGroup.m_arHostIDList[i]);
            }
        }
        //ӳ���ϵ
        snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "\n\t\tRouting_map:\n");
        std::map<int, std::vector<int> >::iterator itorMap = tGroup.m_tRoutingMap.begin();
        for (; itorMap != tGroup.m_tRoutingMap.end(); ++itorMap)
        {
            snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "\t\t\tRouing_ID: %d\tHost_ID_List:", itorMap->first);
            for (unsigned i = 0; i < itorMap->second.size(); i++)
            {
                if(0 == i)
                {
                    snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), " %d", itorMap->second[i]);
                }
                else
                {
                    snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), ", %d", itorMap->second[i]);
                }
            }
            snprintf(sData+strlen(sData), MAX_REP_SEND_BUF_LEN-strlen(sData), "\n");
        }
        return 0;
    }

    int TMdbStatMonitor::GetHostState(int iHostID, char* sState)
    {
        int iRet = 0;
        if (iHostID == m_pServerConfig->m_iLoaclHostID || iHostID == m_pServerConfig->m_iRepHostID)//Ϊ����������
        {
            SAFESTRCPY(sState, MAX_NAME_LEN, "SERVER");
            return iRet;
        }

        std::map<int, TMdbRepState>::iterator itor = m_tMdbState.find(iHostID);
        if (itor == m_tMdbState.end())
        {
            return ERR_APP_INVALID_PARAM;
        }
        sState[0] = '\0';
        switch(itor->second.m_eState)
        {
        case E_MDB_STATE_ABNORMAL:
            {
                SAFESTRCPY(sState, MAX_NAME_LEN, "ABNORMAL");
                break;
            }
        case  E_MDB_STATE_CREATED:
            {
                SAFESTRCPY(sState, MAX_NAME_LEN, "CREATED");
                break;
            }
        case  E_MDB_STATE_CREATING:
            {
                SAFESTRCPY(sState, MAX_NAME_LEN, "CREATING");
                break;
            }
        case  E_MDB_STATE_RUNNING:
            {
                SAFESTRCPY(sState, MAX_NAME_LEN, "RUNNING");
                break;
            }
        case  E_MDB_STATE_DESTROYED:
            {
                SAFESTRCPY(sState, MAX_NAME_LEN, "DESTROYED");
                break;
            }
        case  E_MDB_STATE_UNKNOWN:
            {
                SAFESTRCPY(sState, MAX_NAME_LEN, "UNKNOWN");
                break;
            }
        default:
            {
                return ERR_APP_INVALID_PARAM;
            }
        }
        return iRet;
    }

    EMdbRepState TMdbStatMonitor::GetHostState(int iHostID)
    {
        std::map<int, TMdbRepState>::iterator itor = m_tMdbState.find(iHostID);
        if (itor == m_tMdbState.end())
        {
            return E_MDB_STATE_UNKNOWN;
        }
        else
        {
            return itor->second.m_eState;
        }
    }

    TMdbServerEngine::TMdbServerEngine(): m_pClient(NULL), m_pMonitor(NULL), m_pConfig(NULL)
    {

    }

    TMdbServerEngine::~TMdbServerEngine()
    {

    }

    int TMdbServerEngine::Init(TMdbStatMonitor *pMonitor, TMdbServerClient* pClient, TMdbServerConfig *pConfig)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pMonitor);
        CHECK_OBJ(pClient);
        CHECK_OBJ(pConfig);

        m_pClient = pClient;
        m_pMonitor  = pMonitor;
        m_pConfig = pConfig;

        TADD_FUNC("Finish.");
        return iRet;        
    }

    bool TMdbServerEngine::OnConnect(TMdbConnectEvent* pEventInfo, TMdbEventPump* pEventPump)
    {
        TADD_FUNC("Start.");
        bool bRet = true;
        pEventInfo->pPeerInfo->SetHelper(new(std::nothrow) TMdbWinntTcpHelper(pEventInfo->pPeerInfo));

        TMdbRepHost tClientHost;
        tClientHost.m_iHostID = -1;
        tClientHost.m_iPort = pEventInfo->pPeerInfo->GetRemotePort();
        tClientHost.m_strIP = pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str();
        m_arClientHost.push_back(tClientHost);

        TADD_NORMAL("Remote connection [%s:%u] is established!\n", tClientHost.m_strIP.c_str(), tClientHost.m_iPort);
        pEventInfo->pPeerInfo->SetPeerTimeout(MDB_NTC_DATA_IDLE_TIMEOUT, m_iHeartbeatWarning);//���ó�ʱʱ��

        TADD_FUNC("Finish.");
        return bRet;
    }

    bool TMdbServerEngine::OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump)
    {
        TADD_FUNC("Start.");
        bool bRet = true;
        TMdbRepNTCServer::OnDisconnect(pEventInfo, pEventPump);
        //mdb�������������÷��������
        int iHostId = GetHostID(pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(), pEventInfo->pPeerInfo->GetRemotePort());
        if(iHostId > 0)
        {
            //����״̬Ϊ�쳣            
            EMdbRepState eState = m_pMonitor->GetHostState(iHostId);
            if (eState == E_MDB_STATE_RUNNING)//mdb���л��ߴ��������ж���
            {
                TADD_WARNING("Host[%d|%s:%u] disconnected while its state is [RUNNING]", iHostId, pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(), 
                    pEventInfo->pPeerInfo->GetRemotePort());
                m_pMonitor->UpdateState(iHostId, E_MDB_STATE_ABNORMAL);
            }
        }

        //ɾ����Ӧ�Ŀͻ�����Ϣ
        RemoveClient(pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(), pEventInfo->pPeerInfo->GetRemotePort());

        TADD_FUNC("Finish.");
        return bRet;
    }

    bool TMdbServerEngine::OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump)
    {
        TADD_FUNC("Start.");

        bool bRet = true;
        TMdbWinntTcpHelper* pWinntcpHelper = static_cast<TMdbWinntTcpHelper*>(pEventInfo->pPeerInfo->GetHelper());
        TMdbWinntTcpMsg* pWinntTcpMsg = static_cast<TMdbWinntTcpMsg*>(pEventInfo->pMsgInfo);
        const char* pcBodyBuffer = pWinntTcpMsg->GetBody();
        int iBodyLength = pWinntTcpMsg->GetBodyLength();
        switch (pWinntTcpMsg->oMsgHead.type)
        {
        case MDB_TCP_CHECK_TYPE://������
            {
                TADD_DETAIL("Recv check package from [%s:%d]\n", pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(), pEventInfo->pPeerInfo->GetRemotePort());
            }
            break;
        case MDB_ASYN_EVENT:
        case MDB_SYNC_EVENT:
            {
                TADD_NORMAL("Recv event[%u],length[%d]:%s\n", pWinntTcpMsg->oMsgHead.event, iBodyLength, pcBodyBuffer);
                DealEvent(pWinntcpHelper, pWinntTcpMsg);
            }
            break;
        default:
            {
                TADD_ERROR(ERR_NET_RECV_DATA_FORMAT, "Unknown Msg type\n");
                bRet = false;
            }
            break;
        }

        TADD_FUNC("Finish.");
        return bRet;
    }

    int TMdbServerEngine::DealEvent(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pWinntcpHelper);
        CHECK_OBJ(pWinntTcpMsg);
        switch(pWinntTcpMsg->oMsgHead.event)
        {
        case E_MDB_REGIST://ע����Ϣ IP|port
            {
                DealRegist(pWinntcpHelper, pWinntTcpMsg);
                break;
            }
        case E_MDB_STAT_CHANGE://mdb�ϱ�״̬��Ϣ, ��ʽ��HostID|state
            {
                DealStateChange(pWinntcpHelper, pWinntTcpMsg, true);
                break;
            }
        case E_MDB_ROUTING_REQUEST://·����������, ��ʽ��HostID|Is-recovery
            {
                DealRoutingRequest(pWinntcpHelper, pWinntTcpMsg);
                break;
            }
        case E_REP_STAT_CHANGE://����ͬ��״̬��Ϣ����ʽ��HostID|state
            {
                DealStateChange(pWinntcpHelper, pWinntTcpMsg, false);
                break;
            }
        case E_REP_SYNC_CONFIG_REQUEST://ͬ�������ļ�
            {
                DealRepConfigSync(pWinntcpHelper);
                break;
            }
        case  E_REP_SYNC_STATE_REQUEST://ͬ��״̬��Ϣ
            {
                DealRepStateSync(pWinntcpHelper);
                break;
            }
        case E_REP_ROUTING_CHANGE: //����·����Ϣ�������ʽ�� HostID|Add or Delete| routing_list
            {
                DealRoutingChange(pWinntTcpMsg, false);
                break;
            }
        case  E_SVR_ROUTING_CHANGE://·����Ϣ����� ��ʽ�� HostID|Add or Delete| routing_list
            {
                DealRoutingChange(pWinntTcpMsg, true);
                break;
            }
        case E_SVR_ROUTING_INFO://��ѯ����·����Ϣ
            {
                DealRoutingQuery(pWinntcpHelper, pWinntTcpMsg);
                break;
            }
        default:
            {
                CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "Unknown event type [%d]", pWinntTcpMsg->oMsgHead.event);
            }
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  DealRegist
    * ��������	: ����ע����Ϣ�� �ظ���ʽ�� HostID|hearbeatWarning
    * ����		: 
    * ����		:  IsRep �Ƿ���Ҫͬ�������������Ա�����״̬�����Ϣ����Ҫͬ��������
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerEngine::DealRegist(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        const char* pMsg = pWinntTcpMsg->GetBody();
        TMdbNtcSplit tSplit;
        tSplit.SplitString(pMsg, '|');
        if (tSplit.GetFieldCount() != 2)
        {
            CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "Invalid Msg format for Register, msg = [%s]", pMsg);
        }

        int iHostID = m_pConfig->GetHostID(tSplit[0], atoi(tSplit[1]));
        char sMsg[MAX_REP_SEND_MSG_LEN];
        snprintf(sMsg, MAX_REP_SEND_MSG_LEN, "%d|%d", iHostID, m_iHeartbeatWarning);
        //����ע�᷵����Ϣ
        if (pWinntcpHelper->SendPacket(/*QuickMDB::*/MDB_SYNC_EVENT, E_MDB_REGIST_ANS, sMsg, strlen(sMsg)))
        {
            TADD_NORMAL("Send Register answer to host[%d] OK.", iHostID);
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send Register answer to host[%d] failed..", iHostID);
        }

        CHECK_RET(SetHostID(pWinntcpHelper->pPeerInfo->GetRemoteAddrInfo().c_str(), pWinntcpHelper->pPeerInfo->GetRemotePort(), iHostID), "Get host ID failed.");
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  DealStateChange
    * ��������	: ����״̬�����Ϣ, ��ʽ��HostID|state
    * ����		: 
    * ����		:  IsRep �Ƿ���Ҫͬ�������������Ա�����״̬�����Ϣ����Ҫͬ��������
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerEngine::DealStateChange(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg, bool IsRep /* = true */)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        const char* pMsg = pWinntTcpMsg->GetBody();
        TMdbNtcSplit tSplit;
        tSplit.SplitString(pMsg, '|');
        if (tSplit.GetFieldCount() != 2)
        {
            CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "Invalid Msg format for MDB state change, msg = [%s]", pMsg);
        }
        int iHostID = atoi(tSplit[0]);
        if (iHostID < 0)
        {
            CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "Invalid HOST_ID[%d]", iHostID);
        }
        EMdbRepState eState = (EMdbRepState)atoi(tSplit[1]);
        if (eState<=E_MDB_STATE_FIRST ||  eState >= E_MDB_STATE_LAST)
        {
            CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "Invalid STATE[%d]", eState);
        }

        //�������÷����״̬
        m_pMonitor->UpdateState(iHostID, eState);
        //֪ͨ���÷��񱸻�״̬�仯
        if (IsRep)
        {
            m_pClient->SendStatChange(pMsg);
        }

        CHECK_RET(SetHostID(pWinntcpHelper->pPeerInfo->GetRemoteAddrInfo().c_str(), pWinntcpHelper->pPeerInfo->GetRemotePort(), iHostID), "Get host ID failed.");

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  DealRoutingRequest
    * ��������	: ����·��������Ϣ, ��ʽ��HostID, �ظ���ʽ��routing_id,routing_id...|routing_Id@ip:port;ip:port%routing_id@ip_port ...|routing_id@ip:port...
    * ����		: 
    * ����		:  ;2:1;4:4||2:[100,200];4:[1,9]|1:10.45.4.41,19801;4:10.45.4.41,1980
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerEngine::DealRoutingRequest(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        const char* pMsg = pWinntTcpMsg->GetBody();
        int iHostID = atoi(pMsg);
        char sMsg[MAX_REP_SEND_BUF_LEN];
        CHECK_RET(m_pConfig->GetRoutingRequestData(iHostID, MAX_REP_SEND_BUF_LEN, sMsg), "GetRoutingRequestData failed.");

        //����·��������Ϣ
        pWinntcpHelper->SendPacket(/*QuickMDB::*/MDB_SYNC_EVENT, E_MDB_ROUTING_REQUEST_ANS, sMsg, strlen(sMsg));

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  DealRoutingQuery
    * ��������	: ·����Ϣ��ѯ
    * ����		: 
    * ����		: 
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerEngine::DealRoutingQuery(TMdbWinntTcpHelper* pWinntcpHelper, TMdbWinntTcpMsg* pWinntTcpMsg)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        const char* pMsg = pWinntTcpMsg->GetBody();
        TMdbNtcSplit tSplit;
        tSplit.SplitString(pMsg, '|');
        if (tSplit.GetFieldCount() != 2)
        {
            CHECK_RET(ERR_NET_RECV_DATA_FORMAT, "Invalid Msg format for MDB state change, msg = [%s]", pMsg);
        }
        char sSendBuf[MAX_REP_SEND_BUF_LEN];
        eQueryType eType = (eQueryType)atoi(tSplit[0]);
        int iID = atoi(tSplit[1]);
        switch(eType)
        {
        case E_QUERY_ROUTING:
            {
                iRet = m_pMonitor->GetRoutingInfo(iID, sSendBuf);
                //CHECK_RET(m_pMonitor->GetRoutingInfo(iID, sSendBuf), "GetRoutingInfo failed.");
                break;
            }
        case  E_QUERY_HOST:
            {
                iRet = m_pMonitor->GetHostInfo(iID, sSendBuf);
                //CHECK_RET(m_pMonitor->GetHostInfo(iID, sSendBuf), "GetHostInfo failed.");
                break;
            }
        case  E_QUERY_GROUP:
            {
                iRet = m_pMonitor->GetGroupInfo(iID, sSendBuf);
                //CHECK_RET(m_pMonitor->GetGroupInfo(iID, sSendBuf), "GetGroupInfo failed.");
                break;
            }
        default:
            {
                CHECK_RET(ERROR_UNKNOWN, "Unknown Msg type.");
            }
        }

        TADD_NORMAL("Send Msg back.");
        //���Ͳ�ѯ��Ϣ
        if (pWinntcpHelper->SendPacket(/*QuickMDB::*/MDB_SYNC_EVENT, E_SVR_ROUTING_INOF_ANS, sSendBuf, strlen(sSendBuf)))
        {
            TADD_NORMAL("Send Msg OK.");
        }
        else
        {
            CHECK_RET(ERR_NET_SEND_FAILED, "Send Msg failed.");
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  DealStateChange
    * ��������	: ����״̬�����Ϣ
    * ����		: 
    * ����		:  IsRep �Ƿ���Ҫͬ�������������Ա�����״̬�����Ϣ����Ҫͬ��������
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerEngine::DealRoutingChange(TMdbWinntTcpMsg* pWinntTcpMsg, bool IsRep)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  DealRepConfigSync
    * ��������	: ������ͬ�������ļ�����
    * ����		: 
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerEngine::DealRepConfigSync(TMdbWinntTcpHelper* pWinntcpHelper)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sMsg[MAX_REP_SEND_BUF_LEN];
        sMsg[0] = '\0';
        CHECK_RET(m_pConfig->GetConfigData(sMsg), "GetConfigData failed.");
        pWinntcpHelper->SendPacket(/*QuickMDB::*/MDB_SYNC_EVENT, E_REP_SYNC_CONFIG_REQUEST_ANS, sMsg, strlen(sMsg));
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  DealStateChange
    * ��������	: ������ͬ��״̬��Ϣ����
    * ����		: 
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerEngine::DealRepStateSync(TMdbWinntTcpHelper* pWinntcpHelper)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sMsg[MAX_REP_SEND_BUF_LEN];
        sMsg[0] = '\0';
        CHECK_RET(m_pMonitor->GetStateData(sMsg, MAX_REP_SEND_BUF_LEN), "GetStateData failed.");
        pWinntcpHelper->SendPacket(/*QuickMDB::*/MDB_SYNC_EVENT, E_REP_SYNC_STATE_REQUEST_ANS, sMsg, strlen(sMsg));

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbServerEngine::SetHostID(const char* sIP, int iPort, int iHostID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        for (unsigned int i = 0; i<m_arClientHost.size(); i++)
        {
            if (iPort == m_arClientHost[i].m_iPort &&/*QuickMDB::*/TMdbNtcStrFunc::StrNoCaseCmp(sIP, m_arClientHost[i].m_strIP.c_str()) == 0)
            {
                m_arClientHost[i].m_iHostID = iHostID;
                TADD_FUNC("Finish.");
                return iRet;
            }
        }

        TADD_ERROR(ERR_APP_INVALID_PARAM, "Connect [%s:%d] not found.", sIP, iPort);
        TADD_FUNC("Finish.");
        return ERR_APP_INVALID_PARAM;
    }

    int TMdbServerEngine::GetHostID(const char* sIP, int iPort)
    {
        TADD_FUNC("Start.");

        for (unsigned int i = 0; i<m_arClientHost.size(); i++)
        {
            if (iPort == m_arClientHost[i].m_iPort &&/*QuickMDB::*/TMdbNtcStrFunc::StrNoCaseCmp(sIP, m_arClientHost[i].m_strIP.c_str()) == 0)
            {
                return m_arClientHost[i].m_iHostID;
            }
        }

        TADD_ERROR(ERR_APP_INVALID_PARAM, "Connect [%s:%d] not found.", sIP, iPort);
        TADD_FUNC("Finish.");
        return -1;
    }

    int TMdbServerEngine::RemoveClient(const char* sIP, int iPort)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        std::vector<TMdbRepHost>::iterator itor = m_arClientHost.begin();
        for (; itor != m_arClientHost.end(); ++itor)
        {
            if (iPort == itor->m_iPort &&/*QuickMDB::*/TMdbNtcStrFunc::StrNoCaseCmp(sIP, itor->m_strIP.c_str()) == 0)
            {
                m_arClientHost.erase(itor);
                return iRet;
            }
        }

        TADD_ERROR(ERR_APP_INVALID_PARAM, "Connect [%s:%d] not found.", sIP, iPort);
        TADD_FUNC("Finish.");
        return -1;
    }
    TMdbServerCenter::TMdbServerCenter(): m_pConfig(NULL), m_pCfgServer(NULL), m_pRepClient(NULL), m_pStatMonitor(NULL), m_pServerShm(NULL), 
        m_pShmMgr(NULL)
    {

    }

    TMdbServerCenter::~TMdbServerCenter()
    {
        SAFE_DELETE(m_pConfig);
        SAFE_DELETE(m_pCfgServer);
        SAFE_DELETE(m_pRepClient);
        SAFE_DELETE(m_pStatMonitor);
        SAFE_DELETE(m_pShmMgr);
    }

    /******************************************************************************
    * ��������	:  Init
    * ��������	: ��ʼ�����÷���
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerCenter::Init()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        bool bRepConn = false;//�뱸���Ƿ����ӳɹ�
        if (TMdbNtcShareMem::CheckExist(MDB_SERVER_SHM_NAME, ENV_QMDB_HOME_NAME))
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM, "mdbServer is running. Please stop it first." );
            return ERR_APP_INVALID_PARAM;
        }

        //��1����ȡ�����ļ�
        m_pConfig = new (std::nothrow) TMdbServerConfig();
        CHECK_OBJ(m_pConfig);
        CHECK_RET(m_pConfig->ReadConfig(),"Read MDB server configuration file error.");

        //��2����ʼ��״̬���
        m_pStatMonitor = new(std::nothrow) TMdbStatMonitor();
        CHECK_OBJ(m_pConfig);
        CHECK_RET(m_pStatMonitor->Init(m_pConfig), "TMdbStatMonitor init failed.");

        //��3���뱸��ͨ��
        m_pRepClient = new(std::nothrow) TMdbServerClient();
        CHECK_OBJ(m_pRepClient);
        CHECK_RET(m_pRepClient->Init(m_pConfig, m_pStatMonitor), "TMdbServerClient init failed.");
        if (m_pRepClient->IsRepExist())//�����ļ��д������÷��񱸻�
        {
            if (m_pRepClient->Connect())//���ӶԶ˳ɹ�
            {
                bRepConn = true;
                //�ӶԶ˻�ȡ�͸��±��������ļ���·����Ϣ
                CHECK_RET(m_pRepClient->SyncLocalCfgFile(),"Synchronize configuration file failed.");

                //�ӶԶ˻�ȡ�͸��������Ѿ����ӵ�QuickMDB״̬
                CHECK_RET(m_pRepClient->SyncLocalMdbState(), "Synchronize QuickMDB state failed.");
            }
        }

        //��4�����ݻ�ȡ����Ϣ����ʼ�����÷���
        m_pCfgServer = new(std::nothrow) TMdbServerEngine();
        CHECK_OBJ(m_pCfgServer);
        CHECK_RET(m_pCfgServer->Init(m_pStatMonitor, m_pRepClient, m_pConfig), "Init TMdbConfigServer failed.");

        //��5�����������Ϣ�����ڴ�
        TMdbNtcShareMem *pShmMgr= new(std::nothrow) TMdbNtcShareMem(MDB_SERVER_SHM_NAME, sizeof(TMDBServerShm), ENV_QMDB_HOME_NAME);
        CHECK_OBJ(pShmMgr);
        if (pShmMgr->IsOK())
        {
            m_pServerShm = (TMDBServerShm*)pShmMgr->GetBuffer();
            CHECK_OBJ(m_pServerShm);
            TMdbDateTime::GetCurrentTimeStr(m_pServerShm->sStartTime);
            if (bRepConn)
            {
                TMdbDateTime::GetCurrentTimeStr(m_pServerShm->sRepCnnTime);
            }
            m_pServerShm->bExit = false;
            m_pServerShm->bIsRepExist = m_pRepClient->IsRepExist();   
            m_pServerShm->iPid = TMdbOS::GetPID();
            //TADD_NORMAL("%s, %s, %s, %d", m_pServerShm->sStartTime, m_pServerShm->bExit?"YES":"NO", m_pServerShm->bIsRepExist?"YES":"NO", m_pServerShm->iPid);
        }
        else
        {
            CHECK_RET(ERR_OS_CREATE_SHM, "Create shared memory failed.");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  Start
    * ��������	: �����������ȴ��ʹ�����������Ϣ
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbServerCenter::Start()
    {
        int iRet = 0;
        //������������, 2�������߳�
        if (m_pCfgServer->Start(m_pConfig->GetLocalHost()->m_strIP.c_str(), m_pConfig->GetLocalHost()->m_iPort, 2, m_pConfig->GetHearbeatWarning()))
        {
            TADD_NORMAL("Start mdbServer OK.");
        }
        else
        {
            TADD_ERROR(ERR_NET_LISTEN_PORT, "Start to listen failed.");

            //TMdbNtcShareMem::Destroy(MDB_SERVER_SHM_NAME);//�����Ѿ������Ĺ����ڴ�

            return ERR_NET_LISTEN_PORT;
        }
        TRepNTCMsg tMsg;
        while(!m_pServerShm->bExit)
        {   
            // TADD_NORMAL("%s, %s, %s, %d", m_pServerShm->sStartTime, m_pServerShm->bExit?"YES":"NO", m_pServerShm->bIsRepExist?"YES":"NO", m_pServerShm->iPid);
            //����������÷��񱸻�
            if (m_pRepClient->IsRepExist())
            {
                if (m_pRepClient->NeedReconnect())//�����Ƿ���Ҫ����
                {
                    if (m_pRepClient->Connect())
                    {
                        TADD_NORMAL("Connect to standby server OK.");
                        TMdbDateTime::GetCurrentTimeStr(m_pServerShm->sRepCnnTime);
                    }
                    else
                    {
                        TADD_WARNING_NO_SCREEN("Connect to standby server failed.");
                    }
                }

                if (E_REP_NTC_OK == m_pRepClient->m_eState)//�������ӣ�ִ�б�������
                {
                    iRet = m_pRepClient->GetMsg(tMsg);
                    if (iRet == 0)
                    {
                        m_pRepClient->DealRepServerCmd(tMsg);
                    }    
                }
            }
            TMdbNtcDateTime::Sleep(100);
        }

        TADD_NORMAL("mdbServer stop OK.");
        return 0;
    }

    int TMdbServerCenter::Stop()
    {
        int iRet = 0;
        if (TMdbNtcShareMem::CheckExist(MDB_SERVER_SHM_NAME, ENV_QMDB_HOME_NAME))
        {
            TMdbNtcShareMem *pShmMgr = new TMdbNtcShareMem(MDB_SERVER_SHM_NAME, 0, ENV_QMDB_HOME_NAME);
            CHECK_OBJ(pShmMgr);
            if (pShmMgr->IsOK())
            {
                m_pServerShm = (TMDBServerShm *)pShmMgr->GetBuffer();
                TADD_NORMAL("Attach TShareMem[%s] OK.", MDB_SERVER_SHM_NAME);
                TADD_NORMAL("Stop mdbServer(Start time[%s]) ...", m_pServerShm->sStartTime);
                m_pServerShm->bExit = true;

                //TADD_NORMAL("%s, %s, %s, %d", m_pServerShm->sStartTime, m_pServerShm->bExit?"YES":"NO", m_pServerShm->bIsRepExist?"YES":"NO", m_pServerShm->iPid);

                TMdbNtcDateTime::Sleep(5000);
                if (TMdbOS::IsProcExistByKill(m_pServerShm->iPid))
                {
                    TMdbOS::KillProc(m_pServerShm->iPid);
                    TADD_NORMAL("mdbServer is stopped by kill command.");
                }
                TMdbNtcShareMem::Destroy(MDB_SERVER_SHM_NAME, ENV_QMDB_HOME_NAME);
                TADD_NORMAL("mdbServer stop OK.");
            }
            else
            {
                iRet = ERR_OS_ATTACH_SHM;
                TADD_ERROR(ERR_OS_ATTACH_SHM, "Attach TShareMem[%s] Failed. ErrorNo=%d, ErrorTest = %s",MDB_SERVER_SHM_NAME, pShmMgr->GetErrorNo(), pShmMgr->GetErrorText().c_str());
            }
            SAFE_DELETE(pShmMgr);
        }   
        else
        {
            TADD_NORMAL("mdbServer is not running.");
        }
        return iRet;
    }
//}

