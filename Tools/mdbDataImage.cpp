/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbFlushFromOra.cpp
*@Description： 内存数据库的Oracle刷新程序（从Oracle写入内存数据库）
*@Author:		li.shugang
*@Date：	    2009年05月11日
*@History:
******************************************************************************************/


#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "Tools/mdbImage.h"
//#include "Helper/mdbProcess.h"

//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;


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
    printf(
        "-------\n"
        " Usage:\n"
        "   mdbDataImage  <sDsn>  [export|import]  <img-dir>\n"
        "   mdbDataImage [ -H | -h ] \n"
        " Example:\n"
        "   mdbDataImage ocs  import /ztesoft/ocs/data/ \n"
        "   mdbDataImage ocs  import \n"
        "   mdbDataImage ocs  export\n"
        "   default <img-dir> is [data-store] in config(sys.xml)\n"
        "-------\n" );
}

int main(int argc, char* argv[])
{
    if((argc != 4 && argc != 3)|| TMdbNtcStrFunc::StrNoCaseCmp(argv[1],"-h") == 0 )
    {
        Help();
        return 0;
    }
    int iRet =0;
    char sDsn[MAX_NAME_LEN] = {0};
    char sCmd[MAX_NAME_LEN] = {0};
    char sImgDir[MAX_FILE_NAME] = {0};
    SAFESTRCPY(sDsn,sizeof(sDsn),argv[1]);
    SAFESTRCPY(sCmd,sizeof(sCmd),argv[2]);
    if(argc== 4){SAFESTRCPY(sImgDir,sizeof(sImgDir),argv[3]);}
    TADD_START(sDsn,"mdbDataImage", 0, true,false);
    try
    {
        TMdbImage tImage;
        if(TMdbNtcStrFunc::StrNoCaseCmp(sCmd,"export") == 0)
        {
            CHECK_RET(tImage.Export(sDsn,sImgDir),"Export [%s] failed..",sDsn);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sCmd,"import") == 0)
        {
            CHECK_RET(tImage.Import(sDsn,sImgDir),"Import [%s][%s] failed..",sDsn,sImgDir);
        }
        else
        {
            Help();
            return 0;
         }
    }
    catch(...)
    {
        printf("Unknown error!\n");
    }
    return iRet;
}


