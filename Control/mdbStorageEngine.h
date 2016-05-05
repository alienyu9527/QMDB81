/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbStorageEngine.h	
*@Description： 存储引擎
*@Author:			jin.shaohua
*@Date：	    2013.7
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
#define STRUCT_NO_CHANGE	  0x00  //结构无变化
#define STRUCT_TABLE_CHANGE     0x01  //表发生变化
#define STRUCT_TABLESPACE_CHANGE	  0x02  //表空间发生变化
#define STRUCT_TABLE_IN_TABLESPACE_CHANGE    0x04  //表空间内有变化的表
#define StructHasChange(V,P)     (((V)&(P))==(P))
#define StructHasAnyChange(V,P)  (((V)&(P))!=0)
#define StructAddChange(V,P)     (V)|=(P)
#define StructSetChange(V,P)     (V)=(P)
#define StructClearChange(V,P)   (V)V&=~(P)
class TMdbStorageCopy;

    class TMdbTSFileHead
    {
    public:
        void Clear();//清理
        std::string ToString();//输出
    public:
        char m_sTSName[MAX_NAME_LEN];
		int iVarcharID;
		int m_iStartPage; //该文件的起始页号
		int m_iEndPage;  //该文件的结束页号
        int m_iPageSize;//页大小
        int m_iPageCount; //页个数
        MDB_INT64  m_iPageOffset;//数据页地址
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
		int LinkFile(const char * sFile);//读取
		int LinkMdb(const char * sDsn,TMdbVarchar *pVarchar);//链接mdb
		int FlushFull(int iStartPageId = 1);
		int FlushHead();
		int StartToReadPage();//开始读取页数据
		int GetNextPage(TMdbPage * &pMdbPage);//获取下一个页数据
		int FlushDirtyPage();//刷出脏页
		int GetVarCharLen(int iPos,int& iVarCharLen);
	public:
		TMdbVarchar *m_pVarchar;
		TMdbTSFileHead m_VarcharHead;
		FILE * m_pVarcharFile;//varchar文件句柄
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
        int LinkFile(const char * sFile);//读取
        int LinkMdb(const char * sDsn,TMdbTableSpace *pMdbTS);//链接mdb
        int FlushFull(int iStartPageId = 1);//刷出表空间
        int StartToReadPage();//开始读取页数据
        int GetNextPage(TMdbPage * &pMdbPage);//获取下一个页数据
        int FlushDirtyPage(int iDirtyPageID);//刷出脏页
        int FlushHead();
    public:
	 FILE * m_pTSFile;//表空间文件	
        TMdbTSFileHead m_tTSFileHead;//文件头  
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
		int Load(TMdbPage * pPage);                                //加载入口
		int SetStructChangeType();                        //设置表空间及表结构变化类型
		int CopyByPage(TMdbPage * pPage);                 //页拷贝,文件页号与内存页号不对应，加载后需要重新落地文件
		int CopyByPageOrder(TMdbPage * pPage);            //页拷贝并且文件页号与内存页号对应，加载后无需重新落地文件
		int CopyByRecord(TMdbPage * pPage);                 //记录拷贝，文件页号与内存页号不对应，加载后需要重新落地文件
		int ColumnCopyByDataTypeChange(TMdbColumn * pNewColumn,TMdbColumn * pOldColumn,char* pMemAddr);          //类型发生变化后的字段拷贝
		int ColumnCopyByDateTimeChange(TMdbColumn * pNewColumn,TMdbColumn * pOldColumn,char* pMemAddr);  
		int GetNextCopySize(int iSize);                               //如果表结构发生变化，需要分段进行记录拷贝，获取分段拷贝大小，>0 表示需要拷贝的长度，<=0表示拷贝完成
		int FillData(char* pMemAddr);
	private:
		TMdbConfig * m_pConfig;
		TMdbShmDSN* m_pShmDSN;
		TMdbTable* m_pTable;                     //表指针
		TMdbTable* m_pOldTable;
		TMdbTableSpace *m_pTMdbTableSpace;     //表空间指针 
		TMdbTableSpaceCtrl* m_pTableSpaceCtrl; //表空间管理器指针
		TMdbPageCtrl*   m_PageCtrl;             //页面管理器
		char * m_pInsertBlock;                    //插入块
		TMdbVarCharCtrl* m_pVarcharCtrl;
		int m_iStructChangeType;              //表结构变化类型
		TMdbRowID m_rowID;                        //没有实际作用，迎合接口
		TMdbRowCtrl m_OldRowCtrl;
		TMdbRowCtrl m_NewRowCtrl;
		TMdbColumnNullFlag * m_NewColNullFlag; //新结构的null标志位
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
	    std::vector<StorageFile> m_vTSFile;//所有的表空间file
	    std::vector<StorageFile> m_vVarCharFile;//所有Varchar空间文件
	    std::vector<StorageFile> m_vTSChangeFile;//所有表空间的临时脏页文件
		std::vector<StorageFile> m_vVarCharChangeFile;//所有Varchar空间的临时脏页文件
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
        int GetNextPage(TMdbPage * &pMdbPage);//获取下一个页数据
    private:
        TMdbShmDSN * m_pShmDSN;
        TMdbFileList m_tFileList;
        TMdbTableSpaceCtrl m_TSCtrl;
        TMdbPageCtrl m_tPageCtrl;
        TMdbStorageCopy StorageCopyCtrl;
        TMdbVarCharCtrl m_tVarCharCtrl;
        TMdbFileBuff m_tFileBuff;
        TMdbTSFileHead m_tTSFileHead;//文件头  
        std::vector<StorageFile> m_vTSFile;//所有的表空间file
        std::vector<StorageFile> m_vVarCharFile;//所有Varchar空间文件
    };
    //mdb存储引擎
    class TMdbStorageEngine
    {
    public:
        TMdbStorageEngine();
        ~TMdbStorageEngine();
        void Clear();//清理
        int Attach(const char * sDsn);//链接共享内存
        int BackupFile();
		int RemoveBakFile();
        int FlushFull();//刷出所有数据，作为起始点
        int RemoveNormalFile(); //删除非varchar文件
        int RemoveVarcharFile();//删除varchar文件
        int RemoveChangeFile();//删除change文件
        //int FlushVarchar(); //刷出varchar页
        //int LoadDataFromDisk();//从磁盘上载数据
        //int LoadVarcharDataFromDisk(); //加载varchar数据
        //int LoadDataFromRedoLog();//从redoLog上载数据
    private:
        //int LinkDiskToMdb();//链接磁盘，读取磁盘数据
        //int ClearRedoLog(int iChkLSN);//清理过期的redo日志
    private:
	std::vector<TMdbTSFile*> m_vTSFile;//所有的表空间file
	std::vector<TMdbVarcharFile*> m_vVarCharFile;//所有Varchar空间文件
	TMdbShmDSN * m_pShmDSN;
	TMdbFileList m_tFileList;
	TMdbConfig * m_pConfig;
    };

    //redo日志解析
    class TMdbRedoLogParser
    {
    public:
        TMdbRedoLogParser();
        ~TMdbRedoLogParser();
        int ParseFile(const char * sFullPathFile);//解析文件
        int NextRecord(char* &sRecordBuff,int & iLen);//获取下一条记录
        int Analyse(const char* sRecord,TMdbLCR& tMdbLcr);//解析一条记录
        MDB_INT64 GetPageLSN(char * sRecordBuff);
		long long GetFileSize(const char* pFile); //获取文件大小
		int GetNum(const  char* pNum,int iNumSize);
		int GetTableName(char* sRecord,std::string& sTableName);
		int GetWhereValue(char * sRecord); 
		char* GetSQL(TMdbLCR& tMdbLcr);//根据解析结果拼写sql
		int GetRoutingID(char * sRecord);
        char GetVersion(char * sRecord);
        int GetSyncFlag(char *sRecord);//获取同步标识
	public:
		TMdbNtcSplit m_tSplitWhere; //解析是否有where
		TMdbNtcSplit m_tSplitCol;   //解析各列
    private:
        FILE * m_pCurFile;
		char* m_pFileBuff;
		long long m_iBuffSize;
		long long m_iCurPos;
		TLCRColm tLcrColumn;     //列信息
		int m_iFactor[10]; // 解析数值型用
		char m_sRout[10];//解析路由id使用
        char m_sSyncFlag[10];//解析同步标识使用
    };
//}
#endif

