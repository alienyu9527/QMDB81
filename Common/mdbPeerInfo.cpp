#include "Common/mdbPeerInfo.h"
#include "Common/mdbPeerProactor.h"
#include "Common/mdbPeerEvent.h"
#include "Common/mdbPeerProtocol.h"
#include "Common/mdbSysThreads.h"
#include "Common/mdbSysTimerManager.h"
//namespace QuickMDB
//{
//using namespace BillingSDK;
struct TMdbNtcCheckPeerTimeout
{
    TMdbEventDispatcher* pEventDispatcher;
    TMdbPeerInfo* pPeerInfo;
    MDB_UINT32 uiSeconds;
    MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutType;
};

TMdbTrafficCtrl::TMdbTrafficCtrl()
{
    m_uiMaxRecvRate = m_uiMaxSendRate = m_uiMaxFlowRate = 0;
}

MDB_UINT32 TMdbTrafficCtrl::GetAllowRecvBytes(TMdbTrafficInfo* pTrafficInfo, MDB_UINT32 uiRecvBytes)
{
    if(m_uiMaxRecvRate+m_uiMaxFlowRate == 0) return uiRecvBytes;
    MDB_UINT32 uiRet = uiRecvBytes;
    //MDB_NTC_DEBUG("m_tLastRecvTime[%d],m_uiSectionRecvBytes:[%u], m_uiSectionSendBytes[%u]", pTrafficInfo->GetLastRecvTime(), pTrafficInfo->GetCurSecRecvBytes(),
    //    pTrafficInfo->GetCurSecSendBytes());
    MDB_UINT32 uiMaxRecvRate = m_uiMaxRecvRate;    
    if(uiMaxRecvRate > 0)
    {
        MDB_UINT32 uiTrafficRecvBytes = pTrafficInfo->GetCurSecRecvBytes();
        if(uiTrafficRecvBytes >= uiMaxRecvRate)
        {
            return 0;
        }
        else if(uiRet+uiTrafficRecvBytes > uiMaxRecvRate)
        {            
            uiRet = uiMaxRecvRate-uiTrafficRecvBytes;
        }
    }
    MDB_UINT32 uiMaxFlowRate = m_uiMaxFlowRate;
    if(uiMaxFlowRate > 0)
    {
        MDB_UINT32 uiTrafficTotalBytes = pTrafficInfo->GetCurSecTotalBytes();
        if(uiTrafficTotalBytes >= uiMaxFlowRate)
        {
            return 0;
        }
        else if(uiRet+uiTrafficTotalBytes > uiMaxFlowRate)
        {
            uiRet = uiMaxFlowRate-uiTrafficTotalBytes;
        }
    }
    return uiRet;
}

MDB_UINT32 TMdbTrafficCtrl::GetAllowSendBytes(TMdbTrafficInfo* pTrafficInfo, MDB_UINT32 uiSendBytes)
{
    if(m_uiMaxSendRate+m_uiMaxFlowRate == 0) return uiSendBytes;
    MDB_UINT32 uiRet = uiSendBytes;
    MDB_UINT32 uiMaxSendRate = m_uiMaxSendRate;
    if(uiMaxSendRate > 0)
    {
        MDB_UINT32 uiTrafficSendBytes = pTrafficInfo->GetCurSecSendBytes();
        if(uiTrafficSendBytes >= uiMaxSendRate)
        {
            return 0;
        }
        else if(uiRet+uiTrafficSendBytes > uiMaxSendRate)
        {            
            uiRet = uiMaxSendRate-uiTrafficSendBytes;
        }
    }
    MDB_UINT32 uiMaxFlowRate = m_uiMaxFlowRate;
    if(uiMaxFlowRate > 0)
    {
        MDB_UINT32 uiTrafficTotalBytes = pTrafficInfo->GetCurSecTotalBytes();
        if(uiTrafficTotalBytes >= uiMaxFlowRate)
        {
            return 0;
        }
        else if(uiRet+uiTrafficTotalBytes > uiMaxFlowRate)
        {
            uiRet = uiMaxFlowRate-uiTrafficTotalBytes;
        }
    }
    return uiRet;
}

struct TMdbPeerEventMonitor:public /*QuickMDB::*/TMdbNtcBaseObject
{
    TMdbSharedPtr<TMdbPeerInfo> pPeerInfo;
    MDB_UINT16 events;
};

///*QuickMDB::*/TMdbNtcQueue TMdbTrafficCtrl::g_oSuspendMonitorPeerQueue(true, false);
/*QuickMDB::*/TMdbNtcQueue TMdbTrafficCtrl::g_oSuspendMonitorPeerQueue;

void TMdbTrafficCtrl::ResumePeerTraffic()
{
    if(!TMdbTrafficCtrl::g_oSuspendMonitorPeerQueue.IsEmpty())
    {
        static TMdbPeerProactor* ppPeerProactor[1024]={NULL};
        TMdbPeerEventMonitor* pEventMonitor = NULL;
        int iCount = 0;
        do
        {
            pEventMonitor = static_cast<TMdbPeerEventMonitor*>(TMdbTrafficCtrl::g_oSuspendMonitorPeerQueue.Pop());
            if(pEventMonitor == NULL) break;
            pEventMonitor->pPeerInfo->AddEventMonitor(pEventMonitor->events);
            TMdbPeerProactor* pPeerProactor = pEventMonitor->pPeerInfo->pPeerProactor;
            if(pPeerProactor)
            {                
                delete pEventMonitor;
                pEventMonitor = NULL;
                for(int i = 0; i < iCount; ++i)
                {
                    if(pPeerProactor == ppPeerProactor[i])
                    {
                        pPeerProactor = NULL;
                        break;
                    }
                }
                if(pPeerProactor)
                {
                    ppPeerProactor[iCount++] = pPeerProactor;
                }
            }            
        }while(1);
        //将牵扯到的前摄器都唤醒
        for (int i = 0; i < iCount; ++i)
        {
            ppPeerProactor[i]->Wakeup();
        }        
    }
}

void TMdbTrafficCtrl::SuspendPeerTraffic(TMdbSharedPtr<TMdbPeerInfo>& pPeerInfo, MDB_UINT16 events)
{
    TMdbPeerEventMonitor* pEventMonitor = new TMdbPeerEventMonitor;
    pEventMonitor->pPeerInfo = pPeerInfo;
    pEventMonitor->events = events;
    g_oSuspendMonitorPeerQueue.Push(pEventMonitor);
}

/*QuickMDB::*/TMdbNtcThreadLock TMdbPacketInfo::ms_oSpinLock;
MDB_ZF_IMPLEMENT_OBJECT(TMdbPacketInfo, TMdbNtcBaseObject);
TMdbPacketInfo::TMdbPacketInfo(MDB_UINT32 uiLength /* = 0 */)
{
    uiOffset = 0;
    m_pcData = NULL;
    this->uiLength = uiLength;
    if(uiLength > 0)
    {
        AllocBuffer(uiLength);
    }
}

TMdbPacketInfo::TMdbPacketInfo(char* pcBuffer, MDB_UINT32 uiLength /* = -1 */)
{
    m_pcData = NULL;
    this->uiLength = 0;
    uiOffset = 0;
    if(uiLength == (MDB_UINT32)-1)
    {
        uiLength = (MDB_UINT32)strlen(pcBuffer);
    }
    if(uiLength > 0)
    {
        char* pcDataBuffer = AllocBuffer(uiLength);
        if(pcBuffer)
        {
            memcpy(pcDataBuffer, pcBuffer, uiLength);
        }
    }
}

TMdbPacketInfo::TMdbPacketInfo(TMdbPacketInfo& oPacketInfo, MDB_UINT32 uiLength /* = -1 */)
{
    uiOffset = 0;
    m_pcData = NULL;
    this->uiLength = uiLength;
    Attach(oPacketInfo, uiLength);
}

void TMdbPacketInfo::Attach(TMdbPacketInfo& oPacketInfo, MDB_UINT32 uiLength /* = -1 */)
{
    Detach();
    m_pcData = oPacketInfo.AddRef();
    uiOffset = oPacketInfo.uiOffset;
    if(uiLength == (MDB_UINT32)-1)
    {
        this->uiLength = oPacketInfo.uiLength;
    }
    else
    {        
        //iOffset = ((TPacketData*)oPacketInfo.pcData)->iLength-oPacketInfo.iLength-oPacketInfo.iOffset;
        this->uiLength = uiLength;
    }
}

char* TMdbPacketInfo::AllocBuffer(MDB_UINT32 uiLength)
{
    if(m_pcData)
    {
        Detach();
    }
    m_pcData = new char[uiLength+sizeof(TPacketData)+1];
    memset(m_pcData, 0x00, uiLength+sizeof(TPacketData));
    m_pcData[uiLength+sizeof(TPacketData)] = '\0';
    AddRef();
    this->uiLength = uiLength;
    uiOffset = 0;
    return GetBuffer();
}

void TMdbPacketInfo::Detach()
{
    if(m_pcData)
    {
        ms_oSpinLock.Lock();
        MDB_UINT32 uiRefcnt = --reinterpret_cast<TPacketData*>(m_pcData)->uiRefcnt;
        ms_oSpinLock.Unlock();
        TADD_DETAIL("Detach[%p] ref_cnt:%d", m_pcData, uiRefcnt);
        if(uiRefcnt != 0)
        {
            m_pcData = NULL;
        }
        else
        {
            delete []m_pcData;
            m_pcData = NULL;
        }
    }
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbMsgInfo, TMdbNtcBaseObject);
TMdbMsgInfo::TMdbMsgInfo(MDB_UINT32 uiLength /* = 0 */)
:oPacketInfo(uiLength)
{
    m_uiRefcnt      =   1;
    m_uiHeadLength  =   0;
}
TMdbRecvPackets::TMdbRecvPackets()
{
    m_uiTotalLength = 0;
    uiSplicedLength = 0;
    pSplicingMsg = NULL;
    SetAutoRelease(true);
}

TMdbRecvPackets::~TMdbRecvPackets()
{
    Clear();
    if(pSplicingMsg)
    {
        pSplicingMsg->Release();
        pSplicingMsg = NULL;
    }
}

bool TMdbRecvPackets::SplicePacket(MDB_UINT32 uiLength, TMdbPacketInfo& oPacketInfo)
{
    if(m_uiTotalLength < uiLength)
    {
        return false;
    }    
    char* pcBuffer = NULL;
    MDB_UINT32 uiCurLength = 0;
    if(oPacketInfo.uiLength <= uiLength)
    {
        pcBuffer = oPacketInfo.AllocBuffer(oPacketInfo.uiLength);
    }
    else
    {
        uiCurLength = oPacketInfo.uiLength-uiLength;
        pcBuffer = oPacketInfo.GetBuffer();
    }
    TMdbPacketInfo* pPacketInfo = NULL;
    while(m_pHeadNode)
    {
        pPacketInfo = static_cast<TMdbPacketInfo*>(m_pHeadNode->pData);
        if(pPacketInfo == NULL) break;
        if((MDB_UINT32)uiCurLength+pPacketInfo->uiLength >= oPacketInfo.uiLength)
        {
            MDB_UINT32 uiCopyLength = oPacketInfo.uiLength-uiCurLength;            
            if(uiCurLength == 0)//表示目标packet可以复用当前包
            {
                oPacketInfo.Attach(*pPacketInfo, uiLength);
            }
            else//进行部分拷贝到目标packet
            {
                memcpy(pcBuffer+uiCurLength, pPacketInfo->GetBuffer(), uiCopyLength);
            }
            pPacketInfo->uiLength -= uiCopyLength;
            pPacketInfo->uiOffset += uiCopyLength;
            m_uiTotalLength -= uiCopyLength;
            if(pPacketInfo->uiLength == 0)
            {
                TMdbNtcBaseList::RemoveHead();
            }
            break;
        }        
        else//当前包不足以拼接成目标packet
        {
            memcpy(pcBuffer+uiCurLength, pPacketInfo->GetBuffer(), pPacketInfo->uiLength);
            uiCurLength += pPacketInfo->uiLength;
            m_uiTotalLength -= pPacketInfo->uiLength;
            //删除当前数据包
            TMdbNtcBaseList::RemoveHead();
        }
    }
    return true;
}

TMdbSendPackets::TMdbSendPackets()
{
    iSendBytes = 0;
    SetAutoRelease(true);
}

TMdbSendPackets::~TMdbSendPackets()
{
    Clear();
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbServerInfo, /*QuickMDB::*/TMdbNtcServerSocket)
TMdbServerInfo::TMdbServerInfo(const char* pszServerHost /* = NULL */, int iServerPort /* = 0 */,
                         TMdbProtocol* pProtocol /* = NULL */, const /*QuickMDB::*/TMdbRuntimeObject* pPeerRuntimeObject /* = NULL */)
{
    if(pszServerHost)
    {
        m_sServerHost = pszServerHost;
    }
    m_iServerPort = iServerPort;
    this->pProtocol = pProtocol;
    this->pPeerProactor = NULL;
    this->pProactorInfo = NULL;
    this->pPeerRuntimeObject = pPeerRuntimeObject;
}

TMdbServerInfo::~TMdbServerInfo()
{
    if(pProtocol)
    {
        delete pProtocol;
        pProtocol = NULL;
    }
    if(pProactorInfo)
    {
        delete pProactorInfo;
        pProactorInfo = NULL;
    }
}

bool TMdbServerInfo::AddEventMonitor(MDB_UINT16 events)
{
    //如果是前摄器线程，则无需唤醒前摄器以触发更新
    if(pPeerProactor->GetThreadHandle() == mdb_ntc_zthread_handle())
    {
        return pPeerProactor->AddEventMonitor(m_uiSocketID, events);
    }
    else
    {
        return pPeerProactor->AsyncAddEventMonitor(m_uiSocketID, events);
    }
}

bool TMdbServerInfo::RemoveEventMonitor(MDB_UINT16 events)
{
    //如果是前摄器线程，则无需唤醒前摄器以触发更新
    if(pPeerProactor->GetThreadHandle() == mdb_ntc_zthread_handle())
    {
        return pPeerProactor->RemoveEventMonitor(m_uiSocketID, events);
    }
    else
    {
        return pPeerProactor->AsyncRemoveEventMonitor(m_uiSocketID, events);
    }
}

TMdbPeerHelper::TMdbPeerHelper(TMdbPeerInfo* pPeerInfo)
{
    this->pPeerInfo = pPeerInfo;
}

TMdbPeerHelper::~TMdbPeerHelper()
{
}

MDB_ZF_IMPLEMENT_DYNCREATE_OBJECT(TMdbPeerInfo, TMdbNtcSocket);

TMdbPeerInfo::TMdbPeerInfo()
{
    pno             =   0;
    pProactorInfo   =   NULL;
    pPeerProactor   =   NULL;
    pServerInfo     =   NULL;
    pCurEventPump   =   NULL;
    m_uiPeerFlag    =   0;
    m_tConnectime   =   time(NULL);
    m_pTrafficCtrl  =   NULL;
    m_pPeerHelper   =   NULL;
    pProtocol       =   NULL;
    memset(&m_uiTimerID, 0x00, sizeof(m_uiTimerID));//重置为0
    memset(&m_uiMaxIdleTime, 0x00, sizeof(m_uiMaxIdleTime));//重置为0
    SetPeerState(MDB_NTC_PEER_IDLE_STATE);
    m_pRecvMsgQueue     =   NULL;
}

void TMdbPeerInfo::Init(QuickMDB_SOCKET uiSocketID , TMdbServerInfo* pServerInfo /* = NULL */)
{
    this->m_uiSocketID = uiSocketID;
    this->pServerInfo = pServerInfo;
    pProtocol       =   pServerInfo?pServerInfo->pProtocol:NULL;
}

TMdbPeerInfo::~TMdbPeerInfo()
{
    if(IsClient())//说明是connect方式，需要连同一起销毁
    {
        delete pServerInfo;
        pServerInfo = NULL;
    }

    if(pProactorInfo)
    {
        delete pProactorInfo;
        pProactorInfo = NULL;
    }
    for (int i = 0; i < MDB_NTC_MAX_IDLE_TIMEOUT_TYPE; ++i)
    {
        if(m_uiTimerID[i] > 0)
        {
            TMdbNtcCheckPeerTimeout* pCheckIdlePeer = NULL;
            TMdbNtcTimerManager::DelTimerById(m_uiTimerID[i], (void**)&pCheckIdlePeer);
            if(pCheckIdlePeer)
            {
                delete pCheckIdlePeer;
                pCheckIdlePeer = NULL;
            }
            m_uiTimerID[i] = 0;            
        }
    }
    if(m_pPeerHelper)
    {
        delete m_pPeerHelper;
        m_pPeerHelper = NULL;
    }
    if(m_pRecvMsgQueue)
    {
        ClearRecvMessage();
        delete m_pRecvMsgQueue;
        m_pRecvMsgQueue = NULL;
    }
}

bool TMdbPeerInfo::SpliceMsg()
{
    if(oRecvPackets.pSplicingMsg && (oRecvPackets.pSplicingMsg->GetLength() == oRecvPackets.uiSplicedLength
        || oRecvPackets.SplicePacket(oRecvPackets.pSplicingMsg->GetLength()-oRecvPackets.uiSplicedLength, oRecvPackets.pSplicingMsg->oPacketInfo)))
    {
        m_oTrafficInfo.AddRecvMsgNum();
        TMdbMsgInfo* pSplicingMsg = oRecvPackets.pSplicingMsg;
        oRecvPackets.pSplicingMsg = NULL;
        oRecvPackets.uiSplicedLength = 0;
        if(pProtocol == NULL || (!pProtocol->PreTranslateMessage(this, pSplicingMsg) && pSplicingMsg))
        {
            pPeerProactor->GetEventDispatcher()->Dispatch(new TMdbRecvMsgEvent(this, pSplicingMsg));
        }
        else if(pSplicingMsg)
        {
            pSplicingMsg->Release();
        }
        return true;
    }
    else
    {
        return false;
    }
}

void TMdbPeerInfo::CheckRecvPackets()
{
    /*if((oRecvPackets.pSplicingMsg == NULL || (SpliceMsg() && !IsShutdown())) && pProtocol)
    {
        
    }*/
    if( NULL == pProtocol )
    {
        return ;
    }
    if( (oRecvPackets.pSplicingMsg == NULL)
            || ( SpliceMsg() && !IsShutdown()) 
       )
    {
        pProtocol->CheckPackets(this);
    }
}
void TMdbPeerInfo::SetMaxRecvRate(MDB_UINT32 uiMaxRecvRate)
{
    m_oSpinLock.Lock();
    if(m_pTrafficCtrl == NULL)
    {
        m_pTrafficCtrl = new TMdbTrafficCtrl;
    }
    m_pTrafficCtrl->SetMaxRecvRate(uiMaxRecvRate);
    m_oSpinLock.Unlock();
}

void TMdbPeerInfo::SetMaxSendRate(MDB_UINT32 uiMaxSendRate)
{
    m_oSpinLock.Lock();
    if(m_pTrafficCtrl == NULL)
    {
        m_pTrafficCtrl = new TMdbTrafficCtrl;
    }
    m_pTrafficCtrl->SetMaxSendRate(uiMaxSendRate);
    m_oSpinLock.Unlock();
}

void TMdbPeerInfo::SetMaxFlowRate(MDB_UINT32 uiMaxFlowRate)
{
    m_oSpinLock.Lock();
    if(m_pTrafficCtrl == NULL)
    {
        m_pTrafficCtrl = new TMdbTrafficCtrl;
    }
    m_pTrafficCtrl->SetMaxFlowRate(uiMaxFlowRate);
    m_oSpinLock.Unlock();
}

void TMdbPeerInfo::SetPeerTimeout(MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutType, MDB_UINT32 uiSeconds)
{
    if(m_uiMaxIdleTime[eTimeoutType] == uiSeconds && m_uiTimerID[eTimeoutType] > 0) return;
    //先杀掉之前的
    if(m_uiTimerID[eTimeoutType] != 0)
    {
        TMdbNtcCheckPeerTimeout* pCheckIdlePeer = NULL;
        TMdbNtcTimerManager::DelTimerById(m_uiTimerID[eTimeoutType], (void**)&pCheckIdlePeer);
        if(pCheckIdlePeer)
        {
            delete pCheckIdlePeer;
            pCheckIdlePeer = NULL;
        }
        m_uiTimerID[eTimeoutType] = 0;
    }
    m_uiMaxIdleTime[eTimeoutType] = uiSeconds;
    if(uiSeconds > 0 && pPeerProactor)
    {
        TMdbNtcCheckPeerTimeout* pCheckIdlePeer = new TMdbNtcCheckPeerTimeout;
        pCheckIdlePeer->pEventDispatcher = pPeerProactor->GetEventDispatcher();
        pCheckIdlePeer->pPeerInfo = this;
        pCheckIdlePeer->uiSeconds = uiSeconds;
        pCheckIdlePeer->eTimeoutType = eTimeoutType;
        m_uiTimerID[eTimeoutType] = MdbNtcAddTimer("", false, uiSeconds*10, CheckPeerTimeout, pCheckIdlePeer);
    }    
}

void TMdbPeerInfo::CheckPeerTimeout(void* pArg)
{
    TMdbNtcCheckPeerTimeout* pCheckPeerTimeout = static_cast<TMdbNtcCheckPeerTimeout*>(pArg);
    TMdbPeerInfo* pPeerInfo = pCheckPeerTimeout->pPeerInfo;
    time_t tRecentTime = pPeerInfo->GetConnectTime();
    if((pCheckPeerTimeout->eTimeoutType == MDB_NTC_DATA_IDLE_TIMEOUT || pCheckPeerTimeout->eTimeoutType == MDB_NTC_RECV_IDLE_TIMEOUT)
        && pPeerInfo->GetLastRecvTime() != -1 && pPeerInfo->GetLastRecvTime() > tRecentTime)
    {
        tRecentTime = pPeerInfo->GetLastRecvTime();
    }
    if((pCheckPeerTimeout->eTimeoutType == MDB_NTC_DATA_IDLE_TIMEOUT || pCheckPeerTimeout->eTimeoutType == MDB_NTC_SEND_IDLE_TIMEOUT)
        && pPeerInfo->GetLastSendTime() != -1 && pPeerInfo->GetLastSendTime() > tRecentTime)
    {
        tRecentTime = pPeerInfo->GetLastSendTime();
    }
    int iElapseSeconds = (int)(time(NULL)-tRecentTime), iNewTimerSeconds = (int)pCheckPeerTimeout->uiSeconds;
    if(iElapseSeconds >= (int)pCheckPeerTimeout->uiSeconds)
    {
        pPeerInfo->SetTimeoutId(pCheckPeerTimeout->eTimeoutType, (MDB_UINT32)-1);
        if(pPeerInfo->IsShutdown())
        {
            delete pCheckPeerTimeout;
            pCheckPeerTimeout = NULL;
            return;
        }
        else
        {
            //触发一个空闲超时的事件
            TMdbTimeoutEvent* pTimeoutEvent = new TMdbTimeoutEvent(pPeerInfo, pCheckPeerTimeout->eTimeoutType);
            pCheckPeerTimeout->pEventDispatcher->Dispatch(pTimeoutEvent);
        }
    }
    else
    {
        iNewTimerSeconds = (int)(pCheckPeerTimeout->uiSeconds-(MDB_UINT32)iElapseSeconds);
    }
    //重新压入一个定时器
    pPeerInfo->SetTimeoutId(pCheckPeerTimeout->eTimeoutType, (MDB_UINT32)MdbNtcAddTimer("", false, (MDB_UINT32)iNewTimerSeconds*10, CheckPeerTimeout, pCheckPeerTimeout));
}

bool TMdbPeerInfo::AddEventMonitor(MDB_UINT16 events, bool bLock /* = true */)
{
    if(pPeerProactor == NULL) return false;
    bool bRet = true;
    if(bLock)
    {
        m_oSpinLock.Lock();
    }    
    if(!IsShutdown() &&  (events == 0 || GetPeerFlag(events) != events))//只要任一个指定的事件不存在，则需要添加
    {
        TADD_DETAIL("add event:peer[%d],flag[%0x],event[%0x]", m_uiSocketID, m_uiPeerFlag, events);
        SetPeerFlag(events);
        //如果是前摄器线程，则无需唤醒前摄器以触发更新
        if(pPeerProactor->GetThreadHandle() == mdb_ntc_zthread_handle())
        {
            bRet = pPeerProactor->AddEventMonitor(m_uiSocketID, events);
        }
        else
        {
            bRet = pPeerProactor->AsyncAddEventMonitor(m_uiSocketID, events);
        }
    }
    else
    {
        TADD_DETAIL("no need to add event:peer[%d],flag[%0x],event[%0x]", m_uiSocketID, m_uiPeerFlag, events);
    }
    if(bLock)
    {
        m_oSpinLock.Unlock();
    }
    return bRet;
}

bool TMdbPeerInfo::RemoveEventMonitor(MDB_UINT16 events, bool bLock /* = true */)
{
    if(pPeerProactor == NULL) return false;
    bool bRet = true;
    if(bLock)
    {
        m_oSpinLock.Lock();
    }
    if(events == 0 || GetPeerFlag(events))//只要任一个指定的事件存在，则需要移除
    {
        TADD_DETAIL("remove event:peer[%d],flag[%0x],event[%0x]", m_uiSocketID, m_uiPeerFlag, events);
        if(events == 0)
        {
            ClearPeerFlag((MDB_UINT16)((MDB_NTC_PEER_EV_READ_FLAG|MDB_NTC_PEER_EV_WRITE_FLAG)&m_uiPeerFlag));
            bRet = pPeerProactor->AsyncRemoveEventMonitor(m_uiSocketID, events);
        }
        else
        {
            ClearPeerFlag(events);
            //如果是前摄器线程，则无需唤醒前摄器以触发更新
            if(pPeerProactor->GetThreadHandle() == mdb_ntc_zthread_handle())
            {
                bRet = pPeerProactor->RemoveEventMonitor(m_uiSocketID, events);           
            }
            else
            {
                bRet = pPeerProactor->AsyncRemoveEventMonitor(m_uiSocketID, events);
            }
        }
    }
    else
    {
        TADD_DETAIL("no need to remove event:peer[%d],flag[%0x],event[%0x]", m_uiSocketID, m_uiPeerFlag, events);
    }
    if(bLock)
    {
        m_oSpinLock.Unlock();
    }
    return bRet;
}

bool TMdbPeerInfo::Shutdown()
{
    bool bRet = true;
    m_oSpinLock.Lock();
    if((m_uiPeerFlag&MDB_NTC_PEER_SHUTDOWN_FLAG) == 0)
    {
        m_uiPeerFlag |= MDB_NTC_PEER_SHUTDOWN_FLAG;
    }
    else
    {
        bRet = false; 
    }
    m_oSpinLock.Unlock();
    return bRet;
}

bool TMdbPeerInfo::Disconnect(/*QuickMDB::*/TMdbNtcStringBuffer sReason /* = "" */, bool bPasvClose /* = false */)
{
    bool bRet = Shutdown();
    if(bRet)
    {
        TADD_DETAIL("fd[%u], disconnect %s", m_uiSocketID, sReason.c_str());
        for (int i = 0; i < MDB_NTC_MAX_IDLE_TIMEOUT_TYPE; ++i)
        {
            if(m_uiTimerID[i] > 0)
            {
                TMdbNtcCheckPeerTimeout* pCheckIdlePeer = NULL;
                TMdbNtcTimerManager::DelTimerById(m_uiTimerID[i], (void**)&pCheckIdlePeer);
                if(pCheckIdlePeer)
                {
                    delete pCheckIdlePeer;
                    pCheckIdlePeer = NULL;
                }
                m_uiTimerID[i] = 0;
            }
        }
        sDisconnectReason = sReason;
        if(bPasvClose)
        {            
            this->SetPeerFlag(MDB_NTC_PEER_PASV_CLOSE_FLAG);
        }
        if(pPeerProactor)
        {
            RemoveEventMonitor(0);
        }
    }
    return bRet;
}

void TMdbPeerInfo::DeleteTrafficCtrl()
{
    m_oSpinLock.Lock();
    if(m_pTrafficCtrl == NULL)
    {
        delete m_pTrafficCtrl;
        m_pTrafficCtrl = NULL;
    }
    m_oSpinLock.Unlock();
}

void TMdbPeerInfo::SetMsgHandleMode(bool bDetached)
{
    TMdbNtcAutoLock oAutoLock(&m_oSpinLock);
    if(bDetached)
    {
        m_uiPeerFlag = (MDB_UINT16)(m_uiPeerFlag|MDB_NTC_PEER_MSG_DETACHED_FLAG);
        if(m_pRecvMsgQueue == NULL) m_pRecvMsgQueue = new /*QuickMDB::*/TMdbNtcQueue;
    }
    else
    {
        m_uiPeerFlag = (MDB_UINT16)(m_uiPeerFlag&(~MDB_NTC_PEER_MSG_DETACHED_FLAG));
        if(m_pRecvMsgQueue == NULL) return;
        do
        {
            TMdbMsgInfo* pMsgInfo = static_cast<TMdbMsgInfo*>(m_pRecvMsgQueue->Pop(0));
            if(pMsgInfo == NULL) break;
            this->GetEventDispatcher()->Dispatch(new TMdbRecvMsgEvent(this, pMsgInfo));
        } while (1);
        m_pRecvMsgQueue->Wakeup();
    }
}

int TMdbPeerInfo::SendMessage(const void* pszMsg, MDB_UINT32 uiMsgLength /* = -1 */, int iMilliSeconds /* = -1 */)
{
    int iSendBytes = 0;
    /*
    TMdbNtcSocketSendEvent* pSocketSendEvent = new TMdbNtcSocketSendEvent(this, pszMsg, iMsgLength);
    do
    {
        if(IsShutdown())
        {
            iRet = -1;
            break;
        }
        TMdbNtcSocketInfo* pSocketInfo = static_cast<TMdbNtcSocketInfo*>(this);
        //根据send事件判断适合的事件泵
        TMdbEventPump* pAdaptiveEventPump = PreDispatch(pSocketSendEvent);
        if(pAdaptiveEventPump == NULL)
        {
            iRet = -1;
            break;
        }
        pSocketSendEvent->AddRef();//当前保留计数，使得处理过程中不会delete
        if(pAdaptiveEventPump->GetThreadHandle() == GetCurrentThreadHandle())
        {
            pSocketInfo->queueSendEvent.Push(pSocketSendEvent);//加入到socketinfo的队列中
            //如果当前线程就是事件泵线程，则直接调用发送
            iRet = DealSocketSendTask(pSocketInfo, iMilliSeconds, pSocketSendEvent);
        }
        else
        {
            //其他线程处理此事件，则当前线程等待
            if(g_pSendThreadEvent == NULL)
            {
                g_pSendThreadEvent = new TMdbNtcThreadEvent;
            }
            pSocketSendEvent->pSendThreadEvent = g_pSendThreadEvent;
            //将事件放到定时器线程中，iMilliSeconds,超时触发g_pThreadEvent
            pSocketInfo->queueSendEvent.Push(pSocketSendEvent);//加入到socketinfo的队列中
            //发起ev_write事件的监听
            if(pSocketInfo->queueSendEvent.GetSize() == 1)
            {
                AddEventMonitor(pSocketSendEvent->pPeerInfo, MDB_NTC_PEER_EV_WRITE_FLAG);
            }
            //等待事件触发
            g_pSendThreadEvent->Wait(iMilliSeconds);
        }
        iSendBytes = pSocketSendEvent->iSendBytes;
    } while (0);    
    pSocketSendEvent->Release(iRet);
    */
    return iSendBytes;    
}

bool TMdbPeerInfo::PostMessage(const void* pMsg, MDB_UINT32 uiMsgLength /* = -1 */)
{
    bool bRet = true;
    if(uiMsgLength == (MDB_UINT32)-1) uiMsgLength = (MDB_UINT32)strlen((const char*)pMsg);
    if(uiMsgLength == 0) return bRet;
    TMdbPacketInfo* pPacketInfo = new TMdbPacketInfo(uiMsgLength);
    memcpy(pPacketInfo->GetBuffer(), pMsg, uiMsgLength);
    bRet = PostMessage(pPacketInfo);
    if(bRet == false)
    {
        delete pPacketInfo;
        pPacketInfo = NULL;
    }
    return bRet;
}

bool TMdbPeerInfo::PostMessage(TMdbPacketInfo* pPacketInfo)
{
    if(IsShutdown())
    {
        mdb_ntc_errstr = "peer is already shutdown";
        return false;
    }
    //pSocketEvent->AddRef();//将事件放到定时器线程中
    oSendPackets.Push(pPacketInfo);//加入到socketinfo的队列中
    //发起ev_write事件的监听
    TADD_DETAIL("PostMessage:fd[%d],queueSendEvent[%u]", m_uiSocketID, oSendPackets.GetSize());
    if(oSendPackets.GetSize() == 1)
    {
        this->AddEventMonitor(MDB_NTC_PEER_EV_WRITE_FLAG);
    }
    return true;
}

void TMdbPeerInfo::ClearRecvMessage()
{
    if(m_pRecvMsgQueue)
    {
        do
        {
            TMdbMsgInfo* pMsgInfo = static_cast<TMdbMsgInfo*>(m_pRecvMsgQueue->Pop(0));
            if(pMsgInfo == NULL) break;
            else pMsgInfo->Release();
        } while (1);
    }
}

MDB_UINT32 MdbNtcAddTimer(const char *ATimerName,bool ALoopFlag,MDB_UINT32 ACount100ms,OnMdbEventFunc AEventFunc,void *pFuncParam)
{
    TMdbNtcTimerManager::Initialize(false);
    char szBuffer[11] = {'\0'};
    for (int i = 0; i < 10; ++i)
    {
        szBuffer[i] = (char)('a'+rand()%26);
    }
    return TMdbNtcTimerManager::AddRelativeTimer(szBuffer, ALoopFlag, ACount100ms, AEventFunc, pFuncParam);
}
//_NTC_END
//}
