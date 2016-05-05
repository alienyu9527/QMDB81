/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbTableCtrl.cpp		
*@Description�� �����
*@Author:		jin.shaohua
*@Date��	    2014/03/06
*@History:
******************************************************************************************/
#include "Control/mdbTableCtrl.h"

//namespace QuickMDB{
	/******************************************************************************
	* ��������	:  
	* ��������	: ����������
	* ����		:  
	* ���		:  
	* ����ֵ	: 
	* ����		:  jin.shaohua
	*******************************************************************************/
	TMdbTableCtrl::TMdbTableCtrl():m_pShmDsn(NULL),m_pTable(NULL)
	{

	}
	TMdbTableCtrl::~TMdbTableCtrl()
	{

	}
	/******************************************************************************
	* ��������	:  Init
	* ��������	: ��ʼ��
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
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
	* ��������	:  CreateTable
	* ��������	: ������
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::CreateTable(TMdbTable * pTable)
	{
		int iRet = 0;
		TADD_FUNC("Start.");
		CHECK_OBJ(m_pShmDsn);CHECK_OBJ(pTable);
		if(m_pShmDsn->GetTable(pTable->m_sTableName) != NULL)
		{//�Ѵ���
			CHECK_RET(ERR_DB_TABLE_IS_EXIST,"table=[%s]",pTable->m_sTableName);
		}
		CHECK_RET(m_pShmDsn->AddTable(pTable),"add table[%s] failed.",pTable->m_sTableName);

		//TODO:Ѱ������ı�ռ�
		//TODO:���ռ�����Ԥ������
		TADD_FUNC("Finish");
		return iRet;
	}
	/******************************************************************************
	* ��������	:  GetFreePage
	* ��������	: ��ȡ����ҳ
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::GetFreePage(TMdbPage * & pFreePage)
	{
		TADD_FUNC("Start,");
		int iRet = 0;
		CHECK_OBJ(m_pTable);
		pFreePage = NULL;
		//TODO:����
		int iFreePageID = m_pTable->m_iFreePageID;
		TADD_DETAIL("iFreePageID=[%d]",iFreePageID);
		do{
			if(iFreePageID > 0)
			{//������ҳ
				if((pFreePage = m_tTSCtrl.GetPage(iFreePageID)) == NULL)
				{
					CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"GetPage[%d] failed",iFreePageID);
				}
			}
			else
			{//û������ҳ��
				TADD_DETAIL("no free pages");
				CHECK_RET_BREAK(m_tTSCtrl.GetEmptyPage(pFreePage),"GetEmptyPage failed");
				if(NULL != pFreePage)
				{
					CHECK_RET_BREAK(AddPageToTop(pFreePage,m_pTable->m_iFreePageID),"AddPageToTop failed.");
					SAFESTRCPY(pFreePage->m_sState,sizeof(pFreePage->m_sState),"free");//�޸�״̬

					pFreePage->m_iRecordSize = 0;//TODO:�ӱ�ռ��ȡҳ��ʱ���ȷ�����ҳ�м�¼��С
					m_pTable->m_iFreePages ++;
					pFreePage->m_iNextPageID = m_pTable->m_iTablePageID;
					m_pTable->m_iTablePageID = pFreePage->m_iPageID;
				}
			}
		}while(0);
		return iRet;
	}
	/******************************************************************************
	* ��������	:  TablePageFreeToFull
	* ��������	: ҳ��������������
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::TablePageFreeToFull(TMdbPage* pCurPage)
	{
		int iRet = 0;
		TADD_FUNC("Start.");
		//��free�����Ƴ�
		CHECK_RET(RemovePage(pCurPage,m_pTable->m_iFreePageID),"RemovePage failed.");
		//��ӵ�full����
		CHECK_RET(AddPageToTop(pCurPage,m_pTable->m_iFullPageID),"AddPageToTop failed.");
		TADD_DETAIL("Cur-Page-ID=%d, NextPageID=%d, PrePageID=%d.", pCurPage->m_iPageID, pCurPage->m_iPrePageID, pCurPage->m_iNextPageID);
		//��״̬����Ϊfull
		SAFESTRCPY(pCurPage->m_sState,sizeof(pCurPage->m_sState), "full");
		++m_pTable->m_iFullPages;
		--m_pTable->m_iFreePages;
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* ��������	:  TablePageFullToFree
	* ��������	:  ҳ��������������
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::TablePageFullToFree(TMdbPage* pCurPage)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		//��full�����Ƴ�
		CHECK_RET(RemovePage(pCurPage,m_pTable->m_iFullPageID),"RemovePage failed.");
		//��ӵ�free����
		CHECK_RET(AddPageToTop(pCurPage,m_pTable->m_iFreePageID),"AddPageToTop failed.");
		//��״̬����Ϊfree
		SAFESTRCPY(pCurPage->m_sState, sizeof(pCurPage->m_sState),"free");
		--m_pTable->m_iFullPages;
		++m_pTable->m_iFreePages;
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* ��������	:  RemovePage
	* ��������	: �Ƴ�һ��page
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableCtrl::RemovePage(TMdbPage * pPageToRemove,int & iHeadPageId)
	{
		TADD_FUNC("Start.iHeadPageId=[%d],page:pre[%d] next[%d]",iHeadPageId,pPageToRemove->m_iPrePageID,pPageToRemove->m_iNextPageID);
		int iRet  = 0;
		CHECK_OBJ(pPageToRemove);
		if(pPageToRemove->m_iPrePageID <= 0)
		{//ͷ�ڵ�
			iHeadPageId = pPageToRemove->m_iNextPageID;
		}
		else
		{//��ͷ�ڵ�
			TMdbPage * pPrePage = m_tTSCtrl.GetPage(pPageToRemove->m_iPrePageID);
			CHECK_OBJ(pPrePage);
			pPrePage->m_iNextPageID = pPageToRemove->m_iNextPageID;
		}
		if(pPageToRemove->m_iNextPageID > 0)
		{//��β�ڵ�
			TMdbPage * pNextPage = m_tTSCtrl.GetPage(pPageToRemove->m_iNextPageID);
			CHECK_OBJ(pNextPage);
			pNextPage->m_iPrePageID = pPageToRemove->m_iPrePageID;
		}
		//�����page
		pPageToRemove->m_iPrePageID  = -1;
		pPageToRemove->m_iNextPageID = -1;
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* ��������	:  AddPageToTop
	* ��������	: ��page��ӵ�ͷ
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
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