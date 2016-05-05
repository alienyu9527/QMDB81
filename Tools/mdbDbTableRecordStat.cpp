/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbOraTableRecordStat.cpp
*@Description： 统计ORACLE侧与QMDB相关的表的记录数
*@Author:		cao.peng
*@Modify:       cao.peng
*@Date：	    2014年02月24日
*@History:
******************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Helper/TThreadLog.h"
#include "Tools/mdbCheckState.h"
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

//using namespace QuickMDB;

void Help(int argc, char* argv[])
{
    printf( "-------\n"
        " Usage\n"
        "   %s \n"
        "      -c <sDsn> -t <table name OR all>\n"
        " Example\n"
        "   %s \n"
        "      -c ocs   -t all \n"
        "      -c ocs   -t slpmd   \n"
        " Note\n"
        "      -c data source .\n"
        "      -t table name OR all .    \n"
        "-------\n" ,argv[0],argv[0]);
}

// mdbShow参数
struct ST_PARAM
{
    char sDsn[MAX_NAME_LEN];
    char sTable[MAX_FILE_NAME];
    ST_PARAM()
    {
        memset(sDsn,0, sizeof(sDsn));
        memset(sTable,0, sizeof(sTable));
    }
};

//检查参数
int CheckParam(int argc, char* argv[],ST_PARAM & stParam)
{   
    CommandLineParser clp(argc, argv);
    clp.set_check_condition("-t", 0, 1);
    clp.set_check_condition("-c", 1);
    if(!clp.check())
    {
        return -1;
    }
    const vector<CommandLineParser::OptArgsPair>& pairs = clp.opt_args_pairs();
    vector<CommandLineParser::OptArgsPair>::const_iterator it;
    for (it = pairs.begin(); it != pairs.end(); ++it)
    {
        const string& opt = (*it)._first;
        const vector<string>& args = (*it)._second;
        if (opt == "-t")
        {
            SAFESTRCPY(stParam.sTable,sizeof(stParam.sTable),args[0].c_str());
            continue;
        }
        if(opt == "-c")
        {
            SAFESTRCPY(stParam.sDsn,sizeof(stParam.sDsn),args[0].c_str());
            stParam.sDsn[strlen(stParam.sDsn)] = '\0';
            continue;
        }
    }
    if(strlen(stParam.sDsn) == 0)
    {
        printf("***Error the '-c <sDsn>' option agrument must have !!\n");
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    //Process::SetProcessSignal();
    int iRet = 0;
    ST_PARAM stParam;
    if(argc == 1 || 0 != CheckParam(argc,argv,stParam))
    {
        Help(argc,argv);
        return 0;
    }

    TADD_START(stParam.sDsn,"mdbOraTableRecordStat", 0,true,false);
    TTableRecordCount tTableRecordCount;
    CHECK_RET(tTableRecordCount.Init(stParam.sDsn),"Failed to init.");
    CHECK_RET(tTableRecordCount.RecordCount(stParam.sTable),"Failed to get the table record count.");
    return iRet;
}

