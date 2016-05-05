#include "Common/mdbFileUtils.h"
//#include "Common/mdbLogInterface.h"
#include "Common/mdbSysThreads.h"
#include "Common/mdbStrUtils.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>

//for os_sun GetDiskFreeSpace
#if defined(OS_SUN)
    #include <sys/mnttab.h>
#endif

//for GetDiskFreeSpace
#if defined(OS_FREEBSD)
    #include <sys/param.h>
    #include <sys/mount.h>
#elif defined(OS_HP) || defined(OS_SUN)
     #include <sys/vfs.h>
#elif !defined(OS_WINDOWS)
    #include <sys/statfs.h>
#endif

//for ftok等
#ifdef OS_WINDOWS
    #include <direct.h>
    #include <io.h>
#else
    #include <sys/types.h>
    #include <sys/ipc.h>
#endif

//for opendir and fnmatch
#ifndef OS_WINDOWS
    #include <dirent.h>
    #include <fnmatch.h>
#endif
//namespace QuickMDB
//{
        bool TMdbNtcPathOper::IsSameFileSystem(const char *pszPath1, const char *pszPath2)
        {
            //errno = 0;
            if ((NULL == pszPath1) || (NULL == pszPath2) || ('\0' == pszPath1[0]) ||('\0' == pszPath2[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                return false;
            }
            
            struct stat StatBuf1;
            struct stat StatBuf2;

            if (0 != stat(pszPath1, &StatBuf1))
            {
                //errno = errno;
                return false;
            }
            
            if (0 != stat(pszPath2, &StatBuf2))
            {
                //errno = errno;
                return false;
            }
            
            if (StatBuf1.st_dev == StatBuf2.st_dev)
            {
                return true;
            }
            else
            {
                return false;
            }

        }

        MDB_INT64 TMdbNtcPathOper::GetCreateTime(const char *pszFilePath)
        {
            //errno = 0;
            if ((NULL == pszFilePath) || ('\0' == pszFilePath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                return -1;
            }
            
            struct stat StatBuf;

            if (0 != stat(pszFilePath, &StatBuf))
            {
                //errno = errno;
                return (time_t)-1;
            }
            MDB_INT64 iTime = -1;

            iTime = StatBuf.st_ctime;

            return iTime;
        }

        MDB_INT64 TMdbNtcPathOper::GetModifyTime(const char *pszFilePath)
        {
            //errno = 0;
            if ((NULL == pszFilePath) || ('\0' == pszFilePath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                return -1;
            }
            struct stat StatBuf;
            if (0 != stat(pszFilePath, &StatBuf))
            {
                //errno = errno;
                return (time_t)-1;
            }
            MDB_INT64 iTime = -1;
            iTime = StatBuf.st_mtime;

            return iTime;
        }

        MDB_INT64 TMdbNtcPathOper::GetAccessTime(const char *pszFilePath)
        {
            //errno = 0;
            if ((NULL == pszFilePath) || ('\0' == pszFilePath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                return -1;
            }
            
            MDB_INT64 iTime = -1;
            struct stat StatBuf;
            if (0 != stat(pszFilePath, &StatBuf))
            {
                //errno = errno;
                return (time_t)-1;
            }
            
            iTime = StatBuf.st_atime;

            return iTime;
        }

        bool TMdbNtcPathOper::IsExist(const char *pszPath)
        {
            if ((NULL == pszPath) || ('\0' == pszPath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                return false;
            }
            
            int rtn = -1;
            
        #ifdef OS_WINDOWS
            rtn = _access(pszPath, 0x00);
        #else
            rtn = access(pszPath, F_OK);    
        #endif
            if(rtn != 0)
            {
                //errno = errno;
            }
            return rtn == 0;
        }

        bool TMdbNtcPathOper::IsFileExist(const char *pszFilePath)
        {
            struct stat buf;
            int iRet = stat(pszFilePath, &buf);
            return (iRet == 0 && (buf.st_mode & S_IFDIR) == 0);
        }

        bool TMdbNtcPathOper::IsDirExist(const char *pszDirPath)
        {
            struct stat buf;
            int iRet = stat(pszDirPath, &buf);
            return (iRet == 0 && (buf.st_mode & S_IFDIR) == S_IFDIR);
        }

        TMdbNtcStringBuffer TMdbNtcPathOper::SearchFile(const char* pszDirPath, const char* pszFileName, bool bSearchSub /* = false */)
        {
            if(pszDirPath == NULL || *pszDirPath == '\0'
                || pszFileName == NULL || *pszFileName == '\0')
            {
                return (const char*)NULL;
            }
            TMdbNtcStringBuffer sFilePath;
            sFilePath.Append(pszDirPath);
            bool bEndWithDelimitation = true;//是否最后以界定符结尾
            if(sFilePath.at(sFilePath.GetLength()-1) != MDB_NTC_ZS_PATH_DELIMITATED_CHAR)
            {
                bEndWithDelimitation = false;
                sFilePath.Append(MDB_NTC_ZS_PATH_DELIMITATED_CHAR);
            }
            sFilePath.Append(pszFileName);
            if(!IsFileExist(sFilePath.c_str()))//如果直接下属的文件不存在
            {
                if(bSearchSub == false)//不搜索子目录，直接返回NULL
                {
                    return (const char*)NULL;
                }
                else
                {
                    sFilePath.Clear();
#ifdef OS_WINDOWS
                    char szFind[MAX_PATH] = {'\0'};
                    if(bEndWithDelimitation)
                    {
                        snprintf(szFind, sizeof(szFind), "%s*.*", pszDirPath);
                    }
                    else
                    {
                        snprintf(szFind, sizeof(szFind), "%s"MDB_NTC_ZS_PATH_DELIMITATED"*.*", pszDirPath);
                    }
                    szFind[sizeof(szFind)-1] = '\0';
                    WIN32_FIND_DATA FindFileData;
                    HANDLE hFind = ::FindFirstFile(szFind, &FindFileData);  
                    if(INVALID_HANDLE_VALUE == hFind)
                    {
                        //errno = GetLastError();
                        TADD_WARNING("TMdbNtcFileScanner::SearchFile failed, errno = %d \n",errno);
                        return (const char*)NULL;
                    }
                    char szSubDirPath[512] = {0};
                    do
                    {
                        if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        {  
                            if(strcmp(FindFileData.cFileName, ".") != 0
                                && strcmp(FindFileData.cFileName, "..") != 0)//文件夹递归
                            {
                                if(bEndWithDelimitation)
                                {
                                    snprintf(szSubDirPath, sizeof(szSubDirPath), "%s""%s", pszDirPath, FindFileData.cFileName);
                                }
                                else
                                {
                                    snprintf(szSubDirPath, sizeof(szSubDirPath), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", pszDirPath, FindFileData.cFileName);
                                }
                                sFilePath = SearchFile(szSubDirPath, pszFileName, bSearchSub);
                                if(!sFilePath.IsEmpty())
                                {
                                    break;
                                }
                            }
                        }
                    }while(FindNextFile(hFind,&FindFileData));
                    FindClose(hFind);
                    hFind = NULL;
#else
                    DIR * pDir = opendir(pszDirPath);
                    if (NULL == pDir)
                    {
                        //errno = errno;
                        TADD_WARNING("TMdbNtcFileScanner::SearchFile failed, errno = %d \n", errno);
                        return (const char*)NULL;
                    }
                    dirent *pDirent = NULL;                    
                    char szSubDirPath[512] = {0};
                    struct stat buf;//获取文件的详细信息
                    do
                    {
                        pDirent = readdir(pDir);
                        if(pDirent == NULL)
                        {
                            break;
                        }
                        if ((0 == strcmp(pDirent->d_name, ".")) 
                            || 0 == strcmp(pDirent->d_name, ".."))
                        {
                            continue;
                        }
                        
                        if(bEndWithDelimitation)
                        {
                            snprintf(szSubDirPath, sizeof(szSubDirPath), "%s%s", pszDirPath, pDirent->d_name);
                        }
                        else
                        {
                            snprintf(szSubDirPath, sizeof(szSubDirPath), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", pszDirPath, pDirent->d_name);
                        }
                        szSubDirPath[sizeof(szSubDirPath)-1] = '\0';

                        //获取文件信息失败，退出本次循环
                        if(stat(szSubDirPath, &buf) != 0)
                        {
                            continue;
                        }
                        //获取文件信息成功，如果不是目录，退出本次循环
                        if(S_ISDIR(buf.st_mode) == 0)
                        {
                            continue;
                        }
                        
                        sFilePath = SearchFile(szSubDirPath, pszFileName, bSearchSub);
                        if(!sFilePath.IsEmpty())
                        {
                            break;
                        }
                    }while(1);
                    closedir(pDir);
                    pDir = NULL;
#endif
                    return sFilePath;
                }
            }
            else
            {
                return sFilePath;
            }
        }

        TMdbNtcStringBuffer TMdbNtcPathOper::SearchDir(const char* pszDirPath, const char* pszSubDirName, bool bSearchSub /* = false */)
        {
            if(pszDirPath == NULL || *pszDirPath == '\0'
                || pszSubDirName == NULL || *pszSubDirName == '\0')
            {
                return (const char*)NULL;
            }
            TMdbNtcStringBuffer sMatchDirPath;
            sMatchDirPath.Append(pszDirPath);
            bool bEndWithDelimitation = true;//是否最后以界定符结尾
            if(sMatchDirPath.at(sMatchDirPath.GetLength()-1) != MDB_NTC_ZS_PATH_DELIMITATED_CHAR)
            {
                bEndWithDelimitation = false;
                sMatchDirPath.Append(MDB_NTC_ZS_PATH_DELIMITATED_CHAR);
            }
            sMatchDirPath.Append(pszSubDirName);
            if(!IsDirExist(sMatchDirPath.c_str()))//如果直接下属的文件不存在
            {
                if(bSearchSub == false)//不搜索子目录，直接返回NULL
                {
                    return (const char*)NULL;
                }
                else
                {
                    sMatchDirPath.Clear();
#ifdef OS_WINDOWS
                    char szFind[MAX_PATH] = {'\0'};
                    if(bEndWithDelimitation)
                    {
                        snprintf(szFind, sizeof(szFind), "%s*.*", pszDirPath);
                    }
                    else
                    {
                        snprintf(szFind, sizeof(szFind), "%s"MDB_NTC_ZS_PATH_DELIMITATED"*.*", pszDirPath);
                    }
                    szFind[sizeof(szFind)-1] = '\0';
                    WIN32_FIND_DATA FindFileData;
                    HANDLE hFind = ::FindFirstFile(szFind, &FindFileData);  
                    if(INVALID_HANDLE_VALUE == hFind)
                    {
                        //errno = GetLastError();
                        TADD_WARNING("TMdbNtcFileScanner::SearchDir failed, errno = %d \n",errno);
                        return (const char*)NULL;
                    }
                    char szSubDirPath[512] = {0};
                    do
                    {
                        if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        {  
                            if(strcmp(FindFileData.cFileName, ".") != 0
                                && strcmp(FindFileData.cFileName, "..") != 0)//文件夹递归
                            {
                                if(bEndWithDelimitation)
                                {
                                    snprintf(szSubDirPath, sizeof(szSubDirPath), "%s""%s", pszDirPath, FindFileData.cFileName);
                                }
                                else
                                {
                                    snprintf(szSubDirPath, sizeof(szSubDirPath), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", pszDirPath, FindFileData.cFileName);
                                }
                                szSubDirPath[sizeof(szSubDirPath)-1] = '\0';
                                sMatchDirPath = SearchDir(szSubDirPath, pszSubDirName, bSearchSub);
                                if(!sMatchDirPath.IsEmpty())
                                {
                                    break;
                                }
                            }
                        }
                    }while(FindNextFile(hFind,&FindFileData));
                    FindClose(hFind);
                    hFind = NULL;
#else
                    DIR * pDir = opendir(pszDirPath);
                    if (NULL == pDir)
                    {
                        //errno = errno;
                        TADD_WARNING("TMdbNtcFileScanner::SearchDir failed, errno = %d \n",errno);
                        return (const char*)NULL;
                    }
                    dirent *pDirent = NULL;                    
                    char szSubDirPath[512] = {0};
                    struct stat buf;//获取文件的详细信息
                    do
                    {
                        pDirent = readdir(pDir);
                        if(pDirent == NULL)
                        {
                            break;
                        }
                        if ((0 == strcmp(pDirent->d_name, ".")) 
                            || 0 == strcmp(pDirent->d_name, ".."))
                        {
                            continue;
                        }

                        if(bEndWithDelimitation)
                        {
                            snprintf(szSubDirPath, sizeof(szSubDirPath), "%s%s", pszDirPath, pDirent->d_name);
                        }
                        else
                        {
                            snprintf(szSubDirPath, sizeof(szSubDirPath), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", pszDirPath, pDirent->d_name);
                        }
                        szSubDirPath[sizeof(szSubDirPath)-1] = '\0';

                        //获取文件信息失败，退出本次循环
                        if(stat(szSubDirPath, &buf) != 0)
                        {
                            continue;
                        }
                        //获取文件信息成功，如果不是目录，退出本次循环
                        if(S_ISDIR(buf.st_mode) == 0)
                        {
                            continue;
                        }
                        
                        sMatchDirPath = SearchDir(szSubDirPath, pszSubDirName, bSearchSub);
                        if(!sMatchDirPath.IsEmpty())
                        {
                            break;
                        }
                    }while(1);
                    closedir(pDir);
                    pDir = NULL;
#endif
                    return sMatchDirPath;
                }
            }
            else
            {
                return sMatchDirPath;
            }
        }

        TMdbNtcStringBuffer TMdbNtcPathOper::GetCurrentPath()
        {
            TMdbNtcStringBuffer sCurPath;
#ifdef OS_WINDOWS
            char* pszCurPath = sCurPath.GetBuffer(256);
            ::GetCurrentDirectory(256, pszCurPath);
            sCurPath.UpdateLength();
#else
            TMdbNtcStrFunc oStrFunc;
            sCurPath = oStrFunc.ReplaceEnv("$(PWD)");
#endif
            return sCurPath;
        }

        TMdbNtcStringBuffer TMdbNtcPathOper::GetParentPath(const char* pszPath, int iLength /* = -1 */)
        {
            if(pszPath == NULL) return (const char*)NULL;
            if(iLength == -1)
            {
                iLength = (int)strlen(pszPath);
            }

            if(iLength == 0) return (const char*)NULL;

            const char* p = pszPath+iLength-1;
            do
            {
                if( *p == '/' || *p == '\\' )
                {
                    break;
                }
                else
                {
                    --p;
                }
            }while(p > pszPath);

            if(p <= pszPath)
            {
                return (const char*)NULL;
            }
            else
            {
                return TMdbNtcStringBuffer(pszPath, (int)(p-pszPath));
            }
        }

        bool TMdbNtcFileOper::MakeFile(const char *pszFilePath)
        {
            FILE* fp = fopen(pszFilePath, "a+");
            if(fp)
            {
                fclose(fp);
                fp = NULL;
                return true;
            }
            else
            {
                return false;
            }
        }

        bool TMdbNtcFileOper::Rename(const char * pszSrcFilePath, const char * pszDestFilePath, bool bSameFileSystem, MDB_UINT64 uiFileSize )
        {
            if ((NULL == pszDestFilePath)|| (NULL == pszSrcFilePath) || ('\0' ==  pszSrcFilePath[0])|| ('\0' == pszDestFilePath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                TADD_WARNING("TMdbNtcFileOper::Rename failed, errno = %d \n",errno);
                return false;
            }
            else if(strcmp(pszSrcFilePath, pszDestFilePath) == 0)
            {
                return true;
            }
            
            if (!IsFileExist(pszSrcFilePath))
            {
                TADD_WARNING("TMdbNtcFileOper::Rename failed! src_file[%s], des_file[%s],but src_file not exist!,errno = %d \n",pszSrcFilePath,pszDestFilePath,errno);
                return false;
            }

            TMdbNtcStringBuffer sParentPath = GetParentPath(pszDestFilePath);
            if(!TMdbNtcDirOper::MakeFullDir(sParentPath.c_str()))
            {
                return false;
            }
            else if(!TMdbNtcFileOper::Remove(pszDestFilePath))
            {
                TADD_WARNING("TMdbNtcFileOper::Rename failed!src_file[%s], des_file[%s],Remove des_file failed, errno = %d \n", pszSrcFilePath,pszDestFilePath, errno);
                return false;
            }
            
            if( uiFileSize > 0 )
            {
                MDB_UINT64 uiFileSizeTemp = 0;
                if( GetFileSize( pszSrcFilePath, uiFileSizeTemp ) )
                {
                    if( uiFileSize > uiFileSizeTemp )
                    {
                        TADD_WARNING("TMdbNtcFileOper::Rename failed!src_file[%s], des_file[%s], uiFileSize=%"MDB_NTC_ZS_FORMAT_UINT64" > uiFileSizeTemp=%"MDB_NTC_ZS_FORMAT_UINT64", nothing to do \n", pszSrcFilePath,pszDestFilePath,uiFileSize,uiFileSizeTemp);
                        return false;
                    }
                }
                else
                {
                    TADD_WARNING("TMdbNtcFileOper::Rename failed!src_file[%s], des_file[%s], GetFileSize is error \n", pszSrcFilePath,pszDestFilePath);
                    return false;
                }
            }

        #if defined(OS_WINDOWS)
            ::rename(pszSrcFilePath, pszDestFilePath);
        #else
            if (bSameFileSystem)
            {
                if (0 == rename(pszSrcFilePath, pszDestFilePath))
                {
                    return true;
                }
                else
                {
                    //errno = errno;
                    TADD_WARNING("TMdbNtcFileOper::Rename failed, src_file[%s], des_file[%s],errno = %d \n",pszSrcFilePath,pszDestFilePath,errno);;
                    return false;
                }
            }
            else
            {
                //用系统命令处理改名操作
                char sCmdLine[1024] = {0};
                snprintf(sCmdLine, sizeof(sCmdLine), "mv %s %s", 
                         pszSrcFilePath, pszDestFilePath);
                sCmdLine[sizeof(sCmdLine)-1] = '\0';
                system(sCmdLine);
            }

        #endif

            if (!IsFileExist(pszSrcFilePath)) 
            {
                return true;
            }
            else
            {
                TADD_WARNING("TMdbNtcFileOper::Rename failed, src_file[%s], des_file[%s],errno = %d \n",pszSrcFilePath,pszDestFilePath,errno);
                return false;
            }
        }

        bool TMdbNtcFileOper::Remove(const char *pszFilePath)
        {
            if ((NULL == pszFilePath) || ('\0' == pszFilePath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                TADD_WARNING("TMdbNtcFileOper::Remove failed, errno = %d \n",errno);
                return false;
            }
            
            if (!IsExist(pszFilePath))
            {
                return true;
            } 

        #ifdef OS_WINDOWS
            if(DeleteFile(pszFilePath))
            {
                return true;
            }
            else
            {
                //errno = GetLastError();
                TADD_WARNING("TMdbNtcFileOper::Remove [%s] failed, errno = %d \n",pszFilePath, errno);
                return false;
            }
        #else
            if (0 != ::remove(pszFilePath)) 
            {
                //errno = errno;
                TADD_WARNING("TMdbNtcFileOper::Remove [%s] failed, errno = %d \n",pszFilePath, errno);
                return false;
            }
            else
            {
                return true;
            }
        #endif
        }

        bool TMdbNtcFileOper::Copy(const char * pszSrcFilePath, const char * pszDestFilePath, bool bFailIfExists /* = false */)
        {
            if ((NULL == pszSrcFilePath) || (NULL == pszDestFilePath) || ('\0' == pszSrcFilePath[0]) || ('\0' == pszDestFilePath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                TADD_WARNING("TMdbNtcFileOper::Copy failed, errno = %d \n",errno);
                return false;
            }
            if(TMdbNtcFileOper::IsFileExist(pszDestFilePath))
            {
                if(bFailIfExists)
                {
                    TADD_WARNING("File exists, errno = %d \n",errno);
                    return false;
                }
            }
            else
            {
                TMdbNtcStringBuffer sParentPath = TMdbNtcPathOper::GetParentPath(pszDestFilePath);
                TMdbNtcDirOper::MakeFullDir(sParentPath.c_str());
            }
#ifdef OS_WINDOWS
            return CopyFile(pszSrcFilePath, pszDestFilePath, bFailIfExists)==TRUE;
#else
            std::ifstream input(pszSrcFilePath, std::ios::binary);
            if(!input)
            {
                TADD_WARNING("Open file failed, errno = %d \n",errno);
                return false;
            }
            std::ofstream output(pszDestFilePath, std::ios::binary);
            if(!output)
            {
                input.close();
                TADD_WARNING("Create file failed, errno = %d \n",errno);
                return false;
            }
            output << input.rdbuf();
            input.close();
            output.close();
            return true;
#endif            
        }

        bool TMdbNtcFileOper::ReadContent(const char *pszFilePath, TMdbNtcStringBuffer& sContent)
        {
            bool bRet = true;
            struct stat file_stat;
            int iRet = stat(pszFilePath, &file_stat);
            bRet = (iRet == 0 && (file_stat.st_mode & S_IFDIR) == 0);
            if(!bRet) return bRet;
            sContent.Clear();
            if(file_stat.st_size == 0) return true;
            if(NULL == sContent.Reserve((MDB_UINT32)file_stat.st_size+1)) return false;
            FILE* fp = fopen(pszFilePath, "rb");
            if(fp == NULL) return false;
            size_t uiReadSize = fread(sContent.GetBuffer(), 1, (MDB_UINT32)file_stat.st_size, fp);
            fclose(fp);
            fp = NULL;
            sContent.UpdateLength((int)uiReadSize);
            if(uiReadSize != (MDB_UINT32)file_stat.st_size) return false;
            else return true;
        }

        bool TMdbNtcFileOper::WriteContent(const char *pszFilePath, const void* pszContent, int iLength /* = -1 */)
        {
            bool bRet = true;
            if(iLength == -1) iLength = pszContent? (int)strlen((const char*)pszContent):0;
            FILE* fp = fopen(pszFilePath, "wb");
            if(fp == NULL) return false;
            size_t uiWriteSize = 0;
            if(iLength > 0)
            {
                uiWriteSize = fwrite(pszContent, 1, (MDB_UINT32)iLength, fp);
            }
            fclose(fp);
            fp = NULL;
            if(uiWriteSize != (size_t)iLength) return false;
            else return bRet;
        }

        bool TMdbNtcFileOper::AppendContent(const char *pszFilePath, const void* pszContent, int iLength /* = -1 */)
        {
            bool bRet = true;
            if(iLength == -1) iLength = pszContent? (int)strlen((const char*)pszContent):0;
            FILE* fp = fopen(pszFilePath, "ab");
            if(fp == NULL) return false;
            size_t uiWriteSize = 0;
            if(iLength > 0)
            {
                uiWriteSize = fwrite(pszContent, 1, (MDB_UINT32)iLength, fp);
            }
            fclose(fp);
            fp = NULL;
            if(uiWriteSize != (size_t)iLength) return false;
            else return bRet;
        }

        const char* TMdbNtcFileOper::GetFileExt(const char * pszFilePath)
        {
            if (NULL == pszFilePath || '\0' == *pszFilePath)
            {
                return "";
            }
            const char* pszFileExt = strrchr(pszFilePath, '.');
            if(pszFileExt) return pszFileExt+1;
            else return "";
        }

        const char* TMdbNtcFileOper::GetFileName(const char * pszFilePath)
        {
            if (NULL == pszFilePath || '\0' == *pszFilePath)
            {
                return "";
            }
            int iLength = (int)strlen(pszFilePath);
            const char* p = pszFilePath+iLength, *pszNameStart = NULL;
            if( pszFilePath[iLength-1] == '/' || pszFilePath[iLength-1] == '\\' )
            {
                return "";                
            }
            do
            {
                --p;
                if(*p == '/' || *p == '\\')
                {
                    if(pszNameStart)
                    {
                        break;
                    }
                }
                else if(isgraph(*p) || *(unsigned char*)p > 127)
                {
                    pszNameStart = p;
                }
            } while (p != pszFilePath);
            if(pszNameStart) return pszNameStart;
            else return "";
        }

        TMdbNtcStringBuffer TMdbNtcFileOper::GetPureFileName(const char * pszFilePath)
        {
            TMdbNtcStringBuffer sFileName;
            if (NULL == pszFilePath || '\0' == *pszFilePath)
            {
                return sFileName;
            }
            int iLength = (int)strlen(pszFilePath);
            const char* p = pszFilePath+iLength, *pszNameStart = NULL, *pszNameEnd = NULL;
            do 
            {
                --p;
                if(*p == '/' || *p == '\\')
                {
                    if(pszNameStart)
                    {
                        break;
                    }                    
                }
                else if(isgraph(*p) || *(unsigned char*)p > 127)
                {
                    pszNameStart = p;
                    if(!pszNameEnd)
                    {
                        pszNameEnd = p;
                    }
                }                
            } while (p != pszFilePath);
            if(pszNameStart)
            {
                sFileName = TMdbNtcStringBuffer(pszNameStart, (int)(pszNameEnd-pszNameStart+1));
            }
            return sFileName;
        }

        #ifndef OS_WINDOWS
        key_t TMdbNtcFileOper::Ftok(const char *pszFilePath)
        {
            bool bRet = false;
            key_t key = -1;
            if ((NULL == pszFilePath) || ('\0' == pszFilePath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                return key;
            }
            bRet = TMdbNtcPathOper::IsExist(pszFilePath);
            if(!bRet)
            {
                bRet = TMdbNtcDirOper::MakeFullDir(pszFilePath);
            }
            if(bRet)
            {
                key = ftok(pszFilePath, 1110);
                //errno = errno;
            }
            return key;        
        }
        #endif


        bool TMdbNtcFileOper::GetFileSize(const char *pszFilePath, MDB_UINT64& uiSize)
        {
            if (!TMdbNtcFileOper::IsExist(pszFilePath))
            {
                TADD_WARNING("TMdbNtcFileOper::GetFileSize[%s] failed, errno = %d \n", pszFilePath?pszFilePath:"",errno);
                return false;
            }

            FILE* fp = fopen(pszFilePath, "r");
            if (NULL == fp)
            {
                //errno = errno;
                TADD_WARNING("TMdbNtcFileOper::GetFileSize[%s] failed, errno = %d \n", pszFilePath,errno);
                return false;
            }

            fseek(fp, 0L, SEEK_END);
            uiSize = (MDB_UINT64)ftell(fp);
            fclose(fp);
            
            return true;
        }

        bool TMdbNtcDirOper::MakeDir(const char *pszDirPath)
        {
            int rtn = -1;
            if (NULL == pszDirPath || *pszDirPath == '\0')
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                TADD_WARNING("MakeDir failed, pszDirPath is NULL \n" );
                return false;
            }            
            else if(IsDirExist(pszDirPath))//判断目录是否已经存在
            {
                return true;
            }
            
        #ifdef OS_WINDOWS
            rtn = ::_mkdir(pszDirPath);
        #else
            rtn = ::mkdir(pszDirPath, O_CREAT|S_IRUSR|S_IWUSR|S_IXUSR);
        #endif 
            if( 0 == rtn )
            {
                return true;
            }
            else
            {
                //errno = errno;
                //MDB_NTC_ADD_GLOBAL_WARN_TF("MakeDir[%s] failed, errno = %d \n", pszDirPath,errno );
                return false;
            }
        }

        bool TMdbNtcDirOper::MakeFullDir(const char *pszDirPath)
        {
            bool bRet = true;
            if (NULL == pszDirPath || *pszDirPath == '\0')
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                TADD_WARNING("MakeFullDir failed, pszDirPath is NULL \n" );
                return false;
            }
            else if(!MakeDir(pszDirPath))
            {
                MDB_UINT32 uiLength = (MDB_UINT32)strlen(pszDirPath);
                if(*(pszDirPath+uiLength-1) == ':') return true;
                TMdbNtcStringBuffer sParentPath = GetParentPath(pszDirPath, (int)uiLength);
                if(sParentPath == "." || sParentPath == ".."
                    || sParentPath.IsEmpty() || sParentPath[sParentPath.length()-1] == ':')
                {
                    TADD_WARNING("MakeFullDir[%s] failed, errno = %d \n", pszDirPath,errno);
                    return false;
                }
                else if(MakeFullDir(sParentPath.c_str()))//创建父分类的目录
                {
                    return MakeDir(pszDirPath);
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return bRet;
            }
        }

        bool TMdbNtcDirOper::Rename(const char * pszSrcPath, const char * pszDestPath, bool bSameFileSystem)
        {
            if ((NULL == pszSrcPath)|| (NULL == pszDestPath) || ('\0' ==  pszSrcPath[0])|| ('\0' == pszDestPath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                TADD_WARNING("TMdbNtcDirOper::Rename failed, errno = %d \n",errno);
                return false;
            }
            else if(strcmp(pszSrcPath, pszDestPath) == 0)
            {
                return true;
            }
            
            if (!IsDirExist(pszSrcPath))
            {
                TADD_WARNING("TMdbNtcDirOper::Rename failed, errno = %d \n",errno);
                return false;
            }    
            
#if defined(OS_WINDOWS)
            TMdbNtcStringBuffer sParentPath = GetParentPath(pszDestPath);
            if(!sParentPath.IsEmpty() && sParentPath[sParentPath.length()-1] != ':')
            {
                if(!TMdbNtcDirOper::MakeFullDir(sParentPath.c_str())) return false;
            }
            ::rename(pszSrcPath, pszDestPath);
#else
            if (bSameFileSystem)
            {
                if (0 == rename(pszSrcPath, pszDestPath))
                {
                    return true;
                }
                else
                {
                    //errno = errno;
                    TADD_WARNING("TMdbNtcDirOper::Rename failed, errno = %d \n",errno);
                    return false;
                }
            }
            else
            {
                //用系统命令处理改名操作
                char sCmdLine[1024] = {0};
                snprintf(sCmdLine, sizeof(sCmdLine), "mv %s %s", 
                    pszSrcPath, pszDestPath);
                sCmdLine[sizeof(sCmdLine)-1] = '\0';
                system(sCmdLine);
            }
            
#endif
            
            if (!IsDirExist(pszSrcPath)) 
            {
                return true;
            }
            else
            {
                TADD_WARNING("TMdbNtcDirOper::Rename failed, errno = %d \n",errno);;
                return false;
            }
        }

        bool TMdbNtcDirOper::Copy(const char * pszSrcDirPath, const char * pszDestDirPath)
        {
            if ((NULL == pszSrcDirPath) || (NULL == pszDestDirPath) || ('\0' == pszSrcDirPath[0]) || ('\0' == pszDestDirPath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                TADD_WARNING("TMdbNtcDirOper::Copy failed, errno = %d \n",errno);
                return false;
            }
            
            //判断是否有原文件
            if (!IsDirExist(pszSrcDirPath))
            {
                TADD_WARNING("TMdbNtcDirOper::Copy failed, errno = %d \n",errno);
                return false;
            }
            
            char sCmdLine[4096] = {0};
/*
            if(bIncludeSelf)
            {
                TMdbNtcStringBuffer sDestDir;
                sDestDir<<pszDestDirPath<<MDB_NTC_ZS_PATH_DELIMITATED<<TMdbNtcFileOper::GetFileName(pszSrcDirPath);
                if(!TMdbNtcDirOper::MakeFullDir(sDestDir.c_str())) return false;
#if defined(OS_WINDOWS)
                snprintf(sCmdLine, sizeof(sCmdLine), "copy /y \"%s\" \"%s\"", pszSrcDirPath, sDestDir.c_str());
#else
                snprintf(sCmdLine, sizeof(sCmdLine), "cp -rf \"%s\" \"%s\"", pszSrcDirPath, sDestDir.c_str());
#endif
            }
            else
            {
*/              
                //如果目的路径已存在，则不创建，如果不存在，则创建到父路径
                if(!IsDirExist(pszDestDirPath))
                {
                    TMdbNtcStringBuffer  sParentPath;
                    int iLength = (int)strlen(pszDestDirPath);
                    const char* p = pszDestDirPath+iLength-1;
                    if( *p == '/' || *p == '\\' )
                    {
                        sParentPath = TMdbNtcPathOper::GetParentPath(pszDestDirPath);
                        sParentPath = TMdbNtcPathOper::GetParentPath(sParentPath.c_str());
                    }
                    else
                    {
                        sParentPath = TMdbNtcPathOper::GetParentPath(pszDestDirPath);
                    }

                    if (!TMdbNtcDirOper::MakeFullDir(sParentPath.c_str()))
                    {
                        TADD_WARNING("TMdbNtcDirOper::Copy failed, errno = %d \n",errno);
                        return false;
                    }
                    
                }
                
#if defined(OS_WINDOWS)
                snprintf(sCmdLine, sizeof(sCmdLine), "copy /y \"%s"MDB_NTC_ZS_PATH_DELIMITATED"*.*\" \"%s\"", pszSrcDirPath, pszDestDirPath);
#else
                snprintf(sCmdLine, sizeof(sCmdLine), "cp -rf %s %s", pszSrcDirPath, pszDestDirPath);
#endif
//            }
            sCmdLine[sizeof(sCmdLine)-1] = '\0';
            
            int iRet = system(sCmdLine);
            if(iRet != 0)
            {
                //errno = errno;
                TADD_WARNING("TMdbNtcDirOper::Copy failed, errno = %d \n",errno);
                return false;
            }
            else
            {
                if (IsDirExist(pszDestDirPath))
                {
                    return true;
                }
                else
                {
                    TADD_WARNING("TMdbNtcDirOper::Copy failed, errno = %d \n",errno);
                    return false;
                }
            }
        }

        bool TMdbNtcDirOper::Remove(const char *pszDirPath, bool bForce /* = false */)
        {   
            if ((NULL == pszDirPath) || ('\0' == pszDirPath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                TADD_WARNING("Rmdir failed, pszDirPath is NULL \n");
                return false;
            }
            int iRet = -1;
#ifdef OS_WINDOWS
            iRet = _rmdir(pszDirPath);
#else
            iRet = ::rmdir(pszDirPath);
#endif
            if( 0 == iRet || errno == ENOENT)
            {
                return true;
            }
            else
            {
            #if defined(OS_HP) ||defined(OS_SUN)
                if(bForce && errno == EEXIST)
            #else
                if(bForce && errno == ENOTEMPTY)
            #endif
                {
                    //扫描子目录挨个删除
#ifdef OS_WINDOWS
                    SHFILEOPSTRUCT FileOp={0};
                    FileOp.fFlags = FOF_NOCONFIRMATION; //不出现确认对话框
                    //FileOp.fFlags |= FOF_ALLOWUNDO;//允许放回回收站
                    FileOp.pFrom = pszDirPath;
                    FileOp.pTo = NULL;      //一定要是NULL
                    FileOp.wFunc = FO_DELETE;    //删除操作
                    iRet = SHFileOperation(&FileOp);
                    return iRet==0?true:false;
#else
                    iRet = system(TMdbNtcStringBuffer(1024, "rm -rf %s", pszDirPath).c_str());
                    return iRet==0?true:false;
#endif
                }
                else
                {
                    //errno = errno;
                    TADD_WARNING("Rmdir[%s] failed, errno = %d \n", pszDirPath,errno);
                    return false;
                }
            }
        }

        bool TMdbNtcDirOper::GetDiskFreeSpace(const char *pszDirPath, MDB_UINT64& uiSize)
        {
            int iRet = 0;
            if ((NULL == pszDirPath) || ('\0' == pszDirPath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                TADD_WARNING("GetDiskFreeSpace failed, pszDirPath is NULL \n");
                return false;
            }
        #ifdef OS_WINDOWS
            char    driver[5] = {0};
            DWORD   SectorsPerCluster,
                BytesPerSector,
                NumberOfFreeClusters,
                TotalNumberOfClusters;
            
            if((pszDirPath[0] >='A')
                &&(pszDirPath[0] <='z')
                &&(pszDirPath[1] ==':') )
            {
                driver[0] = pszDirPath[0];
                driver[1] = pszDirPath[1];
//                driver[2] = '\\';  /*Added a '\' in the end of the string wangjin 2003-11-24*/
                driver[2] = MDB_NTC_ZS_PATH_DELIMITATED_CHAR;
                driver[3] = '\0';
            } 
            else
            {
                strncpy(driver, "\\", sizeof(driver)-1);
            }
            
            
            if (::GetDiskFreeSpace(driver, &SectorsPerCluster, &BytesPerSector,
                                 &NumberOfFreeClusters,&TotalNumberOfClusters))
            {
                uiSize =  (MDB_INT64)((DWORDLONG) SectorsPerCluster*BytesPerSector*
        NumberOfFreeClusters / 1024);
            }
            else
            {
                //errno = GetLastError();
                TADD_WARNING("TMdbNtcFileOper::GetDiskFreeSpace failed, errno = %d \n",errno);
                return false;
            }
        #elif defined(OS_SUN)
            struct statvfs buf;
            iRet = statvfs(pszDirPath, &buf);
            if(iRet == 0)
            {
                /*
                free blocks                  (f_bfree)
                free blocks for non-root     (f_bavail)
                */
                uiSize += buf.f_bavail*(MDB_INT64)buf.f_frsize;
            }
            /*
            struct statvfs buf;
            struct mnttab mp;
            FILE *fp;
            
            if((fp = fopen("/etc/mnttab","r" )) == NULL )  
            {
                //errno = errno;
                MDB_NTC_ADD_GLOBAL_WARN_TF("GetDiskFreeSpace failed, errno = %d \n",errno);
                return false;
            }
            
            while (getmntent(fp, &mp) != -1)
            {
                if (strncmp(mp.mnt_mntopts, "ignore", 6)== 0) continue; 
                if (mp.mnt_mountp != NULL) 
                {
                    if (statvfs(mp.mnt_mountp,&buf) == -1 ) 
                    {
                        //errno = errno;
                        fclose(fp);
                        MDB_NTC_ADD_GLOBAL_WARN_TF("GetDiskFreeSpace failed, errno = %d \n",errno);
                        return false;
                    } 
                    else 
                    {   //size in kilo bytes
                        uiSize += (MDB_INT64)( buf.f_bfree * (buf.f_frsize/1024.0));
                    }
                }
            }   
            fclose(fp);
            */
        #else

            struct statfs diskInfo;
            iRet = statfs((char*)pszDirPath, &diskInfo);
            if(iRet == 0)
            {
                // 每个block里面包含的字节数
                MDB_INT64 blocksize = diskInfo.f_bsize;
                //再计算下剩余的空间大小
                uiSize = diskInfo.f_bavail * (MDB_UINT64)blocksize;
            }
            else
            {
                //errno = errno;
            }
        #endif
            if( 0 == iRet )
            {
                return true;
            }
            else
            {
                TADD_WARNING("GetDiskFreeSpace failed, errno = %d \n",errno);
                return false;
            }
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcFileScanner, TMdbNtcBaseObject);
        TMdbNtcFileScanner::TMdbNtcFileScanner()
        {
            m_lsFilterPattern.SetAutoRelease(true);
            m_lsResultPath.SetAutoRelease(true);
        }

        TMdbNtcFileScanner::~TMdbNtcFileScanner()
        {
            Clear();
        }

        void TMdbNtcFileScanner::Clear()
        {
            ClearResult();
            ClearFilter();
        }

        void TMdbNtcFileScanner::AddFilter(const char* pszPattern)
        {
            if ( NULL == pszPattern || '\0' == pszPattern[0] )
            {
                return ;
            }
            
            TMdbNtcStringObject *pObject = new TMdbNtcStringObject(pszPattern);
            if (NULL == m_lsFilterPattern.FindData(*pObject))
            {
                m_lsFilterPattern.AddTail(pObject);
            }
            else
            {
                delete pObject;
                pObject = NULL;
            }
        }

        bool TMdbNtcFileScanner::DelFilter(const char* pszPattern)
        {
            if ( NULL == pszPattern || '\0' == pszPattern[0] || m_lsFilterPattern.GetSize() < 1 )
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                return false;
            }
            
            if (0 < m_lsFilterPattern.Remove(TMdbNtcStringObject(pszPattern)))
            {
                return true;
            }
            
            return false;
        }

#ifdef OS_WINDOWS
        bool mdb_ntc_match(const char *pattern, const char *content)
        {
         // if we reatch both end of two string, we are done
         if ('\0' == *pattern && '\0' == *content)
          return true;
         /* make sure that the characters after '*' are present in second string.
              this function assumes that the first string will not contain two
               consecutive '*'*/
         if ('*' == *pattern && '\0' != *(pattern + 1) && '\0' == *content)
          return false;
         // if the first string contains '?', or current characters of both
            // strings match
         if ('?' == *pattern || *pattern == *content)
          return match(pattern + 1, content + 1);
         /* if there is *, then there are two possibilities
               a) We consider current character of second string
               b) We ignore current character of second string.*/
         if ('*' == *pattern)
          return match(pattern + 1, content) || match(pattern, content + 1);
         return false;
        }
#endif

        bool TMdbNtcFileScanner::IsSatisfy(const char* pszName)
        {
            if(pszName == NULL || *pszName == '\0') return false;
            if(m_lsFilterPattern.IsEmpty()) return true;
            //通配符匹配
            TMdbNtcBaseList::iterator itPattern = m_lsFilterPattern.IterBegin(); 
            TMdbNtcBaseList::iterator itEnd = m_lsFilterPattern.IterEnd();
            for(; itPattern != itEnd; ++itPattern)
            {
                TMdbNtcStringObject *pStrNode = static_cast<TMdbNtcStringObject*>(itPattern.data());

#ifdef OS_WINDOWS
                if(mdb_ntc_match(pStrNode->c_str(), pszName)) return true;
#else
                if (0 == fnmatch(pStrNode->c_str(), pszName, FNM_NOESCAPE))
                {
                    return true;
                }
#endif
            }
            return false;
        }

        bool TMdbNtcFileScanner::scanFile(const char* pszScanPath, bool bIncludeSubDir, int iMaxNum)
        {
            if ((NULL == pszScanPath) || ('\0' == pszScanPath[0]))
            {
                //errno = MDB_NTC_ERROR_PARAMETER;
                TADD_WARNING("TMdbNtcFileScanner::ScanFile failed, errno = %d \n",errno );
                return false;
            }
            if(iMaxNum == 0)
            {
                return true;
            }
            m_sScanPath = pszScanPath;
            if(m_sScanPath.GetLength() > 0 && m_sScanPath[m_sScanPath.GetLength()-1] == MDB_NTC_ZS_PATH_DELIMITATED_CHAR)
            {
                m_sScanPath.Delete((int)m_sScanPath.GetLength()-1);
            }
            TMdbNtcBaseList DirList;
            DirList.SetAutoRelease(true);
            const char* pszFileName = NULL;
        #ifdef OS_WINDOWS
            char szFind[MAX_PATH] = {'\0'};
            snprintf(szFind, sizeof(szFind), "%s"MDB_NTC_ZS_PATH_DELIMITATED"*.*", m_sScanPath.c_str());
            szFind[sizeof(szFind)-1] = '\0';
            WIN32_FIND_DATA FindFileData;
            HANDLE hFind = ::FindFirstFile(szFind,&FindFileData);  
            if(INVALID_HANDLE_VALUE == hFind)
            {
                //errno = GetLastError();
                TADD_WARNING("TMdbNtcFileScanner::ScanFile failed, errno = %d \n",errno);
                return false;
            }
            char szFilePath[512] = {0};
            do
            {
                pszFileName = FindFileData.cFileName;
                if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if(bIncludeSubDir
                        && strcmp(FindFileData.cFileName, ".") != 0
                        && strcmp(FindFileData.cFileName, "..") != 0)//文件夹递归
                    {
                        snprintf(szFilePath, sizeof(szFilePath), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", m_sScanPath.c_str(), pszFileName);
                        szFilePath[sizeof(szFilePath)-1] = '\0';
                        DirList.AddTail(new TMdbNtcStringObject(szFilePath));
                    }
                }
                else
                {                    
        #else
            DIR * pDir = opendir(m_sScanPath.c_str());
            if (NULL == pDir)
            {
                //errno = errno;
                TADD_WARNING("TMdbNtcFileScanner::ScanFile failed, errno = %d \n",errno);
                return false;
            }    
            dirent *pDirent = NULL;
            char szFilePath[512] = {0};
            struct stat buf;//获取文件的详细信息
            while (1)
            {
                pDirent = readdir(pDir);
                if(pDirent == NULL)
                {
                    break;
                }
                pszFileName = pDirent->d_name;
                if ((0 == strcmp(pszFileName, ".")) 
                    || 0 == strcmp(pszFileName, ".."))
                {
                    continue;
                }
                
                *szFilePath = '\0';
                snprintf(szFilePath, sizeof(szFilePath), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", m_sScanPath.c_str(), pszFileName);
                szFilePath[sizeof(szFilePath)-1] = '\0';
                //获取文件属性失败，跳出，继续下一次循环
                if(stat(szFilePath, &buf) != 0)
                {
                    continue;
                }
                //获取文件属性成功，判断文件类型是否是目录
                if(S_ISDIR(buf.st_mode) && bIncludeSubDir )
                {
                    
                    DirList.AddTail(new TMdbNtcStringObject(szFilePath));
                }
                else
                {                    
        #endif
                    if(!IsSatisfy(pszFileName))//如果不满足
                    {
                        continue;
                    }
                    snprintf(szFilePath, sizeof(szFilePath), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", m_sScanPath.c_str(), pszFileName);
                    szFilePath[sizeof(szFilePath)-1] = '\0';
                    m_lsResultPath.AddTail(new TMdbNtcStringObject(szFilePath));
                    if ((-1 != iMaxNum) && 
                        (((unsigned int)iMaxNum) <= m_lsResultPath.GetSize()))
                    {
                        break;
                    }
        #ifdef OS_WINDOWS
                }
            }while(FindNextFile(hFind,&FindFileData));
            FindClose(hFind);
            hFind = NULL;
        #else
                }
            }
            closedir(pDir);
            pDir = NULL;
        #endif
            if (!DirList.IsEmpty())
            {
                if(iMaxNum == -1 || m_lsResultPath.GetSize() <((unsigned int)iMaxNum))
                {
                    TMdbNtcBaseList::iterator itor = DirList.IterBegin();
                    TMdbNtcBaseList::iterator itorEnd = DirList.IterEnd();
                    for(; itor != itorEnd; ++itor)
                    {
                        TMdbNtcStringObject *pStr = (TMdbNtcStringObject *)itor.data();                
                        scanFile(pStr->c_str(), bIncludeSubDir, iMaxNum);
                        if (iMaxNum != -1 && m_lsResultPath.GetSize() >= ((unsigned int)iMaxNum))
                        {
                            break;
                        }
                    }
                }
                DirList.Clear();
            }
            if (m_itCur != m_lsResultPath.IterBegin())
            {
                m_itCur = m_lsResultPath.IterBegin();
            }
            return true;
        }

        bool TMdbNtcFileScanner::scanDir(const char* pszScanPath, bool bIncludeSubDir, int iMaxNum)
        {
            if ((NULL == pszScanPath) || ('\0' == pszScanPath[0]))
            {
                TADD_WARNING("TMdbNtcFileScanner::ScanDir failed, errno = %d \n",errno );
                return false;
            }
            if(iMaxNum == 0)
            {
                return true;
            }
            m_sScanPath = pszScanPath;
            if(m_sScanPath.GetLength() > 0 && m_sScanPath[m_sScanPath.GetLength()-1] == MDB_NTC_ZS_PATH_DELIMITATED_CHAR)
            {
                m_sScanPath.Delete((int)m_sScanPath.GetLength()-1);
            }
            TMdbNtcBaseList DirList;
            DirList.SetAutoRelease(true);
            const char* pszFileName = NULL;
#ifdef OS_WINDOWS
            char szFind[MAX_PATH] = {'\0'};
            snprintf(szFind, sizeof(szFind), "%s"MDB_NTC_ZS_PATH_DELIMITATED"*.*", m_sScanPath.c_str());
            szFind[sizeof(szFind)-1] = '\0';
            WIN32_FIND_DATA FindFileData;
            HANDLE hFind = ::FindFirstFile(szFind,&FindFileData);  
            if(INVALID_HANDLE_VALUE == hFind)
            {
                //errno = GetLastError();
                TADD_WARNING("TMdbNtcFileScanner::ScanDir failed, errno = %d \n",errno);
                return false;
            }
            do
            {
                pszFileName = FindFileData.cFileName;
                if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {  
                    if(strcmp(pszFileName, ".") != 0
                        && strcmp(pszFileName, "..") != 0)//文件夹递归
                    {
                        DirList.AddTail(new TMdbNtcStringObject(MAX_PATH, "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", m_sScanPath.c_str(), pszFileName));
                    }
                }
            }while(FindNextFile(hFind,&FindFileData));
            FindClose(hFind);
            hFind = NULL;
#else
            DIR * pDir = opendir(m_sScanPath.c_str());
            if (NULL == pDir)
            {
                TADD_WARNING("TMdbNtcFileScanner::ScanDir failed, errno = %d \n",errno);
                return false;
            }    
            dirent *pDirent = NULL;
            char szFilePath[512] = {0};
            struct stat buf;//获取文件的详细信息
            while (1)
            {
                pDirent = readdir(pDir);
                if(pDirent == NULL)
                {
                    break;
                }
                pszFileName = pDirent->d_name;
                if ((0 == strcmp(pszFileName, ".")) 
                    || 0 == strcmp(pszFileName, ".."))
                {
                    continue;
                }
                
                *szFilePath = '\0';
                
                snprintf(szFilePath, sizeof(szFilePath), "%s"MDB_NTC_ZS_PATH_DELIMITATED"%s", m_sScanPath.c_str(), pszFileName);
                szFilePath[sizeof(szFilePath)-1] = '\0';
                
                if(stat(szFilePath, &buf) == 0)
                {
                    if(S_ISDIR(buf.st_mode))
                    {
                        
                        DirList.AddTail(new TMdbNtcStringObject(szFilePath));
                    }
                }
            }
            closedir(pDir);
            pDir = NULL;
#endif
            if (!DirList.IsEmpty())
            {
                TMdbNtcBaseList::iterator itor = DirList.IterBegin();
                TMdbNtcBaseList::iterator itorEnd = DirList.IterEnd();
                for(; itor != itorEnd; ++itor)
                {
                    TMdbNtcStringObject *pStr = (TMdbNtcStringObject *)itor.data();
                    if(IsSatisfy(TMdbNtcFileOper::GetFileName(pStr->c_str())))//如果满足
                    {
                        m_lsResultPath.AddTail(new TMdbNtcStringObject(*pStr));
                        if ((-1 != iMaxNum) && 
                            (((unsigned int)iMaxNum) <= m_lsResultPath.GetSize()))
                        {
                            break;
                        }
                    }
                    if(bIncludeSubDir)
                    {
                        scanDir(pStr->c_str(), bIncludeSubDir, iMaxNum);
                        if (iMaxNum != -1 && m_lsResultPath.GetSize() >= ((unsigned int)iMaxNum))
                        {
                            break;
                        }
                    }
                }
                DirList.Clear();
            }
            if (m_itCur != m_lsResultPath.IterBegin())
            {
                m_itCur = m_lsResultPath.IterBegin();
            }
            return true;
        }

        void TMdbNtcFileScanner::Sort(bool bSortAsc, const TMdbNtcObjCompare &oCompare)
        {
            m_lsResultPath.Sort(TMdbNtcQuickSort(bSortAsc), oCompare);
            if (m_itCur != m_lsResultPath.IterBegin())
            {
                m_itCur = m_lsResultPath.IterBegin();
            }
        }

        const char* TMdbNtcFileScanner::GetNext()
        {
            if (m_itCur == m_lsResultPath.IterEnd())
            {
                return NULL;
            }
            TMdbNtcBaseList::iterator itor = m_itCur++;
            return static_cast<TMdbNtcStringObject*>(itor.data())->c_str();
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcFileLock, TMdbNtcBaseObject);
        TMdbNtcFileLock::TMdbNtcFileLock(const char *pszFilePath)
        {
            m_bLocked = false;
            m_sFilePath = pszFilePath;
        #ifndef OS_WINDOWS
                m_hFile = -1;
        #else
                m_hFile = INVALID_HANDLE_VALUE;
        #endif

        }

        TMdbNtcFileLock::~TMdbNtcFileLock()
        {
            Unlock();
        }

        bool TMdbNtcFileLock::Lock(bool bBlock /* = false */)
        {
        #ifdef OS_WINDOWS
            BOOL fileQ = false;
            
            //打开文件
            m_hFile = CreateFile((m_sFilePath.c_str()),              // open file
                                      GENERIC_READ,                    // open for writing 
                                      0,                               // do not share 
                                      NULL,                            // no security 
                                      OPEN_ALWAYS,                     // open or create 
                                      FILE_ATTRIBUTE_NORMAL,           // normal file 
                                      NULL);                           // no attr. template 
            
            if (m_hFile == INVALID_HANDLE_VALUE) 
            {
                //errno = GetLastError();
                TADD_WARNING("TMdbNtcFileLock::Lock failed, errno = %d \n",errno);
                return false;
            }
            
            if (bBlock)
            {
                //阻塞式加锁
                fileQ = LockFile(m_hFile, 0, 0, 0, 0); 
            }
            else
            {
                //加锁
                OVERLAPPED oOverlapped;//需要清零后作为最后一个参数传入，否则会崩溃
                memset(&oOverlapped, 0x00, sizeof(oOverlapped));
                fileQ = LockFileEx(m_hFile, LOCKFILE_FAIL_IMMEDIATELY, 0, 0, 0, &oOverlapped);         
            }

            if (!fileQ)
            {
                //errno = GetLastError();
                CloseHandle(m_hFile);
                TADD_WARNING("TMdbNtcFileLock::Lock failed, errno = %d \n",errno);
                return false;
            }
            
        #else
            int fileQ = -1;
            int ifd = -1;            
            
            if (bBlock)
            {
                ifd = F_LOCK;
            }
            else
            {
                ifd = F_TLOCK;
            }
            
            if (0 != access(m_sFilePath.c_str(), F_OK)) 
            {
                int iCreatQ = open(m_sFilePath.c_str(), O_CREAT, 0666);       
                if (0 > iCreatQ)        
                {
                    //errno = errno;
                    TADD_WARNING("open[%s] failed, errno = %d,%s \n", m_sFilePath.c_str(),errno, strerror(errno));
                    return false;        
                }
                
                close(iCreatQ);   
           } 
            
            m_hFile = open(m_sFilePath.c_str(), O_RDWR);
            
            if (m_hFile <= 0)	
            {
                //errno = errno;
                TADD_WARNING("open[%s] failed, errno = %d,%s \n", m_sFilePath.c_str(),errno, strerror(errno));
                return false;
            }
            
            //锁住文件
            fileQ = lockf(m_hFile, ifd, 0);
            
            if (fileQ < 0)	
            {
                //errno = errno;
                close(m_hFile);
                TADD_WARNING("lockf[%s] failed, errno = %d,%s \n", m_sFilePath.c_str(),errno, strerror(errno));
                return false;
            }
            
        #endif
            m_bLocked = true;
            return true;
        }

        bool TMdbNtcFileLock::Unlock()
        {
            if (!m_bLocked)
            {
                return false;
            }
            
        #ifdef OS_WINDOWS
            //解锁
            BOOL fileQ = UnlockFile(m_hFile, 0, 0, 0, 0); 
            
            CloseHandle(m_hFile);
            if (FALSE == fileQ)
            {
                //errno = GetLastError();
                TADD_WARNING("TMdbNtcFileLock::Unlock failed, errno = %d \n",  errno);
                return false;
            }
            
        #else
            int fileQ = -1;
            int ifd = F_ULOCK;

            //解锁文件
            fileQ = lockf(m_hFile, ifd, 0);
            
            close(m_hFile);
            
            if (fileQ < 0)	
            {
                //errno = errno;
                TADD_WARNING("TMdbNtcFileLock::Unlock failed, errno = %d \n",errno);
                return false;
            }
            
        #endif
            m_bLocked = false;
            return true;
        }
//}
