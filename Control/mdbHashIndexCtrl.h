#ifndef _MDB_HASH_INDEX_CTRL_H_
#define _MDB_HASH_INDEX_CTRL_H_


#include "Helper/mdbDictionary.h"
#include "Helper/mdbConfig.h"

//namespace QuickMDB{

    class TMdbRowCtrl;
    
    struct ST_HASH_INDEX_INFO
    {
        
        TMdbBaseIndexMgrInfo     * pBIndexMgr;    //��������������
        TMdbConflictIndexMgrInfo * pCIndexMgr;	  //��ͻ����������
        int 					   iBaseIndexPos; //��������λ��
        int 					   iConflictIndexPos;//��ͻ����λ��
        TMdbBaseIndex            * pBaseIndex;    //����������Ϣ
        TMdbConflictIndex        * pConflictIndex;//��ͻ������Ϣ
        TMdbIndexNode            * pBaseIndexNode;//����������
        TMdbIndexNode            * pConflictIndexNode;//��ͻ������

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
        //������������
        int AttachDsn(TMdbShmDSN * pMdbShmDsn);//�����Ϲ����ڴ������
        int AttachTable(TMdbShmDSN * pMdbShmDsn, TMdbTable* pTable);
        //int CreateAllIndex(TMdbConfig &config);
        int AddTableSingleIndex(TMdbTable * pTable,int iIndexPos,size_t iDataSize);
        //int AddTableIndex(TMdbTable * pTable,size_t iDataSize);//�������
        int DeleteTableIndex(ST_HASH_INDEX_INFO& tIndexInfo);//ɾ��ĳ���������
        int TruncateTableIndex(ST_HASH_INDEX_INFO& tIndexInfo);
        //int DeleteTableSpecifiedIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,const char* pIdxName);
        //��ȡ����+ ��ͻ�����ڵ�
        //int AttachTable(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable);
        //int GetBCIndex(TMdbIndexNode ** pBaseNode,TMdbIndexNode ** pConflictNode);//��ȡ����+ ��ͻ����
        int InsertIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndex, TMdbRowID& rowID);//���������ڵ�
        int UpdateIndexNode(TMdbRowIndex& tOldRowIndex,TMdbRowIndex& tNewRowIndex,ST_HASH_INDEX_INFO& tHashInfo,TMdbRowID& tRowId);//���������ڵ�
        int DeleteIndexNode(TMdbRowIndex& tRowIndex,ST_HASH_INDEX_INFO& tHashIndexInfo,TMdbRowID& rowID);
        int SelectIndexNode(MDB_INT64 iIndexValue);//���ڲ�ѯ�������ڵ�
        int PrintIndexInfo(ST_HASH_INDEX_INFO& tIndexInfo,int iDetialLevel,bool bConsole);//��ӡ��ϸ������ϢiDetialLevel=[0~3] 0-������Ϣ��1-һ����Ϣ��2-��ϸ��Ϣ
        int PrintIndexInfoDetail(int iDetialLevel,bool bConsole,ST_HASH_INDEX_INFO & stIndexInfo);//��ӡ��ϸ������ϢiDetialLevel=[0~3] 0-������Ϣ��1-һ����Ϣ��2-��ϸ��Ϣ
        //ST_TABLE_INDEX_INFO * GetIndexByColumnPos(int iColumnPos,int &iColNoPos);//����columnpos��ȡindexnode
        //ST_TABLE_INDEX_INFO * GetScanAllIndex();//��ȡȫ������������
        //ST_TABLE_INDEX_INFO * GetVerfiyPKIndex();//��ȡУ������������
        //bool CombineCMPIndex(ST_INDEX_VALUE & stLeftIndexValue,ST_INDEX_VALUE & stRightIndexValue, ST_INDEX_VALUE & stOutIndexValue);//ƴ���������
        int PrintWarnIndexInfo(int iMaxCNodeCount);//��ӡ���ڳ�ͻ������ָ�����ȵ�����
        //int RebuildTableIndex(bool bNeedToClean);//���¹���ĳ��������
        int FindRowIndexCValue(ST_HASH_INDEX_INFO tHashIndexInfo,TMdbRowIndex & tRowIndex,TMdbRowID& rowID);//���ҳ�ͻ����ֵ
        //int GetIndexByIndexColumn(std::vector<ST_INDEX_COLUMN> & vIndexColumn,std::vector<ST_INDEX_VALUE> & vIndexValue);//���ݿ��ܵ������л�ȡ����
        //int GenerateIndexValue(ST_INDEX_COLUMN * pIndexColumnArr[],ST_TABLE_INDEX_INFO  * pstTableIndexInfo,int iCurPos,std::vector<ST_INDEX_VALUE> & vIndexValue);
        //int RemoveDupIndexColumn(std::vector<ST_INDEX_COLUMN> & vLeftIndexColumn,std::vector<ST_INDEX_COLUMN> & vRightIndexColumn);//����ظ��Ŀ���������
        //int RemoveDupIndexValue(std::vector<ST_INDEX_VALUE> & vLeftIndexValue,std::vector<ST_INDEX_VALUE> & vRightIndexValue);//����ظ�������
    private:
        //void CleanTableIndexInfo();//������������Ϣ
        int InitBCIndex(ST_HASH_INDEX_INFO & tTableIndex,TMdbTable * pTable);
        int InitIndexNode(TMdbIndexNode* pNode,MDB_INT64 iSize,bool bList);
        int GetFreeBIndexShm(MDB_INT64 iBaseIndexSize,size_t iDataSize,
            ST_HASH_INDEX_INFO & stTableIndexInfo);//��ȡ���������ڴ��
        int GetFreeCIndexShm(MDB_INT64 iConflictIndexSize,size_t iDataSize,
            ST_HASH_INDEX_INFO & stTableIndexInfo);//��ȡ���������ڴ��
        int CreateNewBIndexShm(size_t iShmSize);//�����µĻ��������ڴ��
        int CreateNewCIndexShm(size_t iShmSize);//�����µĳ�ͻ�����ڴ��
        int OutPutInfo(bool bConsole,const char * fmt, ...);//�����Ϣ
        int GetTableByIndexName(const char * sIndexName,TMdbTable * & pTable);//������������ȡ��
        int RecycleIndexSpace(TMDBIndexFreeSpace tFreeSpace[],size_t iPosAdd,size_t iSize);//���������ռ�
        int DefragIndexSpace(TMDBIndexFreeSpace tFreeSpace[]);//���������ռ�
        int PrintIndexSpace(const char * sPreInfo,TMDBIndexFreeSpace tFreeSpace[]);//��ӡ�����ռ���Ϣ

     
    private:
        TMdbTable * m_pAttachTable;
        TMdbShmDSN * m_pMdbShmDsn;//MDB���������
        TMdbDSN   * m_pMdbDsn;
        
        //ST_HASH_INDEX_INFO   m_arrTableIndex[MAX_INDEX_COUNTS];//����ĳ�ű��������Ϣ
        
        MDB_INT64 m_lSelectIndexValue;//���ڲ�ѯ��������ͻ��
        
    };
//}

#endif
