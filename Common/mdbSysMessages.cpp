#include "Common/mdbSysMessages.h"
//#include "Common/mdbLogInterface.h"
#include "Common/mdbSysThreads.h"
//namespace QuickMDB
//{
        /*
        小括弧内是字段占用的字节数，整个区域分为概要信息区(24)+消息缓存区(uiShmSize)
        总的缓存大小和剩余缓存大小可以通过GeTShareMemSize()及游标位置算出。
        概要信息区：Push次数(4)|Pop次数(4)|当前读游标(4)|当前写游标(4)
        消息缓存区：每一个消息块，前端都有2个字节的消息类型标志，4字节的长度标志(type|length)
        */
        static const char* const MDB_NTC_MQSHM_PREFIX = "MQ_SHM_";         ///<共享内存名的前辍
        static const char* const MDB_NTC_MQEVENT_PREFIX = "MQ_EVENT_";     ///<事件名的前辍
        static const char* const MDB_NTC_MQSEM_PUSH_PREFIX = "MQ_SEM_PUSH_";   ///<Push互斥信号量名的前辍
        static const char* const MDB_NTC_MQSEM_POP_PREFIX = "MQ_SEM_POP_";     ///<Pop互斥信号量名的前辍

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
            if(!bRet && uiShmSize == 0)//如果仅仅是附接，则失败
            {
                m_bConstructResult = false;
                TADD_WARNING("ShareMem is not exist and size is 0.\n");
                return;
            }

            //创建同步信号量
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
                        
                        //创建事件
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
                            //设置共享内存管理区信息
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
                pQueue = new TMdbNtcMessageQueue(pszName, 0, pszInstEnvName, NULL, false);//以附接方式
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
            bool bNotify = false; //是否需要通知等待Receive的线程
            do
            {
                if (uiNewPushCursor > uiPopCursor)
                {
                    if (uiNewPushCursor > m_uiShmSize-uiMsgLen)//末端不够存放
                    {
                        uiNewPushCursor = 0; //将Push游标移到共享内存前段
                        if (uiPopCursor >= uiMsgLen)//前端够存放
                        {
                            bRet = true;
                            break;
                        }
                    }
                    else//末端够存放
                    {
                        bRet = true;
                        break;
                    }
                }
                else if(uiNewPushCursor == uiPopCursor)
                {
                    if(m_pMgrInfo->uiPushTimes == m_pMgrInfo->uiPopTimes)
                    {
                        uiNewPushCursor = 0;//说明无数据,将游标移到头部
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
                else if (uiPopCursor-uiNewPushCursor >= uiMsgLen)//说明之间的区域是够存放
                {
                    bRet = true;
                    break;
                }
                //如果之间区域不够存放，且push到末尾的区域也不够存放，则需要将push移到头位置
                else if(m_uiShmSize-uiNewPushCursor < uiMsgLen)
                {
                    uiNewPushCursor = 0;
                }
                //下面是针对不够存放的情况，做覆盖处理
                if(bAllowOverlap && m_uiShmSize >= uiMsgLen)
                {
                    bRet = true;//做覆盖
                    if(!m_pPopLock->Lock())
                    {
                        bRet = false;
                        break;
                    }
                    MDB_UINT32& uiPopCursor  = m_pMgrInfo->uiPopCursor;//重新赋值一次，得到最新的
                    MDB_UINT32 uiCurMsgLen = 0, iValidLength = uiPopCursor-uiNewPushCursor;
                    while(iValidLength < uiMsgLen)
                    {
                        if(uiPopCursor == m_pMgrInfo->uiPushCursor
                            && m_pMgrInfo->uiPushTimes == m_pMgrInfo->uiPopTimes)//说明已经全部取完，故可以从头开始存放
                        {
                            uiPopCursor = 0;
                            uiNewPushCursor = 0;
                            break;
                        }
                        else if(uiPopCursor <= m_uiShmSize-ms_uiMsgHeadLen)
                        {
                            memcpy(&uiCurMsgLen, m_pContentAddr + uiPopCursor + sizeof(uiType), sizeof(uiCurMsgLen));
                        }
                        else//末尾不够存放
                        {
                            uiCurMsgLen = (MDB_UINT32)-1;                            
                        }
                        if(uiCurMsgLen == (MDB_UINT32)-1)
                        {
                            uiPopCursor = 0;//从头开始
                            uiNewPushCursor = 0;//必须从头开始连续存放
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
                    if(m_pMgrInfo->uiPushCursor <= m_uiShmSize-ms_uiMsgHeadLen)//需要将上一次push开始的位置置为0xFF，这样不至于被pop到
                    {
                        memset(m_pContentAddr+m_pMgrInfo->uiPushCursor, 0xFF, ms_uiMsgHeadLen); //写消息头长度信息为-1
                    }
                    m_pMgrInfo->uiPushCursor = uiNewPushCursor;
                }
                SetMsgHead(uiType, uiLength); //写消息头长度信息
                if(pBuffer && uiLength > 0)
                {
                    memcpy(m_pContentAddr + uiNewPushCursor + ms_uiMsgHeadLen, pBuffer, uiLength); //写消息体
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
            bool bNotify = false, bLocked = false; //是否需要通知其他等待Pop的线程
            do 
            {
                if(IsEmpty())
                {
                    if(bLocked)//加锁情况下，得知为空，则需要解锁后wait
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
                    else if(mdb_ntc_zthread_testcancel())//需要退出
                    {
                        if(!IsEmpty())
                        {
                            //通知到下一个线程去做，自己退出
                            m_pEvent->SetEvent();
                        }
                        return false;
                    }
                    else
                    {
                        bNotify = true;//经过了等待，最后再结合IsEmpty，得到是否需要通知
                        bLocked = true;
                        if(!m_pPopLock->Lock())
                        {
                            return false;
                        }
                    }
                }
                else if(bLocked == false)//有数据了，需要加锁后确认
                {
                    bLocked = true;
                    if(!m_pPopLock->Lock())
                    {
                        return false;
                    }
                }
                else//是加锁后确认的有数据，则直接跳出
                {
                    break;
                }
            } while (1);  
            MDB_UINT32 uiMsgLen = 0;
            MDB_UINT16 uiType = 0;
            GetMsgHead(uiType, uiMsgLen);
            if((MDB_UINT32)-1 == uiMsgLen)
            {//遇到共享内存末端
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
            //因为游标用于send判断，所以放在最后赋值
            m_pMgrInfo->uiPopCursor += uiMsgLen + ms_uiMsgHeadLen; //要在m_pMgrInfo->uiPopTimes更新之前调用(与IsEmpty函数在Push中调用相关)
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
            if (m_pMgrInfo->uiPushTimes < m_pMgrInfo->uiPopTimes) //说明push已经溢出，从头开始计数了
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
                && m_pMgrInfo->uiPopTimes != m_pMgrInfo->uiPushTimes)   //消息待读取的位置大于等于消息待写入的位置 并且 还有待读取的消息 (此种情况空闲空间连续)
            {
                uiFreeSize = m_pMgrInfo->uiPopCursor - m_pMgrInfo->uiPushCursor;    //消息待读取的位置-消息待写入的位置(即空闲处为两个位置之间的大小)
            }
            else    //消息待读取的位置小于消息待写入的位置 或者 没有待读取的消息 (此种情况空闲空间可能不连续)
            {
                uiFreeSize = m_uiShmSize-m_pMgrInfo->uiPushCursor;  //消息队列的后空闲处(即从消息待写入位置开始到消息队列的末端)
                //因为前后空闲处不可以连续使用，所以只能选择最大的空闲大小
                if(uiFreeSize < m_pMgrInfo->uiPopCursor) uiFreeSize = m_pMgrInfo->uiPopCursor;  //消息队列的前空闲处(即从消息队列的开头位置开始到消息待读取的位置)
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
