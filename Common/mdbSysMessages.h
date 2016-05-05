/**
 * @file SysShareMQ.hxx
 * ���̼乲�����Ϣ���У����ڹ����ڴ棩
 * 
 * @author 
 * @version 1.0
 * @date 2013/05/20
 * @bug �½�����bug
 * @bug ��Ԫ���ԣ�δ����bug
 * @warning 
 */
#ifndef  _MDB_SYS_MESSAGES_HXX_
#define  _MDB_SYS_MESSAGES_HXX_
#include "Common/mdbSysShareMems.h"
#include "Common/mdbSysLocks.h"
//namespace QuickMDB
//{
        class TMdbNtcMessageQueue;
        /**
         * @brief �ص���ʽ������Ϣ�ĺ�����������
         * 
         * @param pMQ       [in] �������ǵ���Ϣ����
         * @param uiType    [in] ��Ϣ����
         * @param pBuffer   [in] ��Ϣbuffer
         * @param uiLength  [in] ��Ϣ����
         * @param pArg      [in] �ص������Ĳ���
         */
        typedef void (*mdb_ntc_queue_msg_callback)(TMdbNtcMessageQueue* pMQ, MDB_UINT16 uiType, const void* pBuffer, MDB_UINT32 uiLength, void* pArg);
        /**
         * @brief ������Ϣ���е����
         * 
         */
        class TMdbNtcMessageQueue:public TMdbNtcSystemNameObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcMessageQueue);
        protected:
            struct RECV_MSG
            {
                MDB_UINT16& uiType;
                TMdbNtcDataBuffer& sOut;
                RECV_MSG(MDB_UINT16& uiTypeParam, TMdbNtcDataBuffer& sOutParam)
                    :uiType(uiTypeParam), sOut(sOutParam)
                {
                }
            };
            static void recv_queue_msg_callback(TMdbNtcMessageQueue* pMQ, MDB_UINT16 uiType, const void* pBuffer, MDB_UINT32 uiLength, void* pArg);
        public:
            /**
             * @brief ������Ϣ����
             * 
             * @param pszName  [in] ��Ϣ���е�����
             * @param uiShmSize [in] ��Ϣ����ռ���ڴ�Ĵ�С������Ǹ��ӵĻ��������贫��size
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @param pParentNameObject [in] ����������������
             * @param bCheckVersion [in] �Ƿ��鹲���ڴ�İ汾�����ⲻͬ�汾�Ĺ����ڴ�ṹ���쵼�·����쳣
             * @return ��
             */
            TMdbNtcMessageQueue(const char* pszName, MDB_UINT32 uiShmSize = 0,
                const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME, TMdbNtcSystemNameObject* pParentNameObject = NULL, bool bCheckVersion = true);
            ~TMdbNtcMessageQueue();
            /**
             * @brief ���ø�����Ϣʱ�Ļص�����
             * 
             * @param func [in] ������Ϣʱ�Ļص�����
             * @param pArg [in] �ص������Ĳ���
             */
            inline void SetMsgOverlappedCallback(mdb_ntc_queue_msg_callback func, void* pArg = NULL)
            {
                m_fnMsgOverlappedCallback = func;
                m_pMsgOverlappedArg = NULL;
            }
            /**
             * @brief ��ø�����Ϣʱ�Ļص�����
             * 
             * @return msg_overlapped_callback
             * @retval ������Ϣʱ�Ļص�����
             */
            inline mdb_ntc_queue_msg_callback GetMsgOverlappedCallback()
            {
                return m_fnMsgOverlappedCallback;
            }
            /**
             * @brief �жϵ�ǰ��Ϣ�����Ƿ���Ȼ��Ч
             * 
             * @return bool
             * @retval true ��Ч
             */
            bool IsOK();
            /**
             * @brief ͬ��������Ϣ�����追��buffer���������Ч��
             * �������ȽϺ�ʱ������������Ϣ��ѹ�϶࣬�����¸��Ƿ�ʽѹ����Ϣʱ����������ǰ��Ϣ������ϡ�
             * 
             * @param deal_func     [in] ������
             * @param pArg          [in] �ص�ʱ�Ĳ���
             * @param iMilliSeconds [in] ��ʱ���ã���λ����,-1��ʾ������0��ʾ��������������ʾ��ʱ����ʱ��
             * @return bool
             * @retval true ��ȡ����Ϣ�����������
             */
            bool SyncDealMsg(mdb_ntc_queue_msg_callback deal_func, void* pArg = NULL, MDB_INT32 iMilliSeconds = -1);
            /**
             * @brief ѹ��һ����Ϣ
             *              
             * @param pBuffer [in] Ҫѹ�����Ϣbuffer
             * @param uiLength   [in] Ҫѹ�����Ϣ���ȣ�-1��ʾ��'\0'��β
             * @param bAllowOverlap [in] �Ƿ������Ǿɵ�δȡ������Ϣ
             * @return bool
             * @retval true �ɹ�
             */
            inline bool Send(const void* pBuffer, MDB_UINT32 uiLength = -1, bool bAllowOverlap = false)
            {
                return Send(0, pBuffer, uiLength, bAllowOverlap);
            }

            /**
             * @brief ѹ��һ����Ϣ
             * 
             * @param uiType [in] Ҫѹ�����������
             * @param pBuffer [in] Ҫѹ�����Ϣbuffer
             * @param uiLength   [in] Ҫѹ�����Ϣ���ȣ�-1��ʾ��'\0'��β
             * @param bAllowOverlap [in] �Ƿ������Ǿɵ�δȡ������Ϣ
             * @return bool
             * @retval true �ɹ�
             */
            bool Send(MDB_UINT16 uiType, const void* pBuffer, MDB_UINT32 uiLength = -1, bool bAllowOverlap = false);

            /**
             * @brief ��ȡһ����Ϣ
             * 
             * @param sOut          [out] �����buffer
             * @param iMilliSeconds [in] ��ʱ���ã���λ����,-1��ʾ������0��ʾ��������������ʾ��ʱ����ʱ��
             * @return bool
             * @retval true �ɹ�
             */
            inline bool Receive(TMdbNtcDataBuffer& sOut, MDB_INT32 iMilliSeconds = -1)
            {
                MDB_UINT16 uiType = 0;
                RECV_MSG oRecvMsg(uiType, sOut);
                return SyncDealMsg(recv_queue_msg_callback, &oRecvMsg, iMilliSeconds);
            }

            /**
             * @brief ��ȡһ����Ϣ
             * 
             * @param uiType        [out] ���ݵ�����
             * @param sOut          [out] �����buffer
             * @param iMilliSeconds [in] ��ʱ���ã���λ����,-1��ʾ������0��ʾ��������������ʾ��ʱ����ʱ��
             * @return bool
             * @retval true �ɹ�
             */
            inline bool Receive(MDB_UINT16& uiType, TMdbNtcDataBuffer& sOut, MDB_INT32 iMilliSeconds = -1)
            {
                RECV_MSG oRecvMsg(uiType, sOut);
                return SyncDealMsg(recv_queue_msg_callback, &oRecvMsg, iMilliSeconds);
            }
            /**
             * @brief �жϵ�ǰ�����Ƿ�Ϊ��
             * 
             * @return bool
             * @retval true �ն���
             */
            inline bool IsEmpty() const
            {
                return m_pMgrInfo?(m_pMgrInfo->uiPopTimes == m_pMgrInfo->uiPushTimes):true;
            }
            /**
             * @brief ��ȡ�����е���Ϣ��
             * 
             * @return unsigned int
             * @retval ��Ϣ��
             */
            MDB_UINT32 GetCount() const;

            /**
             * @brief �������ѹ�����Ϣ��
             * 
             * @return MDB_UINT32
             * @retval ѹ�����Ϣ��
             */
            inline MDB_UINT32 GetPushTimes() const
            {
                return m_pMgrInfo?m_pMgrInfo->uiPushTimes:0;
            }

            /**
             * @brief �Ӷ�����ȡ������Ϣ��
             * 
             * @return MDB_UINT32
             * @retval ȡ������Ϣ��
             */
            inline MDB_UINT32 GetPopTimes() const
            {
                return m_pMgrInfo?m_pMgrInfo->uiPopTimes:0;
            }
            /**
             * @brief �����Ϣ���е������ֽ���
             * 
             * @return MDB_UINT32
             * @retval �����ֽ���
             */
            inline MDB_UINT32 GetTotalSize() const
            {
                return m_uiShmSize;
            }
            /**
             * @brief ��õ�ǰ��Ϣ������ʹ�õ��ֽ���
             * 
             * @return MDB_UINT32
             * @retval ��ʹ�õ��ֽ���
             */
            MDB_UINT32 GetUsedSize() const;
            /**
             * @brief ��û�ʣ����ֽ������������ж��Ƿ񻹷ŵ���
             * 
             * @return MDB_UINT32
             */
            MDB_UINT32 GetFreeSize() const;
            /**
             * @brief ������Ϣ����
             * 
             * @return bool
             * @retval true �ɹ�
             */
            virtual bool ManualFree();
        public://�ṩһЩ��̬����
            /**
             * @brief �����Ϣ�����Ƿ����
             * 
             * @param pszName [in] ��Ϣ���е�����
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true ����
             */
            static bool CheckExist(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            /**
             * @brief ������Ϣ����
             * 
             * @param pszName [in] ��Ϣ���е�����
             * @param pszInstEnvName [in] IPCʵ����ʹ�õĻ�����������
             * @return bool
             * @retval true �ɹ�
             */
            static bool Destroy(const char* pszName, const char* pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
        private:
            /**
             *@brief ������Ϣͷ
             *
             *@param uiType [in] ��Ϣ����
             *@param uiBodyLen [in] ��Ϣ��ĳ���
             *@return ��
             */
            void SetMsgHead( MDB_UINT16 uiType, MDB_UINT32 uiBodyLen);

            /**
             *@brief ��ȡ��Ϣͷ��Ϣ
             *
             *@param uiType [out] ��Ϣ����
             *@param uiBodyLen [out] ��Ϣ��ĳ���
             *@return ��
             *@author zhou.qilong1@zte.com.cn
             */
            void GetMsgHead(MDB_UINT16& uiType, MDB_UINT32& uiBodyLen);
        private:
            /**
             *@brief ������Ϣ���й����ڴ�������Ľṹ
             */
            struct TMQMgrInfo
            {
                MDB_UINT32  uiPushCursor;   ///< Push�α��ֵ
                MDB_UINT32  uiPopCursor;    ///< Pop�α��ֵ
                MDB_UINT32  uiPushTimes;    ///< ��Push����Ϣ����
                MDB_UINT32  uiPopTimes;     ///< ��Pop����Ϣ����
                MDB_UINT32  uiLastPushTime; ///< ���ѹ���ʱ��
                MDB_UINT32  uiLastPopTime;  ///< ����ȡ��ʱ��                
                char    szReserve[128]; ///< ����һЩ�ռ䣬�Ա�����
            };
            TMdbNtcShareMem*      m_pShm;     ///< ��Ϣ�����ϵ���Ϣ����ŵĹ����ڴ�
            TMdbNtcProcessLock*   m_pPushLock, *m_pPopLock;   ///< ��Ϣ�����ϵĻ����ź���
            TMdbNtcProcessEvent*  m_pEvent;   ///< ʹ���¼�����ɽ��̼��֪ͨ
            TMQMgrInfo*     m_pMgrInfo;    ///<���������Ϣ�����ڴ��������Ϣ
            unsigned char*  m_pContentAddr; ///<��Ϣ���ݶε���ʼ��ַ
            MDB_UINT32          m_uiShmSize;  ///<������ͳ��������Ϣ������еĴ�С
            mdb_ntc_queue_msg_callback  m_fnMsgOverlappedCallback;///< ������Ϣʱ�ص�����
            void*           m_pMsgOverlappedArg;///< ������Ϣʱ�ص������Ĳ���
            static const MDB_UINT32 ms_uiMsgHeadLen;///< ÿ����Ϣ��ͷ����
        };
//}
#endif
