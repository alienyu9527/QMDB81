////////////////////////////////////////////////
// Name: TThreadLog.h
// Author: Li.ShuGang
// Date: 2008/11/12
// Description: 线程安全的日志类
////////////////////////////////////////////////
/*
* $History: TThreadLog.h $
 *
 * *****************  Version 1.0  *****************
*/

//注意:在统一个线程中,不允许多次使用TADD_START,否则会覆盖.

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



    #define  TLOG_NORMAL       0     //正常日志, 无论如何都记录(处理文件(批量)级)
    #define  TLOG_FLOW         1     //详细日志(记录级)
    #define  TLOG_FUNC         2     //一般调试日志, 输出函数调用过程
    #define  TLOG_DETAIL       3     //详细调试记录, 输出运行时数据

    #define  TLOG_WARNING -100     //一般告警(数据错误)
    #define  TLOG_FATAL   -200     //严重告警(系统资源不足, 程序断言失败)

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
        int iThreadID;   //线程ID
        int m_iNowLogLevel;
        int m_iInitLogLevel;  //TADD_START传进来的日志级别
        bool bTraceFlag;

        char sInfo[MAX_BUSINESS_NAME_LEN];
        char sPath[MAX_PATH_NAME_LEN];
        char m_pszFileName[MAX_PATH_NAME_LEN];
        char m_pszLogTemp[MAX_LOG_BUFFER_LEN];

        FILE *m_fp;

    };


    //系统日志类
    class TThreadLog
    {
    public:
        
        void Release();

    public:
        /******************************************************************************
        * 函数名称	:  Start()
        * 函数描述	:  获取当前进程ID
        * 输入		:  sDsn DSN名称，sPathName 路径，sAppName 进程名，iLevel
        *           :  日志级别，bFlag 是否在屏幕打印
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        int Start(const char* sDsn,const char * sAppName, int iLevel,bool bFlag,bool bIsMonitor=true);
        void End();

    	int OffLineStart(const char* sDsn,const char *sAppName, int iLevel,bool bFlag);
        /******************************************************************************
        * 函数名称	:  Log()
        * 函数描述	:  写日志
        * 输入		:  fmt 参数列表
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void Log(const char * fmt, ...);

        /******************************************************************************
        * 函数名称	:  LogEx()
        * 函数描述	:  记录额外信息的log
        * 输入		:  sFileName 日志的文件名，iFileLine 调用log的位置，sFuncName 函数名
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        #ifdef OS_SUN
        void LogEx(const char * sFileName,int  iFileLine,const char * fmt, ...);//记录额外信息的log
        #else
        void LogEx(const char * sFileName,int  iFileLine,const char * sFuncName,const char * fmt, ...);//记录额外信息的log
        #endif

        void PureLog(const char * fmt, ...);//只输出信息不加，文件名，行号之类

        /******************************************************************************
        * 函数名称	:  SetLogLevel()
        * 函数描述	:  设置日志级别
        * 输入		:  iLevel 日志级别
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void SetLogLevel(int iLevel);
        inline void SetNowLogLevel(int iLevel)
        {
            m_iNowLogLevel = iLevel;
        }

        /******************************************************************************
        * 函数名称	:  SetLogSize()
        * 函数描述	:  设置日志文件大小
        * 输入		:  iLogSize 日志大小(MB为单位)
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void SetLogSize(int iLogSize);

        /******************************************************************************
        * 函数名称	:  SetShowConsole()
        * 函数描述	:  设置是否在屏幕打印日志
        * 输入		:  bFlag 是否在屏幕打印
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
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
        * 函数名称	:  GetNowLogLevel()
        * 函数描述	:  获取当前日志级别
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  日志级别
        * 作者		:  li.shugang
        *******************************************************************************/
        int GetNowLogLevel();

        /******************************************************************************
        * 函数名称	:  GetLogSize()
        * 函数描述	:  获取日志文件大小
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  日志文件大小(MB)
        * 作者		:  li.shugang
        *******************************************************************************/
        int  GetLogSize() ;
        FILE *GetLogFile(void);
        bool GetStartFlag(){return (m_sRegProcName[0] == '\0');};
    public:
        TThreadLog();
        TThreadLog(const TThreadLog& log) { }
        ~TThreadLog();
        /******************************************************************************
        * 函数名称	:  GetFileSize()
        * 函数描述	:  获取日志文件大小
        * 输入		:  sFullPathFileName 日志文件名
        * 输出		:  无
        * 返回值	:  日志文件大小
        * 作者		:  li.shugang
        *******************************************************************************/
        long GetFileSize(char * sFullPathFileName);

        /******************************************************************************
        * 函数名称	:  CheckAndBackup()
        * 函数描述	:  检查文件是否超出限制，若是，则将其备份为.old文件，并创建一个新的文件
        * 输入		:  sFileName 日志文件名，fp 日志文件对应的句柄
        * 输出		:  无
        * 返回值	:  true 成功，false 失败
        * 作者		:  li.shugang
        *******************************************************************************/
        bool CheckAndBackup(const char *sFullPathFileName, const char* sNoPathFileName, FILE*& fp);

        /******************************************************************************
        * 函数名称	:  IsExist()
        * 函数描述	:  判断日志文件是否存在
        * 输入		:  sFileName 文件名
        * 输出		:  无
        * 返回值	:  true 存在，false 不存在
        * 作者		:  li.shugang
        *******************************************************************************/
        bool IsExist(const char *sFileName);

        /******************************************************************************
        * 函数名称	:  GetID()
        * 函数描述	:  获取当前进程ID
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  进程ID
        * 作者		:  li.shugang
        *******************************************************************************/
         int  GetCurPID();
        
        /******************************************************************************
        * 函数名称	:  GetLevelName()
        * 函数描述	:  获取日志级别名称
        * 输入		:  iLogLevel 日志级别
        * 输出		:  sInfo 日志级别对应的名称
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void GetLevelName(int iLogLevel, char sInfo[]);

        /******************************************************************************
        * 函数名称	:  GetProcLogLevel()
        * 函数描述	:  获取当前进程日志级别
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  日志级别
        * 作者		:  li.shugang
        *******************************************************************************/
        int GetProcLogLevel();
        FILE* GetFile();
        /******************************************************************************
        * 函数名称	:  GetCurrentTimeStr()
        * 函数描述	:  获取当前时间,字符串表示
        * 输入		:  iLen 时间字符串长度，bLongFlag 是否长格式
        * 输出		:  sTime 当前时间字符串
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void GetCurrentTimeStr(char sTime[]=NULL,size_t iLen=0,const bool bLongFlag=false);

        /******************************************************************************
        * 函数名称	:  TryAttachShm()
        * 函数描述	:  连接共享内存
        * 输入		:  sDsn DSN名
        * 输出		:  无
        * 返回值	:  true 连接成功；false 连接失败
        * 作者		:  cao.peng
        *******************************************************************************/
        bool TryAttachShm(const char* sDsn);
        int ResetLogLevelAfterDetach();//在detach之后重新设置日志级别
        std::string GetDetailErrorDesc(int iErrCode);//获取详细错误信息
        int InitLogPath(const char* sDsn);//初始化日志目录
    	
    public:
        int  m_iLogLevel;     //设定的日志的级别
        int * m_pCurLogLevel;
        char m_sPath[MAX_PATH_NAME_LEN];
        char m_pszFileName[MAX_PATH_NAME_LEN];
    	char m_NoPathFileName[MAX_PATH_NAME_LEN];
        char m_pszErrorFileName[MAX_PATH_NAME_LEN];//只记录错误日志
    	char m_NoPathErrorFileName[MAX_PATH_NAME_LEN];
        int m_iNowLogLevel;

    private:
        bool m_bShowConsole;  //是否在屏幕打印
        bool m_bMemExit;//共享内存是否存在

        int  m_iLogSize;      //设定LOG文件最大的大小，单位为M，如果超出此大小，则备份文件
        int m_iLogCount;
        TMdbErrorHelper m_tErrHelper;//错误码帮助
       //static TThreadLog* volatile m_pInstance;      //单个实例
        static TMutex m_tMutex;
        int iPos ;
        char m_sAppName[128];
        char m_sRegProcName[256];//注册时用的全名
        TMdbShmDSN *m_pShmDSN;
        TMdbProc   *m_pProc;
    	TMdbCfgDSN *m_pDsn;
        int m_iPid;
        bool m_logFlag;//日志是否生效
        //TTLInfo *m_ptInfo[MAX_THREAD_COUNTS];  //每个线程的信息
    };

    


extern "C" TThreadLog* gpLogInstance;      //单个实例

//是否使用日志,TADD == Thread Add
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

//定义打印信息宏
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
//sun环境中不支持##消除逗号所以在外面包了一层
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

//sun机器下TADD_ERROR_CODE = TADD_ERROR
#define TADD_ERROR_CODE(_iErrCode,...) QMDB_LOG_INST->SetNowLogLevel(TLOG_FATAL), QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__),QMDB_LOG_INST->PureLog(__VA_ARGS__)

#define TADD_WARNING_NO_SCREEN(...) QMDB_LOG_INST->SetNowLogLevel(TLOG_WARNING), QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__)

#define TADD_NORMAL_NO_SCREEN(...) if(0 <= *(QMDB_LOG_INST->m_pCurLogLevel)) \
		QMDB_LOG_INST->SetNowLogLevel(TLOG_NORMAL),QMDB_LOG_INST->LogEx( __WHERE_SHORT__,__VA_ARGS__)


#else
//sun机器下编译错误

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


//判断_ret值,_ret可能是函数返回。若不为0则报错并返回错误码
#define CHECK_RET(_ret,...) if((iRet = _ret)!= 0){ TADD_ERROR_CODE(iRet,__VA_ARGS__);return iRet;}
//安全删除指针
#define SAFE_DELETE(_obj) if(NULL != _obj){delete _obj;_obj=NULL;}
//安全删除指针数组
#define SAFE_DELETE_ARRAY(_obj) if(NULL != _obj){delete[] _obj;_obj=NULL;}
//判断指针是否为空，若为空，报错并返回错误码
#define CHECK_OBJ(_obj) if(NULL == _obj){TADD_ERROR_CODE(ERR_APP_PARAM_INVALID, #_obj" is null"); return ERR_APP_PARAM_INVALID;}
#define CHECK_OBJ_BREAK(_obj) if(NULL == _obj){TADD_ERROR_CODE(ERR_APP_PARAM_INVALID, #_obj" is null"); iRet = ERR_APP_PARAM_INVALID;break;}

//判断_ret值，若不为0，则报错并break
#define CHECK_RET_BREAK(_ret,...) if((iRet = _ret)!=0){TADD_ERROR(_ret,__VA_ARGS__);break;}
//安全close指针
#define SAFE_CLOSE(_fp) if(NULL != _fp){fclose(_fp);_fp=NULL;}
//安全free指针
#define SAFE_FREE(obj) if (NULL != obj){free(obj);obj = NULL;}
//获取当前进程ID
//#define TMDB_CUR_PID ( TThreadLog::IsInstance()?QMDB_LOG_INST->GetCurPID():0)
#define TMDB_CUR_PID  QMDB_LOG_INST->GetCurPID()

#define CHECK_RET_NONE(_ret,...) if((iRet = _ret)!= 0){ TADD_ERROR(iRet,__VA_ARGS__);}



//}


#endif //__THREAD_SAFE_LOG_H__
