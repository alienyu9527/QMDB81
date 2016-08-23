#ifndef _MDB_M_HASH_INDEX_CTRL_H_
#define _MDB_M_HASH_INDEX_CTRL_H_



#include "Helper/mdbDictionary.h"
#include "Helper/SqlParserStruct.h"
#include "Control/mdbIndexAlgo.h"
#define MAX_VALUE(_x,_y) (_x)<(_y)?(_y):(_x)

/*
    阶梯式索引
*/ 

//namespace QuickMDB{

    class TMdbRowCtrl;

    

    

    // 基础链节点/阶梯链节点的节点信息
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
        int m_iHashValue; // hash  值
        TMdbRowID m_tRowId; //  数据位置
        int m_iLayerPos;
        int m_iConfPos;
    };

    // 基本索引节点
    class TMdbMHashBaseIndexNode
    {
    public:
        void Clear()
        {
            m_tBaseNode.Clear();
        }
    public:
        TMdbMHashIndexNodeInfo m_tBaseNode; // 基本链节点信息
    };

    // 冲突索引节点
    class TMdbMHashConfIndexNode
    {
    public:
        void Clear()
        {
            m_iNextNode = -1;
            m_tRowId.Clear();
        }
    public:
        TMdbRowID m_tRowId; // 数据位置
        int m_iNextNode; // 下一个冲突索引节点位置
    };

    // 阶梯索引节点
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
        TMdbMHashIndexNodeInfo m_atNode[MAX_BODY_NODE_NUM]; // 阶梯索引节点包含的节点
    };

    // 基本索引
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
        char sName[MAX_NAME_LEN];   //索引名称
        char sTabName[MAX_NAME_LEN]; // 表名
        size_t  iPosAdd;               //索引偏移量
        size_t  iSize;                 //基础索引总体大小，单位为字节
        char cState;                //索引状态：’0’-未创建;’1’-在使用中;’2’-正在创建;’3’正在销毁;
        char sCreateTime[MAX_TIME_LEN]; //索引创建时间
        int  iConflictIndexPos;     //冲突索引pos
        int iLayerIndexPos; // 阶梯式索引pos
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
        int iSeq;         //第几个共享内存段
        TMutex tMutex;    //管理区共享锁
        TMdbMHashBaseIndex tIndex[MAX_MHASH_INDEX_COUNT]; //基础索引信息
        int iIndexCounts;  //已有索引数
        TMDBIndexFreeSpace tFreeSpace[MAX_MHASH_INDEX_COUNT];//空闲空间
        MDB_INT64 iTotalSize;   //总大小
    };


    // 基本链节点对应的锁
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
        char sName[MAX_NAME_LEN];   //索引名称
        char sTabName[MAX_NAME_LEN]; // 表名
        size_t  iPosAdd;               //索引偏移量
        size_t  iSize;                 //锁大小，单位为字节
        char cState;                //状态：’0’-未创建;’1’-在使用中;’2’-正在创建;’3’正在销毁;
        char sCreateTime[MAX_TIME_LEN]; //索引创建时间
        int  iMutextMgrPos;         // 锁管理区的pos
        int  iMutexPos;     // 锁pos
        int iMutexCnt; // 锁个数
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
        int iSeq;         //第几个共享内存段
        TMutex tMutex;    //管理区共享锁
        TMdbMHashBaseMutex aBaseMutex[MAX_MHASH_INDEX_COUNT]; //基础索引锁信息
        int iCounts;  //已有索引数
        TMDBIndexFreeSpace tFreeSpace[MAX_MHASH_INDEX_COUNT];//空闲空间
        MDB_INT64 iTotalSize;   //总大小
        
    };

    // 冲突索引
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
        size_t GetTotalCount(){return static_cast<size_t>(iTotalNodes);}//总个数
        char cState;                //索引状态：’0’-未创建;’1’-在使用中;’2’-正在创建;’3’正在销毁
        char sCreateTime[MAX_TIME_LEN]; //索引创建时间
        int  iFreeHeadPos;         //空闲头结点位置
        int  iFreeNodeCounts;      //剩余空闲节点数
        int iTotalNodes;  // 总的节点数
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
        TMdbMHashConflictIndex tIndex[MAX_MHASH_INDEX_COUNT]; //冲突索引信息
        int iIndexCounts;  //已有索引数
        int iFreeBlockId; // 空闲链首节点
        int iTotalBlocks; // 已有冲突索引内存块个数
    };


    // 阶梯索引
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
        size_t GetTotalCount(){return static_cast<size_t>(iTotalNodes);}//总个数
        char cState;                //索引状态：’0’-未创建;’1’-在使用中;’2’-正在创建;’3’正在销毁
        char sCreateTime[MAX_TIME_LEN]; //索引创建时间
        int  iFreeHeadPos;         //空闲头结点位置
        int  iFreeNodeCounts;      //剩余空闲节点数
        int iTotalNodes;  // 总的节点数
        int iHeadBlockId;
        int iLayerLimit; // 最高层数限制
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
        TMdbMhashLayerIndex tIndex[MAX_MHASH_INDEX_COUNT]; //阶梯索引信息
        int iIndexCounts;  //已有索引数
        int iFreeBlockId; // 空闲链首节点
        int iTotalBlocks; // 已有冲突索引内存块个数
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
        
        TMdbMHashBaseIndexMgrInfo     * pBIndexMgr;    //基础索引管理区
        TMdbMHashMutexMgrInfo* pMutexMgr;
        
        int 					   iBaseIndexPos; //基础索引位置
        int iMutexPos;
        
        TMdbMHashBaseIndex            * pBaseIndex;    //基础索引信息
        TMdbMHashConflictIndex        * pConflictIndex;//冲突索引信息
        TMdbMhashLayerIndex* pLayerIndex; // 阶梯索引信息            
        TMdbMHashBaseMutex* pMutex;
        
        TMdbMHashBaseIndexNode            * pBaseIndexNode;//基础索引链
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
        int InsertIndexNode(long long iHashValue,ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID);//插入索引节点
        int DeleteIndexNode(long long iHashValue, ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID);//删除索引节点
        int UpdateIndexNode(long long iOldHashValue, long long iNewHashValue,ST_MHASH_INDEX_INFO& tMHashIndex, TMdbRowID& rowID);//更新索引节点

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
        int CreateMHashNewBIndexShm(size_t iShmSize);//创建新的基础索引内存块
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

         // bConf: true -- 查找冲突索引块链表； false -- 查找阶梯索引块链表
        TMdbMhashBlock* GetBlockById(int iBlockID, bool bConf);

        // bConf: true -- 处理的是冲突索引块； false -- 处理的是阶梯索引块
        int AddBlock(int& iHeadId, TMdbMhashBlock* pBlockToAdd, bool bConf);

        // bConf: true -- 处理的是冲突索引块； false -- 处理的是阶梯索引块
        int RemoveBlock(TMdbMhashBlock* pBlockToDel, int&  iHead, bool bConf);

        // bConf: true -- 处理的是冲突索引块； false -- 处理的是阶梯索引块
        char* GetAddrByIndexNodeId(int iHeadBlock,int iIndexNodeId, int iNodeSize, bool bConf);

        int GetFreeConfPos();
        int GetFreeLayerPos();


        // 根据表的基础索引节点数，划分锁个数
        int CalcBaseMutexCount(int iBaseCont);
        

    private:
        TMdbTable * m_pAttachTable;
        TMdbShmDSN * m_pMdbShmDsn;//MDB共享管理区
        TMdbDSN   * m_pMdbDsn;   
		TMdbLocalLink* m_pLink;
        MDB_INT64 m_lSelectIndexValue;//正在查询的索引冲突链
        
    };
//}

#endif
