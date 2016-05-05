/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbSize.cpp   
*@Description： 内存使用显示
*@Author:       wang.liebao
*@Date：        20150707
*@History:
******************************************************************************************/


#include <stdio.h>
#include <stdlib.h>

#include "Helper/mdbStruct.h"
#include "Helper/TThreadLog.h"
//#include "Helper/mdbStrFunc.h"
#include "Tools/mdbInfo.h"
#include "Helper/mdbCommandlineParser.h"


void Help()
{
    printf("-------\n"
        		" Usage\n"
        		"   mdbSize \n"
        		"      -c sDsn    -t [tablename]\n"
                "                 -t tablename -r recordcount \n"
                "                 -m \n"
                "                 -m -d \n"
                "                 -m -d -t \n"
                " Example\n" 
                "   mdbSize \n"  
                "      -c ocs -t bal\n" 
                "      -c ocs -t bal -r 100000\n" 
                "      -c ocs -t \n" 
                "      -c ocs -m \n" 
                "      -c ocs -m -d \n"
                "      -c ocs -m -d -t \n" 
                " Note\n"
                "      -c data source.\n"
                "      -t tablename in qmdb,print mem cost of table,\n"
                "         or print all tables in qmdb with no table specified.\n"
                "      -r recordcount,print mem cost of table with the recordcount when used with option -t.\n"
                "         table's total recordcount will be used when -r isn't set.\n"
                "      -m print mem cost of resource.\n"
                "      -d print detail info of resource with -m.\n"
                "      -H|-h Print Help.\n"
                
        		"-------\n");

}

int main(int argc, char* argv[])
{
    int iRet = 0;
    if(argc < 4 || strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        Help();
        return iRet;
    }

    
    //定义参数格式和选项
    CommandLineParser clp(argc, argv);
    //clp.set_check_condition("", 7);					// 指定程序的参数个数
    // 指定选项的参数个数，下同
	clp.set_check_condition("-d", 0);
	clp.set_check_condition("-h", 0);
	clp.set_check_condition("-c", 1);
	clp.set_check_condition("-m", 0);
	clp.set_check_condition("-r", 1);
	clp.set_check_condition("-t", 0 ,1);

    if(clp.check() == false)
	{
    	Help();
        return 0;
	}
	
	char sTableName[MAX_NAME_LEN]       ={0};
	char sConn[MAX_NAME_LEN]            ={0};
	bool bResource  = false;
	bool bDetail    = false;
	bool bTable     = false;
	int  iCount     = 0;
    const vector<CommandLineParser::OptArgsPair>& pairs = clp.opt_args_pairs();
	vector<CommandLineParser::OptArgsPair>::const_iterator it;
	for (it = pairs.begin(); it != pairs.end(); ++it)
	{
		const string& opt = (*it)._first;
		const vector<string>& args = (*it)._second;
		
		if(opt == "-t")
		{
		    if(args.size() > 0 && TMdbNtcStrFunc::StrNoCaseCmp(args[0].c_str(),"all")!=0)
		    {
                SAFESTRCPY(sTableName,sizeof(sTableName),args[0].c_str());
    			sTableName[strlen(sTableName)]='\0';
			}
			bTable = true;
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
		if(opt == "-r")
		{
		    iCount = TMdbNtcStrFunc::StrToInt(args[0].c_str());
		    if(iCount <= 0)
		    {
		        printf("\ninvalid recordcount,it must >0 !! \n");
		        return 0;
		    }
			continue;
		}
		
        if(opt == "-m")
        {
            bResource = true;
            continue;
        }
        if(opt == "-d")
        {
            bDetail = true;
            continue;
        }
    }

    TADD_START(sConn,"mdbSize",0,false,false);
    try
    {
        TMdbSizeInfo tMdbSizeInfo;
        CHECK_RET(tMdbSizeInfo.Init(sConn),"Failed to initialize.");
        if(bResource)
        {
           tMdbSizeInfo.PrintResourceInfo(bDetail);
        }
        if(bTable)
        {
            tMdbSizeInfo.PrintTableInfo(sTableName,iCount);
        }
        
    }
    catch(...)
    {
        printf("UnKown error!\n");       
    }
    
    return 0;
}

