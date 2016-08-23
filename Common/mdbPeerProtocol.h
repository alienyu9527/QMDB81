/**
 * @file PeerProtocol.hxx
 * 协议类，用于网络通讯的协议解析扩展
 * 
 * @author w
 * @version 1.0
 * @date 2012/05/17
 * @bug 新建，无bug
 * @bug 单元测试，未发现bug
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
 * @brief sunday算法子串查找
 * 
 */
class TMdbSundaySearch
{
public:
    TMdbSundaySearch(const char* pszPattern);
    /**
     * @brief 设置查找的子串
     * 
     * @param pszPattern [in] 子串
     */
    void SetPattern(const char* pszPattern);
    /**
     * @brief 查找子串
     * 
     * @param pszSrc [in] 源串
     * @param iLength [in] 源串的长度
     * @param bComparePartly [in] 是否部分程度上查找，即如果末尾部分匹配是否也返回匹配位置
     * @return int
     * @retval -1表示没找到
     */
    int Search(const char* pszSrc, int iLength = -1, bool bComparePartly = false) const;
    inline const TMdbNtcStringBuffer& GetPattern() const
    {
        return m_sPattern;
    }
protected:
    TMdbNtcStringBuffer m_sPattern;
    int m_iShiftTable[256];///< 每个字符的偏移值
};

/**
 * @brief 协议类型
 * 协议分为四类
 * 1.消息包固定长度
 * 2.消息包以标识结束(有些协议以\n表示消息包结束)
 * 3.消息头固定长度，根据消息头分析，消息包总长度(如DIAMETER协议)
 * 4.消息头以标识结束，根据消息头分析，消息包总长度(如HTTP协议是\r\n\r\n)
 */

/**
 * @brief 协议基类
 * 根据协议判断一个消息包是否有效完整
 */
class TMdbProtocol:public TMdbNtcBaseObject
{
public:
    MDB_ZF_DECLARE_OBJECT(TMdbProtocol);
    /**
     * @brief 根据消息包的数据段来判断是否消息包的合法性
     * 如果合法，则判定是否可以算出消息包长度，如果可以，则初始化pMsgPackets->pSplicingMsg
     * 
     * @param pMsgPackets [in] 数据包的信息
     * @return 无
     */
    virtual void CheckPackets(TMdbPeerInfo* pPeerInfo) = 0;
    /**
     * @brief 预处理接收到的消息包
     * 
     * @param pPeerInfo [in] 连接信息
     * @param pMsgInfo [in/out] 消息包
     * @return bool
     * @retval true 表示已经被预处理，不再继续分发
     * @retval false 表示未被预处理，需要继续分发
     */
    virtual bool PreTranslateMessage(TMdbPeerInfo* pPeerInfo, TMdbMsgInfo*& pMsgInfo);
};

/**
 * @brief 消息包固定长度的协议
 * 
 */
class TMdbMsgFixLength:public TMdbProtocol
{
public:
    MDB_ZF_DECLARE_OBJECT(TMdbMsgFixLength);
    /**
     * @brief 构造函数
     * 
     * @param iHeadLength [in] 消息固定长度
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
     * @brief 根据buffer判断生成何种类型的msg对象
     * 
     * @param pPeerInfo [in] 当前的链接信息
     * @param pcBuffer [in] buffer
     * @param uiBufferLength [in] buffer长度
     * @param pMsgInfo [out] 生成的消息对象
     * @retval true 继续处理
     * @retval false 协议无需继续处理
     */
    virtual bool GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcBuffer, MDB_UINT32 uiBufferLength, TMdbMsgInfo*& pMsgInfo);
protected:
    MDB_UINT32 m_uiFixLength;///< 消息的固定长度
};

/**
 * @brief 消息包特定结束符的协议
 * 
 */
class TMdbMsgSpecTerm:public TMdbProtocol
{    
public:
    MDB_ZF_DECLARE_OBJECT(TMdbMsgSpecTerm);
    /**
     * @brief 构造函数
     * 
     * @param pszSpecTerm [in] 消息包结束符
     */
    TMdbMsgSpecTerm(const char* pszSpecTerm);
    virtual void CheckPackets(TMdbPeerInfo* pPeerInfo);
    /**
     * @brief 设置分隔字符
     * 
     * @param pszSpecTerm [in] 消息包结束符
     */
    void SetSpecTerm(const char* pszSpecTerm);
protected:
    /**
     * @brief 使用sunday算法查找匹配的子串，即使末尾部分匹配也可以
     * 
     * @param pszSrc [in] 源串
     * @param iLength [in] 源串的长度
     * @return bool
     * @retval true 成功
     */
    bool SundaySearch(const char *pszSrc, int iLength = -1);
    /**
     * @brief 根据buffer判断生成何种类型的msg对象
     * 
     * @param pPeerInfo [in] 当前的链接信息
     * @param pcBuffer [in] buffer
     * @param uiBufferLength [in] buffer长度
     * @param pMsgInfo [out] 生成的消息对象
     * @retval true 继续处理
     * @retval false 协议无需继续处理
     */
    virtual bool GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcBuffer, MDB_UINT32 uiBufferLength, TMdbMsgInfo*& pMsgInfo);
private:
    TMdbSundaySearch m_oSubStrSearch;///< 子串查找的算法
};

/**
 * @brief 消息头固定长度的协议
 * 
 */
class TMdbHeadFixLength:public TMdbProtocol
{
public:
    MDB_ZF_DECLARE_OBJECT(TMdbHeadFixLength);
    /**
     * @brief 构造函数
     * 
     * @param uiHeadFixLength [in] 消息头固定长度
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
     * @brief 根据buffer判断生成何种类型的msg对象
     * 
     * @param pPeerInfo [in] 当前的链接信息
     * @param pcBuffer [in] buffer
     * @param uiBufferLength [in] buffer长度
     * @param pMsgInfo [out] 生成的消息对象
     * @return bool
     * @retval true 继续处理
     * @retval false 协议无需继续处理
     */
    virtual bool GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcHeadBuffer, MDB_UINT32 uiHeadLength, TMdbMsgInfo*& pMsgInfo) = 0;
protected:
    MDB_UINT32  m_uiHeadFixLength;  ///< 消息头固定长度
    char*   m_pcHeadBuffer;     ///< 用于临时拷贝存储head使用
};

/**
 * @brief 消息头特定结束符的协议
 * 
 */
class TMdbHeadSpecTerm:public TMdbMsgSpecTerm
{
public:
    MDB_ZF_DECLARE_OBJECT(TMdbHeadSpecTerm);
    /**
     * @brief 构造函数
     * 
     * @param pszSpecTerm [in] 消息包头的结束符
     */
    TMdbHeadSpecTerm(const char* pszHeadSpecTerm);
protected:
    /**
     * @brief 根据buffer判断生成何种类型的msg对象
     * 
     * @param pPeerInfo [in] 当前的链接信息
     * @param pcBuffer [in] buffer
     * @param uiHeadLength [in] buffer长度
     * @param pMsgInfo [out] 生成的消息对象
     * @retval true 继续处理
     * @retval false 协议无需继续处理
     */
    virtual bool GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcHeadBuffer, MDB_UINT32 uiHeadLength, TMdbMsgInfo*& pMsgInfo) = 0;
protected:    
    TMdbNtcString m_sHeadSpecTerm;///< 消息包结束符
};
//}
#endif
