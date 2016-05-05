/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbCheckPoint .cpp		
*@Description： 检测点
*@Author:      dong.chun
*@Date：	    2013.7
*@History:
******************************************************************************************/

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "Helper/TThreadLog.h"
#include "Control/mdbStorageEngine.h"
#include "Helper/mdbDateTime.h"
#include "Control/mdbProcCtrl.h"
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
        "-------\n" ,argv[0],argv[0],argv[0]);
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
    int iRet = 0;
	
    TMdbCheckPoint tMdbCheckPoint;
    CHECK_RET(tMdbCheckPoint.Init(argv[1]),"Init failed.");
   
    TMdbProcCtrl tProcCtrl;
    tProcCtrl.Init(argv[1]);
	int iwait = 0;
    do
    {
        if(tProcCtrl.IsCurProcStop())
        {
            TADD_NORMAL("Get exit msg....");
            iRet = 0;
            break;
        }

		if(iwait <= 0)
		{
	        tProcCtrl.UpdateProcHeart(0);
	        if(tMdbCheckPoint.NeedLinkFile())
	        {
	             CHECK_RET(tMdbCheckPoint.LinkFile(),"Attach failed.");
	        }
	        if(tMdbCheckPoint.DoCheckPoint() != 0)
	        {
	            TADD_ERROR(-1,"DoCheckPoint Faild");
	        }
	        tProcCtrl.UpdateProcHeart(0);

			iwait = 120;  //total 120*5 = 600 秒
		}

		iwait--;
			
        TMdbDateTime::Sleep(5);
    }while(1);
    return iRet;   
}


