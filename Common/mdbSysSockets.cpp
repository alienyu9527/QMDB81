#include "Common/mdbSysSockets.h"
#include "Common/mdbSysThreads.h"
#include "Common/mdbStrUtils.h"
//#include "Common/mdbLogInterface.h"
#include <assert.h>
#ifdef OS_SUN
#include <sys/filio.h>
#endif
#ifdef OS_WINDOWS
const char*  inet_ntop(int af, const void *src, char *dst, size_t size)
{
    switch (af)
    {
    case AF_INET:
        {
            int l = _snprintf(dst, size, "%u.%u.%u.%u",
                ((const unsigned char*)src)[0], ((const unsigned char*)src)[1],
                ((const unsigned char*)src)[2], ((const unsigned char*)src)[3]);
            if (l <= 0 || l >= (int)size)
            {
                return (NULL);
            }
            return dst;
        }
    default:
        return (NULL);
    }
}
#endif
//namespace QuickMDB
//{
//    namespace BillingSDK
//    {
        class TMdbNtcSocketInit
        {
        public:
            TMdbNtcSocketInit()
            {                
                Init();
            }            
            ~TMdbNtcSocketInit()
            {
                Uninit();
            }
            bool Init();
            bool Uninit();
        };

        bool TMdbNtcSocketInit::Init()
        {
            #ifdef OS_WINDOWS
                WSADATA  wsaData;
                WORD wVer=0x202;
                return WSAStartup(wVer,&wsaData)==0;
            #else
                return true;
            #endif
        }

        bool TMdbNtcSocketInit::Uninit()
        {
            #ifdef OS_WINDOWS
                return WSACleanup()==0;
            #else
                return true;
            #endif
        }

        TMdbNtcSocketInit g_oMdbNtcSocketInit;///< 全局变量先初始化socket组件

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcSocket, TMdbNtcBaseObject);
        TMdbNtcSocket::TMdbNtcSocket(unsigned int uiSocketID /* = MDB_NTC_INVALID_SOCKET */)
        {
            m_uiSocketID = uiSocketID;
        }

        TMdbNtcSocket::~TMdbNtcSocket()
        {
            Close();
        }

        int TMdbNtcSocket::GetLastError()
        {
#ifdef OS_WINDOWS
            return WSAGetLastError();
#else
            return errno;
#endif
        }

        void TMdbNtcSocket::SetLastError(int iErrorCode)
        {
#ifdef OS_WINDOWS
            WSASetLastError(iErrorCode);
#else
            errno = iErrorCode;
#endif
        }
        
        TMdbNtcStringBuffer TMdbNtcSocket::GetErrorInfo(int iErrCode)
        {
            TMdbNtcStringBuffer sRet;
#ifdef OS_WINDOWS
            LPTSTR lpBuffer = NULL;
            DWORD dwLength = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,   iErrCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
                (LPTSTR) &lpBuffer, 0, NULL);
            if(dwLength == 0)
            {
                sRet = "FormatMessage Netive Error";
            }
            else
            {
                sRet.Assign(lpBuffer, dwLength);
                LocalFree(lpBuffer);
            }
#else
            sRet = strerror(iErrCode);
#endif
            return sRet;
        }

        bool TMdbNtcSocket::Close()
        {
            bool bRet = 0;
            if(m_uiSocketID != MDB_NTC_INVALID_SOCKET)
            {
                bRet = Close( m_uiSocketID );
                m_uiSocketID = MDB_NTC_INVALID_SOCKET;
            }    
            return bRet;
        }

        bool TMdbNtcSocket::GetLocalAddrInfo(QuickMDB_SOCKET uiSocketID, sockaddr_in& oAddrInfo)
        {
            
#if defined OS_HP || defined OS_WINDOWS
            int len = sizeof(oAddrInfo);
            int iRet = getsockname((int)uiSocketID, (struct sockaddr *)&oAddrInfo, &len);
#else
            unsigned int len = sizeof(oAddrInfo);
            int iRet = getsockname((int)uiSocketID, (struct sockaddr *)&oAddrInfo, (socklen_t*)&len);
#endif
            return iRet==0?true:false;
        }

        MDB_UINT32 TMdbNtcSocket::GetLocalPort(QuickMDB_SOCKET uiSocketID)
        {
            struct sockaddr_in oAddrInfo;
            if(!GetLocalAddrInfo(uiSocketID, oAddrInfo)) return 0;
            else return ntohs(oAddrInfo.sin_port);
        }

        TMdbNtcStringBuffer TMdbNtcSocket::GetLocalAddrInfo(QuickMDB_SOCKET uiSocketID)
        {
            struct sockaddr_in oAddrInfo;
            if(!GetLocalAddrInfo(uiSocketID, oAddrInfo)) return "";
        #ifdef OS_WINDOWS
            return inet_ntoa(oAddrInfo.sin_addr);
        #else
            TMdbNtcStringBuffer sRet(128);
            inet_ntop(AF_INET, &oAddrInfo.sin_addr, sRet.GetBuffer(), 128);
            sRet.UpdateLength();
            return sRet;
        #endif
        }

        bool TMdbNtcSocket::GetRemoteAddrInfo(QuickMDB_SOCKET uiSocketID, sockaddr_in& oAddrInfo)
        {
#if defined OS_HP || defined OS_WINDOWS
            int len = sizeof(oAddrInfo);
            int iRet = getpeername((int)uiSocketID, (struct sockaddr *)&oAddrInfo, &len);
#else
            unsigned int len = sizeof(oAddrInfo);
            int iRet = getpeername((int)uiSocketID, (struct sockaddr *)&oAddrInfo, (socklen_t*)&len);
#endif
            //if(iRet != 0) TADD_WARNING("getpeername failed, errno=%d, errmsg=[%s].",errno, strerror(errno));
            
            return iRet==0?true:false;
        }

        MDB_UINT32 TMdbNtcSocket::GetRemotePort(QuickMDB_SOCKET uiSocketID)
        {
            struct sockaddr_in oAddrInfo;
            if(!GetRemoteAddrInfo(uiSocketID, oAddrInfo)) return 0;
            else return ntohs(oAddrInfo.sin_port);
        }

        TMdbNtcStringBuffer TMdbNtcSocket::GetRemoteAddrInfo(QuickMDB_SOCKET uiSocketID)
        {
            struct sockaddr_in oAddrInfo;
            if(!GetRemoteAddrInfo(uiSocketID, oAddrInfo)) return "";
        #ifdef OS_WINDOWS
            return inet_ntoa(oAddrInfo.sin_addr);
        #else
            TMdbNtcStringBuffer sRet(128);
            inet_ntop(AF_INET, &oAddrInfo.sin_addr, sRet.GetBuffer(), 128);
            sRet.UpdateLength();
            return sRet;
        #endif
        }

        bool TMdbNtcSocket::SetBlockFlag(QuickMDB_SOCKET fd, bool bBlock)
        {
            if(bBlock)
            {        
#ifndef OS_WINDOWS
                int ul = 0;
                ioctl((int)fd, FIONBIO, &ul);
#else
                u_long ul = 0;
                ioctlsocket(fd, FIONBIO, &ul);
#endif
            }
            else
            {        
#ifndef OS_WINDOWS
                int ul = 1;
                ioctl((int)fd, FIONBIO, &ul);
#else
                u_long ul = 1;
                int iRet = ioctlsocket(fd, FIONBIO, &ul);
                iRet = iRet ? errno : 0;
#endif
            }
            return true;
        }

        bool TMdbNtcSocket::SetSendBufSize(QuickMDB_SOCKET fd, int iSendBufSize)
        {
            int iRet = 0;
            int sndbuf = iSendBufSize;   
#if defined OS_HP || defined OS_WINDOWS
            int sndbufsize= sizeof(int); 
#else
            unsigned int sndbufsize= sizeof(unsigned int); 
#endif
#ifdef OS_LINUX
            sndbuf = iSendBufSize/2;//linux下，setsockopt传入的表示缓冲区大小的参数是实际大小的1/2 
#endif    
            if(setsockopt((int)fd,SOL_SOCKET,SO_SNDBUF,(const char*)&sndbuf,sndbufsize) == 0)
            {
                return true;
            }
            else
            {
                TADD_WARNING( "Set send buffer size:%d failed", iSendBufSize);
                iRet = TMdbNtcSocket::GetLastError();
            }
            return iRet==0?true:false;
        }

        bool TMdbNtcSocket::SetRecvBufSize(QuickMDB_SOCKET fd, int iRecvBufSize)
        {
            int iRet = 0;
            int rcvbuf = iRecvBufSize;
#if defined OS_HP || defined OS_WINDOWS
            int rcvbufsize= sizeof(int); 
#else
            unsigned int rcvbufsize= sizeof(unsigned int); 
#endif
#ifdef OS_LINUX
            rcvbuf = iRecvBufSize/2;//linux下，setsockopt传入的表示缓冲区大小的参数是实际大小的1/2
#endif
            if(setsockopt((int)fd,SOL_SOCKET,SO_RCVBUF,(char*)&rcvbuf,rcvbufsize) == 0)
            {                
                return false;
            }
            else
            {
                iRet = TMdbNtcSocket::GetLastError();
                TADD_WARNING( "Set recv buffer size:%d failed", iRecvBufSize);
            }
            return iRet==0?true:false;
        }

        bool TMdbNtcSocket::GetSendBufSize(QuickMDB_SOCKET fd, int& iSendBufSize)
        {
            int iRet = 0;
#if defined OS_HP || defined OS_WINDOWS
            int len = sizeof(iSendBufSize);
            iRet = getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&iSendBufSize, &len);
#else
            unsigned int len = sizeof(iSendBufSize);
            iRet = getsockopt((int)fd, SOL_SOCKET, SO_SNDBUF, (int*)&iSendBufSize, (socklen_t*)&len);
#endif
            return iRet==0?true:false;
        }

        bool TMdbNtcSocket::GetRecvBufSize(QuickMDB_SOCKET fd, int& iRecvBufSize)
        {
            int iRet = 0;
#if defined OS_HP || defined OS_WINDOWS
            int len=sizeof(iRecvBufSize);
            iRet = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&iRecvBufSize, &len);
#else
            unsigned int len=sizeof(iRecvBufSize);
            iRet = getsockopt((int)fd, SOL_SOCKET, SO_RCVBUF, (int*)&iRecvBufSize, (socklen_t*)&len);
#endif
            return iRet==0?true:false;
        }

        bool TMdbNtcSocket::SetSocketLinger(QuickMDB_SOCKET fd, unsigned int uiLingerTime /* = 30 */)
        {
            struct linger so_linger;
            so_linger.l_onoff = 1;
            so_linger.l_linger = (int)uiLingerTime;
            if(0 == setsockopt((int)fd,
                SOL_SOCKET,
                SO_LINGER,
                (const char*)&so_linger,
                sizeof(so_linger)))
            {
                return true;
            }
            else
            {
                return false;//TMdbNtcSocket::GetLastError();
            }
        }

        bool TMdbNtcSocket::Send(QuickMDB_SOCKET fd, const char* pszBuffer, int& iSize, int iMilliSeconds /* = -1 */)
        {
            if( pszBuffer == NULL || iSize ==0)
            {
                TADD_WARNING( "OS_SocketSend() input parameter is error" );
                return false;//ERROR_SEND_PARAMETER;
            }
            int iRet = 0;
            if(iMilliSeconds == -1)
            {
                iSize = (int)send((int)fd, pszBuffer, (MDB_UINT32)iSize, 0);
                if(iSize < 0)
                {
                    return false;
                    //iRet = TMdbNtcSocket::GetLastError();
                    //MDB_NTC_ADD_GLOBAL_WARN_TF( "OS_SocketSend() is fail, errno=%d", iRet);
                }
            }
            else
            {
                int iTotalSendBytes = 0;
                fd_set wset;
                FD_ZERO(&wset);
                FD_SET(fd, &wset);
                struct timeval tVal, tZero;
                tVal.tv_sec = iMilliSeconds/1000; 
                tVal.tv_usec = (iMilliSeconds*1000)%1000000;
                memset(&tZero, 0x00, sizeof(tZero));
                while(1)
                {
                    int iReady = select((int)fd+1, NULL, &wset, NULL, iTotalSendBytes==0?&tVal:&tZero);
                    if(iReady == 0)//超时没有数据
                    {
                        break;
                    }
                    else if(iReady == -1)
                    {
                        iRet = TMdbNtcSocket::GetLastError();
                        break;
                    }
                    else
                    {
#ifndef OS_WINDOWS
                        int iSendBytes = (int)send((int)fd, (void *)(pszBuffer+iTotalSendBytes), (MDB_UINT32)(iSize-iTotalSendBytes), 0);
#else
                        int iSendBytes = send(fd, (const char *)(pszBuffer+iTotalSendBytes), iSize-iTotalSendBytes, 0);
#endif
                        if(iSendBytes <= 0)
                        {
                            iRet = TMdbNtcSocket::GetLastError();
                            break;
                        }
                        else
                        {
                            iTotalSendBytes += iSendBytes;
                        }
                        //如果全部字节都发送完，或者超时时间为0，则直接返回
                        if(iTotalSendBytes == iSize || iMilliSeconds == 0)
                        {
                            break;
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
                iSize = iTotalSendBytes;
            }
            return iRet==0?true:false;
        }

        bool TMdbNtcSocket::Recv(QuickMDB_SOCKET fd, char* pszBuffer, int& iSize, int iMilliSeconds /* = -1 */)
        {
            if( pszBuffer == NULL || iSize == 0 )
            {
                TADD_WARNING( "OS_SocketRecv() input parameter is error" );
                return false;//ERROR_SEND_PARAMETER;
            }
            int iRet = 0;
            if(iMilliSeconds == -1)
            {
                iSize = (int)recv((int)fd , pszBuffer, (MDB_UINT32)iSize, 0);
                if(iSize < 0)
                {
                    return false;
                    //iRet = TMdbNtcSocket::GetLastError();
                    //MDB_NTC_ADD_GLOBAL_WARN_TF( "OS_SocketRecv() is fail, errno=%d", iRet);
                }
            }
            else
            {
                int iTotalRecvBytes = 0;
                fd_set rset;
                FD_ZERO(&rset);
                FD_SET(fd, &rset);
                struct timeval tVal, tZero;
                tVal.tv_sec = iMilliSeconds/1000; 
                tVal.tv_usec = (iMilliSeconds*1000)%1000000;
                memset(&tZero, 0x00, sizeof(tZero));
                while(1)
                {
                    int iReady = select((int)fd+1, &rset, NULL, NULL, iTotalRecvBytes==0?&tVal:&tZero);
                    if(iReady == 0)//超时没有数据
                    {
                        break;
                    }
                    else if(iReady == -1)
                    {
                        iRet = TMdbNtcSocket::GetLastError();
                        break;
                    }
                    else
                    {
                        int iRecvBytes = (int)recv((int)fd, pszBuffer+iTotalRecvBytes, (MDB_UINT32)(iSize-iTotalRecvBytes),0);            
                        if(iRecvBytes < 0)
                        {
                            iRet = TMdbNtcSocket::GetLastError();
                            break;
                        }
                        else if(iRecvBytes == 0)//没有接收到数据
                        {
                            break;
                        }
                        else
                        {
                            iTotalRecvBytes += iRecvBytes;
                        }
                        //如果全部字节都发送完，或者超时时间为0，则直接返回
                        if(iTotalRecvBytes == iSize || iTotalRecvBytes == 0)
                        {
                            break;
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
                iSize = iTotalRecvBytes;
            }
            return iRet==0?true:false;
        }

        bool TMdbNtcSocket::Close(QuickMDB_SOCKET fd)
        {
            int iRet = 0;
#ifndef SD_BOTH
#define SD_BOTH 0x02
#endif
            iRet = shutdown((int)fd, SD_BOTH);
#ifndef OS_WINDOWS
            iRet = close((int)fd);
#else
            iRet = closesocket(fd);
#endif
            return iRet==0?true:false;
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcClientSocket, TMdbNtcSocket);
        bool TMdbNtcClientSocket::Connect(QuickMDB_SOCKET& fd, const char* pszRemote, int iPort, int iMilliSeconds /* = -1 */)
        {
            const char* pszIP = pszRemote;            
            TMdbNtcStringBuffer sIP;
            if(pszRemote && *pszRemote && !TMdbNtcStrFunc::IsIPAddress(pszRemote))
            {
                struct hostent *host_entry = gethostbyname(pszRemote);
                if(host_entry)
                {
                    sIP.Snprintf(32, "%d.%d.%d.%d",
                        (host_entry->h_addr_list[0][0]&0x00ff),
                        (host_entry->h_addr_list[0][1]&0x00ff),
                        (host_entry->h_addr_list[0][2]&0x00ff),
                        (host_entry->h_addr_list[0][3]&0x00ff));
                    pszIP = sIP.c_str();
                }
                else
                {
                    return false;
                }
            }
            if(pszIP == NULL) pszIP = "127.0.0.1";
            struct sockaddr_in ServerAddr;
            memset(&ServerAddr, 0, sizeof(ServerAddr));
            ServerAddr.sin_family         = AF_INET;
            ServerAddr.sin_addr.s_addr    = inet_addr(pszIP);
            ServerAddr.sin_port           = htons( (MDB_UINT16)iPort );
            int iRet = 0;
            fd = (MDB_UINT32)socket(AF_INET, SOCK_STREAM, 0);
            if(fd == MDB_NTC_INVALID_SOCKET)
            {
                iRet = TMdbNtcSocket::GetLastError();
                TADD_WARNING( "create socket failed, errno=%d", iRet);
                return false;
            }
            do 
            {
                SetBlockFlag(fd, false);
                iRet = connect((int)fd, (struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
                if (iRet < 0)
                {
                    iRet = TMdbNtcSocket::GetLastError();
                    if (iRet != MDB_NTC_ZS_EINPROGRESS && iRet != MDB_NTC_ZS_EWOULDBLOCK) 
                    {
                        TADD_WARNING("connect socket failed, errno=%d,%s", iRet, GetErrorInfo(iRet).c_str());
                        iRet = -1;
                        break;
                    }
                    else
                    {
                        iRet = 0;
                    }
                }
                else if (iRet == 0)
                {
                    break;//一般是同一台主机调用，会返回 0 
                }
                fd_set rset, wset;        
                FD_ZERO(&rset); 
                FD_SET(fd, &rset);
                wset = rset;  // 这里会做 block copy
                struct timeval tval;
                if(iMilliSeconds < 0)
                {
                    iMilliSeconds = 3000;//默认3秒
                }
                tval.tv_sec = iMilliSeconds/1000; 
                tval.tv_usec = (iMilliSeconds*1000)%1000000;
                
                // 如果nsec 为0，将使用缺省的超时时间，即其结构指针为 NULL 
                // 如果tval结构中的时间为0，表示不做任何等待，立刻返回 
                iRet = select((int)fd+1, &rset, &wset, NULL, &tval);
                if(iRet == 0)
                {
                    iRet = MDB_NTC_ZS_ETIMEDOUT;
                    TADD_WARNING( "select error: timeout");
                    break;
                } 
                
                if(FD_ISSET(fd, &rset) || FD_ISSET(fd, &wset))
                { 
#if defined OS_HP || defined OS_WINDOWS
                    int len = sizeof(int); 
#else
                    socklen_t len = sizeof(socklen_t);
#endif
                    // 如果连接成功，此调用返回 0 
                    if (getsockopt((int)fd, SOL_SOCKET, SO_ERROR, (char*)&iRet, &len) < 0)
                    {
                        iRet = -1;
                    }
                } 
                else
                {
                    iRet = -1;
                    TADD_WARNING( "select error: iSocketId  not set");
                }
            } while (0);         
            if (iRet != 0) 
            {
                Close(fd);
                fd = MDB_NTC_INVALID_SOCKET;
                if(iRet > 0)
                {
#ifdef OS_WINDOWS
                    WSASetLastError(iRet);
#else
                    errno = iRet;
#endif
                }
                return false;
            }
            else
            {
                SetBlockFlag(fd, true);//设置为阻塞模式
                return true;
            }
        }

        bool TMdbNtcClientSocket::Disconnect()
        {
            return Close();
        }

        bool TMdbNtcClientSocket::Reconnect(int iMilliSeconds /* = -1 */)
        {
            return Connect(m_uiSocketID, m_sRemoteHost.c_str(), m_iRemotePort, iMilliSeconds);
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcServerSocket, TMdbNtcSocket);
        TMdbNtcServerSocket::TMdbNtcServerSocket()
        {
            m_iServerPort = 0;
        }

        bool TMdbNtcServerSocket::Listen(QuickMDB_SOCKET& fd, const char* pszAddress, int iPort, bool bReuseAddr /* = true */)
        {
            int iRet = -1;
            const char* pszIP = pszAddress;            
            TMdbNtcStringBuffer sIP;
            if(pszAddress && *pszAddress && !TMdbNtcStrFunc::IsIPAddress(pszAddress))
            {
                struct hostent *host_entry = gethostbyname(pszAddress);
                if(host_entry)
                {
                    sIP.Snprintf(32, "%d.%d.%d.%d",
                        (host_entry->h_addr_list[0][0]&0x00ff),
                        (host_entry->h_addr_list[0][1]&0x00ff),
                        (host_entry->h_addr_list[0][2]&0x00ff),
                        (host_entry->h_addr_list[0][3]&0x00ff));
                    pszIP = sIP.c_str();
                }
                else
                {
                    return false;
                }
            }
            struct sockaddr_in myAddr;
            memset((char*)&myAddr, 0x00, sizeof(myAddr));//初始化
            myAddr.sin_family         = AF_INET;
            if(pszIP == NULL || *pszIP == '\0')
            {
                myAddr.sin_addr.s_addr = htonl( INADDR_ANY );
            }
            else
            {
                myAddr.sin_addr.s_addr = inet_addr( pszIP );
            }
            myAddr.sin_port           = htons( (MDB_UINT16)iPort );
            
            if((fd = (MDB_UINT32)socket(AF_INET,SOCK_STREAM,0)) == (MDB_UINT32)-1)
            {
                iRet = TMdbNtcSocket::GetLastError();
                TADD_WARNING( "create socket failed, errno=%d", iRet);
                return false;
            }
            do 
            {
				if(bReuseAddr)
				{
					int opt =  1;
					iRet = setsockopt((int)fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
					if(iRet != 0)
					{
						iRet = TMdbNtcSocket::GetLastError();
						TADD_WARNING( "set sockopt  failed, errno=%d", iRet);
						break;
					}
				}
                iRet = bind((int)fd, (struct sockaddr *)&myAddr, sizeof(myAddr));
                if(iRet != 0)
                {
                    iRet = TMdbNtcSocket::GetLastError();
                    TADD_WARNING( "bind socket failed, errno=%d", iRet);            
                    break;
                }
                iRet = listen((int)fd, FD_SETSIZE);
                if(iRet != 0)
                {
                    iRet = TMdbNtcSocket::GetLastError();
                    TADD_WARNING( "listen socket failed ,errno=%d", iRet);            
                    break;
                }
            } while (0);
            if(iRet != 0)
            {
                Close(fd);
                fd = MDB_NTC_INVALID_SOCKET;
                TMdbNtcSocket::SetLastError(iRet);
            }    
            return iRet==0?true:false;
        }

        bool TMdbNtcServerSocket::Accept(QuickMDB_SOCKET serv_fd, QuickMDB_SOCKET& client_fd, sockaddr_in* ptAddrInfo /* = NULL */, int iMilliSeconds /* = -1 */)
        {
            int iRet = 0;
            fd_set rset, wset; //为select准备数据
            struct timeval tval;
            tval.tv_sec = iMilliSeconds/1000; 
            tval.tv_usec = (iMilliSeconds*1000)%1000000;
            FD_ZERO(&rset);
            FD_SET(serv_fd, &rset);
            wset = rset;  // 这里会做 block copy 
            iRet = select((int)serv_fd+1, &rset, &wset, NULL, iMilliSeconds ==-1 ? NULL: &tval);
            //select 成功的话返回一个非零值
            switch (iRet) 
            {
            case 0:
                {
                    iRet = TMdbNtcSocket::GetLastError();
                    TADD_WARNING( "Accept failed due to time out !");
                    return  false ;//EINTR
                }
                break;
            default:
                int iLen = sizeof(struct sockaddr);
#if defined(OS_WINDOWS) || defined(OS_HP) //int accept(int s, void *addr, int *addrlen);
                client_fd = accept(serv_fd, (struct sockaddr*)ptAddrInfo, &iLen);
#else
                client_fd = (MDB_UINT32)accept((int)serv_fd, (struct sockaddr*)ptAddrInfo, (socklen_t*)&iLen);
#endif
                if(client_fd == (MDB_UINT32)-1)
                {
                    iRet = TMdbNtcSocket::GetLastError();
                    TADD_WARNING( "accept failed,error:%d,%s", iRet, GetErrorInfo(iRet).c_str());
                    return false;
                }
                else
                {
                    iRet = 0;
                }
            }
            return iRet==0?true:false;
        }
//    }
//}
