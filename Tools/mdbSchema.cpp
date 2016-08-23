
/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbSchema.cpp   
*@Description：	数据库以及表定义的导出
*@Author:       miao.jianxin
*@Date：        2016年6月6日
*@History:
******************************************************************************************/



#include <stdio.h>
#include <stdlib.h>

#include "Helper/mdbStruct.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbConfig.h"
//#include "Helper/mdbStrFunc.h"
#include "Tools/mdbBulkCtrl.h"
#include  "Tools/mdbExptDDLCtrl.h"
#include "Helper/mdbCommandlineParser.h"

//using namespace QuickMDB;

void Help()
{
    printf("-------\n"
        		" Usage\n"
        		"   mdbSchema \n"
        		"      -[l|h|n|s|c] \n"
                " Example\n" 
                "   mdbSchema \n"  
                "      -l -c 'dsn=tt;uid=tt;pwd=tt' \n" 
                "      -n tables -s -c 'dsn=tt;uid=tt;pwd=tt'  \n"  
                " Note\n"
                "      -l show all objects name in db.\n"
                "      -n point the object type to export ddl:tables,jobs,sequences \n"
                "      -s include system tables.\n"
                "      -H|-h Print Help.\n"
                
        		"-------\n");

}


//解析连接字符串，分出dsn,uid,pwd
int ParseQmdbOper(const char * sOperStr,char* dsn, char* uid, char* pwd, int iSize)
{
    int iRet = 0;
    TMdbNtcSplit tSplit;
    tSplit.SplitString(sOperStr,';');
    int i = 0;
    const char * sPrefix[] = {"dsn=","uid=","pwd="};
    int iCount = 3;
    char * sContextAddr[] = {dsn,uid,pwd};
    for(i = 0;i < tSplit.GetFieldCount() ;++i)
    {
        int j = 0;
        for(j = 0 ;j < iCount;++j)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(tSplit[i],sPrefix[j],strlen(sPrefix[j])) == 0)
            {//前缀相同
                SAFESTRCPY(sContextAddr[j],iSize,tSplit[i]+strlen(sPrefix[j]));
                break;
            }
        }
        if(j == iCount)
        {
            printf("[%s] is error\n",tSplit[i]);
            return ERR_APP_INVALID_PARAM;
        }
    }
    
    return iRet;
}

//检查该用户的权限
int CheckConStrValid(char* dsn, char* uid, char* pwd)
{

	int iRet = 0;//上载SYS配置文件
    TMdbConfig *pMdbConfig = TMdbConfigMgr::GetDsnConfig(dsn);
    CHECK_OBJ(pMdbConfig);
   	if(strlen(uid) == 0 || strlen(pwd) == 0)
   	{

		TADD_ERROR(ERR_APP_INT_VALUE_INVALID,"the username or password can't be empty");
		return ERR_APP_INT_VALUE_INVALID;
   	}
    else
    {
        CHECK_RET(pMdbConfig->CheckParam(uid, pwd, dsn), "input uid or  password error.");
    }
    //校验用户权限
    if(pMdbConfig->GetAccess() != MDB_ADMIN)
    {
        CHECK_RET(ERR_DB_PRIVILEGE_INVALID,"No admin access=[%c].",pMdbConfig->GetAccess());
    }
	return iRet;

}

int ExptDDL(char*  sDsn,bool bShowObject,bool bIncSystable,char* sTypeName)
{
	int iRet = 0;
	try
    {
       TMdbExptDDLCtrl ddlctrl;
	   ddlctrl.Init(sDsn);
	 

		//show object names
		if(bShowObject && bIncSystable && sTypeName[0] == 0)
		{
			 CHECK_RET(ddlctrl.ExptUserTablesDDL(true),"get user table ddl ddl failed");
			 CHECK_RET(ddlctrl.ExptSysTablesDDL(true),"get sys table ddl ddl failed");
			 CHECK_RET(ddlctrl.ExportJobs(true),"get job ddl failed");
			 CHECK_RET(ddlctrl.ExportSequences(true),"get sequence ddl failed");

		}
		else if(bShowObject && !bIncSystable && sTypeName[0] == 0)
		{
			
			CHECK_RET(ddlctrl.ExptUserTablesDDL(true),"get user table ddl ddl failed");
			CHECK_RET(ddlctrl.ExportJobs(true),"get job ddl failed");
			CHECK_RET(ddlctrl.ExportSequences(true),"get sequence ddl failed");
		}

		else if(sTypeName[0] != 0)
		{
			//tables,sequences,jobs
			if(TMdbNtcStrFunc::StrNoCaseCmp(sTypeName,"tables") == 0)
			{
				CHECK_RET(ddlctrl.ExptUserTablesDDL(bShowObject),"get user table ddl ddl failed");
				if(bIncSystable)
					CHECK_RET(ddlctrl.ExptSysTablesDDL(bShowObject),"get sys table ddl ddl failed");
			}
			else if(TMdbNtcStrFunc::StrNoCaseCmp(sTypeName,"sequences") == 0)
			{
				CHECK_RET(ddlctrl.ExportSequences(bShowObject),"get sequence ddl failed");
			}
			else if(TMdbNtcStrFunc::StrNoCaseCmp(sTypeName,"jobs") == 0)
			{
				CHECK_RET(ddlctrl.ExportJobs(bShowObject),"get job ddl failed");
			}
			else
			{
				TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid type name for mdbSchema");
			}
			
		}
		else
		{
			if(bIncSystable)
			{
				CHECK_RET(ddlctrl.ExptDatabaseDDL(),"get database ddl failed");
			  	CHECK_RET(ddlctrl.ExptUserTablesDDL(),"get user table ddl ddl failed");
				CHECK_RET(ddlctrl.ExptSysTablesDDL(),"get sys table ddl ddl failed");
			  	CHECK_RET(ddlctrl.ExportJobs(),"get job ddl failed");
			  	CHECK_RET(ddlctrl.ExportSequences(),"get sequence ddl failed");
			}
			else
			{
				CHECK_RET(ddlctrl.ExptDatabaseDDL(),"get database ddl failed");
				CHECK_RET(ddlctrl.ExptUserTablesDDL(),"get user table ddl ddl failed");
				CHECK_RET(ddlctrl.ExportJobs(),"get job ddl failed");
				CHECK_RET(ddlctrl.ExportSequences(),"get sequence ddl failed");
			}

		}
      
    }
    catch(...)
    {
        printf("Unknown error!\n");       
    }

	return iRet;
	
}
int main(int argc, char* argv[])
{
    int iRet = 0;
    if(argc < 2 || strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        Help();
        return iRet;
    }
	bool bShowObject = false;
	bool bIncSystable = false;
	char sTypeName[128] = {0};
	char sConStr[256] = {0};
	
    char sDsn[32] = {0};
	char sUid[32] = {0};
	char sPwd[32] = {0};
	
    //定义参数格式和选项
    CommandLineParser clp(argc, argv);
    //clp.set_check_condition("", 7);					// 指定程序的参数个数
    // 指定选项的参数个数，下同
	clp.set_check_condition("-l", 0);

	clp.set_check_condition("-h", 0);
	
	clp.set_check_condition("-n", 1);
	clp.set_check_condition("-s", 0);
	clp.set_check_condition("-c", 1);

	
	
    if(clp.check() == false)
	{
    	Help();
        return 0;
	}
	
	
    const std::vector<CommandLineParser::OptArgsPair>& pairs = clp.opt_args_pairs();
	std::vector<CommandLineParser::OptArgsPair>::const_iterator it;

	
	for (it = pairs.begin(); it != pairs.end(); ++it)
	{
		const string& opt = (*it)._first;
		const std::vector<string>& args = (*it)._second;
		
		if(opt == "-l")
		{
			
           bShowObject = true;
			continue;
		}
		if(opt == "-h" || opt == "-H")
		{
        	Help();
        	return 0;
		}
		if(opt == "-c")
		{
            SAFESTRCPY(sConStr,sizeof(sConStr),args[0].c_str());
			sConStr[strlen(sConStr)] = '\0';
			continue;
		}
        if(opt == "-n")
		{
            SAFESTRCPY(sTypeName,sizeof(sTypeName),args[0].c_str());
			sTypeName[strlen(sTypeName)]='\0';
            continue;
		}
        if(opt == "-s")
		{
           bIncSystable =true;
            continue;
		}
		
       
    
        
    }

	//if(bShowObject && sTypeName[0] != 0)
	//{
	//	TADD_ERROR(ERR_APP_INVALID_PARAM,"can't show object names and export ddl at the same time,the cmd option is invalid");
	//	return 0;

	//}

	CHECK_RET(ParseQmdbOper(sConStr,sDsn,sUid,sPwd,32),"the connection str is invalid");
	CHECK_RET(CheckConStrValid(sDsn,sUid,sPwd),"the user is invaid for this dsn");
	
	

    TADD_START(sDsn,"mdbSchema",0,false,false);
    TADD_NORMAL("[BEGIN]");
    
    CHECK_RET(ExptDDL(sDsn,bShowObject,bIncSystable,sTypeName),"ExptDDL failed");
    TADD_NORMAL("[END]");
    return 0;
}

