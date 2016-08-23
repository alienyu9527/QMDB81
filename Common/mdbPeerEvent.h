/**
 * @file mdbPeerEvent.h
 * ��������ص��¼���Ϣ
 * 
 * @author jiang.jinzhou
 * @version 1.0
 * @date 2013/09/05
 * @bug �½�����bug
 * @bug ��Ԫ���ԣ�δ����bug
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
 * @brief �¼���,��Ӧһ���߳�
 * 
 */
class TMdbEventPump:public TMdbNtcThread
{
    MDB_ZF_DECLARE_OBJECT(TMdbEventPump);
public:
    TMdbEventPump();
    virtual ~TMdbEventPump();
    /**
     * @brief ѹ��һ���������¼�
     * 
     * @param pEventInfo [in] �¼�
     * @return ��     
     */
    virtual void PushEvent(TMdbEventInfo* pEventInfo);
    /**
     * @brief ��ӡ�¼��õ�״̬
     * 
     */
    virtual TMdbNtcStringBuffer ToString( ) const;
    /**
     * @brief ��õ�ǰ���еĴ�С
     * 
     * @return MDB_UINT32
     * @retval ��ǰ���еĴ�С
     */
    inline MDB_UINT32 GetQueueSize()
    {
        return m_queueEvent.GetSize();
    }
protected:
    /**
     * @brief ǰ������ִ�к������˺��������˳�
     * ���¼��ˣ������¼��ַ��������¼����ɸ��¼�������
     */
    virtual int Execute();
protected:
    TMdbNtcQueue  m_queueEvent;///< ����ʽ���¼�����
};

/**
 * @brief �¼��ַ���
 * 
 */
class TMdbEventDispatcher
{
public:
    TMdbEventDispatcher();
    virtual ~TMdbEventDispatcher();
    /**
     * @brief ����һ���¼���
     * 
     * @return TMdbEventPump*
     * @retval �������¼���
     */
    virtual TMdbEventPump* CreateEventPump();
    /**
     * @brief ����¼���
     * 
     * @param pEventPump [in] �¼���
     */
    void AddEventPump(TMdbEventPump* pEventPump);
    /**
     * @brief ����¼��õĸ���
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetPumpCount()
    {
        return m_arrayPump.GetSize();
    }
    /**
     * @brief ��ʼ���¼��õ���Ŀ
     * 
     * @param iPumpNum [in] �¼��õ���Ŀ
     */
    void InitEventPump(int iPumpNum);    
    /**
     * @brief �ַ��¼�
     * 
     * @param pEventInfo [in] �¼�
     * @return TMdbEventPump*
     * @retval ʹ�õ��¼���
     */
    virtual TMdbEventPump* Dispatch(TMdbEventInfo* pEventInfo);
    /**
     * @brief ��ø�����С���¼���
     * 
     * @return TMdbEventPump*
     * @retval ������С���¼���
     */
    TMdbEventPump* GetLeastEventPump();
    /**
     * @brief ��ӡ�¼��õ�״̬
     * 
     * @return TMdbNtcStringBuffer
     */
    TMdbNtcStringBuffer PrintAllPump();
protected:
    /**
     * @brief Ԥ�ַ���������Ϊ�˻���ʺϵ��¼��ã������ַ�
     * 
     * @param pEventInfo [in] �¼�     
     * @return TMdbEventPump*
     * @retval �ʺϵ��¼���
     */
    virtual TMdbEventPump* PreDispatch(TMdbEventInfo* pEventInfo)
    {
        return GetLeastEventPump();
    }
protected:
    TMdbNtcAutoArray m_arrayPump;///< ��Ϣ������    
};

/**
 * @brief �¼�������
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
     * @brief �����¼�
     * 
     * @param pEventInfo [in] �¼�
     * @param pEventPump [in] �¼���
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool ProcessEvent(TMdbEventInfo* pEventInfo, TMdbEventPump* pEventPump) = 0;
};

class TMdbProactor;

/**
 * @brief �¼���Ϣ
 * 
 */
class TMdbEventInfo:public TMdbNtcBaseObject
{
public:
    MDB_ZF_DECLARE_OBJECT(TMdbEventInfo);
    MDB_UINT32 uiRefCnt;///< ���ü���
    TMdbProactor* pProactor;///< �������¼���ǰ����    
    TMdbEventHandler* pEventHandler;///< ���¼��Ĵ������������¼���·�ɴ��¼�����Ӧ�Ĵ�����
    TMdbEventInfo():uiRefCnt(1),pProactor(NULL),pEventHandler(NULL)
    {
    }
    inline void AddRef()
    {
        ++uiRefCnt;
    }
    /**
     * @brief �ͷ�event
     * 
     * @param iRetCode [in] ������
     * @return ��
     */
    void Release(int iRetCode = 0);
    MDB_UINT32 RefCnt()
    {
        return uiRefCnt;
    }
};

class TMdbPeerInfo;
/**
 * @brief �����ϵ��¼���Ϣ
 * 
 */
class TMdbPeerEvent:public TMdbEventInfo
{
    MDB_ZF_DECLARE_OBJECT(TMdbPeerEvent);
public:
    TMdbPeerInfo* pPeerInfo;///< ������Ϣ
    /**
     * @brief ���캯��
     * 
     * @param pno [in] ���Ӻ�
     * @param iEventType [in] �¼�����
     */
    TMdbPeerEvent(TMdbPeerInfo* pPeerInfoParam):pPeerInfo(pPeerInfoParam)
    {
    }
};

/**
 * @brief ���ӳɹ��¼�
 * 
 */
class TMdbConnectEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbConnectEvent);
public:
    /**
     * @brief ���캯��
     * 
     * @param pSocketInfo [in] ���ӳɹ���socket��Ϣ
     */
    TMdbConnectEvent(TMdbPeerInfo* pPeerInfo):TMdbPeerEvent(pPeerInfo)
    {
    }
    /**
     * @brief ���ӽ�����ʱ��
     * 
     * @return time_t
     * @retval ���ӽ�����ʱ��
     */
    inline time_t GetConnectTime()
    {
        return pPeerInfo->GetConnectTime();
    }
};

/**
 * @brief �Ͽ��¼�
 * 
 */
class TMdbDisconnectEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbDisconnectEvent);
public:
    /**
     * @brief ���캯��
     * 
     * @param bPasvClose [in] �Ƿ�Ϊ�����ر�
     * @param pPeerInfo [in] ������Ϣ
     */
    TMdbDisconnectEvent(TMdbPeerInfo* pPeerInfo, time_t tDisconnectTime = time(NULL))
        :TMdbPeerEvent(pPeerInfo),m_tDisconnectTime(tDisconnectTime)
    {
    }
    /**
     * @brief �Ƿ񱻶��ر�
     * 
     * @return bool
     * @retval true  �����ر�
     * @retval false �����ر�
     */
    inline bool IsPasvClose()
    {
        return pPeerInfo->IsPasvClose();
    }
    /**
     * @brief ���ӶϿ���ԭ��
     * 
     * @return TMdbNtcStringBuffer
     * @retval �Ͽ���ԭ��
     */
    inline TMdbNtcStringBuffer GetDisconnectReason()
    {
        return pPeerInfo->sDisconnectReason;
    }
    /**
     * @brief ���ӶϿ���ʱ��
     * 
     * @return time_t
     */
    inline time_t GetDisconnectTime()
    {
        return m_tDisconnectTime;
    }
protected:
    time_t  m_tDisconnectTime;///< ��·�Ͽ���ʱ��
};

/**
 * @brief ���г�ʱ�¼�
 * 
 */
class TMdbTimeoutEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbTimeoutEvent);
public:
    MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutType;
    /**
     * @brief ���캯��
     * 
     * @param pSocketInfo [in] ���ӳɹ���socket��Ϣ
     * @param eIdleTimeoutType [in] ��������
     */
    TMdbTimeoutEvent(TMdbPeerInfo* pPeerInfo, MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutTypeParam)
        :TMdbPeerEvent(pPeerInfo),eTimeoutType(eTimeoutTypeParam)
    {
    }
};

/**
 * @brief �����¼�
 * 
 */
class TMdbDataEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbDataEvent);
public:
    TMdbPacketInfo* pPacketInfo;///< ���ݰ���Ϣ
    /**
     * @brief ���캯��
     * 
     * @param pSocketInfo [in] socket��Ϣ
     * @param pMsgInfo [in] ��Ϣ����Ϣ
     */
    TMdbDataEvent(TMdbPeerInfo* pPeerInfo, TMdbPacketInfo* pPacketInfoParam)
        :TMdbPeerEvent(pPeerInfo),pPacketInfo(pPacketInfoParam)
    {
    }
};

/**
 * @brief ��Ϣ�¼�
 * 
 */
class TMdbRecvMsgEvent:public TMdbPeerEvent
{
    MDB_ZF_DECLARE_OBJECT(TMdbRecvMsgEvent);
public:
    TMdbMsgInfo* pMsgInfo;///< ��Ϣ����Ϣ
    /**
     * @brief ���캯��
     * 
     * @param pSocketInfo [in] socket��Ϣ
     * @param pMsgInfo [in] ��Ϣ����Ϣ
     */
    TMdbRecvMsgEvent(TMdbPeerInfo* pPeerInfo, TMdbMsgInfo* pMsgInfoParam)
        :TMdbPeerEvent(pPeerInfo),pMsgInfo(pMsgInfoParam)
    {
    }
    /**
     * @brief ��������������ʱ�����Զ�����pMsgInfo->Release()
     * 
     */
    virtual ~TMdbRecvMsgEvent();
};

/**
 * @brief �����¼�
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
