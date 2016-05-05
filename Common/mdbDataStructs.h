/********************************************************
*   Copyright (c) 2003-2015 ZTESoft Technology Co.,Ltd.
*
*   All rights rescrved.
*
*   @file    Sdk/mdbDataStructs.h
*   @brief   算法以及数据结构定义
*   @version 1.0
*   @author  Jiang.jinzhou ,zhang.he
*   @date    Design 2012/03/21,Programming2012/07/05
*   @bug     (新建，无bug)
*   @warning (公用功能，统一使用)
*********************************************************/
#ifndef _MDB_H_DataStructs_
#define _MDB_H_DataStructs_

#include "Common/mdbCommons.h"
#include "Common/mdbBaseObject.h"
#include <utility>

//namespace QuickMDB
//{
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        //&&&&                                                              &&&&&&
        //&&&&                           算法部分                           &&&&&&
        //&&&&                                                              &&&&&&
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        
        /**
         * @brief 比较函数，需要自定义比较时可用,重载Compare即可
         * 
         */
        class TMdbNtcObjCompare:public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcObjCompare);
        public:
            TMdbNtcObjCompare(){};
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject* pObject1, const TMdbNtcBaseObject* pObject2) const
            {
                return pObject1->Compare(pObject2);
            }
        };        
        
        /**
         * @brief 指针比较函数，需要自定义比较时可用,重载Compare即可
         * 
         */
        class TMdbNtcPointerCompare:public TMdbNtcObjCompare
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcPointerCompare);
        public:
            TMdbNtcPointerCompare(){};
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject* pObject1, const TMdbNtcBaseObject* pObject2) const
            {
                return pObject2 == pObject1 ? 0:(pObject1 > pObject2 ? 1:-1);
            }
        };

        extern TMdbNtcObjCompare     g_oMdbNtcObjectCompare;///< 对象比较方法
        extern TMdbNtcPointerCompare g_oMdbNtcPointerCompare;///< 指针比较方法

        /**
         * @brief 不区分大小写比较TStringObject
         * 
         */
        class TMdbNtcNoCaseCompare:public TMdbNtcObjCompare
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcNoCaseCompare);
        public:
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject* pObject1, const TMdbNtcBaseObject* pObject2) const
            {
                const TMdbNtcStringObject *p1 =  static_cast<const TMdbNtcStringObject*>(pObject1), *p2 = static_cast<const TMdbNtcStringObject*>(pObject2);
                return static_cast<const TMdbNtcStringBuffer*>(p1)->Compare(*static_cast<const TMdbNtcStringBuffer*>(p2), false);//不区分大小写排序
            }
        };

        
        /**
         * @brief 二分查找
         * 
         * @param pDataHead   [in] 数组的首地址
         * @param uiDataSize   [in] 数组的大小
         * @param pCompareObj [in] 相比较的数据信息
         * @param pCompare    [in] 比较函数，如果为NULL，则只进行指针比较
         * @return int
         * @retval 对应数组的下标 >=0 查找成功，-1查找失败
         */
        int MdbNtcBinarySearch(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
        /**
         * @brief 排序算法的基类
         * 
         */
        class TMdbNtcSort:public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcSort);
            /** \example  example_Algorithm.cpp
             * This is an example of how to use the TMdbNtcSort class.
             * More details about this example.
             */ 
        public:
            /**
             * @brief 构造函数
             * 
             * @param bSortAsc [in] 升序与否
             */
            TMdbNtcSort(bool bSortAsc = true)
            {
                m_bSortAsc = bSortAsc;
            }
            /**
             * @brief 设置升序还是倒序
             * 
             * @param bSortAsc [in] 升序与否
             */
            void SetSortAsc(bool bSortAsc)
            {
                m_bSortAsc = bSortAsc;
            }
            /**
             * @brief 判断是否为升序
             * 
             * @return bool
             */
            bool IsSortAsc()
            {
                return m_bSortAsc;
            }
            /**
             * @brief 按照传入的数组首地址排序
             * 
             * @param pDataHead  [in] 数组的首地址
             * @param uiDataSize [in] 数组的大小
             * @param oCompare   [in] 比较函数
             */
            virtual void Sort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare) const = 0 ;
            /**
             * @brief 交换数据
             * 
             * @param pDataHead   [in] 数组的首地址
             * @param uiLocation1 [in] 交换元素的下标1
             * @param uiLocation2 [in] 交换元素的下标2
             * @param oCompare    [in] 比较函数
             */
            void Swap(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiLocation1, MDB_UINT32 uiLocation2) const
            {
                TMdbNtcBaseObject * pTempObj = NULL;
                pTempObj = pDataHead[uiLocation1];
                pDataHead[uiLocation1] = pDataHead[uiLocation2];
                pDataHead[uiLocation2] = pTempObj;
            }
        protected:
            bool m_bSortAsc;///< 是否为正序
        };
        
        /**
         * @brief 快速排序
         * 快速排序在最好情况下为O（nlog(2)(n)），最坏情况为O（n∧2）
         * 快速排序是递归的，对于内存非常有限的机器来说，它不是一个好的选择，适合数据量不是很大的场合
         */
        class TMdbNtcQuickSort:public TMdbNtcSort
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcQuickSort);
           /** \example  example_Algorithm.cpp
             * This is an example of how to use the TMdbNtcQuickSort class.
             * More details about this example.
             */ 
        public:
            /**
             * @brief 构造函数
             * 
             * @param bSortAsc [in] 升序与否
             */
            TMdbNtcQuickSort(bool bSortAsc = true):TMdbNtcSort(bSortAsc)
            {
            }
            /**
             * @brief 按照传入的数组首地址排序
             * 
             * @param pDataHead  [in] 数组的首地址
             * @param uiDataSize [in] 数组的大小
             * @param pCompare   [in] 比较函数
             */
            virtual void Sort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare) const;
        protected:
            /**
             * @brief 递归快速排序算法
             * 
             * @param pDataHead  [in] 数组的首地址
             * @param uiLow      [in] 开始排序的最低位置
             * @param uiHigh     [in] 结束排序的最高位置
             * @param oCompare   [in] 比较函数
             */
            void QSort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiLow, MDB_UINT32 uiHigh,const TMdbNtcObjCompare &oCompare) const;
            /**
             * @brief 一趟快速排序算法
             * 
             * @param pDataHead       [in] 数组的首地址
             * @param uiLow           [in] 开始排序的最低位置
             * @param uiHigh          [in] 结束排序的最高位置
             * @param oCompare        [in] 比较函数
             * @param bIsExchangeLow  [in] 一趟排序中，low指针上移过程中是否发生过数据交换
             * @param bIsExchangeHigh [in] 一趟排序中，high指针下移过程中是否发生过数据交换
             * @return MDB_UINT32
             * @retval 枢轴位置
             */
            MDB_UINT32 Partion(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiLow, MDB_UINT32 uiHigh,const TMdbNtcObjCompare &oCompare, bool &bIsExchangeLow,bool &bIsExchangeHigh) const;
        };
        
        /**
         * @brief 堆排序
         * 堆排序适合于数据量非常大的场合（百万数据）。 
         * 堆排序的最坏时间复杂度为O(nlog2n)。堆序的平均性能较接近于最坏性能。 
         * 由于建初始堆所需的比较次数较多，所以堆排序不适宜于记录数较少的文件。 
         */
        class TMdbNtcHeapSort:public TMdbNtcSort
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcHeapSort);
            /** \example  example_Algorithm.cpp
             * This is an example of how to use the TMdbNtcHeapSort class.
             * More details about this example.
             */ 
        public:
            /**
             * @brief 构造函数
             * 
             * @param bSortAsc [in] 升序与否
             */
            TMdbNtcHeapSort(bool bSortAsc = true):TMdbNtcSort(bSortAsc){}
            /**
             * @brief 按照传入的数组首地址排序
             * 
             * @param pDataHead  [in] 数组的首地址
             * @param uiDataSize [in] 数组的大小
             * @param oCompare   [in] 比较函数
             */
            virtual void Sort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare) const;
        protected:
            /**
             * @brief 调整堆
             * 详细说明：根据m_bSortAsc建堆，若其值为true，调整成大顶堆，否则调整成小顶堆
             * @param pDataHead  [in] 数组的首地址
             * @param uiStartLoc [in] 调整堆开始位置
             * @param uiMaxLoc   [in] 调整堆结束位置
             * @param oCompare   [in] 比较函数
             */
            void HeapAdjust(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiStartLoc, MDB_UINT32 uiMaxLoc, const TMdbNtcObjCompare &oCompare) const;
        };
        
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        //&&&&                                                              &&&&&&
        //&&&&                       数据结构部分                           &&&&&&
        //&&&&                                                              &&&&&&
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
                
        /**
         *   @brief  容器节点基类
         *   提供节点获取元数据的方法。
         */
        class TMdbNtcBaseNode
        {
        public:
            TMdbNtcBaseNode(TMdbNtcBaseObject * pData = NULL)
            {
                this->pData = pData;
            }
            virtual ~TMdbNtcBaseNode()
            {
            }
            /**
             * @brief 节点间的比较
             * 
             * @param pNode [in] 与之比较的节点
             * @return MDB_INT64
             */
            virtual MDB_INT64 Compare(const TMdbNtcBaseNode* pNode) const
            {
                return pData->Compare(pNode->pData);
            }
            virtual TMdbNtcStringBuffer ToString() const
            {
                return pData?pData->ToString():"null";
            }
        public:
            TMdbNtcBaseObject * pData;  ///< 数据信息
        };
        
        /**
         * @brief 容器基类定义
         */
        class TMdbNtcContainer:public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcContainer);
        public:
            //typedef TMdbNtcBaseObject** iterator;
           /**
            *   @brief  迭代器类
            *  提供容器的元素遍历的方法。
            *  使用方法举例:
            *  TBaseContainer::iterator itor = pContainer->IterBegin(), itorEnd = pContainer->IterEnd();
            *  for(; itor != itorEnd; ++itor)
            *  {
            *       itor.data();//迭代器对应的元数据TBaseObject*
            *  }
            */
            class iterator
            {
            public:
                /**
                 *	@brief	迭代器类缺省构造函数
                 *	(详细说明)
                 *	@param pDataContainer  [in],所属容器
                 *	@return 无
                 *  @retval
                 */  		  
                iterator(const TMdbNtcContainer* pDataContainer = NULL )
                {
                    this->pDataContainer = pDataContainer;
                    this->ppObject = NULL;
                    this->pParentContainer = NULL;
                    iLastStep = 0;
                }
                /**
                 *	@brief	迭代器类构造函数
                 *	(详细说明)
                 *	@param pDataContainer  [in],所属容器
                 *	@param ppObject  [in],指向的数据指针的地址
                 *	@return 无
                 *  @retval
                 */  		  
                iterator(const TMdbNtcContainer* pDataContainer, TMdbNtcBaseObject** ppObject )
                {
                    this->pDataContainer = pDataContainer;
                    this->ppObject = ppObject;
                    pParentContainer = NULL;
                    iLastStep = 0;
                }
                /**
                 *	@brief	迭代器类构造函数
                 *	(详细说明)
                 *	@param pDataContainer  [in],所属容器
                 *	@param pNodeObject  [in],指向的数据指针的地址
                 *	@return 无
                 *  @retval
                 */  		  
                iterator(const TMdbNtcContainer* pDataContainer, TMdbNtcBaseNode* pNodeObject )
                {
                    this->pDataContainer = pDataContainer;
                    this->pNodeObject = pNodeObject;
                    pParentContainer = NULL;
                    iLastStep = 0;
                }
                /**
                 *	@brief	迭代器类构造函数
                 *	(详细说明)
                 *	@param pDataContainer   [in] 所属容器
                 *	@param pParentContainer [in] 所属容器的父容器
                 *	@param pNodeObject  [in],指向的数据指针的地址
                 *	@return 无
                 *  @retval
                 */  		  
                iterator(const TMdbNtcContainer* pDataContainer, const TMdbNtcContainer* pParentContainer, TMdbNtcBaseNode* pNodeObject )
                {
                    this->pDataContainer = pDataContainer;
                    this->pNodeObject = pNodeObject;
                    this->pParentContainer = pParentContainer;
                    iLastStep = 0;
                }
                /**
                 *	@brief	迭代器类构造函数
                 *	(详细说明)
                 *	@param pDataContainer  [in],所属容器
                 *	@param itor  [in],指向的数据指针的迭代器
                 *	@return 无
                 *  @retval
                 */  		  
                iterator(const TMdbNtcContainer* pDataContainer, const iterator& itor)
                {
                    this->pDataContainer = pDataContainer;
                    this->pNodeObject = itor.pNodeObject;
                    pParentContainer = NULL;
                    iLastStep = 0;
                }
                /**
                 *  @brief  ++操作符重载
                 *  前置++
                 *  @param 无
                 *  @return iterator &
                 *  @retval  后一个迭代器
                 */           
                inline iterator& operator++()
                {
                    *this = pDataContainer->IterNext(*this);
                    if (ppObject == NULL && pParentContainer)
                    {
                        *this = pParentContainer->IterNext(*this);
                    }
                    return (*this);
                }
                /**
                 *  @brief  ++操作符重载
                 *  后置++
                 *  @param int
                 *  @return iterator 
                 *  @retval 后一个迭代器
                 */           
                inline iterator operator++(int)
                {
                    iterator tmp = *this;
                    ++*this;
                    return (tmp);
                }
                /**
                 *  @brief  --操作符重载
                 *  前置--
                 *  @param 无
                 *  @return iterator &
                 *  @retval 前一个迭代器
                 */           
                inline iterator& operator--()
                {
                    *this = pDataContainer->IterPrev(*this);
                    if (ppObject == NULL && pParentContainer)
                    {
                        *this = pParentContainer->IterPrev(*this);
                    }
                    return (*this);
                }
                /**
                 *  @brief  --操作符重载
                 *  后置--
                 *  @param 无
                 *  @return iterator &
                 *  @retval 前一个迭代器
                 */           
                inline iterator operator--(int)
                {
                    iterator tmp = *this;
                    --*this;
                    return (tmp);
                }
                /**
                 *  @brief  +操作符重载
                 *  实现迭代器的向后偏移
                 *  @param iStep [in] ,向后偏移的步长
                 *  @return iterator 
                 *  @retval 偏移后的迭代器
                 */               
                inline iterator operator+(int iStep) const
                {
                    if(iStep > 0)
                    {
                        iterator itor = pDataContainer->IterNext(*this, iStep);
                        if (itor.ppObject || pParentContainer == NULL || itor.iLastStep >= iStep) return itor;
                        else return pParentContainer->IterNext(itor,iStep - itor.iLastStep);
                    }
                    else if(iStep == 0) return *this;
                    else return operator-(-iStep);
                }
                /**
                 *  @brief  -操作符重载
                 *  实现迭代器的向前偏移
                 *  @param iStep [in] ,向前偏移的步长
                 *  @return iterator 
                 *  @retval  偏移后的迭代器
                 */               
                inline iterator operator-(int iStep) const
                {
                    if(iStep > 0)
                    {
                        iterator itor = pDataContainer->IterPrev(*this, -iStep);
                        if (itor.ppObject || pParentContainer == NULL || (-itor.iLastStep) >= iStep) return itor;
                        else return pParentContainer->IterPrev(itor, -(iStep+itor.iLastStep));
                    }
                    else if(iStep == 0) return *this;
                    else return operator+(-iStep);
                }
                /**
                 *  @brief  ==操作符重载
                 *  比较两个迭代器是否相等
                 *  @param itor [in] ,比较的迭代器
                 *  @return bool
                 *  @retval true--相等，false--不相等
                 */                  
                inline bool operator==(const iterator& itor) const
                {
                    return (ppObject == itor.ppObject);
                }
                /**
                 *  @brief  ！=操作符重载
                 *  比较两个迭代器是否不相等
                 *  @param itor [in] ,比较的迭代器
                 *  @return bool 
                 *  @retval  true--不相等，false--相等
                 */                  
                inline bool operator!=(const iterator& itor) const
                {
                    return (!(*this == itor));
                }

                inline TMdbNtcBaseObject* operator -> () const
                {
                    return data();
                }

                inline TMdbNtcBaseObject* operator -> ()
                {
                    return data();
                }

                inline iterator& assign(TMdbNtcBaseObject* pData)
                {
                    TMdbNtcBaseObject*& pPreData = data();
                    if(pPreData && 
                        ((pDataContainer && pDataContainer->IsAutoRelease()) || (pParentContainer && pParentContainer->IsAutoRelease()))
                        )
                    {
                        delete pPreData;
                        pPreData = pData;
                    }
                    else
                    {
                        pPreData = pData;
                    }
                    return *this;
                }

                inline iterator& operator = (TMdbNtcBaseObject* pData)
                {
                    return assign(pData);
                }

                inline operator const TMdbNtcBaseObject* () const
                {
                    return data();
                }

                inline operator TMdbNtcBaseObject* ()
                {
                    return data();
                }
               /**
                *   @brief  交换迭代器
                *   
                *   @param itor [in] ,交换的迭代器
                *   @return void
                *   @retval
                */              
                inline void swap(iterator itor)
                {
                     (const_cast<TMdbNtcContainer *>(pDataContainer))->IterSwap(*this, itor);
                }
                /**
                 *  @brief  获取迭代器对应的元数据
                 *  
                 *  @param 无
                 *  @return TMdbNtcBaseObject*&
                 *  @retval 元素或者节点指针的引用
                 */               
                inline TMdbNtcBaseObject*& data() 
                {
                    if(pDataContainer && ppObject) 
                    {
                        return pDataContainer->IterData(*this);
                    }
                    else
                    {
                        return TMdbNtcContainer::ms_pNullObject;
                    }
                }
                /**
                 *  @brief  获取迭代器对应的元数据
                 *  
                 *  @param 无
                 *  @return TMdbNtcBaseObject*&
                 *  @retval 元素指针的引用
                 */               
                inline TMdbNtcBaseObject* data() const
                {
                    return ppObject?*ppObject:(TMdbNtcContainer::ms_pNullObject);
                }
                /**
                 *  @brief  获取迭代器对应的结点的元数据
                 *  
                 *  @param 无
                 *  @return TNodeObject*&
                 *  @retval 节点指针的引用
                 */               
                inline TMdbNtcBaseNode*& node() 
                {
                    return pNodeObject;
                }
                /**
                 *  @brief  获取迭代器对应的结点的元数据
                 *  
                 *  @param 无
                 *  @return TNodeObject*&
                 *  @retval 节点指针
                 */               
                inline TMdbNtcBaseNode* node() const
                {
                    return pNodeObject;
                }
            public:     
                const TMdbNtcContainer*     pDataContainer; ///< 容器
                const TMdbNtcContainer*     pParentContainer;///< 父容器
                int                   iLastStep;      ///< 记录上一次迭代器前移或者后移的步数
                union
                {
                    TMdbNtcBaseObject**  ppObject;    ///< 元素地址
                    TMdbNtcBaseNode*   pNodeObject; ///< 节点地址
                };
            };
            friend class iterator;
        public:
            /**
             * @brief 构造函数
             * 
             */
            TMdbNtcContainer();
            /**
             * @brief 析构函数
             * 析构时，会调用Clear清空里面的元素或节点
             */
            virtual ~TMdbNtcContainer();
            /**
             * @brief 设置释放自动释放元素指向的内存
             * 
             * @param bAutuRelease [in] 是否自动释放元素指向的内存
             * @return 无
             */
            virtual void SetAutoRelease(bool bAutuRelease);
            /**
             * @brief 是否自动释放元素指向的内存
             * 
             * @return bool
             * @retval true 自动释放
             */
            inline bool IsAutoRelease() const
            {
                return m_bAutoRelease;
            }
            /**
             *  @brief  判断容器是否为空
             *  (详细说明)
             *  @param 无
             *  @return bool
             *  @retval  true--空，false--非空
             */
            virtual bool IsEmpty() const
            {
                return GetSize() == 0;
            }
            /**
             *  @brief  获取容器中元素数目
             *  (详细说明)
             *  @param 无
             *  @return MDB_UINT32
             *  @retval 元素数目
             */    
            virtual MDB_UINT32 GetSize() const = 0;
            /**
             * @brief 获得容器占用内存
             *      
             * @return MDB_UINT32
             * @retval 内存占用
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const = 0;
            /**
             * @brief 获得容器中的元数据占用内存的大小
             *      
             * @return MDB_UINT32
             * @retval 容器中的元数据占用内存的大小
             */
            virtual MDB_UINT32 GetDataMemoryUsage() const;
            /**
             * @brief 获得容器和元数据占用内存的总大小
             *      
             * @return MDB_UINT32
             * @retval 容器和元数据占用内存的总大小
             */
            inline MDB_UINT32 GetTotalMemoryUsage() const
            {
                return GetContainerMemoryUsage()+GetDataMemoryUsage();
            }
            /**
             *  @brief  清空容器
             *  (详细说明)
             *  @param 无
             *  @return void
             *  @retval
             */        
            virtual void Clear() = 0;
            /**
             * @brief 打印容器信息
             *  
             * @param  fp [in] 打印的方向，NULL表示直接往stdout输出,否则通过文件指针输出
             * @return 无
             */
            virtual void Print(FILE* fp = NULL) const;
            /**
             * @brief 移除某个迭代器对应的节点，返回下一个迭代器
             *  
             * @param   itor [in] 要移除的迭代器
             * @return  iterator
             * @retval  下一个迭代器
             */
            virtual iterator IterErase(iterator itor) = 0;
            /**
             * @brief 根据迭代器区间移除节点，返回下一个迭代器
             *  
             * @param   itorBegin [in] 移除区间的开始迭代器
             * @param   itorEnd   [in] 移除区间的结束迭代器
             * @return  iterator
             * @retval  下一个迭代器
             */
            virtual iterator IterErase(iterator itorBegin, iterator itorEnd);
            /**
             *  @brief  通过迭代器交换，完成元素和节点的交换
             *  (详细说明)
             *  @param itor1  [in],迭代器1
             *  @param itor2  [in],迭代器2
             *  @return void
             */            
            virtual void IterSwap(iterator itor1, iterator itor2) = 0;
            /**
             *   @brief  获得开始迭代器
             *   (详细说明)
             *   @param 无
             *   @return void
             */    
            virtual iterator IterBegin() const
            {
                return IterEnd();
            }
            /**
             *   @brief  获得终止迭代器
             *   (详细说明)
             *   @param 无
             *   @return void
             */    
            inline iterator IterEnd() const
            {
                return iterator(this);
            }
            /**
             *  @brief  获得尾元素迭代器
             *  (详细说明)
             *  @param 无
             *  @return void
             */    
            virtual iterator IterLast() const
            {
                return iterator(this);
            }
            /**
             *   @brief 根据迭代器获取对应的元数据
             *    (详细说明)
             *  @param itor  [in],迭代器
             *  @return TMdbNtcBaseObject *&
             *  @retval 元素指针的引用
             */      
            virtual TMdbNtcBaseObject*& IterData(TMdbNtcContainer::iterator itor) const
            {
                return itor.pNodeObject? itor.pNodeObject->pData:ms_pNullObject;
            }
            /**
             * @brief 根据比较查找匹配的数据迭代器
             * 
             * @param oData       [in] 比较的数据
             * @param oCompare    [in] 比较方法，如果为值为g_PointerCompare，则只进行指针比较
             * @param itLastFound [in] 上一次找到的迭代器(查找时从此迭代器下一个开始)，如果为end,则从头开始
             * @return iterator
             */
            iterator IterFind(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, iterator itLastFound = ms_itorEnd) const;
        protected:
            /**
             *  @brief  获取前一个迭代器
             *  (详细说明)
             *  @param itCur  [in],当前迭代器
             *  @param iStep  [in],步长
             *  @return iterator
             *  @retval  前一个迭代器
             */  
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const
            {
                return IterEnd();
            }
            /**
             *  @brief  获取后一个迭代器
             *  (详细说明)
             *  @param itCur  [in],当前迭代器
             *  @param iStep  [in],步长
             *  @return iterator
             *  @retval  后一个迭代器
             */  
            virtual iterator IterNext(iterator itCur, int iStep = 1) const
            {
                return IterEnd();
            }
        protected:
            bool m_bAutoRelease;                ///< 自动释放元素内存，默认false，不自动释放
            static iterator ms_itorEnd;         ///< 作为参数默认取值传递结束迭代器来使用
            static TMdbNtcBaseObject* ms_pNullObject; ///< 作为当迭代器指的的节点为NULL，又要满足不抛异常，且要返回引用
        };
        
        /**
         * @brief 从开始迭代器查找到结束迭代器，查找匹配的元数据
         * 
         * @param itorBegin [in] 开始迭代器
         * @param itorEnd   [in] 结束迭代器
         * @param oData     [in] 需要比较的对象
         * @param oCompare  [in] 比较方法
         * @return iterator
         * @retval 元数据的迭代器
         */
        TMdbNtcContainer::iterator IterFind(TMdbNtcContainer::iterator itorBegin, TMdbNtcContainer::iterator itorEnd,
            const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
        
        /**
         * @brief 动态数组
         * 
         */
        class TMdbNtcAutoArray:public TMdbNtcContainer
        {
            /** \example  example_TAutoArray.cpp
             * This is an example of how to use the TMdbNtcAutoArray class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcAutoArray);
        public:
            /**
             *  @brief  构造函数
             *  (详细说明)
             *  @param uiGrowBy [in],分配内存时增长的元素个数以免达到上限时，压入新元素频繁分配内存
             *  @return 无
             */  
            TMdbNtcAutoArray(MDB_UINT32 uiGrowBy = 0);
            /**
             *  @brief 拷贝构造函数
             *  (详细说明)
             *  @param oSrcArray [in] 源容器
             *  @return 无
             */  
            TMdbNtcAutoArray(const TMdbNtcAutoArray& oSrcArray);
            /**
             *  @brief 构造函数，从其他容器中构造
             *  (详细说明)
             *  @param pSrcContainer [in] 源容器
             *  @return 无
             */  
            TMdbNtcAutoArray(const TMdbNtcContainer* pSrcContainer);
            /**
             * @brief 赋值运算法
             * 
             *  @param oSrcArray [in] 源容器
             *  @return 无
             */
            inline TMdbNtcAutoArray& operator = (const TMdbNtcAutoArray& oSrcArray)
            {
                return *this = static_cast<const TMdbNtcContainer&>(oSrcArray);
            }
            /**
             * @brief 赋值运算法
             * 
             *  @param oSrcContainer [in] 源容器
             *  @return 无
             */
            TMdbNtcAutoArray& operator = (const TMdbNtcContainer& oSrcContainer);
            /**
             * @brief 析构函数
             * 
             */
            virtual ~TMdbNtcAutoArray();
            /**
             * @brief 得到数组大小（实际元素数目）
             * 在使用一个数组之前，使用SetSize建立它的大小和为它分配内存。
             * 如果不使用SetSize，则为数组添加元素就会引起频繁地重新分配和拷贝。
             * 频繁地重新分配和拷贝不但没有效率，而且导致内存碎片。
             * @param 无
             * @return  MDB_UINT32
             * @retval 元素数目
             */
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief 获得容器占用内存
             *      
             * @return MDB_UINT32
             * @retval 内存占用
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief 清空容器
             *（详细说明）
             * @param 无
             * @return void
             */
            virtual void Clear();
            /**
             * @brief 获得预分配的数组的大小
             *  
             * @param 无
             * @return MDB_UINT32
             * @retval  数组容量
             */
            MDB_UINT32 GetCapacity() const
            {
                return m_uiCapacity;
            }
            /**
             * @brief 获取数组首地址
             *  
             * @param 无
             * @return TMdbNtcBaseObject**
             * @retval  数组首地址
             */
            TMdbNtcBaseObject** GetData() const
            {
                return m_pData;
            }
            /**
             * @brief 获得重新分配时元素的增长数目
             *  
             * @param 无
             * @return MDB_UINT32
             * @retval 元素的增长数目
             */
            MDB_UINT32 GetGrowBy() const
            {
                return m_uiGrowBy;
            }
            /**
             * @brief 预留数组大小，但数组元素数目不变。新插入节点时，无需频繁分配内存
             *  
             * @param  uiCapacity [in] 预留的容量
             * @return void
             */
            void Reserve(MDB_UINT32 uiCapacity);
            /**
             * @brief 设置数组的大小,数组元素数目发生变化
             * 通过此方法可以改变数组大小，若数组增长，则以NULL作为新元素压入，若数组缩小，则删除释放裁剪的元素
             * 
             * @param  uiNewSize [in] 指定数组的大小,取值为非负数
             * @return void
             */
            void SetSize(MDB_UINT32 uiNewSize);
            /**
             * @brief 通过[]运算符获取元素
             * 详细说明 用于常量数组
             * @param  uiIndex [in] 数组下标
             * @return TMdbNtcBaseObject*
             * @retval  相应元素的下标
             */
            TMdbNtcBaseObject* operator[](MDB_UINT32 uiIndex) const
            {
                if(uiIndex < m_uiSize) return m_pData[uiIndex];
                else return NULL;
            }
            /**
             * @brief 通过[]运算符获取元素
             * 详细说明 用于常量数组
             * @param  iIndex [in] 数组下标
             * @return TMdbNtcBaseObject*
             * @retval 相应下标的元素
             */
            TMdbNtcBaseObject*& operator[](MDB_UINT32 uiIndex)
            {
                return m_pData[uiIndex];
            }
            /**
             * @brief 根据下标获取元素
             *  
             * @param  iIndex [in] 索要获取的元素下标
             * @return TMdbNtcBaseObject*
             * @retval 相应下标的元素
             */
            TMdbNtcBaseObject* GetAt(MDB_UINT32 uiIndex) const
            {
                if(uiIndex < m_uiSize) return m_pData[uiIndex];
                else return NULL;
            }
            /**
             * @brief 得到元素的下标
             * (详细说明)
             * @param oData     [in] 比较的数据
             * @param  oCompare [in] 比较函数
             * @param  uiStart  [in] 开始查找位置
             * @return int
             * @retval >=0--获取下标成功，-1--获取失败
             */
            int FindIndex(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, MDB_UINT32 uiStart = 0) const;
            /**
             * @brief 得到匹配的数据指针
             * (详细说明)
             * @param oData    [in] 比较的数据
             * @param oCompare [in] 比较方法
             * @param uiStart  [in] 开始查找位置
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 返回数据的指针
             */
            TMdbNtcBaseObject* FindData(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, MDB_UINT32 uiStart = 0) const
            {
                int iIndex = FindIndex(oData, oCompare, uiStart);
                return iIndex>=0?*(m_pData+iIndex):NULL;
            }
            /**
             * @brief 添加一个元素
             * (详细说明)
             * @param  pNewObj [in] 新的元素指针
             * @return int
             * @retval 新添加的元素的下标
             */
            int Add(TMdbNtcBaseObject* pNewObj);
            /**
             * @brief 追加其他容器的数据
             * 
             * @param itSrcBegin [in] 开始迭代器
             * @param itSrcEnd   [in] 结束迭代器;仅作为结束标志，即使包含有效数据也不添加到容器中，默认到末尾
             * @return int
             * @retval 新添加的元素的开始下标
             */
            int Add(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief 插入一个元素
             * (详细说明)
             * @param uiIndex [in] 指定位置的下标
             * @param pNewObj [in] 新数据元素的指针
             * @return MDB_UINT32
             * @retval 添加的元素的下标
             */
            MDB_UINT32 Insert(MDB_UINT32 uiIndex, TMdbNtcBaseObject* pNewObj);
            /**
             * @brief 追加其他容器的数据
             * 
             * @param uiIndex    [in] 指定位置的下标
             * @param itSrcBegin [in] 开始迭代器
             * @param itSrcEnd   [in] 结束迭代器;仅作为结束标志，即使包含有效数据也不添加到容器中
             * @return MDB_UINT32
             * @retval 新添加的元素的开始下标
             */
            MDB_UINT32 Insert(MDB_UINT32 uiIndex, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief 连续删除指定个数的元素
             *  
             * @param uiIndex    [in] 要删除的第一个元素下标
             * @param uiDelCount [in] 需要连续删除的元素个数
             * @return int
             * @retval  删除的元素个数
             */
            int Remove(MDB_UINT32 uiIndex, MDB_UINT32 uiDelCount = 1);
            /**
             * @brief 根据数据来移除匹配的节点
             * 详细说明 遍历，将与之匹配的节点都移除掉
             * 
             * @param  pData     [in] 要移除的数据元素指针             
             * @param  iDelCount [in] 需要删除的元素个数,-1表示删除所有匹配节点
             * @param  oCompare  [in] 比较函数
             * @return int
             * @retval  删除的元素个数
             */
            int Remove(const TMdbNtcBaseObject &oData, int iDelCount = 1, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief 合并数组, Src数组中的数据将移到本数组中，Src数组元素数目减少
             * (详细说明)
             * @param  iDestIndex    [in] 插入目标数组的位置（下标）,-1表示添加到末尾
             * @param  pSrcContainer [in] 源容器
             * @param  iSrcStart     [in] 需要插入的源数据元素的开始位置（下标），值小于0从头部开始
             * @param  iSrcCount     [in] 需要插入的源数据元素的数目,(小于)-1表示开始位置到数组的末尾
             * @return int
             * @retval 添加的元素下标
             */
            int Combine( int iDestIndex, TMdbNtcContainer* pSrcContainer, int iSrcStart = 0, int iSrcCount = -1);
            /**
             * @brief 元素排序
             * (详细说明)
             * @param  oSort    [in] 排序方法类
             * @param  oCompare [in] 比较函数
             * @return void
             */
            void Sort(const TMdbNtcSort& oSort, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief 去掉重复的元素
             *  
             * @param  oCompare [in] 比较函数
             * @return int
             * @retval  去重的元素个数
             */
            int RemoveDup(const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
        public:
            /**
             * @brief 获得开始迭代器
             * 
             * @param  无
             * @return iterator
             * @retval 开始迭代器
             */
            virtual iterator IterBegin() const;
           /**
            *  @brief   获得尾元素迭代器
            *  (详细说明)
            *  @param 无
            *  @return void
            */    
            virtual iterator IterLast() const;
           /**
             *  @brief  根据迭代器获取对应的元数据
             *  (详细说明)
             *  @param itor  [in],迭代器
             *  @return TMdbNtcBaseObject *&
             *  @retval 元数据指针的引用
             */      
            virtual TMdbNtcBaseObject*& IterData(iterator itor) const
            {
                return itor.ppObject?(*itor.ppObject):ms_pNullObject;
            }   
            /**
             * @brief 移除某个迭代器对应的节点，返回下一个迭代器
             * 
             * @param itor [in] 要移除的迭代器
             * @return iterator
             * @retval 下一个迭代器
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief 根据迭代器区间移除节点，返回下一个迭代器
             *  
             * @param   itorBegin [in] 移除区间的开始迭代器
             * @param   itorEnd   [in] 移除区间的结束迭代器
             * @return  iterator
             * @retval  下一个迭代器
             */
            virtual iterator IterErase(iterator itorBegin, iterator itorEnd);
            /**
             * @brief 通过迭代器交换，完成元素和节点的交换
             * 
             * @param itor1 [in] 迭代器1
             * @param itor2 [in] 迭代器2
             */
            virtual void IterSwap(iterator itor1, iterator itor2);
            /**
             * @brief 通过迭代器定位插入数据
             * 
             * @param itor  [in] 目标位置迭代器
             * @param pData [in] 数据
             * @return  iterator
             * @retval  指向pData的迭代器
             */
            iterator IterInsert(iterator itor,TMdbNtcBaseObject* pData);
        protected:
            /**
             * @brief 获得前一个迭代器
             * 
             * @return iterator
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief 获得后一个迭代器
             * 
             * @return iterator
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
        protected:
            TMdbNtcBaseObject**   m_pData;        ///< 数据的首地址
            MDB_UINT32    m_uiSize;       ///< 用户当前定义的数组的大小
            MDB_UINT32    m_uiCapacity;   ///< 数组的最大容量
            MDB_UINT32    m_uiGrowBy;     ///< 分配内存时增长的元素个数
        };
        
        /**
        *   @brief  栈
        *   基于动态数组实现
        *   赋予基类成员变量新含义 
        *    m_pData--------栈底地址
        *    m_uiSize-------栈顶位置,栈顶元素的下一位
        *    m_uiCapacity --栈容量
        *    m_uiGrowB------保留基类中的含义，栈的成员方法中未用到
        *  
        */
        class  TMdbNtcStack:protected TMdbNtcAutoArray
        {
            /** \example  example_TStack.cpp
             * This is an example of how to use the TMdbNtcStack class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcStack);
        public:
            /**
             * @brief 构造函数
             * 
             * @param uiStackCapacity [in] 栈容量
             * @return 无
             * @retval 
             */
            TMdbNtcStack (MDB_UINT32 uiStackCapacity);
            /**
             *  @brief 拷贝构造函数
             *  (详细说明)
             *  @param oSrcStack [in] 源容器
             *  @return 无
             */  
            TMdbNtcStack(const TMdbNtcStack& oSrcStack):TMdbNtcAutoArray(oSrcStack)
            {
            }
            /**
             *  @brief 构造函数，从其他容器中构造
             *  (详细说明)
             *  @param pSrcContainer [in] 源容器
             *  @return 无
             */  
            TMdbNtcStack(const TMdbNtcContainer* pSrcContainer):TMdbNtcAutoArray(pSrcContainer)
            {
            }
            /**
             * @brief 赋值运算法
             * 
             *  @param oSrcStack [in] 源容器
             *  @return 无
             */
            inline TMdbNtcStack& operator = (const TMdbNtcStack& oSrcStack)
            {
                *static_cast<TMdbNtcAutoArray*>(this) = static_cast<const TMdbNtcContainer&>(oSrcStack);
                return *this;
            }
            /**
             * @brief 赋值运算法
             * 
             *  @param oSrcContainer [in] 源容器
             *  @return 无
             */
            inline TMdbNtcStack& operator = (const TMdbNtcContainer& oSrcContainer)
            {
                *static_cast<TMdbNtcAutoArray*>(this) = static_cast<const TMdbNtcContainer&>(oSrcContainer);
                return *this;
            }
            /**
             * @brief 入栈
             *  将数据压入栈顶
             * @param pData [in] 压入栈顶的数据指针
             * @return int
             * @retval 栈顶位置
             */
            int Push (TMdbNtcBaseObject *pData);
            /**
             * @brief  出栈
             *  弹出栈顶元素
             * @param void 
             * @return TMdbNtcBaseObject *
             * @retval  弹出栈顶的的元素,NULL表示无元素
             */
            TMdbNtcBaseObject * Pop ();
            /**
             * @brief  获取栈顶元素
             *  
             * @param void 
             * @return TMdbNtcBaseObject *
             * @retval  获取栈顶的的元素(不弹出栈顶元素),NULL表示无元素
             */
            TMdbNtcBaseObject * Top () const;
            //下面是将TAutoArray里的一些接口权限公开出来，供外部使用
            using TMdbNtcAutoArray::SetAutoRelease;
            using TMdbNtcAutoArray::Print;
            using TMdbNtcAutoArray::IsEmpty;
            using TMdbNtcAutoArray::GetSize;
            using TMdbNtcAutoArray::GetCapacity;
            using TMdbNtcAutoArray::GetDataMemoryUsage;
            using TMdbNtcAutoArray::GetContainerMemoryUsage;
            using TMdbNtcAutoArray::GetTotalMemoryUsage;
            using TMdbNtcAutoArray::Clear;
        };
        
        
        /**
         * @brief 双向链表
         * 
         */
        class TMdbNtcBaseList:public TMdbNtcContainer
        {
            /** \example  example_TBaseList.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcBaseList);
        public:
            /**
             * @brief 双向链表的节点信息
             * 
             */
            class TNode:public TMdbNtcBaseNode
            {
            public:
                /**
                 * @brief 构造函数
                 * 
                 * @param pData [in] 数据信息
                 * @return void 
                 */
                 TNode(TMdbNtcBaseObject *pData = NULL);
                /**
                 * @brief 比较两个节点
                 * 实际比较pData,如果pData为NULL，则排在最后
                 * 
                 * @param pNode [in] 要与之比较的节点
                 * @return int
                 * @retval >0 大于 =0 等于 <0 小于
                 */
                 //virtual int Compare(const TNode *pNode) const;
            public:
                TNode*         pPrev;///< 上一结点指针
                TNode*         pNext;///< 下一结点指针
            };
            /**
             * @brief 构造函数
             * 
             * @param bAutuRelease [in] 是否自动释放元素指向的内存
             * @return 无
             */
            TMdbNtcBaseList();
            /**
             *  @brief 拷贝构造函数
             *  (详细说明)
             *  @param oSrcList [in] 源链表
             *  @return 无
             */  
            TMdbNtcBaseList(const TMdbNtcBaseList& oSrcList);
            /**
             *  @brief 构造函数，从其他容器中构造
             *  (详细说明)
             *  @param pSrcContainer [in] 源容器
             *  @return 无
             */  
            TMdbNtcBaseList(const TMdbNtcContainer* pSrcContainer);
            virtual ~TMdbNtcBaseList();
            /**
             * @brief 赋值运算法
             * 
             *  @param oSrcList [in] 源链表
             *  @return 无
             */
            inline TMdbNtcBaseList& operator = (const TMdbNtcBaseList& oSrcList)
            {
                return *this = static_cast<const TMdbNtcContainer&>(oSrcList);
            }
            /**
             * @brief 赋值运算法
             * 
             *  @param oSrcContainer [in] 源容器
             *  @return 无
             */
            TMdbNtcBaseList& operator = (const TMdbNtcContainer& oSrcContainer);
            /**
             *  @brief  判断容器是否为空
             *  (详细说明)
             *  @param 无
             *  @return bool 
             *  @retval  true表示为空，false表示非空;
             */
            virtual bool IsEmpty() const
            {
                return m_uiSize == 0 ;
            }
            /**
             * @brief 得到链表元素的数目
             *
             * @param  :无
             * @return MDB_UINT32
             * @retval 得到链表元素的个数
             */
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief 获得容器占用内存
             *      
             * @return MDB_UINT32
             * @retval 内存占用
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief 清空容器
             *
             * @param  :无
             * @return void
             */
            virtual void Clear();
            /**
             * @brief 获取头结点
             *
             * @param  :无
             * @return TNode*,得到头结点
             */
            TNode* GetHead() const
            {
                return m_pHeadNode;
            }
            /**
             * @brief 获取尾节点
             *
             * @param  :无
             * @return TNode*,得到尾结点
             */
            TNode* GetTail() const
            {
                return m_pTailNode;
            }
            /**
             * @brief 往头部添加节点
             *
             * @param  :pData [in] 需要添加的节点数据
             * @return TNode*,返回头结点
             */
            TNode* AddHead(TMdbNtcBaseObject* pData);
            /**
             * @brief 追加其他容器的数据
             * 
             * @param itSrcBegin [in] 开始迭代器
             * @param itSrcEnd   [in] 结束迭代器;仅作为结束标志，即使包含有效数据也不添加到容器中
             * @return TNode*
             * @retval 插入后节点指针
             */
            TNode* AddHead(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief 往尾部添加新节点
             * 
             * @param pNode [in] 新节点
             * @return TNode*
             * @retval 返回尾节点
             */
            TNode* AddTail(TNode* pNode);
            /**
             * @brief 往尾部添加新节点
             * 
             * @param pData [in] 需要添加的节点数据
             * @return TNode*
             * @retval 返回尾节点
             */
            inline TNode* AddTail(TMdbNtcBaseObject* pData)
            {
                return AddTail(new TNode(pData));
            }
            /**
             * @brief 追加其他容器的数据
             * 
             * @param itSrcBegin [in] 开始迭代器
             * @param itSrcEnd   [in] 结束迭代器;仅作为结束标志，即使包含有效数据也不添加到容器中，默认到末尾
             * @return TNode*
             * @retval 插入后节点指针
             */
            TNode* AddTail(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief 往指定节点前添加新节点
             * 
             * @param pNode [in] 插入位置的参考节点
             * @param pData [in] 需要添加的数据节点
             * @return TNode*
             * @retval 插入后节点指针
             */
            TNode* InsertBefore(TNode* pNode, TMdbNtcBaseObject* pData);
            /**
             * @brief 追加其他容器的数据
             * 
             * @param pNode      [in] 插入位置的参考节点
             * @param itSrcBegin [in] 开始迭代器
             * @param itSrcEnd   [in] 结束迭代器;仅作为结束标志，即使包含有效数据也不添加到容器中
             * @return TNode*
             * @retval 插入后节点指针
             */
            TNode* InsertBefore(TNode* pNode, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief 往指定节点后添加新节点
             * 
             * @param pNode [in] 插入位置的参考节点
             * @param pData [in] 需要添加的数据节点
             * @return TNode*
             * @retval 插入后节点指针
             */
            TNode* InsertAfter(TNode* pNode, TMdbNtcBaseObject* pData);
            /**
             * @brief 追加其他容器的数据
             * 
             * @param pNode      [in] 插入位置的参考节点
             * @param itSrcBegin [in] 开始迭代器
             * @param itSrcEnd   [in] 结束迭代器;仅作为结束标志，即使包含有效数据也不添加到容器中
             * @return TNode*
             * @retval 插入后节点指针
             */
            TNode* InsertAfter(TNode* pNode, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief 往指定位置添加新节点
             * 如果iIndex位置尚且没有数据，则插入到末尾
             * 
             * @param iIndex [in] 指定位置，-1表示末尾
             * @param pData  [in] 需要添加的节点数据
             * @return TNode*
             * @retval 插入后节点指针
             */
            TNode* InsertAt(MDB_UINT32 iIndex, TMdbNtcBaseObject* pData);
            /**
             * @brief 追加其他容器的数据
             * 
             * @param iIndex     [in] 指定位置，-1表示末尾
             * @param itSrcBegin [in] 开始迭代器
             * @param itSrcEnd   [in] 结束迭代器;仅作为结束标志，即使包含有效数据也不添加到容器中
             * @return TNode*
             * @retval 插入后节点指针
             */
            TNode* InsertAt(int iIndex, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief 移除头结点
             * 同时释放节点数组所指向的内存
             * 
             * @return TNode*
             * @retval 返回新的头结点，若为NULL，链表空
             */
            TNode* RemoveHead();
            /**
             * @brief 移除尾节点
             * 同时释放节点数组所指向的内存
             * 
             * @return TNode*
             * @retval 返回新的尾结点，若为NULL，链表空
             */
            TNode* RemoveTail();
            /**
             * @brief 连续删除指定个数的元素
             * 
             * @param iIndex    [in] 要删除的第一个元素下标
             * @param iDelCount [in] 需要连续删除的元素个数,-1表示删除到末尾
             * @return int
             * @retval 删除的个数
             */
            int Remove(int iIndex, int iDelCount = 1);
            /**
             * @brief 移除匹配的节点，返回下一个节点
             * 注意:如果删除的是尾节点的时候，应该返回NULL
             * @param pNode [in] 要移除的节点
             * @return int
             * @retval 返回下一个节点
             */
            TNode* Remove(TNode* pNode);
            /**
             * @brief 根据数据来移除匹配的节点
             * 遍历链表，将与之匹配的节点都移除掉
             * 
             * @param pData     [in] 要移除的数据             
             * @param iDelCount [in] 需要删除的元素个数，如果为-1，则表示删除所有匹配节点
             * @param oCompare  [in] 比较方法
             * @return int
             * @retval 删除的个数
             */
            int Remove(const TMdbNtcBaseObject &oData, int iDelCount = 1, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief 按照序列位置查找数据
             * 
             * @param  uiIndex [in] 序号位置
             * @return TMdbNtcBaseObject*
             * @retval NULL 表示没有找到，非NULL，表示找到的数据地址
             */
            TMdbNtcBaseObject* GetDataAt(MDB_UINT32 uiIndex) const
            {
                TNode* pNode = GetAt(uiIndex);
                return pNode?pNode->pData:NULL;
            }
            /**
             * @brief 按照序列位置查找节点
             * 
             * @param uiIndex [in] 序号位置
             * @return TNode*
             * @retval 查找到的节点指针
             */
            TNode* GetAt(MDB_UINT32 uiIndex) const;
            /**
             * @brief 根据数据查找节点，可用于迭代查找下一个匹配的
             * 
             * @param pData          [in] 数据指针
             * @param oCompare       [in] 比较方法
             * @param pLastFoundNode [in] 上一次查找到的节点, NULL表示从头开始查找     
             * @return TNode*
             * @retval 查找到的节点
             */
            TNode* FindNode(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, TNode* pLastFoundNode = NULL) const;
            /**
             * @brief 得到匹配的数据指针，返回第一个匹配的节点
             * (详细说明)
             * @param oData    [in] 比较的数据
             * @param oCompare [in] 比较方法
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 返回数据的指针     
             */
            TMdbNtcBaseObject* FindData(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare) const
            {
                TNode* pNode = FindNode(oData, oCompare);
                return pNode?pNode->pData:NULL;
            }
            /**
             * @brief 根据节点查找序列位置
             * 
             * @param pNode [in] 节点地址
             * @return int
             * @retval >=0 已找到，-1 没找到
             */
            int FindIndex(TNode* pNode) const;
            /**
             * @brief 根据数据查找序列位置
             * 
             * @param pData    [in] 数据指针
             * @param pCompare [in] 比较函数，如果为NULL，则只进行指针比较
             * @param uiStart  [in] 开始查找的位置,默认为0
             * @return int
             * @retval >=0 已找到，-1 没找到
             */
            int FindIndex(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, MDB_UINT32 uiStart = 0) const;
            /**
             * @brief 合并链表，Src链表中的数据将移到本链表中，Src链表节点数目减少
             * 
             * @param iDestIndex    [in] 将待合并链表插入的位置,-1表示添加到末尾
             * @param pSrcContainer [in] 待合并的容器
             * @param iSrcStart     [in] 需要插入的数据开始位置
             * @param iSrcCount     [in] 需要插入的源数据元素的数目,(小于)-1表示开始位置到链表的末尾
             * @return int
             * @retval >=0 添加的新节点的下标
             */
            int Combine(int iDestIndex, TMdbNtcContainer* pSrcContainer, int iSrcStart = 0, int iSrcCount = -1);
            /**
             * @brief 将链表的所有节点里的元数据生成数组
             * 
             * @param arrayData [out] 传出的数组
             * @return void
             */
            void GenerateArray(TMdbNtcAutoArray& arrayData) const;
            /**
             * @brief 链表节点的排序
             * 此排序比迭代器排序要快，因为不是迭代器排序是通过随机访问实现，
             * 而此排序是先将节点组织成数组的迭代器排序后，再更新链表节点顺序
             * 
             * @param oSort    [in] 排序方法类
             * @param oCompare [in] 比较方法
             */
            void Sort(const TMdbNtcSort& oSort, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief 去掉重复数据的节点
             * 
             * @param oCompare [in] 比较方法
             * @return int
             * @retval 去重的节点个数
             */
            int RemoveDup(const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief 交换两个链表
             * 
             * @param List1 [in] 链表1
             * @param List2 [in] 链表2
             * @return void
             */
            void SwapList(TMdbNtcBaseList &SrcList);
        public:
            /**
             * @brief 获得开始迭代器
             * 
             * @return iterator
             */
            virtual iterator IterBegin() const;
           /**
            *   @brief  获得尾元素迭代器
            *   (详细说明)
            *   @param 无
            *   @return void
            */    
            virtual iterator IterLast() const;
            /**
             * @brief 移除某个迭代器对应的节点，返回下一个迭代器
             * 
             * @param itor [in] 要移除的迭代器
             * @return iterator
             * @retval 下一个迭代器
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief 通过迭代器交换，完成元素和节点的交换
             * 
             * @param itor1 [in] 迭代器1
             * @param itor2 [in] 迭代器2
             * @return void
             */
            virtual void IterSwap(iterator itor1, iterator itor2) ;
            /**
             * @brief 通过迭代器定位插入数据
             * 
             * @param itor  [in] 目标位置迭代器
             * @param pData [in] 数据
             * @return  iterator
             * @retval  指向pData的迭代器
             */
            inline iterator IterInsert(iterator itor,TMdbNtcBaseObject* pData)
            {
                return iterator(this, InsertBefore(static_cast<TNode*>(itor.pNodeObject), pData));
            }
        protected:
            /**
             * @brief 获得前一个迭代器
             * 
             * @param itCur [in] 当前迭代器
             * @param iStep [in] 步长
             * @return iterator，前一个迭代器
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief 获得后一个迭代器
             * 
             * @param itCur [in] 当前迭代器
             * @param iStep [in] 步长
             * @return iterator，后一个迭代器
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
        protected:
            TNode*        m_pHeadNode;       ///< 头结点指针
            TNode*        m_pTailNode;       ///< 尾指针
            MDB_UINT32  m_uiSize;          ///< 链表节点数目
        };
        
        /**
         * @brief 平衡二叉树
         * 
         */
        class TMdbNtcAvlTree:public TMdbNtcContainer
        {
            /** \example  example_TAvlTree.cpp
             * This is an example of how to use the TMdbNtcAvlTree class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcAvlTree);
        private:
            /**
             *  @brief 禁止拷贝构造函数(因为比较函数无法复制)
             */  
            TMdbNtcAvlTree(const TMdbNtcAvlTree& oSrcTree)
            {
            }
        public:
            /**
             * @brief 平衡二叉树树的节点
             * 
             */
            class TNode:public TMdbNtcBaseNode
            {
            public:
                /**
                 * @brief 构造函数
                 * 
                 * @param pData [in] 数据信息
                 * @return 无
                 */
                TNode(TMdbNtcBaseObject *pData = NULL);
                /**
                 * @brief 交换节点，针对二叉树删除节点时
                 * 
                 * @param pSwapNode [in] 要交换的节点
                 * @return bool
                 * @retval true 成功
                 */
                virtual void SwapNode(TNode* pSwapNode);
            public:
                int      iBalanceFactor; ///平衡因子，是节点的右子树的高度减去左子树的高度的高度差 
                TNode    *pLeftSubNode;  ///左子节点 
                TNode    *pRightSubNode; ///右子节点
                TNode    *pParentNode;   ///父节点
            };
        public:
            /**
             * @brief 构造函数
             * 
             * @param pObjCompare [in] 元数据的比较函数，需要传入new出的对象，
             * 如果为NULL，则会使用TAvlTree::CompareNode，默认会使用数据本身的Compare方法
             * @return 无
             */
            TMdbNtcAvlTree( TMdbNtcObjCompare *pCompare = NULL);
            /**
             * @brief 赋值运算法
             * 
             *  @param oSrcArray [in] 源容器
             *  @return 无
             */
            inline TMdbNtcAvlTree& operator = (const TMdbNtcAvlTree& oSrcTree)
            {
                return *this = static_cast<const TMdbNtcContainer&>(oSrcTree);
            }
            /**
             * @brief 赋值运算法
             * 
             *  @param oSrcContainer [in] 源容器
             *  @return 无
             */
            TMdbNtcAvlTree& operator = (const TMdbNtcContainer& oSrcContainer);
            /**
             * @brief 析构函数
             * 
             */
            virtual ~TMdbNtcAvlTree();
            /**
             * @brief 打印容器信息
             *  
             * @param  fp [in] 打印的方向，NULL表示直接往stdout输出,否则通过文件指针输出
             * @return 无
             */
            virtual void Print(FILE* fp = NULL) const;
            /**
             * @brief 得到容器中元素的数目
             * 
             * @param void
             * @return MDB_UINT32 元素的数目
             */
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief 获得容器占用内存
             *      
             * @return MDB_UINT32
             * @retval 内存占用
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief 清空容器
             * 
             * @param  void
             * @return void
             */
            virtual void Clear();
            /**
             * @brief 添加节点, 如果相应的位置上已经存在节点，则用新的替代
             * 
             * @param pData [in] 数据信息
             * @return iterator
             * @retval 新添加的树节点迭代器
             */
            iterator Add(TMdbNtcBaseObject* pData);
            /**
             * @brief 追加其他容器的数据
             * 
             * @param itSrcBegin [in] 开始迭代器
             * @param itSrcEnd   [in] 结束迭代器;仅作为结束标志，即使包含有效数据也不添加到容器中，默认到末尾
             * @return iterator
             * @retval 新添加的树节点迭代器
             */
            iterator Add(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief 根据数据查找匹配的节点，并删除节点
             * 
             * @param pData     [in] 数据信息
             * @param iDelCount [in] 需要连续删除的元素个数,-1表示删除所有匹配的
             * @return int
             * @retval 删除的个数
             */
            inline int Remove(const TMdbNtcBaseObject &oData, int iDelCount = 1)
            {                
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                return Remove(oNode, iDelCount);
            }
            /**
             * @brief 得到匹配的数据指针，返回第一个匹配的节点
             * (详细说明)
             * @param oData    [in] 比较的数据
             * @param oCompare [in] 比较方法
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 返回数据的指针     
             */
            inline TMdbNtcBaseObject* FindData(const TMdbNtcBaseObject &oData) const
            {
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                iterator itor = IterFind(oNode);
                return itor.pNodeObject?itor.pNodeObject->pData:NULL;
            }
        public:
            /**
             * @brief 获得开始迭代器
             *  迭代器中的数据是节点地址的地址
             * @param  无
             * @return iterator
             */
            virtual iterator IterBegin() const;
            /**
             *  @brief  获得尾元素迭代器
             *  (详细说明)
             *  @param 无
             *  @return void
             */
            virtual iterator IterLast() const;
            /**
             * @brief 根据数据信息，查找数据的迭代器
             * 
             * @param oData       [in] 数据信息
             * @param itLastFound [in] 上一次找到的迭代器(查找时从此迭代器下一个开始)，如果为end,则从头开始
             * @return iterator
             */
            inline iterator IterFind(const TMdbNtcBaseObject &oData, iterator itLastFound = ms_itorEnd) const
            {
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                return IterFind(oNode, itLastFound);
            }
            /**
             * @brief 根据数据信息，返回一个迭代器，指向不小于pData的第一个元素
             * 
             * @param oData [in] 数据信息
             * @return iterator
             */
            inline iterator LowerBound(const TMdbNtcBaseObject &oData) const
            {
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                return LowerBound(oNode);
            }
            /**
             * @brief 根据数据信息，返回一个迭代器，指向大于pData的第一个元素
             * 
             * @param oData [in] 数据信息
             * @return iterator
             */
            inline iterator UpperBound(const TMdbNtcBaseObject &oData) const
            {
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                return UpperBound(oNode);
            }
            /**
             * @brief 根据数据信息，返回一个迭代器，指向小于pData的第一个元素
             * 
             * @param oData [in] 数据信息
             * @return iterator
             */
            inline iterator LessBound(const TMdbNtcBaseObject &oData) const
            {
                return IterPrev(LowerBound(oData));
            }
            /**
             * @brief 得到与此数据相等的开始和结束迭代器
             * 
             * @param oData [in] 数据信息
             * @return std::pair<iterator, iterator>
             * @retval 通过pair的first和second得到lowerbound和upperbound
             */
            inline std::pair<iterator, iterator> EqualRange(const TMdbNtcBaseObject &oData) const
            {
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                return EqualRange(oNode);
            }
            /**
             * @brief 移除某个迭代器对应的节点，返回下一个迭代器
             * 
             * @param itor [in] 要移除的迭代器
             * @return iterator
             * @retval 下一个迭代器
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief 通过迭代器交换，完成元素和节点的交换
             * 
             * @param itor1 [in] 迭代器1
             * @param itor2 [in] 迭代器2
             * @return void
             */
            virtual void IterSwap(iterator itor1, iterator itor2) ;
        protected:
            /**
             * @brief 根据查找匹配的节点，并删除节点
             * 
             * @param oNode       [in] 节点信息
             * @param iDelCount [in] 需要连续删除的元素个数,-1表示删除所有匹配的
             * @return int
             * @retval 删除的个数
             */
            int Remove(const TNode &oNode, int iDelCount = 1);
            /**
             * @brief 根据节点信息，查找迭代器
             * 
             * @param oNode       [in] 节点信息
             * @param itLastFound [in] 上一次找到的迭代器(查找时从此迭代器下一个开始)，如果为end,则从头开始
             * @return iterator
             */
            iterator IterFind(const TNode& oNode, iterator itLastFound = ms_itorEnd) const;
            /**
             * @brief 根据节点信息，返回一个迭代器，指向不小于oNode的第一个元素
             * 
             * @param oNode       [in] 节点信息
             * @return iterator
             */
            iterator LowerBound(const TNode& oNode) const;
            /**
             * @brief 根据节点信息，返回一个迭代器，指向大于oNode的第一个元素
             * 
             * @param oNode       [in] 节点信息
             * @return iterator
             */
            iterator UpperBound(const TNode& oNode) const;
            /**
             * @brief 得到与此节点相等的开始和结束迭代器
             * 
             * @param oNode [in] 节点信息
             * @return std::pair<iterator, iterator>
             * @retval 通过pair的first和second得到lowerbound和upperbound
             */
            std::pair<iterator, iterator> EqualRange(const TNode& oNode) const;
            /**
             * @brief 获得前一个迭代器
             * 
             * @param  itCur [in] 当前迭代器
             * @param  iStep [in] 步长
             * @return iterator，前一个迭代器
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief 获得后一个迭代器
             * 
             * @param  itCur [in] 当前迭代器
             * @param  iStep [in] 步长
             * @return iterator，后一个迭代器
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
            /**
             * @brief 删除树节点
             * 
             * @param pAvlNode [in] 要删除的节点
             * @return iterator
             * @retval  后继节点的迭代器
             */
            iterator Remove(TNode* pAvlNode);
            /**
             * @brief 后序遍历清除平衡二叉树
             * 
             * @param pParentNode [in] 根节点
             * @return void
             * @retval 
             */
            void  ClearAvlTree(TNode * pParentNode);
            /**
             * @brief 将数据插入到子树的合适位置
             * 
             * @param ppParentNode   [in] 子树的父节点地址
             * @param pNewNode       [in] 新数据节点
             * @return TNode*
             * @retval 树节点指针
             */
            TNode* InsertNode(TNode** ppParentNode, TNode* pNewNode);
           /**
             * @brief 平衡树节点
             * 
             * @param pParentNode   [in] 子树的父节点指针
             * @param iBalance      [in]  插入新节点前父节点的平衡度
             * @return TNode*
             * @retval 平衡后的父节点
             */
            TNode* BalanceTree(TNode* pParentNode, int iBalance );
            /**
             * @brief 对节点进行左旋
             * 
             * @param pNode [in] 需要左旋的父节点
             * @return TNode*
             * @retval 平衡后的父节点
             */
            TNode* LeftRotate(TNode* pNode);
            /**
             * @brief 对节点进行右旋
             * 
             * @param pNode [in] 需要右旋的父节点
             * @return TNode*
             * @retval 平衡后的父节点
             */
            TNode* RightRotate(TNode* pNode);
            /**
             * @brief 根据父节点得到最左子节点
             * 
             * @param pParentNode [in] 父节点
             * @return TNode*
             * @retval 父节点的最左子节点
             */
            TNode* GetLeftMostNode(TNode* pParentNode) const;
            /**
             * @brief 根据父节点得到最右子节点
             * 
             * @param pParentNode [in] 父节点
             * @return TNode*
             * @retval 父节点的最右子节点
             */
            TNode* GetRightMostNode(TNode* pParentNode) const;
        protected:
            int          m_iBalanceFactor;   ///< 平衡因子,当节点平衡因子为 [-m_iBalanceFactor, m_iBalanceFactor] 的时候是平衡的
            TNode*       m_pRootNode;        ///< 平衡树的根节点
            int          m_iAvlFlag;         ///< 高度增减标志
            int          m_iAvlHeight;       ///< 高度平衡树的高度  
            MDB_UINT32 m_uiSize;           ///< 元素的数目
            TMdbNtcObjCompare* m_pObjCompare;      ///< 元数据的比较函数
        };

        /**
         * @brief 以整数为键值的map
         * 
         */
        class TMdbNtcIntMap:public TMdbNtcAvlTree
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcIntMap);
        protected:
            /**
             * @brief 节点类型
             * 
             */
            class TNode:public TMdbNtcAvlTree::TNode
            {
            public:
                
                TNode(MDB_INT64 iKeyParam, TMdbNtcBaseObject* pData = NULL)
                    :TMdbNtcAvlTree::TNode(pData),iKey(iKeyParam)
                {
                }
                /**
                 * @brief 节点间的比较
                 * 
                 * @param pNode [in] 与之比较的节点
                 * @return MDB_INT64
                 */
                virtual MDB_INT64 Compare(const TMdbNtcBaseNode* pNode) const
                {
                    return iKey-static_cast<const TNode*>(pNode)->iKey;
                }
                virtual void SwapNode(TMdbNtcAvlTree::TNode* pSwapNode);
                virtual TMdbNtcStringBuffer ToString() const
                {
                    TMdbNtcStringBuffer sRet;
                    sRet<<"key="<<iKey<<" value="<<pData->ToString();
                    return sRet;
                }
            public:
                MDB_INT64 iKey;
            };
        public:
            /**
             * @brief map的迭代器
             * 
             */
            class iterator:public TMdbNtcContainer::iterator
            {
            public:
                iterator(const TMdbNtcContainer::iterator& itor)
                {
                    memcpy(this, &itor, sizeof(TMdbNtcContainer::iterator));
                }
                TMdbNtcContainer::iterator base()
                {
                    return *reinterpret_cast<TMdbNtcContainer::iterator*>(this);
                }
                /**
                 * @brief 提供first函数,方便得到key
                 * 
                 * @return MDB_INT64
                 * @retval key
                 */
                MDB_INT64 first()
                {
                    if(pNodeObject == NULL || pNodeObject->pData == NULL) return 0;
                    else return static_cast<TNode*>(pNodeObject)->iKey;
                }
                /**
                 * @brief 取到key对应的值
                 * 
                 * @return TMdbNtcBaseObject
                 * @retval 取到key对应的值
                 */
                TMdbNtcBaseObject* second()
                {
                    if(pNodeObject == NULL || pNodeObject->pData == NULL) return NULL;
                    else return static_cast<TNode*>(pNodeObject)->pData;
                }
                inline iterator& operator = (TMdbNtcBaseObject* pData)
                {
                    assign(pData);
                    return *this;
                }
                inline iterator& operator = (TMdbNtcContainer::iterator& itor)
                {
                    memcpy(this, &itor, sizeof(TMdbNtcContainer::iterator));
                    return *this;
                }
            };
        public:
            /**
             * @brief 添加
             * 
             * @param iKey [in] key
             * @param pData [in] 数据指针
             * @return iterator
             * @retval 得到数据对应的迭代器
             */
            iterator Add(MDB_INT64 iKey, TMdbNtcBaseObject* pData);
            /**
             * @brief 根据key来查找数据，并移除key
             * 
             * @param iKey     [in] key
             * @param iDelCount [in] 需要连续删除的元素个数,-1表示删除所有匹配的
             * @return int
             * @retval 删除的个数
             */
            inline int Remove(MDB_INT64 iKey, int iDelCount = 1)
            {                
                TNode oNode(iKey);
                return TMdbNtcAvlTree::Remove(oNode, iDelCount);
            }
            /**
             * @brief 得到匹配的数据指针，返回第一个匹配的节点
             * 
             * @param iKey    [in] 比较的key
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 返回数据的指针   
             */
            inline TMdbNtcBaseObject* FindData(MDB_INT64 iKey) const
            {
                TNode oNode(iKey);
                iterator itor = TMdbNtcAvlTree::IterFind(oNode);
                return itor.pNodeObject?itor.pNodeObject->pData:NULL;
            }
        public:            
            /**
             * @brief 根据数据信息，查找数据的迭代器
             * 
             * @param iKey       [in] key
             * @param itLastFound [in] 上一次找到的迭代器(查找时从此迭代器下一个开始)，如果为end,则从头开始
             * @return iterator
             */
            inline iterator IterFind(MDB_INT64 iKey, TMdbNtcContainer::iterator itLastFound = ms_itorEnd) const
            {
                TNode oNode(iKey);
                return TMdbNtcAvlTree::IterFind(oNode, itLastFound);
            }
            /**
             * @brief 通过[]运算符获取元素
             *
             * @param iKey    [in] 比较的key
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 返回数据的指针
             */
            inline iterator operator[](MDB_INT64 iKey) const
            {
                return IterFind(iKey);
            }
            /**
             * @brief 通过[]运算符获取元素
             *
             * @param iKey    [in] 比较的key
             * @return iterator
             */
            inline iterator operator[](MDB_INT64 iKey)
            {
                iterator itor = IterFind(iKey);
                if(itor.pNodeObject) return itor;
                else return Add(iKey, NULL);
            }
            /**
             * @brief 根据数据信息，返回一个迭代器，指向不小于pData的第一个元素
             * 
             * @param iKey [in] key
             * @return iterator
             */
            inline iterator LowerBound(MDB_INT64 iKey) const
            {
                TNode oNode(iKey);
                return TMdbNtcAvlTree::LowerBound(oNode);
            }
            /**
             * @brief 根据数据信息，返回一个迭代器，指向大于pData的第一个元素
             * 
             * @param iKey [in] key
             * @return iterator
             */
            inline iterator UpperBound(MDB_INT64 iKey) const
            {
                TNode oNode(iKey);
                return TMdbNtcAvlTree::UpperBound(oNode);
            }
            /**
             * @brief 根据数据信息，返回一个迭代器，指向小于pData的第一个元素
             * 
             * @param oData [in] 数据信息
             * @return iterator
             */
            inline iterator LessBound(MDB_INT64 iKey) const
            {
                return IterPrev(LowerBound(iKey));
            }
            /**
             * @brief 得到与此数据相等的开始和结束迭代器
             * 
             * @param iKey [in] key
             * @return std::pair<iterator, iterator>
             * @retval 通过pair的first和second得到lowerbound和upperbound
             */
            inline std::pair<iterator, iterator> EqualRange(MDB_INT64 iKey) const
            {
                TNode oNode(iKey);
                std::pair<TMdbNtcContainer::iterator, TMdbNtcContainer::iterator> stRet = TMdbNtcAvlTree::EqualRange(oNode);
                return std::pair<iterator, iterator>(stRet.first, stRet.second);
            }
        };

        /**
         * @brief 以string为键值的map
         * 
         */
        class TMdbNtcStrMap:public TMdbNtcAvlTree
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcStrMap);
        protected:
            /**
             * @brief 节点类型
             * 
             */
            class TNode:public TMdbNtcAvlTree::TNode
            {
            public:
                TNode(TMdbNtcStringBuffer sKeyParam, TMdbNtcBaseObject* pData = NULL)
                    :TMdbNtcAvlTree::TNode(pData),sKey(sKeyParam)
                {
                }                
                /**
                 * @brief 节点间的比较
                 * 
                 * @param pNode [in] 与之比较的节点
                 * @return MDB_INT64
                 */
                virtual MDB_INT64 Compare(const TMdbNtcBaseNode* pNode) const
                {
                    return sKey.Compare(static_cast<const TNode*>(pNode)->sKey);
                }
                virtual void SwapNode(TMdbNtcAvlTree::TNode* pSwapNode);
                virtual TMdbNtcStringBuffer ToString() const
                {
                    TMdbNtcStringBuffer sRet;
                    sRet<<"key="<<sKey<<" value="<<pData->ToString();
                    return sRet;
                }
            public:
                TMdbNtcStringBuffer sKey;
            };
        public:
            /**
             * @brief map的迭代器
             * 
             */
            class iterator:public TMdbNtcContainer::iterator
            {
            public:
                iterator(const TMdbNtcContainer::iterator& itor)
                {
                    memcpy(this, &itor, sizeof(TMdbNtcContainer::iterator));
                }
                TMdbNtcContainer::iterator base()
                {
                    return *reinterpret_cast<TMdbNtcContainer::iterator*>(this);
                }
                /**
                 * @brief 提供first函数,方便得到key
                 * 
                 * @return TMdbNtcStringBuffer
                 * @retval key
                 */
                TMdbNtcStringBuffer first()
                {
                    if(pNodeObject == NULL || pNodeObject->pData == NULL) return 0;
                    else return static_cast<TNode*>(pNodeObject)->sKey;
                }
                /**
                 * @brief 取到key对应的值
                 * 
                 * @return TMdbNtcBaseObject
                 * @retval 取到key对应的值
                 */
                TMdbNtcBaseObject* second()
                {
                    if(pNodeObject == NULL || pNodeObject->pData == NULL) return NULL;
                    else return static_cast<TNode*>(pNodeObject)->pData;
                }
                inline iterator& operator = (TMdbNtcBaseObject* pData)
                {
                    assign(pData);
                    return *this;
                }
                inline iterator& operator = (TMdbNtcContainer::iterator& itor)
                {
                    memcpy(this, &itor, sizeof(TMdbNtcContainer::iterator));
                    return *this;
                }
            };
        public:
            /**
             * @brief 构造函数
             * 
             * @param bCaseSensitive [in] 是否区分大小写，默认区分大小写
             * @return bool
             * @retval true 成功
             */
            TMdbNtcStrMap(bool bCaseSensitive = true);
            /**
             * @brief 添加
             * 
             * @param sKey [in] key
             * @param pData [in] 数据指针
             * @return iterator
             * @retval 得到数据对应的迭代器
             */
            iterator Add(TMdbNtcStringBuffer sKey, TMdbNtcBaseObject* pData);
            /**
             * @brief 根据key来查找数据，并移除key
             * 
             * @param sKey [in] key
             * @param iDelCount [in] 需要连续删除的元素个数,-1表示删除所有匹配的
             * @return int
             * @retval 删除的个数
             */
            inline int Remove(TMdbNtcStringBuffer sKey, int iDelCount = 1)
            {
                if(!m_bCaseSensitive) sKey.ToLower();
                TNode oNode(sKey);
                return TMdbNtcAvlTree::Remove(oNode, iDelCount);
            }
            /**
             * @brief 得到匹配的数据指针，返回第一个匹配的节点
             * (详细说明)
             * @param sKey    [in] 比较的key
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 返回数据的指针     
             */
            inline TMdbNtcBaseObject* FindData(TMdbNtcStringBuffer sKey) const
            {
                return IterFind(sKey).data();
            }
        public:
            /**
             * @brief 根据数据信息，查找数据的迭代器
             * 
             * @param sKey       [in] key
             * @param itLastFound [in] 上一次找到的迭代器(查找时从此迭代器下一个开始)，如果为end,则从头开始
             * @return iterator
             */
            inline iterator IterFind(TMdbNtcStringBuffer sKey, TMdbNtcContainer::iterator itLastFound = ms_itorEnd) const
            {
                if(!m_bCaseSensitive) sKey.ToLower();
                TNode oNode(sKey);                
                return TMdbNtcAvlTree::IterFind(oNode, itLastFound);
            }
            /**
             * @brief 通过[]运算符获取元素
             *
             * @param sKey    [in] 比较的key
             * @return iterator
             */
            inline iterator operator[](TMdbNtcStringBuffer sKey) const
            {
                return IterFind(sKey);
            }
            /**
             * @brief 通过[]运算符获取元素
             *
             * @param sKey    [in] 比较的key
             * @return iterator
             */
            inline iterator operator[](TMdbNtcStringBuffer sKey)
            {
                iterator itor = IterFind(sKey);
                if(itor.pNodeObject) return itor;
                else return Add(sKey, NULL);
            }
            /**
             * @brief 根据数据信息，返回一个迭代器，指向不小于pData的第一个元素
             * 
             * @param sKey [in] key
             * @return iterator
             */
            inline iterator LowerBound(TMdbNtcStringBuffer sKey) const
            {
                if(!m_bCaseSensitive) sKey.ToLower();
                TNode oNode(sKey);
                return TMdbNtcAvlTree::LowerBound(oNode);
            }
            /**
             * @brief 根据数据信息，返回一个迭代器，指向大于pData的第一个元素
             * 
             * @param sKey [in] key
             * @return iterator
             */
            inline iterator UpperBound(TMdbNtcStringBuffer sKey) const
            {
                if(!m_bCaseSensitive) sKey.ToLower();
                TNode oNode(sKey);
                return TMdbNtcAvlTree::UpperBound(oNode);
            }
            /**
             * @brief 根据数据信息，返回一个迭代器，指向小于pData的第一个元素
             * 
             * @param oData [in] 数据信息
             * @return iterator
             */
            inline iterator LessBound(TMdbNtcStringBuffer sKey) const
            {
                return IterPrev(LowerBound(sKey));
            }
            /**
             * @brief 得到与此数据相等的开始和结束迭代器
             * 
             * @param sKey [in] key
             * @return std::pair<iterator, iterator>
             * @retval 通过pair的first和second得到lowerbound和upperbound
             */
            inline std::pair<iterator, iterator> EqualRange(TMdbNtcStringBuffer sKey) const
            {
                if(!m_bCaseSensitive) sKey.ToLower();
                TNode oNode(sKey);
                std::pair<TMdbNtcContainer::iterator, TMdbNtcContainer::iterator> stRet = TMdbNtcAvlTree::EqualRange(oNode);
                return std::pair<iterator, iterator>(stRet.first, stRet.second);
            }
        protected:
            bool m_bCaseSensitive;
        };
        
        /**
         * @brief HASH链表类
         * 基于TBaseObject::ToHash得到hash值构建
         */
        class TMdbNtcHashList:public TMdbNtcContainer
        {
            /** \example  example_THashList.cpp
             * This is an example of how to use the TMdbNtcHashList class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcHashList);
        public:
            TMdbNtcHashList();
            virtual ~TMdbNtcHashList();
            /**
             * @brief 打印容器信息
             *  
             * @param  fp [in] 打印的方向，NULL表示直接往stdout输出,否则通过文件指针输出
             * @return 无
             */
            virtual void Print(FILE* fp = NULL) const;
            /**
             * @brief 初始化hash表
             * 
             * @param uiTableNum [in] hash数组大小
             * @return MDB_UINT32 实际数组大小
             */
            MDB_UINT32 InitHashTable(MDB_UINT32 uiTableNum);
            /**
             * @brief 得到元素数目
             * 
             * @return MDB_UINT32
             */
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief 获得容器占用内存
             *      
             * @return MDB_UINT32
             * @retval 内存占用
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief 清空容器
             * 
             */
            virtual void Clear();
            /**
             * @brief 添加元素
             * 
             * @param pData    [in] 元素数据信息
             * @return iterator
             * @retval 节点的迭代器
             */
            iterator Add(TMdbNtcBaseObject* pData);
            /**
             * @brief 根据hash key和数据查找匹配的元素，返回第一个匹配的数据
             * 
             * @param oData    [in] 参考比较的数据信息
             * @param oCompare [in] 比较方法
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 找到相应的元素
             */
            inline TMdbNtcBaseObject* FindData(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare) const
            {
                return IterFind(oData, g_oMdbNtcObjectCompare).data();
            }
            /**
             * @brief 根据hash key和数据删除匹配的元素
             * 
             * @param pData     [in] 参考比较的数据信息             
             * @param iDelCount [in] 需要连续删除的元素个数,-1表示删除所有匹配的
             * @param oCompare  [in] 比较方法
             * @return int
             * @retval 删除的个数
            */
            int Remove(const TMdbNtcBaseObject &oData, int iDelCount = 1, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
        public:
            /**
             * @brief 获得开始迭代器
             * 容器指向list,数据指向data
             * 
             * @return iterator
             */
            virtual iterator IterBegin() const;
            /**
             *  @brief  获得尾元素迭代器
             *  (详细说明)
             *  @param 无
             *  @return iterator
             */    
            virtual iterator IterLast() const;            
            /**
             * @brief 根据字符串键，查找数据的迭代器
             * 
             * @param oData       [in] 参考比较的数据信息     
             * @param oCompare    [in] 比较方法
             * @param itLastFound [in] 上一次找到的迭代器(查找时从此迭代器下一个开始)，如果为end,则从头开始
             * @return iterator
             */
            iterator IterFind(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, iterator itLastFound = ms_itorEnd) const;
            /**
             * @brief 移除某个迭代器对应的节点，返回下一个迭代器
             * 
             * @param itor [in] 要移除的迭代器
             * @return iterator
             * @retval 下一个迭代器
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief 通过迭代器交换，完成元素和节点的交换
             * 
             * @param itor1 [in] 迭代器1
             * @param itor2 [in] 迭代器2
             */
            virtual void IterSwap(iterator itor1, iterator itor2) ;
        protected:
            /**
             * @brief 根据hash key得到key对应的hash链表
             * 
             * @param uiHashKey [in] hash key
             * @return TMdbNtcBaseList*
             * @retval 
             */
            inline TMdbNtcBaseList* FindList(MDB_UINT64 uiHashKey) const
            {
                return m_pHashList?&m_pHashList[uiHashKey%m_uiTableNum]:NULL;
            }
            /**
             * @brief 获得前一个迭代器
             * 
             * @return iterator
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief 获得后一个迭代器
             * 
             * @return iterator
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
        protected:
            MDB_UINT32      m_uiTableNum;   ///< hash数组大小
            MDB_UINT32      m_uiSize;       ///< hash元素数目
            TMdbNtcBaseList*  m_pHashList;    ///< 用于存放hash数据的吊桶链表    
        };

        /**
         * @brief IntHash类
         * 
         */
        class TMdbNtcIntHash:public TMdbNtcHashList
        {
            /** \example  example_TStrHashList.cpp
             * This is an example of how to use the TMdbNtcHashList class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcIntHash);
        protected:
            /**
             * @brief 字符串hashlist中节点的data
             * 
             */
            class TNode:public TMdbNtcBaseList::TNode
            {
            public:
                /**
                 * @brief 构造函数
                 * 
                 * @param iKeyParam  [in] key
                 * @param pDataParam [in] 数据信息
                 * @return 无
                 */
                TNode(MDB_INT64 iKeyParam, TMdbNtcBaseObject *pDataParam = NULL)
                    :TMdbNtcBaseList::TNode(pDataParam),iKey(iKeyParam)
                {
                }
                virtual TMdbNtcStringBuffer ToString() const
                {
                    TMdbNtcStringBuffer sRet;
                    sRet<<"key="<<iKey<<" value="<<pData->ToString();
                    return sRet;
                }
             public:
                 MDB_INT64 iKey;
            };
        public:
            /**
             * @brief int hash的迭代器
             * 
             */
            class iterator:public TMdbNtcContainer::iterator
            {
            public:
                iterator(const TMdbNtcContainer::iterator& itor)
                {
                    memcpy(this, &itor, sizeof(TMdbNtcContainer::iterator));
                }
                TMdbNtcContainer::iterator base()
                {
                    return *reinterpret_cast<TMdbNtcContainer::iterator*>(this);
                }
                /**
                 * @brief 提供first函数,方便得到key
                 * 
                 * @return MDB_INT64
                 * @retval iKey
                 */
                MDB_INT64 first()
                {
                    if(pNodeObject == NULL || pNodeObject->pData == NULL) return 0;
                    else return static_cast<TNode*>(pNodeObject)->iKey;
                }
                /**
                 * @brief 取到key对应的值
                 * 
                 * @return TMdbNtcBaseObject
                 * @retval 取到key对应的值
                 */
                TMdbNtcBaseObject* second()
                {
                    if(pNodeObject == NULL || pNodeObject->pData == NULL) return NULL;
                    else return static_cast<TNode*>(pNodeObject)->pData;
                }
                inline iterator& operator = (TMdbNtcBaseObject* pData)
                {
                    assign(pData);
                    return *this;
                }
                inline iterator& operator = (TMdbNtcContainer::iterator& itor)
                {
                    memcpy(this, &itor, sizeof(TMdbNtcContainer::iterator));
                    return *this;
                }
            };
        public:
            /**
             * @brief 添加元素
             * 
             * @param iKey  [in] hash key
             * @param pData [in] 元素数据信息
             * @return iterator
             * @retval 节点的迭代器
             */
            iterator Add(MDB_INT64 iKey, TMdbNtcBaseObject* pData);
            /**
             * @brief 根据hash key和数据查找匹配的元素，返回第一个匹配的数据
             * 
             * @param iKey  [in] hash key
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 找到相应的元素
             */
            inline TMdbNtcBaseObject* FindData(MDB_INT64 iKey) const
            {
                return IterFind(iKey).data();
            }
            /**
             * @brief 根据hash key删除元素
             * 
             * @param iKey  [in] hash key
             * @param iDelCount [in] 需要连续删除的元素个数,-1表示删除所有匹配的
             * @return int
             * @retval 删除的个数
            */
            int Remove(MDB_INT64 iKey, int iDelCount = 1);
        public:
            /**
             * @brief 根据字符串键，查找数据的迭代器
             * 
             * @param iKey        [in] hash key
             * @param itLastFound [in] 上一次找到的迭代器(查找时从此迭代器下一个开始)，如果为end,则从头开始
             * @return iterator
             */
            iterator IterFind(MDB_INT64 iKey, iterator itLastFound = ms_itorEnd) const;
            /**
             * @brief 通过[]运算符获取元素
             *
             * @param iKey    [in] 比较的key
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 返回数据的指针
             */
            inline iterator operator[](MDB_INT64 iKey) const
            {
                return IterFind(iKey);
            }
            /**
             * @brief 通过[]运算符获取元素
             *
             * @param iKey    [in] 比较的key
             * @return iterator
             */
            inline iterator operator[](MDB_INT64 iKey)
            {
                iterator itor = IterFind(iKey);
                if(itor.pNodeObject) return itor;
                else return Add(iKey, NULL);
            }
        };

        /**
         * @brief StrHash类
         * 
         */
        class TMdbNtcStrHash:public TMdbNtcHashList
        {
            /** \example  example_TStrHashList.cpp
             * This is an example of how to use the TMdbNtcHashList class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcStrHash);
        protected:
            /**
             * @brief 字符串hashlist中节点的data
             * 
             */
            class TNode:public TMdbNtcBaseList::TNode
            {
            public:
                /**
                 * @brief 构造函数
                 * 
                 * @param sKeyParam  [in] key
                 * @param pDataParam [in] 数据信息
                 * @return 无
                 */
                TNode(TMdbNtcStringBuffer sKeyParam, TMdbNtcBaseObject *pDataParam = NULL)
                    :TMdbNtcBaseList::TNode(pDataParam),sKey(sKeyParam)
                {
                }
                virtual TMdbNtcStringBuffer ToString() const
                {
                    return "key="+sKey+" value="+pData->ToString();
                }
             public:
                 TMdbNtcStringBuffer  sKey;
            };
        public:
            /**
             * @brief int hash的迭代器
             * 
             */
            class iterator:public TMdbNtcContainer::iterator
            {
            public:
                iterator(const TMdbNtcContainer::iterator& itor)
                {
                    memcpy(this, &itor, sizeof(TMdbNtcContainer::iterator));
                }
                TMdbNtcContainer::iterator base()
                {
                    return *reinterpret_cast<TMdbNtcContainer::iterator*>(this);
                }
                /**
                 * @brief 提供first函数,方便得到key
                 * 
                 * @return TMdbNtcStringBuffer
                 */
                TMdbNtcStringBuffer first()
                {
                    if(pNodeObject == NULL || pNodeObject->pData == NULL) return 0;
                    else return static_cast<TNode*>(pNodeObject)->sKey;
                }
                /**
                 * @brief 取到key对应的值
                 * 
                 * @return TMdbNtcBaseObject
                 * @retval 取到key对应的值
                 */
                TMdbNtcBaseObject* second()
                {
                    if(pNodeObject == NULL || pNodeObject->pData == NULL) return NULL;
                    else return static_cast<TNode*>(pNodeObject)->pData;
                }
                inline iterator& operator = (TMdbNtcBaseObject* pData)
                {
                    assign(pData);
                    return *this;
                }
                inline iterator& operator = (TMdbNtcContainer::iterator& itor)
                {
                    memcpy(this, &itor, sizeof(TMdbNtcContainer::iterator));
                    return *this;
                }
            };
        public:
            /**
             * @brief 构造函数
             * 
             * @param bCaseSensitive [in] 是否区分大小写，默认区分大小写
             * @param pHashFunc [in] 字符串所使用的hash函数，如果为NULL，则默认使用函数HashFunc
             * @return 无
             */
            TMdbNtcStrHash(bool bCaseSensitive = true, mdb_ntc_hash_func pHashFunc = NULL);
            /**
             * @brief 添加元素
             * 
             * @param sKey [in] hash key
             * @param pData [in] 元素数据信息
             * @return iterator
             * @retval 节点的迭代器
             */
            iterator Add(TMdbNtcStringBuffer sKey, TMdbNtcBaseObject* pData);
            /**
             * @brief 根据hash key和数据查找匹配的元素，返回第一个匹配的数据
             * 
             * @param sKey     [in] hash key
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 找到相应的元素
             */
            inline TMdbNtcBaseObject* FindData(TMdbNtcStringBuffer sKey) const
            {
                return IterFind(sKey).data();
            }
            /**
             * @brief 根据hash key删除元素
             * 
             * @param sKey [in] hash key
             * @param iDelCount [in] 需要连续删除的元素个数,-1表示删除所有匹配的
             * @return int
             * @retval 删除的个数
            */
            int Remove(TMdbNtcStringBuffer sKey, int iDelCount = 1);
        public:
            /**
             * @brief 根据字符串键，查找数据的迭代器
             * 
             * @param sKey        [in] hash key
             * @param itLastFound [in] 上一次找到的迭代器(查找时从此迭代器下一个开始)，如果为end,则从头开始
             * @return iterator
             */
            iterator IterFind(TMdbNtcStringBuffer sKey, iterator itLastFound = ms_itorEnd) const;
            /**
             * @brief 通过[]运算符获取元素
             *
             * @param sKey    [in] 比较的key
             * @return iterator
             */
            inline iterator operator[](TMdbNtcStringBuffer sKey) const
            {
                return IterFind(sKey);
            }
            /**
             * @brief 通过[]运算符获取元素
             *
             * @param sKey    [in] 比较的key
             * @return iterator
             */
            inline iterator operator[](TMdbNtcStringBuffer sKey)
            {
                iterator itor = IterFind(sKey);
                if(itor.pNodeObject) return itor;
                else return Add(sKey, NULL);
            }
        protected:
            mdb_ntc_hash_func   m_pHashFunc;///< 字符串hash函数指针
            bool        m_bCaseSensitive;///< 是否区分大小写
        };

        /**
         * @brief 键树(hash表变种)
         * 功能相当于std::map<std::string, TMdbNtcBaseObject*>或者std::set<std::string>
         * 键树是用于对于字符串作为键索引的情况，效率比常规map要高
         * 
         */
        class TMdbNtcKeyTree:public TMdbNtcContainer
        {
           /** \example  example_TKeyTree.cpp
             * This is an example of how to use the TMdbNtcAutoArray class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcKeyTree);
        public:
            /**
             * @brief 键树节点信息
             * 
             */
            class TNode:public TMdbNtcBaseNode
            {
            public: 
                TNode**    ppSubNode;     ///< 记录子节点的键
                TNode*&    pParentNode;   ///< 父节点,使用引用类型方便快速定位父节点在其兄弟节点中的序号,向上回溯时，
            public:
                /**
                 * @brief 构造函数
                 * @param pParentNodeRef [in] 父节点的指针引用
                 * @param pData          [in] 数据信息  
                 * @return void
                 * @retval
                 */
                TNode(TNode*& pParentNodeRef, TMdbNtcBaseObject* pDataNode = NULL );
                /**
                 * @brief 析构函数
                 * @param  [in] 
                 * @return void
                 * @retval
                 */
                ~TNode()
                {
                    if(ppSubNode != NULL)
                    {
                        delete [] ppSubNode;
                    }
                }
            };
            /**
             * @brief 构造函数
             * 
             * @param iCharacterIndex [in] 键树字符对应的子节点索引
             */
            TMdbNtcKeyTree(const int iCharacterIndex[256]);
            /**
             * @brief 析构函数
             * 
             */
            virtual ~TMdbNtcKeyTree();
            /**
             * @brief 得到元素数目
             * 
             * @return MDB_UINT32
             */
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief 获得容器占用内存
             *      
             * @return MDB_UINT32
             * @retval 内存占用
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief 清空容器
             * 
             */
            virtual void Clear();
            /**
             * @brief 增加一个键值,如果已经存在键值，则将之前的数据删除,再用新的替代
             * 
             * @param pszKeyName [in] 字符串key
             * @param pData      [in] 数据
             * @return iterator
             * @retval 数据节点的迭代器
             */
            virtual iterator Add(const char* pszKeyName, TMdbNtcBaseObject* pData);
            /**
             * @brief 根据字符串键，删除数据
             * 
             * @param pszKeyName [in] 字符串key
             * @return int
             * @retval 删除的个数
             */
            virtual int Remove(const char* pszKeyName);
            /**
             * @brief 根据字符串键，查找数据
             * 
             * @param pszKeyName [in] 字符串key
             * @return TMdbNtcBaseObject*
             */
            TMdbNtcBaseObject* Find(const char* pszKeyName);
            /**
             * @brief 根据前缀获取匹配的元素
             * 
             * @param pszPrefix [in] 关键字前缀
             * @param oDataList [out] 数据存放链表
             */
            void MatchPrefix(const char* pszPrefix, TMdbNtcBaseList& oDataList);
        public:
            /**
             * @brief 获得开始迭代器
             * 
             * 
             * @return iterator
             */
            virtual iterator IterBegin() const;
            /**
             *  @brief  获得尾元素迭代器
             *  (详细说明)
             *  @param 无
             *  @return iterator
             */    
            virtual iterator IterLast() const;
            /**
             * @brief 根据字符串键，查找数据的迭代器
             * 
             * @param pszKeyName [in] 字符串key
             * @return iterator
             */
            iterator IterFind(const char* pszKeyName) const;
            /**
             * @brief 通过[]运算符获取元素
             *
             * @param pszKeyName    [in] 比较的key
             * @return iterator
             */
            inline iterator operator[](const char* pszKeyName) const
            {
                return IterFind(pszKeyName);
            }
            /**
             * @brief 通过[]运算符获取元素
             *
             * @param pszKeyName    [in] 比较的key
             * @return iterator
             */
            inline iterator operator[](const char* pszKeyName)
            {
                iterator itor = IterFind(pszKeyName);
                if(itor.pNodeObject) return itor;
                else return Add(pszKeyName, NULL);
            }
            /**
             * @brief 根据字符串键，返回一个迭代器，指向不小于键值的第一个迭代器
             * 
             * @param pszKeyName [in] 字符串key
             * @return iterator
             */
            iterator LowerBound(const char* pszKeyName) const;
            /**
             * @brief 根据字符串键，返回一个迭代器，指向大于键值的第一个迭代器
             * 
             * @param pszKeyName [in] 字符串key
             * @return iterator
             */
            iterator UpperBound(const char* pszKeyName) const;
            /**
             * @brief 根据数据信息，返回一个迭代器，指向小于pData的第一个元素
             * 
             * @param oData [in] 数据信息
             * @return iterator
             */
            inline iterator LessBound(const char* pszKeyName) const
            {
                return IterPrev(LowerBound(pszKeyName));
            }
            /**
             * @brief 根据前缀获取匹配的元素
             * 
             * @param pszPrefix [in] 关键字前缀
             * @return std::pair<iterator, iterator>
             * @retval lower_bounder和upper_bounder迭代器
             */
            std::pair<iterator, iterator> MatchPrefix(const char* pszPrefix) const;
            /**
             * @brief 根据字符串键，查找最深匹配
             * 
             * @param pszKeyName [in] 字符串key
             * @return iterator
             * @retval 得到最深匹配的迭代器，如果为end，则没有找到
             */
            iterator MatchDeep(const char* pszKeyName) const;
            /**
             * @brief 移除某个迭代器对应的节点，返回下一个迭代器
             * 
             * @param itor [in] 要移除的迭代器
             * @return iterator
             * @retval 下一个迭代器
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief 通过迭代器交换，完成元素和节点的交换
             * 
             * @param itor1 [in] 迭代器1
             * @param itor2 [in] 迭代器2
             */
            virtual void IterSwap(iterator itor1, iterator itor2);
        protected:
            /**
             * @brief 删除指定节点
             * 
             * @param pCurNode [in] 当前节点
             * @return void
             * @retval 无
             */
            virtual void Remove(TMdbNtcKeyTree::TNode* pCurNode);
            /**
             * @brief 后序遍历清除键树，用于销毁整个树
             * @param pParentNode [in]  根节点指针
             * @param pParentNode [out] 根节点指针
             * 
             */
            void DeleteKeyTree(TNode * &pParentNode);
            /**
             * @brief 获得前一个迭代器
             * 
             * @return iterator
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief 获得后一个迭代器
             * 
             * @return iterator
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
            /**
              * @brief 查找本节点在父节点的所有子节点中的位置
              * 
              * @param pCurNode [in] 当前节点
              * @return int
              * @retval 本节点在父节点的所有子节点中的位置
              */
            int FindIndex(TNode* pCurNode) const;
            /**
              * @brief 反向查找本节点在父节点的所有子节点中的位置
              * 
              * @param pCurNode [in] 当前节点
              * @return int
              * @retval 反向本节点在父节点的所有子节点中的位置
              */
            int ReverseFindIndex(TNode* pCurNode) const;
            /**
             * @brief 查找左边非空兄弟节点的序号
             * 
             * @param ppCurNode [in] 当前节点的地址
             * @return int
             * @retval 非空兄弟节点的序号
             */
            int FindSiblingPrevIndex(TNode** ppCurNode) const;
            /**
             * @brief 查找右边非空兄弟节点的序号
             * 
             * @param ppCurNode [in] 当前节点的地址
             * @return int
             * @retval 非空兄弟节点的序号
             */
            int FindSiblingNextIndex(TNode** ppCurNode) const;
            /**
             * @brief 查找右边非空兄弟节点或者向上回溯的父节点
             * 
             * @param ppCurNode [in] 当前节点
             * @return int
             * @retval 右边非空兄弟节点或者向上回溯的父节点
             */
            TNode* FindSiblingOrParentNext(TNode** ppCurNode) const;
            /**
             * @brief 删除指定节点
             * 
             * @param ppCurNode [in] 当前节点
             * @return void
             * @retval 无
             */
            void Remove(TMdbNtcKeyTree::TNode** ppCurNode);
            /**
             * @brief 根据key查找对应的节点地址
             * 
             * @param pszKeyName [in] key
             * @return TNode*
             * @retval key对应的节点地址
             */
            TNode** FindNode(const char* pszKeyName) const;
            /**
             * @brief 获取子节点占用内存大小
             * 
             * @param pParentNode [in] 父节点,如果为NULL，则表示计算所有的节点数目(包含root节点)
             * @return MDB_UINT32
             * @retval 子节点占用内存大小
             */
            virtual MDB_UINT32 GetChildNodeMemoryUsage(TNode* pParentNode = NULL) const;
            /**
             * @brief 获取子节点个数
             * 
             * @param pParentNode [in] 父节点,如果为NULL，则表示计算所有的节点数目(包含root节点)
             * @return MDB_UINT32
             * @retval 子节点个数
             */
            virtual MDB_UINT32 GetChildNodeCount(TNode* pParentNode = NULL) const;
        protected:
            TNode*       m_pRootNode;            ///< 键树根节点
            MDB_UINT32 m_uiTNodeCount;         ///< 最大孩子数目
            MDB_UINT32 m_uiSize;               ///< 数据节点个数
            int    m_iCharacterIndex[256];       ///< 键值过滤表
            static TNode* m_s_pNullNode;         ///< 用于做引用传递时赋值
        };
        
        /**
         * @brief 多重键树，也就是一个键可以多个值对应
         * 功能相当于std::multimap<std::string, TMdbNtcBaseObject*>或者std::multiset<std::string>
         * 节点对应数据是有序的, 这样通过迭代器完成对多个值的遍历
         * 
         */
        class TMdbNtcMultiKeyTree:public TMdbNtcKeyTree
        {
            /** \example  example_TMultiKeyTree.cpp
             * This is an example of how to use the TMdbNtcAutoArray class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcMultiKeyTree);
        public:
            /**
             * @brief 多重键树的节点
             * 
             */
            class TNode:public TMdbNtcKeyTree::TNode
            {
            public:
                 /**
                 * @brief 构造函数
                 * @param pParentNodeRef [in] 父节点的指针引用
                 * @param pData [in] 数据信息  
                 * @return void
                 * @retval 
                 */
                TNode(TMdbNtcKeyTree::TNode*& pParentNodeRef, TMdbNtcBaseObject* pData  = NULL )
                    :TMdbNtcKeyTree::TNode(pParentNodeRef, pData)
                {          
                    pNext = NULL;
                    pPrev = NULL;
                }
                ~TNode()
                {
                    if(pPrev || pNext)
                    {
                        ppSubNode = NULL;
                        if(pPrev) pPrev->pNext = pNext;
                        if(pNext) pNext->pPrev = pPrev;
                    }
                }
            public:
                TNode*        pNext;            /// <指向后继数据节点
                TNode*        pPrev;            /// <指向前驱数据节点
            };
            /**
             * @brief 构造函数
             * 
             * @param iCharacterIndex [in] 键树字符对应的子节点索引
             */
            TMdbNtcMultiKeyTree(const int iCharacterIndex[256]);
            /**
             * @brief 析构函数
             * 
             * @param [in]
             * @return int
             * @retval 0 成功
             */
            virtual ~TMdbNtcMultiKeyTree();
            /**
             * @brief 清空容器
             * 
             */
            virtual void Clear();
            /**
             * @brief 获得容器占用内存
             *      
             * @return MDB_UINT32
             * @retval 内存占用
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief 增加一个键值
             * 
             * @param pszKeyName [in] 字符串key
             * @param pData      [in] 数据
             * @return iterator
             * @retval 数据节点的迭代器
             */
            virtual iterator Add(const char* pszKeyName, TMdbNtcBaseObject* pData);    
            /**
             * @brief 根据字符串键，删除数据
             * 
             * @param pszKeyName [in] 字符串key
             * @return int
             * @retval 删除的个数
             */
            virtual int Remove(const char* pszKeyName);
            /**
             *  @brief  获得尾元素迭代器
             *  (详细说明)
             *  @param 无
             *  @return iterator
             */    
            virtual iterator IterLast() const;
            /**
             * @brief 移除某个迭代器对应的节点，返回下一个迭代器
             * 
             * @param itor [in] 要移除的迭代器
             * @return iterator
             * @retval 下一个迭代器
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief 根据字符串键，查找键值对应的开始
             * 
             * @param pszKeyName [in] 字符串key
             * @return std::pair<iterator, iterator>
             * @retval 通过pair的first和second得到lowerbound和upperbound
             */
            std::pair<iterator, iterator> EqualRange(const char* pszKeyName) const;
        protected:
            /**
             * @brief 删除指定节点
             * 
             * @param pCurNode [in] 当前节点
             * @return void
             * @retval 无
             */
            virtual void Remove(TMdbNtcKeyTree::TNode* pCurNode);
            /**
             * @brief 后序遍历清除键树
             * 
             * @param pParentNode [in]  根节点指针
             * @param pParentNode [out] 根节点指针
             */
            void DeleteKeyTree(TNode * &pParentNode);    
            /**
             * @brief 获得前一个迭代器
             * 
             * @return iterator
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief 获得后一个迭代器
             * 
             * @return iterator
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
            /**
             * @brief 检查同一key的数据数目
             * 
             * @param pNode [in] 处于此key位置的首节点
             * @return int
             * @retval 数据的数目
             */
            int GetKeyValueCount(TNode* pNode) const;
            /**
             * @brief 检查当前节点是同一key的重复节点中的第几个
             * 
             * @param pNode [in] 当前节点
             * @param pHeadNode [in] 重复节点的头节点
             * @return int
             * @retval 当前节点是同一key的重复节点中的第几个
             */
            int GetNodeIndex(TNode* pNode, TNode* pHeadNode = NULL) const;
            /**
             * @brief 获取结点pNode所属链表的头结点在父结点的ppsubnode数组中的相应位置的元素地址
             * 
             * @param pNode [in] 当前节点
             * @return TNode**
             * @retval 二级指针，表示地址
             */
            TNode** GetHeadAddr(TNode* pNode) const;
            /**
             * @brief 获取子节点占用内存大小
             * 
             * @param pParentNode [in] 父节点,如果为NULL，则表示计算所有的节点数目(包含root节点)
             * @return MDB_UINT32
             * @retval 子节点占用内存大小
             */
            virtual MDB_UINT32 GetChildNodeMemoryUsage(TMdbNtcKeyTree::TNode* pParentNode = NULL) const;
            /**
             * @brief 获取子节点个数
             * 
             * @param pParentNode [in] 父节点,如果为NULL，则表示计算所有的节点数目(包含root节点)
             * @return MDB_UINT32
             * @retval 子节点个数
             */
            virtual MDB_UINT32 GetChildNodeCount(TMdbNtcKeyTree::TNode* pParentNode = NULL) const;
        };
        
        extern const int g_iMdbNtcKeyTreeDigitIndex[256];///< 提供常规的小写字母键树，大写字母键树，数字键树的字符对应的子节点索引
        /**
         * @brief 数字键树
         * 字符串都是数字
         */
        class TMdbNtcDigitKeyTree:public TMdbNtcKeyTree
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcDigitKeyTree);
        public:
            TMdbNtcDigitKeyTree():TMdbNtcKeyTree(g_iMdbNtcKeyTreeDigitIndex)
            {
            }
        };
        
        /**
         * @brief 多重数字键树
         * 
         */
        class TMdbNtcMultiDigitKeyTree:public TMdbNtcMultiKeyTree
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcMultiDigitKeyTree);
        public:
            TMdbNtcMultiDigitKeyTree():TMdbNtcMultiKeyTree(g_iMdbNtcKeyTreeDigitIndex)
            {
            }
        };
        class TMdbNtcThreadLock;
        class TMdbNtcThreadCond;
        /**
         * @brief 线程安全队列
         * 
         *
        class TMdbNtcQueue:public TMdbNtcContainer
        {
            / \example  example_TQueue.cpp
             * This is an example of how to use the TMdbNtcAutoArray class.
             * More details about this example.
             *
            MDB_ZF_DECLARE_OBJECT(TMdbNtcQueue);
        private:
            *
             * @brief 禁止拷贝构造函数
             * 
             * @param oSrcQueue [in] 源队列
             *
            TMdbNtcQueue(const TMdbNtcQueue& oSrcQueue)
            {
            }
            TMdbNtcQueue& operator = (const TMdbNtcQueue& oSrcQueue)
            {
                return *this;
            }
        public:
            **
             * @brief 多重键树的节点
             * 
             *
            class TNode:public TMdbNtcBaseNode
            {
            public:
                 *
                 * @brief 构造函数
                 * @param pData [in] 数据信息  
                 * @return void
                 * @retval 
                 *
                TNode(TMdbNtcBaseObject* pData  = NULL ):TMdbNtcBaseNode(pData)
                {          
                    pNext = NULL;
                }
            public:
                TNode*        pNext;            ///< 指向后继数据节点
            };
             *
             * @brief 构造函数
             * 
             * @param bPushLock [in] 压入时加锁（多个线程写入队列时）
             * @param bPopLock  [in]  弹出时加锁（多个线程读取队列时）
             * 
             * @return 无
             *
            TMdbNtcQueue(bool bPushLock = false, bool bPopLock = false);
            **
             * @brief 析构函数
             * 
             *
            virtual ~TMdbNtcQueue();
            **
             * @brief 向队列尾部压入数据
             * 
             * @param pData [in] 要压入的数据
             * @return bool
             * @retval true 成功，false 失败
             *
            bool Push(TMdbNtcBaseObject* pData);
            **
             * @brief 向队列尾部压入数据
             * 
             * @param itor [in] 要压入的开始位置迭代器
             * @param itor_end [in] 要压入的结束位置迭代器，默认到结尾
             *
            void Push(TMdbNtcContainer::iterator itor_begin, TMdbNtcContainer::iterator itor_end = ms_itorEnd);
            **
             * @brief 从队列头部取数据
             *      
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 取成功
             *
            TMdbNtcBaseObject* Pop();
            **
             * @brief 获得头结点信息，但是并不取出
             * 
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 取成功
             *
            inline TMdbNtcBaseObject* Front()
            {
                if(m_pHeadNode == m_pTailNode) return NULL;
                else return m_pHeadNode->pNext->pData;
            }
            **
             *  @brief  判断容器是否为空
             *  (详细说明)
             *  @param 无
             *  @return 类型--bool  ;含义说明--true表示为空，false表示非空;
             *
            virtual bool IsEmpty() const
            {
                return m_pHeadNode == m_pTailNode;
            }
            **
             *  @brief  获取容器中元素数目
             *  (详细说明)
             *  @param 无
             *  @return 类型--MDB_UINT32 ;含义说明--元素数目;
             *  
            virtual MDB_UINT32 GetSize() const;
            **
             * @brief 向队列中压入的消息数
             * 
             * @return MDB_UINT32
             * @retval 压入的消息数
             *
            inline MDB_UINT32 GetPushTimes() const
            {
                return m_uiPushTimes;
            }

            **
             * @brief 从队列中取出的消息数
             * 
             * @return MDB_UINT32
             * @retval 取出的消息数
             *
            inline MDB_UINT32 GetPopTimes() const
            {
                return m_uiPopTimes;
            }
            **
             * @brief 获得容器中的元数据占用内存的大小
             *      
             * @return MDB_UINT32
             * @retval 容器中的元数据占用内存的大小
             *
            virtual MDB_UINT32 GetDataMemoryUsage() const;
            **
             * @brief 获得容器占用内存
             *      
             * @return MDB_UINT32
             * @retval 内存占用
             *
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            **
             * @brief 获得容器和元数据占用内存的总大小
             *      
             * @return MDB_UINT32
             * @retval 容器和元数据占用内存的总大小
             *
            using TMdbNtcContainer::GetTotalMemoryUsage;
            **
             *  @brief  清空容器
             *  (详细说明)
             *  @param 无
             *  @return void
             *      
            virtual void Clear();
            **
             * @brief 打印容器信息
             *  
             * @param fp [in] 打印的方向，NULL表示直接往stdout输出,否则通过文件指针输出
             * @return 无
             *
            virtual void Print(FILE* fp = NULL) const;
        protected:
            **
             * @brief 获得开始迭代器
             * 
             * 
             * @return iterator
             *
            virtual iterator IterBegin() const;
            **
             *   @brief  获得尾元素迭代器
             *   (详细说明)
             *   @param 无
             *   @return void
             *  
            virtual iterator IterLast() const;
            **
             * @brief 移除某个迭代器对应的节点，返回下一个迭代器
             * 
             * @param itor [in] 要移除的迭代器
             * @return iterator
             * @retval 下一个迭代器
             *
            virtual iterator IterErase(iterator itor);
            **
             * @brief 通过迭代器交换，完成元素和节点的交换
             * 
             * @param itor1 [in] 迭代器1
             * @param itor2 [in] 迭代器2
             *
            virtual void IterSwap(iterator itor1, iterator itor2) ;  
        protected:
            **
             * @brief 获得前一个迭代器
             * 
             * @return iterator
             *
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            **
             * @brief 获得后一个迭代器
             * 
             * @return iterator
             *
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
        protected:
            TMdbNtcThreadLock  *m_pHeadMutex;   ///< 头锁
            TMdbNtcThreadLock  *m_pTailMutex;   ///< 尾锁              
            TNode        *m_pHeadNode;    ///< 头结点
            TNode        *m_pTailNode;    ///< 尾节点
            MDB_UINT32  m_uiPushTimes;  ///< 记录压入数目
            MDB_UINT32  m_uiPopTimes;   ///< 记录弹出次数，压入数目减去弹出数目就是当前size，这样避免getsize遍历
        };

        class TThreadEvent;
        **
         * @brief 阻塞队列
         * 
         *
        class TBlockingQueue:public TMdbNtcQueue
        {
            MDB_ZF_DECLARE_OBJECT(TBlockingQueue);
        private:
            **
             * @brief 禁止拷贝构造函数
             * 
             * @param oSrcQueue [in] 源队列
             *
            TBlockingQueue(const TBlockingQueue& oSrcQueue)
            {
            }
            TBlockingQueue& operator = (const TBlockingQueue& oSrcQueue)
            {
                return *this;
            }
        public:
            **
             * @brief 构造函数
             * 
             * @param bPushLock [in] 压入时加锁（多个线程写入队列时）
             * @param bPopLock  [in]  弹出时加锁（多个线程读取队列时）
             * 
             * @return 无
             *
            TBlockingQueue(bool bPushLock = false, bool bPopLock = false);
            ~TBlockingQueue();
            **
             * @brief 判断当前队列是否仍然有效
             * 
             * @return bool
             * @retval true 有效
             *
            bool IsOK();
            **
             * @brief 向队列尾部压入数据
             * 
             * @param pData         [in] 要压入的数据
             * @param iMilliSeconds [in] 等待超时时间单位毫秒
             * @return bool
             * @retval true 成功，false 失败
             *
            bool Push(TMdbNtcBaseObject* pData, int iMilliSeconds  = -1);
            **
             * @brief 向队列尾部压入数据
             * 
             * @param itor [in] 要压入的开始位置迭代器
             * @param itor_end [in] 要压入的结束位置迭代器，默认到结尾
             *
            void Push(TMdbNtcContainer::iterator itor_begin, TMdbNtcContainer::iterator itor_end = ms_itorEnd);
            **
             * @brief 从队列头部取数据
             * 
             * @param iMilliSeconds [in] 等待超时时间单位毫秒
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 取成功
             *
            TMdbNtcBaseObject* Pop(int iMilliSeconds = -1);
            **
             * @brief 唤醒阻塞的队列
             * 
             *
            void Wakeup();
        protected:
            TThreadEvent* m_pPopEvent;   ///通知队列中有元素，可以pop
            TThreadEvent* m_pPushEvent;  ///通知队列中有空间，可以push
        };
        */

        /**
         * @brief 线程安全队列
         * 
         */
        class TMdbNtcQueue:public TMdbNtcContainer
        {
            /** \example  example_TQueue.cpp
             * This is an example of how to use the TMdbNtcAutoArray class.
             * More details about this example.
             */
            MDB_ZF_DECLARE_OBJECT(TMdbNtcQueue);
        private:
            /**
             * @brief 禁止拷贝构造函数
             * 
             * @param oSrcQueue [in] 源队列
             */
            TMdbNtcQueue(const TMdbNtcQueue& oSrcQueue)
            {
            }
            TMdbNtcQueue& operator = (const TMdbNtcQueue& oSrcQueue)
            {
                return *this;
            }
        public:
            class TNode:public TMdbNtcBaseNode
            {
            public:
                 /**
                 * @brief 构造函数
                 * @param pData [in] 数据信息  
                 * @return void
                 * @retval 
                 */
                TNode(TMdbNtcBaseObject* pData  = NULL ):TMdbNtcBaseNode(pData)
                {          
                    pNext = NULL;
                }
            public:
                TNode*        pNext;            ///< 指向后继数据节点
            };
            /**
             * @brief 构造函数
             * 
             * @param void
             * 
             * @return 无
             */
            TMdbNtcQueue();
            ~TMdbNtcQueue();
        public:
            /**
             * @brief 判断当前队列是否仍然有效
             * 
             * @return bool
             * @retval true 有效
             */
            bool IsOK();
            /**
             * @brief 获得头结点信息，但是并不取出
             * 
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 取成功
             */
            inline TMdbNtcBaseObject* Front()
            {
                if(m_pHeadNode == m_pTailNode) return NULL;
                else return m_pHeadNode->pNext->pData;
            }
            /**
             * @brief 向队列中压入的消息数
             * 
             * @return MDB_UINT32
             * @retval 压入的消息数
             */
            inline MDB_UINT64 GetPushTimes() const
            {
                return m_uiPushTimes;
            }

            /**
             * @brief 从队列中取出的消息数
             * 
             * @return MDB_UINT32
             * @retval 取出的消息数
             */
            inline MDB_UINT64 GetPopTimes() const
            {
                return m_uiPopTimes;
            }
            /**
             * @brief 向队列尾部压入数据
             * 
             * @param pData         [in] 要压入的数据
             * @param iMilliSeconds [in] 等待超时时间单位毫秒
             * @return bool
             * @retval true 成功，false 失败
             */
            bool Push(TMdbNtcBaseObject* pData, int iMilliSeconds  = -1);
            /**
             * @brief 从队列头部取数据
             * 
             * @param iMilliSeconds [in] 等待超时时间单位毫秒
             * @return TMdbNtcBaseObject*
             * @retval 非NULL 取成功
             */
            TMdbNtcBaseObject* Pop(int iMilliSeconds = -1);
            /**
             * @brief 唤醒阻塞的队列
             * 
             */
            void Wakeup();
        public:
            /**
             *  @brief  判断容器是否为空
             *  (详细说明)
             *  @param 无
             *  @return 类型--bool  ;含义说明--true表示为空，false表示非空;
             */
            virtual bool IsEmpty() const
            {
                return m_uiSize == 0;
            }
            /**
             *  @brief  获取容器中元素数目
             *  (详细说明)
             *  @param 无
             *  @return 类型--MDB_UINT32 ;含义说明--元素数目;
             */    
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief 获得容器中的元数据占用内存的大小
             *      
             * @return MDB_UINT32
             * @retval 容器中的元数据占用内存的大小
             */
            virtual MDB_UINT32 GetDataMemoryUsage() const;
            /**
             * @brief 获得容器占用内存
             *      
             * @return MDB_UINT32
             * @retval 内存占用
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief 获得容器和元数据占用内存的总大小
             *      
             * @return MDB_UINT32
             * @retval 容器和元数据占用内存的总大小
             */
            using TMdbNtcContainer::GetTotalMemoryUsage;
            /**
             *  @brief  清空容器
             *  (详细说明)
             *  @param 无
             *  @return void
             */        
            virtual void Clear();
            /**
             * @brief 打印容器信息
             *  
             * @param fp [in] 打印的方向，NULL表示直接往stdout输出,否则通过文件指针输出
             * @return 无
             */
            virtual void Print(FILE* fp = NULL) const;
        protected:
            /**
             * @brief 获得开始迭代器
             * 
             * 
             * @return iterator
             */
            virtual iterator IterBegin() const;
            /**
             *   @brief  获得尾元素迭代器
             *   (详细说明)
             *   @param 无
             *   @return void
             */    
            virtual iterator IterLast() const;
            /**
             * @brief 移除某个迭代器对应的节点，返回下一个迭代器
             * 
             * @param itor [in] 要移除的迭代器
             * @return iterator
             * @retval 下一个迭代器
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief 通过迭代器交换，完成元素和节点的交换
             * 
             * @param itor1 [in] 迭代器1
             * @param itor2 [in] 迭代器2
             */
            virtual void IterSwap(iterator itor1, iterator itor2) ;
        protected:
            /**
             * @brief 获得前一个迭代器
             * 
             * @return iterator
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief 获得后一个迭代器
             * 
             * @return iterator
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
        protected:
            TNode           *m_pHeadNode;   ///< 头结点
            TNode           *m_pTailNode;   ///< 尾节点   
            MDB_UINT32           m_uiSize;      ///< 数据个数 
            MDB_UINT64           m_uiPushTimes; ///< push次数
            MDB_UINT64           m_uiPopTimes;  ///< push次数
            TMdbNtcThreadLock      *m_pMutex;     ///< 线程锁 
            TMdbNtcThreadCond      *m_pPopCond;   ///通知队列中有元素，可以pop
            TMdbNtcThreadCond      *m_pPushCond;  ///通知队列中有空间，可以push
            
        };
//}

#endif










