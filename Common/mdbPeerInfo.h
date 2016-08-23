/**
 * @file mdbPeerInfo.h
 * 连接信息
 * 
 * @author jiang.jinzhou
 * @version 1.0
 * @date 2013/09/05
 * @bug 新建，无bug
 * @bug 单元测试，未发现bug
 * @warning 
 */
#ifndef _MDB_PEER_INFO_HXX_
#define _MDB_PEER_INFO_HXX_
#include "Common/mdbSysSockets.h"
#include "Common/mdbPeerProactor.h"
#include "Common/mdbCommons.h"
//namespace QuickMDB
//{
class TMdbTrafficInfo;
/**
 * @brief 流量控制器
 * 
 */
class TMdbTrafficCtrl
{
public:
    TMdbTrafficCtrl();
    /**
     * @brief 设置最大接收速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiMaxRecvRate [in] 总的最大速率
     * @return 无
     */
    inline void SetMaxRecvRate(MDB_UINT32 uiMaxRecvRate)
    {
        this->m_uiMaxRecvRate = uiMaxRecvRate;
    }
    /**
     * @brief 设置最大发送速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiMaxSendRate [in] 总的最大速率
     * @return 无
     */
    inline void SetMaxSendRate(MDB_UINT32 uiMaxSendRate)
    {
        this->m_uiMaxSendRate = uiMaxSendRate;
    }
    /**
     * @brief 设置最大接收和发送总速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiMaxFlowRate [in] 总的最大速率
     * @return 无
     */
    inline void SetMaxFlowRate(MDB_UINT32 uiMaxFlowRate)
    {
        this->m_uiMaxFlowRate = uiMaxFlowRate;
    }
    /**
     * @brief 获得最大接收速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetMaxRecvRate()
    {
        return m_uiMaxRecvRate;
    }
    /**
     * @brief 获得最大发送速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetMaxSendRate()
    {
        return m_uiMaxSendRate;
    }
    /**
     * @brief 获得最大接收和发送总速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetMaxFlowRate()
    {
        return m_uiMaxFlowRate;
    }
    /**
     * @brief 获得连接当前可以接收的字节数
     * 
     * @param pPeerInfo [in] 连接信息
     * @param pTrafficInfo [in] 流量信息
     * @param uiRecvBytes [in] 需要接收的字节数
     * @return MDB_UINT32
     * @retval 可以接收的字节数
     */
    MDB_UINT32 GetAllowRecvBytes(TMdbTrafficInfo* pTrafficInfo, MDB_UINT32 uiRecvBytes);
    /**
     * @brief 获得连接当前可以发送的字节数
     * 
     * @param pPeerInfo [in] 连接信息
     * @param pTrafficInfo [in] 流量信息
     * @param uiSendBytes [in] 需要发送的字节数
     * @return MDB_UINT32
     * @retval 可以接收的字节数
     */
    MDB_UINT32 GetAllowSendBytes(TMdbTrafficInfo* pTrafficInfo, MDB_UINT32 uiSendBytes);
    /**
     * @brief 挂起连接的流量
     * 
     * @param pPeerInfo [in] 连接信息，必须使用SharedPtr，即通过FindPeerInfo获得的结果，这样安全
     * @param events [in] 上下行，PEER_EV_READ_FLAG和PEER_EV_WRITE_FLAG的组合
     * @return 无
     */
    static void SuspendPeerTraffic(TMdbSharedPtr<TMdbPeerInfo>& pPeerInfo, MDB_UINT16 events);
protected:
    /**
     * @brief 更新流量限制的区间时间
     * 
     */
    static void ResumePeerTraffic();
protected:
    MDB_UINT32  m_uiMaxRecvRate;      ///< 最大的接收速率，单位为bytes/s，=0表示不设置
    MDB_UINT32  m_uiMaxSendRate;      ///< 最大的发送速率，单位为bytes/s，=0表示不设置
    MDB_UINT32  m_uiMaxFlowRate;      ///< 最大的流量速率，包括接收和发送，单位为bytes/s，=0表示不设置
    static  TMdbNtcQueue g_oSuspendMonitorPeerQueue;
};

/**
 * @brief 流量信息
 * 
 */
class TMdbTrafficInfo
{
public:
    TMdbTrafficInfo()
        :m_tLastRecvTime(-1), m_tLastSendTime(-1),
        m_tLastRecvMsgTime(-1), m_tLastSendMsgTime(-1),
        m_uiLastSecRecvBytes(0), m_uiLastSecSendBytes(0),
        m_uiSecRecvBytes(0), m_uiSecSendBytes(0),
        m_uiTotalRecvBytes(0), m_uiTotalSendBytes(0),
        m_uiLastSecRecvMsgNum(0), m_uiLastSecSendMsgNum(0),
        m_uiSecRecvMsgNum(0), m_uiSecSendMsgNum(0),
        m_uiTotalRecvMsgNum(0), m_uiTotalSendMsgNum(0)
    {
    }
    /**
     * @brief 获得上一次接收数据时间
     * 
     * @return time_t
     */
    inline time_t GetLastRecvTime() const
    {
        return m_tLastRecvTime;
    }
    /**
     * @brief 获得上一次发送数据时间
     * 
     * @return time_t
     */
    inline time_t GetLastSendTime() const
    {
        return m_tLastSendTime;
    }
    /**
     * @brief 获得上一个消息包时间
     * 
     * @return time_t
     */
    inline time_t GetLastRecvMsgTime() const
    {
        return m_tLastRecvMsgTime;
    }
    /**
     * @brief 获得发送上一个消息包时间
     * 
     * @return time_t
     */
    inline time_t GetLastSendMsgTime() const
    {
        return m_tLastSendMsgTime;
    }
    /**
     * @brief 获得上一次接收数据的的字节数(周期内统计)
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetLastSecRecvBytes() const
    {
        switch (time(NULL)-m_tLastRecvTime)
        {
        case 0:
            return m_uiLastSecRecvBytes;
        case 1:
            return m_uiSecRecvBytes;
        default:
            return 0;
        }
    }
    /**
     * @brief 获得上一次发送数据的的字节数(周期内统计)
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetLastSecSendBytes() const
    {
        switch (time(NULL)-m_tLastSendTime)
        {
        case 0:
            return m_uiLastSecSendBytes;
        case 1:
            return m_uiSecSendBytes;
        default:
            return 0;
        }
    }
    /**
     * @brief 获得总的接收的字节数
     * 
     * @return MDB_UINT64
     */
    inline MDB_UINT64 GetTotalRecvBytes() const
    {
        return m_uiTotalRecvBytes;
    }
    /**
     * @brief 获得总的发送的字节数
     * 
     * @return MDB_UINT64
     */
    inline MDB_UINT64 GetTotalSendBytes() const
    {
        return m_uiTotalSendBytes;
    }
    /**
     * @brief 获得当前周期的接收的字节数，也就是实时速率
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecRecvBytes() const
    {
        return time(NULL) == m_tLastRecvTime?m_uiSecRecvBytes:0;
    }
    /**
     * @brief 获得当前周期的发送的字节数，也就是实时速率
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecSendBytes() const
    {
        return time(NULL) == m_tLastSendTime?m_uiSecSendBytes:0;
    }
    /**
     * @brief 获得当前周期的总的字节数，也就是实时速率
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecTotalBytes() const
    {
        time_t ttCurSecTime = time(NULL);//保证接收和发送比较的是同一个时间戳
        return (ttCurSecTime==m_tLastRecvTime?m_uiSecRecvBytes:0) + (ttCurSecTime==m_tLastSendTime?m_uiSecSendBytes:0);
    }
    /**
     * @brief 累加接收字节数
     * 
     * @param uiRecvBytes [in] 新接收的字节数
     * @return 无
     */
    inline void AddRecvBytes(MDB_UINT32 uiRecvBytes)
    {
        time_t ttCurTime = time(NULL);
        if(m_tLastRecvTime == ttCurTime)
        {
            m_uiSecRecvBytes += uiRecvBytes;
        }
        else
        {
            m_tLastRecvTime = ttCurTime;
            m_uiLastSecRecvBytes = m_uiSecRecvBytes;
            m_uiSecRecvBytes = uiRecvBytes;
        }
        m_uiTotalRecvBytes += uiRecvBytes; 
    }
    /**
     * @brief 累加接发送字节数
     * 
     * @param uiRecvBytes [in] 新发送的字节数
     * @return 无
     */
    inline void AddSendBytes(MDB_UINT32 uiSendBytes)
    {
        time_t ttCurTime = time(NULL);
        if(m_tLastSendTime == ttCurTime)
        {
            m_uiSecSendBytes += uiSendBytes;
        }
        else
        {
            m_tLastSendTime = ttCurTime;
            m_uiLastSecSendBytes = m_uiSecSendBytes;
            m_uiSecSendBytes = uiSendBytes;
        }
        m_uiTotalSendBytes += uiSendBytes;
    }
    /**
     * @brief 获得上一次接收数据的的消息数(周期内统计)
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetLastSecRecvMsgNum() const
    {
        switch (time(NULL)-m_tLastRecvMsgTime)
        {
        case 0:
            return m_uiLastSecRecvMsgNum;
        case 1:
            return m_uiSecRecvMsgNum;
        default:
            return 0;
        }
    }
    /**
     * @brief 获得上一次发送数据的的消息数(周期内统计)
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetLastSecSendMsgNum() const
    {
        switch (time(NULL)-m_tLastSendMsgTime)
        {
        case 0:
            return m_uiLastSecSendMsgNum;
        case 1:
            return m_uiSecSendMsgNum;
        default:
            return 0;
        }
    }
    /**
     * @brief 获得总的接收的消息数
     * 
     * @return MDB_UINT64
     */
    inline MDB_UINT64 GetTotalRecvMsgNum() const
    {
        return m_uiTotalRecvMsgNum;
    }
    /**
     * @brief 获得总的发送的消息数
     * 
     * @return MDB_UINT64
     */
    inline MDB_UINT64 GetTotalSendMsgNum() const
    {
        return m_uiTotalSendMsgNum;
    }
    /**
     * @brief 获得当前周期的接收的消息数，也就是实时速率
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecRecvMsgNum() const
    {
        return time(NULL) == m_tLastRecvMsgTime?m_uiSecRecvMsgNum:0;
    }
    /**
     * @brief 获得当前周期的发送的消息数，也就是实时速率
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecSendMsgNum() const
    {
        return time(NULL) == m_tLastSendMsgTime?m_uiSecSendMsgNum:0;
    }
    /**
     * @brief 获得当前周期的总的消息数，也就是实时速率
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecTotalMsgNum() const
    {
        time_t ttCurSecTime = time(NULL);//保证接收和发送比较的是同一个时间戳
        return (ttCurSecTime==m_tLastRecvMsgTime?m_uiSecRecvMsgNum:0) + (ttCurSecTime==m_tLastSendMsgTime?m_uiSecSendMsgNum:0);
    }
    /**
     * @brief 累加接收消息数
     * 
     * @param uiRecvMsgNum [in] 新接收的消息数
     * @return 无
     */
    inline void AddRecvMsgNum(MDB_UINT32 uiRecvMsgNum = 1)
    {
        time_t ttCurTime = time(NULL);
        if(m_tLastRecvMsgTime == ttCurTime)
        {
            m_uiSecRecvMsgNum += uiRecvMsgNum;
        }
        else
        {
            m_tLastRecvMsgTime = ttCurTime;
            m_uiLastSecRecvMsgNum = m_uiSecRecvMsgNum;
            m_uiSecRecvMsgNum = uiRecvMsgNum;
        }
        m_uiTotalRecvMsgNum += uiRecvMsgNum;        
    }
    /**
     * @brief 累加接发送消息数
     * 
     * @param uiRecvMsgNum [in] 新发送的消息数
     * @return 无
     */
    inline void AddSendMsgNum(MDB_UINT32 uiSendMsgNum = 1)
    {
        time_t ttCurTime = time(NULL);
        if(m_tLastSendMsgTime == ttCurTime)
        {
            m_uiSecSendMsgNum += uiSendMsgNum;
        }
        else
        {
            m_tLastSendMsgTime = ttCurTime;
            m_uiLastSecSendMsgNum = m_uiSecSendMsgNum;
            m_uiSecSendMsgNum = uiSendMsgNum;
        }
        m_uiTotalSendMsgNum += uiSendMsgNum;
    }
protected:
    time_t  m_tLastRecvTime;      ///< 上一次收取数据时间
    time_t  m_tLastSendTime;      ///< 上一次发送数据时间
    time_t  m_tLastRecvMsgTime;      ///< 上一次收取数据时间
    time_t  m_tLastSendMsgTime;      ///< 上一次发送数据时间
    MDB_UINT32  m_uiLastSecRecvBytes;    ///< 上一流量周期收取的字节数
    MDB_UINT32  m_uiLastSecSendBytes;    ///< 上一流量周期发送的字节数
    MDB_UINT32  m_uiSecRecvBytes;    ///< 当前流量周期收取的字节数
    MDB_UINT32  m_uiSecSendBytes;    ///< 当前流量周期发送的字节数
    MDB_UINT64  m_uiTotalRecvBytes;   ///< 接收的总字节数
    MDB_UINT64  m_uiTotalSendBytes;   ///< 发送的总字节数
    MDB_UINT32  m_uiLastSecRecvMsgNum;    ///< 上一流量周期收取的消息数
    MDB_UINT32  m_uiLastSecSendMsgNum;    ///< 上一流量周期发送的消息数
    MDB_UINT32  m_uiSecRecvMsgNum;    ///< 当前流量周期收取的消息数
    MDB_UINT32  m_uiSecSendMsgNum;    ///< 当前流量周期发送的消息数
    MDB_UINT64  m_uiTotalRecvMsgNum;   ///< 接收的总消息数
    MDB_UINT64  m_uiTotalSendMsgNum;   ///< 发送的总消息数
};

/**
 * @brief 数据包类，单次收发数据得到的数据包，不一定是完整的一个消息包
 * 
 */
class TMdbPacketInfo:public TMdbNtcBaseObject
{
    MDB_ZF_DECLARE_OBJECT(TMdbPacketInfo);
protected:
    class TPacketData
    {
    public:    
        MDB_UINT32    uiRefcnt;///<引用个数
    };
public:
    MDB_UINT32 uiOffset;///< 相对buffer起始地址的偏移值
    MDB_UINT32 uiLength;///< packet的长度
    TMdbPacketInfo(MDB_UINT32 uiLength = 0);
    TMdbPacketInfo(char* pcBuffer, MDB_UINT32 uiLength = (MDB_UINT32)-1);
    TMdbPacketInfo(TMdbPacketInfo& oPacketInfo, MDB_UINT32 uiLength = (MDB_UINT32)-1);
    TMdbPacketInfo& operator = (TMdbPacketInfo& oPacketInfo)
    {
        Attach(oPacketInfo);
        return *this;
    }
    virtual ~TMdbPacketInfo()
    {
        Detach();
    }        
    char* AllocBuffer(MDB_UINT32 uiLength);
    void Attach(TMdbPacketInfo& oPacketInfo, MDB_UINT32 uiLength = (MDB_UINT32)-1);
    void Detach();
    inline MDB_UINT32 RefCnt() const
    {
        return reinterpret_cast<const TPacketData*>(m_pcData)->uiRefcnt;
    }
    inline char* GetBuffer()
    {
        return m_pcData?(m_pcData+sizeof(TPacketData)+uiOffset):NULL;
    }
    inline const char* GetBuffer() const
    {
        return m_pcData?(m_pcData+sizeof(TPacketData)+uiOffset):NULL;
    }
    inline MDB_UINT32 GetLength() const
    {
        return uiLength;
    }
    inline void SetLength(MDB_UINT32 uiLength)
    {
        this->uiLength = uiLength;
    }
protected:
    inline char* AddRef()
    {
        ms_oSpinLock.Lock();
        ++reinterpret_cast<TPacketData*>(m_pcData)->uiRefcnt;
        ms_oSpinLock.Unlock();
        //MDB_NTC_DEBUG("Attach[%p] ref_cnt:%d", m_pcData, RefCnt());
        return m_pcData;
    }
    char* m_pcData;///< 数据包信息和buffer, buffer实际长度比iLength多一字节存储'\0'
    static TMdbNtcThreadLock ms_oSpinLock;///< 自旋锁用于对引用计数的互斥
};

struct TMdbSpecTermMatchInfo
{
    TMdbNtcBaseList::iterator itLastMatchPacket;//针对指定结束符的上一次匹配的包位置
    MDB_UINT32  uiLastMatchCount;///< 表示匹配的终止符个数
    MDB_UINT32  uiLastMatchMsgLength;///< 上一次匹配完的数据包长度
    TMdbPacketInfo oSpecTermPacket;///< 缓存的从开始到终止符的数据包，用于校验（特别是消息头结束符的协议）
    TMdbSpecTermMatchInfo()
        :itLastMatchPacket(NULL), uiLastMatchCount(0), uiLastMatchMsgLength(0),oSpecTermPacket()
    {
    }
    ~TMdbSpecTermMatchInfo()
    {        
    }
    inline void Reset()
    {
        itLastMatchPacket.pNodeObject = NULL;
        uiLastMatchCount = uiLastMatchMsgLength = 0;
        oSpecTermPacket.Detach();
    }
};

/**
 * @brief 消息类，一个完整的消息包
 * 
 */
class TMdbMsgInfo:public TMdbNtcBaseObject
{    
public:
    MDB_ZF_DECLARE_OBJECT(TMdbMsgInfo);
    TMdbMsgInfo(MDB_UINT32 uiLength = 0);
    inline char* AllocBuffer(MDB_UINT32 uiLength)
    {
        return oPacketInfo.AllocBuffer(uiLength);
    }
    inline char* GetBuffer()
    {
        return oPacketInfo.GetBuffer();
    }
    inline const char* GetBuffer() const
    {
        return oPacketInfo.GetBuffer();
    }
    inline const char* GetHead() const
    {
        return GetBuffer();
    }
    inline const char* GetBody() const
    {
        return GetBuffer()+m_uiHeadLength;
    }
    inline MDB_UINT32 GetLength() const
    {
        return oPacketInfo.GetLength();
    }
    inline MDB_UINT32 GetHeadLength() const
    {
        return m_uiHeadLength;
    }
    inline MDB_UINT32 GetBodyLength() const
    {
        return GetLength()-m_uiHeadLength;
    }
    inline void SetLength(MDB_UINT32 uiLength)
    {
        oPacketInfo.SetLength(uiLength);
    }
    inline void SetHeadLength(MDB_UINT32 uiHeadLength)
    {
        this->m_uiHeadLength = uiHeadLength;
    }
    inline void AttachPacket(TMdbPacketInfo& oRefPacket, MDB_UINT32 uiLength = (MDB_UINT32)-1)
    {
        this->oPacketInfo.Attach(oRefPacket, uiLength);
    }
    inline void DetachPacket()
    {
        oPacketInfo.Detach();
    }
    /**
     * @brief 如果想要将消息留作后用，则需要增加引用
     * 
     */
    inline void AddRef()
    {
        ++m_uiRefcnt;
    }
    /**
     * @brief 消息指针的释放，都通过Release
     * 
     */
    inline void Release()
    {
        --m_uiRefcnt;
        if(m_uiRefcnt == 0)
        {
            delete this;
        }
    }    
    inline MDB_UINT32 RefCnt() const
    {
        return m_uiRefcnt;
    }
public:
    TMdbPacketInfo oPacketInfo;
protected:
    MDB_UINT32  m_uiRefcnt;///<引用个数
    MDB_UINT32  m_uiHeadLength;///< 消息包头长度
};

/**
 * @brief 已接收的数据包
 * 
 */
class TMdbRecvPackets:protected TMdbNtcBaseList
{
public:
    TMdbRecvPackets();
    ~TMdbRecvPackets();
    inline MDB_UINT32 GetTotalLength()
    {
        return m_uiTotalLength;
    }
    /**
     * @brief 添加packet
     * 
     * @param pPacketInfo [in] packet信息
     */
    inline void AddPacket(TMdbPacketInfo* pPacketInfo)
    {
        AddTail(pPacketInfo);
        m_uiTotalLength += pPacketInfo->GetLength();
    }
    using TMdbNtcBaseList::GetSize;
    using TMdbNtcBaseList::IterBegin;
    using TMdbNtcBaseList::IterEnd;
    /**
     * @brief 拼接数据
     * 会根据需要自行申请或直接置换packet的buffer
     * 
     * @param uiLength [in] 存放的buffer长度
     * @param oPacketInfo [out] 生成的buffer
     * @return bool
     * @retval true 成功
     */
    bool SplicePacket(MDB_UINT32 uiLength, TMdbPacketInfo& oPacketInfo);
protected:
    MDB_UINT32              m_uiTotalLength;///< 数据包总长度,能够用来快速判断是否达到完整的消息包
public:
    MDB_UINT32              uiSplicedLength;///< 已经拼接的长度
    TMdbMsgInfo*           pSplicingMsg;///< 正在拼接的msg
    TMdbSpecTermMatchInfo  oSpecTermMatchInfo;///< 上一次终止符匹配信息
};

/**
 * @brief 带发送的数据包
 * 
 */
class TMdbSendPackets:public TMdbNtcQueue
{
public:
    int iSendBytes;///< 已发送字节数
    TMdbSendPackets();
    ~TMdbSendPackets();
    using TMdbNtcQueue::IterBegin;
    using TMdbNtcQueue::IterEnd;
};

class TMdbProactorInfo
{
public:
    virtual ~TMdbProactorInfo()
    {
    }
};

class TMdbProtocol;
class TMdbPeerProactor;
/**
 * @brief 服务端信息
 * 
 */
class TMdbServerInfo: public TMdbNtcServerSocket
{
    MDB_ZF_DECLARE_OBJECT(TMdbServerInfo);
public:
    TMdbProtocol*              pProtocol;  ///< 所使用的协议
    const TMdbRuntimeObject* pPeerRuntimeObject;///< 连接的运行时期类，用于创建新的连接对象
    TMdbPeerProactor*          pPeerProactor;///< 监听套接字所使用的前摄器
    TMdbProactorInfo*          pProactorInfo;///< listen socket所使用的前摄器相关信息
    TMdbServerInfo(const char* pszServerHost = NULL, int iServerPort = 0, TMdbProtocol* pProtocol = NULL, const TMdbRuntimeObject* pPeerRuntimeObject = NULL);
    virtual ~TMdbServerInfo();
    /**
     * @brief 添加需要监听的事件
     * 
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @return bool
     * @retval true 成功
     */
    bool AddEventMonitor(MDB_UINT16 events);
    /**
     * @brief 移除不需要监听的事件
     * 
     * @param events [in] 不需要监听事件,如EV_READ,EV_WRITE
     * @return bool
     * @retval true 成功
     */
    bool RemoveEventMonitor(MDB_UINT16 events);
};

/**
 * @brief 比较函数，需要自定义比较时可用,重载Compare即可
 * 
 */
class TMdbServerInfoCompare:public TMdbNtcObjCompare
{
public:
    virtual MDB_INT64 Compare(const TMdbNtcBaseObject* pObject1, const TMdbNtcBaseObject* pObject2) const
    {
        return static_cast<const TMdbServerInfo*>(pObject1)->GetServerHost() == static_cast<const TMdbServerInfo*>(pObject2)->GetServerHost()
            && static_cast<const TMdbServerInfo*>(pObject1)->GetServerPort() == static_cast<const TMdbServerInfo*>(pObject2)->GetServerPort();
    }
};

class TMdbPeerInfo;
/**
 * @brief 连接的辅助工具
 * 
 */
class TMdbPeerHelper
{
public:
    TMdbPeerInfo*  pPeerInfo;    
    TMdbPeerHelper(TMdbPeerInfo* pPeerInfo = NULL);
    virtual ~TMdbPeerHelper();
};

/**
 * @brief 连接状态
 * 
 */
enum MDB_NTC_PEER_STATE
{
    MDB_NTC_PEER_IDLE_STATE         =   0x00,  ///< 连接处于空闲状态
    MDB_NTC_PEER_RECV_STATE         =   0x01,  ///< 连接处于接收数据状态
    MDB_NTC_PEER_WRITE_STATE        =   0x02,  ///< 连接处于发送数据状态
};
const MDB_UINT16 MDB_NTC_PEER_SHUTDOWN_FLAG     =   0x04;///< 表示连接是否等待关闭，关闭则表示不再接受新事件
const MDB_UINT16 MDB_NTC_PEER_PASV_CLOSE_FLAG   =   0x08;///< 表示是否被动关闭
const MDB_UINT16 MDB_NTC_PEER_EV_READ_FLAG      =   0x10;///< 表示连接正被监听读
const MDB_UINT16 MDB_NTC_PEER_EV_WRITE_FLAG     =   0x20;///< 表示连接正被监听写
const MDB_UINT16 MDB_NTC_PEER_MSG_DETACHED_FLAG =   0x40;///< 表示连接的消息独自处理

const MDB_UINT16 MDB_NTC_PEER_STATE_MASK    =   0x03;
const MDB_UINT16 MDB_NTC_PEER_EV_MASK       =   0x30;///< 读写事件的mask

/**
 * @brief 空闲超时的类型
 * 
 */
enum MDB_NTC_PEER_TIMEOUT_TYPE
{
    MDB_NTC_DATA_IDLE_TIMEOUT   =   0,///< 超时没有收发数据
    MDB_NTC_RECV_IDLE_TIMEOUT   =   1,///< 超时没有收到数据
    MDB_NTC_SEND_IDLE_TIMEOUT   =   2,///< 超时没有发送数据
    MDB_NTC_DURATION_TIMEOUT    =   3,///< 链接建立时长达到
    MDB_NTC_MAX_IDLE_TIMEOUT_TYPE
};

class TSendMsgEvent;
class TMdbEventPump;
/**
 * @brief peer连接信息
 * 
 */
class TMdbPeerInfo:public TMdbNtcSocket
{
    MDB_ZF_DECLARE_DYNCREATE_OBJECT(TMdbPeerInfo);
public:
    MDB_UINT16  pno;                ///< pno
    TMdbProactorInfo*  pProactorInfo;///< 连接所使用的前摄器信息
    TMdbPeerProactor*  pPeerProactor;///< 连接上的事件依赖的前摄器    
    TMdbServerInfo*    pServerInfo; ///< socket服务端信息
    TMdbEventPump*     pCurEventPump;///< 当前所处通道
    TMdbProtocol*      pProtocol;///< 连接所使用的协议，默认指向Server的Protocol    
    TMdbSendPackets    oSendPackets;   ///< 待发送的数据包
    TMdbRecvPackets    oRecvPackets;   ///< 已收到的数据包    
    TMdbNtcStringBuffer      sDisconnectReason;///< 断开原因
protected:
    MDB_UINT16          m_uiPeerFlag;       ///< flag标志位，可以得到有关pno的信息
    MDB_UINT32          m_uiTimerID[MDB_NTC_MAX_IDLE_TIMEOUT_TYPE];     ///< 空闲定时器id
    MDB_UINT32          m_uiMaxIdleTime[MDB_NTC_MAX_IDLE_TIMEOUT_TYPE];     ///< 最大空闲时间，>0表示有效
    time_t          m_tConnectime;      ///< 连接时间
    TMdbTrafficInfo    m_oTrafficInfo;///流量信息
    TMdbTrafficCtrl*   m_pTrafficCtrl;   ///< 流量控制器
    TMdbPeerHelper*    m_pPeerHelper;///< 用于为peer提供更好的接口或扩充信息
    TMdbNtcThreadLock  m_oSpinLock;
public:
    /**
     * @brief 构造函数
     *      
     */
    TMdbPeerInfo();
    virtual ~TMdbPeerInfo();
    /**
     * @brief 初始化
     * 
     * @param uiSocketID [in] socket描述符
     * @param pServerInfo [in] 服务端信息
     * @return 无
     */
    void Init(QuickMDB_SOCKET uiSocketID, TMdbServerInfo* pServerInfo = NULL);
    /**
     * @brief 设置helper
     * 
     * @param pPeerHelper [in] helper
     */
    inline void SetHelper(TMdbPeerHelper* pPeerHelper)
    {
        if(m_pPeerHelper)
        {
            delete m_pPeerHelper;
            m_pPeerHelper = NULL;
        }
        m_pPeerHelper = pPeerHelper;
        m_pPeerHelper->pPeerInfo = this;
    }
    /**
     * @brief 获得helper
     * 
     * @return TMdbPeerHelper*
     */
    inline TMdbPeerHelper* GetHelper()
    {
        return m_pPeerHelper;
    }
    /**
     * @brief 设置连接的定时器
     * 
     * @param eTimeoutType  [in] 超时的类型
     * @param uiSeconds     [in] 超时的时间，单位秒
     */
    void SetPeerTimeout(MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutType, MDB_UINT32 uiSeconds);
    /**
     * @brief 设置Timeout对应的定时器id
     * 
     * @param eTimeoutType  [in] 超时的类型
     * @param uiTimerId     [in] 定时器id
     */
    inline void SetTimeoutId(MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutType, MDB_UINT32 uiTimerId)
    {
        m_uiTimerID[eTimeoutType] = uiTimerId;
    }
    /**
     * @brief 获得最大空闲时间
     * 
     * @param eIdleTimeoutType [in] 空闲类型
     * @return int
     * @retval 最大空闲时间
     */
    inline MDB_UINT32 GetPeerTimeout(MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutType)
    {
        return m_uiMaxIdleTime[eTimeoutType];
    }
    /**
     * @brief 获得连接时间
     * 
     * @return time_t
     */
    inline time_t GetConnectTime()
    {
        return m_tConnectime;
    }
    /**
     * @brief 获得上一次接收数据时间
     * 
     * @return time_t
     */
    inline time_t GetLastRecvTime() const
    {
        return m_oTrafficInfo.GetLastRecvTime();
    }
    /**
     * @brief 获得上一次发送数据时间
     * 
     * @return time_t
     */
    inline time_t GetLastSendTime() const
    {
        return m_oTrafficInfo.GetLastSendTime();
    }
    /**
     * @brief 获得流量信息
     * 
     * @return TMdbTrafficCtrl&
     * @retval 流量信息
     */
    inline TMdbTrafficInfo* GetTrafficInfo()
    {
        return &m_oTrafficInfo;
    }    
    /**
     * @brief 获得流量控制器
     * 
     * @return TTrafficRateCtrl*
     */
    TMdbTrafficCtrl* GetTrafficCtrl()
    {
        return m_pTrafficCtrl;
    }
    /**
     * @brief 删除流量控制器
     *      
     * @return 无
     */
    void DeleteTrafficCtrl();
    /**
     * @brief 设置最大接收速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiMaxRecvRate [in] 总的最大速率
     * @return 无
     */
    void SetMaxRecvRate(MDB_UINT32 uiMaxRecvRate);
    /**
     * @brief 设置最大发送速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiMaxSendRate [in] 总的最大速率
     * @return 无
     */
    void SetMaxSendRate(MDB_UINT32 uiMaxSendRate);
    /**
     * @brief 设置最大接收和发送总速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiMaxFlowRate [in] 总的最大速率
     * @return 无
     */
    void SetMaxFlowRate(MDB_UINT32 uiMaxFlowRate);
    /**
     * @brief 获得总的最大接收速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetMaxRecvRate()
    {
        return m_pTrafficCtrl?m_pTrafficCtrl->GetMaxRecvRate():0;
    }
    /**
     * @brief 获得总的最大发送速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetMaxSendRate()
    {
        return m_pTrafficCtrl?m_pTrafficCtrl->GetMaxSendRate():0;
    }
    /**
     * @brief 获得总的最大接收和发送总速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetMaxFlowRate()
    {
        return m_pTrafficCtrl?m_pTrafficCtrl->GetMaxFlowRate():0;
    }
    /**
     * @brief 加锁
     * 
     * @return bool
     * @retval true 成功
     */
    inline bool Lock()
    {
        return m_oSpinLock.Lock();
    }
    /**
     * @brief 解锁
     * 
     * @return bool
     * @retval true 成功
     */
    inline bool Unlock()
    {
        return m_oSpinLock.Unlock();
    }
    /**
     * @brief 设置消息处理模式
     * 
     * @param bDetached [in] 是否分离开独立处理
     * @return 无
     */
    void SetMsgHandleMode(bool bDetached);
    /**
     * @brief 消息是否被独立处理
     * 
     * @return bool
     * @retval true 消息独立处理
     */
    inline bool IsMsgDetached()
    {
        return (m_uiPeerFlag&MDB_NTC_PEER_MSG_DETACHED_FLAG)==MDB_NTC_PEER_MSG_DETACHED_FLAG;
    }
    /**
     * @brief 设置连接标识
     * 
     * @param uiFlag [in] 连接标识
     * @return 无
     */
    inline void SetPeerFlag(MDB_UINT16 uiFlag)
    {
        if(uiFlag&MDB_NTC_PEER_MSG_DETACHED_FLAG)
        {
            SetMsgHandleMode(true);
            uiFlag = (MDB_UINT16)(uiFlag&(~MDB_NTC_PEER_MSG_DETACHED_FLAG));
            if(uiFlag == 0) return;
        }
        Lock();        
        m_uiPeerFlag = (MDB_UINT16)(m_uiPeerFlag|uiFlag);
        Unlock();
    }
    /**
     * @brief 获得连接标识状态
     * 
     * @param uiFlagMask [in] 连接标识
     * @return MDB_UINT16
     */
    inline MDB_UINT16 GetPeerFlag(MDB_UINT16 uiFlagMask = 0xFFFF)
    {
        return (MDB_UINT16)(m_uiPeerFlag&uiFlagMask);
    }
    /**
     * @brief 获得连接标识状态
     * 
     * @param uiFlag [in] 连接标识
     * @return bool
     */
    inline bool IsSetPeerFlag(MDB_UINT16 uiFlag)
    {
        return (m_uiPeerFlag&uiFlag)==uiFlag;
    }
    /**
     * @brief 清除连接标识
     * 
     * @param uiFlag [in] 连接标识
     * @return 无
     */
    inline void ClearPeerFlag(MDB_UINT16 uiFlag)
    {
        if(uiFlag&MDB_NTC_PEER_MSG_DETACHED_FLAG)
        {
            SetMsgHandleMode(false);
            uiFlag = (MDB_UINT16)(uiFlag&(~MDB_NTC_PEER_MSG_DETACHED_FLAG));
            if(uiFlag == 0) return;
        }
        Lock();
        m_uiPeerFlag = (MDB_UINT16)(m_uiPeerFlag&(~uiFlag));
        Unlock();
    }
    /**
     * @brief 是否被动关闭
     * 
     * @return bool
     * @retval true 是被动关闭
     */
    inline bool IsPasvClose()
    {
        return (m_uiPeerFlag&MDB_NTC_PEER_PASV_CLOSE_FLAG)==MDB_NTC_PEER_PASV_CLOSE_FLAG;
    }
    /**
     * @brief 获得连接状态
     * 
     * @return PEER_TYPE
     * @retval 连接类型
     */
    inline MDB_NTC_PEER_STATE GetPeerState()
    {
        return (MDB_NTC_PEER_STATE)(m_uiPeerFlag&MDB_NTC_PEER_STATE_MASK);
    }
    /**
     * @brief 设置连接状态
     * 
     * @param ePeerState [in] 连接状态
     * @return 无
     */
    inline void SetPeerState(MDB_NTC_PEER_STATE ePeerState)
    {
        Lock();
        m_uiPeerFlag = (MDB_UINT16)((m_uiPeerFlag&(~MDB_NTC_PEER_STATE_MASK))|ePeerState);
        Unlock();
    }
    /**
     * @brief 连接是否已被shutdown关闭，如果是，则不要再往此连接上产生事件
     * 
     * @return bool
     * @retval true 关闭
     */
    inline bool IsShutdown()
    {
        return (m_uiPeerFlag & MDB_NTC_PEER_SHUTDOWN_FLAG)==MDB_NTC_PEER_SHUTDOWN_FLAG;
    }
    /**
     * @brief 同步方式发送数据
     * 
     * @param pszMsg [in] 消息包
     * @param uiMsgLength [in] 消息包长度,-1表示以\0结尾
     * @param iMilliSeconds [in] 超时时间（单位毫秒）,-1表示不超时
     * @return int
     * @retval 设定的时间内，实际发送字节数
     */
    virtual int SendMessage(const void* pszMsg, MDB_UINT32 uiMsgLength = (MDB_UINT32)-1, int iMilliSeconds = -1);
    /**
     * @brief 异步方式发送数据,放人事件队列中
     * 
     * @param pMsg [in] 消息包
     * @param uiMsgLength [in] 消息包长度,-1表示字符串以\0结尾
     * @return bool
     * @retval true 成功
     */
    virtual bool PostMessage(const void* pMsg, MDB_UINT32 uiMsgLength = (MDB_UINT32)-1);
    bool PostMessage(TMdbPacketInfo* pPacketInfo);
    /**
     * @brief 获取一个待处理消息事件(阻塞)
     * 
     * @param iMilliSeconds [in] 超时设置，单位为毫秒，-1表示不超时
     * @return TMdbMsgInfo*
     * @retval 非NULL 成功
     */
    inline TMdbMsgInfo* GetMessage(int iMilliSeconds = -1)
    {
        if(m_pRecvMsgQueue) return static_cast<TMdbMsgInfo*>(m_pRecvMsgQueue->Pop(iMilliSeconds));
        else return NULL;
    }
    /**
     * @brief 压入一个消息
     * 
     * @param pMsgInfo [in] 消息
     * @return bool
     */
    inline bool AddRecvMessage(TMdbMsgInfo* pMsgInfo)
    {
        if(m_pRecvMsgQueue)
        {
            pMsgInfo->AddRef();
            m_pRecvMsgQueue->Push(pMsgInfo);
            return true;
        }
        else return false;
    }
    /**
     * @brief 清理掉所有收到的消息
     * 
     */
    void ClearRecvMessage();
    /**
     * @brief 关闭连接
     * 
     * @param sReason       [in] 关闭连接的原因
     * @param bPasvClose    [in] 是否被动关闭(也就是对方主动关闭的)
     * @return bool
     * @retval true 成功
     */
    virtual bool Disconnect(TMdbNtcStringBuffer sReason = "", bool bPasvClose = false);
    /**
     * @brief 获得与此连接相关的前摄器所使用的分发器
     * 
     * @return TMdbEventDispatcher*
     * @retval 分发器
     */
    inline TMdbEventDispatcher* GetEventDispatcher()
    {
        return pPeerProactor?pPeerProactor->GetEventDispatcher():NULL;
    }
    /**
     * @brief 是否作为客户端连接
     * 
     * @return bool
     * @retval true 是
     */
    inline bool IsClient()
    {
        return pServerInfo && pServerInfo->GetSocketID() == MDB_NTC_INVALID_SOCKET;
    }
    virtual MDB_INT64 Compare(const TMdbNtcBaseObject *pObject) const
    {
        return pno-static_cast<const TMdbPeerInfo*>(pObject)->pno;
    }
    /**
     * @brief 检查接收到的消息包
     * 
     */
    void CheckRecvPackets();
    /**
     * @brief 拼接消息包,当有完整消息包时，触发OnRecvMsg事件
     * 
     * @return bool
     * @retval true 成功
     */
    bool SpliceMsg();
public://与事件监控与否有关的函数
    /**
     * @brief 添加需要监听的事件
     * 
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @param bLock [in] 是否需要加锁互斥，默认需要
     * @return bool
     * @retval true 成功
     */
    bool AddEventMonitor(MDB_UINT16 events, bool bLock = true);
    /**
     * @brief 移除不需要监听的事件
     * 
     * @param events [in] 不需要监听事件,如EV_READ,EV_WRITE
     * @param bLock [in] 是否需要加锁互斥，默认需要
     * @return bool
     * @retval true 成功
     */
    bool RemoveEventMonitor(MDB_UINT16 events, bool bLock = true);
protected:
    /**
     * @brief 关闭连接，不再接收新的事件
     * 
     * @return bool
     * @retval true 成功
     */
    bool Shutdown();
    /**
     * @brief 检查空闲的连接(定时器的回调)
     * 
     * @param pArg [in] TCheckIdlePeer结构信息
     * @return 无
     */
    static void CheckPeerTimeout(void* pArg);
protected:
    TMdbNtcQueue* m_pRecvMsgQueue;///< 如果采用消息分离模式，那么此链路消息会进入此队列，不再走OnRecvMsg
};
MDB_UINT32 MdbNtcAddTimer(const char *ATimerName,bool ALoopFlag,MDB_UINT32 ACount100ms,OnMdbEventFunc AEventFunc,void *pFuncParam);
//}
#endif
