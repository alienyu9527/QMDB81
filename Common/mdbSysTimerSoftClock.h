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

        //定时任务推送模块
        //时间信息类
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


        //时钟信息类
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
        * @brief 定时器信息类
        *
        */
        class TMdbNtcTimerInfo : public TMdbNtcBaseObject {
        public:
            /*
             * @brief 构造函数
             *
             */
            TMdbNtcTimerInfo();
            /*
             * @brief 析构函数
             *
             */
            virtual ~TMdbNtcTimerInfo();
            /*
             * @brief 打印定时器信息
             *
             * @return TMdbNtcStringBuffer
             * @retval 定时器信息的对象
             */
            TMdbNtcStringBuffer ToString(void);
            /*
             * @brief 获取时钟队列数组中的位置
             *
             * @return unsigned short int
             * @retval 位置
             */
            unsigned short int GetQuery10s(void);
            /*
             * @brief 获取时钟队列数组中的位置
             *
             * @return unsigned short int
             * @retval 位置
             */
            unsigned short int GetQuery100s(void);
        public:
            TMdbNtcTimerTypes TimerType;//定时器类型
            char Name[40];        //定时器名称
            MDB_INT64 Count1ms;       //时间的毫秒数
            MDB_INT64 Reset;          //恢位毫秒数，加入时钟队列时设置该值
            bool LoopFlag;        //是否循环使用

            OnMdbEventFunc OnTimerEvent;//任务入口函数
            void        *Param;      //任务入口函数的参数
            TMdbNtcTimerQuery *Query1s;    //时钟队列指针，个位级 1s~9s,或者<1s或者>999s
            TMdbNtcTimerQuery *Query10s;   //时钟队列指针，十位级 10s~90s
            TMdbNtcTimerQuery *Query100s;  //时钟队列指针，百位级 100s~900s
            TMdbNtcTimerInfo  *Next;       //时钟队列中下一个定时器
            unsigned int m_uiTimerId;//定时器id
        };

        //时钟列队类
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
            void AddTimerInfo(TMdbNtcTimerInfo *ATimerInfo, bool bLockFlag);//增加>=1秒并且<1000秒的相对定时器。
            void AddTimerInfoEx(TMdbNtcTimerInfo *ATimerInfo, bool bLockFlag);//增加定时器到指定时钟队列里(1、绝对定时器，2、<1或>=1000的相对定时器)。
            bool DelTimerInfo(TMdbNtcDelTimerTypes DelTimerTypes,const char *ATimerFlag,void **ppFuncParam);
        private:
            unsigned short int FIndex;
            MDB_INT64 FReset;
            unsigned int FCount;//表示每个时钟队列中存在的未到期的定时器个数
            unsigned int FCount1ms;//表示该时钟队列的定时间隔（比如1000表示1秒，10000表示10秒）
            TMdbNtcTimerInfo *FTimerHead;
        };

        //软时钟线程类
        class TMdbNtcSoftClockThread : public TMdbNtcThread {
        public:
            virtual ~TMdbNtcSoftClockThread();
            virtual int Execute(void);//线程具体执行函数
            static TMdbNtcSoftClockThread* GetInstancePtr( bool bOnlyGetFlag );
        private:
            TMdbNtcSoftClockThread();
            void BeginCounter(void);
            MDB_INT64 EndCounter(void);
            void TimerProcess(void);
            void ShowTimerInfo(void);
        private:
            static TMdbNtcSoftClockThread *_instance;
            MDB_INT64 FCount1s;//给绝对定时器使用
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
