/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbChkPnt.cpp   
*@Description�� �ֶ�ִ��checkpoint
*@Author:      	miao.jianxin
*@Date��        2016��4��12��
*@History:
******************************************************************************************/

/****************************************************************************************

******************************************************************************************/

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "Helper/mdbStruct.h"
#include "Helper/TThreadLog.h"
#include "Control/mdbStorageEngine.h"
#include "Helper/mdbDateTime.h"
#include "Control/mdbProcCtrl.h"




//using namespace QuickMDB;

void Help()
{
    printf("-------\n"
        		" Usage\n"
        		" mdbChkPnt <dsn>\n"
        		" example: mdbChkPnt DC"
                " -H|-h Print Help.\n"
                
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
	char sUpperDsn[64];
    memset(sUpperDsn,0,sizeof(sUpperDsn));
    SAFESTRCPY(sUpperDsn, sizeof(sUpperDsn), argv[1]);
	TMdbNtcStrFunc::ToUpper(sUpperDsn);
    TADD_START(argv[1],"mdbChkPnt", 0, false,false);
   
    TMdbCheckPoint tMdbCheckPoint;
    CHECK_RET(tMdbCheckPoint.Init(sUpperDsn),"Init failed.");
   
    if(tMdbCheckPoint.NeedLinkFile())
	{
		CHECK_RET(tMdbCheckPoint.LinkFile(),"Attach failed.");
	}
	if(tMdbCheckPoint.DoCheckPoint() != 0)
	{
		TADD_ERROR(-1,"DoCheckPoint Faild");
		return -1;

	}
	    
    return iRet;  

}

