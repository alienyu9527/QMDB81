/**
 * @file PeerProtocol.hxx
 * Э���࣬��������ͨѶ��Э�������չ
 * 
 * @author w
 * @version 1.0
 * @date 2012/05/17
 * @bug �½�����bug
 * @bug ��Ԫ���ԣ�δ����bug
 * @warning 
 */
#ifndef _MDB_PEER_PROTOCOL_HXX_
#define _MDB_PEER_PROTOCOL_HXX_
#include "Common/mdbNTC.h"
#include "Common/mdbCommons.h"
#include "Common/mdbStrings.h"
#include "Common/mdbSysThreads.h"
#include "Common/mdbPeerInfo.h"
//namespace QuickMDB
//{
/**
 * @brief sunday�㷨�Ӵ�����
 * 
 */
class TMdbSundaySearch
{
public:
    TMdbSundaySearch(const char* pszPattern);
    /**
     * @brief ���ò��ҵ��Ӵ�
     * 
     * @param pszPattern [in] �Ӵ�
     */
    void SetPattern(const char* pszPattern);
    /**
     * @brief �����Ӵ�
     * 
     * @param pszSrc [in] Դ��
     * @param iLength [in] Դ���ĳ���
     * @param bComparePartly [in] �Ƿ񲿷̶ֳ��ϲ��ң������ĩβ����ƥ���Ƿ�Ҳ����ƥ��λ��
     * @return int
     * @retval -1��ʾû�ҵ�
     */
    int Search(const char* pszSrc, int iLength = -1, bool bComparePartly = false) const;
    inline const TMdbNtcStringBuffer& GetPattern() const
    {
        return m_sPattern;
    }
protected:
    TMdbNtcStringBuffer m_sPattern;
    int m_iShiftTable[256];///< ÿ���ַ���ƫ��ֵ
};

/**
 * @brief Э������
 * Э���Ϊ����
 * 1.��Ϣ���̶�����
 * 2.��Ϣ���Ա�ʶ����(��ЩЭ����\n��ʾ��Ϣ������)
 * 3.��Ϣͷ�̶����ȣ�������Ϣͷ��������Ϣ���ܳ���(��DIAMETERЭ��)
 * 4.��Ϣͷ�Ա�ʶ������������Ϣͷ��������Ϣ���ܳ���(��HTTPЭ����\r\n\r\n)
 */

/**
 * @brief Э�����
 * ����Э���ж�һ����Ϣ���Ƿ���Ч����
 */
class TMdbProtocol:public TMdbNtcBaseObject
{
public:
    MDB_ZF_DECLARE_OBJECT(TMdbProtocol);
    /**
     * @brief ������Ϣ�������ݶ����ж��Ƿ���Ϣ���ĺϷ���
     * ����Ϸ������ж��Ƿ���������Ϣ�����ȣ�������ԣ����ʼ��pMsgPackets->pSplicingMsg
     * 
     * @param pMsgPackets [in] ���ݰ�����Ϣ
     * @return ��
     */
    virtual void CheckPackets(TMdbPeerInfo* pPeerInfo) = 0;
    /**
     * @brief Ԥ������յ�����Ϣ��
     * 
     * @param pPeerInfo [in] ������Ϣ
     * @param pMsgInfo [in/out] ��Ϣ��
     * @return bool
     * @retval true ��ʾ�Ѿ���Ԥ�������ټ����ַ�
     * @retval false ��ʾδ��Ԥ������Ҫ�����ַ�
     */
    virtual bool PreTranslateMessage(TMdbPeerInfo* pPeerInfo, TMdbMsgInfo*& pMsgInfo);
};

/**
 * @brief ��Ϣ���̶����ȵ�Э��
 * 
 */
class TMdbMsgFixLength:public TMdbProtocol
{
public:
    MDB_ZF_DECLARE_OBJECT(TMdbMsgFixLength);
    /**
     * @brief ���캯��
     * 
     * @param iHeadLength [in] ��Ϣ�̶�����
     */
    TMdbMsgFixLength(MDB_UINT32 uiFixLength):m_uiFixLength(uiFixLength)
    {
    }
    inline MDB_UINT32 GetFixLength()
    {
        return m_uiFixLength;
    }
    virtual void CheckPackets(TMdbPeerInfo* pPeerInfo);
protected:
    /**
     * @brief ����buffer�ж����ɺ������͵�msg����
     * 
     * @param pPeerInfo [in] ��ǰ��������Ϣ
     * @param pcBuffer [in] buffer
     * @param uiBufferLength [in] buffer����
     * @param pMsgInfo [out] ���ɵ���Ϣ����
     * @retval true ��������
     * @retval false Э�������������
     */
    virtual bool GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcBuffer, MDB_UINT32 uiBufferLength, TMdbMsgInfo*& pMsgInfo);
protected:
    MDB_UINT32 m_uiFixLength;///< ��Ϣ�Ĺ̶�����
};

/**
 * @brief ��Ϣ���ض���������Э��
 * 
 */
class TMdbMsgSpecTerm:public TMdbProtocol
{    
public:
    MDB_ZF_DECLARE_OBJECT(TMdbMsgSpecTerm);
    /**
     * @brief ���캯��
     * 
     * @param pszSpecTerm [in] ��Ϣ��������
     */
    TMdbMsgSpecTerm(const char* pszSpecTerm);
    virtual void CheckPackets(TMdbPeerInfo* pPeerInfo);
    /**
     * @brief ���÷ָ��ַ�
     * 
     * @param pszSpecTerm [in] ��Ϣ��������
     */
    void SetSpecTerm(const char* pszSpecTerm);
protected:
    /**
     * @brief ʹ��sunday�㷨����ƥ����Ӵ�����ʹĩβ����ƥ��Ҳ����
     * 
     * @param pszSrc [in] Դ��
     * @param iLength [in] Դ���ĳ���
     * @return bool
     * @retval true �ɹ�
     */
    bool SundaySearch(const char *pszSrc, int iLength = -1);
    /**
     * @brief ����buffer�ж����ɺ������͵�msg����
     * 
     * @param pPeerInfo [in] ��ǰ��������Ϣ
     * @param pcBuffer [in] buffer
     * @param uiBufferLength [in] buffer����
     * @param pMsgInfo [out] ���ɵ���Ϣ����
     * @retval true ��������
     * @retval false Э�������������
     */
    virtual bool GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcBuffer, MDB_UINT32 uiBufferLength, TMdbMsgInfo*& pMsgInfo);
private:
    TMdbSundaySearch m_oSubStrSearch;///< �Ӵ����ҵ��㷨
};

/**
 * @brief ��Ϣͷ�̶����ȵ�Э��
 * 
 */
class TMdbHeadFixLength:public TMdbProtocol
{
public:
    MDB_ZF_DECLARE_OBJECT(TMdbHeadFixLength);
    /**
     * @brief ���캯��
     * 
     * @param uiHeadFixLength [in] ��Ϣͷ�̶�����
     */
    TMdbHeadFixLength(MDB_UINT32 uiHeadFixLength);
    virtual ~TMdbHeadFixLength();
    virtual void CheckPackets(TMdbPeerInfo* pPeerInfo);
public:
    inline MDB_UINT32 GetHeadFixLength()
    {
        return m_uiHeadFixLength;
    }
protected:
    /**
     * @brief ����buffer�ж����ɺ������͵�msg����
     * 
     * @param pPeerInfo [in] ��ǰ��������Ϣ
     * @param pcBuffer [in] buffer
     * @param uiBufferLength [in] buffer����
     * @param pMsgInfo [out] ���ɵ���Ϣ����
     * @return bool
     * @retval true ��������
     * @retval false Э�������������
     */
    virtual bool GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcHeadBuffer, MDB_UINT32 uiHeadLength, TMdbMsgInfo*& pMsgInfo) = 0;
protected:
    MDB_UINT32  m_uiHeadFixLength;  ///< ��Ϣͷ�̶�����
    char*   m_pcHeadBuffer;     ///< ������ʱ�����洢headʹ��
};

/**
 * @brief ��Ϣͷ�ض���������Э��
 * 
 */
class TMdbHeadSpecTerm:public TMdbMsgSpecTerm
{
public:
    MDB_ZF_DECLARE_OBJECT(TMdbHeadSpecTerm);
    /**
     * @brief ���캯��
     * 
     * @param pszSpecTerm [in] ��Ϣ��ͷ�Ľ�����
     */
    TMdbHeadSpecTerm(const char* pszHeadSpecTerm);
protected:
    /**
     * @brief ����buffer�ж����ɺ������͵�msg����
     * 
     * @param pPeerInfo [in] ��ǰ��������Ϣ
     * @param pcBuffer [in] buffer
     * @param uiHeadLength [in] buffer����
     * @param pMsgInfo [out] ���ɵ���Ϣ����
     * @retval true ��������
     * @retval false Э�������������
     */
    virtual bool GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcHeadBuffer, MDB_UINT32 uiHeadLength, TMdbMsgInfo*& pMsgInfo) = 0;
protected:    
    TMdbNtcString m_sHeadSpecTerm;///< ��Ϣ��������
};
//}
#endif
