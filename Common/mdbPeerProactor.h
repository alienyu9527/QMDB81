/**
 * @file PeerProactor.hxx
 * 连接的前摄器定义
 * 
 * @author w
 * @version 1.0
 * @date 2012/10/06
 * @bug 新建，无bug
 * @bug 单元测试，未发现bug
 * @warning 
 */
#ifndef _MDB_PEER_PROACTOR_H_
#define _MDB_PEER_PROACTOR_H_
#include "Common/mdbNTC.h"
//namespace QuickMDB
//{
/**
 * @brief 创建管道
 * 
 * @param type [in] SOCK_STREAM或SOCK_DGRAM
 * @param protocol [in] 协议簇
 * @param sock [out] 创建成功后返回的对等两端套接字
 * @return bool
 * @retval true 成功
 */
int mdb_ntc_socket_pair(int type, int protocol, unsigned int sock[2]);
int mdb_ntc_get_need_read_size(unsigned int fd);

class TMdbEventDispatcher;
/**
 * @brief 前摄器
 * 
 */
class TMdbProactor
{
public:
    TMdbProactor():m_pEventDispatcher(NULL)
    {
    }
    virtual ~TMdbProactor()
    {
    }
    /**
     * @brief 设置事件分发器
     * 
     * @param pEventDispatcher [in] 事件分发器
     */
    inline void SetEventDispatcher(TMdbEventDispatcher* pEventDispatcher)
    {
        m_pEventDispatcher = pEventDispatcher;
    }
    /**
     * @brief 获得事件分发器
     * 
     * @param pEventDispatcher [in] 事件分发器
     */
    inline TMdbEventDispatcher* GetEventDispatcher()
    {
        return m_pEventDispatcher;
    }
    /**
     * @brief 前摄器启动
     * 
     * @return bool
     * @retval true 成功
     */
    virtual bool Start() = 0;
    /**
     * @brief 前摄器停止
     * 
     * @return bool
     * @retval true 成功
     */
    virtual bool Stop() = 0;
protected:
    TMdbEventDispatcher* m_pEventDispatcher;///< 指定的事件分发器
};

/**
 * @brief 信号前摄器
 * 能够为进程提供信号的摄取和处理接口
 * 
 */
 class TMdbSignalProactor:public TMdbProactor
 {
 public:
     /**
     * @brief 前摄器启动
     * 
     * @return bool
     * @retval true 成功
     */
    virtual bool Start();
    /**
     * @brief 前摄器停止
     * 
     * @return bool
     * @retval true 成功
     */
    virtual bool Stop();
 };

/**
 * @brief 连接所使用的前摄器类型
 * 
 */
enum MDB_PEER_PROACTOR_TYPE
{
    MDB_PROACTOR_SELECT    =   0,//general

#ifndef _WIN32
    MDB_PROACTOR_POLL      =   1,//linux/unix
#endif

#if defined(OS_LINUX) || defined(_LINUX)
    MDB_PROACTOR_EPOLL     =   2,//linux
#elif defined(OS_IBM)
    MDB_PROACTOR_POLLSET   =   3,//aix
#elif defined(OS_SUN)
    MDB_PROACTOR_EVENT_PORTS     =   4,//solaris
#elif defined(_WIN32)
    MDB_PROACTOR_IOCP      =   5,//win32
#elif defined(OS_FREEBSD) || defined(_FREEBSD)
    MDB_PROACTOR_KQUEUE    =   6,//freebsd
#endif
};

class TMdbNtcEngine;
class TMdbServerInfo;
class TMdbPeerInfo;

/**
 * @brief socket前摄器
 * 此模式是io操作完成后向用户通知
 * 前摄器基于线程，也就是由单独线程摄取io事件
 * 
 */
class TMdbPeerProactor:public TMdbProactor, public TMdbNtcThread
{
    friend class TMdbServerInfo;
    friend class TMdbPeerInfo;
    MDB_ZF_DECLARE_OBJECT(TMdbPeerProactor);
protected:
    /**
     * @brief 异步发起事件监听的变更
     * 
     */
    struct ASYNC_MODIFY_MONITOR
    {
        bool    bAddMonitor;///< 是否是添加事件
        MDB_UINT16 ntc_events;     ///< 要添加或移除监听的事件
        QuickMDB_SOCKET  fd;         ///< 描述符
        ASYNC_MODIFY_MONITOR()
        {
            memset(this, 0x00, sizeof(ASYNC_MODIFY_MONITOR));
            fd = MDB_NTC_INVALID_SOCKET;
        }
    };
public:
    TMdbPeerProactor();
    virtual ~TMdbPeerProactor();
    /**
     * @brief 初始化前摄器
     * 
     * @param pAcpfEngine [in] peer连接管理器
     * @return bool
     * @retval true 成功
     */
    virtual bool Init(TMdbNtcEngine* pAcpfEngine);
    /**
     * @brief 获得PeerMgr
     * 
     * @return TPeerManager*
     * @retval peer连接管理器
     */
    inline TMdbNtcEngine* GetPeerHelper()
    {
        return m_pNtcEngine;
    }
    /**
     * @brief 前摄器启动
     * 
     * @return bool
     * @retval true 成功
     */
    virtual bool Start();
    /**
     * @brief 前摄器停止
     * 
     * @return bool
     * @retval true 成功
     */
    virtual bool Stop();
    /**
     * @brief 唤醒前摄器
     *      
     * @return 无
     */
    virtual void Wakeup();
    /**
     * @brief 清理前摄器
     * 
     * @return 无
     */
    virtual void Cleanup();
    /**
     * @brief 处理server的accept请求
     * 
     * @param pServerInfo [in] server信息
     * @return bool
     * @retval true 成功，继续监听accept
     * @retval false 失败，无需监听accept
     */
    bool OnServerAccept(TMdbServerInfo* pServerInfo);
    /**
     * @brief 收取peer上的数据
     * 
     * @param pPeerInfo [in] 连接信息
     * @return bool
     * @retval true 继续监听可读
     * @retval false 停止监听可读
     */
    bool OnPeerRecv(TMdbPeerInfo* pPeerInfo);
    /**
     * @brief 发送peer上的数据
     * 
     * @param pPeerInfo [in] 连接信息
     * @return bool
     * @retval true 继续监听可写
     * @retval false 停止监听可写
     */
    bool OnPeerSend(TMdbPeerInfo* pPeerInfo);
protected:
    /**
     * @brief 处理异步操作
     * 
     */
    void DealAsyncOperator();
    /**
     * @brief 添加新的客户端连接
     * 
     * @param new_fd [in] 客户端fd
     * @param pServerInfo [in] server信息
     * @return bool
     * @retval true 成功
     */
    virtual bool AddNewClient(QuickMDB_SOCKET new_fd, TMdbServerInfo* pServerInfo);
    /**
     * @brief 添加需要监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @return 无
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events) = 0;
    /**
     * @brief 去除监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要去除的事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events) = 0;
    /**
     * @brief 异步添加需要监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @return 无
     */
    bool AsyncAddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief 异步去除监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要去除的事件,如EV_READ,EV_WRITE
     * @return bool
     */
    bool AsyncRemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    QuickMDB_SOCKET intr_fd[2];///< socket pair用于中断select等循环
    TMdbNtcThreadLock spinlock;
    TMdbNtcEngine* m_pNtcEngine;
};

/**
 * @brief select模型
 * 
 */
class TMdbProactorSelect:public TMdbPeerProactor
{
    MDB_ZF_DECLARE_OBJECT(TMdbProactorSelect);
public:
    TMdbProactorSelect();
    /**
     * @brief 清理前摄器
     * 
     * @return 无
     */
    virtual void Cleanup();
protected:
    /**
     * @brief 添加需要监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @return 无
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief 去除监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要去除的事件,如EV_READ,EV_WRITE
     * @return 无
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief 前摄器的执行函数，此函数不会退出
     * 有事件了，调用事件分发器，将事件分派给事件处理器
     * 
     * @return int
     * @retval 0 成功
     */
    virtual int Execute();
public:
    fd_set all_rset, all_wset;
    QuickMDB_SOCKET rmax_fd, wmax_fd;///< 读和写的最大fd
    int rcount, wcount;    
};

#ifndef OS_WINDOWS
#include <sys/poll.h>
/**
 * @brief poll模型
 * 
 */
class TMdbProactorPoll:public TMdbPeerProactor
{
    MDB_ZF_DECLARE_OBJECT(TMdbProactorPoll);
public:
    TMdbProactorPoll();
protected:
    /**
     * @brief 添加需要监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @return 无
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief 去除监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要去除的事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    /**
     * @brief 前摄器的执行函数，此函数不会退出
     * 有事件了，调用事件分发器，将事件分派给事件处理器
     */
    virtual int Execute();
protected:
    struct pollfd m_arrayAllFds[65536];///< poll事件集合
    int m_uiPollFdSize;///< poll事件集合大小
};
#endif

#ifdef OS_LINUX
/**
 * @brief epoll模型
 * linux系统下的可用通知型异步io
 * 
 */
class TMdbProactorEpoll:public TMdbPeerProactor
{
    MDB_ZF_DECLARE_OBJECT(TMdbProactorEpoll);
public:
    TMdbProactorEpoll();
    virtual ~TMdbProactorEpoll();
protected:
    /**
     * @brief 前摄器的执行函数，此函数不会退出
     * 有事件了，调用事件分发器，将事件分派给事件处理器
     */
    virtual int Execute();
    /**
     * @brief 添加需要监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief 去除监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要去除的事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    int m_iEpollFd;///< epoll_create结果,epoll的fd本身也占用一个文件描述符
};
#endif

#ifdef OS_IBM
#include <sys/pollset.h>
/**
 * @brief Pollset模型
 * AIX系统下的可用通知型异步io
 * 
 */
class TMdbProactorPollset:public TMdbPeerProactor
{
    MDB_ZF_DECLARE_OBJECT(TMdbProactorPollset);
public:
    TMdbProactorPollset();
    virtual ~TMdbProactorPollset();
protected:
    /**
     * @brief 添加需要监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief 去除监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要去除的事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief 前摄器的执行函数，此函数不会退出
     * 有事件了，调用事件分发器，将事件分派给事件处理器
     */
    virtual int Execute();
protected:
    pollset_t m_iPollsetFd;///< Pollset_create结果,Pollset的fd本身也占用一个文件描述符
};
#endif

#ifdef OS_WINDOWS
//区别是何种完成事件  
enum MDB_IOCP_EVENT  
{  
    MDB_IOCP_EVENT_ACCEPT,
    MDB_IOCP_EVENT_WSARECV,
    MDB_IOCP_EVENT_WSASEND 
};  

struct MDB_OVERLAPPEDPLUS:public OVERLAPPED
{  
    MDB_IOCP_EVENT io_type;//指示是何种IO操作  
    WSABUF  wsaDataBuf;
    DWORD dwFlags;
    DWORD   dwBytes;
    bool    bMonitor;///< 是否监控
    MDB_OVERLAPPEDPLUS(MDB_IOCP_EVENT io_type)
    {
        memset(this, 0x00, sizeof(MDB_OVERLAPPEDPLUS));
        this->io_type = io_type;
    }
};

/**
 * @brief iocp模型
 * windows下的数据完成型异步io
 */
class TMdbProactorIocp:public TMdbPeerProactor
{
    MDB_ZF_DECLARE_OBJECT(TMdbProactorIocp);
public:
    TMdbProactorIocp();
    virtual ~TMdbProactorIocp();
protected:
    /**
     * @brief 前摄器的执行函数，此函数不会退出
     * 有事件了，调用事件分发器，将事件分派给事件处理器
     */
    virtual int Execute();
    /**
     * @brief 添加需要监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @return 无
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief 去除监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要去除的事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    HANDLE          m_hCompletionPort;///< 完成端口的句柄
    MDB_OVERLAPPEDPLUS  m_oIntrOverlapped;///< 用于中断fd的overlapped
};
#endif

#ifdef OS_FREEBSD
/**
 * @brief kqueue模型
 * 
 */
class TMdbProactorKqueue:public TMdbPeerProactor
{
    MDB_ZF_DECLARE_OBJECT(TMdbProactorKqueue);
public:
    TMdbProactorKqueue();
    virtual ~TMdbProactorKqueue();
protected:
    /**
     * @brief 添加需要监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief 去除监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要去除的事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    /**
     * @brief 前摄器的执行函数，此函数不会退出
     * 有事件了，调用事件分发器，将事件分派给事件处理器
     */
    virtual int Execute();
protected:
    int m_kq;///< 创建的kqueue标识    
};
#endif

#ifdef OS_SUN
/**
 * @brief 适合solaris的event ports模型
 * 
 */
class TMdbProactorEventPorts:public TMdbPeerProactor
{
    MDB_ZF_DECLARE_OBJECT(TMdbProactorEventPorts);
public:
    TMdbProactorEventPorts();
    virtual ~TMdbProactorEventPorts();
protected:
    /**
     * @brief 添加需要监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要监听事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief 去除监听的事件
     * 
     * @param fd [in] socket描述符
     * @param events [in] 需要去除的事件,如EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    /**
     * @brief 前摄器的执行函数，此函数不会退出
     * 有事件了，调用事件分发器，将事件分派给事件处理器
     */
    virtual int Execute();
protected:
    int m_portfd;///< 创建的port标识    
};
#endif

//_NTC_END
//}
#endif
