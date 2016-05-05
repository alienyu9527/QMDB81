/****************************************************************************************
*@Copyrights  2011，中兴软创（南京）计算机有限公司 开发部 CCB项目--QMDB团队
*@                   All rights reserved.
*@Name：	    mdbCleanRestoreTool.cpp
*@Description： qmdb的过期数据清理、备份与恢复工具
*@Author:		li.ming
*@Date：	    2013年12月05日
*@History:
******************************************************************************************/
#include "stdio.h"
#include "stdlib.h"

#include "Helper/mdbStruct.h"
#include "Helper/TThreadLog.h"
#include "Tools/mdbOverdueDataCtrl.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;


//using namespace QuickMDB;

int main(int argc, char* argv[])
{
    if(argc < 4 || strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        printf("-------\n"
        		" Usage:\n"
        		"   %s -b <bakfile_path> <filename> \n"
        		"   %s -d <bakfile_path>  <tablename|all> dsnname \n"
        		"   %s -r <bakfile_path>  <tablename|all> dsnname \n"
        		"   %s [ -H | -h ] \n"
        		" Example:\n" 
        		"   %s -b /home/QuickMDB/bakdata /home/QuickMDB/bakdata/cleandata.txt\n"  
        		"   %s -d /home/QuickMDB/bakdata all\n"    
        		"   %s -r /home/QuickMDB/bakdata subs cc\n"    
        		" Note:\n"
        		"     <bakfile_path>: table data bak file path\n"
        		"     <filename>: file name,include path\n"
        		"     -H|-h Print Help.\n"
        		"-------\n",argv[0],argv[0],argv[0],argv[0],argv[0],argv[0],argv[0]);

        return 0;
    }
    TADD_OFFSTART("OFFLINE",argv[0], 0, true);
    
    char sOperType[MAX_FILE_NAME] ={0};
    char sPath[MAX_FILE_NAME] = {0};
    char sProcObject[MAX_FILE_NAME] = {0};
    char sDsn[MAX_NAME_LEN] ={0};

    SAFESTRCPY(sOperType, sizeof(sOperType), argv[1]);
    SAFESTRCPY(sPath, sizeof(sPath), argv[2]);
    SAFESTRCPY(sProcObject, sizeof(sProcObject), argv[3]);
    if(0  != TMdbNtcStrFunc::StrNoCaseCmp(sProcObject, "all")  && 0 != TMdbNtcStrFunc::StrNoCaseCmp(sOperType,"-b"))
    {
        if(argc < 5)
        {
            TADD_ERROR(-1,"invalid cmd,check cmd,pls");
            return -1;
        }
        SAFESTRCPY(sDsn, sizeof(sDsn), argv[4]);
    }

    if(0 == TMdbNtcStrFunc::StrNoCaseCmp(sOperType,"-d") || 0 == TMdbNtcStrFunc::StrNoCaseCmp(sOperType,"-r"))
    {
        if(0  != TMdbNtcStrFunc::StrNoCaseCmp(sProcObject, "all") )
        {
            if(argc < 5)
            {
                TADD_ERROR(-1,"invalid cmd,check cmd,pls");
                return -1;
            }
            SAFESTRCPY(sDsn, sizeof(sDsn), argv[4]);
        }
        
    }

    TOverDueDataCtrl tCleanCtrl;

    int iRet = 0;
    if(0 == TMdbNtcStrFunc::StrNoCaseCmp(sOperType, "-b"))
    {
        iRet = tCleanCtrl.RunBak(sPath, sProcObject);
    }
    else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(sOperType, "-d"))
    {
        if(strlen(sDsn) != 0)
        {
            iRet = tCleanCtrl.RunClean(sPath, sProcObject, sDsn);
        }
        else
        {
            iRet = tCleanCtrl.RunClean(sPath, sProcObject);
        }
        
    }
    else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(sOperType, "-r"))
    {
        iRet = tCleanCtrl.RunRestore(sPath, sProcObject,sDsn);
    }
    else
    {
        printf("invalid cmd type:[%s]\n", sOperType);
        return 0;
    }

    if(iRet < 0 )
    {
        printf("process failed.\n");
    }

    return 0;
    
}
