/**
 * @file PeerProactor.hxx
 * ���ӵ�ǰ��������
 * 
 * @author w
 * @version 1.0
 * @date 2012/10/06
 * @bug �½�����bug
 * @bug ��Ԫ���ԣ�δ����bug
 * @warning 
 */
#ifndef _MDB_PEER_PROACTOR_H_
#define _MDB_PEER_PROACTOR_H_
#include "Common/mdbNTC.h"
//namespace QuickMDB
//{
/**
 * @brief �����ܵ�
 * 
 * @param type [in] SOCK_STREAM��SOCK_DGRAM
 * @param protocol [in] Э���
 * @param sock [out] �����ɹ��󷵻صĶԵ������׽���
 * @return bool
 * @retval true �ɹ�
 */
int mdb_ntc_socket_pair(int type, int protocol, unsigned int sock[2]);
int mdb_ntc_get_need_read_size(unsigned int fd);

class TMdbEventDispatcher;
/**
 * @brief ǰ����
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
     * @brief �����¼��ַ���
     * 
     * @param pEventDispatcher [in] �¼��ַ���
     */
    inline void SetEventDispatcher(TMdbEventDispatcher* pEventDispatcher)
    {
        m_pEventDispatcher = pEventDispatcher;
    }
    /**
     * @brief ����¼��ַ���
     * 
     * @param pEventDispatcher [in] �¼��ַ���
     */
    inline TMdbEventDispatcher* GetEventDispatcher()
    {
        return m_pEventDispatcher;
    }
    /**
     * @brief ǰ��������
     * 
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Start() = 0;
    /**
     * @brief ǰ����ֹͣ
     * 
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Stop() = 0;
protected:
    TMdbEventDispatcher* m_pEventDispatcher;///< ָ�����¼��ַ���
};

/**
 * @brief �ź�ǰ����
 * �ܹ�Ϊ�����ṩ�źŵ���ȡ�ʹ���ӿ�
 * 
 */
 class TMdbSignalProactor:public TMdbProactor
 {
 public:
     /**
     * @brief ǰ��������
     * 
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Start();
    /**
     * @brief ǰ����ֹͣ
     * 
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Stop();
 };

/**
 * @brief ������ʹ�õ�ǰ��������
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
 * @brief socketǰ����
 * ��ģʽ��io������ɺ����û�֪ͨ
 * ǰ���������̣߳�Ҳ�����ɵ����߳���ȡio�¼�
 * 
 */
class TMdbPeerProactor:public TMdbProactor, public TMdbNtcThread
{
    friend class TMdbServerInfo;
    friend class TMdbPeerInfo;
    MDB_ZF_DECLARE_OBJECT(TMdbPeerProactor);
protected:
    /**
     * @brief �첽�����¼������ı��
     * 
     */
    struct ASYNC_MODIFY_MONITOR
    {
        bool    bAddMonitor;///< �Ƿ�������¼�
        MDB_UINT16 ntc_events;     ///< Ҫ��ӻ��Ƴ��������¼�
        QuickMDB_SOCKET  fd;         ///< ������
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
     * @brief ��ʼ��ǰ����
     * 
     * @param pAcpfEngine [in] peer���ӹ�����
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Init(TMdbNtcEngine* pAcpfEngine);
    /**
     * @brief ���PeerMgr
     * 
     * @return TPeerManager*
     * @retval peer���ӹ�����
     */
    inline TMdbNtcEngine* GetPeerHelper()
    {
        return m_pNtcEngine;
    }
    /**
     * @brief ǰ��������
     * 
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Start();
    /**
     * @brief ǰ����ֹͣ
     * 
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Stop();
    /**
     * @brief ����ǰ����
     *      
     * @return ��
     */
    virtual void Wakeup();
    /**
     * @brief ����ǰ����
     * 
     * @return ��
     */
    virtual void Cleanup();
    /**
     * @brief ����server��accept����
     * 
     * @param pServerInfo [in] server��Ϣ
     * @return bool
     * @retval true �ɹ�����������accept
     * @retval false ʧ�ܣ��������accept
     */
    bool OnServerAccept(TMdbServerInfo* pServerInfo);
    /**
     * @brief ��ȡpeer�ϵ�����
     * 
     * @param pPeerInfo [in] ������Ϣ
     * @return bool
     * @retval true ���������ɶ�
     * @retval false ֹͣ�����ɶ�
     */
    bool OnPeerRecv(TMdbPeerInfo* pPeerInfo);
    /**
     * @brief ����peer�ϵ�����
     * 
     * @param pPeerInfo [in] ������Ϣ
     * @return bool
     * @retval true ����������д
     * @retval false ֹͣ������д
     */
    bool OnPeerSend(TMdbPeerInfo* pPeerInfo);
protected:
    /**
     * @brief �����첽����
     * 
     */
    void DealAsyncOperator();
    /**
     * @brief ����µĿͻ�������
     * 
     * @param new_fd [in] �ͻ���fd
     * @param pServerInfo [in] server��Ϣ
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool AddNewClient(QuickMDB_SOCKET new_fd, TMdbServerInfo* pServerInfo);
    /**
     * @brief �����Ҫ�������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return ��
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events) = 0;
    /**
     * @brief ȥ���������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫȥ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events) = 0;
    /**
     * @brief �첽�����Ҫ�������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return ��
     */
    bool AsyncAddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief �첽ȥ���������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫȥ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    bool AsyncRemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    QuickMDB_SOCKET intr_fd[2];///< socket pair�����ж�select��ѭ��
    TMdbNtcThreadLock spinlock;
    TMdbNtcEngine* m_pNtcEngine;
};

/**
 * @brief selectģ��
 * 
 */
class TMdbProactorSelect:public TMdbPeerProactor
{
    MDB_ZF_DECLARE_OBJECT(TMdbProactorSelect);
public:
    TMdbProactorSelect();
    /**
     * @brief ����ǰ����
     * 
     * @return ��
     */
    virtual void Cleanup();
protected:
    /**
     * @brief �����Ҫ�������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return ��
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief ȥ���������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫȥ�����¼�,��EV_READ,EV_WRITE
     * @return ��
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief ǰ������ִ�к������˺��������˳�
     * ���¼��ˣ������¼��ַ��������¼����ɸ��¼�������
     * 
     * @return int
     * @retval 0 �ɹ�
     */
    virtual int Execute();
public:
    fd_set all_rset, all_wset;
    QuickMDB_SOCKET rmax_fd, wmax_fd;///< ����д�����fd
    int rcount, wcount;    
};

#ifndef OS_WINDOWS
#include <sys/poll.h>
/**
 * @brief pollģ��
 * 
 */
class TMdbProactorPoll:public TMdbPeerProactor
{
    MDB_ZF_DECLARE_OBJECT(TMdbProactorPoll);
public:
    TMdbProactorPoll();
protected:
    /**
     * @brief �����Ҫ�������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return ��
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief ȥ���������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫȥ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    /**
     * @brief ǰ������ִ�к������˺��������˳�
     * ���¼��ˣ������¼��ַ��������¼����ɸ��¼�������
     */
    virtual int Execute();
protected:
    struct pollfd m_arrayAllFds[65536];///< poll�¼�����
    int m_uiPollFdSize;///< poll�¼����ϴ�С
};
#endif

#ifdef OS_LINUX
/**
 * @brief epollģ��
 * linuxϵͳ�µĿ���֪ͨ���첽io
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
     * @brief ǰ������ִ�к������˺��������˳�
     * ���¼��ˣ������¼��ַ��������¼����ɸ��¼�������
     */
    virtual int Execute();
    /**
     * @brief �����Ҫ�������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief ȥ���������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫȥ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    int m_iEpollFd;///< epoll_create���,epoll��fd����Ҳռ��һ���ļ�������
};
#endif

#ifdef OS_IBM
#include <sys/pollset.h>
/**
 * @brief Pollsetģ��
 * AIXϵͳ�µĿ���֪ͨ���첽io
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
     * @brief �����Ҫ�������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief ȥ���������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫȥ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief ǰ������ִ�к������˺��������˳�
     * ���¼��ˣ������¼��ַ��������¼����ɸ��¼�������
     */
    virtual int Execute();
protected:
    pollset_t m_iPollsetFd;///< Pollset_create���,Pollset��fd����Ҳռ��һ���ļ�������
};
#endif

#ifdef OS_WINDOWS
//�����Ǻ�������¼�  
enum MDB_IOCP_EVENT  
{  
    MDB_IOCP_EVENT_ACCEPT,
    MDB_IOCP_EVENT_WSARECV,
    MDB_IOCP_EVENT_WSASEND 
};  

struct MDB_OVERLAPPEDPLUS:public OVERLAPPED
{  
    MDB_IOCP_EVENT io_type;//ָʾ�Ǻ���IO����  
    WSABUF  wsaDataBuf;
    DWORD dwFlags;
    DWORD   dwBytes;
    bool    bMonitor;///< �Ƿ���
    MDB_OVERLAPPEDPLUS(MDB_IOCP_EVENT io_type)
    {
        memset(this, 0x00, sizeof(MDB_OVERLAPPEDPLUS));
        this->io_type = io_type;
    }
};

/**
 * @brief iocpģ��
 * windows�µ�����������첽io
 */
class TMdbProactorIocp:public TMdbPeerProactor
{
    MDB_ZF_DECLARE_OBJECT(TMdbProactorIocp);
public:
    TMdbProactorIocp();
    virtual ~TMdbProactorIocp();
protected:
    /**
     * @brief ǰ������ִ�к������˺��������˳�
     * ���¼��ˣ������¼��ַ��������¼����ɸ��¼�������
     */
    virtual int Execute();
    /**
     * @brief �����Ҫ�������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return ��
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief ȥ���������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫȥ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    HANDLE          m_hCompletionPort;///< ��ɶ˿ڵľ��
    MDB_OVERLAPPEDPLUS  m_oIntrOverlapped;///< �����ж�fd��overlapped
};
#endif

#ifdef OS_FREEBSD
/**
 * @brief kqueueģ��
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
     * @brief �����Ҫ�������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief ȥ���������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫȥ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    /**
     * @brief ǰ������ִ�к������˺��������˳�
     * ���¼��ˣ������¼��ַ��������¼����ɸ��¼�������
     */
    virtual int Execute();
protected:
    int m_kq;///< ������kqueue��ʶ    
};
#endif

#ifdef OS_SUN
/**
 * @brief �ʺ�solaris��event portsģ��
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
     * @brief �����Ҫ�������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool AddEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
    /**
     * @brief ȥ���������¼�
     * 
     * @param fd [in] socket������
     * @param events [in] ��Ҫȥ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     */
    virtual bool RemoveEventMonitor(QuickMDB_SOCKET fd, MDB_UINT16 events);
protected:
    /**
     * @brief ǰ������ִ�к������˺��������˳�
     * ���¼��ˣ������¼��ַ��������¼����ɸ��¼�������
     */
    virtual int Execute();
protected:
    int m_portfd;///< ������port��ʶ    
};
#endif

//_NTC_END
//}
#endif
