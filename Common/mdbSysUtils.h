/**
* @file SysUtils.hxx
* @brief IPC���õ����Լ����࣬ϵͳ��ص����뺯��
*
* IPC���õ����Լ����࣬ϵͳ��ص����뺯��
*
* @author Ge.zhengyi
* @version 1.0
* @date 20121214
* @warning ��̳и���������IPC����
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
                typedef DWORD    pid_t;///< pid����
        #endif
        /**
         * @brief ��ϵͳ���
         * 
         */
        class TMdbNtcSysUtils
        {
        public:
            /**
             * @brief ��õ�ǰ�û�id
             * 
             * @return MDB_UINT32
             */
            static MDB_UINT32 GetUserId(void);
            /**
             * @brief ��õ�ǰ�û�����
             * 
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetUserName();
            /**
             * @brief ����pid�õ����̵�������
             * 
             * @param pid [in] ���̵�pid
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetCmdLine(pid_t pid);
            /**
             * @brief �Ƚ������������Ƿ�һ�£��������ıȽϻ����·����
             * 
             * @param pszCmdLine1 [in] ������1
             * @param pszCmdLine1 [in] ������2
             * @return bool
             * @retval true һ��
             */
            static bool CompareCmdLine(const char* pszCmdLine1, const char* pszCmdLine2);
            /**
             * @brief ���������л�ȡ��������
             * 
             * @param pszCmdLine [in] ������
             * @return TMdbNtcStringBuffer
             * @retval �õ��Ľ�����
             */
            static TMdbNtcStringBuffer GetProcName(const char* pszCmdLine);
            /**
             * @brief ����pid�õ�������
             * 
             * @param pid [in] ���̵�pid
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetProcName(pid_t pid);
            /**
             * @brief ����ܵ��ڴ��С
             * 
             * @return MDB_UINT64
             */
            static MDB_UINT64 GetMemoryTotalSize();
            /**
             * @brief ��ÿ����ڴ��С
             * 
             * @return MDB_UINT64
             */
            static MDB_UINT64 GetMemoryFreeSize();
            /**
             * @brief ���cpu����Ŀ
             * 
             * @return MDB_UINT32
             */
            static MDB_UINT32 GetCpuNum();
            /**
             * @brief ��ȡ����ռ��cpu�İٷֱ�
             * 
             * @param pid       [in]    ���̵�id
             * @param dPercent [out]   ����ռ��cpu�İٷֱ�
             * @return bool
             * @retval true �ɹ�
             */
            static bool GetProcCpuPercent(pid_t pid, double& dPercent);
            /**
             * @brief ��ȡ����ռ���ڴ�İٷֱ�
             * 
             * @param pid       [in]    ���̵�id
             * @param dPercent [out]   ����ռ���ڴ�İٷֱ�
             * @return bool
             * @retval true �ɹ�
             */
            static bool GetProcMemPercent(pid_t pid, double& dPercent);
            /**
             * @brief ��ȡ����ռ���ڴ�Ĵ�С
             * 
             * @param pid       [in]    ���̵�id
             * @param uiBytes   [out]   ����ռ���ڴ�Ĵ�С
             * @return bool
             * @retval true �ɹ�
             */
            static bool GetProcMemUsage(pid_t pid, MDB_UINT32& uiBytes);
            /**
             * @brief ���ݽ��̵������еõ����̵�pid���ҿ��Խ���û�������
             * 
             * @param pszCmdLine [in] ���̵�������
             * @param pszUserName [in] �û������ƣ������ΪNULL����ͬʱ��Ϊ��������
             * @return pid_t
             * @retval >0 ����pid
             */
            static pid_t SearchPidByCmdLine(const char* pszCmdLine, const char* pszUserName = NULL);
            /**
             * @brief ���ݽ��̵����Ƶõ����̵�pid���ҿ��Խ���û�������
             * 
             * @param pszProcName [in] ���̵�����
             * @param pszUserName [in] �û������ƣ������ΪNULL����ͬʱ��Ϊ��������
             * @return pid_t
             * @retval >0 ����pid
             */
            static pid_t SearchPidByName(const char* pszProcName, const char* pszUserName = NULL);

            /**
             * @brief �������Ƿ����
             * 
             * @param pid [in] ���̵�id
             * @param bEnumProcesses [in] �Ƿ�ö�����н��̣������ڶԿ��û����̻�ϵͳ��ȫ���̵��ж�
             * @return bool
             * @retval true ����
             */
            static bool IsProcessExist(pid_t pid, bool bEnumProcesses = false);
            /**
             * @brief ����ָ�����̣�����SIGINT�ź�
             * 
             * @param pid [in] ���̵�id
             */
            static void TermProcess(pid_t pid);
            /**
             * @brief ǿ�ƽ���ָ�����̣�����SIGKILL�ź�
             * 
             * @param pid [in] ���̵�id
             */
            static void KillProcess(pid_t pid);
            /**
             * @brief ���������
             * 
             * @param sHostName [out] ��õ�������
             * @return bool
             * @retval true �ɹ�
             */
            static bool GetHostName(TMdbNtcStringBuffer& sHostName);
            /**
             * @brief ��ñ�����ip
             * 
             * @param sIPList [out] ����ж��ip����ʹ��;�ָ�
             * @return int
             * @retval -1 ʧ��
             * @retval >=0 ip����Ŀ
             */
            static int GetHostIP(TMdbNtcStringBuffer& sIPList);
            /**
             * @brief �ó����̨����
             * 
             * @return bool
             * @retval true �ɹ�
             */
            static bool RunBackground();
            /**
             * @brief ���������ڴ�γߴ磨�ֽڣ�
             * 
             * @return MDB_UINT64
             */
            static MDB_UINT64 GetShmMax();
            /**
             * @brief ���ÿ����������ڴ������
             * 
             * @return MDB_UINT64
             */
            static MDB_UINT64 GetShmSeg();
            /**
             * @brief ִ��������õ���������
             * 
             * @param pszCmd [in] ����
             * @param sOutput [out] �������Ϣ
             * @return bool
             * @retval true �ɹ�
             */
            static bool GetCmdOutput(const char* pszCmd, TMdbNtcStringBuffer& sOutput);
        };
        /*
        * @brief ϵͳ�����������
        *
        * ϵͳ������������װ
        */
        class TMdbNtcSystemNameObject : public TMdbNtcComponentObject
        {
            friend class TMdbNtcSysNameObjectHelper;
            MDB_ZF_DECLARE_OBJECT(TMdbNtcSystemNameObject);
        public:
           /**
            * @brief ���캯��
            * 
            * @param pszName [in] �������������
            * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
            * @param pParentNameObject [in] ����������������
            *
            */
            TMdbNtcSystemNameObject(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject*  pParentNameObject = NULL);

           /**
            * @brief ��������
            *
            * �����������
            */
            virtual ~TMdbNtcSystemNameObject();

            /**
            * @brief �õ�������ʾ
            *
            * �õ�������ʾ
            *
            * @return bool
            * @retval true  ����
            * @retval false ����
            */
            bool IsCreated(void) const;

            /**
            * @brief �õ��������
            *
            * �õ��������
            *
            * @return int
            * @retval �������
            */
            int GetErrorNo(void);

            /**
            * @brief �õ�������Ϣ
            *
            * �õ�������Ϣ
            *
            * @return TMdbNtcStringBuffer
            * @retval ������Ϣ
            */
            TMdbNtcStringBuffer GetErrorText(void);
            /**
            * @brief �õ����ӱ�ʾ
            *
            * �õ����ӱ�ʾ
            *
            * @return bool
            * @retval true  ������
            * @retval false δ����
            */
            virtual bool IsOK(void) const;
            /**
             * @brief ����������������
             * 
             * @return const char*
             */
            inline const char* GetName(void) const
            {
                return m_sName.c_str();
            }
            /**
             * @brief ���IPCʵ����ʹ�õĻ�����������
             * 
             * @return const char*
             */
            inline const char* GetInstEnvName()
            {
                return m_sInstEnvName.c_str();
            }
            /**
             * @brief �����������Դ
             * 
             * @return bool
             * @retval true �ɹ�
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
            bool                m_bSelfCreated;     ///< �Ƿ��Լ�����(true��ʾ����/false��ʾ����)
            int                 m_iErrorNo;         ///< �������
            TMdbNtcStringBuffer       m_sErrorText;       ///< ������Ϣ
            bool                m_bIsOK;            ///< �Ƿ�����(true��ʾ������/false��ʾδ����)
            
            TMdbNtcSystemNameObject*  m_pParentNameObject;///< ����������������
            TMdbNtcStringBuffer       m_sInstEnvName;///< IPCʵ����ʹ�õĻ�����������
            TMdbNtcStringBuffer       m_sIpcProcFilePath; ///< ��ʹ�õ�ipc������Ϣ·��
            TMdbNtcStringBuffer       m_sNameObjectProcFilePath; ///< ��ʹ�õ��������������Ϣ·��
        protected:
            TMdbNtcStringBuffer       m_sName;///< IPC����
        };

        /**
         * @brief �����������֣����Ը�����¼�����������Ϣ���Լ���ʹ�õ���Դ
         * 
         */
        class TMdbNtcSysNameObjectHelper
        {
        public:
            /**
             * @brief �������ipc��Դ��·��
             * 
             * @param pRuntimeObject [in] ������������ͣ���ZF_RUNTIME_OBJECT(TMdbNtcShareMem)��
             * @param pszName [in] �������������                          
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetIpcPath(const TMdbRuntimeObject* pRuntimeObject, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief �������ipc��Դ��·��
             * 
             * @param pNameObject [in] ��������
             * @return TMdbNtcStringBuffer
             */
            inline static TMdbNtcStringBuffer GetIpcPath(TMdbNtcSystemNameObject* pNameObject)
            {
                return GetIpcPath(pNameObject->GetRuntimeObject(), pNameObject->GetName(), pNameObject->GetInstEnvName());
            }
            /**
             * @brief ����������������·��
             * 
             * @param pRuntimeObject [in] ������������ͣ���ZF_RUNTIME_OBJECT(TMdbNtcShareMem)��
             * @param pszName [in] �������������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetNameObjectPath(const TMdbRuntimeObject* pRuntimeObject, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ����������������·��
             * 
             * @param pNameObject [in] ��������
             * @return TMdbNtcStringBuffer
             */
            inline static TMdbNtcStringBuffer GetNameObjectPath(TMdbNtcSystemNameObject* pNameObject)
            {
                return GetNameObjectPath(pNameObject->GetRuntimeObject(), pNameObject->GetName(), pNameObject->GetInstEnvName());
            }
            /**
             * @brief ָ������������ע���¼
             * 
             * @param pNameObject [in] ��������
             * @return bool
             * @retval true �ɹ�
             */
            static bool SignIn(TMdbNtcSystemNameObject* pNameObject);
            /**
             * @brief ָ������������ע����¼
             * 
             * @param pNameObject [in] ��������
             * @return bool
             * @retval true �ɹ�
             */
            static bool SignOut(TMdbNtcSystemNameObject* pNameObject);
            /**
             * @brief ָ�������������ͷŵ���Դ
             * 
             * @param pNameObject [in] ��������
             * @return bool
             * @retval true �ɹ�
             */
            static bool Free(TMdbNtcSystemNameObject* pNameObject);
            /**
             * @brief ָ�������������ͷŵ���Դ
             * 
             * @param pRuntimeObject [in] ������������ͣ���ZF_RUNTIME_OBJECT(TMdbNtcShareMem)��
             * @param pszName [in] �������������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true �ɹ�
             */
            static bool Free(const TMdbRuntimeObject* pRuntimeObject, const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        protected:
            static bool GenIpcProcFilePath(TMdbNtcSystemNameObject* pNameObject);
            static bool GenNameObjectProcFilePath(TMdbNtcSystemNameObject* pNameObject);
        };
//}
#endif
