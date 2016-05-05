#ifndef _MDB_TRIE_INDEX_CTRL_H_
#define _MDB_TRIE_INDEX_CTRL_H_



#include "Helper/mdbDictionary.h"
#include "Helper/SqlParserStruct.h"
#define MAX_VALUE(_x,_y) (_x)<(_y)?(_y):(_x)

/*
    �ֵ�������
*/ 


    class TMdbRowCtrl;

	// ���ڵ�ڵ㡢��ͻ�ڵ���Ϣ
    class TMdbTrieIndexNodeInfo
    {
    public:
        void Clear()
        {
            m_tRowId.Clear();
            m_iBranchPos = -1;
            m_iNextConfPos = -1;
        }
    public:
        TMdbRowID m_tRowId; 	//����λ��
        int m_iBranchPos;		
        int m_iNextConfPos;			
    };


//template<class CharSet,int SIZE>

#define SET_SIZE  10
	//���ڵ�(������֧�ڵ�͸��ڵ�)
    class TMdbTrieIndexNode
    {
    public:
        void Clear()
        {
        	m_ch = 0;
            m_NodeInfo.Clear();
			memset(m_iChildrenPos,-1,sizeof(m_iChildrenPos));
			m_iNextPos = -1;
        }

		bool IsHaveChild()
		{
			for(int i = 0; i++; i<SET_SIZE)
			{
				if(m_iChildrenPos[i]>0) return true;
			}	
			return false;
		}

		int FindNextChild(int iCurChildIdx)
		{
			for(int i=iCurChildIdx;i++;i<SET_SIZE)
			{
				if(m_iChildrenPos[i]>0) return i;
			}

			return -1;
		}
		
    public:
		char m_ch;
		int m_iChildrenPos[SET_SIZE];// ���ӽڵ�λ��
		int m_iNextPos;//���ڿ���������¼��һ�����нڵ��λ��
        TMdbTrieIndexNodeInfo m_NodeInfo; 
    };

    // ��ͻ�ڵ�
    class TMdbTrieConfIndexNode
    {
    public:
        void Clear()
        {
            m_iNextPos = -1;
            m_NodeInfo.Clear();
        }
    public:        
        TMdbTrieIndexNodeInfo m_NodeInfo;         
        int m_iNextPos; // ��һ����ͻ�����ڵ�λ��
    };

	//���ڵ�
	class TMdbTrieRootIndex
	{
	public:
		void Clear()
		{
			memset(sName, 0, sizeof(sName));
			iPosAdd = 0;
			iSize	= 0;
			cState	= '0';
			memset(sTabName, 0, sizeof(sTabName));
			iConflictIndexPos = -1;
			memset(sCreateTime, 0, sizeof(sCreateTime));
			iTrieBranchIndexPos = -1;
		}
		char sName[MAX_NAME_LEN];	//��������
		char sTabName[MAX_NAME_LEN]; // ����
		size_t	iPosAdd;			   //����ƫ����
		size_t	iSize;				   //�������������С����λΪ�ֽ�
		char cState;				//����״̬����0��-δ����;��1��-��ʹ����;��2��-���ڴ���;��3����������;
		char sCreateTime[MAX_TIME_LEN]; //��������ʱ��
		
		int  iConflictIndexPos; 	//��ͻ����pos
		int iTrieBranchIndexPos; // ������pos

		int iTotalNodes;
		int iHeadBlockId;
		int iFreeHeadPos;
		int iFreeNodeCounts;
				
	};
	
	class TMdbTrieRootIndexMgrInfo
	{
	public:
		void Clear()
		{
			iSeq = 0;
			iIndexCounts = 0;
			iTotalSize = 0;
			for(int i = 0; i <MAX_BRIE_INDEX_COUNT; i++ )
			{
				tIndex[i].Clear();
				tFreeSpace[i].Clear();
			}
		}
	public:
		int iSeq;		  //�ڼ��������ڴ��
		TMutex tMutex;	  //������������
		TMdbTrieRootIndex tIndex[MAX_BRIE_INDEX_COUNT]; //����������Ϣ
		int iIndexCounts;  //����������
		TMDBIndexFreeSpace tFreeSpace[MAX_BRIE_INDEX_COUNT];//���пռ�
		MDB_INT64 iTotalSize;	//�ܴ�С
	};


    class TMdbTrieBranchIndex
    {
    public:
        void Clear()
        {
            cState  = '0';
            memset(sCreateTime, 0, sizeof(sCreateTime));
            iFreeHeadPos = -1;
            iFreeNodeCounts = 0;
            iTotalNodes = 0;
            iHeadBlockId = -1;
        }
        size_t GetTotalCount(){return iTotalNodes;}//�ܸ���
        char cState;                //����״̬����0��-δ����;��1��-��ʹ����;��2��-���ڴ���;��3����������
        char sCreateTime[MAX_TIME_LEN]; //��������ʱ��
        int  iFreeHeadPos;         //����ͷ���λ��
        int  iFreeNodeCounts;      //ʣ����нڵ���
        int iTotalNodes;  // �ܵĽڵ���
        int iHeadBlockId;
        
    };

    class TMdbTrieBranchIndexMgrInfo
    {
    public:
        void Clear()
        {
            iIndexCounts = 0;
            for(int i = 0; i <MAX_BRIE_INDEX_COUNT; i++ )
            {
                tIndex[i].Clear();
            }
            iFreeBlockId = -1;
            iTotalBlocks = 0;
        }
    public:
        TMdbTrieBranchIndex tIndex[MAX_BRIE_INDEX_COUNT]; //��ͻ������Ϣ
        int iIndexCounts;  //����������
        int iFreeBlockId; // �������׽ڵ�
        int iTotalBlocks; // ���г�ͻ�����ڴ�����
    };
	

	
    // ��ͻ����
    class TMdbTrieConflictIndex
    {
    public:
        void Clear()
        {
            cState  = '0';
            memset(sCreateTime, 0, sizeof(sCreateTime));
            iFreeHeadPos = -1;
            iFreeNodeCounts = 0;
            iTotalNodes = 0;
            iHeadBlockId = -1;
        }
        size_t GetTotalCount(){return iTotalNodes;}//�ܸ���
        char cState;                //����״̬����0��-δ����;��1��-��ʹ����;��2��-���ڴ���;��3����������
        char sCreateTime[MAX_TIME_LEN]; //��������ʱ��
        int  iFreeHeadPos;         //����ͷ���λ��
        int  iFreeNodeCounts;      //ʣ����нڵ���
        int iTotalNodes;  // �ܵĽڵ���
        int iHeadBlockId;
        
    };

	//��ͻ����������
    class TMdbTrieConflictIndexMgrInfo
    {
    public:
        void Clear()
        {
            iIndexCounts = 0;
            for(int i = 0; i <MAX_BRIE_INDEX_COUNT; i++ )
            {
                tIndex[i].Clear();
            }
            iFreeBlockId = -1;
            iTotalBlocks = 0;
        }
    public:
        TMdbTrieConflictIndex tIndex[MAX_BRIE_INDEX_COUNT]; //��ͻ������Ϣ
        int iIndexCounts;  //����������
        int iFreeBlockId; // �������׽ڵ�
        int iTotalBlocks; // ���г�ͻ�����ڴ�����
    };

	//���������Ϣ����ʱ����
    struct ST_TRIE_INDEX_INFO
    {
        void Clear()
        {
            pRootIndexMgr = NULL;
            pRootIndex = NULL;

			iRootIndexPos  = 0;
			
            pBranchIndexMgr = NULL;
            pBranchIndex = NULL;

			pConflictIndexMgr = NULL;
			pConflictIndex = NULL;

			pRootNode = NULL;
        }
        
        TMdbTrieRootIndexMgrInfo     * pRootIndexMgr;    //�������ڵ������
        TMdbTrieRootIndex            * pRootIndex;    //�������ڵ���Ϣ

		int iRootIndexPos;

		TMdbTrieBranchIndexMgrInfo 		* pBranchIndexMgr;
        TMdbTrieBranchIndex        		* pBranchIndex;//��ͻ�ڵ�������Ϣ
				
		TMdbTrieConflictIndexMgrInfo * pConflictIndexMgr;
        TMdbTrieConflictIndex        * pConflictIndex;//��ͻ�ڵ�������Ϣ

		TMdbTrieIndexNode*  pRootNode;
        
    };

   
    
    
    class TMdbTrieIndexCtrl
    {
    public:
        TMdbTrieIndexCtrl();
        ~TMdbTrieIndexCtrl();

    public:
        // attch shm 
        int AttachDsn(TMdbShmDSN * pMdbShmDsn);
        int AttachTable(TMdbShmDSN * pMdbShmDsn,TMdbTable* pTable);
        
        // add & delete index
        int AddTableSingleIndex(TMdbTable * pTable,int iIndexPos,size_t iDataSize);

        // index node operation (insert & delete & update)
        int InsertIndexNode(char*  sTrieWord,ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID);//���������ڵ�
        int DeleteIndexNode(char*  sTrieWord, ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID);//ɾ�������ڵ�
        int UpdateIndexNode(char*  sTrieOldWord, char*  sNewTrieWord,ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID);//���������ڵ�

        // delete index
        int DeleteTableIndex(ST_TRIE_INDEX_INFO& tIndexInfo);
		int TruncateTableIndex(ST_TRIE_INDEX_INFO& tIndexInfo);

		int OutPutInfo(bool bConsole,const char * fmt, ...);
		
        // print index info
        int PrintIndexInfo(ST_TRIE_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole);
		
		int PrintIndexInfoDetail(int iDetialLevel,bool bConsole, ST_TRIE_INDEX_INFO & stIndexInfo);

	private:

        // create index shm space
        int CreateNewTrieRootIndexShm(size_t iShmSize);//�����µĻ��������ڴ��
                

        // init index node
        int InitTrieNode(TMdbTrieIndexNode* pNode,MDB_INT64 iSize,bool bList);
        
        int GetTrieFreeRootIndexShm(MDB_INT64 iBaseIndexSize,size_t iDataSize,ST_TRIE_INDEX_INFO & stTableIndexInfo);
        int GetFreeConflictShm(MDB_INT64 iConflictSize,size_t iDataSize,TMdbTrieBlock* & pFreeBlock);
		int GetFreeBranchShm(MDB_INT64 iConflictSize,size_t iDataSize,TMdbTrieBlock*& pFreeBlock);
        
        int InitTrieRootIndex(ST_TRIE_INDEX_INFO & tTableIndex,TMdbTable * pTable);
		int InitTrieBranchIndex(ST_TRIE_INDEX_INFO & tTableIndex);
        int InitConflictIndex(ST_TRIE_INDEX_INFO & tTableIndex);
		
        int InsertConflictNode(TMdbTrieIndexNodeInfo& tNodeInfo, ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID);
		int ApplyNewConflictNode(ST_TRIE_INDEX_INFO & tTableIndex);
		int ApplyNewBranchNode(ST_TRIE_INDEX_INFO & tTableIndex);
        
        // Recycle
        int RecycleIndexSpace(TMDBIndexFreeSpace tFreeSpace[],size_t iPosAdd,size_t iSize);
        int DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[]);

        // delete node 
        int DeleteIndexNodeOnConfList(ST_TRIE_INDEX_INFO& tTrieIndex,TMdbTrieIndexNodeInfo* pNodeInfo, TMdbRowID& tRowId,bool & bFound);

        TMdbTrieBlock* GetBlockById(int iBlockID, bool bConf);

        int AddBlock(int& iHeadId, TMdbTrieBlock* pBlockToAdd, bool bConf);

        int RemoveBlock(TMdbTrieBlock* pBlockToDel, int&  iHead, bool bConf);

        char* GetAddrByIndexNodeId(int iHeadBlock,int iIndexNodeId, int iNodeSize, bool bConf);

        int GetFreeConfPos();
		int GetFreeBranchPos();

        

    private:
        TMdbTable * m_pAttachTable;
        TMdbShmDSN * m_pMdbShmDsn;//MDB���������
        TMdbDSN   * m_pMdbDsn;        

        
    };
//}

#endif
