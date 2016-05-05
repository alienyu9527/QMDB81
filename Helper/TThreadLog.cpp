////////////////////////////////////////////////
// Name: TThreadLog.cpp
// Author: Li.ShuGang
// Date: 2008/11/12
// Description: 线程安全的日志类
////////////////////////////////////////////////
/*
* $History: TThreadLog.hcpp $
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
#include "Helper/TThreadLog.h"
#include "Helper/TBaseException.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbStruct.h"
#include "Control/mdbProcCtrl.h"
#include "Helper/mdbFileList.h"
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
//#include "BillingSDK.h"
using namespace std;
//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{
    
    TMutex TThreadLog::m_tMutex(true);
    TThreadLog* gpLogInstance = new(std::nothrow) TThreadLog();

    //新增一个变量LOGID
    TTLInfo::TTLInfo()
    {
        iThreadID = -1;
        m_fp      = NULL;
        m_iNowLogLevel = 0;
        m_iInitLogLevel = 0;
        memset(sInfo, 0, sizeof(sInfo));
        memset(sPath, 0, sizeof(sPath));
        memset(m_pszFileName, 0, sizeof(m_pszFileName));
        memset(m_pszLogTemp, 0, sizeof(m_pszLogTemp));
        bTraceFlag = false;

    }


    TTLInfo::~TTLInfo()
    {
        Clear();
    }

    /******************************************************************************
    * 函数名称	:  Clear()
    * 函数描述	:  初始化
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TTLInfo::Clear()
    {
        iThreadID = -1;
        m_fp      = NULL;
        m_iNowLogLevel = 0;
        memset(sInfo, 0, sizeof(sInfo));
        memset(m_pszFileName, 0, sizeof(m_pszFileName));
    }


    TThreadLog::TThreadLog()
    {
        //变量初始化
        m_bShowConsole = false;
        m_iLogLevel = 0;  //当前日志的级别
        m_iLogSize  = 128; //设定LOG文件最大的大小，单位为M，如果超出此大小，则备份文件
        m_iLogCount = 1;
        m_iNowLogLevel = 0;
        m_pShmDSN = NULL;
        m_iPid = 0;
        m_bMemExit = false;
        m_logFlag = false;
        m_pCurLogLevel = &m_iLogLevel;
        //获取日志目录
        memset(m_sPath, 0, sizeof(m_sPath));
        memset(m_pszFileName, 0, sizeof(m_pszFileName));
    	memset(m_NoPathFileName,0,sizeof(m_NoPathFileName));
        memset(m_pszErrorFileName, 0, sizeof(m_pszErrorFileName));
    	memset(m_NoPathErrorFileName,0,sizeof(m_NoPathErrorFileName));
        memset(m_sAppName,0,sizeof(m_sAppName));
        m_sRegProcName[0] = '\0';
    }


    TThreadLog::~TThreadLog()
    {
    }

    /******************************************************************************
    * 函数名称	:  IsExist()
    * 函数描述	:  判断日志文件是否存在
    * 输入		:  sFileName 文件名
    * 输出		:  无
    * 返回值	:  true 存在，false 不存在
    * 作者		:  li.shugang
    *******************************************************************************/
    bool TThreadLog::IsExist(const char *sFileName)
    {
        int iRetCode =0;
#ifndef _WIN32
        iRetCode = access(sFileName, F_OK);
#else
        iRetCode = access(sFileName, 0x00);
#endif
        return iRetCode == 0;
    }

    /******************************************************************************
    * 函数名称	:  GetID()
    * 函数描述	:  获取当前进程ID
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  进程ID
    * 作者		:  li.shugang
    *******************************************************************************/
    int TThreadLog::GetCurPID()
    {
        if(m_iPid <= 0){m_iPid = TMdbOS::GetPID();}
        return m_iPid;
    }

    /******************************************************************************
    * 函数名称	:  Start()
    * 函数描述	:  获取当前进程ID
    * 输入		:  sDsn DSN名称，sPathName 路径，sAppName 进程名，iLevel
    *           :  日志级别，bFlag 是否在屏幕打印
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    int TThreadLog::Start(const char* sDsn,const char *sAppName, int iLevel,bool bFlag,bool bIsMonitor)
    {
        if(m_sRegProcName[0] != '\0') return 0;
        m_tMutex.Lock(true);
        int iRet = 0;
        char sPocName[128] = {0};
        TMdbProcCtrl tProcCtrl;
        TMdbNtcSplit mdbSplit;
        mdbSplit.SplitString(sAppName, " ");
        SAFESTRCPY(sPocName,sizeof(sPocName),mdbSplit[0]);
        do
        {
            if(iLevel == -1)
            {
                m_logFlag = false;
                break;
            }
            if (NULL == sDsn)
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM,"Dsn is NULL.");
                iRet = ERR_APP_INVALID_PARAM;
                break;
            }
            
            if(NULL != m_pShmDSN)
            {
                CHECK_RET_BREAK(tProcCtrl.Init(sDsn),"tProcCtrl.Init(%s) failed.",sDsn);
                tProcCtrl.RegProc(sAppName,bIsMonitor);
                break;
            }
            //读取错误码配置
            m_tErrHelper.InitErrorDescription();//无论错误与否都不中断
            //获取配置LogLevel
            memset(m_sAppName,'\0',sizeof(m_sAppName));
            SAFESTRCPY(m_sAppName,sizeof(m_sAppName), sPocName);
            m_sAppName[strlen(m_sAppName)] = '\0';
            
            CHECK_RET_BREAK(InitLogPath(sDsn),"Failed to initialize the log directory.");
#ifndef _WIN32
            if (strlen(m_pszFileName) == 0)
            {
                sprintf(m_pszFileName,"%s%s.log", m_sPath, sPocName);
            	snprintf(m_NoPathFileName,sizeof(m_NoPathFileName),"%s.log", sPocName);
                sprintf(m_pszErrorFileName,"%s%s.log_ERROR", m_sPath, sPocName);
            	snprintf(m_NoPathErrorFileName,sizeof(m_NoPathErrorFileName),"%s.log_ERROR",sPocName);
            }
#else
            sprintf(m_pszFileName,"%s%s.log", m_sPath, sPocName);
        	snprintf(m_NoPathFileName,sizeof(m_NoPathFileName),"%s.log", sPocName);
            sprintf(m_pszErrorFileName,"%s%s.log_ERROR", m_sPath, sPocName);
        	snprintf(m_NoPathErrorFileName,sizeof(m_NoPathErrorFileName),"%s.log_ERROR", sPocName);
#endif
            m_logFlag = true;
            //其他配置
            m_iLogLevel    = iLevel;
            m_bShowConsole = bFlag;
            m_iPid = TMdbOS::GetPID();
            if(TryAttachShm(sDsn))
            {
                TMdbConfig *pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
                if (NULL == pConfig)
                {
                    CHECK_RET_BREAK(ERR_APP_INVALID_PARAM, "Failed to Load MDB[%s] configuration files.", sDsn);
                }
                m_pDsn = pConfig->GetDSN();
	            CHECK_OBJ(m_pDsn);
	            //m_iLogSize = m_pDsn->iLogFileSize;
	            m_iLogCount = m_pDsn->iLogCount;
                CHECK_RET_BREAK(tProcCtrl.Init(sDsn),"Failed to initialize the m_pProcCtrl.");
                m_pProc = tProcCtrl.RegProc(sAppName,bIsMonitor);
                if(NULL == m_pProc)
                {
                    CHECK_RET_BREAK(ERR_APP_REGISTER_FAILED,"Failed to Register the process [%s].",sAppName);
                    break;
                }
                GetProcLogLevel();
                m_bMemExit = true;
            }
        }
        while(0);
        SAFESTRCPY(m_sRegProcName,sizeof(m_sRegProcName), sAppName);
        m_tMutex.UnLock(true);
        return iRet;
    }

    void TThreadLog::End()
    {
    }

    int TThreadLog::OffLineStart(const char* sDsn,const char *sAppName, int iLevel,bool bFlag)
    {
        if(m_tMutex.Lock(true) != 0)
        {
            return -1;
        }
        int iRet = 0;
        char sPocName[128] = {0};
        TMdbNtcSplit mdbSplit;
        mdbSplit.SplitString(sAppName, " ");
        SAFESTRCPY(sPocName,sizeof(sPocName),mdbSplit[0]);
        do
        {
            if(iLevel == -1)
            {
                m_logFlag = false;
                break;
            }
            if (NULL == sDsn)
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM,"Dsn is NULL.");
                iRet = ERR_APP_INVALID_PARAM;
                break;
            }
            //读取错误码配置
            m_tErrHelper.InitErrorDescription();
            //获取配置LogLevel
            memset(m_sAppName,'\0',sizeof(m_sAppName));
            SAFESTRCPY(m_sAppName,sizeof(m_sAppName), sPocName);
            m_sAppName[strlen(m_sAppName)] = '\0'; 
            CHECK_RET_BREAK(InitLogPath(sDsn),"Failed to initialize the log directory.");
            if (strlen(m_pszFileName) == 0)
            {
                sprintf(m_pszFileName,"%s%s.log", m_sPath, sPocName);
                sprintf(m_pszErrorFileName,"%s%s.log_ERROR", m_sPath,sPocName);
	            snprintf(m_NoPathFileName,sizeof(m_NoPathFileName),"%s.log", sPocName);
	            snprintf(m_NoPathErrorFileName,sizeof(m_NoPathErrorFileName),"%s.log_ERROR",sPocName);
            }
            
            m_logFlag = true;
            m_iLogLevel = iLevel;
            m_bShowConsole = bFlag;
        }
        while(0);
        if( m_tMutex.UnLock(true) != 0)
        {
            return -1;
        }
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  AttachShm()
    * 函数描述	:  连接共享内存
    * 输入		:  sDsn DSN名
    * 输出		:  无
    * 返回值	:  true 连接成功；false 连接失败
    * 作者		:  cao.peng
    *******************************************************************************/
    bool TThreadLog::TryAttachShm(const char* sDsn)
    {
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn,true);
        if(NULL == m_pShmDSN)
        {
            return false;
        }
        else
        {
            return true;
        }
        
        
    }

    /******************************************************************************
    * 函数名称	:  SetLogLevel()
    * 函数描述	:  设置日志级别
    * 输入		:  iLevel 日志级别
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TThreadLog::SetLogLevel(int iLevel)
    {
        //设置进程日志级别
        //这里还是设置传入的日志界别
        if(m_logFlag == false)
        {
            return;
        }
        m_iLogLevel = iLevel;
    }

    /******************************************************************************
    * 函数名称	:  GetNowLogLevel()
    * 函数描述	:  获取当前日志级别
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  日志级别
    * 作者		:  li.shugang
    *******************************************************************************/
    int TThreadLog::GetNowLogLevel()
    {
        if(m_logFlag == false)
        {
            return 0;
        }
        return m_iNowLogLevel;
    }

    /******************************************************************************
    * 函数名称	:  SetLogSize()
    * 函数描述	:  设置日志文件大小
    * 输入		:  iLogSize 日志大小(MB为单位)
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TThreadLog::SetLogSize(int iLogSize)
    {
        if(m_logFlag == false)
        {
            return;
        }
        m_iLogSize = iLogSize ;
    }

    /******************************************************************************
    * 函数名称	:  GetLogSize()
    * 函数描述	:  获取日志文件大小
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  日志文件大小(MB)
    * 作者		:  li.shugang
    *******************************************************************************/
    int TThreadLog::GetLogSize()
    {
        if(m_logFlag == false)
        {
            return 0;
        }
        return m_iLogSize ;
    }

    /******************************************************************************
    * 函数名称	:  SetShowConsole()
    * 函数描述	:  设置是否在屏幕打印日志
    * 输入		:  bFlag 是否在屏幕打印
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TThreadLog::SetShowConsole(bool bFlag)
    {
        if(m_logFlag == false)
        {
            return;
        }
        m_bShowConsole = bFlag;
    }

    /******************************************************************************
    * 函数名称	:  GetLevelName()
    * 函数描述	:  获取日志级别名称
    * 输入		:  iLogLevel 日志级别
    * 输出		:  sInfo 日志级别对应的名称
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TThreadLog::GetLevelName(int iLogLevel, char sInfo[])
    {
        if(m_logFlag == false)
        {
            return;
        }
        switch(iLogLevel)
        {
        case TLOG_FLOW       :
            strcpy(sInfo,"[ FLOW ]");
            break;
        case TLOG_DETAIL     :
            strcpy(sInfo,"[DETAIL]");
            break;
        case TLOG_FUNC       :
            strcpy(sInfo,"[ FUNC ]");
            break;
        case TLOG_WARNING    :
            strcpy(sInfo,"[ WARN ]");
            break;
        case TLOG_FATAL      :
            strcpy(sInfo,"[ ERROR]");
            break;
        case TLOG_NORMAL     :
            strcpy(sInfo,"[NORMAL]");
            break;
        default              :
            strcpy(sInfo,"[ (N/A)]");
            break;
        }
    }

    /******************************************************************************
    * 函数名称	:  GetProcLogLevel()
    * 函数描述	:  获取当前进程日志级别
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  日志级别
    * 作者		:  li.shugang
    *******************************************************************************/
    int TThreadLog::GetProcLogLevel()
    {
        if(!m_logFlag || NULL == m_pProc)
        {
            return 0;
        }

        if(m_pDsn->iLogLevel != m_pProc->iLogLevel)
        {
            m_pProc->iLogLevel = m_pDsn->iLogLevel;
        }
        
        m_iLogLevel = m_pProc->iLogLevel;
        m_pCurLogLevel = &(m_pProc->iLogLevel);
        return m_iLogLevel;
    }

    /******************************************************************************
    * 函数名称	:  LogEx()
    * 函数描述	:  记录额外信息的log
    * 输入		:  sFileName 日志的文件名，iFileLine 调用log的位置，sFuncName 函数名
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    #ifdef OS_SUN
    void TThreadLog::LogEx(const char * sFileName,int  iFileLine,const char * fmt, ...)
    #else
    void TThreadLog::LogEx(const char * sFileName,int  iFileLine,const char * sFuncName,const char * fmt, ...)
    #endif
    {
        char sLogTemp[10240];
        va_list ap;
        va_start(ap,fmt);
        memset(sLogTemp,0,sizeof(sLogTemp));
        vsnprintf(sLogTemp, sizeof(sLogTemp), fmt, ap);
        va_end (ap);
        #ifdef OS_SUN
        Log("[%s:%d]: %s",sFileName,iFileLine,sLogTemp);//记录
        #else
        Log("[%s:%d][%s]: %s",sFileName,iFileLine,sFuncName,sLogTemp);//记录
        #endif
    }

    /******************************************************************************
    * 函数名称	:  PureLog
    * 函数描述	:  只输出信息不加，文件名，行号之类
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  jin.shaohua
    *******************************************************************************/
    void TThreadLog::PureLog(const char * fmt, ...)
    {
        //需要输出到屏幕
        if(m_iNowLogLevel < 0 || (true == m_bShowConsole && m_iNowLogLevel <= 0))
        {
            char sLogTemp[10240];
            va_list ap;
            va_start(ap,fmt);
            memset(sLogTemp,0,sizeof(sLogTemp));
            vsnprintf(sLogTemp, sizeof(sLogTemp), fmt, ap);
            va_end (ap);
            printf("%s\n",sLogTemp);
        }
    }

    /******************************************************************************
    * 函数名称	:  Log()
    * 函数描述	:  写日志
    * 输入		:  fmt 参数列表
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TThreadLog::Log(const char * fmt, ...)
    {
        if(m_logFlag == false)
        {
            return;
        }
        char sDate[30];
        char sInfo[15];
        FILE * fp = NULL;
        memset(sDate,0,sizeof(sDate));
        memset(sInfo,0,sizeof(sInfo));
        GetCurrentTimeStr(sDate, sizeof(sDate), true);
        GetLevelName(m_iNowLogLevel, sInfo);
        char sLogTemp[1024*10];
        va_list ap;
        va_start(ap,fmt);
        memset(sLogTemp,0,sizeof(sLogTemp));
        vsnprintf(sLogTemp, sizeof(sLogTemp), fmt, ap);
        va_end (ap);
#ifdef WIN32
        SAFE_CLOSE(fp);
#endif
        fp = fopen(m_pszFileName, "a+");
        if(fp == NULL)
        {
            printf("(%s) : Open file [%s] failed. \n", m_sAppName,m_pszFileName);
            return ;
        }
        else
        {
            CheckAndBackup(m_pszFileName, m_NoPathFileName, fp);
            fprintf(fp, "%s %s [%d-%lu]|%s\n", sDate, sInfo, TMdbOS::GetPID(),TMdbOS::GetTID(), sLogTemp);
            fflush(fp);
        }
        SAFE_CLOSE(fp);
        
        if(TLOG_FATAL == m_iNowLogLevel)
        {
            //对于错误日志的专门写到一个文件中去
            FILE * fpErr = NULL;
#ifdef WIN32
            SAFE_CLOSE(fpErr);
#endif
            fpErr = fopen(m_pszErrorFileName, "a+");
            if(fpErr == NULL)
            {
                printf("(%s) : Open file [%s] failed. \n", m_sAppName,m_pszErrorFileName);
                return;
            }
            else
            {
                CheckAndBackup(m_pszErrorFileName, m_NoPathErrorFileName, fpErr);               
                fprintf(fpErr, "%s %s [%d-%lu]|%s\n", sDate, sInfo, TMdbOS::GetPID(),TMdbOS::GetTID(), sLogTemp);
                fflush(fpErr);
            }
            SAFE_CLOSE(fpErr);
        }

    }

    /******************************************************************************
    * 函数名称	:  GetFileSize()
    * 函数描述	:  获取日志文件大小
    * 输入		:  sFullPathFileName 日志文件名
    * 输出		:  无
    * 返回值	:  日志文件大小
    * 作者		:  li.shugang
    *******************************************************************************/
    long TThreadLog::GetFileSize(char * sFullPathFileName)
    {
        if(m_logFlag == false)
        {
            return 0;
        }
        if(NULL == sFullPathFileName)
        {
            return -1;
        }
        struct stat sbuf;
        if (stat(sFullPathFileName, &sbuf) == 0)
            return sbuf.st_size;
        else
            return -1;
    }

    /******************************************************************************
    * 函数名称	:  CheckAndBackup()
    * 函数描述	:  检查文件是否超出限制，若是，则将其备份为.old文件，并创建一个新的文件
    * 输入		:  sFileName 日志文件名，fp 日志文件对应的句柄
    * 输出		:  无
    * 返回值	:  true 成功，false 失败
    * 作者		:  li.shugang
    *******************************************************************************/
    bool TThreadLog::CheckAndBackup(const char * sFileName, const char* sNoPathFileName, FILE*& fp)
    {
        if(m_logFlag == false)
        {
            return true;
        }
        if(NULL == sFileName)
        {
            return false;
        }
        char     sFileNameOld[255];
    	char     sNoPathFileNameOld[255];
        long     filesize = -1 ;
        //int      filehandle = -1 ;
        struct stat buf;
        memset(sFileNameOld,0, 255);
    	memset(sNoPathFileNameOld,0, 255);
        //判断文件是否存在
        if(stat(sFileName,&buf)<0)
        {
            if(errno == ENOENT)
            {
                SAFE_CLOSE(fp);
                fp = fopen(sFileName,"a+");
            }
        }
        filesize = GetFileSize((char*)sFileName);

        if (filesize == -1)
        {
            return false;
        }

        if (filesize >= m_iLogSize*1024*1024)
        {
            //超过最长长度,以当前时间YYYYMMDDHHM24SS表示
            SAFE_CLOSE(fp);
            char sCurTime[MAX_TIME_LEN];
            TMdbDateTime::GetCurrentTimeStr(sCurTime);
            sprintf(sFileNameOld,"%s.%s.%s", sFileName, "old", sCurTime);
        	snprintf(sNoPathFileNameOld,sizeof(sNoPathFileNameOld),"%s.%s", sNoPathFileName, "old");
            TMdbNtcFileOper::Remove(sFileNameOld);
            
            //将日志文件重命名
            TMdbNtcFileOper::Rename(sFileName,sFileNameOld);

            TMdbNtcFileOper::Remove(sFileName);
            fp = fopen (sFileName,"a+");
            if(fp == NULL)
            {
                printf("Check And Backup Log File: Open file [%s] fail.\n",sFileName);
                return false;
            }
			
			TMdbFileList m_tFileList; //查找以存在文件
	        m_tFileList.Init(m_sPath);
	        m_tFileList.GetFileList(1, 0, sNoPathFileNameOld,"");
	        int iCount = m_tFileList.GetFileCounts();
	        if(iCount > m_iLogCount)
	        {
	            char sFileNewName[255];
	            memset(sFileNewName,0,255);
	            while(m_tFileList.Next(&sFileNewName[0]) == 0)
	            {         
	            	TMdbNtcFileOper::Remove(sFileNewName);
	               break;
	            }
	        }
        }
        return true ;
    }

    /******************************************************************************
    * 函数名称	:  GetCurrentTimeStr()
    * 函数描述	:  获取当前时间,字符串表示
    * 输入		:  iLen 时间字符串长度，bLongFlag 是否长格式
    * 输出		:  sTime 当前时间字符串
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TThreadLog::GetCurrentTimeStr(char sTime[], size_t iLen, const bool bLongFlag)
    {
        time_t tCurrent;
        struct tm *tm_Cur;
        char sCurtime[40];
        sCurtime[39]=0;
        time(&tCurrent); //取得当前时间的time_t值
        tm_Cur = localtime(&tCurrent); //取得当前时间的tm值

        if(bLongFlag)
        {
            sprintf(sCurtime,"%04d-%02d-%02d %02d:%02d:%02d",tm_Cur->tm_year+1900,tm_Cur->tm_mon+1,tm_Cur->tm_mday,tm_Cur->tm_hour,tm_Cur->tm_min,tm_Cur->tm_sec);
        }
        else
        {
            sprintf(sCurtime,"%04d%02d%02d%02d%02d%02d",tm_Cur->tm_year+1900,tm_Cur->tm_mon+1,tm_Cur->tm_mday,tm_Cur->tm_hour,tm_Cur->tm_min,tm_Cur->tm_sec);
        }

        if(sTime!=NULL)
        {
            if(iLen>0)
            {
                strncpy(sTime,sCurtime,iLen-1);
                sTime[iLen-1]='\0';
            }
            else
                strcpy(sTime,sCurtime);
        }

        return;
    }



    /******************************************************************************
    * 函数名称	:  ResetLogLevelAfterDetach
    * 函数描述	:  在detach之后重新设置日志级别
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TThreadLog::ResetLogLevelAfterDetach()
    {
        if(NULL != m_pCurLogLevel)
        {
            m_iLogLevel = *m_pCurLogLevel;
            m_pCurLogLevel = &m_iLogLevel;
        }
        return 0;
    }

    /******************************************************************************
    * 函数名称	:  GetDetailErrorDesc
    * 函数描述	: 获取详细错误信息
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    std::string TThreadLog::GetDetailErrorDesc(int iErrCode)
    {
        m_tErrHelper.InitErrorDescription();
        iErrCode = iErrCode<0?0-iErrCode:iErrCode;//若小于0取反
        char sTemp[1024] = {0};
        sprintf(sTemp,"Error=[%d].Detail:",iErrCode);
        std::string sRet = sTemp;
        return sRet;
    }

    int TThreadLog::InitLogPath(const char* sDsn)
    {
        int iRet = 0;
        char sDsnName[MAX_NAME_LEN] = {0};
        CHECK_OBJ(sDsn);
        SAFESTRCPY(sDsnName, sizeof(sDsnName), sDsn);
        TMdbNtcStrFunc::ToLower(sDsnName);
#ifdef WIN32
        sprintf(m_sPath, "%s\\log\\%s\\", getenv("QuickMDB_HOME"),sDsnName);
        //判断目录是否存在.
#else
        sprintf(m_sPath, "%s/log/%s/", getenv("QuickMDB_HOME"),sDsnName);
#endif
        //创建日志目录
        if(!TMdbNtcDirOper::IsExist(m_sPath))
        {
            if(!TMdbNtcDirOper::MakeFullDir(m_sPath))
            {
                CHECK_RET(ERR_OS_CREATE_DIR,"Mkdir(%s) failed.",m_sPath);
            }
        }
        return iRet;
    }

    string  TThreadLog::FormatToCLI(int iFmtCliType,const char * fmt,...)
    {
        char sTemp[1024]= {0};
        va_list ap;
        va_start(ap,fmt);
        memset(sTemp,0,sizeof(sTemp));
        vsnprintf(sTemp, sizeof(sTemp), fmt, ap);
        va_end (ap);
        char sSuffix[8] = {0};
        switch(iFmtCliType)
        {
           case FMT_CLI_OK:
                sprintf(sSuffix,"OK");
                break;
           case FMT_CLI_FAILED:
                sprintf(sSuffix,"FAILED");
                break;
           case FMT_CLI_START:
                sprintf(sSuffix,"START");
                break;
           case FMT_CLI_SUCCESS:
                sprintf(sSuffix,"SUCCESS");
                break;
           default:
                break;
        }
        int iStart = strlen(sTemp);
        int iTotalLen = 90;
        int i = 0;
        for(i = iStart;i < iTotalLen;++i)
        {
            sTemp[i] = '.';
        }
        if(0 == sTemp[i])
        {
            sprintf(sTemp+i,"%s",sSuffix);
        }
        std::string sRet = sTemp;
        return sRet;
    }


//}
