/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbStorageEngine.h	
*@Description�� �洢����
*@Author:			jin.shaohua
*@Date��	    2013.7
*@History:
******************************************************************************************/
#ifndef _MDB_STORAGE_ENGINE_H_
#define  _MDB_STORAGE_ENGINE_H_
#include "Helper/mdbStruct.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Control/mdbPageCtrl.h"
#include "Helper/mdbShm.h"
#include "Helper/mdbDictionary.h"
#include "Helper/mdbFileList.h"
#include "Control/mdbVarcharMgr.h"
#include "Control/mdbRowCtrl.h"
#include <vector>
//#include "BillingSDK.h"
//#include "BillingNTC.h"

//using namespace QuickMDB;
//using namespace ZSmart::BillingSDK;	

//    namespace QuickMDB{
#define STRUCT_NO_CHANGE	  0x00  //�ṹ�ޱ仯
#define STRUCT_TABLE_CHANGE     0x01  //�����仯
#define STRUCT_TABLESPACE_CHANGE	  0x02  //��ռ䷢���仯
#define STRUCT_TABLE_IN_TABLESPACE_CHANGE    0x04  //��ռ����б仯�ı�
#define StructHasChange(V,P)     (((V)&(P))==(P))
#define StructHasAnyChange(V,P)  (((V)&(P))!=0)
#define StructAddChange(V,P)     (V)|=(P)
#define StructSetChange(V,P)     (V)=(P)
#define StructClearChange(V,P)   (V)V&=~(P)
class TMdbStorageCopy;

    class TMdbTSFileHead
    {
    public:
        void Clear();//����
        std::string ToString();//���
    public:
        char m_sTSName[MAX_NAME_LEN];
		int iVarcharID;
		int m_iStartPage; //���ļ�����ʼҳ��
		int m_iEndPage;  //���ļ��Ľ���ҳ��
        int m_iPageSize;//ҳ��С
        int m_iPageCount; //ҳ����
        MDB_INT64  m_iPageOffset;//����ҳ��ַ
        MDB_INT64  m_iCheckPointLSN;
    };

	class StorageFileHandle
	{
	public:
		StorageFileHandle();
		~StorageFileHandle();
		char m_sFileName[MAX_NAME_LEN];
		FILE * m_fFile;
	};
	

	class StorageFile
	{
	public:
		StorageFile();
		~StorageFile();
		void clear();
		char m_sTSName[MAX_NAME_LEN];
		int    m_iVarcharId;
		std::vector<StorageFileHandle> m_vStorageFile;
	};

	class TMdbFileBuff
	{
	public:
		TMdbFileBuff();
		~TMdbFileBuff();
		int Init(int iSize,FILE* pTSFile);
		int ReadFromFile();
		char* Next();
	private:
		char* m_pBuff;
		size_t m_iBuffSize;
		int m_iSize;
		FILE * m_pFile;
		int iReadPageCount;
	};
	class TMdbVarcharFile
	{
	public:
		TMdbVarcharFile();
		~TMdbVarcharFile();
		int LinkFile(const char * sFile);//��ȡ
		int LinkMdb(const char * sDsn,TMdbVarchar *pVarchar);//����mdb
		int FlushFull(int iStartPageId = 1);
		int FlushHead();
		int StartToReadPage();//��ʼ��ȡҳ����
		int GetNextPage(TMdbPage * &pMdbPage);//��ȡ��һ��ҳ����
		int FlushDirtyPage();//ˢ����ҳ
		int GetVarCharLen(int iPos,int& iVarCharLen);
	public:
		TMdbVarchar *m_pVarchar;
		TMdbTSFileHead m_VarcharHead;
		FILE * m_pVarcharFile;//varchar�ļ����
		TMdbShmDSN* m_pShmDSN;
		TMdbPageCtrl m_tPageCtrl;
		TMdbVarCharCtrl m_VarCharCtrl;
		TMdbFileBuff m_tFileBuff;
	};

    class TMdbTSFile
    {
    public:
        TMdbTSFile();
        ~TMdbTSFile();
        int LinkFile(const char * sFile);//��ȡ
        int LinkMdb(const char * sDsn,TMdbTableSpace *pMdbTS);//����mdb
        int FlushFull(int iStartPageId = 1);//ˢ����ռ�
        int StartToReadPage();//��ʼ��ȡҳ����
        int GetNextPage(TMdbPage * &pMdbPage);//��ȡ��һ��ҳ����
        int FlushDirtyPage(int iDirtyPageID);//ˢ����ҳ
        int FlushHead();
    public:
	 FILE * m_pTSFile;//��ռ��ļ�	
        TMdbTSFileHead m_tTSFileHead;//�ļ�ͷ  
        TMdbShmDSN * m_pShmDSN;
        TMdbTableSpace *m_pMdbTS;
        TMdbTableSpaceCtrl m_tTSCtrl;
        TMdbPageCtrl m_tPageCtrl;
	 TMdbFileBuff m_tFileBuff;
    };

	class TMdbStorageCopy
	{
	public:
		TMdbStorageCopy();
		~TMdbStorageCopy();
		int Init(TMdbShmDSN* pShmDSN,TMdbTable * pTable,TMdbTableSpace* pTMdbTableSpace,TMdbPageCtrl*pPageCtrl,TMdbTableSpaceCtrl* pTableSpaceCtrl);
		int Load(TMdbPage * pPage);                                //�������
		int SetStructChangeType();                        //���ñ�ռ估��ṹ�仯����
		int CopyByPage(TMdbPage * pPage);                 //ҳ����,�ļ�ҳ�����ڴ�ҳ�Ų���Ӧ�����غ���Ҫ��������ļ�
		int CopyByPageOrder(TMdbPage * pPage);            //ҳ���������ļ�ҳ�����ڴ�ҳ�Ŷ�Ӧ�����غ�������������ļ�
		int CopyByRecord(TMdbPage * pPage);                 //��¼�������ļ�ҳ�����ڴ�ҳ�Ų���Ӧ�����غ���Ҫ��������ļ�
		int ColumnCopyByDataTypeChange(TMdbColumn * pNewColumn,TMdbColumn * pOldColumn,char* pMemAddr);          //���ͷ����仯����ֶο���
		int ColumnCopyByDateTimeChange(TMdbColumn * pNewColumn,TMdbColumn * pOldColumn,char* pMemAddr);  
		int GetNextCopySize(int iSize);                               //�����ṹ�����仯����Ҫ�ֶν��м�¼��������ȡ�ֶο�����С��>0 ��ʾ��Ҫ�����ĳ��ȣ�<=0��ʾ�������
		int FillData(char* pMemAddr);
	private:
		TMdbConfig * m_pConfig;
		TMdbShmDSN* m_pShmDSN;
		TMdbTable* m_pTable;                     //��ָ��
		TMdbTable* m_pOldTable;
		TMdbTableSpace *m_pTMdbTableSpace;     //��ռ�ָ�� 
		TMdbTableSpaceCtrl* m_pTableSpaceCtrl; //��ռ������ָ��
		TMdbPageCtrl*   m_PageCtrl;             //ҳ�������
		char * m_pInsertBlock;                    //�����
		TMdbVarCharCtrl* m_pVarcharCtrl;
		int m_iStructChangeType;              //��ṹ�仯����
		TMdbRowID m_rowID;                        //û��ʵ�����ã�ӭ�Ͻӿ�
		TMdbRowCtrl m_OldRowCtrl;
		TMdbRowCtrl m_NewRowCtrl;
		TMdbColumnNullFlag * m_NewColNullFlag; //�½ṹ��null��־λ
		TMdbColumnNullFlag * m_OldColNullFlag;
	};

    class TMdbCheckPoint
    {
    public:
        TMdbCheckPoint();
        ~TMdbCheckPoint();
        public:
        int    Init(char* sDsn);
        int    Init(TMdbTableSpace* pTS);
        int    Init(TMdbVarchar *pVarchar);
		bool   NeedFlushFile();
        bool   NeedLinkFile();
		int    LinkStorageFile(const char * sTableSpaceName = NULL);
		int    LinkChangeFile(const char * sTableSpaceName = NULL);
        int    LinkFile(const char * sTableSpaceName = NULL);
		int    WritePageArrayToFile(char* sArray, int & iCount);
		int    WriteChangeFile();
		int    WriteVarcharPageArrayToFile(char* sArray, int & iCount);
		int    WriteVarcharChangeFile();
		int    FlushChangeFile(const char * sTableSpaceName = NULL);
		int    FlushStorageFile(const char * sTableSpaceName = NULL);
        int    DoCheckPoint(const char * sTableSpaceName = NULL);         
        int    FlushDirtyPage();
		int    GetFlushChangeFile(FILE* &pFile);
        int    FlushVarcharDirtyPage();
		int    GetFlushVarcharChangeFile(FILE* &pFile);
        int    GetFlushFile(int iPageId,FILE* &pFile);
        int    GetVarcharFlushFile(int iPageId,FILE* &pFile);
        int    ClearRedoLog();
        
    private:
	    TMdbShmDSN * m_pShmDSN;
	    TMdbTableSpace* m_pTS;
	    TMdbVarchar* m_pVarchar;
	    TMdbFileList m_tFileList;
	    TMdbTSFileHead m_tTSFileHead;
	    TMdbTableSpaceCtrl m_TSCtrl;
	    TMdbVarCharCtrl m_VarCharCtrl;
	    TMdbPageCtrl m_tPageCtrl;
	    int m_iCurPos;
		int m_iCurChangePos;
	    long long m_iMaxLsn;
	    TMdbNtcSplit m_tSplit;
	    std::vector<StorageFile> m_vTSFile;//���еı�ռ�file
	    std::vector<StorageFile> m_vVarCharFile;//����Varchar�ռ��ļ�
	    std::vector<StorageFile> m_vTSChangeFile;//���б�ռ����ʱ��ҳ�ļ�
		std::vector<StorageFile> m_vVarCharChangeFile;//����Varchar�ռ����ʱ��ҳ�ļ�
		char* m_pVarCharArray;
    };

    class TMdbLoadFromDisk
    {
    public:
        TMdbLoadFromDisk();
        ~TMdbLoadFromDisk();
        public:
        int Init(char* sDsn);
        int LinkFile();
        int LoadNormalData();
        int LoadVarcharData();
        int LoadRedoLog();
        int StartReadPage(FILE* pFile);
        int GetNextPage(TMdbPage * &pMdbPage);//��ȡ��һ��ҳ����
    private:
        TMdbShmDSN * m_pShmDSN;
        TMdbFileList m_tFileList;
        TMdbTableSpaceCtrl m_TSCtrl;
        TMdbPageCtrl m_tPageCtrl;
        TMdbStorageCopy StorageCopyCtrl;
        TMdbVarCharCtrl m_tVarCharCtrl;
        TMdbFileBuff m_tFileBuff;
        TMdbTSFileHead m_tTSFileHead;//�ļ�ͷ  
        std::vector<StorageFile> m_vTSFile;//���еı�ռ�file
        std::vector<StorageFile> m_vVarCharFile;//����Varchar�ռ��ļ�
    };
    //mdb�洢����
    class TMdbStorageEngine
    {
    public:
        TMdbStorageEngine();
        ~TMdbStorageEngine();
        void Clear();//����
        int Attach(const char * sDsn);//���ӹ����ڴ�
        int BackupFile();
		int RemoveBakFile();
        int FlushFull();//ˢ���������ݣ���Ϊ��ʼ��
        int RemoveNormalFile(); //ɾ����varchar�ļ�
        int RemoveVarcharFile();//ɾ��varchar�ļ�
        int RemoveChangeFile();//ɾ��change�ļ�
        //int FlushVarchar(); //ˢ��varcharҳ
        //int LoadDataFromDisk();//�Ӵ�����������
        //int LoadVarcharDataFromDisk(); //����varchar����
        //int LoadDataFromRedoLog();//��redoLog��������
    private:
        //int LinkDiskToMdb();//���Ӵ��̣���ȡ��������
        //int ClearRedoLog(int iChkLSN);//������ڵ�redo��־
    private:
	std::vector<TMdbTSFile*> m_vTSFile;//���еı�ռ�file
	std::vector<TMdbVarcharFile*> m_vVarCharFile;//����Varchar�ռ��ļ�
	TMdbShmDSN * m_pShmDSN;
	TMdbFileList m_tFileList;
	TMdbConfig * m_pConfig;
    };

    //redo��־����
    class TMdbRedoLogParser
    {
    public:
        TMdbRedoLogParser();
        ~TMdbRedoLogParser();
        int ParseFile(const char * sFullPathFile);//�����ļ�
        int NextRecord(char* &sRecordBuff,int & iLen);//��ȡ��һ����¼
        int Analyse(const char* sRecord,TMdbLCR& tMdbLcr);//����һ����¼
        MDB_INT64 GetPageLSN(char * sRecordBuff);
		long long GetFileSize(const char* pFile); //��ȡ�ļ���С
		int GetNum(const  char* pNum,int iNumSize);
		int GetTableName(char* sRecord,std::string& sTableName);
		int GetWhereValue(char * sRecord); 
		char* GetSQL(TMdbLCR& tMdbLcr);//���ݽ������ƴдsql
		int GetRoutingID(char * sRecord);
        char GetVersion(char * sRecord);
        int GetSyncFlag(char *sRecord);//��ȡͬ����ʶ
	public:
		TMdbNtcSplit m_tSplitWhere; //�����Ƿ���where
		TMdbNtcSplit m_tSplitCol;   //��������
    private:
        FILE * m_pCurFile;
		char* m_pFileBuff;
		long long m_iBuffSize;
		long long m_iCurPos;
		TLCRColm tLcrColumn;     //����Ϣ
		int m_iFactor[10]; // ������ֵ����
		char m_sRout[10];//����·��idʹ��
        char m_sSyncFlag[10];//����ͬ����ʶʹ��
    };
//}
#endif

