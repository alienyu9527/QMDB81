#include "Common/mdbPeerEvent.h"
#include "Common/mdbPeerInfo.h"
//namespace QuickMDB
//{
//using namespace QuickMDB;
MDB_ZF_IMPLEMENT_OBJECT(TMdbEventInfo, /*QuickMDB::*/TMdbNtcBaseObject)
void TMdbEventInfo::Release(int iRetCode)
{
    --uiRefCnt;
    if(uiRefCnt == 0)
    {
        delete this;
    }
}

TMdbEventDispatcher::TMdbEventDispatcher()
{
    m_arrayPump.SetAutoRelease(true);
}

TMdbEventDispatcher::~TMdbEventDispatcher()
{
    m_arrayPump.Clear();
}

TMdbEventPump* TMdbEventDispatcher::CreateEventPump()
{
    return new TMdbEventPump;
}

void TMdbEventDispatcher::AddEventPump(TMdbEventPump* pEventPump)
{
    m_arrayPump.Add(pEventPump);
}

void TMdbEventDispatcher::InitEventPump(int iPumpNum)
{
    for (int i = 0; i < iPumpNum; ++i)
    {
        m_arrayPump.Add(CreateEventPump());
    }
}

TMdbEventPump* TMdbEventDispatcher::GetLeastEventPump()
{
    //根据压力最小的事件泵
    TMdbEventPump* pEventPump = NULL, *pLessEventPump = NULL;
    MDB_UINT32 uiLessSize = 0;
    for (unsigned int i = 0; i < m_arrayPump.GetSize(); ++i)
    {
        pEventPump = static_cast<TMdbEventPump*>(m_arrayPump.GetAt(i));
        if(pLessEventPump == NULL || pEventPump->GetQueueSize() < uiLessSize)
        {
            uiLessSize = pEventPump->GetQueueSize();
            pLessEventPump = pEventPump;
        }
    }
    return pLessEventPump;
}

TMdbEventPump* TMdbEventDispatcher::Dispatch(TMdbEventInfo* pEventInfo)
{
    //根据压力最小的事件泵
    TMdbEventPump* pSuitableEventPump = PreDispatch(pEventInfo);
    if(pSuitableEventPump)
    {
        pSuitableEventPump->PushEvent(pEventInfo);
    }
    else//没有找到合适的事件泵的情况下，直接调用处理，方便无事件泵的情况下事件处理
    {
        if(pEventInfo->pEventHandler)
        {
            pEventInfo->pEventHandler->ProcessEvent(pEventInfo, NULL);
        }
        else
        {
            pEventInfo->Release(-1);//没有找到合适的事件泵
        }
    }
    return pSuitableEventPump;
}

/*QuickMDB::*/TMdbNtcStringBuffer TMdbEventDispatcher::PrintAllPump()
{
    TMdbNtcStringBuffer sRet;
    for (unsigned int i = 0; i < m_arrayPump.GetSize(); ++i)
    {
        sRet<<"event pump["<<i<<"]:"<<static_cast<TMdbEventPump*>(m_arrayPump[i])->ToString()<<'\n';
    }
    if(!sRet.IsEmpty())
    {
        sRet.Delete((int)(sRet.GetLength()-1));
    }
    return sRet;
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbEventPump, TMdbNtcThread);
TMdbEventPump::TMdbEventPump()
{
    m_queueEvent.SetAutoRelease(true);
}

TMdbEventPump::~TMdbEventPump()
{
    m_queueEvent.Clear();
}

/*QuickMDB::*/TMdbNtcStringBuffer TMdbEventPump::ToString() const
{
    return /*QuickMDB::*/TMdbNtcStringBuffer(512, "thread_id[%"MDB_NTC_ZS_FORMAT_UINT64"],state[%s] queue_size[%d],processed[%u:%u]", GetThreadId(),
        mdb_ntc_zthread_state_name(GetThreadState()), m_queueEvent.GetSize(), m_queueEvent.GetPopTimes(),m_queueEvent.GetPushTimes());
}

void TMdbEventPump::PushEvent(TMdbEventInfo* pEventInfo)
{
    m_queueEvent.Push(pEventInfo);
}

int TMdbEventPump::Execute()
{
    TMdbEventInfo* pEventInfo = NULL;
    do
    {
        pEventInfo = static_cast<TMdbEventInfo*>(m_queueEvent.Pop());
        if(pEventInfo)
        {
            if(pEventInfo->pEventHandler)
            {                
                pEventInfo->pEventHandler->ProcessEvent(pEventInfo, this);
            }
        }
        else
        {
            if(TestCancel())
            {
                m_queueEvent.Clear();
                Cancel();
            }
        }
    } while (1);
    return 0;
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbPeerEvent, TMdbEventInfo);
MDB_ZF_IMPLEMENT_OBJECT(TMdbConnectEvent, TMdbPeerEvent);
MDB_ZF_IMPLEMENT_OBJECT(TMdbDisconnectEvent, TMdbPeerEvent);
MDB_ZF_IMPLEMENT_OBJECT(TMdbRecvMsgEvent, TMdbPeerEvent);
MDB_ZF_IMPLEMENT_OBJECT(TMdbTimeoutEvent, TMdbPeerEvent);
MDB_ZF_IMPLEMENT_OBJECT(TMdbErrorEvent, TMdbPeerEvent);

TMdbRecvMsgEvent::~TMdbRecvMsgEvent()
{
    if(pMsgInfo)
    {
        pMsgInfo->Release();
        pMsgInfo = NULL;
    }
}
//_NTC_END
//}
