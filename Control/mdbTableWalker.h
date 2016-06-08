/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbTableWalker.cpp		
*@Description�� MDB������ࡣ�����������б�����
*@Author:		jin.shaohua
*@Date��	    2012.04
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

    // ������������ʽ��������¼����λ��
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
        bool m_bCheckLastLayerConf; // �Ƿ���Ҫ�����ײ���ݽڵ��ϵĳ�ͻ���ڵ�
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
        int iChildIdx; //��¼�ߵ���һ�����ӽڵ���
    };
	

    /*
    ** mdbTable������,�������������������ȫ�����
    */
    class TMdbTableWalker
    {
    public:
    	TMdbTableWalker();
    	~TMdbTableWalker();
    	int AttachTable(TMdbShmDSN * pShmDSN,TMdbTable * m_pMdbTable);//���ӱ�
        int WalkByIndex(ST_TABLE_INDEX_INFO* pIndexInfo, long long lIndexValue);
    	bool Next();//��һ��
    	inline char * GetDataAddr(){	return m_pDataAddr;}//��ȡ���ݵ�ַ
    	inline MDB_INT32 GetDataSize(){return m_iDataSize;}//��ȡ���ݴ�С
    	inline char * GetPageAddr(){return (char *)m_pCurPage;}//��ȡҳ��ַ
    	inline MDB_INT32 GetPagePos(){return m_iPagePos;}//
    	inline TMdbRowID GetDataRowID(){return m_tCurRowIDData;}//��ȡ���ݵ�rowid;
    	int StopWalk();//ֹͣ����
    	int WalkByPage(int iStartPageID);//����ҳ����������
    	bool NextByPage();//
    	char* GetAddressRowID(TMdbRowID* pRowID, int &iDataSize, bool bGetPageAddr = false);
		
    protected:
        // hash ��������
        int WalkByHashIndex(ST_TABLE_INDEX_INFO* pIndexInfo, long long lIndexValue);
        bool NextByHash();

        // m-hash��������
        int WalkByMHashIndex(ST_TABLE_INDEX_INFO* pIndexInfo, long long lIndexValue); 
        bool NextByMHash();
        char* GetAddrByMhashIndexNodeId(int iHeadBlock,int iIndexNodeId, int iNodeSize, bool bConf);
        TMdbMhashBlock* GetMhashBlockById(int iBlockID, bool bConf);

		//trie��������
		int WalkByTrieIndex(ST_TABLE_INDEX_INFO* pIndexInfo);		
		bool NextByTrie();
		char* GetAddrByTrieIndexNodeId(int iHeadBlock,int iIndexNodeId, int iNodeSize, bool bConf);
		TMdbTrieBlock* GetTrieBlockById(int iBlockID, bool bConf);
		
    private:
        TMdbShmDSN * 	m_pShmDSN;//
        TMdbDSN * 		m_pMdbDSN;//DSN
        TMdbTableSpace * m_pTableSpace;//��ռ�
        TMdbTable * 	       m_pMdbTable;//��
        MDB_INT32  			m_iDataSize;   //���ݳ���
        char * 			m_pDataAddr;//���ݵ�ַ��Ҳ�ɱ�ʾ�����ڵ�ĵ�ַ
        char *			m_pNextDataAddr;//��ʱ���������ڱ���m_pDataAddr����һ��λ��
        int     m_iPagePos;     //������ҳ���е�λ��
        TMdbRowID 		m_tNextRowIDData,m_tCurRowIDData;
        TMdbTableSpaceCtrl m_tTSCtrl;//��ռ����
        int m_iCurWalkIndexAlgo; //��ǰ���������㷨����
        
        // hash��������ʹ��
        TMdbIndexNode* m_pConflictIndex; 
        MDB_INT32 			m_iNextIndexPos; //��һ��index��λ��

        // ����ʽ��������ʹ��
        TMdbMHashBaseIndexNode* m_pMHashBaseIdx; // �����������ڵ�
        TMHashRowIndex m_tMHashRowIdx; // ��¼��������

		//�ֵ�����������ʹ�� 
		TMdbTrieIndexNode* m_pTrieRootNode;
		TMdbTrieConflictIndex* m_pTrieConfIndex;
        TMdbTrieBranchIndex* m_pTrieBranchIndex;
		bool m_bStopScanTrie;
				
        bool m_bScanAll;
		
	public:	
        TMdbPage * m_pCurPage;    //��������ҳ��ĵ�ַ
        TMdbPage * m_pStartPage; //�Ƿ��������FREE����ͷ���
		int m_iScanPageType; //0 free��  1 full��
		int m_iFirstFullPageID;
		int m_iFirstFreePageID;
		TMdbPage * m_pNextPage;
		int m_iAffect;
		char m_sTrieWord[MAX_TRIE_WORD_LEN];
    };
//}
#endif

