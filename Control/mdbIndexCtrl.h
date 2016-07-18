#ifndef _MDB_INDEX_CTRL_H_
#define _MDB_INDEX_CTRL_H_
#include <vector>
#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"
#include <map>
#include "Helper/mdbDictionary.h"
#include "Helper/SqlParserStruct.h"
#include "Control/mdbHashIndexCtrl.h" 
#include "Control/mdbMHashIndexCtrl.h"
#include "Control/mdbTrieIndexCtrl.h"


class TMdbLocalLink;

    //index分配方案
    struct ST_SHM_ASSIGN_CELL
    {
        int iTable;//表pos
        int iIndex;//index  pos
        MDB_INT64 iSize;// size
    };
    struct ST_SHM_ASSIGN
    {
        std::vector<ST_SHM_ASSIGN_CELL> vCell;
        MDB_INT64 iMemLeft;     //剩余内存大小
    };

    //表索引信息
    struct ST_TABLE_INDEX_INFO
    {
        bool bInit;//是否被初始化
        TMdbIndex* pIndexInfo;//记录索引信息
        int  iIndexPos;//第几个index
        ST_HASH_INDEX_INFO m_HashIndexInfo;
        ST_MHASH_INDEX_INFO m_MHashIndexInfo;     
		ST_TRIE_INDEX_INFO m_TrieIndexInfo;
        


        void Clear()
        {
            bInit = false;
            pIndexInfo = NULL;
            iIndexPos = -1;
            m_HashIndexInfo.Clear();
            m_MHashIndexInfo.Clear();
            m_TrieIndexInfo.Clear();
        }
		
    };

    class TMdbIndexCtrl
    {
    public:
        TMdbIndexCtrl();
        ~TMdbIndexCtrl();

        // for kernel 
        int CreateAllIndex(TMdbConfig &config);
        int AddTableIndex(TMdbTable * pTable,size_t iDataSize);
        int AddTableSingleIndex(TMdbTable * pTable,size_t iDataSize);
        int DeleteTableIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable);
		int TruncateTableIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable);
        int DeleteTableSpecifiedIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,const char* pIdxName);


		int SetLinkInfo(TMdbLocalLink* pLink);
		int ReturnAllIndexNodeToTable(TMdbLocalLink* pLink, TMdbShmDSN * pMdbShmDsn);
		
        int AttachDsn(TMdbShmDSN * pMdbShmDsn);
        int AttachTable(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable);
		int AttachHashIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,int& iFindIndexs);
		int AttachMHashIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,int& iFindIndexs);
		int AttachTrieIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,int& iFindIndexs);
        
		int ReAttachSingleIndex(int iIndexPos);
		int ReAttachSingleHashIndex(int iIndexPos);
		int ReAttachSingleMHashIndex(int iIndexPos);
        ST_TABLE_INDEX_INFO * GetIndexByColumnPos(int iColumnPos,int &iColNoPos);
	 	ST_TABLE_INDEX_INFO * GetAllIndexByColumnPos(int iColumnPos,int &iColNoPos,int &iCurIndexPos);//根据columnpos获取indexnode
		ST_TABLE_INDEX_INFO * GetIndexByName(const char* sName);

        //根据可能的索引列获取索引
        int GetIndexByIndexColumn(std::vector<ST_INDEX_COLUMN> & vIndexColumn,std::vector<ST_INDEX_VALUE> & vIndexValue, ST_TABLE_INDEX_INFO* pHintIndex);
		int FindBestIndex(ST_INDEX_COLUMN ** ppIndexColumnArr,std::map<int,int>& mapCfgToWhereClause,int iHintIndexPosInCfg);

		ST_TABLE_INDEX_INFO * GetScanAllIndex();//获取全量遍历的索引

        ST_TABLE_INDEX_INFO * GetVerfiyPKIndex();//获取校验主键的索引

        int InsertIndexNode(int iIndexPos,char* pDataAddr, TMdbRowCtrl& tRowCtrl,TMdbRowID& rowID);//插入索引节点
        int DeleteIndexNode(int iIndexPos,char* pAddr,TMdbRowCtrl& tRowCtrl, TMdbRowID& rowID);
        int UpdateIndexNode(int iIndexPos,char* pOldData,ST_INDEX_VALUE& tMemIndexValue,TMdbRowCtrl& tRowCtrl,TMdbRowID& tRowId);//更新索引节点

        int PrintIndexInfo(int iDetialLevel,bool bConsole);       
        int PrintWarnIndexInfo(int iMaxCNodeCount);

		int GetTrieWord( ST_INDEX_VALUE & stIndexValue, char* sTrieWord);
		int GetTrieWord( TMdbRowCtrl& tRowCtrl,char* pAddr, TMdbIndex* pIndex, char* sTrieWord);
		
        long long CalcIndexValue( TMdbRowCtrl& tRowCtrl,char* pAddr, TMdbIndex* pIndex, int& iError);
        int RenameTableIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,const char *sNewTableName);//rename 表后 调整索引中的表名
		bool CheckHashConflictIndexFull();
		
    private:
        void CleanTableIndexInfo();
        int GenerateIndexValue(ST_INDEX_COLUMN *pIndexColumnArr [] ,ST_TABLE_INDEX_INFO  * pstTableIndexInfo,int iCurPos,std::vector<ST_INDEX_VALUE> & vIndexValue);

        // hash值计算
        long long CalcMPIndexValue(TMdbRowCtrl& tRowCtrl, char* pAddr, TMdbIndex* pIndex, int& iError);
        long long CalcOneIndexValue(TMdbRowCtrl& tRowCtrl, char* pAddr, TMdbIndex* pIndex, int& iError);
        int CalcMemValueHash(ST_INDEX_VALUE & stIndexValue,long long & llValue);   

    private:
        TMdbShmDSN * m_pMdbShmDsn;
        TMdbDSN   * m_pMdbDsn;		
		TMdbLocalLink *m_pLink;

        TMdbHashIndexCtrl m_tHashIndex;
        TMdbMHashIndexCtrl m_tMHashIndex;
		TMdbTrieIndexCtrl m_tTrieIndex;

        TMdbTable * m_pAttachTable;

		//此处记录的是当前表 表级别的索引信息，真实使用时,由链接到此申请索引节点
        ST_TABLE_INDEX_INFO   m_arrTableIndex[MAX_INDEX_COUNTS];

        char     m_sTempValue[MAX_BLOB_LEN];//存放临时字符串，为了性能
        
    };
//}
#endif
