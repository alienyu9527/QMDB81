/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbDDLExecuteEngine.cpp
*@Description�� SQLִ������
*@Author:	    cao.peng
*@Date��	   
*@History:
******************************************************************************************/
#include "Control/mdbDDLExecuteEngine.h"
#include "Helper/mdbDateTime.h"
#include "Helper/SyntaxTreeAnalyse.h"
#include "Helper/mdbEncrypt.h"
#include "Control/mdbTableInfoCtrl.h"
//#include "OracleFlush/mdbLoadFromOra.h"
//#include "OracleFlush/mdbLoadFromPeer.h"
#include "Control/mdbJobCtrl.h"
#include "Control/mdbStorageEngine.h"

//namespace QuickMDB{

    #define CONVERT_Y_N(_src,_dest) \
                    if(TMdbNtcStrFunc::StrNoCaseCmp((_src),"Y") == 0)\
                    {(_dest) = true;}\
                    else {(_dest) = false;}\


TMdbDDLExecuteEngine::TMdbDDLExecuteEngine():
    m_pMdbSqlParser(NULL),
    m_pTable(NULL),
    m_pDsn(NULL),
    m_pShmDSN(NULL),
    m_pConfig(NULL),
    m_pUser(NULL)
{
    m_pDBLink = NULL;
    m_pScript = NULL;
    memset(m_sSSQL,0,sizeof(m_sSSQL));
    memset(m_sISQL,0,sizeof(m_sISQL));
    memset(sTableSpaceName,0,sizeof(sTableSpaceName));
}

TMdbDDLExecuteEngine::~TMdbDDLExecuteEngine()
{
    SAFE_DELETE(m_pTable);
    if(m_pDBLink)
    {
        m_pDBLink->Disconnect();
    }
    SAFE_DELETE(m_pDBLink);
    SAFE_DELETE(m_pScript);
}

/******************************************************************************
* ��������	:  Init
* ��������	:  ��ʼ��
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::Init(TMdbSqlParser * pMdbSqlParser)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pMdbSqlParser);
    m_pMdbSqlParser = pMdbSqlParser;
    m_pConfig = pMdbSqlParser->m_pMdbConfig;
    CHECK_OBJ(m_pConfig);
    m_pShmDSN = pMdbSqlParser->m_tSqlParserHelper.GetMdbShmDSN();
    CHECK_OBJ(m_pShmDSN);
    m_pDsn = m_pShmDSN->GetInfo();
    CHECK_OBJ(m_pDsn);
    CHECK_OBJ(pMdbSqlParser->m_pDDLSqlStruct);    
    m_pUser = pMdbSqlParser->m_pDDLSqlStruct->pUser;
    if(NULL == m_pTable)
    {
        m_pTable = new(std::nothrow) TMdbTable();
        CHECK_OBJ(m_pTable);
        m_pTable->Clear();
    }
    if(NULL != pMdbSqlParser->m_pDDLSqlStruct->pTable)
    {
        memcpy(m_pTable,pMdbSqlParser->m_pDDLSqlStruct->pTable,sizeof(TMdbTable));
    }
    if(NULL == m_pScript)
    {
        m_pScript = new(std::nothrow) TMdbScript();
        CHECK_RET(m_pScript->Init(pMdbSqlParser),"Failed to init m_pScript.");
    }
    m_pMdbJob = pMdbSqlParser->m_pDDLSqlStruct->pMdbJob;
    CHECK_RET(m_tProcCtrl.Init(m_pDsn->sName),"Failed to init m_tProcCtrl.");
    TADD_FUNC("Finish.");    
    return iRet;
}

/******************************************************************************
* ��������	:  SetDB
* ��������	:  ����DB
* ����		:  pShmDsn - DSN��Ϣ  pMdb - mdb������Ϣ
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::SetDB(TMdbShmDSN * pShmDsn,TMdbConfig *pConfig)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pShmDsn);
    CHECK_OBJ(pConfig);
    m_pShmDSN = pShmDsn;
    m_pConfig = pConfig;
    CHECK_RET(m_pShmDSN->Attach(),"DSN Attach failed.");
    m_pDsn = m_pShmDSN->GetInfo();
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  Execute
* ��������	:  DDL�������
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::Execute()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iSqlType[2];
    CHECK_OBJ(m_pMdbSqlParser->m_pDDLSqlStruct);
    memset(iSqlType,0,sizeof(iSqlType));
    //����ܷ����Ķ��������Ƿ����
    CHECK_RET(m_pMdbSqlParser->CheckTableExist(),"Table[%s] not exist",m_pMdbSqlParser->m_stSqlStruct.sTableName);
    m_pScript->InitDsn(m_pDsn->sName);
    m_pScript->SetTableSpace(m_pMdbSqlParser->m_pDDLSqlStruct->sTableSpaceName);    
    if(0 != m_pTable->sTableName[0])
    {
        CHECK_RET(m_pScript->InitTableInfo(m_pDsn->sName,m_pTable),"InitTableInfo failed");
    }
    iSqlType[0] = m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0];
    iSqlType[1] = m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[1];
    switch(iSqlType[0])
    {
        case TK_CREATE:
        {   
            iRet = ExecuteCreateOpt(iSqlType[1]);
            if(iRet != 0)
            {//����ʧ�ܾͰѱ�Ŀ¼ɾ��
                m_pScript->RemoveTabDir();
                CHECK_RET(iRet,"ExecuteCreateOpt failed");
            }
            break;
        }
        case TK_DROP:
        {       
            CHECK_RET(ExecuteDropOpt(iSqlType[1]),"ExecuteDropOpt failed");
            break;
        }
        case TK_TRUNCATE:
        {       
            CHECK_RET(ExecuteTruncateOpt(iSqlType[1]),"ExecuteTruncateOpt failed");
            break;
        }
        case TK_ALTER:
        {      
            CHECK_RET(ExecuteAlterOpt(iSqlType[1]),"ExecuteAlterOpt failed");
            break;
        }
        case TK_MODIFY:
        {
            if(iSqlType[1] == TK_TABLESYS)
            {
                CHECK_RET(ExecuteModifyTableAttr(),"ExecuteCreateUser failed");
            }
            else if(iSqlType[1] == TK_COLUMN)
            {
                CHECK_RET(ExecuteModifyTable(),"ExecuteModifyTableColumn failed");
            }
            else if(iSqlType[1] == TK_PARAMETER)
            {
                CHECK_RET(ModifyFlushSQLOrLoadSQLParam(),"ExecuteModifyTableColumn failed");
            }
            else
            {
                CHECK_RET(ERR_SQL_INVALID,"error sql type[%d %d]",\
                    m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0],\
                    m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[1]);
            }
            break;
        }
        case TK_ADD:
        {
            if(iSqlType[1] == TK_COLUMN)
            {
                CHECK_RET(ExecuteModifyTable(),"ExecuteModifyTableColumn failed");
            }
            else if(iSqlType[1] == TK_SEQUENCE)
            {
                CHECK_RET(AddSequenceToOracle(),"ExecuteAddSequence failed");
            }
            else if(iSqlType[1] == TK_PARAMETER)
            {
                CHECK_RET(AddFlushSQLOrLoadSQLParam(),"AddFlushSQLOrLoadSQLParam failed");
            }
            else
            {
                CHECK_RET(-1,"error sql type[%d %d]",m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0],\
                    m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[1]);
            }
            break;
        }
        case TK_USE:
        {
            if(iSqlType[1] == TK_TABLESPACE)
            {
                SAFESTRCPY(sTableSpaceName,sizeof(sTableSpaceName),\
                    m_pMdbSqlParser->m_pDDLSqlStruct->sTableSpaceName);
            }
            else
            {
                CHECK_RET(ERR_SQL_INVALID,"Dynamic does not support the use database command.");
            }
            break;
        }
        case TK_LOAD:
        {
            CHECK_RET(LoadDataFromRepOrOracle(m_pTable->sTableName),"LoadDataFromRepOrOracle failed");
            break;
        }
        case TK_REMOVE:
        {
            CHECK_RET(ExecuteRemoveJob(),"ExecuteRemoveJob failed");
            break;
        }
        case TK_RENAME:
        {
            CHECK_RET(ExecuteRenameTable(m_pMdbSqlParser->m_pDDLSqlStruct->sNewTableName,true),"ExecuteRenameTable failed");
            break;
        }
        
        default:
        {
            CHECK_RET(ERR_SQL_INVALID,"Sql type[%d %d] not supported.",\
                m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0],\
                m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[1]);
        }
    }    
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteCreateOpt
* ��������	:  ִ��create����
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteCreateOpt(const int iSqlType)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(iSqlType == TK_TABLE)
    {
        CHECK_RET(ExecuteCreateTable(),"ExecuteCreateTable failed");
    }
    else if(iSqlType == TK_INDEX)
    {
        CHECK_RET(ExecuteAddIndex(m_pTable->sTableName),"ExecuteAddIndex failed");
    }
    else if(iSqlType == TK_USER)
    {
        CHECK_RET(ExecuteCreateUser(),"ExecuteCreateUser failed");
    }
    else if(iSqlType == TK_TABLESPACE)
    {
        CHECK_RET(ExecuteCreateTablespace(),"ExecuteCreateTablespace failed");
    }
    else if(iSqlType == TK_JOB)
    {
        CHECK_RET(ExecuteCreateJob(),"ExecuteCreateJob failed");
    }
    else
    {
        CHECK_RET(ERR_SQL_INVALID,"Sql type[%d %d] not supported.",\
            m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0],\
            m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[1]);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteDropOpt
* ��������	:  ִ��DROP����
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteDropOpt(const int iSqlType)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(iSqlType == TK_TABLE)
    {
        CHECK_RET(ExecuteDropTable(m_pTable->sTableName,true),"ExecuteDropTable failed");
    }
    /*else if(iSqlType == TK_INDEX)
    {
        CHECK_RET(ExecuteDropIndex(m_pTable->sTableName,\
            m_pMdbSqlParser->m_pDDLSqlStruct->sIndexName),"ExecuteDropTable failed");
    }*/
    else if(iSqlType == TK_USER)
    {
        CHECK_RET(ExecuteDropUser(),"ExecuteDropUser failed");
    }
    else if(iSqlType == TK_TABLESPACE)
    {
        CHECK_RET(ExecuteDropTablespace(),"ExecuteDropTablespace failed");
    }
    else if(iSqlType == TK_COLUMN)
    {   
        CHECK_RET(ExecuteModifyTable(),"ExecuteModifyTableColumn failed");
    }
    else if(iSqlType == TK_SEQUENCE)
    {
        CHECK_RET(DelSequenceToOracle(),"ExecuteDelSequence failed");
    }
    else if(iSqlType == TK_PARAMETER)
    {
        CHECK_RET(DropFlushSQLOrLoadSQLParam(),"DropFlushSQLOrLoadSQLParam failed");
    }
    else if(iSqlType == TK_DATABASE)
    {
        CHECK_RET(ExecuteDropDsn(),"ExecuteDropDsn failed.");
    }
    else
    {
        CHECK_RET(ERR_SQL_INVALID,"Sql type[%d %d] not supported.",\
            m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0],\
            m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[1]);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteTruncateOpt
* ��������	:  ִ��TRUNCATE����
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jiang.xiaolong
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteTruncateOpt(const int iSqlType)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(iSqlType == TK_TABLE)
    {
        CHECK_RET(ExecuteTruncateTable(m_pTable->sTableName,true),"ExecuteDropTable failed");
    }
    else
    {
        CHECK_RET(ERR_SQL_INVALID,"Sql type[%d %d] not supported.",\
            m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0],\
            m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[1]);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteAlterOpt
* ��������	:  ִ��ALTER����
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteAlterOpt(const int iSqlType)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(iSqlType == TK_USER)
    {
        CHECK_RET(ExecuteAlterUser(),"ExecuteAlterUser failed");
    }
    else if(iSqlType == TK_TABLESPACE)
    {
        CHECK_RET(ExecuteAlterTablespace(),"ExecuteCreateUser failed");
    }
    else if(iSqlType == TK_DATABASE)
    {
        CHECK_RET(ExecuteAlterDsn(),"ExecuteAlterDsn failed");
    }
    else if(iSqlType == TK_SEQUENCE)
    {
        CHECK_RET(AlterSequence(),"AlterSequence failed");
    }
    else if(iSqlType == TK_JOB)
    {
        CHECK_RET(ExecuteAlterJob(),"ExecuteAlterJob failed");
    }
    else
    {
        CHECK_RET(ERR_SQL_INVALID,"Sql type[%d %d] not supported.",\
            m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0],\
            m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[1]);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteDropDsn
* ��������	:  ɾ�����ݿ�
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteDropDsn()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TADD_ERROR(ERROR_UNKNOWN,"Does not support dynamic drop database[%s].",m_pDsn->sName);
    iRet =  ERR_SQL_INVALID;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteAlterDsn
* ��������	:  �޸�DSN����(IP���˿ڡ�dsn-value������̬�޸�)
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteAlterDsn()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bIsStartRep = false;
    bool bIsStartOraRep = false;
    bool bIsStartCapture = false;
    bool bRestartOraRep = false;
    bool bTrue = false;
    //��̬�޸�DSN��������Ϣ��Ҫ����
    CHECK_RET(m_pShmDSN->LockDSN(),"lock failed.");
    for(int i =0;i<MAX_SQL_COUNTS;i++)
    {
        TMdbOtherAttr &tMdbAttr = m_pMdbSqlParser->m_pDDLSqlStruct->m_tAttr[i];
        if(tMdbAttr.sAttrName[0] == 0){continue;}            
        if(TMdbNtcStrFunc::StrNoCaseCmp(tMdbAttr.sAttrName,"log-level") == 0)
        {
            m_pDsn->iLogLevel = TMdbNtcStrFunc::StrToInt(tMdbAttr.sAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(tMdbAttr.sAttrName,"file-size") == 0)
        {
            m_pShmDSN->GetSyncArea()->m_iFileSize = TMdbNtcStrFunc::StrToInt(tMdbAttr.sAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(tMdbAttr.sAttrName,"ora-rep-counts") == 0)
        {
            int iOraRepCounts = TMdbNtcStrFunc::StrToInt(tMdbAttr.sAttrValue);
            if(iOraRepCounts < 0 || iOraRepCounts > 10)
            {
                CHECK_RET_BREAK(ERR_APP_INVALID_PARAM,\
                    "Invalid parameter(ora_rep_counts) value %s,value in the range of 1-9.",tMdbAttr.sAttrValue);   
            }
            m_pDsn->m_iOraRepCounts = iOraRepCounts;  
            bRestartOraRep = true;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(tMdbAttr.sAttrName,"is-ora-rep") == 0)
        {
            //�ж��Ƿ�̬����oracleͬ����
            bTrue = (!m_pDsn->m_bIsOraRep 
                        && TMdbNtcStrFunc::StrNoCaseCmp(tMdbAttr.sAttrValue,"Y") == 0);
            if(bTrue)
            {   
                TADD_NORMAL("Dynamically create the oracle synchronization area, Need to restart the business processes.");
                CHECK_RET_BREAK(m_pShmDSN->CreateSyncAreaShm(),\
                                    "Can't create OraShm shared memory.");
                bIsStartOraRep = true;
            }
            CONVERT_Y_N(tMdbAttr.sAttrValue,m_pDsn->m_bIsOraRep);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(tMdbAttr.sAttrName,"is-rep") == 0)
        {
            //�ж��Ƿ�̬��������ͬ����
            bTrue = (!m_pDsn->m_bIsRep
                        && TMdbNtcStrFunc::StrNoCaseCmp(tMdbAttr.sAttrValue,"Y") == 0);
            if(bTrue)
            {
                TADD_NORMAL("Dynamically create the Rep synchronization area, Need to restart the business processes.");
                CHECK_RET_BREAK(m_pShmDSN->CreateSyncAreaShm(),\
                    "Can't create RepShm shared memory.");
                bIsStartRep = true;
            }
            CONVERT_Y_N(tMdbAttr.sAttrValue,m_pDsn->m_bIsRep);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(tMdbAttr.sAttrName,"is-capture-router") == 0)
        {
            //�ж��Ƿ�̬����CAPTRUE
            bTrue = (!m_pDsn->m_bIsCaptureRouter
                        && TMdbNtcStrFunc::StrNoCaseCmp(tMdbAttr.sAttrValue,"Y") == 0);
            if(bTrue)
            {
                TADD_NORMAL("Dynamically create the capture synchronization area,Need to restart the business processes.");
                CHECK_RET_BREAK(m_pShmDSN->CreateSyncAreaShm(),\
                                "Can't create CaptrueShm shared memory.");
                bIsStartCapture = true;
            }
            CONVERT_Y_N(tMdbAttr.sAttrValue,m_pDsn->m_bIsCaptureRouter);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(tMdbAttr.sAttrName,"not-load-table-list") == 0)
        {
            TADD_WARNING("Dynamically modified !! Need to restart the business processes manually.");
        }
        else
        {
            TADD_WARNING("The DSN attribute[%s] does not allow modified in the shared memory.",\
                    tMdbAttr.sAttrName);
            memset(tMdbAttr.sAttrName,0,sizeof(tMdbAttr.sAttrName));
        }
    }
    CHECK_RET(m_pShmDSN->UnLockDSN(),"unlock failed.");
    CHECK_RET(iRet,"ExecuteAlterDsn failed."); 
    //�޸�XML�ļ�
    CHECK_RET(m_pScript->ModifyDsn(true),\
                 "Modify the DSN(%s) configuration file failed.",m_pDsn->sName);
    //����ORACLEͬ����ؽ���
    if(bIsStartOraRep)
    {
        CHECK_RET(StartOraRepProcess(),"Start the oracle synchronization process fails.");
    }
    //��������ͬ����ؽ���
    if(bIsStartRep)
    {
        CHECK_RET(StartRepProcess(),"Start the standby synchronization process fails.");
    }
    //����captureˢ�½���
    if(bIsStartCapture)
    {
        CHECK_RET(StartCaptureProcess(),"Start the captrue flush process fails.");
    }
    if(bRestartOraRep)
    {   
        CHECK_RET(RestartOraRepProcess(),"Restart the oracle synchronization process fails..");
    }    

    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteCreateTable
* ��������	:  ������
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteCreateTable(const bool bIsGenXML)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(m_pTable);
    TMdbTableSpace * pTableSapce = NULL;
    //����У��һ���е��������ͺͳ���
    if(bIsGenXML)
    {
        for(int i=0;i<m_pTable->iColumnCounts;i++)
        {
            CHECK_RET(m_pConfig->CheckColumnProperty(m_pTable,i),"Column attribute check fails.");
        }
        //��̬������SQL�б���ָ������
        if(m_pTable->m_tPriKey.iColumnCounts <= 0)
        {
            CHECK_RET(ERR_TAB_NO_PRIMARY_KEY,"Dynamic create table[%s] must specify the primary key.",m_pTable->sTableName);
        }
        
        //������sql��ˢ��sql�Ƿ���Ч
        CHECK_RET(m_pScript->CheckSQLValidity(m_pTable->m_sLoadSQL),"CheckSQLValidity loadSQL failed");
        CHECK_RET(m_pScript->CheckSQLValidity(m_pTable->m_sFlushSQL),"CheckSQLValidity FlushSQL failed");
    }
   
    //����������п��Բ�ָ����ռ�ID��������Ҫʹ��USE TABLESPACEָ����ռ�
    if(strlen(m_pTable->m_sTableSpace) == 0)
    {
        pTableSapce = m_pShmDSN->GetTableSpaceAddrByName(sTableSpaceName);
    }
    else
    {
        pTableSapce = m_pShmDSN->GetTableSpaceAddrByName(m_pTable->m_sTableSpace);
    }
    //�ñ��Ӧ�ı�ռ䲻������Ҫ����һ��
    if(NULL == pTableSapce)
    { 
        pTableSapce = m_pConfig->GetTableSpaceByName(m_pTable->m_sTableSpace);
        CHECK_OBJ(pTableSapce);
        CHECK_RET(m_mdbTSCtrl.CreateTableSpace(pTableSapce,m_pConfig,true),"CreateTableSpace failed...");
    }
    SAFESTRCPY(m_pTable->m_sTableSpace, sizeof(m_pTable->m_sTableSpace), pTableSapce->sName);
    
    //�Ƿ���ϵͳ��(ϵͳ��������)�Լ��ڹ����ڴ����Ƿ����
    if(!IsValidNewTable(m_pTable->sTableName))
    {
        CHECK_RET(-1,"table[%s] is not valid....",m_pTable->sTableName);
    }
    
    //�����б��������Ϣ�ͱ���һ����¼�Ĵ�С
    CHECK_RET(m_pConfig->SetColOffset(m_pTable), "SetColOffset failed.") ;
    CHECK_RET(m_pConfig->CalcOneRecordSize(m_pTable), "CalcOneRecordSize failed.");
    
    //���������ڴ��
    TMdbTable *pTable = NULL;
    CHECK_RET(m_pShmDSN->AddNewTable(pTable,m_pTable), "AddNewTable failed.");

    //�����Ļ��������ͳ�ͻ����
    TMdbIndexCtrl tIndexCtrl;
    tIndexCtrl.AttachDsn(m_pShmDSN);
    CHECK_RET(tIndexCtrl.AddTableIndex(pTable,m_pConfig->GetDSN()->iDataSize),\
        "Can't InitIndexMem table=%s.",pTable->sTableName);
    pTable->iMagic_n++;
    //�����������ļ����ڴ�����Ϣ
    TMdbTable *pConfigTable = m_pConfig->GetIdleTable();
    CHECK_OBJ(pConfigTable);
    pConfigTable->Clear();
    memcpy(pConfigTable,pTable,sizeof(TMdbTable));
    m_pConfig->SetTableCounts(m_pConfig->GetTableCounts()+1);
    
    //���ɶ�Ӧ��xml�ļ�
    if(bIsGenXML)
    {
        //��ֹʹ��USEָ����ռ�����
        SAFESTRCPY(m_pMdbSqlParser->m_pDDLSqlStruct->pTable->m_sTableSpace, sizeof(m_pMdbSqlParser->m_pDDLSqlStruct->pTable->m_sTableSpace), m_pTable->m_sTableSpace);
        m_pScript->CreateTable(m_pMdbSqlParser->m_pDDLSqlStruct->pTable,true);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteDropTable
* ��������	:  ɾ����
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteDropTable(const char *pTableName,const bool bIsGenXML)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTableName);
    if(IsSysTable(pTableName))
    {
        CHECK_RET(-1,"system table can`t be droped....");
    }
    TMdbTable * pTable = m_pShmDSN->GetTableByName(pTableName);
    CHECK_OBJ(pTable);
    TMdbTableInfoCtrl tDrop;
    CHECK_RET(tDrop.SetDSN(m_pDsn->sName),"Failed to set the dsn[%s].",m_pDsn->sName);
    CHECK_RET(tDrop.DropTable(pTableName,bIsGenXML),"Drop Table[%s] failed.",pTableName);
    CHECK_RET(DeleteDBARecord(pTableName),"DeleteDBARecord failed.");
    TMdbTable *pTableCf = m_pConfig->GetTable(pTableName);
    if(NULL != pTableCf)
    {
        pTableCf->Clear();
    }
    
    TMdbTable *pTableCfOld = m_pConfig->GetOldTableStruct(pTableName);
    if(NULL != pTableCfOld)
    {
        pTableCfOld->Clear();
    }
    
    m_pConfig->SetTableCounts(m_pConfig->GetTableCounts()-1);

    //���ɶ�Ӧ��xml�ļ�
    if(bIsGenXML)
    {
        m_pScript->DropTable(pTableName);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteTruncateTable
* ��������	:  truncate��
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jiang.xiaolong
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteTruncateTable(const char *pTableName,const bool bIsGenXML)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTableName);
    if(IsSysTable(pTableName))
    {
        CHECK_RET(-1,"system table can`t be Truncated....");
    }
    TMdbTable * pTable = m_pShmDSN->GetTableByName(pTableName);
    CHECK_OBJ(pTable);
    TMdbTableInfoCtrl tTruncate;
    CHECK_RET(tTruncate.SetDSN(m_pDsn->sName),"Failed to set the dsn[%s].",m_pDsn->sName);
    CHECK_RET(tTruncate.TruncateTable(pTableName),"Truncate Table[%s] failed.",pTableName);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteAddIndex
* ��������	:  ��������
* ����		: 
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteAddIndex(const char * pTableName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char sSQL[MAX_SQL_LEN] = {0};
    TMdbIndexCtrl tIndexCtrl;
    CHECK_OBJ(pTableName);
    CHECK_OBJ(m_pTable);
    TMdbTable * pTable = m_pShmDSN->GetTableByName(pTableName);
    CHECK_OBJ(pTable);
    snprintf(sSQL,sizeof(sSQL),"select * from %s",pTable->sTableName);
    //�����жϲ����ı��Ƿ��м�¼������м�¼�ǲ������û������������
    /*
    if(!m_tMDB.IsConnect())
    {
        CHECK_RET(ConnectMDB(),"connect QMDB Faild.");
    }
    TMdbQuery *pQuery = m_tMDB.CreateDBQuery();
    CHECK_OBJ(pQuery);
    pQuery->SetSQL(sSQL,QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);
    pQuery->Open();
    if(pQuery->Next())
    {
        TADD_ERROR(ERROR_UNKNOWN,"The index table[%s] has data exist, not allowed to add.",pTable->sTableName);
        SAFE_DELETE(pQuery);
        return ERR_SQL_INDEX_COLUMN_ERROR;
    }
    SAFE_DELETE(pQuery);
    */
    //��������
    CHECK_RET(pTable->tTableMutex.Lock(true,&(m_pDsn->tCurTime)),"lock failed.");
    memcpy(&(pTable->tIndex[pTable->iIndexCounts]),\
        &(m_pTable->tIndex[m_pTable->iIndexCounts-1]),sizeof(TMdbIndex));
    tIndexCtrl.AttachDsn(m_pShmDSN);
    TADD_DETAIL("SHM:index = %s,new index = %s.",pTable->tIndex[pTable->iIndexCounts].sName,\
        m_pTable->tIndex[m_pTable->iIndexCounts-1].sName);
    CHECK_RET(tIndexCtrl.AddTableSingleIndex(pTable,m_pConfig->GetDSN()->iDataSize),\
        "Can't InitIndexMem table=%s.",pTable->sTableName);
    pTable->iIndexCounts++;
    //���¼���һ����¼�ĳ���
    CHECK_RET(m_pConfig->CalcOneRecordSize(pTable),"CalcOneRecordSize failed.");
    CHECK_RET(pTable->tTableMutex.UnLock(true),"unlock failed.");
    
    //���������ļ����ڴ�����Ϣ
    TMdbTable *pTableCf = m_pConfig->GetTable(pTableName);
    if(NULL != pTableCf)
    {
        memcpy(pTableCf,pTable,sizeof(TMdbTable));
    }
    
    //���ɶ�Ӧ��xml�ļ�
    m_pScript->AddIndex(true);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteDropIndex
* ��������	:  ɾ������
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteDropIndex(const char * pTableName,const char *pIndxName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iIndexPos = -1;
    TMdbIndexCtrl tIndexCtrl;
    CHECK_OBJ(pTableName);
    CHECK_OBJ(pIndxName);
    TMdbTable * pTable = m_pShmDSN->GetTableByName(pTableName);
    CHECK_OBJ(pTable);
    //����ɾ��������λ��
    for(int i = 0;i< pTable->iIndexCounts;i++)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tIndex[i].sName,pIndxName)== 0)
        {
            iIndexPos = i;
            break;
        }
    }
    if(iIndexPos <0)
    {
        CHECK_RET(ERR_TAB_INDEX_NOT_EXIST,"Index [%s] does not exist.",pIndxName);
    }
    CHECK_RET(pTable->tTableMutex.Lock(true,&(m_pDsn->tCurTime)),"lock failed.");
    //����������������ϵ���Ϣ
    do
    {
        CHECK_RET_BREAK(tIndexCtrl.DeleteTableSpecifiedIndex(m_pShmDSN,\
        pTable,m_pMdbSqlParser->m_pDDLSqlStruct->sIndexName),"DeleteTableIndex failed...");

        //����Ҫɾ����������Ϣ
        pTable->tIndex[iIndexPos].Clear();
        for(;iIndexPos < pTable->iIndexCounts-1;iIndexPos++)
        {
            memcpy(&(pTable->tIndex[iIndexPos]),&(pTable->tIndex[iIndexPos+1]),sizeof(TMdbIndex));
            pTable->tIndex[iIndexPos+1].Clear();
        }
        pTable->iIndexCounts--;
    }while(0);
    CHECK_RET(pTable->tTableMutex.UnLock(true),"unlock failed.");
    
    //���ɶ�Ӧ��xml�ļ�
    if(0 == iRet)
    {
        m_pScript->DropIndex(true);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteCreateUser
* ��������	:  �����û�
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteCreateUser()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //���ȼ�������û��Ƿ����
    CHECK_OBJ(m_pUser);
    TMDbUser* pUser = m_pConfig->GetUser(m_pUser->sUser);
    if(pUser != NULL)
    {
        CHECK_RET(ERR_SQL_INVALID,"user[%s] does exist.",m_pUser->sUser);
    }
    //�ж��û����Ƿ�ﵽ���ֵ
    if(m_pConfig->GetUserCounts() >= MAX_USER_COUNT)
    {
        TADD_ERROR(ERROR_UNKNOWN,"The number of users reached a maximum value[64].");
        return ERR_DB_MAX_USER_NUMBER;
    }
    pUser = m_pConfig->GetUser(m_pConfig->GetUserCounts());
    CHECK_OBJ(pUser);
    SAFESTRCPY(pUser->sUser,sizeof(pUser->sUser),m_pUser->sUser);
    SAFESTRCPY(pUser->sPwd,sizeof(pUser->sPwd),m_pUser->sPwd);
    SAFESTRCPY(pUser->sAccess,sizeof(pUser->sAccess),m_pUser->sAccess);
    m_pConfig->SetUserCounts(m_pConfig->GetUserCounts()+1);
    //ͬ��DBA_USER��
    CHECK_RET(SyncUser(TK_CREATE,pUser),"SyncUser failed.");
    //���ɶ�Ӧ���ļ�
    m_pScript->CreateUser(pUser,true);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteAlterUser
* ��������	:  �޸��û���Ϣ
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteAlterUser()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char sPwd[MAX_NAME_LEN] = {0};
    TMDbUser* pUser = m_pConfig->GetUser(m_pUser->sUser);
    if(pUser == NULL)
    {
        CHECK_RET(ERR_SQL_INVALID,"user[%s] does not exist.",m_pUser->sUser);
    }
    CHECK_OBJ(m_pCurOperUser);
    //�ж��޸ĵ��û��Ƿ�Ϊ����Ա�˻�
    if(TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess,"Administrator") == 0
        && TMdbNtcStrFunc::StrNoCaseCmp(m_pUser->sUser,m_pCurOperUser->sUser) != 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Not allowed to modify other admin user information.",m_pUser->sUser);
        return ERR_DB_PRIVILEGE_INVALID;
    }
    //��ֵ����
    SAFESTRCPY(pUser->sUser,sizeof(pUser->sUser),m_pUser->sUser);
    if(m_pUser->sPwd[0] == 0)
    {
        TMdbEncrypt::EncryptEx(pUser->sPwd,sPwd);
        SAFESTRCPY(m_pUser->sPwd,sizeof(m_pUser->sPwd),sPwd);
    }
    else
    {
        TMdbEncrypt::DecryptEx(m_pUser->sPwd,sPwd);
        SAFESTRCPY(pUser->sPwd,sizeof(pUser->sPwd),sPwd);
    }
    if(m_pUser->sAccess[0] == 0)
    {
        SAFESTRCPY(m_pUser->sAccess,sizeof(m_pUser->sAccess),pUser->sAccess);
    }
    else
    {
        SAFESTRCPY(pUser->sAccess,sizeof(pUser->sAccess),m_pUser->sAccess);
    }
    //ͬ������DBA_USER��
    CHECK_RET(SyncUser(TK_ALTER,m_pUser),"SyncUser failed.");
    //���ɶ�Ӧ���ļ�
    m_pScript->ModifyUser(pUser,true);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteDropUser
* ��������	:  ɾ���û�
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteDropUser()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TMDbUser* pUser = m_pConfig->GetUser(m_pUser->sUser);
    if(pUser == NULL)
    {
        CHECK_RET(ERR_SQL_INVALID,"user[%s] does not exist.",m_pUser->sUser);
    } 
    CHECK_OBJ(m_pCurOperUser);
    //�ж�ɾ�����û��Ƿ�Ϊ����Ա�˻�
    if(TMdbNtcStrFunc::StrNoCaseCmp(pUser->sAccess,"Administrator") == 0
        && TMdbNtcStrFunc::StrNoCaseCmp(m_pUser->sUser,m_pCurOperUser->sUser) != 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Not allowed to delete other admin user[%s].",m_pUser->sUser);
        return ERR_DB_PRIVILEGE_INVALID;
    }
    //ͬ������DBA_USER��
    CHECK_RET(SyncUser(TK_DROP,pUser),"SyncUser failed.");
    pUser->Clear();
    m_pConfig->SetUserCounts(m_pConfig->GetUserCounts()-1);
    
    //���ɶ�Ӧ���ļ�
    m_pScript->DropUser(m_pUser,true);
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* ��������	:  ExecuteCreateTablespace
* ��������	:  ������ռ�
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteCreateTablespace()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char sTablespaceName[MAX_NAME_LEN] = {0};
    SAFESTRCPY(sTablespaceName,sizeof(sTablespaceName),m_pMdbSqlParser->m_pDDLSqlStruct->pTablespace->sName);
    if(NULL != m_pShmDSN->GetTableSpaceAddrByName(sTablespaceName))
    {
        CHECK_RET(ERR_SQL_INVALID,"tablespace[%s] already exists.",sTablespaceName);
    }   
    
    TMdbTableSpace * pTableSpace = m_pConfig->GetIdleTableSpace();
    CHECK_OBJ(pTableSpace);
    memcpy(pTableSpace,m_pMdbSqlParser->m_pDDLSqlStruct->pTablespace,sizeof(TMdbTableSpace));
    
    m_pConfig->SetTablespaceCounts(m_pConfig->GetTableSpaceCounts()+1);
    CHECK_RET(m_mdbTSCtrl.CreateTableSpace(pTableSpace,m_pConfig,true),"CreateTableSpace failed...");

    //���ɶ�Ӧ���ļ�
    m_pScript->CreateTableSpace(pTableSpace,true);

    // if file storage table space , do checkpoint
    if(m_mdbTSCtrl.IsFileStorage())
    {
        TMdbCheckPoint tTMdbCheckPoint;
        CHECK_RET(tTMdbCheckPoint.Init(m_pShmDSN->GetInfo()->sName),"Init failed.");
        //CHECK_RET(tTMdbCheckPoint.LinkFile(m_pTable->m_sTableSpace),"Attach failed.");
        //CHECK_RET(tTMdbCheckPoint.DoCheckPoint(m_pTable->m_sTableSpace),"FlushOneTable falied");
		CHECK_RET(tTMdbCheckPoint.LinkFile(),"Attach failed.");
        CHECK_RET(tTMdbCheckPoint.DoCheckPoint(),"FlushOneTable falied");
    }
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* ��������	:  ExecuteAlterTablespace
* ��������	:  �޸ı�ռ���Ϣ
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteAlterTablespace()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TADD_ERROR(ERROR_UNKNOWN,"Does not support dynamic alter table space[%s] attributes.",\
                    m_pMdbSqlParser->m_pDDLSqlStruct->pTablespace->sName);
    iRet =  ERR_SQL_INVALID;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteDropTablespace
* ��������	:  ɾ����ռ䣬����ñ�ռ������û���Ļ���������ɾ��
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteDropTablespace()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char sTablespaceName[MAX_NAME_LEN] = {0};
    SAFESTRCPY(sTablespaceName,sizeof(sTablespaceName),m_pMdbSqlParser->m_pDDLSqlStruct->pTablespace->sName);
    TMdbTableSpace * pShmTableSapce = m_pShmDSN->GetTableSpaceAddrByName(sTablespaceName);
    if(NULL == pShmTableSapce)
    {
        CHECK_RET(ERR_SQL_INVALID,"tablespace[%s] does not exist.",sTablespaceName);
    }  
    //��������иñ�ռ��Ƿ�����û���
    TMdbTableSpace * pTableSapce = m_pConfig->GetTableSpaceByName(sTablespaceName);
    CHECK_OBJ(pTableSapce);
    for(int i = 0;i<MAX_TABLE_COUNTS;i++)
    {
        TMdbTable* pTmp = m_pConfig->GetTableByPos(i);
        if(NULL == pTmp)
        {   
            continue;
        }
        if(0 ==  TMdbNtcStrFunc::StrNoCaseCmp(  pTmp->m_sTableSpace, pTableSapce->sName))
        {
            TADD_ERROR(ERROR_UNKNOWN,"There are other users table in tablespace,not allowed to delete the tablespace[%s].",sTablespaceName);
            return ERR_SQL_INVALID;
        }
    }
    pTableSapce->Clear();
    m_pConfig->SetTablespaceCounts(m_pConfig->GetTableSpaceCounts()-1);
    
    //��鹲���ڴ��иñ�ռ����Ƿ����û���
    TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
    for(;itor != m_pShmDSN->m_TableList.end();++itor)
    {
        TMdbTable * pTable = &(*itor);
        if(0 ==  TMdbNtcStrFunc::StrNoCaseCmp(pTable->m_sTableSpace , pShmTableSapce->sName))
        {
            TADD_ERROR(ERROR_UNKNOWN,"There are other users table in tablespace[%s],not allowed to delete.",sTablespaceName);
            return ERR_SQL_INVALID;
        }
    }
    //���ɶ�Ӧ���ļ�
    m_pScript->DropTableSpace(sTablespaceName,true);

    // if file storage table space , do checkpoint
    CHECK_RET(m_mdbTSCtrl.Init(m_pDsn->sName,sTablespaceName),"m_mdbTSCtrl.Init failed");
    if(m_mdbTSCtrl.IsFileStorage())
    {
        TMdbCheckPoint tTMdbCheckPoint;
        CHECK_RET(tTMdbCheckPoint.Init(m_pShmDSN->GetInfo()->sName),"Init failed.");
       // CHECK_RET(tTMdbCheckPoint.LinkFile(m_pTable->m_sTableSpace),"Attach failed.");
      //  CHECK_RET(tTMdbCheckPoint.DoCheckPoint(m_pTable->m_sTableSpace),"FlushOneTable falied");
		CHECK_RET(tTMdbCheckPoint.LinkFile(),"Attach failed.");
        CHECK_RET(tTMdbCheckPoint.DoCheckPoint(),"FlushOneTable falied");
    }
    
    pShmTableSapce->Clear();
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteModifyTableAttr
* ��������	:  �޸ı����� 
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteModifyTableAttr()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bIsCreateRep = false;
    CHECK_OBJ(m_pTable);
    TMdbTable * pTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);
    CHECK_OBJ(pTable);
    //�Ե�ǰ���ʱ����
    CHECK_RET(pTable->tTableMutex.Lock(true,&m_pDsn->tCurTime),"lock failed.");
    do
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"is-read-lock") == 0)
        {
            CONVERT_Y_N(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue,pTable->bReadLock);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"shard-backup") == 0)
        {
            TADD_WARNING("Modify shard-backup attribute,need to restart the business process.");
            CONVERT_Y_N(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue,pTable->m_bShardBack);
            bIsCreateRep = true;
        }
        /*else if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"table-level") == 0)
        {
            CONVERT_Y_N(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue,pTable->m_bShardBack);
        }*/
        else if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"is-write-lock") == 0)
        {
            CONVERT_Y_N(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue,pTable->bWriteLock);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"is-rollback") == 0)
        {
            CONVERT_Y_N(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue,pTable->bRollBack);		
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"checkPrikey") == 0)
        {
            CONVERT_Y_N(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue,pTable->bIsCheckPriKey);			
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"loadtype") == 0)
        {
            pTable->iLoadType = TMdbNtcStrFunc::StrToInt(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"filter-sql") == 0)
        {
            SAFESTRCPY(pTable->m_sFilterSQL,sizeof(pTable->m_sFilterSQL),m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"load-sql") == 0)
        {
            CHECK_RET_BREAK(m_pScript->CheckSQLValidity(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue),"CheckSQLValidity load-sql failed");
            SAFESTRCPY(pTable->m_sLoadSQL,sizeof(pTable->m_sLoadSQL),m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"flush-sql") == 0)
        {
            CHECK_RET_BREAK(m_pScript->CheckSQLValidity(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue),"CheckSQLValidity flush-sql failed");
            TADD_WARNING("Modify flush-sql attribute,need to restart the mdbFlushFromOra process.");
            SAFESTRCPY(pTable->m_sFlushSQL,sizeof(pTable->m_sFlushSQL),m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr,"rep-type") == 0)
        {
            TADD_WARNING("Modify rep-type attribute,need to restart the business process.");
            pTable->iRepAttr = m_pConfig->GetRepType(m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue);
			if(pTable->iRepAttr ==  REP_TO_DB_MDB)
			{
				pTable->iRepAttr = REP_TO_DB;
				pTable->m_bShardBack = true;
			}
				
            if(pTable->iRepAttr < 0)
            {
                CHECK_RET_BREAK(ERR_TAB_ATTR_NOT_EXIST,"table rep_type[%s] error",\
                        m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttrValue);
            }
            //�޸������е�ͬ������
            //CHECK_RET_BREAK(ModifColumnRepType(pTable),"BatchModifColumnRepType failed.");
            bIsCreateRep = true;
        }
        else
        {
            CHECK_RET_BREAK(ERR_TAB_ATTR_NOT_EXIST,"Table attributes [%s] can not be modified.",\
                         m_pMdbSqlParser->m_pDDLSqlStruct->sTableAttr);
        }
    }while(0);
    pTable->iMagic_n++;
    CHECK_RET(pTable->tTableMutex.UnLock(true),"unlock failed.");
    //���ɶ�Ӧ���ļ�
    m_pScript->ModifyTableAttribute(true);
    //����ͬ����
    if(bIsCreateRep)
    {
        CHECK_RET(CreateSyncShm(pTable->iRepAttr, pTable->m_bShardBack),"Can't create the shared memory.");
    }
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* ��������	:  ExecuteModifyTable
* ��������	:  �޸�������
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteModifyTable()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //�����ӵ��н���У��
    if(m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0] == TK_ADD)
    {
        TMdbColumn &tAddColumn = m_pTable->tColumn[m_pTable->iColumnCounts-1];
        CHECK_RET(CorrectionColumn(&tAddColumn,m_pTable),"Column attribute check fails.");
    }
    //���޸ĵ���У��
    if(m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0] == TK_MODIFY)
    {
        TMdbColumn &tModifyColumn = m_pTable->tColumn[m_pMdbSqlParser->m_pDDLSqlStruct->iModifyColumnPos];
        std::vector<string>::iterator itor = m_pMdbSqlParser->m_pDDLSqlStruct->vModifyColumnAttr.begin();
        for(;itor != m_pMdbSqlParser->m_pDDLSqlStruct->vModifyColumnAttr.end();++itor)
        {
            CHECK_RET(CorrectionColumn(&tModifyColumn,m_pTable,itor->c_str()),"Column attribute check fails.");
        }
    }
    //�����ƫ������Ϣ
    m_pConfig->SetColOffset(m_pTable);
    //���¼���
    CHECK_RET(m_pConfig->CalcOneRecordSize(m_pTable), "CalcOneRecordSize failed.");
    
    //���ȴ���һ����ʱ��
    char sTmpTableName[MAX_NAME_LEN] ={0};
    snprintf(sTmpTableName,sizeof(sTmpTableName),"%s_bak",m_pTable->sTableName);
    CHECK_RET(CreateTmpTable(sTmpTableName),"Can not create temp table[%s].",m_pTable->sTableName);
    TMdbTable *pTmpTable = m_pShmDSN->GetTableByName(sTmpTableName);
    CHECK_OBJ(pTmpTable);
    TMdbTable * pOldTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);
    CHECK_OBJ(pOldTable);

    //���ϱ�����Ǩ�Ƶ��½���ʱ��ȥ
    CHECK_RET(MigDataFromOldTONewTable(pOldTable,pTmpTable,false),\
        "Failure of migrating data from the old table[%s] into the new table[%s].",\
            pOldTable->sTableName,pTmpTable->sTableName);
    
    //ɾ���ϱ�
    CHECK_RET(ExecuteDropTable(m_pTable->sTableName,false),\
                "Can not delete the old table[%s].",m_pTable->sTableName);

    //�½���
    CHECK_RET(ExecuteCreateTable(false),"Can not create the new table[%s].",\
                                m_pTable->sTableName);

    //������ʱ�����ݵ�Ŀ���
    TMdbTable * pNewTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);
    CHECK_OBJ(pNewTable);
    CHECK_RET(MigDataFromOldTONewTable(pTmpTable,pNewTable,false),\
        "Failure of migrating data from the old table[%s] into the new table[%s].",\
            pTmpTable->sTableName,pNewTable->sTableName);
 
    //ɾ����ʱ��
    CHECK_RET(ExecuteDropTable(pTmpTable->sTableName,false),\
                "Can not delete the temp table[%s].",pTmpTable->sTableName);

    //�����������ļ���Ϣ
    TMdbTable *pConfigTable = m_pConfig->GetTableByName(pNewTable->sTableName);
    CHECK_OBJ(pConfigTable);
    pConfigTable->Clear();
    memcpy(pConfigTable,pNewTable,sizeof(TMdbTable));
    pNewTable->iMagic_n++;
    //�޸Ķ�Ӧ�ı�ṹXML�ļ�
    if(m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0] == TK_ADD)
    {
        m_pScript->AddColumn(m_pTable,true, false);//onlineģʽ��дupdate�ļ�
    }
    /*else if(m_pMdbSqlParser->m_pDDLSqlStruct->iSqlType[0] == TK_DROP)
    {
        m_pScript->DropColumn(m_pMdbSqlParser,true);
    }*/
    else
    {
        m_pScript->ModifyColumn(true);
    }

    // if file storage table space , do checkpoint
    //miaojx add
    CHECK_RET(m_mdbTSCtrl.Init(m_pDsn->sName,m_pTable->m_sTableSpace),"m_mdbTSCtrl.Init failed");
	
    if(m_mdbTSCtrl.IsFileStorage())
    {
        TMdbCheckPoint tTMdbCheckPoint;
        CHECK_RET(tTMdbCheckPoint.Init(m_pShmDSN->GetInfo()->sName),"Init failed.");
       // CHECK_RET(tTMdbCheckPoint.LinkFile(m_pTable->m_sTableSpace),"Attach failed.");
       // CHECK_RET(tTMdbCheckPoint.DoCheckPoint(m_pTable->m_sTableSpace),"FlushOneTable falied");
        CHECK_RET(tTMdbCheckPoint.LinkFile(),"Attach failed.");
       CHECK_RET(tTMdbCheckPoint.DoCheckPoint(),"FlushOneTable falied");
    }
    
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  CreateTmpTable
* ��������	:  ����һ����ʱ��
* ����		:  
* ���		:  ��ʱ��ID
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:
*******************************************************************************/
int TMdbDDLExecuteEngine::CreateTmpTable(const char* psTableName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char cTmpIndexName[MAX_NAME_LEN] = {0};
    CHECK_OBJ(m_pTable);
    TMdbTable * pNewTable = NULL;
    TMdbTable * pSrcTable = new(std::nothrow) TMdbTable();
    pSrcTable->Clear();

    TMdbTable * pTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);
    CHECK_OBJ(pTable);
    memcpy(pSrcTable,pTable,sizeof(TMdbTable));
    SAFESTRCPY(pSrcTable->sTableName, sizeof(pSrcTable->sTableName), psTableName);
    SAFESTRCPY(pSrcTable->m_sTableSpace, sizeof(pSrcTable->m_sTableSpace), pTable->m_sTableSpace);
    m_pShmDSN->AddNewTable(pNewTable,pSrcTable);
    
    //������ʱ����
    for(int i=0; i<pNewTable->iIndexCounts; ++i)
    {
        memset(cTmpIndexName,0,sizeof(cTmpIndexName));
        SAFESTRCPY(cTmpIndexName,sizeof(cTmpIndexName),pTable->tIndex[i].sName);
        snprintf(pNewTable->tIndex[i].sName,sizeof(pNewTable->tIndex[i].sName),"temp_%s",cTmpIndexName);
    }
    
    //�����Ļ��������ͳ�ͻ����
    TMdbIndexCtrl tIndexCtrl;
    tIndexCtrl.AttachDsn(m_pShmDSN);
    CHECK_RET(tIndexCtrl.AddTableIndex(pNewTable,m_pConfig->GetDSN()->iDataSize),\
               "Can't InitIndexMem table=%s.",pNewTable->sTableName);

    SAFE_DELETE(pSrcTable);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  CheckColumnInTable
* ��������	:  ���ָ�������Ƿ�����ĳ�ֱ�
* ����		:  
* ���		:
* ����ֵ	:  true ����;false ������
* ����		:  cao.peng
*******************************************************************************/
bool TMdbDDLExecuteEngine::CheckColumnInTable(const char* pClomName,const TMdbTable * pTable)
{
    TADD_FUNC("Start.");
    for(int i=0; i<pTable->iColumnCounts; ++i)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(pClomName,pTable->tColumn[i].sName) == 0)
        {
            return true;
        }
    }
    TADD_FUNC("Finish.");
    return false;
}

/******************************************************************************
* ��������	:  InitSelectSQL
* ��������	:  ��ʼ��ָ����Ĳ�ѯSQL
* ����		:  
* ���		:
* ����ֵ	:  ��
* ����		:  cao.peng
*******************************************************************************/
void TMdbDDLExecuteEngine::InitSelectSQL(const TMdbTable * pTable)
{
    TADD_FUNC("Start.");
    memset(m_sSSQL,0,MAX_SQL_LEN);
    SAFESTRCPY(m_sSSQL,MAX_SQL_LEN,"select ");
    bool bFlag = false;
    for(int j=0; j<pTable->iColumnCounts; ++j)
    {
        if(bFlag == false)
        {
            sprintf(m_sSSQL+strlen(m_sSSQL), " %s", pTable->tColumn[j].sName);
            bFlag = true;
        }
        else
        {
            sprintf(m_sSSQL+strlen(m_sSSQL), ", %s", pTable->tColumn[j].sName);
        }
    }
    sprintf(m_sSSQL+strlen(m_sSSQL), " from %s", pTable->sTableName);
    TADD_NORMAL("MDB-SQL=%s\n",m_sSSQL);
    TADD_FUNC("Finish.");
    return;
}

/******************************************************************************
* ��������	:  InitInsertSQL
* ��������	:  ��ʼ��ָ����Ĳ���SQL���
* ����		:  
* ���		:
* ����ֵ	:  ��
* ����		:  cao.peng
*******************************************************************************/
void TMdbDDLExecuteEngine::InitInsertSQL(const TMdbTable * pTable)
{
    TADD_FUNC("Start.");
    bool bFirstComlun = true;
    memset(m_sISQL, 0, sizeof(m_sISQL));
    //��ʼ��MDB-SQL
    for(int i=0; i<pTable->iColumnCounts; ++i)
    {
        if(bFirstComlun)
        {
            sprintf(m_sISQL, "insert into %s(%s", pTable->sTableName, pTable->tColumn[i].sName);
            bFirstComlun = false;
        }
        else
        {
            sprintf(&m_sISQL[strlen(m_sISQL)], ", %s", pTable->tColumn[i].sName);
        }
    }

    sprintf(&m_sISQL[strlen(m_sISQL)], ") %s (", "values");

    for(int i=0; i<pTable->iColumnCounts; ++i)
    {
        if(i == 0)
        {
            sprintf(&m_sISQL[strlen(m_sISQL)], ":%s", pTable->tColumn[i].sName);
        }
        else
        {
            sprintf(&m_sISQL[strlen(m_sISQL)], ", :%s", pTable->tColumn[i].sName);
        }
    }
    m_sISQL[strlen(m_sISQL)] = ')';
    TADD_DETAIL("Insert SQL,m_sMSQL=[%s].",m_sISQL);
    TADD_FUNC("Finish.");
    return;
}

/******************************************************************************
* ��������	:  IsValidNewTable
* ��������	:  �ж��Ƿ���ϵͳ��(ϵͳ��������)�Լ��ڹ����ڴ����Ƿ����
* ����		:  
* ���		:
* ����ֵ	:  true��false
* ����		:  cao.peng
*******************************************************************************/
bool TMdbDDLExecuteEngine::IsValidNewTable(const char * pTableName)
{
    TADD_FUNC("Begin."); 
    //�ж��Ƿ���ϵͳ��
    if(IsSysTable(pTableName))
    {
        TADD_ERROR(ERROR_UNKNOWN,"system-table=[%s] is invalid.",pTableName);
        return false; 	
    }
    
    //�жϱ����Ƿ��Ѵ���
    if(m_pShmDSN->GetTableByName(pTableName) != NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Table = [%s] does exist.",pTableName);
        return false;
    }
    
    //�жϱ��ID�Ƿ��Ѿ�����
    if(m_pShmDSN->GetTableByName(m_pTable->sTableName) != NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Table(%s) ID = [%s] does exist.",pTableName,m_pTable->sTableName);
        return false;
    }
    TADD_FUNC("Finish."); 
    return true;
}

/******************************************************************************
* ��������	:  IsSysTable
* ��������	:  �ж�ָ���ı��Ƿ�Ϊϵͳ��
* ����		:  pTableName - ����
* ���		:
* ����ֵ	:  true - �ǣ�false -����ϵͳ��
* ����		:  cao.peng
*******************************************************************************/
bool TMdbDDLExecuteEngine::IsSysTable(const char * pTableName)
{
    TADD_FUNC("Begin."); 
    //�����Ƿ�Ϊϵͳ��
    if(strlen(pTableName) > 4)
    {
        char sHead[32] = {0};
        memset(sHead, 0, sizeof(sHead));
        strncpy(sHead, pTableName, 4);
        if(TMdbNtcStrFunc::StrNoCaseCmp(sHead, "dba_") == 0)
        {
            return true;
        }
    }
    TADD_FUNC("Finish."); 
    return false;
}

/******************************************************************************
* ��������	:  ConnectMDB
* ��������	:  ����QMDB
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ConnectMDB()
{
    TADD_FUNC("Begin."); 
    int iRet =  0; 
    CHECK_OBJ(m_pDsn);
    try
    {
        if(!m_tMDB.ConnectAsMgr(m_pDsn->sName))
        {
            TADD_ERROR(ERROR_UNKNOWN,"Can't Connect [%s]", m_pDsn->sName);
            return -1;
        } 
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());
        return ERR_DB_NOT_CONNECTED;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
        return ERR_DB_NOT_CONNECTED;
    }
    TADD_FUNC("Finish."); 
    return iRet;
}

/******************************************************************************
* ��������	:  MigDataFromOldTONewTable
* ��������	:  ��������Ǩ��(��һ�ű�����һ�ű�) 
* ����		:  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::MigDataFromOldTONewTable(TMdbTable * pOldTable,TMdbTable * pNewTable,bool bIsRep)
{
    TADD_FUNC("Begin."); 
    int iRet =  0;
    CHECK_OBJ(pOldTable);
    CHECK_OBJ(pNewTable);

    memset(m_sISQL,0,sizeof(m_sSSQL));
    memset(m_sSSQL,0,sizeof(m_sSSQL));
    InitInsertSQL(pNewTable);
    InitSelectSQL(pOldTable);
    CHECK_RET(ConnectMDB(),"connect QMDB Faild.");
    
    TMdbQuery *pQueryOld = m_tMDB.CreateDBQuery();
    TMdbQuery *pQueryNew = m_tMDB.CreateDBQuery();
    CHECK_OBJ(pQueryOld);
    CHECK_OBJ(pQueryNew);
    try
    {
        if(bIsRep)
        {
            pQueryNew->CloseSQL();
            pQueryNew->SetSQL(m_sISQL);
            pQueryOld->CloseSQL();
            pQueryOld->SetSQL(m_sSSQL);
        }
        else
        {
            pQueryNew->CloseSQL();
            pQueryNew->SetSQL(m_sISQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH|QUERY_NO_REDOFLUSH,0);

            pQueryOld->CloseSQL();
            pQueryOld->SetSQL(m_sSSQL,QUERY_NO_ORAFLUSH |QUERY_NO_SHARDFLUSH|QUERY_NO_REDOFLUSH,0);
        }
       
        pQueryOld->Open();
        while(pQueryOld->Next())
        {
            int m = 0;
            for(int i=0; i<pNewTable->iColumnCounts; ++i)
            {
                //����±��ֶ����ϱ����Ƿ����,�������ֶα�����Ĭ��ֵ
                if(!CheckColumnInTable(pNewTable->tColumn[i].sName,pOldTable))
                {
                    TADD_DETAIL("pTmpTable->tColumn[i].sName = %s",pNewTable->tColumn[i].sName);
                    TADD_DETAIL("pTmpTable->tColumn[i].m_bNullable = %d",pNewTable->tColumn[i].m_bNullable);
                    if(pNewTable->tColumn[i].m_bNullable)
                    {
                        pQueryNew->SetParameterNULL(i);
                    }
                    if(pNewTable->tColumn[i].iDataType == DT_Int)
                    {
                        pQueryNew->SetParameter(i, TMdbNtcStrFunc::StrToInt(pNewTable->tColumn[i].iDefaultValue));
                    }
                    else
                    {
                        pQueryNew->SetParameter(i, pNewTable->tColumn[i].iDefaultValue);
                    }
                    continue;
                }
                
                //ͬʱ��Ҫ����ϱ���ֶ����±����Ƿ����
                if(!CheckColumnInTable(pOldTable->tColumn[m].sName,pNewTable))
                {
                    m++;
                }
                
                TADD_DETAIL("pTmpTable->tColumn[i].sName = %s",pNewTable->tColumn[i].sName);
                switch(pNewTable->tColumn[i].iDataType)
                {
                    case DT_Int:
                    {
                        if( pQueryOld->Field(m).isNULL())
                        {
                            pQueryNew->SetParameterNULL(i);
                        }
                        else
                        {
                            pQueryNew->SetParameter(i, pQueryOld->Field(m).AsInteger());
                        }
                        break;
                    }
                    case DT_Blob:
                    {
                        char temp[MAX_BLOB_LEN]={0};
                        SAFESTRCPY(temp,sizeof(temp),pQueryOld->Field(m).AsString());
                        pQueryNew->SetParameter(pNewTable->tColumn[i].sName,(char*)temp);
                        break;
                    }
                    //�ַ�������
                    case DT_Char:
                    case DT_VarChar:
                    {
                        if( pQueryOld->Field(m).isNULL())
                        {
                            pQueryNew->SetParameterNULL(i);
                        }
                        else
                        {
                            pQueryNew->SetParameter(i, pQueryOld->Field(m).AsString());
                        }
                        break;
                    }  
                    default:
                    {
                        if( pQueryOld->Field(m).isNULL())
                        {
                            pQueryNew->SetParameterNULL(i);
                        }
                        else
                        {
                            pQueryNew->SetParameter(i, pQueryOld->Field(m).AsString());
                        }
                        break;
                    }
                }
                m++;
            }    
            pQueryNew->Execute();
            pQueryNew->Commit();
        }
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN," ERROR_SQL=%s.\nERROR_MSG=%s.\n", e.GetErrSql(), e.GetErrMsg());
        iRet = -1;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown Error.");
        iRet = -1;
    }
    SAFE_DELETE(pQueryOld);
    SAFE_DELETE(pQueryNew);
    m_tMDB.Disconnect();
    TADD_FUNC("Finish."); 
    return iRet;
}

/******************************************************************************
* ��������	:  SyncUser
* ��������	:  ����DBA_USER���¼ͬ��
* ����		:  iOptType ͬ�����ͣ�TK_CREATE ��ʾ�����û���TK_ALTER��ʾ�޸��û���
*           :  TK_DROP��ʾɾ���û�;pUser �û���Ϣ
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::SyncUser(int iOptType,TMDbUser* pUser)
{
    TADD_FUNC("Begin."); 
    int iRet = 0;
    TMdbQuery *pQuery = NULL;
    CHECK_OBJ(pUser);
    char sISQL[MAX_SQL_LEN] = "insert into dba_user "
                       "(user_name, user_pwd, access_attr) values ("
                       ":user_name, :user_pwd, :access_attr)";
    char sUSQL[MAX_SQL_LEN] = "update dba_user set user_pwd=:user_pwd,access_attr=:access_attr"
                       " where user_name = :user_name";
    char sDSQL[MAX_SQL_LEN] = "delete from dba_user where user_name=:user_name ";
    char sSSQL[MAX_SQL_LEN] = "select user_name from dba_user where user_name=:user_name ";

    try
    {
        //�����ڴ����ݿ�
        if(!m_tMDB.IsConnect())
        {
            CHECK_RET(ConnectMDB(),"connect QMDB Faild.");
        }
        pQuery = m_tMDB.CreateDBQuery();
        CHECK_OBJ(pQuery);

        //���Ȳ�ѯ�����û��Ƿ��Ѿ�����DBA_USER����
        pQuery->SetSQL(sSSQL,QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);
        pQuery->SetParameter("user_name",pUser->sUser);
        pQuery->Open();
        bool bInMdb = pQuery->Next();
        //ѡ��dba_userͬ������
        if(iOptType == TK_CREATE)
        {
            if(bInMdb)
            {
                TADD_WARNING("New user[%s] already exists in the dba_user table.",pUser->sUser);
            }
            else
            {
                TADD_FLOW("user_name=[%s], user_pwd=[%s], access_attr=[%s].",\
                    pUser->sUser, pUser->sPwd, pUser->sAccess);
                pQuery->SetSQL(sISQL,QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);
                pQuery->SetParameter("user_name",   pUser->sUser);
                pQuery->SetParameter("user_pwd",    pUser->sPwd);
                pQuery->SetParameter("access_attr", pUser->sAccess);
                pQuery->Execute();
                pQuery->Commit(); 
            }
        }
        else if(iOptType == TK_ALTER)
        {
            //����޸ĵ��û���dba_user�в����ڣ�ֱ�Ӳ���һ����¼���������
            if(!bInMdb)
            {
                TADD_FLOW("user_name=[%s], user_pwd=[%s], access_attr=[%s].",\
                    pUser->sUser, pUser->sPwd, pUser->sAccess);
                pQuery->SetSQL(sISQL,QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);
                pQuery->SetParameter("user_name",   pUser->sUser);
                pQuery->SetParameter("user_pwd",    pUser->sPwd);
                pQuery->SetParameter("access_attr", pUser->sAccess);
                pQuery->Execute();
                pQuery->Commit(); 
            }
            else
            {
                TADD_FLOW("user_name=[%s], user_pwd=[%s], access_attr=[%s].",\
                    pUser->sUser, pUser->sPwd, pUser->sAccess);
                pQuery->SetSQL(sUSQL,QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);
                pQuery->SetParameter("user_pwd",    pUser->sPwd);
                pQuery->SetParameter("access_attr", pUser->sAccess);
                pQuery->SetParameter("user_name",   pUser->sUser);
                pQuery->Execute();
                pQuery->Commit(); 
            }
        }
        else
        {
            //ɾ�����û���dba_user�в����ڣ���ֻ��ӡ�澯��Ϣ
            if(!bInMdb)
            {
                TADD_WARNING("The user[%s] does not exist in the dba_user table.",pUser->sUser);
            }
            else
            {
                pQuery->SetSQL(sDSQL,QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH,0);
                pQuery->SetParameter("user_name",   pUser->sUser);
                pQuery->Execute();
                pQuery->Commit(); 
            }
        }
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN," ERROR_SQL=%s.\nERROR_MSG=%s.\n", e.GetErrSql(), e.GetErrMsg());
        iRet = -1;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown Error.");
        iRet = -1;
    }
    SAFE_DELETE(pQuery);
    TADD_FUNC("Finish."); 
    return iRet;
}

/******************************************************************************
* ��������	:  LoadDataFromRepOrOracle
* ��������	:  �½���ʱ��Ҫ�ӱ����������ݻ��ߴ�oracle������
* ����		:  pTableName  ����
*           :  
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::LoadDataFromRepOrOracle(const char *pTableName)
{
    TADD_FUNC("Begin."); 
    int iRet = 0;
	/*
    //bool bIsRepSuccess = false;
    char sPrintBuf[MAX_PATH_NAME_LEN] = {0};
    CHECK_OBJ(pTableName);
    TMdbTable * pTable = m_pShmDSN->GetTableByName(pTableName);
    CHECK_OBJ(pTable);

    //�ȴӱ�������
    if(IsLoadFromRep(pTable))
    {
        TMdbLoadFromPeer tLoadRep;
        CHECK_RET(tLoadRep.Init(m_pConfig),"LoadRep.Init failed....");
        //ͨ���ж�loadTable�Ľ�����ж��Ƿ��ڴ�oracle����
        if(tLoadRep.LoadTable(pTable->sTableName) != 0 
            || tLoadRep.GetTableLoadRecords() == 0)
        {
            bIsRepSuccess = false;
        }
        else
        {
            snprintf(sPrintBuf,sizeof(sPrintBuf),\
                "Load table=[%s] finished,Total record counts=[%d]",\
                pTable->sTableName,tLoadRep.GetTableLoadRecords());
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,sPrintBuf);
            bIsRepSuccess = true;
        }
    }

    //����ӱ�������ʧ��(����û���������߱�����û��ͬ����)�����oracle��������
    if(IsLoadFromOracle(pTable) && !bIsRepSuccess)
    {
        TMdbReLoadFromOra tLoadOra;
        CHECK_RET(tLoadOra.Init(m_pDsn->sName),"loadOra.Init failed....");
        CHECK_RET(tLoadOra.LoadAll(pTable->sTableName),\
            "Upload table[%s] data from the oracle failed.",pTable->sTableName);   
    }
    */
    TADD_FUNC("Finish."); 
    return iRet;
}

/******************************************************************************
* ��������	:  IsLoadFromOracle
* ��������	:  �ж��Ƿ���Ҫ��oracle����������
* ����		:  pTable ���ر����
*           :  
* ���		:
* ����ֵ	:  true:��Ҫ��false:����Ҫ
* ����		:  cao.peng
*******************************************************************************/
bool TMdbDDLExecuteEngine::IsLoadFromOracle(TMdbTable * pTable)
{
    TADD_FUNC("Begin."); 
    bool bLoadFromOra = false;
    if(!m_pDsn->m_bIsOraRep)
    {
        return bLoadFromOra;
    }
    //�ж��Ƿ���Ҫ��Oracle����
    if(pTable->iRepAttr == REP_FROM_DB 
        || pTable->iRepAttr == REP_TO_DB)
    {
        bLoadFromOra = true;
    }
    TADD_FUNC("Finish."); 
    return bLoadFromOra;
}

/******************************************************************************
* ��������	:  IsLoadFromRep
* ��������	:  �ж��Ƿ���Ҫ�ӱ�����������
* ����		:  pTable ���ر����
*           :  
* ���		:
* ����ֵ	:  true:��Ҫ��false:����Ҫ
* ����		:  cao.peng
*******************************************************************************/
bool TMdbDDLExecuteEngine::IsLoadFromRep(TMdbTable * pTable)
{
    TADD_FUNC("Begin."); 
    bool bLoadFromRep = false;
    //�ж��Ƿ���Ҫ��Oracle����
    if(!m_pDsn->m_bIsRep)
    {
        return bLoadFromRep;
    }

    return pTable->m_bShardBack;
    
    TADD_FUNC("Finish."); 
    return bLoadFromRep;
}

/******************************************************************************
* ��������	:  CorrectionColumn
* ��������	:  �䳤�洢�����Ⱥ�����ת���Լ�����У��
* ����		:  pColumn �ж���;bIsZipTime �Ƿ�ѹ��,��Ҫ����ʱ����
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::CorrectionColumn(TMdbColumn *pColumn,TMdbTable * pTable,const char *pColumnAttr)
{
    TADD_FUNC("Begin."); 
    int iRet = 0;
    CHECK_OBJ(pColumn);
    //������
    if(!pColumnAttr)
    {
        if(pColumn->iDataType <= 0)//У����������
        {
            TADD_ERROR(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"Invalid datatype[%d] of ColumnName=[%s]",pColumn->iDataType,pColumn->sName);
            return ERR_TAB_COLUMN_DATA_TYPE_INVALID;
        }
    
        if(false == pColumn->bIsDefault)//������Ĭ��ֵ
        {
            TADD_ERROR(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"no default value of ColumnName=[%s]",pColumn->sName);
            return ERR_TAB_COLUMN_DATA_TYPE_INVALID;
        }
        
    }
    
    //ֻ�е��޸��еĳ�������ʱ������ҪУ���еĳ���
    if(!pColumnAttr
        ||( pColumnAttr !=NULL && TMdbNtcStrFunc::StrNoCaseCmp(pColumnAttr,"data-len") == 0))
    {
        if(pColumn->iColumnLen < 16  
            && (pColumn->iDataType==DT_Blob||pColumn->iDataType==DT_VarChar))
        {
            TADD_WARNING("ColoumName=[%s],ColoumType=VARCHAR|BLOB, ColoumLen=[%d] is too small",\
                        pColumn->sName,pColumn->iColumnLen);
            pColumn->iDataType = DT_Char;
        }
        //int���͵��г���ͳһΪ8
        if(pColumn->iDataType == DT_Int)
        {
            pColumn->iColumnLen = 8;
        }
        else if(pColumn->iDataType == DT_DateStamp)
        {
            if(pColumn->bIsDefault)
            {
                if(strlen(pColumn->iDefaultValue) != 14)
                {
                    TADD_ERROR(ERR_TAB_COLUMN_LENGTH_INVALID,"ColumnName=[%s],default_value length is wrong,it must=14.",pColumn->sName);
                    return ERR_TAB_COLUMN_LENGTH_INVALID;
                }
            }
            pColumn->iColumnLen = pTable->GetTimeValueLength();
        }
        else
        {
            if(pColumn->iColumnLen <= 0 || pColumn->iColumnLen >= MAX_BLOB_LEN-1)
            {
                TADD_ERROR(ERR_TAB_COLUMN_LENGTH_INVALID,"ColumnName=[%s], invalid length[%d]", pColumn->sName,pColumn->iColumnLen);
                return ERR_TAB_COLUMN_LENGTH_INVALID;
            }
        
            //�����Ĭ��ֵ������Ҫȷ��Ĭ��ֵ�ĳ��Ȳ��ɳ���ǰ�������õĳ���
            if(pColumn->bIsDefault)
            {
                if(pColumn->iColumnLen < (int)strlen(pColumn->iDefaultValue))
                {
                    TADD_ERROR(ERR_TAB_COLUMN_LENGTH_INVALID,"ColumnName=[%s], default_value length must < ColumnLength", pColumn->sName);
                    return ERR_TAB_COLUMN_LENGTH_INVALID;
                }
            }
            pColumn->iColumnLen += 1;
        }
    }
   
    TADD_FUNC("Finish."); 
    return iRet;
}

/******************************************************************************
* ��������	:  AddSequenceToOracle
* ��������	:  ����oracle��mdb_sequence���м�¼
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::AddSequenceToOracle()
{
    TADD_FUNC("Begin."); 
    int iRet = 0;
    /*
    char sSQL[MAX_SQL_LEN] = {0};
    char sLocalIP[MAX_IP_LEN] = {0};
    TMDBDBQueryInterface *pInsertOra = NULL;
    snprintf(sSQL,sizeof(sSQL),"%s%s","insert into ",m_pTable->sTableName);
    snprintf(sSQL+strlen(sSQL),sizeof(sSQL)-strlen(sSQL),"%s",\
        "(SEQ_NAME,DSN_NAME,START_NUMBER,END_NUMBER,STEP_NUMBER,CUR_NUMBER,LOCAL_IP) "); 
    snprintf(sSQL+strlen(sSQL),sizeof(sSQL)-strlen(sSQL),"%s",\
        "VALUES(:SEQ_NAME,:DSN_NAME,:START_NUMBER,:END_NUMBER,:STEP_NUMBER,:CUR_NUMBER,:LOCAL_IP)");
        */
    //У���������������ڴ����Ƿ����
    TMemSeq& tMemSeq = m_pMdbSqlParser->m_pDDLSqlStruct->tMemSeq;
    TMemSeq* pSeq = m_pShmDSN->GetMemSeqByName(tMemSeq.sSeqName);
    if(pSeq != NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Sequences[%s] already exists in the current DSN[%s],Check to see if you want to modify the sequence name.",\
                    pSeq->sSeqName,m_pDsn->sName);
        return ERR_DB_SEQUECE_EXISTS;
    }
    /*
    //�ж��û���û�и�LOCAL_IP�����û����ӵ�ǰDSN�����ļ��ж�ȡ
    if(strlen(m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sLocalIP) == 0)
    {
        SAFESTRCPY(sLocalIP,sizeof(sLocalIP),m_pDsn->sLocalIP);
    }
    else
    {
        SAFESTRCPY(sLocalIP,sizeof(sLocalIP),m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sLocalIP);
    }
    */
    //У�鵱ǰ����ֵ�Ƿ�Ϊ��ʼ����+N*����
    if(tMemSeq.iCur != 0 && (tMemSeq.iCur - tMemSeq.iStart)%tMemSeq.iStep != 0)
    {
        TADD_WARNING("Suggest the current sequence[%s] value is equal to the starting value plus n times step value.",\
            tMemSeq.sSeqName);
    }
    try
    {
        /*
        //����oracle�����б�
        CHECK_RET(ConnectOracle(),"Connect Oracle Faild.");
        pInsertOra = m_pDBLink->CreateDBQuery();
        CHECK_OBJ(pInsertOra);
        pInsertOra->SetSQL(sSQL);
        pInsertOra->SetParameter("SEQ_NAME",tMemSeq.sSeqName);
        pInsertOra->SetParameter("DSN_NAME",m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sName);
        pInsertOra->SetParameter("START_NUMBER",tMemSeq.iStart);
        pInsertOra->SetParameter("END_NUMBER",tMemSeq.iEnd);
        pInsertOra->SetParameter("STEP_NUMBER",tMemSeq.iStep);
        pInsertOra->SetParameter("CUR_NUMBER",tMemSeq.iCur);
        pInsertOra->SetParameter("LOCAL_IP",sLocalIP);
        pInsertOra->Execute();
        pInsertOra->Commit();
        SAFE_DELETE(pInsertOra);
        //�ж����ӵ�����DSN�ǲ��ǵ�ǰ���ݿ�ģ�������ǣ����޸ĵ�ǰ�ڴ�����
        if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sName,m_pDsn->sName) != 0)
        {
            TADD_DETAIL("Modify the sequence belongs to the DSN[%s],Current DSN=[%s].",\
                            m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sName,m_pDsn->sName);
            return iRet;
        }
        
        //������ӵ����зǱ����ģ����������ڴ�������
        if(TMdbNtcStrFunc::StrNoCaseCmp(sLocalIP,m_pDsn->sLocalIP) !=0)
        {
            TADD_NORMAL("New sequence[%s] host for %s,without adding to the current memory.",\
                    tMemSeq.sSeqName,sLocalIP);
            return iRet;
        }
        */
        //�ڹ����ڴ�����������
        pSeq = NULL;
        m_pShmDSN->AddNewMemSeq(pSeq);
        CHECK_OBJ(pSeq);
        CHECK_RET(pSeq->tMutex.Lock(true),"lock failed.");
        SAFESTRCPY(pSeq->sSeqName,sizeof(pSeq->sSeqName),tMemSeq.sSeqName);
        pSeq->iStart = tMemSeq.iStart;
        pSeq->iEnd = tMemSeq.iEnd;
        pSeq->iCur = tMemSeq.iCur;
        pSeq->iStep = tMemSeq.iStep;
        CHECK_RET(pSeq->tMutex.UnLock(true),"unlock failed.");
    }
    catch(TMDBDBExcpInterface &e)
    {   
        //SAFE_DELETE(pInsertOra);
        TADD_ERROR(ERROR_UNKNOWN,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        return ERR_SQL_INVALID;
    }
    catch(TMdbException &e)
    {
        //SAFE_DELETE(pInsertOra);
        TADD_ERROR(ERROR_UNKNOWN,"Insert-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        return ERR_SQL_INVALID;
    }
    TADD_FUNC("Finish."); 
    return iRet;
}

/******************************************************************************
* ��������	:  DelSequenceToOracle
* ��������	:  ɾ��oracle��sequence����ָ������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::DelSequenceToOracle()
{
    TADD_FUNC("Begin."); 
    int iRet = 0;
    /*
    char sSQL[MAX_SQL_LEN] = {0};
    char sLocalIP[MAX_IP_LEN] = {0};
    TMDBDBQueryInterface *pInsertOra = NULL;
    snprintf(sSQL,sizeof(sSQL),"%s%s","delete from ",m_pTable->sTableName); 
    snprintf(sSQL+strlen(sSQL),sizeof(sSQL)-strlen(sSQL),"%s",\
        " where upper(SEQ_NAME) = upper(:SEQ_NAME) and upper(DSN_NAME)=upper(:DSN_NAME) and LOCAL_IP=:LOCAL_IP"); 
        */
    TMemSeq& tMemSeq = m_pMdbSqlParser->m_pDDLSqlStruct->tMemSeq;
    /*
    //�ж��û���û�и�LOCAL_IP�����û����ӵ�ǰDSN�����ļ��ж�ȡ
    if(strlen(m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sLocalIP) == 0)
    {
        SAFESTRCPY(sLocalIP,sizeof(sLocalIP),m_pDsn->sLocalIP);
    }
    else
    {
        SAFESTRCPY(sLocalIP,sizeof(sLocalIP),m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sLocalIP);
    }
    */
    try
    {
        /*
        //ɾ��oracle���б���ָ������
        CHECK_RET(ConnectOracle(),"Connect Oracle Faild.");
        pInsertOra = m_pDBLink->CreateDBQuery();
        CHECK_OBJ(pInsertOra);
        pInsertOra->SetSQL(sSQL);
        pInsertOra->SetParameter("SEQ_NAME",tMemSeq.sSeqName);
        pInsertOra->SetParameter("DSN_NAME",m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sName);
        pInsertOra->SetParameter("LOCAL_IP",sLocalIP);
        pInsertOra->Execute();
        pInsertOra->Commit();
        SAFE_DELETE(pInsertOra);
        
        //�ж�ɾ��������DSN�ǲ��ǵ�ǰ���ݿ�ģ�������ǣ����޸ĵ�ǰ�ڴ�����
        if(TMdbNtcStrFunc::StrNoCaseCmp(m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sName,m_pDsn->sName) != 0
            || TMdbNtcStrFunc::StrNoCaseCmp(sLocalIP,m_pDsn->sLocalIP) != 0)
        {
            TADD_NORMAL("Modify the sequence belongs to the DSN[%s],Current DSN=[%s].",\
                            m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sName,m_pDsn->sName);
            return iRet;
        }
        */
        //��չ����ڴ��ж�Ӧ������
        TMemSeq*  pSeq = m_pShmDSN->GetMemSeqByName(tMemSeq.sSeqName);
        if(pSeq == NULL){return iRet;}
        CHECK_RET(pSeq->tMutex.Lock(true),"lock failed.");
        pSeq->Clear();
        CHECK_RET(pSeq->tMutex.UnLock(true),"unlock failed.");
    }
    catch(TMDBDBExcpInterface &e)
    {
        //SAFE_DELETE(pInsertOra);
        TADD_ERROR(ERROR_UNKNOWN,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        return ERR_SQL_INVALID;
    }
    catch(TMdbException &e)
    {
        //SAFE_DELETE(pInsertOra);
        TADD_ERROR(ERROR_UNKNOWN,"Insert-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        return ERR_SQL_INVALID;
    }
    TADD_FUNC("Finish."); 
    return iRet;
}

/******************************************************************************
* ��������	:  AlterSequence
* ��������	:  �޸Ĺ����ڴ�������(oracle��������ͬ�����̴���)
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::AlterSequence()
{
    TADD_FUNC("Begin."); 
    int iRet = 0;
    //char sDSN[MAX_NAME_LEN] = {0};
    TMemSeq* pSeq = NULL;
    /*
    char sSQL[MAX_SQL_LEN] = {0};
    char sLocalIP[MAX_IP_LEN] = {0};
    TMDBDBQueryInterface *pInsertOra = NULL;
    snprintf(sSQL,sizeof(sSQL),"UPDATE %s_MDB_SEQUENCE ",m_pDsn->sName); 
    snprintf(sSQL+strlen(sSQL),sizeof(sSQL)-strlen(sSQL),"%s"," SET START_NUMBER = :START_NUMBER,"
        "END_NUMBER = :END_NUMBER,CUR_NUMBER = :CUR_NUMBER,STEP_NUMBER =:STEP_NUMBER");
    snprintf(sSQL+strlen(sSQL),sizeof(sSQL)-strlen(sSQL),"%s",\
        " WHERE upper(SEQ_NAME) = upper(:SEQ_NAME) and upper(DSN_NAME)=upper(:DSN_NAME) and LOCAL_IP=:LOCAL_IP"); 
        */
    TMemSeq& tMemSeq = m_pMdbSqlParser->m_pDDLSqlStruct->tMemSeq;

    //ͨ��������ȡDSN��
    /*
    TMdbNtcSplit mdbSplit;
    mdbSplit.SplitString(m_pTable->sTableName, '_');
    SAFESTRCPY(sDSN,sizeof(sDSN),mdbSplit[0]);
    //�ж��޸ĵ����е�DSN�Ƿ�Ϊ��ǰ��DSN��������Ǳ����˳�
    if(TMdbNtcStrFunc::StrNoCaseCmp(sDSN,m_pDsn->sName) != 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Modify the sequence[%s] in the current DSN[%s] does not exist.",\
            tMemSeq.sSeqName,m_pDsn->sName);
        return ERR_DB_SEQUECE_NOT_EXIST;
    }
    */
    //�ж��޸ĵ������Ƿ����
    pSeq = m_pShmDSN->GetMemSeqByName(tMemSeq.sSeqName);
    if(NULL == pSeq)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Can't find Sequence=[%s] .", \
            tMemSeq.sSeqName);
        return ERR_DB_SEQUECE_NOT_EXIST;
    }
    /*
    //�ж��û���û�и�LOCAL_IP�����û����ӵ�ǰDSN�����ļ��ж�ȡ
    if(strlen(m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sLocalIP) == 0)
    {
        SAFESTRCPY(sLocalIP,sizeof(sLocalIP),m_pDsn->sLocalIP);
    }
    else
    {
        SAFESTRCPY(sLocalIP,sizeof(sLocalIP),m_pMdbSqlParser->m_pDDLSqlStruct->pDsn->sLocalIP);
    }
    //����û�ָ����IP������Ϊ������IP��ַ
    if(TMdbNtcStrFunc::StrNoCaseCmp(sLocalIP,m_pDsn->sLocalIP) != 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"LOCAL_IP specified must be configured for IP[%s].",m_pDsn->sLocalIP);
        return ERR_APP_INVALID_PARAM;
    }
    */
    //У���޸ĵĵ�ǰ����ֵ�Ƿ�Ϸ�
    if(tMemSeq.iCur > 0 && (tMemSeq.iCur - pSeq->iStart)%pSeq->iStep != 0)
    {
        TADD_WARNING("Suggest the current sequence value is equal to the starting value plus n times step value.");
    }
    try
    {
        /*
        //����ORACLE������ORACLE��SEQUENCE��
        CHECK_RET(ConnectOracle(),"Connect Oracle Faild.");
        pInsertOra = m_pDBLink->CreateDBQuery();
        CHECK_OBJ(pInsertOra);
        pInsertOra->SetSQL(sSQL);
        pInsertOra->SetParameter("SEQ_NAME",tMemSeq.sSeqName);
        pInsertOra->SetParameter("DSN_NAME",m_pDsn->sName);
        pInsertOra->SetParameter("LOCAL_IP",m_pDsn->sLocalIP);
        pInsertOra->SetParameter("START_NUMBER",pSeq->iStart);
        pInsertOra->SetParameter("END_NUMBER",pSeq->iEnd);
        pInsertOra->SetParameter("CUR_NUMBER",pSeq->iCur);
        pInsertOra->SetParameter("STEP_NUMBER",pSeq->iStep);
        pInsertOra->Execute();
        pInsertOra->Commit();
        */
        
        //�����ڴ�������ֵ
        CHECK_RET(pSeq->tMutex.Lock(true),"lock failed.");
        if(tMemSeq.iStart > 0)
        {
            pSeq->iStart = tMemSeq.iStart;
        }
        if(tMemSeq.iEnd > 0)
        {
            pSeq->iEnd = tMemSeq.iEnd;
        }
        if(tMemSeq.iCur > 0)
        {
            pSeq->iCur = tMemSeq.iCur;
        }
        if(tMemSeq.iStep > 0)
        {
            pSeq->iStep = tMemSeq.iStep;
        }
        CHECK_RET(pSeq->tMutex.UnLock(true),"unlock failed.");
    }
    catch(TMDBDBExcpInterface &e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        iRet = ERR_SQL_INVALID;
    }
    catch(TMdbException &e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Insert-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        iRet =  ERR_SQL_INVALID;
    }
    //SAFE_DELETE(pInsertOra);
    TADD_FUNC("Finish."); 
    return iRet;
}

/******************************************************************************
* ��������	:  ConnectOracle
* ��������	:  ����ORACLE
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ConnectOracle()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //������Ӳ�Ϊ�գ����ж������Ƿ����������������ֱ�ӷ��أ����������������ձ��query��
    //��������ӣ����´�������
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
        m_pDBLink->SetLogin(m_pConfig->GetDSN()->sOracleUID, m_pConfig->GetDSN()->sOraclePWD, m_pConfig->GetDSN()->sOracleID);
        if(!m_pDBLink->Connect())
        {
            TADD_ERROR(ERROR_UNKNOWN,"Connect Oracle Faild,user=[%s],pwd=[%s],dsn=[%s].",\
            m_pConfig->GetDSN()->sOracleUID,m_pConfig->GetDSN()->sOraclePWD,m_pConfig->GetDSN()->sOracleID);
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
* ��������	:  CreateSyncShm
* ��������	:  ���ݱ��ͬ������������������oracleͬ����
* ����		:  iRepType ���ͬ������
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::CreateSyncShm(int iRepType, bool bShardBackUp)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_RET(m_pShmDSN->CreateSyncAreaShm(),"Can't create RepShm shared memory.");
    //��������ͬ����
    if(bShardBackUp)
    {
        //CHECK_RET(m_pShmDSN->CreateSyncAreaShm(SA_REP),"Can't create RepShm shared memory.");
        //CHECK_RET(m_pShmDSN->LockDSN(),"lock failed.");
        //m_pDsn->m_bIsRep = true;
        //CHECK_RET(m_pShmDSN->UnLockDSN(),"unlock failed.");
        
        if(false == m_pConfig->GetIsStartShardBackupRep())
        {
        //֮ǰû�б�Ϊ��Ƭ����ʱ���Ŵ���;
            CHECK_RET(m_pShmDSN->CreateShardBackupRepInfoShm(m_pDsn->sName),"Can't create shard-backup share memory.");
        }
        CHECK_RET(StartRepProcess(),"Start the standby synchronization process fails.");
    }
    //����Oracleͬ����
    if(!m_pDsn->m_bIsOraRep 
        && (iRepType == REP_TO_DB))
    {
        //CHECK_RET(m_pShmDSN->CreateSyncAreaShm(SA_ORACLE),"Can't create OraShm shared memory.");
        CHECK_RET(m_pShmDSN->LockDSN(),"lock failed.");
        m_pDsn->m_bIsOraRep = true;
        CHECK_RET(m_pShmDSN->UnLockDSN(),"unlock failed.");
        CHECK_RET(StartOraRepProcess(),"Start the oracle synchronization process fails.");
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  StartOraRepProcess
* ��������	:  ����ORacleͬ����ؽ���
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::StartOraRepProcess()
{
    TADD_FUNC("Start..");
    int iRet = 0;
    char sProcFullName[MAX_NAME_LEN] = {0};
    //�����Ҫ��Oracleͬ������������ؽ���
    if(m_pDsn->m_bIsOraRep)
    {
        //sprintf(sProcFullName,"mdbFlushOra %s",m_pDsn->sName);
        //m_tProcCtrl.Restart(sProcFullName,true);
        for(int i=-1; i<m_pConfig->GetDSN()->iOraRepCounts; ++i)
        {
            sprintf(sProcFullName, "mdbDbRep %s %d %d",m_pDsn->sName,m_pConfig->GetDSN()->iOraRepCounts, i);
            m_tProcCtrl.Restart(sProcFullName,true);
        }
        if(m_pConfig->GetIsStartFlushFromOra())
        {
            sprintf(sProcFullName, "mdbFlushFromDb %s", m_pDsn->sName);
            m_tProcCtrl.Restart(sProcFullName,true);
            //����mdbClean����,����%DSN%_MDB_CHANGE_NOTIF ����
            sprintf(sProcFullName, "mdbClean %s", m_pDsn->sName);
            m_tProcCtrl.Restart(sProcFullName,true);
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  RestartOraRepProcess
* ��������	:  ����Oracleͬ����ؽ���
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::RestartOraRepProcess()
{
    TADD_FUNC("Start..");
    int iRet = 0;
    char sProcFullName[MAX_NAME_LEN] = {0};
    //ͣ����oracle��������
    if(m_pDsn->m_bIsOraRep)
    {
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        //������״̬
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc & tMdbProc = *itor;
            if(tMdbProc.sName[0] == 0){continue;}
            if(TMdbNtcStrFunc::FindString(tMdbProc.sName,"mdbOraRep") != -1 
                || TMdbNtcStrFunc::FindString(tMdbProc.sName,"mdbFlushOra") != -1)
            {
                tMdbProc.bIsMonitor = false;
                iRet = m_tProcCtrl.StopProcess(tMdbProc.iPid,10);
                if(iRet != 0)
                {
                    tMdbProc.bIsMonitor = true;
                    break;
                }
            }
        }
        if(0 != iRet)
        {
            for(itor = tProcList.begin();itor != tProcList.end();++itor)
            {
                TMdbProc & tMdbProc = *itor; 
                if(tMdbProc.sName[0] == 0){continue;}
                if(TMdbNtcStrFunc::FindString(tMdbProc.sName,"mdbOraRep") != -1
                    || TMdbNtcStrFunc::FindString(tMdbProc.sName,"mdbFlushOra") != -1)
                {
                    tMdbProc.bIsMonitor = true;
                }
            }
        }
        else
        {//��������б���oracle���ݽ���
            for(itor = tProcList.begin();itor != tProcList.end();++itor)
            {
                TMdbProc & tMdbProc = *itor; 
                if(tMdbProc.sName[0] == 0){continue;}
                if(TMdbNtcStrFunc::FindString(tMdbProc.sName,"mdbOraRep") != -1
                    || TMdbNtcStrFunc::FindString(tMdbProc.sName,"mdbFlushOra") != -1)
                {
                    tMdbProc.Clear();
                }
            }
            //��������oracle���ݽ���
            sprintf(sProcFullName,"mdbFlushOra %s",m_pDsn->sName);
            m_tProcCtrl.Restart(sProcFullName,true);
            for(int i=-1; i<m_pDsn->m_iOraRepCounts; ++i)
            {
                sprintf(sProcFullName, "mdbOraRep %s %d %d",m_pDsn->sName,m_pDsn->m_iOraRepCounts, i);
                iRet = m_tProcCtrl.Restart(sProcFullName,true);
                if(0 != iRet){break;}
            }
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* ��������	:  StartRepProcess
* ��������	:  ��������ͬ������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::StartRepProcess()
{
    TADD_FUNC("Start..");
    int iRet = 0;
    char sProcFullName[MAX_NAME_LEN] = {0};
    //�����Ҫ�뱸��ͬ������������ؽ���
    if(m_pConfig->GetIsStartShardBackupRep())
    {
        //��������ˢ�µ���־�Ľ���
        sprintf(sProcFullName, "mdbFlushRep %s",m_pDsn->sName);
        m_tProcCtrl.Restart(sProcFullName,true);
        sprintf(sProcFullName, "mdbRepServer %s",m_pDsn->sName);
        m_tProcCtrl.Restart(sProcFullName,true);
        sprintf(sProcFullName, "mdbRepClient %s",m_pDsn->sName);
        m_tProcCtrl.Restart(sProcFullName,true);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  StartCaptureProcess
* ��������	:  ����Capture����ˢ������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::StartCaptureProcess()
{
    TADD_FUNC("Start..");
    int iRet = 0;
    char sProcFullName[MAX_NAME_LEN] = {0};
    if(m_pDsn->m_bIsCaptureRouter)
    {
        //��������ˢ�µ���־�Ľ���
        sprintf(sProcFullName, "mdbFlushCapture %s",m_pDsn->sName);
        m_tProcCtrl.Restart(sProcFullName,true);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteCreateJob
* ��������	:  ����job
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteCreateJob()
{
    TADD_FUNC("Start..");
    int iRet = 0;
    CHECK_OBJ(m_pMdbJob);
    TMdbJobCtrl tJobCtrl;
    CHECK_RET(tJobCtrl.Init(m_pDsn->sName),"Failed to Initialize tJobCtrl.");
    CHECK_RET(tJobCtrl.AddNewJob(m_pMdbJob),"Failed to add the new job[%s].",m_pMdbJob->m_sName);
    //�޸�QuickMDB_JOB.xml�ļ�
    CHECK_RET(m_pScript->CreateJob(m_pMdbJob),"Failed to modify  QuickMDB_JOB.xml file.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteAlterJob
* ��������	:  �޸�job����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteAlterJob()
{
    TADD_FUNC("Start..");
    int iRet = 0;
    // TODO:
    //��ֹ�޸ĵ�job���ڱ�ִ�У���ɾ��job��Ȼ����������job
    TMdbJob tTmpJob;
    TMdbJobCtrl tJobCtrl;
    TMdbJob *pMemJob = m_pShmDSN->GetJobByName(m_pMdbJob->m_sName);
    if(NULL == pMemJob)
    {
        CHECK_RET(ERR_APP_INVALID_PARAM,"Not find job[%s]",m_pMdbJob->m_sName);
    }
    tTmpJob.Clear();
    memcpy(&tTmpJob,m_pMdbJob,sizeof(TMdbJob));
    memcpy(m_pMdbJob,pMemJob,sizeof(TMdbJob));
    //����jobֵ
    if(tTmpJob.m_sExecuteDate[0] != 0)
    {
        SAFESTRCPY(m_pMdbJob->m_sExecuteDate,sizeof(m_pMdbJob->m_sExecuteDate),tTmpJob.m_sExecuteDate);
    }
    if(tTmpJob.m_iInterval > 0)
    {
        m_pMdbJob->m_iInterval = tTmpJob.m_iInterval;
    }
    if(tTmpJob.m_iRateType > 0)
    {
        m_pMdbJob->m_iRateType = tTmpJob.m_iRateType;
    }
    if(tTmpJob.m_sSQL[0] != 0)
    {
        SAFESTRCPY(m_pMdbJob->m_sSQL,sizeof(m_pMdbJob->m_sSQL),tTmpJob.m_sSQL);
    }
    //��ȡ�����JOB
    CHECK_RET(tJobCtrl.Init(m_pDsn->sName),"Failed to Initialize tJobCtrl.");
    CHECK_RET(tJobCtrl.DeleteJobByName(m_pMdbJob->m_sName),"Failed to remove the job[%s].",m_pMdbJob->m_sName);
    CHECK_RET(tJobCtrl.AddNewJob(m_pMdbJob),"Failed to add the new job[%s].",m_pMdbJob->m_sName);
    //�޸�QuickMDB_JOB.xml�ļ�
    CHECK_RET(m_pScript->AlterJob(m_pMdbJob),"Failed to modify  QuickMDB_JOB.xml file.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteRemoveJob
* ��������	:  ɾ��job
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ExecuteRemoveJob()
{
    TADD_FUNC("Start..");
    int iRet = 0;
    // TODO:
    CHECK_OBJ(m_pMdbJob);
    TMdbJobCtrl tJobCtrl;
    CHECK_RET(tJobCtrl.Init(m_pDsn->sName),"Failed to Initialize tJobCtrl.");
    CHECK_RET(tJobCtrl.DeleteJobByName(m_pMdbJob->m_sName),"Failed to remove the job[%s].",m_pMdbJob->m_sName);
    //�޸�QuickMDB_JOB.xml�ļ�
    CHECK_RET(m_pScript->RemoveJob(m_pMdbJob),"Failed to modify  QuickMDB_JOB.xml file.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  AddFlushSQLOrLoadSQLParam
* ��������	:  ����flush-sql or load-sql�а󶨲�����Ӧ�Ĳ����ڵ���Ϣ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::AddFlushSQLOrLoadSQLParam()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(m_pTable);
    TMdbTable * pTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);
    CHECK_OBJ(pTable);
     //���֧��10������
    if(pTable->iParameterCount >= MAX_INDEX_COUNTS)
    {
        m_pMdbSqlParser->m_pDDLSqlStruct->pTable->iParameterCount--;
        CHECK_RET(ERR_SQL_INVALID,\
            "The number of SQL(flush-sql and load-sql) binding parameters have reached the maximum[10].");
    }
    TMdbParameter &tParam = m_pTable->tParameter[m_pTable->iParameterCount-1];
    //�ж������Ĳ����Ƿ��Ѿ�����
    for(int iPos = 0;iPos < MAX_INDEX_COUNTS;iPos++)
    {
        if(pTable->tParameter[iPos].sName[0] == 0)
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tParameter[iPos].sName,tParam.sName) == 0
            && pTable->tParameter[iPos].iParameterType == tParam.iParameterType)
        {
            m_pMdbSqlParser->m_pDDLSqlStruct->pTable->iParameterCount--;
            CHECK_RET(ERR_SQL_INVALID,"Parameter [%s] already exists.",tParam.sName);
        }
    }
    //���ɶ�Ӧ���ļ�
    CHECK_RET(m_pScript->AddFlushOrLoadSQLParam(m_pTable,true),"AddFlushOrLoadSQLParam failed.");
    //�Ե�ǰ���ʱ����
    CHECK_RET(pTable->tTableMutex.Lock(true,&m_pDsn->tCurTime),"lock failed.");
    memcpy(&(pTable->tParameter[pTable->iParameterCount]),&(tParam),sizeof(TMdbParameter));
    pTable->iParameterCount++;
    CHECK_RET(pTable->tTableMutex.UnLock(true),"unlock failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  DropFlushSQLOrLoadSQLParam
* ��������	:  ɾ���������е�parameter�ڵ�
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::DropFlushSQLOrLoadSQLParam()
{
    TADD_FUNC("Start..");
    int iRet = 0;
    int iParamPos = -1;
    bool bIsExist = false;
    TMdbTable * pTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);
    CHECK_OBJ(pTable);
    TMdbParameter &tParam = m_pMdbSqlParser->m_pDDLSqlStruct->tParam;
    //�ж�ɾ���Ĳ����Ƿ��Ѿ�����
    for(iParamPos = 0;iParamPos < MAX_INDEX_COUNTS;iParamPos++)
    {
        if(pTable->tParameter[iParamPos].sName[0] == 0)
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tParameter[iParamPos].sName,tParam.sName) == 0)
        {
            bIsExist = true;
            break;
        }
    }
    if(!bIsExist)
    {
        CHECK_RET(ERR_SQL_INVALID,"Parameter[%s] does not exist.",tParam.sName);
    }
    //���ɶ�Ӧ���ļ�
    CHECK_RET(m_pScript->DropFlushOrLoadSQLParam(m_pTable,tParam.sName,true),"DropFlushOrLoadSQLParam failed.");
    //�Ե�ǰ���ʱ����
    CHECK_RET(pTable->tTableMutex.Lock(true,&m_pDsn->tCurTime),"lock failed.");
    for(;iParamPos < pTable->iParameterCount-1;iParamPos++)
    {
        memcpy(&(pTable->tParameter[iParamPos]), \
               &(pTable->tParameter[iParamPos+1]),sizeof(TMdbParameter)); 
        pTable->tParameter[iParamPos+1].Clear();
    }
    pTable->iParameterCount --;
    CHECK_RET(pTable->tTableMutex.UnLock(true),"unlock failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ModifyFlushSQLOrLoadSQLParam
* ��������	:  �޸�flush-sql or load-sql�а󶨲���������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::ModifyFlushSQLOrLoadSQLParam()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iParamPos = -1;
    bool bIsExist = false;
    TMdbTable * pTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);
    CHECK_OBJ(pTable);
    TMdbParameter &tParam = m_pMdbSqlParser->m_pDDLSqlStruct->tParam;
    //�ж��޸ĵĲ����Ƿ��Ѿ�����
    for(iParamPos = 0;iParamPos < MAX_INDEX_COUNTS;iParamPos++)
    {
        if(pTable->tParameter[iParamPos].sName[0] == 0)
        {
            continue;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tParameter[iParamPos].sName,tParam.sName) == 0)
        {
            bIsExist = true;
            break;
        }
    }
    if(!bIsExist)
    {
        CHECK_RET(ERR_SQL_INVALID,"Modification of the parameter[%s] does not exist.",tParam.sName);
    }
    //���ɶ�Ӧ���ļ�
    CHECK_RET(m_pScript->ModifyFlushOrLoadSQLParam(true),"ModifyFlushOrLoadSQLParam failed.");
    //�Ե�ǰ���ʱ����
    CHECK_RET(pTable->tTableMutex.Lock(true,&m_pDsn->tCurTime),"lock failed.");
    if(tParam.iDataType > 0)
    {
        pTable->tParameter[iParamPos].iDataType = tParam.iDataType;
    }
    if(tParam.iParameterType >=0)
    {
        pTable->tParameter[iParamPos].iParameterType = tParam.iParameterType;
    }
    if(tParam.sValue[0] != 0)
    {
        SAFESTRCPY(pTable->tParameter[iParamPos].sValue,\
            sizeof(pTable->tParameter[iParamPos].sValue), tParam.sValue);
    }
    CHECK_RET(pTable->tTableMutex.UnLock(true),"unlock failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  SetCurOperateUser
* ��������	:  ���õ�ǰ�������û���Ϣ
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbDDLExecuteEngine::SetCurOperateUser(const char* pUserName)
{
    int iRet = 0;
    //��鵱ǰ�������û��Ƿ����
    CHECK_OBJ(pUserName);
    TMDbUser* pUser = m_pConfig->GetUser(pUserName);
    if(pUser == NULL)
    {
        CHECK_RET(ERR_SQL_INVALID,"user[%s] does not exist.",pUserName);
    }
    //���浱ǰ�������û���Ϣ
    m_pCurOperUser = pUser;
    return iRet;
}

/******************************************************************************
* ��������	:  ModifColumnRepType
* ��������	:  �޸��е�ͬ����������ͬ������һ��
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
/*int TMdbDDLExecuteEngine::ModifColumnRepType(TMdbTable * pTable)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pTable);
    for(int i=0;i<pTable->iColumnCounts;i++)
    {
        pTable->tColumn[i].iRepAttr = pTable->iRepAttr;
    }
    TADD_FUNC("Finish.");
    return iRet;
}*/

//����drop�������Ҫ��ʱɾ��ϵͳ���ж�Ӧ�ļ�¼
int TMdbDDLExecuteEngine::DeleteDBARecord(const char* psTableName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(psTableName);
    char sSQL[MAX_SQL_LEN] = {0};
    char sTabName[MAX_NAME_LEN] ={0};
    SAFESTRCPY(sTabName, sizeof(sTabName), psTableName);
    TMdbNtcStrFunc::ToUpper(sTabName);
    snprintf(sSQL,sizeof(sSQL),"delete from dba_tables where table_name='%s'",sTabName);
    //ɾ��dba_tables��ָ�����¼
    if(!m_tMDB.IsConnect())
    {
        CHECK_RET(ConnectMDB(),"connect QMDB Faild.");
    }
    TMdbQuery *pQuery = m_tMDB.CreateDBQuery();
    CHECK_OBJ(pQuery);
    try
    {
        pQuery->SetSQL(sSQL);
        pQuery->Execute();
        pQuery->Commit();
        //ɾ��dba_column��ָ�����¼
        memset(sSQL,0,sizeof(sSQL));
        snprintf(sSQL,sizeof(sSQL),"delete from dba_column where table_name='%s'",sTabName);
        pQuery->SetSQL(sSQL);
        pQuery->Execute();
        pQuery->Commit();
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERR_DB_NOT_CONNECTED,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());
        iRet = ERR_DB_NOT_CONNECTED;
    }
    catch(...)
    {
        TADD_ERROR(ERR_DB_NOT_CONNECTED,"Unknown error!\n");
        iRet =  ERR_DB_NOT_CONNECTED;
    }
    SAFE_DELETE(pQuery);
    TADD_FUNC("Finish.");
    return iRet;
}

//rename table
int TMdbDDLExecuteEngine::ExecuteRenameTable(const char *pNewTableName,const bool bIsGenXML)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pNewTableName);
    if(0 == m_pTable->sTableName[0])
    {
        TADD_ERROR(ERR_TAB_TABLE_NAME_INVALID,"invalid tablename");
        return ERR_TAB_TABLE_NAME_INVALID;
    }
    if(IsSysTable(pNewTableName) || IsSysTable(m_pTable->sTableName))
    {
        CHECK_RET(-1,"system table rename operation error,[%s] to [%s]",m_pTable->sTableName,pNewTableName);
    }
    
    TMdbTable * pTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);
    CHECK_OBJ(pTable);
    char sOldTableName[MAX_NAME_LEN]={0}; 
    SAFESTRCPY(sOldTableName, sizeof(sOldTableName), pTable->sTableName);
    //������Ϣ�޸�
    TMdbIndexCtrl tIndexCtrl;
    tIndexCtrl.AttachDsn(m_pShmDSN);
    tIndexCtrl.RenameTableIndex(m_pShmDSN,pTable,pNewTableName);
    //����Ϣ�޸�
    SAFESTRCPY(pTable->sTableName, sizeof(pTable->sTableName), pNewTableName);
    //ɾ��dba�о���Ϣ
    CHECK_RET(DeleteDBARecord(sOldTableName),"DeleteDBARecord failed.");
    TMdbTable *pTableConfig = m_pConfig->GetTableByName(pTable->sTableName);
    if(NULL != pTableConfig)
    {
        SAFESTRCPY(pTableConfig->sTableName, sizeof(pTableConfig->sTableName), pNewTableName);
    }
    iRet = ChangePageTableInfo(pTable,pNewTableName);    
    if(iRet != 0)
    {
        tIndexCtrl.RenameTableIndex(m_pShmDSN,pTable,sOldTableName);
        SAFESTRCPY(pTable->sTableName, sizeof(pTable->sTableName), sOldTableName);
        //ɾ��dba�о���Ϣ
        CHECK_RET(DeleteDBARecord(pNewTableName),"DeleteDBARecord failed.");
        SAFESTRCPY(pTableConfig->sTableName, sizeof(pTableConfig->sTableName), sOldTableName);
        return iRet;
    }
    //���ɶ�Ӧ��xml�ļ�
    if(bIsGenXML)
    {
        iRet = m_pScript->RenameTable(sOldTableName,pNewTableName,true);
        if(iRet != 0)
        {//����
            tIndexCtrl.RenameTableIndex(m_pShmDSN,pTable,sOldTableName);
            SAFESTRCPY(pTable->sTableName, sizeof(pTable->sTableName), sOldTableName);
            //ɾ��dba�о���Ϣ
            CHECK_RET(DeleteDBARecord(pNewTableName),"DeleteDBARecord failed.");
            SAFESTRCPY(pTableConfig->sTableName, sizeof(pTableConfig->sTableName), sOldTableName);
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}

//rename ����Ҫ�޸�ҳ�еı���Ϣ
int TMdbDDLExecuteEngine::ChangePageTableInfo(TMdbTable * pTable,const char *sNewTableName)
{
	int iRet = 0;
	TMdbPage* pPage = NULL;
    CHECK_RET(m_mdbTSCtrl.Init(m_pDsn->sName,m_pTable->m_sTableSpace),"m_mdbTSCtrl.Init failed");
    //ȫҳ
	int iPageID = pTable->iFullPageID;
	while(iPageID > 0)
	{
		pPage = (TMdbPage*)m_mdbTSCtrl.GetAddrByPageID(iPageID);
        CHECK_OBJ(pPage);
        SAFESTRCPY(pPage->m_sTableName, sizeof(pPage->m_sTableName), sNewTableName);
        m_mdbTSCtrl.SetPageDirtyFlag(iPageID);//������ҳ        
		iPageID = pPage->m_iNextPageID;
	}
	
	//����ҳ
	iPageID = pTable->iFreePageID;
	int iEndPageID = iPageID;
	while(iPageID > 0)
	{
        pPage = (TMdbPage*)m_mdbTSCtrl.GetAddrByPageID(iPageID);
        CHECK_OBJ(pPage);
        SAFESTRCPY(pPage->m_sTableName, sizeof(pPage->m_sTableName), sNewTableName);
        m_mdbTSCtrl.SetPageDirtyFlag(iPageID);//������ҳ        
        iPageID = pPage->m_iNextPageID;
        if(iPageID == iEndPageID) break;
	}

    //ֻ���ļ��洢������checkpoint
    if(m_mdbTSCtrl.IsFileStorage())
    {
        TMdbCheckPoint tTMdbCheckPoint;
        CHECK_RET(tTMdbCheckPoint.Init(m_pShmDSN->GetInfo()->sName),"Init failed.");
       // CHECK_RET(tTMdbCheckPoint.LinkFile(m_pTable->m_sTableSpace),"Attach failed.");
       // CHECK_RET(tTMdbCheckPoint.DoCheckPoint(m_pTable->m_sTableSpace),"FlushOneTable falied");
       CHECK_RET(tTMdbCheckPoint.LinkFile(),"Attach failed.");
       CHECK_RET(tTMdbCheckPoint.DoCheckPoint(),"FlushOneTable falied");
    }
    return iRet;
}
//}
