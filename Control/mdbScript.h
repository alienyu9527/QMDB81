/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbScript.h
*@Description�� ����DSN����ṹ�������Զ�Ӧ��XML�ļ�����
*@Author:       cao.peng
*@Date��        2013��03��20��
*@History:
******************************************************************************************/
#ifndef _MDB_SCRIPT_H_
#define _MDB_SCRIPT_H_

#include "Helper/TThreadLog.h"
#include "Helper/mdbSQLParser.h"
#include "Helper/SyntaxTreeAnalyse.h"
#include "Helper/mdbConfig.h"
#include "Helper/mdbXML.h"

//namespace QuickMDB{

#define MAX_CMD_LEN 4096
#define MAX_LINE_LEN 2048

#define ADDSYSINFO(pSys,name,value)\
{\
    if(NULL == name)\
    {\
        TADD_ERROR(ERR_APP_INVALID_PARAM,"System parameter name is empty.");\
    }\
    else\
    {\
        MDBXMLElement * pSysAttr = new MDBXMLElement("section");\
        pSys->LinkEndChild(pSysAttr);\
        pSysAttr->SetAttribute("name", name);\
        pSysAttr->SetAttribute("value",value);\
    }\
}

#define ADDSYSINFO_YN(pSys,name,value)\
{\
    if(NULL == name)\
    {\
        TADD_ERROR(ERR_APP_INVALID_PARAM,"System parameter name is empty.");\
    }\
    else\
    {\
        MDBXMLElement * pSysAttr = new MDBXMLElement("section");\
        pSys->LinkEndChild(pSysAttr);\
        pSysAttr->SetAttribute("name", name);\
        if(value)\
        	pSysAttr->SetAttribute("value","Y");\
       	else\
			pSysAttr->SetAttribute("value","N");\
    }\
}

    class TTableScript
    {
    public:
        TTableScript();
        ~TTableScript();
        void Clear();

    public:
        char m_sTableFileName[MAX_PATH_NAME_LEN];
        char m_sTabAlterFile[MAX_FILE_NAME];
        char sTableName[MAX_NAME_LEN];
        char m_sTabFilePath[MAX_PATH_NAME_LEN];
        bool m_bIsGenTable[2];
        TMdbTable *m_pTable;
        MDBXMLElement  *m_pTableRoot;
        MDBXMLDocument *m_pTableDocument;
        MDBXMLElement  *m_pTabAlterRoot;
        MDBXMLDocument *m_pTabAlterDoc;
    };

class TJobScript
{
public:
	TJobScript();
	~TJobScript();
	void Clear();
	int Init();
public:
	char m_sJobFileName[MAX_NAME_LEN];//job�ļ�����
	MDBXMLDocument * m_pJobDocument;//��job�����ļ�ʹ��
};

class TDsnScript
{
public:
    TDsnScript();
    ~TDsnScript();

    void Clear();
    int Init(const char *pszDSN);
    TTableScript* GetTableByTableName(const char*pTableName);
    int AddTable(const char*pTableName,TTableScript *&pTableScript);
public:
    char m_sDsnName[MAX_NAME_LEN];
    char m_sSysFileName[MAX_PATH_NAME_LEN];
    char m_sSysFileNameBAK[MAX_PATH_NAME_LEN];//BAK
    bool m_bIsGenDSN[4];
    TMdbCfgDSN *m_pCfgDSN;
    TMdbTableSpace *m_pTablespace[MAX_TABLE_COUNTS];
    TMDbUser *m_pUser[MAX_DSN_COUNTS];
    MDBXMLElement *m_pSysRoot;
    MDBXMLDocument *m_pSysDocument;
    MDBXMLElement *m_pSysRootBAK;//BAK file
    MDBXMLDocument *m_pSysDocumentBAK;//BAK file
    
    TTableScript *m_tTableMgr[MAX_TABLE_COUNTS];
    TJobScript m_tJob;


    // ��ռ�����¼�����ļ�ʹ��
    char m_sTsAlterFile[MAX_PATH_NAME_LEN];
    MDBXMLElement *m_pTsAlterRoot;
    MDBXMLDocument *m_pTsAlterDoc; 
};

class TMdbScript
{
public:
	TMdbScript();
	~TMdbScript();

	int Init(TMdbSqlParser *pSqlParse);
	int InitDsn(const char *pDSN,bool bUseFlag = false);
	int InitTableInfo(const char *pDSN,TMdbTable *pTable);
	int Execute(const char *pszDDLSql);
	int ProcessDsnRep(const int iSqlType1,const int iSqlType2);
	int ProcessTableRep(const int iSqlType1,const int iSqlType2);
	int ExecuteDropOpt(const int iSqlType);//ִ��drop����
	int BakFile(const char* pFilePathName);
	bool IsGenDsnXml();
	int CreateTableSpace(TMdbTableSpace* pTableSpace,bool bIsDynamic);
	int ModifyTableSpace(TMdbTableSpace* pTableSpace,bool bIsDynamic);
	int DropTableSpace(const char *pTablespaceName,bool bIsDynamic);
	int CreateUser(TMDbUser* pUser,bool bIsDynamic);
	int ModifyUser(TMDbUser* pUser,bool bIsDynamic);
	int DropUser(TMDbUser* pUser,bool bIsDynamic);
	int GenConnectInfo(TMdbCfgDSN  *pDsn);
	int AddSysInfo(MDBXMLElement *pESys,TMdbCfgDSN  *pDsn);
	int CreateDsn(TMdbCfgDSN  *pDsn);
	int ModifyDsn(bool bIsDynamic);
	int DropDsn(const char *pDsn);
	int CreateTable(TMdbTable* pTable,bool bIsDynamic);
	int DropTable(const char *pTableName);
	int AddIndex(bool bIsDynamic);
	int DropIndex(bool bIsDynamic);
	int AddPrimaryKey(TMdbTable* pTable);
	int AddColumn(TMdbTable* pTable,bool bIsDynamic, bool bWriteUpdate = true);
	int ModifyColumn(bool bIsDynamic);
	int DropColumn(bool bIsDynamic);
	int AddSingleTableAttribute(TMdbTable* pTable);
	int ModifyTableAttribute(bool bIsDynamic);
	int DropTableAttribute(TMdbTable* pTable);
	int GetDataType(int iDataType,char *pDataType,const int iLen);
	int GetDataRepType(int iRepType,char *pRepType,const int iLen);
	int AddSequenceToOracle();//����oracle��mdb_sequence���м�¼
	int DelSequenceToOracle();//ɾ��oracle��sequence����ָ������
	int ConnectOracle();//����oracle
	int InitJobInfo(const char *pDSN);//��ʼ��job�����ļ�·��������·��
	int CreateJob(TMdbJob *pMdbJob);//����job(��QuickMDB_JOB.xml�ļ�������job)
	int AlterJob(TMdbJob *pMdbJob);//�޸�job(��QuickMDB_JOB.xml�ļ����޸�job)
	int RemoveJob(TMdbJob *pMdbJob);//ɾ��job
	int AddFlushOrLoadSQLParam(TMdbTable* pTable,bool bIsDynamic);//����flush-sql or load-sql����
	int DropFlushOrLoadSQLParam(TMdbTable* pTable,const char *pParamName,bool bIsDynamic);//ɾ��flush-sql or load-sql����
	int ModifyFlushOrLoadSQLParam(bool bIsDynamic);//�޸�flush-sql or load-sql����
    int CheckSQLValidity(const char* pSQL);//������sql��ˢ��sql����Ч��
	void RemoveTabDir();//ɾ��tabĿ¼
    bool CheckXmlTableSpaceExist(const char* sTablespaceName);//����ռ��Ƿ����
	void SetTableSpace(const char* sTablespaceName);//���ñ�ռ�����
	int RenameTable(const char* sOldTableName,const char* sNewTableName,bool bIsDynamic); //rename table
    int SaveSysCfgFile(int &iDsnPos);//����ϵͳ�����ļ����޸�
	
protected:
    int CheckAndOperSequence(TMemSeq &tMemSeq,bool &bExist,bool bDel=false);//����ģʽ ��������Ƿ����
	int FindDsnEx(const char *pszDSN);
	int SetDsnEx(const char *pszDSN);
	int GeColumPosByName(MDBXMLElement* pTableRoot,const char *pColumnName);
	int GetSysRootNode(bool bIsDynamic);
	int GetTableRootNode(const char *pTableName,TTableScript * &pTableScript,bool bIsDynamic);
	int LoadDSNCofig();//����DSNϵͳ�����ļ�
	int NewJobNode(TMdbJob *pMdbJob,MDBXMLElement *pJobRoot);//����job�ڵ�
	int DeleteSpecifiedIndexColumn(MDBXMLElement* pTableEle,const int iColumnPos);//ɾ��ָ��������
	int ReplaceSpecialCharactersForSQL(const char *pszFullFileName);//�滻flush-sql or load-sql�����ַ�

    // ��¼������ռ�ı䶯
    int WriteTsAddInfo(TDsnScript* pDsnScript,const char* psTsName);

    // ��¼��ռ�page-size�䶯
    int WriteTsPageSizeAlterInfo(TDsnScript* pDsnScript, const char* psTsName,size_t iOldValue, size_t iNewValue);

    // ��¼ɾ����ռ�ı䶯
    int WriteTsDelInfo(TDsnScript* pDsnScript,const char* psTsName);

    // ��¼��������
    int WriteAddColumn(TTableScript* psTabScript, const char* psColmName, int iColumCount);

	// ��¼��ɾ����
	int WriteDropColumn(TTableScript* psTabScript, const char* psColmName, int iColumCount);

    // ��¼��data-type �䶯
    int WriteColmDataTypeAlterInfo(TTableScript* psTabScript, const char* psColmName, int iOldValue, int iNewValue);

    // ��¼��data-len�䶯
    int WriteColmDataLenAlterInfo(TTableScript* psTabScript, const char* psColmName, int iOldValue, int iNewValue);

    // ��¼��is-zip-time�䶯
    int WriteTabZipTimeAlterInfo(TTableScript* psTabScript, const char* psOldValue,const  char* psNewValue);

    // ��¼��table-space�䶯
    int WriteTabTsAlterInfo(TTableScript* psTabScript,const char* psOldValue,const char* psNewValue);
private:
    int ChangeSequenceFile(const char* sSeqFileName,int iPageCount,char *&pBuffer);//����ģʽ ɾ������
    //�¼ӵ��ļ�β��
    int AddToSequenceFile(const char* sSeqFileName,TMemSeq &tMemSeq);
    void GetTableSpaceFromTableFile(const char *sTabName,const char *sTabFileName, char *sTabspaceName);//��ȡ��ռ������
    
    
private:
	char m_sLastDSN[MAX_NAME_LEN];
	char m_sTablespaceName[MAX_NAME_LEN];
	char m_sSysFilePath[MAX_PATH_NAME_LEN];
	char m_sSysFileBakPath[MAX_PATH_NAME_LEN];
	char m_sTabFilePath[MAX_PATH_NAME_LEN];
    
	TDsnScript m_tDsnMgr[MAX_DSN_COUNTS];
	TMdbSqlParser *m_pMdbSqlParser;
	TMDBDBInterface *m_pDBLink; //Oracle����
	TMdbConfig m_tMdbConfig;  //����DSN�����ļ��û���ȡoracle������Ϣ
};

//}
#endif
