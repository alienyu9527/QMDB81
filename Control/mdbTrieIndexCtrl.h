#ifndef _MDB_TRIE_INDEX_CTRL_H_
#define _MDB_TRIE_INDEX_CTRL_H_



#include "Helper/mdbDictionary.h"
#include "Helper/SqlParserStruct.h"
#define MAX_VALUE(_x,_y) (_x)<(_y)?(_y):(_x)

/*
    字典树索引
*/ 


    class TMdbRowCtrl;

	// 树节点节点、冲突节点信息
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
        TMdbRowID m_tRowId; 	//数据位置
        int m_iBranchPos;		
        int m_iNextConfPos;			
    };


//template<class CharSet,int SIZE>

#define SET_SIZE  10
	//树节点(包含分支节点和根节点)
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
		int m_iChildrenPos[SET_SIZE];// 孩子节点位置
		int m_iNextPos;//用于空闲链，记录下一个空闲节点的位置
        TMdbTrieIndexNodeInfo m_NodeInfo; 
    };

    // 冲突节点
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
        int m_iNextPos; // 下一个冲突索引节点位置
    };

	//根节点
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
		char sName[MAX_NAME_LEN];	//索引名称
		char sTabName[MAX_NAME_LEN]; // 表名
		size_t	iPosAdd;			   //索引偏移量
		size_t	iSize;				   //基础索引总体大小，单位为字节
		char cState;				//索引状态：’0’-未创建;’1’-在使用中;’2’-正在创建;’3’正在销毁;
		char sCreateTime[MAX_TIME_LEN]; //索引创建时间
		
		int  iConflictIndexPos; 	//冲突索引pos
		int iTrieBranchIndexPos; // 树索引pos

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
		int iSeq;		  //第几个共享内存段
		TMutex tMutex;	  //管理区共享锁
		TMdbTrieRootIndex tIndex[MAX_BRIE_INDEX_COUNT]; //基础索引信息
		int iIndexCounts;  //已有索引数
		TMDBIndexFreeSpace tFreeSpace[MAX_BRIE_INDEX_COUNT];//空闲空间
		MDB_INT64 iTotalSize;	//总大小
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
        size_t GetTotalCount(){return iTotalNodes;}//总个数
        char cState;                //索引状态：’0’-未创建;’1’-在使用中;’2’-正在创建;’3’正在销毁
        char sCreateTime[MAX_TIME_LEN]; //索引创建时间
        int  iFreeHeadPos;         //空闲头结点位置
        int  iFreeNodeCounts;      //剩余空闲节点数
        int iTotalNodes;  // 总的节点数
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
        TMdbTrieBranchIndex tIndex[MAX_BRIE_INDEX_COUNT]; //冲突索引信息
        int iIndexCounts;  //已有索引数
        int iFreeBlockId; // 空闲链首节点
        int iTotalBlocks; // 已有冲突索引内存块个数
    };
	

	
    // 冲突索引
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
        size_t GetTotalCount(){return iTotalNodes;}//总个数
        char cState;                //索引状态：’0’-未创建;’1’-在使用中;’2’-正在创建;’3’正在销毁
        char sCreateTime[MAX_TIME_LEN]; //索引创建时间
        int  iFreeHeadPos;         //空闲头结点位置
        int  iFreeNodeCounts;      //剩余空闲节点数
        int iTotalNodes;  // 总的节点数
        int iHeadBlockId;
        
    };

	//冲突索引管理器
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
        TMdbTrieConflictIndex tIndex[MAX_BRIE_INDEX_COUNT]; //冲突索引信息
        int iIndexCounts;  //已有索引数
        int iFreeBlockId; // 空闲链首节点
        int iTotalBlocks; // 已有冲突索引内存块个数
    };

	//存放索引信息的临时变量
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
        
        TMdbTrieRootIndexMgrInfo     * pRootIndexMgr;    //索引根节点管理区
        TMdbTrieRootIndex            * pRootIndex;    //索引根节点信息

		int iRootIndexPos;

		TMdbTrieBranchIndexMgrInfo 		* pBranchIndexMgr;
        TMdbTrieBranchIndex        		* pBranchIndex;//冲突节点索引信息
				
		TMdbTrieConflictIndexMgrInfo * pConflictIndexMgr;
        TMdbTrieConflictIndex        * pConflictIndex;//冲突节点索引信息

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
        int InsertIndexNode(char*  sTrieWord,ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID);//插入索引节点
        int DeleteIndexNode(char*  sTrieWord, ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID);//删除索引节点
        int UpdateIndexNode(char*  sTrieOldWord, char*  sNewTrieWord,ST_TRIE_INDEX_INFO& tTrieIndex, TMdbRowID& rowID);//更新索引节点

        // delete index
        int DeleteTableIndex(ST_TRIE_INDEX_INFO& tIndexInfo);
		int TruncateTableIndex(ST_TRIE_INDEX_INFO& tIndexInfo);

		int OutPutInfo(bool bConsole,const char * fmt, ...);
		
        // print index info
        int PrintIndexInfo(ST_TRIE_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole);
		
		int PrintIndexInfoDetail(int iDetialLevel,bool bConsole, ST_TRIE_INDEX_INFO & stIndexInfo);

	private:

        // create index shm space
        int CreateNewTrieRootIndexShm(size_t iShmSize);//创建新的基础索引内存块
                

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
        TMdbShmDSN * m_pMdbShmDsn;//MDB共享管理区
        TMdbDSN   * m_pMdbDsn;        

        
    };
//}

#endif
