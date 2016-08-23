/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbServer.cpp	
*@Description�� ���÷��������̣�·�ɷ������״̬��أ�˫������
*@Author:		jiang.lili
*@Date��	    2014/05/4
*@History:
******************************************************************************************/
#include "stdio.h"
#include "stdlib.h"

#include "Helper/TThreadLog.h"
#include "Interface/mdbQuery.h"
#include "Replication/mdbServerCtrl.h"
//using namespace QuickMDB;

int main(int argc, char* argv[])
{   
    //Process::SetProcessSignal();
#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); /* run in backgroud */
#endif	

    int iRet = 0;

    TADD_OFFSTART("OFFLINE","mdbServer", 0,true);
    try
    {
        TMdbServerCenter tServer;
        if (2 == argc && TMdbNtcStrFunc::StrNoCaseCmp(argv[1], "stop") == 0)
        {
            CHECK_RET(tServer.Stop(), "mdbServer stop failed.");
        }
        else if (1 == argc || (2 == argc && TMdbNtcStrFunc::StrNoCaseCmp(argv[1], "start") == 0))
        {
            CHECK_RET(tServer.Init(), "mdbServer init failed.");
            CHECK_RET(tServer.Start(), "mdbServer start failed.");
        }   
        else
        {
            printf("Invalid command.\n");
        }
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

