/**
 * @file Sdk/mdbComponent.h
 * @brief 组件基础类
 *
 * 公用组件基础类
 *
 * @author 技术框架小组
 * @version 1.0
 * @date 20121214
 * @warning
 */

#ifndef _MDB_H_Component_
#define _MDB_H_Component_

#include "Common/mdbCommons.h"
#include "Common/mdbBaseObject.h"

//namespace QuickMDB
//{
        //class TMdbNtcBaseLog;
        /**
         * @brief BillingSDK全局参数类，提供SDK所有的全局参数
         * 
         */

        class TMdbNtcParameter
        {
        public:
            /**
             * @brief 获得系统的日志路径（如果该目录不存在就创建）
             * @return TMdbNtcStringBuffer
             */
            //static TMdbNtcStringBuffer GetLogPath(void);
            /**
             * @brief 获得系统的性能统计路径（如果该目录不存在就创建）
             * @return TMdbNtcStringBuffer
             */
            //static TMdbNtcStringBuffer GetPerfStatPath(void);
            /**
             * @brief 获得系统的ipc资源路径(IPC实例所使用的环境变量名称/sys/ipc目录)
             * 
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetIPCKeyPath(const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 获得系统的命名对象路径(IPC实例所使用的环境变量名称/sys/sno目录)
             * 
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetNameObjectPath(const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        };

        class TMdbNtcComponentObject : public TMdbNtcBaseObject 
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcComponentObject);
        public:
            TMdbNtcComponentObject(void);
            //void SetLogger(TMdbNtcBaseLog *pLogger);
            //TMdbNtcBaseLog* GetLogger() const;
        protected:
            //TMdbNtcBaseLog *m_pLogger;
            unsigned int m_iLogCode;
        };
/*
        class TSystemObject : public TMdbNtcBaseObject 
        {
            MDB_ZF_DECLARE_OBJECT(TSystemObject);
        public:

        };
*/
//}
#endif

