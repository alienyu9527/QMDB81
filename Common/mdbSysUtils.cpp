/**
* @file SysUtils.cxx
* @brief IPC共用的类以及基类，系统相关的类与函数
*
* IPC共用的类以及基类，系统相关的类与函数
*
* @author Ge.zhengyi
* @version 1.0
* @date 20121214
* @warning 请继承该类进行相关IPC操作
*/

#include "Common/mdbSysUtils.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#ifdef OS_WINDOWS
    #include <io.h>
    #include <direct.h>
#else
    #include <sys/types.h>
    #include <sys/ipc.h>    
#endif
#include "Common/mdbFileUtils.h"
#include "Common/mdbDateUtils.h"
#include "Common/mdbSysShareMems.h"
#include "Common/mdbSysMessages.h"
#include "Common/mdbDataStructs.h"
#include "Common/mdbSysSockets.h"
#include "Common/mdbStrUtils.h"
#include "Common/mdbNtcSplit.h"
//#include "Common/mdbLogInterface.h"
#ifdef OS_WINDOWS
    #include <process.h>
    #include <tlhelp32.H>
    #include "psapi.h"
    /*
    这样会导致使用BillingSDK.lib的应用按照绝对路径无法找到此lib，还是将库添加到工程中有效
    #define  LIBPATH(p,f)   p##f
    #pragma  comment(lib,LIBPATH(__FILE__, "/../psapi.lib"))
    */
    //#include "../include/SoUtils.hxx"
    #define MDB_NTC_BUF_SIZE 512
    #define MdbNtcProcessBasicInformation 0        
    typedef struct
    {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR  Buffer;
    } UNICODE_STRING, *PUNICODE_STRING;

    typedef struct
    {
        ULONG          AllocationSize;
        ULONG          ActualSize;
        ULONG          Flags;
        ULONG          Unknown1;
        UNICODE_STRING Unknown2;
        HANDLE         InputHandle;
        HANDLE         OutputHandle;
        HANDLE         ErrorHandle;
        UNICODE_STRING CurrentDirectory;
        HANDLE         CurrentDirectoryHandle;
        UNICODE_STRING SearchPaths;
        UNICODE_STRING ApplicationName;
        UNICODE_STRING CommandLine;
        PVOID          EnvironmentBlock;
        ULONG          Unknown[9];
        UNICODE_STRING Unknown3;
        UNICODE_STRING Unknown4;
        UNICODE_STRING Unknown5;
        UNICODE_STRING Unknown6;
    } PROCESS_PARAMETERS, *PPROCESS_PARAMETERS;

    typedef struct
    {
        ULONG               AllocationSize;
        ULONG               Unknown1;
        HINSTANCE           ProcessHinstance;
        PVOID               ListDlls;
        PPROCESS_PARAMETERS ProcessParameters;
        ULONG               Unknown2;
        HANDLE              Heap;
    } PEB, *PPEB;

    typedef struct
    {
        DWORD ExitStatus;
        PPEB  PebBaseAddress;
        DWORD AffinityMask;
        DWORD BasePriority;
        ULONG UniqueProcessId;
        ULONG InheritedFromUniqueProcessId;
    }   PROCESS_BASIC_INFORMATION;

	#ifdef  PROCESS_BASIC_INFORMATION  
    #undef  PROCESS_BASIC_INFORMATION  
    typedef struct _PROCESS_BASIC_INFORMATION {  
        NTSTATUS        ExitStatus;  
        ULONG           PebBaseAddress;  
        ULONG_PTR       AffinityMask;  
        LONG            BasePriority;  
        ULONG_PTR       UniqueProcessId;  
        ULONG_PTR       InheritedFromUniqueProcessId;  
    } PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;
    #endif

	typedef LONG (WINAPI *PROCNTQSIP)(HANDLE,UINT,PVOID,ULONG,PULONG);
	PROCNTQSIP NtQueryInformationProcess;

    BOOL GetProcessCmdLine(DWORD dwId,LPWSTR wBuf,DWORD dwBufLen)
    {
        LONG                      status;
        HANDLE                    hProcess;
        PROCESS_BASIC_INFORMATION pbi;
        PEB                       Peb;
        PROCESS_PARAMETERS        ProcParam;
        DWORD                     dwDummy;
        DWORD                     dwSize;
        LPVOID                    lpAddress;
        BOOL                      bRet = FALSE;
    
        // Get process handle
        hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,dwId);
        if (!hProcess)
            return FALSE;
    
        // Retrieve information
        status = NtQueryInformationProcess( hProcess, MdbNtcProcessBasicInformation,
            (PVOID)&pbi, sizeof(PROCESS_BASIC_INFORMATION), NULL);

        if (status)
            goto cleanup;
    
        if (!ReadProcessMemory( hProcess, pbi.PebBaseAddress, &Peb, sizeof(PEB), &dwDummy)
            )
            goto cleanup;
    
        if (!ReadProcessMemory( hProcess, Peb.ProcessParameters, &ProcParam,
            sizeof(PROCESS_PARAMETERS), &dwDummy)
            )
            goto cleanup;
    
        lpAddress = ProcParam.CommandLine.Buffer;
        dwSize = ProcParam.CommandLine.Length;
    
        if (dwBufLen<dwSize)
            goto cleanup;
    
        if (!ReadProcessMemory( hProcess, lpAddress, wBuf, dwSize, &dwDummy))
            goto cleanup;   
    
        bRet = TRUE;
    
    cleanup:
        CloseHandle (hProcess);    
        return bRet;
    }
#else
    #include <sys/types.h>
    #include <pwd.h>
#endif

//namespace QuickMDB
//{
        /**
         * @brief 分析cmdline，分解参数到TStringObject数组
         * 
         * @param pszCmdLine [in] 命令行
         * @param oCmdArgArray  [out] 得到的参数数组，元素类型TStringObject
         * @return 无
         */
        void MdbNtcGetCmdArg(const char* pszCmdLine, TMdbNtcAutoArray& oCmdArgArray)
        {
            if(pszCmdLine == NULL || *pszCmdLine == '\0') return;
            const char* p = pszCmdLine;
            char cStartQuote = '\0';
            const char* pszStart = p;
            for (; *p != '\0'; ++p)
            {
                if(*p == cStartQuote)
                {
                    TMdbNtcStringObject* pArg = new TMdbNtcStringObject(pszStart, (int)(p-pszStart));
                    pArg->Trim();
                    oCmdArgArray.Add(pArg);
                    pszStart = p+1;
                    cStartQuote = '\0';
                }
                else if(*p == '"' || *p == '\'')
                {
                    cStartQuote = *p;
                    pszStart = p+1;
                }
                else if(cStartQuote == '\0' && *p == ' ')
                {
                    if(p != pszStart)
                    {
                        TMdbNtcStringObject* pArg = new TMdbNtcStringObject(pszStart, (int)(p-pszStart));
                        pArg->Trim();
                        oCmdArgArray.Add(pArg);
                    }                    
                    pszStart = p+1;           
                }
            }
            if(p != pszStart)
            {
                TMdbNtcStringObject* pArg = new TMdbNtcStringObject(pszStart, (int)(p-pszStart));
                pArg->Trim();
                oCmdArgArray.Add(pArg);
            }
        }

        bool TMdbNtcSysUtils::CompareCmdLine(const char* pszCmdLine1, const char* pszCmdLine2)
        {
            TMdbNtcAutoArray oCmdField1, oCmdField2;
            MdbNtcGetCmdArg(pszCmdLine1, oCmdField1);
            MdbNtcGetCmdArg(pszCmdLine2, oCmdField2);
            if(oCmdField1.GetSize() != oCmdField2.GetSize()) return false;
            for (MDB_UINT32 i = 0; i < oCmdField1.GetSize(); ++i)
            {
                TMdbNtcStringObject* pField1 = static_cast<TMdbNtcStringObject*>(oCmdField1[i]);
                TMdbNtcStringObject* pField2 = static_cast<TMdbNtcStringObject*>(oCmdField2[i]);
                if(i == 0)
                {
                    //判断程序是否一致
                    if(0 != TMdbNtcFileOper::GetPureFileName(pField1->c_str()).Compare(
                        TMdbNtcFileOper::GetPureFileName(pField2->c_str()))
                        )
                    {
                        return false;
                    }
                }
                else if(pField1->Compare(pField2) != 0)
                {
                    return false;
                }
            }
            return true;
        }

        MDB_UINT32 TMdbNtcSysUtils::GetUserId(void)
        {
        #ifdef OS_WINDOWS
            return 0;
        #else
            return getuid();
        #endif
        }

        TMdbNtcStringBuffer TMdbNtcSysUtils::GetUserName()
        {
            TMdbNtcStringBuffer sUserName;
            MDB_UINT32 uid = GetUserId();
        #ifdef OS_WINDOWS
            DWORD dwSize = 128;
            ::GetUserName(sUserName.GetBuffer(dwSize), &dwSize); 
        #else
            passwd* pPasswd = getpwuid(uid);
            sUserName = pPasswd->pw_name;
        #endif
            return sUserName;
        }
#ifdef OS_FREEBSD
        #define MDB_NTC_PS_FIELD_COUNT          11
#else
        #define MDB_NTC_PS_FIELD_COUNT          8
#endif
        #define MDB_NTC_PS_FIELD_NAME_INDEX (MDB_NTC_PS_FIELD_COUNT-1)

#ifdef OS_HP
        #define MDB_NTC_PS_AUX_FIELD_COUNT              14
        #define MDB_NTC_PS_AUX_PID_INDEX                3
        #define MDB_NTC_PS_AUX_RSS_INDEX                9
        #define MDB_NTC_PS_AUX_NAME_INDEX               (MDB_NTC_PS_AUX_FIELD_COUNT-1)
#elif defined(OS_SUN)
        #define MDB_NTC_PS_AUX_FIELD_COUNT              7
        #define MDB_NTC_PS_AUX_PID_INDEX                1
        #define MDB_NTC_PS_AUX_CPU_PERCENT_INDEX        3
        #define MDB_NTC_PS_AUX_MEM_PERCENT_INDEX        4
        #define MDB_NTC_PS_AUX_RSS_INDEX                5
        #define MDB_NTC_PS_AUX_NAME_INDEX               (MDB_NTC_PS_AUX_FIELD_COUNT-1)
#else
        #define MDB_NTC_PS_AUX_FIELD_COUNT              11
        #define MDB_NTC_PS_AUX_PID_INDEX                1
        #define MDB_NTC_PS_AUX_CPU_PERCENT_INDEX        2
        #define MDB_NTC_PS_AUX_MEM_PERCENT_INDEX        3
        #define MDB_NTC_PS_AUX_RSS_INDEX                5
        #define MDB_NTC_PS_AUX_NAME_INDEX               (MDB_NTC_PS_AUX_FIELD_COUNT-1)
#endif        

        TMdbNtcStringBuffer TMdbNtcSysUtils::GetProcName(const char* pszCmdLine)
        {
            TMdbNtcStringBuffer sProcName;
            if(pszCmdLine == NULL || *pszCmdLine == '\0') return sProcName;
            const char* p = pszCmdLine;
            char cStartQuote = '\0';
            const char* pszStart = p;
            for (; *p != '\0'; ++p)
            {
                if(*p == '/' || *p == '\\')
                {
                    pszStart = p+1;
                }
                else if(*p == cStartQuote)
                {
                    break;
                }
                else if(*p == '"' || *p == '\'')
                {
                    cStartQuote = *p;
                    pszStart = p+1;
                }
                else if(cStartQuote == '\0' && *p == ' ')
                {
                    if(p == pszStart)
                    {
                        pszStart = p+1;
                    }
                    else
                    {
                        break;
                    }                    
                }
            }
            if(p != pszStart)
            {
                sProcName = TMdbNtcStringBuffer(pszStart, (int)(p-pszStart));
                sProcName.Trim();
            }            
            return sProcName;
        }

        TMdbNtcStringBuffer TMdbNtcSysUtils::GetProcName(pid_t pid)
        {
            TMdbNtcStringBuffer sCmdLine = GetCmdLine(pid);
            if(sCmdLine.IsEmpty()) return sCmdLine;
            else return GetProcName(sCmdLine.c_str());
        }

        bool TMdbNtcSysUtils::GetProcCpuPercent(pid_t pid, double& dPercent)
        {
#ifdef OS_WINDOWS
            return true;
#else
#ifdef OS_HP
            TMdbNtcStringBuffer sCmd(1024, "UNIX95= ps -e -o user,pid,ppid,pcpu,sz,args|grep \"%u\"|grep -v grep", pid);
#elif defined(OS_SUN)
            TMdbNtcStringBuffer sCmd(1024, "ps -efo user,pid,ppid,pcpu,pmem,rss,comm|grep \"%u\"|grep -v grep", pid);
#else
            TMdbNtcStringBuffer sCmd(1024, "ps aux|grep \"%u\"|grep -v grep", pid);
#endif            
            FILE* fp = popen(sCmd.c_str(), "r");
            if(NULL == fp)
            {
                perror("popen执行失败！");
                return false;
            }
            char szBuf[2048] = {'\0'};
            while(fgets(szBuf, sizeof(szBuf), fp) != NULL)
            {
                int iLength = (int)strlen(szBuf), i = 0;
                for (i = 0; i < iLength; ++i)
                {
                    if(0 == isgraph(szBuf[i]))
                    {
                        szBuf[i] = ' ';
                    }
                }
                //取出pid
                TMdbNtcSplit split;
                split.SplitData(szBuf, iLength, ' ', true);
                int iFieldCount = (int)split.GetFieldCount();
                if(iFieldCount >= MDB_NTC_PS_AUX_FIELD_COUNT && atoi(split[1]) == pid)
                {
                    dPercent = atof(split[2]);
                    break;
                }
            }
            /*等待命令执行完毕并关闭管道及文件指针*/
            pclose(fp);
            fp = NULL;
            return true;
#endif
        }

        bool TMdbNtcSysUtils::GetProcMemPercent(pid_t pid, double& dPercent)
        {
#ifdef OS_WINDOWS
            MDB_UINT32 uiBytes = 0;            
            if(GetProcMemUsage(pid, uiBytes))
            {
                MDB_UINT64 uiTotalBytes = GetMemoryTotalSize();
                if(uiTotalBytes == 0) return false;
                dPercent = ((double)uiBytes)/(MDB_INT64)uiTotalBytes;
                return true;
            }
            else
            {
                return false;
            }
#else
#ifdef OS_SUN
            TMdbNtcStringBuffer sCmd(1024, "ps -efo user,pid,ppid,pcpu,pmem,rss,comm|grep \"%u\"|grep -v grep", pid);
#else
            TMdbNtcStringBuffer sCmd(1024, "ps aux|grep \"%u\"|grep -v grep", pid);
#endif 
            FILE* fp = popen(sCmd.c_str(), "r");
            if(NULL == fp)
            {
                perror("popen执行失败！");
                return false;
            }
            char szBuf[2048] = {'\0'};
            while(fgets(szBuf, sizeof(szBuf), fp) != NULL)
            {
                int iLength = (int)strlen(szBuf), i = 0;
                for (i = 0; i < iLength; ++i)
                {
                    if(0 == isgraph(szBuf[i]))
                    {
                        szBuf[i] = ' ';
                    }
                }
                //取出pid
                TMdbNtcSplit split;
                split.SplitData(szBuf, iLength, ' ', true);
                int iFieldCount = (int)split.GetFieldCount();
                if(iFieldCount >= MDB_NTC_PS_AUX_FIELD_COUNT && atoi(split[1]) == pid)
                {
                    dPercent = atof(split[3]);
                    break;
                }
            }
            /*等待命令执行完毕并关闭管道及文件指针*/
            pclose(fp);
            fp = NULL;
            return true;
#endif
        }

        bool TMdbNtcSysUtils::GetProcMemUsage(pid_t pid, MDB_UINT32& uiBytes)
        {
#ifdef OS_WINDOWS
            PROCESS_MEMORY_COUNTERS pmc;  
            if(GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))  
            {  
                uiBytes = static_cast<MDB_UINT32>(pmc.WorkingSetSize);  
                return true;
            }
            return false;
#endif
            return true;
        }

        TMdbNtcStringBuffer TMdbNtcSysUtils::GetCmdLine(pid_t pid)
        {
            TMdbNtcStringBuffer sProcessCmd;
            if(pid == 0) return sProcessCmd;
        #ifdef OS_WINDOWS
            /*
            HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                pid);
            if(hProcess == NULL) return sProcessCmd;
            DWORD	(WINAPI *lpfGetModuleFileNameEx)(HANDLE, HMODULE,LPTSTR, DWORD);
            HMODULE hLibPsApi = LoadLibrary("PSAPI.DLL");
            do 
            {
                if(hLibPsApi)
                {
                    lpfGetModuleFileNameEx	= (DWORD (WINAPI *) (HANDLE, HMODULE,
                        LPTSTR, DWORD) ) GetProcAddress (hLibPsApi,"GetModuleFileNameExA") ;
                    if(lpfGetModuleFileNameEx)
                    {
                        sProcessCmd.Reserve(256);
                        lpfGetModuleFileNameEx(hProcess, NULL, sProcessCmd.GetBuffer(), sProcessCmd.GetAllocLength());
                    }
                }
            } while (0);
            if(hLibPsApi)
            {
                FreeLibrary(hLibPsApi);
                hLibPsApi = NULL;
            }
            if(hProcess)
            {
                CloseHandle(hProcess);
                hProcess = NULL;
            }
            */
            int    dwBufLen = MDB_NTC_BUF_SIZE*2;
            WCHAR  wstr[MDB_NTC_BUF_SIZE]   = {'\0'};
            DWORD  dwMinSize = 0;
            NtQueryInformationProcess = (PROCNTQSIP)GetProcAddress(
                GetModuleHandle("ntdll"),
                "NtQueryInformationProcess"
                );
            if (!NtQueryInformationProcess){ return sProcessCmd;}
            
            if (! GetProcessCmdLine(pid, wstr, dwBufLen)){
                return sProcessCmd;
            }
            //count the byte number for second call, dwMinSize is length
            dwMinSize = WideCharToMultiByte(CP_OEMCP, 0, wstr, -1, NULL, 0, NULL, NULL);
            sProcessCmd.Reserve(dwMinSize+1);
            //convert utf16 to multibyte and save to mbch
            dwMinSize = WideCharToMultiByte(CP_OEMCP, 0, (PWSTR)wstr, -1,
                sProcessCmd.GetBuffer(), dwMinSize, NULL, NULL);
            sProcessCmd.UpdateLength();
        #else
            #ifdef OS_FREEBSD
            TMdbNtcStringBuffer sCmd(1024, "ps aux|grep \"%u\"|grep -v grep", pid);
            #else
            TMdbNtcStringBuffer sCmd(1024, "ps -ef|grep \"%u\"|grep -v grep", pid);
            #endif            
            FILE* fp = popen(sCmd.c_str(), "r");
            if(NULL == fp)
            {
                perror("popen执行失败！");
                return 0;
            }
            char szBuf[2048] = {'\0'};
            while(fgets(szBuf, sizeof(szBuf), fp) != NULL)
            {
                int iLength = (int)strlen(szBuf), i = 0;
                for (i = 0; i < iLength; ++i)
                {
                    if(0 == isgraph(szBuf[i]))
                    {
                        szBuf[i] = ' ';
                    }
                }
                //取出pid
                TMdbNtcSplit split;
                split.SplitData(szBuf, iLength, ' ', true);
                int iFieldCount = (int)split.GetFieldCount();
                if(iFieldCount >= MDB_NTC_PS_FIELD_COUNT && atoi(split[1]) == pid)
                {
                    for (i = MDB_NTC_PS_FIELD_NAME_INDEX; i < iFieldCount; ++i)
                    {
                        if(i != MDB_NTC_PS_FIELD_NAME_INDEX)
                        {
                            sProcessCmd.Append(' ');
                        }
                        sProcessCmd.Append(split[(MDB_UINT32)i]);                
                    }
                    break;
                }
            }
            /*等待命令执行完毕并关闭管道及文件指针*/
            pclose(fp);
            fp = NULL;
        #endif
            return sProcessCmd;
        }

        pid_t TMdbNtcSysUtils::SearchPidByName(const char* pszProcName, const char* pszUserName /* = NULL */)
        {
            if(pszProcName == NULL || *pszProcName == '\0')
            {
                return 0;
            }
            pid_t pid = 0;
        #ifdef OS_WINDOWS
            PROCESSENTRY32 pe32;
            // 在使用这个结构之前，先设置它的大小
            pe32.dwSize = sizeof(pe32); 
            // 给系统内的所有进程拍一个快照
            HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if(hProcessSnap == INVALID_HANDLE_VALUE)
            {
                return -1;
            }
            // 遍历进程快照，轮流显示每个进程的信息
            BOOL bMore = ::Process32First(hProcessSnap, &pe32);
            while(bMore)
            {
                if(strcmp(pszProcName, pe32.szExeFile)==0)
                {
                    pid = pe32.th32ProcessID;
                    break;
                }
                
                bMore = ::Process32Next(hProcessSnap, &pe32);
            }            
            // 不要忘记清除掉snapshot对象
            ::CloseHandle(hProcessSnap);
            hProcessSnap = NULL;
        #else
            #ifdef OS_FREEBSD
            TMdbNtcStringBuffer sCmd(1024, "ps aux|grep \"%s\"|grep -v grep", pszProcName);
            #else
            TMdbNtcStringBuffer sCmd(1024, "ps -ef|grep \"%s\"|grep -v grep", pszProcName);
            #endif           
            FILE* fp = popen(sCmd.c_str(), "r");
            if(NULL == fp)
            {
                perror("popen执行失败！");
                return 0;
            }            
            char szBuf[2048] = {'\0'};
            while(fgets(szBuf, sizeof(szBuf), fp) != NULL)
            {
                int iLength = (int)strlen(szBuf), i = 0;
                for (i = 0; i < iLength; ++i)
                {
                    if(0 == isgraph(szBuf[i]))
                    {
                        szBuf[i] = ' ';
                    }
                }
                //取出pid
                TMdbNtcSplit split;
                split.SplitData(szBuf, iLength, ' ', true);
                int iFieldCount = (int)split.GetFieldCount();
                if(iFieldCount == MDB_NTC_PS_FIELD_COUNT
                    && strcmp(TMdbNtcFileOper::GetFileName(split[MDB_NTC_PS_FIELD_NAME_INDEX]), pszProcName) == 0
                    && (pszUserName == NULL || strcmp(split[0],pszUserName) == 0))
                {
                    pid = atoi(split[1]);
                    break;
                }
            }
            /*等待命令执行完毕并关闭管道及文件指针*/
            pclose(fp);
            fp = NULL;
        #endif
            return pid;
        }

        pid_t TMdbNtcSysUtils::SearchPidByCmdLine(const char* pszCmdLine, const char* pszUserName /* = NULL */)
        {
            if(pszCmdLine == NULL || *pszCmdLine == '\0')
            {
                return 0;
            }
            TMdbNtcAutoArray oCmdArgArray;
            MdbNtcGetCmdArg(pszCmdLine, oCmdArgArray);
            if(oCmdArgArray.GetSize() == 0) return 0;
            pid_t pid = 0;
            TMdbNtcStringBuffer sPureProcName = TMdbNtcFileOper::GetPureFileName(
                static_cast<TMdbNtcStringObject*>(oCmdArgArray[0])->c_str());
        #ifdef OS_WINDOWS
            PROCESSENTRY32 pe32;
            // 在使用这个结构之前，先设置它的大小
            pe32.dwSize = sizeof(pe32); 
            
            // 给系统内的所有进程拍一个快照
            HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if(hProcessSnap == INVALID_HANDLE_VALUE)
            {
                return -1;
            }
            // 遍历进程快照，轮流显示每个进程的信息
            BOOL bMore = ::Process32First(hProcessSnap, &pe32);
            while(bMore)
            {
                if(sPureProcName.Compare(pe32.szExeFile)==0)
                {
                    //根据pid得到程序参数
                    TMdbNtcStringBuffer sCurCmdLine = GetCmdLine(pe32.th32ProcessID);
                    TMdbNtcAutoArray oCurCmdArgArray;
                    MdbNtcGetCmdArg(sCurCmdLine.c_str(), oCurCmdArgArray);
                    if(oCmdArgArray.GetSize() == oCurCmdArgArray.GetSize())
                    {
                        //因为进程名已经比较过了，所以直接比较参数
                        MDB_UINT32 i = 1;
                        for (; i < oCmdArgArray.GetSize(); ++i)
                        {
                            TMdbNtcStringObject* pField1 = static_cast<TMdbNtcStringObject*>(oCmdArgArray[i]);
                            TMdbNtcStringObject* pField2 = static_cast<TMdbNtcStringObject*>(oCurCmdArgArray[i]);
                            if(pField1->Compare(pField2) != 0)
                            {
                                break;
                            }
                        }
                        if(i == oCmdArgArray.GetSize())
                        {
                            pid = pe32.th32ProcessID;
                            break;
                        }
                    }
                }                
                bMore = ::Process32Next(hProcessSnap, &pe32);
            }            
            // 不要忘记清除掉snapshot对象
            ::CloseHandle(hProcessSnap);
            hProcessSnap = NULL;
        #else
            #ifdef OS_FREEBSD
            TMdbNtcStringBuffer sCmd(1024, "ps aux|grep \"%s\"|grep -v grep", sPureProcName.c_str());
            #else
            TMdbNtcStringBuffer sCmd(1024, "ps -ef|grep \"%s\"|grep -v grep", sPureProcName.c_str());
            #endif            
            FILE* fp = popen(sCmd.c_str(), "r");
            if(NULL == fp)
            {
                perror("popen执行失败！");
                return 0;
            }            
            char szBuf[2048] = {'\0'};
            while(fgets(szBuf, sizeof(szBuf), fp) != NULL)
            {
                int iLength = (int)strlen(szBuf), i = 0;
                for (i = 0; i < iLength; ++i)
                {
                    if(0 == isgraph(szBuf[i]))
                    {
                        szBuf[i] = ' ';
                    }
                }
                //取出pid
                TMdbNtcSplit split;
                split.SplitData(szBuf, iLength, ' ', true);
                int iFieldCount = (int)split.GetFieldCount();
                if(iFieldCount == MDB_NTC_PS_FIELD_NAME_INDEX+(int)oCmdArgArray.GetSize()
                    && (pszUserName == NULL || strcmp(split[0],pszUserName) == 0))
                {
                    if(sPureProcName.Compare(
                        TMdbNtcFileOper::GetFileName(split[MDB_NTC_PS_FIELD_NAME_INDEX])) != 0)
                    {
                        break;
                    }
                    for (i = 1; i < (int)oCmdArgArray.GetSize(); ++i)
                    {
                        if(static_cast<TMdbNtcStringObject*>(oCmdArgArray[(MDB_UINT32)i])->Compare(split[MDB_NTC_PS_FIELD_NAME_INDEX+(MDB_UINT32)i]) != 0)
//						if( strcmp((static_cast<TMdbNtcStringObject*>(oCmdArgArray[i])->ToString()).c_str(),split[MDB_NTC_PS_FIELD_NAME_INDEX+i]) != 0 )
                        {
                            break;
                        }
                    }
                    if(i == (int)oCmdArgArray.GetSize())
                    {
                        pid = atoi(split[1]);
                        break;
                    }
                }
            }
            /*等待命令执行完毕并关闭管道及文件指针*/
            pclose(fp);
            fp = NULL;
            /*
            if(-1 == rc)
            {
                perror("关闭文件指针失败");
                exit(1);
            }
            else
            {
                printf("命令【%s】子进程结束状态【%d】命令返回值【%d】\r\n", command, rc, WEXITSTATUS(rc));
            }
            */
        #endif
            return pid;
        }

        bool TMdbNtcSysUtils::IsProcessExist(pid_t pid, bool bEnumProcesses /* = false */)
        {
            if(pid == 0) return false;
            if(bEnumProcesses == false)
            {
#ifdef OS_WINDOWS
                HANDLE hProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
                if(hProcessHandle)
                {
                    CloseHandle(hProcessHandle);
                    return true;
                }
                else
                {
                    return false;
                }
#else
                return kill(pid, 0) == 0;
#endif
            }
            else
            {
#ifdef OS_WINDOWS
                bool bRet = false;
                PROCESSENTRY32 pe32;
                // 在使用这个结构之前，先设置它的大小
                pe32.dwSize = sizeof(pe32); 
                // 给系统内的所有进程拍一个快照
                HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                if(hProcessSnap == INVALID_HANDLE_VALUE) return false;
                // 遍历进程快照，轮流显示每个进程的信息
                BOOL bMore = ::Process32First(hProcessSnap, &pe32);
                while(bMore)
                {
                    if(pe32.th32ProcessID == pid)
                    {
                        bRet = true;
                        break;
                    }
                    bMore = ::Process32Next(hProcessSnap, &pe32);
                }            
                // 不要忘记清除掉snapshot对象
                ::CloseHandle(hProcessSnap);
                hProcessSnap = NULL;
                return bRet;
#elif defined(OS_HP)
                char szCmd[128] = {0};
                memset(szCmd,0,sizeof(szCmd));
                sprintf(szCmd,"%s\"%d\"%s  ","ps -ef|grep -v grep|grep ", pid, "|awk '{print $2}'");
                bool bRet = false;
                FILE * p = popen(szCmd,"r");
                if(NULL == p)
                {
                    TADD_WARNING("errorno=%d, error-msg=%s\n.", errno, strerror(errno));
                    return false;
                }
                char pidBuf[16] = {0};
                while(1)
                {
                    memset(pidBuf,0,sizeof(pidBuf));
                    fgets(pidBuf,sizeof(pidBuf),p);
                    if(strlen(pidBuf)==0) break;
                    if(pid == atoi(pidBuf))
                    {
                        bRet = true;
                        break;
                    }
                }
                pclose(p);
                return bRet;
#else
                char szProcFile[32] = {0};
                sprintf(szProcFile,"/proc/%u",pid);
                return TMdbNtcPathOper::IsExist(szProcFile);
#endif
            }
        }

        void TMdbNtcSysUtils::TermProcess(pid_t pid)
        {
            if(pid > 0)
            {
            #ifdef OS_WINDOWS
                HANDLE hProcess = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, pid);
                if(hProcess)
                {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            #else
                kill(pid, SIGINT);
            #endif
            }
        }

        void TMdbNtcSysUtils::KillProcess(pid_t pid)
        {
            if(pid > 0)
            {
#ifdef OS_WINDOWS
                HANDLE hProcess = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, pid);
                if(hProcess)
                {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
#else
                kill(pid, SIGKILL);
#endif
            }
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcSystemNameObject, TMdbNtcComponentObject);
        TMdbNtcSystemNameObject::TMdbNtcSystemNameObject(const char* pszName,
            const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */)
        {
            m_bSelfCreated = false;
            m_iErrorNo = 0;
            m_sErrorText = "";
            m_bIsOK = false;
            m_sInstEnvName = pszInstEnvName;
            this->m_pParentNameObject = pParentNameObject;
            if(pszName == NULL || *pszName == '\0')
            {
                m_bConstructResult = false;
                TADD_WARNING("System Name Object's name is empty.");
                return;
            }
            else m_sName = pszName;
        }

        TMdbNtcSystemNameObject::~TMdbNtcSystemNameObject( )
        {
            TMdbNtcSysNameObjectHelper::SignOut(this);
        }

        bool TMdbNtcSystemNameObject::IsCreated(void) const
        {
            return m_bSelfCreated;
        }

        int TMdbNtcSystemNameObject::GetErrorNo(void)
        {
#ifdef OS_WINDOWS
            return GetLastError();
#else
            return errno;
#endif
        }

        TMdbNtcStringBuffer TMdbNtcSystemNameObject::GetErrorText(void)
        {
            return m_sErrorText;
        }

        bool TMdbNtcSystemNameObject::IsOK(void) const
        {
            return m_bIsOK;
        }

        void TMdbNtcSystemNameObject::AddSystemNameObject(const int iIPCKey)
        {

        }

        void TMdbNtcSystemNameObject::DeleteSystemNameObject(const int iIPCKey)
        {

        }

        TMdbNtcStringBuffer TMdbNtcSysNameObjectHelper::GetIpcPath(const TMdbRuntimeObject* pRuntimeObject, const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            if(pszName == NULL || *pszName == '\0') return "";
            TMdbNtcStringBuffer sIpcPath = TMdbNtcParameter::GetIPCKeyPath(pszInstEnvName);            
            if(pRuntimeObject == MDB_ZF_RUNTIME_OBJECT(TMdbNtcShareMem))
            {
                sIpcPath.Append(MDB_NTC_ZS_PATH_DELIMITATED"shm"MDB_NTC_ZS_PATH_DELIMITATED);
            }
            else if(pRuntimeObject == MDB_ZF_RUNTIME_OBJECT(TMdbNtcProcessEvent)
                || pRuntimeObject == MDB_ZF_RUNTIME_OBJECT(TMdbNtcProcessNotify)
                || pRuntimeObject == MDB_ZF_RUNTIME_OBJECT(TMdbNtcProcessLock))
            {
                sIpcPath.Append(MDB_NTC_ZS_PATH_DELIMITATED"sem"MDB_NTC_ZS_PATH_DELIMITATED);
            }
            else
            {
                sIpcPath.Append(MDB_NTC_ZS_PATH_DELIMITATED);
            }
            sIpcPath.Append(pRuntimeObject->GetObjectName());
            sIpcPath.Append('_');
            sIpcPath.Append(pszName);
#ifdef  OS_WINDOWS
            char *pEnv = sIpcPath.GetBuffer();
            while(*pEnv!='\0')
            {
                if(*pEnv=='\\' ||*pEnv==':'||*pEnv=='.')
                    *pEnv = '_';
                pEnv++;
            }
#endif
            return sIpcPath;
        }
        
        TMdbNtcStringBuffer TMdbNtcSysNameObjectHelper::GetNameObjectPath(const TMdbRuntimeObject* pRuntimeObject, const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            if(pszName == NULL || *pszName == '\0') return "";
            TMdbNtcStringBuffer sNameObjectPath=TMdbNtcParameter::GetNameObjectPath(pszInstEnvName);
            sNameObjectPath.Append(MDB_NTC_ZS_PATH_DELIMITATED);
            sNameObjectPath.Append(pRuntimeObject->GetObjectName());
            sNameObjectPath.Append(MDB_NTC_ZS_PATH_DELIMITATED);
            sNameObjectPath.Append(pszName);
            return sNameObjectPath;
        }

        bool TMdbNtcSysNameObjectHelper::GenIpcProcFilePath(TMdbNtcSystemNameObject* pNameObject)
        {
            TMdbNtcStringBuffer sProcFilePath = GetIpcPath(pNameObject);
            if(sProcFilePath.IsEmpty())
            {
                return false;
            }            
            if(TMdbNtcDirOper::MakeFullDir(sProcFilePath.c_str()))
            {
                sProcFilePath.Append(pNameObject->IsCreated()?MDB_NTC_ZS_PATH_DELIMITATED"C":MDB_NTC_ZS_PATH_DELIMITATED"A");
                sProcFilePath.Append(TMdbNtcShortDateTime::Format());
                sProcFilePath.setdelimiter('_');
                sProcFilePath<<getpid()<<TMdbNtcSysUtils::GetProcName(getpid());
                if(pNameObject->IsCreated())
                {
                    if(pNameObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcShareMem)))
                    {
                        sProcFilePath<<static_cast<TMdbNtcShareMem*>(pNameObject)->GetSize();
                    }
                    else if(pNameObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcMessageQueue)))
                    {
                        sProcFilePath<<static_cast<TMdbNtcMessageQueue*>(pNameObject)->GetTotalSize();
                    }
                }
                //printf("make ipc %s\n", sProcFilePath.c_str());
                TMdbNtcStringBuffer sCmdLine = TMdbNtcSysUtils::GetCmdLine(getpid());
                if(TMdbNtcFileOper::WriteContent(sProcFilePath.c_str(), sCmdLine.c_str(), (int)sCmdLine.length()))
                {
                    pNameObject->m_sIpcProcFilePath = sProcFilePath;
                    return true;
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
        }

        bool TMdbNtcSysNameObjectHelper::GenNameObjectProcFilePath(TMdbNtcSystemNameObject* pNameObject)
        {
            TMdbNtcStringBuffer sProcFilePath = GetNameObjectPath(pNameObject);
            if(sProcFilePath.IsEmpty())
            {
                return false;
            }
            if(TMdbNtcDirOper::MakeFullDir(sProcFilePath.c_str()))
            {
                sProcFilePath.Append(pNameObject->IsCreated()?MDB_NTC_ZS_PATH_DELIMITATED"C":MDB_NTC_ZS_PATH_DELIMITATED"A");
                sProcFilePath.Append(TMdbNtcShortDateTime::Format());
                sProcFilePath.setdelimiter('_');
                sProcFilePath<<getpid()<<TMdbNtcSysUtils::GetProcName(getpid());
                if(pNameObject->IsCreated())
                {
                    if(pNameObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcShareMem)))
                    {
                        sProcFilePath<<static_cast<TMdbNtcShareMem*>(pNameObject)->GetSize();
                    }
                    else if(pNameObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcMessageQueue)))
                    {
                        sProcFilePath<<static_cast<TMdbNtcMessageQueue*>(pNameObject)->GetTotalSize();
                    }
                }
                //printf("make sno %s\n", sProcFilePath.c_str());
                TMdbNtcStringBuffer sCmdLine = TMdbNtcSysUtils::GetCmdLine(getpid());
                if(TMdbNtcFileOper::WriteContent(sProcFilePath.c_str(), sCmdLine.c_str(), (int)sCmdLine.length()))
                {
                    pNameObject->m_sNameObjectProcFilePath = sProcFilePath;
                    return true;
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
        }

        bool TMdbNtcSysNameObjectHelper::SignIn(TMdbNtcSystemNameObject* pNameObject)
        {
            bool bRet = true;
            if(pNameObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcShareMem))
                || pNameObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcProcessEvent))
                || pNameObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcProcessNotify))
                || pNameObject->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcProcessLock)))
            {
                //存储ipc资源信息
                bRet = GenIpcProcFilePath(pNameObject)||bRet;
            }
            //存储命名对象信息
            if(pNameObject->m_pParentNameObject == NULL)
            {
                bRet = GenNameObjectProcFilePath(pNameObject)||bRet;
            }
            return bRet;
        }

        bool TMdbNtcSysNameObjectHelper::SignOut(TMdbNtcSystemNameObject* pNameObject)
        {
            if(pNameObject->IsCreated()) return true;
            bool bRet = true;
            if(!pNameObject->m_sIpcProcFilePath.IsEmpty())
            {
                //printf("remove ipc %s\n", pNameObject->m_sIpcProcFilePath.c_str());
                if(TMdbNtcFileOper::Remove(pNameObject->m_sIpcProcFilePath.c_str()))
                {
                    pNameObject->m_sIpcProcFilePath.Clear();
                }
                else
                {
                    bRet = false;
                }
            }
            if(!pNameObject->m_sNameObjectProcFilePath.IsEmpty())
            {
                //printf("remove sno %s\n", pNameObject->m_sNameObjectProcFilePath.c_str());
                if(TMdbNtcFileOper::Remove(pNameObject->m_sNameObjectProcFilePath.c_str()))
                {
                    pNameObject->m_sNameObjectProcFilePath.Clear();
                }
                else
                {
                    bRet = false;
                }
            }
            return bRet;
        }

        bool TMdbNtcSysNameObjectHelper::Free(TMdbNtcSystemNameObject* pNameObject)
        {
            if(!pNameObject->IsCreated())//自身为附接者，先注销
            {
                SignOut(pNameObject);//子
            }
            return Free(pNameObject->GetRuntimeObject(), pNameObject->GetName(), pNameObject->GetInstEnvName());
        }

        bool TMdbNtcSysNameObjectHelper::Free(const TMdbRuntimeObject* pRuntimeObject, const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            return true;
        }

        bool TMdbNtcSysUtils::RunBackground()
        {
#ifdef OS_WINDOWS
            HWND hwnd = ::FindWindow("ConsoleWindowClass",NULL);            
            if(hwnd) //找到控制台句柄
            {
                ShowOwnedPopups(hwnd,SW_HIDE);//显示或隐藏由指定窗口所有的全部弹出式窗口
                ::ShowWindow(hwnd,SW_HIDE); //隐藏控制台窗口
                return true;
            }
            else return false;
#else
            if (fork() > 0) exit(0); // run in backgroud
            return true;
#endif
        }

        bool TMdbNtcSysUtils::GetCmdOutput(const char* pszCmd, TMdbNtcStringBuffer& sOutput)
        {
#ifdef OS_WINDOWS            
            //由于_popen不适合windows运行时程序，故需要CreateProcess来完成
            HANDLE   hRead = NULL, hWrite = NULL;
            SECURITY_ATTRIBUTES   sa;
            sa.nLength   =   sizeof(SECURITY_ATTRIBUTES);     
            sa.lpSecurityDescriptor   =   NULL;     
            sa.bInheritHandle   =   TRUE;     
            if(!CreatePipe(&hRead, &hWrite, &sa, 0))       
            {
                return false;
            }
            bool bRet = true;
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
                if(!CreateProcess(NULL, (LPSTR)pszCmd, NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi))
                {
                    bRet = false;
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
                while(ReadFile(hRead, szBuffer, sizeof(szBuffer)-1, &uiBytesRead, NULL))
                {
                    sOutput.Append(szBuffer);
                }
            } while (0);
            CloseHandle(hRead);
            hRead = NULL;
            if(hWrite)
            {
                CloseHandle(hWrite);
                hWrite = NULL;
            }
            /*如果末位字符为\n则删除*/
            if(sOutput[sOutput.length()-1] == '\n')
            {
                sOutput.Delete(sOutput.length()-1,-1);
            }
            
            return bRet;
#else
            FILE* fp = popen(pszCmd, "r");
            if(NULL == fp)
            {
                perror("popen执行失败！");
                return false;
            }
            char szBuf[2048] = {'\0'};
            while(fgets(szBuf, sizeof(szBuf), fp) != NULL)
            {
                sOutput.Append(szBuf);
            }
            /*等待命令执行完毕并关闭管道及文件指针*/
            pclose(fp);
            fp = NULL;
            /*如果末位字符为\n则删除*/
            if(sOutput[sOutput.length()-1] == '\n')
            {
                sOutput.Delete(sOutput.length()-1,-1);
            }
            
            return true;
#endif            
        }
//}
#ifdef OS_WINDOWS
    #if _MSC_VER < 1300
        //为了获得内存信息
        typedef struct _MEMORYSTATUSEX {
            DWORD dwLength;
            DWORD dwMemoryLoad;
            DWORDLONG ullTotalPhys;
            DWORDLONG ullAvailPhys;
            DWORDLONG ullTotalPageFile;
            DWORDLONG ullAvailPageFile;
            DWORDLONG ullTotalVirtual;
            DWORDLONG ullAvailVirtual;
            DWORDLONG ullAvailExtendedVirtual;
        } MEMORYSTATUSEX, *LPMEMORYSTATUSEX;
    #endif
#elif defined(OS_LINUX)
    #include <sys/sysinfo.h>//for GetMemoryTotalSize
#elif defined(OS_IBM)
    #include <libperfstat.h>//for GetMemoryTotalSize
#elif defined(OS_HP)
    #include <sys/unistd.h>//for GetMemoryTotalSize
#endif

#ifndef OS_WINDOWS
    #include <net/if.h>//for GetHostIP
#endif

//namespace QuickMDB
//{
        MDB_UINT64 TMdbNtcSysUtils::GetMemoryTotalSize()
        {
#ifdef OS_WINDOWS
            MEMORYSTATUSEX        Meminfo;            
            typedef BOOL (WINAPI * Fun_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX lpBuffer);            
            Fun_GlobalMemoryStatusEx        GlobalMemoryStatusEx_;
            TSoHelper oHelper;
            if(!oHelper.OpenLibrary("kernel32.dll")) return 0;
            GlobalMemoryStatusEx_ = (Fun_GlobalMemoryStatusEx)oHelper.LoadSymbol("GlobalMemoryStatusEx");
            if(GlobalMemoryStatusEx_ == NULL) return 0;
            memset(&Meminfo, 0, sizeof(Meminfo));
            Meminfo.dwLength = sizeof(Meminfo);
            GlobalMemoryStatusEx_(&Meminfo);
            return Meminfo.ullTotalPhys;
#elif defined(OS_LINUX)
            struct sysinfo si;
            if(sysinfo (&si) != 0) return 0;
            else return si.totalram*si.mem_unit;
#elif defined(OS_IBM)
            perfstat_memory_total_t  m1;     //perfstat_memory_total_t结构定义了内存信息            
            //获取内存信息，函数定义见头文件
            if (perfstat_memory_total(NULL,&m1,sizeof(perfstat_memory_total_t),1) < 0)  return 0;
            else return m1.real_total*4096;
#elif defined(OS_HP)
            return sysconf(_SC_MEM_MBYTES)*1024;//_SC_NUM_CPUS
#elif defined(OS_SUN)
            FILE* fp = popen("prtconf | grep Memory|sed -n '1p'|awk 'BEGIN{FS=\" \"}{print $3}'", "r");
            if(NULL == fp)
            {
                perror("popen执行失败！");
                return 0;
            }
            MDB_UINT64 uiTotalSize = 0;
            char szBuf[2048] = {'\0'};
            while(fgets(szBuf, sizeof(szBuf), fp) != NULL)
            {
                uiTotalSize = atoi(szBuf);
                uiTotalSize *= 1024*1024;
            }
            /*等待命令执行完毕并关闭管道及文件指针*/
            pclose(fp);
            fp = NULL;
            return uiTotalSize;
#else
            return 0;
#endif
        }

        MDB_UINT64 TMdbNtcSysUtils::GetMemoryFreeSize()
        {
#ifdef OS_WINDOWS
            MEMORYSTATUSEX        Meminfo;            
            typedef BOOL (WINAPI * Fun_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX lpBuffer);            
            Fun_GlobalMemoryStatusEx        GlobalMemoryStatusEx_;
            TSoHelper oHelper;
            if(!oHelper.OpenLibrary("kernel32.dll")) return 0;
            GlobalMemoryStatusEx_ = (Fun_GlobalMemoryStatusEx)oHelper.LoadSymbol("GlobalMemoryStatusEx");
            if(GlobalMemoryStatusEx_ == NULL) return 0;
            memset(&Meminfo, 0, sizeof(Meminfo));
            Meminfo.dwLength = sizeof(Meminfo);
            GlobalMemoryStatusEx_(&Meminfo);
            return Meminfo.ullAvailPhys;
#elif defined(OS_LINUX)            
            FILE* fp = popen("cat /proc/meminfo|awk 'BEGIN{FS=\" \"}{print $2}'", "r");
            if(NULL == fp)
            {
                perror("popen执行失败！");
                return 0;
            }
            MDB_UINT64 uiMemTotal = 0, uiMemFree = 0, uiBuffers = 0, uiCached = 0;
            MDB_UINT32 i = 0;
            char szBuf[1024] = {'\0'};
            while(fgets(szBuf, sizeof(szBuf), fp) != NULL)
            {
                if(i == 0) uiMemTotal = (MDB_UINT64)TMdbNtcStrFunc::StrToInt(szBuf);
                else if(i == 1) uiMemFree = (MDB_UINT64)TMdbNtcStrFunc::StrToInt(szBuf);
                else if(i == 2) uiBuffers = (MDB_UINT64)TMdbNtcStrFunc::StrToInt(szBuf);
                else if(i == 3) uiCached = (MDB_UINT64)TMdbNtcStrFunc::StrToInt(szBuf);
                else break;
                ++i;
            }
            /*等待命令执行完毕并关闭管道及文件指针*/
            pclose(fp);
            fp = NULL;
            //空闲内存计算方式：如果Cached值大于MemTotal值则空闲内存为MemFree值，否则空闲内存为MemFree值+Buffers值+Cached值
            if(uiCached > uiMemTotal) return uiMemFree*1024;
            else return (uiMemFree+uiBuffers+uiCached)*1024;
#elif defined(OS_IBM)
            perfstat_memory_total_t  m1;     //perfstat_memory_total_t结构定义了内存信息            
            //获取内存信息，函数定义见头文件
            if (perfstat_memory_total(NULL,&m1,sizeof(perfstat_memory_total_t),1) < 0)  return 0;
            else return m1.real_free*4096;
#else
            TMdbNtcStringBuffer sOutput;
            if(!GetCmdOutput("vmstat|sed -n '$p'|awk '{printf(\"%s\", $5);}'", sOutput)) return 0;
            return (MDB_UINT64)sOutput.ToInt64()*4096;
#endif
        }

        MDB_UINT32 TMdbNtcSysUtils::GetCpuNum()
        {
#ifdef OS_WINDOWS
            SYSTEM_INFO sysinfo;
            GetSystemInfo(&sysinfo);
            return sysinfo.dwNumberOfProcessors;
#elif defined(OS_HP)
            return sysconf(_SC_NUM_CPUS);
#elif defined(OS_FREEBSD)
            int nm[2];
            size_t len = 4;
            uint32_t count;
        
            nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
            sysctl(nm, 2, &count, &len, NULL, 0);
        
            if(count < 1)
            {
                nm[1] = HW_NCPU;
                sysctl(nm, 2, &count, &len, NULL, 0);
                if(count < 1) { count = 1; }
            }
            return count;
#else
            return (MDB_UINT32)sysconf(_SC_NPROCESSORS_ONLN);
#endif
        }

        bool TMdbNtcSysUtils::GetHostName(TMdbNtcStringBuffer& sHostName)
        {
            sHostName.Reserve(255);
            //获取本地主机名称
            if (gethostname(sHostName.GetBuffer(), sHostName.GetAllocLength()) == MDB_NTC_SOCKET_ERROR)
            {
                TADD_WARNING("Error %d when getting local host name.", TMdbNtcSocket::GetLastError());
                return false;
            }
            sHostName.UpdateLength();
            return true;
        }

        int TMdbNtcSysUtils::GetHostIP(TMdbNtcStringBuffer& sIPList)
        {
            int iIPCount = 0;
            sIPList.setdelimiter(';');
#ifdef OS_LINUX
            QuickMDB_SOCKET fd = (MDB_UINT32)socket (AF_INET, SOCK_DGRAM, 0);
            if(fd == (MDB_UINT32)MDB_NTC_SOCKET_ERROR)
            {
                TADD_WARNING("cpm: socket");
                return -1;
            }
            struct ifreq buf[16];
            // struct arpreq arp;
            struct ifconf ifc;            
            ifc.ifc_len = sizeof buf;
            ifc.ifc_buf = (caddr_t) buf;
            if (!ioctl ((int)fd, SIOCGIFCONF, (char *) &ifc))
            {
                //获取接口信息
                int intrface = ifc.ifc_len / (int)sizeof (struct ifreq);
                //printf("interface num is intrface=%d\n\n\n",intrface);
                char szIP[32] = {'\0'};
                //根据接口信息循环获取设备IP和MAC地址
                while (intrface-- > 0)
                {
                    //获取当前网卡的IP地址
                    if (!(ioctl ((int)fd, SIOCGIFADDR, (char *) &buf[intrface])))
                    {
                        inet_ntop(AF_INET, &((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr, szIP, sizeof(szIP));
                        //const char* pszIP = inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr);
                        if(strcmp(szIP, "127.0.0.1") != 0)
                        {
                            sIPList<<szIP;
                            ++iIPCount;
                        }                            
                    }
                    else
                    {
                        TADD_WARNING("cpm: ioctl device %s", buf[intrface].ifr_name);
                        iIPCount = -1;
                        break;
                    }
                } //while
            }
            else
            {
                TADD_WARNING("cpm: ioctl");
                iIPCount = -1;
            }         
            close((int)fd);
#else
            char host_name[255] = {'\0'};
            //获取本地主机名称
            if (gethostname(host_name, sizeof(host_name)) == MDB_NTC_SOCKET_ERROR)
            {
                TADD_WARNING("Error %d when getting local host name.", TMdbNtcSocket::GetLastError());
                return -1;
            }
            //printf("Host name is: %s\n", host_name);
            //从主机名数据库中得到对应的“主机”
            struct hostent *phe = gethostbyname(host_name);
            if (phe == 0)
            {
                TADD_WARNING("Yow! Bad host lookup.");
                return -1;
            }
            char szIP[32] = {'\0'};
            /* 根据地址类型，将地址打出来 */
            switch(phe->h_addrtype)
            {
            case AF_INET:
                {
                    //循环得出本地机器所有IP地址
                    for (int i = 0; phe->h_addr_list[i] != 0; ++i)
                    {
                        inet_ntop(phe->h_addrtype, phe->h_addr_list[i], szIP, sizeof(szIP));
                        //const char* pszIP = inet_ntoa(*(struct in_addr*)phe->h_addr_list[i]);
                        if(strcmp(szIP, "127.0.0.1") != 0)
                        {
                            sIPList<<szIP;
                        }                        
                        ++iIPCount;
                    }
                }
                break;
            case AF_INET6:
            default:
                {
                    TADD_WARNING("unknown address type");
                    return -1;
                }                
                break;
            }
#endif
            return iIPCount;
        }

        MDB_UINT64 TMdbNtcSysUtils::GetShmMax()
        {
#ifdef OS_LINUX
            TMdbNtcStringBuffer sContent;
            if(!TMdbNtcFileOper::ReadContent("/proc/sys/kernel/shmmax", sContent)) return 0;
            else return (MDB_UINT64)sContent.ToInt64();
#elif defined(OS_IBM)
            /*
            The following limits apply to shared memory:
            *    Maximum shared-memory segment size is:
                *    256 MB before AIX(R) 4.3.1
                *    2 GB for AIX(R) 4.3.1 through AIX(R) 5.1
                *    32 TB for 64-bit applications for AIX(R) 5.1 and later
            */
            TMdbNtcStringBuffer sOutput;
            if(!GetCmdOutput("oslevel", sOutput)) return 0;
            TMdbNtcSplit oSplit;
            oSplit.SplitString(sOutput.c_str(), '.', true);
            MDB_UINT32 uiFieldCount = oSplit.GetFieldCount();
            if(uiFieldCount == 0) return 0;
            double dVersion = sOutput.ToDouble();
            if(atoi(oSplit[0]) < 4 || uiFieldCount < 2 || atoi(oSplit[1]) < 3
                || uiFieldCount < 3 || atoi(oSplit[2]) < 1)
            {
                return 256*1024*1024;
            }
            else if(atoi(oSplit[0]) < 5 || uiFieldCount < 2 || atoi(oSplit[1]) < 1)
            {
                return 2*1024*1024*1024;
            }
            else
            {
                if(sizeof(long) == 4) return MDB_NTC_ZS_MAX_UINT32;
                else return 32*1024*1024*1024*1024;
            }
#elif defined(OS_HP)
            TMdbNtcStringBuffer sContent;
            if(!GetCmdOutput("kctune shmmax|sed -n '$p'|awk '{print $2}'", sContent)) return 0;
            else return (MDB_UINT64)sContent.ToInt64();
#elif defined(OS_SUN)
            TMdbNtcStringBuffer sContent;
            if(!GetCmdOutput("prctl -n project.max-shm-memory -i process $$|grep privileged|sed -n '$p'|awk '{printf(\"%s\",$2);}'", sContent)) return 0;
            if(sContent.ReverseFind("EB") >= 0) return (MDB_UINT64)(MDB_INT64)(sContent.ToDouble()*1024*1024*1024*1024*1024);
            else if(sContent.ReverseFind("PB") >= 0) return (MDB_UINT64)(MDB_INT64)(sContent.ToDouble()*1024*1024*1024*1024);
            else if(sContent.ReverseFind("GB") >= 0) return (MDB_UINT64)(MDB_INT64)(sContent.ToDouble()*1024*1024*1024);
            else if(sContent.ReverseFind("MB") >= 0) return (MDB_UINT64)(MDB_INT64)(sContent.ToDouble()*1024*1024);
            else if(sContent.ReverseFind("KB") >= 0) return (MDB_UINT64)(MDB_INT64)(sContent.ToDouble()*1024);
            else return (MDB_UINT64)sContent.ToInt64();
#else
            if(sizeof(long) == 4) return MDB_NTC_ZS_MAX_UINT32;
            else return MDB_NTC_ZS_MAX_UINT64;
#endif
        }

        MDB_UINT64 TMdbNtcSysUtils::GetShmSeg()
        {
#ifdef OS_LINUX
            //linux下没有找到SHMSEG的取值，这里可以使用系统范围内最大共享内存段数量SHMMNI
            TMdbNtcStringBuffer sContent;
            if(!TMdbNtcFileOper::ReadContent("/proc/sys/kernel/shmmni", sContent)) return 0;
            else return (MDB_UINT64)sContent.ToInt64();
#elif defined(OS_IBM)
            /*
            对于64位进程，同一进程可连接最多268435456个共享内存段；
            对于32位进程，同一进程可连接最多11个共享内存段，除非使用扩展的shmat；export EXTSHM=ON 
            */
            if(sizeof(long) == 8) return 26843545611;
            else
            {
                const char* pszExtShm = getenv("EXTSHM");
                if(pszExtShm == NULL || mdb_ntc_stricmp(pszExtShm, "ON") != 0) return 11;
                else return 26843545611;
            }
#elif defined(OS_HP)
            TMdbNtcStringBuffer sContent;
            if(!GetCmdOutput("kctune shmseg|sed -n '$p'|awk '{print $2}'", sContent)) return 0;
            else return (MDB_UINT64)sContent.ToInt64();
#elif defined(OS_SUN)
            //It is usually set to shmmni, but it should always be less than 65535.
            TMdbNtcStringBuffer sContent;
            if(!GetCmdOutput("prctl -n project.max-shm-ids -i process $$|grep privileged|sed -n '$p'|awk '{printf(\"%s\",$2);}'", sContent)) return 0;
            if(sContent.ReverseFind("EB") >= 0) return (MDB_UINT64)(MDB_INT64)(sContent.ToDouble()*1024*1024*1024*1024*1024);
            else if(sContent.ReverseFind("PB") >= 0) return (MDB_UINT64)(MDB_INT64)(sContent.ToDouble()*1024*1024*1024*1024);
            else if(sContent.ReverseFind("GB") >= 0) return (MDB_UINT64)(MDB_INT64)(sContent.ToDouble()*1024*1024*1024);
            else if(sContent.ReverseFind("MB") >= 0) return (MDB_UINT64)(MDB_INT64)(sContent.ToDouble()*1024*1024);
            else if(sContent.ReverseFind("KB") >= 0) return (MDB_UINT64)(MDB_INT64)(sContent.ToDouble()*1024);
            else return (MDB_UINT64)sContent.ToInt64();
#else
            if(sizeof(long) == 4) return MDB_NTC_ZS_MAX_UINT32;
            else return MDB_NTC_ZS_MAX_UINT64;
#endif
        }
//}
