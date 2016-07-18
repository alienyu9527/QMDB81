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

    //index���䷽��
    struct ST_SHM_ASSIGN_CELL
    {
        int iTable;//��pos
        int iIndex;//index  pos
        MDB_INT64 iSize;// size
    };
    struct ST_SHM_ASSIGN
    {
        std::vector<ST_SHM_ASSIGN_CELL> vCell;
        MDB_INT64 iMemLeft;     //ʣ���ڴ��С
    };

    //��������Ϣ
    struct ST_TABLE_INDEX_INFO
    {
        bool bInit;//�Ƿ񱻳�ʼ��
        TMdbIndex* pIndexInfo;//��¼������Ϣ
        int  iIndexPos;//�ڼ���index
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
	 	ST_TABLE_INDEX_INFO * GetAllIndexByColumnPos(int iColumnPos,int &iColNoPos,int &iCurIndexPos);//����columnpos��ȡindexnode
		ST_TABLE_INDEX_INFO * GetIndexByName(const char* sName);

        //���ݿ��ܵ������л�ȡ����
        int GetIndexByIndexColumn(std::vector<ST_INDEX_COLUMN> & vIndexColumn,std::vector<ST_INDEX_VALUE> & vIndexValue, ST_TABLE_INDEX_INFO* pHintIndex);
		int FindBestIndex(ST_INDEX_COLUMN ** ppIndexColumnArr,std::map<int,int>& mapCfgToWhereClause,int iHintIndexPosInCfg);

		ST_TABLE_INDEX_INFO * GetScanAllIndex();//��ȡȫ������������

        ST_TABLE_INDEX_INFO * GetVerfiyPKIndex();//��ȡУ������������

        int InsertIndexNode(int iIndexPos,char* pDataAddr, TMdbRowCtrl& tRowCtrl,TMdbRowID& rowID);//���������ڵ�
        int DeleteIndexNode(int iIndexPos,char* pAddr,TMdbRowCtrl& tRowCtrl, TMdbRowID& rowID);
        int UpdateIndexNode(int iIndexPos,char* pOldData,ST_INDEX_VALUE& tMemIndexValue,TMdbRowCtrl& tRowCtrl,TMdbRowID& tRowId);//���������ڵ�

        int PrintIndexInfo(int iDetialLevel,bool bConsole);       
        int PrintWarnIndexInfo(int iMaxCNodeCount);

		int GetTrieWord( ST_INDEX_VALUE & stIndexValue, char* sTrieWord);
		int GetTrieWord( TMdbRowCtrl& tRowCtrl,char* pAddr, TMdbIndex* pIndex, char* sTrieWord);
		
        long long CalcIndexValue( TMdbRowCtrl& tRowCtrl,char* pAddr, TMdbIndex* pIndex, int& iError);
        int RenameTableIndex(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,const char *sNewTableName);//rename ��� ���������еı���
		bool CheckHashConflictIndexFull();
		
    private:
        void CleanTableIndexInfo();
        int GenerateIndexValue(ST_INDEX_COLUMN *pIndexColumnArr [] ,ST_TABLE_INDEX_INFO  * pstTableIndexInfo,int iCurPos,std::vector<ST_INDEX_VALUE> & vIndexValue);

        // hashֵ����
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

		//�˴���¼���ǵ�ǰ�� �����������Ϣ����ʵʹ��ʱ,�����ӵ������������ڵ�
        ST_TABLE_INDEX_INFO   m_arrTableIndex[MAX_INDEX_COUNTS];

        char     m_sTempValue[MAX_BLOB_LEN];//�����ʱ�ַ�����Ϊ������
        
    };
//}
#endif
