#include "Common/mdbProtocolWinntTcp.h"
//using namespace ZSmart::BillingSDK;
//namespace QuickMDB{

#define MDB_MAKEUINT32(a,b)      ((MDB_UINT32)(((MDB_UINT16)(a)) | ((MDB_UINT32)((MDB_UINT16)(b)))<<16))
#define MDB_SECRET_MASK 0x956654AD
#define  MDB_ISBIGENDIAN MdbNtcIsBigEndian()

/*** the following is used to convert  format between 
    big endian and little endian   ***/
#define MDB_M_32_SWAP(a) {                          \
    unsigned int  _tmp = a;                     \
    ((char *)&a)[0] = ((char *)&_tmp)[3];       \
    ((char *)&a)[1] = ((char *)&_tmp)[2];       \
    ((char *)&a)[2] = ((char *)&_tmp)[1];       \
    ((char *)&a)[3] = ((char *)&_tmp)[0];       \
    }

#define MDB_M_16_SWAP(a) {                          \
    unsigned short _tmp = a;                    \
    ((char *)&a)[0] = ((char *)&_tmp)[1];               \
    ((char *)&a)[1] = ((char *)&_tmp)[0];               \
    }

class TMdbZXEndian
{
public:
    static int LE_TO_BE16(unsigned char *p,const char *fmt);
    static int BE_TO_LE16(unsigned char *p,const char *fmt);
private:
    typedef struct
    {
        unsigned int pre;
        unsigned int tail;
    }SWAP32;
    static SWAP32 ms_con32[32];
};

TMdbZXEndian::SWAP32 TMdbZXEndian::ms_con32[32] ={
    {0x00000001,0x00000001},
    {0x00000002,0x00000003},
    {0x00000004,0x00000007},
    {0x00000008,0x0000000f},
    {0x00000010,0x0000001f},
    {0x00000020,0x0000003f},
    {0x00000040,0x0000007f},
    {0x00000080,0x000000ff},
    
    {0x00000100,0x000001ff},
    {0x00000200,0x000003ff},
    {0x00000400,0x000007ff},
    {0x00000800,0x00000fff},
    {0x00001000,0x00001fff},
    {0x00002000,0x00003fff},
    {0x00004000,0x00007fff},
    {0x00008000,0x0000ffff},
    
    {0x00010000,0x0001ffff},
    {0x00020000,0x0003ffff},
    {0x00040000,0x0007ffff},
    {0x00080000,0x000fffff},
    {0x00100000,0x001fffff},
    {0x00200000,0x003fffff},
    {0x00400000,0x007fffff},
    {0x00800000,0x00ffffff},
    
    {0x01000000,0x01ffffff},
    {0x02000000,0x03ffffff},
    {0x04000000,0x07ffffff},
    {0x08000000,0x0fffffff},
    {0x10000000,0x1fffffff},
    {0x20000000,0x3fffffff},
    {0x40000000,0x7fffffff},
    {0x80000000,0xffffffff},
};

/******************************************************************************
*  Function Name:  LE_TO_BE16()
*  Description  :  convert unsigned short from little-endian to big-endian. 
*  INPUT    :  will be converted char --p,bit format--fmt.
*  OUTPUT   :  if error return ,then return -1,else return 0. 
*  Author/Date  :  li.shiliang/1999-03-21 
*  Note     :
*  Modify   :  
*******************************************************************************/

int TMdbZXEndian::LE_TO_BE16(unsigned char *p,const char *fmt)
{
    MDB_UINT16 tmp,ch,loc,i=0,j=0,k=0;
    
    while (fmt[j])  { 
        if (! ((fmt[j]>='a' && fmt[j]<='g')||
            (fmt[j]>='0' && fmt[j]<='9')))
            return -1;
        
        if (fmt[j]>='a')
            k = (MDB_UINT16)(k+fmt[j]-0x57);
        else
            k = (MDB_UINT16)(k+fmt[j]-0x30);
        j++;
    }
    if (k!=16) return -1;
    
    memcpy((unsigned char *)&tmp,p,2);
    MDB_M_16_SWAP(tmp);
    ch = 0;loc = 0;
    while (fmt[i])  {
        if ( fmt[i]>='a')
            j = (MDB_UINT16)(fmt[i]-0x57);
        else
            j = (MDB_UINT16)(fmt[i]-0x30);
        loc = (MDB_UINT16)(loc+j);
        ch = (unsigned short)(ch + (tmp&ms_con32[j-1].tail)*ms_con32[16-loc].pre);
        tmp = (MDB_UINT16)(tmp>>j);
        i++;
    }
    memcpy(p,(unsigned char *)&ch,2);
    return(0);
}

/******************************************************************************
*  Function Name:  BE_TO_LE16()
*  Description  :  convert unsigned short from big-endian to little-endian. 
*  INPUT    :  will be converted short --p,bit format--fmt.
*  OUTPUT   :  if error return ,then return -1,else return 0. 
*  Author/Date  :  li.shiliang/1999-03-21 
*  Note     :
*  Modify   :  
*******************************************************************************/
int TMdbZXEndian::BE_TO_LE16(unsigned char *p,const char *fmt)
{
    MDB_UINT16 tmp,ch,loc,i=0,j=0,k=0;
    unsigned char fmt1[16] = {0};
    
    while (fmt[j])  
    { 
        if (! ((fmt[j]>='a' && fmt[j]<='g')||
            (fmt[j]>='0' && fmt[j]<='9')))
            return -1;
        if (fmt[j]>='a')
            k = (MDB_UINT16)(k+fmt[j]-0x57);
        else
            k = (MDB_UINT16)(k+fmt[j]-0x30);
        j++;
    }
    if (k!=16) return -1;
    
    memcpy(fmt1,fmt,j); 
    fmt1[j]=0;
    for (k=0;k<=(j-1)/2;k++) 
    {
        ch = fmt1[j-k-1];
        fmt1[j-k-1] = fmt1[k];
        fmt1[k] = (unsigned char)ch;
    }
    
    memcpy((unsigned char *)&tmp,p,2);
    ch = 0;loc = 0;
    while (fmt1[i]) {
        if ( fmt1[i]>='a')
            j = (MDB_UINT16)(fmt1[i]-0x57);
        else
            j = (MDB_UINT16)(fmt1[i]-0x30);
        loc = (MDB_UINT16)(loc+j);
        ch = (unsigned short)(ch + (tmp&ms_con32[j-1].tail)*ms_con32[16-loc].pre);
        tmp = (MDB_UINT16)(tmp>>j);
        i++;
    }
    MDB_M_16_SWAP(ch);
    memcpy(p,(unsigned char *)&ch,2);
    return 0;
}

bool TMdbWintcpMsgHead::CheckMsgHead() const
{
    MDB_UINT32 nTemp1, nTemp2;
    
    if((len & 0x0002) == 0)
    {
        nTemp1 = MDB_MAKEUINT32(len, event);
        nTemp1 &= MDB_SECRET_MASK;
        nTemp2 = MDB_MAKEUINT32(receiver.pno, sender.pno);
        nTemp2 = ~nTemp2;
        nTemp2 &= (~MDB_SECRET_MASK);
        nTemp1 |= nTemp2;
        
        if (secretfield != nTemp1)
            return false;
        else
            return true;
    }
    else
    {
        nTemp1 = MDB_MAKEUINT32(event, len);
        nTemp1 = ~nTemp1;
        nTemp1 &= MDB_SECRET_MASK;
        nTemp2 = MDB_MAKEUINT32(sender.pno, receiver.pno);
        nTemp2 &= (~MDB_SECRET_MASK);
        nTemp1 |= nTemp2;
        
        if (secretfield != nTemp1)
            return false;
        else
            return true;
    }
}

void TMdbWintcpMsgHead::MakeSecretHead()
{
    if (sender.module > 0 && sender.module <64)
    {
        sender.MpLeft = 1;
    }
    else if (sender.module > 63 && sender.module <129)
    {
        sender.MpLeft = 0;
    }
    MDB_UINT32 iTemp1 = 0, iTemp2 = 0;
    if ((len & 0x0002) == 0)
    {
        iTemp1 = MDB_MAKEUINT32(len, event);
        iTemp1 &= MDB_SECRET_MASK;
        iTemp2 = MDB_MAKEUINT32(receiver.pno, sender.pno);
        iTemp2 = ~iTemp2;
        iTemp2 &= (~MDB_SECRET_MASK);
        iTemp1 |= iTemp2;
        secretfield = iTemp1;
    }
    else
    {
        iTemp1 = MDB_MAKEUINT32(event, len);
        iTemp1 = ~iTemp1;
        iTemp1 &= MDB_SECRET_MASK;
        iTemp2 = MDB_MAKEUINT32(sender.pno, receiver.pno);
        iTemp2 &= (~MDB_SECRET_MASK);
        iTemp1 |= iTemp2;
        secretfield = iTemp1;
    }
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbWinntTcpProtocol, TMdbHeadFixLength);
TMdbWinntTcpProtocol::TMdbWinntTcpProtocol():TMdbHeadFixLength(sizeof(TMdbWintcpMsgHead))
{
}

void TMdbWinntTcpProtocol::HeadFromNetBuffer(TMdbWintcpMsgHead& oMsgHead, const void* pcData)
{
    if(&oMsgHead != pcData)
    {
        memcpy(&oMsgHead, pcData, sizeof(TMdbWintcpMsgHead));
    }
    //由于消息头采用的是小端字节序，所以如果当前cpu是大端字节序，则需要进行转换
    if(MDB_ISBIGENDIAN)
    {
        TMdbZXEndian::LE_TO_BE16((MDB_UINT8 *)&oMsgHead,"e11");
        TMdbZXEndian::LE_TO_BE16(((MDB_UINT8 *)&oMsgHead)+sizeof(MDB_PID),"e11");
        MDB_M_16_SWAP(oMsgHead.event);
        MDB_M_16_SWAP(oMsgHead.len);
        MDB_M_32_SWAP(oMsgHead.secretfield);
        MDB_M_32_SWAP(oMsgHead.reserve);
    }
}

void TMdbWinntTcpProtocol::HeadToNetBuffer(const TMdbWintcpMsgHead& oMsgHead, void* pcData)
{
    if(MDB_ISBIGENDIAN)
    {
        TMdbWintcpMsgHead oMsgHeadLittle = oMsgHead;
        TMdbZXEndian::BE_TO_LE16((MDB_UINT8 *)&oMsgHeadLittle,"e11");
        TMdbZXEndian::BE_TO_LE16(((MDB_UINT8 *)&oMsgHeadLittle)+sizeof(MDB_PID),"e11");
        MDB_M_16_SWAP(oMsgHeadLittle.event);
        MDB_M_16_SWAP(oMsgHeadLittle.len);
        MDB_M_32_SWAP(oMsgHeadLittle.secretfield);
        MDB_M_32_SWAP(oMsgHeadLittle.reserve);
        memcpy(pcData, &oMsgHeadLittle, sizeof(TMdbWintcpMsgHead));
    }
    else if(pcData != (void*)&oMsgHead)
    {
        memcpy(pcData, &oMsgHead, sizeof(TMdbWintcpMsgHead));
    }
}

bool TMdbWinntTcpProtocol::GenerateMsgInfo(TMdbPeerInfo* pPeerInfo, const char* pcHeadBuffer, MDB_UINT32 uiHeadLength, TMdbMsgInfo*& pMsgInfo)
{
    bool bRet = true;
    TMdbWinntTcpMsg* pWinntTcpMsg = new TMdbWinntTcpMsg;
    do
    {
        //解析消息头
        TMdbWinntTcpProtocol::HeadFromNetBuffer(pWinntTcpMsg->oMsgHead, pcHeadBuffer);
        //printf("iHeadLength[%d], bodylength[%d],MDB_ISBIGENDIAN[%d]\n", iHeadLength, pWinntTcpMsg->oMsgHead.len, MDB_ISBIGENDIAN);
        bRet = TMdbWinntTcpProtocol::CheckMsgHead(pWinntTcpMsg->oMsgHead);
        if(!bRet)
        {
            /*QuickMDB::*/TMdbNtcStringBuffer sErrorInfo;
            sErrorInfo.Snprintf(64, "CheckMsgHead failed! event[%u], bodylength[%u]\n", pWinntTcpMsg->oMsgHead.event, pWinntTcpMsg->oMsgHead.len);
            pPeerInfo->Disconnect(sErrorInfo);
            break;
        }
        pWinntTcpMsg->SetHeadLength(uiHeadLength);
        pWinntTcpMsg->SetLength(pWinntTcpMsg->oMsgHead.len + uiHeadLength);
    } while (0);
    if(bRet == false)
    {
        delete pWinntTcpMsg;
        pWinntTcpMsg = NULL;
    }
    else
    {
        pMsgInfo = pWinntTcpMsg;
    }
    return bRet;
}

MDB_PID TMdbWinntTcpHelper::ms_oSysSelfPID;///< 系统自身的pid
TMdbWinntTcpHelper::TMdbWinntTcpHelper(TMdbPeerInfo* pPeerInfo)
:TMdbPeerHelper(pPeerInfo)
{
    memset(&pid, 0x00, sizeof(pid));
}

void TMdbWinntTcpHelper::SetPID(MDB_UINT8 module, MDB_UINT8 postOffice /* = 0 */, MDB_UINT16 pno /* = 0 */)
{
    pid.module = module;
    pid.postOffice = postOffice;
    pid.pno = pno;
}

void TMdbWinntTcpHelper::SetSysSelfPID(MDB_UINT8 module, MDB_UINT8 postOffice /* = 0 */, MDB_UINT16 pno /* = 0 */)
{
    ms_oSysSelfPID.module = module;
    ms_oSysSelfPID.postOffice = postOffice;
    ms_oSysSelfPID.pno = pno;
}

bool TMdbWinntTcpHelper::SendPacket(MDB_UINT8 type, MDB_UINT16 event, const void* pValue, MDB_UINT16 uiLength, MDB_PID receiver, MDB_PID sender)
{
    bool bRet = true;
    if(pValue == NULL)
    {
        uiLength = 0;
    }
    TMdbWintcpMsgHead oMsgHead;
    oMsgHead.type = type;
    oMsgHead.receiver = receiver;
    oMsgHead.sender = sender;
    oMsgHead.event = event;
    oMsgHead.len = uiLength;
    oMsgHead.MakeSecretHead();
    TMdbWinntTcpProtocol::HeadToNetBuffer(oMsgHead, &oMsgHead);
    bRet &= pPeerInfo->PostMessage(&oMsgHead, sizeof(TMdbWintcpMsgHead));
    if(uiLength > 0) bRet &= pPeerInfo->PostMessage(pValue, uiLength);
    return bRet;
}

//}
