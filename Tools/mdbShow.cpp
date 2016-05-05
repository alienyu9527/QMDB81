/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbShow.cpp
*@Description： 内存数据库的数据显示程序
*@Author:		li.shugang
*@Modify:       jiang.mingjun
*@Date：	    2009年03月10日
*@History:
******************************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Helper/TThreadLog.h"
#include "Tools/mdbInfo.h"
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

#define CHECK_IF_ALL(_args,_set)\
if(_args.size()== 0)\
{\
    SAFESTRCPY(_set,sizeof(_set),"all");\
}\
else\
{\
    SAFESTRCPY(_set,sizeof(_set),_args[0].c_str());\
}\
_set[strlen(_set)] = '\0';\
 

#define CHECK_TO_PRINT(_tocheck,_doall,_doone)\
if(strlen(_tocheck) !=0 )\
{\
    if(TMdbNtcStrFunc::StrNoCaseCmp(_tocheck, "all") == 0)\
    {\
       _doall;\
    }\
    else\
    {\
        _doone;\
    }\
}



// mdbShow参数
struct ST_SHOW_PARAM
{
    bool bMore;
    bool bShowDsn;
    char caLevel[2];
    int ilevel ;
    char sDsn[256];
    
    char sTable[256];
    char sTablespace[256];
    char sMem[256];
    char sProcess[256];
    char sLink[256];
    char sSeq[256];
    char sSQL[256];
    char sRouters[MAX_ROUTER_LIST_LEN];
    char sJob[256];//job
    char sUserName[MAX_NAME_LEN];//user
    bool bPrintLock;
    bool bPrintRouting;//是否打印路由信息
    bool bPrintNotLoadFromDB;//表不从数据库加载的附加配置信息
    ST_SHOW_PARAM()
    {
        memset(sDsn,        0, sizeof(sDsn));
        memset(caLevel,     0, sizeof(caLevel));
        memset(sTable,      0, sizeof(sTable));
        memset(sMem,        0, sizeof(sMem));
        memset(sProcess,    0, sizeof(sProcess));
        memset(sLink,       0, sizeof(sLink));
        memset(sSeq,        0, sizeof(sSeq));
        memset(sSQL,        0, sizeof(sSQL));
        memset(sTablespace, 0, sizeof(sTablespace));  
        memset(sRouters, 0, sizeof(sRouters));  
        memset(sJob, 0, sizeof(sJob));
        memset(sUserName,0,sizeof(sUserName));
        bMore = false;
        bShowDsn = false;
        ilevel = 0;
        bPrintLock = false;
        bPrintRouting = false;
        bPrintNotLoadFromDB = false;
    }
};

//检查参数
int CheckParam(int argc, char* argv[],ST_SHOW_PARAM & stShowParam)
{   
    bool bIsHelp = false;
    CommandLineParser clp(argc, argv);
    clp.set_check_condition("-t", 0, 1);//显示表信息
    clp.set_check_condition("-s", 0, 1);//显示表空间信息
    clp.set_check_condition("-l", 0, 1);//显示链接信息
    clp.set_check_condition("-p", 0, 1);//显示进程信息
    clp.set_check_condition("-m", 0, 1);//显示内存块信息
    clp.set_check_condition("-d", 0);     //是否显示详细信息
    clp.set_check_condition("-q", 0, 1);  //显示序列
    clp.set_check_condition("-n", 0);     //显示数据库信息
    //clp.set_check_condition("-Q", 0, 1);  //显示系统SQL
    clp.set_check_condition("-H", 0);
    clp.set_check_condition("-h", 0);
    clp.set_check_condition("-c", 1);//指定dsn
    clp.set_check_condition("-L", 1);//设置日志级别
    clp.set_check_condition("-r", 0,1);//显示捕获路由信息
    clp.set_check_condition("-j", 0,1);//显示job信息
    clp.set_check_condition("-u", 0,1);//显示用户信息
    clp.set_check_condition("-k", 0);//显示锁信息
    clp.set_check_condition("-f", 0);//显示路由备份信息
    clp.set_check_condition("-o", 0);//表不从数据库加载的附加配置信息
    
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
         if(opt == "-j")
        {
            CHECK_IF_ALL(args,stShowParam.sJob);
            continue;
        }
        if(opt == "-r")
        {
            CHECK_IF_ALL(args,stShowParam.sRouters);
            continue;
        }
        if (opt == "-t")
        {
            CHECK_IF_ALL(args,stShowParam.sTable);
            continue;
        }
        if(opt == "-s")
        {
            CHECK_IF_ALL(args,stShowParam.sTablespace);
            continue;
        }
        if(opt == "-m")
        {
            CHECK_IF_ALL(args,stShowParam.sMem);
            continue;
        }
        if(opt == "-p")
        {
            CHECK_IF_ALL(args,stShowParam.sProcess);
            continue;
        }
        if(opt == "-l")
        {
            CHECK_IF_ALL(args,stShowParam.sLink);
            continue;
        }
        if(opt == "-n")
        {
            stShowParam.bShowDsn = true;
            continue;
        }
        if(opt == "-q")
        {
            CHECK_IF_ALL(args,stShowParam.sSeq);
            continue;
        }
        /*if(opt == "-Q")
        {
            CHECK_IF_ALL(args,stShowParam.sSQL);
            continue;
        }*/
        if(opt == "-L") //日志级别
        {
            SAFESTRCPY(stShowParam.caLevel,sizeof(stShowParam.caLevel),args[0].c_str());
            stShowParam.caLevel[strlen(stShowParam.caLevel)]='\0';
            stShowParam.ilevel = atoi(stShowParam.caLevel);
            continue;
        }
        if(opt == "-c")
        {
            SAFESTRCPY(stShowParam.sDsn,sizeof(stShowParam.sDsn),args[0].c_str());
            stShowParam.sDsn[strlen(stShowParam.sDsn)] = '\0';
            continue;
        }
        if(opt == "-d")
        {
            stShowParam.bMore = true;
            continue;
        }
        if(opt == "-u")
        {
            CHECK_IF_ALL(args,stShowParam.sUserName);
            continue;
        }
        if(opt == "-k")
        {
            stShowParam.bPrintLock = true;
            continue;
        }
        if(opt == "-f")
        {
            stShowParam.bPrintRouting = true;
            continue;
        }
        if(opt == "-o")
        {
            stShowParam.bPrintNotLoadFromDB = true;
            continue;
        }
        
        bIsHelp = (opt == "-h" || opt == "-H");
        if(bIsHelp)
        {
            return -1;
        }
    }
    if(strlen(stShowParam.sDsn) == 0)
    {
        printf("Invalid command! Must specify a DSN by using '-c <sDsn>' for all the operation.\n");
        return -1;
    }
    return 0;
}

//}

//using namespace QuickMDB;

void Help(int argc, char* argv[])
{
    printf( "-------\n"
        " Usage\n"
        "   %s \n"
        "      -c <DSN> -n [-d]\n"
        "               -t <table-name OR all> [-d]\n"
        "               -s <table-space-name OR all> [-d] \n"
        "               -l <local OR remote OR all> [-d]\n"
        "               -p <process-name OR all> [-d]\n"
        "               -m <memory-id OR all> [-d] \n"
        "               -q <memory-sequence-name OR all> [-d]\n"
        "               -j <job-name OR all> \n"
        "               -u <user-name OR all> \n"
        "               -k \n"  
        "               -f \n"
        "               -r <router-list OR close> \n"
        "               -L <level>\n"
        "               -o \n"
        "               -H|h\n"
        " Example\n"
        "   %s \n"
        "      -c ocs   -t all -d \n"
        "      -c ocs   -t bal   \n"
        "      -c ocs   -s tab1-L 3 \n"
        "      -c ocs   -m 897979 -p QuickMDB -L 2 \n"
        "      -c ocs   -r 0,1,2,3 \n"
        "      -c ocs   -r \n"
        "      -c ocs   -r close\n"
        "      -c ocs   -n -d \n"
        "      -c ocs   -l all \n"
        " Note\n"
        "      -c specify a DSN.\n"
        "      -n show DSN information. \n"
        "      -t show table information.    \n"
        "      -s show table space information. \n"
        "      -l show links information. \n"
        "      -p show MDB process information.\n"
        "      -m show memory information. \n"
        "      -q show memory sequence information.\n"
        "      -j show job information.\n"
        "      -u show user information.\n"
        "      -k show all the locks information.\n"
        "      -f show local routing replication information.\n"
        "      -d to show more information.\n"
        "      -r start or stop capturing routing information. \n"
        "      -L set mdbShow's log level, level 0 default.\n"
        "      -o show tables not to load from database.\n"
        "      -H|-h print help information.\n"        
        "-------\n" ,argv[0],argv[0]);
}
    
int main(int argc, char* argv[])
{
    //Process::SetProcessSignal();
    int iRet = 0;
    ST_SHOW_PARAM stShowParam;
    if(argc == 1 || 0 != CheckParam(argc,argv,stShowParam))
    {
        Help(argc,argv);
        return 0;
    }
    TMdbInfo info(stShowParam.bMore);
    TADD_START(stShowParam.sDsn,"mdbShow", stShowParam.ilevel, true,false);
    printf("Version : %s\n",MDB_VERSION);
    CHECK_RET(info.Connect(stShowParam.sDsn),"Can't connect to DSN=[%s].",stShowParam.sDsn);
    printf("\n--------------------------*****SHOW-INFOMATION START*****--------------------------\n");

    if(stShowParam.bShowDsn == true)
    {
        info.PrintDSN(stShowParam.sDsn);
    }
    CHECK_TO_PRINT(stShowParam.sSeq,info.PrintSeq(),info.PrintSeq(stShowParam.sSeq));
    CHECK_TO_PRINT(stShowParam.sTable,info.PrintTable(),info.PrintTable(stShowParam.sTable));//table: 表
    CHECK_TO_PRINT(stShowParam.sTablespace,info.PrintTableSpace(),info.PrintTableSpace(stShowParam.sTablespace));//table-space: 表空间
    CHECK_TO_PRINT(stShowParam.sMem,info.PrintMem(),info.PrintMem(stShowParam.sMem));//memory: 内存
    CHECK_TO_PRINT(stShowParam.sProcess,info.PrintProc(),info.PrintProc(stShowParam.sProcess));//process: 进程
    //CHECK_TO_PRINT(stShowParam.sSQL,info.PrintSQL(),info.PrintSQL(atoi(stShowParam.sSQL)));//SQL: 系统SQL
    CHECK_TO_PRINT(stShowParam.sRouters,info.SetRouterToCapture(NULL),info.SetRouterToCapture(stShowParam.sRouters));//路由捕获
    CHECK_TO_PRINT(stShowParam.sJob,info.PrintJob(NULL),info.PrintJob(stShowParam.sJob));//job
    CHECK_TO_PRINT(stShowParam.sUserName,info.PrintUser(NULL),info.PrintUser(stShowParam.sUserName));//user info
    if(stShowParam.bPrintLock)//打印锁信息
    {
        info.PrintUsageOfLock();
    }
    if (stShowParam.bPrintRouting)//打印路由信息
    {
        info.PrintRoutingRep();
    }
    if(strlen(stShowParam.sLink) != 0) //link: 链接
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(stShowParam.sLink, "local") == 0)
        {
            info.PrintLink(1);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(stShowParam.sLink, "remote") == 0)
        {
            info.PrintLink(2);
        }
        else
        {
            info.PrintLink(0);
        }
    }
    if (stShowParam.bPrintNotLoadFromDB)//
    {
        info.PrintNotLoadFromDBInfo();
    }
    
    printf("\n---------------------------*****SHOW-INFOMATION END*****---------------------------\n");
    return iRet;
}


