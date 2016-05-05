/**
 * @file mdbSysThreads.h
 * @brief �߳���ķ�װ
 *
 *
 * @author Ge.zhengyi, Jiang.jinzhou, Du.jiagen, Zhang.he
 * @version 1.0
 * @date 20121214
 * @warning
 */
#ifndef _MDB_H_SysThreads_
#define _MDB_H_SysThreads_
#ifndef OS_WINDOWS
    #include <stdio.h>
    #include <pthread.h>
    #include <unistd.h>
#endif
#include "Common/mdbComponent.h"
#include "Common/mdbSysLocks.h"
#include "Common/mdbDataStructs.h"

//namespace QuickMDB
//{
        //���β�ͬƽ̨�����ID����
    #ifdef OS_WINDOWS
        typedef MDB_UINT64   MDB_NTC_THREAD_ID;           ///�߳�ID
        typedef HANDLE   MDB_NTC_THREAD_HANDLE;           ///�߳̾��
        typedef unsigned int (__stdcall *mdbThreadFunc)(void* pThreadParam);
    #else
        typedef MDB_UINT64    MDB_NTC_THREAD_ID;                   ///�߳�ID
        typedef pthread_t MDB_NTC_THREAD_HANDLE;         ///�߳̾��
        #ifdef    __cplusplus
        extern "C" {
        #endif
            typedef void* (*mdbThreadFunc)(void* pThreadParam);
        #ifdef    __cplusplus
            }
        #endif
    #endif
        #ifdef OS_WINDOWS
            extern __declspec( thread ) HANDLE g_hMdbNtcCurThreadHandle;///< ��ǰ�̵߳ľ����ʹ���ֲ߳̾��洢��
        #endif
        enum MDB_NTC_THREAD_STATE  ///�߳�״̬
        {
            MDB_NTC_THREAD_INVALID  =   0,///��Ч����ʼ״̬
            MDB_NTC_THREAD_RUN      =   1,///����̬
            MDB_NTC_THREAD_SUSPEND  =   2,///����̬
            MDB_NTC_THREAD_EXIT     =   3 ///�˳�̬
        };

        /**
         * @brief �����߳��˳�ʱ����Ҫִ�е�������
         * ���ؾ���ʵ�ֺ�ѹ�뵽TMdbNtcThread::CleanupPush�������߳��˳�ʱ�����ε���
         * 
         */
        class TMdbNtcCleanupCallback:public TMdbNtcBaseObject
        {
        public:
            /**
             * @brief �ص���������
             * 
             * @return ��
             */
            virtual void Cleanup() = 0;
        };

        class TMdbNtcBaseEvent;
        class TMdbNtcBaseNotify;
        /**
         * @brief ���߳���
         *
         */
        class TMdbNtcThread:public TMdbNtcComponentObject
        {
            /** \example  example_TMdbNtcThread.cpp
              * This is an example of how to use the TMdbNtcThread class.
              * More details about this example.
              */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThread);
        public:
            /**
             * @brief ���캯��
             *
             * @param iStackSize [in] �߳�ջ��С
             * @return ��
             * @retval
             */
            TMdbNtcThread (int iStackSize = 1024 *1024);
            ~TMdbNtcThread();
            /**
             * @brief ��ʼ���������������ش˺���
             * 
             * @param [in]
             * @return int
             * @retval 0 �ɹ�
             */
            /**
             * @brief ��õ�ǰ�߳�ջ�Ĵ�С
             * 
             * @return int
             * @retval �߳�ջ��С
             */
            inline int GetStackSize() const
            {
                return m_iStackSize;
            }
            /**
             * @brief �����߳�ջ��С
             * 
             * @param iStackSize [in] �߳�ջ��С
             * @return ��
             */
            inline void SetStackSize(int iStackSize)
            {
                m_iStackSize = iStackSize;
            }
            /**
             * @brief ��ȡ�߳̾��
             *
             * ��ȡ�߳̾��
             *
             * @return MDB_NTC_THREAD_HANDLE 
             * @retval �߳̾��
             */
            inline MDB_NTC_THREAD_HANDLE GetThreadHandle() const
            {
                return this?m_hHandle:0;
            }
            /**
             * @brief ��ȡ�߳�״̬
             *      
             * @return MDB_NTC_THREAD_STATE     
             */
            inline MDB_NTC_THREAD_STATE GetThreadState() const
            {
                return this?m_eThreadState:MDB_NTC_THREAD_INVALID;
            }
            /**
             * @brief ��ȡ�̺߳�
             *
             * @param ��
             * @return MDB_NTC_THREAD_ID 
             * @retval >0 �߳�ID��������ȡʧ��
             */
            inline MDB_NTC_THREAD_ID GetThreadId() const
            {
                return this?m_iThreadId:0;
            }
            /** 
             * @brief �����̵߳�״̬���˺�����Ҫ�ṩ���¼�/֪ͨ�������ӿڵ��ã��Է��㼰ʱ���ĵ�ǰ�̵߳�״̬
             * 
             * @param eThreadState [in] �߳�״̬
             * @return ��
             */
            void SetThreadState(MDB_NTC_THREAD_STATE eThreadState);
        public:
            /**
             * @brief �����̣߳���ִ���߳�Execute����
             *
             * @param ��
             * @return bool 
             * @retval true--�ɹ�������ʧ��
             */
            bool Run();
            /**
             * @brief �����߳�
             * ������Ϊ��ǰ�߳�ʱ�����̹���
             * ������Ϊ�����߳�ʱ�����ù����ʶ����Ҫ�߳�ʹ��TestSuspend����Ƿ���Ҫ��������������Suspend
             * @return bool
             * @retval true �ɹ�
             */
            bool Suspend();
            /**
             * @brief �����߳�
             *
             * @return bool
             * @retval true �ɹ�
             */
            bool  Resume ();
            /**
             * @brief ȡ��һ���߳�
             * ע�⣺linux/unix�²�������pthread_cancel������ͨ���ڲ�������ɣ�ͨ��TestCancel/zthread_testcancel�������ȡ��
             * 
             * @return bool
             * @retval true �ɹ�
             */
            bool Cancel();
            /**
             * @brief ɱ���߳� 
             *
             * @param ��
             * @return bool 
             * @retval true--�ɹ�������ʧ��
             */
            bool Kill();
            /**
             * @brief �ȴ��߳̽���,�������̵߳���
             *  
             * @param iMilliSeconds [in] ��ʱʱ�䣬-1��ʾ������ʱ����,��λ������
             * @param pExitCode [in] ��ΪNULL��ֵΪ�̵߳ķ�����
             * @return bool
             * @retval true--�ɹ���false--�߳�δ�˳�
             */
           bool Wait(int iMilliSeconds = -1, int *pExitCode = NULL);
           /**
             * @brief ����Ƿ���Ҫȡ����ǰ���߳�
             *
             * @return bool
             * @retval true ��ʾҪ�˳���ǰ�߳�
             */
            inline bool TestCancel()
            {
                return this?m_bNeedCancel:false;
            }
            /**
             * @brief ��⵱ǰ�߳��Ƿ���Ҫ����
             * 
             * @return bool
             * @retval true ��ʾҪ����ǰ�߳�
             */
            inline bool TestSuspend()
            {
                return this?m_bNeedSuspend:false;
            }
            /**
             * @brief ���߳���ѹ��һ��������
             * 
             * @param pCleanup [in] ��������Ϣ
             * @return ��
             */
            void CleanupPush(TMdbNtcCleanupCallback* pCleanup);
        protected:
            /**
             * @brief �߳�ִ�к���, ���ھ����̶߳��ԣ���Ҫ���ش˺���
             *
             * @param ��
             * @return  int 
             * @retval  �߳��˳��ķ�����
             */	
            virtual int  Execute() = 0;
            /**
             * @brief �߳������������߳��˳�ʱ�����ô˺���
             * 
             * @return ��
             */
            virtual void Cleanup();
            /**
             * @brief �߳���ں���
             *
             * @param ��
             * @return  void
             * @retval
             */	
            static unsigned int agent(void* p);
        private:
            static void ThreadCleanup(void* pArg);
        public:
            TMdbNtcBaseEvent*   pBlockingEvent;///< ��ǰ�߳����������ŵ��¼�
            TMdbNtcBaseNotify*  pBlockingNotify;///< ��ǰ�߳����������ŵ�֪ͨ           
            time_t              m_tThreadUpdateTime;// �̸߳���ʱ��
        private:
            int	            m_iStackSize;       ///�߳�ջ��С
            MDB_NTC_THREAD_HANDLE   m_hHandle;    ///< �߳̾��
            MDB_NTC_THREAD_ID       m_iThreadId;  ///< �߳�ID
            TMdbNtcThreadLock     m_oMutex;      ///< ������
            bool            m_bNeedSuspend;        ///< �߳��Ƿ���Ҫ����
            bool            m_bNeedCancel;         ///< �߳��Ƿ���Ҫ�˳�
            MDB_NTC_THREAD_STATE    m_eThreadState;  ///< �߳�״̬
            int             m_iExitCode;///�߳��˳��룬Ĭ��ֵΪ0
            TMdbNtcBaseList       m_lsCleanupCallback;///< ������������
            TMdbNtcThreadEvent    m_oRunOkEvent;///< �߳��������ʱ���¼�
#if defined(OS_UNIX)
            pthread_cond_t  m_oSuspendCond; ///����������ʵ�ֹ���Ϳ����
#endif
        };
        
        #if defined(OS_WINDOWS) || defined(OS_LINUX)
        /**
         * @brief �ֲ߳̾��洢�࣬������ʵ���ݵĴ洢
         * 
         */
        template<typename _Ty>
        class TMdbNtcThreadLocal
        {
        public:
            /**
             * @brief ȡ���ֲ߳̾��洢������
             *      
             * @return _Ty
             * @retval �ֲ߳̾��洢������
             */
            inline _Ty& Value()
            {
                return m_oLocalValue;
            }
            inline const _Ty& Value() const
            {
                return m_oLocalValue;
            }
            /**
             * @brief ���������
             * 
             * @param ty [in] ����
             * @return _Ty
             * @retval �ֲ��洢����
             */
            inline _Ty& operator = (const _Ty& ty)
            {
                return m_oLocalValue = const_cast<_Ty&>(ty);
            }
            inline bool operator < (const _Ty& ty) const
            {
                return m_oLocalValue < ty;
            }
            inline bool operator <= (const _Ty& ty) const
            {
                return m_oLocalValue <= ty;
            }
            inline bool operator > (const _Ty& ty) const
            {
                return m_oLocalValue > ty;
            }
            inline bool operator >= (const _Ty& ty) const
            {
                return m_oLocalValue >= ty;
            }
            inline bool operator == (const _Ty& ty) const
            {
                return m_oLocalValue == ty;
            }
            inline bool operator != (const _Ty& ty) const
            {
                return m_oLocalValue != ty;
            }
            inline bool operator !() const
            {
                return !m_oLocalValue;
            }
            inline operator const _Ty&() const
            {
                return m_oLocalValue;
            }
            inline _Ty& operator ->()
            {
                return m_oLocalValue;
            }
            _Ty m_oLocalValue;///< �ֲ߳̾��洢������
        };

        #else
        /**
         * @brief �ֲ߳̾��洢�࣬������ʵ���ݵĴ洢
         * 
         */
        template<typename _Ty>
        class TMdbNtcThreadLocal
        {
        public:
            /**
             * @brief �ֲ߳̾�������������
             * 
             * @param pointer [in] �ֲ߳̾������ָ��
             * @return ��     
             */
            static inline void destructor(void * pointer)
            {
                if(pointer)
                {
                    delete (_Ty*)pointer;
                }
            }
            /**
             * @brief ���캯��
             * 
             */
            TMdbNtcThreadLocal()
            {        
                pthread_key_create(&m_tKey, destructor);
            }
            /**
             * @brief ��������
             * 
             */
            ~TMdbNtcThreadLocal()
            {
                pthread_key_delete(m_tKey);
            }
            /**
             * @brief ���������
             * 
             * @param ty [in] ����
             * @return _Ty
             * @retval �ֲ��洢����
             */
            inline _Ty& operator = (const _Ty& ty)
            {        
                return Value() = ty;
            }
            inline bool operator < (const _Ty& ty) const
            {
                return Value() < ty;
            }
            inline bool operator <= (const _Ty& ty) const
            {
                return Value() <= ty;
            }
            inline bool operator > (const _Ty& ty) const
            {
                return Value() > ty;
            }
            inline bool operator >= (const _Ty& ty) const
            {
                return Value() >= ty;
            }
            inline bool operator == (const _Ty& ty) const
            {
                return Value() == ty;
            }
            inline bool operator != (const _Ty& ty) const
            {
                return Value() != ty;
            }
            inline bool operator !() const
            {
                return !Value();
            }
            inline operator const _Ty&() const
            {
                return Value();
            }
            inline const _Ty& Value() const
            {
                return const_cast<TMdbNtcThreadLocal<_Ty>* >(this)->Value();
            }
            /**
             * @brief ȡ���ֲ߳̾��洢������
             *      
             * @return _Ty
             * @retval �ֲ߳̾��洢������
             */
            _Ty& Value()
            {
                _Ty* pLocalValue = (_Ty*)pthread_getspecific(m_tKey);        
                if(pLocalValue == NULL)
                {
                    pLocalValue = new _Ty;
                    pthread_setspecific(m_tKey, pLocalValue);
                }
                return *pLocalValue;
            }
        private:
            pthread_key_t m_tKey;///< �ֲ߳̾��洢��Ӧ��key
        };

        /**
         * @brief ���ָ�����͵�ƫ�ػ�
         * 
         */
        template<typename _Ty>
        class TMdbNtcThreadLocal<_Ty*>
        {
        public:
            /**
             * @brief �ֲ߳̾�������������
             * 
             * @param pointer [in] �ֲ߳̾������ָ��
             * @return ��     
             */
            static inline void destructor(void * pointer)
            {
                if(pointer)
                {
                    delete (_Ty**)pointer;
                }
            }
            /**
             * @brief ���캯��
             * 
             */
            TMdbNtcThreadLocal()
            {        
                pthread_key_create(&m_tKey, destructor);
            }
            /**
             * @brief ��������
             * 
             */
            ~TMdbNtcThreadLocal()
            {
                pthread_key_delete(m_tKey);
            }
            /**
             * @brief ���������
             * 
             * @param ty [in] ����
             * @return _Ty*
             * @retval �ֲ��洢����
             */
            inline _Ty*& operator = (const _Ty* ty)
            {        
                return Value() = const_cast<_Ty*>(ty);
            }
            inline bool operator < (const _Ty*& ty) const
            {
                return Value() < ty;
            }
            inline bool operator <= (const _Ty*& ty) const
            {
                return Value() <= ty;
            }
            inline bool operator > (const _Ty*& ty) const
            {
                return Value() > ty;
            }
            inline bool operator >= (const _Ty*& ty) const
            {
                return Value() >= ty;
            }
            inline bool operator == (const _Ty*& ty) const
            {
                return Value() == ty;
            }
            inline bool operator != (const _Ty* ty) const
            {
                return Value() != ty;
            }
            inline bool operator !() const
            {
                return !Value();
            }
            inline operator _Ty*&()
            {
                return Value();
            }
            inline _Ty*& operator ->()
            {
                return Value();
            }
            inline const _Ty*& Value() const
            {
                return const_cast<TMdbNtcThreadLocal<_Ty*>* >(this)->Value();
            }
            /**
             * @brief ȡ���ֲ߳̾��洢������
             *      
             * @return _Ty*
             * @retval �ֲ߳̾��洢������
             */
            _Ty*& Value()
            {
                _Ty** pLocalValue = (_Ty**)pthread_getspecific(m_tKey);        
                if(pLocalValue == NULL)
                {
                    pLocalValue = new _Ty*;
                    memset(pLocalValue, 0x00, sizeof(_Ty*));
                    pthread_setspecific(m_tKey, pLocalValue);
                }
                return *pLocalValue;
            }
        private:
            pthread_key_t m_tKey;///< �ֲ߳̾��洢��Ӧ��key
        };
        #endif

        #ifdef OS_WINDOWS
            #define mdb_ntc_thread_local(type, var) __declspec( thread )  /*QuickMDB::*/TMdbNtcThreadLocal<type> var
        #elif defined(OS_LINUX)
            #define mdb_ntc_thread_local(type, var) __thread /*QuickMDB::*/TMdbNtcThreadLocal<type> var
        #else
            #define mdb_ntc_thread_local(type, var) /*QuickMDB::*/TMdbNtcThreadLocal<type> var
        #endif
        #define mdb_ntc_extern_thread_local extern mdb_ntc_thread_local
        #define mdb_ntc_static_thread_local static mdb_ntc_thread_local

        mdb_ntc_extern_thread_local(TMdbNtcThread*, g_pMdbNtcCurThread);
        /**
         * @brief ����״̬���״̬������
         * 
         * @return const char*
         * @retval ״̬����
         */
        const char* mdb_ntc_zthread_state_name(MDB_NTC_THREAD_STATE eState);

        /**
         * @brief ��ȡ��ǰ�߳���Ϣ
         *
         * @return THREAD_STRUCT
         * @retval �߳���Ϣ
         */
        inline TMdbNtcThread* mdb_ntc_zthread()
        {
            return g_pMdbNtcCurThread.Value();
        }

        /**
         * @brief ��ȡ��ǰ�߳̾��
         * 
         * @param ��
         * @return MDB_NTC_THREAD_HANDLE
         * @retval ��ǰ�߳̾��
         */
        MDB_NTC_THREAD_HANDLE mdb_ntc_zthread_handle();
        /**
         * @brief ��ȡ��ǰ�߳�id
         * 
         * @param ��
         * @return MDB_NTC_THREAD_ID
         * @retval ��ǰ�߳�id
         */
        inline MDB_NTC_THREAD_ID mdb_ntc_zthread_id()
        {
            return mdb_ntc_zthread()->GetThreadId();
        }

        /**
         * @brief ��ȡ��ǰ�߳�״̬
         *
         * @return int
         * @retval ��Ӧ�̵߳�����״̬
         */
        inline MDB_NTC_THREAD_STATE mdb_ntc_zthread_state()
        {
            return g_pMdbNtcCurThread->GetThreadState();
        }

        /**
         * @brief �õ�ǰ�̹߳���
         * 
         * @return bool
         * @retval true �ɹ�
         */
        inline bool mdb_ntc_zthread_suspend()
        {
            return g_pMdbNtcCurThread->Suspend();
        }
        /**
         * @brief �õ�ǰ�̻߳���
         * 
         * @return bool
         * @retval true �ɹ�
         */
        inline bool mdb_ntc_zthread_resume()
        {
            return g_pMdbNtcCurThread->Resume();
        }
        /**
         * @brief ��⵱ǰ�߳��Ƿ���Ҫ����
         * 
         * @return bool
         * @retval true ��ʾҪ����ǰ�߳�
         */
        inline bool mdb_ntc_zthread_testsuspend()
        {
            return g_pMdbNtcCurThread->TestSuspend();
        }

        /**
         * @brief �õ�ǰ�߳��˳�
         * 
         * @return bool
         * @retval true �ɹ�
         */
        inline bool mdb_ntc_zthread_cancel()
        {
            return g_pMdbNtcCurThread->Cancel();
        }
        /**
         * @brief �˳���ǰ�߳�
         * 
         * @return bool
         * @retval true �ɹ�
         */
        inline bool mdb_ntc_zthread_exit()
        {
            return g_pMdbNtcCurThread->Cancel();
        }

        /**
         * @brief ��⵱ǰ�߳��Ƿ�Ҫ���˳�
         *
         * @return bool
         * @retval true ��ʾ��Ҫ�˳���ǰ�̣߳����Ե���zthread_exit/zthread_cancel����
         */
        inline bool mdb_ntc_zthread_testcancel()
        {
            return g_pMdbNtcCurThread->TestCancel();
        }

        /**
         * @brief ����ֲ߳̾��洢ָ�������ڴ���Զ��ͷ�
         * 
         */
         template<typename _Ty>
         class TMdbNtcTLSPointer:public TMdbNtcCleanupCallback
         {
         public:
             _Ty* pPointer;
             TMdbNtcTLSPointer(_Ty* pPointer)
             {
                 this->pPointer = pPointer;
             }
             void virtual Cleanup()
             {
                 if(pPointer)
                 {
                     delete pPointer;
                     pPointer = NULL;
                 }
             }
         };

         /**
          * @brief ѹ���Զ��ͷŵĶ���
          * 
          * @param pPointer [in] ��Ҫ�߳��˳�ʱ���Զ��ͷŵĶ���
          * @return bool
          * @retval true ѹ��ɹ�
          */
         template<typename _Ty>
         bool mdb_ntc_zthread_cleanup_push(_Ty* pPointer)
         {
             TMdbNtcThread* pThread = mdb_ntc_zthread();
             if(pThread == NULL) return false;
             else
             {                 
                 pThread->CleanupPush(new TMdbNtcTLSPointer<_Ty>(pPointer));
                 return true;
             }
         }
//    }
//}
#endif
