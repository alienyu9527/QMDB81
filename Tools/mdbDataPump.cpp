/****************************************************************************************
*@Copyrights  2011，中兴软创（南京）计算机有限公司 开发部 CCB项目--QMDB团队
*@                   All rights reserved.
*@Name：	    mdbDataPump.cpp
*@Description： 工具根据指定表或SQL导出数据或者导入数据
*@Author:		cao.peng
*@Date：	    2013年12月26日
*@History:
******************************************************************************************/
#include "stdio.h"
#include "stdlib.h"

#include "Helper/mdbStruct.h"
#include "Helper/TThreadLog.h"
#include "Tools/mdbDataPumpCtrl.h"

//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//using namespace QuickMDB;

void Help(int argc, char* argv[])
{
    printf("-------\n"
            " Usage:\n"
            " Export data commands:\n"
            "   %s dsn=<Dsn-name> operate=export tables=<table-names> format=T dumpfile=<file-name>\n"
            "   %s dsn=<Dsn-name> operate=export dumpfile=<file-names> format=S sql=<sql-info>\n"
            " Import data commands:\n"
            "   %s dsn=<Dsn-name> operate=import dumpfile=<file-name>\n"
            "   %s [ -H | -h ] \n"
            " Example:\n" 
            "   %s dsn=r12 operate=export tables=bal format=T dumpfile=/ztesoft/qmdb/test.txt\n"  
            "   %s dsn=r12 operate=export tables=bal format=S dumpfile=/ztesoft/qmdb/test.sql\n"  
            "   %s dsn=r12 operate=import dumpfile=/ztesoft/qmdb/test.txt\n"      
            " Note:\n"
            "   dsn: dsn-name.\n"
            "   operate: Modes of operation,it must be export or import.\n"
            "   tables: Export table,Support multiple tables, use commas(,) between table partitioning.\n"
            "   format: Export file formats.T:*.txt;S:*.sql.\n"
            "   dumpfile: Export file name,include path.\n"
            "   sql: The SQL export data.\n"
            "   -H|-h Print Help.\n"
            "-------\n",argv[0],argv[0],argv[0],argv[0],argv[0],argv[0],argv[0]);
}

int main(int argc, char* argv[])
{
    int iRet = 0;
    if(argc < 4 || strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        Help(argc,argv);
        return iRet;
    }
    char cFormat;
    char sDsn[MAX_NAME_LEN] ={0};
    char sOperType[MAX_FILE_NAME] ={0};
    char sFileName[MAX_FILE_NAME] = {0};
    char sTableName[MAX_FILE_NAME] = {0};
    char sSQL[MAX_SQL_LEN] = {0};
    char sCmdLiine[MAX_SQL_LEN] = {0};
    char sTmpValue[MAX_SQL_LEN] = {0};

    //解析参数
    std::vector<string> vCmdParam;
    TMdbNtcSplit  tPkSplit;
    for(int i = 1;i<argc;i++)
    {
        SAFESTRCPY(sCmdLiine,sizeof(sCmdLiine),argv[i]);
        tPkSplit.SplitString(sCmdLiine,'=');
        for(int j =0;j<tPkSplit.GetFieldCount();j++)
        {
            SAFESTRCPY(sTmpValue,sizeof(sTmpValue),tPkSplit[j]);
            TMdbNtcStrFunc::Trim(sTmpValue);
            if(strlen(sTmpValue)==0){continue;}
            vCmdParam.push_back(sTmpValue);
        }
    }
    //参数和值要成对
    if(vCmdParam.size()%2 != 0)
    {
        Help(argc,argv);
        TADD_ERROR(-1,"Command line argument format is wrong,please check.");
        return iRet;
    }
    
    for(std::vector<string>::size_type j=0;j != vCmdParam.size();)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(vCmdParam[j].c_str(),"dsn") == 0)
        {
            SAFESTRCPY(sDsn, sizeof(sDsn),vCmdParam[j+1].c_str());
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(vCmdParam[j].c_str(),"operate") == 0)
        {
            SAFESTRCPY(sOperType, sizeof(sOperType),vCmdParam[j+1].c_str());
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(vCmdParam[j].c_str(),"tables") == 0)
        {
            SAFESTRCPY(sTableName, sizeof(sTableName),vCmdParam[j+1].c_str());
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(vCmdParam[j].c_str(),"format") == 0)
        {
            cFormat = vCmdParam[j+1].c_str()[0];
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(vCmdParam[j].c_str(),"dumpfile") == 0)
        {
            SAFESTRCPY(sFileName, sizeof(sFileName),vCmdParam[j+1].c_str());
        } 
        else if(TMdbNtcStrFunc::StrNoCaseCmp(vCmdParam[j].c_str(),"sql") == 0)
        {
            SAFESTRCPY(sSQL, sizeof(sSQL),vCmdParam[j+1].c_str());
        }
        j = j+2;
    }
    TADD_START(sDsn,"mdbDataPump", 0,false,false);

    TMdbDataPumpCtrl tDataPumpCtrl;
    CHECK_RET(tDataPumpCtrl.Init(sDsn),"Failed to initialize.");
    if(TMdbNtcStrFunc::StrNoCaseCmp(sOperType,"export") == 0)
    {
        CHECK_RET(tDataPumpCtrl.ExportData(sTableName,cFormat,sFileName,sSQL),"Failed to export data.");
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sOperType,"import") == 0)
    {
        CHECK_RET(tDataPumpCtrl.ImportData(sFileName),"Failed to import data.");
    }
    else
    {
        TADD_ERROR(-1,"%s operation not supported.",sOperType);
        return ERR_APP_INVALID_PARAM;
    }
    return 0;
}

