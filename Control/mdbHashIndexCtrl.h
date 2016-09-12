#ifndef _MDB_HASH_INDEX_CTRL_H_
#define _MDB_HASH_INDEX_CTRL_H_


#include "Helper/mdbDictionary.h"
#include "Helper/mdbConfig.h"


//namespace QuickMDB{
	struct ST_LINK_INDEX_INFO;
	class TMdbHashMutexMgrInfo;
	class TMdbHashBaseMutex;    struct ST_HASH_INDEX_INFO
    {
        
        TMdbBaseIndexMgrInfo     * pBIndexMgr;    //��������������
        TMdbConflictIndexMgrInfo * pCIndexMgr;	  //��ͻ����������        
        TMdbHashMutexMgrInfo	 * pMutexMgr;
		
        int 					   iBaseIndexPos; //��������λ��
        int 					   iConflictIndexPos;//��ͻ����λ��
        int 				       iMutexPos;
        
        TMdbBaseIndex            * pBaseIndex;    //����������Ϣ
        TMdbConflictIndex        * pConflictIndex;//��ͻ������Ϣ
        
        TMdbIndexNode            * pBaseIndexNode;//����������
        TMdbIndexNode            * pConflictIndexNode;//��ͻ������
		TMdbReIndexNode 		 * pReConfNode; 
		TMdbHashBaseMutex		 * pMutex;
        TMiniMutex* pMutexNode;
        void Clear()
        {
            pBIndexMgr = NULL;
            pCIndexMgr = NULL;
			pMutexMgr = NULL;
            iBaseIndexPos = -1;
            iConflictIndexPos = -1;
			iMutexPos = -1;
            pBaseIndex = NULL;
            pConflictIndex = NULL;
			pMutex = NULL;			
            pBaseIndexNode = NULL;
            pConflictIndexNode = NULL;
			pReConfNode = NULL;
			pMutexNode = NULL;        }
        
    };
	
	// �������ڵ��Ӧ����
    class TMdbHashBaseMutex
    {
    public:
        void Clear()
        {
            memset(sName, 0, sizeof(sName));
            iPosAdd = 0;
            iSize   = 0;
            cState  = '0';
            memset(sTabName, 0, sizeof(sTabName));
            memset(sCreateTime, 0, sizeof(sCreateTime));
            iMutextMgrPos = -1;
            iMutexPos = -1;
            iMutexCnt = 1;
        }
        char sName[MAX_NAME_LEN];   //��������
        char sTabName[MAX_NAME_LEN]; // ����
        size_t  iPosAdd;               //����ƫ����
        size_t  iSize;                 //����С����λΪ�ֽ�
        char cState;                //״̬����0��-δ����;��1��-��ʹ����;��2��-���ڴ���;��3����������;
        char sCreateTime[MAX_TIME_LEN]; //��������ʱ��
        int  iMutextMgrPos;         // ����������pos
        int  iMutexPos;     // ��pos
        int iMutexCnt; // ������
    };

    class TMdbHashMutexMgrInfo
    {
    public:
        void Clear()
        {
            iSeq = 0;
            iCounts = 0;
            iTotalSize = 0;
            for(int i = 0; i <MAX_BASE_INDEX_COUNTS; i++ )
            {
                aBaseMutex[i].Clear();
                tFreeSpace[i].Clear();
            }
        }

    public:
        int iSeq;         //�ڼ��������ڴ��
        TMutex tMutex;    //������������
        TMdbHashBaseMutex aBaseMutex[MAX_BASE_INDEX_COUNTS]; //������������Ϣ
        int iCounts;  //����������
        TMDBIndexFreeSpace tFreeSpace[MAX_BASE_INDEX_COUNTS];//���пռ�
        MDB_INT64 iTotalSize;   //�ܴ�С
        
    };
	
    class TMdbRowCtrl;	
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

		int InsertIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndex, TMdbRowID& rowID);//���������ڵ�
	    int UpdateIndexNode(TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId);//���������ڵ�
	    int DeleteIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID);

		int InsertIndexNode2(int iIndexPos,TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndex, TMdbRowID& rowID);//���������ڵ�
        int UpdateIndexNode2(int iIndexPos,TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId);//���������ڵ�
        int DeleteIndexNode2(int iIndexPos,TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID);

		int PrintIndexInfo(ST_HASH_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole);//��ӡ��ϸ������ϢiDetialLevel=[0~3] 0-������Ϣ��1-һ����Ϣ��2-��ϸ��Ϣ
        int PrintIndexInfoDetail(int iDetialLevel,bool bConsole,ST_HASH_INDEX_INFO & stIndexInfo);//��ӡ��ϸ������ϢiDetialLevel=[0~3] 0-������Ϣ��1-һ����Ϣ��2-��ϸ��Ϣ
		int PrintRePosIndexInfoDetail(int iDetialLevel,bool bConsole, ST_HASH_INDEX_INFO & stIndexInfo);        int PrintWarnIndexInfo(int iMaxCNodeCount);//��ӡ���ڳ�ͻ������ָ�����ȵ�����
        int FindRowIndexCValue(ST_HASH_INDEX_INFO tHashIndexInfo,TMdbRowIndex & tRowIndex,TMdbRowID& rowID);//���ҳ�ͻ����ֵ
	
		int GetCApplyNum(int iBaseCont);
		int ApplyConflictNodeFromTable(ST_LINK_INDEX_INFO& tLinkIndexInfo, ST_HASH_INDEX_INFO& tTableHashIndex);
		int ReturnConflictNodeToTable(ST_LINK_INDEX_INFO& tLinkIndexInfo, ST_HASH_INDEX_INFO& tTableHashIndex);
		int ReturnAllIndexNodeToTable(ST_LINK_INDEX_INFO& tLinkIndexInfo, ST_HASH_INDEX_INFO& tTableHashIndex);
		int RenameTableIndex(TMdbShmDSN * pMdbShmDsn, TMdbTable* pTable, const char *sNewTableName, int& iFindIndexs);
		int InsertRedirectIndexNode(int iIndexPos,char* pAddr,TMdbRowIndex &tRowIndex, ST_HASH_INDEX_INFO & tHashIndex, TMdbRowID& rowID);
		int DeleteRedirectIndexNode(int iIndexPos,char* pAddr,TMdbRowIndex & tRowIndex,ST_HASH_INDEX_INFO & tHashIndex,TMdbRowID& rowID);
		int UpdateRedirectIndexNode(int iIndexPos,char* pAddr,TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashIndex,TMdbRowID& rowID);

	private:        
		int InitBCIndex(ST_HASH_INDEX_INFO & tTableIndex,TMdbTable * pTable);		
		int InitBRePosCIndex(ST_HASH_INDEX_INFO & tTableIndex,TMdbTable * pTable);
        int InitIndexNode(TMdbIndexNode* pNode,MDB_INT64 iSize,bool bList);
		int InitRePosIndexNode(TMdbReIndexNode* pNode,MDB_INT64 iSize,bool bList);
		int InitHashMutex(ST_HASH_INDEX_INFO & tTableIndex,TMdbTable * pTable);		
		int InitMutexNode(TMiniMutex* pNode,MDB_INT64 iSize);
		
        int GetFreeBIndexShm(MDB_INT64 iBaseIndexSize,size_t iDataSize,ST_HASH_INDEX_INFO & stTableIndexInfo);//��ȡ���������ڴ��
        int GetFreeCIndexShm(MDB_INT64 iConflictIndexSize,size_t iDataSize,ST_HASH_INDEX_INFO & stTableIndexInfo,bool bRePosIndex);//��ȡ���������ڴ��
		int GetHashFreeMutexShm(MDB_INT64 iMutexSize,size_t iDataSize,ST_HASH_INDEX_INFO & stTableIndexInfo);

		int CreateNewBIndexShm(size_t iShmSize);//�����µĻ��������ڴ��
        int CreateNewCIndexShm(size_t iShmSize);//�����µĳ�ͻ�����ڴ��       
		int CreateHashNewMutexShm(size_t iShmSize);
		
		int CalcBaseMutexCount(int iBaseCont);        int OutPutInfo(bool bConsole,const char * fmt, ...);//�����Ϣ
        int GetTableByIndexName(const char * sIndexName,TMdbTable * & pTable);//������������ȡ��
        int RecycleIndexSpace(TMDBIndexFreeSpace tFreeSpace[],size_t iPosAdd,size_t iSize);//���������ռ�
        int DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[]);//���������ռ�
        int PrintIndexSpace(const char * sPreInfo,TMDBIndexFreeSpace tFreeSpace[]);//��ӡ�����ռ���Ϣ


     
    private:
        TMdbTable * m_pAttachTable;
        TMdbShmDSN * m_pMdbShmDsn;//MDB���������
        TMdbDSN   * m_pMdbDsn;
		TMdbRowCtrl*  m_pRowCtrl;//��¼����
        TMdbLocalLink *m_pLink;        MDB_INT64 m_lSelectIndexValue;//���ڲ�ѯ��������ͻ��
        
    };
//}

#endif
