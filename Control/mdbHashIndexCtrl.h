#ifndef _MDB_HASH_INDEX_CTRL_H_
#define _MDB_HASH_INDEX_CTRL_H_


#include "Helper/mdbDictionary.h"
#include "Helper/mdbConfig.h"


//namespace QuickMDB{
	struct ST_LINK_INDEX_INFO;
	class TMdbHashMutexMgrInfo;
	class TMdbHashBaseMutex;    struct ST_HASH_INDEX_INFO
    {
        
        TMdbBaseIndexMgrInfo     * pBIndexMgr;    //基础索引管理区
        TMdbConflictIndexMgrInfo * pCIndexMgr;	  //冲突索引管理区        
        TMdbHashMutexMgrInfo	 * pMutexMgr;
		
        int 					   iBaseIndexPos; //基础索引位置
        int 					   iConflictIndexPos;//冲突索引位置
        int 				       iMutexPos;
        
        TMdbBaseIndex            * pBaseIndex;    //基础索引信息
        TMdbConflictIndex        * pConflictIndex;//冲突索引信息
        
        TMdbIndexNode            * pBaseIndexNode;//基础索引链
        TMdbIndexNode            * pConflictIndexNode;//冲突索引链
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
	
	// 基本链节点对应的锁
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
        int iSeq;         //第几个共享内存段
        TMutex tMutex;    //管理区共享锁
        TMdbHashBaseMutex aBaseMutex[MAX_BASE_INDEX_COUNTS]; //基础索引锁信息
        int iCounts;  //已有索引数
        TMDBIndexFreeSpace tFreeSpace[MAX_BASE_INDEX_COUNTS];//空闲空间
        MDB_INT64 iTotalSize;   //总大小
        
    };
	
    class TMdbRowCtrl;	
    class TMdbHashIndexCtrl
    {
    public:
        TMdbHashIndexCtrl();
        ~TMdbHashIndexCtrl();
        //创建基础索引
        int AttachDsn(TMdbShmDSN * pMdbShmDsn);//连接上共享内存管理区
        int AttachTable(TMdbShmDSN * pMdbShmDsn, TMdbTable* pTable);		
		void SetLink(TMdbLocalLink* pLink){m_pLink=pLink;}
		
        int AddTableSingleIndex(TMdbTable * pTable,int iIndexPos,size_t iDataSize);
        int DeleteTableIndex(ST_HASH_INDEX_INFO& tIndexInfo);//删除某个表的索引
        int TruncateTableIndex(ST_HASH_INDEX_INFO& tIndexInfo);

		int InsertIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndex, TMdbRowID& rowID);//插入索引节点
	    int UpdateIndexNode(TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId);//更新索引节点
	    int DeleteIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID);

		int InsertIndexNode2(int iIndexPos,TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndex, TMdbRowID& rowID);//插入索引节点
        int UpdateIndexNode2(int iIndexPos,TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId);//更新索引节点
        int DeleteIndexNode2(int iIndexPos,TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID);

		int PrintIndexInfo(ST_HASH_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole);//打印详细索引信息iDetialLevel=[0~3] 0-最少信息，1-一般信息，2-详细信息
        int PrintIndexInfoDetail(int iDetialLevel,bool bConsole,ST_HASH_INDEX_INFO & stIndexInfo);//打印详细索引信息iDetialLevel=[0~3] 0-最少信息，1-一般信息，2-详细信息
		int PrintRePosIndexInfoDetail(int iDetialLevel,bool bConsole, ST_HASH_INDEX_INFO & stIndexInfo);        int PrintWarnIndexInfo(int iMaxCNodeCount);//打印存在冲突链大于指定长度的索引
        int FindRowIndexCValue(ST_HASH_INDEX_INFO tHashIndexInfo,TMdbRowIndex & tRowIndex,TMdbRowID& rowID);//查找冲突索引值
	
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
		
        int GetFreeBIndexShm(MDB_INT64 iBaseIndexSize,size_t iDataSize,ST_HASH_INDEX_INFO & stTableIndexInfo);//获取空闲索引内存块
        int GetFreeCIndexShm(MDB_INT64 iConflictIndexSize,size_t iDataSize,ST_HASH_INDEX_INFO & stTableIndexInfo,bool bRePosIndex);//获取空闲索引内存块
		int GetHashFreeMutexShm(MDB_INT64 iMutexSize,size_t iDataSize,ST_HASH_INDEX_INFO & stTableIndexInfo);

		int CreateNewBIndexShm(size_t iShmSize);//创建新的基础索引内存块
        int CreateNewCIndexShm(size_t iShmSize);//创建新的冲突索引内存块       
		int CreateHashNewMutexShm(size_t iShmSize);
		
		int CalcBaseMutexCount(int iBaseCont);        int OutPutInfo(bool bConsole,const char * fmt, ...);//输出信息
        int GetTableByIndexName(const char * sIndexName,TMdbTable * & pTable);//根据索引名获取表
        int RecycleIndexSpace(TMDBIndexFreeSpace tFreeSpace[],size_t iPosAdd,size_t iSize);//回收索引空间
        int DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[]);//整理索引空间
        int PrintIndexSpace(const char * sPreInfo,TMDBIndexFreeSpace tFreeSpace[]);//打印索引空间信息


     
    private:
        TMdbTable * m_pAttachTable;
        TMdbShmDSN * m_pMdbShmDsn;//MDB共享管理区
        TMdbDSN   * m_pMdbDsn;
		TMdbRowCtrl*  m_pRowCtrl;//记录控制
        TMdbLocalLink *m_pLink;        MDB_INT64 m_lSelectIndexValue;//正在查询的索引冲突链
        
    };
//}

#endif
