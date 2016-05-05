/**
 * @file Sdk/mdbComponent.h
 * @brief ���������
 *
 * �������������
 *
 * @author �������С��
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
         * @brief BillingSDKȫ�ֲ����࣬�ṩSDK���е�ȫ�ֲ���
         * 
         */

        class TMdbNtcParameter
        {
        public:
            /**
             * @brief ���ϵͳ����־·���������Ŀ¼�����ھʹ�����
             * @return TMdbNtcStringBuffer
             */
            //static TMdbNtcStringBuffer GetLogPath(void);
            /**
             * @brief ���ϵͳ������ͳ��·���������Ŀ¼�����ھʹ�����
             * @return TMdbNtcStringBuffer
             */
            //static TMdbNtcStringBuffer GetPerfStatPath(void);
            /**
             * @brief ���ϵͳ��ipc��Դ·��(IPCʵ����ʹ�õĻ�����������/sys/ipcĿ¼)
             * 
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetIPCKeyPath(const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ���ϵͳ����������·��(IPCʵ����ʹ�õĻ�����������/sys/snoĿ¼)
             * 
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
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

