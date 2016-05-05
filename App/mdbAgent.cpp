/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbAgent.cpp		
*@Description： 内存数据库的C-S模式代理程序
*@Author:		li.shugang
*@Date：	    2009年07月07日
*@History:
******************************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "Helper/TThreadLog.h"
#include "Agent/mdbAgentServer.h"
//#include "Helper/mdbProcess.h"

//#define SAFE_CLOSE(_fp) if(NULL != _fp){fclose(_fp);_fp=NULL;}

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

#ifdef _LINUX
void dump(int signo)
{
	char buf[1024] = {0};
    char cmd[1024] = {0};
    FILE *fh = NULL;

    snprintf(buf, sizeof(buf), "/proc/%d/cmdline", getpid());
    if(!(fh = fopen(buf, "r")))
    	exit(0);
    if(!fgets(buf, sizeof(buf), fh))
    	exit(0);
    SAFE_CLOSE(fh);
    if(buf[strlen(buf) - 1] == '\n')
    	buf[strlen(buf) - 1] = '\0';
    snprintf(cmd, sizeof(cmd), "gdb %s %d>>.coredump", buf, getpid());
    system(cmd);
    exit(0);
}
#endif

//using namespace QuickMDB;

int main(int argc, char* argv[])
{   
    int iRet = 0;
    //Process::SetProcessSignal();
    
    //#ifdef _LINUX	
    //signal(SIGSEGV, &dump);
    //#endif
	
    if(argc != 3)
    {   
    	printf("Invalid parameters.\n Exit!\n\n");            
        return 0;
    }
    
#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); // run in backgroud 
#endif    

#ifdef WIN32
	WSADATA wsaData;
	iRet = WSAStartup(MAKEWORD(2,2),&wsaData);
	if(iRet != 0)
	{
        TADD_ERROR("mdbagent::init socket faild");  
        return 0;
    }
#endif
    try
    {
        char sName[64];
        memset(sName, 0, sizeof(sName));

        snprintf(sName,sizeof(sName),"%s %s %s", argv[0], argv[1], argv[2]);
        TADD_START(argv[1],sName, 0, false ,true);
        
        int iAgentPort = TMdbNtcStrFunc::StrToInt(argv[2]);
		if(iAgentPort <= 0) 
		{
			TADD_ERROR(ERROR_UNKNOWN, "Invalid agent-port[%d]!", iAgentPort);
			return -1;
		}
		
        TMdbAgentServer Agent;
        if(Agent.Init(argv[1]) != 0)
            return -1;

        if(Agent.PreSocket(iAgentPort) < 0)
            return 0;

        iRet = Agent.StartServer(sName);
        if(iRet != 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"mdbagent run faild");  
        }
    }
    catch(TMdbException& e)
    {
        printf("ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
    }
    catch (TBaseException &e)
    {
    	printf("ERROR_MSG=%s\n",e.GetErrMsg());
    }
    catch(...)
    {
        printf("Unknown error!\n");       
    }
    //TADD_END();
    return 0;   
}


