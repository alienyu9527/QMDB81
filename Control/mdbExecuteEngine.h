#ifndef _MDB_EXECUTE_ENGINE_H_
#define _MDB_EXECUTE_ENGINE_H_
#include "Helper/mdbSQLParser.h"
/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbExecuteEngine.h
*@Description�� SQLִ������
*@Author:	     jin.shaohua
*@Date��	    2012.05
*@History:
******************************************************************************************/
#include "Control/mdbTableWalker.h"
#include "Control/mdbPageCtrl.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Control/mdbLimitCtrl.h"
#include "Control/mdbFlush.h"
#include "Helper/mdbErrorHelper.h"
#include "Control/mdbObserveCtrl.h"
#include "Control/mdbVarcharMgr.h"
#include "Control/mdbRowCtrl.h"
#include "Helper/mdbCacheTable.h"
#include "Interface/mdbQuery.h"

//namespace QuickMDB{

    class TMdbRollback;
    class NoOcpParse;
    //mdb ִ������
    class TMdbExecuteEngine
    {
    public:
        TMdbExecuteEngine();
        ~TMdbExecuteEngine();
        int Init(TMdbSqlParser * pMdbSqlParser, MDB_INT32 iFlag, TMdbLocalLink* pLocalLink);//��ʼ������
		int Init(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,TMdbLocalLink* pLocalLink);
		int Execute();//ִ��
        int Next(bool & bResult);//��ȡ��һ��
        int FillCollist();//�����ֵ
        int GetRowsAffected()
        {
            return m_iRowsAffected;    //��ȡӰ��ļ�¼��
        }
		void SetLink(TMdbLocalLink* pLocalLink);
        void SetRollback(bool bCanRollback);
        int GetOneRowData(void *pStruct,int* Column);//��ȡһ��������Ϣ
		int GetOneRowData(TMdbColumnAddr* pTColumnAddr, bool bBlob = true);
		int IsPKExist(char* pDataAddr);
        long long GetRowTimeStamp();
        int CheckRowDataStruct(int* Column);//У��ṹ������Ϣ
        int ReBuildTableFromPage(const char * sDSN,TMdbTable * pMdbTable);//���ڴ�ҳ���¹�����
		
		int FSCheckInvalidData();
		int FSRemoveInvalidData();
		
		
		int BuildSingleIndexFromPage(TMdbShmDSN * pShmDSN,TMdbTable * pMdbTable,int iIndexPos);
		void ClearLastExecute();
        int FillFieldForCSBin(NoOcpParse &tParseData,bool bFirst);
		void SetCancelPoint(int* pPoint);	
		int ExecuteDelete(char* pPage,char* pDataAddr,TMdbRowID rowID,TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable);     protected:
        int ExecuteInsert();//ִ�в���
        int ExecuteUpdate();//ִ�и���
        int ExecuteDelete();//ִ��ɾ��
		
		int ExecuteUpdateTrans();
		int CloneVarChar(char* pDestBlock,char* pSourceBlock, ST_MEM_VALUE_LIST & stMemSkipList);
			
        int FillSqlParserValue(ST_MEM_VALUE_LIST & stMemValueList);//����������ֵ
        int CheckWhere(bool &bResult);//���where�����Ƿ�����
		int CheckVisible(bool &bResult);
		int CheckDiskFree();
		bool IsUseTrans();
        
        int InsertDataFill(char* const &sTmp);//���Ҫ���������
        int InsertData(char* pAddr, int iSize,TMdbRowID&  rowID);//�����ݲ��뵽�ڴ���
        int UpdateData();//�����ݸ��µ�
        int ChangeInsertIndex( char* pAddr, TMdbRowID& rowID);
        int ChangeUpdateIndex(std::vector<ST_INDEX_VALUE > & vUpdateIndex);//��update ��������ɵ��������
        int GetNextIndex(ST_TABLE_INDEX_INFO * & pTableIndex,long long & llValue);//��ȡ��һ������
		int SetTrieWord(ST_INDEX_VALUE&  stIndexValue);
		//int GetNextIndex(ST_TABLE_INDEX_INFO * & pTableIndex);//��ȡ��һ������
        //long long CalcMPIndexValue( char* pAddr, int iIndexPos, int& iError);
        //long long CalcOneIndexValue( char* pAddr, int iIndexPos, int& iError);
        //long long CalcIndexValue( char* pAddr, int iIndexPos, int& iError);
        int CalcMemValueHash(ST_INDEX_VALUE & stIndexValue,long long & llValue);
		int CalcMemValueHash(ST_TABLE_INDEX_INFO * pstTablePKIndex,char* pDataAddr,long long & llValue,TMdbColumnAddr &tColumnAddr);
		int DeleteVarCharValue( char*  const &pAddr);//ɾ���䳤����
        int ClearMemValue(ST_MEM_VALUE_LIST & stMemValueList);//��������
        int ExecuteCacheSelect();//ִ�л����ѯ
        bool CacheNext();//����next
        //bool NormalNext();
        inline bool  IsNeedReadLock();//�Ƿ���Ҫ�Ӷ���
        int NextWhere(bool & bResult);//����where����        
		int NextWhereByPage(bool & bResult);
		int NextWhereByIndex(bool & bResult);
		
        int IsPKExist();//��������Ƿ��Ѵ���
		bool IsPKValueSame(TMdbColumnAddr& tIColumnAddr, TMdbColumnAddr& tNColumnAddr);
		
        bool IsDataPosBefore();//�ü�¼�Ƿ񱻶�λ��
        inline int * GetRowIndexArray(char * pDataAddr);//��ȡrowindex����
        int GetUpdateDiff();//��ȡ��������ֵ
        int SetRowDataTimeStamp(char* pAddr, int iOffset,long long iTimeStamp = 0);
        int UpdateRowDataTimeStamp(char* const & pAddr, int iOffset, long long iTimeStamp = 0);
		void SetDataFlagInsert(char* pAddr);
		int SetDataFlagDelete(char* pAddr);	
		void  InitDataMutex(char* pAddr);
		bool CheckIsStop(){return m_pIsStop&&(1==*m_pIsStop);}
		
	private:

        TMdbSqlParser * m_pMdbSqlParser;//�﷨���ṹ
        int m_iCurIndex;//����ʹ�õڼ�������
        std::vector<ST_INDEX_VALUE > * m_pVIndex;
        TMdbTableWalker m_MdbTableWalker;//mdb ��ı�����
        char * m_pDataAddr;//���ݵ�ַ
        TMdbRowID m_tCurRowIDData ;
        char* m_pPageAddr;    //��������ҳ��ĵ�ַ
        int   m_iPagePos;     //������ҳ���е�λ��

        bool m_bScanAll;//ȫ������
        long long m_llScanAllPos;//��¼ȫ���������ϴ�λ��
        TMdbTable * m_pTable;//������ĳ����
        TMdbDSN   * m_pDsn;
		TMdbShmDSN* m_pMdbShmDsn;


        TMdbPageCtrl   m_mdbPageCtrl;              //ҳ������Ϣ
        TMdbTableSpaceCtrl m_mdbTSCtrl;            //��ռ������Ϣ
        int m_iRowsAffected;						//Ӱ��ļ�¼��
        TMdbIndexCtrl   m_mdbIndexCtrl;//��������
        int    m_iMoniNext; //ģ��next
        int    m_iNextType;//next������
        TMDBLimitCtrl  m_tLimitCtrl;//limit
        char * m_pInsertBlock;//�����
        char * m_pVarCharBlock;//�䳤char ������������������

		bool  m_bCanRollBack;
        TMdbFlush    m_tMdbFlush;//mdb ˢ��ģ��

        char     m_sTempValue[MAX_BLOB_LEN];//�����ʱ�ַ�����Ϊ������
        TObserveTableExec m_tObserveTableExec;//��ر����
        //TMdbVarcharMgr m_tVarcharMgr;
        TMdbVarCharCtrl m_tVarcharCtrl;
        char * m_pUpdateBlock;//���¿�
        TMdbRowCtrl m_tRowCtrl;//��¼����
        int* m_aRowIndexPos; 
        TMdbCacheTable m_tCacheTable;//�����
		TMdbLocalLink* m_pLocalLink;
		int* m_pIsStop;    //��;ֹͣ
		TMdbColumnAddr m_tIColumnAddr,m_tNColumnAddr;
	public:
        TMdbErrorHelper m_tError;
    };
//}



#endif
