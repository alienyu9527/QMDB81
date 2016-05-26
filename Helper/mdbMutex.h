/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbMutex.h		
*@Description： 内存数据库的锁管理控制
*@Author:		li.shugang
*@Date：	    2008年11月30日
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_MUTX_CTROL_H__
#define __ZX_MINI_DATABASE_MUTX_CTROL_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbDictionary.h"
#include "Helper/TMutex.h"

//namespace QuickMDB{

    //锁类型，初始化锁的时候用
    enum E_MUTEX_TYPE
    {
    	MUTEX_TYPE_PAGE  = 1, //普通页锁
    	MUTEX_TYPE_VARCHAR_PAGE = 2, //varchar页锁
    	MUTEX_TYPE_ROW = 3 //行锁
    };

    class TMdbMutex
    {
    public:
        TMdbMutex()
        {
            m_bInitFlag = false;
            m_pMutex    = NULL;
			iMutexCount = 0;
        }
        
        
        /******************************************************************************
        * 函数名称	:  Init()
        * 函数描述	:  初始化锁地址    
        * 输入		:  pszDSN, 锁管理区所属的DSN 
        * 输出		:  pDSNOut, 锁对应的DSN指针
        * 返回值	:  成功返回0, 失败返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Init(const char* pszDSN, TMdbDSN* &pDSNOut,E_MUTEX_TYPE eMutexType);
        
        /******************************************************************************
        * 函数名称	:  Lock()
        * 函数描述	:  对某个位置加锁    
        * 输入		:  无 
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        int Lock(int iPos);
        
        /******************************************************************************
        * 函数名称	:  UnLock()
        * 函数描述	:  对某个位置解锁    
        * 输入		:  无 
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void UnLock(int iPos);
        
    private:
        bool m_bInitFlag;    
        TMutex *m_pMutex;
        int m_iPID;
        //int m_iTID;
    	unsigned long int m_iTID;
        TMdbDSN* m_pDSN;
		int iMutexCount;
    };
//}


#endif //__ZX_MINI_DATABASE_MUTX_CTROL_H__


