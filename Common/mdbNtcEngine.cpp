#include "Common/mdbNtcEngine.h"
#include "Common/mdbPeerProtocol.h"
#include "Common/mdbNtcSplit.h"
#include "Common/mdbStrUtils.h"
#include "Common/mdbSysTimerManager.h"
#include <new>
#ifndef OS_WINDOWS
#include <netdb.h>
#endif
//namespace /*QuickMDB::*/
//{
//using namespace /*QuickMDB::*/;
mdb_ntc_thread_local(/*QuickMDB::*/::TMdbNtcThreadEvent*, g_pSendThreadEvent);///< send message�������߳��¼�
const TMdbNtcNonProxy g_oMdbNtcNonProxy;
MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcBaseProxy, TMdbNtcBaseObject);
MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcNonProxy, TMdbNtcBaseProxy);
TMdbNtcNonProxy::TMdbNtcNonProxy()
{
}

QuickMDB_SOCKET TMdbNtcNonProxy::Connect(const char* pszRemote, int iPort, int iMilliSeconds /* = -1 */) const
{
    QuickMDB_SOCKET fd = MDB_NTC_INVALID_SOCKET;
    if(!TMdbNtcClientSocket::Connect(fd, pszRemote, iPort, iMilliSeconds))
    {
        mdb_ntc_errstr.Snprintf(1024, "errno:%u,%s", TMdbNtcSocket::GetLastError(), TMdbNtcSocket::GetLastErrorInfo().c_str());
    }
    return fd;
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcHttpProxy, TMdbNtcBaseProxy);
TMdbNtcHttpProxy::TMdbNtcHttpProxy(const char* pszProxyAddress, int iProxyPort, const char* pszUserName /* = NULL */, const char* pszPassword /* = NULL */)
:m_sProxyAddress(pszProxyAddress), m_iProxyPort(iProxyPort), m_sUserName(pszUserName), m_sPassword(pszPassword)
{
}

QuickMDB_SOCKET TMdbNtcHttpProxy::Connect(const char* pszRemote, int iPort, int iMilliSeconds /* = -1 */) const
{
    bool bRet = true;
    QuickMDB_SOCKET fd = MDB_NTC_INVALID_SOCKET;
    do
    {
        if(!TMdbNtcClientSocket::Connect(fd, m_sProxyAddress.c_str(), m_iProxyPort, iMilliSeconds))
        {
            mdb_ntc_errstr.Snprintf(1024, "errno:%u,%s", TMdbNtcSocket::GetLastError(), TMdbNtcSocket::GetLastErrorInfo().c_str());
            break;
        }
        TMdbNtcSocket::SetBlockFlag(fd, true);
        TMdbNtcStringBuffer sConnectBuffer(256, "CONNECT %s:%d HTTP/1.1\r\nAccept: */*\r\nHost: %s:%d\r\nProxy-Connection: Keep-Alive\r\nContent-Length: 0\r\n\r\n",
            pszRemote, iPort, pszRemote, iPort);
        int iSendSize = (int)sConnectBuffer.length();
        bRet = TMdbNtcClientSocket::Send(fd, sConnectBuffer.c_str(), iSendSize, iMilliSeconds);
        if(!bRet) break;
        char szRecvBuffer[1024] = {'\0'};
        int iRecvSize = sizeof(szRecvBuffer)-1;
        bRet = TMdbNtcClientSocket::Recv(fd, szRecvBuffer, iRecvSize, iMilliSeconds);
        if(!bRet)
        {
            mdb_ntc_errstr.Clear()<<"error:"<<TMdbNtcSocket::GetLastError()<<' '<<TMdbNtcSocket::GetLastErrorInfo();
            break;
        }
        else if(iRecvSize == 0)
        {
            bRet = false;
            mdb_ntc_errstr = "recv timeout";
            break;
        }
        char* pszSplit = strstr(szRecvBuffer, "\r\n");
        if(pszSplit) *pszSplit = '\0';
        TMdbNtcSplit oSplit;
        oSplit.SplitString(szRecvBuffer, ' ', true);
        if(oSplit.GetFieldCount() < 2)
        {
            mdb_ntc_errstr = "header format error";
            bRet = false;
            break;
        }
        int iStatusCode = atoi(oSplit[1]);
        if(iStatusCode != 200)
        {
            mdb_ntc_errstr.Clear()<<"response:"<<iStatusCode;
            for (MDB_UINT32 i = 2; i < oSplit.GetFieldCount(); ++i)
            {
                mdb_ntc_errstr<<" "<<oSplit[i];
            }
            bRet = false;
            break;
        }
        //������δ��ȫ��������ȡ���
        do
        {
            iRecvSize = sizeof(szRecvBuffer)-1;
            bRet = TMdbNtcClientSocket::Recv(fd, szRecvBuffer, iRecvSize, 0);
            if(!bRet)
            {
                mdb_ntc_errstr.Clear()<<"error:"<<TMdbNtcSocket::GetLastError()<<' '<<TMdbNtcSocket::GetLastErrorInfo();
                break;
            }
            else if(iRecvSize == 0)
            {
                break;
            }
        } while (1);
        if(!bRet) break;
    } while (0);
    if(fd != MDB_NTC_INVALID_SOCKET && !bRet)
    {
        TMdbNtcSocket::Close(fd);
        fd = MDB_NTC_INVALID_SOCKET;
    }
    return fd;
}
MDB_ZF_IMPLEMENT_OBJECT(TMdbPeerEventPump, TMdbEventPump);
TMdbPeerEventPump::TMdbPeerEventPump()
{
    uiFdCount = 0;
}

TMdbSharedPtr<TMdbPeerInfo>  TMdbNtcEngine::ms_pNullPeerInfo = NULL;
TMdbNtcEngine::TMdbNtcEngine()
{
    m_lsServerInfo.SetAutoRelease(true);
    m_pSocketProactor   =   NULL;
    m_ppPeerNo = NULL;
    memset(m_arraySocketInfo, 0x00, sizeof(m_arraySocketInfo));
    SetMaxPeerNo(256);//Ĭ��256
}

TMdbNtcEngine::~TMdbNtcEngine()
{
    Stop();
    m_lsServerInfo.Clear();
}

bool TMdbNtcEngine::IsStart()
{
    return m_pSocketProactor && m_pSocketProactor->GetThreadHandle();
}

bool TMdbNtcEngine::InitSocketProactor()
{
#ifdef _WIN32
    return InitSocketProactor(MDB_PROACTOR_IOCP);
#elif defined(OS_LINUX)
    return InitSocketProactor(MDB_PROACTOR_EPOLL);
#elif defined(OS_SUN)
    return InitSocketProactor(MDB_PROACTOR_EVENT_PORTS);    
#elif defined(OS_FREEBSD)
    return InitSocketProactor(MDB_PROACTOR_KQUEUE);
#elif defined(OS_IBM)
    return InitSocketProactor(MDB_PROACTOR_POLLSET);
#elif defined(OS_UNIX)
    return InitSocketProactor(MDB_PROACTOR_POLL);
#else
    return InitSocketProactor(MDB_PROACTOR_SELECT);
#endif
}

bool TMdbNtcEngine::InitSocketProactor(MDB_PEER_PROACTOR_TYPE emProactorType)
{
    if(m_pSocketProactor)
    {
        return true;
    }
    //emProactorType = MDB_PROACTOR_SELECT;
    switch(emProactorType)
    {
    case MDB_PROACTOR_SELECT:
        m_pSocketProactor = new TMdbProactorSelect;
        break;
#ifndef _WIN32
    case MDB_PROACTOR_POLL:
        m_pSocketProactor = new TMdbProactorPoll;
        break;
#endif
        
#if defined(OS_LINUX) || defined(_LINUX)
    case MDB_PROACTOR_EPOLL:
        m_pSocketProactor = new TMdbProactorEpoll;
        break;
#elif defined(OS_SUN)
    case MDB_PROACTOR_EVENT_PORTS:
        m_pSocketProactor = new TMdbProactorEventPorts;
        break;
#elif defined(_WIN32)
    case MDB_PROACTOR_IOCP:
        m_pSocketProactor = new TMdbProactorIocp;
        break;
#elif defined(OS_FREEBSD) || defined(_FREEBSD)
    case MDB_PROACTOR_KQUEUE:
        m_pSocketProactor = new TMdbProactorKqueue;
        break;
#elif defined(OS_IBM)
    case MDB_PROACTOR_POLLSET:
        m_pSocketProactor = new TMdbProactorPollset;
        break;
#endif
    default:
        break;
    }
    if(m_pSocketProactor == NULL)
    {
        mdb_ntc_errstr = "There is no matching proactor type";
        return false;
    }
    else
    {
        //Ϊǰ�������÷ַ���
        bool bRet = m_pSocketProactor->Init(this);
        if(bRet == false && m_pSocketProactor)
        {
            delete m_pSocketProactor;
            m_pSocketProactor = NULL;
        }
        return bRet;
    }
}

void TMdbNtcEngine::SetMaxPeerNo(MDB_UINT16 uiMaxPeerNo)
{
    if(uiMaxPeerNo > GetMaxPeerNo())
    {
        TMdbSharedPtr<TMdbPeerInfo>* ppPeerNo = (TMdbSharedPtr<TMdbPeerInfo>*)new char[sizeof(TMdbSharedPtr<TMdbPeerInfo>)*uiMaxPeerNo];//������Ĺ��캯��
        //TMdbSharedPtr<TMdbPeerInfo>
        //memset(ppPeerNo, 0x00, sizeof(TMdbSharedPtr<TMdbPeerInfo>)*uiMaxPeerNo);
        m_oSpinLock.Lock();        
        TMdbSharedPtr<TMdbPeerInfo>* ppPeerNoPre = m_ppPeerNo;
        if(GetMaxPeerNo() > 0)
        {
            memcpy(ppPeerNo, m_ppPeerNo, sizeof(TMdbSharedPtr<TMdbPeerInfo>)*GetMaxPeerNo());
        }
        for (MDB_UINT32 i = GetMaxPeerNo(); i < uiMaxPeerNo; ++i)
        {
            new (ppPeerNo+i) TMdbSharedPtr<TMdbPeerInfo>;
        }
        m_ppPeerNo = ppPeerNo;
        m_oIdMgr.SetPoolSize(uiMaxPeerNo);
        m_oSpinLock.Unlock();
        if(ppPeerNoPre)
        {
            delete [](char*)ppPeerNoPre;
            ppPeerNoPre = NULL;
        }
    }
    else
    {
        m_oIdMgr.SetPoolSize(uiMaxPeerNo);
    }
}

TMdbPeerInfo* TMdbNtcEngine::CreatePeerInfo()
{
    return new TMdbPeerInfo();
}

TMdbSharedPtr<TMdbPeerInfo> TMdbNtcEngine::AddPeerInfo(QuickMDB_SOCKET new_fd, TMdbServerInfo* pServerInfo, MDB_UINT16 uiPeerFlag /* = 0 */)
{
    int iPno = -1;
    TMdbSharedPtr<TMdbPeerInfo> pRetPeerInfo = NULL;
    m_oSpinLock.Lock();
    if(m_oIdMgr.AllocRes(iPno))
    {
        TMdbNtcSocket::SetBlockFlag(new_fd, false);//����Ϊ������
        TMdbPeerInfo* pPeerInfo = NULL;
        if(pServerInfo->pPeerRuntimeObject)
        {
            TMdbNtcBaseObject* pNewObj = pServerInfo->pPeerRuntimeObject->CreateObject();
            //Ϊ�˷�ֹpeer������δʹ��ZF_DECLARE_DYNCREATE_OBJECT�����嶯̬������������Ҫ�Ƚ϶����GetRuntimeObject�Ƿ�ƥ��
            if(pNewObj &&  pNewObj->GetRuntimeObject() != pServerInfo->pPeerRuntimeObject)
            {
                m_oIdMgr.FreeRes(iPno);
                delete pNewObj;
                pNewObj = NULL;
                return NULL;
            }
            pPeerInfo = MDB_ZF_DYNAMIC_CAST(TMdbPeerInfo, pNewObj);
        }
        else pPeerInfo = CreatePeerInfo();
        if(pPeerInfo == NULL)
        {
            m_oIdMgr.FreeRes(iPno);
            return NULL;
        }
        pPeerInfo->Init(new_fd, pServerInfo);
        pPeerInfo->pno = (MDB_UINT16)iPno;
        if(uiPeerFlag != 0) pPeerInfo->SetPeerFlag(uiPeerFlag);
        pRetPeerInfo = m_ppPeerNo[iPno] = pPeerInfo;        
        m_arraySocketInfo[new_fd] = pPeerInfo;
        pPeerInfo->pPeerProactor = m_pSocketProactor;
        pPeerInfo->AddEventMonitor(0);
    }
    else if(pServerInfo->GetSocketID() == MDB_NTC_INVALID_SOCKET)
    {
        delete pServerInfo;
        pServerInfo = NULL;
    }
    m_oSpinLock.Unlock();
    if(pRetPeerInfo && m_pSocketProactor)
    {
        TMdbConnectEvent* pConnectSocketInfo = new TMdbConnectEvent(pRetPeerInfo);
        pConnectSocketInfo->pProactor = m_pSocketProactor;
        m_pSocketProactor->GetEventDispatcher()->Dispatch(pConnectSocketInfo);
    }
    return pRetPeerInfo;
}

void TMdbNtcEngine::DelPeerInfo(TMdbPeerInfo* pPeerInfo)
{
    if(pPeerInfo)
    {
        MDB_UINT16 pno = pPeerInfo->pno;
        m_oSpinLock.Lock();
        if(m_ppPeerNo && pPeerInfo == m_ppPeerNo[pno])
        {
            m_arraySocketInfo[pPeerInfo->GetSocketID()] = NULL;
            pPeerInfo->Close();//�ر�socket
            m_ppPeerNo[pno] = NULL;//���ù����У�������Release
            m_oIdMgr.FreeRes(pno);
        }
        m_oSpinLock.Unlock();
    }
}

MDB_UINT32 TMdbNtcEngine::GetPeerAllowRecvBytes(TMdbPeerInfo* pPeerInfo, MDB_UINT32 uiRecvBytes)
{
    MDB_UINT32 uiRet = m_oTotalTrafficCtrl.GetAllowRecvBytes(&m_oTotalTrafficInfo, uiRecvBytes);
    //MDB_NTC_DEBUG("total traffic[%d], [%d]", uiRet, uiRecvBytes);    
    //�뵥�����ӵ��������
    TMdbTrafficCtrl* pTrafficCtrl = pPeerInfo->GetTrafficCtrl();
    if(pTrafficCtrl == NULL)
    {
        pTrafficCtrl = &m_oPeerTrafficCtrl;
    }
    uiRet = pTrafficCtrl->GetAllowRecvBytes(pPeerInfo->GetTrafficInfo(), uiRet);
    //MDB_NTC_DEBUG("peer traffic[%d], [%d]", uiRet, uiRecvBytes);
    return uiRet;
}

MDB_UINT32 TMdbNtcEngine::GetPeerAllowSendBytes(TMdbPeerInfo* pPeerInfo, MDB_UINT32 uiSendBytes)
{
    MDB_UINT32 uiRet = m_oTotalTrafficCtrl.GetAllowSendBytes(&m_oTotalTrafficInfo, uiSendBytes);
    //�뵥�����ӵ��������
    TMdbTrafficCtrl* pTrafficCtrl = pPeerInfo->GetTrafficCtrl();
    if(pTrafficCtrl == NULL)
    {
        pTrafficCtrl = &m_oPeerTrafficCtrl;
    }
    uiRet = pTrafficCtrl->GetAllowSendBytes(pPeerInfo->GetTrafficInfo(), uiRet);
    return uiRet;
}

TMdbServerInfo* TMdbNtcEngine::FindServerInfo(const char* pszIP, int iPort)
{
    TMdbServerInfo serverInfo(pszIP, iPort);
    TMdbNtcBaseList::iterator itor = m_lsServerInfo.IterFind(serverInfo, TMdbServerInfoCompare());
    if(itor != m_lsServerInfo.IterEnd())
    {
        return static_cast<TMdbServerInfo*>(itor.data());
    }
    else
    {
        return NULL;
    }
}

bool TMdbNtcEngine::Start()
{
    bool bRet = true;
    do
    {
        //�����¼���,���û���κ��¼��õ�����£���ǰ���������¼���ֱ�ӵ��ô�������ProcessEvent
        for (MDB_UINT32 i = 0; i < m_arrayPump.GetSize(); ++i)
        {
            TMdbEventPump* pEventPump = static_cast<TMdbEventPump*>(m_arrayPump.GetAt(i));
            if(!pEventPump->Run())//�����¼���ʧ��
            {
                mdb_ntc_errstr.Snprintf(1024, "start event pump %u[%s] failed!", i, pEventPump->GetObjectName());
                bRet = false;
                break;
            }
        }
        if(bRet == false)
        {
            break;
        }        
        //�����û�г�ʼ��ǰ����������ݻ����Զ���ʼ��һ��
        if(m_pSocketProactor == NULL)
        {
            bRet = InitSocketProactor();
            if(bRet == false)
            {
                break;
            }
        }
        if(!m_lsServerInfo.IsEmpty())
        {
            //���Ѿ�add listen�ķ���ˣ���ӵ�ǰ������
            TMdbNtcBaseList::iterator itor = m_lsServerInfo.IterBegin(), itEnd = m_lsServerInfo.IterEnd();
            for (; itor != itEnd; ++itor)
            {
                TMdbServerInfo* pServerInfo = static_cast<TMdbServerInfo*>(itor.data());
                pServerInfo->pPeerProactor = m_pSocketProactor;
                if(pServerInfo->GetSocketID() > 0)
                {
                    pServerInfo->pPeerProactor = m_pSocketProactor;
                    pServerInfo->AddEventMonitor(0);
                    pServerInfo->AddEventMonitor(MDB_NTC_PEER_EV_READ_FLAG);
                }
            }
        }
        if(m_ppPeerNo)
        {
            TMdbPeerInfo* pPeerInfo = NULL;
            for (MDB_UINT16 i = 0; i < GetMaxPeerNo(); ++i)
            {
                pPeerInfo = m_ppPeerNo[i];
                if(pPeerInfo && pPeerInfo->GetSocketID() > 0 && !pPeerInfo->IsShutdown())
                {
                    pPeerInfo->pPeerProactor = m_pSocketProactor;
                    pPeerInfo->AddEventMonitor(0);
                    TMdbConnectEvent* pConnectSocketInfo = new TMdbConnectEvent(pPeerInfo);
                    pConnectSocketInfo->pProactor = m_pSocketProactor;
                    m_pSocketProactor->GetEventDispatcher()->Dispatch(pConnectSocketInfo);
                }
            }
        }
        if(!m_pSocketProactor->Run())
        {
            mdb_ntc_errstr = "start socket proactor failed";
            bRet = false;
            //����ǰ����ʧ��
            break;
        }
    } while (0);
    return bRet;
}

bool TMdbNtcEngine::Stop()
{
    //ֹͣsocketǰ����
    if(m_pSocketProactor && m_pSocketProactor->GetThreadState() != MDB_NTC_THREAD_EXIT)
    {
        m_pSocketProactor->Stop();
    }
    //Ȼ����ֹͣ�¼���
    for (unsigned int i = 0; i < m_arrayPump.GetSize(); ++i)
    {
        TMdbEventPump* pEventPump = static_cast<TMdbEventPump*>(m_arrayPump.GetAt(i));
        if(pEventPump->GetThreadState() == MDB_NTC_THREAD_RUN || pEventPump->GetThreadState() == MDB_NTC_THREAD_SUSPEND)
        {
            pEventPump->Cancel();//�����˳�flag
            pEventPump->Wait();//�ȴ��߳��˳�
        }
    }
    //ɾ�����е�����
    if(m_ppPeerNo)
    {
        TMdbPeerInfo* pPeerInfo = NULL;
        for (int i = 0; i < GetMaxPeerNo(); ++i)
        {
            pPeerInfo = m_ppPeerNo[i];
            if(pPeerInfo)
            {
                if(pPeerInfo->GetSocketID() > 0)
                {
                    m_arraySocketInfo[pPeerInfo->GetSocketID()] = NULL;
                }
                m_ppPeerNo[i].~TMdbSharedPtr<TMdbPeerInfo>();
            }
        }
        delete [](char*)m_ppPeerNo;//���Զ����ø���Ԫ�ص�������������������release
        m_ppPeerNo = NULL;
    }
    if(m_pSocketProactor)
    {
        delete m_pSocketProactor;
        m_pSocketProactor = NULL;
    }
    return true;
}

//ǿ��ɱ���߳�
bool TMdbNtcEngine::Kill()
{
    //ɱ��socketǰ����
    if(m_pSocketProactor)
    {
        m_pSocketProactor->Kill();
    }
    //Ȼ����ֹͣ�¼���
    for (unsigned int i = 0; i < m_arrayPump.GetSize(); ++i)
    {
        TMdbEventPump* pEventPump = static_cast<TMdbEventPump*>(m_arrayPump.GetAt(i));
        if(pEventPump)
        {
            pEventPump->Kill();
        }
    }
    //�ͷ����е�������Դ
    if(m_ppPeerNo)
    {
        TMdbPeerInfo* pPeerInfo = NULL;
        for (int i = 0; i < GetMaxPeerNo(); ++i)
        {
            pPeerInfo = m_ppPeerNo[i];
            if(pPeerInfo)
            {
                if(pPeerInfo->GetSocketID() > 0)
                {
                    m_arraySocketInfo[pPeerInfo->GetSocketID()] = NULL;
                }
                m_ppPeerNo[i].~TMdbSharedPtr<TMdbPeerInfo>();
            }
        }
        delete [](char*)m_ppPeerNo;//���Զ����ø���Ԫ�ص�������������������release
        m_ppPeerNo = NULL;
    }
    if(m_pSocketProactor)
    {
        delete m_pSocketProactor;
        m_pSocketProactor = NULL;
    }
    return true;
}



TMdbSharedPtr<TMdbPeerInfo> TMdbNtcEngine::Connect(const char* pszRemote, int iPort, TMdbProtocol* pProtocol /* = NULL */, int iMilliSeconds /* = -1 */,
                                          MDB_UINT16 uiPeerFlag /* = 0 */, const /*QuickMDB::*/::TMdbRuntimeObject* pPeerRuntimeObject /* = NULL */, const TMdbNtcBaseProxy& oProxy /* = g_oMdbNtcNonProxy */)
{
    if(GetMaxPeerNo() == GetPeerCount())
    {
        //�ﵽ����������
        mdb_ntc_errstr = "reach peer limit";
        return NULL;
    }
    QuickMDB_SOCKET fd = oProxy.Connect(pszRemote, iPort, iMilliSeconds);
    if(fd == MDB_NTC_INVALID_SOCKET) return NULL;
    return AddPeerInfo(fd, new TMdbServerInfo(pszRemote, iPort, pProtocol, pPeerRuntimeObject), uiPeerFlag);
}

bool TMdbNtcEngine::AddListen(const char* pszAddress, int iPort, TMdbProtocol* pProtocol, const /*QuickMDB::*/::TMdbRuntimeObject* pPeerRuntimeObject /* = NULL */)
{
    bool bRet = true;
    TMdbServerInfo* pServerInfo = new TMdbServerInfo(pszAddress, iPort, pProtocol, pPeerRuntimeObject);
    if(!pServerInfo->Listen(pszAddress, iPort))
    {
        mdb_ntc_errstr.Snprintf(1024, "errno:%u,%s", TMdbNtcSocket::GetLastError(), TMdbNtcSocket::GetLastErrorInfo().c_str());
        bRet = false;
        delete pServerInfo;
        pServerInfo = NULL;
    }
    else
    {        
        //����Ϊ������
        pServerInfo->SetBlockFlag(false);
        m_oSpinLock.Lock();
        m_lsServerInfo.AddTail(pServerInfo);
        m_oSpinLock.Unlock();
        pServerInfo->pPeerProactor = m_pSocketProactor;
        m_arraySocketInfo[pServerInfo->GetSocketID()] = pServerInfo;
        if(m_pSocketProactor)
        {
            pServerInfo->AddEventMonitor(0);
            pServerInfo->AddEventMonitor(MDB_NTC_PEER_EV_READ_FLAG);
        }
    }
    return bRet;
}

bool TMdbNtcEngine::Disconnect(MDB_UINT16 pno)
{
    bool bRet = true;
    TMdbPeerInfo* pPeerInfo = FindPeerInfo(pno);
    if(pPeerInfo)
    {
        bRet = pPeerInfo->Disconnect();
    }
    else
    {
        bRet = false;
    }
    return bRet;
}

TMdbEventPump* TMdbNtcEngine::CreateEventPump()
{
    return new TMdbPeerEventPump;
}

bool TMdbNtcEngine::ProcessEvent(TMdbEventInfo* pEventInfo, TMdbEventPump* pEventPump /* = NULL */)
{
    bool bRet = true;
    //MDB_NTC_DEBUG("****process[%s] begin...", pEventInfo->GetObjectName());
    if(pEventInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbRecvMsgEvent)))
    {
        TMdbRecvMsgEvent* pRecvMsgEvent = static_cast<TMdbRecvMsgEvent*>(pEventInfo);
        if(pRecvMsgEvent->pPeerInfo->IsMsgDetached())
        {            
            bRet = pRecvMsgEvent->pPeerInfo->AddRecvMessage(pRecvMsgEvent->pMsgInfo);
        }
        else
        {
            bRet = OnRecvMsg(pRecvMsgEvent, pEventPump);
        }
    }
    else if(pEventInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbConnectEvent)))
    {
        TMdbConnectEvent* pConnectEvent = static_cast<TMdbConnectEvent*>(pEventInfo);
        TMdbPeerInfo* pPeerInfo = pConnectEvent->pPeerInfo;
        int iRecvBufSize, iSendBufSize;
        TMdbNtcSocket::GetRecvBufSize(pPeerInfo->GetSocketID(), iRecvBufSize);
        TMdbNtcSocket::GetSendBufSize(pPeerInfo->GetSocketID(), iSendBufSize);
        if(iRecvBufSize < 65536)
        {
            TMdbNtcSocket::SetRecvBufSize(pPeerInfo->GetSocketID(), 65536);
        }
        if(iSendBufSize < 65536)
        {
            TMdbNtcSocket::SetSendBufSize(pPeerInfo->GetSocketID(), 65536);
        }
        //TMdbNtcSocket::GetRecvBufSize(pPeerInfo->GetSocketID(), iRecvBufSize);
        //TMdbNtcSocket::GetSendBufSize(pPeerInfo->GetSocketID(), iSendBufSize);
        bRet = OnConnect(pConnectEvent, pEventPump);
        if(bRet && !pPeerInfo->IsShutdown())
        {
            pPeerInfo->AddEventMonitor(MDB_NTC_PEER_EV_READ_FLAG);
        }
    }
    else if(pEventInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbDisconnectEvent)))
    {
        TMdbDisconnectEvent* pDisconnectEvent = static_cast<TMdbDisconnectEvent*>(pEventInfo);
        bRet = OnDisconnect(pDisconnectEvent, pEventPump);
        pDisconnectEvent->pPeerInfo->SetMsgHandleMode(false);
        DelPeerInfo(pDisconnectEvent->pPeerInfo);
        if(pEventPump && pEventPump->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbPeerEventPump)))
        {
            this->m_oSpinLock.Lock();
            --static_cast<TMdbPeerEventPump*>(pEventPump)->uiFdCount;
            this->m_oSpinLock.Unlock();
        }
    }
    else if(pEventInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbTimeoutEvent)))
    {
        bRet = OnTimeOut(static_cast<TMdbTimeoutEvent*>(pEventInfo), pEventPump);
    }
    else if(pEventInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbErrorEvent)))
    {
        bRet = OnError(static_cast<TMdbErrorEvent*>(pEventInfo), pEventPump);
    }
    else
    {
        bRet = OnOtherEvent(pEventInfo, pEventPump);
    }
    TADD_DETAIL("****process[%s] end", pEventInfo->GetObjectName());
    pEventInfo->Release(0);
    return bRet;
}

bool TMdbNtcEngine::OnConnect(TMdbConnectEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    return true;
}

bool TMdbNtcEngine::OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    return true;
}

/*
bool TMdbNtcEngine::OnRead(TMdbPeerEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    if(!pEventInfo->pPeerInfo->pPeerProactor->OnPeerRecv(pEventInfo->pPeerInfo))
    {
        pEventInfo->pPeerInfo->AddEventMonitor(MDB_NTC_PEER_EV_READ_FLAG);
    }
    return true;
}

bool TMdbNtcEngine::OnWrite(TMdbPeerEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    if(!pEventInfo->pPeerInfo->pPeerProactor->OnPeerSend(pEventInfo->pPeerInfo))
    {
        pEventInfo->pPeerInfo->AddEventMonitor(MDB_NTC_PEER_EV_WRITE_FLAG);
    }
    return true;
}
*/

bool TMdbNtcEngine::OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    return true;
}

bool TMdbNtcEngine::OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    //MDB_NTC_DEBUG("OnTimeOut:%ld", time(NULL)-pEventInfo->pPeerInfo->GetConnectTime());
    return true;
}

bool TMdbNtcEngine::OnError(TMdbErrorEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    printf("error:%s\n", pEventInfo->sErrorInfo.c_str());
    return true;
}

TMdbEventPump* TMdbNtcEngine::Dispatch(TMdbEventInfo* pEventInfo)
{
    pEventInfo->pEventHandler = this;
    return TMdbEventDispatcher::Dispatch(pEventInfo);
}

TMdbEventPump* TMdbNtcEngine::PreDispatch(TMdbEventInfo* pEventInfo)
{
    if(pEventInfo->IsDerivedFrom(MDB_ZF_RUNTIME_OBJECT(TMdbPeerEvent)))
    {
        //�����socket�¼�������Ҫ����fd��ǰ����ͨ�����ַ�
        TMdbPeerEvent* pPeerEvent = static_cast<TMdbPeerEvent*>(pEventInfo);
        TMdbEventPump* pEventPump = NULL;
        if(pPeerEvent->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbConnectEvent)))
        {
            TMdbPeerEventPump* pLeastEventPump = NULL;
            //ѡ���¼��ã�����ÿ���¼�������������fd��ѡ��
            for (unsigned int i = 0; i < m_arrayPump.GetSize(); ++i)
            {
                TMdbEventPump* pEventPump = static_cast<TMdbEventPump*>(m_arrayPump.GetAt(i));
                if(pEventPump->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbPeerEventPump)))
                {
                    if(pLeastEventPump == NULL || pLeastEventPump->uiFdCount > static_cast<TMdbPeerEventPump*>(pEventPump)->uiFdCount)
                    {
                        pLeastEventPump = static_cast<TMdbPeerEventPump*>(pEventPump);
                    }
                }
            }
            if(pLeastEventPump)
            {
                pEventPump = pLeastEventPump;
                this->m_oSpinLock.Lock();
                ++pLeastEventPump->uiFdCount;
                this->m_oSpinLock.Unlock();
            }
            else
            {
                pEventPump = TMdbEventDispatcher::PreDispatch(pEventInfo);
            }
            pPeerEvent->pPeerInfo->pCurEventPump = pEventPump;
        }
        else
        {
            TMdbPeerInfo* pPeerInfo = pPeerEvent->pPeerInfo;
            if(pPeerInfo)
            {
                //��������Ѿ��رգ��򷵻�NULL
                pPeerInfo->Lock();
                if(!pPeerInfo->IsShutdown() || pPeerEvent->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbDisconnectEvent)))
                {
                    //����socketѡ���ĸ�ͨ������Ҫ�����ʵ��Ļ���
                    if(pPeerInfo->pCurEventPump)
                    {
                        pEventPump = pPeerInfo->pCurEventPump;         
                    }
                    else
                    {
                        pEventPump = TMdbEventDispatcher::PreDispatch(pEventInfo);
                        pPeerInfo->pCurEventPump = pEventPump;
                    }
                }
                pPeerInfo->Unlock();
            }
        }        
        return pEventPump;
    }
    else
    {
        return TMdbEventDispatcher::PreDispatch(pEventInfo);
    }    
}

TMdbSyncEngine::TMdbSyncEngine()
{
    m_oMsgQueue.SetAutoRelease(true);
}

TMdbSyncEngine::~TMdbSyncEngine()
{
    Stop();
    m_oMsgQueue.Clear();
}

bool TMdbSyncEngine::OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    pEventInfo->AddRef();//�¼���������߳��д����������ô���+1
    m_oMsgQueue.Push(pEventInfo);
    return true;
}

bool TMdbSyncEngine::ProcessEvent(TMdbEventInfo* pEventInfo, TMdbEventPump* pEventPump)
{
    bool bDisconnect = pEventInfo->IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbDisconnectEvent));
    bool bRet = TMdbNtcEngine::ProcessEvent(pEventInfo, pEventPump);
    if(bDisconnect)
    {
        //�ж��Ƿ��������Ӷ��ѹر�
        if(GetPeerCount() == 0)
        {
            m_oMsgQueue.Wakeup();
        }
    }
    return bRet;
}

//_NTC_END
//}
