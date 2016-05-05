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

        //�����߳���
        class TMdbNtcWorkThread : public TMdbNtcThread
        {
        public:
            TMdbNtcWorkThread(bool bNeedLock);
            virtual ~TMdbNtcWorkThread();
            virtual int Execute(void);//�߳̾���ִ�к���
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
            TMdbNtcWorkThread     *m_pTWorkThread;//�̶߳���ָ��
            MDB_NTC_THREAD_STATE    m_thread_state;//�߳�״̬
        };

        class TMdbNtcThreadStateInfo
        {
        public:
            TMdbNtcThreadStateInfo();
        public:
            TMdbNtcThreadLock     m_thread_lock;//�߳���
            unsigned int    m_uiSumThreadCount;//���߳���
            unsigned int    m_uiRunThreadCount;//�����߳���
            TMdbNtcAutoArray      m_TAutoArray;//���ȫ�������߳���Ϣ
        };

        //��Դ�����߳���
        class TMdbNtcResourceManagerThread : public TMdbNtcThread
        {
        public:
            virtual ~TMdbNtcResourceManagerThread();
            virtual int Execute(void);//�߳̾���ִ�к���
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

