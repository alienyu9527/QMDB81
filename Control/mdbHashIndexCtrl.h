#ifndef _MDB_HASH_INDEX_CTRL_H_
#define _MDB_HASH_INDEX_CTRL_H_


#include "Helper/mdbDictionary.h"
#include "Helper/mdbConfig.h"

//namespace QuickMDB{

    class TMdbRowCtrl;
	struct ST_LINK_INDEX_INFO;
    
    struct ST_HASH_INDEX_INFO
    {
        
        TMdbBaseIndexMgrInfo     * pBIndexMgr;    //基础索引管理区
        TMdbConflictIndexMgrInfo * pCIndexMgr;	  //冲突索引管理区
        int 					   iBaseIndexPos; //基础索引位置
        int 					   iConflictIndexPos;//冲突索引位置
        TMdbBaseIndex            * pBaseIndex;    //基础索引信息
        TMdbConflictIndex        * pConflictIndex;//冲突索引信息
        TMdbIndexNode            * pBaseIndexNode;//基础索引链
        TMdbIndexNode            * pConflictIndexNode;//冲突索引链

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
        //创建基础索引
        int AttachDsn(TMdbShmDSN * pMdbShmDsn);//连接上共享内存管理区
        int AttachTable(TMdbShmDSN * pMdbShmDsn, TMdbTable* pTable);		
		void SetLink(TMdbLocalLink* pLink){m_pLink=pLink;}
		
        int AddTableSingleIndex(TMdbTable * pTable,int iIndexPos,size_t iDataSize);
        int DeleteTableIndex(ST_HASH_INDEX_INFO& tIndexInfo);//删除某个表的索引
        int TruncateTableIndex(ST_HASH_INDEX_INFO& tIndexInfo);
        int InsertIndexNode(int iIndexPos,TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndex, TMdbRowID& rowID);//插入索引节点
        int UpdateIndexNode(int iIndexPos,TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId);//更新索引节点
        int DeleteIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID);
        int SelectIndexNode(MDB_INT64 iIndexValue);//正在查询的索引节点
        int PrintIndexInfo(ST_HASH_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole);//打印详细索引信息iDetialLevel=[0~3] 0-最少信息，1-一般信息，2-详细信息
        int PrintIndexInfoDetail(int iDetialLevel,bool bConsole,ST_HASH_INDEX_INFO & stIndexInfo);//打印详细索引信息iDetialLevel=[0~3] 0-最少信息，1-一般信息，2-详细信息
        int PrintWarnIndexInfo(int iMaxCNodeCount);//打印存在冲突链大于指定长度的索引
        int FindRowIndexCValue(ST_HASH_INDEX_INFO tHashIndexInfo,TMdbRowIndex & tRowIndex,TMdbRowID& rowID);//查找冲突索引值

		int GetFreeConflictNode(ST_LINK_INDEX_INFO& tLinkIndexInfo, ST_HASH_INDEX_INFO& tTableHashIndex);

	private:
        int InitBCIndex(ST_HASH_INDEX_INFO & tTableIndex,TMdbTable * pTable);
        int InitIndexNode(TMdbIndexNode* pNode,MDB_INT64 iSize,bool bList);
        int GetFreeBIndexShm(MDB_INT64 iBaseIndexSize,size_t iDataSize,
            ST_HASH_INDEX_INFO & stTableIndexInfo);//获取空闲索引内存块
        int GetFreeCIndexShm(MDB_INT64 iConflictIndexSize,size_t iDataSize,
            ST_HASH_INDEX_INFO & stTableIndexInfo);//获取空闲索引内存块
        int CreateNewBIndexShm(size_t iShmSize);//创建新的基础索引内存块
        int CreateNewCIndexShm(size_t iShmSize);//创建新的冲突索引内存块
        int OutPutInfo(bool bConsole,const char * fmt, ...);//输出信息
        int GetTableByIndexName(const char * sIndexName,TMdbTable * & pTable);//根据索引名获取表
        int RecycleIndexSpace(TMDBIndexFreeSpace tFreeSpace[],size_t iPosAdd,size_t iSize);//回收索引空间
        int DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[]);//整理索引空间
        int PrintIndexSpace(const char * sPreInfo,TMDBIndexFreeSpace tFreeSpace[]);//打印索引空间信息

     
    private:
        TMdbTable * m_pAttachTable;
        TMdbShmDSN * m_pMdbShmDsn;//MDB共享管理区
        TMdbDSN   * m_pMdbDsn;
		TMdbLocalLink *m_pLink;
                
        MDB_INT64 m_lSelectIndexValue;//正在查询的索引冲突链
        
    };
//}

#endif
