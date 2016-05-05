/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbFlushFromOra.cpp		
*@Description： 内存数据库的Oracle刷新程序（从Oracle写入内存数据库）
*@Author:		li.shugang
*@Date：	    2009年05月11日
*@History:
******************************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "Helper/TThreadLog.h"
#include "Dbflush/mdbChangeNotify.h"

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
    if(argc != 2|| strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        printf("-------\n"
        		" Usage:\n"
        		"   %s <sDsn> \n"
        		"   %s [ -H | -h ] \n"
        		" Example:\n" 
        		"   %s Check \n"              
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
    TADD_START(argv[1], sName, 0,false ,true);
    try
    {
        TMdbChangeNotify notify;
        if(notify.Init(argv[1], sName) < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"TMdbChangeNotify:init() faild");
            return 0;
        }
        notify.Start();
        
    }
    catch(TMdbException& e)
    {
        printf("ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
    }
    catch(TMDBDBExcpInterface& e)
    {
        printf("ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
    }
    catch(...)
    {
        printf("Unknown error!\n");       
    }
    
    return 0;   
}


