/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbMutex.cpp		
*@Description�� �ڴ����ݿ�����������
*@Author:		li.shugang
*@Date��	    2008��11��30��
*@History:
******************************************************************************************/
#include "Helper/mdbMutex.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbOS.h"

//namespace QuickMDB{       

    /******************************************************************************
    * ��������	:  Init()
    * ��������	:  ��ʼ������ַ    
    * ����		:  �� 
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0, ʧ�ܷ���-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbMutex::Init(const char* pszDSN, TMdbDSN* &pDSNOut,E_MUTEX_TYPE eMutexType)
    {        
    	m_iPID = TMdbOS::GetPID();
        m_iTID = TMdbOS::GetTID();
        m_pDSN = NULL;
        
        TMdbShmDSN* pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN);
        if(pShmDSN == NULL)
        {
            return -1;    
        }
        TMdbDSN* pDSN = pShmDSN->GetInfo();
        if(pDSN != NULL)
        {
    		switch(eMutexType)
    		{
    			case MUTEX_TYPE_PAGE://ҳ��
    				m_pMutex = (TMutex*)  pShmDSN->GetPageMutexAddr();
					iMutexCount = MAX_MUTEX_COUNTS;
					break;
				case MUTEX_TYPE_VARCHAR_PAGE://VARCHARҳ��
					m_pMutex = (TMutex*) pShmDSN->GetVarcharPageMutexAddr();
					iMutexCount = MAX_VARCHAR_MUTEX_COUNTS;
					break;
    			case MUTEX_TYPE_ROW://����
    				m_pMutex = (TMutex*)pShmDSN->GetRowMutexAddr();
					iMutexCount = MAX_ROW_MUTEX_COUNTS;
    				break;
    			default:
    				return -1;
    				break;
    		}
            pDSNOut = pDSN;
            m_pDSN  = pDSN;
            return 0;
        }
        
        pDSNOut = NULL;
        
        return -1;
    }

    /******************************************************************************
    * ��������	:  Lock()
    * ��������	:  ��ĳ��λ�ü���    
    * ����		:  �� 
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbMutex::Lock(int iPos)
    {
        if(m_pMutex != NULL)
        {
            int iRet = m_pMutex[iPos%iMutexCount].Lock(true);    
            if(m_pDSN != NULL)
            {
                m_pMutex[iPos%iMutexCount].m_tCurTime.tv_sec = m_pDSN->tCurTime.tv_sec;
                m_pMutex[iPos%iMutexCount].m_tCurTime.tv_usec = m_pDSN->tCurTime.tv_usec;
                //strncpy(m_pMutex[iPos%MAX_MUTEX_COUNTS].m_sTime, m_pDSN->sCurTime, MAX_TIME_LEN);
            }
            //bIsLock = true;
            return iRet;
        }
        
        return 0;
    }

    /******************************************************************************
    * ��������	:  UnLock()
    * ��������	:  ��ĳ��λ�ý���    
    * ����		:  �� 
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbMutex::UnLock(int iPos)
    {
        if(m_pMutex != NULL)
        {
            //bIsLock = false;
            m_pMutex[iPos%iMutexCount].m_tCurTime.tv_sec = 0;
            m_pMutex[iPos%iMutexCount].m_tCurTime.tv_usec = 0;
            //m_pMutex[iPos%MAX_MUTEX_COUNTS].m_sTime[0] = '\0';
            m_pMutex[iPos%iMutexCount].UnLock(true);    
        }
    }
//}
