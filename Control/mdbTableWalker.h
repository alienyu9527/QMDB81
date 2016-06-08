/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbTableWalker.cpp		
*@Description： MDB表遍历类。按照索引进行遍历。
*@Author:		jin.shaohua
*@Date：	    2012.04
*@History:
******************************************************************************************/

#ifndef _MDB_TABLE_WALKER_H_
#define _MDB_TABLE_WALKER_H_
#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Control/mdbIndexAlgo.h"


#include <stack> 


#define TYPE_SCAN_PAGE_FULL   1
#define TYPE_SCAN_PAGE_FREE   0
#define TYPE_SCAN_PAGE_END    -1

//namespace QuickMDB{

    // 辅助遍历阶梯式索引，记录索引位置
    class TMHashRowIndex
    {
    public:
        void Clear()
        {
            m_iHashValue = 0;
            iBaseIndexPos = -1;
            iConflictIndexPos = -1;
            iLayerIndexPos = -1;
            pConfIndex = NULL;
            pLayerIndex = NULL;
            m_bCheckLastLayerConf = true;
        }

        bool IsOnConflict()
        {
            return iConflictIndexPos>=0?true:false;
        }
        
        bool IsOnLayer()
        {
            return iLayerIndexPos>=0?true:false;
        }
        
    public:
        long long m_iHashValue;
        int iBaseIndexPos;
        int iConflictIndexPos;
        int iLayerIndexPos;
        TMdbMHashConflictIndex* pConfIndex;
        TMdbMhashLayerIndex* pLayerIndex;
        bool m_bCheckLastLayerConf; // 是否需要检测最底层阶梯节点上的冲突链节点
    };

    class TMHashScanLayerPos
    {
    public:
        void Clear()
        {
            iLayerPos = -1;
            iOffset = 0;
        }
        
    public:
        int iLayerPos;
        int iOffset;
    };


	class TTrieBranchPos
    {
    public:
		TTrieBranchPos(int bpos,int cidx)
		{
			iBranchPos = bpos;
			iChildIdx = cidx;
		}
        void Clear()
        {
            iBranchPos = -1;
            iChildIdx = -1;
        }
        
    public:
        int iBranchPos;
        int iChildIdx; //记录走到哪一个孩子节点了
    };
	

    /*
    ** mdbTable遍历类,根据索引来遍历表或者全表遍历
    */
    class TMdbTableWalker
    {
    public:
    	TMdbTableWalker();
    	~TMdbTableWalker();
    	int AttachTable(TMdbShmDSN * pShmDSN,TMdbTable * m_pMdbTable);//连接表
        int WalkByIndex(ST_TABLE_INDEX_INFO* pIndexInfo, long long lIndexValue);
    	bool Next();//下一个
    	inline char * GetDataAddr(){	return m_pDataAddr;}//获取数据地址
    	inline MDB_INT32 GetDataSize(){return m_iDataSize;}//获取数据大小
    	inline char * GetPageAddr(){return (char *)m_pCurPage;}//获取页地址
    	inline MDB_INT32 GetPagePos(){return m_iPagePos;}//
    	inline TMdbRowID GetDataRowID(){return m_tCurRowIDData;}//获取数据的rowid;
    	int StopWalk();//停止遍历
    	int WalkByPage(int iStartPageID);//根据页来遍历数据
    	bool NextByPage();//
    	char* GetAddressRowID(TMdbRowID* pRowID, int &iDataSize, bool bGetPageAddr = false);
		
    protected:
        // hash 索引遍历
        int WalkByHashIndex(ST_TABLE_INDEX_INFO* pIndexInfo, long long lIndexValue);
        bool NextByHash();

        // m-hash索引遍历
        int WalkByMHashIndex(ST_TABLE_INDEX_INFO* pIndexInfo, long long lIndexValue); 
        bool NextByMHash();
        char* GetAddrByMhashIndexNodeId(int iHeadBlock,int iIndexNodeId, int iNodeSize, bool bConf);
        TMdbMhashBlock* GetMhashBlockById(int iBlockID, bool bConf);

		//trie索引遍历
		int WalkByTrieIndex(ST_TABLE_INDEX_INFO* pIndexInfo);		
		bool NextByTrie();
		char* GetAddrByTrieIndexNodeId(int iHeadBlock,int iIndexNodeId, int iNodeSize, bool bConf);
		TMdbTrieBlock* GetTrieBlockById(int iBlockID, bool bConf);
		
    private:
        TMdbShmDSN * 	m_pShmDSN;//
        TMdbDSN * 		m_pMdbDSN;//DSN
        TMdbTableSpace * m_pTableSpace;//表空间
        TMdbTable * 	       m_pMdbTable;//表
        MDB_INT32  			m_iDataSize;   //数据长度
        char * 			m_pDataAddr;//数据地址，也可表示索引节点的地址
        char *			m_pNextDataAddr;//临时变量，用于保存m_pDataAddr的下一个位置
        int     m_iPagePos;     //数据在页面中的位置
        TMdbRowID 		m_tNextRowIDData,m_tCurRowIDData;
        TMdbTableSpaceCtrl m_tTSCtrl;//表空间管理
        int m_iCurWalkIndexAlgo; //当前索引遍历算法类型
        
        // hash索引遍历使用
        TMdbIndexNode* m_pConflictIndex; 
        MDB_INT32 			m_iNextIndexPos; //下一个index的位置

        // 阶梯式索引遍历使用
        TMdbMHashBaseIndexNode* m_pMHashBaseIdx; // 基础索引链节点
        TMHashRowIndex m_tMHashRowIdx; // 记录索引遍历

		//字典树索引遍历使用 
		TMdbTrieIndexNode* m_pTrieRootNode;
		TMdbTrieConflictIndex* m_pTrieConfIndex;
        TMdbTrieBranchIndex* m_pTrieBranchIndex;
		bool m_bStopScanTrie;
				
        bool m_bScanAll;
		
	public:	
        TMdbPage * m_pCurPage;    //数据所在页面的地址
        TMdbPage * m_pStartPage; //是否遍历过表FREE链的头结点
		int m_iScanPageType; //0 free链  1 full链
		int m_iFirstFullPageID;
		int m_iFirstFreePageID;
		TMdbPage * m_pNextPage;
		int m_iAffect;
		char m_sTrieWord[MAX_TRIE_WORD_LEN];
    };
//}
#endif

