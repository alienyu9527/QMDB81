/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbAutoUnlockThread.h		
*@Description�� �Զ���������
*@Author:			jin.shaohua
*@Date��	    2013.4
*@History:
******************************************************************************************/
#ifndef _MDB_AUTO_UNLOCK_THREAD_H_
#define _MDB_AUTO_UNLOCK_THREAD_H_
#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{
    //mdb�Զ���������
    class TMdbAutoUnlockThread
    {
    public:
        TMdbAutoUnlockThread();
        ~TMdbAutoUnlockThread();
        void Init(TMdbShmDSN* pShmDSN);
        int Start();
        void Stop();  //ֹͣ����
        bool IsRun()
        {
            return m_bIsRun;
        }
    private:
        int svc();
        static void* agent(void* p);
        void ReleaseMutex();//�ͷų�ʱ��
        int DiffMSecond(struct timeval tTV1, struct timeval tTV2);
        int CheckAndRelease(TMutex & tMutex,struct timeval tNow, int iTimeOutMS,bool bRelaseNow);//��Ⲣ�ͷ���
    private:
        bool  m_bRunFlag;
        bool m_bIsRun;
        TMdbShmDSN* m_pShmDSN;
        TMdbDSN    *m_pDsn;
        time_t m_tLastTime;
    };
//}

#endif

