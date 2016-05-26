/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbMutex.cpp		
*@Description： 内存数据库的锁管理控制
*@Author:		li.shugang
*@Date：	    2008年11月30日
*@History:
******************************************************************************************/
#include "Helper/mdbMutex.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbOS.h"

//namespace QuickMDB{       

    /******************************************************************************
    * 函数名称	:  Init()
    * 函数描述	:  初始化锁地址    
    * 输入		:  无 
    * 输出		:  无
    * 返回值	:  成功返回0, 失败返回-1
    * 作者		:  li.shugang
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
    			case MUTEX_TYPE_PAGE://页锁
    				m_pMutex = (TMutex*)  pShmDSN->GetPageMutexAddr();
					iMutexCount = MAX_MUTEX_COUNTS;
					break;
				case MUTEX_TYPE_VARCHAR_PAGE://VARCHAR页锁
					m_pMutex = (TMutex*) pShmDSN->GetVarcharPageMutexAddr();
					iMutexCount = MAX_VARCHAR_MUTEX_COUNTS;
					break;
    			case MUTEX_TYPE_ROW://行锁
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
    * 函数名称	:  Lock()
    * 函数描述	:  对某个位置加锁    
    * 输入		:  无 
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
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
    * 函数名称	:  UnLock()
    * 函数描述	:  对某个位置解锁    
    * 输入		:  无 
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
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
