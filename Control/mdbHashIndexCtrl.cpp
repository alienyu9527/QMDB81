#include "Control/mdbHashIndexCtrl.h" 
#include "Helper/mdbDateTime.h"
#include "Control/mdbRowCtrl.h"

//namespace QuickMDB{

    /******************************************************************************
    * 函数名称	:  TMdbHashIndexCtrl
    * 函数描述	:  索引控制模块，构造
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbHashIndexCtrl::TMdbHashIndexCtrl():
        m_pAttachTable(NULL),
        m_pMdbShmDsn(NULL),
        m_pMdbDsn(NULL)
    {
        /*int i = 0;
        for(i = 0; i<MAX_INDEX_COUNTS; i++)
        {
            m_arrTableIndex[i].Clear();//清理
        }*/
        m_lSelectIndexValue = -1;
    }

    /******************************************************************************
    * 函数名称	:  ~TMdbHashIndexCtrl
    * 函数描述	:  索引控制模块，析构
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbHashIndexCtrl::~TMdbHashIndexCtrl()
    {

    }
    /******************************************************************************
    * 函数名称	:  SelectIndexNode
    * 函数描述	:  记录正在查询的索引节点
    * 输入		:  iIndexValue - 正在查询的索引节点值
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::SelectIndexNode(MDB_INT64 iIndexValue)
    {
        m_lSelectIndexValue = iIndexValue;
        return 0;
    }

    /******************************************************************************
    * 函数名称	:  DeleteIndexNode
    * 函数描述	:  删除索引节点
    * 输入		:  iIndexPos - 表上的第几条索引
    * 输入		:  iIndexValue -索引节点值
    * 输入		:  rowID -要删除的记录
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::DeleteIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID)
    {
        TADD_FUNC("rowID[%d|%d]",rowID.GetPageID(),rowID.GetDataOffset());
        int iRet = 0;
        CHECK_RET(m_pAttachTable->tTableMutex.Lock(m_pAttachTable->bWriteLock,&m_pMdbDsn->tCurTime),"Lock failed.");
        CHECK_RET(FindRowIndexCValue(tHashIndexInfo, tRowIndex,rowID),"FindRowIndexCValue Failed");//查找冲突索引值
        TMdbIndexNode * pBaseNode = &(tHashIndexInfo.pBaseIndexNode[tRowIndex.iBaseIndexPos]);
        TMdbIndexNode * pDataNode = NULL;
        if(false == tRowIndex.IsCurNodeInConflict())
        {//基础链上
            pDataNode = pBaseNode;
        }
        else
        {//冲突链上
            pDataNode = &(tHashIndexInfo.pConflictIndexNode[tRowIndex.iConflictIndexPos]);
        }
        TADD_DETAIL("pDataNode,rowid[%d|%d],NextPos[%d]",
                            pDataNode->tData.GetPageID(),pDataNode->tData.GetDataOffset(),pDataNode->iNextPos);
        //判断rowid是否正确
        if(pDataNode->tData == rowID)
        {
            if(tRowIndex.iConflictIndexPos < 0)
            {//基础链上直接清理下
                TADD_DETAIL("Row in BaseIndex.");
                pDataNode->tData.Clear();//只做下清理就可以了
            }
            else
            {//冲突链上
                TMdbIndexNode * pPreNode = tRowIndex.IsPreNodeInConflict()?&(tHashIndexInfo.pConflictIndexNode[tRowIndex.iPreIndexPos]):pBaseNode;//获取前置节点
                pPreNode->iNextPos  = pDataNode->iNextPos;// 跳过该节点
                //将节点删除
                pDataNode->iNextPos = tHashIndexInfo.pConflictIndex->iFreeHeadPos;
                tHashIndexInfo.pConflictIndex->iFreeHeadPos = tRowIndex.iConflictIndexPos;
                tHashIndexInfo.pConflictIndex->iFreeNodeCounts ++;//剩余节点数-1
                pDataNode->tData.Clear();
            }
        }
        else
        {//error
            TADD_WARNING("not find indexnode to delete....index[%d|%d],rowid[%d|%d]",
                         tRowIndex.iBaseIndexPos,tRowIndex.iConflictIndexPos,rowID.GetPageID(),rowID.GetDataOffset());
            PrintIndexInfo(tHashIndexInfo,1,false);
        }

        CHECK_RET(m_pAttachTable->tTableMutex.UnLock(m_pAttachTable->bWriteLock),"Unlock failed.");
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  UpdateIndexNode
    * 函数描述	:  更新索引节点
    * 输入		:  iIndexPos - 表上的第几条索引
    * 输入		:  iOldValue - 旧值，iNewValue -新值
    * 输出		:  rowID -要更新的记录
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::UpdateIndexNode(TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId)
    {
        TADD_FUNC("Startrow[%d|%d]",tRowId.GetPageID(),tRowId.GetDataOffset());
        int iRet = 0;

        CHECK_RET(DeleteIndexNode( tOldRowIndex,tHashInfo,tRowId),"DeleteIndexNode failed ,tOldRowIndex[%d|%d],row[%d|%d]",
                  tOldRowIndex.iBaseIndexPos,tOldRowIndex.iConflictIndexPos,tRowId.GetPageID(),tRowId.GetDataOffset());//先删除
        CHECK_RET(InsertIndexNode( tNewRowIndex,tHashInfo,tRowId),"InsertIndexNode failed ,tNewRowIndex[%d|%d],row[%d|%d]",
                  tNewRowIndex.iBaseIndexPos,tNewRowIndex.iConflictIndexPos,tRowId.GetPageID(),tRowId.GetDataOffset());//再增加
                  
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  UpdateIndexNode
    * 函数描述	:  插入索引节点
    * 输入		:  iIndexPos - 表上的第几条索引
    * 输入		:  iIndexValue -索引值
    * 输出		:  rowID -要插入的记录
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::InsertIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndex, TMdbRowID& rowID)
    {
        TADD_FUNC("Start.rowID[%d|%d]",  rowID.GetPageID(),rowID.GetDataOffset());
        int iRet = 0;
        CHECK_RET(m_pAttachTable->tTableMutex.Lock(m_pAttachTable->bWriteLock,&m_pMdbDsn->tCurTime),"Lock failed.");
        TMdbIndexNode * pBaseNode = &(tHashIndex.pBaseIndexNode[tRowIndex.iBaseIndexPos]);
        tRowIndex.iPreIndexPos       = -1;
        if(pBaseNode->tData.IsEmpty())
        {
            //基础索引链上没有值
            TADD_DETAIL("Row can insert BaseIndex.");
            pBaseNode->tData = rowID;
            tRowIndex.iConflictIndexPos = -1;
        }
        else
        {
            TADD_DETAIL("Row can insert ConflictIndex.");
            if(tHashIndex.pConflictIndex->iFreeHeadPos >= 0)
            {
                //还有空闲冲突节点, 对冲突链进行头插
                int iFreePos = tHashIndex.pConflictIndex->iFreeHeadPos;
                TMdbIndexNode * pFreeNode = &(tHashIndex.pConflictIndexNode[iFreePos]);//空闲冲突节点
                pFreeNode->tData = rowID;//放入数据
                tHashIndex.pConflictIndex->iFreeHeadPos = pFreeNode->iNextPos;
                pFreeNode->iNextPos  = pBaseNode->iNextPos;
                pBaseNode->iNextPos  = iFreePos;
                tHashIndex.pConflictIndex->iFreeNodeCounts --;//剩余节点数-1
                tRowIndex.iConflictIndexPos = iFreePos;//在冲突链上某位置
            }
            else
            {
                //没有空闲冲突节点了。
                TADD_ERROR(ERR_TAB_NO_CONFLICT_INDEX_NODE,"No free conflict indexnode.....tRowIndex[%d|%d],rowid[%d|%d],record-counts[%d] of table[%s] is too small",
                           tRowIndex.iBaseIndexPos,tRowIndex.iConflictIndexPos,rowID.GetPageID(),rowID.GetDataOffset(),
                           m_pAttachTable->iRecordCounts,m_pAttachTable->sTableName);
                PrintIndexInfo(tHashIndex,0,false);
                iRet = ERR_TAB_NO_CONFLICT_INDEX_NODE;
            }
        }
        CHECK_RET(m_pAttachTable->tTableMutex.UnLock(m_pAttachTable->bWriteLock),"unlock failed.");
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  FindRowIndexCValue
    * 函数描述	:  查找冲突索引值
    * 输入		:  iIndexPos - 表上的第几条索引
    * 输入		:  tRowIndex -记录所对应的索引值
    * 输出		:  rowID -记录的rowid
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::FindRowIndexCValue(ST_HASH_INDEX_INFO tHashIndexInfo,TMdbRowIndex & tRowIndex,TMdbRowID& rowID)
    {
        TADD_FUNC("rowID[%d|%d]", rowID.GetPageID(),rowID.GetDataOffset());
        int iRet = 0;
        int iCount = 3;//找三次
        bool bFind = false;
        while(1)
        {
            TMdbIndexNode * pBaseNode = &(tHashIndexInfo.pBaseIndexNode[tRowIndex.iBaseIndexPos]);
            TMdbIndexNode * pTempNode = pBaseNode;
            int iCurPos = -1;//当前位置值
            tRowIndex.iPreIndexPos = iCurPos;
            do
            {
                if(pTempNode->tData == rowID)
                {//找到
                    tRowIndex.iConflictIndexPos = iCurPos;
                    bFind = true;
                    break;
                }
                if(pTempNode->iNextPos < 0)
                {//找完，没找到
                    TADD_WARNING("Not find iCount = [%d]",iCount);
                    break;
                }
                else
                {//查找下一个
                    tRowIndex.iPreIndexPos = iCurPos;
                    iCurPos = pTempNode->iNextPos;
                    pTempNode = &(tHashIndexInfo.pConflictIndexNode[pTempNode->iNextPos]);
                }
            }while(1);
            if(iCount <= 0 || bFind){break;}//找完了
            iCount --;
        }
        if(false == bFind)
        {//找了N次都没找到
            CHECK_RET(ERR_DB_INVALID_VALUE,"not find index node rowID[%d|%d]",
                                rowID.GetPageID(),rowID.GetDataOffset());
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  CleanTableIndexInfo
    * 函数描述	:  清理表的索引信息
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*void TMdbHashIndexCtrl::CleanTableIndexInfo()
    {
        int i = 0;
        for(i = 0; i<MAX_INDEX_COUNTS; i++)
        {
            m_arrTableIndex[i].Clear();//清理
        }
    }*/

    /******************************************************************************
    * 函数名称	:  OutPutInfo
    * 函数描述	:  输出信息
    * 输入		:  bConsole - 是否向控制台输出
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::OutPutInfo(bool bConsole,const char * fmt, ...)
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



    /******************************************************************************
    * 函数名称	:  PrintIndexInfo
    * 函数描述	:  打印索引信息
    * 输入		:  iDetialLevel - 详细级别 =0 基本信息，>0 详细信息
    * 输入		:  bConsole    - 是否向控制台输出
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::PrintIndexInfo(ST_HASH_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole)
    {
        int iRet = 0;
        int iBICounts = tIndexInfo.pBaseIndex->iSize/sizeof(TMdbIndexNode);//基础索引个数
        int iCICounts = tIndexInfo.pConflictIndex->iSize/sizeof(TMdbIndexNode);//冲突索引个数
        OutPutInfo(bConsole,"\n\n============[%s]===========\n",tIndexInfo.pBaseIndex->sName);
        OutPutInfo(bConsole,"[BaseIndex] 	 counts=[%d]\n",iBICounts);
        OutPutInfo(bConsole,"[ConfilictIndex] counts=[%d],FreeHeadPos=[%d],FreeNodes=[%d]\n",
                   iCICounts,
                   tIndexInfo.pConflictIndex->iFreeHeadPos,
                   tIndexInfo.pConflictIndex->iFreeNodeCounts);
        if(iDetialLevel >0 )
        {//详细信息
           PrintIndexInfoDetail(iDetialLevel,bConsole,tIndexInfo);
        }
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  PrintIndexInfoDetail
    * 函数描述	:  打印索引信息
    * 输入		:  iDetialLevel - 详细级别 =0 基本信息，>0 详细信息
    * 输入		:  bConsole    - 是否向控制台输出
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::PrintIndexInfoDetail(int iDetialLevel,bool bConsole, ST_HASH_INDEX_INFO & stIndexInfo)
    {
        int arrRange[][3] = {{0,10,0},{10,100,0},{100,500,0},{500,2000,0},{2000,5000,0},{5000,-1,0}};//区间范围，值
        int iRangeCount = sizeof(arrRange)/(3*sizeof(int));
        int iBICounts = stIndexInfo.pBaseIndex->iSize/sizeof(TMdbIndexNode);//基础索引个数
        int iCICounts = stIndexInfo.pConflictIndex->iSize/sizeof(TMdbIndexNode);//冲突索引个数
        int i = 0;
        for(i = 0; i < iBICounts; ++i)
        {//遍历每条链
            int iCount = 0;
            int iNextPos = stIndexInfo.pBaseIndexNode[i].iNextPos;
            iCount = 0;
            while(iNextPos >= 0)
            {
                iCount++;
                if(iCount > iBICounts)
                {
                    OutPutInfo(bConsole,"\nOMG,unlimited loop...\n");
                    return 0;
                }
                iNextPos = stIndexInfo.pConflictIndexNode[iNextPos].iNextPos;
            }
            int j = 0;
            for(j = 0;j < iRangeCount;++j)
            {
                if(iCount > arrRange[j][0] &&( iCount <= arrRange[j][1] || arrRange[j][1] < 0))
                {
                    arrRange[j][2]++;//落在此区间内
                    break;
                }
            }
        }
        OutPutInfo(bConsole,"\nBaseIndex Detail:\n");
        for(i = 0;i< iRangeCount;++i)
        {
            OutPutInfo(bConsole,"[%-6d ~ %-6d] counts = [%-6d]\n",arrRange[i][0],arrRange[i][1],arrRange[i][2]);
        }
        //统计冲突剩余链
        int iNextPos = stIndexInfo.pConflictIndex->iFreeHeadPos;
        int iCount = 0;
        while(iNextPos >= 0)
        {
            iCount++;
            if(iCount > iCICounts)
            {
                OutPutInfo(bConsole,"\nOMG,unlimited loop...iCount[%d],iCICounts[%d]\n",iCount,iCICounts);
                return 0;
            }
            iNextPos = stIndexInfo.pConflictIndexNode[iNextPos].iNextPos;
        }
        OutPutInfo(bConsole,"free conflict nodes = %d\n",iCount);
        if(iDetialLevel > 1)
        {
            int i = 0;
            int iPrintCounts = iDetialLevel<iCICounts?iDetialLevel:iCICounts;
            printf("Conflict node detail:\n");
            for(i = 0;i<iPrintCounts;++i)
            {
                    if(i%10 == 0){printf("\n");}
                    printf("[%d:%d] ",i,/*stIndexInfo.pConflictIndexNode[i].iPrePos,*/stIndexInfo.pConflictIndexNode[i].iNextPos);
            }
            printf("\n");
        }
        return 0;
    }

    /******************************************************************************
    * 函数名称	:  GetBCIndex
    * 函数描述	:  获取基础+ 冲突索引
    * 输入		:
    * 输入		:
    * 输出		:  pBaseNode -一组 基础索引 pConflictNode-一组冲突索引
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::GetBCIndex(TMdbIndexNode ** pBaseNode,TMdbIndexNode ** pConflictNode)
    {
        TADD_FUNC("Start");
        int iRet = 0;
        int i = 0;
        for(i = 0; i<m_pAttachTable->iIndexCounts; i++)
        {
            if(false == m_arrTableIndex[i].bInit)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Table index is not init....");
                return -1;//信息未初始化
            }
            pBaseNode[i] = m_arrTableIndex[i].pBaseIndexNode;
            pConflictNode[i] = m_arrTableIndex[i].pConflictIndexNode;
            TADD_DETAIL("Base[%p],Conflict[%p].",pBaseNode[i],pConflictNode[i]);
        }
        TADD_FUNC("Finish");
        return iRet;
    }*/

    /******************************************************************************
    * 函数名称	:  AttachDsn
    * 函数描述	:  连接上共享内存管理区
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::AttachDsn(TMdbShmDSN * pMdbShmDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pMdbShmDsn);
        m_pMdbShmDsn = pMdbShmDsn;
        CHECK_RET(m_pMdbShmDsn->Attach(),"attach failed...");
        m_pMdbDsn = pMdbShmDsn->GetInfo();
        CHECK_OBJ(m_pMdbDsn);
        return iRet;
    }

    int TMdbHashIndexCtrl::AttachTable(TMdbShmDSN * pMdbShmDsn, TMdbTable* pTable)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        CHECK_RET(AttachDsn(pMdbShmDsn),"attch dsn failed.");
        m_pAttachTable = pTable;
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  AttachTable
    * 函数描述	:  连接上表，/获取基础+ 冲突索引信息
    * 输入		:  pMdbShmDsn - DSN内存区
    * 输入		:  pTable    - 表信息
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::AttachTable(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable)
    {
        int iRet = 0;
        TADD_FUNC("AttachTable(%s) : Start.", pTable->sTableName);
        int iFindIndexs = 0;
        m_pAttachTable = pTable;
        CleanTableIndexInfo();//清理
        //遍历所有的基础索引块
        for(int n=0; n<MAX_SHM_ID; ++n)
        {
            TADD_DETAIL("SetIndex(%s) : Shm-ID no.[%d].", pTable->sTableName, n);
            char * pBaseIndexAddr = pMdbShmDsn->GetBaseIndex(n);
            if(pBaseIndexAddr == NULL)
                continue;

            TMdbBaseIndexMgrInfo *pBIndexMgr = (TMdbBaseIndexMgrInfo*)pBaseIndexAddr;//获取基础索引内容
            for(int i=0; i<pTable->iIndexCounts; ++i)
            {
                TADD_DETAIL("Need--pTable->tIndex[%d].sName=[%s].", i, pTable->tIndex[i].sName);
                for(int j=0; j<MAX_BASE_INDEX_COUNTS; ++j)
                {
                    //比较索引名称,如果相同，则把锁地址和索引地址记录下来
                    if((0 == TMdbNtcStrFunc::StrNoCaseCmp(pTable->sTableName, pBIndexMgr->tIndex[j].sTabName))
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
                        m_arrTableIndex[i].pBIndexMgr = pBIndexMgr;
                        m_arrTableIndex[i].pCIndexMgr = pCIndexMgr;

                        m_arrTableIndex[i].iBaseIndexPos = j;
                        m_arrTableIndex[i].iConflictIndexPos = pBIndexMgr->tIndex[j].iConflictIndexPos;

                        m_arrTableIndex[i].pBaseIndex = &(pBIndexMgr->tIndex[j]);
                        m_arrTableIndex[i].pConflictIndex = &(pCIndexMgr->tIndex[pBIndexMgr->tIndex[j].iConflictIndexPos]);

                        m_arrTableIndex[i].pBaseIndexNode = (TMdbIndexNode*)(pBaseIndexAddr + m_arrTableIndex[i].pBaseIndex->iPosAdd);
                        m_arrTableIndex[i].pConflictIndexNode = (TMdbIndexNode*)(pConflictIndexAddr + m_arrTableIndex[i].pConflictIndex->iPosAdd);
                        ++iFindIndexs;
                        break;
                    }
                    //TADD_DETAIL("TMdbTableCtrl::SetIndex() :iFindIndexs=%d.", iFindIndexs);
                }
            }
            TADD_DETAIL("iFindIndexs=%d.pTable->iIndexCounts=%d", iFindIndexs,pTable->iIndexCounts);
            if(iFindIndexs == pTable->iIndexCounts)
            {
                break;
            }
        }

        if(iFindIndexs < pTable->iIndexCounts)
        {
            TADD_ERROR(ERR_TAB_INDEX_NOT_EXIST,"AttachTable(%s) : iFindIndexs[%d] < pTable->iIndexCounts[%d].",pTable->sTableName,iFindIndexs, pTable->iIndexCounts);
            return ERR_TAB_INDEX_NOT_EXIST;
        }
        TADD_FUNC("AttachTable(%s) : Finish.", pTable->sTableName);
        return iRet;
    }*/
    /******************************************************************************
    * 函数名称	:  InitIndexNode
    * 函数描述	:  初始化索引节点
    * 输入		:  pNode - 索引的头指针
    * 输入		:  iSize   - 索引大小 bList -是否要连成串
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::InitIndexNode(TMdbIndexNode* pNode,MDB_INT64 iSize,bool bList)
    {
        MDB_INT64 iCount = iSize/sizeof(TMdbIndexNode);
        if(bList)
        {
            //需要连成双链表状态
            pNode[iCount - 1].iNextPos = -1;//结尾
            for(MDB_INT64 n=0; n<iCount-1; ++n)
            {
                pNode->tData.Clear();
                pNode->iNextPos = n + 1;//连成串
                //pNode->iPrePos   =  - 1;
                ++pNode;
            }
        }
        else
        {
            for(MDB_INT64 n=0; n<iCount; ++n)
            {
                pNode->tData.Clear();
                pNode->iNextPos = -1;
                //pNode->iPrePos   = -1;
                ++pNode;
            }
        }
        return 0;
    }

    /******************************************************************************
    * 函数名称	:  CreateNewBIndexShm
    * 函数描述	:  创建新的基础索引内存块
    * 输入		:  iShmSize - 内存块大小
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::CreateNewBIndexShm(size_t iShmSize)
    {
        TADD_FUNC("Start.Size=[%lu].",iShmSize);
        int iRet = 0;
        if(MAX_SHM_ID == m_pMdbShmDsn->GetInfo()->iBaseIndexShmCounts)
        {
            CHECK_RET(ERR_OS_NO_MEMROY,"can't create new shm,MAX_SHM_COUNTS[%d]",MAX_SHM_ID);
        }
        int iPos = m_pMdbDsn->iBaseIndexShmCounts;
        TADD_FLOW("Create IndexShm:[%d],base_key[0x%0x]",iPos,m_pMdbDsn->iBaseIndexShmKey[iPos]);
        CHECK_RET(TMdbShm::Create(m_pMdbDsn->iBaseIndexShmKey[iPos], iShmSize, m_pMdbDsn->iBaseIndexShmID[iPos]),
                  " Can't Create BaseIndexShm errno=%d[%s].", errno, strerror(errno));
        TADD_FLOW("Base_SHM_ID =[%d].SHM_SIZE=[%lu].",m_pMdbDsn->iBaseIndexShmID[iPos],iShmSize);
        TADD_NORMAL("Create Base IndexShm:[%d],size=[%luMB]",iPos,iShmSize/(1024*1024));
        m_pMdbDsn->iBaseIndexShmCounts ++;
        CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");//对于新创建的shm获取新映射地址
        //初始化索引区信息
        TMdbBaseIndexMgrInfo* pBaseIndexMgr = (TMdbBaseIndexMgrInfo*)m_pMdbShmDsn->GetBaseIndex(iPos);// ->m_pBaseIndexShmAddr[iPos];
        CHECK_OBJ(pBaseIndexMgr);
        pBaseIndexMgr->iSeq = iPos;
        int i = 0;
        for(i = 0; i<MAX_BASE_INDEX_COUNTS; i++)
        {
            pBaseIndexMgr->tIndex[i].Clear();
            pBaseIndexMgr->tFreeSpace[i].Clear();
        }
        pBaseIndexMgr->tFreeSpace[0].iPosAdd = sizeof(TMdbBaseIndexMgrInfo);
        pBaseIndexMgr->tFreeSpace[0].iSize   = iShmSize - sizeof(TMdbBaseIndexMgrInfo);

        pBaseIndexMgr->iIndexCounts = 0;
        pBaseIndexMgr->iTotalSize   = iShmSize;
        pBaseIndexMgr->tMutex.Create();
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  CreateNewCIndexShm
    * 函数描述	:  创建新的冲突索引内存块
    * 输入		:  iShmSize - 内存块大小
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::CreateNewCIndexShm(size_t iShmSize)
    {
        TADD_FUNC("Start.size=[%lu].",iShmSize);
        int iRet = 0;
        if(MAX_SHM_ID == m_pMdbShmDsn->GetInfo()->iConflictIndexShmCounts)
        {
            CHECK_RET(ERR_OS_NO_MEMROY,"can't create new shm,MAX_SHM_COUNTS[%d]",MAX_SHM_ID);
        }
        int iPos = m_pMdbDsn->iConflictIndexShmCounts;//pos
        TADD_FLOW("Create IndexShm:[%d],conflict_key[0x%0x]",iPos,m_pMdbDsn->iConflictIndexShmKey[iPos]);
        CHECK_RET(TMdbShm::Create(m_pMdbDsn->iConflictIndexShmKey[iPos], iShmSize, m_pMdbDsn->iConflictIndexShmID[iPos]),
                  " Can't Create ConflictIndexShm, errno=%d[%s].", errno, strerror(errno));
        TADD_FLOW("Conflict_SHM_ID = [%d],SHM_SIZE=[%lu]",m_pMdbDsn->iConflictIndexShmID[iPos],iShmSize);
        TADD_NORMAL("Create Conflict IndexShm:[%d],size=[%luMB]",iPos,iShmSize/(1024*1024));
        m_pMdbDsn->iConflictIndexShmCounts ++;
        CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");//对于新创建的shm获取新映射地址
        //初始化索引区信息
        TMdbConflictIndexMgrInfo* pConflictIndexMgr = (TMdbConflictIndexMgrInfo*)m_pMdbShmDsn->GetConflictIndex(iPos);//m_pConflictIndexShmAddr[iPos];
        CHECK_OBJ(pConflictIndexMgr);
        pConflictIndexMgr->iSeq = iPos;
        int i = 0;
        for(i = 0; i<MAX_BASE_INDEX_COUNTS; i++)
        {
            pConflictIndexMgr->tIndex[i].Clear();
            pConflictIndexMgr->tFreeSpace[i].Clear();
        }
        pConflictIndexMgr->tFreeSpace[0].iPosAdd = sizeof(TMdbConflictIndexMgrInfo);
        pConflictIndexMgr->tFreeSpace[0].iSize   = iShmSize - sizeof(TMdbConflictIndexMgrInfo);
        //pConflictIndexMgr->iLeftAdd = sizeof(TMdbConflictIndexMgrInfo);
        pConflictIndexMgr->iIndexCounts = 0;
        pConflictIndexMgr->iTotalSize   = iShmSize;
        pConflictIndexMgr->tMutex.Create();
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  InitBCIndex
    * 函数描述	:  初始化基础/冲突索引
    * 输入		: pTable - 表信息
    * 输入		:
    * 输出		:   tTableIndex - 表所对应的索引信息
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::InitBCIndex(ST_HASH_INDEX_INFO & tTableIndex,TMdbTable * pTable)
    {
        int iRet = 0;
        //初始化基础索引
        SAFESTRCPY(tTableIndex.pBaseIndex->sTabName, sizeof(tTableIndex.pBaseIndex->sTabName), pTable->sTableName);
        
        tTableIndex.pBaseIndex->cState   = '1';
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pBaseIndex->sCreateTime);
        InitIndexNode((TMdbIndexNode *)((char *)tTableIndex.pBIndexMgr + tTableIndex.pBaseIndex->iPosAdd),
                      tTableIndex.pBaseIndex->iSize,false);
        tTableIndex.pBIndexMgr->iIndexCounts ++;
        //初始化冲突索引
        tTableIndex.pConflictIndex->cState = '1';
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pConflictIndex->sCreateTime);
        InitIndexNode((TMdbIndexNode *)((char *)tTableIndex.pCIndexMgr + tTableIndex.pConflictIndex->iPosAdd),
                      tTableIndex.pConflictIndex->iSize,true);
        tTableIndex.pConflictIndex->iFreeHeadPos = 0;//空闲结点位置
        tTableIndex.pConflictIndex->iFreeNodeCounts = tTableIndex.pConflictIndex->iSize/sizeof(TMdbIndexNode);//记录空闲冲突节点个数
        tTableIndex.pCIndexMgr->iIndexCounts ++;

        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  GetFreeBIndexShm
    * 函数描述	:  获取空闲基础索引块，只做获取不做任何修改
    * 输入		:  iBaseIndexSize -- 基础索引大小
    * 输入		:  iDataSize  --  一个共享内存块大小
    * 输出		:  stTableIndexInfo  -- 获取到的索引信息
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::GetFreeBIndexShm(MDB_INT64 iBaseIndexSize,size_t iDataSize,
                                        ST_HASH_INDEX_INFO & stTableIndexInfo)
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
        TMdbBaseIndexMgrInfo* pBIndexMgr     = NULL;
        int i = 0;
        for(i = 0; i < MAX_SHM_ID; i++)
        {
            pBIndexMgr = (TMdbBaseIndexMgrInfo*)m_pMdbShmDsn->GetBaseIndex(i);
            if(NULL ==  pBIndexMgr ) //需要申请新的索引内存
            {
                CHECK_RET(CreateNewBIndexShm(iDataSize),"CreateNewBIndexShm[%d]failed",i);
                pBIndexMgr = (TMdbBaseIndexMgrInfo*)m_pMdbShmDsn->GetBaseIndex(i);
                CHECK_OBJ(pBIndexMgr);
            }
            CHECK_RET(pBIndexMgr->tMutex.Lock(true),"Lock failed.");
            do
            {
                int j = 0;
                TMdbBaseIndex * pBaseIndex = NULL;
                //搜寻可以放置索引信息的位置
                for(j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
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
                for(j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
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
            CHECK_RET(pBIndexMgr->tMutex.UnLock(true),"unlock failed.");
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
    /******************************************************************************
    * 函数名称	:  GetFreeCIndexShm
    * 函数描述	:  获取空闲冲突索引内存块，只做获取不做任何修改
    * 输入		:  iConflictIndexSize -- 冲突索引大小
    * 输入		:  iDataSize  --  一个共享内存块大小
    * 输出		:  stTableIndexInfo  -- 获取到的索引信息
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::GetFreeCIndexShm(MDB_INT64 iConflictIndexSize,size_t iDataSize,
                                        ST_HASH_INDEX_INFO & stTableIndexInfo)
    {
        TADD_FUNC("Start.iConflictIndexSize=[%lld].iDataSize=[%lld]",iConflictIndexSize,iDataSize);
        int iRet = 0;
        if((size_t)iConflictIndexSize > iDataSize - 10*1024*1024)
        {
            //所需空间太大一个内存块都不够放//预留10M空间
            CHECK_RET(-1,"DataSize is[%luM],it's too small,must > [%lldM],please change it",
                      iDataSize/1024/1024, iConflictIndexSize/1024/1024);
        }
        bool bFind = false;
        TMdbConflictIndexMgrInfo* pCIndexMgr     = NULL;
        int i = 0;
        for(i = 0; i < MAX_SHM_ID; i++)
        {
            pCIndexMgr = (TMdbConflictIndexMgrInfo*)m_pMdbShmDsn->GetConflictIndex(i);
            if(NULL ==  pCIndexMgr ) //需要申请新的索引内存
            {
                CHECK_RET(CreateNewCIndexShm(iDataSize),"CreateNewCIndexShm[%d]failed",i);
                pCIndexMgr = (TMdbConflictIndexMgrInfo*)m_pMdbShmDsn->GetConflictIndex(i);
                CHECK_OBJ(pCIndexMgr);
            }
            CHECK_RET(pCIndexMgr->tMutex.Lock(true),"lock failed.");
            do
            {
                int j = 0;
                TMdbConflictIndex * pConflictIndex = NULL;
                //搜寻可以放置索引信息的位置
                for(j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
                {
                    if('0' == pCIndexMgr->tIndex[j].cState)
                    {
                        //未创建的
                        pConflictIndex = &(pCIndexMgr->tIndex[j]);
                        stTableIndexInfo.pBaseIndex->iConflictMgrPos   = i;
                        stTableIndexInfo.pBaseIndex->iConflictIndexPos = j;
                        stTableIndexInfo.iConflictIndexPos = j;
                        break;
                    }
                }
                if(NULL == pConflictIndex)
                {
                    break;   //没有空闲位置可以放索引信息
                }
                //搜寻是否还有空闲内存
                for(j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
                {
                    if(pCIndexMgr->tFreeSpace[j].iPosAdd >0)
                    {
                        TMDBIndexFreeSpace & tFreeSpace = pCIndexMgr->tFreeSpace[j];
                        if(tFreeSpace.iSize >=(size_t) iConflictIndexSize)
                        {
                            pConflictIndex->cState = '2';//更改状态
                            pConflictIndex->iPosAdd   = tFreeSpace.iPosAdd;
                            pConflictIndex->iSize     = iConflictIndexSize;
                            if(tFreeSpace.iSize - iConflictIndexSize > 0)
                            {
                                //还有剩余空间
                                tFreeSpace.iPosAdd += iConflictIndexSize;
                                tFreeSpace.iSize   -= iConflictIndexSize;
                            }
                            else
                            {
                                //该块空闲空间正好用光
                                tFreeSpace.Clear();
                            }
                            CHECK_RET_BREAK(DefragIndexSpace(pCIndexMgr->tFreeSpace),"DefragIndexSpace failed...");
                        }
                    }
                }
                CHECK_RET_BREAK(iRet,"iRet = [%d].",iRet);
                if('2' != pConflictIndex->cState)
                {
                    //没有空闲内存块放索引节点
                    break;
                }
                else
                {
                    //申请到空闲内存块
                    stTableIndexInfo.pConflictIndex = pConflictIndex;
                    stTableIndexInfo.pCIndexMgr     = pCIndexMgr;
                    bFind = true;
                    break;
                }
            }while(0);
            CHECK_RET(pCIndexMgr->tMutex.UnLock(true),"unlock failed.");
            if(bFind)
            {
                break;   //找到
            }
        }
        if(!bFind)
        {
            CHECK_RET(-1,"GetFreeCIndexShm failed....");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  AddTableIndex
    * 函数描述	:  添加索引
    * 输入		:  pTable - 表信息
    * 输入		:  iDataSize - 最大可申请的内存区大小
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::AddTableIndex(TMdbTable * pTable,size_t iDataSize)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        TADD_FLOW("AddTableIndex Table=[%s],Size=[%lu] start.", pTable->sTableName,iDataSize);
        MDB_INT64 iBaseIndexSize     = pTable->iRecordCounts * sizeof(TMdbIndexNode); //计算单个基础索引需要的空间
        MDB_INT64 iConflictIndexSize = pTable->iRecordCounts * sizeof(TMdbIndexNode); //计算单个冲突索引需要的空间
        ST_TABLE_INDEX_INFO stTableIndex;
        for(int i=0; i<pTable->iIndexCounts; ++i)
        {
            stTableIndex.Clear();
            CHECK_RET(GetFreeBIndexShm(iBaseIndexSize, iDataSize,stTableIndex),"GetFreeBIndexShm failed..");
            CHECK_RET(GetFreeCIndexShm(iConflictIndexSize, iDataSize,stTableIndex),"GetFreeCIndexShm failed...");
            if('2' == stTableIndex.pBaseIndex->cState)
            {
                //找到空闲位置
                SAFESTRCPY(stTableIndex.pBaseIndex->sName,sizeof(stTableIndex.pBaseIndex->sName),pTable->tIndex[i].sName);
                CHECK_RET(InitBCIndex(stTableIndex,pTable),"InitBCIndex failed...");
            }
            else
            {
                CHECK_RET(-1,"not find pos for new index....");
            }
            TADD_FLOW("Index[%s]",pTable->tIndex[i].sName);
        }
        TADD_FLOW("AddTableIndex : Table=[%s] finish.", pTable->sTableName);
        return iRet;
    }*/

    int TMdbHashIndexCtrl::AddTableSingleIndex(TMdbTable * pTable,int iIndexPos, size_t iDataSize)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        TADD_FLOW("AddTableIndex Table=[%s],Size=[%lu] start.", pTable->sTableName,iDataSize);
        MDB_INT64 iBaseIndexSize     = pTable->iRecordCounts * sizeof(TMdbIndexNode); //计算单个基础索引需要的空间
        MDB_INT64 iConflictIndexSize = pTable->iRecordCounts * sizeof(TMdbIndexNode); //计算单个冲突索引需要的空间
        ST_HASH_INDEX_INFO stTableIndex;
        stTableIndex.Clear();
        CHECK_RET(GetFreeBIndexShm(iBaseIndexSize, iDataSize,stTableIndex),"GetFreeBIndexShm failed..");
        CHECK_RET(GetFreeCIndexShm(iConflictIndexSize, iDataSize,stTableIndex),"GetFreeCIndexShm failed...");
        if('2' == stTableIndex.pBaseIndex->cState)
        {
            //找到空闲位置
            SAFESTRCPY(stTableIndex.pBaseIndex->sName,sizeof(stTableIndex.pBaseIndex->sName),pTable->tIndex[iIndexPos].sName);
            CHECK_RET(InitBCIndex(stTableIndex,pTable),"InitBCIndex failed...");
        }
        else
        {
            CHECK_RET(-1,"not find pos for new index....");
        }
        TADD_FLOW("Index[%s]",pTable->tIndex[iIndexPos].sName);
        TADD_FLOW("AddTableIndex : Table=[%s] finish.", pTable->sTableName);
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  CreateAllIndex
    * 函数描述	:  创建所有索引shm ，基础索引+ 冲突索引
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::CreateAllIndex(TMdbConfig &config)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_pMdbShmDsn = TMdbShmMgr::GetShmDSN(config.GetDSN()->sName);
        m_pMdbDsn = m_pMdbShmDsn->GetInfo();
        CHECK_OBJ(m_pMdbShmDsn);
        CHECK_OBJ(m_pMdbDsn);
        m_pMdbDsn->iBaseIndexShmCounts = m_pMdbDsn->iConflictIndexShmCounts = 0;//没有counts
        int i = 0;
        TMdbTable * pTable = NULL;
        TMdbTable tTempTable;
        for( i = 0; i<MAX_TABLE_COUNTS; i++)
        {
            //遍历所有table
            pTable  = config.GetTableByPos(i);//m_pTable + i;
            if(NULL != pTable)
            {
                memcpy(&tTempTable,pTable,sizeof(tTempTable));
                tTempTable.ResetRecordCounts();//加载到内存中时记录数发生了改变
                CHECK_RET(AddTableIndex(&tTempTable,config.GetDSN()->iDataSize),"AddTableIndex failed...");
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }*/

    /******************************************************************************
    * 函数名称	:  GetTableByIndexName
    * 函数描述	:  根据索引名获取表
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::GetTableByIndexName(const char * sIndexName,TMdbTable * & pRetTable)
    {
        TADD_FUNC("Start.sIndexName=[%s]",sIndexName);
        int iRet = 0;
        pRetTable = NULL;
        CHECK_OBJ(m_pMdbShmDsn);
        int j = 0;
        TShmList<TMdbTable>::iterator itor = m_pMdbShmDsn->m_TableList.begin();
        for(;itor != m_pMdbShmDsn->m_TableList.end();++itor)
        {
            TMdbTable * pTable = &(*itor);
            if(pTable->sTableName[0] == 0){continue;}
            // 遍历表，找出索引名所对应的表
            for(j = 0; j<pTable->iIndexCounts; j++)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tIndex[j].sName,sIndexName) == 0)
                {
                    pRetTable = pTable;
                    return iRet;
                }
            }
        }
        TADD_FUNC("Finish.pRetTable=[%p].",pRetTable);
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  DeleteTableIndex
    * 函数描述	:  删除某个表的索引
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::DeleteTableIndex(ST_HASH_INDEX_INFO& tIndexInfo)
    {
        int iRet = 0;

        //清理基础索引信息
        CHECK_RET(tIndexInfo.pBIndexMgr->tMutex.Lock(true),"lock failed.");
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
        //清理冲突索引信息
        CHECK_RET(tIndexInfo.pCIndexMgr->tMutex.Lock(true),"lock failed.");
        do
        {
            CHECK_RET_BREAK(RecycleIndexSpace(tIndexInfo.pCIndexMgr->tFreeSpace,
                                              tIndexInfo.pConflictIndex->iPosAdd,
                                              tIndexInfo.pConflictIndex->iSize),"RecycleIndexSpace failed...");
            tIndexInfo.pConflictIndex->Clear();
            tIndexInfo.pCIndexMgr->iIndexCounts --;//冲突索引-1
        }
        while(0);
        CHECK_RET(tIndexInfo.pCIndexMgr->tMutex.UnLock(true),"unlock failed.");
        
        TADD_FUNC("Finish.");
        return iRet;
    }

	/******************************************************************************
    * 函数名称	:  TruncateTableIndex
    * 函数描述	:  删除某个表的索引
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::TruncateTableIndex(ST_HASH_INDEX_INFO& tIndexInfo)
    {
        int iRet = 0;
		//初始化基础索引
		InitIndexNode((TMdbIndexNode *)((char *)tIndexInfo.pBIndexMgr + tIndexInfo.pBaseIndex->iPosAdd),
					  tIndexInfo.pBaseIndex->iSize,false);
		//初始化冲突索引
		InitIndexNode((TMdbIndexNode *)((char *)tIndexInfo.pCIndexMgr + tIndexInfo.pConflictIndex->iPosAdd),
					  tIndexInfo.pConflictIndex->iSize,true);
		tIndexInfo.pConflictIndex->iFreeHeadPos = 0;//空闲结点位置
		tIndexInfo.pConflictIndex->iFreeNodeCounts = tIndexInfo.pConflictIndex->iSize/sizeof(TMdbIndexNode);//记录空闲冲突节点个数

        TADD_FUNC("Finish.");
        return iRet;
    }

    /*int TMdbHashIndexCtrl::DeleteTableSpecifiedIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,const char* pIdxName)
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
                //清理基础索引信息
                m_arrTableIndex[i].pBIndexMgr->tMutex.Lock(true);
                do
                {
                    CHECK_RET_BREAK(RecycleIndexSpace(m_arrTableIndex[i].pBIndexMgr->tFreeSpace,
                                                      m_arrTableIndex[i].pBaseIndex->iPosAdd,
                                                      m_arrTableIndex[i].pBaseIndex->iSize),"RecycleIndexSpace failed...");
                    m_arrTableIndex[i].pBaseIndex->Clear();
                    m_arrTableIndex[i].pBIndexMgr->iIndexCounts --;//基础索引-1
                }
                while(0);
                m_arrTableIndex[i].pBIndexMgr->tMutex.UnLock(true);
                CHECK_RET(iRet,"ERROR.");
                //清理冲突索引信息
                m_arrTableIndex[i].pCIndexMgr->tMutex.Lock(true);
                do
                {
                    CHECK_RET_BREAK(RecycleIndexSpace(m_arrTableIndex[i].pCIndexMgr->tFreeSpace,
                                                      m_arrTableIndex[i].pConflictIndex->iPosAdd,
                                                      m_arrTableIndex[i].pConflictIndex->iSize),"RecycleIndexSpace failed...");
                    m_arrTableIndex[i].pConflictIndex->Clear();
                    m_arrTableIndex[i].pCIndexMgr->iIndexCounts --;//冲突索引-1
                }
                while(0);
                m_arrTableIndex[i].pCIndexMgr->tMutex.UnLock(true);
                CHECK_RET(iRet,"ERROR.");
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }*/

    /******************************************************************************
    * 函数名称	:  RecycleIndexSpace
    * 函数描述	:  回收索引空间
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::RecycleIndexSpace(TMDBIndexFreeSpace tFreeSpace[],size_t iPosAdd,size_t iSize)
    {
        TADD_FUNC("Start.tFreeSpace=[%p],iPosAdd=[%d],iSize=[%d].",tFreeSpace,iPosAdd,iSize);
        int iRet = 0;
        int i = 0;
        for(i = 0; i<MAX_BASE_INDEX_COUNTS; i++)
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
    /******************************************************************************
    * 函数名称	:  PrintIndexSpace
    * 函数描述	: 打印索引空间信息
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::PrintIndexSpace(const char * sPreInfo,TMDBIndexFreeSpace tFreeSpace[])
    {
        printf("\n%s:",sPreInfo);
        for(int i = 0; i<MAX_BASE_INDEX_COUNTS; i++)
        {
            if(tFreeSpace[i].iPosAdd == 0)
            {
                break;   //结束
            }
            printf("\n(%d)PosAdd=[%d],Size[%d] ",i,(int)tFreeSpace[i].iPosAdd,(int)tFreeSpace[i].iSize);
        }
        return 0;
    }
    /******************************************************************************
    * 函数名称	:  DefragIndexSpace
    * 函数描述	:  整理索引空间
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[])
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int i,j;
        //先按从小到大排序，并且将无用的节点(iposAdd < 0)排到后面
        TMDBIndexFreeSpace tTempSapce;
        for(i = 0; i<MAX_BASE_INDEX_COUNTS; i++)
        {
            for(j = i; j<MAX_BASE_INDEX_COUNTS; j++)
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
        for(i = 0; i<MAX_BASE_INDEX_COUNTS - 1;)
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
                if(i+2 == MAX_BASE_INDEX_COUNTS)
                {
                    tFreeSpace[i+1].Clear();
                }
                else
                {
                    //将后来的节点前移
                    memmove(&(tFreeSpace[i+1]),&(tFreeSpace[i+2]),
                            sizeof(TMDBIndexFreeSpace)*(MAX_BASE_INDEX_COUNTS-i-2));
                    tFreeSpace[MAX_BASE_INDEX_COUNTS - 1].Clear();
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
    /******************************************************************************
    * 函数名称	:  GetIndexByColumnPos
    * 函数描述	:  根据columnpos获取indexnode
    * 输入		:  iColumnPos - 列位置
    * 输入		:  iColNoPos-这个索引组中的位置
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*ST_TABLE_INDEX_INFO * TMdbHashIndexCtrl::GetIndexByColumnPos(int iColumnPos,int &iColNoPos)
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
    }*/

    /******************************************************************************
    * 函数名称	:  RemoveDupIndexColumn
    * 函数描述	:  清除重复的可能索引列
    * 输入		:  
    * 输入		:  
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::RemoveDupIndexColumn(std::vector<ST_INDEX_COLUMN> & vLeftIndexColumn,
                            std::vector<ST_INDEX_COLUMN> & vRightIndexColumn)
    {
         //去除重复的可能索引列
        std::vector<ST_INDEX_COLUMN>::iterator itorLeft = vLeftIndexColumn.begin();
        for(;itorLeft != vLeftIndexColumn.end(); ++itorLeft)
        {
            std::vector<ST_INDEX_COLUMN>::iterator itorRight = vRightIndexColumn.begin();
            for(;itorRight != vRightIndexColumn.end();)
            {
                if(itorLeft->pColumn == itorRight->pColumn)
                {//有列相同
                   itorRight =  vRightIndexColumn.erase(itorRight);
                }
                else
                {
                    ++ itorRight;
                }
            }
        }
        return 0;
    }*/
    /******************************************************************************
    * 函数名称	:  RemoveDupIndexValue
    * 函数描述	:  清除重复的索引列
    * 输入		:  
    * 输入		:  
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::RemoveDupIndexValue(std::vector<ST_INDEX_VALUE> & vLeftIndexValue,std::vector<ST_INDEX_VALUE> & vRightIndexValue)
    {
        std::vector<ST_INDEX_VALUE>::iterator itorLeft = vLeftIndexValue.begin();
        for(;itorLeft != vLeftIndexValue.end();++itorLeft)
        {
             std::vector<ST_INDEX_VALUE>::iterator itorRight = vRightIndexValue.begin();
             for(;itorRight != vRightIndexValue.end();)
             {
                if(itorLeft->pstTableIndex == itorRight->pstTableIndex)
                {//有列相同
                   itorRight =  vRightIndexValue.erase(itorRight);
                }
                else
                {
                    ++ itorRight;
                }
            }
        }
        return 0;
    }*/


    /******************************************************************************
    * 函数名称	:  GenerateIndexValue
    * 函数描述	:  生成索引值组合
    * 输入		:  
    * 输入		:  
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::GenerateIndexValue(ST_INDEX_COLUMN *pIndexColumnArr [] ,ST_TABLE_INDEX_INFO  * pstTableIndexInfo,
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
    }*/

    /******************************************************************************
    * 函数名称	:  GetIndexByIndexColumn
    * 函数描述	:  根据可能的索引列获取索引
    * 输入		:  vIndexColumn - 可能的索引列
    * 输入		:  vIndexValue-获取到的索引
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/

    /*int TMdbHashIndexCtrl::GetIndexByIndexColumn(std::vector<ST_INDEX_COLUMN> & vIndexColumn,std::vector<ST_INDEX_VALUE> & vIndexValue)
    {
        TADD_FUNC("Start.vIndexColumn.size=[%d],vIndexValue.size[%d]",vIndexColumn.size(),vIndexValue.size());
        int iRet = 0;
        ST_INDEX_COLUMN * pIndexColumnArr[MAX_COLUMN_COUNTS] = {0};
        int i = 0;
        for(i = 0;i< vIndexColumn.size();++i)
        {
            pIndexColumnArr[vIndexColumn[i].pColumn->iPos] = &(vIndexColumn[i]);
        }
        for(i = 0; i < m_pAttachTable->iIndexCounts; i++)
        {
            if(m_arrTableIndex[i].bInit)
            {
                TMdbIndex * pMdbIndex = m_arrTableIndex[i].pIndexInfo;
                int j = 0;
                bool bFind = false;
                for(j = 0; j<MAX_INDEX_COLUMN_COUNTS; j++)
                {
                    if(pMdbIndex->iColumnNo[j] < 0){break;}//该索引检测完毕
                    if(NULL == pIndexColumnArr[pMdbIndex->iColumnNo[j]])
                    {
                        bFind = false;
                        break;
                    }
                    else
                    {
                        bFind = true;
                    }
                }
                if(bFind)
                {//可以使用该索引
                    std::vector<ST_INDEX_VALUE> vTemp;
                    CHECK_RET(GenerateIndexValue(pIndexColumnArr,&(m_arrTableIndex[i]),0,vTemp),"GenerateIndexValue failed.");
                    vIndexValue.insert(vIndexValue.end(),vTemp.begin(),vTemp.end());
                    break;//找到就可以退出
                }
            }
        }
        vIndexColumn.clear();//处理完清除
        TADD_FUNC("Start.vIndexColumn.size=[%d],vIndexValue.size[%d]",vIndexColumn.size(),vIndexValue.size());
        return iRet;
    }*/

    /******************************************************************************
    * 函数名称	:  GetScanAllIndex
    * 函数描述	:  获取全量遍历的索引
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*ST_TABLE_INDEX_INFO * TMdbHashIndexCtrl::GetScanAllIndex()
    {
        if(m_arrTableIndex[0].bInit)
        {
            return &(m_arrTableIndex[0]);
        }
        else
        {
            return NULL;
        }
    }*/

    /******************************************************************************
    * 函数名称	:  GetVerfiyPKIndex
    * 函数描述	:  获取校验主键的索引
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*ST_TABLE_INDEX_INFO * TMdbHashIndexCtrl::GetVerfiyPKIndex()
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
    }*/
    /******************************************************************************
    * 函数名称	:  CombineCMPIndex
    * 函数描述	:  拼接组合索引
    * 输入		:  stLeftIndexValue,stRightIndexValue待组合
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*bool TMdbHashIndexCtrl::CombineCMPIndex(ST_INDEX_VALUE & stLeftIndexValue,ST_INDEX_VALUE & stRightIndexValue,
                                    ST_INDEX_VALUE & stOutIndexValue)
    {
        std::map<int ,ST_EXPR *> mapColumnExpr;
        int i = 0;
        for(i = 0;i < MAX_INDEX_COLUMN_COUNTS;++i)
        {
            int iPos = -1;
            if(NULL != stLeftIndexValue.pExprArr[i])
            {
                iPos = stLeftIndexValue.pstTableIndex->pIndexInfo->iColumnNo[i];
                if(iPos < 0  || mapColumnExpr.find(iPos) != mapColumnExpr.end())
                {//查找是否存在
                    return false;
                }
                mapColumnExpr[iPos] = stLeftIndexValue.pExprArr[i];
            }
            if(NULL != stRightIndexValue.pExprArr[i])
            {
                 iPos = stRightIndexValue.pstTableIndex->pIndexInfo->iColumnNo[i];
                if(iPos < 0  || mapColumnExpr.find(iPos) != mapColumnExpr.end())
                {//查找是否存在
                    return false;
                }
                mapColumnExpr[iPos] = stRightIndexValue.pExprArr[i];
            }
        }
        bool bFind = false;
        std::map<int,int> mapColumnToPos;//列对于与值的哪列
        //遍历该表的所有索引，查找包含这些列的索引 
        for(i = 0; i < m_pAttachTable->iIndexCounts; i++)
        {
            TMdbIndex * pMdbIndex = m_arrTableIndex[i].pIndexInfo;
            std::map<int ,ST_EXPR *>::iterator itor = mapColumnExpr.begin();
            mapColumnToPos.clear();
            for(;itor != mapColumnExpr.end();++itor)
            {
                int iColumnPos= itor->first;
                int j = 0;
                bFind = false;
                for(j = 0; j<MAX_INDEX_COLUMN_COUNTS; j++)
                {
                    if(iColumnPos == pMdbIndex->iColumnNo[j])
                    {
                        mapColumnToPos[iColumnPos] = j;
                        bFind = true;
                        break;
                    }
                }
                if(false == bFind ){break;}//有一个不匹配
            }
            if(true == bFind){break;}//都匹配
        }
        
        if(true == bFind && i <  m_pAttachTable->iIndexCounts)
        {//找到
            std::map<int ,ST_EXPR *>::iterator itor = mapColumnExpr.begin();
            stOutIndexValue.pstTableIndex = &m_arrTableIndex[i];
            for(;itor != mapColumnExpr.end();++itor)
            {
                if(mapColumnToPos.end() == mapColumnToPos.find(itor->first))
                {
                    TADD_ERROR(ERROR_UNKNOWN,"not find[%d] in mapColumnToPos",itor->first);
                    return false;
                }
                stOutIndexValue.pExprArr[mapColumnToPos[itor->first]] = itor->second;
            }
            return true;
        }
        else
        {//没有找打
            return false;
        }
        return false;
    }*/

    /******************************************************************************
    * 函数名称	:  PrintWarnIndexInfo
    * 函数描述	: 打印存在冲突链大于指定长度的索引
    * 输入		:  iMaxCNodeCount 最大冲突链长度，存在大于该长度冲突链的索引会被打印
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jiang.lili
    *******************************************************************************/
    int TMdbHashIndexCtrl::PrintWarnIndexInfo(int iMaxCNodeCount)
    {
        int iRet = 0;
        TADD_FUNC("PrintIndexInfo(%d): Start.", iMaxCNodeCount);

        //遍历所有的基础索引块
        TMdbBaseIndexMgrInfo *pBIndexMgr = NULL;//基础索引管理区
        TMdbConflictIndexMgrInfo *pCIndexMgr = NULL;//冲突索引管理区
        TMdbBaseIndex *pBaseIndex = NULL;//基础索引指针
        TMdbConflictIndex *pConfIndex = NULL;//冲突索引指针
        char * pBaseIndexAddr = NULL;//基础索引区地址
        char * pConflictIndexAddr = NULL;//冲突索引区地址
        TMdbIndexNode *pBaseIndexNode = NULL;//基础索引链
        TMdbIndexNode *pConfIndexNode = NULL;//冲突索引链
        int iBICounts = 0;//基础索引节点数
        int iCICounts = 0;//冲突索引节点数
        int iConflictIndexPos = 0;//冲突索引序号
        int iIndexNodeCount = 0;//索引链上的节点数
        int iIndexLinkCount = 0;//超长的冲突索引链个数
        bool bExist = false;//是否存在超长的索引

        for(int n=0; n<MAX_SHM_ID; ++n)
        {
            pBaseIndexAddr = m_pMdbShmDsn->GetBaseIndex(n);
            if(pBaseIndexAddr == NULL)
            {
                continue;
            }

            pBIndexMgr = (TMdbBaseIndexMgrInfo*)pBaseIndexAddr;//获取基础索引内容
     
            for (int i = 0; i < MAX_BASE_INDEX_COUNTS; i++)
            {
                pBaseIndex = &pBIndexMgr->tIndex[i];
                TMdbTable* pTable = m_pMdbShmDsn->GetTableByName(pBaseIndex->sTabName);
                if (pTable->bIsSysTab|| pBaseIndex->cState == '0')
                {
                    continue;
                }
                pConflictIndexAddr = m_pMdbShmDsn->GetConflictIndex(pBaseIndex->iConflictMgrPos);//获取对应的冲突索引内容
                pCIndexMgr  = (TMdbConflictIndexMgrInfo *)pConflictIndexAddr;
                CHECK_OBJ(pCIndexMgr);

                iBICounts = pBaseIndex->iSize/sizeof(TMdbIndexNode);//基础索引个数
                iConflictIndexPos = pBaseIndex->iConflictIndexPos;
                pConfIndex = &pCIndexMgr->tIndex[iConflictIndexPos];
                iCICounts =pConfIndex->iSize/sizeof(TMdbIndexNode);//冲突索引个数

                pBaseIndexNode = (TMdbIndexNode*)(pBaseIndexAddr + pBaseIndex->iPosAdd);//基础索引链
                pConfIndexNode = (TMdbIndexNode*)(pConflictIndexAddr + pConfIndex->iPosAdd);//冲突索引连

                iIndexLinkCount = 0;
                for(int j = 0; j < iBICounts; j++)
                {//遍历每条链               
                    int iNextPos = pBaseIndexNode[j].iNextPos;
                    iIndexNodeCount = 0;
                    while(iNextPos >= 0)
                    {
                        iIndexNodeCount++;
                        if(iIndexNodeCount > iCICounts)
                        {
                            OutPutInfo(true,"\nOMG,unlimited loop...\n");
                            return 0;
                        }
                        iNextPos = pConfIndexNode[iNextPos].iNextPos;
                    }
                    if (iIndexNodeCount > iMaxCNodeCount)
                    {
                        iIndexLinkCount++;
                    }                 
                }
                if (iIndexLinkCount > 0)
                {
                    if (!bExist)
                    {
                        bExist = true;
                        OutPutInfo(true, "Index link longer than %d: \n\n", iMaxCNodeCount);
                    }
                    OutPutInfo(true, "    TABLE = [ %s], INDEX_NAME = [ %s ], LINK_COUNT = [ %d ]\n",  
                        pBaseIndex->sTabName, pBaseIndex->sName, iIndexLinkCount);
                }          
            }
        }

        if (!bExist)
        {
            OutPutInfo(true, "All the Indexes are OK. No conflict index link longer than %d.\n ", iMaxCNodeCount);
        }

        TADD_FUNC("PrintIndexInfo(%d): Finish.", iMaxCNodeCount);
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  RebuildTableIndex
    * 函数描述	:  重新构建某表索引区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::RebuildTableIndex(bool bNeedToClean)
    {
        TADD_FUNC("Start");
        int iRet = 0;
        CHECK_OBJ(m_pAttachTable);
        if(bNeedToClean)
        {
            //TODO:需要清理老的索引区
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }*/

    
//}


