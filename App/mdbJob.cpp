/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbJob .cpp		
*@Description： mdb job程序
*@Author:	jin.shaohua
*@Date：	    2013.4
*@History:
******************************************************************************************/

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "Helper/TThreadLog.h"
//#include "Helper/mdbProcess.h"
#include "Control/mdbJobCtrl.h"
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

    //int iRet = 0;
    
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
    //Process::SetProcessSignal();
#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); /* run in backgroud */
#endif        
    char sName[64];
    memset(sName, 0, sizeof(sName));
    sprintf(sName, "%s %s", argv[0], argv[1]);
    TADD_START(argv[1],sName, 0,false ,true);
    int iRet = 0;
    TMdbJobCtrl tJobCtrl;
    CHECK_RET(tJobCtrl.Init(argv[1]),"tJobCtrl init failed.");
    CHECK_RET(tJobCtrl.StartJob(),"Start faild.");
    return iRet;   
}


