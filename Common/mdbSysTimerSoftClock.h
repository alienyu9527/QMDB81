#ifndef _MDB_SYS_TIMER_SOFT_CLOCK_H_
#define _MDB_SYS_TIMER_SOFT_CLOCK_H_
#include "Common/mdbCommons.h"
#include "Common/mdbComponent.h"
#include "Common/mdbSysThreads.h"
#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include <sys/stat.h>

//namespace  QuickMDB
//{
        class TMdbNtcTimerQuery;
        class TMdbNtcSoftClockThread;

        //��ʱ��������ģ��
        //ʱ����Ϣ��
        class TMdbNtcClockInfo : public TMdbNtcBaseObject {
        public:
            unsigned short int  Year;        // 1994-2099
            unsigned char       Month;       // 1-12
            unsigned char       Day;         // 1-31

            unsigned char       Hour;        // 0-23
            unsigned char       Minute;      // 0-59
            unsigned char       Second;      // 0-59

            unsigned char       Count10ms;   // 0-99
        public:
            MDB_UINT64 GetSecond(void);
            void SetNowClock(void);
            void SetClockByDateTimeStr(const char *AClockStr);
            void SetClockByLongDateTimeStr(const char *AClockStr);
        };


        //ʱ����Ϣ��
        enum TMdbNtcTimerTypes
        {
            MDB_NTC_TIMER_TYPE_ABSOLUTE = 0,
            MDB_NTC_TIMER_TYPE_RELATIVE = 1
        };

        enum TMdbNtcDelTimerTypes
        {
            MDB_NTC_DEL_TIMER_BY_NAME = 0,
            MDB_NTC_DEL_TIMER_BY_ID   = 1
        };
       /*
        * @brief ��ʱ����Ϣ��
        *
        */
        class TMdbNtcTimerInfo : public TMdbNtcBaseObject {
        public:
            /*
             * @brief ���캯��
             *
             */
            TMdbNtcTimerInfo();
            /*
             * @brief ��������
             *
             */
            virtual ~TMdbNtcTimerInfo();
            /*
             * @brief ��ӡ��ʱ����Ϣ
             *
             * @return TMdbNtcStringBuffer
             * @retval ��ʱ����Ϣ�Ķ���
             */
            TMdbNtcStringBuffer ToString(void);
            /*
             * @brief ��ȡʱ�Ӷ��������е�λ��
             *
             * @return unsigned short int
             * @retval λ��
             */
            unsigned short int GetQuery10s(void);
            /*
             * @brief ��ȡʱ�Ӷ��������е�λ��
             *
             * @return unsigned short int
             * @retval λ��
             */
            unsigned short int GetQuery100s(void);
        public:
            TMdbNtcTimerTypes TimerType;//��ʱ������
            char Name[40];        //��ʱ������
            MDB_INT64 Count1ms;       //ʱ��ĺ�����
            MDB_INT64 Reset;          //��λ������������ʱ�Ӷ���ʱ���ø�ֵ
            bool LoopFlag;        //�Ƿ�ѭ��ʹ��

            OnMdbEventFunc OnTimerEvent;//������ں���
            void        *Param;      //������ں����Ĳ���
            TMdbNtcTimerQuery *Query1s;    //ʱ�Ӷ���ָ�룬��λ�� 1s~9s,����<1s����>999s
            TMdbNtcTimerQuery *Query10s;   //ʱ�Ӷ���ָ�룬ʮλ�� 10s~90s
            TMdbNtcTimerQuery *Query100s;  //ʱ�Ӷ���ָ�룬��λ�� 100s~900s
            TMdbNtcTimerInfo  *Next;       //ʱ�Ӷ�������һ����ʱ��
            unsigned int m_uiTimerId;//��ʱ��id
        };

        //ʱ���ж���
        #define MDB_NTC_TIMER_QUERY_SIZE 31
        class TMdbNtcTimerQuery : public TMdbNtcComponentObject {
            friend class TMdbNtcSoftClockThread;
            friend class TMdbNtcTimerManager;
        public:
            TMdbNtcTimerQuery(unsigned int ATimerIndex);
            unsigned int GetCount(void);
            unsigned short int GetIndex(void);
            TMdbNtcStringBuffer ToString(void);
        private:
            bool SubCount100ms( );
            void AddTimerInfo(TMdbNtcTimerInfo *ATimerInfo, bool bLockFlag);//����>=1�벢��<1000�����Զ�ʱ����
            void AddTimerInfoEx(TMdbNtcTimerInfo *ATimerInfo, bool bLockFlag);//���Ӷ�ʱ����ָ��ʱ�Ӷ�����(1�����Զ�ʱ����2��<1��>=1000����Զ�ʱ��)��
            bool DelTimerInfo(TMdbNtcDelTimerTypes DelTimerTypes,const char *ATimerFlag,void **ppFuncParam);
        private:
            unsigned short int FIndex;
            MDB_INT64 FReset;
            unsigned int FCount;//��ʾÿ��ʱ�Ӷ����д��ڵ�δ���ڵĶ�ʱ������
            unsigned int FCount1ms;//��ʾ��ʱ�Ӷ��еĶ�ʱ���������1000��ʾ1�룬10000��ʾ10�룩
            TMdbNtcTimerInfo *FTimerHead;
        };

        //��ʱ���߳���
        class TMdbNtcSoftClockThread : public TMdbNtcThread {
        public:
            virtual ~TMdbNtcSoftClockThread();
            virtual int Execute(void);//�߳̾���ִ�к���
            static TMdbNtcSoftClockThread* GetInstancePtr( bool bOnlyGetFlag );
        private:
            TMdbNtcSoftClockThread();
            void BeginCounter(void);
            MDB_INT64 EndCounter(void);
            void TimerProcess(void);
            void ShowTimerInfo(void);
        private:
            static TMdbNtcSoftClockThread *_instance;
            MDB_INT64 FCount1s;//�����Զ�ʱ��ʹ��
            int FStandardCount;
/*
#ifdef OS_WINDOWS
            unsigned long FBeginTp;
            unsigned long FEndTp;
#else
            struct timeval FBeginTp;
            struct timeval FEndTp;
#endif
*/
            TMdbNtcAccurateTime FBeginTp;
            TMdbNtcAccurateTime FEndTp;

        };

//}

#endif
