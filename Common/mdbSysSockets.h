/**
 * @file SysSockets.hxx
 * @brief 对socket的封装
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
         * @brief socket网络通信类
         * 
         * 基于socket连接发送和接收数据，以及对socket一些属性设置，如接收和发送缓冲大小，是否阻塞等
         * 
         */
        class TMdbNtcSocket:public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcSocket);
        public://静态方法
            static int GetLastError();
            static void SetLastError(int iErrorCode);
            /**
             * @brief 得到socket的错误信息
             * 
             * @param iErrCode [in] 错误码
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetErrorInfo(int iErrCode);
            inline static TMdbNtcStringBuffer GetLastErrorInfo()
            {
                return GetErrorInfo(GetLastError());
            }
            /**
             * @brief 发送数据包
             * 
             * @param fd [in] socket的描述符
             * @param pszBuffer [in] 要发送的数据包buffer
             * @param iSize [in/out] in为要发送的数据包大小, out为返回实际发送的大小
             * @param iTimeOut [in] 超时设置,默认-1表示不超时(阻塞)，单位毫秒
             * @return bool
             * @retval true 成功
             */
            static bool Send(QuickMDB_SOCKET fd, const char* pszBuffer, int& iSize, int iMilliSeconds = -1);
            /**
             * @brief 接收数据包
             * 
             * @param fd [in] socket的描述符
             * @param pszBuffer [out] 接收的数据包
             * @param iSize [in/out] in为buffer最大size，out为收到的数据包大小
             * @param iTimeOut [in] 超时设置,默认-1表示不超时(阻塞)，单位毫秒
             * @return bool
             * @retval true 成功
             */
            static bool Recv(QuickMDB_SOCKET fd, char* pszBuffer, int& iSize, int iMilliSeconds = -1);
            /**
             * @brief 关闭socket套接字
             * 
             * @param fd [in] socket的描述符
             * @return bool
             * @retval true 成功
             */
            static bool Close(QuickMDB_SOCKET fd);
            /**
             * @brief 获得本地socket的端口
             * 
             * @param uiSocketID [out] socket的描述符.
             * @return MDB_UINT32
             * @retval 端口号
             */
            static MDB_UINT32 GetLocalPort(QuickMDB_SOCKET uiSocketID);
            /**
             * @brief 获取本地socket的地址信息
             * 
             * @param uiSocketID [in] socket的描述符
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetLocalAddrInfo(QuickMDB_SOCKET uiSocketID);
            /**
             * @brief 获取本地socket的地址信息
             * 
             * @param uiSocketID [in] socket的描述符
             * @param oAddrInfo [out] socket的地址信息.
             * @return bool
             * @retval true 成功
             */
            static bool GetLocalAddrInfo(QuickMDB_SOCKET uiSocketID, sockaddr_in& oAddrInfo);
            /**
             * @brief 获得远程socket的端口
             * 
             * @param uiSocketID [out] socket的描述符.
             * @return MDB_UINT32
             * @retval 端口号
             */
            static MDB_UINT32 GetRemotePort(QuickMDB_SOCKET uiSocketID);
            /**
             * @brief 获取远程socket的地址信息
             * 
             * @param uiSocketID [in] socket的描述符
             * @return TMdbNtcStringBuffer
             */
            static TMdbNtcStringBuffer GetRemoteAddrInfo(QuickMDB_SOCKET uiSocketID);
            /**
             * @brief 获取远程socket的地址信息
             * 
             * @param uiSocketID [in] socket的描述符
             * @param oAddrInfo [out] socket的地址信息.
             * @return bool
             * @retval true 成功
             */
            static bool GetRemoteAddrInfo(QuickMDB_SOCKET uiSocketID, sockaddr_in& oAddrInfo);
            /**
            * @brief 设置socket链接是否阻塞
            * 
            * @param fd [in] socket的描述符
            * @param bBlock [in] 是否阻塞
            * @return bool
            * @retval 0：成功，非0：失败
            */
            static bool SetBlockFlag(QuickMDB_SOCKET fd, bool bBlock);
            /**
             * @brief 设置发送缓冲区的大小
             * 
             * @param fd [in] socket的描述符
             * @param iSendBufSize [in] 发送缓冲区的大小
             * @return bool
             * @retval true 成功
             */
            static bool SetSendBufSize(QuickMDB_SOCKET fd, int iSendBufSize);
            /**
             * @brief 设置接收缓冲区的大小
             * 
             * @param fd [in] socket的描述符
             * @param iRecvBufSize [in] 接收缓冲区的大小
             * @return bool
             * @retval true 成功
             */
            static bool SetRecvBufSize(QuickMDB_SOCKET fd, int iRecvBufSize);
            /**
             * @brief 获取发送缓冲区的大小
             * 
             * @param fd [in] socket的描述符
             * @param iSendBufSize [out] 发送缓冲区的大小
             * @return bool
             * @retval true 成功
             */
            static bool GetSendBufSize(QuickMDB_SOCKET fd, int& iSendBufSize);
            /**
             * @brief 获取接收缓冲区的大小
             * 
             * @param fd [in] socket的描述符
             * @param iRecvBufSize [out] 接收缓冲区的大小
             * @return bool
             * @retval true 成功
             */
            static bool GetRecvBufSize(QuickMDB_SOCKET fd, int& iRecvBufSize);
            /**
             * @brief 设置优雅关闭
             * 
             * @param fd [in] socket描述字
             * @param uiLingerTime [in] 如果尚有数据，延时关闭时间（秒）
             * @return bool
             * @retval true 成功
             */
            static bool SetSocketLinger(QuickMDB_SOCKET fd, unsigned int uiLingerTime = 30);
        public:
            /**
             * @brief 构造函数
             * 
             * @param uiSocketID [in] socket描述符
             */
            TMdbNtcSocket(QuickMDB_SOCKET uiSocketID = MDB_NTC_INVALID_SOCKET);
            /**
             * @brief 析构函数，同时close关联的socket
             * 
             */
            virtual ~TMdbNtcSocket();
            /**
            * @brief 返回本身的socketID
            */
            inline QuickMDB_SOCKET GetSocketID(void)
            {
                return m_uiSocketID;
            }
            /**
             * @brief 设置描述符
             * 
             * @param uiSocketID [in] socket描述符
             */
            inline void SetSocketID(QuickMDB_SOCKET uiSocketID)
            {
                m_uiSocketID = uiSocketID;
            }
            /**
             * @brief 对象之间的比较
             * 注意此函数为const,重载时需注意
             * @param pObject [in] 需要与之比较的对象
             * @return MDB_INT64
             * @retval =0 相等， >0 大于, <0 小于
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const
            {
                return m_uiSocketID-static_cast<const TMdbNtcSocket*>(pObject)->m_uiSocketID;
            }    
            /**
             * @brief 发送数据包
             * 
             * @param pszBuffer [in] 要发送的数据包buffer
             * @param iSendSize [in/out] in为要发送的数据包大小, out为返回实际发送的大小
             * @param iTimeOut [in] 超时设置,默认-1表示不超时(阻塞)，单位毫秒
             * @return bool
             * @retval true 成功
             */
            inline bool Send(const char* pszBuffer, int& iSendSize, int iMilliSeconds = -1)
            {
                return Send(m_uiSocketID,  pszBuffer,  iSendSize, iMilliSeconds);
            }
            /**
             * @brief 接收数据包
             * 
             * @param pszBuffer [out] 接收的数据包
             * @param iRecvSize [in/out] in为buffer最大size，out为收到的数据包大小
             * @param iTimeOut [in] 超时设置,默认-1表示不超时(阻塞)，单位毫秒
             * @return bool
             * @retval true 成功
             */
            inline bool Recv(char* pszBuffer, int& iRecvSize, int iMilliSeconds = -1)
            {
                return Recv(m_uiSocketID, pszBuffer, iRecvSize, iMilliSeconds);
            }
            /**
             * @brief 关闭socket
             * 
             * @return bool
             * @retval true 成功
             */
            bool Close();
            /**
            * @brief 设置socket链接是否阻塞
            * 
            * @param bBlock [in] 是否阻塞
            * @return bool
            * @retval 0：成功，非0：失败
            */
            inline bool SetBlockFlag(bool bBlock)
            {
                return SetBlockFlag( m_uiSocketID, bBlock );
            }
            /**
             * @brief 设置发送缓冲区的大小
             * 
             * @param iSendBufSize [in] 发送缓冲区的大小
             * @return bool
             * @retval true 成功
             */
            inline bool SetSendBufSize(int iSendBufSize)
            {
                return SetSendBufSize( m_uiSocketID, iSendBufSize);
            }
            /**
             * @brief 设置接收缓冲区的大小
             * 
             * @param iRecvBufSize [in] 接收缓冲区的大小
             * @return bool
             * @retval true 成功
             */
            inline bool SetRecvBufSize(int iRecvBufSize)
            {
                return SetRecvBufSize(m_uiSocketID, iRecvBufSize);
            }
            /**
             * @brief 获取发送缓冲区的大小
             * 
             * @param iSendBufSize [out] 发送缓冲区的大小
             * @return bool
             * @retval true 成功
             */
            inline bool GetSendBufSize(int& iSendBufSize)
            {
                return GetSendBufSize(m_uiSocketID, iSendBufSize);
            }
            /**
             * @brief 获取接收缓冲区的大小
             * 
             * @param iRecvBufSize [out] 接收缓冲区的大小
             * @return bool
             * @retval true 成功
             */
            inline bool GetRecvBufSize(int& iRecvBufSize)
            {
                return GetRecvBufSize(m_uiSocketID, iRecvBufSize);
            }
            /**
             * @brief 获得本地socket的端口
             * 
             * @return MDB_UINT32
             * @retval 端口号
             */
            inline MDB_UINT32 GetLocalPort()
            {
                return GetLocalPort(m_uiSocketID);
            }
            /**
             * @brief 获取本地socket的地址信息
             * 
             * @param oAddrInfo [out] socket的地址信息.
             * @return bool
             * @retval true 成功
             */
            inline bool GetLocalAddrInfo(sockaddr_in& oAddrInfo)
            {
                return GetLocalAddrInfo(m_uiSocketID, oAddrInfo);
            }
            /**
             * @brief 获取本地socket的地址信息
             * 
             * @param uiSocketID [in] socket的描述符
             * @return TMdbNtcStringBuffer
             */
            inline TMdbNtcStringBuffer GetLocalAddrInfo()
            {
                return GetLocalAddrInfo(m_uiSocketID);
            }
            /**
             * @brief 获得远程socket的端口
             * 
             * @return MDB_UINT32
             * @retval 端口号
             */
            inline MDB_UINT32 GetRemotePort()
            {
                return GetRemotePort(m_uiSocketID);
            }
            /**
             * @brief 获取远程socket的地址信息
             * 
             * @param oAddrInfo [out] socket的地址信息.
             * @return bool
             * @retval true 成功
             */
            inline bool GetRemoteAddrInfo(sockaddr_in& oAddrInfo)
            {
                return GetRemoteAddrInfo(m_uiSocketID, oAddrInfo);
            }
            /**
             * @brief 获取远程socket的地址信息
             * 
             * @param uiSocketID [in] socket的描述符
             * @return TMdbNtcStringBuffer
             */
            inline TMdbNtcStringBuffer GetRemoteAddrInfo()
            {
                return GetRemoteAddrInfo(m_uiSocketID);
            }
        protected:
            QuickMDB_SOCKET m_uiSocketID;///< socket描述符
        };

        /**
         * @brief 客户端socket类
         * 对socket客户端的一些操作封装，如连接，断开等，同时继承于TMdbNtcSocket所以也具备发送和接收数据
         */
        class TMdbNtcClientSocket:public TMdbNtcSocket
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcClientSocket);
        public://静态方法
            /**
             * @brief 连接服务端
             * 
             * @param fd [out] 连接成功后得到的socket描述符
             * @param pszRemoteHost [in] 远程服务端的ip地址或域名
             * @param iPort [in] 远程服务端的端口
             * @param iMilliSeconds [in] 连接超时时间(毫秒)，超过此值则判为连接失败,取值为-1表示按默认时间来(3s)
             * @return bool
             * @retval true 成功
             */
            static bool Connect(QuickMDB_SOCKET& fd, const char* pszRemoteHost, int iPort, int iMilliSeconds = -1);
            /**
             * @brief 断开链接
             * 
             * @param fd [in] socket的描述符
             * @return bool
             * @retval true 成功
             */
            inline static bool Disconnect(QuickMDB_SOCKET fd)
            {
                return Close(fd);
            }
        public:
            /**
             * @brief 连接服务端
             * 
             * @param pszRemoteHost [in] 远程服务端的ip地址或域名
             * @param iPort [in] 远程服务端的端口
             * @param iMilliSeconds [in] 连接超时时间(毫秒)，超过此值则判为连接失败,取值为-1表示按默认时间来(3s)
             * @return bool
             * @retval true 成功
             */
            inline bool Connect(const char* pszRemoteHost, int iPort, int iMilliSeconds = -1)
            {
                m_sRemoteHost   =   pszRemoteHost;
                m_iRemotePort   =   iPort;
                return Connect(m_uiSocketID, pszRemoteHost, iPort, iMilliSeconds);
            }
            /**
             * @brief 重连服务端
             * 
             * @param iMilliSeconds [in] 连接超时时间(毫秒)，超过此值则判为连接失败,取值为-1表示按默认时间来(3s)
             * @return bool
             * @retval true 成功
             */
            bool Reconnect(int iMilliSeconds = -1);
            /**
             * @brief 断开链接
             * 
             * @return bool
             * @retval true 成功
             */
            bool Disconnect();
        protected:
            TMdbNtcStringBuffer m_sRemoteHost;///< 远程服务端的ip地址或者域名
            int m_iRemotePort;///< 远程服务端的端口
        };

        /**
         * @brief 服务端socket类
         * 对socket服务端的一些操作封装，如监听和接受连接等
         */
        class TMdbNtcServerSocket:public TMdbNtcSocket
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcServerSocket);
        public://静态接口
            /**
             * @brief 监听本机的某一端口
             * 
             * @param fd [out] 创建成功的监听套接字
             * @param pszAddress [in] pszAddress为NULL表示所有ip
             * @param iPort [in] 要绑定的端口
			 * @param bReuseAddr [in] 是否重用端口
             * @return bool
             * @retval true 成功
             */
            static bool Listen(QuickMDB_SOCKET& fd, const char* pszAddress, int iPort, bool bReuseAddr = true);
            /**
             * @brief 接收客户端连接
             * 
             * @param serv_fd [in] 监听套接字
             * @param client_fd [out] 连入的socket端的socket描述字     
             * @param ptAddrInfo [out] socket端地址信息, 如果为NULL，则无需赋值此信息
             * @param iMilliSeconds [in] 超时设置,默认-1表示不超时(阻塞式)，单位毫秒
             * @return bool
             * @retval true 成功
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
             * @brief 监听本机的某一端口
             * 
             * @param pszAddress [in] pszAddress为NULL表示所有ip
             * @param iPort [in] 要绑定的端口
			 * @param bReuseAddr [in] 是否重用端口
             * @return bool
             * @retval true 成功
             */
            inline bool Listen(const char* pszAddress, int iPort, bool bReuseAddr = true)
            {
                m_sServerHost   =   pszAddress;
                m_iServerPort   =   iPort;
                return Listen(m_uiSocketID, pszAddress, iPort, bReuseAddr);
            }
            /**
             * @brief 接收客户端连接
             * 
             * @param uiSocketID [out] 连入的socket端的socket描述字
             * @param ptAddrInfo [out] socket端地址信息, 如果为NULL，则无需赋值此信息
             * @param iMilliSeconds [in] 超时设置,默认-1表示不超时(阻塞式)，单位毫秒
             * @return bool
             * @retval true 成功
             */
            inline bool Accept(QuickMDB_SOCKET& client_fd, sockaddr_in* ptAddrInfo = NULL, int iMilliSeconds = -1)
            {
                return  Accept(m_uiSocketID, client_fd, ptAddrInfo, iMilliSeconds );
            }
        protected:
            TMdbNtcStringBuffer   m_sServerHost; ///< 服务端ip地址或域名
            int             m_iServerPort;      ///< 服务端端口
        };
//    }
//}
#endif
