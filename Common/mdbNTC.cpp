#include "Common/mdbNTC.h"
//namespace QuickMDB
//{
mdb_ntc_thread_local(TMdbNtcStringBuffer*, ntc_errstr_t);
//using namespace QuickMDB;
TMdbNtcThreadLock g_oMdbNtcSharedPtrSpinLock;///< ÓÃ×÷shared_ptrµÄ»¥³â
TMdbNtcStringBuffer MdbNtcFormatHexString(const void* pcBuffer, unsigned int uiLength)
{
    const unsigned char* pszBuffer = (const unsigned char*)pcBuffer;
    unsigned int uiOutBufferLength = uiLength*3+uiLength/16+100;
    TMdbNtcStringBuffer sBuffer((int)uiOutBufferLength);
    char* szOutputBuffer = sBuffer.GetBuffer();
    memset(szOutputBuffer, 0x00, uiOutBufferLength);
    unsigned int i = 0, iOffset = 0;
    strncpy(szOutputBuffer+iOffset, "01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16\n"\
                                    "-----------------------------------------------\n",
                                    96);
    iOffset += 96;
    const char* pszOut = szOutputBuffer+iOffset;
    for (i = 0; i < uiLength; i++)
    {        
        if(i%16 == 15)
        {
            snprintf(szOutputBuffer+iOffset, uiOutBufferLength-iOffset, "%02X\n", *(pszBuffer+i));
            pszOut = szOutputBuffer+iOffset+3;
        }
        else
        {
            snprintf(szOutputBuffer+iOffset, uiOutBufferLength-iOffset, "%02X ", *(pszBuffer+i));
        }
        iOffset += 3;
    }
    if(i%16 != 15)
    {
        *(szOutputBuffer+iOffset++) = '\n';
    }
    *(szOutputBuffer+iOffset) = '\0';
    sBuffer.UpdateLength((int)iOffset);
    return sBuffer;
}

void TMdbIdMgr::InitRes(int* pResArray, MDB_UINT32 uiStart, MDB_UINT32 uiEnd)
{
    for (MDB_UINT32 i = uiStart; i < uiEnd; ++i)
    {
        pResArray[i] = (int)i;
    }
}

TMdbNtcAttrInfo::TMdbNtcAttrInfo(TMdbNtcStringBuffer sName, TMdbNtcStringBuffer sValue)
{
    this->sName = sName;
    this->sValue = sValue;
}

TMdbNtcStringBuffer TMdbNtcAttrInfo::ToString() const
{
    return sName+"=["+sValue+"]";
}

MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcAttrMgr, TMdbNtcBaseObject);
TMdbNtcAttrMgr::TMdbNtcAttrMgr(bool bCaseSentive /* = true */):m_oAttrMap(new TMdbNtcAttrCompare(bCaseSentive))
{
    m_oAttrMap.SetAutoRelease(true);
}

TMdbNtcAttrMgr::~TMdbNtcAttrMgr()
{
    m_oAttrMap.Clear();
}

TMdbNtcStringBuffer TMdbNtcAttrMgr::ToString() const
{
    TMdbNtcStringBuffer sRet;
    TMdbNtcAttrInfo* pAttrInfo = NULL;
    TMdbNtcAvlTree::iterator itor = m_oAttrMap.IterBegin(), itor_end = m_oAttrMap.IterEnd();
    for (; itor != itor_end; ++itor)
    {
        pAttrInfo = static_cast<TMdbNtcAttrInfo*>(itor.data());
        sRet<<'['<<pAttrInfo->sName<<"]="<<pAttrInfo->sValue<<'\n';
    }
    if(!sRet.IsEmpty())
    {
        sRet.Delete((int)(sRet.GetLength()-1));
    }
    return sRet;
}

TMdbNtcAttrInfo* TMdbNtcAttrMgr::AddAttrInfo(TMdbNtcStringBuffer sName, TMdbNtcStringBuffer sValue)
{
    TMdbNtcAttrInfo* pAttrInfo = new TMdbNtcAttrInfo(sName, sValue);
    m_oAttrMap.Add(pAttrInfo);
    return pAttrInfo;
}

bool TMdbNtcAttrMgr::DelAttrInfo(TMdbNtcStringBuffer sName)
{
    TMdbNtcAvlTree::iterator itor = m_oAttrMap.IterFind(TMdbNtcAttrInfo(sName));
    if(itor != m_oAttrMap.IterEnd())
    {
        m_oAttrMap.IterErase(itor);
        return true;
    }
    else
    {
        return false;
    }
}

TMdbNtcAttrInfo* TMdbNtcAttrMgr::GetAttrInfo(TMdbNtcStringBuffer sName) const
{
    return static_cast<TMdbNtcAttrInfo*>(m_oAttrMap.FindData(TMdbNtcAttrInfo(sName)));
}

TMdbNtcStringBuffer TMdbNtcAttrMgr::AttrValueAsString(TMdbNtcStringBuffer sName) const
{
    TMdbNtcAttrInfo* pAttrInfo = GetAttrInfo(sName);
    return pAttrInfo?pAttrInfo->ValueAsString():TMdbNtcStringBuffer();
}

MDB_INT64 TMdbNtcAttrMgr::AttrValueAsInterger(TMdbNtcStringBuffer sName) const
{
    TMdbNtcAttrInfo* pAttrInfo = GetAttrInfo(sName);
    return pAttrInfo?pAttrInfo->ValueAsInterger():0;
}

double TMdbNtcAttrMgr::AttrValueAsFloat(TMdbNtcStringBuffer sName) const
{
    TMdbNtcAttrInfo* pAttrInfo = GetAttrInfo(sName);
    return pAttrInfo?pAttrInfo->ValueAsFloat():0;
}

void TMdbNtcAttrMgr::SetAttrValue(TMdbNtcStringBuffer sName, TMdbNtcStringBuffer sValue)
{
    TMdbNtcAttrInfo* pAttrInfo = GetAttrInfo(sName);
    if(pAttrInfo)
    {
        pAttrInfo->sValue = sValue;
    }
    else
    {
        AddAttrInfo(sName, sValue);
    }
}

void TMdbNtcAttrMgr::SetAttrValue(TMdbNtcStringBuffer sName, MDB_INT64 iValue)
{
    TMdbNtcStringBuffer sValue(32, "%"MDB_NTC_ZS_FORMAT_INT64, iValue);
    SetAttrValue(sName, sValue);
}

void TMdbNtcAttrMgr::SetAttrValue(TMdbNtcStringBuffer sName, MDB_UINT64 uiValue)
{
    TMdbNtcStringBuffer sValue(32, "%"MDB_NTC_ZS_FORMAT_UINT64, uiValue);
    SetAttrValue(sName, sValue);
}

void TMdbNtcAttrMgr::SetAttrValue(TMdbNtcStringBuffer sName, MDB_INT32 iValue)
{
    TMdbNtcStringBuffer sValue(32, "%d", iValue);
    SetAttrValue(sName, sValue);
}

void TMdbNtcAttrMgr::SetAttrValue(TMdbNtcStringBuffer sName, MDB_UINT32 uiValue)
{
    TMdbNtcStringBuffer sValue(32, "%u", uiValue);
    SetAttrValue(sName, sValue);
}

void TMdbNtcAttrMgr::SetAttrValue(TMdbNtcStringBuffer sName, double dValue)
{
    TMdbNtcStringBuffer sValue(32, "%f", dValue);
    SetAttrValue(sName, sValue);
}
//}
