/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbCreateTable.cpp		
*@Description： 创建表
*@Author:		li.shugang
@Modify :       
*@Date：	    2009年11月11日
*@History:      提供统一的帮助说明风格
******************************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Helper/TThreadLog.h"
#include "Control/mdbLogInfo.h"
#include "Control/mdbTableInfoCtrl.h"
//#include "Helper/mdbProcess.h"

#ifdef WIN32
	#pragma comment(lib, "Interface.lib")
	#pragma comment(lib, "Helper.lib")
	#pragma comment(lib, "Tools.lib")
	#pragma comment(lib, "Control.lib")
	#pragma comment(lib, "Monitor.lib")
	#pragma comment(lib, "DataCheck.lib")
	#pragma comment(lib, "OracleFlush.lib")
	#pragma comment(lib, "Agent.lib")
	#pragma comment(lib, "Replication.lib")
#endif

//using namespace QuickMDB;

int main(int argc, char* argv[])
{      
 	char sDSN[32];
    char sTableName[32];
 	memset(sDSN, 0, sizeof(sDSN));
 	memset(sTableName, 0, sizeof(sTableName));
 	
    if(argc != 3)
    {    
     	printf("Usage : \n\t%s <DSN> <TABLE_NAME>\n\n", argv[0]);
        return 0;
    }
    SAFESTRCPY(sDSN,sizeof(sDSN),argv[1]);
    SAFESTRCPY(sTableName,sizeof(sTableName),argv[2]);
	//TADD_START(sDSN,0, "mdbCreateTable", 0, true);
    TADD_START(sDSN,"mdbCreateTable", 0, true,false);
    
	TMdbTableInfoCtrl tTableInfoCtrl;
	int iRet = tTableInfoCtrl.SetDSN(sDSN);
	if(iRet < 0)
	{
		return iRet;	
	}
	iRet = tTableInfoCtrl.CreateNewTable(sTableName);
    if(iRet < 0)
	{
		return iRet;	
	}
    
    TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Create table[%s].",sTableName);
    return 0;   
}


