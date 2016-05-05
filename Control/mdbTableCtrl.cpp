/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbTableCtrl.cpp		
*@Description： 表管理
*@Author:		jin.shaohua
*@Date：	    2014/03/06
*@History:
******************************************************************************************/
#include "Control/mdbTableCtrl.h"

//namespace QuickMDB{
	/******************************************************************************
	* 函数名称	:  
	* 函数描述	: 构造与析构
	* 输入		:  
	* 输出		:  
	* 返回值	: 
	* 作者		:  jin.shaohua
	*******************************************************************************/
	TMdbTableCtrl::TMdbTableCtrl():m_pShmDsn(NULL),m_pTable(NULL)
	{

	}
	TMdbTableCtrl::~TMdbTableCtrl()
	{

	}
	/******************************************************************************
	* 函数名称	:  Init
	* 函数描述	: 初始化
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::Init(const char * sDsn,const char * sTablename)
	{
		int iRet = 0;
		CHECK_OBJ(sDsn);
		m_pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
		CHECK_OBJ(m_pShmDsn);
		if(NULL != sTablename)
		{
			m_pTable = m_pShmDsn->GetTable(sTablename);
			CHECK_OBJ(m_pTable);
			CHECK_RET(m_tTSCtrl.Init(sDsn,m_pTable->m_sTablespaceName),"init failed.");
		}
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  CreateTable
	* 函数描述	: 创建表
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::CreateTable(TMdbTable * pTable)
	{
		int iRet = 0;
		TADD_FUNC("Start.");
		CHECK_OBJ(m_pShmDsn);CHECK_OBJ(pTable);
		if(m_pShmDsn->GetTable(pTable->m_sTableName) != NULL)
		{//已存在
			CHECK_RET(ERR_DB_TABLE_IS_EXIST,"table=[%s]",pTable->m_sTableName);
		}
		CHECK_RET(m_pShmDsn->AddTable(pTable),"add table[%s] failed.",pTable->m_sTableName);

		//TODO:寻找适配的表空间
		//TODO:向表空间申请预留数据
		TADD_FUNC("Finish");
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  GetFreePage
	* 函数描述	: 获取自由页
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::GetFreePage(TMdbPage * & pFreePage)
	{
		TADD_FUNC("Start,");
		int iRet = 0;
		CHECK_OBJ(m_pTable);
		pFreePage = NULL;
		//TODO:加锁
		int iFreePageID = m_pTable->m_iFreePageID;
		TADD_DETAIL("iFreePageID=[%d]",iFreePageID);
		do{
			if(iFreePageID > 0)
			{//有自由页
				if((pFreePage = m_tTSCtrl.GetPage(iFreePageID)) == NULL)
				{
					CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"GetPage[%d] failed",iFreePageID);
				}
			}
			else
			{//没有自由页了
				TADD_DETAIL("no free pages");
				CHECK_RET_BREAK(m_tTSCtrl.GetEmptyPage(pFreePage),"GetEmptyPage failed");
				if(NULL != pFreePage)
				{
					CHECK_RET_BREAK(AddPageToTop(pFreePage,m_pTable->m_iFreePageID),"AddPageToTop failed.");
					SAFESTRCPY(pFreePage->m_sState,sizeof(pFreePage->m_sState),"free");//修改状态

					pFreePage->m_iRecordSize = 0;//TODO:从表空间获取页的时候才确定这个页中记录大小
					m_pTable->m_iFreePages ++;
					pFreePage->m_iNextPageID = m_pTable->m_iTablePageID;
					m_pTable->m_iTablePageID = pFreePage->m_iPageID;
				}
			}
		}while(0);
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  TablePageFreeToFull
	* 函数描述	: 页从自由链到满链
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::TablePageFreeToFull(TMdbPage* pCurPage)
	{
		int iRet = 0;
		TADD_FUNC("Start.");
		//从free链上移除
		CHECK_RET(RemovePage(pCurPage,m_pTable->m_iFreePageID),"RemovePage failed.");
		//添加到full链上
		CHECK_RET(AddPageToTop(pCurPage,m_pTable->m_iFullPageID),"AddPageToTop failed.");
		TADD_DETAIL("Cur-Page-ID=%d, NextPageID=%d, PrePageID=%d.", pCurPage->m_iPageID, pCurPage->m_iPrePageID, pCurPage->m_iNextPageID);
		//把状态设置为full
		SAFESTRCPY(pCurPage->m_sState,sizeof(pCurPage->m_sState), "full");
		++m_pTable->m_iFullPages;
		--m_pTable->m_iFreePages;
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  TablePageFullToFree
	* 函数描述	:  页从满链到自由链
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::TablePageFullToFree(TMdbPage* pCurPage)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		//从full链上移除
		CHECK_RET(RemovePage(pCurPage,m_pTable->m_iFullPageID),"RemovePage failed.");
		//添加到free链上
		CHECK_RET(AddPageToTop(pCurPage,m_pTable->m_iFreePageID),"AddPageToTop failed.");
		//把状态设置为free
		SAFESTRCPY(pCurPage->m_sState, sizeof(pCurPage->m_sState),"free");
		--m_pTable->m_iFullPages;
		++m_pTable->m_iFreePages;
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  RemovePage
	* 函数描述	: 移除一个page
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::RemovePage(TMdbPage * pPageToRemove,int & iHeadPageId)
	{
		TADD_FUNC("Start.iHeadPageId=[%d],page:pre[%d] next[%d]",iHeadPageId,pPageToRemove->m_iPrePageID,pPageToRemove->m_iNextPageID);
		int iRet  = 0;
		CHECK_OBJ(pPageToRemove);
		if(pPageToRemove->m_iPrePageID <= 0)
		{//头节点
			iHeadPageId = pPageToRemove->m_iNextPageID;
		}
		else
		{//非头节点
			TMdbPage * pPrePage = m_tTSCtrl.GetPage(pPageToRemove->m_iPrePageID);
			CHECK_OBJ(pPrePage);
			pPrePage->m_iNextPageID = pPageToRemove->m_iNextPageID;
		}
		if(pPageToRemove->m_iNextPageID > 0)
		{//非尾节点
			TMdbPage * pNextPage = m_tTSCtrl.GetPage(pPageToRemove->m_iNextPageID);
			CHECK_OBJ(pNextPage);
			pNextPage->m_iPrePageID = pPageToRemove->m_iPrePageID;
		}
		//清理该page
		pPageToRemove->m_iPrePageID  = -1;
		pPageToRemove->m_iNextPageID = -1;
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  AddPageToTop
	* 函数描述	: 将page添加到头
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::AddPageToTop(TMdbPage * pPageToAdd,int & iHeadPageId)
	{
		TADD_FUNC("Start.iHeadPageId=[%d],page:pre[%d] next[%d]",iHeadPageId,pPageToAdd->m_iPrePageID,pPageToAdd->m_iNextPageID);
		int iRet = 0;
		if(iHeadPageId > 0)
		{
			TMdbPage * pNextPage    = m_tTSCtrl.GetPage(iHeadPageId);
			CHECK_OBJ(pNextPage);
			pNextPage->m_iPrePageID        = pPageToAdd->m_iPageID;
			pPageToAdd->m_iNextPageID = iHeadPageId;
			pPageToAdd->m_iPrePageID   = -1;
			iHeadPageId = pPageToAdd->m_iPageID;
		}
		else
		{
			iHeadPageId = pPageToAdd->m_iPageID;
			pPageToAdd->m_iNextPageID = -1;
			pPageToAdd->m_iPrePageID   = -1;
		}
		TADD_FUNC("Finish.");
		return iRet;
	}
//}