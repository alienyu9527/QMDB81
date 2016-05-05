/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbTimeThread.h
*@Description： 负责管理时间信息
*@Author:		li.shugang
*@Date：	    2009年06月12日
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

    //定义
    class TMdbTimeThread
    {
    public:
        TMdbTimeThread();
        ~TMdbTimeThread();
        void Init(TMdbShmDSN* pShmDSN);
        int Start();//开始计时
        void Stop(); //停止处理
        bool IsRun()
        {
            return m_bIsRun;
        }

    private:
        int svc();
        static void* agent(void* p);
        int CheckDiskSpace();//检测剩余磁盘空间
        int CollectTableOperInfo();//获取表的操作信息
    	void PrintAllSeqCurValue();//打印所有序列的详细信息
    private:
        bool  m_bRunFlag;
        bool m_bIsRun;
        TMdbShmDSN* m_pShmDSN;
        TMdbDSN    *m_pDsn;
        int m_iFlag;
    };
//}


#endif //__ZX_MINI_DATABASE_TIME_THREAD_H__

