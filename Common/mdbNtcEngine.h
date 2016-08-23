/**
* @file NtcEngine.hxx
 * 网络通信组件
 * 
 * @author jiang.jinzhou@zte.com.cn
 * @version 1.0
 * @date 2012/04/24
 * @bug 新建，无bug
 * @bug 单元测试，未发现bug
 * @warning 
 */
#ifndef _MDB_NTC_ENGINE_HXX_
#define _MDB_NTC_ENGINE_HXX_
#include "Common/mdbSysThreads.h"
#include "Common/mdbNTC.h"
#include "Common/mdbPeerInfo.h"
#include "Common/mdbPeerEvent.h"
//namespace QuickMDB
//{
mdb_ntc_extern_thread_local(TMdbNtcThreadEvent*, g_pSendThreadEvent);
/**
 * @brief 代理基类
 * 
 */
 class TMdbNtcBaseProxy:public TMdbNtcBaseObject
{
    MDB_ZF_DECLARE_OBJECT(TMdbNtcBaseProxy);
public:
    virtual QuickMDB_SOCKET Connect(const char* pszRemote, int iPort, int iMilliSeconds = -1) const = 0;
};

/**
 * @brief 无代理
 * 
 */
class TMdbNtcNonProxy:public TMdbNtcBaseProxy
{
    MDB_ZF_DECLARE_OBJECT(TMdbNtcNonProxy);
public:
    TMdbNtcNonProxy();
    QuickMDB_SOCKET Connect(const char* pszRemote, int iPort, int iMilliSeconds = -1) const;
};

extern const TMdbNtcNonProxy g_oMdbNtcNonProxy;///< 默认不使用代理

/**
 * @brief http代理
 * 
 */
class TMdbNtcHttpProxy:public TMdbNtcBaseProxy
{
    MDB_ZF_DECLARE_OBJECT(TMdbNtcHttpProxy);
public:
    TMdbNtcHttpProxy(const char* pszProxyAddress, int iProxyPort = 80, const char* pszUserName = NULL, const char* pszPassword = NULL);
    virtual QuickMDB_SOCKET Connect(const char* pszRemote, int iPort = 80, int iMilliSeconds = -1) const;
protected:
    TMdbNtcStringBuffer   m_sProxyAddress;
    int m_iProxyPort;
    TMdbNtcStringBuffer   m_sUserName;
    TMdbNtcStringBuffer   m_sPassword;
};
/**
 * @brief 事件泵,对应一个线程
 * 
 */
class TMdbPeerEventPump:public TMdbEventPump
{
    MDB_ZF_DECLARE_OBJECT(TMdbPeerEventPump);
public:
    TMdbPeerEventPump();
    MDB_UINT32 uiFdCount;///< 此事件泵上的fd数目
};

/**
 * @brief socket组件的基类，提供前摄器,事件分发器和事件处理器的封装
 * 对于socket可读，可写等操作的处理
 * 
 */
class TMdbNtcEngine:public TMdbEventDispatcher, public TMdbEventHandler
{
public:
    TMdbNtcEngine();
    virtual ~TMdbNtcEngine();
    /**
     * @brief 判断网络通讯框架是否已经启动
     * 
     * @return bool
     * @retval true 已经启动了
     */
    bool IsStart();
    //网络通讯框架的启动和停止
    /**
     * @brief 开始启动网络通讯
     * 将会启动前摄器，并检查事件泵数目，如果没有事件泵，则初始化一个
     * 
     * @return bool
     * @retval true 成功
     */
    virtual bool Start();
    /**
     * @brief 停止网络通讯
     * 
     * @return bool
     * @retval true 成功
     */
    virtual bool Stop();

	 /**
     * @brief 强制杀死网络通讯
     * 
     * @return bool
     * @retval true 成功
     */
    virtual bool Kill();
		
    /**
     * @brief 由系统自动选择适合的前摄器
     * 
     * @return bool
     * @retval true 成功
     */
    bool InitSocketProactor();
    /**
     * @brief 初始化socket前摄器
     * 
     * @param emProactorType [in] 打算使用的前摄器
     * @return bool
     * @retval true 成功
     */
    bool InitSocketProactor(MDB_PEER_PROACTOR_TYPE emProactorType);
    /**
     * @brief 设置最大的连接数
     * 
     * @param uiMaxPeerNo [in]最大的连接数
     * @return 无
     */
    void SetMaxPeerNo(MDB_UINT16 uiMaxPeerNo);
    /**
     * @brief 获得最大的连接数
     * 
     * @return MDB_UINT16
     * @retval 连接数
     */
    inline MDB_UINT16 GetMaxPeerNo()
    {
        return (MDB_UINT16)m_oIdMgr.GetPoolSize();
    }
    /**
     * @brief 连接服务端
     * 
     * @param pszRemote [in] 远程服务端的ip地址或域名
     * @param iPort [in] 远程服务端的端口
     * @param pProtocol [in] 远程服务端的所使用的协议，如果为NULL，则会将收到的数据无协议解析直接生成新消息
     * @param iMilliSeconds [in] 连接超时时间(毫秒)，超过此值则判为连接失败,取值为-1表示按默认时间来(3s)
     * @param uiPeerFlag [in] 连接的flag设置
     * @param pPeerRuntimeObject [in] 连接所对应的运行时期对象，用于创建连接实例
     * @param g_oMdbNtcNonProxy   [in] 所使用的代理
     * @return TMdbSharedPtr<TMdbPeerInfo>
     * @retval NULL 表示未连接成功
     */
    TMdbSharedPtr<TMdbPeerInfo> Connect(const char* pszRemote, int iPort, TMdbProtocol* pProtocol = NULL, int iMilliSeconds = -1, MDB_UINT16 uiPeerFlag = 0,
        const TMdbRuntimeObject* pPeerRuntimeObject = NULL, const TMdbNtcBaseProxy& oProxy = g_oMdbNtcNonProxy);
    /**
     * @brief 监听本机的某一端口
     * 
     * @param pszAddress [in] pszAddress为NULL表示所有ip
     * @param iPort [in] 要绑定的端口
     * @param pProtocol [in] 指定使用的协议，如果为NULL，则会将收到的数据无协议解析直接生成新消息
     * @param pPeerRuntimeObject [in] 连接所对应的运行时期对象，用于创建连接实例
     * @return bool
     * @retval true 成功
     */
    bool AddListen(const char* pszAddress, int iPort, TMdbProtocol* pProtocol = NULL, const TMdbRuntimeObject* pPeerRuntimeObject = NULL);
    /**
     * @brief 断开指定的连接
     * 
     * @param pno [in] 连接号
     * @return bool
     * @retval true 成功
     */
    bool Disconnect(MDB_UINT16 pno);
public://下面与pno信息维护有关
    /**
     * @brief 获得当前的连接数
     * 
     * @return MDB_UINT16
     * @retval 连接数
     */
    inline MDB_UINT16 GetPeerCount()
    {
        return (MDB_UINT16)m_oIdMgr.GetUsedSize();
    }
    /**
     * @brief 添加连接信息
     * 
     * @param new_fd [in] 新的描述符信息
     * @param pServerInfo [in] server信息
     * @param uiPeerFlag [in] 连接上的flag设置
     * @return TMdbSharedPtr<TMdbPeerInfo>
     * @retval socket连接信息
     */
    TMdbSharedPtr<TMdbPeerInfo> AddPeerInfo(QuickMDB_SOCKET new_fd, TMdbServerInfo* pServerInfo, MDB_UINT16 uiPeerFlag = 0);
    /**
     * @brief 删除连接信息
     * 
     * @param pno [in] 连接号
     * @return 无
     */
    inline void DelPeerInfo(MDB_UINT16 pno)
    {
        if(m_ppPeerNo && m_ppPeerNo[pno])
        {
            DelPeerInfo(m_ppPeerNo[pno]);
        }
    }
    /**
     * @brief 删除连接信息
     * 
     * @param pPeerInfo [in] 连接信息
     * @return 无
     */
    void DelPeerInfo(TMdbPeerInfo* pPeerInfo);
        /**
     * @brief 根据连接号查找连接信息
     * 
     * @param pno [in] 连接号
     * @return TMdbPeerInfo*
     */
    inline TMdbSharedPtr<TMdbPeerInfo>& FindPeerInfo(MDB_UINT16 pno)
    {
        return (m_ppPeerNo&&pno<m_oIdMgr.GetPoolCapacity())?m_ppPeerNo[pno]:ms_pNullPeerInfo;
    }
    /**
     * @brief 查找socket信息
     * 
     * @param fd [in] socket描述符
     * @return TMdbNtcSocket*
     */
    inline TMdbNtcSocket* FindSocketInfo(QuickMDB_SOCKET fd)
    {
        return m_arraySocketInfo[fd];
    }
    /**
     * @brief 获得Server信息
     * 
     * @param pszIP [in] server的ip
     * @param iPort [in] server的port
     * @return TMdbServerInfo*
     */
    TMdbServerInfo* FindServerInfo(const char* pszIP, int iPort);
public://下面是虚函数部分
    /**
     * @brief 判断连接是否被允许接入（当有新客户端连接时，在OnConnect事件之前）
     * 
     * @param new_fd [in] socket描述字
     * @param pServerInfo [in] 服务端信息
     * @return bool
     * @retval true 允许接入
     * @retval false 不允许接入
     */
    virtual bool CheckNewClient(QuickMDB_SOCKET new_fd, TMdbServerInfo* pServerInfo)
    {
        return true;
    }
    /**
     * @brief 生成一个socket连接信息
     * 
     * @return TMdbPeerInfo*
     * @retval socket连接信息
     */
    virtual TMdbPeerInfo* CreatePeerInfo();
    /*
     * @brief 创建一个事件泵
     * 
     * @return TMdbEventPump*
     * @retval 创建的事件泵
     */
    virtual TMdbEventPump* CreateEventPump();
    /**
     * @brief 处理事件
     * 
     * @param pEventInfo [in] 事件
     * @param pEventPump [in] 事件泵
     * @return bool
     * @retval true 成功
     */
    virtual bool ProcessEvent(TMdbEventInfo* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief 分发事件
     * 
     * @param pEventInfo [in] 事件
     * @return TMdbEventPump*
     * @retval 使用的事件泵
     */
    virtual TMdbEventPump* Dispatch(TMdbEventInfo* pEventInfo);
protected://下面是事件处理部分
    /**
     * @brief 预分发，仅仅是为了获得适合的事件泵，并不分发
     * 
     * @param pEventInfo [in] 事件     
     * @return TMdbEventPump*
     * @retval 适合的事件泵
     */
    virtual TMdbEventPump* PreDispatch(TMdbEventInfo* pEventInfo);
    /**
     * @brief 当有socket连接或者socket连接成功时，触发此函数
     * 
     * @param pEventInfo [in] 事件信息
     * @param pEventPump [in] 事件泵
     * @return bool
     * @retval true 开始监听读事件并处理
     * @retval false 不监听读事件，可由手动监听
     */
    virtual bool OnConnect(TMdbConnectEvent* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief 当有连接断开时，触发此函数
     * 
     * @param pEventInfo [in] 事件信息
     * @param pEventPump [in] 事件泵
     * @return bool
     * @retval true 成功
     */
    virtual bool OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief 当客户端有消息来的时候，触发此函数
     * 
     * @param pEventInfo [in] 事件信息
     * @param pEventPump [in] 事件泵
     * @return bool
     * @retval true 成功
     */
    virtual bool OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief 当空闲超时时，触发此函数
     * 
     * @param pEventInfo [in] 事件信息
     * @param pEventPump [in] 事件泵
     * @return bool
     * @retval true 继续设置超时
     * @retval false 取消超时
     */
    virtual bool OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief 当发生错误时，触发此函数，如解析消息包出错等
     * 
     * @param pEventInfo [in] 事件信息
     * @param pEventPump [in] 事件泵
     * @return bool
     * @retval true 成功
     */
    virtual bool OnError(TMdbErrorEvent* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief 扩展的事件，默认调用此函数
     * 
     * @param pEventInfo [in] 事件信息
     * @param pEventPump [in] 事件泵
     * @return bool
     * @retval true 成功
     */
    virtual bool OnOtherEvent(TMdbEventInfo* pEventInfo, TMdbEventPump* pEventPump)
    {
        return true;
    }
public://下面与流量相关
    /**
     * @brief 获得流量信息
     * 
     * @return TMdbTrafficCtrl&
     * @retval 流量信息
     */
    TMdbTrafficInfo* GetTotalTrafficInfo()
    {
        return &m_oTotalTrafficInfo;
    }
    /**
     * @brief 设置总的最大接收速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiTotalMaxRecvRate [in] 总的最大速率
     * @return 无
     */
    inline void SetTotalMaxRecvRate(MDB_UINT32 uiTotalMaxRecvRate)
    {
        m_oTotalTrafficCtrl.SetMaxRecvRate(uiTotalMaxRecvRate);
    }
    /**
     * @brief 设置总的最大发送速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiTotalMaxSendRate [in] 总的最大速率
     * @return 无
     */
    inline void SetTotalMaxSendRate(MDB_UINT32 uiTotalMaxSendRate)
    {
        m_oTotalTrafficCtrl.SetMaxSendRate(uiTotalMaxSendRate);
    }
    /**
     * @brief 设置总的最大接收和发送总速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiTotalMaxFlowRate [in] 总的最大速率
     * @return 无
     */
    inline void SetTotalMaxFlowRate(MDB_UINT32 uiTotalMaxFlowRate)
    {
        m_oTotalTrafficCtrl.SetMaxFlowRate(uiTotalMaxFlowRate);
    }
    /**
     * @brief 获得总的最大接收速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetTotalMaxRecvRate()
    {
        return m_oTotalTrafficCtrl.GetMaxRecvRate();
    }
    /**
     * @brief 获得总的最大发送速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetTotalMaxSendRate()
    {
        return m_oTotalTrafficCtrl.GetMaxSendRate();
    }
    /**
     * @brief 获得总的最大接收和发送总速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetTotalMaxFlowRate()
    {
        return m_oTotalTrafficCtrl.GetMaxFlowRate();
    }
    /**
     * @brief 设置单条连接上的最大接收速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiTotalMaxRecvRate [in] 总的最大速率
     * @return 无
     */
    inline void SetPeerMaxRecvRate(MDB_UINT32 uiPeerMaxRecvRate)
    {
        m_oPeerTrafficCtrl.SetMaxRecvRate(uiPeerMaxRecvRate);
    }
    /**
     * @brief 设置单条连接上的最大发送速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiPeerMaxSendRate [in] 总的最大速率
     * @return 无
     */
    inline void SetPeerMaxSendRate(MDB_UINT32 uiPeerMaxSendRate)
    {
        m_oPeerTrafficCtrl.SetMaxSendRate(uiPeerMaxSendRate);
    }
    /**
     * @brief 设置单条连接上的最大接收和发送总速率，单位为bytes/s，=0表示不设置
     * 
     * @param uiPeerMaxFlowRate [in] 总的最大速率
     * @return 无
     */
    inline void SetPeerMaxFlowRate(MDB_UINT32 uiPeerMaxFlowRate)
    {
        m_oPeerTrafficCtrl.SetMaxFlowRate(uiPeerMaxFlowRate);
    }
    /**
     * @brief 获得单条连接上的最大接收速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetPeerMaxRecvRate()
    {
        return m_oPeerTrafficCtrl.GetMaxRecvRate();
    }
    /**
     * @brief 获得单条连接上的最大发送速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetPeerMaxSendRate()
    {
        return m_oPeerTrafficCtrl.GetMaxSendRate();
    }
    /**
     * @brief 获得单条连接上的最大接收和发送总速率，单位为bytes/s，=0表示不设置
     * 
     * @return 无
     */
    inline MDB_UINT32 GetPeerMaxFlowRate()
    {
        return m_oPeerTrafficCtrl.GetMaxFlowRate();
    }
    /**
     * @brief 获得连接当前可以发送的字节数
     * 
     * @param pPeerInfo [in] 连接信息
     * @param uiRecvBytes [in] 需要接收的字节数
     * @return MDB_UINT32
     * @retval 可以接收的字节数
     */
    MDB_UINT32 GetPeerAllowRecvBytes(TMdbPeerInfo* pPeerInfo, MDB_UINT32 uiRecvBytes);
    /**
     * @brief 获得连接当前可以发送的字节数
     * 
     * @param pPeerInfo [in] 连接信息
     * @param uiSendBytes [in] 需要发送的字节数
     * @return MDB_UINT32
     * @retval 可以接收的字节数
     */
    MDB_UINT32 GetPeerAllowSendBytes(TMdbPeerInfo* pPeerInfo, MDB_UINT32 uiSendBytes);
    /**
     * @brief 累加接收字节数
     * 
     * @param uiRecvBytes [in] 新接收的字节数
     * @return 无
     */
    inline void AddTotalRecvBytes(MDB_UINT32 uiRecvBytes)
    {
        m_oSpinLock.Lock();
        m_oTotalTrafficInfo.AddRecvBytes(uiRecvBytes);
        m_oSpinLock.Unlock();
    }
    /**
     * @brief 累加接发送节数
     * 
     * @param uiRecvBytes [in] 新发送的字节数
     * @return 无
     */
    inline void AddTotalSendBytes(MDB_UINT32 uiSendBytes)
    {
        m_oSpinLock.Lock();
        m_oTotalTrafficInfo.AddSendBytes(uiSendBytes);
        m_oSpinLock.Unlock();
    }
protected:
    TMdbIdMgr                          m_oIdMgr;///< ID分配器
    TMdbNtcBaseList   m_lsServerInfo;///< 服务端信息
    TMdbSharedPtr<TMdbPeerInfo>*          m_ppPeerNo;        ///< pno对应peer信息
    TMdbNtcSocket*    m_arraySocketInfo[65536];
    TMdbNtcThreadLock   m_oSpinLock;///< 自旋锁，用于短时间的互斥
    TMdbPeerProactor*                  m_pSocketProactor;///< 所用的socket前摄器
    
    TMdbTrafficCtrl                    m_oTotalTrafficCtrl;///< 总的流量速率限制
    TMdbTrafficCtrl                    m_oPeerTrafficCtrl;///< 每个链接的统一的流量限制，如果某个连接有了单独限制，将不参考此设置
    TMdbTrafficInfo                    m_oTotalTrafficInfo;///< 总的链路流量信息
private:
    static TMdbSharedPtr<TMdbPeerInfo>    ms_pNullPeerInfo;///< 用于返回引用
};

/**
 * @brief 异步模式
 * 
 */
typedef TMdbNtcEngine TMdbAsyncEngine;

/**
 * @brief 同步模式
 * 
 */
class TMdbSyncEngine:public TMdbNtcEngine
{
public:
    TMdbSyncEngine();
    virtual ~TMdbSyncEngine();
    /**
     * @brief 获取一个待处理消息事件(阻塞)
     * 
     * @param iMilliSeconds [in] 超时设置，单位为毫秒，-1表示不超时
     * @return TMdbRecvMsgEvent*
     * @retval 非NULL 成功
     */
    inline TMdbRecvMsgEvent* GetMessage(int iMilliSeconds = -1)
    {
        return static_cast<TMdbRecvMsgEvent*>(m_oMsgQueue.Pop(iMilliSeconds));
    }
protected:
    /**
     * @brief 当客户端有消息来的时候，触发此函数，对于同步模式，会将消息压入到消息泵中
     * 
     * @param pEventInfo [in] 事件信息
     * @param pEventPump [in] 事件泵
     * @return bool
     * @retval true 成功
     */
    virtual bool OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump);
    virtual bool ProcessEvent(TMdbEventInfo* pEventInfo, TMdbEventPump* pEventPump);
protected:
    //可能有多个事件泵向消息队列中压入，所以需要使用尾锁，但只有一个线程GetMessage所以头部无需加锁
    TMdbNtcQueue m_oMsgQueue;///< 阻塞式的消息队列
};
//}
#endif
