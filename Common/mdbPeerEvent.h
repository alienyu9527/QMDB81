/**
 * @file mdbPeerEvent.h
 * 与连接相关的事件信息
 * 
 * @author jiang.jinzhou
 * @version 1.0
 * @date 2013/09/05
 * @bug 新建，无bug
 * @bug 单元测试，未发现bug
 * @warning 
 */
#ifndef _MDB_PEER_EVENT_HXX_
#define _MDB_PEER_EVENT_HXX_
#include "Common/mdbNTC.h"
#include "Common/mdbPeerInfo.h"
//namespace QuickMDB
//{

class TMdbEventInfo;

/**
 * @brief 事件泵,对应一个线程
 * 
 */
class TMdbEventPump:public TMdbNtcThread
{
    MDB_ZF_DECLARE_OBJECT(TMdbEventPump);
public:
    TMdbEventPump();
    virtual ~TMdbEventPump();
    /**
     * @brief 压入一个待处理事件
     * 
     * @param pEventInfo [in] 事件
     * @return 无     
     */
    virtual void PushEvent(TMdbEventInfo* pEventInfo);
    /**
     * @brief 打印事件泵的状态
     * 
     */
    virtual TMdbNtcStringBuffer ToString( ) const;
    /**
     * @brief 获得当前队列的大小
     * 
     * @return MDB_UINT32
     * @retval 当前队列的大小
     */
    inline MDB_UINT32 GetQueueSize()
    {
        return m_queueEvent.GetSize();
    }
protected:
    /**
     * @brief 前摄器的执行函数，此函数不会退出
     * 有事件了，调用事件分发器，将事件分派给事件处理器
     */
    virtual int Execute();
protected:
    TMdbNtcQueue  m_queueEvent;///< 阻塞式的事件队列
};

/**
 * @brief 事件分发器
 * 
 */
class TMdbEventDispatcher
{
public:
    TMdbEventDispatcher();
    virtual ~TMdbEventDispatcher();
    /**
     * @brief 创建一个事件泵
     * 
     * @return TMdbEventPump*
     * @retval 创建的事件泵
     */
    virtual TMdbEventPump* CreateEventPump();
    /**
     * @brief 添加事件泵
     * 
     * @param pEventPump [in] 事件泵
     */
    void AddEventPump(TMdbEventPump* pEventPump);
    /**
     * @brief 获得事件泵的个数
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetPumpCount()
    {
        return m_arrayPump.GetSize();
    }
    /**
     * @brief 初始化事件泵的数目
     * 
     * @param iPumpNum [in] 事件泵的数目
     */
    void InitEventPump(int iPumpNum);    
    /**
     * @brief 分发事件
     * 
     * @param pEventInfo [in] 事件
     * @return TMdbEventPump*
     * @retval 使用的事件泵
     */
    virtual TMdbEventPump* Dispatch(TMdbEventInfo* pEventInfo);
    /**
     * @brief 获得负载最小的事件泵
     * 
     * @return TMdbEventPump*
     * @retval 负载最小的事件泵
     */
    TMdbEventPump* GetLeastEventPump();
    /**
     * @brief 打印事件泵的状态
     * 
     * @return TMdbNtcStringBuffer
     */
    TMdbNtcStringBuffer PrintAllPump();
protected:
    /**
     * @brief 预分发，仅仅是为了获得适合的事件泵，并不分发
     * 
     * @param pEventInfo [in] 事件     
     * @return TMdbEventPump*
     * @retval 适合的事件泵
     */
    virtual TMdbEventPump* PreDispatch(TMdbEventInfo* pEventInfo)
    {
        return GetLeastEventPump();
    }
protected:
    TMdbNtcAutoArray m_arrayPump;///< 消息泵数组    
};

/**
 * @brief 事件处理器
 * 
 * 
 */
class TMdbEventHandler
{
public:
    virtual ~TMdbEventHandler()
    {
    }
    /**
     * @brief 处理事件
     * 
     * @param pEventInfo [in] 事件
     * @param pEventPump [in] 事件泵
     * @return bool
     * @retval true 成功
     */
    virtual bool ProcessEvent(TMdbEventInfo* pEventInfo, TMdbEventPump* pEventPump) = 0;
};

class TMdbProactor;

/**
 * @brief 事件信息
 * 
 */
class TMdbEventInfo:public TMdbNtcBaseObject
{
public:
    MDB_ZF_DECLARE_OBJECT(TMdbEventInfo);
    MDB_UINT32 uiRefCnt;///< 引用计数
    TMdbProactor* pProactor;///< 触发此事件的前摄器    
    TMdbEventHandler* pEventHandler;///< 此事件的处理器，方便事件泵路由此事件到对应的处理器
    TMdbEventInfo():uiRefCnt(1),pProactor(NULL),pEventHandler(NULL)
    {
    }
    inline void AddRef()
    {
        ++uiRefCnt;
    }
    /**
     * @brief 释放event
     * 
     * @param iRetCode [in] 返回码
     * @return 无
     */
    void Release(int iRetCode = 0);
    MDB_UINT32 RefCnt()
    {
        return uiRefCnt;
    }
};

class TMdbPeerInfo;
/**
 * @brief 连接上的事件信息
 * 
 */
class TMdbPeerEvent:public TMdbEventInfo
{
    MDB_ZF_DECLARE_OBJECT(TMdbPeerEvent);
public:
    TMdbPeerInfo* pPeerInfo;///< 连接信息
    /**
     * @brief 构造函数
     * 
     * @param pno [in] 连接号
     * @param iEventType [in] 事件类型
     */
    TMdbPeerEvent(TMdbPeerInfo* pPeerInfoParam):pPeerInfo(pPeerInfoParam)
    {
    }
};

/**
 * @brief 连接成功事件
 * 
 */
class TMdbConnectEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbConnectEvent);
public:
    /**
     * @brief 构造函数
     * 
     * @param pSocketInfo [in] 连接成功的socket信息
     */
    TMdbConnectEvent(TMdbPeerInfo* pPeerInfo):TMdbPeerEvent(pPeerInfo)
    {
    }
    /**
     * @brief 链接建立的时间
     * 
     * @return time_t
     * @retval 链接建立的时间
     */
    inline time_t GetConnectTime()
    {
        return pPeerInfo->GetConnectTime();
    }
};

/**
 * @brief 断开事件
 * 
 */
class TMdbDisconnectEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbDisconnectEvent);
public:
    /**
     * @brief 构造函数
     * 
     * @param bPasvClose [in] 是否为被动关闭
     * @param pPeerInfo [in] 连接信息
     */
    TMdbDisconnectEvent(TMdbPeerInfo* pPeerInfo, time_t tDisconnectTime = time(NULL))
        :TMdbPeerEvent(pPeerInfo),m_tDisconnectTime(tDisconnectTime)
    {
    }
    /**
     * @brief 是否被动关闭
     * 
     * @return bool
     * @retval true  被动关闭
     * @retval false 主动关闭
     */
    inline bool IsPasvClose()
    {
        return pPeerInfo->IsPasvClose();
    }
    /**
     * @brief 链接断开的原因
     * 
     * @return TMdbNtcStringBuffer
     * @retval 断开的原因
     */
    inline TMdbNtcStringBuffer GetDisconnectReason()
    {
        return pPeerInfo->sDisconnectReason;
    }
    /**
     * @brief 链接断开的时间
     * 
     * @return time_t
     */
    inline time_t GetDisconnectTime()
    {
        return m_tDisconnectTime;
    }
protected:
    time_t  m_tDisconnectTime;///< 链路断开的时间
};

/**
 * @brief 空闲超时事件
 * 
 */
class TMdbTimeoutEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbTimeoutEvent);
public:
    MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutType;
    /**
     * @brief 构造函数
     * 
     * @param pSocketInfo [in] 连接成功的socket信息
     * @param eIdleTimeoutType [in] 空闲类型
     */
    TMdbTimeoutEvent(TMdbPeerInfo* pPeerInfo, MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutTypeParam)
        :TMdbPeerEvent(pPeerInfo),eTimeoutType(eTimeoutTypeParam)
    {
    }
};

/**
 * @brief 数据事件
 * 
 */
class TMdbDataEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbDataEvent);
public:
    TMdbPacketInfo* pPacketInfo;///< 数据包信息
    /**
     * @brief 构造函数
     * 
     * @param pSocketInfo [in] socket信息
     * @param pMsgInfo [in] 消息包信息
     */
    TMdbDataEvent(TMdbPeerInfo* pPeerInfo, TMdbPacketInfo* pPacketInfoParam)
        :TMdbPeerEvent(pPeerInfo),pPacketInfo(pPacketInfoParam)
    {
    }
};

/**
 * @brief 消息事件
 * 
 */
class TMdbRecvMsgEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbRecvMsgEvent);
public:
    TMdbMsgInfo* pMsgInfo;///< 消息包信息
    /**
     * @brief 构造函数
     * 
     * @param pSocketInfo [in] socket信息
     * @param pMsgInfo [in] 消息包信息
     */
    TMdbRecvMsgEvent(TMdbPeerInfo* pPeerInfo, TMdbMsgInfo* pMsgInfoParam)
        :TMdbPeerEvent(pPeerInfo),pMsgInfo(pMsgInfoParam)
    {
    }
    /**
     * @brief 析构函数，析构时，会自动调用pMsgInfo->Release()
     * 
     */
    virtual ~TMdbRecvMsgEvent();
};

/**
 * @brief 错误事件
 * 
 */
class TMdbErrorEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbErrorEvent);
public:
    TMdbNtcStringBuffer   sErrorInfo;
    TMdbErrorEvent(TMdbPeerInfo* pPeerInfo = NULL):TMdbPeerEvent(pPeerInfo)
    {
    }
    
};
//_NTC_END
//}
#endif
