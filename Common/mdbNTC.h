/**
 * @file mdbNTC.h
 * NTCͷ�ļ�
 * 
 * @author jiang.jinzhou@zte.com.cn
 * @version 1.0
 * @date 2012/08/28
 * @bug �½�����bug
 * @bug ��Ԫ���ԣ�δ����bug
 * @warning 
 */
#ifndef _MDB_NTC_COMMON_H_
#define _MDB_NTC_COMMON_H_
#include "Common/mdbSysSockets.h"
#include "Common/mdbSysLocks.h"
#include "Common/mdbDateUtils.h"
#include "Common/mdbSysThreads.h"
//#include "Common/mdbLogInterface.h"

//namespace QuickMDB{
/*QuickMDB::*/TMdbNtcStringBuffer MdbNtcFormatHexString(const void* pcBuffer, unsigned int uiLength);
mdb_ntc_extern_thread_local(/*QuickMDB::*/TMdbNtcStringBuffer*, ntc_errstr_t);
inline /*QuickMDB::*/TMdbNtcStringBuffer& mdb_ntc_errstr_fun()
{
    /*QuickMDB::*/TMdbNtcStringBuffer* pBuffer = ntc_errstr_t.Value();
    if(pBuffer) return *pBuffer;
    else
    {
        ntc_errstr_t = pBuffer = new /*QuickMDB::*/TMdbNtcStringBuffer;
        /*QuickMDB::*/mdb_ntc_zthread_cleanup_push(pBuffer);
        return *pBuffer;
    }
}
#define mdb_ntc_errstr mdb_ntc_errstr_fun()

//֮���Բ�����shared_ptr�ľ�̬��Ա������Ϊshared_ptrΪģ�壬�ڲ�ͬ��������չ�������ڶ��ģ���д��ڶ��
extern /*QuickMDB::*/TMdbNtcThreadLock g_oMdbNtcSharedPtrSpinLock;///< ����shared_ptr�Ļ���
template<class _Ty>
class TMdbSharedPtr
{
public:
    TMdbSharedPtr(_Ty* pObj = NULL):m_pcData(NULL),m_pRefCnt(NULL)
    {
        if(pObj)
        {
            Attach(pObj);
        }
    }
    TMdbSharedPtr(const TMdbSharedPtr<_Ty>& ptrRef):m_pcData(NULL),m_pRefCnt(NULL)
    {
        Attach(ptrRef);
    }
    ~TMdbSharedPtr()
    {
        if(m_pcData)
        {
            Release();
        }
    }
    inline unsigned int RefCnt()
    {
        return m_pRefCnt?*m_pRefCnt:0;
    }
    inline operator _Ty*()
    {
        return Value();
    }
    _Ty& operator*() const
    {
        return *m_pcData;
    }
    _Ty* operator ->()
    {
        return Value();
    }

#if defined(OS_WINDOWS) && _MSC_VER < 1300//���VC6
    //Ϊ��֧��win32�µ�NULL==TMdbSharedPtr�ıȽ�
    friend bool operator == (const _Ty* pObj, TMdbSharedPtr<_Ty>& ptrRef)
    {
        return pObj==ptrRef.Value();
    }
    //Ϊ��֧��win32�µ�NULL!=TMdbSharedPtr�ıȽ�
    friend bool operator != (const _Ty* pObj, TMdbSharedPtr<_Ty>& ptrRef)
    {
        return pObj!=ptrRef.Value();
    }
    /*����ʵ�֣��ѱ�operator _Ty*()֧��
    bool operator == (const _Ty* pObj)
    {
        return pObj == Value();
    }
    bool operator!()
    {
        return Value() == NULL;
    }
    */
#endif
    inline TMdbSharedPtr<_Ty>& operator = (const TMdbSharedPtr<_Ty>& ptrRef)
    {
        return Attach(ptrRef);
    }
    inline TMdbSharedPtr<_Ty>& operator = (const _Ty* pObj)
    {
        return Attach(pObj);
    }
    inline _Ty* Value() const
    {
        return m_pcData;
    }
protected:
    TMdbSharedPtr<_Ty>& Attach(const _Ty* pObj)
    {        
        if(m_pcData)
        {
            Release();
        }
        if(pObj)
        {
            m_pRefCnt = new unsigned int(1);
            m_pcData = const_cast<_Ty*>(pObj);
        }
        return *this;
    }
    TMdbSharedPtr<_Ty>& Attach(const TMdbSharedPtr<_Ty>& ptrRef)
    {
        if(m_pRefCnt != ptrRef.m_pRefCnt)//�ж��Ƿ��ǲ�ͬ������
        {
            if(m_pcData) Release();
            //����
            if(ptrRef.m_pcData)
            {
                g_oMdbNtcSharedPtrSpinLock.Lock();
                if(ptrRef.m_pcData)
                {
                    memcpy(this, &ptrRef, sizeof(TMdbSharedPtr<_Ty>));
                    ++*m_pRefCnt;
                }
                g_oMdbNtcSharedPtrSpinLock.Unlock();
            }
        }
        return *this;
    }
    unsigned int Release()
    {
        unsigned int uiRefCnt = 0;
        if(m_pcData)
        {
            _Ty* pDelValue = NULL;
            g_oMdbNtcSharedPtrSpinLock.Lock();
            uiRefCnt = --*m_pRefCnt;
            if(uiRefCnt == 0)
            {
                pDelValue = m_pcData;
            }
            m_pcData = NULL;
            g_oMdbNtcSharedPtrSpinLock.Unlock();
            //���ͷŲ���������󣬲���������
            if(uiRefCnt == 0)
            {
                delete pDelValue;
                pDelValue = NULL;
                delete m_pRefCnt;
                m_pRefCnt = NULL;
            }
            else
            {
                m_pRefCnt = NULL;
            }
        }
        return uiRefCnt;
    }
protected:
    _Ty* m_pcData;
    unsigned int* m_pRefCnt;///< ָ�����ü���
};

/**
 * @brief ��Դ��
 * 
 */
template<class _Ty>
class TMdbResPool
{
public:
    TMdbResPool();
    virtual ~TMdbResPool()
    {
        if(m_pResArray)
        {
            UninitRes(m_pResArray, 0, m_uiPoolSize);
            delete []m_pResArray;        
            m_pResArray = NULL;
        }
    }
    /**
     * @brief �����Դ�������Դ��
     * 
     * @return MDB_UINT32
     * @retval �����Դ��
     */
    inline MDB_UINT32 GetPoolSize()
    {
        return m_uiPoolSize;
    }
    /**
     * @brief �������(>=m_iPoolSize)
     * 
     * @return MDB_UINT32
     * @retval ����
     */
    inline MDB_UINT32 GetPoolCapacity()
    {
        return m_iPoolCapacity;
    }
    /**
     * @brief ������Դ�صĴ�С
     * 
     * @param uiPoolSize [in] ��Դ�صĴ�С
     * @return ��
     */
    void SetPoolSize(MDB_UINT32 uiPoolSize);
    /**
     * @brief ����һ����Դ
     * 
     * @return bool
     * @retval true �ɹ�
     */
    bool AllocRes(_Ty& res);
    /**
     * @brief �ͷ���Դ
     * 
     * @param res [in] Ҫ�ͷŵ���Դ
     * @return ��
     */
    void FreeRes(_Ty res);
    /**
     * @brief ������ڱ�ʹ�õ���Դ��
     *      
     * @return int
     * @retval ���ڱ�ʹ�õ���Դ��
     */
    inline MDB_UINT32 GetUsedSize()
    {
        //return m_iTailCursor>=m_iHeadCursor?(m_iPoolSize-(m_iTailCursor-m_iHeadCursor)):(m_iHeadCursor-m_iTailCursor);
        return m_uiAllocTimes>=m_uiFreeTimes?(m_uiAllocTimes-m_uiFreeTimes):(m_uiAllocTimes+(/*QuickMDB::*/MDB_NTC_ZS_MAX_UINT32-m_uiFreeTimes));
    }
    /**
     * @brief ��ÿ��е���Դ��
     *      
     * @return int
     * @retval ���е���Դ��
     */
    inline MDB_UINT32 GetIdleSize()
    {
        return m_uiPoolSize-GetUsedSize();
    }
protected:
    /**
     * @brief ��ʼ����Դ�ĺ���, Ԫ�ط�Χ[uiStart, uiEnd)
     * 
     * @param pResArray [in] ��Դ����
     * @param uiStart   [in] ��ʼλ��
     * @param uiEnd     [in] ����λ��
     * @return ��
     */
    virtual void InitRes(_Ty* pResArray, MDB_UINT32 uiStart, MDB_UINT32 uiEnd)
    {
    }
    /**
     * @brief ������Դ�ĺ���, Ԫ�ط�Χ[uiStart, uiEnd)
     * 
     * @param pResArray [in] ��Դ����
     * @param uiStart   [in] ��ʼλ��
     * @param uiEnd     [in] ����λ��
     * @return ��
     */
    virtual void UninitRes(_Ty* pResArray, MDB_UINT32 uiStart, MDB_UINT32 uiEnd)
    {
    }
protected:
    _Ty*    m_pResArray;///< ��Դ������
    MDB_UINT32  m_iPoolCapacity;///< ��Դ������(>=m_iPoolSize)
    MDB_UINT32  m_uiPoolSize;///< ��Դ�صĴ�С
    MDB_UINT32  m_uiHeadCursor;///< �α�ͷ
    MDB_UINT32  m_uiTailCursor;///< �α�β
    MDB_UINT32  m_uiAllocTimes;///< ����Ĵ���
    MDB_UINT32  m_uiFreeTimes;///< �ͷŵĴ���
    /*QuickMDB::*/TMdbNtcThreadLock  m_oAllocMutex, m_oFreeMutex;///< ������Դ�����ͷ���Դ��
};

template<class _Ty>
TMdbResPool<_Ty>::TMdbResPool():m_pResArray(NULL),m_iPoolCapacity(0),m_uiPoolSize(0),
    m_uiHeadCursor(0),m_uiTailCursor((MDB_UINT32)-1),m_uiAllocTimes(0),m_uiFreeTimes(0)
{
}

template<class _Ty>
void TMdbResPool<_Ty>::SetPoolSize(MDB_UINT32 uiPoolSize)
{
    m_oAllocMutex.Lock();
    m_oFreeMutex.Lock();
    if(uiPoolSize > m_uiPoolSize)
    {
        m_iPoolCapacity = uiPoolSize;
        _Ty* pResArray = new _Ty[uiPoolSize];
        //��Ҫ���ó�ʼ�����麯��
        InitRes(pResArray, m_uiPoolSize, uiPoolSize);
        if(m_uiPoolSize > 0)
        {
            memcpy(pResArray, m_pResArray, sizeof(_Ty)*m_uiPoolSize);
            if(m_uiTailCursor == m_uiPoolSize)
            {
                m_uiTailCursor = uiPoolSize;
            }
            else if(m_uiTailCursor > m_uiHeadCursor)//˵��tail��m_iPoolSize֮ǰ�Ǳ�ռ���ˣ���Ҫ���ƶ�
            {
                memmove(pResArray+m_uiHeadCursor+m_uiPoolSize-m_uiTailCursor, pResArray+m_uiHeadCursor, (m_uiTailCursor-m_uiHeadCursor)*sizeof(pResArray[0]));
                m_uiHeadCursor += m_uiPoolSize-m_uiTailCursor;
                m_uiTailCursor = uiPoolSize;
            }
            else if(m_uiTailCursor == m_uiHeadCursor)
            {
                m_uiHeadCursor = m_uiPoolSize;
                m_uiTailCursor = uiPoolSize;
            }
            else//˵��tail��head֮��Ϊ��ʹ�õģ�Ҳ��Ҫ�ƶ�ʹ������
            {
                if(m_uiPoolSize > m_uiHeadCursor)
                {
                    memmove(pResArray+m_uiTailCursor, pResArray+m_uiHeadCursor, (m_uiPoolSize-m_uiHeadCursor)*sizeof(pResArray[0]));
                }            
                m_uiHeadCursor = m_uiTailCursor+(m_uiPoolSize-m_uiHeadCursor);
                m_uiTailCursor = uiPoolSize;
            }
        }
        else
        {
            m_uiHeadCursor = 0;
            m_uiTailCursor = uiPoolSize;
        }
        if(m_pResArray)
        {
            delete []m_pResArray;
            m_pResArray = NULL;
        }
        m_pResArray = pResArray;
    }
    else
    {
        m_uiTailCursor = uiPoolSize;
    }
    m_uiPoolSize = uiPoolSize;
    m_oFreeMutex.Unlock();
    m_oAllocMutex.Unlock();
}

template<class _Ty>
bool TMdbResPool<_Ty>::AllocRes(_Ty& res)
{
    if(GetIdleSize() == 0)
    {
        return false;
    }
    else
    {
        m_oAllocMutex.Lock();
        if(GetIdleSize() == 0)
        {
            m_oAllocMutex.Unlock();
            return false;
        }
        if(m_uiHeadCursor >= m_uiPoolSize)
        {
            m_uiHeadCursor = 0;
        }
        res = m_pResArray[m_uiHeadCursor++];
        ++m_uiAllocTimes;
        m_oAllocMutex.Unlock();
        TADD_DETAIL("head:[%d], tail:[%d], pool_size:[%d], pool_capacity:[%d], res:[%d]\n", m_uiHeadCursor, m_uiTailCursor, m_uiPoolSize, m_iPoolCapacity, res);
        return true;
    }
}

template<class _Ty>
void TMdbResPool<_Ty>::FreeRes(_Ty res)
{
    m_oFreeMutex.Lock();
    if(m_uiTailCursor >= m_uiPoolSize)
    {
        m_uiTailCursor = 0;
    }
    m_pResArray[m_uiTailCursor++] = res;
    ++m_uiFreeTimes;
    m_oFreeMutex.Unlock();
    TADD_DETAIL("head:[%d], tail:[%d], pool_size:[%d], pool_capacity:[%d], res:[%d]\n", m_uiHeadCursor, m_uiTailCursor, m_uiPoolSize, m_iPoolCapacity, res);
}

/**
 * @brief id������
 * 
 */
class TMdbIdMgr:public TMdbResPool<int>
{
protected:
    /**
     * @brief ��ʼ����Դ�ĺ���, Ԫ�ط�Χ[uiStart, uiEnd)
     * 
     * @param pResArray [in] ��Դ����
     * @param uiStart   [in] ��ʼλ��
     * @param uiEnd     [in] ����λ��
     * @return ��
     */
    virtual void InitRes(int* pResArray, MDB_UINT32 uiStart, MDB_UINT32 uiEnd);
};

/**
 * @brief ����httpͷ�������
 * 
 */
class TMdbNtcAttrInfo:public /*QuickMDB::*/TMdbNtcBaseObject
{
public:
    /*QuickMDB::*/TMdbNtcStringBuffer sName;///< ��������
    /*QuickMDB::*/TMdbNtcStringBuffer sValue;///< ����ȡֵ
public:
    TMdbNtcAttrInfo(/*QuickMDB::*/TMdbNtcStringBuffer sName = "", /*QuickMDB::*/TMdbNtcStringBuffer sValue = "");
    /**
     * @brief �����������������ֵ
     * 
     * @param sName [in] ��������
     * @return const char*
     * @retval ����ȡֵ
     */
    inline /*QuickMDB::*/TMdbNtcStringBuffer ValueAsString()
    {
        return sValue;
    }
    /**
     * @brief �����������������ֵ
     * 
     * @param sName [in] ��������
     * @return MDB_INT64
     * @retval ����ȡֵ
     */
    inline MDB_INT64 ValueAsInterger()
    {
#ifdef _WIN32
        return _atoi64(sValue.c_str());
#else
        return atoll(sValue.c_str());
#endif        
    }
    /**
     * @brief �����������������ֵ
     * 
     * @param sName [in] ��������
     * @return double
     * @retval ����ȡֵ
     */
    inline double ValueAsFloat()
    {
        return atof(sValue.c_str());
    }
    virtual /*QuickMDB::*/TMdbNtcStringBuffer ToString() const;
    virtual MDB_INT64 Compare(const /*QuickMDB::*/TMdbNtcBaseObject *pObject) const
    {
        return sName.Compare(static_cast<const TMdbNtcAttrInfo*>(pObject)->sName, false);//�����ִ�Сд
    }
};

/**
 * @brief ���ԱȽ�
 * 
 */
class TMdbNtcAttrCompare:public /*QuickMDB::*/TMdbNtcObjCompare
{
public:
    /**
     * @brief ���캯��
     * 
     * @param bCaseSentive [in] �Ƿ����ִ�Сд
     * @return ��
     */
    TMdbNtcAttrCompare(bool bCaseSentive = true):m_bCaseSentive(bCaseSentive)
    {
    }
    virtual MDB_INT64 Compare(const /*QuickMDB::*/TMdbNtcBaseObject* pObject1, const /*QuickMDB::*/TMdbNtcBaseObject* pObject2) const
    {
        return (static_cast<const TMdbNtcAttrInfo*>(pObject1))->sName.Compare((static_cast<const TMdbNtcAttrInfo*>(pObject2))->sName, m_bCaseSentive);
    }
private:
    bool m_bCaseSentive;///< �Ƿ����ִ�Сд
};

/**
 * @brief ���Թ���
 * 
 */
class TMdbNtcAttrMgr:public /*QuickMDB::*/TMdbNtcBaseObject
{
    MDB_ZF_DECLARE_OBJECT(TMdbNtcAttrMgr);
public:
    /**
     * @brief ���캯��
     * 
     * @param bCaseSentive [in] �Ƿ����ִ�Сд
     * @return ��
     */
    TMdbNtcAttrMgr(bool bCaseSentive = true);
    virtual ~TMdbNtcAttrMgr();
    /**
     * @brief �������
     * 
     * @param sName [in] ��������
     * @param sValue [in] ����ȡֵ
     * @return TMdbNtcAttrInfo*
     */
    TMdbNtcAttrInfo* AddAttrInfo(/*QuickMDB::*/TMdbNtcStringBuffer sName, /*QuickMDB::*/TMdbNtcStringBuffer sValue);
    /**
     * @brief ɾ��һ������
     * 
     * @param sName [in] ��������
     * @return bool
     * @retval true �ҵ���ɾ��
     */
    bool DelAttrInfo(/*QuickMDB::*/TMdbNtcStringBuffer sName);
    /**
     * @brief �����������������ȡֵ
     * 
     * @param sName [in] ��������
     * @return TMdbNtcAttrInfo*
     * @retval ������Ϣ
     */
    TMdbNtcAttrInfo* GetAttrInfo(/*QuickMDB::*/TMdbNtcStringBuffer sName) const;
    /**
     * @brief �����������������ֵ
     * 
     * @param sName [in] ��������
     * @return const char*
     * @retval ����ȡֵ
     */
    /*QuickMDB::*/TMdbNtcStringBuffer AttrValueAsString(/*QuickMDB::*/TMdbNtcStringBuffer sName) const;
    /**
     * @brief �����������������ֵ
     * 
     * @param sName [in] ��������
     * @return MDB_INT64
     * @retval ����ȡֵ
     */
    MDB_INT64 AttrValueAsInterger(/*QuickMDB::*/TMdbNtcStringBuffer sName) const;
    /**
     * @brief �����������������ֵ
     * 
     * @param sName [in] ��������
     * @return double
     * @retval ����ȡֵ
     */
    double AttrValueAsFloat(/*QuickMDB::*/TMdbNtcStringBuffer sName) const;
    /**
     * @brief ������������������ȡֵ
     * 
     * @param sName [in] ��������
     * @param sValue [in] ����ȡֵ
     * @return ��
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, /*QuickMDB::*/TMdbNtcStringBuffer sValue);
    /**
     * @brief ������������������ȡֵ
     * 
     * @param sName [in] ��������
     * @param iValue [in] ����ȡֵ
     * @return ��
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, MDB_INT64 iValue);
    /**
     * @brief ������������������ȡֵ
     * 
     * @param sName [in] ��������
     * @param uiValue [in] ����ȡֵ
     * @return ��
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, MDB_UINT64 uiValue);
    /**
     * @brief ������������������ȡֵ
     * 
     * @param sName [in] ��������
     * @param iValue [in] ����ȡֵ
     * @return ��
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, MDB_INT32 iValue);
    /**
     * @brief ������������������ȡֵ
     * 
     * @param sName [in] ��������
     * @param uiValue [in] ����ȡֵ
     * @return ��
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, MDB_UINT32 uiValue);
    /**
     * @brief ������������������ȡֵ
     * 
     * @param sName [in] ��������
     * @param dValue [in] ����ȡֵ
     * @return ��
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, double dValue);
    /**
     * @brief ������Եĸ���
     * 
     * @return unsigned int
     * @retval ���Ը���
     */
    inline unsigned int GetAttrCount()
    {
        return m_oAttrMap.GetSize();
    }
    /**
     * @brief ������Ե������Ŀ�ʼ
     * 
     * @return iterator
     * @retval ��ʼ������
     */
    inline /*QuickMDB::*/TMdbNtcContainer::iterator IterBegin()
    {
        return m_oAttrMap.IterBegin();
    }
    /**
     * @brief ������Ե������Ľ���
     * 
     * @return iterator
     * @retval ����������
     */
    inline /*QuickMDB::*/TMdbNtcContainer::iterator IterEnd()
    {
        return m_oAttrMap.IterEnd();
    }
    /**
     * @brief ��������������ƥ�������ֵ
     * 
     * @param sName [in] ������
     * @return std::pair
     */
    inline std::pair</*QuickMDB::*/TMdbNtcContainer::iterator, /*QuickMDB::*/TMdbNtcContainer::iterator>
        IterFindMulti(/*QuickMDB::*/TMdbNtcStringBuffer sName)
    {
        return m_oAttrMap.EqualRange(TMdbNtcAttrInfo(sName));
    }
    inline /*QuickMDB::*/TMdbNtcContainer::iterator
        IterFind(/*QuickMDB::*/TMdbNtcStringBuffer sName)
    {
        return m_oAttrMap.IterFind(TMdbNtcAttrInfo(sName));
    }
    /**
     * @brief �����������
     * 
     */
    inline void Clear()
    {
        m_oAttrMap.Clear();
    }
    /**
     * @brief �����Ա��Ϣ
     *
     * �����Ա��Ϣ
     *
     * @return TMdbNtcStringBuffer
     */
    virtual /*QuickMDB::*/TMdbNtcStringBuffer ToString() const;
protected:
    /*QuickMDB::*/TMdbNtcAvlTree m_oAttrMap;///< Э��ͷ������map
};
////_MDB//_NTC_END
//}
#endif
