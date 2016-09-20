#include "Helper/mdbDateTime.h"
#include "Control/mdbHashIndexCtrl.h" 
#include "Control/mdbRowCtrl.h"
#include "Control/mdbMgrShm.h"
#include "Control/mdbLinkCtrl.h" 

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
        m_pMdbDsn(NULL),
        m_pRowCtrl(NULL)
    {
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
		SAFE_DELETE(m_pRowCtrl);
    }


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
    * ��������	:  DeleteIndexNode
    * ��������	:  ɾ�������ڵ�
    * ����		:  iIndexPos - ���ϵĵڼ�������
    * ����		:  iIndexValue -�����ڵ�ֵ
    * ����		:  rowID -Ҫɾ���ļ�¼
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::DeleteIndexNode2(int iIndexPos,TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID)
    {
        TADD_FUNC("rowID[%d|%d]",rowID.GetPageID(),rowID.GetDataOffset());
        int iRet = 0;

		TMdbIndexNode * pBaseNode = &(tHashIndexInfo.pBaseIndexNode[tRowIndex.iBaseIndexPos]);
	    TMdbIndexNode * pDataNode = NULL;
		int iMutexPos = tRowIndex.iBaseIndexPos % tHashIndexInfo.pMutex->iMutexCnt;
        TMiniMutex* pMutex = &(tHashIndexInfo.pMutexNode[iMutexPos]);
		
		CHECK_RET(pMutex->Lock(true),"lock failed.");
        do
        {
	        CHECK_RET_BREAK(FindRowIndexCValue(tHashIndexInfo, tRowIndex,rowID),"FindRowIndexCValue Failed");//���ҳ�ͻ����ֵ

	        if(false == tRowIndex.IsCurNodeInConflict())
	        {//��������
	            pDataNode = pBaseNode;
	        }
	        else
	        {//��ͻ����
	            pDataNode = &(tHashIndexInfo.pConflictIndexNode[tRowIndex.iConflictIndexPos]);
	        }
	        TADD_DETAIL("pDataNode,rowid[%d|%d],NextPos[%d]",pDataNode->tData.GetPageID(),pDataNode->tData.GetDataOffset(),pDataNode->iNextPos);
	        //�ж�rowid�Ƿ���ȷ
	        if(pDataNode->tData == rowID)
	        {
	            if(tRowIndex.iConflictIndexPos < 0)
	            {//��������ֱ��������
	                TADD_DETAIL("Row in BaseIndex.");
	                pDataNode->tData.Clear();//ֻ��������Ϳ�����
	            }
	            else//��ͻ����
	            {
		            TMdbSingleTableIndexInfo* pTableIndexInfo = m_pLink->FindCurTableIndex(m_pAttachTable->sTableName);
					CHECK_OBJ_BREAK(pTableIndexInfo);
					
					ST_LINK_INDEX_INFO &tLinkIndexInfo = pTableIndexInfo->arrLinkIndex[iIndexPos];
	                TMdbIndexNode * pPreNode = tRowIndex.IsPreNodeInConflict()?&(tHashIndexInfo.pConflictIndexNode[tRowIndex.iPreIndexPos]):pBaseNode;//��ȡǰ�ýڵ�
	                pPreNode->iNextPos  = pDataNode->iNextPos;// �����ýڵ�
	                
	                //���ڵ�ɾ��,��������
	                pDataNode->iNextPos = tLinkIndexInfo.iHashCFreeHeadPos;
	                tLinkIndexInfo.iHashCFreeHeadPos = tRowIndex.iConflictIndexPos;
	                tLinkIndexInfo.iHashCFreeNodeCounts++;
	                pDataNode->tData.Clear();

					CHECK_RET(ReturnConflictNodeToTable(tLinkIndexInfo, tHashIndexInfo),"ReturnConflictNodeToTable Failed");
	            }
	        }
	        else
	        {//error
	            TADD_WARNING("not find indexnode to delete....index[%d|%d],rowid[%d|%d]",tRowIndex.iBaseIndexPos,tRowIndex.iConflictIndexPos,rowID.GetPageID(),rowID.GetDataOffset());
	            PrintIndexInfo(tHashIndexInfo,0,false);
	        }
        }
		while(0);		
		CHECK_RET(pMutex->UnLock(true),"lock failed.");

        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbHashIndexCtrl::DeleteRedirectIndexNode(int iIndexPos,char* pAddr,TMdbRowIndex & tRowIndex,ST_HASH_INDEX_INFO & tHashIndex,TMdbRowID& rowID)
	{
	    TADD_FUNC("IndexPos[%d],tRowIndex[%d|%d],rowID[%d|%d]",iIndexPos,tRowIndex.iBaseIndexPos,tRowIndex.iConflictIndexPos,
	                                                                    rowID.GetPageID(),rowID.GetDataOffset());
		int iRet = 0;

		bool bBase = false;
        long long iIndexNodePos = 0;
        CHECK_RET(m_pRowCtrl->GetIndexPos(pAddr,m_pAttachTable->tIndex[iIndexPos].iRIdxOffset,iIndexPos,bBase,iIndexNodePos),
            "Get index node pos from data failed.index=[%s]", m_pAttachTable->tIndex[iIndexPos].sName);
        if(!bBase)
        {
            tRowIndex.iConflictIndexPos = static_cast<int>(iIndexNodePos);
        }
        else
        {
            tRowIndex.iConflictIndexPos = -1;
        }

        CHECK_RET(m_pAttachTable->tTableMutex.Lock(m_pAttachTable->bWriteLock,&m_pMdbDsn->tCurTime),"Lock failed.");
	    if(false == tRowIndex.IsCurNodeInConflict())
	    {//��������
	        TMdbIndexNode * pBaseNode = &(tHashIndex.pBaseIndexNode[tRowIndex.iBaseIndexPos]);

	        if(pBaseNode->tData == rowID)
	        {
	            TADD_DETAIL("Row in BaseIndex.");
	            pBaseNode->tData.Clear();//ֻ��������Ϳ�����
	            TADD_DETAIL("delete on base[%d]",tRowIndex.iBaseIndexPos);
	        }
	        else
	        {//error
	            TADD_WARNING("not find indexnode to delete....pos[%d],index[%d|%d],rowid[%d|%d]",
	                         iIndexPos,tRowIndex.iBaseIndexPos,tRowIndex.iConflictIndexPos,rowID.GetPageID(),rowID.GetDataOffset());
	            PrintIndexInfo(tHashIndex,1,false);
	        }
	    }
	    else
	    {//��ͻ����
	        TMdbReIndexNode* pDataNode = &(tHashIndex.pReConfNode[tRowIndex.iConflictIndexPos]);
	        TADD_DETAIL("to delete on conflict[%d], pre[%s][%d],next[%d]",tRowIndex.iConflictIndexPos, 
	            pDataNode->bPreBase?"BASE":"CONF", pDataNode->iPrePos, pDataNode->iNextPos);
	        if(pDataNode->tData == rowID)
	        {            
	            if(pDataNode->bPreBase) // ǰһ���ڵ��ڻ�������
	            {
	                TMdbIndexNode * pPreNode = &(tHashIndex.pBaseIndexNode[tRowIndex.iBaseIndexPos]);
	                pPreNode->iNextPos = pDataNode->iNextPos;

	                if(pDataNode->iNextPos >= 0)
	                {
	                    TMdbReIndexNode* pNextNode = &(tHashIndex.pReConfNode[pDataNode->iNextPos]);
	                    pNextNode->SetPreNode(tRowIndex.iBaseIndexPos,true);
	                }
	            }
	            else
	            {
	                TMdbReIndexNode* pPreNode = &(tHashIndex.pReConfNode[pDataNode->iPrePos]);
	                pPreNode->iNextPos = pDataNode->iNextPos;
	                if(pDataNode->iNextPos >= 0)
	                {
	                    TMdbReIndexNode* pNextNode = &(tHashIndex.pReConfNode[pDataNode->iNextPos]);
	                    pNextNode->SetPreNode(pDataNode->iPrePos,false);
	                }
	            }

	            pDataNode->iNextPos = tHashIndex.pConflictIndex->iFreeHeadPos;
	            pDataNode->SetPreNode(-1,false);
	            tHashIndex.pConflictIndex->iFreeHeadPos = tRowIndex.iConflictIndexPos;
	            tHashIndex.pConflictIndex->iFreeNodeCounts ++;

	            pDataNode->tData.Clear();

	        }
	        else
	        {//error
	            TADD_WARNING("not find indexnode to delete....pos[%d],index[%d|%d],rowid[%d|%d]",
	                         iIndexPos,tRowIndex.iBaseIndexPos,tRowIndex.iConflictIndexPos,rowID.GetPageID(),rowID.GetDataOffset());
	            PrintIndexInfo(tHashIndex,1,false);
	        }
	    }
        CHECK_RET(m_pAttachTable->tTableMutex.UnLock(m_pAttachTable->bWriteLock),"unlock failed.");
	    
	    TADD_FUNC("Finish.");
	    return iRet;
	}

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
    * ����		:  iOldValue - ��ֵ��iNewValue -��ֵ
    * ���		:  rowID -Ҫ���µļ�¼
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::UpdateIndexNode2(int iIndexPos,TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId)
    {
        TADD_FUNC("Startrow[%d|%d]",tRowId.GetPageID(),tRowId.GetDataOffset());
        int iRet = 0;

        CHECK_RET(DeleteIndexNode2(iIndexPos,tOldRowIndex,tHashInfo,tRowId),"DeleteIndexNode2 failed ,tOldRowIndex[%d|%d],row[%d|%d]",
                  tOldRowIndex.iBaseIndexPos,tOldRowIndex.iConflictIndexPos,tRowId.GetPageID(),tRowId.GetDataOffset());//��ɾ��
        CHECK_RET(InsertIndexNode2(iIndexPos,tNewRowIndex,tHashInfo,tRowId),"InsertIndexNode2 failed ,tNewRowIndex[%d|%d],row[%d|%d]",
                  tNewRowIndex.iBaseIndexPos,tNewRowIndex.iConflictIndexPos,tRowId.GetPageID(),tRowId.GetDataOffset());//������
                  
        TADD_FUNC("Finish.");
        return iRet;
    }

	
    int TMdbHashIndexCtrl::UpdateRedirectIndexNode(int iIndexPos,char* pAddr,TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashIndex,TMdbRowID& rowID)
    {
		int iRet = 0;

        CHECK_RET(DeleteRedirectIndexNode(iIndexPos,pAddr, tOldRowIndex,tHashIndex,rowID),"DeleteIndexNode2 failed ,tOldRowIndex[%d|%d],row[%d|%d]",
                  tOldRowIndex.iBaseIndexPos,tOldRowIndex.iConflictIndexPos,rowID.GetPageID(),rowID.GetDataOffset());//��ɾ��
        CHECK_RET(InsertRedirectIndexNode(iIndexPos,pAddr, tNewRowIndex,tHashIndex,rowID),"InsertRedirectIndexNode failed ,tNewRowIndex[%d|%d],row[%d|%d]",
                  tNewRowIndex.iBaseIndexPos,tNewRowIndex.iConflictIndexPos,rowID.GetPageID(),rowID.GetDataOffset());//������
		return iRet;
	}


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
    * ��������	:  InsertIndexNode
    * ��������	:  ���������ڵ�
    * ����		:  iIndexPos - ���ϵĵڼ�������
    * ����		:  iIndexValue -����ֵ
    * ���		:  rowID -Ҫ����ļ�¼
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbHashIndexCtrl::InsertIndexNode2(int iIndexPos,TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tTableHashIndex, TMdbRowID& rowID)
    {
        TADD_FUNC("Start.rowID[%d|%d]",  rowID.GetPageID(),rowID.GetDataOffset());
        int iRet = 0;
        TMdbIndexNode * pBaseNode = &(tTableHashIndex.pBaseIndexNode[tRowIndex.iBaseIndexPos]);
        tRowIndex.iPreIndexPos       = -1;

		int iMutexPos = tRowIndex.iBaseIndexPos % tTableHashIndex.pMutex->iMutexCnt;
        TMiniMutex* pMutex = &(tTableHashIndex.pMutexNode[iMutexPos]);

		
		CHECK_RET(pMutex->Lock(true),"lock failed.");
        do
        {
		    if(pBaseNode->tData.IsEmpty())
		    {
		        //������������û��ֵ
		        TADD_DETAIL("Row can insert BaseIndex.");
		        pBaseNode->tData = rowID;
		        tRowIndex.iConflictIndexPos = -1;
		    }
		    else
		    {
		    	TMdbSingleTableIndexInfo* pTableIndexInfo = m_pLink->FindCurTableIndex(m_pAttachTable->sTableName);
				CHECK_OBJ(pTableIndexInfo);
				
				ST_LINK_INDEX_INFO &tLinkIndexInfo = pTableIndexInfo->arrLinkIndex[iIndexPos];

				int iTry = 3;
				do
				{
					if(tLinkIndexInfo.iHashCFreeHeadPos>= 0)
		            {
		                //���п��г�ͻ�ڵ�, �Գ�ͻ������ͷ��
		                int iFreePos = tLinkIndexInfo.iHashCFreeHeadPos;
		                TMdbIndexNode * pFreeNode = &(tTableHashIndex.pConflictIndexNode[iFreePos]);//���г�ͻ�ڵ�
		                pFreeNode->tData = rowID;//��������
		                tLinkIndexInfo.iHashCFreeHeadPos = pFreeNode->iNextPos;
		                pFreeNode->iNextPos  = pBaseNode->iNextPos;
		                pBaseNode->iNextPos  = iFreePos;
		                tLinkIndexInfo.iHashCFreeNodeCounts --;//ʣ��ڵ���-1
		                tRowIndex.iConflictIndexPos = iFreePos;//�ڳ�ͻ����ĳλ��
		                break;
		            }
		            else
		            {
		            	//�����ϵĳ�ͻ�ڵ㲻�����ӱ�����
		            	iRet = ApplyConflictNodeFromTable(tLinkIndexInfo, tTableHashIndex);

						if(iRet!=0){iTry--;}
						
						//��ೢ�����Σ�Ҫ�ǻ����벻�����ͱ���
						if(iTry < 0)
						{
			                TADD_ERROR(ERR_TAB_NO_CONFLICT_INDEX_NODE,"No free conflict indexnode.....tRowIndex[%d|%d],rowid[%d|%d],record-counts[%d] of table[%s] is too small",
			                           tRowIndex.iBaseIndexPos,tRowIndex.iConflictIndexPos,rowID.GetPageID(),rowID.GetDataOffset(),
			                           m_pAttachTable->iRecordCounts,m_pAttachTable->sTableName);
			                PrintIndexInfo(tTableHashIndex,0,false);
			                iRet = ERR_TAB_NO_CONFLICT_INDEX_NODE;
							break;
						}
		            }
				}while(1);	
		    }
        }
        while(0);
        CHECK_RET(pMutex->UnLock(true), "unlock failed.");  
		
        TADD_FUNC("Finish.");
        return iRet;
    }
	int TMdbHashIndexCtrl::InsertRedirectIndexNode(int iIndexPos,char* pAddr,TMdbRowIndex &tRowIndex, ST_HASH_INDEX_INFO & tHashIndex, TMdbRowID& rowID)
	{
	    int iRet = 0;
	    TADD_FUNC("tRowIndex[%d|%d],rowID[%d|%d]",tRowIndex.iBaseIndexPos,tRowIndex.iConflictIndexPos,rowID.GetPageID(),rowID.GetDataOffset());
        CHECK_RET(m_pAttachTable->tTableMutex.Lock(m_pAttachTable->bWriteLock,&m_pMdbDsn->tCurTime),"Lock failed.");
	    TMdbIndexNode * pBaseNode = &(tHashIndex.pBaseIndexNode[tRowIndex.iBaseIndexPos]);
	    tRowIndex.iPreIndexPos       = -1;
	    if(pBaseNode->tData.IsEmpty())
	    {
	        //������������û��ֵ
	        TADD_DETAIL("Row can insert BaseIndex.");
	        pBaseNode->tData = rowID;
	        tRowIndex.iConflictIndexPos = -1;//ֱ�ӷ��ڻ�������
	        TADD_DETAIL("insert on base[%d]",tRowIndex.iBaseIndexPos);
	    }
	    else
	    {
	        TADD_DETAIL("Row can insert ConflictIndex.");
	        if(tHashIndex.pConflictIndex->iFreeHeadPos >= 0)
	        {
	            //���п��г�ͻ�ڵ�, �Գ�ͻ������ͷ��
	            int iFreePos = tHashIndex.pConflictIndex->iFreeHeadPos;
	            TMdbReIndexNode * pFreeNode = &(tHashIndex.pReConfNode[iFreePos]);//���г�ͻ�ڵ�
	            pFreeNode->tData = rowID;//��������
	            tHashIndex.pConflictIndex->iFreeHeadPos = pFreeNode->iNextPos;
	            if(pBaseNode->iNextPos >= 0)
	            {//����
	                TMdbReIndexNode * pNextNode = &(tHashIndex.pReConfNode[pBaseNode->iNextPos]);
	                pNextNode->SetPreNode(iFreePos,false);
	            }
	            pFreeNode->iNextPos  = pBaseNode->iNextPos;
	            pFreeNode->SetPreNode(tRowIndex.iBaseIndexPos,true);
	            pBaseNode->iNextPos  = iFreePos;
	            tHashIndex.pConflictIndex->iFreeNodeCounts --;//ʣ��ڵ���-1
	            tRowIndex.iConflictIndexPos = iFreePos;//�ڳ�ͻ����ĳλ��
	            TADD_DETAIL("insert on conflict[%d], pre:[%s][%d], next[%d]"
	                ,iFreePos, pFreeNode->bPreBase?"BASE":"CONF",pFreeNode->iPrePos,pFreeNode->iNextPos);
	            
	        }
	        else
	        {
	            //û�п��г�ͻ�ڵ��ˡ�
	            TADD_ERROR(ERR_TAB_NO_CONFLICT_INDEX_NODE,"No free conflict indexnode.....tRowIndex[%d|%d],rowid[%d|%d],record-counts[%d] of table[%s] is too small",
	                       tRowIndex.iBaseIndexPos,tRowIndex.iConflictIndexPos,rowID.GetPageID(),rowID.GetDataOffset(),m_pAttachTable->iRecordCounts,m_pAttachTable->sTableName);
                PrintIndexInfo(tHashIndex,0,false);
	            iRet = ERR_TAB_NO_CONFLICT_INDEX_NODE;
	        }
	    }
        CHECK_RET(m_pAttachTable->tTableMutex.UnLock(m_pAttachTable->bWriteLock),"unlock failed.");

		//��������������Ҫ�ڼ�¼������ϱ��
		if(tRowIndex.IsCurNodeInConflict())
        {
            m_pRowCtrl->SetIndexPos(pAddr,m_pAttachTable->tIndex[iIndexPos].iRIdxOffset,iIndexPos, false,tRowIndex.iConflictIndexPos);
        }
        else
        {
            m_pRowCtrl->SetIndexPos(pAddr,m_pAttachTable->tIndex[iIndexPos].iRIdxOffset,iIndexPos, true,tRowIndex.iBaseIndexPos);
        }
		
	    TADD_FUNC("Finish.");
	    return iRet;
	}

	int TMdbHashIndexCtrl::ApplyConflictNodeFromTable(ST_LINK_INDEX_INFO& tLinkIndexInfo, ST_HASH_INDEX_INFO& tTableHashIndex)
    {
    	int iRet = 0;
		//tTableHashIndex ���ڱ���Ĺ�����Դ����Ҫ��������
        CHECK_RET(m_pAttachTable->tTableMutex.Lock(m_pAttachTable->bWriteLock,&m_pMdbDsn->tCurTime),"Lock failed.");
		int iAskNodeNum = GetCApplyNum(m_pAttachTable->iTabLevelCnts);

		do
		{
			if(tTableHashIndex.pConflictIndex->iFreeHeadPos < 0) 
			{
				iRet = ERR_TAB_NO_CONFLICT_INDEX_NODE;
				break;
			}

			tLinkIndexInfo.iHashCFreeHeadPos= tTableHashIndex.pConflictIndex->iFreeHeadPos;
			
			int iFreePos = -1;	
			int iLastPos = -1;//��¼���뵽�����һ���ڵ��λ��
			for(int i = 0;i<iAskNodeNum;i++)
			{	
				iFreePos = tTableHashIndex.pConflictIndex->iFreeHeadPos;
				//������ĳ�ͻ�ڵ�Ҳ��������
				if(-1 == iFreePos){break;}
				
				TMdbIndexNode * pFreeNode = &(tTableHashIndex.pConflictIndexNode[iFreePos]);

				//������ĳ�ͻ�ڵ㳤��
				tTableHashIndex.pConflictIndex->iFreeHeadPos = pFreeNode->iNextPos;
				tTableHashIndex.pConflictIndex->iFreeNodeCounts--;
				
				tLinkIndexInfo.iHashCFreeNodeCounts++;
				iLastPos = iFreePos;

			}

			if(-1 != iLastPos)
			{
				TMdbIndexNode* pLastNode = &(tTableHashIndex.pConflictIndexNode[iLastPos]);
				pLastNode->iNextPos = -1;
			}

		}while(0);

		//TADD_NORMAL("Link Ask %d nodes From Table %s.",tLinkIndexInfo.iHashCFreeNodeCounts,m_pAttachTable->sTableName);

		CHECK_RET(m_pAttachTable->tTableMutex.UnLock(m_pAttachTable->bWriteLock),"unlock failed.");
		return iRet;
	}


	
	int TMdbHashIndexCtrl::ReturnConflictNodeToTable(ST_LINK_INDEX_INFO& tLinkIndexInfo, ST_HASH_INDEX_INFO& tTableHashIndex)
	{
		int iRet = 0;
		int iNum = GetCApplyNum(m_pAttachTable->iTabLevelCnts);
		//ֻ�������ϴ��ڹ���Ľڵ㣬�Ż�黹����
		if(tLinkIndexInfo.iHashCFreeNodeCounts > 2* iNum)
		{			
			CHECK_RET(m_pAttachTable->tTableMutex.Lock(m_pAttachTable->bWriteLock,&m_pMdbDsn->tCurTime),"Lock failed.");
			int iReturnNum = iNum;
			int iFreePosOfTable = tTableHashIndex.pConflictIndex->iFreeHeadPos;			
			tTableHashIndex.pConflictIndex->iFreeHeadPos = tLinkIndexInfo.iHashCFreeHeadPos;
			TMdbIndexNode * pFreeNode = NULL;
			
			//�������ǰ��iReturnNum���ڵ�黹
			for(int i=0; i<iReturnNum; i++)
			{
				pFreeNode = &(tTableHashIndex.pConflictIndexNode[tLinkIndexInfo.iHashCFreeHeadPos]);
				tLinkIndexInfo.iHashCFreeHeadPos = pFreeNode->iNextPos;
				tLinkIndexInfo.iHashCFreeNodeCounts --;
			}
			pFreeNode->iNextPos = iFreePosOfTable;
			tTableHashIndex.pConflictIndex->iFreeNodeCounts +=iReturnNum;
			
			CHECK_RET(m_pAttachTable->tTableMutex.UnLock(m_pAttachTable->bWriteLock),"unlock failed.");
			
		}


		return iRet;

	}

	
	int  TMdbHashIndexCtrl::ReturnAllIndexNodeToTable(ST_LINK_INDEX_INFO& tLinkIndexInfo, ST_HASH_INDEX_INFO& tTableHashIndex)
	{
		int iRet = 0;

		if(tLinkIndexInfo.iHashCFreeNodeCounts <=0) return iRet;
		
		CHECK_RET(m_pAttachTable->tTableMutex.Lock(m_pAttachTable->bWriteLock,&m_pMdbDsn->tCurTime),"Lock failed.");

		int iFreePosOfTable = tTableHashIndex.pConflictIndex->iFreeHeadPos;			
		tTableHashIndex.pConflictIndex->iFreeHeadPos = tLinkIndexInfo.iHashCFreeHeadPos;
		TMdbIndexNode * pFreeNode = NULL;
		
		while(tLinkIndexInfo.iHashCFreeNodeCounts > 0)
		{
			pFreeNode = &(tTableHashIndex.pConflictIndexNode[tLinkIndexInfo.iHashCFreeHeadPos]);
			tLinkIndexInfo.iHashCFreeHeadPos = pFreeNode->iNextPos;
			tLinkIndexInfo.iHashCFreeNodeCounts --;
			tTableHashIndex.pConflictIndex->iFreeNodeCounts++;
		}
		
		pFreeNode->iNextPos = iFreePosOfTable;
		
		CHECK_RET(m_pAttachTable->tTableMutex.UnLock(m_pAttachTable->bWriteLock),"unlock failed.");		
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
        size_t iBICounts = tIndexInfo.pBaseIndex->iSize/sizeof(TMdbIndexNode);//������������
        
		size_t iCICounts = 0;
		bool bReConf = (NULL == tIndexInfo.pReConfNode) ? false :true;
        if(bReConf)
        {
            iCICounts = tIndexInfo.pConflictIndex->iSize/sizeof(TMdbReIndexNode);
        }
        else
        {
            iCICounts = tIndexInfo.pConflictIndex->iSize/sizeof(TMdbIndexNode);//��ͻ��������
        }
        OutPutInfo(bConsole,"\n\n============[%s]===========\n",tIndexInfo.pBaseIndex->sName);		
        OutPutInfo(bConsole,"[IsRedirectIndex] 	 [%d]\n",bReConf);
        OutPutInfo(bConsole,"[BaseIndex] 	 counts=[%lu]\n",iBICounts);
        OutPutInfo(bConsole,"[ConfilictIndex] counts=[%lu],FreeHeadPos=[%d],FreeNodes=[%d]\n",
                   iCICounts,
                   tIndexInfo.pConflictIndex->iFreeHeadPos,
                   tIndexInfo.pConflictIndex->iFreeNodeCounts);
        if(iDetialLevel >0 )
        {//��ϸ��Ϣ
       	 	if(bReConf)
            {
                PrintRePosIndexInfoDetail(iDetialLevel,bConsole,tIndexInfo);
            }
			else
			{
           		PrintIndexInfoDetail(iDetialLevel,bConsole,tIndexInfo);
			}
        }
        return iRet;
    }

	
	int TMdbHashIndexCtrl::PrintRePosIndexInfoDetail(int iDetialLevel,bool bConsole, ST_HASH_INDEX_INFO & stIndexInfo)
	{
		size_t arrRange[][3] = {{0,10,0},{10,100,0},{100,500,0},{500,2000,0},{2000,5000,0},{5000, 0 ,0}};//���䷶Χ��ֵ
		size_t iRangeCount = sizeof(arrRange)/(3*sizeof(size_t));
		size_t iBICounts = stIndexInfo.pBaseIndex->iSize/sizeof(TMdbIndexNode);//������������
		size_t iCICounts = stIndexInfo.pConflictIndex->iSize/sizeof(TMdbReIndexNode);//��ͻ��������

		for(size_t i = 0; i < iBICounts; ++i)
		{//����ÿ����
			size_t iCount = 0;
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
				iNextPos = stIndexInfo.pReConfNode[iNextPos].iNextPos;
			}
			size_t j = 0;
			for(j = 0;j < iRangeCount;++j)
			{
				if(iCount > arrRange[j][0] &&( iCount <= arrRange[j][1] || arrRange[j][1] == 0))
				{
					arrRange[j][2]++;//���ڴ�������
					break;
				}
			}
		}
		OutPutInfo(bConsole,"\nBaseIndex Detail:\n");
		for(size_t i = 0;i< iRangeCount;++i)
		{
			OutPutInfo(bConsole,"[%-6lu ~ %-6lu] counts = [%-6lu]\n",arrRange[i][0],arrRange[i][1],arrRange[i][2]);
		}
		//ͳ�Ƴ�ͻʣ����
		int iNextPos = stIndexInfo.pConflictIndex->iFreeHeadPos;
		size_t iCount = 0;
		while(iNextPos >= 0)
		{
			iCount++;
			if(iCount > iCICounts)
			{
				OutPutInfo(bConsole,"\nOMG,unlimited loop...iCount[%lu],iCICounts[%lu]\n",iCount,iCICounts);
				return 0;
			}
			iNextPos = stIndexInfo.pReConfNode[iNextPos].iNextPos;
		}
		OutPutInfo(bConsole,"free conflict nodes = %lu\n",iCount);
		if(iDetialLevel > 1)
		{
			int i = 0;
			int iPrintCounts = iDetialLevel<(int)iCICounts?iDetialLevel:(int)iCICounts;
			printf("Conflict node detail:\n");
			for(i = 0;i<iPrintCounts;++i)
			{
					if(i%10 == 0){printf("\n");}
					printf("[%d:%d] ",i,stIndexInfo.pReConfNode[i].iNextPos);
			}
			printf("\n");
		}
		return 0;
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
        size_t arrRange[][3] = {{0,10,0},{10,100,0},{100,500,0},{500,2000,0},{2000,5000,0},{5000,0 ,0}};//���䷶Χ��ֵ
        size_t iRangeCount = sizeof(arrRange)/(3*sizeof(size_t));
        size_t iBICounts = stIndexInfo.pBaseIndex->iSize/sizeof(TMdbIndexNode);//������������
        size_t iCICounts = stIndexInfo.pConflictIndex->iSize/sizeof(TMdbIndexNode);//��ͻ��������
		size_t i = 0;
        for( i = 0; i < iBICounts; ++i)
        {//����ÿ����
            size_t iCount = 0;
            int iNextPos = stIndexInfo.pBaseIndexNode[i].iNextPos;

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
            size_t j = 0;
            for(j = 0;j < iRangeCount;++j)
            {
                if(iCount > arrRange[j][0] &&( iCount <= arrRange[j][1] || arrRange[j][1]== 0))
                {
                    arrRange[j][2]++;//���ڴ�������
                    break;
                }
            }
        }
        OutPutInfo(bConsole,"\nBaseIndex Detail:\n");
        for(  i = 0;i< iRangeCount;++i)
        {
            OutPutInfo(bConsole,"[%-6lu ~ %-6lu] counts = [%-6lu]\n",arrRange[i][0],arrRange[i][1],arrRange[i][2]);
        }
        //ͳ�Ƴ�ͻʣ����
        int iNextPos = stIndexInfo.pConflictIndex->iFreeHeadPos;
        size_t iCount = 0;
        while(iNextPos >= 0)
        {
            iCount++;
            if(iCount > iCICounts)
            {
                OutPutInfo(bConsole,"\nOMG,unlimited loop...iCount[%lu],iCICounts[%lu]\n",iCount,iCICounts);
                return 0;
            }
            iNextPos = stIndexInfo.pConflictIndexNode[iNextPos].iNextPos;
        }
        OutPutInfo(bConsole,"free conflict nodes = %lu\n",iCount);
        if(iDetialLevel > 1)
        {
            int i = 0;
            int iPrintCounts = iDetialLevel<iCICounts?iDetialLevel:(int)iCICounts;
            printf("Conflict node detail:\n");
            for(i = 0;i<iPrintCounts;++i)
            {
                    if(i%10 == 0){printf("\n");}
                    printf("[%d:%d] ",i,stIndexInfo.pConflictIndexNode[i].iNextPos);
            }
            printf("\n");
        }
        return 0;
    }

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
		SAFE_DELETE(m_pRowCtrl);
		m_pRowCtrl = new (std::nothrow) TMdbRowCtrl;
		CHECK_OBJ(m_pRowCtrl);
		CHECK_RET(m_pRowCtrl->Init(m_pMdbDsn->sName,pTable),"m_pRowCtrl.AttachTable failed.");//��¼����
        return iRet;
    }

	
    int TMdbHashIndexCtrl::RenameTableIndex(TMdbShmDSN * pMdbShmDsn, TMdbTable* pTable, const char *sNewTableName, int& iFindIndexs)
    {
    	int  iRet = 0;
		CHECK_RET(AttachTable(pMdbShmDsn, pTable),"hash index attach table failed.");
		for(int n=0; n<MAX_SHM_ID; ++n)
		{
		    TADD_DETAIL("Attach (%s) hash Index : Shm-ID no.[%d].", pTable->sTableName, n);
		    char * pBaseIndexAddr = pMdbShmDsn->GetBaseIndex(n);
		    if(pBaseIndexAddr == NULL)
		        continue;

		    TMdbBaseIndexMgrInfo *pBIndexMgr = (TMdbBaseIndexMgrInfo*)pBaseIndexAddr;//��ȡ������������
		    for(int j=0; j<MAX_BASE_INDEX_COUNTS && iFindIndexs<pTable->iIndexCounts; ++j)
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
                ++pNode;
            }
        }
        else
        {
            for(MDB_INT64 n=0; n<iCount; ++n)
            {
                pNode->tData.Clear();
                pNode->iNextPos = -1;
                ++pNode;
            }
        }
        return 0;
    }

int TMdbHashIndexCtrl::InitRePosIndexNode(TMdbReIndexNode* pNode,MDB_INT64 iSize,bool bList)
	{
	    MDB_INT64 iCount = iSize/sizeof(TMdbReIndexNode);
	    if(bList)
	    {
	        //��Ҫ����˫����״̬
	        pNode[iCount - 1].iNextPos = -1;//��β
	        for(MDB_INT64 n=0; n<iCount-1; ++n)
	        {
	            pNode->tData.Clear();
	            pNode->iNextPos = n + 1;//���ɴ�
	            ++pNode;
	        }
	    }
	    else
	    {
	        for(MDB_INT64 n=0; n<iCount; ++n)
	        {
	            pNode->tData.Clear();
	            pNode->iNextPos = -1;
	            ++pNode;
	        }
	    }
	    return 0;
	}	int TMdbHashIndexCtrl::CreateHashNewMutexShm(size_t iShmSize)
    {
        TADD_FUNC("Start.Size=[%lu].",iShmSize);
        int iRet = 0;
        if(MAX_SHM_ID == m_pMdbShmDsn->GetInfo()->iHashMutexShmCnt)
        {
            CHECK_RET(ERR_OS_NO_MEMROY,"can't create new shm,MAX_SHM_COUNTS[%d]",MAX_SHM_ID);
        }
        int iPos = m_pMdbDsn->iHashMutexShmCnt;
        TADD_FLOW("Create mutexShm:[%d],mutex_key[0x%0x]",iPos,m_pMdbDsn->iHashMutexShmKey[iPos]);
        CHECK_RET(TMdbShm::Create(m_pMdbDsn->iHashMutexShmKey[iPos], iShmSize, m_pMdbDsn->iHashMutexShmID[iPos]),
                  " Can't Create mutexIndexShm errno=%d[%s].", errno, strerror(errno));
        TADD_FLOW("Mutex_SHM_ID =[%d].SHM_SIZE=[%lu].",m_pMdbDsn->iHashMutexShmID[iPos],iShmSize);
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Create HashMutex IndexShm:[%d],size=[%luMB]",iPos,iShmSize/(1024*1024));
        m_pMdbDsn->iHashMutexShmCnt ++;
        CHECK_RET(m_pMdbShmDsn->ReAttachIndex(),"ReAttachIndex failed....");//�����´�����shm��ȡ��ӳ���ַ
        //��ʼ����������Ϣ
        TMdbHashMutexMgrInfo* pMutexMgr = (TMdbHashMutexMgrInfo*)m_pMdbShmDsn->GetHashMutex(iPos);// ->m_pBaseIndexShmAddr[iPos];
        CHECK_OBJ(pMutexMgr);
        pMutexMgr->iSeq = iPos;
        int i = 0;
        for(i = 0; i<MAX_MHASH_INDEX_COUNT; i++)
        {
            pMutexMgr->aBaseMutex[i].Clear();
            pMutexMgr->tFreeSpace[i].Clear();
        }
        pMutexMgr->tFreeSpace[0].iPosAdd = sizeof(TMdbHashMutexMgrInfo);
        pMutexMgr->tFreeSpace[0].iSize   = iShmSize - sizeof(TMdbHashMutexMgrInfo);

        pMutexMgr->iCounts= 0;
        pMutexMgr->iTotalSize   = iShmSize;
        pMutexMgr->tMutex.Create();
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbHashIndexCtrl::GetHashFreeMutexShm(MDB_INT64 iMutexSize,size_t iDataSize,ST_HASH_INDEX_INFO & stTableIndexInfo)
    {
        TADD_FUNC("Start.iMutexSize=[%lld].iDataSize=[%lu]",iMutexSize,iDataSize);
        int iRet = 0;
        if((size_t)iMutexSize > iDataSize - 10*1024*1024)
        {
            //����ռ�̫��һ���ڴ�鶼������//Ԥ��10M�ռ�
            CHECK_RET(-1,"DataSize is[%luM],it's too small,must > [%lldM],please change it",
                      iDataSize/1024/1024, iMutexSize/1024/1024);
        }
        bool bFind = false;
        TMdbHashMutexMgrInfo* pMutexMgr     = NULL;
        int i = 0;
        for(i = 0; i < MAX_SHM_ID; i++)
        {
            pMutexMgr = (TMdbHashMutexMgrInfo*)m_pMdbShmDsn->GetHashMutex(i);
            if(NULL ==  pMutexMgr ) //��Ҫ�����µ������ڴ�
            {
                CHECK_RET(CreateHashNewMutexShm(iDataSize),"CreateNewBIndexShm[%d]failed",i);
                pMutexMgr = (TMdbHashMutexMgrInfo*)m_pMdbShmDsn->GetHashMutex(i);
                CHECK_OBJ(pMutexMgr);
            }
            CHECK_RET(pMutexMgr->tMutex.Lock(true),"Lock Faild");
            do
            {
                int j = 0;
                TMdbHashBaseMutex* pMutex = NULL;
                //��Ѱ���Է���������Ϣ��λ��
                for(j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
                {
                    if('0' == pMutexMgr->aBaseMutex[j].cState)
                    {
                        //δ������
                        pMutex = &(pMutexMgr->aBaseMutex[j]);
                        stTableIndexInfo.pBaseIndex->iMutexMgrPos= i;
                        stTableIndexInfo.pBaseIndex->iMutexPos= j;
                        stTableIndexInfo.iMutexPos= j;
                        break;
                    }
                }
                if(NULL == pMutex)
                {
                    break;   //û�п���λ�ÿ��Է�������Ϣ
                }
                //��Ѱ�Ƿ��п����ڴ�
                for(j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
                {
                    if(pMutexMgr->tFreeSpace[j].iPosAdd >0)
                    {
                        TMDBIndexFreeSpace& tFreeSpace = pMutexMgr->tFreeSpace[j];
                        if(tFreeSpace.iSize >= (size_t)iMutexSize)
                        {
                            pMutex->cState = '2';//����״̬
                            pMutex->iPosAdd   = tFreeSpace.iPosAdd;
                            pMutex->iSize     = iMutexSize;
                            if(tFreeSpace.iSize - iMutexSize > 0)
                            {
                                //����ʣ��ռ�
                                tFreeSpace.iPosAdd += iMutexSize;
                                tFreeSpace.iSize   -= iMutexSize;
								printf("Use Mutex size:%d,Left:%d,[%d][%d].\n",iMutexSize,tFreeSpace.iSize,i,j);
                            }
                            else
                            {
                                //�ÿ���пռ������ù�
                                tFreeSpace.Clear();
                            }
                            CHECK_RET_BREAK(DefragIndexSpace(pMutexMgr->tFreeSpace),"DefragIndexSpace failed...");
                        }
                    }
                }
                CHECK_RET_BREAK(iRet,"iRet = [%d]",iRet);
                if('2' != pMutex->cState)
                {
                    //û�п����ڴ��������ڵ�
                    break;
                }
                else
                {
                    //���뵽�����ڴ��
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
                break;   //�ҵ�
            }
        }
        if(!bFind)
        {
            CHECK_RET(-1,"GetMHashFreeMutexShm failed....");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }    /******************************************************************************
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

	int TMdbHashIndexCtrl::InitBRePosCIndex(ST_HASH_INDEX_INFO & tTableIndex,TMdbTable * pTable)
	{
	    int iRet = 0;
	    //��ʼ����������
        SAFESTRCPY(tTableIndex.pBaseIndex->sTabName, sizeof(tTableIndex.pBaseIndex->sTabName), pTable->sTableName);
	    tTableIndex.pBaseIndex->cState   = '1';
	    TMdbDateTime::GetCurrentTimeStr(tTableIndex.pBaseIndex->sCreateTime);
	    InitIndexNode((TMdbIndexNode *)((char *)tTableIndex.pBIndexMgr + tTableIndex.pBaseIndex->iPosAdd),tTableIndex.pBaseIndex->iSize,false);
	    tTableIndex.pBIndexMgr->iIndexCounts ++;
	    //��ʼ����ͻ����
	    tTableIndex.pConflictIndex->cState = '1';
	    TMdbDateTime::GetCurrentTimeStr(tTableIndex.pConflictIndex->sCreateTime);
	    InitRePosIndexNode((TMdbReIndexNode *)((char *)tTableIndex.pCIndexMgr + tTableIndex.pConflictIndex->iPosAdd),
	                  tTableIndex.pConflictIndex->iSize,true);
	    tTableIndex.pConflictIndex->iFreeHeadPos = 0;//���н��λ��
	    tTableIndex.pConflictIndex->iFreeNodeCounts = tTableIndex.pConflictIndex->iSize/sizeof(TMdbReIndexNode);//��¼���г�ͻ�ڵ����
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
                                        ST_HASH_INDEX_INFO & stTableIndexInfo, bool bRePosIndex)
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
							pConflictIndex->bRePos = bRePosIndex;
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

 int TMdbHashIndexCtrl::CalcBaseMutexCount(int iBaseCont)
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

	//��ͻ������ÿ�����������
	int TMdbHashIndexCtrl::GetCApplyNum(int iBaseCont)
	{
		return iBaseCont/100;
		
	}

    int TMdbHashIndexCtrl::AddTableSingleIndex(TMdbTable * pTable,int iIndexPos, size_t iDataSize)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        TADD_FLOW("AddTableIndex Table=[%s],Size=[%lu] start.", pTable->sTableName,iDataSize);
        MDB_INT64 iBaseIndexSize     = pTable->iRecordCounts * sizeof(TMdbIndexNode); //���㵥������������Ҫ�Ŀռ�
        MDB_INT64 iConflictIndexSize = pTable->iRecordCounts * sizeof(TMdbIndexNode); //���㵥����ͻ������Ҫ�Ŀռ�
        
		int iMutexCount = CalcBaseMutexCount(pTable->iTabLevelCnts);
		MDB_INT64 iMutexSize = iMutexCount * sizeof(TMutex);
		ST_HASH_INDEX_INFO stTableIndex;
        stTableIndex.Clear();
		
        CHECK_RET(GetFreeBIndexShm(iBaseIndexSize, iDataSize,stTableIndex),"GetFreeBIndexShm failed..");
        CHECK_RET(GetHashFreeMutexShm(iMutexSize, iDataSize,stTableIndex),"GetFreeBIndexShm failed..");
		CHECK_RET(InitHashMutex(stTableIndex,pTable),"InitHashMutex failed...");
		SAFESTRCPY(stTableIndex.pMutex->sName, sizeof(stTableIndex.pMutex->sName), pTable->tIndex[iIndexPos].sName);
		
	    if(pTable->tIndex[iIndexPos].IsRedirectIndex())
	    {
	        iConflictIndexSize = pTable->iRecordCounts * sizeof(TMdbReIndexNode);
	        CHECK_RET(GetFreeCIndexShm(iConflictIndexSize, iDataSize,stTableIndex,true),"GetFreeCIndexShm failed...");
	        if('2' == stTableIndex.pBaseIndex->cState)
	        {
	            //�ҵ�����λ��
	            SAFESTRCPY(stTableIndex.pBaseIndex->sName,sizeof(stTableIndex.pBaseIndex->sName),pTable->tIndex[iIndexPos].sName);
				CHECK_RET(InitBRePosCIndex(stTableIndex,pTable),"InitBRePosCIndex failed...");
	        }
	        else
	        {
	            CHECK_RET(-1,"not find pos for new index....");
	        }
	    }
	    else
	    {
	        iConflictIndexSize = pTable->iRecordCounts * sizeof(TMdbIndexNode);
	        CHECK_RET(GetFreeCIndexShm(iConflictIndexSize,iDataSize,stTableIndex,false),"GetFreeCIndexShm failed...");
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
	    }
		
        TADD_FLOW("Index[%s]",pTable->tIndex[iIndexPos].sName);
        TADD_FLOW("AddTableIndex : Table=[%s] finish.", pTable->sTableName);
        return iRet;
    }


  int TMdbHashIndexCtrl::InitHashMutex(ST_HASH_INDEX_INFO & tTableIndex,TMdbTable * pTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //��ʼ����������
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
	int TMdbHashIndexCtrl::InitMutexNode(TMiniMutex* pNode,MDB_INT64 iSize)
    {
        MDB_INT64 iCount = iSize/sizeof(TMiniMutex);
        for(MDB_INT64 n=0; n<iCount; ++n)
        {
            pNode->Create();
            ++pNode;
        }
        return 0;
    }

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

		if(NULL != tIndexInfo.pReConfNode)
		{
		
			//��ʼ����ͻ����
			InitRePosIndexNode((TMdbReIndexNode *)((char *)tIndexInfo.pCIndexMgr +tIndexInfo.pConflictIndex->iPosAdd),
						  tIndexInfo.pConflictIndex->iSize,true);
			tIndexInfo.pConflictIndex->iFreeHeadPos = 0;//���н��λ��
			tIndexInfo.pConflictIndex->iFreeNodeCounts = tIndexInfo.pConflictIndex->iSize/sizeof(TMdbReIndexNode);//��¼���г�ͻ�ڵ����
		}
		else
		{
			InitIndexNode((TMdbIndexNode *)((char *)tIndexInfo.pCIndexMgr + tIndexInfo.pConflictIndex->iPosAdd),
						  tIndexInfo.pConflictIndex->iSize,true);
			tIndexInfo.pConflictIndex->iFreeHeadPos = 0;//���н��λ��
			tIndexInfo.pConflictIndex->iFreeNodeCounts = tIndexInfo.pConflictIndex->iSize/sizeof(TMdbIndexNode);//��¼���г�ͻ�ڵ����
		}
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
		TMdbReIndexNode* pReConfNode = NULL;
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

				if(pConfIndex->bRePos)
	            {
	                iCICounts =pConfIndex->iSize/sizeof(TMdbReIndexNode);//��ͻ��������
	                pReConfNode = (TMdbReIndexNode*)(pConflictIndexAddr + pConfIndex->iPosAdd);//��ͻ������
	            }
				else
		        {
		            iCICounts =pConfIndex->iSize/sizeof(TMdbIndexNode);//��ͻ��������
		            pConfIndexNode = (TMdbIndexNode*)(pConflictIndexAddr + pConfIndex->iPosAdd);//��ͻ������
		        }
				 

                pBaseIndexNode = (TMdbIndexNode*)(pBaseIndexAddr + pBaseIndex->iPosAdd);//����������

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
						if(pConfIndex->bRePos)
	                    {
	                        iNextPos = pReConfNode[iNextPos].iNextPos;
	                    }
						else
						{
                        	iNextPos = pConfIndexNode[iNextPos].iNextPos;
						}
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


