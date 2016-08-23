#include "Common/mdbDataStructs.h"
#include "Common/mdbSysLocks.h"
#include "Common/mdbSysThreads.h"
#include <math.h>

//#include "Sdk/mdbMemoryLeakDetectInterface.h"

//namespace QuickMDB
//{
        #define  MDB_NTC_MAX_QUENE_SIZE  1000000    //队列最大长度
        
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcObjCompare, TMdbNtcBaseObject);
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcPointerCompare, TMdbNtcObjCompare);
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcNoCaseCompare, TMdbNtcObjCompare);
        TMdbNtcObjCompare     g_oMdbNtcObjectCompare;
        TMdbNtcPointerCompare g_oMdbNtcPointerCompare;
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        //&&&&                                                              &&&&&&
        //&&&&                        算法实现部分                          &&&&&&
        //&&&&                                                              &&&&&&
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        
        //二分查找数据
        int MdbNtcBinarySearch(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare)
        {
            int iIndex = -1;
            int low = 0,mid,high;
        
            high = (int)(uiDataSize-1);
            MDB_INT64 iResult = 0;
            while (low <= high )
            {
                mid = (low + high)/2;
                iResult = oCompare.Compare(pDataHead[mid], &oData);
                if (iResult == 0)
                {
                    iIndex = mid;
                    break;
                }
                else if (iResult > 0 )
                {
                    high = mid - 1;
                }
                else
                {
                    low = mid + 1;                    
                }
            }
            return iIndex;
        }
        
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcSort, TMdbNtcBaseObject);
        //////////////////////////////////////////////////////////////////////////
        //
        //快速排序TQuickSort定义
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcQuickSort, TMdbNtcSort);
        //快速排序算法
        void TMdbNtcQuickSort::Sort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcObjCompare &oCompare) const
        {
            if ( uiDataSize > 1 )
            {
                QSort(pDataHead, 0, uiDataSize-1, oCompare);   
            }     
        }
        
        //递归快速排序,设置两个布尔变量记录一趟排序中，low和high从两端向中间移动的过程中是否发生过互换，如果未发生过互换，则
        //无需对相应的子表进行排序,以提高表数据基本有序(即最坏情况)时的算法性能
        void TMdbNtcQuickSort::QSort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiLow, MDB_UINT32 uiHigh,const TMdbNtcObjCompare &oCompare) const
        {
            if (uiLow < uiHigh)
            {
                bool bIsExchangeLow  = false;//一趟排序中，low指针上移过程中是否发生过数据交换
                bool bIsExchangeHigh = false;//一趟排序中，high指针下移过程中是否发生过数据交换
        
                //一趟快速排序，将表一分为二，uiPivoLoc是枢轴位置
                unsigned int uiPivoLoc = Partion(pDataHead, uiLow, uiHigh, oCompare, bIsExchangeLow, bIsExchangeHigh);
                if (bIsExchangeLow)
                {
                    //对低子表进行递归排序
                    QSort(pDataHead, uiLow, uiPivoLoc - 1, oCompare);
                }
                if (bIsExchangeHigh)
                {
                    //对高子表进行递归排序
                    QSort(pDataHead, uiPivoLoc + 1, uiHigh, oCompare);
                }
                
            }
        }
        
        //一趟排序，选择合适的值作为枢轴以提高算法性能，在移动low和high的过程的同时进行起泡操作，并设置是否发生交换标识，用以改善最坏情况下的性能
        MDB_UINT32 TMdbNtcQuickSort::Partion(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiLow, MDB_UINT32 uiHigh,const TMdbNtcObjCompare &oCompare, bool &bIsExchangeLow,bool &bIsExchangeHigh) const
        {
            unsigned int uiTempLow = uiLow;
            unsigned int uiTempHigh = uiHigh;
        
            //选择uiLow,uiHigh,(uiLow+uiHigh)/2，三个位置比较关键值，取三者中中值的记录为枢轴
            unsigned int uiMiddle = (uiLow+uiHigh)/2;
            if (oCompare.Compare(pDataHead[uiLow], pDataHead[uiMiddle]) < 0)
            {
                Swap(pDataHead, uiLow, uiMiddle);
            }    
            if (oCompare.Compare(pDataHead[uiHigh], pDataHead[uiLow]) < 0)
            {
                Swap(pDataHead, uiHigh, uiLow);
            }    
            if (oCompare.Compare(pDataHead[uiMiddle], pDataHead[uiLow]) > 0)
            {
                Swap(pDataHead, uiMiddle, uiLow);
            }
        
            //将第一个记录作为枢轴
            TMdbNtcBaseObject * pPivoKey = pDataHead[uiLow];
        
            //完成一趟排序
            //升序
            if (m_bSortAsc)
            {
                while (uiLow < uiHigh)
                {
                    //高端记录调整
                    while (uiLow < uiHigh && oCompare.Compare(pDataHead[uiHigh], pPivoKey) >= 0)
                    {
                        //同时进行冒泡排序
                        if (oCompare.Compare(pDataHead[uiHigh], pDataHead[uiHigh-1]) < 0)
                        {
                            Swap(pDataHead, uiHigh, uiHigh-1);
                            bIsExchangeHigh = true;
                        }
                        --uiHigh;
                    }
                    //和枢轴记录交换
                    Swap(pDataHead, uiLow, uiHigh);
                    //若交换后的uiLow记录小于前一个记录，则仍需对低端子表进行排序
                    if (!bIsExchangeLow && uiLow > uiTempLow && oCompare.Compare(pDataHead[uiLow],pDataHead[uiLow-1]) < 0)
                    {
                        bIsExchangeLow = true;
                    }
        
                    //低端记录调整
                    while (uiLow < uiHigh && oCompare.Compare(pDataHead[uiLow], pPivoKey) <= 0)
                    {
                        //同时进行冒泡排序
                        if (oCompare.Compare(pDataHead[uiLow], pDataHead[uiLow+1]) > 0)
                        {
                            Swap(pDataHead, uiLow, uiLow+1);
                            bIsExchangeLow = true;
                        }
        
                        ++uiLow;
                    }
                    //和枢轴记录交换
                    Swap(pDataHead, uiLow, uiHigh);
                    //若交换后的uiHigh记录大于后一个记录，则仍需对高端子表进行排序
                    if (!bIsExchangeHigh && uiHigh < uiTempHigh && oCompare.Compare(pDataHead[uiHigh],pDataHead[uiHigh+1]) > 0)
                    {
                        bIsExchangeHigh = true;
                    }
                }
            } 
            //降序
            else
            {
                while (uiLow < uiHigh)
                {
                    //高端记录调整
                    while (uiLow < uiHigh && oCompare.Compare(pDataHead[uiHigh], pPivoKey) <= 0)
                    {
                        //同时进行冒泡排序
                        if (oCompare.Compare(pDataHead[uiHigh], pDataHead[uiHigh-1]) > 0)
                        {
                            Swap(pDataHead, uiHigh, uiHigh-1);
                            bIsExchangeHigh = true;
                        }
                        --uiHigh;
                    }
                    //和枢轴记录交换
                    Swap(pDataHead, uiLow, uiHigh);
                    //若交换后的uiLow记录大于前一个记录，则仍需对低端子表进行排序
                    if (!bIsExchangeLow && uiLow > uiTempLow && oCompare.Compare(pDataHead[uiLow],pDataHead[uiLow-1]) > 0)
                    {
                        bIsExchangeLow = true;
                    }
        
                    //低端记录调整
                    while (uiLow < uiHigh && oCompare.Compare(pDataHead[uiLow], pPivoKey) >= 0)
                    {
                        //同时进行冒泡排序
                        if (oCompare.Compare(pDataHead[uiLow], pDataHead[uiLow+1]) < 0)
                        {
                            Swap(pDataHead, uiLow, uiLow+1);
                            bIsExchangeLow = true;
                        }
        
                        ++uiLow;
                    }
                    //和枢轴记录交换
                    Swap(pDataHead, uiLow, uiHigh);
                    //若交换后的uiHigh记录小于后一个记录，则仍需对高端子表进行排序
                    if (!bIsExchangeHigh && uiHigh < uiTempHigh && oCompare.Compare(pDataHead[uiHigh],pDataHead[uiHigh+1]) < 0)
                    {
                        bIsExchangeHigh = true;
                    }
                }
            }
            
        
            return uiLow;
        }
        
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcHeapSort, TMdbNtcSort);
        //////////////////////////////////////////////////////////////////////////
        //
        //堆排序THeapSort定义
        //
        //////////////////////////////////////////////////////////////////////////
        //处理逻辑：
        //升序：建成大顶堆，则堆顶元素是最大元素，将堆顶元素与最后一个元素交换位置，
        //      那么最大元素就移动到了上升序列中的目的位置。重新建成大顶堆，最后一
        //      个元素不参与此次建堆。重复上述操作，当最后一次建堆完成时，所有元素
        //      呈上升序列排列！
        //降序：建成小顶堆，则堆顶元素是最小元素，将堆顶元素与最后一个元素交换位置，
        //      那么最小元素就移动到了下降序列中的目的位置。重新建成小顶堆，最后一
        //      个元素不参与此次建堆。重复上述操作，当最后一次建堆完成时，所有元素
        //      呈下降序列排列！
        
        //按照传入的数组首地址排序
        void TMdbNtcHeapSort::Sort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcObjCompare &oCompare) const
        {
            if ( uiDataSize <= 1)
            {
                return;
            }
        
            int i = 0;
            //将全部数组元素建成大顶堆或者小顶堆
            for (i = (int)(uiDataSize/2 -1); i >= 0; --i)
            {
                HeapAdjust(pDataHead, (MDB_UINT32)i,uiDataSize-1,oCompare);
            }
        
            for (i = (int)uiDataSize-1;i > 0; --i)
            {
                //将堆顶元素和最后一个元素交换
                Swap(pDataHead, 0, (MDB_UINT32)i);
                //将元素0~i-1重新调整为大顶堆或者小顶推
                HeapAdjust(pDataHead, 0, (MDB_UINT32)(i-1),oCompare);
            }
        }
        
        //调整堆
        void TMdbNtcHeapSort::HeapAdjust(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiStartLoc, MDB_UINT32 uiMaxLoc, const TMdbNtcObjCompare &oCompare) const
        {
            TMdbNtcBaseObject * pTempObj = pDataHead[uiStartLoc];
        
            //升序，调整成大顶堆
            if (m_bSortAsc)
            {
                for ( MDB_UINT32 j = 2*uiStartLoc+1; j <= uiMaxLoc; j = 2*j+1)
                {
                    //和左右孩子中较大的比较大小和交换
                    if ( j < uiMaxLoc && oCompare.Compare(pDataHead[j], pDataHead[j+1]) < 0)
                    {
                        ++j;
                    }
                    if (oCompare.Compare(pTempObj, pDataHead[j] ) < 0)
                    {
                        pDataHead[uiStartLoc] = pDataHead[j];
                        uiStartLoc = j;
                    }
                }
                pDataHead[uiStartLoc] = pTempObj;
            } 
            //降序，调整成小顶堆
            else
            {
                for ( MDB_UINT32 j = 2*uiStartLoc+1; j <= uiMaxLoc; j = 2*j+1)
                {
                    //和左右孩子中较小的比较大小和交换
                    if ( j < uiMaxLoc && oCompare.Compare(pDataHead[j], pDataHead[j+1]) > 0)
                    {
                        ++j;
                    }
                    if (oCompare.Compare(pTempObj, pDataHead[j] ) > 0)
                    {
                        pDataHead[uiStartLoc] = pDataHead[j];
                        uiStartLoc = j;
                    }
                }
                pDataHead[uiStartLoc] = pTempObj;
            }   
        }
        
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        //&&&&                                                              &&&&&&
        //&&&&                    数据结构实现部分                          &&&&&&
        //&&&&                                                              &&&&&&
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        //数据键值过滤数组
        const int g_iMdbNtcKeyTreeDigitIndex[256]=
        {
            /*0-19*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*20-39*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*40-59*/
            -1, -1, -1, -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1,
            /*60-79*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*80-99*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*100-119*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*120-139*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*140-159*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*160-179*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*180-199*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*200-219*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*220-239*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            /*240-255*/
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
        };
        
        //////////////////////////////////////////////////////////////////////////
        //
        //容器基类TContainer定义
        //
        //////////////////////////////////////////////////////////////////////////
#define MdbNtcReleaseData(pData) if(m_bAutoRelease && pData) {delete pData;pData = NULL;}else{pData = NULL;}
#define MdbNtcReleaseNode(pNode) if(pNode) {MdbNtcReleaseData(pNode->pData);delete pNode;pNode=NULL;}
        //#define IterRelease1(itor) if(m_bAutoRelease){TMdbNtcBaseObject* pBaseObject = itor.data();if(pBaseObject) {delete pBaseObject;pBaseObject=NULL;}}
        //#define IterRelease2(itor1, itor2) {for(iterator itor = itor1, itor_end = itor2; itor != itor_end; ++itor) IterRelease1(itor);}

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcContainer, TMdbNtcBaseObject);
        TMdbNtcBaseObject* TMdbNtcContainer::ms_pNullObject = NULL;
        TMdbNtcContainer::iterator TMdbNtcContainer::ms_itorEnd;
        TMdbNtcContainer::TMdbNtcContainer()
        {
            m_bAutoRelease = false;
        }

        TMdbNtcContainer::~TMdbNtcContainer()
        {
        }

        void TMdbNtcContainer::SetAutoRelease(bool bAutuRelease)
        {
            m_bAutoRelease = bAutuRelease;
        }
        //打印容器信息
        void TMdbNtcContainer::Print(FILE* fp /* = NULL */) const
        {
            if(fp == NULL) 
            {
                fp = stdout;
            }
            TMdbNtcBaseObject* pBaseObject = NULL;
            iterator itor = IterBegin(), itEnd = IterEnd();
            for (;itor != itEnd; ++itor)
            {
                pBaseObject = itor.data();
                fprintf(fp, "%s\n", pBaseObject?(pBaseObject->ToString().c_str()):"null");
            }
        }
        
        TMdbNtcContainer::iterator TMdbNtcContainer::IterErase(TMdbNtcContainer::iterator itorBegin, TMdbNtcContainer::iterator itorEnd)
        {    
            iterator itor = itorBegin;
            for (; itor != itorEnd;)
            {   
                itor = IterErase(itor);
            }
            return itor;
        }

        unsigned int TMdbNtcContainer::GetDataMemoryUsage() const
        {
            unsigned int uiSize = 0;
            TMdbNtcBaseObject* pBaseObject = NULL;
            iterator itor = IterBegin(), itEnd = IterEnd();
            for (;itor != itEnd; ++itor)
            {
                pBaseObject = itor.data();
                if(pBaseObject) uiSize += pBaseObject->GetObjectSize();
            }
            return uiSize;
        }
        
        //根据数据查找迭代器
        TMdbNtcContainer::iterator TMdbNtcContainer::IterFind(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare,
                                                  iterator itLastFound /*= ms_itorEnd*/) const
        {
            if(itLastFound.ppObject == NULL) itLastFound = IterBegin();
            else ++itLastFound;
            return ::IterFind(itLastFound, IterEnd(), oData, oCompare);
        }
        
        //从开始迭代器查找到结束迭代器，查找匹配的元数据
        TMdbNtcContainer::iterator IterFind(TMdbNtcContainer::iterator itorBegin, TMdbNtcContainer::iterator itorEnd, const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare)
        {
            TMdbNtcContainer::iterator itor = itorBegin;
            for (; itor != itorEnd; ++itor)
            {
                if (oCompare.Compare(&oData,itor.data()) == 0 )
                {
                    break;
                }
            }
            return itor;
        }
        
        //////////////////////////////////////////////////////////////////////////
        //
        //动态数组TAutoArray定义
        //
        //////////////////////////////////////////////////////////////////////////
        
        //注意：动态数组创建的时候所有预留空间、扩容时新增空间以及删除元素的空间全部
        //将值设置为NULL，那么在对数组进行某些操作的时候，需要考虑是否需要重复置NULL
        
        //构造函数
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcAutoArray, TMdbNtcContainer);
        TMdbNtcAutoArray::TMdbNtcAutoArray(MDB_UINT32 uiGrowBy /* = 0 */)
        {
            //变量初始化
            m_uiGrowBy = uiGrowBy;
            m_pData = NULL;
            m_uiSize = 0;
            m_uiCapacity = 0;
        
        }
        
        //根据现有容器初始化新建容器
        TMdbNtcAutoArray::TMdbNtcAutoArray(const TMdbNtcContainer* pSrcContainer)
        {
            m_uiGrowBy = 0;
            m_pData = NULL;
            m_uiSize = 0;
            m_uiCapacity = 0;
            if(pSrcContainer && !pSrcContainer->IsEmpty())
            {
                *this = *pSrcContainer;
            }
        }

        TMdbNtcAutoArray::TMdbNtcAutoArray(const TMdbNtcAutoArray& oSrcArray)
        {
            m_uiGrowBy = 0;
            m_pData = NULL;
            m_uiSize = 0;
            m_uiCapacity = 0;
            if(!oSrcArray.IsEmpty())
            {
                *this = oSrcArray;
            }
        }

        TMdbNtcAutoArray& TMdbNtcAutoArray::operator = (const TMdbNtcContainer& oSrcContainer)
        {
            Clear();
            unsigned int uiSize = oSrcContainer.GetSize();
            if (uiSize > 0 )
            {        
                //预留
                Reserve(uiSize);
                if(oSrcContainer.IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcAutoArray)))
                {
                    memcpy(m_pData, static_cast<const TMdbNtcAutoArray*>(&oSrcContainer)->m_pData, sizeof(TMdbNtcBaseObject*)*uiSize);
                }
                else
                {
                    //因为传入参数的容器类型不确定，跨容器使用迭代器遍历
                    iterator itor = oSrcContainer.IterBegin(),itEnd = oSrcContainer.IterEnd();
                    unsigned int i = 0;
                    for (;itor != itEnd; ++itor)
                    {
                        m_pData[i++] = itor.data();
                    }
                }
                m_uiSize = uiSize;
            }
            return *this;
        }
        
        //析构函数
        TMdbNtcAutoArray::~TMdbNtcAutoArray()
        {
            Clear();
            if(m_pData)
            {
                delete []m_pData;
                m_pData = NULL;
            }
        }
        
        MDB_UINT32 TMdbNtcAutoArray::GetContainerMemoryUsage() const
        {
            return (MDB_UINT32)(sizeof(TMdbNtcAutoArray)+sizeof(TMdbNtcBaseObject*)*m_uiCapacity);
        }
        
        #define MdbNtcReleaseArray(ppData, uiStartIndex, uiStopIndex) if(m_bAutoRelease && uiStartIndex < uiStopIndex){\
            for (MDB_UINT32 uiReleaseIndex = uiStartIndex; uiReleaseIndex < uiStopIndex; ++uiReleaseIndex)\
            {\
                if(ppData[uiReleaseIndex])\
                {\
                    delete ppData[uiReleaseIndex];\
                }\
            }\
            memset(m_pData + uiStartIndex, 0x00, (uiStopIndex - uiStartIndex)*sizeof(TMdbNtcBaseObject *));\
        }
        //清空容器
        void TMdbNtcAutoArray::Clear()
        {    
            if(m_pData && m_uiSize > 0 )
            {
                MdbNtcReleaseArray(m_pData, 0, m_uiSize);
                m_uiSize = 0;
            }
        }
        
        //预留数组大小
        void TMdbNtcAutoArray::Reserve(MDB_UINT32 uiCapacity)
        {
            //预留小于等于容器容量则不扩容
            if (m_uiCapacity >= uiCapacity )
            {
                return;
            }
            //预留大于容器容量，数组未创建，扩容
            else if(NULL == m_pData )
            {
                m_pData = new TMdbNtcBaseObject*[uiCapacity];
                memset(m_pData, 0x00, uiCapacity *sizeof(TMdbNtcBaseObject* ));
                m_uiCapacity = uiCapacity;
            }
            //预留大于容器容量，数组已创建，扩容
            else
            {
                //保存旧数组首地址
                TMdbNtcBaseObject ** pPre = m_pData;
                //创建新数组
                m_pData = new TMdbNtcBaseObject*[uiCapacity];
                //拷贝旧元素
                if (m_uiSize > 0)
                {
                    memcpy(m_pData, pPre, m_uiSize*sizeof(TMdbNtcBaseObject*));
                }
                
                //重置未使用的预留空间
                memset(m_pData + m_uiSize , 0x00 ,(uiCapacity-m_uiSize)*sizeof(TMdbNtcBaseObject*));
        
                m_uiCapacity = uiCapacity;
                
                //释放旧数组空间
                delete[] pPre;
            }
           
        }
        
        //设置数组的大小,数组元素数目发生变化
        void TMdbNtcAutoArray::SetSize(MDB_UINT32 uiNewSize)
        {
            //容器为空
            if (NULL == m_pData )
            {
                //新size为0，直接返回
                if(0 == uiNewSize )
                {
                    return;
                }
                //根据既定策略获取容器容量的大小
                unsigned int uiAllocSize = (uiNewSize > m_uiGrowBy )? uiNewSize:m_uiGrowBy;
                m_pData = new TMdbNtcBaseObject*[uiAllocSize];
                //使用iAllocSize而不是uiNewSize,是因为使用uiNewSize可能导致未使用到的预留空间未被初始化
                memset(m_pData, 0x00, uiAllocSize * sizeof(TMdbNtcBaseObject * ));
                m_uiSize = uiNewSize;
                m_uiCapacity = uiAllocSize;
            }
            //容器非空，新size小于等于容器容量
            else if (uiNewSize <= m_uiCapacity)
            {
                //将超出新size部分的旧元素置为空，iNewSize>m_uiSize的情况，由于未使用空间已经被置为NULL，无须重复操作
                if (m_uiSize > uiNewSize)
                {
                    MdbNtcReleaseArray(m_pData, uiNewSize, m_uiSize);
                }
                m_uiSize = uiNewSize;
            } 
            //新size大于容器容量，则开辟新空间
            else
            {
                unsigned int uiNewCapacity = 0;      //新容量大小
                unsigned int uiGrowBy  =  m_uiGrowBy;
                //m_iGrowBy等于0，采用既定策略设置m_iGrowBy的大小
                if(0 ==uiGrowBy)
                {
                    uiGrowBy = m_uiSize/8;
                    uiGrowBy = (uiGrowBy < 4)? 4:((uiGrowBy >1024 ) ? 1024:uiGrowBy );           
                }
                //设置新的容量大小
                if (uiNewSize < m_uiCapacity + uiGrowBy)
                {
                    uiNewCapacity = m_uiCapacity + uiGrowBy;
                }
                else 
                {
                    uiNewCapacity = uiNewSize;
                }
                //预留
                Reserve(uiNewCapacity);
                m_uiSize = uiNewSize;  
            }
        }
        
        //得到指定元素的下标
        int TMdbNtcAutoArray::FindIndex(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare, MDB_UINT32 uiStart /* = 0*/) const
        {
            int iIndex = -1;
        
            if (m_uiSize > 0 )
            {
                for (MDB_UINT32 i = uiStart; i < m_uiSize; ++i)
                {
                    if (oCompare.Compare(m_pData[i],&oData)  == 0 )
                    {
                        iIndex = (int)i;
                        break;
                    }
                }
            }
        
            return iIndex;
        }
        
        //添加新元素,返回下标
        int TMdbNtcAutoArray::Add(TMdbNtcBaseObject* pNewObj)
        {
            int iIndex = 0;
        
            iIndex = (int)m_uiSize;
        
            //容器大小增1
            SetSize(m_uiSize+1);
            
            //添加新元素
            m_pData[iIndex] = pNewObj;
        
            return iIndex;
        }
        
        int TMdbNtcAutoArray::Add(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd /* = ms_itorEnd */)
        {    
            if(itSrcBegin == itSrcEnd)
            {
                return -1;
            }
            else
            {
                MDB_UINT32 uiIndex = GetSize();
                for (TMdbNtcContainer::iterator itor = itSrcBegin; itor != itSrcEnd; ++itor)
                {
                    Add(itor.data());
                }
                return (int)uiIndex;
            }
        }
        
        //插入新元素
        MDB_UINT32 TMdbNtcAutoArray::Insert(MDB_UINT32 uiIndex, TMdbNtcBaseObject* pNewObj)
        { 
            //如果指定的下标大于等于容器大小，则新元素插入到尾部,该分支包含了m_uiSize等于0，以及m_pData等于NULL的情况
            if (uiIndex >= m_uiSize)
            {
                uiIndex = m_uiSize;
                SetSize(m_uiSize+1);
            }
            else
            {
                SetSize(m_uiSize+1);
                //指定下标及后面的元素全部向后移动一个位置
                memmove(m_pData+uiIndex+1, m_pData+uiIndex, (m_uiSize-uiIndex-1)*sizeof(TMdbNtcBaseObject*));
            }
            m_pData[uiIndex] = pNewObj;
            return uiIndex;
        }
        
        MDB_UINT32 TMdbNtcAutoArray::Insert(MDB_UINT32 uiIndex, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd)
        {
            MDB_UINT32 uiRet    = 0;
            bool bFirstTime = true;
            
            TMdbNtcContainer::iterator itor = itSrcBegin;
            for (MDB_UINT32 i = 0; itor != itSrcEnd; ++itor, ++i)
            {
                if( bFirstTime)
                {
                    uiRet = Insert(uiIndex+i, itor.data());
                    bFirstTime = false;
                }
                else
                {

                    Insert(uiIndex+i, itor.data());
                }
            }
            return uiRet;
        }
        
        //连续删除指定个数的元素
        int TMdbNtcAutoArray::Remove(MDB_UINT32 uiIndex,MDB_UINT32 uiDelCount /* = 1 */)
        {            
            int iRetDelCount = 0;
            //如果指定的下标大于等于容器大小，或者删除元素的个数为0，则删除失败直接返回0(注意：该语句已将容器为空的情况包括在内)
            if ((uiIndex >= m_uiSize) || (0 == uiDelCount))
            {
                return 0;
            } 
            else
            {        
                //如果删除到数组尾部，直接将删除部分的元素置为NULL即可
                if (uiDelCount >= m_uiSize - uiIndex )
                {
                    iRetDelCount = (int)(m_uiSize - uiIndex);
                    MdbNtcReleaseArray(m_pData, uiIndex, m_uiSize);
                }
                else
                {
                    iRetDelCount = (int)uiDelCount;
                    MdbNtcReleaseArray(m_pData, uiIndex, uiIndex+uiDelCount);
                    //元素前移覆盖掉要删除的元素
                    memmove(m_pData+uiIndex,m_pData+(uiIndex+(MDB_UINT32)iRetDelCount),(m_uiSize-uiIndex-(MDB_UINT32)iRetDelCount)*sizeof(TMdbNtcBaseObject*));
                    //必须将移动元素后空出的空间置空
                    memset(m_pData+m_uiSize-iRetDelCount, 0x00, (MDB_UINT32)iRetDelCount*sizeof(TMdbNtcBaseObject*));
                }        
                //修改容器大小
                m_uiSize -= (MDB_UINT32)(iRetDelCount);
            }
        
            return iRetDelCount;
        }
        
        //根据数据来移除匹配的节点  
        int TMdbNtcAutoArray::Remove(const TMdbNtcBaseObject &oData, int iDelCount /* = 1 */, const TMdbNtcObjCompare &oCompare /* = g_oMdbNtcObjectCompare */)
        {
            int iRetDelCount = 0;            //临时变量，实际删除的元素个数
            int iDestIndex = -1;             //临时变量，指定数据的下标
            unsigned int uiCurIndex  =  0;   //临时变量，当前开始遍历的下标
            int iTempDelCount = iDelCount;
        
            ++iDelCount;                     //先加1是为了校正循环中先对该变量做--操作的偏差
        
            //遍历数组删除匹配的元素
            while (uiCurIndex <  m_uiSize )
            {
                //控制删除匹配元素的个数，如果为uiDelCount为-1，则删除所有匹配的元素
                if ((iTempDelCount != -1 ) && --iDelCount < 1 )
                {
                    break;
                }
        
                iDestIndex = FindIndex(oData,oCompare,uiCurIndex);
                
                //查找成功，则移除该元素
                if (iDestIndex >= 0)
                {
                    
                    
                    Remove((MDB_UINT32)iDestIndex);
                    ++iRetDelCount;
                    uiCurIndex = (MDB_UINT32)iDestIndex;
                }
                else
                {
                    //查找失败直接退出
                    break;
                }               
            }
           
            return iRetDelCount;
        }
        
        //合并数组
        int TMdbNtcAutoArray::Combine(int iDestIndex, TMdbNtcContainer* pSrcContainer, int iSrcStart /* = 0 */,int iSrcCount /* = -1 */)
        {
            int iIndex = -1;
            bool bIsOffset = true;                          //是否需要后移元素
        
            MDB_UINT32 uiSize = pSrcContainer->GetSize();  //获取源容器的大小
            
            //源容器为空，直接退出
            if (0 == uiSize)
            {
                return iIndex;
            }
        
            //实参规整--目标开始位置规整，调整为尾部，无须后移操作
            if (iDestIndex<= -1 || ((MDB_UINT32)iDestIndex) >= m_uiSize )
            { 
                iDestIndex = (int)m_uiSize;
                bIsOffset = false;
            }
            //实参规整--源数据开始位置规整，调整为头部
            if (iSrcStart < 0)
            {
                iSrcStart = 0;
            }
            //实参规整--源数据拷贝数目规整，调整为开始元素起的所有元素个数
            if (iSrcCount <= -1)
            {
                iSrcCount = (int)uiSize-iSrcStart;
            }
        
            //如果源容器要拷贝的数目超过最大大小
            if (iSrcStart+iSrcCount > (int)uiSize)
            {
                iSrcCount = (int)uiSize-iSrcStart;
            }
            if(iSrcCount <= 0)
            {
                return iIndex;
            }
            else
            {
                MDB_UINT32 iTempDestIndex = (MDB_UINT32)iDestIndex;  //临时变量，保存目标位置，作为工作变量
         
                //设置目标数组大小
                SetSize(m_uiSize+(MDB_UINT32)iSrcCount);
        
                //如果插入到目标数组的尾部，或者目标数组为空，则不需要后移元素
                if (bIsOffset)
                {
                    //元素后移
                    memmove(m_pData+iDestIndex+iSrcCount, m_pData+iDestIndex,(MDB_UINT32)iSrcCount*sizeof(TMdbNtcBaseObject*));
        
                }
         
                //插入元素
                iterator itorSrcBegin = pSrcContainer->IterBegin()+iSrcStart;
                iterator itor = itorSrcBegin;
                for (int i = 0; i < iSrcCount && itor.pNodeObject; ++itor, ++i)
                {
                    m_pData[iTempDestIndex++] = itor.data();
                }
                bool bAutoRelease = pSrcContainer->IsAutoRelease();
                pSrcContainer->SetAutoRelease(false);
                pSrcContainer->IterErase(itorSrcBegin, itor);
                pSrcContainer->SetAutoRelease(bAutoRelease);
                iIndex = iDestIndex;
            }
            return iIndex;
        }
        
        void TMdbNtcAutoArray::Sort(const TMdbNtcSort& oSort, const TMdbNtcObjCompare &oCompare)
        {
            if (m_uiSize > 1)
            {
                oSort.Sort(m_pData,m_uiSize,oCompare);
            }   
        }
        
        //移除重复的元素
        int TMdbNtcAutoArray::RemoveDup(const TMdbNtcObjCompare &oCompare)
        {
            
            int iCount = 0;
            //若元素个数大于1，才进行移除重复元素的操作
            if (m_uiSize > 1 )
            {
                for (unsigned int i = 0; i < m_uiSize; ++i )
                {
                    for (unsigned int j = i+1; j < m_uiSize;)
                    {
                        if (oCompare.Compare(m_pData[i], m_pData[j]) == 0 )
                        {
                            
                            
                            Remove(j);
                            ++iCount;
                        }
                        else
                        {
                            ++j;
                        }
                    }
                }
            }
        
            return iCount;
        }
        
        //获得开始迭代器
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterBegin() const
        {
            return iterator(this, m_uiSize==0 ? NULL:m_pData);
        }
        
        //获得尾元素迭代器
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterLast() const
        {
            return iterator(this, m_uiSize == 0? NULL:(m_pData+m_uiSize-1));
        }
        
        //移除某个迭代器对应的节点，返回下一个迭代器
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterErase(TMdbNtcAutoArray::iterator itor)
        {
            if(itor.ppObject == NULL || itor.ppObject >= m_pData+m_uiSize) return IterEnd();
            MDB_UINT32 uiIndex = (MDB_UINT32)(itor.ppObject - m_pData);
            Remove(uiIndex);
            if(uiIndex >= m_uiSize) return IterEnd();
            else return iterator(this, m_pData+uiIndex);
        }
        
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterErase(TMdbNtcAutoArray::iterator itorBegin, TMdbNtcAutoArray::iterator itorEnd)
        {
            
            if(m_uiSize == 0 || itorBegin.ppObject == NULL) return IterEnd();    
            TMdbNtcAutoArray::iterator itorRet = itorBegin;
            //获得index
            MDB_UINT32 iStart = (MDB_UINT32)(itorBegin.ppObject-m_pData);    
            Remove(iStart, itorEnd.ppObject?((MDB_UINT32)(itorEnd.ppObject-m_pData)):(MDB_UINT32)-1);    
            if(m_uiSize > iStart) 
				   itorRet.ppObject = m_pData+iStart;
            else
				  itorRet.ppObject = NULL;
            return itorRet;
        }
        
        //通过迭代器交换，完成元素和节点的交换
        void TMdbNtcAutoArray::IterSwap(TMdbNtcAutoArray::iterator itor1, TMdbNtcAutoArray::iterator itor2)
        {
            if (itor1.ppObject && itor2.ppObject)
            {
                 TMdbNtcBaseObject * pTemp = *(itor1.ppObject);
                 *(itor1.ppObject) = *(itor2.ppObject);
                 *(itor2.ppObject) = pTemp;
            } 
        }

        //通过迭代器定位插入数据
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterInsert(TMdbNtcAutoArray::iterator itor,TMdbNtcBaseObject* pData)
        {
            MDB_UINT32 uiIndex = 0;
            if(itor.ppObject && itor.ppObject >=  m_pData && itor.ppObject < m_pData+m_uiSize)
            {
                uiIndex = (MDB_UINT32)(itor.ppObject - m_pData);
                uiIndex = Insert(uiIndex,pData);
                return iterator(this,m_pData+uiIndex);
            }
            else
            {
                //如果是结束迭代器，或者没有找到位置，则添加到动态数组尾部
                Add(pData);
                return iterator(this,m_pData+m_uiSize-1);
            }
            
        }
        
        //获取前一个迭代器
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterPrev(iterator itCur, int iStep /* = -1 */) const
        {
            if(m_pData == NULL) 
            {
                return IterEnd();
            }
        
            int iIndex = -1;   //当前迭代器对应的下标
        
            //若偏移的步长超出数组的范围，则取第一个返回
            if (iStep <= -1)
            {
                if (itCur.ppObject && itCur.ppObject >=  m_pData && itCur.ppObject < m_pData+m_uiSize )
                {
                    iIndex = (int)(itCur.ppObject-m_pData);
                    if(iIndex >= (-iStep))
                    {
                        itCur.ppObject = &m_pData[iIndex + iStep];
                        return itCur;
                    }
                    else
                    {
                        return IterBegin();
                    }
                }
                else
                {
                    return IterBegin();
                }
            }
            else if(0 == iStep)
            {
                return itCur;
            }
            else
            {
                return IterNext(itCur,iStep);
            }
        }
        
        //获取后一个迭代器
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterNext(iterator itCur, int iStep /* = 1 */) const
        {
            if (NULL == m_pData )
            {
                return IterEnd();
            }
            
            int iIndex = -1; //当前迭代器对应的下标
           
            //若偏移的步长超出数组的范围，则取最后一个返回
            if (iStep >= 1)
            {
                if (itCur.ppObject && itCur.ppObject>=m_pData && itCur.ppObject<m_pData+m_uiSize)
                {
        
                    iIndex = (int)(itCur.ppObject -m_pData);
        
                    if (m_uiSize >=(unsigned int )(iIndex + iStep+1))
                    {
                        itCur.ppObject = &m_pData[iIndex+iStep];
                        return itCur;
                    }
                    else
                    {
                        return IterEnd();
                    }
                }
                else
                {
                    return IterEnd();
                }
            }
            else if(0 == iStep)
            {
                return itCur;
            }
            else
            {
                return IterPrev(itCur,iStep);
            }
        }
        
        //////////////////////////////////////////////////////////////////////////
        //
        //栈TStack定义
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcStack, TMdbNtcAutoArray);
        //构造函数
        TMdbNtcStack::TMdbNtcStack (MDB_UINT32 uiStackCapacity)
        {
            Reserve(uiStackCapacity);
        }
        
        //入栈
        int TMdbNtcStack::Push (TMdbNtcBaseObject *pData)
        {
            //栈满，拒绝入栈
            if (m_uiSize >= m_uiCapacity)
            {
                return -1;
            }
            else
            {
                m_pData[m_uiSize] = pData;
                ++m_uiSize;
        
                return (int)m_uiSize;
            }
        }
        
        //出栈
        TMdbNtcBaseObject * TMdbNtcStack::Pop ()
        {  
            //栈空，返回NULL
            if(0 == m_uiSize)
            {
                return NULL;
            }
            else
            {
                TMdbNtcBaseObject * pRet ;
                pRet = m_pData[m_uiSize-1];        
                //删除栈顶元素
                m_pData[m_uiSize-1] = NULL;
                //栈顶位置退1
                --m_uiSize;
                return pRet;
            }
        
        }
        
        //获取栈顶元素
        TMdbNtcBaseObject * TMdbNtcStack::Top () const
        {
               
            //栈空，返回NULL
          
            if(0 == m_uiSize)
            {
                return NULL;
            }
            else
            {
                return m_pData[m_uiSize-1];
            }
        }
        
        
        //////////////////////////////////////////////////////////////////////////
        //
        //链表TBaseList::TNode定义
        //
        //////////////////////////////////////////////////////////////////////////
        //构造函数
        TMdbNtcBaseList::TNode::TNode(TMdbNtcBaseObject *pData /* = NULL */)
        {
            this->pData = pData;
            pPrev = NULL;
            pNext = NULL;
        }
        
        //////////////////////////////////////////////////////////////////////////
        //
        //双向链表TBaseList定义
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcBaseList, TMdbNtcContainer);
        //缺省构造函数
        TMdbNtcBaseList::TMdbNtcBaseList()
        {
            //构建头节点
            m_pHeadNode = m_pTailNode = NULL;
            m_uiSize = 0;
        }

        TMdbNtcBaseList::TMdbNtcBaseList(const TMdbNtcBaseList& oSrcList)
        {
            m_pHeadNode = m_pTailNode = NULL;
            m_uiSize = 0;
            if(!oSrcList.IsEmpty())
            {
                this->AddTail(oSrcList.IterBegin(), oSrcList.IterEnd());
            }
        }

        TMdbNtcBaseList::TMdbNtcBaseList(const TMdbNtcContainer* pSrcContainer)
        {
            m_pHeadNode = m_pTailNode = NULL;
            m_uiSize = 0;
            if(pSrcContainer && !pSrcContainer->IsEmpty())
            {
                this->AddTail(pSrcContainer->IterBegin(), pSrcContainer->IterEnd());
            }
        }

        TMdbNtcBaseList::~TMdbNtcBaseList()
        {
            Clear();
        }

        TMdbNtcBaseList& TMdbNtcBaseList::operator =(const TMdbNtcContainer& oSrcContainer)
        {
            m_pHeadNode = m_pTailNode = NULL;
            m_uiSize = 0;
            if(!oSrcContainer.IsEmpty())
            {
                this->AddTail(oSrcContainer.IterBegin(), oSrcContainer.IterEnd());
            }
            return *this;
        }
        
        MDB_UINT32 TMdbNtcBaseList::GetContainerMemoryUsage() const
        {
            return (MDB_UINT32)(sizeof(TMdbNtcBaseList)+sizeof(TNode)*m_uiSize);
        }
        
        //清空容器
        void TMdbNtcBaseList::Clear()
        {
            TNode *pCurNode = m_pHeadNode, *pPrevNode = NULL;
            m_pTailNode = m_pHeadNode = NULL;
            m_uiSize = 0;
            while (pCurNode)
            {
                pPrevNode = pCurNode;
                pCurNode = pCurNode->pNext;
                MdbNtcReleaseData(pPrevNode->pData);
                delete pPrevNode;
                pPrevNode = NULL;
            }
        }
        
        //添加节点到链表头部
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::AddHead(TMdbNtcBaseObject* pData)
        {
            //创建节点
            TNode *pNewNode = new TNode(pData);
            pNewNode->pNext = m_pHeadNode;
            if(m_pHeadNode)
            {
                m_pHeadNode->pPrev = pNewNode;
            }
            m_pHeadNode = pNewNode;
            if(m_pTailNode == NULL) m_pTailNode = pNewNode;
            ++m_uiSize;
            return pNewNode;
        }
        
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::AddHead(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd)
        {
            TMdbNtcBaseList::TNode* pNode = NULL;
            for (TMdbNtcContainer::iterator itor = itSrcBegin; itor != itSrcEnd; ++itor)
            {
                if(itor == itSrcBegin)
                {
                    pNode = AddHead(itor.data());
                }
                else
                {
                    pNode = InsertAfter(pNode, itor.data());
                }
            }
            return GetHead();
        }
        
        //在尾部添加节点
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::AddTail(TNode* pNewNode)
        {
            pNewNode->pPrev = m_pTailNode;
            if(m_pHeadNode == NULL) m_pHeadNode = pNewNode;
            if(m_pTailNode)
            {
                m_pTailNode->pNext = pNewNode;
            }
            m_pTailNode = pNewNode;
            ++m_uiSize;
            return pNewNode;
        }
        
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::AddTail(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd /* = ms_itorEnd */)
        {
            TMdbNtcBaseList::TNode* pNode = GetTail();
            for (TMdbNtcContainer::iterator itor = itSrcBegin; itor != itSrcEnd; ++itor)
            {
                AddTail(itor.data());
            }
            return pNode?pNode->pNext:GetHead();
        }
        
        //添加到参考节点的前面，pNode为确定已在链表中存在中的节点
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::InsertBefore(TMdbNtcBaseList::TNode* pCurNode, TMdbNtcBaseObject* pData)
        {
            if(pCurNode == NULL) return AddTail(pData);
            TNode * pNewNode  = new TNode(pData);
            pNewNode->pPrev = pCurNode->pPrev;
            pNewNode->pNext = pCurNode;    
            if(pCurNode->pPrev)
            {
                pCurNode->pPrev->pNext = pNewNode;
            }
            pCurNode->pPrev = pNewNode;
            if(m_pHeadNode == pCurNode)
            {
                m_pHeadNode = pNewNode;
            }
            ++m_uiSize;
            return pNewNode;
        }
        
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::InsertBefore(TMdbNtcBaseList::TNode* pCurNode, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd)
        {
            TMdbNtcBaseList::TNode* pNode = pCurNode->pPrev;
            for (TMdbNtcContainer::iterator itor = itSrcBegin; itor != itSrcEnd; ++itor)
            {
                InsertBefore(pCurNode, itor.data());
            }
            return pNode?pNode:GetHead();
        }
        
        //添加节点到参考节点的后面，pNode为确定已在链表中存在中的节点
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::InsertAfter(TMdbNtcBaseList::TNode* pCurNode, TMdbNtcBaseObject* pData)
        {
            if(pCurNode == NULL) return AddTail(pData);
            TNode * pNewNode = new TNode(pData);
            pNewNode->pPrev = pCurNode;
            pNewNode->pNext = pCurNode->pNext;    
            if(pCurNode->pNext)
            {
                pCurNode->pNext->pPrev = pNewNode;
            }
            pCurNode->pNext = pNewNode;
            if(m_pTailNode == pCurNode)
            {
                m_pTailNode = pNewNode;
            }
            ++m_uiSize;
            return pNewNode;
        }
        
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::InsertAfter(TMdbNtcBaseList::TNode* pCurNode, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd)
        {
            TMdbNtcBaseList::TNode* pNode = pCurNode, *pRetNode = NULL;
            for (TMdbNtcContainer::iterator itor = itSrcBegin; itor != itSrcEnd; ++itor)
            {
                pNode = InsertAfter(pNode, itor.data());
                if(pRetNode == NULL)
                {
                    pRetNode = pNode;
                }
            }
            return pRetNode;
        }
        
        //在指定位置插入节点
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::InsertAt(MDB_UINT32 iIndex, TMdbNtcBaseObject* pData)
        {
            if(iIndex < 0 || iIndex >= m_uiSize)
            {
                return AddTail(pData);
            }
            else if(iIndex == 0)
            {
                return AddHead(pData);
            }
            else
            {
                TNode* pCurNode = GetAt(iIndex);
                if(pCurNode)
                {
                    return InsertBefore(pCurNode, pData);
                }
                else
                {
                    return AddTail(pData);
                }
            }
        }
        
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::InsertAt(int iIndex, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd)
        {
            if(itSrcBegin == itSrcEnd)
            {
                return NULL;
            }
            else
            {
                TMdbNtcContainer::iterator itor = itSrcBegin;
                TMdbNtcBaseList::TNode* pRetNode = InsertAt((MDB_UINT32)iIndex, itor.data()), *pCurNode = pRetNode;
                for (++itor; itor != itSrcEnd; ++itor)
                {
                    pCurNode = InsertAfter(pCurNode, itor.data());
                }
                return pRetNode;
            }
        }
        
        //删除头结点
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::RemoveHead()
        {
            //非空链表
            if (m_pHeadNode)
            {                
                TNode * pTempNode = m_pHeadNode;
                m_pHeadNode = m_pHeadNode->pNext;
                if(m_pHeadNode) m_pHeadNode->pPrev = NULL;
                --m_uiSize;
                MdbNtcReleaseData(pTempNode->pData);
                delete pTempNode;
                if(m_pHeadNode == NULL)
                {
                    m_pTailNode = NULL;
                }
            }
            return m_pHeadNode;
        }
        
        //删除尾节点
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::RemoveTail()
        {
            if (m_pTailNode)
            {        
                TNode * pTempNode = m_pTailNode;
                m_pTailNode = m_pTailNode->pPrev;
                if(m_pTailNode) m_pTailNode->pNext = NULL;
                --m_uiSize;
                MdbNtcReleaseData(pTempNode->pData);
                delete pTempNode;
                if(m_pTailNode == NULL)
                {
                    m_pHeadNode = NULL;
                }
            }
            return m_pTailNode;
        }
        
        //连续删除若干个指定位置起的元素，删除的是有效数据节点
        int TMdbNtcBaseList::Remove(int iIndex, int iDelCount /* = 1 */)
        {
            //合法性判断
            if (iIndex < 0 || iIndex >= (int)m_uiSize)
            {
                return 0;
            }
            int iRetDelCount = 0;
            TNode* pCurNode = GetAt((MDB_UINT32)iIndex);
            if(pCurNode)
            {
                TNode* pPrevNode = pCurNode->pPrev, *pTempNode = NULL;
                while (pCurNode && (iDelCount == -1 || iRetDelCount < iDelCount))
                {
                    ++iRetDelCount;
                    pTempNode = pCurNode;        
                    pCurNode = pCurNode->pNext;
                    MdbNtcReleaseData(pTempNode->pData);
                    delete pTempNode;
                    pTempNode = NULL;
                }
                if(pPrevNode)
                {
                    pPrevNode->pNext = pCurNode;
                    if(pCurNode)
                    {
                        pCurNode->pPrev = pPrevNode;
                    }
                }
                else
                {
                    m_pHeadNode = pCurNode;
                    if(pCurNode)
                    {
                        pCurNode->pPrev = NULL;
                    }
                }
                if(pCurNode == NULL)//如果最后的节点已经为NULL，则需要修改Tail指针
                {
                    m_pTailNode = pPrevNode;
                }
            }
            m_uiSize -= (MDB_UINT32)(iRetDelCount);
            return iRetDelCount;
        }
        
        //移除匹配的节点
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::Remove(TNode* pNode)
        {
            if(pNode == NULL || m_uiSize == 0) return NULL;    
            if(pNode == m_pHeadNode)
            {
                return RemoveHead();
            }
            else if(pNode == m_pTailNode)
            {
                RemoveTail(); 
                return  NULL;
            }
            else
            {
                TNode* pNextNode =  pNode->pNext;
                pNode->pPrev->pNext = pNextNode;
                pNextNode->pPrev = pNode->pPrev;
                MdbNtcReleaseData(pNode->pData);
                delete pNode;
                pNode = NULL;
                --m_uiSize;
                return pNextNode;
            }    
        }
        
        //根据数据来移除匹配的节点
        int TMdbNtcBaseList::Remove(const TMdbNtcBaseObject &oData, int iDelCount /* = 1 */, const TMdbNtcObjCompare &oCompare /* = g_oMdbNtcObjectCompare */)
        {
            int iRetDelCount = 0;             //临时变量，实际删除的元素个数
            TNode* pCurNode = m_pHeadNode;
            while (pCurNode)
            {
                if (oCompare.Compare(pCurNode->pData , &oData ) == 0 )
                {
                    ++iRetDelCount;
                    TNode* pTempNode = pCurNode;            
                    pCurNode = pCurNode->pNext;
                    
                    
                    Remove(pTempNode);
                    if(iDelCount != -1 && iRetDelCount >= iDelCount)
                    {
                        break;
                    }
                }
                else
                {
                    pCurNode = pCurNode->pNext;
                }
            }
            return iRetDelCount;
        }
        
        //根据序号查找结点
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::GetAt(MDB_UINT32 uiIndex) const
        {
            //实参合法性判断
            if (uiIndex >= m_uiSize)
            {
                return NULL;
            }
            TNode * pDestNode = m_pHeadNode;
            //定位
            MDB_UINT32 i = 0;
            while ( pDestNode && i < uiIndex)
            {
                pDestNode = pDestNode->pNext;
                ++i;
            }
            return pDestNode;
        }
        
        //根据数据查找节点
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::FindNode(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare, TNode* pLastFoundNode /*= NULL*/) const
        {
            if(m_pHeadNode == NULL) return NULL;
            TNode * pCurNode = pLastFoundNode?pLastFoundNode->pNext:m_pHeadNode;
            while (pCurNode)
            {
                if (oCompare.Compare(pCurNode->pData , &oData ) == 0 )
                {
                    break;
                }
                pCurNode = pCurNode->pNext;
            }
            return pCurNode;
        }
        
        //根据节点查找位置
        int TMdbNtcBaseList::FindIndex(TNode* pNode) const
        {
            if(pNode == NULL) return -1;
            int iIndex = -1;    
            TNode * pDestNode = m_pHeadNode;
            while (pDestNode)
            {
                ++iIndex;
                if (pDestNode == pNode )
                {
                    return iIndex;
                }
                pDestNode = pDestNode->pNext;
            }
            return -1;
        }
        
        //根据节点数据查找位置
        int TMdbNtcBaseList::FindIndex(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare,MDB_UINT32 uiStart /* = 0*/) const
        {
            int iIndex = -1;
            TNode *pCurNode = NULL;//当前节点的位置
            if(uiStart < m_uiSize/2)//从前面定位到开始查询的位置
            {
                iIndex = 0;
                pCurNode = m_pHeadNode;
                while (pCurNode && iIndex < (int)uiStart)
                {
                    ++iIndex;
                    pCurNode = pCurNode->pNext;
                }
            }
            else//从后面定位到开始查询的位置
            {
                iIndex = (int)(m_uiSize-1);
                pCurNode = m_pTailNode;
                while (pCurNode && iIndex > (int)uiStart)
                {
                    --iIndex;
                    pCurNode = pCurNode->pPrev;
                }
            }
            while(pCurNode)
            {        
                if (oCompare.Compare(pCurNode->pData, &oData) == 0)
                {
                    return iIndex;
                }
                pCurNode = pCurNode->pNext;
                ++iIndex;
            }
            return -1;
        }
        
        //合并链表
        int TMdbNtcBaseList::Combine(int iDestIndex, TMdbNtcContainer* pSrcContainer, int iSrcStart /* = 0 */, int iSrcCount /* = -1 */)
        {
            MDB_UINT32 uiSrcSize = pSrcContainer->GetSize();
            
            //源容器为空,直接返回
            if (0 == uiSrcSize  )
            {
                return -1;
            }
            
            //实参规整--源数据拷贝数目规整，调整为开始元素起的所有元素个数
            if (iSrcCount <= -1)
            {
                iSrcCount = (int)uiSrcSize-iSrcStart;
            }
            
            //如果源容器要拷贝的数目超过最大大小
            if (iSrcStart+iSrcCount > (int)uiSrcSize)
            {
                iSrcCount = (int)uiSrcSize-iSrcStart;
            }
            if(iSrcCount <= 0)
            {
                return -1;
            }
            else
            {
                TNode* pDestNode = GetAt((MDB_UINT32)iDestIndex);
                iterator itorSrcBegin = pSrcContainer->IterBegin()+iSrcStart;
                iterator itor = itorSrcBegin;
                for (int i = 0;  i < iSrcCount && itor.pNodeObject; ++itor, ++i)
                {
                    if(i == 0)
                    {
                        pDestNode = InsertBefore(pDestNode, itor.data());
                    }
                    else
                    {
                        pDestNode = InsertAfter(pDestNode, itor.data());
                    }            
                }
                bool bAutoRelease = pSrcContainer->IsAutoRelease();
                pSrcContainer->SetAutoRelease(false);
                pSrcContainer->IterErase(itorSrcBegin, itor);
                pSrcContainer->SetAutoRelease(bAutoRelease);
                return iDestIndex;
            }
        }
        
        //生成数组
        void TMdbNtcBaseList::GenerateArray(TMdbNtcAutoArray& arrayData) const
        {
            if (m_uiSize > 0 )
            {
                //预留
                arrayData.Reserve(m_uiSize);
                TNode * pCurNode = m_pHeadNode;
                while (pCurNode != NULL)
                {
                    arrayData.Add(pCurNode->pData);
                    pCurNode = pCurNode->pNext;
                }
            }
        }
        
        //排序
        void TMdbNtcBaseList::Sort(const TMdbNtcSort& oSort, const TMdbNtcObjCompare &oCompare)
        {
            if (m_uiSize > 1)
            {
                TMdbNtcAutoArray AutoArray;
                GenerateArray(AutoArray);        
                oSort.Sort(AutoArray.GetData(), AutoArray.GetSize(), oCompare);
                
                iterator itor = AutoArray.IterBegin(), itEnd = AutoArray.IterEnd();        
                TNode *pCurNode = m_pHeadNode;
                for (; itor != itEnd; ++itor)
                {
                    pCurNode->pData = itor.data();
                    pCurNode = pCurNode->pNext;
                }
            }
        }
        
        //移除数据重复的节点
        int TMdbNtcBaseList::RemoveDup(const TMdbNtcObjCompare &oCompare)
        {
            
            int iCount = 0;
            if (m_uiSize > 1)
            {
                TNode * pOutNode = m_pHeadNode;  //外层循环节点指针
                TNode * pInNode;   //内层循环节点指针        
                while (pOutNode != NULL)
                {
                    pInNode = pOutNode->pNext;
                    while (pInNode != NULL )
                    {
                        if (oCompare.Compare(pOutNode->pData, pInNode->pData) == 0 )
                        {
                            ++iCount;
                            
                            
                            pInNode = Remove(pInNode);
        
                        }
                        else
                        {
                            pInNode = pInNode->pNext;
                        }
                    }
                    pOutNode = pOutNode->pNext;
                }      
            }   
            return iCount;
        }
        
        //交换两个链表
        void TMdbNtcBaseList::SwapList(TMdbNtcBaseList  &SrcList)
        {
            TNode * pTempHead;
            TNode * pTempTail;
            unsigned int uiTemp;
            
            pTempHead = m_pHeadNode;
            pTempTail = m_pTailNode;
            uiTemp = m_uiSize;
        
            m_pHeadNode = SrcList.m_pHeadNode;
            m_pTailNode = SrcList.m_pTailNode;
            m_uiSize    = SrcList.m_uiSize;
        
            SrcList.m_pHeadNode = pTempHead;
            SrcList.m_pTailNode = pTempTail;
            SrcList.m_uiSize    = uiTemp;
            
        }
        //获得开始迭代器
        TMdbNtcBaseList::iterator TMdbNtcBaseList::IterBegin() const
        {
            return iterator(this, m_uiSize == 0 ? NULL:m_pHeadNode);
        }
        
        //获得尾元素迭代器
        TMdbNtcBaseList::iterator TMdbNtcBaseList::IterLast() const
        {
            return iterator(this,m_uiSize == 0? NULL:m_pTailNode);
        }
        
        //移除迭代器对应的节点,返回下一个迭代器
        TMdbNtcBaseList::iterator TMdbNtcBaseList::IterErase(TMdbNtcBaseList::iterator itor)
        {
            TNode * pDelNode = (TNode*)itor.pNodeObject;
            if(pDelNode == NULL) return IterEnd();
            else
            {
                ++itor;
                Remove(pDelNode);
                return itor;
            }
        }
        
        //通过迭代器交换数据
        void TMdbNtcBaseList::IterSwap(TMdbNtcBaseList::iterator itor1, TMdbNtcBaseList::iterator itor2)
        {
            if (itor1.pNodeObject&& itor2.pNodeObject)
            {
                TMdbNtcBaseObject * pTemp = itor1.pNodeObject->pData;
                itor1.pNodeObject->pData= itor2.pNodeObject->pData;
                itor2.pNodeObject->pData= pTemp;
            } 
        }
        
        //获得前一个迭代器,找不到和超出范围的话使用IterEnd比较好,且复用itCur这个迭代器，以免丢失父容器的信息
        TMdbNtcBaseList::iterator TMdbNtcBaseList::IterPrev(TMdbNtcBaseList::iterator itCur, int iStep /* = -1 */) const
        {
            if(itCur.pNodeObject == NULL) return itCur;
            itCur.iLastStep = 0;
            if (iStep <= -1)
            {
                TNode * pCurNode = (TNode*)itCur.pNodeObject;
                while(1)
                {
                    pCurNode = pCurNode->pPrev;
                    if(pCurNode == NULL)
                    {
                        break;
                    }
                    --itCur.iLastStep;
                    if(itCur.iLastStep == iStep)
                    {
                        break;
                    }
                }
                itCur.pNodeObject = pCurNode;
                return itCur;
            }
            else if (iStep > 0)
            {
                return IterNext(itCur,iStep);
            }
            else
            {
                return itCur;
            }
        }
        
        //获得后一个迭代器
        TMdbNtcBaseList::iterator TMdbNtcBaseList::IterNext(TMdbNtcBaseList::iterator itCur, int iStep /* = 1 */) const
        {
            if(itCur.pNodeObject == NULL) return itCur;
            itCur.iLastStep = 0;
            if (iStep >= 1)
            {
                TNode * pCurNode = (TNode*)itCur.pNodeObject;
                while(1)
                {
                    pCurNode = pCurNode->pNext;
                    if(pCurNode == NULL)
                    {
                        break;
                    }
                    ++itCur.iLastStep;
                    if(itCur.iLastStep == iStep)
                    {
                        break;
                    }
                }
                itCur.pNodeObject = pCurNode;
                return itCur;
            }
            else if (iStep < 0)
            {
                return IterPrev(itCur, iStep);
            }
            else
            {
                return itCur;
            }
        }
        
        //////////////////////////////////////////////////////////////////////////
        //
        //平衡二叉树TAvlTree定义
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcAvlTree, TMdbNtcContainer);
        //树结点构造函数
        TMdbNtcAvlTree::TNode::TNode(TMdbNtcBaseObject *pData /* = NULL */)
        {
            this->pData = pData;
            iBalanceFactor = 0;
            pLeftSubNode = NULL;
            pRightSubNode = NULL;
            pParentNode = NULL;
        }

        void TMdbNtcAvlTree::TNode::SwapNode(TNode* pSwapNode)
        {
            TMdbNtcBaseObject* pTempData = this->pData;
            this->pData = pSwapNode->pData;
            pSwapNode->pData = pTempData;
        }

        //构造函数
        TMdbNtcAvlTree::TMdbNtcAvlTree(TMdbNtcObjCompare *pCompare /* = NULL */)
        {
            m_pObjCompare = pCompare;
            m_iBalanceFactor = 1;
            m_pRootNode = NULL;
            m_iAvlFlag = 0;
            m_iAvlHeight = 0;
            m_uiSize = 0;
        }
        
        TMdbNtcAvlTree& TMdbNtcAvlTree::operator = (const TMdbNtcContainer& oSrcContainer)
        {
            Clear();
            if(!oSrcContainer.IsEmpty())
            {
                //从源容器中拷贝数据，跨容器使用迭代器
                Add(oSrcContainer.IterBegin(), oSrcContainer.IterEnd());
            }
            return *this;
        }

        TMdbNtcAvlTree::~TMdbNtcAvlTree()
        {
            Clear();
            if(m_pObjCompare != NULL && m_pObjCompare != &g_oMdbNtcObjectCompare
                && m_pObjCompare != &g_oMdbNtcPointerCompare)
            {
                delete m_pObjCompare;
                m_pObjCompare = NULL;
            }
        }

        void TMdbNtcAvlTree::Print(FILE* fp /* = NULL */) const
        {
            if(fp == NULL) 
            {
                fp = stdout;
            }
            iterator itor = IterBegin(), itEnd = IterEnd();
            for (;itor != itEnd; ++itor)
            {
                fprintf(fp, "%s\n", itor.pNodeObject->ToString().c_str());
            }
        }
        
        MDB_UINT32 TMdbNtcAvlTree::GetContainerMemoryUsage() const
        {
            //将m_pObjCompare指向所占用的内存按照TObjCompare来估算
            return (MDB_UINT32)(sizeof(TMdbNtcAvlTree)+sizeof(TNode)*m_uiSize+(m_pObjCompare?sizeof(TMdbNtcObjCompare):0));
        }
        
        //清空容器
        void TMdbNtcAvlTree::Clear()
        {
            if(m_pRootNode)
            {
                //清空平衡二叉树
                ClearAvlTree(m_pRootNode);
                //重置部分成员变量
                m_pRootNode = NULL;
                m_uiSize = 0;
                m_iAvlFlag = 0;
                m_iAvlHeight = 0;
            }
        }
        
        //后序遍历清除平衡二叉树
        void  TMdbNtcAvlTree::ClearAvlTree(TMdbNtcAvlTree::TNode * pParentNode)
        {
            if (pParentNode)
            {
                //清除左子树
                ClearAvlTree(pParentNode->pLeftSubNode);
                //清除右子树
                ClearAvlTree(pParentNode->pRightSubNode);
                //清除根节点
                MdbNtcReleaseNode(pParentNode);
            }
        }
        
        TMdbNtcAvlTree::TNode* TMdbNtcAvlTree::GetLeftMostNode(TNode* pParentNode) const
        {
            if(pParentNode == NULL) return NULL;
            TNode* pLeftMostNode = pParentNode;
            while (pLeftMostNode->pLeftSubNode)
            {
                pLeftMostNode = pLeftMostNode->pLeftSubNode;
            }
            return pLeftMostNode;
        }
        
        TMdbNtcAvlTree::TNode* TMdbNtcAvlTree::GetRightMostNode(TNode* pParentNode) const
        {
            if(pParentNode == NULL) return NULL;
            TNode* pRightMostNode = pParentNode;
            while (pRightMostNode->pRightSubNode)
            {
                pRightMostNode = pRightMostNode->pRightSubNode;
            }
            return pRightMostNode;
        }
        
        //添加新节点
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::Add(TMdbNtcBaseObject* pData)
        {
            TNode* pRetNode = NULL;
            if(m_pRootNode == NULL)
            {
                ++m_uiSize;
                pRetNode = m_pRootNode = new TNode(pData);
            }
            else
            {
                TNode* pNewNode = new TNode(pData);
                pRetNode = InsertNode(&m_pRootNode, pNewNode);
            }    
            if (m_iAvlFlag != 0)
            {
                m_iAvlHeight += m_iAvlFlag;
            }
            return iterator(this, pRetNode);
        }

        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::Add(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd /* = ms_itorEnd */)
        {
            TMdbNtcContainer::iterator itRet = IterEnd();
            for (iterator itor = itSrcBegin; itor != itSrcEnd; ++itor)
            {
                itRet = Add(itor.data());
            }
            return itRet;
        }
        
        //删除数据匹配的节点
        int TMdbNtcAvlTree::Remove(const TNode &oNode, int iDelCount /* = 1 */)
        {
            if(m_uiSize == 0) return 0;
            int iRetDelCount = 0;
            iterator itor = IterEnd(), itEnd = IterEnd();
            do
            {
                itor = IterFind(oNode, itor);
                if(itor == itEnd)
                {
                    break;
                }
                ++iRetDelCount;                
                itor = IterErase(itor);
                if(itor == itEnd)
                {
                    break;
                }
                --itor;
            }while(iDelCount == -1 || iRetDelCount < iDelCount);
            return iRetDelCount;
        }
        
        //删除树节点,返回的结果为下一个节点的迭代器
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::Remove(TNode* pAvlNode)
        {
            /*如果节点指针为空或者树为空，返回空迭代器*/
             if(pAvlNode == NULL || m_pRootNode == NULL)
            {
                return IterEnd();
            }
         
            /*变量声明*/
            int iBalance ;              //平衡度
            bool bIsLeftChild = false;  //左孩子右孩子标识，true--待处理节点是父节点的左孩子，false--待处理节点是父节点的右孩子
            bool bIsChangedData = false;//节点交换数据标志，true--发生过数据交换，false--未发生过数据交换
            TNode * pNextNode;          //保存节点交换数据情况下，保存待删除节点的后继节点的指针
            TNode * pNode;              //过渡结点指针
            iterator itor;              //返回的迭代器
            /* 删除节点*/ 
            while(pAvlNode != NULL)
            {
                //左孩子为空
                if(pAvlNode->pLeftSubNode == NULL)
                {
                    m_iAvlFlag = -1; /* 高度减少 */
        
                    --m_uiSize;      /* 容器大小减1*/
                    
                    //如果删除的节点是根节点，则处理完成后直接返回,无须调整平衡
                    if (pAvlNode->pParentNode == NULL )
                    {
                        m_pRootNode = pAvlNode->pRightSubNode;
                        if(m_pRootNode != NULL )
                        {
                            m_pRootNode->pParentNode = NULL;
                        }
                        MdbNtcReleaseNode(pAvlNode);        
                        //调整树高度
                        m_iAvlHeight += m_iAvlFlag;
        
                        return iterator(this,m_pRootNode);
                    } 
                    else
                    {
                        //保存待删除节点父节点的平衡度
                        iBalance = pAvlNode->pParentNode->iBalanceFactor;
        
                        //判断是父节点的左孩子还是右孩子
                        if(pAvlNode->pParentNode->pLeftSubNode == pAvlNode)
                        {
                            bIsLeftChild = true;
                        }
                        else
                        {
                            bIsLeftChild = false;
                        }
                        
                        //获取待删除节点的后继节点的迭代器
                        if (!bIsChangedData)
                        {
                            //如果发生过数据交换，已经获取其后继节点的迭代器，无须再获取
                           itor = iterator(this,pAvlNode);
                           ++itor;
                        }
        
                        //保存待删除节点指针
                        pNode = pAvlNode;
        
                        if(bIsLeftChild)
                        {
                            //待删除节点的父节点的左孩子指向待删除节点的右孩子
                            pAvlNode->pParentNode->pLeftSubNode=pAvlNode->pRightSubNode;
                            //修改父节点平衡度
                            pAvlNode->pParentNode->iBalanceFactor  -= m_iAvlFlag;
                        }
                        else
                        {
                            //待删除节点的父节点的右孩子指向待删除节点的右孩子
                            pAvlNode->pParentNode->pRightSubNode=pAvlNode->pRightSubNode;
                            //修改父节点平衡度
                            pAvlNode->pParentNode->iBalanceFactor  += m_iAvlFlag;
                        }
                        
                        //指向右孩子
                        pAvlNode = pAvlNode->pRightSubNode;
        
                        //待删除节点非叶子节点，重置右孩子的父指针
                        if (pAvlNode != NULL)
                        {
                            pAvlNode->pParentNode = pNode->pParentNode; 
                        }
        
                        //指向待删除节点的父节点
                        pAvlNode = pNode->pParentNode;
                        MdbNtcReleaseNode(pNode);        
                        break;
                    }           
                }
                //右孩子为空
                else if(pAvlNode->pRightSubNode== NULL)
                {
                    m_iAvlFlag = -1; /* 高度减少 */
        
                    --m_uiSize;      /* 容器大小减1*/
        
                    //如果删除的节点是根节点，则处理完成后直接返回,无须调整平衡
                    if (pAvlNode->pParentNode == NULL)
                    {
                        //这种情况下左孩子一定不为空
                        m_pRootNode = pAvlNode->pLeftSubNode;
                        m_pRootNode->pParentNode = NULL;
                        MdbNtcReleaseNode(pAvlNode);        
                        //重置最右节点指针，最左节点指针未变动无须重置
                        //m_pRightMostNode = m_pRootNode;
                       
                        //调整树高度
                        m_iAvlHeight += m_iAvlFlag;
        
                        return iterator(this);
                    } 
                    else
                    {
                        //保存待删除节点父节点的平衡度
                        iBalance = pAvlNode->pParentNode->iBalanceFactor;
        
                        //判断是父节点的左孩子还是右孩子
                        if(pAvlNode->pParentNode->pLeftSubNode == pAvlNode)
                        {
                            bIsLeftChild = true;
                        }
                        else
                        {
                            bIsLeftChild = false;
                        }
        
                        //获取待删除节点的后继节点的迭代器
                        if (!bIsChangedData)
                        {
                            //如果发生过数据交换，已经获取其后继节点的迭代器，无须再获取
                            itor = iterator(this,pAvlNode);
                            ++itor;
                        }
        
                        //保存待删除节点指针
                        pNode = pAvlNode;
        
                        if(bIsLeftChild)
                        {
                            //待删除节点的父节点的左孩子指向待删除节点的左孩子
                            pAvlNode->pParentNode->pLeftSubNode=pAvlNode->pLeftSubNode;
                            //修改父节点平衡度
                            pAvlNode->pParentNode->iBalanceFactor  -= m_iAvlFlag;
                        }
                        else
                        {
                            //待删除节点的父节点的右孩子指向待删除节点的左孩子
                            pAvlNode->pParentNode->pRightSubNode=pAvlNode->pLeftSubNode;
                            //修改父节点平衡度
                            pAvlNode->pParentNode->iBalanceFactor  += m_iAvlFlag;
                        }
        
                        //指向左孩子
                        pAvlNode = pAvlNode->pLeftSubNode;
        
                        //待删除节点非叶子节点，重置左孩子的父指针
                        if (pAvlNode != NULL)
                        {
                            pAvlNode->pParentNode = pNode->pParentNode; 
                        }
                        
                        //指向待删除节点的父节点
                        pAvlNode = pNode->pParentNode;
                        MdbNtcReleaseNode(pNode);        
                        break;        
                    }            
                }
                else
                {
                    //左子树高，则交换节点pParentNode和其前驱节点的数据，并删除前驱节点
                    //反之，若右子树高，则交换节点pParentNode和其后继节点的数据，并删除后继节点
        
                    if(pAvlNode->iBalanceFactor< 0 ) 
                    {
                        //获取后继节点的迭代器
                        pNextNode = pAvlNode->pRightSubNode;
                        while (pNextNode->pRightSubNode != NULL)
                        {
                            pNextNode = pNextNode->pLeftSubNode;
                        }
                        itor = iterator(this,pNextNode);
        
                        /* 找到前驱节点 ，前驱节点为左子树的最右节点*/
                        pNode = pAvlNode->pLeftSubNode; 
                        while (pNode->pRightSubNode != NULL)
                        {
                            pNode = pNode->pRightSubNode;
                        }
        
                        /* 交换数据区 */
                        pAvlNode->SwapNode(pNode);
                        
                        //设置交换后的结点为待删除节点
                        pAvlNode = pNode;
                    } 
                    else 
                    {
                        /* 找到后继节点 ，后继节点为右子树的最左节点*/
                        pNode = pAvlNode->pRightSubNode; 
                        while (pNode->pLeftSubNode != NULL)
                        {
                            pNode = pNode->pLeftSubNode;
                        }
                        
                        //获取后继节点的迭代器,交换数据后，就是节点pAvlNode本身
                        itor = iterator(this,pAvlNode);
        
                        /* 交换数据区 */
                        pAvlNode->SwapNode(pNode);

                        //设置交换后的结点为待删除节点
                        pAvlNode = pNode;
                    }
        
                    bIsChangedData = true;
                }     
            }
        
            /*调整平衡*/
            while(pAvlNode != NULL)
            {
                //如果是pAvlNode是根节点，直接调整平衡
                if (pAvlNode->pParentNode == NULL)
                {
                    //调整平衡
                    if (MDB_ABS(m_iAvlFlag) >= m_iBalanceFactor)
                    {
                        m_pRootNode = BalanceTree(pAvlNode,iBalance);
                    }
                    break;
                } 
                else
                {
                    //保存父节点指针
                    pNode = pAvlNode->pParentNode;
        
                    //判断是父节点的左孩子还是右孩子
                    if(pNode->pLeftSubNode == pAvlNode)
                    {
                        bIsLeftChild = true;
                    }
                    else
                    {
                        bIsLeftChild = false;
                    }
        
                    //调整平衡
                    if (MDB_ABS(m_iAvlFlag) >= m_iBalanceFactor)
                    {
                        pAvlNode = BalanceTree(pAvlNode,iBalance);
                        //父节点的孩子节点指向平衡后的子树
                        if (bIsLeftChild)
                        {
                            pNode->pLeftSubNode = pAvlNode;
                        } 
                        else
                        {
                            pNode->pRightSubNode = pAvlNode;
                        }
                    }
        
                    //调整后保存父节点的平衡度，然后调整父节点的平衡度
                    iBalance = pAvlNode->pParentNode->iBalanceFactor;
                    if (bIsLeftChild)
                    {
                        pAvlNode->pParentNode->iBalanceFactor -= m_iAvlFlag;
                    }
                    else
                    {
                        pAvlNode->pParentNode->iBalanceFactor  += m_iAvlFlag;
                    }
                    //指向父节点
                    pAvlNode = pAvlNode->pParentNode;
                }              
            }
        
        
            //调整树的高度
            if (m_iAvlFlag != 0)
            {
                m_iAvlHeight += m_iAvlFlag;
            }
            return itor;
        }
        
        //获取开始迭代器
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::IterBegin() const
        {
            return iterator(this, m_pRootNode?GetLeftMostNode(m_pRootNode):NULL);
        }
        
        //获得尾元素迭代器
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::IterLast() const
        {
            return iterator(this, m_pRootNode?GetRightMostNode(m_pRootNode):NULL);
        }
        
        //移除迭代器对应的节点，返回下一个迭代器
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::IterErase(TMdbNtcAvlTree::iterator itor)
        {
            
            if (m_pRootNode == NULL || itor.pNodeObject== NULL)
            {
                return IterEnd();
            }
            return Remove((TNode *)(itor.pNodeObject));
        }
        
        //通过迭代器交换，完成元素和节点的交换
        void TMdbNtcAvlTree::IterSwap(TMdbNtcAvlTree::iterator itor1, TMdbNtcAvlTree::iterator itor2)
        {
            //交换无意义，直接返回
            return;
        }
        
        //根据数据查找迭代器
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::IterFind(const TNode &oNode, iterator itLastFound /*= ms_itorEnd*/) const
        {            
            iterator itRet = itLastFound.pNodeObject?(++itLastFound):LowerBound(oNode);
            if (itRet.pNodeObject)
            {
                if (itRet.pNodeObject->pData
                    && m_pObjCompare?(m_pObjCompare->Compare(oNode.pData, itRet.pNodeObject->pData) == 0):(oNode.Compare(itRet.pNodeObject) == 0))
                {
                    return  itRet;
                }
                else
                {
                    return IterEnd();
                }
            }
            else
            {
                return itRet;
            }
        }
        
        //获得前一个迭代器
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::IterPrev(TMdbNtcAvlTree::iterator itCur, int iStep /* = -1 */) const
        {
            if (m_pRootNode == 0)
            {
                return IterEnd();
            }    
        
            if (iStep <= -1 )
            {
                itCur.iLastStep = 0;
                TNode * pCurNode = (TNode *)itCur.pNodeObject;
                while (pCurNode != NULL && itCur.iLastStep > iStep/* && pCurNode != m_pLeftMostNode*/)
                {
                    //若左孩子不为空，则左子树的最右节点是当前节点在中序遍历中该节点的前驱
                    //若左孩子为空，则父节点,祖父节点.....中第一个小于该节点是当前节点在中序遍历中该节点的前驱
                    if (pCurNode->pLeftSubNode != NULL)
                    {
                        pCurNode = GetRightMostNode(pCurNode->pLeftSubNode);
                        --itCur.iLastStep;
                    } 
                    else
                    {
                        TNode* pParentNode = pCurNode->pParentNode;
                        do
                        {
                            //如果当前节点的父节点为空，则表示已经处于top，则next需要到左边树寻找
                            if(pParentNode == NULL)
                            {
                                pCurNode = GetRightMostNode(pCurNode->pLeftSubNode);
                            }
                            //如果当前节点是父节点的右子节点，则cur赋值为parent，
                            else if(pCurNode == pParentNode->pRightSubNode)
                            {
                                pCurNode = pParentNode;
                                --itCur.iLastStep;
                                break;
                            }
                            //如果当前节点是父节点的左子节点，则继续上溯
                            else
                            {
                                pCurNode = pParentNode;
                                pParentNode = pParentNode->pParentNode;
                                if(pParentNode == NULL)
                                {
                                    pCurNode = NULL;
                                    --itCur.iLastStep;
                                    break;
                                }
                            }
                        } while(pCurNode);
                    }
                }
                itCur.pNodeObject = pCurNode;
                return itCur;
            }
            else if (iStep == 0)
            {
                return itCur;
            } 
            else
            {
                return IterNext(itCur,iStep);
            }
        }
        
        //获得后一个迭代器
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::IterNext(TMdbNtcAvlTree::iterator itCur, int iStep /* = 1 */) const
        {
            if (m_pRootNode == 0 || itCur.pNodeObject == NULL)
            {
                return IterEnd();
            }
        
            if (iStep >= 1 )
            {
                itCur.iLastStep = 0;
                TNode * pCurNode = (TNode *)itCur.pNodeObject;
                while (pCurNode != NULL && itCur.iLastStep < iStep /*&& pCurNode != m_pRightMostNode*/)
                {
                    //若右孩子不为空，则右子树的最左节点是当前节点在中序遍历中该节点的后继
                    //若右孩子为空，则父节点，祖父节点...中第一个为父节点的左子节点即停止,此时的父节点即为下一个
                    if (pCurNode->pRightSubNode != NULL)
                    {
                        pCurNode = GetLeftMostNode(pCurNode->pRightSubNode);
                        ++itCur.iLastStep;
                    }
                    else
                    {
                        TNode* pParentNode = pCurNode->pParentNode;
                        do
                        {
                            //如果当前节点的父节点为空，则表示已经处于top，则next需要到右边树寻找
                            if(pParentNode == NULL)
                            {
                                pCurNode = GetLeftMostNode(pCurNode->pRightSubNode);
                            }
                            //如果当前节点是父节点的左子节点，则cur赋值为parent，
                            else if(pCurNode == pParentNode->pLeftSubNode)
                            {
                                pCurNode = pParentNode;
                                ++itCur.iLastStep;
                                break;
                            }
                            //如果当前节点是父节点的右子节点，则继续上溯
                            else
                            {
                                pCurNode = pParentNode;
                                pParentNode = pParentNode->pParentNode;
                                if(pParentNode == NULL)
                                {
                                    pCurNode = NULL;
                                    ++itCur.iLastStep;
                                    break;
                                }
                            }
                        } while(pCurNode);
                    }
                }
                itCur.pNodeObject = pCurNode;
                return itCur;       
            }
            else if (iStep == 0)
            {
                return itCur;
            } 
            else
            {
                return IterPrev(itCur,iStep);
            }
        }
        
        //根据数据信息，返回一个迭代器，指向不小于pData的第一个元素
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::LowerBound(const TNode &oNode) const
        {
            if (m_pRootNode == NULL)
            {
                return IterEnd();
            }
            TNode *pCurNode = m_pRootNode;
            do 
            {
                MDB_INT64 iCmpRet = 0;
                if(m_pObjCompare) iCmpRet = m_pObjCompare->Compare(pCurNode->pData, oNode.pData);
                else iCmpRet = pCurNode->Compare(&oNode);
                if (iCmpRet >= 0)
                {
                    //当前结点左孩子为空，表明当前节点是大于参考值的第一个节点
                    if (pCurNode->pLeftSubNode == NULL)
                    {
                        return iterator(this,pCurNode);                
                    }
                    else
                    {
                        pCurNode = pCurNode->pLeftSubNode;
                    }
                } 
                else
                {
                    //当前节点的右孩子为空，表明中序遍历中该节点的后继就是大于参考值的第一个节点
                    if (pCurNode->pRightSubNode == NULL )
                    {
                        iterator itRetor(this,pCurNode);
                        return ++itRetor;
                    }
                    else
                    {
                        pCurNode = pCurNode->pRightSubNode;
                    }
                }
            } while (pCurNode);
            return iterator(this, pCurNode);
        }
        
        //根据数据信息，返回一个迭代器，指向大于pData的第一个元素
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::UpperBound(const TNode &oNode) const
        {
            TNode *pCurNode = m_pRootNode;
            while (pCurNode)
            {
                MDB_INT64 iCmpRet = 0;
                if(m_pObjCompare) iCmpRet = m_pObjCompare->Compare(pCurNode->pData, oNode.pData);
                else iCmpRet = pCurNode->Compare(&oNode);
                //取中序遍历中该节点的后继返回
                if (iCmpRet > 0)
                {
                    //当前结点左孩子为空，表明当前节点是大于参考值的第一个节点
                    if (pCurNode->pLeftSubNode == NULL)
                    {
                        break;
                    }
                    else
                    {
                        pCurNode = pCurNode->pLeftSubNode;
                    }
                } 
                else
                { 
                    //当前节点的右孩子为空，表明中序遍历中该节点的后继就是大于参考值的第一个节点
                    if (pCurNode->pRightSubNode == NULL )
                    {
                        iterator itRetor(this,pCurNode);
                        return  ++itRetor;
                    } 
                    else
                    {
                        pCurNode = pCurNode->pRightSubNode;
                    }
                }
            }
            return iterator(this, pCurNode);
        }

        //得到与此数据相等的开始和结束迭代器
        std::pair<TMdbNtcAvlTree::iterator, TMdbNtcAvlTree::iterator> TMdbNtcAvlTree::EqualRange(const TNode &oNode) const
        {
            iterator itor1,itor2;
            itor1 = LowerBound(oNode);
            itor2 = itor1;
            while (itor2.pNodeObject && itor2.pNodeObject->pData
                &&  m_pObjCompare?(m_pObjCompare->Compare(itor2.pNodeObject->pData,oNode.pData) == 0):(itor2.pNodeObject->Compare(&oNode) == 0)
                )
            {
                ++itor2;
            }
            std::pair<TMdbNtcAvlTree::iterator, TMdbNtcAvlTree::iterator> pr(itor1,itor2);
            return pr;
        }
        
        //插入数据
        TMdbNtcAvlTree::TNode* TMdbNtcAvlTree::InsertNode(TMdbNtcAvlTree::TNode** ppParentNode, TNode* pNewNode)
        {
            if(ppParentNode == NULL) return NULL;
            TNode* pRetNode = NULL, *pParentNode = *ppParentNode;
            int iBalance;
        
            iBalance = pParentNode->iBalanceFactor;
        
            MDB_INT64 iCmpRet = 0;
            if(m_pObjCompare) iCmpRet = m_pObjCompare->Compare(pParentNode->pData, pNewNode->pData);
            else iCmpRet = pParentNode->Compare(pNewNode);
        
            //待插入值小于父节点键值，插入到左子树
            if (iCmpRet > 0)
            {
                if (pParentNode->pLeftSubNode != NULL)
                {
                    pRetNode = InsertNode(&pParentNode->pLeftSubNode, pNewNode);
                    pParentNode->iBalanceFactor -= m_iAvlFlag ; /* 左子树高度增加 */
                }
                else
                {
                    pRetNode = pParentNode->pLeftSubNode = pNewNode; /* 建立新节点 */
                    pParentNode->pLeftSubNode->pParentNode = pParentNode;//指向父节点
                    m_iAvlFlag = 1; /* 高度增加 */
                    ++m_uiSize;     //容器大小增1
                    pParentNode->iBalanceFactor -= m_iAvlFlag ; /* 左子树高度增加 */
                }
            } 
            else 
            {
                if (pParentNode->pRightSubNode != NULL )
                {
                    pRetNode = InsertNode(&pParentNode->pRightSubNode, pNewNode);
                    pParentNode->iBalanceFactor   += m_iAvlFlag; /* 右子树高度增加 */
                } 
                else
                {
                    pRetNode = pParentNode->pRightSubNode = pNewNode;/* 建立新节点 */
                    pParentNode->pRightSubNode->pParentNode = pParentNode;//指向父节点
                    m_iAvlFlag = 1;  /* 高度增加 */
                    ++m_uiSize;     //容器大小增1
                    pParentNode->iBalanceFactor   += m_iAvlFlag; /* 右子树高度增加 */
                }
            }
        
            //将插入节点后的树调整为平衡树，如果是针对root进行平衡，则需要更新root节点指针
            *ppParentNode = BalanceTree(pParentNode, iBalance);
            return pRetNode;
        }
        
        //平衡树节点
        TMdbNtcAvlTree::TNode* TMdbNtcAvlTree::BalanceTree(TMdbNtcAvlTree::TNode* pParentNode, int iBalance)
        {
            if (m_iAvlFlag > 0) 
            {
                //左子树过高于右子树: 需要右旋
                if(pParentNode->iBalanceFactor < -m_iBalanceFactor ) 
                {
                    if(pParentNode->pLeftSubNode->iBalanceFactor > 0) 
                    {
                        if (pParentNode->pLeftSubNode->iBalanceFactor > 1 && pParentNode->pLeftSubNode->pRightSubNode->iBalanceFactor > 0) 
                            pParentNode->iBalanceFactor += 1;
                        pParentNode->pLeftSubNode = LeftRotate(pParentNode->pLeftSubNode);
                    }
                    pParentNode = RightRotate(pParentNode);
                    m_iAvlFlag = 0; /* 本节点的高度不增加 */
                }
                //右子树过高于左子树: 需要左旋
                else if(pParentNode->iBalanceFactor > m_iBalanceFactor) 
                {
                    if(pParentNode->pRightSubNode->iBalanceFactor < 0) 
                    {
                        if (pParentNode->pRightSubNode->iBalanceFactor < -1 && pParentNode->pRightSubNode->pLeftSubNode->iBalanceFactor <0)
                            pParentNode->iBalanceFactor -= 1;
                        pParentNode->pRightSubNode = RightRotate(pParentNode->pRightSubNode);
                    }
                    pParentNode = LeftRotate(pParentNode);
                    m_iAvlFlag = 0; /* 本节点的高度不增加 */
                }
                else if((iBalance > 0 && iBalance > pParentNode->iBalanceFactor) ||
                    (iBalance < 0 && iBalance < pParentNode->iBalanceFactor))
                {
                    m_iAvlFlag = 0; /* 本节点的高度不增加 */
                }
                else
                {
                    m_iAvlFlag = 1; /* 本节点的高度增加 */
                }
            }
            else//(m_iAvlFlag <0)
            {   
                //左子树过高于右子树: 需要右旋
                if(pParentNode->iBalanceFactor < -m_iBalanceFactor) 
                {
                    if(pParentNode->pLeftSubNode->iBalanceFactor > 0) 
                    {
                        if (pParentNode->pLeftSubNode->iBalanceFactor > 1 && pParentNode->pLeftSubNode->pRightSubNode->iBalanceFactor > 0) 
                            pParentNode->iBalanceFactor += 1;
                        pParentNode->pLeftSubNode = LeftRotate(pParentNode->pLeftSubNode);
                        m_iAvlFlag = -1;
                    }
                    else if (pParentNode->pLeftSubNode->iBalanceFactor == 0)
                        m_iAvlFlag =  0;/* 本节点的高度不减少 */
                    else
                        m_iAvlFlag = -1;/* 本节点的高度减少 */
                    pParentNode = RightRotate(pParentNode);
                }
                //右子树过高于左子树: 需要左旋 
                else if(pParentNode->iBalanceFactor > m_iBalanceFactor) 
                {
                    if(pParentNode->pRightSubNode->iBalanceFactor < 0) 
                    { 
                        if (pParentNode->pRightSubNode->iBalanceFactor < -1 && pParentNode->pRightSubNode->pLeftSubNode->iBalanceFactor <0)
                            pParentNode->iBalanceFactor -= 1;
                        pParentNode->pRightSubNode = RightRotate(pParentNode->pRightSubNode);
                        m_iAvlFlag = -1;
                    }
                    else if (pParentNode->pRightSubNode->iBalanceFactor == 0)
                        m_iAvlFlag =  0;/* 本节点的高度不减少 */
                    else
                        m_iAvlFlag = -1;/* 本节点的高度减少 */
                    pParentNode = LeftRotate(pParentNode);
                }
                else if((iBalance > 0 && iBalance > pParentNode->iBalanceFactor) ||
                    (iBalance < 0 && iBalance < pParentNode->iBalanceFactor))
                {
                    m_iAvlFlag = -1; /* 本节点的高度减少 */
                }
                else
                {
                    m_iAvlFlag = 0; /* 本节点的高度不减少 */
                }
            }
            return pParentNode;
        }
        
        /*
        * 单向左旋例程
        *            X              Y
        *           / \            / \
        *          A   Y    ==>   X   C
        *             / \        / \
        *            B   C      A   B
        * 平衡因子变更分析
        * 旋转之前：
        * 假定 X 的平衡因子是 x, Y 的平衡因子是 y, 
        * 设 A 的高度为 h, 则 Y 的高度为 h+x 
        * 节点 B 高度为 h+x-1-max(y,0); 
        * 节点 C 的高度为 h+x-1+MIN(y,0);
        * 旋转之后：
        * 节点 X 的新平衡因子是 x-1-max(y,0); 
        * 节点 Y 的新平衡因子是 C-(max(A,B)+1) => MIN(C-A-1,C-B-1) 
        *     => MIN(x-2+MIN(y,0),y-1)
        */
        
        TMdbNtcAvlTree::TNode* TMdbNtcAvlTree::LeftRotate(TMdbNtcAvlTree::TNode* pNode)
        {
            TNode *pTempNode;
            int x,y;
        
            //左旋处理
            pTempNode = pNode;
            pNode = pNode->pRightSubNode;
            pNode->pParentNode = pTempNode->pParentNode;
        
            pTempNode->pRightSubNode = pNode->pLeftSubNode;
            if(pNode->pLeftSubNode) 
            {
                pNode->pLeftSubNode->pParentNode = pTempNode;
            }
        
            pNode->pLeftSubNode = pTempNode;
            pTempNode->pParentNode = pNode;
        
            //获取旧平衡因子
            x = pTempNode->iBalanceFactor;
            y = pNode->iBalanceFactor;
        
            //设置新平衡因子
            pTempNode->iBalanceFactor = x-1-MDB_MAX(y, 0);
            pNode->iBalanceFactor     = MDB_MIN(x-2+MDB_MIN(y, 0), y-1);
        
            return pNode;
        }
        
        /*
        * 单向右旋例程
        *            X              Y
        *           / \            / \
        *          Y   C    ==>   A   X
        *         / \                / \
        *        A   B              B   C
        * 平衡因子变更分析
        * 旋转之前：
        * 假定 X 的平衡因子是 x, 节点 Y 的平衡因子是 y, 
        * 设 C 的高度为 h, 则 Y 的高度为 h-x
        * 节点 A 高度为 h-x-1-max(y,0); 
        * 节点 B 的高度为 h-x-1+MIN(y,0);
        * 旋转之后：
        * 节点 X 的新平衡因子是 x+1-MIN(y,0)
        * 节点 Y 的新平衡因子是 max(B,C)+1-A => max(B-A+1,C-A+1) 
        *     => max(y+1,x+2+max(y,0))
        */
        TMdbNtcAvlTree::TNode* TMdbNtcAvlTree::RightRotate(TMdbNtcAvlTree::TNode* pNode)
        {
            TNode * pTempNode;
            int x,y;
            
            //右旋处理
            pTempNode = pNode;
            pNode = pNode->pLeftSubNode;
            pNode->pParentNode = pTempNode->pParentNode;
        
            pTempNode->pLeftSubNode = pNode->pRightSubNode;
            if(pNode->pRightSubNode)
            {
                pNode->pRightSubNode->pParentNode = pTempNode;
            }   
        
            pNode->pRightSubNode = pTempNode;
            pTempNode->pParentNode = pNode;
            
            //获取旧平衡因子
            x = pTempNode->iBalanceFactor;
            y = pNode->iBalanceFactor;
        
            //设置新平衡因子
            pTempNode->iBalanceFactor = x+1-MDB_MIN(y, 0);
            pNode->iBalanceFactor     = MDB_MAX(x+2+MDB_MAX(y, 0), y+1);
        
            return pNode;
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcIntMap, TMdbNtcAvlTree);
        void TMdbNtcIntMap::TNode::SwapNode(TMdbNtcAvlTree::TNode* pSwapNode)
        {
            MDB_INT64 iKeyTemp = iKey;
            iKey = static_cast<TNode*>(pSwapNode)->iKey;
            static_cast<TNode*>(pSwapNode)->iKey = iKeyTemp;
            TMdbNtcBaseObject* pTempData = this->pData;
            this->pData = pSwapNode->pData;
            pSwapNode->pData = pTempData;
        }

        TMdbNtcIntMap::iterator TMdbNtcIntMap::Add(MDB_INT64 iKey, TMdbNtcBaseObject* pData)
        {
            TMdbNtcAvlTree::TNode* pRetNode = NULL;
            if(m_pRootNode == NULL)
            {
                ++m_uiSize;
                pRetNode = m_pRootNode = new TNode(iKey, pData);
            }
            else
            {
                pRetNode = new TNode(iKey, pData);
                InsertNode(&m_pRootNode, pRetNode);
            }
            if (m_iAvlFlag != 0)
            {
                m_iAvlHeight += m_iAvlFlag;
            }
            return TMdbNtcContainer::iterator(this, pRetNode);
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcStrMap, TMdbNtcAvlTree);
        void TMdbNtcStrMap::TNode::SwapNode(TMdbNtcAvlTree::TNode* pSwapNode)
        {
            TMdbNtcStringBuffer sKeyTemp = sKey;
            sKey = static_cast<TNode*>(pSwapNode)->sKey;
            static_cast<TNode*>(pSwapNode)->sKey = sKeyTemp;
            TMdbNtcBaseObject* pTempData = this->pData;
            this->pData = pSwapNode->pData;
            pSwapNode->pData = pTempData;
        }

        TMdbNtcStrMap::TMdbNtcStrMap(bool bCaseSensitive)
            :m_bCaseSensitive(bCaseSensitive)
        {
        }

        TMdbNtcStrMap::iterator TMdbNtcStrMap::Add(TMdbNtcStringBuffer sKey, TMdbNtcBaseObject* pData)
        {
            if(!m_bCaseSensitive) sKey.ToLower();
            TMdbNtcAvlTree::TNode* pRetNode = NULL;
            if(m_pRootNode == NULL)
            {
                ++m_uiSize;
                pRetNode = m_pRootNode = new TNode(sKey, pData);
            }
            else
            {                
                pRetNode = new TNode(sKey, pData);
                InsertNode(&m_pRootNode, pRetNode);
            }
            if (m_iAvlFlag != 0)
            {
                m_iAvlHeight += m_iAvlFlag;
            }
            return TMdbNtcContainer::iterator(this, pRetNode);
        }
        //////////////////////////////////////////////////////////////////////////
        //
        //哈希表HashFunc定义
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcHashList, TMdbNtcContainer);
        //构造函数
        TMdbNtcHashList::TMdbNtcHashList()
        {
            m_uiTableNum = 0;
            m_uiSize = 0;
            m_pHashList = NULL;
        }
        
        TMdbNtcHashList::~TMdbNtcHashList()
        {
            Clear();
            if(m_pHashList)
            {
                delete []m_pHashList;
                m_pHashList = NULL;
            }
        }

        void TMdbNtcHashList::Print(FILE* fp /* = NULL */) const
        {
            if(fp == NULL) 
            {
                fp = stdout;
            }
            iterator itor = IterBegin(), itEnd = IterEnd();
            for (;itor != itEnd; ++itor)
            {
                fprintf(fp, "%s\n", itor.pNodeObject->ToString().c_str());
            }
        }
        
        //初始化链表
        unsigned int TMdbNtcHashList::InitHashTable(unsigned int uiTableNum)
        {
            if (uiTableNum <= 0 || m_pHashList)
            {
                return m_uiTableNum;
            }
            if(uiTableNum <= 2)
            {
                m_uiTableNum = 2;
            }
            else
            {
                //取大于等于iTableNum的最小质数
                unsigned int i = uiTableNum%2==0?(uiTableNum+1):uiTableNum;
                do
                {
                    unsigned int s = (unsigned int)(sqrt(double(i)));//s为循环上限
                    s = (s/2)*2+1;
                    unsigned int j = 2;
                    for(; j <= s; ++j)           //用j检测i是否为质数
                    {
                        if(i%j==0)                         //   如果不是质数
                        {
                            break;                           //跳出for循环
                        }
                    }
                    if(j > s)
                    {
                        m_uiTableNum = i;
                        break;
                    }
                    else
                    {
                        ++i;
                    }
                }while(1);
            }
            m_pHashList = new TMdbNtcBaseList[m_uiTableNum];
            return m_uiTableNum;
        }
        
        unsigned int TMdbNtcHashList::GetContainerMemoryUsage() const
        {
            unsigned int uiSize = sizeof(TMdbNtcHashList);
            for (unsigned int i = 0; i < m_uiTableNum; ++i)
            {
                uiSize += m_pHashList[i].GetContainerMemoryUsage();
            }
            return uiSize;
        }
        
        //清空容器
        void TMdbNtcHashList::Clear()
        {            
            if(m_pHashList)
            {
                for (unsigned int i = 0; i < m_uiTableNum; ++i)
                {
                    m_pHashList[i].SetAutoRelease(m_bAutoRelease);
                    m_pHashList[i].Clear();
                    m_pHashList[i].SetAutoRelease(false);
                }
            }
            m_uiSize = 0;
        }
        
        TMdbNtcHashList::iterator TMdbNtcHashList::Add(TMdbNtcBaseObject* pData)
        {
            if (m_uiTableNum <= 0)
            {
                return IterEnd();
            }
            TMdbNtcBaseList* pList = FindList(pData->ToHash());
            if(pList)
            {
                TMdbNtcBaseList::TNode* pNewNode = pList->AddTail(pData);
                ++m_uiSize;
                return iterator(pList, this, pNewNode);
            }
            else
            {
                return IterEnd();
            }
        }
        
        int TMdbNtcHashList::Remove(const TMdbNtcBaseObject &oData, int iDelCount /* = 1 */, const TMdbNtcObjCompare &oCompare /* = g_oMdbNtcObjectCompare */)
        {
            TMdbNtcHashList::iterator itor;
            int iRetCount = 0;
            do
            {                
                itor = TMdbNtcHashList::IterFind(oData, oCompare);
                if(itor.pNodeObject == NULL)
                {
                    break;
                }
                ++iRetCount;
                IterErase(itor++);
            } while (iDelCount == -1 || iRetCount < iDelCount);
            return iRetCount;
        }
        
        //获取开始迭代器
        TMdbNtcHashList::iterator TMdbNtcHashList::IterBegin() const
        {
            if(m_uiSize == 0) return IterEnd();
            for (unsigned int i = 0; i < m_uiTableNum; ++i)
            {
                if (m_pHashList[i].GetSize() > 0 )
                {
                    iterator itor = m_pHashList[i].IterBegin();
                    itor.pParentContainer = this;
                    return itor;
                }
            }
            return iterator(this);
        }
        
        TMdbNtcHashList::iterator TMdbNtcHashList::IterLast() const
        {
            if(m_uiSize == 0) return IterEnd();
            for (int i = (int)m_uiTableNum-1; i >= 0 ; --i)
            {
                if (m_pHashList[i].GetSize() > 0 )
                {
                    iterator itor = m_pHashList[i].IterLast();
                    itor.pParentContainer = this;
                    return itor;
                }
            }
            return iterator(this);
        }
        
        //移除某个迭代器对应的节点，返回下一个迭代器,此处需要修改
        TMdbNtcHashList::iterator TMdbNtcHashList::IterErase(TMdbNtcHashList::iterator itor)
        {            
            if(itor.pNodeObject == NULL) return itor;
            TMdbNtcBaseList* pList = (TMdbNtcBaseList*)itor.pDataContainer;
            if(pList)
            {
                pList->SetAutoRelease(m_bAutoRelease);
                itor = pList->IterErase(itor);
                pList->SetAutoRelease(false);
                --m_uiSize;
                if(itor.pNodeObject == NULL)//需要考虑切换链表
                {
                    itor.pParentContainer = this;
                    ++itor;
                }        
                return itor;
            }
            else
            {
                return IterEnd();
            }
        }
        
        //通过迭代器交换，完成元素和节点的交换
        void TMdbNtcHashList::IterSwap(TMdbNtcHashList::iterator itor1, TMdbNtcHashList::iterator itor2)
        {
            if (itor1.pNodeObject&& itor2.pNodeObject)
            {
                TMdbNtcBaseObject * pTemp = itor1.pNodeObject->pData;
                itor1.pNodeObject->pData = itor2.pNodeObject->pData;
                itor2.pNodeObject->pData= pTemp;
            } 
        }
        
        //根据字符串键，查找数据的迭代器
        TMdbNtcHashList::iterator TMdbNtcHashList::IterFind(const TMdbNtcBaseObject &oData,
                                                const TMdbNtcObjCompare &oCompare, iterator itLastFound /*= ms_itorEnd*/) const
        {
            TMdbNtcBaseList::TNode* pCurNode = static_cast<TMdbNtcBaseList::TNode*>(itLastFound.pNodeObject);
            if(pCurNode == NULL)
            {
                TMdbNtcBaseList* pList = FindList(oData.ToHash());
                if(pList == NULL) return IterEnd();
                else pCurNode = pList->GetHead();
                itLastFound.pDataContainer = pList;
            }
            else pCurNode = pCurNode->pNext;
            while (pCurNode)
            {
                if(oCompare.Compare(pCurNode->pData, &oData) == 0)
                {
                    break;
                }
                pCurNode = pCurNode->pNext;
            }
            itLastFound.pNodeObject = pCurNode;
            itLastFound.pParentContainer = this;
            return itLastFound;
        }
        
        //获得前一个迭代器,完成子容器的切换
        TMdbNtcHashList::iterator TMdbNtcHashList::IterPrev(TMdbNtcHashList::iterator itCur, int iStep /* = -1 */) const
        {
            if (itCur.pNodeObject != NULL )
            {
                return itCur-(-iStep);//走链表遍历
            }
        
            //步长小于等于-1，则进行子容器的切换
            if(iStep <= -1)
            {
                itCur.iLastStep = 0;        
                if (m_uiTableNum == 0 || itCur.pDataContainer == NULL
                    || itCur.pDataContainer >= m_pHashList || m_pHashList+m_uiTableNum < itCur.pDataContainer)
                {
                    //传入实参非法，返回End迭代器
                    itCur.pNodeObject = NULL;
                    return itCur;
                }
                else
                {
                    //切换子容器
                    int iIndex = (int )(((TMdbNtcBaseList*)itCur.pDataContainer) - m_pHashList);
                    //切换子容器并偏移迭代器
                    for (--iIndex; iIndex >= 0; --iIndex)
                    {
                        if (m_pHashList[iIndex].GetSize() > 0)
                        {
                            //如果步长大于当前非空子容器的容器大小，则从步长中减去容器大小
                            //切换到下一个非空容器，重复上述动作，直到步长小于等于容器大小
                            if ((itCur.iLastStep-iStep) > (int)m_pHashList[iIndex].GetSize() )
                            {
                                itCur.iLastStep -= (int)(m_pHashList[iIndex].GetSize());
                            } 
                            else
                            {
                                iterator itor = m_pHashList[iIndex].IterLast()-(itCur.iLastStep-iStep-1);
                                itor.pParentContainer = this;
                                itor.iLastStep += itCur.iLastStep;
                                return itor;
                            }            
                        }
                    }
                    //超出父容器范围，返回End迭代器
                    itCur.pNodeObject = NULL;
                    return itCur;          
                }
            }
            //步长为0，返回当前迭代器
            else if(iStep == 0)
            {
                return itCur;
            }
            else
            {
                return IterNext(itCur, iStep);
            }
            
        }
        
        //获得后一个迭代器,完成子迭代器的切换
        TMdbNtcHashList::iterator TMdbNtcHashList::IterNext(TMdbNtcAvlTree::iterator itCur, int iStep /* = 1 */) const
        {
            if (itCur.pNodeObject != NULL )
            {
                return itCur+iStep;
            }
        
             //步长大于等于1，则进行子容器的切换
            if(iStep >= 1)
            {
                itCur.iLastStep = 0;
                if (m_uiTableNum == 0 || itCur.pDataContainer == NULL
                    || itCur.pDataContainer < m_pHashList || m_pHashList+m_uiTableNum <= itCur.pDataContainer)
                {
                    //传入实参非法，返回End迭代器
                    itCur.pNodeObject = NULL;
                    return itCur;
                }
                else
                {
                    //切换子容器
                    unsigned uiIndex = (unsigned int )(((TMdbNtcBaseList*)itCur.pDataContainer) - m_pHashList);
                    //切换子容器并偏移迭代器
                    for (++uiIndex; uiIndex < m_uiTableNum; ++uiIndex)
                    {
                        if (m_pHashList[uiIndex].GetSize() > 0)
                        {
                            //如果步长大于当前非空子容器的容器大小，则从步长中减去容器大小
                            //切换到下一个非空容器，重复上述动作，直到步长小于等于容器大小
                            if ((iStep-itCur.iLastStep) > (int)m_pHashList[uiIndex].GetSize() )
                            {
                                itCur.iLastStep += (int)(m_pHashList[uiIndex].GetSize());
                            } 
                            else
                            {
                                iterator itor = m_pHashList[uiIndex].IterBegin() + (iStep-itCur.iLastStep-1);
                                itor.pParentContainer = this;
                                itor.iLastStep += itCur.iLastStep;
                                return itor;
                            }           
                        }
                    }    
                    //超出父容器范围，返回End迭代器
                    itCur.pNodeObject = NULL;
                    return itCur;          
                }
            }
            else if(iStep == 0)
            {
                return itCur;
            }
            else
            {
                return IterPrev(itCur, iStep);
            }    
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcIntHash, TMdbNtcHashList);
        
        TMdbNtcIntHash::iterator TMdbNtcIntHash::Add(MDB_INT64 iKey, TMdbNtcBaseObject* pData)
        {
            if (m_uiTableNum <= 0)
            {
                return IterEnd();
            }
            TMdbNtcBaseList* pList = FindList((MDB_UINT64)iKey);
            if(pList)
            {
                TNode* pNewNode = new TNode(iKey, pData);
                pList->AddTail(pNewNode);
                ++m_uiSize;
                return TMdbNtcContainer::iterator(pList, this, pNewNode);
            }
            else
            {
                return IterEnd();
            }
        }

        int TMdbNtcIntHash::Remove(MDB_INT64 iKey, int iDelCount /* = 1 */)
        {
            int iRetDelCount = 0;
            TNode* pCurNode = NULL;
            TMdbNtcBaseList* pList = FindList((MDB_UINT64)iKey);
            if( pList )
            {
                pCurNode = static_cast<TNode*>(pList->GetHead());
                while (pCurNode)
                {
                    if(pCurNode->iKey == iKey)
                    {
                        ++iRetDelCount;
                        --m_uiSize;
                        MdbNtcReleaseData(pCurNode->pData);
                        pCurNode = static_cast<TNode*>(pList->Remove(pCurNode));
                        if(iDelCount != -1 && iRetDelCount >= iDelCount)
                        {
                            break;
                        }
                    }
                    else
                    {
                        pCurNode = static_cast<TNode*>(pCurNode->pNext);
                    }
                }
            }
            return iRetDelCount;
        }
        
        TMdbNtcIntHash::iterator TMdbNtcIntHash::IterFind(MDB_INT64 iKey, iterator itLastFound /* = ms_itorEnd */) const
        {
            TNode* pCurNode = static_cast<TNode*>(itLastFound.pNodeObject);
            if(pCurNode == NULL)
            {
                TMdbNtcBaseList* pList = FindList((MDB_UINT64)iKey);
                if(pList == NULL) return IterEnd();
                else pCurNode = static_cast<TNode*>(pList->GetHead());
                itLastFound.pDataContainer = pList;
            }
            else
            {
                pCurNode = static_cast<TNode*>(pCurNode->pNext);
            }
            while (pCurNode)
            {
                if(pCurNode->iKey == iKey)
                {
                    break;
                }
                pCurNode = static_cast<TNode*>(pCurNode->pNext);
            }
            itLastFound.pNodeObject = pCurNode;
            itLastFound.pParentContainer = this;
            return itLastFound;
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcStrHash, TMdbNtcHashList);
        TMdbNtcStrHash::TMdbNtcStrHash(bool bCaseSensitive /* = true */, mdb_ntc_hash_func pHashFunc /* = NULL */)
            :m_bCaseSensitive(bCaseSensitive)
        {
            if(NULL == pHashFunc)
            {
                m_pHashFunc = MdbNtcHashFunc;
            }
            else
            {
                m_pHashFunc = pHashFunc;
            }
        }
        
        TMdbNtcStrHash::iterator TMdbNtcStrHash::Add(TMdbNtcStringBuffer sKey, TMdbNtcBaseObject* pData)
        {
            if (m_uiTableNum <= 0)
            {
                return IterEnd();
            }
            if(!m_bCaseSensitive) sKey.ToLower();
            TMdbNtcBaseList* pList = FindList(m_pHashFunc(sKey.c_str()));
            if(pList)
            {
                TNode* pNewNode = new TNode(sKey, pData);
                pList->AddTail(pNewNode);
                ++m_uiSize;
                return TMdbNtcContainer::iterator(pList, this, pNewNode);
            }
            else
            {
                return IterEnd();
            }
        }

        int TMdbNtcStrHash::Remove(TMdbNtcStringBuffer sKey, int iDelCount /* = 1 */)
        {
            if(!m_bCaseSensitive) sKey.ToLower();
            int iRetDelCount = 0;
            TNode* pCurNode = NULL;
            TMdbNtcBaseList* pList = FindList(m_pHashFunc(sKey.c_str()));
            if( pList )
            {
                pCurNode = static_cast<TNode*>(pList->GetHead());
                while (pCurNode)
                {
                    if(pCurNode->sKey == sKey)
                    {
                        ++iRetDelCount;
                        --m_uiSize;
                        MdbNtcReleaseData(pCurNode->pData);
                        pCurNode = static_cast<TNode*>(pList->Remove(pCurNode));
                        if(iDelCount != -1 && iRetDelCount >= iDelCount)
                        {
                            break;
                        }
                    }
                    else
                    {
                        pCurNode = static_cast<TNode*>(pCurNode->pNext);
                    }
                }
            }
            return iRetDelCount;
        }

        TMdbNtcStrHash::iterator TMdbNtcStrHash::IterFind(TMdbNtcStringBuffer sKey, iterator itLastFound /* = ms_itorEnd */) const
        {
            if(!m_bCaseSensitive) sKey.ToLower();
            TNode* pCurNode = static_cast<TNode*>(itLastFound.pNodeObject);
            if(pCurNode == NULL)
            {
                TMdbNtcBaseList* pList = FindList(m_pHashFunc(sKey.c_str()));
                if(pList == NULL) return IterEnd();
                else pCurNode = static_cast<TNode*>(pList->GetHead());
                itLastFound.pDataContainer = pList;
            }
            else
            {
                pCurNode = static_cast<TNode*>(pCurNode->pNext);
            }
            while (pCurNode)
            {
                if(pCurNode->sKey.Compare(sKey) == 0)
                {
                    break;
                }
                pCurNode = static_cast<TNode*>(pCurNode->pNext);
            }
            itLastFound.pNodeObject = pCurNode;
            itLastFound.pParentContainer = this;
            return itLastFound;
        }

        //////////////////////////////////////////////////////////////////////////
        //
        //键树TKeyTree定义
        //
        //////////////////////////////////////////////////////////////////////////
        //始终存在一个根节点，树为空的时候，根节点的ppSubNode为空，所有叶节点的该
        //指针也为空，并据此判断是否为叶节点，删除叶节点的时候，如果父节点变为叶节
        //点，则释放父节点的ppSubNode指针指向的内存空间，并赋值为NULL以保持一致性。
        
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcKeyTree, TMdbNtcContainer);
        TMdbNtcKeyTree::TNode* TMdbNtcKeyTree::m_s_pNullNode = NULL;        
        //键树节点类构造函数
        TMdbNtcKeyTree::TNode::TNode(TNode*& pParentNodeRef, TMdbNtcBaseObject* pData /* = NULL */)
        :pParentNode(pParentNodeRef)
        {
            ppSubNode = NULL;
            this->pData = pData;
        }
        
        int TMdbNtcKeyTree::FindIndex(TNode* pCurNode) const
        {
            if(pCurNode && pCurNode->pParentNode)
            {
                TNode** ppSubNode = pCurNode->pParentNode->ppSubNode;
                for (MDB_UINT32 i = 0; i < m_uiTNodeCount; ++i)
                {
                    if(ppSubNode[i] == pCurNode)
                    {
                        return (int)i;
                    }
                }
            }
            return -1;
        }
        
        int TMdbNtcKeyTree::ReverseFindIndex(TNode* pCurNode) const
        {
            if(pCurNode && pCurNode->pParentNode)
            {
                TNode** ppSubNode = pCurNode->pParentNode->ppSubNode;
                for (int i = (int)m_uiTNodeCount-1; i >= 0; --i)
                {
                    if(ppSubNode[i] == pCurNode)
                    {
                        return (int)i;
                    }
                }
            }
            return -1;
        }
        
        int TMdbNtcKeyTree::FindSiblingPrevIndex(TNode** ppCurNode) const
        {
            if(ppCurNode && *ppCurNode)
            {
                TNode* pParentNode = (*ppCurNode)->pParentNode;
                if(pParentNode)
                {
                    int iIndex = (int)(ppCurNode-pParentNode->ppSubNode);
                    for (--iIndex; iIndex >= 0; --iIndex)
                    {
                        if(pParentNode->ppSubNode[iIndex])//左边还有非空兄弟节点
                        {
                            return iIndex;
                        }
                    }
                }        
            }
            return -1;
        }
        
        int TMdbNtcKeyTree::FindSiblingNextIndex(TNode** ppCurNode) const
        {
            if(ppCurNode && *ppCurNode)
            {
                TNode* pParentNode = (*ppCurNode)->pParentNode;
                if(pParentNode)
                {
					MDB_UINT32 iIndex = (MDB_UINT32)(ppCurNode-pParentNode->ppSubNode);
                    for (++iIndex; iIndex < m_uiTNodeCount; ++iIndex)
                    {
                        if(pParentNode->ppSubNode[iIndex])//右边还有非空兄弟节点
                        {
                            return (int)iIndex;
                        }
                    }
                }        
            }
            return -1;
        }
        
        TMdbNtcKeyTree::TNode* TMdbNtcKeyTree::FindSiblingOrParentNext(TNode** ppCurNode) const
        {
            if(ppCurNode == NULL) return NULL;    
            while(*ppCurNode)
            {
                int iIndex = FindSiblingNextIndex(ppCurNode);
                //右边没有非空节点，则父节点继续向上回溯
                if(iIndex == -1)
                {
                    ppCurNode = &(*ppCurNode)->pParentNode;
                }
                else
                {
                    return (*ppCurNode)->pParentNode->ppSubNode[iIndex];
                }
            }
            return NULL;
        }
        
        //键树构造函数
        TMdbNtcKeyTree::TMdbNtcKeyTree(const int iCharacterIndex[256])
        {
            int iMax = 0;
            for (int i = 0; i < 256; ++i)
            {
                m_iCharacterIndex[i] = iCharacterIndex[i];
                if (iCharacterIndex[i] > iMax)
                {
                    iMax = iCharacterIndex[i];
                }
            }
            
            //最大孩子数
            m_uiTNodeCount = (MDB_UINT32)iMax + 1;
            //创建根节点
            m_pRootNode = new TNode(m_s_pNullNode);
        
            m_uiSize = 0;
        }
        
        TMdbNtcKeyTree::~TMdbNtcKeyTree()
        {
            Clear();
            if(m_pRootNode)
            {
                delete m_pRootNode;
                m_pRootNode = NULL;
            }
        }
        
        MDB_UINT32 TMdbNtcKeyTree::GetChildNodeCount(TNode* pParentNode /*= NULL*/) const
        {
            unsigned uiCount = 0;
            if(pParentNode == NULL)
            {
                pParentNode = m_pRootNode;
                if(pParentNode) ++uiCount;
                else return uiCount;
            }
            if(pParentNode->ppSubNode)
            {
                TNode* pChildNode = NULL;
                for (MDB_UINT32 i = 0; i < m_uiTNodeCount; ++i)
                {
                    pChildNode = pParentNode->ppSubNode[i];
                    if(pChildNode)
                    {
                        ++uiCount;
                        if(pChildNode->ppSubNode)
                        {
                            uiCount += GetChildNodeCount(pChildNode);
                        }
                    }
                }
            }
            return uiCount;
        }
        
        MDB_UINT32 TMdbNtcKeyTree::GetChildNodeMemoryUsage(TNode* pParentNode /*= NULL*/) const
        {
            unsigned uiSize = 0;
            if(pParentNode == NULL)
            {
                pParentNode = m_pRootNode;
                if(pParentNode) uiSize += (MDB_UINT32)sizeof(TNode);
                else return uiSize;
            }
            if(pParentNode->ppSubNode)
            {
                uiSize += (MDB_UINT32)sizeof(TNode*)*m_uiTNodeCount;//申请的子节点指针数据占用内存的大小
                TNode* pChildNode = NULL;
                for (MDB_UINT32 i = 0; i < m_uiTNodeCount; ++i)
                {
                    pChildNode = pParentNode->ppSubNode[i];
                    if(pChildNode)
                    {
                        uiSize += (MDB_UINT32)sizeof(TNode);
                        if(pChildNode->ppSubNode)
                        {
                            uiSize += GetChildNodeMemoryUsage(pChildNode);
                        }
                    }
                }
            }
            return uiSize;
        }
        
        MDB_UINT32 TMdbNtcKeyTree::GetContainerMemoryUsage() const
        {    
            return (MDB_UINT32)(sizeof(TMdbNtcKeyTree)+GetChildNodeMemoryUsage(NULL));
        }
        
        //清空节点
        void TMdbNtcKeyTree::Clear()
        {
            if(m_pRootNode)
            {
                DeleteKeyTree(m_pRootNode);
                m_pRootNode = new TNode(m_s_pNullNode);
                m_uiSize = 0;
            }
        }
        
        //后序遍历清除键树
        void TMdbNtcKeyTree::DeleteKeyTree(TMdbNtcKeyTree::TNode * &pParentNode)
        {
            if (pParentNode == NULL )
            {
                return;
            }
            if(pParentNode->ppSubNode != NULL )
            {
                for (unsigned int i = 0; i < m_uiTNodeCount; ++i)
                {
                    //非叶节点
                    if (pParentNode->ppSubNode[i] != NULL )
                    {
                        DeleteKeyTree(pParentNode->ppSubNode[i]);
                    }
                }
            }
            MdbNtcReleaseData(pParentNode->pData);
            delete pParentNode;
            pParentNode = NULL;
        }
        
        //增加一个键值,如果已经存在键值，则将之前的数据删除,再用新的替代
        TMdbNtcContainer::iterator TMdbNtcKeyTree::Add(const char* pszKeyName, TMdbNtcBaseObject* pData)
        {
            if(pszKeyName == NULL)
            {
                return IterEnd();
            }
            TNode ** ppCurNode = &m_pRootNode, *pCurNode = *ppCurNode;  //当前树节点
            const char *p    = pszKeyName;   //字符指针
            int   ikey       = -1;           //键对应的位置    
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //过滤掉非法字符
                if (ikey != -1)
                {
                    //如果是叶节点，则初始化ppSubNode
                    if (pCurNode->ppSubNode == NULL)
                    {
                        //初始化ppSubNode
                        pCurNode->ppSubNode = new TNode*[m_uiTNodeCount];
                        memset(pCurNode->ppSubNode, 0x00, m_uiTNodeCount*sizeof(TNode*));
                        //创建键节点
                        pCurNode->ppSubNode[ikey] = new TNode(*ppCurNode);
                    } 
                    else
                    {
                        //如果键节点不存在，则创建
                        if (pCurNode->ppSubNode[ikey] == NULL )
                        {
                            pCurNode->ppSubNode[ikey] = new TNode(*ppCurNode);
                        }
                    }
                    ppCurNode = &pCurNode->ppSubNode[ikey];
                    pCurNode = *ppCurNode;
                }
                ++p;
            }
            if (pCurNode->pData == NULL)
            {
                ++m_uiSize;
            }
            else
            {
                if(m_bAutoRelease)
                {
                    delete pCurNode->pData;
                    pCurNode->pData = NULL;
                }
            }
            pCurNode->pData = pData;
            return iterator(this, pCurNode);
        }
        
        //根据字符串键，删除数据
        int TMdbNtcKeyTree::Remove(const char* pszKeyName)
        {
            
            if(pszKeyName == NULL) return 0;
            int iRetDelCount = 0;
            TNode** ppCurNode = FindNode(pszKeyName);
            if(ppCurNode)
            {
                iRetDelCount = 1;
                Remove(*ppCurNode);
            }
            return iRetDelCount;
        }
        
        TMdbNtcKeyTree::TNode** TMdbNtcKeyTree::FindNode(const char* pszKeyName) const
        {
            if(pszKeyName == NULL)
            {
                return NULL;
            }
            TNode** ppCurNode = (TNode**)&m_pRootNode, *pCurNode = *ppCurNode;
            const char  *p   = pszKeyName;   //字符指针
            int   ikey       = -1;           //键对应的位置
            //根据键字符串定位节点
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //过滤掉非法字符
                if (ikey != -1)
                {
                    //如果当前节点指针为空，或者当前节点指针为叶节点，则跳出循环
                    if (pCurNode == NULL || pCurNode->ppSubNode == NULL )
                    {
                        break;
                    } 
                    else
                    {
                        ppCurNode = &pCurNode->ppSubNode[ikey];
                        pCurNode = *ppCurNode;
                    }            
                }
                ++p;
            }
            if(pCurNode && pCurNode->pData && *p == '\0')//存在数据，且key相匹配
            {
                return ppCurNode;
            }
            else
            {
                return NULL;
            }
        }
        
        void TMdbNtcKeyTree::Remove(TMdbNtcKeyTree::TNode* pCurNode)
        {
            if(pCurNode == m_pRootNode)
            {
                Remove(&m_pRootNode);
            }
            else
            {
                int iIndex = FindIndex(pCurNode);
                if(iIndex != -1)
                {
                    Remove(pCurNode->pParentNode->ppSubNode+iIndex);
                }
            }
        }
        
        void TMdbNtcKeyTree::Remove(TMdbNtcKeyTree::TNode** ppCurNode)
        {    
            TMdbNtcKeyTree::TNode* pCurNode = *ppCurNode;
            if(pCurNode->pData)
            {
                if(m_bAutoRelease)
                {
                    delete pCurNode->pData;
                    pCurNode->pData = NULL;
                }
                else
                {
                    pCurNode->pData = NULL;
                }
                --m_uiSize;
            }
            //没有子节点的情况下，可以删除此节点
            if(pCurNode != m_pRootNode && pCurNode->ppSubNode == NULL)
            {
                TNode** ppParentNode = &pCurNode->pParentNode, *pParentNode = *ppParentNode;
                int i = (int)(ppCurNode-pParentNode->ppSubNode);
                if(i != -1) pParentNode->ppSubNode[i] = NULL;
                delete pCurNode;
                pCurNode = NULL;
                //检查父级下的子节点是否都为NULL
                while (pParentNode)
                {
                    MDB_UINT32 i = 0;
                    for (i=0; i < m_uiTNodeCount; ++i)
                    {
                        if(pParentNode->ppSubNode[i])
                        {
                            break;
                        }
                    }
                    if(i < m_uiTNodeCount)//有非空节点跳出循环
                    {
                        break;
                    }
                    else//没有非空子节点，则需要释放ppSubNode                
                    {                    
                        delete pParentNode->ppSubNode;
                        pParentNode->ppSubNode = NULL;
                        //当前父节点为根节点，或者含有数据，则无需继续回溯
                        if(pParentNode ==  m_pRootNode || pParentNode->pData)
                        {
                            break;
                        }
                        else
                        {
                            //得到当前节点在上级节点的子序号
                            i = (MDB_UINT32)(ppParentNode-pParentNode->pParentNode->ppSubNode);
                            if(i != (MDB_UINT32)-1) pParentNode->pParentNode->ppSubNode[i] = NULL;
                            ppParentNode = &pParentNode->pParentNode;
                            delete pParentNode;      
                            pParentNode = *ppParentNode;                        
                        }
                    }
                }
            }
        }
        
        //根据字符串键，查找数据
        TMdbNtcBaseObject* TMdbNtcKeyTree::Find(const char* pszKeyName)
        {
            iterator itor = IterFind(pszKeyName);
            if(itor.pNodeObject) return itor.pNodeObject->pData;
            else return NULL;
        }
        
        //根据前缀获取匹配的元素
        void TMdbNtcKeyTree::MatchPrefix(const char* pszPrefix, TMdbNtcBaseList& oDataList)
        {
            std::pair<TMdbNtcKeyTree::iterator, TMdbNtcKeyTree::iterator> RetPair = MatchPrefix(pszPrefix);
            for(iterator itor = RetPair.first; itor != RetPair.second; ++itor)
            {
                oDataList.AddTail(itor.pNodeObject->pData);
            }
        }
        
        //移除某个迭代器对应的节点，返回下一个迭代器
        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::IterErase(TMdbNtcKeyTree::iterator itor)
        {            
            if (m_pRootNode->ppSubNode == NULL || itor.pNodeObject == NULL)
            {
                return IterEnd();
            }
            
            TNode* pCurNode = (TNode *)itor.pNodeObject;
            ++itor;
            Remove(pCurNode);
            return itor;
        }
        
        //获取开始迭代器
        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::IterBegin() const
        {
            if(m_uiSize == 0) return IterEnd();
            else if(m_pRootNode->pData) return iterator(this, m_pRootNode);
            TNode * pCurNode = m_pRootNode;
            while (pCurNode != NULL && pCurNode->ppSubNode != NULL)
            {
                unsigned int i = 0;
                for (i = 0; i < m_uiTNodeCount; ++i)
                {
                    if(pCurNode->pData)
                    {
                        break;
                    }
                    else if(pCurNode->ppSubNode[i] != NULL)
                    {
                        pCurNode = pCurNode->ppSubNode[i];
                        break;
                    }
                }
                if(pCurNode->pData || i == m_uiTNodeCount)
                {
                    break;
                }
            }
        
            iterator itRet(this, pCurNode);
            if (pCurNode->pData == NULL)
            {
                ++itRet;
            }
            return itRet;
        }
        
        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::IterLast() const
        {
            if(m_uiSize == 0) return IterEnd();
            TNode * pCurNode = m_pRootNode;
            while (pCurNode != NULL && pCurNode->ppSubNode != NULL)
            {
                int i = 0;
                for (i = (int)m_uiTNodeCount-1; i >= 0; --i)
                {
                    if (pCurNode->ppSubNode[i] != NULL)
                    {
                        pCurNode = pCurNode->ppSubNode[i];
                        break;
                    }
                }
                if(i < 0)
                {
                    break;
                }
            }
            
            iterator itRet(this, pCurNode);
            
            if (pCurNode->pData == NULL)
            {
                --itRet;
            }
            return itRet;
        }
        
        //根据字符串键，查找数据的迭代器
        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::IterFind(const char* pszKeyName) const
        {
            if(pszKeyName == NULL) return IterEnd();
            TNode** ppCurNode = FindNode(pszKeyName);
            if(ppCurNode)
            {
                return iterator(this, *ppCurNode);
            }
            else
            {
                return iterator(this);
            }
        }
        
        //通过迭代器交换，完成元素和节点的交换
        void TMdbNtcKeyTree::IterSwap(TMdbNtcKeyTree::iterator itor1, TMdbNtcKeyTree::iterator itor2)
        {
            if (itor1.pNodeObject&& itor2.pNodeObject)
            {
                TMdbNtcBaseObject * pTemp = itor1.pNodeObject->pData;
                itor1.pNodeObject->pData = itor2.pNodeObject->pData;
                itor2.pNodeObject->pData= pTemp;
            }
        }
        
        //获得前一个迭代器
        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::IterPrev(TMdbNtcKeyTree::iterator itCur, int iStep /* = -1 */) const
        {
            if (m_pRootNode == NULL)
            {
                return IterEnd();
            }
        
            if (iStep <= -1 )
            {
                iterator itRet = itCur;
                itRet.iLastStep = 0;        
                TNode* pCurNode = (TNode *)itCur.pNodeObject;
                bool bSearchChild = false;
                //寻找前驱节点，向兄弟节点查找，如果没有非空，则向父节点回溯
                while (pCurNode)
                {
                    if(bSearchChild)
                    {
                        if(pCurNode->ppSubNode)
                        {
                            int i = 0;
                            for (i = (int)m_uiTNodeCount-1; i >= 0; --i)
                            {
                                if(pCurNode->ppSubNode[i])
                                {
                                    pCurNode = pCurNode->ppSubNode[i];//current转移到子级
                                    break;
                                }
                            }
                            //找到非空子节点
                            if(i >= 0)
                            {
                                continue;
                            }
                        }
                    }
                    if(itCur.pNodeObject != pCurNode && pCurNode->pData)
                    {
                        itRet.pNodeObject = pCurNode;
                        --itRet.iLastStep;
                        if(itRet.iLastStep == iStep)
                        {
                            break;
                        }
                    }
                    if(pCurNode->pParentNode)
                    {
                        //查找当前节点在父节点中的位置
                        int i = ReverseFindIndex(pCurNode);
                        //判断是否找到当前节点在兄弟节点的位置
                        if(i >= 0)
                        {
                            i = this->FindSiblingPrevIndex(&pCurNode->pParentNode->ppSubNode[i]);
                            if(i < 0)
                            {
                                pCurNode = pCurNode->pParentNode;
                                bSearchChild = false;
                            }
                            else
                            {
                                pCurNode = pCurNode->pParentNode->ppSubNode[i];
                                bSearchChild = true;
                            }
                        }
                        else//如果没有找到位置，这是不可能的，说明有问题
                        {
                            break;//跳出循环，结束查找
                        }
                    }
                    else
                    {                
                        break;//跳出循环，结束查找
                    }
                }
                if(itRet.iLastStep > iStep)
                {
                    itRet.pNodeObject = NULL;
                    --itRet.iLastStep;
                }
                return itRet;
            } 
            else if (iStep == 0)
            {
                return itCur;
            }
            else
            {
                return IterNext(itCur, iStep);
            }
        }
        
        //获得后一个迭代器
        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::IterNext(TMdbNtcKeyTree::iterator itCur, int iStep /* = 1 */) const
        {
            if (m_pRootNode == NULL)
            {
                return IterEnd();
            }
        
            if (iStep >= 1 )
            {
                iterator itRet = itCur;
                itRet.iLastStep = 0;        
                TNode* pCurNode = (TNode *)itCur.pNodeObject;
                //寻找右继节点，如果存在最左子节点则即为右继节点，否则向右兄弟节点或者向父级查找
                while (pCurNode)
                {
                    if(pCurNode->pData && pCurNode != itCur.pNodeObject)
                    {
                        itRet.pNodeObject = pCurNode;
                        ++itRet.iLastStep;
                        if(itRet.iLastStep == iStep)
                        {
                            break;
                        }
                    }
                    if(pCurNode->ppSubNode)
                    {
                        unsigned int i = 0;
                        for (; i < m_uiTNodeCount; ++i)
                        {
                            if(pCurNode->ppSubNode[i])
                            {
                                pCurNode = pCurNode->ppSubNode[i];//current转移到子级
                                break;
                            }
                        }
                        //找到非空子节点
                        if(i < m_uiTNodeCount)
                        {
                            continue;
                        }
                    }
                    if(pCurNode->pParentNode)
                    {
                        TNode** ppParentNode = &pCurNode->pParentNode, *pParentNode = *ppParentNode;
                        //查找当前节点在父节点中的位置
                        int iIndex = FindIndex(pCurNode);
                        if(iIndex == -1)//如果没有找到位置，这是不可能的，说明有问题
                        {
                            break;//跳出循环，结束查找
                        }
                        //判断是否找到当前节点在兄弟节点的位置
                        else
                        {
                            pCurNode = FindSiblingOrParentNext(&pParentNode->ppSubNode[iIndex]);
                            if(pCurNode == NULL)
                            {
                                break;//跳出循环，结束查找
                            }
                        }
                    }
                    else
                    {                
                        break;//跳出循环，结束查找
                    }
                }
                if(itRet.iLastStep < iStep)
                {
                    itRet.pNodeObject = NULL;
                    ++itRet.iLastStep;
                }
                return itRet;
            } 
            else if (iStep == 0)
            {
                return itCur;
            }
            else
            {
                return IterNext(itCur, iStep);
            }    
        }
        
        //根据字符串键，返回一个迭代器，指向不小于键值的第一个迭代器
        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::LowerBound(const char* pszKeyName) const
        {
            if(pszKeyName == NULL) return IterEnd();
            TNode *pCurNode = m_pRootNode;   //当前节点
            const char  *p   = pszKeyName;   //字符指针
            int   ikey       = -1;           //键对应的位置
            //根据键字符串定位节点
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //过滤掉非法字符
                if (ikey != -1)
                {
                    //如果当前节点指针为空，或者当前节点指针为叶节点，则跳出循环
                    if (pCurNode == NULL || pCurNode->ppSubNode == NULL )
                    {
                        break;
                    }
                    else
                    {
                        pCurNode = pCurNode->ppSubNode[ikey];
                    }
                }
                ++p;
            }
            if(pCurNode)
            {
                if(pCurNode->pData && *p == '\0')//存在数据，且key相匹配
                {
                    return iterator(this, pCurNode);
                }
                else//没有数据
                {
                    //使用域作用符修饰++,以避免在多重键树中调用该函数时使用多重键树的++操作，导致得到重复的值
                    return TMdbNtcKeyTree::IterNext(iterator(this, pCurNode));
                }
            }
            else
            {
                return iterator(this);
            }
        }
        
        //根据字符串键，返回一个迭代器，指向大于键值的第一个迭代器
        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::UpperBound(const char* pszKeyName) const
        {
            if(pszKeyName == NULL) return IterEnd();
            TNode *pCurNode = m_pRootNode;  //当前节点
            const char  *p   = pszKeyName;   //字符指针
            int   ikey       = -1;           //键对应的位置
            //根据键字符串定位节点
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //过滤掉非法字符
                if (ikey != -1)
                {
                    //如果当前节点指针为空，或者当前节点指针为叶节点，则跳出循环
                    if (pCurNode == NULL || pCurNode->ppSubNode == NULL )
                    {
                        break;
                    } 
                    else
                    {
                        pCurNode = pCurNode->ppSubNode[ikey];
                    }            
                }
                ++p;
            }
            if(pCurNode)
            {
                //使用域作用符修饰++,以避免在多重键树中调用该函数时使用多重键树的++操作，导致得到重复的值
                return TMdbNtcKeyTree::IterNext(iterator(this, pCurNode));
            }
            else
            {
                return iterator(this);
            }
        }
        
        //根据前缀获取匹配的元素
        std::pair<TMdbNtcKeyTree::iterator, TMdbNtcKeyTree::iterator> TMdbNtcKeyTree::MatchPrefix(const char* pszPrefix) const
        {
            std::pair<iterator, iterator> retPair(IterEnd(), IterEnd());
            if(pszPrefix == NULL) return retPair;
            TNode ** ppCurNode = (TNode**)&m_pRootNode;  //当前节点
            const char  *p   = pszPrefix;   //字符指针
            int   ikey       = -1;           //键对应的位置
            //根据键字符串定位节点
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //过滤掉非法字符
                if (ikey != -1)
                {
                    //如果当前节点指针为空，或者当前节点指针为叶节点，则跳出循环
                    if (*ppCurNode == NULL || (*ppCurNode)->ppSubNode == NULL )
                    {
                        return retPair;
                    } 
                    else
                    {
                        ppCurNode = &(*ppCurNode)->ppSubNode[ikey];
                    }
                }
                ++p;
            }
            if(*ppCurNode)
            {
                if((*ppCurNode)->pData)
                {
                    retPair.first = iterator(this, *ppCurNode);
                }
                else//没有数据，则通过++查找大于他的有数据节点
                {
                    retPair.first = ++iterator(this, *ppCurNode);            
                }
                //查找结束位置
                //查找右边非空兄弟节点或者向上回溯的父节点
                TNode* pEndNode = FindSiblingOrParentNext(ppCurNode);
                if(pEndNode)
                {
                    if(pEndNode->pData)
                    {
                        retPair.second = iterator(this, pEndNode);
                    }
                    else//没有数据，则通过++查找大于他的有数据节点
                    {
                        retPair.second = ++iterator(this, pEndNode);            
                    }
                }
            }
            return retPair;
        }

        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::MatchDeep(const char* pszKeyName) const
        {
            iterator itor(this);
            if(pszKeyName == NULL) return itor;
            TNode * pCurNode = m_pRootNode;  //当前节点
            const char  *p   = pszKeyName;   //字符指针
            int   ikey       = -1;           //键对应的位置
            //根据键字符串定位节点
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //过滤掉非法字符
                if (ikey != -1)
                {
                    //如果当前节点指针为空，或者当前节点指针为叶节点，则跳出循环
                    if (pCurNode == NULL || pCurNode->ppSubNode == NULL )
                    {
                        break;
                    } 
                    else
                    {
                        pCurNode = pCurNode->ppSubNode[ikey];
                        if(pCurNode && pCurNode->pData)
                        {
                            itor.pNodeObject = pCurNode;
                        }
                    }
                }
                ++p;
            }
            return itor;
        }
        
        //////////////////////////////////////////////////////////////////////////
        //
        //多重键树TMultiKeyTree定义
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcMultiKeyTree, TMdbNtcKeyTree);
        //多重键树构造函数
        TMdbNtcMultiKeyTree::TMdbNtcMultiKeyTree(const int iCharacterIndex[256]):TMdbNtcKeyTree(iCharacterIndex)
        {
            if(m_pRootNode)
            {
                delete m_pRootNode;
                m_pRootNode = new TNode(m_s_pNullNode);
            }
        }
        
        TMdbNtcMultiKeyTree::~TMdbNtcMultiKeyTree()
        {
            Clear();
            if(m_pRootNode)
            {
                delete m_pRootNode;
                m_pRootNode = NULL;
            }
        }
        
        //清空节点
        void TMdbNtcMultiKeyTree::Clear()
        {
            if(m_pRootNode)
            {
                DeleteKeyTree((TNode*&)m_pRootNode);
                m_pRootNode = new TNode(m_s_pNullNode);
                m_uiSize = 0;
            }
        }
        
        MDB_UINT32 TMdbNtcMultiKeyTree::GetChildNodeCount(TMdbNtcKeyTree::TNode* pParentNode /*= NULL*/) const
        {
            unsigned uiCount = 0;
            if(pParentNode == NULL)
            {
                pParentNode = m_pRootNode;
                if(pParentNode) ++uiCount;
                else return uiCount;
            }
            if(pParentNode->ppSubNode)
            {
                TNode* pChildNode = NULL;
                for (MDB_UINT32 i = 0; i < m_uiTNodeCount; ++i)
                {
                    pChildNode = (TNode*)pParentNode->ppSubNode[i];
                    if(pChildNode)
                    {
                        if(pChildNode->ppSubNode)
                        {
                            uiCount += GetChildNodeCount(pChildNode);
                        }
                        do 
                        {
                            ++uiCount;
                            pChildNode = pChildNode->pNext;
                        } while (pChildNode);                
                    }
                }
            }
            return uiCount;
        }
        
        MDB_UINT32 TMdbNtcMultiKeyTree::GetChildNodeMemoryUsage(TMdbNtcKeyTree::TNode* pParentNode /*= NULL*/) const
        {
            unsigned uiSize = 0;
            if(pParentNode == NULL)
            {
                pParentNode = m_pRootNode;
                if(pParentNode) uiSize += (MDB_UINT32)sizeof(TMdbNtcKeyTree::TNode);//根节点是分配为TKeyTree::TNode
                else return uiSize;
            }
            if(pParentNode->ppSubNode)
            {
                uiSize += (MDB_UINT32)sizeof(TNode*)*m_uiTNodeCount;//申请的子节点指针数据占用内存的大小
                TNode* pChildNode = NULL;
                for (MDB_UINT32 i = 0; i < m_uiTNodeCount; ++i)
                {
                    pChildNode = (TNode*)pParentNode->ppSubNode[i];
                    if(pChildNode)
                    {
                        if(pChildNode->ppSubNode)
                        {
                            uiSize += GetChildNodeMemoryUsage(pChildNode);
                        }
                        do 
                        {
                            uiSize += (MDB_UINT32)sizeof(TNode);
                            pChildNode = pChildNode->pNext;
                        } while (pChildNode);
                    }
                }
            }
            return uiSize;
        }
        
        MDB_UINT32 TMdbNtcMultiKeyTree::GetContainerMemoryUsage() const
        {    
            return (MDB_UINT32)(sizeof(TMdbNtcMultiKeyTree)+GetChildNodeMemoryUsage(NULL));
        }
        
        //后序遍历清除键树
        void TMdbNtcMultiKeyTree::DeleteKeyTree(TNode * &pParentNode)
        {
            if (pParentNode == NULL )
            {
                return;
            }
            if(pParentNode->ppSubNode != NULL )
            {
                for (unsigned int i = 0; i < m_uiTNodeCount; ++i)
                {
                    //非叶节点
                    if (pParentNode->ppSubNode[i] != NULL )
                    {
                        DeleteKeyTree((TNode*&)(pParentNode->ppSubNode[i]));
                    }
                }
            }

            TNode * pTempNode = pParentNode;
            TNode * pCurNode = NULL;
            do 
            {
                pCurNode = pTempNode;
                pTempNode = pCurNode->pNext;
                MdbNtcReleaseData(pCurNode->pData);
                delete pCurNode;
                pCurNode = NULL;
            } while (pTempNode);            
            pParentNode = NULL;
        }
        
        //添加键值
        TMdbNtcContainer::iterator TMdbNtcMultiKeyTree::Add(const char* pszKeyName, TMdbNtcBaseObject* pData)
        {
            if(pszKeyName == NULL)
            {
                return IterEnd();
            }
            TMdbNtcKeyTree::TNode ** ppCurNode = &m_pRootNode, *pCurNode = *ppCurNode;  //当前树节点
            const char *p    = pszKeyName;   //字符指针
            int   ikey       = -1;           //键对应的位置    
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //过滤掉非法字符
                if (ikey != -1)
                {
                    //如果是叶节点，则初始化ppSubNode
                    if (pCurNode->ppSubNode == NULL)
                    {
                        //初始化ppSubNode
                        pCurNode->ppSubNode = new TMdbNtcKeyTree::TNode*[m_uiTNodeCount];
                        memset(pCurNode->ppSubNode, 0x00, m_uiTNodeCount*sizeof(TMdbNtcKeyTree::TNode*));
                        TNode* pTempNextNode = ((TNode*)pCurNode)->pNext;
                        while(pTempNextNode)
                        {
                            //需要对这些重复键值节点的ppSubNode都指向head的ppSubNode
                            pTempNextNode->ppSubNode = pCurNode->ppSubNode;
                            pTempNextNode = pTempNextNode->pNext;
                        }              
                        //创建键节点
                        pCurNode->ppSubNode[ikey] = new TNode(*ppCurNode);
                    } 
                    else
                    {
                        //如果键节点不存在，则创建
                        if (pCurNode->ppSubNode[ikey] == NULL )
                        {
                            pCurNode->ppSubNode[ikey] = new TNode(*ppCurNode);
                        }
                    }
                    ppCurNode = &pCurNode->ppSubNode[ikey];
                    pCurNode = *ppCurNode;
                }
                ++p;
            }
            ++m_uiSize;
            if (pCurNode->pData == NULL)
            {
                pCurNode->pData = pData;
                return iterator(this, pCurNode);
            }
            else
            {
                TNode * pPrevNode = (TNode*)pCurNode;
                TNode * pNewNode  = new TNode(pCurNode->pParentNode, pData);
                pNewNode->ppSubNode = pCurNode->ppSubNode;
                while (pPrevNode->pNext)
                {
                    pPrevNode = pPrevNode->pNext;
                }
                pPrevNode->pNext = pNewNode;
                pNewNode->pPrev  = pPrevNode;
                return iterator(this, pNewNode);
            }
        }
        
        void TMdbNtcMultiKeyTree::Remove(TMdbNtcKeyTree::TNode* pCurNode)
        {
            if(pCurNode == m_pRootNode)
            {
                if(pCurNode->pData)
                {
                    if(m_bAutoRelease)
                    {
                        delete pCurNode->pData;
                    }
                    pCurNode->pData = NULL;
                    --m_uiSize;
                }        
                return ;
            }
            TMdbNtcKeyTree::TNode** ppParentNode = &pCurNode->pParentNode, *pParentNode = *ppParentNode;
            /*如果pCurNode是头结点，有两种场景需要重置pCurNode的父节点指向pCurNode的指针
              场景1:pCurNode有兄弟结点，即pCurNode->pNext为真
              场景2:pCurNode没有兄弟结点，而且pCurNode不存在子节点，即pCurNode->pNext为假，且pCurNode->ppSubNode == NULL
              说明:这两种场景都是可以delete掉pCurNode的情况，因此需要重置头指针
            */
            if(((TNode*)pCurNode)->pPrev == NULL && 
                  (((TNode*)pCurNode)->pNext || pCurNode->ppSubNode == NULL))
            {
                //获取父节点中保存的头结点指针的地址
                TNode** ppHeadNode = GetHeadAddr((TNode*)pCurNode);
                if(ppHeadNode)
                {
                    //将头结点中的指向下一个
                    *ppHeadNode = ((TNode*)pCurNode)->pNext;
                }
            }
            if(pCurNode->pData)
            {
                if(m_bAutoRelease)
                {
                    delete pCurNode->pData;
                }
                pCurNode->pData = NULL;
                --m_uiSize;
            }
            //有兄弟节点，则可以直接删除此节点
            if(((TNode*)pCurNode)->pPrev || ((TNode*)pCurNode)->pNext)
            {
                delete pCurNode;
                pCurNode = NULL;
            }
            //如果没有兄弟结点，而且无子结点，则可以直接删除，且应该向上回溯判断父节点是否可以删除
            else if(pCurNode->ppSubNode == NULL )
            {
                delete pCurNode;
                pCurNode = NULL;
                //检查父级下的子节点是否都为NULL
                while (pParentNode)
                {
                    MDB_UINT32 i = 0;
                    for (i=0; i < m_uiTNodeCount; ++i)
                    {
                        if(pParentNode->ppSubNode[i])
                        {
                            break;
                        }
                    }
                    if(i < m_uiTNodeCount)//有非空节点跳出循环
                    {
                        break;
                    }
                    else//没有非空子节点，则需要释放ppSubNode                
                    {
                        delete[] pParentNode->ppSubNode;
                        pParentNode->ppSubNode = NULL;
                        if(((TNode*)pParentNode)->pNext)
                        {
                            //需要对这些重复节点的ppSubNode都置为NULL
                            TNode* pTempNode = ((TNode*)pParentNode)->pNext;
                            do
                            {
                                pTempNode->ppSubNode = NULL;
                                pTempNode = pTempNode->pNext;
                            }while(pTempNode);
                        }
                        //当前父节点为根节点，或者含有数据，则无需继续回溯
                        if(pParentNode == m_pRootNode || pParentNode->pData)
                        {
                            break;
                        }
                        else
                        {
                            //得到当前节点在上级节点的子序号
                            i = (MDB_UINT32)(ppParentNode-pParentNode->pParentNode->ppSubNode);
                            if(i != (MDB_UINT32)-1) pParentNode->pParentNode->ppSubNode[i] = NULL;
                            ppParentNode = &pParentNode->pParentNode;
                            delete pParentNode;      
                            pParentNode = *ppParentNode;
                        }
                    }
                }
            }
        }
        
        //根据字符串键，删除数据
        int TMdbNtcMultiKeyTree::Remove(const char* pszKeyName)
        {
            if(pszKeyName == NULL) return 0;
            int iRetDelCount = 0;
            TMdbNtcKeyTree::TNode** ppCurNode = FindNode(pszKeyName);
            if(ppCurNode)
            {
                TNode* pCurNode = (TNode*)*ppCurNode, *pTempNode = NULL;
                //先删除后面的键值相同的节点
                if(pCurNode->pNext)
                {
                    pCurNode = pCurNode->pNext;
                    while (pCurNode)
                    {
                        ++iRetDelCount;
                        pTempNode = pCurNode;
                        pCurNode = pCurNode->pNext;
                        MdbNtcReleaseData(pTempNode->pData);
                        delete pTempNode;
                        pTempNode = NULL;
                    }
                    pCurNode = (TNode*)*ppCurNode;
                    pCurNode->pNext = NULL;
                }
                if(pCurNode->ppSubNode == NULL)
                {
                    m_uiSize -= (MDB_UINT32)iRetDelCount;
                    if(pCurNode->pData) ++iRetDelCount;
                    Remove(pCurNode);//size 在其中已经发生改变
                }
                else if(pCurNode->pData)
                {
                    ++iRetDelCount;
                    MdbNtcReleaseData(pCurNode->pData);
                    m_uiSize -= (MDB_UINT32)iRetDelCount;
                }
            }
            return iRetDelCount;
        }
        
        TMdbNtcMultiKeyTree::iterator TMdbNtcMultiKeyTree::IterLast() const
        {
            if(m_uiSize == 0) return IterEnd();
            TMdbNtcKeyTree::TNode * pCurNode = m_pRootNode;
            while (pCurNode != NULL && pCurNode->ppSubNode != NULL)
            {
                int i = 0;
                for (i = (int)m_uiTNodeCount-1; i >= 0; --i)
                {
                    if (pCurNode->ppSubNode[i] != NULL)
                    {
                        pCurNode = pCurNode->ppSubNode[i];
                        break;
                    }
                }
                if(i < 0)
                {
                    break;
                }
            }
            
            iterator itRet(this, pCurNode);
            
            if (pCurNode->pData == NULL)
            {
                --itRet;
            }
            else
            {
                //向右查找最后的重复节点
                while (((TNode*)pCurNode)->pNext)
                {
                    pCurNode = ((TNode*)pCurNode)->pNext;
                }
                itRet.pNodeObject = pCurNode;
            }
            return itRet;
        }
        
        TMdbNtcMultiKeyTree::iterator TMdbNtcMultiKeyTree::IterErase(TMdbNtcMultiKeyTree::iterator itor)
        {
            TMdbNtcKeyTree::TNode* pCurNode = (TMdbNtcKeyTree::TNode*)itor.pNodeObject;
            if(pCurNode == NULL)
            {
                return IterEnd();
            }
            ++itor;
            TMdbNtcMultiKeyTree::Remove(pCurNode);
            return itor;
        }
        
        //根据字符串键，查找键值对应的开始
        std::pair<TMdbNtcMultiKeyTree::iterator, TMdbNtcMultiKeyTree::iterator> TMdbNtcMultiKeyTree::EqualRange(const char* pszKeyName) const
        {
            std::pair<iterator, iterator> retPair(IterEnd(), IterEnd());
            TNode** ppHeadCurNode = (TNode**)FindNode(pszKeyName);
            if(ppHeadCurNode)
            {
                TNode* pCurNode = *ppHeadCurNode;
                if(pCurNode->pData)
                {
                    retPair.first.pNodeObject = pCurNode;
                    while(pCurNode->pNext)
                    {
                        pCurNode = pCurNode->pNext;
                    }
                    retPair.second.pNodeObject = pCurNode;
                    ++retPair.second;
                }
            }
            return retPair;
        }
        
        TMdbNtcMultiKeyTree::TNode** TMdbNtcMultiKeyTree::GetHeadAddr(TMdbNtcMultiKeyTree::TNode* pNode) const
        {
            if(pNode == NULL || pNode->pParentNode == NULL
                || pNode->pParentNode->ppSubNode == NULL) return NULL;
            TMdbNtcKeyTree::TNode** ppCurNode = NULL;
            for (unsigned int i = 0; i < m_uiTNodeCount; ++i)
            {
                ppCurNode = &pNode->pParentNode->ppSubNode[i];
                if(*ppCurNode && &(*ppCurNode)->ppSubNode == &pNode->ppSubNode)
                {
                    return (TNode**)ppCurNode;
                }
            }
            return NULL; 
        }
        
        int TMdbNtcMultiKeyTree::GetNodeIndex(TMdbNtcMultiKeyTree::TNode* pNode, TMdbNtcMultiKeyTree::TNode* pHeadNode /*= NULL*/) const
        {
            if(pHeadNode == NULL)
            {
                TNode** ppHeadNode = GetHeadAddr(pNode);
                pHeadNode = ppHeadNode?*ppHeadNode:NULL;
            }
            if(pHeadNode)
            {
                TNode* pCurNode = pHeadNode;
                //找到首节点了
                int iIndex = 0;
                do
                {
                    if(pCurNode == pNode)
                    {
                        return iIndex;
                    }
                    else
                    {
                        ++iIndex;
                    }
                    pCurNode = pCurNode->pNext;
                }while(pCurNode);
                return -1;
            }
            return -1;
        }
        
        int TMdbNtcMultiKeyTree::GetKeyValueCount(TMdbNtcMultiKeyTree::TNode* pNode) const
        {
            int iCount = 0;
            while (pNode && pNode->pData)
            {
                ++iCount;
                pNode = pNode->pNext;
            }
            return iCount;
        }
        
        //获得前一个迭代器
        TMdbNtcMultiKeyTree::iterator TMdbNtcMultiKeyTree::IterPrev(TMdbNtcMultiKeyTree::iterator itCur, int iStep /* = -1 */) const
        {
            if (m_pRootNode == NULL)
            {
                return IterEnd();
            }
        
            if (iStep <= -1 )
            {
                iterator itRet = itCur;
                itRet.iLastStep = 0;        
                TNode* pCurNode = (TNode *)itCur.pNodeObject;
                bool bSearchChild = false;
                //寻找前驱节点，向兄弟节点查找，如果没有非空，则向父节点回溯
                while (pCurNode)
                {
                    //如果当前节点不是迭代器所指，则获取当前节点的子节点
                    if(bSearchChild)
                    {
                        if(pCurNode->ppSubNode)
                        {
                            int i = 0;
                            for (i = (int)m_uiTNodeCount-1; i >= 0; --i)
                            {
                                if(pCurNode->ppSubNode[i])
                                {
                                    pCurNode = (TNode*)pCurNode->ppSubNode[i];//current转移到子级
                                    break;
                                }
                            }
                            //找到非空子节点
                            if(i >= 0)
                            {
                                continue;
                            }
                        }
                    }
                    else if(pCurNode->pPrev)
                    {
                        pCurNode = pCurNode->pPrev;
                    }
                    if(pCurNode != itCur.pNodeObject && pCurNode->pData)
                    {
                        while(1)
                        {
                            --itRet.iLastStep;
                            if(itRet.iLastStep == iStep || pCurNode->pPrev == NULL)
                            {
                                break;
                            }
                            pCurNode = pCurNode->pPrev;
                        }
                        if(itRet.iLastStep == iStep)
                        {
                            itRet.pNodeObject = pCurNode;
                            break;
                        }
                    }
                    
                    if(pCurNode->pParentNode)
                    {
                        //查找当前节点在父节点中的位置
                        int i = ReverseFindIndex(pCurNode);
                        //判断是否找到当前节点在兄弟节点的位置
                        if(i >= 0)
                        {
                            i = this->FindSiblingPrevIndex(&pCurNode->pParentNode->ppSubNode[i]);
                            if(i < 0)
                            {
                                pCurNode = (TNode*)pCurNode->pParentNode;
                                bSearchChild = false;
                            }
                            else
                            {
                                pCurNode = (TNode*)pCurNode->pParentNode->ppSubNode[i];
                                bSearchChild = true;
                            }
                        }
                        else//如果没有找到位置，这是不可能的，说明有问题
                        {
                            break;//跳出循环，结束查找
                        }
                    }
                    else
                    {                
                        break;//跳出循环，结束查找
                    }
                }
                if(itRet.iLastStep > iStep)
                {
                    itRet.pNodeObject = NULL;
                    --itRet.iLastStep;
                }
                return itRet;
            } 
            else if (iStep == 0)
            {
                return itCur;
            }
            else
            {
                return IterNext(itCur, iStep);
            }
        }
        
        //获得后一个迭代器
        TMdbNtcMultiKeyTree::iterator TMdbNtcMultiKeyTree::IterNext(TMdbNtcMultiKeyTree::iterator itCur, int iStep /* = 1 */) const
        {
            if (m_pRootNode == NULL)
            {
                return IterEnd();
            }
        
            if (iStep >= 1 )
            {
                iterator itRet = itCur;
                itRet.iLastStep = 0;        
                TNode* pCurNode = (TNode *)itCur.pNodeObject;        
                //寻找右继节点，如果存在最左子节点则即为右继节点，否则向右兄弟节点或者向父级查找
                while (pCurNode)
                {
                    if(pCurNode->pData && pCurNode != itCur.pNodeObject)
                    {
                        itRet.pNodeObject = pCurNode;
                        ++itRet.iLastStep;
                        if(itRet.iLastStep == iStep)
                        {
                            break;
                        }
                    }
                    while(pCurNode->pNext)
                    {
                        pCurNode = pCurNode->pNext;
                        itRet.pNodeObject = pCurNode;
                        ++itRet.iLastStep;
                        if(itRet.iLastStep == iStep)
                        {
                            break;
                        }
                    }
                    if(itRet.iLastStep == iStep)
                    {
                        break;
                    }
                    if(pCurNode->ppSubNode)
                    {
                        unsigned int i = 0;
                        for (i=0; i < m_uiTNodeCount; ++i)
                        {
                            if(pCurNode->ppSubNode[i])
                            {
                                pCurNode = (TNode*)pCurNode->ppSubNode[i];//current转移到子级
                                break;
                            }
                        }
                        //找到非空子节点
                        if(i < m_uiTNodeCount)
                        {
                            continue;
                        }
                    }
                    if(pCurNode->pParentNode)
                    {
                        MDB_UINT32 i = 0;
                        TMdbNtcKeyTree::TNode** ppParentNode = &pCurNode->pParentNode, *pParentNode = *ppParentNode;
                        //查找当前节点在父节点中的位置
                        TNode * pHeadNode = pCurNode;
                        while( pHeadNode->pPrev != NULL) 
                        {
                            pHeadNode = pHeadNode->pPrev;
                        }
                        i = (MDB_UINT32)FindIndex(pHeadNode);
                        //判断是否找到当前节点在兄弟节点的位置
                        if(i < m_uiTNodeCount)
                        {
                            pCurNode = (TNode*)FindSiblingOrParentNext(&pParentNode->ppSubNode[i]);
                            if(pCurNode == NULL)
                            {
                                break;//跳出循环，结束查找
                            }
                        }
                        else//如果没有找到位置，这是不可能的，说明有问题
                        {
                            break;//跳出循环，结束查找
                        }
                    }
                    else
                    {                
                        break;//跳出循环，结束查找
                    }
                }
                if(itRet.iLastStep < iStep)
                {
                    itRet.pNodeObject = NULL;
                    ++itRet.iLastStep;
                }
                return itRet;
            } 
            else if (iStep == 0)
            {
                return itCur;
            }
            else
            {
                return IterNext(itCur, iStep);
            }
        }
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcDigitKeyTree, TMdbNtcKeyTree);
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcMultiDigitKeyTree, TMdbNtcMultiKeyTree);
        /*
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcQueue, TMdbNtcContainer);
        TMdbNtcQueue::TMdbNtcQueue(bool bPushLock , bool bPopLock )
        {
            m_pHeadNode = m_pTailNode = new TNode;
            if(bPushLock)
            {
                m_pTailMutex = new TMdbNtcThreadLock;
            }
            else
            {
                m_pTailMutex = NULL;
            }
            if(bPopLock)
            {
                m_pHeadMutex = new TMdbNtcThreadLock;
            }
            else
            {
                m_pHeadMutex = NULL;
            }
            m_uiPushTimes = 0;
            m_uiPopTimes = 0;
        }

        TMdbNtcQueue::~TMdbNtcQueue()
        {
            Clear();
            if(m_pHeadMutex)
            {
                delete m_pHeadMutex;
                m_pHeadMutex = NULL;
            }
            if(m_pTailMutex)
            {
                delete m_pTailMutex;
                m_pTailMutex = NULL;
            }
            if(m_pHeadNode)
            {
                delete m_pHeadNode;
                m_pHeadNode = NULL;
                m_pTailNode = NULL;
            }
        }
        
        bool TMdbNtcQueue::Push(TMdbNtcBaseObject* pData)
        {    
            TNode* pNode = new TNode(pData); 
            if(m_pTailMutex) m_pTailMutex->Lock();
            //队列满，直接返回
            if(((m_uiPushTimes + 1)%MDB_NTC_MAX_QUENE_SIZE) == m_uiPopTimes)
            {
                if(m_pTailMutex) m_pTailMutex->Unlock();
                return false;
            }
            //m_uiPushTimes增1
            m_uiPushTimes = (m_uiPushTimes+1)%MDB_NTC_MAX_QUENE_SIZE;
            
            m_pTailNode->pNext = pNode;
            m_pTailNode = pNode;
            if(m_pTailMutex) m_pTailMutex->Unlock();
            return true;
        }

        void TMdbNtcQueue::Push(TMdbNtcContainer::iterator itor_begin, TMdbNtcContainer::iterator itor_end )
        {
            iterator itor = itor_begin;
            while (itor != itor_end)
            {
                Push(itor.data());
                ++itor;
            }
        }
        
        TMdbNtcBaseObject* TMdbNtcQueue::Pop()
        {
            if(m_pHeadNode == m_pTailNode) return NULL;
            TMdbNtcBaseObject* pData = NULL;    
            if(m_pHeadMutex) m_pHeadMutex->Lock(); 
            if(m_pHeadNode == m_pTailNode) 
            {
                if(m_pHeadMutex) m_pHeadMutex->Unlock();
                return NULL;
            }
            TNode* pNewHeadNode = m_pHeadNode->pNext, *pOldHead = m_pHeadNode;
            //m_uiPopTimes增1
            m_uiPopTimes = (m_uiPopTimes + 1) % MDB_NTC_MAX_QUENE_SIZE;
            pData = pNewHeadNode->pData;
            m_pHeadNode = pNewHeadNode;
            m_pHeadNode->pData = NULL;

            if(m_pHeadMutex) m_pHeadMutex->Unlock();

            delete pOldHead;
            pOldHead = NULL;
            
            return pData;
        }
        
        unsigned int TMdbNtcQueue::GetSize() const
        {
            return (m_uiPushTimes-m_uiPopTimes+MDB_NTC_MAX_QUENE_SIZE)%MDB_NTC_MAX_QUENE_SIZE; 
        }
        
        unsigned int TMdbNtcQueue::GetDataMemoryUsage() const
        {
            if(m_pHeadNode == m_pTailNode) return 0;
            unsigned uiSize = 0;
            if(m_pHeadMutex) m_pHeadMutex->Lock();
            uiSize = TMdbNtcContainer::GetDataMemoryUsage();
            if(m_pHeadMutex) m_pHeadMutex->Unlock();
            return uiSize;
        }
        
        MDB_UINT32 TMdbNtcQueue::GetContainerMemoryUsage() const
        {
            //算TNode时需要算上空节点
            return (MDB_UINT32)(sizeof(TMdbNtcQueue)+sizeof(TMdbNtcThreadLock)*2+sizeof(TNode)*(GetSize()+1));
        }
        
        void TMdbNtcQueue::Clear()
        {
            if(m_pHeadNode == m_pTailNode) return;
            if(m_pHeadMutex) m_pHeadMutex->Lock();    
            TNode* pCurNode = m_pHeadNode->pNext, *pPrevNode = NULL;
            while (pCurNode)
            {
                pPrevNode = pCurNode;
                pCurNode = pCurNode->pNext;
                MdbNtcReleaseData(pPrevNode->pData);
                delete pPrevNode;
                pPrevNode = NULL;
            }
            m_pTailNode = m_pHeadNode;
            m_uiPopTimes = m_uiPushTimes;
            if(m_pHeadMutex) m_pHeadMutex->Unlock();
        }
        
        void TMdbNtcQueue::Print(FILE* fp ) const
        {
            if(m_pHeadNode == m_pTailNode) return;
            if(m_pHeadMutex) m_pHeadMutex->Lock();
            TMdbNtcContainer::Print(fp);
            if(m_pHeadMutex) m_pHeadMutex->Unlock();
        }
        
        TMdbNtcQueue::iterator TMdbNtcQueue::IterBegin() const
        {
            if(m_pHeadNode == m_pTailNode)  return IterEnd();
            else return iterator(this, m_pHeadNode->pNext);
        }
        
        TMdbNtcQueue::iterator TMdbNtcQueue::IterLast() const
        {
            if(m_pHeadNode == m_pTailNode)  return IterEnd();
            else return iterator(this, m_pTailNode);
        }
        
        TMdbNtcQueue::iterator TMdbNtcQueue::IterErase(iterator itor)
        {
            
            if(itor.pNodeObject == NULL) return IterEnd();
            TNode* pCurNode = m_pHeadNode->pNext, *pPrevNode = m_pHeadNode;
            while (pCurNode && itor.pNodeObject != pCurNode)
            {
                pPrevNode = pCurNode;
                pCurNode = pCurNode->pNext;        
            }     
            if(pCurNode)//找到
            {
                m_uiPopTimes = (m_uiPopTimes + 1) % MDB_NTC_MAX_QUENE_SIZE;
                pPrevNode->pNext = pCurNode->pNext;
                MdbNtcReleaseData(pCurNode->pData);
                delete pCurNode;
                pCurNode = NULL;
                return iterator(this, pPrevNode->pNext);
            }
            else
            {
                return IterEnd();
            }
        }
        
        void TMdbNtcQueue::IterSwap(iterator itor1, iterator itor2)
        {
            if(itor1.pNodeObject && itor2.pNodeObject)
            {
                TMdbNtcBaseObject* pTmpData = itor1.data();
                itor1.pNodeObject->pData = itor2.pNodeObject->pData;
                itor2.pNodeObject->pData = pTmpData;
            }
        }
        
        TMdbNtcQueue::iterator TMdbNtcQueue::IterPrev(iterator itCur, int iStep ) const
        {
            if(itCur.pNodeObject == NULL) return IterEnd();
            TNode* pCurNode = m_pHeadNode->pNext, *pPrevNode = m_pHeadNode;
            while (pCurNode && itCur.pNodeObject != pCurNode)
            {
                pPrevNode = pCurNode;
                pCurNode = pCurNode->pNext;
            }
            if(pCurNode == NULL || pPrevNode == m_pHeadNode) return IterEnd();//没有找到此节点    
            else return iterator(this, pPrevNode);
        }
        
        TMdbNtcQueue::iterator TMdbNtcQueue::IterNext(iterator itCur, int iStep) const
        {
            if(itCur.pNodeObject == NULL) return IterEnd();
            else return iterator(this, ((TNode*)itCur.pNodeObject)->pNext);
        }

        MDB_ZF_IMPLEMENT_OBJECT(TBlockingQueue, TMdbNtcQueue);
        TBlockingQueue::TBlockingQueue(bool bPushLock , bool bPopLock )
            :TMdbNtcQueue(bPushLock, bPopLock)
        {
            m_pPopEvent  = new TThreadEvent();
            m_pPushEvent = new TThreadEvent();
            m_pPushEvent->SetEvent();
        }

        TBlockingQueue::~TBlockingQueue()
        {
            if(m_pPopEvent)
            {
                delete m_pPopEvent;
                m_pPopEvent = NULL;
            }

            if(m_pPushEvent)
            {
                delete m_pPushEvent;
                m_pPushEvent = NULL;
            }
            
        }

        bool TBlockingQueue::IsOK()
        {
            bool bRet = true;
            do
            {
            } while (0);
            return bRet;
        }
        
        void TBlockingQueue::Push(TMdbNtcContainer::iterator itor_begin, TMdbNtcContainer::iterator itor_end)
        {
            bool bNotify = false;
            if(m_pTailMutex) m_pTailMutex->Lock();
            TMdbNtcContainer::iterator itor = itor_begin;
            while(itor != itor_end)
            {
                TNode* pNode = new TNode(itor.data());
                ++m_uiPushTimes;
                m_pTailNode->pNext = pNode;
                m_pTailNode = pNode;
                if(!bNotify && ABS(m_uiPushTimes-m_uiPopTimes)==1)
                {
                    bNotify = true;
                }
                ++itor;
            }
            if(m_pTailMutex) m_pTailMutex->Unlock();
            if(bNotify && m_uiPushTimes != m_uiPopTimes)
            {
                m_pEvent->SetEvent();
            }
        }
        
        bool TBlockingQueue::Push(TMdbNtcBaseObject* pData,int iMilliSeconds)
        {
            bool bNotify = false, bLocked = false;
            if(iMilliSeconds == 0)
            {
                //队列满，直接返回
                if((m_uiPushTimes + 1)%MDB_NTC_MAX_QUENE_SIZE == m_uiPopTimes)
                {
                    return false;
                }
                else
                {
                    bLocked = true;
                    if(m_pTailMutex) 
                    {
                        m_pTailMutex->Lock();
                    }
                }
                
            }

            do
            {
                if(bLocked)
                {
                    //队列满,失败返回
                    if((m_uiPushTimes + 1)%MDB_NTC_MAX_QUENE_SIZE == m_uiPopTimes)
                    {
                        if(m_pTailMutex) 
                        {
                            m_pTailMutex->Unlock();
                        }
                        return false;
                    }
                    else
                    {
                        bNotify = true;
                        break;
                    }
                }
                
                if(!m_pPushEvent->Wait(iMilliSeconds))
                {
                    //超时，失败返回
                    return false;
                }
                else if(mdb_ntc_zthread_testcancel())
                {
                    //如果存在争用，通知到下一个线程去做，自己退出
                    if(m_pTailMutex && ((m_uiPushTimes + 1)%MDB_NTC_MAX_QUENE_SIZE != m_uiPopTimes))
                    {                            
                        m_pPushEvent->SetEvent();
                    }
                    return false;
                }
                else
                {

                    bLocked = true;
                    if(m_pTailMutex) 
                    {
                        m_pTailMutex->Lock();
                    }
                }
                
            }while(1);
            TNode* pNode = new TNode(pData);
            //m_uiPushTimes增1
            m_uiPushTimes = (m_uiPushTimes + 1 )%MDB_NTC_MAX_QUENE_SIZE;
            m_pTailNode->pNext = pNode;
            m_pTailNode = pNode;
            if(GetSize() == 1)
            {
                m_pPopEvent->SetEvent();
            }
            bNotify &= (GetSize()< MDB_NTC_MAX_QUENE_SIZE-1); 
            if(m_pTailMutex) 
            {
                m_pTailMutex->Unlock();
            }
            if(bNotify)
            {
                m_pPushEvent->SetEvent();
            }

            return true;
            
        }

        TMdbNtcBaseObject* TBlockingQueue::Pop(int iMilliSeconds)
        {
            bool bNotify = false, bLocked = false; //是否需要通知其他等待Pop的线程
            if(iMilliSeconds == 0)
            {
                if(m_pHeadNode == m_pTailNode) return NULL;
                else
                {
                    bLocked = true;
                    if(m_pHeadMutex) m_pHeadMutex->Lock();
                }
            }
            do
            {
                if(bLocked)//加锁情况下，得知为空，则需要解锁后wait
                {
                    if(m_pHeadNode == m_pTailNode)
                    {
                        if(m_pHeadMutex)
                        {
                            m_pHeadMutex->Unlock();
                        }
                        return NULL;
                    }
                    else
                    {
                        bNotify = true;
                        break;
                    }
                }
                //如果等待过程中接到通知，则返回true
                if(!m_pPopEvent->Wait(iMilliSeconds))
                {
                    return NULL;
                }
                else if(mdb_ntc_zthread_testcancel())//需要退出
                {
                    //如果存在争用，通知到下一个线程去做，自己退出
                    if(m_pHeadMutex && m_pHeadNode != m_pTailNode)
                    {                            
                        m_pPopEvent->SetEvent();
                    }
                    return NULL;
                }
                else
                {
                    bLocked = true;
                    if(m_pHeadMutex) m_pHeadMutex->Lock();
                }
            } while (1);
            TMdbNtcBaseObject* pRetObj = NULL;
            TNode* pNewHeadNode = m_pHeadNode->pNext, *pOldHead = m_pHeadNode;
            m_uiPopTimes = (m_uiPopTimes + 1) % MDB_NTC_MAX_QUENE_SIZE;
            pRetObj = pNewHeadNode->pData;
            pNewHeadNode->pData = NULL;
            m_pHeadNode = pNewHeadNode;
            //队列从满变成非满
            if((m_uiPopTimes-m_uiPushTimes+MDB_NTC_MAX_QUENE_SIZE)%MDB_NTC_MAX_QUENE_SIZE == 2)
            {
                m_pPushEvent->SetEvent();
            }
            bNotify &= (m_pHeadNode != m_pTailNode);
            if(m_pHeadMutex) m_pHeadMutex->Unlock();
            delete pOldHead;
            pOldHead = NULL;
            if(bNotify)//只有在竞争的情况下，才会通知下一个。
            {
                m_pPopEvent->SetEvent();
            }
            return pRetObj;
        }

        void TBlockingQueue::Wakeup()
        {
            for (MDB_UINT32 i = 0; i < m_pPopEvent->GetWaitCount(); ++i)
            {
                m_pPopEvent->PulseEvent();
            }

            for (MDB_UINT32 i = 0; i < m_pPushEvent->GetWaitCount(); ++i)
            {
                m_pPushEvent->PulseEvent();
            }
        }
        */

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcQueue, TMdbNtcContainer);
        TMdbNtcQueue::TMdbNtcQueue()
        {
            m_pHeadNode   = new TNode;
            m_pTailNode   = m_pHeadNode;
            m_uiSize      = 0;
            m_uiPushTimes = 0;
            m_uiPopTimes  = 0;
            m_pMutex      = new TMdbNtcThreadLock;
            m_pPopCond    = new TMdbNtcThreadCond(m_pMutex);
            m_pPushCond   = new TMdbNtcThreadCond(m_pMutex);
        }

        TMdbNtcQueue::~TMdbNtcQueue()
        {
            Clear();
            
            m_uiSize      = 0;
            m_uiPushTimes = 0;
            m_uiPopTimes  = 0;
            
            if(m_pMutex)
            {
                delete m_pMutex;
                m_pMutex = NULL;
            }
            if(m_pPopCond)
            {
                delete m_pPopCond;
                m_pPopCond = NULL;
            }

            if(m_pPushCond)
            {
                delete m_pPushCond;
                m_pPushCond = NULL;
            }

            if(m_pHeadNode)
            {
                delete m_pHeadNode;
                m_pHeadNode = NULL;
                m_pTailNode = NULL;
            }
        }

        bool TMdbNtcQueue::IsOK()
        {
            bool bRet = true;
            do
            {
            } while (0);
            return bRet;
        }

        bool TMdbNtcQueue::Push(TMdbNtcBaseObject* pData, int iMilliSeconds /* = -1 */)
        {
            bool bNotify = false;
            TNode* pNode = new TNode(pData);
            //加锁
            m_pPushCond->Lock();

            //队列满
            if(m_uiSize >= MDB_NTC_MAX_QUENE_SIZE)
            {
                bool bError = false;
                if(iMilliSeconds != 0)
                {
                    while(m_uiSize == MDB_NTC_MAX_QUENE_SIZE)
                    {
                        if(!m_pPushCond->Wait(iMilliSeconds))
                        {
                            //超时返回
                            bError = true;
                            break;
                        }
                        else if(mdb_ntc_zthread_testcancel())//需要退出
                        {
                            //当前线程被取消,通知其他push线程放数据
                            if(m_uiSize < MDB_NTC_MAX_QUENE_SIZE)
                            {
                                bNotify = true;
                            }
                            bError = true;
                            break; 
                        }
                        
                    }
                    
                }
                else
                {
                    bError = true;
                }

                if(bError)
                {
                    m_pPushCond->Unlock();
                    delete pNode;
                    pNode = NULL;
                    if(bNotify)
                    {
                        m_pPushCond->SetEvent();
                    }
                    return false;
                }
                
            }

            ++m_uiSize;
            ++m_uiPushTimes;
            m_pTailNode->pNext = pNode;
            m_pTailNode = pNode;
            
            if(m_uiSize == 1)
            {
                bNotify = true;
            }
            //解锁
            m_pPushCond->Unlock();
            if(bNotify)
            {
                m_pPopCond->SetEvent();//通知pop线程可以取数据了
            }

            return true;
        }

        TMdbNtcBaseObject* TMdbNtcQueue::Pop(int iMilliSeconds /* = -1 */)
        {
            bool bNotify = false;
            //加锁
            m_pPopCond->Lock();
            //队列空
            if(m_uiSize <= 0)
            {
                bool bError = false;
                if(iMilliSeconds != 0)
                {
                    while( 0 == m_uiSize)
                    {
                        if(!m_pPopCond->Wait(iMilliSeconds))
                        {
                            //超时返回
                            bError = true;
                            break;
                        }
                        else if(mdb_ntc_zthread_testcancel())//需要退出
                        {
                            //当前线程被取消,通知其他pop线程取数据
                            if(m_uiSize > 0)
                            {
                                bNotify = true;
                            }
                            bError = true;
                            break; 
                        }
                    }
                }
                else
                {
                    bError = true;
                }
                
                if(bError)
                {
                    m_pPopCond->Unlock();
                    if(bNotify)
                    {
                        m_pPopCond->SetEvent();
                    }
                    return NULL;
                }
                
            }

            
            TNode* pOldHead     = m_pHeadNode;
            m_pHeadNode         = m_pHeadNode->pNext;
            TMdbNtcBaseObject*pRetObj = m_pHeadNode->pData;
            m_pHeadNode->pData  = NULL;
            
            --m_uiSize;
            ++m_uiPopTimes;
            if(m_uiSize == MDB_NTC_MAX_QUENE_SIZE-1)
            {
                bNotify = true;
            }
            
            //解锁
            m_pPopCond->Unlock();
            if(bNotify)
            {
                m_pPushCond->SetEvent();//通知push线程可以放数据了
            }
            //删除旧的头结点
            delete pOldHead;
            pOldHead = NULL;
            
            return pRetObj;
        }


        void TMdbNtcQueue::Wakeup()
        {
            m_pPushCond->Lock();
            for (MDB_UINT32 i = 0; i < m_pPushCond->GetWaitCount(); ++i)
            {
                m_pPushCond->PulseEvent();
            }
            m_pPushCond->Unlock();
            m_pPopCond->Lock();
            for (MDB_UINT32 i = 0; i < m_pPopCond->GetWaitCount(); ++i)
            {
                m_pPopCond->PulseEvent();
            }
            m_pPopCond->Unlock();
        }

        unsigned int TMdbNtcQueue::GetDataMemoryUsage() const
        {
            if(0 == m_uiSize) return 0;
            unsigned uiSize = 0;
            if(m_pMutex) m_pMutex->Lock();
            uiSize = TMdbNtcContainer::GetDataMemoryUsage();
            if(m_pMutex) m_pMutex->Unlock();
            return uiSize;
        }

        MDB_UINT32 TMdbNtcQueue::GetContainerMemoryUsage() const
        {
            //算TNode时需要算上空节点
            return (MDB_UINT32)(sizeof(TMdbNtcQueue)+sizeof(TMdbNtcThreadLock)+sizeof(TMdbNtcThreadCond)*2+sizeof(TNode)*(GetSize()+1));
        }

        void TMdbNtcQueue::Clear()
        {
            if(m_uiSize == 0) return;

            if(m_pMutex) m_pMutex->Lock(); 
            TNode* pCurNode = m_pHeadNode->pNext, *pPrevNode = NULL;
            while (pCurNode)
            {
                pPrevNode = pCurNode;
                pCurNode = pCurNode->pNext;
                MdbNtcReleaseData(pPrevNode->pData);
                delete pPrevNode;
                pPrevNode = NULL;
            }
            m_pTailNode = m_pHeadNode;
            m_uiSize    = 0;
            if(m_pMutex) m_pMutex->Unlock();
        }

        void TMdbNtcQueue::Print(FILE* fp /*= NULL*/) const
        {
            if(m_uiSize == 0) return;
            if(m_pMutex) m_pMutex->Lock();
            TMdbNtcContainer::Print(fp);
            if(m_pMutex) m_pMutex->Unlock();
        }


        TMdbNtcQueue::iterator TMdbNtcQueue::IterBegin() const
        {
            if(m_uiSize == 0)  return IterEnd();
            else return iterator(this, m_pHeadNode->pNext);
        }
        
        TMdbNtcQueue::iterator TMdbNtcQueue::IterLast() const
        {
            if(m_uiSize == 0)  return IterEnd();
            else return iterator(this, m_pTailNode);
        }
        
        TMdbNtcQueue::iterator TMdbNtcQueue::IterErase(iterator itor)
        {
            
            if(itor.pNodeObject == NULL) return IterEnd();
            TNode* pCurNode = m_pHeadNode->pNext, *pPrevNode = m_pHeadNode;
            while (pCurNode && itor.pNodeObject != pCurNode)
            {
                pPrevNode = pCurNode;
                pCurNode = pCurNode->pNext;        
            }     
            if(pCurNode)//找到
            {
                --m_uiSize;
                ++m_uiPopTimes;
                pPrevNode->pNext = pCurNode->pNext;
                MdbNtcReleaseData(pCurNode->pData);
                delete pCurNode;
                pCurNode = NULL;
                return iterator(this, pPrevNode->pNext);
            }
            else
            {
                return IterEnd();
            }
        }

        void TMdbNtcQueue::IterSwap(iterator itor1, iterator itor2)
        {
            if(itor1.pNodeObject && itor2.pNodeObject)
            {
                TMdbNtcBaseObject* pTmpData = itor1.data();
                itor1.pNodeObject->pData = itor2.pNodeObject->pData;
                itor2.pNodeObject->pData = pTmpData;
            }
        }

        TMdbNtcQueue::iterator TMdbNtcQueue::IterPrev(iterator itCur, int iStep /*= -1*/) const
        {
            if(itCur.pNodeObject == NULL) return IterEnd();
            TNode* pCurNode = m_pHeadNode->pNext, *pPrevNode = m_pHeadNode;
            while (pCurNode && itCur.pNodeObject != pCurNode)
            {
                pPrevNode = pCurNode;
                pCurNode = pCurNode->pNext;
            }
            if(pCurNode == NULL || pPrevNode == m_pHeadNode) return IterEnd();//没有找到此节点    
            else return iterator(this, pPrevNode);
        }
        
        TMdbNtcQueue::iterator TMdbNtcQueue::IterNext(iterator itCur, int iStep /*= 1*/) const
        {
            if(itCur.pNodeObject == NULL) return IterEnd();
            else return iterator(this, ((TNode*)itCur.pNodeObject)->pNext);
        }
        
//}

