#include "Common/mdbSysTimerSoftClock.h"
#include "Common/mdbSysTimerTaskExecute.h"
#include "Common/mdbSysTimerManager.h"
#include "Common/mdbDateUtils.h"
//#include "Common/mdbLogInterface.h"
#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include <sys/stat.h>
using namespace std;
//namespace  QuickMDB
//{
        
        unsigned int TMdbNtcTimerManager::TimerId = 0;
        bool TMdbNtcTimerManager::bHasInitialize = false;
        extern TMdbNtcTimerQuery *gpMdbNtcTimerItems[MDB_NTC_TIMER_QUERY_SIZE];
        extern TMdbNtcThreadStateInfo gTMdbNtcThreadStateInfo;

        bool TMdbNtcTimerManager::Initialize( bool bDynamic )
        {
            bool bRet = true;
            if( !bHasInitialize )
            {
                bHasInitialize = true;
                bRet = TMdbNtcSoftClockThread::GetInstancePtr( false )->Run( );
                if( bRet )
                {
                    bRet = TMdbNtcResourceManagerThread::GetInstancePtr( false, bDynamic )->Run( );
                }
            }
            return bRet;
        }

        unsigned int TMdbNtcTimerManager::AddRelativeTimer(const char *ATimerName,bool ALoopFlag,unsigned int ACount100ms,OnMdbEventFunc AEventFunc,void *pFuncParam)
        {
            if(ACount100ms == 0 || ACount100ms >= (24*60*60*10))
            {
                TADD_WARNING("TMdbNtcTimerManager::AddRelativeTimer() : Input %d is error. Value in {1s..24H}",ACount100ms);
                return 0;
            }

            //是否1秒-999秒
            TMdbNtcTimerQuery *pCurrTimerQuery = NULL;
            TMdbNtcTimerInfo *pTimerInfo = new TMdbNtcTimerInfo();
            if(ACount100ms>=10 && ACount100ms < 10 * 1000)
            {
                unsigned short int Count100s   = (unsigned short int)(ACount100ms / 1000);
                unsigned short int Count10s    = (unsigned short int)((ACount100ms % 1000) / 100);
                unsigned short int Count1s     = (unsigned short int)((ACount100ms % 100) /10);

                strncpy(pTimerInfo->Name,ATimerName,sizeof(pTimerInfo->Name)-1);
                pTimerInfo->Name[sizeof(pTimerInfo->Name)-1]='\0';
                pTimerInfo->TimerType = MDB_NTC_TIMER_TYPE_RELATIVE;
                pTimerInfo->LoopFlag = ALoopFlag;
                pTimerInfo->Count1ms = ACount100ms * 100;
                pTimerInfo->OnTimerEvent = AEventFunc;
                pTimerInfo->Param = pFuncParam;

                if(Count1s!=0)
                {
                    pCurrTimerQuery = gpMdbNtcTimerItems[Count1s];

                    pTimerInfo->Query10s = Count10s!=0 ? gpMdbNtcTimerItems[Count10s  + 10] : NULL;
                    pTimerInfo->Query100s = Count100s!=0 ? gpMdbNtcTimerItems[Count100s  + 20] : NULL;
                }
                else if(Count10s!=0)
                {
                    pCurrTimerQuery = gpMdbNtcTimerItems[Count10s  + 10];
                    pTimerInfo->Query100s = Count100s!=0 ? gpMdbNtcTimerItems[Count100s  + 20] : NULL;
                }
                else if(Count100s!=0)
                {
                    pCurrTimerQuery =  gpMdbNtcTimerItems[Count100s  + 20];
                }

                if(pCurrTimerQuery!=NULL)
                {
                    pTimerInfo->m_uiTimerId = ++TMdbNtcTimerManager::TimerId;
                    pTimerInfo->Query1s = pCurrTimerQuery;
                    pCurrTimerQuery->AddTimerInfo(pTimerInfo,true);
                }
                else
                {
                    delete pTimerInfo;
                    pTimerInfo = NULL;
                }

            }
            else
            {

                strncpy(pTimerInfo->Name,ATimerName,sizeof(pTimerInfo->Name)-1);
                pTimerInfo->Name[sizeof(pTimerInfo->Name)-1]='\0';
                pTimerInfo->TimerType = MDB_NTC_TIMER_TYPE_RELATIVE;
                pTimerInfo->LoopFlag = ALoopFlag;
                pTimerInfo->Count1ms = ACount100ms * 100;
                pTimerInfo->OnTimerEvent = AEventFunc;
                pTimerInfo->Param = pFuncParam;

                pCurrTimerQuery = gpMdbNtcTimerItems[0];
                if(pCurrTimerQuery!=NULL)
                {
                    pTimerInfo->m_uiTimerId = ++TMdbNtcTimerManager::TimerId;
                    pTimerInfo->Query1s = pCurrTimerQuery;
                    pCurrTimerQuery->AddTimerInfoEx(pTimerInfo,true);
                }
                else
                {
                    delete pTimerInfo;
                    pTimerInfo = NULL;
                }
            }
            if(pCurrTimerQuery!=NULL)
                return pTimerInfo->m_uiTimerId;
            else
                return 0;
        }

        unsigned int TMdbNtcTimerManager::AddAbsTimer(const char *ATimerName,const char *ATimerStr,OnMdbEventFunc AEventFunc,void *pFuncParam)
        {
            TMdbNtcClockInfo ClockInfo;
            ClockInfo.SetNowClock();
            MDB_UINT64 iNowSecond = ClockInfo.GetSecond();

            if(strlen(ATimerStr)==MDB_NTC_ZS_MAX_DATE_LONG_SIZE-1)
            {
                ClockInfo.SetClockByLongDateTimeStr(ATimerStr);
            }
            else if(strlen(ATimerStr)==MDB_NTC_ZS_MAX_DATE_SHORT_SIZE-1)
            {
                ClockInfo.SetClockByDateTimeStr(ATimerStr);
            }
            else
            {
                TADD_WARNING("TMdbNtcTimerManager::AddAbsTimer(Name=%s,Date=%s) Input Date format error.",ATimerName,ATimerStr);
                return 0;
            }

            MDB_INT64 iTimerCount1s = (MDB_INT64)(ClockInfo.GetSecond() -iNowSecond);

            TMdbNtcTimerQuery *pCurrTimerQuery = NULL;
            TMdbNtcTimerInfo *pTimerInfo = NULL;
            if(iTimerCount1s>0)
            {
                pTimerInfo = new TMdbNtcTimerInfo();

                strncpy(pTimerInfo->Name,ATimerName,sizeof(pTimerInfo->Name)-1);
                pTimerInfo->Name[sizeof(pTimerInfo->Name)-1]='\0';
                pTimerInfo->TimerType = MDB_NTC_TIMER_TYPE_ABSOLUTE;
                pTimerInfo->Count1ms = iTimerCount1s * 1000;
                pTimerInfo->OnTimerEvent = AEventFunc;
                pTimerInfo->Param = pFuncParam;

                pCurrTimerQuery = gpMdbNtcTimerItems[30];
                if(pCurrTimerQuery!=NULL)
                {
                    pTimerInfo->m_uiTimerId = ++TMdbNtcTimerManager::TimerId;
                    pTimerInfo->Query1s = pCurrTimerQuery;
                    pCurrTimerQuery->AddTimerInfoEx(pTimerInfo,true);
                }
                else
                {
                    delete pTimerInfo;
                    pTimerInfo = NULL;
                }
            }
            if(pCurrTimerQuery!=NULL)
                return pTimerInfo->m_uiTimerId;
            else
                return 0;
        }

        bool TMdbNtcTimerManager::DelTimerByName(const char *ATimerName,void **ppFuncParam)
        {
            bool bRet = false;
            TMdbNtcTimerQuery *pCurrTimerQuery = NULL;
            for( int i = 0; i < MDB_NTC_TIMER_QUERY_SIZE; i++ )
            {
                pCurrTimerQuery = gpMdbNtcTimerItems[i];
                if( NULL != pCurrTimerQuery )
                {
                    bRet = pCurrTimerQuery->DelTimerInfo( MDB_NTC_DEL_TIMER_BY_NAME, ATimerName, ppFuncParam );
                    if( bRet )
                        break;
                }
            }
            return bRet;
        }

        bool TMdbNtcTimerManager::DelTimerById(const unsigned int uiTimerId,void **ppFuncParam)
        {
            bool bRet = false;
            TMdbNtcStringBuffer sTimerId;
            sTimerId <<uiTimerId;  
        
            TMdbNtcTimerQuery *pCurrTimerQuery = NULL;
            for( int i = 0; i < MDB_NTC_TIMER_QUERY_SIZE; i++ )
            {
                pCurrTimerQuery = gpMdbNtcTimerItems[i];
                if( NULL != pCurrTimerQuery )
                {
                    bRet = pCurrTimerQuery->DelTimerInfo( MDB_NTC_DEL_TIMER_BY_ID,sTimerId.c_str(), ppFuncParam );
                    if( bRet )
                        break;
                }
            }
            return bRet;
        }
        
        void TMdbNtcTimerManager::KillAllThr()
        {
            //结束软时钟线程
            TMdbNtcSoftClockThread* pTSoftClockThread=TMdbNtcSoftClockThread::GetInstancePtr(true); //得到软时钟线程对像
            pTSoftClockThread->Cancel();
             
             //结束工作线程池管理线程
            TMdbNtcResourceManagerThread *pTResourceManagerThread = TMdbNtcResourceManagerThread::GetInstancePtr(true,true);
            pTResourceManagerThread->Cancel();
             
            //结束所有的工作线程
            gTMdbNtcThreadStateInfo.m_thread_lock.Lock();
            for(unsigned int j=0;j<gTMdbNtcThreadStateInfo.m_TAutoArray.GetSize();j++)
            {
                TMdbNtcWorkThread* pTWorkThread = ((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(j)))->m_pTWorkThread;
                pTWorkThread->Cancel();
            }
            gTMdbNtcThreadStateInfo.m_thread_lock.Unlock(); 
        }

//}

