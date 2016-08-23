/**
* @file NtcEngine.hxx
 * ����ͨ�����
 * 
 * @author jiang.jinzhou@zte.com.cn
 * @version 1.0
 * @date 2012/04/24
 * @bug �½�����bug
 * @bug ��Ԫ���ԣ�δ����bug
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
 * @brief �������
 * 
 */
 class TMdbNtcBaseProxy:public TMdbNtcBaseObject
{
    MDB_ZF_DECLARE_OBJECT(TMdbNtcBaseProxy);
public:
    virtual QuickMDB_SOCKET Connect(const char* pszRemote, int iPort, int iMilliSeconds = -1) const = 0;
};

/**
 * @brief �޴���
 * 
 */
class TMdbNtcNonProxy:public TMdbNtcBaseProxy
{
    MDB_ZF_DECLARE_OBJECT(TMdbNtcNonProxy);
public:
    TMdbNtcNonProxy();
    QuickMDB_SOCKET Connect(const char* pszRemote, int iPort, int iMilliSeconds = -1) const;
};

extern const TMdbNtcNonProxy g_oMdbNtcNonProxy;///< Ĭ�ϲ�ʹ�ô���

/**
 * @brief http����
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
 * @brief �¼���,��Ӧһ���߳�
 * 
 */
class TMdbPeerEventPump:public TMdbEventPump
{
    MDB_ZF_DECLARE_OBJECT(TMdbPeerEventPump);
public:
    TMdbPeerEventPump();
    MDB_UINT32 uiFdCount;///< ���¼����ϵ�fd��Ŀ
};

/**
 * @brief socket����Ļ��࣬�ṩǰ����,�¼��ַ������¼��������ķ�װ
 * ����socket�ɶ�����д�Ȳ����Ĵ���
 * 
 */
class TMdbNtcEngine:public TMdbEventDispatcher, public TMdbEventHandler
{
public:
    TMdbNtcEngine();
    virtual ~TMdbNtcEngine();
    /**
     * @brief �ж�����ͨѶ����Ƿ��Ѿ�����
     * 
     * @return bool
     * @retval true �Ѿ�������
     */
    bool IsStart();
    //����ͨѶ��ܵ�������ֹͣ
    /**
     * @brief ��ʼ��������ͨѶ
     * ��������ǰ������������¼�����Ŀ�����û���¼��ã����ʼ��һ��
     * 
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Start();
    /**
     * @brief ֹͣ����ͨѶ
     * 
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Stop();

	 /**
     * @brief ǿ��ɱ������ͨѶ
     * 
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Kill();
		
    /**
     * @brief ��ϵͳ�Զ�ѡ���ʺϵ�ǰ����
     * 
     * @return bool
     * @retval true �ɹ�
     */
    bool InitSocketProactor();
    /**
     * @brief ��ʼ��socketǰ����
     * 
     * @param emProactorType [in] ����ʹ�õ�ǰ����
     * @return bool
     * @retval true �ɹ�
     */
    bool InitSocketProactor(MDB_PEER_PROACTOR_TYPE emProactorType);
    /**
     * @brief ��������������
     * 
     * @param uiMaxPeerNo [in]����������
     * @return ��
     */
    void SetMaxPeerNo(MDB_UINT16 uiMaxPeerNo);
    /**
     * @brief �������������
     * 
     * @return MDB_UINT16
     * @retval ������
     */
    inline MDB_UINT16 GetMaxPeerNo()
    {
        return (MDB_UINT16)m_oIdMgr.GetPoolSize();
    }
    /**
     * @brief ���ӷ����
     * 
     * @param pszRemote [in] Զ�̷���˵�ip��ַ������
     * @param iPort [in] Զ�̷���˵Ķ˿�
     * @param pProtocol [in] Զ�̷���˵���ʹ�õ�Э�飬���ΪNULL����Ὣ�յ���������Э�����ֱ����������Ϣ
     * @param iMilliSeconds [in] ���ӳ�ʱʱ��(����)��������ֵ����Ϊ����ʧ��,ȡֵΪ-1��ʾ��Ĭ��ʱ����(3s)
     * @param uiPeerFlag [in] ���ӵ�flag����
     * @param pPeerRuntimeObject [in] ��������Ӧ������ʱ�ڶ������ڴ�������ʵ��
     * @param g_oMdbNtcNonProxy   [in] ��ʹ�õĴ���
     * @return TMdbSharedPtr<TMdbPeerInfo>
     * @retval NULL ��ʾδ���ӳɹ�
     */
    TMdbSharedPtr<TMdbPeerInfo> Connect(const char* pszRemote, int iPort, TMdbProtocol* pProtocol = NULL, int iMilliSeconds = -1, MDB_UINT16 uiPeerFlag = 0,
        const TMdbRuntimeObject* pPeerRuntimeObject = NULL, const TMdbNtcBaseProxy& oProxy = g_oMdbNtcNonProxy);
    /**
     * @brief ����������ĳһ�˿�
     * 
     * @param pszAddress [in] pszAddressΪNULL��ʾ����ip
     * @param iPort [in] Ҫ�󶨵Ķ˿�
     * @param pProtocol [in] ָ��ʹ�õ�Э�飬���ΪNULL����Ὣ�յ���������Э�����ֱ����������Ϣ
     * @param pPeerRuntimeObject [in] ��������Ӧ������ʱ�ڶ������ڴ�������ʵ��
     * @return bool
     * @retval true �ɹ�
     */
    bool AddListen(const char* pszAddress, int iPort, TMdbProtocol* pProtocol = NULL, const TMdbRuntimeObject* pPeerRuntimeObject = NULL);
    /**
     * @brief �Ͽ�ָ��������
     * 
     * @param pno [in] ���Ӻ�
     * @return bool
     * @retval true �ɹ�
     */
    bool Disconnect(MDB_UINT16 pno);
public://������pno��Ϣά���й�
    /**
     * @brief ��õ�ǰ��������
     * 
     * @return MDB_UINT16
     * @retval ������
     */
    inline MDB_UINT16 GetPeerCount()
    {
        return (MDB_UINT16)m_oIdMgr.GetUsedSize();
    }
    /**
     * @brief ���������Ϣ
     * 
     * @param new_fd [in] �µ���������Ϣ
     * @param pServerInfo [in] server��Ϣ
     * @param uiPeerFlag [in] �����ϵ�flag����
     * @return TMdbSharedPtr<TMdbPeerInfo>
     * @retval socket������Ϣ
     */
    TMdbSharedPtr<TMdbPeerInfo> AddPeerInfo(QuickMDB_SOCKET new_fd, TMdbServerInfo* pServerInfo, MDB_UINT16 uiPeerFlag = 0);
    /**
     * @brief ɾ��������Ϣ
     * 
     * @param pno [in] ���Ӻ�
     * @return ��
     */
    inline void DelPeerInfo(MDB_UINT16 pno)
    {
        if(m_ppPeerNo && m_ppPeerNo[pno])
        {
            DelPeerInfo(m_ppPeerNo[pno]);
        }
    }
    /**
     * @brief ɾ��������Ϣ
     * 
     * @param pPeerInfo [in] ������Ϣ
     * @return ��
     */
    void DelPeerInfo(TMdbPeerInfo* pPeerInfo);
        /**
     * @brief �������ӺŲ���������Ϣ
     * 
     * @param pno [in] ���Ӻ�
     * @return TMdbPeerInfo*
     */
    inline TMdbSharedPtr<TMdbPeerInfo>& FindPeerInfo(MDB_UINT16 pno)
    {
        return (m_ppPeerNo&&pno<m_oIdMgr.GetPoolCapacity())?m_ppPeerNo[pno]:ms_pNullPeerInfo;
    }
    /**
     * @brief ����socket��Ϣ
     * 
     * @param fd [in] socket������
     * @return TMdbNtcSocket*
     */
    inline TMdbNtcSocket* FindSocketInfo(QuickMDB_SOCKET fd)
    {
        return m_arraySocketInfo[fd];
    }
    /**
     * @brief ���Server��Ϣ
     * 
     * @param pszIP [in] server��ip
     * @param iPort [in] server��port
     * @return TMdbServerInfo*
     */
    TMdbServerInfo* FindServerInfo(const char* pszIP, int iPort);
public://�������麯������
    /**
     * @brief �ж������Ƿ�������루�����¿ͻ�������ʱ����OnConnect�¼�֮ǰ��
     * 
     * @param new_fd [in] socket������
     * @param pServerInfo [in] �������Ϣ
     * @return bool
     * @retval true �������
     * @retval false ���������
     */
    virtual bool CheckNewClient(QuickMDB_SOCKET new_fd, TMdbServerInfo* pServerInfo)
    {
        return true;
    }
    /**
     * @brief ����һ��socket������Ϣ
     * 
     * @return TMdbPeerInfo*
     * @retval socket������Ϣ
     */
    virtual TMdbPeerInfo* CreatePeerInfo();
    /*
     * @brief ����һ���¼���
     * 
     * @return TMdbEventPump*
     * @retval �������¼���
     */
    virtual TMdbEventPump* CreateEventPump();
    /**
     * @brief �����¼�
     * 
     * @param pEventInfo [in] �¼�
     * @param pEventPump [in] �¼���
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool ProcessEvent(TMdbEventInfo* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief �ַ��¼�
     * 
     * @param pEventInfo [in] �¼�
     * @return TMdbEventPump*
     * @retval ʹ�õ��¼���
     */
    virtual TMdbEventPump* Dispatch(TMdbEventInfo* pEventInfo);
protected://�������¼�������
    /**
     * @brief Ԥ�ַ���������Ϊ�˻���ʺϵ��¼��ã������ַ�
     * 
     * @param pEventInfo [in] �¼�     
     * @return TMdbEventPump*
     * @retval �ʺϵ��¼���
     */
    virtual TMdbEventPump* PreDispatch(TMdbEventInfo* pEventInfo);
    /**
     * @brief ����socket���ӻ���socket���ӳɹ�ʱ�������˺���
     * 
     * @param pEventInfo [in] �¼���Ϣ
     * @param pEventPump [in] �¼���
     * @return bool
     * @retval true ��ʼ�������¼�������
     * @retval false ���������¼��������ֶ�����
     */
    virtual bool OnConnect(TMdbConnectEvent* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief �������ӶϿ�ʱ�������˺���
     * 
     * @param pEventInfo [in] �¼���Ϣ
     * @param pEventPump [in] �¼���
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief ���ͻ�������Ϣ����ʱ�򣬴����˺���
     * 
     * @param pEventInfo [in] �¼���Ϣ
     * @param pEventPump [in] �¼���
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief �����г�ʱʱ�������˺���
     * 
     * @param pEventInfo [in] �¼���Ϣ
     * @param pEventPump [in] �¼���
     * @return bool
     * @retval true �������ó�ʱ
     * @retval false ȡ����ʱ
     */
    virtual bool OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief ����������ʱ�������˺������������Ϣ�������
     * 
     * @param pEventInfo [in] �¼���Ϣ
     * @param pEventPump [in] �¼���
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool OnError(TMdbErrorEvent* pEventInfo, TMdbEventPump* pEventPump);
    /**
     * @brief ��չ���¼���Ĭ�ϵ��ô˺���
     * 
     * @param pEventInfo [in] �¼���Ϣ
     * @param pEventPump [in] �¼���
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool OnOtherEvent(TMdbEventInfo* pEventInfo, TMdbEventPump* pEventPump)
    {
        return true;
    }
public://�������������
    /**
     * @brief ���������Ϣ
     * 
     * @return TMdbTrafficCtrl&
     * @retval ������Ϣ
     */
    TMdbTrafficInfo* GetTotalTrafficInfo()
    {
        return &m_oTotalTrafficInfo;
    }
    /**
     * @brief �����ܵ����������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiTotalMaxRecvRate [in] �ܵ��������
     * @return ��
     */
    inline void SetTotalMaxRecvRate(MDB_UINT32 uiTotalMaxRecvRate)
    {
        m_oTotalTrafficCtrl.SetMaxRecvRate(uiTotalMaxRecvRate);
    }
    /**
     * @brief �����ܵ���������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiTotalMaxSendRate [in] �ܵ��������
     * @return ��
     */
    inline void SetTotalMaxSendRate(MDB_UINT32 uiTotalMaxSendRate)
    {
        m_oTotalTrafficCtrl.SetMaxSendRate(uiTotalMaxSendRate);
    }
    /**
     * @brief �����ܵ������պͷ��������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiTotalMaxFlowRate [in] �ܵ��������
     * @return ��
     */
    inline void SetTotalMaxFlowRate(MDB_UINT32 uiTotalMaxFlowRate)
    {
        m_oTotalTrafficCtrl.SetMaxFlowRate(uiTotalMaxFlowRate);
    }
    /**
     * @brief ����ܵ����������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetTotalMaxRecvRate()
    {
        return m_oTotalTrafficCtrl.GetMaxRecvRate();
    }
    /**
     * @brief ����ܵ���������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetTotalMaxSendRate()
    {
        return m_oTotalTrafficCtrl.GetMaxSendRate();
    }
    /**
     * @brief ����ܵ������պͷ��������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetTotalMaxFlowRate()
    {
        return m_oTotalTrafficCtrl.GetMaxFlowRate();
    }
    /**
     * @brief ���õ��������ϵ����������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiTotalMaxRecvRate [in] �ܵ��������
     * @return ��
     */
    inline void SetPeerMaxRecvRate(MDB_UINT32 uiPeerMaxRecvRate)
    {
        m_oPeerTrafficCtrl.SetMaxRecvRate(uiPeerMaxRecvRate);
    }
    /**
     * @brief ���õ��������ϵ���������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiPeerMaxSendRate [in] �ܵ��������
     * @return ��
     */
    inline void SetPeerMaxSendRate(MDB_UINT32 uiPeerMaxSendRate)
    {
        m_oPeerTrafficCtrl.SetMaxSendRate(uiPeerMaxSendRate);
    }
    /**
     * @brief ���õ��������ϵ������պͷ��������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiPeerMaxFlowRate [in] �ܵ��������
     * @return ��
     */
    inline void SetPeerMaxFlowRate(MDB_UINT32 uiPeerMaxFlowRate)
    {
        m_oPeerTrafficCtrl.SetMaxFlowRate(uiPeerMaxFlowRate);
    }
    /**
     * @brief ��õ��������ϵ����������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetPeerMaxRecvRate()
    {
        return m_oPeerTrafficCtrl.GetMaxRecvRate();
    }
    /**
     * @brief ��õ��������ϵ���������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetPeerMaxSendRate()
    {
        return m_oPeerTrafficCtrl.GetMaxSendRate();
    }
    /**
     * @brief ��õ��������ϵ������պͷ��������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetPeerMaxFlowRate()
    {
        return m_oPeerTrafficCtrl.GetMaxFlowRate();
    }
    /**
     * @brief ������ӵ�ǰ���Է��͵��ֽ���
     * 
     * @param pPeerInfo [in] ������Ϣ
     * @param uiRecvBytes [in] ��Ҫ���յ��ֽ���
     * @return MDB_UINT32
     * @retval ���Խ��յ��ֽ���
     */
    MDB_UINT32 GetPeerAllowRecvBytes(TMdbPeerInfo* pPeerInfo, MDB_UINT32 uiRecvBytes);
    /**
     * @brief ������ӵ�ǰ���Է��͵��ֽ���
     * 
     * @param pPeerInfo [in] ������Ϣ
     * @param uiSendBytes [in] ��Ҫ���͵��ֽ���
     * @return MDB_UINT32
     * @retval ���Խ��յ��ֽ���
     */
    MDB_UINT32 GetPeerAllowSendBytes(TMdbPeerInfo* pPeerInfo, MDB_UINT32 uiSendBytes);
    /**
     * @brief �ۼӽ����ֽ���
     * 
     * @param uiRecvBytes [in] �½��յ��ֽ���
     * @return ��
     */
    inline void AddTotalRecvBytes(MDB_UINT32 uiRecvBytes)
    {
        m_oSpinLock.Lock();
        m_oTotalTrafficInfo.AddRecvBytes(uiRecvBytes);
        m_oSpinLock.Unlock();
    }
    /**
     * @brief �ۼӽӷ��ͽ���
     * 
     * @param uiRecvBytes [in] �·��͵��ֽ���
     * @return ��
     */
    inline void AddTotalSendBytes(MDB_UINT32 uiSendBytes)
    {
        m_oSpinLock.Lock();
        m_oTotalTrafficInfo.AddSendBytes(uiSendBytes);
        m_oSpinLock.Unlock();
    }
protected:
    TMdbIdMgr                          m_oIdMgr;///< ID������
    TMdbNtcBaseList   m_lsServerInfo;///< �������Ϣ
    TMdbSharedPtr<TMdbPeerInfo>*          m_ppPeerNo;        ///< pno��Ӧpeer��Ϣ
    TMdbNtcSocket*    m_arraySocketInfo[65536];
    TMdbNtcThreadLock   m_oSpinLock;///< �����������ڶ�ʱ��Ļ���
    TMdbPeerProactor*                  m_pSocketProactor;///< ���õ�socketǰ����
    
    TMdbTrafficCtrl                    m_oTotalTrafficCtrl;///< �ܵ�������������
    TMdbTrafficCtrl                    m_oPeerTrafficCtrl;///< ÿ�����ӵ�ͳһ���������ƣ����ĳ���������˵������ƣ������ο�������
    TMdbTrafficInfo                    m_oTotalTrafficInfo;///< �ܵ���·������Ϣ
private:
    static TMdbSharedPtr<TMdbPeerInfo>    ms_pNullPeerInfo;///< ���ڷ�������
};

/**
 * @brief �첽ģʽ
 * 
 */
typedef TMdbNtcEngine TMdbAsyncEngine;

/**
 * @brief ͬ��ģʽ
 * 
 */
class TMdbSyncEngine:public TMdbNtcEngine
{
public:
    TMdbSyncEngine();
    virtual ~TMdbSyncEngine();
    /**
     * @brief ��ȡһ����������Ϣ�¼�(����)
     * 
     * @param iMilliSeconds [in] ��ʱ���ã���λΪ���룬-1��ʾ����ʱ
     * @return TMdbRecvMsgEvent*
     * @retval ��NULL �ɹ�
     */
    inline TMdbRecvMsgEvent* GetMessage(int iMilliSeconds = -1)
    {
        return static_cast<TMdbRecvMsgEvent*>(m_oMsgQueue.Pop(iMilliSeconds));
    }
protected:
    /**
     * @brief ���ͻ�������Ϣ����ʱ�򣬴����˺���������ͬ��ģʽ���Ὣ��Ϣѹ�뵽��Ϣ����
     * 
     * @param pEventInfo [in] �¼���Ϣ
     * @param pEventPump [in] �¼���
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool OnRecvMsg(TMdbRecvMsgEvent* pEventInfo, TMdbEventPump* pEventPump);
    virtual bool ProcessEvent(TMdbEventInfo* pEventInfo, TMdbEventPump* pEventPump);
protected:
    //�����ж���¼�������Ϣ������ѹ�룬������Ҫʹ��β������ֻ��һ���߳�GetMessage����ͷ���������
    TMdbNtcQueue m_oMsgQueue;///< ����ʽ����Ϣ����
};
//}
#endif
