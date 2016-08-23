#include "Common/mdbSysMessages.h"
//#include "Common/mdbLogInterface.h"
#include "Common/mdbSysThreads.h"
//namespace QuickMDB
//{
        /*
        С���������ֶ�ռ�õ��ֽ��������������Ϊ��Ҫ��Ϣ��(24)+��Ϣ������(uiShmSize)
        �ܵĻ����С��ʣ�໺���С����ͨ��GeTShareMemSize()���α�λ�������
        ��Ҫ��Ϣ����Push����(4)|Pop����(4)|��ǰ���α�(4)|��ǰд�α�(4)
        ��Ϣ��������ÿһ����Ϣ�飬ǰ�˶���2���ֽڵ���Ϣ���ͱ�־��4�ֽڵĳ��ȱ�־(type|length)
        */
        static const char* const MDB_NTC_MQSHM_PREFIX = "MQ_SHM_";         ///<�����ڴ�����ǰ�
        static const char* const MDB_NTC_MQEVENT_PREFIX = "MQ_EVENT_";     ///<�¼�����ǰ�
        static const char* const MDB_NTC_MQSEM_PUSH_PREFIX = "MQ_SEM_PUSH_";   ///<Push�����ź�������ǰ�
        static const char* const MDB_NTC_MQSEM_POP_PREFIX = "MQ_SEM_POP_";     ///<Pop�����ź�������ǰ�

        const MDB_UINT32 TMdbNtcMessageQueue::ms_uiMsgHeadLen = sizeof(MDB_UINT16)+sizeof(MDB_UINT32);
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcMessageQueue, TMdbNtcSystemNameObject);

        TMdbNtcMessageQueue::TMdbNtcMessageQueue(const char* pszName, MDB_UINT32 uiShmSize /* = 0 */,
            const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */, TMdbNtcSystemNameObject* pParentNameObject /* = NULL */,
            bool bCheckVersion /* = true */ )
            :TMdbNtcSystemNameObject(pszName, pszInstEnvName, pParentNameObject)
        {
            m_pShm = NULL;
            m_pPushLock = NULL;
            m_pPopLock = NULL;
            m_pMgrInfo = NULL;
            m_pContentAddr = NULL;
            m_uiShmSize = 0;
            m_pEvent = NULL;
            m_fnMsgOverlappedCallback = NULL;
            m_pMsgOverlappedArg = NULL;
            if (NULL == pszName || '\0' == pszName[0])
            {
                m_bConstructResult = false;
                TADD_WARNING("name is not valid\n");
                return;
            }
            else if(uiShmSize > 0 && uiShmSize <= ms_uiMsgHeadLen)
            {
                m_bConstructResult = false;
                TADD_WARNING("size[%d] is not valid\n", uiShmSize);
                return;
            }

            bool bRet = false;
            m_bSelfCreated = false;
            TMdbNtcStringBuffer oTStrBuff;
            oTStrBuff = MDB_NTC_MQSHM_PREFIX;
            oTStrBuff.Append(pszName);
            bRet = TMdbNtcShareMem::CheckExist(oTStrBuff.c_str(), pszInstEnvName);
            if(!bRet && uiShmSize == 0)//��������Ǹ��ӣ���ʧ��
            {
                m_bConstructResult = false;
                TADD_WARNING("ShareMem is not exist and size is 0.\n");
                return;
            }

            //����ͬ���ź���
            oTStrBuff = MDB_NTC_MQSEM_PUSH_PREFIX;
            oTStrBuff.Append(pszName);
            
            m_pPushLock = new TMdbNtcProcessLock(oTStrBuff.c_str(), pszInstEnvName, this);
            bRet = m_pPushLock->GetConstructResult();
            if (bRet)
            {
                bRet = m_pPushLock->Lock();
                if(bRet)
                {
                    do
                    {
                        oTStrBuff = MDB_NTC_MQSHM_PREFIX;
                        oTStrBuff.Append(pszName);
                        if(!TMdbNtcShareMem::CheckExist(oTStrBuff.c_str(), pszInstEnvName))
                        {
                            if( 0 == uiShmSize )
                            {
                                bRet = false;
                                break;
                            }
                            else
                                m_bSelfCreated = true;
                        }
                        
                        m_pShm = new TMdbNtcShareMem(oTStrBuff.c_str(), m_bSelfCreated?(uiShmSize+sizeof(TMQMgrInfo)):0, pszInstEnvName, this, bCheckVersion);
                        bRet = m_pShm->GetConstructResult();
                        if(!bRet)
                        {
                            break;
                        }
                        
                        m_pMgrInfo = reinterpret_cast<TMQMgrInfo*>(m_pShm->GetBuffer());
                        if (NULL == m_pMgrInfo)
                        {
                            bRet = false;
                            break;
                        }
                        
                        oTStrBuff = MDB_NTC_MQSEM_POP_PREFIX;
                        oTStrBuff.Append(pszName);                
                        m_pPopLock = new TMdbNtcProcessLock(oTStrBuff.c_str(), pszInstEnvName, this);
                        bRet = m_pPopLock->GetConstructResult();
                        if (!bRet)
                        {
                            break;
                        }
                        
                        //�����¼�
                        oTStrBuff = MDB_NTC_MQEVENT_PREFIX;
                        oTStrBuff.Append(pszName);
                        m_pEvent = new TMdbNtcProcessEvent(oTStrBuff.c_str(), pszInstEnvName, this);
                        bRet = m_pEvent->GetConstructResult();
                        if (!bRet)
                        {
                            break;
                        }
                        m_pContentAddr = reinterpret_cast<unsigned char*>(m_pMgrInfo+1);
                        
                        if((MDB_UINT32)m_pShm->GetSize() <= sizeof(TMQMgrInfo)+ms_uiMsgHeadLen)
                        {
                            bRet = false;
                            TADD_WARNING("shm size is not valid\n");
                            break;;
                        }
                        if(m_bSelfCreated)
                        {
                            //���ù����ڴ��������Ϣ
                            m_pMgrInfo->uiLastPushTime = (MDB_UINT32)-1;
                            m_pMgrInfo->uiLastPopTime = (MDB_UINT32)-1;
                            m_pMgrInfo->uiPopCursor = 0;
                            m_pMgrInfo->uiPushCursor = 0;
                            m_pMgrInfo->uiPopTimes = 0;
                            m_pMgrInfo->uiPushTimes = 0;
                            memset(m_pMgrInfo->szReserve, 0x00, sizeof(m_pMgrInfo->szReserve));
                        }
                        m_uiShmSize = (MDB_UINT32)m_pShm->GetSize() - (MDB_UINT32)sizeof(TMQMgrInfo);
                    } while (0);
                    m_pPushLock->Unlock();
                }
            }

            m_bConstructResult = bRet;
            if(m_bConstructResult)
            {
                TMdbNtcSysNameObjectHelper::SignIn(this);
            }
            else if(m_bSelfCreated)
            {
                ManualFree();
            }
        }

        TMdbNtcMessageQueue::~TMdbNtcMessageQueue()
        {
            if(m_pShm)
            {
                delete m_pShm;
                m_pShm = NULL;
            }
            if(m_pPushLock)
            {
                delete m_pPushLock;
                m_pPushLock = NULL;
            }
            if(m_pPopLock)
            {
                delete m_pPopLock;
                m_pPopLock = NULL;
            }
            if(m_pEvent)
            {
                delete m_pEvent;
                m_pEvent = NULL;
            }
            m_pMgrInfo = NULL;
            m_pContentAddr = NULL;
            //m_sName.Clear();
            m_uiShmSize = 0;
        }

        bool TMdbNtcMessageQueue::IsOK()
        {
            if(this == NULL || m_pShm == NULL || m_pPopLock == NULL || m_pPushLock == NULL) return false;
            else return m_pShm->IsOK() && m_pPopLock->IsOK() && m_pPushLock->IsOK();
        }

        bool TMdbNtcMessageQueue::ManualFree()
        {
            if(m_pShm)
            {
                m_pShm->ManualFree();
                delete m_pShm;
                m_pShm = NULL;
            }
            if(m_pPopLock)
            {
                m_pPopLock->ManualFree();
                delete m_pPopLock;
                m_pPopLock = NULL;
            }
            if(m_pPushLock)
            {
                m_pPushLock->ManualFree();
                delete m_pPushLock;
                m_pPushLock = NULL;
            }
            if(m_pEvent)
            {
                m_pEvent->ManualFree();
                delete m_pEvent;
                m_pEvent = NULL;
            }
            m_uiShmSize = 0;
            m_pMgrInfo = NULL;
            m_pContentAddr = NULL;
            return TMdbNtcSysNameObjectHelper::Free(this);
        }

        bool TMdbNtcMessageQueue::CheckExist(const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            if( NULL == pszName || *pszName == '\0' )
                return false;
            TMdbNtcStringBuffer oTStrBuff = MDB_NTC_MQSHM_PREFIX;
            oTStrBuff.Append(pszName);
            return TMdbNtcShareMem::CheckExist(oTStrBuff.c_str(), pszInstEnvName);
        }

        bool TMdbNtcMessageQueue::Destroy(const char* pszName, const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {
            if( NULL == pszName || *pszName == '\0')
                return false;
            bool bRet = false;
            TMdbNtcMessageQueue* pQueue = NULL;
            try
            {
                pQueue = new TMdbNtcMessageQueue(pszName, 0, pszInstEnvName, NULL, false);//�Ը��ӷ�ʽ
                if(pQueue->GetConstructResult())
                {
                    bRet = pQueue->ManualFree();
                }
            }
            catch (TMdbNtcException* e)
            {
                TADD_WARNING("Destroy share mq[%s] failed!%s\n", pszName, e->GetErrMsg());
                bRet = false;
            }
            if(pQueue)
            {
                delete pQueue;
                pQueue = NULL;
            }
            return bRet||TMdbNtcSysNameObjectHelper::Free(&oRuntimeObject, pszName, pszInstEnvName);
        }        

        bool TMdbNtcMessageQueue::Send(MDB_UINT16 uiType, const void* pBuffer, MDB_UINT32 uiLength /* = -1 */, bool bAllowOverlap /* = false */)
        {
            MDB_NTC_ZF_ASSERT(m_pMgrInfo);            
            if (uiLength == (MDB_UINT32)-1)
            {
                uiLength = pBuffer?  (MDB_UINT32)strlen((const char*)pBuffer):0;
            }
            if(GetFreeSize() < uiLength && !bAllowOverlap)
            {
                return false;
            }
            MDB_UINT32 uiMsgLen = ms_uiMsgHeadLen + uiLength;
            bool bRet = false;
            if(!m_pPushLock->Lock())
            {
                return false;
            }
            MDB_UINT32 uiPopCursor  = m_pMgrInfo->uiPopCursor;
            MDB_UINT32 uiNewPushCursor = m_pMgrInfo->uiPushCursor;
            bool bNotify = false; //�Ƿ���Ҫ֪ͨ�ȴ�Receive���߳�
            do
            {
                if (uiNewPushCursor > uiPopCursor)
                {
                    if (uiNewPushCursor > m_uiShmSize-uiMsgLen)//ĩ�˲������
                    {
                        uiNewPushCursor = 0; //��Push�α��Ƶ������ڴ�ǰ��
                        if (uiPopCursor >= uiMsgLen)//ǰ�˹����
                        {
                            bRet = true;
                            break;
                        }
                    }
                    else//ĩ�˹����
                    {
                        bRet = true;
                        break;
                    }
                }
                else if(uiNewPushCursor == uiPopCursor)
                {
                    if(m_pMgrInfo->uiPushTimes == m_pMgrInfo->uiPopTimes)
                    {
                        uiNewPushCursor = 0;//˵��������,���α��Ƶ�ͷ��
                        if(!m_pPopLock->Lock())
                        {
                            m_pPushLock->Unlock();
                            return false;
                        }
                        m_pMgrInfo->uiPopCursor = 0;
                        m_pPopLock->Unlock();
                        bRet = true;
                        break;
                    }
                }
                else if (uiPopCursor-uiNewPushCursor >= uiMsgLen)//˵��֮��������ǹ����
                {
                    bRet = true;
                    break;
                }
                //���֮�����򲻹���ţ���push��ĩβ������Ҳ������ţ�����Ҫ��push�Ƶ�ͷλ��
                else if(m_uiShmSize-uiNewPushCursor < uiMsgLen)
                {
                    uiNewPushCursor = 0;
                }
                //��������Բ�����ŵ�����������Ǵ���
                if(bAllowOverlap && m_uiShmSize >= uiMsgLen)
                {
                    bRet = true;//������
                    if(!m_pPopLock->Lock())
                    {
                        bRet = false;
                        break;
                    }
                    MDB_UINT32& uiPopCursor  = m_pMgrInfo->uiPopCursor;//���¸�ֵһ�Σ��õ����µ�
                    MDB_UINT32 uiCurMsgLen = 0, iValidLength = uiPopCursor-uiNewPushCursor;
                    while(iValidLength < uiMsgLen)
                    {
                        if(uiPopCursor == m_pMgrInfo->uiPushCursor
                            && m_pMgrInfo->uiPushTimes == m_pMgrInfo->uiPopTimes)//˵���Ѿ�ȫ��ȡ�꣬�ʿ��Դ�ͷ��ʼ���
                        {
                            uiPopCursor = 0;
                            uiNewPushCursor = 0;
                            break;
                        }
                        else if(uiPopCursor <= m_uiShmSize-ms_uiMsgHeadLen)
                        {
                            memcpy(&uiCurMsgLen, m_pContentAddr + uiPopCursor + sizeof(uiType), sizeof(uiCurMsgLen));
                        }
                        else//ĩβ�������
                        {
                            uiCurMsgLen = (MDB_UINT32)-1;                            
                        }
                        if(uiCurMsgLen == (MDB_UINT32)-1)
                        {
                            uiPopCursor = 0;//��ͷ��ʼ
                            uiNewPushCursor = 0;//�����ͷ��ʼ�������
                            iValidLength = 0;
                        }
                        else
                        {
                            /*
                            printf("now[%u] pop %u vs %u\nused_size[%u], pop_cursor[%u], push_cursor[%u], newpush_cursor[%u]\n",
                                m_uiShmSize,
                                m_pMgrInfo->uiPushTimes, m_pMgrInfo->uiPopTimes,
                                GetUsedSize(),
                                m_pMgrInfo->uiPopCursor, m_pMgrInfo->uiPushCursor, uiNewPushCursor); 
                                */
                            if(m_fnMsgOverlappedCallback)
                            {
                                m_fnMsgOverlappedCallback(this, *(MDB_UINT16*)(m_pContentAddr + uiPopCursor),
                                    m_pContentAddr + uiPopCursor+ms_uiMsgHeadLen, uiCurMsgLen, m_pMsgOverlappedArg);
                            }
                            uiPopCursor += uiCurMsgLen+ms_uiMsgHeadLen;
                            ++m_pMgrInfo->uiPopTimes;
                            iValidLength += uiCurMsgLen+ms_uiMsgHeadLen;
                        }
                    }
                    m_pPopLock->Unlock();
                }
            } while (0);
            if(bRet)
            {
                if(m_pMgrInfo->uiPushCursor != uiNewPushCursor)
                {
                    if(m_pMgrInfo->uiPushCursor <= m_uiShmSize-ms_uiMsgHeadLen)//��Ҫ����һ��push��ʼ��λ����Ϊ0xFF�����������ڱ�pop��
                    {
                        memset(m_pContentAddr+m_pMgrInfo->uiPushCursor, 0xFF, ms_uiMsgHeadLen); //д��Ϣͷ������ϢΪ-1
                    }
                    m_pMgrInfo->uiPushCursor = uiNewPushCursor;
                }
                SetMsgHead(uiType, uiLength); //д��Ϣͷ������Ϣ
                if(pBuffer && uiLength > 0)
                {
                    memcpy(m_pContentAddr + uiNewPushCursor + ms_uiMsgHeadLen, pBuffer, uiLength); //д��Ϣ��
                }
                m_pMgrInfo->uiPushCursor += uiMsgLen;
                m_pMgrInfo->uiPushTimes++;
                m_pMgrInfo->uiLastPushTime = (MDB_UINT32)time(NULL);
                bNotify = MDB_ABS(m_pMgrInfo->uiPushTimes-m_pMgrInfo->uiPopTimes)==1;
            }
            m_pPushLock->Unlock();
            if (bNotify)
            {
                m_pEvent->SetEvent();
            }

            return bRet;
        }

        void TMdbNtcMessageQueue::recv_queue_msg_callback(TMdbNtcMessageQueue* pMQ, MDB_UINT16 uiType, const void* pBuffer, MDB_UINT32 uiLength, void* pArg)
        {
            RECV_MSG* pRecvMsg = reinterpret_cast<RECV_MSG*>(pArg);
            pRecvMsg->uiType = uiType;
            if(uiLength > 0)
            {
                pRecvMsg->sOut.SetBuffer((const unsigned char*)pBuffer, uiLength);
            }
            else
            {
                pRecvMsg->sOut.Clear();
            }
        }

        bool TMdbNtcMessageQueue::SyncDealMsg(mdb_ntc_queue_msg_callback deal_func, void* pArg /* = NULL */, MDB_INT32 iMilliSeconds /* = -1 */)
        {
            if( NULL == deal_func )
                return false;
            MDB_NTC_ZF_ASSERT(m_pMgrInfo != NULL);
            bool bNotify = false, bLocked = false; //�Ƿ���Ҫ֪ͨ�����ȴ�Pop���߳�
            do 
            {
                if(IsEmpty())
                {
                    if(bLocked)//��������£���֪Ϊ�գ�����Ҫ������wait
                    {
                        m_pPopLock->Unlock();
                        bLocked = false;
                    }
                    if(iMilliSeconds == 0)
                    {
                        return false;
                    }
                    else if(!m_pEvent->Wait(iMilliSeconds))
                    {
                        return false;
                    }
                    else if(mdb_ntc_zthread_testcancel())//��Ҫ�˳�
                    {
                        if(!IsEmpty())
                        {
                            //֪ͨ����һ���߳�ȥ�����Լ��˳�
                            m_pEvent->SetEvent();
                        }
                        return false;
                    }
                    else
                    {
                        bNotify = true;//�����˵ȴ�������ٽ��IsEmpty���õ��Ƿ���Ҫ֪ͨ
                        bLocked = true;
                        if(!m_pPopLock->Lock())
                        {
                            return false;
                        }
                    }
                }
                else if(bLocked == false)//�������ˣ���Ҫ������ȷ��
                {
                    bLocked = true;
                    if(!m_pPopLock->Lock())
                    {
                        return false;
                    }
                }
                else//�Ǽ�����ȷ�ϵ������ݣ���ֱ������
                {
                    break;
                }
            } while (1);  
            MDB_UINT32 uiMsgLen = 0;
            MDB_UINT16 uiType = 0;
            GetMsgHead(uiType, uiMsgLen);
            if((MDB_UINT32)-1 == uiMsgLen)
            {//���������ڴ�ĩ��
                m_pMgrInfo->uiPopCursor = 0;
                GetMsgHead(uiType, uiMsgLen);
                if((MDB_UINT32)-1 == uiMsgLen)
                {
                    m_pPopLock->Unlock();
                    return false;
                }
            }
            deal_func(this, uiType, m_pContentAddr+m_pMgrInfo->uiPopCursor+ms_uiMsgHeadLen, uiMsgLen, pArg);
            ++m_pMgrInfo->uiPopTimes;
            m_pMgrInfo->uiLastPopTime = (MDB_UINT32)time(NULL);
            //��Ϊ�α�����send�жϣ����Է������ֵ
            m_pMgrInfo->uiPopCursor += uiMsgLen + ms_uiMsgHeadLen; //Ҫ��m_pMgrInfo->uiPopTimes����֮ǰ����(��IsEmpty������Push�е������)
            bNotify &= !IsEmpty();
            m_pPopLock->Unlock();
            if (bNotify)
            {
                m_pEvent->SetEvent();
            }

            return true;
        }

        MDB_UINT32 TMdbNtcMessageQueue::GetCount() const
        {
            if(m_pMgrInfo == NULL) return 0;
            if (m_pMgrInfo->uiPushTimes < m_pMgrInfo->uiPopTimes) //˵��push�Ѿ��������ͷ��ʼ������
            {
                return m_pMgrInfo->uiPushTimes + MDB_NTC_ZS_MAX_UINT32 - m_pMgrInfo->uiPopTimes;
            }
            else
            {
                return m_pMgrInfo->uiPushTimes - m_pMgrInfo->uiPopTimes;
            }
        }

        MDB_UINT32 TMdbNtcMessageQueue::GetUsedSize() const
        {
            if(m_pMgrInfo == NULL) return 0;
            if (m_pMgrInfo->uiPopCursor >= m_pMgrInfo->uiPushCursor
                && m_pMgrInfo->uiPopTimes != m_pMgrInfo->uiPushTimes)
            {
                return m_uiShmSize-m_pMgrInfo->uiPopCursor+m_pMgrInfo->uiPushCursor;
            }
            else
            {
                return m_pMgrInfo->uiPushCursor - m_pMgrInfo->uiPopCursor;
            }
        }

        MDB_UINT32 TMdbNtcMessageQueue::GetFreeSize() const
        {
            if(m_pMgrInfo == NULL) return 0;
            MDB_UINT32 uiFreeSize = 0;
            if (m_pMgrInfo->uiPopCursor >= m_pMgrInfo->uiPushCursor
                && m_pMgrInfo->uiPopTimes != m_pMgrInfo->uiPushTimes)   //��Ϣ����ȡ��λ�ô��ڵ�����Ϣ��д���λ�� ���� ���д���ȡ����Ϣ (����������пռ�����)
            {
                uiFreeSize = m_pMgrInfo->uiPopCursor - m_pMgrInfo->uiPushCursor;    //��Ϣ����ȡ��λ��-��Ϣ��д���λ��(�����д�Ϊ����λ��֮��Ĵ�С)
            }
            else    //��Ϣ����ȡ��λ��С����Ϣ��д���λ�� ���� û�д���ȡ����Ϣ (����������пռ���ܲ�����)
            {
                uiFreeSize = m_uiShmSize-m_pMgrInfo->uiPushCursor;  //��Ϣ���еĺ���д�(������Ϣ��д��λ�ÿ�ʼ����Ϣ���е�ĩ��)
                //��Ϊǰ����д�����������ʹ�ã�����ֻ��ѡ�����Ŀ��д�С
                if(uiFreeSize < m_pMgrInfo->uiPopCursor) uiFreeSize = m_pMgrInfo->uiPopCursor;  //��Ϣ���е�ǰ���д�(������Ϣ���еĿ�ͷλ�ÿ�ʼ����Ϣ����ȡ��λ��)
            }
            if(uiFreeSize < ms_uiMsgHeadLen) return 0;
            else return uiFreeSize-ms_uiMsgHeadLen;
        }

        void TMdbNtcMessageQueue::SetMsgHead(MDB_UINT16 uiType, MDB_UINT32 uiBodyLen)
        {
            memcpy(m_pContentAddr + m_pMgrInfo->uiPushCursor, &uiType, sizeof(uiType));
            memcpy(m_pContentAddr + m_pMgrInfo->uiPushCursor + sizeof(uiType), &uiBodyLen, sizeof(uiBodyLen));
        }

        void TMdbNtcMessageQueue::GetMsgHead(MDB_UINT16& uiType, MDB_UINT32& uiBodyLen)
        {
            if(m_pMgrInfo->uiPopCursor > m_uiShmSize-ms_uiMsgHeadLen)
            {
                uiBodyLen = (MDB_UINT32)-1;
                return;
            }
            memcpy(&uiType, m_pContentAddr + m_pMgrInfo->uiPopCursor, sizeof(uiType));
            memcpy(&uiBodyLen, m_pContentAddr + m_pMgrInfo->uiPopCursor + sizeof(uiType), sizeof(uiBodyLen));
        }
//}
