/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbServerTools.cpp	
*@Description: mdbServer查询和命令发送工具
*@Author:		jiang.lili
*@Date：	    2015/01/28
*@History:
******************************************************************************************/
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbCommandlineParser.h"
//#include "Helper/mdbProcess.h"

#include "Tools/mdbServerTool.h"
#include "Replication/mdbServerConfig.h"

#ifdef WIN32
#pragma comment(lib,"Interface.lib")
#pragma comment(lib,"Helper.lib")
#pragma comment(lib,"Tools.lib")
#pragma comment(lib,"Control.lib")
#pragma comment(lib,"Monitor.lib")
#pragma comment(lib,"DataCheck.lib")
#pragma comment(lib,"OracleFlush.lib")
#pragma comment(lib,"Agent.lib")
#pragma comment(lib,"Replication.lib")
#endif

//namespace QuickMDB
//{
    TMdbServerTool::TMdbServerTool():m_pClient(NULL)
    {

    }

    TMdbServerTool::~TMdbServerTool()
    {
        //if (m_pClient!=NULL && m_pClient->IsStart())
        //{
        //    m_pClient->Disconnect();
        //}
        
        SAFE_DELETE(m_pClient);
    }
    
    int TMdbServerTool::Init()
    {
        int iRet = 0;
        //（1）读取配置文件
        TMdbServerConfig *pConfig = new(std::nothrow) TMdbServerConfig();
        CHECK_OBJ(pConfig);
        CHECK_RET(pConfig->ReadConfig(),"Read MDB server configuration file error.");
        pConfig->GetLocalHost();
        //（2）连接配置服务
        m_pClient = new (std::nothrow)TMdbRepNTCClient();
        CHECK_OBJ(m_pClient);
        if (!m_pClient->Connect(pConfig->GetLocalHost()->m_strIP.c_str(), pConfig->GetLocalHost()->m_iPort))
        {
            TADD_NORMAL("Connect to mdbServer failed. Check if mdbServer is started.");
        }

        SAFE_DELETE(pConfig);
        return iRet;
    }

    int TMdbServerTool::ExecCmd(const ST_SERVERTOOL_PARAM &stParam)
    {
        int iRet = 0;
        if (stParam.bHost)
        {
            ShowHost(stParam.iHostID);            
        }
        else if (stParam.bRouting)
        {
            ShowRouting(stParam.iRoutingID);
        }
        else if (stParam.bGroup)
        {
            ShowGroup(stParam.iGroupID);
        }
        else
        {
            printf("Invalid command.\n");
        }

        return iRet;
    }

    void TMdbServerTool::ShowRouting(int iRoutingID)
    {
        char sBuf[MAX_REP_SEND_MSG_LEN];
        snprintf(sBuf, MAX_REP_SEND_MSG_LEN, "%d|%d", E_QUERY_ROUTING, iRoutingID);
        if (!m_pClient->SendPacket(E_SVR_ROUTING_INFO, sBuf, strlen(sBuf)))
        {
            TADD_ERROR(ERR_NET_SEND_FAILED, "SendPacket to mdbServer failed.");
            return;
        }
        TRepNTCMsg tMsg;
        if (m_pClient->GetMsg(tMsg) == 0)
        {
            //TADD_NORMAL("MSG:[%s]", tMsg.ToString());
            printf("\n%s\n", tMsg.psMsg);
        }
        else
        {
            TADD_ERROR(ERR_NET_RECV_FAILED, "GetMsg failed.");
        }
    }

    void TMdbServerTool::ShowHost(int iHostID)
    {
        char sBuf[MAX_REP_SEND_MSG_LEN];
        snprintf(sBuf, MAX_REP_SEND_MSG_LEN, "%d|%d", E_QUERY_HOST, iHostID);
        if (!m_pClient->SendPacket(E_SVR_ROUTING_INFO, sBuf, strlen(sBuf)))
        {
            TADD_ERROR(ERR_NET_SEND_FAILED, "SendPacket to mdbServer failed.");
            return;
        }
        TRepNTCMsg tMsg;
        if (m_pClient->GetMsg(tMsg) == 0)
        {
            //TADD_NORMAL("MSG:[%s]", tMsg.ToString());
            printf("\n%s\n", tMsg.psMsg);
        }
        else
        {
            TADD_ERROR(ERR_NET_RECV_FAILED, "GetMsg failed.");
        }
        
    }

    void TMdbServerTool::ShowGroup(int iGroupID)
    {
        char sBuf[MAX_REP_SEND_MSG_LEN];
        snprintf(sBuf, MAX_REP_SEND_MSG_LEN, "%d|%d", E_QUERY_GROUP, iGroupID);
        if (!m_pClient->SendPacket(E_SVR_ROUTING_INFO, sBuf, strlen(sBuf)))
        {
            TADD_ERROR(ERR_NET_SEND_FAILED, "SendPacket to mdbServer failed.");
            return;
        }
        TRepNTCMsg tMsg;
        if (m_pClient->GetMsg(tMsg) == 0)
        {
            //TADD_NORMAL("MSG:[%s]", tMsg.ToString());
            printf("\n%s\n", tMsg.psMsg);
        }
        else
        {
            TADD_ERROR(ERR_NET_RECV_FAILED, "GetMsg failed.");
        }

    }

    int CheckParam(int argc, char* argv[],ST_SERVERTOOL_PARAM & stParam)
    {
        CommandLineParser clp(argc, argv);
        clp.set_check_condition("-r", 1);
        clp.set_check_condition("-m", 1);
        clp.set_check_condition("-g", 1);
        clp.set_check_condition("-h", 0);
        clp.set_check_condition("-H", 0);
         if( clp.check() == false)
        {
            return -1;
        }

        const vector<CommandLineParser::OptArgsPair>& pairs = clp.opt_args_pairs();
        vector<CommandLineParser::OptArgsPair>::const_iterator it;
        for (it = pairs.begin(); it != pairs.end(); ++it)
        {
            const string& opt = (*it)._first;
            const vector<string>& args = (*it)._second;
            if (opt == "-r")
            {
                stParam.bRouting = true;
                if (TMdbNtcStrFunc::StrNoCaseCmp(args[0].c_str(), "all")==0)
                {
                    stParam.iRoutingID = -1;
                }
                else
                {
                    stParam.iRoutingID = atoi(args[0].c_str());
                }
                continue;
            }
            if (opt == "-m")
            {
                stParam.bHost = true;
                if (TMdbNtcStrFunc::StrNoCaseCmp(args[0].c_str(), "all")==0)
                {
                    stParam.iHostID = -1;
                }
                else
                {
                    stParam.iHostID = atoi(args[0].c_str());
                }
                continue;
            }
            if (opt == "-g")
            {
                stParam.bGroup = true;
                if (TMdbNtcStrFunc::StrNoCaseCmp(args[0].c_str(), "all")==0)
                {
                    stParam.iGroupID = -1;
                }
                else
                {
                    stParam.iGroupID = atoi(args[0].c_str());
                }
                continue;
            }

            if(opt == "-h" || opt == "-H")
            {
                return -1;
            }         
        }

        return 0;
    }


//}

//using namespace QuickMDB;



void Help(int argc, char* argv[])
{
    printf("-------\n"
        " Usage:\n"
        "   %s -r <all | rule-id>\n"
        "   %s -m <all | host-id>\n"
        "   %s -g <all | group-id>\n"
        "   %s [ -H | -h ] \n"
        " Example:\n"
        "   %s \n"
        "      -r 100 \n"
        "      -m all \n"
        "      -g 1 \n"
        " Note:\n"
        "      -r Show the routing rules.\n"
        "      -m Show the host information.\n"
        "      -g Show the group information.\n"
        "-------\n", argv[0], argv[0], argv[0], argv[0], argv[0]);
}


int main(int argc, char* argv[])
{
    ST_SERVERTOOL_PARAM stParam;
    if(argc == 1 || CheckParam(argc,argv,stParam) != 0)
    {
        Help(argc,argv);
        return 0;
    }

    int iRet = 0;
    TADD_OFFSTART("OFFLINE",argv[0], 0,true);
    TMdbServerTool tTool;
    CHECK_RET(tTool.Init(), "mdbServerTool init failed.");
    CHECK_RET(tTool.ExecCmd(stParam), "mdbServer execute the command failed.");

    return iRet;
}