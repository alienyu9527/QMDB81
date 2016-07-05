/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbIndexCtrl.cpp
*@Description： MDB的索引控制模块
*@Author:	 	jin.shaohua
*@Date：	    2012.04
*@History:
******************************************************************************************/
#include "Control/mdbIndexCtrl.h"
#include "Helper/mdbDateTime.h"
#include <map>
//#include "BillingSDK.h"
#include "Control/mdbRowCtrl.h"
#include "Control/mdbLinkCtrl.h"
//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{

    TMdbIndexCtrl::TMdbIndexCtrl()
    {
    }

    TMdbIndexCtrl::~TMdbIndexCtrl()
    {
    }

    int TMdbIndexCtrl::CreateAllIndex(TMdbConfig &config)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_pMdbShmDsn = TMdbShmMgr::GetShmDSN(config.GetDSN()->sName);
        m_pMdbDsn = m_pMdbShmDsn->GetInfo();
        
        CHECK_OBJ(m_pMdbShmDsn);
        CHECK_OBJ(m_pMdbDsn);

        CHECK_RET(m_tHashIndex.AttachDsn(m_pMdbShmDsn),"hashctrl attach failed.");
        CHECK_RET(m_tMHashIndex.AttachDsn(m_pMdbShmDsn),"M-hashctrl attach failed.");
        CHECK_RET(m_tTrieIndex.AttachDsn(m_pMdbShmDsn),"Triectrl attach failed.");

		//hash
        m_pMdbDsn->iBaseIndexShmCounts = 0;
        m_pMdbDsn->iConflictIndexShmCounts = 0;

		//mhash
        m_pMdbDsn->iMHashBaseIdxShmCnt = 0;
        m_pMdbDsn->iMHashConfIdxShmCnt = 0;
        m_pMdbDsn->iMHashLayerIdxShmCnt = 0;

		//trie
		m_pMdbDsn->iTrieRootIdxShmCnt = 0;
		m_pMdbDsn->iTrieBranchIdxShmCnt = 0;
        m_pMdbDsn->iTrieConfIdxShmCnt = 0;

        
        int i = 0;
        TMdbTable * pTable = NULL;
        TMdbTable tTempTable;
        for( i = 0; i<MAX_TABLE_COUNTS; i++)
        {
            //遍历所有table
            pTable  = config.GetTableByPos(i);
            if(NULL != pTable)
            {
                memcpy(&tTempTable,pTable,sizeof(tTempTable));
                tTempTable.ResetRecordCounts();//加载到内存中时记录数发生了改变
                CHECK_RET(AddTableIndex(&tTempTable,config.GetDSN()->iDataSize),"AddTableIndex failed...");
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbIndexCtrl::AddTableIndex(TMdbTable * pTable,size_t iDataSize)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        TADD_FLOW("AddTableIndex Table=[%s],Size=[%lu] start.", pTable->sTableName,iDataSize);
        for(int i=0; i<pTable->iIndexCounts; ++i)
        {
            if(pTable->tIndex[i].sName[0] == 0)
            {
                continue;
            }

            TADD_FLOW("Index[%s],algoType=[%d]",pTable->tIndex[i].sName,pTable->tIndex[i].m_iAlgoType);            
            if(pTable->tIndex[i].m_iAlgoType == INDEX_HASH)
            {
                iRet = m_tHashIndex.AddTableSingleIndex(pTable, i, iDataSize);
            }
            else if(pTable->tIndex[i].m_iAlgoType == INDEX_M_HASH)
            {
                iRet = m_tMHashIndex.AddTableSingleIndex(pTable,  i,iDataSize);
            }
			else if(pTable->tIndex[i].m_iAlgoType == INDEX_TRIE)
            {
                iRet = m_tTrieIndex.AddTableSingleIndex(pTable,  i,iDataSize);
            }
            else
            {
                TADD_ERROR(-1, "Unknown index algo type .not add this index[%s]", pTable->tIndex[i].sName);
                return ERR_TAB_INDEX_INVALID_TYPE;
            }

            if(iRet != 0)
            {
                TADD_ERROR(-1, "add index[%s] failed", pTable->tIndex[i].sName);
                return ERR_TAB_ADD_INDEX_FAILED;
            }
            
        }
        
        TADD_FLOW("AddTableIndex : Table=[%s] finish.", pTable->sTableName);
        return iRet;
    }

    int TMdbIndexCtrl::DeleteTableIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable)
    {
        int iRet = 0;
        CHECK_OBJ(pMdbShmDsn);
        CHECK_OBJ(pTable);
        TADD_FUNC("Start.table=[%s].",pTable->sTableName);
        CHECK_RET(AttachDsn(pMdbShmDsn),"Attach dsn failed...");
        CHECK_RET(AttachTable(pMdbShmDsn,pTable),"Attach table[%s] failed....",pTable->sTableName);//先Attach上表获取索引信息
        int i = 0;
        for(i = 0; i<pTable->iIndexCounts; i++)
        {
            if(m_arrTableIndex[i].bInit)
            {
                if(pTable->tIndex[i].m_iAlgoType == INDEX_HASH)
                {
                    iRet = m_tHashIndex.DeleteTableIndex(m_arrTableIndex[i].m_HashIndexInfo);
                }
                else if(pTable->tIndex[i].m_iAlgoType == INDEX_M_HASH)
                {
                    iRet = m_tMHashIndex.DeleteTableIndex(m_arrTableIndex[i].m_MHashIndexInfo);
                }
				else if(pTable->tIndex[i].m_iAlgoType == INDEX_TRIE)
                {
                    iRet = m_tTrieIndex.DeleteTableIndex(m_arrTableIndex[i].m_TrieIndexInfo);
                }
				
                else
                {
                    TADD_ERROR(-1, "Unknown index algo type .not add this index[%s]", pTable->tIndex[i].sName);
                    return ERR_TAB_INDEX_INVALID_TYPE;
                }
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbIndexCtrl::TruncateTableIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pMdbShmDsn);
        CHECK_OBJ(pTable);
        TADD_FUNC("Start.table=[%s].",pTable->sTableName);
        CHECK_RET(AttachDsn(pMdbShmDsn),"Attach dsn failed...");
        CHECK_RET(AttachTable(pMdbShmDsn,pTable),"Attach table[%s] failed....",pTable->sTableName);//先Attach上表获取索引信息
        		
		for(int i = 0; i < pTable->iIndexCounts; i++)
		{
			if(pTable->tIndex[i].m_iAlgoType == INDEX_HASH)
            {
				iRet = m_tHashIndex.TruncateTableIndex(m_arrTableIndex[i].m_HashIndexInfo);
            }
            else if(pTable->tIndex[i].m_iAlgoType == INDEX_M_HASH)
            {
            	iRet = m_tMHashIndex.TruncateTableIndex(m_arrTableIndex[i].m_MHashIndexInfo);		        
            }
			else if(pTable->tIndex[i].m_iAlgoType == INDEX_TRIE)
            {
            	iRet = m_tTrieIndex.TruncateTableIndex(m_arrTableIndex[i].m_TrieIndexInfo);		        
            }
            else
            {
                TADD_ERROR(-1, "Unknown index algo type .not add this index[%s]", pTable->tIndex[pTable->iIndexCounts].sName);
                return ERR_TAB_INDEX_INVALID_TYPE;
            }				
		}
        TADD_FUNC("Finish.");
        return iRet;
    }
	
    int TMdbIndexCtrl::AddTableSingleIndex(TMdbTable * pTable,size_t iDataSize)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        TADD_FLOW("AddTableIndex Table=[%s],Size=[%lu] start.", pTable->sTableName,iDataSize);
            

            TADD_FLOW("Index[%s]",pTable->tIndex[pTable->iIndexCounts].sName);            
            if(pTable->tIndex[pTable->iIndexCounts].m_iAlgoType == INDEX_HASH)
            {
                iRet = m_tHashIndex.AddTableSingleIndex(pTable, pTable->iIndexCounts, iDataSize);
            }
            else if(pTable->tIndex[pTable->iIndexCounts].m_iAlgoType == INDEX_M_HASH)
            {
                iRet = m_tMHashIndex.AddTableSingleIndex(pTable, pTable->iIndexCounts, iDataSize);
            }
			else if(pTable->tIndex[pTable->iIndexCounts].m_iAlgoType == INDEX_TRIE)
            {
                iRet = m_tTrieIndex.AddTableSingleIndex(pTable, pTable->iIndexCounts, iDataSize);
            }
			
            else
            {
                TADD_ERROR(-1, "Unknown index algo type .not add this index[%s]", pTable->tIndex[pTable->iIndexCounts].sName);
                return ERR_TAB_INDEX_INVALID_TYPE;
            }

            if(iRet != 0)
            {
                TADD_ERROR(-1, "add index[%s] failed", pTable->tIndex[pTable->iIndexCounts].sName);
                return ERR_TAB_ADD_INDEX_FAILED;
            }
            
        
        TADD_FLOW("AddTableIndex : [%s] finish.", pTable->tIndex[pTable->iIndexCounts -1].sName);
        return iRet;
    }

    ST_TABLE_INDEX_INFO * TMdbIndexCtrl::GetIndexByColumnPos(int iColumnPos,int &iColNoPos)
    {
        TADD_FUNC("Start.iColumnPos=[%d].",iColumnPos);
        int i = 0;
        for(i = 0; i < m_pAttachTable->iIndexCounts; i++)
        {
            if(m_arrTableIndex[i].bInit)
            {
                TMdbIndex * pMdbIndex = m_arrTableIndex[i].pIndexInfo;
                int j = 0;
                for(j = 0; j<MAX_INDEX_COLUMN_COUNTS; j++)
                {
                    if(iColumnPos == pMdbIndex->iColumnNo[j])
                    {
                        iColNoPos = j;
                        TADD_FUNC("Finish.iColNoPos=[%d]",iColNoPos);
                        return &m_arrTableIndex[i];
                    }
                }
            }
        }
        return NULL;
    }

	ST_TABLE_INDEX_INFO * TMdbIndexCtrl::GetIndexByName(const char* sName)
    {
        int i = 0;
        for(i = 0; i < m_pAttachTable->iIndexCounts; i++)
        {
            if(m_arrTableIndex[i].bInit)
            {
                if(0 == strncasecmp(sName,m_arrTableIndex[i].pIndexInfo->sName,MAX_NAME_LEN))
                {
                    return &m_arrTableIndex[i];
                }
            }
        }
        return NULL;
	}
    

ST_TABLE_INDEX_INFO * TMdbIndexCtrl::GetAllIndexByColumnPos(int iColumnPos,int &iColNoPos,int &iCurIndexPos)
{
    TADD_FUNC("Start.iColumnPos=[%d].",iColumnPos);
    int i = iCurIndexPos+1;
    for(; i < m_pAttachTable->iIndexCounts; i++)
    {
        if(m_arrTableIndex[i].bInit)
        {
            TMdbIndex * pMdbIndex = m_arrTableIndex[i].pIndexInfo;
            int j = 0;
            for(j = 0; j<MAX_INDEX_COLUMN_COUNTS; j++)
            {
                if(iColumnPos == pMdbIndex->iColumnNo[j])
                {
                    iColNoPos = j;
                    iCurIndexPos = i;
                    TADD_FUNC("Finish.iColNoPos=[%d]",iColNoPos);
                    return &m_arrTableIndex[i];
                }
            }
        }
    }
    return NULL;
}


int TMdbIndexCtrl::ReAttachSingleIndex(int iIndexPos)
{
	int iRet = 0;
	CHECK_OBJ(m_pAttachTable);		
	TADD_DETAIL("ReAttachSingleIndex:tIndex[%d].sName=[%s].", iIndexPos, m_pAttachTable->tIndex[iIndexPos].sName); 

	int iAlgoType = m_pAttachTable->tIndex[iIndexPos].m_iAlgoType;

	switch(iAlgoType)
	{
		case INDEX_HASH:
			CHECK_RET(ReAttachSingleHashIndex(iIndexPos),"ReAttachSingleHashIndex failed.");
			break;
		case INDEX_M_HASH:
			CHECK_RET(ReAttachSingleMHashIndex(iIndexPos),"ReAttachSingleMHashIndex failed.");
			break;
		default:
			TADD_ERROR(-1,"ReAttachSingleIndex invalid AlgoType %d.",iAlgoType);
			break;
	}
	return iRet;
}


int TMdbIndexCtrl::ReAttachSingleHashIndex(int iIndexPos)
{
	int iRet = 0;
	CHECK_OBJ(m_pMdbShmDsn);
	for(int n=0; n<MAX_SHM_ID; ++n)
    {
        TADD_DETAIL("Attach (%s) hash Index : Shm-ID no.[%d].", m_pAttachTable->sTableName, n);
        char * pBaseIndexAddr = m_pMdbShmDsn->GetBaseIndex(n);
        if(pBaseIndexAddr == NULL)
            continue;

        TMdbBaseIndexMgrInfo *pBIndexMgr = (TMdbBaseIndexMgrInfo*)pBaseIndexAddr;//获取基础索引内容

		int i = iIndexPos;
        TADD_DETAIL("HASH:tIndex[%d].sName=[%s].", i, m_pAttachTable->tIndex[i].sName);
        for(int j=0; j<MAX_BASE_INDEX_COUNTS; ++j)
        {
            //比较索引名称,如果相同，则把锁地址和索引地址记录下来
            if((0 == TMdbNtcStrFunc::StrNoCaseCmp( m_pAttachTable->sTableName, pBIndexMgr->tIndex[j].sTabName))
                &&(TMdbNtcStrFunc::StrNoCaseCmp(m_pAttachTable->tIndex[i].sName, pBIndexMgr->tIndex[j].sName) == 0))
            {
                TADD_DETAIL("Find index[%s]",m_pAttachTable->tIndex[i].sName);
                //找到索引
                char * pConflictIndexAddr = m_pMdbShmDsn->GetConflictIndex(pBIndexMgr->tIndex[j].iConflictMgrPos);
                TMdbConflictIndexMgrInfo *pCIndexMgr  = (TMdbConflictIndexMgrInfo *)pConflictIndexAddr;
                CHECK_OBJ(pCIndexMgr);

                m_arrTableIndex[i].bInit  = true;
                m_arrTableIndex[i].iIndexPos  = i;
                m_arrTableIndex[i].pIndexInfo = &(m_pAttachTable->tIndex[i]);//保存index的信息
                
                m_arrTableIndex[i].m_HashIndexInfo.pBIndexMgr = pBIndexMgr;
                m_arrTableIndex[i].m_HashIndexInfo.pCIndexMgr = pCIndexMgr;
                m_arrTableIndex[i].m_HashIndexInfo.iBaseIndexPos = j;
                m_arrTableIndex[i].m_HashIndexInfo.iConflictIndexPos = pBIndexMgr->tIndex[j].iConflictIndexPos;
                m_arrTableIndex[i].m_HashIndexInfo.pBaseIndex = &(pBIndexMgr->tIndex[j]);
                m_arrTableIndex[i].m_HashIndexInfo.pConflictIndex = &(pCIndexMgr->tIndex[pBIndexMgr->tIndex[j].iConflictIndexPos]);
                m_arrTableIndex[i].m_HashIndexInfo.pBaseIndexNode = (TMdbIndexNode*)(pBaseIndexAddr + m_arrTableIndex[i].m_HashIndexInfo.pBaseIndex->iPosAdd);
                m_arrTableIndex[i].m_HashIndexInfo.pConflictIndexNode = (TMdbIndexNode*)(pConflictIndexAddr + m_arrTableIndex[i].m_HashIndexInfo.pConflictIndex->iPosAdd);
                
                break;
            }
        }
		
    }

	return iRet;
}

int TMdbIndexCtrl::ReAttachSingleMHashIndex(int iIndexPos)
{
	int iRet = 0;
	CHECK_OBJ(m_pMdbShmDsn);
	for(int n=0; n<MAX_SHM_ID; ++n)
	{
		TADD_DETAIL("Attach (%s) m-hash Index : Shm-ID no.[%d].", m_pAttachTable->sTableName, n);
		char * pBaseIndexAddr = m_pMdbShmDsn->GetMHashBaseIndex(n);
		if(pBaseIndexAddr == NULL)
			continue;

		TMdbMHashBaseIndexMgrInfo *pMHashBIndexMgr = (TMdbMHashBaseIndexMgrInfo*)pBaseIndexAddr;//获取基础索引内容
		TMdbMHashConflictIndexMgrInfo* pMHashConfMgr = m_pMdbShmDsn->GetMHashConfMgr();
		CHECK_OBJ(pMHashConfMgr);
		TMdbMHashLayerIndexMgrInfo* pMHashLayerMgr = m_pMdbShmDsn->GetMHashLayerMgr();
		CHECK_OBJ(pMHashLayerMgr);
		
		int i = iIndexPos;
		TADD_DETAIL("MHASH , tIndex[%d].sName=[%s].", i, m_pAttachTable->tIndex[i].sName);
		for(int j=0; j<MAX_MHASH_INDEX_COUNT; ++j)
		{
			//比较索引名称,如果相同，则把锁地址和索引地址记录下来
			if((0 == TMdbNtcStrFunc::StrNoCaseCmp(m_pAttachTable->sTableName,pMHashBIndexMgr->tIndex[j].sTabName))
				&&(TMdbNtcStrFunc::StrNoCaseCmp(m_pAttachTable->tIndex[i].sName, pMHashBIndexMgr->tIndex[j].sName) == 0))
			{
				TADD_DETAIL("Find index[%s]",m_pAttachTable->tIndex[i].sName);
				//找到索引

				char* pMutexAddr = m_pMdbShmDsn->GetMHashMutex(pMHashBIndexMgr->tIndex[j].iMutexMgrPos);
				TMdbMHashMutexMgrInfo*pMutexMgr  = (TMdbMHashMutexMgrInfo *)pMutexAddr;
				CHECK_OBJ(pMutexMgr);

				m_arrTableIndex[i].bInit  = true;
				m_arrTableIndex[i].iIndexPos  = i;
				m_arrTableIndex[i].pIndexInfo = &(m_pAttachTable->tIndex[i]);//保存index的信息

				m_arrTableIndex[i].m_MHashIndexInfo.pBIndexMgr = pMHashBIndexMgr;
				m_arrTableIndex[i].m_MHashIndexInfo.pMutexMgr = pMutexMgr;
				
				m_arrTableIndex[i].m_MHashIndexInfo.iBaseIndexPos = j;
				m_arrTableIndex[i].m_MHashIndexInfo.iMutexPos = pMHashBIndexMgr->tIndex[j].iMutexPos;
				
				m_arrTableIndex[i].m_MHashIndexInfo.pBaseIndex = &(pMHashBIndexMgr->tIndex[j]);
				
				m_arrTableIndex[i].m_MHashIndexInfo.pConflictIndex = &(pMHashConfMgr->tIndex[pMHashBIndexMgr->tIndex[j].iConflictIndexPos]);
				m_arrTableIndex[i].m_MHashIndexInfo.pLayerIndex = &(pMHashLayerMgr->tIndex[pMHashBIndexMgr->tIndex[j].iLayerIndexPos]);  
				m_arrTableIndex[i].m_MHashIndexInfo.pMutex = &(pMutexMgr->aBaseMutex[pMHashBIndexMgr->tIndex[j].iMutexPos]);
				
				m_arrTableIndex[i].m_MHashIndexInfo.pBaseIndexNode = (TMdbMHashBaseIndexNode*)(pBaseIndexAddr + m_arrTableIndex[i].m_MHashIndexInfo.pBaseIndex->iPosAdd);
				m_arrTableIndex[i].m_MHashIndexInfo.pMutexNode = (TMutex*)(pMutexAddr+ m_arrTableIndex[i].m_MHashIndexInfo.pMutex->iPosAdd);
				
				break;
			}
		}
	}	
	return iRet;
}


//为链接填充索引信息
int TMdbIndexCtrl::SetLinkInfo(TMdbLocalLink* pLink)
{
	int iRet = 0;
	int iPos = -1;
	for(int i=0; i<MAX_TABLE_COUNTS; i++)
	{
		if(0 == strcasecmp(pLink->m_AllTableIndexInfo[i].sTableName, m_pAttachTable->sTableName))
		{
			iPos = i;
			break;
		}
	}

	if(-1 == iPos)
	{	
		//找不到的情况，添加一个
		for(int i=0; i<MAX_TABLE_COUNTS; i++)
		{
			if(0 == pLink->m_AllTableIndexInfo[i].sTableName[0])
			{
				iPos = i;
				break;
			}
		}

		if( -1 == iPos)
		{
			TADD_ERROR(ERR_TAB_TABLE_NUM_EXCEED_MAX,"Too Many Table Operating On One Link.");
			return ERR_TAB_TABLE_NUM_EXCEED_MAX;
		}

		//begin  拷贝索引部分信息
		strncpy(pLink->m_AllTableIndexInfo[iPos].sTableName, m_pAttachTable->sTableName, MAX_NAME_LEN);
		for(int j=0; j<MAX_INDEX_COUNTS; j++)
		{
			if(m_arrTableIndex[j].bInit)
			{
				pLink->m_AllTableIndexInfo[iPos].arrLinkIndex[j].bInit = true;
				strncpy(pLink->m_AllTableIndexInfo[iPos].arrLinkIndex[j].sName, m_arrTableIndex[j].pIndexInfo->sName, MAX_NAME_LEN);
				pLink->m_AllTableIndexInfo[iPos].arrLinkIndex[j].iAlgoType = m_arrTableIndex[j].pIndexInfo->m_iAlgoType;
			}
			else
			{
				pLink->m_AllTableIndexInfo[iPos].arrLinkIndex[j].bInit = false;
				continue;
			}
		}
		//end 
		
	}

	m_pLink = pLink;
	m_tHashIndex.SetLink(pLink);
	m_tMHashIndex.SetLink(pLink);
	m_tTrieIndex.SetLink(pLink);
	return iRet;
}

int TMdbIndexCtrl::AttachTable(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable)
    {
        int iRet = 0;
        TADD_FUNC("AttachTable(%s) : Start.", pTable->sTableName);
        int iFindIndexs = 0;
        m_pAttachTable = pTable;
        CleanTableIndexInfo();//清理

        CHECK_RET(m_tHashIndex.AttachTable(pMdbShmDsn, pTable),"hash index attach table failed.");
        CHECK_RET(m_tMHashIndex.AttachTable(pMdbShmDsn, pTable),"M-hash index attach table failed.");
        CHECK_RET(m_tTrieIndex.AttachTable(pMdbShmDsn, pTable),"Trie index attach table failed.");

		AttachHashIndex(pMdbShmDsn, pTable, iFindIndexs);
		AttachMHashIndex(pMdbShmDsn, pTable, iFindIndexs);
		AttachTrieIndex(pMdbShmDsn, pTable, iFindIndexs);
		              
        if(iFindIndexs < pTable->iIndexCounts)
        {
            TADD_ERROR(-1,"AttachTable(%s) : iFindIndexs[%d] < pTable->iIndexCounts[%d].",pTable->sTableName,iFindIndexs, pTable->iIndexCounts);
            return ERR_TAB_INDEX_NUM_INVALID;
        }
        TADD_FUNC("AttachTable(%s) : Finish.", pTable->sTableName);
        return iRet;
    }

    int  TMdbIndexCtrl::AttachHashIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,int& iFindIndexs)
    {
        for(int n=0; n<MAX_SHM_ID; ++n)
        {
            TADD_DETAIL("Attach (%s) hash Index : Shm-ID no.[%d].", pTable->sTableName, n);
            char * pBaseIndexAddr = pMdbShmDsn->GetBaseIndex(n);
            if(pBaseIndexAddr == NULL)
                continue;

            TMdbBaseIndexMgrInfo *pBIndexMgr = (TMdbBaseIndexMgrInfo*)pBaseIndexAddr;//获取基础索引内容
            for(int i=0; i<pTable->iIndexCounts; ++i)
            {
                TADD_DETAIL("HASH:tIndex[%d].sName=[%s].", i, pTable->tIndex[i].sName);
                for(int j=0; j<MAX_BASE_INDEX_COUNTS; ++j)
                {
                    //比较索引名称,如果相同，则把锁地址和索引地址记录下来
                    if((0 == TMdbNtcStrFunc::StrNoCaseCmp( pTable->sTableName, pBIndexMgr->tIndex[j].sTabName))
                        &&(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tIndex[i].sName, pBIndexMgr->tIndex[j].sName) == 0))
                    {
                        TADD_DETAIL("Find index[%s]",pTable->tIndex[i].sName);
                        //找到索引
                        char * pConflictIndexAddr = pMdbShmDsn->GetConflictIndex(pBIndexMgr->tIndex[j].iConflictMgrPos);
                        TMdbConflictIndexMgrInfo *pCIndexMgr  = (TMdbConflictIndexMgrInfo *)pConflictIndexAddr;
                        CHECK_OBJ(pCIndexMgr);

                        m_arrTableIndex[i].bInit  = true;
                        m_arrTableIndex[i].iIndexPos  = i;
                        m_arrTableIndex[i].pIndexInfo = &(pTable->tIndex[i]);//保存index的信息
                        
                        m_arrTableIndex[i].m_HashIndexInfo.pBIndexMgr = pBIndexMgr;
                        m_arrTableIndex[i].m_HashIndexInfo.pCIndexMgr = pCIndexMgr;
                        m_arrTableIndex[i].m_HashIndexInfo.iBaseIndexPos = j;
                        m_arrTableIndex[i].m_HashIndexInfo.iConflictIndexPos = pBIndexMgr->tIndex[j].iConflictIndexPos;
                        m_arrTableIndex[i].m_HashIndexInfo.pBaseIndex = &(pBIndexMgr->tIndex[j]);
                        m_arrTableIndex[i].m_HashIndexInfo.pConflictIndex = &(pCIndexMgr->tIndex[pBIndexMgr->tIndex[j].iConflictIndexPos]);
                        m_arrTableIndex[i].m_HashIndexInfo.pBaseIndexNode = (TMdbIndexNode*)(pBaseIndexAddr + m_arrTableIndex[i].m_HashIndexInfo.pBaseIndex->iPosAdd);
                        m_arrTableIndex[i].m_HashIndexInfo.pConflictIndexNode = (TMdbIndexNode*)(pConflictIndexAddr + m_arrTableIndex[i].m_HashIndexInfo.pConflictIndex->iPosAdd);
                        
                        ++iFindIndexs;
                        break;
                    }
                }
            }
            
            TADD_DETAIL("iFindIndexs=%d.pTable->iIndexCounts=%d", iFindIndexs,pTable->iIndexCounts);
            if(iFindIndexs == pTable->iIndexCounts)
            {
                break;
            }
        }
		return 0;
	}

    int  TMdbIndexCtrl::AttachMHashIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,int& iFindIndexs)
    {
        for(int n=0; n<MAX_SHM_ID; ++n)
        {
            TADD_DETAIL("Attach (%s) m-hash Index : Shm-ID no.[%d].", pTable->sTableName, n);
            char * pBaseIndexAddr = pMdbShmDsn->GetMHashBaseIndex(n);
            if(pBaseIndexAddr == NULL)
                continue;

            TMdbMHashBaseIndexMgrInfo *pMHashBIndexMgr = (TMdbMHashBaseIndexMgrInfo*)pBaseIndexAddr;//获取基础索引内容
            TMdbMHashConflictIndexMgrInfo* pMHashConfMgr = pMdbShmDsn->GetMHashConfMgr();
            CHECK_OBJ(pMHashConfMgr);
            TMdbMHashLayerIndexMgrInfo* pMHashLayerMgr = pMdbShmDsn->GetMHashLayerMgr();
            CHECK_OBJ(pMHashLayerMgr);
            for(int i=0; i<pTable->iIndexCounts; ++i)
            {
                TADD_DETAIL("MHASH , tIndex[%d].sName=[%s].", i, pTable->tIndex[i].sName);
                for(int j=0; j<MAX_MHASH_INDEX_COUNT; ++j)
                {
                    //比较索引名称,如果相同，则把锁地址和索引地址记录下来
                    if((0 == TMdbNtcStrFunc::StrNoCaseCmp(pTable->sTableName,pMHashBIndexMgr->tIndex[j].sTabName))
                        &&(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tIndex[i].sName, pMHashBIndexMgr->tIndex[j].sName) == 0))
                    {
                        TADD_DETAIL("Find index[%s]",pTable->tIndex[i].sName);
                        //找到索引

                        char* pMutexAddr = pMdbShmDsn->GetMHashMutex(pMHashBIndexMgr->tIndex[j].iMutexMgrPos);
                        TMdbMHashMutexMgrInfo*pMutexMgr  = (TMdbMHashMutexMgrInfo *)pMutexAddr;
                        CHECK_OBJ(pMutexMgr);

                        m_arrTableIndex[i].bInit  = true;
                        m_arrTableIndex[i].iIndexPos  = i;
                        m_arrTableIndex[i].pIndexInfo = &(pTable->tIndex[i]);//保存index的信息

                        m_arrTableIndex[i].m_MHashIndexInfo.pBIndexMgr = pMHashBIndexMgr;
                        m_arrTableIndex[i].m_MHashIndexInfo.pMutexMgr = pMutexMgr;
                        
                        m_arrTableIndex[i].m_MHashIndexInfo.iBaseIndexPos = j;
                        m_arrTableIndex[i].m_MHashIndexInfo.iMutexPos = pMHashBIndexMgr->tIndex[j].iMutexPos;
                        
                        m_arrTableIndex[i].m_MHashIndexInfo.pBaseIndex = &(pMHashBIndexMgr->tIndex[j]);
                        
                        m_arrTableIndex[i].m_MHashIndexInfo.pConflictIndex = &(pMHashConfMgr->tIndex[pMHashBIndexMgr->tIndex[j].iConflictIndexPos]);
                        m_arrTableIndex[i].m_MHashIndexInfo.pLayerIndex = &(pMHashLayerMgr->tIndex[pMHashBIndexMgr->tIndex[j].iLayerIndexPos]);  
                        m_arrTableIndex[i].m_MHashIndexInfo.pMutex = &(pMutexMgr->aBaseMutex[pMHashBIndexMgr->tIndex[j].iMutexPos]);
                        
                        m_arrTableIndex[i].m_MHashIndexInfo.pBaseIndexNode = (TMdbMHashBaseIndexNode*)(pBaseIndexAddr + m_arrTableIndex[i].m_MHashIndexInfo.pBaseIndex->iPosAdd);
                        m_arrTableIndex[i].m_MHashIndexInfo.pMutexNode = (TMutex*)(pMutexAddr+ m_arrTableIndex[i].m_MHashIndexInfo.pMutex->iPosAdd);
                        
                        ++iFindIndexs;
                        break;
                    }
                }
            }
            
            TADD_DETAIL("iFindIndexs=%d.pTable->iIndexCounts=%d", iFindIndexs,pTable->iIndexCounts);
            if(iFindIndexs == pTable->iIndexCounts)
            {
                break;
            }
        }
		return 0;
	}

	int  TMdbIndexCtrl::AttachTrieIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,int& iFindIndexs)
    {
        for(int n=0; n<MAX_BRIE_SHMID_COUNT; ++n)
        {
            TADD_DETAIL("Attach (%s) trie Index : Shm-ID no.[%d].", pTable->sTableName, n);
            char * pRootIndexAddr = pMdbShmDsn->GetTrieRootIndex(n);
            if(pRootIndexAddr == NULL)
                continue;

            TMdbTrieRootIndexMgrInfo *pMTrieRootIndexMgr = (TMdbTrieRootIndexMgrInfo*)pRootIndexAddr;//获取基础索引内容
            TMdbTrieConflictIndexMgrInfo* pTrieConfMgr = (TMdbTrieConflictIndexMgrInfo*)pMdbShmDsn->GetTrieConfMgr();
            TMdbTrieBranchIndexMgrInfo* pTrieBranchMgr = (TMdbTrieBranchIndexMgrInfo*)pMdbShmDsn->GetTrieBranchMgr();
            CHECK_OBJ(pTrieConfMgr);
            
            for(int i=0; i<pTable->iIndexCounts; ++i)
            {
                TADD_DETAIL("Trie , tIndex[%d].sName=[%s].", i, pTable->tIndex[i].sName);
                for(int j=0; j<MAX_BRIE_INDEX_COUNT; ++j)
                {
                    //比较索引名称,如果相同，则把索引地址记录下来
                    if((0 == TMdbNtcStrFunc::StrNoCaseCmp(pTable->sTableName,pMTrieRootIndexMgr->tIndex[j].sTabName))
                        &&(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tIndex[i].sName, pMTrieRootIndexMgr->tIndex[j].sName) == 0))
                    {
                        TADD_DETAIL("Find index[%s]",pTable->tIndex[i].sName);
                        //找到索引

                        m_arrTableIndex[i].bInit  = true;
                        m_arrTableIndex[i].iIndexPos  = i;
                        m_arrTableIndex[i].pIndexInfo = &(pTable->tIndex[i]);//保存index的信息

                        m_arrTableIndex[i].m_TrieIndexInfo.pRootIndexMgr = pMTrieRootIndexMgr;
                        m_arrTableIndex[i].m_TrieIndexInfo.iRootIndexPos = j;
                        m_arrTableIndex[i].m_TrieIndexInfo.pRootIndex = &(pMTrieRootIndexMgr->tIndex[j]);
                        m_arrTableIndex[i].m_TrieIndexInfo.pConflictIndex = &(pTrieConfMgr->tIndex[pMTrieRootIndexMgr->tIndex[j].iConflictIndexPos]);
                        m_arrTableIndex[i].m_TrieIndexInfo.pBranchIndex = &(pTrieBranchMgr->tIndex[pMTrieRootIndexMgr->tIndex[j].iTrieBranchIndexPos]);
						m_arrTableIndex[i].m_TrieIndexInfo.pRootNode = (TMdbTrieIndexNode*)(pRootIndexAddr+m_arrTableIndex[i].m_TrieIndexInfo.pRootIndex->iPosAdd);
						++iFindIndexs;
                        break;
                    }
                }
            }
            
            TADD_DETAIL("iFindIndexs=%d.pTable->iIndexCounts=%d", iFindIndexs,pTable->iIndexCounts);
            if(iFindIndexs == pTable->iIndexCounts)
            {
                break;
            }
        } 
		return 0;
	}


    

    void TMdbIndexCtrl::CleanTableIndexInfo()
    {
        int i = 0;
        for(i = 0; i<MAX_INDEX_COUNTS; i++)
        {
            m_arrTableIndex[i].Clear();//清理
        }
    }

	int TMdbIndexCtrl::FindBestIndex(ST_INDEX_COLUMN ** ppIndexColumnArr,std::map<int,int>& mapCfgToWhereClause,int iHintIndexPosInCfg)
	{	
	    int i = 0;
		int iIndexPos = -1; //最优索引位置，如果为-1就表示没有找到索引
		int iIndexColCount = -1;//最优索引列的个数
		bool bHintUsed = false;
		
		for(i = 0; i < m_pAttachTable->iIndexCounts; i++)
		{
			if(m_arrTableIndex[i].bInit)
			{
				TMdbIndex * pMdbIndex = m_arrTableIndex[i].pIndexInfo;
				int j = 0;
				bool bFind = false;
				for(j = 0; j<MAX_INDEX_COLUMN_COUNTS; j++)
				{
					int iColumPos = pMdbIndex->iColumnNo[j];
					if(iColumPos < 0){break;}//该索引检测完毕
					if(NULL == ppIndexColumnArr[iColumPos])
					{
						bFind = false;
						break;
					}
					else
					{
						bFind = true;
					}
				}
				if(bFind == true)
				{
					//如果找到了，看其索引是不是最优，最优的标准是索引列越多越好
					if(iIndexPos < 0)
					{
						iIndexPos = i;
						iIndexColCount = j;

						if(iHintIndexPosInCfg == i)
						{
							bHintUsed = true;
						}
					}
					else
					{
						//索引匹配列数匹配数量多的,优先级最高
						if(j > iIndexColCount)
						{
							iIndexColCount = j;
							iIndexPos = i;
						}
						//如果索引列数相同，优先Hint
						else if(j == iIndexColCount && iHintIndexPosInCfg == i)
						{
							iIndexPos = i;
							bHintUsed = true;
						}
						//如果索引列数相同，且没有Hint,选择where靠前的
						else if(j == iIndexColCount &&
							bHintUsed == false &&
							mapCfgToWhereClause[i] < mapCfgToWhereClause[iIndexPos])
						{
							iIndexPos = i;
						}

					}
				}
				
			}
		}		
		return iIndexPos;
	}

    int TMdbIndexCtrl::GetIndexByIndexColumn(std::vector<ST_INDEX_COLUMN> & vIndexColumn,std::vector<ST_INDEX_VALUE> & vIndexValue, ST_TABLE_INDEX_INFO* pHintIndex)
    {
        TADD_FUNC("Start.vIndexColumn.size=[%d],vIndexValue.size[%d]",vIndexColumn.size(),vIndexValue.size());
        int iRet = 0;
        ST_INDEX_COLUMN * pIndexColumnArr[MAX_COLUMN_COUNTS] = {0};
		std::map<int,int>  mapCfgToWhereClause;

        for(int i = 0;i< vIndexColumn.size();++i)
        {
            pIndexColumnArr[vIndexColumn[i].pColumn->iPos] = &(vIndexColumn[i]);
			mapCfgToWhereClause[vIndexColumn[i].pColumn->iPos] = i;
        }
        
        int iIndexPos = -1; //最优索引位置，如果为-1就表示没有找到索引
		int iHintIndexPosInCfg = pHintIndex ? pHintIndex->iIndexPos: -1;
        
		iIndexPos = FindBestIndex(pIndexColumnArr,mapCfgToWhereClause,iHintIndexPosInCfg);
		        
        if(iIndexPos >= 0)
        {//可以使用该索引
            std::vector<ST_INDEX_VALUE> vTemp;
			TADD_FLOW("GetIndex:%s",m_arrTableIndex[iIndexPos].pIndexInfo->sName);
            CHECK_RET(GenerateIndexValue(pIndexColumnArr,&(m_arrTableIndex[iIndexPos]),0,vTemp),"GenerateIndexValue failed.");
            vIndexValue.insert(vIndexValue.end(),vTemp.begin(),vTemp.end());
        }
        vIndexColumn.clear();//处理完清除
        TADD_FUNC("Start.vIndexColumn.size=[%d],vIndexValue.size[%d]",vIndexColumn.size(),vIndexValue.size());
        return iRet;
    }

    int TMdbIndexCtrl::GenerateIndexValue(ST_INDEX_COLUMN *pIndexColumnArr [] ,ST_TABLE_INDEX_INFO  * pstTableIndexInfo,
                                                                       int iCurPos,std::vector<ST_INDEX_VALUE> & vIndexValue)
    {
        int iRet = 0;
        TADD_FUNC("iCurPos = [%d],vIndexValue.size=[%d]",iCurPos,vIndexValue.size());
        if(pstTableIndexInfo->pIndexInfo->iColumnNo[iCurPos] < 0 || iCurPos >= MAX_INDEX_COLUMN_COUNTS){return iRet;}//索引列遍历完毕
        int iColPos = pstTableIndexInfo->pIndexInfo->iColumnNo[iCurPos];
        ST_INDEX_COLUMN * pstIndexColumn = pIndexColumnArr[iColPos];
        CHECK_OBJ(pstIndexColumn);
        if(0 == iCurPos)
        {//首次填充
            if(0 != vIndexValue.size()){CHECK_RET(ERR_APP_INVALID_PARAM,"vIndexValue should be 0,but size=[%d]",vIndexValue.size());}
            unsigned int i = 0;
            for(i = 0; i< pstIndexColumn->vExpr.size();++i)
            {
                ST_INDEX_VALUE tIndexValue;
                tIndexValue.pstTableIndex = pstTableIndexInfo;
                tIndexValue.pExprArr[iCurPos] = pstIndexColumn->vExpr[i];
                vIndexValue.push_back(tIndexValue);
            }
        }
        else
        {
            std::vector<ST_INDEX_VALUE> vTemp;
            vTemp.assign(vIndexValue.begin(),vIndexValue.end());
            vIndexValue.clear();
            unsigned int i = 0;
            for(i = 0;i < vTemp.size();++i)
            {
                unsigned int j = 0;
                for(j = 0; j < pstIndexColumn->vExpr.size();++ j)
                {//组合
                    ST_INDEX_VALUE & tIndexValue  =  vTemp[i];
                    tIndexValue.pstTableIndex = pstTableIndexInfo;
                    tIndexValue.pExprArr[iCurPos] = pstIndexColumn->vExpr[j];
                    vIndexValue.push_back(tIndexValue);
                }
            }
        }
        pIndexColumnArr[iColPos] = NULL;//标识已经处理过。
        return GenerateIndexValue(pIndexColumnArr,pstTableIndexInfo,iCurPos+1,vIndexValue);
    }

    ST_TABLE_INDEX_INFO * TMdbIndexCtrl::GetScanAllIndex()
    {
        if(m_arrTableIndex[0].bInit)
        {
            return &(m_arrTableIndex[0]);
        }
        else
        {
            return NULL;
        }
    }

    ST_TABLE_INDEX_INFO * TMdbIndexCtrl::GetVerfiyPKIndex()
    {
        TADD_FUNC("Start.");
        if(0 == m_pAttachTable->m_tPriKey.iColumnCounts){return NULL;}
        int arrMatch[MAX_INDEX_COUNTS] = {0};//记录各个索引与主键的匹配度
        int i = 0;
        for(i = 0; i < m_pAttachTable->iIndexCounts;++i)
        {
            if(m_arrTableIndex[i].bInit)
            {
                TMdbIndex * pMdbIndex = m_arrTableIndex[i].pIndexInfo; 
                int j = 0;
                for(j = 0;j < m_pAttachTable->m_tPriKey.iColumnCounts;++j)
                {
                    int k = 0;
                    for(k = 0;k < MAX_INDEX_COLUMN_COUNTS;++k)
                    {
                        if(m_pAttachTable->m_tPriKey.iColumnNo[j] == pMdbIndex->iColumnNo[k])
                        {//有一样的列
                            arrMatch[i]++;
                            break;
                        }
                    }
                }
            }
        }
        //挑选匹配度最高的索引,并且是部分或全部主键列，不能比主键列多
        int iPos = 0;
        for(i = 0;i < m_pAttachTable->iIndexCounts;++i)
        {
            TMdbIndex * pMdbIndex = m_arrTableIndex[i].pIndexInfo; 
            int j = 0;
            bool bFindSame = false;
            for(j = 0;j < MAX_INDEX_COLUMN_COUNTS;++j)
            {
                if(pMdbIndex->iColumnNo[j] < 0){continue;}
                int k = 0;
                bFindSame = false;
                for(k = 0;k < m_pAttachTable->m_tPriKey.iColumnCounts;++k)
                {
                    if(pMdbIndex->iColumnNo[j] >=0 && m_pAttachTable->m_tPriKey.iColumnNo[k] == pMdbIndex->iColumnNo[j])
                    {//有不一样的列
                        bFindSame = true;
                        break;
                    }
                }
                if(false == bFindSame){break;}
            }
            if(false == bFindSame){arrMatch[i] = 0;}
            if(arrMatch[iPos] < arrMatch[i])
            {
                iPos = i;
            }
        }
        TADD_FUNC("Finish.");
        return 0 == arrMatch[iPos]?NULL:&(m_arrTableIndex[iPos]); 
    }

    int TMdbIndexCtrl::InsertIndexNode(int iIndexPos,char* pAddr, TMdbRowCtrl& tRowCtrl,TMdbRowID& rowID)
    {
        TADD_FUNC("IndexPos[%d],rowID[%d|%d]",iIndexPos,rowID.GetPageID(),rowID.GetDataOffset());
        int iRet = 0;
        ST_TABLE_INDEX_INFO & stTableIndex = m_arrTableIndex[iIndexPos];
        if(false == stTableIndex.bInit)
        {
            //如果有动态创建的索引，此时需要重新attch
        	ReAttachSingleIndex(iIndexPos);
			if(false == stTableIndex.bInit)
			{
	            TADD_ERROR(-1,"stTableIndex is not init....");
	            return -1;
			}
        }
        int iError = 0;
        if(stTableIndex.pIndexInfo->m_iAlgoType == INDEX_HASH)
        {
            TMdbRowIndex tRowIndex;
            tRowIndex.Clear();
            
            tRowIndex.iBaseIndexPos = CalcIndexValue(tRowCtrl, pAddr, stTableIndex.pIndexInfo, iError);
            if(0 != iError)
            {
                TADD_ERROR(iError,"CalcIndexValue(iError=%d) failed.",iError);
                return -1;
            }
            
            iRet = m_tHashIndex.InsertIndexNode(iIndexPos,tRowIndex,stTableIndex.m_HashIndexInfo, rowID);
        }
        else if(stTableIndex.pIndexInfo->m_iAlgoType == INDEX_M_HASH)
        {
            long long iHashValue = CalcIndexValue(tRowCtrl, pAddr, stTableIndex.pIndexInfo, iError);
            if(0 != iError)
            {
                TADD_ERROR(iError,"CalcIndexValue(iError=%d) failed.",iError);
                return -1;
            }
            iRet = m_tMHashIndex.InsertIndexNode(iHashValue, stTableIndex.m_MHashIndexInfo, rowID);
        }
		else if(stTableIndex.pIndexInfo->m_iAlgoType == INDEX_TRIE)
        {
        	char sTrieWord[MAX_TRIE_WORD_LEN] = {0};
        	CHECK_RET(GetTrieWord(tRowCtrl, pAddr, stTableIndex.pIndexInfo, sTrieWord),"GetTrieWord failed.");
            iRet = m_tTrieIndex.InsertIndexNode(sTrieWord,stTableIndex.m_TrieIndexInfo, rowID);
        }
        
        return iRet;
    }


    int TMdbIndexCtrl::DeleteIndexNode(int iIndexPos,char* pAddr,TMdbRowCtrl& tRowCtrl, TMdbRowID& rowID)
    {
        TADD_FUNC("IndexPos[%d],rowID[%d|%d]",iIndexPos,rowID.GetPageID(),rowID.GetDataOffset());
        int iRet = 0;
        ST_TABLE_INDEX_INFO & stTableIndex = m_arrTableIndex[iIndexPos];
        if(false == stTableIndex.bInit)
        {
        	//如果有动态创建的索引，此时需要重新attch
        	ReAttachSingleIndex(iIndexPos);
			if(false == stTableIndex.bInit)
			{
	            TADD_ERROR(-1,"stTableIndex is not init....");
	            return -1;
			}
        }

        int iError = 0;
        
        if(stTableIndex.pIndexInfo->m_iAlgoType == INDEX_HASH)
        {
            TMdbRowIndex tRowIndex;
            tRowIndex.Clear();
            
            tRowIndex.iBaseIndexPos = CalcIndexValue(tRowCtrl, pAddr, stTableIndex.pIndexInfo, iError);
            if(0 != iError)
            {
                TADD_ERROR(iError,"CalcIndexValue(iError=%d) failed.",iError);
                return -1;
            }
            iRet = m_tHashIndex.DeleteIndexNode(tRowIndex, stTableIndex.m_HashIndexInfo,rowID);
        }
        else if(stTableIndex.pIndexInfo->m_iAlgoType == INDEX_M_HASH)
        {
            long long iHashValue = CalcIndexValue(tRowCtrl, pAddr, stTableIndex.pIndexInfo, iError);
            if(0 != iError)
            {
                TADD_ERROR(iError,"CalcIndexValue(iError=%d) failed.",iError);
                return -1;
            }
            iRet = m_tMHashIndex.DeleteIndexNode(iHashValue, stTableIndex.m_MHashIndexInfo, rowID);
        }
		else if(stTableIndex.pIndexInfo->m_iAlgoType == INDEX_TRIE)
        {
			char sTrieWord[MAX_TRIE_WORD_LEN] = {0};
			CHECK_RET(GetTrieWord(tRowCtrl, pAddr, stTableIndex.pIndexInfo, sTrieWord),"GetTrieWord failed.");
			iRet = m_tTrieIndex.DeleteIndexNode(sTrieWord,stTableIndex.m_TrieIndexInfo, rowID);
		}

        //对于处于building状态的索引，找不到索引节点为合理的,不抛出错误
		if(iRet!=0  && stTableIndex.pIndexInfo->bBuilding==true)
		{
			TADD_DETAIL("DeleteIndexNode failed,Maybe it's in building.Table[%s],Index[%s].",
				m_pAttachTable->sTableName,stTableIndex.pIndexInfo->sName);
			iRet = 0;
		}
        return iRet;
    }

    int TMdbIndexCtrl::UpdateIndexNode(int iIndexPos,char* pOldData,ST_INDEX_VALUE& tMemIndexValue,TMdbRowCtrl& tRowCtrl,TMdbRowID& tRowId)
    {
        TADD_FUNC("IndexPos[%d]",iIndexPos);
        int iRet = 0;
        ST_TABLE_INDEX_INFO & stTableIndex = m_arrTableIndex[iIndexPos];
        if(false == stTableIndex.bInit)
        {
            //如果有动态创建的索引，此时需要重新attch
        	ReAttachSingleIndex(iIndexPos);
			if(false == stTableIndex.bInit)
			{
	            TADD_ERROR(-1,"stTableIndex is not init....");
	            return -1;
			}
        }

         int iError = 0;
         
        if(stTableIndex.pIndexInfo->m_iAlgoType == INDEX_HASH)
        {
            TMdbRowIndex tOldRowIndex;
            tOldRowIndex.Clear();
            
           
            tOldRowIndex.iBaseIndexPos = CalcIndexValue(tRowCtrl, pOldData, stTableIndex.pIndexInfo, iError);
            if(0 != iError)
            {
                TADD_ERROR(iError,"CalcIndexValue(iError=%d) failed.",iError);
                return -1;
            }
            long long llNewValue = 0;
            CHECK_RET(CalcMemValueHash(tMemIndexValue,llNewValue),"CalcMemValueHash error");
            if(llNewValue == tOldRowIndex.iBaseIndexPos)
            {
                TADD_DETAIL("hash value is same, not need to update");
                return iRet;
            }

            TMdbRowIndex tNewRowIndex;
            tNewRowIndex.Clear();
            tNewRowIndex.iBaseIndexPos = llNewValue;
            
            iRet = m_tHashIndex.UpdateIndexNode(iIndexPos, tOldRowIndex, tNewRowIndex, stTableIndex.m_HashIndexInfo,tRowId);
            
        }
        else if(stTableIndex.pIndexInfo->m_iAlgoType == INDEX_M_HASH)
        {
            long long iOldHashValue = 0;
            iOldHashValue = CalcIndexValue(tRowCtrl, pOldData, stTableIndex.pIndexInfo, iError);
            if(0 != iError)
            {
                TADD_ERROR(iError,"CalcMHashIndexValue(iError=%d) failed.",iError);
                return -1;
            }
            
            long long iNewHashValue = 0;
            CHECK_RET(CalcMemValueHash(tMemIndexValue,iNewHashValue),"CalcMemValueHash error");
            
            iRet = m_tMHashIndex.UpdateIndexNode( iOldHashValue, iNewHashValue, stTableIndex.m_MHashIndexInfo,tRowId);
        }
		else if(stTableIndex.pIndexInfo->m_iAlgoType == INDEX_TRIE)
        {  	
         	char sTrieWordOld[MAX_TRIE_WORD_LEN] = {0};
			char sTrieWordNew[MAX_TRIE_WORD_LEN] = {0};
			
			CHECK_RET(GetTrieWord(tRowCtrl, pOldData, stTableIndex.pIndexInfo, sTrieWordOld),"GetTrieWord failed.");
			CHECK_RET(GetTrieWord(tMemIndexValue, sTrieWordNew),"GetTrieWord failed.");
            iRet = m_tTrieIndex.UpdateIndexNode( sTrieWordOld, sTrieWordNew, stTableIndex.m_TrieIndexInfo,tRowId);
        }
        return iRet;
    }

    int TMdbIndexCtrl::PrintIndexInfo(int iDetialLevel,bool bConsole)
    {
        int iRet = 0;
        int i = 0;
        char sCurTime[64] = {0};
        TMdbDateTime::GetCurrentTimeStr(sCurTime);
        for(i = 0; i < MAX_INDEX_COUNTS; i++)
        {
            ST_TABLE_INDEX_INFO & stIndexInfo = m_arrTableIndex[i];
            if(stIndexInfo.bInit)
            {
                if(stIndexInfo.pIndexInfo->m_iAlgoType == INDEX_HASH)
                {
                    iRet = m_tHashIndex.PrintIndexInfo( stIndexInfo.m_HashIndexInfo,iDetialLevel, bConsole);
                }
                else if(stIndexInfo.pIndexInfo->m_iAlgoType == INDEX_M_HASH)
                {
                    iRet = m_tMHashIndex.PrintIndexInfo( stIndexInfo.m_MHashIndexInfo,iDetialLevel, bConsole);
                }
				else if(stIndexInfo.pIndexInfo->m_iAlgoType == INDEX_TRIE)
                {
                    iRet = m_tTrieIndex.PrintIndexInfo( stIndexInfo.m_TrieIndexInfo,iDetialLevel, bConsole);
                }
            }
        }
        return iRet;
    }

    long long TMdbIndexCtrl::CalcIndexValue( TMdbRowCtrl& tRowCtrl,char* pAddr, TMdbIndex* pIndex, int& iError)
    {
        TADD_FUNC("Start(pAddr=%p).",pAddr);
        iError = 0;
        long long llValue = 0;
        if(pIndex->m_iIndexType == HT_CMP)
        {
            llValue = CalcMPIndexValue(tRowCtrl,pAddr, pIndex, iError);
        }
        else
        {
            llValue = CalcOneIndexValue(tRowCtrl,pAddr, pIndex, iError);
        }
        llValue = llValue<0? -llValue:llValue;
        if(pIndex->m_iAlgoType == INDEX_HASH)
        {
            llValue = llValue % m_pAttachTable->iRecordCounts;
        }        
        TADD_FUNC("Finish(iIndexValue=%lld).", llValue);
        return llValue;
    }

	int TMdbIndexCtrl::GetTrieWord( ST_INDEX_VALUE & stIndexValue, char* sTrieWord)
    {
		int  iRet = 0; 
        if(HT_Char == stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType)
        {
			SAFESTRCPY(sTrieWord,MAX_TRIE_WORD_LEN,stIndexValue.pExprArr[0]->pExprValue->sValue);
		}
		return iRet;
    }

	int TMdbIndexCtrl::GetTrieWord( TMdbRowCtrl& tRowCtrl,char* pAddr, TMdbIndex* pIndex, char* sTrieWord)
    {
		int  iRet = 0; 
        if(HT_Char == pIndex->m_iIndexType)
        {
			int iCol = pIndex->iColumnNo[0];
            long long llValue = 0;
			char *sValue = NULL;
            int iValueType = 0;
            CHECK_RET(tRowCtrl.GetOneColumnValue(pAddr,&(m_pAttachTable->tColumn[iCol]),llValue,sValue,-1,iValueType), \
				"GetOneColumnValue failed.");

			SAFESTRCPY(sTrieWord, MAX_TRIE_WORD_LEN, sValue);
            return iRet;
		}
		CHECK_RET(-1,"Trie Index Only Support Char/VarChar.");
		return iRet;
    }

    long long TMdbIndexCtrl::CalcMPIndexValue(TMdbRowCtrl& tRowCtrl, char* pAddr, TMdbIndex* pIndex, int& iError)
    {
        TADD_FUNC("Start(pAddr=%p).",pAddr);
        iError = 0;
        //首先找到本索引对应的数据
        m_sTempValue[0] = 0;
        for(int i=0; i<MAX_INDEX_COLUMN_COUNTS; ++i)
        {
            if(pIndex->iColumnNo[i] < 0)
            {
                break;
            }
            //获取索引列所在数据的偏移量
            int iCol = pIndex->iColumnNo[i];
            long long llValue = 0;
            char * sValue = NULL;
            int iValueType = 0;
            iError = tRowCtrl.GetOneColumnValue(pAddr,&(m_pAttachTable->tColumn[iCol]),llValue,sValue,-1,iValueType);
            if(iError != 0)
            {
                TADD_ERROR(iError,"GetOneColumnValue failed.");
                return 0;
            }
            switch(iValueType)
            {
                case MEM_Int:
                    sprintf(m_sTempValue,"%s;%lld", m_sTempValue, llValue);
                    break;
                case MEM_Str:
                    sprintf(m_sTempValue,"%s;%s", m_sTempValue, sValue);
                    break;
                case MEM_Null:
                    sprintf(m_sTempValue,"%s;0", m_sTempValue);
                    break;
                default:
                    TADD_ERROR(ERR_SQL_TYPE_INVALID,"iValueType=%d invalid.", iValueType);
                    iError = ERR_SQL_TYPE_INVALID;
                    break;
            }
            TADD_DETAIL("sTempValue=%s.", m_sTempValue);
        }
        long long llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
        TADD_FUNC("Finish(iIndexValue=%lld).", llValue);
        return llValue;
    }

    long long TMdbIndexCtrl::CalcOneIndexValue(TMdbRowCtrl& tRowCtrl, char* pAddr, TMdbIndex* pIndex, int& iError)
    {
        TADD_FUNC("Start.");
        iError = 0;
        int iCol = pIndex->iColumnNo[0];
        TMdbColumn * pColumn = &(m_pAttachTable->tColumn[iCol]);
        TADD_DETAIL("ColoumName=[%s], offset=%d,iCol=%d",pColumn->sName,pColumn->iOffSet,iCol);
        long long llValue = 0;
        char * sValue = 0;
        int iValueType = 0;
        iError = tRowCtrl.GetOneColumnValue(pAddr,pColumn,llValue,sValue,-1,iValueType);
        if(iError != 0)
        {
            TADD_ERROR(iError,"GetOneColumnValue[%s] failed.",pColumn->sName);
            return 0;
        }
        switch(iValueType)
        {
            case MEM_Int://直接用获取的数值
                break;
            case MEM_Str:
                llValue = TMdbNtcStrFunc::StrToHash(sValue);
                break;
            case MEM_Null:
                llValue = 0;
                break;
            default:
                TADD_ERROR(ERR_SQL_TYPE_INVALID,"iValueType=%d invalid.", iValueType);
                iError = ERR_SQL_TYPE_INVALID;
                break;
        }
        TADD_FUNC("Finish(iIndexValue=%lld).", llValue);
        return llValue;
    }


    int TMdbIndexCtrl::CalcMemValueHash(ST_INDEX_VALUE & stIndexValue,long long & llValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_sTempValue[0] = 0;
        if(HT_CMP ==  stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType)
        {
            //组合索引
            int i = 0;
            for(i = 0; i < MAX_INDEX_COLUMN_COUNTS; ++i)
            {
                if(stIndexValue.pstTableIndex->pIndexInfo->iColumnNo[i] < 0)
                {
                    break;
                }
                CHECK_OBJ(stIndexValue.pExprArr[i]);
                ST_MEM_VALUE * pstMemValue = stIndexValue.pExprArr[i]->pExprValue;
                CHECK_OBJ(pstMemValue);
                if(pstMemValue->IsNull())
                {
                    sprintf(m_sTempValue, "%s;0",m_sTempValue);
                }
                else if(MemValueHasProperty(pstMemValue,MEM_Int))
                {
                    sprintf(m_sTempValue, "%s;%lld", m_sTempValue, pstMemValue->lValue);
                }
                else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
                {
                    sprintf(m_sTempValue, "%s;%s", m_sTempValue, pstMemValue->sValue);
                }
            }
            llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
        }
        else
        {
            //单索引
            if(stIndexValue.pExprArr[0]->pExprValue->IsNull())
            {
                llValue = 0;
            }
            else  if(MemValueHasAnyProperty(stIndexValue.pExprArr[0]->pExprValue,MEM_Str|MEM_Date))
            {
                llValue = TMdbNtcStrFunc::StrToHash(stIndexValue.pExprArr[0]->pExprValue->sValue);
            }
            else if(MemValueHasProperty(stIndexValue.pExprArr[0]->pExprValue,MEM_Int))
            {
                llValue = stIndexValue.pExprArr[0]->pExprValue->lValue;
            }
        }
        llValue = llValue<0? -llValue:llValue;
        if(stIndexValue.pstTableIndex->pIndexInfo->m_iAlgoType == INDEX_HASH)
        {
            //进行散列
            llValue = llValue % m_pAttachTable->iRecordCounts;
        }
        
        TADD_FUNC("Finish.llValue = %lld.m_sTempValue=[%s].",llValue,m_sTempValue);
        return iRet;
    }
    

    int TMdbIndexCtrl::AttachDsn(TMdbShmDSN * pMdbShmDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pMdbShmDsn);
        m_pMdbShmDsn = pMdbShmDsn;
        CHECK_RET(m_pMdbShmDsn->Attach(),"attach failed...");
        m_pMdbDsn = pMdbShmDsn->GetInfo();
        CHECK_OBJ(m_pMdbDsn);
        CHECK_RET(m_tHashIndex.AttachDsn(pMdbShmDsn),"hash attach failed...");
        CHECK_RET(m_tMHashIndex.AttachDsn(pMdbShmDsn),"mhash attach failed...");
        CHECK_RET(m_tTrieIndex.AttachDsn(pMdbShmDsn),"trie attach failed...");
        return iRet;
    }

    int TMdbIndexCtrl::DeleteTableSpecifiedIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,const char* pIdxName)
    {
        int iRet = 0;
        CleanTableIndexInfo();
        CHECK_OBJ(pMdbShmDsn);
        CHECK_OBJ(pTable);
        TADD_FUNC("Start.table=[%s].",pTable->sTableName);
        CHECK_OBJ(pIdxName);
        CHECK_RET(AttachDsn(pMdbShmDsn),"Attach dsn failed...");
        //先Attach上表获取索引信息
        CHECK_RET(AttachTable(pMdbShmDsn,pTable),"Attach table[%s] failed....",pTable->sTableName);
        for(int i = 0; i<pTable->iIndexCounts; i++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tIndex[i].sName,pIdxName) != 0)
            {
                continue;
            }
            if(m_arrTableIndex[i].bInit)
            {
                if(pTable->tIndex[i].m_iAlgoType == INDEX_HASH)
                {
                    iRet = m_tHashIndex.DeleteTableIndex(m_arrTableIndex[i].m_HashIndexInfo);
                }
                else if(pTable->tIndex[i].m_iAlgoType == INDEX_M_HASH)
                {
                    iRet = m_tMHashIndex.DeleteTableIndex(m_arrTableIndex[i].m_MHashIndexInfo);
                }
				else if(pTable->tIndex[i].m_iAlgoType == INDEX_TRIE)
                {
                    iRet = m_tTrieIndex.DeleteTableIndex(m_arrTableIndex[i].m_TrieIndexInfo);
                }
                else
                {
                    TADD_ERROR(-1, "Unknown index algo type .not add this index[%s]", pTable->tIndex[i].sName);
                    return ERR_TAB_INDEX_INVALID_TYPE;
                }
                
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbIndexCtrl::PrintWarnIndexInfo(int iMaxCNodeCount)
    {
        int iRet = 0;
        return iRet;
    }

int TMdbIndexCtrl::RenameTableIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,const char *sNewTableName)
{
    int iRet = 0;
    int iFindIndexs = 0;
    CHECK_RET(m_tHashIndex.AttachTable(pMdbShmDsn, pTable),"hash index attach table failed.");
    CHECK_RET(m_tMHashIndex.AttachTable(pMdbShmDsn, pTable),"M-hash index attach table failed.");
    CHECK_RET(m_tTrieIndex.AttachTable(pMdbShmDsn, pTable),"Trie index attach table failed.");

    // attach hash index
    for(int n=0; n<MAX_SHM_ID; ++n)
    {
        TADD_DETAIL("Attach (%s) hash Index : Shm-ID no.[%d].", pTable->sTableName, n);
        char * pBaseIndexAddr = pMdbShmDsn->GetBaseIndex(n);
        if(pBaseIndexAddr == NULL)
            continue;

        TMdbBaseIndexMgrInfo *pBIndexMgr = (TMdbBaseIndexMgrInfo*)pBaseIndexAddr;//获取基础索引内容
        for(int j=0; j<MAX_BASE_INDEX_COUNTS; ++j)
        {
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp( pTable->sTableName, pBIndexMgr->tIndex[j].sTabName))
            {
                iFindIndexs++;
                SAFESTRCPY(pBIndexMgr->tIndex[j].sTabName,sizeof(pBIndexMgr->tIndex[j].sTabName),sNewTableName);                    
            }
            if(iFindIndexs == pTable->iIndexCounts)
            {
                return iRet;
            }
            
        }
        if(iFindIndexs == pTable->iIndexCounts)
        {
            return iRet;
        }
        
        
    }

    // attach m-hash index, if exist
    for(int n=0; n<MAX_SHM_ID; ++n)
    {
        char * pBaseIndexAddr = pMdbShmDsn->GetMHashBaseIndex(n);
        if(pBaseIndexAddr == NULL)
            continue;

        TMdbMHashBaseIndexMgrInfo *pMHashBIndexMgr = (TMdbMHashBaseIndexMgrInfo*)pBaseIndexAddr;//获取基础索引内容
        TMdbMHashConflictIndexMgrInfo* pMHashConfMgr = pMdbShmDsn->GetMHashConfMgr();
        CHECK_OBJ(pMHashConfMgr);
        TMdbMHashLayerIndexMgrInfo* pMHashLayerMgr = pMdbShmDsn->GetMHashLayerMgr();
        CHECK_OBJ(pMHashLayerMgr);
        for(int j=0; j<MAX_MHASH_INDEX_COUNT; ++j)
        {
            //比较索引名称,如果相同，则把锁地址和索引地址记录下来
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(pTable->sTableName,pMHashBIndexMgr->tIndex[j].sTabName))
            {
                ++iFindIndexs;
                SAFESTRCPY(pMHashBIndexMgr->tIndex[j].sTabName,sizeof(pMHashBIndexMgr->tIndex[j].sTabName),sNewTableName);                    
                
            }
            if(iFindIndexs == pTable->iIndexCounts)
            {
                return iRet;
            }
            
        }
        
        if(iFindIndexs == pTable->iIndexCounts)
        {
            return iRet;
        }
    }  

	for(int n=0; n<MAX_BRIE_SHMID_COUNT; ++n)
    {
        char * pBaseIndexAddr = pMdbShmDsn->GetTrieRootIndex(n);
        if(pBaseIndexAddr == NULL)
            continue;

        TMdbTrieRootIndexMgrInfo *pBIndexMgr = (TMdbTrieRootIndexMgrInfo*)pBaseIndexAddr;//获取基础索引内容
        for(int j=0; j<MAX_BRIE_INDEX_COUNT; ++j)
        {
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp( pTable->sTableName, pBIndexMgr->tIndex[j].sTabName))
            {
                iFindIndexs++;
                SAFESTRCPY(pBIndexMgr->tIndex[j].sTabName,sizeof(pBIndexMgr->tIndex[j].sTabName),sNewTableName);                    
            }
            if(iFindIndexs == pTable->iIndexCounts)
            {
                return iRet;
            }
            
        }
        if(iFindIndexs == pTable->iIndexCounts)
        {
            return iRet;
        }
        
        
    }

    if(iFindIndexs != pTable->iIndexCounts)
    {
        TADD_ERROR(-1,"AttachTable(%s) : iFindIndexs[%d] < pTable->iIndexCounts[%d].",pTable->sTableName,iFindIndexs, pTable->iIndexCounts);
        return ERR_TAB_INDEX_NUM_INVALID;
    }
    return iRet;
}

bool TMdbIndexCtrl::CheckHashConflictIndexFull()
{
    if(!m_pAttachTable) 
    {
        TADD_ERROR(-1,"table not attached");
        return false;
    }
    for(int i=0; i<m_pAttachTable->iIndexCounts; ++i)
    {
        if(false == m_arrTableIndex[i].bInit)
        {
            TADD_ERROR(-1,"stTableIndex is not init....");
            return false;
        }

		//只检测hash索引
		if(m_arrTableIndex[i].pIndexInfo->m_iAlgoType != INDEX_HASH)
		{
			continue;
		}
		
        if(m_arrTableIndex[i].m_HashIndexInfo.pConflictIndex->iFreeHeadPos < 0)
        {
            TADD_ERROR(-1,"table[%s] index[%s] has no conflict index node,record-counts[%d] is too small",
                m_pAttachTable->sTableName,m_arrTableIndex[i].m_HashIndexInfo.pBaseIndex->sName,m_pAttachTable->iRecordCounts);
            return false;
        }
    }
    return true;
}


    

//}
