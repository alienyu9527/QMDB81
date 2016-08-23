#include "Common/mdbPeerProtocol.h"
#include "Common/mdbNtcEngine.h"
//namespace QuickMDB
//{
//using namespace QuickMDB;
TMdbSundaySearch::TMdbSundaySearch(const char* pszPattern)
{
    SetPattern(pszPattern);    
}

void TMdbSundaySearch::SetPattern(const char* pszPattern)
{
    // construct delta shift table
    m_sPattern = pszPattern;
    int m = (int)m_sPattern.GetLength();
    for (int i = 0; i < 256; i++)
    {
        m_iShiftTable[i] = m + 1;
    }
    const char* p = pszPattern;
    while(*p)
    {
        m_iShiftTable[(MDB_UINT8)(*p)] = m - (int)(p-pszPattern);
        ++p;
    }
}

int TMdbSundaySearch::Search(const char* pszSrc, int iLength /* = -1 */, bool bComparePartly /* = false */) const
{
    // get the length of the text and the pattern, if necessary    
    int m = (int)m_sPattern.GetLength();
    if (m == 0)
    {
        return 0;//pszSrc;
    }
    if (iLength < 0)
    {
        iLength = (int)strlen(pszSrc);
    }
    const unsigned char *patt = (const unsigned char*)m_sPattern.c_str(), *patt_end = patt+m;
    // start searching...
    const unsigned char *t = NULL, *tx = (const unsigned char*)pszSrc, *tx_end = (const unsigned char*)(pszSrc+iLength), *p = NULL;
    
    while (tx + m <= tx_end)
    {
        for (p = patt, t = tx; p != patt_end && t != tx_end; ++p, ++t)
        {
            if (*p != *t)  // found a mismatch
            {
                break;
            }
        }
        if (p == patt_end)   // Yes! we found it!
        {
            return (int)(tx-(const unsigned char*)pszSrc);//tx;
        }
        tx += m_iShiftTable[tx[m]];  // move the pattern by a distance
    }
    
    /*
    const char* pFind = strstr(pszSrc, (const char*)patt);
    if(pFind)
    {
        return pFind-pszSrc;
    }
    */
    if(bComparePartly)
    {
        if(iLength >= m)
        {
            tx = (const unsigned char*)(pszSrc+iLength-m);
        }
        else
        {
            tx = (const unsigned char*)(pszSrc);
        }
        
        do 
        {
            for (p = patt, t = tx; p != patt_end && t != tx_end; ++p, ++t)
            {
                if (*p != *t)  // found a mismatch
                {
                    break;
                }
            }
            if (p == patt_end || t == tx_end)   // Yes! we found it!
            {
                return (int)(tx-(const unsigned char*)pszSrc);//(char*)tx;
            }
            else
            {
                ++tx;
            }
        } while (tx < tx_end);        
    }
    /*
        do
        {
            for (p = patt, t = tx; p != patt_end && t != tx_end; ++p, ++t)
            {
                if (*p != *t)  // found a mismatch
                {
                    break;
                }
            }
            if (p == patt_end || t == tx_end)   // Yes! we found it!
            {
                return tx-(const unsigned char*)pszSrc;//(char*)tx;
            }
            else if(tx+m_iShiftTable[tx[m]] < tx_end)
            {
                tx += m_iShiftTable[tx[m]];  // move the pattern by a distance            
            }
            else if(tx+1 < tx_end)
            {
                ++tx;
            }
            else
            {
                break;
            }
        }while(1);
    */
    return -1;
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbProtocol, TMdbNtcBaseObject);
bool TMdbProtocol::PreTranslateMessage(TMdbPeerInfo* pPeerInfo, TMdbMsgInfo*& pMsgInfo)
{
    return false;
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbMsgFixLength, TMdbProtocol);
void TMdbMsgFixLength::CheckPackets(TMdbPeerInfo* pPeerInfo)
{
    if(pPeerInfo->oRecvPackets.GetTotalLength() < m_uiFixLength)
    {
        return;//还不够消息头的长度
    }
}

bool TMdbMsgFixLength::GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcBuffer, MDB_UINT32 uiBufferLength, TMdbMsgInfo*& pMsgInfo)
{
    if(uiBufferLength < m_uiFixLength)
    {
        return false;
    }
    pMsgInfo = new TMdbMsgInfo;
    pMsgInfo->SetLength(uiBufferLength);
    return true;
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbMsgSpecTerm, TMdbProtocol);
TMdbMsgSpecTerm::TMdbMsgSpecTerm(const char* pszSpecTerm):m_oSubStrSearch(pszSpecTerm)
{
}

void TMdbMsgSpecTerm::SetSpecTerm(const char* pszSpecTerm)
{
    m_oSubStrSearch.SetPattern(pszSpecTerm);
}

void TMdbMsgSpecTerm::CheckPackets(TMdbPeerInfo* pPeerInfo)
{
    bool bRet = true;
    TMdbRecvPackets& oRecvPackets = pPeerInfo->oRecvPackets;
    TMdbSpecTermMatchInfo& oSpecTermMatchInfo = oRecvPackets.oSpecTermMatchInfo;    
    TMdbNtcBaseList::iterator itor = pPeerInfo->oRecvPackets.oSpecTermMatchInfo.itLastMatchPacket;
    if(itor.pNodeObject == NULL)
    {
        itor = pPeerInfo->oRecvPackets.IterBegin();
    }
    else
    {        
        ++itor;//移到下一个进行判断
    }
    MDB_UINT32 uiMsgLength = pPeerInfo->oRecvPackets.oSpecTermMatchInfo.uiLastMatchMsgLength;
    bool bGenerateMsg = false;    
    int iSplitPos = -1;
    TMdbMsgInfo* pSplicingMsg = NULL;
    while (itor.pNodeObject)
    {
        MDB_UINT32 uiSpecTermLength = m_oSubStrSearch.GetPattern().GetLength();
        TMdbPacketInfo* pPacketInfo = static_cast<TMdbPacketInfo*>(itor.data());        
        //MDB_NTC_DEBUG("packet length[%d], iLastMatchCount[%d]", pPacketInfo->GetLength(), oSpecTermMatchInfo.iLastMatchCount);
        if(oSpecTermMatchInfo.uiLastMatchCount > 0)
        {
            if(strncmp(pPacketInfo->GetBuffer(), m_oSubStrSearch.GetPattern().c_str()+oSpecTermMatchInfo.uiLastMatchCount,
                uiSpecTermLength-oSpecTermMatchInfo.uiLastMatchCount) == 0)
            {
                //说明找到了终止符，可以直接生成消息包
                uiMsgLength += uiSpecTermLength-oSpecTermMatchInfo.uiLastMatchCount;
                bGenerateMsg = true;
            }
            else
            {
                oSpecTermMatchInfo.uiLastMatchCount = 0;
            }
        }
        //经过上面预先匹配后，未找到满足的，则走常规查找
        if(bGenerateMsg == false)
        {
            //应该使用sunday算法
            //MDB_NTC_DEBUG("search begin");
            if(uiSpecTermLength == 1)
            {
                const char* pcSplit = strchr(pPacketInfo->GetBuffer(), m_oSubStrSearch.GetPattern()[0]);
                if(pcSplit) iSplitPos = (int)(pcSplit-pPacketInfo->GetBuffer());
                else iSplitPos = -1;
            }
            else
            {
                iSplitPos = m_oSubStrSearch.Search(pPacketInfo->GetBuffer(), (int)pPacketInfo->GetLength(), true);
            }
            //MDB_NTC_DEBUG("search end");
            if(iSplitPos >= 0)
            {
                if(pPacketInfo->GetLength() >= (MDB_UINT32)(iSplitPos)+uiSpecTermLength)//长度够终止符长度的了
                {
                    uiMsgLength += (MDB_UINT32)(iSplitPos) + uiSpecTermLength;
                    bGenerateMsg = true;
                }
                else
                {
                    uiMsgLength += pPacketInfo->GetLength();
                    oSpecTermMatchInfo.itLastMatchPacket = itor;
                    oSpecTermMatchInfo.uiLastMatchCount = pPacketInfo->GetLength()-(MDB_UINT32)iSplitPos;
                    oSpecTermMatchInfo.uiLastMatchMsgLength = uiMsgLength;
                    ++itor;
                }                
            }
            else
            {
                uiMsgLength += pPacketInfo->GetLength();
                oSpecTermMatchInfo.itLastMatchPacket = itor;
                oSpecTermMatchInfo.uiLastMatchMsgLength = uiMsgLength;
                ++itor;
            }
        }
        /*
        MDB_NTC_DEBUG("iMsgLength:[%d], MatchCount[%d], LastMatchMsgLength[%d]", iMsgLength, oSpecTermMatchInfo.iLastMatchCount,
            oSpecTermMatchInfo.iLastMatchMsgLength);
        MDB_NTC_DEBUG("iOffsetBytes:%d, iLength:%d, iSplitPos:%d %s", pPacketInfo->iOffset, pPacketInfo->iLength,
                    iSplitPos, iSplitPos>=0?"found":"not found");
        */
        if(bGenerateMsg)
        {
            bGenerateMsg = false;//重置为false
            oSpecTermMatchInfo.oSpecTermPacket.SetLength(uiMsgLength);
            oRecvPackets.SplicePacket(uiMsgLength, oSpecTermMatchInfo.oSpecTermPacket);
            bRet = GenerateMsgInfo(pPeerInfo, oSpecTermMatchInfo.oSpecTermPacket.GetBuffer(), uiMsgLength, pSplicingMsg);            
            if(bRet)
            {
                if(pSplicingMsg)
                {
                    if(pSplicingMsg->GetLength() > uiMsgLength)//拷贝部分 oSpecTermPa
                    {
                        pSplicingMsg->AllocBuffer(pSplicingMsg->GetLength());
                        pPeerInfo->oRecvPackets.uiSplicedLength = oSpecTermMatchInfo.oSpecTermPacket.GetLength();
                        memcpy(pSplicingMsg->GetBuffer(), oSpecTermMatchInfo.oSpecTermPacket.GetBuffer(), pPeerInfo->oRecvPackets.uiSplicedLength);
                    }
                    else//复用 oSpecTermPacket
                    {
                        pSplicingMsg->AttachPacket(oSpecTermMatchInfo.oSpecTermPacket);
                        pPeerInfo->oRecvPackets.uiSplicedLength = oSpecTermMatchInfo.oSpecTermPacket.GetLength();
                    }
                    pPeerInfo->oRecvPackets.pSplicingMsg = pSplicingMsg;
                    pSplicingMsg = NULL;
                    /*
                    else if(pSplicingMsg->pcData != oSpecTermMatchInfo.oSpecTermPacket.pcData)
                    {
                        oSpecTermMatchInfo.oSpecTermPacket.Release();
                    }
                    */
                    oSpecTermMatchInfo.Reset();
                    if(pPeerInfo->SpliceMsg())
                    {
                        if(oRecvPackets.GetTotalLength() > 0 && pPeerInfo->pProtocol == this)
                        {
                            uiMsgLength = 0;
                            itor = oRecvPackets.IterBegin();
                            continue;
                        }
                    }
                }
                else
                {
                    oSpecTermMatchInfo.Reset();
                }
            }
            break;
        }
    }
}

bool TMdbMsgSpecTerm::GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcBuffer, MDB_UINT32 uiBufferLength, TMdbMsgInfo*& pMsgInfo)
{
    pMsgInfo = new TMdbMsgInfo;
    pMsgInfo->SetLength(uiBufferLength);
    return true;
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbHeadFixLength, TMdbProtocol);
TMdbHeadFixLength::TMdbHeadFixLength(MDB_UINT32 uiHeadFixLength)
{
    MDB_NTC_ZF_ASSERT(uiHeadFixLength > 0);
    m_uiHeadFixLength = uiHeadFixLength;
    m_pcHeadBuffer = new char[uiHeadFixLength];
    memset(m_pcHeadBuffer, 0x00, uiHeadFixLength);
}

TMdbHeadFixLength::~TMdbHeadFixLength()
{
    if(m_pcHeadBuffer)
    {
        delete []m_pcHeadBuffer;
        m_pcHeadBuffer = NULL;
    }
}

void TMdbHeadFixLength::CheckPackets(TMdbPeerInfo* pPeerInfo)
{
    TMdbRecvPackets& oRecvPackets = pPeerInfo->oRecvPackets;
    if(oRecvPackets.GetTotalLength() < m_uiHeadFixLength)
    {
        return;//还不够消息头的长度
    }
    TMdbMsgInfo* pSplicingMsg = NULL;
    bool bRet = true;
    MDB_UINT32 uiCurLength = 0;
    TMdbNtcBaseList::iterator itor = oRecvPackets.IterBegin();
    while (itor.pNodeObject)
    {
        TMdbPacketInfo* pPacketInfo = static_cast<TMdbPacketInfo*>(itor.data());
        if(uiCurLength+pPacketInfo->GetLength() >= m_uiHeadFixLength)
        {
            if(uiCurLength == 0)
            {
                bRet = GenerateMsgInfo(pPeerInfo, pPacketInfo->GetBuffer(), m_uiHeadFixLength, pSplicingMsg);
            }
            else
            {
                memcpy(m_pcHeadBuffer+uiCurLength, pPacketInfo->GetBuffer(), m_uiHeadFixLength-uiCurLength);
                uiCurLength = 0;
                bRet = GenerateMsgInfo(pPeerInfo, m_pcHeadBuffer, m_uiHeadFixLength, pSplicingMsg);
            }
            if(bRet)
            {
                if(pSplicingMsg)
                {
                    pSplicingMsg->SetHeadLength(m_uiHeadFixLength);
                    oRecvPackets.pSplicingMsg = pSplicingMsg;
                    pSplicingMsg = NULL;
                    if(pPeerInfo->SpliceMsg() && oRecvPackets.GetTotalLength() >= m_uiHeadFixLength && pPeerInfo->pProtocol == this)
                    {
                        itor = oRecvPackets.IterBegin();
                        continue;
                    }
                }
            }
            break;
        }
        else
        {
            //拼接
            memcpy(m_pcHeadBuffer+uiCurLength, pPacketInfo->GetBuffer(), pPacketInfo->GetLength());
            uiCurLength += pPacketInfo->GetLength();
            ++itor;
        }
    }
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbHeadSpecTerm, TMdbMsgSpecTerm);
TMdbHeadSpecTerm::TMdbHeadSpecTerm(const char* pszHeadSpecTerm)
    :TMdbMsgSpecTerm(pszHeadSpecTerm)
{
}
//}
//_NTC_END
