/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbVarcharMgr.cpp
*@Description： 负责管理QuickMDB的变长存储区控制接口
*@Author:		li.shugang
*@Date：	    2012年02月20日
*@History:
******************************************************************************************/

#include "Control/mdbVarcharMgr.h"
#include "Control/mdbMgrShm.h"
#include "Control/mdbStorageEngine.h"

//namespace QuickMDB{
	TMdbVarCharCtrl::TMdbVarCharCtrl()
	{
	    m_pVarChar = NULL;
	    m_pShmDSN  = NULL;
	    m_pDsn     = NULL;
	    m_pVarcharFile = NULL;
	}

	TMdbVarCharCtrl::~TMdbVarCharCtrl()
	{
	    SAFE_CLOSE(m_pVarcharFile);
	}

	int TMdbVarCharCtrl::Init(const char* sDsn)
	{
	    int iRet = 0;
	    m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
	    CHECK_OBJ(m_pShmDSN);
	    m_pDsn = m_pShmDSN->GetInfo();
	    CHECK_OBJ(m_pDsn);
	    m_mdbPageCtrl.SetDSN(m_pDsn->sName, MUTEX_TYPE_VARCHAR_PAGE);
	    SetStorageFlag();
	    return iRet;
	}
	/******************************************************************************
	* 函数名称	:  GetFree
	* 函数描述	:  获取空闲的varchar共享内存段
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  成功返回id，否则返回0
	* 作者		:  dong.chun
	*******************************************************************************/
	int TMdbVarCharCtrl::GetFree()
	{
	    CHECK_OBJ(m_pDsn);
	    int i = 0;
	    for(; i<MAX_VARCHAR_SHM_ID; i++)
	    {
	        if(m_pDsn->iVarCharShmID[i] == INITVAl)
	        {
	            return i;
	        }
	        else
	        {
	            continue;
	        }
	    }
	    return INITVAl;
	}

	/******************************************************************************
	* 函数名称	:  CreateVarChar
	* 函数描述	:  创建VARCHAR的各个段
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  成功返回0，否则返回-1
	* 作者		:  dong.chun
	*******************************************************************************/
	int TMdbVarCharCtrl::CreateVarChar()
	{
	    TADD_FUNC(" Start.");
	    //这里不实际分配空间
	    int iRet = 0;
	    CHECK_OBJ(m_pShmDSN);
	    CHECK_OBJ(m_pDsn);
	    for(int i = VC_16; i<=VC_8192;i++)
	    {
	        m_pVarChar = NULL;
	        CHECK_RET(m_pShmDSN->AddNewVarChar(m_pVarChar),"AddNewVarchar");
	        CHECK_OBJ(m_pVarChar);
	        m_pVarChar->iVarcharID = i;
	        m_pVarChar->tMutex.Create();
	        m_pVarChar->iPageSize = 32*1024;
	    }
	    long long m_iMgrKey = 0x5765454 + 1000000*(m_pDsn->llDsnValue);
	    for(int i=0; i<MAX_VARCHAR_SHM_ID; ++i)
	    {
	        //数据块
	        m_pDsn->iVarCharShmKey[i] = m_iMgrKey + 10000*i + 1978;
	        m_pDsn->iVarCharShmID[i]  = INITVAl;
	    }
	    TADD_FUNC("Finish.");
	    return iRet;
	}

	/******************************************************************************
	* 函数名称	:  CreateShm
	* 函数描述	:  创建某个块的共享内存，并将之分页
	* 输入		:  
	* 输入		:  
	* 输出		:  
	* 返回值	: 
	* 作者		:  dong.chun
	*******************************************************************************/
	int TMdbVarCharCtrl::CreateShm(TMdbVarchar *pVarChar,bool bAddToFile)
	{
	    TADD_FUNC(" Start.");
	    CHECK_OBJ(m_pDsn);
	    CHECK_OBJ(pVarChar);
	    int iRet = 0;
	    //创建共享内存，如果已经存在，或者达到共享内存个数上限则报错
	    int iPos = GetFree();
	    if(iPos < 0)
	    {
	        CHECK_RET(iPos,"Can't Create VarcharSHM,MAX=[%d].",MAX_VARCHAR_SHM_ID);
	    }

	    iRet = TMdbShm::Create(m_pDsn->iVarCharShmKey[iPos], MAX_VARCHAR_SIZE, m_pDsn->iVarCharShmID[iPos]);
	    if(iRet != 0)
	    {
	        CHECK_RET(iRet,"Can't create VarChar share memory, errno=%d[%s].",errno, strerror(errno));
	    }
	    //链接到共享内存上面
	    TADD_NORMAL("TMdbVarcharMgr::CreateShm() : iPos=%d, iVarCharShmID=%d.",iPos,m_pDsn->iVarCharShmID[iPos]);
	    
	    char* pAddr = NULL;
	    CHECK_RET(m_pShmDSN->InitVarchar(iPos,m_pDsn->iVarCharShmID[iPos],pAddr),"init Varchar Faild");
	    CHECK_OBJ(pAddr);
	    ++(m_pDsn->iVarCharShmCounts);
	    

	    //串联该共享内存
	    TMdbTSNode nod;
	    nod.iPageStart = pVarChar->iTotalPages+1;
	    nod.iPageEnd   = pVarChar->iTotalPages + (MAX_VARCHAR_SIZE-10)/pVarChar->iPageSize; //注意是包含
	    nod.iShmID     = m_pDsn->iVarCharShmID[iPos];
	    nod.iOffSet    = 0;
	    nod.iNext      = 0;
	    CHECK_RET(AddNode(pVarChar, nod),"AddVarcharNode Faild");
           CHECK_RET(InitPage(pAddr, nod.iPageStart, nod.iPageEnd, pVarChar),"InitPage Faild");
	    //如果有文件存储需求，则将新申请的内存映射到文件中
	    if(bAddToFile == true && NeedStorage() == true)
	    {
	        CHECK_RET(AddToFile(pVarChar,nod),"InitPage Faild");
	    }
	    TADD_FUNC("TMdbVarcharMgr::CreateShm(%d) : Finish(pAddr=%p).", m_pDsn->iVarCharShmCounts, *pAddr);
	    return iRet;
	}

	    /******************************************************************************
	    * 函数名称	:  VarCharWhichStore
	    * 函数描述	:  判定应该放到哪个存储区
	    * 输入		:  
	    * 输入		:  
	    * 输出		:  
	    * 返回值	: 
	    * 作者		:  dong.chun
	    *******************************************************************************/
	    int TMdbVarCharCtrl::VarCharWhichStore(int iLen,int& iPos)
	    {
	        TADD_FUNC("TMdbVarcharMgr::VarCharWhichStore()  : Start.");
	        int iRet = 0;
	        static int iWhich[10] = {16,32,64,128,256,512,1024,2048,4096,8192};
	        if(iLen < 0 || iLen > 8192)
	        {
	            TADD_ERROR(-1," iLen=%d.", iLen);
	            return -1;
	        }
	        int i = 0;
	        for(i=0; i<10; i++)
	        {
	            if(iLen < iWhich[i])
	            {
	                break;
	            }
	        }
	        if(i == 10)
	        {
	            TADD_ERROR(-1," iLen=%d.", iLen);
	            return -1;
	        }
	        iPos = i;
	        TADD_DETAIL("iPos=%d, iLen=%d", i, iLen);
	        CHECK_OBJ(m_pShmDSN);
	        m_pVarChar = NULL;
	        m_pVarChar = m_pShmDSN->GetVarchar(iPos);
	        CHECK_OBJ(m_pVarChar);
	        TADD_FUNC("Finish(iPos=%d).", i);
	        return iRet;
	    }
	    /******************************************************************************
	    * 函数名称	:  AddNode
	    * 函数描述	:  增加节点
	    * 输入		:  
	    * 输入		:  
	    * 输出		:  
	    * 返回值	: 
	    * 作者		:  dong.chun
	    *******************************************************************************/
	    int TMdbVarCharCtrl::AddNode(TMdbVarchar * pVarChar, TMdbTSNode & nod)
	    {
	        TADD_FUNC("Start.nod:pageStart[%d],pageEnd[%d]",nod.iPageStart,nod.iPageEnd);
	        int iRet = 0;
	        CHECK_OBJ(m_pShmDSN);
	        CHECK_OBJ(m_pDsn);
	        //判断管理区空间是否足够
	        TMdbConfig * pConfig = TMdbConfigMgr::GetMdbConfig(m_pDsn->sName);
	        CHECK_OBJ(pConfig);
	        if(m_pShmDSN->GetUsedSize() >= pConfig->GetDSN()->iManagerSize - sizeof(TMdbTSNode))
	        {//超出内存范围
	            CHECK_RET(ERR_OS_NO_MEMROY,"m_pShmDSN->GetFreeSize[%luMB] > iManagerSize[%luMB]",
	                m_pShmDSN->GetUsedSize()/(1024*1024),pConfig->GetDSN()->iManagerSize/(1024*1024));
	        }
	        
	        CHECK_RET(m_pDsn->tMutex.Lock(true, &m_pDsn->tCurTime),"m_pDsn->tMutex.Lock failed");    //加锁
	        TMdbTSNode* pNode = (TMdbTSNode*)&pVarChar->tNode;
	        while(1){
	            TMdbTSNode * pNext =  GetNextNode(pNode);
	            if(NULL == pNext){break;}
	            pNode = pNext;
	        }//找到最后一个节点

	        if(pNode->iPageEnd > 0)
	        {
	            pNode =  (TMdbTSNode*)m_pShmDSN->Alloc(sizeof(TMdbTSNode),pNode->iNext);
	            //把数据放进去
	            memcpy(pNode, &nod, sizeof(TMdbTSNode));
	        }
	        else
	        {
	            memcpy(pNode, &nod, sizeof(TMdbTSNode));
	        }
	        //解锁
	        m_pDsn->tMutex.UnLock(true, m_pDsn->sCurTime);
	        
	        TADD_FUNC("Finish.");
	        return iRet;
	    }
	    
	    /******************************************************************************
	    * 函数名称	:  GetNextNode
	    * 函数描述	:  获取下一个节点
	    * 输入		:  
	    * 输入		:  
	    * 输出		:  
	    * 返回值	: 
	    * 作者		:  dong.chun
	    *******************************************************************************/
	    TMdbTSNode* TMdbVarCharCtrl::GetNextNode(TMdbTSNode * pCurNode)
	    {
	        if(m_pShmDSN == NULL){return NULL;}
	        if(NULL == pCurNode){return NULL;}
	        if(0 == pCurNode->iNext){return NULL;}
	        return (TMdbTSNode*)m_pShmDSN->GetAddrByOffset(pCurNode->iNext);
	    }
	    
	    /******************************************************************************
	    * 函数名称	:  InitPage
	    * 函数描述	:  初始化每个页面
	    * 输入		:  
	    * 输入		:  
	    * 输出		:  
	    * 返回值	: 
	    * 作者		:  dong.chun
	    *******************************************************************************/
	    int TMdbVarCharCtrl::InitPage(char * pAddr, int iPageSID, int iPageFID,TMdbVarchar* pVarChar)
	    {
	        int iRet = 0;
	        int iPrePageID = -1;
	        //调整Empty页面的链接, 如果没有Empty页面，直接把StartPageID给他，否则把最后一个Empty页面的iNextPageID设置为iPageSID
	        if(pVarChar->iFreePageId <= 0)
	        {
	            pVarChar->iFreePageId = iPageSID;
	        }
	        else
	        {
	            int iLastFreePageID = pVarChar->iFreePageId;
	            TMdbPage *pPage = NULL;
                TADD_DETAIL("iLastFreePageID=%d",iLastFreePageID);
	            while(true)
	            {
	                pPage = (TMdbPage*)GetAddrByPageID(pVarChar,iLastFreePageID);
	                CHECK_OBJ(pPage);
                    TADD_DETAIL("iLastFreePageID=%d, m_iNextPageID=%d",iLastFreePageID,pPage->m_iNextPageID);
	                if(pPage->m_iNextPageID <= 0)
	                {
	                    pPage->m_iNextPageID = iPageSID;
	                    iPrePageID = iLastFreePageID;
	                    TADD_DETAIL("iLastEmptyPageID=[%d],iPrePageID=[%d]",iLastFreePageID,iPrePageID);
	                    break;
	                }
	                else
	                {
	                    iLastFreePageID = pPage->m_iNextPageID;
	                }
	            }
	        }
	        TADD_DETAIL("(iSize=%lu) : iFreePageId=%d.", pVarChar->iPageSize, pVarChar->iFreePageId);
	        //初始化每一个页
	        TMdbPage *pPage = (TMdbPage*)pAddr;
	        for(int i=iPageSID; i<=iPageFID; ++i)
	        {
	            pPage->Init(i,pVarChar->iPageSize,pVarChar->iVarcharID);//初始化
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
	            TADD_DETAIL("(iSize=%lu) : PageAddr=%p, iPageID=%d, iNextPageID=%d.", pVarChar->iPageSize, pPage, pPage->m_iPageID, pPage->m_iNextPageID);
	            pPage = (TMdbPage*)(pAddr + pVarChar->iPageSize * (i+1-iPageSID));
	            ++pVarChar->iTotalPages;
	    }
	        TADD_DETAIL("(iSize=%lu) : iFreePageId=%d.", pVarChar->iPageSize, pVarChar->iFreePageId);
	        TADD_FUNC("Finish.");
	        return iRet;
	}
	    
	/******************************************************************************
	* 函数名称	:  GetAddrByPageID
	* 函数描述	:  根据pageid获取地址
	* 输入		:  
	* 输入		:  
	* 输出		:  
	* 返回值	: 
	* 作者		:  dong.chun
	*******************************************************************************/
	char* TMdbVarCharCtrl::GetAddrByPageID(TMdbVarchar* pVarChar,int iPageID)
	{
	    if(pVarChar == NULL)
	    {
	        TADD_ERROR(ERR_APP_INVALID_PARAM," pVarChar == NULL.");
	        return NULL;
	    }
	    char* pAddr = NULL;
	    if(iPageID > 0)
	    {
	        //取到自由页的地址
	        TMdbTSNode* pNodeTmp = &pVarChar->tNode;
	        while(true)
	        {
	            //如果页面在当前的范围内, 则转换成内部地址
	            if(iPageID >= pNodeTmp->iPageStart && iPageID <= pNodeTmp->iPageEnd)
	            {
	                //取到共享内存段的地址
	                pAddr = m_pShmDSN->GetVarcharShmAddr(pNodeTmp->iShmID);
	                if(pAddr == NULL)
	                {
	                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Can't GetShmAddr(iShmID=%d)", pNodeTmp->iShmID);
	                    return NULL;
	                }
	                //计算出相应页面的地址
	                pAddr += (iPageID-pNodeTmp->iPageStart)*pVarChar->iPageSize;
	                break;
	            }
	            else
	            {
	                //如果后续没有节点, 说明PageID没有注册, 直接报错
	                if(0 == pNodeTmp->iNext )
	                {
	                    TADD_ERROR(ERROR_UNKNOWN,"Can't find PageID=%d", iPageID);
	                    return NULL;
	                }
	                else
	                {
	                    pNodeTmp = GetNextNode(pNodeTmp);
	                }
	            }
	        }
	    }
	    else
	    {
	        //如果没有相应页面, 返回NULL
	        //TADD_ERROR("no page-id=%d.",iPageID);
	        return NULL;
	    }
	    return pAddr;
	}

char* TMdbVarCharCtrl::GetAddrByPageID(TMdbVarchar* pVarChar,int iPageID,bool& isNoPage)
{
    isNoPage = false;
    if(pVarChar == NULL)
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM," pVarChar == NULL.");
        return NULL;
    }
    char* pAddr = NULL;
    if(iPageID > 0)
    {
        //取到自由页的地址
        TMdbTSNode* pNodeTmp = &pVarChar->tNode;
        while(true)
        {
            //如果页面在当前的范围内, 则转换成内部地址
            if(iPageID >= pNodeTmp->iPageStart && iPageID <= pNodeTmp->iPageEnd)
            {
                //取到共享内存段的地址
                pAddr = m_pShmDSN->GetVarcharShmAddr(pNodeTmp->iShmID);
                if(pAddr == NULL)
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Can't GetShmAddr(iShmID=%d)", pNodeTmp->iShmID);
                    return NULL;
                }
                //计算出相应页面的地址
                pAddr += (iPageID-pNodeTmp->iPageStart)*pVarChar->iPageSize;
                break;
            }
            else
            {
                //如果后续没有节点, 说明PageID没有注册, 直接报错
                if(0 == pNodeTmp->iNext )
                {
                    isNoPage = true;
                    return NULL;
                }
                else
                {
                    pNodeTmp = GetNextNode(pNodeTmp);
                }
            }
        }
    }
    else
    {
        //如果没有相应页面, 返回NULL
        TADD_ERROR(ERR_APP_INVALID_PARAM,"no page-id=%d.",iPageID);
        return NULL;
    }
    return pAddr;
}
/******************************************************************************
* 函数名称	:  Insert
* 函数描述	:  插入一条varchar数据
* 输入		:  varchar数据
* 输入		:  
* 输出		:  数据所在位置
* 返回值	: 
* 作者		:  dong.chun
*******************************************************************************/
int TMdbVarCharCtrl::Insert(char * pValue, int& iWhichPos,unsigned int& iRowId,char cStorage)
{
    TADD_FUNC(" Start.");
    int iRet = 0;
    iWhichPos = -1;
    CHECK_RET(VarCharWhichStore(strlen(pValue),iWhichPos),"Get VarCharWhichStore[%d] Faild",iWhichPos);
    TMdbRowID rowID;
    while(true)
    {
        TMdbPage* pMdbPage = NULL;
        char* pMemDataAddr = NULL;
        CHECK_RET(m_pVarChar->tMutex.Lock(true, &m_pDsn->tCurTime),"tMutex.Lock() failed.");
        CHECK_RET_BREAK(GetFreePage(pMdbPage),"GetFreePage Faild");
        if(pMdbPage == NULL)
        {
            TADD_ERROR(-1,"pMdbPage is NULL");
            iRet = -1;
            break;
        }
        CHECK_RET_BREAK(m_mdbPageCtrl.Attach((char *)pMdbPage, false, true),"Can't Attach to page");
        int iSize = GetValueSize(iWhichPos);
        iRet = m_mdbPageCtrl.InsertData((unsigned char*)pValue, iSize, rowID,pMemDataAddr,false);
        if(ERR_PAGE_NO_MEMORY == iRet)
        {
            //对于找到的自由页满了而无法插入数据，则找下一个自由页
            TADD_DETAIL("Current page is Full.");
            CHECK_RET_BREAK(PageFreeToFull(pMdbPage),"PageFreeToFull Faild");
            CHECK_RET(m_pVarChar->tMutex.UnLock(true),"tMutex.Lock() failed.");
            continue;
        }
        else if(iRet < 0)
        {
            CHECK_RET(m_pVarChar->tMutex.UnLock(true),"tMutex.Lock() failed.");
            CHECK_RET(iRet,"Insert varchar Data failed.");
        }
	TMdbPageNode * pPageNode = (TMdbPageNode *)(pMemDataAddr - sizeof(TMdbPageNode));
	pPageNode->cStorage = cStorage;
	if(NeedStorage())
	{
	       TADD_DETAIL("whichpos=[%d],page_id=[%d]",iWhichPos,pMdbPage->m_iPageID);
		SetPageDirtyFlag(pMdbPage->m_iPageID);
	}
	else
	{
		pPageNode->cStorage = 'N';
	}
        iRowId = rowID.m_iRowID;
        CHECK_RET(m_pVarChar->tMutex.UnLock(true),"tMutex.Lock() failed.");
        break;
    }
    if(iRet != 0)
    {
        CHECK_RET(m_pVarChar->tMutex.UnLock(true),"tMutex.Lock() failed.");
    }
    TADD_FUNC("Finish.");
    return iRet;
}

	/******************************************************************************
	* 函数名称	:  Update
	* 函数描述	:  修改一条varchar数据
	* 输入		:  varchar数据
	* 输入		:  
	* 输出		:  数据所在位置
	* 返回值	: 
	* 作者		:  dong.chun
	*******************************************************************************/
	int TMdbVarCharCtrl::Update(char * pValue,int& iWhichPos, unsigned int& iRowId,char cStorage)
	{
	    TADD_FUNC(" Start.");
	    int iRet = 0;
	    unsigned int iOldRowId = iRowId;
	    int iLen = strlen(pValue);
	    int iNewWhichPos = -1;
	    int iOldWhichPos = iWhichPos;
	    CHECK_RET(VarCharWhichStore(iLen,iNewWhichPos),"Get VarCharWhichStore[%d] Faild",iNewWhichPos);
	    if(iWhichPos == iNewWhichPos)
	    {
	        TMdbRowID tRowId;
	        tRowId.SetRowId(iRowId);
	        char* pAddr = GetAddressRowId(&tRowId);
			TMdbPage* pMdbPage = NULL;
			pMdbPage = (TMdbPage *)GetAddrByPageID(m_pVarChar,tRowId.GetPageID());
			CHECK_OBJ(pMdbPage);
	        CHECK_RET(m_mdbPageCtrl.Attach((char *)pMdbPage, false, true),"Can't Attach to page");
			m_mdbPageCtrl.WLock();
	        memcpy(pAddr,pValue,iLen);
	        pAddr[iLen] = 0;
			m_mdbPageCtrl.UnWLock();
			if(NeedStorage())
			{
	        	SetPageDirtyFlag(tRowId.GetPageID());
			}
	    }
	    else
	    {
	        //插入新数据
	        CHECK_RET(Insert(pValue,iWhichPos,iRowId,cStorage),"Insert[%d] Faild",iWhichPos);
	        CHECK_RET(Delete(iOldWhichPos,iOldRowId),"Delete[%d] Faild",iOldWhichPos);
	    }    
	    TADD_FUNC("Finish.");
	    return iRet;
	}

	/******************************************************************************
	* 函数名称	:  Delete
	* 函数描述	:  删除一条varchar数据
	* 输入		:  varchar数据
	* 输入		:  
	* 输出		:  数据所在位置
	* 返回值	: 
	* 作者		:  dong.chun
	*******************************************************************************/
	int TMdbVarCharCtrl::Delete(int& iWhichPos, unsigned int& iRowId)
	{
	    TADD_FUNC(" Start.");
	    int iRet = 0;
	    m_pVarChar = m_pShmDSN->GetVarchar(iWhichPos);
	    CHECK_OBJ(m_pVarChar);
	    TMdbRowID tRowId;
	    tRowId.SetRowId(iRowId);
	    int iPageId = tRowId.GetPageID();
	    TMdbPage* pPageAddr = (TMdbPage*)GetAddrByPageID(m_pVarChar,iPageId);
	    CHECK_OBJ(pPageAddr);
	    CHECK_RET(m_mdbPageCtrl.Attach((char *)pPageAddr, false, true),"Can't Attach to page");
	    int iPagePos = tRowId.GetDataOffset();
	    CHECK_RET(m_mdbPageCtrl.DeleteData(iPagePos,false),"DeleteData Faild");
	    CHECK_RET(m_pVarChar->tMutex.Lock(true, &m_pDsn->tCurTime),"tMutex.Lock() failed.");
	    if(pPageAddr->bNeedToMoveToFreeList() && TMdbNtcStrFunc::StrNoCaseCmp(pPageAddr->m_sState, "full") == 0)
	    {
	        CHECK_RET(PageFullToFree(pPageAddr),"PageFullToFree error");
	    }
	    CHECK_RET(m_pVarChar->tMutex.UnLock(true),"tMutex.Lock() failed.");
		if(NeedStorage())
		{
	    	SetPageDirtyFlag(iPageId);
		}
	    TADD_FUNC("Finish.");
	    return iRet;
	}

	int TMdbVarCharCtrl::SetStorgePos(int iWhichPos, unsigned int iRowId,char* pAddr)
	{
	    int i = 0;
	    char* pChar = &pAddr[i];
	    *pChar = '0' + iWhichPos;
	    //使用哪一块
	    i += 1;
	    unsigned int* pInt = (unsigned int*)&pAddr[i];
	    *pInt = iRowId;
	    return 0;
	}

	int TMdbVarCharCtrl::GetStoragePos(char * pAddr, int & iWhichPos, unsigned int & iRowId)
	{
	    int i = 0;
	    const char *pHead = &(pAddr[i]);
	    iWhichPos = (int)((*pHead)-'0'); //判断是16字节还是32字节等
	    i += 1;
	    iRowId = *(unsigned int*)&pAddr[i];
	    return 0;
	}

	int TMdbVarCharCtrl::GetVarcharValue(char * pResultValue, char * const pData)
	{
	    TADD_FUNC("Start.");
	    int iRet = 0;
	    int iWhichPos = -1;
	    unsigned int iRowId = 0;
	    GetStoragePos(pData, iWhichPos, iRowId);
	    TADD_DETAIL("pData=[%s],iWhichPos=[%d],iRowId=[%ud].",pData,iWhichPos,iRowId);
	    if(iWhichPos < VC_16 || iWhichPos > VC_8192)
	    {
	        TADD_ERROR(ERROR_UNKNOWN,"iWhichFlag=[%d],iWhichFlag IN [0,9],pData=[%s]",iWhichPos,pData);
	        pResultValue[0] = 0;
	        return iRet;
	    }
	    m_pVarChar = m_pShmDSN->GetVarchar(iWhichPos);
	    CHECK_OBJ(m_pVarChar);
	    TMdbRowID tRowId;
	    tRowId.SetRowId(iRowId);
	    char* pAddr = GetAddressRowId(&tRowId);
	    CHECK_OBJ(pAddr);
	    int iSize = GetValueSize(iWhichPos);
	    SAFESTRCPY(pResultValue, iSize, pAddr);
	    //memcpy(pResultValue,pAddr,iSize);
	    //pResultValue[iSize] = 0;
	    TADD_FUNC("TMdbVarcharMgr::GetVarcharValue() ： Finish(0).");
	    return iRet;
	}

	int TMdbVarCharCtrl::CreateOrAddFile()
	{
	    return 0;
	}
	/*
	int TMdbVarCharCtrl::ReBuildFromDisk(TMdbVarcharFile * pVarCharFile)
	{
	    int iRet = 0;
	    TADD_FUNC("Start.");
	    CHECK_OBJ(pVarCharFile);
	    TMdbPageCtrl   PageCtrl;
	    CHECK_RET(PageCtrl.SetDSN(m_pShmDSN->GetInfo()->sName),"PageCtrl SetDSN[%s] Faild.",m_pShmDSN->GetInfo()->sName);
	    TMdbPage * pFilePage = NULL;
	    CHECK_RET(pVarCharFile->StartToReadPage(),"StartToReadPage failed.");
	    while((iRet = pVarCharFile->GetNextPage(pFilePage)) > 0)
	    {
	        //TADD_DETAIL("TSID=[%d],sTableName=[%s],pageid=[%d]",pTSFile->m_tTSFileHead.m_iTSID,pFilePage->m_sTableName,pFilePage->m_iPageID);
	        //TMdbTable * pTable = m_pShmDSN->GetTableByName(pFilePage->m_sTableName);
	        //if(pTable == NULL){continue;}
	        //CHECK_OBJ(pTable);
	        //CHECK_RET(StorageCopyCtrl.Init(pTable, m_pTMdbTableSpace,&PageCtrl,this),"StorageCopyCtrl Init Faild.");
	        //CHECK_RET(StorageCopyCtrl.Load(pFilePage),"StorageCopyCtrl Load Faild,PageID[%d],table_name[%s]",pFilePage->m_iPageID,pFilePage->m_sTableName);
	    }
	    TADD_FUNC("Finish");
	    return iRet;
	}
	*/

	/******************************************************************************
	* 函数名称	:  GetFreePage
	* 函数描述	:  获取一个空闲页
	* 输入		:  段位置
	* 输入		:  
	* 输出		:  空闲地址
	* 返回值	: 
	* 作者		:  dong.chun
	*******************************************************************************/
	int TMdbVarCharCtrl::GetFreePage(TMdbPage * & pFreePage)
	{
	    TADD_FUNC(" Start.");
	    int iRet = 0;
	    CHECK_OBJ(m_pVarChar);
	    pFreePage = NULL;
	    do{
	        if(m_pVarChar->iFreePageId <= 0)
	        {//没有空闲页了需要申请
	            CHECK_RET_BREAK(CreateShm(m_pVarChar),"CreateVarchar error.");
	            if(m_pVarChar->iFreePageId <= 0)
	            {
	                CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"m_pVarChar->iFreePageId[%d] error.",
	                                                    m_pVarChar->iFreePageId);
	            }
	        }
	        pFreePage = (TMdbPage *)GetAddrByPageID(m_pVarChar,m_pVarChar->iFreePageId);
	        if(NULL == pFreePage)
	        {
	            CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"GetAddrByPageID[%d] error.",m_pVarChar->iFreePageId);
	        }
	    }while(0);
	    TADD_FUNC("Finish.");
	    return iRet;
	}
	/******************************************************************************
	* 函数名称	:  GetValueSize
	* 函数描述	:  获取数据存储长度
	* 输入		:  段位置
	* 输入		:  
	* 输出		:  实际存储长度
	* 返回值	: 
	* 作者		:  dong.chun
	*******************************************************************************/
	int TMdbVarCharCtrl::GetValueSize(int iWhichPos)
	{
	    int iActLen=-1;
	    switch(iWhichPos)
	    {
	        case VC_16:{iActLen = 16;break;}
	        case VC_32:{iActLen = 32;break;}
	        case VC_64:{iActLen = 64;break;}
	        case VC_128:{iActLen = 128;break;}
	        case VC_256:{iActLen = 256;break;}
	        case VC_512:{iActLen = 512;break;}
	        case VC_1024:{iActLen = 1024;break;}
	        case VC_2048:{iActLen = 2048;break;}
	        case VC_4096:{iActLen = 4096;break;}
	        case VC_8192:{iActLen = 8192;break;}
	        default:
	        {
	            TADD_ERROR(-1,"GetValueSize Faild,whichpos[%d]",iWhichPos);
	            break;
	        }
	    };
	    return iActLen;
	}

	/******************************************************************************
	* 函数名称	:  RemovePage
	* 函数描述	:  移除一个page
	* 输入		:  
	* 输入		:  
	* 输出		:  
	* 返回值	: 
	* 作者		:  dong.chun
	*******************************************************************************/
	int TMdbVarCharCtrl::RemovePage(TMdbPage * pPageToRemove, int & iHeadPageId)
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
	        TMdbPage * pPrePage = (TMdbPage * )GetAddrByPageID(m_pVarChar,pPageToRemove->m_iPrePageID);
	        CHECK_OBJ(pPrePage);
	        pPrePage->m_iNextPageID = pPageToRemove->m_iNextPageID;
	    }
	    
	    if(pPageToRemove->m_iNextPageID > 0)
	    {//非尾节点
	        TMdbPage * pNextPage = (TMdbPage *)GetAddrByPageID(m_pVarChar,pPageToRemove->m_iNextPageID);
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
	* 函数描述	:  增加一个page
	* 输入		:  
	* 输入		:  
	* 输出		:  
	* 返回值	: 
	* 作者		:  dong.chun
	*******************************************************************************/
	int TMdbVarCharCtrl::AddPageToTop(TMdbPage * pPageToAdd, int & iHeadPageId)
	{
	    TADD_FUNC("Start.iHeadPageId=[%d],page:pre[%d] next[%d]",iHeadPageId,pPageToAdd->m_iPrePageID,pPageToAdd->m_iNextPageID);
	    int iRet = 0;
	    if(iHeadPageId > 0)
	    {
	        TMdbPage * pNextPage    = (TMdbPage *)GetAddrByPageID(m_pVarChar,iHeadPageId);
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

	int TMdbVarCharCtrl::PageFreeToFull(TMdbPage* pCurPage)
	{
	    TADD_FUNC("Start.");
	    int iRet = 0;
	    CHECK_OBJ(m_pVarChar);
	    //从free链上移除
	    CHECK_RET(RemovePage(pCurPage,m_pVarChar->iFreePageId),"RemovePage failed.");
	    //添加到full链上
	    CHECK_RET(AddPageToTop(pCurPage,m_pVarChar->iFullPageId),"AddPageToTop failed.");
	    //把状态设置为full
	    SAFESTRCPY(pCurPage->m_sState,sizeof(pCurPage->m_sState), "full");
	    TADD_FUNC("Finish.");
	    return iRet;
	}

	int TMdbVarCharCtrl::PageFullToFree(TMdbPage* pCurPage)
	{
	    TADD_FUNC("Start.");
	    int iRet = 0;
	    CHECK_OBJ(m_pVarChar);
	        //从full链上移除
	    CHECK_RET(RemovePage(pCurPage,m_pVarChar->iFullPageId),"RemovePage failed.");
	    //添加到free链上
	    CHECK_RET(AddPageToTop(pCurPage,m_pVarChar->iFreePageId),"AddPageToTop failed.");
	    //把状态设置为free
	    SAFESTRCPY(pCurPage->m_sState, sizeof(pCurPage->m_sState),"free");
	    TADD_FUNC("Finish.");
	    return iRet;
	}

	int TMdbVarCharCtrl::SetPageDirtyFlag(int iPageID)
	{
	    int iRet = 0;
	    if(iPageID < 0){CHECK_RET(ERR_APP_INVALID_PARAM,"PageID can not be [%d]",iPageID);}
	    TMdbTSNode * pTSNode = GetNodeByPageId(iPageID);
	    CHECK_OBJ(pTSNode);
	    int iBytePos = ((iPageID - pTSNode->iPageStart)/8)%MAX_BIT_MAP_SIZE; //防止溢出
	    int iBitPos = (iPageID - pTSNode->iPageStart)%8;
           TADD_DETAIL("iPageID=[%d],bytepos=[%d],bitpos=[%d],start[%d],end=[%d]",iPageID,iBytePos,iBitPos,pTSNode->iPageStart,pTSNode->iPageEnd);
	    //设置脏页
	    unsigned char cTemp = (0x1<<iBitPos);
	    pTSNode->bmDirtyPage[iBytePos] |= cTemp;
	    return iRet;
	}
	/******************************************************************************
	* 函数名称	:  GetAddressRowId
	* 函数描述	:  根据rowid获取数据地址
	* 输入		:  
	* 输入		:  
	* 输出		:  
	* 返回值	: 
	* 作者		:  dong.chun
	*******************************************************************************/
	char* TMdbVarCharCtrl::GetAddressRowId(TMdbRowID* pRowID)
	{
	    TADD_FUNC("Start.RowID[%d|%d]",pRowID->GetPageID(),pRowID->GetDataOffset());
	    char* pAddr = NULL;
	    TMdbTSNode* pNodeTmp = &m_pVarChar->tNode;
	    int iPageID = pRowID->GetPageID();
	    //首先找到对应的页面地址
	    while(true)
	    {
	        if(NULL == pNodeTmp)
	        {
	            TADD_ERROR(-1,"TSNode is null ");
	            return NULL;
	        }
	        //如果页面在当前的范围内, 则转换成内部地址
	        if(iPageID >= pNodeTmp->iPageStart && iPageID <= pNodeTmp->iPageEnd)
	        {
	            if(pNodeTmp->iShmID < 0)return NULL;
	            pAddr = m_pShmDSN->GetVarcharShmAddr(pNodeTmp->iShmID);
	            if(pAddr == NULL)
	            {
	                TADD_ERROR(-1,"Can't GetShmAddr(iShmID=%d)", pNodeTmp->iShmID);
	                return NULL;    
	            }
	            //计算出相应页面的地址,找到
	            pAddr += (iPageID-pNodeTmp->iPageStart)*m_pVarChar->iPageSize;
	            break;
	        }
	        else
	        {
	            if(0 == pNodeTmp->iNext )
	            {
	                return NULL;    
	            }
	            else
	            {     
	                pNodeTmp = (TMdbTSNode*)(m_pShmDSN->GetAddrByOffset(pNodeTmp->iNext));
	            }
	        }
	    }
	    if(NULL != pAddr)
	    {
	        TMdbPage * pPage = (TMdbPage * )pAddr;
	        pAddr = pAddr + pPage->RecordPosToDataOffset(pRowID->GetDataOffset());
	    }
	    return pAddr;
	}

	TMdbTSNode* TMdbVarCharCtrl::GetNodeByPageId(int iPageID)
	{
	    if(m_pVarChar == NULL){return NULL;}
	    if(iPageID <= 0){return NULL;}
	    TMdbTSNode * pTSNode = &(m_pVarChar->tNode);
	    while(pTSNode != NULL)
	    {
	        if(iPageID >= pTSNode->iPageStart && iPageID <= pTSNode->iPageEnd)
	        {
	            return pTSNode;
	        }
	        pTSNode = GetNextNode(pTSNode);
	    }
        return NULL;
	}

	bool TMdbVarCharCtrl::NeedStorage()
	{
	    return m_bStorageFlag;
	}

int TMdbVarCharCtrl::AddToFile(TMdbVarchar *pVarChar,TMdbTSNode& node)
{
    int iRet = 0;
    char sStorageFile[MAX_FILE_NAME] = {0};
    sprintf(sStorageFile,"%s/%s%d_%d%s",m_pShmDSN->GetInfo()->sStorageDir,VARCHAR_FILE_PREFIX,pVarChar->iVarcharID,pVarChar->iBlockId<1?0:pVarChar->iBlockId-1,TS_FILE_SUFFIX);
    TADD_NORMAL("Add to [%s]",sStorageFile);
    TMdbTSFileHead m_VarcharHead;
    m_VarcharHead.Clear();
    m_VarcharHead.iVarcharID = m_pVarChar->iVarcharID;
    m_VarcharHead.m_iPageOffset = sizeof(TMdbTSFileHead);
    m_VarcharHead.m_iPageSize = m_pVarChar->iPageSize;
    m_VarcharHead.m_iEndPage = node.iPageEnd;
    m_pVarcharFile = NULL;
    SAFE_CLOSE(m_pVarcharFile);
    if(TMdbNtcFileOper::IsExist(sStorageFile))
    {
        long long iFileSize=GetFileSize(sStorageFile);
        if(iFileSize < 0)
        {
            TADD_ERROR(-1,"GetFileSize() failed.", sStorageFile);
            return iFileSize;    
        }
        long long iAddSize = (node.iPageEnd - node.iPageStart) * m_pVarChar->iPageSize;
        if(iFileSize + iAddSize >= 1024*1024*1024)
        {
            SAFE_CLOSE(m_pVarcharFile);
            TADD_NORMAL("[%s] is Full",sStorageFile);
            memset(sStorageFile,0,sizeof(sStorageFile));
            //pVarChar->iBlockId++;
            m_VarcharHead.m_iStartPage = node.iPageStart;
            sprintf(sStorageFile,"%s/%s%d_%d%s",m_pShmDSN->GetInfo()->sStorageDir,VARCHAR_FILE_PREFIX,pVarChar->iVarcharID,pVarChar->iBlockId,TS_FILE_SUFFIX);
            TADD_NORMAL("Add to New File[%s]",sStorageFile);
            m_pVarcharFile = fopen(sStorageFile,"wb+");
			pVarChar->iBlockId++;
        }
        else
        {
            m_pVarcharFile = fopen(sStorageFile,"rb+");
            if(fread(&m_VarcharHead,sizeof(m_VarcharHead),1,m_pVarcharFile) != 1)
            {
                if(feof(m_pVarcharFile))
                {//文件结束
                    CHECK_RET(ERR_APP_INVALID_PARAM,"reach file end.");
                }
                CHECK_RET(ERR_OS_READ_FILE,"fread(%s) errno=%d, errormsg=[%s].",sStorageFile, errno, strerror(errno));
            }
            m_VarcharHead.m_iEndPage = node.iPageEnd;
        }
    }
    else
    {
        m_VarcharHead.m_iStartPage = node.iPageStart;
        m_pVarcharFile = fopen(sStorageFile,"wb+");
		pVarChar->iBlockId++;
    }
    
    CHECK_OBJ(m_pVarcharFile);

    //刷出文件头
    CHECK_RET(fseek(m_pVarcharFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
    if(fwrite(&m_VarcharHead, sizeof(TMdbTSFileHead), 1, m_pVarcharFile)!= 1 )
    {
        CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
    }
    //移动到文件尾，把新增加的页面写入文件
    CHECK_RET(fseek(m_pVarcharFile,0,SEEK_END),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
    int iPageID = node.iPageStart;
    for(;iPageID <= pVarChar->iTotalPages; ++iPageID)
    {
        TMdbPage * pPage = (TMdbPage *)GetAddrByPageID(pVarChar,iPageID);
        CHECK_OBJ(pPage);
        m_mdbPageCtrl.Attach((char *)pPage,false,true);
        m_mdbPageCtrl.WLock();
        do{
            if(fwrite(pPage, pPage->m_iPageSize, 1, m_pVarcharFile)!= 1 )
            {
                CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite(%s) errno=%d, errormsg=[%s].",sStorageFile, errno, strerror(errno));
            }
        }while(0);
        m_mdbPageCtrl.UnWLock();
    }

    //刷出文件头
    CHECK_RET(fseek(m_pVarcharFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
    if(fwrite(&m_VarcharHead, sizeof(TMdbTSFileHead), 1, m_pVarcharFile)!= 1 )
    {
        CHECK_RET(ERR_OS_WRITE_FILE,"fwrite errno=%d, errormsg=[%s].", errno, strerror(errno));
    }
    fflush(m_pVarcharFile);
    return iRet;
}

long long TMdbVarCharCtrl::GetFileSize(char * sFile)
{
        struct stat f_stat;
        if(stat(sFile, &f_stat) == -1) 
        {
            return ERR_OS_NO_FILE;
        }
        return (long long)f_stat.st_size;
}
void TMdbVarCharCtrl::SetStorageFlag()
{
    TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
    for(;itor != m_pShmDSN->m_TSList.end();++itor)
    {
        TMdbTableSpace * pTS = &(*itor);
        if(pTS->sName[0] != 0)
        {
            if(pTS->m_bFileStorage) 
            {
                m_bStorageFlag = true;
                return;
            }
        }
    }
    m_bStorageFlag = false;
    return;
 }

void TMdbVarCharCtrl::SetVarchar(TMdbVarchar * pVarChar)
{
    m_pVarChar = pVarChar;
    return;
}

//}

