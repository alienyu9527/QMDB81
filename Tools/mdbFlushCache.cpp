/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbFlushCache.cpp		
*@Description： Cache的数据全量的从Oracle刷新到QuickMDB中
*@Author:		zhang.lin
*@Date：	    2012年02月07日
*@History:
******************************************************************************************/


#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "Helper/TThreadLog.h"
#include "Tools/mdbDbFlushMdb.h"

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
    
    if(argc != 3|| strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        printf("-------\n"
        		" Usage:\n"
        		"   %s <dsn> <tablename|all> \n"
        		"   %s [ -H | -h ] \n"
        		" Example:\n" 
        		"   %s dsnname all \n"              
        		" Note:\n"
        		"     <dsn>: dsn name.\n"
        		"     <tablename>: table name.\n"
        		"     -H|-h Print Help.\n"
        		"-------\n",argv[0],argv[0],argv[0]);
        return 0;
    }
    TADD_START(argv[1],"mdbFlushCache", 0, true,false);
    
    int iRet = 0;
    try
    {
        TMdbDbFlushMdb FlushCache;
        iRet = FlushCache.Init(argv[1],argv[2]);
        if(iRet < 0)
        {
            TADD_ERROR(-1,"Init[%s] faild .\n",argv[1]); 
        	return iRet;	
        }
        iRet = FlushCache.FlushData(argv[2]);
        if(iRet < 0)
        {
            TADD_ERROR(-1,"CheckData[%s] faild .\n",argv[2]); 
        	return iRet;	
        }
    }
    catch(TMdbException& e)
    {
        printf("ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
    }
    catch(TMDBDBExcpInterface& e)
    {
        printf("ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
    }
    catch(...)
    {
        printf("Unknown error!\n");       
    }
    
    return 0;   
}


