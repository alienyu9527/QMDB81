/**
* @file SysShareMems.hxx
* @brief 共享内存操作类
*
* 封装了共享内存的常用操作
*
* @author Ge.zhengyi
* @version 1.0
* @date 20121214
* @warning 请统一使用该类进行共享内存的操作
*/

#ifndef _MDB_H_SysShareMems_
#define _MDB_H_SysShareMems_

#include "Common/mdbSysUtils.h"

//namespace QuickMDB
//{
        class TMdbNtcShareMemDataInfo;
        /*
        * @brief 共享内存操作类
        *
        * 实现共享内存各种操作
        */
        class TMdbNtcShareMem : public TMdbNtcSystemNameObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcShareMem);
        public:
            /**
             * @brief 构造函数
             * 
             * @param pszName [in] 共享内存的名称
             * @param ASize [in] 共享内存的大小，如果为0，表示Attach
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @param pParentNameObject [in] 所隶属的命名对象
             * @param bCheckVersion [in] 是否检查共享内存的版本，以免不同版本的共享内存结构差异导致访问异常
             * @return 无
             */
            TMdbNtcShareMem(const char *pszName, unsigned long ASize = 0,
                const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL, bool bCheckVersion = true);
            virtual ~TMdbNtcShareMem();
            bool IsOK() const;
            MDB_UINT64 GetSize() const;
            unsigned char * GetBuffer();
            MDB_INT64 GetShmID();
            bool ManualFree();
            TMdbNtcStringBuffer ToString() const;
            bool IsLastestVersion() const;
        public://提供一些静态方法
            /**
             * @brief 检测共享内存是否存在
             * 
             * @param pszName [in] 共享内存的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 存在
             */
            static bool CheckExist(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 销毁共享内存
             * 
             * @param pszName [in] 共享内存的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 成功
             */
            static bool Destroy(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        private:
#ifdef OS_WINDOWS
            HANDLE m_iHandle;
#else
            int m_iHandle;
            int m_iNameKey;
#endif
            void *m_pFileView;
        };
//}
#endif

