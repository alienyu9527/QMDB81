/********************************************************
*   Copyright (c) 2003-2015 ZTESoft Technology Co.,Ltd.
*
*   All rights rescrved.
*
*   @file    Sdk/mdbDataStructs.h
*   @brief   �㷨�Լ����ݽṹ����
*   @version 1.0
*   @author  Jiang.jinzhou ,zhang.he
*   @date    Design 2012/03/21,Programming2012/07/05
*   @bug     (�½�����bug)
*   @warning (���ù��ܣ�ͳһʹ��)
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
        //&&&&                           �㷨����                           &&&&&&
        //&&&&                                                              &&&&&&
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        
        /**
         * @brief �ȽϺ�������Ҫ�Զ���Ƚ�ʱ����,����Compare����
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
         * @brief ָ��ȽϺ�������Ҫ�Զ���Ƚ�ʱ����,����Compare����
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

        extern TMdbNtcObjCompare     g_oMdbNtcObjectCompare;///< ����ȽϷ���
        extern TMdbNtcPointerCompare g_oMdbNtcPointerCompare;///< ָ��ȽϷ���

        /**
         * @brief �����ִ�Сд�Ƚ�TStringObject
         * 
         */
        class TMdbNtcNoCaseCompare:public TMdbNtcObjCompare
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcNoCaseCompare);
        public:
            virtual MDB_INT64 Compare(const TMdbNtcBaseObject* pObject1, const TMdbNtcBaseObject* pObject2) const
            {
                const TMdbNtcStringObject *p1 =  static_cast<const TMdbNtcStringObject*>(pObject1), *p2 = static_cast<const TMdbNtcStringObject*>(pObject2);
                return static_cast<const TMdbNtcStringBuffer*>(p1)->Compare(*static_cast<const TMdbNtcStringBuffer*>(p2), false);//�����ִ�Сд����
            }
        };

        
        /**
         * @brief ���ֲ���
         * 
         * @param pDataHead   [in] ������׵�ַ
         * @param uiDataSize   [in] ����Ĵ�С
         * @param pCompareObj [in] ��Ƚϵ�������Ϣ
         * @param pCompare    [in] �ȽϺ��������ΪNULL����ֻ����ָ��Ƚ�
         * @return int
         * @retval ��Ӧ������±� >=0 ���ҳɹ���-1����ʧ��
         */
        int MdbNtcBinarySearch(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
        /**
         * @brief �����㷨�Ļ���
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
             * @brief ���캯��
             * 
             * @param bSortAsc [in] �������
             */
            TMdbNtcSort(bool bSortAsc = true)
            {
                m_bSortAsc = bSortAsc;
            }
            /**
             * @brief ���������ǵ���
             * 
             * @param bSortAsc [in] �������
             */
            void SetSortAsc(bool bSortAsc)
            {
                m_bSortAsc = bSortAsc;
            }
            /**
             * @brief �ж��Ƿ�Ϊ����
             * 
             * @return bool
             */
            bool IsSortAsc()
            {
                return m_bSortAsc;
            }
            /**
             * @brief ���մ���������׵�ַ����
             * 
             * @param pDataHead  [in] ������׵�ַ
             * @param uiDataSize [in] ����Ĵ�С
             * @param oCompare   [in] �ȽϺ���
             */
            virtual void Sort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare) const = 0 ;
            /**
             * @brief ��������
             * 
             * @param pDataHead   [in] ������׵�ַ
             * @param uiLocation1 [in] ����Ԫ�ص��±�1
             * @param uiLocation2 [in] ����Ԫ�ص��±�2
             * @param oCompare    [in] �ȽϺ���
             */
            void Swap(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiLocation1, MDB_UINT32 uiLocation2) const
            {
                TMdbNtcBaseObject * pTempObj = NULL;
                pTempObj = pDataHead[uiLocation1];
                pDataHead[uiLocation1] = pDataHead[uiLocation2];
                pDataHead[uiLocation2] = pTempObj;
            }
        protected:
            bool m_bSortAsc;///< �Ƿ�Ϊ����
        };
        
        /**
         * @brief ��������
         * ������������������ΪO��nlog(2)(n)��������ΪO��n��2��
         * ���������ǵݹ�ģ������ڴ�ǳ����޵Ļ�����˵��������һ���õ�ѡ���ʺ����������Ǻܴ�ĳ���
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
             * @brief ���캯��
             * 
             * @param bSortAsc [in] �������
             */
            TMdbNtcQuickSort(bool bSortAsc = true):TMdbNtcSort(bSortAsc)
            {
            }
            /**
             * @brief ���մ���������׵�ַ����
             * 
             * @param pDataHead  [in] ������׵�ַ
             * @param uiDataSize [in] ����Ĵ�С
             * @param pCompare   [in] �ȽϺ���
             */
            virtual void Sort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare) const;
        protected:
            /**
             * @brief �ݹ���������㷨
             * 
             * @param pDataHead  [in] ������׵�ַ
             * @param uiLow      [in] ��ʼ��������λ��
             * @param uiHigh     [in] ������������λ��
             * @param oCompare   [in] �ȽϺ���
             */
            void QSort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiLow, MDB_UINT32 uiHigh,const TMdbNtcObjCompare &oCompare) const;
            /**
             * @brief һ�˿��������㷨
             * 
             * @param pDataHead       [in] ������׵�ַ
             * @param uiLow           [in] ��ʼ��������λ��
             * @param uiHigh          [in] ������������λ��
             * @param oCompare        [in] �ȽϺ���
             * @param bIsExchangeLow  [in] һ�������У�lowָ�����ƹ������Ƿ��������ݽ���
             * @param bIsExchangeHigh [in] һ�������У�highָ�����ƹ������Ƿ��������ݽ���
             * @return MDB_UINT32
             * @retval ����λ��
             */
            MDB_UINT32 Partion(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiLow, MDB_UINT32 uiHigh,const TMdbNtcObjCompare &oCompare, bool &bIsExchangeLow,bool &bIsExchangeHigh) const;
        };
        
        /**
         * @brief ������
         * �������ʺ����������ǳ���ĳ��ϣ��������ݣ��� 
         * ��������ʱ�临�Ӷ�ΪO(nlog2n)�������ƽ�����ܽϽӽ�������ܡ� 
         * ���ڽ���ʼ������ıȽϴ����϶࣬���Զ����������ڼ�¼�����ٵ��ļ��� 
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
             * @brief ���캯��
             * 
             * @param bSortAsc [in] �������
             */
            TMdbNtcHeapSort(bool bSortAsc = true):TMdbNtcSort(bSortAsc){}
            /**
             * @brief ���մ���������׵�ַ����
             * 
             * @param pDataHead  [in] ������׵�ַ
             * @param uiDataSize [in] ����Ĵ�С
             * @param oCompare   [in] �ȽϺ���
             */
            virtual void Sort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare) const;
        protected:
            /**
             * @brief ������
             * ��ϸ˵��������m_bSortAsc���ѣ�����ֵΪtrue�������ɴ󶥶ѣ����������С����
             * @param pDataHead  [in] ������׵�ַ
             * @param uiStartLoc [in] �����ѿ�ʼλ��
             * @param uiMaxLoc   [in] �����ѽ���λ��
             * @param oCompare   [in] �ȽϺ���
             */
            void HeapAdjust(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiStartLoc, MDB_UINT32 uiMaxLoc, const TMdbNtcObjCompare &oCompare) const;
        };
        
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        //&&&&                                                              &&&&&&
        //&&&&                       ���ݽṹ����                           &&&&&&
        //&&&&                                                              &&&&&&
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
                
        /**
         *   @brief  �����ڵ����
         *   �ṩ�ڵ��ȡԪ���ݵķ�����
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
             * @brief �ڵ��ıȽ�
             * 
             * @param pNode [in] ��֮�ȽϵĽڵ�
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
            TMdbNtcBaseObject * pData;  ///< ������Ϣ
        };
        
        /**
         * @brief �������ඨ��
         */
        class TMdbNtcContainer:public TMdbNtcBaseObject
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcContainer);
        public:
            //typedef TMdbNtcBaseObject** iterator;
           /**
            *   @brief  ��������
            *  �ṩ������Ԫ�ر����ķ�����
            *  ʹ�÷�������:
            *  TBaseContainer::iterator itor = pContainer->IterBegin(), itorEnd = pContainer->IterEnd();
            *  for(; itor != itorEnd; ++itor)
            *  {
            *       itor.data();//��������Ӧ��Ԫ����TBaseObject*
            *  }
            */
            class iterator
            {
            public:
                /**
                 *	@brief	��������ȱʡ���캯��
                 *	(��ϸ˵��)
                 *	@param pDataContainer  [in],��������
                 *	@return ��
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
                 *	@brief	�������๹�캯��
                 *	(��ϸ˵��)
                 *	@param pDataContainer  [in],��������
                 *	@param ppObject  [in],ָ�������ָ��ĵ�ַ
                 *	@return ��
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
                 *	@brief	�������๹�캯��
                 *	(��ϸ˵��)
                 *	@param pDataContainer  [in],��������
                 *	@param pNodeObject  [in],ָ�������ָ��ĵ�ַ
                 *	@return ��
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
                 *	@brief	�������๹�캯��
                 *	(��ϸ˵��)
                 *	@param pDataContainer   [in] ��������
                 *	@param pParentContainer [in] ���������ĸ�����
                 *	@param pNodeObject  [in],ָ�������ָ��ĵ�ַ
                 *	@return ��
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
                 *	@brief	�������๹�캯��
                 *	(��ϸ˵��)
                 *	@param pDataContainer  [in],��������
                 *	@param itor  [in],ָ�������ָ��ĵ�����
                 *	@return ��
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
                 *  @brief  ++����������
                 *  ǰ��++
                 *  @param ��
                 *  @return iterator &
                 *  @retval  ��һ��������
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
                 *  @brief  ++����������
                 *  ����++
                 *  @param int
                 *  @return iterator 
                 *  @retval ��һ��������
                 */           
                inline iterator operator++(int)
                {
                    iterator tmp = *this;
                    ++*this;
                    return (tmp);
                }
                /**
                 *  @brief  --����������
                 *  ǰ��--
                 *  @param ��
                 *  @return iterator &
                 *  @retval ǰһ��������
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
                 *  @brief  --����������
                 *  ����--
                 *  @param ��
                 *  @return iterator &
                 *  @retval ǰһ��������
                 */           
                inline iterator operator--(int)
                {
                    iterator tmp = *this;
                    --*this;
                    return (tmp);
                }
                /**
                 *  @brief  +����������
                 *  ʵ�ֵ����������ƫ��
                 *  @param iStep [in] ,���ƫ�ƵĲ���
                 *  @return iterator 
                 *  @retval ƫ�ƺ�ĵ�����
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
                 *  @brief  -����������
                 *  ʵ�ֵ���������ǰƫ��
                 *  @param iStep [in] ,��ǰƫ�ƵĲ���
                 *  @return iterator 
                 *  @retval  ƫ�ƺ�ĵ�����
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
                 *  @brief  ==����������
                 *  �Ƚ������������Ƿ����
                 *  @param itor [in] ,�Ƚϵĵ�����
                 *  @return bool
                 *  @retval true--��ȣ�false--�����
                 */                  
                inline bool operator==(const iterator& itor) const
                {
                    return (ppObject == itor.ppObject);
                }
                /**
                 *  @brief  ��=����������
                 *  �Ƚ������������Ƿ����
                 *  @param itor [in] ,�Ƚϵĵ�����
                 *  @return bool 
                 *  @retval  true--����ȣ�false--���
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
                *   @brief  ����������
                *   
                *   @param itor [in] ,�����ĵ�����
                *   @return void
                *   @retval
                */              
                inline void swap(iterator itor)
                {
                     (const_cast<TMdbNtcContainer *>(pDataContainer))->IterSwap(*this, itor);
                }
                /**
                 *  @brief  ��ȡ��������Ӧ��Ԫ����
                 *  
                 *  @param ��
                 *  @return TMdbNtcBaseObject*&
                 *  @retval Ԫ�ػ��߽ڵ�ָ�������
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
                 *  @brief  ��ȡ��������Ӧ��Ԫ����
                 *  
                 *  @param ��
                 *  @return TMdbNtcBaseObject*&
                 *  @retval Ԫ��ָ�������
                 */               
                inline TMdbNtcBaseObject* data() const
                {
                    return ppObject?*ppObject:(TMdbNtcContainer::ms_pNullObject);
                }
                /**
                 *  @brief  ��ȡ��������Ӧ�Ľ���Ԫ����
                 *  
                 *  @param ��
                 *  @return TNodeObject*&
                 *  @retval �ڵ�ָ�������
                 */               
                inline TMdbNtcBaseNode*& node() 
                {
                    return pNodeObject;
                }
                /**
                 *  @brief  ��ȡ��������Ӧ�Ľ���Ԫ����
                 *  
                 *  @param ��
                 *  @return TNodeObject*&
                 *  @retval �ڵ�ָ��
                 */               
                inline TMdbNtcBaseNode* node() const
                {
                    return pNodeObject;
                }
            public:     
                const TMdbNtcContainer*     pDataContainer; ///< ����
                const TMdbNtcContainer*     pParentContainer;///< ������
                int                   iLastStep;      ///< ��¼��һ�ε�����ǰ�ƻ��ߺ��ƵĲ���
                union
                {
                    TMdbNtcBaseObject**  ppObject;    ///< Ԫ�ص�ַ
                    TMdbNtcBaseNode*   pNodeObject; ///< �ڵ��ַ
                };
            };
            friend class iterator;
        public:
            /**
             * @brief ���캯��
             * 
             */
            TMdbNtcContainer();
            /**
             * @brief ��������
             * ����ʱ�������Clear��������Ԫ�ػ�ڵ�
             */
            virtual ~TMdbNtcContainer();
            /**
             * @brief �����ͷ��Զ��ͷ�Ԫ��ָ����ڴ�
             * 
             * @param bAutuRelease [in] �Ƿ��Զ��ͷ�Ԫ��ָ����ڴ�
             * @return ��
             */
            virtual void SetAutoRelease(bool bAutuRelease);
            /**
             * @brief �Ƿ��Զ��ͷ�Ԫ��ָ����ڴ�
             * 
             * @return bool
             * @retval true �Զ��ͷ�
             */
            inline bool IsAutoRelease() const
            {
                return m_bAutoRelease;
            }
            /**
             *  @brief  �ж������Ƿ�Ϊ��
             *  (��ϸ˵��)
             *  @param ��
             *  @return bool
             *  @retval  true--�գ�false--�ǿ�
             */
            virtual bool IsEmpty() const
            {
                return GetSize() == 0;
            }
            /**
             *  @brief  ��ȡ������Ԫ����Ŀ
             *  (��ϸ˵��)
             *  @param ��
             *  @return MDB_UINT32
             *  @retval Ԫ����Ŀ
             */    
            virtual MDB_UINT32 GetSize() const = 0;
            /**
             * @brief �������ռ���ڴ�
             *      
             * @return MDB_UINT32
             * @retval �ڴ�ռ��
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const = 0;
            /**
             * @brief ��������е�Ԫ����ռ���ڴ�Ĵ�С
             *      
             * @return MDB_UINT32
             * @retval �����е�Ԫ����ռ���ڴ�Ĵ�С
             */
            virtual MDB_UINT32 GetDataMemoryUsage() const;
            /**
             * @brief ���������Ԫ����ռ���ڴ���ܴ�С
             *      
             * @return MDB_UINT32
             * @retval ������Ԫ����ռ���ڴ���ܴ�С
             */
            inline MDB_UINT32 GetTotalMemoryUsage() const
            {
                return GetContainerMemoryUsage()+GetDataMemoryUsage();
            }
            /**
             *  @brief  �������
             *  (��ϸ˵��)
             *  @param ��
             *  @return void
             *  @retval
             */        
            virtual void Clear() = 0;
            /**
             * @brief ��ӡ������Ϣ
             *  
             * @param  fp [in] ��ӡ�ķ���NULL��ʾֱ����stdout���,����ͨ���ļ�ָ�����
             * @return ��
             */
            virtual void Print(FILE* fp = NULL) const;
            /**
             * @brief �Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
             *  
             * @param   itor [in] Ҫ�Ƴ��ĵ�����
             * @return  iterator
             * @retval  ��һ��������
             */
            virtual iterator IterErase(iterator itor) = 0;
            /**
             * @brief ���ݵ����������Ƴ��ڵ㣬������һ��������
             *  
             * @param   itorBegin [in] �Ƴ�����Ŀ�ʼ������
             * @param   itorEnd   [in] �Ƴ�����Ľ���������
             * @return  iterator
             * @retval  ��һ��������
             */
            virtual iterator IterErase(iterator itorBegin, iterator itorEnd);
            /**
             *  @brief  ͨ�����������������Ԫ�غͽڵ�Ľ���
             *  (��ϸ˵��)
             *  @param itor1  [in],������1
             *  @param itor2  [in],������2
             *  @return void
             */            
            virtual void IterSwap(iterator itor1, iterator itor2) = 0;
            /**
             *   @brief  ��ÿ�ʼ������
             *   (��ϸ˵��)
             *   @param ��
             *   @return void
             */    
            virtual iterator IterBegin() const
            {
                return IterEnd();
            }
            /**
             *   @brief  �����ֹ������
             *   (��ϸ˵��)
             *   @param ��
             *   @return void
             */    
            inline iterator IterEnd() const
            {
                return iterator(this);
            }
            /**
             *  @brief  ���βԪ�ص�����
             *  (��ϸ˵��)
             *  @param ��
             *  @return void
             */    
            virtual iterator IterLast() const
            {
                return iterator(this);
            }
            /**
             *   @brief ���ݵ�������ȡ��Ӧ��Ԫ����
             *    (��ϸ˵��)
             *  @param itor  [in],������
             *  @return TMdbNtcBaseObject *&
             *  @retval Ԫ��ָ�������
             */      
            virtual TMdbNtcBaseObject*& IterData(TMdbNtcContainer::iterator itor) const
            {
                return itor.pNodeObject? itor.pNodeObject->pData:ms_pNullObject;
            }
            /**
             * @brief ���ݱȽϲ���ƥ������ݵ�����
             * 
             * @param oData       [in] �Ƚϵ�����
             * @param oCompare    [in] �ȽϷ��������ΪֵΪg_PointerCompare����ֻ����ָ��Ƚ�
             * @param itLastFound [in] ��һ���ҵ��ĵ�����(����ʱ�Ӵ˵�������һ����ʼ)�����Ϊend,���ͷ��ʼ
             * @return iterator
             */
            iterator IterFind(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, iterator itLastFound = ms_itorEnd) const;
        protected:
            /**
             *  @brief  ��ȡǰһ��������
             *  (��ϸ˵��)
             *  @param itCur  [in],��ǰ������
             *  @param iStep  [in],����
             *  @return iterator
             *  @retval  ǰһ��������
             */  
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const
            {
                return IterEnd();
            }
            /**
             *  @brief  ��ȡ��һ��������
             *  (��ϸ˵��)
             *  @param itCur  [in],��ǰ������
             *  @param iStep  [in],����
             *  @return iterator
             *  @retval  ��һ��������
             */  
            virtual iterator IterNext(iterator itCur, int iStep = 1) const
            {
                return IterEnd();
            }
        protected:
            bool m_bAutoRelease;                ///< �Զ��ͷ�Ԫ���ڴ棬Ĭ��false�����Զ��ͷ�
            static iterator ms_itorEnd;         ///< ��Ϊ����Ĭ��ȡֵ���ݽ�����������ʹ��
            static TMdbNtcBaseObject* ms_pNullObject; ///< ��Ϊ��������ָ�ĵĽڵ�ΪNULL����Ҫ���㲻���쳣����Ҫ��������
        };
        
        /**
         * @brief �ӿ�ʼ���������ҵ�����������������ƥ���Ԫ����
         * 
         * @param itorBegin [in] ��ʼ������
         * @param itorEnd   [in] ����������
         * @param oData     [in] ��Ҫ�ȽϵĶ���
         * @param oCompare  [in] �ȽϷ���
         * @return iterator
         * @retval Ԫ���ݵĵ�����
         */
        TMdbNtcContainer::iterator IterFind(TMdbNtcContainer::iterator itorBegin, TMdbNtcContainer::iterator itorEnd,
            const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
        
        /**
         * @brief ��̬����
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
             *  @brief  ���캯��
             *  (��ϸ˵��)
             *  @param uiGrowBy [in],�����ڴ�ʱ������Ԫ�ظ�������ﵽ����ʱ��ѹ����Ԫ��Ƶ�������ڴ�
             *  @return ��
             */  
            TMdbNtcAutoArray(MDB_UINT32 uiGrowBy = 0);
            /**
             *  @brief �������캯��
             *  (��ϸ˵��)
             *  @param oSrcArray [in] Դ����
             *  @return ��
             */  
            TMdbNtcAutoArray(const TMdbNtcAutoArray& oSrcArray);
            /**
             *  @brief ���캯���������������й���
             *  (��ϸ˵��)
             *  @param pSrcContainer [in] Դ����
             *  @return ��
             */  
            TMdbNtcAutoArray(const TMdbNtcContainer* pSrcContainer);
            /**
             * @brief ��ֵ���㷨
             * 
             *  @param oSrcArray [in] Դ����
             *  @return ��
             */
            inline TMdbNtcAutoArray& operator = (const TMdbNtcAutoArray& oSrcArray)
            {
                return *this = static_cast<const TMdbNtcContainer&>(oSrcArray);
            }
            /**
             * @brief ��ֵ���㷨
             * 
             *  @param oSrcContainer [in] Դ����
             *  @return ��
             */
            TMdbNtcAutoArray& operator = (const TMdbNtcContainer& oSrcContainer);
            /**
             * @brief ��������
             * 
             */
            virtual ~TMdbNtcAutoArray();
            /**
             * @brief �õ������С��ʵ��Ԫ����Ŀ��
             * ��ʹ��һ������֮ǰ��ʹ��SetSize�������Ĵ�С��Ϊ�������ڴ档
             * �����ʹ��SetSize����Ϊ�������Ԫ�ؾͻ�����Ƶ�������·���Ϳ�����
             * Ƶ�������·���Ϳ�������û��Ч�ʣ����ҵ����ڴ���Ƭ��
             * @param ��
             * @return  MDB_UINT32
             * @retval Ԫ����Ŀ
             */
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief �������ռ���ڴ�
             *      
             * @return MDB_UINT32
             * @retval �ڴ�ռ��
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief �������
             *����ϸ˵����
             * @param ��
             * @return void
             */
            virtual void Clear();
            /**
             * @brief ���Ԥ���������Ĵ�С
             *  
             * @param ��
             * @return MDB_UINT32
             * @retval  ��������
             */
            MDB_UINT32 GetCapacity() const
            {
                return m_uiCapacity;
            }
            /**
             * @brief ��ȡ�����׵�ַ
             *  
             * @param ��
             * @return TMdbNtcBaseObject**
             * @retval  �����׵�ַ
             */
            TMdbNtcBaseObject** GetData() const
            {
                return m_pData;
            }
            /**
             * @brief ������·���ʱԪ�ص�������Ŀ
             *  
             * @param ��
             * @return MDB_UINT32
             * @retval Ԫ�ص�������Ŀ
             */
            MDB_UINT32 GetGrowBy() const
            {
                return m_uiGrowBy;
            }
            /**
             * @brief Ԥ�������С��������Ԫ����Ŀ���䡣�²���ڵ�ʱ������Ƶ�������ڴ�
             *  
             * @param  uiCapacity [in] Ԥ��������
             * @return void
             */
            void Reserve(MDB_UINT32 uiCapacity);
            /**
             * @brief ��������Ĵ�С,����Ԫ����Ŀ�����仯
             * ͨ���˷������Ըı������С������������������NULL��Ϊ��Ԫ��ѹ�룬��������С����ɾ���ͷŲü���Ԫ��
             * 
             * @param  uiNewSize [in] ָ������Ĵ�С,ȡֵΪ�Ǹ���
             * @return void
             */
            void SetSize(MDB_UINT32 uiNewSize);
            /**
             * @brief ͨ��[]�������ȡԪ��
             * ��ϸ˵�� ���ڳ�������
             * @param  uiIndex [in] �����±�
             * @return TMdbNtcBaseObject*
             * @retval  ��ӦԪ�ص��±�
             */
            TMdbNtcBaseObject* operator[](MDB_UINT32 uiIndex) const
            {
                if(uiIndex < m_uiSize) return m_pData[uiIndex];
                else return NULL;
            }
            /**
             * @brief ͨ��[]�������ȡԪ��
             * ��ϸ˵�� ���ڳ�������
             * @param  iIndex [in] �����±�
             * @return TMdbNtcBaseObject*
             * @retval ��Ӧ�±��Ԫ��
             */
            TMdbNtcBaseObject*& operator[](MDB_UINT32 uiIndex)
            {
                return m_pData[uiIndex];
            }
            /**
             * @brief �����±��ȡԪ��
             *  
             * @param  iIndex [in] ��Ҫ��ȡ��Ԫ���±�
             * @return TMdbNtcBaseObject*
             * @retval ��Ӧ�±��Ԫ��
             */
            TMdbNtcBaseObject* GetAt(MDB_UINT32 uiIndex) const
            {
                if(uiIndex < m_uiSize) return m_pData[uiIndex];
                else return NULL;
            }
            /**
             * @brief �õ�Ԫ�ص��±�
             * (��ϸ˵��)
             * @param oData     [in] �Ƚϵ�����
             * @param  oCompare [in] �ȽϺ���
             * @param  uiStart  [in] ��ʼ����λ��
             * @return int
             * @retval >=0--��ȡ�±�ɹ���-1--��ȡʧ��
             */
            int FindIndex(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, MDB_UINT32 uiStart = 0) const;
            /**
             * @brief �õ�ƥ�������ָ��
             * (��ϸ˵��)
             * @param oData    [in] �Ƚϵ�����
             * @param oCompare [in] �ȽϷ���
             * @param uiStart  [in] ��ʼ����λ��
             * @return TMdbNtcBaseObject*
             * @retval ��NULL �������ݵ�ָ��
             */
            TMdbNtcBaseObject* FindData(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, MDB_UINT32 uiStart = 0) const
            {
                int iIndex = FindIndex(oData, oCompare, uiStart);
                return iIndex>=0?*(m_pData+iIndex):NULL;
            }
            /**
             * @brief ���һ��Ԫ��
             * (��ϸ˵��)
             * @param  pNewObj [in] �µ�Ԫ��ָ��
             * @return int
             * @retval ����ӵ�Ԫ�ص��±�
             */
            int Add(TMdbNtcBaseObject* pNewObj);
            /**
             * @brief ׷����������������
             * 
             * @param itSrcBegin [in] ��ʼ������
             * @param itSrcEnd   [in] ����������;����Ϊ������־����ʹ������Ч����Ҳ����ӵ������У�Ĭ�ϵ�ĩβ
             * @return int
             * @retval ����ӵ�Ԫ�صĿ�ʼ�±�
             */
            int Add(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief ����һ��Ԫ��
             * (��ϸ˵��)
             * @param uiIndex [in] ָ��λ�õ��±�
             * @param pNewObj [in] ������Ԫ�ص�ָ��
             * @return MDB_UINT32
             * @retval ��ӵ�Ԫ�ص��±�
             */
            MDB_UINT32 Insert(MDB_UINT32 uiIndex, TMdbNtcBaseObject* pNewObj);
            /**
             * @brief ׷����������������
             * 
             * @param uiIndex    [in] ָ��λ�õ��±�
             * @param itSrcBegin [in] ��ʼ������
             * @param itSrcEnd   [in] ����������;����Ϊ������־����ʹ������Ч����Ҳ����ӵ�������
             * @return MDB_UINT32
             * @retval ����ӵ�Ԫ�صĿ�ʼ�±�
             */
            MDB_UINT32 Insert(MDB_UINT32 uiIndex, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief ����ɾ��ָ��������Ԫ��
             *  
             * @param uiIndex    [in] Ҫɾ���ĵ�һ��Ԫ���±�
             * @param uiDelCount [in] ��Ҫ����ɾ����Ԫ�ظ���
             * @return int
             * @retval  ɾ����Ԫ�ظ���
             */
            int Remove(MDB_UINT32 uiIndex, MDB_UINT32 uiDelCount = 1);
            /**
             * @brief �����������Ƴ�ƥ��Ľڵ�
             * ��ϸ˵�� ����������֮ƥ��Ľڵ㶼�Ƴ���
             * 
             * @param  pData     [in] Ҫ�Ƴ�������Ԫ��ָ��             
             * @param  iDelCount [in] ��Ҫɾ����Ԫ�ظ���,-1��ʾɾ������ƥ��ڵ�
             * @param  oCompare  [in] �ȽϺ���
             * @return int
             * @retval  ɾ����Ԫ�ظ���
             */
            int Remove(const TMdbNtcBaseObject &oData, int iDelCount = 1, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief �ϲ�����, Src�����е����ݽ��Ƶ��������У�Src����Ԫ����Ŀ����
             * (��ϸ˵��)
             * @param  iDestIndex    [in] ����Ŀ�������λ�ã��±꣩,-1��ʾ��ӵ�ĩβ
             * @param  pSrcContainer [in] Դ����
             * @param  iSrcStart     [in] ��Ҫ�����Դ����Ԫ�صĿ�ʼλ�ã��±꣩��ֵС��0��ͷ����ʼ
             * @param  iSrcCount     [in] ��Ҫ�����Դ����Ԫ�ص���Ŀ,(С��)-1��ʾ��ʼλ�õ������ĩβ
             * @return int
             * @retval ��ӵ�Ԫ���±�
             */
            int Combine( int iDestIndex, TMdbNtcContainer* pSrcContainer, int iSrcStart = 0, int iSrcCount = -1);
            /**
             * @brief Ԫ������
             * (��ϸ˵��)
             * @param  oSort    [in] ���򷽷���
             * @param  oCompare [in] �ȽϺ���
             * @return void
             */
            void Sort(const TMdbNtcSort& oSort, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief ȥ���ظ���Ԫ��
             *  
             * @param  oCompare [in] �ȽϺ���
             * @return int
             * @retval  ȥ�ص�Ԫ�ظ���
             */
            int RemoveDup(const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
        public:
            /**
             * @brief ��ÿ�ʼ������
             * 
             * @param  ��
             * @return iterator
             * @retval ��ʼ������
             */
            virtual iterator IterBegin() const;
           /**
            *  @brief   ���βԪ�ص�����
            *  (��ϸ˵��)
            *  @param ��
            *  @return void
            */    
            virtual iterator IterLast() const;
           /**
             *  @brief  ���ݵ�������ȡ��Ӧ��Ԫ����
             *  (��ϸ˵��)
             *  @param itor  [in],������
             *  @return TMdbNtcBaseObject *&
             *  @retval Ԫ����ָ�������
             */      
            virtual TMdbNtcBaseObject*& IterData(iterator itor) const
            {
                return itor.ppObject?(*itor.ppObject):ms_pNullObject;
            }   
            /**
             * @brief �Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
             * 
             * @param itor [in] Ҫ�Ƴ��ĵ�����
             * @return iterator
             * @retval ��һ��������
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief ���ݵ����������Ƴ��ڵ㣬������һ��������
             *  
             * @param   itorBegin [in] �Ƴ�����Ŀ�ʼ������
             * @param   itorEnd   [in] �Ƴ�����Ľ���������
             * @return  iterator
             * @retval  ��һ��������
             */
            virtual iterator IterErase(iterator itorBegin, iterator itorEnd);
            /**
             * @brief ͨ�����������������Ԫ�غͽڵ�Ľ���
             * 
             * @param itor1 [in] ������1
             * @param itor2 [in] ������2
             */
            virtual void IterSwap(iterator itor1, iterator itor2);
            /**
             * @brief ͨ����������λ��������
             * 
             * @param itor  [in] Ŀ��λ�õ�����
             * @param pData [in] ����
             * @return  iterator
             * @retval  ָ��pData�ĵ�����
             */
            iterator IterInsert(iterator itor,TMdbNtcBaseObject* pData);
        protected:
            /**
             * @brief ���ǰһ��������
             * 
             * @return iterator
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief ��ú�һ��������
             * 
             * @return iterator
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
        protected:
            TMdbNtcBaseObject**   m_pData;        ///< ���ݵ��׵�ַ
            MDB_UINT32    m_uiSize;       ///< �û���ǰ���������Ĵ�С
            MDB_UINT32    m_uiCapacity;   ///< ������������
            MDB_UINT32    m_uiGrowBy;     ///< �����ڴ�ʱ������Ԫ�ظ���
        };
        
        /**
        *   @brief  ջ
        *   ���ڶ�̬����ʵ��
        *   ��������Ա�����º��� 
        *    m_pData--------ջ�׵�ַ
        *    m_uiSize-------ջ��λ��,ջ��Ԫ�ص���һλ
        *    m_uiCapacity --ջ����
        *    m_uiGrowB------���������еĺ��壬ջ�ĳ�Ա������δ�õ�
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
             * @brief ���캯��
             * 
             * @param uiStackCapacity [in] ջ����
             * @return ��
             * @retval 
             */
            TMdbNtcStack (MDB_UINT32 uiStackCapacity);
            /**
             *  @brief �������캯��
             *  (��ϸ˵��)
             *  @param oSrcStack [in] Դ����
             *  @return ��
             */  
            TMdbNtcStack(const TMdbNtcStack& oSrcStack):TMdbNtcAutoArray(oSrcStack)
            {
            }
            /**
             *  @brief ���캯���������������й���
             *  (��ϸ˵��)
             *  @param pSrcContainer [in] Դ����
             *  @return ��
             */  
            TMdbNtcStack(const TMdbNtcContainer* pSrcContainer):TMdbNtcAutoArray(pSrcContainer)
            {
            }
            /**
             * @brief ��ֵ���㷨
             * 
             *  @param oSrcStack [in] Դ����
             *  @return ��
             */
            inline TMdbNtcStack& operator = (const TMdbNtcStack& oSrcStack)
            {
                *static_cast<TMdbNtcAutoArray*>(this) = static_cast<const TMdbNtcContainer&>(oSrcStack);
                return *this;
            }
            /**
             * @brief ��ֵ���㷨
             * 
             *  @param oSrcContainer [in] Դ����
             *  @return ��
             */
            inline TMdbNtcStack& operator = (const TMdbNtcContainer& oSrcContainer)
            {
                *static_cast<TMdbNtcAutoArray*>(this) = static_cast<const TMdbNtcContainer&>(oSrcContainer);
                return *this;
            }
            /**
             * @brief ��ջ
             *  ������ѹ��ջ��
             * @param pData [in] ѹ��ջ��������ָ��
             * @return int
             * @retval ջ��λ��
             */
            int Push (TMdbNtcBaseObject *pData);
            /**
             * @brief  ��ջ
             *  ����ջ��Ԫ��
             * @param void 
             * @return TMdbNtcBaseObject *
             * @retval  ����ջ���ĵ�Ԫ��,NULL��ʾ��Ԫ��
             */
            TMdbNtcBaseObject * Pop ();
            /**
             * @brief  ��ȡջ��Ԫ��
             *  
             * @param void 
             * @return TMdbNtcBaseObject *
             * @retval  ��ȡջ���ĵ�Ԫ��(������ջ��Ԫ��),NULL��ʾ��Ԫ��
             */
            TMdbNtcBaseObject * Top () const;
            //�����ǽ�TAutoArray���һЩ�ӿ�Ȩ�޹������������ⲿʹ��
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
         * @brief ˫������
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
             * @brief ˫������Ľڵ���Ϣ
             * 
             */
            class TNode:public TMdbNtcBaseNode
            {
            public:
                /**
                 * @brief ���캯��
                 * 
                 * @param pData [in] ������Ϣ
                 * @return void 
                 */
                 TNode(TMdbNtcBaseObject *pData = NULL);
                /**
                 * @brief �Ƚ������ڵ�
                 * ʵ�ʱȽ�pData,���pDataΪNULL�����������
                 * 
                 * @param pNode [in] Ҫ��֮�ȽϵĽڵ�
                 * @return int
                 * @retval >0 ���� =0 ���� <0 С��
                 */
                 //virtual int Compare(const TNode *pNode) const;
            public:
                TNode*         pPrev;///< ��һ���ָ��
                TNode*         pNext;///< ��һ���ָ��
            };
            /**
             * @brief ���캯��
             * 
             * @param bAutuRelease [in] �Ƿ��Զ��ͷ�Ԫ��ָ����ڴ�
             * @return ��
             */
            TMdbNtcBaseList();
            /**
             *  @brief �������캯��
             *  (��ϸ˵��)
             *  @param oSrcList [in] Դ����
             *  @return ��
             */  
            TMdbNtcBaseList(const TMdbNtcBaseList& oSrcList);
            /**
             *  @brief ���캯���������������й���
             *  (��ϸ˵��)
             *  @param pSrcContainer [in] Դ����
             *  @return ��
             */  
            TMdbNtcBaseList(const TMdbNtcContainer* pSrcContainer);
            virtual ~TMdbNtcBaseList();
            /**
             * @brief ��ֵ���㷨
             * 
             *  @param oSrcList [in] Դ����
             *  @return ��
             */
            inline TMdbNtcBaseList& operator = (const TMdbNtcBaseList& oSrcList)
            {
                return *this = static_cast<const TMdbNtcContainer&>(oSrcList);
            }
            /**
             * @brief ��ֵ���㷨
             * 
             *  @param oSrcContainer [in] Դ����
             *  @return ��
             */
            TMdbNtcBaseList& operator = (const TMdbNtcContainer& oSrcContainer);
            /**
             *  @brief  �ж������Ƿ�Ϊ��
             *  (��ϸ˵��)
             *  @param ��
             *  @return bool 
             *  @retval  true��ʾΪ�գ�false��ʾ�ǿ�;
             */
            virtual bool IsEmpty() const
            {
                return m_uiSize == 0 ;
            }
            /**
             * @brief �õ�����Ԫ�ص���Ŀ
             *
             * @param  :��
             * @return MDB_UINT32
             * @retval �õ�����Ԫ�صĸ���
             */
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief �������ռ���ڴ�
             *      
             * @return MDB_UINT32
             * @retval �ڴ�ռ��
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief �������
             *
             * @param  :��
             * @return void
             */
            virtual void Clear();
            /**
             * @brief ��ȡͷ���
             *
             * @param  :��
             * @return TNode*,�õ�ͷ���
             */
            TNode* GetHead() const
            {
                return m_pHeadNode;
            }
            /**
             * @brief ��ȡβ�ڵ�
             *
             * @param  :��
             * @return TNode*,�õ�β���
             */
            TNode* GetTail() const
            {
                return m_pTailNode;
            }
            /**
             * @brief ��ͷ����ӽڵ�
             *
             * @param  :pData [in] ��Ҫ��ӵĽڵ�����
             * @return TNode*,����ͷ���
             */
            TNode* AddHead(TMdbNtcBaseObject* pData);
            /**
             * @brief ׷����������������
             * 
             * @param itSrcBegin [in] ��ʼ������
             * @param itSrcEnd   [in] ����������;����Ϊ������־����ʹ������Ч����Ҳ����ӵ�������
             * @return TNode*
             * @retval �����ڵ�ָ��
             */
            TNode* AddHead(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief ��β������½ڵ�
             * 
             * @param pNode [in] �½ڵ�
             * @return TNode*
             * @retval ����β�ڵ�
             */
            TNode* AddTail(TNode* pNode);
            /**
             * @brief ��β������½ڵ�
             * 
             * @param pData [in] ��Ҫ��ӵĽڵ�����
             * @return TNode*
             * @retval ����β�ڵ�
             */
            inline TNode* AddTail(TMdbNtcBaseObject* pData)
            {
                return AddTail(new TNode(pData));
            }
            /**
             * @brief ׷����������������
             * 
             * @param itSrcBegin [in] ��ʼ������
             * @param itSrcEnd   [in] ����������;����Ϊ������־����ʹ������Ч����Ҳ����ӵ������У�Ĭ�ϵ�ĩβ
             * @return TNode*
             * @retval �����ڵ�ָ��
             */
            TNode* AddTail(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief ��ָ���ڵ�ǰ����½ڵ�
             * 
             * @param pNode [in] ����λ�õĲο��ڵ�
             * @param pData [in] ��Ҫ��ӵ����ݽڵ�
             * @return TNode*
             * @retval �����ڵ�ָ��
             */
            TNode* InsertBefore(TNode* pNode, TMdbNtcBaseObject* pData);
            /**
             * @brief ׷����������������
             * 
             * @param pNode      [in] ����λ�õĲο��ڵ�
             * @param itSrcBegin [in] ��ʼ������
             * @param itSrcEnd   [in] ����������;����Ϊ������־����ʹ������Ч����Ҳ����ӵ�������
             * @return TNode*
             * @retval �����ڵ�ָ��
             */
            TNode* InsertBefore(TNode* pNode, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief ��ָ���ڵ������½ڵ�
             * 
             * @param pNode [in] ����λ�õĲο��ڵ�
             * @param pData [in] ��Ҫ��ӵ����ݽڵ�
             * @return TNode*
             * @retval �����ڵ�ָ��
             */
            TNode* InsertAfter(TNode* pNode, TMdbNtcBaseObject* pData);
            /**
             * @brief ׷����������������
             * 
             * @param pNode      [in] ����λ�õĲο��ڵ�
             * @param itSrcBegin [in] ��ʼ������
             * @param itSrcEnd   [in] ����������;����Ϊ������־����ʹ������Ч����Ҳ����ӵ�������
             * @return TNode*
             * @retval �����ڵ�ָ��
             */
            TNode* InsertAfter(TNode* pNode, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief ��ָ��λ������½ڵ�
             * ���iIndexλ������û�����ݣ�����뵽ĩβ
             * 
             * @param iIndex [in] ָ��λ�ã�-1��ʾĩβ
             * @param pData  [in] ��Ҫ��ӵĽڵ�����
             * @return TNode*
             * @retval �����ڵ�ָ��
             */
            TNode* InsertAt(MDB_UINT32 iIndex, TMdbNtcBaseObject* pData);
            /**
             * @brief ׷����������������
             * 
             * @param iIndex     [in] ָ��λ�ã�-1��ʾĩβ
             * @param itSrcBegin [in] ��ʼ������
             * @param itSrcEnd   [in] ����������;����Ϊ������־����ʹ������Ч����Ҳ����ӵ�������
             * @return TNode*
             * @retval �����ڵ�ָ��
             */
            TNode* InsertAt(int iIndex, TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief �Ƴ�ͷ���
             * ͬʱ�ͷŽڵ�������ָ����ڴ�
             * 
             * @return TNode*
             * @retval �����µ�ͷ��㣬��ΪNULL�������
             */
            TNode* RemoveHead();
            /**
             * @brief �Ƴ�β�ڵ�
             * ͬʱ�ͷŽڵ�������ָ����ڴ�
             * 
             * @return TNode*
             * @retval �����µ�β��㣬��ΪNULL�������
             */
            TNode* RemoveTail();
            /**
             * @brief ����ɾ��ָ��������Ԫ��
             * 
             * @param iIndex    [in] Ҫɾ���ĵ�һ��Ԫ���±�
             * @param iDelCount [in] ��Ҫ����ɾ����Ԫ�ظ���,-1��ʾɾ����ĩβ
             * @return int
             * @retval ɾ���ĸ���
             */
            int Remove(int iIndex, int iDelCount = 1);
            /**
             * @brief �Ƴ�ƥ��Ľڵ㣬������һ���ڵ�
             * ע��:���ɾ������β�ڵ��ʱ��Ӧ�÷���NULL
             * @param pNode [in] Ҫ�Ƴ��Ľڵ�
             * @return int
             * @retval ������һ���ڵ�
             */
            TNode* Remove(TNode* pNode);
            /**
             * @brief �����������Ƴ�ƥ��Ľڵ�
             * ������������֮ƥ��Ľڵ㶼�Ƴ���
             * 
             * @param pData     [in] Ҫ�Ƴ�������             
             * @param iDelCount [in] ��Ҫɾ����Ԫ�ظ��������Ϊ-1�����ʾɾ������ƥ��ڵ�
             * @param oCompare  [in] �ȽϷ���
             * @return int
             * @retval ɾ���ĸ���
             */
            int Remove(const TMdbNtcBaseObject &oData, int iDelCount = 1, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief ��������λ�ò�������
             * 
             * @param  uiIndex [in] ���λ��
             * @return TMdbNtcBaseObject*
             * @retval NULL ��ʾû���ҵ�����NULL����ʾ�ҵ������ݵ�ַ
             */
            TMdbNtcBaseObject* GetDataAt(MDB_UINT32 uiIndex) const
            {
                TNode* pNode = GetAt(uiIndex);
                return pNode?pNode->pData:NULL;
            }
            /**
             * @brief ��������λ�ò��ҽڵ�
             * 
             * @param uiIndex [in] ���λ��
             * @return TNode*
             * @retval ���ҵ��Ľڵ�ָ��
             */
            TNode* GetAt(MDB_UINT32 uiIndex) const;
            /**
             * @brief �������ݲ��ҽڵ㣬�����ڵ���������һ��ƥ���
             * 
             * @param pData          [in] ����ָ��
             * @param oCompare       [in] �ȽϷ���
             * @param pLastFoundNode [in] ��һ�β��ҵ��Ľڵ�, NULL��ʾ��ͷ��ʼ����     
             * @return TNode*
             * @retval ���ҵ��Ľڵ�
             */
            TNode* FindNode(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, TNode* pLastFoundNode = NULL) const;
            /**
             * @brief �õ�ƥ�������ָ�룬���ص�һ��ƥ��Ľڵ�
             * (��ϸ˵��)
             * @param oData    [in] �Ƚϵ�����
             * @param oCompare [in] �ȽϷ���
             * @return TMdbNtcBaseObject*
             * @retval ��NULL �������ݵ�ָ��     
             */
            TMdbNtcBaseObject* FindData(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare) const
            {
                TNode* pNode = FindNode(oData, oCompare);
                return pNode?pNode->pData:NULL;
            }
            /**
             * @brief ���ݽڵ��������λ��
             * 
             * @param pNode [in] �ڵ��ַ
             * @return int
             * @retval >=0 ���ҵ���-1 û�ҵ�
             */
            int FindIndex(TNode* pNode) const;
            /**
             * @brief �������ݲ�������λ��
             * 
             * @param pData    [in] ����ָ��
             * @param pCompare [in] �ȽϺ��������ΪNULL����ֻ����ָ��Ƚ�
             * @param uiStart  [in] ��ʼ���ҵ�λ��,Ĭ��Ϊ0
             * @return int
             * @retval >=0 ���ҵ���-1 û�ҵ�
             */
            int FindIndex(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, MDB_UINT32 uiStart = 0) const;
            /**
             * @brief �ϲ�����Src�����е����ݽ��Ƶ��������У�Src����ڵ���Ŀ����
             * 
             * @param iDestIndex    [in] �����ϲ���������λ��,-1��ʾ��ӵ�ĩβ
             * @param pSrcContainer [in] ���ϲ�������
             * @param iSrcStart     [in] ��Ҫ��������ݿ�ʼλ��
             * @param iSrcCount     [in] ��Ҫ�����Դ����Ԫ�ص���Ŀ,(С��)-1��ʾ��ʼλ�õ������ĩβ
             * @return int
             * @retval >=0 ��ӵ��½ڵ���±�
             */
            int Combine(int iDestIndex, TMdbNtcContainer* pSrcContainer, int iSrcStart = 0, int iSrcCount = -1);
            /**
             * @brief ����������нڵ����Ԫ������������
             * 
             * @param arrayData [out] ����������
             * @return void
             */
            void GenerateArray(TMdbNtcAutoArray& arrayData) const;
            /**
             * @brief ����ڵ������
             * ������ȵ���������Ҫ�죬��Ϊ���ǵ�����������ͨ���������ʵ�֣�
             * �����������Ƚ��ڵ���֯������ĵ�����������ٸ�������ڵ�˳��
             * 
             * @param oSort    [in] ���򷽷���
             * @param oCompare [in] �ȽϷ���
             */
            void Sort(const TMdbNtcSort& oSort, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief ȥ���ظ����ݵĽڵ�
             * 
             * @param oCompare [in] �ȽϷ���
             * @return int
             * @retval ȥ�صĽڵ����
             */
            int RemoveDup(const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
            /**
             * @brief ������������
             * 
             * @param List1 [in] ����1
             * @param List2 [in] ����2
             * @return void
             */
            void SwapList(TMdbNtcBaseList &SrcList);
        public:
            /**
             * @brief ��ÿ�ʼ������
             * 
             * @return iterator
             */
            virtual iterator IterBegin() const;
           /**
            *   @brief  ���βԪ�ص�����
            *   (��ϸ˵��)
            *   @param ��
            *   @return void
            */    
            virtual iterator IterLast() const;
            /**
             * @brief �Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
             * 
             * @param itor [in] Ҫ�Ƴ��ĵ�����
             * @return iterator
             * @retval ��һ��������
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief ͨ�����������������Ԫ�غͽڵ�Ľ���
             * 
             * @param itor1 [in] ������1
             * @param itor2 [in] ������2
             * @return void
             */
            virtual void IterSwap(iterator itor1, iterator itor2) ;
            /**
             * @brief ͨ����������λ��������
             * 
             * @param itor  [in] Ŀ��λ�õ�����
             * @param pData [in] ����
             * @return  iterator
             * @retval  ָ��pData�ĵ�����
             */
            inline iterator IterInsert(iterator itor,TMdbNtcBaseObject* pData)
            {
                return iterator(this, InsertBefore(static_cast<TNode*>(itor.pNodeObject), pData));
            }
        protected:
            /**
             * @brief ���ǰһ��������
             * 
             * @param itCur [in] ��ǰ������
             * @param iStep [in] ����
             * @return iterator��ǰһ��������
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief ��ú�һ��������
             * 
             * @param itCur [in] ��ǰ������
             * @param iStep [in] ����
             * @return iterator����һ��������
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
        protected:
            TNode*        m_pHeadNode;       ///< ͷ���ָ��
            TNode*        m_pTailNode;       ///< βָ��
            MDB_UINT32  m_uiSize;          ///< ����ڵ���Ŀ
        };
        
        /**
         * @brief ƽ�������
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
             *  @brief ��ֹ�������캯��(��Ϊ�ȽϺ����޷�����)
             */  
            TMdbNtcAvlTree(const TMdbNtcAvlTree& oSrcTree)
            {
            }
        public:
            /**
             * @brief ƽ����������Ľڵ�
             * 
             */
            class TNode:public TMdbNtcBaseNode
            {
            public:
                /**
                 * @brief ���캯��
                 * 
                 * @param pData [in] ������Ϣ
                 * @return ��
                 */
                TNode(TMdbNtcBaseObject *pData = NULL);
                /**
                 * @brief �����ڵ㣬��Զ�����ɾ���ڵ�ʱ
                 * 
                 * @param pSwapNode [in] Ҫ�����Ľڵ�
                 * @return bool
                 * @retval true �ɹ�
                 */
                virtual void SwapNode(TNode* pSwapNode);
            public:
                int      iBalanceFactor; ///ƽ�����ӣ��ǽڵ���������ĸ߶ȼ�ȥ�������ĸ߶ȵĸ߶Ȳ� 
                TNode    *pLeftSubNode;  ///���ӽڵ� 
                TNode    *pRightSubNode; ///���ӽڵ�
                TNode    *pParentNode;   ///���ڵ�
            };
        public:
            /**
             * @brief ���캯��
             * 
             * @param pObjCompare [in] Ԫ���ݵıȽϺ�������Ҫ����new���Ķ���
             * ���ΪNULL�����ʹ��TAvlTree::CompareNode��Ĭ�ϻ�ʹ�����ݱ����Compare����
             * @return ��
             */
            TMdbNtcAvlTree( TMdbNtcObjCompare *pCompare = NULL);
            /**
             * @brief ��ֵ���㷨
             * 
             *  @param oSrcArray [in] Դ����
             *  @return ��
             */
            inline TMdbNtcAvlTree& operator = (const TMdbNtcAvlTree& oSrcTree)
            {
                return *this = static_cast<const TMdbNtcContainer&>(oSrcTree);
            }
            /**
             * @brief ��ֵ���㷨
             * 
             *  @param oSrcContainer [in] Դ����
             *  @return ��
             */
            TMdbNtcAvlTree& operator = (const TMdbNtcContainer& oSrcContainer);
            /**
             * @brief ��������
             * 
             */
            virtual ~TMdbNtcAvlTree();
            /**
             * @brief ��ӡ������Ϣ
             *  
             * @param  fp [in] ��ӡ�ķ���NULL��ʾֱ����stdout���,����ͨ���ļ�ָ�����
             * @return ��
             */
            virtual void Print(FILE* fp = NULL) const;
            /**
             * @brief �õ�������Ԫ�ص���Ŀ
             * 
             * @param void
             * @return MDB_UINT32 Ԫ�ص���Ŀ
             */
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief �������ռ���ڴ�
             *      
             * @return MDB_UINT32
             * @retval �ڴ�ռ��
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief �������
             * 
             * @param  void
             * @return void
             */
            virtual void Clear();
            /**
             * @brief ��ӽڵ�, �����Ӧ��λ�����Ѿ����ڽڵ㣬�����µ����
             * 
             * @param pData [in] ������Ϣ
             * @return iterator
             * @retval ����ӵ����ڵ������
             */
            iterator Add(TMdbNtcBaseObject* pData);
            /**
             * @brief ׷����������������
             * 
             * @param itSrcBegin [in] ��ʼ������
             * @param itSrcEnd   [in] ����������;����Ϊ������־����ʹ������Ч����Ҳ����ӵ������У�Ĭ�ϵ�ĩβ
             * @return iterator
             * @retval ����ӵ����ڵ������
             */
            iterator Add(TMdbNtcContainer::iterator itSrcBegin, TMdbNtcContainer::iterator itSrcEnd);
            /**
             * @brief �������ݲ���ƥ��Ľڵ㣬��ɾ���ڵ�
             * 
             * @param pData     [in] ������Ϣ
             * @param iDelCount [in] ��Ҫ����ɾ����Ԫ�ظ���,-1��ʾɾ������ƥ���
             * @return int
             * @retval ɾ���ĸ���
             */
            inline int Remove(const TMdbNtcBaseObject &oData, int iDelCount = 1)
            {                
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                return Remove(oNode, iDelCount);
            }
            /**
             * @brief �õ�ƥ�������ָ�룬���ص�һ��ƥ��Ľڵ�
             * (��ϸ˵��)
             * @param oData    [in] �Ƚϵ�����
             * @param oCompare [in] �ȽϷ���
             * @return TMdbNtcBaseObject*
             * @retval ��NULL �������ݵ�ָ��     
             */
            inline TMdbNtcBaseObject* FindData(const TMdbNtcBaseObject &oData) const
            {
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                iterator itor = IterFind(oNode);
                return itor.pNodeObject?itor.pNodeObject->pData:NULL;
            }
        public:
            /**
             * @brief ��ÿ�ʼ������
             *  �������е������ǽڵ��ַ�ĵ�ַ
             * @param  ��
             * @return iterator
             */
            virtual iterator IterBegin() const;
            /**
             *  @brief  ���βԪ�ص�����
             *  (��ϸ˵��)
             *  @param ��
             *  @return void
             */
            virtual iterator IterLast() const;
            /**
             * @brief ����������Ϣ���������ݵĵ�����
             * 
             * @param oData       [in] ������Ϣ
             * @param itLastFound [in] ��һ���ҵ��ĵ�����(����ʱ�Ӵ˵�������һ����ʼ)�����Ϊend,���ͷ��ʼ
             * @return iterator
             */
            inline iterator IterFind(const TMdbNtcBaseObject &oData, iterator itLastFound = ms_itorEnd) const
            {
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                return IterFind(oNode, itLastFound);
            }
            /**
             * @brief ����������Ϣ������һ����������ָ��С��pData�ĵ�һ��Ԫ��
             * 
             * @param oData [in] ������Ϣ
             * @return iterator
             */
            inline iterator LowerBound(const TMdbNtcBaseObject &oData) const
            {
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                return LowerBound(oNode);
            }
            /**
             * @brief ����������Ϣ������һ����������ָ�����pData�ĵ�һ��Ԫ��
             * 
             * @param oData [in] ������Ϣ
             * @return iterator
             */
            inline iterator UpperBound(const TMdbNtcBaseObject &oData) const
            {
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                return UpperBound(oNode);
            }
            /**
             * @brief ����������Ϣ������һ����������ָ��С��pData�ĵ�һ��Ԫ��
             * 
             * @param oData [in] ������Ϣ
             * @return iterator
             */
            inline iterator LessBound(const TMdbNtcBaseObject &oData) const
            {
                return IterPrev(LowerBound(oData));
            }
            /**
             * @brief �õ����������ȵĿ�ʼ�ͽ���������
             * 
             * @param oData [in] ������Ϣ
             * @return std::pair<iterator, iterator>
             * @retval ͨ��pair��first��second�õ�lowerbound��upperbound
             */
            inline std::pair<iterator, iterator> EqualRange(const TMdbNtcBaseObject &oData) const
            {
                TNode oNode(const_cast<TMdbNtcBaseObject*>(&oData));
                return EqualRange(oNode);
            }
            /**
             * @brief �Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
             * 
             * @param itor [in] Ҫ�Ƴ��ĵ�����
             * @return iterator
             * @retval ��һ��������
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief ͨ�����������������Ԫ�غͽڵ�Ľ���
             * 
             * @param itor1 [in] ������1
             * @param itor2 [in] ������2
             * @return void
             */
            virtual void IterSwap(iterator itor1, iterator itor2) ;
        protected:
            /**
             * @brief ���ݲ���ƥ��Ľڵ㣬��ɾ���ڵ�
             * 
             * @param oNode       [in] �ڵ���Ϣ
             * @param iDelCount [in] ��Ҫ����ɾ����Ԫ�ظ���,-1��ʾɾ������ƥ���
             * @return int
             * @retval ɾ���ĸ���
             */
            int Remove(const TNode &oNode, int iDelCount = 1);
            /**
             * @brief ���ݽڵ���Ϣ�����ҵ�����
             * 
             * @param oNode       [in] �ڵ���Ϣ
             * @param itLastFound [in] ��һ���ҵ��ĵ�����(����ʱ�Ӵ˵�������һ����ʼ)�����Ϊend,���ͷ��ʼ
             * @return iterator
             */
            iterator IterFind(const TNode& oNode, iterator itLastFound = ms_itorEnd) const;
            /**
             * @brief ���ݽڵ���Ϣ������һ����������ָ��С��oNode�ĵ�һ��Ԫ��
             * 
             * @param oNode       [in] �ڵ���Ϣ
             * @return iterator
             */
            iterator LowerBound(const TNode& oNode) const;
            /**
             * @brief ���ݽڵ���Ϣ������һ����������ָ�����oNode�ĵ�һ��Ԫ��
             * 
             * @param oNode       [in] �ڵ���Ϣ
             * @return iterator
             */
            iterator UpperBound(const TNode& oNode) const;
            /**
             * @brief �õ���˽ڵ���ȵĿ�ʼ�ͽ���������
             * 
             * @param oNode [in] �ڵ���Ϣ
             * @return std::pair<iterator, iterator>
             * @retval ͨ��pair��first��second�õ�lowerbound��upperbound
             */
            std::pair<iterator, iterator> EqualRange(const TNode& oNode) const;
            /**
             * @brief ���ǰһ��������
             * 
             * @param  itCur [in] ��ǰ������
             * @param  iStep [in] ����
             * @return iterator��ǰһ��������
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief ��ú�һ��������
             * 
             * @param  itCur [in] ��ǰ������
             * @param  iStep [in] ����
             * @return iterator����һ��������
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
            /**
             * @brief ɾ�����ڵ�
             * 
             * @param pAvlNode [in] Ҫɾ���Ľڵ�
             * @return iterator
             * @retval  ��̽ڵ�ĵ�����
             */
            iterator Remove(TNode* pAvlNode);
            /**
             * @brief ����������ƽ�������
             * 
             * @param pParentNode [in] ���ڵ�
             * @return void
             * @retval 
             */
            void  ClearAvlTree(TNode * pParentNode);
            /**
             * @brief �����ݲ��뵽�����ĺ���λ��
             * 
             * @param ppParentNode   [in] �����ĸ��ڵ��ַ
             * @param pNewNode       [in] �����ݽڵ�
             * @return TNode*
             * @retval ���ڵ�ָ��
             */
            TNode* InsertNode(TNode** ppParentNode, TNode* pNewNode);
           /**
             * @brief ƽ�����ڵ�
             * 
             * @param pParentNode   [in] �����ĸ��ڵ�ָ��
             * @param iBalance      [in]  �����½ڵ�ǰ���ڵ��ƽ���
             * @return TNode*
             * @retval ƽ���ĸ��ڵ�
             */
            TNode* BalanceTree(TNode* pParentNode, int iBalance );
            /**
             * @brief �Խڵ��������
             * 
             * @param pNode [in] ��Ҫ�����ĸ��ڵ�
             * @return TNode*
             * @retval ƽ���ĸ��ڵ�
             */
            TNode* LeftRotate(TNode* pNode);
            /**
             * @brief �Խڵ��������
             * 
             * @param pNode [in] ��Ҫ�����ĸ��ڵ�
             * @return TNode*
             * @retval ƽ���ĸ��ڵ�
             */
            TNode* RightRotate(TNode* pNode);
            /**
             * @brief ���ݸ��ڵ�õ������ӽڵ�
             * 
             * @param pParentNode [in] ���ڵ�
             * @return TNode*
             * @retval ���ڵ�������ӽڵ�
             */
            TNode* GetLeftMostNode(TNode* pParentNode) const;
            /**
             * @brief ���ݸ��ڵ�õ������ӽڵ�
             * 
             * @param pParentNode [in] ���ڵ�
             * @return TNode*
             * @retval ���ڵ�������ӽڵ�
             */
            TNode* GetRightMostNode(TNode* pParentNode) const;
        protected:
            int          m_iBalanceFactor;   ///< ƽ������,���ڵ�ƽ������Ϊ [-m_iBalanceFactor, m_iBalanceFactor] ��ʱ����ƽ���
            TNode*       m_pRootNode;        ///< ƽ�����ĸ��ڵ�
            int          m_iAvlFlag;         ///< �߶�������־
            int          m_iAvlHeight;       ///< �߶�ƽ�����ĸ߶�  
            MDB_UINT32 m_uiSize;           ///< Ԫ�ص���Ŀ
            TMdbNtcObjCompare* m_pObjCompare;      ///< Ԫ���ݵıȽϺ���
        };

        /**
         * @brief ������Ϊ��ֵ��map
         * 
         */
        class TMdbNtcIntMap:public TMdbNtcAvlTree
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcIntMap);
        protected:
            /**
             * @brief �ڵ�����
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
                 * @brief �ڵ��ıȽ�
                 * 
                 * @param pNode [in] ��֮�ȽϵĽڵ�
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
             * @brief map�ĵ�����
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
                 * @brief �ṩfirst����,����õ�key
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
                 * @brief ȡ��key��Ӧ��ֵ
                 * 
                 * @return TMdbNtcBaseObject
                 * @retval ȡ��key��Ӧ��ֵ
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
             * @brief ���
             * 
             * @param iKey [in] key
             * @param pData [in] ����ָ��
             * @return iterator
             * @retval �õ����ݶ�Ӧ�ĵ�����
             */
            iterator Add(MDB_INT64 iKey, TMdbNtcBaseObject* pData);
            /**
             * @brief ����key���������ݣ����Ƴ�key
             * 
             * @param iKey     [in] key
             * @param iDelCount [in] ��Ҫ����ɾ����Ԫ�ظ���,-1��ʾɾ������ƥ���
             * @return int
             * @retval ɾ���ĸ���
             */
            inline int Remove(MDB_INT64 iKey, int iDelCount = 1)
            {                
                TNode oNode(iKey);
                return TMdbNtcAvlTree::Remove(oNode, iDelCount);
            }
            /**
             * @brief �õ�ƥ�������ָ�룬���ص�һ��ƥ��Ľڵ�
             * 
             * @param iKey    [in] �Ƚϵ�key
             * @return TMdbNtcBaseObject*
             * @retval ��NULL �������ݵ�ָ��   
             */
            inline TMdbNtcBaseObject* FindData(MDB_INT64 iKey) const
            {
                TNode oNode(iKey);
                iterator itor = TMdbNtcAvlTree::IterFind(oNode);
                return itor.pNodeObject?itor.pNodeObject->pData:NULL;
            }
        public:            
            /**
             * @brief ����������Ϣ���������ݵĵ�����
             * 
             * @param iKey       [in] key
             * @param itLastFound [in] ��һ���ҵ��ĵ�����(����ʱ�Ӵ˵�������һ����ʼ)�����Ϊend,���ͷ��ʼ
             * @return iterator
             */
            inline iterator IterFind(MDB_INT64 iKey, TMdbNtcContainer::iterator itLastFound = ms_itorEnd) const
            {
                TNode oNode(iKey);
                return TMdbNtcAvlTree::IterFind(oNode, itLastFound);
            }
            /**
             * @brief ͨ��[]�������ȡԪ��
             *
             * @param iKey    [in] �Ƚϵ�key
             * @return TMdbNtcBaseObject*
             * @retval ��NULL �������ݵ�ָ��
             */
            inline iterator operator[](MDB_INT64 iKey) const
            {
                return IterFind(iKey);
            }
            /**
             * @brief ͨ��[]�������ȡԪ��
             *
             * @param iKey    [in] �Ƚϵ�key
             * @return iterator
             */
            inline iterator operator[](MDB_INT64 iKey)
            {
                iterator itor = IterFind(iKey);
                if(itor.pNodeObject) return itor;
                else return Add(iKey, NULL);
            }
            /**
             * @brief ����������Ϣ������һ����������ָ��С��pData�ĵ�һ��Ԫ��
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
             * @brief ����������Ϣ������һ����������ָ�����pData�ĵ�һ��Ԫ��
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
             * @brief ����������Ϣ������һ����������ָ��С��pData�ĵ�һ��Ԫ��
             * 
             * @param oData [in] ������Ϣ
             * @return iterator
             */
            inline iterator LessBound(MDB_INT64 iKey) const
            {
                return IterPrev(LowerBound(iKey));
            }
            /**
             * @brief �õ����������ȵĿ�ʼ�ͽ���������
             * 
             * @param iKey [in] key
             * @return std::pair<iterator, iterator>
             * @retval ͨ��pair��first��second�õ�lowerbound��upperbound
             */
            inline std::pair<iterator, iterator> EqualRange(MDB_INT64 iKey) const
            {
                TNode oNode(iKey);
                std::pair<TMdbNtcContainer::iterator, TMdbNtcContainer::iterator> stRet = TMdbNtcAvlTree::EqualRange(oNode);
                return std::pair<iterator, iterator>(stRet.first, stRet.second);
            }
        };

        /**
         * @brief ��stringΪ��ֵ��map
         * 
         */
        class TMdbNtcStrMap:public TMdbNtcAvlTree
        {
            MDB_ZF_DECLARE_OBJECT(TMdbNtcStrMap);
        protected:
            /**
             * @brief �ڵ�����
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
                 * @brief �ڵ��ıȽ�
                 * 
                 * @param pNode [in] ��֮�ȽϵĽڵ�
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
             * @brief map�ĵ�����
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
                 * @brief �ṩfirst����,����õ�key
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
                 * @brief ȡ��key��Ӧ��ֵ
                 * 
                 * @return TMdbNtcBaseObject
                 * @retval ȡ��key��Ӧ��ֵ
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
             * @brief ���캯��
             * 
             * @param bCaseSensitive [in] �Ƿ����ִ�Сд��Ĭ�����ִ�Сд
             * @return bool
             * @retval true �ɹ�
             */
            TMdbNtcStrMap(bool bCaseSensitive = true);
            /**
             * @brief ���
             * 
             * @param sKey [in] key
             * @param pData [in] ����ָ��
             * @return iterator
             * @retval �õ����ݶ�Ӧ�ĵ�����
             */
            iterator Add(TMdbNtcStringBuffer sKey, TMdbNtcBaseObject* pData);
            /**
             * @brief ����key���������ݣ����Ƴ�key
             * 
             * @param sKey [in] key
             * @param iDelCount [in] ��Ҫ����ɾ����Ԫ�ظ���,-1��ʾɾ������ƥ���
             * @return int
             * @retval ɾ���ĸ���
             */
            inline int Remove(TMdbNtcStringBuffer sKey, int iDelCount = 1)
            {
                if(!m_bCaseSensitive) sKey.ToLower();
                TNode oNode(sKey);
                return TMdbNtcAvlTree::Remove(oNode, iDelCount);
            }
            /**
             * @brief �õ�ƥ�������ָ�룬���ص�һ��ƥ��Ľڵ�
             * (��ϸ˵��)
             * @param sKey    [in] �Ƚϵ�key
             * @return TMdbNtcBaseObject*
             * @retval ��NULL �������ݵ�ָ��     
             */
            inline TMdbNtcBaseObject* FindData(TMdbNtcStringBuffer sKey) const
            {
                return IterFind(sKey).data();
            }
        public:
            /**
             * @brief ����������Ϣ���������ݵĵ�����
             * 
             * @param sKey       [in] key
             * @param itLastFound [in] ��һ���ҵ��ĵ�����(����ʱ�Ӵ˵�������һ����ʼ)�����Ϊend,���ͷ��ʼ
             * @return iterator
             */
            inline iterator IterFind(TMdbNtcStringBuffer sKey, TMdbNtcContainer::iterator itLastFound = ms_itorEnd) const
            {
                if(!m_bCaseSensitive) sKey.ToLower();
                TNode oNode(sKey);                
                return TMdbNtcAvlTree::IterFind(oNode, itLastFound);
            }
            /**
             * @brief ͨ��[]�������ȡԪ��
             *
             * @param sKey    [in] �Ƚϵ�key
             * @return iterator
             */
            inline iterator operator[](TMdbNtcStringBuffer sKey) const
            {
                return IterFind(sKey);
            }
            /**
             * @brief ͨ��[]�������ȡԪ��
             *
             * @param sKey    [in] �Ƚϵ�key
             * @return iterator
             */
            inline iterator operator[](TMdbNtcStringBuffer sKey)
            {
                iterator itor = IterFind(sKey);
                if(itor.pNodeObject) return itor;
                else return Add(sKey, NULL);
            }
            /**
             * @brief ����������Ϣ������һ����������ָ��С��pData�ĵ�һ��Ԫ��
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
             * @brief ����������Ϣ������һ����������ָ�����pData�ĵ�һ��Ԫ��
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
             * @brief ����������Ϣ������һ����������ָ��С��pData�ĵ�һ��Ԫ��
             * 
             * @param oData [in] ������Ϣ
             * @return iterator
             */
            inline iterator LessBound(TMdbNtcStringBuffer sKey) const
            {
                return IterPrev(LowerBound(sKey));
            }
            /**
             * @brief �õ����������ȵĿ�ʼ�ͽ���������
             * 
             * @param sKey [in] key
             * @return std::pair<iterator, iterator>
             * @retval ͨ��pair��first��second�õ�lowerbound��upperbound
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
         * @brief HASH������
         * ����TBaseObject::ToHash�õ�hashֵ����
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
             * @brief ��ӡ������Ϣ
             *  
             * @param  fp [in] ��ӡ�ķ���NULL��ʾֱ����stdout���,����ͨ���ļ�ָ�����
             * @return ��
             */
            virtual void Print(FILE* fp = NULL) const;
            /**
             * @brief ��ʼ��hash��
             * 
             * @param uiTableNum [in] hash�����С
             * @return MDB_UINT32 ʵ�������С
             */
            MDB_UINT32 InitHashTable(MDB_UINT32 uiTableNum);
            /**
             * @brief �õ�Ԫ����Ŀ
             * 
             * @return MDB_UINT32
             */
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief �������ռ���ڴ�
             *      
             * @return MDB_UINT32
             * @retval �ڴ�ռ��
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief �������
             * 
             */
            virtual void Clear();
            /**
             * @brief ���Ԫ��
             * 
             * @param pData    [in] Ԫ��������Ϣ
             * @return iterator
             * @retval �ڵ�ĵ�����
             */
            iterator Add(TMdbNtcBaseObject* pData);
            /**
             * @brief ����hash key�����ݲ���ƥ���Ԫ�أ����ص�һ��ƥ�������
             * 
             * @param oData    [in] �ο��Ƚϵ�������Ϣ
             * @param oCompare [in] �ȽϷ���
             * @return TMdbNtcBaseObject*
             * @retval ��NULL �ҵ���Ӧ��Ԫ��
             */
            inline TMdbNtcBaseObject* FindData(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare) const
            {
                return IterFind(oData, g_oMdbNtcObjectCompare).data();
            }
            /**
             * @brief ����hash key������ɾ��ƥ���Ԫ��
             * 
             * @param pData     [in] �ο��Ƚϵ�������Ϣ             
             * @param iDelCount [in] ��Ҫ����ɾ����Ԫ�ظ���,-1��ʾɾ������ƥ���
             * @param oCompare  [in] �ȽϷ���
             * @return int
             * @retval ɾ���ĸ���
            */
            int Remove(const TMdbNtcBaseObject &oData, int iDelCount = 1, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare);
        public:
            /**
             * @brief ��ÿ�ʼ������
             * ����ָ��list,����ָ��data
             * 
             * @return iterator
             */
            virtual iterator IterBegin() const;
            /**
             *  @brief  ���βԪ�ص�����
             *  (��ϸ˵��)
             *  @param ��
             *  @return iterator
             */    
            virtual iterator IterLast() const;            
            /**
             * @brief �����ַ��������������ݵĵ�����
             * 
             * @param oData       [in] �ο��Ƚϵ�������Ϣ     
             * @param oCompare    [in] �ȽϷ���
             * @param itLastFound [in] ��һ���ҵ��ĵ�����(����ʱ�Ӵ˵�������һ����ʼ)�����Ϊend,���ͷ��ʼ
             * @return iterator
             */
            iterator IterFind(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare = g_oMdbNtcObjectCompare, iterator itLastFound = ms_itorEnd) const;
            /**
             * @brief �Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
             * 
             * @param itor [in] Ҫ�Ƴ��ĵ�����
             * @return iterator
             * @retval ��һ��������
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief ͨ�����������������Ԫ�غͽڵ�Ľ���
             * 
             * @param itor1 [in] ������1
             * @param itor2 [in] ������2
             */
            virtual void IterSwap(iterator itor1, iterator itor2) ;
        protected:
            /**
             * @brief ����hash key�õ�key��Ӧ��hash����
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
             * @brief ���ǰһ��������
             * 
             * @return iterator
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief ��ú�һ��������
             * 
             * @return iterator
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
        protected:
            MDB_UINT32      m_uiTableNum;   ///< hash�����С
            MDB_UINT32      m_uiSize;       ///< hashԪ����Ŀ
            TMdbNtcBaseList*  m_pHashList;    ///< ���ڴ��hash���ݵĵ�Ͱ����    
        };

        /**
         * @brief IntHash��
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
             * @brief �ַ���hashlist�нڵ��data
             * 
             */
            class TNode:public TMdbNtcBaseList::TNode
            {
            public:
                /**
                 * @brief ���캯��
                 * 
                 * @param iKeyParam  [in] key
                 * @param pDataParam [in] ������Ϣ
                 * @return ��
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
             * @brief int hash�ĵ�����
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
                 * @brief �ṩfirst����,����õ�key
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
                 * @brief ȡ��key��Ӧ��ֵ
                 * 
                 * @return TMdbNtcBaseObject
                 * @retval ȡ��key��Ӧ��ֵ
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
             * @brief ���Ԫ��
             * 
             * @param iKey  [in] hash key
             * @param pData [in] Ԫ��������Ϣ
             * @return iterator
             * @retval �ڵ�ĵ�����
             */
            iterator Add(MDB_INT64 iKey, TMdbNtcBaseObject* pData);
            /**
             * @brief ����hash key�����ݲ���ƥ���Ԫ�أ����ص�һ��ƥ�������
             * 
             * @param iKey  [in] hash key
             * @return TMdbNtcBaseObject*
             * @retval ��NULL �ҵ���Ӧ��Ԫ��
             */
            inline TMdbNtcBaseObject* FindData(MDB_INT64 iKey) const
            {
                return IterFind(iKey).data();
            }
            /**
             * @brief ����hash keyɾ��Ԫ��
             * 
             * @param iKey  [in] hash key
             * @param iDelCount [in] ��Ҫ����ɾ����Ԫ�ظ���,-1��ʾɾ������ƥ���
             * @return int
             * @retval ɾ���ĸ���
            */
            int Remove(MDB_INT64 iKey, int iDelCount = 1);
        public:
            /**
             * @brief �����ַ��������������ݵĵ�����
             * 
             * @param iKey        [in] hash key
             * @param itLastFound [in] ��һ���ҵ��ĵ�����(����ʱ�Ӵ˵�������һ����ʼ)�����Ϊend,���ͷ��ʼ
             * @return iterator
             */
            iterator IterFind(MDB_INT64 iKey, iterator itLastFound = ms_itorEnd) const;
            /**
             * @brief ͨ��[]�������ȡԪ��
             *
             * @param iKey    [in] �Ƚϵ�key
             * @return TMdbNtcBaseObject*
             * @retval ��NULL �������ݵ�ָ��
             */
            inline iterator operator[](MDB_INT64 iKey) const
            {
                return IterFind(iKey);
            }
            /**
             * @brief ͨ��[]�������ȡԪ��
             *
             * @param iKey    [in] �Ƚϵ�key
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
         * @brief StrHash��
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
             * @brief �ַ���hashlist�нڵ��data
             * 
             */
            class TNode:public TMdbNtcBaseList::TNode
            {
            public:
                /**
                 * @brief ���캯��
                 * 
                 * @param sKeyParam  [in] key
                 * @param pDataParam [in] ������Ϣ
                 * @return ��
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
             * @brief int hash�ĵ�����
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
                 * @brief �ṩfirst����,����õ�key
                 * 
                 * @return TMdbNtcStringBuffer
                 */
                TMdbNtcStringBuffer first()
                {
                    if(pNodeObject == NULL || pNodeObject->pData == NULL) return 0;
                    else return static_cast<TNode*>(pNodeObject)->sKey;
                }
                /**
                 * @brief ȡ��key��Ӧ��ֵ
                 * 
                 * @return TMdbNtcBaseObject
                 * @retval ȡ��key��Ӧ��ֵ
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
             * @brief ���캯��
             * 
             * @param bCaseSensitive [in] �Ƿ����ִ�Сд��Ĭ�����ִ�Сд
             * @param pHashFunc [in] �ַ�����ʹ�õ�hash���������ΪNULL����Ĭ��ʹ�ú���HashFunc
             * @return ��
             */
            TMdbNtcStrHash(bool bCaseSensitive = true, mdb_ntc_hash_func pHashFunc = NULL);
            /**
             * @brief ���Ԫ��
             * 
             * @param sKey [in] hash key
             * @param pData [in] Ԫ��������Ϣ
             * @return iterator
             * @retval �ڵ�ĵ�����
             */
            iterator Add(TMdbNtcStringBuffer sKey, TMdbNtcBaseObject* pData);
            /**
             * @brief ����hash key�����ݲ���ƥ���Ԫ�أ����ص�һ��ƥ�������
             * 
             * @param sKey     [in] hash key
             * @return TMdbNtcBaseObject*
             * @retval ��NULL �ҵ���Ӧ��Ԫ��
             */
            inline TMdbNtcBaseObject* FindData(TMdbNtcStringBuffer sKey) const
            {
                return IterFind(sKey).data();
            }
            /**
             * @brief ����hash keyɾ��Ԫ��
             * 
             * @param sKey [in] hash key
             * @param iDelCount [in] ��Ҫ����ɾ����Ԫ�ظ���,-1��ʾɾ������ƥ���
             * @return int
             * @retval ɾ���ĸ���
            */
            int Remove(TMdbNtcStringBuffer sKey, int iDelCount = 1);
        public:
            /**
             * @brief �����ַ��������������ݵĵ�����
             * 
             * @param sKey        [in] hash key
             * @param itLastFound [in] ��һ���ҵ��ĵ�����(����ʱ�Ӵ˵�������һ����ʼ)�����Ϊend,���ͷ��ʼ
             * @return iterator
             */
            iterator IterFind(TMdbNtcStringBuffer sKey, iterator itLastFound = ms_itorEnd) const;
            /**
             * @brief ͨ��[]�������ȡԪ��
             *
             * @param sKey    [in] �Ƚϵ�key
             * @return iterator
             */
            inline iterator operator[](TMdbNtcStringBuffer sKey) const
            {
                return IterFind(sKey);
            }
            /**
             * @brief ͨ��[]�������ȡԪ��
             *
             * @param sKey    [in] �Ƚϵ�key
             * @return iterator
             */
            inline iterator operator[](TMdbNtcStringBuffer sKey)
            {
                iterator itor = IterFind(sKey);
                if(itor.pNodeObject) return itor;
                else return Add(sKey, NULL);
            }
        protected:
            mdb_ntc_hash_func   m_pHashFunc;///< �ַ���hash����ָ��
            bool        m_bCaseSensitive;///< �Ƿ����ִ�Сд
        };

        /**
         * @brief ����(hash�����)
         * �����൱��std::map<std::string, TMdbNtcBaseObject*>����std::set<std::string>
         * ���������ڶ����ַ�����Ϊ�������������Ч�ʱȳ���mapҪ��
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
             * @brief �����ڵ���Ϣ
             * 
             */
            class TNode:public TMdbNtcBaseNode
            {
            public: 
                TNode**    ppSubNode;     ///< ��¼�ӽڵ�ļ�
                TNode*&    pParentNode;   ///< ���ڵ�,ʹ���������ͷ�����ٶ�λ���ڵ������ֵܽڵ��е����,���ϻ���ʱ��
            public:
                /**
                 * @brief ���캯��
                 * @param pParentNodeRef [in] ���ڵ��ָ������
                 * @param pData          [in] ������Ϣ  
                 * @return void
                 * @retval
                 */
                TNode(TNode*& pParentNodeRef, TMdbNtcBaseObject* pDataNode = NULL );
                /**
                 * @brief ��������
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
             * @brief ���캯��
             * 
             * @param iCharacterIndex [in] �����ַ���Ӧ���ӽڵ�����
             */
            TMdbNtcKeyTree(const int iCharacterIndex[256]);
            /**
             * @brief ��������
             * 
             */
            virtual ~TMdbNtcKeyTree();
            /**
             * @brief �õ�Ԫ����Ŀ
             * 
             * @return MDB_UINT32
             */
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief �������ռ���ڴ�
             *      
             * @return MDB_UINT32
             * @retval �ڴ�ռ��
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief �������
             * 
             */
            virtual void Clear();
            /**
             * @brief ����һ����ֵ,����Ѿ����ڼ�ֵ����֮ǰ������ɾ��,�����µ����
             * 
             * @param pszKeyName [in] �ַ���key
             * @param pData      [in] ����
             * @return iterator
             * @retval ���ݽڵ�ĵ�����
             */
            virtual iterator Add(const char* pszKeyName, TMdbNtcBaseObject* pData);
            /**
             * @brief �����ַ�������ɾ������
             * 
             * @param pszKeyName [in] �ַ���key
             * @return int
             * @retval ɾ���ĸ���
             */
            virtual int Remove(const char* pszKeyName);
            /**
             * @brief �����ַ���������������
             * 
             * @param pszKeyName [in] �ַ���key
             * @return TMdbNtcBaseObject*
             */
            TMdbNtcBaseObject* Find(const char* pszKeyName);
            /**
             * @brief ����ǰ׺��ȡƥ���Ԫ��
             * 
             * @param pszPrefix [in] �ؼ���ǰ׺
             * @param oDataList [out] ���ݴ������
             */
            void MatchPrefix(const char* pszPrefix, TMdbNtcBaseList& oDataList);
        public:
            /**
             * @brief ��ÿ�ʼ������
             * 
             * 
             * @return iterator
             */
            virtual iterator IterBegin() const;
            /**
             *  @brief  ���βԪ�ص�����
             *  (��ϸ˵��)
             *  @param ��
             *  @return iterator
             */    
            virtual iterator IterLast() const;
            /**
             * @brief �����ַ��������������ݵĵ�����
             * 
             * @param pszKeyName [in] �ַ���key
             * @return iterator
             */
            iterator IterFind(const char* pszKeyName) const;
            /**
             * @brief ͨ��[]�������ȡԪ��
             *
             * @param pszKeyName    [in] �Ƚϵ�key
             * @return iterator
             */
            inline iterator operator[](const char* pszKeyName) const
            {
                return IterFind(pszKeyName);
            }
            /**
             * @brief ͨ��[]�������ȡԪ��
             *
             * @param pszKeyName    [in] �Ƚϵ�key
             * @return iterator
             */
            inline iterator operator[](const char* pszKeyName)
            {
                iterator itor = IterFind(pszKeyName);
                if(itor.pNodeObject) return itor;
                else return Add(pszKeyName, NULL);
            }
            /**
             * @brief �����ַ�����������һ����������ָ��С�ڼ�ֵ�ĵ�һ��������
             * 
             * @param pszKeyName [in] �ַ���key
             * @return iterator
             */
            iterator LowerBound(const char* pszKeyName) const;
            /**
             * @brief �����ַ�����������һ����������ָ����ڼ�ֵ�ĵ�һ��������
             * 
             * @param pszKeyName [in] �ַ���key
             * @return iterator
             */
            iterator UpperBound(const char* pszKeyName) const;
            /**
             * @brief ����������Ϣ������һ����������ָ��С��pData�ĵ�һ��Ԫ��
             * 
             * @param oData [in] ������Ϣ
             * @return iterator
             */
            inline iterator LessBound(const char* pszKeyName) const
            {
                return IterPrev(LowerBound(pszKeyName));
            }
            /**
             * @brief ����ǰ׺��ȡƥ���Ԫ��
             * 
             * @param pszPrefix [in] �ؼ���ǰ׺
             * @return std::pair<iterator, iterator>
             * @retval lower_bounder��upper_bounder������
             */
            std::pair<iterator, iterator> MatchPrefix(const char* pszPrefix) const;
            /**
             * @brief �����ַ���������������ƥ��
             * 
             * @param pszKeyName [in] �ַ���key
             * @return iterator
             * @retval �õ�����ƥ��ĵ����������Ϊend����û���ҵ�
             */
            iterator MatchDeep(const char* pszKeyName) const;
            /**
             * @brief �Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
             * 
             * @param itor [in] Ҫ�Ƴ��ĵ�����
             * @return iterator
             * @retval ��һ��������
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief ͨ�����������������Ԫ�غͽڵ�Ľ���
             * 
             * @param itor1 [in] ������1
             * @param itor2 [in] ������2
             */
            virtual void IterSwap(iterator itor1, iterator itor2);
        protected:
            /**
             * @brief ɾ��ָ���ڵ�
             * 
             * @param pCurNode [in] ��ǰ�ڵ�
             * @return void
             * @retval ��
             */
            virtual void Remove(TMdbNtcKeyTree::TNode* pCurNode);
            /**
             * @brief ������������������������������
             * @param pParentNode [in]  ���ڵ�ָ��
             * @param pParentNode [out] ���ڵ�ָ��
             * 
             */
            void DeleteKeyTree(TNode * &pParentNode);
            /**
             * @brief ���ǰһ��������
             * 
             * @return iterator
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief ��ú�һ��������
             * 
             * @return iterator
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
            /**
              * @brief ���ұ��ڵ��ڸ��ڵ�������ӽڵ��е�λ��
              * 
              * @param pCurNode [in] ��ǰ�ڵ�
              * @return int
              * @retval ���ڵ��ڸ��ڵ�������ӽڵ��е�λ��
              */
            int FindIndex(TNode* pCurNode) const;
            /**
              * @brief ������ұ��ڵ��ڸ��ڵ�������ӽڵ��е�λ��
              * 
              * @param pCurNode [in] ��ǰ�ڵ�
              * @return int
              * @retval ���򱾽ڵ��ڸ��ڵ�������ӽڵ��е�λ��
              */
            int ReverseFindIndex(TNode* pCurNode) const;
            /**
             * @brief ������߷ǿ��ֵܽڵ�����
             * 
             * @param ppCurNode [in] ��ǰ�ڵ�ĵ�ַ
             * @return int
             * @retval �ǿ��ֵܽڵ�����
             */
            int FindSiblingPrevIndex(TNode** ppCurNode) const;
            /**
             * @brief �����ұ߷ǿ��ֵܽڵ�����
             * 
             * @param ppCurNode [in] ��ǰ�ڵ�ĵ�ַ
             * @return int
             * @retval �ǿ��ֵܽڵ�����
             */
            int FindSiblingNextIndex(TNode** ppCurNode) const;
            /**
             * @brief �����ұ߷ǿ��ֵܽڵ�������ϻ��ݵĸ��ڵ�
             * 
             * @param ppCurNode [in] ��ǰ�ڵ�
             * @return int
             * @retval �ұ߷ǿ��ֵܽڵ�������ϻ��ݵĸ��ڵ�
             */
            TNode* FindSiblingOrParentNext(TNode** ppCurNode) const;
            /**
             * @brief ɾ��ָ���ڵ�
             * 
             * @param ppCurNode [in] ��ǰ�ڵ�
             * @return void
             * @retval ��
             */
            void Remove(TMdbNtcKeyTree::TNode** ppCurNode);
            /**
             * @brief ����key���Ҷ�Ӧ�Ľڵ��ַ
             * 
             * @param pszKeyName [in] key
             * @return TNode*
             * @retval key��Ӧ�Ľڵ��ַ
             */
            TNode** FindNode(const char* pszKeyName) const;
            /**
             * @brief ��ȡ�ӽڵ�ռ���ڴ��С
             * 
             * @param pParentNode [in] ���ڵ�,���ΪNULL�����ʾ�������еĽڵ���Ŀ(����root�ڵ�)
             * @return MDB_UINT32
             * @retval �ӽڵ�ռ���ڴ��С
             */
            virtual MDB_UINT32 GetChildNodeMemoryUsage(TNode* pParentNode = NULL) const;
            /**
             * @brief ��ȡ�ӽڵ����
             * 
             * @param pParentNode [in] ���ڵ�,���ΪNULL�����ʾ�������еĽڵ���Ŀ(����root�ڵ�)
             * @return MDB_UINT32
             * @retval �ӽڵ����
             */
            virtual MDB_UINT32 GetChildNodeCount(TNode* pParentNode = NULL) const;
        protected:
            TNode*       m_pRootNode;            ///< �������ڵ�
            MDB_UINT32 m_uiTNodeCount;         ///< �������Ŀ
            MDB_UINT32 m_uiSize;               ///< ���ݽڵ����
            int    m_iCharacterIndex[256];       ///< ��ֵ���˱�
            static TNode* m_s_pNullNode;         ///< ���������ô���ʱ��ֵ
        };
        
        /**
         * @brief ���ؼ�����Ҳ����һ�������Զ��ֵ��Ӧ
         * �����൱��std::multimap<std::string, TMdbNtcBaseObject*>����std::multiset<std::string>
         * �ڵ��Ӧ�����������, ����ͨ����������ɶԶ��ֵ�ı���
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
             * @brief ���ؼ����Ľڵ�
             * 
             */
            class TNode:public TMdbNtcKeyTree::TNode
            {
            public:
                 /**
                 * @brief ���캯��
                 * @param pParentNodeRef [in] ���ڵ��ָ������
                 * @param pData [in] ������Ϣ  
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
                TNode*        pNext;            /// <ָ�������ݽڵ�
                TNode*        pPrev;            /// <ָ��ǰ�����ݽڵ�
            };
            /**
             * @brief ���캯��
             * 
             * @param iCharacterIndex [in] �����ַ���Ӧ���ӽڵ�����
             */
            TMdbNtcMultiKeyTree(const int iCharacterIndex[256]);
            /**
             * @brief ��������
             * 
             * @param [in]
             * @return int
             * @retval 0 �ɹ�
             */
            virtual ~TMdbNtcMultiKeyTree();
            /**
             * @brief �������
             * 
             */
            virtual void Clear();
            /**
             * @brief �������ռ���ڴ�
             *      
             * @return MDB_UINT32
             * @retval �ڴ�ռ��
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief ����һ����ֵ
             * 
             * @param pszKeyName [in] �ַ���key
             * @param pData      [in] ����
             * @return iterator
             * @retval ���ݽڵ�ĵ�����
             */
            virtual iterator Add(const char* pszKeyName, TMdbNtcBaseObject* pData);    
            /**
             * @brief �����ַ�������ɾ������
             * 
             * @param pszKeyName [in] �ַ���key
             * @return int
             * @retval ɾ���ĸ���
             */
            virtual int Remove(const char* pszKeyName);
            /**
             *  @brief  ���βԪ�ص�����
             *  (��ϸ˵��)
             *  @param ��
             *  @return iterator
             */    
            virtual iterator IterLast() const;
            /**
             * @brief �Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
             * 
             * @param itor [in] Ҫ�Ƴ��ĵ�����
             * @return iterator
             * @retval ��һ��������
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief �����ַ����������Ҽ�ֵ��Ӧ�Ŀ�ʼ
             * 
             * @param pszKeyName [in] �ַ���key
             * @return std::pair<iterator, iterator>
             * @retval ͨ��pair��first��second�õ�lowerbound��upperbound
             */
            std::pair<iterator, iterator> EqualRange(const char* pszKeyName) const;
        protected:
            /**
             * @brief ɾ��ָ���ڵ�
             * 
             * @param pCurNode [in] ��ǰ�ڵ�
             * @return void
             * @retval ��
             */
            virtual void Remove(TMdbNtcKeyTree::TNode* pCurNode);
            /**
             * @brief ��������������
             * 
             * @param pParentNode [in]  ���ڵ�ָ��
             * @param pParentNode [out] ���ڵ�ָ��
             */
            void DeleteKeyTree(TNode * &pParentNode);    
            /**
             * @brief ���ǰһ��������
             * 
             * @return iterator
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief ��ú�һ��������
             * 
             * @return iterator
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
            /**
             * @brief ���ͬһkey��������Ŀ
             * 
             * @param pNode [in] ���ڴ�keyλ�õ��׽ڵ�
             * @return int
             * @retval ���ݵ���Ŀ
             */
            int GetKeyValueCount(TNode* pNode) const;
            /**
             * @brief ��鵱ǰ�ڵ���ͬһkey���ظ��ڵ��еĵڼ���
             * 
             * @param pNode [in] ��ǰ�ڵ�
             * @param pHeadNode [in] �ظ��ڵ��ͷ�ڵ�
             * @return int
             * @retval ��ǰ�ڵ���ͬһkey���ظ��ڵ��еĵڼ���
             */
            int GetNodeIndex(TNode* pNode, TNode* pHeadNode = NULL) const;
            /**
             * @brief ��ȡ���pNode���������ͷ����ڸ�����ppsubnode�����е���Ӧλ�õ�Ԫ�ص�ַ
             * 
             * @param pNode [in] ��ǰ�ڵ�
             * @return TNode**
             * @retval ����ָ�룬��ʾ��ַ
             */
            TNode** GetHeadAddr(TNode* pNode) const;
            /**
             * @brief ��ȡ�ӽڵ�ռ���ڴ��С
             * 
             * @param pParentNode [in] ���ڵ�,���ΪNULL�����ʾ�������еĽڵ���Ŀ(����root�ڵ�)
             * @return MDB_UINT32
             * @retval �ӽڵ�ռ���ڴ��С
             */
            virtual MDB_UINT32 GetChildNodeMemoryUsage(TMdbNtcKeyTree::TNode* pParentNode = NULL) const;
            /**
             * @brief ��ȡ�ӽڵ����
             * 
             * @param pParentNode [in] ���ڵ�,���ΪNULL�����ʾ�������еĽڵ���Ŀ(����root�ڵ�)
             * @return MDB_UINT32
             * @retval �ӽڵ����
             */
            virtual MDB_UINT32 GetChildNodeCount(TMdbNtcKeyTree::TNode* pParentNode = NULL) const;
        };
        
        extern const int g_iMdbNtcKeyTreeDigitIndex[256];///< �ṩ�����Сд��ĸ��������д��ĸ���������ּ������ַ���Ӧ���ӽڵ�����
        /**
         * @brief ���ּ���
         * �ַ�����������
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
         * @brief �������ּ���
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
         * @brief �̰߳�ȫ����
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
             * @brief ��ֹ�������캯��
             * 
             * @param oSrcQueue [in] Դ����
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
             * @brief ���ؼ����Ľڵ�
             * 
             *
            class TNode:public TMdbNtcBaseNode
            {
            public:
                 *
                 * @brief ���캯��
                 * @param pData [in] ������Ϣ  
                 * @return void
                 * @retval 
                 *
                TNode(TMdbNtcBaseObject* pData  = NULL ):TMdbNtcBaseNode(pData)
                {          
                    pNext = NULL;
                }
            public:
                TNode*        pNext;            ///< ָ�������ݽڵ�
            };
             *
             * @brief ���캯��
             * 
             * @param bPushLock [in] ѹ��ʱ����������߳�д�����ʱ��
             * @param bPopLock  [in]  ����ʱ����������̶߳�ȡ����ʱ��
             * 
             * @return ��
             *
            TMdbNtcQueue(bool bPushLock = false, bool bPopLock = false);
            **
             * @brief ��������
             * 
             *
            virtual ~TMdbNtcQueue();
            **
             * @brief �����β��ѹ������
             * 
             * @param pData [in] Ҫѹ�������
             * @return bool
             * @retval true �ɹ���false ʧ��
             *
            bool Push(TMdbNtcBaseObject* pData);
            **
             * @brief �����β��ѹ������
             * 
             * @param itor [in] Ҫѹ��Ŀ�ʼλ�õ�����
             * @param itor_end [in] Ҫѹ��Ľ���λ�õ�������Ĭ�ϵ���β
             *
            void Push(TMdbNtcContainer::iterator itor_begin, TMdbNtcContainer::iterator itor_end = ms_itorEnd);
            **
             * @brief �Ӷ���ͷ��ȡ����
             *      
             * @return TMdbNtcBaseObject*
             * @retval ��NULL ȡ�ɹ�
             *
            TMdbNtcBaseObject* Pop();
            **
             * @brief ���ͷ�����Ϣ�����ǲ���ȡ��
             * 
             * @return TMdbNtcBaseObject*
             * @retval ��NULL ȡ�ɹ�
             *
            inline TMdbNtcBaseObject* Front()
            {
                if(m_pHeadNode == m_pTailNode) return NULL;
                else return m_pHeadNode->pNext->pData;
            }
            **
             *  @brief  �ж������Ƿ�Ϊ��
             *  (��ϸ˵��)
             *  @param ��
             *  @return ����--bool  ;����˵��--true��ʾΪ�գ�false��ʾ�ǿ�;
             *
            virtual bool IsEmpty() const
            {
                return m_pHeadNode == m_pTailNode;
            }
            **
             *  @brief  ��ȡ������Ԫ����Ŀ
             *  (��ϸ˵��)
             *  @param ��
             *  @return ����--MDB_UINT32 ;����˵��--Ԫ����Ŀ;
             *  
            virtual MDB_UINT32 GetSize() const;
            **
             * @brief �������ѹ�����Ϣ��
             * 
             * @return MDB_UINT32
             * @retval ѹ�����Ϣ��
             *
            inline MDB_UINT32 GetPushTimes() const
            {
                return m_uiPushTimes;
            }

            **
             * @brief �Ӷ�����ȡ������Ϣ��
             * 
             * @return MDB_UINT32
             * @retval ȡ������Ϣ��
             *
            inline MDB_UINT32 GetPopTimes() const
            {
                return m_uiPopTimes;
            }
            **
             * @brief ��������е�Ԫ����ռ���ڴ�Ĵ�С
             *      
             * @return MDB_UINT32
             * @retval �����е�Ԫ����ռ���ڴ�Ĵ�С
             *
            virtual MDB_UINT32 GetDataMemoryUsage() const;
            **
             * @brief �������ռ���ڴ�
             *      
             * @return MDB_UINT32
             * @retval �ڴ�ռ��
             *
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            **
             * @brief ���������Ԫ����ռ���ڴ���ܴ�С
             *      
             * @return MDB_UINT32
             * @retval ������Ԫ����ռ���ڴ���ܴ�С
             *
            using TMdbNtcContainer::GetTotalMemoryUsage;
            **
             *  @brief  �������
             *  (��ϸ˵��)
             *  @param ��
             *  @return void
             *      
            virtual void Clear();
            **
             * @brief ��ӡ������Ϣ
             *  
             * @param fp [in] ��ӡ�ķ���NULL��ʾֱ����stdout���,����ͨ���ļ�ָ�����
             * @return ��
             *
            virtual void Print(FILE* fp = NULL) const;
        protected:
            **
             * @brief ��ÿ�ʼ������
             * 
             * 
             * @return iterator
             *
            virtual iterator IterBegin() const;
            **
             *   @brief  ���βԪ�ص�����
             *   (��ϸ˵��)
             *   @param ��
             *   @return void
             *  
            virtual iterator IterLast() const;
            **
             * @brief �Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
             * 
             * @param itor [in] Ҫ�Ƴ��ĵ�����
             * @return iterator
             * @retval ��һ��������
             *
            virtual iterator IterErase(iterator itor);
            **
             * @brief ͨ�����������������Ԫ�غͽڵ�Ľ���
             * 
             * @param itor1 [in] ������1
             * @param itor2 [in] ������2
             *
            virtual void IterSwap(iterator itor1, iterator itor2) ;  
        protected:
            **
             * @brief ���ǰһ��������
             * 
             * @return iterator
             *
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            **
             * @brief ��ú�һ��������
             * 
             * @return iterator
             *
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
        protected:
            TMdbNtcThreadLock  *m_pHeadMutex;   ///< ͷ��
            TMdbNtcThreadLock  *m_pTailMutex;   ///< β��              
            TNode        *m_pHeadNode;    ///< ͷ���
            TNode        *m_pTailNode;    ///< β�ڵ�
            MDB_UINT32  m_uiPushTimes;  ///< ��¼ѹ����Ŀ
            MDB_UINT32  m_uiPopTimes;   ///< ��¼����������ѹ����Ŀ��ȥ������Ŀ���ǵ�ǰsize����������getsize����
        };

        class TThreadEvent;
        **
         * @brief ��������
         * 
         *
        class TBlockingQueue:public TMdbNtcQueue
        {
            MDB_ZF_DECLARE_OBJECT(TBlockingQueue);
        private:
            **
             * @brief ��ֹ�������캯��
             * 
             * @param oSrcQueue [in] Դ����
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
             * @brief ���캯��
             * 
             * @param bPushLock [in] ѹ��ʱ����������߳�д�����ʱ��
             * @param bPopLock  [in]  ����ʱ����������̶߳�ȡ����ʱ��
             * 
             * @return ��
             *
            TBlockingQueue(bool bPushLock = false, bool bPopLock = false);
            ~TBlockingQueue();
            **
             * @brief �жϵ�ǰ�����Ƿ���Ȼ��Ч
             * 
             * @return bool
             * @retval true ��Ч
             *
            bool IsOK();
            **
             * @brief �����β��ѹ������
             * 
             * @param pData         [in] Ҫѹ�������
             * @param iMilliSeconds [in] �ȴ���ʱʱ�䵥λ����
             * @return bool
             * @retval true �ɹ���false ʧ��
             *
            bool Push(TMdbNtcBaseObject* pData, int iMilliSeconds  = -1);
            **
             * @brief �����β��ѹ������
             * 
             * @param itor [in] Ҫѹ��Ŀ�ʼλ�õ�����
             * @param itor_end [in] Ҫѹ��Ľ���λ�õ�������Ĭ�ϵ���β
             *
            void Push(TMdbNtcContainer::iterator itor_begin, TMdbNtcContainer::iterator itor_end = ms_itorEnd);
            **
             * @brief �Ӷ���ͷ��ȡ����
             * 
             * @param iMilliSeconds [in] �ȴ���ʱʱ�䵥λ����
             * @return TMdbNtcBaseObject*
             * @retval ��NULL ȡ�ɹ�
             *
            TMdbNtcBaseObject* Pop(int iMilliSeconds = -1);
            **
             * @brief ���������Ķ���
             * 
             *
            void Wakeup();
        protected:
            TThreadEvent* m_pPopEvent;   ///֪ͨ��������Ԫ�أ�����pop
            TThreadEvent* m_pPushEvent;  ///֪ͨ�������пռ䣬����push
        };
        */

        /**
         * @brief �̰߳�ȫ����
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
             * @brief ��ֹ�������캯��
             * 
             * @param oSrcQueue [in] Դ����
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
                 * @brief ���캯��
                 * @param pData [in] ������Ϣ  
                 * @return void
                 * @retval 
                 */
                TNode(TMdbNtcBaseObject* pData  = NULL ):TMdbNtcBaseNode(pData)
                {          
                    pNext = NULL;
                }
            public:
                TNode*        pNext;            ///< ָ�������ݽڵ�
            };
            /**
             * @brief ���캯��
             * 
             * @param void
             * 
             * @return ��
             */
            TMdbNtcQueue();
            ~TMdbNtcQueue();
        public:
            /**
             * @brief �жϵ�ǰ�����Ƿ���Ȼ��Ч
             * 
             * @return bool
             * @retval true ��Ч
             */
            bool IsOK();
            /**
             * @brief ���ͷ�����Ϣ�����ǲ���ȡ��
             * 
             * @return TMdbNtcBaseObject*
             * @retval ��NULL ȡ�ɹ�
             */
            inline TMdbNtcBaseObject* Front()
            {
                if(m_pHeadNode == m_pTailNode) return NULL;
                else return m_pHeadNode->pNext->pData;
            }
            /**
             * @brief �������ѹ�����Ϣ��
             * 
             * @return MDB_UINT32
             * @retval ѹ�����Ϣ��
             */
            inline MDB_UINT64 GetPushTimes() const
            {
                return m_uiPushTimes;
            }

            /**
             * @brief �Ӷ�����ȡ������Ϣ��
             * 
             * @return MDB_UINT32
             * @retval ȡ������Ϣ��
             */
            inline MDB_UINT64 GetPopTimes() const
            {
                return m_uiPopTimes;
            }
            /**
             * @brief �����β��ѹ������
             * 
             * @param pData         [in] Ҫѹ�������
             * @param iMilliSeconds [in] �ȴ���ʱʱ�䵥λ����
             * @return bool
             * @retval true �ɹ���false ʧ��
             */
            bool Push(TMdbNtcBaseObject* pData, int iMilliSeconds  = -1);
            /**
             * @brief �Ӷ���ͷ��ȡ����
             * 
             * @param iMilliSeconds [in] �ȴ���ʱʱ�䵥λ����
             * @return TMdbNtcBaseObject*
             * @retval ��NULL ȡ�ɹ�
             */
            TMdbNtcBaseObject* Pop(int iMilliSeconds = -1);
            /**
             * @brief ���������Ķ���
             * 
             */
            void Wakeup();
        public:
            /**
             *  @brief  �ж������Ƿ�Ϊ��
             *  (��ϸ˵��)
             *  @param ��
             *  @return ����--bool  ;����˵��--true��ʾΪ�գ�false��ʾ�ǿ�;
             */
            virtual bool IsEmpty() const
            {
                return m_uiSize == 0;
            }
            /**
             *  @brief  ��ȡ������Ԫ����Ŀ
             *  (��ϸ˵��)
             *  @param ��
             *  @return ����--MDB_UINT32 ;����˵��--Ԫ����Ŀ;
             */    
            virtual MDB_UINT32 GetSize() const
            {
                return m_uiSize;
            }
            /**
             * @brief ��������е�Ԫ����ռ���ڴ�Ĵ�С
             *      
             * @return MDB_UINT32
             * @retval �����е�Ԫ����ռ���ڴ�Ĵ�С
             */
            virtual MDB_UINT32 GetDataMemoryUsage() const;
            /**
             * @brief �������ռ���ڴ�
             *      
             * @return MDB_UINT32
             * @retval �ڴ�ռ��
             */
            virtual MDB_UINT32 GetContainerMemoryUsage() const;
            /**
             * @brief ���������Ԫ����ռ���ڴ���ܴ�С
             *      
             * @return MDB_UINT32
             * @retval ������Ԫ����ռ���ڴ���ܴ�С
             */
            using TMdbNtcContainer::GetTotalMemoryUsage;
            /**
             *  @brief  �������
             *  (��ϸ˵��)
             *  @param ��
             *  @return void
             */        
            virtual void Clear();
            /**
             * @brief ��ӡ������Ϣ
             *  
             * @param fp [in] ��ӡ�ķ���NULL��ʾֱ����stdout���,����ͨ���ļ�ָ�����
             * @return ��
             */
            virtual void Print(FILE* fp = NULL) const;
        protected:
            /**
             * @brief ��ÿ�ʼ������
             * 
             * 
             * @return iterator
             */
            virtual iterator IterBegin() const;
            /**
             *   @brief  ���βԪ�ص�����
             *   (��ϸ˵��)
             *   @param ��
             *   @return void
             */    
            virtual iterator IterLast() const;
            /**
             * @brief �Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
             * 
             * @param itor [in] Ҫ�Ƴ��ĵ�����
             * @return iterator
             * @retval ��һ��������
             */
            virtual iterator IterErase(iterator itor);
            /**
             * @brief ͨ�����������������Ԫ�غͽڵ�Ľ���
             * 
             * @param itor1 [in] ������1
             * @param itor2 [in] ������2
             */
            virtual void IterSwap(iterator itor1, iterator itor2) ;
        protected:
            /**
             * @brief ���ǰһ��������
             * 
             * @return iterator
             */
            virtual iterator IterPrev(iterator itCur, int iStep = -1) const;
            /**
             * @brief ��ú�һ��������
             * 
             * @return iterator
             */
            virtual iterator IterNext(iterator itCur, int iStep = 1) const;
        protected:
            TNode           *m_pHeadNode;   ///< ͷ���
            TNode           *m_pTailNode;   ///< β�ڵ�   
            MDB_UINT32           m_uiSize;      ///< ���ݸ��� 
            MDB_UINT64           m_uiPushTimes; ///< push����
            MDB_UINT64           m_uiPopTimes;  ///< push����
            TMdbNtcThreadLock      *m_pMutex;     ///< �߳��� 
            TMdbNtcThreadCond      *m_pPopCond;   ///֪ͨ��������Ԫ�أ�����pop
            TMdbNtcThreadCond      *m_pPushCond;  ///֪ͨ�������пռ䣬����push
            
        };
//}

#endif










