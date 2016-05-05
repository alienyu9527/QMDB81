#include "Common/mdbSysLocks.h"
#include "Common/mdbSysUtils.h"
#include "Common/mdbStrUtils.h"
#include "Common/mdbSysShareMems.h"
#include "Common/mdbSysThreads.h"
#include "Common/mdbFileUtils.h"
#include "Common/mdbDateUtils.h"

//#include "Common/mdbLogInterface.h"
#ifndef OS_WINDOWS
    #include <sys/sem.h>
    #include <sys/types.h>
    #include <signal.h>
#else
#endif

//namespace QuickMDB
//{
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcProcessLock, TMdbNtcSystemNameObject);
        TMdbNtcProcessLock::TMdbNtcProcessLock(const char *pszSemName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */)
            :TMdbNtcSystemNameObject(pszSemName, pszInstEnvName, pParentNameObject)
        {
            m_bConstructResult = true;
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(this);
            if(sIpcPath.IsEmpty())
            {
                m_bConstructResult = false;
                TADD_WARNING( "GetIpcPath failed! name[%s]", pszSemName);
                return;
            }
#ifdef OS_WINDOWS
            //检测信号是否存在
            m_vHandle=OpenSemaphore(SEMAPHORE_ALL_ACCESS,false, sIpcPath.c_str());

            if(m_vHandle==NULL)
            {
                //不存在则生成
                m_vHandle=CreateSemaphore(NULL,1, 1, sIpcPath.c_str());
                if(m_vHandle==NULL)
                {
                    m_bConstructResult = false;
                    TADD_WARNING("TMdbNtcProcessLock::TMdbNtcProcessLock() : CreateSemaphore(Name=%s) ERRNO=[%u] ",
                        pszSemName, GetLastError());
                    return;
                }
                m_bSelfCreated = true;
            }
            else
            {
                m_bSelfCreated = false;
            }
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey == -1)
            {
                m_bConstructResult = false;
                TADD_WARNING("MakeIPCKey(%s) RETURN=[False],ErrorCode=%d\n", sIpcPath.c_str(), errno);
                return;
            }
            //检测信号是否存在
            m_vHandle=semget(iNameKey,1,0666);
            if(m_vHandle>=0)
            {
                m_bSelfCreated = false;
            }
            else
            {
                //不存在则生成
                m_vHandle=semget(iNameKey, 1, 0666 | IPC_CREAT | IPC_EXCL );
                if(m_vHandle<0)
                {
                    m_bConstructResult = false;
                    TADD_WARNING("TMdbNtcProcessLock::TMdbNtcProcessLock() : semget(key=%d) RETURN=[%d]", iNameKey,errno);
                    return;
                }

                //设置信号灯
                semun semopts;
                semopts.val=1;
                int iRet=semctl(m_vHandle,0,SETVAL,semopts);

                if(iRet<0)
                {
                    semctl(m_vHandle,0,IPC_RMID,0);
                    int iTemp = m_vHandle;
                    m_vHandle = -1;
                    m_bConstructResult = false;
                    TADD_WARNING("TMdbNtcProcessLock::TMdbNtcProcessLock() : semctl(key=%d id=%d) RETURN=[%d]", iNameKey,iTemp,errno);
                    return;
                }
                m_bSelfCreated = true;
            }
#endif
            if(m_bConstructResult)
            {
                TMdbNtcSysNameObjectHelper::SignIn(this);
            }
        }

        TMdbNtcProcessLock::~TMdbNtcProcessLock( )
        {
#ifdef  OS_WINDOWS
            if(m_vHandle!=NULL)
            {
                CloseHandle(m_vHandle);
            }
#endif
        }

        bool TMdbNtcProcessLock::IsOK() const
        {
#ifdef  OS_WINDOWS
            return m_vHandle!=NULL;
#else
            if(m_vHandle==-1) return false;


            semun semopts;
            semopts.val=0;
            return semctl(m_vHandle,0,GETVAL,semopts)>0;
#endif
        }

        bool TMdbNtcProcessLock::Lock( int iMilliSeconds )
        {
#ifdef  OS_WINDOWS
            if(m_vHandle==NULL) return false;
            return WAIT_OBJECT_0 == WaitForSingleObject(m_vHandle, iMilliSeconds);
#else
            if(m_vHandle==-1) return false;

            int iRet = -1;
            struct sembuf buf_p;
            buf_p.sem_num = 0;
            buf_p.sem_op = -1;
            buf_p.sem_flg = SEM_UNDO;
            if( iMilliSeconds < 0 )
            {
                __TRYAGAIN_TProcessLock:
                iRet=semop(m_vHandle, &buf_p, 1 ); //p操作,semop的第三个参数：sembuf结构变量的个数，通常为1
                if( 0 != iRet )
                {
                    if( errno == EINTR )
                    {
                         goto __TRYAGAIN_TProcessLock;
                    }
                }
            }
            else if(iMilliSeconds == 0)
            {
                buf_p.sem_flg |= IPC_NOWAIT;
                iRet=semop(m_vHandle, &buf_p, 1 );
            }
            else
            {
        #ifdef OS_FREEBSD//freebsd不支持semtimedop
                iRet=semop(m_vHandle, &buf_p, 1 );
        #else
                MDB_NTC_MAKE_SEM_TIMEOUT(timeout, iMilliSeconds);
                iRet=semtimedop(m_vHandle, &buf_p, 1, &timeout);
        #endif
            }
            return iRet==0;
#endif
        }

        bool TMdbNtcProcessLock::Unlock( )
        {
#ifdef  OS_WINDOWS
            if(m_vHandle==NULL) return false;
            return ReleaseSemaphore(m_vHandle,1,NULL) == TRUE;
#else
            if(m_vHandle==-1) return false;

            struct sembuf buf_p;

            buf_p.sem_num=0;
            buf_p.sem_op=1;                            //减少信号量的值
            buf_p.sem_flg=SEM_UNDO;

            return semop(m_vHandle,&buf_p,1)==0;
#endif
        }

        bool TMdbNtcProcessLock::ManualFree( )
        {
#ifdef  OS_WINDOWS
            if(m_vHandle!=NULL)
            {
                CloseHandle(m_vHandle);
                m_vHandle = NULL;
            }
#else
            if(m_vHandle>0)
            {
                semctl(m_vHandle,0,IPC_RMID,0);
                m_vHandle = -1;
            }
#endif
            return TMdbNtcSysNameObjectHelper::Free(this);
        }

        TMdbNtcStringBuffer TMdbNtcProcessLock::ToString( )
        {
            TMdbNtcStringBuffer sLineText;

            sLineText << "TBaseSemaphore :\n";
            if(IsOK())
                sLineText << "  Handle = [" << m_vHandle << "]\n";
            else
                sLineText << "  Handle = [NULL]\n";
            sLineText << "  Name = [" << GetName() << "]\n";
            sLineText << "  Created = [" << (m_bSelfCreated ? "True" :"False") << "]\n";

            return sLineText;
        }

        bool TMdbNtcProcessLock::CheckExist(const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return false;
            }
#ifdef OS_WINDOWS
            MDB_IPC_HANDLE hHandle = OpenSemaphore(SEMAPHORE_ALL_ACCESS,false, sIpcPath.c_str());
            if(hHandle)
            {
                CloseHandle(hHandle);
                hHandle = NULL;
                return true;
            }
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                MDB_IPC_HANDLE hHandle=semget(iNameKey,1,0666);
                if(hHandle>=0)
                {
                    return true;
                }
            }
#endif
            return false;
        }

        bool TMdbNtcProcessLock::Destroy(const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return false;
            }
#ifdef OS_WINDOWS
            //当引用计数变为0时，系统自动释放锁占用的资源。
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                MDB_IPC_HANDLE hHandle = semget(iNameKey,1,0666);
                if(hHandle>=0)
                {
                    semctl(hHandle,0,IPC_RMID,0);
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
#endif
            return TMdbNtcSysNameObjectHelper::Free(&oRuntimeObject, pszName, pszInstEnvName);
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcProcessLockPool::TPoolLock, TMdbNtcSystemNameObject);
        TMdbNtcProcessLockPool::TPoolLock::TPoolLock(TLockInfo *pLockInfo, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */)
            :TMdbNtcSystemNameObject(pLockInfo->szName, pszInstEnvName, pParentNameObject)
        {
            m_pLockInfo = pLockInfo;
        }

        TMdbNtcProcessLockPool::TPoolLock::~TPoolLock()
        {
            m_pLockInfo = NULL;
        }

        bool TMdbNtcProcessLockPool::TPoolLock::IsOK() const
        {
            if(m_pLockInfo)
            {
                return m_pLockInfo->bUsed;
            }
            else
            {
                TADD_WARNING("Mutex info is null");
                return false;
            }
        }

        TMdbNtcStringBuffer TMdbNtcProcessLockPool::TPoolLock::ToString() const
        {
            if(m_pLockInfo==NULL)
            {
                TADD_WARNING("Mutex info is null");
                return "";
            }

            TMdbNtcStringBuffer sLineText;
            sLineText << "CurrentPID=[" << (MDB_UINT32)m_pLockInfo->uiCurPid << "],Use=[" << m_pLockInfo->bUsed
                << "],Name=["<< m_pLockInfo->szName << "],Index=[" << m_pLockInfo->uiIndex
                << "],SelfCreate=["<< m_bSelfCreated << "]";
            return sLineText;
        }

        bool TMdbNtcProcessLockPool::TPoolLock::ManualFree()
        {
            m_pLockInfo->uiCurPid = 0;
            memset(m_pLockInfo->szName, 0x00, sizeof(m_pLockInfo->szName));
            m_pLockInfo->bUsed = false;
            return TMdbNtcSysNameObjectHelper::Free(this);
        }

        static pid_t gs_uimdbCurPid = 0;///< 用于锁池记录当前pid，避免频繁获取当前进程的pid
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcProcessLockPool, TMdbNtcSystemNameObject);
        TMdbNtcProcessLockPool::TMdbNtcProcessLockPool(MDB_UINT32 uiLockSize, const char* pszPoolName, MDB_UINT32 uiLockCount /* = 0 */,
            const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, bool bCheckVersion /* = true */)
            :TMdbNtcSystemNameObject(pszPoolName, pszInstEnvName)
        {
            if(gs_uimdbCurPid == 0) gs_uimdbCurPid = getpid();
            m_pPoolBuffer = NULL;
            m_ppLocks = NULL;
            m_uiLockSize = uiLockSize;
            if(m_bConstructResult == false) return;
            TMdbNtcStringBuffer sBuffer;
            sBuffer<<GetObjectName()<<"_SEM_"<<pszPoolName;
            m_pSemaphore = new TMdbNtcProcessLock(sBuffer.c_str(), pszInstEnvName, this);
            if(false == m_pSemaphore->GetConstructResult())
            {
                delete m_pSemaphore;
                m_pSemaphore = NULL;
                m_bConstructResult = false;
                return;
            }
            TMdbNtcAutoLock AutoLock(m_pSemaphore);
            sBuffer.Clear()<<GetObjectName()<<"_SHM_"<<pszPoolName;
            m_pPoolBuffer = new TMdbNtcShareMem(sBuffer.c_str(), uiLockCount==0?0:(m_uiLockSize*uiLockCount+sizeof(MDB_UINT32)), pszInstEnvName, this, bCheckVersion);
            if(false == m_pPoolBuffer->GetConstructResult())
            {
                delete m_pPoolBuffer;
                m_pPoolBuffer = NULL;
                m_bConstructResult = false;
                return;
            }
            MDB_UINT32 uiRealLockCount = *(MDB_UINT32*)m_pPoolBuffer->GetBuffer();
            if(m_pPoolBuffer->IsCreated())
            {
                m_bSelfCreated = true;
                memset(m_pPoolBuffer->GetBuffer(), 0x00, m_uiLockSize*uiLockCount+sizeof(MDB_UINT32));
                *(MDB_UINT32*)m_pPoolBuffer->GetBuffer() = uiRealLockCount = uiLockCount;
            }
            if(uiRealLockCount > 10000 || uiRealLockCount == 0)
            {
                m_pPoolBuffer->ManualFree();
                delete m_pPoolBuffer;
                m_pPoolBuffer = NULL;
                m_bConstructResult = false;
                TADD_WARNING("Lock Pool Error! Name=[%s],MutexPoolCount=[%u]", m_pPoolBuffer->GetName(), uiRealLockCount);
                return;
            }
            m_ppLocks = new TPoolLock* [uiRealLockCount];
            memset(m_ppLocks, 0x00, sizeof(TPoolLock*)*uiRealLockCount);
        }

        TMdbNtcProcessLockPool::~TMdbNtcProcessLockPool()
        {
            if(m_ppLocks)
            {
                MDB_UINT32 uiLockCount = GetLockCount();
                for (MDB_UINT32 i = 0; i < uiLockCount; ++i)
                {
                    if(m_ppLocks[i])
                    {
                        delete m_ppLocks[i];
                        m_ppLocks[i] = NULL;
                    }
                }
                delete []m_ppLocks;
                m_ppLocks = NULL;
            }
            if(m_pPoolBuffer!=NULL)
            {
                delete m_pPoolBuffer;
                m_pPoolBuffer = NULL;
            }
            if(m_pSemaphore!=NULL)
            {
                delete m_pSemaphore;
                m_pSemaphore = NULL;
            }
        }

        TMdbNtcStringBuffer TMdbNtcProcessLockPool::ToString() const
        {
            TMdbNtcStringBuffer sLineText;

            sLineText <<GetObjectName()<< " : \n";
            sLineText << "  LockCount = " << GetLockCount() << "\n";

            return sLineText;
        }

        bool TMdbNtcProcessLockPool::ManualFree()
        {
            if(m_pPoolBuffer)
            {
                TMdbNtcAutoLock oAutoLock(m_pSemaphore);
                if(m_pPoolBuffer->IsLastestVersion())
                {
                    MDB_UINT32 uiLockCount = GetLockCount();
                    TLockInfo* pLockInfo = reinterpret_cast<TLockInfo*>(m_pPoolBuffer->GetBuffer() + sizeof(MDB_UINT32));
                    for (MDB_UINT32 i = 0; i < uiLockCount; ++i)
                    {
                        if(pLockInfo->bUsed)
                        {
                            TPoolLock* pLock = GetNewLock(pLockInfo, this);
                            if(pLock == NULL) return false;
                            pLock->ManualFree();
                            delete pLock;
                            pLock = NULL;
                        }
                        if(m_ppLocks && m_ppLocks[i])
                        {
                            delete m_ppLocks[i];
                            m_ppLocks[i] = NULL;
                        }
                        pLockInfo = reinterpret_cast<TLockInfo*>(reinterpret_cast<unsigned char*>(pLockInfo)+m_uiLockSize);
                    }
                    if(m_ppLocks)
                    {
                        delete []m_ppLocks;
                        m_ppLocks = NULL;
                    }
                }
                else
                {
                    if(m_ppLocks)
                    {
                        delete []m_ppLocks;
                        m_ppLocks = NULL;
                    }
                }
                m_pPoolBuffer->ManualFree();
                delete m_pPoolBuffer;
                m_pPoolBuffer = NULL;
            }
            if(m_pSemaphore)
            {
                m_pSemaphore->ManualFree();
                delete m_pSemaphore;
                m_pSemaphore = NULL;
            }
            return true;
        }

        TMdbNtcBaseLock* TMdbNtcProcessLockPool::GetLock(const char *pszName, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */)
        {
            if(m_pSemaphore == NULL || m_pPoolBuffer == NULL)
            {
                TADD_WARNING("Pool is not initialized!");
                return NULL;
            }
            else if(!m_pPoolBuffer->IsLastestVersion())
            {
                TADD_WARNING("Pool'version is not lastest!");
                return NULL;
            }
            TMdbNtcAutoLock AutoLock(m_pSemaphore);
            MDB_UINT32 uiLockCount = GetLockCount();
            TLockInfo* pLockInfo = reinterpret_cast<TLockInfo*>(m_pPoolBuffer->GetBuffer()+ sizeof(MDB_UINT32));
            MDB_UINT32 uiIndex = (MDB_UINT32)-1, i = 0;
            for(i = 0;i < uiLockCount; ++i)
            {
                if(pLockInfo->bUsed)
                {
                    if(strcmp(pLockInfo->szName+m_sName.length()+1, pszName)==0)
                    {
                        if(m_ppLocks[i]) return m_ppLocks[i];//如果已经初始化过，则直接返回
                        uiIndex = i;
                        break;
                    }
                }
                else if(uiIndex == (MDB_UINT32)-1)
                {
                    uiIndex = i;
                }
                pLockInfo = reinterpret_cast<TLockInfo*>(reinterpret_cast<unsigned char*>(pLockInfo)+m_uiLockSize);
            }
            if(uiIndex == (MDB_UINT32)-1)
            {
                TADD_WARNING("TMdbNtcProcessLockPool::GetLock(Name=[%s], PoolCount[%u]) Result is Null.", pszName, uiLockCount);
                return NULL;
            }
            pLockInfo = GetLockInfo(uiIndex);
            if(i == uiLockCount)
            {
                pLockInfo->uiIndex = uiIndex;
                MdbNtcSnprintf(pLockInfo->szName, sizeof(pLockInfo->szName), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", m_sName.c_str(), pszName);
            }
            TPoolLock *pLock = GetNewLock(pLockInfo, pParentNameObject);
            if(pLock)
            {
                //MDB_NTC_ADD_DETAIL_TF("Open mutex : %s",pMutexInfo->ToString().c_str());
                //检测占有锁的进程是否存在，如果不存在，则释放锁
                if(pLockInfo->uiCurPid != 0 && !TMdbNtcSysUtils::IsProcessExist(pLockInfo->uiCurPid))
                {
                    TADD_NORMAL("Mutex[%s]'s current pid[%u] is not exist, so unlock it.",
                        pszName, pLockInfo->uiCurPid);
                    pLock->Unlock();
                }
                m_ppLocks[uiIndex] = pLock;
            }
            else
            {
                TADD_WARNING("%s mutex[%s] failed!", i==uiLockCount?"Create":"Open", pszName);
            }
            return pLock;
        }

        TMdbNtcBaseLock* TMdbNtcProcessLockPool::GetLock(MDB_UINT32 uiIndex, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */)
        {
            if(m_ppLocks && m_ppLocks[uiIndex]) return m_ppLocks[uiIndex];//如果已经初始化过，则直接返回
            if(m_pSemaphore == NULL || m_pPoolBuffer == NULL)
            {
                TADD_WARNING("Pool is not initialized!");
                return NULL;
            }
            else if(!m_pPoolBuffer->IsLastestVersion())
            {
                TADD_WARNING("Pool'version is not lastest!");
                return NULL;
            }
            else if(uiIndex >= GetLockCount())
            {
                TADD_WARNING("Index[%u] is more than mutex count[%u]!", uiIndex, GetLockCount());
                return NULL;
            }
            TMdbNtcAutoLock AutoLock(m_pSemaphore);
            //加锁后，再判断一次
            if(m_ppLocks[uiIndex]) return m_ppLocks[uiIndex];//如果已经初始化过，则直接返回
            TLockInfo* pLockInfo = GetLockInfo(uiIndex);
            if(pLockInfo->bUsed == false)
            {
                pLockInfo->uiIndex = uiIndex;
                MdbNtcSnprintf(pLockInfo->szName, sizeof(pLockInfo->szName), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%u", m_sName.c_str(), uiIndex);
            }
            TPoolLock* pLock = GetNewLock(pLockInfo, pParentNameObject);
            if(pLock)
            {
                m_ppLocks[uiIndex] = pLock;
            }
            else
            {
                TADD_WARNING("Get Lock[%s] failed!", pLockInfo->szName);
            }
            return pLock;
        }

        int TMdbNtcProcessLockPool::FindIndex(const char* pszName)
        {
            if(m_pSemaphore == NULL || m_pPoolBuffer == NULL)
            {
                TADD_WARNING("Pool is not initialized!");
                return -1;
            }
            else if(!m_pPoolBuffer->IsLastestVersion())
            {
                TADD_WARNING("Pool'version is not lastest!");
                return -1;
            }
            MDB_UINT32 uiLockCount = GetLockCount();
            TLockInfo* pLockInfo = reinterpret_cast<TLockInfo*>(m_pPoolBuffer->GetBuffer()+ sizeof(MDB_UINT32));
            for(MDB_UINT32 i = 0;i < uiLockCount; ++i)
            {
                if(pLockInfo->bUsed && strcmp(pLockInfo->szName+m_sName.length()+1, pszName)==0)
                {
                    return (int)i;
                }
                else
                {
                    pLockInfo = reinterpret_cast<TLockInfo*>(reinterpret_cast<unsigned char*>(pLockInfo)+m_uiLockSize);
                }
            }
            return -1;
        }

        bool TMdbNtcProcessLockPool::Destroy(MDB_UINT32 uiIndex)
        {
            if(m_pSemaphore == NULL || m_pPoolBuffer == NULL)
            {
                TADD_WARNING("Pool is not initialized!");
                return false;
            }
            else if(!m_pPoolBuffer->IsLastestVersion())
            {
                TADD_WARNING("Pool'version is not lastest!");
                return false;
            }
            if(uiIndex == (MDB_UINT32)-1) return true;
            TMdbNtcAutoLock oAutoLock(m_pSemaphore);
            TLockInfo* pLockInfo = GetLockInfo(uiIndex);
            if(!pLockInfo->bUsed) return true;
            TPoolLock* pLock = GetNewLock(pLockInfo, this);
            if(pLock == NULL) return false;
            pLock->ManualFree();
            delete pLock;
            pLock = NULL;
            if(m_ppLocks && m_ppLocks[uiIndex])
            {
                delete m_ppLocks[uiIndex];
                m_ppLocks[uiIndex] = NULL;
            }
            return true;
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcProcessPerfLock, TMdbNtcProcessLockPool::TPoolLock);
        TMdbNtcProcessPerfLock::TMdbNtcProcessPerfLock(TLockInfo *pLockInfo, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */)
            :TMdbNtcProcessLockPool::TPoolLock(pLockInfo, pszInstEnvName, pParentNameObject)
        {
#ifdef OS_WINDOWS
            m_hLockHandle = NULL;
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(this);
            if(sIpcPath.IsEmpty())
            {
                m_bConstructResult = false;
                TADD_WARNING( "GetIpcPath failed! name[%s]", pLockInfo->szName);
                return;
            }
            m_hLockHandle = OpenMutex(MUTEX_ALL_ACCESS, true, sIpcPath.c_str());
            if(m_hLockHandle == NULL)
            {
                //不存在则生成
                m_hLockHandle = CreateMutex(NULL,0, sIpcPath.c_str());
                if(m_hLockHandle == NULL)
                {
                    m_bConstructResult = false;
                    TADD_WARNING("TAbsWinMutex::TAbsWinMutex() : CreateMutex(Name=%s) RETURN=[NULL]", sIpcPath.c_str());
                    return;
                }
                pLockInfo->bUsed = true;
                m_bSelfCreated = true;
            }
#else
            if(pLockInfo->bUsed == false)
            {
                //设置互斥锁属性
                pthread_mutexattr_t hLockAttr;
                int iRet = ::pthread_mutexattr_init(&hLockAttr);
                if(iRet != 0)
                {
                    TADD_WARNING("pthread_mutexattr_init is fail. ErrorCode = %d",iRet);
                    return;
                }
                do
                {
                    //默认：进程间使用
                    iRet = ::pthread_mutexattr_setpshared(&hLockAttr, PTHREAD_PROCESS_SHARED);
                    if(iRet != 0)
                    {
                        TADD_WARNING("pthread_mutexattr_setpshared is fail. ErrorCode = %d",iRet);
                        break;
                    }

#ifdef OS_HP
                    iRet =  ::pthread_mutexattr_setspin_np(&hLockAttr, PTHREAD_MUTEX_SPINONLY_NP);
                    if(iRet != 0)
                    {
                        TADD_WARNING("pthread_mutexattr_setspin_np is fail. ErrorCode = %d",iRet);
                        break;
                    }
#endif
                    //设置互斥锁
                    iRet = ::pthread_mutex_init(&pLockInfo->m_hLock, &hLockAttr);
                    if(iRet != 0)
                    {
                        TADD_WARNING("pthread_mutex_init is fail. ErrorCode = %d",iRet);
                        break;
                    }
                } while (0);
                ::pthread_mutexattr_destroy(&hLockAttr);
                if(iRet != 0)
                {
                    m_bConstructResult = false;
                    return;
                }
                pLockInfo->bUsed = true;
                m_bSelfCreated = true;
            }
            m_bConstructResult = true;
#endif
            if(m_bConstructResult)
            {
                TMdbNtcSysNameObjectHelper::SignIn(this);
            }
        }

        TMdbNtcProcessPerfLock::~TMdbNtcProcessPerfLock()
        {
#ifdef OS_WINDOWS
            if(m_hLockHandle!=NULL)
            {
                CloseHandle(m_hLockHandle);
                m_hLockHandle=NULL;
            }
#endif
        }

        bool TMdbNtcProcessPerfLock::ManualFree()
        {
            if(m_pLockInfo == NULL)
            {
                TADD_WARNING("Lock info is null");
                return false;
            }
            else if(!m_pLockInfo->bUsed) return true;
#ifdef OS_WINDOWS
            if(m_hLockHandle!=NULL)
            {
                CloseHandle(m_hLockHandle);
                m_hLockHandle=NULL;
            }
#else
            //释放互斥锁
            ::pthread_mutex_destroy(&static_cast<TLockInfo*>(m_pLockInfo)->m_hLock);
#endif
            return TPoolLock::ManualFree();
        }

        TMdbNtcStringBuffer TMdbNtcProcessPerfLock::ToString() const
        {
            if(m_pLockInfo==NULL)
            {
                TADD_WARNING("Lock info is null");
                return "";
            }

            TMdbNtcStringBuffer sLineText = TPoolLock::ToString();
#ifdef OS_WINDOWS
            sLineText << ",Handle=[" << m_hLockHandle << "]";
#endif
            return sLineText;
        }

        bool TMdbNtcProcessPerfLock::Lock(int iMilliSeconds)
        {
            if(m_pLockInfo == NULL)
            {
                TADD_WARNING("Lock info is null.");
                return false;
            }
            else if(m_pLockInfo->bUsed == false)
            {
                TADD_WARNING("Lock is not initialized.");
                return false;
            }
#ifdef OS_WINDOWS
            if(m_hLockHandle == NULL)
            {
                TADD_WARNING("Lock handle is not initialized.");
                return false;
            }
            else WaitForSingleObject(m_hLockHandle, INFINITE);
#else
            ::pthread_mutex_lock(&static_cast<TLockInfo*>(m_pLockInfo)->m_hLock);
#endif
            m_pLockInfo->uiCurPid = gs_uimdbCurPid;
            return true;
        }

        bool TMdbNtcProcessPerfLock::Unlock()
        {
            if(m_pLockInfo == NULL)
            {
                TADD_WARNING("Lock info is null.");
                return false;
            }
            else if(m_pLockInfo->bUsed == false)
            {
                TADD_WARNING("Lock is not initialized.");
                return false;
            }
#ifdef OS_WINDOWS
            if(m_hLockHandle == NULL)
            {
                TADD_WARNING("Lock handle is not initialized.");
                return false;
            }
            else ReleaseMutex(m_hLockHandle);
#else
            ::pthread_mutex_unlock(&static_cast<TLockInfo*>(m_pLockInfo)->m_hLock);
#endif
            m_pLockInfo->uiCurPid = 0;
            return true;
        }

        bool TMdbNtcProcessPerfLock::CheckExist(const char* pszPoolName, const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
#ifdef OS_WINDOWS
            char szName[MDB_NTC_ZS_MAX_IPC_KEY_NAME_SIZE] = {'\0'};
            MdbNtcSnprintf(szName, sizeof(szName), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", pszPoolName, pszName);
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(MDB_ZF_RUNTIME_OBJECT(TMdbNtcProcessPerfLock), szName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                TADD_WARNING( "GetIpcPath failed! name[%s]", pszName);
                return false;
            }
            HANDLE hHandle = OpenMutex(MUTEX_ALL_ACCESS, true, sIpcPath.c_str());
            if(hHandle)
            {
                CloseHandle(hHandle);
                hHandle = NULL;
                return true;
            }
#else
            TMdbNtcProcessPerfLockPool oPool(pszPoolName, 0, pszInstEnvName);
            if(oPool.GetConstructResult())
            {
                return oPool.CheckExist(pszName);
            }
#endif
            return false;
        }

        bool TMdbNtcProcessPerfLock::Destroy(const char* pszPoolName, const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            bool bRet = true;
            TMdbNtcProcessPerfLockPool oPool(pszPoolName, 0, pszInstEnvName);
            if(oPool.GetConstructResult())
            {
                bRet = oPool.Destroy(pszName);
            }
            return bRet;
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcProcessPerfLockPool, TMdbNtcProcessLockPool);
        TMdbNtcProcessPerfLockPool::TMdbNtcProcessPerfLockPool(const char* pszPoolName, MDB_UINT32 uiLockCount /* = 0 */, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, bool bCheckVersion /* = true */)
            :TMdbNtcProcessLockPool(sizeof(TMdbNtcProcessPerfLock::TLockInfo), pszPoolName, uiLockCount, pszInstEnvName, bCheckVersion)
        {
        }

        bool TMdbNtcProcessPerfLockPool::Lock(MDB_UINT32 uiIndex, int iMilliSeconds/* =-1 */)
        {
            if(m_ppLocks && uiIndex < GetLockCount() && m_ppLocks[uiIndex]) return m_ppLocks[uiIndex]->Lock(iMilliSeconds);
            TMdbNtcBaseLock* pLock = GetLock(uiIndex);
            if(pLock == NULL) return false;
            else return pLock->Lock(iMilliSeconds);
        }

        bool TMdbNtcProcessPerfLockPool::TryLock(MDB_UINT32 uiIndex)
        {
            if(m_ppLocks && uiIndex < GetLockCount() && m_ppLocks[uiIndex]) return m_ppLocks[uiIndex]->TryLock();
            TMdbNtcBaseLock* pLock = GetLock(uiIndex);
            if(pLock == NULL) return false;
            else return pLock->TryLock();
        }

        bool TMdbNtcProcessPerfLockPool::Unlock(MDB_UINT32 uiIndex)
        {
            if(m_ppLocks && uiIndex < GetLockCount() && m_ppLocks[uiIndex]) return m_ppLocks[uiIndex]->Unlock();
            TMdbNtcBaseLock* pLock = GetLock(uiIndex);
            if(pLock == NULL) return false;
            else return pLock->Unlock();
        }

        TMdbNtcProcessLockPool::TPoolLock* TMdbNtcProcessPerfLockPool::GetNewLock(TMdbNtcProcessLockPool::TLockInfo* pLockInfo, TMdbNtcSystemNameObject* pParentNameObject)
        {
            TMdbNtcProcessPerfLock* pLock = new TMdbNtcProcessPerfLock(static_cast<TMdbNtcProcessPerfLock::TLockInfo*>(pLockInfo), m_sInstEnvName.c_str(), pParentNameObject);
            if(pLock->GetConstructResult()) return pLock;
            delete pLock;
            pLock = NULL;
            return NULL;
        }

        bool TMdbNtcProcessPerfLockPool::DestroyPool(const char* pszPoolName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            TMdbNtcProcessPerfLockPool oLockPool(pszPoolName, 0, pszInstEnvName, false);
            if(oLockPool.GetConstructResult())
            {
                return oLockPool.ManualFree();
            }
            else
            {
                return true;
            }
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcProcessRWLock, TMdbNtcProcessLockPool::TPoolLock);
        TMdbNtcProcessRWLock::TMdbNtcProcessRWLock(TLockInfo *pLockInfo, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */)
            :TMdbNtcProcessLockPool::TPoolLock(pLockInfo, pszInstEnvName, pParentNameObject)
        {
#ifdef OS_WINDOWS
            m_hLockHandle = NULL;
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(this);
            if(sIpcPath.IsEmpty())
            {
                m_bConstructResult = false;
                TADD_WARNING( "GetIpcPath failed! name[%s]", pLockInfo->szName);
                return;
            }
            m_hLockHandle = OpenMutex(MUTEX_ALL_ACCESS, true, sIpcPath.c_str());
            if(m_hLockHandle == NULL)
            {
                //不存在则生成
                m_hLockHandle = CreateMutex(NULL,0, sIpcPath.c_str());
                if(m_hLockHandle == NULL)
                {
                    m_bConstructResult = false;
                    TADD_WARNING("TMdbNtcProcessRWLock::TMdbNtcProcessRWLock() : CreateMutex(Name=%s) RETURN=[NULL]", sIpcPath.c_str());
                    return;
                }
                pLockInfo->bUsed = true;
                m_bSelfCreated = true;
            }
#else
            if(pLockInfo->bUsed == false)
            {
                //设置互斥锁属性
                pthread_rwlockattr_t    hLockAttr;
                int iRet = ::pthread_rwlockattr_init(&hLockAttr);
                if(iRet != 0)
                {
                    TADD_WARNING("pthread_rwlockattr_init is fail. ErrorCode = %d",iRet);
                    return;
                }
                do
                {
                    //默认：进程间使用
                    iRet = ::pthread_rwlockattr_setpshared(&hLockAttr, PTHREAD_PROCESS_SHARED);
                    if(iRet != 0)
                    {
                        TADD_WARNING("pthread_rwlockattr_setpshared is fail. ErrorCode = %d",iRet);
                        break;
                    }
                    //设置互斥锁
                    iRet = ::pthread_rwlock_init(&pLockInfo->m_hRWLock, &hLockAttr);
                    if(iRet != 0)
                    {
                        TADD_WARNING("pthread_rwlock_init is fail. ErrorCode = %d",iRet);
                        break;
                    }
                } while (0);
                //释放互斥锁属性
                ::pthread_rwlockattr_destroy(&hLockAttr);
                if(iRet != 0)
                {
                    m_bConstructResult = false;
                    return;
                }
                pLockInfo->bUsed = true;
                m_bSelfCreated = true;
            }
            m_bConstructResult = true;
#endif
            if(m_bConstructResult)
            {
                TMdbNtcSysNameObjectHelper::SignIn(this);
            }
        }

        TMdbNtcProcessRWLock::~TMdbNtcProcessRWLock()
        {
#ifdef OS_WINDOWS
            if(m_hLockHandle!=NULL)
            {
                CloseHandle(m_hLockHandle);
                m_hLockHandle=NULL;
            }
#endif
        }

        bool TMdbNtcProcessRWLock::ManualFree()
        {
            if(m_pLockInfo == NULL)
            {
                TADD_WARNING("Mutex info is null");
                return false;
            }
            else if(!m_pLockInfo->bUsed) return true;
#ifdef OS_WINDOWS
            if(m_hLockHandle!=NULL)
            {
                CloseHandle(m_hLockHandle);
                m_hLockHandle=NULL;
            }
#else
            //释放互斥锁
            ::pthread_rwlock_destroy(&static_cast<TLockInfo*>(m_pLockInfo)->m_hRWLock);
#endif
            return TPoolLock::ManualFree();
        }

        TMdbNtcStringBuffer TMdbNtcProcessRWLock::ToString() const
        {
            if(m_pLockInfo==NULL)
            {
                TADD_WARNING("Lock info is null");
                return "";
            }

            TMdbNtcStringBuffer sLineText = TPoolLock::ToString();
#ifdef OS_WINDOWS
            sLineText << ",Handle=[" << m_hLockHandle << "]";
#endif
            return sLineText;
        }

        bool TMdbNtcProcessRWLock::Lock(int iMilliSeconds/* =-1 */)
        {
            return WLock(iMilliSeconds);
        }

        bool TMdbNtcProcessRWLock::RLock(int iMilliSeconds/* =-1 */)
        {
            if(m_pLockInfo == NULL)
            {
                TADD_WARNING("Lock info is null.");
                return false;
            }
            else if(m_pLockInfo->bUsed == false)
            {
                TADD_WARNING("Lock is not initialized.");
                return false;
            }
#ifdef OS_WINDOWS
            if(m_hLockHandle == NULL)
            {
                TADD_WARNING("Lock handle is not initialized.");
                return false;
            }
            else WaitForSingleObject(m_hLockHandle, (DWORD)iMilliSeconds);
#else
            if(iMilliSeconds == -1)
            {
                if(pthread_rwlock_rdlock(&static_cast<TLockInfo*>(m_pLockInfo)->m_hRWLock) != 0)
                {
                    TADD_WARNING( "RLock failed, errno = %d \n", errno );
                    return false;
                }
            }
            else
            {
#ifdef OS_HP
                int iElpasedMilliSeconds = 0;
                do
                {
                    if(TryRLock()) return true;
                    TMdbNtcDateTime::Sleep(50);
                    iElpasedMilliSeconds += 50;
                } while (iElpasedMilliSeconds <= iMilliSeconds);
                return false;
#else
                MDB_NTC_MAKE_TIMEOUT(timeout, iMilliSeconds);
                if(pthread_rwlock_timedwrlock(&static_cast<TLockInfo*>(m_pLockInfo)->m_hRWLock, &timeout) != 0)
                {
                    TADD_WARNING( "RLock failed, errno = %d \n", errno );
                    return false;
                }
#endif
            }
#endif
            m_pLockInfo->uiCurPid = gs_uimdbCurPid;
            return true;
        }

        bool TMdbNtcProcessRWLock::TryRLock()
        {
            if(m_pLockInfo == NULL)
            {
                TADD_WARNING("Lock info is null.");
                return false;
            }
            else if(m_pLockInfo->bUsed == false)
            {
                TADD_WARNING("Lock is not initialized.");
                return false;
            }
#ifdef OS_WINDOWS
            if(m_hLockHandle == NULL)
            {
                TADD_WARNING("Lock handle is not initialized.");
                return false;
            }
            else WaitForSingleObject(m_hLockHandle, 0);
#else
            int iRet = pthread_rwlock_tryrdlock(&static_cast<TLockInfo*>(m_pLockInfo)->m_hRWLock);
            if(iRet == EBUSY) return false;
            else if(iRet != 0)
            {
                TADD_WARNING( "TryRLock failed, errno = %d \n", errno );
                return false;
            }
#endif
            m_pLockInfo->uiCurPid = gs_uimdbCurPid;
            return true;
        }

        bool TMdbNtcProcessRWLock::WLock(int iMilliSeconds/* =-1 */)
        {
            if(m_pLockInfo == NULL)
            {
                TADD_WARNING("Lock info is null.");
                return false;
            }
            else if(m_pLockInfo->bUsed == false)
            {
                TADD_WARNING("Lock is not initialized.");
                return false;
            }
#ifdef OS_WINDOWS
            if(m_hLockHandle == NULL)
            {
                TADD_WARNING("Lock handle is not initialized.");
                return false;
            }
            else WaitForSingleObject(m_hLockHandle, (DWORD)iMilliSeconds);
#else
            if(iMilliSeconds == -1)
            {
                if(pthread_rwlock_wrlock(&static_cast<TLockInfo*>(m_pLockInfo)->m_hRWLock) != 0)
                {
                    TADD_WARNING( "RLock failed, errno = %d \n", errno );
                    return false;
                }
            }
            else
            {
#ifdef OS_HP
                int iElpasedMilliSeconds = 0;
                do
                {
                    if(TryWLock()) return true;
                    TMdbNtcDateTime::Sleep(50);
                    iElpasedMilliSeconds += 50;
                } while (iElpasedMilliSeconds <= iMilliSeconds);
                return false;
#else
                MDB_NTC_MAKE_TIMEOUT(timeout, iMilliSeconds);
                if(pthread_rwlock_timedrdlock(&static_cast<TLockInfo*>(m_pLockInfo)->m_hRWLock, &timeout) != 0)
                {
                    TADD_WARNING( "RLock failed, errno = %d \n", errno );
                    return false;
                }
#endif
            }
#endif
            m_pLockInfo->uiCurPid = gs_uimdbCurPid;
            return true;
        }

        bool TMdbNtcProcessRWLock::TryWLock()
        {
            if(m_pLockInfo == NULL)
            {
                TADD_WARNING("Lock info is null.");
                return false;
            }
            else if(m_pLockInfo->bUsed == false)
            {
                TADD_WARNING("Lock is not initialized.");
                return false;
            }
#ifdef OS_WINDOWS
            if(m_hLockHandle == NULL)
            {
                TADD_WARNING("Lock handle is not initialized.");
                return false;
            }
            WaitForSingleObject(m_hLockHandle, 0);
#else
            int iRet = pthread_rwlock_trywrlock(&static_cast<TLockInfo*>(m_pLockInfo)->m_hRWLock);
            if(iRet == EBUSY) return false;
            else if(iRet != 0)
            {
                TADD_WARNING( "TryWLock failed, errno = %d \n", errno );
                return false;
            }
#endif
            m_pLockInfo->uiCurPid = gs_uimdbCurPid;
            return true;
        }

        bool TMdbNtcProcessRWLock::Unlock()
        {
            if(m_pLockInfo == NULL)
            {
                TADD_WARNING("Lock info is null.");
                return false;
            }
            else if(m_pLockInfo->bUsed == false)
            {
                TADD_WARNING("Lock is not initialized.");
                return false;
            }
#ifdef OS_WINDOWS
            if(m_hLockHandle == NULL)
            {
                TADD_WARNING("Lock handle is not initialized.");
                return false;
            }
            else ReleaseMutex(m_hLockHandle);
#else
            ::pthread_rwlock_unlock(&static_cast<TLockInfo*>(m_pLockInfo)->m_hRWLock);
#endif
            m_pLockInfo->uiCurPid = 0;
            return true;
        }

        bool TMdbNtcProcessRWLock::CheckExist(const char* pszPoolName, const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
#ifdef OS_WINDOWS
            char szName[MDB_NTC_ZS_MAX_IPC_KEY_NAME_SIZE] = {'\0'};
            MdbNtcSnprintf(szName, sizeof(szName), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", pszPoolName, pszName);
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(MDB_ZF_RUNTIME_OBJECT(TMdbNtcProcessPerfLock), szName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                TADD_WARNING( "GetIpcPath failed! name[%s]", pszName);
                return false;
            }
            HANDLE hHandle = OpenMutex(MUTEX_ALL_ACCESS, true, sIpcPath.c_str());
            if(hHandle)
            {
                CloseHandle(hHandle);
                hHandle = NULL;
                return true;
            }
#else
            TMdbNtcProcessPerfLockPool oPool(pszPoolName, 0, pszInstEnvName);
            if(oPool.GetConstructResult())
            {
                return oPool.CheckExist(pszName);
            }
#endif
            return false;
        }

        bool TMdbNtcProcessRWLock::Destroy(const char* pszPoolName, const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            bool bRet = true;
            TMdbNtcProcessPerfLockPool oPool(pszPoolName, 0, pszInstEnvName);
            if(oPool.GetConstructResult())
            {
                bRet = oPool.Destroy(pszName);
            }
            return bRet;
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcProcessRWLockPool, TMdbNtcProcessLockPool);
        TMdbNtcProcessRWLockPool::TMdbNtcProcessRWLockPool(const char* pszPoolName, MDB_UINT32 uiLockCount /* = 0 */, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, bool bCheckVersion /* = true */)
            :TMdbNtcProcessLockPool(sizeof(TMdbNtcProcessRWLock::TLockInfo), pszPoolName, uiLockCount, pszInstEnvName, bCheckVersion)
        {
        }

        TMdbNtcProcessLockPool::TPoolLock* TMdbNtcProcessRWLockPool::GetNewLock(TMdbNtcProcessLockPool::TLockInfo* pLockInfo, TMdbNtcSystemNameObject* pParentNameObject)
        {
            TMdbNtcProcessRWLock* pLock = new TMdbNtcProcessRWLock(static_cast<TMdbNtcProcessRWLock::TLockInfo*>(pLockInfo), m_sInstEnvName.c_str(), pParentNameObject);
            if(pLock->GetConstructResult()) return pLock;
            else
            {
                delete pLock;
                pLock = NULL;
                return NULL;
            }
        }

        bool TMdbNtcProcessRWLockPool::DestroyPool(const char* pszPoolName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            TMdbNtcProcessPerfLockPool oLockPool(pszPoolName, 0, pszInstEnvName, false);
            if(oLockPool.GetConstructResult())
            {
                return oLockPool.ManualFree();
            }
            else
            {
                return true;
            }
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcThreadLock, TMdbNtcComponentObject);
        TMdbNtcThreadLock::TMdbNtcThreadLock()
        {
        #ifdef OS_WINDOWS
            InitializeCriticalSection(&m_hMutex);
        #else
            pthread_mutexattr_t    attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);//递归锁
            pthread_mutex_init(&m_hMutex, &attr);
            pthread_mutexattr_destroy(&attr);
        #endif
        }

        TMdbNtcThreadLock::~TMdbNtcThreadLock()
        {
        #ifdef OS_WINDOWS
            DeleteCriticalSection(&m_hMutex);
        #else
            pthread_mutex_destroy(&m_hMutex);
        #endif
        }

        bool TMdbNtcThreadLock::Lock(int iMilliSeconds)
        {
#ifdef OS_WINDOWS
            EnterCriticalSection(&m_hMutex);
            return true;
#elif defined(OS_HP)
            return pthread_mutex_lock(&m_hMutex)==0;
#else
            if(iMilliSeconds == -1) return pthread_mutex_lock(&m_hMutex)==0;
            MDB_NTC_MAKE_TIMEOUT(timeout, iMilliSeconds);
            return pthread_mutex_timedlock(&m_hMutex, &timeout)==0;
#endif
        }

        bool TMdbNtcThreadLock::TryLock()
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            iRet = TryEnterCriticalSection(&m_hMutex)?0:-1;
        #else
            iRet = pthread_mutex_trylock(&m_hMutex);
        #endif
            return iRet==0?true:false;
        }

        bool TMdbNtcThreadLock::Unlock()
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            LeaveCriticalSection(&m_hMutex);
        #else
            iRet = pthread_mutex_unlock(&m_hMutex);
        #endif
            return iRet==0?true:false;
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcThreadRWLock, TMdbNtcComponentObject);
        //初始化读写锁
        TMdbNtcThreadRWLock::TMdbNtcThreadRWLock()
        {
#ifdef OS_WINDOWS
            InitializeCriticalSection(&m_hRWLock);
#else
            if (pthread_rwlock_init(&m_hRWLock,NULL) != 0)
            {
                m_bConstructResult = false;
                TADD_WARNING("InitRwLock failed, errno = %d \n", errno );
                return;
            }
#endif
        }

        TMdbNtcThreadRWLock::~TMdbNtcThreadRWLock()
        {
#ifdef OS_WINDOWS
            DeleteCriticalSection(&m_hRWLock);
#else
            if (pthread_rwlock_destroy(&m_hRWLock) != 0)
            {
                TADD_WARNING( "~TMdbNtcThreadRWLock failed, errno = %d \n", errno );
            }
#endif
        }

        bool TMdbNtcThreadRWLock::Lock(int iMilliSeconds/* =-1 */)
        {
            return WLock(iMilliSeconds);
        }

        //获取读锁--阻塞
        bool TMdbNtcThreadRWLock::RLock(int iMilliSeconds/* =-1 */)
        {
#ifdef OS_WINDOWS
            EnterCriticalSection(&m_hRWLock);
            return true;
#else
            if(iMilliSeconds == -1)
            {
                if(pthread_rwlock_rdlock(&m_hRWLock) != 0)
                {
                    TADD_WARNING( "RLock failed, errno = %d \n", errno );
                    return false;
                }
            }
            else
            {
#ifdef OS_HP
                int iElpasedMilliSeconds = 0;
                do
                {
                    if(TryRLock()) return true;
                    TMdbNtcDateTime::Sleep(50);
                    iElpasedMilliSeconds += 50;
                } while (iElpasedMilliSeconds <= iMilliSeconds);
                return false;
#else
                MDB_NTC_MAKE_TIMEOUT(timeout, iMilliSeconds);
                if(pthread_rwlock_timedwrlock(&m_hRWLock, &timeout) != 0)
                {
                    TADD_WARNING( "RLock failed, errno = %d \n", errno );
                    return false;
                }
#endif
            }
            return true;
#endif
        }

        //获取读锁--非阻塞
        bool TMdbNtcThreadRWLock::TryRLock()
        {
#ifdef OS_WINDOWS
            return TryEnterCriticalSection(&m_hRWLock)==TRUE;
#else
            int iRet = pthread_rwlock_tryrdlock(&m_hRWLock);
            if(iRet == 0) return true;
            else if(iRet == EBUSY) return false;
            else
            {
                TADD_WARNING( "TryRLock failed, errno = %d \n", errno );
                return false;
            }
#endif
        }

        //获取写锁--阻塞
        bool TMdbNtcThreadRWLock::WLock(int iMilliSeconds/* =-1 */)
        {
#ifdef OS_WINDOWS
            EnterCriticalSection(&m_hRWLock);
            return true;
#else
            if(iMilliSeconds == -1)
            {
                if(pthread_rwlock_wrlock(&m_hRWLock) != 0)
                {
                    TADD_WARNING( "RLock failed, errno = %d \n", errno );
                    return false;
                }
            }
            else
            {
#ifdef OS_HP
                int iElpasedMilliSeconds = 0;
                do
                {
                    if(TryWLock()) return true;
                    TMdbNtcDateTime::Sleep(50);
                    iElpasedMilliSeconds += 50;
                } while (iElpasedMilliSeconds <= iMilliSeconds);
                return false;
#else
                MDB_NTC_MAKE_TIMEOUT(timeout, iMilliSeconds);
                if(pthread_rwlock_timedrdlock(&m_hRWLock, &timeout) != 0)
                {
                    TADD_WARNING( "RLock failed, errno = %d \n", errno );
                    return false;
                }
#endif
            }
            return true;
#endif
        }

        //获取写锁--非阻塞
        bool TMdbNtcThreadRWLock::TryWLock()
        {
#ifdef OS_WINDOWS
            return TryEnterCriticalSection(&m_hRWLock)==TRUE;
#else
            int iRet = pthread_rwlock_trywrlock(&m_hRWLock);
            if(iRet == 0) return true;
            else if(iRet == EBUSY) return false;
            else
            {
                TADD_WARNING( "TryRLock failed, errno = %d \n", errno );
                return false;
            }
#endif
        }

        //解除锁定读写锁
        bool TMdbNtcThreadRWLock::Unlock()
        {
#ifdef OS_WINDOWS
            LeaveCriticalSection(&m_hRWLock);
            return true;
#else
            if (pthread_rwlock_unlock(&m_hRWLock) != 0)
            {
                TADD_WARNING( "UnLock failed, errno = %d \n", errno );
                return false;
            }
            return true;
#endif
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcProcessEvent, TMdbNtcSystemNameObject);
        TMdbNtcProcessEvent::TMdbNtcProcessEvent(const char* pszEventName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */)
            :TMdbNtcSystemNameObject(pszEventName, pszInstEnvName, pParentNameObject)
        {
            m_bConstructResult=true;
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(this);
            if(sIpcPath.IsEmpty())
            {
                m_bConstructResult = false;
                TADD_WARNING( "GetIpcPath failed! name[%s]", pszEventName);
                return;
            }
#ifdef OS_WINDOWS
            m_hHandle = OpenEvent( EVENT_ALL_ACCESS, FALSE, sIpcPath.c_str() );
            if(m_hHandle==NULL)
            {
                m_hHandle = CreateEvent(NULL, FALSE, FALSE, sIpcPath.c_str()); //第三个参数有信号、无信号，TRUE OR FALSE ？[杜]
            }
            m_bConstructResult = (m_hHandle!=NULL);
#else
            m_hHandle=0;
            key_t iKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iKey == -1)
            {
                m_bConstructResult = false;
                TADD_WARNING("MakeIPCKey(%s) RETURN=[False],ErrorCode=%d\n", sIpcPath.c_str(), errno);
                return;
            }
            m_hHandle = semget (iKey, 0, 0666);
            if(m_hHandle >= 0)
            {
                m_bSelfCreated = false;
            }
            else
            {
                m_hHandle = semget( iKey, 1, 0666 | IPC_CREAT | IPC_EXCL );
                if( -1 == m_hHandle )
                {
                    m_hHandle = 0;
                    m_bConstructResult = false;
                    TADD_WARNING("semget(%s,%d,0666|IPC_CREAT) RETURN=[False],ErrorCode=%d\n", sIpcPath.c_str(), iKey,errno);
                    return;
                }
                m_bSelfCreated = true;
            }
#endif
            if(m_bConstructResult)
            {
                TMdbNtcSysNameObjectHelper::SignIn(this);
            }
            else if(m_bSelfCreated)
            {
                ManualFree();
            }
        }

        TMdbNtcProcessEvent::~TMdbNtcProcessEvent()
        {
#ifdef OS_WINDOWS
            if(m_hHandle)
            {
                CloseHandle(m_hHandle);
            }
#endif // OS_WINDOWS
        }

        //UNIX下说白了，就是P操作
        bool TMdbNtcProcessEvent::Wait(int iMilliSeconds)
        {
            int iRet = 0;
            TMdbNtcThread* pCurThread = mdb_ntc_zthread();
            if(pCurThread)
            {
                //设置挂起状态和线程状态
                pCurThread->pBlockingEvent = this;
                pCurThread->SetThreadState(MDB_NTC_THREAD_SUSPEND);
                if(pCurThread->TestCancel())
                {
                    pCurThread->SetThreadState(MDB_NTC_THREAD_RUN);
                    return false;
                }
            }
#ifdef OS_WINDOWS
            //直到hHandle成为发信号状态时才返
            int wait_time =( iMilliSeconds == -1) ? INFINITE: iMilliSeconds ;
            iRet = WaitForSingleObject(m_hHandle, wait_time);
            if(iRet != WAIT_OBJECT_0)
            {
                switch(iRet)
                {
                case WAIT_ABANDONED:
                    iRet = -1;
                    break;
                case WAIT_TIMEOUT:
                    iRet = -1;
                    break;
                default:
                    iRet = -1;
                    break;
                }
            }
#else
            struct sembuf buf_p;
            buf_p.sem_num = 0; //信号量数量
            buf_p.sem_op = -1;
            buf_p.sem_flg = SEM_UNDO;
            if( iMilliSeconds < 0 )
            {
                __TRYAGAIN_TProcessEvent:
                iRet=semop(m_hHandle, &buf_p, 1 ); //p操作,semop的第三个参数：sembuf结构变量的个数，通常为1
                if( 0 != iRet )
                {
                    if( errno == EINTR )
                    {
                         goto __TRYAGAIN_TProcessEvent;
                    }
                }
            }
            else if(iMilliSeconds == 0)
            {
                buf_p.sem_flg |= IPC_NOWAIT;
                iRet=semop(m_hHandle, &buf_p, 1 );
            }
            else
            {
        #ifdef OS_FREEBSD
                //freebsd不支持semtimedop
                iRet=semop(m_hHandle, &buf_p, 1 ); //p操作,semop的第三个参数：sembuf结构变量的个数，通常为1
        #else
                MDB_NTC_MAKE_SEM_TIMEOUT(timeout, iMilliSeconds);
                iRet=semtimedop(m_hHandle, &buf_p, 1, &timeout);
        #endif
            }
#endif
            if(pCurThread)
            {
                //设置运行状态
                pCurThread->SetThreadState(MDB_NTC_THREAD_RUN);
                pCurThread->pBlockingEvent = NULL;
            }
            return iRet==0;
        }

        bool TMdbNtcProcessEvent::SetEvent()
        {
#ifdef OS_WINDOWS
            /*
              设置事件的状态为有标记，释放任意等待线程。
              此事件将保持有标记，直到一个线程被释放，系统将设置事件的状态为无标记；如果没有线程在等待，
            则此事件将保持有标记，直到一个线程被释放。
            */
            if( !::SetEvent(m_hHandle) )//对于自动事件，由系统重置为无信号状态，除非一个线程被释放
            {
                return false;
            }
#else
            union semun semopts;
            semopts.val = 1;//将取值重置为1，这样可以有线程获取到
            //联合体中val成员的值设置信号量集合中单个信号量的值
            if( -1 == semctl(m_hHandle, 0, SETVAL, semopts ) )
            {
                return false;
            }
#endif
            return true;
        }

        bool TMdbNtcProcessEvent::PulseEvent()
        {
#ifdef OS_WINDOWS
            /*
            PulseEvent()是一个比较有意思的使用方法,正如这个API的名字,它使一个Event
            对象的状态发生一次脉冲变化,从无信号变成有信号再变成无信号,而整个操作是原子的.
            对自动复位的Event对象,它仅释放第一个等到该事件的thread（如果有),而对于人工复位的Event对象,它释放所有等待的thread.
            */
            if(!::PulseEvent(m_hHandle))
            {
                return false;
            }
#else
            int semncnt = semctl(m_hHandle, 0, GETNCNT, 0);
            if(semncnt == -1)
            {
                TADD_WARNING("semctl(%d,GETNCNT) RETURN=[False],ErrorCode=%d\n", m_hHandle,errno);
                return false;
            }
            else if(semncnt > 0)
            {
                union semun semopts;
                semopts.val = semncnt;//将取值重置为等待的数目，这样可以所有线程获取到
                //联合体中val成员的值设置信号量集合中单个信号量的值
                if( -1 == semctl(m_hHandle, 0, SETVAL, semopts ) )
                {
                    return false;
                }
            }
#endif
            return true;
        }

        bool TMdbNtcProcessEvent::ManualFree()
        {
#ifdef OS_WINDOWS
            if(m_hHandle && !CloseHandle(m_hHandle))
            {
                return false;
            }
            m_hHandle = NULL;
#else
            if(m_hHandle)
            {
                if( semctl(m_hHandle, 0, IPC_RMID, 0 ) == -1 )
                {
                    TADD_WARNING("semctl(%d,IPC_RMID) RETURN=[False],ErrorCode=%d\n", m_hHandle,errno);
                    return false;
                }
                m_hHandle = 0;
            }
#endif
            return TMdbNtcSysNameObjectHelper::Free(this);
        }

        bool TMdbNtcProcessEvent::CheckExist(const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return false;
            }
#ifdef OS_WINDOWS
            MDB_IPC_HANDLE hHandle = OpenSemaphore(SEMAPHORE_ALL_ACCESS,false, sIpcPath.c_str());
            if(hHandle)
            {
                CloseHandle(hHandle);
                hHandle = NULL;
                return true;
            }
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                MDB_IPC_HANDLE hHandle = semget(iNameKey,1,0666);
                if(hHandle>=0)
                {
                    return true;
                }
            }
#endif
            return false;
        }

        bool TMdbNtcProcessEvent::Destroy(const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return false;
            }
#ifdef OS_WINDOWS
            //当引用计数变为0时，系统自动释放锁占用的资源。
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                MDB_IPC_HANDLE hHandle = semget(iNameKey,1,0666);
                if(hHandle>=0)
                {
                    semctl(hHandle,0,IPC_RMID,0);
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
#endif
            return TMdbNtcSysNameObjectHelper::Free(&oRuntimeObject, pszName, pszInstEnvName);
        }

        int TMdbNtcProcessEvent::GetNCnt(const char* pszName, const char* pszInstEnvName)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return -1;
            }
#ifdef OS_WINDOWS
            //先空着
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                MDB_IPC_HANDLE hHandle = semget(iNameKey,1,0666);
                if(hHandle>=0)
                {
                    int iRslt = semctl(hHandle, 0, GETNCNT);
                    if(iRslt == -1)
                    {
                        return -1;
                    }
                    else
                    {
                        return iRslt;
                    }
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }

#endif
        }

        int TMdbNtcProcessEvent::GetVal(const char* pszName, const char* pszInstEnvName)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return -1;
            }
#ifdef OS_WINDOWS
            //先空着
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                MDB_IPC_HANDLE hHandle = semget(iNameKey,1,0666);
                if(hHandle>=0)
                {
                    int iRslt = semctl(hHandle, 0, GETVAL);
                    if(iRslt == -1)
                    {
                        return -1;
                    }
                    else
                    {
                        return iRslt;
                    }
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }

#endif
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcThreadEvent, TMdbNtcComponentObject);
        TMdbNtcThreadEvent::TMdbNtcThreadEvent()
        {
            m_uiWaitCount = 0;
        #ifdef OS_WINDOWS
            m_hHandle = CreateEvent(NULL, FALSE, FALSE, NULL); //第三个参数有信号、无信号，TRUE OR FALSE ？[杜]
            m_bConstructResult = m_hHandle != NULL;
        #else
            pthread_cond_init(&m_oCondition, NULL);
            m_bSignaled = false;
            m_bConstructResult = true;
        #endif
        }

        TMdbNtcThreadEvent::~TMdbNtcThreadEvent()
        {
        #ifdef OS_WINDOWS
            CloseHandle(m_hHandle);
        #else
            pthread_cond_destroy(&m_oCondition);
        #endif
        }

        bool TMdbNtcThreadEvent::Wait(int iMilliSeconds /* = -1 */)
        {
            int iRet = 0;
            TMdbNtcThread* pCurThread = mdb_ntc_zthread();
            if(pCurThread)
            {
                //设置挂起状态和线程状态
                pCurThread->pBlockingEvent = this;
                pCurThread->SetThreadState(MDB_NTC_THREAD_SUSPEND);
                if(pCurThread->TestCancel())
                {
                    pCurThread->SetThreadState(MDB_NTC_THREAD_RUN);
                    return false;
                }
            }
        #ifdef OS_WINDOWS
            m_oMutex.Lock();
            ++m_uiWaitCount;
            m_oMutex.Unlock();
            //直到hHandle成为发信号状态时才返
            int wait_time =( iMilliSeconds == -1) ? INFINITE: iMilliSeconds ;
            iRet = WaitForSingleObject(m_hHandle, wait_time);
            m_oMutex.Lock();
            --m_uiWaitCount;
            m_oMutex.Unlock();
            if(iRet != WAIT_OBJECT_0)
            {
                switch(iRet)
                {
                case WAIT_ABANDONED:
                    iRet = -1;
                    break;
                case WAIT_TIMEOUT:
                    iRet = -1;
                    break;
                default:
                    iRet = -1;
                    break;
                }
            }
        #else
            m_oMutex.Lock();
            do
            {
                //加锁后，仍然要判断是否已经是有信号状态了(状态可能是在获得锁的过程中修改了)
                if(m_bSignaled)//如果当前已经是有信号状态
                {
                    m_bSignaled = false;
                    break;
                }
                ++m_uiWaitCount;
                if(iMilliSeconds < 0)
                {
                    iRet = pthread_cond_wait(&m_oCondition, &m_oMutex.GetMutex());
                }
                else
                {
                    MDB_NTC_MAKE_TIMEOUT(timeout, iMilliSeconds);
                    iRet = pthread_cond_timedwait(&m_oCondition, &m_oMutex.GetMutex(), &timeout);
                }
                --m_uiWaitCount;
                //获得事件后，对于自动复位事件，需要重置信号状态
                if(m_bSignaled)
                {
                    m_bSignaled = false;
                }
            } while (0);
            m_oMutex.Unlock();
        #endif
            if(pCurThread)
            {
                //设置运行状态
                pCurThread->SetThreadState(MDB_NTC_THREAD_RUN);
                pCurThread->pBlockingEvent = NULL;
            }
            return iRet==0?true:false;
        }

        bool TMdbNtcThreadEvent::SetEvent()
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            /*
              设置事件的状态为有标记，释放任意等待线程。
              如果事件是手工的，此事件将保持有标记直到调用ResetEvent，这种情况下将释放多个线程；
              如果事件是自动的，此事件将保持有标记，直到一个线程被释放，系统将设置事件的状态为无标记；如果没有线程在等待，
            则此事件将保持有标记，直到一个线程被释放。
            */
            if( !::SetEvent(m_hHandle) )//对于自动事件，由系统重置为无信号状态，除非一个线程被释放
            {
                return false;
            }
        #else
            m_oMutex.Lock();
            //对于自动事件的false参数，除非有一个等待触发，条件变量还不支持（因为条件变量只针对等待中的队列），
            if(m_uiWaitCount > 0)
            {
                iRet = pthread_cond_signal(&m_oCondition);
            }
            else
            {
                m_bSignaled = true;//这样不管怎么样，都会除非有一个等待触发,信号才会自动复位
            }
            m_oMutex.Unlock();
        #endif
            return iRet==0?true:false;
        }

        bool TMdbNtcThreadEvent::PulseEvent()
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            /*
            PulseEvent()是一个比较有意思的使用方法,正如这个API的名字,它使一个Event
            对象的状态发生一次脉冲变化,从无信号变成有信号再变成无信号,而整个操作是原子的.
            对自动复位的Event对象,它仅释放第一个等到该事件的thread（如果有),而对于人工复位的Event对象,它释放所有等待的thread.
            */
            if(!::PulseEvent(m_hHandle))
            {
                return false;
            }
        #else
            m_oMutex.Lock();
            if(m_uiWaitCount > 0)
            {
                iRet = pthread_cond_signal(&m_oCondition);
            }
            m_oMutex.Unlock();
        #endif
            return iRet==0?true:false;
        }


        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcThreadCond, TMdbNtcComponentObject);
        TMdbNtcThreadCond::TMdbNtcThreadCond(TMdbNtcThreadLock  *pMutexLock)
        {
            m_pMutex = pMutexLock;
        #ifdef OS_WINDOWS
            m_hHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
            m_bConstructResult = m_hHandle!=NULL;
        #else
            pthread_cond_init(&m_oCondition, NULL);
            m_bConstructResult = true;
        #endif
            m_uiWaitCount = 0;
        
        }

        TMdbNtcThreadCond::~TMdbNtcThreadCond()
        {
        #ifdef OS_WINDOWS
            CloseHandle(m_hHandle);
        #else
            pthread_cond_destroy(&m_oCondition);
        #endif
        }


        bool TMdbNtcThreadCond::Lock()
        {
             if(m_pMutex)
             {
                 return m_pMutex->Lock();
             }
             else
             {
                 return false;
             }
        }

        bool TMdbNtcThreadCond::Unlock()
        {
             if(m_pMutex)
             {
                 return m_pMutex->Unlock();
             }
             else
             {
                 return false;
             }
        }

        bool TMdbNtcThreadCond::Wait(int iMilliSeconds /*= -1*/)
        {
            int iRet = 0;

            TMdbNtcThread* pCurThread = mdb_ntc_zthread();
            if(pCurThread)
            {
                //设置挂起状态和线程状态
                pCurThread->pBlockingEvent = this;
                pCurThread->SetThreadState(MDB_NTC_THREAD_SUSPEND);
                if(pCurThread->TestCancel())
                {
                    pCurThread->SetThreadState(MDB_NTC_THREAD_RUN);
                    pCurThread->pBlockingEvent = NULL;
                    return false;
                }
            }

        #ifdef OS_WINDOWS
            //m_oMutex->Lock();
            ++m_uiWaitCount;
            //m_oMutex->Unlock();
            //直到hHandle成为发信号状态时才返
            int wait_time =( iMilliSeconds == -1) ? INFINITE: iMilliSeconds ;
            iRet = WaitForSingleObject(m_hHandle, wait_time);
            //m_oMutex.Lock();
            --m_uiWaitCount;
            //m_oMutex.Unlock();
            if(iRet != WAIT_OBJECT_0)
            {
                switch(iRet)
                {
                case WAIT_ABANDONED:
                    iRet = -1;
                    break;
                case WAIT_TIMEOUT:
                    iRet = -1;
                    break;
                default:
                    iRet = -1;
                    break;
                }
            }
        #else
            //m_oMutex.Lock();
            ++m_uiWaitCount;
            //m_oMutex.Unlock();
            if(-1 == iMilliSeconds)
            {
                iRet = pthread_cond_wait(&m_oCondition, &m_pMutex->GetMutex());
            }
            else
            {
                MDB_NTC_MAKE_TIMEOUT(timeout, iMilliSeconds);
                iRet = pthread_cond_timedwait(&m_oCondition, &m_pMutex->GetMutex(), &timeout);
            }
            //m_oMutex.Lock();
            --m_uiWaitCount;
            //m_oMutex.Unlock();
        #endif
            if(pCurThread)
            {
                //设置运行状态
                pCurThread->SetThreadState(MDB_NTC_THREAD_RUN);
                pCurThread->pBlockingEvent = NULL;
            }
            return iRet == 0;
        }

        bool TMdbNtcThreadCond::SetEvent()
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            if( !::SetEvent(m_hHandle) )
            {
                return false;
            }
        #else
            iRet = pthread_cond_signal(&m_oCondition);
        #endif
            return iRet==0?true:false;
        }

        bool TMdbNtcThreadCond::PulseEvent()
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            if( !::PulseEvent(m_hHandle) )
            {
                return false;
            }
        #else
            iRet = pthread_cond_signal(&m_oCondition);
        #endif
            return iRet==0?true:false;
        }


        bool TMdbNtcThreadCond::BroadCast()
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            if( !::SetEvent(m_hHandle) )
            {
                return false;
            }
        #else
            iRet = pthread_cond_broadcast(&m_oCondition);
        #endif
            return iRet==0?true:false;
        }

        void TMdbNtcThreadCond::SetLock(TMdbNtcThreadLock  *pMutexLock)
        {
            m_pMutex = pMutexLock;
        }


        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcProcessNotify, TMdbNtcSystemNameObject);
        TMdbNtcProcessNotify::TMdbNtcProcessNotify(const char* pszNotifyName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */)
            :TMdbNtcSystemNameObject(pszNotifyName, pszInstEnvName, pParentNameObject)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(this);
            if(sIpcPath.IsEmpty())
            {
                m_bConstructResult = false;
                TADD_WARNING( "GetIpcPath failed! name[%s]", pszNotifyName);
                return;
            }
#ifdef OS_WINDOWS
            m_hHandle = OpenEvent( EVENT_ALL_ACCESS, FALSE, sIpcPath.c_str() );
            if(m_hHandle==NULL)
            {
                m_hHandle = CreateEvent(NULL, TRUE, FALSE, sIpcPath.c_str()); //第三个参数有信号、无信号，TRUE OR FALSE ？[杜]
            }
            m_bConstructResult = m_hHandle!=NULL;
#else
            m_bConstructResult = true;
            key_t Key = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(Key == -1)
            {
                m_bConstructResult = false;
                TADD_WARNING("MakeIPCKey(%s) RETURN=[False],ErrorCode=%d\n", sIpcPath.c_str(),errno);
                return;
            }
            m_hHandle = semget (Key, 0, 0666);
            if(m_hHandle >= 0)
            {
                m_bSelfCreated = false;
            }
            else
            {
                m_hHandle = semget( Key, 1, 0666 | IPC_CREAT | IPC_EXCL );
                if( -1 == m_hHandle )
                {
                    m_hHandle = 0;
                    m_bConstructResult = false;
                    TADD_WARNING("semget(%d,0666 | IPC_CREAT) RETURN=[False],ErrorCode=%d\n", Key,errno);
                    return;
                }
                else
                {
                    m_bSelfCreated = true;
                    m_bConstructResult = Reset();//将信号量置为无信号状态。
                }
            }
#endif // OS_WINDOWS
            if(m_bConstructResult)
            {
                TMdbNtcSysNameObjectHelper::SignIn(this);
            }
            else if(m_bSelfCreated)
            {
                ManualFree();
            }
        }

        TMdbNtcProcessNotify::~TMdbNtcProcessNotify()
        {

        }

        //UNIX下说白了，就是P操作
        bool TMdbNtcProcessNotify::Wait(int iMilliSeconds)
        {
            int iRet = 0;
            TMdbNtcThread* pCurThread = mdb_ntc_zthread();
            if(pCurThread)
            {
                //设置挂起状态和线程状态
                pCurThread->pBlockingNotify = this;
                pCurThread->SetThreadState(MDB_NTC_THREAD_SUSPEND);
                if(pCurThread->TestCancel())
                {
                    pCurThread->SetThreadState(MDB_NTC_THREAD_RUN);
                    return false;
                }
            }
#ifdef OS_WINDOWS
            //直到hHandle成为发信号状态时才返
            int wait_time =( iMilliSeconds == -1) ? INFINITE: iMilliSeconds ;
            iRet = WaitForSingleObject(m_hHandle, wait_time);
            if(iRet != WAIT_OBJECT_0)
            {
                switch(iRet)
                {
                case WAIT_ABANDONED:
                    iRet = -1;
                    break;
                case WAIT_TIMEOUT:
                    iRet = -1;
                    break;
                default:
                    iRet = -1;
                    break;
                }
            }
#else
            //只等待信号量值变为0
            struct sembuf buf_p;
            buf_p.sem_num = 0; //信号量数量
            buf_p.sem_op = 0;
            buf_p.sem_flg = SEM_UNDO;
            if( iMilliSeconds < 0 )
            {
                __TRYAGAIN_TProcessNotify:
                iRet=semop(m_hHandle, &buf_p, 1 ); //p操作,semop的第三个参数：sembuf结构变量的个数，通常为1
                if( 0 != iRet )
                {
                    if( errno == EINTR )
                    {
                         goto __TRYAGAIN_TProcessNotify;
                    }
                }
            }
            else if(iMilliSeconds == 0)
            {
                buf_p.sem_flg |= IPC_NOWAIT;
                iRet=semop(m_hHandle, &buf_p, 1 );
            }
            else
            {
        #ifdef OS_FREEBSD
                //freebsd不支持semtimedop
                iRet=semop(m_hHandle, &buf_p, 1 ); //p操作,semop的第三个参数：sembuf结构变量的个数，通常为1
        #else
                MDB_NTC_MAKE_SEM_TIMEOUT(timeout, iMilliSeconds);
                iRet=semtimedop(m_hHandle, &buf_p, 1, &timeout);
        #endif
            }
#endif
            if(pCurThread)
            {
                //设置运行状态
                pCurThread->SetThreadState(MDB_NTC_THREAD_RUN);
                pCurThread->pBlockingNotify = NULL;
            }
            return iRet==0;
        }

        bool TMdbNtcProcessNotify::Notify()
        {
#ifdef OS_WINDOWS
            /*
              设置事件的状态为有标记，释放任意等待线程。
              如果事件是手工的，此事件将保持有标记直到调用ResetEvent，这种情况下将释放多个线程；
              如果事件是自动的，此事件将保持有标记，直到一个线程被释放，系统将设置事件的状态为无标记；如果没有线程在等待，
            则此事件将保持有标记，直到一个线程被释放。
            */
            if( !::SetEvent(m_hHandle) )//对于自动事件，由系统重置为无信号状态，除非一个线程被释放
            {
                return false;
            }
#else
            union semun semopts;
            semopts.val = 0;//将信号量值变为0，这样等待的线程就可以立刻感知到了。
            //联合体中val成员的值设置信号量集合中单个信号量的值
            if( -1 == semctl(m_hHandle, 0, SETVAL, semopts ) )
            {
                return false;
            }
#endif
            return true;
        }

        bool TMdbNtcProcessNotify::Reset()
        {
#ifdef OS_WINDOWS
            if(! ResetEvent(m_hHandle) )
            {
                return false;
            }
#else
            union semun semopts;
            semopts.val = 1;//将取值重置为1，这样就那些等待取值为0的线程，就会阻塞。
            //联合体中val成员的值设置信号量集合中单个信号量的值
            if( -1 == semctl(m_hHandle, 0, SETVAL, semopts ) )
            {
                return false;
            }
#endif
            return true;
        }

        bool TMdbNtcProcessNotify::ManualFree()
        {
#ifdef OS_WINDOWS
            if(m_hHandle && ! CloseHandle(m_hHandle))
            {
                return false;
            }
            m_hHandle = NULL;
#else
            if(m_hHandle)
            {
                if( semctl(m_hHandle, 0, IPC_RMID, 0 ) == -1 )
                {
                    TADD_WARNING("semctl(%d,IPC_RMID) RETURN=[False],ErrorCode=%d\n", m_hHandle,errno);
                    return false;
                }
                m_hHandle = 0;
            }
#endif
            return TMdbNtcSysNameObjectHelper::Free(this);;
        }

        bool TMdbNtcProcessNotify::CheckExist(const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return false;
            }
#ifdef OS_WINDOWS
            MDB_IPC_HANDLE hHandle = OpenSemaphore(SEMAPHORE_ALL_ACCESS,false, sIpcPath.c_str());
            if(hHandle)
            {
                CloseHandle(hHandle);
                hHandle = NULL;
                return true;
            }
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                MDB_IPC_HANDLE hHandle = semget(iNameKey,1,0666);
                if(hHandle>=0)
                {
                    return true;
                }
            }
#endif
            return false;
        }

        bool TMdbNtcProcessNotify::Destroy(const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return false;
            }
#ifdef OS_WINDOWS
            //当引用计数变为0时，系统自动释放锁占用的资源。
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                MDB_IPC_HANDLE hHandle = semget(iNameKey,1,0666);
                if(hHandle>=0)
                {
                    semctl(hHandle,0,IPC_RMID,0);
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
#endif
            return TMdbNtcSysNameObjectHelper::Free(&oRuntimeObject, pszName, pszInstEnvName);
        }

        int TMdbNtcProcessNotify::GetNCnt(const char* pszName, const char* pszInstEnvName)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return -1;
            }
#ifdef OS_WINDOWS
            //先空着
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                MDB_IPC_HANDLE hHandle = semget(iNameKey,1,0666);
                if(hHandle>=0)
                {
                    int iRslt = semctl(hHandle, 0, GETNCNT);
                    if(iRslt == -1)
                    {
                        return -1;
                    }
                    else
                    {
                        return iRslt;
                    }
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }

#endif
        }

        int TMdbNtcProcessNotify::GetVal(const char* pszName, const char* pszInstEnvName)
        {
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return -1;
            }
#ifdef OS_WINDOWS
            //先空着
#else
            int iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                MDB_IPC_HANDLE hHandle = semget(iNameKey,1,0666);
                if(hHandle>=0)
                {
                    int iRslt = semctl(hHandle, 0, GETVAL);
                    if(iRslt == -1)
                    {
                        return -1;
                    }
                    else
                    {
                        return iRslt;
                    }
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                return -1;
            }

#endif
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcThreadNotify, TMdbNtcComponentObject);
        TMdbNtcThreadNotify::TMdbNtcThreadNotify()
        {
            m_uiWaitCount = 0;
        #ifdef OS_WINDOWS
            m_hHandle = CreateEvent(NULL, FALSE, FALSE, NULL); //第三个参数有信号、无信号，TRUE OR FALSE ？[杜]
            m_bConstructResult = m_hHandle!=NULL;
        #else
            pthread_cond_init(&m_oCondition, NULL);
            m_bSignaled = false;
            m_bConstructResult = true;
        #endif
        }

        TMdbNtcThreadNotify::~TMdbNtcThreadNotify()
        {
        #ifdef OS_WINDOWS
            CloseHandle(m_hHandle);
        #else
            pthread_cond_destroy(&m_oCondition);
        #endif
        }

        bool TMdbNtcThreadNotify::Wait(int iMilliSeconds /* = -1 */)
        {
            int iRet = 0;
            TMdbNtcThread* pCurThread = mdb_ntc_zthread();
            if(pCurThread)
            {
                //设置挂起状态和线程状态
                pCurThread->pBlockingNotify = this;
                pCurThread->SetThreadState(MDB_NTC_THREAD_SUSPEND);
                if(pCurThread->TestCancel())
                {
                    pCurThread->SetThreadState(MDB_NTC_THREAD_RUN);
                    return false;
                }
            }
        #ifdef OS_WINDOWS
            m_oMutex.Lock();
            ++m_uiWaitCount;
            m_oMutex.Unlock();
            //直到hHandle成为发信号状态时才返
            int wait_time =( iMilliSeconds == -1) ? INFINITE: iMilliSeconds ;
            iRet = WaitForSingleObject(m_hHandle, wait_time);
            m_oMutex.Lock();
            --m_uiWaitCount;
            m_oMutex.Unlock();
            if(iRet != WAIT_OBJECT_0)
            {
                switch(iRet)
                {
                case WAIT_ABANDONED:
                    iRet = -1;
                    break;
                case WAIT_TIMEOUT:
                    iRet = -1;
                    break;
                default:
                    iRet = -1;
                    break;
                }
            }
        #else
            if(!m_bSignaled)//如果当前无信号，则等待
            {
                m_oMutex.Lock();
                //拿到锁后，仍需判断下，因为有可能在锁阻塞时，已经调用了Notify
                if(!m_bSignaled)//如果当前无信号，则等待
                {
                    do
                    {
                        ++m_uiWaitCount;
                        if(iMilliSeconds < 0)
                        {
                            iRet = pthread_cond_wait(&m_oCondition, &m_oMutex.GetMutex());
                        }
                        else
                        {
                            MDB_NTC_MAKE_TIMEOUT(timeout, iMilliSeconds);
                            iRet = pthread_cond_timedwait(&m_oCondition, &m_oMutex.GetMutex(), &timeout);
                        }
                        --m_uiWaitCount;
                    } while (0);
                }
                m_oMutex.Unlock();
            }
        #endif
            if(pCurThread)
            {
                //设置运行状态
                pCurThread->SetThreadState(MDB_NTC_THREAD_RUN);
                pCurThread->pBlockingNotify = NULL;
            }
            return iRet==0?true:false;
        }

        bool TMdbNtcThreadNotify::Notify()
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            /*
              设置事件的状态为有标记，释放任意等待线程。
              如果事件是手工的，此事件将保持有标记直到调用ResetEvent，这种情况下将释放多个线程；
              如果事件是自动的，此事件将保持有标记，直到一个线程被释放，系统将设置事件的状态为无标记；如果没有线程在等待，
            则此事件将保持有标记，直到一个线程被释放。
            */
            if( !::SetEvent(m_hHandle) )//对于自动事件，由系统重置为无信号状态，除非一个线程被释放
            {
                return false;
            }
        #else
            m_oMutex.Lock();
            m_bSignaled = true;
            if(m_uiWaitCount > 0)
            {
                iRet = pthread_cond_broadcast(&m_oCondition);
            }
            m_oMutex.Unlock();
        #endif
            return iRet==0?true:false;
        }

        bool TMdbNtcThreadNotify::Reset()
        {
        #ifdef OS_WINDOWS
            /*
            PulseEvent()是一个比较有意思的使用方法,正如这个API的名字,它使一个Event
            对象的状态发生一次脉冲变化,从无信号变成有信号再变成无信号,而整个操作是原子的.
            对自动复位的Event对象,它仅释放第一个等到该事件的thread（如果有),而对于人工复位的Event对象,它释放所有等待的thread.
            */
            if(!::ResetEvent(m_hHandle))
            {
                return false;
            }
        #else
            m_oMutex.Lock();
            m_bSignaled = false;
            m_oMutex.Unlock();
        #endif
            return true;
        }

        //TMdbNtcAutoLock
        //==========================================================================
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcAutoLock, TMdbNtcBaseObject);
        TMdbNtcAutoLock::TMdbNtcAutoLock(TMdbNtcBaseLock *pLock)
        {
            m_bConstructResult = true;
            m_pLock = pLock;
            if(m_pLock)
            {
                m_pLock->Lock();
            }
        }

        TMdbNtcAutoLock::~TMdbNtcAutoLock()
        {
            if(m_pLock)
            {
                m_pLock->Unlock();
            }
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcThreadSpinLock, TMdbNtcComponentObject);
        TMdbNtcThreadSpinLock::TMdbNtcThreadSpinLock()
        {
        #ifdef OS_WINDOWS
            InitializeCriticalSectionAndSpinCount(&m_spin, 0x80000FA0);//初始化自旋锁
        #elif defined(OS_SUN)
            pthread_mutex_init(&m_spin,NULL);
        #elif defined(OS_HP)
            pthread_mutex_init(&m_spin,NULL);
        #else
            int iRet = pthread_spin_init(&m_spin, PTHREAD_PROCESS_PRIVATE);
            if(iRet != 0)
            {
                m_bConstructResult = false;
                TADD_WARNING("Create SpinLock Failed! Error:%s",strerror(iRet));
                return;
            }
        #endif
            m_bConstructResult = true;
        }

        TMdbNtcThreadSpinLock::~TMdbNtcThreadSpinLock()
        {
        #ifdef OS_WINDOWS
            DeleteCriticalSection(&m_spin);
        #elif defined(OS_SUN)
            pthread_mutex_destroy(&m_spin);
        #elif defined(OS_HP)
            pthread_mutex_destroy(&m_spin);
        #else
            pthread_spin_destroy(&m_spin);
        #endif
        }

        bool TMdbNtcThreadSpinLock::Lock(int iMilliSeconds)
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            EnterCriticalSection(&m_spin);
        #elif defined(OS_SUN)
            iRet = pthread_mutex_lock(&m_spin);
        #elif defined(OS_HP)
            iRet = pthread_mutex_lock(&m_spin);
        #else
            iRet = pthread_spin_lock(&m_spin);
        #endif
            return iRet==0?true:false;
        }

        bool TMdbNtcThreadSpinLock::TryLock()
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            iRet = TryEnterCriticalSection(&m_spin)?0:-1;
        #elif defined(OS_SUN)
            iRet = pthread_mutex_trylock(&m_spin);
        #elif defined(OS_HP)
            iRet = pthread_mutex_trylock(&m_spin);
        #else
            iRet = pthread_spin_trylock(&m_spin);
        #endif
            return iRet==0?true:false;
        }

        bool TMdbNtcThreadSpinLock::Unlock()
        {
            int iRet = 0;
        #ifdef OS_WINDOWS
            LeaveCriticalSection(&m_spin);
        #elif defined(OS_SUN)
            iRet = pthread_mutex_unlock(&m_spin);
        #elif defined(OS_HP)
            iRet = pthread_mutex_unlock(&m_spin);
        #else
            iRet = pthread_spin_unlock(&m_spin);
        #endif
            return iRet==0?true:false;
        }
//}
