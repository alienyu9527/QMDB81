#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include <sys/stat.h>

#include "Common/mdbSysTimerSoftClock.h"
#include "Common/mdbSysTimerTaskExecute.h"
#include "Common/mdbDateUtils.h"

using namespace std;
//namespace  QuickMDB
//{

        TMdbNtcThreadStateInfo gTMdbNtcThreadStateInfo;
        TMdbNtcResourceManagerThread* TMdbNtcResourceManagerThread::_instance = NULL;
        extern TMdbNtcQueue gTMdbNtcBlockingQueue;

        TMdbNtcWorkThread::TMdbNtcWorkThread(bool bNeedLock):TMdbNtcThread(10*1024*1024)
        {
            m_bNeedLock = bNeedLock;
        }

        TMdbNtcWorkThread::~TMdbNtcWorkThread()
        {

        }

        int TMdbNtcWorkThread::Execute(void)
        {
            while(TestCancel()!=true)
            {
                //接受定时任务
                TMdbNtcTimerInfo* pTTimerInfo = (TMdbNtcTimerInfo*)gTMdbNtcBlockingQueue.Pop();
//printf("Pop TMdbNtcTimerInfo=%p\n",pTTimerInfo);

                if( pTTimerInfo != NULL ) //队列中有任务
                {
                    if( m_bNeedLock )
                    {
                        unsigned int j = 0;
                        gTMdbNtcThreadStateInfo.m_thread_lock.Lock();
                        for(j=0;j<gTMdbNtcThreadStateInfo.m_TAutoArray.GetSize();j++)
                        {
                            if( this == ((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(j)))->m_pTWorkThread )
                            {
                                ((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(j)))->m_thread_state = MDB_NTC_THREAD_RUN;
                                gTMdbNtcThreadStateInfo.m_uiRunThreadCount++;
                                //cout<<__LINE__<<" j = "<<j<<"  RunThrNum= "<<gTMdbNtcThreadStateInfo.m_uiRunThreadCount<<"  ";
                                break;
                            }
                        }
                        gTMdbNtcThreadStateInfo.m_thread_lock.Unlock();

                        ExecuteTask(pTTimerInfo);

                        gTMdbNtcThreadStateInfo.m_thread_lock.Lock();

                        for(j=0;j<gTMdbNtcThreadStateInfo.m_TAutoArray.GetSize();j++)
                        {
                            if( this == ((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(j)))->m_pTWorkThread )
                            {
                                ((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(j)))->m_thread_state = MDB_NTC_THREAD_INVALID;
                                gTMdbNtcThreadStateInfo.m_uiRunThreadCount--;
                                //cout<<__LINE__<<" j = "<<j<<"  RunThrNum= "<<gTMdbNtcThreadStateInfo.m_uiRunThreadCount<<"  ";
                                break;
                            }
                        }

//                        ((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(j)))->m_thread_state = MDB_NTC_THREAD_INVALID;
//                        gTMdbNtcThreadStateInfo.m_uiRunThreadCount--;
                        //cout<<__LINE__<<" j = "<<j<<"  RunThrNum= "<<gTMdbNtcThreadStateInfo.m_uiRunThreadCount<<endl;
                        gTMdbNtcThreadStateInfo.m_thread_lock.Unlock();
                        //TDateTime::Sleep(1000);
                    }
                    else
                    {
                        ExecuteTask(pTTimerInfo);
                    }
                }
            }
            return 0;
        }

        int TMdbNtcWorkThread::ExecuteTask(void *pTTimerInfo)
        {

//printf("Now=%s -- TMdbNtcWorkThread::ExecuteTask Begin--\n",TMdbNtcLongDateTime::Format().c_str());

            TMdbNtcTimerInfo* pTimerTask=(TMdbNtcTimerInfo*)pTTimerInfo;
            (*(pTimerTask->OnTimerEvent))(pTimerTask->Param);
            //cout<<" ThrID["<<GetThreadId()<<"]"<<"-- TMdbNtcWorkThread::ExecuteTask  Finish-- "<<endl;
            if( ! ( pTimerTask->LoopFlag ) )
            {
                delete pTimerTask;
                pTimerTask = NULL;
            }
            return 0;
        }

        TMdbNtcThreadStateDetailInfo::TMdbNtcThreadStateDetailInfo( TMdbNtcWorkThread *pTWorkThread_In )
        {
            m_pTWorkThread = pTWorkThread_In;
//            m_thread_id = thread_id_In;
//            m_thread_handle = thread_handle_In;
            m_thread_state = MDB_NTC_THREAD_INVALID;//初始状态
        }

        TMdbNtcStringBuffer TMdbNtcThreadStateDetailInfo::ToString(void)
        {
            char sLineTemp[1024]={0};
            snprintf(sLineTemp,sizeof(sLineTemp),"thread_point=%p,thread_id=%"MDB_NTC_ZS_FORMAT_INT64",thread_handle=%"MDB_NTC_ZS_FORMAT_INT64",thread_state=%d",
                m_pTWorkThread,m_pTWorkThread->GetThreadId(),static_cast<MDB_INT64>(m_pTWorkThread->GetThreadHandle()),m_thread_state);
            return sLineTemp;
        }

        TMdbNtcThreadStateInfo::TMdbNtcThreadStateInfo()
        {
            m_uiSumThreadCount = 0;
            m_uiRunThreadCount = 0;
        }

        TMdbNtcResourceManagerThread::TMdbNtcResourceManagerThread( bool bDynamic ):TMdbNtcThread(10*1024*1024)
        {
            m_bDynamic = bDynamic;
        }

        TMdbNtcResourceManagerThread::~TMdbNtcResourceManagerThread()
        {

        }

        TMdbNtcResourceManagerThread* TMdbNtcResourceManagerThread::GetInstancePtr( bool bOnlyGetFlag, bool bDynamic )
        {
            if( bOnlyGetFlag )
            {
                return _instance;
            }
            else
            {
                if( NULL == _instance )
                {
                    _instance = new TMdbNtcResourceManagerThread( bDynamic );
                }
                return _instance;
            }
        }

        int TMdbNtcResourceManagerThread::Execute(void)
        {

            gTMdbNtcThreadStateInfo.m_TAutoArray.SetAutoRelease(true);

            while(TestCancel()!=true)
            {
                if( !m_bDynamic )
                {
                    CreateWorkThread( );
                    TMdbNtcDateTime::Sleep(2000);
                    Cancel();
                }
                else
                {
//                    cout<<"Comming ---->"<<endl;
                    gTMdbNtcThreadStateInfo.m_thread_lock.Lock();
                    //输出空闲线程数量 Idle
                    //cout<<" ThrIdleNum= "<<gTMdbNtcThreadStateInfo.m_uiSumThreadCount - gTMdbNtcThreadStateInfo.m_uiRunThreadCount;
                    //cout<<" ThrSumNum = "<<gTMdbNtcThreadStateInfo.m_uiSumThreadCount;
                   // cout<<" ThrRunNum = "<<gTMdbNtcThreadStateInfo.m_uiRunThreadCount<<endl;

                    if(gTMdbNtcThreadStateInfo.m_uiSumThreadCount - gTMdbNtcThreadStateInfo.m_uiRunThreadCount <1
                    && gTMdbNtcThreadStateInfo.m_uiSumThreadCount < 10)//无空闲线程且工作线程总个数小于10的时候，创建工作线程
                    {
                        CreateWorkThread();
                    }
                    else
                    if(gTMdbNtcThreadStateInfo.m_uiSumThreadCount - gTMdbNtcThreadStateInfo.m_uiRunThreadCount > 1
                    )//空闲线程数个数大于1的时候，删除空闲线程
                    {
                       // cout<<" -------- DeleteIdleThread ----"<<endl;
                        DeleteIdleThread();
                    }
                    gTMdbNtcThreadStateInfo.m_thread_lock.Unlock();
                    TMdbNtcDateTime::Sleep(1000);
                }
            }
            return 0;
        }

        int TMdbNtcResourceManagerThread::CreateWorkThread(void)
        {
            TMdbNtcWorkThread *pTWorkThread = new TMdbNtcWorkThread(m_bDynamic);

            pTWorkThread->CleanupPush(new TMdbNtcTLSPointer<TMdbNtcThread>(pTWorkThread));

            pTWorkThread->Run();
            //cout<<__FUNCTION__<<"-THRHandle()= "<<pTWorkThread->GetThreadHandle()<<endl;
            gTMdbNtcThreadStateInfo.m_TAutoArray.Add(new TMdbNtcThreadStateDetailInfo(pTWorkThread));
//            cout<<((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(0)))->ToString().c_str()<<endl;
            gTMdbNtcThreadStateInfo.m_uiSumThreadCount++;
            return 0;
        }

        int TMdbNtcResourceManagerThread::DeleteIdleThread(void)
        {
            int iCycleNumber = (int)(gTMdbNtcThreadStateInfo.m_uiSumThreadCount - gTMdbNtcThreadStateInfo.m_uiRunThreadCount - 1);
            for(int i=0;i<iCycleNumber;i++)
            {
                for(unsigned int j=0;j<gTMdbNtcThreadStateInfo.m_TAutoArray.GetSize();j++)
                {
                    if(((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(j)))->m_thread_state != MDB_NTC_THREAD_RUN)
                    {

//printf("TMdbNtcResourceManagerThread::DeleteIdleThread() m_pTWorkThread=%p\n",((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(j)))->m_pTWorkThread);
                        ((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(j)))->m_pTWorkThread->Cancel();
                        gTMdbNtcThreadStateInfo.m_TAutoArray.Remove(j);
                        gTMdbNtcThreadStateInfo.m_uiSumThreadCount--;
                        break;
                    }
                }
            }
            return 0;
        }
/*
        void DisplayThreadStateInfo()
        {

            cout<<" m_TAutoArray Size = "<<gTMdbNtcThreadStateInfo.m_TAutoArray.GetSize()<<endl;
            cout<<" RunThrNum = "<<gTMdbNtcThreadStateInfo.m_uiRunThreadCount<<endl;
            cout<<" m_uiSumThreadCount = "<<gTMdbNtcThreadStateInfo.m_uiSumThreadCount<<endl;
            for(int j=0;j<gTMdbNtcThreadStateInfo.m_TAutoArray.GetSize();j++)
            {
                TMdbNtcThreadStateDetailInfo* pTThreadStateDetailInfo=((TMdbNtcThreadStateDetailInfo *)(gTMdbNtcThreadStateInfo.m_TAutoArray.GetAt(j)));
                TMdbNtcWorkThread* pTWorkThread=pTThreadStateDetailInfo->m_pTWorkThread;
                if( pTWorkThread->GetThreadState() == MDB_NTC_THREAD_INVALID )
                {
                    cout<<" Pos["<<j<<"]"<<" MDB_NTC_THREAD_INVALID"<<"  ThrHandle= "<<pTWorkThread->GetThreadHandle() <<endl;
                }
                else
                {
                    cout<<" Pos["<<j<<"]"<<" MDB_NTC_THREAD_RUN"<<"  ThrHandle= "<<pTWorkThread->GetThreadHandle() <<endl;
                }
            }
        }
*/

//}
