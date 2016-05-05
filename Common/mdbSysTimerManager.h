/**
 * @file SysTimerManager.hxx
 * 定时器相关
 * 
 * @author jiang.jinzhou
 * @version 1.0
 * @date 2013/12/20
 * @bug 新建，无bug
 * @bug 单元测试，未发现bug
 * @warning 
 */
#ifndef _MDB_SYS_TIMER_MANAGER_H_
#define _MDB_SYS_TIMER_MANAGER_H_

#include "Common/mdbCommons.h"
#include "Common/mdbComponent.h"

#include <iostream>
#include <string>
#include <sstream>
#include <time.h>
#include <sys/stat.h>

//namespace  QuickMDB
//{
        /**
         * @brief 定时器管理类
         * 
         */
        class TMdbNtcTimerManager : public TMdbNtcComponentObject
        {
        public:
            /**
              * @brief 初始化
              * @param bDynamic [in] 是否需要动态伸缩
              * @retval  true成功/false失败
              */            
            static bool Initialize( bool bDynamic = false );

            /**
              * @brief 新增相对定时器
              * @param ATimerName [in] 定时器名称
              * @param ALoopFlag [in] 是否循环定时器
              * @param ACount100ms [in] 定时任务执行间隔(单位：百毫秒)
              * @param AEventFunc [in] 定时任务函数指针
              * @param pFuncParam [in] 定时任务函数参数
              * @retval  >0成功/0失败
              */
            static unsigned int AddRelativeTimer(const char *ATimerName,bool ALoopFlag,unsigned int ACount100ms,OnMdbEventFunc AEventFunc,void *pFuncParam);

            /**
              * @brief 新增绝对定时器
              * @param ATimerName [in] 定时器名称
              * @param ATimerStr [in] 定时任务执行时间
              * @param AEventFunc [in] 定时任务函数指针
              * @param pFuncParam [in] 定时任务函数参数
              * @retval >0成功/0失败
              */
            static unsigned int AddAbsTimer(const char *ATimerName,const char *ATimerStr,OnMdbEventFunc AEventFunc,void *pFuncParam);

            /**
              * @brief 删除定时器
              * @param ATimerName [in] 定时器名称
              * @param ppFuncParam [out] 定时器参数
              * @retval true成功/false失败
              */
            static bool DelTimerByName(const char *ATimerName,void **ppFuncParam = NULL);

            /**
              * @brief 删除定时器
              * @param uiTimerId [in] 定时器序号
              * @param ppFuncParam [out] 定时器参数
              * @retval true成功/false失败
              */
            static bool DelTimerById(const unsigned int uiTimerId,void **ppFuncParam = NULL);
            
            /**
              * @brief 删除与定时器相关的所有线程
            */            
            static void KillAllThr();
        public:
            static unsigned int TimerId;
            static bool bHasInitialize;
        };

//}

#endif
