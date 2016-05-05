/**
* @file SysUtils.hxx
* @brief IPC共用的类以及基类，系统相关的类与函数
*
* IPC共用的类以及基类，系统相关的类与函数
*
* @author Ge.zhengyi
* @version 1.0
* @date 20121214
* @warning 请继承该类进行相关IPC操作
*/

#ifndef _MDB_H_SysUtils_
#define _MDB_H_SysUtils_

//#include "Sdk/mdbCommons.h"
#include "Common/mdbStrings.h"
#include "Common/mdbComponent.h"
//#include "Sdk/mdbBaseObject.h"
//namespace QuickMDB
//{
        #ifdef OS_WINDOWS
                typedef DWORD    pid_t;///< pid类型
        #endif
        /**
         * @brief 与系统相关
         * 
         */
        class TMdbNtcSysUtils
        {
        public:
            /**
             * @brief 获得当前用户id
             * 
             * @return MDB_UINT32
             */
            static MDB_UINT32 GetUserId(void);
            /**
             * @brief 获得当前用户名称
             * 
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetUserName();
            /**
             * @brief 根据pid得到进程的命令行
             * 
             * @param pid [in] 进程的pid
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetCmdLine(pid_t pid);
            /**
             * @brief 比较两个命令行是否一致（进程名的比较会忽略路径）
             * 
             * @param pszCmdLine1 [in] 命令行1
             * @param pszCmdLine1 [in] 命令行2
             * @return bool
             * @retval true 一致
             */
            static bool CompareCmdLine(const char* pszCmdLine1, const char* pszCmdLine2);
            /**
             * @brief 从命令行中获取进程名称
             * 
             * @param pszCmdLine [in] 命令行
             * @return TMdbNtcStringBuffer
             * @retval 得到的进程名
             */
            static TMdbNtcStringBuffer GetProcName(const char* pszCmdLine);
            /**
             * @brief 根据pid得到进程名
             * 
             * @param pid [in] 进程的pid
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetProcName(pid_t pid);
            /**
             * @brief 获得总的内存大小
             * 
             * @return MDB_UINT64
             */
            static MDB_UINT64 GetMemoryTotalSize();
            /**
             * @brief 获得空闲内存大小
             * 
             * @return MDB_UINT64
             */
            static MDB_UINT64 GetMemoryFreeSize();
            /**
             * @brief 获得cpu的数目
             * 
             * @return MDB_UINT32
             */
            static MDB_UINT32 GetCpuNum();
            /**
             * @brief 获取进程占用cpu的百分比
             * 
             * @param pid       [in]    进程的id
             * @param dPercent [out]   进程占用cpu的百分比
             * @return bool
             * @retval true 成功
             */
            static bool GetProcCpuPercent(pid_t pid, double& dPercent);
            /**
             * @brief 获取进程占用内存的百分比
             * 
             * @param pid       [in]    进程的id
             * @param dPercent [out]   进程占用内存的百分比
             * @return bool
             * @retval true 成功
             */
            static bool GetProcMemPercent(pid_t pid, double& dPercent);
            /**
             * @brief 获取进程占用内存的大小
             * 
             * @param pid       [in]    进程的id
             * @param uiBytes   [out]   进程占用内存的大小
             * @return bool
             * @retval true 成功
             */
            static bool GetProcMemUsage(pid_t pid, MDB_UINT32& uiBytes);
            /**
             * @brief 根据进程的命令行得到进程的pid，且可以结合用户来过滤
             * 
             * @param pszCmdLine [in] 进程的命令行
             * @param pszUserName [in] 用户的名称，如果不为NULL，则同时作为条件过滤
             * @return pid_t
             * @retval >0 进程pid
             */
            static pid_t SearchPidByCmdLine(const char* pszCmdLine, const char* pszUserName = NULL);
            /**
             * @brief 根据进程的名称得到进程的pid，且可以结合用户来过滤
             * 
             * @param pszProcName [in] 进程的名称
             * @param pszUserName [in] 用户的名称，如果不为NULL，则同时作为条件过滤
             * @return pid_t
             * @retval >0 进程pid
             */
            static pid_t SearchPidByName(const char* pszProcName, const char* pszUserName = NULL);

            /**
             * @brief 检测进程是否存在
             * 
             * @param pid [in] 进程的id
             * @param bEnumProcesses [in] 是否枚举所有进程，适用于对跨用户进程或系统安全进程的判断
             * @return bool
             * @retval true 存在
             */
            static bool IsProcessExist(pid_t pid, bool bEnumProcesses = false);
            /**
             * @brief 结束指定进程，发送SIGINT信号
             * 
             * @param pid [in] 进程的id
             */
            static void TermProcess(pid_t pid);
            /**
             * @brief 强制结束指定进程，发送SIGKILL信号
             * 
             * @param pid [in] 进程的id
             */
            static void KillProcess(pid_t pid);
            /**
             * @brief 获得主机名
             * 
             * @param sHostName [out] 获得的主机名
             * @return bool
             * @retval true 成功
             */
            static bool GetHostName(TMdbNtcStringBuffer& sHostName);
            /**
             * @brief 获得本机的ip
             * 
             * @param sIPList [out] 如果有多个ip，则使用;分割
             * @return int
             * @retval -1 失败
             * @retval >=0 ip的数目
             */
            static int GetHostIP(TMdbNtcStringBuffer& sIPList);
            /**
             * @brief 让程序后台运行
             * 
             * @return bool
             * @retval true 成功
             */
            static bool RunBackground();
            /**
             * @brief 获得最大共享内存段尺寸（字节）
             * 
             * @return MDB_UINT64
             */
            static MDB_UINT64 GetShmMax();
            /**
             * @brief 获得每进程最大共享内存段数量
             * 
             * @return MDB_UINT64
             */
            static MDB_UINT64 GetShmSeg();
            /**
             * @brief 执行命令，并得到命令的输出
             * 
             * @param pszCmd [in] 命令
             * @param sOutput [out] 输出的信息
             * @return bool
             * @retval true 成功
             */
            static bool GetCmdOutput(const char* pszCmd, TMdbNtcStringBuffer& sOutput);
        };
        /*
        * @brief 系统命名对象基类
        *
        * 系统命名对象基类封装
        */
        class TMdbNtcSystemNameObject : public TMdbNtcComponentObject
        {
            friend class TMdbNtcSysNameObjectHelper;
            MDB_ZF_DECLARE_OBJECT(TMdbNtcSystemNameObject);
        public:
           /**
            * @brief 构造函数
            * 
            * @param pszName [in] 命名对象的名称
            * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
            * @param pParentNameObject [in] 所隶属的命名对象
            *
            */
            TMdbNtcSystemNameObject(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);

           /**
            * @brief 析构函数
            *
            * 类的析构函数
            */
            virtual ~TMdbNtcSystemNameObject();

            /**
            * @brief 得到创建标示
            *
            * 得到创建标示
            *
            * @return bool
            * @retval true  创建
            * @retval false 连接
            */
            bool IsCreated(void) const;

            /**
            * @brief 得到错误代码
            *
            * 得到错误代码
            *
            * @return int
            * @retval 错误代码
            */
            int GetErrorNo(void);

            /**
            * @brief 得到错误信息
            *
            * 得到错误信息
            *
            * @return TMdbNtcStringBuffer
            * @retval 错误信息
            */
            TMdbNtcStringBuffer GetErrorText(void);
            /**
            * @brief 得到连接标示
            *
            * 得到连接标示
            *
            * @return bool
            * @retval true  已连接
            * @retval false 未连接
            */
            virtual bool IsOK(void) const;
            /**
             * @brief 获得命名对象的名称
             * 
             * @return const char*
             */
            inline const char* GetName(void) const
            {
                return m_sName.c_str();
            }
            /**
             * @brief 获得IPC实例所使用的环境变量名称
             * 
             * @return const char*
             */
            inline const char* GetInstEnvName()
            {
                return m_sInstEnvName.c_str();
            }
            /**
             * @brief 销毁申请的资源
             * 
             * @return bool
             * @retval true 成功
             */
            virtual bool ManualFree() = 0;
        protected:
#ifndef OS_WINDOWS
            union semun
            {
                int val;
                struct semid_ds *buf;
                ushort *array;
            } semopts;
#endif
        protected:
            void AddSystemNameObject(const int iIPCKey);
            void DeleteSystemNameObject(const int iIPCKey);
        protected:
            bool                m_bSelfCreated;     ///< 是否自己创建(true表示创建/false表示链接)
            int                 m_iErrorNo;         ///< 错误代码
            TMdbNtcStringBuffer       m_sErrorText;       ///< 错误信息
            bool                m_bIsOK;            ///< 是否正常(true标示已连接/false标示未连接)
            
            TMdbNtcSystemNameObject*  m_pParentNameObject;///< 所隶属的命名对象
            TMdbNtcStringBuffer       m_sInstEnvName;///< IPC实例所使用的环境变量名称
            TMdbNtcStringBuffer       m_sIpcProcFilePath; ///< 所使用的ipc进程信息路径
            TMdbNtcStringBuffer       m_sNameObjectProcFilePath; ///< 所使用的命名对象进程信息路径
        protected:
            TMdbNtcStringBuffer       m_sName;///< IPC名称
        };

        /**
         * @brief 命名对象助手，可以辅助记录命名对象的信息，以及所使用的资源
         * 
         */
        class TMdbNtcSysNameObjectHelper
        {
        public:
            /**
             * @brief 获得自身ipc资源的路径
             * 
             * @param pRuntimeObject [in] 命名对象的类型，如ZF_RUNTIME_OBJECT(TMdbNtcShareMem)等
             * @param pszName [in] 命名对象的名称                          
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetIpcPath(const TMdbRuntimeObject* pRuntimeObject, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 获得自身ipc资源的路径
             * 
             * @param pNameObject [in] 命名对象
             * @return TMdbNtcStringBuffer
             */
            inline static TMdbNtcStringBuffer GetIpcPath(TMdbNtcSystemNameObject* pNameObject)
            {
                return GetIpcPath(pNameObject->GetRuntimeObject(), pNameObject->GetName(), pNameObject->GetInstEnvName());
            }
            /**
             * @brief 获得自身命名对象的路径
             * 
             * @param pRuntimeObject [in] 命名对象的类型，如ZF_RUNTIME_OBJECT(TMdbNtcShareMem)等
             * @param pszName [in] 命名对象的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetNameObjectPath(const TMdbRuntimeObject* pRuntimeObject, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief 获得自身命名对象的路径
             * 
             * @param pNameObject [in] 命名对象
             * @return TMdbNtcStringBuffer
             */
            inline static TMdbNtcStringBuffer GetNameObjectPath(TMdbNtcSystemNameObject* pNameObject)
            {
                return GetNameObjectPath(pNameObject->GetRuntimeObject(), pNameObject->GetName(), pNameObject->GetInstEnvName());
            }
            /**
             * @brief 指定的命名对象注册记录
             * 
             * @param pNameObject [in] 命名对象
             * @return bool
             * @retval true 成功
             */
            static bool SignIn(TMdbNtcSystemNameObject* pNameObject);
            /**
             * @brief 指定的命名对象注销记录
             * 
             * @param pNameObject [in] 命名对象
             * @return bool
             * @retval true 成功
             */
            static bool SignOut(TMdbNtcSystemNameObject* pNameObject);
            /**
             * @brief 指定的命名对象释放掉资源
             * 
             * @param pNameObject [in] 命名对象
             * @return bool
             * @retval true 成功
             */
            static bool Free(TMdbNtcSystemNameObject* pNameObject);
            /**
             * @brief 指定的命名对象释放掉资源
             * 
             * @param pRuntimeObject [in] 命名对象的类型，如ZF_RUNTIME_OBJECT(TMdbNtcShareMem)等
             * @param pszName [in] 命名对象的名称
             * @param pszInstEnvName [in] IPC实例所使用的环境变量名称
             * @return bool
             * @retval true 成功
             */
            static bool Free(const TMdbRuntimeObject* pRuntimeObject, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        protected:
            static bool GenIpcProcFilePath(TMdbNtcSystemNameObject* pNameObject);
            static bool GenNameObjectProcFilePath(TMdbNtcSystemNameObject* pNameObject);
        };
//}
#endif
