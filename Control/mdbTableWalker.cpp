/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbTableWalker.cpp		
*@Description： MDB表遍历类。按照索引进行遍历。
*@Author:		jin.shaohua
*@Date：	    2012.04
*@History:
******************************************************************************************/
#include "mdbTableWalker.h"


//namespace QuickMDB{

/******************************************************************************
* 函数名称	:  TMdbTableWalker
* 函数描述	:  表遍历类的构造
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbTableWalker::TMdbTableWalker():
m_pShmDSN(NULL),
m_pMdbDSN(NULL),
m_pTableSpace(NULL),
m_pMdbTable(NULL),
m_iDataSize(0),
m_pDataAddr(NULL),
m_pNextDataAddr(NULL),
m_pCurPage(NULL),
m_iPagePos(0),
m_pStartPage(NULL)
{
	m_tNextRowIDData.Clear();
	m_tCurRowIDData.Clear();
	m_iNextIndexPos = -1;
    m_bScanAll = false;
	m_iCurWalkIndexAlgo = 0;
	m_pConflictIndex = NULL;
	m_pMHashBaseIdx = NULL;
	m_tMHashRowIdx.Clear();
	m_iScanPageType = TYPE_SCAN_PAGE_FREE;
	m_iFirstFullPageID = -1;
	m_iFirstFreePageID = -1;
	m_pNextPage = NULL;
	m_iAffect = 0;
	m_pTrieRootNode = NULL;
	m_pTrieConfIndex = NULL;
    m_pTrieBranchIndex = NULL;	
	m_bStopScanTrie = true;
	
}
/******************************************************************************
* 函数名称	:  ~TMdbTableWalker
* 函数描述	:  表遍历类的析构
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbTableWalker::~TMdbTableWalker()
{
	
}

/******************************************************************************
* 函数名称	:  AttachTable
* 函数描述	:  链接上某张表
* 输入		:  pShmDSN - DSN 内存区
* 输入		:  pMdbTable - 需要遍历的表
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbTableWalker::AttachTable(TMdbShmDSN * pShmDSN,TMdbTable * pMdbTable)
{
	int iRet = 0;
    StopWalk();
	CHECK_OBJ(pShmDSN);
	CHECK_OBJ(pMdbTable);
	TADD_FUNC("Start.DSN=[%s],Table=[%s].",pShmDSN->GetInfo()->sName,pMdbTable->sTableName);
	m_pMdbTable = pMdbTable;//MDB表
	m_pShmDSN = pShmDSN;
	m_pMdbDSN = pShmDSN->GetInfo();//获取数据库DSN信息
	CHECK_OBJ(m_pMdbDSN);
	m_pTableSpace = m_pShmDSN->GetTableSpaceAddrByName(pMdbTable->m_sTableSpace);
	CHECK_OBJ(m_pTableSpace);
    m_tTSCtrl.Init(pShmDSN->GetInfo()->sName,pMdbTable->m_sTableSpace);
	TADD_FUNC("Finish.AttachTable [%s] ",pMdbTable->sTableName);
	return iRet;
}
     

    int TMdbTableWalker::WalkByIndex(ST_TABLE_INDEX_INFO* pIndexInfo, long long lIndexValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        m_iCurWalkIndexAlgo = pIndexInfo->pIndexInfo->m_iAlgoType;
        TADD_DETAIL("m_iCurWalkIndexAlgo=%d, indexname=[%s],lIndexValue=[%lld].",m_iCurWalkIndexAlgo, pIndexInfo->pIndexInfo->sName,lIndexValue);
        switch(m_iCurWalkIndexAlgo)
        {
            case INDEX_HASH:
                iRet = WalkByHashIndex(pIndexInfo, lIndexValue);
                break;
            case INDEX_M_HASH:
                iRet = WalkByMHashIndex(pIndexInfo, lIndexValue);
                break;
			case INDEX_TRIE:
                iRet = WalkByTrieIndex(pIndexInfo);
                break;
        }

        return iRet;
    }


/******************************************************************************
* 函数名称	:  StopWalk
* 函数描述	:  停止遍历(暂无用)
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbTableWalker::StopWalk()
{
	int iRet = 0;
	m_tCurRowIDData.Clear();
	m_tNextRowIDData.Clear();
	m_iNextIndexPos = -1;
	m_iDataSize = 0;
	m_pDataAddr =NULL;
	m_pCurPage = NULL;
	m_pStartPage = NULL;
	m_iPagePos = 0;
	m_iCurWalkIndexAlgo = INDEX_UNKOWN;
	
	m_pMHashBaseIdx = NULL;
	m_tMHashRowIdx.Clear();
		
	m_pTrieRootNode = NULL;
	m_pTrieConfIndex = NULL;
    m_pTrieBranchIndex = NULL;
	m_bStopScanTrie = true;

	m_bScanAll =false;
	m_iScanPageType = TYPE_SCAN_PAGE_FREE;
	m_iAffect = 0;
	m_iFirstFullPageID = -1;
	m_iFirstFreePageID = -1;
	m_pNextPage = NULL;
	
	return iRet;
}

/******************************************************************************
* 函数名称	:  Next
* 函数描述	:  获取下一个:false = 没有获取到，true = 有获取到
冲突索引节点
+---++---+
 |data|Next|
+---++---+

* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
    bool TMdbTableWalker::Next()
    {
        TADD_FUNC("Start.");
        bool bRet = false;
        m_pDataAddr = NULL;//初始化为NULL

        if(m_iCurWalkIndexAlgo == INDEX_HASH)
        {
            bRet = NextByHash();
        }
        else if(m_iCurWalkIndexAlgo == INDEX_M_HASH)
        {
            bRet = NextByMHash();
        }
		else if(m_iCurWalkIndexAlgo == INDEX_TRIE)
        {
            bRet = NextByTrie();
        }

        TADD_FUNC("Finish.");
        return bRet;
    }

/******************************************************************************
* 函数名称	:  GetAddressRowID
* 函数描述	:  根据RowID, 找到对应的位置
* 输入		:  pRowID - 某条记录的rowid
* 输入		:  bGetPageAddr - 是否获取page位置
* 输出		:  iDataSize - 数据大小
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
char* TMdbTableWalker::GetAddressRowID(TMdbRowID* pRowID, int &iDataSize, bool bGetPageAddr)
{    
    TADD_FUNC("Start.RowID[%d|%d],bGetPageAddr[%d]",pRowID->GetPageID(),pRowID->GetDataOffset(),bGetPageAddr);
    char* pAddr = NULL;
    TMdbTSNode* pNodeTmp = &m_pTableSpace->tNode;
    int iPageID = pRowID->GetPageID();
    //首先找到对应的页面地址
    while(true)
    {
        if(NULL == pNodeTmp)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"TSNode is null ");
            return NULL;
        }
        //如果页面在当前的范围内, 则转换成内部地址
        if(iPageID >= pNodeTmp->iPageStart && iPageID <= pNodeTmp->iPageEnd)
        {
            if(pNodeTmp->iShmID < 0)return NULL;
            pAddr = m_pShmDSN->GetDataShmAddr(pNodeTmp->iShmID);
            if(pAddr == NULL)
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM,"Can't GetShmAddr(iShmID=%d)", pNodeTmp->iShmID);
                return NULL;    
            }
            //计算出相应页面的地址,找到
            pAddr += pNodeTmp->iOffSet + (iPageID-pNodeTmp->iPageStart)*m_pTableSpace->iPageSize;
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
        m_pCurPage = (TMdbPage *)pAddr;
        iDataSize = pPage->m_iRecordSize;
        pAddr = pAddr + m_pCurPage->RecordPosToDataOffset(pRowID->GetDataOffset());
        m_iPagePos  = pRowID->GetDataOffset();
    }
    TADD_FUNC("Finish.m_pPageAddr=[%p],m_iPagePos=[%d],iDataSize=[%d],pAddr=[%p].",m_pCurPage,m_iPagePos,iDataSize,pAddr);
    return pAddr;
}

/******************************************************************************
* 函数名称	:  WalkByPage
* 函数描述	:  根据页来遍历数据
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbTableWalker::WalkByPage(int iStartPageID)
{
    int iRet = 0;
    if(iStartPageID <= 0){m_pCurPage = NULL;m_pStartPage = NULL; return iRet;}
    m_pCurPage = (TMdbPage *)m_tTSCtrl.GetAddrByPageID(iStartPageID);
	m_pStartPage = (TMdbPage *)m_tTSCtrl.GetAddrByPageID(iStartPageID);
    TADD_DETAIL("m_iPageID=[%d],m_iRecordCounts=[%d],m_iFreeOffSet=[%d],head=[%d]",m_pCurPage->m_iPageID,m_pCurPage->m_iRecordCounts,m_pCurPage->m_iFreeOffSet,sizeof(TMdbPage));
    return iRet;
}

/******************************************************************************
* 函数名称	:  NextByPage
* 函数描述	:  页中的下一条数据
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  true/false
* 作者		:  jin.shaohua
*******************************************************************************/
bool TMdbTableWalker::NextByPage()
{	
	int iRet = 0; 
	while(NULL != m_pCurPage)
	{	
		//加读锁
		CHECK_RET_BREAK(m_mdbPageCtrl.Attach((char*)m_pCurPage, m_pMdbTable->bReadLock, m_pMdbTable->bWriteLock),"TMdbTableWalker::m_mdbPageCtrl.Attach faild");
	    CHECK_RET_BREAK(m_mdbPageCtrl.RLock(),"TMdbTableWalker::PageCtrl.RLock() failed.");
    	//获取下一数据
    	m_pDataAddr = m_pCurPage->GetNextDataAddr(m_pDataAddr,m_iPagePos,m_pNextDataAddr);
		
		//以防页面被回收后丢失信息,需要提前取出下一个页面
		if(m_pNextPage == NULL && m_pCurPage->m_iNextPageID>0)
		{
			m_pNextPage = (TMdbPage *)m_tTSCtrl.GetAddrByPageID(m_pCurPage->m_iNextPageID);
			//TADD_NORMAL("CurPage:%d.NextPage:%d.",m_pCurPage->m_iPageID,m_pNextPage->m_iPageID);
		}
		//解读锁
	    m_mdbPageCtrl.UnRLock();
		
    	if(NULL == m_pDataAddr)
		{
        	//尝试下一页
        	if(m_pCurPage->m_iNextPageID <= 0)
        	{
				m_pCurPage = NULL;
				break;
			}
			
			//TADD_NORMAL("CurPage:%d has no data.move to NextPage:%d.",m_pCurPage->m_iPageID,m_pNextPage->m_iPageID);

			m_pCurPage = m_pNextPage;
			m_pNextPage = NULL;
			
    		if(m_pCurPage == NULL){break;}
			if(m_pCurPage->m_iPageID == m_pStartPage->m_iPageID)
			{
				break;
			}
    	}
    	else
		{				
            m_tCurRowIDData.SetPageID(m_pCurPage->m_iPageID);
            m_tCurRowIDData.SetDataOffset(m_pCurPage->DataOffsetToRecordPos(m_iPagePos));
            m_iDataSize = m_pCurPage->m_iRecordSize;
			m_iPagePos  = m_tCurRowIDData.GetDataOffset();
			//TADD_NORMAL("CurPage:%d ,m_iPagePos :%d.",m_pCurPage->m_iPageID,m_iPagePos);
            return true;
    	}
    }
    return false;
}

    int TMdbTableWalker::WalkByHashIndex(ST_TABLE_INDEX_INFO* pIndexInfo, long long lIndexValue)
    {
        int iRet = 0;
        TADD_DETAIL("walk by hash index.");

        m_tCurRowIDData.Clear();
                
        m_tNextRowIDData =  pIndexInfo->m_HashIndexInfo.pBaseIndexNode[lIndexValue].tData;
        m_iNextIndexPos =  pIndexInfo->m_HashIndexInfo.pBaseIndexNode[lIndexValue].iNextPos; 
        m_pConflictIndex = pIndexInfo->m_HashIndexInfo.pConflictIndexNode;
        
        TADD_FUNC("Finish.next RowIdData[%d|%d] next pos=[%d].", m_tNextRowIDData.GetPageID(),  m_tNextRowIDData.GetDataOffset(), m_iNextIndexPos);
        return iRet;

    }

    

    /******************************************************************************
    * 函数名称	:  NextByHash
    * 函数描述	:  获取下一个:false = 没有获取到，true = 有获取到
    冲突索引节点
    +---++---+
     |data|Next|
    +---++---+

    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool TMdbTableWalker::NextByHash()
    {
        TADD_FUNC("Start.");
        m_pDataAddr = NULL;//初始化为NULL
        TMdbIndexNode* pTempIndexNode = NULL;
        do
        {	//获取数据
            if(false == m_tNextRowIDData.IsEmpty())
            {//根据rowID取到真实数据的地址
                TADD_DETAIL("m_tNextRowIDData.iPageID=%d, m_tRowIDData.iNum=%d.",  m_tNextRowIDData.GetPageID(), m_tNextRowIDData.GetDataOffset());
                m_pDataAddr     = GetAddressRowID(&m_tNextRowIDData, m_iDataSize, true);
                m_tCurRowIDData = m_tNextRowIDData;
            }
            if(NULL == m_pDataAddr && m_iNextIndexPos < 0)
            {//链遍历结束
                StopWalk();//做一些清理工作
                TADD_FUNC("Finish[false].");
                return false;
            }
            if(m_iNextIndexPos < 0)
            {//没有下一个节点
                m_tNextRowIDData.Clear();
            }
            else
            {//有下一个节点
                pTempIndexNode   = &(m_pConflictIndex[m_iNextIndexPos]);
                m_tNextRowIDData = pTempIndexNode->tData;
                m_iNextIndexPos  = pTempIndexNode->iNextPos;
            }
            if(NULL != m_pDataAddr)
            {//获取到数据
                TADD_FUNC("Finish[true].");
                return true;
            }
        }while(1);
        TADD_FUNC("Finish[false],out of while.");
        return false;
    }

    int TMdbTableWalker::WalkByMHashIndex(ST_TABLE_INDEX_INFO* pIndexInfo, long long lIndexValue)
    {
        int iRet = 0;
        
        m_tCurRowIDData.Clear();
        m_tMHashRowIdx.Clear();

        m_tMHashRowIdx.m_iHashValue = lIndexValue;
        m_tMHashRowIdx.iBaseIndexPos = lIndexValue% m_pMdbTable->iTabLevelCnts;
        TADD_DETAIL("M-HASH.[%s][%s] basepos=[%lld],hash[%lld]",pIndexInfo->m_MHashIndexInfo.pBaseIndex->sTabName,pIndexInfo->m_MHashIndexInfo.pBaseIndex->sName,m_tMHashRowIdx.iBaseIndexPos ,lIndexValue);

        if(pIndexInfo->m_MHashIndexInfo.pBaseIndexNode[m_tMHashRowIdx.iBaseIndexPos].m_tBaseNode.m_iHashValue == m_tMHashRowIdx.m_iHashValue)
        {
            m_tMHashRowIdx.iConflictIndexPos = pIndexInfo->m_MHashIndexInfo.pBaseIndexNode[m_tMHashRowIdx.iBaseIndexPos].m_tBaseNode.m_iConfPos;
            m_tMHashRowIdx.iLayerIndexPos =-1;
        }
        else
        {
            m_tMHashRowIdx.iLayerIndexPos = pIndexInfo->m_MHashIndexInfo.pBaseIndexNode[m_tMHashRowIdx.iBaseIndexPos].m_tBaseNode.m_iLayerPos;
            m_tMHashRowIdx.iConflictIndexPos = -1;
        }
        
        m_tNextRowIDData = pIndexInfo->m_MHashIndexInfo.pBaseIndexNode[m_tMHashRowIdx.iBaseIndexPos].m_tBaseNode.m_tRowId;

        m_pMHashBaseIdx = pIndexInfo->m_MHashIndexInfo.pBaseIndexNode;

        TMdbMHashConflictIndexMgrInfo* pConfMgr = m_pShmDSN->GetMHashConfMgr();
        CHECK_OBJ(pConfMgr);
        m_tMHashRowIdx.pConfIndex = &(pConfMgr->tIndex[pIndexInfo->m_MHashIndexInfo.pBaseIndex->iConflictIndexPos]);
        TMdbMHashLayerIndexMgrInfo* pLayerMgr = m_pShmDSN->GetMHashLayerMgr();
        m_tMHashRowIdx.pLayerIndex= &(pLayerMgr->tIndex[pIndexInfo->m_MHashIndexInfo.pBaseIndex->iLayerIndexPos]);      
        
        TADD_FUNC("Finish.next Row[%d],next pos:conf[%lld],layer[%lld].", m_tNextRowIDData.m_iRowID,  m_tMHashRowIdx.iConflictIndexPos,m_tMHashRowIdx.iLayerIndexPos);
        return iRet;
    }
	
    bool TMdbTableWalker::NextByMHash()
    {
        TADD_FUNC("Start.");
        m_pDataAddr = NULL;//初始化为NULL
             
        int iConfNodeSize = sizeof(TMdbMHashConfIndexNode);
        int iLayerNodeSize = sizeof(TMdbMhashLayerIndexNode);
        do
        {	//获取数据
            if(false == m_tNextRowIDData.IsEmpty())
            {//根据rowID取到真实数据的地址
                TADD_DETAIL("m_tNextRowIDData.iPageID=%d, m_tRowIDData.iNum=%d.",  m_tNextRowIDData.GetPageID(), m_tNextRowIDData.GetDataOffset());
                m_pDataAddr     = GetAddressRowID(&m_tNextRowIDData, m_iDataSize, true);
                m_tCurRowIDData = m_tNextRowIDData;
            }
            if(NULL == m_pDataAddr && m_tMHashRowIdx.IsOnConflict()== false && m_tMHashRowIdx.IsOnLayer() == false)
            {//链遍历结束
                StopWalk();//做一些清理工作
                TADD_FUNC("Finish[false].");
                return false;
            }
            if(m_tMHashRowIdx.IsOnConflict()== false && m_tMHashRowIdx.IsOnLayer() == false)
            {//没有下一个节点
                m_tNextRowIDData.Clear();

                // 基本链和基本链上的冲突链遍历完成后，还需要遍历最底层阶梯节点上的冲突链
                if(m_tMHashRowIdx.m_bCheckLastLayerConf) 
                {
                	TADD_DETAIL("check last layer");
                    m_tMHashRowIdx.m_bCheckLastLayerConf = false;

                    // 获取最下层阶梯索引节点上的冲突链节点
                    TMdbMHashBaseIndexNode* pBaseNode = NULL;
                    TMdbMhashLayerIndexNode* pLayerNode = NULL;
                    int iNextLayerPos = -1;
                    int iLastLayerPos = -1;
                    int iLayerOffSet = -1;
                    iLayerOffSet = TMdbMhashAlgo::CalcLayerPos(m_tMHashRowIdx.m_iHashValue);     
                    
                    pBaseNode = &(m_pMHashBaseIdx[m_tMHashRowIdx.iBaseIndexPos]);
                    iNextLayerPos = pBaseNode->m_tBaseNode.m_iLayerPos;
                    iLastLayerPos = iNextLayerPos;
                    
                    while(iNextLayerPos >= 0)
                    {
                        pLayerNode = (TMdbMhashLayerIndexNode*)GetAddrByMhashIndexNodeId(m_tMHashRowIdx.pLayerIndex->iHeadBlockId,
                            iNextLayerPos,iLayerNodeSize,false );
                        CHECK_OBJ(pLayerNode);
                        iLastLayerPos = iNextLayerPos;
                        iNextLayerPos = pLayerNode->m_atNode[iLayerOffSet].m_iLayerPos;                         
                    }

                    if(iLastLayerPos > 0) // 存在找到的最底层阶梯索引节点
                    {
                        pLayerNode = (TMdbMhashLayerIndexNode*)GetAddrByMhashIndexNodeId(m_tMHashRowIdx.pLayerIndex->iHeadBlockId,
                            iLastLayerPos,iLayerNodeSize,false );
                        CHECK_OBJ(pLayerNode);
                        m_tNextRowIDData = pLayerNode->m_atNode[iLayerOffSet].m_tRowId;
                        m_tMHashRowIdx.iConflictIndexPos = pLayerNode->m_atNode[iLayerOffSet].m_iConfPos;
                        m_tMHashRowIdx.iLayerIndexPos = -1;
                        TADD_DETAIL("get last layer.next pos:conf[%lld],layer[%lld]",m_tMHashRowIdx.iConflictIndexPos,m_tMHashRowIdx.iLayerIndexPos);
                    }
                    
                }
                
                
            }
            else
            {//有下一个节点
                if(m_tMHashRowIdx.IsOnConflict())
                {
                	TADD_DETAIL("check on conflict");
                    TMdbMHashConfIndexNode* pTmpNode = (TMdbMHashConfIndexNode*)GetAddrByMhashIndexNodeId(m_tMHashRowIdx.pConfIndex->iHeadBlockId
                                                                                                                                                    , m_tMHashRowIdx.iConflictIndexPos, iConfNodeSize, true);
                    CHECK_OBJ(pTmpNode);
                    m_tNextRowIDData = pTmpNode->m_tRowId;
                    m_tMHashRowIdx.iConflictIndexPos = pTmpNode->m_iNextNode;
                    m_tMHashRowIdx.iLayerIndexPos = -1;
                    TADD_DETAIL("CONF:next on conflict[%d]",m_tMHashRowIdx.iConflictIndexPos);
                }
                else if(m_tMHashRowIdx.IsOnLayer())
                {
                	TADD_DETAIL("check on layer");
                    // 若已经是遍历阶梯链节点，则无需再遍历最底层阶梯链上的冲突链。
                    m_tMHashRowIdx.m_bCheckLastLayerConf = false;
                    
                    TMdbMhashLayerIndexNode* pTmpNode = (TMdbMhashLayerIndexNode*)GetAddrByMhashIndexNodeId(m_tMHashRowIdx.pLayerIndex->iHeadBlockId,
                            m_tMHashRowIdx.iLayerIndexPos,iLayerNodeSize,false );
                    CHECK_OBJ(pTmpNode);
                    
                    int LayerPos = TMdbMhashAlgo::CalcLayerPos(m_tMHashRowIdx.m_iHashValue);
                    m_tNextRowIDData = pTmpNode->m_atNode[LayerPos].m_tRowId;

                   
                    
                    if(pTmpNode->m_atNode[LayerPos].m_iHashValue == m_tMHashRowIdx.m_iHashValue)
                    {
                        m_tMHashRowIdx.iConflictIndexPos = pTmpNode->m_atNode[LayerPos].m_iConfPos;
                        m_tMHashRowIdx.iLayerIndexPos = -1;
                        TADD_DETAIL("LAYER:next on conflict[%d]", m_tMHashRowIdx.iConflictIndexPos);
                    }
                    else
                    {
                        m_tMHashRowIdx.iLayerIndexPos = pTmpNode->m_atNode[LayerPos].m_iLayerPos;
                        m_tMHashRowIdx.iConflictIndexPos = -1;
                        TADD_DETAIL("LAYER:next on layer[%d]", m_tMHashRowIdx.iLayerIndexPos);
                    }
                }
            }
            if(NULL != m_pDataAddr)
            {//获取到数据
                TADD_FUNC("Finish[true].");
                return true;
            }
        }while(1);
        TADD_FUNC("Finish[false],out of while.");
        return false;
    }

	int TMdbTableWalker::WalkByTrieIndex(ST_TABLE_INDEX_INFO* pIndexInfo)
    {
        int iRet = 0;
        TADD_DETAIL("walk by trie index.");

        m_tCurRowIDData.Clear();

		m_pTrieRootNode = pIndexInfo->m_TrieIndexInfo.pRootNode;
		m_pTrieConfIndex = pIndexInfo->m_TrieIndexInfo.pConflictIndex;
		m_pTrieBranchIndex = pIndexInfo->m_TrieIndexInfo.pBranchIndex;

		m_bStopScanTrie = false;
        return iRet;
    }

	bool TMdbTableWalker::NextByTrie()
    {
        TADD_FUNC("Start.");
        m_pDataAddr = NULL;//初始化为NULL

		if(m_bStopScanTrie == true)	
		{
			StopWalk();
			return false;
		}
	
		TMdbTrieIndexNode* pRoot = m_pTrieRootNode;
		TMdbTrieIndexNode* pCur = pRoot;
		TMdbTrieIndexNode* pChild = NULL;

		int cPos = 0;
		int iChrIndex = 0;
		int iChildPos = -1;

		if (m_sTrieWord[0]== 0) return false;


		//到树枝上找数据
		if(m_iNextIndexPos < 0)
		{
		    for(;;++cPos)
		    {
		    	CHECK_OBJ(pCur);

				//没到结尾符，存在一下个孩子节点
				if(m_sTrieWord[cPos])
				{
			    	iChrIndex = m_sTrieWord[cPos] - '0';
					iChildPos = pCur->m_iChildrenPos[iChrIndex];
				}

				//没有数据
				if( (pCur == pRoot)&&(-1==iChildPos) )
				{
					StopWalk();
					return false;
				}

				//没有孩子节点了
				if( ((-1 == iChildPos) || (m_sTrieWord[cPos]==0)) && (pCur != pRoot))
				{
					m_tCurRowIDData = pCur->m_NodeInfo.m_tRowId;
					m_iNextIndexPos = pCur->m_NodeInfo.m_iNextConfPos;
					if(!m_tCurRowIDData.IsEmpty())
					{
						m_pDataAddr = GetAddressRowID(&m_tCurRowIDData, m_iDataSize, true);
						if(m_iNextIndexPos < 0)
						{
							//停止遍历
							m_bStopScanTrie = true;
						}						
						return true;
					}
					break;
				}
								
				pChild = (TMdbTrieIndexNode*)GetAddrByTrieIndexNodeId(m_pTrieBranchIndex->iHeadBlockId, iChildPos ,sizeof(TMdbTrieIndexNode),false);				
				CHECK_OBJ(pChild);
				pCur = pChild;
			}
		}


		//到冲突链上找数据
		if(m_iNextIndexPos > 0)
		{
			TMdbTrieConfIndexNode* pConflictNode = (TMdbTrieConfIndexNode*)GetAddrByTrieIndexNodeId(m_pTrieConfIndex->iHeadBlockId, m_iNextIndexPos ,sizeof(TMdbTrieConfIndexNode),true);
			CHECK_OBJ(pConflictNode);
			m_tCurRowIDData = pConflictNode->m_NodeInfo.m_tRowId;
			m_iNextIndexPos = pConflictNode->m_NodeInfo.m_iNextConfPos;
			if(!m_tCurRowIDData.IsEmpty())
			{
				m_pDataAddr = GetAddressRowID(&m_tCurRowIDData, m_iDataSize, true);
			}
		}

		
		//冲突链结束			
		if(m_iNextIndexPos < 0)
		{
			//停止遍历
			m_bStopScanTrie = true;

		}
		return true;	
    }

	TMdbMhashBlock* TMdbTableWalker::GetMhashBlockById(int iBlockID, bool bConf)
    {
        if(bConf)
        {
            return m_pShmDSN->GetMhashConfBlockById( iBlockID);
        }
        else
        {
            return m_pShmDSN->GetMhashLayerBlockById( iBlockID);
        }
    }	


    TMdbTrieBlock* TMdbTableWalker::GetTrieBlockById(int iBlockID, bool bConf)
    {
        if(bConf)
        {
            return m_pShmDSN->GetTrieConfBlockById( iBlockID);
        }
        else
        {
            return m_pShmDSN->GetTrieBranchBlockById( iBlockID);
        }
    }	
	
	char* TMdbTableWalker::GetAddrByMhashIndexNodeId(int iHeadBlock,int iIndexNodeId,int iNodeSize, bool bConf)
	{
		TADD_FUNC("Start.");
		char* pAddr = NULL;

		TMdbMhashBlock* pHeadBlock = GetMhashBlockById(iHeadBlock, bConf);
		if(pHeadBlock == NULL)
		{
			TADD_ERROR(ERR_APP_INVALID_PARAM," index head block is invalid, head blockid[%d]",iHeadBlock);
			return NULL;
		}
		
		if(iIndexNodeId > 0)
		{
			TMdbMhashBlock* pTmpBlock = pHeadBlock;
			while(true)
			{
				
				if(iIndexNodeId >= pTmpBlock->iStartNode && iIndexNodeId <= pTmpBlock->iEndNode)
				{
					//计算出相应的地址
					 if(bConf)
					{
						pAddr = m_pShmDSN->GetMhashConfShmAddr(pTmpBlock->iShmID);
					}
					else
					{
						pAddr = m_pShmDSN->GetMhashLayerShmAddr(pTmpBlock->iShmID);
					}

					if(NULL == pAddr)
					{
						TADD_ERROR(ERR_APP_INVALID_PARAM,"Can't get block (iShmID=%d)", pTmpBlock->iShmID);
						return NULL;
					}
					pAddr +=  (iIndexNodeId-pTmpBlock->iStartNode)*iNodeSize;
					break;
				}
				else
				{
					if(pTmpBlock->iNextBlock < 0)
					{
						TADD_ERROR(ERROR_UNKNOWN,"Can't find Node=%d", iIndexNodeId);
						return NULL;
					}
					else
					{
						pTmpBlock = GetMhashBlockById(pTmpBlock->iNextBlock,bConf);
					}
				}
			}
		}
		else
		{
			return NULL;
		}
		return pAddr;
	}

	 char* TMdbTableWalker::GetAddrByTrieIndexNodeId(int iHeadBlock,int iIndexNodeId, int iNodeSize, bool bConf)
    {
        TADD_FUNC("Start.");
        TADD_DETAIL("iHeadBlock=%d,iIndexNodeId=%d,iNodeSize=%d, bConf=[%s]"
            ,iHeadBlock,iIndexNodeId, iNodeSize, bConf?"TRUE":"FALSE");
        
        char* pAddr = NULL;

        TMdbTrieBlock* pHeadBlock = GetTrieBlockById(iHeadBlock, bConf);
        if(pHeadBlock == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM," index head block is invalid, head blockid[%d]",iHeadBlock);
            return NULL;
        }
        
        if(iIndexNodeId > 0)
        {
            TMdbTrieBlock* pTmpBlock = pHeadBlock;
            while(pTmpBlock!=NULL)
            {
                TADD_DETAIL("iStartNode=[%d],iEndNode=[%d],iBlockId=[%d],iNextBlock=[%d]",pTmpBlock->iStartNode, pTmpBlock->iEndNode, pTmpBlock->iBlockId,pTmpBlock->iNextBlock);
                if(iIndexNodeId >= pTmpBlock->iStartNode && iIndexNodeId <= pTmpBlock->iEndNode)
                {
                    //计算出相应的地址
                    
                    if(bConf)
                    {
                        pAddr = m_pShmDSN->GetTrieConfShmAddr(pTmpBlock->iShmID);
                    }
                    else
                    {
                        pAddr = m_pShmDSN->GetTrieBranchShmAddr(pTmpBlock->iShmID);
                    }

                    if(NULL == pAddr)
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"Can't get block (iShmID=%d)", pTmpBlock->iShmID);
                        return NULL;
                    }
                    pAddr +=  (iIndexNodeId-pTmpBlock->iStartNode)*iNodeSize;
                    
                    break;
                }
                else
                {
                    if(pTmpBlock->iNextBlock < 0)
                    {
                        TADD_ERROR(ERROR_UNKNOWN,"Can't find Node=%d", iIndexNodeId);
                        return NULL;
                    }
                    else
                    {
                        pTmpBlock = GetTrieBlockById(pTmpBlock->iNextBlock,bConf);
                    }
                }
            }
        }
        else
        {
            return NULL;
        }
        return pAddr;
    }

	
//}

