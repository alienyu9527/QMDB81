/****************************************************************************************
*@Copyrights  2011�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QMDB�Ŷ�
*@                   All rights reserved.
*@Name��	    mdbShadowFlush.cpp
*@Description�� ����Ӱ�ӱ�ģʽ��ORCLE2MDBͬ��
*@Author:		li.ming
*@Date��	    2013��12��20��
*@History:
******************************************************************************************/
#include <string.h>
#include <unistd.h>

#include "Dbflush/mdbShadowFlushCtrl.h"
#include "Helper/TDBFactory.h"
#include "Helper/TThreadLog.h"

void PrintUsage(char* sName)
{
    printf("-------\n"
        		" Usage:\n"
        		"   %s  <dsn name>\n"
        		"   %s [ -H | -h ] \n"
        		" Example:\n" 
        		"   %s cc"  
        		"-------\n",sName,sName,sName);
}

//using namespace QuickMDB;

int main(int   argc,   char*   argv[])
{
    if(argc !=  2 ||  strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        PrintUsage(argv[0]);
        return 0;
    }

    /* run in backgroud */
#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); 
#endif 

    char sName[64];
    memset(sName, 0, sizeof(sName));
    snprintf(sName,sizeof(sName), "%s %s", argv[0], argv[1]);
    TADD_OFFSTART(argv[0],sName, 0, false);

    int iRet = 0;
    try
    {
        TShadowFlushCtrl tFlushCtrl;
        CHECK_RET(tFlushCtrl.Init(argv[1]),"Init Flush process(shadow mode) failed.");
        CHECK_RET(tFlushCtrl.Run(),"Flush process(shadow mode) failed.");
    }
    catch(TMDBDBExcpInterface& e)
    {
        printf("ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
    }
    catch(...)
    {
        printf("Unknown error!\n");       
    }
    
    return iRet;
}
