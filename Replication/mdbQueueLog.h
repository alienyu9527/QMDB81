/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbQueueLog.h		
*@Description：Queue 中数据落地日志
*@Author:	   jin.shaohua
*@Date：	    2013.4
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

    //内存队列数据落地
    class TMdbQueueLog
    {
    public:
        TMdbQueueLog();
        ~TMdbQueueLog();
        int Init(const char * sDsn);
        int Start();
    private:
        int WriteToFile();//写入文件
        int CheckBackupFile();//检查各同步文件是否需要备份
        int GetSyncType(char* sData);
    private:
        TMdbProcCtrl m_tProcCtrl;//进程管理
        TMdbQueue m_tMdbQueueCtrl; //内存缓冲管理器
        TMdbOnlineRepQueue m_tMdbOnlineRepQueueCtrl[MAX_ALL_REP_HOST_COUNT]; //内存缓冲管理器
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

