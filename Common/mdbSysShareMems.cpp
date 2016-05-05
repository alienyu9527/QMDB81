/**
* @file SysShareMems.cxx
* @brief 共享内存操作类
*
* 封装了共享内存的常用操作
*
* @author 技术框架组
* @version 1.0
* @date 20121214
* @warning 请统一使用该类进行共享内存的操作
*/

#include "Common/mdbSysShareMems.h"
#include "Common/mdbStrUtils.h"
#include "Common/mdbFileUtils.h"
//#include "Common/mdbLogInterface.h"
#ifndef OS_WINDOWS
    #include <sys/shm.h>
#else

#endif

//namespace QuickMDB
//{
        const MDB_UINT8 g_uiMdbNtcShmStructVer = 1;///< 当前的共享内存头信息结构的版本，如果结构发生变化，递增
        /*
        * @brief 共享内存信息类
        *
        * 存储共享内存信息
        */
        class TMdbNtcShareMemDataInfo
        {
        public:
            inline void InitMemData(const char* pszName, MDB_UINT64 uiShmSize)
            {
                uiShmStructVer = g_uiMdbNtcShmStructVer;
                MdbNtcStrcpy(szName, pszName, sizeof(szName));
                uiSize = uiShmSize;
                bInvalid = 0;
            }
            inline unsigned char *GetMemData( )
            {
                return ((unsigned char *)this) + sizeof(TMdbNtcShareMemDataInfo);
            }
        public:
#ifdef OS_WINDOWS
            MDB_UINT32  uiDeamonPid;    ///< 守护进程的pid
#endif
            MDB_UINT8   uiShmStructVer; ///< 共享内存头信息结构的版本，需与g_uiShmStructVer一致，避免不同版本对接造成数据紊乱
            MDB_UINT8   bInvalid;       ///< 是否已经无效
            char    szName[32];     ///< 共享内存名称
            MDB_UINT64  uiSize;         ///< 共享内存大小
        };

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcShareMem, TMdbNtcSystemNameObject);
        TMdbNtcShareMem::TMdbNtcShareMem(const char *pszName, unsigned long uiSize /* = 0 */,
            const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */,
            bool bCheckVersion /* = true */)
            :TMdbNtcSystemNameObject(pszName, pszInstEnvName, pParentNameObject)
        {            
#ifdef OS_WINDOWS
            m_iHandle = 0;
#else
            m_iHandle = -1;
#endif
            m_pFileView = NULL;
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(this);
            if(sIpcPath.IsEmpty())
            {
                m_bConstructResult = false;
                TADD_WARNING( "GetIpcPath failed! name[%s]", pszName);
                return;
            }
            do 
            {
#ifdef  OS_WINDOWS
                m_iHandle = OpenFileMapping( FILE_MAP_ALL_ACCESS, false, sIpcPath.c_str() );
                if( m_iHandle == NULL )
                {
                    if(uiSize == 0)
                    {
                        m_bConstructResult = false;
                        break;
                    }
                    m_iHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, uiSize+sizeof(TMdbNtcShareMemDataInfo), sIpcPath.c_str() );
                    if( m_iHandle == NULL )
                    {
                        m_bConstructResult = false;
                        TADD_WARNING( "CreateFileMapping(Name=%s) RETURN=[NULL]", sIpcPath.c_str() );
                        break;
                    }
                    m_bSelfCreated = true;
                }
                m_pFileView = MapViewOfFile( m_iHandle, FILE_MAP_WRITE, 0, 0, 0 );
                if( m_pFileView == NULL )
                {
                    m_bConstructResult = false;
                    TADD_WARNING( "MapViewOfFile(Name=%s,Handle=%"MDB_NTC_ZS_FORMAT_INT64") RETURN=[NULL]", sIpcPath.c_str(), m_iHandle );
                    break;
                }
                if(m_bSelfCreated)
                {
                    reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->InitMemData(pszName, uiSize );                    
                    //启动一个守护进程                    
                    TCHAR szPath[MAX_PATH];
                    if(!GetModuleFileName( NULL, szPath, MAX_PATH))
                    {
                        m_bConstructResult = false;
                        TADD_WARNING( "Get module path failed");
                        break;
                    }
                    HANDLE   hRead = NULL, hWrite = NULL;
                    SECURITY_ATTRIBUTES   sa;
                    sa.nLength   =   sizeof(SECURITY_ATTRIBUTES);     
                    sa.lpSecurityDescriptor   =   NULL;     
                    sa.bInheritHandle   =   TRUE;     
                    if(!CreatePipe(&hRead, &hWrite, &sa, 0))       
                    {
                        m_bConstructResult = false;
                        TADD_WARNING( "Create Pipe failed");
                        break;
                    }
                    do 
                    {
                        STARTUPINFO   si;
                        PROCESS_INFORMATION   pi;  
                        si.cb   =   sizeof(STARTUPINFO);
                        GetStartupInfo(&si);       
                        si.hStdError    =   hWrite;     
                        si.hStdOutput   =   hWrite;
                        si.wShowWindow  =   SW_HIDE;     
                        si.dwFlags      =   STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
                        TMdbNtcStringBuffer sCmdLine;
                        sCmdLine.Snprintf(1024, "%s\\shm_deamon.exe \"%s\" \"%s\"",
                            TMdbNtcFileOper::GetParentPath(szPath).c_str(), pszName, pszInstEnvName);
                        if(!CreateProcess(NULL, sCmdLine.GetBuffer(), NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS|DETACHED_PROCESS, NULL, NULL, &si, &pi))
                        {
                            m_bConstructResult = false;
                            TADD_WARNING( "Create deamon process failed");
                            break;
                        }
                        CloseHandle(hWrite);
                        hWrite = NULL;
                        if(pi.hProcess)
                        {
                            CloseHandle(pi.hProcess);
                            pi.hProcess = NULL;
                        }
                        if(pi.hThread)
                        {
                            CloseHandle(pi.hThread);
                            pi.hThread = NULL;
                        }
                        //读取子进程信息
                        char   szBuffer[1024]   =   {0};     
                        DWORD  uiBytesRead = 0;
                        if(!ReadFile(hRead, szBuffer, sizeof(szBuffer)-1, &uiBytesRead, NULL))
                        {
                            m_bConstructResult = false;
                            TADD_WARNING( "Read deamon process's info failed");
                            break;
                        }
                        else if(TMdbNtcStrFunc::StrPrefix(szBuffer, "fail"))
                        {
                            m_bConstructResult = false;
                            TADD_WARNING( "Error in deamon process[%s]", szBuffer);
                            break;
                        }
                        reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->uiDeamonPid = pi.dwProcessId;
                    } while (0);
                    CloseHandle(hRead);
                    hRead = NULL;
                    if(hWrite)
                    {
                        CloseHandle(hWrite);
                        hWrite = NULL;
                    }                    
                }
#else
                m_iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
                if(m_iNameKey == -1)
                {
                    m_bConstructResult = false;
                    TADD_WARNING("MakeIPCKey(%s) RETURN=[False],ErrorCode=%d\n", sIpcPath.c_str(), errno);
                    break;
                }
                m_iHandle = shmget( m_iNameKey, 0, 0666 );
                if( m_iHandle >= 0 )
                {
                    m_bSelfCreated = false;
                }
                else if(uiSize > 0)
                {
                    m_iHandle = shmget( m_iNameKey, uiSize+sizeof(TMdbNtcShareMemDataInfo), 0666 | IPC_CREAT | IPC_EXCL );
                    if( m_iHandle < 0 )
                    {
                        m_bConstructResult = false;
                        TADD_WARNING( "ShareMemoryGet(Name=%s,Key=%d) ErrorCode=%d RETURN=[NULL]", sIpcPath.c_str(), m_iNameKey, errno );
                        break;
                    }
                    m_bSelfCreated = true;
                }
                else
                {
                    m_bConstructResult = false;
                    break;
                }
                m_pFileView = shmat( m_iHandle, 0, 0 );
                if( (MDB_UINT64)-1 == (MDB_UINT64)m_pFileView)
                {
                    m_bConstructResult = false;
                    TADD_WARNING( "ShareMemoryAttach(Name=%s,Key=%d) ErrorCode=%d RETURN=[NULL]", sIpcPath.c_str(), m_iNameKey, errno );
                    break;
                }
                if(m_bSelfCreated)
                {
                    reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->InitMemData(pszName, uiSize);
                }
#endif
            } while (0);
            //检查版本是否匹配
            if(bCheckVersion && m_bConstructResult && !m_bSelfCreated && reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->uiShmStructVer != g_uiMdbNtcShmStructVer)
            {
                m_bConstructResult = false;
                TADD_WARNING( "ShareMemoryAttach[%s] failed, because of unmatched version[%u vs %u].",
                    reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->szName, reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->uiShmStructVer, g_uiMdbNtcShmStructVer);
            }
            if(m_bConstructResult)
            {
                TMdbNtcSysNameObjectHelper::SignIn(this);
            }
            else
            {
                if(m_bSelfCreated) ManualFree();
                else
                {
#ifdef  OS_WINDOWS
                    if(m_pFileView!=NULL)
                    {
                        UnmapViewOfFile(m_pFileView);
                        m_pFileView = NULL;
                    }
                    if(m_iHandle)
                    {
                        CloseHandle(m_iHandle);
                        m_iHandle = NULL;
                    }
#else  
                    if(m_pFileView!=NULL)
                    {
#ifdef OS_SUN
                        shmdt((char*)m_pFileView);
#else
                        shmdt(m_pFileView);
#endif
                        m_pFileView = NULL;
                    }
                    m_iHandle = -1;
#endif
                }
            }
        }

        TMdbNtcShareMem::~TMdbNtcShareMem()
        {
            #ifdef  OS_WINDOWS
                if(m_pFileView!=NULL)
                {
                    UnmapViewOfFile(m_pFileView);
                    m_pFileView = NULL;
                }
                if(m_iHandle)
                {
                    CloseHandle(m_iHandle);
                    m_iHandle = NULL;
                }
            #else  
                if(m_pFileView!=NULL)
                {
                    #ifdef OS_SUN
                        shmdt((char*)m_pFileView);
                    #else
                        shmdt(m_pFileView);
                    #endif
                    m_pFileView = NULL;
                }
                m_iHandle = -1;
            #endif
            m_bConstructResult = false;
        }


        MDB_UINT64 TMdbNtcShareMem::GetSize() const
        {
            return m_pFileView==NULL ? 0:reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->uiSize;
        }

        unsigned char* TMdbNtcShareMem::GetBuffer()
        {
            return reinterpret_cast<unsigned char*>(m_pFileView)+sizeof(TMdbNtcShareMemDataInfo);
        }
        MDB_INT64 TMdbNtcShareMem::GetShmID()
        {
            return (MDB_INT64)m_iHandle;
        }

        bool TMdbNtcShareMem::ManualFree()
        {
            bool bRet = true;            
#ifdef  OS_WINDOWS
            if( m_pFileView != NULL )
            {
                MDB_UINT32 uiDeamonPid = reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->uiDeamonPid;
                reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->uiDeamonPid = 0;
                reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->bInvalid = 1;
                bRet = UnmapViewOfFile( m_pFileView )?true:false;
                m_pFileView = NULL;
                if(uiDeamonPid > 0)
                {
                    TMdbNtcSysUtils::KillProcess(uiDeamonPid);
                }
            }
            if( bRet && ( m_iHandle != NULL ) )
            {
                bRet = CloseHandle( m_iHandle )?true:false;
                m_iHandle = NULL;
            }
#else
            int iRet = 0;
            if( m_pFileView != NULL )
            {
                reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->bInvalid = 1;
#ifndef OS_SUN
                iRet = shmdt( m_pFileView );
#else
                iRet = shmdt( (char *)m_pFileView );
#endif

                if( iRet == -1 )
                {
                    bRet = false;
                }
                else
                {
                    m_pFileView = NULL;
                }
            }
            if( bRet && ( m_iHandle > -1 ) )
            {
                iRet = shmctl( m_iHandle, IPC_RMID, 0 );
                if( iRet == -1 )
                {
                    bRet = false;
                }
                else
                {
                    m_iHandle = -1;
                }
            }
#endif
            return bRet&&TMdbNtcSysNameObjectHelper::Free(this);
        }

        bool TMdbNtcShareMem::IsOK() const
        {
#ifdef OS_WINDOWS
            if( m_pFileView == NULL || m_iHandle == NULL )
#else
            if( m_pFileView == NULL || m_iHandle == -1 )
#endif
            {
                return false;
            }
            else
            {
                return reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->bInvalid==0;
            }
        }

        bool TMdbNtcShareMem::IsLastestVersion() const
        {
#ifdef OS_WINDOWS
            if( m_pFileView == NULL || m_iHandle == NULL )
#else
            if( m_pFileView == NULL || m_iHandle == -1 )
#endif
            {
                return false;
            }
            else
            {
                return reinterpret_cast<TMdbNtcShareMemDataInfo*>(m_pFileView)->uiShmStructVer==g_uiMdbNtcShmStructVer;
            }
        }

        TMdbNtcStringBuffer TMdbNtcShareMem::ToString() const
        {
            TMdbNtcStringBuffer sLineText;
            sLineText << "TBaseSharedMem :\n";
            if(IsOK())
            {
                sLineText << "  Handle = [" << m_iHandle << "]\n" ;
                sLineText << "  FileView = [" << m_pFileView << "]\n";
            }
            else
                sLineText << "  Handle = [NULL]\n";
#ifndef  OS_WINDOWS
            sLineText << "  NameKey = [" << m_iNameKey << "(0x" << MdbNtcStrUtils.IntToHexStr(m_iNameKey) << ")]\n";
#endif
            sLineText << "  Name = [" << GetName() << "]\n";
            sLineText << "  Created = [" << (m_bSelfCreated ? "True" :"False") << "]\n";
            sLineText << "  Size = [" << GetSize() << "]\n";

            return sLineText;
        }

        bool TMdbNtcShareMem::CheckExist(const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            bool bRet = false;
            TMdbNtcStringBuffer sIpcPath = TMdbNtcSysNameObjectHelper::GetIpcPath(&oRuntimeObject, pszName, pszInstEnvName);
            if(sIpcPath.IsEmpty())
            {
                return false;
            }
#ifdef  OS_WINDOWS
            HANDLE iHandle = OpenFileMapping( FILE_MAP_ALL_ACCESS, false, sIpcPath.c_str());
            if( iHandle != NULL )
            {
                CloseHandle(iHandle);
                iHandle = NULL;
                bRet = true;
            }
#else
            key_t iNameKey = TMdbNtcFileOper::Ftok(sIpcPath.c_str());
            if(iNameKey != -1)
            {
                int iHandle = shmget( iNameKey, 0, 0666 );
                if( iHandle != -1 )
                {
                    bRet = true;
                }
            }
#endif
            return bRet;
        }

        bool TMdbNtcShareMem::Destroy(const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            TMdbNtcShareMem oShareMem(pszName, 0, pszInstEnvName, NULL, false);
            if(oShareMem.GetConstructResult()) return oShareMem.ManualFree();
            else return true;
        }
//}
