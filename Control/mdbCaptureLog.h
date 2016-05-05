#ifndef _MDB_CAPTURE_LOG_H_
#define _MDB_CAPTURE_LOG_H_

#include "Helper/mdbQueue.h"
#include "Helper/mdbRepRecd.h"

//namespace QuickMDB{

    // 负责路由捕获数据的落地处理
    class TMdbCaptureLog
    {
    public:
        TMdbCaptureLog();
        ~TMdbCaptureLog();

        int Init(const char* psDsn, TMdbQueue & mdbQueueCtrl);
        int Log(bool bEmpty = false);

    private:

        //  转换成QDG使用的记录格式
        int Adapt();
        int BackUp();
        int WriteToFile();

    private:
        char m_sDsn[MAX_NAME_LEN];
        char m_sLogPath[MAX_PATH_NAME_LEN];
        TMdbQueue *m_pQueueCtrl;//内存缓冲管理器
        TMdbRepRecdDecode* m_pParser;
        std::string m_sTempFile;//临时文件名
        std::string m_sOKFile;//OK文件名
        FILE * m_pCurFile;//当前可写的文件
        char* m_pData;

        int m_iCheckCnt;
        int  m_iMaxFileSize;
        int m_iLogTime;

        char m_sTime[MAX_TIME_LEN];
        int m_iRecdLen ;
        TMdbRepRecd m_tParseRecd;
        char m_sRecd[MAX_VALUE_LEN];
    };

//}

#endif
