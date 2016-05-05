/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbDataCheck.cpp		
*@Description： 本地mdb与其他mdb或oracle进行数据比较和恢复工具
*@Author:		jiang.lili
*@Date：	    2014年04月
*@History:      
******************************************************************************************/
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "Helper/mdbCommandlineParser.h"
//#include "Helper/mdbProcess.h"

#include "Tools/mdbDataCmp.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

using std::string;

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

void Help()
{
    static char usage[1024] =
        "-------\n"
        " Usage\n"
        "   mdbDataCheck \n"
        "      -c <data_source> -f <config_file> [-d] [-h]  \n"
        " Example\n" 
        "   mdbDataCheck -c db1 -f /ztesoft/qmdbtest/QuickMDB/mdbCheck.xml \n"  
        " Note\n"
        "      -c data source name to compare.\n"
        "      -f configuration file name.\n"
        "      -d show the detail difference.\n"
        "      -H|-h Print Help.\n"
        "-------\n" ;
    printf("%s", usage);
}

int CmdParser(int argc, char* argv[], TMdbDataCheckConfig *pConfig)
{
    int iRet = 0;
    //定义参数格式和选项
    CommandLineParser clp(argc, argv);
    // 指定选项的参数个数，下同
    clp.set_check_condition("-f", 1);
    clp.set_check_condition("-c", 1);
    clp.set_check_condition("-d", 0);

    if(clp.check() == false)
    {
        //Help();
        return -1;
    }

    std::string strCfgFile;
    std::string strDsnName;

    const vector<CommandLineParser::OptArgsPair>& pairs = clp.opt_args_pairs();
    vector<CommandLineParser::OptArgsPair>::const_iterator it;
    for (it = pairs.begin(); it != pairs.end(); ++it)
    {
        const string& opt = (*it)._first;
        const vector<string>& args = (*it)._second;

        if (opt == "-f")
        {
            strCfgFile = args[0];
        }
        else if (opt == "-c")
        {
            strDsnName = args[0];
        }
        else if (opt == "-d")
        {
            TMdbCheckDataMgr::m_bDetail = true;
        }
    }

    if (strCfgFile.size() == 0 || !TMdbNtcFileOper::IsExist(strCfgFile.c_str()))
    {
        printf("Invalid configuration file name [%s].\n", strCfgFile.c_str());
    }
    else
    {
        CHECK_RET(pConfig->Init(strCfgFile, strDsnName), "TMdbDataCheckConfig init failed.");
    }

    return iRet;
}

int main(int argc, char* argv[])
{     
    if(argc == 1 || (argc == 2 && TMdbNtcStrFunc::StrNoCaseCmp(argv[1], "-h") == 0))
    {
        Help();
        return 0;
    }
    //Process::SetProcessSignal();  

    int iRet = 0;
    try
    {
        TMdbDataCheckConfig * pConfig = new (std::nothrow) TMdbDataCheckConfig();
        CHECK_OBJ(pConfig);
        iRet = CmdParser(argc, argv, pConfig);
        if (0 == iRet)
        {
            TADD_START(pConfig->m_tCheckInfo.m_strDsn.c_str(), "mdbDataCheck",  pConfig->m_tCheckInfo.m_iLogLevel, true,false);  
            TMdbCheckDataMgr tDataCheck;
            CHECK_RET(tDataCheck.Init(pConfig), "TMdbCheckData init() failed.");
            CHECK_RET(tDataCheck.Start(), "TMdbCheckData start() failed.");
        }
        else
        {
            Help();
            return iRet;
        }
        SAFE_DELETE(pConfig);
    }
    catch(...)
    {
        printf("Unknown error!\n");       
    }

    return iRet;   
}


