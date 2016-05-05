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

#define MDB_NTC_THREAD_BASE               -400             //thread������ʼ����
#define MDB_NTC_OS_THREAD_PARAME_ERROR  MDB_NTC_THREAD_BASE-1      //��������

//namespace QuickMDB
//{
#ifdef OS_WINDOWS
        __declspec( thread ) HANDLE g_hMdbNtcCurThreadHandle = NULL;
#endif

        //��ȡ��ǰ�߳̾��
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
        //ȫ�ֺ�������
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
        //���߳���TMdbNtcThread����
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcThread, TMdbNtcComponentObject);
        //���캯��
        TMdbNtcThread::TMdbNtcThread(int iStackSize)
        {
            m_iStackSize = iStackSize;
            m_hHandle         = 0;
            m_iThreadId       = 0;
            m_iExitCode       = 0;//�߳��˳��룬Ĭ��ֵΪ0
            m_bNeedSuspend    = false;//�߳��Ƿ����0--������-1--�������1--�ѹ���
            m_bNeedCancel     = false;//�Ƿ�ɱ���̣߳�false--��ɱ����true--ɱ��
            m_eThreadState    = MDB_NTC_THREAD_INVALID;
            pBlockingEvent    = NULL;
            pBlockingNotify   = NULL;
			m_tThreadUpdateTime = 0;
#if defined(OS_UNIX)
            pthread_cond_init(&m_oSuspendCond,NULL); ///����������ʼ��
#endif
        }

        TMdbNtcThread::~TMdbNtcThread()
        {
        #if defined(OS_UNIX)
            pthread_cond_destroy(&m_oSuspendCond); ///������������
        #endif
        }

        void TMdbNtcThread::SetThreadState(MDB_NTC_THREAD_STATE eThreadState)
        {
            m_oMutex.Lock();
            m_eThreadState = eThreadState;
            m_oMutex.Unlock();
        }

        //�����̣߳���ִ���߳�Execute����
        bool TMdbNtcThread::Run()
        {
            int iRet = 0;
            //�����߳�
            m_iExitCode = 0;
            m_eThreadState = MDB_NTC_THREAD_RUN;
            do
            {
#ifndef OS_WINDOWS
                //�����߳����Բ���ʼ��
                pthread_attr_t attr;
                iRet = pthread_attr_init(&attr);
                if ( iRet)
                {
                    TADD_WARNING( "pthread_attr_init failed, errno = %d \n",errno);
                    iRet=errno;
                    break;
                }
                //�����߳�ջ��С
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
                //�����߳�
                if (0 != pthread_create(&(m_hHandle), &attr, (mdbThreadFunc)agent, this)) 
                {
                    TADD_WARNING( "pthread_create failed, errno = %d \n",errno);
                    iRet = errno;
                    break;
                }
                //windowsƽ̨
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
            //�����߳�״̬Ϊ����̬���մ����̣߳����뻥���޸�
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

        //�����߳�
        bool TMdbNtcThread::Suspend ()
        {
            if(this == NULL) return false;
            int iRet = 0;
            //���̱߳������
            if (m_hHandle == mdb_ntc_zthread_handle())
            {
                //���뻥����
                m_oMutex.Lock();
                //���ù���״̬���߳�״̬
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
            //���������̵߳Ĺ����ʶ����ʾ�������
            else
            {
                m_oMutex.Lock();
                m_bNeedSuspend = true;
                m_oMutex.Unlock();
            } 
            return iRet==0?true:false;
        }

        //�����߳�
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

        //ɱ���߳�
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
                //����
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
                //pthread_cancel(GetThreadHandle());//ȡ���̣߳��̻߳��ӳٳ��������������źŵ�����£���Ȼ�ܹ�����accept,select�Ⱥ���
#endif
            }
            return iRet==0?true:false;
        }

        //ɱ���߳�
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
                    iRet = WaitForSingleObject(m_hHandle, -1);//����߳��Ƿ���������                   
                }
                else
                {
                    iRet = -1;//��ֹ����
                }
#else
                if(0 == pthread_cancel(m_hHandle))//ǿ��ȡ���̣߳������߳�ȡ�������Ƿ�Ϊ�첽����ִ��
                {
                    Wait(-1, NULL);
                }
#endif
            }
            return iRet==0?true:false;
        }

        //�ȴ��߳̽������������̵߳���
        bool TMdbNtcThread::Wait(int iMilliSeconds /* = -1 */, int *pExitCode /* = NULL */)
        {
            if(m_hHandle == 0) return true;//�Ѿ�����
            else if(m_hHandle == mdb_ntc_zthread_handle()) return false;//�������Լ����Լ�
#ifdef OS_WINDOWS
            return WaitForSingleObject(m_hHandle, iMilliSeconds) == 0;//����߳��Ƿ���������
#else
            int iTotalWaitMs = 0;
            do
            {
                MDB_NTC_THREAD_HANDLE hHandle = m_hHandle;
                if(hHandle == 0) return true;//�Ѿ�����
                else if(pthread_kill(hHandle, 0) != 0)//��Ϊ0��ʾ�߳��Ѿ���������
                {
                    return true;
                }
                TMdbNtcDateTime::Sleep(100);//�ȴ�100ms
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
            //����ȡ���߳�
            if(m_pThread)
            {
                if(bThreadKill)
                {
                    mdb_ntc_zthread_cancel(m_pThread, callback, pArg);
                }
        #ifndef OS_WINDOWS
                //����linux/unix�µ�pthread_cancel����������Ѿ������߳��˳����򲻻�ִ�������
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
            //���̵߳�������
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
            //���������ź�
            sigset_t oldmask, newmask;
            sigemptyset(&oldmask);
            sigfillset(&newmask);
            pthread_sigmask(SIG_BLOCK, &newmask, &oldmask);
            pThread->m_hHandle = mdb_ntc_zthread_handle();
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);//����Ŀ���̵߳�ȡ��״̬
            pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);//�ı�Ŀ���̶߳���ȡ���������Ӧ���첽����������ִ�У�
            //pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);//�ı�Ŀ���̶߳���ȡ���������Ӧ���ӳٳ��������̵߳Ĵ���һֱ���ӳٵ�������Ż�ȥִ�У�
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
            //freebsd 9.0��֧��pthread_getthreadid_np();
            pThread->m_iThreadId = (MDB_NTC_THREAD_ID)pThread->m_hHandle;
            // OS_HP || OS_SUNƽ̨�ϣ������ID��ͬ
    #else
            pThread->m_iThreadId = pThread->m_hHandle;
    #endif
#endif
            pThread->m_oRunOkEvent.SetEvent();
            pThread->m_iExitCode = pThread->Execute();
#ifndef OS_WINDOWS
            pthread_cleanup_pop(0);
#endif
            pThread->Cleanup();//��������
            return 0;
        }
//}
