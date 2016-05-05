#include "Common/mdbPeerProactor.h"
#include "Common/mdbNtcEngine.h"
#include "Common/mdbPeerProtocol.h"
#include "Common/mdbPeerEvent.h"
#ifdef OS_WINDOWS
    #if _MSC_VER < 1300
        #include "./ws2tcpip.hxx"
    #else
        #include <ws2tcpip.h>
    #endif
#else
    #include <sys/socket.h>
#endif
#ifdef OS_SUN
    #include <sys/filio.h>
#endif
//namespace QuickMDB
//{
//using namespace QuickMDB;
#ifdef OS_WINDOWS
static int __mdb_stream_socketpair(addrinfo* ai,QuickMDB_SOCKET sock[2])
{
    QuickMDB_SOCKET listener,client = MDB_NTC_INVALID_SOCKET,server = MDB_NTC_INVALID_SOCKET;
    int opt = 1;

    listener = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (MDB_NTC_INVALID_SOCKET==listener)
        goto fail;

    setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt, sizeof(opt)); 

    if(MDB_NTC_SOCKET_ERROR==bind(listener,ai->ai_addr,ai->ai_addrlen))
        goto fail;

    if (MDB_NTC_SOCKET_ERROR==getsockname(listener,ai->ai_addr,(int*)&ai->ai_addrlen))
        goto fail;

    if(MDB_NTC_SOCKET_ERROR==listen(listener,SOMAXCONN))
        goto fail;

    client = socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
    if (MDB_NTC_INVALID_SOCKET==client)
        goto fail;

    if (MDB_NTC_SOCKET_ERROR==connect(client,ai->ai_addr,ai->ai_addrlen))
        goto fail;

    server = accept(listener,0,0);
    if (MDB_NTC_INVALID_SOCKET==server)
        goto fail;

    closesocket(listener);
    sock[0] = client, sock[1] = server;
    return 0;

fail:
    if(MDB_NTC_INVALID_SOCKET!=listener)
        closesocket(listener);
    if (MDB_NTC_INVALID_SOCKET!=client)
        closesocket(client);
    return -1;
}

static int __mdb_dgram_socketpair(struct addrinfo* ai,QuickMDB_SOCKET sock[2])
{
    QuickMDB_SOCKET client = MDB_NTC_INVALID_SOCKET,server=MDB_NTC_INVALID_SOCKET;
    struct addrinfo addr,*res = NULL;
    const char* address;
    int opt = 1;

    server = socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
    if (MDB_NTC_INVALID_SOCKET==server)
        goto fail;

    setsockopt(server,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt, sizeof(opt)); 

    if(MDB_NTC_SOCKET_ERROR==bind(server,ai->ai_addr,ai->ai_addrlen))
        goto fail;

    if (MDB_NTC_SOCKET_ERROR==getsockname(server,ai->ai_addr,(int*)&ai->ai_addrlen))
        goto fail;

    client = socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);
    if (MDB_NTC_INVALID_SOCKET==client)
        goto fail;

    memset(&addr,0,sizeof(addr));
    addr.ai_family = ai->ai_family;
    addr.ai_socktype = ai->ai_socktype;
    addr.ai_protocol = ai->ai_protocol;

    if (AF_INET6==addr.ai_family)
        address = "0:0:0:0:0:0:0:1";
    else
        address = "127.0.0.1";

    if (getaddrinfo(address,"0",&addr,&res))
        goto fail;

    setsockopt(client,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt, sizeof(opt)); 
    if(MDB_NTC_SOCKET_ERROR==bind(client,res->ai_addr,res->ai_addrlen))
        goto fail;

    if (MDB_NTC_SOCKET_ERROR==getsockname(client,res->ai_addr,(int*)&res->ai_addrlen))
        goto fail;

    if (MDB_NTC_SOCKET_ERROR==connect(server,res->ai_addr,res->ai_addrlen))
        goto fail;

    if (MDB_NTC_SOCKET_ERROR==connect(client,ai->ai_addr,ai->ai_addrlen))
        goto fail;

    freeaddrinfo(res);
    sock[0] = client, sock[1] = server;
    return 0;

fail:
    if (MDB_NTC_INVALID_SOCKET!=client)
        closesocket(client);
    if (MDB_NTC_INVALID_SOCKET!=server)
        closesocket(server);
    if (res)
        freeaddrinfo(res);
    return -1;
}

static int mdb_win32_socketpair(int family,int type,int protocol,unsigned int sock[2])
{
    const char* address;
    struct addrinfo addr,*ai;
    int ret = -1;

    memset(&addr,0,sizeof(addr));
    addr.ai_family = family;
    addr.ai_socktype = type;
    addr.ai_protocol = protocol;

    if (AF_INET6==family)
        address = "0:0:0:0:0:0:0:1";
    else
        address = "127.0.0.1";

    if (0==getaddrinfo(address,"0",&addr,&ai))
    {
        if (SOCK_STREAM==type)
            ret = __mdb_stream_socketpair(ai,sock);
        else if(SOCK_DGRAM==type)
            ret = __mdb_dgram_socketpair(ai,sock); 
        freeaddrinfo(ai);
    }
    return ret;
}
#endif

int mdb_ntc_socket_pair(int type, int protocol, unsigned int sock[2])
{
#ifndef WIN32
    return socketpair(AF_UNIX, type, protocol, (int*)sock);
#else
    return mdb_win32_socketpair(AF_INET, type,protocol,sock);
#endif
}

bool TMdbSignalProactor::Start()
{
    return true;
}

bool TMdbSignalProactor::Stop()
{
    return true;
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbPeerProactor, TMdbNtcThread);
TMdbPeerProactor::TMdbPeerProactor()
{
    intr_fd[0] = intr_fd[1] = MDB_NTC_INVALID_SOCKET;
    m_pNtcEngine = NULL;
}

TMdbPeerProactor::~TMdbPeerProactor()
{
    if(intr_fd[0] != MDB_NTC_INVALID_SOCKET)
    {
        TMdbNtcSocket::Close(intr_fd[0]);
    }
    if(intr_fd[1] != MDB_NTC_INVALID_SOCKET)
    {
        TMdbNtcSocket::Close(intr_fd[1]);
    }
}

bool TMdbPeerProactor::Init(TMdbNtcEngine* pNtcEngine)
{
    m_pEventDispatcher = pNtcEngine;
    m_pNtcEngine = pNtcEngine;
    int iRet = mdb_ntc_socket_pair(SOCK_STREAM, 0, intr_fd);//socket pair用于异步通知到前摄器
    if (iRet != 0)
    {
        mdb_ntc_errstr.Snprintf(1024, "error:%u,%s", TMdbNtcSocket::GetLastError(), TMdbNtcSocket::GetLastErrorInfo().c_str());
        TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
        return false;
    }
    TADD_DETAIL("mdb_ntc_socket_pair intrfd[%d,%d]ok", intr_fd[0], intr_fd[1]);
    TMdbNtcSocket::SetBlockFlag(intr_fd[1], false);
    int iBufferSize = 1024*1024;//缓冲大小
    TMdbNtcSocket::SetSendBufSize(intr_fd[0],iBufferSize);
    TMdbNtcSocket::SetRecvBufSize(intr_fd[0],iBufferSize);
    TMdbNtcSocket::SetSendBufSize(intr_fd[1],iBufferSize);
    TMdbNtcSocket::SetRecvBufSize(intr_fd[1],iBufferSize);
    return true;
}

bool TMdbPeerProactor::Start()
{
    return true;
}

bool TMdbPeerProactor::Stop()
{
    Cancel();//设置退出flag
    Wakeup();//唤醒前摄器，前摄器便会检测到cancel
    Wait();//等待线程退出
    return true;
}

void TMdbPeerProactor::Wakeup()
{
    if(intr_fd[0] != MDB_NTC_INVALID_SOCKET)
    {
        ASYNC_MODIFY_MONITOR oMonitor;
        //向intr_fd写入信息，中断select
        int iRet = (int)send((int)intr_fd[0], (char*)&oMonitor, sizeof(oMonitor), 0);
        if(iRet < 0)
        {
            iRet = TMdbNtcSocket::GetLastError();
            mdb_ntc_errstr.Snprintf(1024, "send socket[%d] iRet:%d, error:%s", intr_fd[0],  iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
            TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
        }
    }
}

bool TMdbPeerProactor::AsyncAddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(intr_fd[0] != MDB_NTC_INVALID_SOCKET)
    {
        TADD_DETAIL("async add monitor:fd[%u],events[%x]", fd, events);
        ASYNC_MODIFY_MONITOR oMonitor;
        oMonitor.bAddMonitor = true;
        oMonitor.fd = fd;
        oMonitor.ntc_events = events;
        //向intr_fd写入信息，中断select
        int iRet = (int)send((int)intr_fd[0], (char*)&oMonitor, sizeof(oMonitor), 0);
        if(iRet < 0)
        {
            iRet = TMdbNtcSocket::GetLastError();
            mdb_ntc_errstr.Snprintf(1024, "send socket[%d] iRet:%d, error:%s", intr_fd[0],  iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
            TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

bool TMdbPeerProactor::AsyncRemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(intr_fd[0] != MDB_NTC_INVALID_SOCKET)
    {
        TADD_DETAIL("async remove monitor:fd[%u],events[%x]", fd, events);
        ASYNC_MODIFY_MONITOR oMonitor;
        oMonitor.bAddMonitor = false;
        oMonitor.fd = fd;
        oMonitor.ntc_events = events;
        //向intr_fd写入信息，中断select
        int iRet = (int)send((int)intr_fd[0], (char*)&oMonitor, sizeof(oMonitor), 0);
        if(iRet < 0)
        {
            iRet = TMdbNtcSocket::GetLastError();
            mdb_ntc_errstr.Snprintf(1024, "send socket[%d] iRet:%d, error:%s", intr_fd[0],  iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
            TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

void TMdbPeerProactor::Cleanup()
{

}

int mdb_ntc_get_need_read_size(QuickMDB_SOCKET fd)
{
    int iRet = 0;
#ifdef OS_WINDOWS
    unsigned long iSize = 0;
    iRet = ioctlsocket(fd, FIONREAD, &iSize);
#else
    int iSize = 0;
    iRet = ioctl((int)fd, FIONREAD, &iSize);
#endif    
    if(iRet != 0)
    {
        return -1;
    }
    else
    {
        return iSize;
    }
}

bool TMdbPeerProactor::AddNewClient(QuickMDB_SOCKET new_fd, TMdbServerInfo* pServerInfo)
{
    if(!m_pNtcEngine->CheckNewClient(new_fd, pServerInfo)) return false;
    TMdbSharedPtr<TMdbPeerInfo> pRetPeerInfo = m_pNtcEngine->AddPeerInfo(new_fd, pServerInfo);
    if(pRetPeerInfo)
    {
        /*
        //判断是否已经达到了最大连接数，达到则移除server的监听
        if(m_pPeerHelper->GetMaxPeerNo() == m_pPeerHelper->GetPeerCount())
        {
            RemoveServerMonitor(i, false);
        }
        */
        return true;
    }
    else
    {
        mdb_ntc_errstr = "peer count limit reached";
        TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
        //抛出一个错误事件出去
        TMdbErrorEvent* pErrorEvent = new TMdbErrorEvent;
        pErrorEvent->sErrorInfo.Snprintf(128, "remote[%s:%u] is refused, because of peer count limit reached.",
            TMdbNtcSocket::GetRemoteAddrInfo(new_fd).c_str(), TMdbNtcSocket::GetRemotePort(new_fd));
        m_pEventDispatcher->Dispatch(pErrorEvent);
        return false;
    }
}

bool TMdbPeerProactor::OnServerAccept(TMdbServerInfo* pServerInfo)
{
    TADD_DETAIL("[%u]-wait accept", pServerInfo->GetSocketID());
    bool bMonitorRead = true;///< 是否需要继续监听可读
    do
    {
        QuickMDB_SOCKET new_fd = (QuickMDB_SOCKET)accept((int)pServerInfo->GetSocketID(), NULL, NULL);//server fd已经被设置为非阻塞
        TADD_DETAIL("new_fd[%d] ", new_fd);
        if (new_fd == MDB_NTC_INVALID_SOCKET)
        {
            int iErrorCode = TMdbNtcSocket::GetLastError();
            if (iErrorCode != EAGAIN && iErrorCode != MDB_NTC_ZS_EWOULDBLOCK)
            {
                pServerInfo->RemoveEventMonitor(MDB_NTC_PEER_EV_READ_FLAG);
                mdb_ntc_errstr.Snprintf(1024, "accept failed!error:%u,%s", iErrorCode, TMdbNtcSocket::GetErrorInfo(iErrorCode).c_str());
                TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
                TMdbErrorEvent* pErrorEvent = new TMdbErrorEvent;
                pErrorEvent->sErrorInfo = mdb_ntc_errstr;
                m_pEventDispatcher->Dispatch(pErrorEvent);
                bMonitorRead = false;
            }
            break;
        }
        else if(!AddNewClient(new_fd, pServerInfo))
        {
            TMdbNtcSocket::Close(new_fd);//关闭链接
            //RemoveServerMonitor(i, false);//移除服务端的监听
        }
    }while (1);
    return true;
}

bool TMdbPeerProactor::OnPeerRecv(TMdbPeerInfo* pPeerInfo)
{
    TADD_DETAIL("[%u]-wait read", pPeerInfo->GetSocketID());
    if(pPeerInfo == NULL || pPeerInfo->IsShutdown())
    {
        pPeerInfo->RemoveEventMonitor(MDB_NTC_PEER_EV_READ_FLAG);
        return false;
    }
    bool bMonitorRead = false;///< 是否需要继续监听可读
    pPeerInfo->SetPeerState(MDB_NTC_PEER_RECV_STATE);
    QuickMDB_SOCKET fd = pPeerInfo->GetSocketID();
    do
    {
        int iBufferSize = mdb_ntc_get_need_read_size(fd);
        if(iBufferSize <= 0)
        {
            //判断缓冲区无数据，是否是因为对方断开的原因
            char c;
            iBufferSize = (int)recv((int)fd, &c, 1, MSG_PEEK);//非阻塞,正常时会返回-1且errno为EAGAIN或MDB_NTC_ZS_EWOULDBLOCK
            if(iBufferSize <= 0)
            {
                if(iBufferSize == 0)//表示非阻塞连接正常断开
                {
                    pPeerInfo->Disconnect("remote normal disconnect", true);
                }
                else//说明没有数据,非阻塞,正常时会返回-1且errno为EAGAIN或MDB_NTC_ZS_EWOULDBLOCK
                {
                    int iErrorCode = TMdbNtcSocket::GetLastError();
                    if(!(iErrorCode == EAGAIN || iErrorCode == MDB_NTC_ZS_EWOULDBLOCK || iErrorCode == EINTR))
                    {
                        pPeerInfo->Disconnect(TMdbNtcSocket::GetErrorInfo(iErrorCode), true);
                    }
                    else
                    {
                        bMonitorRead = true;//需要继续监听收取
                    }
                }
                break;
            }
            else
            {
                continue;
            }
        }
        //检测流量限制
        MDB_UINT32 uiAllowRecvBytes = m_pNtcEngine->GetPeerAllowRecvBytes(pPeerInfo, (MDB_UINT32)iBufferSize);
        if(iBufferSize != (int)uiAllowRecvBytes)
        {
            iBufferSize = (int)uiAllowRecvBytes;
            pPeerInfo->RemoveEventMonitor(MDB_NTC_PEER_EV_READ_FLAG);
            TMdbTrafficCtrl::SuspendPeerTraffic(m_pNtcEngine->FindPeerInfo(pPeerInfo->pno), MDB_NTC_PEER_EV_READ_FLAG);
            if(uiAllowRecvBytes == 0)//暂时因流量限制，不允许接收
            {
                break;
            }
        }
        TMdbPacketInfo* pPacketInfo = NULL;
        TMdbMsgInfo* pMsgInfo = NULL;
        if(pPeerInfo->pProtocol == NULL)//如果没有绑定协议，则需要将收到的数据包直接生成消息事件
        {
            pMsgInfo = new TMdbMsgInfo((MDB_UINT32)iBufferSize);
            pPacketInfo = &pMsgInfo->oPacketInfo;
        }
        else
        {
            pPacketInfo = new TMdbPacketInfo((MDB_UINT32)iBufferSize);
        }        
        iBufferSize = (int)recv((int)fd, pPacketInfo->GetBuffer(), (size_t)iBufferSize, 0);
        if(iBufferSize > 0)
        {
            pPeerInfo->GetTrafficInfo()->AddRecvBytes((MDB_UINT32)iBufferSize);
            m_pNtcEngine->AddTotalRecvBytes((MDB_UINT32)iBufferSize);
            pPacketInfo->SetLength((MDB_UINT32)iBufferSize);
            pPacketInfo->GetBuffer()[iBufferSize] = '\0';
            if(pMsgInfo)
            {
                GetEventDispatcher()->Dispatch(new TMdbRecvMsgEvent(pPeerInfo, pMsgInfo));
            }
            else
            {
                pPeerInfo->oRecvPackets.AddPacket(pPacketInfo);
                //使用对应的协议检查数据包是否完整
                pPeerInfo->CheckRecvPackets();
                if(pPeerInfo->IsShutdown())
                {
                    break;
                }
            }
        }
        else
        {
            if(iBufferSize == 0)//表示非阻塞连接正常断开
            {
                pPeerInfo->Disconnect("remote normal disconnect", true);
            }
            else//说明没有数据,非阻塞,正常时会返回-1且errno为EAGAIN或MDB_NTC_ZS_EWOULDBLOCK
            {
                int iErrorCode = TMdbNtcSocket::GetLastError();
                if(!(iErrorCode == EAGAIN || iErrorCode == MDB_NTC_ZS_EWOULDBLOCK || iErrorCode == EINTR))
                {
                    pPeerInfo->Disconnect(TMdbNtcSocket::GetErrorInfo(iErrorCode), true);
                }
                else
                {
                    bMonitorRead = true;//需要继续接听收取
                }
            }
            if(pMsgInfo)
            {
                delete pMsgInfo;
                pMsgInfo = NULL;
            }
            else
            {
                delete pPacketInfo;
                pPacketInfo = NULL;
            }            
            break;
        }
    } while (1);
    return bMonitorRead;
}

bool TMdbPeerProactor::OnPeerSend(TMdbPeerInfo* pPeerInfo)
{
    TADD_DETAIL("[%u]-wait write", pPeerInfo->GetSocketID());
    if(pPeerInfo == NULL || pPeerInfo->IsShutdown())
    {
        pPeerInfo->RemoveEventMonitor(MDB_NTC_PEER_EV_WRITE_FLAG);
        return false;
    }
    TMdbSendPackets& oSendPackets = pPeerInfo->oSendPackets;
    if(oSendPackets.IsEmpty()) return false;
    bool bMonitorWrite = false;///< 是否需要继续监听写
    pPeerInfo->SetPeerState(MDB_NTC_PEER_WRITE_STATE);
    TMdbPacketInfo* pCurPacketInfo = NULL;
    do
    {
        pCurPacketInfo = static_cast<TMdbPacketInfo*>(oSendPackets.Front());
        if(pCurPacketInfo == NULL)
        {
            pPeerInfo->Lock();
            if(pPeerInfo->oSendPackets.IsEmpty())
            {
                pPeerInfo->RemoveEventMonitor(MDB_NTC_PEER_EV_WRITE_FLAG, false);
            }
            else
            {
                bMonitorWrite = true;
            }
            pPeerInfo->Unlock();
            break;
        }
        int iLeftSendBytes = (int)(pCurPacketInfo->GetLength())-oSendPackets.iSendBytes;
        if(iLeftSendBytes > 0)
        {
            int iSendBytes = (int)send((int)pPeerInfo->GetSocketID(),
                pCurPacketInfo->GetBuffer()+oSendPackets.iSendBytes, (size_t)iLeftSendBytes, 0);
            if(iSendBytes <= 0)
            {
                int iErrorCode = TMdbNtcSocket::GetLastError();
                //MDB_NTC_DEBUG("iRet[%d],%s, send:[%d]/[%d]", iRet, zxerror(iRet), iActualSendBytes, iSendBytes);//, pEventInfo->pcData+pEventInfo->iSendBytes);
                if(!(iErrorCode == EAGAIN || iErrorCode == MDB_NTC_ZS_EWOULDBLOCK || iErrorCode == EINTR))
                {
                    pPeerInfo->Disconnect(TMdbNtcSocket::GetErrorInfo(iErrorCode), true);
                }
                else
                {
                    bMonitorWrite = true;//说明发不动了，继续监听
                }
                break;
            }
            else
            {
                oSendPackets.iSendBytes += iSendBytes;
                iLeftSendBytes -= iSendBytes;
                pPeerInfo->GetTrafficInfo()->AddSendBytes((MDB_UINT32)iSendBytes);
                m_pNtcEngine->AddTotalSendBytes((MDB_UINT32)iSendBytes);
            }
        }
        if(iLeftSendBytes <= 0)
        {
            pPeerInfo->GetTrafficInfo()->AddSendMsgNum();
            TMdbPacketInfo* pTMdbPacketInfo= static_cast<TMdbPacketInfo*>(oSendPackets.Pop());
            oSendPackets.iSendBytes = 0;
            if ( pTMdbPacketInfo ) //实际上:pTMdbPacketInfo == pCurPacketInfo
            {
                delete pTMdbPacketInfo;
                pTMdbPacketInfo = NULL;
            }
            /*
            if(pCurPacketInfo->pSendThreadEvent)//处理完成后，触发事件通知当前sendmessage等待的线程
            {
                pCurPacketInfo->pSendThreadEvent->PulseEvent();
            }
            */
        }
        else//说明发不动了，需要继续监听可写
        {
            bMonitorWrite = true;
            break;
        }
    }while(1);
    return bMonitorWrite;
}

void TMdbPeerProactor::DealAsyncOperator()
{
    do
    {
        ASYNC_MODIFY_MONITOR oMonitor;
        int iRecvLength = (int)recv((int)intr_fd[1], (char*)&oMonitor, sizeof(oMonitor), 0);
        if(iRecvLength != sizeof(oMonitor))
        {
            break;
        }
        else if(oMonitor.fd != MDB_NTC_INVALID_SOCKET)
        {
            if(oMonitor.bAddMonitor)
            {
                AddEventMonitor(oMonitor.fd, oMonitor.ntc_events);
            }
            else
            {
                RemoveEventMonitor(oMonitor.fd, oMonitor.ntc_events);
                //如果是要彻底移除监听，且没有事件泵，则需要前摄器来分发Disconnect事件
                if(oMonitor.ntc_events == 0)
                {
                    TMdbNtcSocket* pSocket = m_pNtcEngine->FindSocketInfo(oMonitor.fd);
                    if(pSocket && !pSocket->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
                    {
                        m_pNtcEngine->Dispatch(new TMdbDisconnectEvent(static_cast<TMdbPeerInfo*>(pSocket)));//将事件分发下去
                    }
                }
            }
        }
    } while (1);
    if(this->TestCancel())
    {
        TADD_DETAIL("proactor begin exit.");
        Cleanup();
        Cancel();
    }
}

/*--------------------------------- begin select proactor------------------------------------------*/
MDB_ZF_IMPLEMENT_OBJECT(TMdbProactorSelect, TMdbPeerProactor);
TMdbProactorSelect::TMdbProactorSelect()
{
    rmax_fd = wmax_fd = 0;
    FD_ZERO(&all_rset);
    FD_ZERO(&all_wset);
    rcount = wcount = 0;
}

void TMdbProactorSelect::Cleanup()
{
    FD_ZERO(&all_rset);
    FD_ZERO(&all_wset);
    TMdbPeerProactor::Cleanup();
}

bool TMdbProactorSelect::AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(fd == MDB_NTC_INVALID_SOCKET)
    {
        return false;
    }
    TADD_DETAIL("AddEventMonitor fd[%d] event[%x]", fd, events);
    if(events == 0)
    {
        return true;
    }
    bool bUpdate = false;
    if(events&MDB_NTC_PEER_EV_READ_FLAG && !(FD_ISSET(fd, &all_rset)))
    {
        FD_SET(fd, &all_rset);
        if(fd > rmax_fd)
        {
            rmax_fd = fd;
        }
        ++rcount;
        bUpdate = true;
    }
    if(events&MDB_NTC_PEER_EV_WRITE_FLAG && !(FD_ISSET(fd, &all_wset)))
    {
        FD_SET(fd, &all_wset);
        if(fd > wmax_fd)
        {
            wmax_fd = fd;
        }
        ++wcount;        
        bUpdate = true;
    }
    return bUpdate;
}

bool TMdbProactorSelect::RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{     
    bool bUpdate = false;
    TADD_DETAIL("RemoveEventMonitor fd[%d] event[%x]", fd, events);
    if(events == 0)
    {
        events = MDB_NTC_PEER_EV_READ_FLAG|MDB_NTC_PEER_EV_WRITE_FLAG;
    }
    if(events&MDB_NTC_PEER_EV_READ_FLAG && FD_ISSET(fd, &all_rset))
    {
        FD_CLR(fd, &all_rset);     
        if(fd >= rmax_fd)
        {
            rmax_fd = fd-1;
        }
        --rcount;
        bUpdate = true;
    }
    if(events&MDB_NTC_PEER_EV_WRITE_FLAG && FD_ISSET(fd, &all_wset))
    {
        FD_CLR(fd, &all_wset);
        if(fd >= wmax_fd)
        {
            wmax_fd = fd-1;
        }
        --wcount;
        bUpdate = true;
    }
    return bUpdate;
}

int TMdbProactorSelect::Execute()
{
    int iRet = 0;
    if(m_pNtcEngine == NULL)
    {
        return -1;
    }
    AddEventMonitor(intr_fd[1], MDB_NTC_PEER_EV_READ_FLAG);
    QuickMDB_SOCKET max_fd = 0;
    fd_set rset, wset;
    TMdbPeerInfo* pPeerInfo = NULL;
    bool bNeedSyncSet = false;///< 是否需要同步套接字集合
    do
    {
        if(bNeedSyncSet)
        {
            DealAsyncOperator();
            bNeedSyncSet = false;
        }
        memcpy(&rset, &all_rset, sizeof(fd_set));
        memcpy(&wset, &all_wset, sizeof(fd_set));
        /*
        1．正常情况下返回就绪的文件描述符个数；
        2．经过了timeout时长后仍无设备准备好，返回值为0；
        3．如果select被某个信号中断，它将返回-1并设置errno为EINTR。
        4．如果出错，返回-1并设置相应的errno。 
        */
        max_fd = (rmax_fd>wmax_fd?rmax_fd:wmax_fd);
        TADD_DETAIL("rcount[%d],wcount[%d]", rcount, wcount);
        int iReadyCount = select((int)(max_fd+1), &rset, &wset, NULL, NULL);
        if(iReadyCount == 0)//超时没有数据
        {
            continue;
        }
        else if(iReadyCount == -1)
        {
            iRet = TMdbNtcSocket::GetLastError();
            if(iRet == EINTR)//被信号中断
            {
                iRet = 0;
                continue;
            }
            else
            {
                mdb_ntc_errstr.Snprintf(1024, "select failed!error:%u,%s", iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
                TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
                return iRet;
            }
        }
        TADD_DETAIL("select ret:%d", iReadyCount);
        //如果有事件的就是中断的fd，则中断目的已经达到，接收完buffer后，继续监听
        if(FD_ISSET(intr_fd[1], &rset))
        {
            bNeedSyncSet = true;
            if(iReadyCount == 1)
            {
                continue;
            }
            else
            {
                --iReadyCount;
            }
        }
        bool bIsSet = false;
        for (unsigned int i = 0; i <= max_fd && iReadyCount > 0; ++i)
        {
            if(i == intr_fd[1])
            {
                continue;
            }
            if (FD_ISSET(i, &rset))
            {
                bIsSet = true;
                TMdbNtcSocket* pSocketInfo = m_pNtcEngine->FindSocketInfo(i);
                if(pSocketInfo == NULL)
                {
                    RemoveEventMonitor(i, MDB_NTC_PEER_EV_READ_FLAG);
                }
                else if(pSocketInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
                {
                    OnServerAccept(static_cast<TMdbServerInfo*>(pSocketInfo));
                }
                else
                {
                    pPeerInfo = static_cast<TMdbPeerInfo*>(pSocketInfo);
                    OnPeerRecv(pPeerInfo);
                }
            }
            if (FD_ISSET(i, &wset))
            {
                bIsSet = true;
                if(pPeerInfo == NULL)
                {
                    pPeerInfo = static_cast<TMdbPeerInfo*>(m_pNtcEngine->FindSocketInfo(i));
                }
                if(pPeerInfo == NULL)
                {
                    RemoveEventMonitor(i, MDB_NTC_PEER_EV_WRITE_FLAG);
                }
                else
                {
                    OnPeerSend(static_cast<TMdbPeerInfo*>(pPeerInfo));
                }
            }
            if(bIsSet)
            {
                --iReadyCount;
                bIsSet  = false;//reset to false
                if(pPeerInfo && pPeerInfo->IsShutdown())
                {
                    bNeedSyncSet = true;
                }
                pPeerInfo = NULL;
            }
        }
    } while (1);
    return iRet;
}

/*--------------------------------- begin poll proactor------------------------------------------*/
#ifndef OS_WINDOWS
MDB_ZF_IMPLEMENT_OBJECT(TMdbProactorPoll, TMdbPeerProactor);
TMdbProactorPoll::TMdbProactorPoll()
{
    memset(m_arrayAllFds, 0x00, sizeof(m_arrayAllFds));
    m_uiPollFdSize = 0;
}

bool TMdbProactorPoll::AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(events == 0) return true;
    //根据fd查找index
    int iIndex = 0;
    for (; iIndex < m_uiPollFdSize; ++iIndex)
    {
        if(m_arrayAllFds[iIndex].fd == (int)fd)
        {
            break;
        }
    }
    if(iIndex == m_uiPollFdSize)
    {
        ++m_uiPollFdSize;
        m_arrayAllFds[iIndex].fd = (int)fd;
        m_arrayAllFds[iIndex].events = 0;
    }
    if(events&MDB_NTC_PEER_EV_READ_FLAG)
    {
        m_arrayAllFds[iIndex].events |= POLLIN|POLLPRI;
    }
    if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
    {
        m_arrayAllFds[iIndex].events |= POLLOUT|POLLWRBAND;
    }
    return true;
}

bool TMdbProactorPoll::RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    //根据fd查找index
    int iIndex = 0;
    for (; iIndex < m_uiPollFdSize; ++iIndex)
    {
        if(m_arrayAllFds[iIndex].fd == (int)fd)
        {
            break;
        }
    }
    if(iIndex == m_uiPollFdSize)
    {
        return true;
    }
    if(events == 0)
    {
        if(iIndex+1 < m_uiPollFdSize)
        {
            memmove(m_arrayAllFds+iIndex, m_arrayAllFds+iIndex+1,
                (size_t)(m_uiPollFdSize-iIndex-1)*sizeof(m_arrayAllFds[0]));
        }
        --m_uiPollFdSize;
    }
    else
    {
        if(events&MDB_NTC_PEER_EV_READ_FLAG)
        {
            m_arrayAllFds[iIndex].events &= ~(POLLIN|POLLPRI);
        }
        if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
        {
            m_arrayAllFds[iIndex].events &= ~(POLLOUT|POLLWRBAND);
        }
    }
    return true;
}

int TMdbProactorPoll::Execute()
{
    int iRet = 0;
    if(m_pNtcEngine == NULL)
    {
        return -1;
    }
    spinlock.Lock();
    m_arrayAllFds[m_uiPollFdSize].fd = (int)intr_fd[1];
    m_arrayAllFds[m_uiPollFdSize].events = POLLIN|POLLPRI;
    ++m_uiPollFdSize;
    spinlock.Unlock();
    bool bNeedSyncSet = false;///< 是否需要同步套接字集合
    do
    {
        if(bNeedSyncSet)
        {
            DealAsyncOperator();
            bNeedSyncSet = false;
        }
        //MDB_NTC_DEBUG("m_iPoolEventsSize[%d]", m_uiPollFdSize);
        int iReadyCount = poll(m_arrayAllFds, (nfds_t)m_uiPollFdSize, -1);
        if(iReadyCount == 0)//超时没有数据
        {
            continue;
        }
        else if(iReadyCount == -1)
        {
            iRet = TMdbNtcSocket::GetLastError();
            if(iRet == EINTR)//被信号中断
            {
                iRet = 0;
                continue;
            }
            else
            {
                mdb_ntc_errstr.Snprintf(1024, "select failed!error:%u,%s", iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
                TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
                return iRet;
            }
        }
        TADD_DETAIL("poll ret:%d", iReadyCount);
        //如果有事件的就是中断的fd，则中断目的已经达到，接收完buffer后，继续监听
        if(m_arrayAllFds[0].revents != 0)
        {
            bNeedSyncSet = true;
            if(iReadyCount == 1)
            {
                continue;
            }
            else
            {
                --iReadyCount;
            }
        }
        for(int i = 1; i < m_uiPollFdSize && iReadyCount > 0; ++i)
        {
            struct pollfd& event = m_arrayAllFds[i];
            if(event.revents == 0) continue;
            --iReadyCount;
            TADD_DETAIL("fd:%d", event.fd);
            TMdbNtcSocket* pSocketInfo = m_pNtcEngine->FindSocketInfo((QuickMDB_SOCKET)event.fd);
            if(pSocketInfo == NULL)
            {
                RemoveEventMonitor((QuickMDB_SOCKET)event.fd, 0);
            }
            else if(pSocketInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
            {
                OnServerAccept(static_cast<TMdbServerInfo*>(pSocketInfo));
            }
            else
            {
                TMdbPeerInfo* pPeerInfo = static_cast<TMdbPeerInfo*>(pSocketInfo);
                if(event.revents & POLLERR)//断开了
                {
                    pPeerInfo->Disconnect("remote abnormal disconnect", true);
                }
                else
                {
                    if(event.revents & (POLLIN|POLLPRI))
                    {   
                        OnPeerRecv(pPeerInfo);
                    }
                    if(event.revents & (POLLOUT|POLLWRBAND))
                    {
                        OnPeerSend(pPeerInfo);
                    }
                }
                if(pPeerInfo->IsShutdown())
                {
                    bNeedSyncSet = true;
                }
            }
        }
    }while (1);
    return iRet;
}
#endif

/*--------------------------------- begin epoll proactor------------------------------------------*/
#ifdef OS_LINUX
#include <sys/epoll.h>
#define MDB_EPOLL_SIZE  65535
MDB_ZF_IMPLEMENT_OBJECT(TMdbProactorEpoll, TMdbPeerProactor);
TMdbProactorEpoll::TMdbProactorEpoll()
{
    m_iEpollFd = -1;
}

TMdbProactorEpoll::~TMdbProactorEpoll()
{
    if(m_iEpollFd != -1)
    {
        TMdbNtcSocket::Close((QuickMDB_SOCKET)m_iEpollFd);
        m_iEpollFd = -1;
    }
}

bool TMdbProactorEpoll::AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(m_iEpollFd == -1) return false;
    int iRet = 0;    
    struct epoll_event ev;
    ev.data.fd = (int)fd;
    ev.events = 0;
    if(events == 0)
    {
        iRet = epoll_ctl(m_iEpollFd, EPOLL_CTL_ADD, (int)fd, &ev);
    }
    else
    {
        TMdbNtcSocket* pSocket = m_pNtcEngine->FindSocketInfo(fd);
        if(pSocket && !pSocket->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
        {
            events = static_cast<TMdbPeerInfo*>(pSocket)->GetPeerFlag(MDB_NTC_PEER_EV_MASK);
        }
        if(events&MDB_NTC_PEER_EV_READ_FLAG)
        {
            ev.events |= EPOLLIN|EPOLLPRI;
        }
        if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
        {
            ev.events |= EPOLLOUT|EPOLLWRBAND;
        }
        iRet = epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, (int)fd, &ev);
    }
    if(iRet != 0)
    {
        if(errno == EEXIST)//如果已经存在此描述，则无需添加，不算错误
        {
            return true;
        }
        else
        {
            mdb_ntc_errstr.Snprintf(1024, "AddEventMonitor:epoll_ctl failed!fd:%u,event:%x,error:%u,%s",fd, events, errno, strerror(errno));
            TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
            return false;
        }
    }
    return true;
}

bool TMdbProactorEpoll::RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(m_iEpollFd == -1) return false;
    int iRet = 0;    
    struct epoll_event ev;
    ev.data.fd = (int)fd;
    ev.events = 0;
    if(events == 0)
    {
        iRet = epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, (int)fd, &ev);
    }
    else
    {
        TMdbNtcSocket* pSocket = m_pNtcEngine->FindSocketInfo(fd);
        if(pSocket && !pSocket->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
        {
            events = static_cast<TMdbPeerInfo*>(pSocket)->GetPeerFlag(MDB_NTC_PEER_EV_MASK);
            if(events&MDB_NTC_PEER_EV_READ_FLAG)
            {
                ev.events |= EPOLLIN|EPOLLPRI;
            }
            if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
            {
                ev.events |= EPOLLOUT|EPOLLWRBAND;
            }
        }
        iRet = epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, (int)fd, &ev);
    }
    if(iRet != 0)
    {
        if(errno == EEXIST)//如果已经存在此描述，则无需添加，不算错误
        {
            return true;
        }
        else
        {
            mdb_ntc_errstr.Snprintf(1024, "RemoveEventMonitor:epoll_ctl failed!fd:%u,event:%x,error:%u,%s",fd, events, errno, strerror(errno));
            TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
            return false;
        }
    }
    return true;
}

int TMdbProactorEpoll::Execute()
{
    if(m_pNtcEngine == NULL)
    {
        return -1;
    }
    m_iEpollFd = epoll_create(MDB_EPOLL_SIZE);//如果出错返回-1
    if(m_iEpollFd == -1)
    {
        mdb_ntc_errstr.Snprintf(1024, "epoll_create failed!error:%u,%s", TMdbNtcSocket::GetLastError(), TMdbNtcSocket::GetLastErrorInfo().c_str());
        return false;
    }
    AddEventMonitor(intr_fd[1], 0);
    AddEventMonitor(intr_fd[1], MDB_NTC_PEER_EV_READ_FLAG);
    struct epoll_event *events = new epoll_event[MDB_EPOLL_SIZE];
    mdb_ntc_zthread_cleanup_push(events);//线程退出时，清理此内存
    //bool bNeedSyncSet = false;///< 是否需要同步套接字集合
    int iSync = 0;//中断请求数
    int iBatchCount = 100;//一次中断个数
    
    do
    {
        if(iSync > 0)
        {
            DealAsyncOperator();
            //bNeedSyncSet = false;
            iSync = 0;
        }
        int nfds = epoll_wait(m_iEpollFd, events, MDB_EPOLL_SIZE, -1);
        TADD_DETAIL("after wait %d", nfds);
        for(int i=0; i < nfds; ++i)
        {
            QuickMDB_SOCKET fd = (QuickMDB_SOCKET)events[i].data.fd;
            //MDB_NTC_DEBUG("Execute:fd[%d], events[%d]", fd, events[i].events);
            if(fd == intr_fd[1])
            {
                //bNeedSyncSet = true;
                ++iSync;
            }
            else
            {
                TMdbNtcSocket* pSocketInfo = m_pNtcEngine->FindSocketInfo(fd);
                if(pSocketInfo == NULL)
                {
                    RemoveEventMonitor(fd, 0);
                }
                else if(pSocketInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
                {
                    OnServerAccept(static_cast<TMdbServerInfo*>(pSocketInfo));
                }
                else
                {
                    TMdbPeerInfo* pPeerInfo = static_cast<TMdbPeerInfo*>(pSocketInfo);
                    if(events[i].events&EPOLLERR)//断开了
                    {
                        pPeerInfo->Disconnect("remote abnormal disconnect", true);
                    }
                    else
                    {
                        if(events[i].events&(EPOLLIN|EPOLLPRI))//读事件
                        {
                            OnPeerRecv(pPeerInfo);
                        }
                        if(events[i].events&(EPOLLOUT|EPOLLWRBAND))//写事件
                        {
                            OnPeerSend(pPeerInfo);
                        }
                    }
                    if(pPeerInfo->IsShutdown())
                    {
                        //bNeedSyncSet = true;
                        ++iSync;
                    }
                }
            }
            if((1 + iSync)%iBatchCount == 0)
            {//增加处理频率，避免并发大量连接请求处理阻塞
                DealAsyncOperator();
                //bNeedSyncSet = false;
                iSync = 0;
            }
        }
    } while (1);
    return 0;
}
#endif

/*--------------------------------- begin pollset proactor------------------------------------------*/

#ifdef OS_IBM
MDB_ZF_IMPLEMENT_OBJECT(TMdbProactorPollset, TMdbPeerProactor);
TMdbProactorPollset::TMdbProactorPollset()
{
    m_iPollsetFd = -1;
}

TMdbProactorPollset::~TMdbProactorPollset()
{
    if(m_iPollsetFd != -1)
    {
        pollset_destroy(m_iPollsetFd);
        m_iPollsetFd = -1;
    }
}

bool TMdbProactorPollset::AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(m_iPollsetFd == -1) return false;
    TADD_DETAIL("AddEventMonitor:fd[%d], events[%0x]", fd, events);
    int iRet = 0;
    struct poll_ctl ev;
    ev.fd = fd;
    ev.events = 0;
    if(events == 0)
    {
        return true;
        /*
        ev.cmd = PS_ADD;
        iRet = pollset_ctl(m_iPollsetFd, &ev, 1);
        */
    }
    else
    {
        //ev.cmd = PS_MOD;
		/*
        TMdbNtcSocket* pSocket = m_pNtcEngine->FindSocketInfo(fd);
        if(pSocket && !pSocket->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
        {
            events = static_cast<TMdbPeerInfo*>(pSocket)->GetPeerFlag(MDB_NTC_PEER_EV_MASK);
        }
        if(events&MDB_NTC_PEER_EV_READ_FLAG)
        {
            ev.events |= POLLIN|POLLPRI;
        }
        if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
        {
            ev.events |= POLLOUT|POLLWRBAND;
        }
        //先删除
        ev.cmd = PS_DELETE;
        iRet = pollset_ctl(m_iPollsetFd, &ev, 1);
		*/
		if(events&MDB_NTC_PEER_EV_READ_FLAG)
        {
            ev.events |= POLLIN|POLLPRI;
        }
        if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
        {
            ev.events |= POLLOUT|POLLWRBAND;
        }
        ev.cmd = PS_MOD;
        iRet = pollset_ctl(m_iPollsetFd, &ev, 1);
    }            
    if(iRet != 0)
    {
        if(errno == EEXIST)//如果已经存在此描述，则无需添加，不算错误
        {
            return true;
        }
        else
        {
            mdb_ntc_errstr.Snprintf(1024, "AddEventMonitor:pollset_ctl failed!fd:%u,event:%x,error:%u,%s",fd, events,
                errno, strerror(errno));
            TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
            return false;
        }
    }
    return true;
}

bool TMdbProactorPollset::RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(m_iPollsetFd == -1) return false;
    TADD_DETAIL("RemoveEventMonitor:fd[%d], events[%0x]", fd, events);
    int iRet = 0;
    struct poll_ctl ev;
    ev.fd = fd;
    ev.events = 0;
    if(events == 0)
    {
        ev.cmd = PS_DELETE;
        iRet = pollset_ctl(m_iPollsetFd, &ev, 1);
    }
    else
    {
        ev.cmd = PS_MOD;
        TMdbNtcSocket* pSocket = m_pNtcEngine->FindSocketInfo(fd);
        if(pSocket && !pSocket->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
        {
            events = static_cast<TMdbPeerInfo*>(pSocket)->GetPeerFlag(MDB_NTC_PEER_EV_MASK);
            if(events&MDB_NTC_PEER_EV_READ_FLAG)
            {
                ev.events |= POLLIN|POLLPRI;
            }
            if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
            {
                ev.events |= POLLOUT|POLLWRBAND;
            }
        }
        //先删除
        ev.cmd = PS_DELETE;
        iRet = pollset_ctl(m_iPollsetFd, &ev, 1);
        if(ev.events != 0)
        {
            ev.cmd = PS_MOD;
            iRet = pollset_ctl(m_iPollsetFd, &ev, 1);
        }        
    }
    if(iRet != 0)
    {
        if(errno == EEXIST)//如果已经存在此描述，则无需添加，不算错误
        {
            return true;
        }
        else
        {
            mdb_ntc_errstr.Snprintf(1024, "RemoveEventMonitor:pollset_ctl failed!fd:%u,event:%x,error:%u,%s",fd, events,
                errno, strerror(errno));
            TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
            return false;
        }
    }
    return true;
}

int TMdbProactorPollset::Execute()
{
    int iRet = 0;
    if(m_pNtcEngine == NULL)
    {
        return -1;
    }
    m_iPollsetFd = pollset_create(OPEN_MAX);//如果出错返回-1
    if(m_iPollsetFd == -1)
    {
        mdb_ntc_errstr.Snprintf(1024, "pollset_create failed!error:%u,%s", TMdbNtcSocket::GetLastError(), TMdbNtcSocket::GetLastErrorInfo().c_str());
        return -1;
    }
    AddEventMonitor(intr_fd[1], 0);
    AddEventMonitor(intr_fd[1], MDB_NTC_PEER_EV_READ_FLAG);
    struct pollfd *events = new pollfd[OPEN_MAX];
    mdb_ntc_zthread_cleanup_push(events);
    bool bNeedSyncSet = false;///< 是否需要同步套接字集合
    do
    {
        if(bNeedSyncSet)
        {
            DealAsyncOperator();
            bNeedSyncSet = false;
        }
        int nfds = pollset_poll(m_iPollsetFd, events, OPEN_MAX, -1);
        TADD_DETAIL("after wait %d", nfds);
        for(int i=0; i < nfds; ++i)
        {
            QuickMDB_SOCKET fd = events[i].fd;
            //MDB_NTC_DEBUG("Execute:fd[%d], events[%d]", fd, events[i].events);
            if(fd == intr_fd[1])
            {
                bNeedSyncSet = true;
            }
            else
            {
                TMdbNtcSocket* pSocketInfo = m_pNtcEngine->FindSocketInfo(fd);
                if(pSocketInfo == NULL)
                {
                    RemoveEventMonitor(fd, 0);
                }
                else if(pSocketInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
                {
                    OnServerAccept(static_cast<TMdbServerInfo*>(pSocketInfo));
                }
                else
                {
                    TMdbPeerInfo* pPeerInfo = static_cast<TMdbPeerInfo*>(pSocketInfo);
                    if(events[i].events & POLLERR)//断开了
                    {
                        pPeerInfo->Disconnect("remote abnormal disconnect", true);
                    }
                    else
                    {
                        if(events[i].events & (POLLIN|POLLPRI))
                        {   
                            OnPeerRecv(pPeerInfo);
                        }
                        if(events[i].events & (POLLOUT|POLLWRBAND))
                        {
                            OnPeerSend(pPeerInfo);
                        }
                    }
                    if(pPeerInfo->IsShutdown())
                    {
                        bNeedSyncSet = true;
                    }
                }
            }
        }
    } while (1);
    return 0;
}
#endif

/*--------------------------------- begin iocp proactor------------------------------------------*/
#ifdef _WIN32
#include <winsock2.h>
#include <mswsock.h>
#pragma comment(lib, "Mswsock.lib")
//#define DATA_BUFSIZE 8192
typedef BOOL ( WINAPI * MDB_PFNACCEPTEX ) ( QuickMDB_SOCKET, QuickMDB_SOCKET, PVOID, DWORD, DWORD, DWORD, LPDWORD, LPOVERLAPPED );
MDB_PFNACCEPTEX g_pmdbfnAcceptEx = NULL;

class TServerProactorInfo:public TMdbProactorInfo
{
public:
    MDB_OVERLAPPEDPLUS overlapped;
    char szBuffer[2*(sizeof(sockaddr_in) + 16)];
    DWORD dwBytesReceived;
    QuickMDB_SOCKET client_fd;
    TServerProactorInfo():overlapped(MDB_IOCP_EVENT_ACCEPT)
    {
        memset(szBuffer, 0x00, sizeof(szBuffer));
        client_fd = MDB_NTC_INVALID_SOCKET;
        dwBytesReceived = 0;
    }
    ~TServerProactorInfo()
    {
        if(client_fd != MDB_NTC_INVALID_SOCKET)
        {
            TMdbNtcSocket::Close(client_fd);
            client_fd = MDB_NTC_INVALID_SOCKET;
        }        
    }
};

class TMdbPeerProactorInfo:public TMdbProactorInfo
{
public:
    TMdbPeerInfo* pPeerInfo;
    MDB_OVERLAPPEDPLUS overlapped_recv;
    MDB_OVERLAPPEDPLUS overlapped_send;
    TMdbPeerProactorInfo(TMdbPeerInfo* pPeerInfoRef)
        :pPeerInfo(pPeerInfoRef),overlapped_recv(MDB_IOCP_EVENT_WSARECV),overlapped_send(MDB_IOCP_EVENT_WSASEND)
    {
    }
};

MDB_ZF_IMPLEMENT_OBJECT(TMdbProactorIocp, TMdbPeerProactor);
TMdbProactorIocp::TMdbProactorIocp()
:m_oIntrOverlapped(MDB_IOCP_EVENT_WSARECV)
{
    m_hCompletionPort = INVALID_HANDLE_VALUE;    
}

TMdbProactorIocp::~TMdbProactorIocp()
{
    if(m_hCompletionPort != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hCompletionPort);
        m_hCompletionPort = INVALID_HANDLE_VALUE;
    }
}

bool TMdbProactorIocp::AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(m_hCompletionPort == INVALID_HANDLE_VALUE) return false;
    int iRet = 0;
    if(events == 0)
    {
        if (CreateIoCompletionPort((HANDLE)fd, m_hCompletionPort, (DWORD)fd, 0) == NULL)
        {
            iRet = TMdbNtcSocket::GetLastError();
            mdb_ntc_errstr.Snprintf(1024, "AddEventMonitor:CreateIoCompletionPort failed!fd:%u,event:%x,error:%u,%s",fd, events,
                iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
            TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
            return false;
        }
        else
        {
            return true;
        }
    }
    else if(fd == intr_fd[1])
    {
        m_oIntrOverlapped.bMonitor = true;
        iRet = WSARecv(fd, &(m_oIntrOverlapped.wsaDataBuf), 1,
            &m_oIntrOverlapped.dwBytes, &m_oIntrOverlapped.dwFlags,
            &m_oIntrOverlapped, NULL);
        if(iRet != 0)
        {
            iRet = TMdbNtcSocket::GetLastError();
            if(iRet != ERROR_IO_PENDING)
            {
                mdb_ntc_errstr.Snprintf(1024, "AddEventMonitor:WSARecv failed!fd:%u,event:%x,error:%u,%s",fd, events,
                    iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
                TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
                return false;
            }
        }
        return true;
    }
    TMdbNtcSocket* pSocketInfo = m_pNtcEngine->FindSocketInfo(fd);
    if(pSocketInfo == NULL) return false;
    if(!pSocketInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
    {
        TMdbPeerInfo* pPeerInfo = static_cast<TMdbPeerInfo*>(pSocketInfo);
        TMdbPeerProactorInfo* pProactorInfo = static_cast<TMdbPeerProactorInfo*>(pPeerInfo->pProactorInfo);
        if(pProactorInfo == NULL)
        {
            pPeerInfo->pProactorInfo = pProactorInfo = new TMdbPeerProactorInfo(pPeerInfo);
        }
        if(events&MDB_NTC_PEER_EV_READ_FLAG)
        {
            /*
            //判断是否已经超过了限流
            //MDB_UINT32 uiAllowRecvBytes = m_pPeerMgr->GetPeerAllowRecvBytes(pSocketInfo, DATA_BUFSIZE);
            int iRecvBuffSize = 0;
            pPeerInfo->GetRecvBufSize(fd, iRecvBuffSize);
            if(iRecvBuffSize <= 0)
            {
                iRecvBuffSize = DATA_BUFSIZE;
            }
            pProactorInfo->pPacketInfo = new TMdbPacketInfo(iRecvBuffSize);
            pProactorInfo->wsaRecvDataBuf.buf = pProactorInfo->pPacketInfo->GetBuffer();
            pProactorInfo->wsaRecvDataBuf.len = iRecvBuffSize-1;
            pPeerInfo->SetPeerState(MDB_NTC_PEER_RECV_STATE);
            */
            pProactorInfo->overlapped_recv.bMonitor = true;
            iRet = WSARecv(fd, &(pProactorInfo->overlapped_recv.wsaDataBuf), 1,
                &pProactorInfo->overlapped_recv.dwBytes, &pProactorInfo->overlapped_recv.dwFlags,
                &pProactorInfo->overlapped_recv, NULL);
            if(iRet != 0)
            {
                iRet = TMdbNtcSocket::GetLastError();
                if(iRet != ERROR_IO_PENDING)
                {
                    mdb_ntc_errstr.Snprintf(1024, "AddEventMonitor:WSARecv failed!fd:%u,event:%x,error:%u,%s",fd, events,
                        iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
                    TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
                    return false;
                }
            }
        }
        if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
        {
            pProactorInfo->overlapped_send.bMonitor = true;
            /*
            TMdbSendPackets& oSendPackets = pPeerInfo->oSendPackets;
            if(oSendPackets.IsEmpty()) return true;
            pPeerInfo->SetPeerState(MDB_NTC_PEER_WRITE_STATE);
            TMdbPacketInfo* pCurPacketInfo = static_cast<TMdbPacketInfo*>(oSendPackets.Front());
            pProactorInfo->wsaSendDataBuf.buf = pCurPacketInfo->GetBuffer()+oSendPackets.iSendBytes;
            pProactorInfo->wsaSendDataBuf.len = pCurPacketInfo->GetLength()-oSendPackets.iSendBytes;
            */
            iRet = WSASend(fd, &(pProactorInfo->overlapped_send.wsaDataBuf), 1,
                &pProactorInfo->overlapped_send.dwBytes, 0,
                &pProactorInfo->overlapped_send, NULL);                                
            if(iRet != 0)
            {
                iRet = TMdbNtcSocket::GetLastError();
                if(iRet != ERROR_IO_PENDING)
                {
                    mdb_ntc_errstr.Snprintf(1024, "AddEventMonitor:WSASend failed!fd:%u,event:%x,error:%u,%s",fd, events,
                        iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
                    TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
                    return false;
                }
            }
        }
    }
    else
    {
        if(g_pmdbfnAcceptEx == NULL)
        {
            mdb_ntc_errstr = "g_pmdbfnAcceptEx is null";
            return false;
        }
        TMdbServerInfo* pServerInfo = static_cast<TMdbServerInfo*>(pSocketInfo);
        TServerProactorInfo* pProactorInfo = static_cast<TServerProactorInfo*>(pServerInfo->pProactorInfo);
        if(pProactorInfo == NULL)
        {
            pServerInfo->pProactorInfo = pProactorInfo = new TServerProactorInfo;
        }
        pProactorInfo->overlapped.bMonitor = true;
        pProactorInfo->client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,0,0,WSA_FLAG_OVERLAPPED);    
        DWORD dwAddrSize = sizeof( SOCKADDR_IN ) + 16;
        if(FALSE == g_pmdbfnAcceptEx(fd, pProactorInfo->client_fd,
            pProactorInfo->szBuffer, 
            0,       //传0进来，接受的连接的时候不接受数据,
            dwAddrSize, dwAddrSize, &pProactorInfo->dwBytesReceived,
            (LPOVERLAPPED )&pProactorInfo->overlapped))
        {
            iRet = GetLastError();
            if(iRet != ERROR_IO_PENDING)
            {
                mdb_ntc_errstr.Snprintf(1024, "AddEventMonitor:g_pmdbfnAcceptEx failed!fd:%u,event:%x,error:%u,%s",fd, events,
                        iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
                TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
                return false;
            }
        }
    }
    if(iRet == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool TMdbProactorIocp::RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(m_hCompletionPort == INVALID_HANDLE_VALUE) return false;
    if(fd == intr_fd[1])
    {
        m_oIntrOverlapped.bMonitor = false;
        return true;
    }
    TMdbNtcSocket* pSocketInfo = m_pNtcEngine->FindSocketInfo(fd);
    if(pSocketInfo == NULL) return false;
    if(events == 0)
    {
        TMdbPeerInfo* pPeerInfo = MDB_ZF_DYNAMIC_CAST(TMdbPeerInfo, pSocketInfo);
        if(pPeerInfo)
        {
            TMdbPeerProactorInfo* pProactorInfo = static_cast<TMdbPeerProactorInfo*>(pPeerInfo->pProactorInfo);
            if(pProactorInfo) pProactorInfo->pPeerInfo = NULL;
            pPeerInfo->pProactorInfo = NULL;//真正的释放，留在GetQueuedCompletionStatus环节
        }
    }
    else
    {
        if(!pSocketInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
        {
            TMdbPeerProactorInfo* pProactorInfo = static_cast<TMdbPeerProactorInfo*>(static_cast<TMdbPeerInfo*>(pSocketInfo)->pProactorInfo);
            if(pProactorInfo)
            {
                if(events&MDB_NTC_PEER_EV_READ_FLAG)
                {
                    pProactorInfo->overlapped_recv.bMonitor = false;
                }
                else if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
                {
                    pProactorInfo->overlapped_send.bMonitor = false;
                }
            }
        }
        else
        {
            TServerProactorInfo* pProactorInfo = static_cast<TServerProactorInfo*>(static_cast<TMdbServerInfo*>(pSocketInfo)->pProactorInfo);
            if(pProactorInfo)
            {
                pProactorInfo->overlapped.bMonitor = false;
            }
        }
    }
    return true;
}

int TMdbProactorIocp::Execute()
{
    int iRet = 0;
    //创建一个IO完成端口
    m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
    if (m_hCompletionPort == INVALID_HANDLE_VALUE ) 
    {
        iRet = TMdbNtcSocket::GetLastError();
        mdb_ntc_errstr.Snprintf(1024, "CreateIoCompletionPort failed!error:%u,%s", iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
        TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
        return -1;
    }
    DWORD dwBytes = 0;
    GUID guidAcceptEx = WSAID_ACCEPTEX;
    iRet = ::WSAIoctl(intr_fd[1], SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx, sizeof( guidAcceptEx ),
        &g_pmdbfnAcceptEx, sizeof(g_pmdbfnAcceptEx), &dwBytes, NULL, NULL );
    if(iRet != 0)
    {
        iRet = TMdbNtcSocket::GetLastError();
        mdb_ntc_errstr.Snprintf(1024, "WSAIoctl failed!error:%u,%s", iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
        TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
        return iRet;
    }
    AddEventMonitor(intr_fd[1], 0);
    AddEventMonitor(intr_fd[1], MDB_NTC_PEER_EV_READ_FLAG);
    DWORD dwBytesTransferred = 0;
    MDB_OVERLAPPEDPLUS* over_type = NULL;
    QuickMDB_SOCKET fd = MDB_NTC_INVALID_SOCKET;
    bool bNeedSyncSet = false;///< 是否需要同步套接字集合
    while (1)
    {
        if(bNeedSyncSet)
        {
            DealAsyncOperator();
            bNeedSyncSet = false;
        }
        over_type = NULL;
        bool bRet = GetQueuedCompletionStatus(m_hCompletionPort,
            &dwBytesTransferred,                    //的I/O操作所传送数据的字节数
            (LPDWORD)&fd,           //用于存放与之关联的Completion键
            (LPOVERLAPPED*)&over_type,
            INFINITE)?true:false;
        if(bRet == false)
        {
            iRet = GetLastError();
            if(ERROR_INVALID_HANDLE == iRet)
            {
                TADD_DETAIL("完成端口被关闭,退出");
                return iRet;
            }
            if(fd == intr_fd[1])
            {
                continue;
            }
            int iStructOffset = -1;
            if(over_type->io_type == MDB_IOCP_EVENT_WSARECV)
            {
                iStructOffset = (int)&((class TMdbPeerProactorInfo*)0)->overlapped_recv;
            }
            else if(over_type->io_type == MDB_IOCP_EVENT_WSASEND)
            {
                iStructOffset = (int)&((class TMdbPeerProactorInfo*)0)->overlapped_send;
            }
            if(iStructOffset >= 0)
            {
                TMdbPeerProactorInfo* pProactorInfo = reinterpret_cast<TMdbPeerProactorInfo*>(((char*)over_type)-iStructOffset);
                if(pProactorInfo->pPeerInfo)
                {
                    pProactorInfo->pPeerInfo->pProactorInfo = NULL;
                    pProactorInfo->pPeerInfo->Disconnect("remote normal disconnect", true);
                }
                delete pProactorInfo;
                pProactorInfo = NULL;
            }
        }
        else if(over_type->bMonitor == false)
        {
            continue;
        }
        else if(fd == intr_fd[1])
        {
            bNeedSyncSet = true;
            AddEventMonitor(fd, MDB_NTC_PEER_EV_READ_FLAG);
        }
        else
        {
            TMdbNtcSocket* pSocket = m_pNtcEngine->FindSocketInfo(fd);
            if(pSocket == NULL)
            {
                TMdbNtcSocket::Close(fd);
                continue;
            }
            switch(over_type->io_type)
            {
            case MDB_IOCP_EVENT_ACCEPT:
                {
                    TMdbServerInfo* pServerInfo = static_cast<TMdbServerInfo*>(pSocket);
                    TServerProactorInfo* pProactorInfo = static_cast<TServerProactorInfo*>(pServerInfo->pProactorInfo);
                    // The socket sAcceptSocket does not inherit the properties of the socket associated with
                    // sListenSocket parameter until SO_UPDATE_ACCEPT_CONTEXT is set on the socket
                    QuickMDB_SOCKET listen_fd = pServerInfo->GetSocketID(), new_fd = pProactorInfo->client_fd;
                    pProactorInfo->client_fd = MDB_NTC_INVALID_SOCKET;
                    setsockopt(new_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                        (char*)&listen_fd, sizeof(listen_fd) );//继承监听 socket 的属性
                    if(AddNewClient(new_fd, pServerInfo))
                    {
                        AddEventMonitor(listen_fd, MDB_NTC_PEER_EV_READ_FLAG);//继续监视
                    }
                }
                break;
            case MDB_IOCP_EVENT_WSARECV:
                {                    
                    TMdbPeerInfo* pPeerInfo = static_cast<TMdbPeerInfo*>(pSocket);
                    if(OnPeerRecv(pPeerInfo))
                    {
                        AddEventMonitor(fd, MDB_NTC_PEER_EV_READ_FLAG);//继续接收                        
                    }
                    if(pPeerInfo->IsShutdown())
                    {
                        bNeedSyncSet = true;
                    }
                    /*
                    if(dwBytesTransferred == 0 && pPeerInfo->GetPeerFlag(MDB_NTC_PEER_EV_READ_FLAG))
                    {
                        AddEventMonitor(fd, MDB_NTC_PEER_EV_READ_FLAG);//继续接收
                        break;
                    }
                    TMdbPeerProactorInfo* pProactorInfo = static_cast<TMdbPeerProactorInfo*>(pPeerInfo->pProactorInfo);
                    pPeerInfo->GetTrafficInfo()->AddRecvBytes(dwBytesTransferred);
                    m_pAcpfEngine->AddTotalRecvBytes(dwBytesTransferred);
                    pProactorInfo->pPacketInfo->SetLength(dwBytesTransferred);
                    char* pszBuffer = pProactorInfo->pPacketInfo->GetBuffer();
                    pszBuffer[dwBytesTransferred] = '\0';
                    pPeerInfo->oRecvPackets.AddPacket(pProactorInfo->pPacketInfo);
                    pProactorInfo->pPacketInfo = NULL;
                    pProactorInfo->wsaRecvDataBuf.buf = NULL;
                    pPeerInfo->SetPeerState(MDB_NTC_PEER_IDLE_STATE);
                    //说明收到了dwBytesTransferred个字节，开始组包
                    if(pPeerInfo->CheckRecvPackets())//使用对应的协议检查数据包是否完整
                    {
                        if(pPeerInfo->GetPeerFlag(MDB_NTC_PEER_EV_READ_FLAG))
                        {
                            AddEventMonitor(fd, MDB_NTC_PEER_EV_READ_FLAG);//继续接收
                        }                        
                    }
                    else
                    {
                        pPeerInfo->SetPeerState(PEER_ERROR_STATE);
                    }
                    */
                }
                break;
            case MDB_IOCP_EVENT_WSASEND:
                {
                    TMdbPeerInfo* pPeerInfo = static_cast<TMdbPeerInfo*>(pSocket);
                    if(OnPeerSend(pPeerInfo))
                    {
                        AddEventMonitor(fd, MDB_NTC_PEER_EV_WRITE_FLAG);//继续发送
                    }
                    if(pPeerInfo->IsShutdown())
                    {
                        bNeedSyncSet = true;
                    }
                    /*
                    TMdbPeerProactorInfo* pProactorInfo = static_cast<TMdbPeerProactorInfo*>(pPeerInfo->pProactorInfo);
                    pProactorInfo->wsaSendDataBuf.buf = NULL;
                    pPeerInfo->GetTrafficInfo()->AddSendBytes(dwBytesTransferred);
                    m_pAcpfEngine->AddTotalSendBytes(dwBytesTransferred);
                    TMdbSendPackets& oSendPackets = pPeerInfo->oSendPackets;
                    oSendPackets.iSendBytes += dwBytesTransferred;
                    pPeerInfo->SetPeerState(MDB_NTC_PEER_IDLE_STATE);
                    TMdbPacketInfo* pCurPacketInfo = static_cast<TMdbPacketInfo*>(oSendPackets.Front());
                    if(pCurPacketInfo)
                    {
                        pCurPacketInfo->uiLength -= dwBytesTransferred;
                        if(pCurPacketInfo->GetLength() == 0)
                        {
                            oSendPackets.Pop();
                            delete pCurPacketInfo;
                            pCurPacketInfo = NULL;
                        }
                    }
                    pPeerInfo->Lock();
                    if(!oSendPackets.IsEmpty() && pPeerInfo->GetPeerFlag(MDB_NTC_PEER_EV_WRITE_FLAG))
                    {
                        AddEventMonitor(fd, MDB_NTC_PEER_EV_WRITE_FLAG);
                    }
                    pPeerInfo->Unlock();
                    */
                }
                break;
            }
        }
    }
    return iRet;
}
#endif

/*--------------------------------- begin kqueue proactor------------------------------------------*/
#ifdef OS_FREEBSD
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
MDB_ZF_IMPLEMENT_OBJECT(TMdbProactorKqueue, TMdbPeerProactor);
TMdbProactorKqueue::TMdbProactorKqueue()
{
    m_kq = -1;
}

TMdbProactorKqueue::~TMdbProactorKqueue()
{
    if(m_kq != -1)
    {
        TMdbNtcSocket::Close(m_kq);
        m_kq = -1;
    }
}

bool TMdbProactorKqueue::AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(m_kq == -1) return false;
    //MDB_NTC_DEBUG("add fd[%d] event[%d]", fd, events);
    struct kevent changes;
    if(events == 0)
    {
        EV_SET(&changes, fd, EVFILT_READ|EVFILT_WRITE, EV_ADD|EV_DISABLE , 0, 0, NULL);
    }
    else
    {
        int event = 0;
        if(events&MDB_NTC_PEER_EV_READ_FLAG)
        {
            event |= EVFILT_READ;
        }
        if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
        {
            event |= EVFILT_WRITE;
        }
        if(event == 0)
        {
            return true;
        }
        EV_SET(&changes, fd, event, EV_ADD|EV_ENABLE, 0, 0, NULL);
    }
    int iRet = kevent(m_kq, &changes, 1, NULL, 0, NULL);
    if(iRet != 0)
    {
        mdb_ntc_errstr.Snprintf(1024, "AddEventMonitor:kqueue failed!fd:%u,event:%x,error:%u,%s",fd, events, errno, strerror(errno));
        TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
        return false;
    }
    else
    {
        return true;
    }
}

bool TMdbProactorKqueue::RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(m_kq == -1) return false;
    //MDB_NTC_DEBUG("remove fd[%d] event[%d]", fd, events);
    struct kevent changes;
    if(events == 0)
    {
        EV_SET(&changes, fd, EVFILT_READ|EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    }
    else
    {
        int event = 0;
        if(events&MDB_NTC_PEER_EV_READ_FLAG)
        {
            event |= EVFILT_READ;
        }
        if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
        {
            event |= EVFILT_WRITE;
        }
        if(event == 0)
        {
            return true;
        }
        EV_SET(&changes, fd, event, EV_ADD|EV_DISABLE , 0, 0, NULL);
    }
    int iRet = kevent(m_kq, &changes, 1, NULL, 0, NULL);
    if(iRet != 0)
    {
        mdb_ntc_errstr.Snprintf(1024, "RemoveEventMonitor:kqueue failed!fd:%u,event:%x,error:%u,%s",fd, events, errno, strerror(errno));
        TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
        return false;
    }
    else
    {
        return true;
    }
}

int TMdbProactorKqueue::Execute()
{
    int iRet = 0;
    if(m_pNtcEngine == NULL)
    {
        return -1;
    }
    m_kq = kqueue();//如果出错返回-1
    if(m_kq == -1)
    {
        mdb_ntc_errstr.Snprintf(1024, "kqueue failed!error:%u,%s", TMdbNtcSocket::GetLastError(), TMdbNtcSocket::GetLastErrorInfo().c_str());
        return -1;
    }
    AddEventMonitor(intr_fd[1], 0);
    AddEventMonitor(intr_fd[1], MDB_NTC_PEER_EV_READ_FLAG);
    struct kevent* events = new struct kevent[65536];
    mdb_ntc_zthread_cleanup_push(events);
    memset(events, 0x00, sizeof(kevent)*65536);
    bool bNeedSyncSet = false;///< 是否需要同步套接字集合
    do
    {
        if(bNeedSyncSet)
        {
            DealAsyncOperator();
            bNeedSyncSet = false;
        }
        int iReadyCount = kevent(m_kq, NULL, 0, events, 65536, NULL);
        if(iReadyCount == 0)//超时没有数据
        {
            continue;
        }
        else if(iReadyCount == -1)
        {
            iRet = TMdbNtcSocket::GetLastError();
            if(iRet == EINTR)//被信号中断
            {
                iRet = 0;
                continue;
            }
            else
            {
                mdb_ntc_errstr.Snprintf(1024, "select failed!error:%u,%s", iRet, TMdbNtcSocket::GetErrorInfo(iRet).c_str());
                TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
                return iRet;
            }
        }
        TADD_DETAIL("kevent ret:%d", iReadyCount);
        for (int i = 0; i < iReadyCount; i++)
        {
            struct kevent& event = events[i];            
            QuickMDB_SOCKET fd = event.ident;
            if(fd == intr_fd[1])
            {
                bNeedSyncSet = true;
            }
            else
            {
                TMdbNtcSocket* pSocketInfo = m_pNtcEngine->FindSocketInfo(fd);
                if(pSocketInfo == NULL)
                {
                    RemoveEventMonitor(fd, 0);
                }
                else if(pSocketInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
                {
                    OnServerAccept(static_cast<TMdbServerInfo*>(pSocketInfo));
                }
                else
                {
                    TMdbPeerInfo* pPeerInfo = static_cast<TMdbPeerInfo*>(pSocketInfo);
                    if(event.flags&EV_EOF || event.flags&EV_ERROR)//断开了
                    {
                        pPeerInfo->Disconnect("remote abnormal disconnect", true);
                    }
                    else if(event.filter == EVFILT_READ)
                    {
                        OnPeerRecv(pPeerInfo);
                    }
                    else if(event.filter == EVFILT_WRITE)
                    {
                        OnPeerSend(pPeerInfo);
                    }
                    if(pPeerInfo->IsShutdown())
                    {
                        bNeedSyncSet = true;
                    }
                }
            }
        }
    } while (1);
    return iRet;
}
#endif

/*--------------------------------- begin eventports proactor------------------------------------------*/
#ifdef OS_SUN
#include <port.h> /* for event ports */  
MDB_ZF_IMPLEMENT_OBJECT(TMdbProactorEventPorts, TMdbPeerProactor);
TMdbProactorEventPorts::TMdbProactorEventPorts()
{
    m_portfd = -1;    
}

TMdbProactorEventPorts::~TMdbProactorEventPorts()
{
    if(m_portfd != -1)
    {
        TMdbNtcSocket::Close(m_portfd);
    }
}

bool TMdbProactorEventPorts::AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(m_portfd == -1) return false;
    int iRet = 0;
    //MDB_NTC_DEBUG("add fd[%d] event[%d]", fd, events);fflush(stdout);
    if(events == 0)
    {
        return true;
    }
    else
    {
        int event = 0;
        TMdbNtcSocket* pSocket = m_pNtcEngine->FindSocketInfo(fd);
        if(pSocket && !pSocket->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
        {
            events = static_cast<TMdbPeerInfo*>(pSocket)->GetPeerFlag(MDB_NTC_PEER_EV_MASK);
        }
        if(events&MDB_NTC_PEER_EV_READ_FLAG)
        {
            event |= POLLIN|POLLPRI;
        }
        if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
        {
            event |= POLLOUT|POLLWRBAND;
        }
        if(event == 0)
        {
            return true;
        }
        iRet = port_associate(m_portfd, PORT_SOURCE_FD, fd, event, NULL);
    }
    if(iRet != 0)
    {
        mdb_ntc_errstr.Snprintf(1024, "AddEventMonitor:port_dissociate failed!fd:%u,event:%x,error:%u,%s",fd, events,
                errno, strerror(errno));
        TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
        return false;
    }
    return true;
}

bool TMdbProactorEventPorts::RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events)
{
    if(m_portfd == -1) return false;
    int iRet = 0;
    //MDB_NTC_DEBUG("remove fd[%d] event[%d]", fd, events);fflush(stdout);
    if(events == 0)
    {
        iRet = port_dissociate(m_portfd, PORT_SOURCE_FD, fd);
        if(iRet != 0)
        {
            mdb_ntc_errstr.Snprintf(1024, "RemoveEventMonitor:port_dissociate failed!fd:%u,event:%x,error:%u,%s",fd, events,
                errno, strerror(errno));
            TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
            return false;
        }
    }
    else
    {
        int event = 0;
        TMdbNtcSocket* pSocket = m_pNtcEngine->FindSocketInfo(fd);
        if(pSocket && !pSocket->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
        {
            events = static_cast<TMdbPeerInfo*>(pSocket)->GetPeerFlag(MDB_NTC_PEER_EV_MASK);
            if(events&MDB_NTC_PEER_EV_READ_FLAG)
            {
                event |= POLLIN|POLLPRI;
            }
            if(events&MDB_NTC_PEER_EV_WRITE_FLAG)
            {
                event |= POLLOUT|POLLWRBAND;
            }
        }
        iRet = port_associate(m_portfd, PORT_SOURCE_FD, fd, event, NULL);
        if(iRet != 0)
        {
            mdb_ntc_errstr.Snprintf(1024, "port_associate failed!error:%u,%s", errno, strerror(errno));
            TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
            return false;
        }
    }
    return true;
}

int TMdbProactorEventPorts::Execute()
{
    int iRet = 0;
    if(m_pNtcEngine == NULL)
    {
        return -1;
    }
    m_portfd = port_create();//如果出错返回-1
    if(m_portfd == -1)
    {
        mdb_ntc_errstr.Snprintf(1024, "port_create failed!error:%u,%s", errno, strerror(errno));
        TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
        return false;
    }
    AddEventMonitor(intr_fd[1], 0);
    AddEventMonitor(intr_fd[1], MDB_NTC_PEER_EV_READ_FLAG);
    //port_event_t *events = new port_event_t[OPEN_MAX];
    port_event_t event;
    bool bNeedSyncSet = false;///< 是否需要同步套接字集合
    do 
    {
        if(bNeedSyncSet)
        {
            DealAsyncOperator();
            bNeedSyncSet = false;
        }
        //unsigned int uiReadyCount = OPEN_MAX;
        //if(port_getn(m_portfd, events, OPEN_MAX, &uiReadyCount, NULL) < 0)
        if(port_get(m_portfd, &event, NULL) < 0)
        {
            iRet = errno;
            if(iRet == EINTR)//被信号中断
            {
                iRet = 0;
                continue;
            }
            else
            {
                mdb_ntc_errstr.Snprintf(1024, "port_get failed!error:%u,%s", iRet, strerror(iRet));
                TADD_DETAIL("%s",mdb_ntc_errstr.c_str());
                return iRet;
            }
        }
        QuickMDB_SOCKET fd = 0;
        TADD_DETAIL("fd:%d, portev_source:%d, event:%x", event.portev_object, event.portev_source, event.portev_events);
        if (event.portev_source == PORT_SOURCE_FD)
        {
            fd = event.portev_object;
            if(fd == intr_fd[1])
            {
                bNeedSyncSet = true;
                AddEventMonitor(fd, MDB_NTC_PEER_EV_READ_FLAG);
            }
            else
            {
                TMdbNtcSocket* pSocketInfo = m_pNtcEngine->FindSocketInfo(fd);
                if(pSocketInfo)
                {
                    if(pSocketInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbServerInfo)))
                    {
                        if(OnServerAccept(static_cast<TMdbServerInfo*>(pSocketInfo)))
                        {
                            AddEventMonitor(fd, MDB_NTC_PEER_EV_READ_FLAG);
                        }
                    }
                    else
                    {
                        TMdbPeerInfo* pPeerInfo = static_cast<TMdbPeerInfo*>(pSocketInfo);                        
                        if(event.portev_events & POLLERR)//断开了
                        {
                            pPeerInfo->Disconnect("remote abnormal disconnect", true);
                        }
                        else
                        {
                            if(event.portev_events & (POLLIN|POLLPRI))
                            {
                                if(OnPeerRecv(pPeerInfo))
                                {
                                    AddEventMonitor(fd, MDB_NTC_PEER_EV_READ_FLAG);
                                }
                            }
                            if(event.portev_events & (POLLOUT|POLLWRBAND))
                            {
                                if(OnPeerSend(pPeerInfo))
                                {
                                    AddEventMonitor(fd, MDB_NTC_PEER_EV_READ_FLAG);
                                }
                            }
                        }
                        if(pPeerInfo->IsShutdown())
                        {
                            bNeedSyncSet = true;
                        }
                    }
                }
            }
        }
    } while (1);
    return iRet;
}
#endif
//_NTC_END
//}
