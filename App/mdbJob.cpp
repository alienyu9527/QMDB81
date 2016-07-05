/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbJob .cpp		
*@Description： mdb job程序
*@Author:	jin.shaohua
*@Date：	    2013.4
*@History:
******************************************************************************************/

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "Helper/TThreadLog.h"
#include "Helper/mdbCommandlineParser.h"
#include "Control/mdbJobCtrl.h"

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


void Help()
{
	printf("-------\n"
					" Usage:\n"
					"	 <sDsn> \n"
					"	 <sDsn> -k <jobname>\n"
					"	 [ -H | -h ] \n"
					" Example:\n" 
					"	 Check \n"			  
					" Note:\n"
					"	  <sDsn>: data source.\n"
					"	  -H|-h Print Help.\n"
					"-------\n");




}

int main(int argc, char* argv[])
{  
	int iRet = 0;
    if(argc < 2 || strcmp(argv[1],"-h")==0||strcmp(argv[1],"-H")==0)
    {
        Help();
        return iRet;
    }
    //定义参数格式和选项
    CommandLineParser clp(argc, argv);
    // 指定选项的参数个数，下同
	clp.set_check_condition("-h", 0);
	clp.set_check_condition(argv[1], 0);
	clp.set_check_condition("-k", 1);

    if(clp.check() == false)
	{
    	Help();
        return 0;
	}

    const vector<CommandLineParser::OptArgsPair>& pairs = clp.opt_args_pairs();
	vector<CommandLineParser::OptArgsPair>::const_iterator it;
	for (it = pairs.begin(); it != pairs.end(); ++it)
	{
		const string& opt = (*it)._first;
		const vector<string>& args = (*it)._second;
		
		if(opt == "-h" || opt == "-H")
		{
        	Help();
        	return 0;
		}

		//启动job进程
		if(opt == argv[0] && argc == 2)
		{
			setsid();
    		if (fork() > 0) exit(0); /* run in backgroud */
            char sName[64];
		    memset(sName, 0, sizeof(sName));
		    sprintf(sName, "%s %s", argv[0], argv[1]);
		    TADD_START(argv[1],sName, 0,false ,true);
		    int iRet = 0;
		    TMdbJobCtrl tJobCtrl;
		    CHECK_RET(tJobCtrl.Init(argv[1]),"tJobCtrl init failed.");
		    CHECK_RET(tJobCtrl.StartJob(),"Start faild.");
			return iRet;
		}

		//中断执行中的job
		if(opt == "-k" && argc == 4)
		{
			printf("kill Job : dsn[%s] JobName[%s].\n",argv[1],args[0].c_str());
			TMdbJobCtrl tJobCtrl;
		    CHECK_RET(tJobCtrl.Init(argv[1]),"tJobCtrl init failed.");
			CHECK_RET(tJobCtrl.SetCancel(args[0].c_str()),"SetCancel faild.");
			
		}
	}
    return 0;   
}


