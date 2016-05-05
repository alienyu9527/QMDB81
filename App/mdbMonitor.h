/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbMonitor.h
*@Description：mdb监控模块
*@Author:		jin.shaohua
*@Date：	    2012.10
*@History:
******************************************************************************************/
#ifndef _MDB_MONITOR_H_
#define _MDB_MONITOR_H_
#include "Helper/mdbStruct.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbProcCtrl.h"

//namespace QuickMDB{


//mdb监控模块
class TMdbMonitor
{
public:
    TMdbMonitor();
    ~TMdbMonitor();
    int Init(const char * sDsn);//初始化
    int Start();//开始监控
private:
    int SpaceCtrl();//控制调整表空间的大小
    //int FlushSequence();//刷新序列,同步到oracle
    bool IsDBStop();
    int WritePerformanceToFile();
    void WriteSession(FILE *fp);
    void WriteProcess(FILE *fp);
    void WriteMemory(FILE *fp);
    void WriteTableSpace(FILE *fp);
    //重新连接Oracle
   // int ReConnectOracle();
private:
    TMdbShmDSN*         m_pShmDSN;
    TMdbDatabase *		m_pMdbLink; //mdb 链接
    TMdbQuery *		m_pQueryMem;
    TMdbQuery *		m_pQuerySession;
    
    TMdbQuery *		m_pQueryTS;
    TMdbDSN	* 		m_pDsn;
    TMdbProcCtrl         m_tProcCtrl;//进程控制
};


//}
#endif

