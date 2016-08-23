#include "Common/mdbDataStructs.h"
#include "Common/mdbSysLocks.h"
#include "Common/mdbSysThreads.h"
#include <math.h>

//#include "Sdk/mdbMemoryLeakDetectInterface.h"

//namespace QuickMDB
//{
        #define  MDB_NTC_MAX_QUENE_SIZE  1000000    //������󳤶�
        
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcObjCompare, TMdbNtcBaseObject);
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcPointerCompare, TMdbNtcObjCompare);
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcNoCaseCompare, TMdbNtcObjCompare);
        TMdbNtcObjCompare     g_oMdbNtcObjectCompare;
        TMdbNtcPointerCompare g_oMdbNtcPointerCompare;
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        //&&&&                                                              &&&&&&
        //&&&&                        �㷨ʵ�ֲ���                          &&&&&&
        //&&&&                                                              &&&&&&
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        
        //���ֲ�������
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
        //��������TQuickSort����
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcQuickSort, TMdbNtcSort);
        //���������㷨
        void TMdbNtcQuickSort::Sort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcObjCompare &oCompare) const
        {
            if ( uiDataSize > 1 )
            {
                QSort(pDataHead, 0, uiDataSize-1, oCompare);   
            }     
        }
        
        //�ݹ��������,������������������¼һ�������У�low��high���������м��ƶ��Ĺ������Ƿ��������������δ��������������
        //�������Ӧ���ӱ��������,����߱����ݻ�������(������)ʱ���㷨����
        void TMdbNtcQuickSort::QSort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiLow, MDB_UINT32 uiHigh,const TMdbNtcObjCompare &oCompare) const
        {
            if (uiLow < uiHigh)
            {
                bool bIsExchangeLow  = false;//һ�������У�lowָ�����ƹ������Ƿ��������ݽ���
                bool bIsExchangeHigh = false;//һ�������У�highָ�����ƹ������Ƿ��������ݽ���
        
                //һ�˿������򣬽���һ��Ϊ����uiPivoLoc������λ��
                unsigned int uiPivoLoc = Partion(pDataHead, uiLow, uiHigh, oCompare, bIsExchangeLow, bIsExchangeHigh);
                if (bIsExchangeLow)
                {
                    //�Ե��ӱ���еݹ�����
                    QSort(pDataHead, uiLow, uiPivoLoc - 1, oCompare);
                }
                if (bIsExchangeHigh)
                {
                    //�Ը��ӱ���еݹ�����
                    QSort(pDataHead, uiPivoLoc + 1, uiHigh, oCompare);
                }
                
            }
        }
        
        //һ������ѡ����ʵ�ֵ��Ϊ����������㷨���ܣ����ƶ�low��high�Ĺ��̵�ͬʱ�������ݲ������������Ƿ���������ʶ�����Ը��������µ�����
        MDB_UINT32 TMdbNtcQuickSort::Partion(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiLow, MDB_UINT32 uiHigh,const TMdbNtcObjCompare &oCompare, bool &bIsExchangeLow,bool &bIsExchangeHigh) const
        {
            unsigned int uiTempLow = uiLow;
            unsigned int uiTempHigh = uiHigh;
        
            //ѡ��uiLow,uiHigh,(uiLow+uiHigh)/2������λ�ñȽϹؼ�ֵ��ȡ��������ֵ�ļ�¼Ϊ����
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
        
            //����һ����¼��Ϊ����
            TMdbNtcBaseObject * pPivoKey = pDataHead[uiLow];
        
            //���һ������
            //����
            if (m_bSortAsc)
            {
                while (uiLow < uiHigh)
                {
                    //�߶˼�¼����
                    while (uiLow < uiHigh && oCompare.Compare(pDataHead[uiHigh], pPivoKey) >= 0)
                    {
                        //ͬʱ����ð������
                        if (oCompare.Compare(pDataHead[uiHigh], pDataHead[uiHigh-1]) < 0)
                        {
                            Swap(pDataHead, uiHigh, uiHigh-1);
                            bIsExchangeHigh = true;
                        }
                        --uiHigh;
                    }
                    //�������¼����
                    Swap(pDataHead, uiLow, uiHigh);
                    //���������uiLow��¼С��ǰһ����¼��������ԵͶ��ӱ��������
                    if (!bIsExchangeLow && uiLow > uiTempLow && oCompare.Compare(pDataHead[uiLow],pDataHead[uiLow-1]) < 0)
                    {
                        bIsExchangeLow = true;
                    }
        
                    //�Ͷ˼�¼����
                    while (uiLow < uiHigh && oCompare.Compare(pDataHead[uiLow], pPivoKey) <= 0)
                    {
                        //ͬʱ����ð������
                        if (oCompare.Compare(pDataHead[uiLow], pDataHead[uiLow+1]) > 0)
                        {
                            Swap(pDataHead, uiLow, uiLow+1);
                            bIsExchangeLow = true;
                        }
        
                        ++uiLow;
                    }
                    //�������¼����
                    Swap(pDataHead, uiLow, uiHigh);
                    //���������uiHigh��¼���ں�һ����¼��������Ը߶��ӱ��������
                    if (!bIsExchangeHigh && uiHigh < uiTempHigh && oCompare.Compare(pDataHead[uiHigh],pDataHead[uiHigh+1]) > 0)
                    {
                        bIsExchangeHigh = true;
                    }
                }
            } 
            //����
            else
            {
                while (uiLow < uiHigh)
                {
                    //�߶˼�¼����
                    while (uiLow < uiHigh && oCompare.Compare(pDataHead[uiHigh], pPivoKey) <= 0)
                    {
                        //ͬʱ����ð������
                        if (oCompare.Compare(pDataHead[uiHigh], pDataHead[uiHigh-1]) > 0)
                        {
                            Swap(pDataHead, uiHigh, uiHigh-1);
                            bIsExchangeHigh = true;
                        }
                        --uiHigh;
                    }
                    //�������¼����
                    Swap(pDataHead, uiLow, uiHigh);
                    //���������uiLow��¼����ǰһ����¼��������ԵͶ��ӱ��������
                    if (!bIsExchangeLow && uiLow > uiTempLow && oCompare.Compare(pDataHead[uiLow],pDataHead[uiLow-1]) > 0)
                    {
                        bIsExchangeLow = true;
                    }
        
                    //�Ͷ˼�¼����
                    while (uiLow < uiHigh && oCompare.Compare(pDataHead[uiLow], pPivoKey) >= 0)
                    {
                        //ͬʱ����ð������
                        if (oCompare.Compare(pDataHead[uiLow], pDataHead[uiLow+1]) < 0)
                        {
                            Swap(pDataHead, uiLow, uiLow+1);
                            bIsExchangeLow = true;
                        }
        
                        ++uiLow;
                    }
                    //�������¼����
                    Swap(pDataHead, uiLow, uiHigh);
                    //���������uiHigh��¼С�ں�һ����¼��������Ը߶��ӱ��������
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
        //������THeapSort����
        //
        //////////////////////////////////////////////////////////////////////////
        //�����߼���
        //���򣺽��ɴ󶥶ѣ���Ѷ�Ԫ�������Ԫ�أ����Ѷ�Ԫ�������һ��Ԫ�ؽ���λ�ã�
        //      ��ô���Ԫ�ؾ��ƶ��������������е�Ŀ��λ�á����½��ɴ󶥶ѣ����һ
        //      ��Ԫ�ز�����˴ν��ѡ��ظ����������������һ�ν������ʱ������Ԫ��
        //      �������������У�
        //���򣺽���С���ѣ���Ѷ�Ԫ������СԪ�أ����Ѷ�Ԫ�������һ��Ԫ�ؽ���λ�ã�
        //      ��ô��СԪ�ؾ��ƶ������½������е�Ŀ��λ�á����½���С���ѣ����һ
        //      ��Ԫ�ز�����˴ν��ѡ��ظ����������������һ�ν������ʱ������Ԫ��
        //      ���½��������У�
        
        //���մ���������׵�ַ����
        void TMdbNtcHeapSort::Sort(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiDataSize, const TMdbNtcObjCompare &oCompare) const
        {
            if ( uiDataSize <= 1)
            {
                return;
            }
        
            int i = 0;
            //��ȫ������Ԫ�ؽ��ɴ󶥶ѻ���С����
            for (i = (int)(uiDataSize/2 -1); i >= 0; --i)
            {
                HeapAdjust(pDataHead, (MDB_UINT32)i,uiDataSize-1,oCompare);
            }
        
            for (i = (int)uiDataSize-1;i > 0; --i)
            {
                //���Ѷ�Ԫ�غ����һ��Ԫ�ؽ���
                Swap(pDataHead, 0, (MDB_UINT32)i);
                //��Ԫ��0~i-1���µ���Ϊ�󶥶ѻ���С����
                HeapAdjust(pDataHead, 0, (MDB_UINT32)(i-1),oCompare);
            }
        }
        
        //������
        void TMdbNtcHeapSort::HeapAdjust(TMdbNtcBaseObject** pDataHead, MDB_UINT32 uiStartLoc, MDB_UINT32 uiMaxLoc, const TMdbNtcObjCompare &oCompare) const
        {
            TMdbNtcBaseObject * pTempObj = pDataHead[uiStartLoc];
        
            //���򣬵����ɴ󶥶�
            if (m_bSortAsc)
            {
                for ( MDB_UINT32 j = 2*uiStartLoc+1; j <= uiMaxLoc; j = 2*j+1)
                {
                    //�����Һ����нϴ�ıȽϴ�С�ͽ���
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
            //���򣬵�����С����
            else
            {
                for ( MDB_UINT32 j = 2*uiStartLoc+1; j <= uiMaxLoc; j = 2*j+1)
                {
                    //�����Һ����н�С�ıȽϴ�С�ͽ���
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
        //&&&&                    ���ݽṹʵ�ֲ���                          &&&&&&
        //&&&&                                                              &&&&&&
        //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
        //���ݼ�ֵ��������
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
        //��������TContainer����
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
        //��ӡ������Ϣ
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
        
        //�������ݲ��ҵ�����
        TMdbNtcContainer::iterator TMdbNtcContainer::IterFind(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare,
                                                  iterator itLastFound /*= ms_itorEnd*/) const
        {
            if(itLastFound.ppObject == NULL) itLastFound = IterBegin();
            else ++itLastFound;
            return ::IterFind(itLastFound, IterEnd(), oData, oCompare);
        }
        
        //�ӿ�ʼ���������ҵ�����������������ƥ���Ԫ����
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
        //��̬����TAutoArray����
        //
        //////////////////////////////////////////////////////////////////////////
        
        //ע�⣺��̬���鴴����ʱ������Ԥ���ռ䡢����ʱ�����ռ��Լ�ɾ��Ԫ�صĿռ�ȫ��
        //��ֵ����ΪNULL����ô�ڶ��������ĳЩ������ʱ����Ҫ�����Ƿ���Ҫ�ظ���NULL
        
        //���캯��
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcAutoArray, TMdbNtcContainer);
        TMdbNtcAutoArray::TMdbNtcAutoArray(MDB_UINT32 uiGrowBy /* = 0 */)
        {
            //������ʼ��
            m_uiGrowBy = uiGrowBy;
            m_pData = NULL;
            m_uiSize = 0;
            m_uiCapacity = 0;
        
        }
        
        //��������������ʼ���½�����
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
                //Ԥ��
                Reserve(uiSize);
                if(oSrcContainer.IsKindOf(MDB_ZF_RUNTIME_OBJECT(TMdbNtcAutoArray)))
                {
                    memcpy(m_pData, static_cast<const TMdbNtcAutoArray*>(&oSrcContainer)->m_pData, sizeof(TMdbNtcBaseObject*)*uiSize);
                }
                else
                {
                    //��Ϊ����������������Ͳ�ȷ����������ʹ�õ���������
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
        
        //��������
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
        //�������
        void TMdbNtcAutoArray::Clear()
        {    
            if(m_pData && m_uiSize > 0 )
            {
                MdbNtcReleaseArray(m_pData, 0, m_uiSize);
                m_uiSize = 0;
            }
        }
        
        //Ԥ�������С
        void TMdbNtcAutoArray::Reserve(MDB_UINT32 uiCapacity)
        {
            //Ԥ��С�ڵ�����������������
            if (m_uiCapacity >= uiCapacity )
            {
                return;
            }
            //Ԥ��������������������δ����������
            else if(NULL == m_pData )
            {
                m_pData = new TMdbNtcBaseObject*[uiCapacity];
                memset(m_pData, 0x00, uiCapacity *sizeof(TMdbNtcBaseObject* ));
                m_uiCapacity = uiCapacity;
            }
            //Ԥ���������������������Ѵ���������
            else
            {
                //����������׵�ַ
                TMdbNtcBaseObject ** pPre = m_pData;
                //����������
                m_pData = new TMdbNtcBaseObject*[uiCapacity];
                //������Ԫ��
                if (m_uiSize > 0)
                {
                    memcpy(m_pData, pPre, m_uiSize*sizeof(TMdbNtcBaseObject*));
                }
                
                //����δʹ�õ�Ԥ���ռ�
                memset(m_pData + m_uiSize , 0x00 ,(uiCapacity-m_uiSize)*sizeof(TMdbNtcBaseObject*));
        
                m_uiCapacity = uiCapacity;
                
                //�ͷž�����ռ�
                delete[] pPre;
            }
           
        }
        
        //��������Ĵ�С,����Ԫ����Ŀ�����仯
        void TMdbNtcAutoArray::SetSize(MDB_UINT32 uiNewSize)
        {
            //����Ϊ��
            if (NULL == m_pData )
            {
                //��sizeΪ0��ֱ�ӷ���
                if(0 == uiNewSize )
                {
                    return;
                }
                //���ݼȶ����Ի�ȡ���������Ĵ�С
                unsigned int uiAllocSize = (uiNewSize > m_uiGrowBy )? uiNewSize:m_uiGrowBy;
                m_pData = new TMdbNtcBaseObject*[uiAllocSize];
                //ʹ��iAllocSize������uiNewSize,����Ϊʹ��uiNewSize���ܵ���δʹ�õ���Ԥ���ռ�δ����ʼ��
                memset(m_pData, 0x00, uiAllocSize * sizeof(TMdbNtcBaseObject * ));
                m_uiSize = uiNewSize;
                m_uiCapacity = uiAllocSize;
            }
            //�����ǿգ���sizeС�ڵ�����������
            else if (uiNewSize <= m_uiCapacity)
            {
                //��������size���ֵľ�Ԫ����Ϊ�գ�iNewSize>m_uiSize�����������δʹ�ÿռ��Ѿ�����ΪNULL�������ظ�����
                if (m_uiSize > uiNewSize)
                {
                    MdbNtcReleaseArray(m_pData, uiNewSize, m_uiSize);
                }
                m_uiSize = uiNewSize;
            } 
            //��size���������������򿪱��¿ռ�
            else
            {
                unsigned int uiNewCapacity = 0;      //��������С
                unsigned int uiGrowBy  =  m_uiGrowBy;
                //m_iGrowBy����0�����üȶ���������m_iGrowBy�Ĵ�С
                if(0 ==uiGrowBy)
                {
                    uiGrowBy = m_uiSize/8;
                    uiGrowBy = (uiGrowBy < 4)? 4:((uiGrowBy >1024 ) ? 1024:uiGrowBy );           
                }
                //�����µ�������С
                if (uiNewSize < m_uiCapacity + uiGrowBy)
                {
                    uiNewCapacity = m_uiCapacity + uiGrowBy;
                }
                else 
                {
                    uiNewCapacity = uiNewSize;
                }
                //Ԥ��
                Reserve(uiNewCapacity);
                m_uiSize = uiNewSize;  
            }
        }
        
        //�õ�ָ��Ԫ�ص��±�
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
        
        //�����Ԫ��,�����±�
        int TMdbNtcAutoArray::Add(TMdbNtcBaseObject* pNewObj)
        {
            int iIndex = 0;
        
            iIndex = (int)m_uiSize;
        
            //������С��1
            SetSize(m_uiSize+1);
            
            //�����Ԫ��
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
        
        //������Ԫ��
        MDB_UINT32 TMdbNtcAutoArray::Insert(MDB_UINT32 uiIndex, TMdbNtcBaseObject* pNewObj)
        { 
            //���ָ�����±���ڵ���������С������Ԫ�ز��뵽β��,�÷�֧������m_uiSize����0���Լ�m_pData����NULL�����
            if (uiIndex >= m_uiSize)
            {
                uiIndex = m_uiSize;
                SetSize(m_uiSize+1);
            }
            else
            {
                SetSize(m_uiSize+1);
                //ָ���±꼰�����Ԫ��ȫ������ƶ�һ��λ��
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
        
        //����ɾ��ָ��������Ԫ��
        int TMdbNtcAutoArray::Remove(MDB_UINT32 uiIndex,MDB_UINT32 uiDelCount /* = 1 */)
        {            
            int iRetDelCount = 0;
            //���ָ�����±���ڵ���������С������ɾ��Ԫ�صĸ���Ϊ0����ɾ��ʧ��ֱ�ӷ���0(ע�⣺������ѽ�����Ϊ�յ������������)
            if ((uiIndex >= m_uiSize) || (0 == uiDelCount))
            {
                return 0;
            } 
            else
            {        
                //���ɾ��������β����ֱ�ӽ�ɾ�����ֵ�Ԫ����ΪNULL����
                if (uiDelCount >= m_uiSize - uiIndex )
                {
                    iRetDelCount = (int)(m_uiSize - uiIndex);
                    MdbNtcReleaseArray(m_pData, uiIndex, m_uiSize);
                }
                else
                {
                    iRetDelCount = (int)uiDelCount;
                    MdbNtcReleaseArray(m_pData, uiIndex, uiIndex+uiDelCount);
                    //Ԫ��ǰ�Ƹ��ǵ�Ҫɾ����Ԫ��
                    memmove(m_pData+uiIndex,m_pData+(uiIndex+(MDB_UINT32)iRetDelCount),(m_uiSize-uiIndex-(MDB_UINT32)iRetDelCount)*sizeof(TMdbNtcBaseObject*));
                    //���뽫�ƶ�Ԫ�غ�ճ��Ŀռ��ÿ�
                    memset(m_pData+m_uiSize-iRetDelCount, 0x00, (MDB_UINT32)iRetDelCount*sizeof(TMdbNtcBaseObject*));
                }        
                //�޸�������С
                m_uiSize -= (MDB_UINT32)(iRetDelCount);
            }
        
            return iRetDelCount;
        }
        
        //�����������Ƴ�ƥ��Ľڵ�  
        int TMdbNtcAutoArray::Remove(const TMdbNtcBaseObject &oData, int iDelCount /* = 1 */, const TMdbNtcObjCompare &oCompare /* = g_oMdbNtcObjectCompare */)
        {
            int iRetDelCount = 0;            //��ʱ������ʵ��ɾ����Ԫ�ظ���
            int iDestIndex = -1;             //��ʱ������ָ�����ݵ��±�
            unsigned int uiCurIndex  =  0;   //��ʱ��������ǰ��ʼ�������±�
            int iTempDelCount = iDelCount;
        
            ++iDelCount;                     //�ȼ�1��Ϊ��У��ѭ�����ȶԸñ�����--������ƫ��
        
            //��������ɾ��ƥ���Ԫ��
            while (uiCurIndex <  m_uiSize )
            {
                //����ɾ��ƥ��Ԫ�صĸ��������ΪuiDelCountΪ-1����ɾ������ƥ���Ԫ��
                if ((iTempDelCount != -1 ) && --iDelCount < 1 )
                {
                    break;
                }
        
                iDestIndex = FindIndex(oData,oCompare,uiCurIndex);
                
                //���ҳɹ������Ƴ���Ԫ��
                if (iDestIndex >= 0)
                {
                    
                    
                    Remove((MDB_UINT32)iDestIndex);
                    ++iRetDelCount;
                    uiCurIndex = (MDB_UINT32)iDestIndex;
                }
                else
                {
                    //����ʧ��ֱ���˳�
                    break;
                }               
            }
           
            return iRetDelCount;
        }
        
        //�ϲ�����
        int TMdbNtcAutoArray::Combine(int iDestIndex, TMdbNtcContainer* pSrcContainer, int iSrcStart /* = 0 */,int iSrcCount /* = -1 */)
        {
            int iIndex = -1;
            bool bIsOffset = true;                          //�Ƿ���Ҫ����Ԫ��
        
            MDB_UINT32 uiSize = pSrcContainer->GetSize();  //��ȡԴ�����Ĵ�С
            
            //Դ����Ϊ�գ�ֱ���˳�
            if (0 == uiSize)
            {
                return iIndex;
            }
        
            //ʵ�ι���--Ŀ�꿪ʼλ�ù���������Ϊβ����������Ʋ���
            if (iDestIndex<= -1 || ((MDB_UINT32)iDestIndex) >= m_uiSize )
            { 
                iDestIndex = (int)m_uiSize;
                bIsOffset = false;
            }
            //ʵ�ι���--Դ���ݿ�ʼλ�ù���������Ϊͷ��
            if (iSrcStart < 0)
            {
                iSrcStart = 0;
            }
            //ʵ�ι���--Դ���ݿ�����Ŀ����������Ϊ��ʼԪ���������Ԫ�ظ���
            if (iSrcCount <= -1)
            {
                iSrcCount = (int)uiSize-iSrcStart;
            }
        
            //���Դ����Ҫ��������Ŀ��������С
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
                MDB_UINT32 iTempDestIndex = (MDB_UINT32)iDestIndex;  //��ʱ����������Ŀ��λ�ã���Ϊ��������
         
                //����Ŀ�������С
                SetSize(m_uiSize+(MDB_UINT32)iSrcCount);
        
                //������뵽Ŀ�������β��������Ŀ������Ϊ�գ�����Ҫ����Ԫ��
                if (bIsOffset)
                {
                    //Ԫ�غ���
                    memmove(m_pData+iDestIndex+iSrcCount, m_pData+iDestIndex,(MDB_UINT32)iSrcCount*sizeof(TMdbNtcBaseObject*));
        
                }
         
                //����Ԫ��
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
        
        //�Ƴ��ظ���Ԫ��
        int TMdbNtcAutoArray::RemoveDup(const TMdbNtcObjCompare &oCompare)
        {
            
            int iCount = 0;
            //��Ԫ�ظ�������1���Ž����Ƴ��ظ�Ԫ�صĲ���
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
        
        //��ÿ�ʼ������
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterBegin() const
        {
            return iterator(this, m_uiSize==0 ? NULL:m_pData);
        }
        
        //���βԪ�ص�����
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterLast() const
        {
            return iterator(this, m_uiSize == 0? NULL:(m_pData+m_uiSize-1));
        }
        
        //�Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
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
            //���index
            MDB_UINT32 iStart = (MDB_UINT32)(itorBegin.ppObject-m_pData);    
            Remove(iStart, itorEnd.ppObject?((MDB_UINT32)(itorEnd.ppObject-m_pData)):(MDB_UINT32)-1);    
            if(m_uiSize > iStart) 
				   itorRet.ppObject = m_pData+iStart;
            else
				  itorRet.ppObject = NULL;
            return itorRet;
        }
        
        //ͨ�����������������Ԫ�غͽڵ�Ľ���
        void TMdbNtcAutoArray::IterSwap(TMdbNtcAutoArray::iterator itor1, TMdbNtcAutoArray::iterator itor2)
        {
            if (itor1.ppObject && itor2.ppObject)
            {
                 TMdbNtcBaseObject * pTemp = *(itor1.ppObject);
                 *(itor1.ppObject) = *(itor2.ppObject);
                 *(itor2.ppObject) = pTemp;
            } 
        }

        //ͨ����������λ��������
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
                //����ǽ���������������û���ҵ�λ�ã�����ӵ���̬����β��
                Add(pData);
                return iterator(this,m_pData+m_uiSize-1);
            }
            
        }
        
        //��ȡǰһ��������
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterPrev(iterator itCur, int iStep /* = -1 */) const
        {
            if(m_pData == NULL) 
            {
                return IterEnd();
            }
        
            int iIndex = -1;   //��ǰ��������Ӧ���±�
        
            //��ƫ�ƵĲ�����������ķ�Χ����ȡ��һ������
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
        
        //��ȡ��һ��������
        TMdbNtcAutoArray::iterator TMdbNtcAutoArray::IterNext(iterator itCur, int iStep /* = 1 */) const
        {
            if (NULL == m_pData )
            {
                return IterEnd();
            }
            
            int iIndex = -1; //��ǰ��������Ӧ���±�
           
            //��ƫ�ƵĲ�����������ķ�Χ����ȡ���һ������
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
        //ջTStack����
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcStack, TMdbNtcAutoArray);
        //���캯��
        TMdbNtcStack::TMdbNtcStack (MDB_UINT32 uiStackCapacity)
        {
            Reserve(uiStackCapacity);
        }
        
        //��ջ
        int TMdbNtcStack::Push (TMdbNtcBaseObject *pData)
        {
            //ջ�����ܾ���ջ
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
        
        //��ջ
        TMdbNtcBaseObject * TMdbNtcStack::Pop ()
        {  
            //ջ�գ�����NULL
            if(0 == m_uiSize)
            {
                return NULL;
            }
            else
            {
                TMdbNtcBaseObject * pRet ;
                pRet = m_pData[m_uiSize-1];        
                //ɾ��ջ��Ԫ��
                m_pData[m_uiSize-1] = NULL;
                //ջ��λ����1
                --m_uiSize;
                return pRet;
            }
        
        }
        
        //��ȡջ��Ԫ��
        TMdbNtcBaseObject * TMdbNtcStack::Top () const
        {
               
            //ջ�գ�����NULL
          
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
        //����TBaseList::TNode����
        //
        //////////////////////////////////////////////////////////////////////////
        //���캯��
        TMdbNtcBaseList::TNode::TNode(TMdbNtcBaseObject *pData /* = NULL */)
        {
            this->pData = pData;
            pPrev = NULL;
            pNext = NULL;
        }
        
        //////////////////////////////////////////////////////////////////////////
        //
        //˫������TBaseList����
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcBaseList, TMdbNtcContainer);
        //ȱʡ���캯��
        TMdbNtcBaseList::TMdbNtcBaseList()
        {
            //����ͷ�ڵ�
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
        
        //�������
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
        
        //��ӽڵ㵽����ͷ��
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::AddHead(TMdbNtcBaseObject* pData)
        {
            //�����ڵ�
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
        
        //��β����ӽڵ�
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
        
        //��ӵ��ο��ڵ��ǰ�棬pNodeΪȷ�����������д����еĽڵ�
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
        
        //��ӽڵ㵽�ο��ڵ�ĺ��棬pNodeΪȷ�����������д����еĽڵ�
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
        
        //��ָ��λ�ò���ڵ�
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
        
        //ɾ��ͷ���
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::RemoveHead()
        {
            //�ǿ�����
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
        
        //ɾ��β�ڵ�
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
        
        //����ɾ�����ɸ�ָ��λ�����Ԫ�أ�ɾ��������Ч���ݽڵ�
        int TMdbNtcBaseList::Remove(int iIndex, int iDelCount /* = 1 */)
        {
            //�Ϸ����ж�
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
                if(pCurNode == NULL)//������Ľڵ��Ѿ�ΪNULL������Ҫ�޸�Tailָ��
                {
                    m_pTailNode = pPrevNode;
                }
            }
            m_uiSize -= (MDB_UINT32)(iRetDelCount);
            return iRetDelCount;
        }
        
        //�Ƴ�ƥ��Ľڵ�
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
        
        //�����������Ƴ�ƥ��Ľڵ�
        int TMdbNtcBaseList::Remove(const TMdbNtcBaseObject &oData, int iDelCount /* = 1 */, const TMdbNtcObjCompare &oCompare /* = g_oMdbNtcObjectCompare */)
        {
            int iRetDelCount = 0;             //��ʱ������ʵ��ɾ����Ԫ�ظ���
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
        
        //������Ų��ҽ��
        TMdbNtcBaseList::TNode* TMdbNtcBaseList::GetAt(MDB_UINT32 uiIndex) const
        {
            //ʵ�κϷ����ж�
            if (uiIndex >= m_uiSize)
            {
                return NULL;
            }
            TNode * pDestNode = m_pHeadNode;
            //��λ
            MDB_UINT32 i = 0;
            while ( pDestNode && i < uiIndex)
            {
                pDestNode = pDestNode->pNext;
                ++i;
            }
            return pDestNode;
        }
        
        //�������ݲ��ҽڵ�
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
        
        //���ݽڵ����λ��
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
        
        //���ݽڵ����ݲ���λ��
        int TMdbNtcBaseList::FindIndex(const TMdbNtcBaseObject &oData, const TMdbNtcObjCompare &oCompare,MDB_UINT32 uiStart /* = 0*/) const
        {
            int iIndex = -1;
            TNode *pCurNode = NULL;//��ǰ�ڵ��λ��
            if(uiStart < m_uiSize/2)//��ǰ�涨λ����ʼ��ѯ��λ��
            {
                iIndex = 0;
                pCurNode = m_pHeadNode;
                while (pCurNode && iIndex < (int)uiStart)
                {
                    ++iIndex;
                    pCurNode = pCurNode->pNext;
                }
            }
            else//�Ӻ��涨λ����ʼ��ѯ��λ��
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
        
        //�ϲ�����
        int TMdbNtcBaseList::Combine(int iDestIndex, TMdbNtcContainer* pSrcContainer, int iSrcStart /* = 0 */, int iSrcCount /* = -1 */)
        {
            MDB_UINT32 uiSrcSize = pSrcContainer->GetSize();
            
            //Դ����Ϊ��,ֱ�ӷ���
            if (0 == uiSrcSize  )
            {
                return -1;
            }
            
            //ʵ�ι���--Դ���ݿ�����Ŀ����������Ϊ��ʼԪ���������Ԫ�ظ���
            if (iSrcCount <= -1)
            {
                iSrcCount = (int)uiSrcSize-iSrcStart;
            }
            
            //���Դ����Ҫ��������Ŀ��������С
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
        
        //��������
        void TMdbNtcBaseList::GenerateArray(TMdbNtcAutoArray& arrayData) const
        {
            if (m_uiSize > 0 )
            {
                //Ԥ��
                arrayData.Reserve(m_uiSize);
                TNode * pCurNode = m_pHeadNode;
                while (pCurNode != NULL)
                {
                    arrayData.Add(pCurNode->pData);
                    pCurNode = pCurNode->pNext;
                }
            }
        }
        
        //����
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
        
        //�Ƴ������ظ��Ľڵ�
        int TMdbNtcBaseList::RemoveDup(const TMdbNtcObjCompare &oCompare)
        {
            
            int iCount = 0;
            if (m_uiSize > 1)
            {
                TNode * pOutNode = m_pHeadNode;  //���ѭ���ڵ�ָ��
                TNode * pInNode;   //�ڲ�ѭ���ڵ�ָ��        
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
        
        //������������
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
        //��ÿ�ʼ������
        TMdbNtcBaseList::iterator TMdbNtcBaseList::IterBegin() const
        {
            return iterator(this, m_uiSize == 0 ? NULL:m_pHeadNode);
        }
        
        //���βԪ�ص�����
        TMdbNtcBaseList::iterator TMdbNtcBaseList::IterLast() const
        {
            return iterator(this,m_uiSize == 0? NULL:m_pTailNode);
        }
        
        //�Ƴ���������Ӧ�Ľڵ�,������һ��������
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
        
        //ͨ����������������
        void TMdbNtcBaseList::IterSwap(TMdbNtcBaseList::iterator itor1, TMdbNtcBaseList::iterator itor2)
        {
            if (itor1.pNodeObject&& itor2.pNodeObject)
            {
                TMdbNtcBaseObject * pTemp = itor1.pNodeObject->pData;
                itor1.pNodeObject->pData= itor2.pNodeObject->pData;
                itor2.pNodeObject->pData= pTemp;
            } 
        }
        
        //���ǰһ��������,�Ҳ����ͳ�����Χ�Ļ�ʹ��IterEnd�ȽϺ�,�Ҹ���itCur��������������ⶪʧ����������Ϣ
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
        
        //��ú�һ��������
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
        //ƽ�������TAvlTree����
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcAvlTree, TMdbNtcContainer);
        //����㹹�캯��
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

        //���캯��
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
                //��Դ�����п������ݣ�������ʹ�õ�����
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
            //��m_pObjCompareָ����ռ�õ��ڴ水��TObjCompare������
            return (MDB_UINT32)(sizeof(TMdbNtcAvlTree)+sizeof(TNode)*m_uiSize+(m_pObjCompare?sizeof(TMdbNtcObjCompare):0));
        }
        
        //�������
        void TMdbNtcAvlTree::Clear()
        {
            if(m_pRootNode)
            {
                //���ƽ�������
                ClearAvlTree(m_pRootNode);
                //���ò��ֳ�Ա����
                m_pRootNode = NULL;
                m_uiSize = 0;
                m_iAvlFlag = 0;
                m_iAvlHeight = 0;
            }
        }
        
        //����������ƽ�������
        void  TMdbNtcAvlTree::ClearAvlTree(TMdbNtcAvlTree::TNode * pParentNode)
        {
            if (pParentNode)
            {
                //���������
                ClearAvlTree(pParentNode->pLeftSubNode);
                //���������
                ClearAvlTree(pParentNode->pRightSubNode);
                //������ڵ�
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
        
        //����½ڵ�
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
        
        //ɾ������ƥ��Ľڵ�
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
        
        //ɾ�����ڵ�,���صĽ��Ϊ��һ���ڵ�ĵ�����
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::Remove(TNode* pAvlNode)
        {
            /*����ڵ�ָ��Ϊ�ջ�����Ϊ�գ����ؿյ�����*/
             if(pAvlNode == NULL || m_pRootNode == NULL)
            {
                return IterEnd();
            }
         
            /*��������*/
            int iBalance ;              //ƽ���
            bool bIsLeftChild = false;  //�����Һ��ӱ�ʶ��true--������ڵ��Ǹ��ڵ�����ӣ�false--������ڵ��Ǹ��ڵ���Һ���
            bool bIsChangedData = false;//�ڵ㽻�����ݱ�־��true--���������ݽ�����false--δ���������ݽ���
            TNode * pNextNode;          //����ڵ㽻����������£������ɾ���ڵ�ĺ�̽ڵ��ָ��
            TNode * pNode;              //���ɽ��ָ��
            iterator itor;              //���صĵ�����
            /* ɾ���ڵ�*/ 
            while(pAvlNode != NULL)
            {
                //����Ϊ��
                if(pAvlNode->pLeftSubNode == NULL)
                {
                    m_iAvlFlag = -1; /* �߶ȼ��� */
        
                    --m_uiSize;      /* ������С��1*/
                    
                    //���ɾ���Ľڵ��Ǹ��ڵ㣬������ɺ�ֱ�ӷ���,�������ƽ��
                    if (pAvlNode->pParentNode == NULL )
                    {
                        m_pRootNode = pAvlNode->pRightSubNode;
                        if(m_pRootNode != NULL )
                        {
                            m_pRootNode->pParentNode = NULL;
                        }
                        MdbNtcReleaseNode(pAvlNode);        
                        //�������߶�
                        m_iAvlHeight += m_iAvlFlag;
        
                        return iterator(this,m_pRootNode);
                    } 
                    else
                    {
                        //�����ɾ���ڵ㸸�ڵ��ƽ���
                        iBalance = pAvlNode->pParentNode->iBalanceFactor;
        
                        //�ж��Ǹ��ڵ�����ӻ����Һ���
                        if(pAvlNode->pParentNode->pLeftSubNode == pAvlNode)
                        {
                            bIsLeftChild = true;
                        }
                        else
                        {
                            bIsLeftChild = false;
                        }
                        
                        //��ȡ��ɾ���ڵ�ĺ�̽ڵ�ĵ�����
                        if (!bIsChangedData)
                        {
                            //������������ݽ������Ѿ���ȡ���̽ڵ�ĵ������������ٻ�ȡ
                           itor = iterator(this,pAvlNode);
                           ++itor;
                        }
        
                        //�����ɾ���ڵ�ָ��
                        pNode = pAvlNode;
        
                        if(bIsLeftChild)
                        {
                            //��ɾ���ڵ�ĸ��ڵ������ָ���ɾ���ڵ���Һ���
                            pAvlNode->pParentNode->pLeftSubNode=pAvlNode->pRightSubNode;
                            //�޸ĸ��ڵ�ƽ���
                            pAvlNode->pParentNode->iBalanceFactor  -= m_iAvlFlag;
                        }
                        else
                        {
                            //��ɾ���ڵ�ĸ��ڵ���Һ���ָ���ɾ���ڵ���Һ���
                            pAvlNode->pParentNode->pRightSubNode=pAvlNode->pRightSubNode;
                            //�޸ĸ��ڵ�ƽ���
                            pAvlNode->pParentNode->iBalanceFactor  += m_iAvlFlag;
                        }
                        
                        //ָ���Һ���
                        pAvlNode = pAvlNode->pRightSubNode;
        
                        //��ɾ���ڵ��Ҷ�ӽڵ㣬�����Һ��ӵĸ�ָ��
                        if (pAvlNode != NULL)
                        {
                            pAvlNode->pParentNode = pNode->pParentNode; 
                        }
        
                        //ָ���ɾ���ڵ�ĸ��ڵ�
                        pAvlNode = pNode->pParentNode;
                        MdbNtcReleaseNode(pNode);        
                        break;
                    }           
                }
                //�Һ���Ϊ��
                else if(pAvlNode->pRightSubNode== NULL)
                {
                    m_iAvlFlag = -1; /* �߶ȼ��� */
        
                    --m_uiSize;      /* ������С��1*/
        
                    //���ɾ���Ľڵ��Ǹ��ڵ㣬������ɺ�ֱ�ӷ���,�������ƽ��
                    if (pAvlNode->pParentNode == NULL)
                    {
                        //�������������һ����Ϊ��
                        m_pRootNode = pAvlNode->pLeftSubNode;
                        m_pRootNode->pParentNode = NULL;
                        MdbNtcReleaseNode(pAvlNode);        
                        //�������ҽڵ�ָ�룬����ڵ�ָ��δ�䶯��������
                        //m_pRightMostNode = m_pRootNode;
                       
                        //�������߶�
                        m_iAvlHeight += m_iAvlFlag;
        
                        return iterator(this);
                    } 
                    else
                    {
                        //�����ɾ���ڵ㸸�ڵ��ƽ���
                        iBalance = pAvlNode->pParentNode->iBalanceFactor;
        
                        //�ж��Ǹ��ڵ�����ӻ����Һ���
                        if(pAvlNode->pParentNode->pLeftSubNode == pAvlNode)
                        {
                            bIsLeftChild = true;
                        }
                        else
                        {
                            bIsLeftChild = false;
                        }
        
                        //��ȡ��ɾ���ڵ�ĺ�̽ڵ�ĵ�����
                        if (!bIsChangedData)
                        {
                            //������������ݽ������Ѿ���ȡ���̽ڵ�ĵ������������ٻ�ȡ
                            itor = iterator(this,pAvlNode);
                            ++itor;
                        }
        
                        //�����ɾ���ڵ�ָ��
                        pNode = pAvlNode;
        
                        if(bIsLeftChild)
                        {
                            //��ɾ���ڵ�ĸ��ڵ������ָ���ɾ���ڵ������
                            pAvlNode->pParentNode->pLeftSubNode=pAvlNode->pLeftSubNode;
                            //�޸ĸ��ڵ�ƽ���
                            pAvlNode->pParentNode->iBalanceFactor  -= m_iAvlFlag;
                        }
                        else
                        {
                            //��ɾ���ڵ�ĸ��ڵ���Һ���ָ���ɾ���ڵ������
                            pAvlNode->pParentNode->pRightSubNode=pAvlNode->pLeftSubNode;
                            //�޸ĸ��ڵ�ƽ���
                            pAvlNode->pParentNode->iBalanceFactor  += m_iAvlFlag;
                        }
        
                        //ָ������
                        pAvlNode = pAvlNode->pLeftSubNode;
        
                        //��ɾ���ڵ��Ҷ�ӽڵ㣬�������ӵĸ�ָ��
                        if (pAvlNode != NULL)
                        {
                            pAvlNode->pParentNode = pNode->pParentNode; 
                        }
                        
                        //ָ���ɾ���ڵ�ĸ��ڵ�
                        pAvlNode = pNode->pParentNode;
                        MdbNtcReleaseNode(pNode);        
                        break;        
                    }            
                }
                else
                {
                    //�������ߣ��򽻻��ڵ�pParentNode����ǰ���ڵ�����ݣ���ɾ��ǰ���ڵ�
                    //��֮�����������ߣ��򽻻��ڵ�pParentNode�����̽ڵ�����ݣ���ɾ����̽ڵ�
        
                    if(pAvlNode->iBalanceFactor< 0 ) 
                    {
                        //��ȡ��̽ڵ�ĵ�����
                        pNextNode = pAvlNode->pRightSubNode;
                        while (pNextNode->pRightSubNode != NULL)
                        {
                            pNextNode = pNextNode->pLeftSubNode;
                        }
                        itor = iterator(this,pNextNode);
        
                        /* �ҵ�ǰ���ڵ� ��ǰ���ڵ�Ϊ�����������ҽڵ�*/
                        pNode = pAvlNode->pLeftSubNode; 
                        while (pNode->pRightSubNode != NULL)
                        {
                            pNode = pNode->pRightSubNode;
                        }
        
                        /* ���������� */
                        pAvlNode->SwapNode(pNode);
                        
                        //���ý�����Ľ��Ϊ��ɾ���ڵ�
                        pAvlNode = pNode;
                    } 
                    else 
                    {
                        /* �ҵ���̽ڵ� ����̽ڵ�Ϊ������������ڵ�*/
                        pNode = pAvlNode->pRightSubNode; 
                        while (pNode->pLeftSubNode != NULL)
                        {
                            pNode = pNode->pLeftSubNode;
                        }
                        
                        //��ȡ��̽ڵ�ĵ�����,�������ݺ󣬾��ǽڵ�pAvlNode����
                        itor = iterator(this,pAvlNode);
        
                        /* ���������� */
                        pAvlNode->SwapNode(pNode);

                        //���ý�����Ľ��Ϊ��ɾ���ڵ�
                        pAvlNode = pNode;
                    }
        
                    bIsChangedData = true;
                }     
            }
        
            /*����ƽ��*/
            while(pAvlNode != NULL)
            {
                //�����pAvlNode�Ǹ��ڵ㣬ֱ�ӵ���ƽ��
                if (pAvlNode->pParentNode == NULL)
                {
                    //����ƽ��
                    if (MDB_ABS(m_iAvlFlag) >= m_iBalanceFactor)
                    {
                        m_pRootNode = BalanceTree(pAvlNode,iBalance);
                    }
                    break;
                } 
                else
                {
                    //���游�ڵ�ָ��
                    pNode = pAvlNode->pParentNode;
        
                    //�ж��Ǹ��ڵ�����ӻ����Һ���
                    if(pNode->pLeftSubNode == pAvlNode)
                    {
                        bIsLeftChild = true;
                    }
                    else
                    {
                        bIsLeftChild = false;
                    }
        
                    //����ƽ��
                    if (MDB_ABS(m_iAvlFlag) >= m_iBalanceFactor)
                    {
                        pAvlNode = BalanceTree(pAvlNode,iBalance);
                        //���ڵ�ĺ��ӽڵ�ָ��ƽ��������
                        if (bIsLeftChild)
                        {
                            pNode->pLeftSubNode = pAvlNode;
                        } 
                        else
                        {
                            pNode->pRightSubNode = pAvlNode;
                        }
                    }
        
                    //�����󱣴游�ڵ��ƽ��ȣ�Ȼ��������ڵ��ƽ���
                    iBalance = pAvlNode->pParentNode->iBalanceFactor;
                    if (bIsLeftChild)
                    {
                        pAvlNode->pParentNode->iBalanceFactor -= m_iAvlFlag;
                    }
                    else
                    {
                        pAvlNode->pParentNode->iBalanceFactor  += m_iAvlFlag;
                    }
                    //ָ�򸸽ڵ�
                    pAvlNode = pAvlNode->pParentNode;
                }              
            }
        
        
            //�������ĸ߶�
            if (m_iAvlFlag != 0)
            {
                m_iAvlHeight += m_iAvlFlag;
            }
            return itor;
        }
        
        //��ȡ��ʼ������
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::IterBegin() const
        {
            return iterator(this, m_pRootNode?GetLeftMostNode(m_pRootNode):NULL);
        }
        
        //���βԪ�ص�����
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::IterLast() const
        {
            return iterator(this, m_pRootNode?GetRightMostNode(m_pRootNode):NULL);
        }
        
        //�Ƴ���������Ӧ�Ľڵ㣬������һ��������
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::IterErase(TMdbNtcAvlTree::iterator itor)
        {
            
            if (m_pRootNode == NULL || itor.pNodeObject== NULL)
            {
                return IterEnd();
            }
            return Remove((TNode *)(itor.pNodeObject));
        }
        
        //ͨ�����������������Ԫ�غͽڵ�Ľ���
        void TMdbNtcAvlTree::IterSwap(TMdbNtcAvlTree::iterator itor1, TMdbNtcAvlTree::iterator itor2)
        {
            //���������壬ֱ�ӷ���
            return;
        }
        
        //�������ݲ��ҵ�����
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
        
        //���ǰһ��������
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
                    //�����Ӳ�Ϊ�գ��������������ҽڵ��ǵ�ǰ�ڵ�����������иýڵ��ǰ��
                    //������Ϊ�գ��򸸽ڵ�,�游�ڵ�.....�е�һ��С�ڸýڵ��ǵ�ǰ�ڵ�����������иýڵ��ǰ��
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
                            //�����ǰ�ڵ�ĸ��ڵ�Ϊ�գ����ʾ�Ѿ�����top����next��Ҫ�������Ѱ��
                            if(pParentNode == NULL)
                            {
                                pCurNode = GetRightMostNode(pCurNode->pLeftSubNode);
                            }
                            //�����ǰ�ڵ��Ǹ��ڵ�����ӽڵ㣬��cur��ֵΪparent��
                            else if(pCurNode == pParentNode->pRightSubNode)
                            {
                                pCurNode = pParentNode;
                                --itCur.iLastStep;
                                break;
                            }
                            //�����ǰ�ڵ��Ǹ��ڵ�����ӽڵ㣬���������
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
        
        //��ú�һ��������
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
                    //���Һ��Ӳ�Ϊ�գ���������������ڵ��ǵ�ǰ�ڵ�����������иýڵ�ĺ��
                    //���Һ���Ϊ�գ��򸸽ڵ㣬�游�ڵ�...�е�һ��Ϊ���ڵ�����ӽڵ㼴ֹͣ,��ʱ�ĸ��ڵ㼴Ϊ��һ��
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
                            //�����ǰ�ڵ�ĸ��ڵ�Ϊ�գ����ʾ�Ѿ�����top����next��Ҫ���ұ���Ѱ��
                            if(pParentNode == NULL)
                            {
                                pCurNode = GetLeftMostNode(pCurNode->pRightSubNode);
                            }
                            //�����ǰ�ڵ��Ǹ��ڵ�����ӽڵ㣬��cur��ֵΪparent��
                            else if(pCurNode == pParentNode->pLeftSubNode)
                            {
                                pCurNode = pParentNode;
                                ++itCur.iLastStep;
                                break;
                            }
                            //�����ǰ�ڵ��Ǹ��ڵ�����ӽڵ㣬���������
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
        
        //����������Ϣ������һ����������ָ��С��pData�ĵ�һ��Ԫ��
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
                    //��ǰ�������Ϊ�գ�������ǰ�ڵ��Ǵ��ڲο�ֵ�ĵ�һ���ڵ�
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
                    //��ǰ�ڵ���Һ���Ϊ�գ�������������иýڵ�ĺ�̾��Ǵ��ڲο�ֵ�ĵ�һ���ڵ�
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
        
        //����������Ϣ������һ����������ָ�����pData�ĵ�һ��Ԫ��
        TMdbNtcAvlTree::iterator TMdbNtcAvlTree::UpperBound(const TNode &oNode) const
        {
            TNode *pCurNode = m_pRootNode;
            while (pCurNode)
            {
                MDB_INT64 iCmpRet = 0;
                if(m_pObjCompare) iCmpRet = m_pObjCompare->Compare(pCurNode->pData, oNode.pData);
                else iCmpRet = pCurNode->Compare(&oNode);
                //ȡ��������иýڵ�ĺ�̷���
                if (iCmpRet > 0)
                {
                    //��ǰ�������Ϊ�գ�������ǰ�ڵ��Ǵ��ڲο�ֵ�ĵ�һ���ڵ�
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
                    //��ǰ�ڵ���Һ���Ϊ�գ�������������иýڵ�ĺ�̾��Ǵ��ڲο�ֵ�ĵ�һ���ڵ�
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

        //�õ����������ȵĿ�ʼ�ͽ���������
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
        
        //��������
        TMdbNtcAvlTree::TNode* TMdbNtcAvlTree::InsertNode(TMdbNtcAvlTree::TNode** ppParentNode, TNode* pNewNode)
        {
            if(ppParentNode == NULL) return NULL;
            TNode* pRetNode = NULL, *pParentNode = *ppParentNode;
            int iBalance;
        
            iBalance = pParentNode->iBalanceFactor;
        
            MDB_INT64 iCmpRet = 0;
            if(m_pObjCompare) iCmpRet = m_pObjCompare->Compare(pParentNode->pData, pNewNode->pData);
            else iCmpRet = pParentNode->Compare(pNewNode);
        
            //������ֵС�ڸ��ڵ��ֵ�����뵽������
            if (iCmpRet > 0)
            {
                if (pParentNode->pLeftSubNode != NULL)
                {
                    pRetNode = InsertNode(&pParentNode->pLeftSubNode, pNewNode);
                    pParentNode->iBalanceFactor -= m_iAvlFlag ; /* �������߶����� */
                }
                else
                {
                    pRetNode = pParentNode->pLeftSubNode = pNewNode; /* �����½ڵ� */
                    pParentNode->pLeftSubNode->pParentNode = pParentNode;//ָ�򸸽ڵ�
                    m_iAvlFlag = 1; /* �߶����� */
                    ++m_uiSize;     //������С��1
                    pParentNode->iBalanceFactor -= m_iAvlFlag ; /* �������߶����� */
                }
            } 
            else 
            {
                if (pParentNode->pRightSubNode != NULL )
                {
                    pRetNode = InsertNode(&pParentNode->pRightSubNode, pNewNode);
                    pParentNode->iBalanceFactor   += m_iAvlFlag; /* �������߶����� */
                } 
                else
                {
                    pRetNode = pParentNode->pRightSubNode = pNewNode;/* �����½ڵ� */
                    pParentNode->pRightSubNode->pParentNode = pParentNode;//ָ�򸸽ڵ�
                    m_iAvlFlag = 1;  /* �߶����� */
                    ++m_uiSize;     //������С��1
                    pParentNode->iBalanceFactor   += m_iAvlFlag; /* �������߶����� */
                }
            }
        
            //������ڵ���������Ϊƽ��������������root����ƽ�⣬����Ҫ����root�ڵ�ָ��
            *ppParentNode = BalanceTree(pParentNode, iBalance);
            return pRetNode;
        }
        
        //ƽ�����ڵ�
        TMdbNtcAvlTree::TNode* TMdbNtcAvlTree::BalanceTree(TMdbNtcAvlTree::TNode* pParentNode, int iBalance)
        {
            if (m_iAvlFlag > 0) 
            {
                //������������������: ��Ҫ����
                if(pParentNode->iBalanceFactor < -m_iBalanceFactor ) 
                {
                    if(pParentNode->pLeftSubNode->iBalanceFactor > 0) 
                    {
                        if (pParentNode->pLeftSubNode->iBalanceFactor > 1 && pParentNode->pLeftSubNode->pRightSubNode->iBalanceFactor > 0) 
                            pParentNode->iBalanceFactor += 1;
                        pParentNode->pLeftSubNode = LeftRotate(pParentNode->pLeftSubNode);
                    }
                    pParentNode = RightRotate(pParentNode);
                    m_iAvlFlag = 0; /* ���ڵ�ĸ߶Ȳ����� */
                }
                //������������������: ��Ҫ����
                else if(pParentNode->iBalanceFactor > m_iBalanceFactor) 
                {
                    if(pParentNode->pRightSubNode->iBalanceFactor < 0) 
                    {
                        if (pParentNode->pRightSubNode->iBalanceFactor < -1 && pParentNode->pRightSubNode->pLeftSubNode->iBalanceFactor <0)
                            pParentNode->iBalanceFactor -= 1;
                        pParentNode->pRightSubNode = RightRotate(pParentNode->pRightSubNode);
                    }
                    pParentNode = LeftRotate(pParentNode);
                    m_iAvlFlag = 0; /* ���ڵ�ĸ߶Ȳ����� */
                }
                else if((iBalance > 0 && iBalance > pParentNode->iBalanceFactor) ||
                    (iBalance < 0 && iBalance < pParentNode->iBalanceFactor))
                {
                    m_iAvlFlag = 0; /* ���ڵ�ĸ߶Ȳ����� */
                }
                else
                {
                    m_iAvlFlag = 1; /* ���ڵ�ĸ߶����� */
                }
            }
            else//(m_iAvlFlag <0)
            {   
                //������������������: ��Ҫ����
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
                        m_iAvlFlag =  0;/* ���ڵ�ĸ߶Ȳ����� */
                    else
                        m_iAvlFlag = -1;/* ���ڵ�ĸ߶ȼ��� */
                    pParentNode = RightRotate(pParentNode);
                }
                //������������������: ��Ҫ���� 
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
                        m_iAvlFlag =  0;/* ���ڵ�ĸ߶Ȳ����� */
                    else
                        m_iAvlFlag = -1;/* ���ڵ�ĸ߶ȼ��� */
                    pParentNode = LeftRotate(pParentNode);
                }
                else if((iBalance > 0 && iBalance > pParentNode->iBalanceFactor) ||
                    (iBalance < 0 && iBalance < pParentNode->iBalanceFactor))
                {
                    m_iAvlFlag = -1; /* ���ڵ�ĸ߶ȼ��� */
                }
                else
                {
                    m_iAvlFlag = 0; /* ���ڵ�ĸ߶Ȳ����� */
                }
            }
            return pParentNode;
        }
        
        /*
        * ������������
        *            X              Y
        *           / \            / \
        *          A   Y    ==>   X   C
        *             / \        / \
        *            B   C      A   B
        * ƽ�����ӱ������
        * ��ת֮ǰ��
        * �ٶ� X ��ƽ�������� x, Y ��ƽ�������� y, 
        * �� A �ĸ߶�Ϊ h, �� Y �ĸ߶�Ϊ h+x 
        * �ڵ� B �߶�Ϊ h+x-1-max(y,0); 
        * �ڵ� C �ĸ߶�Ϊ h+x-1+MIN(y,0);
        * ��ת֮��
        * �ڵ� X ����ƽ�������� x-1-max(y,0); 
        * �ڵ� Y ����ƽ�������� C-(max(A,B)+1) => MIN(C-A-1,C-B-1) 
        *     => MIN(x-2+MIN(y,0),y-1)
        */
        
        TMdbNtcAvlTree::TNode* TMdbNtcAvlTree::LeftRotate(TMdbNtcAvlTree::TNode* pNode)
        {
            TNode *pTempNode;
            int x,y;
        
            //��������
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
        
            //��ȡ��ƽ������
            x = pTempNode->iBalanceFactor;
            y = pNode->iBalanceFactor;
        
            //������ƽ������
            pTempNode->iBalanceFactor = x-1-MDB_MAX(y, 0);
            pNode->iBalanceFactor     = MDB_MIN(x-2+MDB_MIN(y, 0), y-1);
        
            return pNode;
        }
        
        /*
        * ������������
        *            X              Y
        *           / \            / \
        *          Y   C    ==>   A   X
        *         / \                / \
        *        A   B              B   C
        * ƽ�����ӱ������
        * ��ת֮ǰ��
        * �ٶ� X ��ƽ�������� x, �ڵ� Y ��ƽ�������� y, 
        * �� C �ĸ߶�Ϊ h, �� Y �ĸ߶�Ϊ h-x
        * �ڵ� A �߶�Ϊ h-x-1-max(y,0); 
        * �ڵ� B �ĸ߶�Ϊ h-x-1+MIN(y,0);
        * ��ת֮��
        * �ڵ� X ����ƽ�������� x+1-MIN(y,0)
        * �ڵ� Y ����ƽ�������� max(B,C)+1-A => max(B-A+1,C-A+1) 
        *     => max(y+1,x+2+max(y,0))
        */
        TMdbNtcAvlTree::TNode* TMdbNtcAvlTree::RightRotate(TMdbNtcAvlTree::TNode* pNode)
        {
            TNode * pTempNode;
            int x,y;
            
            //��������
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
            
            //��ȡ��ƽ������
            x = pTempNode->iBalanceFactor;
            y = pNode->iBalanceFactor;
        
            //������ƽ������
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
        //��ϣ��HashFunc����
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcHashList, TMdbNtcContainer);
        //���캯��
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
        
        //��ʼ������
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
                //ȡ���ڵ���iTableNum����С����
                unsigned int i = uiTableNum%2==0?(uiTableNum+1):uiTableNum;
                do
                {
                    unsigned int s = (unsigned int)(sqrt(double(i)));//sΪѭ������
                    s = (s/2)*2+1;
                    unsigned int j = 2;
                    for(; j <= s; ++j)           //��j���i�Ƿ�Ϊ����
                    {
                        if(i%j==0)                         //   �����������
                        {
                            break;                           //����forѭ��
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
        
        //�������
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
        
        //��ȡ��ʼ������
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
        
        //�Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������,�˴���Ҫ�޸�
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
                if(itor.pNodeObject == NULL)//��Ҫ�����л�����
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
        
        //ͨ�����������������Ԫ�غͽڵ�Ľ���
        void TMdbNtcHashList::IterSwap(TMdbNtcHashList::iterator itor1, TMdbNtcHashList::iterator itor2)
        {
            if (itor1.pNodeObject&& itor2.pNodeObject)
            {
                TMdbNtcBaseObject * pTemp = itor1.pNodeObject->pData;
                itor1.pNodeObject->pData = itor2.pNodeObject->pData;
                itor2.pNodeObject->pData= pTemp;
            } 
        }
        
        //�����ַ��������������ݵĵ�����
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
        
        //���ǰһ��������,������������л�
        TMdbNtcHashList::iterator TMdbNtcHashList::IterPrev(TMdbNtcHashList::iterator itCur, int iStep /* = -1 */) const
        {
            if (itCur.pNodeObject != NULL )
            {
                return itCur-(-iStep);//���������
            }
        
            //����С�ڵ���-1����������������л�
            if(iStep <= -1)
            {
                itCur.iLastStep = 0;        
                if (m_uiTableNum == 0 || itCur.pDataContainer == NULL
                    || itCur.pDataContainer >= m_pHashList || m_pHashList+m_uiTableNum < itCur.pDataContainer)
                {
                    //����ʵ�ηǷ�������End������
                    itCur.pNodeObject = NULL;
                    return itCur;
                }
                else
                {
                    //�л�������
                    int iIndex = (int )(((TMdbNtcBaseList*)itCur.pDataContainer) - m_pHashList);
                    //�л���������ƫ�Ƶ�����
                    for (--iIndex; iIndex >= 0; --iIndex)
                    {
                        if (m_pHashList[iIndex].GetSize() > 0)
                        {
                            //����������ڵ�ǰ�ǿ���������������С����Ӳ����м�ȥ������С
                            //�л�����һ���ǿ��������ظ�����������ֱ������С�ڵ���������С
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
                    //������������Χ������End������
                    itCur.pNodeObject = NULL;
                    return itCur;          
                }
            }
            //����Ϊ0�����ص�ǰ������
            else if(iStep == 0)
            {
                return itCur;
            }
            else
            {
                return IterNext(itCur, iStep);
            }
            
        }
        
        //��ú�һ��������,����ӵ��������л�
        TMdbNtcHashList::iterator TMdbNtcHashList::IterNext(TMdbNtcAvlTree::iterator itCur, int iStep /* = 1 */) const
        {
            if (itCur.pNodeObject != NULL )
            {
                return itCur+iStep;
            }
        
             //�������ڵ���1����������������л�
            if(iStep >= 1)
            {
                itCur.iLastStep = 0;
                if (m_uiTableNum == 0 || itCur.pDataContainer == NULL
                    || itCur.pDataContainer < m_pHashList || m_pHashList+m_uiTableNum <= itCur.pDataContainer)
                {
                    //����ʵ�ηǷ�������End������
                    itCur.pNodeObject = NULL;
                    return itCur;
                }
                else
                {
                    //�л�������
                    unsigned uiIndex = (unsigned int )(((TMdbNtcBaseList*)itCur.pDataContainer) - m_pHashList);
                    //�л���������ƫ�Ƶ�����
                    for (++uiIndex; uiIndex < m_uiTableNum; ++uiIndex)
                    {
                        if (m_pHashList[uiIndex].GetSize() > 0)
                        {
                            //����������ڵ�ǰ�ǿ���������������С����Ӳ����м�ȥ������С
                            //�л�����һ���ǿ��������ظ�����������ֱ������С�ڵ���������С
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
                    //������������Χ������End������
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
        //����TKeyTree����
        //
        //////////////////////////////////////////////////////////////////////////
        //ʼ�մ���һ�����ڵ㣬��Ϊ�յ�ʱ�򣬸��ڵ��ppSubNodeΪ�գ�����Ҷ�ڵ�ĸ�
        //ָ��ҲΪ�գ����ݴ��ж��Ƿ�ΪҶ�ڵ㣬ɾ��Ҷ�ڵ��ʱ��������ڵ��ΪҶ��
        //�㣬���ͷŸ��ڵ��ppSubNodeָ��ָ����ڴ�ռ䣬����ֵΪNULL�Ա���һ���ԡ�
        
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcKeyTree, TMdbNtcContainer);
        TMdbNtcKeyTree::TNode* TMdbNtcKeyTree::m_s_pNullNode = NULL;        
        //�����ڵ��๹�캯��
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
                        if(pParentNode->ppSubNode[iIndex])//��߻��зǿ��ֵܽڵ�
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
                        if(pParentNode->ppSubNode[iIndex])//�ұ߻��зǿ��ֵܽڵ�
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
                //�ұ�û�зǿսڵ㣬�򸸽ڵ�������ϻ���
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
        
        //�������캯��
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
            
            //�������
            m_uiTNodeCount = (MDB_UINT32)iMax + 1;
            //�������ڵ�
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
                uiSize += (MDB_UINT32)sizeof(TNode*)*m_uiTNodeCount;//������ӽڵ�ָ������ռ���ڴ�Ĵ�С
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
        
        //��սڵ�
        void TMdbNtcKeyTree::Clear()
        {
            if(m_pRootNode)
            {
                DeleteKeyTree(m_pRootNode);
                m_pRootNode = new TNode(m_s_pNullNode);
                m_uiSize = 0;
            }
        }
        
        //��������������
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
                    //��Ҷ�ڵ�
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
        
        //����һ����ֵ,����Ѿ����ڼ�ֵ����֮ǰ������ɾ��,�����µ����
        TMdbNtcContainer::iterator TMdbNtcKeyTree::Add(const char* pszKeyName, TMdbNtcBaseObject* pData)
        {
            if(pszKeyName == NULL)
            {
                return IterEnd();
            }
            TNode ** ppCurNode = &m_pRootNode, *pCurNode = *ppCurNode;  //��ǰ���ڵ�
            const char *p    = pszKeyName;   //�ַ�ָ��
            int   ikey       = -1;           //����Ӧ��λ��    
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //���˵��Ƿ��ַ�
                if (ikey != -1)
                {
                    //�����Ҷ�ڵ㣬���ʼ��ppSubNode
                    if (pCurNode->ppSubNode == NULL)
                    {
                        //��ʼ��ppSubNode
                        pCurNode->ppSubNode = new TNode*[m_uiTNodeCount];
                        memset(pCurNode->ppSubNode, 0x00, m_uiTNodeCount*sizeof(TNode*));
                        //�������ڵ�
                        pCurNode->ppSubNode[ikey] = new TNode(*ppCurNode);
                    } 
                    else
                    {
                        //������ڵ㲻���ڣ��򴴽�
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
        
        //�����ַ�������ɾ������
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
            const char  *p   = pszKeyName;   //�ַ�ָ��
            int   ikey       = -1;           //����Ӧ��λ��
            //���ݼ��ַ�����λ�ڵ�
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //���˵��Ƿ��ַ�
                if (ikey != -1)
                {
                    //�����ǰ�ڵ�ָ��Ϊ�գ����ߵ�ǰ�ڵ�ָ��ΪҶ�ڵ㣬������ѭ��
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
            if(pCurNode && pCurNode->pData && *p == '\0')//�������ݣ���key��ƥ��
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
            //û���ӽڵ������£�����ɾ���˽ڵ�
            if(pCurNode != m_pRootNode && pCurNode->ppSubNode == NULL)
            {
                TNode** ppParentNode = &pCurNode->pParentNode, *pParentNode = *ppParentNode;
                int i = (int)(ppCurNode-pParentNode->ppSubNode);
                if(i != -1) pParentNode->ppSubNode[i] = NULL;
                delete pCurNode;
                pCurNode = NULL;
                //��鸸���µ��ӽڵ��Ƿ�ΪNULL
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
                    if(i < m_uiTNodeCount)//�зǿսڵ�����ѭ��
                    {
                        break;
                    }
                    else//û�зǿ��ӽڵ㣬����Ҫ�ͷ�ppSubNode                
                    {                    
                        delete pParentNode->ppSubNode;
                        pParentNode->ppSubNode = NULL;
                        //��ǰ���ڵ�Ϊ���ڵ㣬���ߺ������ݣ��������������
                        if(pParentNode ==  m_pRootNode || pParentNode->pData)
                        {
                            break;
                        }
                        else
                        {
                            //�õ���ǰ�ڵ����ϼ��ڵ�������
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
        
        //�����ַ���������������
        TMdbNtcBaseObject* TMdbNtcKeyTree::Find(const char* pszKeyName)
        {
            iterator itor = IterFind(pszKeyName);
            if(itor.pNodeObject) return itor.pNodeObject->pData;
            else return NULL;
        }
        
        //����ǰ׺��ȡƥ���Ԫ��
        void TMdbNtcKeyTree::MatchPrefix(const char* pszPrefix, TMdbNtcBaseList& oDataList)
        {
            std::pair<TMdbNtcKeyTree::iterator, TMdbNtcKeyTree::iterator> RetPair = MatchPrefix(pszPrefix);
            for(iterator itor = RetPair.first; itor != RetPair.second; ++itor)
            {
                oDataList.AddTail(itor.pNodeObject->pData);
            }
        }
        
        //�Ƴ�ĳ����������Ӧ�Ľڵ㣬������һ��������
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
        
        //��ȡ��ʼ������
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
        
        //�����ַ��������������ݵĵ�����
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
        
        //ͨ�����������������Ԫ�غͽڵ�Ľ���
        void TMdbNtcKeyTree::IterSwap(TMdbNtcKeyTree::iterator itor1, TMdbNtcKeyTree::iterator itor2)
        {
            if (itor1.pNodeObject&& itor2.pNodeObject)
            {
                TMdbNtcBaseObject * pTemp = itor1.pNodeObject->pData;
                itor1.pNodeObject->pData = itor2.pNodeObject->pData;
                itor2.pNodeObject->pData= pTemp;
            }
        }
        
        //���ǰһ��������
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
                //Ѱ��ǰ���ڵ㣬���ֵܽڵ���ң����û�зǿգ����򸸽ڵ����
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
                                    pCurNode = pCurNode->ppSubNode[i];//currentת�Ƶ��Ӽ�
                                    break;
                                }
                            }
                            //�ҵ��ǿ��ӽڵ�
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
                        //���ҵ�ǰ�ڵ��ڸ��ڵ��е�λ��
                        int i = ReverseFindIndex(pCurNode);
                        //�ж��Ƿ��ҵ���ǰ�ڵ����ֵܽڵ��λ��
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
                        else//���û���ҵ�λ�ã����ǲ����ܵģ�˵��������
                        {
                            break;//����ѭ������������
                        }
                    }
                    else
                    {                
                        break;//����ѭ������������
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
        
        //��ú�һ��������
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
                //Ѱ���Ҽ̽ڵ㣬������������ӽڵ���Ϊ�Ҽ̽ڵ㣬���������ֵܽڵ�����򸸼�����
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
                                pCurNode = pCurNode->ppSubNode[i];//currentת�Ƶ��Ӽ�
                                break;
                            }
                        }
                        //�ҵ��ǿ��ӽڵ�
                        if(i < m_uiTNodeCount)
                        {
                            continue;
                        }
                    }
                    if(pCurNode->pParentNode)
                    {
                        TNode** ppParentNode = &pCurNode->pParentNode, *pParentNode = *ppParentNode;
                        //���ҵ�ǰ�ڵ��ڸ��ڵ��е�λ��
                        int iIndex = FindIndex(pCurNode);
                        if(iIndex == -1)//���û���ҵ�λ�ã����ǲ����ܵģ�˵��������
                        {
                            break;//����ѭ������������
                        }
                        //�ж��Ƿ��ҵ���ǰ�ڵ����ֵܽڵ��λ��
                        else
                        {
                            pCurNode = FindSiblingOrParentNext(&pParentNode->ppSubNode[iIndex]);
                            if(pCurNode == NULL)
                            {
                                break;//����ѭ������������
                            }
                        }
                    }
                    else
                    {                
                        break;//����ѭ������������
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
        
        //�����ַ�����������һ����������ָ��С�ڼ�ֵ�ĵ�һ��������
        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::LowerBound(const char* pszKeyName) const
        {
            if(pszKeyName == NULL) return IterEnd();
            TNode *pCurNode = m_pRootNode;   //��ǰ�ڵ�
            const char  *p   = pszKeyName;   //�ַ�ָ��
            int   ikey       = -1;           //����Ӧ��λ��
            //���ݼ��ַ�����λ�ڵ�
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //���˵��Ƿ��ַ�
                if (ikey != -1)
                {
                    //�����ǰ�ڵ�ָ��Ϊ�գ����ߵ�ǰ�ڵ�ָ��ΪҶ�ڵ㣬������ѭ��
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
                if(pCurNode->pData && *p == '\0')//�������ݣ���key��ƥ��
                {
                    return iterator(this, pCurNode);
                }
                else//û������
                {
                    //ʹ�������÷�����++,�Ա����ڶ��ؼ����е��øú���ʱʹ�ö��ؼ�����++���������µõ��ظ���ֵ
                    return TMdbNtcKeyTree::IterNext(iterator(this, pCurNode));
                }
            }
            else
            {
                return iterator(this);
            }
        }
        
        //�����ַ�����������һ����������ָ����ڼ�ֵ�ĵ�һ��������
        TMdbNtcKeyTree::iterator TMdbNtcKeyTree::UpperBound(const char* pszKeyName) const
        {
            if(pszKeyName == NULL) return IterEnd();
            TNode *pCurNode = m_pRootNode;  //��ǰ�ڵ�
            const char  *p   = pszKeyName;   //�ַ�ָ��
            int   ikey       = -1;           //����Ӧ��λ��
            //���ݼ��ַ�����λ�ڵ�
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //���˵��Ƿ��ַ�
                if (ikey != -1)
                {
                    //�����ǰ�ڵ�ָ��Ϊ�գ����ߵ�ǰ�ڵ�ָ��ΪҶ�ڵ㣬������ѭ��
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
                //ʹ�������÷�����++,�Ա����ڶ��ؼ����е��øú���ʱʹ�ö��ؼ�����++���������µõ��ظ���ֵ
                return TMdbNtcKeyTree::IterNext(iterator(this, pCurNode));
            }
            else
            {
                return iterator(this);
            }
        }
        
        //����ǰ׺��ȡƥ���Ԫ��
        std::pair<TMdbNtcKeyTree::iterator, TMdbNtcKeyTree::iterator> TMdbNtcKeyTree::MatchPrefix(const char* pszPrefix) const
        {
            std::pair<iterator, iterator> retPair(IterEnd(), IterEnd());
            if(pszPrefix == NULL) return retPair;
            TNode ** ppCurNode = (TNode**)&m_pRootNode;  //��ǰ�ڵ�
            const char  *p   = pszPrefix;   //�ַ�ָ��
            int   ikey       = -1;           //����Ӧ��λ��
            //���ݼ��ַ�����λ�ڵ�
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //���˵��Ƿ��ַ�
                if (ikey != -1)
                {
                    //�����ǰ�ڵ�ָ��Ϊ�գ����ߵ�ǰ�ڵ�ָ��ΪҶ�ڵ㣬������ѭ��
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
                else//û�����ݣ���ͨ��++���Ҵ������������ݽڵ�
                {
                    retPair.first = ++iterator(this, *ppCurNode);            
                }
                //���ҽ���λ��
                //�����ұ߷ǿ��ֵܽڵ�������ϻ��ݵĸ��ڵ�
                TNode* pEndNode = FindSiblingOrParentNext(ppCurNode);
                if(pEndNode)
                {
                    if(pEndNode->pData)
                    {
                        retPair.second = iterator(this, pEndNode);
                    }
                    else//û�����ݣ���ͨ��++���Ҵ������������ݽڵ�
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
            TNode * pCurNode = m_pRootNode;  //��ǰ�ڵ�
            const char  *p   = pszKeyName;   //�ַ�ָ��
            int   ikey       = -1;           //����Ӧ��λ��
            //���ݼ��ַ�����λ�ڵ�
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //���˵��Ƿ��ַ�
                if (ikey != -1)
                {
                    //�����ǰ�ڵ�ָ��Ϊ�գ����ߵ�ǰ�ڵ�ָ��ΪҶ�ڵ㣬������ѭ��
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
        //���ؼ���TMultiKeyTree����
        //
        //////////////////////////////////////////////////////////////////////////
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcMultiKeyTree, TMdbNtcKeyTree);
        //���ؼ������캯��
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
        
        //��սڵ�
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
                if(pParentNode) uiSize += (MDB_UINT32)sizeof(TMdbNtcKeyTree::TNode);//���ڵ��Ƿ���ΪTKeyTree::TNode
                else return uiSize;
            }
            if(pParentNode->ppSubNode)
            {
                uiSize += (MDB_UINT32)sizeof(TNode*)*m_uiTNodeCount;//������ӽڵ�ָ������ռ���ڴ�Ĵ�С
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
        
        //��������������
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
                    //��Ҷ�ڵ�
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
        
        //��Ӽ�ֵ
        TMdbNtcContainer::iterator TMdbNtcMultiKeyTree::Add(const char* pszKeyName, TMdbNtcBaseObject* pData)
        {
            if(pszKeyName == NULL)
            {
                return IterEnd();
            }
            TMdbNtcKeyTree::TNode ** ppCurNode = &m_pRootNode, *pCurNode = *ppCurNode;  //��ǰ���ڵ�
            const char *p    = pszKeyName;   //�ַ�ָ��
            int   ikey       = -1;           //����Ӧ��λ��    
            while(*p)
            {
                ikey = m_iCharacterIndex[(int)*p];
                //���˵��Ƿ��ַ�
                if (ikey != -1)
                {
                    //�����Ҷ�ڵ㣬���ʼ��ppSubNode
                    if (pCurNode->ppSubNode == NULL)
                    {
                        //��ʼ��ppSubNode
                        pCurNode->ppSubNode = new TMdbNtcKeyTree::TNode*[m_uiTNodeCount];
                        memset(pCurNode->ppSubNode, 0x00, m_uiTNodeCount*sizeof(TMdbNtcKeyTree::TNode*));
                        TNode* pTempNextNode = ((TNode*)pCurNode)->pNext;
                        while(pTempNextNode)
                        {
                            //��Ҫ����Щ�ظ���ֵ�ڵ��ppSubNode��ָ��head��ppSubNode
                            pTempNextNode->ppSubNode = pCurNode->ppSubNode;
                            pTempNextNode = pTempNextNode->pNext;
                        }              
                        //�������ڵ�
                        pCurNode->ppSubNode[ikey] = new TNode(*ppCurNode);
                    } 
                    else
                    {
                        //������ڵ㲻���ڣ��򴴽�
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
            /*���pCurNode��ͷ��㣬�����ֳ�����Ҫ����pCurNode�ĸ��ڵ�ָ��pCurNode��ָ��
              ����1:pCurNode���ֵܽ�㣬��pCurNode->pNextΪ��
              ����2:pCurNodeû���ֵܽ�㣬����pCurNode�������ӽڵ㣬��pCurNode->pNextΪ�٣���pCurNode->ppSubNode == NULL
              ˵��:�����ֳ������ǿ���delete��pCurNode������������Ҫ����ͷָ��
            */
            if(((TNode*)pCurNode)->pPrev == NULL && 
                  (((TNode*)pCurNode)->pNext || pCurNode->ppSubNode == NULL))
            {
                //��ȡ���ڵ��б����ͷ���ָ��ĵ�ַ
                TNode** ppHeadNode = GetHeadAddr((TNode*)pCurNode);
                if(ppHeadNode)
                {
                    //��ͷ����е�ָ����һ��
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
            //���ֵܽڵ㣬�����ֱ��ɾ���˽ڵ�
            if(((TNode*)pCurNode)->pPrev || ((TNode*)pCurNode)->pNext)
            {
                delete pCurNode;
                pCurNode = NULL;
            }
            //���û���ֵܽ�㣬�������ӽ�㣬�����ֱ��ɾ������Ӧ�����ϻ����жϸ��ڵ��Ƿ����ɾ��
            else if(pCurNode->ppSubNode == NULL )
            {
                delete pCurNode;
                pCurNode = NULL;
                //��鸸���µ��ӽڵ��Ƿ�ΪNULL
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
                    if(i < m_uiTNodeCount)//�зǿսڵ�����ѭ��
                    {
                        break;
                    }
                    else//û�зǿ��ӽڵ㣬����Ҫ�ͷ�ppSubNode                
                    {
                        delete[] pParentNode->ppSubNode;
                        pParentNode->ppSubNode = NULL;
                        if(((TNode*)pParentNode)->pNext)
                        {
                            //��Ҫ����Щ�ظ��ڵ��ppSubNode����ΪNULL
                            TNode* pTempNode = ((TNode*)pParentNode)->pNext;
                            do
                            {
                                pTempNode->ppSubNode = NULL;
                                pTempNode = pTempNode->pNext;
                            }while(pTempNode);
                        }
                        //��ǰ���ڵ�Ϊ���ڵ㣬���ߺ������ݣ��������������
                        if(pParentNode == m_pRootNode || pParentNode->pData)
                        {
                            break;
                        }
                        else
                        {
                            //�õ���ǰ�ڵ����ϼ��ڵ�������
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
        
        //�����ַ�������ɾ������
        int TMdbNtcMultiKeyTree::Remove(const char* pszKeyName)
        {
            if(pszKeyName == NULL) return 0;
            int iRetDelCount = 0;
            TMdbNtcKeyTree::TNode** ppCurNode = FindNode(pszKeyName);
            if(ppCurNode)
            {
                TNode* pCurNode = (TNode*)*ppCurNode, *pTempNode = NULL;
                //��ɾ������ļ�ֵ��ͬ�Ľڵ�
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
                    Remove(pCurNode);//size �������Ѿ������ı�
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
                //���Ҳ��������ظ��ڵ�
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
        
        //�����ַ����������Ҽ�ֵ��Ӧ�Ŀ�ʼ
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
                //�ҵ��׽ڵ���
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
        
        //���ǰһ��������
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
                //Ѱ��ǰ���ڵ㣬���ֵܽڵ���ң����û�зǿգ����򸸽ڵ����
                while (pCurNode)
                {
                    //�����ǰ�ڵ㲻�ǵ�������ָ�����ȡ��ǰ�ڵ���ӽڵ�
                    if(bSearchChild)
                    {
                        if(pCurNode->ppSubNode)
                        {
                            int i = 0;
                            for (i = (int)m_uiTNodeCount-1; i >= 0; --i)
                            {
                                if(pCurNode->ppSubNode[i])
                                {
                                    pCurNode = (TNode*)pCurNode->ppSubNode[i];//currentת�Ƶ��Ӽ�
                                    break;
                                }
                            }
                            //�ҵ��ǿ��ӽڵ�
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
                        //���ҵ�ǰ�ڵ��ڸ��ڵ��е�λ��
                        int i = ReverseFindIndex(pCurNode);
                        //�ж��Ƿ��ҵ���ǰ�ڵ����ֵܽڵ��λ��
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
                        else//���û���ҵ�λ�ã����ǲ����ܵģ�˵��������
                        {
                            break;//����ѭ������������
                        }
                    }
                    else
                    {                
                        break;//����ѭ������������
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
        
        //��ú�һ��������
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
                //Ѱ���Ҽ̽ڵ㣬������������ӽڵ���Ϊ�Ҽ̽ڵ㣬���������ֵܽڵ�����򸸼�����
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
                                pCurNode = (TNode*)pCurNode->ppSubNode[i];//currentת�Ƶ��Ӽ�
                                break;
                            }
                        }
                        //�ҵ��ǿ��ӽڵ�
                        if(i < m_uiTNodeCount)
                        {
                            continue;
                        }
                    }
                    if(pCurNode->pParentNode)
                    {
                        MDB_UINT32 i = 0;
                        TMdbNtcKeyTree::TNode** ppParentNode = &pCurNode->pParentNode, *pParentNode = *ppParentNode;
                        //���ҵ�ǰ�ڵ��ڸ��ڵ��е�λ��
                        TNode * pHeadNode = pCurNode;
                        while( pHeadNode->pPrev != NULL) 
                        {
                            pHeadNode = pHeadNode->pPrev;
                        }
                        i = (MDB_UINT32)FindIndex(pHeadNode);
                        //�ж��Ƿ��ҵ���ǰ�ڵ����ֵܽڵ��λ��
                        if(i < m_uiTNodeCount)
                        {
                            pCurNode = (TNode*)FindSiblingOrParentNext(&pParentNode->ppSubNode[i]);
                            if(pCurNode == NULL)
                            {
                                break;//����ѭ������������
                            }
                        }
                        else//���û���ҵ�λ�ã����ǲ����ܵģ�˵��������
                        {
                            break;//����ѭ������������
                        }
                    }
                    else
                    {                
                        break;//����ѭ������������
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
            //��������ֱ�ӷ���
            if(((m_uiPushTimes + 1)%MDB_NTC_MAX_QUENE_SIZE) == m_uiPopTimes)
            {
                if(m_pTailMutex) m_pTailMutex->Unlock();
                return false;
            }
            //m_uiPushTimes��1
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
            //m_uiPopTimes��1
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
            //��TNodeʱ��Ҫ���Ͽսڵ�
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
            if(pCurNode)//�ҵ�
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
            if(pCurNode == NULL || pPrevNode == m_pHeadNode) return IterEnd();//û���ҵ��˽ڵ�    
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
                //��������ֱ�ӷ���
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
                    //������,ʧ�ܷ���
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
                    //��ʱ��ʧ�ܷ���
                    return false;
                }
                else if(mdb_ntc_zthread_testcancel())
                {
                    //����������ã�֪ͨ����һ���߳�ȥ�����Լ��˳�
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
            //m_uiPushTimes��1
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
            bool bNotify = false, bLocked = false; //�Ƿ���Ҫ֪ͨ�����ȴ�Pop���߳�
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
                if(bLocked)//��������£���֪Ϊ�գ�����Ҫ������wait
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
                //����ȴ������нӵ�֪ͨ���򷵻�true
                if(!m_pPopEvent->Wait(iMilliSeconds))
                {
                    return NULL;
                }
                else if(mdb_ntc_zthread_testcancel())//��Ҫ�˳�
                {
                    //����������ã�֪ͨ����һ���߳�ȥ�����Լ��˳�
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
            //���д�����ɷ���
            if((m_uiPopTimes-m_uiPushTimes+MDB_NTC_MAX_QUENE_SIZE)%MDB_NTC_MAX_QUENE_SIZE == 2)
            {
                m_pPushEvent->SetEvent();
            }
            bNotify &= (m_pHeadNode != m_pTailNode);
            if(m_pHeadMutex) m_pHeadMutex->Unlock();
            delete pOldHead;
            pOldHead = NULL;
            if(bNotify)//ֻ���ھ���������£��Ż�֪ͨ��һ����
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
            //����
            m_pPushCond->Lock();

            //������
            if(m_uiSize >= MDB_NTC_MAX_QUENE_SIZE)
            {
                bool bError = false;
                if(iMilliSeconds != 0)
                {
                    while(m_uiSize == MDB_NTC_MAX_QUENE_SIZE)
                    {
                        if(!m_pPushCond->Wait(iMilliSeconds))
                        {
                            //��ʱ����
                            bError = true;
                            break;
                        }
                        else if(mdb_ntc_zthread_testcancel())//��Ҫ�˳�
                        {
                            //��ǰ�̱߳�ȡ��,֪ͨ����push�̷߳�����
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
            //����
            m_pPushCond->Unlock();
            if(bNotify)
            {
                m_pPopCond->SetEvent();//֪ͨpop�߳̿���ȡ������
            }

            return true;
        }

        TMdbNtcBaseObject* TMdbNtcQueue::Pop(int iMilliSeconds /* = -1 */)
        {
            bool bNotify = false;
            //����
            m_pPopCond->Lock();
            //���п�
            if(m_uiSize <= 0)
            {
                bool bError = false;
                if(iMilliSeconds != 0)
                {
                    while( 0 == m_uiSize)
                    {
                        if(!m_pPopCond->Wait(iMilliSeconds))
                        {
                            //��ʱ����
                            bError = true;
                            break;
                        }
                        else if(mdb_ntc_zthread_testcancel())//��Ҫ�˳�
                        {
                            //��ǰ�̱߳�ȡ��,֪ͨ����pop�߳�ȡ����
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
            
            //����
            m_pPopCond->Unlock();
            if(bNotify)
            {
                m_pPushCond->SetEvent();//֪ͨpush�߳̿��Է�������
            }
            //ɾ���ɵ�ͷ���
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
            //��TNodeʱ��Ҫ���Ͽսڵ�
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
            if(pCurNode)//�ҵ�
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
            if(pCurNode == NULL || pPrevNode == m_pHeadNode) return IterEnd();//û���ҵ��˽ڵ�    
            else return iterator(this, pPrevNode);
        }
        
        TMdbNtcQueue::iterator TMdbNtcQueue::IterNext(iterator itCur, int iStep /*= 1*/) const
        {
            if(itCur.pNodeObject == NULL) return IterEnd();
            else return iterator(this, ((TNode*)itCur.pNodeObject)->pNext);
        }
        
//}

