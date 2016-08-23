/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbRepClient.cpp		
*@Description： 分片备份主备同步客户端
*@Author:		jiang.lili
*@Date：	    2014/03/20
*@History:
******************************************************************************************/
#include "stdio.h"
#include "stdlib.h"

#include "Helper/TThreadLog.h"
#include "Interface/mdbQuery.h"
#include "Replication/mdbRepClientCtrl.h"
//using namespace QuickMDB;

int main(int argc, char* argv[])
{   
    //Process::SetProcessSignal();
    int iRet = 0;
    if(argc != 3 || strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        printf("-------\n"
        		" Usage:\n"
        		"   %s <sDsn> <iRepHostID>\n"
        		"   %s [ -H | -h ] \n"
        		" Example:\n" 
        		"   %s ocs 1\n"              
        		" Note:\n"
        		"     <sDsn>: data source\n"
        		"     <iRepHostID>: shard buck-up host id\n"
        		"     -H|-h Print Help.\n"
        		"-------\n",argv[0],argv[0],argv[0]);
        return 0;
    }
#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); /* run in backgroud */
#endif
    
    
    char sName[MAX_NAME_LEN];
    memset(sName, 0, sizeof(sName));
	int iHostID = atoi(argv[2]);
    sprintf(sName, "%s %s %d", argv[0], argv[1], iHostID);
    TADD_START(argv[1], sName, 0,false ,true);
    
    try
    {
        TRepClient tRepClient;
        CHECK_RET(tRepClient.Init(argv[1], iHostID), "mdbRepClient init failed.");
        CHECK_RET(tRepClient.Start(), "mdbRepClient run failed.");
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

