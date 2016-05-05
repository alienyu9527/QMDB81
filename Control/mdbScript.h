/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbScript.h
*@Description： 负责DSN、表结构、表属性对应的XML文件操作
*@Author:       cao.peng
*@Date：        2013年03月20日
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
	char m_sJobFileName[MAX_NAME_LEN];//job文件名称
	MDBXMLDocument * m_pJobDocument;//打开job配置文件使用
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


    // 表空间变更记录配置文件使用
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
	int ExecuteDropOpt(const int iSqlType);//执行drop操作
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
	int AddSequenceToOracle();//增加oracle侧mdb_sequence表中记录
	int DelSequenceToOracle();//删除oracle侧sequence表中指定序列
	int ConnectOracle();//连接oracle
	int InitJobInfo(const char *pDSN);//初始化job配置文件路径及备份路径
	int CreateJob(TMdbJob *pMdbJob);//创建job(在QuickMDB_JOB.xml文件中增加job)
	int AlterJob(TMdbJob *pMdbJob);//修改job(在QuickMDB_JOB.xml文件中修改job)
	int RemoveJob(TMdbJob *pMdbJob);//删除job
	int AddFlushOrLoadSQLParam(TMdbTable* pTable,bool bIsDynamic);//增加flush-sql or load-sql参数
	int DropFlushOrLoadSQLParam(TMdbTable* pTable,const char *pParamName,bool bIsDynamic);//删除flush-sql or load-sql参数
	int ModifyFlushOrLoadSQLParam(bool bIsDynamic);//修改flush-sql or load-sql参数
    int CheckSQLValidity(const char* pSQL);//检查加载sql、刷新sql的有效性
	void RemoveTabDir();//删除tab目录
    bool CheckXmlTableSpaceExist(const char* sTablespaceName);//检查表空间是否存在
	void SetTableSpace(const char* sTablespaceName);//设置表空间名字
	int RenameTable(const char* sOldTableName,const char* sNewTableName,bool bIsDynamic); //rename table
    int SaveSysCfgFile(int &iDsnPos);//保存系统配置文件的修改
	
protected:
    int CheckAndOperSequence(TMemSeq &tMemSeq,bool &bExist,bool bDel=false);//离线模式 检查序列是否存在
	int FindDsnEx(const char *pszDSN);
	int SetDsnEx(const char *pszDSN);
	int GeColumPosByName(MDBXMLElement* pTableRoot,const char *pColumnName);
	int GetSysRootNode(bool bIsDynamic);
	int GetTableRootNode(const char *pTableName,TTableScript * &pTableScript,bool bIsDynamic);
	int LoadDSNCofig();//上载DSN系统配置文件
	int NewJobNode(TMdbJob *pMdbJob,MDBXMLElement *pJobRoot);//生成job节点
	int DeleteSpecifiedIndexColumn(MDBXMLElement* pTableEle,const int iColumnPos);//删除指定索引列
	int ReplaceSpecialCharactersForSQL(const char *pszFullFileName);//替换flush-sql or load-sql特殊字符

    // 记录新增表空间的变动
    int WriteTsAddInfo(TDsnScript* pDsnScript,const char* psTsName);

    // 记录表空间page-size变动
    int WriteTsPageSizeAlterInfo(TDsnScript* pDsnScript, const char* psTsName,size_t iOldValue, size_t iNewValue);

    // 记录删除表空间的变动
    int WriteTsDelInfo(TDsnScript* pDsnScript,const char* psTsName);

    // 记录表增加列
    int WriteAddColumn(TTableScript* psTabScript, const char* psColmName, int iColumCount);

	// 记录表删除列
	int WriteDropColumn(TTableScript* psTabScript, const char* psColmName, int iColumCount);

    // 记录表data-type 变动
    int WriteColmDataTypeAlterInfo(TTableScript* psTabScript, const char* psColmName, int iOldValue, int iNewValue);

    // 记录表data-len变动
    int WriteColmDataLenAlterInfo(TTableScript* psTabScript, const char* psColmName, int iOldValue, int iNewValue);

    // 记录表is-zip-time变动
    int WriteTabZipTimeAlterInfo(TTableScript* psTabScript, const char* psOldValue,const  char* psNewValue);

    // 记录表table-space变动
    int WriteTabTsAlterInfo(TTableScript* psTabScript,const char* psOldValue,const char* psNewValue);
private:
    int ChangeSequenceFile(const char* sSeqFileName,int iPageCount,char *&pBuffer);//离线模式 删除序列
    //新加的文件尾部
    int AddToSequenceFile(const char* sSeqFileName,TMemSeq &tMemSeq);
    void GetTableSpaceFromTableFile(const char *sTabName,const char *sTabFileName, char *sTabspaceName);//获取表空间的名字
    
    
private:
	char m_sLastDSN[MAX_NAME_LEN];
	char m_sTablespaceName[MAX_NAME_LEN];
	char m_sSysFilePath[MAX_PATH_NAME_LEN];
	char m_sSysFileBakPath[MAX_PATH_NAME_LEN];
	char m_sTabFilePath[MAX_PATH_NAME_LEN];
    
	TDsnScript m_tDsnMgr[MAX_DSN_COUNTS];
	TMdbSqlParser *m_pMdbSqlParser;
	TMDBDBInterface *m_pDBLink; //Oracle链接
	TMdbConfig m_tMdbConfig;  //加载DSN配置文件用户获取oracle连接信息
};

//}
#endif
