////////////////////////////////////////////////
// Name: mdbOraLog.cpp
// Author: Li.ShuGang
// Date: 2009/03/25
// Description: 刷新Oracle的日志类
////////////////////////////////////////////////
/*
* $History: mdbOraLog.cpp $
* 
* *****************  Version 1.0  ***************** 
*/
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <string>
#include <sstream>
#include <errno.h>
#include <sys/stat.h>
#include "Dbflush/mdbDbLog.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbOS.h"
#include "Helper/SqlParserStruct.h"
#include "Helper/mdbDateTime.h"

#ifdef  _WIN32
#include <time.h>
#include <windows.h>
#else
#include <unistd.h>
#endif  //_UNIX


#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <io.h>
#include <direct.h>
#endif

using namespace std;

//namespace QuickMDB{

    TMdbOraLog::TMdbOraLog(): m_pQueueCtrl(NULL)
    {
        //变量初始化
        memset(m_sDSN, 0, sizeof(m_sDSN));
        memset(m_sLogPath, 0, sizeof(m_sLogPath));

        m_iLogTime = 1;   //默认同步文件为1秒
        m_iLogSize = 128; //设定LOG文件最大的大小，单位为M，如果超出此大小，则备份文件
        m_iPID     = TMdbOS::GetPID();
        //m_pOraQueue= NULL;        
        m_fpInst   = NULL;

        m_iPosInst = 0;
        m_pLogSize = NULL;
        m_pShmDSN = NULL;
        m_pConfig = NULL;
        m_pDsn    = NULL;
        m_pszRecord = NULL;
        m_psMyFileTempInsert = NULL;
        for(int i=0; i<10; ++i)
        {
            m_fp[i]      = NULL;
            m_pszMyFileTemp[i] = NULL;
            m_iMyFilePos[i] = 0;
            m_iPosOra[i] = 0;
        }
        m_iOraRepCounts = 1;
        m_iMyFilePosInsert = 0;

    }


    TMdbOraLog::~TMdbOraLog()
    {        
        for(int i=0; i<10; ++i)
        {
            if(m_pszMyFileTemp[i] != NULL)
            {
                delete m_pszMyFileTemp[i];
                m_pszMyFileTemp[i] = NULL;    
            }
        }

        if(m_psMyFileTempInsert != NULL)
        {
            delete m_psMyFileTempInsert;
            m_psMyFileTempInsert = NULL;    
        }

        for(int i=0; i<m_iOraRepCounts; ++i)
        {
            SAFE_CLOSE(m_fp[i]);
        }
        SAFE_CLOSE(m_fpInst);
    }


    //设置DSN名称
    int TMdbOraLog::Init(const char* pszDSN, TMdbQueue & mdbQueueCtrl)
    {
        TADD_FUNC("TMdbOraLog::Init(%s) : Start.", pszDSN);
        int iRet  = 0;
        if(pszDSN == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"pszDSN == NULL.");
            return ERR_APP_INVALID_PARAM;    
        }
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        CHECK_OBJ(m_pConfig);

        //获取日志目录
        memset(m_sLogPath, 0, sizeof(m_sLogPath));
        SAFESTRCPY(m_sLogPath,sizeof(m_sLogPath),m_pConfig->GetDSN()->sLogDir);
        if(m_sLogPath[strlen(m_sLogPath)-1] != '/')
        {
            m_sLogPath[strlen(m_sLogPath)] = '/';   
        }
        TADD_NORMAL("Oracle-Log-Dir      = [%s].", m_sLogPath);
        m_iBufSize = m_pConfig->GetDSN()->iLogBuffSize*1024*1024;
        //获取日志的时间
        m_iLogTime = m_pConfig->GetDSN()->iLogTime;
        TADD_NORMAL("Oracle-Log-Time     = [%d]s.", m_iLogTime);   

        //开始连接到内存数据库上, 获取到FlushOracle的共享内存
        m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN);
        CHECK_OBJ(m_pShmDSN);
        m_pDsn = m_pShmDSN->GetInfo();
        CHECK_OBJ(m_pDsn);
        m_iOraRepCounts = m_pDsn->m_iOraRepCounts;
        if(m_iOraRepCounts<=0 || m_iOraRepCounts > 10)
        {
            m_iOraRepCounts = 1;    
        }

        //从共享内存中获取日志文件大小
        m_pLogSize = &(m_pShmDSN->GetSyncArea()->m_iFileSize);
        if(*m_pLogSize < 0)
        {
            m_pLogSize = &m_iLogSize;
        }
        TADD_NORMAL("Oracle-Log-FileSize = [%d]M.", *m_pLogSize); 
        //创建目录
        if(TMdbNtcDirOper::IsExist(m_sLogPath) == false)
        {
            if(TMdbNtcDirOper::MakeFullDir(m_sLogPath) == false)
            {
                TADD_ERROR(ERR_OS_CREATE_DIR," Can't create dir=[%s].",m_sLogPath);   
                return ERR_OS_CREATE_DIR; 
            }
        }
        //创建更新文件    
        for(int i=0; i<10; ++i)
        {
            m_fp[i] = NULL;
        }

        for(int i=0; i<m_iOraRepCounts; ++i)
        {
            sprintf(m_pszFileName[i], "%sOra%d", m_sLogPath, i);      
            m_fp[i] = fopen(m_pszFileName[i], "wb");
            setbuf(m_fp[i],NULL);
            memset(m_sOraOldTime[i], 0, MAX_TIME_LEN);
            TMdbDateTime::GetCurrentTimeStr(m_sOraOldTime[i]);
        }

        //创建插入/删除文件
        sprintf(m_pszFileNameInst, "%sInsert_Ora", m_sLogPath);      
        m_fpInst = fopen(m_pszFileNameInst, "wb");
        setbuf(m_fpInst,NULL);
        //m_pOraQueue = (TMdbMemQueue*)m_pShmDSN->GetSyncAreaShm(SA_ORACLE);
        //m_QueueCtrl.Init(m_pOraQueue, m_pDsn,true);
        CHECK_RET(m_tProcCtrl.Init(pszDSN),"m_tProcCtrl.Init(%s)",pszDSN);
        memset(m_sOldTime, 0, sizeof(m_sOldTime));
        TMdbDateTime::GetCurrentTimeStr(m_sOldTime);
        for(int i=0; i<m_iOraRepCounts; ++i)
        {
            if(m_pszMyFileTemp[i] == NULL)
            {
                m_pszMyFileTemp[i] = new(std::nothrow) char[MAX_VALUE_LEN];
                CHECK_OBJ(m_pszMyFileTemp[i]);
                memset(m_pszMyFileTemp[i],0,MAX_VALUE_LEN);
            }
        }

        if(m_psMyFileTempInsert == NULL)
        {
            m_psMyFileTempInsert = new(std::nothrow) char[MAX_VALUE_LEN];
            CHECK_OBJ(m_psMyFileTempInsert);
            memset(m_psMyFileTempInsert,0,MAX_VALUE_LEN);
        }
        /*
        if(m_pszRecord == NULL)
        {
        m_pszRecord = new char[MAX_VALUE_LEN];
        CHECK_OBJ(m_pszRecord);
        memset(m_pszRecord,0,MAX_VALUE_LEN);
        }
        */

        m_iCheckCounts = 0;
        m_pQueueCtrl = &mdbQueueCtrl;
        TADD_FUNC("(%s) : Finish.", pszDSN);
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  Log
    * 函数描述	: 负责缓冲区向物理数据库同步数据落地
    * 输入		:  bEmpty 缓冲区是否有数据
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		: 
    *******************************************************************************/
    int TMdbOraLog::Log(bool bEmpty /* = false */)
    {   
        TADD_FUNC("TMdbOraLog::Log() : Start.");
        int iRet = 0;
        //检查备份
        if(m_iCheckCounts >= 100)
        {
            //检查是否需要备份
            CheckBack();
            m_iCheckCounts = 0;
        }
        m_iCheckCounts++;

        if (bEmpty)//没有数据
        {
            return iRet;
        }
        m_pszRecord = m_pQueueCtrl->GetData();
        if(m_pszRecord[0] == 0)//没有数据
        {
            return iRet;
        }

        int iSQLType = m_pQueueCtrl->GetSqlType();
        int iLen = m_pQueueCtrl->GetRecordLen();
        if(iSQLType == TK_UPDATE)
        {         
            int iDealPos = GetPos(m_pszRecord, iLen);
            int iBuffPos = m_iMyFilePos[iDealPos];
            CheckAndWriteToFile(m_iMyFilePos[iDealPos],m_iPosOra[iDealPos],&m_pszMyFileTemp[iDealPos][iBuffPos],m_pszRecord,m_pszMyFileTemp[iDealPos],iLen,m_fp[iDealPos]);
        }
        else if(iSQLType == TK_INSERT || iSQLType == TK_DELETE)
        {
            TADD_DETAIL("TMdbOraLog::Log() : Insert-Msg=[%s].", m_pszRecord);
            CheckAndWriteToFile(m_iMyFilePosInsert,m_iPosInst,&m_psMyFileTempInsert[m_iMyFilePosInsert],m_pszRecord,m_psMyFileTempInsert,iLen,m_fpInst);
        }
        else
        {
            TADD_ERROR(ERROR_UNKNOWN,"Invalid DATA=[%s], m_iType=[%d]", m_pszRecord, iSQLType);
        }
        return iRet;
    }

    //检查是否需要备份
    void TMdbOraLog::CheckBack()
    {
        TADD_FUNC("TMdbOraLog::CheckBack() : Start.");

        WriteToFile(m_iMyFilePosInsert,m_psMyFileTempInsert,m_fpInst);
        CheckAndBackup(m_pszFileNameInst,m_iPosInst,m_fpInst,m_sOldTime);
        for(int i=0; i<m_iOraRepCounts; ++i)
        {
            WriteToFile(m_iMyFilePos[i],m_pszMyFileTemp[i],m_fp[i]);
            CheckAndBackup(m_pszFileName[i],m_iPosOra[i],m_fp[i],m_sOraOldTime[i]);
        }
        TADD_FUNC("TMdbOraLog::CheckBack() : Finish.");
    }
    void TMdbOraLog::CheckAndWriteToFile(int &iBuffPos,int &iFilePos,char* sdest,char*ssrc,char* sFirst,int iLen,FILE *fp)
    {
        if(iBuffPos + iLen < MAX_VALUE_LEN)
        {
            strncpy(sdest, ssrc, iLen);	
            iBuffPos+=iLen;
        }
        else
        {
            if(fwrite(sFirst, iBuffPos, 1, fp) == 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"fwrite() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));	
            }

            strncpy(sFirst, ssrc, iLen);
            iBuffPos = iLen;
        }
        iFilePos += iLen;
        return;
    }

    void TMdbOraLog::WriteToFile(int& iFileBuffPos,char* sFileBuff,FILE *fp)
    {
        if(iFileBuffPos <=0)
        {
            return;
        }

        if(fwrite(sFileBuff, iFileBuffPos, 1, fp) == 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"fwrite() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));	
        }
        iFileBuffPos = 0;
        return;
    }


    int TMdbOraLog::GetPos(const char* pszMsg, int iLen)
    {
        m_curLcr.Clear();
        m_tParser.Analyse(pszMsg, m_curLcr);

        if(m_curLcr.m_vWColms.size() > 0)
        {
            return TMdbNtcStrFunc::StrToHash(m_curLcr.m_vWColms[0].m_sColmValue.c_str()) % m_iOraRepCounts;
        }

        return 0;
        /*
        //性能优化
        char sTemp[64] = { 0 };
        for(int i=0; i<iLen; ++i)
        {
        if(pszMsg[i] == ',' &&  pszMsg[i+1] == '@' && pszMsg[i+2] == '|')
        {
        for(int n=i+6; n<iLen; ++n)
        {
        if(pszMsg[n] == ',' || (pszMsg[n] == '^' && pszMsg[n+1] == '^'))
        {
        break;    
        }

        if(n-i-6 >= 64)
        break;
        sTemp[n-i-6] = pszMsg[n];                  
        }

        return TMdbNtcStrFunc::StrToHash(sTemp) % m_iOraRepCounts;
        }   
        }
        return 0;
        */
    }


    long TMdbOraLog::GetFileSize(char * sFullPathFileName)
    {
        struct stat sbuf;
        if (stat(sFullPathFileName, &sbuf) == 0)
            return sbuf.st_size;
        else 
            return -1;
    }

    bool TMdbOraLog::CheckAndBackup(char* sFileName,int &iFilePos,FILE *&fp,char* sOldTime)
    {
        try
        {
            if(GetFileSize(sFileName) == 0)
            {
                return true;
            }    

            if(iFilePos >= *m_pLogSize * 1024 * 1024 || TMdbDateTime::GetDiffSeconds(m_pDsn->sCurTime, sOldTime) >= m_iLogTime || TMdbDateTime::GetDiffSeconds(m_pDsn->sCurTime, sOldTime) < 0)
            {
                SAFESTRCPY(sOldTime,MAX_TIME_LEN,m_pDsn->sCurTime);
                char sFileNameOld[MAX_PATH_NAME_LEN];
                memset(sFileNameOld, 0, MAX_PATH_NAME_LEN);
                sprintf(sFileNameOld,"%s.%s", sFileName, sOldTime);
                TADD_DETAIL("TMdbOraLog::CheckAndBackup() : sFileNameOld=%s.", sFileNameOld); 
                TMdbDateTime::MSleep(20);            
                if(fclose(fp)!=0)
                {
                    TADD_ERROR(ERROR_UNKNOWN," CheckAndBackup() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));	
                }

                TMdbNtcFileOper::Rename(sFileName, sFileNameOld);
                iFilePos = 0;
                fp = fopen (sFileName,"wb");
                if(fp == NULL)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Open file [%s] failed.", m_pszFileNameInst);
                    return false;
                }
                setbuf(fp,NULL);
            }        
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"TMdbOraLog::CheckAndBackupInsert() :  failed.");
            return false;
        }
        return true;
    }

    TMdbRedoLog::TMdbRedoLog(): m_pQueueCtrl(NULL)
    {
        memset(m_sDSN, 0, sizeof(m_sDSN));
        memset(m_sLogPath, 0, sizeof(m_sLogPath));

        m_iLogTime = 1;   //默认同步文件为1秒
        m_iLogSize = 128; //设定LOG文件最大的大小，单位为M，如果超出此大小，则备份文件
        m_iPID     = TMdbOS::GetPID();
        m_pLogSize = NULL;
        m_pShmDSN = NULL;
        m_pConfig = NULL;
        m_pDsn    = NULL;
        m_pszRecord = NULL;
        m_fp      = NULL;
        m_pszMyFileTemp = NULL;
        m_iMyFilePos = 0;
        m_iPos = 0;
        m_iLsn = 0;
    }

    TMdbRedoLog::~TMdbRedoLog()
    {

    }

    int TMdbRedoLog::Init(const char* pszDSN, TMdbQueue & mdbQueueCtrl)
    {
        TADD_FUNC("TMdbOraLog::Init(%s) : Start.", pszDSN);
        int iRet  = 0;
        CHECK_OBJ(pszDSN);
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        CHECK_OBJ(m_pConfig);

        //获取日志目录
        memset(m_sLogPath, 0, sizeof(m_sLogPath));
        SAFESTRCPY(m_sLogPath,sizeof(m_sLogPath),m_pConfig->GetDSN()->sRedoDir);
        if(m_sLogPath[strlen(m_sLogPath)-1] != '/')
        {
            m_sLogPath[strlen(m_sLogPath)] = '/';   
        }
        m_iBufSize = m_pConfig->GetDSN()->iRedoBuffSize*1024*1024;
        //获取日志的时间
        m_iLogTime = m_pConfig->GetDSN()->iLogTime;

        //开始连接到内存数据库上, 获取到FlushOracle的共享内存
        m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN);
        CHECK_OBJ(m_pShmDSN);
        m_pDsn = m_pShmDSN->GetInfo();
        CHECK_OBJ(m_pDsn);
        //从共享内存中获取日志文件大小
        m_pLogSize = &(m_pShmDSN->GetSyncArea()->m_iFileSize);
        if(*m_pLogSize < 0)
        {
            m_pLogSize = &m_iLogSize;
        }
        //创建目录
        if(TMdbNtcDirOper::IsExist(m_sLogPath) == false)
        {
            if(TMdbNtcDirOper::MakeFullDir(m_sLogPath) == false)
            {
                TADD_ERROR(ERR_OS_CREATE_DIR," Can't create dir=[%s].",m_sLogPath);   
                return ERR_OS_CREATE_DIR; 
            }
        }
        //创建更新文件    
        m_fp = NULL;
        sprintf(m_pszFileName, "%sRedo", m_sLogPath);
        m_fp = fopen(m_pszFileName, "wb");
        setbuf(m_fp,NULL);
        memset(m_sOldTime, 0, sizeof(m_sOldTime));
        TMdbDateTime::GetCurrentTimeStr(m_sOldTime);
        CHECK_RET(m_tProcCtrl.Init(pszDSN),"m_tProcCtrl.Init(%s)",pszDSN);
        if(m_pszMyFileTemp == NULL)
        {
            m_pszMyFileTemp = new(std::nothrow) char[MAX_VALUE_LEN];
            CHECK_OBJ(m_pszMyFileTemp);
            memset(m_pszMyFileTemp,0,MAX_VALUE_LEN);
        }
        m_iCheckCounts = 0;
        m_pQueueCtrl = &mdbQueueCtrl;
        TADD_FUNC("(%s) : Finish.", pszDSN);
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  Log
    * 函数描述	: 负责缓冲区需要回滚的数据落地
    * 输入		:  bEmpty 缓冲区是否有数据
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		: 
    *******************************************************************************/
    int TMdbRedoLog::Log(bool bEmpty /* = false */)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //检查是否需要备份
        if(m_iCheckCounts >= 100)
        {
            CheckBack();
            m_iCheckCounts = 0;
        }
        m_iCheckCounts++;

        if (bEmpty)//没有数据
        {
            return iRet;
        }

        m_pszRecord = m_pQueueCtrl->GetData();
        if(m_pszRecord[0] == 0)//没有数据
        {
            return iRet;
        }
        long long iLsn = GetLsn();
        m_iLsn = m_iLsn < iLsn?iLsn:m_iLsn;
        int iLen = m_pQueueCtrl->GetRecordLen();
        TADD_DETAIL("Redo: [%s]", m_pszRecord);
        CheckAndWriteToFile(m_iMyFilePos,m_iPos,&m_pszMyFileTemp[m_iMyFilePos],m_pszRecord,m_pszMyFileTemp,iLen,m_fp);
        return iRet;
    }

    void TMdbRedoLog::CheckBack()
    {
        TADD_FUNC(" Start.");
        WriteToFile(m_iMyFilePos,m_pszMyFileTemp,m_fp);
        CheckAndBackup(m_pszFileName,m_iPos,m_fp,m_sOldTime);
        TADD_FUNC("Finish.");
    }
    void TMdbRedoLog::CheckAndWriteToFile(int &iBuffPos,int &iFilePos,char* sdest,char*ssrc,char* sFirst,int iLen,FILE *fp)
    {
        if(iBuffPos + iLen < MAX_VALUE_LEN)
        {
            strncpy(sdest, ssrc, iLen);	
            iBuffPos+=iLen;
        }
        else
        {
            if(fwrite(sFirst, iBuffPos, 1, fp) == 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"fwrite() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));	
            }

            strncpy(sFirst, ssrc, iLen);
            iBuffPos = iLen;
        }
        iFilePos += iLen;
        return;
    }

    void TMdbRedoLog::WriteToFile(int& iFileBuffPos,char* sFileBuff,FILE *fp)
    {
        if(iFileBuffPos <=0)
        {
            return;
        }

        if(fwrite(sFileBuff, iFileBuffPos, 1, fp) == 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"fwrite() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));	
        }
        iFileBuffPos = 0;
        return;
    }
    long TMdbRedoLog::GetFileSize(char * sFullPathFileName)
    {
        struct stat sbuf;
        if (stat(sFullPathFileName, &sbuf) == 0)
            return sbuf.st_size;
        else 
            return -1;
    }

    bool TMdbRedoLog::CheckAndBackup(char* sFileName,int &iFilePos,FILE *&fp,char* sOldTime)
    {
        try
        {
            if(GetFileSize(sFileName) == 0)
            {
                return true;
            }    
            if(iFilePos >= *m_pLogSize * 1024 * 1024 || TMdbDateTime::GetDiffSeconds(m_pDsn->sCurTime, sOldTime) >= m_iLogTime || TMdbDateTime::GetDiffSeconds(m_pDsn->sCurTime, sOldTime) < 0)
            {
                SAFESTRCPY(sOldTime,MAX_TIME_LEN,m_pDsn->sCurTime);
                char sFileNameOld[MAX_PATH_NAME_LEN];
                memset(sFileNameOld, 0, MAX_PATH_NAME_LEN);
                sprintf(sFileNameOld,"%s.%s.%lld.OK", sFileName, sOldTime,m_iLsn);
                TADD_DETAIL("TMdbOraLog::CheckAndBackup() : sFileNameOld=%s.", sFileNameOld); 
                TMdbDateTime::MSleep(20);            
                if(fclose(fp)!=0)
                {
                    TADD_ERROR(ERROR_UNKNOWN," CheckAndBackup() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));	
                }

                TMdbNtcFileOper::Rename(sFileName, sFileNameOld);
                iFilePos = 0;
                m_iLsn = 0;
                fp = fopen (sFileName,"wb");
                if(fp == NULL)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Open file [%s] failed.", sFileName);
                    return false;
                }
                setbuf(fp,NULL);
            }        
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"TMdbOraLog::CheckAndBackupInsert() :  failed.");
            return false;
        }
        return true;
    }

    long long TMdbRedoLog::GetLsn()
    {
        char sTemp[32] = {0};
        memcpy(sTemp,m_pszRecord+11,20);
        return TMdbNtcStrFunc::StrToInt(sTemp);
    }

//}

