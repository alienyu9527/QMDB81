/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbObserve.cpp
*@Description： mdb的设置观测点
*@Author:		jin.shaohua
*@Date：	    2012.10
*@History:
******************************************************************************************/
#include "Helper/TThreadLog.h"
#include "Control/mdbObserveCtrl.h"
#include "Control/mdbMgrShm.h"


//namespace QuickMDB{

    //输出观测点信息
    int ShowObservePoint(char* argv[])
    {
        TObserveMgr  tObMgr;
        tObMgr.ShowAllObservePoint(argv[1]);
        return 0;
    }

    //设置观测点
    int SetObservePoint(char* argv[])
    {
        int iRet = 0;
        char * sDsn = argv[1];
        TMdbShmDSN * pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(pShmDsn);
        TObservePoint * pObPoint = pShmDsn->GetObPiontByName(argv[2]);
        CHECK_OBJ(pObPoint);
        TObserveMgr  tObMgr;
        TObserveBase * pBase = tObMgr.GetObserveInst(pObPoint->m_iType);
        CHECK_OBJ(pBase);
        CHECK_RET(pBase->Init(sDsn,pObPoint->m_iType),"int[%s,%d] error.",sDsn,pObPoint->m_iType);
        TADD_NORMAL("Parse param[%s]",argv[4]);
        //先检测参数
        CHECK_RET(pBase->ParseParam(argv[4]),"parse Param[%s] error.",argv[4]);
        SAFESTRCPY(pObPoint->m_sParam,sizeof(pObPoint->m_sParam),argv[4]);
        //检测结束时间
        if(TMdbNtcStrFunc::IsDigital(argv[3]))
        {
            long long  llObSec = TMdbNtcStrFunc::StrToInt(argv[3]);
            if(llObSec <= 0)
            {//停止观测
                pBase->StopObServe();
            }
            else
            {//设置观测时间
                gettimeofday(&(pObPoint->m_tTerminal), NULL);
                pObPoint->m_tTerminal.tv_sec += llObSec;
            }
        }
        else
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"End-time[%s] is error.",argv[3]);
        }
        ShowObservePoint(argv);
        return iRet;
    }


//}


void Help(char* argv[])
{
  printf("%s <DSN>  #show all observe point \n",argv[0]);
  printf("%s <DSN> <observe_point> <time(sec)> <params> #set one observe point \n",argv[0]);
  printf("example:\n");
  printf("\t %s ocs \n",argv[0]);
  printf("\t mdbObserve ocs ob_table_exec 36000 '--filename={table_name}-{table_id}-{pid}-{tid}.txt --filesize=16 --tableids=1,2,457 --flushcycle=1|10'\n");
  printf("\t mdbObserve ocs ob_table_exec 36000 '--filename={table_name}-{table_id}.txt --tableids=1,2,457 --flushcycle=2|128'\n");
  printf("\t mdbObserve ocs ob_table_exec 0 '' \n");
}

//using namespace QuickMDB;

int main(int argc, char* argv[])
{
	TADD_START(argv[1],"mdbObserve", 0, true,false);
	switch(argc)
	{
	case 2:
		ShowObservePoint(argv);
		break;
	case 5:
		SetObservePoint(argv);
		break;
	default:
		 printf("Invalid parameters.\n ");
        Help(argv);
		break;
	}
    	return 0;
}

