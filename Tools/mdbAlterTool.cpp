/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbAlterTool.cpp		
*@Description�� ����ϵͳ���������
*@Author:		zhang.lin
*@Date��	    2012��02��23��
*@History:      
******************************************************************************************/


#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#ifndef _WIN32
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "Helper/TThreadLog.h"
#include "time.h"
#include "Common/mdbStrUtils.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Tools/mdbAlterInfo.h"
#endif



//using namespace QuickMDB;


//mdbAgent ץȡ���ݰ�����
#define DUMP_PACKAGE "dump_package"
#define NO_DUMP_PACKAGE "no_dump_package"
/******************************************************************************
* ��������  :  CheckArgs()
* ��������  :  �������Ϸ���,sys �÷�
* ����      :  ��
* ���      :  ��
* ����ֵ    :  �ɹ�����true, ʧ�ܷ���false
* ����      :  zhang.lin
*******************************************************************************/
bool CheckArgs(char *sOper,char *sSet,char *sEqual)
{
    bool bCheck = true;
    if(TMdbNtcStrFunc::StrNoCaseCmp(sOper, "-p") == 0)
    {
        return true;
    }
    if( TMdbNtcStrFunc::StrNoCaseCmp(sOper, "sys") != 0 && TMdbNtcStrFunc::StrNoCaseCmp(sOper, "table") != 0)
    {
        printf("Error : Invalid parameter[%s],expected keyword [sys/table].\n\n",sOper);
        bCheck =  false;
    }
    if( TMdbNtcStrFunc::StrNoCaseCmp(sSet, "set") != 0)
    {
        printf("Error : Invalid parameter[%s],expected keyword [set].\n\n",sSet);
        bCheck =  false;
    }
    if( TMdbNtcStrFunc::StrNoCaseCmp(sEqual, "=") != 0)
    {
        printf("Error : Invalid parameter[%s],expected keyword [=].\n\n",sEqual);
        bCheck =  false;
    }
    if(!bCheck)
    {
        printf("Example Command:\n" 
                    "mdbAlterTool ocs sys set local_active_ip = '127.0.0.1'\n"  
                    "mdbAlterTool ocs table ocs_session set  bIsCheckPriKey = 'Y'\n"
                    );  
        return false;
    }
    return true;
}

void Help(int argc, char* argv[])
{
    printf("-------\n"
            " Usage\n"
            "   %s    <sDsn>\n"
            "\t\t\t   sys  set <pro> = <value>\n"
            "\t\t\t   table <table> set <pro> = <value>\n"
            //"\t\t\t   table <table> column <colName> set length = <value>\n"
            "\t\t\t   -p {<process-id> <IsMonitor>}\n"
            "\t\t\t   < %s | %s > \n"
            "\t\t\t   < -H | -h > \n"
            " Example\n" 
            "   %s    ocs\n"  
            "\t\t\t   sys set iLogLevel = 0 \n"
            "\t\t\t   table ocs_session set  bIsCheckPriKey = 'Y'\n"  
            //"\t\t\t   table ocs_session column EVENT set  length = 2048 \n" 
            "\t\t\t   -p 2048 Y \n" 
            "\t\t\t   %s  \n" 
            "\t\t\t   -h \n"
            " Notes\n"
            "      sDsn : data source name.\n"
            "      pro  : property name .    \n"
            "      value: property value .    \n"
            "      table: table name . \n"
            //"      colName: column name . \n"
            "      -p: Specify whether to monitor process,If IsMonitor to Y is monitored or not monitored.\n"
            "      %s :Dump Cs package to file .\n"
            "      %s :Stop Dumping Cs package to file .\n"
            "      -H|-h Print Help.\n"
            "-------\n" 
            ,argv[0],DUMP_PACKAGE,NO_DUMP_PACKAGE,argv[0],NO_DUMP_PACKAGE,DUMP_PACKAGE,NO_DUMP_PACKAGE);
}



int main(int argc, char* argv[])
{
    int iDumpPackage = -1;
    char cProcState = PFREE;
    
    if(argc < 3)
    {
        Help(argc,argv);
        return 0;
    }
    
    int iRet = 0;
    char sDsn[256] = {0};
    SAFESTRCPY(sDsn,sizeof(sDsn),argv[1]);//dsn
    char sOper[256] = {0};
    SAFESTRCPY(sOper,sizeof(sOper),argv[2]);//���������:sys or table

    char sTableName[256] = {0};//����
    char sColName[256] = {0};//����
    char sColumn[256] = {0};//column ��ʾ��
    char sSet[256] = {0};//set ��ʾ��
    char sProperty[256] = {0};//������
    char sEqual[256] = {0};// == ��
    char sProValue[256] = {0};//����ֵ
    int iPid = 0;
    bool bIsMonitor = false;

    if(argc == 3)
    {
        if( TMdbNtcStrFunc::StrNoCaseCmp(argv[2],DUMP_PACKAGE) == 0)
        {
            iDumpPackage = 1;
            cProcState = PDUMP;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(argv[2],NO_DUMP_PACKAGE) == 0)
        {
            iDumpPackage = 0;
        }
        else
        {
            Help(argc,argv);
            return 0;
        }
    }
    else if(argc == 5)
    {
        iPid = TMdbNtcStrFunc::StrToInt(argv[3]);
        if( TMdbNtcStrFunc::StrNoCaseCmp(argv[4], "Y") == 0)
        {
            bIsMonitor = true;
        }
        else if( TMdbNtcStrFunc::StrNoCaseCmp(argv[4], "N") == 0)
        {
            bIsMonitor = false;
        }
        else
        {
            printf("Error : Invalid parameter[%s],it must be Y(y) or N(n).\n\n",argv[4]);
            return -1;
        }
    }
    else if(argc == 7)//sys
    {
        SAFESTRCPY(sSet,sizeof(sSet),argv[3]);//set ��ʾ��
        SAFESTRCPY(sProperty,sizeof(sProperty),argv[4]);//������
        SAFESTRCPY(sEqual,sizeof(sEqual),argv[5]);// == ��ʾ  
        SAFESTRCPY(sProValue,sizeof(sProValue),argv[6]);//����ֵ
    }
    else if(argc == 8)//table
    {
        SAFESTRCPY(sTableName,sizeof(sTableName),argv[3]);//������
        SAFESTRCPY(sSet,sizeof(sSet),argv[4]);//set ��ʾ��
        SAFESTRCPY(sProperty,sizeof(sProperty),argv[5]);//������
        SAFESTRCPY(sEqual,sizeof(sEqual),argv[6]);// == ��ʾ  
        SAFESTRCPY(sProValue,sizeof(sProValue),argv[7]);//����ֵ
    }
    else if(argc == 10)//column
    {
        SAFESTRCPY(sTableName,sizeof(sTableName),argv[3]);//������
        SAFESTRCPY(sColumn,sizeof(sColumn),argv[4]);//column��ʾ��
        SAFESTRCPY(sColName,sizeof(sColName),argv[5]);//������
        if( TMdbNtcStrFunc::StrNoCaseCmp(sColumn, "column") != 0)
        {
            printf("Error : Invalid parameter[%s],expected keyword [column].\n\n",sColumn);
            return -1;
        }
        SAFESTRCPY(sSet,sizeof(sSet),argv[6]);//set ��ʾ��
        char sLen[256];//length��ʾ
        memset(sLen,0,sizeof(sLen));
        SAFESTRCPY(sLen,sizeof(sLen),argv[7]);
        if( TMdbNtcStrFunc::StrNoCaseCmp(sLen, "length") != 0)
        {
            printf("Error : Invalid parameter[%s],expected keyword [length].\n\n",sLen);
            return -1;
        }
        SAFESTRCPY(sEqual,sizeof(sEqual),argv[8]);// == ��ʾ  
        SAFESTRCPY(sProValue,sizeof(sProValue),argv[9]);//����ֵ
    }
    else 
    {
        Help(argc,argv);
        return 0;
    }

    if(-1 == iDumpPackage && !CheckArgs(sOper,sSet,sEqual))
    {
        return 0;
    }

    TADD_START(sDsn,"mdbAlterTool",  0, true,false);
    TMdbAlterInfo  AltInfo;
    bool bCheck = AltInfo.LinkDsn(sDsn);
    if(!bCheck)
    {
        return 0;                                                     
    }
    if(sTableName[0] != '\0')
    {
        if(sColName[0] == '\0')
        {
            CHECK_RET(AltInfo.AlterTable(sTableName,sProperty,sProValue),"AlterTable failed");
        }
        else
        {
            bCheck = AltInfo.AlterColLen(sTableName,sColName,sProValue);
        }
    }
    else if(-1 != iDumpPackage)
    {
        CHECK_RET(AltInfo.SetDumpPackage(cProcState),"SetDumpPackage failed.");
    
    }
    else if(iPid > 0)
    {
        CHECK_RET(AltInfo.SetProccessIsMonitor(iPid,bIsMonitor),"SetProccessIsMonitor failed.");
    }
    else
    {
        CHECK_RET(AltInfo.AlterSys(sProperty,sProValue),"AlterSys failed.");
    }
    return iRet;
}


