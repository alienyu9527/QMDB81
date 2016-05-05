/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbAgent.cpp
*@Description： 内存数据库的index信息查看
*@Author:		jiang.mingjun
*@Date：	    2010年05月11日
*@History:
******************************************************************************************/
#include "Helper/TThreadLog.h"
#include "Control/mdbIndexCtrl.h"


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
    int iRet = 0;
    if(argc != 4)
    {
        printf("Invalid parameters.\n %s <DSN> <tablename> <detaillevel [0,1,2] 2=full detail> \n\n",argv[0]);
        return iRet;
    }

    TADD_START(argv[1],"mdbIndexLook",  0, false,false);
    char * sDsn = argv[1];
    TMdbShmDSN * pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
    if(NULL == pShmDsn)
    {
        TADD_ERROR(-1," pShmDsn[%s] is null.",argv[1]);
        return 0;
    }
    TMdbTable * pTable = pShmDsn->GetTableByName(argv[2]);
    if(NULL == pTable)
    {
        TADD_ERROR(-1," Table [%s] not exist.",argv[2]);
        return 0;
    }

    TMdbIndexCtrl indexCtrl;
    CHECK_RET(indexCtrl.AttachTable(pShmDsn,pTable),"indexCtrl.AttachTable failed");
    printf("please wait.....\n");
    indexCtrl.PrintIndexInfo(atoi(argv[3]),true);
    printf("finish.....\n");
    //TADD_END();
    return 0;
}


