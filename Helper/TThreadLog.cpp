////////////////////////////////////////////////
// Name: TThreadLog.cpp
// Author: Li.ShuGang
// Date: 2008/11/12
// Description: �̰߳�ȫ����־��
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

    //����һ������LOGID
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
    * ��������	:  Clear()
    * ��������	:  ��ʼ��
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
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
        //������ʼ��
        m_bShowConsole = false;
        m_iLogLevel = 0;  //��ǰ��־�ļ���
        m_iLogSize  = 128; //�趨LOG�ļ����Ĵ�С����λΪM����������˴�С���򱸷��ļ�
        m_iLogCount = 1;
        m_iNowLogLevel = 0;
        m_pShmDSN = NULL;
        m_iPid = 0;
        m_bMemExit = false;
        m_logFlag = false;
        m_pCurLogLevel = &m_iLogLevel;
        //��ȡ��־Ŀ¼
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
    * ��������	:  IsExist()
    * ��������	:  �ж���־�ļ��Ƿ����
    * ����		:  sFileName �ļ���
    * ���		:  ��
    * ����ֵ	:  true ���ڣ�false ������
    * ����		:  li.shugang
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
    * ��������	:  GetID()
    * ��������	:  ��ȡ��ǰ����ID
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ����ID
    * ����		:  li.shugang
    *******************************************************************************/
    int TThreadLog::GetCurPID()
    {
        if(m_iPid <= 0){m_iPid = TMdbOS::GetPID();}
        return m_iPid;
    }

    /******************************************************************************
    * ��������	:  Start()
    * ��������	:  ��ȡ��ǰ����ID
    * ����		:  sDsn DSN���ƣ�sPathName ·����sAppName ��������iLevel
    *           :  ��־����bFlag �Ƿ�����Ļ��ӡ
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
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
            //��ȡ����������
            m_tErrHelper.InitErrorDescription();//���۴�����񶼲��ж�
            //��ȡ����LogLevel
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
            //��������
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
            //��ȡ����������
            m_tErrHelper.InitErrorDescription();
            //��ȡ����LogLevel
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
    * ��������	:  AttachShm()
    * ��������	:  ���ӹ����ڴ�
    * ����		:  sDsn DSN��
    * ���		:  ��
    * ����ֵ	:  true ���ӳɹ���false ����ʧ��
    * ����		:  cao.peng
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
    * ��������	:  SetLogLevel()
    * ��������	:  ������־����
    * ����		:  iLevel ��־����
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TThreadLog::SetLogLevel(int iLevel)
    {
        //���ý�����־����
        //���ﻹ�����ô������־���
        if(m_logFlag == false)
        {
            return;
        }
        m_iLogLevel = iLevel;
    }

    /******************************************************************************
    * ��������	:  GetNowLogLevel()
    * ��������	:  ��ȡ��ǰ��־����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ��־����
    * ����		:  li.shugang
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
    * ��������	:  SetLogSize()
    * ��������	:  ������־�ļ���С
    * ����		:  iLogSize ��־��С(MBΪ��λ)
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
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
    * ��������	:  GetLogSize()
    * ��������	:  ��ȡ��־�ļ���С
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ��־�ļ���С(MB)
    * ����		:  li.shugang
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
    * ��������	:  SetShowConsole()
    * ��������	:  �����Ƿ�����Ļ��ӡ��־
    * ����		:  bFlag �Ƿ�����Ļ��ӡ
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
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
    * ��������	:  GetLevelName()
    * ��������	:  ��ȡ��־��������
    * ����		:  iLogLevel ��־����
    * ���		:  sInfo ��־�����Ӧ������
    * ����ֵ	:  ��
    * ����		:  li.shugang
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
    * ��������	:  GetProcLogLevel()
    * ��������	:  ��ȡ��ǰ������־����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ��־����
    * ����		:  li.shugang
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
    * ��������	:  LogEx()
    * ��������	:  ��¼������Ϣ��log
    * ����		:  sFileName ��־���ļ�����iFileLine ����log��λ�ã�sFuncName ������
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
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
        Log("[%s:%d]: %s",sFileName,iFileLine,sLogTemp);//��¼
        #else
        Log("[%s:%d][%s]: %s",sFileName,iFileLine,sFuncName,sLogTemp);//��¼
        #endif
    }

    /******************************************************************************
    * ��������	:  PureLog
    * ��������	:  ֻ�����Ϣ���ӣ��ļ������к�֮��
    * ����		:  
    * ���		:  
    * ����ֵ	:  
    * ����		:  jin.shaohua
    *******************************************************************************/
    void TThreadLog::PureLog(const char * fmt, ...)
    {
        //��Ҫ�������Ļ
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
    * ��������	:  Log()
    * ��������	:  д��־
    * ����		:  fmt �����б�
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
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
            //���ڴ�����־��ר��д��һ���ļ���ȥ
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
    * ��������	:  GetFileSize()
    * ��������	:  ��ȡ��־�ļ���С
    * ����		:  sFullPathFileName ��־�ļ���
    * ���		:  ��
    * ����ֵ	:  ��־�ļ���С
    * ����		:  li.shugang
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
    * ��������	:  CheckAndBackup()
    * ��������	:  ����ļ��Ƿ񳬳����ƣ����ǣ����䱸��Ϊ.old�ļ���������һ���µ��ļ�
    * ����		:  sFileName ��־�ļ�����fp ��־�ļ���Ӧ�ľ��
    * ���		:  ��
    * ����ֵ	:  true �ɹ���false ʧ��
    * ����		:  li.shugang
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
        //�ж��ļ��Ƿ����
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
            //���������,�Ե�ǰʱ��YYYYMMDDHHM24SS��ʾ
            SAFE_CLOSE(fp);
            char sCurTime[MAX_TIME_LEN];
            TMdbDateTime::GetCurrentTimeStr(sCurTime);
            sprintf(sFileNameOld,"%s.%s.%s", sFileName, "old", sCurTime);
        	snprintf(sNoPathFileNameOld,sizeof(sNoPathFileNameOld),"%s.%s", sNoPathFileName, "old");
            TMdbNtcFileOper::Remove(sFileNameOld);
            
            //����־�ļ�������
            TMdbNtcFileOper::Rename(sFileName,sFileNameOld);

            TMdbNtcFileOper::Remove(sFileName);
            fp = fopen (sFileName,"a+");
            if(fp == NULL)
            {
                printf("Check And Backup Log File: Open file [%s] fail.\n",sFileName);
                return false;
            }
			
			TMdbFileList m_tFileList; //�����Դ����ļ�
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
    * ��������	:  GetCurrentTimeStr()
    * ��������	:  ��ȡ��ǰʱ��,�ַ�����ʾ
    * ����		:  iLen ʱ���ַ������ȣ�bLongFlag �Ƿ񳤸�ʽ
    * ���		:  sTime ��ǰʱ���ַ���
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TThreadLog::GetCurrentTimeStr(char sTime[], size_t iLen, const bool bLongFlag)
    {
        time_t tCurrent;
        struct tm *tm_Cur;
        char sCurtime[40];
        sCurtime[39]=0;
        time(&tCurrent); //ȡ�õ�ǰʱ���time_tֵ
        tm_Cur = localtime(&tCurrent); //ȡ�õ�ǰʱ���tmֵ

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
    * ��������	:  ResetLogLevelAfterDetach
    * ��������	:  ��detach֮������������־����
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  GetDetailErrorDesc
    * ��������	: ��ȡ��ϸ������Ϣ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    std::string TThreadLog::GetDetailErrorDesc(int iErrCode)
    {
        m_tErrHelper.InitErrorDescription();
        iErrCode = iErrCode<0?0-iErrCode:iErrCode;//��С��0ȡ��
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
        //�ж�Ŀ¼�Ƿ����.
#else
        sprintf(m_sPath, "%s/log/%s/", getenv("QuickMDB_HOME"),sDsnName);
#endif
        //������־Ŀ¼
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
