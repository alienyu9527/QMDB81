#include "Control/mdbMHashIndexCtrl.h"
#include "Control/mdbRowCtrl.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbDateTime.h"
 

//namespace QuickMDB{

    

    TMdbMHashIndexCtrl::TMdbMHashIndexCtrl()
    {
        m_pAttachTable = NULL;
        m_pMdbShmDsn = NULL;
        m_pMdbDsn = NULL;
        m_lSelectIndexValue = 0;
    }

    TMdbMHashIndexCtrl::~TMdbMHashIndexCtrl()
    {
    }



    int TMdbMHashIndexCtrl::AttachDsn(TMdbShmDSN * pMdbShmDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pMdbShmDsn);
        m_pMdbShmDsn = pMdbShmDsn;
        CHECK_RET(m_pMdbShmDsn->Attach(),"attach failed...");
        m_pMdbDsn = pMdbShmDsn->GetInfo();
        CHECK_OBJ(m_pMdbDsn);
        return iRet;
    }

    int TMdbMHashIndexCtrl::AttachTable(TMdbShmDSN * pMdbShmDsn,TMdbTable* pTable)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        CHECK_RET(AttachDsn(pMdbShmDsn),"attch dsn failed.");
        m_pAttachTable = pTable;
        return iRet;
    }

	
    int TMdbMHashIndexCtrl::RenameTableIndex(TMdbShmDSN * pMdbShmDsn, TMdbTable* pTable, const char *sNewTableName, int& iFindIndexs)
    {
    	int iRet = 0;
		CHECK_RET(AttachTable(pMdbShmDsn, pTable),"M-hash index attach table failed.");
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
	        for(int j=0; j<MAX_MHASH_INDEX_COUNT && iFindIndexs<pTable->iIndexCounts; ++j)
	        {
	            //比较索引名称,如果相同，则把锁地址和索引地址记录下来
	            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(pTable->sTableName,pMHashBIndexMgr->tIndex[j].sTabName))
	            {
	                ++iFindIndexs;
	                SAFESTRCPY(pMHashBIndexMgr->tIndex[j].sTabName,sizeof(pMHashBIndexMgr->tIndex[j].sTabName),sNewTableName);                    
	                
	            }
	            
	        }
	        
	        if(iFindIndexs == pTable->iIndexCounts)
	        {
	            return iRet;
	        }
    	} 
		
		return iRet;
	}
    int TMdbMHashIndexCtrl::AddTableSingleIndex(TMdbTable * pTable,int iIndexPos,size_t iDataSize)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        TADD_FLOW("AddTableIndex Table=[%s],Size=[%lu] start.", pTable->sTableName,iDataSize);
        
        MDB_INT64 iBaseIndexSize     = pTable->iTabLevelCnts * sizeof(TMdbMHashBaseIndexNode); //计算单个基础索引需要的空间
        int iMutexCount = CalcBaseMutexCount(pTable->iTabLevelCnts);
        MDB_INT64 iMutexSize = iMutexCount *sizeof(TMiniMutex);

        TADD_DETAIL("TAB:[%s],iTabLevelCnts=[%d],iBaseIndexSize=[%d],iMutexSize=[%d],iMutexCount=[%d]",
            pTable->sTableName,pTable->iTabLevelCnts, iBaseIndexSize,iMutexSize,iMutexCount);
        
        ST_MHASH_INDEX_INFO stTableIndex;
        stTableIndex.Clear();
        CHECK_RET(GetMHashFreeBIndexShm(iBaseIndexSize, iDataSize,stTableIndex),"GetFreeBIndexShm failed..");
        CHECK_RET(GetMHashFreeMutexShm(iMutexSize, iDataSize,stTableIndex),"GetMHashFreeMutexShm failed..");
        if('2' == stTableIndex.pBaseIndex->cState)
        {
            //找到空闲位置
            SAFESTRCPY(stTableIndex.pBaseIndex->sName,sizeof(stTableIndex.pBaseIndex->sName),pTable->tIndex[iIndexPos].sName);
            CHECK_RET(InitMHashBaseIndex(stTableIndex,pTable),"InitMHashBaseIndex failed...");
            CHECK_RET(InitMHashMutex(stTableIndex,pTable),"InitMHashMutex failed...");
            SAFESTRCPY(stTableIndex.pMutex->sName, sizeof(stTableIndex.pMutex->sName), pTable->tIndex[iIndexPos].sName);
            CHECK_RET(CreateConflictIndex(stTableIndex),"create conflict index failed.");
            CHECK_RET(CreateLayerIndex(stTableIndex,pTable->tIndex[iIndexPos].iMaxLayer),"create layer index failed...");
            
        }
        else
        {
            CHECK_RET(-1,"not find pos for new index....");
        }
        TADD_FLOW("AddTableIndex : Table=[%s],index[%s] finish.", pTable->sTableName,pTable->tIndex[iIndexPos].sName);
        return iRet;
    }

    int TMdbMHashIndexCtrl::InitMHashMutex(ST_MHASH_INDEX_INFO & tTableIndex,TMdbTable * pTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //初始化基础索引
        SAFESTRCPY(tTableIndex.pMutex->sTabName, sizeof(tTableIndex.pMutex->sTabName), pTable->sTableName);
        tTableIndex.pMutex->cState   = '1';
        tTableIndex.pMutex->iMutexCnt = tTableIndex.pMutex->iSize/sizeof(TMiniMutex);
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pMutex->sCreateTime);
        InitMutexNode((TMiniMutex*)((char *)tTableIndex.pMutexMgr+ tTableIndex.pMutex->iPosAdd),
                      tTableIndex.pMutex->iSize);
        tTableIndex.pMutexMgr->iCounts++;
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbMHashIndexCtrl::InitMHashBaseIndex(ST_MHASH_INDEX_INFO & tTableIndex,TMdbTable * pTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //初始化基础索引
        SAFESTRCPY(tTableIndex.pBaseIndex->sTabName, sizeof(tTableIndex.pBaseIndex->sTabName), pTable->sTableName);
        tTableIndex.pBaseIndex->cState   = '1';
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pBaseIndex->sCreateTime);
        InitBaseIndexNode((TMdbMHashBaseIndexNode *)((char *)tTableIndex.pBIndexMgr + tTableIndex.pBaseIndex->iPosAdd),
                      tTableIndex.pBaseIndex->iSize,false);
        tTableIndex.pBIndexMgr->iIndexCounts ++;
        TADD_FUNC("Finish.");
        return iRet;
    }


    int TMdbMHashIndexCtrl::InitBaseIndexNode(TMdbMHashBaseIndexNode* pNode,MDB_INT64 iSize,bool bList)
    {
        MDB_INT64 iCount = iSize/sizeof(TMdbMHashBaseIndexNode);
        for(MDB_INT64 n=0; n<iCount; ++n)
        {
            pNode->m_tBaseNode.Clear();
            //pNode->m_tBaseNode.iNextPos = -1;
            ++pNode;
        }
        return 0;
    }

    int TMdbMHashIndexCtrl::CreateMHashNewBIndexShm(size_t iShmSize)
    {
        TADD_FUNC("Start.Size=[%lu].",iShmSize);
        int iRet = 0;
        if(MAX_SHM_ID == m_pMdbShmDsn->GetInfo()->iMHashBaseIdxShmCnt)
        {
            CHECK_RET(ERR_OS_NO_MEMROY,"can't create new shm,MAX_SHM_COUNTS[%d]",MAX_SHM_ID);
        }
        int iPos = m_pMdbDsn->iMHashBaseIdxShmCnt;
        TADD_FLOW("Create IndexShm:[%d],base_key[0x%0x]",iPos,m_pMdbDsn->iMHashBaseIdxShmKey[iPos]);
        CHECK_RET(TMdbShm::Create(m_pMdbDsn->iMHashBaseIdxShmKey[iPos], iShmSize, m_pMdbDsn->iMHashBaseIdxShmID[iPos]),
                  " Can't Create BaseIndexShm errno=%d[%s].", errno, strerror(errno));
        TADD_FLOW("Base_SHM_ID =[%d].SHM_SIZE=[%lu].",m_pMdbDsn->iMHashBaseIdxShmID[iPos],iShmSize);
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Create MHASH Base IndexShm:[%d],size=[%luMB]",iPos,iShmSize/(1024*1024));
        m_pMdbDsn->iMHashBaseIdxShmCnt ++;
        CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");//对于新创建的shm获取新映射地址
        //初始化索引区信息
        TMdbMHashBaseIndexMgrInfo* pBaseIndexMgr = (TMdbMHashBaseIndexMgrInfo*)m_pMdbShmDsn->GetMHashBaseIndex(iPos);// ->m_pBaseIndexShmAddr[iPos];
        CHECK_OBJ(pBaseIndexMgr);
        pBaseIndexMgr->iSeq = iPos;
        int i = 0;
        for(i = 0; i<MAX_MHASH_INDEX_COUNT; i++)
        {
            pBaseIndexMgr->tIndex[i].Clear();
            pBaseIndexMgr->tFreeSpace[i].Clear();
        }
        pBaseIndexMgr->tFreeSpace[0].iPosAdd = sizeof(TMdbMHashBaseIndexMgrInfo);
        pBaseIndexMgr->tFreeSpace[0].iSize   = iShmSize - sizeof(TMdbMHashBaseIndexMgrInfo);

        pBaseIndexMgr->iIndexCounts = 0;
        pBaseIndexMgr->iTotalSize   = iShmSize;
        pBaseIndexMgr->tMutex.Create();
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbMHashIndexCtrl::GetMHashFreeMutexShm(MDB_INT64 iMutexSize,size_t iDataSize,ST_MHASH_INDEX_INFO & stTableIndexInfo)
    {
        TADD_FUNC("Start.iMutexSize=[%lld].iDataSize=[%lu]",iMutexSize,iDataSize);
        int iRet = 0;
        if((size_t)iMutexSize > iDataSize - 10*1024*1024)
        {
            //所需空间太大一个内存块都不够放//预留10M空间
            CHECK_RET(-1,"DataSize is[%luM],it's too small,must > [%lldM],please change it",
                      iDataSize/1024/1024, iMutexSize/1024/1024);
        }
        bool bFind = false;
        TMdbMHashMutexMgrInfo* pMutexMgr     = NULL;
        int i = 0;
        for(i = 0; i < MAX_SHM_ID; i++)
        {
            pMutexMgr = (TMdbMHashMutexMgrInfo*)m_pMdbShmDsn->GetMHashMutex(i);
            if(NULL ==  pMutexMgr ) //需要申请新的索引内存
            {
                CHECK_RET(CreateMHashNewMutexShm(iDataSize),"CreateNewBIndexShm[%d]failed",i);
                pMutexMgr = (TMdbMHashMutexMgrInfo*)m_pMdbShmDsn->GetMHashMutex(i);
                CHECK_OBJ(pMutexMgr);
            }
            CHECK_RET(pMutexMgr->tMutex.Lock(true),"Lock Faild");
            do
            {
                int j = 0;
                TMdbMHashBaseMutex* pMutex = NULL;
                //搜寻可以放置索引信息的位置
                for(j = 0; j<MAX_MHASH_INDEX_COUNT; j++)
                {
                    if('0' == pMutexMgr->aBaseMutex[j].cState)
                    {
                        //未创建的
                        pMutex = &(pMutexMgr->aBaseMutex[j]);
                        stTableIndexInfo.pBaseIndex->iMutexMgrPos= i;
                        stTableIndexInfo.pBaseIndex->iMutexPos= j;
                        stTableIndexInfo.iMutexPos= j;
                        break;
                    }
                }
                if(NULL == pMutex)
                {
                    break;   //没有空闲位置可以放索引信息
                }
                //搜寻是否还有空闲内存
                for(j = 0; j<MAX_MHASH_INDEX_COUNT; j++)
                {
                    if(pMutexMgr->tFreeSpace[j].iPosAdd >0)
                    {
                        TMDBIndexFreeSpace& tFreeSpace = pMutexMgr->tFreeSpace[j];
                        if(tFreeSpace.iSize >= (size_t)iMutexSize)
                        {
                            pMutex->cState = '2';//更改状态
                            pMutex->iPosAdd   = tFreeSpace.iPosAdd;
                            pMutex->iSize     = iMutexSize;
                            if(tFreeSpace.iSize - iMutexSize > 0)
                            {
                                //还有剩余空间
                                tFreeSpace.iPosAdd += iMutexSize;
                                tFreeSpace.iSize   -= iMutexSize;
                            }
                            else
                            {
                                //该块空闲空间正好用光
                                tFreeSpace.Clear();
                            }
                            CHECK_RET_BREAK(DefragIndexSpace(pMutexMgr->tFreeSpace),"DefragIndexSpace failed...");
                        }
                    }
                }
                CHECK_RET_BREAK(iRet,"iRet = [%d]",iRet);
                if('2' != pMutex->cState)
                {
                    //没有空闲内存块放索引节点
                    break;
                }
                else
                {
                    //申请到空闲内存块
                    stTableIndexInfo.pMutex= pMutex;
                    stTableIndexInfo.pMutexMgr = pMutexMgr;
                    bFind = true;
                    break;
                }
            }
            while(0);
            CHECK_RET(pMutexMgr->tMutex.UnLock(true),"UnLock Faild");
            if(bFind)
            {
                break;   //找到
            }
        }
        if(!bFind)
        {
            CHECK_RET(-1,"GetMHashFreeMutexShm failed....");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }


    int TMdbMHashIndexCtrl::GetMHashFreeBIndexShm(MDB_INT64 iBaseIndexSize,size_t iDataSize,ST_MHASH_INDEX_INFO & stTableIndexInfo)
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
        TMdbMHashBaseIndexMgrInfo* pBIndexMgr     = NULL;
        int i = 0;
        for(i = 0; i < MAX_SHM_ID; i++)
        {
            pBIndexMgr = (TMdbMHashBaseIndexMgrInfo*)m_pMdbShmDsn->GetMHashBaseIndex(i);
            if(NULL ==  pBIndexMgr ) //需要申请新的索引内存
            {
                CHECK_RET(CreateMHashNewBIndexShm(iDataSize),"CreateNewBIndexShm[%d]failed",i);
                pBIndexMgr = (TMdbMHashBaseIndexMgrInfo*)m_pMdbShmDsn->GetMHashBaseIndex(i);
                CHECK_OBJ(pBIndexMgr);
            }
            CHECK_RET(pBIndexMgr->tMutex.Lock(true),"Lock Faild");
            do
            {
                int j = 0;
                TMdbMHashBaseIndex * pBaseIndex = NULL;
                //搜寻可以放置索引信息的位置
                for(j = 0; j<MAX_MHASH_INDEX_COUNT; j++)
                {
                    if('0' == pBIndexMgr->tIndex[j].cState)
                    {
                        //未创建的
                        pBaseIndex = &(pBIndexMgr->tIndex[j]);
                        stTableIndexInfo.iBaseIndexPos = j;
                        break;
                    }
                }
                if(NULL == pBaseIndex)
                {
                    break;   //没有空闲位置可以放索引信息
                }
                //搜寻是否还有空闲内存
                for(j = 0; j<MAX_MHASH_INDEX_COUNT; j++)
                {
                    if(pBIndexMgr->tFreeSpace[j].iPosAdd >0)
                    {
                        TMDBIndexFreeSpace& tFreeSpace = pBIndexMgr->tFreeSpace[j];
                        if(tFreeSpace.iSize >= (size_t)iBaseIndexSize)
                        {
                            pBaseIndex->cState = '2';//更改状态
                            pBaseIndex->iPosAdd   = tFreeSpace.iPosAdd;
                            pBaseIndex->iSize     = iBaseIndexSize;
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
                            CHECK_RET_BREAK(DefragIndexSpace(pBIndexMgr->tFreeSpace),"DefragIndexSpace failed...");
                        }
                    }
                }
                CHECK_RET_BREAK(iRet,"iRet = [%d]",iRet);
                if('2' != pBaseIndex->cState)
                {
                    //没有空闲内存块放索引节点
                    break;
                }
                else
                {
                    //申请到空闲内存块
                    stTableIndexInfo.pBaseIndex = pBaseIndex;
                    stTableIndexInfo.pBIndexMgr = pBIndexMgr;
                    bFind = true;
                    break;
                }
            }
            while(0);
            CHECK_RET(pBIndexMgr->tMutex.UnLock(true),"UnLock Faild");
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

    int TMdbMHashIndexCtrl::GetFreeConflictShm(MDB_INT64 iConflictSize,size_t iDataSize,TMdbMhashBlock*& pFreeBlock)
    {
        TADD_FUNC("Start.iConfIndexSize=[%lld].iDataSize=[%lu]",iConflictSize,iDataSize);
        int iRet = 0;

        if((size_t)iConflictSize > iDataSize -  10*1024*1024)
        {
            //所需空间太大一个内存块都不够放//预留10M空间
            CHECK_RET(-1,"DataSize is[%luM],it's too small,must > [%lldM],please change it",
                      iDataSize/1024/1024, iConflictSize/1024/1024);
        }

        TMdbMhashBlock* pBlock = NULL;
        TMdbMHashConflictIndexMgrInfo* pMgr =m_pMdbShmDsn->GetMHashConfMgr();
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
			TADD_NORMAL("Conf  Find FreeBlock : %d,next:%d\n",pFreeBlock->iBlockId,pFreeBlock->iNextBlock);
            return iRet;
        }
        else
        {
            int iPos = GetFreeConfPos();
            if(iPos < 0)
            {
                CHECK_RET(iPos,"Can't Create new mhash conflict shm,MAX=[%d].",MAX_MHASH_SHMID_COUNT);
            }
            iRet = TMdbShm::Create(m_pMdbDsn->iMHashConfIdxShmKey[iPos], iConflictSize+8, m_pMdbDsn->iMHashConfIdxShmID[iPos]);
            if(iRet != 0)
            {
                CHECK_RET(iRet,"Can't create mhash conflict shm share memory, errno=%d[%s].",errno, strerror(errno));
            }
            m_pMdbDsn->iMHashConfIdxShmCnt++;
            CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");
            TADD_DETAIL(" iPos=%d, iMHashConfIdxShmID=%d.",iPos,m_pMdbDsn->iMHashConfIdxShmID[iPos]);
            char* pShmAddr = NULL;
            CHECK_RET(m_pMdbShmDsn->InitMHashConfBlock(iPos, pShmAddr),"InitMHashConfBlock failed.");
            
             CHECK_RET(m_pMdbShmDsn->AddNewMhashConflict(pFreeBlock),"add new mhash conflict shm block info to mgrshm failed.");
             CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
             pFreeBlock->iShmID = m_pMdbDsn->iMHashConfIdxShmID[iPos];
             pFreeBlock->iSize = iConflictSize;             
             pFreeBlock->iBlockId = pMgr->iTotalBlocks;
             pMgr->iTotalBlocks++;
             pFreeBlock->tMutex.Create();
			 TADD_NORMAL("Conf  Create FreeBlock : %d,next:%d\n",pFreeBlock->iBlockId,pFreeBlock->iNextBlock);
             CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
            
        }

        return iRet;
    }

    int TMdbMHashIndexCtrl::GetFreeLayerShm(MDB_INT64 iLayerSize,size_t iDataSize,TMdbMhashBlock* & pFreeBlock)
    {
        TADD_FUNC("Start.iLayerSize=[%lld].iDataSize=[%lu]",iLayerSize,iDataSize);
        int iRet = 0;

        if((size_t)iLayerSize > iDataSize -  10*1024*1024)
        {
            //所需空间太大一个内存块都不够放//预留10M空间
            CHECK_RET(-1,"DataSize is[%luM],it's too small,must > [%lldM],please change it",
                      iDataSize/1024/1024, iLayerSize/1024/1024);
        }

        TMdbMhashBlock* pBlock = NULL;
        TMdbMHashLayerIndexMgrInfo* pMgr =m_pMdbShmDsn->GetMHashLayerMgr();
        CHECK_OBJ(pMgr);

        pBlock = GetBlockById(pMgr->iFreeBlockId, false);
        bool bFound = false;
        while(pBlock != NULL)
        {
            if(pBlock->iSize >= iLayerSize)
            {
                bFound = true;
                break;
            }

            pBlock = GetBlockById(pBlock->iNextBlock,false);
        }

        if(bFound)
        {
            CHECK_RET(RemoveBlock(pBlock, pMgr->iFreeBlockId,false),"Remove free layer block failed.");
            pFreeBlock = pBlock;
			TADD_NORMAL("Layer  Find FreeBlock : %d,Next:%d\n",pFreeBlock->iBlockId, pFreeBlock->iNextBlock);
            return iRet;
        }
        else
        {
            int iPos = GetFreeLayerPos();
            if(iPos < 0)
            {
                CHECK_RET(iPos,"Can't Create new mhash layer shm,MAX=[%d].",MAX_MHASH_SHMID_COUNT);
            }
            iRet = TMdbShm::Create(m_pMdbDsn->iMHashLayerIdxShmKey[iPos], iLayerSize+8, m_pMdbDsn->iMHashLayerIdxShmID[iPos]);
            if(iRet != 0)
            {
                CHECK_RET(iRet,"Can't create mhash layer shm share memory, errno=%d[%s].",errno, strerror(errno));
            }
            m_pMdbDsn->iMHashLayerIdxShmCnt++;
            CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");
            TADD_DETAIL(" iPos=%d, iMHashLayerIdxShmID=%d.",iPos,m_pMdbDsn->iMHashLayerIdxShmID[iPos]);
            char* pShmAddr = NULL;
            CHECK_RET(m_pMdbShmDsn->InitMHashLayerBlock(iPos, pShmAddr),"InitMHashLayerBlock failed.");
            
             CHECK_RET(m_pMdbShmDsn->AddNewMhashLayer(pFreeBlock),"add new mhash layer shm block info to mgrshm failed.");
             CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
             pFreeBlock->iShmID = m_pMdbDsn->iMHashLayerIdxShmID[iPos];
             pFreeBlock->iSize = iLayerSize;             
             pFreeBlock->iBlockId = pMgr->iTotalBlocks;
             pMgr->iTotalBlocks++;
             pFreeBlock->tMutex.Create();
			 TADD_NORMAL("Layer  Create FreeBlock : %d,Next:%d\n",pFreeBlock->iBlockId,pFreeBlock->iNextBlock);
             
             CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
            
        }

        return iRet;
    }

    
    

    int TMdbMHashIndexCtrl::DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[])
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int i,j;
        //先按从小到大排序，并且将无用的节点(iposAdd < 0)排到后面
        TMDBIndexFreeSpace tTempSapce;
        for(i = 0; i<MAX_MHASH_INDEX_COUNT; i++)
        {
            for(j = i; j<MAX_MHASH_INDEX_COUNT; j++)
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
        for(i = 0; i<MAX_MHASH_INDEX_COUNT - 1;)
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
                if(i+2 == MAX_MHASH_INDEX_COUNT)
                {
                    tFreeSpace[i+1].Clear();
                }
                else
                {
                    //将后来的节点前移
                    memmove(&(tFreeSpace[i+1]),&(tFreeSpace[i+2]),
                            sizeof(TMDBIndexFreeSpace)*(MAX_MHASH_INDEX_COUNT-i-2));
                    tFreeSpace[MAX_MHASH_INDEX_COUNT - 1].Clear();
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

    int TMdbMHashIndexCtrl::InsertIndexNode(long long iHashValue,ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        long long iBaseIndexPos = iHashValue % m_pAttachTable->iTabLevelCnts;
        TADD_DETAIL("INSERT:[%s][%s] basepos=[%lld],hash[%lld],row[%d]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName, iBaseIndexPos,iHashValue,rowID.m_iRowID);
        
        TMdbMHashBaseIndexNode * pBaseNode = &(tMHashIndex.pBaseIndexNode[iBaseIndexPos]);
        int iMutexPos = iBaseIndexPos % tMHashIndex.pMutex->iMutexCnt;
        TMiniMutex* pMutex = &(tMHashIndex.pMutexNode[iMutexPos]);
        

        CHECK_RET(pMutex->Lock(true),"lock failed.");
        do
        {
            if(pBaseNode->m_tBaseNode.m_tRowId.IsEmpty())
            {
                if(pBaseNode->m_tBaseNode.m_iHashValue > 0 && pBaseNode->m_tBaseNode.m_iHashValue != iHashValue)
                {
                    CHECK_RET_BREAK(InsertToLayerList(iHashValue, 1, &(pBaseNode->m_tBaseNode), tMHashIndex, rowID),"insert into layer list failed.");                    
                }
                else
                {
                    // 插入基础链
                    TADD_DETAIL("insert on Base.[%s][%s][%lld]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,iBaseIndexPos);
                    pBaseNode->m_tBaseNode.m_tRowId = rowID;
                    pBaseNode->m_tBaseNode.m_iHashValue = iHashValue;
                    
                }
                
            }
            else
            {
                if(pBaseNode->m_tBaseNode.m_iHashValue == iHashValue)
                {
                    // 插入冲突链
                    TADD_DETAIL("insert into conflict list.");
                    CHECK_RET_BREAK(InsertConflictNode(pBaseNode->m_tBaseNode, tMHashIndex, rowID),"insert into conflict list failed.");
                }
                else
                {
                    // 插入阶梯链
                    TADD_DETAIL("insert into Layer list.");
                    CHECK_RET_BREAK(InsertToLayerList(iHashValue, 0, &(pBaseNode->m_tBaseNode), tMHashIndex, rowID),"insert into layer list failed.");
                }
            }
        }
        while(0);
        CHECK_RET(pMutex->UnLock(true), "unlock failed.");        
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbMHashIndexCtrl::InsertConflictNode(TMdbMHashIndexNodeInfo& tNodeInfo, ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID)
    {
        TADD_FUNC("start to insert into conflict list.");
        int iRet = 0;
        if(tMHashIndex.pConflictIndex->iFreeHeadPos < 0)
        {
            CHECK_RET(ApplyNewConflictNode(tMHashIndex),"no more free conflict node space.");
        }

        int iFreePos = tMHashIndex.pConflictIndex->iFreeHeadPos;
        TMdbMHashConfIndexNode * pFreeNode = (TMdbMHashConfIndexNode*)GetAddrByIndexNodeId(tMHashIndex.pConflictIndex->iHeadBlockId,iFreePos,sizeof(TMdbMHashConfIndexNode),true);
        CHECK_OBJ(pFreeNode);
        pFreeNode->m_tRowId = rowID;//放入数据
        tMHashIndex.pConflictIndex->iFreeHeadPos = pFreeNode->m_iNextNode;
        pFreeNode->m_iNextNode = tNodeInfo.m_iConfPos;
        tNodeInfo.m_iConfPos = iFreePos;
        tMHashIndex.pConflictIndex->iFreeNodeCounts --;//剩余节点数-1
        
        TADD_DETAIL("insert on conflict[%s][%s][%d]", tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,iFreePos);

        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbMHashIndexCtrl::InsertToLayerList(long long iHashValue,int iCurLayer, TMdbMHashIndexNodeInfo * pLastLayerNode,ST_MHASH_INDEX_INFO& tMHashIndex,TMdbRowID& rowID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
		iCurLayer++;
		
        int iNodeSize = sizeof(TMdbMhashLayerIndexNode);
        if(pLastLayerNode->m_iLayerPos > 0)
        {
            TMdbMhashLayerIndexNode* pLayerNode = (TMdbMhashLayerIndexNode*)GetAddrByIndexNodeId(tMHashIndex.pLayerIndex->iHeadBlockId
                                                                                                                                            , pLastLayerNode->m_iLayerPos , iNodeSize, false);
            CHECK_OBJ(pLayerNode);

            int iLayerPos = TMdbMhashAlgo::CalcLayerPos(iHashValue);
            if(iLayerPos < 0 || iLayerPos >= MAX_BODY_NODE_NUM)
            {
                TADD_ERROR(-1, "invalid layer pos[%dd]", iLayerPos);
                return -1;
            }
            TADD_DETAIL("iLayerPos=[%d]",iLayerPos);

            if(pLayerNode->m_atNode[iLayerPos].m_tRowId.IsEmpty())
            {
                if(pLayerNode->m_atNode[iLayerPos].m_iHashValue > 0 && pLayerNode->m_atNode[iLayerPos].m_iHashValue != iHashValue)
                {
                    CHECK_RET(InsertToLayerList(iHashValue,iCurLayer,&(pLayerNode->m_atNode[iLayerPos]), tMHashIndex, rowID),"insert into next layer node failed.");
                }
                else
                {
                    TADD_DETAIL("insert on layer[%s][%s][%lld][%d].",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,pLastLayerNode->m_iLayerPos,iLayerPos );
                    pLayerNode->m_atNode[iLayerPos].m_tRowId = rowID;
                    pLayerNode->m_atNode[iLayerPos].m_iHashValue = iHashValue;
                    pLayerNode->m_atNode[iLayerPos].m_iLayerPos = -1;
                    pLayerNode->m_atNode[iLayerPos].m_iConfPos=-1;        
                }
                        
            }
            else
            {
                if(pLayerNode->m_atNode[iLayerPos].m_iHashValue == iHashValue)
                {
                    TADD_DETAIL("insert into layer conflict node.");
                    CHECK_RET(InsertConflictNode(pLayerNode->m_atNode[iLayerPos], tMHashIndex,rowID ),"Insert into layer conflict list node failed.");
                }
                else
                {
                    TADD_DETAIL("insert into next layer node.");
                    CHECK_RET(InsertToLayerList(iHashValue,iCurLayer,&(pLayerNode->m_atNode[iLayerPos]), tMHashIndex, rowID),"insert into next layer node failed.");
                }
            }

        }
        else
        {
            // 达到MAX_MHASH_LAYER 最大层数,插入冲突节点
            if(iCurLayer > tMHashIndex.pLayerIndex->iLayerLimit)
            {
                TADD_DETAIL("archive max layer limit.insert into conflict list on layer node.");
                CHECK_RET(InsertConflictNode(*pLastLayerNode, tMHashIndex,rowID ),"Insert into layer conflict list node failed.");
            }
            else
            {
                TADD_DETAIL("insert into new layer .");
                
                if(tMHashIndex.pLayerIndex->iFreeHeadPos < 0)
                {
                    CHECK_RET(ApplyNewLayerNode(tMHashIndex),"no more free layer node space.");
                }

                int iFreePos = tMHashIndex.pLayerIndex->iFreeHeadPos;
                TMdbMhashLayerIndexNode * pFreeNode =(TMdbMhashLayerIndexNode*)GetAddrByIndexNodeId(tMHashIndex.pLayerIndex->iHeadBlockId
                                                                                                                                            , iFreePos , iNodeSize, false);
                CHECK_OBJ(pFreeNode);
                tMHashIndex.pLayerIndex->iFreeHeadPos = pFreeNode->m_iNextPos;
                tMHashIndex.pLayerIndex->iFreeNodeCounts --;//剩余节点数-1      
                
                pLastLayerNode->m_iLayerPos = iFreePos;                  
                TADD_DETAIL("iFreePos=[%d]",iFreePos);
                
                pFreeNode->m_iLayeCnt = iCurLayer;

                int iLayerPos = TMdbMhashAlgo::CalcLayerPos(iHashValue);
                if(iLayerPos < 0 || iLayerPos >= MAX_BODY_NODE_NUM)
                {
                    TADD_ERROR(-1, "invalid layer pos[%d]", iLayerPos);
                    return -1;
                }

                pFreeNode->m_atNode[iLayerPos].m_tRowId = rowID;
                pFreeNode->m_atNode[iLayerPos].m_iHashValue = iHashValue;
                pFreeNode->m_atNode[iLayerPos].m_iLayerPos = -1;
                pFreeNode->m_atNode[iLayerPos].m_iConfPos = -1;       
                TADD_DETAIL("insert on new layer[%s][%s][%d][%d].",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,iFreePos,iLayerPos );   

            }
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }

  

    int TMdbMHashIndexCtrl::ApplyNewConflictNode(ST_MHASH_INDEX_INFO & tTableIndex)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        int iNewCount = m_pAttachTable->iTabLevelCnts;
        int iNodeSize = sizeof(TMdbMHashConfIndexNode);
        long long iConflictSize = iNewCount * iNodeSize;
        TMdbMhashBlock* pBlock = NULL;
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
        
        char* pAddr = m_pMdbShmDsn->GetMhashConfShmAddr(pBlock->iShmID);
        TMdbMHashConfIndexNode* pNode = (TMdbMHashConfIndexNode*)pAddr;
        for(int i = pBlock->iStartNode; i <= pBlock->iEndNode; i++)
        {
            pNode->m_tRowId.Clear();
            //pNode->m_iNextNode = i +1;
            if(i != pBlock->iEndNode)
            {
                pNode->m_iNextNode = i +1;
            }
            else
            {
                pNode->m_iNextNode = -1;
            }
            pNode = (TMdbMHashConfIndexNode*)(pAddr+iNodeSize*(i+1-pBlock->iStartNode)) ;
        }
        //pNode->m_iNextNode = -1;// 结尾
        CHECK_RET(pBlock->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
        
        return iRet;
    }

    int TMdbMHashIndexCtrl::ApplyNewLayerNode(ST_MHASH_INDEX_INFO & tTableIndex)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        int iNewCount = m_pAttachTable->iTabLevelCnts;
        int iNodeSize = sizeof(TMdbMhashLayerIndexNode);
        long long iLayerSize = iNewCount * iNodeSize;
        TMdbMhashBlock* pBlock = NULL;
        CHECK_RET(GetFreeLayerShm(iLayerSize, m_pMdbDsn->m_iDataSize,pBlock), "Get Free Layer index shm failed.");

        CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
        do
        {
            pBlock->iStartNode = tTableIndex.pLayerIndex->iTotalNodes + 1;
            pBlock->iEndNode = tTableIndex.pLayerIndex->iTotalNodes + iNewCount;
            iRet = AddBlock(tTableIndex.pLayerIndex->iHeadBlockId, pBlock,false);
            CHECK_RET_BREAK(iRet,"AddBlock failed.");
            tTableIndex.pLayerIndex->iTotalNodes += iNewCount;
        }while(0);
        CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");

        CHECK_RET(iRet,"init block failed.");

        tTableIndex.pLayerIndex->iFreeHeadPos = pBlock->iStartNode;
        tTableIndex.pLayerIndex->iFreeNodeCounts += iNewCount;
        
        CHECK_RET(pBlock->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock block failed.");
        
        char* pAddr = m_pMdbShmDsn->GetMhashLayerShmAddr(pBlock->iShmID);
        TMdbMhashLayerIndexNode* pNode = (TMdbMhashLayerIndexNode*)pAddr;
        for(int i = pBlock->iStartNode; i <= pBlock->iEndNode; i++)
        {
            for(int j = 0; j< MAX_BODY_NODE_NUM;j++)
            {
                pNode->m_atNode[j].Clear();
            }
            pNode->m_iLayeCnt = 0;
            //pNode->m_iNextPos = i +1;           
            if(i != pBlock->iEndNode)
            {
                pNode->m_iNextPos = i +1;  
            }
            else
            {
                pNode->m_iNextPos = -1;  
            }
            pNode = (TMdbMhashLayerIndexNode*)(pAddr+iNodeSize*(i+1-pBlock->iStartNode)) ;
        }
        //pNode->m_iNextPos = -1;// 结尾
        CHECK_RET(pBlock->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
        
        return iRet;
    }


    int TMdbMHashIndexCtrl::InitMutexNode(TMiniMutex* pNode,MDB_INT64 iSize)
    {
        MDB_INT64 iCount = iSize/sizeof(TMiniMutex);
        for(MDB_INT64 n=0; n<iCount; ++n)
        {
            pNode->Create();
            ++pNode;
        }
        return 0;
    }

    int TMdbMHashIndexCtrl::CreateMHashNewMutexShm(size_t iShmSize)
    {
        TADD_FUNC("Start.Size=[%lu].",iShmSize);
        int iRet = 0;
        if(MAX_SHM_ID == m_pMdbShmDsn->GetInfo()->iMHashMutexShmCnt)
        {
            CHECK_RET(ERR_OS_NO_MEMROY,"can't create new shm,MAX_SHM_COUNTS[%d]",MAX_SHM_ID);
        }
        int iPos = m_pMdbDsn->iMHashMutexShmCnt;
        TADD_FLOW("Create mutexShm:[%d],mutex_key[0x%0x]",iPos,m_pMdbDsn->iMHashMutexShmKey[iPos]);
        CHECK_RET(TMdbShm::Create(m_pMdbDsn->iMHashMutexShmKey[iPos], iShmSize, m_pMdbDsn->iMHashMutexShmID[iPos]),
                  " Can't Create mutexIndexShm errno=%d[%s].", errno, strerror(errno));
        TADD_FLOW("Mutex_SHM_ID =[%d].SHM_SIZE=[%lu].",m_pMdbDsn->iMHashMutexShmID[iPos],iShmSize);
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Create Base IndexShm:[%d],size=[%luMB]",iPos,iShmSize/(1024*1024));
        m_pMdbDsn->iMHashMutexShmCnt ++;
        CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");//对于新创建的shm获取新映射地址
        //初始化索引区信息
        TMdbMHashMutexMgrInfo* pMutexMgr = (TMdbMHashMutexMgrInfo*)m_pMdbShmDsn->GetMHashMutex(iPos);// ->m_pBaseIndexShmAddr[iPos];
        CHECK_OBJ(pMutexMgr);
        pMutexMgr->iSeq = iPos;
        int i = 0;
        for(i = 0; i<MAX_MHASH_INDEX_COUNT; i++)
        {
            pMutexMgr->aBaseMutex[i].Clear();
            pMutexMgr->tFreeSpace[i].Clear();
        }
        pMutexMgr->tFreeSpace[0].iPosAdd = sizeof(TMdbMHashMutexMgrInfo);
        pMutexMgr->tFreeSpace[0].iSize   = iShmSize - sizeof(TMdbMHashMutexMgrInfo);

        pMutexMgr->iCounts= 0;
        pMutexMgr->iTotalSize   = iShmSize;
        pMutexMgr->tMutex.Create();
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbMHashIndexCtrl::DeleteIndexNode(long long iHashValue, ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        long long iBaseIndexPos = iHashValue % m_pAttachTable->iTabLevelCnts;
        TADD_DETAIL("DELETE:[%s][%s] basepos=[%lld],hash[%lld],row[%d]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,iBaseIndexPos,iHashValue,rowID.m_iRowID);
        TMdbMHashBaseIndexNode * pBaseNode = &(tMHashIndex.pBaseIndexNode[iBaseIndexPos]);
        int iMutexPos = iBaseIndexPos % tMHashIndex.pMutex->iMutexCnt;
        TMiniMutex* pMutex = &(tMHashIndex.pMutexNode[iMutexPos]);
        bool bFound = false;
        CHECK_RET(pMutex->Lock(true), "lock failed.");
        do
        {
            if(pBaseNode->m_tBaseNode.m_tRowId == rowID)
            {
                
                TADD_DETAIL("delete on base.[%s][%s][%lld].",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,iBaseIndexPos );
                bFound = true;
                pBaseNode->m_tBaseNode.m_tRowId.Clear();

                // 将冲突链上移一个节点
                if(pBaseNode->m_tBaseNode.m_iConfPos > 0)
                {
                    int iCurPos = pBaseNode->m_tBaseNode.m_iConfPos;
                    int iNodeSize = sizeof(TMdbMHashConfIndexNode);
                    TMdbMHashConfIndexNode* pUpNode = (TMdbMHashConfIndexNode*)GetAddrByIndexNodeId(tMHashIndex.pConflictIndex->iHeadBlockId, iCurPos, iNodeSize, true);
                    CHECK_OBJ(pUpNode);

                    // 上移
                    pBaseNode->m_tBaseNode.m_tRowId.m_iRowID = pUpNode->m_tRowId.m_iRowID;
                    pBaseNode->m_tBaseNode.m_iConfPos = pUpNode->m_iNextNode;
                    
                    // 回收
                    pUpNode->m_iNextNode = tMHashIndex.pConflictIndex->iFreeHeadPos;
                    tMHashIndex.pConflictIndex->iFreeHeadPos = iCurPos;
                    tMHashIndex.pConflictIndex->iFreeNodeCounts++;             
                    TADD_DETAIL("[%s][%s] basepos=[%lld], up conf[%d],new conf[%d]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,iBaseIndexPos,iCurPos,pBaseNode->m_tBaseNode.m_iConfPos);
                }
                
                break;
            }
            else if(iHashValue == pBaseNode->m_tBaseNode.m_iHashValue)
            {
                // 遍历冲突索引
                if(pBaseNode->m_tBaseNode.m_iConfPos > 0)
                {
                    // 遍历基础链上的冲突链
                    TADD_DETAIL("check conflict node on base list.confpos[%d]",pBaseNode->m_tBaseNode.m_iConfPos);
                    CHECK_RET(DeleteIndexNodeOnConfList(tMHashIndex, &pBaseNode->m_tBaseNode, rowID, bFound),"error ! when delete index node on conflict list ");
                }
            }
            
            
            if(!bFound)
            {// 遍历阶梯索引
                TADD_DETAIL("check on base-layer list,layerpos[%d]",pBaseNode->m_tBaseNode.m_iLayerPos);
                if(pBaseNode->m_tBaseNode.m_iLayerPos > 0)
                {// 存在下层阶梯索引
                    CHECK_RET(DeleteIndexNodeOnLayerList(tMHashIndex, &(pBaseNode->m_tBaseNode),iHashValue, rowID, bFound), "error! when check to delete on layer list.");
                }
                else
                {
                    TADD_DETAIL("no base-layer list to check.");
                }
            }

        }
        while(0);
        
        CHECK_RET(pMutex->UnLock(true),"unlock failed.");        

        if(!bFound)
        {
            TADD_WARNING("not find indexnode to delete.[%s][%s][%lld],hash[%d]row[%d]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,iBaseIndexPos,iHashValue,rowID.m_iRowID);
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    

    int TMdbMHashIndexCtrl::DeleteIndexNodeOnConfList
        (ST_MHASH_INDEX_INFO& tMHashIndex,TMdbMHashIndexNodeInfo* pNodeInfo, TMdbRowID& tRowId,bool & bFound)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        TMdbMHashConfIndexNode* pPre = NULL;
        int iNodeSize = sizeof(TMdbMHashConfIndexNode);

        int iCurPos = -1;
        TMdbMHashConfIndexNode* pConfNode = (TMdbMHashConfIndexNode*)GetAddrByIndexNodeId(tMHashIndex.pConflictIndex->iHeadBlockId, pNodeInfo->m_iConfPos, iNodeSize, true);
        CHECK_OBJ(pConfNode);
        iCurPos = pNodeInfo->m_iConfPos;
        TADD_DETAIL("---[%s][%s][%d]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,iCurPos);
        do
        {
            
            if(NULL == pConfNode)
            {
                TADD_DETAIL("node is null ,break.no node deleted.");
                break;
            }

            TADD_DETAIL("------[%s][%s][%d],row[%d]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,iCurPos,pConfNode->m_tRowId.m_iRowID);
            
            if(pConfNode->m_tRowId == tRowId)
            {
                TADD_DETAIL("delete on conflict.[%s][%s][%d]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,iCurPos);
                bFound = true;
                if(pPre == NULL)
                {
                    pNodeInfo->m_iConfPos = pConfNode->m_iNextNode;
                }
                else
                {
                    pPre->m_iNextNode = pConfNode->m_iNextNode;
                }
                
                // add top to free list
                pConfNode->m_iNextNode = tMHashIndex.pConflictIndex->iFreeHeadPos;
                tMHashIndex.pConflictIndex->iFreeHeadPos = iCurPos;
                tMHashIndex.pConflictIndex->iFreeNodeCounts++;

                pConfNode->m_tRowId.Clear();
                
                break;
            }

            if(pConfNode->m_iNextNode < 0)
            {// 冲突链结束
                TADD_DETAIL("not found on conflict list.");
                break;
            }
            else
            {// 查找下一个
                TADD_DETAIL("continue to check next conflict node on list.");
                pPre = pConfNode;
                iCurPos= pPre->m_iNextNode;
                pConfNode =(TMdbMHashConfIndexNode*)GetAddrByIndexNodeId(tMHashIndex.pConflictIndex->iHeadBlockId, pConfNode->m_iNextNode, iNodeSize, true);
            }
            
        }while(1);
                        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbMHashIndexCtrl::DeleteIndexNodeOnLayerList(ST_MHASH_INDEX_INFO& tMHashIndex,TMdbMHashIndexNodeInfo* pNodeInfo, long long iHashValue,TMdbRowID& tRowId,bool & bFound)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iNodeSize = sizeof(TMdbMhashLayerIndexNode);
        TMdbMhashLayerIndexNode* pLayerNode = (TMdbMhashLayerIndexNode*)GetAddrByIndexNodeId(tMHashIndex.pLayerIndex->iHeadBlockId
                              , pNodeInfo->m_iLayerPos , iNodeSize, false);
        
        int iLayerPos = TMdbMhashAlgo::CalcLayerPos(iHashValue);

        if(pLayerNode->m_atNode[iLayerPos].m_tRowId == tRowId)
        {
            TADD_DETAIL("delete on layer.[%s][%s][%lld][%d]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,pNodeInfo->m_iLayerPos,iLayerPos);
            bFound = true;
            pLayerNode->m_atNode[iLayerPos].m_tRowId.Clear();

            // 将冲突链上移一个节点
            if(pLayerNode->m_atNode[iLayerPos].m_iConfPos > 0)
            {
                    int iCurPos = pLayerNode->m_atNode[iLayerPos].m_iConfPos;
                    int iConfNodeSize = sizeof(TMdbMHashConfIndexNode);
                    TMdbMHashConfIndexNode* pUpNode = (TMdbMHashConfIndexNode*)GetAddrByIndexNodeId(tMHashIndex.pConflictIndex->iHeadBlockId, iCurPos, iConfNodeSize, true);
                    CHECK_OBJ(pUpNode);

                    // 上移
                    pLayerNode->m_atNode[iLayerPos].m_tRowId.m_iRowID = pUpNode->m_tRowId.m_iRowID;
                    pLayerNode->m_atNode[iLayerPos].m_iConfPos = pUpNode->m_iNextNode;
                    
                    // 回收
                    pUpNode->m_iNextNode = tMHashIndex.pConflictIndex->iFreeHeadPos;
                    tMHashIndex.pConflictIndex->iFreeHeadPos = iCurPos;
                    tMHashIndex.pConflictIndex->iFreeNodeCounts++;          
                    TADD_DETAIL("[%s][%s] [%lld][%d], up conf[%d],new conf[%d]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName,pNodeInfo->m_iLayerPos,iLayerPos,iCurPos,pLayerNode->m_atNode[iLayerPos].m_iConfPos);
                    
            }
            
        }
        else
        {
            if(pLayerNode->m_atNode[iLayerPos].m_iHashValue == iHashValue)
            {
                // check conflict list.
                if(pLayerNode->m_atNode[iLayerPos].m_iConfPos > 0)
                {
                    TADD_DETAIL("check on layer-conflict list.pLayerNode->m_atNode[%d].m_iConfPos=[%d]",iLayerPos,pLayerNode->m_atNode[iLayerPos].m_iConfPos);
                    CHECK_RET(DeleteIndexNodeOnConfList(tMHashIndex, &(pLayerNode->m_atNode[iLayerPos]), tRowId, bFound), "error! when check to delete on layer-conflict list.");
                }
                else
                {
                    TADD_WARNING( "found same hash value on layernode,but no same rowid. ");
                }
                
                
            }
            else
            {
                if(pLayerNode->m_atNode[iLayerPos].m_iLayerPos > 0)
                {
                    // recursion to check next layer node.
                    TADD_DETAIL("recursion to check next layer node:pLayerNode->m_atNode[%d].m_iLayerPos=[%d]",iLayerPos,pLayerNode->m_atNode[iLayerPos].m_iLayerPos);
                    CHECK_RET(DeleteIndexNodeOnLayerList(tMHashIndex, &(pLayerNode->m_atNode[iLayerPos]),iHashValue, tRowId, bFound), "error! when check to delete on next layer list.");
                }
                else
                {
                    // arrive bottom,check conflict node if exsit.
                    if(pLayerNode->m_atNode[iLayerPos].m_iConfPos > 0)
                    {
                        TADD_DETAIL("arrive bottom,check conflict node if exsit:pLayerNode->m_atNode[%d].m_iConfPos=[%d]",iLayerPos,pLayerNode->m_atNode[iLayerPos].m_iConfPos);
                        CHECK_RET(DeleteIndexNodeOnConfList(tMHashIndex, &(pLayerNode->m_atNode[iLayerPos]), tRowId, bFound), "error! when check to delete on layer-conflict list.");
                    }
                    else
                    {
                        TADD_WARNING("At bottom conflict list ,still no node found to delete.. ");
                    }
                }
            }
        }
        
        
        TADD_FUNC("Finish");
        return iRet;
    }

    int TMdbMHashIndexCtrl::UpdateIndexNode(long long iOldHashValue, long long iNewHashValue,ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID)
    {
        int iRet = 0;
        TADD_DETAIL("UPDATE:[%s][%s]oldvalue[%lld],newvalue=[%lld],row[%d]",tMHashIndex.pBaseIndex->sTabName,tMHashIndex.pBaseIndex->sName, iOldHashValue,  iNewHashValue,rowID.m_iRowID);
        CHECK_RET(DeleteIndexNode(iOldHashValue, tMHashIndex,  rowID),"delete index node failed.");
        CHECK_RET(InsertIndexNode(iNewHashValue, tMHashIndex,  rowID),"insert index node failed.");
        
        return iRet;
    }

    int TMdbMHashIndexCtrl::DeleteTableIndex(ST_MHASH_INDEX_INFO& tIndexInfo)
    {
        int iRet = 0;

        //清理基础索引信息
        CHECK_RET(tIndexInfo.pBIndexMgr->tMutex.Lock(true), "lock failed.");
        do
        {
            CHECK_RET_BREAK(RecycleIndexSpace(tIndexInfo.pBIndexMgr->tFreeSpace,
                                              tIndexInfo.pBaseIndex->iPosAdd,
                                              tIndexInfo.pBaseIndex->iSize),"RecycleIndexSpace failed...");
            tIndexInfo.pBaseIndex->Clear();
            tIndexInfo.pBIndexMgr->iIndexCounts --;//基础索引-1
        }
        while(0);
        CHECK_RET(tIndexInfo.pBIndexMgr->tMutex.UnLock(true),"unlock failed.");
        CHECK_RET(iRet,"ERROR.");

        // 清理基础链锁信息
        CHECK_RET(tIndexInfo.pMutexMgr->tMutex.Lock(true),"lock failed.");
        do
        {
            CHECK_RET_BREAK(RecycleIndexSpace(tIndexInfo.pMutexMgr->tFreeSpace,
                                              tIndexInfo.pMutex->iPosAdd,
                                              tIndexInfo.pMutex->iSize),"RecycleIndexSpace failed...");
            tIndexInfo.pMutex->Clear();
            tIndexInfo.pMutexMgr->iCounts--;
        }
        while(0);
        CHECK_RET(tIndexInfo.pMutexMgr->tMutex.UnLock(true),"unlock failed.");
        CHECK_RET(iRet,"ERROR.");
                
        CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
        do
        {
            // 冲突索引清理
            TMdbMHashConflictIndexMgrInfo* pConfMgr =m_pMdbShmDsn->GetMHashConfMgr();
            if(NULL == pConfMgr)
            {
                break;
            }
            TMdbMhashBlock* pBlock = GetBlockById(tIndexInfo.pConflictIndex->iHeadBlockId, true);
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

            // 阶梯索引清理
            TMdbMHashLayerIndexMgrInfo* pLayerMgr = m_pMdbShmDsn->GetMHashLayerMgr();
            if(NULL == pLayerMgr)
            {
                break;
            }
            TMdbMhashBlock* pLayerBlock = GetBlockById(tIndexInfo.pLayerIndex->iHeadBlockId, false);
            while(pLayerBlock)
            {
                if(AddBlock(pLayerMgr->iFreeBlockId, pLayerBlock, false) < 0)
                {
                    break;
                }

                pLayerBlock = GetBlockById(pLayerBlock->iNextBlock, false);
            }
            tIndexInfo.pLayerIndex->Clear();
            pLayerMgr->iIndexCounts--;
            
        }while(0);
        CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");    

        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbMHashIndexCtrl::TruncateTableIndex(ST_MHASH_INDEX_INFO& tIndexInfo)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		//初始化基础索引
		InitBaseIndexNode((TMdbMHashBaseIndexNode *)((char *)tIndexInfo.pBIndexMgr + tIndexInfo.pBaseIndex->iPosAdd),
		              tIndexInfo.pBaseIndex->iSize,false);

		//重置所有链mutex
		TMiniMutex* pMutex = (TMiniMutex*)((char *)tIndexInfo.pMutexMgr+ tIndexInfo.pMutex->iPosAdd);
		for(MDB_INT64 n=0; n<tIndexInfo.pMutex->iSize/sizeof(TMiniMutex); ++n)
		{
		    CHECK_RET(pMutex->UnLock(true), "unlock failed.");
		    ++pMutex;
		}

		CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
		do
		{
			// 冲突索引清理
			TMdbMHashConflictIndexMgrInfo* pConfMgr =m_pMdbShmDsn->GetMHashConfMgr();
			if(NULL == pConfMgr)
			{
				break;
			}
			TMdbMhashBlock* pBlock = GetBlockById(tIndexInfo.pConflictIndex->iHeadBlockId, true);
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

			// 阶梯索引清理
			TMdbMHashLayerIndexMgrInfo* pLayerMgr = m_pMdbShmDsn->GetMHashLayerMgr();
			if(NULL == pLayerMgr)
			{
				break;
			}
			TMdbMhashBlock* pLayerBlock = GetBlockById(tIndexInfo.pLayerIndex->iHeadBlockId, false);
			while(pLayerBlock)
			{
				int iNextBlockId = pLayerBlock->iNextBlock;
				TADD_NORMAL("Layer  Return Block:%d,next:%d\n",pLayerBlock->iBlockId,iNextBlockId);				
	
				if(AddBlock(pLayerMgr->iFreeBlockId, pLayerBlock, false) < 0)
				{
					break;
				}			
				pLayerBlock = GetBlockById(iNextBlockId, false);
			}
			tIndexInfo.pLayerIndex->iFreeHeadPos = -1;
		    tIndexInfo.pLayerIndex->iFreeNodeCounts = 0;
		    tIndexInfo.pLayerIndex->iTotalNodes = 0;
		    tIndexInfo.pLayerIndex->iHeadBlockId = -1;						
		}while(0);
		CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");	

		TADD_FUNC("Finish.");
		return iRet;
	}

	int TMdbMHashIndexCtrl::OutPutInfo(bool bConsole,const char * fmt, ...)
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

     int TMdbMHashIndexCtrl::PrintIndexInfo(ST_MHASH_INDEX_INFO& stIndexInfo,int iDetialLevel,bool bConsole)
    {
        int iRet = 0;
        int iBICounts = stIndexInfo.pBaseIndex->iSize/sizeof(TMdbMHashBaseIndexNode);//基础索引个数
        int iCICounts = stIndexInfo.pConflictIndex->GetTotalCount();//冲突索引个数
        int iLICounts = stIndexInfo.pLayerIndex->GetTotalCount();//阶梯索引个数
        OutPutInfo(bConsole,"\n\n============[%s]===========\n",stIndexInfo.pBaseIndex->sName);
        OutPutInfo(bConsole,"[BaseIndex] 	 counts=[%d]\n",iBICounts);
        OutPutInfo(bConsole,"[ConfilictIndex] counts=[%d],FreeHeadPos=[%d],FreeNodes=[%d]\n",
                   iCICounts,
                   stIndexInfo.pConflictIndex->iFreeHeadPos,
                   stIndexInfo.pConflictIndex->iFreeNodeCounts);
		OutPutInfo(bConsole,"[LayerIndex] counts=[%d],FreeHeadPos=[%d],FreeNodes=[%d]\n",
                   iLICounts,
                   stIndexInfo.pLayerIndex->iFreeHeadPos,
                   stIndexInfo.pLayerIndex->iFreeNodeCounts);

		
        if(iDetialLevel >0 )
        {//详细信息
           PrintIndexInfoDetail(iDetialLevel,bConsole,stIndexInfo);
        }
        
        return iRet;
    }

	 int TMdbMHashIndexCtrl::PrintIndexInfoDetail(int iDetialLevel,bool bConsole, ST_MHASH_INDEX_INFO & stIndexInfo)
	 {			 
		 int iBICounts = stIndexInfo.pBaseIndex->iSize/sizeof(TMdbMHashBaseIndexNode);//基础索引个数
		 int iTotalDepth = 0;
		 int iTotalConflict = 0;
		 int iMaxDepth = 0;

		 int i = 0;
		 iDetialLevel =  iBICounts>iDetialLevel?iDetialLevel:iBICounts;
		 for(i = 0; i < iDetialLevel; ++i)
		 {
			 TMdbMHashBaseIndexNode * pBaseNode = &(stIndexInfo.pBaseIndexNode[i]);
			 
			 OutPutInfo(true,"BaseNode:%d\n",i);
			 
			 CalcIndexDepthAndConflict(stIndexInfo,pBaseNode->m_tBaseNode.m_iLayerPos,iTotalConflict,iTotalDepth,0);

			 iMaxDepth = MAX_VALUE(iMaxDepth,iTotalDepth);
		 }
		 
		 OutPutInfo(true,"MaxDepth=[%d],TotalConflictOnLayer=[%d]\n",iMaxDepth,iTotalConflict);

		 return 0;
	 }
	 
	 /******************************************************************************
		 * 函数名称  :	CalcIndexDepthAndConflict
		 * 函数描述  :	统计阶梯索引的深度和冲突
		 * 输入输出  :	iTotalConflict - 总的冲突索引数
		 * 输入输出  :	iTotalDepth -当前节点最大深度
		 * 输入 	 :	iCurDepth -当前位置的深度
		 * 返回值	 :	0 - 成功!0 -失败
		 * 作者 	 :	yu.lianxiang
		 *******************************************************************************/
	int TMdbMHashIndexCtrl::CalcIndexDepthAndConflict(ST_MHASH_INDEX_INFO & stIndexInfo,int iPos,int& iTotalConflict,int& iTotalDepth,int iCurDepth)
	{
		int iRet = 0;
	    if (-1 == iPos) return iRet;
		
	    TMdbMhashLayerIndexNode* pLayerNode = (TMdbMhashLayerIndexNode*)GetAddrByIndexNodeId(stIndexInfo.pLayerIndex->iHeadBlockId, iPos, sizeof(TMdbMhashLayerIndexNode), false);

		iCurDepth ++;
		int iArrayDepth[MAX_BODY_NODE_NUM] = {0,0,0,0,0,0,0,0};
		for(int i=0;i<MAX_BODY_NODE_NUM;i++)
		{
			if(pLayerNode->m_atNode[i].m_tRowId.IsEmpty())
            {
               continue;         
            }
			else
			{
			    //统计冲突节点
			    int iConfPos = pLayerNode->m_atNode[i].m_iConfPos;
				int iCurConfNum = 0;
				if( iConfPos>=0 )
				{
					TMdbMHashConfIndexNode * pNode = (TMdbMHashConfIndexNode*)GetAddrByIndexNodeId(stIndexInfo.pConflictIndex->iHeadBlockId,iConfPos,sizeof(TMdbMHashConfIndexNode),true);
					while(NULL!=pNode)
					{
						iTotalConflict++;
						iCurConfNum++;
						pNode=(TMdbMHashConfIndexNode*)GetAddrByIndexNodeId(stIndexInfo.pConflictIndex->iHeadBlockId,pNode->m_iNextNode,sizeof(TMdbMHashConfIndexNode),true);
					}
				}
				
				OutPutInfo(true,"|");
				for(int k=0;k<iCurDepth;k++)
				{
					OutPutInfo(true,"-");
				}
				OutPutInfo(true,"CurDepth=[%d],LayNodeID=[%d],ConflictBranch len=[%d] \n",pLayerNode->m_iLayeCnt,i,iCurConfNum);
				//递归检索子阶梯
				CalcIndexDepthAndConflict(stIndexInfo,pLayerNode->m_atNode[i].m_iLayerPos,iTotalConflict,iArrayDepth[i],iCurDepth);
			}
		}
		
		iTotalDepth = MdbMaxIntArray(iArrayDepth,MAX_BODY_NODE_NUM) +1;
		return iRet;
	}

     int TMdbMHashIndexCtrl::RecycleIndexSpace(TMDBIndexFreeSpace tFreeSpace[],size_t iPosAdd,size_t iSize)
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

    int TMdbMHashIndexCtrl::CreateConflictIndex(ST_MHASH_INDEX_INFO & tTableIndex)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbMHashConflictIndexMgrInfo* pMgr =m_pMdbShmDsn->GetMHashConfMgr();
        CHECK_OBJ(pMgr);

        int iPos = 0;
        bool bFree = false;
        for(int i = 0; i < MAX_MHASH_INDEX_COUNT; i++)
        {
            if('0' != pMgr->tIndex[i].cState){continue;}
            iPos = i;
            bFree = true;
            break;
        }

        if(!bFree)
        {
            TADD_ERROR(-1, "no free pos to create confilict index .");
            return ERR_TAB_INDEX_NUM_EXCEED_MAX;
        }

        tTableIndex.pBaseIndex->iConflictIndexPos=iPos;
        tTableIndex.pConflictIndex = &(pMgr->tIndex[iPos]);
        CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
        tTableIndex.pConflictIndex->Clear();
        tTableIndex.pConflictIndex->cState = '1';
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pConflictIndex->sCreateTime);        
        CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
        
        return iRet;
    }

    int TMdbMHashIndexCtrl::CreateLayerIndex(ST_MHASH_INDEX_INFO & tTableIndex, const int iMaxLayer)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbMHashLayerIndexMgrInfo* pMgr =m_pMdbShmDsn->GetMHashLayerMgr();
        CHECK_OBJ(pMgr);

        int iPos = 0;
        bool bFree = false;
        for(int i = 0; i < MAX_MHASH_INDEX_COUNT; i++)
        {
            if('0' != pMgr->tIndex[i].cState){continue;}
            iPos = i;
            bFree = true;
            break;
        }

        if(!bFree)
        {
            TADD_ERROR(-1, "no free pos to create layer index .");
            return ERR_TAB_INDEX_NUM_EXCEED_MAX;
        }

        tTableIndex.pBaseIndex->iLayerIndexPos =iPos;
        tTableIndex.pLayerIndex = &(pMgr->tIndex[iPos]);
        CHECK_RET(m_pMdbDsn->tMutex.Lock(true, &(m_pMdbDsn->tCurTime)),"lock dsn failed.");
        tTableIndex.pLayerIndex->Clear();
        tTableIndex.pLayerIndex->cState = '1';
        tTableIndex.pLayerIndex->iLayerLimit = iMaxLayer;
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pLayerIndex->sCreateTime);        
        CHECK_RET(m_pMdbDsn->tMutex.UnLock(true, m_pMdbDsn->sCurTime),"unlock failed.");
        
        return iRet;
    }

    char* TMdbMHashIndexCtrl::GetAddrByIndexNodeId(int iHeadBlock,int iIndexNodeId, int iNodeSize, bool bConf)
    {
        TADD_FUNC("Start.");
        TADD_DETAIL("iHeadBlock=%d,iIndexNodeId=%d,iNodeSize=%d, bConf=[%s]"
            ,iHeadBlock,iIndexNodeId, iNodeSize, bConf?"TRUE":"FALSE");
        
        char* pAddr = NULL;

        TMdbMhashBlock* pHeadBlock = GetBlockById(iHeadBlock, bConf);
        if(pHeadBlock == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM," index head block is invalid, head blockid[%d]",iHeadBlock);
            return NULL;
        }
        
        if(iIndexNodeId > 0)
        {
            TMdbMhashBlock* pTmpBlock = pHeadBlock;
            while(pTmpBlock!=NULL)
            {
                TADD_DETAIL("iStartNode=[%d],iEndNode=[%d],iBlockId=[%d],iNextBlock=[%d]",pTmpBlock->iStartNode, pTmpBlock->iEndNode, pTmpBlock->iBlockId,pTmpBlock->iNextBlock);
                if(iIndexNodeId >= pTmpBlock->iStartNode && iIndexNodeId <= pTmpBlock->iEndNode)
                {
                    //计算出相应的地址
                    
                    if(bConf)
                    {
                        pAddr = m_pMdbShmDsn->GetMhashConfShmAddr(pTmpBlock->iShmID);
                    }
                    else
                    {
                        pAddr = m_pMdbShmDsn->GetMhashLayerShmAddr(pTmpBlock->iShmID);
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


    int TMdbMHashIndexCtrl::AddBlock(int& iHeadId, TMdbMhashBlock* pBlockToAdd, bool bConf)
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
            TMdbMhashBlock* pTmp = GetBlockById(iHeadId,bConf);
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


    int TMdbMHashIndexCtrl::RemoveBlock(TMdbMhashBlock* pBlockToDel, int&  iHead, bool bConf)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TADD_DETAIL("BlockToRemove[%d], HeadBlock[%d]", pBlockToDel->iBlockId, iHead);

        if(iHead < 0)
        {
            TADD_ERROR(-1, "head=[%d], can't remove any block", iHead);
            return -1;
        }
        TMdbMhashBlock* pHead = GetBlockById(iHead,bConf);
        CHECK_OBJ(pHead);

        if(pBlockToDel->iBlockId == pHead->iBlockId)
        {
            if(pHead->iNextBlock > 0)
            {
                TMdbMhashBlock* pNext = GetBlockById(pHead->iNextBlock,bConf);
                CHECK_OBJ(pNext);
                iHead = pNext->iBlockId;
            }
            else
            {
                iHead = -1;
            }
            
        }
        else
        {
            TMdbMhashBlock* pPre = pHead;
            TMdbMhashBlock* pCur = pPre;
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
                TMdbMhashBlock* pNext = GetBlockById(pCur->iNextBlock,bConf);
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


    TMdbMhashBlock* TMdbMHashIndexCtrl::GetBlockById(int iBlockID, bool bConf)
    {
        if(bConf)
        {
            return m_pMdbShmDsn->GetMhashConfBlockById( iBlockID);
        }
        else
        {
            return m_pMdbShmDsn->GetMhashLayerBlockById( iBlockID);
        }
    }

    int TMdbMHashIndexCtrl::GetFreeConfPos()
    {
        TADD_FUNC("Start.");
        int i = 0;
        for(; i<MAX_MHASH_SHMID_COUNT; i++)
        {
            if(m_pMdbDsn->iMHashConfIdxShmID[i] == INITVAl)
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

    int TMdbMHashIndexCtrl::GetFreeLayerPos()
    {
        TADD_FUNC("Start.");
        int i = 0;
        for(; i<MAX_MHASH_SHMID_COUNT; i++)
        {
            if(m_pMdbDsn->iMHashLayerIdxShmID[i] == INITVAl)
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

    int TMdbMHashIndexCtrl::CalcBaseMutexCount(int iBaseCont)
    {
        int iRet = 1;// at least 1
				
		if(iBaseCont < 9973)
		{
			iRet = iBaseCont;
		}
		else if(iBaseCont>= 9973 && iBaseCont < 99991)
		{
			iRet = 9973;
		}
		else if(iBaseCont >= 99991)
		{
			iRet = 99991;
		}
        
        return iRet;
    }
    
//}
