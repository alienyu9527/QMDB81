/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbPageCtrl.cpp		
*@Description： 负责管理某个页的空间信息：获取一个自由页、空闲页
*@Author:		li.shugang
*@Date：	    2008年11月30日
*@History:
******************************************************************************************/
#include "Control/mdbPageCtrl.h"
#include "string.h"


//namespace QuickMDB{

    TMdbPageCtrl::TMdbPageCtrl()
    {
		m_pDSN = NULL;
    }


    TMdbPageCtrl::~TMdbPageCtrl()
    {
        
    }
    /******************************************************************************
    * 函数名称	:  SetDSN()
    * 函数描述	:  删除页中一个数据    
    * 输入		:  iPos, 数据的节点位置  
    * 输出		:  无
    * 返回值	:  成功返回0，如果没有找到数据则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbPageCtrl::SetDSN(const char* pszDSN, E_MUTEX_TYPE eMutexType)
    {
        return m_tMutex.Init(pszDSN, m_pDSN, eMutexType);
    }
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
    int TMdbPageCtrl::Attach(char* pAddr, bool bReadLock, bool bWriteLock)
    {
        TADD_FUNC("Start.");
        //检测参数是否合法
        CHECK_OBJ(pAddr);
        m_bReadLock = bReadLock;
        m_bWriteLock = bWriteLock;
        //初始化页信息
        m_pAddr = pAddr;
        m_pPage = (TMdbPage*)pAddr;
        return 0;
    }
    //加读锁
    int TMdbPageCtrl::RLock()
    {
        if(m_bReadLock)
        {
			return m_tMutex.Lock(m_pPage->m_iPageID);
        }
        return 0;
    }
    //加写锁
    int TMdbPageCtrl::WLock()
    {
        if(m_bWriteLock)
        {
			return m_tMutex.Lock(m_pPage->m_iPageID);
        }
        return 0;
    }
    //解读锁
    void TMdbPageCtrl::UnRLock()
    {
        if(m_bReadLock)
        {
            m_tMutex.UnLock(m_pPage->m_iPageID);
        }
    }
    //解写锁
    void TMdbPageCtrl::UnWLock()
    {
        if(m_bWriteLock)
        {
            m_tMutex.UnLock(m_pPage->m_iPageID);
        }
    }
    /******************************************************************************
    * 函数名称	:  InsertData()
    * 函数描述	:  向页中插入一个数据    
    * 输入		:  pData, 数据的首地址  
    * 输入		:  iLen, 数据的大小 
    * 输出		:  rowID, 返回的ROWID,   pMemData - 内存地址
    * 返回值	:  成功返回0，如果没有空间则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbPageCtrl::InsertData(unsigned char* pData, int iLen, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn)
    {
        TADD_FUNC("Start.");
        //检测参数是否合法
        if(pData==NULL || iLen<0 ||iLen>1024*1024*10)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Paramter invalid. pData=%p, iSize=%d.",pData, iLen);
            return ERR_APP_INVALID_PARAM;    
        }
        if(WLock() < 0)//加锁
        	return ERR_SQL_EXECUTE_TIMEOUT;
        int iRet = 0;
        do
        {   //申请一块新空间
            int iDataOffset = 0;
            iRet = m_pPage->GetFreeRecord(iDataOffset,iLen);
            if(iRet == ERR_PAGE_NO_MEMORY)
            {//page没有空间了，无需报错
                break;
            }
            else if(0 != iRet)
            {//其他错误
                CHECK_RET_BREAK(iRet,"GetFreeRecord failed,RowId[%d|%d],size=[%d],page=[%s]",
                        rowID.GetPageID(),rowID.GetDataOffset(),iLen,m_pPage->ToString().c_str());
            }
            rowID.SetDataOffset(m_pPage->DataOffsetToRecordPos(iDataOffset));//设置数据位置
            rowID.SetPageID(m_pPage->m_iPageID);
            //rowID.iPageID = m_pPage->m_iPageID;
            //pMemData = m_pAddr + rowID.iDataOffset;
            pMemData = m_pAddr + iDataOffset;
            memcpy(pMemData, pData, iLen);
            //修改头信息:记录数、数据偏移量、剩余空间大小、时间戳
            SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);
            ++(m_pPage->m_iRecordCounts); 
            if(UpdateLsn){UpdatePageLSN();}
            TADD_DETAIL("page_id=[%d],offsetPos=[%d],rowid=[%d].",m_pPage->m_iPageID,iDataOffset,rowID.m_iRowID);
            
        }while(0);
        UnWLock();//解锁
        TADD_FUNC("Finish.");
        return iRet;
    }

	/******************************************************************************
	* 函数名称	:  InsertData()
	* 函数描述	:  向页中插入一个数据	 
	* 输入		:  pData, 数据的首地址	
	* 输入		:  iLen, 待插入数据的大小 
	* 输入		:  iSize, 页记录的大小 
	* 输出		:  rowID, 返回的ROWID,	 pMemData - 内存地址
	* 返回值	:  成功返回0，如果没有空间则返回-1
	* 作者		:  li.shugang
	*******************************************************************************/
	int TMdbPageCtrl::InsertData(unsigned char* pData,int iLen, int iSize, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn)
	{
		TADD_FUNC("Start.");
		//检测参数是否合法
		if(pData==NULL || iSize<0 ||iSize>1024*1024*10)
		{
			TADD_ERROR(ERR_APP_INVALID_PARAM,"Paramter invalid. pData=%p, iSize=%d.",pData, iSize);
			return ERR_APP_INVALID_PARAM;	 
		}
		if(WLock() < 0)//加锁
			return ERR_SQL_EXECUTE_TIMEOUT;
		int iRet = 0;
		do
		{	//申请一块新空间
			int iDataOffset = 0;
			iRet = m_pPage->GetFreeRecord(iDataOffset,iSize);
			if(iRet == ERR_PAGE_NO_MEMORY)
			{//page没有空间了，无需报错
				break;
			}
			else if(0 != iRet)
			{//其他错误
				CHECK_RET_BREAK(iRet,"GetFreeRecord failed,RowId[%d|%d],size=[%d],page=[%s]",
						rowID.GetPageID(),rowID.GetDataOffset(),iSize,m_pPage->ToString().c_str());
			}
			rowID.SetDataOffset(m_pPage->DataOffsetToRecordPos(iDataOffset));//设置数据位置
			rowID.SetPageID(m_pPage->m_iPageID);
			//rowID.iPageID = m_pPage->m_iPageID;
			//pMemData = m_pAddr + rowID.iDataOffset;
			pMemData = m_pAddr + iDataOffset;
			memcpy(pMemData, pData, iLen);
			//修改头信息:记录数、数据偏移量、剩余空间大小、时间戳
			SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);
			++(m_pPage->m_iRecordCounts); 
			if(UpdateLsn){UpdatePageLSN();}
			TADD_DETAIL("page_id=[%d],offsetPos=[%d],rowid=[%d].",m_pPage->m_iPageID,iDataOffset,rowID.m_iRowID);
			
		}while(0);
		UnWLock();//解锁
		TADD_FUNC("Finish.");
		return iRet;
	}


    int TMdbPageCtrl::InsertData_NoMutex(unsigned char* pData, int iLen, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn)
    {
        TADD_FUNC("Start.");
        //检测参数是否合法
        if(pData==NULL || iLen<0 ||iLen>1024*1024*10)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Paramter invalid. pData=%p, iSize=%d.",pData, iLen);
            return ERR_APP_INVALID_PARAM;    
        }
        int iRet = 0;
        do
        {   //申请一块新空间
            int iDataOffset = 0;
            iRet = m_pPage->GetFreeRecord(iDataOffset,iLen);
            if(iRet == ERR_PAGE_NO_MEMORY)
            {//page没有空间了，无需报错
                break;
            }
            else if(0 != iRet)
            {//其他错误
                CHECK_RET_BREAK(iRet,"GetFreeRecord failed,RowId[%d|%d],size=[%d],page=[%s]",
                        rowID.GetPageID(),rowID.GetDataOffset(),iLen,m_pPage->ToString().c_str());
            }
            rowID.SetDataOffset(m_pPage->DataOffsetToRecordPos(iDataOffset));//设置数据位置
            rowID.SetPageID(m_pPage->m_iPageID);
            //rowID.iPageID = m_pPage->m_iPageID;
            //pMemData = m_pAddr + rowID.iDataOffset;
            pMemData = m_pAddr + iDataOffset;
            memcpy(pMemData, pData, iLen);
            //修改头信息:记录数、数据偏移量、剩余空间大小、时间戳
            SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);
            ++(m_pPage->m_iRecordCounts); 
            if(UpdateLsn){UpdatePageLSN();}
            TADD_DETAIL("page_id=[%d],offsetPos=[%d],rowid=[%d].",m_pPage->m_iPageID,iDataOffset,rowID.m_iRowID);
            
        }while(0);
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbPageCtrl::InsertData_NoMutex(unsigned char* pData, int iLen, int iSize, TMdbRowID& rowID,char * & pMemData,bool UpdateLsn)
    {
        TADD_FUNC("Start.");
        //检测参数是否合法
        if(pData==NULL || iLen<0 ||iLen>1024*1024*10)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Paramter invalid. pData=%p, iSize=%d.",pData, iLen);
            return ERR_APP_INVALID_PARAM;    
        }
        int iRet = 0;
        do
        {   //申请一块新空间
            int iDataOffset = 0;
            iRet = m_pPage->GetFreeRecord(iDataOffset,iSize);
            if(iRet == ERR_PAGE_NO_MEMORY)
            {//page没有空间了，无需报错
                break;
            }
            else if(0 != iRet)
            {//其他错误
                CHECK_RET_BREAK(iRet,"GetFreeRecord failed,RowId[%d|%d],size=[%d],page=[%s]",
                        rowID.GetPageID(),rowID.GetDataOffset(),iLen,m_pPage->ToString().c_str());
            }
            rowID.SetDataOffset(m_pPage->DataOffsetToRecordPos(iDataOffset));//设置数据位置
            rowID.SetPageID(m_pPage->m_iPageID);
            pMemData = m_pAddr + iDataOffset;
            memcpy(pMemData, pData, iLen);
            //修改头信息:记录数、数据偏移量、剩余空间大小、时间戳
            SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);
            ++(m_pPage->m_iRecordCounts); 
            if(UpdateLsn){UpdatePageLSN();}
            TADD_DETAIL("page_id=[%d],offsetPos=[%d],rowid=[%d].",m_pPage->m_iPageID,iDataOffset,rowID.m_iRowID);
            
        }while(0);
        TADD_FUNC("Finish.");
        return iRet;
    }
	
    /******************************************************************************
    * 函数名称	:  DeleteData()
    * 函数描述	:  删除页中一个数据    
    * 输入		:  iPos, 数据的节点位置  
    * 输出		:  无
    * 返回值	:  成功返回0，如果没有找到数据则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbPageCtrl::DeleteData(int iDataOffset,bool UpdateLsn)
    {
        TADD_FUNC("(iPos=%d) : Start.", iDataOffset);
        int iRet = 0;
        if(WLock() < 0)//加锁
        	return ERR_SQL_EXECUTE_TIMEOUT;
        do
        {
            CHECK_RET_BREAK(m_pPage->PushBack(m_pPage->RecordPosToDataOffset(iDataOffset)),"PushBack(%d) page=[%s] failed.",iDataOffset,m_pPage->ToString().c_str());//归还偏移
            //打印空闲节点的分布情况
            SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);//时间戳
            --m_pPage->m_iRecordCounts;
            if(UpdateLsn){UpdatePageLSN();}
        }while(0);
        UnWLock();
        TADD_FUNC("Finish.");
        return iRet;
    }
    int TMdbPageCtrl::DeleteData_NoMutex(int iDataOffset,bool UpdateLsn)
    {
        TADD_FUNC("(iPos=%d) : Start.", iDataOffset);
        int iRet = 0;
        do
        {
            CHECK_RET_BREAK(m_pPage->PushBack(m_pPage->RecordPosToDataOffset(iDataOffset)),"PushBack(%d) page=[%s] failed.",iDataOffset,m_pPage->ToString().c_str());//归还偏移
            //打印空闲节点的分布情况
            SAFESTRCPY(m_pPage->m_sUpdateTime,sizeof(m_pPage->m_sUpdateTime),m_pDSN->sCurTime);//时间戳
            --m_pPage->m_iRecordCounts;
            if(UpdateLsn){UpdatePageLSN();}
        }while(0);
        TADD_FUNC("Finish.");
        return iRet;
    }
//}

