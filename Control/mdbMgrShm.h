/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbShmDSN.h
*@Description： 管理区的相关数据的控制，需要考虑多个DSN的情形
*@Author:		li.shugang
*@Date：	    2008年11月30日
*@History:
******************************************************************************************/
#ifndef __ZX_MINI_DATABASE_MANAGER_SHARE_MEMORY_H__
#define __ZX_MINI_DATABASE_MANAGER_SHARE_MEMORY_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbShm.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbIndexCtrl.h"
#include "Control/mdbVarcharMgr.h"
#include "Helper/mdbShmSTL.h"
#include "Helper/mdbDictionary.h"
#include "Control/mdbRepCommon.h"
#include "Control/mdbTrieIndexCtrl.h"



//namespace QuickMDB{

    class TMdbIndexCtrl;
    class TMdbMHashConflictIndexMgrInfo;
    class TMdbMHashLayerIndexMgrInfo;
    class TObservePoint;
    class TMdbShmDSN
    {
    public:
        TMdbShmDSN();
        ~TMdbShmDSN();
    public:
        int CreateMgrShm(TMdbConfig &config);//创建管理区的共享内存块
        int CreateNewDataShm(TMdbConfig &config); // 从系统中创建一个新的共享内存块
        int Attach(const char * pszDSN, TMdbConfig &config);//关联共享内存
        int TryAttach();//仅仅是尝试attach
        int TryAttachEx(TMdbConfig &config);//尝试连接共享内存，看看是否存在，不输出任何信息
        int Attach();//关联共享内存
        int ReAttachIndex();//重新关联索引
        int Detach();   //取消和共享内存的关联
        int Destroy(); //删除共享内存
        TMdbDSN* GetInfo();//获取信息
        int LockDSN();//锁住管理区
        int UnLockDSN();//解锁
    public:
        int AddNewTableSpace(TMdbTableSpace * & pNewTableSpace);//获取一个新的表空间
        int AddNewTable(TMdbTable* & pNewTable,TMdbTable * pSrcTable);//新增一个表
        int AddNewProc(TMdbProc * & pNewProc);//新增一个进程信息
        int AddNewLocalLink(TMdbLocalLink *& pNewLink);//新增一个链接
        int AddNewRemoteLink(TMdbRemoteLink *& pNewLink);//新增一个链接
        int AddNewRepLink(TMdbRepLink *& pNewLink);//新增一个链接
        int AddNewMemSeq(TMemSeq *& pNewMemSeq);//新增一个memseq
        int AddNewJob(TMdbJob *& pNewJob);// 新增一个新job
        int AddNewVarChar(TMdbVarchar *&pNewVarChar); //新增一个varchar段
        int AddNewMhashConflict(TMdbMhashBlock *&pNewBlock);
        int AddNewMhashLayer(TMdbMhashBlock *&pNewBlock);
		
		int AddNewTrieConflict(TMdbTrieBlock *&pNewBlock);
		int AddNewTrieBranch(TMdbTrieBlock*& pNewBlock);

		TMdbJob * GetJobByName(const char * sName);//根据名字获取job
        char * GetAddrByOffset(size_t iOffset){return m_tMgrShmAlloc.GetStartAddr() + iOffset;}
        
        int CreateSyncAreaShm();//创建同步区
        int DetachSyncArea();//断链同步区
        int DestroySyncArea();//销毁同步区
        TMdbSyncArea * GetSyncArea(){return &(m_pTMdbDSN->m_arrSyncArea);}

        int CreateShardBackupRepInfoShm(const char * pszDSN);
        int DestroyShardBackupRepInfoShm();
        
    public:
        char * GetSyncAreaShm();//获取同步区共享内存
        // get hash index
        char* GetBaseIndex(int iPos);
        char* GetConflictIndex(int iPos);
        // get m-hash index
        char* GetMHashBaseIndex(int iPos);
        char* GetMHashMutex(int iPos);		
        TMdbMHashConflictIndexMgrInfo* GetMHashConfMgr();
        TMdbMHashLayerIndexMgrInfo* GetMHashLayerMgr();

		// trie index
		char* GetTrieRootIndex(int iPos);
		TMdbTrieConflictIndexMgrInfo* GetTrieConfMgr();
		TMdbTrieBranchIndexMgrInfo* GetTrieBranchMgr();
		
        int CreateMhashMgrShm(); // 创建阶梯式hash索引的冲突索引管理区以及阶梯索引管理区
        int DetachMhashMgr();
        int DestroyMhashMgr();

		
		int CreateTrieMgrShm();		
		int DetachTrieMgr();
		int DestroyTrieMgr();
		


    public:        
        char* GetDataShm(int iPos);
        char* GetDataShmAddr(SHAMEM_T iShmID);
        char* GetVarcharShmAddr(SHAMEM_T iShmID);
        int   AttachvarCharBlockShm(int iPos, char **pAddr);
    public:
        TMdbTable * GetTableByName(const char * pTableName);//根据表名获取表
        //TMdbTable * GetTableById(int iTableId);//根据表id获取表
        //TMdbTableSpace * GetTableSpaceAddrById(int iId);//根据tablespace-id获取
    	TMdbTableSpace * GetTableSpaceAddrByName(const char * pTablespaceName);//根据表空间名获取
    	TMdbVarchar    * GetVarchar(int iWhichPos); //获取varchar段
        TObservePoint * GetObPiontById(int iObId);//获取observe point
        TObservePoint * GetObPiontByName(const char * sName);
        TMdbProc * GetProcByPid(int iPid);//根据进程ID，获取进程信息
        char * GetVarcharMgrAddr();//获取varchar 管理区
        char * GetPageMutexAddr();//获取页锁地址
        char * GetVarcharPageMutexAddr();//获取varchar页锁地址
        TMemSeq * GetMemSeqByName(const char * sSeqName);//根据seqname获取
        TMdbProc * GetProcByName(const char * sName);//根据名字获取进程
        size_t GetUsedSize(){return m_tMgrShmAlloc.m_pShmHead->m_iFreeOffSet;};//获取空闲大小
        void * Alloc(size_t iAllocSize,size_t & iOffset){return m_tMgrShmAlloc.Allocate(iAllocSize,iOffset);}//分配
        int InitVarchar(int iPos,int iShmId,char*& pVarcharAddr);
		
        int InitMHashConfBlock(int iPos, char*& pAddr);
        int InitMHashLayerBlock(int iPos, char*& pAddr);
        TMdbMhashBlock* GetMhashConfBlockById(int iBlockID);
        TMdbMhashBlock* GetMhashLayerBlockById(int iBlockID);		
		char* GetMhashConfShmAddr(SHAMEM_T iShmID);
        char* GetMhashLayerShmAddr(SHAMEM_T iShmID);	
		
		int InitTrieBranchBlock(int iPos, char*& pAddr);	
		int InitTrieConfBlock(int iPos, char*& pAddr);	
		TMdbTrieBlock* TMdbShmDSN::GetTrieConfBlockById(int iBlockID);
		TMdbTrieBlock* TMdbShmDSN::GetTrieBranchBlockById(int iBlockID);
		char* GetTrieBranchShmAddr(SHAMEM_T iShmID);			
		char* GetTrieConfShmAddr(SHAMEM_T iShmID);

    private:
        int InitDSN(TMdbConfig &config); //初始化DSN
        int CreateDataShm(TMdbConfig &config);//创建数据区
        int SetDsnRepAttr(TMdbConfig &config);//设置DSN同步属性
        int   InitDsnAddr(TMdbConfig &config);//初始化地址偏移

    private:
        TMdbShmRepMgr* m_pShmMgr;
        char * m_arrSyncAreaShm;   //同步区共享内存
        
        char* m_pBaseIndexShmAddr[MAX_SHM_ID];//基础索引地址
        char* m_pConflictIndexShmAddr[MAX_SHM_ID];//冲突索引地址
        
        char* m_pMHashBaseIndexShmAddr[MAX_SHM_ID]; // 阶梯式索引基础索引地址
        char* m_pMHashMutexShmAddr[MAX_SHM_ID]; // 阶梯式索引锁地址
        char* m_pMHashConfIndexShmAddr[MAX_MHASH_SHMID_COUNT]; // 阶梯式索引冲突索引地址
        char* m_pMHashLayerIndexShmAddr[MAX_MHASH_SHMID_COUNT]; // 阶梯式索引阶梯索引地址

		char* m_pTrieRootIndexShmAddr[MAX_BRIE_SHMID_COUNT]; //树形索引基础索引地址
        char* m_pTrieBranchIndexShmAddr[MAX_BRIE_SHMID_COUNT]; // 树形索引冲突索引地址
        char* m_pTrieConfIndexShmAddr[MAX_BRIE_SHMID_COUNT]; // 树形索引冲突索引地址

        int*  m_pOtherShmID[MAX_SHM_ID];
        char* m_pOtherShmAddr[MAX_SHM_ID];
	 	int * m_pVarcharShmID[MAX_VARCHAR_SHM_ID];
	 	char* m_pVarcharShmAddr[MAX_VARCHAR_SHM_ID];
        TMdbDSN *m_pTMdbDSN;   //管理区信息
        bool m_bIsAttach;  //是否已经关联上共享内存
        long long  m_iMgrKey;
        long long  m_iVarCharKey;
        TMdbIndexCtrl * m_pMdbIndexCtrl;
        bool m_bTryAttach;
        TShmAlloc m_tMgrShmAlloc;//共享内存分配器
        char* m_pMHashConfMgr; // 阶梯式hash索引冲突段管理信息
        char* m_pMHashLayerMgr; // 阶梯式hash索引阶梯索引段管理信息

		char* m_pTrieConfMgr; 
        char* m_pTrieBranchMgr;
        
    public:
        TShmList<TMdbTable>   m_TableList;//表链
        TShmList<TMdbTableSpace> m_TSList;//表空间
        TShmList<TObservePoint>    m_ObserverList;//观测点
        TShmList<TMdbProc>           m_ProcList;//进程链表
        TShmList<TMdbLocalLink>    m_LocalLinkList; //本地链接
        TShmList<TMdbRemoteLink> m_RemoteLinkList;//远端链接
        TShmList<TMemSeq>          m_MemSeqList;// 
        TShmList<TMdbRepLink>     m_RepLinkList;
        TShmList<TMdbJob>            m_JobList;//job列表
        TShmList<TMdbVarchar>        m_VarCharList; //Varchar段
        
        TShmList<TMdbMhashBlock> m_MhashConfList; // 阶梯式索引冲突索引块链表
        TShmList<TMdbMhashBlock> m_MhashLayerList; // 阶梯式索引阶梯索引块链表

		TShmList<TMdbTrieBlock> m_TrieBranchList; // 树形索引树节点索引块链表
        TShmList<TMdbTrieBlock> m_TrieConfList; // 树形索引冲突索引块链表
        
    };


    //负责管理每个DSN信息
    class TMdbShmMgr
    {
    public:
        /******************************************************************************
        * 函数名称	:  GetShmDSN()
        * 函数描述	:  创建管理区的共享内存块
        * 输入		:  config, 配置信息bSkipErrorLog -  跳过错误日志
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        static TMdbShmDSN* GetShmDSN(const char* pszDSN,bool bSkipErrorLog = false);
        /******************************************************************************
        * 函数名称	:  CreateMgrShm()
        * 函数描述	:  创建管理区的共享内存块
        * 输入		:  config, 配置信息
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        static int CreateMgrShm(TMdbConfig &config);
    private:
        static TMdbShmDSN m_tShmDSN[MAX_DSN_COUNTS];
    };

//}
#endif //__ZX_MINI_DATABASE_MANAGER_SHARE_MEMORY_H__


