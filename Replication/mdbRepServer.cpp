/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbRepServer.cpp		
*@Description： 分片备份主备同步服务端
*@Author:		jiang.lili
*@Date：	    2014/03/20
*@History:
******************************************************************************************/
#include "stdio.h"
#include "stdlib.h"

#include "Helper/TThreadLog.h"
#include "Interface/mdbQuery.h"
#include "Replication/mdbRepServerCtrl.h"
//using namespace QuickMDB;

int main(int argc, char* argv[])
{   

    //Process::SetProcessSignal();
    if(argc != 2 || strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        printf("-------\n"
        		" Usage:\n"
        		"   %s <sDsn> \n"
        		"   %s [ -H | -h ] \n"
        		" Example:\n" 
        		"   %s ocs\n"              
        		" Note:\n"
        		"     <sDsn>: data source\n"
        		"     -H|-h Print Help.\n"
        		"-------\n" ,argv[0],argv[0],argv[0]);
        return 0;
    }
#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); /* run in backgroud */
#endif
    
    int iRet = 0;
    char sName[MAX_NAME_LEN];
    memset(sName, 0, sizeof(sName));
    sprintf(sName, "%s %s", argv[0], argv[1]);
    TADD_START(argv[1],sName, 0,false,true);
    
    try
    {
        TRepServer tRepServer;
        CHECK_RET(tRepServer.Init(argv[1]), "RepServer Init failed.");
        CHECK_RET(tRepServer.Start(), "RepServer start failed.");
    }
    catch(TMdbException& e)
    {
        printf("ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
    }
    catch(...)
    {
        printf("Unknown error!\n");       
    }

    return 0;   
}

