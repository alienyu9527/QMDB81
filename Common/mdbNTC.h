/**
 * @file mdbNTC.h
 * NTC头文件
 * 
 * @author jiang.jinzhou@zte.com.cn
 * @version 1.0
 * @date 2012/08/28
 * @bug 新建，无bug
 * @bug 单元测试，未发现bug
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

//之所以不放入shared_ptr的静态成员，是因为shared_ptr为模板，在不同编译器下展开可能在多个模块中存在多份
extern /*QuickMDB::*/TMdbNtcThreadLock g_oMdbNtcSharedPtrSpinLock;///< 用作shared_ptr的互斥
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

#if defined(OS_WINDOWS) && _MSC_VER < 1300//针对VC6
    //为了支持win32下的NULL==TMdbSharedPtr的比较
    friend bool operator == (const _Ty* pObj, TMdbSharedPtr<_Ty>& ptrRef)
    {
        return pObj==ptrRef.Value();
    }
    //为了支持win32下的NULL!=TMdbSharedPtr的比较
    friend bool operator != (const _Ty* pObj, TMdbSharedPtr<_Ty>& ptrRef)
    {
        return pObj!=ptrRef.Value();
    }
    /*无需实现，已被operator _Ty*()支持
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
        if(m_pRefCnt != ptrRef.m_pRefCnt)//判断是否是不同的引用
        {
            if(m_pcData) Release();
            //加锁
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
            //将释放操作放在最后，不在锁区间
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
    unsigned int* m_pRefCnt;///< 指向引用计数
};

/**
 * @brief 资源池
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
     * @brief 获得资源池最大资源数
     * 
     * @return MDB_UINT32
     * @retval 最大资源数
     */
    inline MDB_UINT32 GetPoolSize()
    {
        return m_uiPoolSize;
    }
    /**
     * @brief 获得容量(>=m_iPoolSize)
     * 
     * @return MDB_UINT32
     * @retval 容量
     */
    inline MDB_UINT32 GetPoolCapacity()
    {
        return m_iPoolCapacity;
    }
    /**
     * @brief 设置资源池的大小
     * 
     * @param uiPoolSize [in] 资源池的大小
     * @return 无
     */
    void SetPoolSize(MDB_UINT32 uiPoolSize);
    /**
     * @brief 申请一个资源
     * 
     * @return bool
     * @retval true 成功
     */
    bool AllocRes(_Ty& res);
    /**
     * @brief 释放资源
     * 
     * @param res [in] 要释放的资源
     * @return 无
     */
    void FreeRes(_Ty res);
    /**
     * @brief 获得正在被使用的资源数
     *      
     * @return int
     * @retval 正在被使用的资源数
     */
    inline MDB_UINT32 GetUsedSize()
    {
        //return m_iTailCursor>=m_iHeadCursor?(m_iPoolSize-(m_iTailCursor-m_iHeadCursor)):(m_iHeadCursor-m_iTailCursor);
        return m_uiAllocTimes>=m_uiFreeTimes?(m_uiAllocTimes-m_uiFreeTimes):(m_uiAllocTimes+(/*QuickMDB::*/MDB_NTC_ZS_MAX_UINT32-m_uiFreeTimes));
    }
    /**
     * @brief 获得空闲的资源数
     *      
     * @return int
     * @retval 空闲的资源数
     */
    inline MDB_UINT32 GetIdleSize()
    {
        return m_uiPoolSize-GetUsedSize();
    }
protected:
    /**
     * @brief 初始化资源的函数, 元素范围[uiStart, uiEnd)
     * 
     * @param pResArray [in] 资源数组
     * @param uiStart   [in] 开始位置
     * @param uiEnd     [in] 结束位置
     * @return 无
     */
    virtual void InitRes(_Ty* pResArray, MDB_UINT32 uiStart, MDB_UINT32 uiEnd)
    {
    }
    /**
     * @brief 销毁资源的函数, 元素范围[uiStart, uiEnd)
     * 
     * @param pResArray [in] 资源数组
     * @param uiStart   [in] 开始位置
     * @param uiEnd     [in] 结束位置
     * @return 无
     */
    virtual void UninitRes(_Ty* pResArray, MDB_UINT32 uiStart, MDB_UINT32 uiEnd)
    {
    }
protected:
    _Ty*    m_pResArray;///< 资源的数组
    MDB_UINT32  m_iPoolCapacity;///< 资源池容量(>=m_iPoolSize)
    MDB_UINT32  m_uiPoolSize;///< 资源池的大小
    MDB_UINT32  m_uiHeadCursor;///< 游标头
    MDB_UINT32  m_uiTailCursor;///< 游标尾
    MDB_UINT32  m_uiAllocTimes;///< 申请的次数
    MDB_UINT32  m_uiFreeTimes;///< 释放的次数
    /*QuickMDB::*/TMdbNtcThreadLock  m_oAllocMutex, m_oFreeMutex;///< 申请资源锁和释放资源锁
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
        //需要调用初始化的虚函数
        InitRes(pResArray, m_uiPoolSize, uiPoolSize);
        if(m_uiPoolSize > 0)
        {
            memcpy(pResArray, m_pResArray, sizeof(_Ty)*m_uiPoolSize);
            if(m_uiTailCursor == m_uiPoolSize)
            {
                m_uiTailCursor = uiPoolSize;
            }
            else if(m_uiTailCursor > m_uiHeadCursor)//说明tail与m_iPoolSize之前是被占用了，需要被移动
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
            else//说明tail与head之间为已使用的，也需要移动使其连续
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
 * @brief id分配器
 * 
 */
class TMdbIdMgr:public TMdbResPool<int>
{
protected:
    /**
     * @brief 初始化资源的函数, 元素范围[uiStart, uiEnd)
     * 
     * @param pResArray [in] 资源数组
     * @param uiStart   [in] 开始位置
     * @param uiEnd     [in] 结束位置
     * @return 无
     */
    virtual void InitRes(int* pResArray, MDB_UINT32 uiStart, MDB_UINT32 uiEnd);
};

/**
 * @brief 定义http头里的属性
 * 
 */
class TMdbNtcAttrInfo:public /*QuickMDB::*/TMdbNtcBaseObject
{
public:
    /*QuickMDB::*/TMdbNtcStringBuffer sName;///< 属性名称
    /*QuickMDB::*/TMdbNtcStringBuffer sValue;///< 属性取值
public:
    TMdbNtcAttrInfo(/*QuickMDB::*/TMdbNtcStringBuffer sName = "", /*QuickMDB::*/TMdbNtcStringBuffer sValue = "");
    /**
     * @brief 根据属性名获得属性值
     * 
     * @param sName [in] 属性名称
     * @return const char*
     * @retval 属性取值
     */
    inline /*QuickMDB::*/TMdbNtcStringBuffer ValueAsString()
    {
        return sValue;
    }
    /**
     * @brief 根据属性名获得属性值
     * 
     * @param sName [in] 属性名称
     * @return MDB_INT64
     * @retval 属性取值
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
     * @brief 根据属性名获得属性值
     * 
     * @param sName [in] 属性名称
     * @return double
     * @retval 属性取值
     */
    inline double ValueAsFloat()
    {
        return atof(sValue.c_str());
    }
    virtual /*QuickMDB::*/TMdbNtcStringBuffer ToString() const;
    virtual MDB_INT64 Compare(const /*QuickMDB::*/TMdbNtcBaseObject *pObject) const
    {
        return sName.Compare(static_cast<const TMdbNtcAttrInfo*>(pObject)->sName, false);//不区分大小写
    }
};

/**
 * @brief 属性比较
 * 
 */
class TMdbNtcAttrCompare:public /*QuickMDB::*/TMdbNtcObjCompare
{
public:
    /**
     * @brief 构造函数
     * 
     * @param bCaseSentive [in] 是否区分大小写
     * @return 无
     */
    TMdbNtcAttrCompare(bool bCaseSentive = true):m_bCaseSentive(bCaseSentive)
    {
    }
    virtual MDB_INT64 Compare(const /*QuickMDB::*/TMdbNtcBaseObject* pObject1, const /*QuickMDB::*/TMdbNtcBaseObject* pObject2) const
    {
        return (static_cast<const TMdbNtcAttrInfo*>(pObject1))->sName.Compare((static_cast<const TMdbNtcAttrInfo*>(pObject2))->sName, m_bCaseSentive);
    }
private:
    bool m_bCaseSentive;///< 是否区分大小写
};

/**
 * @brief 属性管理
 * 
 */
class TMdbNtcAttrMgr:public /*QuickMDB::*/TMdbNtcBaseObject
{
    MDB_ZF_DECLARE_OBJECT(TMdbNtcAttrMgr);
public:
    /**
     * @brief 构造函数
     * 
     * @param bCaseSentive [in] 是否区分大小写
     * @return 无
     */
    TMdbNtcAttrMgr(bool bCaseSentive = true);
    virtual ~TMdbNtcAttrMgr();
    /**
     * @brief 添加属性
     * 
     * @param sName [in] 属性名称
     * @param sValue [in] 属性取值
     * @return TMdbNtcAttrInfo*
     */
    TMdbNtcAttrInfo* AddAttrInfo(/*QuickMDB::*/TMdbNtcStringBuffer sName, /*QuickMDB::*/TMdbNtcStringBuffer sValue);
    /**
     * @brief 删除一个属性
     * 
     * @param sName [in] 属性名称
     * @return bool
     * @retval true 找到并删除
     */
    bool DelAttrInfo(/*QuickMDB::*/TMdbNtcStringBuffer sName);
    /**
     * @brief 根据属性名获得属性取值
     * 
     * @param sName [in] 属性名称
     * @return TMdbNtcAttrInfo*
     * @retval 属性信息
     */
    TMdbNtcAttrInfo* GetAttrInfo(/*QuickMDB::*/TMdbNtcStringBuffer sName) const;
    /**
     * @brief 根据属性名获得属性值
     * 
     * @param sName [in] 属性名称
     * @return const char*
     * @retval 属性取值
     */
    /*QuickMDB::*/TMdbNtcStringBuffer AttrValueAsString(/*QuickMDB::*/TMdbNtcStringBuffer sName) const;
    /**
     * @brief 根据属性名获得属性值
     * 
     * @param sName [in] 属性名称
     * @return MDB_INT64
     * @retval 属性取值
     */
    MDB_INT64 AttrValueAsInterger(/*QuickMDB::*/TMdbNtcStringBuffer sName) const;
    /**
     * @brief 根据属性名获得属性值
     * 
     * @param sName [in] 属性名称
     * @return double
     * @retval 属性取值
     */
    double AttrValueAsFloat(/*QuickMDB::*/TMdbNtcStringBuffer sName) const;
    /**
     * @brief 根据属性名设置属性取值
     * 
     * @param sName [in] 属性名称
     * @param sValue [in] 属性取值
     * @return 无
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, /*QuickMDB::*/TMdbNtcStringBuffer sValue);
    /**
     * @brief 根据属性名设置属性取值
     * 
     * @param sName [in] 属性名称
     * @param iValue [in] 属性取值
     * @return 无
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, MDB_INT64 iValue);
    /**
     * @brief 根据属性名设置属性取值
     * 
     * @param sName [in] 属性名称
     * @param uiValue [in] 属性取值
     * @return 无
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, MDB_UINT64 uiValue);
    /**
     * @brief 根据属性名设置属性取值
     * 
     * @param sName [in] 属性名称
     * @param iValue [in] 属性取值
     * @return 无
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, MDB_INT32 iValue);
    /**
     * @brief 根据属性名设置属性取值
     * 
     * @param sName [in] 属性名称
     * @param uiValue [in] 属性取值
     * @return 无
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, MDB_UINT32 uiValue);
    /**
     * @brief 根据属性名设置属性取值
     * 
     * @param sName [in] 属性名称
     * @param dValue [in] 属性取值
     * @return 无
     */
    void SetAttrValue(/*QuickMDB::*/TMdbNtcStringBuffer sName, double dValue);
    /**
     * @brief 获得属性的个数
     * 
     * @return unsigned int
     * @retval 属性个数
     */
    inline unsigned int GetAttrCount()
    {
        return m_oAttrMap.GetSize();
    }
    /**
     * @brief 获得属性迭代器的开始
     * 
     * @return iterator
     * @retval 开始迭代器
     */
    inline /*QuickMDB::*/TMdbNtcContainer::iterator IterBegin()
    {
        return m_oAttrMap.IterBegin();
    }
    /**
     * @brief 获得属性迭代器的结束
     * 
     * @return iterator
     * @retval 结束迭代器
     */
    inline /*QuickMDB::*/TMdbNtcContainer::iterator IterEnd()
    {
        return m_oAttrMap.IterEnd();
    }
    /**
     * @brief 根据属性名查找匹配的属性值
     * 
     * @param sName [in] 属性名
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
     * @brief 清除所有属性
     * 
     */
    inline void Clear()
    {
        m_oAttrMap.Clear();
    }
    /**
     * @brief 输出成员信息
     *
     * 输出成员信息
     *
     * @return TMdbNtcStringBuffer
     */
    virtual /*QuickMDB::*/TMdbNtcStringBuffer ToString() const;
protected:
    /*QuickMDB::*/TMdbNtcAvlTree m_oAttrMap;///< 协议头的属性map
};
////_MDB//_NTC_END
//}
#endif
