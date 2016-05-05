////////////////////////////////////////////////
// Name: mdbOraLog.h
// Author: Li.ShuGang
// Date: 2009/03/25
// Description: 刷新Oracle的日志类
////////////////////////////////////////////////
/*
* $History: mdbOraLog.h $
* 
* *****************  Version 1.0  ***************** 
*/

#ifndef __MDB_ORACLE_LOG_H__
#define __MDB_ORACLE_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <string>
#include <sstream>
#include "Helper/TMutex.h"
#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"
#include "Control/mdbFlush.h"
#include "Control/mdbProcCtrl.h"
#include "Helper/mdbQueue.h"
#include "Control/mdbStorageEngine.h"

using namespace std;

//namespace QuickMDB{



#define MAX_BUSINESS_NAME_LEN 32
#define MAX_PATH_NAME_LEN 512
#define MAX_LOG_BUFFER_LEN 10240



    //系统日志类
    class TMdbOraLog 
    {
    public:
        TMdbOraLog();
        ~TMdbOraLog();

        //设置DSN名称
        int Init(const char* pszDSN, TMdbQueue & mdbQueueCtrl);

        //把数据落地为文件日志
        int Log(bool bEmpty = false);	

    private:    
        void CheckBack();                                                  //检查是否需要备份
        long GetFileSize(char *sFullPathFileName);                         //获取文件的大小，单位是字节
        int  GetPos(const char* pszMsg, int iLen);
        void CheckAndWriteToFile(int &iBuffPos,int &iFilePos,char* sdest,char*ssrc,char* sFirst,int iLen,FILE *fp);
        void WriteToFile(int& iFileBuffPos,char* sFileBuff,FILE *fp);
        bool CheckAndBackup(char* sFileName,int &iFilePos,FILE *&fp,char* sOldTime);

    private:
        long m_iNumber;       //计数,用来记录update文件的序号
        int  m_iLogTime;      //设定日志时间，单位秒
        int  m_iLogSize;      //设定LOG文件最大的大小，单位为M，如果超出此大小，则备份文件
        int m_iCheckCounts;
        char m_sLogPath[MAX_PATH_NAME_LEN]; //日志目录
        int  m_iPID;          //进程PID
        char m_sDSN[64];      //DSN名称
        TMdbQueue *m_pQueueCtrl;//内存缓冲管理器
        int  m_iOraRepCounts;
        char* m_pszRecord;


        FILE* m_fp[10];            //文件句柄
        char m_pszFileName[MAX_PATH_NAME_LEN][MAX_PATH_NAME_LEN];
        char m_sOldTime[MAX_TIME_LEN];
        char m_sOraOldTime[10][MAX_TIME_LEN];
        char* m_pszMyFileTemp[10];  //文件缓冲
        int  m_iMyFilePos[10];     //文件位置

        int  m_iPosInst;
        int m_iPosOra[10];
        FILE* m_fpInst;
        char m_pszFileNameInst[MAX_PATH_NAME_LEN];
        char* m_psMyFileTempInsert;
        int  m_iMyFilePosInsert;

        TMdbShmDSN *m_pShmDSN;
        TMdbConfig *m_pConfig;
        TMdbDSN    *m_pDsn; 
        int  m_iBufSize;
        int *m_pLogSize;
        TMdbProcCtrl	m_tProcCtrl;
        TMdbRedoLogParser m_tParser;
        TMdbLCR m_curLcr;
    };

    class TMdbRedoLog
    {
    public:
        TMdbRedoLog();
        ~TMdbRedoLog();
        int Init(const char* pszDSN, TMdbQueue & mdbQueueCtrl);
        int Log(bool bEmpty = false);
    private:    
        void CheckBack();                                                  //检查是否需要备份
        long GetFileSize(char *sFullPathFileName);                         //获取文件的大小，单位是字节
        void CheckAndWriteToFile(int &iBuffPos,int &iFilePos,char* sdest,char*ssrc,char* sFirst,int iLen,FILE *fp);
        void WriteToFile(int& iFileBuffPos,char* sFileBuff,FILE *fp);
        bool CheckAndBackup(char* sFileName,int &iFilePos,FILE *&fp,char* sOldTime);
        long long GetLsn();	
    private:
        long m_iNumber;       //计数,用来记录update文件的序号
        int  m_iLogTime;      //设定日志时间，单位秒
        int  m_iLogSize;      //设定LOG文件最大的大小，单位为M，如果超出此大小，则备份文件
        int m_iCheckCounts;
        char m_sLogPath[MAX_PATH_NAME_LEN]; //日志目录
        int  m_iPID;          //进程PID
        char m_sDSN[64];      //DSN名称
        TMdbQueue *m_pQueueCtrl;//内存缓冲管理器
        char* m_pszRecord;
        long long m_iLsn;


        FILE* m_fp;            //文件句柄
        char m_pszFileName[MAX_PATH_NAME_LEN];
        char m_sOldTime[MAX_TIME_LEN];
        char* m_pszMyFileTemp;  //文件缓冲
        int  m_iMyFilePos;     //文件位置
        int m_iPos;

        TMdbShmDSN *m_pShmDSN;
        TMdbConfig *m_pConfig;
        TMdbDSN    *m_pDsn; 
        int  m_iBufSize;
        int *m_pLogSize;
        TMdbProcCtrl	m_tProcCtrl;
        TMdbRedoLogParser m_tParser;
        TMdbLCR m_curLcr;
    };
//}

#endif //__MDB_ORACLE_LOG_H__
