#ifndef _MDB_OVERDUE_DATA_CTRL_H_
#define _MDB_OVERDUE_DATA_CTRL_H_

#include <string>
#include <vector>
#include "Interface/mdbQuery.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbFileList.h"
#include "Helper/mdbDictionary.h"


using namespace std;

//namespace QuickMDB{
        

    class TTableCleanInfo
    {
    public:
        TTableCleanInfo(){}
        ~TTableCleanInfo(){}

    public:
        std::string m_sDsn;
        std::string m_sTableName;
        std::string m_sSQL;
    };

    class TCleanFileParser
    {
    public:
        TCleanFileParser(){}
        ~TCleanFileParser(){}

        int Parse(char* sFullFileName);

    private:
        int ParseLine(char* sLineText);

    public:
        std::vector<TTableCleanInfo> m_vCleanTab;
    };


    class TMdbConnSet
    {
    public:
        TMdbConnSet();
        ~TMdbConnSet();

        int Create(const char* psDsnName);
        int Destroy();
        TMdbQuery* GetQuery();

    public:    
        std::string m_sDsn;
        TMdbDatabase* m_pDBLink;
        TMdbQuery* m_pQuery;
    };

    class TColmItem
    {
    public:
        TColmItem();
         ~TColmItem();
         
         void Clear();

    public:
        bool m_bIsTime;
        bool m_bNull; // 是否为空
        char m_sName[MAX_NAME_LEN]; // 列名
        char m_sValue[MAX_BLOB_LEN]; // 列值
    };

    class TRecordParser
    {
    public:
        TRecordParser();
        ~TRecordParser();

        void Clear();
        int Parse(char* pszRecord);
        int GetColm(char* psData, int& iColmLen);
        int GetColumnValue(char* pValuePair,TColmItem & stColumnValue,int &iValuePairLen);
        const char* GetRecordStr();
        void Print();

    public:
        std::vector<TColmItem> m_vColmSet;
        int m_iRecdLen;
        std::string m_sRecd;
    };

    class TRecordWriter
    {
    public:
    	TRecordWriter();
    	~TRecordWriter();
        
    	int Init(const char* sFilePath, char* sFileName);
    	void Clear();
    	int Write(char* sData);
    	int Write();
    	
    private:
        char m_sFileName[MAX_PATH_NAME_LEN];
        char m_sBuf[MAX_SEND_BUF];
        FILE* m_fp;
        int m_iBufPos;
        MDB_INT64 m_iFileSize;	
    };

    enum TDataOperType
    {
        OPER_UNKOWN = 0, // 未知
        OPER_CLEAN = 1, // 清理
        OPER_RESTORE = 2 //  恢复
    };

    // 处理一个表的备份数据文件
    class TFileProcesser
    {
    public:
        TFileProcesser();
        ~TFileProcesser();

        int Init(const char* psFileName, const char* psDsnName,const char* psTabName,TMdbQuery* pMdbQry, TDataOperType tOperType);
        void Clear();

        int Excute();

    private:
        
        int ProcBuff();
        void SaveIncompleteData();
        
        int ExcuteClean( TRecordParser* pRecd);
        int ExcuteRestore( TRecordParser* pRecd);

        int GenCleanSQL();
        int GenRestoreSQL( TRecordParser* pRecd);

        int GetPk();
        bool IsPK(const TColmItem& tColm);

        int GetColmIsTime(TRecordParser* pRecd);


    private:
        
        TDataOperType m_eOperType;
        
        FILE* m_fp;                    //文件句柄
        TMdbQuery* m_pMdbQry;
        TRecordParser* m_pRecParser;

        std::string m_sDsnName;
        std::string m_sTabName;
        std::string m_sFileName;

        char* m_psFileBuff; // 文件数据缓存
        MDB_INT64 m_iBuffPos;// 文件数据缓存的偏移量
        MDB_INT64 m_iBuffLen; // 文件数据缓存的大小
        MDB_INT64 m_iBuffDataLen; // 文件数据缓存中数据的长度
        
        char m_sRecdBuff[MAX_SEND_BUF];

        MDB_INT64 m_iResidueLen; // 不完整数据的长度
        char m_sResidueBuff[MAX_SEND_BUF*2];

        MDB_INT64 m_iStatCnt;

        char m_sSQL[MAX_SQL_LEN];

        std::vector<TColmItem> m_vPkSet;

    };

    class TOverDueDataCtrl
    {
    public:
        TOverDueDataCtrl();
        ~TOverDueDataCtrl();

        int RunBak(const char* sPath, char* sFileName);
        int RunClean(const char* sPath, char* sTabName, char* sDsnName = NULL);
        int RunRestore(const char* sPath, char* sTabName, char* sDsnName = NULL);

    private:
        int CheckCleanConfig();
        TMdbQuery* GetMdbQuery(const char* sDsnName);

         int GetTableInfoFromFileName(const char* psFileName, char* psDsnName, char* psTabName);

        // clean
        int CleanAllTable();
        int CleanOneTable(const char* psTableName, const char* psDsnName, const char* psFileName);

        // restore
        int RestoreAllTable();
        int RestoreOneTable(const char* psTableName, const char* psDsnName, const char* psFileName);
       
        
        // bak
        int BakTable(const TTableCleanInfo& tTabInfo);
        void SetData(int iFieldCount, int iBuffLen);
        int InitTabWriter(const TTableCleanInfo& tTabInfo);


    private:
        std::vector<TMdbConnSet*> m_vMdbConn;
        std::string m_sBakPath;
        TCleanFileParser* m_pConfigOper;
        bool m_bAllTab;

        TMdbQuery* m_pCurTabQry; // 当前表的qmdb查询句柄

        char m_sDataBuff[MAX_VALUE_LEN]; // 数据缓存

        TMdbFileList m_tFileList; // 目录下文件读取器

        TRecordWriter* m_pCurTabWriter; // 当前表的写落地文件
        TFileProcesser* m_pFilePrc;
        
    };

//}


#endif
