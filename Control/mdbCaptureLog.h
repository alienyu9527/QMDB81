#ifndef _MDB_CAPTURE_LOG_H_
#define _MDB_CAPTURE_LOG_H_

#include "Helper/mdbQueue.h"
#include "Helper/mdbRepRecd.h"

//namespace QuickMDB{

    // ����·�ɲ������ݵ���ش���
    class TMdbCaptureLog
    {
    public:
        TMdbCaptureLog();
        ~TMdbCaptureLog();

        int Init(const char* psDsn, TMdbQueue & mdbQueueCtrl);
        int Log(bool bEmpty = false);

    private:

        //  ת����QDGʹ�õļ�¼��ʽ
        int Adapt();
        int BackUp();
        int WriteToFile();

    private:
        char m_sDsn[MAX_NAME_LEN];
        char m_sLogPath[MAX_PATH_NAME_LEN];
        TMdbQueue *m_pQueueCtrl;//�ڴ滺�������
        TMdbRepRecdDecode* m_pParser;
        std::string m_sTempFile;//��ʱ�ļ���
        std::string m_sOKFile;//OK�ļ���
        FILE * m_pCurFile;//��ǰ��д���ļ�
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
