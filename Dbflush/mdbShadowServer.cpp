/****************************************************************************************
*@Copyrights  2011�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QMDB�Ŷ�
*@                   All rights reserved.
*@Name��	    mdbShadowServer.cpp
*@Description�� Ӱ�ӱ�ˢ�½��̶����������ؽ���
*@Author:		li.ming
*@Date��	    2014��4��17��
*@History:
******************************************************************************************/

#include <unistd.h>
#include "Helper/TThreadLog.h"
#include "Dbflush/mdbShaowServerCtrl.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;
//using namespace QuickMDB;

void PrintUsage(const char* psName)
{
    printf("-------\n"
        		" Usage:\n"
        		"   %s  <dsn name> start \n"
        		"   %s  <dsn name> stop \n"
        		"   %s [ -H | -h ] \n"
        		" Example:\n" 
        		"   %s cc start"  
        		"-------\n",psName,psName,psName, psName);
}


int main(int   argc,   char*   argv[])
{
    int iRet = 0;

    if(argc !=  3 ||  strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        PrintUsage(argv[0]);
        return 0;
    }

    /* run in backgroud */
#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); 
#endif 

    TADD_OFFSTART("OFFLINE",argv[0], 0, false);

    TMdbShadowSvrCtrl tProcCtrl;
  
    if(0 == TMdbNtcStrFunc::StrNoCaseCmp(argv[2], "start")) // start
    {
        CHECK_RET(tProcCtrl.Start(argv[1]),"Start shaodow flush server failed.");
        CHECK_RET(tProcCtrl.Monitor(),"monitor shaodow flush failed.");
    }
    else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(argv[2], "stop")) //stop
    {
        CHECK_RET(tProcCtrl.Stop(argv[1]),"stop shaodow flush  server failed.");
    }
    else
    {
        PrintUsage(argv[0]);
    }    
    
    return iRet;
}
