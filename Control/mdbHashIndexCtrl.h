#ifndef _MDB_HASH_INDEX_CTRL_H_
#define _MDB_HASH_INDEX_CTRL_H_


#include "Helper/mdbDictionary.h"
#include "Helper/mdbConfig.h"

//namespace QuickMDB{

    class TMdbRowCtrl;
	struct ST_LINK_INDEX_INFO;
    
    struct ST_HASH_INDEX_INFO
    {
        
        TMdbBaseIndexMgrInfo     * pBIndexMgr;    //��������������
        TMdbConflictIndexMgrInfo * pCIndexMgr;	  //��ͻ����������
        int 					   iBaseIndexPos; //��������λ��
        int 					   iConflictIndexPos;//��ͻ����λ��
        TMdbBaseIndex            * pBaseIndex;    //����������Ϣ
        TMdbConflictIndex        * pConflictIndex;//��ͻ������Ϣ
        TMdbIndexNode            * pBaseIndexNode;//����������
        TMdbIndexNode            * pConflictIndexNode;//��ͻ������

        void Clear()
        {
            pBIndexMgr = NULL;
            pCIndexMgr = NULL;
            iBaseIndexPos = -1;
            iConflictIndexPos = -1;
            pBaseIndex = NULL;
            pConflictIndex = NULL;
            pBaseIndexNode = NULL;
            pConflictIndexNode = NULL;
        }
        
    };

    class TMdbHashIndexCtrl
    {
    public:
        TMdbHashIndexCtrl();
        ~TMdbHashIndexCtrl();
        //������������
        int AttachDsn(TMdbShmDSN * pMdbShmDsn);//�����Ϲ����ڴ������
        int AttachTable(TMdbShmDSN * pMdbShmDsn, TMdbTable* pTable);		
		void SetLink(TMdbLocalLink* pLink){m_pLink=pLink;}
		
        int AddTableSingleIndex(TMdbTable * pTable,int iIndexPos,size_t iDataSize);
        int DeleteTableIndex(ST_HASH_INDEX_INFO& tIndexInfo);//ɾ��ĳ���������
        int TruncateTableIndex(ST_HASH_INDEX_INFO& tIndexInfo);
        int InsertIndexNode(int iIndexPos,TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndex, TMdbRowID& rowID);//���������ڵ�
        int UpdateIndexNode(int iIndexPos,TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId);//���������ڵ�
        int DeleteIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID);
        int SelectIndexNode(MDB_INT64 iIndexValue);//���ڲ�ѯ�������ڵ�
        int PrintIndexInfo(ST_HASH_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole);//��ӡ��ϸ������ϢiDetialLevel=[0~3] 0-������Ϣ��1-һ����Ϣ��2-��ϸ��Ϣ
        int PrintIndexInfoDetail(int iDetialLevel,bool bConsole,ST_HASH_INDEX_INFO & stIndexInfo);//��ӡ��ϸ������ϢiDetialLevel=[0~3] 0-������Ϣ��1-һ����Ϣ��2-��ϸ��Ϣ
        int PrintWarnIndexInfo(int iMaxCNodeCount);//��ӡ���ڳ�ͻ������ָ�����ȵ�����
        int FindRowIndexCValue(ST_HASH_INDEX_INFO tHashIndexInfo,TMdbRowIndex & tRowIndex,TMdbRowID& rowID);//���ҳ�ͻ����ֵ

		int GetFreeConflictNode(ST_LINK_INDEX_INFO& tLinkIndexInfo, ST_HASH_INDEX_INFO& tTableHashIndex);

	private:
        int InitBCIndex(ST_HASH_INDEX_INFO & tTableIndex,TMdbTable * pTable);
        int InitIndexNode(TMdbIndexNode* pNode,MDB_INT64 iSize,bool bList);
        int GetFreeBIndexShm(MDB_INT64 iBaseIndexSize,size_t iDataSize,
            ST_HASH_INDEX_INFO & stTableIndexInfo);//��ȡ���������ڴ��
        int GetFreeCIndexShm(MDB_INT64 iConflictIndexSize,size_t iDataSize,
            ST_HASH_INDEX_INFO & stTableIndexInfo);//��ȡ���������ڴ��
        int CreateNewBIndexShm(size_t iShmSize);//�����µĻ��������ڴ��
        int CreateNewCIndexShm(size_t iShmSize);//�����µĳ�ͻ�����ڴ��
        int OutPutInfo(bool bConsole,const char * fmt, ...);//�����Ϣ
        int GetTableByIndexName(const char * sIndexName,TMdbTable * & pTable);//������������ȡ��
        int RecycleIndexSpace(TMDBIndexFreeSpace tFreeSpace[],size_t iPosAdd,size_t iSize);//���������ռ�
        int DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[]);//���������ռ�
        int PrintIndexSpace(const char * sPreInfo,TMDBIndexFreeSpace tFreeSpace[]);//��ӡ�����ռ���Ϣ

     
    private:
        TMdbTable * m_pAttachTable;
        TMdbShmDSN * m_pMdbShmDsn;//MDB���������
        TMdbDSN   * m_pMdbDsn;
		TMdbLocalLink *m_pLink;
                
        MDB_INT64 m_lSelectIndexValue;//���ڲ�ѯ��������ͻ��
        
    };
//}

#endif
