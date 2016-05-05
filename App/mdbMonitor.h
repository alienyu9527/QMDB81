/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbMonitor.h
*@Description��mdb���ģ��
*@Author:		jin.shaohua
*@Date��	    2012.10
*@History:
******************************************************************************************/
#ifndef _MDB_MONITOR_H_
#define _MDB_MONITOR_H_
#include "Helper/mdbStruct.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbProcCtrl.h"

//namespace QuickMDB{


//mdb���ģ��
class TMdbMonitor
{
public:
    TMdbMonitor();
    ~TMdbMonitor();
    int Init(const char * sDsn);//��ʼ��
    int Start();//��ʼ���
private:
    int SpaceCtrl();//���Ƶ�����ռ�Ĵ�С
    //int FlushSequence();//ˢ������,ͬ����oracle
    bool IsDBStop();
    int WritePerformanceToFile();
    void WriteSession(FILE *fp);
    void WriteProcess(FILE *fp);
    void WriteMemory(FILE *fp);
    void WriteTableSpace(FILE *fp);
    //��������Oracle
   // int ReConnectOracle();
private:
    TMdbShmDSN*         m_pShmDSN;
    TMdbDatabase *		m_pMdbLink; //mdb ����
    TMdbQuery *		m_pQueryMem;
    TMdbQuery *		m_pQuerySession;
    
    TMdbQuery *		m_pQueryTS;
    TMdbDSN	* 		m_pDsn;
    TMdbProcCtrl         m_tProcCtrl;//���̿���
};


//}
#endif

