/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbQueueLog.h		
*@Description��Queue �����������־
*@Author:	   jin.shaohua
*@Date��	    2013.4
*@History:
******************************************************************************************/
#ifndef __MDB_QUEUE_LOG_H_
#define __MDB_QUEUE_LOG_H_
#include "Control/mdbProcCtrl.h"
#include "Control/mdbStorageEngine.h"
#include "Control/mdbCaptureLog.h"
#include "Helper/mdbQueue.h"
#include "Dbflush/mdbDbLog.h"
#include "Replication/mdbRepLog.h"
#include <string>

//namespace QuickMDB{

    //�ڴ�����������
    class TMdbQueueLog
    {
    public:
        TMdbQueueLog();
        ~TMdbQueueLog();
        int Init(const char * sDsn);
        int Start();
    private:
        int WriteToFile();//д���ļ�
        int CheckBackupFile();//����ͬ���ļ��Ƿ���Ҫ����
        int GetSyncType(char* sData);
    private:
        TMdbProcCtrl m_tProcCtrl;//���̹���
        TMdbQueue m_tMdbQueueCtrl; //�ڴ滺�������
        TMdbOnlineRepQueue m_tMdbOnlineRepQueueCtrl[MAX_ALL_REP_HOST_COUNT]; //�ڴ滺�������
        TMdbShmDSN * m_pShmDsn;
        TMdbDSN * m_pDsn;
        TMdbOraLog m_mdbDBlog;
        TRoutingRepLog m_mdbReplog;
		TRoutingRepLogDispatcher m_mdbRepDispatcher;
        TMdbRedoLog m_mdbRedolog;
        TMdbCaptureLog m_tCaptureLog;
        TMdbRedoLogParser m_LogParser;
        TMdbConfig  *m_pConfig;
    };
//}

#endif

