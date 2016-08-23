////////////////////////////////////////////////
// Name: TThreadLog.h
// Author: Li.ShuGang
// Date: 2008/11/12
// Description: �̰߳�ȫ����־��
////////////////////////////////////////////////
/*
* $History: TThreadLog.h $
 *
 * *****************  Version 1.0  *****************
*/

//ע��:��ͳһ���߳���,��������ʹ��TADD_START,����Ḳ��.

#ifndef __THREAD_SAFE_LOG_H__
#define __THREAD_SAFE_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <string>
#include <sstream>
#include "Helper/TMutex.h"
#include "Helper/mdbErrorHelper.h"

using namespace std;

//namespace QuickMDB{
    
    class TMdbShmDSN;
    class TMdbDSN;
    class TMdbProc;
    class TMdbProcCtrl;
    class TMdbCfgDSN;



    #define  TLOG_NORMAL       0     //������־, ������ζ���¼(�����ļ�(����)��)
    #define  TLOG_FLOW         1     //��ϸ��־(��¼��)
    #define  TLOG_FUNC         2     //һ�������־, ����������ù���
    #define  TLOG_DETAIL       3     //��ϸ���Լ�¼, �������ʱ����

    #define  TLOG_WARNING -100     //һ��澯(���ݴ���)
    #define  TLOG_FATAL   -200     //���ظ澯(ϵͳ��Դ����, �������ʧ��)

    #define MAX_THREAD_COUNTS 128
    #define MAX_SYSTEM_THREAD_COUNTS 32
    #define MAX_BUSINESS_NAME_LEN 32
    #define MAX_PATH_NAME_LEN 512
    #define MAX_LOG_BUFFER_LEN 10240

    enum E_FORMAT_CLI
    {
        FMT_CLI_OK          = 1,  //
        FMT_CLI_FAILED  = 2,
        FMT_CLI_START   = 3,
        FMT_CLI_SUCCESS = 4
    };

    class TTLInfo
    {
    public:
        TTLInfo();
        ~TTLInfo();
        void Clear();

    public:
        int iThreadID;   //�߳�ID
        int m_iNowLogLevel;
        int m_iInitLogLevel;  //TADD_START����������־����
        bool bTraceFlag;

        char sInfo[MAX_BUSINESS_NAME_LEN];
        char sPath[MAX_PATH_NAME_LEN];
        char m_pszFileName[MAX_PATH_NAME_LEN];
        char m_pszLogTemp[MAX_LOG_BUFFER_LEN];

        FILE *m_fp;

    };


    //ϵͳ��־��
    class TThreadLog
    {
    public:
        
        void Release();

    public:
        /******************************************************************************
        * ��������	:  Start()
        * ��������	:  ��ȡ��ǰ����ID
        * ����		:  sDsn DSN���ƣ�sPathName ·����sAppName ��������iLevel
        *           :  ��־����bFlag �Ƿ�����Ļ��ӡ
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        int Start(const char* sDsn,const char * sAppName, int iLevel,bool bFlag,bool bIsMonitor=true);
        void End();

    	int OffLineStart(const char* sDsn,const char *sAppName, int iLevel,bool bFlag);
        /******************************************************************************
        * ��������	:  Log()
        * ��������	:  д��־
        * ����		:  fmt �����б�
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void Log(const char * fmt, ...);

        /******************************************************************************
        * ��������	:  LogEx()
        * ��������	:  ��¼������Ϣ��log
        * ����		:  sFileName ��־���ļ�����iFileLine ����log��λ�ã�sFuncName ������
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        #ifdef OS_SUN
        void LogEx(const char * sFileName,int  iFileLine,const char * fmt, ...);//��¼������Ϣ��log
        #else
        void LogEx(const char * sFileName,int  iFileLine,const char * sFuncName,const char * fmt, ...);//��¼������Ϣ��log
        #endif

        void PureLog(const char * fmt, ...);//ֻ�����Ϣ���ӣ��ļ������к�֮��

        /******************************************************************************
        * ��������	:  SetLogLevel()
        * ��������	:  ������־����
        * ����		:  iLevel ��־����
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void SetLogLevel(int iLevel);
        inline void SetNowLogLevel(int iLevel)
        {
            m_iNowLogLevel = iLevel;
        }

        /******************************************************************************
        * ��������	:  SetLogSize()
        * ��������	:  ������־�ļ���С
        * ����		:  iLogSize ��־��С(MBΪ��λ)
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void SetLogSize(int iLogSize);

        /******************************************************************************
        * ��������	:  SetShowConsole()
        * ��������	:  �����Ƿ�����Ļ��ӡ��־
        * ����		:  bFlag �Ƿ�����Ļ��ӡ
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void SetShowConsole(bool bFlag);
        void SetTrace(bool bFlag);
        void SetTraceInfo(const char* pszInfo);

        string  FormatToCLI(int iFmtCliType,const char * fmt,...);

        inline int  GetLogLevel()
        {
            return m_iLogLevel;
        }

        /******************************************************************************
        * ��������	:  GetNowLogLevel()
        * ��������	:  ��ȡ��ǰ��־����
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��־����
        * ����		:  li.shugang
        *******************************************************************************/
        int GetNowLogLevel();

        /******************************************************************************
        * ��������	:  GetLogSize()
        * ��������	:  ��ȡ��־�ļ���С
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��־�ļ���С(MB)
        * ����		:  li.shugang
        *******************************************************************************/
        int  GetLogSize() ;
        FILE *GetLogFile(void);
        bool GetStartFlag(){return (m_sRegProcName[0] == '\0');};
    public:
        TThreadLog();
        TThreadLog(const TThreadLog& log) { }
        ~TThreadLog();
        /******************************************************************************
        * ��������	:  GetFileSize()
        * ��������	:  ��ȡ��־�ļ���С
        * ����		:  sFullPathFileName ��־�ļ���
        * ���		:  ��
        * ����ֵ	:  ��־�ļ���С
        * ����		:  li.shugang
        *******************************************************************************/
        long GetFileSize(char * sFullPathFileName);

        /******************************************************************************
        * ��������	:  CheckAndBackup()
        * ��������	:  ����ļ��Ƿ񳬳����ƣ����ǣ����䱸��Ϊ.old�ļ���������һ���µ��ļ�
        * ����		:  sFileName ��־�ļ�����fp ��־�ļ���Ӧ�ľ��
        * ���		:  ��
        * ����ֵ	:  true �ɹ���false ʧ��
        * ����		:  li.shugang
        *******************************************************************************/
        bool CheckAndBackup(const char *sFullPathFileName, const char* sNoPathFileName, FILE*& fp);

        /******************************************************************************
        * ��������	:  IsExist()
        * ��������	:  �ж���־�ļ��Ƿ����
        * ����		:  sFileName �ļ���
        * ���		:  ��
        * ����ֵ	:  true ���ڣ�false ������
        * ����		:  li.shugang
        *******************************************************************************/
        bool IsExist(const char *sFileName);

        /******************************************************************************
        * ��������	:  GetID()
        * ��������	:  ��ȡ��ǰ����ID
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ����ID
        * ����		:  li.shugang
        *******************************************************************************/
         int  GetCurPID();
        
        /******************************************************************************
        * ��������	:  GetLevelName()
        * ��������	:  ��ȡ��־��������
        * ����		:  iLogLevel ��־����
        * ���		:  sInfo ��־�����Ӧ������
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void GetLevelName(int iLogLevel, char sInfo[]);

        /******************************************************************************
        * ��������	:  GetProcLogLevel()
        * ��������	:  ��ȡ��ǰ������־����
        * ����		:  ��
        * ���		:  ��
        * ����ֵ	:  ��־����
        * ����		:  li.shugang
        *******************************************************************************/
        int GetProcLogLevel();
        FILE* GetFile();
        /******************************************************************************
        * ��������	:  GetCurrentTimeStr()
        * ��������	:  ��ȡ��ǰʱ��,�ַ�����ʾ
        * ����		:  iLen ʱ���ַ������ȣ�bLongFlag �Ƿ񳤸�ʽ
        * ���		:  sTime ��ǰʱ���ַ���
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void GetCurrentTimeStr(char sTime[]=NULL,size_t iLen=0,const bool bLongFlag=false);

        /******************************************************************************
        * ��������	:  TryAttachShm()
        * ��������	:  ���ӹ����ڴ�
        * ����		:  sDsn DSN��
        * ���		:  ��
        * ����ֵ	:  true ���ӳɹ���false ����ʧ��
        * ����		:  cao.peng
        *******************************************************************************/
        bool TryAttachShm(const char* sDsn);
        int ResetLogLevelAfterDetach();//��detach֮������������־����
        std::string GetDetailErrorDesc(int iErrCode);//��ȡ��ϸ������Ϣ
        int InitLogPath(const char* sDsn);//��ʼ����־Ŀ¼
    	
    public:
        int  m_iLogLevel;     //�趨����־�ļ���
        int * m_pCurLogLevel;
        char m_sPath[MAX_PATH_NAME_LEN];
        char m_pszFileName[MAX_PATH_NAME_LEN];
    	char m_NoPathFileName[MAX_PATH_NAME_LEN];
        char m_pszErrorFileName[MAX_PATH_NAME_LEN];//ֻ��¼������־
    	char m_NoPathErrorFileName[MAX_PATH_NAME_LEN];
        int m_iNowLogLevel;

    private:
        bool m_bShowConsole;  //�Ƿ�����Ļ��ӡ
        bool m_bMemExit;//�����ڴ��Ƿ����

        int  m_iLogSize;      //�趨LOG�ļ����Ĵ�С����λΪM����������˴�С���򱸷��ļ�
        int m_iLogCount;
        TMdbErrorHelper m_tErrHelper;//���������
       //static TThreadLog* volatile m_pInstance;      //����ʵ��
        static TMutex m_tMutex;
        int iPos ;
        char m_sAppName[128];
        char m_sRegProcName[256];//ע��ʱ�õ�ȫ��
        TMdbShmDSN *m_pShmDSN;
        TMdbProc   *m_pProc;
    	TMdbCfgDSN *m_pDsn;
        int m_iPid;
        bool m_logFlag;//��־�Ƿ���Ч
        //TTLInfo *m_ptInfo[MAX_THREAD_COUNTS];  //ÿ���̵߳���Ϣ
    };

    


extern "C" TThreadLog* gpLogInstance;      //����ʵ��

//�Ƿ�ʹ����־,TADD == Thread Add
#ifndef _DEBUG
#define TIS_DEBUG(x) if(false)
#else
#define TIS_DEBUG(x) if(true)
#endif

#define QMDB_LOG_INST gpLogInstance

#define TRACE_START(x) QMDB_LOG_INST->IsTrackFlag = (x)
#define TADD_START(sDsn,sAppName,iLevel,bFlag,bIsMonitor) QMDB_LOG_INST->Start(sDsn, sAppName, iLevel,bFlag,bIsMonitor)
#define TADD_OFFSTART(sDsn,sAppName,iLevel,bFlag) QMDB_LOG_INST->OffLineStart(sDsn,sAppName, iLevel,bFlag)

#define TADD_END() QMDB_LOG_INST->End()
#define TADD_SET_LOG_LEVEL(iLevel) QMDB_LOG_INST->SetLogLevel(iLevel)
#define TADD_SET_LOG_SHOWCONSOLE(isShow) QMDB_LOG_INST->SetShowConsole(isShow)

#define TADD_START_FLAG QMDB_LOG_INST->GetStartFlag()
#define TADD_REST_LOG_LEVEL_AFTER_DETACH QMDB_LOG_INST->ResetLogLevelAfterDetach()
#define TADD_ERROR_HELPER(_code)  QMDB_LOG_INST->GetDetailErrorDesc(_code).c_str()

//�����ӡ��Ϣ��
#ifdef OS_SUN
#define __WHERE_SHORT__ __FILE__,__LINE__
#define __WHEREFORMAT_SHORT__ "[%s:%d]: "
#else
#define __WHERE_SHORT__ __FILE__,__LINE__,__FUNCTION__
#define __WHEREFORMAT_SHORT__ "[%s:%d][%s]: "
#endif


#if 1
//add by jin.shaohua
#ifdef OS_SUN
#define __FUNCTION__ ""
//sun�����в�֧��##���������������������һ��
#define TADD_ERROR(RetCode ,...) QMDB_LOG_INST->SetNowLogLevel(TLOG_FATAL), QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__),QMDB_LOG_INST->PureLog(__VA_ARGS__)

#define TADD_WARNING(...) QMDB_LOG_INST->SetNowLogLevel(TLOG_WARNING), QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__),QMDB_LOG_INST->PureLog(__VA_ARGS__)

#define TADD_NORMAL(...) if(0 <= *(QMDB_LOG_INST->m_pCurLogLevel)) \
		QMDB_LOG_INST->SetNowLogLevel(TLOG_NORMAL),QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__),QMDB_LOG_INST->PureLog(__VA_ARGS__)

#define TADD_FLOW(...)  if(1 <= *(QMDB_LOG_INST->m_pCurLogLevel)) \
		QMDB_LOG_INST->SetNowLogLevel(TLOG_FLOW),QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__),QMDB_LOG_INST->PureLog(__VA_ARGS__)

#define TADD_FUNC(...) if(2 <= *(QMDB_LOG_INST->m_pCurLogLevel)) \
		QMDB_LOG_INST->SetNowLogLevel(TLOG_FUNC),QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__),QMDB_LOG_INST->PureLog(__VA_ARGS__)

#define TADD_DETAIL(...) if(3 <= *(QMDB_LOG_INST->m_pCurLogLevel))  \
		QMDB_LOG_INST->SetNowLogLevel(TLOG_DETAIL),QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__),QMDB_LOG_INST->PureLog(__VA_ARGS__)
		
#define TADD_NORMAL_TO_CLI(iSuffixType,...) TADD_NORMAL("%s",QMDB_LOG_INST->FormatToCLI(iSuffixType,__VA_ARGS__).c_str())

#define TADD_ERROR_TO_CLI(RetCode,iSuffixType,...) TADD_ERROR(RetCode,"%s",QMDB_LOG_INST->FormatToCLI(iSuffixType,__VA_ARGS__).c_str())

//sun������TADD_ERROR_CODE = TADD_ERROR
#define TADD_ERROR_CODE(_iErrCode,...) QMDB_LOG_INST->SetNowLogLevel(TLOG_FATAL), QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__),QMDB_LOG_INST->PureLog(__VA_ARGS__)

#define TADD_WARNING_NO_SCREEN(...) QMDB_LOG_INST->SetNowLogLevel(TLOG_WARNING), QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__)

#define TADD_NORMAL_NO_SCREEN(...) if(0 <= *(QMDB_LOG_INST->m_pCurLogLevel)) \
		QMDB_LOG_INST->SetNowLogLevel(TLOG_NORMAL),QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__)


#else
//sun�����±������

#define TADD_ERROR(RetCode,FMT,...) QMDB_LOG_INST->SetNowLogLevel(TLOG_FATAL), QMDB_LOG_INST->Log(__WHEREFORMAT_SHORT__ FMT,__WHERE_SHORT__,##__VA_ARGS__),QMDB_LOG_INST->PureLog(FMT,##__VA_ARGS__)

#define TADD_WARNING(FMT,...) QMDB_LOG_INST->SetNowLogLevel(TLOG_WARNING), QMDB_LOG_INST->Log(__WHEREFORMAT_SHORT__ FMT,__WHERE_SHORT__,##__VA_ARGS__),QMDB_LOG_INST->PureLog(FMT,##__VA_ARGS__)

#define TADD_NORMAL(FMT,...) if(0 <= *(QMDB_LOG_INST->m_pCurLogLevel)) \
		QMDB_LOG_INST->SetNowLogLevel(TLOG_NORMAL),QMDB_LOG_INST->Log(__WHEREFORMAT_SHORT__ FMT,__WHERE_SHORT__,##__VA_ARGS__),QMDB_LOG_INST->PureLog(FMT,##__VA_ARGS__)

#define TADD_FLOW(FMT,...)  if(1 <= *(QMDB_LOG_INST->m_pCurLogLevel)) \
		QMDB_LOG_INST->SetNowLogLevel(TLOG_FLOW),QMDB_LOG_INST->Log(__WHEREFORMAT_SHORT__ FMT,__WHERE_SHORT__,##__VA_ARGS__),QMDB_LOG_INST->PureLog(FMT,##__VA_ARGS__)

#define TADD_FUNC(FMT,...) if(2 <= *(QMDB_LOG_INST->m_pCurLogLevel)) \
			QMDB_LOG_INST->SetNowLogLevel(TLOG_FUNC),QMDB_LOG_INST->Log(__WHEREFORMAT_SHORT__ FMT,__WHERE_SHORT__,##__VA_ARGS__),QMDB_LOG_INST->PureLog(FMT,##__VA_ARGS__)

#define TADD_DETAIL(FMT,...) if(3 <= *(QMDB_LOG_INST->m_pCurLogLevel))  \
		QMDB_LOG_INST->SetNowLogLevel(TLOG_DETAIL),QMDB_LOG_INST->Log(__WHEREFORMAT_SHORT__ FMT,__WHERE_SHORT__,##__VA_ARGS__),QMDB_LOG_INST->PureLog(FMT,##__VA_ARGS__)

#define TADD_NORMAL_TO_CLI(iSuffixType,FMT,...) TADD_NORMAL("%s",QMDB_LOG_INST->FormatToCLI(iSuffixType,FMT,##__VA_ARGS__).c_str())

#define TADD_ERROR_TO_CLI(RetCode,iSuffixType,FMT,...) TADD_ERROR(RetCode,"%s",QMDB_LOG_INST->FormatToCLI(iSuffixType,FMT,##__VA_ARGS__).c_str())

#define TADD_ERROR_CODE(_iErrCode,FMT,...) TADD_ERROR(_iErrCode,"%s"FMT,QMDB_LOG_INST->GetDetailErrorDesc(_iErrCode).c_str(),##__VA_ARGS__)

#define TADD_NORMAL_NO_SCREEN(FMT,...) if(0 <= *(QMDB_LOG_INST->m_pCurLogLevel)) \
		QMDB_LOG_INST->SetNowLogLevel(TLOG_NORMAL),QMDB_LOG_INST->Log(__WHEREFORMAT_SHORT__ FMT,__WHERE_SHORT__,##__VA_ARGS__)
#define TADD_WARNING_NO_SCREEN(FMT,...) QMDB_LOG_INST->SetNowLogLevel(TLOG_WARNING), QMDB_LOG_INST->Log(__WHEREFORMAT_SHORT__ FMT,__WHERE_SHORT__,##__VA_ARGS__)

#endif
#endif


#define IS_LOG(x) if(x <= *(QMDB_LOG_INST->m_pCurLogLevel))


//�ж�_retֵ,_ret�����Ǻ������ء�����Ϊ0�򱨴����ش�����
#define CHECK_RET(_ret,...) if((iRet = _ret)!= 0){ TADD_ERROR_CODE(iRet,__VA_ARGS__);return iRet;}
//��ȫɾ��ָ��
#define SAFE_DELETE(_obj) if(NULL != _obj){delete _obj;_obj=NULL;}
//��ȫɾ��ָ������
#define SAFE_DELETE_ARRAY(_obj) if(NULL != _obj){delete[] _obj;_obj=NULL;}
//�ж�ָ���Ƿ�Ϊ�գ���Ϊ�գ��������ش�����
#define CHECK_OBJ(_obj) if(NULL == _obj){TADD_ERROR_CODE(ERR_APP_PARAM_INVALID, #_obj" is null"); return ERR_APP_PARAM_INVALID;}
#define CHECK_OBJ_BREAK(_obj) if(NULL == _obj){TADD_ERROR_CODE(ERR_APP_PARAM_INVALID, #_obj" is null"); iRet = ERR_APP_PARAM_INVALID;break;}

//�ж�_retֵ������Ϊ0���򱨴�break
#define CHECK_RET_BREAK(_ret,...) if((iRet = _ret)!=0){TADD_ERROR(_ret,__VA_ARGS__);break;}
//��ȫcloseָ��
#define SAFE_CLOSE(_fp) if(NULL != _fp){fclose(_fp);_fp=NULL;}
//��ȫfreeָ��
#define SAFE_FREE(obj) if (NULL != obj){free(obj);obj = NULL;}
//��ȡ��ǰ����ID
//#define TMDB_CUR_PID ( TThreadLog::IsInstance()?QMDB_LOG_INST->GetCurPID():0)
#define TMDB_CUR_PID  QMDB_LOG_INST->GetCurPID()

#define CHECK_RET_NONE(_ret,...) if((iRet = _ret)!= 0){ TADD_ERROR(iRet,__VA_ARGS__);}



//}


#endif //__THREAD_SAFE_LOG_H__
