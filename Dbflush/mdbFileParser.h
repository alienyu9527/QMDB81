////////////////////////////////////////////////
// Name: mdbFileParser.h
// Author: Li.ShuGang
// Date: 2009/03/27
// Description: 读取minidb生成的日志文件,并进行解析
////////////////////////////////////////////////
/*
* $History: mdbFileParser.h $
 * 
 * *****************  Version 1.0  ***************** 
*/
#ifndef __MINI_DATABASE_FILE_PARSER_H__
#define __MINI_DATABASE_FILE_PARSER_H__

#include "Helper/mdbConfig.h"
#include "Control/mdbFlush.h"
#include "Interface/mdbQuery.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{

    #define MAX_COMMIT_COUNTS 1000

    //内存数据库的某一个列的某一个数据
    class TMdbData
    {
    public:
        TMdbData();
        ~TMdbData();

        void Clear();

        char sName[MAX_NAME_LEN];   //列名称
        char sPName[MAX_NAME_LEN];  //参数名称
        int  iType;                 //数据类型 DT_Int DT_Char
        char sValue[MAX_BLOB_LEN]; //数据值
        char cOperType;             //'+', '-', '='
        int iLen;
        int isNull; //-1表示NULL，0表示非NULL
    };




    //一个文件记录解析出来的数据结果
    /*class TMdbOneRecord
    {
    public:
        TMdbOneRecord();
        ~TMdbOneRecord();
        
        int Parse(unsigned char* pszRecord, int iLen);
        int Init();
            
        char m_sSQL[MAX_SQL_LEN]; //解析之后的SQL
        int m_iDataCounts;        //数据列数
        int m_iWhereCounts;       //条件列数
        int m_iTableID;           //表ID
        int m_iType;              //操作类型(Insert/Update/Delete)
        TMdbData m_tData[MAX_COLUMN_COUNTS];         //Set(Insert)的数据类型
        // TMdbWhereClause m_tWhere[MAX_COLUMN_COUNTS];  //Where的数据类型
        TMdbConfig *m_pConfig;  //配置文件
        char* m_pszRecord;
        TFlushData m_tFlushData;
    	bool m_bIsBlobTable;
        int DelTableQuery(const int iTableId);
    private:
        //int GenSQL();//生成sql
        int PushData(int iPos,ST_COLUMN_VALUE & stColumnValue,bool bIsExit=false);
        int CreateDBLink();
        int InitAllQuery();
        int AddTableQueryDynamic(const int iTableId);
        TMdbTable* m_pTable;
        TMdbDatabase m_tMinidb;
        TMdbQuery *m_pQuery[MAX_TABLE_COUNTS];
        TMdbShmDSN *m_pShmDSN;
        bool m_bIsAttach;
    };*/


    //处理一个文件,拆分出一条条的记录，并进行错误记录和记录规整 
    /*class TMdbFileParser
    {
    public:
        TMdbFileParser();
        ~TMdbFileParser();
        
    public:
        //设置读取的目录
        int Init(const char* pszFullFileName, const char* pszDSN);
        
        //处理下一个记录
        TMdbOneRecord* Next(); 

        //将这条纪录写入错误文件
        bool RecordError(const char* pszLogPath,const char* pszErrorLogPath);
        int ClearSQLCacheByTableId(const int iTableId);//清理sql缓存
        
    private:
        //获取文件大小
        long GetFileSize(const char* pszFullFileName);

    private:
        unsigned char *m_pMemBuffer;   //一个文件的缓冲
        long m_iMemSize;               //缓冲大小
        FILE* m_fp;                    //文件句柄
        FILE* m_errfp;              //写错误文件的文件句柄
        long m_iPos;                   //当前读取的位置
        long m_iOldPos;             //上一次读取的位置
        int   m_iLen;                  //上一次处理纪录的长度
        long m_iFileSize;              //当前文件大小
        TMdbConfig *m_pConfig;  //配置文件
        TMdbOneRecord *m_pOneRecord;
        char m_sFullFileName[MAX_PATH_NAME_LEN];
    };*/

	struct ST_REP_COUNT
	{
		std::string m_sTableName;
		int m_iInsertCounts;
		int m_iDeleteCounts;
		int m_iUpdateCounts;
	};

    class TMdbOraRepFileStat
    {
    public:
    	TMdbOraRepFileStat();
    	~TMdbOraRepFileStat();
    	
    	void Clear();
    	void Stat(TMdbLCR& tLCR);
    	void PrintStatInfo();
    public:
		ST_REP_COUNT m_RepCount[MAX_TABLE_COUNTS];
    	//std::string m_sTableName[MAX_TABLE_COUNTS];
    	//int m_iInsertCounts[MAX_TABLE_COUNTS]; //insert记录数
    	//int m_iDeleteCounts[MAX_TABLE_COUNTS]; //delete记录数
    	//int m_iUpdateCounts[MAX_TABLE_COUNTS]; //update记录数	
    };

//}
#endif //__MINI_DATABASE_FILE_PARSER_H__


