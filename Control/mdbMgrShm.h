/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbShmDSN.h
*@Description�� ��������������ݵĿ��ƣ���Ҫ���Ƕ��DSN������
*@Author:		li.shugang
*@Date��	    2008��11��30��
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
        int CreateMgrShm(TMdbConfig &config);//�����������Ĺ����ڴ��
        int CreateNewDataShm(TMdbConfig &config); // ��ϵͳ�д���һ���µĹ����ڴ��
        int Attach(const char * pszDSN, TMdbConfig &config);//���������ڴ�
        int TryAttach();//�����ǳ���attach
        int TryAttachEx(TMdbConfig &config);//�������ӹ����ڴ棬�����Ƿ���ڣ�������κ���Ϣ
        int Attach();//���������ڴ�
        int ReAttachIndex();//���¹�������
        int Detach();   //ȡ���͹����ڴ�Ĺ���
        int Destroy(); //ɾ�������ڴ�
        TMdbDSN* GetInfo();//��ȡ��Ϣ
        int LockDSN();//��ס������
        int UnLockDSN();//����
    public:
        int AddNewTableSpace(TMdbTableSpace * & pNewTableSpace);//��ȡһ���µı�ռ�
        int AddNewTable(TMdbTable* & pNewTable,TMdbTable * pSrcTable);//����һ����
        int AddNewProc(TMdbProc * & pNewProc);//����һ��������Ϣ
        int AddNewLocalLink(TMdbLocalLink *& pNewLink);//����һ������
        int AddNewRemoteLink(TMdbRemoteLink *& pNewLink);//����һ������
        int AddNewRepLink(TMdbRepLink *& pNewLink);//����һ������
        int AddNewMemSeq(TMemSeq *& pNewMemSeq);//����һ��memseq
        int AddNewJob(TMdbJob *& pNewJob);// ����һ����job
        int AddNewVarChar(TMdbVarchar *&pNewVarChar); //����һ��varchar��
        int AddNewMhashConflict(TMdbMhashBlock *&pNewBlock);
        int AddNewMhashLayer(TMdbMhashBlock *&pNewBlock);
		
		int AddNewTrieConflict(TMdbTrieBlock *&pNewBlock);
		int AddNewTrieBranch(TMdbTrieBlock*& pNewBlock);

		TMdbJob * GetJobByName(const char * sName);//�������ֻ�ȡjob
        char * GetAddrByOffset(size_t iOffset){return m_tMgrShmAlloc.GetStartAddr() + iOffset;}
        
        int CreateSyncAreaShm();//����ͬ����
        int DetachSyncArea();//����ͬ����
        int DestroySyncArea();//����ͬ����
        TMdbSyncArea * GetSyncArea(){return &(m_pTMdbDSN->m_arrSyncArea);}

        int CreateShardBackupRepInfoShm(const char * pszDSN);
        int DestroyShardBackupRepInfoShm();
        
    public:
        char * GetSyncAreaShm();//��ȡͬ���������ڴ�
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
		
        int CreateMhashMgrShm(); // ��������ʽhash�����ĳ�ͻ�����������Լ���������������
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
        TMdbTable * GetTableByName(const char * pTableName);//���ݱ�����ȡ��
        //TMdbTable * GetTableById(int iTableId);//���ݱ�id��ȡ��
        //TMdbTableSpace * GetTableSpaceAddrById(int iId);//����tablespace-id��ȡ
    	TMdbTableSpace * GetTableSpaceAddrByName(const char * pTablespaceName);//���ݱ�ռ�����ȡ
    	TMdbVarchar    * GetVarchar(int iWhichPos); //��ȡvarchar��
        TObservePoint * GetObPiontById(int iObId);//��ȡobserve point
        TObservePoint * GetObPiontByName(const char * sName);
        TMdbProc * GetProcByPid(int iPid);//���ݽ���ID����ȡ������Ϣ
        char * GetVarcharMgrAddr();//��ȡvarchar ������
        char * GetPageMutexAddr();//��ȡҳ����ַ
        char * GetVarcharPageMutexAddr();//��ȡvarcharҳ����ַ
        TMemSeq * GetMemSeqByName(const char * sSeqName);//����seqname��ȡ
        TMdbProc * GetProcByName(const char * sName);//�������ֻ�ȡ����
        size_t GetUsedSize(){return m_tMgrShmAlloc.m_pShmHead->m_iFreeOffSet;};//��ȡ���д�С
        void * Alloc(size_t iAllocSize,size_t & iOffset){return m_tMgrShmAlloc.Allocate(iAllocSize,iOffset);}//����
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
        int InitDSN(TMdbConfig &config); //��ʼ��DSN
        int CreateDataShm(TMdbConfig &config);//����������
        int SetDsnRepAttr(TMdbConfig &config);//����DSNͬ������
        int   InitDsnAddr(TMdbConfig &config);//��ʼ����ַƫ��

    private:
        TMdbShmRepMgr* m_pShmMgr;
        char * m_arrSyncAreaShm;   //ͬ���������ڴ�
        
        char* m_pBaseIndexShmAddr[MAX_SHM_ID];//����������ַ
        char* m_pConflictIndexShmAddr[MAX_SHM_ID];//��ͻ������ַ
        
        char* m_pMHashBaseIndexShmAddr[MAX_SHM_ID]; // ����ʽ��������������ַ
        char* m_pMHashMutexShmAddr[MAX_SHM_ID]; // ����ʽ��������ַ
        char* m_pMHashConfIndexShmAddr[MAX_MHASH_SHMID_COUNT]; // ����ʽ������ͻ������ַ
        char* m_pMHashLayerIndexShmAddr[MAX_MHASH_SHMID_COUNT]; // ����ʽ��������������ַ

		char* m_pTrieRootIndexShmAddr[MAX_BRIE_SHMID_COUNT]; //������������������ַ
        char* m_pTrieBranchIndexShmAddr[MAX_BRIE_SHMID_COUNT]; // ����������ͻ������ַ
        char* m_pTrieConfIndexShmAddr[MAX_BRIE_SHMID_COUNT]; // ����������ͻ������ַ

        int*  m_pOtherShmID[MAX_SHM_ID];
        char* m_pOtherShmAddr[MAX_SHM_ID];
	 	int * m_pVarcharShmID[MAX_VARCHAR_SHM_ID];
	 	char* m_pVarcharShmAddr[MAX_VARCHAR_SHM_ID];
        TMdbDSN *m_pTMdbDSN;   //��������Ϣ
        bool m_bIsAttach;  //�Ƿ��Ѿ������Ϲ����ڴ�
        long long  m_iMgrKey;
        long long  m_iVarCharKey;
        TMdbIndexCtrl * m_pMdbIndexCtrl;
        bool m_bTryAttach;
        TShmAlloc m_tMgrShmAlloc;//�����ڴ������
        char* m_pMHashConfMgr; // ����ʽhash������ͻ�ι�����Ϣ
        char* m_pMHashLayerMgr; // ����ʽhash�������������ι�����Ϣ

		char* m_pTrieConfMgr; 
        char* m_pTrieBranchMgr;
        
    public:
        TShmList<TMdbTable>   m_TableList;//����
        TShmList<TMdbTableSpace> m_TSList;//��ռ�
        TShmList<TObservePoint>    m_ObserverList;//�۲��
        TShmList<TMdbProc>           m_ProcList;//��������
        TShmList<TMdbLocalLink>    m_LocalLinkList; //��������
        TShmList<TMdbRemoteLink> m_RemoteLinkList;//Զ������
        TShmList<TMemSeq>          m_MemSeqList;// 
        TShmList<TMdbRepLink>     m_RepLinkList;
        TShmList<TMdbJob>            m_JobList;//job�б�
        TShmList<TMdbVarchar>        m_VarCharList; //Varchar��
        
        TShmList<TMdbMhashBlock> m_MhashConfList; // ����ʽ������ͻ����������
        TShmList<TMdbMhashBlock> m_MhashLayerList; // ����ʽ������������������

		TShmList<TMdbTrieBlock> m_TrieBranchList; // �����������ڵ�����������
        TShmList<TMdbTrieBlock> m_TrieConfList; // ����������ͻ����������
        
    };


    //�������ÿ��DSN��Ϣ
    class TMdbShmMgr
    {
    public:
        /******************************************************************************
        * ��������	:  GetShmDSN()
        * ��������	:  �����������Ĺ����ڴ��
        * ����		:  config, ������ϢbSkipErrorLog -  ����������־
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        static TMdbShmDSN* GetShmDSN(const char* pszDSN,bool bSkipErrorLog = false);
        /******************************************************************************
        * ��������	:  CreateMgrShm()
        * ��������	:  �����������Ĺ����ڴ��
        * ����		:  config, ������Ϣ
        * ���		:  ��
        * ����ֵ	:  �ɹ�����0�����򷵻�-1
        * ����		:  li.shugang
        *******************************************************************************/
        static int CreateMgrShm(TMdbConfig &config);
    private:
        static TMdbShmDSN m_tShmDSN[MAX_DSN_COUNTS];
    };

//}
#endif //__ZX_MINI_DATABASE_MANAGER_SHARE_MEMORY_H__


