/**
 * @file Sdk/mdbSysLocks.h
 * @brief 与锁相关的封装
 *
 * 与锁相关的封装
 *
 * @author Ge.zhengyi, Jiang.jinzhou, Du.jiagen, Zhang.he
 * @version 1.0
 * @date 20121214
 * @warning
 */
#ifndef _MDB_NTC_H_SysLocks_
#define _MDB_NTC_H_SysLocks_
//#include "Sdk/mdbCommons.h"
//#include "Sdk/mdbBaseObject.h"
#include "Common/mdbSysShareMems.h"
//#include "Sdk/mdbComponent.h"
//#include "Sdk/mdbSysUtils.h"
#ifndef OS_WINDOWS
#include <pthread.h>
#endif
//namespace QuickMDB
//{
//    namespace BillingSDK
//    {

        /*
        * @brief 锁基类
        *
        * 通过派生类实现进程间、线程间的各种锁操作。
        */
        class TMdbNtcBaseLock
        {
        public:
            virtual ~TMdbNtcBaseLock()
            {
            }
            virtual bool Lock(int iMilliSeconds=-1) = 0;
            /**
             * @brief 尝试加锁
             *
             * @return bool
             * @retval true 成功
             */
            virtual bool TryLock()
            {
                return Lock(0);
            }
            virtual bool Unlock() = 0;
        };
        /*
        * @brief 进程锁操作类
        *
        * 实现进程间各种操作
        */
        class TMdbNtcProcessLock : public TMdbNtcBaseLock, public TMdbNtcSystemNameObject
        {

        /** \example example_TSem.cpp
        * This is an example of how to use the TSem class.
        * More details about this example.
        */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessLock);
        public:
            /**
            * @brief 构造函数
            *
            * 类的构造函数
            *
            * @param pszSemName [in] 信号量名称
            * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
            * @param pParentNameObject [in] 所隶属的命名对象
            */
            TMdbNtcProcessLock( const char *pszSemName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);

            /**
            * @brief 析构函数
            *
            * 类的析构函数
            */
            virtual ~TMdbNtcProcessLock( );

            bool IsOK() const;

           /**
            * @brief 加锁
            *
            * 加锁操作
            *
            * @parame lMilliSecond [in] 超时(单位：毫秒)
            * @return bool
            * @retval true  成功
            * @retval false 失败
            */
            bool Lock(int iMilliSecond = -1);
            /**
            * @brief 解锁
            *
            * 解锁操作
            *
            * @return bool
            * @retval true  成功
            * @retval false 失败
            */
            bool Unlock();

            /**
            * @brief 主动销毁
            *
            * 销毁信号量
            *
            * @return bool
            * @retval true  成功
            * @retval false 失败
            */
            virtual bool ManualFree();

            /**
            * @brief 输出数据成员
            *
            * 输出数据成员
            *
            * @return TMdbNtcStringBuffer
            */
            TMdbNtcStringBuffer ToString( );
        public://提供一些静态方法
            /**
             * @brief 检测互斥锁是否存在
             *
             * @param pszName [in] 互斥锁的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 存在
             */
            static bool CheckExist(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 销毁互斥锁
             *
             * @param pszName [in] 互斥锁的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 成功
             */
            static bool Destroy(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        private:
            MDB_IPC_HANDLE m_vHandle;
        };

        /**
         * @brief 跨进程锁池
         *
         */
        class TMdbNtcProcessLockPool:public TMdbNtcSystemNameObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessLockPool);
            /**
             * @brief 锁的基本信息
             *
             */
            class TLockInfo
            {
            public:
                MDB_UINT32  uiIndex;
                bool    bUsed;
                pid_t   uiCurPid;
                char    szName[MDB_NTC_ZS_MAX_IPC_KEY_NAME_SIZE];
            };
            /**
             * @brief 锁池中是锁基类
             *
             */
            class TPoolLock:public TMdbNtcBaseLock, public TMdbNtcSystemNameObject
            {
                MDB_ZF_DECLARE_OBJECT(TPoolLock);
            public:
                virtual ~TPoolLock();
                virtual bool IsOK() const;
                virtual TMdbNtcStringBuffer ToString() const;
                virtual bool ManualFree();
            protected:
                /**
                 * @brief 构造函数
                 *
                 * @param pLockInfo [in] 锁的信息
                 * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
                 * @param pParentNameObject [in] 所隶属的命名对象
                 */
                TPoolLock(TLockInfo *pLockInfo, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject* pParentNameObject = NULL);
            protected:
                TLockInfo *m_pLockInfo;///< 锁的信息
            };
        public:
            virtual TMdbNtcStringBuffer ToString() const;
            /**
             * @brief 获得一个锁
             *
             * @param pszName [in] 锁的名称
             * @param pParentNameObject [in] 所隶属的父级命名对象，默认为NULL
             * @return TMdbNtcBaseLock*
             * @retval 非NULL 成功，无需调用者delete
             */
            TMdbNtcBaseLock* GetLock(const char *pszName, TMdbNtcSystemNameObject* pParentNameObject = NULL);
            /**
             * @brief 根据索引获得一个锁
             *
             * @param uiIndex [in] 所在的序列号
             * @param pParentNameObject [in] 所隶属的父级命名对象，默认为NULL
             * @return TMdbNtcBaseLock*
             * @retval 非NULL 成功，无需调用者delete
             */
            TMdbNtcBaseLock* GetLock(MDB_UINT32 uiIndex, TMdbNtcSystemNameObject* pParentNameObject = NULL);
            /**
             * @brief 销毁锁池
             *
             * @return bool
             * @retval true 成功
             */
            bool ManualFree();
            /**
             * @brief 获得池的锁容量
             *
             * @return MDB_UINT32
             */
            inline MDB_UINT32 GetLockCount() const
            {
                return m_pPoolBuffer?(*(MDB_UINT32*)m_pPoolBuffer->GetBuffer()):0;
            }
            /**
             * @brief 检测某名称的锁是否存在
             *
             * @param pszName [in] 锁的名称
             * @return bool
             * @retval true 存在
             */
            inline bool CheckExist(const char* pszName)
            {
                return FindIndex(pszName)!=-1;
            }
            /**
             * @brief 销毁锁
             *
             * @param pszName [in] 锁的名称
             * @return bool
             * @retval true 成功
             */
            inline bool Destroy(const char* pszName)
            {
                int iIndex = FindIndex(pszName);
                if(iIndex == -1) return true;
                else return Destroy((MDB_UINT32)iIndex);
            }
            /**
             * @brief 销毁指定位置的锁
             *
             * @param uiIndex [in] 所在的序列号
             * @return bool
             * @retval true 成功
             */
            virtual bool Destroy(MDB_UINT32 uiIndex);
        protected:
            /**
             * @brief 构造函数
             *
             * @param uiLockSize [in] 每个锁占用的大小
             * @param pszPoolName [in] 锁池的名称
             * @param uiLockCount [in] 锁的数目，如果为0，则表示附接到ipc实例的锁池
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @param bCheckVersion [in] 是否检查共享内存的版本，以免不同版本的共享内存结构差异导致访问异常
             */
            TMdbNtcProcessLockPool(MDB_UINT32 uiLockSize, const char* pszPoolName, MDB_UINT32 uiLockCount = 0, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, bool bCheckVersion = true);
            virtual ~TMdbNtcProcessLockPool();
            /**
             * @brief 根据锁名称获得所在的索引
             *
             * @param pszName [in] 锁的名称
             * @return int
             * @retval -1 没有找到
             */
            int FindIndex(const char* pszName);
            /**
             * @brief 获得新的锁对象
             *
             * @param pLockInfo         [in] 锁信息
             * @param pParentNameObject [in] 锁所属的命名对象
             * @return TMdbNtcProcessLock*
             */
            virtual TPoolLock* GetNewLock(TLockInfo* pLockInfo, TMdbNtcSystemNameObject* pParentNameObject) = 0;
            /**
             * @brief 获得指定位置的所信息
             *
             * @param uiIndex [in] 锁的序列号
             * @return TLockInfo*
             */
            inline TLockInfo* GetLockInfo(MDB_UINT32 uiIndex)
            {
                return m_pPoolBuffer==NULL?NULL:reinterpret_cast<TLockInfo*>(m_pPoolBuffer->GetBuffer()+sizeof(MDB_UINT32)+uiIndex*m_uiLockSize);
            }
        protected:
            TMdbNtcProcessLock*       m_pSemaphore;
            TMdbNtcShareMem*          m_pPoolBuffer;
            TPoolLock**         m_ppLocks;///< 池内所有的锁
            MDB_UINT32              m_uiLockSize;///< 每个锁占用的大小
        };

        /**
         * @brief 高性能锁
         *
         */
        class TMdbNtcProcessPerfLock : public TMdbNtcProcessLockPool::TPoolLock
        {
            friend class TMdbNtcProcessPerfLockPool;//只能由此友元类创建类对象
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessPerfLock);
        protected:
            /**
             * @brief 锁的基本信息
             *
             */
            class TLockInfo:public TMdbNtcProcessLockPool::TLockInfo
            {
                friend class TMdbNtcProcessPerfLock;
            protected:
#ifndef OS_WINDOWS
                pthread_mutex_t     m_hLock;
#endif
            };
        public:
            virtual ~TMdbNtcProcessPerfLock();
            virtual bool ManualFree();
            virtual TMdbNtcStringBuffer ToString() const;
            bool Lock(int iMilliSeconds=-1);
            bool Unlock();
            /**
             * @brief 检测某名称的高性能锁是否存在
             *
             * @param pszPoolName [in] 高性能锁池的名称
             * @param pszName [in] 高性能锁的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 存在
             */
            static bool CheckExist(const char* pszPoolName, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 销毁高性能锁
             *
             * @param pszPoolName [in] 高性能锁池的名称
             * @param pszName [in] 高性能锁的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 成功
             */
            static bool Destroy(const char* pszPoolName, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        protected:
            /**
             * @brief 构造函数
             *
             * @param pLockInfo [in] 锁的信息
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @param pParentNameObject [in] 所隶属的命名对象
             */
            TMdbNtcProcessPerfLock(TLockInfo *pLockInfo, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);
        protected:
#ifdef OS_WINDOWS
            HANDLE m_hLockHandle;
#endif
        };

        /**
         * @brief 跨进程的高性能锁池
         *
         */
        class TMdbNtcProcessPerfLockPool : public TMdbNtcProcessLockPool
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessPerfLockPool);
        public:
            /**
             * @brief 构造函数
             *
             * @param pszPoolName [in] 高性能锁池的名称
             * @param uiLockCount [in] 高性能锁的数目，如果为0，则表示附接到ipc实例的高性能锁池
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @param bCheckVersion [in] 是否检查共享内存的版本，以免不同版本的共享内存结构差异导致访问异常
             */
            TMdbNtcProcessPerfLockPool(const char* pszPoolName, MDB_UINT32 uiLockCount = 0, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, bool bCheckVersion = true);
            /**
             * @brief 对指定序号的锁进行加锁
             *
             * @param uiIndex       [in] 锁的序号
             * @param iMilliSeconds [in] 等待的时间
             * @return bool
             * @retval true 成功
             */
            bool Lock(MDB_UINT32 uiIndex, int iMilliSeconds=-1);
            /**
             * @brief 尝试对指定序号的锁进行加锁
             *
             * @param uiIndex       [in] 锁的序号
             * @return bool
             * @retval true 成功
             */
            bool TryLock(MDB_UINT32 uiIndex);
            /**
             * @brief 对指定序号的锁进行解锁
             *
             * @param uiIndex       [in] 锁的序号
             * @return bool
             * @retval true 成功
             */
            bool Unlock(MDB_UINT32 uiIndex);
        protected:
            virtual TMdbNtcProcessLockPool::TPoolLock* GetNewLock(TLockInfo* pLockInfo, TMdbNtcSystemNameObject* pParentNameObject);
        public://提供一些静态方法
            /**
             * @brief 销毁高性能锁的池
             *
             * @param pszPoolName [in] 高性能锁池的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 成功
             */
            static bool DestroyPool(const char* pszPoolName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        };

        /**
         * @brief 跨进程读写锁
         *
         */
        class TMdbNtcProcessRWLock : public TMdbNtcProcessLockPool::TPoolLock
        {
            friend class TMdbNtcProcessRWLockPool;//只能由此友元类创建类对象
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessRWLock);
        protected:
            /**
             * @brief 锁的基本信息
             *
             */
            class TLockInfo:public TMdbNtcProcessLockPool::TLockInfo
            {
                friend class TMdbNtcProcessRWLock;
            protected:
#ifndef OS_WINDOWS
                pthread_rwlock_t        m_hRWLock;
#endif
            };
        public:
            virtual ~TMdbNtcProcessRWLock();
            virtual bool ManualFree();
            virtual TMdbNtcStringBuffer ToString() const;
            virtual bool Lock(int iMilliSeconds=-1);
            /**
             * @brief 获取读锁--阻塞
             *
             * @param 无
             * @return bool
             * @retval true--成功，其他失败
             */
            bool RLock(int iMilliSeconds=-1);
            /**
             * @brief 获取读锁--非阻塞
             *
             * @param 无
             * @return bool
             * @retval true--成功，其他失败
             */
            bool TryRLock();
            /**
             * @brief 获取写锁--阻塞
             *
             * @param 无
             * @return bool
             * @retval true--成功，其他失败
             */
            bool WLock(int iMilliSeconds=-1);
            /**
             * @brief 获取写锁--非阻塞
             *
             * @param 无
             * @return bool
             * @retval true--成功，其他失败
             */
            bool TryWLock();
            /**
             * @brief 解除锁定读写锁
             *
             * @param 无
             * @return bool
             * @retval true--成功，其他失败
             */
            bool Unlock();
            /**
             * @brief 检测某名称的读写锁是否存在
             *
             * @param pszPoolName [in] 读写锁池的名称
             * @param pszName [in] 读写锁的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 存在
             */
            static bool CheckExist(const char* pszPoolName, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 销毁读写锁
             *
             * @param pszPoolName [in] 读写锁池的名称
             * @param pszName [in] 读写锁的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 成功
             */
            static bool Destroy(const char* pszPoolName, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        protected:
            /**
             * @brief 构造函数
             *
             * @param pLockInfo [in] 锁的信息
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @param pParentNameObject [in] 所隶属的命名对象
             */
            TMdbNtcProcessRWLock(TLockInfo *pLockInfo, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);
        protected:
#ifdef OS_WINDOWS
            HANDLE m_hLockHandle;
#endif
        };

        //进程锁
        //==========================================================================
        class TMdbNtcProcessRWLockPool : public TMdbNtcProcessLockPool
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessRWLockPool);
        public:
            /**
             * @brief 构造函数
             *
             * @param pszPoolName [in] 读写锁池的名称
             * @param uiLockCount [in] 读写锁的数目，如果为0，则表示附接到ipc实例的读写锁池
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @param bCheckVersion [in] 是否检查共享内存的版本，以免不同版本的共享内存结构差异导致访问异常
             */
            TMdbNtcProcessRWLockPool(const char* pszPoolName, MDB_UINT32 uiLockCount = 0, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, bool bCheckVersion = true);
        protected:
            TMdbNtcProcessLockPool::TPoolLock* GetNewLock(TMdbNtcProcessLockPool::TLockInfo* pLockInfo, TMdbNtcSystemNameObject* pParentNameObject);
        public://提供一些静态方法
            /**
             * @brief 销毁读写锁的池
             *
             * @param pszPoolName [in] 读写锁池的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 成功
             */
            static bool DestroyPool(const char* pszPoolName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        };

       /**
        * @brief 线程锁操作类
        *
        * 实现线程间各种操作
        * windows下使用临界区，linux/unix下使用线程条件变量
        */
        class TMdbNtcThreadLock:public TMdbNtcBaseLock, public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadLock);
        public:
            TMdbNtcThreadLock();
            ~TMdbNtcThreadLock();
            /**
             * @brief 加锁
             *
             * @return bool
             * @retval true 成功
             */
            virtual bool Lock(int iMilliSecond = -1);
            /**
             * @brief 尝试加锁
             *
             * @return bool
             * @retval true 成功
             */
            bool TryLock();
            /**
             * @brief 解锁
             *
             * @return bool
             * @retval true 成功
             */
            virtual bool Unlock();
        #ifdef OS_WINDOWS
            inline CRITICAL_SECTION& GetMutex()
            {
                return m_hMutex;
            }
        #else
            inline pthread_mutex_t& GetMutex()
            {
                return m_hMutex;
            }
        #endif
        protected:
        #ifdef OS_WINDOWS
            CRITICAL_SECTION m_hMutex;///< 线程互斥对象
        #else
            pthread_mutex_t  m_hMutex;///< 线程互斥对象
        #endif
        };

        /**
         * 读写锁类
         *
         */
        class TMdbNtcThreadRWLock: public TMdbNtcBaseLock,public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadRWLock);
        public:
           /**
            * @brief 构造函数
            *
            * @param 无
            * @return 无
            */
            TMdbNtcThreadRWLock();
            /**
             * @brief 析构函数
             *
             * @param 无
             * @return 无
             */
            ~TMdbNtcThreadRWLock();
            virtual bool Lock(int iMilliSeconds=-1);
            /**
             * @brief 获取读锁--阻塞
             *
             * @param 无
             * @return bool
             * @retval true--成功，其他失败
             */
            bool RLock(int iMilliSeconds=-1);
            /**
             * @brief 获取读锁--非阻塞
             *
             * @param 无
             * @return bool
             * @retval true--成功，其他失败
             */
            bool TryRLock();
            /**
             * @brief 获取写锁--阻塞
             *
             * @param 无
             * @return bool
             * @retval true--成功，其他失败
             */
            bool WLock(int iMilliSeconds=-1);
            /**
             * @brief 获取写锁--非阻塞
             *
             * @param 无
             * @return bool
             * @retval true--成功，其他失败
             */
            bool TryWLock();
            /**
             * @brief 解除锁定读写锁
             *
             * @param 无
             * @return bool
             * @retval true--成功，其他失败
             */
            bool Unlock();
        protected:
        #ifdef OS_WINDOWS
            CRITICAL_SECTION m_hRWLock;
        #else
            pthread_rwlock_t  m_hRWLock;
        #endif

        };

        /*
        * @brief 事件基类
        *
        * 通过派生类实现进程间、线程间的各种事件操作。
        */
        class TMdbNtcBaseEvent
        {
        public:
            virtual ~TMdbNtcBaseEvent()
            {
            }
            virtual bool Wait(int iMilliSeconds)=0;
            virtual bool SetEvent()=0;
            virtual bool PulseEvent()=0;
        };

        /**
         * @brief 事件辅助类
         * 进程级事件
         */
        class TMdbNtcProcessEvent:public TMdbNtcBaseEvent, public TMdbNtcSystemNameObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessEvent);
        public:
            /**
             * @brief 构造函数
             *
             * @param pszEventName [in] 事件名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @param pParentNameObject [in] 所隶属的命名对象
             * @return 无
             */
            TMdbNtcProcessEvent(const char* pszEventName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);
            ~TMdbNtcProcessEvent();
            /**
             * @brief 获得事件句柄
             *
             * @return MDB_IPC_HANDLE
             * @retval 通知句柄
             */
            MDB_IPC_HANDLE GetHandle()
            {
                return m_hHandle;
            }
            /**
             * @brief 等待信号产生。
             *
             * @param iMilliSeconds [in] 等待超时时间单位毫秒
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            virtual bool Wait(int iMilliSeconds = -1);
            /**
             * @brief 直到一个等待的线程被唤醒，事件才复位
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            virtual bool SetEvent();
            /**
             * @brief 唤醒所有等待的线程,事件立即复位。
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            virtual bool PulseEvent();
            /**
             * @brief 销毁事件，相对于Create创建的事件。
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            virtual bool ManualFree();
        public://提供一些静态方法
            /**
             * @brief 检测事件是否存在
             *
             * @param pszName [in] 事件的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 存在
             */
            static bool CheckExist(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 销毁事件
             *
             * @param pszName [in] 事件的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 成功
             */
            static bool Destroy(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 获取正在等待资源的个数
             *
             * @param pszName [in] 事件的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return int
             * @retval -1   失败
             * @retval 其他 成功
             */
            static int GetNCnt(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 获取事件的当前值
             *
             * @param pszName [in] 事件的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return int
             * @retval -1   失败
             * @retval 其他 成功
             */
            static int GetVal (const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        protected:
            MDB_IPC_HANDLE m_hHandle;///< 事件句柄
        };

        /**
         * @brief 线程级事件
         *
         * win32下使用匿名事件，unix/linux下使用线程条件变量
         */
        class TMdbNtcThreadEvent:public TMdbNtcBaseEvent, public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadEvent);
        public:
            /**
             * @brief 构造函数
             *
             */
            TMdbNtcThreadEvent();
            ~TMdbNtcThreadEvent();
            /**
             * @brief 获得等待的线程数目
             *
             * @return MDB_UINT32
             * @retval 等待的线程数目
             */
            inline MDB_UINT32 GetWaitCount()
            {
                return m_uiWaitCount;
            }
            /**
             * @brief 等待事件触发。
             *
             * @param iMilliSeconds [in] 等待超时时间单位毫秒
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            virtual bool Wait(int iMilliSeconds = -1);
            /**
             * @brief 直到一个等待的线程被唤醒，事件才复位
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            virtual bool SetEvent();
            /**
             * @brief 唤醒所有等待的线程,事件立即复位。
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            virtual bool PulseEvent();
        protected:
        #ifdef OS_WINDOWS
            MDB_IPC_HANDLE    m_hHandle;///< 事件句柄
        #else
            pthread_cond_t  m_oCondition;///< 条件变量
            bool            m_bSignaled;///< 有信号状态
        #endif
            TMdbNtcThreadLock     m_oMutex;///< 互斥锁
            MDB_UINT32          m_uiWaitCount;///< 等待此事件的线程数目
        };


        /**
         * @brief 条件变量
         *
         * win32下使用匿名通知，unix/linux下使用线程条件变量
         */
        class TMdbNtcThreadCond:public TMdbNtcBaseEvent,public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadCond);
        public:
            /**
             * @brief 构造函数
             *
             */
            TMdbNtcThreadCond(TMdbNtcThreadLock  *pMutexLock);
            ~TMdbNtcThreadCond();
        public:
            /**
             * @brief 获得等待的线程数目
             *
             * @return MDB_UINT32
             * @retval 等待的线程数目
             */
            inline MDB_UINT32 GetWaitCount()
            {
                return m_uiWaitCount;
            }
           /**
            * @brief 加锁
            *
            * 加锁操作
            *
            * @parame void
            * @return bool
            * @retval true  成功
            * @retval false 失败
            */
            bool Lock();
            /**
            * @brief 解锁
            *
            * 解锁操作
            *
            * @return bool
            * @retval true  成功
            * @retval false 失败
            */
            bool Unlock();
            /**
             * @brief 等待通知触发。
             *
             * @param iMilliSeconds [in] 等待超时时间单位毫秒
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            bool Wait(int iMilliSeconds = -1);
            /**
             * @brief 发起通知,唤醒一个等待线程
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            bool SetEvent();
            /**
             * @brief 唤醒所有等待的线程,事件立即复位。
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            bool PulseEvent();
            /**
             * @brief 发起广播,唤醒所有等待线程
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            bool BroadCast();
            /**
             * @brief 设置锁
             *
             * @param pMutexLock [in] 互斥锁指针
             * @return void
             */
            void SetLock(TMdbNtcThreadLock  *pMutexLock);
        protected:
        #ifdef OS_WINDOWS
            MDB_IPC_HANDLE      m_hHandle;///< 通知句柄
        #else
            pthread_cond_t  m_oCondition;///< 条件变量
        #endif
            TMdbNtcThreadLock     *m_pMutex; ///< 互斥锁,外部传入
            //TMdbNtcThreadLock     m_oMutex;  ///< 互斥锁,用来多线程访问m_uiWaitCount
            MDB_UINT32      m_uiWaitCount; ///< 等待此条件变量的线程数目
        };


        /*
        * @brief 通知基类
        *
        * 通过派生类实现进程间、线程间的各种通知操作。
        */
        class TMdbNtcBaseNotify
        {
        public:
            virtual ~TMdbNtcBaseNotify()
            {
            }
            virtual bool Wait(int iMilliSeconds)=0;
            virtual bool Notify()=0;
            virtual bool Reset()=0;
        };

        /**
         * @brief 通知辅助类
         * 进程级通知
         */
        class TMdbNtcProcessNotify:public TMdbNtcBaseNotify, public TMdbNtcSystemNameObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessNotify);
        public:
            /**
             * @brief 打开或创建一个通知
             *
             * @param pszNotifyName [in] 通知的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @param pParentNameObject [in] 所隶属的命名对象
             */
            TMdbNtcProcessNotify(const char* pszNotifyName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);
            ~TMdbNtcProcessNotify();
            /**
             * @brief 获得通知句柄
             *
             * @return MDB_IPC_HANDLE
             * @retval 通知句柄
             */
            MDB_IPC_HANDLE GetHandle()
            {
                return m_hHandle;
            }
            /**
             * @brief 等待信号产生。
             *
             * @param iMilliSeconds [in] 等待超时时间单位毫秒
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            bool Wait(int iMilliSeconds = -1);
            /**
             * @brief 发起通知，信号持续到手工调用Reset
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            bool Notify();
            /**
             * @brief 重置通知为无信号状态
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            bool Reset();
            /**
             * @brief 销毁通知，相对于Create创建的通知。
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            virtual bool ManualFree();
        public://提供一些静态方法
            /**
             * @brief 检测通知是否存在
             *
             * @param pszName [in] 通知的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 存在
             */
            static bool CheckExist(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 销毁通知
             *
             * @param pszName [in] 通知的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 成功
             */
            static bool Destroy(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 获取正在等待资源的个数
             *
             * @param pszName [in] 事件的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return int
             * @retval -1   失败
             * @retval 其他 成功
             */
            static int GetNCnt(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 获取事件的当前值
             *
             * @param pszName [in] 事件的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return int
             * @retval -1   失败
             * @retval 其他 成功
             */
            static int GetVal (const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        protected:
            MDB_IPC_HANDLE m_hHandle;///< 通知句柄
        };

        /**
         * @brief 线程级通知
         *
         * win32下使用匿名通知，unix/linux下使用线程条件变量
         */
        class TMdbNtcThreadNotify:public TMdbNtcBaseNotify, public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadNotify);
        public:
            /**
             * @brief 构造函数
             *
             */
            TMdbNtcThreadNotify();
            ~TMdbNtcThreadNotify();
            /**
             * @brief 获得等待的线程数目
             *
             * @return MDB_UINT32
             * @retval 等待的线程数目
             */
            inline MDB_UINT32 GetWaitCount()
            {
                return m_uiWaitCount;
            }
            /**
             * @brief 等待通知触发。
             *
             * @param iMilliSeconds [in] 等待超时时间单位毫秒
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            bool Wait(int iMilliSeconds = -1);
            /**
             * @brief 发起通知，信号持续到手工调用Reset
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            bool Notify();
            /**
             * @brief 重置通知为无信号状态
             *
             * @return bool
             * @retval true  成功
             * @retval false 失败
             */
            bool Reset();
        protected:
        #ifdef OS_WINDOWS
            MDB_IPC_HANDLE    m_hHandle;///< 通知句柄
        #else
            pthread_cond_t  m_oCondition;///< 条件变量
            bool            m_bSignaled;///< 有信号状态
        #endif
            TMdbNtcThreadLock     m_oMutex;///< 互斥锁
            MDB_UINT32          m_uiWaitCount;///< 等待此通知的线程数目
        };

        //自动锁
        //==========================================================================
        class TMdbNtcAutoLock : public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcAutoLock);
        private:
            TMdbNtcBaseLock *m_pLock;
        public:
            TMdbNtcAutoLock(TMdbNtcBaseLock *pLock);
            virtual ~TMdbNtcAutoLock();
        };

        /**
         * @brief 自旋锁
         *
         */
        class TMdbNtcThreadSpinLock:public TMdbNtcBaseLock, public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadSpinLock);
        public:
            TMdbNtcThreadSpinLock();
            ~TMdbNtcThreadSpinLock();
            /**
             * @brief 加锁
             *
             * @return bool
             * @retval true 成功
             */
            bool Lock(int iMilliSeconds=-1);
            /**
             * @brief 尝试加锁
             *
             * @return bool
             * @retval true 成功
             */
            bool TryLock();
            /**
             * @brief 解锁
             *
             * @return bool
             * @retval true 成功
             */
            bool Unlock();
        protected:
        #ifdef OS_WINDOWS
            CRITICAL_SECTION  m_spin;
        #elif defined(OS_SUN)
            pthread_mutex_t   m_spin;
        #elif defined(OS_HP)
            pthread_mutex_t   m_spin;
        #else
            pthread_spinlock_t m_spin;
        #endif
        };
//        typedef TMdbNtcThreadSpinLock TSpinLock;
//    }
//}
#endif
