#include <string>
#include <iostream>

#ifndef _WIN32
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "Helper/TThreadLog.h"
#include "Tools/mdbDataExport.h"
#include "time.h"
#endif

//using namespace QuickMDB;

int main(int argc, char* argv[])
{
    int iRet = 0;
#ifdef DB_NODB
		printf("mdbExport is useless in NODB mode\n");
#else
	bool bIsForceExport = false;
    char sWhereCond[MAX_SQL_LEN];
    memset(sWhereCond,0,sizeof(sWhereCond));

    if(argc < 4)
    {
    	printf("Usage:\n\t%s dsn srcTable objTable <\"where condition\"> <-f>\n", argv[0]);
    	return 0;
    }
    if(argc >= 5)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(argv[4],"-f") == 0)
    	{
            bIsForceExport = true;
			if(argc >= 6)
			{
                strncpy(sWhereCond,argv[5],MAX_SQL_LEN-1);
			}
    	}
		else
		{
		    strncpy(sWhereCond,argv[4],MAX_SQL_LEN-1);
		}
    }

    
    TADD_START(argv[1],"mdbExport", 0, true,false);
    try
    {
        TMdbDataExport tExport;
        iRet = tExport.Login(argv[1]);
        if(iRet < 0)
        {
        	return iRet;	
        }
        if(!bIsForceExport)
        {
            iRet = tExport.Export(argv[2], argv[3],sWhereCond);
        }
		else
		{
            iRet = tExport.ForceExport(argv[2], argv[3],sWhereCond);
		}
        if(iRet < 0)
        {
        	return iRet;	
        }      
    }
    catch(TMdbException& e)
    {
        printf("ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
    }
    catch(...)
    {
        printf("Unknown error!\n");       
    }
#endif
	
    return iRet;
}



