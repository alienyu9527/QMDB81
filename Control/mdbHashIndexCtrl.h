#ifndef _MDB_HASH_INDEX_CTRL_H_
#define _MDB_HASH_INDEX_CTRL_H_


#include "Helper/mdbDictionary.h"
#include "Helper/mdbConfig.h"

//namespace QuickMDB{

    class TMdbRowCtrl;
    
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
        //int CreateAllIndex(TMdbConfig &config);
        int AddTableSingleIndex(TMdbTable * pTable,int iIndexPos,size_t iDataSize);
        //int AddTableIndex(TMdbTable * pTable,size_t iDataSize);//添加索引
        int DeleteTableIndex(ST_HASH_INDEX_INFO& tIndexInfo);//删除某个表的索引
        int TruncateTableIndex(ST_HASH_INDEX_INFO& tIndexInfo);
        //int DeleteTableSpecifiedIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,const char* pIdxName);
        //获取基础+ 冲突索引节点
        //int AttachTable(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable);
        //int GetBCIndex(TMdbIndexNode ** pBaseNode,TMdbIndexNode ** pConflictNode);//获取基础+ 冲突索引
        int InsertIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndex, TMdbRowID& rowID);//插入索引节点
        int UpdateIndexNode(TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId);//更新索引节点
        int DeleteIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID);
        int SelectIndexNode(MDB_INT64 iIndexValue);//正在查询的索引节点
        int PrintIndexInfo(ST_HASH_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole);//打印详细索引信息iDetialLevel=[0~3] 0-最少信息，1-一般信息，2-详细信息
        int PrintIndexInfoDetail(int iDetialLevel,bool bConsole,ST_HASH_INDEX_INFO & stIndexInfo);//打印详细索引信息iDetialLevel=[0~3] 0-最少信息，1-一般信息，2-详细信息
        //ST_TABLE_INDEX_INFO * GetIndexByColumnPos(int iColumnPos,int &iColNoPos);//根据columnpos获取indexnode
        //ST_TABLE_INDEX_INFO * GetScanAllIndex();//获取全量遍历的索引
        //ST_TABLE_INDEX_INFO * GetVerfiyPKIndex();//获取校验主键的索引
        //bool CombineCMPIndex(ST_INDEX_VALUE & stLeftIndexValue,ST_INDEX_VALUE & stRightIndexValue, ST_INDEX_VALUE & stOutIndexValue);//拼接组合索引
        int PrintWarnIndexInfo(int iMaxCNodeCount);//打印存在冲突链大于指定长度的索引
        //int RebuildTableIndex(bool bNeedToClean);//重新构建某表索引区
        int FindRowIndexCValue(ST_HASH_INDEX_INFO tHashIndexInfo,TMdbRowIndex & tRowIndex,TMdbRowID& rowID);//查找冲突索引值
        //int GetIndexByIndexColumn(std::vector<ST_INDEX_COLUMN> & vIndexColumn,std::vector<ST_INDEX_VALUE> & vIndexValue);//根据可能的索引列获取索引
        //int GenerateIndexValue(ST_INDEX_COLUMN * pIndexColumnArr[],ST_TABLE_INDEX_INFO  * pstTableIndexInfo,int iCurPos,std::vector<ST_INDEX_VALUE> & vIndexValue);
        //int RemoveDupIndexColumn(std::vector<ST_INDEX_COLUMN> & vLeftIndexColumn,std::vector<ST_INDEX_COLUMN> & vRightIndexColumn);//清除重复的可能索引列
        //int RemoveDupIndexValue(std::vector<ST_INDEX_VALUE> & vLeftIndexValue,std::vector<ST_INDEX_VALUE> & vRightIndexValue);//清除重复的索引
    private:
        //void CleanTableIndexInfo();//清理表的索引信息
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
        
        //ST_HASH_INDEX_INFO   m_arrTableIndex[MAX_INDEX_COUNTS];//保存某张表的索引信息
        
        MDB_INT64 m_lSelectIndexValue;//正在查询的索引冲突链
        
    };
//}

#endif
