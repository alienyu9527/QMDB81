/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbTableSpaceCtrl.cpp
*@Description�� �������ĳ����ı�ռ���Ϣ����ȡһ������ҳ������ҳ
*@Author:		li.shugang
*@Date��	    2008��12��01��
*@History:
******************************************************************************************/
#include "Control/mdbTableSpaceCtrl.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbDateTime.h"
#include "Control/mdbPageCtrl.h"
#include "Control/mdbStorageEngine.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{


    TMdbTableSpaceCtrl::TMdbTableSpaceCtrl()
    {
        memset(m_sTableSpaceName, 0, sizeof(m_sTableSpaceName));
        m_pTMdbTableSpace = NULL;
        m_pDsn            = NULL;
		m_pTSFile         = NULL;
    }


    TMdbTableSpaceCtrl::~TMdbTableSpaceCtrl()
    {
		SAFE_CLOSE(m_pTSFile);
    }


    /******************************************************************************
    * ��������	:  Init
    * ��������	:  ��ʼ��
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableSpaceCtrl::Init(const char * pszDsn,char* psTsName)
    {
        int iRet = 0;
        CHECK_OBJ(pszDsn);
        m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDsn);
        CHECK_OBJ(m_pShmDSN);
        m_pTMdbTableSpace = m_pShmDSN->GetTableSpaceAddrByName(psTsName);
        CHECK_OBJ(m_pTMdbTableSpace);
		SAFESTRCPY(m_sTableSpaceName, sizeof(m_sTableSpaceName), psTsName);
        m_pDsn = m_pShmDSN->GetInfo();
        CHECK_OBJ(m_pDsn);
		m_iHasTableChange = 0;
		m_pTSFile = NULL;
        return iRet;
    }

    /******************************************************************************
    * ��������	:  CreateTableSpace()
    * ��������	:  ������ռ�
    * ����		:  pTableSpace, ��ռ�
    * ����		:  pConfig, ������Ϣ
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbTableSpaceCtrl::CreateTableSpace(TMdbTableSpace* pTableSpace, TMdbConfig* pConfig, bool bAddToFile)
    {
        TADD_FUNC(" Start.");
        int iRet = 0;
        //������
        CHECK_OBJ(pConfig);
        CHECK_OBJ(pTableSpace);
        m_pShmDSN = TMdbShmMgr::GetShmDSN(pConfig->GetDSN()->sName);
        CHECK_OBJ(m_pShmDSN);
        //ȡ��DSN������Ϣ
        m_pDsn = m_pShmDSN->GetInfo();
        CHECK_OBJ(m_pDsn);
        TMdbTableSpace * pNewTS = NULL;
        CHECK_RET(m_pShmDSN->AddNewTableSpace(pNewTS),"AddNewTableSpace");
        CHECK_OBJ(pNewTS);
        //���������б�ռ���ڴ���
        memcpy(pNewTS, pTableSpace, sizeof(TMdbTableSpace));
        m_pTMdbTableSpace = pNewTS;
        long long llDataSize = 0;
        CHECK_RET(pConfig->GetOneTableSpaceDataSize(m_pTMdbTableSpace->sName,llDataSize),"GetOneTableSpaceDataSize failed.");
        if(llDataSize < 0)
        {
          CHECK_RET(ERR_APP_INVALID_PARAM,"GetDataSize()[%lld].",llDataSize);
        }
        llDataSize = (llDataSize+pTableSpace->iPageSize-1)/pTableSpace->iPageSize * pTableSpace->iPageSize;
        int iDataMinPage = llDataSize/pTableSpace->iPageSize;//�������������pages
        int iAskPageCount = iDataMinPage < pTableSpace->iRequestCounts? pTableSpace->iRequestCounts:iDataMinPage;

        //������ռ������������Ϣ
        pNewTS->tEmptyMutex.Create();
        //pNewTS->m_tFreeMutex.Create();
        //pNewTS->m_tFullMutex.Create();
        pNewTS->iEmptyPages  = 0;
        pNewTS->iTotalPages  = 0;
        pNewTS->iEmptyPageID = -1;
        pNewTS->cState       = TableSpace_using;
        TMdbDateTime::GetCurrentTimeStr(pNewTS->sCreateTime);
        TADD_FLOW("Table-Space=[%s],iDataMinPage=[%d],iAskPageCount=[%d]",
            pTableSpace->sName, iDataMinPage,iAskPageCount);
        CHECK_RET(AskNewPages(iAskPageCount, bAddToFile),"AskNewPages(%d) failed",iAskPageCount);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GetFreePage()
    * ��������	:  ��ȡһ������ҳ, ���û������ҳ, �򷵻�һ������ҳ:����ҳ����ݱ�ͬ����ͬ����һ��Ϳ���ҳ��һ��
    * ����		:  pTable, ����ҳ��Ӧ��ĳ����
    * ���		:
    * ����ֵ	:  �ɹ���������ҳ/����ҳ���, ���򷵻�NULL
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbTableSpaceCtrl::GetFreePage(TMdbTable* pTable,TMdbPage * & pFreePage, bool bAddToFile)
    {
        
        int iRet = 0;
        CHECK_OBJ(m_pTMdbTableSpace);
        CHECK_OBJ(pTable);
        TADD_DETAIL("TS[%s]TAB[%s],GetFreePage BEGIN",m_pTMdbTableSpace->sName, pTable->sTableName);
        pFreePage = NULL;
        CHECK_RET(pTable->tFreeMutex.Lock(true, &m_pDsn->tCurTime),"[%s].tFreeMutex.Lock() failed.",pTable->sTableName);
        //int iFreePageID = GetTableFreePageId(pTable);
        TADD_DETAIL("iFreePageID=[%d]",pTable->iFreePageID);
        do{
            if(pTable->iFreePageID >0 && pTable->iFreePages > 100)
            {//������ҳ
                if((pFreePage = (TMdbPage *)GetAddrByPageID(pTable->iFreePageID)) == NULL)
                {
                      CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"GetAddrByPageID[%d] failed",pTable->iFreePageID);
                }
				pTable->iFreePageID = pFreePage->m_iNextPageID;
            }
            else
            {//û������ҳ��
                TADD_DETAIL("no free pages");
                int iReqCounts = pTable->GetFreePageRequestCount();
                if(iReqCounts > m_pTMdbTableSpace->iRequestCounts)
                {
                    iReqCounts= m_pTMdbTableSpace->iRequestCounts;
                }
                
                do
                {
                    iReqCounts--;
                    CHECK_RET_BREAK(GetEmptyPage(pTable,pFreePage,bAddToFile),"GetEmptyPage failed");
                    if(NULL != pFreePage)
                    {
                        //CHECK_RET(m_pTMdbTableSpace->m_tFreeMutex.Lock(true, &m_pDsn->tCurTime),"[%s].m_tFreeMutex.Lock() failed.",m_pTMdbTableSpace->sName);
                        iRet = AddPageToCircle(pFreePage,pTable->iFreePageID);
                        
                        CHECK_RET_BREAK(iRet,"AddPageToTop failed.");
                        
                        SAFESTRCPY(pFreePage->m_sState,sizeof(pFreePage->m_sState),"free");//�޸�״̬
                        pFreePage->m_iRecordSize = pTable->iOneRecordSize;//�ӱ�ռ��ȡҳ��ʱ���ȷ�����ҳ�м�¼��С
                        memcpy(pFreePage->m_sTableName,pTable->sTableName,sizeof(pFreePage->m_sTableName));
                        //m_pTMdbTableSpace->m_tFreeMutex.UnLock(true, m_pDsn->sCurTime);
                        
                        pTable->iFreePages ++;
						pTable->iFreePageID = pFreePage->m_iNextPageID;
                    }
                    
                }while(iReqCounts> 0);
            }
        }while(0);
        pTable->tFreeMutex.UnLock(true, m_pDsn->sCurTime);
        if(NULL != pFreePage)
        {
            TADD_FUNC("Finish.GetFreePage[%d],pre[%d],next[%d]",pFreePage->m_iPageID,pFreePage->m_iPrePageID,pFreePage->m_iNextPageID);
        }
        else
        {
            TADD_FUNC("Finish.GetFreePage[NULL]");
        }

        TADD_DETAIL("TS[%s]TAB[%s],GetFreePage FINISH",m_pTMdbTableSpace->sName, pTable->sTableName);
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GetEmptyPage()
    * ��������	:  ��ȡһ������ҳ, ���û�п���ҳ, �������µĿ���ҳ, �����������
    * ����		:  ��
    * ���		:  iEmptyPage, ȡ���Ŀ���ҳ
    * ����ֵ	:  �ɹ����ؿ���ҳ���, ���򷵻�NULL
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbTableSpaceCtrl::GetEmptyPage(TMdbTable* pTable,TMdbPage * & pEmptyPage,bool bAddToFile)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(m_pTMdbTableSpace);
        pEmptyPage = NULL;
        CHECK_RET(m_pTMdbTableSpace->tEmptyMutex.Lock(true, &m_pDsn->tCurTime),"tEmptyMutex.Lock() failed.");
        do{
            TADD_DETAIL("iEmptyPageID=%d.", m_pTMdbTableSpace->iEmptyPageID);
            if(m_pTMdbTableSpace->iEmptyPageID <= 0)
            {//û�п���ҳ����Ҫ����
                CHECK_RET_BREAK(AskNewPages(m_pTMdbTableSpace->iRequestCounts, bAddToFile),"AskNewPages(%d) error.",
                                                        m_pTMdbTableSpace->iRequestCounts);
                if(m_pTMdbTableSpace->iEmptyPageID <= 0)
                {
                    CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"m_pTMdbTableSpace->iEmptyPageID[%d] error.",
                                                        m_pTMdbTableSpace->iEmptyPageID);
                }
            }
            CHECK_RET_BREAK(GetPageFromTop(pEmptyPage,m_pTMdbTableSpace->iEmptyPageID),"GetPageFromTop failed.");
            if(NULL == pEmptyPage)
            {
                CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"GetAddrByPageID[%d] error.",m_pTMdbTableSpace->iEmptyPageID);
            }
            CHECK_RET_BREAK(RemovePage(pEmptyPage,m_pTMdbTableSpace->iEmptyPageID),"RemovePage failed.");
            TADD_DETAIL("iEmptyPageID=%d.", m_pTMdbTableSpace->iEmptyPageID);
            IS_LOG(3)
            {
                TMdbPage * pTemp = NULL;
                GetPageFromTop(pTemp,m_pTMdbTableSpace->iEmptyPageID);
                if(NULL != pTemp)
                {
                    TADD_DETAIL("pTemp[%d][%d][%d],",pTemp->m_iPageID,pTemp->m_iPrePageID,pTemp->m_iNextPageID);
                }
                else
                {
                    TADD_DETAIL("[%d] no page found.",m_pTMdbTableSpace->iEmptyPageID);
                }
                
            }
            --m_pTMdbTableSpace->iEmptyPages;
        }while(0);
        m_pTMdbTableSpace->tEmptyMutex.UnLock(true, m_pDsn->sCurTime);
        if(NULL != pEmptyPage)
        {
            TADD_FUNC("Finish.pEmptyPage[%d],pre[%d],next[%d]",pEmptyPage->m_iPageID,pEmptyPage->m_iPrePageID,pEmptyPage->m_iNextPageID);
        }
        else
        {
            TADD_FUNC("Finish.pEmptyPage[NULL]");
        }

        return iRet;
    }
    /******************************************************************************
    * ��������	:  GetEmptyPageByPageID()
    * ��������	:  ��ȡһ��ָ��ҳ�ŵĿ���ҳ
    * ����		:  ��
    * ���		:  iEmptyPage, ȡ���Ŀ���ҳ
    * ����ֵ	:  �ɹ����ؿ���ҳ���, ���򷵻�NULL
    * ����		:  dong.chun
    *******************************************************************************/
    int TMdbTableSpaceCtrl::GetEmptyPageByPageID(TMdbPage * & pEmptyPage, int iPageId, bool bAddToFile)
    {//ҳ�Ż������ڵ�����£�Ҫ����ռ��Ի�ȡ����ҳ��,ע������ռ�ĺ������ж�
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(m_pTMdbTableSpace);
        pEmptyPage = NULL;
        CHECK_RET(m_pTMdbTableSpace->tEmptyMutex.Lock(true, &m_pDsn->tCurTime),"tEmptyMutex.Lock() failed.");
        do{
            TADD_DETAIL("iEmptyPageID=%d.", iPageId);
            if(iPageId <= 0)
            {
                CHECK_RET_BREAK(ERR_APP_PARAM_INVALID,"iPageId(%d) error.",iPageId);
            }
			if(iPageId > m_pTMdbTableSpace->iTotalPages)
			{
				int iAskCount = (iPageId - m_pTMdbTableSpace->iTotalPages)/m_pTMdbTableSpace->iRequestCounts + 1;
				for (int i = 0; i < iAskCount; i++)
				{
					CHECK_RET_BREAK(AskNewPages(m_pTMdbTableSpace->iRequestCounts, false),"AskNewPages(%d) error.", m_pTMdbTableSpace->iRequestCounts);
				}
			}
            CHECK_RET_BREAK(GetPageFromTop(pEmptyPage,iPageId),"GetPageFromTop failed.");
            if(NULL == pEmptyPage)
            {
                CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"GetAddrByPageID[%d] error.",iPageId);
            }

            if(TMdbNtcStrFunc::StrNoCaseCmp(pEmptyPage->m_sState,"empty") != 0)
            {
                CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"Page State[%d] Not Empty.",iPageId);
            }
            CHECK_RET_BREAK(RemovePage(pEmptyPage,m_pTMdbTableSpace->iEmptyPageID),"RemovePage failed.");
            --m_pTMdbTableSpace->iEmptyPages;
        }while(0);
        if(m_pTMdbTableSpace->tEmptyMutex.UnLock(true, m_pDsn->sCurTime) != 0)
        {
            TADD_ERROR(-1,"UnLock failed");
            return -1;
        }
        if(NULL != pEmptyPage)
        {
            TADD_FUNC("Finish.pEmptyPage[%d],pre[%d],next[%d]",pEmptyPage->m_iPageID,pEmptyPage->m_iPrePageID,pEmptyPage->m_iNextPageID);
        }
        else
        {
            TADD_FUNC("Finish.pEmptyPage[NULL]");
        }

        return iRet;
    }
    //��ʼ�������ҳ�棺�������������
    int TMdbTableSpaceCtrl::InitPage(char* pAddr, int iPageSID, int iPageFID, size_t iSize)
    {
        TADD_FUNC("Start.iPageSID[%d],iPageFID[%d],(iSize=%lu) : ", iPageSID,iPageFID,iSize);
        int iPrePageID = -1;
        //����Emptyҳ�������, ���û��Emptyҳ�棬ֱ�Ӱ�StartPageID��������������һ��Emptyҳ���iNextPageID����ΪiPageSID
        if(m_pTMdbTableSpace->iEmptyPageID <= 0)
        {
            m_pTMdbTableSpace->iEmptyPageID = iPageSID;
        }
        else
        {
            int iLastEmptyPageID = m_pTMdbTableSpace->iEmptyPageID;
            TMdbPage *pPage = NULL;
            while(true)
            {
                pPage = (TMdbPage*)GetAddrByPageID(iLastEmptyPageID);
                CHECK_OBJ(pPage);
                if(pPage->m_iNextPageID <= 0)
                {
                    pPage->m_iNextPageID = iPageSID;
                    iPrePageID = iLastEmptyPageID;
                    TADD_DETAIL("iLastEmptyPageID=[%d],iPrePageID=[%d]",iLastEmptyPageID,iPrePageID);
                    break;
                }
                else
                {
                    iLastEmptyPageID = pPage->m_iNextPageID;
                }
            }
        }
        TADD_DETAIL("(iSize=%lu) : iEmptyPageID=%d.", iSize, m_pTMdbTableSpace->iEmptyPageID);
        //��ʼ��ÿһ��ҳ
        TMdbPage *pPage = (TMdbPage*)pAddr;
        for(int i=iPageSID; i<=iPageFID; ++i)
        {
            pPage->Init(i,iSize);//��ʼ��
            if(i == iPageSID)
            {
                pPage->m_iPrePageID = iPrePageID;
                if(i != iPageFID)
                    pPage->m_iNextPageID = i+1;
                else
                {
                    pPage->m_iNextPageID = -1;
                }
            }
            else if(i != iPageFID)
            {
                pPage->m_iPrePageID  = i-1;
                pPage->m_iNextPageID = i+1;
            }
            else
            {
                pPage->m_iPrePageID  = i-1;
                pPage->m_iNextPageID = -1;
            }
            TADD_DETAIL("(iSize=%lu) : PageAddr=%p, iPageID=%d, iNextPageID=%d.", iSize, pPage, pPage->m_iPageID, pPage->m_iNextPageID);
            pPage = (TMdbPage*)(pAddr + iSize * (i+1-iPageSID));
            ++m_pTMdbTableSpace->iTotalPages;
            ++m_pTMdbTableSpace->iEmptyPages;
        }
        TADD_DETAIL("(iSize=%lu) : iEmptyPageID=%d.", iSize, m_pTMdbTableSpace->iEmptyPageID);
        TADD_FUNC("Finish.");
        return 0;
    }


    //�ѽڵ�����ռ䣬��������
    int TMdbTableSpaceCtrl::AddNode(TMdbTableSpace* pTSTmp, TMdbTSNode &nod)
    {
        TADD_FUNC("Start.nod:pageStart[%d],pageEnd[%d]",nod.iPageStart,nod.iPageEnd);
        int iRet = 0;
        CHECK_OBJ(m_pShmDSN);
        m_pDsn = m_pShmDSN->GetInfo();//ȡ��DSN������Ϣ
        //�жϹ������ռ��Ƿ��㹻
        TMdbConfig * pConfig = TMdbConfigMgr::GetMdbConfig(m_pDsn->sName);
        CHECK_OBJ(pConfig);
        if(m_pShmDSN->GetUsedSize() >= pConfig->GetDSN()->iManagerSize - sizeof(TMdbTSNode))
        {//�����ڴ淶Χ
            CHECK_RET(ERR_OS_NO_MEMROY,"m_pShmDSN->GetFreeSize[%luMB] > iManagerSize[%luMB]",
                m_pShmDSN->GetUsedSize()/(1024*1024),pConfig->GetDSN()->iManagerSize/(1024*1024));
        }
        CHECK_OBJ(m_pDsn);
        CHECK_RET(m_pDsn->tMutex.Lock(true, &m_pDsn->tCurTime),"m_pDsn->tMutex.Lock failed");    //����
        
        TMdbTSNode* pNode = (TMdbTSNode*)&pTSTmp->tNode;
        while(1){
            TMdbTSNode * pNext =  GetNextNode(pNode);
            if(NULL == pNext){break;}
            pNode = pNext;
        }//�ҵ����һ���ڵ�
        /*
        while(pNode->iPageEnd > 0 && pNode->iNext > 0)
        {
            pNode = GetNextNode(pNode);//(TMdbTSNode*)(m_pShmDSN->GetAddrByOffset(pNode->iNext));//(TMdbTSNode*)(pAddr + pNode->iNext);
        }
        */
        //���iPageEnd>0��˵���Ѿ��нڵ���
        if(pNode->iPageEnd > 0)
        {
            pNode =  (TMdbTSNode*)m_pShmDSN->Alloc(sizeof(TMdbTSNode),pNode->iNext);
            //�����ݷŽ�ȥ
            memcpy(pNode, &nod, sizeof(TMdbTSNode));
        }
        else
        {
            memcpy(pNode, &nod, sizeof(TMdbTSNode));
        }
        //����
        m_pDsn->tMutex.UnLock(true, m_pDsn->sCurTime);

        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * ��������	:  GetAddrByPageID()
    * ��������	:  ����һ���µ�ҳ��
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����ʵ�ʵ�ַ�����򷵻�NULL
    * ����		:  li.shugang
    *******************************************************************************/
    char* TMdbTableSpaceCtrl::GetAddrByPageID(int iPageID)
    {
        if(m_pTMdbTableSpace == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM," m_pTMdbTableSpace == NULL.");
            return NULL;
        }
        char* pAddr = NULL;
        if(iPageID > 0)
        {
            //ȡ������ҳ�ĵ�ַ
            TMdbTSNode* pNodeTmp = &m_pTMdbTableSpace->tNode;
            while(true)
            {
                //���ҳ���ڵ�ǰ�ķ�Χ��, ��ת�����ڲ���ַ
                if(iPageID >= pNodeTmp->iPageStart && iPageID <= pNodeTmp->iPageEnd)
                {
                    //ȡ�������ڴ�εĵ�ַ
                    pAddr = m_pShmDSN->GetDataShmAddr(pNodeTmp->iShmID);
                    if(pAddr == NULL)
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"Can't GetShmAddr(iShmID=%d)", pNodeTmp->iShmID);
                        return NULL;
                    }
                    //�������Ӧҳ��ĵ�ַ
                    pAddr += pNodeTmp->iOffSet + (iPageID-pNodeTmp->iPageStart)*m_pTMdbTableSpace->iPageSize;
                    break;
                }
                else
                {
                    //�������û�нڵ�, ˵��PageIDû��ע��, ֱ�ӱ���
                    if(0 == pNodeTmp->iNext )
                    {
                        TADD_ERROR(ERROR_UNKNOWN,"pNodeTmp has no next,Can't find PageID=%d", iPageID);
                        return NULL;
                    }
                    else
                    {
                        pNodeTmp = GetNextNode(pNodeTmp);
                    }
                }
            }
			if (NULL == pAddr)
			{
				TADD_ERROR(ERROR_UNKNOWN,"Searched all TSNodes ,still find no page-id=%d.",iPageID);
			}
        }
        else
        {
            //���û����Ӧҳ��, ����NULL
            TADD_ERROR(ERROR_UNKNOWN,"Invalid page id <= 0 ,page-id=%d.",iPageID);
            return NULL;
        }
        return pAddr;
    }


    /******************************************************************************
    * ��������	:  PushBackPage()
    * ��������	:  �ѵ�ǰҳ��黹����ռ䣬����ҳ�����
    * ����		:  pPage�������յ�ҳ��
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  li.shugang
    *******************************************************************************/
    void TMdbTableSpaceCtrl::PushBackPage(TMdbPage* pPage)
    {
        TADD_FUNC("Start.");

        if(pPage == NULL)
        {
            TADD_FUNC("Finish(pPage==NULL).");
            return;
        }
        m_pTMdbTableSpace->tEmptyMutex.Lock(true, &m_pDsn->tCurTime);
        pPage->Init(pPage->m_iPageID,pPage->m_iPageSize);
        AddPageToTop(pPage,m_pTMdbTableSpace->iEmptyPageID);
        m_pTMdbTableSpace->iEmptyPages++;
        m_pTMdbTableSpace->tEmptyMutex.UnLock(true, m_pDsn->sCurTime);
        TADD_FUNC("Finish.");
    }


    /******************************************************************************
    * ��������	:  TablePageFreeToFull
    * ��������	:  �ѵ�ǰҳ���Free-Page�ƶ���Full-Page
    * ����		:  pTable -  ��Ҫ�ƶ��ı�pCurPage - ��Ҫ�ƶ���page
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableSpaceCtrl::TablePageFreeToFull(TMdbTable * pTable,TMdbPage* pCurPage)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

		CHECK_OBJ(pCurPage);
        //��free�����Ƴ�
        CHECK_RET(pTable->tFreeMutex.Lock(true, &m_pDsn->tCurTime),"[%s].tFreeMutex.Lock() failed.",pTable->sTableName);
		do
		{
			//move �� full ״̬�����ٲ���
			if(0 != strcmp(pCurPage->m_sState,"free"))
			{
				TADD_FLOW("Page may be moving in other thread, will move again.");
				pTable->tFreeMutex.UnLock(true, m_pDsn->sCurTime);
				return iRet;
			}
	        CHECK_RET_BREAK(RemovePageFromCircle(pCurPage,pTable->iFreePageID),"RemovePageFromCircle failed.");
			//��״̬���move
	        SAFESTRCPY(pCurPage->m_sState,sizeof(pCurPage->m_sState), "move");
			--pTable->iFreePages;
		}while(0);
        pTable->tFreeMutex.UnLock(true, m_pDsn->sCurTime);

         //��ӵ�full����
        CHECK_RET(pTable->tFullMutex.Lock(true, &m_pDsn->tCurTime),"[%s].tFreeMutex.Lock() failed.",pTable->sTableName);
		do
		{
	        CHECK_RET_BREAK(AddPageToTop(pCurPage,pTable->iFullPageID),"AddPageToTop failed.");
	        //��״̬����Ϊfull
	        SAFESTRCPY(pCurPage->m_sState,sizeof(pCurPage->m_sState), "full");
	        ++pTable->iFullPages;
		}while(0);

        pTable->tFullMutex.UnLock(true, m_pDsn->sCurTime);
        
        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * ��������	:  TablePageFullToFree
    * ��������	:  �ѵ�ǰҳ���Full-Page�ƶ���Free-Page
    * ����		:  pTable -  ��Ҫ�ƶ��ı�pCurPage - ��Ҫ�ƶ���page
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableSpaceCtrl::TablePageFullToFree(TMdbTable * pTable,TMdbPage* pCurPage)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //��full�����Ƴ�
        CHECK_RET(pTable->tFullMutex.Lock(true, &m_pDsn->tCurTime),"[%s].tFreeMutex.Lock() failed.",pTable->sTableName);
        do
        {
        	if(0 != strcmp(pCurPage->m_sState,"full"))
			{
				TADD_FLOW("Page may be moving in other thread, will move again.");
				pTable->tFreeMutex.UnLock(true, m_pDsn->sCurTime);
				return iRet;
			}
            CHECK_RET_BREAK(RemovePage(pCurPage,pTable->iFullPageID),"RemovePage failed.");
	        SAFESTRCPY(pCurPage->m_sState,sizeof(pCurPage->m_sState), "move");
            --pTable->iFullPages;
        }while(0);
        pTable->tFullMutex.UnLock(true, m_pDsn->sCurTime);
        
        //��ӵ�free����
        CHECK_RET(pTable->tFreeMutex.Lock(true, &m_pDsn->tCurTime),"[%s].tFreeMutex.Lock() failed.",pTable->sTableName);
        do
        {
            CHECK_RET_BREAK(AddPageToCircle(pCurPage,pTable->iFreePageID),"AddPageToCircle failed.");
             //��״̬����Ϊfree
            SAFESTRCPY(pCurPage->m_sState, sizeof(pCurPage->m_sState),"free");
            
            ++pTable->iFreePages;
        }while(0);
        pTable->tFreeMutex.UnLock(true, m_pDsn->sCurTime);
        
        TADD_FUNC("Finish.");
        return iRet;
    }
	long long TMdbTableSpaceCtrl::GetFileSize(char * sFile)
	{
	        struct stat f_stat;
	        if(stat(sFile, &f_stat) == -1) 
	        {
	            return ERR_OS_NO_FILE;
	        }
	        return (long long)f_stat.st_size;
	}
	int TMdbTableSpaceCtrl::AddToFile(TMdbTableSpace *pTableSpace,TMdbTSNode& node)
	{
	    int iRet = 0;
	    char sStorageFile[MAX_FILE_NAME] = {0};
	    snprintf(sStorageFile,sizeof(sStorageFile),"%s/%s%s_%d%s",m_pShmDSN->GetInfo()->sStorageDir,TS_FILE_PREFIX,pTableSpace->sName,pTableSpace->m_iBlockId<1?0:pTableSpace->m_iBlockId-1,TS_FILE_SUFFIX);
	    TADD_NORMAL("Add to [%s]",sStorageFile);
	    TMdbTSFileHead m_TSHead;
	    m_TSHead.Clear();
	    m_pTSFile = NULL;
	    SAFE_CLOSE(m_pTSFile);
	    if(TMdbNtcFileOper::IsExist(sStorageFile))
	    {
	        long long iFileSize=GetFileSize(sStorageFile);
	        if(iFileSize < 0)
	        {
	            TADD_ERROR(-1,"GetFileSize() failed.", sStorageFile);
	            return iFileSize;    
	        }
			long long iAddSize = (node.iPageEnd - node.iPageStart) * pTableSpace->iPageSize;
	        if(iFileSize + iAddSize >= 1024*1024*1024)//Ӧ���жϵ�ǰ���ӵ�ҳ������ļ���С�Ƿ񳬳�1G������Ӧ��������ҳˢ�����ļ�
	        {
                SAFE_CLOSE(m_pTSFile);
				TADD_NORMAL("[%s] is Full",sStorageFile);
	            memset(sStorageFile,0,sizeof(sStorageFile));
	            snprintf(sStorageFile,sizeof(sStorageFile),"%s/%s%s_%d%s",m_pShmDSN->GetInfo()->sStorageDir,TS_FILE_PREFIX,pTableSpace->sName,pTableSpace->m_iBlockId,TS_FILE_SUFFIX);
	            TADD_NORMAL("Add to New File[%s]",sStorageFile);
			    strncpy(m_TSHead.m_sTSName, pTableSpace->sName, sizeof(m_TSHead.m_sTSName));
			    m_TSHead.m_iPageOffset = sizeof(TMdbTSFileHead);
			    m_TSHead.m_iPageSize = pTableSpace->iPageSize;
		        m_TSHead.m_iStartPage = node.iPageStart;
		        m_pTSFile = fopen(sStorageFile,"wb+");
				CHECK_OBJ(m_pTSFile);
				pTableSpace->m_iBlockId++;
				
			    //ˢ���ļ�ͷ
			    CHECK_RET(fseek(m_pTSFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
			    if(fwrite(&m_TSHead, sizeof(TMdbTSFileHead), 1, m_pTSFile)!= 1 )
			    {
			        CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
			    }
	        }
	        else
	        {
	            m_pTSFile = fopen(sStorageFile,"rb+");
	            if(fread(&m_TSHead,sizeof(m_TSHead),1,m_pTSFile) != 1)
	            {
	                if(feof(m_pTSFile))
	                {//�ļ�����
	                    CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
	                }
	                CHECK_RET(ERR_OS_READ_FILE,"fread(%s) errno=%d, errormsg=[%s].",sStorageFile, errno, strerror(errno));
	            }
	        }
	    }
	    else
	    {
		    strncpy(m_TSHead.m_sTSName, pTableSpace->sName, sizeof(m_TSHead.m_sTSName));
		    m_TSHead.m_iPageOffset = sizeof(TMdbTSFileHead);
		    m_TSHead.m_iPageSize = pTableSpace->iPageSize;
	        m_TSHead.m_iStartPage = node.iPageStart;
	        m_pTSFile = fopen(sStorageFile,"wb+");
			pTableSpace->m_iBlockId++;
			
		    //ˢ���ļ�ͷ
		    CHECK_RET(fseek(m_pTSFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
		    if(fwrite(&m_TSHead, sizeof(TMdbTSFileHead), 1, m_pTSFile)!= 1 )
		    {
		        CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
		    }
	    }
	    TMdbPageCtrl mdbPageCtrl;
		mdbPageCtrl.SetDSN(m_pDsn->sName);
	    CHECK_OBJ(m_pTSFile);

	    //�ƶ����ļ�β���������ӵ�ҳ��д���ļ�
	    CHECK_RET(fseek(m_pTSFile,0,SEEK_END),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
	    int iPageID = node.iPageStart;
	    for(;iPageID <= node.iPageEnd; ++iPageID)
	    {
	        TMdbPage * pPage = (TMdbPage *)GetAddrByPageID(iPageID);
	        CHECK_OBJ(pPage);
	        mdbPageCtrl.Attach((char *)pPage,false,true);
	        mdbPageCtrl.WLock();
	        do{
	            if(fwrite(pPage, pTableSpace->iPageSize, 1, m_pTSFile)!= 1 )
	            {
	                CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite(%s) errno=%d, errormsg=[%s].",sStorageFile, errno, strerror(errno));
	            }
	        }while(0);
	        mdbPageCtrl.UnWLock();
	    }
		m_TSHead.m_iEndPage = node.iPageEnd;
		m_TSHead.m_iPageCount += (node.iPageEnd - node.iPageStart + 1);
        //TADD_NORMAL("pTableSpace->iTotalPages = [%d], m_TSHead.m_iStartPage = [%d], m_TSHead.m_iEndPage = [%d], m_TSHead.m_iPageCount = [%d], node.iPageStart = [%d], node.iPageEnd = [%d]", pTableSpace->iTotalPages, m_TSHead.m_iStartPage, m_TSHead.m_iEndPage, m_TSHead.m_iPageCount, node.iPageStart, node.iPageEnd);
	    //ˢ���ļ�ͷ
	    CHECK_RET(fseek(m_pTSFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
	    if(fwrite(&m_TSHead, sizeof(TMdbTSFileHead), 1, m_pTSFile)!= 1 )
	    {
	        CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
	    }
	    fflush(m_pTSFile);
	    return iRet;
	}
	
    /******************************************************************************
    * ��������	:  AskNewPages
    * ��������	:  ��ϵͳ����һ����ҳ��
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableSpaceCtrl::AskNewPages(int iNewPageCount, bool bAddToFile)
    {
        TADD_FUNC("Start.iNewPageCount=[%d]",iNewPageCount);
        int iRet = 0;
        CHECK_OBJ(m_pTMdbTableSpace);
        if(m_pTMdbTableSpace->sName[0] == 0)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"(m_pTMdbTableSpace->sName[0] == 0)");
        }
        if(iNewPageCount < m_pTMdbTableSpace->iRequestCounts)
        {//һ�������ҳ������������趨ֵ
            CHECK_RET(ERR_APP_INVALID_PARAM,"ask pages[%d] < min[%d].",iNewPageCount,m_pTMdbTableSpace->iRequestCounts);
        }
        if(m_pTMdbTableSpace->iTotalPages + iNewPageCount >= MAX_MDB_PAGE_COUNT)
        {//ÿ����ռ����ҳ����
            CHECK_RET(ERR_APP_INVALID_PARAM,"MAX_PAGE_COUNT[%d] in one TS,TS[%s] iTotalPages = [%d],cannot ask newpage[%d] ",
                MAX_MDB_PAGE_COUNT,m_pTMdbTableSpace->sName,m_pTMdbTableSpace->iTotalPages,iNewPageCount);
        }
        size_t iAskSize =iNewPageCount * m_pTMdbTableSpace->iPageSize;//�����ҳ���С=ҳ����*ҳ��ߴ�
        TADD_DETAIL("lAskSize=[%lu]",iAskSize);
        TMdbDSN* pDsn = m_pShmDSN->GetInfo();
        for(int i = 0;i < MAX_SHM_ID;++i)
        {
            
            TMdbShmHead* pHead = (TMdbShmHead*)m_pShmDSN->GetDataShm(i);
            if(NULL == pHead)
            {//��Ҫ�������ڴ��
                TMdbConfig * pConfig = TMdbConfigMgr::GetMdbConfig(pDsn->sName);
                CHECK_OBJ(pConfig);
                CHECK_RET(m_pShmDSN->CreateNewDataShm(*pConfig),"CreateNewDataShm[%s] failed.",pDsn->sName);
				pHead = (TMdbShmHead*)m_pShmDSN->GetDataShm(i);
				if(NULL == pHead)
                {
                    CHECK_RET(ERR_OS_NO_MEMROY,"GetNewShm failed,pos=%d ",i);
                }
                TADD_DETAIL("TS[%s]create new data shm:[%d]", m_pTMdbTableSpace->sName,i);
            }
            size_t iRealSize = 0;
            do{
                size_t iOffSet = pHead->GetMemory(iAskSize,iRealSize,m_pTMdbTableSpace->iPageSize,m_pTMdbTableSpace->iRequestCounts);
                //����ڴ�������ڴ治�㣬�������һ���ڴ��
                if(iOffSet == 0){break;}
                 //���ɽڵ���Ϣ
                TMdbTSNode nod;
                nod.iPageStart = m_pTMdbTableSpace->iTotalPages+1;
                nod.iPageEnd   = m_pTMdbTableSpace->iTotalPages + iRealSize/m_pTMdbTableSpace->iPageSize; //ע���ǰ���
                nod.iShmID     = pHead->iShmID;
                nod.iOffSet      = iOffSet;
                nod.iNext         = 0;
                
                TADD_DETAIL("TS[%s]iPageStart=%d, iPageEnd=%d.", m_pTMdbTableSpace->sName,nod.iPageStart, nod.iPageEnd);
                TADD_DETAIL("TS:[%s]SHM_ID=[%d].iOffSet=[%lu],iNext=[%lu]", m_pTMdbTableSpace->sName,nod.iShmID,nod.iOffSet,nod.iNext);
                //�ѽڵ�����ռ䣬��������
                AddNode(m_pTMdbTableSpace, nod);
                //��ҳ�洮����
                InitPage((char*)pHead+iOffSet, nod.iPageStart, nod.iPageEnd, m_pTMdbTableSpace->iPageSize);
				
				//������ļ��洢��������������ڴ�ӳ�䵽�ļ���
			    if(bAddToFile == true && m_pTMdbTableSpace->m_bFileStorage == true)
			    {
			        CHECK_RET(AddToFile(m_pTMdbTableSpace,nod),"InitPage Faild");
			    }
            }while(0);
            TADD_FLOW("datashm[%d]:iRealSize=%lu, lAskSize=%lu.", i,iRealSize, iAskSize);
            if(iAskSize > iRealSize)
            {
                iAskSize -= iRealSize;
            }
            else
            {//ȫ������
                iAskSize = 0;
                break;
            }
        }
        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  RemovePage
    * ��������	:  �Ƴ�һ��page
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableSpaceCtrl::RemovePage(TMdbPage * pPageToRemove,int & iHeadPageId)
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
            TMdbPage * pPrePage = (TMdbPage * )GetAddrByPageID(pPageToRemove->m_iPrePageID);
            CHECK_OBJ(pPrePage);
            pPrePage->m_iNextPageID = pPageToRemove->m_iNextPageID;
        }
        
        if(pPageToRemove->m_iNextPageID > 0)
        {//��β�ڵ�
            TMdbPage * pNextPage = (TMdbPage *)GetAddrByPageID(pPageToRemove->m_iNextPageID);
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
    * ��������	:  ��page��ӵ�ͷ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableSpaceCtrl::AddPageToTop(TMdbPage * pPageToAdd,int & iHeadPageId)
    {
        TADD_FUNC("Start.iHeadPageId=[%d],page:pre[%d] next[%d]",iHeadPageId,pPageToAdd->m_iPrePageID,pPageToAdd->m_iNextPageID);
        int iRet = 0;
        if(iHeadPageId > 0)
        {
            TMdbPage * pNextPage    = (TMdbPage *)GetAddrByPageID(iHeadPageId);
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
    /******************************************************************************
    * ��������	:  GetPageFromTop
    * ��������	:  ��listͷ��ȡһ��page
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableSpaceCtrl::GetPageFromTop(TMdbPage * &pPageToGet,int  iHeadPageId)
    {
        int iRet = 0;
        pPageToGet = NULL;
        if(iHeadPageId > 0)
        {
            pPageToGet = (TMdbPage *)GetAddrByPageID(iHeadPageId);
        }
        return iRet;
    }
    /******************************************************************************
    * ��������	:  GetNextNode
    * ��������	:  ��ȡ��һ��node
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbTSNode * TMdbTableSpaceCtrl::GetNextNode(TMdbTSNode * pCurNode)
    {
        if(NULL == pCurNode){return NULL;}
        if(0 == pCurNode->iNext){return NULL;}
        return (TMdbTSNode*)m_pShmDSN->GetAddrByOffset(pCurNode->iNext);
    }

    /******************************************************************************
    * ��������	:  ReBuildFromDisk
    * ��������	:  �Ӵ������¹�����ռ� ��ҳ����
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  dong.chun
    *******************************************************************************/
    int TMdbTableSpaceCtrl::ReBuildFromDisk(TMdbTSFile * pTSFile)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        CHECK_OBJ(pTSFile);
        CHECK_OBJ(m_pTMdbTableSpace);
        TMdbConfig* m_pConfig = TMdbConfigMgr::GetMdbConfig(m_pShmDSN->GetInfo()->sName);
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {
			TMdbTable * pTable = &(*itor);
	        if(pTable->bIsSysTab){continue;}//����ϵͳ��
	        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(pTable->m_sTableSpace, m_sTableSpaceName))
            {
                if(m_pConfig->IsTableAlter(pTable->sTableName))
                {
                    StructAddChange(m_iHasTableChange,STRUCT_TABLE_IN_TABLESPACE_CHANGE);
                }
            }
        }
		
        TMdbStorageCopy StorageCopyCtrl;
        TMdbPageCtrl   PageCtrl;
        CHECK_RET(PageCtrl.SetDSN(m_pShmDSN->GetInfo()->sName),"PageCtrl SetDSN[%s] Faild.",m_pShmDSN->GetInfo()->sName);
        TMdbPage * pFilePage = NULL;
        CHECK_RET(pTSFile->StartToReadPage(),"StartToReadPage failed.");
        while((iRet = pTSFile->GetNextPage(pFilePage)) > 0)
        {
            TADD_DETAIL("TS=[%s],sTableName=[%s],pageid=[%d]",pTSFile->m_tTSFileHead.m_sTSName,pFilePage->m_sTableName,pFilePage->m_iPageID);
            TMdbTable * pTable = m_pShmDSN->GetTableByName(pFilePage->m_sTableName);
            if(pTable == NULL){continue;}
            CHECK_RET(StorageCopyCtrl.Init(m_pShmDSN,pTable, m_pTMdbTableSpace,&PageCtrl,this),"StorageCopyCtrl Init Faild.");
            CHECK_RET(StorageCopyCtrl.Load(pFilePage),"StorageCopyCtrl Load Faild,PageID[%d],table_name[%s]",pFilePage->m_iPageID,pFilePage->m_sTableName);
        }
        TADD_FUNC("Finish");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  SetPageDirtyFlag
    * ��������	:  ������ҳ��ʶ
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableSpaceCtrl::SetPageDirtyFlag(int iPageID)
    {
        int iRet = 0;
        if(iPageID < 0){CHECK_RET(ERR_APP_INVALID_PARAM,"PageID can not be [%d]",iPageID);}
        TMdbTSNode * pTSNode = GetNodeByPageID(iPageID);
        CHECK_OBJ(pTSNode);
        int iBytePos = ((iPageID - pTSNode->iPageStart)/8)%MAX_BIT_MAP_SIZE; //��ֹ���
        int iBitPos = (iPageID - pTSNode->iPageStart)%8;
        //������ҳ
        unsigned char cTemp = (0x1<<iBitPos);
        pTSNode->bmDirtyPage[iBytePos] |= cTemp;
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GetNodeByPageID
    * ��������	:  ����pageid��ȡnode
    * ����		:  
    * ���		:  
    * ����ֵ	:  !NULL - �ɹ�NULL-ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbTSNode * TMdbTableSpaceCtrl::GetNodeByPageID(int iPageID)
    {
        TADD_FUNC("Start.");
        if(iPageID <= 0){return NULL;}
        TMdbTSNode * pTSNode = &(m_pTMdbTableSpace->tNode);
        while(pTSNode != NULL)
        {
            if(iPageID >= pTSNode->iPageStart && iPageID <= pTSNode->iPageEnd)
            {
                return pTSNode;
            }
            pTSNode = GetNextNode(pTSNode);
        }
        TADD_FUNC("Finish.");
        return NULL;
        
    }
    /******************************************************************************
    * ��������	:  InsertDataFill
    * ��������	:  ���ڲ����ֵ���ڴ�ֲ��������
    * ����		:  
    * ���		:  sTmp - �������ڴ�ռ�
    * ����ֵ	:  0 - �ɹ�!0 - ���ɹ�
    * ����		:  dong.chun
    *******************************************************************************/
    int TMdbTableSpaceCtrl::InsertDataFill(char * const & sTmp)
    {
        return 0;
    }


    int TMdbTableSpaceCtrl::GetHasTableChange()
    {
    	return m_iHasTableChange;
    }
    int TMdbTableSpaceCtrl::SetHasTableChange()
    {
        int iRet = 0;
        TMdbConfig* m_pConfig = TMdbConfigMgr::GetMdbConfig(m_pShmDSN->GetInfo()->sName);
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {
            TMdbTable * pTable = &(*itor);
            if(pTable->bIsSysTab){continue;}//����ϵͳ��
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(pTable->m_sTableSpace, m_sTableSpaceName))
            {
                if(m_pConfig->IsTableAlter(pTable->sTableName))
                {
                    StructAddChange(m_iHasTableChange,STRUCT_TABLE_IN_TABLESPACE_CHANGE);
                }
            }
        }
        return iRet;
    }
    
    int TMdbTableSpaceCtrl::AddPageToCircle(TMdbPage * pPageToAdd,int & iHeadPageId)
    {
        TADD_FUNC("iHeadPageId=[%d],page:pre[%d] next[%d]",iHeadPageId,pPageToAdd->m_iPrePageID,pPageToAdd->m_iNextPageID);
        int iRet = 0;
        if(iHeadPageId > 0)
        {
            TMdbPage * pHeadPage    = (TMdbPage *)GetAddrByPageID(iHeadPageId);
            CHECK_OBJ(pHeadPage);
            TADD_DETAIL("TS[%s]head[%d], head-pre[%d], head-next[%d]" , m_pTMdbTableSpace->sName,pHeadPage->m_iPageID, pHeadPage->m_iPrePageID,pHeadPage->m_iNextPageID);

            TMdbPage* pTailPage = (TMdbPage *)GetAddrByPageID(pHeadPage->m_iPrePageID);
            CHECK_OBJ(pTailPage);
            TADD_DETAIL("TS[%s]tail[%d], tail-pre[%d], tail-next[%d]" , m_pTMdbTableSpace->sName,pTailPage->m_iPageID, pTailPage->m_iPrePageID,pTailPage->m_iNextPageID);

            pPageToAdd->m_iNextPageID = pHeadPage->m_iPageID;
            pPageToAdd->m_iPrePageID = pHeadPage->m_iPrePageID;

            pTailPage->m_iNextPageID = pPageToAdd->m_iPageID;
            pHeadPage->m_iPrePageID = pPageToAdd->m_iPageID;
            
            //iHeadPageId = pPageToAdd->m_iPageID;
            TADD_DETAIL("1:TS[%s]pPageToAdd[%d], pre[%d],next[%d]",m_pTMdbTableSpace->sName, pPageToAdd->m_iPageID, pPageToAdd->m_iPrePageID,pPageToAdd->m_iNextPageID);
        }
        else
        {
            iHeadPageId = pPageToAdd->m_iPageID;
            pPageToAdd->m_iNextPageID = iHeadPageId;
            pPageToAdd->m_iPrePageID   = iHeadPageId;
            TADD_DETAIL("2:TS[%s]pPageToAdd[%d], pre[%d],next[%d]",m_pTMdbTableSpace->sName, pPageToAdd->m_iPageID, pPageToAdd->m_iPrePageID,pPageToAdd->m_iNextPageID);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbTableSpaceCtrl::RemovePageFromCircle(TMdbPage * pPageToRemove,int & iHeadPageId)
    {
        TADD_FUNC("TEST.iHeadPageId=[%d],page:pre[%d] next[%d]",iHeadPageId,pPageToRemove->m_iPrePageID,pPageToRemove->m_iNextPageID);
        int iRet  = 0;
        CHECK_OBJ(pPageToRemove);


        if(pPageToRemove->m_iNextPageID == pPageToRemove->m_iPageID) // ֻʣһ���ڵ�
        {
            iHeadPageId = -1;
            TADD_DETAIL("1:TS[%s]pPageToRemove[%d], pre[%d],next[%d],iHeadPageId=[%d]", m_pTMdbTableSpace->sName,pPageToRemove->m_iPageID, pPageToRemove->m_iPrePageID,pPageToRemove->m_iNextPageID,iHeadPageId);
        }
        else
        {
            TMdbPage * pPrePage = (TMdbPage * )GetAddrByPageID(pPageToRemove->m_iPrePageID);
            CHECK_OBJ(pPrePage);
            TADD_DETAIL("TS[%s]pPrePage[%d], pPrePage->pre[%d],pPrePage->next[%d]", m_pTMdbTableSpace->sName,pPrePage->m_iPageID, pPrePage->m_iPrePageID,pPrePage->m_iNextPageID);

            TMdbPage * pNextPage = (TMdbPage * )GetAddrByPageID(pPageToRemove->m_iNextPageID);
            CHECK_OBJ(pNextPage);
            TADD_DETAIL("TS[%s]pNextPage[%d], pNextPage->pre[%d],pNextPage->next[%d]", m_pTMdbTableSpace->sName,pNextPage->m_iPageID, pNextPage->m_iPrePageID,pNextPage->m_iNextPageID);

            pPrePage->m_iNextPageID = pNextPage->m_iPageID;
            pNextPage->m_iPrePageID = pPrePage->m_iPageID;
            if(iHeadPageId == pPageToRemove->m_iPageID)
            {
                iHeadPageId = pNextPage->m_iPageID;
                TADD_DETAIL("TS[%s],iHeadPageId=[%d]",m_pTMdbTableSpace->sName,iHeadPageId);
            }
        }
        
        //�����page
        pPageToRemove->m_iPrePageID  = -1;
        pPageToRemove->m_iNextPageID = -1;
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbTableSpaceCtrl::GetTableFreePageId(TMdbTable* pTable)
    {
        int iCurFreePageId = pTable->iFreePageID;
    
        if(iCurFreePageId > 0)
        {
            TMdbPage * pCurFreePage = (TMdbPage * )GetAddrByPageID(iCurFreePageId);
            if(NULL == pCurFreePage)
            {
                return -1;
            }
            pTable->iFreePageID = pCurFreePage->m_iNextPageID;   
        }
        TADD_DETAIL("TS[%s],TAB[%s], freepageid[%d]",m_pTMdbTableSpace->sName, pTable->sTableName,iCurFreePageId);
        return iCurFreePageId;
    }
//}


