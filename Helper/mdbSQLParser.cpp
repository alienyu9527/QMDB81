/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbSQLParser.cpp		
*@Description�� Sql����������Ľӿ���
*@Author:	    jin.shaohua
*@Date��	    2012.05
*@History:
******************************************************************************************/
#include "Helper/mdbSQLParser.h"
#include "Helper/Tokenize.h"
#include "Helper/mdbMalloc.h"
#include <iostream>
#include <queue>
#include <math.h>
#include "mdbFunc.h"
#include <stdarg.h>
#include "Helper/SqlParserHelper.h"
#include "parser.c"
#include "Helper/mdbErr.h"
#include "Helper/SyntaxTreeAnalyse.h"
#include "Helper/mdbEncrypt.h"


#include "Helper/mdbBase.h"
//#include "BillingSDK.h"

//DDL�ؼ��֣��¼ӹؼ�����Ҫ��������롣
char gKeywordsDDL[][MAX_DDL_CMD_LEN] =
{
    DDL_KEYWORD_CREATE,
    DDL_KEYWORD_ALTER,
    DDL_KEYWORD_DROP,
    DDL_KEYWORD_USE,
    DDL_KEYWORD_CONNECT,
    DDL_KEYWORD_LOAD,
    DDL_KEYWORD_ADD,
    DDL_KEYWORD_REMOVE,
    DDL_KEYWORD_TRUNCATE,
    DDL_KEYWORD_RENAME
};

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{


#define CHECK_RET_FILL(_ret,_code,...)if((iRet = _ret)!=0){TADD_ERROR(ERR_DB_NOT_CONNECTED,__VA_ARGS__);m_tError.FillErrMsg(_code,__VA_ARGS__);return _code;}
#define CHECK_OBJ_FILL(_obj)  if(NULL == _obj){TADD_ERROR(ERR_APP_INVALID_PARAM, #_obj" is null");m_tError.FillErrMsg(ERR_APP_INVALID_PARAM,#_obj" is null");return ERR_APP_INVALID_PARAM;}
#define CHECK_RET_FILL_BREAK(_ret,_code,...)if((iRet = _ret)!=0){TADD_ERROR(_ret,__VA_ARGS__);m_tError.FillErrMsg(_code,__VA_ARGS__);iRet = _code;break;}
#define CHECK_OBJ_FILL_BREAK(_obj) if(NULL == _obj){TADD_ERROR(ERR_OS_NO_MEMROY, #_obj" is null");m_tError.FillErrMsg(ERR_OS_NO_MEMROY,#_obj" is null");break;}

#define CONVERT_Y_N(_src,_dest) \
    if(TMdbNtcStrFunc::StrNoCaseCmp((_src),"Y") == 0)\
    {(_dest) = true;}\
else {(_dest) = false;}\


    /******************************************************************************
    * ��������	:  TMdbSqlParser
    * ��������	:  ����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbSqlParser::TMdbSqlParser():m_pMdbConfig(NULL), m_bGetValueChecked(false)
{
	m_pDDLSqlStruct = NULL;
	m_bIsDDLSQL     = false;
	m_iSetTimeStamp = 0;
	m_pHintIndex = NULL;
}

/******************************************************************************
* ��������	:  ~TMdbSqlParser
* ��������	:  ��������һЩ������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  jin.shaohua
*******************************************************************************/
TMdbSqlParser::~TMdbSqlParser()
{
    CleanResult();//�����ڴ�
	SAFE_DELETE(m_pDDLSqlStruct);
}

/******************************************************************************
* ��������	:  Init
* ��������	:  SQL��������ʼ������������ע�ᣬ������Դ��
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0-��ȷ
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::Init()
{
    CleanResult();//������
    return 0;
}



/******************************************************************************
* ��������	:  SetDB
* ��������	:  ����MDB��������Ϣ
* ����		:  pMdbDsn - DSN��Ϣ
* ����		:  pConfig     - mdb������Ϣ
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::SetDB(TMdbShmDSN * pMdbDsn,TMdbConfig * pConfig)
{
    int iRet = 0;
    CHECK_OBJ(pMdbDsn);
    CHECK_OBJ(pConfig);
    m_pMdbConfig = pConfig;//����Ҫ����
    CHECK_RET_FILL(m_tSqlParserHelper.SetDB(pMdbDsn),ERR_DB_NOT_CONNECTED," SetDB failed..");
    //m_tMdbExpr.SetNullFlag(m_pMdbConfig->GetDSN()->m_bNull);//����null�������
    m_tMdbExpr.SetNullFlag(true);//����null�������,ʹ���¹��򡣲��������߼�

    return iRet;
}

/******************************************************************************
* ��������	:  SetMDBConfig
* ��������	:  ����MDB����
* ����		:  pConfig     - mdb������Ϣ
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::SetMDBConfig(TMdbConfig * pConfig)
{
    int iRet = 0;
    CHECK_OBJ(pConfig);
    m_pMdbConfig = pConfig;
    return iRet;
}


/******************************************************************************
* ��������	:  ParseSQL
* ��������	:  ����SQL���
* ����		:  sSql-��������sql���
* ���		:  ��
* ����ֵ	:  0-��ȷ
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ParseSQL(const char * sSql)
{
    CHECK_OBJ(sSql);
    if(0 == sSql[0] || ';' ==  sSql[0] )
    {
        m_tError.FillErrMsg(ERR_SQL_INVALID,"sql[%s] is empty",sSql);
        return ERR_SQL_INVALID;
    }
    //DDL ���
    int iRet = 0;
    CHECK_RET_FILL(CheckDDLSQL(sSql),ERR_SQL_INVALID,"CheckDDLSQL failed");
    TADD_FUNC("Start.SQL=[%s].",sSql);
    int tokenType;
    Token sLastToken;
    memset(&sLastToken,0x00,sizeof(Token));
    void * pEngine = NULL;//�﷨������
    do
    {
        Init();//��ʼ����һЩ������
        pEngine = mdbParserAlloc((void*(*)(size_t))malloc);
        if (NULL == pEngine )
        {//error
            m_tError.FillErrMsg(ERR_OS_NO_MEMROY,"Engine  is null,sql=[%s].",sSql);
            break;
        }
        unsigned int i = 0;
        while( sSql[i]!=0 && 0 == m_tError.GetErrCode())
        {
            sLastToken.z = &sSql[i];
            sLastToken.n = static_cast<unsigned int>(GetToken((unsigned char*)&sSql[i],&tokenType));
            i += sLastToken.n;
            switch( tokenType )
            {
            case TK_SPACE: 
                {
                    break;
                }
            case TK_ILLEGAL: 
                {
                    m_tError.FillErrMsg(ERR_SQL_INVALID,"unrecognised token[%s]",sLastToken.z);
                    break;
                }
            default: 
                {
                    TADD_DETAIL("[str = %s][len = %d][type = %d]\n",sLastToken.z,sLastToken.n,tokenType);
                    mdbParser(pEngine, tokenType, sLastToken, this);
                    break;
                }
            }
        }
        if (0 != m_tError.GetErrCode())
        {//�д���
            m_tError.FillErrMsg(ERR_SQL_INVALID,"sql=[%s] Error near [%s],ErrCode=[%d],ErrMsg[%s]",
                sSql,sLastToken.z,m_tError.GetErrCode(),m_tError.GetErrMsg());
            break;
        }
        if (TK_SEMI != tokenType)
        {//δ����
            m_tError.FillErrMsg(ERR_SQL_INVALID,"sql=[%s] is not end",sSql);
            break;
        }
    }while(0);
    if(NULL != pEngine)
    {
        mdbParserFree(pEngine,free);
        pEngine = NULL;
    }
    TADD_FUNC("Finish.Error Code=[%d].",m_tError.GetErrCode());
    if(strlen(sSql) >= MAX_SQL_LEN)
    {
        strncpy(m_stSqlStruct.sSQL,sSql,MAX_SQL_LEN-1);
        m_stSqlStruct.sSQL[MAX_SQL_LEN-1]='\0';
    }
    else
    {
        SAFESTRCPY(m_stSqlStruct.sSQL,sizeof(m_stSqlStruct.sSQL),sSql);//����SQL���
    }
    return m_tError.GetErrCode();
}

//���DDL SQL
int TMdbSqlParser::CheckDDLSQL(const char * sSql)
{
    int iRet = 0;
    m_bIsDDLSQL = false;
    char sCmd[MAX_DDL_CMD_LEN+2] = {0};//�����Ҫ���MAX_DDL_CMD_LEN���ַ�
    char *p = (char*)memccpy(sCmd, sSql, ' ', sizeof(sCmd)-1);
    if(NULL == p) return iRet;//û�пո�
    *(p-1) = '\0';//ȥ���ո�
    for(int i = 0; i<(int)sizeof(gKeywordsDDL)/MAX_DDL_CMD_LEN; ++i)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(gKeywordsDDL[i], sCmd) == 0)
        {
            m_bIsDDLSQL = true;
            break;
        }
    }

    if(m_bIsDDLSQL)
    {
        CHECK_RET(NewDDLStruct(),"NewDDLStruct failed");
    }
    return iRet;
}
//���ڲ�����SQL����ֱ�ӵ���DDL �������Ľӿڣ���Ҫ�ֹ�����ռ�
int TMdbSqlParser::NewDDLStruct()
{
    if(!m_pDDLSqlStruct)
    {
        m_pDDLSqlStruct = new(std::nothrow) _ST_DDL_SQL_STRUCT();
        CHECK_OBJ(m_pDDLSqlStruct);
    }
    return 0;
}
/******************************************************************************
* ��������	:  FillTableName
* ��������	:  ��ȡsql����еı����Լ�����
* ����		:  pToken - ������token
* ����		:  pTokenAlias -  ��ı�����token
* ���		:  ��
* ����ֵ	:  0 - ��ȷ
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillTableName(const Token * pToken,const Token * pTokenAlias)
{
    CHECK_OBJ(pToken);
    TADD_FUNC("Start.Token=[%s].",pToken->z);
    int iRet = 0;
    m_stSqlStruct.sTableName= QMDB_MALLOC->NameFromToken(pToken);
    m_stSqlStruct.sAlias    = QMDB_MALLOC->NameFromToken(pTokenAlias);
    TADD_DETAIL("Table[%s],alias[%s]",m_stSqlStruct.sTableName,m_stSqlStruct.sAlias== NULL?"nil":m_stSqlStruct.sAlias);

    m_stSqlStruct.pMdbTable = m_tSqlParserHelper.GetMdbTable(m_stSqlStruct.sTableName);
    if(NULL == m_stSqlStruct.pMdbTable)
    {
        CHECK_RET_FILL(ERR_TAB_NO_TABLE,ERR_TAB_NO_TABLE,"not find table [%s]",m_stSqlStruct.sTableName);
    }
    //m_tSqlParserHelper.BuildColumIndexMap(m_stSqlStruct.pMdbTable);//����column -  indexӳ��
    m_stSqlStruct.pShmDSN = m_tSqlParserHelper.GetMdbShmDSN();//����shmdsn
    TADD_FUNC("Finish");
    return iRet;
}

/******************************************************************************
* ��������	:  CleanResult
* ��������	:  �����ϴε�SQL�������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbSqlParser::CleanResult()
{
    TADD_FUNC("Start.");
    QMDB_MALLOC->ReleaseExprList(m_stSqlStruct.pColList);
    QMDB_MALLOC->ReleaseExprList(m_stSqlStruct.pOrderby);
    QMDB_MALLOC->ReleaseExprList(m_stSqlStruct.pGroupby);
    QMDB_MALLOC->ReleaseExpr(m_stSqlStruct.pHaving);
    QMDB_MALLOC->ReleaseExpr(m_stSqlStruct.pWhere);
    QMDB_MALLOC->ReleaseExpr(m_stSqlStruct.pstLimit);
    QMDB_MALLOC->ReleaseExpr(m_stSqlStruct.pstOffset);
    QMDB_MALLOC->ReleaseIdList(m_stSqlStruct.pstIdList);

    QMDB_MALLOC->ReleaseStr(m_stSqlStruct.sTableName);
    QMDB_MALLOC->ReleaseStr(m_stSqlStruct.sAlias);
    QMDB_MALLOC->ReleaseExpr(m_stSqlStruct.pstRouterID);//
    m_stSqlStruct.clear();
	if(m_pDDLSqlStruct)
	{
        SAFE_DELETE(m_pDDLSqlStruct->pDsn);
    	SAFE_DELETE(m_pDDLSqlStruct->pTable);
        SAFE_DELETE(m_pDDLSqlStruct->pUser);
        SAFE_DELETE(m_pDDLSqlStruct->pTablespace);
        SAFE_DELETE(m_pDDLSqlStruct->pProAttr);
        SAFE_DELETE(m_pDDLSqlStruct->pMdbJob);
    	m_pDDLSqlStruct->clear();
	}
    //�������ֵ	
    m_listInputCollist.clear();
    m_listInputVariable.clear();
    m_listInputWhere.clear();
    m_listOutputCollist.clear();
    m_listInputOrderby.clear();
    m_listOutputOrderby.clear();

    m_listInputLimit.clear();
    m_listOutputLimit.clear();

    m_listInputPriKey.clear();
    m_listOutputPriKey.clear();

    m_listInputAggValue.clear();
    m_listOutputGroupBy.clear();
    m_listOutputHaving.clear();
    m_listInputGroupBy.clear();
    m_listInputHaving.clear();
    //��������
    CleanOtherMem();
    //m_iSourceId = SOURCE_APP;
    m_tError.FillErrMsg(0,"");//���������Ϣ
    m_bGetValueChecked = false;
    m_vAggExpr.clear();
    m_iSetTimeStamp = -1;
	m_pHintIndex = NULL;
    TADD_FUNC("Finish.");
}
/******************************************************************************
* ��������	:  ClearLastExecute
* ��������	:  ������һ�ε�ִ��
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ClearLastExecute()
{
    int iRet = 0;
    unsigned int i = 0;
    for(i = 0; i < m_stSqlStruct.vExprCalcPerExec.size();++i)
    {
        ST_EXPR * pstExpr = m_stSqlStruct.vExprCalcPerExec[i];
        ExprCalcClearProperty(pstExpr,CALC_NO_NEED);
    }
    /*
    m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pColList,CALC_PER_Exec);
    m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pOrderby,CALC_PER_Exec);
    m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pstLimit,CALC_PER_Exec);
    m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pstOffset,CALC_PER_Exec);
    m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pstRouterID,CALC_PER_Exec);
    m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pWhere,CALC_PER_Exec);   
    */
    m_stSqlStruct.iRowPos = 0; //����row pos
    return iRet;
}


/******************************************************************************
* ��������	:  IdListAppend
* ��������	:  ��idlist (ST_ID_LIST),���Ԫ��
* ����		:  pList - ����ӵ�idlist��pToken-��ӵ�Ԫ��
* ���		:  ��
* ����ֵ	:  �ɹ�- ������Ӻ��idlist,  ʧ��- 0
* ����		:  jin.shaohua
*******************************************************************************/
ST_ID_LIST * TMdbSqlParser::IdListAppend(ST_ID_LIST *pList, Token *pToken){
    int i;
    if( pList==0 ){
        pList = QMDB_MALLOC->AllocIdList();
        if( pList==0 ) return 0;
        pList->iAllocNum = 0;
    }
    pList->pstItem = (ID_LIST_ITEM *)QMDB_MALLOC->ArrayAllocate(
        pList->pstItem,
        sizeof(pList->pstItem[0]),
        5,
        &pList->iIdNum,
        &pList->iAllocNum,
        &i
        );
    if( i<0 ){
        QMDB_MALLOC->ReleaseIdList(pList);
        return 0;
    }
    pList->pstItem[i].zName = QMDB_MALLOC->NameFromToken(pToken);
    return pList;
}
/******************************************************************************
* ��������	:  BuildSelect
* ��������	:  ����select�����ṹ
* ����		:  pFirst - first �ؼ����顣isDistinct - �Ƿ����distinct
* ����		:  pColList - ��ѯ�����   pstWhere - where ������
* ����		:  pstOrderby - order by �飬 pLimit - limit �飬pOffset - pOffset��
pstGroupby - group by �Ӿ�pstHaving - having�Ӿ�

* ���		:  ��
* ����ֵ	:  ST_SQL_STRUCT * - �����ɹ���NULL- ����ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::BuildSelect(ST_EXPR_LIST * pHint,ST_EXPR * pFirst,int isDistinct,ST_EXPR_LIST * pColList,
                               ST_EXPR * pstWhere,ST_EXPR_LIST * pstGroupby,ST_EXPR * pstHaving,
                               ST_EXPR_LIST * pstOrderby,ST_EXPR * pLimit,
                               ST_EXPR * pOffset)
{
    TADD_FUNC("Start.pColList=[%p],pWhere=[%p],pOrderby=[%p],pFirst=[%p],pLimit=[%p],pOffset=[%p]",
        pColList,pstWhere,pstOrderby,pFirst,pLimit,pOffset);
    int iRet = 0;	
	
    m_stSqlStruct.iSqlType  = TK_SELECT;
    m_stSqlStruct.pColList  = pColList;
    m_stSqlStruct.pWhere    = pstWhere;
    m_stSqlStruct.pOrderby  = pstOrderby;
    m_stSqlStruct.pGroupby  = pstGroupby;
    m_stSqlStruct.pHaving   = pstHaving;
	m_stSqlStruct.pHintList = pHint;
    if(NULL == m_stSqlStruct.pGroupby && NULL != m_stSqlStruct.pHaving)
    {//having�Ӿ��������group by��
        CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"[having]SQL should be used with[group by]");
    }
    CHECK_RET_FILL(FillFirstAndLimit( pFirst, pLimit,pOffset),ERR_SQL_INVALID,"FillFirstAndLimit failed.");//       
    if(NULL == m_stSqlStruct.pMdbTable)
    {
        CHECK_RET_FILL(ERR_SQL_INVALID,ERR_TAB_NO_TABLE,"table does not exist.");
    }
    CHECK_RET_FILL(RemoveTableAlias(),ERR_SQL_INVALID,"RemoveTableAlias failed");//ȥ�������    
	CHECK_RET_FILL(FillHintInfo(),ERR_SQL_INVALID,"FillHintInfo Failed.");
    //CHECK_RET_FILL(FillSequenceValue(),ERR_SQL_INVALID,"FillSequenceValue failed");	
    CHECK_RET_FILL(SpanTKALLCollumnList(m_stSqlStruct.pColList),ERR_TAB_COLUMN_NOT_EXIST,"span * error.");//����*
    CHECK_RET_FILL(FillMdbInfo(),ERR_SQL_FILL_MDB_INFO,"FillMdbInfo error.");//���MDB�����Ϣ
    CHECK_RET_FILL(CollectWhereIndex(),ERR_TAB_INDEX_NOT_EXIST,"CollectWhereIndex failed");//�Ѽ�where�е�index
    CHECK_RET_FILL(m_tSqlTypeCheck.FillValidType(&m_stSqlStruct),ERR_SQL_INVALID,"FillValidType failed..");
    CHECK_RET_FILL(FillOrderbyInfo(m_stSqlStruct.pOrderby),ERR_SQL_INVALID,"FillOrderbyInfo error...");
    CHECK_RET_FILL(BuildGroupBy(),ERR_SQL_INVALID,"BuildGroupBy error...");//����group by
    CollectOutputValue();//�Ѽ��������
    CollectInputValue();//�Ѽ���Ҫ��������ݽڵ�
    if(m_listInputLimit.vMemValue.size() != 0)
    {//limit offset �в�����������Ϣ
        CHECK_RET_FILL(-1,ERR_SQL_INVALID,"column can not in limit offset first.");
    }
    VectorToList();
    ChangeValueToRefence(m_listInputWhere);
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* ��������	:  BuildInsert
* ��������	:  ����insert�����ṹ
* ����		:  pToken - ������token, pCollist - ���������Ϣ
* ����		:  pstExprList - �����ֵ��Ϣ
* ���		:  ��
* ����ֵ	:  ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::BuildInsert(ST_ID_LIST * pCollist,ST_EXPR_LIST * pstExprList)
{
    TADD_FUNC("Start.pCollist=[%p],pstExprList=[%p]",pCollist,pstExprList);
    int iRet = 0;
    m_stSqlStruct.iSqlType = TK_INSERT;
    m_stSqlStruct.pstIdList= pCollist;
    m_stSqlStruct.pColList =  pstExprList;
    CHECK_RET_FILL(RemoveTableAlias(),ERR_SQL_INVALID,"RemoveTableAlias failed");//ȥ�������
    //CHECK_RET_FILL(FillSequenceValue(),ERR_SQL_INVALID,"FillSequenceValue failed");       
    CHECK_RET_FILL(CollectRouterID(m_stSqlStruct.pstRouterID),ERR_SQL_FILL_MDB_INFO,"CollectRouterID faild.");//�Ѽ�·����Ϣ
    CHECK_RET_FILL(FillMdbInfo(),ERR_SQL_FILL_MDB_INFO,"FillMdbInfo error.");//���MDB�����Ϣ
    CHECK_RET_FILL(CollectWhereIndex(),ERR_TAB_INDEX_NOT_EXIST,"CollectWhereIndex failed");//�Ѽ�where�е�index
    CHECK_RET_FILL(SpanInsertCollumnList(m_stSqlStruct.pstIdList),ERR_SQL_FILL_MDB_INFO,
        "SpanInsertCollumnList error.");//��չ����ֵ���Ѽ���Ҫ��������ݽڵ㡣
    CHECK_RET_FILL(m_tSqlTypeCheck.FillValidType(&m_stSqlStruct),ERR_SQL_INVALID,"FillValidType failed..");
    CHECK_RET_FILL(CollectPrimaryKeyFromSQL(m_stSqlStruct.pstIdList),ERR_SQL_INVALID,"CollectPrimaryKey failed.");
    if(m_stSqlStruct.pMdbTable->bIsCheckPriKey && m_listOutputPriKey.vMemValue.size()>0)
    {//�ж��Ƿ���Ҫ����У��
        m_stSqlStruct.bCheckInsertPriKey = true;
        CHECK_RET_FILL(CollectIndexForVerifyPK(m_stSqlStruct.pstIdList),ERR_SQL_INVALID,"CollectIndexForVerifyPK failed.");
    }
    else
    {
        m_stSqlStruct.bCheckInsertPriKey = false;
    }
    CollectInputValue();//�Ѽ���Ҫ��������ݽڵ�
    VectorToList();
    ChangeValueToRefence(m_listInputWhere);
    TADD_FUNC("Finish.CheckPK=[%d]",m_stSqlStruct.bCheckInsertPriKey);
    return iRet;
}

/******************************************************************************
* ��������	:  BuildDelete
* ��������	:  ����delete �����ṹ
* ����		:  pToken - ������token��pstExpr - where ������
* ���		:  ��
* ����ֵ	:  ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::BuildDelete(ST_EXPR_LIST * pHint, ST_EXPR * pFirst,ST_EXPR * pstWhereExpr,ST_EXPR * pLimit,ST_EXPR * pOffset)
{
    TADD_FUNC("Start,pstExpr=[%p]",pstWhereExpr);
    int iRet = 0;
    m_stSqlStruct.iSqlType = TK_DELETE;
    m_stSqlStruct.pWhere = pstWhereExpr;
	m_stSqlStruct.pHintList = pHint;
	
    CHECK_RET_FILL(FillFirstAndLimit( pFirst, pLimit,pOffset),ERR_SQL_INVALID,"FillFirstAndLimit failed.");//
    CHECK_RET_FILL(RemoveTableAlias(),ERR_SQL_INVALID,"RemoveTableAlias failed");//ȥ�������
	CHECK_RET_FILL(FillHintInfo(),ERR_SQL_INVALID,"FillHintInfo Failed.");
	//CHECK_RET_FILL(FillSequenceValue(),ERR_SQL_INVALID,"FillSequenceValue failed");       
    CHECK_RET_FILL(CollectRouterID(m_stSqlStruct.pstRouterID),ERR_SQL_FILL_MDB_INFO,"CollectRouterID faild.");//�Ѽ�·����Ϣ
    CHECK_RET_FILL(FillMdbInfo(),ERR_SQL_FILL_MDB_INFO,"FillMdbInfo error.");//���MDB�����Ϣ
    CHECK_RET_FILL(CollectWhereIndex(),ERR_TAB_INDEX_NOT_EXIST,"CollectWhereIndex failed");//�Ѽ�where�е�index
    CHECK_RET_FILL(m_tSqlTypeCheck.FillValidType(&m_stSqlStruct),ERR_SQL_INVALID,"FillValidType failed..");
    CHECK_RET_FILL(CollectPrimaryKeyFromTable(),ERR_SQL_INVALID,"CollectPrimaryKeyFromTable failed.");
    CollectOutputValue();//���ֵ
    CollectInputValue();//�Ѽ���Ҫ��������ݽڵ�
    VectorToList();
    ChangeValueToRefence(m_listInputWhere);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  BuildUpdate
* ��������	:  ����update�����ṹ
* ����		:  pToken - ������token��pCollist - set ֵ�����飬pstExpr - where ������
* ���		:  ��
* ����ֵ	:  ��
* ����		:  jin.shaohua
*******************************************************************************/
int  TMdbSqlParser::BuildUpdate(ST_EXPR_LIST * pHint,ST_EXPR * pFirst,ST_SET_LIST stSetList,ST_EXPR * pstWhereExpr,ST_EXPR * pLimit,ST_EXPR * pOffset)
{
    TADD_FUNC("Start,stSetList=[%p],pstExpr=[%p]",stSetList,pstWhereExpr);
    int iRet = 0;
    m_stSqlStruct.iSqlType = TK_UPDATE;
    m_stSqlStruct.pColList  = stSetList.pExprList;
    m_stSqlStruct.pstIdList = stSetList.pIdList;
    m_stSqlStruct.pWhere = pstWhereExpr;
	m_stSqlStruct.pHintList = pHint;
	
    CHECK_RET_FILL(FillFirstAndLimit( pFirst, pLimit,pOffset),ERR_SQL_INVALID,"FillFirstAndLimit failed.");//
    CHECK_RET_FILL(RemoveTableAlias(),ERR_SQL_INVALID,"RemoveTableAlias failed");//ȥ�������
	CHECK_RET_FILL(FillHintInfo(),ERR_SQL_INVALID,"FillHintInfo Failed.");
	//CHECK_RET_FILL(FillSequenceValue(),ERR_SQL_INVALID,"FillSequenceValue failed");   
    CHECK_RET_FILL(CheckSetList(stSetList),ERR_SQL_INVALID,"CheckSetList failed.");
    CHECK_RET_FILL(CollectRouterID(m_stSqlStruct.pstRouterID),ERR_SQL_FILL_MDB_INFO,"CollectRouterID faild.");//�Ѽ�·����Ϣ
    CHECK_RET_FILL(FillMdbInfo(),ERR_SQL_FILL_MDB_INFO,"FillMdbInfo error.");//���MDB�����Ϣ
    CHECK_RET_FILL(CollectWhereIndex(),ERR_TAB_INDEX_NOT_EXIST,"CollectWhereIndex failed");//�Ѽ�where�е�index
    //CHECK_RET_FILL(SpanUpdateCollumnList(m_stSqlStruct.pstIdList),ERR_SQL_FILL_MDB_INFO,"SpanUpdateCollumnList failed.");
    CHECK_RET_FILL(CollectChangeIndex(m_stSqlStruct.pstIdList),ERR_SQL_FILL_MDB_INFO,"CollectChangeIndex error.");
    CHECK_RET_FILL(m_tSqlTypeCheck.FillValidType(&m_stSqlStruct),ERR_SQL_INVALID,"FillValidType failed..");
    CHECK_RET_FILL(CollectPrimaryKeyFromSQL(m_stSqlStruct.pstIdList),ERR_SQL_INVALID,"CollectPrimaryKey failed.");
    if(m_listOutputPriKey.vMemValue.size()!=0)
    {
        CHECK_RET_FILL(-1,ERR_SQL_INVALID,"can't update primary key");
    }
    CHECK_RET_FILL(CollectPrimaryKeyFromTable(),ERR_SQL_INVALID,"CollectPrimaryKeyFromTable failed.");
    CollectOutputValue();//�������ֵ
    CollectInputValue();//�Ѽ���Ҫ��������ݽڵ�
    VectorToList();
    ChangeValueToRefence(m_listInputWhere);
    TADD_FUNC("Finish");
    return iRet;
}


/******************************************************************************
* ��������	:  CollectOutputValue
* ��������	:  �Ѽ���Ҫ�����ֵ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbSqlParser::CollectOutputValue()
{
    TADD_FUNC("Start");
    int i = 0;
    //select list
    if(TK_SELECT == m_stSqlStruct.iSqlType)
    {
        ST_EXPR_LIST * pList = m_stSqlStruct.pColList;
        for (i = 0;i<pList->iItemNum;i++)
        {
            ST_EXPR_ITEM & item = pList->pExprItems[i];
            ST_MEM_VALUE * pMemValue = item.pExpr->pExprValue;
            if(item.sName != NULL)
            {
                pMemValue->sAlias = item.sName;
            }
            else if(item.sSpan != NULL)
            {
                pMemValue->sAlias = item.sSpan;
            }
            else
            {//����
                m_tError.FillErrMsg(ERR_SQL_INVALID,"sName and sSpan is NULL");
                continue;
            }
            m_listOutputCollist.vMemValue.push_back(pMemValue);
        }
    }
    //update list
    if(TK_UPDATE == m_stSqlStruct.iSqlType)
    {
        ST_ID_LIST * pstIdList = m_stSqlStruct.pstIdList;
        for(i = 0;i<pstIdList->iIdNum;i++)
        {
            m_listOutputCollist.vMemValue.push_back(pstIdList->pstItem[i].pExpr->pExprValue);
        }

    }
    TADD_DETAIL("m_listOutputCollist.size[%d].",m_listOutputCollist.vMemValue.size());
    //order by list
    if(NULL != m_stSqlStruct.pOrderby)
    {
        ST_EXPR_LIST * pList = m_stSqlStruct.pOrderby;
        for(i = 0;i < pList->iItemNum;i++)
        {
            m_listOutputOrderby.vMemValue.push_back(pList->pExprItems[i].pExpr->pExprValue);
        }
    }
    TADD_DETAIL("m_listOutputOrderby.size[%d].",m_listOutputOrderby.vMemValue.size());
    //group by
    if(NULL != m_stSqlStruct.pGroupby)
    {
        ST_EXPR_LIST * pList = m_stSqlStruct.pGroupby;
        for(i = 0;i < pList->iItemNum;i++)
        {
            m_listOutputGroupBy.vMemValue.push_back(pList->pExprItems[i].pExpr->pExprValue);
        }
    }
    TADD_DETAIL("m_listOutputGroupBy.size[%d].",m_listOutputGroupBy.vMemValue.size());
    //having
    if(NULL != m_stSqlStruct.pHaving)
    {
        m_listOutputHaving.vMemValue.push_back(m_stSqlStruct.pHaving->pExprValue);
    }
    //limit offset
    if(NULL != m_stSqlStruct.pstLimit)
    {
        m_listOutputLimit.vMemValue.push_back(m_stSqlStruct.pstLimit->pExprValue);
        if(NULL != m_stSqlStruct.pstOffset)
        {
            m_listOutputLimit.vMemValue.push_back(m_stSqlStruct.pstOffset->pExprValue);
        }
    }
    TADD_DETAIL("m_listOutputLimit.size[%d].",m_listOutputLimit.vMemValue.size());
    TADD_FUNC("Finish");
}

/******************************************************************************
* ��������	:  CollectInputValue
* ��������	:  �Ѽ���Ҫ�����ֵ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbSqlParser::CollectInputValue()
{
    TADD_FUNC("Start.");
    ST_WALKER stWalker;
    stWalker.xExprCallback = FillArrayInput;
    stWalker.pMdbSqlParser = this;
    stWalker.iInputType  = WRC_InputCollist;
    TWalker::WalkExprList(&stWalker,m_stSqlStruct.pColList);
    TWalker::WalkExpr(&stWalker,m_stSqlStruct.pstRouterID);//�Ѽ�·����Ϣ
    TWalker::WalkExpr(&stWalker,m_stSqlStruct.pHaving);//
    stWalker.iInputType  = WRC_InputWhere;
    TWalker::WalkExpr(&stWalker,m_stSqlStruct.pWhere);
    stWalker.iInputType  = WRC_InputOrderby;
    TWalker::WalkExprList(&stWalker,m_stSqlStruct.pOrderby);
    stWalker.iInputType  = WRC_InputLimit;
    TWalker::WalkExpr(&stWalker,m_stSqlStruct.pstLimit);
    TWalker::WalkExpr(&stWalker,m_stSqlStruct.pstOffset);
    stWalker.iInputType  = WRC_InputGroupBy;
    TWalker::WalkExprList(&stWalker,m_stSqlStruct.pGroupby);

    TADD_FUNC("Finish.");
}

/******************************************************************************
* ��������	:  ExecuteWhere
* ��������	:  ����where����ֵ
* ����		:  ��
* ���		:  bResult - ������true/false
* ����ֵ	:  0 - ����ɹ���!0 - ����ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ExecuteWhere(bool & bResult)
{
    TADD_FUNC("Start");
    int iRet = 0;
    if(NULL == m_stSqlStruct.pWhere)
    {//û��where ����
        TADD_DETAIL("where is NULL");
        bResult = true;
    }
    else
    {//����where����
        m_tMdbExpr.SetExecRowPos(m_stSqlStruct.iRowPos);
        //CHECK_RET_FILL(m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pWhere,CALC_PER_Row),ERR_SQL_CAL_WHERE_EXP_ERROR,"ResetCalcFlag  where statemet failed..");
        CHECK_RET_FILL(m_tMdbExpr.CalcExpr(m_stSqlStruct.pWhere),ERR_SQL_CAL_WHERE_EXP_ERROR,"calc where statemet failed..");
        ST_MEM_VALUE *pValue = m_stSqlStruct.pWhere->pExprValue;
        if(MemValueHasProperty(pValue,MEM_Int))
        {
            bResult = pValue->lValue == 0 ? false:true;
        }
        else
        {
            CHECK_RET_FILL(ERR_SQL_CAL_WHERE_EXP_ERROR,ERR_SQL_CAL_WHERE_EXP_ERROR,"where result is error..");
        }
    }
    TADD_FUNC("Finish,iRet=[%d],bResult=[%d]",iRet,bResult);
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteSQL
* ��������	:  ���﷨����������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ExecuteSQL()
{
    TADD_FUNC("Start.iRowPos = [%lld].",m_stSqlStruct.iRowPos);
    int iRet = 0;
    m_tMdbExpr.SetExecRowPos(m_stSqlStruct.iRowPos);
    //CHECK_RET_FILL(m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pColList,CALC_PER_Row),ERR_SQL_CALCULATE_EXP_ERROR,"ResetCalcExprList[ColList] failed.");
    CHECK_RET_FILL(m_tMdbExpr.CalcExprList(m_stSqlStruct.pColList),ERR_SQL_CALCULATE_EXP_ERROR,"CalcExprList[ColList] failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::SimpleColumnCheck(ST_EXPR * pstExpr)
{
	int iRet = 0;
	if(pstExpr == NULL) return iRet;
	TMdbColumn * pColumn = NULL;
	pColumn = NULL;
	if(pstExpr->iOpcode == TK_ID &&
		pstExpr->pExprValue->pColumn != NULL)
	{
		pColumn = pstExpr->pExprValue->pColumn;
		
		bool bMatch = false;
		if(m_stSqlStruct.pGroupby != NULL)
		{
			for(int j = 0; j < m_stSqlStruct.pGroupby->iItemNum; j++)
			{
				ST_EXPR * pstExprTemp =  m_stSqlStruct.pGroupby->pExprItems[j].pExpr;
	            if(pColumn == pstExprTemp->pExprValue->pColumn)
	            {//����ͬ
	                bMatch = true;
	                break;
	            }
			}
		}
		if(!bMatch)
		{
			if(m_stSqlStruct.bIsSingleGroupFunc)
			{
				TADD_ERROR(ERR_SQL_INVALID, "Not a single-group group function.");
				return ERR_SQL_INVALID;
			}
	    	m_stSqlStruct.bIsSimpleCol = true;
		}
	}
	else if(pstExpr->iOpcode == TK_PLUS ||
		pstExpr->iOpcode == TK_MINUS||
		pstExpr->iOpcode == TK_STAR||
		pstExpr->iOpcode == TK_SLASH)
	{
		ST_EXPR * pstExprTemp =  pstExpr->pLeft;
		if(pstExprTemp != NULL)
		{
			SimpleColumnCheck(pstExprTemp);
		}
		pstExprTemp =  pstExpr->pRight;
		if(pstExprTemp != NULL)
		{
			SimpleColumnCheck(pstExprTemp);
		}
	}
	return iRet;
}

/******************************************************************************
* ��������	:  FillMdbInfo
* ��������	:  ����ʽ�������MDB�����Ϣ(����Ϣ)
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ���!0- ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillMdbInfo()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    ST_WALKER stWalker;
    stWalker.xExprCallback = FillExprColumn;
    stWalker.pMdbSqlParser = this;
    CHECK_RET_FILL(TWalker::WalkExprList(&stWalker,m_stSqlStruct.pColList),ERR_SQL_FILL_MDB_INFO,"Fill mdb info [select/update column] error.");
    CHECK_RET_FILL(TWalker::WalkExpr(&stWalker,m_stSqlStruct.pWhere),ERR_SQL_FILL_MDB_INFO,"Fill mdb info [where] error.");
    CHECK_RET_FILL(TWalker::WalkExprList(&stWalker,m_stSqlStruct.pOrderby),ERR_SQL_FILL_MDB_INFO,"Fill mdb info [order by] error.");
    CHECK_RET_FILL(TWalker::WalkExpr(&stWalker,m_stSqlStruct.pstLimit),ERR_SQL_FILL_MDB_INFO,"Fill mdb info [limit] error.");
    CHECK_RET_FILL(TWalker::WalkExpr(&stWalker,m_stSqlStruct.pstOffset),ERR_SQL_FILL_MDB_INFO,"Fill mdb info [offset] error.");
    CHECK_RET_FILL(TWalker::WalkExpr(&stWalker,m_stSqlStruct.pstRouterID),ERR_SQL_FILL_MDB_INFO,"Fill mdb info [routing id] error.");
    CHECK_RET_FILL(TWalker::WalkExprList(&stWalker,m_stSqlStruct.pGroupby),ERR_SQL_FILL_MDB_INFO,"Fill mdb info [group by] error.");
    CHECK_RET_FILL(TWalker::WalkExpr(&stWalker,m_stSqlStruct.pHaving),ERR_SQL_FILL_MDB_INFO,"Fill mdb info [having] error.");
    CHECK_RET_FILL(FillIdList(m_stSqlStruct.pstIdList),ERR_SQL_FILL_MDB_INFO,"Fill mdb info [where] error.");
	if(m_stSqlStruct.pColList != NULL)
	{
		for(int i = 0; i < m_stSqlStruct.pColList->iItemNum; i++)
		{
			CHECK_RET(SimpleColumnCheck(m_stSqlStruct.pColList->pExprItems[i].pExpr), "SimpleColumnCheck failed!");
		}
	}
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* ��������	:  FillIdList
* ��������	:  ���IDlist����ȡID����Ӧ������Ϣ,
* ����		:  ST_ID_LIST * pIdList - ������idlist
* ���		:  ��
* ����ֵ	:  0 - �ɹ���!0- ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillIdList(ST_ID_LIST * pIdList)
{
    TADD_FUNC("Start,pIdList=[%p]",pIdList);
    int iRet = 0;
    if(NULL == pIdList || pIdList->iIdNum == 0) return iRet;

    int i = 0;
    if(pIdList->iIdNum != m_stSqlStruct.pColList->iItemNum)
    {//insert  ,update �к�ֵ����Ӧ
        CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"column num[%d] != values num[%d]",pIdList->iIdNum,
            m_stSqlStruct.pColList->iItemNum);
    }
    for(i = 0;i<pIdList->iIdNum;i++)
    {//�������Ϣ���е�ֵ
        ID_LIST_ITEM & item = pIdList->pstItem[i];
        TADD_DETAIL("column name=[%s]",item.zName);
        item.pColumn = m_tSqlParserHelper.GetMdbColumn(m_stSqlStruct.pMdbTable,item.zName);//��ȡ����Ϣ
        if(NULL == item.pColumn)
        {
            CHECK_RET_FILL(ERR_TAB_COLUMN_NOT_EXIST,ERR_TAB_COLUMN_NOT_EXIST,"not find column[%s]...",item.zName);
        }
        item.pExpr = m_stSqlStruct.pColList->pExprItems[i].pExpr;//һ��������Ӧһ����ֵ

        CHECK_OBJ(item.pExpr);    
        CHECK_RET(TranslateBlobColumnValue(item.pExpr,item.pColumn),"Translate value BlobColumn[%s] failed.",item.pColumn->sName);
        item.pExpr->pExprValue->pColumnToSet = item.pColumn;//�������ID��ֵ����Ӧ����
    }
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* ��������	:  FillExprColumn
* ��������	:  ����ʽ���������Ϣ
* ����		:  pstWalker - �����࣬pstExpr �����Expr
* ���		:  ��
* ����ֵ	:  0 - �ɹ���!0- ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillExprColumn(ST_WALKER * pstWalker,ST_EXPR * pstExpr)
{
    TADD_FUNC("Start.pstWalker=[%p],pstExpr=[%p]",pstWalker,pstExpr);
    if(NULL == pstExpr || NULL == pstWalker) return WRC_Continue;
    TMdbSqlParser * pMdbSqlParser = pstWalker->pMdbSqlParser;
    if(TK_ID == pstExpr->iOpcode)
    {//����
        TMdbColumn * pColumn  = NULL;
        TADD_DETAIL("Column = [%s].",pstExpr->sTorken);
        pColumn = pMdbSqlParser->m_tSqlParserHelper.GetMdbColumn(pMdbSqlParser->m_stSqlStruct.pMdbTable,pstExpr->sTorken);
        if(NULL == pColumn)
        {//û���ҵ�����Ϣ
            pMdbSqlParser->m_tError.FillErrMsg(ERR_TAB_COLUMN_NOT_EXIST,"column[%s]does not exist....",pstExpr->sTorken);
            return WRC_Abort;
        }
        else
        {
            pstExpr->pExprValue->pColumn = pColumn;
        }
    }
    /*
    else if(TK_ID_TABLENAME == pstExpr->iOpcode)
    {//���������߱�ı���
    TADD_DETAIL("table_name = [%s].",pstExpr->sTorken);
    if(TMdbNtcStrFunc::StrNoCaseCmp(pMdbSqlParser->m_stSqlStruct.sTableName, pstExpr->sTorken) != 0&&
    (NULL == pMdbSqlParser->m_stSqlStruct.sAlias || TMdbNtcStrFunc::StrNoCaseCmp(pMdbSqlParser->m_stSqlStruct.sAlias, pstExpr->sTorken) != 0 )
    )
    {
    pMdbSqlParser->m_tError.FillErrMsg(ERR_TAB_NO_TABLE,"tablename or alias [%s] is not exist....",pstExpr->sTorken);
    return WRC_Abort;
    }
    }
    */
    return WRC_Continue;
}


/******************************************************************************
* ��������	:  FillArrayInput
* ��������	:  �����Ҫ�������Ϣ�ڵ�
* ����		:  pstWalker - �����࣬pstExpr - �����Ľڵ�
* ���		:  ��
* ����ֵ	:  0 - �ɹ���!0- ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillArrayInput(ST_WALKER * pstWalker,ST_EXPR * pstExpr)
{
    TADD_FUNC("Start.pstWalker=[%p],pstExpr=[%p]",pstWalker,pstExpr);
    if(NULL == pstExpr || NULL == pstWalker) return WRC_Continue;
    TMdbSqlParser * pMdbSqlParser = pstWalker->pMdbSqlParser;
    std::vector<_ST_MEM_VALUE *> * pInputArray = NULL;
    switch(pstWalker->iInputType)
    {//�ж������������
    case WRC_InputWhere:
        pInputArray = &(pMdbSqlParser->m_listInputWhere.vMemValue);
        break;
    case WRC_InputCollist:
        pInputArray = &(pMdbSqlParser->m_listInputCollist.vMemValue);
        break;
    case WRC_InputOrderby:
        pInputArray = &(pMdbSqlParser->m_listInputOrderby.vMemValue);
        break;
    case WRC_InputLimit:
        pInputArray = &(pMdbSqlParser->m_listInputLimit.vMemValue);//������limit�в����Ժ�������Ϣ
        break;
    case WRC_InputGroupBy:
        pInputArray = &(pMdbSqlParser->m_listInputGroupBy.vMemValue);
        break;
    }
    if(NULL == pInputArray)
    {
        pMdbSqlParser->m_tError.FillErrMsg(ERR_SQL_INVALID,"pstWalker->iInputType[%d] error",pstWalker->iInputType);
        return WRC_Abort;
    }
    switch(pstExpr->iOpcode)
    {
    case TK_VARIABLE://�󶨱���
        {        
            if(NULL == pstExpr->pExprValue->sAlias)
            {
                pstExpr->pExprValue->sAlias = pstExpr->sTorken +1;//ȥ��:
            }
            std::vector<ST_MEM_VALUE *>::iterator itor  = pMdbSqlParser->m_listInputVariable.vMemValue.begin();
            for(;itor != pMdbSqlParser->m_listInputVariable.vMemValue.end();++itor)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp((*itor)->sAlias,pstExpr->pExprValue->sAlias) == 0 )
                {//���ظ��İ󶨱�����
                    pMdbSqlParser->m_tError.FillErrMsg(ERR_SQL_INVALID,"Bind Param[%s] is duplicate",pstExpr->pExprValue->sAlias);
                    return WRC_Abort;
                }
            }
            pstExpr->pExprValue->iFlags |= MEM_Variable;
            pMdbSqlParser->m_listInputVariable.vMemValue.push_back(pstExpr->pExprValue);
            if(NULL == pstExpr->pExprValue->pColumnToSet )
            {//�ȸ��ݰ󶨱�������ȡ���ܵ���Ҫ���õı�����Ϣ���ع���ʹ��
                pstExpr->pExprValue->pColumnToSet = pMdbSqlParser->m_tSqlParserHelper.GetMdbColumn(pMdbSqlParser->m_stSqlStruct.pMdbTable,pstExpr->pExprValue->sAlias);
            }
        } 
        break;
    case TK_ID://����
        if(NULL == pstExpr->pExprValue->sAlias)
        {
            pstExpr->pExprValue->sAlias = pstExpr->sTorken;
        }
        pInputArray->push_back(pstExpr->pExprValue);
        break;
    case TK_FUNCTION://�ۺϺ�������ʱֵ
        {
            if(FuncHasProperty(pstExpr->pFunc->pFuncDef,FUNC_AGGREAGATE))
            {//�ǾۺϺ���
                pMdbSqlParser->m_listInputAggValue.vMemValue.push_back(pstExpr->pExprValue);
                pMdbSqlParser->m_vAggExpr.push_back(pstExpr);
            }
        }
        break;
    }
    TADD_FUNC("Finish");
    return WRC_Continue;
}

/******************************************************************************
* ��������	:  CollectWhereIndex
* ��������	:  �Ѽ�where�����еĿ���ʹ��index �Լ�ȫ����������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ���!0- ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::CollectWhereIndex()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_RET_FILL(m_MdbIndexCtrl.AttachTable(m_stSqlStruct.pShmDSN,m_stSqlStruct.pMdbTable),
        ERR_DB_NOT_CONNECTED,
        "m_MdbIndexCtrl.AttachTable failed....");//�Ѽ�table����Ӧ��index��Ϣ

	m_stSqlStruct.pScanAllIndex = m_pHintIndex;
	if(NULL == m_stSqlStruct.pScanAllIndex)
	{
		m_stSqlStruct.pScanAllIndex = m_MdbIndexCtrl.GetScanAllIndex();//��ȡ�������е�index
	}

    if(NULL != m_stSqlStruct.pWhere)
    {
        CHECK_RET_FILL(OptimizeWhereClause(m_stSqlStruct.pWhere,m_stSqlStruct.tWhereOrClause),ERR_SQL_INVALID, "Optimize Where failed.");//�Ż�where ����
        CHECK_RET_FILL(GetWhereIndex(m_stSqlStruct.vIndexUsed,m_stSqlStruct.tWhereOrClause),ERR_SQL_INVALID,"Get Where Index failed.");//�Ѽ�����
        CHECK_RET_FILL(CheckLPM(m_stSqlStruct.pWhere, m_stSqlStruct.vIndexUsed),ERR_SQL_INVALID, "CheckLPM failed.");//���where������ƥ������
	}
    TADD_FUNC("Finish.vIndexUsed[%d].",m_stSqlStruct.vIndexUsed.size());
    return iRet;
}


int TMdbSqlParser::FillHintInfo()
{
	int iRet = 0;
	_ST_EXPR_LIST *  pHintList =  m_stSqlStruct.pHintList;

	if(NULL==pHintList){return iRet;}

	CHECK_RET_FILL(m_MdbIndexCtrl.AttachTable(m_stSqlStruct.pShmDSN,m_stSqlStruct.pMdbTable),
			ERR_DB_NOT_CONNECTED,
			"m_MdbIndexCtrl.AttachTable failed....");//�Ѽ�table����Ӧ��index��Ϣ
	
	for(int i = 0;i<pHintList->iItemNum;i++)
	{
		if(TK_HINT == pHintList->pExprItems[i].pExpr->iOpcode)
		{
			ST_EXPR *pRight = pHintList->pExprItems[i].pExpr->pRight;
			ST_EXPR *pLeft = pHintList->pExprItems[i].pExpr->pLeft;
			if(TK_INDEX == pRight->iOpcode)
			{
				char* pIndexName = pLeft->pExprValue->sValue;
				CHECK_OBJ(pIndexName);
				TADD_NORMAL("Hint Index name = %s",pIndexName);
				m_pHintIndex= m_MdbIndexCtrl.GetIndexByName(pIndexName);
				if(NULL == m_pHintIndex)
				{
					TADD_WARNING("Hint Index name = %s, is invalid",pIndexName);
				}				
				return iRet;				
			}			
		}

		//if there are more hints, deal with it below
		//if(TK_HINT == pHintList->pExprItems[i].pExpr->iOpcode){......}

	}

	return iRet;
}

/******************************************************************************
* ��������	:  GetWhereIndex
* ��������	:  ��ȡ��ʹ�õ�indexֵ
* ����		:  ��
* ���		:  vIndex - ��ȡ����������Ϣ
* ����ֵ	:  0 - �ɹ���!0- ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::GetWhereIndex(std::vector< ST_INDEX_VALUE >* & pVIndex)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int i = 0;
    std::vector< ST_INDEX_VALUE>::iterator itor = m_stSqlStruct.vIndexUsed.begin();
    for(;itor != m_stSqlStruct.vIndexUsed.end();++itor)
    {
        for(i = 0;i<MAX_INDEX_COLUMN_COUNTS;i++)
        {
            //CHECK_RET_FILL(m_tMdbExpr.ResetCalcFlagByCalcLevel((*itor).pExprArr[i],CALC_PER_Row),ERR_SQL_CALCULATE_EXP_ERROR,"CalcExpr[where index] error...");
            CHECK_RET_FILL(m_tMdbExpr.CalcExpr((*itor).pExprArr[i]),ERR_SQL_CALCULATE_EXP_ERROR,
                "CalcExpr[where index] error...");
            TADD_DETAIL("Calc index[%s][%d],value[%s].",(*itor).pstTableIndex->pIndexInfo->sName,i,
                m_tMdbExpr.ShowExprValue((*itor).pExprArr[i]));
        }
    }
    pVIndex = &(m_stSqlStruct.vIndexUsed);//����
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  OptimizeWhereClause
* ��������	: �Ż�where ����,��where����ȫ��ת����(A and B and c) or (A and C and D)����ʽ
* ����		:  
* ���		:  
* ����ֵ	:   0 - �ɹ���!0- ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::OptimizeWhereClause(ST_EXPR * pstExpr,ST_WHERE_OR_CLAUSE & tWhereOrClause)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(NULL == pstExpr){return 0;}

    ST_WHERE_OR_CLAUSE tLeftWOrC;
    ST_WHERE_OR_CLAUSE tRightWOrC;
    OptimizeWhereClause(pstExpr->pLeft,tLeftWOrC);
    OptimizeWhereClause(pstExpr->pRight,tRightWOrC);
    switch(pstExpr->iOpcode)
    {
    case TK_AND://���
        {
            size_t i = 0;
            for(i = 0;i < tLeftWOrC.m_vAndClause.size();++i)
            {
                size_t j = 0;
                for(j = 0;j < tRightWOrC.m_vAndClause.size();++j)
                {
                    ST_WHERE_AND_CLAUSE tWhereAndCaluse;
                    tWhereAndCaluse.m_vExpr.insert(tWhereAndCaluse.m_vExpr.end(),tLeftWOrC.m_vAndClause[i].m_vExpr.begin(),tLeftWOrC.m_vAndClause[i].m_vExpr.end());
                    tWhereAndCaluse.m_vExpr.insert(tWhereAndCaluse.m_vExpr.end(),tRightWOrC.m_vAndClause[j].m_vExpr.begin(),tRightWOrC.m_vAndClause[j].m_vExpr.end());
                    tWhereOrClause.m_vAndClause.push_back(tWhereAndCaluse);
                }
            }
        }
        break;
    case TK_OR: //����or �ϳ�һ��or
        {
            tWhereOrClause.m_vAndClause.insert(tWhereOrClause.m_vAndClause.end(),tLeftWOrC.m_vAndClause.begin(),tLeftWOrC.m_vAndClause.end());
            tWhereOrClause.m_vAndClause.insert(tWhereOrClause.m_vAndClause.end(),tRightWOrC.m_vAndClause.begin(),tRightWOrC.m_vAndClause.end());
        }
        break;
    default://����������С���Ӵ���
        {
            ST_WHERE_AND_CLAUSE tWhereAndCaluse;
            tWhereAndCaluse.m_vExpr.push_back(pstExpr);

            tWhereOrClause.m_vAndClause.push_back(tWhereAndCaluse);
        }
        break;
    }
    TADD_FUNC("Finish");
    return iRet;
}
/******************************************************************************
* ��������	:  GetWhereIndex
* ��������	:  ��ȡwhere�����е�������Ϣ
* ����		:  pstExpr - where ������
* ���		:  vIndex - ��ȡ����������Ϣ
* ����ֵ	:   0 - �ɹ���!0- ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::GetWhereIndex(std::vector< ST_INDEX_VALUE > & vIndexValue,ST_WHERE_OR_CLAUSE & tWhereOrClause)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //��ѯ����where���е�����
    std::vector<ST_WHERE_AND_CLAUSE> & vStWandC = tWhereOrClause.m_vAndClause;
    std::vector< ST_INDEX_COLUMN > vIndexColumn;
    size_t i = 0;
    for( i = 0;i < vStWandC.size();++i)
    {
        vIndexColumn.clear();
        size_t j = 0;          
        for(j = 0;j < vStWandC[i].m_vExpr.size();j++)
        {
            GetSubWhereIndex(vStWandC[i].m_vExpr[j],vIndexColumn);
        }
        size_t iBefore = vIndexValue.size();
        m_MdbIndexCtrl.GetIndexByIndexColumn(vIndexColumn,vIndexValue,m_pHintIndex);
        if(iBefore == vIndexValue.size())
        {//��һ��û���������Ͷ���������
            vIndexValue.clear();
            break;
        }
    }
    TADD_FUNC("Finish.vIndex.size=[%d],vIndexColumn.size=[%d]",vIndexValue.size(),vIndexColumn.size());
    return iRet;
}

//�ƥ�䲻��������жϣ���Ҫ�����⴦��
int TMdbSqlParser::CheckLPM(ST_EXPR * pstExpr,std::vector< ST_INDEX_VALUE > & vIndexValue)
{
	TADD_FUNC("Start.");
    int iRet = 0;
    if(NULL == pstExpr){return 0;}
	CheckLPM(pstExpr->pLeft,vIndexValue);
	CheckLPM(pstExpr->pRight,vIndexValue);
	if(pstExpr->pExprValue->pColumn)
	{	
		int iColNoPos = 0;
		ST_TABLE_INDEX_INFO * pTableIndex = m_MdbIndexCtrl.GetIndexByColumnPos(pstExpr->pExprValue->pColumn->iPos,iColNoPos);
		if(pTableIndex && pTableIndex->pIndexInfo->m_iAlgoType==INDEX_TRIE)
		{
			for(size_t i=0; i<vIndexValue.size(); i++)
			{
				ST_INDEX_VALUE stIndexValue = vIndexValue[i];
				if(0 == TMdbNtcStrFunc::StrNoCaseCmp(stIndexValue.pstTableIndex->pIndexInfo->sName, pTableIndex->pIndexInfo->sName))
				{
					MemValueSetProperty(pstExpr->pExprValue, MEM_LPM);
					printf("Where %s set LPM\n",pstExpr->pExprValue->pColumn->sName);
				}
			}		
		}
	}
	return iRet;
}

/******************************************************************************
* ��������	:  GetSubWhereIndex
* ��������	:  ��ȡ��where�����е�index
* ����		:  pstExpr - where ������
* ���		:  vIndex - ��ȡ����������Ϣ
* ����ֵ	:   0 - �ɹ���!0- ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::GetSubWhereIndex(ST_EXPR * pstExpr,std::vector<ST_INDEX_COLUMN> & vIndexColumn)
{
    TADD_FUNC("Start.pstExpr=[%p]",pstExpr);
    int iRet = 0;
    if(NULL == pstExpr){return 0;}       
    TADD_FLOW("Expr->Op[%d][%s],[%s]",pstExpr->iOpcode,
        TokenName[pstExpr->iOpcode],
        m_tMdbExpr.ReplaceNull(pstExpr->sTorken));
    switch(pstExpr->iOpcode)
    {
    case TK_AND://and ȥ���ظ�������������
    case TK_OR://��or �����£���������
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"expr.opcode can not be [and]/[or].");
        }
    case TK_EQ:// = ֻ��=�Ų��������� a = 1����a=:a ��������ʽ����Ч
        {
            ST_TABLE_INDEX_INFO * pTableIndex = NULL;
            ST_EXPR * pExpr = NULL;
            int iColNoPos = 0;
            if(TK_ID == pstExpr->pLeft->iOpcode && false == m_tMdbExpr.IsContainColumn(pstExpr->pRight)/*TK_ID!= pstExpr->pRight->iOpcode*/)
            {//��֦
                pTableIndex = 
                    m_MdbIndexCtrl.GetIndexByColumnPos(pstExpr->pLeft->pExprValue->pColumn->iPos,iColNoPos);
                pExpr = pstExpr->pRight;//������ֵ
                pExpr->pExprValue->pColumnToSet = pstExpr->pLeft->pExprValue->pColumn;
            }
            else if(TK_ID == pstExpr->pRight->iOpcode && false == m_tMdbExpr.IsContainColumn(pstExpr->pLeft)/*TK_ID!= pstExpr->pLeft->iOpcode*/)
            {//��֦
                pTableIndex = 
                    m_MdbIndexCtrl.GetIndexByColumnPos(pstExpr->pRight->pExprValue->pColumn->iPos,iColNoPos);
                pExpr = pstExpr->pLeft;//������ֵ
                pExpr->pExprValue->pColumnToSet = pstExpr->pRight->pExprValue->pColumn;
            }
            if(NULL != pTableIndex && NULL != pExpr)
            {//��������
                ST_INDEX_COLUMN indexColumn;
                indexColumn.vExpr.push_back(pExpr);
                indexColumn.pColumn = pExpr->pExprValue->pColumnToSet;
                indexColumn.pstTableIndex = pTableIndex;
                TADD_FLOW("collect index[%s],iColNoPos[%d],expr[%s].",
                    pTableIndex->pIndexInfo->sName,
                    iColNoPos,
                    m_tMdbExpr.ReplaceNull(pExpr->sTorken));
                vIndexColumn.push_back(indexColumn);
            }
            break;
            //TODO:��������һ֦�еĲ��ܰ���TK_ID���͵Ľڵ�
        }
    case TK_IN://in�ؼ���
        {//ֻ�е���֦Ϊcolumn��ʱ
            if(TK_ID == pstExpr->pLeft->iOpcode)
            {
                int i = 0;
                _ST_EXPR_LIST * pInArgs = pstExpr->pFunc->pFuncArgs;
                if(pInArgs != NULL)
                {
                    ST_TABLE_INDEX_INFO * pTableIndex = NULL;
                    int iColNoPos = 0;
                    pTableIndex = m_MdbIndexCtrl.GetIndexByColumnPos(pstExpr->pLeft->pExprValue->pColumn->iPos,iColNoPos);
                    if(NULL != pTableIndex)
                    {
                        ST_INDEX_COLUMN indexColumn;
                        for(i = 0;i<pInArgs->iItemNum;++i)
                        {
                            if(m_tMdbExpr.IsContainColumn(pInArgs->pExprItems[i].pExpr) == true)
                            {//in �б��в��ɰ�����
                                break;
                            }
                            indexColumn.vExpr.push_back(pInArgs->pExprItems[i].pExpr);
                            pInArgs->pExprItems[i].pExpr->pExprValue->pColumnToSet = pstExpr->pLeft->pExprValue->pColumn;
                        }
                        indexColumn.pColumn = pstExpr->pLeft->pExprValue->pColumn;
                        indexColumn.pstTableIndex = pTableIndex;
                        TADD_FLOW("collect index[%s],iColNoPos[%d],expr[%s].",
                            pTableIndex->pIndexInfo->sName,
                            iColNoPos,
                            m_tMdbExpr.ReplaceNull(pstExpr->sTorken));
                        vIndexColumn.push_back(indexColumn);
                    }
                }
            }
            break;
        }
    case TK_ID:// ����
        break;
    default:
        break;	
    }
    TADD_FUNC("Finish.,vIndexColumn.size=[%d]",vIndexColumn.size());
    return iRet;
}

#if 0
/******************************************************************************
* ��������	:  RemoveDupIndex
* ��������	:  �Ƴ��ظ�������
* ����		:  vLeftIndex 
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::RemoveDupIndex(std::vector< ST_INDEX_VALUE > & vIndex)
{
    TADD_FUNC("Start.vIndex.size[%d]",vIndex.size());
    int iRet = 0;
    if(vIndex.size() < 2){return 0;}//ֱ�ӷ���
    std::vector<ST_INDEX_VALUE>::iterator itor = vIndex.begin();
    for(;itor != vIndex.end();++itor)
    {
        std::vector<ST_INDEX_VALUE>::iterator itorScan = itor + 1;
        bool  bDup = true;
        for(;itorScan != vIndex.end();)
        {
            if(itor->pstTableIndex->pIndexInfo == itorScan->pstTableIndex->pIndexInfo)
            {
                int i = 0;
                for(i = 0;i< MAX_INDEX_COLUMN_COUNTS;++i)
                {
                    if(itor->pExprArr[i] != itorScan->pExprArr[i])
                    {
                        bDup = false;
                        break;
                    }
                }
            }
            else{bDup = false;}

            if(bDup) {itorScan = vIndex.erase(itorScan);}
            else{++itorScan;}
        }

    }
    TADD_FUNC("Finish.vIndex.size[%d]",vIndex.size());
    return iRet;
}
#endif

/******************************************************************************
* ��������	:  SpanInsertCollumnList
* ��������	:  ����insert column
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::SpanInsertCollumnList(ST_ID_LIST * pExistList)
{
    TADD_FUNC("Start.pExistList=[%p]",pExistList);
    int iRet = 0;
    int i = 0;
    m_listOutputCollist.clear();
    for(i = 0;i<m_stSqlStruct.pMdbTable->iColumnCounts;i++)
    {
        TMdbColumn * pColumn = &(m_stSqlStruct.pMdbTable->tColumn[i]);
        int j = 0;
        bool bIsExist = false;
        for(j = 0;j<pExistList->iIdNum;j++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pColumn->sName,pExistList->pstItem[j].zName) == 0)
            {//�ҵ�
                m_listOutputCollist.vMemValue.push_back(pExistList->pstItem[j].pExpr->pExprValue);
                bIsExist = true;
                break;
            }
        }
        if(bIsExist){continue;}//���ҵ�continue

        if(pColumn->bIsDefault)
        {//��Ĭ��ֵ
            TADD_DETAIL("column[%s] has default value[%s]",pColumn->sName,pColumn->iDefaultValue);
            ST_MEM_VALUE * pMemValue = QMDB_MALLOC->AllocMemValue(NULL);
            CHECK_RET_FILL(m_tSqlTypeCheck.ConverType(pColumn,pMemValue),0,"ConverType error.");
            pMemValue->pColumnToSet = pColumn;
            //����Ĭ��ֵ
            if(MemValueHasProperty(pMemValue,MEM_Int))
            {
                pMemValue->lValue = TMdbNtcStrFunc::StrToInt(pColumn->iDefaultValue);
            }
            else if(MemValueHasAnyProperty(pMemValue,MEM_Str|MEM_Date))
            {
                if(DT_DateStamp == pColumn->iDataType && 0 == TMdbNtcStrFunc::StrNoCaseCmp("sysdate",pColumn->iDefaultValue))
                {
                    SAFESTRCPY(pMemValue->sValue,pMemValue->iSize,m_tSqlParserHelper.GetMdbShmDSN()->GetInfo()->sCurTime);
                }
                else
                {
                    std::string encoded;
                    const char * sDefault = pColumn->iDefaultValue;
                    if(DT_Blob == pColumn->iDataType)
                    {//blob����Ҫת��
                        encoded = Base::base64_encode(reinterpret_cast<const unsigned char*>(pColumn->iDefaultValue),strlen(pColumn->iDefaultValue));
                        sDefault = encoded.c_str();
                    }
                    if((int)strlen(sDefault) >= pMemValue->iSize)
                    {
                        CHECK_RET(ERR_SQL_INVALID,"Column[%s] is blob,after encode=[%s],len[%d] >= column-size[%d]",
                            pColumn->sName,sDefault,strlen(sDefault),pMemValue->iSize);
                    }
                    SAFESTRCPY(pMemValue->sValue,pMemValue->iSize,sDefault);
                }
            }  
            m_listOutputCollist.vMemValue.push_back(pMemValue);
            m_arrOtherMemValue.push_back(pMemValue);//�����Է�ֹ�ڴ�й¶
        }
        else if(pColumn->m_bNullable)
        {
            TADD_DETAIL("column[%s] has default value[%s]",pColumn->sName,pColumn->iDefaultValue);
            ST_MEM_VALUE * pMemValue = QMDB_MALLOC->AllocMemValue(NULL);
            CHECK_RET_FILL(m_tSqlTypeCheck.ConverType(pColumn,pMemValue),0,"ConverType error.");
            pMemValue->pColumnToSet = pColumn;
            //����Ĭ��ֵnull
            pMemValue->SetNull();
            m_listOutputCollist.vMemValue.push_back(pMemValue);
            m_arrOtherMemValue.push_back(pMemValue);//�����Է�ֹ�ڴ�й¶
        }
        else
        {//û��Ĭ��ֵ������
            CHECK_RET_FILL(ERR_TAB_COLUMN_VALUE_INVALID,ERR_TAB_COLUMN_VALUE_INVALID,
                "column[%s] has no default value...",pColumn->sName);
        }

    }
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* ��������	:  SpanUpdateCollumnList
* ��������	:  ����update column,�ɼ���Ҫ�����ֵ�б�
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::SpanUpdateCollumnList(ST_ID_LIST *pstIdList)
{
    TADD_FUNC("Start.pstIdList=[%p]",pstIdList);
    int iRet = 0;
    int i = 0;
    for(i = 0;i<pstIdList->iIdNum;i++)
    {
        m_listOutputCollist.vMemValue.push_back(pstIdList->pstItem[i].pExpr->pExprValue);
    }
    TADD_FUNC("Finish");
    return iRet;
}


/******************************************************************************
* ��������	:  CleanOtherMem
* ��������	:  ���������ڴ����ݣ���ֹ�ڴ�й¶
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbSqlParser::CleanOtherMem()
{
    std::vector<_ST_MEM_VALUE *>::iterator itor = m_arrOtherMemValue.begin();
    for(;itor != m_arrOtherMemValue.end();++itor )
    {
        QMDB_MALLOC->ReleaseMemValue(*itor);
    }
    m_arrOtherMemValue.clear();
    std::vector<_ST_EXPR *>::iterator itorExpr = m_arrOtherExpr.begin();
    for(;itorExpr != m_arrOtherExpr.end();++itorExpr )
    {
        QMDB_MALLOC->ReleaseExpr(*itorExpr);
    }
    m_arrOtherExpr.clear();
}

/******************************************************************************
* ��������	:  SpanTKALLCollumnList
* ��������	:  ��չ* �ֶ�֧�� *,����֧��a.*
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::SpanTKALLCollumnList(ST_EXPR_LIST * pCollist)
{
    TADD_FUNC("Start.pCollist=[%p]",pCollist);
    int iRet = 0;
    int i = 0;
    for(i = 0;i<pCollist->iItemNum;i++)
    {
        ST_EXPR_ITEM & item = pCollist->pExprItems[i];
        if(TK_ALL == item.pExpr->iOpcode )
        {//*
            TADD_FUNC("Find token[*]");
            int j = 0;
            TMdbExpr tMdbExpr;
            tMdbExpr.DeleteExpr(pCollist,item.pExpr);
            TMdbTable * pTable = m_stSqlStruct.pMdbTable;
            for(j = 0;j < pTable->iColumnCounts;j++)
            {
                Token token;
                token.z = pTable->tColumn[j].sName;
                token.n = strlen(pTable->tColumn[j].sName);
                ST_EXPR * pstExpr = tMdbExpr.BuildPExpr(TK_ID,NULL,NULL,&token);
                tMdbExpr.ExprListAppend(pCollist,pstExpr);
                pCollist->pExprItems[pCollist->iItemNum - 1].sSpan = 
                    QMDB_MALLOC->CopyFromStr(pTable->tColumn[j].sName);

            }		
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* ��������	:  ClearMemValue
* ��������	:  ��������,ֻ���������ݣ��������ڴ�����
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbSqlParser::ClearMemValue(ST_MEM_VALUE_LIST & stMemValueList)
{
    int i = 0;
    for(i = 0;i<stMemValueList.iItemNum;i++)
    {
        ST_MEM_VALUE * pstMemValue = stMemValueList.pValueArray[i];
        pstMemValue->ClearValue();
        /*
        if(MemValueHasProperty(pstMemValue,MEM_Str))
        {
        //memset(pstMemValue->sValue,0x00,pstMemValue->iSize);
        pstMemValue->sValue[0] = 0;
        }
        pstMemValue->lValue = 0;
        MemValueClearProperty(pstMemValue,MEM_Null);//ȥ��null���
        */
    }
    return;
}

/******************************************************************************
* ��������	:  CollectChangeIndex
* ��������	:  ��ȡupdate ����insert����������������������� ��ֵ��
�����������������һ����û�б�������ֶ����MemValue���ں��Զ����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::CollectChangeIndex(ST_ID_LIST *pstIdList)
{
    TADD_FUNC("Start.pstIdList=[%p]",pstIdList);
    int iRet = 0;
    int i = 0;
    int j = 0;
    int iColNoPos = 0;
    ST_TABLE_INDEX_INFO * pTableIndex = NULL;
    for(i = 0;i < pstIdList->iIdNum;i++)
    {	int iCurIndexPos = -1;
    TMdbColumn * pColumnToSet = pstIdList->pstItem[i].pExpr->pExprValue->pColumnToSet;
    while((pTableIndex = m_MdbIndexCtrl.GetAllIndexByColumnPos(pColumnToSet->iPos,iColNoPos,iCurIndexPos)) != NULL)
    { 
        //pTableIndex = m_MdbIndexCtrl.GetIndexByColumnPos(pColumnToSet->iPos,iColNoPos);
        //if(NULL == pTableIndex){continue;}//��������
        //�鿴��vIndexUpdate���Ƿ��Ѿ��м�¼
        bool bFind = false;
        for(j = 0;j < (int)m_stSqlStruct.vIndexChanged.size();j++)
        {
            if(pTableIndex == m_stSqlStruct.vIndexChanged[j].pstTableIndex)
            {
                bFind = true;
                m_stSqlStruct.vIndexChanged[j].pExprArr[iColNoPos] = pstIdList->pstItem[i].pExpr;
            }
        }
        if(!bFind)
        {
            ST_INDEX_VALUE stIndexValue;
            stIndexValue.pstTableIndex = pTableIndex;
            stIndexValue.pExprArr[iColNoPos] = pstIdList->pstItem[i].pExpr;;
            m_stSqlStruct.vIndexChanged.push_back(stIndexValue);
        }
    }
    }
    //�����µ����������һ����ֵ���ں���(����set��)
    for(i = 0;i <(int) m_stSqlStruct.vIndexChanged.size();i++)
    {
        ST_INDEX_VALUE & stIndexValue = m_stSqlStruct.vIndexChanged[i];
        if(HT_CMP ==  stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType)
        {
            for(j = 0;j < MAX_INDEX_COLUMN_COUNTS;j++)
            {
                int iColumnPos = stIndexValue.pstTableIndex->pIndexInfo->iColumnNo[j];
                if(iColumnPos >= 0 && NULL == stIndexValue.pExprArr[j])
                {//��ֵ���ڴ���
                    ST_EXPR * pTempExpr = QMDB_MALLOC->AllocExpr();
                    pTempExpr->pExprValue = QMDB_MALLOC->AllocMemValue(NULL);
                    pTempExpr->pExprValue->pColumn = &(m_stSqlStruct.pMdbTable->tColumn[iColumnPos]);
                    m_tSqlTypeCheck.ConverType(pTempExpr->pExprValue->pColumn,pTempExpr->pExprValue);
                    m_listInputCollist.vMemValue.push_back(pTempExpr->pExprValue);
                    m_arrOtherExpr.push_back(pTempExpr);//��ֹ�ڴ�й¶
                    stIndexValue.pExprArr[j] = pTempExpr;
                }
            }
        }
    }
    TADD_FUNC("Finish.vIndexChanged.size=[%d]",m_stSqlStruct.vIndexChanged.size());
    return iRet;
}



/******************************************************************************
* ��������	:  ExecuteOrderby
* ��������	:  ����orderby ��ֵ
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ExecuteOrderby()
{
    // m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pOrderby,CALC_PER_Row);
    return m_tMdbExpr.CalcExprList(m_stSqlStruct.pOrderby);
}


/******************************************************************************
* ��������	:  ExecuteGroupby
* ��������	:  ����Group by ��ֵ
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ExecuteGroupby()
{
    //m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pGroupby,CALC_PER_Row);
    return m_tMdbExpr.CalcExprList(m_stSqlStruct.pGroupby);
}
/******************************************************************************
* ��������	:  ExecuteHaving
* ��������	:  ����having��ֵ
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ExecuteHaving()
{
    //m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pHaving,CALC_PER_Row);
    return m_tMdbExpr.CalcExpr(m_stSqlStruct.pHaving);
}


/******************************************************************************
* ��������	:  FillOrderbyInfo
* ��������	: ���orderby ��Ϣ  
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillOrderbyInfo(ST_EXPR_LIST * pExprList)
{
    TADD_FUNC("Start.pExprList=[%p]",pExprList);
    int iRet = 0;
    int i = 0;
    if(NULL == pExprList){return 0;}
    if(pExprList->iItemNum > MAX_ORDERBY_COUNT)
    {//orderby ��̫��
        CHECK_RET_FILL(ERR_SQL_TOO_MUCH_FOR_ORDERBY,ERR_SQL_TOO_MUCH_FOR_ORDERBY,
            "order by num is too many [%d] > [%d]",pExprList->iItemNum,MAX_ORDERBY_COUNT);
    }
    m_stSqlStruct.stOrderbyInfo.iNum = pExprList->iItemNum;
    TADD_DETAIL("orderby num[%d]",m_stSqlStruct.stOrderbyInfo.iNum);
    for(i = 0;i<pExprList->iItemNum;i++)
    {
        ST_MEM_VALUE * pstMemValue = pExprList->pExprItems[i].pExpr->pExprValue;
        if(MemValueHasProperty(pstMemValue,MEM_Int))
        {
            m_stSqlStruct.stOrderbyInfo.iTypeArray[i] = MEM_Int;
            m_stSqlStruct.stOrderbyInfo.iSizeArray[i] = sizeof(long long);
        }
        else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
        {
            m_stSqlStruct.stOrderbyInfo.iTypeArray[i] = MEM_Str;
            m_stSqlStruct.stOrderbyInfo.iSizeArray[i] = pstMemValue->iSize;
        }
        else
        {
            CHECK_RET_FILL(ERR_SQL_TYPE_INVALID,ERR_SQL_TYPE_INVALID,
                "memValue[%s] type[%d] error...",pstMemValue->sAlias,pstMemValue->iFlags);
        }
        if(0 == i)
        {
            m_stSqlStruct.stOrderbyInfo.iOffsetArray[i] = 0;
        }
        else
        {
            m_stSqlStruct.stOrderbyInfo.iOffsetArray[i] = m_stSqlStruct.stOrderbyInfo.iOffsetArray[i-1] + 
                m_stSqlStruct.stOrderbyInfo.iSizeArray[i-1];
        }
        m_stSqlStruct.stOrderbyInfo.iTotalSize   += m_stSqlStruct.stOrderbyInfo.iSizeArray[i];
        m_stSqlStruct.stOrderbyInfo.iSortType[i] = pExprList->pExprItems[i].iSortOrder;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  ExecuteLimit
* ��������	:  ����limitֵ
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ExecuteLimit()
{
    int iRet = 0;
    // CHECK_RET_FILL(m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pstLimit,CALC_PER_Row),ERR_SQL_CALCULATE_EXP_ERROR,"ResetCalcExpr[Limit] error.");
    CHECK_RET_FILL(m_tMdbExpr.CalcExpr(m_stSqlStruct.pstLimit),ERR_SQL_CALCULATE_EXP_ERROR,"CalcExpr[Limit] error.");
    //CHECK_RET_FILL(m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pstOffset,CALC_PER_Row),ERR_SQL_CALCULATE_EXP_ERROR,"ResetCalcExpr[Offset] error.");
    CHECK_RET_FILL(m_tMdbExpr.CalcExpr(m_stSqlStruct.pstOffset),ERR_SQL_CALCULATE_EXP_ERROR,"CalcExpr[Offset] error.");
    return m_tError.GetErrCode();
}

/******************************************************************************
* ��������	:  ChangeValueToRefence
* ��������	:  ��memvalue�е�ֵ�ĳ�����ֵ,�Ż�ʹ�䲻��Ҫ���ڴ��п�������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ChangeValueToRefence(ST_MEM_VALUE_LIST & tListMemValue)
{
    int iRet = 0;
    std::vector<ST_MEM_VALUE *>::iterator itor = tListMemValue.vMemValue.begin();
    for(;itor !=  tListMemValue.vMemValue.end();++itor)
    {
        ST_MEM_VALUE * pMemValue = *itor;
        if(MemValueHasProperty(pMemValue,MEM_Dyn))
        {//���ڶ�̬������ַ�ֵ�ĳɣ�ȥ����̬����
            MemValueClearProperty(pMemValue,MEM_Dyn);
            SAFE_DELETE_ARRAY(pMemValue->sValue);
            pMemValue->iSize = -1;
        }
    }
    return iRet;
}




/******************************************************************************
* ��������	:  VectorToList
* ��������	:  Ϊ������
* ����		:  
* ���		:  
* ����ֵ	: 
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbSqlParser::VectorToList()
{
    m_listInputCollist.VectorToList();
    m_listInputVariable.VectorToList();
    m_listInputWhere.VectorToList();
    m_listOutputCollist.VectorToList();
    m_listInputOrderby.VectorToList();
    m_listOutputOrderby.VectorToList();
    m_listInputLimit.VectorToList();
    m_listOutputLimit.VectorToList();
    m_listOutputPriKey.VectorToList();
    m_listInputPriKey.VectorToList();
    m_listInputAggValue.VectorToList();

    m_listInputGroupBy.VectorToList();//��Ҫ�����group by
    m_listOutputGroupBy.VectorToList();//��Ҫ�����group by
    m_listInputHaving.VectorToList();
    m_listOutputHaving.VectorToList();//

}
/******************************************************************************
* ��������	:  GenRollbackSql
* ��������	:  ���ɻع���sql���
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::GenRollbackSql(char *  sNewSql)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int i = 0;
    //������ƴ��where ���
    char sWhere[1024] = {0};
    TMdbPrimaryKey & tPriKey = m_stSqlStruct.pMdbTable->m_tPriKey;
    TMdbTable  * pTable = m_stSqlStruct.pMdbTable;
    for(i = 0;i < tPriKey.iColumnCounts;i++)
    {
        sprintf(sWhere+strlen(sWhere)," %s=:%s and",pTable->tColumn[tPriKey.iColumnNo[i]].sName,
            pTable->tColumn[tPriKey.iColumnNo[i]].sName);
    }
    sWhere[strlen(sWhere)-3] = '\0';//ȥ�����һ��'and'
    switch(m_stSqlStruct.iSqlType)
    {
    case TK_INSERT:// ->delete
        {
            sprintf(sNewSql,"delete from %s where %s",pTable->sTableName,sWhere);
        }
        break;
    case TK_DELETE://->insert
        {
            char sColumn[1024] = {0};
            char sValue[1024]  = {0};
            for(i = 0;i < pTable->iColumnCounts;i++)
            {
                sprintf(sColumn+strlen(sColumn)," %s,",pTable->tColumn[i].sName);
                sprintf(sValue+strlen(sValue)," :%s,",pTable->tColumn[i].sName);
            }
            sColumn[strlen(sColumn) -1] = '\0';//ȥ�����һ����
            sValue[strlen(sValue) -1]   = '\0';
            sprintf(sNewSql,"insert into %s (%s)values(%s)",pTable->sTableName,sColumn,sValue);
        }
        break;
    case TK_UPDATE://->update
        {
            char sSet[1024] = {0};
            for(i = 0;i<m_stSqlStruct.pColList->iItemNum;i++)
            {
                TMdbColumn * pColumnToSet = m_stSqlStruct.pColList->pExprItems[i].pExpr->pExprValue->pColumnToSet;
                if(true == pColumnToSet->bIncrementalUpdate())
                {//ʹ�������ع�
                    sprintf(sSet+strlen(sSet)," %s =%s +:%s,",pColumnToSet->sName,pColumnToSet->sName,pColumnToSet->sName);
                }
                else
                {//����ֵ��ʹ��ȫ���ع�
                    sprintf(sSet+strlen(sSet)," %s=:%s,",pColumnToSet->sName,pColumnToSet->sName);
                }
            }
            sSet[strlen(sSet) -1]   = '\0';
            sprintf(sNewSql,"update %s set %s where %s",pTable->sTableName,sSet,sWhere);
        }
        break;
    default:
        CHECK_RET_FILL(-1,0,"GenRollbackSql failed sql type[%d] error.",m_stSqlStruct.iSqlType);
        break;
    }
    TADD_FUNC("Finish.SQL=[%s]",sNewSql);
    return iRet;
}

/******************************************************************************
* ��������	:  CollectPrimaryKey
* ��������	:  ��SQL���Ѽ�������Ϣ
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::CollectPrimaryKeyFromSQL(ST_ID_LIST * pIdList)
{
    TADD_FUNC("Start.pIdList=[%p]",pIdList);
    int iRet = 0;
    CHECK_OBJ_RET_0(pIdList);
    int i = 0;
    for(i = 0;i<pIdList->iIdNum;i++)
    {
        ST_MEM_VALUE * pstMemValue = pIdList->pstItem[i].pExpr->pExprValue;
        if(m_tSqlParserHelper.IsPrimaryKey(m_stSqlStruct.pMdbTable,pstMemValue->pColumnToSet))
        {
            m_listOutputPriKey.vMemValue.push_back(pstMemValue);
            //ͬʱ�Ѽ�����������ֵ���ں�ȥ��䣬���������ظ���У��
            ST_MEM_VALUE * pNewMemValue = QMDB_MALLOC->AllocMemValue(NULL);
            pNewMemValue->pColumn = pstMemValue->pColumnToSet;
            m_tSqlTypeCheck.ConverType(pNewMemValue->pColumn,pNewMemValue);
            m_listInputPriKey.vMemValue.push_back(pNewMemValue);
            m_arrOtherMemValue.push_back(pNewMemValue);//�����Է�ֹ�ڴ�й¶
        }
    }
    TADD_FUNC("Finish m_listInputPriKey.size[%d]",m_listInputPriKey.vMemValue.size());
    return iRet;
}

/******************************************************************************
* ��������	:  CollectPrimaryKeyFromTable
* ��������	:  �ӱ��л�ȡ������Ϣ
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::CollectPrimaryKeyFromTable()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int i = 0;
    TMdbTable * pMdbTable = m_stSqlStruct.pMdbTable;
    for(i = 0;i<pMdbTable->m_tPriKey.iColumnCounts;i++)
    {
        TMdbColumn * pColumn = &(pMdbTable->tColumn[pMdbTable->m_tPriKey.iColumnNo[i]]);
        ST_MEM_VALUE * pNewMemValue = QMDB_MALLOC->AllocMemValue(NULL);
        pNewMemValue->pColumn = pColumn;
        pNewMemValue->pColumnToSet = pColumn;
        m_tSqlTypeCheck.ConverType(pNewMemValue->pColumn,pNewMemValue);
        m_listInputPriKey.vMemValue.push_back(pNewMemValue);
        m_arrOtherMemValue.push_back(pNewMemValue);//�����Է�ֹ�ڴ�й¶
    }
    TADD_FUNC("Finish,m_listInputPriKey.size(%d)",m_listInputPriKey.vMemValue.size());
    return iRet;
}

/******************************************************************************
* ��������	:  AddColumn
* ��������	:  �����
* ����		:  pToken - ����
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::AddColumn(Token *pToken)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    bool bIsExist = false;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
	int iCurPos  = m_pDDLSqlStruct->pTable->iColumnCounts;
    char * sName = QMDB_MALLOC->NameFromToken(pToken);
    do{
        //������ӵ����Ƿ��Ѿ�����
        for(int i=0;i<iCurPos;i++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_pDDLSqlStruct->pTable->tColumn[i].sName,sName) == 0)
            {
                bIsExist = true;
                break;
            }
        }
        if(bIsExist)
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"Column [%s] already exists.",sName);
        }
        //��ʼ������
    	TMdbColumn & tColumn = m_pDDLSqlStruct->pTable->tColumn[iCurPos];
        tColumn.Clear();
        tColumn.iPos = iCurPos;
        SAFESTRCPY(tColumn.sName,sizeof(tColumn.sName),sName);
        m_pDDLSqlStruct->pTable->iColumnCounts ++;
    }while(0);
    TADD_DETAIL("sColumnName = %s",sName);
    QMDB_MALLOC->ReleaseStr(sName);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  AddColumnAttribute
* ��������	:  ���������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::AddColumnAttribute(const char * sAttrName, Token *pToken)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(sAttrName);
    CHECK_OBJ_FILL(pToken);
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pTable);
	int iCurPos  = m_pDDLSqlStruct->pTable->iColumnCounts;
    char * sAttr = QMDB_MALLOC->NameFromToken(pToken);
	TMdbColumn & tColumn = m_pDDLSqlStruct->pTable->tColumn[iCurPos-1];
    do
    {
        TADD_DETAIL("sAttrName = %s,sAttr=%s",sAttrName,sAttr);
        if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"data-len") == 0)
        {
            tColumn.iColumnLen = TMdbNtcStrFunc::StrToInt(sAttr);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"data-type") == 0)
        {	
            tColumn.iDataType = m_pMdbConfig->GetDataType(sAttr);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"default-value") == 0)
        {	
            tColumn.bIsDefault = true;
            SAFESTRCPY(tColumn.iDefaultValue,sizeof(tColumn.iDefaultValue),sAttr);
        }
		else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"rep-type") == 0)
		{
		}
        else
        {
            CHECK_RET_FILL_BREAK(-1,ERR_SQL_NO_COLUMN_ATTR,"no column attrname[%s]",sAttrName);
        }
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sAttr);
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::AddColumnNULLAttribute()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
	int iCurPos  = m_pDDLSqlStruct->pTable->iColumnCounts;
	TMdbColumn & tColumn = m_pDDLSqlStruct->pTable->tColumn[iCurPos-1];
    tColumn.m_bNullable = true;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  BuildCreateTable
* ��������	:  ����create table�﷨
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::BuildCreateTable(int iIFNE,Token * pTableName)//����createtable
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sName =  QMDB_MALLOC->NameFromToken(pTableName);
    do{
        if(NULL != m_pMdbConfig)
        {
            if(m_tSqlParserHelper.GetMdbTable(sName) != NULL)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"table[%s] already exists.",sName);
            }
        }

    	m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);//if not exist
		if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        }      
		CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
		m_pDDLSqlStruct->pTable->Clear();
        //TMdbNtcStrFunc::ToUpper(sName,m_pDDLSqlStruct->pTable->sTableName);
        strcpy(m_pDDLSqlStruct->pTable->sTableName, TMdbNtcStrFunc::ToUpper(sName));
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sName);
	m_pDDLSqlStruct->iSqlType[0] = TK_CREATE;
	m_pDDLSqlStruct->iSqlType[1] = TK_TABLE;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildDropTable(int iIFE,Token * pTableName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sName =  QMDB_MALLOC->NameFromToken(pTableName);
    do{
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        }
		CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
		m_pDDLSqlStruct->pTable->Clear();

        if(NULL != m_pMdbConfig)
        {
            TMdbTable *pTable = m_tSqlParserHelper.GetMdbTable(sName);
            if(pTable == NULL)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                    "Table[%s] does not exist,please check.",sName);
            }
    		m_pDDLSqlStruct->bIfNE = (iIFE != 0?false:true);//if exist
            memcpy(m_pDDLSqlStruct->pTable,pTable,sizeof(TMdbTable));
        }
        SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,sizeof(m_pDDLSqlStruct->pTable->sTableName),sName);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sName);
	m_pDDLSqlStruct->iSqlType[0] = TK_DROP;
	m_pDDLSqlStruct->iSqlType[1] = TK_TABLE;
    TADD_FUNC("Finish.");
    return iRet;

}

int TMdbSqlParser::BuildTruncateTable(int iIFE,Token * pTableName)
{
	TADD_FUNC("Start.");
	int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
	char * sName =  QMDB_MALLOC->NameFromToken(pTableName);
	do{
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        }
		CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);

        if(NULL != m_pMdbConfig)
        {
            TMdbTable *pTable = m_tSqlParserHelper.GetMdbTable(sName);
            if(pTable == NULL)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                    "Table[%s] does not exist,please check.",sName);
            }
    		m_pDDLSqlStruct->bIfNE = (iIFE != 0?false:true);//if exist
            memcpy(m_pDDLSqlStruct->pTable,pTable,sizeof(TMdbTable));
        }
        SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,sizeof(m_pDDLSqlStruct->pTable->sTableName),sName);
    }
    while(0);
	QMDB_MALLOC->ReleaseStr(sName);
	m_pDDLSqlStruct->iSqlType[0] = TK_TRUNCATE;
	m_pDDLSqlStruct->iSqlType[1] = TK_TABLE;
	TADD_FUNC("Finish.");
	return iRet;

}


int TMdbSqlParser::BuildCreateDsn(int iIFNE,Token * pDsnName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sDsnName =  QMDB_MALLOC->NameFromToken(pDsnName);
    do{
        InitDsnAttrName();
        if(m_pMdbConfig != NULL)
        {
            m_pDDLSqlStruct->pDsn = m_pMdbConfig->GetDSN();
    		if(NULL != m_pDDLSqlStruct->pDsn)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"DSN[%s] does exist.",sDsnName);
            }
        }
        m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);//if not exist
        if(NULL == m_pDDLSqlStruct->pDsn)
        {
            m_pDDLSqlStruct->pDsn = new (std::nothrow)TMdbCfgDSN();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pDsn);
            m_pDDLSqlStruct->pDsn->Init();
        }

        if(NULL == m_pDDLSqlStruct->pProAttr)
        {
            m_pDDLSqlStruct->pProAttr = new (std::nothrow)TMdbCfgProAttr();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pProAttr);
            
        }
        SAFESTRCPY(m_pDDLSqlStruct->sDsnName, sizeof(m_pDDLSqlStruct->sDsnName),sDsnName);
        SAFESTRCPY(m_pDDLSqlStruct->pDsn->sName, sizeof(m_pDDLSqlStruct->pDsn->sName),sDsnName);
    }while(0);
    QMDB_MALLOC->ReleaseStr(sDsnName);
    m_pDDLSqlStruct->iSqlType[0] = TK_CREATE;
    m_pDDLSqlStruct->iSqlType[1] = TK_DATABASE;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildAlterDsn(int iIFE,Token * pDsnName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sDsnName =  QMDB_MALLOC->NameFromToken(pDsnName);
    do{
        InitDsnAttrName();
        if(m_pMdbConfig != NULL)
        {
            TMdbCfgDSN  *pDsn = m_pMdbConfig->GetDSN();
            if(NULL == pDsn)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"DSN[%s] does not exist.",sDsnName);
            }
        }
        if(NULL == m_pDDLSqlStruct->pDsn)
        {
            m_pDDLSqlStruct->pDsn = new (std::nothrow)TMdbCfgDSN();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pDsn);
            m_pDDLSqlStruct->pDsn->Clear();
            m_pDDLSqlStruct->pDsn->Init();
        }
        if(NULL == m_pDDLSqlStruct->pProAttr)
        {
            m_pDDLSqlStruct->pProAttr = new (std::nothrow)TMdbCfgProAttr();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pProAttr);
            
        }
        m_pDDLSqlStruct->bIfNE = (iIFE != 0?false:true);//if exist
        for(int i=0;i<MAX_SQL_COUNTS;i++)
        {
            m_pDDLSqlStruct->m_tAttr[i].Clear();
        }
        SAFESTRCPY(m_pDDLSqlStruct->sDsnName, sizeof(m_pDDLSqlStruct->sDsnName),sDsnName);
    }while(0);
    QMDB_MALLOC->ReleaseStr(sDsnName);
    m_pDDLSqlStruct->iSqlType[0] = TK_ALTER;
    m_pDDLSqlStruct->iSqlType[1] = TK_DATABASE;
    TADD_FUNC("Finish.");
    return iRet;
}


int TMdbSqlParser::BuildDropDsn(int iIFE,Token * pDsnName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sDsnName =  QMDB_MALLOC->NameFromToken(pDsnName);
    do{
        if(m_pMdbConfig != NULL)
        {
            m_pDDLSqlStruct->pDsn = m_pMdbConfig->GetDSN();
    		if(NULL == m_pDDLSqlStruct->pDsn)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"DSN[%s] does not exist.",sDsnName);
            }
        }

        m_pDDLSqlStruct->bIfNE = (iIFE != 0?false:true);//if exist
        SAFESTRCPY(m_pDDLSqlStruct->sDsnName,sizeof(m_pDDLSqlStruct->sDsnName),sDsnName);
    }while(0);
    QMDB_MALLOC->ReleaseStr(sDsnName);
    m_pDDLSqlStruct->iSqlType[0] = TK_DROP;
    m_pDDLSqlStruct->iSqlType[1] = TK_DATABASE;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildUser(int iIFNE,Token * pUserName,const int iSqlType)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sUseName =  QMDB_MALLOC->NameFromToken(pUserName);
    CHECK_OBJ_FILL(sUseName);
    do{
        m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);
        if(NULL == m_pDDLSqlStruct->pUser)
        {
            m_pDDLSqlStruct->pUser = new (std::nothrow)TMDbUser();
        }
        CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pUser);
        m_pDDLSqlStruct->pUser->Clear();
        SAFESTRCPY(m_pDDLSqlStruct->pUser->sUser,sizeof(m_pDDLSqlStruct->pUser->sUser),sUseName);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sUseName);
    m_pDDLSqlStruct->iSqlType[0] = iSqlType;
    m_pDDLSqlStruct->iSqlType[1] = TK_USER;
    TADD_FUNC("Finish.");
    return iRet;

}

int TMdbSqlParser::CheckNumberValid(char* sPassword)
{
	 int i = 0;
	 int iRet = 0;

	 while (sPassword[i] != '\0')
	{
		if(sPassword[i] >= '0' && sPassword[i] <= '9')
			i++;
		else
			break;
	}

	if(sPassword[i] != '\0')
	{
		iRet = ERR_APP_INVALID_PARAM;
			
	}

	return iRet;

}

int TMdbSqlParser::CheckStrValid(char* sPassword)
{

	int i = 0;
	int iRet = 0;
	while (sPassword[i] != '\0')
	{
		if((sPassword[i] >= 'a' && sPassword[i] <= 'z') ||
			(sPassword[i] >= 'A' && sPassword[i] <= 'Z')||
			(sPassword[i] >= '0' && sPassword[i] <= '9')||
			sPassword[i] == '_' ||
			sPassword[i] == '#' ||
			sPassword[i] == '$' )
			i++;
		else
			break;
	}

	if(sPassword[i] != '\0')
	{
		iRet = ERR_APP_INVALID_PARAM;
			
	}

	return iRet;

}

int TMdbSqlParser::CheckPasswordValid(char* sPassword)
{
	 TADD_FUNC("Start.");
	 int iRet = 0;
	
	 if(sPassword[0] >= '0' && sPassword[0] <= '9')
	 {
		iRet = CheckNumberValid(sPassword);
			
	 }
	 else if((sPassword[0] >= 'a' && sPassword[0] <= 'z') ||
	 	     (sPassword[0] >= 'A' && sPassword[0] <= 'Z'))
	 {

		iRet = CheckStrValid(sPassword);

	 }

	 else
	 {
	 	iRet = ERR_APP_INVALID_PARAM;

	 }
	 
	 TADD_FUNC("Finish.");
     return iRet;

}
int TMdbSqlParser::AddUserAttribute(Token * pPassword,Token * pAccess)
{
    TADD_FUNC("Start.");
    int iRet = 0;
	int iRtnValue = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sAccess = NULL;
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pUser);
    char * sPassword =  QMDB_MALLOC->NameFromToken(pPassword);
    CHECK_OBJ_FILL(sPassword);
	
    if(NULL != pAccess)
    {
        sAccess =  QMDB_MALLOC->NameFromToken(pAccess);
        CHECK_OBJ_FILL(sAccess);
    }
    else
    {
        sAccess = new (std::nothrow)char[2];
		CHECK_OBJ_FILL(sAccess);
        strcpy(sAccess,"A");
    }
    do{
		//����������õ���Ч��
		iRtnValue = CheckPasswordValid(sPassword);
		CHECK_RET_FILL_BREAK(iRtnValue,iRtnValue,"the password is invalid");
        //���û�������м��ܴ���
        TMdbEncrypt::EncryptEx(sPassword,m_pDDLSqlStruct->pUser->sPwd);
        if(TMdbNtcStrFunc::StrNoCaseCmp(sAccess, "A") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->pUser->sAccess,sizeof(m_pDDLSqlStruct->pUser->sAccess),"Administrator");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAccess, "W") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->pUser->sAccess,sizeof(m_pDDLSqlStruct->pUser->sAccess),"Read-Write");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAccess, "R") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->pUser->sAccess,sizeof(m_pDDLSqlStruct->pUser->sAccess),"ReadOnly");
        }
        else
        {
            CHECK_RET_FILL_BREAK(ERR_APP_INVALID_PARAM,ERR_APP_INVALID_PARAM,\
                "Invalid Access=[%s].",sAccess);
        }
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sPassword);
    QMDB_MALLOC->ReleaseStr(sAccess);
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::ModifyUserAttribute(Token * pPassword,Token * pAccess)
{
    TADD_FUNC("Start.");
    int iRet = 0;
	int iRtnValue = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sAccess = NULL;
    char * sPassword =  NULL;
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pUser);
    do
    {
        if(NULL != pPassword)
        {
            sPassword = QMDB_MALLOC->NameFromToken(pPassword);
            CHECK_OBJ_FILL_BREAK(sPassword);
			iRet = CheckPasswordValid(sPassword);
			CHECK_RET_FILL_BREAK(iRtnValue,iRtnValue,"the password is invalid");
            TMdbEncrypt::EncryptEx(sPassword,m_pDDLSqlStruct->pUser->sPwd);
        }
        if(NULL != pAccess)
        {
            sAccess =  QMDB_MALLOC->NameFromToken(pAccess);
            CHECK_OBJ_FILL_BREAK(sAccess);
            if(TMdbNtcStrFunc::StrNoCaseCmp(sAccess, "A") == 0)
            {
                SAFESTRCPY(m_pDDLSqlStruct->pUser->sAccess,sizeof(m_pDDLSqlStruct->pUser->sAccess),"Administrator");
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(sAccess, "W") == 0)
            {
                SAFESTRCPY(m_pDDLSqlStruct->pUser->sAccess,sizeof(m_pDDLSqlStruct->pUser->sAccess),"Read-Write");
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(sAccess, "R") == 0)
            {
                SAFESTRCPY(m_pDDLSqlStruct->pUser->sAccess,sizeof(m_pDDLSqlStruct->pUser->sAccess),"ReadOnly");
            }
            else
            {
                CHECK_RET_FILL_BREAK(ERR_APP_INVALID_PARAM,ERR_APP_INVALID_PARAM,\
                    "Invalid Access=[%s].",sAccess);
            }
        }  
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sPassword);
    QMDB_MALLOC->ReleaseStr(sAccess);
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildAlterTableSpace(int iIFE,Token * pTablespaceName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sTableSpaceName =  QMDB_MALLOC->NameFromToken(pTablespaceName);
    CHECK_OBJ_FILL(sTableSpaceName);
    //����ռ��Ƿ����
    do{
        m_pDDLSqlStruct->bIfNE = (iIFE != 0?false:true);
        if(m_pDDLSqlStruct->pTablespace == NULL)
        {
            m_pDDLSqlStruct->pTablespace = new (std::nothrow)TMdbTableSpace();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTablespace);
            m_pDDLSqlStruct->pTablespace->Clear();
        }

        if(m_pMdbConfig != NULL)
        {
            TMdbTableSpace *pTableSpace = m_pMdbConfig->GetTableSpaceByName(sTableSpaceName);
            if(pTableSpace== NULL)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"tablespace[%s] does not exist.",sTableSpaceName);
            }
            memcpy(m_pDDLSqlStruct->pTablespace,pTableSpace,sizeof(TMdbTableSpace));
        }
        SAFESTRCPY(m_pDDLSqlStruct->sTableSpaceName,sizeof(m_pDDLSqlStruct->sTableSpaceName),sTableSpaceName);
        SAFESTRCPY(m_pDDLSqlStruct->pTablespace->sName,sizeof(m_pDDLSqlStruct->pTablespace->sName),sTableSpaceName);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sTableSpaceName);
    m_pDDLSqlStruct->iSqlType[0] = TK_ALTER;
    m_pDDLSqlStruct->iSqlType[1] = TK_TABLESPACE;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildCreateTableSpace(int iIFNE,Token * pTablespaceName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sTableSpaceName =  QMDB_MALLOC->NameFromToken(pTablespaceName);
    CHECK_OBJ_FILL(sTableSpaceName);
    //����ռ��Ƿ����
    do{
        if(NULL != m_pMdbConfig)
        {
            if(m_pMdbConfig->GetTableSpaceByName(sTableSpaceName) != NULL)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"tablespace[%s] does exist.",sTableSpaceName);
            }
        }

        m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);//if not exist
        if(NULL == m_pDDLSqlStruct->pTablespace)
        {
            m_pDDLSqlStruct->pTablespace = new (std::nothrow)TMdbTableSpace(); 
        } 
        CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTablespace);
        m_pDDLSqlStruct->pTablespace->Clear();
        SAFESTRCPY(m_pDDLSqlStruct->sTableSpaceName,sizeof(m_pDDLSqlStruct->sTableSpaceName),sTableSpaceName);
        SAFESTRCPY(m_pDDLSqlStruct->pTablespace->sName,sizeof(m_pDDLSqlStruct->pTablespace->sName),sTableSpaceName);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sTableSpaceName);
    m_pDDLSqlStruct->iSqlType[0] = TK_CREATE;
    m_pDDLSqlStruct->iSqlType[1] = TK_TABLESPACE;
    TADD_FUNC("Finish.");
    return iRet;

}

int TMdbSqlParser::BuildDropTableSpace(int iIFE,Token * pTablespaceName)
{   
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sTableSpaceName =  QMDB_MALLOC->NameFromToken(pTablespaceName);
    CHECK_OBJ_FILL(sTableSpaceName);
    //����ռ��Ƿ����
    do{
        if(m_pDDLSqlStruct->pTablespace == NULL)
        {
            m_pDDLSqlStruct->pTablespace = new (std::nothrow)TMdbTableSpace();
        }
        m_pDDLSqlStruct->pTablespace->Clear();

        if(NULL != m_pMdbConfig)
        {
            TMdbTableSpace *pTablespace = m_pMdbConfig->GetTableSpaceByName(sTableSpaceName);
            if(pTablespace == NULL)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"tablespace[%s] does not exist.",sTableSpaceName);
            }
            m_pDDLSqlStruct->bIfNE = (iIFE != 0?false:true);
            memcpy(m_pDDLSqlStruct->pTablespace,pTablespace,sizeof(TMdbTableSpace));
        }
        SAFESTRCPY(m_pDDLSqlStruct->sTableSpaceName,sizeof(m_pDDLSqlStruct->sTableSpaceName),sTableSpaceName);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sTableSpaceName);
    m_pDDLSqlStruct->iSqlType[0] = TK_DROP;
    m_pDDLSqlStruct->iSqlType[1] = TK_TABLESPACE;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::AddTablespaceAttribute(Token * pPageSize,Token * pAskPage,Token* pStorage)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sPageSize = NULL;
    char * sAskPage = NULL;
    char* sStorage = NULL;
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pTablespace);
    do{
        if(NULL == pPageSize)
        {
            m_pDDLSqlStruct->pTablespace->iPageSize = 32*1024;
        }
        else
        {
            sPageSize =  QMDB_MALLOC->NameFromToken(pPageSize);
            CHECK_OBJ_FILL_BREAK(sPageSize);
            m_pDDLSqlStruct->pTablespace->iPageSize = TMdbNtcStrFunc::StrToInt(sPageSize)*1024;
        }

        if(NULL == pAskPage)
        {
            m_pDDLSqlStruct->pTablespace->iRequestCounts = 1000;
        }
        else
        {
            sAskPage  =  QMDB_MALLOC->NameFromToken(pAskPage);
            CHECK_OBJ_FILL_BREAK(sAskPage);
            m_pDDLSqlStruct->pTablespace->iRequestCounts = TMdbNtcStrFunc::StrToInt(sAskPage);
        }

        if(NULL == pStorage)
        {
            m_pDDLSqlStruct->pTablespace->m_bFileStorage = false;
        }
        else
        {
            sStorage  =  QMDB_MALLOC->NameFromToken(pStorage);
            CHECK_OBJ_FILL_BREAK(sStorage);
            m_pDDLSqlStruct->pTablespace->m_bFileStorage = (0 == TMdbNtcStrFunc::StrNoCaseCmp(sStorage, "Y"))?true:false;
        }

    }while(0);
    QMDB_MALLOC->ReleaseStr(sPageSize);
    QMDB_MALLOC->ReleaseStr(sAskPage);
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::ModifyTablespaceAttribute(Token * pPageSize,Token * pAskPage, Token* pStorage)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sPageSize = NULL;
    char * sAskPage = NULL;
    char * sStorage = NULL;
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pTablespace);
    do{
        //ʹ��m_tAttr�����ǰ���������¼�޸ĵ�������
        if(pPageSize != NULL)
        {
            sPageSize =  QMDB_MALLOC->NameFromToken(pPageSize);
            CHECK_OBJ_FILL_BREAK(sPageSize);
            m_pDDLSqlStruct->pTablespace->iPageSize = TMdbNtcStrFunc::StrToInt(sPageSize)*1024;
            SAFESTRCPY(m_pDDLSqlStruct->m_tAttr[0].sAttrName,\
                sizeof(m_pDDLSqlStruct->m_tAttr[0].sAttrName),"PAGESIZE");
        }

        if(pAskPage != NULL)
        {
            sAskPage  =  QMDB_MALLOC->NameFromToken(pAskPage);
            CHECK_OBJ_FILL_BREAK(sAskPage);
            m_pDDLSqlStruct->pTablespace->iRequestCounts = TMdbNtcStrFunc::StrToInt(sAskPage);
            SAFESTRCPY(m_pDDLSqlStruct->m_tAttr[1].sAttrName,\
                sizeof(m_pDDLSqlStruct->m_tAttr[1].sAttrName),"ASKPAGE");
        }

        if(pStorage != NULL)
        {
            sStorage  =  QMDB_MALLOC->NameFromToken(pStorage);
            CHECK_OBJ_FILL_BREAK(sStorage);
            m_pDDLSqlStruct->pTablespace->m_bFileStorage = (0 == TMdbNtcStrFunc::StrNoCaseCmp(sStorage, "Y"))?true:false;
            SAFESTRCPY(m_pDDLSqlStruct->m_tAttr[1].sAttrName,\
                sizeof(m_pDDLSqlStruct->m_tAttr[1].sAttrName),"STORAGE");
        }

    }while(0);
    QMDB_MALLOC->ReleaseStr(sPageSize);
    QMDB_MALLOC->ReleaseStr(sAskPage);
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildDataSourceForOracle(Token * pDataSource)
{
	#ifdef DB_ORACLE
        return BuildDataSource(pDataSource);
	#elif DB_MYSQL    
       //	TADD_ERROR(ERR_SQL_INVALID, "Used DB in sql is wrong ,this qmdb is for mysql"); 
		m_tError.FillErrMsg(ERR_SQL_INVALID,"Used DB in sql is wrong,this qmdb is for oracle");
        return ERR_SQL_INVALID;
	#endif
	
}

int TMdbSqlParser::BuildDataSourceForMySQL(Token * pDataSource)
{

	#ifdef DB_MYSQL
        return BuildDataSource(pDataSource);
	#elif DB_ORACLE    
       	//TADD_ERROR(ERR_SQL_INVALID, "Used DB in sql is wrong ,this qmdb is for mysql"); 
		 m_tError.FillErrMsg(ERR_SQL_INVALID,"Used DB in sql is wrong,this qmdb is for mysql");
        return ERR_SQL_INVALID;
	#endif
}

int TMdbSqlParser::BuildDataSource(Token * pDataSource)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sDataSource =  QMDB_MALLOC->NameFromToken(pDataSource);
    CHECK_OBJ_FILL(sDataSource);
    //����ռ��Ƿ����
    do{
        TMdbNtcSplit mdbSplit1,mdbSplit2;
        TMdbNtcStrFunc::Trim(sDataSource,'\n');
        mdbSplit1.SplitString(sDataSource, '/');
        int iListCount = mdbSplit1.GetFieldCount();
        if(2 != iListCount)
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"Oracle connection[%s] format error.",sDataSource);
        }
        mdbSplit2.SplitString(mdbSplit1[1], '@');
        iListCount = mdbSplit2.GetFieldCount();
        if(2 != iListCount)
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"Oracle connection[%s] format error.",sDataSource);
        }
        if(NULL == m_pDDLSqlStruct->pDsn)
        {
            m_pDDLSqlStruct->pDsn = new (std::nothrow)TMdbCfgDSN();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pDsn);
            m_pDDLSqlStruct->pDsn->Init();
        }
#ifdef DB_ORACLE
        m_pDDLSqlStruct->pDsn->cType = MDB_DS_TYPE_ORACLE;
#elif DB_MYSQL    
        m_pDDLSqlStruct->pDsn->cType = MDB_DS_TYPE_MYSQL;
#else        
        TADD_ERROR(ERROR_UNKNOWN, "[%s:%d]  DB_TYPE is wrong, check Makefile",__FILE__,__LINE__); 
        iRet = -1;
#endif 

        SAFESTRCPY(m_pDDLSqlStruct->pDsn->sOracleUID,sizeof(m_pDDLSqlStruct->pDsn->sOracleUID),mdbSplit1[0]);
        SAFESTRCPY(m_pDDLSqlStruct->pDsn->sOracleID,sizeof(m_pDDLSqlStruct->pDsn->sOracleID),mdbSplit2[1]);
        //������Ҫ���ܴ���
        TMdbEncrypt::EncryptEx(const_cast<char*>(mdbSplit2[0]),m_pDDLSqlStruct->pDsn->sOraclePWD);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sDataSource);
    m_pDDLSqlStruct->iSqlType[0] = TK_CONNECT;
    m_pDDLSqlStruct->iSqlType[1] = TK_CONNECT;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildUseDsn(int iIFE,Token *pDsnName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sDsnName =  QMDB_MALLOC->NameFromToken(pDsnName);
    CHECK_OBJ_FILL(sDsnName);
    do{
        m_pDDLSqlStruct->bIfNE = (iIFE != 0?false:true);//if exist
        SAFESTRCPY(m_pDDLSqlStruct->sDsnName,sizeof(m_pDDLSqlStruct->sDsnName ),sDsnName);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sDsnName);
    m_pDDLSqlStruct->iSqlType[0] = TK_USE;
    m_pDDLSqlStruct->iSqlType[1] = TK_DATABASE;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildUseTablespace(int iIFE,Token *pTablespaceName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sTablespaceName =  QMDB_MALLOC->NameFromToken(pTablespaceName);
    CHECK_OBJ_FILL(sTablespaceName);
    do{
        m_pDDLSqlStruct->bIfNE = (iIFE != 0?false:true);//if exist
        if(m_pMdbConfig != NULL)
        {
            TMdbTableSpace *pTableSpace = m_pMdbConfig->GetTableSpaceByName(sTablespaceName);
            if(pTableSpace== NULL)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"tablespace[%s] does not exist.",sTablespaceName);
            }
        }
        SAFESTRCPY(m_pDDLSqlStruct->sTableSpaceName,sizeof(m_pDDLSqlStruct->sTableSpaceName),sTablespaceName);
    }while(0);
    QMDB_MALLOC->ReleaseStr(sTablespaceName);
    m_pDDLSqlStruct->iSqlType[0] = TK_USE;
    m_pDDLSqlStruct->iSqlType[1] = TK_TABLESPACE;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::AddDsnAttribute(Token *pPropertyName,Token *pValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pDsn);
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pDsn);
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pProAttr);
    TMdbCfgDSN * pDsn =  m_pDDLSqlStruct->pDsn;
    TMdbCfgProAttr *pProAttr = m_pDDLSqlStruct->pProAttr;
    char * sAttrName  = QMDB_MALLOC->NameFromToken(pPropertyName);
    char * sAttrValue = QMDB_MALLOC->NameFromToken(pValue);
    do{
        int i = 0;
        char sTmpAttrName[MAX_NAME_LEN] = {0};
        for(i=0;i<MAX_SQL_COUNTS;i++)
        {
            if(m_pDDLSqlStruct->m_tAttr[i].sAttrName[0] == 0)
            {
                SAFESTRCPY(m_pDDLSqlStruct->m_tAttr[i].sAttrValue,
                    sizeof(m_pDDLSqlStruct->m_tAttr[i].sAttrValue),sAttrValue);
                break;
            }
        }
        //TMdbNtcStrFunc::ToLower(sAttrName,sTmpAttrName);
        strcpy(sTmpAttrName, TMdbNtcStrFunc::ToLower(sAttrName));
        std::map<string,string>::iterator itor = m_mapDsnAttrName.find(sTmpAttrName);
        if (itor != m_mapDsnAttrName.end()) 
        { 
            SAFESTRCPY(m_pDDLSqlStruct->m_tAttr[i].sAttrName,
                sizeof(m_pDDLSqlStruct->m_tAttr[i].sAttrName),(itor->second).c_str());
        } 
        else
        { 
            CHECK_RET_FILL_BREAK(-1,-1,"DSN attribute[%s] does not exist.",sAttrName);
        }
        SetDsnLinkAttribute(pDsn,sAttrName,sAttrValue);
        SetDsnRepAttribute(pDsn,sAttrName,sAttrValue);
        SetDsnOtherAttribute(pDsn,sAttrName,sAttrValue);
        SetCfgProAttribute(pProAttr,sAttrName,sAttrValue);
    }while(0);
    QMDB_MALLOC->ReleaseStr(sAttrName);
    QMDB_MALLOC->ReleaseStr(sAttrValue);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  AddTableAttribute
* ��������	:  ��ӱ�����
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::AddTableAttribute(Token *pAttrName, Token *pAttrValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    TMdbTable * pNewTable =  m_pDDLSqlStruct->pTable;
    CHECK_OBJ_FILL(pNewTable);
    char * sAttrName  = QMDB_MALLOC->NameFromToken(pAttrName);
    char * sAttrValue = QMDB_MALLOC->NameFromToken(pAttrValue);
    do
    {
        TMdbNtcStrFunc::TrimLeft(sAttrValue,'[');
        TMdbNtcStrFunc::TrimRight(sAttrValue,']');
        TMdbNtcStrFunc::Trim(sAttrValue,'"');
        TADD_DETAIL("sAttrName = %s,sAttrValue = %s",sAttrName,sAttrValue);
        if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"table_space") == 0)
        {
            SAFESTRCPY(pNewTable->m_sTableSpace, sizeof(pNewTable->m_sTableSpace), sAttrValue);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"table-space");
        }
		else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"table_id") == 0)
		{
			 pNewTable->m_iTableId= TMdbNtcStrFunc::StrToInt(sAttrValue);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"table-id");

		}
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"record_counts") == 0)
        {
            pNewTable->iRecordCounts = TMdbNtcStrFunc::StrToInt(sAttrValue);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"record-counts");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"expand_record") == 0)
        {
            pNewTable->iExpandRecords = TMdbNtcStrFunc::StrToInt(sAttrValue);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"expand-record");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"shard_backup") == 0)
        {
            pNewTable->m_bShardBack = (0 == TMdbNtcStrFunc::StrNoCaseCmp(sAttrValue, "Y") ?true:false);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"shard-backup");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"table_level") == 0)
        {
            pNewTable->iTableLevel =pNewTable->GetInnerTableLevel(sAttrValue);;
            if(pNewTable->iTableLevel < 0)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"table level[%s] error, should by(MINI or SMALL or LARGE or HUGE)",sAttrValue);
            }
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"table-level");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_read_lock") == 0)
        {
            CONVERT_Y_N(sAttrValue,pNewTable->bReadLock);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"is-read-lock");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_write_lock") == 0)
        {
            CONVERT_Y_N(sAttrValue,pNewTable->bWriteLock);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"is-write-lock");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_rollback") == 0)
        {
            CONVERT_Y_N(sAttrValue,pNewTable->bRollBack);		
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"is-rollback");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_zip_time") == 0)
        {
            pNewTable->m_cZipTimeType = sAttrValue[0];
            //CONVERT_Y_N(sAttrValue,pNewTable->m_bIsZipTime);	
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"Is-Zip-Time");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"checkPrikey") == 0)
        {
            CONVERT_Y_N(sAttrValue,pNewTable->bIsCheckPriKey);	
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"checkPrikey");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"loadtype") == 0)
        {
            pNewTable->iLoadType = TMdbNtcStrFunc::StrToInt(sAttrValue);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"LoadType");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"filter_sql") == 0)
        {
            SAFESTRCPY(pNewTable->m_sFilterSQL,sizeof(pNewTable->m_sFilterSQL),sAttrValue);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"filter-sql");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"load_sql") == 0)
        {
            SAFESTRCPY(pNewTable->m_sLoadSQL,sizeof(pNewTable->m_sLoadSQL),sAttrValue);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"load-sql");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"flush_sql") == 0)
        {
            SAFESTRCPY(pNewTable->m_sFlushSQL,sizeof(pNewTable->m_sFlushSQL),sAttrValue);
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"flush-sql");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"rep_type") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"rep-type");
            pNewTable->iRepAttr = m_pMdbConfig->GetRepType(sAttrValue);
			if(pNewTable->iRepAttr ==  REP_TO_DB_MDB)
			{
				pNewTable->iRepAttr = REP_TO_DB;
				pNewTable->m_bShardBack = true;;
			}
            if(pNewTable->iRepAttr < 0)
            {
                CHECK_RET_FILL_BREAK(-1,ERR_TAB_ATTR_NOT_EXIST,"table rep_type[%s] error",sAttrValue);
            }
        }
        else
        {
            CHECK_RET_FILL_BREAK(-1,ERR_TAB_ATTR_NOT_EXIST,"no table attrname[%s]",sAttrName);
        }

        SAFESTRCPY(m_pDDLSqlStruct->sTableAttrValue,sizeof(m_pDDLSqlStruct->sTableAttrValue),sAttrValue);     
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sAttrName);
    QMDB_MALLOC->ReleaseStr(sAttrValue);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  AddTablePrimayKey
* ��������	:  ��ӱ��������Ϣ
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::AddTablePrimayKey(ST_ID_LIST * pIdList)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pIdList);
    TMdbTable * pNewTable = m_pDDLSqlStruct->pTable;
    int i = 0;
    int iPos = 0;
    do{
        for(i = 0;i<pIdList->iIdNum;i++)
        {
            iPos = m_tSqlParserHelper.GetMdbColumnPos(m_pDDLSqlStruct->pTable,pIdList->pstItem[i].zName);
            if(iPos < 0)
            {
                CHECK_RET_FILL_BREAK(ERR_TAB_COLUMN_NOT_EXIST,ERR_TAB_COLUMN_NOT_EXIST,\
                    "not find column[%s].",pIdList->pstItem[i].zName);
            }
            pNewTable->m_tPriKey.iColumnNo[i] = iPos;
        }
        SAFESTRCPY(pNewTable->m_tPriKey.m_sTableName, sizeof(pNewTable->m_tPriKey.m_sTableName), pNewTable->sTableName);
        pNewTable->m_tPriKey.iColumnCounts = pIdList->iIdNum;
        CHECK_RET_FILL_BREAK(m_tSqlParserHelper.CheckPrimaryKeyColumn(pNewTable),\
            ERR_TAB_PK_COLUMN_POS_INVALID,"Primary key check failed.");
    }while(0);
    QMDB_MALLOC->ReleaseIdList(pIdList);
    TADD_FUNC("Finish.PK.counts=[%d].",pNewTable->m_tPriKey.iColumnCounts);
    return iRet;
}


/******************************************************************************
* ��������	:  BuildCreateIndex
* ��������	:  ������������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::BuildCreateIndex(int iIFNE,Token * pIndexName,Token *pTableName,ST_ID_LIST * pIdList, Token* psAlgoName, Token* pLayerLimit)
{
    TADD_FUNC("Start");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pIndexName);
    CHECK_OBJ_FILL(pTableName);
    CHECK_OBJ_FILL(pIdList);
    // CHECK_OBJ_FILL(psAlgoName);
	char * sAlgoName = NULL;
	if(psAlgoName != NULL)
    	sAlgoName = QMDB_MALLOC->NameFromToken(psAlgoName);
	else
	{
		sAlgoName = new(std::nothrow) char[32];
		if(sAlgoName ==  NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY,"memory not enough,failed for new");
			return ERR_OS_NO_MEMROY;
		}
		SAFESTRCPY(sAlgoName,32,"hash");
	}
	
	char * sIndexName = QMDB_MALLOC->NameFromToken(pIndexName);
    char * sTableName = QMDB_MALLOC->NameFromToken(pTableName);
	
    char* sLayerLimit = NULL;
    TADD_DETAIL("index[%s],table[%s], Algo[%s].",sIndexName,sTableName, sAlgoName);
    m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);//if not exist
    do
    {
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
            m_pDDLSqlStruct->pTable->Clear();
        }
        if(NULL == m_pMdbConfig)
        {
            if(pIdList->iIdNum > MAX_INDEX_COLUMN_COUNTS)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"index_column counts[%d] is too many",MAX_INDEX_COLUMN_COUNTS);
            }

            SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,\
                sizeof(m_pDDLSqlStruct->pTable->sTableName),sTableName);

            SAFESTRCPY(m_pDDLSqlStruct->pTable->tIndex[m_pDDLSqlStruct->pTable->iIndexCounts].sName,\
                sizeof(m_pDDLSqlStruct->pTable->tIndex[m_pDDLSqlStruct->pTable->iIndexCounts].sName),sIndexName);

            for(int i = 0;i<pIdList->iIdNum;i++)
            {
                SAFESTRCPY(m_pDDLSqlStruct->m_tAttr[i].sAttrName,\
                    sizeof(m_pDDLSqlStruct->m_tAttr[i].sAttrName),pIdList->pstItem[i].zName);
            }
            m_pDDLSqlStruct->pTable->tIndex[m_pDDLSqlStruct->pTable->iIndexCounts].m_iAlgoType = m_pDDLSqlStruct->pTable->tIndex[m_pDDLSqlStruct->pTable->iIndexCounts].GetInnerAlgoType(sAlgoName);
            int iMaxLayer =1;
            if(NULL == pLayerLimit)
            {
                iMaxLayer = 1;
            }
            else
            {
                sLayerLimit =  QMDB_MALLOC->NameFromToken(pLayerLimit);
                CHECK_OBJ_FILL_BREAK(sLayerLimit);
                iMaxLayer = TMdbNtcStrFunc::StrToInt(sLayerLimit);
                if(iMaxLayer< 1 || iMaxLayer > MAX_INDEX_LAYER_LIMIT)
                {
                    CHECK_RET_FILL_BREAK(ERR_TAB_ADD_INDEX_FAILED,ERR_TAB_ADD_INDEX_FAILED,\
                        "Index Layer Limit=[%d], should be in [1,4].",iMaxLayer);
                }
            }
            m_pDDLSqlStruct->pTable->tIndex[m_pDDLSqlStruct->pTable->iIndexCounts].iMaxLayer =iMaxLayer;
            m_pDDLSqlStruct->pTable->iIndexCounts ++;
            break;
        }   

        TMdbTable * pExistTable = m_pMdbConfig->GetTable(sTableName);
        if(NULL == pExistTable)
        {
            CHECK_RET_FILL_BREAK(ERR_TAB_NO_TABLE,ERR_TAB_NO_TABLE,"not find table[%s].",sTableName);
        }
        if(pExistTable->iIndexCounts == MAX_INDEX_COUNTS)
        {//��������
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"index counts[%d] is full",MAX_INDEX_COUNTS);
        }
        int i = 0;
        for(i = 0;i< pExistTable->iIndexCounts;i++)
        {
            //��������Ƿ��Ѵ���,ע:û����(�ֶ���ͬ������ͬ)�ļ��
            if(TMdbNtcStrFunc::StrNoCaseCmp(pExistTable->tIndex[i].sName,sIndexName)== 0)
            {
                //�Ѵ���
                iRet = ERR_SQL_INVALID;
                break;
            }
        }
        CHECK_RET_FILL_BREAK(iRet,ERR_SQL_INVALID,"index[%s] exist.",sIndexName);
        m_pDDLSqlStruct->pTable->Init(pExistTable);
        TMdbIndex & tNewIndex = m_pDDLSqlStruct->pTable->tIndex[m_pDDLSqlStruct->pTable->iIndexCounts];
        tNewIndex.Clear();
        int iPos = 0;
        for(int i = 0;i < pIdList->iIdNum;i++)
        {
            iPos = m_tSqlParserHelper.GetMdbColumnPos(m_pDDLSqlStruct->pTable,pIdList->pstItem[i].zName);
            if(iPos < 0)
            {
                iRet = ERR_TAB_COLUMN_NOT_EXIST;
                break;
            }
            tNewIndex.iColumnNo[i] = iPos;
        }
        CHECK_RET_FILL_BREAK(iRet,ERR_TAB_COLUMN_NOT_EXIST,"not find column[%s].",pIdList->pstItem[i].zName);
        SAFESTRCPY(tNewIndex.sName,sizeof(tNewIndex.sName),sIndexName);
        int iCPos = tNewIndex.iColumnNo[0];
        //��������hash����
        if(pIdList->iIdNum > 1)
        {
            tNewIndex.m_iIndexType = HT_CMP;
        }
        else if(m_pDDLSqlStruct->pTable->tColumn[iCPos].iDataType == DT_Int)
        {
            tNewIndex.m_iIndexType = HT_Int;
        }
        else
        {
            tNewIndex.m_iIndexType = HT_Char;
        }
        //�������ȼ�Ĭ��Ϊ1
        tNewIndex.iPriority = 1;
        tNewIndex.m_iAlgoType = tNewIndex.GetInnerAlgoType(sAlgoName);

        if(NULL == pLayerLimit)
        {
            tNewIndex.iMaxLayer = 1;
        }
        else
        {
            sLayerLimit =  QMDB_MALLOC->NameFromToken(pLayerLimit);
            CHECK_OBJ_FILL_BREAK(sLayerLimit);
            tNewIndex.iMaxLayer = TMdbNtcStrFunc::StrToInt(sLayerLimit);
            if(tNewIndex.iMaxLayer< 1 || tNewIndex.iMaxLayer > MAX_INDEX_LAYER_LIMIT)
            {
                CHECK_RET_FILL_BREAK(ERR_TAB_ADD_INDEX_FAILED,ERR_TAB_ADD_INDEX_FAILED,\
                    "Index Layer Limit=[%d], should be in [1,4].",tNewIndex.iMaxLayer);
            }
        }

        m_pDDLSqlStruct->pTable->iIndexCounts ++;
        if(m_pDDLSqlStruct->pTable->iIndexCounts >= MAX_INDEX_COUNTS)
        {
            CHECK_RET_FILL_BREAK(ERR_TAB_INDEX_NUM_EXCEED_MAX,ERR_TAB_INDEX_NUM_EXCEED_MAX,\
                "Table=[%s] too many Index,Max=[%d].",m_pDDLSqlStruct->pTable->sTableName, MAX_INDEX_COUNTS);
        }
    }while(0);

    QMDB_MALLOC->ReleaseStr(sIndexName);
    QMDB_MALLOC->ReleaseStr(sTableName);
    QMDB_MALLOC->ReleaseIdList(pIdList);
    QMDB_MALLOC->ReleaseStr(sAlgoName);
    m_pDDLSqlStruct->iSqlType[0] = TK_CREATE;
    m_pDDLSqlStruct->iSqlType[1] = TK_INDEX;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildCreatePrimKey(int iIFNE,Token *pTableName,ST_ID_LIST * pIdList)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pTableName);
    CHECK_OBJ_FILL(pIdList);
    char * sTableName = QMDB_MALLOC->NameFromToken(pTableName);
    TADD_DETAIL("primary table[%s].",sTableName);
    m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);//if not exist
    do
    {
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        }
        CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);

        //У�������ϵ��������ܳ���10��
        if(pIdList->iIdNum > MAX_PRIMARY_KEY_CC)
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                "PrimaryKey_column counts[%d] is too many",MAX_PRIMARY_KEY_CC);
        }

        if(NULL == m_pMdbConfig)
        {   
            SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,\
                sizeof(m_pDDLSqlStruct->pTable->sTableName),sTableName);
            for(int i = 0;i<pIdList->iIdNum;i++)
            {
                SAFESTRCPY(m_pDDLSqlStruct->m_tAttr[i].sAttrName,\
                    sizeof(m_pDDLSqlStruct->m_tAttr[i].sAttrName),pIdList->pstItem[i].zName);
            }
        }
    }while(0);
    QMDB_MALLOC->ReleaseStr(sTableName);
    QMDB_MALLOC->ReleaseIdList(pIdList);
    m_pDDLSqlStruct->iSqlType[0] = TK_ADD;
    m_pDDLSqlStruct->iSqlType[1] = TK_PRIMARY;
    TADD_FUNC("Finish.PK.counts=[%d].",m_pDDLSqlStruct->pTable->m_tPriKey.iColumnCounts);
    return iRet;
}

int TMdbSqlParser::BuildDropIndex(int iIFE,Token * pIndexName,Token * pTableName)
{
    TADD_FUNC("Start");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pIndexName);
    CHECK_OBJ_FILL(pTableName);
    char * sIndexName = QMDB_MALLOC->NameFromToken(pIndexName);
    char * sTableName = QMDB_MALLOC->NameFromToken(pTableName);
    do
    {
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable(); 
        }
        CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
        if(NULL != m_pMdbConfig)
        {
            TMdbTable * pExistTable = m_pMdbConfig->GetTable(sTableName);
            if(NULL == pExistTable)
            {
                CHECK_RET_FILL_BREAK(ERR_TAB_NO_TABLE,ERR_TAB_NO_TABLE,"Table [%s] does not exist.",sTableName);
            }
            m_pDDLSqlStruct->bIfNE = (iIFE != 0?false:true);//if exist
            int indxpos = -1;
            for(int i = 0;i< pExistTable->iIndexCounts;i++)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pExistTable->tIndex[i].sName,sIndexName)== 0)
                {
                    indxpos = i;
                    break;
                }
            }
            if(indxpos <0)
            {
                CHECK_RET_FILL_BREAK(ERR_TAB_INDEX_NOT_EXIST,ERR_TAB_INDEX_NOT_EXIST,"Index [%s] does not exist.",sIndexName);
            }
            TMdbIndex & tDelIndex = pExistTable->tIndex[indxpos];
            tDelIndex.Clear();
            for(;indxpos < pExistTable->iIndexCounts-1;indxpos++)
            {
                memcpy(&(pExistTable->tIndex[indxpos]),\
                    &(pExistTable->tIndex[indxpos+1]),sizeof(TMdbIndex));
                pExistTable->tIndex[indxpos+1].Clear();
            }
            memcpy(m_pDDLSqlStruct->pTable,pExistTable,sizeof(TMdbTable));
            pExistTable->iIndexCounts --;
        }   

        SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,\
            sizeof(m_pDDLSqlStruct->pTable->sTableName),sTableName);
        SAFESTRCPY(m_pDDLSqlStruct->sIndexName,sizeof(m_pDDLSqlStruct->sIndexName),sIndexName);
        m_pDDLSqlStruct->pTable->iIndexCounts --;
    }while(0);
    QMDB_MALLOC->ReleaseStr(sIndexName);
    QMDB_MALLOC->ReleaseStr(sTableName);
    m_pDDLSqlStruct->iSqlType[0] = TK_DROP;
    m_pDDLSqlStruct->iSqlType[1] = TK_INDEX;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildModifyTableAttribute(int iIFNE,Token * pTableName,const char*pOptName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pTableName);
    CHECK_OBJ_FILL(pOptName);
    char * sName =  QMDB_MALLOC->NameFromToken(pTableName);
    do{
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        }
        CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
        m_pDDLSqlStruct->pTable->Clear();
        if(NULL != m_pMdbConfig)
        {
            TMdbTable *pTable = m_pMdbConfig->GetTable(sName);
            if(NULL == pTable)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"table[%s] does not exist.",sName);
            }
            m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);
            memcpy(m_pDDLSqlStruct->pTable,pTable,sizeof(TMdbTable));
        }
        SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,\
            sizeof(m_pDDLSqlStruct->pTable->sTableName),sName);    
        if(TMdbNtcStrFunc::StrNoCaseCmp(pOptName,"add") == 0)
        {
            m_pDDLSqlStruct->iSqlType[0] = TK_ADD;
            m_pDDLSqlStruct->iSqlType[1] = TK_TABLESYS;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pOptName,"drop") == 0)
        {
            m_pDDLSqlStruct->iSqlType[0] = TK_DROP;
            m_pDDLSqlStruct->iSqlType[1] = TK_TABLESYS;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pOptName,"modify") == 0)
        {
            m_pDDLSqlStruct->iSqlType[0] = TK_MODIFY;
            m_pDDLSqlStruct->iSqlType[1] = TK_TABLESYS;
        }
        else
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"Table[%s] attribute[%s] operation is error.",sName,pOptName);
        }
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sName);
    TADD_FUNC("Finish.");
    return iRet;

}

int TMdbSqlParser::DropTableAttribute(Token * pAttrName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    TMdbTable * pNewTable =  m_pDDLSqlStruct->pTable;
    CHECK_OBJ_FILL(pNewTable);
    char * sAttrName  = QMDB_MALLOC->NameFromToken(pAttrName);
    do{
        /*if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"table_id") == 0)
        {
        SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"table-id");
        }
        else */
        if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"table_space") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"table-space");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"shard-backup") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"shard-backup");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"table-level") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"table-level");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"record_counts") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"record-counts");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"expand_record") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"expand-record");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_read_lock") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"is-read-lock");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_write_lock") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"is-write-lock");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_rollback") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"is-rollback");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_zip_time") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"Is-Zip-Time");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"checkPrikey") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"checkPrikey");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"LoadType") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"LoadType");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"filter_sql") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"filter-sql");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"load_sql") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"load-sql");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"flush_sql") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),sAttrName);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"rep_type") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"rep-type");
        }
        else
        {
            CHECK_RET_FILL_BREAK(-1,ERR_TAB_ATTR_NOT_EXIST,"no table attrname[%s]",sAttrName);
        }
    }while(0);
    QMDB_MALLOC->ReleaseStr(sAttrName);
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::ModifyTableAttributeFromSql(char * sAttrName,char * sAttrValue)
{

	int iRet = 0;
	
        if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"table_id") == 0)
        {
        	SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"table-id");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"table_space") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"table_space");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"shard-backup") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"shard-backup");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"table-level") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"table-level");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"record_counts") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"record-counts");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"expand_record") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"expand-record");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_read_lock") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"is-read-lock");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_write_lock") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"is-write-lock");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_rollback") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"is-rollback");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_zip_time") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"Is-Zip-Time");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"checkPriKey") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"checkPriKey");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"loadtype") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"LoadType");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"filter_sql") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"filter-sql");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"load_sql") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"load-sql");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"flush_sql") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"flush-sql");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"rep_type") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"rep-type");
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"shard_backup") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->sTableAttr,sizeof(m_pDDLSqlStruct->sTableAttr),"shard-backup");
        }
        
        else
        {
           //CHECK_RET_FILL_BREAK(-1,ERR_TAB_ATTR_NOT_EXIST,"no table attrname[%s]",sAttrName);
           iRet = -1;
		   TADD_ERROR(ERR_TAB_ATTR_NOT_EXIST,"no table attrname[%s]",sAttrName);
		   m_tError.FillErrMsg(ERR_TAB_ATTR_NOT_EXIST,"no table attrname[%s]",sAttrName);
        }
		
        SAFESTRCPY(m_pDDLSqlStruct->sTableAttrValue,sizeof(m_pDDLSqlStruct->sTableAttrValue),sAttrValue);
    
	
	return iRet;
}
int TMdbSqlParser::ModifyTableAttribute(Token *pAttrName, Token *pAttrValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    TMdbTable * pNewTable =  m_pDDLSqlStruct->pTable;
    CHECK_OBJ_FILL(pNewTable);
    char * sAttrName   = QMDB_MALLOC->NameFromToken(pAttrName);
    char * sAttrValue  = QMDB_MALLOC->NameFromToken(pAttrValue);
    TADD_DETAIL("pAttrName = %s,pAttrValue=%s",sAttrName,sAttrValue);

	iRet = ModifyTableAttributeFromSql(sAttrName,sAttrValue);
    QMDB_MALLOC->ReleaseStr(sAttrName);
    QMDB_MALLOC->ReleaseStr(sAttrValue);
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildAddTableColumn(int iIFNE,Token * pTableName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    char * sName =  QMDB_MALLOC->NameFromToken(pTableName);
    do{
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        }      
        CHECK_OBJ_FILL(m_pDDLSqlStruct->pTable);
        m_pDDLSqlStruct->pTable->Clear();
        if(NULL != m_pMdbConfig)
        {
            TMdbTable * pTable = m_pMdbConfig->GetTable(sName);
            if(NULL == pTable)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"table[%s] does not exist.",sName);
            }
            m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);//if exist
            if(pTable->iColumnCounts >= MAX_COLUMN_COUNTS)
            {
                CHECK_RET_FILL(ERR_TAB_COLUMN_NUM_EXCEED_MAX,ERR_TAB_COLUMN_NUM_EXCEED_MAX,
                    "table[%s] already reach max column count[%d].",sName,pTable->iColumnCounts);
            }

            memcpy(m_pDDLSqlStruct->pTable,pTable,sizeof(TMdbTable));
        }
        SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,sizeof(m_pDDLSqlStruct->pTable->sTableName),sName);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sName);
    m_pDDLSqlStruct->iSqlType[0] = TK_ADD;
    m_pDDLSqlStruct->iSqlType[1] = TK_COLUMN;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::BuildDropTableColumn(int iIFNE,Token * pTableName,Token * pColumnName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    int iFindPos = -1;
    CHECK_OBJ_FILL(pTableName);
    CHECK_OBJ_FILL(pColumnName);
    char * sTableName = QMDB_MALLOC->NameFromToken(pTableName);
    char * sColumnName = QMDB_MALLOC->NameFromToken(pColumnName);
    do{
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
            m_pDDLSqlStruct->pTable->Clear();
        }  
        //ʹ��sModifyColumnAttr��ʱ����ɾ��������
        m_pDDLSqlStruct->vModifyColumnAttr.push_back(sColumnName);
        if(NULL == m_pMdbConfig)
        {
            SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,\
                sizeof(m_pDDLSqlStruct->pTable->sTableName),sTableName);
            break;
        }
        //��̬�²�֧��ɾ���в���
        CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"Does not support dynamic drop column.");
        TMdbTable * pTable = m_pMdbConfig->GetTable(sTableName);
        if(NULL == pTable)
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"table[%s] does not exist.",sTableName);
        }
        memcpy(m_pDDLSqlStruct->pTable,pTable,sizeof(TMdbTable));
        int iColumnIndex  = -1;
        for(int i=0;i<m_pDDLSqlStruct->pTable->iColumnCounts;i++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_pDDLSqlStruct->pTable->tColumn[i].sName,sColumnName) == 0)
            {
                iFindPos = m_pDDLSqlStruct->pTable->tColumn[i].iPos;
                iColumnIndex = i;
                break;
            }
        }

        if(iFindPos < 0)
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                "The column[%s] of the table[%s] does not exist.",sColumnName,sTableName);
        }
        m_pDDLSqlStruct->pTable->tColumn[iColumnIndex].Clear();

        for(int i=0;i<m_pDDLSqlStruct->pTable->iColumnCounts;i++)
        {
            if(m_pDDLSqlStruct->pTable->tColumn[i].iPos > iFindPos)
            {
                m_pDDLSqlStruct->pTable->tColumn[i].iPos -= 1;
            }
        } 

        for(;iColumnIndex < m_pDDLSqlStruct->pTable->iColumnCounts-1;iColumnIndex++)
        {
            memcpy(&(m_pDDLSqlStruct->pTable->tColumn[iColumnIndex]), \
                &(m_pDDLSqlStruct->pTable->tColumn[iColumnIndex+1]),sizeof(TMdbColumn)); 
            m_pDDLSqlStruct->pTable->tColumn[iColumnIndex+1].Clear();
        }
        m_pDDLSqlStruct->pTable->iColumnCounts --;
    }while(0);
    QMDB_MALLOC->ReleaseStr(sTableName);
    QMDB_MALLOC->ReleaseStr(sColumnName);
    m_pDDLSqlStruct->iSqlType[0] = TK_DROP;
    m_pDDLSqlStruct->iSqlType[1] = TK_COLUMN;
    TADD_FUNC("Finish.");
    return iRet;    
}

int TMdbSqlParser::BuildModifyTableColumn(int iIFNE,Token * pTableName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pTableName);
    char * sName =  QMDB_MALLOC->NameFromToken(pTableName);
    do{
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        } 
        CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
        if(NULL != m_pMdbConfig)
        {
            TMdbTable * pTable = m_pMdbConfig->GetTable(sName);
            if(NULL == pTable)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"table[%s] does not exist.",sName);
            }
            m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);//if exist
            memcpy(m_pDDLSqlStruct->pTable,pTable,sizeof(TMdbTable));
        }
        SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,\
            sizeof(m_pDDLSqlStruct->pTable->sTableName),sName);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sName);
    m_pDDLSqlStruct->iSqlType[0] = TK_ALTER;
    m_pDDLSqlStruct->iSqlType[1] = TK_COLUMN;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::ModifyColumn(Token * pColumnName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iFindPos = -1;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pColumnName);
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pTable);
    char * sColumnName = QMDB_MALLOC->NameFromToken(pColumnName);
    do{
        //��̬��ִ�У�Ĭ���޸ĵ��ǵ�һ��
        if(NULL == m_pMdbConfig)
        {
            m_pDDLSqlStruct->iModifyColumnPos = 0;
            SAFESTRCPY(m_pDDLSqlStruct->pTable->tColumn[0].sName, \
                sizeof(m_pDDLSqlStruct->pTable->tColumn[0].sName),sColumnName);
            break;
        }

        //�����Ƕ�ִ̬��
        int iColumnCounts  = m_pDDLSqlStruct->pTable->iColumnCounts;
        for(int i=0;i<iColumnCounts;i++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_pDDLSqlStruct->pTable->tColumn[i].sName,sColumnName) == 0)
            {
                iFindPos = i;
                break;
            }
        }

        if(iFindPos < 0)
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                "The column[%s] of the table[%s] does not exist.",sColumnName,m_pDDLSqlStruct->pTable->sTableName);
        }
        m_pDDLSqlStruct->iModifyColumnPos = iFindPos;
    }while(0);
    QMDB_MALLOC->ReleaseStr(sColumnName);
    m_pDDLSqlStruct->iSqlType[0] = TK_MODIFY;
    m_pDDLSqlStruct->iSqlType[1] = TK_COLUMN;
    TADD_FUNC("Finish.");
    return iRet; 
}

int TMdbSqlParser::ModifyColumnAttribute(const char * sAttrName, Token *pToken)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(sAttrName);
    CHECK_OBJ_FILL(pToken);
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pTable);
    char * sAttr = QMDB_MALLOC->NameFromToken(pToken);
    TMdbColumn & tColumn = m_pDDLSqlStruct->pTable->tColumn[m_pDDLSqlStruct->iModifyColumnPos];
    do
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"data-len") == 0)
        {
            if((tColumn.iDataType == DT_Char
                || tColumn.iDataType == DT_VarChar
                || tColumn.iDataType == DT_Blob)
                && TMdbNtcStrFunc::StrToInt(sAttr) < tColumn.iColumnLen-1)
            {
                CHECK_RET_FILL_BREAK(-1,ERR_SQL_DATA_LEN_ERROR,\
                    "Modifying the length of the column[%d] is less than the original length[%d]",\
                    TMdbNtcStrFunc::StrToInt(sAttr),tColumn.iColumnLen-1);
            }
            tColumn.iColumnLen = TMdbNtcStrFunc::StrToInt(sAttr);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"data-type") == 0)
        {
            int iDataType = m_pMdbConfig->GetDataType(sAttr);
            if(iDataType <= 0)
            {
                CHECK_RET_FILL_BREAK(-1,ERR_SQL_DATA_TYPE_INVALID,\
                    "The column[%s] data type[%s] error.",tColumn.sName,sAttr);
            }

            if(NULL == m_pMdbConfig) 
            {//����
                tColumn.iDataType = iDataType;
            }
            else if(iDataType != tColumn.iDataType)
            {//��ͬ������
                TMdbTable* pMdbTable = m_tSqlParserHelper.GetMdbTable(m_pDDLSqlStruct->pTable->sTableName);
                if(!pMdbTable)
                {
                    CHECK_RET_FILL_BREAK(-1,ERR_TAB_NO_TABLE,"not find table [%s]",m_pDDLSqlStruct->pTable->sTableName);
                }
                if(pMdbTable->iCounts <= 0)
                {//û�����ݵ�ʱ��
                    tColumn.iDataType = iDataType;
                }
                else if(tColumn.iDataType == DT_Int || iDataType == DT_Int)
                {//�����ݵ�ʱ�����β������������ͻ���ת��
                    CHECK_RET_FILL_BREAK(-1,ERR_SQL_DATA_TYPE_INVALID,\
                        "Do not modify column[%s] data type[%s] dynamicly with records.",
                        tColumn.sName,sAttr);
                }
                else
                {//��Ϊ���ο����޸�
                    tColumn.iDataType = iDataType;
                }
            }

        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"default-value") == 0)
        {	
            tColumn.bIsDefault = true;
            SAFESTRCPY(tColumn.iDefaultValue,sizeof(tColumn.iDefaultValue),sAttr);
        }
		else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"rep-type") == 0)
		{
		}
        else
        {
            CHECK_RET_FILL_BREAK(-1,ERR_SQL_NO_COLUMN_ATTR,"no column attrname[%s]",sAttrName);
        }
        m_pDDLSqlStruct->vModifyColumnAttr.push_back(sAttrName);
    }while(0);
    QMDB_MALLOC->ReleaseStr(sAttr);
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::ModifyColumnNULLAttribute(const int iNullAble)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(m_pDDLSqlStruct->pTable);
    m_pDDLSqlStruct->vModifyColumnAttr.push_back("Null-able");
    TMdbColumn & tColumn = m_pDDLSqlStruct->pTable->tColumn[m_pDDLSqlStruct->iModifyColumnPos];
    if(iNullAble == 0)
    {
        tColumn.m_bNullable = false; 
    }
    else
    {
        tColumn.m_bNullable = true;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  BuildLoadData
* ��������	:  ����ĳ�ֱ������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::BuildLoadData(Token * pTableName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(m_pMdbConfig);
    char * sName =  QMDB_MALLOC->NameFromToken(pTableName);
    do{
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
        }

        TMdbTable *pTable = m_pMdbConfig->GetTable(sName);
        if(pTable == NULL)
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"table[%s] does not exist.",sName);
        }
        SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,\
            sizeof(m_pDDLSqlStruct->pTable->sTableName),sName);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sName);
    m_pDDLSqlStruct->iSqlType[0] = TK_LOAD;
    m_pDDLSqlStruct->iSqlType[1] = TK_DATA;
    TADD_FUNC("Finish.");
    return iRet;

}

/******************************************************************************
* ��������	:  CollectIndexForVerifyPK
* ��������	:  ��ȡ���ڼ��������������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::CollectIndexForVerifyPK(ST_ID_LIST * pIdList)
{
    TADD_FUNC("Start.pIdList=[%p]",pIdList);
    int iRet = 0;
    CHECK_OBJ_RET_0(pIdList);
    m_stSqlStruct.stIndexForVerifyPK.pstTableIndex  = m_MdbIndexCtrl.GetVerfiyPKIndex();
    if(NULL == m_stSqlStruct.stIndexForVerifyPK.pstTableIndex)
    {//������û������
        CHECK_RET_FILL(ERR_APP_CONFIG_ITEM_VALUE_INVALID,ERR_APP_CONFIG_ITEM_VALUE_INVALID,
            "Table[%s] want to check pk conflict,but no index in PK.",m_stSqlStruct.pMdbTable->sTableName);
    }
    CHECK_OBJ_FILL(m_stSqlStruct.stIndexForVerifyPK.pstTableIndex);
    TMdbIndex *pMdbIndex = m_stSqlStruct.stIndexForVerifyPK.pstTableIndex->pIndexInfo;
    int i = 0;
    for(i = 0;i<pIdList->iIdNum;i++)
    {
        ST_MEM_VALUE * pstMemValue = pIdList->pstItem[i].pExpr->pExprValue;
        int j = 0;
        for(j = 0;j < MAX_INDEX_COLUMN_COUNTS && (pMdbIndex->iColumnNo[j] >= 0);++j)
        {
            if(pstMemValue->pColumnToSet->iPos == pMdbIndex->iColumnNo[j])
            {//��ȡֵ
                m_stSqlStruct.stIndexForVerifyPK.pExprArr[j] = pIdList->pstItem[i].pExpr;
            }
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* ��������	:  IsFullIndex
* ��������	:  �Ƿ���������������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbSqlParser::IsFullIndex(std::vector< ST_INDEX_VALUE > & vIndex)
{
    TADD_FUNC("Start size=[%d].",vIndex.size());
    bool bRet = true;
    if(vIndex.size() == 0){return false;}
    std::vector< ST_INDEX_VALUE >::iterator itor = vIndex.begin();
    for(;itor != vIndex.end();++itor)
    {
        if(false == IsFullIndex(*itor))
        {
            return false;
        }
    }
    TADD_FUNC("Finish ret=[%d].",bRet);
    return bRet;
}
/******************************************************************************
* ��������	:  IsFullIndex
* ��������	:  �Ƿ�������������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbSqlParser::IsFullIndex( ST_INDEX_VALUE & stIndexValue)
{
    if(HT_CMP == stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType)
    {// �������
        int i = 0;
        for(i = 0;i<MAX_INDEX_COLUMN_COUNTS;i++)
        {
            if(stIndexValue.pstTableIndex->pIndexInfo->iColumnNo[i]>=0 &&
                NULL == stIndexValue.pExprArr[i])
            {//������
                TADD_FLOW("CMP_Index[%s] is not complete.",stIndexValue.pstTableIndex->pIndexInfo->sName);
                return false;
            }
        }
    }
    return true;
}

/******************************************************************************
* ��������	:  CheckSetList
* ��������	:  ���update��set�б�
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::CheckSetList(ST_SET_LIST & stSetList)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int i = 0;
    for(i = 0;i < stSetList.pExprList->iItemNum;++i)
    {
        ST_EXPR * pstExpr = stSetList.pExprList->pExprItems[i].pExpr;
        //set �б��в��ܰ���and ��or
        if(true == m_tMdbExpr.IsContainOpcode(pstExpr,TK_AND) ||
            true == m_tMdbExpr.IsContainOpcode(pstExpr,TK_OR) 
            )
        {
            CHECK_RET(ERR_SQL_INVALID,"set list cannot contain (and/or).");      
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* ��������	:  RemoveTableAlias
* ��������	:  ȥ�������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::RemoveTableAlias()
{
    int iRet = 0;
    int i = 0;
    char * sAlias = (NULL == m_stSqlStruct.sAlias)?m_stSqlStruct.sTableName:m_stSqlStruct.sAlias;
    if(NULL != m_stSqlStruct.pColList)
    {
        for(i = 0; i<m_stSqlStruct.pColList->iItemNum;++i)
        {
            CHECK_RET(m_tMdbExpr.RemoveTableAlias(m_stSqlStruct.pColList->pExprItems[i].pExpr,sAlias),"pColList RemoveTableAlias failed.");
        }
    }
    if(NULL != m_stSqlStruct.pstIdList)
    {
        for(i = 0;i<m_stSqlStruct.pstIdList->iIdNum;++i)
        {
            CHECK_RET(m_tMdbExpr.RemoveTableAlias(m_stSqlStruct.pstIdList->pstItem[i].pExpr,sAlias),"pstIdList RemoveTableAlias failed.");
        }
    }
    CHECK_RET(m_tMdbExpr.RemoveTableAlias(m_stSqlStruct.pWhere,sAlias),"pWhere RemoveTableAlias failed.");
    CHECK_RET(m_tMdbExpr.RemoveTableAlias(m_stSqlStruct.pstLimit,sAlias),"pstLimit RemoveTableAlias failed.");
    CHECK_RET(m_tMdbExpr.RemoveTableAlias(m_stSqlStruct.pstOffset,sAlias),"pstOffset RemoveTableAlias failed.");
    if(NULL != m_stSqlStruct.pGroupby)
    {
        for(i = 0;i<m_stSqlStruct.pGroupby->iItemNum;++i)
        {
            CHECK_RET(m_tMdbExpr.RemoveTableAlias(m_stSqlStruct.pGroupby->pExprItems[i].pExpr,sAlias),"pGroupby RemoveTableAlias failed.");
        }
    }
    if(NULL != m_stSqlStruct.pOrderby)
    {
        for(i = 0;i<m_stSqlStruct.pOrderby->iItemNum;++i)
        {
            CHECK_RET(m_tMdbExpr.RemoveTableAlias(m_stSqlStruct.pOrderby->pExprItems[i].pExpr,sAlias),"pOrderby RemoveTableAlias failed.");
        }
    }
    return iRet;
}

/******************************************************************************
* ��������	:  IsTableAlias
* ��������	:  �ж��Ƿ��Ǳ�ı���
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbSqlParser::IsTableAlias(Token * pToken)
{
    if(NULL == pToken){return false;}
    char * sAlias = (NULL == m_stSqlStruct.sAlias)?m_stSqlStruct.sTableName:m_stSqlStruct.sAlias;
    if(NULL == sAlias){return false;}
    if(TMdbNtcStrFunc::StrNoCaseCmp(sAlias,pToken->z,pToken->n) == 0 && strlen(sAlias) == pToken->n)
    {
        return true;
    }
    return false;
}

int TMdbSqlParser::SetNtcAgentPortAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue)
{
	SAFESTRCPY(pDsn->sNtcPortStr,sizeof(pDsn->sNtcPortStr),sAttrValue);
	if(pDsn->sNtcPortStr[0] == 0) 
	{
		
	}
	else
	{
		char sNtcPortStr[64] = {0};
		SAFESTRCPY(sNtcPortStr, 64, pDsn->sNtcPortStr);
		TMdbNtcSplit tSplit;
		tSplit.SplitString(sNtcPortStr, ',');
		if(tSplit.GetFieldCount() <= 0)
		{
			TADD_ERROR(ERR_NET_IP_INVALID,"Too few  use ntc agent port value!");
			return ERR_NET_IP_INVALID;
		}
		else if(tSplit.GetFieldCount() > MAX_AGENT_PORT_COUNTS)
		{
			TADD_ERROR(ERR_NET_IP_INVALID,"Too many use ntc agent port value!");
			return ERR_NET_IP_INVALID;
		}
		else
		{
			char sTempPort[16] = {0};
			for(int i = 0; i<tSplit.GetFieldCount(); i++)
			{
				memset(sTempPort, 0, sizeof(sTempPort));
				SAFESTRCPY(sTempPort, sizeof(sTempPort), tSplit[i]);
				TMdbNtcStrFunc::Trim(sTempPort, ' ');
				pDsn->iNtcPort[i] = TMdbNtcStrFunc::StrToInt(sTempPort);//����˿�
				if(pDsn->iNtcPort[i] <= 0)
				{
					TADD_ERROR(ERR_NET_IP_INVALID,"Invalid use ntc agent port value!");
					return ERR_NET_IP_INVALID;
				}
				TADD_DETAIL("m_tDsn.iNtcPort[%d] = [%d]", i, pDsn->iNtcPort[i]);
			}
		}
	}
    
	return 0;
}

int TMdbSqlParser::SetNoNtcAgentPortAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue)
{
	SAFESTRCPY(pDsn->sNoNtcPortStr,sizeof(pDsn->sNoNtcPortStr),sAttrValue);
	if(pDsn->sNoNtcPortStr[0] == 0) 
	{
		
	}
	else
	{
		char sNoNtcPortStr[64] = {0};
		SAFESTRCPY(sNoNtcPortStr, 64, pDsn->sNoNtcPortStr);
		TMdbNtcSplit tSplit;
		tSplit.SplitString(sNoNtcPortStr, ',');
		if(tSplit.GetFieldCount() <= 0)
		{
			TADD_ERROR(ERR_NET_IP_INVALID,"Too few not use agent port value!");
			return ERR_NET_IP_INVALID;
		}
		else if(tSplit.GetFieldCount() > MAX_AGENT_PORT_COUNTS)
		{
			TADD_ERROR(ERR_NET_IP_INVALID,"Too many not  use ntc agent port value!");
			return ERR_NET_IP_INVALID;
		}
		else
		{
			char sTempPort[16] = {0};
			for(int i = 0; i<tSplit.GetFieldCount(); i++)
			{
				memset(sTempPort, 0, sizeof(sTempPort));
				SAFESTRCPY(sTempPort, sizeof(sTempPort), tSplit[i]);
				TMdbNtcStrFunc::Trim(sTempPort, ' ');
				pDsn->iNoNtcPort[i] = TMdbNtcStrFunc::StrToInt(sTempPort);//����˿�
				if(pDsn->iNoNtcPort[i] <= 0)
				{
					TADD_ERROR(ERR_NET_IP_INVALID,"Invalid not use ntc agent port value!");
					return ERR_NET_IP_INVALID;
				}
				TADD_DETAIL("m_tDsn.iNoNtcPort[%d] = [%d]", i, pDsn->iNoNtcPort[i]);
			}
		}
	}

	return 0;
}

int TMdbSqlParser::SetNtcPortsAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue)
{
	int iRet = 0;
	if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"use_ntc_agent_port") == 0)
	{
		CHECK_RET(SetNtcAgentPortAttribute(pDsn,sAttrName,sAttrValue),"get use_ntc_agent_port failed");

	}
	if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"notuse_ntc_agent_port") == 0)
	{
		CHECK_RET(SetNoNtcAgentPortAttribute(pDsn,sAttrName,sAttrValue),"get not use_ntc_agent_port failed");

	}
	if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_use_ntc") == 0)
		CONVERT_Y_N(sAttrValue,pDsn->m_bUseNTC);

	return iRet;
}

int TMdbSqlParser::SetDsnLinkAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(pDsn);
    CHECK_OBJ_FILL(sAttrName);
    CHECK_OBJ_FILL(sAttrValue);
    /*if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"local_standby_ip") == 0)
    {
    SAFESTRCPY(pDsn->sPeerIP,sizeof(pDsn->sPeerIP),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"local_standby_port") == 0)
    {
    pDsn->iPeerPort = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"local_active_port") == 0)
    {
    pDsn->iLocalPort = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"local_active_ip") == 0)
    {
    SAFESTRCPY(pDsn->sLocalIP,sizeof(pDsn->sPeerIP),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"peer_active_ip") == 0)
    {
    SAFESTRCPY(pDsn->sActiveIP,sizeof(pDsn->sPeerIP),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"peer_active_port") == 0)
    {
    pDsn->iActivePort = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"peer_standby_ip") == 0)
    {
    SAFESTRCPY(pDsn->sStandByIP,sizeof(pDsn->sPeerIP),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"peer_standby_port") == 0)
    {
    pDsn->iStandbyPort = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }*/
    if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Local_Ip") == 0)
    {
        SAFESTRCPY(pDsn->sLocalIP,sizeof(pDsn->sLocalIP),sAttrValue);
    }
	if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Local_Active_Ip") == 0)
    {
        SAFESTRCPY(pDsn->sLocalIP_active,sizeof(pDsn->sLocalIP_active),sAttrValue);
    }
	
    if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Rep_Server_ip") == 0)
    {
        SAFESTRCPY(pDsn->sRepSvrIp,sizeof(pDsn->sRepSvrIp),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Rep_Server_port") == 0)
    {
        pDsn->iRepSvrPort= TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Rep_standby_Server_ip") == 0)
    {
        SAFESTRCPY(pDsn->sRepStandbySvrIp,sizeof(pDsn->sRepStandbySvrIp),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Rep_standby_Server_port") == 0)
    {
        pDsn->iRepStandbySvrPort= TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Rep_Local_port") == 0)
    {
        pDsn->iRepLocalPort= TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Rep_file_invalid_time") == 0)
    {
        pDsn->iInvalidRepFileTime = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"agent_port") == 0)
    {
        //pDsn->iAgentPort = TMdbNtcStrFunc::StrToInt(sAttrValue);
		SAFESTRCPY(pDsn->sAgentPortStr,sizeof(pDsn->sAgentPortStr),sAttrValue);
		if(pDsn->sAgentPortStr[0] == 0) 
		{
			pDsn->iAgentPort[0] = 19804;
			for(int i = 1; i<MAX_AGENT_PORT_COUNTS; i++)
			{
				pDsn->iAgentPort[i] = 0;
			}
			memset(pDsn->sAgentPortStr, 0, sizeof(pDsn->sAgentPortStr));
			SAFESTRCPY(pDsn->sAgentPortStr, sizeof(pDsn->sAgentPortStr), "19804");
		}
		else
		{
		    char sAgentPort[64] = {0};
			SAFESTRCPY(sAgentPort, 64, pDsn->sAgentPortStr);
			TMdbNtcSplit tSplit;
			tSplit.SplitString(sAgentPort, ',');
			if(tSplit.GetFieldCount() <= 0)
			{
				TADD_ERROR(ERR_NET_IP_INVALID, "Too few agent port value!");
				return ERR_NET_IP_INVALID;
			}
			else if(tSplit.GetFieldCount() > MAX_AGENT_PORT_COUNTS)
			{
				TADD_ERROR(ERR_NET_IP_INVALID, "Too many agent port value!");
				return ERR_NET_IP_INVALID;
			}
			else
			{
				char sTempPort[16] = {0};
				for(int i = 0; i<tSplit.GetFieldCount(); i++)
				{
					memset(sTempPort, 0, sizeof(sTempPort));
					SAFESTRCPY(sTempPort, sizeof(sTempPort), tSplit[i]);
					TMdbNtcStrFunc::Trim(sTempPort, ' ');
					pDsn->iAgentPort[i] = TMdbNtcStrFunc::StrToInt(sTempPort);//����˿�
					if(pDsn->iAgentPort[i] <= 0)
					{
						TADD_ERROR(ERR_NET_IP_INVALID, "Invalid agent port value!");
						return ERR_NET_IP_INVALID;
					}
					TADD_DETAIL("m_tDsn.iAgentPort[%d] = [%d]", i, pDsn->iAgentPort[i]);
				}
			}
		}
    }
	CHECK_RET(SetNtcPortsAttribute(pDsn,sAttrName,sAttrValue),"SetNtcPortsAttribute failed");
	
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::SetDsnRepAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(pDsn);
    CHECK_OBJ_FILL(sAttrName);
    CHECK_OBJ_FILL(sAttrValue);
    if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_ora_rep") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->bIsOraRep);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_rep") == 0)
    {
    	CONVERT_Y_N(sAttrValue,pDsn->bIsRep);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_mem_load") == 0)
    {
    CONVERT_Y_N(sAttrValue,pDsn->bIsMemLoad);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_peer_rep") == 0)
    {
    	CONVERT_Y_N(sAttrValue,pDsn->bIsPeerRep);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_online_rep") == 0)
    {
    	CONVERT_Y_N(sAttrValue,pDsn->bIsOnlineRep);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_ReadOnlyAttr") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->bIsReadOnlyAttr);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_capture_router") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->bIsCaptureRouter);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_disk_storage") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->m_bIsDiskStorage);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Rep_Type") == 0)
    {
        pDsn->iRepType = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"rep_file_timeout") == 0)
    {
        pDsn->m_iRepFileTimeout = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"long_sql_time") == 0)
    {
        pDsn->m_iLongSqlTime = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_shadow") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->m_bShadow);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_single_disaster") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->m_bSingleDisaster);
    }
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_shard_backup") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->m_bIsShardBackup);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::SetDsnOtherAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(pDsn);
    CHECK_OBJ_FILL(sAttrName);
    CHECK_OBJ_FILL(sAttrValue);
    if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"dsn_value") == 0)
    {
        //pDsn->iValue = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"log_level") == 0)
    {
        pDsn->iLogLevel = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"log_time") == 0)
    {
        pDsn->iLogTime = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"log_count") == 0)
    {
        pDsn->iLogCount = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"kill_time") == 0)
    {
        pDsn->iKillTime = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Delay_Time") == 0)
    {
        pDsn->iDelayTime = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Clean_Time") == 0)
    {
        pDsn->iCleanTime = TMdbNtcStrFunc::StrToInt(sAttrValue);	
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"ora_rep_counts") == 0)
    {
        pDsn->iOraRepCounts = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"buf_size") == 0)
    {
        pDsn->iLogBuffSize = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"file_size") == 0)
    {
        pDsn->iLogFileSize = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"file_path") == 0)
    {
        SAFESTRCPY(pDsn->sLogDir,sizeof(pDsn->sLogDir),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"manager_size") == 0)
    {
        pDsn->iManagerSize = TMdbNtcStrFunc::StrToInt(sAttrValue)*1024*1024;
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"data_size") == 0)
    {
        pDsn->iDataSize =  TMdbNtcStrFunc::StrToInt(sAttrValue)*1024*1024;
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"client_timeout") == 0)
    {
        pDsn->iClientTimeout = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"orarep_interval") == 0)
    {
        pDsn->m_iOraRepInterval = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"orarep_delaytime") == 0)
    {
        pDsn->m_iOraRepDelaySec = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Is_Seq_Cache") == 0)
    {
        pDsn->m_iSeqCacheSize = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"routing_list") == 0)
    {
        SAFESTRCPY(pDsn->sRoutingList,sizeof(pDsn->sRoutingList),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"routing_name") == 0)
    {
        SAFESTRCPY(pDsn->sRoutingName,sizeof(pDsn->sRoutingName),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"is_null") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->m_bNull);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"CsPumpMaxCount") == 0)
    {
        pDsn->m_iCSPumpMaxCount = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"CsPumpInitCount") == 0)
    {
        pDsn->m_iCSPumpInitCount = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"CsPeerCountPerPump") == 0)
    {
        pDsn->m_iCSPeerCountPerPump = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"valid_ip") == 0)
    {
        SAFESTRCPY(pDsn->m_sCSValidIP,sizeof(pDsn->m_sCSValidIP),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"invalid_ip") == 0)
    {
        SAFESTRCPY(pDsn->m_sCSInValidIP,sizeof(pDsn->m_sCSInValidIP),sAttrValue);
    }

    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Is_Answer") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->m_bSQLIsAnswer);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"not_load_table_list") == 0)
    {
        pDsn->m_strNotLoadFromDBList = sAttrValue;
    }

	else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Is_Reload_Ora") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->m_bReloadOra);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Is_Reload_Encrypt") == 0)
    {
        CONVERT_Y_N(sAttrValue,pDsn->m_bReloadEncrypt);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Reload_Cfg_Name") == 0)
    {
        SAFESTRCPY(pDsn->m_sReloadCfgName,sizeof(pDsn->m_sReloadCfgName),sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"Reload_Db_Type") == 0)
    {
        SAFESTRCPY(pDsn->m_sReloadDbType,sizeof(pDsn->m_sReloadDbType),sAttrValue);
    }
    
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbSqlParser::SetCfgProAttribute(TMdbCfgProAttr *pProAttr,char *sAttrName,char *sAttrValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(pProAttr);
    CHECK_OBJ_FILL(sAttrName);
    CHECK_OBJ_FILL(sAttrValue);
    if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"HeartBeatWarning") == 0)
    {
        pProAttr->iHeartBeatWarning = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"HeartBeatFatal") == 0)
    {
        pProAttr->iHeartBeatFatal = TMdbNtcStrFunc::StrToInt(sAttrValue);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

void TMdbSqlParser::InitDsnAttrName()
{
    TADD_FUNC("Start.");
    m_mapDsnAttrName["dsn_value"] = "dsn-value";
    m_mapDsnAttrName["log_level"] = "log-level";
    m_mapDsnAttrName["log_time"] = "log-time";
	m_mapDsnAttrName["log_count"] = "log-count";
    m_mapDsnAttrName["kill_time"] = "kill-time";
    m_mapDsnAttrName["net_drop_time"] = "Net-Drop-Time";
    m_mapDsnAttrName["delay_time"] = "Delay-Time";
    m_mapDsnAttrName["clean_time"] = "Clean-Time";
    m_mapDsnAttrName["delay_counts"] = "Delay-Counts";
    m_mapDsnAttrName["rep_type"] = "Rep-Type";

    m_mapDsnAttrName["ora_rep_counts"] = "ora-rep-counts";
    m_mapDsnAttrName["is_ora_rep"] = "is-ora-rep";
    m_mapDsnAttrName["is_rep"] = "is-rep";
    m_mapDsnAttrName["is_shard_backup"] = "is-shard-backup";
	m_mapDsnAttrName["is_online_rep"] = "is-online-rep";
    m_mapDsnAttrName["is_peer_rep"] = "is-peer-rep";
    m_mapDsnAttrName["is_capture_router"] = "is-capture-router";
    m_mapDsnAttrName["is_disk_storage"] = "is-disk-storage";

    m_mapDsnAttrName["is_readonlyattr"] = "is-ReadOnlyAttr";
    m_mapDsnAttrName["heartbeatwarning"] = "HeartBeatWarning";
    m_mapDsnAttrName["heartbeatfatal"] = "HeartBeatFatal";
    m_mapDsnAttrName["buf_size"] = "buf-size";
    m_mapDsnAttrName["file_size"] = "file-size";
    m_mapDsnAttrName["file_path"] = "file-path";
    //m_mapDsnAttrName["local_standby_ip"] = "local-standby-ip";
    //m_mapDsnAttrName["local_standby_port"] = "local-standby-port";

    //m_mapDsnAttrName["local_active_port"] = "local-active-port";
    //m_mapDsnAttrName["local_active_ip"] = "local-active-ip";
    //m_mapDsnAttrName["peer_active_ip"] = "peer-active-ip";
    //m_mapDsnAttrName["peer_active_port"] = "peer-active-port";
    //m_mapDsnAttrName["peer_standby_ip"] = "peer-standby-ip";
    //m_mapDsnAttrName["peer_standby_port"] = "peer-standby-port";
    m_mapDsnAttrName["local_ip"] = "Local-Ip";
	m_mapDsnAttrName["local_active_ip"] = "local-active-ip";
    m_mapDsnAttrName["agent_port"] = "agent-port";
	
	m_mapDsnAttrName["use_ntc_agent_port"] = "use-ntc-agent-port";
	m_mapDsnAttrName["notuse_ntc_agent_port"] = "notuse-ntc-agent-port";

	
    m_mapDsnAttrName["manager_size"] = "manager-size";
    m_mapDsnAttrName["data_size"] = "data-size";
    m_mapDsnAttrName["client_timeout"] = "client-timeout";
    m_mapDsnAttrName["orarep_interval"] = "orarep-interval";
    m_mapDsnAttrName["orarep_delaytime"] = "orarep-delaytime";
    m_mapDsnAttrName["is_seq_cache"] = "Is-Seq-Cache";
    m_mapDsnAttrName["routing_list"] = "routing-list";
    m_mapDsnAttrName["routing_name"] = "routing-name";
    m_mapDsnAttrName["is_disk_storage"] = "is-disk-storage";

    m_mapDsnAttrName["long_sql_time"] = "long-sql-time";
    m_mapDsnAttrName["rep_file_timeout"] = "rep-file-timeout";
    m_mapDsnAttrName["is_shadow"] = "is-shadow";
    m_mapDsnAttrName["is_single_disaster"] = "is-single-disaster";
    m_mapDsnAttrName["is_shard_backup"] = "is-shard-backup";
    m_mapDsnAttrName["is_null"] = "is-null";
    m_mapDsnAttrName["cspumpmaxcount"] = "CsPumpMaxCount";
    m_mapDsnAttrName["cspumpinitcount"] = "CsPumpInitCount";
    m_mapDsnAttrName["cspeercountperpump"] = "CsPeerCountPerPump";
    m_mapDsnAttrName["valid_ip"] = "valid-ip";
    m_mapDsnAttrName["invalid_ip"] = "invalid-ip";

    m_mapDsnAttrName["rep_server_ip"] = "Rep-Server-ip";
    m_mapDsnAttrName["rep_server_port"] = "Rep-Server-port";
    m_mapDsnAttrName["rep_standby_server_ip"] = "Rep-standby-Server-ip";
    m_mapDsnAttrName["rep_standby_server_port"] = "Rep-standby-Server-port";
    m_mapDsnAttrName["rep_local_port"] = "Rep-Local-port";
    m_mapDsnAttrName["rep_file_invalid_time"] = "Rep-file-invalid-time";
    m_mapDsnAttrName["is_answer"] = "is-answer";
    m_mapDsnAttrName["not_load_table_list"] = "not-load-table-list";

	m_mapDsnAttrName["is_reload_ora"] = "Is-Reload-Ora";
    m_mapDsnAttrName["is_reload_encrypt"] = "Is-Reload-Encrypt";
    m_mapDsnAttrName["reload_cfg_name"] = "Reload-Cfg-Name";
    m_mapDsnAttrName["reload_db_type"] = "Reload-Db-Type";

	m_mapDsnAttrName["is_use_ntc"] = "is-use-ntc";
	
    TADD_FUNC("Finish.");
}

/******************************************************************************
* ��������	:  BuildAddSequence
* ��������	:  �����������нṹ
* ����		:  pTableName ����
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::BuildAddSequence()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    //CHECK_OBJ_FILL(pTableName);
    //char * sTableName =  QMDB_MALLOC->NameFromToken(pTableName);
    do{
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        } 
        if(NULL == m_pDDLSqlStruct->pDsn)
        {
            m_pDDLSqlStruct->pDsn = new (std::nothrow)TMdbCfgDSN();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pDsn);
        }
        CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
        m_pDDLSqlStruct->pTable->Clear();
        //TMdbNtcStrFunc::ToUpper(sTableName,m_pDDLSqlStruct->pTable->sTableName);
        //strcpy(m_pDDLSqlStruct->pTable->sTableName, TMdbNtcStrFunc::ToUpper(sTableName));
    }while(0);
    //QMDB_MALLOC->ReleaseStr(sTableName);
    m_pDDLSqlStruct->iSqlType[0] = TK_ADD;
    m_pDDLSqlStruct->iSqlType[1] = TK_SEQUENCE;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  BuildDelSequence
* ��������	:  ����ɾ�����нṹ
* ����		:  pTableName ����
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::BuildDelSequence()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    //CHECK_OBJ_FILL(pTableName);
    //char * sTableName =  QMDB_MALLOC->NameFromToken(pTableName);
    do{
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        }  
        if(NULL == m_pDDLSqlStruct->pDsn)
        {
            m_pDDLSqlStruct->pDsn = new (std::nothrow)TMdbCfgDSN();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pDsn);
        }
        CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
        m_pDDLSqlStruct->pTable->Clear();
        //TMdbNtcStrFunc::ToUpper(sTableName,m_pDDLSqlStruct->pTable->sTableName);
        //strcpy(m_pDDLSqlStruct->pTable->sTableName,TMdbNtcStrFunc::ToUpper(sTableName) );
    }while(0);
    //QMDB_MALLOC->ReleaseStr(sTableName);
    m_pDDLSqlStruct->iSqlType[0] = TK_DROP;
    m_pDDLSqlStruct->iSqlType[1] = TK_SEQUENCE;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  BuildAlterSequence
* ��������	:  ����������нṹ
* ����		:  pTableName ����
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::BuildAlterSequence()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    //CHECK_OBJ_FILL(pTableName);
    //char * sTableName =  QMDB_MALLOC->NameFromToken(pTableName);
    do{
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        }  
        if(NULL == m_pDDLSqlStruct->pDsn)
        {
            m_pDDLSqlStruct->pDsn = new (std::nothrow)TMdbCfgDSN();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pDsn);
        }
        CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
        m_pDDLSqlStruct->pTable->Clear();
        //TMdbNtcStrFunc::ToUpper(sTableName,m_pDDLSqlStruct->pTable->sTableName);
        //strcpy(m_pDDLSqlStruct->pTable->sTableName, TMdbNtcStrFunc::ToUpper(sTableName));
    }while(0);
    //QMDB_MALLOC->ReleaseStr(sTableName);
    m_pDDLSqlStruct->iSqlType[0] = TK_ALTER;
    m_pDDLSqlStruct->iSqlType[1] = TK_SEQUENCE;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  AddSequenceValue
* ��������	:  ��ȡ�������л�ɾ�����е���ֵ
* ����		:  pColumnName ������pValue ��ֵ
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::AddSequenceValue(Token *pColumnName,Token *pValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pColumnName);
    CHECK_OBJ_FILL(pValue);
    char * sColumnName   = QMDB_MALLOC->NameFromToken(pColumnName);
    char * sAttrValue  = QMDB_MALLOC->NameFromToken(pValue);
    TADD_DETAIL("pAttrName = %s,pAttrValue=%s",sColumnName,sAttrValue);
    do{        
        if(TMdbNtcStrFunc::StrNoCaseCmp(sColumnName,"SEQ_NAME") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->tMemSeq.sSeqName,\
                sizeof(m_pDDLSqlStruct->tMemSeq.sSeqName),sAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sColumnName,"DSN_NAME") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->pDsn->sName,\
                sizeof(m_pDDLSqlStruct->pDsn->sName),sAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sColumnName,"START_NUMBER") == 0)
        {
            m_pDDLSqlStruct->tMemSeq.iStart = TMdbNtcStrFunc::StrToInt(sAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sColumnName,"END_NUMBER") == 0)
        {
            m_pDDLSqlStruct->tMemSeq.iEnd = TMdbNtcStrFunc::StrToInt(sAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sColumnName,"STEP_NUMBER") == 0)
        {
            m_pDDLSqlStruct->tMemSeq.iStep = TMdbNtcStrFunc::StrToInt(sAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sColumnName,"CUR_NUMBER") == 0)
        {
            m_pDDLSqlStruct->tMemSeq.iCur = TMdbNtcStrFunc::StrToInt(sAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sColumnName,"LOCAL_IP") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->pDsn->sLocalIP,\
                sizeof(m_pDDLSqlStruct->pDsn->sLocalIP),sAttrValue);
        }
        else
        {
            CHECK_RET_FILL_BREAK(-1,ERR_TAB_ATTR_NOT_EXIST,\
                "The column[%s] of the table[%s] does not exist",pColumnName,m_pDDLSqlStruct->pTable->sTableName);
        }
    }while(0);
    QMDB_MALLOC->ReleaseStr(sColumnName);
    QMDB_MALLOC->ReleaseStr(sAttrValue);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  CollectRouterID
* ��������	:  �ɼ�routerID��Ϣ
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::CollectRouterID(_ST_EXPR * & pstExpr)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    QMDB_MALLOC->ReleaseExpr(pstExpr);
    TMdbTable * pTable = m_stSqlStruct.pMdbTable;
    CHECK_OBJ_FILL(pTable);
    int i = 0;
    for(i = 0;i < pTable->iColumnCounts;++i)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(pTable->tColumn[i].sName,m_pMdbConfig->GetDSN()->sRoutingName) == 0)
        {//��·����Ϣ��
            TADD_FUNC("Find column[%d] is [%s]",i,m_pMdbConfig->GetDSN()->sRoutingName);
            Token token;
            token.z = m_pMdbConfig->GetDSN()->sRoutingName;
            token.n = strlen(m_pMdbConfig->GetDSN()->sRoutingName);
            TMdbExpr tMdbExpr;
            pstExpr =  tMdbExpr.BuildPExpr(TK_ID,NULL,NULL,&token);
            break;
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* ��������	:  IsCurRowNeedCapture
* ��������	:  ��ǰ���Ƿ���Ҫ����
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbSqlParser::IsCurRowNeedCapture()
{
    if(NULL == m_stSqlStruct.pstRouterID){return false;}//û��·��ID
    if(TK_SELECT == m_stSqlStruct.iSqlType){return false;}//select ������
   // if(m_iSourceId != SOURCE_APP){return false;}//����Ҫ��أ������Ǳ���ͬ��������
    if(false == m_stSqlStruct.pShmDSN->GetInfo()->m_bIsCaptureRouter){return false;}//����Ҫ����
    int * arrRouterToCap = m_stSqlStruct.pShmDSN->GetInfo()->m_arrRouterToCapture;
    if(NULL == arrRouterToCap){return false;}
    int i = 0;
    long long llRowRouter = m_stSqlStruct.pstRouterID->pExprValue->lValue;
    for(i = 1;i <= arrRouterToCap[0];++i)
    {
        if(llRowRouter == arrRouterToCap[i])
        {
            return true;
        }
    }
    return false;
}

/******************************************************************************
* ��������	:  BuildCreateJob
* ��������	:  ��������job
* ����		:  pJobName job��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::BuildCreateJob(Token * pJobName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pJobName);
    char * sJobName =  QMDB_MALLOC->NameFromToken(pJobName);
    do
    {
        if(NULL == m_pDDLSqlStruct->pMdbJob)
        {
            m_pDDLSqlStruct->pMdbJob = new (std::nothrow)TMdbJob();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pMdbJob);
            m_pDDLSqlStruct->pMdbJob->Clear();
        } 
        SAFESTRCPY(m_pDDLSqlStruct->pMdbJob->m_sName,\
            sizeof(m_pDDLSqlStruct->pMdbJob->m_sName),sJobName);
    }while(0); 
    QMDB_MALLOC->ReleaseStr(sJobName);
    m_pDDLSqlStruct->iSqlType[0] = TK_CREATE;
    m_pDDLSqlStruct->iSqlType[1] = TK_JOB;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  BuildAlterJob
* ��������	:  �����޸�job
* ����		:  pJobName job��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::BuildAlterJob(Token * pJobName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pJobName);
    char * sJobName =  QMDB_MALLOC->NameFromToken(pJobName);
    do
    {
        if(NULL == m_pDDLSqlStruct->pMdbJob)
        {
            m_pDDLSqlStruct->pMdbJob = new (std::nothrow)TMdbJob();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pMdbJob);
            m_pDDLSqlStruct->pMdbJob->Clear();
        } 
        SAFESTRCPY(m_pDDLSqlStruct->pMdbJob->m_sName,\
            sizeof(m_pDDLSqlStruct->pMdbJob->m_sName),sJobName);
    }while(0); ;
    QMDB_MALLOC->ReleaseStr(sJobName);
    m_pDDLSqlStruct->iSqlType[0] = TK_ALTER;
    m_pDDLSqlStruct->iSqlType[1] = TK_JOB;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  BuildRemoveJob
* ��������	:  ����ɾ��job
* ����		:  pJobName job��
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::BuildRemoveJob(Token * pJobName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pJobName);
    char * sJobName =  QMDB_MALLOC->NameFromToken(pJobName);
    do
    {
        if(NULL == m_pDDLSqlStruct->pMdbJob)
        {
            m_pDDLSqlStruct->pMdbJob = new (std::nothrow)TMdbJob();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pMdbJob);
            m_pDDLSqlStruct->pMdbJob->Clear();
        } 
        SAFESTRCPY(m_pDDLSqlStruct->pMdbJob->m_sName,\
            sizeof(m_pDDLSqlStruct->pMdbJob->m_sName),sJobName);
    }while(0); 
    QMDB_MALLOC->ReleaseStr(sJobName);
    m_pDDLSqlStruct->iSqlType[0] = TK_REMOVE;
    m_pDDLSqlStruct->iSqlType[1] = TK_JOB;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  AddJobValue
* ��������	:  ��ȡjob���Ժ�ֵ
* ����		:  pAttrName job���ԣ�pAttrValue ��ֵ
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::AddJobValue(Token *pAttrName,Token *pAttrValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pAttrName);
    CHECK_OBJ_FILL(pAttrValue);
    char * sAttrName   = QMDB_MALLOC->NameFromToken(pAttrName);
    char * sAttrValue  = QMDB_MALLOC->NameFromToken(pAttrValue);
    TADD_DETAIL("pAttrName = %s,pAttrValue=%s",sAttrName,sAttrValue);
    do{        
        if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"exec_date") == 0)
        {
            if(!TMdbNtcStrFunc::IsDateTime(sAttrValue))
            {
                TADD_ERROR(ERROR_UNKNOWN,"the value[%s] of exec_date wrong format,it must be YYYYMMDDHHMMSS.",sAttrValue);
                break;
            }
            SAFESTRCPY(m_pDDLSqlStruct->pMdbJob->m_sExecuteDate,\
                sizeof(m_pDDLSqlStruct->pMdbJob->m_sExecuteDate),sAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"interval") == 0)
        {
            m_pDDLSqlStruct->pMdbJob->m_iInterval = TMdbNtcStrFunc::StrToInt(sAttrValue);
            if(m_pDDLSqlStruct->pMdbJob->m_iInterval <= 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"The value[%d] of interval is less than zero.",\
                    m_pDDLSqlStruct->pMdbJob->m_iInterval);
                break;
            }
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"ratetype") == 0)
        {
            CHECK_RET_BREAK(m_pDDLSqlStruct->pMdbJob->SetRateType(sAttrValue),\
                "ratetype[%s] is error ,it must be sec or min or hour or day or month or year.",sAttrValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sAttrName,"sql") == 0)
        {
            SAFESTRCPY(m_pDDLSqlStruct->pMdbJob->m_sSQL,\
                sizeof(m_pDDLSqlStruct->pMdbJob->m_sSQL),sAttrValue);
        }
        else
        {
            CHECK_RET_FILL_BREAK(-1,ERR_TAB_ATTR_NOT_EXIST,\
                "Job[%s] attribute[%s] does not exist",m_pDDLSqlStruct->pMdbJob->m_sName,sAttrName);
        }
    }while(0);
    QMDB_MALLOC->ReleaseStr(sAttrName);
    QMDB_MALLOC->ReleaseStr(sAttrValue);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  BuildAddFlushSQLParam
* ��������	:  ����flush-sql or load-sql�а󶨲�������Ϣ
* ����		:  
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::BuildAddFlushSQLParam(int iIFNE,Token *pTableName,Token *pParamName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pTableName);
    CHECK_OBJ_FILL(pParamName);
    char * pTable_Name = QMDB_MALLOC->NameFromToken(pTableName);
    char * pParam_Name = QMDB_MALLOC->NameFromToken(pParamName);
    do{        
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
            m_pDDLSqlStruct->pTable->Clear();
        }
        if(NULL != m_pMdbConfig)
        {
            TMdbTable *pTable = m_pMdbConfig->GetTable(pTable_Name);
            if(pTable == NULL)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                    "Table[%s] configuration file does not exist,please check.",pTable_Name);
            }   
        }
        m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);
        //�������10������
        if(m_pDDLSqlStruct->pTable->iParameterCount >= MAX_INDEX_COUNTS)
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                "The number of SQL(flush-sql and load-sql) binding parameters have reached the maximum[10].");
        }
        m_pDDLSqlStruct->pTable->iParameterCount++;
        TMdbParameter &tParam = m_pDDLSqlStruct->pTable->tParameter[m_pDDLSqlStruct->pTable->iParameterCount-1];
        SAFESTRCPY(tParam.sName,sizeof(tParam.sName),pParam_Name);
        SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,sizeof(m_pDDLSqlStruct->pTable->sTableName),pTable_Name);
    }while(0);
    m_pDDLSqlStruct->iSqlType[0] = TK_ADD;
    m_pDDLSqlStruct->iSqlType[1] = TK_PARAMETER;
    QMDB_MALLOC->ReleaseStr(pTable_Name);
    QMDB_MALLOC->ReleaseStr(pParam_Name);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  AddFlushSQLorLoadSQLParamAttr
* ��������	:  �������parameter����������
* ����		:  
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::AddFlushSQLorLoadSQLParamAttr(const char * pAttrName,Token *pAttrValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pAttrName);
    CHECK_OBJ_FILL(pAttrValue);
    char * pValue = QMDB_MALLOC->NameFromToken(pAttrValue);
    do{        
        TMdbParameter &tParam = m_pDDLSqlStruct->pTable->tParameter[m_pDDLSqlStruct->pTable->iParameterCount-1];
        if(TMdbNtcStrFunc::StrNoCaseCmp(pAttrName,"data-type") == 0)
        {
            TMdbConfig tMdbConfig;
            tParam.iDataType = tMdbConfig.GetDataType(pValue);
            if(tParam.iDataType < 0)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"Invalid data type[%s]",pValue);
            }
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttrName,"value") == 0)
        {	
            SAFESTRCPY(tParam.sValue,sizeof(tParam.sValue),pValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttrName,"parameter-type") == 0)
        {	
            tParam.iParameterType = atoi(pValue);
        }
        else
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                "Parameter attribute[%s] is not supported.",pAttrName);
        }
    }while(0);
    QMDB_MALLOC->ReleaseStr(pValue);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  BuildModifyFlushSQLParam
* ��������	:  �����޸ı�ָ���Ĳ���
* ����		:  
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::BuildModifyFlushSQLParam(int iIFNE,Token *pTableName,Token *pParamName,const char*pOptName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pTableName);
    CHECK_OBJ_FILL(pParamName);
    char * pTable_Name = QMDB_MALLOC->NameFromToken(pTableName);
    char * pParam_Name = QMDB_MALLOC->NameFromToken(pParamName);
    do{        
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
            CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);
            m_pDDLSqlStruct->pTable->Clear();
        }
        m_pDDLSqlStruct->bIfNE = (iIFNE != 0?true:false);
        m_pDDLSqlStruct->tParam.Clear();
        SAFESTRCPY(m_pDDLSqlStruct->tParam.sName,sizeof(m_pDDLSqlStruct->tParam.sName),pParam_Name);
        SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,sizeof(m_pDDLSqlStruct->pTable->sTableName),pTable_Name);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pOptName,"drop") == 0)
        {
            m_pDDLSqlStruct->iSqlType[0] = TK_DROP;
            m_pDDLSqlStruct->iSqlType[1] = TK_PARAMETER;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pOptName,"modify") == 0)
        {
            m_pDDLSqlStruct->iSqlType[0] = TK_MODIFY;
            m_pDDLSqlStruct->iSqlType[1] = TK_PARAMETER;
        }
        else
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                "Does not support to %s parameters[%s].",pOptName,pParam_Name);
        }
    }while(0);
    QMDB_MALLOC->ReleaseStr(pTable_Name);
    QMDB_MALLOC->ReleaseStr(pParam_Name);
    TADD_FUNC("Finish.");
    return iRet;

}

/******************************************************************************
* ��������	:  ModifyFlushSQLorLoadSQLParamAttr
* ��������	:  ����޸Ĳ���������
* ����		:  
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  cao.peng
*******************************************************************************/
int TMdbSqlParser::ModifyFlushSQLorLoadSQLParamAttr(const char * pAttrName,Token *pAttrValue)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(m_pDDLSqlStruct);
    CHECK_OBJ_FILL(pAttrName);
    CHECK_OBJ_FILL(pAttrValue);
    char * pValue = QMDB_MALLOC->NameFromToken(pAttrValue);
    do{        
        TADD_DETAIL("AttrName = %s,AttrValue = %s.",pAttrName,pValue);
        if(TMdbNtcStrFunc::StrNoCaseCmp(pAttrName,"data-type") == 0)
        {
            TMdbConfig tMdbConfig;
            m_pDDLSqlStruct->tParam.iDataType = tMdbConfig.GetDataType(pValue);
            if(m_pDDLSqlStruct->tParam.iDataType < 0)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"Invalid data type[%s]",pValue);
            }
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttrName,"value") == 0)
        {	
            SAFESTRCPY(m_pDDLSqlStruct->tParam.sValue,sizeof(m_pDDLSqlStruct->tParam.sValue),pValue);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttrName,"parameter-type") == 0)
        {	
            m_pDDLSqlStruct->tParam.iParameterType = atoi(pValue);
        }
        else
        {
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                "Parameter attribute[%s] is not supported.",pAttrName);
        }      
    }while(0);
    QMDB_MALLOC->ReleaseStr(pValue);
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* ��������	:  TranslateBlobColumnValue
* ��������	:  ת��sql�е�blobֵ
* ����		:  
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::TranslateBlobColumnValue(ST_EXPR * pExpr,TMdbColumn * pColumn)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ_FILL(pExpr);
    CHECK_OBJ_FILL(pColumn);
    if(DT_Blob == pColumn->iDataType)
    {
        switch(pExpr->iOpcode)
        {
        case TK_STRING:
            {
                //ת��
                std::string encoded = Base::base64_encode(reinterpret_cast<const unsigned char*>(pExpr->pExprValue->sValue),strlen(pExpr->pExprValue->sValue));
                QMDB_MALLOC->ReleaseStr(pExpr->pExprValue->sValue);
                pExpr->pExprValue->sValue = QMDB_MALLOC->CopyFromStr(encoded.c_str());
                pExpr->pExprValue->iSize  = strlen(pExpr->pExprValue->sValue) +1;
                MemValueSetProperty(pExpr->pExprValue,MEM_Str|MEM_Dyn);
            }
            break;
        case TK_ID: //�кͰ󶨱�������ת��         
        case TK_VARIABLE:
        case TK_NULL:
            break;
        default:
            CHECK_RET(ERR_SQL_INVALID,"Column[%s] can not set use [%s]",pColumn->sName,TokenName[pExpr->iOpcode]);
            break;
        }
    }
    return iRet;
}
/******************************************************************************
* ��������	:  FillFirstAndLimit
* ��������	:  ���first limit
* ����		:  
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillFirstAndLimit(ST_EXPR * pFirst,ST_EXPR * pLimit,ST_EXPR * pOffset)
{
    int iRet = 0;
    if(NULL != pFirst && NULL != pLimit)
    {//first ��limit ֻ����һ��
        CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"first and limit can be only one.");
    }
    if(NULL != pFirst)
    {
        m_stSqlStruct.pstLimit = pFirst;
    }
    else
    {
        m_stSqlStruct.pstLimit = pLimit;
    }
    m_stSqlStruct.pstOffset = pOffset;
    return iRet;
}

void TMdbSqlParser::SetTimeStamp(long long iTimeStamp)
{
    m_iSetTimeStamp = iTimeStamp;
}

long long TMdbSqlParser::GetTimeStamp()
{
    return m_iSetTimeStamp;
}

int TMdbSqlParser::CheckOrderByInGroupBy()
{
    int iRet = 0;
	//order by�е���, ��������� group by ����
	if(m_stSqlStruct.pOrderby != NULL)
	{
	    for(int i = 0; i < m_stSqlStruct.pOrderby->iItemNum ;++i)
	    {
	        ST_EXPR * pstSelExpr =  m_stSqlStruct.pOrderby->pExprItems[i].pExpr;
	        if(NULL == pstSelExpr->pExprValue || NULL == pstSelExpr->pExprValue->pColumn){continue;}
	        bool bMatch = false;//û�ҵ�
	        int j = 0;
	        for(j = 0;j < m_stSqlStruct.pGroupby->iItemNum;++j)
	        {  
	            ST_EXPR * pstExpr =  m_stSqlStruct.pGroupby->pExprItems[j].pExpr;
	            if(pstSelExpr->pExprValue->pColumn == pstExpr->pExprValue->pColumn)
	            {//����ͬ
	                bMatch = true;
	                break;
	            }
	        }
	        if(false == bMatch)
	        {//group by ��order by �б�ƥ��
	            CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"Column[%s] in order by ,but not in group by.",
	                pstSelExpr->pExprValue->pColumn->sName);
	        }
	    }
	}
	return iRet;
}
/******************************************************************************
* ��������	:  BuildGroupBy
* ��������	:  ����group by�Ӿ�,
group by ��һ��ԭ��,���� select �������������,û��ʹ�þۺϺ�������,
��������� group by ����
* ����		:  
* ���		:  ��
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::BuildGroupBy()
{
    int iRet = 0;
    if(NULL == m_stSqlStruct.pGroupby && NULL != m_stSqlStruct.pHaving)
    {//having�Ӿ��������group by��
        CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"[having]SQL should be used with[group by]");
    }
    if(NULL == m_stSqlStruct.pGroupby){return 0;}//û��group by
    //group by ������
    int i = 0;
    for(i = 0;i < m_stSqlStruct.pGroupby->iItemNum;++i)
    {  
        ST_EXPR * pstExpr =  m_stSqlStruct.pGroupby->pExprItems[i].pExpr;
        if(NULL == pstExpr->pExprValue || NULL == pstExpr->pExprValue->pColumn)
        {
            CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"Group by column[%d] is error.",i);
        }
    }
    //select �������������,û��ʹ�þۺϺ�������, ��������� group by ����
    if(m_stSqlStruct.pColList != NULL)
	{
	    for(i = 0; i < m_stSqlStruct.pColList->iItemNum ;++i)
	    {
	        ST_EXPR * pstSelExpr =  m_stSqlStruct.pColList->pExprItems[i].pExpr;
	        if(NULL == pstSelExpr->pExprValue || NULL == pstSelExpr->pExprValue->pColumn){continue;}//
	        bool bMatch = false;//û�ҵ�
	        int j = 0;
	        for(j = 0;j < m_stSqlStruct.pGroupby->iItemNum;++j)
	        {  
	            ST_EXPR * pstExpr =  m_stSqlStruct.pGroupby->pExprItems[j].pExpr;
	            if(pstSelExpr->pExprValue->pColumn == pstExpr->pExprValue->pColumn)
	            {//����ͬ
	                bMatch = true;
	                break;
	            }
	        }
	        if(false == bMatch)
	        {//group by ��select �б�ƥ��
	            CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"Column[%s] in select ,but not in group by.",
	                pstSelExpr->pExprValue->pColumn->sName);
	        }
	    }
	}
	
    return iRet;
}

int TMdbSqlParser::CheckTableExist()
{
    int iRet = 0;
    if(NULL == m_stSqlStruct.sTableName) return iRet;
    if(m_stSqlStruct.sTableName[0] != 0 && NULL == m_tSqlParserHelper.GetMdbTable(m_stSqlStruct.sTableName)) 
    {
        iRet = ERR_TAB_NO_TABLE;
    }
    return iRet;
}

int TMdbSqlParser::BuildRenameTable(Token * pOldTableName,Token * pNewTableName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char * sOldName =  QMDB_MALLOC->NameFromToken(pOldTableName);
    char * sNewName =  QMDB_MALLOC->NameFromToken(pNewTableName);
    
    do{
        char sOldTableName[MAX_NAME_LEN]={0}; 
        char sNewTableName[MAX_NAME_LEN]={0}; 
        SAFESTRCPY(sOldTableName, sizeof(sOldTableName), sOldName);
        SAFESTRCPY(sNewTableName, sizeof(sNewTableName), sNewName);
        TMdbNtcStrFunc::ToUpper(sOldTableName);
        TMdbNtcStrFunc::ToUpper(sNewTableName);
        if(NULL == m_pDDLSqlStruct->pTable)
        {
            m_pDDLSqlStruct->pTable = new (std::nothrow)TMdbTable();
        }
        CHECK_OBJ_FILL_BREAK(m_pDDLSqlStruct->pTable);

        if(NULL != m_pMdbConfig)
        {
            TMdbTable *pTable = m_tSqlParserHelper.GetMdbTable(sOldTableName);
            if(pTable == NULL)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                    "Table[%s] does not exist,please check.",sOldTableName);
            }
            memcpy(m_pDDLSqlStruct->pTable,pTable,sizeof(TMdbTable));
            
            pTable = m_tSqlParserHelper.GetMdbTable(sNewTableName);
            if(pTable)
            {
                CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,\
                    "Table[%s] exist,can not rename [%s] to it,please check.",sNewTableName,sOldTableName);
            }
            
        }
        else
        {//����������ڣ�Ҳ����rename
            SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,sizeof(m_pDDLSqlStruct->pTable->sTableName),sOldTableName);
        }
        SAFESTRCPY(m_pDDLSqlStruct->sNewTableName,sizeof(m_pDDLSqlStruct->sNewTableName),sNewTableName);
    }
    while(0);
    QMDB_MALLOC->ReleaseStr(sOldName);
    QMDB_MALLOC->ReleaseStr(sNewName);
    
    m_pDDLSqlStruct->iSqlType[0] = TK_RENAME;
    m_pDDLSqlStruct->iSqlType[1] = TK_TABLE;
    TADD_FUNC("Finish.");
    return iRet;

}


//}

