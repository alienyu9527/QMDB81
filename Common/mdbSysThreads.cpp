#include "Common/mdbSysThreads.h"
#include "Common/mdbDateUtils.h"
//#include "Common/mdbLogInterface.h"
#ifndef OS_WINDOWS
    #include <signal.h>
    #if defined(OS_LINUX)
        #include <sys/syscall.h>
        #define gettid() syscall(__NR_gettid)  
    #elif defined(OS_FREEBSD)
        #include <pthread_np.h>
    #endif
#endif

#define MDB_NTC_THREAD_BASE               -400             //thread操作起始常量
#define MDB_NTC_OS_THREAD_PARAME_ERROR  MDB_NTC_THREAD_BASE-1      //参数错误

//namespace QuickMDB
//{
#ifdef OS_WINDOWS
        __declspec( thread ) HANDLE g_hMdbNtcCurThreadHandle = NULL;
#endif

        //获取当前线程句柄
        MDB_NTC_THREAD_HANDLE  mdb_ntc_zthread_handle()
        {
            MDB_NTC_THREAD_HANDLE  tThreadHandle = 0;
            
#ifdef OS_WINDOWS
            tThreadHandle = g_hMdbNtcCurThreadHandle;
            if(tThreadHandle == NULL)
            {
                g_hMdbNtcCurThreadHandle = tThreadHandle = GetCurrentThread();
            }
#else
            tThreadHandle = pthread_self();
#endif
            
            return tThreadHandle;
        }    
        
        //////////////////////////////////////////////////////////////////////////
        //
        //全局函数定义
        //
        //////////////////////////////////////////////////////////////////////////
        mdb_ntc_thread_local(TMdbNtcThread*, g_pMdbNtcCurThread);

        const char* mdb_ntc_zthread_state_name(MDB_NTC_THREAD_STATE eState)
        {
            switch(eState)
            {
            case MDB_NTC_THREAD_INVALID:
                return "invalid";
                break;
            case MDB_NTC_THREAD_RUN:
                return "run";
                break;
            case MDB_NTC_THREAD_SUSPEND:
                return "suspend";
                break;
            case MDB_NTC_THREAD_EXIT:
                return "exit";
                break;
            default:
                return "unknown";
                break;
            }
        }
        //////////////////////////////////////////////////////////////////////////
        //
        //多线程类TMdbNtcThread定义
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcThread, TMdbNtcComponentObject);
        //构造函数
        TMdbNtcThread::TMdbNtcThread(int iStackSize)
        {
            m_iStackSize = iStackSize;
            m_hHandle         = 0;
            m_iThreadId       = 0;
            m_iExitCode       = 0;//线程退出码，默认值为0
            m_bNeedSuspend    = false;//线程是否挂起，0--不挂起，-1--申请挂起，1--已挂起
            m_bNeedCancel     = false;//是否杀死线程，false--不杀死，true--杀死
            m_eThreadState    = MDB_NTC_THREAD_INVALID;
            pBlockingEvent    = NULL;
            pBlockingNotify   = NULL;
			m_tThreadUpdateTime = 0;
#if defined(OS_UNIX)
            pthread_cond_init(&m_oSuspendCond,NULL); ///条件变量初始化
#endif
        }

        TMdbNtcThread::~TMdbNtcThread()
        {
        #if defined(OS_UNIX)
            pthread_cond_destroy(&m_oSuspendCond); ///条件变量销毁
        #endif
        }

        void TMdbNtcThread::SetThreadState(MDB_NTC_THREAD_STATE eThreadState)
        {
            m_oMutex.Lock();
            m_eThreadState = eThreadState;
            m_oMutex.Unlock();
        }

        //创建线程，并执行线程Execute函数
        bool TMdbNtcThread::Run()
        {
            int iRet = 0;
            //创建线程
            m_iExitCode = 0;
            m_eThreadState = MDB_NTC_THREAD_RUN;
            do
            {
#ifndef OS_WINDOWS
                //定义线程属性并初始化
                pthread_attr_t attr;
                iRet = pthread_attr_init(&attr);
                if ( iRet)
                {
                    TADD_WARNING( "pthread_attr_init failed, errno = %d \n",errno);
                    iRet=errno;
                    break;
                }
                //设置线程栈大小
                if (m_iStackSize > 0)
                {
                    pthread_attr_setstacksize(&attr, (MDB_UINT32)m_iStackSize);
                }
                else
                {
                    iRet= MDB_NTC_OS_THREAD_PARAME_ERROR;
                    break;
                }
                iRet = pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
                if(iRet != 0)
                {
                    TADD_WARNING( "pthread_attr_setdetachstate failed, errno = %d \n",errno);
                    iRet=errno;
                    break;
                }                
                //创建线程
                if (0 != pthread_create(&(m_hHandle), &attr, (mdbThreadFunc)agent, this)) 
                {
                    TADD_WARNING( "pthread_create failed, errno = %d \n",errno);
                    iRet = errno;
                    break;
                }
                //windows平台
#else
                unsigned int uiPthreadId = 0;
                m_hHandle = (HANDLE)_beginthreadex( NULL, m_iStackSize, (mdbThreadFunc)agent, this, 0, &uiPthreadId );
                if( !m_hHandle )
                {
                    iRet = errno;
                    TADD_WARNING( "_beginthreadex failed, errno = %d \n", errno);
                    break;
                }
                else
                {
                    m_iThreadId = uiPthreadId;
                }
#endif
            }while(0);
            //设置线程状态为运行态，刚创建线程，无须互斥修改
            if(iRet != 0)
            {
                TADD_WARNING( "Run() failed \n" );
            }
            else
            {
                m_oRunOkEvent.Wait();
            }
            return iRet==0?true:false;
        }

        //挂起线程
        bool TMdbNtcThread::Suspend ()
        {
            if(this == NULL) return false;
            int iRet = 0;
            //将线程本身挂起
            if (m_hHandle == mdb_ntc_zthread_handle())
            {
                //申请互斥锁
                m_oMutex.Lock();
                //设置挂起状态和线程状态
                m_bNeedSuspend = false;
                m_eThreadState = MDB_NTC_THREAD_SUSPEND;
                m_oMutex.Unlock();
#ifdef OS_IBM
                if( pthread_suspend_np(m_hHandle) != 0 )
                {
                    TADD_WARNING( "mdb_ntc_zthread_suspend failed, errno = %d \n",errno);
                    iRet =  errno;        
                }
#elif defined(OS_HP)
                if(pthread_suspend(m_hHandle) != 0)
                {
                    TADD_WARNING( "mdb_ntc_zthread_suspend failed, errno = %d \n",errno);
                    iRet = errno; 
                }
#elif defined(OS_WINDOWS)
                if (SuspendThread(m_hHandle) != 0)
                {
                    TADD_WARNING( "mdb_ntc_zthread_suspend failed, errno = %d \n",errno);
                    iRet = errno; 
                }
#elif defined(OS_SUN) || defined(OS_LINUX) || defined(OS_UNIX)
                m_oMutex.Lock();
                if (pthread_cond_wait(&(m_oSuspendCond), &m_oMutex.GetMutex()) != 0)
                {
                    TADD_WARNING( "mdb_ntc_zthread_suspend failed, errno = %d \n",errno);
                    iRet = errno; 
                }
                m_oMutex.Unlock();
#endif                
                m_oMutex.Lock();
                m_eThreadState = MDB_NTC_THREAD_RUN;
                m_oMutex.Unlock();
            } 
            //设置其他线程的挂起标识，表示申请挂起
            else
            {
                m_oMutex.Lock();
                m_bNeedSuspend = true;
                m_oMutex.Unlock();
            } 
            return iRet==0?true:false;
        }

        //唤醒线程
        bool TMdbNtcThread::Resume ()
        {
            if(this == NULL) return false;
            bool bRet = true;
        #ifdef OS_IBM
            bRet = pthread_unsuspend_np(m_hHandle) == 0;
        #elif defined(OS_HP)
            bRet = pthread_continue(m_hHandle) == 0;
        #elif defined(OS_WINDOWS)
            bRet = ResumeThread(m_hHandle) > 1;
        #elif defined(OS_SUN) ||defined(OS_LINUX) || defined(OS_UNIX)  
            m_oMutex.Lock();
            bRet = pthread_cond_signal(&(m_oSuspendCond)) == 0;
            m_oMutex.Unlock();
        #endif
            return bRet;
        }

        //杀死线程
        bool TMdbNtcThread::Cancel()
        {
            if(this == NULL) return false;
            int iRet = 0;
            if(m_hHandle == mdb_ntc_zthread_handle())
            {
#ifdef OS_WINDOWS
                Cleanup();
                _endthreadex(0);
#else
                pthread_exit(0);
#endif
            }
            else
            {
                //加锁
                m_oMutex.Lock();
                m_bNeedCancel = true;
                m_oMutex.Unlock();
                if(pBlockingEvent)
                {                    
                    pBlockingEvent->SetEvent();
                }
                else if(pBlockingNotify)
                {
                    pBlockingNotify->Notify();
                }
                
#ifndef OS_WINDOWS
                //pthread_cancel(GetThreadHandle());//取消线程，线程会延迟撤销，这样屏蔽信号的情况下，仍然能够触发accept,select等函数
#endif
            }
            return iRet==0?true:false;
        }

        //杀死线程
        bool TMdbNtcThread::Kill()
        {
            int iRet = 0;
            if(m_hHandle == mdb_ntc_zthread_handle())
            {
#ifdef OS_WINDOWS
                Cleanup();
                _endthreadex(0);
#else
                pthread_exit(0);
#endif
            }
            else
            {
                m_oMutex.Lock();
                m_bNeedCancel = true;
                if(pBlockingEvent)
                {
                    pBlockingEvent->PulseEvent();
                    pBlockingEvent = NULL;
                }
                if(pBlockingNotify)
                {
                    pBlockingNotify->Notify();
                    pBlockingNotify = NULL;
                }
                m_oMutex.Unlock();
#ifdef OS_WINDOWS
                if(TerminateThread(m_hHandle, 0))
                {
                    iRet = WaitForSingleObject(m_hHandle, -1);//检查线程是否真正结束                   
                }
                else
                {
                    iRet = -1;//终止结束
                }
#else
                if(0 == pthread_cancel(m_hHandle))//强制取消线程，根据线程取消类型是否为异步立即执行
                {
                    Wait(-1, NULL);
                }
#endif
            }
            return iRet==0?true:false;
        }

        //等待线程结束，供其他线程调用
        bool TMdbNtcThread::Wait(int iMilliSeconds /* = -1 */, int *pExitCode /* = NULL */)
        {
            if(m_hHandle == 0) return true;//已经结束
            else if(m_hHandle == mdb_ntc_zthread_handle()) return false;//不可以自己等自己
#ifdef OS_WINDOWS
            return WaitForSingleObject(m_hHandle, iMilliSeconds) == 0;//检查线程是否真正结束
#else
            int iTotalWaitMs = 0;
            do
            {
                MDB_NTC_THREAD_HANDLE hHandle = m_hHandle;
                if(hHandle == 0) return true;//已经结束
                else if(pthread_kill(hHandle, 0) != 0)//不为0表示线程已经不存在了
                {
                    return true;
                }
                TMdbNtcDateTime::Sleep(100);//等待100ms
                iTotalWaitMs += 100;
                if(iMilliSeconds != -1 && iTotalWaitMs >= iMilliSeconds)
                {
                    break;
                }
            } while (1);
            return false;
#endif
        }
        /*
        void TMdbNtcThread::TestCancel(void (*callback)(void* pArg), void* pArg)
        {
            //请求取消线程
            if(m_pThread)
            {
                if(bThreadKill)
                {
                    mdb_ntc_zthread_cancel(m_pThread, callback, pArg);
                }
        #ifndef OS_WINDOWS
                //兼容linux/unix下的pthread_cancel，如果上面已经触发线程退出，则不会执行下面的
                pthread_testcancel();
        #endif
            }
        }
        */

        void TMdbNtcThread::CleanupPush(TMdbNtcCleanupCallback* pCleanup)
        {
            m_oMutex.Lock();
            m_lsCleanupCallback.AddTail(pCleanup);
            m_oMutex.Unlock();
        }

        void TMdbNtcThread::Cleanup()
        {
            //作线程的清理工作
            m_bNeedCancel = false;
            m_bNeedSuspend = false;
            m_eThreadState = MDB_NTC_THREAD_EXIT;
#ifdef OS_WINDOWS
            if(m_hHandle)
            {
                CloseHandle(m_hHandle);
            }
#endif
            m_hHandle = 0;
            TMdbNtcBaseList oSwapList;
            oSwapList.SetAutoRelease(true);
            oSwapList.SwapList(m_lsCleanupCallback);
            TMdbNtcBaseList::iterator itor = oSwapList.IterBegin(),
                itor_end = oSwapList.IterEnd();
            for (; itor != itor_end; ++itor)
            {
                TMdbNtcCleanupCallback* pCleanup = static_cast<TMdbNtcCleanupCallback*>(itor.data());
                if(pCleanup)
                {
                    pCleanup->Cleanup();
                }
            }
            oSwapList.Clear();
        }

        void TMdbNtcThread::ThreadCleanup(void* pArg)
        {
            TMdbNtcThread* pThread = reinterpret_cast<TMdbNtcThread*>(pArg);
            if(pThread)
            {
                pThread->Cleanup();
            }
        }

        unsigned int TMdbNtcThread::agent(void* p)
        {
            TMdbNtcThread* pThread = (TMdbNtcThread*)p;
            g_pMdbNtcCurThread = pThread;
#ifdef OS_WINDOWS
            g_hMdbNtcCurThreadHandle = pThread->m_hHandle;
#else
            //屏蔽所有信号
            sigset_t oldmask, newmask;
            sigemptyset(&oldmask);
            sigfillset(&newmask);
            pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);
            pThread->m_hHandle = mdb_ntc_zthread_handle();
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);//更改目标线程的取消状态
            pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);//改变目标线程对于取消请求的响应：异步撤销（立即执行）
            //pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);//改变目标线程对于取消请求的响应：延迟撤销（让线程的处理一直被延迟到撤消点才会去执行）
            pthread_cleanup_push(ThreadCleanup, pThread);
    #if defined(OS_IBM)
            int iPthreadId = 0;
            if(0 != pthread_getunique_np(&(pThread->m_hHandle), &iPthreadId))
            {
                //MDB_NTC_ADD_WARN_TF( "pthread_getunique_np failed, errno = %d \n",errno);
            }
            else
            {
                pThread->m_iThreadId = iPthreadId;
            }
    #elif defined(OS_LINUX)
            pThread->m_iThreadId = (MDB_NTC_THREAD_ID)gettid();
    #elif defined(OS_FREEBSD)
            //freebsd 9.0后支持pthread_getthreadid_np();
            pThread->m_iThreadId = (MDB_NTC_THREAD_ID)pThread->m_hHandle;
            // OS_HP || OS_SUN平台上，句柄和ID相同
    #else
            pThread->m_iThreadId = pThread->m_hHandle;
    #endif
#endif
            pThread->m_oRunOkEvent.SetEvent();
            pThread->m_iExitCode = pThread->Execute();
#ifndef OS_WINDOWS
            pthread_cleanup_pop(0);
#endif
            pThread->Cleanup();//先做清理
            return 0;
        }
//}
