////////////////////////////////////////////////
// Name: mdbFileParser.h
// Author: Li.ShuGang
// Date: 2009/03/27
// Description: ��ȡminidb���ɵ���־�ļ�,�����н���
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

    //�ڴ����ݿ��ĳһ���е�ĳһ������
    class TMdbData
    {
    public:
        TMdbData();
        ~TMdbData();

        void Clear();

        char sName[MAX_NAME_LEN];   //������
        char sPName[MAX_NAME_LEN];  //��������
        int  iType;                 //�������� DT_Int DT_Char
        char sValue[MAX_BLOB_LEN]; //����ֵ
        char cOperType;             //'+', '-', '='
        int iLen;
        int isNull; //-1��ʾNULL��0��ʾ��NULL
    };




    //һ���ļ���¼�������������ݽ��
    /*class TMdbOneRecord
    {
    public:
        TMdbOneRecord();
        ~TMdbOneRecord();
        
        int Parse(unsigned char* pszRecord, int iLen);
        int Init();
            
        char m_sSQL[MAX_SQL_LEN]; //����֮���SQL
        int m_iDataCounts;        //��������
        int m_iWhereCounts;       //��������
        int m_iTableID;           //��ID
        int m_iType;              //��������(Insert/Update/Delete)
        TMdbData m_tData[MAX_COLUMN_COUNTS];         //Set(Insert)����������
        // TMdbWhereClause m_tWhere[MAX_COLUMN_COUNTS];  //Where����������
        TMdbConfig *m_pConfig;  //�����ļ�
        char* m_pszRecord;
        TFlushData m_tFlushData;
    	bool m_bIsBlobTable;
        int DelTableQuery(const int iTableId);
    private:
        //int GenSQL();//����sql
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


    //����һ���ļ�,��ֳ�һ�����ļ�¼�������д����¼�ͼ�¼���� 
    /*class TMdbFileParser
    {
    public:
        TMdbFileParser();
        ~TMdbFileParser();
        
    public:
        //���ö�ȡ��Ŀ¼
        int Init(const char* pszFullFileName, const char* pszDSN);
        
        //������һ����¼
        TMdbOneRecord* Next(); 

        //��������¼д������ļ�
        bool RecordError(const char* pszLogPath,const char* pszErrorLogPath);
        int ClearSQLCacheByTableId(const int iTableId);//����sql����
        
    private:
        //��ȡ�ļ���С
        long GetFileSize(const char* pszFullFileName);

    private:
        unsigned char *m_pMemBuffer;   //һ���ļ��Ļ���
        long m_iMemSize;               //�����С
        FILE* m_fp;                    //�ļ����
        FILE* m_errfp;              //д�����ļ����ļ����
        long m_iPos;                   //��ǰ��ȡ��λ��
        long m_iOldPos;             //��һ�ζ�ȡ��λ��
        int   m_iLen;                  //��һ�δ����¼�ĳ���
        long m_iFileSize;              //��ǰ�ļ���С
        TMdbConfig *m_pConfig;  //�����ļ�
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
    	//int m_iInsertCounts[MAX_TABLE_COUNTS]; //insert��¼��
    	//int m_iDeleteCounts[MAX_TABLE_COUNTS]; //delete��¼��
    	//int m_iUpdateCounts[MAX_TABLE_COUNTS]; //update��¼��	
    };

//}
#endif //__MINI_DATABASE_FILE_PARSER_H__


