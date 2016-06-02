#include "Control/mdbHashIndexCtrl.h" 
#include "Helper/mdbDateTime.h"
#include "Control/mdbRowCtrl.h"

//namespace QuickMDB{

    /******************************************************************************
    * ��������	:  TMdbHashIndexCtrl
    * ��������	:  ��������ģ�飬����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbHashIndexCtrl::TMdbHashIndexCtrl():
        m_pAttachTable(NULL),
        m_pMdbShmDsn(NULL),
        m_pMdbDsn(NULL)
    {
        /*int i = 0;
        for(i = 0; i<MAX_INDEX_COUNTS; i++)
        {
            m_arrTableIndex[i].Clear();//����
        }*/
        m_lSelectIndexValue = -1;
    }

    /******************************************************************************
    * ��������	:  ~TMdbHashIndexCtrl
    * ��������	:  ��������ģ�飬����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbHashIndexCtrl::~TMdbHashIndexCtrl()
    {

    }
    /******************************************************************************
    * ��������	:  SelectIndexNode
    * ��������	:  ��¼���ڲ�ѯ�������ڵ�
    * ����		:  iIndexValue - ���ڲ�ѯ�������ڵ�ֵ
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::SelectIndexNode(MDB_INT64 iIndexValue)
    {
        m_lSelectIndexValue = iIndexValue;
        return 0;
    }

    /******************************************************************************
    * ��������	:  DeleteIndexNode
    * ��������	:  ɾ�������ڵ�
    * ����		:  iIndexPos - ���ϵĵڼ�������
    * ����		:  iIndexValue -�����ڵ�ֵ
    * ����		:  rowID -Ҫɾ���ļ�¼
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::DeleteIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID)
    {
        TADD_FUNC("rowID[%d|%d]",rowID.GetPageID(),rowID.GetDataOffset());
        int iRet = 0;
        CHECK_RET(m_pAttachTable->tTableMutex.Lock(m_pAttachTable->bWriteLock,&m_pMdbDsn->tCurTime),"Lock failed.");
        CHECK_RET(FindRowIndexCValue(tHashIndexInfo, tRowIndex,rowID),"FindRowIndexCValue Failed");//���ҳ�ͻ����ֵ
        TMdbIndexNode * pBaseNode = &(tHashIndexInfo.pBaseIndexNode[tRowIndex.iBaseIndexPos]);
        TMdbIndexNode * pDataNode = NULL;
        if(false == tRowIndex.IsCurNodeInConflict())
        {//��������
            pDataNode = pBaseNode;
        }
        else
        {//��ͻ����
            pDataNode = &(tHashIndexInfo.pConflictIndexNode[tRowIndex.iConflictIndexPos]);
        }
        TADD_DETAIL("pDataNode,rowid[%d|%d],NextPos[%d]",
                            pDataNode->tData.GetPageID(),pDataNode->tData.GetDataOffset(),pDataNode->iNextPos);
        //�ж�rowid�Ƿ���ȷ
        if(pDataNode->tData == rowID)
        {
            if(tRowIndex.iConflictIndexPos < 0)
            {//��������ֱ��������
                TADD_DETAIL("Row in BaseIndex.");
                pDataNode->tData.Clear();//ֻ��������Ϳ�����
            }
            else
            {//��ͻ����
                TMdbIndexNode * pPreNode = tRowIndex.IsPreNodeInConflict()?&(tHashIndexInfo.pConflictIndexNode[tRowIndex.iPreIndexPos]):pBaseNode;//��ȡǰ�ýڵ�
                pPreNode->iNextPos  = pDataNode->iNextPos;// �����ýڵ�
                //���ڵ�ɾ��
                pDataNode->iNextPos = tHashIndexInfo.pConflictIndex->iFreeHeadPos;
                tHashIndexInfo.pConflictIndex->iFreeHeadPos = tRowIndex.iConflictIndexPos;
                tHashIndexInfo.pConflictIndex->iFreeNodeCounts ++;//ʣ��ڵ���-1
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
    * ��������	:  UpdateIndexNode
    * ��������	:  ���������ڵ�
    * ����		:  iIndexPos - ���ϵĵڼ�������
    * ����		:  iOldValue - ��ֵ��iNewValue -��ֵ
    * ���		:  rowID -Ҫ���µļ�¼
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::UpdateIndexNode(TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId)
    {
        TADD_FUNC("Startrow[%d|%d]",tRowId.GetPageID(),tRowId.GetDataOffset());
        int iRet = 0;

        CHECK_RET(DeleteIndexNode( tOldRowIndex,tHashInfo,tRowId),"DeleteIndexNode failed ,tOldRowIndex[%d|%d],row[%d|%d]",
                  tOldRowIndex.iBaseIndexPos,tOldRowIndex.iConflictIndexPos,tRowId.GetPageID(),tRowId.GetDataOffset());//��ɾ��
        CHECK_RET(InsertIndexNode( tNewRowIndex,tHashInfo,tRowId),"InsertIndexNode failed ,tNewRowIndex[%d|%d],row[%d|%d]",
                  tNewRowIndex.iBaseIndexPos,tNewRowIndex.iConflictIndexPos,tRowId.GetPageID(),tRowId.GetDataOffset());//������
                  
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  UpdateIndexNode
    * ��������	:  ���������ڵ�
    * ����		:  iIndexPos - ���ϵĵڼ�������
    * ����		:  iIndexValue -����ֵ
    * ���		:  rowID -Ҫ����ļ�¼
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
            //������������û��ֵ
            TADD_DETAIL("Row can insert BaseIndex.");
            pBaseNode->tData = rowID;
            tRowIndex.iConflictIndexPos = -1;
        }
        else
        {
            TADD_DETAIL("Row can insert ConflictIndex.");
            if(tHashIndex.pConflictIndex->iFreeHeadPos >= 0)
            {
                //���п��г�ͻ�ڵ�, �Գ�ͻ������ͷ��
                int iFreePos = tHashIndex.pConflictIndex->iFreeHeadPos;
                TMdbIndexNode * pFreeNode = &(tHashIndex.pConflictIndexNode[iFreePos]);//���г�ͻ�ڵ�
                pFreeNode->tData = rowID;//��������
                tHashIndex.pConflictIndex->iFreeHeadPos = pFreeNode->iNextPos;
                pFreeNode->iNextPos  = pBaseNode->iNextPos;
                pBaseNode->iNextPos  = iFreePos;
                tHashIndex.pConflictIndex->iFreeNodeCounts --;//ʣ��ڵ���-1
                tRowIndex.iConflictIndexPos = iFreePos;//�ڳ�ͻ����ĳλ��
            }
            else
            {
                //û�п��г�ͻ�ڵ��ˡ�
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
    * ��������	:  FindRowIndexCValue
    * ��������	:  ���ҳ�ͻ����ֵ
    * ����		:  iIndexPos - ���ϵĵڼ�������
    * ����		:  tRowIndex -��¼����Ӧ������ֵ
    * ���		:  rowID -��¼��rowid
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::FindRowIndexCValue(ST_HASH_INDEX_INFO tHashIndexInfo,TMdbRowIndex & tRowIndex,TMdbRowID& rowID)
    {
        TADD_FUNC("rowID[%d|%d]", rowID.GetPageID(),rowID.GetDataOffset());
        int iRet = 0;
        int iCount = 3;//������
        bool bFind = false;
        while(1)
        {
            TMdbIndexNode * pBaseNode = &(tHashIndexInfo.pBaseIndexNode[tRowIndex.iBaseIndexPos]);
            TMdbIndexNode * pTempNode = pBaseNode;
            int iCurPos = -1;//��ǰλ��ֵ
            tRowIndex.iPreIndexPos = iCurPos;
            do
            {
                if(pTempNode->tData == rowID)
                {//�ҵ�
                    tRowIndex.iConflictIndexPos = iCurPos;
                    bFind = true;
                    break;
                }
                if(pTempNode->iNextPos < 0)
                {//���꣬û�ҵ�
                    TADD_WARNING("Not find iCount = [%d]",iCount);
                    break;
                }
                else
                {//������һ��
                    tRowIndex.iPreIndexPos = iCurPos;
                    iCurPos = pTempNode->iNextPos;
                    pTempNode = &(tHashIndexInfo.pConflictIndexNode[pTempNode->iNextPos]);
                }
            }while(1);
            if(iCount <= 0 || bFind){break;}//������
            iCount --;
        }
        if(false == bFind)
        {//����N�ζ�û�ҵ�
            CHECK_RET(ERR_DB_INVALID_VALUE,"not find index node rowID[%d|%d]",
                                rowID.GetPageID(),rowID.GetDataOffset());
        }
        return iRet;
    }

    /******************************************************************************
    * ��������	:  CleanTableIndexInfo
    * ��������	:  ������������Ϣ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*void TMdbHashIndexCtrl::CleanTableIndexInfo()
    {
        int i = 0;
        for(i = 0; i<MAX_INDEX_COUNTS; i++)
        {
            m_arrTableIndex[i].Clear();//����
        }
    }*/

    /******************************************************************************
    * ��������	:  OutPutInfo
    * ��������	:  �����Ϣ
    * ����		:  bConsole - �Ƿ������̨���
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  PrintIndexInfo
    * ��������	:  ��ӡ������Ϣ
    * ����		:  iDetialLevel - ��ϸ���� =0 ������Ϣ��>0 ��ϸ��Ϣ
    * ����		:  bConsole    - �Ƿ������̨���
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::PrintIndexInfo(ST_HASH_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole)
    {
        int iRet = 0;
        int iBICounts = tIndexInfo.pBaseIndex->iSize/sizeof(TMdbIndexNode);//������������
        int iCICounts = tIndexInfo.pConflictIndex->iSize/sizeof(TMdbIndexNode);//��ͻ��������
        OutPutInfo(bConsole,"\n\n============[%s]===========\n",tIndexInfo.pBaseIndex->sName);
        OutPutInfo(bConsole,"[BaseIndex] 	 counts=[%d]\n",iBICounts);
        OutPutInfo(bConsole,"[ConfilictIndex] counts=[%d],FreeHeadPos=[%d],FreeNodes=[%d]\n",
                   iCICounts,
                   tIndexInfo.pConflictIndex->iFreeHeadPos,
                   tIndexInfo.pConflictIndex->iFreeNodeCounts);
        if(iDetialLevel >0 )
        {//��ϸ��Ϣ
           PrintIndexInfoDetail(iDetialLevel,bConsole,tIndexInfo);
        }
        return iRet;
    }
    /******************************************************************************
    * ��������	:  PrintIndexInfoDetail
    * ��������	:  ��ӡ������Ϣ
    * ����		:  iDetialLevel - ��ϸ���� =0 ������Ϣ��>0 ��ϸ��Ϣ
    * ����		:  bConsole    - �Ƿ������̨���
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::PrintIndexInfoDetail(int iDetialLevel,bool bConsole, ST_HASH_INDEX_INFO & stIndexInfo)
    {
        int arrRange[][3] = {{0,10,0},{10,100,0},{100,500,0},{500,2000,0},{2000,5000,0},{5000,-1,0}};//���䷶Χ��ֵ
        int iRangeCount = sizeof(arrRange)/(3*sizeof(int));
        int iBICounts = stIndexInfo.pBaseIndex->iSize/sizeof(TMdbIndexNode);//������������
        int iCICounts = stIndexInfo.pConflictIndex->iSize/sizeof(TMdbIndexNode);//��ͻ��������
        int i = 0;
        for(i = 0; i < iBICounts; ++i)
        {//����ÿ����
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
                    arrRange[j][2]++;//���ڴ�������
                    break;
                }
            }
        }
        OutPutInfo(bConsole,"\nBaseIndex Detail:\n");
        for(i = 0;i< iRangeCount;++i)
        {
            OutPutInfo(bConsole,"[%-6d ~ %-6d] counts = [%-6d]\n",arrRange[i][0],arrRange[i][1],arrRange[i][2]);
        }
        //ͳ�Ƴ�ͻʣ����
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
    * ��������	:  GetBCIndex
    * ��������	:  ��ȡ����+ ��ͻ����
    * ����		:
    * ����		:
    * ���		:  pBaseNode -һ�� �������� pConflictNode-һ���ͻ����
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
                return -1;//��Ϣδ��ʼ��
            }
            pBaseNode[i] = m_arrTableIndex[i].pBaseIndexNode;
            pConflictNode[i] = m_arrTableIndex[i].pConflictIndexNode;
            TADD_DETAIL("Base[%p],Conflict[%p].",pBaseNode[i],pConflictNode[i]);
        }
        TADD_FUNC("Finish");
        return iRet;
    }*/

    /******************************************************************************
    * ��������	:  AttachDsn
    * ��������	:  �����Ϲ����ڴ������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  AttachTable
    * ��������	:  �����ϱ�/��ȡ����+ ��ͻ������Ϣ
    * ����		:  pMdbShmDsn - DSN�ڴ���
    * ����		:  pTable    - ����Ϣ
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::AttachTable(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable)
    {
        int iRet = 0;
        TADD_FUNC("AttachTable(%s) : Start.", pTable->sTableName);
        int iFindIndexs = 0;
        m_pAttachTable = pTable;
        CleanTableIndexInfo();//����
        //�������еĻ���������
        for(int n=0; n<MAX_SHM_ID; ++n)
        {
            TADD_DETAIL("SetIndex(%s) : Shm-ID no.[%d].", pTable->sTableName, n);
            char * pBaseIndexAddr = pMdbShmDsn->GetBaseIndex(n);
            if(pBaseIndexAddr == NULL)
                continue;

            TMdbBaseIndexMgrInfo *pBIndexMgr = (TMdbBaseIndexMgrInfo*)pBaseIndexAddr;//��ȡ������������
            for(int i=0; i<pTable->iIndexCounts; ++i)
            {
                TADD_DETAIL("Need--pTable->tIndex[%d].sName=[%s].", i, pTable->tIndex[i].sName);
                for(int j=0; j<MAX_BASE_INDEX_COUNTS; ++j)
                {
                    //�Ƚ���������,�����ͬ���������ַ��������ַ��¼����
                    if((0 == TMdbNtcStrFunc::StrNoCaseCmp(pTable->sTableName, pBIndexMgr->tIndex[j].sTabName))
                            &&(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tIndex[i].sName, pBIndexMgr->tIndex[j].sName) == 0))
                    {
                        TADD_DETAIL("Find index[%s]",pTable->tIndex[i].sName);
                        //�ҵ�����
                        char * pConflictIndexAddr = pMdbShmDsn->GetConflictIndex(pBIndexMgr->tIndex[j].iConflictMgrPos);
                        TMdbConflictIndexMgrInfo *pCIndexMgr  = (TMdbConflictIndexMgrInfo *)pConflictIndexAddr;
                        CHECK_OBJ(pCIndexMgr);

                        m_arrTableIndex[i].bInit  = true;
                        m_arrTableIndex[i].iIndexPos  = i;
                        m_arrTableIndex[i].pIndexInfo = &(pTable->tIndex[i]);//����index����Ϣ
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
    * ��������	:  InitIndexNode
    * ��������	:  ��ʼ�������ڵ�
    * ����		:  pNode - ������ͷָ��
    * ����		:  iSize   - ������С bList -�Ƿ�Ҫ���ɴ�
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::InitIndexNode(TMdbIndexNode* pNode,MDB_INT64 iSize,bool bList)
    {
        MDB_INT64 iCount = iSize/sizeof(TMdbIndexNode);
        if(bList)
        {
            //��Ҫ����˫����״̬
            pNode[iCount - 1].iNextPos = -1;//��β
            for(MDB_INT64 n=0; n<iCount-1; ++n)
            {
                pNode->tData.Clear();
                pNode->iNextPos = n + 1;//���ɴ�
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
    * ��������	:  CreateNewBIndexShm
    * ��������	:  �����µĻ��������ڴ��
    * ����		:  iShmSize - �ڴ���С
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");//�����´�����shm��ȡ��ӳ���ַ
        //��ʼ����������Ϣ
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
    * ��������	:  CreateNewCIndexShm
    * ��������	:  �����µĳ�ͻ�����ڴ��
    * ����		:  iShmSize - �ڴ���С
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");//�����´�����shm��ȡ��ӳ���ַ
        //��ʼ����������Ϣ
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
    * ��������	:  InitBCIndex
    * ��������	:  ��ʼ������/��ͻ����
    * ����		: pTable - ����Ϣ
    * ����		:
    * ���		:   tTableIndex - ������Ӧ��������Ϣ
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::InitBCIndex(ST_HASH_INDEX_INFO & tTableIndex,TMdbTable * pTable)
    {
        int iRet = 0;
        //��ʼ����������
        SAFESTRCPY(tTableIndex.pBaseIndex->sTabName, sizeof(tTableIndex.pBaseIndex->sTabName), pTable->sTableName);
        
        tTableIndex.pBaseIndex->cState   = '1';
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pBaseIndex->sCreateTime);
        InitIndexNode((TMdbIndexNode *)((char *)tTableIndex.pBIndexMgr + tTableIndex.pBaseIndex->iPosAdd),
                      tTableIndex.pBaseIndex->iSize,false);
        tTableIndex.pBIndexMgr->iIndexCounts ++;
        //��ʼ����ͻ����
        tTableIndex.pConflictIndex->cState = '1';
        TMdbDateTime::GetCurrentTimeStr(tTableIndex.pConflictIndex->sCreateTime);
        InitIndexNode((TMdbIndexNode *)((char *)tTableIndex.pCIndexMgr + tTableIndex.pConflictIndex->iPosAdd),
                      tTableIndex.pConflictIndex->iSize,true);
        tTableIndex.pConflictIndex->iFreeHeadPos = 0;//���н��λ��
        tTableIndex.pConflictIndex->iFreeNodeCounts = tTableIndex.pConflictIndex->iSize/sizeof(TMdbIndexNode);//��¼���г�ͻ�ڵ����
        tTableIndex.pCIndexMgr->iIndexCounts ++;

        return iRet;
    }

    /******************************************************************************
    * ��������	:  GetFreeBIndexShm
    * ��������	:  ��ȡ���л��������飬ֻ����ȡ�����κ��޸�
    * ����		:  iBaseIndexSize -- ����������С
    * ����		:  iDataSize  --  һ�������ڴ���С
    * ���		:  stTableIndexInfo  -- ��ȡ����������Ϣ
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::GetFreeBIndexShm(MDB_INT64 iBaseIndexSize,size_t iDataSize,
                                        ST_HASH_INDEX_INFO & stTableIndexInfo)
    {
        TADD_FUNC("Start.iBaseIndexSize=[%lld].iDataSize=[%lu]",iBaseIndexSize,iDataSize);
        int iRet = 0;
        if((size_t)iBaseIndexSize > iDataSize - 10*1024*1024)
        {
            //����ռ�̫��һ���ڴ�鶼������//Ԥ��10M�ռ�
            CHECK_RET(-1,"DataSize is[%luM],it's too small,must > [%lldM],please change it",
                      iDataSize/1024/1024, iBaseIndexSize/1024/1024);
        }
        bool bFind = false;
        TMdbBaseIndexMgrInfo* pBIndexMgr     = NULL;
        int i = 0;
        for(i = 0; i < MAX_SHM_ID; i++)
        {
            pBIndexMgr = (TMdbBaseIndexMgrInfo*)m_pMdbShmDsn->GetBaseIndex(i);
            if(NULL ==  pBIndexMgr ) //��Ҫ�����µ������ڴ�
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
                //��Ѱ���Է���������Ϣ��λ��
                for(j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
                {
                    if('0' == pBIndexMgr->tIndex[j].cState)
                    {
                        //δ������
                        pBaseIndex = &(pBIndexMgr->tIndex[j]);
                        stTableIndexInfo.iBaseIndexPos = j;
                        break;
                    }
                }
                if(NULL == pBaseIndex)
                {
                    break;   //û�п���λ�ÿ��Է�������Ϣ
                }
                //��Ѱ�Ƿ��п����ڴ�
                for(j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
                {
                    if(pBIndexMgr->tFreeSpace[j].iPosAdd >0)
                    {
                        TMDBIndexFreeSpace& tFreeSpace = pBIndexMgr->tFreeSpace[j];
                        if(tFreeSpace.iSize >= (size_t)iBaseIndexSize)
                        {
                            pBaseIndex->cState = '2';//����״̬
                            pBaseIndex->iPosAdd   = tFreeSpace.iPosAdd;
                            pBaseIndex->iSize     = iBaseIndexSize;
                            if(tFreeSpace.iSize - iBaseIndexSize > 0)
                            {
                                //����ʣ��ռ�
                                tFreeSpace.iPosAdd += iBaseIndexSize;
                                tFreeSpace.iSize   -= iBaseIndexSize;
                            }
                            else
                            {
                                //�ÿ���пռ������ù�
                                tFreeSpace.Clear();
                            }
                            CHECK_RET_BREAK(DefragIndexSpace(pBIndexMgr->tFreeSpace),"DefragIndexSpace failed...");
                        }
                    }
                }
                CHECK_RET_BREAK(iRet,"iRet = [%d]",iRet);
                if('2' != pBaseIndex->cState)
                {
                    //û�п����ڴ��������ڵ�
                    break;
                }
                else
                {
                    //���뵽�����ڴ��
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
                break;   //�ҵ�
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
    * ��������	:  GetFreeCIndexShm
    * ��������	:  ��ȡ���г�ͻ�����ڴ�飬ֻ����ȡ�����κ��޸�
    * ����		:  iConflictIndexSize -- ��ͻ������С
    * ����		:  iDataSize  --  һ�������ڴ���С
    * ���		:  stTableIndexInfo  -- ��ȡ����������Ϣ
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::GetFreeCIndexShm(MDB_INT64 iConflictIndexSize,size_t iDataSize,
                                        ST_HASH_INDEX_INFO & stTableIndexInfo)
    {
        TADD_FUNC("Start.iConflictIndexSize=[%lld].iDataSize=[%lld]",iConflictIndexSize,iDataSize);
        int iRet = 0;
        if((size_t)iConflictIndexSize > iDataSize - 10*1024*1024)
        {
            //����ռ�̫��һ���ڴ�鶼������//Ԥ��10M�ռ�
            CHECK_RET(-1,"DataSize is[%luM],it's too small,must > [%lldM],please change it",
                      iDataSize/1024/1024, iConflictIndexSize/1024/1024);
        }
        bool bFind = false;
        TMdbConflictIndexMgrInfo* pCIndexMgr     = NULL;
        int i = 0;
        for(i = 0; i < MAX_SHM_ID; i++)
        {
            pCIndexMgr = (TMdbConflictIndexMgrInfo*)m_pMdbShmDsn->GetConflictIndex(i);
            if(NULL ==  pCIndexMgr ) //��Ҫ�����µ������ڴ�
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
                //��Ѱ���Է���������Ϣ��λ��
                for(j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
                {
                    if('0' == pCIndexMgr->tIndex[j].cState)
                    {
                        //δ������
                        pConflictIndex = &(pCIndexMgr->tIndex[j]);
                        stTableIndexInfo.pBaseIndex->iConflictMgrPos   = i;
                        stTableIndexInfo.pBaseIndex->iConflictIndexPos = j;
                        stTableIndexInfo.iConflictIndexPos = j;
                        break;
                    }
                }
                if(NULL == pConflictIndex)
                {
                    break;   //û�п���λ�ÿ��Է�������Ϣ
                }
                //��Ѱ�Ƿ��п����ڴ�
                for(j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
                {
                    if(pCIndexMgr->tFreeSpace[j].iPosAdd >0)
                    {
                        TMDBIndexFreeSpace & tFreeSpace = pCIndexMgr->tFreeSpace[j];
                        if(tFreeSpace.iSize >=(size_t) iConflictIndexSize)
                        {
                            pConflictIndex->cState = '2';//����״̬
                            pConflictIndex->iPosAdd   = tFreeSpace.iPosAdd;
                            pConflictIndex->iSize     = iConflictIndexSize;
                            if(tFreeSpace.iSize - iConflictIndexSize > 0)
                            {
                                //����ʣ��ռ�
                                tFreeSpace.iPosAdd += iConflictIndexSize;
                                tFreeSpace.iSize   -= iConflictIndexSize;
                            }
                            else
                            {
                                //�ÿ���пռ������ù�
                                tFreeSpace.Clear();
                            }
                            CHECK_RET_BREAK(DefragIndexSpace(pCIndexMgr->tFreeSpace),"DefragIndexSpace failed...");
                        }
                    }
                }
                CHECK_RET_BREAK(iRet,"iRet = [%d].",iRet);
                if('2' != pConflictIndex->cState)
                {
                    //û�п����ڴ��������ڵ�
                    break;
                }
                else
                {
                    //���뵽�����ڴ��
                    stTableIndexInfo.pConflictIndex = pConflictIndex;
                    stTableIndexInfo.pCIndexMgr     = pCIndexMgr;
                    bFind = true;
                    break;
                }
            }while(0);
            CHECK_RET(pCIndexMgr->tMutex.UnLock(true),"unlock failed.");
            if(bFind)
            {
                break;   //�ҵ�
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
    * ��������	:  AddTableIndex
    * ��������	:  �������
    * ����		:  pTable - ����Ϣ
    * ����		:  iDataSize - ����������ڴ�����С
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::AddTableIndex(TMdbTable * pTable,size_t iDataSize)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        TADD_FLOW("AddTableIndex Table=[%s],Size=[%lu] start.", pTable->sTableName,iDataSize);
        MDB_INT64 iBaseIndexSize     = pTable->iRecordCounts * sizeof(TMdbIndexNode); //���㵥������������Ҫ�Ŀռ�
        MDB_INT64 iConflictIndexSize = pTable->iRecordCounts * sizeof(TMdbIndexNode); //���㵥����ͻ������Ҫ�Ŀռ�
        ST_TABLE_INDEX_INFO stTableIndex;
        for(int i=0; i<pTable->iIndexCounts; ++i)
        {
            stTableIndex.Clear();
            CHECK_RET(GetFreeBIndexShm(iBaseIndexSize, iDataSize,stTableIndex),"GetFreeBIndexShm failed..");
            CHECK_RET(GetFreeCIndexShm(iConflictIndexSize, iDataSize,stTableIndex),"GetFreeCIndexShm failed...");
            if('2' == stTableIndex.pBaseIndex->cState)
            {
                //�ҵ�����λ��
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
        MDB_INT64 iBaseIndexSize     = pTable->iRecordCounts * sizeof(TMdbIndexNode); //���㵥������������Ҫ�Ŀռ�
        MDB_INT64 iConflictIndexSize = pTable->iRecordCounts * sizeof(TMdbIndexNode); //���㵥����ͻ������Ҫ�Ŀռ�
        ST_HASH_INDEX_INFO stTableIndex;
        stTableIndex.Clear();
        CHECK_RET(GetFreeBIndexShm(iBaseIndexSize, iDataSize,stTableIndex),"GetFreeBIndexShm failed..");
        CHECK_RET(GetFreeCIndexShm(iConflictIndexSize, iDataSize,stTableIndex),"GetFreeCIndexShm failed...");
        if('2' == stTableIndex.pBaseIndex->cState)
        {
            //�ҵ�����λ��
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
    * ��������	:  CreateAllIndex
    * ��������	:  ������������shm ����������+ ��ͻ����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::CreateAllIndex(TMdbConfig &config)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_pMdbShmDsn = TMdbShmMgr::GetShmDSN(config.GetDSN()->sName);
        m_pMdbDsn = m_pMdbShmDsn->GetInfo();
        CHECK_OBJ(m_pMdbShmDsn);
        CHECK_OBJ(m_pMdbDsn);
        m_pMdbDsn->iBaseIndexShmCounts = m_pMdbDsn->iConflictIndexShmCounts = 0;//û��counts
        int i = 0;
        TMdbTable * pTable = NULL;
        TMdbTable tTempTable;
        for( i = 0; i<MAX_TABLE_COUNTS; i++)
        {
            //��������table
            pTable  = config.GetTableByPos(i);//m_pTable + i;
            if(NULL != pTable)
            {
                memcpy(&tTempTable,pTable,sizeof(tTempTable));
                tTempTable.ResetRecordCounts();//���ص��ڴ���ʱ��¼�������˸ı�
                CHECK_RET(AddTableIndex(&tTempTable,config.GetDSN()->iDataSize),"AddTableIndex failed...");
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }*/

    /******************************************************************************
    * ��������	:  GetTableByIndexName
    * ��������	:  ������������ȡ��
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
            // �������ҳ�����������Ӧ�ı�
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
    * ��������	:  DeleteTableIndex
    * ��������	:  ɾ��ĳ���������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::DeleteTableIndex(ST_HASH_INDEX_INFO& tIndexInfo)
    {
        int iRet = 0;

        //�������������Ϣ
        CHECK_RET(tIndexInfo.pBIndexMgr->tMutex.Lock(true),"lock failed.");
        do
        {
            CHECK_RET_BREAK(RecycleIndexSpace(tIndexInfo.pBIndexMgr->tFreeSpace,
                                              tIndexInfo.pBaseIndex->iPosAdd,
                                              tIndexInfo.pBaseIndex->iSize),"RecycleIndexSpace failed...");
            tIndexInfo.pBaseIndex->Clear();
            tIndexInfo.pBIndexMgr->iIndexCounts --;//��������-1
        }
        while(0);
        CHECK_RET(tIndexInfo.pBIndexMgr->tMutex.UnLock(true),"unlock failed.");
        CHECK_RET(iRet,"ERROR.");
        //�����ͻ������Ϣ
        CHECK_RET(tIndexInfo.pCIndexMgr->tMutex.Lock(true),"lock failed.");
        do
        {
            CHECK_RET_BREAK(RecycleIndexSpace(tIndexInfo.pCIndexMgr->tFreeSpace,
                                              tIndexInfo.pConflictIndex->iPosAdd,
                                              tIndexInfo.pConflictIndex->iSize),"RecycleIndexSpace failed...");
            tIndexInfo.pConflictIndex->Clear();
            tIndexInfo.pCIndexMgr->iIndexCounts --;//��ͻ����-1
        }
        while(0);
        CHECK_RET(tIndexInfo.pCIndexMgr->tMutex.UnLock(true),"unlock failed.");
        
        TADD_FUNC("Finish.");
        return iRet;
    }

	/******************************************************************************
    * ��������	:  TruncateTableIndex
    * ��������	:  ɾ��ĳ���������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::TruncateTableIndex(ST_HASH_INDEX_INFO& tIndexInfo)
    {
        int iRet = 0;
		//��ʼ����������
		InitIndexNode((TMdbIndexNode *)((char *)tIndexInfo.pBIndexMgr + tIndexInfo.pBaseIndex->iPosAdd),
					  tIndexInfo.pBaseIndex->iSize,false);
		//��ʼ����ͻ����
		InitIndexNode((TMdbIndexNode *)((char *)tIndexInfo.pCIndexMgr + tIndexInfo.pConflictIndex->iPosAdd),
					  tIndexInfo.pConflictIndex->iSize,true);
		tIndexInfo.pConflictIndex->iFreeHeadPos = 0;//���н��λ��
		tIndexInfo.pConflictIndex->iFreeNodeCounts = tIndexInfo.pConflictIndex->iSize/sizeof(TMdbIndexNode);//��¼���г�ͻ�ڵ����

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
        //��Attach�ϱ��ȡ������Ϣ
        CHECK_RET(AttachTable(pMdbShmDsn,pTable),"Attach table[%s] failed....",pTable->sTableName);
        for(int i = 0; i<pTable->iIndexCounts; i++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tIndex[i].sName,pIdxName) != 0)
            {
                continue;
            }
            if(m_arrTableIndex[i].bInit)
            {
                //�������������Ϣ
                m_arrTableIndex[i].pBIndexMgr->tMutex.Lock(true);
                do
                {
                    CHECK_RET_BREAK(RecycleIndexSpace(m_arrTableIndex[i].pBIndexMgr->tFreeSpace,
                                                      m_arrTableIndex[i].pBaseIndex->iPosAdd,
                                                      m_arrTableIndex[i].pBaseIndex->iSize),"RecycleIndexSpace failed...");
                    m_arrTableIndex[i].pBaseIndex->Clear();
                    m_arrTableIndex[i].pBIndexMgr->iIndexCounts --;//��������-1
                }
                while(0);
                m_arrTableIndex[i].pBIndexMgr->tMutex.UnLock(true);
                CHECK_RET(iRet,"ERROR.");
                //�����ͻ������Ϣ
                m_arrTableIndex[i].pCIndexMgr->tMutex.Lock(true);
                do
                {
                    CHECK_RET_BREAK(RecycleIndexSpace(m_arrTableIndex[i].pCIndexMgr->tFreeSpace,
                                                      m_arrTableIndex[i].pConflictIndex->iPosAdd,
                                                      m_arrTableIndex[i].pConflictIndex->iSize),"RecycleIndexSpace failed...");
                    m_arrTableIndex[i].pConflictIndex->Clear();
                    m_arrTableIndex[i].pCIndexMgr->iIndexCounts --;//��ͻ����-1
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
    * ��������	:  RecycleIndexSpace
    * ��������	:  ���������ռ�
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
                //�ҵ�һ�����м�¼���¼�¿�������
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
    * ��������	:  PrintIndexSpace
    * ��������	: ��ӡ�����ռ���Ϣ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::PrintIndexSpace(const char * sPreInfo,TMDBIndexFreeSpace tFreeSpace[])
    {
        printf("\n%s:",sPreInfo);
        for(int i = 0; i<MAX_BASE_INDEX_COUNTS; i++)
        {
            if(tFreeSpace[i].iPosAdd == 0)
            {
                break;   //����
            }
            printf("\n(%d)PosAdd=[%d],Size[%d] ",i,(int)tFreeSpace[i].iPosAdd,(int)tFreeSpace[i].iSize);
        }
        return 0;
    }
    /******************************************************************************
    * ��������	:  DefragIndexSpace
    * ��������	:  ���������ռ�
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[])
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int i,j;
        //�Ȱ���С�������򣬲��ҽ����õĽڵ�(iposAdd < 0)�ŵ�����
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
        //�鿴���ڵĽڵ��Ƿ���Ժϲ�
        for(i = 0; i<MAX_BASE_INDEX_COUNTS - 1;)
        {
            if(tFreeSpace[i].iPosAdd == 0)
            {
                break;   //����
            }
            if(tFreeSpace[i].iPosAdd + tFreeSpace[i].iSize == tFreeSpace[i+1].iPosAdd )
            {
                //���Ժϲ�����ͣ��������ڵ�
                TADD_DETAIL("FreeSpace[%d] and [%d] can merge.",i,i+1);
                tFreeSpace[i].iSize += tFreeSpace[i+1].iSize;
                if(i+2 == MAX_BASE_INDEX_COUNTS)
                {
                    tFreeSpace[i+1].Clear();
                }
                else
                {
                    //�������Ľڵ�ǰ��
                    memmove(&(tFreeSpace[i+1]),&(tFreeSpace[i+2]),
                            sizeof(TMDBIndexFreeSpace)*(MAX_BASE_INDEX_COUNTS-i-2));
                    tFreeSpace[MAX_BASE_INDEX_COUNTS - 1].Clear();
                }
            }
            else if(tFreeSpace[i].iPosAdd + tFreeSpace[i].iSize > tFreeSpace[i+1].iPosAdd &&
                    tFreeSpace[i+1].iPosAdd > 0 )
            {
                //˵����ַ���ص�����
                CHECK_RET(-1,"address is overlaped....");
            }
            else
            {
                i++;//�ж���һ���ڵ�
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  GetIndexByColumnPos
    * ��������	:  ����columnpos��ȡindexnode
    * ����		:  iColumnPos - ��λ��
    * ����		:  iColNoPos-����������е�λ��
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  RemoveDupIndexColumn
    * ��������	:  ����ظ��Ŀ���������
    * ����		:  
    * ����		:  
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::RemoveDupIndexColumn(std::vector<ST_INDEX_COLUMN> & vLeftIndexColumn,
                            std::vector<ST_INDEX_COLUMN> & vRightIndexColumn)
    {
         //ȥ���ظ��Ŀ���������
        std::vector<ST_INDEX_COLUMN>::iterator itorLeft = vLeftIndexColumn.begin();
        for(;itorLeft != vLeftIndexColumn.end(); ++itorLeft)
        {
            std::vector<ST_INDEX_COLUMN>::iterator itorRight = vRightIndexColumn.begin();
            for(;itorRight != vRightIndexColumn.end();)
            {
                if(itorLeft->pColumn == itorRight->pColumn)
                {//������ͬ
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
    * ��������	:  RemoveDupIndexValue
    * ��������	:  ����ظ���������
    * ����		:  
    * ����		:  
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
                {//������ͬ
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
    * ��������	:  GenerateIndexValue
    * ��������	:  ��������ֵ���
    * ����		:  
    * ����		:  
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::GenerateIndexValue(ST_INDEX_COLUMN *pIndexColumnArr [] ,ST_TABLE_INDEX_INFO  * pstTableIndexInfo,
        int iCurPos,std::vector<ST_INDEX_VALUE> & vIndexValue)
    {
        int iRet = 0;
        TADD_FUNC("iCurPos = [%d],vIndexValue.size=[%d]",iCurPos,vIndexValue.size());
        if(pstTableIndexInfo->pIndexInfo->iColumnNo[iCurPos] < 0 || iCurPos >= MAX_INDEX_COLUMN_COUNTS){return iRet;}//�����б������
        int iColPos = pstTableIndexInfo->pIndexInfo->iColumnNo[iCurPos];
        ST_INDEX_COLUMN * pstIndexColumn = pIndexColumnArr[iColPos];
        CHECK_OBJ(pstIndexColumn);
        if(0 == iCurPos)
        {//�״����
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
                {//���
                    ST_INDEX_VALUE & tIndexValue  =  vTemp[i];
                    tIndexValue.pstTableIndex = pstTableIndexInfo;
                    tIndexValue.pExprArr[iCurPos] = pstIndexColumn->vExpr[j];
                    vIndexValue.push_back(tIndexValue);
                }
            }
        }
        pIndexColumnArr[iColPos] = NULL;//��ʶ�Ѿ��������
        return GenerateIndexValue(pIndexColumnArr,pstTableIndexInfo,iCurPos+1,vIndexValue);
    }*/

    /******************************************************************************
    * ��������	:  GetIndexByIndexColumn
    * ��������	:  ���ݿ��ܵ������л�ȡ����
    * ����		:  vIndexColumn - ���ܵ�������
    * ����		:  vIndexValue-��ȡ��������
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
                    if(pMdbIndex->iColumnNo[j] < 0){break;}//������������
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
                {//����ʹ�ø�����
                    std::vector<ST_INDEX_VALUE> vTemp;
                    CHECK_RET(GenerateIndexValue(pIndexColumnArr,&(m_arrTableIndex[i]),0,vTemp),"GenerateIndexValue failed.");
                    vIndexValue.insert(vIndexValue.end(),vTemp.begin(),vTemp.end());
                    break;//�ҵ��Ϳ����˳�
                }
            }
        }
        vIndexColumn.clear();//���������
        TADD_FUNC("Start.vIndexColumn.size=[%d],vIndexValue.size[%d]",vIndexColumn.size(),vIndexValue.size());
        return iRet;
    }*/

    /******************************************************************************
    * ��������	:  GetScanAllIndex
    * ��������	:  ��ȡȫ������������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  GetVerfiyPKIndex
    * ��������	:  ��ȡУ������������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*ST_TABLE_INDEX_INFO * TMdbHashIndexCtrl::GetVerfiyPKIndex()
    {
        TADD_FUNC("Start.");
        if(0 == m_pAttachTable->m_tPriKey.iColumnCounts){return NULL;}
        int arrMatch[MAX_INDEX_COUNTS] = {0};//��¼����������������ƥ���
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
                        {//��һ������
                            arrMatch[i]++;
                            break;
                        }
                    }
                }
            }
        }
        //��ѡƥ�����ߵ�����,�����ǲ��ֻ�ȫ�������У����ܱ������ж�
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
                    {//�в�һ������
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
    * ��������	:  CombineCMPIndex
    * ��������	:  ƴ���������
    * ����		:  stLeftIndexValue,stRightIndexValue�����
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
                {//�����Ƿ����
                    return false;
                }
                mapColumnExpr[iPos] = stLeftIndexValue.pExprArr[i];
            }
            if(NULL != stRightIndexValue.pExprArr[i])
            {
                 iPos = stRightIndexValue.pstTableIndex->pIndexInfo->iColumnNo[i];
                if(iPos < 0  || mapColumnExpr.find(iPos) != mapColumnExpr.end())
                {//�����Ƿ����
                    return false;
                }
                mapColumnExpr[iPos] = stRightIndexValue.pExprArr[i];
            }
        }
        bool bFind = false;
        std::map<int,int> mapColumnToPos;//�ж�����ֵ������
        //�����ñ���������������Ұ�����Щ�е����� 
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
                if(false == bFind ){break;}//��һ����ƥ��
            }
            if(true == bFind){break;}//��ƥ��
        }
        
        if(true == bFind && i <  m_pAttachTable->iIndexCounts)
        {//�ҵ�
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
        {//û���Ҵ�
            return false;
        }
        return false;
    }*/

    /******************************************************************************
    * ��������	:  PrintWarnIndexInfo
    * ��������	: ��ӡ���ڳ�ͻ������ָ�����ȵ�����
    * ����		:  iMaxCNodeCount ����ͻ�����ȣ����ڴ��ڸó��ȳ�ͻ���������ᱻ��ӡ
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TMdbHashIndexCtrl::PrintWarnIndexInfo(int iMaxCNodeCount)
    {
        int iRet = 0;
        TADD_FUNC("PrintIndexInfo(%d): Start.", iMaxCNodeCount);

        //�������еĻ���������
        TMdbBaseIndexMgrInfo *pBIndexMgr = NULL;//��������������
        TMdbConflictIndexMgrInfo *pCIndexMgr = NULL;//��ͻ����������
        TMdbBaseIndex *pBaseIndex = NULL;//��������ָ��
        TMdbConflictIndex *pConfIndex = NULL;//��ͻ����ָ��
        char * pBaseIndexAddr = NULL;//������������ַ
        char * pConflictIndexAddr = NULL;//��ͻ��������ַ
        TMdbIndexNode *pBaseIndexNode = NULL;//����������
        TMdbIndexNode *pConfIndexNode = NULL;//��ͻ������
        int iBICounts = 0;//���������ڵ���
        int iCICounts = 0;//��ͻ�����ڵ���
        int iConflictIndexPos = 0;//��ͻ�������
        int iIndexNodeCount = 0;//�������ϵĽڵ���
        int iIndexLinkCount = 0;//�����ĳ�ͻ����������
        bool bExist = false;//�Ƿ���ڳ���������

        for(int n=0; n<MAX_SHM_ID; ++n)
        {
            pBaseIndexAddr = m_pMdbShmDsn->GetBaseIndex(n);
            if(pBaseIndexAddr == NULL)
            {
                continue;
            }

            pBIndexMgr = (TMdbBaseIndexMgrInfo*)pBaseIndexAddr;//��ȡ������������
     
            for (int i = 0; i < MAX_BASE_INDEX_COUNTS; i++)
            {
                pBaseIndex = &pBIndexMgr->tIndex[i];
                TMdbTable* pTable = m_pMdbShmDsn->GetTableByName(pBaseIndex->sTabName);
                if (pTable->bIsSysTab|| pBaseIndex->cState == '0')
                {
                    continue;
                }
                pConflictIndexAddr = m_pMdbShmDsn->GetConflictIndex(pBaseIndex->iConflictMgrPos);//��ȡ��Ӧ�ĳ�ͻ��������
                pCIndexMgr  = (TMdbConflictIndexMgrInfo *)pConflictIndexAddr;
                CHECK_OBJ(pCIndexMgr);

                iBICounts = pBaseIndex->iSize/sizeof(TMdbIndexNode);//������������
                iConflictIndexPos = pBaseIndex->iConflictIndexPos;
                pConfIndex = &pCIndexMgr->tIndex[iConflictIndexPos];
                iCICounts =pConfIndex->iSize/sizeof(TMdbIndexNode);//��ͻ��������

                pBaseIndexNode = (TMdbIndexNode*)(pBaseIndexAddr + pBaseIndex->iPosAdd);//����������
                pConfIndexNode = (TMdbIndexNode*)(pConflictIndexAddr + pConfIndex->iPosAdd);//��ͻ������

                iIndexLinkCount = 0;
                for(int j = 0; j < iBICounts; j++)
                {//����ÿ����               
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
    * ��������	:  RebuildTableIndex
    * ��������	:  ���¹���ĳ��������
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbHashIndexCtrl::RebuildTableIndex(bool bNeedToClean)
    {
        TADD_FUNC("Start");
        int iRet = 0;
        CHECK_OBJ(m_pAttachTable);
        if(bNeedToClean)
        {
            //TODO:��Ҫ�����ϵ�������
        }
        
        TADD_FUNC("Finish.");
        return iRet;
    }*/

    
//}


