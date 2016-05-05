#ifndef _MDB_DATA_PUMP_CTRL_H_
#define _MDB_DATA_PUMP_CTRL_H_

#include <string>
#include <vector>
#include <map>
#include <ctype.h>
#include "Interface/mdbQuery.h"
#include "Tools/mdbSQL.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbFileList.h"
#include "Dbflush/mdbFileParser.h"

using namespace std;

//namespace QuickMDB{
        
    enum TFileFormat
    {
        FORMAT_UNKOWN = 0, // 未知
        FORMAT_SQL  = 1, // sql file 
        FORMAT_TEXT = 2 // text file
    };

    class TMdbConnCtrl
    {
    public:
        TMdbConnCtrl();
        ~TMdbConnCtrl();

        int Create(const char* psDsnName);
        int Destroy();
    public:    
        std::string m_sDsn;
        TMdbDatabase* m_pDBLink;
        TMdbQuery* m_pQuery;
    };

    //sql file SQL结构
    struct ST_SQL_INFO
    {
    	int iSqlSeq;//sql 序号
    	std::string sSQL;	// SQL 语句
    	int iParamLineCounts;
    	std::vector<ST_PARAM> vSql2Param;
    };

    class TWriter
    {
    public:
    	TWriter();
    	~TWriter();
        
    	int Init(const char* sFileName);
    	void Clear();
    	int Write(char* sData);
    	int Write();
    	
    private:
        char m_sFileName[MAX_PATH_NAME_LEN];
        char m_sBuf[MAX_SEND_BUF];
        FILE* m_pFile;
        int m_iBufPos;
        MDB_INT64 m_iFileSize;	
    };

    class TMdbExportData
    {
    public:
    	TMdbExportData();
    	~TMdbExportData();
    	
    	int Init(TMdbShmDSN *pShmDSN,int iFileFormat,const char*pDumpFile);
    	int ExportByTableName(const char*pTableName);
    	int ExportBySQL(const char*pSQL);
    	void PrintDumpFileInfo();
    private:
    	int WriteSQLFile(TMdbTable *pMdbTable);
    	int WriteTextFile(TMdbTable *pMdbTable);
    	int GetQuerySQL(TMdbTable *pMdbTable);
    	int GetInsertColumnSQL(TMdbTable *pMdbTable,bool bIsValue);
    	bool IsBlobTable(TMdbTable *pMdbTable);
    	int GetTableNameBySQL(const char * pSQL,char *pTableName,const int iLen);
    public:
    	TWriter m_tWriter;
    	TMdbConnCtrl m_tConnCtrl;
    	TMdbShmDSN *m_pShmDSN;
    	char m_sDumpFile[MAX_PATH_NAME_LEN];
    	char m_sInsertSQL[MAX_SQL_LEN];
    	char m_sSelectSQL[MAX_SQL_LEN];
    	int m_iTableCounts;
    	int m_iFileFormat;
    	std::map<string,int> m_mTableName2Record;
    };

    class TMdbImportData
    {
    public:
    	TMdbImportData();
    	~TMdbImportData();

    	int Init(TMdbShmDSN * pShmDSN,const char*pDumpFile);
    	int ImportData();
    	void PrintDumpFileInfo();
    private:
    	int ImportSqlFile();
    	int ImportTextFile();
    	int ExecuteStaticSQL(const char *pSQL);
    	int ParseRecord(char* pData,int &iSeqSQL);
    	int ExecuteDynamicStatic(int iSeqSQL);
    	void GetNullColumnPos(const char * pData,bool &bIsExistNull);
    	int GetInsertSQL(const char* pTableName);
    private:
    	TMdbConnCtrl m_tConnCtrl;
    	TMdbShmDSN *m_pShmDSN;
    	int m_iFileFormat;
    	FILE* m_pFile;
    	char m_sInsertSQL[MAX_SQL_LEN];
    	TMdbData m_tData[MAX_COLUMN_COUNTS];//Set的数据类型
    	TMdbTable * m_pMdbTable;
    	int m_iTotalRecord;
    	std::map<int,ST_SQL_INFO> m_mSeq2SQL;
    	std::map<int,TMdbConnCtrl*> m_mSeq2ConCtrl;
    };

    class TMdbDataPumpCtrl
    {
    public:
    	TMdbDataPumpCtrl();
    	~TMdbDataPumpCtrl();
    	
    	int Init(const char*pDsn);
    	int ExportData(const char*pTableName,char cFileFormat,const char*pDumpFile,const char*pSQL);
    	int ImportData(const char*pDumpFile);
    private:
    	TMdbDSN    *m_pDsn;  
    	TMdbShmDSN *m_pShmDSN;
    	TMdbExportData m_tExportData;
    	TMdbImportData m_tImportData;
    };
//}
#endif

