/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbScript.h
*@Description： 负责DSN、表结构、表属性对应的XML文件操作
*@Author:       cao.peng
*@Date：        2013年03月20日
*@History:
******************************************************************************************/

#include "Control/mdbScript.h"
#include "Helper/mdbDateTime.h"
#include <stdlib.h>
#include <stdio.h>
#include "dirent.h"

//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{


TTableScript::TTableScript()
{
    m_pTable = NULL;
    m_pTableRoot = NULL;
    m_pTableDocument = NULL;
    m_pTabAlterRoot = NULL;
    m_pTabAlterDoc = NULL;
    memset(m_bIsGenTable,0,sizeof(m_bIsGenTable));
    memset(sTableName,0,sizeof(sTableName));
    memset(m_sTableFileName,0,sizeof(m_sTableFileName));
    memset(m_sTabAlterFile,0,sizeof(m_sTabAlterFile));
}

TTableScript::~TTableScript()
{
    SAFE_DELETE(m_pTableDocument);
    SAFE_DELETE(m_pTabAlterDoc);
}

void TTableScript::Clear()
{
    m_pTable = NULL;
    SAFE_DELETE(m_pTableDocument);
    m_pTableRoot = NULL;
    SAFE_DELETE(m_pTabAlterDoc);
    m_pTabAlterRoot = NULL;
    memset(m_bIsGenTable,0,sizeof(m_bIsGenTable));
    memset(sTableName,0,sizeof(sTableName));
    memset(m_sTableFileName,0,sizeof(m_sTableFileName));
    memset(m_sTabAlterFile,0,sizeof(m_sTabAlterFile));
    memset(m_sTabFilePath,0, sizeof(m_sTabFilePath));
}

TJobScript::TJobScript()
{
    m_pJobDocument = NULL;
    memset(m_sJobFileName,0,sizeof(m_sJobFileName));
}

TJobScript::~TJobScript()
{
    SAFE_DELETE(m_pJobDocument);
}

void TJobScript::Clear()
{
    SAFE_DELETE(m_pJobDocument);
    memset(m_sJobFileName,0,sizeof(m_sJobFileName));
}

int TJobScript::Init()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    m_pJobDocument = new (std::nothrow)MDBXMLDocument();
    CHECK_OBJ(m_pJobDocument); 
    TADD_FUNC("Finish.");
    return iRet;
}

TDsnScript::TDsnScript()
{
    memset(m_sDsnName,0,sizeof(m_sDsnName));
    memset(m_bIsGenDSN,0,sizeof(m_bIsGenDSN));
    memset(m_sSysFileName,0,sizeof(m_sSysFileName));
    memset(m_sTsAlterFile,0,sizeof(m_sTsAlterFile));
    m_sSysFileNameBAK[0] = 0;
    m_pCfgDSN = NULL;
    m_pSysRoot = NULL;
    m_pSysDocument = NULL;
    m_pSysDocumentBAK = NULL;
    m_pSysRootBAK = NULL;
	
    m_pTsAlterRoot = NULL;
	m_pTsAlterDoc = NULL;
    
    for(int i=0;i<MAX_TABLE_COUNTS;i++)
    {
        m_pTablespace[i] = NULL;
    }
    for(int i=0;i<MAX_DSN_COUNTS;i++)
    {
        m_pUser[i] = NULL;
    }

    for(int j=0;j<MAX_TABLE_COUNTS;j++)
    {
        m_tTableMgr[j] = NULL;
    }
}

TDsnScript::~TDsnScript()
{
    SAFE_DELETE(m_pSysDocument);
    SAFE_DELETE(m_pTsAlterDoc);
    for(int j=0;j<MAX_TABLE_COUNTS;j++)
    {
        SAFE_DELETE(m_tTableMgr[j]);
    }
    SAFE_DELETE(m_pSysDocumentBAK);
    
}

void TDsnScript::Clear()
{
    memset(m_sDsnName,0,sizeof(m_sDsnName));
    memset(m_bIsGenDSN,0,sizeof(m_bIsGenDSN));
    memset(m_sSysFileName,0,sizeof(m_sSysFileName));
    memset(m_sTsAlterFile,0,sizeof(m_sTsAlterFile));
    m_pCfgDSN = NULL;
    m_pSysRoot = NULL;
    m_pTsAlterRoot = NULL;
    m_pSysRootBAK = NULL;
    
    for(int i=0;i<MAX_TABLE_COUNTS;i++)
    {
        m_pTablespace[i] = NULL;
    }
    for(int i=0;i<MAX_DSN_COUNTS;i++)
    {
        m_pUser[i] = NULL;
    }
    SAFE_DELETE(m_pSysDocument);
    SAFE_DELETE(m_pSysDocumentBAK);
    SAFE_DELETE(m_pTsAlterDoc);
    for(int j=0;j<MAX_TABLE_COUNTS;j++)
    {
        SAFE_DELETE(m_tTableMgr[j]);
    }
    m_tJob.Clear();
    m_sSysFileNameBAK[0] = 0;
}

/******************************************************************************
* 函数名称	:  Init
* 函数描述	:  初始化工作
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  表文件处理对象
* 作者		:  cao.peng
*******************************************************************************/
int TDsnScript::Init(const char *pszDSN)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pszDSN);
    SAFESTRCPY(m_sDsnName,sizeof(m_sDsnName),pszDSN);
    TMdbNtcStrFunc::ToUpper(m_sDsnName);
    m_pSysDocument = new (std::nothrow)MDBXMLDocument();
    CHECK_OBJ(m_pSysDocument);
    m_pSysDocumentBAK = new (std::nothrow)MDBXMLDocument();
    CHECK_OBJ(m_pSysDocumentBAK);
    m_pTsAlterDoc = new (std::nothrow)MDBXMLDocument();
    CHECK_OBJ(m_pTsAlterDoc);
    CHECK_RET(m_tJob.Init(),"Init job failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  GetTableByTableName
* 函数描述	:  通过表名获取表文件处理对象
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  表文件处理对象
* 作者		:  cao.peng
*******************************************************************************/
TTableScript* TDsnScript::GetTableByTableName(const char*pTableName)
{
    TADD_FUNC("Start.");
    for(int i =0;i<MAX_TABLE_COUNTS;i++)
    {
        if(m_tTableMgr[i] == NULL || m_tTableMgr[i]->sTableName[0] == 0)
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(m_tTableMgr[i]->sTableName,pTableName) == 0)
        {
            return m_tTableMgr[i];
        }
    }
    TADD_FUNC("Finish.");
    return NULL;
}

/******************************************************************************
* 函数名称	:  AddTable
* 函数描述	:  增加表文件处理对象
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TDsnScript::AddTable(const char*pTableName,TTableScript *&pTableScript)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTableName);
    int iTablePos = 0;
    bool bFindIdleTable = false;
    for(iTablePos =0;iTablePos<MAX_TABLE_COUNTS;iTablePos++)
    {
        if(m_tTableMgr[iTablePos] == NULL)
        {
            bFindIdleTable = true;
            break;
        }
        if(m_tTableMgr[iTablePos] != NULL && m_tTableMgr[iTablePos]->sTableName[0] == 0)
        {
            bFindIdleTable = true;
            break;
        }
    }
    if(bFindIdleTable)
    {
        m_tTableMgr[iTablePos] = new (std::nothrow)TTableScript();
        CHECK_OBJ(m_tTableMgr[iTablePos]);
        m_tTableMgr[iTablePos]->Clear();
        m_tTableMgr[iTablePos]->m_pTableDocument = new (std::nothrow)MDBXMLDocument();
        CHECK_OBJ(m_tTableMgr[iTablePos]->m_pTableDocument);
        m_tTableMgr[iTablePos]->m_pTabAlterDoc= new (std::nothrow)MDBXMLDocument();
        CHECK_OBJ(m_tTableMgr[iTablePos]->m_pTabAlterDoc);
        SAFESTRCPY(m_tTableMgr[iTablePos]->sTableName,sizeof(m_tTableMgr[iTablePos]->sTableName),pTableName);
        pTableScript = m_tTableMgr[iTablePos];
    }
    else
    {
        CHECK_RET(ERR_TAB_TABLE_NUM_EXCEED_MAX,"Max table counts=[%d]",MAX_TABLE_COUNTS);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

TMdbScript::TMdbScript()
{
    m_pMdbSqlParser = NULL;
    m_pDBLink = NULL;
    
    memset(m_sLastDSN,0,sizeof(m_sLastDSN));
    memset(m_sTablespaceName,0,sizeof(m_sTablespaceName));
    memset(m_sSysFilePath,0,sizeof(m_sSysFilePath));
    memset(m_sSysFileBakPath,0,sizeof(m_sSysFileBakPath));
    memset(m_sTabFilePath,0,sizeof(m_sTabFilePath));
    
}

TMdbScript::~TMdbScript()
{
    //SAFE_DELETE(m_pMdbSqlParser);
    SAFE_DELETE(m_pDBLink);
}

/******************************************************************************
* 函数名称	:  Init
* 函数描述	:  初始化xml文件存放路径
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::Init(TMdbSqlParser *pSqlParse)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pSqlParse);    
    m_pMdbSqlParser = pSqlParse;
    for(int i=0;i<MAX_DSN_COUNTS;i++)
    {
        m_tDsnMgr[i].Clear();
    }
    
    //新生成的配置文件放到$QuickMDB_HOME/.config/.%DSN%/目录下
    char* pQuickMDBHome = getenv("QuickMDB_HOME");
    if (!pQuickMDBHome)
    {
        TADD_ERROR(ERROR_UNKNOWN,"not find QuickMDB_HOME.");
        return -1;
    }
    snprintf(m_sSysFilePath,sizeof(m_sSysFilePath), "%s/.config/",pQuickMDBHome);
    snprintf(m_sSysFileBakPath,sizeof(m_sSysFileBakPath), "%s/.config/bak/",pQuickMDBHome);
    if(!TMdbNtcDirOper::IsExist(m_sSysFilePath))
    {
        bool bCreate = TMdbNtcDirOper::MakeFullDir(m_sSysFilePath);
    	if(!bCreate)
    	{
    	    TADD_ERROR(ERROR_UNKNOWN,"Mkdir(%s) failed..",m_sSysFilePath);
        	return ERR_OS_CREATE_DIR;
    	} 
    }
    if(!TMdbNtcDirOper::IsExist(m_sSysFileBakPath))
    {
        bool bCreate = TMdbNtcDirOper::MakeFullDir(m_sSysFileBakPath);
    	if(!bCreate)
    	{
    	    TADD_ERROR(ERROR_UNKNOWN,"Mkdir(%s) failed..",m_sSysFileBakPath);
        	return ERR_OS_CREATE_DIR;
    	} 
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  InitDsn
* 函数描述	:  初始化DSN文件路径
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::InitDsn(const char *pDSN,bool bUseFlag)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iPos = -1;
    CHECK_OBJ(pDSN);
    if (pDSN[0] == 0)
    {
        CHECK_RET(ERR_DB_DSN_INVALID, "DSN is empty.  Please specify a database by 'use database' command");
    }
    //新生成的配置文件放到$QuickMDB_HOME/.config/.%DSN%/目录下
    char   sDsnName[MAX_NAME_LEN] = {0};
    char   sConfigHome[MAX_PATH_NAME_LEN] = {0};
    SAFESTRCPY(sDsnName, MAX_NAME_LEN, pDSN);
    TMdbNtcStrFunc::ToUpper(sDsnName);
    SAFESTRCPY(m_sLastDSN,sizeof(m_sLastDSN),pDSN);
    snprintf(sConfigHome,sizeof(sConfigHome), "%s.%s/",m_sSysFilePath,sDsnName);
    if(!TMdbNtcDirOper::IsExist(sConfigHome))
    {
        if(bUseFlag)
        {//use 指令，必须确保实例存在
            CHECK_RET(ERR_DB_DSN_INVALID,"Dsn[%s] not exist..",sDsnName);
        }
        if(!TMdbNtcDirOper::MakeFullDir(sConfigHome))
        {
            TADD_ERROR(ERROR_UNKNOWN,"Mkdir(%s) failed.",sConfigHome);
            return ERR_OS_CREATE_DIR;
        }
    }

    iPos = FindDsnEx(sDsnName);
    if(iPos < 0)
    {
        iPos = SetDsnEx(sDsnName);
    }
    
    snprintf(m_tDsnMgr[iPos].m_sSysFileName,sizeof(m_tDsnMgr[iPos].m_sSysFileName),\
                                "%s.QuickMDB_SYS_%s.xml",sConfigHome,sDsnName);
    snprintf(m_tDsnMgr[iPos].m_sSysFileNameBAK,sizeof(m_tDsnMgr[iPos].m_sSysFileNameBAK),\
                                "%s/.BAK/.QuickMDB_SYS_%s.xml",sConfigHome,sDsnName);
    m_tDsnMgr[iPos].m_sSysFileNameBAK[sizeof(m_tDsnMgr[iPos].m_sSysFileNameBAK)-1] = 0;                         
    snprintf(m_tDsnMgr[iPos].m_sTsAlterFile,sizeof(m_tDsnMgr[iPos].m_sTsAlterFile),\
                                "%s.TableSpace_update.xml",sConfigHome);
    
    CHECK_RET(InitJobInfo(sDsnName),"InitJobInfo failed.");
    TADD_FUNC("Start.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  InitTableInfo
* 函数描述	:  初始化表以及属性的xml名称
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::InitTableInfo(const char *pDSN,TMdbTable *pTable)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pDSN);
    CHECK_OBJ(pTable);
    //新生成的配置文件放到$QuickMDB_HOME/.config/%DSN%/.TABLE/.%TABLE_NAME%目录下
    char  dsnName[MAX_NAME_LEN] = {0};
    char  sConfigHome[MAX_PATH_NAME_LEN] = {0};
    char  sTmpFile[MAX_PATH_NAME_LEN]={0};
    SAFESTRCPY(dsnName, MAX_NAME_LEN, pDSN);
    TMdbNtcStrFunc::ToUpper(dsnName);
    //首先查找DSN文件所在目录,如果不存在则新创建目录
    int iPos = FindDsnEx(dsnName);
    if(iPos < 0)
    {
        iPos = SetDsnEx(dsnName);
    }
    //查找dsn 文件
    if(m_tDsnMgr[iPos].m_sSysFileName[0] == 0 || !TMdbNtcFileOper::IsExist(m_tDsnMgr[iPos].m_sSysFileName))
    {
        CHECK_RET(ERR_DB_DSN_INVALID,"DSN[%s] not exist.",dsnName);
    }
    char sTabName[MAX_NAME_LEN] = {0};
    SAFESTRCPY(sTabName, sizeof(sTabName), pTable->sTableName);
    TMdbNtcStrFunc::ToUpper(sTabName);
    snprintf(sConfigHome,sizeof(sConfigHome), "%s.%s/.TABLE/.%s/",m_sSysFilePath,dsnName, sTabName);
    
    //查找表空间
    const char *pTableSpaceName = pTable->m_sTableSpace;
    if(pTableSpaceName[0] == 0)
    {
        pTableSpaceName = m_sTablespaceName;
    }
    if(pTableSpaceName[0] == 0)
    {//在xml中再找一次
        char sTabspace[MAX_NAME_LEN]={0};
        char sTableFileName[MAX_PATH_NAME_LEN]={0};
        snprintf(sTableFileName,MAX_PATH_NAME_LEN,"%s.Tab_%s_%s.xml",sConfigHome,dsnName,sTabName);
        GetTableSpaceFromTableFile(sTabName,sTableFileName,sTabspace);
        pTableSpaceName = sTabspace;
    }
    if(!CheckXmlTableSpaceExist(pTableSpaceName))
    {//表空间不存在，报错
        CHECK_RET(ERR_DB_TABLESPACE_NOT_EXIST,"tablespace[%s] not exist.",pTableSpaceName);
    }
    
    //查找表是否已经初始化
    TTableScript* pTableScript = m_tDsnMgr[iPos].GetTableByTableName(pTable->sTableName);
    if(pTableScript == NULL)
    {
        CHECK_RET(m_tDsnMgr[iPos].AddTable(pTable->sTableName,pTableScript),"AddTable failed.");
    }

    
    snprintf(pTableScript->m_sTableFileName,sizeof(pTableScript->m_sTableFileName),\
        "%s.Tab_%s_%s.xml",sConfigHome,dsnName,sTabName);
    snprintf(pTableScript->m_sTabAlterFile,sizeof(pTableScript->m_sTableFileName),\
        "%s.Tab_%s_update.xml",sConfigHome,sTabName);
    SAFESTRCPY(pTableScript->m_sTabFilePath, sizeof(pTableScript->m_sTabFilePath), sConfigHome);
    SAFESTRCPY(m_sTabFilePath, sizeof(m_sTabFilePath), sConfigHome);
    //对于修改表结构和表属性的操作，需要模糊匹配到原文件名,
    //防止表结构配置文件中表名有大小写的情况
    if(TMdbNtcDirOper::IsExist(sConfigHome))
    {
	    DIR *dp = opendir(sConfigHome);
	    CHECK_OBJ(dp);
	    struct dirent *dirp;
	    while((dirp=readdir(dp)) != NULL)
	    {
	        memset(sTmpFile,0,sizeof(sTmpFile));
	        snprintf(sTmpFile,sizeof(sTmpFile),"%s%s",sConfigHome,dirp->d_name);
	        if(TMdbNtcStrFunc::StrNoCaseCmp(pTableScript->m_sTableFileName,sTmpFile) == 0)
	        {
	            SAFESTRCPY(pTableScript->m_sTableFileName,sizeof(pTableScript->m_sTableFileName),sTmpFile);
	        }
	        
	    }
	    closedir(dp);
    }
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* 函数名称	:  InitJobInfo
* 函数描述	:  初始化job配置文件路径及备份路径
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::InitJobInfo(const char *pDSN)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pDSN);
    //新生成的配置文件放到$QuickMDB_HOME/.config/%DSN%/.JOB/目录下
    char dsnName[MAX_NAME_LEN] = {0};
    char sConfigHome[MAX_PATH_NAME_LEN] = {0};
    SAFESTRCPY(dsnName, MAX_NAME_LEN, pDSN);
    TMdbNtcStrFunc::ToUpper(dsnName);
    int iPos = FindDsnEx(dsnName);
    if(iPos < 0)
    {
        iPos = SetDsnEx(dsnName);
    }

    snprintf(sConfigHome,sizeof(sConfigHome), "%s.%s/.JOB/",m_sSysFilePath,dsnName);
    snprintf(m_tDsnMgr[iPos].m_tJob.m_sJobFileName,\
        sizeof(m_tDsnMgr[iPos].m_tJob.m_sJobFileName),\
        "%s.QuickMDB_JOB.xml",sConfigHome);
    
    //首先查找JOB文件所在目录,如果不存在则新创建.config/.%DSN%/.JOB/目录
    if(!TMdbNtcFileOper::IsExist(m_tDsnMgr[iPos].m_tJob.m_sJobFileName))
    {
        if(!TMdbNtcDirOper::IsExist(sConfigHome))
        {
            bool bCreate = TMdbNtcDirOper::MakeFullDir(sConfigHome);
        	if(!bCreate)
        	{
        	    TADD_ERROR(ERROR_UNKNOWN,"Mkdir(%s) failed.",sConfigHome);
            	return ERR_OS_CREATE_DIR;
        	} 
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ExportXML
* 函数描述	:  执行XML操作入口
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::Execute(const char *pszDDLSql)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pszDDLSql);
    CHECK_RET(m_pMdbSqlParser->ParseSQL(pszDDLSql),"Error[%d],msg=[%s]",\
        m_pMdbSqlParser->m_tError.GetErrCode(),m_pMdbSqlParser->m_tError.GetErrMsg());
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    if(m_pMdbSqlParser->m_pDDLSqlStruct->sDsnName[0] != 0)
    {
        iRet = FindDsnEx(m_pMdbSqlParser->m_pDDLSqlStruct->sDsnName);
        if(iRet < 0)
        {
            SetDsnEx(m_pMdbSqlParser->m_pDDLSqlStruct->sDsnName);
        }
        SAFESTRCPY(m_sLastDSN,sizeof(m_sLastDSN),m_pMdbSqlParser->m_pDDLSqlStruct->sDsnName);
    }

    SetTableSpace(m_pMdbSqlParser->m_pDDLSqlStruct->sTableSpaceName);
    
    int iSqlType1 = m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0];
    int iSqlType2 = m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[1];
    switch(iSqlType2)
    {
        case TK_DATABASE:
        case TK_TABLESPACE:
        case TK_CONNECT:
        case TK_USER:
        {
            CHECK_RET(ProcessDsnRep(iSqlType1,iSqlType2),"Failed to export the xml.");
            break;
        }
        case TK_TABLE:
        case TK_TABLESYS:
        case TK_COLUMN:
        case TK_INDEX:
        case TK_PRIMARY:
        case TK_PARAMETER:
        {
            CHECK_RET(ProcessTableRep(iSqlType1,iSqlType2),"Failed to export the xml.");
            break;
        }
        case TK_SEQUENCE:
        {
            //增加或删除oracle侧mdb_sequence表中记录
            if(iSqlType1 == TK_ADD)
            {
                CHECK_RET(AddSequenceToOracle(),"AddSequenceToOracle() Failed.");
            }
            else if(iSqlType1 == TK_DROP)
            {
                CHECK_RET(DelSequenceToOracle(),"DelSequenceToOracle() Failed.");
            }
            break;
        }
        case TK_JOB:
        {
            //增加或删除oracle侧mdb_sequence表中记录
            if(iSqlType1 == TK_CREATE)
            {
                CHECK_RET(CreateJob(m_pMdbSqlParser->m_pDDLSqlStruct->pMdbJob),"CreateJob() Failed.");
            }
            else if(iSqlType1 == TK_ALTER)
            {
                CHECK_RET(AlterJob(m_pMdbSqlParser->m_pDDLSqlStruct->pMdbJob),"ALterJob() Failed.");
            }
            else if(iSqlType1 == TK_REMOVE)
            {
                CHECK_RET(RemoveJob(m_pMdbSqlParser->m_pDDLSqlStruct->pMdbJob),"RemoveJob() Failed.");
            }
            break;
        }
        default:
        {
            TADD_ERROR(ERROR_UNKNOWN,"error sql type[%d|%d].",iSqlType1,iSqlType2);
            iRet = -1;
            break;
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ProcessDsnRep
* 函数描述	:  处理DSN相关的操作
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ProcessDsnRep(const int iSqlType1,const int iSqlType2)
{   
    TADD_FUNC("Start.");
    int iRet = 0;
    int iDsnPos = -1;
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    bool bUseFlag = (iSqlType1 == TK_USE);
    //初始化指定DSN文件路径
    CHECK_RET(InitDsn(m_sLastDSN,bUseFlag),"Failed to initialize dsn.");
    //对于USE命令，只保存DSN名就可以了
    if(bUseFlag)
    {
        int iPos = GetSysRootNode(false);
        if(iPos < 0)
        {
            return ERR_DB_DSN_INVALID;
        }
        if(iSqlType2 == TK_TABLESPACE)
        {
            if(!CheckXmlTableSpaceExist(m_sTablespaceName))
            {//表空间不存在，报错
                TADD_ERROR(ERR_DB_TABLESPACE_NOT_EXIST,"talespace[%s] not exist.",m_sTablespaceName);
                return ERR_DB_TABLESPACE_NOT_EXIST;
            }
            
        }
        return iRet;
        
    }
    //处理DSN文件相关命令
    switch(iSqlType1)
    {
        case TK_CREATE:
        {
            if(iSqlType2 == TK_DATABASE)
            {
                iRet = CreateDsn(m_pMdbSqlParser->m_pDDLSqlStruct->pDsn);
            }
            else if(iSqlType2 == TK_USER)
            {
                iRet = CreateUser(m_pMdbSqlParser->m_pDDLSqlStruct->pUser,false);
            }
            else if(iSqlType2 == TK_TABLESPACE)
            {
                iRet = CreateTableSpace(m_pMdbSqlParser->m_pDDLSqlStruct->pTablespace,false);
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"error sql type[%d|%d].",iSqlType1,iSqlType2);
                return -1;
            }
            break;
        }
        case TK_ALTER:
        {
            if(iSqlType2 == TK_DATABASE)
            {
                iRet = ModifyDsn(false);
            }
            else if(iSqlType2 == TK_USER)
            {
                iRet = ModifyUser(m_pMdbSqlParser->m_pDDLSqlStruct->pUser,false);
            }
            else if(iSqlType2 == TK_TABLESPACE)
            {
                iRet = ModifyTableSpace(m_pMdbSqlParser->m_pDDLSqlStruct->pTablespace,false);
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"error sql type[%d|%d].",iSqlType1,iSqlType2);
                return -1;
            }
            break;
        }
        case TK_DROP:
        {
            iRet = ExecuteDropOpt(iSqlType2);
            break;
        }
        case TK_CONNECT:
        {
            iRet = GenConnectInfo(m_pMdbSqlParser->m_pDDLSqlStruct->pDsn);
            break;
        }
        default:
        {
            TADD_ERROR(ERROR_UNKNOWN,"error sql type[%d|%d].",iSqlType1,iSqlType2);
            return -1;
        }
    }
    CHECK_RET(iRet,"Failure of processing DSN config information.");

	//删除数据库实例的操作无需备份文件
	if (TK_DATABASE == iSqlType2 && TK_DROP == iSqlType1){return iRet;}
	
    iDsnPos = FindDsnEx(m_sLastDSN);
    if(iDsnPos < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Invalid DSN[%s].",m_sLastDSN);
        return -1;
    }
    if(IsGenDsnXml() || TMdbNtcFileOper::IsExist(m_tDsnMgr[iDsnPos].m_sSysFileName))
    {
        BakFile(m_tDsnMgr[iDsnPos].m_sSysFileName);
        m_tDsnMgr[iDsnPos].m_pSysDocument->SaveFile(m_tDsnMgr[iDsnPos].m_sSysFileName);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ProcessTableRep
* 函数描述	:  处理表相关的操作
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ProcessTableRep(const int iSqlType1,const int iSqlType2)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    CHECK_RET(InitTableInfo(m_sLastDSN,m_pMdbSqlParser->m_pDDLSqlStruct->pTable),"InitTableInfo failed");
    
    //表管理区中不存在，则需要重新设置
    int iDsnPos = FindDsnEx(m_sLastDSN);
    if(iDsnPos < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Invalid DSN[%s].",m_sLastDSN);
        return -1;
    }
    TTableScript* pTableScript = m_tDsnMgr[iDsnPos].GetTableByTableName(m_pMdbSqlParser->m_pDDLSqlStruct->pTable->sTableName);
    if(pTableScript == NULL)
    {
        CHECK_RET(m_tDsnMgr[iDsnPos].AddTable(m_pMdbSqlParser->m_pDDLSqlStruct->pTable->sTableName,pTableScript),"AddTable failed.");
    }
    switch(iSqlType1)
    {
        case TK_CREATE:
        {
            if(iSqlType2 == TK_TABLE)
            {
                iRet = CreateTable(m_pMdbSqlParser->m_pDDLSqlStruct->pTable,false);
                if(iRet != 0 && iRet != ERR_TAB_CONFIG_TABLENAME_ALREADY_EXIST)
                {//创建失败就把表目录删除
                    RemoveTabDir();
                }
            }
            else if(iSqlType2 == TK_INDEX)
            {
                iRet = AddIndex(false);
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"error sql type[%d|%d].",iSqlType1,iSqlType2);
                return -1;
            }
            break;
        }
        case TK_MODIFY:
        {
            if(iSqlType2 == TK_TABLESYS)
            {
                iRet = ModifyTableAttribute(false);
            }
            else if(iSqlType2 == TK_COLUMN)
            {
                iRet = ModifyColumn(false);
            }
            else if(iSqlType2 == TK_PARAMETER)
            {
                iRet = ModifyFlushOrLoadSQLParam(false);
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"error sql type[%d|%d].",iSqlType1,iSqlType2);
                return -1;
            }
            break;
        }
        case TK_ADD:
        {
            if(iSqlType2 == TK_TABLESYS)
            {
                iRet = AddSingleTableAttribute(m_pMdbSqlParser->m_pDDLSqlStruct->pTable);
            }
            else if(iSqlType2 == TK_COLUMN)
            {
                iRet = AddColumn(m_pMdbSqlParser->m_pDDLSqlStruct->pTable,false, true);
            }
            else if(iSqlType2 == TK_PRIMARY)
            {
                iRet = AddPrimaryKey(m_pMdbSqlParser->m_pDDLSqlStruct->pTable);
            }
            else if(iSqlType2 == TK_PARAMETER)
            {
                iRet = AddFlushOrLoadSQLParam(m_pMdbSqlParser->m_pDDLSqlStruct->pTable, false);
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"error sql type[%d|%d].",iSqlType1,iSqlType2);
                return -1;
            }
            break;
        }
        case TK_DROP:
        {
            iRet = ExecuteDropOpt(iSqlType2);
            break;
        }
        case TK_RENAME:
        {
            iRet = RenameTable(m_pMdbSqlParser->m_pDDLSqlStruct->pTable->sTableName,m_pMdbSqlParser->m_pDDLSqlStruct->sNewTableName,false);
            break;
        }
        
        default:
        {
            TADD_ERROR(ERROR_UNKNOWN,"error sql type[%d|%d].",iSqlType1,iSqlType2);
            iRet = -1;
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ExecuteDropOpt
* 函数描述	:  执行drop操作
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ExecuteDropOpt(const int iSqlType)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    if(iSqlType == TK_TABLE)
    {
        iRet = DropTable(m_pMdbSqlParser->m_pDDLSqlStruct->pTable->sTableName);
    }
    else if(iSqlType == TK_TABLESYS)
    {
        iRet = DropTableAttribute(m_pMdbSqlParser->m_pDDLSqlStruct->pTable);
    }
    else if(iSqlType == TK_COLUMN)
    {
        iRet = DropColumn(false);
    }
    else if(iSqlType == TK_INDEX)
    {
        iRet = DropIndex(false);
    }
    else if(iSqlType == TK_PARAMETER)
    {   
        iRet = DropFlushOrLoadSQLParam(m_pMdbSqlParser->m_pDDLSqlStruct->pTable,\
            m_pMdbSqlParser->m_pDDLSqlStruct->tParam.sName,false);
    }
    else if(iSqlType == TK_DATABASE)
    {
        iRet = DropDsn(m_pMdbSqlParser->m_pDDLSqlStruct->sDsnName);
    }
    else if(iSqlType == TK_USER)
    {
        iRet = DropUser(m_pMdbSqlParser->m_pDDLSqlStruct->pUser,false);
    }
    else if(iSqlType == TK_TABLESPACE)
    {
        iRet = DropTableSpace(m_pMdbSqlParser->m_pDDLSqlStruct->sTableSpaceName,false);
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"error sql type[%d].",iSqlType);
        return -1;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  BakFile
* 函数描述	:  文件备份
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::BakFile(const char* pFilePathName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pFilePathName);
    char sCurTime[MAX_TIME_LEN] = {0};
    char sBakFile[MAX_PATH_NAME_LEN] = {0};
    char sTmpFile[MAX_PATH_NAME_LEN] = {0};
    TMdbDateTime::GetCurrentTimeStr(sCurTime);
    //如果存在则先备份，然后重新生成
    if(TMdbNtcDirOper::IsExist(pFilePathName))
    {
        snprintf(sBakFile,sizeof(sBakFile),"%s/%s",m_sSysFileBakPath,\
            TMdbNtcFileOper::GetFileName(pFilePathName));
        TMdbNtcFileOper::Copy(pFilePathName,sBakFile);
        snprintf(sTmpFile,sizeof(sTmpFile),"%s.%s",sBakFile,sCurTime);
        TMdbNtcFileOper::Rename(sBakFile,sTmpFile);
    }
    TADD_FUNC("Finish.");
    return iRet;

}

/******************************************************************************
* 函数名称	:  IsGenDsnXml
* 函数描述	:  判断是否需要生成DSN对应的XML文件
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  true - 生成 false -不生成
* 作者		:  cao.peng
*******************************************************************************/
bool TMdbScript::IsGenDsnXml()
{
    TADD_FUNC("Start.");
    int iPos = FindDsnEx(m_sLastDSN);
    if(iPos < 0)
    {
        return false;
    }
    for(int i=0;i<4;i++)
    {
        if(!m_tDsnMgr[iPos].m_bIsGenDSN[i])
        {
            return false;
        }
    }
    TADD_FUNC("Finish.");
    return true;
}

/******************************************************************************
* 函数名称	:  GetSysRootNode
* 函数描述	:  获取DSN文件中的MDBConfig节点
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::GetSysRootNode(bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iPos = -1;
    if (m_sLastDSN[0] == '\0')
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM, "DSN is empty. Please specify a database by \'use database' command.");
        return -1;
    }
    if(bIsDynamic)
    {
        iPos = FindDsnEx(m_sLastDSN);
        if(iPos < 0)
        {
            iPos = SetDsnEx(m_sLastDSN);
            m_tDsnMgr[iPos].m_pSysDocument->Clear();
        }
    }
    else
    {
        iPos = FindDsnEx(m_sLastDSN);
        if(iPos < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"DSN[%s] has not been created, can not be used.",m_sLastDSN);
            return -1;
        }
    }
    bool bExist = false;
    if(NULL == m_tDsnMgr[iPos].m_pSysRoot)
    {
        m_tDsnMgr[iPos].m_pSysDocument->Clear();
        bExist = m_tDsnMgr[iPos].m_pSysDocument->LoadFile(m_tDsnMgr[iPos].m_sSysFileName);
        if(!bExist)
        {
            TADD_ERROR(ERROR_UNKNOWN,"LoadXMLFile(name=%s) failed.",m_tDsnMgr[iPos].m_sSysFileName);
            return -1;
        }
        m_tDsnMgr[iPos].m_pSysRoot = m_tDsnMgr[iPos].m_pSysDocument->FirstChildElement("MDBConfig");
        CHECK_OBJ(m_tDsnMgr[iPos].m_pSysRoot);
    }
    if(NULL == m_tDsnMgr[iPos].m_pSysRootBAK && TMdbNtcFileOper::IsExist(m_tDsnMgr[iPos].m_sSysFileNameBAK))
    {
        m_tDsnMgr[iPos].m_pSysDocumentBAK->Clear();
        bExist = m_tDsnMgr[iPos].m_pSysDocumentBAK->LoadFile(m_tDsnMgr[iPos].m_sSysFileNameBAK);
        if(!bExist)
        {
            TADD_ERROR(ERROR_UNKNOWN,"LoadXMLFile(name=%s) failed.",m_tDsnMgr[iPos].m_sSysFileNameBAK);
            return iPos;
        }
        m_tDsnMgr[iPos].m_pSysRootBAK= m_tDsnMgr[iPos].m_pSysDocumentBAK->FirstChildElement("MDBConfig");
        CHECK_OBJ(m_tDsnMgr[iPos].m_pSysRootBAK);
    }
    
    TADD_FUNC("Finish.");
    return iPos;
}

/******************************************************************************
* 函数名称	:  CreateTableSpace
* 函数描述	:  新增表空间
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::CreateTableSpace(TMdbTableSpace* pTableSpace,bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTableSpace);
    int iPos = GetSysRootNode(bIsDynamic);
    if(iPos < 0)
    {
        return -1;
    }
    //判断新建的表空间是否存在
    for(MDBXMLElement* pEle = m_tDsnMgr[iPos].m_pSysRoot->FirstChildElement("table-space"); pEle; pEle=pEle->NextSiblingElement("table-space"))
    {
        if(NULL == pEle->Attribute("name"))
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pTableSpace->sName) == 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"talespace[%s] already exists.",pTableSpace->sName);
            return ERR_DB_TABLESPACE_ALREADY_EXIST;
        }
    }

    MDBXMLElement *pTablespaceElement = new MDBXMLElement("table-space");
    CHECK_OBJ(pTablespaceElement);
    m_tDsnMgr[iPos].m_pSysRoot->LinkEndChild(pTablespaceElement);
    pTablespaceElement->SetAttribute("name",pTableSpace->sName);
    pTablespaceElement->SetAttribute("page-size",pTableSpace->iPageSize/1024);
    pTablespaceElement->SetAttribute("ask-pages",pTableSpace->iRequestCounts);
    pTablespaceElement->SetAttribute("is-file-storage",pTableSpace->m_bFileStorage?"Y":"N");

    //如果是在线创建表空间说明DSN配置文件已经存在了
    if(bIsDynamic)
    {
        // 备份
        CHECK_RET(BakFile(m_tDsnMgr[iPos].m_sSysFileName),\
            "Failed to backup file[%s]",m_tDsnMgr[iPos].m_sSysFileName);
        m_tDsnMgr[iPos].m_pSysDocument->SaveFile(m_tDsnMgr[iPos].m_sSysFileName);
    }

    // 写表空间变更文件
    CHECK_RET(WriteTsAddInfo(&(m_tDsnMgr[iPos]), pTableSpace->sName),"write table space add info failed.");

    m_tDsnMgr[iPos].m_bIsGenDSN[2] = true;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ModifyTableSpace
* 函数描述	:  修改表空间信息
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ModifyTableSpace(TMdbTableSpace* pTableSpace,bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    MDBXMLElement* pEle = NULL;
    bool bIsExist = false;
    size_t iOldPageSize = 0;
    CHECK_OBJ(pTableSpace);    
    int iPos = GetSysRootNode(bIsDynamic);
    if(iPos < 0)
    {
        return -1;
    }
    
    //查询需要修改的表空间节点
    for(pEle = m_tDsnMgr[iPos].m_pSysRoot->FirstChildElement("table-space");pEle;pEle=pEle->NextSiblingElement("table-space"))
    {
        //校验修改的是否
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pTableSpace->sName) == 0)
        {
            bIsExist = true;
            // 获取page-size的值
            iOldPageSize =atoi( pEle->Attribute("page-size"));
            break;
        }
    }
    
    if(bIsExist)
    {
        if(bIsDynamic)
        {
            pEle->SetAttribute("page-size",pTableSpace->iPageSize/1024);
            pEle->SetAttribute("ask-pages",pTableSpace->iRequestCounts);
        }
        else
        {
            //分别重置表空间ID、页大小、每次请求页数
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[0].sAttrName,\
                "PAGESIZE") ==0)
            {
                pEle->SetAttribute("page-size",pTableSpace->iPageSize/1024);
            }
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[1].sAttrName,\
                "ASKPAGE") ==0)
            {
                pEle->SetAttribute("ask-pages",pTableSpace->iRequestCounts);
            }
        }
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"Talespace[%s] does not exist.",pTableSpace->sName);
        return ERR_DB_TPS_ID_NOT_EXIST;
    }
    
    if(bIsDynamic)
    {
        CHECK_RET(BakFile(m_tDsnMgr[iPos].m_sSysFileName),\
            "Failed to backup file[%s]",m_tDsnMgr[iPos].m_sSysFileName);
        m_tDsnMgr[iPos].m_pSysDocument->SaveFile(m_tDsnMgr[iPos].m_sSysFileName);
    }

    if(iOldPageSize != (pTableSpace->iPageSize/1024))
    {
        CHECK_RET(WriteTsPageSizeAlterInfo(&(m_tDsnMgr[iPos]),  pTableSpace->sName,iOldPageSize, (pTableSpace->iPageSize/1024)),"write table space page-szie alter info failed.");
    }
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* 函数名称	:  DropTableSpace
* 函数描述	:  删除表空间
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::DropTableSpace(const char *pTablespaceName,bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    MDBXMLElement* pEle = NULL;
    bool bIsExist = false;
    CHECK_OBJ(pTablespaceName);
    int iPos = GetSysRootNode(bIsDynamic);
    if(iPos < 0)
    {
        return -1;
    }
    
    //查询需要删除的表空间节点是否存在
    for(pEle = m_tDsnMgr[iPos].m_pSysRoot->FirstChildElement("table-space");pEle;pEle=pEle->NextSiblingElement("table-space"))
    {
        //校验修改的是否
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pTablespaceName) == 0)
        {
            bIsExist = true;
            break;
        }
    }
    
    //存在就删除该节点，不存在报错退出
    if(bIsExist)
    {
        m_tDsnMgr[iPos].m_pSysRoot->RemoveChild(pEle);
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"Talespace[%s] does not exist.",pTablespaceName);
        return ERR_DB_TPS_ID_NOT_EXIST;
    }

    if(bIsDynamic)
    {
        CHECK_RET(BakFile(m_tDsnMgr[iPos].m_sSysFileName),\
            "Failed to backup file[%s]",m_tDsnMgr[iPos].m_sSysFileName);
        m_tDsnMgr[iPos].m_pSysDocument->SaveFile(m_tDsnMgr[iPos].m_sSysFileName);
    }
    CHECK_RET(WriteTsDelInfo(&(m_tDsnMgr[iPos]), pTablespaceName),"write tablespace delete info failed.");

    //BAK
    //查询需要删除的表空间节点是否存在
    if(m_tDsnMgr[iPos].m_pSysRootBAK && bIsDynamic) 
    {//动态
        bIsExist = false;
        for(pEle = m_tDsnMgr[iPos].m_pSysRootBAK->FirstChildElement("table-space");pEle;pEle=pEle->NextSiblingElement("table-space"))
        {
            //校验修改的是否
            if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pTablespaceName) == 0)
            {
                bIsExist = true;
                break;
            }
        }
        if(bIsExist)
        {
            m_tDsnMgr[iPos].m_pSysRootBAK->RemoveChild(pEle);
            m_tDsnMgr[iPos].m_pSysDocumentBAK->SaveFile(m_tDsnMgr[iPos].m_sSysFileNameBAK);
        }
    }
    
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* 函数名称	:  CreateUser
* 函数描述	:  新增用户
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::CreateUser(TMDbUser* pUser,bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pUser);
    int iPos = GetSysRootNode(bIsDynamic);
    if(iPos < 0)
    {
        return -1;
    }
    
    //判断新建的用户是否存在
    for(MDBXMLElement* pEle = m_tDsnMgr[iPos].m_pSysRoot->FirstChildElement("user");pEle;pEle=pEle->NextSiblingElement("user"))
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pUser->sUser) == 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"user[%s] already exists.",pUser->sUser);
            return ERR_APP_INVALID_PARAM;
        }
    }

    MDBXMLElement *pUserElement = new MDBXMLElement("user");
    CHECK_OBJ(pUserElement);
    m_tDsnMgr[iPos].m_pSysRoot->LinkEndChild(pUserElement);
    pUserElement->SetAttribute("name",pUser->sUser);
    pUserElement->SetAttribute("password",pUser->sPwd);
    if(TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess, "Administrator") == 0)
    {
        pUserElement->SetAttribute("access","A");
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess, "Read-Write") == 0)
    {
        pUserElement->SetAttribute("access","W");
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess, "ReadOnly") == 0)
    {
        pUserElement->SetAttribute("access","R");

    }
    else
    {   
        TADD_ERROR(ERROR_UNKNOWN,"Invalid Access=[%s].",pUser->sAccess);
        return ERR_APP_INVALID_PARAM;
    }
    
    if(bIsDynamic)
    {   
        CHECK_RET(BakFile(m_tDsnMgr[iPos].m_sSysFileName),\
            "Failed to backup file[%s]",m_tDsnMgr[iPos].m_sSysFileName);
        m_tDsnMgr[iPos].m_pSysDocument->SaveFile(m_tDsnMgr[iPos].m_sSysFileName);
    }
    m_tDsnMgr[iPos].m_bIsGenDSN[3] = true;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ModifyUser
* 函数描述	:  修改用户信息
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ModifyUser(TMDbUser* pUser,bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bIsExist = false;
    MDBXMLElement* pEle = NULL;
    CHECK_OBJ(pUser);
    int iPos = GetSysRootNode(bIsDynamic);
    if(iPos < 0)
    {
        return -1;
    }
    
    //判断修改的用户是否存在
    for(pEle = m_tDsnMgr[iPos].m_pSysRoot->FirstChildElement("user"); pEle; pEle=pEle->NextSiblingElement("user"))
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pUser->sUser) == 0)
        {
            bIsExist = true;
            break;
        }
    }

    //用户存在才修改否则报错退出
    if(bIsExist)
    {
        if(pUser->sPwd[0] != 0)
        {
            pEle->SetAttribute("password",pUser->sPwd);
        }

        if(pUser->sAccess[0] != 0)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess, "Administrator") == 0)
            {
                pEle->SetAttribute("access","A");
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess, "Read-Write") == 0)
            {
                pEle->SetAttribute("access","W");
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess, "ReadOnly") == 0)
            {
                pEle->SetAttribute("access","R");

            }
            else
            {   
                TADD_ERROR(ERROR_UNKNOWN,"Invalid Access=[%s].",pUser->sAccess);
                return ERR_APP_INVALID_PARAM;
            }
        }
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"user[%s] does not exist.",pUser->sUser);
        return ERR_APP_INVALID_PARAM;
    }

    if(bIsDynamic)
    {   
        CHECK_RET(BakFile(m_tDsnMgr[iPos].m_sSysFileName),\
            "Failed to backup file[%s]",m_tDsnMgr[iPos].m_sSysFileName);
        m_tDsnMgr[iPos].m_pSysDocument->SaveFile(m_tDsnMgr[iPos].m_sSysFileName);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  DropUser
* 函数描述	:  删除用户
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::DropUser(TMDbUser* pUser,bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bIsExist = false;
    MDBXMLElement* pEle = NULL;
    CHECK_OBJ(pUser);
    int iPos = GetSysRootNode(bIsDynamic);
    if(iPos < 0)
    {
        return -1;
    }
    //判断用户是否存在
    for(pEle = m_tDsnMgr[iPos].m_pSysRoot->FirstChildElement("user"); pEle; pEle=pEle->NextSiblingElement("user"))
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pUser->sUser) == 0)
        {
            bIsExist = true;
            break;
        }
    }
    //用户存在才允许删除，否则报错退出
    if(bIsExist)
    {
        m_tDsnMgr[iPos].m_pSysRoot->RemoveChild(pEle);
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"user[%s] does not exist.",pUser->sUser);
        return ERR_APP_INVALID_PARAM;
    }
    
    if(bIsDynamic)
    {   
        CHECK_RET(BakFile(m_tDsnMgr[iPos].m_sSysFileName),\
            "Failed to backup file[%s]",m_tDsnMgr[iPos].m_sSysFileName);
        m_tDsnMgr[iPos].m_pSysDocument->SaveFile(m_tDsnMgr[iPos].m_sSysFileName);
    }
    TADD_FUNC("Finish.");
    return iRet;

}

/******************************************************************************
* 函数名称	:  GenConnectInfo
* 函数描述	:  生成oracle链接信息
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::GenConnectInfo(TMdbCfgDSN  *pDsn)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pDsn);
    int iPos = GetSysRootNode(false);
    if(iPos < 0)
    {
        return -1;
    }
    MDBXMLElement* pEDS = m_tDsnMgr[iPos].m_pSysRoot->FirstChildElement("DataSource");
    CHECK_OBJ(pEDS);
    pEDS->SetAttribute("name",m_sLastDSN);
    if(pDsn->cType == MDB_DS_TYPE_ORACLE)
    {   
        pEDS->SetAttribute("type","oracle");
    }
    else if(pDsn->cType == MDB_DS_TYPE_MYSQL)
    {   
        pEDS->SetAttribute("type","mysql");
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"Invalid database type[%c].",pDsn->cType);
        return -1;
    }
    
    pEDS->SetAttribute("user",pDsn->sOracleUID);
    pEDS->SetAttribute("password", pDsn->sOraclePWD);
    pEDS->SetAttribute("db-id", pDsn->sOracleID);
    m_tDsnMgr[iPos].m_bIsGenDSN[1] = true;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  AddSysInfo
* 函数描述	:  增加DSN属性
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::AddSysInfo(MDBXMLElement *pESys,TMdbCfgDSN  *pDsn)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    CHECK_OBJ(pESys);
    ADDSYSINFO(pESys,"dsn-value", pDsn->llValue);
    ADDSYSINFO(pESys,"log-level", pDsn->iLogLevel);
    ADDSYSINFO(pESys,"log-time", pDsn->iLogTime);
    ADDSYSINFO(pESys,"log-count", pDsn->iLogCount);
    ADDSYSINFO(pESys,"kill-time", pDsn->iKillTime);
    ADDSYSINFO(pESys,"Delay-Time", pDsn->iDelayTime);
    ADDSYSINFO(pESys,"Clean-Time", pDsn->iCleanTime);
    ADDSYSINFO(pESys,"Rep-Type", pDsn->iRepType);
    ADDSYSINFO(pESys,"ora-rep-counts", pDsn->iOraRepCounts);
    ADDSYSINFO_YN(pESys,"is-ora-rep", pDsn->bIsOraRep);
    //ADDSYSINFO_YN(pESys,"is-rep", pDsn->bIsRep);
    //ADDSYSINFO_YN(pESys,"is-peer-rep", pDsn->bIsPeerRep);
    ADDSYSINFO_YN(pESys,"is-capture-router", pDsn->bIsCaptureRouter);
    ADDSYSINFO_YN(pESys,"is-disk-storage", pDsn->m_bIsDiskStorage);
    ADDSYSINFO(pESys,"HeartBeatWarning", m_pMdbSqlParser->m_pDDLSqlStruct->pProAttr->iHeartBeatWarning);
    ADDSYSINFO(pESys,"HeartBeatFatal", m_pMdbSqlParser->m_pDDLSqlStruct->pProAttr->iHeartBeatFatal);
    ADDSYSINFO(pESys,"buf-size", pDsn->iLogBuffSize);
    ADDSYSINFO(pESys,"file-size", pDsn->iLogFileSize);
    ADDSYSINFO(pESys,"file-path", pDsn->sLogDir);
    //ADDSYSINFO(pESys,"local-standby-ip", pDsn->sPeerIP);
   // ADDSYSINFO(pESys,"local-standby-port", pDsn->iPeerPort);
   // ADDSYSINFO(pESys,"local-active-port", pDsn->iLocalPort);
    //ADDSYSINFO(pESys,"local-active-ip", pDsn->sLocalIP);
    //ADDSYSINFO(pESys,"peer-active-ip", pDsn->sActiveIP);
   // ADDSYSINFO(pESys,"peer-active-port", pDsn->iActivePort);
    //ADDSYSINFO(pESys,"peer-standby-ip", pDsn->sStandByIP);
   // ADDSYSINFO(pESys,"peer-standby-port", pDsn->iStandbyPort);
    ADDSYSINFO(pESys,"agent-port", pDsn->sAgentPortStr);
	
	ADDSYSINFO(pESys,"use-ntc-agent-port", pDsn->sNtcPortStr);
	ADDSYSINFO(pESys,"notuse-ntc-agent-port", pDsn->sNoNtcPortStr);
	
    ADDSYSINFO(pESys,"manager-size", pDsn->iManagerSize/(1024*1024));
    ADDSYSINFO(pESys,"data-size", pDsn->iDataSize/(1024*1024));
    ADDSYSINFO(pESys,"client-timeout", pDsn->iClientTimeout);
    ADDSYSINFO(pESys,"orarep-interval", pDsn->m_iOraRepInterval);
    ADDSYSINFO(pESys,"orarep-delaytime", pDsn->m_iOraRepDelaySec);
    ADDSYSINFO(pESys,"Is-Seq-Cache", pDsn->m_iSeqCacheSize);
    ADDSYSINFO(pESys,"routing-list", pDsn->sRoutingList);
    ADDSYSINFO(pESys,"routing-name", pDsn->sRoutingName);
    ADDSYSINFO(pESys,"long-sql-time", pDsn->m_iLongSqlTime);
    ADDSYSINFO(pESys,"rep-file-timeout", pDsn->m_iRepFileTimeout);
    ADDSYSINFO_YN(pESys,"is-shadow", pDsn->m_bShadow);
    ADDSYSINFO_YN(pESys,"is-shard-backup", pDsn->m_bIsShardBackup);
    ADDSYSINFO_YN(pESys,"is-single-disaster", pDsn->m_bSingleDisaster);
    ADDSYSINFO_YN(pESys,"is-null", pDsn->m_bNull);
	
	ADDSYSINFO_YN(pESys,"is-use-ntc", pDsn->m_bUseNTC);
	
    ADDSYSINFO(pESys,"CsPumpMaxCount", pDsn->m_iCSPumpMaxCount);
    ADDSYSINFO(pESys,"CsPumpInitCount", pDsn->m_iCSPumpInitCount);
    ADDSYSINFO(pESys,"CsPeerCountPerPump", pDsn->m_iCSPeerCountPerPump);
    ADDSYSINFO(pESys,"valid-ip", pDsn->m_sCSValidIP);
    ADDSYSINFO(pESys,"invalid-ip", pDsn->m_sCSInValidIP);
    ADDSYSINFO(pESys,"Local-Ip", pDsn->sLocalIP);
	if(pDsn->sLocalIP_active[0] != 0)
		ADDSYSINFO(pESys,"Local-Active-Ip", pDsn->sLocalIP_active);
	
    ADDSYSINFO_YN(pESys,"Is-Answer", pDsn->m_bSQLIsAnswer);
    ADDSYSINFO(pESys,"Rep-Server-ip", pDsn->sRepSvrIp);
    ADDSYSINFO(pESys,"Rep-Server-port", pDsn->iRepSvrPort);
    ADDSYSINFO(pESys,"Rep-standby-Server-ip", pDsn->sRepStandbySvrIp);
    ADDSYSINFO(pESys,"Rep-standby-Server-port", pDsn->iRepStandbySvrPort);
    ADDSYSINFO(pESys,"Rep-Local-port", pDsn->iRepLocalPort);
    ADDSYSINFO(pESys,"Rep-file-invalid-time", pDsn->iInvalidRepFileTime);
    ADDSYSINFO(pESys,"not-load-table-list", pDsn->m_strNotLoadFromDBList.c_str());

	ADDSYSINFO_YN(pESys,"Is-Reload-Ora", pDsn->m_bReloadOra);
    ADDSYSINFO_YN(pESys,"Is-Reload-Encrypt", pDsn->m_bReloadEncrypt);
    ADDSYSINFO(pESys,"Reload-Cfg-Name", pDsn->m_sReloadCfgName);
    ADDSYSINFO(pESys,"Reload-Db-Type", pDsn->m_sReloadDbType);
    
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  CreateDsn
* 函数描述	:  新增DSN
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::CreateDsn(TMdbCfgDSN  *pDsn)
{
    TADD_FUNC("Start.");
    int iRet =0;
    CHECK_OBJ(pDsn);
    if (m_sLastDSN[0] == '\0')
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM, "DSN is empty. Please specify a database by \'use database' command.");
        return -1;
    }
    int iPos = FindDsnEx(m_sLastDSN);
    if(iPos < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Dsn[%s] does not exist..",pDsn->sName);
        return -1;
    }
    m_tDsnMgr[iPos].m_pSysDocument->Clear();
    bool bExist = m_tDsnMgr[iPos].m_pSysDocument->LoadFile(m_tDsnMgr[iPos].m_sSysFileName);
    if(bExist)
    {
        CHECK_RET(ERR_DB_DSN_ALREADY_EXISTS,"Dsn[%s] already exist..",pDsn->sName);
        //文件已经存在，需要先备份再删除
        /*
        BakFile(m_tDsnMgr[iPos].m_sSysFileName);
        TMdbNtcFileOper::Remove(m_tDsnMgr[iPos].m_sSysFileName);
        m_tDsnMgr[iPos].m_pSysDocument->Clear();
        */
    }
    MDBXMLElement* pRoot = m_tDsnMgr[iPos].m_pSysDocument->FirstChildElement("MDBConfig");
    if(NULL != pRoot)
    {
        return iRet;
    }
    m_tDsnMgr[iPos].m_pSysRoot = new MDBXMLElement("MDBConfig");
    CHECK_OBJ(m_tDsnMgr[iPos].m_pSysRoot);
    m_tDsnMgr[iPos].m_pSysDocument->LinkEndChild(m_tDsnMgr[iPos].m_pSysRoot);
    MDBXMLElement *pDataSource = new MDBXMLElement("DataSource");
    CHECK_OBJ(pDataSource);
    MDBXMLElement *pESys = new MDBXMLElement("sys");
    CHECK_OBJ(pESys);
    m_tDsnMgr[iPos].m_pSysRoot->LinkEndChild(pDataSource);
    m_tDsnMgr[iPos].m_pSysRoot->LinkEndChild(pESys);
    AddSysInfo(pESys,pDsn);
    m_tDsnMgr[iPos].m_pCfgDSN = pDsn;
    m_tDsnMgr[iPos].m_bIsGenDSN[0] = true;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ModifyDsn
* 函数描述	:  修改DSN属性
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ModifyDsn(bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(m_pMdbSqlParser);
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    int iPos = GetSysRootNode(bIsDynamic);
    if(iPos < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"DSN[%s] does not exist, can not be modified.",m_sLastDSN);
        return ERR_DB_DSN_INVALID;
    }
    MDBXMLElement* pSection = NULL;
    for(int i=0;i<MAX_SQL_COUNTS;i++)
    {
        bool bIsExist = false;
        if(0 == m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrName[0])
        {
            continue;
        }
        MDBXMLElement* pSys = m_tDsnMgr[iPos].m_pSysRoot->FirstChildElement("sys");
        for (pSection=pSys->FirstChildElement("section"); pSection; pSection=pSection->NextSiblingElement("section"))
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pSection->Attribute("name"),\
                m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrName) == 0)
            {
                bIsExist = true;
                break;
            }
        }
        
        if(bIsExist)
        {
            pSection->SetAttribute("value",m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrValue);
        }
        else
        {
            TADD_ERROR(ERROR_UNKNOWN,"DSN[%s] has no the attribute[%s].",m_sLastDSN,\
                m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrName);
            return -1;
        }
    }
    //如果是动态修改DSN属性，直接修改文件
    if(bIsDynamic)
    {
        CHECK_RET(BakFile(m_tDsnMgr[iPos].m_sSysFileName),\
            "Failed to backup file[%s]",m_tDsnMgr[iPos].m_sSysFileName);
        m_tDsnMgr[iPos].m_pSysDocument->SaveFile(m_tDsnMgr[iPos].m_sSysFileName);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  DropDsn
* 函数描述	:  删除DSN文件，包括该DSN下的表文件
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::DropDsn(const char *pDsn)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pDsn);
	
    char   sDsnName[MAX_NAME_LEN] = {0};
    char   sConfigHome[MAX_PATH_NAME_LEN] = {0};
    SAFESTRCPY(sDsnName, MAX_NAME_LEN, pDsn);
    TMdbNtcStrFunc::ToUpper(sDsnName);
    snprintf(sConfigHome,sizeof(sConfigHome), "%s.%s",m_sSysFilePath,sDsnName);
    int iPos = FindDsnEx(pDsn);
    if(iPos >= 0)
    {
        //DSN文件存在则先备份
        //BakFile(m_tDsnMgr[iPos].m_sSysFileName);
        //TMdbNtcFileOper::Remove(m_tDsnMgr[iPos].m_sSysFileName);
        TMdbNtcDirOper::Remove(sConfigHome, true);
        m_tDsnMgr[iPos].Clear();
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  CreateTable
* 函数描述	:  新建表
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::CreateTable(TMdbTable* pTable,bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTable);
    char sRepType[MAX_NAME_LEN] = {0};
    char sDataType[MAX_NAME_LEN] = {0};
    if (m_sLastDSN[0] == '\0')
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM, "DSN is empty. Please specify a database by \'use database' command.");
        return -1;
    }
    //获取当前表配置信息
    int iDsnPos = FindDsnEx(m_sLastDSN);
    if(iDsnPos < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Invalid DSN[%s].",m_sLastDSN);
        return -1;
    }
    //检查加载sql、刷新sql是否有效
    if(!bIsDynamic)
    {//只检查静态执行时
        CHECK_RET(CheckSQLValidity(pTable->m_sLoadSQL),"CheckSQLValidity loadSQL failed");
        CHECK_RET(CheckSQLValidity(pTable->m_sFlushSQL),"CheckSQLValidity FlushSQL failed");
    }
    
    TTableScript *pTableScript = m_tDsnMgr[iDsnPos].GetTableByTableName(pTable->sTableName);
    CHECK_OBJ(pTableScript);
    pTableScript->m_pTableDocument->Clear();
    bool bExist = pTableScript->m_pTableDocument->LoadFile(pTableScript->m_sTableFileName);
    if(bExist)
    {
        CHECK_RET(ERR_TAB_CONFIG_TABLENAME_ALREADY_EXIST,"table[%s] already exists.",pTable->sTableName);
    
        //文件已经存在，需要先备份在删除
        //BakFile(pTableScript->m_sTableFileName);
        //TMdbNtcFileOper::Remove(pTableScript->m_sTableFileName);
    }
    pTableScript->m_pTableDocument->Clear();
    pTableScript->m_pTableRoot = new MDBXMLElement("MDBConfig");
    pTableScript->m_pTableDocument->LinkEndChild(pTableScript->m_pTableRoot);
    MDBXMLElement *pETable = new MDBXMLElement("table");
    pTableScript->m_pTableRoot->LinkEndChild(pETable);
    MDBXMLElement *pNameElement = new MDBXMLElement("name");
    //MDBXMLElement *pTableIdElement = new MDBXMLElement("table-id");
    MDBXMLElement *pTbSpaceElement = new MDBXMLElement("table-space");
    MDBXMLElement *pRecordsElement = new MDBXMLElement("record-counts");
    MDBXMLElement *pExpandElement = new MDBXMLElement("expand-record");
    MDBXMLElement *pIsZipTimeElement = new MDBXMLElement("Is-Zip-Time");
    MDBXMLElement *pRepTypeElement = new MDBXMLElement("rep-type");
    MDBXMLElement *pSharedBackupElement = new MDBXMLElement("shard-backup");
    MDBXMLElement *pTabLevelElement = new MDBXMLElement("table-level");
    pETable->LinkEndChild(pNameElement);
	if(pTable->m_iTableId != -1)
	{
		MDBXMLElement *pTableIdElement = new MDBXMLElement("table-id");
		pETable->LinkEndChild(pTableIdElement);
		pTableIdElement->SetAttribute("value",pTable->m_iTableId);
	}
    //pETable->LinkEndChild(pTableIdElement);
    pETable->LinkEndChild(pTbSpaceElement);
    pETable->LinkEndChild(pRecordsElement);
    pETable->LinkEndChild(pExpandElement);
    pETable->LinkEndChild(pIsZipTimeElement);
    pETable->LinkEndChild(pRepTypeElement);
    pETable->LinkEndChild(pSharedBackupElement);
    pETable->LinkEndChild(pTabLevelElement);

    pNameElement->SetAttribute("value",pTable->sTableName);
    if(strlen(pTable->m_sTableSpace) == 0)
    {
        pTbSpaceElement->SetAttribute("value",m_sTablespaceName);
    }
    else
    {
        pTbSpaceElement->SetAttribute("value",pTable->m_sTableSpace);
    }
    
    pRecordsElement->SetAttribute("value",pTable->iRecordCounts);
    pExpandElement->SetAttribute("value",pTable->iExpandRecords);
    pSharedBackupElement->SetAttribute("value",pTable->m_bShardBack?"Y":"N");
    pTabLevelElement->SetAttribute("value",pTable->iTableLevel);
    /*pIsZipTimeElement->SetAttribute("value",pTable->m_bIsZipTime?"Y":"N");*/
    char sTemp[2] = {0};
    sTemp[0] = pTable->m_cZipTimeType;
    pIsZipTimeElement->SetAttribute("value",sTemp);

    //表的同步属性:0-从Ora同步;1-向Ora同步;2-向备机同步;3-向Ora和备机同步，默认为0
    CHECK_RET(GetDataRepType(pTable->iRepAttr,sRepType,sizeof(sRepType)),"Synchronized table property illegally.");
    pRepTypeElement->SetAttribute("value",sRepType);
    MDBXMLElement *pColumnElement[MAX_COLUMN_COUNTS] = {NULL};
    for(int k = 0;k<pTable->iColumnCounts;k++)
    {
        pColumnElement[k] = new MDBXMLElement("column");
        pETable->LinkEndChild(pColumnElement[k]);
        //设置列名
        pColumnElement[k]->SetAttribute("name",pTable->tColumn[k].sName);
        //设置列位置
        pColumnElement[k]->SetAttribute("column-pos",pTable->tColumn[k].iPos);
        //设置列数据类型
        memset(sDataType,0,sizeof(sDataType));
        iRet = GetDataType(pTable->tColumn[k].iDataType,sDataType,sizeof(sDataType));
        if(0 == iRet)
        {
            pColumnElement[k]->SetAttribute("data-type",sDataType);
        }
        else 
        {
            cout<<"invalid data-type "<<pTable->tColumn[k].iDataType<<endl;
            pColumnElement[k]->SetAttribute("data-type","-1");
        }    
        //设置列长度
        pColumnElement[k]->SetAttribute("data-len",pTable->tColumn[k].iColumnLen);
        
        //设置缺省值和NULL
        pColumnElement[k]->SetAttribute("Is-Default",pTable->tColumn[k].bIsDefault?"Y":"N");
        pColumnElement[k]->SetAttribute("Default-Value",pTable->tColumn[k].iDefaultValue);
        pColumnElement[k]->SetAttribute("Null-able",pTable->tColumn[k].m_bNullable?"Y":"N");
    }
    
    MDBXMLElement *pIndexElement[MAX_INDEX_COUNTS] = {NULL};
    char sIndexColumPos[MAX_INDEX_COLUMN_COUNTS] = {0};        
    for(int k = 0; k<pTable->iIndexCounts;k++)
    {
        pIndexElement[k] = NULL;
        pIndexElement[k] = new MDBXMLElement("index");
        pETable->LinkEndChild(pIndexElement[k]);
        pIndexElement[k]->SetAttribute("name",pTable->tIndex[k].sName);
        for(int i=0;i<MAX_INDEX_COLUMN_COUNTS;i++)
        {
            if(pTable->tIndex[k].iColumnNo[i] <0)
            {
                break;
            }
            sprintf(sIndexColumPos+strlen(sIndexColumPos),"%d,",pTable->tIndex[k].iColumnNo[i]);
        }
        TMdbNtcStrFunc::Trim(sIndexColumPos,',');
        pIndexElement[k]->SetAttribute("column-pos",sIndexColumPos);
        pIndexElement[k]->SetAttribute("priority",pTable->tIndex[k].iPriority);
        pIndexElement[k]->SetAttribute("algo-type",pTable->tIndex[k].m_iAlgoType);
        pIndexElement[k]->SetAttribute("max-layer",pTable->tIndex[k].iMaxLayer);
    }

    MDBXMLElement *pKeyElement[10] = {NULL};
    for(int k =0;k<pTable->m_tPriKey.iColumnCounts;k++)
    {
        pKeyElement[k] = NULL;
        pKeyElement[k] = new MDBXMLElement("pkey");
        pETable->LinkEndChild(pKeyElement[k]);
        pKeyElement[k]->SetAttribute("column-pos",pTable->m_tPriKey.iColumnNo[k]);
    }


    MDBXMLElement *pReadElement = new MDBXMLElement("is-read-lock");
    MDBXMLElement *pWriteElement = new MDBXMLElement("is-write-lock");
    MDBXMLElement *pRollbacklement = new MDBXMLElement("is-rollback");
    MDBXMLElement *pPerfStateElement = new MDBXMLElement("is-PerfStat");
    MDBXMLElement *pPriKeyElement = new MDBXMLElement("checkPriKey");
    MDBXMLElement *pLoadTypeElement = new MDBXMLElement("LoadType");
    MDBXMLElement *pFilterElement = new MDBXMLElement("filter-sql");
    MDBXMLElement *pLoadElement = new MDBXMLElement("load-sql");
    MDBXMLElement *pFlushElement = new MDBXMLElement("flush-sql");
    //pETable->LinkEndChild(pNameElement);
    pETable->LinkEndChild(pReadElement);
    pETable->LinkEndChild(pWriteElement);
    pETable->LinkEndChild(pRollbacklement);
    pETable->LinkEndChild(pPerfStateElement);
    pETable->LinkEndChild(pPriKeyElement);
    pETable->LinkEndChild(pLoadTypeElement);
    pETable->LinkEndChild(pFilterElement);
    pETable->LinkEndChild(pLoadElement);
    pETable->LinkEndChild(pFlushElement);

    //pNameElement->SetAttribute("value",pTable->sTableName);
    pReadElement->SetAttribute("value",pTable->bReadLock?"Y":"N");
    pWriteElement->SetAttribute("value",pTable->bWriteLock?"Y":"N");
    pRollbacklement->SetAttribute("value",pTable->bRollBack?"Y":"N");
    pPerfStateElement->SetAttribute("value",pTable->bIsPerfStat?"Y":"N");
    pPriKeyElement->SetAttribute("value",pTable->bIsCheckPriKey?"Y":"N");
    pLoadTypeElement->SetAttribute("value",pTable->iLoadType);
    pFilterElement->SetAttribute("value",pTable->m_sFilterSQL);
    pLoadElement->SetAttribute("value",pTable->m_sLoadSQL);
    pFlushElement->SetAttribute("value",pTable->m_sFlushSQL);

    for(int i=0;i<pTable->iParameterCount;i++)
    {
        MDBXMLElement *ParmElement = new MDBXMLElement("Parameter");
        pETable->LinkEndChild(ParmElement);
        ParmElement->SetAttribute("name",pTable->tParameter[i].sName);
        if(pTable->tParameter[i].iDataType == DT_Int)
        {
            ParmElement->SetAttribute("data-type","NUMBER");
        }
        else if(pTable->tParameter[i].iDataType == DT_Char)
        {
            ParmElement->SetAttribute("data-type","CHAR");
        }
        else if(pTable->tParameter[i].iDataType == DT_VarChar)
        {
            ParmElement->SetAttribute("data-type","VARCHAR2");
        }
        else if(pTable->tParameter[i].iDataType == DT_DateStamp)
        {
            ParmElement->SetAttribute("data-type","DATESTAMP");
        }
        else if(pTable->tParameter[i].iDataType == DT_Blob)
        {
            ParmElement->SetAttribute("data-type","BLOB");
        }
        else
        {
            TADD_ERROR(ERROR_UNKNOWN,"Column[%s] data type is not correct.",pTable->tParameter[i].sName);
            ParmElement->SetAttribute("data-type"," ");
        }
        
        ParmElement->SetAttribute("value",pTable->tParameter[i].sValue);
        ParmElement->SetAttribute("parameter-type",pTable->tParameter[i].iParameterType);
    }
    if(!TMdbNtcDirOper::IsExist(pTableScript->m_sTabFilePath))
    {
        bool bCreate = TMdbNtcDirOper::MakeFullDir(pTableScript->m_sTabFilePath);
        if(!bCreate)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Mkdir(%s) failed.",pTableScript->m_sTabFilePath);
                return ERR_OS_CREATE_DIR;
        } 
    }
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
    CHECK_RET(ReplaceSpecialCharactersForSQL(pTableScript->m_sTableFileName),"ReplaceSpecialCharacters failed.");
    
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  GetTableRootNode
* 函数描述	:  获取表文件中的MDBConfig节点
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int  TMdbScript::GetTableRootNode(const char *pTableName,TTableScript * &pTableScript,bool bIsDynamic)
{
    TADD_FUNC("Start.");
    CHECK_OBJ(pTableName);
    int iRet = 0;
    if (m_sLastDSN[0] == '\0')
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM, "DSN is empty. Please specify a database by \'use database' command.");
        return -1;
    }
    int iDsnPos = FindDsnEx(m_sLastDSN);
    if(iDsnPos < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Invalid DSN[%s].",m_sLastDSN);
        return -1;
    }
    pTableScript = NULL;
    pTableScript = m_tDsnMgr[iDsnPos].GetTableByTableName(pTableName);
    CHECK_OBJ(pTableScript);
    pTableScript->m_pTableDocument->Clear();
    bool bExist = pTableScript->m_pTableDocument->LoadFile(pTableScript->m_sTableFileName);
    if(!bExist)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Load Table(%s) Info Failed,Check if you have created it! ",pTableName);
        return -1;
    }
    pTableScript->m_pTableRoot = pTableScript->m_pTableDocument->FirstChildElement("MDBConfig");
    CHECK_OBJ(pTableScript->m_pTableRoot);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  AddIndex
* 函数描述	:  新增索引
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::AddIndex(bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iPos = -1;
    char sIndexColumPos[MAX_INDEX_COLUMN_COUNTS] = {0};
    MDBXMLElement* pTableEle = NULL;
    CHECK_OBJ(m_pMdbSqlParser);
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    TMdbTable* pTable = m_pMdbSqlParser->m_pDDLSqlStruct->pTable;
    CHECK_OBJ(pTable);
    TTableScript * pTableScript = NULL;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,bIsDynamic),"GetTableRootNode failed.");
    pTableEle = pTableScript->m_pTableRoot->FirstChildElement("table");
    CHECK_OBJ(pTableEle);
    int iIndexPos = pTable->iIndexCounts-1;
    //检查增加的索引是否已经存在
    for(MDBXMLElement* pEle=pTableEle->FirstChildElement("index");pEle;pEle=pEle->NextSiblingElement("index"))
    {
        if(NULL == pEle->Attribute("name"))
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pTable->tIndex[iIndexPos].sName) == 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Index [%s] already exists.",pTable->sTableName);
            return ERR_TAB_INDEX_ALREADY_EXIST;
        }   
    }
    MDBXMLElement *pIndexElement = new MDBXMLElement("index");
    CHECK_OBJ(pIndexElement);
    pTableEle->LinkEndChild(pIndexElement);
    pIndexElement->SetAttribute("name",pTable->tIndex[iIndexPos].sName);
    if(bIsDynamic)
    {
        for(int j = 0; j<MAX_INDEX_COLUMN_COUNTS; j++)
        {
            iPos = pTable->tIndex[iIndexPos].iColumnNo[j];
            if(iPos < 0)
            {
                break;
            }
            snprintf(sIndexColumPos+strlen(sIndexColumPos),sizeof(sIndexColumPos),"%d,",iPos);
        }
    }
    else
    {
        for(int i=0;i<MAX_INDEX_COLUMN_COUNTS;i++)
        {
            if(0 == m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrName[0])
            {
                continue;
            }
    		iPos = GeColumPosByName(pTableEle,m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrName);
            if(iPos < 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Index[%s] column[%s] does not exist.",\
                    pTable->tIndex[iIndexPos].sName,m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrName);
                return -1;
            }
            snprintf(sIndexColumPos+strlen(sIndexColumPos),sizeof(sIndexColumPos),"%d,",iPos);
            TADD_DETAIL("sAttrName = %s,iPos = %d.",m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrName,iPos);
        }
    }
    
    TMdbNtcStrFunc::Trim(sIndexColumPos,',');
    pIndexElement->SetAttribute("column-pos",sIndexColumPos);
    pIndexElement->SetAttribute("priority",1);
    pIndexElement->SetAttribute("algo-type",pTable->tIndex[iIndexPos].m_iAlgoType);
    pIndexElement->SetAttribute("max-layer",pTable->tIndex[iIndexPos].iMaxLayer);

    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s].",pTableScript->m_sTableFileName);
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  GeColumPosByName
* 函数描述	:  通过列名获取列的位置
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  列在表中的位置
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::GeColumPosByName(MDBXMLElement* pTableRoot,const char *pColumnName)
{
    int iRet = -1;
    CHECK_OBJ(pTableRoot);
    CHECK_OBJ(pColumnName);
    for(MDBXMLElement* pEle=pTableRoot->FirstChildElement("column");pEle;pEle=pEle->NextSiblingElement("column"))
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pColumnName) ==0)
        {
            return atoi(pEle->Attribute("column-pos"));
        }    
    }
    return iRet;
}

/******************************************************************************
* 函数名称	:  DropIndex
* 函数描述	:  删除索引
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::DropIndex(bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bIsExist = false;
    MDBXMLElement* pIndexEle = NULL;
    MDBXMLElement* pTableEle = NULL;
    CHECK_OBJ(m_pMdbSqlParser);
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    TMdbTable* pTable = m_pMdbSqlParser->m_pDDLSqlStruct->pTable;
    TTableScript * pTableScript = NULL;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,bIsDynamic),"GetTableRootNode failed.");
    pTableEle = pTableScript->m_pTableRoot->FirstChildElement("table");
    //检查删除的索引是否已经存在
    for(pIndexEle=pTableEle->FirstChildElement("index");pIndexEle;\
            pIndexEle=pIndexEle->NextSiblingElement("index"))
    {
        TADD_DETAIL("pIndexName = %s,name=%s.",m_pMdbSqlParser->m_pDDLSqlStruct->sIndexName,pIndexEle->Attribute("name"));
        if(TMdbNtcStrFunc::StrNoCaseCmp(pIndexEle->Attribute("name"),m_pMdbSqlParser->m_pDDLSqlStruct->sIndexName) == 0)
        {
            bIsExist = true;
            break;
        }   
    }

    if(bIsExist)
    {
        pTableEle->RemoveChild(pIndexEle);
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"Index[%s] does not exists.",m_pMdbSqlParser->m_pDDLSqlStruct->sIndexName);
        return ERR_TAB_INDEX_NOT_EXIST;
    }
    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* 函数名称	:  AddPrimaryKey
* 函数描述	:  增加主键
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::AddPrimaryKey(TMdbTable* pTable)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iPos = -1;
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    MDBXMLElement* pTableEle = NULL;
    TTableScript * pTableScript = NULL;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,false),"GetTableRootNode failed.");
    pTableEle = pTableScript->m_pTableRoot->FirstChildElement("table");
    for(int i=0;i<MAX_PRIMARY_KEY_CC;i++)
    {
        if( 0 == m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrName[0])
        {
            continue;
        }
        //检查主键列在表中是否存在
        iPos = GeColumPosByName(pTableEle,m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrName);
        if(iPos < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Primary key column[%s] does not exist.",\
                       m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i].sAttrName);
            return -1;
        }
        
        //检查增加的主键是否已经存在
        for(MDBXMLElement* pEle=pTableEle->FirstChildElement("pkey");pEle;pEle=pEle->NextSiblingElement("pkey"))
        {
            if(NULL == pEle->Attribute("column-pos"))
            {
                continue;
            }
            
            if(iPos == atoi(pEle->Attribute("column-pos")))
            {
                TADD_ERROR(ERROR_UNKNOWN,"The table's[%s] primary key already exists.",pTable->sTableName);
                return ERR_TAB_PK_CONFLICT;
            }
        }

        //添加主键
        MDBXMLElement *pKeyElement = new MDBXMLElement("pkey");
        pTableEle->LinkEndChild(pKeyElement);
        pKeyElement->SetAttribute("column-pos",iPos);
    }
    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  AddColumn
* 函数描述	:  新增列
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::AddColumn(TMdbTable* pTable,bool bIsDynamic, bool bWriteUpdate)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iColumCount = 0;
    MDBXMLElement* pEle = NULL;
    TTableScript * pTableScript = NULL;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,bIsDynamic),"GetTableRootNode failed.");
    int iColumPos = pTable->iColumnCounts-1;
    //检查增加的列是否已经存在
    MDBXMLElement* pTableEle = pTableScript->m_pTableRoot->FirstChildElement("table");
    CHECK_OBJ(pTableEle);
    for(pEle=pTableEle->FirstChildElement("column");pEle;pEle=pEle->NextSiblingElement("column"))
    {
        if(NULL == pEle->Attribute("name"))
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pTable->tColumn[iColumPos].sName) == 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Column [%s] already exists.",pTable->tColumn[iColumPos].sName);
            return ERR_TAB_COLUMN_ALREADY_EXIST;
        }  
        iColumCount++;
    }
    
    MDBXMLElement *pColumnElement = new MDBXMLElement("column");
    pTableEle->LinkEndChild(pColumnElement);
    pColumnElement->SetAttribute("name",pTable->tColumn[iColumPos].sName);
    pColumnElement->SetAttribute("column-pos",iColumCount);
    if(pTable->tColumn[iColumPos].iDataType == 1)
    {
        pColumnElement->SetAttribute("data-type","NUMBER");
    }
    else if(pTable->tColumn[iColumPos].iDataType == 2)
    {
        pColumnElement->SetAttribute("data-type","CHAR");
    }
    else if(pTable->tColumn[iColumPos].iDataType == 3)
    {
        pColumnElement->SetAttribute("data-type","VARCHAR");
    }
    else if(pTable->tColumn[iColumPos].iDataType ==4)
    {
        pColumnElement->SetAttribute("data-type","DATESTAMP");
    }
    else if(pTable->tColumn[iColumPos].iDataType ==9)
    {
        pColumnElement->SetAttribute("data-type","BLOB");
    }
    else 
    {
        TADD_DETAIL("invalid data-type = %d.",pTable->tColumn[iColumPos].iDataType);
        pColumnElement->SetAttribute("data-type","-1");
    }    
    pColumnElement->SetAttribute("data-len",pTable->tColumn[iColumPos].iColumnLen);   
    pColumnElement->SetAttribute("Is-Default",pTable->tColumn[iColumPos].bIsDefault?"Y":"N");
    pColumnElement->SetAttribute("Default-Value",pTable->tColumn[iColumPos].iDefaultValue);
    pColumnElement->SetAttribute("Null-able",pTable->tColumn[iColumPos].m_bNullable?"Y":"N"); 
    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);

    // 写增加列的变更信息
    if(bWriteUpdate)
	{
		CHECK_RET(WriteAddColumn(pTableScript, pTable->tColumn[iColumPos].sName, iColumCount),"WriteAddColumn failed.");
	}
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ModifyColumn
* 函数描述	:  修改列属性
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ModifyColumn(bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bIsExist = false;
    MDBXMLElement* pEle = NULL;
    char sType[MAX_NAME_LEN] = {0};
    CHECK_OBJ(m_pMdbSqlParser);
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    TMdbTable* pTable = m_pMdbSqlParser->m_pDDLSqlStruct->pTable;
    CHECK_OBJ(pTable);
    TTableScript * pTableScript = NULL;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,bIsDynamic),"GetTableRootNode failed.");
    MDBXMLElement* pTableEle = pTableScript->m_pTableRoot->FirstChildElement("table");
    CHECK_OBJ(pTableEle);
    int iModifyColum = m_pMdbSqlParser->m_pDDLSqlStruct->iModifyColumnPos;

    int iOldDataType = 0;
    int iOldDataLen = 0;
    //检查增加的索引是否已经存在
    for(pEle=pTableEle->FirstChildElement("column");pEle;pEle=pEle->NextSiblingElement("column"))
    {
        if(NULL == pEle->Attribute("name"))
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pTable->tColumn[iModifyColum].sName) == 0)
        {
            bIsExist = true;
            iOldDataType = atoi(pEle->Attribute("data-type"));
            iOldDataLen = atoi(pEle->Attribute("data-len"));
            break;
        }  
    }

    if(bIsExist)
    {
        std::vector<string>::iterator itor = m_pMdbSqlParser->m_pDDLSqlStruct->vModifyColumnAttr.begin();
        for(;itor != m_pMdbSqlParser->m_pDDLSqlStruct->vModifyColumnAttr.end();++itor)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(itor->c_str(),"data-type") == 0)
            {
                iRet = GetDataType(pTable->tColumn[iModifyColum].iDataType,sType,sizeof(sType));
                CHECK_RET(iRet,"The column data type[%d] is invalid.",pTable->tColumn[iModifyColum].iDataType);
                pEle->SetAttribute("data-type",sType);
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(itor->c_str(),"data-len") == 0)
            {
                if((pTable->tColumn[iModifyColum].iDataType == 2 
                    || pTable->tColumn[iModifyColum].iDataType == 3)
                    && pTable->tColumn[iModifyColum].iColumnLen < atoi(pEle->Attribute("data-len")))
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Modifying the length of the column[%d] is less than the original length[%s].",\
                            pTable->tColumn[iModifyColum].iColumnLen ,pEle->Attribute("data-len"));
                    return ERR_TAB_COLUMN_NOT_EXIST;
                }
                pEle->SetAttribute("data-len", pTable->tColumn[iModifyColum].iColumnLen);
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(itor->c_str(),"Default-Value") == 0)
            {
                pEle->SetAttribute("Default-Value", pTable->tColumn[iModifyColum].iDefaultValue);
                pEle->SetAttribute("Is-Default", "Y");
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(itor->c_str(),"Null-able") == 0)
            {
                pEle->SetAttribute("Null-able", pTable->tColumn[iModifyColum].m_bNullable?"Y":"N");
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"Column attribute [%s] does not exists.",\
                            itor->c_str());
                return ERR_TAB_COLUMN_NOT_EXIST;
            }  
        }
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"Column [%s] does not exists.",pTable->tColumn[iModifyColum].sName);
        return ERR_TAB_COLUMN_NOT_EXIST;
    }
    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);

    if( iOldDataType != pTable->tColumn[iModifyColum].iDataType)
    {
        CHECK_RET(WriteColmDataTypeAlterInfo(pTableScript, pTable->tColumn[iModifyColum].sName, iOldDataType, pTable->tColumn[iModifyColum].iDataType)
                ,"WriteColmDataTypeAlterInfo failed.");
    }

    if(iOldDataLen != pTable->tColumn[iModifyColum].iColumnLen)
    {
        CHECK_RET(WriteColmDataLenAlterInfo(pTableScript, pTable->tColumn[iModifyColum].sName, iOldDataLen, pTable->tColumn[iModifyColum].iColumnLen)
                ,"WriteColmDataLenAlterInfo failed.");
    }
    
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  GetDataType
* 函数描述	:  获取数据类型的字符串表示
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::GetDataType(int iDataType,char *pDataType,const int iLen)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pDataType);
    switch(iDataType)
    {
        case DT_Int:
            SAFESTRCPY(pDataType,iLen,"NUMBER");
            break;
        case DT_Char:
            SAFESTRCPY(pDataType,iLen,"CHAR");
            break;
        case DT_VarChar:
            SAFESTRCPY(pDataType,iLen,"VARCHAR");
            break;
        case DT_DateStamp:
            SAFESTRCPY(pDataType,iLen,"DATESTAMP");
            break;
        case DT_Blob:
            SAFESTRCPY(pDataType,iLen,"BLOB");
            break;  
        default:
            iRet = -1;
            break;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  GetDataRepType
* 函数描述	:  获取数据同步类型的字符串表示
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::GetDataRepType(int iRepType,char *pRepType,const int iLen)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pRepType);
    switch(iRepType)
    {
        case REP_FROM_DB:
            SAFESTRCPY(pRepType,iLen,REP_DB2MDB);
            break;
        case REP_TO_DB:
            SAFESTRCPY(pRepType,iLen,REP_MDB2DB);
            break;
        case REP_NO_REP:
            SAFESTRCPY(pRepType,iLen,REP_NoRep);
            break;
        default:
            iRet = -1;
            break;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  DropColumn
* 函数描述	:  删除表列
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::DropColumn(bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bIsExist = false;
    int iColumnPos = -1;
    CHECK_OBJ(m_pMdbSqlParser);
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    MDBXMLElement* pEle = NULL;
    TTableScript * pTableScript = NULL;
    TMdbTable* pTable = m_pMdbSqlParser->m_pDDLSqlStruct->pTable;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,bIsDynamic),"GetTableRootNode failed.");
    MDBXMLElement* pTableEle = pTableScript->m_pTableRoot->FirstChildElement("table");
    CHECK_OBJ(pTableEle);
    //检查删除的列是否存在
    for(pEle=pTableEle->FirstChildElement("column");pEle;pEle=pEle->NextSiblingElement("column"))
    {
        if(NULL == pEle->Attribute("name"))
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),m_pMdbSqlParser->m_pDDLSqlStruct->vModifyColumnAttr[0].c_str()) == 0)
        {
            iColumnPos = atoi(pEle->Attribute("column-pos"));
            bIsExist = true;
            break;
        }  
    }
    if(bIsExist)
    {
        //如果删除的列为主键列，则报错退出
        for(MDBXMLElement* pKeyEle=pTableEle->FirstChildElement("pkey");pKeyEle;pKeyEle=pKeyEle->NextSiblingElement("pkey"))
        {
            if(NULL == pKeyEle->Attribute("column-pos"))
            {
                continue;
            }
            
            if(iColumnPos == atoi(pKeyEle->Attribute("column-pos")))
            {
                TADD_ERROR(ERROR_UNKNOWN,"The primary key column[%s] cannot be dropped.",\
                    m_pMdbSqlParser->m_pDDLSqlStruct->vModifyColumnAttr[0].c_str());
                return ERR_SQL_INVALID;
            }
        }
        pTableEle->RemoveChild(pEle);
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"Column[%s] of the table[%s] does not exist.",\
            m_pMdbSqlParser->m_pDDLSqlStruct->vModifyColumnAttr[0].c_str(),\
            m_pMdbSqlParser->m_pDDLSqlStruct->pTable->sTableName);
        return ERR_TAB_COLUMN_NOT_EXIST;
    }
    //调整删除列后续列的column-pos
    pEle = NULL;
    int iSearchPos = -1;
    for(pEle=pTableEle->FirstChildElement("column");pEle;pEle=pEle->NextSiblingElement("column"))
    {
        if(NULL == pEle->Attribute("name"))
        {
            continue;
        }
        iSearchPos = atoi(pEle->Attribute("column-pos"));
        if(iSearchPos > iColumnPos)
        {
            iSearchPos =  iSearchPos - 1;
            pEle->SetAttribute("column-pos",iSearchPos);
        }
    }
    //如果删除的列为索引列需要调整
    CHECK_RET(DeleteSpecifiedIndexColumn(pTableEle,iColumnPos),"DeleteSpecifiedIndexColumn failed.");
    //备份修改前的表结构文件
    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
	CHECK_RET(WriteDropColumn(pTableScript, m_pMdbSqlParser->m_pDDLSqlStruct->vModifyColumnAttr[0].c_str(), iColumnPos),"WriteDropColumn failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  DropTable
* 函数描述	:  删除指定表文件
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::DropTable(const char *pTableName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTableName);
    TTableScript * pTableScript = NULL;
    CHECK_RET(GetTableRootNode(pTableName,pTableScript,false),"GetTableRootNode failed.");

    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
    TMdbNtcFileOper::Remove(pTableScript->m_sTableFileName);

    CHECK_RET(BakFile(pTableScript->m_sTabAlterFile),"Failed to backup file[%s]",pTableScript->m_sTabAlterFile);
    TMdbNtcFileOper::Remove(pTableScript->m_sTabAlterFile);

    TMdbNtcDirOper::Remove(pTableScript->m_sTabFilePath, true);
    
    pTableScript->Clear();
    //删除BAK
    char  sBAKTabDir[MAX_PATH_NAME_LEN] = {0};
    char  sTabName[MAX_NAME_LEN] = {0};
	char sDsnName[MAX_NAME_LEN] = {0};
    SAFESTRCPY(sTabName, sizeof(sTabName), pTableName);
	SAFESTRCPY(sDsnName, sizeof(sDsnName), m_sLastDSN);
    TMdbNtcStrFunc::ToUpper(sTabName);
	TMdbNtcStrFunc::ToUpper(sDsnName);
    snprintf(sBAKTabDir,sizeof(sBAKTabDir), "%s.%s/.BAK/.TABLE/.%s/",m_sSysFilePath,sDsnName,sTabName);
    if(TMdbNtcDirOper::IsExist(sBAKTabDir))
    {
        TMdbNtcDirOper::Remove(sBAKTabDir, true);
    }
    TADD_FUNC("Finish.");
    return iRet;
}



/******************************************************************************
* 函数名称	:  AddSingleTableAttribute
* 函数描述	:  增加表属性
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::AddSingleTableAttribute(TMdbTable* pTable)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TTableScript * pTableScript = NULL;
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,false),"GetTableRootNode failed.");
    for(MDBXMLElement* pEle=pTableScript->m_pTableRoot->FirstChildElement("table"); pEle; pEle=pEle->NextSiblingElement("table"))
    {
        if(NULL != pEle->FirstChildElement(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr))
        {
            TADD_ERROR(ERROR_UNKNOWN,"Table attribute [%s] already exists.",m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr);
            return -1;
        }
    }
    
    MDBXMLElement* pEle = pTableScript->m_pTableRoot->FirstChildElement("table");
    MDBXMLElement *pElement = new MDBXMLElement(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr);
    pEle->LinkEndChild(pElement);
    pElement->SetAttribute("value",m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue);
    
    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
    CHECK_RET(ReplaceSpecialCharactersForSQL(pTableScript->m_sTableFileName),"ReplaceSpecialCharacters failed.");

    if(0 == TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr, "Is-Zip-Time"))
    {
        CHECK_RET(WriteTabZipTimeAlterInfo(pTableScript, "N", m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue),"WriteTabZipTimeAlterInfo failed.");
    }

    
    
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ModifyTableAttribute
* 函数描述	:  修改表属性
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ModifyTableAttribute(bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet =0;
    bool bIsExist = false;
    MDBXMLElement* pEAttribute = NULL;
    CHECK_OBJ(m_pMdbSqlParser);
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    TMdbTable* pTable = m_pMdbSqlParser->m_pDDLSqlStruct->pTable;
    CHECK_OBJ(pTable);
    //如果修改的属性在属性表中找不到，需要在到表结构文件中查找
    TTableScript * pTableScript = NULL;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,bIsDynamic),"GetTableRootNode failed.");
    for(MDBXMLElement* pEle=pTableScript->m_pTableRoot->FirstChildElement("table"); pEle; pEle=pEle->NextSiblingElement("table"))
    {
        pEAttribute = pEle->FirstChildElement(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr);
        if(NULL != pEAttribute)
        {
            bIsExist = true;
            break;
        }
    }
    
    if(bIsExist)
    {
        if(!bIsDynamic
            &&(0 == TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr, "load-sql")
                ||0 == TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr, "flush-sql")
               )
           )
        {
            CHECK_RET(CheckSQLValidity(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue),"CheckSQLValidity failed");
        }
        std::string sOldValue =  pEAttribute->Attribute("value");
        pEAttribute->SetAttribute("value", m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue);
        CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
        pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
        CHECK_RET(ReplaceSpecialCharactersForSQL(pTableScript->m_sTableFileName),"ReplaceSpecialCharacters failed.");

        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr, "table-space"))
        {
            CHECK_RET(WriteTabTsAlterInfo(pTableScript, sOldValue.c_str(), m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue),"WriteTabZipTimeAlterInfo failed.");
        }

        if(0 == TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr, "Is-Zip-Time"))
        {
            CHECK_RET(WriteTabZipTimeAlterInfo(pTableScript, sOldValue.c_str(), m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue),"WriteTabZipTimeAlterInfo failed.");
        }
    }
    else
    {   
        TADD_ERROR(ERROR_UNKNOWN,"Table attribule[%s] does not exist.",m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr);
        return -1;
    }
    TADD_FUNC("Finish.");
    return iRet;
}



/******************************************************************************
* 函数名称	:  DropTableAttribute
* 函数描述	:  删除表属性
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::DropTableAttribute(TMdbTable* pTable)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTable);
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    bool bIsExist = false;
    MDBXMLElement* pEle=NULL;
    MDBXMLElement* pAttrEle=NULL;
    //找到MDBConfig节点
    TTableScript * pTableScript = NULL;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,false),"GetTableAttrRootNode failed.");
    //检查删除的表属性是否存在
    for(pEle=pTableScript->m_pTableRoot->FirstChildElement("table"); pEle; pEle=pEle->NextSiblingElement("table"))
    {
        pAttrEle= pEle->FirstChildElement(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr);
        if(NULL != pAttrEle)
        {
            bIsExist = true;
            break;
        }
    }
    
    if(bIsExist)
    {
        pEle->RemoveChild(pAttrEle);
        CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
        pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
        CHECK_RET(ReplaceSpecialCharactersForSQL(pTableScript->m_sTableFileName),"ReplaceSpecialCharacters failed.");
    }
    else
    {
          TADD_ERROR(ERROR_UNKNOWN,"Table attribule[%s] does not exist.",m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr);
           return -1;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  AddFlushOrLoadSQLParam
* 函数描述	:  增加表属性配置文件中的指定的parameter节点信息
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::AddFlushOrLoadSQLParam(TMdbTable* pTable,bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTable);
    MDBXMLElement* pEle = NULL;
    TTableScript * pTableScript = NULL;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,bIsDynamic),"GetTableAttrRootNode failed.");
    //如何参数已经存在，则报错返回
    MDBXMLElement* pTableEle = pTableScript->m_pTableRoot->FirstChildElement("table");
    CHECK_OBJ(pTableEle);
    for(pEle=pTableEle->FirstChildElement("Parameter");pEle;pEle=pEle->NextSiblingElement("Parameter"))
    {
        if(NULL == pEle->Attribute("name"))
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pTable->tParameter[pTable->iParameterCount-1].sName) == 0
            && atoi(pEle->Attribute("parameter-type")) == pTable->tParameter[pTable->iParameterCount-1].iParameterType)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Parameter [%s] already exists.",pTable->tParameter[pTable->iParameterCount-1].sName);
            return ERR_APP_INVALID_PARAM;
        }
    }
    //增加Parameter节点
    MDBXMLElement *pParamElement = new MDBXMLElement("Parameter");
    pTableEle->LinkEndChild(pParamElement);
    pParamElement->SetAttribute("name",pTable->tParameter[pTable->iParameterCount-1].sName);
    char sDataType[MAX_NAME_LEN] = {0};
    GetDataType(pTable->tParameter[pTable->iParameterCount-1].iDataType,sDataType,sizeof(sDataType));
    pParamElement->SetAttribute("data-type",sDataType);
    pParamElement->SetAttribute("value",pTable->tParameter[pTable->iParameterCount-1].sValue);
    pParamElement->SetAttribute("parameter-type",pTable->tParameter[pTable->iParameterCount-1].iParameterType);
    //重新生成表属性配置文件
    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
    CHECK_RET(ReplaceSpecialCharactersForSQL(pTableScript->m_sTableFileName),"ReplaceSpecialCharacters failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  DropFlushOrLoadSQLParam
* 函数描述	:  删除表属性
* 输入		:  删除表属性配置文件中的指定的parameter节点
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::DropFlushOrLoadSQLParam(TMdbTable* pTable,const char *pParamName,bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bFindParam = false;
    MDBXMLElement* pEle = NULL;
    CHECK_OBJ(pTable);
    CHECK_OBJ(pParamName);
    TTableScript * pTableScript = NULL;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,bIsDynamic),"GetTableAttrRootNode failed.");
    //如何参数已经存在，则报错返回
    MDBXMLElement* pTableEle = pTableScript->m_pTableRoot->FirstChildElement("table");
    CHECK_OBJ(pTableEle);
    for(pEle=pTableEle->FirstChildElement("Parameter");pEle;pEle=pEle->NextSiblingElement("Parameter"))
    {
        if(NULL == pEle->Attribute("name"))
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pParamName) == 0)
        {
            bFindParam = true;
            break;
        }
    }
    //如果修改的参数不存在，则退出，不写文件
    if(!bFindParam)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Parameter [%s] does not exist in the file[%s].",pParamName,pTableScript->m_sTableFileName);
        return ERR_APP_INVALID_PARAM;
    }
    //删除Parameter节点
    pTableEle->RemoveChild(pEle);
    //重新生成表属性配置文件
    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
    CHECK_RET(ReplaceSpecialCharactersForSQL(pTableScript->m_sTableFileName),"ReplaceSpecialCharacters failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ModifyFlushOrLoadSQLParam
* 函数描述	:  修改flush-sql or load-sql中的动态参数
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ModifyFlushOrLoadSQLParam(bool bIsDynamic)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bFindParam = false;
    MDBXMLElement* pEle = NULL;
    CHECK_OBJ(m_pMdbSqlParser);
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    TTableScript * pTableScript = NULL;
    TMdbTable* pTable = m_pMdbSqlParser->m_pDDLSqlStruct->pTable;
    CHECK_RET(GetTableRootNode(pTable->sTableName,pTableScript,bIsDynamic),"GetTableAttrRootNode failed.");
    //如何参数已经存在，则报错返回
    MDBXMLElement* pTableEle = pTableScript->m_pTableRoot->FirstChildElement("table");
    CHECK_OBJ(pTableEle);
    for(pEle=pTableEle->FirstChildElement("Parameter");pEle;pEle=pEle->NextSiblingElement("Parameter"))
    {
        if(NULL == pEle->Attribute("name"))
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),m_pMdbSqlParser->m_pDDLSqlStruct->tParam.sName) == 0)
        {
            bFindParam = true;
            break;
        }
    }
    //如果修改的参数不存在，则退出，不写文件
    if(!bFindParam)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Modification of the parameter[%s] does not exist in the file[%s].",\
                m_pMdbSqlParser->m_pDDLSqlStruct->tParam.sName,pTableScript->m_sTableFileName);
        return ERR_APP_INVALID_PARAM;
    }
    
    //修改Parameter节点
    char sDataType[MAX_NAME_LEN] = {0};
    if(m_pMdbSqlParser->m_pDDLSqlStruct->tParam.iDataType > 0)
    {
        GetDataType(m_pMdbSqlParser->m_pDDLSqlStruct->tParam.iDataType,sDataType,sizeof(sDataType));
        pEle->SetAttribute("data-type",sDataType);
    }
    
    if(m_pMdbSqlParser->m_pDDLSqlStruct->tParam.sValue[0] != 0)
    {
        pEle->SetAttribute("value",m_pMdbSqlParser->m_pDDLSqlStruct->tParam.sValue);
    }
    
    if(m_pMdbSqlParser->m_pDDLSqlStruct->tParam.iParameterType >= 0)
    {
        pEle->SetAttribute("parameter-type",m_pMdbSqlParser->m_pDDLSqlStruct->tParam.iParameterType);
    }
    //重新生成表属性配置文件
    CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
    pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
    CHECK_RET(ReplaceSpecialCharactersForSQL(pTableScript->m_sTableFileName),"ReplaceSpecialCharacters failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  FindDsnEx
* 函数描述	:  通过DSN获取其对应的管理对象位置
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  DSN管理对象的位置
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::FindDsnEx(const char *pszDSN)
{
    TADD_FUNC("Start.");
    CHECK_OBJ(pszDSN);
    if (strlen(pszDSN) == 0)
    {
         TADD_ERROR(ERR_APP_INVALID_PARAM, "DSN is empty. Please specify a database by \'use database' command.");
         return ERR_APP_INVALID_PARAM;
    }
    int iPos = -1;
    for(int i = 0;i<MAX_DSN_COUNTS;i++)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(m_tDsnMgr[i].m_sDsnName,pszDSN) == 0)
        {
            iPos = i;
            break;
        }
    }
    TADD_FUNC("Finish.");
    return iPos;
}

/******************************************************************************
* 函数名称	:  SetDsnEx
* 函数描述	:  设置指定DSN
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  DSN管理对应的位置
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::SetDsnEx(const char *pszDSN)
{
    TADD_FUNC("Start.");
    CHECK_OBJ(pszDSN);
    int iRet = 0;
    for(int i = 0;i<MAX_DSN_COUNTS;i++)
    {
        if(0 == m_tDsnMgr[i].m_sDsnName[0])
        {
            m_tDsnMgr[i].Clear();
            CHECK_RET(m_tDsnMgr[i].Init(pszDSN),"Init failed,Dsn-name = [%s].",pszDSN);
            iRet = i;
            break;
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* 函数名称	:  AddSequenceToOracle
* 函数描述	:  增加oracle侧mdb_sequence表中记录
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::AddSequenceToOracle()
{
    TADD_FUNC("Begin."); 
    int iRet = 0;
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    //校验当前序列值是否为起始步长+N*步长
    TMemSeq &tMemSeq = m_pMdbSqlParser->m_pDDLSqlStruct->tMemSeq;
    if(tMemSeq.iCur !=0 && (tMemSeq.iCur - tMemSeq.iStart)%tMemSeq.iStep != 0)
    {
        TADD_WARNING("Suggest the current sequence[%s] value is equal to the starting value plus n times step value.",\
            tMemSeq.sSeqName);
    }
    //检查序列是否已经存在
    bool bExist = false;
    CHECK_RET(LoadDSNCofig(),"Load the DSN configuration file failed.");
    CHECK_RET(CheckAndOperSequence(tMemSeq,bExist),"CheckSequence falied");
    if(bExist)
    {
        CHECK_RET(ERR_DB_SEQUECE_EXISTS,"Sequences[%s] already exists in the current DSN,Check to see if you want to modify the sequence name.",\
                    tMemSeq.sSeqName);
        
    }
    TADD_FUNC("Finish."); 
    return iRet;
}
//检查文件中是否存在序列
int TMdbScript::CheckAndOperSequence(TMemSeq &tMemSeq,bool &bExist,bool bDel)
{
    int iRet = 0;
    bExist = false;
    char sSeqFileName[MAX_NAME_LEN];
    memset(sSeqFileName,0,sizeof(sSeqFileName));
    snprintf(sSeqFileName,sizeof(sSeqFileName),"%s%s",m_tMdbConfig.GetDSN()->sStorageDir,"Sequence.mdb");

    if(TMdbNtcFileOper::IsExist(sSeqFileName) == false)
    {//文件不存在
        if(bDel) return iRet;
        else
        {
            return AddToSequenceFile(sSeqFileName,tMemSeq);
        }
    }
    FILE * pSeqFile = fopen (sSeqFileName,"rb");;
    CHECK_OBJ(pSeqFile);
    int iBuffSize = MAX_SEQUENCE_COUNTS * sizeof(TMemSeq);
    char* pBuff = new(std::nothrow) char[iBuffSize];
    if(pBuff == NULL)
    {
        SAFE_CLOSE(pSeqFile);
        return ERR_OS_NO_MEMROY;
    }
    int iReadPageCount = (int)fread(pBuff,sizeof(TMemSeq),MAX_SEQUENCE_COUNTS,pSeqFile);
    if(iReadPageCount <= 0 && bDel)
    {
        SAFE_CLOSE(pSeqFile);
        SAFE_DELETE_ARRAY(pBuff);
        return iRet;
    }
    
    SAFE_CLOSE(pSeqFile);
    TMemSeq * pMemSeq = NULL;
    TMemSeq * pMemSeqAdd = NULL;
    for(int i = 0; i<iReadPageCount;i++)
    {
        pMemSeq = (TMemSeq*)(pBuff+((int)sizeof(TMemSeq)*i));
        if(!pMemSeq) continue;
        if(!bDel && !pMemSeqAdd && pMemSeq->sSeqName[0] == 0)
        {//新加，记下空闲位置
            pMemSeqAdd = pMemSeq;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pMemSeq->sSeqName,tMemSeq.sSeqName) == 0)
        {   
            bExist = true;
            if(bDel)
            {//需要删除序列
                memset(pMemSeq,0x0,sizeof(TMemSeq));
                iRet = ChangeSequenceFile(sSeqFileName,iReadPageCount,pBuff);
            }
            break;
        }
    }
    if(!bExist && !bDel)
    {//序列不存在，且新加
        if(pMemSeqAdd)
        {//写入空闲位置
            memcpy(pMemSeqAdd,&tMemSeq,sizeof(TMemSeq));
            iRet = ChangeSequenceFile(sSeqFileName,iReadPageCount,pBuff);
        }
        else//无空闲位置
        {
            iRet = AddToSequenceFile(sSeqFileName,tMemSeq);
        }
    }
    SAFE_DELETE_ARRAY(pBuff);
    return iRet;
}
//重新写入文件
int TMdbScript::ChangeSequenceFile(const char* sSeqFileName,int iPageCount,char *&pBuffer)
{
    int iRet = 0;
    CHECK_OBJ(pBuffer);
    FILE* pSeqFile = fopen (sSeqFileName,"wb+");
    CHECK_OBJ(pSeqFile);
    CHECK_RET(fseek(pSeqFile,0,SEEK_SET),"fseek failed,errno=%d,errstr=%s",errno,strerror(errno));
    TMemSeq * pMemSeq = NULL;
    for(int i = 0; i<iPageCount;i++)
    {
        pMemSeq = (TMemSeq*)(pBuffer+((int)sizeof(TMemSeq)*i));
        if(!pMemSeq || 0 == pMemSeq->sSeqName[0] ) continue;
        if(fwrite(pMemSeq, sizeof(TMemSeq), 1, pSeqFile)!= 1 )
        {
            SAFE_CLOSE(pSeqFile);
            CHECK_RET_BREAK(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
        }
    }
    fflush(pSeqFile);
    SAFE_CLOSE(pSeqFile);
    return iRet;
}
//新加的文件尾部
int TMdbScript::AddToSequenceFile(const char* sSeqFileName,TMemSeq &tMemSeq)
{
    int iRet = 0;
    FILE* pSeqFile = fopen (sSeqFileName,"ab");
    CHECK_OBJ(pSeqFile);
    if(fwrite(&tMemSeq, sizeof(TMemSeq), 1, pSeqFile)!= 1 )
    {
        SAFE_CLOSE(pSeqFile);
        CHECK_RET(ERR_OS_WRITE_FILE,"fwrite() errno=%d, errormsg=[%s].", errno, strerror(errno));
    }
    fflush(pSeqFile);
    SAFE_CLOSE(pSeqFile);
    return iRet;
}

/******************************************************************************
* 函数名称	:  DelSequenceToOracle
* 函数描述	:  删除oracle侧sequence表中指定序列
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::DelSequenceToOracle()
{
    TADD_FUNC("Begin."); 
    int iRet = 0;
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    TMemSeq &tMemSeq = m_pMdbSqlParser->m_pDDLSqlStruct->tMemSeq;
    bool bExist = false;
    CHECK_RET(LoadDSNCofig(),"Load the DSN configuration file failed.");
    CHECK_RET(CheckAndOperSequence(tMemSeq,bExist,true),"CheckSequence falied");
    if(!bExist)
    {
        CHECK_RET(ERR_DB_SEQUECE_EXISTS,"Sequences[%s] not exists in the current DSN",tMemSeq.sSeqName);
    }
    TADD_FUNC("Finish."); 
    return iRet;
}

/******************************************************************************
* 函数名称	:  ConnectOracle
* 函数描述	:  连接ORACLE
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ConnectOracle()
{
    TADD_FUNC("Start.");
	int iRet = 0;
	//如果链接不为空，则判断链接是否正常，如果正常则直接返回，如果不正常，则清空表的query，
	//并清空链接，重新创建链接
	if(m_pDBLink != NULL)
	{
		if(m_pDBLink->IsConnect()){return iRet;}
		m_pDBLink->Disconnect();
		SAFE_DELETE(m_pDBLink);
	}
    try
    {
        m_pDBLink = TMDBDBFactory::CeatDB();
		CHECK_OBJ(m_pDBLink);
        //加载DSN配置文件获取ORACLE连接信息
        CHECK_RET(LoadDSNCofig(),"Load the DSN configuration file failed.");
        m_pDBLink->SetLogin(m_tMdbConfig.GetDSN()->sOracleUID, m_tMdbConfig.GetDSN()->sOraclePWD, m_tMdbConfig.GetDSN()->sOracleID);
		if(!m_pDBLink->Connect())
		{
			TADD_ERROR(ERROR_UNKNOWN,"Connect Oracle Faild,user=[%s],pwd=[%s],dsn=[%s].",\
                m_tMdbConfig.GetDSN()->sOracleUID,m_tMdbConfig.GetDSN()->sOraclePWD,m_tMdbConfig.GetDSN()->sOracleID);
			return ERR_DB_NOT_CONNECTED;
		}
    }
    catch(TMDBDBExcpInterface &e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        return ERR_DB_NOT_CONNECTED;
    }
	TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  LoadDSNCofig
* 函数描述	:  上载DSN系统配置文件
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::LoadDSNCofig()
{
    TADD_FUNC("Start.");
	int iRet = 0;
    //检查对应的DSN的XML文件是否存在
    const char *pDsnName = NULL;
    if(m_pMdbSqlParser->m_pDDLSqlStruct)
    {
        if(m_pMdbSqlParser->m_pDDLSqlStruct->pDsn 
            && m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sName[0] != 0)
        {
            pDsnName = m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sName;
        }
        if(!pDsnName 
            && m_pMdbSqlParser->m_pDDLSqlStruct->sDsnName[0] != 0)
        {
            pDsnName = m_pMdbSqlParser->m_pDDLSqlStruct->sDsnName;
        }
    }
    if(!pDsnName && m_sLastDSN[0] != 0)
    {
        pDsnName = m_sLastDSN;
    }
    if(!pDsnName)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Get DSN name error.");
        return ERR_APP_CONFIG_NOT_EXIST;
    }
    
    if (!m_tMdbConfig.IsExist(pDsnName))
    {
        TADD_ERROR(ERROR_UNKNOWN,"QuickMDB_SYS_%s.xml does not exist.",m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sName);
        return ERR_APP_CONFIG_NOT_EXIST;
    }
    if(m_tMdbConfig.IsDsnSame(pDsnName)) return iRet;//加载过
    CHECK_RET(m_tMdbConfig.LoadDsnConfigFile(pDsnName,false),\
             "LoadDsnConfigFile(dsn=%s) failed.",pDsnName);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  CreateJob
* 函数描述	:  创建job(在QuickMDB_JOB.xml文件中增加job)
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::CreateJob(TMdbJob *pMdbJob)
{
    TADD_FUNC("Start.");
    int iRet =0;
    CHECK_OBJ(pMdbJob);
    MDBXMLElement * pJobRoot = NULL;
    if (m_sLastDSN[0] == '\0')
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM, "DSN is empty. Please specify a database by \'use database' command.");
        return -1;
    }
    int iDsnPos = FindDsnEx(m_sLastDSN);
    if(iDsnPos < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Invalid DSN[%s].",m_sLastDSN);
        return -1;
    }
    bool bExist = m_tDsnMgr[iDsnPos].m_tJob.m_pJobDocument->LoadFile(m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
    if(!bExist)
    {
        MDBXMLElement * pJobRoot = new MDBXMLElement("MDB_JOB");
        CHECK_OBJ(pJobRoot);
        m_tDsnMgr[iDsnPos].m_tJob.m_pJobDocument->LinkEndChild(pJobRoot);
        CHECK_RET(NewJobNode(pMdbJob,pJobRoot),"Generated node[%s] fails.",pMdbJob->m_sName);
    }
    else
    {
        pJobRoot = m_tDsnMgr[iDsnPos].m_tJob.m_pJobDocument->FirstChildElement("MDB_JOB");
        CHECK_OBJ(pJobRoot);
        MDBXMLElement *pEle = NULL;
        for(pEle=pJobRoot->FirstChildElement("job");pEle;pEle=pEle->NextSiblingElement("job"))
        {
            if(NULL == pEle->Attribute("name"))
            {
                continue;
            }
            if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pMdbJob->m_sName) == 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Job [%s] already exists.",pMdbJob->m_sName);
                return ERR_DB_JOB_ALREADY_EXIST;
            }  
        }
        CHECK_RET(NewJobNode(pMdbJob,pJobRoot),"Generated node[%s] fails.",pMdbJob->m_sName);
    }
    //备份文件
    CHECK_RET(BakFile(m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName),
        "Failed to backup the file[%s].",m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
    //保存文件
    m_tDsnMgr[iDsnPos].m_tJob.m_pJobDocument->SaveFile(m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  NewJobNode
* 函数描述	:  生成job节点
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::NewJobNode(TMdbJob *pMdbJob,MDBXMLElement *pJobRoot)
{
    TADD_FUNC("Start.");
    int iRet =0;
    CHECK_OBJ(pMdbJob);
    CHECK_OBJ(pJobRoot);
    char sRateType[MAX_NAME_LEN] = {0};
    MDBXMLElement *pJobElement = new MDBXMLElement("job");
    CHECK_OBJ(pJobElement);
    pJobRoot->LinkEndChild(pJobElement);
    pJobElement->SetAttribute("name",pMdbJob->m_sName);
    pJobElement->SetAttribute("exec_date",pMdbJob->m_sExecuteDate);
    pJobElement->SetAttribute("interval",pMdbJob->m_iInterval);
    pMdbJob->GetRateType(sRateType,sizeof(sRateType));
    pJobElement->SetAttribute("ratetype",sRateType);
    pJobElement->SetAttribute("sql",pMdbJob->m_sSQL);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  AlterJob
* 函数描述	:  修改job(在QuickMDB_JOB.xml文件中修改job)
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::AlterJob(TMdbJob *pMdbJob)
{
    TADD_FUNC("Start.");
    int iRet =0;
    CHECK_OBJ(pMdbJob);
    bool bIsExist = false;
    char sRateType[MAX_NAME_LEN] = {0};
    MDBXMLElement * pJobRoot = NULL;
    if (m_sLastDSN[0] == '\0')
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM, "DSN is empty. Please specify a database by \'use database' command.");
        return -1;
    }
    int iDsnPos = FindDsnEx(m_sLastDSN);
    if(iDsnPos < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Invalid DSN[%s].",m_sLastDSN);
        return -1;
    }
    bool bExist = m_tDsnMgr[iDsnPos].m_tJob.m_pJobDocument->LoadFile(m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
    if(bExist)
    {
        pJobRoot = m_tDsnMgr[iDsnPos].m_tJob.m_pJobDocument->FirstChildElement("MDB_JOB");
        CHECK_OBJ(pJobRoot);
        MDBXMLElement *pEle = NULL;
        for(pEle=pJobRoot->FirstChildElement("job");pEle;pEle=pEle->NextSiblingElement("job"))
        {
            if(NULL == pEle->Attribute("name"))
            {
                continue;
            }
            if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pMdbJob->m_sName) == 0)
            {
                bIsExist = true;
                break;
            }  
        }
        //找到job则修改否则报错退出
        if(bIsExist)
        {
            //修改指定job属性
            if(pMdbJob->m_sExecuteDate[0] != 0)
            {
                pEle->SetAttribute("exec_date",pMdbJob->m_sExecuteDate);
            }
            if(pMdbJob->m_iInterval > 0)
            {
                pEle->SetAttribute("interval",pMdbJob->m_iInterval);
            }
            if(pMdbJob->m_iRateType > 0)
            {
                pMdbJob->GetRateType(sRateType,sizeof(sRateType));
                pEle->SetAttribute("ratetype",sRateType);
            }
            if(pMdbJob->m_sSQL[0] != 0)
            {
                pEle->SetAttribute("sql",pMdbJob->m_sSQL);
            }
        }
        else
        {
            TADD_ERROR(ERROR_UNKNOWN,"Job[%s] does not exist in the configuration file[%s].",
                pMdbJob->m_sName,m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
            return ERR_DB_JOB_NOT_EXIST;
        }
        //备份文件
        CHECK_RET(BakFile(m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName),
            "Failed to backup the file[%s].",m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
        m_tDsnMgr[iDsnPos].m_tJob.m_pJobDocument->SaveFile(m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"Job file[%s] does not exist.",m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
        return ERR_DB_JOB_NOT_EXIST;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  RemoveJob
* 函数描述	:  删除Job
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::RemoveJob(TMdbJob *pMdbJob)
{
    TADD_FUNC("Start.");
    int iRet =0;
    CHECK_OBJ(pMdbJob);
    bool bIsExist = false;
    MDBXMLElement * pJobRoot = NULL;
    if (m_sLastDSN[0] == '\0')
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM, "DSN is empty. Please specify a database by \'use database' command.");
        return -1;
    }
    int iDsnPos = FindDsnEx(m_sLastDSN);
    if(iDsnPos < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Invalid DSN[%s].",m_sLastDSN);
        return -1;
    }
    bool bExist = m_tDsnMgr[iDsnPos].m_tJob.m_pJobDocument->LoadFile(m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
    if(bExist)
    {
        pJobRoot = m_tDsnMgr[iDsnPos].m_tJob.m_pJobDocument->FirstChildElement("MDB_JOB");
        CHECK_OBJ(pJobRoot);
        MDBXMLElement *pEle = NULL;
        for(pEle=pJobRoot->FirstChildElement("job");pEle;pEle=pEle->NextSiblingElement("job"))
        {
            if(NULL == pEle->Attribute("name"))
            {
                continue;
            }
            if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),pMdbJob->m_sName) == 0)
            {
                bIsExist = true;
                break;
            }  
        }
        //找到job则修改否则报错退出
        if(bIsExist)
        {
            pJobRoot->RemoveChild(pEle);
        }
        else
        {
            TADD_ERROR(ERROR_UNKNOWN,"Job[%s] does not exist in the configuration file[%s].",
                pMdbJob->m_sName,m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
            return ERR_DB_JOB_NOT_EXIST;
        }
        //备份文件
        CHECK_RET(BakFile(m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName),
            "Failed to backup the file[%s].",m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
        m_tDsnMgr[iDsnPos].m_tJob.m_pJobDocument->SaveFile(m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"Job file[%s] does not exist.",m_tDsnMgr[iDsnPos].m_tJob.m_sJobFileName);
        return ERR_DB_JOB_NOT_EXIST;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  DeleteSpecifiedIndexColumn
* 函数描述	:  删除指定索引列
* 输入		:  pTableEle 表节点，iColumnPos 列pos
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::DeleteSpecifiedIndexColumn(MDBXMLElement* pTableEle,const int iColumnPos)
{
    TADD_FUNC("Start.");
    int iRet =0;
    CHECK_OBJ(pTableEle);
    bool bIndexColumn = false;
    TMdbNtcSplit tSplit;
    int iIndexCounts = 0;
    char sIndexColumnPos[MAX_INDEX_COUNTS] = {0};
    for(MDBXMLElement* pIndexEle=pTableEle->FirstChildElement("index");\
            pIndexEle;pIndexEle=pIndexEle->NextSiblingElement("index"))
    {
        if(NULL == pIndexEle->Attribute("name"))
        {
            continue;
        }
        SAFESTRCPY(sIndexColumnPos,sizeof(sIndexColumnPos),pIndexEle->Attribute("column-pos"));
        tSplit.SplitString(sIndexColumnPos,',');
        iIndexCounts = tSplit.GetFieldCount();
        if(iIndexCounts == 1)
        {
            if(iColumnPos == atoi(tSplit[0]))
            {
                pTableEle->RemoveChild(pIndexEle);
            }
        }
        else
        {
            memset(sIndexColumnPos,0,sizeof(sIndexColumnPos));
            for(int k = 0; k<iIndexCounts;k++)
            {
                if(iColumnPos == atoi(tSplit[k]))
                {
                    bIndexColumn = true;
                    continue;
                }
                snprintf(sIndexColumnPos,sizeof(sIndexColumnPos),"%s,",tSplit[k]);
            }
            if(bIndexColumn)
            {
                TMdbNtcStrFunc::Trim(sIndexColumnPos,',');
                pIndexEle->SetAttribute("column-pos",sIndexColumnPos);
            }
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ReplaceSpecialCharactersForSQL
* 函数描述	:  替换SQL语句中的写XML文件时出现的&lt;、&gt;、&apos;字符
* 输入		:  
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbScript::ReplaceSpecialCharactersForSQL(const char *pszFullFileName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char * pMemBuffer = NULL;
    FILE * fp = NULL;
    unsigned long long iFileSize = 0;
    bool bRet = TMdbNtcFileOper::GetFileSize(pszFullFileName,iFileSize);
    if(bRet == false)
    {
        TADD_ERROR(ERROR_UNKNOWN,"GetFileSize() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));
        return ERR_OS_NO_FILE;
    }
    
    //如果内存空间不足，则申请更大的内存
    pMemBuffer = new(std::nothrow) char[iFileSize+1];    
    if(pMemBuffer == NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"new char[%ld] failed, out of memory.",iFileSize+1);
        return ERR_OS_NO_MEMROY;    
    }
    memset(pMemBuffer,0,iFileSize+1);
    //打开文件
    fp = fopen(pszFullFileName,"rt");
    if(fp == NULL)
    {
        SAFE_DELETE_ARRAY(pMemBuffer);
        TADD_ERROR(ERROR_UNKNOWN,"Failed to open file[%s].",pszFullFileName);
        return -1;   
    }
    fread(pMemBuffer,iFileSize,1,fp);
    SAFE_CLOSE(fp);
    //替换文件中的一下特殊字符
    char* psTmp = new(std::nothrow) char[iFileSize+1];    
    if(psTmp == NULL)
    {
        SAFE_DELETE_ARRAY(pMemBuffer);
        TADD_ERROR(ERROR_UNKNOWN,"new char[%ld] failed, out of memory.",iFileSize+1);
        return ERR_OS_NO_MEMROY;    
    }
    TMdbNtcStrFunc::Replace(pMemBuffer,"&lt;","<",psTmp);
    TMdbNtcStrFunc::Replace(psTmp,"&gt;",">", psTmp);
    TMdbNtcStrFunc::Replace(psTmp,"&apos;","'",psTmp);
    //重新写文件
    fp = fopen(pszFullFileName,"w+");
    if(fp == NULL)
    {
        SAFE_DELETE_ARRAY(pMemBuffer);
        TADD_ERROR(ERROR_UNKNOWN,"Failed to open file[%s].",pszFullFileName);
        return -1;   
    }
    setbuf(fp,NULL);
    if(fwrite(psTmp,strlen(psTmp),1,fp) != 1)
    {
        TADD_ERROR(ERROR_UNKNOWN,"fwrite(%s) error, errno=%d, errormsg=[%s].",pszFullFileName,errno, strerror(errno));
        iRet = ERR_OS_WRITE_FILE;
    }
    SAFE_CLOSE(fp);
    SAFE_DELETE_ARRAY(pMemBuffer);
    SAFE_DELETE_ARRAY(psTmp);
    TADD_FUNC("Finish.");
    return iRet;
}

    int TMdbScript::WriteTsAddInfo(TDsnScript* pDsnScript,const char* psTsName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(pDsnScript);
        CHECK_OBJ(psTsName);

        pDsnScript->m_pTsAlterDoc->Clear();
        bool bExsit = pDsnScript->m_pTsAlterDoc->LoadFile(pDsnScript->m_sTsAlterFile);
        if(bExsit)
        {
            pDsnScript->m_pTsAlterRoot = pDsnScript->m_pTsAlterDoc->FirstChildElement("MDBConfig");
            MDBXMLElement* pETs = pDsnScript->m_pTsAlterRoot->FirstChildElement("table-space");
            CHECK_OBJ(pETs);
            for(MDBXMLElement* pEle = pETs->FirstChildElement("add-ts"); pEle; pEle=pEle->NextSiblingElement("add-ts"))
            {
                if(NULL == pEle->Attribute("value"))
                {
                    continue;
                }
                if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("value"),psTsName) == 0)
                {
                    TADD_NORMAL("talespace[%s] already exists.",psTsName);
                    return 0;
                }
            }

            MDBXMLElement *pEAddTs = new MDBXMLElement("add-ts");
            pETs->LinkEndChild(pEAddTs);
            pEAddTs->SetAttribute("value",psTsName);
            
        }
        else
        {
            pDsnScript->m_pTsAlterRoot = new MDBXMLElement("MDBConfig");
            pDsnScript->m_pTsAlterDoc->LinkEndChild(pDsnScript->m_pTsAlterRoot);
            MDBXMLElement *pETs = new MDBXMLElement("table-space");
            pDsnScript->m_pTsAlterRoot->LinkEndChild(pETs);
            MDBXMLElement *pEAddTs = new MDBXMLElement("add-ts");
            pETs->LinkEndChild(pEAddTs);
            pEAddTs->SetAttribute("value",psTsName);
        }     

        pDsnScript->m_pTsAlterDoc->SaveFile(pDsnScript->m_sTsAlterFile);
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbScript::WriteTsPageSizeAlterInfo(TDsnScript* pDsnScript, const char* psTsName,size_t iOldValue, size_t iNewValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(pDsnScript);
        CHECK_OBJ(psTsName);

        pDsnScript->m_pTsAlterDoc->Clear();
        bool bExsit = pDsnScript->m_pTsAlterDoc->LoadFile(pDsnScript->m_sTsAlterFile);
        if(bExsit)
        {
            pDsnScript->m_pTsAlterRoot = pDsnScript->m_pTsAlterDoc->FirstChildElement("MDBConfig");
            MDBXMLElement* pETs = pDsnScript->m_pTsAlterRoot->FirstChildElement("table-space");
            CHECK_OBJ(pETs);
            for(MDBXMLElement* pEle = pETs->FirstChildElement("mod-pagesize"); pEle; pEle=pEle->NextSiblingElement("mod-pagesize"))
            {
                if(NULL == pEle->Attribute("name"))
                {
                    continue;
                }
                if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),psTsName) == 0)
                {
                    TADD_NORMAL("talespace[%s] already exists.",psTsName);
                    pEle->SetAttribute("old-value",iOldValue);
                    pEle->SetAttribute("new-value",iNewValue);
                    return 0;
                }
            }

            MDBXMLElement *pEModTs = new MDBXMLElement("mod-pagesize");
            pETs->LinkEndChild(pEModTs);
            pEModTs->SetAttribute("name",psTsName);
            pEModTs->SetAttribute("old-value",iOldValue);
            pEModTs->SetAttribute("new-value",iNewValue);
            
        }
        else
        {
            pDsnScript->m_pTsAlterRoot = new MDBXMLElement("MDBConfig");
            pDsnScript->m_pTsAlterDoc->LinkEndChild(pDsnScript->m_pTsAlterRoot);
            MDBXMLElement *pETs = new MDBXMLElement("table-space");
            pDsnScript->m_pTsAlterRoot->LinkEndChild(pETs);
            MDBXMLElement *pEModTs = new MDBXMLElement("mod-pagesize");
            pETs->LinkEndChild(pEModTs);
            pEModTs->SetAttribute("name",psTsName);
            pEModTs->SetAttribute("old-value",iOldValue);
            pEModTs->SetAttribute("new-value",iNewValue);
        }     

        pDsnScript->m_pTsAlterDoc->SaveFile(pDsnScript->m_sTsAlterFile);
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbScript::WriteTsDelInfo(TDsnScript* pDsnScript,const char* psTsName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(pDsnScript);
        CHECK_OBJ(psTsName);

        pDsnScript->m_pTsAlterDoc->Clear();
        bool bExsit = pDsnScript->m_pTsAlterDoc->LoadFile(pDsnScript->m_sTsAlterFile);
        if(bExsit)
        {
            pDsnScript->m_pTsAlterRoot = pDsnScript->m_pTsAlterDoc->FirstChildElement("MDBConfig");
            MDBXMLElement* pETs = pDsnScript->m_pTsAlterRoot->FirstChildElement("table-space");
            CHECK_OBJ(pETs);
            for(MDBXMLElement* pEle = pETs->FirstChildElement("del-ts"); pEle; pEle=pEle->NextSiblingElement("del-ts"))
            {
                if(NULL == pEle->Attribute("value"))
                {
                    continue;
                }
                if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("value"),psTsName) == 0)
                {
                    TADD_NORMAL("talespace[%s] already exists.",psTsName);
                    return 0;
                }
            }

            MDBXMLElement *pEDelTs = new MDBXMLElement("del-ts");
            pETs->LinkEndChild(pEDelTs);
            pEDelTs->SetAttribute("value",psTsName);
            
        }
        else
        {
            pDsnScript->m_pTsAlterRoot = new MDBXMLElement("MDBConfig");
            pDsnScript->m_pTsAlterDoc->LinkEndChild(pDsnScript->m_pTsAlterRoot);
            MDBXMLElement *pETs = new MDBXMLElement("table-space");
            pDsnScript->m_pTsAlterRoot->LinkEndChild(pETs);
            MDBXMLElement *pEDelTs = new MDBXMLElement("del-ts");
            pETs->LinkEndChild(pEDelTs);
            pEDelTs->SetAttribute("value",psTsName);
        }     

        pDsnScript->m_pTsAlterDoc->SaveFile(pDsnScript->m_sTsAlterFile);
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbScript::WriteAddColumn(TTableScript* psTabScript, const char* psColmName, int iColumCount)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(psTabScript);
        CHECK_OBJ(psColmName);

        psTabScript->m_pTabAlterDoc->Clear();
        bool bExsit = psTabScript->m_pTabAlterDoc->LoadFile(psTabScript->m_sTabAlterFile);
        if(bExsit)
        {
            psTabScript->m_pTabAlterRoot = psTabScript->m_pTabAlterDoc->FirstChildElement("MDBConfig");
            MDBXMLElement* pETs = psTabScript->m_pTabAlterRoot->FirstChildElement("table");
            CHECK_OBJ(pETs);
            for(MDBXMLElement* pEle = pETs->FirstChildElement("add-column"); pEle; pEle=pEle->NextSiblingElement("add-column"))
            {
                if(NULL == pEle->Attribute("value"))
                {
                    continue;
                }
                if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("value"),psColmName) == 0)
                {
                    TADD_NORMAL("Column[%s] already exists.",psColmName);
                    return 0;
                }
            }

            MDBXMLElement *pEAddColm = new MDBXMLElement("add-column");
            pETs->LinkEndChild(pEAddColm);
            pEAddColm->SetAttribute("value",psColmName);
            pEAddColm->SetAttribute("position",iColumCount);
            
        }
        else
        {
            psTabScript->m_pTabAlterRoot = new MDBXMLElement("MDBConfig");
            psTabScript->m_pTabAlterDoc->LinkEndChild(psTabScript->m_pTabAlterRoot);
            MDBXMLElement *pETab = new MDBXMLElement("table");
            psTabScript->m_pTabAlterRoot ->LinkEndChild(pETab);
            MDBXMLElement *pEAddColm = new MDBXMLElement("add-column");
            pETab->LinkEndChild(pEAddColm);
            pEAddColm->SetAttribute("value",psColmName);
            pEAddColm->SetAttribute("position",iColumCount);
        }     

        psTabScript->m_pTabAlterDoc->SaveFile(psTabScript->m_sTabAlterFile);
        
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbScript::WriteDropColumn(TTableScript* psTabScript, const char* psColmName, int iColumCount)
	{
		TADD_FUNC("Start.");
		int iRet = 0;

		CHECK_OBJ(psTabScript);
		CHECK_OBJ(psColmName);

		psTabScript->m_pTabAlterDoc->Clear();
		bool bExsit = psTabScript->m_pTabAlterDoc->LoadFile(psTabScript->m_sTabAlterFile);
		if(bExsit)
		{
			psTabScript->m_pTabAlterRoot = psTabScript->m_pTabAlterDoc->FirstChildElement("MDBConfig");
			MDBXMLElement* pETs = psTabScript->m_pTabAlterRoot->FirstChildElement("table");
			CHECK_OBJ(pETs);
			for(MDBXMLElement* pEle = pETs->FirstChildElement("drop-column"); pEle; pEle=pEle->NextSiblingElement("drop-column"))
			{
				if(NULL == pEle->Attribute("value"))
				{
					continue;
				}
				if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("value"),psColmName) == 0)
				{
					TADD_NORMAL("Column[%s] already exists.",psColmName);
					return 0;
				}
			}

			MDBXMLElement *pEDropColm = new MDBXMLElement("drop-column");
			pETs->LinkEndChild(pEDropColm);
			pEDropColm->SetAttribute("value",psColmName);
            pEDropColm->SetAttribute("position",iColumCount);
			
		}
		else
		{
			psTabScript->m_pTabAlterRoot = new MDBXMLElement("MDBConfig");
			psTabScript->m_pTabAlterDoc->LinkEndChild(psTabScript->m_pTabAlterRoot);
			MDBXMLElement *pETab = new MDBXMLElement("table");
			psTabScript->m_pTabAlterRoot ->LinkEndChild(pETab);
			MDBXMLElement *pEDropColm = new MDBXMLElement("drop-column");
			pETab->LinkEndChild(pEDropColm);
			pEDropColm->SetAttribute("value",psColmName);
            pEDropColm->SetAttribute("position",iColumCount);
		}	  

		psTabScript->m_pTabAlterDoc->SaveFile(psTabScript->m_sTabAlterFile);
		
		TADD_FUNC("Finish.");
		return iRet;
	}


    int TMdbScript::WriteColmDataTypeAlterInfo(TTableScript* psTabScript, const char* psColmName, int iOldValue, int iNewValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(psTabScript);
        CHECK_OBJ(psColmName);

        char sOldDataType[MAX_NAME_LEN] = {0};
        char sNewDataType[MAX_NAME_LEN]={0};

        GetDataType(iOldValue, sOldDataType, MAX_NAME_LEN);
        GetDataType(iNewValue, sNewDataType, MAX_NAME_LEN);

        psTabScript->m_pTabAlterDoc->Clear();
        bool bExsit = psTabScript->m_pTabAlterDoc->LoadFile(psTabScript->m_sTabAlterFile);
        if(bExsit)
        {
            psTabScript->m_pTabAlterRoot = psTabScript->m_pTabAlterDoc->FirstChildElement("MDBConfig");
            MDBXMLElement* pETs = psTabScript->m_pTabAlterRoot->FirstChildElement("table");
            CHECK_OBJ(pETs);
            for(MDBXMLElement* pEle = pETs->FirstChildElement("mod-datatype"); pEle; pEle=pEle->NextSiblingElement("mod-datatype"))
            {
                if(NULL == pEle->Attribute("name"))
                {
                    continue;
                }
                if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),psColmName) == 0)
                {
                    TADD_NORMAL("column[%s] already exists.",psColmName);
                    pEle->SetAttribute("old-value",sOldDataType);
                    pEle->SetAttribute("new-value",sNewDataType);
                    return 0;
                }
            }

            MDBXMLElement *pEModTs = new MDBXMLElement("mod-datatype");
            pETs->LinkEndChild(pEModTs);
            pEModTs->SetAttribute("name",psColmName);
            pEModTs->SetAttribute("old-value",sOldDataType);
            pEModTs->SetAttribute("new-value",sNewDataType);
            
        }
        else
        {
            psTabScript->m_pTabAlterRoot = new MDBXMLElement("MDBConfig");
            psTabScript->m_pTabAlterDoc->LinkEndChild(psTabScript->m_pTabAlterRoot);
            MDBXMLElement *pETab = new MDBXMLElement("table");
            psTabScript->m_pTabAlterRoot->LinkEndChild(pETab);
            MDBXMLElement *pEModTs = new MDBXMLElement("mod-datatype");
            pETab->LinkEndChild(pEModTs);
            pEModTs->SetAttribute("name",psColmName);
            pEModTs->SetAttribute("old-value",sOldDataType);
            pEModTs->SetAttribute("new-value",sNewDataType);
        }     

        psTabScript->m_pTabAlterDoc->SaveFile(psTabScript->m_sTabAlterFile);
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbScript::WriteColmDataLenAlterInfo(TTableScript* psTabScript, const char* psColmName, int iOldValue, int iNewValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(psTabScript);
        CHECK_OBJ(psColmName);

        psTabScript->m_pTabAlterDoc->Clear();
        bool bExsit = psTabScript->m_pTabAlterDoc->LoadFile(psTabScript->m_sTabAlterFile);
        if(bExsit)
        {
            psTabScript->m_pTabAlterRoot = psTabScript->m_pTabAlterDoc->FirstChildElement("MDBConfig");
            MDBXMLElement* pETs = psTabScript->m_pTabAlterRoot->FirstChildElement("table");
            CHECK_OBJ(pETs);
            for(MDBXMLElement* pEle = pETs->FirstChildElement("mod-datalen"); pEle; pEle=pEle->NextSiblingElement("mod-datalen"))
            {
                if(NULL == pEle->Attribute("name"))
                {
                    continue;
                }
                if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),psColmName) == 0)
                {
                    TADD_NORMAL("column[%s] already exists.",psColmName);
                    pEle->SetAttribute("old-value",iOldValue);
                    pEle->SetAttribute("new-value",iNewValue);
                    return 0;
                }
            }

            MDBXMLElement *pEModTs = new MDBXMLElement("mod-datalen");
            pETs->LinkEndChild(pEModTs);
            pEModTs->SetAttribute("name",psColmName);
            pEModTs->SetAttribute("old-value",iOldValue);
            pEModTs->SetAttribute("new-value",iNewValue);
            
        }
        else
        {
            psTabScript->m_pTabAlterRoot = new MDBXMLElement("MDBConfig");
            psTabScript->m_pTabAlterDoc->LinkEndChild(psTabScript->m_pTabAlterRoot);
            MDBXMLElement *pETab = new MDBXMLElement("table");
            psTabScript->m_pTabAlterRoot->LinkEndChild(pETab);
            MDBXMLElement *pEModTs = new MDBXMLElement("mod-datalen");
            pETab->LinkEndChild(pEModTs);
            pEModTs->SetAttribute("name",psColmName);
            pEModTs->SetAttribute("old-value",iOldValue);
            pEModTs->SetAttribute("new-value",iNewValue);
        }     

        psTabScript->m_pTabAlterDoc->SaveFile(psTabScript->m_sTabAlterFile);
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbScript::WriteTabZipTimeAlterInfo(TTableScript* psTabScript, const char* psOldValue, const char* psNewValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(psTabScript);
        CHECK_OBJ(psOldValue);
        CHECK_OBJ(psNewValue);

        psTabScript->m_pTabAlterDoc->Clear();
        bool bExsit = psTabScript->m_pTabAlterDoc->LoadFile(psTabScript->m_sTabAlterFile);
        if(bExsit)
        {
            psTabScript->m_pTabAlterRoot = psTabScript->m_pTabAlterDoc->FirstChildElement("MDBConfig");
            MDBXMLElement* pETs = psTabScript->m_pTabAlterRoot->FirstChildElement("table");
            CHECK_OBJ(pETs);
            for(MDBXMLElement* pEle = pETs->FirstChildElement("mod-timezip"); pEle; pEle=pEle->NextSiblingElement("mod-timezip"))
            {
                if(NULL == pEle->Attribute("name"))
                {
                    continue;
                }
                
                pEle->SetAttribute("old-value",psOldValue);
                pEle->SetAttribute("new-value",psNewValue);
                return 0;
            }

            MDBXMLElement *pEModTs = new MDBXMLElement("mod-datalen");
            pETs->LinkEndChild(pEModTs);
            pEModTs->SetAttribute("old-value",psOldValue);
            pEModTs->SetAttribute("new-value",psNewValue);
            
        }
        else
        {
            psTabScript->m_pTabAlterRoot = new MDBXMLElement("MDBConfig");
            psTabScript->m_pTabAlterDoc->LinkEndChild(psTabScript->m_pTabAlterRoot);
            MDBXMLElement *pETab = new MDBXMLElement("table");
            psTabScript->m_pTabAlterRoot->LinkEndChild(pETab);
            MDBXMLElement *pEModTs = new MDBXMLElement("mod-timezip");
            pETab->LinkEndChild(pEModTs);
            pEModTs->SetAttribute("old-value",psOldValue);
            pEModTs->SetAttribute("new-value",psNewValue);
        }     

        psTabScript->m_pTabAlterDoc->SaveFile(psTabScript->m_sTabAlterFile);
        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbScript::WriteTabTsAlterInfo(TTableScript* psTabScript, const char* psOldValue, const char* psNewValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        CHECK_OBJ(psTabScript);
        CHECK_OBJ(psOldValue);
        CHECK_OBJ(psNewValue);

        psTabScript->m_pTabAlterDoc->Clear();
        bool bExsit = psTabScript->m_pTabAlterDoc->LoadFile(psTabScript->m_sTabAlterFile);
        if(bExsit)
        {
            psTabScript->m_pTabAlterRoot = psTabScript->m_pTabAlterDoc->FirstChildElement("MDBConfig");
            MDBXMLElement* pETs = psTabScript->m_pTabAlterRoot->FirstChildElement("table");
            CHECK_OBJ(pETs);
            for(MDBXMLElement* pEle = pETs->FirstChildElement("mod-talespace"); pEle; pEle=pEle->NextSiblingElement("mod-talespace"))
            {
                if(NULL == pEle->Attribute("name"))
                {
                    continue;
                }
                
                pEle->SetAttribute("old-value",psOldValue);
                pEle->SetAttribute("new-value",psNewValue);
                return 0;
            }

            MDBXMLElement *pEModTs = new MDBXMLElement("mod-talespace");
            pETs->LinkEndChild(pEModTs);
            pEModTs->SetAttribute("old-value",psOldValue);
            pEModTs->SetAttribute("new-value",psNewValue);
            
        }
        else
        {
            psTabScript->m_pTabAlterRoot = new MDBXMLElement("MDBConfig");
            psTabScript->m_pTabAlterDoc->LinkEndChild(psTabScript->m_pTabAlterRoot);
            MDBXMLElement *pETab = new MDBXMLElement("table");
            psTabScript->m_pTabAlterRoot->LinkEndChild(pETab);
            MDBXMLElement *pEModTs = new MDBXMLElement("mod-talespace");
            pETab->LinkEndChild(pEModTs);
            pEModTs->SetAttribute("old-value",psOldValue);
            pEModTs->SetAttribute("new-value",psNewValue);
        }     

        psTabScript->m_pTabAlterDoc->SaveFile(psTabScript->m_sTabAlterFile);
        
        TADD_FUNC("Finish.");
        return iRet;
    }

/******************************************************************************
* 函数名称	:  CheckSQLValidity
* 函数描述	:  校验sql的有效性，加载刷新sql
* 输入		:  
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		: 
*******************************************************************************/
int TMdbScript::CheckSQLValidity(const char* pSQL)
{
    int iRet = 0;

	#ifndef DB_NODB
	
    if(!pSQL)
    {
        TADD_WARNING("SQL=NULL");
        return ERR_SQL_INVALID;
    }
    if(pSQL[0] == 0) return 0;//不校验
    #ifdef DB_ORACLE
    char sSQL[MAX_SQL_LEN*4]="explain plan for ";
	#elif DB_MYSQL
    char sSQL[MAX_SQL_LEN*4]="explain ";
	#endif
    char sPrefix[MAX_NAME_LEN]="";
    int iLen = strlen(sSQL);
    char *pSQLBegin = sSQL+iLen;
    SAFESTRCPY(pSQLBegin, sizeof(sSQL)-iLen, pSQL);//保存SQL语句
    TMdbNtcStrFunc::TrimLeft(pSQLBegin, ' ');
    //查询sql判断
    char *pIndex = (char*)memccpy(sPrefix, pSQLBegin, ' ', sizeof(sPrefix)-1);
    if(!pIndex)
    {
        TADD_WARNING("Invalid SQL=[%s]", pSQL);
        return ERR_SQL_INVALID;
    }
    else
    {
        *(pIndex - 1) = 0;
        if(TMdbNtcStrFunc::StrNoCaseCmp("select", sPrefix) != 0)
        {
            TADD_WARNING("Invalid SQL=[%s]", pSQL);
            return ERR_SQL_INVALID;
        }
    }
    //sql 语法分析
    if(ConnectOracle() != 0)
    {
        TADD_WARNING("Connect Oracle Faild");
        return ERR_DB_NOT_CONNECTED;
    }
    TMDBDBQueryInterface* pOraQuery = m_pDBLink->CreateDBQuery();
    if(!pOraQuery)
    {
        TADD_WARNING("Create Oracle Query Failed");
        return ERR_DB_NOT_CONNECTED;
    }
    
    try
    {
        pOraQuery->Close();
        pOraQuery->SetSQL(sSQL);
        pOraQuery->Execute();

    }
    catch(TMDBDBExcpInterface &e)
    {
        TADD_ERROR(ERR_SQL_INVALID,"SQL=[%s], error_msg=[%s].", pSQL, e.GetErrMsg());
        iRet = ERR_SQL_INVALID;
    }
    catch(...)
    {
        TADD_ERROR(ERR_SQL_INVALID,"Unknown error.");
        iRet = ERR_SQL_INVALID;
    }
    pOraQuery->Close();
    SAFE_DELETE(pOraQuery);

	#endif  //DB_NODB
	
    return iRet;
    
}

void TMdbScript::RemoveTabDir()
{
    if(m_sTabFilePath[0] != 0)
    {
        if(TMdbNtcDirOper::IsExist(m_sTabFilePath))
        {
            TMdbNtcDirOper::Remove(m_sTabFilePath);
        }
    }
    return;
}
//检查表空间是否存在
bool TMdbScript::CheckXmlTableSpaceExist(const char* sTablespaceName)
{
    bool bExist = false;
    CHECK_OBJ(sTablespaceName);
    if(sTablespaceName[0] == 0) return false;
    if(TMdbNtcStrFunc::StrNoCaseCmp(SYS_TABLE_SPACE,sTablespaceName) == 0) return true;
    int iPos = GetSysRootNode(false);
    if(iPos < 0)
    {
        return bExist;
    }
    for(MDBXMLElement* pEle = m_tDsnMgr[iPos].m_pSysRoot->FirstChildElement("table-space"); pEle; pEle=pEle->NextSiblingElement("table-space"))
    {
        if(NULL == pEle->Attribute("name"))
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pEle->Attribute("name"),sTablespaceName) == 0)
        {
            bExist = true;
            break;
        }
    }
        
    return bExist;
    
}

void TMdbScript::SetTableSpace(const char * sTablespaceName)
{
    if(!sTablespaceName) return;
    if(sTablespaceName[0] != 0)
    {
        SAFESTRCPY(m_sTablespaceName,sizeof(m_sTablespaceName),sTablespaceName);
    }
    return;
}

//rename table
int TMdbScript::RenameTable(const char* sOldTableName,const char* sNewTableName,bool bIsDynamic)//rename table
{
    TADD_FUNC("Start.");
    int iRet =0;
    bool bIsExist = false;
    MDBXMLElement* pEAttribute = NULL;
    TTableScript * pTableScript = NULL;
    char sNewTabDir[MAX_PATH_NAME_LEN] = {0};//表的新目录
    char sNewTabFullPath[MAX_PATH_NAME_LEN] = {0};//表的新路径
    
    CHECK_RET(GetTableRootNode(sOldTableName,pTableScript,bIsDynamic),"GetTableRootNode failed.");
    for(MDBXMLElement* pEle=pTableScript->m_pTableRoot->FirstChildElement("table"); pEle; pEle=pEle->NextSiblingElement("table"))
    {
        pEAttribute = pEle->FirstChildElement("name");
        if(NULL != pEAttribute)
        {
            bIsExist = true;
            break;
        }
    }
    
    if(bIsExist)
    {
        //目录检查
        //SAFESTRCPY(sNewTabUppName, sizeof(sNewTabUppName), sNewTableName);
        //TMdbNtcStrFunc::ToUpper(sNewTabUppName);
        char sDSN[MAX_NAME_LEN]={0};
        SAFESTRCPY(sDSN, sizeof(sDSN), m_sLastDSN);
        TMdbNtcStrFunc::ToUpper(sDSN);
        snprintf(sNewTabDir,sizeof(sNewTabDir), "%s.%s/.TABLE/.%s/",m_sSysFilePath,sDSN, sNewTableName);
        if(TMdbNtcDirOper::IsExist(sNewTabDir))
        {
            if(bIsDynamic)
            {
                //新目录如果存在，就删除。
                TMdbNtcDirOper::Remove(sNewTabDir,true);
            }
            else
            {//离线直接报错
                TADD_ERROR(ERR_TAB_TABLE_NAME_INVALID,"Table[%s] already exist, can't rename [%s] to ",sNewTableName,sOldTableName);
                return ERR_TAB_TABLE_NAME_INVALID;
            }
        }
        snprintf(sNewTabFullPath,sizeof(sNewTabFullPath),\
            "%s.Tab_%s_%s.xml",sNewTabDir,sDSN,sNewTableName);
    
        //修改前备份
        CHECK_RET(BakFile(pTableScript->m_sTableFileName),"Failed to backup file[%s]",pTableScript->m_sTableFileName);
        CHECK_RET(BakFile(pTableScript->m_sTabAlterFile),"Failed to backup file[%s]",pTableScript->m_sTabAlterFile);
        std::string sOldValue =  pEAttribute->Attribute("value");
        pEAttribute->SetAttribute("value", sNewTableName);
        pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
        //出错回退
        if(false == TMdbNtcDirOper::MakeFullDir(sNewTabDir))
        {
            TADD_WARNING("MakeFullDir[%s] failed",sNewTabDir);
            pEAttribute->SetAttribute("value", sOldValue);
            pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
            return ERR_OS_CREATE_DIR;
        }
        if(false == TMdbNtcFileOper::Copy(pTableScript->m_sTableFileName,sNewTabFullPath))
        {
            TADD_WARNING("Copy[%s] to [%s] failed",pTableScript->m_sTableFileName,sNewTabFullPath);
            pEAttribute->SetAttribute("value", sOldValue);
            pTableScript->m_pTableDocument->SaveFile(pTableScript->m_sTableFileName);
            TMdbNtcDirOper::Remove(sNewTabDir,true);
            return ERR_OS_BACKUP_FILE;
        }
        //删除旧目录
        TMdbNtcDirOper::Remove(m_sTabFilePath,true);
    }
    else
    {   
        TADD_ERROR(ERR_TAB_TABLE_NAME_INVALID,"Table[%s] config invalid.",sOldTableName);
        return ERR_TAB_TABLE_NAME_INVALID;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbScript::SaveSysCfgFile(int &iDsnPos)
{
    int iRet = 0;
    iDsnPos = FindDsnEx(m_sLastDSN);
    if(iDsnPos < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Invalid DSN[%s].",m_sLastDSN);
        return -1;
    }
    if(IsGenDsnXml() || TMdbNtcFileOper::IsExist(m_tDsnMgr[iDsnPos].m_sSysFileName))
    {
        BakFile(m_tDsnMgr[iDsnPos].m_sSysFileName);
        m_tDsnMgr[iDsnPos].m_pSysDocument->SaveFile(m_tDsnMgr[iDsnPos].m_sSysFileName);
    }
    return iRet;
    
}

//读取配置中的表空间
void TMdbScript::GetTableSpaceFromTableFile(const char *sTabName,const char *sTabFileName, char *sTabspaceName)
{
    if(!sTabFileName || !sTabspaceName) return;
    MDBXMLDocument tabXML;
    MDBXMLElement *pTableRoot   = NULL;
    MDBXMLElement *pTableEle    = NULL;
    MDBXMLElement *pTabspaceEle = NULL;
    if(false == tabXML.LoadFile(sTabFileName)) 
    {
        TADD_ERROR(ERR_TAB_CONFIG_TABLENAME_NOT_EXIST,"table[%s] not exist",sTabName);
        return;
    }
    pTableRoot = tabXML.FirstChildElement("MDBConfig");
    if(!pTableRoot) return;
    pTableEle = pTableRoot->FirstChildElement("table");
    if(!pTableEle) return;
    pTabspaceEle =  pTableEle->FirstChildElement("table-space");
    if(!pTabspaceEle) return;
    SAFESTRCPY(sTabspaceName,MAX_NAME_LEN,pTabspaceEle->Attribute("value"));
    return;
}
//}
