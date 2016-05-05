/**
 * @file SysSockets.hxx
 * @brief ��socket�ķ�װ
 *
 * @author Ge.zhengyi, Jiang.jinzhou, Du.jiagen, Zhang.he
 * @version 1.0
 * @date 20121214
 * @warning
 */
#ifndef _MDB_H_SysSockets_
#define _MDB_H_SysSockets_
#include "Common/mdbCommons.h"
#include "Common/mdbBaseObject.h"
#ifndef OS_WINDOWS
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#endif

#ifdef OS_SUN
#include <sys/filio.h>
#endif

#ifndef MDB_NTC_SOCKET_ERROR
#define MDB_NTC_SOCKET_ERROR (-1)
#endif // !MDB_NTC_SOCKET_ERROR

#ifdef OS_WINDOWS
#define MDB_NTC_ZS_ETIMEDOUT               WSAETIMEDOUT
#define MDB_NTC_ZS_EINPROGRESS             WSAEINPROGRESS
#define MDB_NTC_ZS_EWOULDBLOCK             WSAEWOULDBLOCK
#else
#define MDB_NTC_ZS_ETIMEDOUT               ETIMEDOUT
#define MDB_NTC_ZS_EINPROGRESS             EINPROGRESS
#define MDB_NTC_ZS_EWOULDBLOCK             EWOULDBLOCK
#endif
#ifdef OS_WINDOWS
const char*  inet_ntop(int af, const void *src, char *dst, size_t size);
#endif
//namespace QuickMDB
//{
//    namespace BillingSDK
//    {
        #ifndef MDB_NTC_INVALID_SOCKET
        #define MDB_NTC_INVALID_SOCKET (QuickMDB_SOCKET)(~0)
        #endif
        /**
         * @brief socket����ͨ����
         * 
         * ����socket���ӷ��ͺͽ������ݣ��Լ���socketһЩ�������ã�����պͷ��ͻ����С���Ƿ�������
         * 
         */
        class TMdbNtcSocket:public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcSocket);
        public://��̬����
            static int GetLastError();
            static void SetLastError(int iErrorCode);
            /**
             * @brief �õ�socket�Ĵ�����Ϣ
             * 
             * @param iErrCode [in] ������
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetErrorInfo(int iErrCode);
            inline static TMdbNtcStringBuffer GetLastErrorInfo()
            {
                return GetErrorInfo(GetLastError());
            }
            /**
             * @brief �������ݰ�
             * 
             * @param fd [in] socket��������
             * @param pszBuffer [in] Ҫ���͵����ݰ�buffer
             * @param iSize [in/out] inΪҪ���͵����ݰ���С, outΪ����ʵ�ʷ��͵Ĵ�С
             * @param iTimeOut [in] ��ʱ����,Ĭ��-1��ʾ����ʱ(����)����λ����
             * @return bool
             * @retval true �ɹ�
             */
            static bool Send(QuickMDB_SOCKET fd, const char* pszBuffer, int& iSize, int iMilliSeconds = -1);
            /**
             * @brief �������ݰ�
             * 
             * @param fd [in] socket��������
             * @param pszBuffer [out] ���յ����ݰ�
             * @param iSize [in/out] inΪbuffer���size��outΪ�յ������ݰ���С
             * @param iTimeOut [in] ��ʱ����,Ĭ��-1��ʾ����ʱ(����)����λ����
             * @return bool
             * @retval true �ɹ�
             */
            static bool Recv(QuickMDB_SOCKET fd, char* pszBuffer, int& iSize, int iMilliSeconds = -1);
            /**
             * @brief �ر�socket�׽���
             * 
             * @param fd [in] socket��������
             * @return bool
             * @retval true �ɹ�
             */
            static bool Close(QuickMDB_SOCKET fd);
            /**
             * @brief ��ñ���socket�Ķ˿�
             * 
             * @param uiSocketID [out] socket��������.
             * @return MDB_UINT32
             * @retval �˿ں�
             */
            static MDB_UINT32 GetLocalPort(QuickMDB_SOCKET uiSocketID);
            /**
             * @brief ��ȡ����socket�ĵ�ַ��Ϣ
             * 
             * @param uiSocketID [in] socket��������
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetLocalAddrInfo(QuickMDB_SOCKET uiSocketID);
            /**
             * @brief ��ȡ����socket�ĵ�ַ��Ϣ
             * 
             * @param uiSocketID [in] socket��������
             * @param oAddrInfo [out] socket�ĵ�ַ��Ϣ.
             * @return bool
             * @retval true �ɹ�
             */
            static bool GetLocalAddrInfo(QuickMDB_SOCKET uiSocketID, sockaddr_in& oAddrInfo);
            /**
             * @brief ���Զ��socket�Ķ˿�
             * 
             * @param uiSocketID [out] socket��������.
             * @return MDB_UINT32
             * @retval �˿ں�
             */
            static MDB_UINT32 GetRemotePort(QuickMDB_SOCKET uiSocketID);
            /**
             * @brief ��ȡԶ��socket�ĵ�ַ��Ϣ
             * 
             * @param uiSocketID [in] socket��������
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetRemoteAddrInfo(QuickMDB_SOCKET uiSocketID);
            /**
             * @brief ��ȡԶ��socket�ĵ�ַ��Ϣ
             * 
             * @param uiSocketID [in] socket��������
             * @param oAddrInfo [out] socket�ĵ�ַ��Ϣ.
             * @return bool
             * @retval true �ɹ�
             */
            static bool GetRemoteAddrInfo(QuickMDB_SOCKET uiSocketID, sockaddr_in& oAddrInfo);
            /**
            * @brief ����socket�����Ƿ�����
            * 
            * @param fd [in] socket��������
            * @param bBlock [in] �Ƿ�����
            * @return bool
            * @retval 0���ɹ�����0��ʧ��
            */
            static bool SetBlockFlag(QuickMDB_SOCKET fd, bool bBlock);
            /**
             * @brief ���÷��ͻ������Ĵ�С
             * 
             * @param fd [in] socket��������
             * @param iSendBufSize [in] ���ͻ������Ĵ�С
             * @return bool
             * @retval true �ɹ�
             */
            static bool SetSendBufSize(QuickMDB_SOCKET fd, int iSendBufSize);
            /**
             * @brief ���ý��ջ������Ĵ�С
             * 
             * @param fd [in] socket��������
             * @param iRecvBufSize [in] ���ջ������Ĵ�С
             * @return bool
             * @retval true �ɹ�
             */
            static bool SetRecvBufSize(QuickMDB_SOCKET fd, int iRecvBufSize);
            /**
             * @brief ��ȡ���ͻ������Ĵ�С
             * 
             * @param fd [in] socket��������
             * @param iSendBufSize [out] ���ͻ������Ĵ�С
             * @return bool
             * @retval true �ɹ�
             */
            static bool GetSendBufSize(QuickMDB_SOCKET fd, int& iSendBufSize);
            /**
             * @brief ��ȡ���ջ������Ĵ�С
             * 
             * @param fd [in] socket��������
             * @param iRecvBufSize [out] ���ջ������Ĵ�С
             * @return bool
             * @retval true �ɹ�
             */
            static bool GetRecvBufSize(QuickMDB_SOCKET fd, int& iRecvBufSize);
            /**
             * @brief �������Źر�
             * 
             * @param fd [in] socket������
             * @param uiLingerTime [in] ����������ݣ���ʱ�ر�ʱ�䣨�룩
             * @return bool
             * @retval true �ɹ�
             */
            static bool SetSocketLinger(QuickMDB_SOCKET fd, unsigned int uiLingerTime = 30);
        public:
            /**
             * @brief ���캯��
             * 
             * @param uiSocketID [in] socket������
             */
            TMdbNtcSocket(QuickMDB_SOCKET uiSocketID = MDB_NTC_INVALID_SOCKET);
            /**
             * @brief ����������ͬʱclose������socket
             * 
             */
            virtual ~TMdbNtcSocket();
            /**
            * @brief ���ر����socketID
            */
            inline QuickMDB_SOCKET GetSocketID(void)
            {
                return m_uiSocketID;
            }
            /**
             * @brief ����������
             * 
             * @param uiSocketID [in] socket������
             */
            inline void SetSocketID(QuickMDB_SOCKET uiSocketID)
            {
                m_uiSocketID = uiSocketID;
            }
            /**
             * @brief ����֮��ıȽ�
             * ע��˺���Ϊconst,����ʱ��ע��
             * @param pObject [in] ��Ҫ��֮�ȽϵĶ���
             * @return MDB_INT64
             * @retval =0 ��ȣ� >0 ����, <0 С��
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const
            {
                return m_uiSocketID-static_cast<const TMdbNtcSocket*>(pObject)->m_uiSocketID;
            }    
            /**
             * @brief �������ݰ�
             * 
             * @param pszBuffer [in] Ҫ���͵����ݰ�buffer
             * @param iSendSize [in/out] inΪҪ���͵����ݰ���С, outΪ����ʵ�ʷ��͵Ĵ�С
             * @param iTimeOut [in] ��ʱ����,Ĭ��-1��ʾ����ʱ(����)����λ����
             * @return bool
             * @retval true �ɹ�
             */
            inline bool Send(const char* pszBuffer, int& iSendSize, int iMilliSeconds = -1)
            {
                return Send(m_uiSocketID,  pszBuffer,  iSendSize, iMilliSeconds);
            }
            /**
             * @brief �������ݰ�
             * 
             * @param pszBuffer [out] ���յ����ݰ�
             * @param iRecvSize [in/out] inΪbuffer���size��outΪ�յ������ݰ���С
             * @param iTimeOut [in] ��ʱ����,Ĭ��-1��ʾ����ʱ(����)����λ����
             * @return bool
             * @retval true �ɹ�
             */
            inline bool Recv(char* pszBuffer, int& iRecvSize, int iMilliSeconds = -1)
            {
                return Recv(m_uiSocketID, pszBuffer, iRecvSize, iMilliSeconds);
            }
            /**
             * @brief �ر�socket
             * 
             * @return bool
             * @retval true �ɹ�
             */
            bool Close();
            /**
            * @brief ����socket�����Ƿ�����
            * 
            * @param bBlock [in] �Ƿ�����
            * @return bool
            * @retval 0���ɹ�����0��ʧ��
            */
            inline bool SetBlockFlag(bool bBlock)
            {
                return SetBlockFlag( m_uiSocketID, bBlock );
            }
            /**
             * @brief ���÷��ͻ������Ĵ�С
             * 
             * @param iSendBufSize [in] ���ͻ������Ĵ�С
             * @return bool
             * @retval true �ɹ�
             */
            inline bool SetSendBufSize(int iSendBufSize)
            {
                return SetSendBufSize( m_uiSocketID, iSendBufSize);
            }
            /**
             * @brief ���ý��ջ������Ĵ�С
             * 
             * @param iRecvBufSize [in] ���ջ������Ĵ�С
             * @return bool
             * @retval true �ɹ�
             */
            inline bool SetRecvBufSize(int iRecvBufSize)
            {
                return SetRecvBufSize(m_uiSocketID, iRecvBufSize);
            }
            /**
             * @brief ��ȡ���ͻ������Ĵ�С
             * 
             * @param iSendBufSize [out] ���ͻ������Ĵ�С
             * @return bool
             * @retval true �ɹ�
             */
            inline bool GetSendBufSize(int& iSendBufSize)
            {
                return GetSendBufSize(m_uiSocketID, iSendBufSize);
            }
            /**
             * @brief ��ȡ���ջ������Ĵ�С
             * 
             * @param iRecvBufSize [out] ���ջ������Ĵ�С
             * @return bool
             * @retval true �ɹ�
             */
            inline bool GetRecvBufSize(int& iRecvBufSize)
            {
                return GetRecvBufSize(m_uiSocketID, iRecvBufSize);
            }
            /**
             * @brief ��ñ���socket�Ķ˿�
             * 
             * @return MDB_UINT32
             * @retval �˿ں�
             */
            inline MDB_UINT32 GetLocalPort()
            {
                return GetLocalPort(m_uiSocketID);
            }
            /**
             * @brief ��ȡ����socket�ĵ�ַ��Ϣ
             * 
             * @param oAddrInfo [out] socket�ĵ�ַ��Ϣ.
             * @return bool
             * @retval true �ɹ�
             */
            inline bool GetLocalAddrInfo(sockaddr_in& oAddrInfo)
            {
                return GetLocalAddrInfo(m_uiSocketID, oAddrInfo);
            }
            /**
             * @brief ��ȡ����socket�ĵ�ַ��Ϣ
             * 
             * @param uiSocketID [in] socket��������
             * @return TMdbNtcStringBuffer
             */
            inline TMdbNtcStringBuffer GetLocalAddrInfo()
            {
                return GetLocalAddrInfo(m_uiSocketID);
            }
            /**
             * @brief ���Զ��socket�Ķ˿�
             * 
             * @return MDB_UINT32
             * @retval �˿ں�
             */
            inline MDB_UINT32 GetRemotePort()
            {
                return GetRemotePort(m_uiSocketID);
            }
            /**
             * @brief ��ȡԶ��socket�ĵ�ַ��Ϣ
             * 
             * @param oAddrInfo [out] socket�ĵ�ַ��Ϣ.
             * @return bool
             * @retval true �ɹ�
             */
            inline bool GetRemoteAddrInfo(sockaddr_in& oAddrInfo)
            {
                return GetRemoteAddrInfo(m_uiSocketID, oAddrInfo);
            }
            /**
             * @brief ��ȡԶ��socket�ĵ�ַ��Ϣ
             * 
             * @param uiSocketID [in] socket��������
             * @return TMdbNtcStringBuffer
             */
            inline TMdbNtcStringBuffer GetRemoteAddrInfo()
            {
                return GetRemoteAddrInfo(m_uiSocketID);
            }
        protected:
            QuickMDB_SOCKET m_uiSocketID;///< socket������
        };

        /**
         * @brief �ͻ���socket��
         * ��socket�ͻ��˵�һЩ������װ�������ӣ��Ͽ��ȣ�ͬʱ�̳���TMdbNtcSocket����Ҳ�߱����ͺͽ�������
         */
        class TMdbNtcClientSocket:public TMdbNtcSocket
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcClientSocket);
        public://��̬����
            /**
             * @brief ���ӷ����
             * 
             * @param fd [out] ���ӳɹ���õ���socket������
             * @param pszRemoteHost [in] Զ�̷���˵�ip��ַ������
             * @param iPort [in] Զ�̷���˵Ķ˿�
             * @param iMilliSeconds [in] ���ӳ�ʱʱ��(����)��������ֵ����Ϊ����ʧ��,ȡֵΪ-1��ʾ��Ĭ��ʱ����(3s)
             * @return bool
             * @retval true �ɹ�
             */
            static bool Connect(QuickMDB_SOCKET& fd, const char* pszRemoteHost, int iPort, int iMilliSeconds = -1);
            /**
             * @brief �Ͽ�����
             * 
             * @param fd [in] socket��������
             * @return bool
             * @retval true �ɹ�
             */
            inline static bool Disconnect(QuickMDB_SOCKET fd)
            {
                return Close(fd);
            }
        public:
            /**
             * @brief ���ӷ����
             * 
             * @param pszRemoteHost [in] Զ�̷���˵�ip��ַ������
             * @param iPort [in] Զ�̷���˵Ķ˿�
             * @param iMilliSeconds [in] ���ӳ�ʱʱ��(����)��������ֵ����Ϊ����ʧ��,ȡֵΪ-1��ʾ��Ĭ��ʱ����(3s)
             * @return bool
             * @retval true �ɹ�
             */
            inline bool Connect(const char* pszRemoteHost, int iPort, int iMilliSeconds = -1)
            {
                m_sRemoteHost   =   pszRemoteHost;
                m_iRemotePort   =   iPort;
                return Connect(m_uiSocketID, pszRemoteHost, iPort, iMilliSeconds);
            }
            /**
             * @brief ���������
             * 
             * @param iMilliSeconds [in] ���ӳ�ʱʱ��(����)��������ֵ����Ϊ����ʧ��,ȡֵΪ-1��ʾ��Ĭ��ʱ����(3s)
             * @return bool
             * @retval true �ɹ�
             */
            bool Reconnect(int iMilliSeconds = -1);
            /**
             * @brief �Ͽ�����
             * 
             * @return bool
             * @retval true �ɹ�
             */
            bool Disconnect();
        protected:
            TMdbNtcStringBuffer m_sRemoteHost;///< Զ�̷���˵�ip��ַ��������
            int m_iRemotePort;///< Զ�̷���˵Ķ˿�
        };

        /**
         * @brief �����socket��
         * ��socket����˵�һЩ������װ��������ͽ������ӵ�
         */
        class TMdbNtcServerSocket:public TMdbNtcSocket
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcServerSocket);
        public://��̬�ӿ�
            /**
             * @brief ����������ĳһ�˿�
             * 
             * @param fd [out] �����ɹ��ļ����׽���
             * @param pszAddress [in] pszAddressΪNULL��ʾ����ip
             * @param iPort [in] Ҫ�󶨵Ķ˿�
			 * @param bReuseAddr [in] �Ƿ����ö˿�
             * @return bool
             * @retval true �ɹ�
             */
            static bool Listen(QuickMDB_SOCKET& fd, const char* pszAddress, int iPort, bool bReuseAddr = true);
            /**
             * @brief ���տͻ�������
             * 
             * @param serv_fd [in] �����׽���
             * @param client_fd [out] �����socket�˵�socket������     
             * @param ptAddrInfo [out] socket�˵�ַ��Ϣ, ���ΪNULL�������踳ֵ����Ϣ
             * @param iMilliSeconds [in] ��ʱ����,Ĭ��-1��ʾ����ʱ(����ʽ)����λ����
             * @return bool
             * @retval true �ɹ�
             */
            static bool Accept(QuickMDB_SOCKET serv_fd, QuickMDB_SOCKET& client_fd, sockaddr_in* ptAddrInfo = NULL, int iMilliSeconds = -1);
        public:
            TMdbNtcServerSocket();
            inline TMdbNtcStringBuffer GetServerHost() const
            {
                return m_sServerHost;
            }
            inline int GetServerPort() const
            {
                return m_iServerPort;
            }
            /**
             * @brief ����������ĳһ�˿�
             * 
             * @param pszAddress [in] pszAddressΪNULL��ʾ����ip
             * @param iPort [in] Ҫ�󶨵Ķ˿�
			 * @param bReuseAddr [in] �Ƿ����ö˿�
             * @return bool
             * @retval true �ɹ�
             */
            inline bool Listen(const char* pszAddress, int iPort, bool bReuseAddr = true)
            {
                m_sServerHost   =   pszAddress;
                m_iServerPort   =   iPort;
                return Listen(m_uiSocketID, pszAddress, iPort, bReuseAddr);
            }
            /**
             * @brief ���տͻ�������
             * 
             * @param uiSocketID [out] �����socket�˵�socket������
             * @param ptAddrInfo [out] socket�˵�ַ��Ϣ, ���ΪNULL�������踳ֵ����Ϣ
             * @param iMilliSeconds [in] ��ʱ����,Ĭ��-1��ʾ����ʱ(����ʽ)����λ����
             * @return bool
             * @retval true �ɹ�
             */
            inline bool Accept(QuickMDB_SOCKET& client_fd, sockaddr_in* ptAddrInfo = NULL, int iMilliSeconds = -1)
            {
                return  Accept(m_uiSocketID, client_fd, ptAddrInfo, iMilliSeconds );
            }
        protected:
            TMdbNtcStringBuffer   m_sServerHost; ///< �����ip��ַ������
            int             m_iServerPort;      ///< ����˶˿�
        };
//    }
//}
#endif
