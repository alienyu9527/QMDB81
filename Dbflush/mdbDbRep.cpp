/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbOraRep.cpp		
*@Description： 内存数据库的与Oracle数据同步
*@Author:		li.shugang
*@Date：	    2009年04月20日
*@History:
******************************************************************************************/
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "Helper/TThreadLog.h"
#include "Interface/mdbQuery.h"
#include "Dbflush/mdbReadDbLog.h"


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

int CheckParam(const char* pDsn,int iCounts, int iPos)
{
    int iRet = 0;
    CHECK_OBJ(pDsn);
    TMdbConfig * pConfig = TMdbConfigMgr::GetMdbConfig(pDsn);
    CHECK_OBJ(pConfig);
    if(iCounts != pConfig->GetDSN()->iOraRepCounts)
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM,"Process startup parameter[%d] illegal,it must be %d.",\
                        iCounts,pConfig->GetDSN()->iOraRepCounts);
        return ERR_APP_INVALID_PARAM;
    }
    if(iPos < -1 || iPos > iCounts -1)
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM,"Process startup parameter[%d] illegal,it must be -1~%d.",\
            iPos,iCounts -1);
        return ERR_APP_INVALID_PARAM;
    }
    return iRet;
}

int main(int argc, char* argv[])
{   
    int iRet = 0;
    if(argc < 3 || strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {    
        printf("-------\n"
                " Usage:\n"
                "   %s <DsnName> <ProCout> <Mod> \n"
                "   %s [ -H | -h ] \n"
                " Example:\n" 
                "   %s R12 1 1 \n"              
                " Note:\n"
                "     <DsnName>: dsn name.\n"
                "     <ProCout>: Number of processes,it must be equal DSN configuration file ora-rep-counts parameter values.\n"
                "     <Mod>    : Mode id(the value must be less than ProCout) described as follows:\n"
                "                -1:Insert or delete operations synchronization process.\n"
                "                Mode id > -1:Update operation synchronization process.\n"
                "     -H|-h Print Help.\n"
                "-------\n",argv[0],argv[0],argv[0]);
        return 0;
    }
    
    //调试用先注释
    //Process::SetProcessSignal();   
    
    //建议进程输入参数是否合法
    CHECK_RET(CheckParam(argv[1],atoi(argv[2]), atoi(argv[3])),"CheckParam failed.");
    
#ifndef WIN32
    setsid();
    if (fork() > 0) exit(0); /* run in backgroud */
#endif    

    //设置程序名
    char sProcName[1024];
    memset(sProcName, 0, sizeof(sProcName));
    SAFESTRCPY(sProcName,sizeof(sProcName),argv[0]);
    for(int i=1; i<argc; ++i)
    {
        sprintf(sProcName+strlen(sProcName), " %s", argv[i]);  
    }
    TADD_START(argv[1],  sProcName, 0, false, true);  
    try
    {
        mdbReadOraLog oraLog;
        iRet = oraLog.Init(argv[1], sProcName); 
        if(iRet < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"oraLog.Init(%s, %s) error!\n", argv[1], sProcName);  
            return -1;  
        }
        
        oraLog.Start(atoi(argv[2]), atoi(argv[3]));
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n.",  e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMDBDBExcpInterface &e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n.", e.GetErrSql(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");       
    }
    //TADD_END();
    return 0;   
}



