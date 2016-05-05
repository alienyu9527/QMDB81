/**
 * @file ProtocolWinntTcp.hxx
 * wintcpЭ��
 * 
 * @author jiang.jinzhou@zte.com.cn
 * @version 1.0
 * @date 2012/09/18
 * @bug �½�����bug
 * @bug ��Ԫ���ԣ�δ����bug
 * @warning 
 */
#ifndef _MDB_PROTOCOL_WINNTCP_HXX_
#define _MDB_PROTOCOL_WINNTCP_HXX_
#include "Common/mdbPeerProtocol.h"
//_NTC_BEGIN
//namespace QuickMDB{
/*  Message Type    */
const MDB_UINT8 MDB_TCP_CHECK_TYPE  =   0xFF;
const MDB_UINT8 MDB_ASYN_EVENT      =   0x00;
const MDB_UINT8 MDB_SYNC_EVENT      =   0x01;

//���ö��뷽ʽ
#ifdef OS_HP
#pragma pack  1
#elif defined(OS_IBM)
#pragma options align=packed
#elif defined(_WIN32)
#pragma pack(push,1)
#else
#pragma pack(1)
#endif

/**
 * @struct MDB_PID
 * @brief wintcpЭ���PID�ṹ����
 *
 */
struct MDB_PID
{
#ifdef OS_IBM
    unsigned int pno   :14; ///< ���̺�
    unsigned int slave :1;  ///< slave=1/0: ����PP/����PP
    unsigned int MpLeft:1;  ///< 1--Left  MP(B), Up Mp (A)
#else
    unsigned short pno   :14; ///< ���̺�
    unsigned short slave :1;  ///< slave=1/0: ����PP/����PP
    unsigned short MpLeft:1;  ///< 1--Left  MP(B), Up Mp (A)
#endif
    unsigned char  unit;    ///< ��Ԫ��
    unsigned char  module;  ///< ģ���
    unsigned char  postOffice;///< �ֺ�
};

/**
 * @struct TMdbWintcpMsgHead
 * @brief wintcpЭ�����Ϣͷ
 *
 */
class TMdbWintcpMsgHead
{
public:
    MDB_PID     sender;  ///< ���ͷ�PID
    MDB_PID     receiver;///< ���շ�PID
    MDB_UINT16  event; ///< �¼���
    MDB_UINT16  len; ///< ��Ϣ����
    MDB_UINT8   type; ///< ��Ϣ����:�Ƿ�ͬ���� Ŀǰ��ʹ��

    //�����ֶΣ�����ԭϵͳ���ݣ���ʹ��
    MDB_UINT32  secretfield;///< ����ԭϵͳ���ݣ���ʹ��
    MDB_UINT32  ack;  ///< ����ԭϵͳ���ݣ���ʹ��
    MDB_UINT16  acklen;///< ����ԭϵͳ���ݣ���ʹ��
    MDB_UINT32  reserve;///< ����ԭϵͳ���ݣ���ʹ��
    TMdbWintcpMsgHead()
    {
        memset(this, 0x00, sizeof(TMdbWintcpMsgHead));
    }
    bool CheckMsgHead() const;
    void MakeSecretHead();
};


#ifdef OS_HP
    #ifdef _64_BIT_
        #pragma pack 8
    #else
        #pragma pack 4
    #endif
#elif defined(OS_IBM)
    #pragma options align=natural
#elif defined(OS_LINUX)
    #pragma pack()
#elif defined(_WIN32)
    #pragma pack (pop)
#else
    #ifdef _64_BIT_ 
        #pragma pack(8) 
    #else 
        #pragma pack(4) 
    #endif
#endif

/**
 * @brief wintcpЭ�����Ϣ��
 * 
 */
class TMdbWinntTcpMsg:public TMdbMsgInfo
{
public:
    TMdbWintcpMsgHead oMsgHead;///< ��Ϣͷ��Ϣ
    TMdbWinntTcpMsg()
    {
        memset(&oMsgHead, 0x00, sizeof(oMsgHead));
    }
};

/**
 * @brief winnttcpЭ��
 * 
 */
class TMdbWinntTcpProtocol:public TMdbHeadFixLength
{
    MDB_ZF_DECLARE_OBJECT(TMdbWinntTcpProtocol);
public:
    TMdbWinntTcpProtocol();
    /**
     * @brief У����Ϣͷ
     * 
     * @param oMsgHead [in] ��Ϣͷ
     * @return bool
     * @retval true �ɹ�
     */
    inline static bool CheckMsgHead(const TMdbWintcpMsgHead& oMsgHead)
    {
        return oMsgHead.CheckMsgHead();
    }
    /**
     * @brief ������Ϣͷ�ֶΣ�����У���ֶ�
     * 
     * @param oMsgHead [in/out] ��Ϣͷ
     */
    inline static void MakeSecretHead(TMdbWintcpMsgHead& oMsgHead)
    {
        oMsgHead.MakeSecretHead();
    }
    /**
     * @brief ������bufferת��TMsgHead�ṹ
     *      
     * @param oMsgHead [out] ��Ϣͷ
     * @param pcData [in] ����buffer
     */
    static void HeadFromNetBuffer(TMdbWintcpMsgHead& oMsgHead, const void* pcData);
    /**
     * @brief ��TMsgHead�ṹת������buffer
     *      
     * @param oMsgHead [in] ��Ϣͷ
     * @param pcData [out] ����buffer
     */
    static void HeadToNetBuffer(const TMdbWintcpMsgHead& oMsgHead, void* pcData);
protected:
    /**
     * @brief ������Ϣͷ��������Ӧ����Ϣ��
     * 
     * @param pPeerInfo [in] ��ǰ��������Ϣ
     * @param pcHeadBuffer [in] ��Ϣͷbuffer
     * @param uiHeadLength [in] ��Ϣͷbuffer�ĳ���
     * @param pMsgInfo [out] ���ɵ���Ϣ����Ϣ
     * @retval true ��������
     * @retval false Э�������������
     */
    virtual bool GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcHeadBuffer, MDB_UINT32 uiHeadLength, TMdbMsgInfo*& pMsgInfo);
};

/**
 * @brief winntcp�����Ӹ�����Ϣ
 * 
 */
class TMdbWinntTcpHelper:public TMdbPeerHelper
{
public:
    MDB_PID pid;///< ���ӵĶԷ�PID
    TMdbWinntTcpHelper(TMdbPeerInfo* pPeerInfo);
    void SetPID(MDB_UINT8 module, MDB_UINT8 postOffice = 0, MDB_UINT16 pno = 0);
    inline void SetPID(MDB_PID oSysSelfPID)
    {
        pid = oSysSelfPID;
    }    
    inline MDB_UINT8 GetModule()
    {
        return pid.module;
    }
    /**
     * @brief ������Ϣ��
     * 
     * @param type [in] ����
     * @param event [in] �¼�
     * @param pValue [in] buffer
     * @param uiLength [in] buffer����
     * @param receiver [in] �����ߵ�pid
     * @param sender [in] �����ߵ�pid
     * @return bool
     * @retval true �ɹ�
     */
    bool SendPacket(MDB_UINT8 type, MDB_UINT16 event, const void* pValue, MDB_UINT16 uiLength, MDB_PID receiver, MDB_PID sender);
    inline bool SendPacket(MDB_UINT8 type, MDB_UINT16 event, const void* pValue, MDB_UINT16 uiLength, MDB_PID receiver)
    {
        return SendPacket(type, event, pValue, uiLength, receiver, ms_oSysSelfPID);
    }
    inline bool SendPacket(MDB_UINT8 type, MDB_UINT16 event, const void* pValue, MDB_UINT16 uiLength, MDB_PID receiver, MDB_UINT16 sender_pno)
    {
        MDB_PID sender = ms_oSysSelfPID;
        sender.pno = sender_pno;
        return SendPacket(type, event, pValue, uiLength, receiver, sender);
    }
    inline bool SendPacket(MDB_UINT8 type, MDB_UINT16 event, const void* pValue, MDB_UINT16 uiLength)
    {
        return SendPacket(type, event, pValue, uiLength, pid, ms_oSysSelfPID);
    }
    /**
     * @brief ����������
     * 
     * @return bool
     * @retval true �ɹ�
     */
    inline bool SendCheckPacket()
    {
        return SendPacket(MDB_TCP_CHECK_TYPE, 0, "", 11, pid, ms_oSysSelfPID);
    }
    static void SetSysSelfPID(MDB_UINT8 module, MDB_UINT8 postOffice = 0, MDB_UINT16 pno = 0);
    /**
     * @brief ����ϵͳ�����pid
     * 
     * @param oSysSelfPID [in] ϵͳ�����PID
     * @return ��
     */
    inline static void SetSysSelfPID(MDB_PID oSysSelfPID)
    {
        ms_oSysSelfPID = oSysSelfPID;
    }
protected:
    static MDB_PID ms_oSysSelfPID;///< ϵͳ�����PID
};
//}
#endif
