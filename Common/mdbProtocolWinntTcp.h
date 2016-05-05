/**
 * @file ProtocolWinntTcp.hxx
 * wintcp协议
 * 
 * @author jiang.jinzhou@zte.com.cn
 * @version 1.0
 * @date 2012/09/18
 * @bug 新建，无bug
 * @bug 单元测试，未发现bug
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

//设置对齐方式
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
 * @brief wintcp协议的PID结构定义
 *
 */
struct MDB_PID
{
#ifdef OS_IBM
    unsigned int pno   :14; ///< 进程号
    unsigned int slave :1;  ///< slave=1/0: 备用PP/主用PP
    unsigned int MpLeft:1;  ///< 1--Left  MP(B), Up Mp (A)
#else
    unsigned short pno   :14; ///< 进程号
    unsigned short slave :1;  ///< slave=1/0: 备用PP/主用PP
    unsigned short MpLeft:1;  ///< 1--Left  MP(B), Up Mp (A)
#endif
    unsigned char  unit;    ///< 单元号
    unsigned char  module;  ///< 模块号
    unsigned char  postOffice;///< 局号
};

/**
 * @struct TMdbWintcpMsgHead
 * @brief wintcp协议的消息头
 *
 */
class TMdbWintcpMsgHead
{
public:
    MDB_PID     sender;  ///< 发送方PID
    MDB_PID     receiver;///< 接收方PID
    MDB_UINT16  event; ///< 事件号
    MDB_UINT16  len; ///< 消息长度
    MDB_UINT8   type; ///< 消息类型:是否同步。 目前不使用

    //下面字段，用与原系统兼容，不使用
    MDB_UINT32  secretfield;///< 用与原系统兼容，不使用
    MDB_UINT32  ack;  ///< 用与原系统兼容，不使用
    MDB_UINT16  acklen;///< 用与原系统兼容，不使用
    MDB_UINT32  reserve;///< 用与原系统兼容，不使用
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
 * @brief wintcp协议的消息包
 * 
 */
class TMdbWinntTcpMsg:public TMdbMsgInfo
{
public:
    TMdbWintcpMsgHead oMsgHead;///< 消息头信息
    TMdbWinntTcpMsg()
    {
        memset(&oMsgHead, 0x00, sizeof(oMsgHead));
    }
};

/**
 * @brief winnttcp协议
 * 
 */
class TMdbWinntTcpProtocol:public TMdbHeadFixLength
{
    MDB_ZF_DECLARE_OBJECT(TMdbWinntTcpProtocol);
public:
    TMdbWinntTcpProtocol();
    /**
     * @brief 校验消息头
     * 
     * @param oMsgHead [in] 消息头
     * @return bool
     * @retval true 成功
     */
    inline static bool CheckMsgHead(const TMdbWintcpMsgHead& oMsgHead)
    {
        return oMsgHead.CheckMsgHead();
    }
    /**
     * @brief 根据消息头字段，生成校验字段
     * 
     * @param oMsgHead [in/out] 消息头
     */
    inline static void MakeSecretHead(TMdbWintcpMsgHead& oMsgHead)
    {
        oMsgHead.MakeSecretHead();
    }
    /**
     * @brief 从网络buffer转成TMsgHead结构
     *      
     * @param oMsgHead [out] 消息头
     * @param pcData [in] 网络buffer
     */
    static void HeadFromNetBuffer(TMdbWintcpMsgHead& oMsgHead, const void* pcData);
    /**
     * @brief 从TMsgHead结构转成网络buffer
     *      
     * @param oMsgHead [in] 消息头
     * @param pcData [out] 网络buffer
     */
    static void HeadToNetBuffer(const TMdbWintcpMsgHead& oMsgHead, void* pcData);
protected:
    /**
     * @brief 解析消息头，生成相应的消息包
     * 
     * @param pPeerInfo [in] 当前的链接信息
     * @param pcHeadBuffer [in] 消息头buffer
     * @param uiHeadLength [in] 消息头buffer的长度
     * @param pMsgInfo [out] 生成的消息包信息
     * @retval true 继续处理
     * @retval false 协议无需继续处理
     */
    virtual bool GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcHeadBuffer, MDB_UINT32 uiHeadLength, TMdbMsgInfo*& pMsgInfo);
};

/**
 * @brief winntcp的连接辅助信息
 * 
 */
class TMdbWinntTcpHelper:public TMdbPeerHelper
{
public:
    MDB_PID pid;///< 连接的对方PID
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
     * @brief 发送消息包
     * 
     * @param type [in] 类型
     * @param event [in] 事件
     * @param pValue [in] buffer
     * @param uiLength [in] buffer长度
     * @param receiver [in] 接收者的pid
     * @param sender [in] 发送者的pid
     * @return bool
     * @retval true 成功
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
     * @brief 发送心跳包
     * 
     * @return bool
     * @retval true 成功
     */
    inline bool SendCheckPacket()
    {
        return SendPacket(MDB_TCP_CHECK_TYPE, 0, "", 11, pid, ms_oSysSelfPID);
    }
    static void SetSysSelfPID(MDB_UINT8 module, MDB_UINT8 postOffice = 0, MDB_UINT16 pno = 0);
    /**
     * @brief 设置系统自身的pid
     * 
     * @param oSysSelfPID [in] 系统自身的PID
     * @return 无
     */
    inline static void SetSysSelfPID(MDB_PID oSysSelfPID)
    {
        ms_oSysSelfPID = oSysSelfPID;
    }
protected:
    static MDB_PID ms_oSysSelfPID;///< 系统自身的PID
};
//}
#endif
