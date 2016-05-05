//****************************************************************************************
//*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
//*@            All rights reserved.
//*@Name：	   mdbRoutingRep.cpp	
//*@Description: 分片备份主进程，负责与配置服务通信和数据上载
//*@Author:		jiang.lili
//*@Date：	    2014/05/4
//*@History:
//******************************************************************************************/
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "Helper/TThreadLog.h"
//#include "Helper/mdbProcess.h"
#include "Replication/mdbRepCtrl.h"
//#include "BillingSDK.h"
#include "Interface/mdbQuery.h"
//using namespace QuickMDB;

void Help(int argc, char* argv[])
{
    printf("-------\n"
        " Usage:\n"
        "   %s <sDsn> [load] \n"
        " Example:\n" 
        "   %s ocs\n"       
        " Note:\n"
        "     sDsn: data source\n"
        "-------\n" ,argv[0],argv[0]);
    return;
}
int main(int argc, char* argv[])
{   
    //Process::SetProcessSignal();
#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); /* run in backgroud */
#endif	

    int iRet = 0;
    //bool bLoad;//是否只加载数据
    try
    {
        if (argc != 2 )//&& QuickMDB::TMdbNtcStrFunc::StrNoCaseCmp(argv[1], "-h")!=0)//同步数据和状态上报
        {
            Help(argc, argv);
            return 0;
        }
        /*else if ((argc == 4 || argc == 3)&& QuickMDB::TMdbNtcStrFunc::StrNoCaseCmp(argv[2], "load") == 0)//启动时加载数据
        {
            bLoad = true;            
        }*/

        char sName[64];
        memset(sName, 0, sizeof(sName));
        sprintf(sName, "%s %s", argv[0], argv[1]);
        TADD_START(argv[1], sName, 0,false ,true);
        
        TMdbRepCtrl tRepCtrl;
        CHECK_RET(tRepCtrl.Init(argv[1]), "TMdbRepCtrl init failed.");//初始化
        CHECK_RET(tRepCtrl.Start(), "TMdbRepCtrl start failed.");//开始运行

        #if 0
        if (!bLoad)
        {
            CHECK_RET(tRepCtrl.Init(argv[1]), "TMdbRepCtrl init failed.");//初始化
            CHECK_RET(tRepCtrl.Start(), "TMdbRepCtrl start failed.");//开始运行
        }
        else
        {
            if (argc==3)
            {
                CHECK_RET(tRepCtrl.LoadData(argv[1]), "TMdbRepCtrl start failed.");//加载数据
            }
            else
            {
                CHECK_RET(tRepCtrl.LoadData(argv[1], atoi(argv[3])), "TMdbRepCtrl start failed.");//加载数据
            }
            
        }
        #endif 
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

