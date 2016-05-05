/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbDropTable.cpp		
*@Description： 删除表
*@Author:		li.shugang
@Modify :       
*@Date：	    2009年11月03日
*@History:      提供统一的帮助说明风格
******************************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Helper/TThreadLog.h"
#include "Control/mdbLogInfo.h"
//#include "Helper/mdbProcess.h"
#include "Control/mdbTableInfoCtrl.h"
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
    char sDeleteOpt[32] = {0};
    memset(sDSN, 0, sizeof(sDSN));
    memset(sTableName, 0, sizeof(sTableName));
 	
    if(argc != 3)
    {    
     	printf("Usage : \n\t%s <DSN> <TABLE_NAME>\n\n", argv[0]);
        return 0;
    }
    //二次确认用户是否真的需要删除表操作，防止用户误操作的场景
    printf("\n");
    cout<<"Please confirm,if yes, please input Y,otherwise the input N:";
    cin>>sDeleteOpt;
    cout << endl;
    if(TMdbNtcStrFunc::StrNoCaseCmp(sDeleteOpt, "Y") != 0)
    {
        return 0;
    }
    
    SAFESTRCPY(sDSN,sizeof(sDSN),argv[1]);
    SAFESTRCPY(sTableName,sizeof(sTableName),argv[2]);
    TADD_START(sDSN, "mdbDropTable", 1, true,false);
	TMdbTableInfoCtrl tDrop;
	int iRet = tDrop.SetDSN(sDSN);
	if(iRet < 0)
	{
		return iRet;	
	}
	tDrop.DropTable(sTableName);
    if(iRet < 0)
	{
		return iRet;	
	}
    return 0;   
}


