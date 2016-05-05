/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbAutoUnlockThread.h		
*@Description： 自动解锁进程
*@Author:			jin.shaohua
*@Date：	    2013.4
*@History:
******************************************************************************************/
#ifndef _MDB_AUTO_UNLOCK_THREAD_H_
#define _MDB_AUTO_UNLOCK_THREAD_H_
#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{
    //mdb自动解锁进程
    class TMdbAutoUnlockThread
    {
    public:
        TMdbAutoUnlockThread();
        ~TMdbAutoUnlockThread();
        void Init(TMdbShmDSN* pShmDSN);
        int Start();
        void Stop();  //停止处理
        bool IsRun()
        {
            return m_bIsRun;
        }
    private:
        int svc();
        static void* agent(void* p);
        void ReleaseMutex();//释放超时锁
        int DiffMSecond(struct timeval tTV1, struct timeval tTV2);
        int CheckAndRelease(TMutex & tMutex,struct timeval tNow, int iTimeOutMS,bool bRelaseNow);//检测并释放锁
    private:
        bool  m_bRunFlag;
        bool m_bIsRun;
        TMdbShmDSN* m_pShmDSN;
        TMdbDSN    *m_pDsn;
        time_t m_tLastTime;
    };
//}

#endif

