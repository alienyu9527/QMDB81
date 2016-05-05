/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    minidb.cpp
*@Description： 内存数据库的主程序
*@Author:		li.shugang
*@Modify:       jiang.mingjun
*@Date：	    2008年11月25日
*@History:      2009-06-05用户校验改进.格式如:mdb 'oper=create;uid=ocs;pwd=ocs;dsn=mdb'

******************************************************************************************/

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <ctype.h>

#include "Helper/TThreadLog.h"
#include "Helper/mdbDateTime.h"
#include "App/mdbCtrl.h"
#include "App/mdbMonitor.h"
#include "Control/mdbTimeThread.h"
#include "Control/mdbSysTableThread.h"
#include "Helper/mdbOS.h"
//#include "Helper/mdbProcess.h"
//#include "Helper/mdbStrFunc.h"
//#include "Helper/mdbSplit.h"


#ifdef WIN32
#include <conio.h>
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

enum E_QMDB_OPER
{
    QMDB_OPER_CREATE = 1,
    QMDB_OPER_START   = 2,
    QMDB_OPER_STOP     = 3,
    QMDB_OPER_DESTROY = 4,
    QMDB_OPER_CREATE_FROM_DISK = 5, //从本地磁盘加载，使用自己的存储引擎
    QMDB_OPER_FORCE_DESTROY = 6, //强制删除
    QMDB_OPER_UNKNOW = 7
     
};
struct cmdopt
{
    char sOperation[32];
    char sUid[32];
    char sPwd[32];
    char sDsn[32];
    int    iOperType;
};

//QuickMDB  命令类型
enum E_QMDB_CMD
{
    QMDB_CMD_HELP       = 1,    //帮助信息
    QMDB_CMD_VERSION = 2,   //版本信息
    QMDB_CMD_OPER       = 3,   //QMDB操作
    QMDB_CMD_NONE       = 4   //没有操作
};
/******************************************************************************
* 函数名称	:  CheckQmdbOper
* 函数描述	:  检测oper项
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int CheckQmdbOper(struct cmdopt &strValue)
{
    const char * sOpers[] = {"create","start","stop","destroy","create_from_disk","force_destroy"};
    const int    iOpers[] = {QMDB_OPER_CREATE,QMDB_OPER_START,QMDB_OPER_STOP,QMDB_OPER_DESTROY,QMDB_OPER_CREATE_FROM_DISK,QMDB_OPER_FORCE_DESTROY};
    int i = 0;
    for(i = 0;i < 6;++i)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(strValue.sOperation,sOpers[i]) == 0)
        {
               strValue.iOperType = iOpers[i];
               break;
        }
    }
    if(6 == i)
    {
         printf(" You Entered the 'oper'[%s] must in "
         "[create|create_from_disk|destroy|start|stop|force_destroy]'\n\n",strValue.sOperation);
         return ERR_APP_INVALID_PARAM;
    }
    return 0;
}

unsigned char sMdbLower[256] =     {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                                21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                                31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                                41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
                                51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
                                61, 62, 63, 64, 97, 98, 99, 100,101,102,
                                103,104,105,106,107,108,109,110,111,112,
                                113,114,115,116,117,118,119,120,121,122,
                                91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
                                101,102,103,104,105,106,107,108,109,110,
                                111,112,113,114,115,116,117,118,119,120,
                                121,122,123,124,125,126,127,128,129,130,
                                131,132,133,134,135,136,137,138,139,140,
                                141,142,143,144,145,146,147,148,149,150,
                                151,152,153,154,155,156,157,158,159,160,
                                161,162,163,164,165,166,167,168,169,170,
                                171,172,173,174,175,176,177,178,179,180,
                                181,182,183,184,185,186,187,188,189,190,
                                191,192,193,194,195,196,197,198,199,200,
                                201,202,203,204,205,206,207,208,209,210,
                                211,212,213,214,215,216,217,218,219,220,
                                221,222,223,224,225,226,227,228,229,230,
                                231,232,233,234,235,236,237,238,239,240,
                                241,242,243,244,245,246,247,248,249,250,
                                251,252,253,254,255};


/******************************************************************************
* 函数名称	:  ParseQmdbOper
* 函数描述	:  解析qmdb 操作串
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int ParseQmdbOper(const char * sOperStr,struct cmdopt &strValue)
{
    int iRet = 0;
    TMdbNtcSplit tSplit;
    tSplit.SplitString(sOperStr,';');
    int i = 0;
    const char * sPrefix[] = {"dsn=","uid=","pwd=","oper="};
    int iCount = 4;
    char * sContextAddr[] = {strValue.sDsn,strValue.sUid,strValue.sPwd,strValue.sOperation};
    for(i = 0;i < tSplit.GetFieldCount() ;++i)
    {
        int j = 0;
        for(j = 0 ;j < iCount;++j)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(tSplit[i],sPrefix[j],strlen(sPrefix[j])) == 0)
            {//前缀相同
                SAFESTRCPY(sContextAddr[j],32,tSplit[i]+strlen(sPrefix[j]));
                break;
            }
        }
        if(j == iCount)
        {
            printf("[%s] is error\n",tSplit[i]);
            return ERR_APP_INVALID_PARAM;
        }
    }
    if(strlen(strValue.sOperation) != 0)
    {
        CHECK_RET(CheckQmdbOper(strValue),"CheckQmdbOper failed");
    }
    return iRet;
}

/******************************************************************************
* 函数名称	:  Helper
* 函数描述	:  使用帮助
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
void Help(char* argv[])
{
        printf( "-------\n"
        " Usage:\n"
        "   %s  [{<DSN> |  <connStr>}] \n"
        "   %s  [-H | -h ]   \n"
        "   %s  [-V | -v ]   \n"
        " Example:\n"
        "   %s 'dsn=ocs;uid=id;pwd=pd;oper=create' \n"
        "   %s 'dsn=ocs'  \n"
        "   %s  ocs    \n"
        "   %s  -V    \n"
        " Note:\n"
        "   <DSN>, <connStr>:\n"
        "        The ODBC connection string or DSN to use as an\n"
        "        argument to the connect command. The connect command\n"
        "        will then be the first command executed when\n"
        "        starting QuickMDB.\n"
        "   connStr argument: \n"
        "        oper : you can select" 
        "<create|stop|destroy|start|force_destroy> \n"
        "        uid  : MDB user ID\n"
        "        pwd  : MDB user password \n"
        "        dsn  : data source\n"
        "   -H|-h  Print Help.\n"
        "   -H|-h  Print Version Information.\n"
        "-------\n" ,argv[0],argv[0],argv[0],argv[0],argv[0],argv[0],argv[0]);
}

/******************************************************************************
* 函数名称	:  ShowVersion
* 函数描述	:  显示版本信息
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
void ShowVersion()
{
	char *ptrEnv = getenv("QuickMDB_HOME");
	char fileName[MAX_FILE_NAME] = {0};
	char strVer[128] = {0};
	snprintf(fileName,sizeof(fileName),"%s/bin/mdbVersion",ptrEnv);
	
	FILE* pFile = NULL;
	pFile = fopen(fileName,"r");
	if(pFile != NULL)
	{
		char *ch =NULL;
		fgets(strVer,sizeof(strVer), pFile);
		SAFE_CLOSE(pFile);
		pFile = NULL;
		ch = strchr(strVer,'\n');
		if(ch != NULL)
			*ch = '\0';
	}
		printf("\n"
           "========================================================================================\n"
           "* Internal Version : %s                                                             *\n"
		   "* Release Version : %s                                                             *\n"
		   "* Copyright (c) 2011--2020, ZTEsoft                                                    *\n"
           "========================================================================================\n\n",MDB_VERSION,strVer);
}
/******************************************************************************
* 函数名称	:  InteractiveInput
* 函数描述	:  交互式验证输入
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
int InteractiveInput(struct cmdopt &strValue,TMdbConfig * &pConfig)
{
    int iCount = 0;
    int iRet = 0;
    CHECK_OBJ(pConfig);
    while(true)
    {
        printf("\n\n");
        cout<<"Connect DSN="<<strValue.sDsn<<endl;
        cout<<"Please input user_name : ";
        cin>>strValue.sUid;
        cout<<"Please input pass_word : ";
        cin>>strValue.sPwd;
        do
        {
            cout<<"Please input operation :";
            cin>>strValue.sOperation;
        }while(CheckQmdbOper(strValue) != 0);//直到输入正确的oper为止
        iRet = pConfig->CheckParam(strValue.sUid, strValue.sPwd, strValue.sDsn);
        iCount++;
        if(iRet < 0)
        {
            if(iCount>=3)
            {
                printf("\n\nExceed the maximum input times\n\n");
                return -1;
            }
            else
            {
                printf("\n\nYour user_name or pass_word may be wrong!\n\n");
                continue;
            }
        }
        else
        {
            printf("connect 'dsn=%s;uid=%s;pwd=%s;'",strValue.sDsn,strValue.sUid, strValue.sPwd);
            printf("\n\nSuccess!!\n\n");
            break;
        }
    }
    return 0;
}
/******************************************************************************
* 函数名称	:  CmdLineParse
* 函数描述	:  命令行解析
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int CmdLineParse(int argc, char* argv[],struct cmdopt &strValue)
{
    int iRet = 0;
    if(2 == argc && TMdbNtcStrFunc::StrNoCaseCmp(argv[1],"-v") == 0)
    {//版本信息
        return QMDB_CMD_VERSION;
    }
    else if(argc < 2|| TMdbNtcStrFunc::StrNoCaseCmp(argv[1],"-h") == 0)
    {
        return QMDB_CMD_HELP;
    }
    if(TMdbNtcStrFunc::FindString(argv[1],"=") == -1)
    {//QuickMDB ocs 的情况
        SAFESTRCPY(strValue.sDsn,sizeof(strValue.sDsn),argv[1]);
    }
    else if(ParseQmdbOper(argv[1],strValue) != 0 || 0 == strValue.sDsn[0])
    {//解析错误
        return QMDB_CMD_HELP;
    }
    
    //上载SYS配置文件
    TMdbConfig *pMdbConfig = TMdbConfigMgr::GetDsnConfig(strValue.sDsn);
    CHECK_OBJ(pMdbConfig);
    if(strlen(strValue.sUid) == 0 
        || strlen(strValue.sPwd) == 0 
        || strlen(strValue.sOperation) == 0)
    {//缺少输入
        CHECK_RET(InteractiveInput(strValue,pMdbConfig),"input uid  password error.");
    }
    else
    {
        CHECK_RET(pMdbConfig->CheckParam(strValue.sUid, strValue.sPwd, strValue.sDsn), "input uid or  password error.");
    }
    //校验用户权限
    if(pMdbConfig->GetAccess() != MDB_ADMIN)
    {
        CHECK_RET(ERR_DB_PRIVILEGE_INVALID,"No admin access=[%c].",pMdbConfig->GetAccess());
    }
    return 0== iRet?QMDB_CMD_OPER:iRet;
}

/******************************************************************************
* 函数名称	:  OperQmdb
* 函数描述	:  操作数据库,create,start,stop,destroy
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int OperQmdb(struct cmdopt &strValue)
{
//    Process::SetProcessSignal();
    int iRet = 0;
    TMdbCtrl dbCtrl;
    switch(strValue.iOperType)
    {
        case QMDB_OPER_CREATE_FROM_DISK:
                CHECK_RET(dbCtrl.Init(strValue.sDsn),"dbCtrl.Init() failed.");
                dbCtrl.SetLoadFromDisk(true);   //设置从磁盘加载
                if(dbCtrl.IsMdbCreate())
                {
                    printf("share memery is exist ,qmdb is created or dsn value is conflict \n");
                    return ERR_OS_SHM_EXIST;
                }
                TADD_START(strValue.sDsn,"QuickMDB", 0, true,false);
                
                iRet = dbCtrl.Create();
#ifdef WIN32
                //让代码在后台运行,不然当create进程退出的时候
                //共享内存会跟着消失.winwos下的机制跟unix下不一样.
                const char *p =  "ConsoleWindowClass";
                HWND hwnd = FindWindow((LPCWSTR)p,NULL);
                if(hwnd != NULL)
                    ShowWindow(hwnd,SW_HIDE);
                getch();
#endif
            break;
        case QMDB_OPER_CREATE:
                CHECK_RET(dbCtrl.Init(strValue.sDsn),"dbCtrl.Init() failed.");
                dbCtrl.SetLoadFromDisk(false);
                if(dbCtrl.IsMdbCreate())
                {
                    printf("share memery is exist ,qmdb is created or dsn value is conflict \n");
                    return ERR_OS_SHM_EXIST;
                }
                TADD_START(strValue.sDsn,"QuickMDB", 0, true,false);
                
                iRet = dbCtrl.Create();
#ifdef WIN32
                //让代码在后台运行,不然当create进程退出的时候
                //共享内存会跟着消失.winwos下的机制跟unix下不一样.
                const char *p =  "ConsoleWindowClass";
                HWND hwnd = FindWindow((LPCWSTR)p,NULL);
                if(hwnd != NULL)
                    ShowWindow(hwnd,SW_HIDE);
                getch();
#endif
            break;
        case QMDB_OPER_START:
           {
                setsid();
                pid_t pid;
                if((pid = fork() )== -1)
                {
                    printf("fork error \n");
                    exit(-1);
                }
                if(pid > 0)
                {
                    exit(0); //退出父进程
                }
                TADD_START(strValue.sDsn,"QuickMDB", 0, true,false);
                CHECK_RET(dbCtrl.Init(strValue.sDsn),"dbCtrl.Init() failed.");
                if(dbCtrl.IsMdbCreate() == false)
                {
                    CHECK_RET(ERR_DB_NOT_CREATE,"QuickMDB is not created,please create first.");
                }
                CHECK_RET(dbCtrl.Start(),"Start Failed.");
                TMdbMonitor tMonitor;
                CHECK_RET(tMonitor.Init(strValue.sDsn),"tMonitor.Init failed.");
                CHECK_RET(tMonitor.Start(),"tMonitor.Start failed.");
                TADD_NORMAL("\nQuickMDB is down. Process=[%d]",getpid());
            }
            break;
        case QMDB_OPER_STOP:  
            TADD_START(strValue.sDsn,"QuickMDB", 0, true,false);
            iRet = dbCtrl.Stop(strValue.sDsn);
            break;
        case QMDB_OPER_DESTROY:
            TADD_START(strValue.sDsn,"QuickMDB", 0, true,false);
            iRet = dbCtrl.Destroy(strValue.sDsn);
            TMdbDateTime::MSleep(500);
            break;
        case QMDB_OPER_FORCE_DESTROY:
            TADD_START(strValue.sDsn,"QuickMDB", 0, true,false);
            iRet = dbCtrl.Destroy(strValue.sDsn, true);
            TMdbDateTime::MSleep(500);
            break;
            
        default:
            break;
    }
    return iRet;
}
/******************************************************************************
* 函数名称	:  ChangeProcName
* 函数描述	:  修改进程参数名
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  dongchun
*******************************************************************************/
int ChangeProcName(char* argv[])
{
    int iRet = 0;
    TMdbNtcSplit tSplit;
    tSplit.SplitString(argv[1],';');
    struct cmdopt strValue; 
    int i = 0;
    const char * sPrefix[] = {"dsn=","uid=","pwd=","oper="};
    int iCount = 4;
    char * sContextAddr[] = {strValue.sDsn,strValue.sUid,strValue.sPwd,strValue.sOperation};
    for(i = 0;i < tSplit.GetFieldCount() ;++i)
    {
        int j = 0;
        for(j = 0 ;j < iCount;++j)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(tSplit[i],sPrefix[j],strlen(sPrefix[j])) == 0)
            {//前缀相同
                SAFESTRCPY(sContextAddr[j],32,tSplit[i]+strlen(sPrefix[j]));
                break;
            }
        }
        if(j == iCount)
        {
            printf("[%s] is error\n",tSplit[i]);
            return ERR_APP_INVALID_PARAM;
        }
    }
    
    int iLen = strlen(strValue.sPwd);
    for(int m = 0; m<iLen;m++)
    {
        strValue.sPwd[m] = '*';
    }

    char sTemp[245];
    memset(sTemp,0,sizeof(sTemp));
    snprintf(sTemp,sizeof(sTemp),"dsn=%s;uid=%s;pwd=%s;oper=%s",strValue.sDsn,strValue.sUid,strValue.sPwd,strValue.sOperation);
    //printf("stemp=%s",sTemp);
    int len = strlen(argv[1]);
    int isTempLen= strlen(sTemp);
    if(len < isTempLen)
    {
        printf("argv len =[%d],change len=[%d] error\n",len,isTempLen);
        return ERR_APP_INVALID_PARAM;
    }
    SAFESTRCPY(argv[1],len+1,sTemp);
    argv[1][isTempLen] = 0;
    return iRet;
}

/******************************************************************************
* 函数名称	:  main
* 函数描述	:  主进程入口
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  
*******************************************************************************/
int main(int argc, char* argv[])
{
    int iRet = 0;
    struct cmdopt strValue;
    memset(&strValue,0,sizeof(strValue));
    switch(CmdLineParse(argc,argv,strValue))
    {
        case QMDB_CMD_HELP:
            Help(argv);
            break;
        case QMDB_CMD_VERSION:
            ShowVersion();
            break;
        case QMDB_CMD_OPER://验证通过
            iRet = ChangeProcName(argv);
            iRet = OperQmdb(strValue);
            break;
        default:
            Help(argv);
            break;
    }
    return iRet;
}


