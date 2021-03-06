#include "Control/mdbTrieIndexCtrl.h"
#include "Control/mdbRowCtrl.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbDateTime.h"
 

//namespace QuickMDB{

    

    TMdbTrieIndexCtrl::TMdbTrieIndexCtrl()
    {
        m_pAttachTable = NULL;
        m_pMdbShmDsn = NULL;
        m_pMdbDsn = NULL;

    }

    TMdbTrieIndexCtrl::~TMdbTrieIndexCtrl()
    {
    }



    int TMdbTrieIndexCtrl::AttachDsn(TMdbShmDSN * pMdbShmDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pMdbShmDsn);
        m_pMdbShmDsn = pMdbShmDsn;
        CHECK_RET(m_pMdbShmDsn->Attach(),"attach failed...");
        m_pMdbDsn = pMdbShmDsn->GetInfo();
        CHECK_OBJ(m_pMdbDsn);
        return iRet;
    }

    int TMdbTrieIndexCtrl::AttachTable(TMdbShmDSN * pMdbShmDsn,TMdbTable* pTable)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        CHECK_RET(AttachDsn(pMdbShmDsn),"attch dsn failed.");
        m_pAttachTable = pTable;
        return iRet;
    }

	
    int TMdbTrieIndexCtrl::RenameTableIndex(TMdbShmDSN * pMdbShmDsn, TMdbTable* pTable, const char *sNewTableName, int& iFindIndexs)
    {
		int iRet  =0;
		CHECK_RET(AttachTable(pMdbShmDsn, pTable),"Trie index attach table failed.");
		for(int n=0; n<MAX_TRIE_SHMID_COUNT; ++n)
	    {
	        char * pBaseIndexAddr = pMdbShmDsn->GetTrieRootIndex(n);
	        if(pBaseIndexAddr == NULL)
	            continue;

	        TMdbTrieRootIndexMgrInfo *pBIndexMgr = (TMdbTrieRootIndexMgrInfo*)pBaseIndexAddr;//获取基础索引内容
	        for(int j=0; j<MAX_TRIE_INDEX_COUNT && iFindIndexs<pTable->iIndexCounts; ++j)
	        {
	            if(0 == TMdbNtcStrFunc::StrNoCaseCmp( pTable->sTableName, pBIndexMgr->tIndex[j].sTabName))
	            {
	                iFindIndexs++;
	                SAFESTRCPY(pBIndexMgr->tIndex[j].sTabName,sizeof(pBIndexMgr->tIndex[j].sTabName),sNewTableName);                    
	            }
	            
	        }
	        if(iFindIndexs == pTable->iIndexCounts)
	        {
	            return iRet;
	        }
	        
	        
	    }

		return iRet;
	}
	
	// 1 RootNode   +   n TrieNode  +  n ConfictNode
    int TMdbTrieIndexCtrl::AddTableSingleIndex(TMdbTable * pTable,int iIndexPos,size_t iDataSize)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);

		//根节点不需要申请那么多内存
		iDataSize = iDataSize/64;  
			
        TADD_FLOW("AddTableIndex Table=[%s],Size=[%lu] start.", pTable->sTableName,iDataSize);

		//只需要申请一个根节点的空间
        MDB_UINT64 iTrieRootSize     =  sizeof(TMdbTrieIndexNode); 
        ST_TRIE_INDEX_INFO stTableIndex;
        stTableIndex.Clear();		
        CHECK_RET(GetTrieFreeRootIndexShm(iTrieRootSize, iDataSize ,stTableIndex),"GetFreeBIndexShm failed..");
        if('2' == stTableIndex.pRootIndex->cState)
        {
            SAFESTRCPY(stTableIndex.pRootIndex->sName,sizeof(stTableIndex.pRootIndex->sName),pTable->tIndex[iIndexPos].sName);
			//初始化根节点
			CHECK_RET(InitTrieRootIndex(stTableIndex,pTable),"InitTrieRootIndex failed.");
			//初始化树节点
			CHECK_RET(InitTrieBranchIndex(stTableIndex),"InitTrieBranchIndex failed.");
			//初始化冲突节点
			CHECK_RET(InitConflictIndex(stTableIndex),"InitConflictIndex failed.");
            
        }
        else
        {
            CHECK_RET(-1,"not find pos for new index....");
        }
        TADD_FLOW("AddTableIndex : Table=[%s],index[%s] finish.", pTable->sTableName,pTable->tIndex[iIndexPos].sName);
        return iRet;
    }


    int TMdbTrieIndexCtrl::InitTrieRootIndex(ST_TRIE_INDEX_INFO & tTableIndex,TMdbTable * pTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        SAFESTRCPY(tTableIndex.pRootIndex->sTabName, sizeof(tTableIndex.pRootIndex->sTabName), pTable->sTableName);
        tTableIndex.pRootIndex->cState   = '1';
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pRootIndex->sCreateTime);
        InitTrieNode((TMdbTrieIndexNode *)((char *)tTableIndex.pRootIndexMgr + tTableIndex.pRootIndex->iPosAdd),
                      tTableIndex.pRootIndex->iSize,false);
        tTableIndex.pRootIndexMgr->iIndexCounts ++;
        TADD_FUNC("Finish.");
        return iRet;
    }


    int TMdbTrieIndexCtrl::InitTrieNode(TMdbTrieIndexNode* pNode,MDB_INT64 iSize,bool bList)
    {
        MDB_INT64 iCount = iSize/sizeof(TMdbTrieIndexNode);
        for(MDB_INT64 n=0; n<iCount; ++n)
        {
            pNode->Clear();
            ++pNode;
        }
        return 0;
    }
	
	int TMdbTrieIndexCtrl::InitTrieBranchIndex(ST_TRIE_INDEX_INFO & tTableIndex)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbTrieBranchIndexMgrInfo* pBranchMgr =m_pMdbShmDsn->GetTrieBranchMgr();
        CHECK_OBJ(pBranchMgr);

        int iPos = 0;
        bool bFree = false;
        for(int i = 0; i < MAX_TRIE_INDEX_COUNT; i++)
        {
            if('0' != pBranchMgr->tIndex[i].cState){continue;}
            iPos = i;
            bFree = true;
            break;
        }

        if(!bFree)
        {
            TADD_ERROR(-1, "no free pos to create TrieBranch index .");
            return ERR_TAB_INDEX_NUM_EXCEED_MAX;
        }

        tTableIndex.pRootIndex->iTrieBranchIndexPos=iPos;
        tTableIndex.pBranchIndex = &(pBranchMgr->tIndex[iPos]);
        CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
        tTableIndex.pBranchIndex->Clear();
        tTableIndex.pBranchIndex->cState = '1';
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pBranchIndex->sCreateTime);
        CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
		
        return iRet;
    }
	
	
	int TMdbTrieIndexCtrl::InitConflictIndex(ST_TRIE_INDEX_INFO & tTableIndex)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbTrieConflictIndexMgrInfo* pConflictMgr =m_pMdbShmDsn->GetTrieConfMgr();
        CHECK_OBJ(pConflictMgr);

        int iPos = 0;
        bool bFree = false;
        for(int i = 0; i < MAX_TRIE_INDEX_COUNT; i++)
        {
            if('0' != pConflictMgr->tIndex[i].cState){continue;}
            iPos = i;
            bFree = true;
            break;
        }

        if(!bFree)
        {
            TADD_ERROR(-1, "no free pos to create confilict index .");
            return ERR_TAB_INDEX_NUM_EXCEED_MAX;
        }

        tTableIndex.pRootIndex->iConflictIndexPos=iPos;
        tTableIndex.pConflictIndex = &(pConflictMgr->tIndex[iPos]);
        CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
        tTableIndex.pConflictIndex->Clear();
        tTableIndex.pConflictIndex->cState = '1';
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pConflictIndex->sCreateTime);
        CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
		
        return iRet;
    }
	
    int TMdbTrieIndexCtrl::CreateNewTrieRootIndexShm(size_t iShmSize)
    {
        TADD_FUNC("Start.Size=[%lu].",iShmSize);
        int iRet = 0;
        if(MAX_TRIE_SHMID_COUNT == m_pMdbShmDsn->GetInfo()->iTrieRootIdxShmCnt)
        {
            CHECK_RET(ERR_OS_NO_MEMROY,"can't create new TrieRoot shm,MAX_SHM_COUNTS[%d]",MAX_TRIE_SHMID_COUNT);
        }
        int iPos = m_pMdbDsn->iTrieRootIdxShmCnt;
        TADD_FLOW("Create TrieRootIndexShm:[%d],base_key[0x%0x]",iPos,m_pMdbDsn->iTrieRootIdxShmKey[iPos]);
        CHECK_RET(TMdbShm::Create(m_pMdbDsn->iTrieRootIdxShmKey[iPos], iShmSize, m_pMdbDsn->iTrieRootIdxShmID[iPos]),
                  " Can't Create TrieRootIndexShm errno=%d[%s].", errno, strerror(errno));
        TADD_FLOW("Base_SHM_ID =[%d].SHM_SIZE=[%lu].",m_pMdbDsn->iTrieRootIdxShmID[iPos],iShmSize);
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Create Trie Root IndexShm:[%d],size=[%luMB]",iPos,iShmSize/(1024*1024));
        m_pMdbDsn->iTrieRootIdxShmCnt ++;
        CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");//对于新创建的shm获取新映射地址
        //初始化索引区信息
        TMdbTrieRootIndexMgrInfo* pRootIndexMgr = (TMdbTrieRootIndexMgrInfo*)m_pMdbShmDsn->GetTrieRootIndex(iPos);// ->m_pBaseIndexShmAddr[iPos];
        CHECK_OBJ(pRootIndexMgr);
        pRootIndexMgr->iSeq = iPos;
        int i = 0;
        for(i = 0; i<MAX_TRIE_INDEX_COUNT; i++)
        {
            pRootIndexMgr->tIndex[i].Clear();
            pRootIndexMgr->tFreeSpace[i].Clear();
        }
        pRootIndexMgr->tFreeSpace[0].iPosAdd = sizeof(TMdbTrieRootIndexMgrInfo);
        pRootIndexMgr->tFreeSpace[0].iSize   = iShmSize - sizeof(TMdbTrieRootIndexMgrInfo);

        pRootIndexMgr->iIndexCounts = 0;
        pRootIndexMgr->iTotalSize   = iShmSize;
        pRootIndexMgr->tMutex.Create();
        TADD_FUNC("Finish.");
        return iRet;
    }



    int TMdbTrieIndexCtrl::GetTrieFreeRootIndexShm(MDB_UINT64 iBaseIndexSize,size_t iDataSize,ST_TRIE_INDEX_INFO & stTableIndexInfo)
    {
        TADD_FUNC("Start.iBaseIndexSize=[%lld].iDataSize=[%lu]",iBaseIndexSize,iDataSize);
        int iRet = 0;
        if((size_t)iBaseIndexSize > iDataSize - 10*1024*1024)
        {
            //所需空间太大一个内存块都不够放//预留10M空间
            CHECK_RET(-1,"DataSize is[%luM],it's too small,must > [%lldM],please change it",
                      iDataSize/1024/1024, iBaseIndexSize/1024/1024);
        }
        bool bFind = false;
        TMdbTrieRootIndexMgrInfo* pRootIndexMgr     = NULL;
        int i = 0;
        for(i = 0; i < MAX_TRIE_SHMID_COUNT; i++)
        {
            pRootIndexMgr = (TMdbTrieRootIndexMgrInfo*)m_pMdbShmDsn->GetTrieRootIndex(i);
            if(NULL ==  pRootIndexMgr ) //需要申请新的索引内存
            {
                CHECK_RET(CreateNewTrieRootIndexShm(iDataSize),"CreateNewTrieRootIndexShm[%d]failed",i);
                pRootIndexMgr = (TMdbTrieRootIndexMgrInfo*)m_pMdbShmDsn->GetTrieRootIndex(i);
                CHECK_OBJ(pRootIndexMgr);
            }
            CHECK_RET(pRootIndexMgr->tMutex.Lock(true),"Lock Faild");
            do
            {
                int j = 0;
                TMdbTrieRootIndex * pRootIndex = NULL;
                //搜寻可以放置索引信息的位置
                for(j = 0; j<MAX_TRIE_INDEX_COUNT; j++)
                {
                    if('0' == pRootIndexMgr->tIndex[j].cState)
                    {
                        //未创建的
                        pRootIndex = &(pRootIndexMgr->tIndex[j]);
                        stTableIndexInfo.iRootIndexPos = j;
                        break;
                    }
                }
                if(NULL == pRootIndex)
                {
                    break;   //没有空闲位置可以放索引信息
                }
                //搜寻是否还有空闲内存
                for(j = 0; j<MAX_TRIE_INDEX_COUNT; j++)
                {
                    if(pRootIndexMgr->tFreeSpace[j].iPosAdd >0)
                    {
                        TMDBIndexFreeSpace& tFreeSpace = pRootIndexMgr->tFreeSpace[j];
                        if(tFreeSpace.iSize >= (size_t)iBaseIndexSize)
                        {
                            pRootIndex->cState = '2';//更改状态
                            pRootIndex->iPosAdd   = tFreeSpace.iPosAdd;
                            pRootIndex->iSize     = iBaseIndexSize;
                            if(tFreeSpace.iSize - iBaseIndexSize > 0)
                            {
                                //还有剩余空间
                                tFreeSpace.iPosAdd += iBaseIndexSize;
                                tFreeSpace.iSize   -= iBaseIndexSize;
                            }
                            else
                            {
                                //该块空闲空间正好用光
                                tFreeSpace.Clear();
                            }
                            CHECK_RET_BREAK(DefragIndexSpace(pRootIndexMgr->tFreeSpace),"DefragIndexSpace failed...");
                        }
                    }
                }
                CHECK_RET_BREAK(iRet,"iRet = [%d]",iRet);
                if('2' != pRootIndex->cState)
                {
                    //没有空闲内存块放索引节点
                    break;
                }
                else
                {
                    //申请到空闲内存块
                    stTableIndexInfo.pRootIndex = pRootIndex;
                    stTableIndexInfo.pRootIndexMgr = pRootIndexMgr;
                    bFind = true;
                    break;
                }
            }
            while(0);
            CHECK_RET(pRootIndexMgr->tMutex.UnLock(true),"UnLock Faild");
            if(bFind)
            {
                break;   //找到
            }
        }
        if(!bFind)
        {
            CHECK_RET(-1,"GetFreeBIndexShm failed....");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbTrieIndexCtrl::GetFreeBranchShm(MDB_UINT64 iBranchSize,size_t iDataSize,TMdbTrieBlock*& pFreeBlock)
	{
		TADD_FUNC("Start.iConfIndexSize=[%lld].iDataSize=[%lu]",iBranchSize,iDataSize);
		int iRet = 0;

		if((size_t)iBranchSize > iDataSize - 10*1024*1024)
		{
			//所需空间太大一个内存块都不够放//预留10M空间
			CHECK_RET(-1,"DataSize is[%luM],it's too small,must > [%lldM],please change it",
					  iDataSize/1024/1024+10, iBranchSize/1024/1024);
		}

		TMdbTrieBlock* pBlock = NULL;
		TMdbTrieBranchIndexMgrInfo* pBranchMgr =m_pMdbShmDsn->GetTrieBranchMgr();
		CHECK_OBJ(pBranchMgr);

		pBlock = GetBlockById(pBranchMgr->iFreeBlockId, false);
		bool bFound = false;
		while(pBlock != NULL)
		{
			if(pBlock->iSize >= iBranchSize)
			{
				bFound = true;
				break;
			}

			pBlock = GetBlockById(pBlock->iNextBlock,false);
		}


		if(bFound)
		{
			CHECK_RET(RemoveBlock(pBlock, pBranchMgr->iFreeBlockId,false),"Remove free conflict block failed.");
			pFreeBlock = pBlock;			
			TADD_NORMAL("Branch  Find FreeBlock : %d,next:%d\n",pFreeBlock->iBlockId,pFreeBlock->iNextBlock);
			return iRet;
		}
		else
		{
			int iPos = GetFreeBranchPos();
			if(iPos < 0)
			{
				CHECK_RET(iPos,"Can't Create new trie conflict shm,MAX=[%d].",MAX_TRIE_INDEX_COUNT);
			}
			iRet = TMdbShm::Create(m_pMdbDsn->iTrieBranchIdxShmKey[iPos], iBranchSize+8, m_pMdbDsn->iTrieBranchIdxShmID[iPos]);
			if(iRet != 0)
			{
				CHECK_RET(iRet,"Can't create trie Branch shm share memory, errno=%d[%s].",errno, strerror(errno));
			}
			m_pMdbDsn->iTrieBranchIdxShmCnt++;
			CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");
			TADD_DETAIL(" iPos=%d, iTrieBranchIdxShmID=%d.",iPos,m_pMdbDsn->iTrieBranchIdxShmID[iPos]);
			char* pShmAddr = NULL;
			CHECK_RET(m_pMdbShmDsn->InitTrieBranchBlock(iPos, pShmAddr),"InitTrieBranchBlock failed.");
				
			CHECK_RET(m_pMdbShmDsn->AddNewTrieBranch(pFreeBlock),"add new trie conflict shm block info to mgrshm failed.");
			CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
			pFreeBlock->iShmID = m_pMdbDsn->iTrieBranchIdxShmID[iPos];
			pFreeBlock->iSize = iBranchSize; 			
			pFreeBlock->iBlockId = pBranchMgr->iTotalBlocks;
			pBranchMgr->iTotalBlocks++;
			pFreeBlock->tMutex.Create();
			
			TADD_NORMAL("Branch  Create FreeBlock : %d,next:%d\n",pFreeBlock->iBlockId,pFreeBlock->iNextBlock);
			 
			CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
			
		}

		return iRet;
	}

	
    int TMdbTrieIndexCtrl::GetFreeConflictShm(MDB_UINT64 iConflictSize,size_t iDataSize,TMdbTrieBlock*& pFreeBlock)
    {
        TADD_FUNC("Start.iConfIndexSize=[%lld].iDataSize=[%lu]",iConflictSize,iDataSize);
        int iRet = 0;

        if((size_t)iConflictSize > iDataSize -  10*1024*1024)
        {
            //所需空间太大一个内存块都不够放//预留10M空间
            CHECK_RET(-1,"DataSize is[%luM],it's too small,must > [%lldM],please change it",
                      iDataSize/1024/1024, iConflictSize/1024/1024);
        }

        TMdbTrieBlock* pBlock = NULL;
        TMdbTrieConflictIndexMgrInfo* pMgr =m_pMdbShmDsn->GetTrieConfMgr();
        CHECK_OBJ(pMgr);

        pBlock = GetBlockById(pMgr->iFreeBlockId, true);
        bool bFound = false;
        while(pBlock != NULL)
        {
            if(pBlock->iSize >= iConflictSize)
            {
                bFound = true;
                break;
            }

            pBlock = GetBlockById(pBlock->iNextBlock,true);
        }


        if(bFound)
        {
            CHECK_RET(RemoveBlock(pBlock, pMgr->iFreeBlockId,true),"Remove free conflict block failed.");
            pFreeBlock = pBlock;
			TADD_NORMAL("Conf  Find FreeBlock : %d,Next:%d\n",pFreeBlock->iBlockId, pFreeBlock->iNextBlock);
            return iRet;
        }
        else
        {
            int iPos = GetFreeConfPos();
            if(iPos < 0)
            {
                CHECK_RET(iPos,"Can't Create new trie conflict shm,MAX=[%d].",MAX_TRIE_INDEX_COUNT);
            }
            iRet = TMdbShm::Create(m_pMdbDsn->iTrieConfIdxShmKey[iPos], iConflictSize+8, m_pMdbDsn->iTrieConfIdxShmID[iPos]);
            if(iRet != 0)
            {
                CHECK_RET(iRet,"Can't create trie conflict shm share memory, errno=%d[%s].",errno, strerror(errno));
            }
            m_pMdbDsn->iTrieConfIdxShmCnt++;
            CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");
            TADD_DETAIL(" iPos=%d, iTrieConfIdxShmID=%d.",iPos,m_pMdbDsn->iTrieConfIdxShmID[iPos]);
            char* pShmAddr = NULL;
            CHECK_RET(m_pMdbShmDsn->InitTrieConfBlock(iPos, pShmAddr),"InitTrieConfBlock failed.");
            
             CHECK_RET(m_pMdbShmDsn->AddNewTrieConflict(pFreeBlock),"add new trie conflict shm block info to mgrshm failed.");
             CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
             pFreeBlock->iShmID = m_pMdbDsn->iTrieConfIdxShmID[iPos];
             pFreeBlock->iSize = iConflictSize;             
             pFreeBlock->iBlockId = pMgr->iTotalBlocks;
             pMgr->iTotalBlocks++;
			 TADD_NORMAL("Conf  Create FreeBlock : %d,Next:%d\n",pFreeBlock->iBlockId,pFreeBlock->iNextBlock);
             pFreeBlock->tMutex.Create();
             
             CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
            
        }

        return iRet;
    }

	

    int TMdbTrieIndexCtrl::DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[])
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int i,j;
        //先按从小到大排序，并且将无用的节点(iposAdd < 0)排到后面
        TMDBIndexFreeSpace tTempSapce;
        for(i = 0; i<MAX_TRIE_INDEX_COUNT; i++)
        {
            for(j = i; j<MAX_TRIE_INDEX_COUNT; j++)
            {
                if((tFreeSpace[i].iPosAdd > tFreeSpace[j].iPosAdd && tFreeSpace[j].iPosAdd>0)
                        || tFreeSpace[i].iPosAdd == 0)
                {
                    tTempSapce    = tFreeSpace[i];
                    tFreeSpace[i] = tFreeSpace[j];
                    tFreeSpace[j] = tTempSapce;
                }
            }
        }
        //查看相邻的节点是否可以合并
        for(i = 0; i<MAX_TRIE_INDEX_COUNT - 1;)
        {
            if(tFreeSpace[i].iPosAdd == 0)
            {
                break;   //结束
            }
            if(tFreeSpace[i].iPosAdd + tFreeSpace[i].iSize == tFreeSpace[i+1].iPosAdd )
            {
                //可以合并，并停留在这个节点
                TADD_DETAIL("FreeSpace[%d] and [%d] can merge.",i,i+1);
                tFreeSpace[i].iSize += tFreeSpace[i+1].iSize;
                if(i+2 == MAX_TRIE_INDEX_COUNT)
                {
                    tFreeSpace[i+1].Clear();
                }
                else
                {
                    //将后来的节点前移
                    memmove(&(tFreeSpace[i+1]),&(tFreeSpace[i+2]),
                            sizeof(TMDBIndexFreeSpace)*(MAX_TRIE_INDEX_COUNT-i-2));
                    tFreeSpace[MAX_TRIE_INDEX_COUNT - 1].Clear();
                }
            }
            else if(tFreeSpace[i].iPosAdd + tFreeSpace[i].iSize > tFreeSpace[i+1].iPosAdd &&
                    tFreeSpace[i+1].iPosAdd > 0 )
            {
                //说明地址有重叠报错
                CHECK_RET(-1,"address is overlaped....");
            }
            else
            {
                i++;//判断下一个节点
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbTrieIndexCtrl::InsertIndexNode(char* sTrieWord,ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

		TMdbTrieIndexNode* pRoot = &tTrieIndex.pRootNode[0];

		CHECK_OBJ(sTrieWord);
		CHECK_OBJ(pRoot);

		TMdbTrieIndexNode* pCur = pRoot;
		pCur->m_iFatherPos = -1;

		int cPos = 0;
		int iChrIndex = 0;
		
	    for(; sTrieWord[cPos] != 0; ++cPos)
	    {
	    	CHECK_OBJ(pCur);
			if(!isdigit(sTrieWord[cPos]))
			{
				CHECK_RET(ERR_TAB_INDEX_CALC_VALUE_FAILED,"TrieIndex only support '0~9'. Table[%s],Index[%s].",
					m_pAttachTable->sTableName,tTrieIndex.pRootIndex->sName);
			}
	    	iChrIndex = sTrieWord[cPos] - '0';
			
			//当前孩子节点是空的,就填一个上去				
			//节点加锁,添加孩子节点与其他线程排斥
			pCur->tMutex.Lock(true);
			do
			{
		    	if(-1 == pCur->m_iChildrenPos[iChrIndex])  
		    	{
					if(tTrieIndex.pBranchIndex->iFreeHeadPos < 0)
			        {
			            CHECK_RET_BREAK(ApplyNewBranchNode(tTrieIndex),"no more free branch node space.");
			        }
					

					int iFreePos = tTrieIndex.pBranchIndex->iFreeHeadPos;
					TMdbTrieIndexNode* pFreeNode = (TMdbTrieIndexNode*)GetAddrByIndexNodeId(tTrieIndex.pBranchIndex->iHeadBlockId,iFreePos,sizeof(TMdbTrieIndexNode),false);				
			        CHECK_OBJ_BREAK(pFreeNode);
			        pFreeNode->m_ch = sTrieWord[cPos];
					pFreeNode->m_iFatherPos = pCur->m_iCurPos;
					pFreeNode->m_iCurPos = iFreePos;
					pCur->m_iChildrenPos[iChrIndex] = iFreePos;	
					tTrieIndex.pBranchIndex->iFreeHeadPos = pFreeNode->m_iNextPos;  
					tTrieIndex.pBranchIndex->iFreeNodeCounts --;
				}	
			}while(0);			
			pCur->tMutex.UnLock(true);

			
			CHECK_RET(iRet,"Fill ChildNode Failed.")


			if(0 == sTrieWord[cPos+1])
			{				
				pCur = (TMdbTrieIndexNode*)GetAddrByIndexNodeId(tTrieIndex.pBranchIndex->iHeadBlockId, pCur->m_iChildrenPos[iChrIndex] ,sizeof(TMdbTrieIndexNode),false);

				//节点加锁，写排斥
				pCur->tMutex.Lock(true);
				do
				{
					if(pCur->m_NodeInfo.m_tRowId.IsEmpty())
					{
						pCur->m_NodeInfo.m_tRowId = rowID;
					}
					else
					{
						CHECK_RET_BREAK(InsertConflictNode(pCur->m_NodeInfo, tTrieIndex, rowID),"InsertConflictNode failed.");	
					}
				}while(0);
				pCur->tMutex.UnLock(true);

				
				return iRet;
			}
			else
			{
				pCur = (TMdbTrieIndexNode*)GetAddrByIndexNodeId(tTrieIndex.pBranchIndex->iHeadBlockId, pCur->m_iChildrenPos[iChrIndex] ,sizeof(TMdbTrieIndexNode),false);
			}
	    }
      
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbTrieIndexCtrl::InsertConflictNode(TMdbTrieIndexNodeInfo& tNodeInfo, ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID)
    {
        TADD_FUNC("start to insert into conflict list.");
        int iRet = 0;
        if(tTrieIndex.pConflictIndex->iFreeHeadPos < 0)
        {
            CHECK_RET(ApplyNewConflictNode(tTrieIndex),"no more free conflict node space.");
        }
        int iFreePos = tTrieIndex.pConflictIndex->iFreeHeadPos;
        TMdbTrieConfIndexNode * pFreeNode = (TMdbTrieConfIndexNode*)GetAddrByIndexNodeId(tTrieIndex.pConflictIndex->iHeadBlockId,iFreePos,sizeof(TMdbTrieConfIndexNode),true);
        CHECK_OBJ(pFreeNode);
        pFreeNode->m_NodeInfo.m_tRowId = rowID;//放入数据
        tTrieIndex.pConflictIndex->iFreeHeadPos = pFreeNode->m_iNextPos;
        pFreeNode->m_NodeInfo.m_iNextConfPos= tNodeInfo.m_iNextConfPos;
        tNodeInfo.m_iNextConfPos = iFreePos;
        tTrieIndex.pConflictIndex->iFreeNodeCounts --;//剩余节点数-1
        
        TADD_DETAIL("insert on conflict[%s][%s][%d]", tTrieIndex.pRootIndex->sTabName,tTrieIndex.pRootIndex->sName,iFreePos);

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbTrieIndexCtrl::ApplyNewConflictNode(ST_TRIE_INDEX_INFO & tTableIndex)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        int iNewCount = m_pAttachTable->iTabLevelCnts/16;
        int iNodeSize = sizeof(TMdbTrieConfIndexNode);
        MDB_UINT64 iConflictSize = iNewCount * iNodeSize;
        TMdbTrieBlock* pBlock = NULL;
        CHECK_RET(GetFreeConflictShm(iConflictSize, m_pMdbDsn->m_iDataSize,pBlock), "Get Free Conflict index shm failed.");

        CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
        do
        {
            pBlock->iStartNode = tTableIndex.pConflictIndex->iTotalNodes + 1;
            pBlock->iEndNode = tTableIndex.pConflictIndex->iTotalNodes + iNewCount;
            iRet = AddBlock(tTableIndex.pConflictIndex->iHeadBlockId, pBlock,true);
            CHECK_RET_BREAK(iRet,"AddBlock failed.");
            tTableIndex.pConflictIndex->iTotalNodes += iNewCount;
        }while(0);
        CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");

        CHECK_RET(iRet,"init block failed.");

        tTableIndex.pConflictIndex->iFreeHeadPos = pBlock->iStartNode;
        tTableIndex.pConflictIndex->iFreeNodeCounts += iNewCount;
        
        CHECK_RET(pBlock->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock block failed.");
        
        char* pAddr = m_pMdbShmDsn->GetTrieConfShmAddr(pBlock->iShmID);
        TMdbTrieConfIndexNode* pNode = (TMdbTrieConfIndexNode*)pAddr;
        for(int i = pBlock->iStartNode; i <= pBlock->iEndNode; i++)
        {
            pNode->m_NodeInfo.m_tRowId.Clear();
            if(i != pBlock->iEndNode)
            {
                pNode->m_iNextPos = i +1;
            }
            else
            {
                pNode->m_iNextPos = -1;
            }
            pNode = (TMdbTrieConfIndexNode*)(pAddr+iNodeSize*(i+1-pBlock->iStartNode)) ;
        }
        //pNode->m_iNextNode = -1;// 结尾
        CHECK_RET(pBlock->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
        
        return iRet;
    }

	int TMdbTrieIndexCtrl::ApplyNewBranchNode(ST_TRIE_INDEX_INFO & tTableIndex)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        int iNewCount = m_pAttachTable->iTabLevelCnts/8;
        int iNodeSize = sizeof(TMdbTrieIndexNode);
        MDB_UINT64 iBaseSize = iNewCount * iNodeSize;
        TMdbTrieBlock* pBlock = NULL;
        CHECK_RET(GetFreeBranchShm(iBaseSize, m_pMdbDsn->m_iDataSize,pBlock), "Get Free Branch index shm failed.");

        CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
        do
        {
            pBlock->iStartNode = tTableIndex.pBranchIndex->iTotalNodes + 1;
            pBlock->iEndNode = tTableIndex.pBranchIndex->iTotalNodes + iNewCount;
            iRet = AddBlock(tTableIndex.pBranchIndex->iHeadBlockId, pBlock,false);
            CHECK_RET_BREAK(iRet,"AddBlock failed.");
            tTableIndex.pBranchIndex->iTotalNodes += iNewCount;
        }while(0);
        CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");

        CHECK_RET(iRet,"init block failed.");

        tTableIndex.pBranchIndex->iFreeHeadPos = pBlock->iStartNode;
        tTableIndex.pBranchIndex->iFreeNodeCounts += iNewCount;
        
        CHECK_RET(pBlock->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock block failed.");
        
        char* pAddr = m_pMdbShmDsn->GetTrieBranchShmAddr(pBlock->iShmID);
        TMdbTrieIndexNode* pNode = (TMdbTrieIndexNode*)pAddr;
        for(int i = pBlock->iStartNode; i <= pBlock->iEndNode; i++)
        {
            pNode->Clear();
            if(i != pBlock->iEndNode)
            {
                pNode->m_iNextPos = i +1;
            }
            else
            {
                pNode->m_iNextPos = -1;
            }
            pNode = (TMdbTrieIndexNode*)(pAddr+iNodeSize*(i+1-pBlock->iStartNode)) ;
        }
        CHECK_RET(pBlock->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
        
        return iRet;
    }

    int TMdbTrieIndexCtrl::DeleteIndexNode(char* sTrieWord, ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID)
    {
    
    	int iRet =0;
        TADD_FUNC("Start.");
		
		TMdbTrieIndexNode* pRoot = &tTrieIndex.pRootNode[0];

		CHECK_OBJ(sTrieWord);
		CHECK_OBJ(pRoot);

		if(0 == sTrieWord[0])
		{
			return iRet;
		}

		//分支
		TMdbTrieIndexNode* pCur = pRoot;

		int cPos = 0;
		int iChrIndex = 0;

		//完全匹配删除
		for(; sTrieWord[cPos] != 0; ++cPos)
		{
			CHECK_OBJ(pCur);
			if(!isdigit(sTrieWord[cPos])){return iRet;}
			
			iChrIndex = sTrieWord[cPos] - '0';

			//匹配失败
			if(-1 == pCur->m_iChildrenPos[iChrIndex])  
			{
				return iRet;   
			}

			pCur = (TMdbTrieIndexNode*)GetAddrByIndexNodeId(tTrieIndex.pBranchIndex->iHeadBlockId, pCur->m_iChildrenPos[iChrIndex] ,sizeof(TMdbTrieIndexNode),false);

		}

		//节点加锁，写排斥
		pCur->tMutex.Lock(true);
		if(pCur->m_NodeInfo.m_tRowId == rowID)
		{
			pCur->m_NodeInfo.m_tRowId.Clear();
		}
        else if(pCur->m_NodeInfo.m_iNextConfPos>0)
        {
        	bool bFound = false;
			DeleteIndexNodeOnConfList(tTrieIndex,&(pCur->m_NodeInfo),rowID,bFound);
			if(bFound == false)
			{
				TADD_WARNING("rowID %d On TrieConflictList  cannot be found.",rowID);
			}
		}		
		pCur->tMutex.UnLock(true);
		
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbTrieIndexCtrl::DeleteIndexNodeOnConfList(ST_TRIE_INDEX_INFO& tTrieIndex,TMdbTrieIndexNodeInfo* pNodeInfo, TMdbRowID& tRowId,bool & bFound)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        TMdbTrieConfIndexNode* pPre = NULL;
        int iNodeSize = sizeof(TMdbTrieConfIndexNode);

        int iCurPos = -1;
        TMdbTrieConfIndexNode* pConfNode = (TMdbTrieConfIndexNode*)GetAddrByIndexNodeId(tTrieIndex.pConflictIndex->iHeadBlockId, pNodeInfo->m_iNextConfPos, iNodeSize, true);
        CHECK_OBJ(pConfNode);
        iCurPos = pNodeInfo->m_iNextConfPos;
        do
        {
            if(pConfNode->m_NodeInfo.m_tRowId == tRowId)
            {
                bFound = true;
                if(pPre == NULL)
                {
                    pNodeInfo->m_iNextConfPos = pConfNode->m_NodeInfo.m_iNextConfPos;
                }
                else
                {
                    pPre->m_NodeInfo.m_iNextConfPos = pConfNode->m_NodeInfo.m_iNextConfPos;
                }
                
                //回收节点到空闲链表
                pConfNode->m_iNextPos = tTrieIndex.pConflictIndex->iFreeHeadPos;
                tTrieIndex.pConflictIndex->iFreeHeadPos = iCurPos;
                tTrieIndex.pConflictIndex->iFreeNodeCounts++;

                pConfNode->m_NodeInfo.m_tRowId.Clear();
                
                break;
            }

			// 冲突链结束
            if(pConfNode->m_NodeInfo.m_iNextConfPos < 0)
            {
                TADD_DETAIL("not found on conflict list.");
                break;
            }
			// 查找下一个
            else
            {
                TADD_DETAIL("continue to check next conflict node on list.");
                pPre = pConfNode;
                iCurPos= pConfNode->m_NodeInfo.m_iNextConfPos;
                pConfNode =(TMdbTrieConfIndexNode*)GetAddrByIndexNodeId(tTrieIndex.pConflictIndex->iHeadBlockId, pConfNode->m_NodeInfo.m_iNextConfPos, iNodeSize, true);
            }
            
        }while(1);
                        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbTrieIndexCtrl::UpdateIndexNode(char* sTrieWordOld, char* sTrieWordNew,ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID)
    {
        int iRet = 0;
        TADD_DETAIL("UPDATE:[%s][%s]oldvalue[%s],newvalue=[%s],row[%d]",tTrieIndex.pRootIndex->sTabName,tTrieIndex.pRootIndex->sName, sTrieWordOld,  sTrieWordNew,rowID.m_iRowID);
        CHECK_RET(DeleteIndexNode(sTrieWordOld, tTrieIndex,  rowID),"delete index node failed.");
        CHECK_RET(InsertIndexNode(sTrieWordNew, tTrieIndex,  rowID),"insert index node failed.");
        
        return iRet;
    }

    int TMdbTrieIndexCtrl::DeleteTableIndex(ST_TRIE_INDEX_INFO& tIndexInfo)
    {
        int iRet = 0;

        //清理基础索引信息
        CHECK_RET(tIndexInfo.pRootIndexMgr->tMutex.Lock(true), "lock failed.");
        do
        {
            CHECK_RET_BREAK(RecycleIndexSpace(tIndexInfo.pRootIndexMgr->tFreeSpace,
                                              tIndexInfo.pRootIndex->iPosAdd,
                                              tIndexInfo.pRootIndex->iSize),"RecycleIndexSpace failed...");
            tIndexInfo.pRootIndex->Clear();
            tIndexInfo.pRootIndexMgr->iIndexCounts --;//基础索引-1
        }
        while(0);
        CHECK_RET(tIndexInfo.pRootIndexMgr->tMutex.UnLock(true),"unlock failed.");
        CHECK_RET(iRet,"ERROR.");

        
                
        CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
        do
        {
            // 冲突索引清理
            TMdbTrieConflictIndexMgrInfo* pConfMgr =m_pMdbShmDsn->GetTrieConfMgr();
            if(NULL == pConfMgr)
            {
                break;
            }
            TMdbTrieBlock* pBlock = GetBlockById(tIndexInfo.pConflictIndex->iHeadBlockId, true);
            while(pBlock)
            {
                if(AddBlock(pConfMgr->iFreeBlockId, pBlock, true) < 0)
                {
                    break;
                }

                pBlock = GetBlockById(pBlock->iNextBlock, true);
            }
            tIndexInfo.pConflictIndex->Clear();
            pConfMgr->iIndexCounts--;

			
			TMdbTrieBranchIndexMgrInfo* pBranchMgr =m_pMdbShmDsn->GetTrieBranchMgr();
			if(NULL == pBranchMgr)
			{
				break;
			}
			pBlock = GetBlockById(tIndexInfo.pBranchIndex->iHeadBlockId, false);
			while(pBlock)
			{
				if(AddBlock(pBranchMgr->iFreeBlockId, pBlock, true) < 0)
				{
					break;
				}

				pBlock = GetBlockById(pBlock->iNextBlock, true);
			}
			tIndexInfo.pBranchIndex->Clear();
			pBranchMgr->iIndexCounts--;

        }while(0);
        CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");    

        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbTrieIndexCtrl::TruncateTableIndex(ST_TRIE_INDEX_INFO& tIndexInfo)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		//初始化基础索引
		InitTrieNode((TMdbTrieIndexNode *)((char *)tIndexInfo.pRootIndexMgr + tIndexInfo.pRootIndex->iPosAdd),
		              tIndexInfo.pRootIndex->iSize,false);

		

		CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
		do
		{
			// 冲突索引清理
			TMdbTrieConflictIndexMgrInfo* pConfMgr =m_pMdbShmDsn->GetTrieConfMgr();
			if(NULL == pConfMgr)
			{
				break;
			}
			TMdbTrieBlock* pBlock = GetBlockById(tIndexInfo.pConflictIndex->iHeadBlockId, true);
			while(pBlock)
			{
				int iNextBlockId = pBlock->iNextBlock;
				
				TADD_NORMAL("Conf  Return Block:%d,next:%d\n",pBlock->iBlockId,iNextBlockId);
				if(AddBlock(pConfMgr->iFreeBlockId, pBlock, true) < 0)
				{
					break;
				}			
				pBlock = GetBlockById(iNextBlockId, true);
			}
			tIndexInfo.pConflictIndex->iFreeHeadPos = -1;
		    tIndexInfo.pConflictIndex->iFreeNodeCounts = 0;
		    tIndexInfo.pConflictIndex->iTotalNodes = 0;
		    tIndexInfo.pConflictIndex->iHeadBlockId = -1;

			TMdbTrieBranchIndexMgrInfo* pBranchMgr =m_pMdbShmDsn->GetTrieBranchMgr();
			if(NULL == pBranchMgr)
			{
				break;
			}
			pBlock = GetBlockById(tIndexInfo.pBranchIndex->iHeadBlockId, false);
			while(pBlock)
			{
				int iNextBlockId = pBlock->iNextBlock;
				TADD_NORMAL("Branch  Return Block:%d,next:%d\n",pBlock->iBlockId,iNextBlockId);				
				if(AddBlock(pBranchMgr->iFreeBlockId, pBlock, false) < 0)
				{
					break;
				}			
				pBlock = GetBlockById(iNextBlockId, false);
			}
			tIndexInfo.pBranchIndex->iFreeHeadPos = -1;
		    tIndexInfo.pBranchIndex->iFreeNodeCounts = 0;
		    tIndexInfo.pBranchIndex->iTotalNodes = 0;
		    tIndexInfo.pBranchIndex->iHeadBlockId = -1;

								
		}while(0);
		CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");	

		TADD_FUNC("Finish.");
		return iRet;
	}

	int TMdbTrieIndexCtrl::OutPutInfo(bool bConsole,const char * fmt, ...)
    {
        static char sLogTemp[10240];
		memset(sLogTemp,0,sizeof(sLogTemp));
        va_list ap;
        va_start(ap,fmt);
        vsnprintf(sLogTemp, sizeof(sLogTemp), fmt, ap);
        va_end (ap);
        if(bConsole)
        {
            printf("%s",sLogTemp);
        }
        else
        {
            TADD_NORMAL("%s",sLogTemp);
        }
        return 0;
    }

     int TMdbTrieIndexCtrl::PrintIndexInfo(ST_TRIE_INDEX_INFO& stIndexInfo,int iDetialLevel,bool bConsole)
    {
        int iRet = 0;
        int iBICounts = stIndexInfo.pBranchIndex->GetTotalCount();//树枝节点
        int iCICounts = stIndexInfo.pConflictIndex->GetTotalCount();//冲突索引个数
        OutPutInfo(bConsole,"\n\n============[%s]===========\n",stIndexInfo.pRootIndex->sName);
        OutPutInfo(bConsole,"[BranchIndex] 	 counts=[%d],FreeHeadPos=[%d],FreeNodes=[%d]\n",
			iBICounts,stIndexInfo.pBranchIndex->iFreeHeadPos,stIndexInfo.pBranchIndex->iFreeNodeCounts);
        OutPutInfo(bConsole,"[ConfilictIndex] counts=[%d]，FreeHeadPos=[%d],FreeNodes=[%d]\n",
			iCICounts,stIndexInfo.pConflictIndex->iFreeHeadPos,stIndexInfo.pConflictIndex->iFreeNodeCounts);


		TMdbTrieBranchIndexMgrInfo* pBranchMgr =m_pMdbShmDsn->GetTrieBranchMgr();
		CHECK_OBJ(pBranchMgr);
		OutPutInfo(bConsole,"BranchBlock:");					
		TMdbTrieBlock* pBlock = GetBlockById(stIndexInfo.pBranchIndex->iHeadBlockId, false);
		while(pBlock)
		{
			OutPutInfo(bConsole,"%d-",pBlock->iBlockId);					
			pBlock = GetBlockById(pBlock->iNextBlock, false);
		}

		TMdbTrieConflictIndexMgrInfo* pConfMgr =m_pMdbShmDsn->GetTrieConfMgr();
		CHECK_OBJ(pConfMgr);
		OutPutInfo(bConsole,"ConfBlock:");					
		pBlock = GetBlockById(stIndexInfo.pConflictIndex->iHeadBlockId, true);
		while(pBlock)
		{
			OutPutInfo(bConsole,"%d-",pBlock->iBlockId);					
			pBlock = GetBlockById(pBlock->iNextBlock, true);
		}
		
		int iMaxDep = iDetialLevel;
        if(iMaxDep >1 )
        {//详细信息
           PrintIndexInfoDetail(iMaxDep,bConsole,stIndexInfo);
        }
        
        return iRet;
    }

	 int TMdbTrieIndexCtrl::PrintIndexInfoDetail(int iMaxDep,bool bConsole, ST_TRIE_INDEX_INFO & stIndexInfo)
	 {	
		OutPutInfo(bConsole,"TrieIndex(@ - data ; O - empty)\n");
		int iCurDep = 0;
		TMdbTrieIndexNode*  pRoot = stIndexInfo.pRootNode;
		for(int i=0; i<10; i++)
		{
			 TMdbTrieIndexNode*  pChild =(TMdbTrieIndexNode*)GetAddrByIndexNodeId(stIndexInfo.pBranchIndex->iHeadBlockId, pRoot->m_iChildrenPos[i] ,sizeof(TMdbTrieIndexNode),false);
			 if(NULL == pChild) continue;

			 
			 OutPutInfo(bConsole,"Dep:%d",iCurDep);
			 OutPutInfo(bConsole,"[%c]",pChild->m_ch);

			 if(!pChild->m_NodeInfo.m_tRowId.IsEmpty())
			 {
				 OutPutInfo(bConsole,"-@");
			 }
			 else
			 {
				 OutPutInfo(bConsole,"-O");
			 }

			 TMdbTrieConfIndexNode* pNextConf=(TMdbTrieConfIndexNode*)GetAddrByIndexNodeId(stIndexInfo.pConflictIndex->iHeadBlockId,pChild->m_NodeInfo.m_iNextConfPos,sizeof(TMdbTrieConfIndexNode),true);
			 while(pNextConf)
			 {
				 if(!pNextConf->m_NodeInfo.m_tRowId.IsEmpty())
				 {
					 OutPutInfo(bConsole,"-@");
				 }
				 else
				 {
					 OutPutInfo(bConsole,"-O");
				 }
				 pNextConf=(TMdbTrieConfIndexNode*)GetAddrByIndexNodeId(stIndexInfo.pConflictIndex->iHeadBlockId,pNextConf->m_NodeInfo.m_iNextConfPos,sizeof(TMdbTrieConfIndexNode),true);
			 }
			 
			 OutPutInfo(bConsole,"\n");
			 
			 RecursiveShowTrie(pChild, iCurDep ,iMaxDep, bConsole, stIndexInfo);
		}
		return 0;
	 }
	 
	
	int TMdbTrieIndexCtrl::RecursiveShowTrie(TMdbTrieIndexNode* pChild,int iCurDep, int iMaxDep,bool bConsole, ST_TRIE_INDEX_INFO & stIndexInfo)
	{
		iCurDep++;
		if(iMaxDep<iCurDep){return 0;}

		TMdbTrieIndexNode* pRoot = pChild;
		
		for(int i=0; i<10; i++)
		{
			 TMdbTrieIndexNode*  pTemp =(TMdbTrieIndexNode*)GetAddrByIndexNodeId(stIndexInfo.pBranchIndex->iHeadBlockId, pRoot->m_iChildrenPos[i] ,sizeof(TMdbTrieIndexNode),false);
			 if(NULL == pTemp) continue;
			 pChild = pTemp; 

			
			 OutPutInfo(bConsole,"Dep:%d",iCurDep);
			 for(int j=0;j<iCurDep;j++)
			 	OutPutInfo(bConsole,"-");			 
			 OutPutInfo(bConsole,"[%c]",pChild->m_ch);

			 if(!pChild->m_NodeInfo.m_tRowId.IsEmpty())
			 {
				 OutPutInfo(bConsole,"-@");
			 }
			 else
			 {
				 OutPutInfo(bConsole,"-O");
			 }

			 TMdbTrieConfIndexNode* pNextConf=(TMdbTrieConfIndexNode*)GetAddrByIndexNodeId(stIndexInfo.pConflictIndex->iHeadBlockId,pChild->m_NodeInfo.m_iNextConfPos,sizeof(TMdbTrieConfIndexNode),true);
			 while(pNextConf)
			 {
				 if(!pNextConf->m_NodeInfo.m_tRowId.IsEmpty())
				 {
					 OutPutInfo(bConsole,"-@");
				 }
				 else
				 {
					 OutPutInfo(bConsole,"-O");
				 }
				 pNextConf=(TMdbTrieConfIndexNode*)GetAddrByIndexNodeId(stIndexInfo.pConflictIndex->iHeadBlockId,pNextConf->m_NodeInfo.m_iNextConfPos,sizeof(TMdbTrieConfIndexNode),true);
			 }
			 OutPutInfo(bConsole,"\n");
			 
			 RecursiveShowTrie(pChild, iCurDep ,iMaxDep, bConsole, stIndexInfo);
		}
		return 0;
	}
     int TMdbTrieIndexCtrl::RecycleIndexSpace(TMDBIndexFreeSpace tFreeSpace[],size_t iPosAdd,size_t iSize)
    {
        TADD_FUNC("Start.tFreeSpace=[%p],iPosAdd=[%d],iSize=[%d].",tFreeSpace,iPosAdd,iSize);
        int iRet = 0;
        int i = 0;
        for(i = 0; i<MAX_MHASH_INDEX_COUNT; i++)
        {
            if(tFreeSpace[i].iPosAdd == 0)
            {
                //找到一个空闲记录点记录下空闲区域
                tFreeSpace[i].iPosAdd = iPosAdd;
                tFreeSpace[i].iSize   = iSize;
                break;
            }
        }
        CHECK_RET(DefragIndexSpace(tFreeSpace),"DefragIndexSpace failed....");
        TADD_FUNC("Finish.");
        return iRet;
    }

    

    char* TMdbTrieIndexCtrl::GetAddrByIndexNodeId(int iHeadBlock,int iIndexNodeId, int iNodeSize, bool bConf)
    {
        TADD_FUNC("Start.");
        TADD_DETAIL("iHeadBlock=%d,iIndexNodeId=%d,iNodeSize=%d, bConf=[%s]"
            ,iHeadBlock,iIndexNodeId, iNodeSize, bConf?"TRUE":"FALSE");
        
        char* pAddr = NULL;

		if(iHeadBlock<0) return NULL;

        TMdbTrieBlock* pHeadBlock = GetBlockById(iHeadBlock, bConf);
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
                        pAddr = m_pMdbShmDsn->GetTrieConfShmAddr(pTmpBlock->iShmID);
                    }
                    else
                    {
                        pAddr = m_pMdbShmDsn->GetTrieBranchShmAddr(pTmpBlock->iShmID);
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
                        pTmpBlock = GetBlockById(pTmpBlock->iNextBlock,bConf);
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


    int TMdbTrieIndexCtrl::AddBlock(int& iHeadId, TMdbTrieBlock* pBlockToAdd, bool bConf)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pBlockToAdd);

        TADD_DETAIL("head[%d], toadd[%d]", iHeadId, pBlockToAdd->iBlockId);
        if(iHeadId < 0)
        {
            iHeadId= pBlockToAdd->iBlockId;
            pBlockToAdd->iNextBlock = -1;
        }
        else
        {
            TMdbTrieBlock* pTmp = GetBlockById(iHeadId,bConf);
			CHECK_OBJ(pTmp);
            while(1)
            {
                if(pTmp->iNextBlock < 0)
                {
                    break;
                }
                else
                {
                    pTmp = GetBlockById(pTmp->iNextBlock,bConf);
                    CHECK_OBJ(pTmp);
                }
            }

            pTmp->iNextBlock = pBlockToAdd->iBlockId;
            pBlockToAdd->iNextBlock = -1;
                
        }
        
        return iRet;
    }


    int TMdbTrieIndexCtrl::RemoveBlock(TMdbTrieBlock* pBlockToDel, int&  iHeadID, bool bConf)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TADD_DETAIL("BlockToRemove[%d], HeadBlock[%d]", pBlockToDel->iBlockId, iHeadID);

        if(iHeadID < 0)
        {
            TADD_ERROR(-1, "head=[%d], can't remove any block", iHeadID);
            return -1;
        }
        TMdbTrieBlock* pHead = GetBlockById(iHeadID,bConf);
        CHECK_OBJ(pHead);

        if(pBlockToDel->iBlockId == pHead->iBlockId)
        {
            if(pHead->iNextBlock > 0)
            {
                TMdbTrieBlock* pNext = GetBlockById(pHead->iNextBlock,bConf);
                CHECK_OBJ(pNext);
                iHeadID = pNext->iBlockId;
            }
            else
            {
                iHeadID = -1;
            }
            
        }
        else
        {
            TMdbTrieBlock* pPre = pHead;
            TMdbTrieBlock* pCur = pPre;
            while(pCur->iNextBlock > 0)
            {
                TADD_DETAIL("CurBlock[%d], PreBlock[%d]", pCur->iBlockId, pPre->iBlockId);
                if(pCur->iBlockId == pBlockToDel->iBlockId)
                {
                    break;
                }
                pPre = pCur;
                pCur = GetBlockById(pCur->iNextBlock,bConf);        
                if(pCur == NULL)
                {
                    TADD_ERROR(-1, "not find block[%d]", pPre->iNextBlock);
                    return ERROR_UNKNOWN;
                }
            }

            if(pCur->iBlockId != pBlockToDel->iBlockId)
            {
                TADD_ERROR(-1,"not find block to remove");
                return ERROR_UNKNOWN;
            }

            if(pCur->iNextBlock > 0)
            {
                TMdbTrieBlock* pNext = GetBlockById(pCur->iNextBlock,bConf);
                CHECK_OBJ(pNext);
                pPre->iNextBlock = pNext->iBlockId;
            }
            else
            {
                pPre->iNextBlock = -1;
            }

        }

        pBlockToDel->iStartNode= -1;
        pBlockToDel->iEndNode = -1;
        pBlockToDel->iNextBlock = -1; 
        
        TADD_FUNC("Finish.");
        return iRet;
    }


    TMdbTrieBlock* TMdbTrieIndexCtrl::GetBlockById(int iBlockID, bool bConf)
    {
        if(bConf)
        {
            return m_pMdbShmDsn->GetTrieConfBlockById( iBlockID);
        }
        else
        {
            return m_pMdbShmDsn->GetTrieBranchBlockById( iBlockID);
        }
    }

	int TMdbTrieIndexCtrl::GetFreeBranchPos()
    {
        TADD_FUNC("Start.");
        int i = 0;
        for(; i<MAX_TRIE_SHMID_COUNT; i++)
        {
            if(m_pMdbDsn->iTrieBranchIdxShmID[i] == INITVAl)
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

	
    int TMdbTrieIndexCtrl::GetFreeConfPos()
    {
        TADD_FUNC("Start.");
        int i = 0;
        for(; i<MAX_TRIE_SHMID_COUNT; i++)
        {
            if(m_pMdbDsn->iTrieConfIdxShmID[i] == INITVAl)
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
    
//}
