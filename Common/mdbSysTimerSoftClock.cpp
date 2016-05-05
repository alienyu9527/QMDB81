#include "Common/mdbSysTimerSoftClock.h"
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

    TMdbNtcTimerQuery *gpMdbNtcTimerItems[MDB_NTC_TIMER_QUERY_SIZE] = { NULL };
    TMdbNtcThreadLock gMdbNtcTimerTThreadLock;
    TMdbNtcQueue gTMdbNtcBlockingQueue; //��Ϣ����,ѹ��ʱ������������ʱ����
    TMdbNtcSoftClockThread* TMdbNtcSoftClockThread::_instance = NULL;

    //ʱ����Ϣ��
    //==========================================================================
    void TMdbNtcClockInfo::SetNowClock(void)
	{
	#ifdef OS_WINDOWS
        SYSTEMTIME CMOSClock;

        /* get real clock from CMOS */
        GetLocalTime(&CMOSClock);
        if (CMOSClock.wYear <= 1997 )
        {
            CMOSClock.wYear += 100;
        }
        Count10ms = (unsigned char)(CMOSClock.wMilliseconds / 10);
        Year      = (unsigned short int)CMOSClock.wYear;
        Month     = (unsigned char)CMOSClock.wMonth;
        Day       = (unsigned char)CMOSClock.wDay;
        Hour      = (unsigned char)CMOSClock.wHour;
        Minute    = (unsigned char)CMOSClock.wMinute;
        Second    = (unsigned char)CMOSClock.wSecond;
	#else
        time_t dt;
        struct tm tm1;
        struct timeval  tp;

        /* get real clock from CMOS */
        gettimeofday(&tp, NULL);
        dt  = tp.tv_sec;
        localtime_r(&dt,&tm1);
        Count10ms = (unsigned char)(tp.tv_usec / 10000);
        Year      = (unsigned short int)(tm1.tm_year + 1900);
        Month     = (unsigned char)(tm1.tm_mon + 1);
        Day       = (unsigned char)tm1.tm_mday;
        Hour      = (unsigned char)tm1.tm_hour;
        Minute    = (unsigned char)tm1.tm_min;
        Second    = (unsigned char)tm1.tm_sec;
	#endif
	}

    //��������ڸ�ʽ��ת��ΪTClockInfo�ṹ(2010
    void TMdbNtcClockInfo::SetClockByDateTimeStr(const char *AClockStr)
    {
        if(strlen(AClockStr)<14)
        {
            TADD_WARNING("TMdbNtcClockInfo::SetClockByDateTimeStr(%s) : Input DateTime format error!",AClockStr);
            return;
        }

        char ClockTmp[MDB_NTC_ZS_MAX_DATE_SHORT_SIZE] = {0};
        for(int i=0;i<MDB_NTC_ZS_MAX_DATE_SHORT_SIZE;i++)
        {
            ClockTmp[i]=(char)(AClockStr[i]  - '0');
        }

        Second    = (unsigned char)(ClockTmp[12]*10+ClockTmp[13]);
        Minute    = (unsigned char)(ClockTmp[10]*10+ClockTmp[11]);
        Hour      = (unsigned char)(ClockTmp[8]*10+ClockTmp[9]);

        Day       = (unsigned char)(ClockTmp[6]*10+ClockTmp[7]);
        Month     = (unsigned char)(ClockTmp[4]*10+ClockTmp[5]);
        Year      = (short unsigned int)(ClockTmp[0]*1000+ClockTmp[1]*100+ClockTmp[2]*10+ClockTmp[3]);
        Count10ms = 0;
    }

    //���볤���ڸ�ʽ��ת��ΪTClockInfo�ṹ
    void TMdbNtcClockInfo::SetClockByLongDateTimeStr(const char *AClockStr)
    {
        if(strlen(AClockStr)<19)
        {
            TADD_WARNING("TMdbNtcClockInfo::SetClockByLongDateTimeStr(%s) : Input DateTime format error!",AClockStr);
            return;
        }

        char ClockTmp[MDB_NTC_ZS_MAX_DATE_LONG_SIZE] = {0};
        for(int i=0;i<MDB_NTC_ZS_MAX_DATE_LONG_SIZE;i++)
        {
            ClockTmp[i]=(char)(AClockStr[i]  - '0');
        }

        Second    = (unsigned char)(ClockTmp[17]*10+ClockTmp[18]);
        Minute    = (unsigned char)(ClockTmp[14]*10+ClockTmp[15]);
        Hour      = (unsigned char)(ClockTmp[11]*10+ClockTmp[12]);

        Day       = (unsigned char)(ClockTmp[8]*10+ClockTmp[9]);
        Month     = (unsigned char)(ClockTmp[5]*10+ClockTmp[6]);
        Year      = (short unsigned int)(ClockTmp[0]*1000+ClockTmp[1]*100+ClockTmp[2]*10+ClockTmp[3]);
        Count10ms = 0;
    }

    MDB_UINT64  TMdbNtcClockInfo::GetSecond(void)
    {
        unsigned short int  DaysSince1994;
        MDB_UINT64 SecondsSince1994;
        unsigned char DeltaDays[12] = {0, 3, 3, 6, 8, 11, 13, 16, 19, 21, 24, 26};

        DaysSince1994  = (short unsigned int)(Day - 1);
        DaysSince1994  = (short unsigned int)(DaysSince1994 + (Month - 1)* 28);
        DaysSince1994  = (short unsigned int)(DaysSince1994 + DeltaDays[Month - 1]);
        if (Month > 2 && (Year % 4) == 0)
            DaysSince1994 ++;

        DaysSince1994 = (short unsigned int)(DaysSince1994 +(Year - 1994) * 365);
        DaysSince1994 = (short unsigned int)(DaysSince1994 +(Year - 1993) / 4);

        SecondsSince1994 = (MDB_UINT64)(DaysSince1994 * 86400 + Hour * 3600 + Minute * 60 + Second);

        return SecondsSince1994;
    }

    //ʱ����Ϣ��
    //==========================================================================
    TMdbNtcTimerInfo::TMdbNtcTimerInfo()
    {
        TimerType = MDB_NTC_TIMER_TYPE_RELATIVE;
        LoopFlag = false;
        OnTimerEvent = NULL;
        memset(Name,0,sizeof(Name));
        Reset = 0;
        Count1ms = 0;
        Query1s = NULL;
        Query10s = NULL;
        Query100s = NULL;
        Next = NULL;
        m_uiTimerId = 0;
    }

    TMdbNtcTimerInfo::~TMdbNtcTimerInfo()
    {
        TimerType = MDB_NTC_TIMER_TYPE_RELATIVE;
        LoopFlag = false;
        OnTimerEvent = NULL;
        memset(Name,0,sizeof(Name));
        Reset = 0;
        Count1ms = 0;
        Query1s = NULL;
        Query10s = NULL;
        Query100s = NULL;
        Next = NULL;
        m_uiTimerId = 0;
    }

    TMdbNtcStringBuffer TMdbNtcTimerInfo::ToString(void)
    {
        char sLineTemp[1024] = {0};
        snprintf(sLineTemp,sizeof(sLineTemp),"Now=%s,Count1s=%3d,Reset=%4d,10s=%3d,100s=%3d,Name=%s,TTimerTypes=%s,LoopFlag=%s",
            TMdbNtcLongDateTime::Format().c_str(),(int)(Count1ms/1000),(int)(Reset/1000),(int)GetQuery10s(),(int)GetQuery100s(),Name,0==TimerType?"ABSOLUTE":"RELATIVE",LoopFlag?"TRUE":"FALSE");
        return sLineTemp;
    }

    unsigned short int TMdbNtcTimerInfo::GetQuery10s(void)
    {
        return Query10s!=NULL?  Query10s->GetIndex():(unsigned short int)0xFF;
    }

    unsigned short int TMdbNtcTimerInfo::GetQuery100s(void)
    {
        return Query100s!=NULL?Query100s->GetIndex():(unsigned short int)0xFF;
    }

    //ʱ���ж���
    //==========================================================================
    TMdbNtcTimerQuery::TMdbNtcTimerQuery(unsigned int ATimerIndex)
    {
        FCount1ms = 0;

        if(ATimerIndex>0 && ATimerIndex<10)
        {
            FCount1ms = ATimerIndex  * 1000;
        }
        else if(ATimerIndex>10 && ATimerIndex<20)
        {
            FCount1ms = (ATimerIndex-10)  * 1000 * 10 ;
        }
        else if(ATimerIndex>20 && ATimerIndex<30)
        {
            FCount1ms = (ATimerIndex-20)  * 1000 * 100;
        }
        FIndex = (short unsigned int)ATimerIndex;
        FCount = 0;
        FReset = FCount1ms;//���[1��ATimerIndex=1,FCount1ms=1000],[10��ATimerIndex=11,FCount1ms=10000],[100��ATimerIndex=21,FCount1ms=100000]
        FTimerHead = NULL;
    }

    //�õ�ʱ���ж����ʱ������
    unsigned int TMdbNtcTimerQuery::GetCount(void)
    {
        return FCount;
    }

    unsigned short int TMdbNtcTimerQuery::GetIndex(void)
    {
        return FIndex;
    }

    //ÿ�μ�100����
    bool TMdbNtcTimerQuery::SubCount100ms( )
    {
        gMdbNtcTimerTThreadLock.Lock();
        if(FCount>0)//��ʱ�Ӷ����д���δ���ڵĶ�ʱ��
        {
            if(FReset > 100)
            {
                FReset -= 100;
            }
            else
            if( FReset == 100)
            {
                FReset = 0;
            }
            else
            {
                FReset = FCount1ms;
            }

            if( NULL != FTimerHead )
            {
                if(FTimerHead->Reset > 100)//ͷ�ڵ��ִ��ʱ�仹δ����
                {
                    FTimerHead->Reset -= 100;
                }
                else//ͷ�ڵ��ִ��ʱ���Ѿ������Ҫ��ʼ���������ִ��
                {
                    FTimerHead->Reset = 0;

                    TMdbNtcTimerQuery *pNextQuery = NULL;
                    TMdbNtcTimerInfo *pItem = FTimerHead;

                    while(pItem!=NULL && pItem->Reset==0)//����ͷ�ڵ�ͽ�����ͷ�ڵ�������Reset=0�Ľڵ㶼����ִ�У��������Reset!=0�Ľڵ����ֹѭ��
                    {
                        FTimerHead = pItem->Next;
                        pItem->Next = NULL;
                        FCount--;
                        if(FIndex < 10 && pItem->Query10s!=NULL)
                        {
                            pNextQuery = pItem->Query10s;
                        }
                        else if(FIndex < 20 &&  pItem->Query100s!=NULL)
                        {
                            pNextQuery = pItem->Query100s;
                        }

                        //FReset += pItem->Reset;
                        if(pNextQuery==NULL)
                        {
                            //�����ѭ����ʱ����ʱ�䵽ʱ����Ҫ���¼Ӷ�����
                            if(pItem->LoopFlag)
                            {
                                if(FIndex==0)
                                {
                                    pItem->Query1s->AddTimerInfoEx(pItem,false);
                                }
                                else
                                {
                                    pItem->Query1s->AddTimerInfo(pItem,false);
                                }
                            }

                            //printf("==>Execute task %s\n",pItem->ToString().c_str());
                            if(pItem->OnTimerEvent!=NULL)
                            {
                                //pItem->OnTimerEvent(pItem->Param);
                                //������Ķ�ʱ����ѹ�����
                                //printf("Push TMdbNtcTimerInfo=%p\n",pItem);
                                gTMdbNtcBlockingQueue.Push( pItem );
                                //cout<<" Timer is going to  start !"<<endl;
                            }

                            /*if( ! ( pItem->LoopFlag ) )
                            {
                                delete pItem;
                                pItem = NULL;
                            }
                            */
                        }
                        else
                        {
                            pNextQuery->AddTimerInfo(pItem,false);
                        }

                        pItem = FTimerHead;
                    }
                    if( NULL != pItem )
                    {
                        FReset = pItem->Reset;
                    }
                    else
                    {
                        FReset = FCount1ms;
                    }
                }
            }
            else
            {
                TADD_WARNING("TMdbNtcTimerQuery::SubCount100ms() : is abnormal! FCount is %d but FTimerHead is NULL.",FCount);
            }

            //cout<<"SubCount100ms( ) FReset="<<FReset<<endl;//9��FReset=9000

        }
        gMdbNtcTimerTThreadLock.Unlock();
        return FReset>0;
    }

    //���Ӷ�ʱ����ָ��ʱ�Ӷ�����(1�����Զ�ʱ����2��<1��>=1000����Զ�ʱ��)
    void TMdbNtcTimerQuery::AddTimerInfoEx(TMdbNtcTimerInfo *ATimerInfo, bool bLockFlag)
    {
        if( bLockFlag )
            gMdbNtcTimerTThreadLock.Lock();
        //printf("TMdbNtcTimerQuery::AddTimerInfoEx(Index=%d,[%s])\n",FIndex,ATimerInfo->ToString().c_str());
        if (FCount==0)
        {
            ATimerInfo->Reset = ATimerInfo->Count1ms;
            FTimerHead = ATimerInfo;
        }
        else
        {
              //if(FTimerHead==NULL)
              //{
              //    MDB_NTC_ZF_THROW_TEXCEPTION("TimerQuery is data error.");
              //}

            TMdbNtcTimerInfo *pItem = FTimerHead;
            TMdbNtcTimerInfo *pPrevItem = NULL;

            MDB_INT64 iResetCount = 0;
            while(true)
            {
                if( (iResetCount + pItem->Reset) >= ATimerInfo->Count1ms)//����Ϊ���һ���ڵ����
                {
                    if(pPrevItem==NULL)//��Ϊ�׽ڵ����
                    {
                        //100,50<== 80
                        //80,20,50
                        pItem->Reset -= ATimerInfo->Count1ms;
                        ATimerInfo->Reset = ATimerInfo->Count1ms;
                        FTimerHead = ATimerInfo;
                        ATimerInfo->Next = pItem;
                    }
                    else//��Ϊ�м�ڵ����
                    {
                        //100,30 <== 120
                        //100,20,10
                        ATimerInfo->Reset = ATimerInfo->Count1ms - iResetCount;
                        pItem->Reset -= ATimerInfo->Reset;
                        pPrevItem->Next = ATimerInfo;
                        ATimerInfo->Next = pItem;
                    }
                    break;
                }

                if(pItem->Next == NULL)//��Ϊ���һ���ڵ����
                {
                    //100,30 <== 180
                    //100,30,50
                    ATimerInfo->Reset = (ATimerInfo->Count1ms - iResetCount - pItem->Reset);
                    pItem->Next = ATimerInfo;
                    break;
                }

                iResetCount += pItem->Reset;
                pPrevItem = pItem;
                pItem = pItem->Next;
            }
        }
        FCount++;
        if( bLockFlag )
            gMdbNtcTimerTThreadLock.Unlock();
    }

    //����>=1�벢��<1000��Ķ�ʱ��
    void TMdbNtcTimerQuery::AddTimerInfo(TMdbNtcTimerInfo *ATimerInfo, bool bLockFlag)
    {
        if( bLockFlag )
            gMdbNtcTimerTThreadLock.Lock();
        //printf("TMdbNtcTimerQuery::AddTimerInfo(Index=%d,[%s])\n",FIndex,ATimerInfo->ToString().c_str());
        ATimerInfo->Reset = FCount1ms;

        if(FTimerHead!=NULL)
        {
            TMdbNtcTimerInfo *pItem = FTimerHead;
            while(true)
            {
                ATimerInfo->Reset -= pItem->Reset;

                if(pItem->Next==NULL)
                    break;
                pItem = pItem->Next;
            }

            pItem->Next = ATimerInfo;
        }
        else
        {
            FTimerHead = ATimerInfo;
        }
        FCount++;
        if( bLockFlag )
            gMdbNtcTimerTThreadLock.Unlock();
    }

    //���������Ϣ
    TMdbNtcStringBuffer TMdbNtcTimerQuery::ToString(void)
    {
        TMdbNtcStringBuffer sLineText;
        char sLineTemp[1024]={0};
        snprintf(sLineTemp,sizeof(sLineTemp),"Count1s=%3d,Reset=%3d,Count=%d",(int)(FCount1ms/1000),(int)(FReset/1000),FCount);
        sLineText << sLineTemp;

        TMdbNtcTimerInfo *pItem=FTimerHead;
        int i=0;
        while(pItem!=NULL)
        {
            sLineText << "\n  [" << pItem->ToString() << "]";
            pItem = pItem->Next;
            i++;
            if(i>100) break;
        }

        return sLineText;
    }

    bool TMdbNtcTimerQuery::DelTimerInfo(TMdbNtcDelTimerTypes DelTimerTypes, const char *ATimerFlag,void **ppFuncParam)
    {
        bool bRet = false;
        bool bFlag = false;
        gMdbNtcTimerTThreadLock.Lock();
        TMdbNtcTimerInfo *pCurrItem = FTimerHead;
        TMdbNtcTimerInfo *pPreItem = FTimerHead;
        if( ( NULL != FTimerHead ) && ( FCount > 0 ) )//��ʱ����ڵ�����ǿ�
        {
            if( MDB_NTC_DEL_TIMER_BY_NAME == DelTimerTypes )
                bFlag = ( 0 == strcmp( FTimerHead->Name, ATimerFlag ) );
            else
                bFlag = ( FTimerHead->m_uiTimerId == atol(ATimerFlag) );

            if( bFlag )//ɾ������ͷ�ڵ�
            {
                if( NULL != FTimerHead->Next )//ɾ�����������ڵ�
                {
                    FTimerHead->Next->Reset += FTimerHead->Reset;
                    FTimerHead = FTimerHead->Next;
                }
                else//ɾ����û�������ڵ�
                {
                    FTimerHead = NULL;
                }
                if( NULL != ppFuncParam )
                    *ppFuncParam = pCurrItem->Param;
                delete pCurrItem;
                FCount--;
                bRet = true;
            }
            else//ɾ���ķ�ͷ�ڵ�
            {
                pCurrItem = FTimerHead->Next;
                while( pCurrItem )
                {
                    if( MDB_NTC_DEL_TIMER_BY_NAME == DelTimerTypes )
                        bFlag = ( 0 == strcmp( pCurrItem->Name, ATimerFlag ) );
                    else
                        bFlag = ( pCurrItem->m_uiTimerId == atol(ATimerFlag) );
                    if( bFlag )
                    {
                        pPreItem->Next = pCurrItem->Next;
                        if( NULL != pCurrItem->Next )
                            pCurrItem->Next->Reset += pCurrItem->Reset;
                        if( NULL != ppFuncParam )
                            *ppFuncParam = pCurrItem->Param;
                        delete pCurrItem;
                        FCount--;
                        bRet = true;
                        break;
                    }
                    pPreItem = pCurrItem;
                    pCurrItem = pCurrItem->Next;
                }
            }
        }
        gMdbNtcTimerTThreadLock.Unlock();
        return bRet;
    }

    //ʱ��ά���߳���
    //==========================================================================

    TMdbNtcSoftClockThread::TMdbNtcSoftClockThread():TMdbNtcThread(10*1024*1024)
    {
        TADD_DETAIL("TMdbNtcSoftClockThread::TMdbNtcSoftClockThread()\n");
        for( int i = 0; i < MDB_NTC_TIMER_QUERY_SIZE; i++ )
        {
            if( NULL == gpMdbNtcTimerItems[i] )
                gpMdbNtcTimerItems[i] = new TMdbNtcTimerQuery((MDB_UINT32)i);
        }
        FStandardCount = 0;
    }

    TMdbNtcSoftClockThread::~TMdbNtcSoftClockThread()
    {
        TADD_DETAIL("TMdbNtcSoftClockThread::~TMdbNtcSoftClockThread()\n");
    }

    TMdbNtcSoftClockThread* TMdbNtcSoftClockThread::GetInstancePtr( bool bOnlyGetFlag )
    {
        if( bOnlyGetFlag )
        {
            return _instance;
        }
        else
        {
            if( NULL == _instance )
            {
                _instance = new TMdbNtcSoftClockThread();
            }
            return _instance;
        }
    }

    int TMdbNtcSoftClockThread::Execute(void)
    {
        ShowTimerInfo();
        MDB_INT64 iSleepTime = 80;
        MDB_INT64 iInterval=0;
        MDB_UINT64 iCount100ms=0;
        TMdbNtcClockInfo ClockInfo;

        //��ȡ����ʱ�ӵ���ʼֵ
        ClockInfo.SetNowClock();
        FCount1s = (MDB_INT64)ClockInfo.GetSecond();

        BeginCounter();
        while(TestCancel()==false)
        {
            TMdbNtcDateTime::Sleep((int)iSleepTime);
            FStandardCount++;
            iCount100ms++;

            //ÿ����-��ļ�������1
            if(iCount100ms%10 == 0)
            {
                FCount1s++;
                //printf("FCount1s=%d,FBeginTp=%d,FEndTp=%d,iInterval=%3d,iSleepTime=%2d,FStandardCount=%3d\n",
                //    FCount1s,FBeginTp,FEndTp,iInterval,iSleepTime,FStandardCount);
                //ShowTimerInfo();
            }

            //����ʱ�ӽṹ��Ϣ
            TimerProcess();

            iInterval = EndCounter();

            if (iInterval < 100 && iInterval >0)
            {
                TMdbNtcDateTime::Sleep((int)iInterval);
                if(iSleepTime < 80 && iInterval>10)
                    iSleepTime += 10;
            }
            else if(iInterval >= 100)
            {
                BeginCounter();
            }
            else if(iInterval < 0)
            {
                if(labs((long)iInterval) < 2000)
                {
                    iSleepTime -= 10;
                    if(iSleepTime <10) iSleepTime = 10;
                }
                else
                {
                    BeginCounter();
                }
            }//if

            if(FStandardCount >= 500000)
            {
                BeginCounter();
            }
        }//while
        return 0;
    }

    void TMdbNtcSoftClockThread::TimerProcess(void)
    {
        for(int i=0; i<MDB_NTC_TIMER_QUERY_SIZE; i++)
        {
            if ( NULL == gpMdbNtcTimerItems[i] )
                continue;

            gpMdbNtcTimerItems[i]->SubCount100ms( );
        }
    }

    void TMdbNtcSoftClockThread::ShowTimerInfo(void)
    {
        TADD_DETAIL("================================================\n");
        for(int i=0; i<MDB_NTC_TIMER_QUERY_SIZE; i++)
        {
            if( NULL != gpMdbNtcTimerItems[i] && gpMdbNtcTimerItems[i]->GetCount() > 0 )
            {
                printf("[TMdbNtcTimerQuery%02d] ",i);
                cout <<  gpMdbNtcTimerItems[i]->ToString().c_str() << endl;
            }
        }
        printf("\n");
    }

    void TMdbNtcSoftClockThread::BeginCounter(void)
    {
/*
#ifdef OS_WINDOWS
        FBeginTp = GetTickCount();
#else
        gettimeofday(&FBeginTp,NULL);
#endif
*/
        FBeginTp = TMdbNtcAccurateTime::GetCurTime();
        FStandardCount = 0;
    }

    MDB_INT64 TMdbNtcSoftClockThread::EndCounter(void)
    {
        MDB_INT64 iIntervalCount;
/*
#ifdef OS_WINDOWS
        FEndTp = GetTickCount();
        iIntervalCount = FBeginTp + FStandardCount*100 - FEndTp;
#else
        gettimeofday(&FEndTp,NULL);
        iIntervalCount = (FBeginTp.tv_sec*1000+FBeginTp.tv_usec/1000) + FStandardCount*100-(FEndTp.tv_sec*1000 + FEndTp.tv_usec/1000);
#endif
*/
        FEndTp = TMdbNtcAccurateTime::GetCurTime();
        iIntervalCount = FBeginTp - FEndTp + FStandardCount*100;

        return iIntervalCount;
    }

//}
