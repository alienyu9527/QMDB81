/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbParseRepLog.cpp		
*@Description： 解析同步日志，分析出执行的sql以及参数
*@Author:		miao.jianixn
*@Date：	    2016年04月20日
*@History:
******************************************************************************************/
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "Helper/TThreadLog.h"
#include "Interface/mdbQuery.h"
#include "Dbflush/mdbReadDbLog.h"



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
                "   %s <DsnName> <Filename>\n"
                "   %s [ -H | -h ] \n"
                " Example:\n" 
                "   %s R12 /ztesoft/data/mdbr12/Insert_Ora.201201011212\n"              
                " Note:\n"
                "     <DsnName>: dsn name.\n"
                "     <Filename>: filename with full path to parse.\n"
                "     -H|-h Print Help.\n"
                "-------\n",argv[0],argv[0],argv[0]);
        return 0;
    }
    
    

    //设置程序名
    char sProcName[1024];
    memset(sProcName, 0, sizeof(sProcName));
    SAFESTRCPY(sProcName,sizeof(sProcName),argv[0]);
   
    TADD_START(argv[1],  sProcName, 0, false, false);  
    try
    {
        mdbReadOraLog oraLog;
        iRet = oraLog.Init(argv[1]); 
        if(iRet < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"oraLog.Init(%s) error!\n", argv[1]);  
            return -1;  
        }
        
        oraLog.ParseLogFile(argv[2]);
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

