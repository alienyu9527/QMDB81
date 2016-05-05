/**
 * @file mdbSysThreads.h
 * @brief 线程类的封装
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
        //屏蔽不同平台句柄及ID差异
    #ifdef OS_WINDOWS
        typedef MDB_UINT64   MDB_NTC_THREAD_ID;           ///线程ID
        typedef HANDLE   MDB_NTC_THREAD_HANDLE;           ///线程句柄
        typedef unsigned int (__stdcall *mdbThreadFunc)(void* pThreadParam);
    #else
        typedef MDB_UINT64    MDB_NTC_THREAD_ID;                   ///线程ID
        typedef pthread_t MDB_NTC_THREAD_HANDLE;         ///线程句柄
        #ifdef    __cplusplus
        extern "C" {
        #endif
            typedef void* (*mdbThreadFunc)(void* pThreadParam);
        #ifdef    __cplusplus
            }
        #endif
    #endif
        #ifdef OS_WINDOWS
            extern __declspec( thread ) HANDLE g_hMdbNtcCurThreadHandle;///< 当前线程的句柄（使用线程局部存储）
        #endif
        enum MDB_NTC_THREAD_STATE  ///线程状态
        {
            MDB_NTC_THREAD_INVALID  =   0,///无效，初始状态
            MDB_NTC_THREAD_RUN      =   1,///运行态
            MDB_NTC_THREAD_SUSPEND  =   2,///挂起态
            MDB_NTC_THREAD_EXIT     =   3 ///退出态
        };

        /**
         * @brief 用于线程退出时，需要执行的清理函数
         * 重载具体实现后，压入到TMdbNtcThread::CleanupPush，这样线程退出时会依次调用
         * 
         */
        class TMdbNtcCleanupCallback:public TMdbNtcBaseObject
        {
        public:
            /**
             * @brief 回调的清理函数
             * 
             * @return 无
             */
            virtual void Cleanup() = 0;
        };

        class TMdbNtcBaseEvent;
        class TMdbNtcBaseNotify;
        /**
         * @brief 多线程类
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
             * @brief 构造函数
             *
             * @param iStackSize [in] 线程栈大小
             * @return 无
             * @retval
             */
            TMdbNtcThread (int iStackSize = 1024 *1024);
            ~TMdbNtcThread();
            /**
             * @brief 初始化函数，可以重载此函数
             * 
             * @param [in]
             * @return int
             * @retval 0 成功
             */
            /**
             * @brief 获得当前线程栈的大小
             * 
             * @return int
             * @retval 线程栈大小
             */
            inline int GetStackSize() const
            {
                return m_iStackSize;
            }
            /**
             * @brief 设置线程栈大小
             * 
             * @param iStackSize [in] 线程栈大小
             * @return 无
             */
            inline void SetStackSize(int iStackSize)
            {
                m_iStackSize = iStackSize;
            }
            /**
             * @brief 获取线程句柄
             *
             * 获取线程句柄
             *
             * @return MDB_NTC_THREAD_HANDLE 
             * @retval 线程句柄
             */
            inline MDB_NTC_THREAD_HANDLE GetThreadHandle() const
            {
                return this?m_hHandle:0;
            }
            /**
             * @brief 获取线程状态
             *      
             * @return MDB_NTC_THREAD_STATE     
             */
            inline MDB_NTC_THREAD_STATE GetThreadState() const
            {
                return this?m_eThreadState:MDB_NTC_THREAD_INVALID;
            }
            /**
             * @brief 获取线程号
             *
             * @param 无
             * @return MDB_NTC_THREAD_ID 
             * @retval >0 线程ID，其他获取失败
             */
            inline MDB_NTC_THREAD_ID GetThreadId() const
            {
                return this?m_iThreadId:0;
            }
            /** 
             * @brief 设置线程的状态，此函数主要提供给事件/通知等阻塞接口调用，以方便及时更改当前线程的状态
             * 
             * @param eThreadState [in] 线程状态
             * @return 无
             */
            void SetThreadState(MDB_NTC_THREAD_STATE eThreadState);
        public:
            /**
             * @brief 创建线程，并执行线程Execute函数
             *
             * @param 无
             * @return bool 
             * @retval true--成功，其他失败
             */
            bool Run();
            /**
             * @brief 挂起线程
             * 调用者为当前线程时，立刻挂起
             * 调用者为其他线程时，设置挂起标识，需要线程使用TestSuspend检测是否需要挂起，如果是则调用Suspend
             * @return bool
             * @retval true 成功
             */
            bool Suspend();
            /**
             * @brief 唤醒线程
             *
             * @return bool
             * @retval true 成功
             */
            bool  Resume ();
            /**
             * @brief 取消一个线程
             * 注意：linux/unix下并不调用pthread_cancel，而是通过内部机制完成，通过TestCancel/zthread_testcancel函数检测取消
             * 
             * @return bool
             * @retval true 成功
             */
            bool Cancel();
            /**
             * @brief 杀死线程 
             *
             * @param 无
             * @return bool 
             * @retval true--成功，其他失败
             */
            bool Kill();
            /**
             * @brief 等待线程结束,供其他线程调用
             *  
             * @param iMilliSeconds [in] 超时时间，-1表示不做超时处理,单位毫秒数
             * @param pExitCode [in] 不为NULL则赋值为线程的返回码
             * @return bool
             * @retval true--成功；false--线程未退出
             */
           bool Wait(int iMilliSeconds = -1, int *pExitCode = NULL);
           /**
             * @brief 检测是否需要取消当前的线程
             *
             * @return bool
             * @retval true 表示要退出当前线程
             */
            inline bool TestCancel()
            {
                return this?m_bNeedCancel:false;
            }
            /**
             * @brief 检测当前线程是否需要挂起
             * 
             * @return bool
             * @retval true 表示要挂起当前线程
             */
            inline bool TestSuspend()
            {
                return this?m_bNeedSuspend:false;
            }
            /**
             * @brief 往线程中压入一个清理函数
             * 
             * @param pCleanup [in] 清理函数信息
             * @return 无
             */
            void CleanupPush(TMdbNtcCleanupCallback* pCleanup);
        protected:
            /**
             * @brief 线程执行函数, 对于具体线程而言，需要重载此函数
             *
             * @param 无
             * @return  int 
             * @retval  线程退出的返回码
             */	
            virtual int  Execute() = 0;
            /**
             * @brief 线程清理函数，当线程退出时，调用此函数
             * 
             * @return 无
             */
            virtual void Cleanup();
            /**
             * @brief 线程入口函数
             *
             * @param 无
             * @return  void
             * @retval
             */	
            static unsigned int agent(void* p);
        private:
            static void ThreadCleanup(void* pArg);
        public:
            TMdbNtcBaseEvent*   pBlockingEvent;///< 当前线程正在阻塞着的事件
            TMdbNtcBaseNotify*  pBlockingNotify;///< 当前线程正在阻塞着的通知           
            time_t              m_tThreadUpdateTime;// 线程更新时间
        private:
            int	            m_iStackSize;       ///线程栈大小
            MDB_NTC_THREAD_HANDLE   m_hHandle;    ///< 线程句柄
            MDB_NTC_THREAD_ID       m_iThreadId;  ///< 线程ID
            TMdbNtcThreadLock     m_oMutex;      ///< 互斥锁
            bool            m_bNeedSuspend;        ///< 线程是否需要挂起
            bool            m_bNeedCancel;         ///< 线程是否需要退出
            MDB_NTC_THREAD_STATE    m_eThreadState;  ///< 线程状态
            int             m_iExitCode;///线程退出码，默认值为0
            TMdbNtcBaseList       m_lsCleanupCallback;///< 清理函数的链表
            TMdbNtcThreadEvent    m_oRunOkEvent;///< 线程启动完成时的事件
#if defined(OS_UNIX)
            pthread_cond_t  m_oSuspendCond; ///条件变量，实现挂起就靠这个
#endif
        };
        
        #if defined(OS_WINDOWS) || defined(OS_LINUX)
        /**
         * @brief 线程局部存储类，用于真实数据的存储
         * 
         */
        template<typename _Ty>
        class TMdbNtcThreadLocal
        {
        public:
            /**
             * @brief 取得线程局部存储的数据
             *      
             * @return _Ty
             * @retval 线程局部存储的数据
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
             * @brief 重载运算符
             * 
             * @param ty [in] 参数
             * @return _Ty
             * @retval 局部存储数据
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
            _Ty m_oLocalValue;///< 线程局部存储的数据
        };

        #else
        /**
         * @brief 线程局部存储类，用于真实数据的存储
         * 
         */
        template<typename _Ty>
        class TMdbNtcThreadLocal
        {
        public:
            /**
             * @brief 线程局部对象析构函数
             * 
             * @param pointer [in] 线程局部对象的指针
             * @return 无     
             */
            static inline void destructor(void * pointer)
            {
                if(pointer)
                {
                    delete (_Ty*)pointer;
                }
            }
            /**
             * @brief 构造函数
             * 
             */
            TMdbNtcThreadLocal()
            {        
                pthread_key_create(&m_tKey, destructor);
            }
            /**
             * @brief 析构函数
             * 
             */
            ~TMdbNtcThreadLocal()
            {
                pthread_key_delete(m_tKey);
            }
            /**
             * @brief 重载运算符
             * 
             * @param ty [in] 参数
             * @return _Ty
             * @retval 局部存储数据
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
             * @brief 取得线程局部存储的数据
             *      
             * @return _Ty
             * @retval 线程局部存储的数据
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
            pthread_key_t m_tKey;///< 线程局部存储对应的key
        };

        /**
         * @brief 针对指针类型的偏特化
         * 
         */
        template<typename _Ty>
        class TMdbNtcThreadLocal<_Ty*>
        {
        public:
            /**
             * @brief 线程局部对象析构函数
             * 
             * @param pointer [in] 线程局部对象的指针
             * @return 无     
             */
            static inline void destructor(void * pointer)
            {
                if(pointer)
                {
                    delete (_Ty**)pointer;
                }
            }
            /**
             * @brief 构造函数
             * 
             */
            TMdbNtcThreadLocal()
            {        
                pthread_key_create(&m_tKey, destructor);
            }
            /**
             * @brief 析构函数
             * 
             */
            ~TMdbNtcThreadLocal()
            {
                pthread_key_delete(m_tKey);
            }
            /**
             * @brief 重载运算符
             * 
             * @param ty [in] 参数
             * @return _Ty*
             * @retval 局部存储数据
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
             * @brief 取得线程局部存储的数据
             *      
             * @return _Ty*
             * @retval 线程局部存储的数据
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
            pthread_key_t m_tKey;///< 线程局部存储对应的key
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
         * @brief 根据状态获得状态的名称
         * 
         * @return const char*
         * @retval 状态名称
         */
        const char* mdb_ntc_zthread_state_name(MDB_NTC_THREAD_STATE eState);

        /**
         * @brief 获取当前线程信息
         *
         * @return THREAD_STRUCT
         * @retval 线程信息
         */
        inline TMdbNtcThread* mdb_ntc_zthread()
        {
            return g_pMdbNtcCurThread.Value();
        }

        /**
         * @brief 获取当前线程句柄
         * 
         * @param 无
         * @return MDB_NTC_THREAD_HANDLE
         * @retval 当前线程句柄
         */
        MDB_NTC_THREAD_HANDLE mdb_ntc_zthread_handle();
        /**
         * @brief 获取当前线程id
         * 
         * @param 无
         * @return MDB_NTC_THREAD_ID
         * @retval 当前线程id
         */
        inline MDB_NTC_THREAD_ID mdb_ntc_zthread_id()
        {
            return mdb_ntc_zthread()->GetThreadId();
        }

        /**
         * @brief 获取当前线程状态
         *
         * @return int
         * @retval 对应线程的四种状态
         */
        inline MDB_NTC_THREAD_STATE mdb_ntc_zthread_state()
        {
            return g_pMdbNtcCurThread->GetThreadState();
        }

        /**
         * @brief 让当前线程挂起
         * 
         * @return bool
         * @retval true 成功
         */
        inline bool mdb_ntc_zthread_suspend()
        {
            return g_pMdbNtcCurThread->Suspend();
        }
        /**
         * @brief 让当前线程唤醒
         * 
         * @return bool
         * @retval true 成功
         */
        inline bool mdb_ntc_zthread_resume()
        {
            return g_pMdbNtcCurThread->Resume();
        }
        /**
         * @brief 检测当前线程是否需要挂起
         * 
         * @return bool
         * @retval true 表示要挂起当前线程
         */
        inline bool mdb_ntc_zthread_testsuspend()
        {
            return g_pMdbNtcCurThread->TestSuspend();
        }

        /**
         * @brief 让当前线程退出
         * 
         * @return bool
         * @retval true 成功
         */
        inline bool mdb_ntc_zthread_cancel()
        {
            return g_pMdbNtcCurThread->Cancel();
        }
        /**
         * @brief 退出当前线程
         * 
         * @return bool
         * @retval true 成功
         */
        inline bool mdb_ntc_zthread_exit()
        {
            return g_pMdbNtcCurThread->Cancel();
        }

        /**
         * @brief 检测当前线程是否被要求退出
         *
         * @return bool
         * @retval true 表示需要退出当前线程，可以调用zthread_exit/zthread_cancel函数
         */
        inline bool mdb_ntc_zthread_testcancel()
        {
            return g_pMdbNtcCurThread->TestCancel();
        }

        /**
         * @brief 针对线程局部存储指针申请内存的自动释放
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
          * @brief 压入自动释放的对象
          * 
          * @param pPointer [in] 需要线程退出时，自动释放的对象
          * @return bool
          * @retval true 压入成功
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
