/**
 * @file Sdk/mdbSysLocks.h
 * @brief ������صķ�װ
 *
 * ������صķ�װ
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
        * @brief ������
        *
        * ͨ��������ʵ�ֽ��̼䡢�̼߳�ĸ�����������
        */
        class TMdbNtcBaseLock
        {
        public:
            virtual ~TMdbNtcBaseLock()
            {
            }
            virtual bool Lock(int iMilliSeconds=-1) = 0;
            /**
             * @brief ���Լ���
             *
             * @return bool
             * @retval true �ɹ�
             */
            virtual bool TryLock()
            {
                return Lock(0);
            }
            virtual bool Unlock() = 0;
        };
        /*
        * @brief ������������
        *
        * ʵ�ֽ��̼���ֲ���
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
            * @brief ���캯��
            *
            * ��Ĺ��캯��
            *
            * @param pszSemName [in] �ź�������
            * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
            * @param pParentNameObject [in] ����������������
            */
            TMdbNtcProcessLock( const char *pszSemName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);

            /**
            * @brief ��������
            *
            * �����������
            */
            virtual ~TMdbNtcProcessLock( );

            bool IsOK() const;

           /**
            * @brief ����
            *
            * ��������
            *
            * @parame lMilliSecond [in] ��ʱ(��λ������)
            * @return bool
            * @retval true  �ɹ�
            * @retval false ʧ��
            */
            bool Lock(int iMilliSecond = -1);
            /**
            * @brief ����
            *
            * ��������
            *
            * @return bool
            * @retval true  �ɹ�
            * @retval false ʧ��
            */
            bool Unlock();

            /**
            * @brief ��������
            *
            * �����ź���
            *
            * @return bool
            * @retval true  �ɹ�
            * @retval false ʧ��
            */
            virtual bool ManualFree();

            /**
            * @brief ������ݳ�Ա
            *
            * ������ݳ�Ա
            *
            * @return TMdbNtcStringBuffer
            */
            TMdbNtcStringBuffer ToString( );
        public://�ṩһЩ��̬����
            /**
             * @brief ��⻥�����Ƿ����
             *
             * @param pszName [in] ������������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true ����
             */
            static bool CheckExist(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ���ٻ�����
             *
             * @param pszName [in] ������������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true �ɹ�
             */
            static bool Destroy(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        private:
            MDB_IPC_HANDLE m_vHandle;
        };

        /**
         * @brief ���������
         *
         */
        class TMdbNtcProcessLockPool:public TMdbNtcSystemNameObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessLockPool);
            /**
             * @brief ���Ļ�����Ϣ
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
             * @brief ��������������
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
                 * @brief ���캯��
                 *
                 * @param pLockInfo [in] ������Ϣ
                 * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
                 * @param pParentNameObject [in] ����������������
                 */
                TPoolLock(TLockInfo *pLockInfo, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject* pParentNameObject = NULL);
            protected:
                TLockInfo *m_pLockInfo;///< ������Ϣ
            };
        public:
            virtual TMdbNtcStringBuffer ToString() const;
            /**
             * @brief ���һ����
             *
             * @param pszName [in] ��������
             * @param pParentNameObject [in] �������ĸ�����������Ĭ��ΪNULL
             * @return TMdbNtcBaseLock*
             * @retval ��NULL �ɹ������������delete
             */
            TMdbNtcBaseLock* GetLock(const char *pszName, TMdbNtcSystemNameObject* pParentNameObject = NULL);
            /**
             * @brief �����������һ����
             *
             * @param uiIndex [in] ���ڵ����к�
             * @param pParentNameObject [in] �������ĸ�����������Ĭ��ΪNULL
             * @return TMdbNtcBaseLock*
             * @retval ��NULL �ɹ������������delete
             */
            TMdbNtcBaseLock* GetLock(MDB_UINT32 uiIndex, TMdbNtcSystemNameObject* pParentNameObject = NULL);
            /**
             * @brief ��������
             *
             * @return bool
             * @retval true �ɹ�
             */
            bool ManualFree();
            /**
             * @brief ��óص�������
             *
             * @return MDB_UINT32
             */
            inline MDB_UINT32 GetLockCount() const
            {
                return m_pPoolBuffer?(*(MDB_UINT32*)m_pPoolBuffer->GetBuffer()):0;
            }
            /**
             * @brief ���ĳ���Ƶ����Ƿ����
             *
             * @param pszName [in] ��������
             * @return bool
             * @retval true ����
             */
            inline bool CheckExist(const char* pszName)
            {
                return FindIndex(pszName)!=-1;
            }
            /**
             * @brief ������
             *
             * @param pszName [in] ��������
             * @return bool
             * @retval true �ɹ�
             */
            inline bool Destroy(const char* pszName)
            {
                int iIndex = FindIndex(pszName);
                if(iIndex == -1) return true;
                else return Destroy((MDB_UINT32)iIndex);
            }
            /**
             * @brief ����ָ��λ�õ���
             *
             * @param uiIndex [in] ���ڵ����к�
             * @return bool
             * @retval true �ɹ�
             */
            virtual bool Destroy(MDB_UINT32 uiIndex);
        protected:
            /**
             * @brief ���캯��
             *
             * @param uiLockSize [in] ÿ����ռ�õĴ�С
             * @param pszPoolName [in] ���ص�����
             * @param uiLockCount [in] ������Ŀ�����Ϊ0�����ʾ���ӵ�ipcʵ��������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @param bCheckVersion [in] �Ƿ��鹲���ڴ�İ汾�����ⲻͬ�汾�Ĺ����ڴ�ṹ���쵼�·����쳣
             */
            TMdbNtcProcessLockPool(MDB_UINT32 uiLockSize, const char* pszPoolName, MDB_UINT32 uiLockCount = 0, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, bool bCheckVersion = true);
            virtual ~TMdbNtcProcessLockPool();
            /**
             * @brief ���������ƻ�����ڵ�����
             *
             * @param pszName [in] ��������
             * @return int
             * @retval -1 û���ҵ�
             */
            int FindIndex(const char* pszName);
            /**
             * @brief ����µ�������
             *
             * @param pLockInfo         [in] ����Ϣ
             * @param pParentNameObject [in] ����������������
             * @return TMdbNtcProcessLock*
             */
            virtual TPoolLock* GetNewLock(TLockInfo* pLockInfo, TMdbNtcSystemNameObject* pParentNameObject) = 0;
            /**
             * @brief ���ָ��λ�õ�����Ϣ
             *
             * @param uiIndex [in] �������к�
             * @return TLockInfo*
             */
            inline TLockInfo* GetLockInfo(MDB_UINT32 uiIndex)
            {
                return m_pPoolBuffer==NULL?NULL:reinterpret_cast<TLockInfo*>(m_pPoolBuffer->GetBuffer()+sizeof(MDB_UINT32)+uiIndex*m_uiLockSize);
            }
        protected:
            TMdbNtcProcessLock*       m_pSemaphore;
            TMdbNtcShareMem*          m_pPoolBuffer;
            TPoolLock**         m_ppLocks;///< �������е���
            MDB_UINT32              m_uiLockSize;///< ÿ����ռ�õĴ�С
        };

        /**
         * @brief ��������
         *
         */
        class TMdbNtcProcessPerfLock : public TMdbNtcProcessLockPool::TPoolLock
        {
            friend class TMdbNtcProcessPerfLockPool;//ֻ���ɴ���Ԫ�ഴ�������
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessPerfLock);
        protected:
            /**
             * @brief ���Ļ�����Ϣ
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
             * @brief ���ĳ���Ƶĸ��������Ƿ����
             *
             * @param pszPoolName [in] ���������ص�����
             * @param pszName [in] ��������������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true ����
             */
            static bool CheckExist(const char* pszPoolName, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ���ٸ�������
             *
             * @param pszPoolName [in] ���������ص�����
             * @param pszName [in] ��������������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true �ɹ�
             */
            static bool Destroy(const char* pszPoolName, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        protected:
            /**
             * @brief ���캯��
             *
             * @param pLockInfo [in] ������Ϣ
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @param pParentNameObject [in] ����������������
             */
            TMdbNtcProcessPerfLock(TLockInfo *pLockInfo, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);
        protected:
#ifdef OS_WINDOWS
            HANDLE m_hLockHandle;
#endif
        };

        /**
         * @brief ����̵ĸ���������
         *
         */
        class TMdbNtcProcessPerfLockPool : public TMdbNtcProcessLockPool
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessPerfLockPool);
        public:
            /**
             * @brief ���캯��
             *
             * @param pszPoolName [in] ���������ص�����
             * @param uiLockCount [in] ������������Ŀ�����Ϊ0�����ʾ���ӵ�ipcʵ���ĸ���������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @param bCheckVersion [in] �Ƿ��鹲���ڴ�İ汾�����ⲻͬ�汾�Ĺ����ڴ�ṹ���쵼�·����쳣
             */
            TMdbNtcProcessPerfLockPool(const char* pszPoolName, MDB_UINT32 uiLockCount = 0, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, bool bCheckVersion = true);
            /**
             * @brief ��ָ����ŵ������м���
             *
             * @param uiIndex       [in] �������
             * @param iMilliSeconds [in] �ȴ���ʱ��
             * @return bool
             * @retval true �ɹ�
             */
            bool Lock(MDB_UINT32 uiIndex, int iMilliSeconds=-1);
            /**
             * @brief ���Զ�ָ����ŵ������м���
             *
             * @param uiIndex       [in] �������
             * @return bool
             * @retval true �ɹ�
             */
            bool TryLock(MDB_UINT32 uiIndex);
            /**
             * @brief ��ָ����ŵ������н���
             *
             * @param uiIndex       [in] �������
             * @return bool
             * @retval true �ɹ�
             */
            bool Unlock(MDB_UINT32 uiIndex);
        protected:
            virtual TMdbNtcProcessLockPool::TPoolLock* GetNewLock(TLockInfo* pLockInfo, TMdbNtcSystemNameObject* pParentNameObject);
        public://�ṩһЩ��̬����
            /**
             * @brief ���ٸ��������ĳ�
             *
             * @param pszPoolName [in] ���������ص�����
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true �ɹ�
             */
            static bool DestroyPool(const char* pszPoolName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        };

        /**
         * @brief ����̶�д��
         *
         */
        class TMdbNtcProcessRWLock : public TMdbNtcProcessLockPool::TPoolLock
        {
            friend class TMdbNtcProcessRWLockPool;//ֻ���ɴ���Ԫ�ഴ�������
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessRWLock);
        protected:
            /**
             * @brief ���Ļ�����Ϣ
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
             * @brief ��ȡ����--����
             *
             * @param ��
             * @return bool
             * @retval true--�ɹ�������ʧ��
             */
            bool RLock(int iMilliSeconds=-1);
            /**
             * @brief ��ȡ����--������
             *
             * @param ��
             * @return bool
             * @retval true--�ɹ�������ʧ��
             */
            bool TryRLock();
            /**
             * @brief ��ȡд��--����
             *
             * @param ��
             * @return bool
             * @retval true--�ɹ�������ʧ��
             */
            bool WLock(int iMilliSeconds=-1);
            /**
             * @brief ��ȡд��--������
             *
             * @param ��
             * @return bool
             * @retval true--�ɹ�������ʧ��
             */
            bool TryWLock();
            /**
             * @brief ���������д��
             *
             * @param ��
             * @return bool
             * @retval true--�ɹ�������ʧ��
             */
            bool Unlock();
            /**
             * @brief ���ĳ���ƵĶ�д���Ƿ����
             *
             * @param pszPoolName [in] ��д���ص�����
             * @param pszName [in] ��д��������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true ����
             */
            static bool CheckExist(const char* pszPoolName, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ���ٶ�д��
             *
             * @param pszPoolName [in] ��д���ص�����
             * @param pszName [in] ��д��������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true �ɹ�
             */
            static bool Destroy(const char* pszPoolName, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        protected:
            /**
             * @brief ���캯��
             *
             * @param pLockInfo [in] ������Ϣ
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @param pParentNameObject [in] ����������������
             */
            TMdbNtcProcessRWLock(TLockInfo *pLockInfo, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);
        protected:
#ifdef OS_WINDOWS
            HANDLE m_hLockHandle;
#endif
        };

        //������
        //==========================================================================
        class TMdbNtcProcessRWLockPool : public TMdbNtcProcessLockPool
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessRWLockPool);
        public:
            /**
             * @brief ���캯��
             *
             * @param pszPoolName [in] ��д���ص�����
             * @param uiLockCount [in] ��д������Ŀ�����Ϊ0�����ʾ���ӵ�ipcʵ���Ķ�д����
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @param bCheckVersion [in] �Ƿ��鹲���ڴ�İ汾�����ⲻͬ�汾�Ĺ����ڴ�ṹ���쵼�·����쳣
             */
            TMdbNtcProcessRWLockPool(const char* pszPoolName, MDB_UINT32 uiLockCount = 0, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, bool bCheckVersion = true);
        protected:
            TMdbNtcProcessLockPool::TPoolLock* GetNewLock(TMdbNtcProcessLockPool::TLockInfo* pLockInfo, TMdbNtcSystemNameObject* pParentNameObject);
        public://�ṩһЩ��̬����
            /**
             * @brief ���ٶ�д���ĳ�
             *
             * @param pszPoolName [in] ��д���ص�����
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true �ɹ�
             */
            static bool DestroyPool(const char* pszPoolName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        };

       /**
        * @brief �߳���������
        *
        * ʵ���̼߳���ֲ���
        * windows��ʹ���ٽ�����linux/unix��ʹ���߳���������
        */
        class TMdbNtcThreadLock:public TMdbNtcBaseLock, public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadLock);
        public:
            TMdbNtcThreadLock();
            ~TMdbNtcThreadLock();
            /**
             * @brief ����
             *
             * @return bool
             * @retval true �ɹ�
             */
            virtual bool Lock(int iMilliSecond = -1);
            /**
             * @brief ���Լ���
             *
             * @return bool
             * @retval true �ɹ�
             */
            bool TryLock();
            /**
             * @brief ����
             *
             * @return bool
             * @retval true �ɹ�
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
            CRITICAL_SECTION m_hMutex;///< �̻߳������
        #else
            pthread_mutex_t  m_hMutex;///< �̻߳������
        #endif
        };

        /**
         * ��д����
         *
         */
        class TMdbNtcThreadRWLock: public TMdbNtcBaseLock,public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadRWLock);
        public:
           /**
            * @brief ���캯��
            *
            * @param ��
            * @return ��
            */
            TMdbNtcThreadRWLock();
            /**
             * @brief ��������
             *
             * @param ��
             * @return ��
             */
            ~TMdbNtcThreadRWLock();
            virtual bool Lock(int iMilliSeconds=-1);
            /**
             * @brief ��ȡ����--����
             *
             * @param ��
             * @return bool
             * @retval true--�ɹ�������ʧ��
             */
            bool RLock(int iMilliSeconds=-1);
            /**
             * @brief ��ȡ����--������
             *
             * @param ��
             * @return bool
             * @retval true--�ɹ�������ʧ��
             */
            bool TryRLock();
            /**
             * @brief ��ȡд��--����
             *
             * @param ��
             * @return bool
             * @retval true--�ɹ�������ʧ��
             */
            bool WLock(int iMilliSeconds=-1);
            /**
             * @brief ��ȡд��--������
             *
             * @param ��
             * @return bool
             * @retval true--�ɹ�������ʧ��
             */
            bool TryWLock();
            /**
             * @brief ���������д��
             *
             * @param ��
             * @return bool
             * @retval true--�ɹ�������ʧ��
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
        * @brief �¼�����
        *
        * ͨ��������ʵ�ֽ��̼䡢�̼߳�ĸ����¼�������
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
         * @brief �¼�������
         * ���̼��¼�
         */
        class TMdbNtcProcessEvent:public TMdbNtcBaseEvent, public TMdbNtcSystemNameObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessEvent);
        public:
            /**
             * @brief ���캯��
             *
             * @param pszEventName [in] �¼�����
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @param pParentNameObject [in] ����������������
             * @return ��
             */
            TMdbNtcProcessEvent(const char* pszEventName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);
            ~TMdbNtcProcessEvent();
            /**
             * @brief ����¼����
             *
             * @return MDB_IPC_HANDLE
             * @retval ֪ͨ���
             */
            MDB_IPC_HANDLE GetHandle()
            {
                return m_hHandle;
            }
            /**
             * @brief �ȴ��źŲ�����
             *
             * @param iMilliSeconds [in] �ȴ���ʱʱ�䵥λ����
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            virtual bool Wait(int iMilliSeconds = -1);
            /**
             * @brief ֱ��һ���ȴ����̱߳����ѣ��¼��Ÿ�λ
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            virtual bool SetEvent();
            /**
             * @brief �������еȴ����߳�,�¼�������λ��
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            virtual bool PulseEvent();
            /**
             * @brief �����¼��������Create�������¼���
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            virtual bool ManualFree();
        public://�ṩһЩ��̬����
            /**
             * @brief ����¼��Ƿ����
             *
             * @param pszName [in] �¼�������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true ����
             */
            static bool CheckExist(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief �����¼�
             *
             * @param pszName [in] �¼�������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true �ɹ�
             */
            static bool Destroy(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ��ȡ���ڵȴ���Դ�ĸ���
             *
             * @param pszName [in] �¼�������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return int
             * @retval -1   ʧ��
             * @retval ���� �ɹ�
             */
            static int GetNCnt(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ��ȡ�¼��ĵ�ǰֵ
             *
             * @param pszName [in] �¼�������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return int
             * @retval -1   ʧ��
             * @retval ���� �ɹ�
             */
            static int GetVal (const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        protected:
            MDB_IPC_HANDLE m_hHandle;///< �¼����
        };

        /**
         * @brief �̼߳��¼�
         *
         * win32��ʹ�������¼���unix/linux��ʹ���߳���������
         */
        class TMdbNtcThreadEvent:public TMdbNtcBaseEvent, public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadEvent);
        public:
            /**
             * @brief ���캯��
             *
             */
            TMdbNtcThreadEvent();
            ~TMdbNtcThreadEvent();
            /**
             * @brief ��õȴ����߳���Ŀ
             *
             * @return MDB_UINT32
             * @retval �ȴ����߳���Ŀ
             */
            inline MDB_UINT32 GetWaitCount()
            {
                return m_uiWaitCount;
            }
            /**
             * @brief �ȴ��¼�������
             *
             * @param iMilliSeconds [in] �ȴ���ʱʱ�䵥λ����
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            virtual bool Wait(int iMilliSeconds = -1);
            /**
             * @brief ֱ��һ���ȴ����̱߳����ѣ��¼��Ÿ�λ
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            virtual bool SetEvent();
            /**
             * @brief �������еȴ����߳�,�¼�������λ��
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            virtual bool PulseEvent();
        protected:
        #ifdef OS_WINDOWS
            MDB_IPC_HANDLE    m_hHandle;///< �¼����
        #else
            pthread_cond_t  m_oCondition;///< ��������
            bool            m_bSignaled;///< ���ź�״̬
        #endif
            TMdbNtcThreadLock     m_oMutex;///< ������
            MDB_UINT32          m_uiWaitCount;///< �ȴ����¼����߳���Ŀ
        };


        /**
         * @brief ��������
         *
         * win32��ʹ������֪ͨ��unix/linux��ʹ���߳���������
         */
        class TMdbNtcThreadCond:public TMdbNtcBaseEvent,public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadCond);
        public:
            /**
             * @brief ���캯��
             *
             */
            TMdbNtcThreadCond(TMdbNtcThreadLock  *pMutexLock);
            ~TMdbNtcThreadCond();
        public:
            /**
             * @brief ��õȴ����߳���Ŀ
             *
             * @return MDB_UINT32
             * @retval �ȴ����߳���Ŀ
             */
            inline MDB_UINT32 GetWaitCount()
            {
                return m_uiWaitCount;
            }
           /**
            * @brief ����
            *
            * ��������
            *
            * @parame void
            * @return bool
            * @retval true  �ɹ�
            * @retval false ʧ��
            */
            bool Lock();
            /**
            * @brief ����
            *
            * ��������
            *
            * @return bool
            * @retval true  �ɹ�
            * @retval false ʧ��
            */
            bool Unlock();
            /**
             * @brief �ȴ�֪ͨ������
             *
             * @param iMilliSeconds [in] �ȴ���ʱʱ�䵥λ����
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            bool Wait(int iMilliSeconds = -1);
            /**
             * @brief ����֪ͨ,����һ���ȴ��߳�
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            bool SetEvent();
            /**
             * @brief �������еȴ����߳�,�¼�������λ��
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            bool PulseEvent();
            /**
             * @brief ����㲥,�������еȴ��߳�
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            bool BroadCast();
            /**
             * @brief ������
             *
             * @param pMutexLock [in] ������ָ��
             * @return void
             */
            void SetLock(TMdbNtcThreadLock  *pMutexLock);
        protected:
        #ifdef OS_WINDOWS
            MDB_IPC_HANDLE      m_hHandle;///< ֪ͨ���
        #else
            pthread_cond_t  m_oCondition;///< ��������
        #endif
            TMdbNtcThreadLock     *m_pMutex; ///< ������,�ⲿ����
            //TMdbNtcThreadLock     m_oMutex;  ///< ������,�������̷߳���m_uiWaitCount
            MDB_UINT32      m_uiWaitCount; ///< �ȴ��������������߳���Ŀ
        };


        /*
        * @brief ֪ͨ����
        *
        * ͨ��������ʵ�ֽ��̼䡢�̼߳�ĸ���֪ͨ������
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
         * @brief ֪ͨ������
         * ���̼�֪ͨ
         */
        class TMdbNtcProcessNotify:public TMdbNtcBaseNotify, public TMdbNtcSystemNameObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcProcessNotify);
        public:
            /**
             * @brief �򿪻򴴽�һ��֪ͨ
             *
             * @param pszNotifyName [in] ֪ͨ������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @param pParentNameObject [in] ����������������
             */
            TMdbNtcProcessNotify(const char* pszNotifyName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);
            ~TMdbNtcProcessNotify();
            /**
             * @brief ���֪ͨ���
             *
             * @return MDB_IPC_HANDLE
             * @retval ֪ͨ���
             */
            MDB_IPC_HANDLE GetHandle()
            {
                return m_hHandle;
            }
            /**
             * @brief �ȴ��źŲ�����
             *
             * @param iMilliSeconds [in] �ȴ���ʱʱ�䵥λ����
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            bool Wait(int iMilliSeconds = -1);
            /**
             * @brief ����֪ͨ���źų������ֹ�����Reset
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            bool Notify();
            /**
             * @brief ����֪ͨΪ���ź�״̬
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            bool Reset();
            /**
             * @brief ����֪ͨ�������Create������֪ͨ��
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            virtual bool ManualFree();
        public://�ṩһЩ��̬����
            /**
             * @brief ���֪ͨ�Ƿ����
             *
             * @param pszName [in] ֪ͨ������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true ����
             */
            static bool CheckExist(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ����֪ͨ
             *
             * @param pszName [in] ֪ͨ������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true �ɹ�
             */
            static bool Destroy(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ��ȡ���ڵȴ���Դ�ĸ���
             *
             * @param pszName [in] �¼�������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return int
             * @retval -1   ʧ��
             * @retval ���� �ɹ�
             */
            static int GetNCnt(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ��ȡ�¼��ĵ�ǰֵ
             *
             * @param pszName [in] �¼�������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return int
             * @retval -1   ʧ��
             * @retval ���� �ɹ�
             */
            static int GetVal (const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        protected:
            MDB_IPC_HANDLE m_hHandle;///< ֪ͨ���
        };

        /**
         * @brief �̼߳�֪ͨ
         *
         * win32��ʹ������֪ͨ��unix/linux��ʹ���߳���������
         */
        class TMdbNtcThreadNotify:public TMdbNtcBaseNotify, public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadNotify);
        public:
            /**
             * @brief ���캯��
             *
             */
            TMdbNtcThreadNotify();
            ~TMdbNtcThreadNotify();
            /**
             * @brief ��õȴ����߳���Ŀ
             *
             * @return MDB_UINT32
             * @retval �ȴ����߳���Ŀ
             */
            inline MDB_UINT32 GetWaitCount()
            {
                return m_uiWaitCount;
            }
            /**
             * @brief �ȴ�֪ͨ������
             *
             * @param iMilliSeconds [in] �ȴ���ʱʱ�䵥λ����
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            bool Wait(int iMilliSeconds = -1);
            /**
             * @brief ����֪ͨ���źų������ֹ�����Reset
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            bool Notify();
            /**
             * @brief ����֪ͨΪ���ź�״̬
             *
             * @return bool
             * @retval true  �ɹ�
             * @retval false ʧ��
             */
            bool Reset();
        protected:
        #ifdef OS_WINDOWS
            MDB_IPC_HANDLE    m_hHandle;///< ֪ͨ���
        #else
            pthread_cond_t  m_oCondition;///< ��������
            bool            m_bSignaled;///< ���ź�״̬
        #endif
            TMdbNtcThreadLock     m_oMutex;///< ������
            MDB_UINT32          m_uiWaitCount;///< �ȴ���֪ͨ���߳���Ŀ
        };

        //�Զ���
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
         * @brief ������
         *
         */
        class TMdbNtcThreadSpinLock:public TMdbNtcBaseLock, public TMdbNtcComponentObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcThreadSpinLock);
        public:
            TMdbNtcThreadSpinLock();
            ~TMdbNtcThreadSpinLock();
            /**
             * @brief ����
             *
             * @return bool
             * @retval true �ɹ�
             */
            bool Lock(int iMilliSeconds=-1);
            /**
             * @brief ���Լ���
             *
             * @return bool
             * @retval true �ɹ�
             */
            bool TryLock();
            /**
             * @brief ����
             *
             * @return bool
             * @retval true �ɹ�
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
