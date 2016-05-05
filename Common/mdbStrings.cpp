#include "Common/mdbStrings.h"
#include <stdarg.h>
#include <new>
#include <math.h>
#include "Common/mdbSysLocks.h"
#include "Common/mdbStrUtils.h"
//namespace QuickMDB
//{
#define MDB_RESERVE_STRING(iBufferSize) if(m_pszBuffer == NULL\
                                        || reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiRefcnt != 1\
                                        ||  reinterpret_cast<TStringData *>(this->m_pszBuffer-sizeof(TStringData))->uiAllocLength < iBufferSize) Reserve(iBufferSize)

        TMdbNtcString::TStringData::TStringData(unsigned int uiAllocLength /* = 0 */, unsigned int uiLength /* = 0 */, bool bWithLock /* = true */)
        {
            this->uiAllocLength = uiAllocLength;
            this->uiLength = uiLength;
            this->uiRefcnt = 1;
            if(bWithLock)
            {
                pSpinlock = new TMdbNtcThreadSpinLock;
            }
            else pSpinlock = NULL;
        }

        TMdbNtcString::TStringData::~TStringData()
        {
            if(pSpinlock)
            {
                delete pSpinlock;
                pSpinlock = NULL;
            }
        }

        int TMdbNtcString::TStringData::AddRef()
        {
            if(pSpinlock) pSpinlock->Lock();
            unsigned int uiRefcntTmp = ++uiRefcnt;
            if(pSpinlock) pSpinlock->Unlock();
            return (int)uiRefcntTmp;
        }

        unsigned int TMdbNtcString::TStringData::Release(bool bDelLock /* = true */)
        {
            if(uiRefcnt == 0) return uiRefcnt;//说明已经释放了
            //lock
            if(pSpinlock) pSpinlock->Lock();
            unsigned int uiRet = uiRefcnt;
            if (uiRefcnt > 0)
            {
                --uiRefcnt;
            }
            //unlock
            if(pSpinlock) pSpinlock->Unlock();
            if(uiRet == 1)
            {
                if(bDelLock == false)
                {
                    pSpinlock = NULL;
                }
                delete this;
            }
            return uiRet;
        }

        #define MdbGetStringData(pString) ((pString)->m_pszBuffer?reinterpret_cast<TStringData *>((pString)->m_pszBuffer-sizeof(TStringData)):NULL)

        TMdbNtcString::TMdbNtcString()
        {            
            m_pszBuffer = NULL;
            m_bLockFree = false;
            m_iDecimals = -1;
            m_cDelimiter = '\0';
        }

        TMdbNtcString::TMdbNtcString(int iBufferSize, const char* pszFormat /* = NULL */, ...)
        {
            m_pszBuffer = NULL;
            m_bLockFree = false;
            m_iDecimals = -1;
            m_cDelimiter = '\0';
            if(iBufferSize > 0)
            {
                MDB_RESERVE_STRING((MDB_UINT32)iBufferSize);
                if(m_pszBuffer && pszFormat)
                {
                    va_list ap;
                    va_start (ap, pszFormat);            
                    vsnprintf(m_pszBuffer, (MDB_UINT32)iBufferSize, pszFormat, ap);
                    va_end (ap);
                    m_pszBuffer[iBufferSize-1] = '\0';
                    MdbGetStringData(this)->uiLength = (unsigned int)strlen(m_pszBuffer);
                }
            }
        }

        TMdbNtcString::TMdbNtcString(char cSrc, int iRepeat /* = 1 */)
        {
            m_pszBuffer = NULL;
            m_bLockFree = false;
            m_iDecimals = -1;
            m_cDelimiter = '\0';
            Assign(cSrc, iRepeat);
        }

        TMdbNtcString::TMdbNtcString(const char* pszStr, int iLen /* = -1 */)
        {
            m_pszBuffer = NULL;
            m_bLockFree = false;
            m_iDecimals = -1;
            m_cDelimiter = '\0';
            Assign(pszStr, iLen);
        }


        TMdbNtcString::TMdbNtcString(const TMdbNtcString& oStr)
        {
            m_pszBuffer = NULL;
            m_bLockFree = false;
            m_iDecimals = -1;
            m_cDelimiter = '\0';
            Assign(oStr);
        }

        TMdbNtcString::~TMdbNtcString()
        {
            Release();
        }

        TMdbNtcString& TMdbNtcString::Assign(char cSrc, int iRepeat /* = 1 */)
        {
            if (iRepeat <= 0)//不合法的输入
            {
                return *this;
            }
            //当有引用，或无引用且空间不够时，需要重新分配
            if(Refcnt() > 1 || (m_pszBuffer && ((int) GetAllocLength()) < iRepeat+1))
            {
                TMdbNtcStringBuffer sTemp(cSrc, iRepeat);
                *this = sTemp;
                return *this;
            }
            else
            {
                if(m_pszBuffer == NULL)
                {
                    GrowSize((MDB_UINT32)(iRepeat+1));
                }
                memset(m_pszBuffer, cSrc, (MDB_UINT32)iRepeat);
                *(m_pszBuffer+iRepeat) = '\0';
                MdbGetStringData(this)->uiLength = (MDB_UINT32)iRepeat;                
                return *this;
            }
        }

        TMdbNtcString& TMdbNtcString::Assign(const char* pszStr, int iLen /* = -1 */)
        {
            if(pszStr == NULL || iLen == 0)
            {
                Clear();
                return *this;
            }
            else if(m_pszBuffer == pszStr && (iLen < 0 || iLen == (int)GetLength()))//如果是自己赋值给自己
            {
                return *this;
            }
            if(iLen < 0)
            {
                iLen = (int)strlen(pszStr);
            }
            //当有引用，或无引用且空间不够时，需要重新分配
            if(Refcnt() > 1 || (m_pszBuffer && ((int) GetAllocLength()) < iLen+1))
            {
                TMdbNtcStringBuffer sTemp(pszStr, iLen);
                *this = sTemp;
                return *this;
            }
            else
            {
                if(m_pszBuffer == NULL)
                {
                    GrowSize((MDB_UINT32)(iLen+1));
                }
                if(pszStr >= m_pszBuffer && pszStr < m_pszBuffer+GetLength())//如果要拷贝的字符串是自己的一部分
                {
                    memmove(m_pszBuffer, pszStr, (MDB_UINT32)iLen);
                }
                else
                {
                    memcpy(m_pszBuffer, pszStr, (MDB_UINT32)iLen);
                }
                *(m_pszBuffer+iLen) = '\0';
                MdbGetStringData(this)->uiLength = (MDB_UINT32)iLen;
                return *this;
            }
        }

        TMdbNtcString& TMdbNtcString::Assign(const TMdbNtcString& oStr)
        {
            if(this->m_pszBuffer == oStr.m_pszBuffer)//如果就是自己
            {
                return *this;
            }
            //判断对方的buffer是否没有锁
            //对方有锁的情况下，加以用没有问题，之后再赋值为无锁也没关系。
            //因为buffer引用的排斥只与buffer锁有关，而与本身是否是无锁版String无关。
            if(oStr.m_pszBuffer && MdbGetStringData(&oStr)->pSpinlock == NULL)
            {
                //对方buffer无锁的情况下，判断自己本身是否是无锁版的TString
                //如果本身是无锁版的TString，则也无需加锁
                //本身是有锁版的，则需要申请一个锁来
                if(m_bLockFree == false)
                {
                    MdbGetStringData(&oStr)->pSpinlock = new TMdbNtcThreadSpinLock;
                }
            }
            TStringData *pOldStringData = MdbGetStringData(this);
            if(oStr.m_pszBuffer)
            {
                MdbGetStringData(&oStr)->AddRef();
            }
            //此赋值一定要在Release之前，
            //因为Release时，可能有新的引用过来
            m_pszBuffer = oStr.m_pszBuffer;
            if(pOldStringData)
            {
                pOldStringData->Release();
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::Clear()
        {
            MDB_UINT32 uiLength = GetLength();
            if(uiLength > 0)
            {
                TStringData *pStringData = MdbGetStringData(this);
                if(pStringData->uiRefcnt == 1)
                {
                    memset(pStringData->GetBuffer(), 0, uiLength);
                    pStringData->uiLength = 0;
                }
                else
                {
                    //此赋值一定要在Release之前，
                    //因为Release时，可能有新的引用过来
                    m_pszBuffer = NULL;
                    pStringData->Release();
                }
            }
            return *this;
        }

        char* TMdbNtcString::GrowSize(MDB_UINT32 uiSize)
        {
            if (uiSize == 0) return m_pszBuffer;
            
            //空间够用不用申请(对于非引用情况下)
            //此处无需考虑此刻有新的引用出现，即使出现也会引用到正确的pData，此时无需加锁
            if (Refcnt() == 1 && uiSize < GetAllocLength())
            {
                return m_pszBuffer;
            }
            
            //空间不够需要计算新申请的空间
            int iAllockLength = (int)((uiSize + (MDB_UINT32)sizeof(TStringData)) & 0xFFFFFFE0) + 0x20;
            MDB_RESERVE_STRING((MDB_UINT32)iAllockLength-(MDB_UINT32)sizeof(TStringData));
            return m_pszBuffer;
        }

        unsigned int TMdbNtcString::UpdateLength(int iLength/* = -1*/)
        {
            if (0 == GetAllocLength())
            {
                return 0;
            }
            
            unsigned int uiLength = 0;
            
            if (iLength < 0)
            {
                uiLength = (unsigned int)strlen(m_pszBuffer);
            }
            else if (iLength >= (int)GetAllocLength())
            {
                uiLength = GetAllocLength()-1;
            }
            else
            {
                uiLength = (unsigned int)iLength;
            }
            *(m_pszBuffer+uiLength) = '\0';
            MdbGetStringData(this)->uiLength = uiLength;
            return uiLength;
        }

        ///预留函数
        char*  TMdbNtcString::Reserve(MDB_UINT32 uiReserveSize)
        {
            //如果容量已经达到了要预留的大小，则无需再重新分配
            if (uiReserveSize < 1 || (Refcnt() == 1 && (GetAllocLength()) >= uiReserveSize))
            {
                return m_pszBuffer;
            }
            
            MDB_UINT32 uiAllocLen = uiReserveSize + (MDB_UINT32)sizeof(TStringData);
            char* pszNewBuffer = new char[uiAllocLen];
            if (NULL == pszNewBuffer)
            {
                return m_pszBuffer;
            }
            
            pszNewBuffer[uiAllocLen-1] = '\0';
            TStringData *pNewStringData = reinterpret_cast<TStringData*>(pszNewBuffer);
            if (NULL == pNewStringData)
            {
                return m_pszBuffer;
            }
            //先以无锁来初始化
            new ((void*)pNewStringData) TStringData(uiReserveSize, GetLength(), false);
            //pNewStringData->uiAllocLength = iReserveSize;
            //pNewStringData->uiLength = ;
            //pNewStringData->uiRefcnt = 0;
            
            if (pNewStringData->uiLength > 0)
            {
                memcpy(pNewStringData->GetBuffer(), m_pszBuffer, pNewStringData->uiLength);
            }
            *(pNewStringData->GetBuffer() + pNewStringData->uiLength) = '\0';
            //memset(pNewStringData->GetBuffer() + pNewStringData->uiLength, 0, iReserveSize - pNewStringData->uiLength);
            if (m_pszBuffer)
            {
                TStringData *pOldStringData = MdbGetStringData(this);
                //此赋值一定要在Release之前，
                //因为Release时，可能有新的引用过来
                m_pszBuffer = (char*)(pNewStringData+1);
                if(!m_bLockFree)//有锁版string
                {
                    TMdbNtcThreadSpinLock* pSpinLock = pOldStringData->pSpinlock;
                    unsigned int uiRefCnt = pOldStringData->Release(false);
                    //如果没有释放了（说明spinlock还不能转移）或者pSpinLock没有初始化过
                    if(uiRefCnt != 1 || pSpinLock == NULL)
                    {
                        pNewStringData->pSpinlock = new TMdbNtcThreadSpinLock;
                    }
                    else
                    {
                        pNewStringData->pSpinlock = pSpinLock;
                    }
                }
                else//无锁版string扩容，只需要将之前的解除引用即可
                {
                    pOldStringData->Release();
                }
            }
            else
            {
                //根据自身是否为无锁版string来判断是否要初始化锁
                if(!m_bLockFree)
                {
                    pNewStringData->pSpinlock = new TMdbNtcThreadSpinLock;
                }
                m_pszBuffer = (char*)(pNewStringData+1);
            }
            return m_pszBuffer;
            
        }

        void TMdbNtcString::Release()
        {
            TStringData *pStringData = MdbGetStringData(this);
            if (pStringData)
            {
                //此赋值一定要在Release之前，
                //因为Release时，可能有新的引用过来
                m_pszBuffer = NULL;
                pStringData->Release();
            }
        }

        TMdbNtcString& TMdbNtcString::Append(char cSrc, int iRepeat /* = 1 */)
        {
            if(iRepeat <= 0)
            {
                return *this;
            }
            GrowSize(GetLength()+(MDB_UINT32)(iRepeat + 1));
            if(m_pszBuffer)
            {
                memset(m_pszBuffer + GetLength(), cSrc, (MDB_UINT32)iRepeat);
                MdbGetStringData(this)->uiLength += (MDB_UINT32)iRepeat;
                *(m_pszBuffer+GetLength()) = '\0';
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::Append(const char* pszStr, int iLen /* = -1 */)
        {
            if(pszStr == NULL)
            {
                return *this;
            }
            if(iLen < 0)
            {
                iLen = (int)strlen(pszStr);
            }
            if(iLen == 0)
            {
                return *this;
            }
            GrowSize(GetLength()+(MDB_UINT32)(iLen + 1));
            if(m_pszBuffer)
            {
                memcpy(m_pszBuffer + GetLength(), pszStr, (MDB_UINT32)iLen);
                MdbGetStringData(this)->uiLength += (MDB_UINT32)iLen;
                *(m_pszBuffer+GetLength()) = '\0';
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::Append(const TMdbNtcString& oStr)
        {
            int iSrcLength = (int)oStr.GetLength();
            if(iSrcLength <= 0)
            {
                return *this;
            }
            GrowSize(GetLength()+(MDB_UINT32)(iSrcLength + 1));
            if(m_pszBuffer)
            {
                memcpy(m_pszBuffer + GetLength(), oStr.c_str(), (MDB_UINT32)iSrcLength);
                MdbGetStringData(this)->uiLength += (MDB_UINT32)iSrcLength;
                *(m_pszBuffer+GetLength()) = '\0';
            }
            return *this;
        }

        int TMdbNtcString::Insert(int iIndex, char cTarget, int iRepeat /* = 1 */)
        {
            MDB_UINT32 uiLength = GetLength();
            if(iIndex < 0 || iIndex >= (int)uiLength)
            {
                iIndex = (int)uiLength;
            }
            if(iRepeat <= 0) return iIndex;
            GrowSize(uiLength+(MDB_UINT32)(iRepeat+1));
            if(m_pszBuffer)
            {
                //判断是否有字符需要往后移
                if(iIndex < (int)uiLength)
                {
                    memmove(m_pszBuffer+iIndex+iRepeat, m_pszBuffer+iIndex, uiLength-(MDB_UINT32)iIndex);
                }
                memset(m_pszBuffer+iIndex, cTarget, (MDB_UINT32)iRepeat);
                MdbGetStringData(this)->uiLength += (MDB_UINT32)iRepeat;
                *(m_pszBuffer+GetLength()) = '\0';
            }
            return iIndex;
        }

        int TMdbNtcString::Insert(int iIndex, const char* pszTarget, int iLen /* = -1 */)
        {
            int iLength = (int)GetLength();
            if(iIndex < 0 || iIndex >= iLength)
            {
                iIndex = iLength;
            }
            if(pszTarget == NULL)
            {
                iLen = 0;
            }
            else if(iLen < 0)
            {
                iLen = (int)strlen(pszTarget);
            }
            if(iLen == 0)
            {
                return iIndex;
            }
            GrowSize(GetLength()+(MDB_UINT32)(iLen + 1));
            if(m_pszBuffer)
            {
                //判断是否有字符需要往后移
                if(iIndex < iLength)
                {
                    memmove(m_pszBuffer+iIndex+iLen, m_pszBuffer+iIndex, (MDB_UINT32)(iLength-iIndex));
                }
                memcpy(m_pszBuffer+iIndex, pszTarget, (MDB_UINT32)iLen);
                MdbGetStringData(this)->uiLength += (MDB_UINT32)iLen;
                *(m_pszBuffer+MdbGetStringData(this)->uiLength) = '\0';
            }
            return iIndex;
        }

        int TMdbNtcString::Insert(int iIndex, const TMdbNtcString& oStr)
        {
            int iLength = (int)GetLength();
            if(iIndex < 0 || iIndex >= iLength)
            {
                iIndex = iLength;
            }
            int iSrcLength = (int)oStr.GetLength();
            if(iSrcLength <= 0)
            {
                return iIndex;
            }
            GrowSize(GetLength()+(MDB_UINT32)(iSrcLength + 1));
            if(m_pszBuffer)
            {
                //判断是否有字符需要往后移
                if(iIndex < iLength)
                {
                    memmove(m_pszBuffer+iIndex+iSrcLength, m_pszBuffer+iIndex, (MDB_UINT32)(iLength-iIndex));
                }
                memcpy(m_pszBuffer+iIndex, oStr.c_str(), (MDB_UINT32)iSrcLength);
                MdbGetStringData(this)->uiLength += (MDB_UINT32)iSrcLength;
                *(m_pszBuffer+GetLength()) = '\0';
            }
            return iIndex;
        }

        int TMdbNtcString::Remove(char cTarget)
        {
            if(m_pszBuffer == NULL)
            {
                return 0;
            }
            TMdbNtcStringBuffer sTmp;///< 定义临时string存储处理后的string
            int iIndex = -1;
            int iStart = iIndex + 1;
            while(-1 != (iIndex = Find(cTarget, iStart)))
            {
                //如果查找到的字符位置，不是开始位置
                if (iStart < iIndex)
                {
                    if (sTmp.IsEmpty())
                    {                
                        MDB_RESERVE_STRING(GetLength());
                    }
                    
                    sTmp.Append(m_pszBuffer + iStart, iIndex - iStart);
                }
                iStart = iIndex + 1;
            }
            if(iStart < (int)GetLength())
            {
                sTmp.Append(m_pszBuffer + iStart, (int)GetLength() - iStart);
            }    
            int iDelCount = 0;///< 删除的个数
            if (0 != iStart)
            {
                iDelCount = (int)(GetLength() - sTmp.GetLength());
                *this = sTmp;
            }
            
            return iDelCount;
        }

        int TMdbNtcString::Delete(int iStart, int iCount /* = 1 */)
        {
            int iLength = (int)GetLength();
            if(iStart < 0 || iStart >= (int)iLength || iCount == 0)
            {
                return 0;
            }    
            if(iCount < 0 || iCount > iLength-iStart)
            {
                iCount = iLength-iStart;
            }
            GrowSize((MDB_UINT32)(iLength+1));///< 如果只是自身引用则不会重新申请内存，为了解除引用
            if(iStart+iCount != iLength)//要删除的字符不是持续到末尾，则需要移动字符串
            {        
                memmove(m_pszBuffer+iStart, m_pszBuffer+iStart+iCount, (MDB_UINT32)(iLength-iStart-iCount)); 
                memset(m_pszBuffer+iLength-iCount, 0x00, (MDB_UINT32)iCount);//将后面的字符置为0
            }
            else
            {
                 memset(m_pszBuffer+iStart, 0x00, (MDB_UINT32)iCount);//将后面的字符置为0
            }
            MdbGetStringData(this)->uiLength -= (MDB_UINT32)iCount;
            return iCount;
        }

        void TMdbNtcString::Snprintf(int iBufferSize, const char* pszFormat, ...)
        {
            if(iBufferSize <= 0)
            {
                Clear();
                return;
            }
            MDB_RESERVE_STRING((MDB_UINT32)iBufferSize);
            if(pszFormat)
            {
                va_list ap;
                va_start (ap, pszFormat);
                vsnprintf(m_pszBuffer, (MDB_UINT32)iBufferSize, pszFormat, ap);
                va_end (ap);
                m_pszBuffer[iBufferSize-1] = '\0';
                UpdateLength();
            }
            else
            {
                UpdateLength(0);
            }
        }

        void TMdbNtcString::vSnprintf(int iBufferSize, const char* pszFormat, va_list ap)
        {
            if(iBufferSize <= 0)
            {
                Clear();
                return;
            }
            MDB_RESERVE_STRING((MDB_UINT32)iBufferSize);
            if(pszFormat)
            {
                vsnprintf(m_pszBuffer, (MDB_UINT32)iBufferSize, pszFormat, ap);
                m_pszBuffer[iBufferSize-1] = '\0';
                UpdateLength();
            }
            else
            {
                UpdateLength(0);
            }
        }

        TMdbNtcStringBuffer TMdbNtcString::Substr(int iStart, int iCount /* = -1 */) const
        {
            int iLength = (int)GetLength();
            if(iStart < 0 || iStart >= iLength || iCount == 0)
            {
                return "";
            }    
            if(iCount < 0 || iCount > iLength-iStart)
            {
                iCount = iLength-iStart;
            }
            return TMdbNtcStringBuffer(c_str()+iStart, iCount);
        }

        const char* TMdbNtcString::StrPrefix(const char* pszPrefix, bool bCase /* = true */) const
        {
            return TMdbNtcStrFunc::StrPrefix(m_pszBuffer, pszPrefix, bCase);
        }

        const char* TMdbNtcString::StrSuffix(const char* pszSuffix, bool bCase /* = true */) const
        {
            return TMdbNtcStrFunc::StrSuffix(m_pszBuffer, pszSuffix, bCase);
        }        

        int TMdbNtcString::ToInt()
        {
            return m_pszBuffer?atoi(m_pszBuffer):0;
        }

        MDB_INT64 TMdbNtcString::ToInt64()
        {
        #ifdef OS_WINDOWS
            return m_pszBuffer?_atoi64(m_pszBuffer):0;
        #else
            return m_pszBuffer?atoll(m_pszBuffer):0;
        #endif
        }

        double TMdbNtcString::ToDouble(int iPrecision /* = -1 */, bool bRounding /* = true */)
        {
            if(iPrecision < 0)
            {
                return m_pszBuffer?atof(m_pszBuffer):0;
            }
            else
            {
                if(m_pszBuffer == NULL) return 0;
                else
                {
                    double dRet = 0;
                    int iPos = Find('.');
                    if(GetLength() > (unsigned int)(iPrecision+iPos+1))
                    {
                        //判断是否要四舍五入
                        char c = at((MDB_UINT32)(iPos+iPrecision+1));
                        *(m_pszBuffer+iPos+iPrecision+1) = '\0';
                        dRet = atof(m_pszBuffer);
                        *(m_pszBuffer+iPos+iPrecision+1) = c;//恢复
                        if(bRounding && c >= '5' && c <= '9')
                        {
                            dRet += pow((double)10, (double)-iPrecision);
                        }
                    }
                    else
                    {
                        dRet = atof(m_pszBuffer);
                    }
                    return dRet;
                }        
            }
        }
        
        TMdbNtcString& TMdbNtcString::ToLower()
        {
            MDB_UINT32 uiLength = GetLength();
            if(uiLength <= 0) return *this;
            MDB_RESERVE_STRING(uiLength+1);
            if(m_pszBuffer)//仅仅是为了解引用
            {
                TMdbNtcStrFunc::ToLower(m_pszBuffer);
            }    
            return *this;
        }

        TMdbNtcString& TMdbNtcString::ToUpper()
        {
            MDB_UINT32 uiLength = GetLength();
            if(uiLength <= 0) return *this;
            MDB_RESERVE_STRING(uiLength+1);
            if(m_pszBuffer)//仅仅是为了解引用
            {
                TMdbNtcStrFunc::ToUpper(m_pszBuffer);
            }    
            return *this;
        }

        TMdbNtcString& TMdbNtcString::TrimRight(char cTarget)
        {
            MDB_UINT32 uiLength = GetLength();
            if(uiLength <= 0) return *this;
            char* pszLast = m_pszBuffer+uiLength-1;
            char* p = pszLast;
            do 
            {
                if(*p == cTarget)
                {
                    --p;
                }
                else
                {
                    if(p == pszLast) return *this;
                    MDB_UINT32 uiOffset = (MDB_UINT32)(p-m_pszBuffer);
                    MDB_RESERVE_STRING(uiLength+1);
                    if(m_pszBuffer)//仅仅是为了解引用
                    {
                        p = m_pszBuffer+uiOffset;
                        memset(p+1, 0x00, uiLength-uiOffset-1);
                        MdbGetStringData(this)->uiLength = (MDB_UINT32)(p-m_pszBuffer+1);
                    }
                    return *this;
                }
            } while (p >= m_pszBuffer);
            MDB_RESERVE_STRING(uiLength+1);//仅仅是为了解引用
            if(m_pszBuffer)
            {
                *m_pszBuffer = '\0';
                MdbGetStringData(this)->uiLength = 0;
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::TrimLeft(char cTarget)
        {
            MDB_UINT32 uiLength = GetLength();
            if(uiLength <= 0) return *this;
            char* p = m_pszBuffer, *pszEnd = m_pszBuffer+uiLength;
            do 
            {
                if(*p == cTarget)
                {
                    ++p;
                }
                else
                {
                    if(p == m_pszBuffer) return *this;
                    MDB_UINT32 uiOffset = (MDB_UINT32)(p - m_pszBuffer);
                    MDB_UINT32 uiNewLength = (MDB_UINT32)(pszEnd-p);//新长度
                    MDB_RESERVE_STRING(uiLength+1);
                    if(m_pszBuffer)//仅仅是为了解引用
                    {
                        p = m_pszBuffer + uiOffset;
                        if(uiLength > 0)
                        {
                            memmove(m_pszBuffer, p, uiNewLength);        
                        }
                        memset(m_pszBuffer+uiNewLength, 0x00, (MDB_UINT32)(p-m_pszBuffer));
                        MdbGetStringData(this)->uiLength = uiNewLength;
                    }
                    return *this;
                }
            } while (p != pszEnd);
            MDB_RESERVE_STRING(uiLength+1);//仅仅是为了解引用
            if(m_pszBuffer)
            {
                *m_pszBuffer = '\0';
                MdbGetStringData(this)->uiLength = 0;
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::Trim(char cTarget)
        {
            //先对右边处理，这样尽可能的先减少长度
            TrimRight(cTarget);
            TrimLeft(cTarget);
            return *this;
        }

        TMdbNtcString& TMdbNtcString::TrimRight(const char* pszTarget /* = " \t" */)
        {
            if(m_pszBuffer == NULL || pszTarget == NULL || *pszTarget == '\0')
            {
                return *this;
            }
            MDB_UINT32 uiLength = GetLength();
            if(uiLength == 0) return *this;
            char* pszLast = m_pszBuffer+uiLength-1;
            char* p = pszLast;
            do 
            {
                if(strchr(pszTarget, *p)) --p;
                else
                {
                    if(p == pszLast) return *this;
                    MDB_UINT32 uiOffset = (MDB_UINT32)(p-m_pszBuffer);
                    MDB_RESERVE_STRING(uiLength+1);//仅仅是为了解引用
                    if(m_pszBuffer)
                    {
                        p = m_pszBuffer+uiOffset;
                        memset(p+1, 0x00, uiLength-uiOffset-1);
                        MdbGetStringData(this)->uiLength = (MDB_UINT32)(p-m_pszBuffer+1);
                    }
                    return *this;
                }
            } while (p >= m_pszBuffer);
            MDB_RESERVE_STRING(uiLength+1);//仅仅是为了解引用
            if(m_pszBuffer)
            {
                *m_pszBuffer = '\0';
                MdbGetStringData(this)->uiLength = 0;
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::TrimLeft(const char* pszTarget /* = " \t" */)
        {
            if(m_pszBuffer == NULL || pszTarget == NULL || *pszTarget == '\0')
            {
                return *this;
            }
            MDB_UINT32 uiLength = GetLength();
            if(uiLength <= 0) return *this;
            char* p = m_pszBuffer, *pszEnd = m_pszBuffer+uiLength;
            do 
            {
                if(strchr(pszTarget, *p))
                {
                    ++p;
                }
                else
                {
                    if(p == m_pszBuffer) return *this;
                    MDB_UINT32 uiOffset = (MDB_UINT32)(p - m_pszBuffer);
                    MDB_UINT32 uiNewLength = (MDB_UINT32)(pszEnd-p);//新长度;
                    MDB_RESERVE_STRING(uiLength+1);//仅仅是为了解引用
                    if(m_pszBuffer)
                    {
                        p = m_pszBuffer + uiOffset;
                        if(uiNewLength > 0)
                        {
                            memmove(m_pszBuffer, p, uiNewLength);        
                        }
                        memset(m_pszBuffer+uiNewLength, 0x00, (MDB_UINT32)(p-m_pszBuffer));
                        MdbGetStringData(this)->uiLength = uiNewLength;
                    }
                    return *this;
                }
            } while (p != pszEnd);
            MDB_RESERVE_STRING(uiLength+1);//仅仅是为了解引用
            if(m_pszBuffer)
            {
                *m_pszBuffer = '\0';
                MdbGetStringData(this)->uiLength = 0;
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::Trim(const char* pszTarget /* = " \t" */)
        {
            //先对右边处理，这样尽可能的先减少长度
            TrimRight(pszTarget);
            TrimLeft(pszTarget);
            return *this;
        }

        TMdbNtcString& TMdbNtcString::TrimRightStr(const char* pszTarget)
        {
            if(m_pszBuffer == NULL || pszTarget == NULL || *pszTarget == '\0')
            {
                return *this;
            }
            MDB_UINT32 uiLength = GetLength();
            if(uiLength <= 0) return *this;
            MDB_UINT32 uiTargetLength = (MDB_UINT32)strlen(pszTarget);
            if(uiLength < uiTargetLength)//如果本身长度小于要去除的字符的长度，则无需比较了
            {
                return *this;
            }
            char* pszLast = m_pszBuffer+uiLength-uiTargetLength;
            char* p = pszLast;
            while(strncmp(p, pszTarget, uiTargetLength) == 0)
            {
                if(p == pszLast)
                {
                    MDB_UINT32 uiOffset = (MDB_UINT32)(p-m_pszBuffer);
                    MDB_RESERVE_STRING(uiLength+1);
                    if(m_pszBuffer == NULL)//仅仅是为了解引用，解引用失败
                    {
                        return *this;
                    }
                    else
                    {
                        p = m_pszBuffer + uiOffset;
                        pszLast = m_pszBuffer+uiLength-uiTargetLength;
                    }
                }
                memset(p, 0x00, uiTargetLength);
                uiLength -= uiTargetLength;
                if(p >= m_pszBuffer+uiTargetLength)
                {
                    p -= uiTargetLength;
                }
                else
                {
                    break;
                }
            }
            if(m_pszBuffer && uiLength != GetLength())
            {
                MdbGetStringData(this)->uiLength = uiLength;
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::TrimLeftStr(const char* pszTarget)
        {
            if(m_pszBuffer == NULL || pszTarget == NULL || *pszTarget == '\0')
            {
                return *this;
            }
            MDB_UINT32 uiLength = GetLength();
            if(uiLength <= 0) return *this;
            MDB_UINT32 uiTargetLength = (MDB_UINT32)strlen(pszTarget);
            if(uiLength < uiTargetLength)//如果本身长度小于要去除的字符的长度，则无需比较了
            {
                return *this;
            }
            char* p = m_pszBuffer, *pszEnd = m_pszBuffer+uiLength;
            while(strncmp(p, pszTarget, uiTargetLength) == 0)
            {
                p += uiTargetLength;
            }
            if(p != m_pszBuffer)
            {
                MDB_UINT32 uiOffset = (MDB_UINT32)(p - m_pszBuffer);
                MDB_UINT32 uiNewLength = (MDB_UINT32)(pszEnd-p);//新长度
                MDB_RESERVE_STRING(uiLength+1);
                if(m_pszBuffer)//仅仅是为了解引用
                {
                    p = m_pszBuffer + uiOffset;
                    if(uiNewLength > 0)
                    {
                        memmove(m_pszBuffer, p, uiNewLength);        
                    }
                    memset(m_pszBuffer+uiNewLength, 0x00, (MDB_UINT32)(p-m_pszBuffer));
                    MdbGetStringData(this)->uiLength = uiNewLength;
                }        
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::TrimStr(const char* pszTarget)
        {
            //先对右边处理，这样尽可能的先减少长度
            TrimRightStr(pszTarget);
            TrimLeftStr(pszTarget);
            return *this;
        }

        TMdbNtcString& TMdbNtcString::Replace(char cOld, char cNew)
        {
            if(cOld == '\0')
            {
                return *this;
            }
            if(m_pszBuffer)
            {
                bool bFound = false;
                char* p = m_pszBuffer;
                while (*p)
                {
                    if(*p == cOld)
                    {
                        if(bFound == false)
                        {
                            int iOffset = (int)(p-m_pszBuffer);
                            MDB_RESERVE_STRING(GetLength()+1);
                            if(m_pszBuffer)//仅仅是为了解引用
                            {
                                bFound = true;
                                p = m_pszBuffer+iOffset;
                            }
                            else
                            {
                                return *this;
                            }
                        }
                        *p = cNew;
                    }
                    ++p;
                }
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::Replace(const char* pszOld, const char* pszNew)
        {
            if(pszOld == NULL || *pszOld == '\0' || pszNew == NULL)
            {
                return *this;
            }
            int iLength = (int)GetLength();
            if(iLength == 0) return *this;
            if(m_pszBuffer)
            {
                int iOldLength = (int)strlen(pszOld);
                int iNewLength = (int)strlen(pszNew);
                //如果新旧字符串长度相等，则直接在原buffer上替换
                if(iNewLength == iOldLength)
                {
                    char* p = m_pszBuffer, *pPrev = p;
                    while(1)
                    {
                        p = strstr(pPrev, pszOld);
                        if(p == NULL)
                        {
                            break;
                        }
                        memcpy(p, pszNew, (MDB_UINT32)iNewLength);
                        p += iOldLength;
                        //如果后面的buffer长度已经不足一个oldlength，则无需再比较了
                        if(iLength-(p-m_pszBuffer+iOldLength) < iOldLength)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    TMdbNtcStringBuffer sTemp;
                    char* p = m_pszBuffer, *pPrev = p;
                    while(1)
                    {
                        p = strstr(pPrev, pszOld);
                        if(p == NULL)
                        {
                            break;
                        }
                        if(sTemp.m_pszBuffer == NULL)
                        {
                            sTemp.Reserve(GetLength()+(MDB_UINT32)iNewLength-(MDB_UINT32)iOldLength+1);
                        }
                        if(p != pPrev)
                        {
                            sTemp.Append(pPrev, (int)(p-pPrev));
                        }
                        sTemp.Append(pszNew, iNewLength);
                        p += iOldLength;
                        pPrev = p;
                        //如果后面的buffer长度已经不足一个oldlength，则无需再比较了
                        if(iLength-(p-m_pszBuffer) < iOldLength)
                        {
                            break;
                        }
                    }
                    if(*pPrev)
                    {
                        sTemp.Append(pPrev, (int)GetLength()-(int)(pPrev-m_pszBuffer));
                    }
                    //存在并赋值了
                    if(sTemp.m_pszBuffer)
                    {
                        *this = sTemp;
                    }            
                }
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::Replace(int iStart, int iCount, char cNew)
        {
            if(iCount <= 0) return *this;
            int iLength = (int)GetLength();
            if(iLength == 0) return *this;
            if(iStart < 0 || iStart >= iLength) return *this;
            MDB_RESERVE_STRING((MDB_UINT32)iLength+1);
            if(m_pszBuffer == NULL)//解引用
            {
                return *this;
            }
            *(m_pszBuffer+iStart) = cNew;
            if(iStart+iCount < iLength)
            {
                memmove(m_pszBuffer+iStart+1, m_pszBuffer+iStart+iCount, (MDB_UINT32)(iLength-iStart-iCount));
            }
            //将后续腾空的字节清空
            if(iCount > 1)
            {
                memset(m_pszBuffer+iLength-(iCount-1), 0x00, (MDB_UINT32)(iCount-1));
            }
            return *this;
        }

        TMdbNtcString& TMdbNtcString::Replace(int iStart, int iCount, const char* pszNew)
        {
            if(iCount <= 0) return *this;
            if(pszNew == NULL) return *this;
            int iLength = (int)GetLength();
            if(iLength == 0) return *this;
            if(iStart < 0 || iStart >= iLength) return *this;
            int iNewLength = (int)strlen(pszNew);
            MDB_RESERVE_STRING((MDB_UINT32)(iLength+(iNewLength>iCount? (iNewLength-iCount):0)+1));
            if(m_pszBuffer == NULL)//解引用,并看情况是否重新分配预留
            {
                return *this;
            }
            if(iStart+iCount < iLength)
            {
                memmove(m_pszBuffer+iStart+iNewLength, m_pszBuffer+iStart+iCount, (MDB_UINT32)(iLength-iStart-iCount));                
            }            
            memcpy(m_pszBuffer+iStart, pszNew, (MDB_UINT32)iNewLength);
            *(m_pszBuffer+iNewLength+iLength-iCount) = '\0';
            MdbGetStringData(this)->uiLength = (MDB_UINT32)(iLength+iNewLength-iCount);
            return *this;
        }

        int TMdbNtcString::Find(char cTarget, int iStart /* = 0 */) const
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            const char* p = strchr(pszBuffer+iStart, cTarget);
            return (int)(p?(p-pszBuffer):-1);
        }

        int TMdbNtcString::Find(const char* pszTarget, int iStart /* = 0 */) const
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            const char* p = strstr(pszBuffer+iStart, pszTarget);
            return (int)(p?(p-pszBuffer):-1);
        }

        int TMdbNtcString::ReverseFind(char cTarget, int iStart /* = -1 */) const
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) iStart = iLength-1;
            const char* pszBuffer = c_str(), *p = pszBuffer+iStart;
            do 
            {
                if(*p == cTarget)
                {
                    return (int)(p-pszBuffer);
                }
                --p;
            } while (p >= pszBuffer);
            return -1;
        }

        int TMdbNtcString::ReverseFind(const char* pszTarget, int iStart /* = -1 */) const
        {
            if(pszTarget == NULL || *pszTarget == '\0') return -1;
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) iStart = iLength-1;
            int iTargetLength = (int)strlen(pszTarget);
            if(iLength-iStart < iTargetLength) iStart = iLength-iTargetLength;
            const char* pszBuffer = c_str();
            const char *p = pszBuffer+iStart;
            while (p >= pszBuffer)
            {
                if(strncmp(p, pszTarget, (MDB_UINT32)iTargetLength) == 0)
                {
                    return (int)(p-pszBuffer);
                }
                --p;
            }
            return -1;
        }

        int TMdbNtcString::FindFirstOf(char cTarget, int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(*(pszBuffer+i) == cTarget)
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstOf(const char* pszTarget, int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(strchr(pszTarget, *(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstOfVisible(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(isgraph(*(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstOfSpace(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(isspace(*(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstOfBlank(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(*(pszBuffer+i) == 0x09 || *(pszBuffer+i) == 0x20)
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstOfAlpha(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(isalpha(*(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstOfDigit(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(isdigit(*(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstOfAlphaDigit(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(isalnum(*(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstNotOf(char cTarget, int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(*(pszBuffer+i) != cTarget)
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstNotOf(const char* pszTarget, int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(strchr(pszTarget, *(pszBuffer+i)) == NULL)
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstNotOfVisible(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(!isgraph(*(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstNotOfSpace(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(!isspace(*(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstNotOfBlank(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(*(pszBuffer+i) != 0x09 && *(pszBuffer+i) != 0x20)
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstNotOfAlpha(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(!isalpha(*(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstNotOfDigit(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(!isdigit(*(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        int TMdbNtcString::FindFirstNotOfAlphaDigit(int iStart /* = 0 */)
        {
            int iLength = (int)GetLength();
            if(iLength == 0) return -1;
            if(iStart < 0 || iStart >= iLength) return -1;
            const char* pszBuffer = c_str();
            for (int i = iStart; i < iLength; ++i)
            {
                if(!isalnum(*(pszBuffer+i)))
                {
                    return i;
                }
            }
            return -1;
        }

        void TMdbNtcString::Swap(TMdbNtcString& oStr)
        {
            char *pszData = m_pszBuffer;
            m_pszBuffer = oStr.m_pszBuffer;
            oStr.m_pszBuffer = pszData;
        }

        TMdbNtcString& TMdbNtcString::operator<<(MDB_INT16 iValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            char szValue[12]={'\0'};
            snprintf(szValue, sizeof(szValue), "%d", iValue);
            return Append(szValue);
        }
        TMdbNtcString& TMdbNtcString::operator<<(MDB_INT32 iValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            char szValue[12]={'\0'};
            snprintf(szValue, sizeof(szValue), "%d", iValue);
            return Append(szValue);
        }

        TMdbNtcString& TMdbNtcString::operator<<(MDB_INT64 iValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            char szValue[32]={'\0'};
            snprintf(szValue, sizeof(szValue), "%"MDB_NTC_ZS_FORMAT_INT64, iValue);
            return Append(szValue);
        }

        TMdbNtcString& TMdbNtcString::operator<<(MDB_UINT16 iValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            char szValue[12]={'\0'};
            snprintf(szValue, sizeof(szValue), "%u", iValue);
            return Append(szValue);
        }
        TMdbNtcString& TMdbNtcString::operator<<(MDB_UINT32 iValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            char szValue[12]={'\0'};
            snprintf(szValue, sizeof(szValue), "%u", iValue);
            return Append(szValue);
        }

        TMdbNtcString& TMdbNtcString::operator<<(MDB_UINT64 iValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            char szValue[32]={'\0'};
            snprintf(szValue, sizeof(szValue), "%"MDB_NTC_ZS_FORMAT_UINT64, iValue);
            return Append(szValue);
        }

        TMdbNtcString& TMdbNtcString::operator<<(void* pAddress)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            char szAddress[33] = {'\0'};
            snprintf(szAddress, sizeof(szAddress), "%p", pAddress);
            szAddress[sizeof(szAddress)-1] = '\0';
            return  Append(szAddress);
        }

        TMdbNtcString& TMdbNtcString::operator<<(float fValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            char szValue[32]={'\0'};
            if(m_iDecimals >= 0)
            {
                char szFormat[16]={'\0'};
                snprintf(szFormat, sizeof(szFormat), "%%.%df", m_iDecimals);
                snprintf(szValue, sizeof(szValue), szFormat, fValue);

            }
            else
            {
                snprintf(szValue, sizeof(szValue), "%f", fValue);
            }
            return Append(szValue);
        }

        TMdbNtcString& TMdbNtcString::operator<<(double dValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            char szValue[32]={'\0'};
            if(m_iDecimals >= 0)
            {
                char szFormat[16]={'\0'};
                snprintf(szFormat, sizeof(szFormat), "%%.%df", m_iDecimals);
                snprintf(szValue, sizeof(szValue), szFormat, dValue);

            }
            else
            {
                snprintf(szValue, sizeof(szValue), "%f", dValue);
            }
            return Append(szValue);
        }

        TMdbNtcString& TMdbNtcString::operator<<(const char* pszValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            return Append(pszValue);
        }

        TMdbNtcString& TMdbNtcString::operator<<(char cValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            return  Append(cValue);
        }

        TMdbNtcString& TMdbNtcString::operator<<(const TMdbNtcString& sValue)
        {
            if(m_cDelimiter && GetLength() > 0)
            {
                Append(m_cDelimiter);
            }
            return  Append(sValue);
        }

        TMdbNtcStringBuffer::TMdbNtcStringBuffer()
        {
            m_bLockFree = true;
        }
        
        TMdbNtcStringBuffer::TMdbNtcStringBuffer(char cSrc, int iRepeat /* = 1 */)
        {
            m_bLockFree = true;
            Assign(cSrc, iRepeat);
        }
        
        TMdbNtcStringBuffer::TMdbNtcStringBuffer(const char* pszStr, int iLength /* = -1 */)
        {
            m_bLockFree = true;
            Assign(pszStr, iLength);
        }
        
        TMdbNtcStringBuffer::TMdbNtcStringBuffer(int iBufferSize, const char* pszFormat /* = NULL */, ...)
        {
            m_bLockFree = true;
            va_list ap;
            va_start (ap, pszFormat);
            TMdbNtcString::vSnprintf(iBufferSize, pszFormat, ap);
            va_end (ap);
        }
        
        TMdbNtcStringBuffer::TMdbNtcStringBuffer(const TMdbNtcString& oStr)
        {
            m_bLockFree = true;
            Assign(oStr);
        }

        TMdbNtcStringBuffer::TMdbNtcStringBuffer(const TMdbNtcStringBuffer& oStr)
        {
            m_bLockFree = true;
            Assign(oStr);
        }

        TMdbNtcDataBuffer::TMdbNtcDataBuffer()
        {
            m_bLockFree = true;
        }

        TMdbNtcDataBuffer::TMdbNtcDataBuffer(char cSrc, int iRepeat /* = 1 */)
        {
            m_bLockFree = true;
            Assign(cSrc, iRepeat);
        }

        TMdbNtcDataBuffer::TMdbNtcDataBuffer(const unsigned char* pData, unsigned int uiLength)
        {
            m_bLockFree = true;
            Assign((const char*)pData, (int)uiLength);
        }

        TMdbNtcDataBuffer::TMdbNtcDataBuffer(int iBufferSize, const char* pszFormat /* = NULL */, ...)
        {
            m_bLockFree = true;
            va_list ap;
            va_start (ap, pszFormat);
            TMdbNtcString::vSnprintf(iBufferSize, pszFormat, ap);
            va_end (ap);
        }

        TMdbNtcDataBuffer::TMdbNtcDataBuffer(const TMdbNtcString& oStr)
        {
            m_bLockFree = true;
            Assign(oStr);
        }

        TMdbNtcDataBuffer::TMdbNtcDataBuffer(const TMdbNtcDataBuffer& oStr)
        {
            m_bLockFree = true;
            Assign(oStr);
        }

        TMdbNtcStringBuffer operator+(const TMdbNtcString& s1, char c)
        {
            TMdbNtcStringBuffer s(s1);
            s.Append(c);
            return s;
        }

        TMdbNtcStringBuffer operator+(const TMdbNtcString& s1, const char* s2)
        {
            TMdbNtcStringBuffer s(s1);
            s.Append(s2);
            return s;
        }

        TMdbNtcStringBuffer operator+(const TMdbNtcString& s1, const TMdbNtcString& s2)
        {
            TMdbNtcStringBuffer s(s1);
            s.Append(s2);
            return s;
        }

        TMdbNtcStringBuffer operator+(char c, const TMdbNtcString& s2)
        {
            TMdbNtcStringBuffer s(c);
            s.Append(s2);
            return s;
        }

        TMdbNtcStringBuffer operator+(const char* s1, const TMdbNtcString& s2)
        {
            TMdbNtcStringBuffer s(s1);
            s.Append(s2);
            return s;
        }
//}
