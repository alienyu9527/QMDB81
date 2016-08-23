/****************************************************************************************
*@Copyrights  2011，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbClean.cpp
*@Description： 内存数据库的Oracle刷新程序清理工作
*@Author:		zhang.lin
*@Date：	    2011年12月06日
*@History:
******************************************************************************************/

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <ctype.h>

#include "Helper/TThreadLog.h"
#include "Helper/mdbDateTime.h"
#include "Helper/TDBFactory.h"
#include "App/mdbCtrl.h"
#include "Control/mdbTimeThread.h"
#include "Control/mdbSysTableThread.h"
#include "Helper/mdbOS.h"


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

// %DSN%_mdb_change_notif表记录数告警阀值
#define MAX_CHANGE_NOTIF_CNT 100000

TMdbConfig *gConfig = NULL;
TMDBDBInterface *gDBLink = NULL ;
TMdbShmDSN *gShmDSN = NULL ;
TMdbDSN    *gDsn= NULL ;
TMdbProcCtrl gtProcCtrl;
TMDBDBQueryInterface  *gOracleQuery = NULL;
TMDBDBQueryInterface  *gOracleClean = NULL;
TMDBDBQueryInterface  *gQryCnt = NULL;

int ConnectOracle()
{
    try
    {
        gDBLink = TMDBDBFactory::CeatDB();
        if(gDBLink == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
        gDBLink->SetLogin(gConfig->GetDSN()->sOracleUID, gConfig->GetDSN()->sOraclePWD, gConfig->GetDSN()->sOracleID);
        bool bFlag = gDBLink->Connect();
        if (false == bFlag)
        {
            TADD_ERROR(ERROR_UNKNOWN,"connect to oracle error! ");
            return -1;
        }

        gOracleQuery = gDBLink->CreateDBQuery();
        if(gOracleQuery == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"no OracleQuery");
            return -1;
        }
        gOracleClean = gDBLink->CreateDBQuery();
        if(gOracleClean == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"no OracleCleanQuery");
            return -1;
        }

        gQryCnt = gDBLink->CreateDBQuery();
        if(gQryCnt == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"no gQryCnt");
            return -1;
        }
    }
    catch(TMDBDBExcpInterface &e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"SQL=[%s], error_msg=[%s].",  e.GetErrSql(), e.GetErrMsg());
        return ERROR_UNKNOWN;
    }
    return 0;
}

//清理oracle链接
void ClearOracleLink()
{
    SAFE_DELETE(gQryCnt);
    SAFE_DELETE(gOracleQuery);
    SAFE_DELETE(gOracleClean);
    SAFE_DELETE(gDBLink);
}
//重新连接数据库
int ReConnectDB()
{
	if(gDBLink->IsConnect())
	{
		return 0;
	}
    int iWaitSec = 60;//60秒重连一次
    ClearOracleLink();
    while(ConnectOracle() != 0)
    {
        TADD_NORMAL("ReConnectDB failed ....wait(%d) ",iWaitSec);
        TMdbDateTime::Sleep(iWaitSec);
    }
    return 0;
}
int GetTableName(char* sMdbChangeNotifySeqName,char* sMdbChangeNotifyName,char* sDsnName)
{
	char sChangeNotifySeqSQL[MAX_SQL_LEN];
	char sChangeNotifySQL[MAX_SQL_LEN];
	memset(sChangeNotifySeqSQL,0,sizeof(sChangeNotifySeqSQL));
	memset(sChangeNotifySQL,0,sizeof(sChangeNotifySQL));
	sprintf(sChangeNotifySeqSQL, "select TABLE_SEQUENCE from %s_MDB_CHANGE_NOTIFY_SEQ",sDsnName);
	sprintf(sChangeNotifySQL, "select TABLE_SEQUENCE from %s_MDB_CHANGE_NOTIF",sDsnName);
    try
    {
        CHECK_OBJ(gOracleQuery);
    	gOracleQuery->Close();
		gOracleQuery->Commit();
		gOracleQuery->Close();
		gOracleQuery->SetSQL(sChangeNotifySeqSQL);
    	gOracleQuery->Open();
		if(gOracleQuery->Next())
		{
		}
		snprintf(sMdbChangeNotifySeqName,MAX_NAME_LEN,"%s_MDB_CHANGE_NOTIFY_SEQ",sDsnName);
    }
    catch(TMDBDBExcpInterface& e)
    {
    	snprintf(sMdbChangeNotifySeqName,MAX_NAME_LEN,"MDB_CHANGE_NOTIFY_SEQ");
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Init failed : Unknown error!\n");
        TADD_FUNC("Finish.");
        return -1;
    }

    try
    {
    	gOracleQuery->Close();
		gOracleQuery->SetSQL(sChangeNotifySQL);
    	gOracleQuery->Open();
		if(gOracleQuery->Next())
		{
		}
		snprintf(sMdbChangeNotifyName,MAX_NAME_LEN,"%s_MDB_CHANGE_NOTIF",sDsnName);
    }
    catch(TMDBDBExcpInterface& e)
    {
    	snprintf(sMdbChangeNotifyName,MAX_NAME_LEN,"MDB_CHANGE_NOTIF");
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Init failed : Unknown error!\n");
        TADD_FUNC("Finish.");
        return -1;
    }
	
	//TADD_NORMAL("m_sChangeNotifyName=[%s],m_sChangeNotifySeqName=[%s]\n",sMdbChangeNotifyName,sMdbChangeNotifySeqName);
	return 0;
}


bool CheckRepeatDel(const char* psTabName, const MDB_INT64 iCurSeq, std::map<string, MDB_INT64>& tTabSeqMap)
{
    TADD_FUNC("Start.");
    bool bRet = false;

    if(NULL == psTabName)
    {
        TADD_ERROR(ERROR_UNKNOWN,"table name is null.");
        return true;
    }

    std::map<string,MDB_INT64>::iterator iter = tTabSeqMap.begin();
    for(;iter != tTabSeqMap.end();++iter)
    {
        if(iter->first == psTabName)
        {
            
            if(iter->second >= iCurSeq)
            {
                TADD_DETAIL("Table[%s], last  table_sequence =[%lld], current table_sequence = [%lld], will not excute clean."
                        ,psTabName, iter->second, iCurSeq);
                bRet = true;
            }
            break;
        }
    }
    
    TADD_FUNC("Finish.");
    return bRet;
}

void SaveLastDelSeq(const char* psTabName, const MDB_INT64 iCurSeq, const MDB_INT64 iRowAffected,std::map<string, MDB_INT64>& tTabSeqMap)
{
    TADD_FUNC("Start.");
    TADD_DETAIL("iCurSeq=[%lld], iRowAffected=[%lld]", iCurSeq, iRowAffected);

    if(NULL == psTabName)
    {
        TADD_ERROR(ERROR_UNKNOWN,"table name is null.");
        return ;
    }

    MDB_INT64 iSaveSeq = 0;
    bool bFind = false;

    std::map<string,MDB_INT64>::iterator iter = tTabSeqMap.begin();
    for(;iter != tTabSeqMap.end();++iter)
    {
        if(iter->first == psTabName)
        {
            // 未满足延时时间的记录不会被删除，因此存在实际删除记录数少的情况
            if(iRowAffected < iCurSeq - iter->second)
            {
                TADD_DETAIL("rowaffected(%lld) < iMin(%lld) -iLast(%lld)", iRowAffected, iCurSeq, iter->second);
                iSaveSeq = iter->second + iRowAffected;
            }
            else
            {
                TADD_DETAIL("rowaffected(%lld) = iMin(%lld) -iLast(%lld)", iRowAffected, iCurSeq, iter->second);
                iSaveSeq = iCurSeq;
            }
            
            tTabSeqMap[iter->first] = iSaveSeq;
            TADD_DETAIL("Table[%s] save last delete  table_sequence =[%lld].",psTabName,  iSaveSeq);
            bFind  = true;
            break;
        }
    }

    if(bFind == false)
    {
        tTabSeqMap[psTabName] = iCurSeq;
        TADD_DETAIL("Add new  Table[%s] info,set last=[%lld].",psTabName, iCurSeq);
    }
    
    TADD_FUNC("Finish.");
    return ;
}


int CleanData()
{
    int iRet = 0;
    TADD_NORMAL("mdbClean::CleanData() : Start.");
    try
    {
        char sTableName[64];
        memset(sTableName,0,sizeof(sTableName));
        char sSQL[512];
        memset(sSQL,0,sizeof(sSQL));
        char sDeleteSQL[512];
        memset(sDeleteSQL,0,sizeof(sDeleteSQL));
        char sDeleteOverdur[512];
        memset(sDeleteOverdur,0,sizeof(sDeleteOverdur));
        
        char sQryCntSQL[512];
        memset(sQryCntSQL, 0, sizeof(sQryCntSQL));

        memset(sSQL,0,sizeof(sSQL));
        char sUpperDsn[64];
        memset(sUpperDsn,0,sizeof(sUpperDsn));
        SAFESTRCPY(sUpperDsn, sizeof(sUpperDsn), TMdbNtcStrFunc::ToUpper(gDsn->sName));
		char sMdbChangeNotifySeqName[MAX_NAME_LEN];
		char sMdbChangeNotifyName[MAX_NAME_LEN];
		memset(sMdbChangeNotifySeqName,0,sizeof(sMdbChangeNotifySeqName));
		memset(sMdbChangeNotifyName,0,sizeof(sMdbChangeNotifyName));
		CHECK_RET(GetTableName(sMdbChangeNotifySeqName,sMdbChangeNotifyName,sUpperDsn),"GetTableName Faild");
        sprintf(sSQL,"select table_name,min(table_sequence) min from %s group by table_name ",sMdbChangeNotifySeqName);

        memset(sDeleteSQL,0,sizeof(sDeleteSQL));
        memset(sDeleteOverdur,0,sizeof(sDeleteOverdur));
        #ifdef DB_ORACLE
            sprintf(sDeleteSQL,"delete from %s where lower(table_name) = lower(:table_name) and table_sequence <= :table_sequence and update_time < (sysdate - :delay_time/(24*3600)) ",sMdbChangeNotifyName);
            sprintf(sDeleteOverdur,"delete from %s where update_time < (sysdate - interval '3' day) ",sMdbChangeNotifySeqName);
        #elif DB_MYSQL  
            sprintf(sDeleteSQL,"delete from %s where lower(table_name) = lower(:table_name) and table_sequence <= :table_sequence and update_time < (sysdate - interval :delay_time second) ",sMdbChangeNotifyName);
            sprintf(sDeleteOverdur,"delete from %s where update_time < (sysdate - interval '3' day) ",sMdbChangeNotifySeqName);
        #else        
            TADD_ERROR(ERROR_UNKNOWN, "[%s:%d]  DB_TYPE is wrong, check Makefile",__FILE__,__LINE__); 
            return -1;
        #endif
        
        memset(sQryCntSQL, 0, sizeof(sQryCntSQL));
        sprintf(sQryCntSQL, "select count(1) counts from %s ", sMdbChangeNotifyName);

        int iCount =0;
        int iDealyTime  = gConfig->GetDSN()->m_iOraRepDelaySec + gConfig->GetDSN()->m_iOraRepInterval;

        TADD_NORMAL(" Clean time(s) : %d ,iDealyTime(s) : %d", gConfig->GetDSN()->iCleanTime *60, iDealyTime);
        
        std::map<string, MDB_INT64> mTabDelSeqMap; // 记录表每次刷新的table_sequence
        mTabDelSeqMap.clear();
        
        while(1)
        {
            if(NULL == gOracleQuery)
            {
                TADD_ERROR(ERROR_UNKNOWN,"mdbClean::CleanData() : m_pOracleQuery == NULL");
                return -1;
            }
            if(gtProcCtrl.IsCurProcStop())
            {
                TADD_NORMAL("Stop.");
                break;
            }
            gtProcCtrl.UpdateProcHeart(0);

            gQryCnt->Close();
			gQryCnt->Commit();
			
			gQryCnt->Close();
            gQryCnt->SetSQL(sQryCntSQL);
            gQryCnt->Open();
            gQryCnt->Next();
            iCount = gQryCnt->Field("counts").AsInteger();
            if(iCount >= MAX_CHANGE_NOTIF_CNT)
            {
                TADD_WARNING("%s record count[%d] is more than %d, pls check!", sMdbChangeNotifyName, iCount, MAX_CHANGE_NOTIF_CNT);
            }

            //超过3天未刷新的记录认为是无效的，为了不影响其他主机的刷新，直接删除
            gOracleClean->Close();
            gOracleClean->SetSQL(sDeleteOverdur);
            gOracleClean->Execute();
            gOracleClean->Commit();
            int iOverDueCnt = gOracleClean->RowsAffected();
            if(iOverDueCnt > 0)
            {
                TADD_NORMAL("clean %d overdue record.", iOverDueCnt);
            }
            
            gOracleQuery->Close();
			gOracleQuery->Commit();
            gOracleQuery->Close();
            gOracleQuery->SetSQL(sSQL);
            gOracleQuery->Open();
            memset(sTableName,0,sizeof(sTableName));
            long long iMin = 0;

            while(gOracleQuery->Next())
            {
                //strcpy(sTableName, m_pOracleQuery->Field("table_name").AsString());
                SAFESTRCPY(sTableName,sizeof(sTableName),gOracleQuery->Field("table_name").AsString());
                iMin = gOracleQuery->Field("min").AsInteger();

                // 如果上一次该表的min(table_sequence)与此次的一样，避免执行无意义的删除
                if(true == CheckRepeatDel(sTableName, iMin, mTabDelSeqMap))
                {
                    continue;
                }
                
                // clean data
                gOracleClean->Close();
                gOracleClean->SetSQL(sDeleteSQL);
                gOracleClean->SetParameter("table_name",sTableName);
                gOracleClean->SetParameter("table_sequence",iMin);
                gOracleClean->SetParameter("delay_time", iDealyTime);
                gOracleClean->Execute();
                gOracleClean->Commit();

                MDB_INT64 iRwAff = gOracleClean->RowsAffected();
                if(iRwAff > 0)
                {
                    SaveLastDelSeq(sTableName, iMin,  iRwAff,mTabDelSeqMap);
                    TADD_NORMAL("Clean :Table[%s], min(table_sequence):[%lld], delay_time=[%d], RowsAffected=%d.", sTableName,iMin, iDealyTime,gOracleClean->RowsAffected());
                }
                else
                {
                    TADD_DETAIL("Clean :Table[%s]:no row deleted. may wait for delay time.( min(table_sequence):[%lld], delay_time=[%d]).", sTableName,iMin, iDealyTime);
                }
                
            }
            
            gtProcCtrl.UpdateProcHeart(gConfig->GetDSN()->iCleanTime * 60);
        }//end while
    }
    catch(TMDBDBExcpInterface& e)
    {
        CHECK_RET(-1,"Init failed : ERROR_SQL=%s.ERROR_MSG=%s", e.GetErrSql(), e.GetErrMsg());
    }
    catch(...)
    {
        CHECK_RET(-1,"Init failed : Unknown error!");
    }
    return iRet;
}

// ****** clean in shadow mode ******
int GetShadowTableName(char* sNotifySeqName,char* sShadowName,char* sDsnName)
{
    TADD_FUNC("Start");

    TADD_DETAIL("DSN=[%s]", sDsnName);
    sprintf(sShadowName, "QMDB_SHADOW_%s", sDsnName);

    char sNotifySeqSQL[MAX_SQL_LEN];
    memset(sNotifySeqSQL,0,sizeof(sNotifySeqSQL));
    sprintf(sNotifySeqSQL, "select TABLE_SEQUENCE from %s_MDB_CHANGE_NOTIFY_SEQ",sDsnName);

    try
    {
        CHECK_OBJ(gOracleQuery);
        gOracleQuery->Close();
		gOracleQuery->Commit();
        gOracleQuery->Close();
        gOracleQuery->SetSQL(sNotifySeqSQL);
        gOracleQuery->Open();
        if(gOracleQuery->Next())
        {
        }
        snprintf(sNotifySeqName,MAX_NAME_LEN,"%s_MDB_CHANGE_NOTIFY_SEQ",sDsnName);
    }
    catch(TMDBDBExcpInterface& e)
    {
        snprintf(sNotifySeqName,MAX_NAME_LEN,"MDB_CHANGE_NOTIFY_SEQ");
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Init failed : Unknown error!\n");
        TADD_FUNC("Finish.");
        return -1;
    }
    
    TADD_DETAIL("sChangeNotifySeqName=[%s], shadow table name=[%s]\n",sNotifySeqName,sShadowName);
    return 0;
}

int CleanShadowData()
{
    int iRet = 0;
    TADD_NORMAL("Start.");
    try
    {
        char sTableName[64] = {0};

        char sUpperDsn[64] = {0};
        SAFESTRCPY(sUpperDsn, sizeof(sUpperDsn), TMdbNtcStrFunc::ToUpper(gDsn->sName));
        
        char sMdbChangeNotifySeqName[MAX_NAME_LEN]= {0};
        char sShadowName[MAX_NAME_LEN]= {0};		
        CHECK_RET(GetShadowTableName(sMdbChangeNotifySeqName,sShadowName,sUpperDsn),"GetTableName Faild");

        char sSQL[512]= {0};
        sprintf(sSQL,"select table_name,min(table_sequence) min from %s group by table_name ",sMdbChangeNotifySeqName);

        char sDeleteSQL[512]= {0};
        
        #ifdef DB_ORACLE
            sprintf(sDeleteSQL,"delete from %s where lower(table_name) = lower(:table_name) and table_sequence <= :table_sequence and update_time < (sysdate - :delay_time/(24*3600)) ",sShadowName);
        #elif DB_MYSQL
            sprintf(sDeleteSQL,"delete from %s where lower(table_name) = lower(:table_name) and table_sequence <= :table_sequence and update_time < (sysdate - interval :delay_time second) ",sShadowName);
        #else        
            TADD_ERROR(ERROR_UNKNOWN, "[%s:%d]  DB_TYPE is wrong, check Makefile",__FILE__,__LINE__); 
            return -1;
        #endif 
        
        char sQryCntSQL[512]= {0};
        sprintf(sQryCntSQL, "select count(1) counts from %s ", sShadowName);

        int iCount =0;
        int iDealyTime  = gConfig->GetDSN()->m_iOraRepDelaySec + gConfig->GetDSN()->m_iOraRepInterval;
        
        std::map<string, MDB_INT64> mTabDelSeqMap; // 记录表每次刷新的table_sequence
        mTabDelSeqMap.clear();
        
        while(1)
        {
            if(NULL == gOracleQuery)
            {
                TADD_ERROR(ERROR_UNKNOWN,"mdbClean::CleanShadowData() : Oracle  handler is not initialized");
                return -1;
            }
            
            if(gtProcCtrl.IsCurProcStop())
            {
                TADD_NORMAL("Stop.");
                break;
            }
            gtProcCtrl.UpdateProcHeart(0);

            gQryCnt->Close();
			gQryCnt->Commit();
			gQryCnt->Close();
            gQryCnt->SetSQL(sQryCntSQL);
            gQryCnt->Open();
            gQryCnt->Next();
            iCount = gQryCnt->Field("counts").AsInteger();
            if(iCount >= MAX_CHANGE_NOTIF_CNT)
            {
                TADD_WARNING("%s record count[%d] is more than %d, pls check!", sShadowName, iCount, MAX_CHANGE_NOTIF_CNT);
            }
            gOracleQuery->Close();
			gOracleQuery->Commit();
            gOracleQuery->Close();
            gOracleQuery->SetSQL(sSQL);
            gOracleQuery->Open();
            memset(sTableName,0,sizeof(sTableName));
            long long iMin = 0;

            while(gOracleQuery->Next())
            {
                SAFESTRCPY(sTableName,sizeof(sTableName),gOracleQuery->Field("table_name").AsString());
                iMin = gOracleQuery->Field("min").AsInteger();

                // 如果上一次该表的min(table_sequence)与此次的一样，避免执行无意义的删除
                if(true == CheckRepeatDel(sTableName, iMin, mTabDelSeqMap))
                {
                    continue;
                }
                
                // clean data
                gOracleClean->Close();
                gOracleClean->SetSQL(sDeleteSQL);
                gOracleClean->SetParameter("table_name",sTableName);
                gOracleClean->SetParameter("table_sequence",iMin);
                gOracleClean->SetParameter("delay_time", iDealyTime);
                gOracleClean->Execute();
                gOracleClean->Commit();

                MDB_INT64 iRwAff = gOracleClean->RowsAffected();
                if(iRwAff > 0)
                {
                    SaveLastDelSeq(sTableName, iMin,  iRwAff,mTabDelSeqMap);
                    TADD_NORMAL("Clean :Table[%s], min(table_sequence):[%lld], delay_time=[%d], RowsAffected=%d.", sTableName,iMin, iDealyTime,gOracleClean->RowsAffected());
                }
                else
                {
                    TADD_DETAIL("Clean :Table[%s]:no row deleted. may wait for delay time.( min(table_sequence):[%lld], delay_time=[%d]).", sTableName,iMin, iDealyTime);
                }
                
            }
            TADD_NORMAL(" Clean time(s) : %d ", gConfig->GetDSN()->iCleanTime * 60);
            gtProcCtrl.UpdateProcHeart(gConfig->GetDSN()->iCleanTime * 60);
        }//end while
    }
    catch(TMDBDBExcpInterface& e)
    {
        CHECK_RET(-1,"Init failed : ERROR_SQL=%s.ERROR_MSG=%s", e.GetErrSql(), e.GetErrMsg());
    }
    catch(...)
    {
        CHECK_RET(-1,"Init failed : Unknown error!");
    }
    return iRet;
}

int   main(int   argc,   char*   argv[])
{
    int iRet = 0;
//    Process::SetProcessSignal();
    if(argc != 2)
    {
        printf("Invalid parameters.\n Exit!\n\n");
        return 0;
    }
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
    char sName[64];
    memset(sName, 0, sizeof(sName));
    //sprintf(sName, "%s %s", argv[0], argv[1]);
    snprintf(sName,sizeof(sName), "%s %s", argv[0], argv[1]);
    TADD_START(argv[1],sName, 0, false, true);

    //构造配置对象
    if(gConfig == NULL)
    {
        gConfig = new(std::nothrow) TMdbConfig();
        if(gConfig == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
    }
    //检测配置文件
    CHECK_RET(gConfig->Init(),"Connect to mdb failed. Can't find Config-file.");
    CHECK_RET(gConfig->LoadCfg(argv[1]),"Connect to mdb failed. Can't find Config-file.");
    gShmDSN = TMdbShmMgr::GetShmDSN(argv[1]);
    CHECK_OBJ(gShmDSN);
    gDsn = gShmDSN->GetInfo();
    CHECK_OBJ(gDsn);
    CHECK_RET(gtProcCtrl.Init(argv[1]),"gtProcCtrl.Init failed");
    //connect oracle
    ConnectOracle();

    if(true == gConfig->GetDSN()->m_bShadow)
    {
        //clean data
        while(CleanShadowData() != 0)
        {
        	int iWaitSec = 30;
			TADD_NORMAL("CleanShadowData failed ....wait(%d) ",iWaitSec);
			TMdbDateTime::Sleep(iWaitSec);
            ReConnectDB();//重新链接数据库
        }
    }
    else
    {
        //clean data
        while(CleanData() != 0)
        {
        	int iWaitSec = 30;
			TADD_NORMAL("CleanData failed ....wait(%d) ",iWaitSec);
			TMdbDateTime::Sleep(iWaitSec);
            ReConnectDB();//重新链接数据库
        }
    }

    ClearOracleLink();//清理数据库链接
    SAFE_DELETE(gConfig);
    //TADD_END();
    return iRet;
}



