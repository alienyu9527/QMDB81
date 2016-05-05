////////////////////////////////////////////////
// Name: mdbOraLog.h
// Author: Li.ShuGang
// Date: 2009/03/25
// Description: ˢ��Oracle����־��
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



    //ϵͳ��־��
    class TMdbOraLog 
    {
    public:
        TMdbOraLog();
        ~TMdbOraLog();

        //����DSN����
        int Init(const char* pszDSN, TMdbQueue & mdbQueueCtrl);

        //���������Ϊ�ļ���־
        int Log(bool bEmpty = false);	

    private:    
        void CheckBack();                                                  //����Ƿ���Ҫ����
        long GetFileSize(char *sFullPathFileName);                         //��ȡ�ļ��Ĵ�С����λ���ֽ�
        int  GetPos(const char* pszMsg, int iLen);
        void CheckAndWriteToFile(int &iBuffPos,int &iFilePos,char* sdest,char*ssrc,char* sFirst,int iLen,FILE *fp);
        void WriteToFile(int& iFileBuffPos,char* sFileBuff,FILE *fp);
        bool CheckAndBackup(char* sFileName,int &iFilePos,FILE *&fp,char* sOldTime);

    private:
        long m_iNumber;       //����,������¼update�ļ������
        int  m_iLogTime;      //�趨��־ʱ�䣬��λ��
        int  m_iLogSize;      //�趨LOG�ļ����Ĵ�С����λΪM����������˴�С���򱸷��ļ�
        int m_iCheckCounts;
        char m_sLogPath[MAX_PATH_NAME_LEN]; //��־Ŀ¼
        int  m_iPID;          //����PID
        char m_sDSN[64];      //DSN����
        TMdbQueue *m_pQueueCtrl;//�ڴ滺�������
        int  m_iOraRepCounts;
        char* m_pszRecord;


        FILE* m_fp[10];            //�ļ����
        char m_pszFileName[MAX_PATH_NAME_LEN][MAX_PATH_NAME_LEN];
        char m_sOldTime[MAX_TIME_LEN];
        char m_sOraOldTime[10][MAX_TIME_LEN];
        char* m_pszMyFileTemp[10];  //�ļ�����
        int  m_iMyFilePos[10];     //�ļ�λ��

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
        void CheckBack();                                                  //����Ƿ���Ҫ����
        long GetFileSize(char *sFullPathFileName);                         //��ȡ�ļ��Ĵ�С����λ���ֽ�
        void CheckAndWriteToFile(int &iBuffPos,int &iFilePos,char* sdest,char*ssrc,char* sFirst,int iLen,FILE *fp);
        void WriteToFile(int& iFileBuffPos,char* sFileBuff,FILE *fp);
        bool CheckAndBackup(char* sFileName,int &iFilePos,FILE *&fp,char* sOldTime);
        long long GetLsn();	
    private:
        long m_iNumber;       //����,������¼update�ļ������
        int  m_iLogTime;      //�趨��־ʱ�䣬��λ��
        int  m_iLogSize;      //�趨LOG�ļ����Ĵ�С����λΪM����������˴�С���򱸷��ļ�
        int m_iCheckCounts;
        char m_sLogPath[MAX_PATH_NAME_LEN]; //��־Ŀ¼
        int  m_iPID;          //����PID
        char m_sDSN[64];      //DSN����
        TMdbQueue *m_pQueueCtrl;//�ڴ滺�������
        char* m_pszRecord;
        long long m_iLsn;


        FILE* m_fp;            //�ļ����
        char m_pszFileName[MAX_PATH_NAME_LEN];
        char m_sOldTime[MAX_TIME_LEN];
        char* m_pszMyFileTemp;  //�ļ�����
        int  m_iMyFilePos;     //�ļ�λ��
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
