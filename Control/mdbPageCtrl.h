/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbPageCtrl.h		
*@Description： 负责管理某个页的空间信息：获取一个自由页、空闲页
*@Author:		li.shugang
*@Date：	    2008年11月30日
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_PAGE_CTROL_H__
#define __ZX_MINI_DATABASE_PAGE_CTROL_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbMutex.h"
#include "Helper/mdbDictionary.h"

//namespace QuickMDB{
	//页类型,操作锁时使用
    /*enum E_PAGE_TYPE
    {
    	PAGE_TYPE_NO_VARCHAR  = 0, //非varchar页
    	PAGE_TYPE_VARCHAR_16 = 1, //varchar16位页
    	PAGE_TYPE_VARCHAR_32 = 2, //varchar32位页
    	PAGE_TYPE_VARCHAR_64 = 3, //varchar64位页
    	PAGE_TYPE_VARCHAR_128 = 4, //varchar128位页
    	PAGE_TYPE_VARCHAR_256 = 5, //varchar256位页
    	PAGE_TYPE_VARCHAR_512 = 6, //varchar512位页
    	PAGE_TYPE_VARCHAR_1024 = 7, //varchar1024位页
    	PAGE_TYPE_VARCHAR_2048 = 8, //varchar2048位页
    	PAGE_TYPE_VARCHAR_4096 = 9, //varchar4096位页
    	PAGE_TYPE_VARCHAR_8192 = 10 //varchar8192位页
    };*/

    class TMdbPageCtrl
    {
    public:
        TMdbPageCtrl();
        ~TMdbPageCtrl();
        
        /******************************************************************************
        * 函数名称	:  SetDSN()
        * 函数描述	:  删除页中一个数据    
        * 输入		:  iPos, 数据的节点位置  
        * 输出		:  无
        * 返回值	:  成功返回0，如果没有找到数据则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int SetDSN(const char* pszDSN, E_MUTEX_TYPE eMutexType = MUTEX_TYPE_PAGE);
				
        /******************************************************************************
        * 函数名称	:  Attach()
        * 函数描述	:  关联一个页    
        * 输入		:  pAddr, 页的首地址 
        * 输入		:  bReadLock, 是否有读锁
        * 输入		:  bWriteLock, 是否有写锁 
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Attach(char* pAddr, bool bReadLock, bool bWriteLock); 

        /******************************************************************************
        * 函数名称	:  InsertData()
        * 函数描述	:  向页中插入一个数据    
        * 输入		:  pData, 数据的首地址  
        * 输入		:  iSize, 数据的大小 
        * 输出		:  rowID, 返回的ROWID
        * 返回值	:  成功返回0，如果没有空间则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int InsertData(unsigned char* pData, int iSize, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn=true);
	 //此函数内不加叶锁，页锁在函数外手动加	
	 int InsertData_NoMutex(unsigned char* pData, int iSize, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn=true);
	 
        /******************************************************************************
        * 函数名称	:  DeleteData()
        * 函数描述	:  删除页中一个数据    
        * 输入		:  iPos, 数据的节点位置  
        * 输出		:  无
        * 返回值	:  成功返回0，如果没有找到数据则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int DeleteData(int iPos,bool UpdateLsn=true);
	 int DeleteData_NoMutex(int iPos,bool UpdateLsn=true);

        //加锁
        int RLock();
        int WLock();
        
        //解锁
        void UnRLock();
        void UnWLock();
        void UpdatePageLSN(){m_pPage->m_iPageLSN = m_pDSN->GetLSN();}//更新pageLSN
    private:
        bool m_bReadLock, m_bWriteLock;
        TMdbPage *m_pPage;  //页的头
        char* m_pAddr;      
        TMdbMutex m_tMutex;
        TMdbDSN* m_pDSN;
    };

//}
#endif //__ZX_MINI_DATABASE_PAGE_CTROL_H__

