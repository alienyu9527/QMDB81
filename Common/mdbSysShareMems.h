/**
* @file SysShareMems.hxx
* @brief �����ڴ������
*
* ��װ�˹����ڴ�ĳ��ò���
*
* @author Ge.zhengyi
* @version 1.0
* @date 20121214
* @warning ��ͳһʹ�ø�����й����ڴ�Ĳ���
*/

#ifndef _MDB_H_SysShareMems_
#define _MDB_H_SysShareMems_

#include "Common/mdbSysUtils.h"

//namespace QuickMDB
//{
        class TMdbNtcShareMemDataInfo;
        /*
        * @brief �����ڴ������
        *
        * ʵ�ֹ����ڴ���ֲ���
        */
        class TMdbNtcShareMem : public TMdbNtcSystemNameObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcShareMem);
        public:
            /**
             * @brief ���캯��
             * 
             * @param pszName [in] �����ڴ������
             * @param ASize [in] �����ڴ�Ĵ�С�����Ϊ0����ʾAttach
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @param pParentNameObject [in] ����������������
             * @param bCheckVersion [in] �Ƿ��鹲���ڴ�İ汾�����ⲻͬ�汾�Ĺ����ڴ�ṹ���쵼�·����쳣
             * @return ��
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
        public://�ṩһЩ��̬����
            /**
             * @brief ��⹲���ڴ��Ƿ����
             * 
             * @param pszName [in] �����ڴ������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true ����
             */
            static bool CheckExist(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ���ٹ����ڴ�
             * 
             * @param pszName [in] �����ڴ������
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true �ɹ�
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

