/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbFlushRep.cpp		
*@Description： 内存数据库的Replication刷新程序（从内存数据库把数据写入缓冲日志）
*@Author:		li.shugang
*@Date：	    2009年04月16日
*@History:
******************************************************************************************/


#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "Helper/TThreadLog.h"
#include "Interface/mdbQuery.h"
#include "Replication/mdbRepLog.h"
#include "Replication/mdbQueueLog.h"
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

int main(int argc, char* argv[])
{   

    //Process::SetProcessSignal();
    int iRet = 0;

    if(argc != 2|| strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        //TADD_ERROR("[%s : %d] : Less parameter.\n\t %s <DSN>", __FILE__, __LINE__, argv[0]); 
        printf("-------\n"
            " Usage:\n"
            "   %s <sDsn> \n"
            "   %s [ -H | -h ] \n"
            " Example:\n" 
            "   %s ocs \n"              
            " Note:\n"
            "     <sDsn>: data source.\n"
            "     -H|-h Print Help.\n"
            "-------\n",argv[0],argv[0],argv[0]);
        return 0;
    }

#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); /* run in backgroud */
#endif    
    char sName[64];
    memset(sName, 0, sizeof(sName));
    sprintf(sName, "%s %s", argv[0], argv[1]);
    TADD_START(argv[1],sName, 0, false,true);
    try
    {
        TMdbQueueLog tlog;
        /*if(tlog.Init(argv[1]) < 0){return 0;}
        tlog.Start();*/
        CHECK_RET(tlog.Init(argv[1]), "TMdbQueueLog init failed.");
        CHECK_RET(tlog.Start(), "TMdbQueueLog start failed.");        
    }
    catch(TMdbException& e)
    {
        printf("ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
    }
    catch(...)
    {
        printf("Unknown error!\n");       
    }

    return iRet;   
}


