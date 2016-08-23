/**
 * @file mdbPeerInfo.h
 * ������Ϣ
 * 
 * @author jiang.jinzhou
 * @version 1.0
 * @date 2013/09/05
 * @bug �½�����bug
 * @bug ��Ԫ���ԣ�δ����bug
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
 * @brief ����������
 * 
 */
class TMdbTrafficCtrl
{
public:
    TMdbTrafficCtrl();
    /**
     * @brief �������������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiMaxRecvRate [in] �ܵ��������
     * @return ��
     */
    inline void SetMaxRecvRate(MDB_UINT32 uiMaxRecvRate)
    {
        this->m_uiMaxRecvRate = uiMaxRecvRate;
    }
    /**
     * @brief ������������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiMaxSendRate [in] �ܵ��������
     * @return ��
     */
    inline void SetMaxSendRate(MDB_UINT32 uiMaxSendRate)
    {
        this->m_uiMaxSendRate = uiMaxSendRate;
    }
    /**
     * @brief ���������պͷ��������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiMaxFlowRate [in] �ܵ��������
     * @return ��
     */
    inline void SetMaxFlowRate(MDB_UINT32 uiMaxFlowRate)
    {
        this->m_uiMaxFlowRate = uiMaxFlowRate;
    }
    /**
     * @brief ������������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetMaxRecvRate()
    {
        return m_uiMaxRecvRate;
    }
    /**
     * @brief �����������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetMaxSendRate()
    {
        return m_uiMaxSendRate;
    }
    /**
     * @brief ��������պͷ��������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetMaxFlowRate()
    {
        return m_uiMaxFlowRate;
    }
    /**
     * @brief ������ӵ�ǰ���Խ��յ��ֽ���
     * 
     * @param pPeerInfo [in] ������Ϣ
     * @param pTrafficInfo [in] ������Ϣ
     * @param uiRecvBytes [in] ��Ҫ���յ��ֽ���
     * @return MDB_UINT32
     * @retval ���Խ��յ��ֽ���
     */
    MDB_UINT32 GetAllowRecvBytes(TMdbTrafficInfo* pTrafficInfo, MDB_UINT32 uiRecvBytes);
    /**
     * @brief ������ӵ�ǰ���Է��͵��ֽ���
     * 
     * @param pPeerInfo [in] ������Ϣ
     * @param pTrafficInfo [in] ������Ϣ
     * @param uiSendBytes [in] ��Ҫ���͵��ֽ���
     * @return MDB_UINT32
     * @retval ���Խ��յ��ֽ���
     */
    MDB_UINT32 GetAllowSendBytes(TMdbTrafficInfo* pTrafficInfo, MDB_UINT32 uiSendBytes);
    /**
     * @brief �������ӵ�����
     * 
     * @param pPeerInfo [in] ������Ϣ������ʹ��SharedPtr����ͨ��FindPeerInfo��õĽ����������ȫ
     * @param events [in] �����У�PEER_EV_READ_FLAG��PEER_EV_WRITE_FLAG�����
     * @return ��
     */
    static void SuspendPeerTraffic(TMdbSharedPtr<TMdbPeerInfo>& pPeerInfo, MDB_UINT16 events);
protected:
    /**
     * @brief �����������Ƶ�����ʱ��
     * 
     */
    static void ResumePeerTraffic();
protected:
    MDB_UINT32  m_uiMaxRecvRate;      ///< ���Ľ������ʣ���λΪbytes/s��=0��ʾ������
    MDB_UINT32  m_uiMaxSendRate;      ///< ���ķ������ʣ���λΪbytes/s��=0��ʾ������
    MDB_UINT32  m_uiMaxFlowRate;      ///< �����������ʣ��������պͷ��ͣ���λΪbytes/s��=0��ʾ������
    static  TMdbNtcQueue g_oSuspendMonitorPeerQueue;
};

/**
 * @brief ������Ϣ
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
     * @brief �����һ�ν�������ʱ��
     * 
     * @return time_t
     */
    inline time_t GetLastRecvTime() const
    {
        return m_tLastRecvTime;
    }
    /**
     * @brief �����һ�η�������ʱ��
     * 
     * @return time_t
     */
    inline time_t GetLastSendTime() const
    {
        return m_tLastSendTime;
    }
    /**
     * @brief �����һ����Ϣ��ʱ��
     * 
     * @return time_t
     */
    inline time_t GetLastRecvMsgTime() const
    {
        return m_tLastRecvMsgTime;
    }
    /**
     * @brief ��÷�����һ����Ϣ��ʱ��
     * 
     * @return time_t
     */
    inline time_t GetLastSendMsgTime() const
    {
        return m_tLastSendMsgTime;
    }
    /**
     * @brief �����һ�ν������ݵĵ��ֽ���(������ͳ��)
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
     * @brief �����һ�η������ݵĵ��ֽ���(������ͳ��)
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
     * @brief ����ܵĽ��յ��ֽ���
     * 
     * @return MDB_UINT64
     */
    inline MDB_UINT64 GetTotalRecvBytes() const
    {
        return m_uiTotalRecvBytes;
    }
    /**
     * @brief ����ܵķ��͵��ֽ���
     * 
     * @return MDB_UINT64
     */
    inline MDB_UINT64 GetTotalSendBytes() const
    {
        return m_uiTotalSendBytes;
    }
    /**
     * @brief ��õ�ǰ���ڵĽ��յ��ֽ�����Ҳ����ʵʱ����
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecRecvBytes() const
    {
        return time(NULL) == m_tLastRecvTime?m_uiSecRecvBytes:0;
    }
    /**
     * @brief ��õ�ǰ���ڵķ��͵��ֽ�����Ҳ����ʵʱ����
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecSendBytes() const
    {
        return time(NULL) == m_tLastSendTime?m_uiSecSendBytes:0;
    }
    /**
     * @brief ��õ�ǰ���ڵ��ܵ��ֽ�����Ҳ����ʵʱ����
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecTotalBytes() const
    {
        time_t ttCurSecTime = time(NULL);//��֤���պͷ��ͱȽϵ���ͬһ��ʱ���
        return (ttCurSecTime==m_tLastRecvTime?m_uiSecRecvBytes:0) + (ttCurSecTime==m_tLastSendTime?m_uiSecSendBytes:0);
    }
    /**
     * @brief �ۼӽ����ֽ���
     * 
     * @param uiRecvBytes [in] �½��յ��ֽ���
     * @return ��
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
     * @brief �ۼӽӷ����ֽ���
     * 
     * @param uiRecvBytes [in] �·��͵��ֽ���
     * @return ��
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
     * @brief �����һ�ν������ݵĵ���Ϣ��(������ͳ��)
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
     * @brief �����һ�η������ݵĵ���Ϣ��(������ͳ��)
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
     * @brief ����ܵĽ��յ���Ϣ��
     * 
     * @return MDB_UINT64
     */
    inline MDB_UINT64 GetTotalRecvMsgNum() const
    {
        return m_uiTotalRecvMsgNum;
    }
    /**
     * @brief ����ܵķ��͵���Ϣ��
     * 
     * @return MDB_UINT64
     */
    inline MDB_UINT64 GetTotalSendMsgNum() const
    {
        return m_uiTotalSendMsgNum;
    }
    /**
     * @brief ��õ�ǰ���ڵĽ��յ���Ϣ����Ҳ����ʵʱ����
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecRecvMsgNum() const
    {
        return time(NULL) == m_tLastRecvMsgTime?m_uiSecRecvMsgNum:0;
    }
    /**
     * @brief ��õ�ǰ���ڵķ��͵���Ϣ����Ҳ����ʵʱ����
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecSendMsgNum() const
    {
        return time(NULL) == m_tLastSendMsgTime?m_uiSecSendMsgNum:0;
    }
    /**
     * @brief ��õ�ǰ���ڵ��ܵ���Ϣ����Ҳ����ʵʱ����
     * 
     * @return MDB_UINT32
     */
    inline MDB_UINT32 GetCurSecTotalMsgNum() const
    {
        time_t ttCurSecTime = time(NULL);//��֤���պͷ��ͱȽϵ���ͬһ��ʱ���
        return (ttCurSecTime==m_tLastRecvMsgTime?m_uiSecRecvMsgNum:0) + (ttCurSecTime==m_tLastSendMsgTime?m_uiSecSendMsgNum:0);
    }
    /**
     * @brief �ۼӽ�����Ϣ��
     * 
     * @param uiRecvMsgNum [in] �½��յ���Ϣ��
     * @return ��
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
     * @brief �ۼӽӷ�����Ϣ��
     * 
     * @param uiRecvMsgNum [in] �·��͵���Ϣ��
     * @return ��
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
    time_t  m_tLastRecvTime;      ///< ��һ����ȡ����ʱ��
    time_t  m_tLastSendTime;      ///< ��һ�η�������ʱ��
    time_t  m_tLastRecvMsgTime;      ///< ��һ����ȡ����ʱ��
    time_t  m_tLastSendMsgTime;      ///< ��һ�η�������ʱ��
    MDB_UINT32  m_uiLastSecRecvBytes;    ///< ��һ����������ȡ���ֽ���
    MDB_UINT32  m_uiLastSecSendBytes;    ///< ��һ�������ڷ��͵��ֽ���
    MDB_UINT32  m_uiSecRecvBytes;    ///< ��ǰ����������ȡ���ֽ���
    MDB_UINT32  m_uiSecSendBytes;    ///< ��ǰ�������ڷ��͵��ֽ���
    MDB_UINT64  m_uiTotalRecvBytes;   ///< ���յ����ֽ���
    MDB_UINT64  m_uiTotalSendBytes;   ///< ���͵����ֽ���
    MDB_UINT32  m_uiLastSecRecvMsgNum;    ///< ��һ����������ȡ����Ϣ��
    MDB_UINT32  m_uiLastSecSendMsgNum;    ///< ��һ�������ڷ��͵���Ϣ��
    MDB_UINT32  m_uiSecRecvMsgNum;    ///< ��ǰ����������ȡ����Ϣ��
    MDB_UINT32  m_uiSecSendMsgNum;    ///< ��ǰ�������ڷ��͵���Ϣ��
    MDB_UINT64  m_uiTotalRecvMsgNum;   ///< ���յ�����Ϣ��
    MDB_UINT64  m_uiTotalSendMsgNum;   ///< ���͵�����Ϣ��
};

/**
 * @brief ���ݰ��࣬�����շ����ݵõ������ݰ�����һ����������һ����Ϣ��
 * 
 */
class TMdbPacketInfo:public TMdbNtcBaseObject
{
    MDB_ZF_DECLARE_OBJECT(TMdbPacketInfo);
protected:
    class TPacketData
    {
    public:    
        MDB_UINT32    uiRefcnt;///<���ø���
    };
public:
    MDB_UINT32 uiOffset;///< ���buffer��ʼ��ַ��ƫ��ֵ
    MDB_UINT32 uiLength;///< packet�ĳ���
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
    char* m_pcData;///< ���ݰ���Ϣ��buffer, bufferʵ�ʳ��ȱ�iLength��һ�ֽڴ洢'\0'
    static TMdbNtcThreadLock ms_oSpinLock;///< ���������ڶ����ü����Ļ���
};

struct TMdbSpecTermMatchInfo
{
    TMdbNtcBaseList::iterator itLastMatchPacket;//���ָ������������һ��ƥ��İ�λ��
    MDB_UINT32  uiLastMatchCount;///< ��ʾƥ�����ֹ������
    MDB_UINT32  uiLastMatchMsgLength;///< ��һ��ƥ��������ݰ�����
    TMdbPacketInfo oSpecTermPacket;///< ����Ĵӿ�ʼ����ֹ�������ݰ�������У�飨�ر�����Ϣͷ��������Э�飩
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
 * @brief ��Ϣ�࣬һ����������Ϣ��
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
     * @brief �����Ҫ����Ϣ�������ã�����Ҫ��������
     * 
     */
    inline void AddRef()
    {
        ++m_uiRefcnt;
    }
    /**
     * @brief ��Ϣָ����ͷţ���ͨ��Release
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
    MDB_UINT32  m_uiRefcnt;///<���ø���
    MDB_UINT32  m_uiHeadLength;///< ��Ϣ��ͷ����
};

/**
 * @brief �ѽ��յ����ݰ�
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
     * @brief ���packet
     * 
     * @param pPacketInfo [in] packet��Ϣ
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
     * @brief ƴ������
     * �������Ҫ���������ֱ���û�packet��buffer
     * 
     * @param uiLength [in] ��ŵ�buffer����
     * @param oPacketInfo [out] ���ɵ�buffer
     * @return bool
     * @retval true �ɹ�
     */
    bool SplicePacket(MDB_UINT32 uiLength, TMdbPacketInfo& oPacketInfo);
protected:
    MDB_UINT32              m_uiTotalLength;///< ���ݰ��ܳ���,�ܹ����������ж��Ƿ�ﵽ��������Ϣ��
public:
    MDB_UINT32              uiSplicedLength;///< �Ѿ�ƴ�ӵĳ���
    TMdbMsgInfo*           pSplicingMsg;///< ����ƴ�ӵ�msg
    TMdbSpecTermMatchInfo  oSpecTermMatchInfo;///< ��һ����ֹ��ƥ����Ϣ
};

/**
 * @brief �����͵����ݰ�
 * 
 */
class TMdbSendPackets:public TMdbNtcQueue
{
public:
    int iSendBytes;///< �ѷ����ֽ���
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
 * @brief �������Ϣ
 * 
 */
class TMdbServerInfo: public TMdbNtcServerSocket
{
    MDB_ZF_DECLARE_OBJECT(TMdbServerInfo);
public:
    TMdbProtocol*              pProtocol;  ///< ��ʹ�õ�Э��
    const TMdbRuntimeObject* pPeerRuntimeObject;///< ���ӵ�����ʱ���࣬���ڴ����µ����Ӷ���
    TMdbPeerProactor*          pPeerProactor;///< �����׽�����ʹ�õ�ǰ����
    TMdbProactorInfo*          pProactorInfo;///< listen socket��ʹ�õ�ǰ���������Ϣ
    TMdbServerInfo(const char* pszServerHost = NULL, int iServerPort = 0, TMdbProtocol* pProtocol = NULL, const TMdbRuntimeObject* pPeerRuntimeObject = NULL);
    virtual ~TMdbServerInfo();
    /**
     * @brief �����Ҫ�������¼�
     * 
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     * @retval true �ɹ�
     */
    bool AddEventMonitor(MDB_UINT16 events);
    /**
     * @brief �Ƴ�����Ҫ�������¼�
     * 
     * @param events [in] ����Ҫ�����¼�,��EV_READ,EV_WRITE
     * @return bool
     * @retval true �ɹ�
     */
    bool RemoveEventMonitor(MDB_UINT16 events);
};

/**
 * @brief �ȽϺ�������Ҫ�Զ���Ƚ�ʱ����,����Compare����
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
 * @brief ���ӵĸ�������
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
 * @brief ����״̬
 * 
 */
enum MDB_NTC_PEER_STATE
{
    MDB_NTC_PEER_IDLE_STATE         =   0x00,  ///< ���Ӵ��ڿ���״̬
    MDB_NTC_PEER_RECV_STATE         =   0x01,  ///< ���Ӵ��ڽ�������״̬
    MDB_NTC_PEER_WRITE_STATE        =   0x02,  ///< ���Ӵ��ڷ�������״̬
};
const MDB_UINT16 MDB_NTC_PEER_SHUTDOWN_FLAG     =   0x04;///< ��ʾ�����Ƿ�ȴ��رգ��ر����ʾ���ٽ������¼�
const MDB_UINT16 MDB_NTC_PEER_PASV_CLOSE_FLAG   =   0x08;///< ��ʾ�Ƿ񱻶��ر�
const MDB_UINT16 MDB_NTC_PEER_EV_READ_FLAG      =   0x10;///< ��ʾ��������������
const MDB_UINT16 MDB_NTC_PEER_EV_WRITE_FLAG     =   0x20;///< ��ʾ������������д
const MDB_UINT16 MDB_NTC_PEER_MSG_DETACHED_FLAG =   0x40;///< ��ʾ���ӵ���Ϣ���Դ���

const MDB_UINT16 MDB_NTC_PEER_STATE_MASK    =   0x03;
const MDB_UINT16 MDB_NTC_PEER_EV_MASK       =   0x30;///< ��д�¼���mask

/**
 * @brief ���г�ʱ������
 * 
 */
enum MDB_NTC_PEER_TIMEOUT_TYPE
{
    MDB_NTC_DATA_IDLE_TIMEOUT   =   0,///< ��ʱû���շ�����
    MDB_NTC_RECV_IDLE_TIMEOUT   =   1,///< ��ʱû���յ�����
    MDB_NTC_SEND_IDLE_TIMEOUT   =   2,///< ��ʱû�з�������
    MDB_NTC_DURATION_TIMEOUT    =   3,///< ���ӽ���ʱ���ﵽ
    MDB_NTC_MAX_IDLE_TIMEOUT_TYPE
};

class TSendMsgEvent;
class TMdbEventPump;
/**
 * @brief peer������Ϣ
 * 
 */
class TMdbPeerInfo:public TMdbNtcSocket
{
    MDB_ZF_DECLARE_DYNCREATE_OBJECT(TMdbPeerInfo);
public:
    MDB_UINT16  pno;                ///< pno
    TMdbProactorInfo*  pProactorInfo;///< ������ʹ�õ�ǰ������Ϣ
    TMdbPeerProactor*  pPeerProactor;///< �����ϵ��¼�������ǰ����    
    TMdbServerInfo*    pServerInfo; ///< socket�������Ϣ
    TMdbEventPump*     pCurEventPump;///< ��ǰ����ͨ��
    TMdbProtocol*      pProtocol;///< ������ʹ�õ�Э�飬Ĭ��ָ��Server��Protocol    
    TMdbSendPackets    oSendPackets;   ///< �����͵����ݰ�
    TMdbRecvPackets    oRecvPackets;   ///< ���յ������ݰ�    
    TMdbNtcStringBuffer      sDisconnectReason;///< �Ͽ�ԭ��
protected:
    MDB_UINT16          m_uiPeerFlag;       ///< flag��־λ�����Եõ��й�pno����Ϣ
    MDB_UINT32          m_uiTimerID[MDB_NTC_MAX_IDLE_TIMEOUT_TYPE];     ///< ���ж�ʱ��id
    MDB_UINT32          m_uiMaxIdleTime[MDB_NTC_MAX_IDLE_TIMEOUT_TYPE];     ///< ������ʱ�䣬>0��ʾ��Ч
    time_t          m_tConnectime;      ///< ����ʱ��
    TMdbTrafficInfo    m_oTrafficInfo;///������Ϣ
    TMdbTrafficCtrl*   m_pTrafficCtrl;   ///< ����������
    TMdbPeerHelper*    m_pPeerHelper;///< ����Ϊpeer�ṩ���õĽӿڻ�������Ϣ
    TMdbNtcThreadLock  m_oSpinLock;
public:
    /**
     * @brief ���캯��
     *      
     */
    TMdbPeerInfo();
    virtual ~TMdbPeerInfo();
    /**
     * @brief ��ʼ��
     * 
     * @param uiSocketID [in] socket������
     * @param pServerInfo [in] �������Ϣ
     * @return ��
     */
    void Init(QuickMDB_SOCKET uiSocketID, TMdbServerInfo* pServerInfo = NULL);
    /**
     * @brief ����helper
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
     * @brief ���helper
     * 
     * @return TMdbPeerHelper*
     */
    inline TMdbPeerHelper* GetHelper()
    {
        return m_pPeerHelper;
    }
    /**
     * @brief �������ӵĶ�ʱ��
     * 
     * @param eTimeoutType  [in] ��ʱ������
     * @param uiSeconds     [in] ��ʱ��ʱ�䣬��λ��
     */
    void SetPeerTimeout(MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutType, MDB_UINT32 uiSeconds);
    /**
     * @brief ����Timeout��Ӧ�Ķ�ʱ��id
     * 
     * @param eTimeoutType  [in] ��ʱ������
     * @param uiTimerId     [in] ��ʱ��id
     */
    inline void SetTimeoutId(MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutType, MDB_UINT32 uiTimerId)
    {
        m_uiTimerID[eTimeoutType] = uiTimerId;
    }
    /**
     * @brief ���������ʱ��
     * 
     * @param eIdleTimeoutType [in] ��������
     * @return int
     * @retval ������ʱ��
     */
    inline MDB_UINT32 GetPeerTimeout(MDB_NTC_PEER_TIMEOUT_TYPE eTimeoutType)
    {
        return m_uiMaxIdleTime[eTimeoutType];
    }
    /**
     * @brief �������ʱ��
     * 
     * @return time_t
     */
    inline time_t GetConnectTime()
    {
        return m_tConnectime;
    }
    /**
     * @brief �����һ�ν�������ʱ��
     * 
     * @return time_t
     */
    inline time_t GetLastRecvTime() const
    {
        return m_oTrafficInfo.GetLastRecvTime();
    }
    /**
     * @brief �����һ�η�������ʱ��
     * 
     * @return time_t
     */
    inline time_t GetLastSendTime() const
    {
        return m_oTrafficInfo.GetLastSendTime();
    }
    /**
     * @brief ���������Ϣ
     * 
     * @return TMdbTrafficCtrl&
     * @retval ������Ϣ
     */
    inline TMdbTrafficInfo* GetTrafficInfo()
    {
        return &m_oTrafficInfo;
    }    
    /**
     * @brief �������������
     * 
     * @return TTrafficRateCtrl*
     */
    TMdbTrafficCtrl* GetTrafficCtrl()
    {
        return m_pTrafficCtrl;
    }
    /**
     * @brief ɾ������������
     *      
     * @return ��
     */
    void DeleteTrafficCtrl();
    /**
     * @brief �������������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiMaxRecvRate [in] �ܵ��������
     * @return ��
     */
    void SetMaxRecvRate(MDB_UINT32 uiMaxRecvRate);
    /**
     * @brief ������������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiMaxSendRate [in] �ܵ��������
     * @return ��
     */
    void SetMaxSendRate(MDB_UINT32 uiMaxSendRate);
    /**
     * @brief ���������պͷ��������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @param uiMaxFlowRate [in] �ܵ��������
     * @return ��
     */
    void SetMaxFlowRate(MDB_UINT32 uiMaxFlowRate);
    /**
     * @brief ����ܵ����������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetMaxRecvRate()
    {
        return m_pTrafficCtrl?m_pTrafficCtrl->GetMaxRecvRate():0;
    }
    /**
     * @brief ����ܵ���������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetMaxSendRate()
    {
        return m_pTrafficCtrl?m_pTrafficCtrl->GetMaxSendRate():0;
    }
    /**
     * @brief ����ܵ������պͷ��������ʣ���λΪbytes/s��=0��ʾ������
     * 
     * @return ��
     */
    inline MDB_UINT32 GetMaxFlowRate()
    {
        return m_pTrafficCtrl?m_pTrafficCtrl->GetMaxFlowRate():0;
    }
    /**
     * @brief ����
     * 
     * @return bool
     * @retval true �ɹ�
     */
    inline bool Lock()
    {
        return m_oSpinLock.Lock();
    }
    /**
     * @brief ����
     * 
     * @return bool
     * @retval true �ɹ�
     */
    inline bool Unlock()
    {
        return m_oSpinLock.Unlock();
    }
    /**
     * @brief ������Ϣ����ģʽ
     * 
     * @param bDetached [in] �Ƿ���뿪��������
     * @return ��
     */
    void SetMsgHandleMode(bool bDetached);
    /**
     * @brief ��Ϣ�Ƿ񱻶�������
     * 
     * @return bool
     * @retval true ��Ϣ��������
     */
    inline bool IsMsgDetached()
    {
        return (m_uiPeerFlag&MDB_NTC_PEER_MSG_DETACHED_FLAG)==MDB_NTC_PEER_MSG_DETACHED_FLAG;
    }
    /**
     * @brief �������ӱ�ʶ
     * 
     * @param uiFlag [in] ���ӱ�ʶ
     * @return ��
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
     * @brief ������ӱ�ʶ״̬
     * 
     * @param uiFlagMask [in] ���ӱ�ʶ
     * @return MDB_UINT16
     */
    inline MDB_UINT16 GetPeerFlag(MDB_UINT16 uiFlagMask = 0xFFFF)
    {
        return (MDB_UINT16)(m_uiPeerFlag&uiFlagMask);
    }
    /**
     * @brief ������ӱ�ʶ״̬
     * 
     * @param uiFlag [in] ���ӱ�ʶ
     * @return bool
     */
    inline bool IsSetPeerFlag(MDB_UINT16 uiFlag)
    {
        return (m_uiPeerFlag&uiFlag)==uiFlag;
    }
    /**
     * @brief ������ӱ�ʶ
     * 
     * @param uiFlag [in] ���ӱ�ʶ
     * @return ��
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
     * @brief �Ƿ񱻶��ر�
     * 
     * @return bool
     * @retval true �Ǳ����ر�
     */
    inline bool IsPasvClose()
    {
        return (m_uiPeerFlag&MDB_NTC_PEER_PASV_CLOSE_FLAG)==MDB_NTC_PEER_PASV_CLOSE_FLAG;
    }
    /**
     * @brief �������״̬
     * 
     * @return PEER_TYPE
     * @retval ��������
     */
    inline MDB_NTC_PEER_STATE GetPeerState()
    {
        return (MDB_NTC_PEER_STATE)(m_uiPeerFlag&MDB_NTC_PEER_STATE_MASK);
    }
    /**
     * @brief ��������״̬
     * 
     * @param ePeerState [in] ����״̬
     * @return ��
     */
    inline void SetPeerState(MDB_NTC_PEER_STATE ePeerState)
    {
        Lock();
        m_uiPeerFlag = (MDB_UINT16)((m_uiPeerFlag&(~MDB_NTC_PEER_STATE_MASK))|ePeerState);
        Unlock();
    }
    /**
     * @brief �����Ƿ��ѱ�shutdown�رգ�����ǣ���Ҫ�����������ϲ����¼�
     * 
     * @return bool
     * @retval true �ر�
     */
    inline bool IsShutdown()
    {
        return (m_uiPeerFlag & MDB_NTC_PEER_SHUTDOWN_FLAG)==MDB_NTC_PEER_SHUTDOWN_FLAG;
    }
    /**
     * @brief ͬ����ʽ��������
     * 
     * @param pszMsg [in] ��Ϣ��
     * @param uiMsgLength [in] ��Ϣ������,-1��ʾ��\0��β
     * @param iMilliSeconds [in] ��ʱʱ�䣨��λ���룩,-1��ʾ����ʱ
     * @return int
     * @retval �趨��ʱ���ڣ�ʵ�ʷ����ֽ���
     */
    virtual int SendMessage(const void* pszMsg, MDB_UINT32 uiMsgLength = (MDB_UINT32)-1, int iMilliSeconds = -1);
    /**
     * @brief �첽��ʽ��������,�����¼�������
     * 
     * @param pMsg [in] ��Ϣ��
     * @param uiMsgLength [in] ��Ϣ������,-1��ʾ�ַ�����\0��β
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool PostMessage(const void* pMsg, MDB_UINT32 uiMsgLength = (MDB_UINT32)-1);
    bool PostMessage(TMdbPacketInfo* pPacketInfo);
    /**
     * @brief ��ȡһ����������Ϣ�¼�(����)
     * 
     * @param iMilliSeconds [in] ��ʱ���ã���λΪ���룬-1��ʾ����ʱ
     * @return TMdbMsgInfo*
     * @retval ��NULL �ɹ�
     */
    inline TMdbMsgInfo* GetMessage(int iMilliSeconds = -1)
    {
        if(m_pRecvMsgQueue) return static_cast<TMdbMsgInfo*>(m_pRecvMsgQueue->Pop(iMilliSeconds));
        else return NULL;
    }
    /**
     * @brief ѹ��һ����Ϣ
     * 
     * @param pMsgInfo [in] ��Ϣ
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
     * @brief ����������յ�����Ϣ
     * 
     */
    void ClearRecvMessage();
    /**
     * @brief �ر�����
     * 
     * @param sReason       [in] �ر����ӵ�ԭ��
     * @param bPasvClose    [in] �Ƿ񱻶��ر�(Ҳ���ǶԷ������رյ�)
     * @return bool
     * @retval true �ɹ�
     */
    virtual bool Disconnect(TMdbNtcStringBuffer sReason = "", bool bPasvClose = false);
    /**
     * @brief ������������ص�ǰ������ʹ�õķַ���
     * 
     * @return TMdbEventDispatcher*
     * @retval �ַ���
     */
    inline TMdbEventDispatcher* GetEventDispatcher()
    {
        return pPeerProactor?pPeerProactor->GetEventDispatcher():NULL;
    }
    /**
     * @brief �Ƿ���Ϊ�ͻ�������
     * 
     * @return bool
     * @retval true ��
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
     * @brief �����յ�����Ϣ��
     * 
     */
    void CheckRecvPackets();
    /**
     * @brief ƴ����Ϣ��,����������Ϣ��ʱ������OnRecvMsg�¼�
     * 
     * @return bool
     * @retval true �ɹ�
     */
    bool SpliceMsg();
public://���¼��������йصĺ���
    /**
     * @brief �����Ҫ�������¼�
     * 
     * @param events [in] ��Ҫ�����¼�,��EV_READ,EV_WRITE
     * @param bLock [in] �Ƿ���Ҫ�������⣬Ĭ����Ҫ
     * @return bool
     * @retval true �ɹ�
     */
    bool AddEventMonitor(MDB_UINT16 events, bool bLock = true);
    /**
     * @brief �Ƴ�����Ҫ�������¼�
     * 
     * @param events [in] ����Ҫ�����¼�,��EV_READ,EV_WRITE
     * @param bLock [in] �Ƿ���Ҫ�������⣬Ĭ����Ҫ
     * @return bool
     * @retval true �ɹ�
     */
    bool RemoveEventMonitor(MDB_UINT16 events, bool bLock = true);
protected:
    /**
     * @brief �ر����ӣ����ٽ����µ��¼�
     * 
     * @return bool
     * @retval true �ɹ�
     */
    bool Shutdown();
    /**
     * @brief �����е�����(��ʱ���Ļص�)
     * 
     * @param pArg [in] TCheckIdlePeer�ṹ��Ϣ
     * @return ��
     */
    static void CheckPeerTimeout(void* pArg);
protected:
    TMdbNtcQueue* m_pRecvMsgQueue;///< ���������Ϣ����ģʽ����ô����·��Ϣ�����˶��У�������OnRecvMsg
};
MDB_UINT32 MdbNtcAddTimer(const char *ATimerName,bool ALoopFlag,MDB_UINT32 ACount100ms,OnMdbEventFunc AEventFunc,void *pFuncParam);
//}
#endif
