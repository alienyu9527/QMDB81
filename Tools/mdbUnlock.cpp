/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbUnlock.cpp
*@Description： mdb解锁工具
*@Author:	jin.shaohua
*@Date：	    2013.6
*@History:
******************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Helper/TThreadLog.h"
#include "Tools/mdbMutexCtrl.h"
#include "Helper/mdbCommandlineParser.h"
#ifdef WIN32
#pragma comment(lib, "Interface.lib")
#pragma comment(lib, "Helper.lib")
#pragma comment(lib, "Tools.lib")
#pragma comment(lib, "Control.lib")
#pragma comment(lib, "Monitor.lib")
#pragma comment(lib, "DataCheck.lib")
#pragma comment(lib, "OracleFlush.lib")
#pragma comment(lib, "Agent.lib")
#pragma comment(lib, "Replication.lib")
#endif


void Help()
{
    static char usage[2048] =
        "-------\n"
        " Usage\n"
        "   mdbUnlock \n"
        "      -c <sDsn> -n\n"
        "                -t <tablename OR all> \n"
        "                -s <tablespace name OR all> \n"
        "                -q <MemSeq name OR all>\n"
        "                -d  \n"
        "                -p \n"
        "                -o \n"
        "                -r  \n"
        "                -c  \n"
        "                -i  \n"
        " Note\n"
        "      -c data source .\n"
        "      -t table name  .    \n"
        "      -s table-space name . \n"
        "      -p page .\n"
        "      -n dsn  . \n"
        "      -d data .\n"
        "      -q memseq .\n"
        "      -o  oracle shm . \n"
        "      -r  rep shm . \n"
        "      -c  caputre shm . \n"
        "      -i  index shm . \n"
        "-------\n" ;
    printf("%s",usage);
}

//using namespace QuickMDB;

//检查参数
int CheckParam(int argc, char* argv[],std::string & sDsn,std::string & sName,std::string & sOper)
{
    CommandLineParser clp(argc, argv);
    clp.set_check_condition("-c", 1);
    clp.set_check_condition("-n", 0);
    //clp.set_check_condition("-t", 0, 1);
    //clp.set_check_condition("-s", 0, 1);
    //clp.set_check_condition("-q", 0, 1); 
    
    clp.set_check_condition("-t", 1);
    clp.set_check_condition("-s", 1);
    clp.set_check_condition("-q", 1);  
	
    clp.set_check_condition("-p", 0);
    clp.set_check_condition("-d", 0);  
    clp.set_check_condition("-o", 0);  
    clp.set_check_condition("-r", 0);  
    clp.set_check_condition("-c", 0);  
    clp.set_check_condition("-i", 0);  
    if(clp.check() == false)
    {
        return -1;
    }
    const vector<CommandLineParser::OptArgsPair>& pairs = clp.opt_args_pairs();
    vector<CommandLineParser::OptArgsPair>::const_iterator it;
    for (it = pairs.begin(); it != pairs.end(); ++it)
    {
        const string& opt = (*it)._first;
        const vector<string>& args = (*it)._second;
        if(opt == "-c")
        {
            sDsn = args[0];
        }
        else if("-t" == opt || "-s" == opt || "-q" == opt)
        {
        	
            sName = args[0];
            sOper = opt;
        }
        else
        {
            sOper = opt;
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int iRet = 0;
    std::string sDsn = "";
    std::string sName = "";
    std::string sOper = "";
    if(argc < 3 || 0 != CheckParam(argc, argv ,sDsn,sName,sOper))
    {
        Help();
        return 0;
    }
    TADD_OFFSTART(sDsn.c_str(),"mdbUnlock",0, true);
    TADD_NORMAL("CommandLine = [mdbUnlock -c %s %s %s].",sDsn.c_str(),sOper.c_str(),sName.c_str());
    TMdbMutexCtrl tMutexCtrl;
    CHECK_RET(tMutexCtrl.Init(sDsn.c_str()),"tMutexCtrl.Init faild.");
    switch(sOper.at(1))
    {
        case 'n':
             tMutexCtrl.RenewDsnMutex();
            break;
      	case 't':
   			if(TMdbNtcStrFunc::StrNoCaseCmp(sName.c_str(),"all") == 0)
    			tMutexCtrl.RenewTableMutex();
   			else
             	tMutexCtrl.RenewTableMutex(sName.c_str());
            break;
        case 's':
   			if(TMdbNtcStrFunc::StrNoCaseCmp(sName.c_str(),"all") == 0)
    			tMutexCtrl.RenewTablespaceMutex();
   			else
             	tMutexCtrl.RenewTablespaceMutex(sName.c_str());
            break;
        case 'q':
   			if(TMdbNtcStrFunc::StrNoCaseCmp(sName.c_str(),"all") == 0)
    			tMutexCtrl.RenewSeqMutex();
   			else
             	tMutexCtrl.RenewSeqMutex(sName.c_str());
            break;
        case 'p':
            tMutexCtrl.RenewPageMutex();
            break;
        case 'd':
            tMutexCtrl.RenewDataMutex();
            break;
        case 'o':
            tMutexCtrl.RenewSyncAreaMutex(SA_ORACLE);
            break;
        case 'r':
            tMutexCtrl.RenewSyncAreaMutex(SA_REP);
            break;
        case 'c':
            tMutexCtrl.RenewSyncAreaMutex(SA_CAPTURE);
            break;
        case 'i':
            tMutexCtrl.RenewIndexMutex();
            break;
        default:
             {
                TADD_ERROR(ERROR_UNKNOWN,"not suppor oper[%s]",sOper.c_str());
             }
             break;

    }
    return iRet;
}

