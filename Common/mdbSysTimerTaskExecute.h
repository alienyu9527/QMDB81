#ifndef _MDB_SYS_TIMER_TASK_EXECUTE_H_
#define _MDB_SYS_TIMER_TASK_EXECUTE_H_

#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include <sys/stat.h>

#include "Common/mdbCommons.h"
#include "Common/mdbComponent.h"
#include "Common/mdbSysThreads.h"

//namespace  QuickMDB
//{

        //void DisplayThreadStateInfo();

        //工作线程类
        class TMdbNtcWorkThread : public TMdbNtcThread
        {
        public:
            TMdbNtcWorkThread(bool bNeedLock);
            virtual ~TMdbNtcWorkThread();
            virtual int Execute(void);//线程具体执行函数
        private:
            int ExecuteTask(void *pTTimerInfo);
            bool m_bNeedLock;
        };

        class TMdbNtcThreadStateDetailInfo : public TMdbNtcBaseObject
        {
        public:
            TMdbNtcThreadStateDetailInfo( TMdbNtcWorkThread *pTWorkThread_In );
            TMdbNtcStringBuffer ToString(void);
        public:
            TMdbNtcWorkThread     *m_pTWorkThread;//线程对象指针
            MDB_NTC_THREAD_STATE    m_thread_state;//线程状态
        };

        class TMdbNtcThreadStateInfo
        {
        public:
            TMdbNtcThreadStateInfo();
        public:
            TMdbNtcThreadLock     m_thread_lock;//线程锁
            unsigned int    m_uiSumThreadCount;//总线程数
            unsigned int    m_uiRunThreadCount;//工作线程数
            TMdbNtcAutoArray      m_TAutoArray;//存放全部工作线程信息
        };

        //资源管理线程类
        class TMdbNtcResourceManagerThread : public TMdbNtcThread
        {
        public:
            virtual ~TMdbNtcResourceManagerThread();
            virtual int Execute(void);//线程具体执行函数
            static TMdbNtcResourceManagerThread* GetInstancePtr( bool bOnlyGetFlag, bool bDynamic );
        private:
            TMdbNtcResourceManagerThread( bool bDynamic );
            int CreateWorkThread(void);
            int DeleteIdleThread(void);
            bool m_bDynamic;
            static TMdbNtcResourceManagerThread* _instance;
        };

//}

#endif

