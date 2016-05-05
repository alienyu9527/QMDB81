/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbBulkCp.cpp   
*@Description： 内存表的数据导入导出
*@Author:       wang.liebao
*@Date：        2014年11月12日
*@History:
******************************************************************************************/

/****************************************************************************************
*导入导出文件格式:
*@逗号分隔字段，字符串中如果有逗号，必须以单引号包括字段。
*@空以NULL表示
*@举例:
*@NUMBER,VARCHAR,VARCHAR,NUMBER
*@
*@123,'abc,efg',NULL,456
*@123,abc,def,NULL
*@
*@
******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "Helper/mdbStruct.h"
#include "Helper/TThreadLog.h"
//#include "Helper/mdbStrFunc.h"
#include "Tools/mdbBulkCtrl.h"
#include "Helper/mdbCommandlineParser.h"

//using namespace QuickMDB;

void Help()
{
    printf("-------\n"
        		" Usage\n"
        		"   mdbBulkCp \n"
        		"      -[i|o|f] -c <sDsn>  -t tablename  -p filename \n"
                " Example\n" 
                "   mdbBulkCp \n"  
                "      -i -c ocs -t bal -p /oracle/test/TEST/test_import.sh \n" 
                "      -o -c ocs -t bal -p /oracle/test/TEST/test_export \n" 
                "      -f -c ocs -t bal -p /oracle/test/TEST/test_export -s /oracle/test/TEST/bal_sql \n" 
                " Note\n"
                "      -i import data from file.\n"
                "      -o export data from qmdb   to file.\n"
                "      -f export data from oracle to file.\n"
                "      -c data source.\n"
                "      -t tablename in qmdb.\n"
                "      -p file path.\n"
                "      -s SQL file used to export data from oracle.\n"
                "      -H|-h Print Help.\n"
                
        		"-------\n");

}

int main(int argc, char* argv[])
{
    int iRet = 0;
    if(argc < 8 || strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        Help();
        return iRet;
    }

    
    //定义参数格式和选项
    CommandLineParser clp(argc, argv);
    //clp.set_check_condition("", 7);					// 指定程序的参数个数
    // 指定选项的参数个数，下同
	clp.set_check_condition("-i", 0);
	clp.set_check_condition("-o", 0);
	clp.set_check_condition("-f", 0);
	clp.set_check_condition("-h", 0);
	clp.set_check_condition("-c", 1);
	clp.set_check_condition("-p", 1);
	clp.set_check_condition("-s", 1);
	clp.set_check_condition("-t", 0 ,1);

    if(clp.check() == false)
	{
    	Help();
        return 0;
	}
	
	char sTableName[MAX_NAME_LEN]       ={0};
	char sFileName[MAX_PATH_NAME_LEN]   ={0};
	char sSQLFileName[MAX_PATH_NAME_LEN]={0};
	char sConn[MAX_NAME_LEN]            ={0};
	bool bImport        = false;
	bool bExportFromMdb = false;
	bool bExportFromOra = false;
    const vector<CommandLineParser::OptArgsPair>& pairs = clp.opt_args_pairs();
	vector<CommandLineParser::OptArgsPair>::const_iterator it;
	for (it = pairs.begin(); it != pairs.end(); ++it)
	{
		const string& opt = (*it)._first;
		const vector<string>& args = (*it)._second;
		
		if(opt == "-t")
		{
            SAFESTRCPY(sTableName,sizeof(sTableName),args[0].c_str());
			sTableName[strlen(sTableName)]='\0';
			continue;
		}
		if(opt == "-h" || opt == "-H")
		{
        	Help();
        	return 0;
		}
		if(opt == "-c")
		{
            SAFESTRCPY(sConn,sizeof(sConn),args[0].c_str());
			sConn[strlen(sConn)] = '\0';
			continue;
		}
        if(opt == "-p")
		{
            SAFESTRCPY(sFileName,sizeof(sFileName),args[0].c_str());
			sFileName[strlen(sFileName)]='\0';
            continue;
		}
        if(opt == "-s")
		{
            SAFESTRCPY(sSQLFileName,sizeof(sSQLFileName),args[0].c_str());
			sSQLFileName[strlen(sSQLFileName)]='\0';
            continue;
		}
		
        if(opt == "-i")
        {
            bImport = true;
            continue;
        }
        if(opt == "-o")
        {
            bExportFromMdb = true;
            continue;
        }
        if(opt == "-f")
        {
        	#ifndef DB_NODB
            bExportFromOra = true;
            continue;
			#else
			printf("mdbBulkCp cannot use Physical DB in NODB mode\n");
			return iRet;
			#endif
        }
        
        
    }
    
    //char sDsn[MAX_NAME_LEN] ={0};
    //char sSQL[MAX_SQL_LEN] = {0};
    //char sTmpValue[MAX_SQL_LEN] = {0};

    TADD_START(sConn,"mdbBulkCp",0,false,false);
    TADD_NORMAL("[BEGIN]");
    try
    {
        TMdbBulkCtrl tBulkCtrl;
        CHECK_RET(tBulkCtrl.Init(sConn),"Failed to initialize.");
        if(bExportFromMdb)
        {
            CHECK_RET(tBulkCtrl.ExportData(sFileName,sTableName),"Failed to export data.");
        }
        else if(bExportFromOra)
        {
            CHECK_RET(tBulkCtrl.ExportData(sFileName,sTableName,sSQLFileName),"Failed to export data.");
        }
        
        else if(bImport)
        {
            CHECK_RET(tBulkCtrl.ImportData(sFileName,sTableName),"Failed to import data.");
        }
        else
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"operation not supported.");
            return ERR_APP_INVALID_PARAM;
        }
    }
    catch(...)
    {
        printf("Unknown error!\n");       
    }
    
    TADD_NORMAL("[END]");
    return 0;
}

