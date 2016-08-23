#ifndef _MDB_M_HASH_INDEX_CTRL_H_
#define _MDB_M_HASH_INDEX_CTRL_H_



#include "Helper/mdbDictionary.h"
#include "Helper/SqlParserStruct.h"
#include "Control/mdbIndexAlgo.h"
#define MAX_VALUE(_x,_y) (_x)<(_y)?(_y):(_x)

/*
    ����ʽ����
*/ 

//namespace QuickMDB{

    class TMdbRowCtrl;

    

    

    // �������ڵ�/�������ڵ�Ľڵ���Ϣ
    class TMdbMHashIndexNodeInfo
    {
    public:
        void Clear()
        {
            m_iHashValue = 0;
            m_tRowId.Clear();
            m_iLayerPos = -1;
            m_iConfPos = -1;
        }
    public:
        int m_iHashValue; // hash  ֵ
        TMdbRowID m_tRowId; //  ����λ��
        int m_iLayerPos;
        int m_iConfPos;
    };

    // ���������ڵ�
    class TMdbMHashBaseIndexNode
    {
    public:
        void Clear()
        {
            m_tBaseNode.Clear();
        }
    public:
        TMdbMHashIndexNodeInfo m_tBaseNode; // �������ڵ���Ϣ
    };

    // ��ͻ�����ڵ�
    class TMdbMHashConfIndexNode
    {
    public:
        void Clear()
        {
            m_iNextNode = -1;
            m_tRowId.Clear();
        }
    public:
        TMdbRowID m_tRowId; // ����λ��
        int m_iNextNode; // ��һ����ͻ�����ڵ�λ��
    };

    // ���������ڵ�
    class TMdbMhashLayerIndexNode
    {
    public:
        void Clear()
        {
            m_iLayeCnt = 0;
            m_iNextPos = -1;
            for(int i =0; i < MAX_BODY_NODE_NUM; i++)
            {
                m_atNode[i].Clear();
            }
        }
        
    public:
        int m_iLayeCnt;        
        int m_iNextPos;
        TMdbMHashIndexNodeInfo m_atNode[MAX_BODY_NODE_NUM]; // ���������ڵ�����Ľڵ�
    };

    // ��������
    class TMdbMHashBaseIndex
    {
    public:
        void Clear()
        {
            memset(sName, 0, sizeof(sName));
            iPosAdd = 0;
            iSize   = 0;
            cState  = '0';
            memset(sTabName, 0, sizeof(sTabName));
            iConflictIndexPos = -1;
            memset(sCreateTime, 0, sizeof(sCreateTime));
            iLayerIndexPos = -1;
            iMutexMgrPos = -1;
            iMutexPos = -1;
        }
        char sName[MAX_NAME_LEN];   //��������
        char sTabName[MAX_NAME_LEN]; // ����
        size_t  iPosAdd;               //����ƫ����
        size_t  iSize;                 //�������������С����λΪ�ֽ�
        char cState;                //����״̬����0��-δ����;��1��-��ʹ����;��2��-���ڴ���;��3����������;
        char sCreateTime[MAX_TIME_LEN]; //��������ʱ��
        int  iConflictIndexPos;     //��ͻ����pos
        int iLayerIndexPos; // ����ʽ����pos
        int iMutexMgrPos;
        int iMutexPos;
    };

    class TMdbMHashBaseIndexMgrInfo
    {
    public:
        void Clear()
        {
            iSeq = 0;
            iIndexCounts = 0;
            iTotalSize = 0;
            for(int i = 0; i <MAX_MHASH_INDEX_COUNT; i++ )
            {
                tIndex[i].Clear();
                tFreeSpace[i].Clear();
            }
        }
    public:
        int iSeq;         //�ڼ��������ڴ��
        TMutex tMutex;    //������������
        TMdbMHashBaseIndex tIndex[MAX_MHASH_INDEX_COUNT]; //����������Ϣ
        int iIndexCounts;  //����������
        TMDBIndexFreeSpace tFreeSpace[MAX_MHASH_INDEX_COUNT];//���пռ�
        MDB_INT64 iTotalSize;   //�ܴ�С
    };


    // �������ڵ��Ӧ����
    class TMdbMHashBaseMutex
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

    class TMdbMHashMutexMgrInfo
    {
    public:
        void Clear()
        {
            iSeq = 0;
            iCounts = 0;
            iTotalSize = 0;
            for(int i = 0; i <MAX_MHASH_INDEX_COUNT; i++ )
            {
                aBaseMutex[i].Clear();
                tFreeSpace[i].Clear();
            }
        }

    public:
        int iSeq;         //�ڼ��������ڴ��
        TMutex tMutex;    //������������
        TMdbMHashBaseMutex aBaseMutex[MAX_MHASH_INDEX_COUNT]; //������������Ϣ
        int iCounts;  //����������
        TMDBIndexFreeSpace tFreeSpace[MAX_MHASH_INDEX_COUNT];//���пռ�
        MDB_INT64 iTotalSize;   //�ܴ�С
        
    };

    // ��ͻ����
    class TMdbMHashConflictIndex
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
        size_t GetTotalCount(){return static_cast<size_t>(iTotalNodes);}//�ܸ���
        char cState;                //����״̬����0��-δ����;��1��-��ʹ����;��2��-���ڴ���;��3����������
        char sCreateTime[MAX_TIME_LEN]; //��������ʱ��
        int  iFreeHeadPos;         //����ͷ���λ��
        int  iFreeNodeCounts;      //ʣ����нڵ���
        int iTotalNodes;  // �ܵĽڵ���
        int iHeadBlockId;
        
    };


    class TMdbMHashConflictIndexMgrInfo
    {
    public:
        void Clear()
        {
            iIndexCounts = 0;
            for(int i = 0; i <MAX_MHASH_INDEX_COUNT; i++ )
            {
                tIndex[i].Clear();
            }
            iFreeBlockId = -1;
            iTotalBlocks = 0;
        }
    public:
        TMdbMHashConflictIndex tIndex[MAX_MHASH_INDEX_COUNT]; //��ͻ������Ϣ
        int iIndexCounts;  //����������
        int iFreeBlockId; // �������׽ڵ�
        int iTotalBlocks; // ���г�ͻ�����ڴ�����
    };


    // ��������
    class TMdbMhashLayerIndex
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
            iLayerLimit = 1;
        }
        size_t GetTotalCount(){return static_cast<size_t>(iTotalNodes);}//�ܸ���
        char cState;                //����״̬����0��-δ����;��1��-��ʹ����;��2��-���ڴ���;��3����������
        char sCreateTime[MAX_TIME_LEN]; //��������ʱ��
        int  iFreeHeadPos;         //����ͷ���λ��
        int  iFreeNodeCounts;      //ʣ����нڵ���
        int iTotalNodes;  // �ܵĽڵ���
        int iHeadBlockId;
        int iLayerLimit; // ��߲�������
    };

    class TMdbMHashLayerIndexMgrInfo
    {
    public:
        void Clear()
        {
            iTotalBlocks = 0;
            iIndexCounts = 0;
            iFreeBlockId = 0;
            for(int i = 0; i <MAX_MHASH_INDEX_COUNT; i++ )
            {
                tIndex[i].Clear();
            }
        }
        
    public:
        TMdbMhashLayerIndex tIndex[MAX_MHASH_INDEX_COUNT]; //����������Ϣ
        int iIndexCounts;  //����������
        int iFreeBlockId; // �������׽ڵ�
        int iTotalBlocks; // ���г�ͻ�����ڴ�����
    };


    struct ST_MHASH_INDEX_INFO
    {
        void Clear()
        {
            pBIndexMgr = NULL;
            iBaseIndexPos = -1;
            pBaseIndex = NULL;
            pConflictIndex = NULL;
            pLayerIndex = NULL;
            pBaseIndexNode = NULL;
            pMutexMgr = NULL;
            iMutexPos = -1;
            pMutex = NULL;
            pMutexNode = NULL;
        }
        
        TMdbMHashBaseIndexMgrInfo     * pBIndexMgr;    //��������������
        TMdbMHashMutexMgrInfo* pMutexMgr;
        
        int 					   iBaseIndexPos; //��������λ��
        int iMutexPos;
        
        TMdbMHashBaseIndex            * pBaseIndex;    //����������Ϣ
        TMdbMHashConflictIndex        * pConflictIndex;//��ͻ������Ϣ
        TMdbMhashLayerIndex* pLayerIndex; // ����������Ϣ            
        TMdbMHashBaseMutex* pMutex;
        
        TMdbMHashBaseIndexNode            * pBaseIndexNode;//����������
        TMiniMutex* pMutexNode;
    };

   
    
    
    class TMdbMHashIndexCtrl
    {
    public:
        TMdbMHashIndexCtrl();
        ~TMdbMHashIndexCtrl();

    public:
        // attch shm 
        int AttachDsn(TMdbShmDSN * pMdbShmDsn);
        int AttachTable(TMdbShmDSN * pMdbShmDsn,TMdbTable* pTable);		
		void SetLink(TMdbLocalLink* pLink){m_pLink=pLink;}
        
        // add & delete index
        int AddTableSingleIndex(TMdbTable * pTable,int iIndexPos,size_t iDataSize);
		int RenameTableIndex(TMdbShmDSN * pMdbShmDsn, TMdbTable* pTable, const char *sNewTableName, int& iFindIndexs);

        // index node operation (insert & delete & update)
        int InsertIndexNode(long long iHashValue,ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID);//���������ڵ�
        int DeleteIndexNode(long long iHashValue, ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID);//ɾ�������ڵ�
        int UpdateIndexNode(long long iOldHashValue, long long iNewHashValue,ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID);//���������ڵ�

        // delete index
        int DeleteTableIndex(ST_MHASH_INDEX_INFO& tIndexInfo);
		int TruncateTableIndex(ST_MHASH_INDEX_INFO& tIndexInfo);

		int OutPutInfo(bool bConsole,const char * fmt, ...);
		
        // print index info
        int PrintIndexInfo(ST_MHASH_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole);
		
		int PrintIndexInfoDetail(int iDetialLevel,bool bConsole, ST_MHASH_INDEX_INFO & stIndexInfo);
		int CalcIndexDepthAndConflict(ST_MHASH_INDEX_INFO & stIndexInfo,int iPos,int& iTotalConflict,int& iTotalDepth,int iCurDepth);

	private:

        // create index shm space
        int CreateMHashNewBIndexShm(size_t iShmSize);//�����µĻ��������ڴ��
        int CreateMHashNewMutexShm(size_t iShmSize);
        int CreateConflictIndex(ST_MHASH_INDEX_INFO & tTableIndex);
        int CreateLayerIndex(ST_MHASH_INDEX_INFO & tTableIndex, const int iMaxLayer);
                

        // init index node
        int InitBaseIndexNode(TMdbMHashBaseIndexNode* pNode,MDB_INT64 iSize,bool bList);
        int InitLayerIndexNode(TMdbMhashLayerIndexNode* pNode,MDB_INT64 iSize,bool bList); 
        int InitMutexNode(TMiniMutex* pNode,MDB_INT64 iSize);
        
        int GetMHashFreeBIndexShm(MDB_INT64 iBaseIndexSize,size_t iDataSize,ST_MHASH_INDEX_INFO & stTableIndexInfo);
        int GetMHashFreeMutexShm(MDB_INT64 iMutexSize,size_t iDataSize,ST_MHASH_INDEX_INFO & stTableIndexInfo);        
        int GetFreeConflictShm(MDB_INT64 iConflictSize,size_t iDataSize,TMdbMhashBlock* & pFreeBlock);
        int GetFreeLayerShm(MDB_INT64 iLayerSize,size_t iDataSize,TMdbMhashBlock* & pFreeBlock);
        
        
        int InitMHashBaseIndex(ST_MHASH_INDEX_INFO & tTableIndex,TMdbTable * pTable);
        int InitMHashMutex(ST_MHASH_INDEX_INFO & tTableIndex,TMdbTable * pTable);
        
        int InsertConflictNode(TMdbMHashIndexNodeInfo& tNodeInfo, ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID);
        int ApplyNewConflictNode(ST_MHASH_INDEX_INFO & tTableIndex);
        
        int InsertToLayerList(long long iHashValue,int iCurLayer, TMdbMHashIndexNodeInfo * pLastLayerNode,ST_MHASH_INDEX_INFO& tMHashIndex,TMdbRowID& rowID);
        int ApplyNewLayerNode(ST_MHASH_INDEX_INFO & tTableIndex);
        
        // Recycle
        int RecycleIndexSpace(TMDBIndexFreeSpace tFreeSpace[],size_t iPosAdd,size_t iSize);
        int DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[]);

        // delete node 
        int DeleteIndexNodeOnConfList(ST_MHASH_INDEX_INFO& tMHashIndex,TMdbMHashIndexNodeInfo* pNodeInfo, TMdbRowID& tRowId,bool & bFound);
        int DeleteIndexNodeOnLayerList(ST_MHASH_INDEX_INFO& tMHashIndex,TMdbMHashIndexNodeInfo* pNodeInfo, long long iHashValue,TMdbRowID& tRowId,bool & bFound);

         // bConf: true -- ���ҳ�ͻ���������� false -- ���ҽ�������������
        TMdbMhashBlock* GetBlockById(int iBlockID, bool bConf);

        // bConf: true -- ������ǳ�ͻ�����飻 false -- ������ǽ���������
        int AddBlock(int& iHeadId, TMdbMhashBlock* pBlockToAdd, bool bConf);

        // bConf: true -- ������ǳ�ͻ�����飻 false -- ������ǽ���������
        int RemoveBlock(TMdbMhashBlock* pBlockToDel, int&  iHead, bool bConf);

        // bConf: true -- ������ǳ�ͻ�����飻 false -- ������ǽ���������
        char* GetAddrByIndexNodeId(int iHeadBlock,int iIndexNodeId, int iNodeSize, bool bConf);

        int GetFreeConfPos();
        int GetFreeLayerPos();


        // ���ݱ�Ļ��������ڵ���������������
        int CalcBaseMutexCount(int iBaseCont);
        

    private:
        TMdbTable * m_pAttachTable;
        TMdbShmDSN * m_pMdbShmDsn;//MDB���������
        TMdbDSN   * m_pMdbDsn;   
		TMdbLocalLink* m_pLink;
        MDB_INT64 m_lSelectIndexValue;//���ڲ�ѯ��������ͻ��
        
    };
//}

#endif
