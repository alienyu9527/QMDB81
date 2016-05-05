/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbSetLog.cpp
*@Description： 内存数据库的设置各种日志级别
*@Author:		li.shugang
@Modify :       jiang.mingjun
*@Date：	    2009年04月23日
*@History:      提供统一的帮助说明风格
******************************************************************************************/


#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "Helper/TThreadLog.h"
#include "Control/mdbLogInfo.h"
#include "Interface/mdbQuery.h"

#include "Helper/mdbCommandlineParser.h"
//#include "Helper/mdbProcess.h"

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

//namespace QuickMDB{

struct ST_SETLOG_PARAM
{
    char sDsn[256];
    char caLevel[2];
    int iPid;
    int iLogLevel;
    ST_SETLOG_PARAM()
    {
        memset(caLevel,0,sizeof(caLevel));
        memset(sDsn, 0, sizeof(sDsn));
        iPid = -1;
        iLogLevel = -1;
    }
};

//检查参数
int CheckParam(int argc, char* argv[],ST_SETLOG_PARAM & stSetLogParam)
{
    CommandLineParser clp(argc, argv);
    clp.set_check_condition("-p", 2);
    clp.set_check_condition("-H", 0);
    clp.set_check_condition("-h", 0);
    clp.set_check_condition("-c", 1);
    if( clp.check() == false)
    {
        return -1;
    }
    TMdbLogInfo info;
    const vector<CommandLineParser::OptArgsPair>& pairs = clp.opt_args_pairs();
    vector<CommandLineParser::OptArgsPair>::const_iterator it;
    for (it = pairs.begin(); it != pairs.end(); ++it)
    {
        const string& opt = (*it)._first;
        const vector<string>& args = (*it)._second;
        if (opt == "-p")
        {
            stSetLogParam.iPid = atoi(args[0].c_str());
            stSetLogParam.iLogLevel = atoi(args[1].c_str());
            continue;
        }
        if(opt == "-h" || opt == "-H")
        {
            return -1;
        }
        if(opt == "-c")
        {
            SAFESTRCPY(stSetLogParam.sDsn,sizeof(stSetLogParam.sDsn),args[0].c_str());
            stSetLogParam.sDsn[strlen(stSetLogParam.sDsn)] = '\0';
            continue;
        }
    }
    if(argc == 1 || strlen(stSetLogParam.sDsn) == 0 || stSetLogParam.iPid <= 0)
    {
        return -1;
    }
    return 0;
}

//}

//using namespace QuickMDB;

void Help(int argc, char* argv[])
{
    printf("-------\n"
        " Usage:\n"
        "   %s -c <sDsn> -p {<process-id> <log-level>}  \n"
        "   %s [ -H | -h ] \n"
        " Example:\n"
        "   %s \n"
        "      -c ocs   -p 2345 3 \n"
        " Note:\n"
        "      -c data source .\n"
        "      -p set pid process-id log-level .\n"
        "      -H|-h Print Help.\n"
        "-------\n" ,argv[0],argv[0],argv[0]);
}


int main(int argc, char* argv[])
{
    ST_SETLOG_PARAM stSetLogParam;
    if(CheckParam(argc,argv,stSetLogParam) != 0)
    {
        Help(argc,argv);
        return 0;
    }
    int iRet = 0;
    TMdbLogInfo info;
    TADD_START(stSetLogParam.sDsn,"mdbSetLog", 0, true,false);
    //首先解析出数据库名称，然后建立链接，再重新解析参数
    CHECK_RET(info.Connect(stSetLogParam.sDsn),"Can't connect to minidb=[%s].",stSetLogParam.sDsn);
    CHECK_RET(info.SetProc(stSetLogParam.iPid, stSetLogParam.iLogLevel),"SetProc(%d, %d) failed.",
                                stSetLogParam.iPid, stSetLogParam.iLogLevel);
    printf("SetProc LogLevel ( ProcessId: %d, LogLevel: %d)  success!!\n",stSetLogParam.iPid, stSetLogParam.iLogLevel);
    return iRet;
}


