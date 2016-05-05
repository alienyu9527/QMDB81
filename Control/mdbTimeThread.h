/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbTimeThread.h
*@Description�� �������ʱ����Ϣ
*@Author:		li.shugang
*@Date��	    2009��06��12��
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_TIME_THREAD_H__
#define __ZX_MINI_DATABASE_TIME_THREAD_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"
#include "Helper/TThreadLog.h"

#ifdef WIN32
#include <windows.h>
#endif

//namespace QuickMDB{

    //����
    class TMdbTimeThread
    {
    public:
        TMdbTimeThread();
        ~TMdbTimeThread();
        void Init(TMdbShmDSN* pShmDSN);
        int Start();//��ʼ��ʱ
        void Stop(); //ֹͣ����
        bool IsRun()
        {
            return m_bIsRun;
        }

    private:
        int svc();
        static void* agent(void* p);
        int CheckDiskSpace();//���ʣ����̿ռ�
        int CollectTableOperInfo();//��ȡ��Ĳ�����Ϣ
    	void PrintAllSeqCurValue();//��ӡ�������е���ϸ��Ϣ
    private:
        bool  m_bRunFlag;
        bool m_bIsRun;
        TMdbShmDSN* m_pShmDSN;
        TMdbDSN    *m_pDsn;
        int m_iFlag;
    };
//}


#endif //__ZX_MINI_DATABASE_TIME_THREAD_H__

