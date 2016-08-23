#ifndef _MDB_BULK_CTRL_H_
#define _MDB_BULK_CTRL_H_

#include <string>
#include <vector>
#include <map>
#include <ctype.h>
#include "Interface/mdbQuery.h"
#include "Helper/mdbStruct.h"
//#include "Helper/mdbFileList.h"
//#include "Helper/mdbFileOper.h"
//#include "Helper/TOraDBQuery.h"
#include "Helper/TDBFactory.h"
#include "Helper/mdbDictionary.h"

//namespace QuickMDB{


class TMdbConnectCtrl
{
public:
    TMdbConnectCtrl();
    ~TMdbConnectCtrl();

    int CreateMdbQuery(const char* psDsnName);
    int CreateOraQuery(const char* psDsnName);
    int Destroy();
public:    
    TMdbDatabase*       m_pDBLink;
    TMdbQuery*          m_pQuery;
    TMDBDBInterface*       m_pOraDBLink;
    TMDBDBQueryInterface*  m_pOraQuery;
};

//#if 0

class TWriterCtrl
{
public:
	TWriterCtrl();
	~TWriterCtrl();
    
	int Init(const char* sFileName);
	void Clear();
	int Write(char* sData,int iLen);
	int Write();
	
private:
    char    m_sFileName[MAX_PATH_NAME_LEN];
    char    m_sBuf[MAX_SEND_BUF];
    FILE*   m_pFile;
    int     m_iBufPos;
    MDB_INT64   m_iFileSize;	
};

//导出
class TMdbBuckOutData
{
public:
	TMdbBuckOutData();
	~TMdbBuckOutData();
	
	int Init(TMdbShmDSN * pShmDSN,int iExptType,const char*pDumpFile,
            const char *sTableName,const char* sTsFormat,const char* sFilterToken, const char* sStrQuote,const char* pSQLFile=NULL);
	int ExportData();
	bool FilterTable(TMdbTable* pTable);
	int ExportDataForAllTable();

	int FormateDate(char* sMdbDate,char* sFormatDate );
private:
	int WriteTextFileFromMdb(TMdbTable *pMdbTable);
    int WriteTextFileFromOra(TMdbTable *pMdbTable);
	int GetMdbSQL(TMdbTable *pMdbTable);
	int GetOraSQL(const char* pOraSQLFile);
	bool IsBlobTable(TMdbTable *pMdbTable);
public:
	TWriterCtrl     m_tWriter;
	TMdbConnectCtrl m_tConnCtrl;
	TMdbTable*      m_pMdbTable;
	char            m_sSelectSQL[MAX_SQL_LEN*4];//sql
    int             m_iDataSrc;//数据源0--mdb，1--ora
    char            m_sTsFormat[MAX_NAME_LEN];
	char			m_sFilterToken[2];
	char			m_sStrQuote[2];
	bool			m_bAllTable;
	char			m_sFileName[MAX_PATH_FILE];
	TMdbShmDSN*     m_pShmDSN;
};

//导入
class TMdbBuckInData
{
public:
	TMdbBuckInData();
	~TMdbBuckInData();

	int Init(TMdbShmDSN * pShmDSN,const char*pDumpFile,const char *sTableName,const char* sTsFormat,const char* sFilterToken, const char* sStrQuote);
	int ImportData();
private:
	int ImportTextFile();
	int GetInsertSQL();
	bool IsBlobTable(TMdbTable *pMdbTable);
    int ParseData(const char* pData);
    int ParseDataPlus(const char* pData);    //字符串中带逗号的处理
    int ParseDateStamp(char* pBegin,int iColumnIndex );
private:
	TMdbConnectCtrl     m_tConnCtrl;
	TMdbTable*          m_pMdbTable;
	FILE*               m_pFile;
	char                m_sInsertSQL[MAX_SQL_LEN];
	char            	m_sTsFormat[MAX_NAME_LEN];
    char				m_sFilterToken[2];
	char				m_sStrQuote[2];
};

class TMdbBulkCtrl
{
public:
	TMdbBulkCtrl();
	~TMdbBulkCtrl();
	
	int Init(const char*pDsn);
	int ExportData(int iExptType,const char*pDumpFile,const char *sTableName,const char* sTsFormat,const char* sFilterToken, const char* sStrQuote,const char* pOraSQLFile=NULL);
	int ImportData(const char*pDumpFile,const char *sTableName,const char* sTsFormat,const char* sFilterToken, const char* sStrQuote);
private:
	TMdbDSN*        m_pDsn;  
	TMdbShmDSN*     m_pShmDSN;
	TMdbBuckOutData m_tExportData;
	TMdbBuckInData  m_tImportData;
};
//}
#endif

