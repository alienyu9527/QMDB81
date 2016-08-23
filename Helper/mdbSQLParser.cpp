/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbSQLParser.cpp		
*@Description： Sql解析器对外的接口类
*@Author:	    jin.shaohua
*@Date：	    2012.05
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

//DDL关键字，新加关键字需要在这里加入。
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
    * 函数名称	:  TMdbSqlParser
    * 函数描述	:  构造
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbSqlParser::TMdbSqlParser():m_pMdbConfig(NULL), m_bGetValueChecked(false)
{
	m_pDDLSqlStruct = NULL;
	m_bIsDDLSQL     = false;
	m_iSetTimeStamp = 0;
	m_pHintIndex = NULL;
}

/******************************************************************************
* 函数名称	:  ~TMdbSqlParser
* 函数描述	:  析构，做一些清理工作
* 输入		:  无
* 输出		:  无
* 返回值	:  无
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbSqlParser::~TMdbSqlParser()
{
    CleanResult();//清理内存
	SAFE_DELETE(m_pDDLSqlStruct);
}

/******************************************************************************
* 函数名称	:  Init
* 函数描述	:  SQL解析器初始化工作，函数注册，清理资源等
* 输入		:  无
* 输出		:  无
* 返回值	:  0-正确
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::Init()
{
    CleanResult();//清理结果
    return 0;
}



/******************************************************************************
* 函数名称	:  SetDB
* 函数描述	:  设置MDB的连接信息
* 输入		:  pMdbDsn - DSN信息
* 输入		:  pConfig     - mdb配置信息
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::SetDB(TMdbShmDSN * pMdbDsn,TMdbConfig * pConfig)
{
    int iRet = 0;
    CHECK_OBJ(pMdbDsn);
    CHECK_OBJ(pConfig);
    m_pMdbConfig = pConfig;//不需要清理
    CHECK_RET_FILL(m_tSqlParserHelper.SetDB(pMdbDsn),ERR_DB_NOT_CONNECTED," SetDB failed..");
    //m_tMdbExpr.SetNullFlag(m_pMdbConfig->GetDSN()->m_bNull);//设置null处理规则
    m_tMdbExpr.SetNullFlag(true);//设置null处理规则,使用新规则。不兼容老逻辑

    return iRet;
}

/******************************************************************************
* 函数名称	:  SetMDBConfig
* 函数描述	:  设置MDB配置
* 输入		:  pConfig     - mdb配置信息
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::SetMDBConfig(TMdbConfig * pConfig)
{
    int iRet = 0;
    CHECK_OBJ(pConfig);
    m_pMdbConfig = pConfig;
    return iRet;
}


/******************************************************************************
* 函数名称	:  ParseSQL
* 函数描述	:  解析SQL语句
* 输入		:  sSql-待解析的sql语句
* 输出		:  无
* 返回值	:  0-正确
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ParseSQL(const char * sSql)
{
    CHECK_OBJ(sSql);
    if(0 == sSql[0] || ';' ==  sSql[0] )
    {
        m_tError.FillErrMsg(ERR_SQL_INVALID,"sql[%s] is empty",sSql);
        return ERR_SQL_INVALID;
    }
    //DDL 检查
    int iRet = 0;
    CHECK_RET_FILL(CheckDDLSQL(sSql),ERR_SQL_INVALID,"CheckDDLSQL failed");
    TADD_FUNC("Start.SQL=[%s].",sSql);
    int tokenType;
    Token sLastToken;
    memset(&sLastToken,0x00,sizeof(Token));
    void * pEngine = NULL;//语法解析器
    do
    {
        Init();//初始化做一些清理工作
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
        {//有错误
            m_tError.FillErrMsg(ERR_SQL_INVALID,"sql=[%s] Error near [%s],ErrCode=[%d],ErrMsg[%s]",
                sSql,sLastToken.z,m_tError.GetErrCode(),m_tError.GetErrMsg());
            break;
        }
        if (TK_SEMI != tokenType)
        {//未结束
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
        SAFESTRCPY(m_stSqlStruct.sSQL,sizeof(m_stSqlStruct.sSQL),sSql);//保存SQL语句
    }
    return m_tError.GetErrCode();
}

//检查DDL SQL
int TMdbSqlParser::CheckDDLSQL(const char * sSql)
{
    int iRet = 0;
    m_bIsDDLSQL = false;
    char sCmd[MAX_DDL_CMD_LEN+2] = {0};//最多需要检查MAX_DDL_CMD_LEN个字符
    char *p = (char*)memccpy(sCmd, sSql, ' ', sizeof(sCmd)-1);
    if(NULL == p) return iRet;//没有空格
    *(p-1) = '\0';//去掉空格
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
//对于不解析SQL，而直接调用DDL 解析器的接口，需要手工申请空间
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
* 函数名称	:  FillTableName
* 函数描述	:  获取sql语句中的表名以及别名
* 输入		:  pToken - 表名的token
* 输入		:  pTokenAlias -  表的别名的token
* 输出		:  无
* 返回值	:  0 - 正确
* 作者		:  jin.shaohua
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
    //m_tSqlParserHelper.BuildColumIndexMap(m_stSqlStruct.pMdbTable);//构建column -  index映射
    m_stSqlStruct.pShmDSN = m_tSqlParserHelper.GetMdbShmDSN();//设置shmdsn
    TADD_FUNC("Finish");
    return iRet;
}

/******************************************************************************
* 函数名称	:  CleanResult
* 函数描述	:  清理上次的SQL解析结果
* 输入		:  无
* 输出		:  无
* 返回值	:  无
* 作者		:  jin.shaohua
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
    //清理各个值	
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
    //清理其它
    CleanOtherMem();
    //m_iSourceId = SOURCE_APP;
    m_tError.FillErrMsg(0,"");//清理错误信息
    m_bGetValueChecked = false;
    m_vAggExpr.clear();
    m_iSetTimeStamp = -1;
	m_pHintIndex = NULL;
    TADD_FUNC("Finish.");
}
/******************************************************************************
* 函数名称	:  ClearLastExecute
* 函数描述	:  清理上一次的执行
* 输入		:  无
* 输出		:  无
* 返回值	:  无
* 作者		:  jin.shaohua
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
    m_stSqlStruct.iRowPos = 0; //清理row pos
    return iRet;
}


/******************************************************************************
* 函数名称	:  IdListAppend
* 函数描述	:  向idlist (ST_ID_LIST),添加元素
* 输入		:  pList - 被添加的idlist，pToken-添加的元素
* 输出		:  无
* 返回值	:  成功- 返回添加后的idlist,  失败- 0
* 作者		:  jin.shaohua
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
* 函数名称	:  BuildSelect
* 函数描述	:  构建select解析结构
* 输入		:  pFirst - first 关键字组。isDistinct - 是否包含distinct
* 输入		:  pColList - 查询结果，   pstWhere - where 条件组
* 输入		:  pstOrderby - order by 组， pLimit - limit 组，pOffset - pOffset组
pstGroupby - group by 子句pstHaving - having子句

* 输出		:  无
* 返回值	:  ST_SQL_STRUCT * - 构建成功，NULL- 构建失败
* 作者		:  jin.shaohua
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
    {//having子句必须用在group by中
        CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"[having]SQL should be used with[group by]");
    }
    CHECK_RET_FILL(FillFirstAndLimit( pFirst, pLimit,pOffset),ERR_SQL_INVALID,"FillFirstAndLimit failed.");//       
    if(NULL == m_stSqlStruct.pMdbTable)
    {
        CHECK_RET_FILL(ERR_SQL_INVALID,ERR_TAB_NO_TABLE,"table does not exist.");
    }
    CHECK_RET_FILL(RemoveTableAlias(),ERR_SQL_INVALID,"RemoveTableAlias failed");//去除表别名    
	CHECK_RET_FILL(FillHintInfo(),ERR_SQL_INVALID,"FillHintInfo Failed.");
    //CHECK_RET_FILL(FillSequenceValue(),ERR_SQL_INVALID,"FillSequenceValue failed");	
    CHECK_RET_FILL(SpanTKALLCollumnList(m_stSqlStruct.pColList),ERR_TAB_COLUMN_NOT_EXIST,"span * error.");//扩充*
    CHECK_RET_FILL(FillMdbInfo(),ERR_SQL_FILL_MDB_INFO,"FillMdbInfo error.");//填充MDB相关信息
    CHECK_RET_FILL(CollectWhereIndex(),ERR_TAB_INDEX_NOT_EXIST,"CollectWhereIndex failed");//搜集where中的index
    CHECK_RET_FILL(m_tSqlTypeCheck.FillValidType(&m_stSqlStruct),ERR_SQL_INVALID,"FillValidType failed..");
    CHECK_RET_FILL(FillOrderbyInfo(m_stSqlStruct.pOrderby),ERR_SQL_INVALID,"FillOrderbyInfo error...");
    CHECK_RET_FILL(BuildGroupBy(),ERR_SQL_INVALID,"BuildGroupBy error...");//构建group by
    CollectOutputValue();//搜集输出数据
    CollectInputValue();//搜集需要输入的数据节点
    if(m_listInputLimit.vMemValue.size() != 0)
    {//limit offset 中不可以有列信息
        CHECK_RET_FILL(-1,ERR_SQL_INVALID,"column can not in limit offset first.");
    }
    VectorToList();
    ChangeValueToRefence(m_listInputWhere);
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* 函数名称	:  BuildInsert
* 函数描述	:  构建insert解析结构
* 输入		:  pToken - 表名的token, pCollist - 插入的列信息
* 输入		:  pstExprList - 插入的值信息
* 输出		:  无
* 返回值	:  无
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::BuildInsert(ST_ID_LIST * pCollist,ST_EXPR_LIST * pstExprList)
{
    TADD_FUNC("Start.pCollist=[%p],pstExprList=[%p]",pCollist,pstExprList);
    int iRet = 0;
    m_stSqlStruct.iSqlType = TK_INSERT;
    m_stSqlStruct.pstIdList= pCollist;
    m_stSqlStruct.pColList =  pstExprList;
    CHECK_RET_FILL(RemoveTableAlias(),ERR_SQL_INVALID,"RemoveTableAlias failed");//去除表别名
    //CHECK_RET_FILL(FillSequenceValue(),ERR_SQL_INVALID,"FillSequenceValue failed");       
    CHECK_RET_FILL(CollectRouterID(m_stSqlStruct.pstRouterID),ERR_SQL_FILL_MDB_INFO,"CollectRouterID faild.");//搜集路由信息
    CHECK_RET_FILL(FillMdbInfo(),ERR_SQL_FILL_MDB_INFO,"FillMdbInfo error.");//填充MDB相关信息
    CHECK_RET_FILL(CollectWhereIndex(),ERR_TAB_INDEX_NOT_EXIST,"CollectWhereIndex failed");//搜集where中的index
    CHECK_RET_FILL(SpanInsertCollumnList(m_stSqlStruct.pstIdList),ERR_SQL_FILL_MDB_INFO,
        "SpanInsertCollumnList error.");//扩展插入值，搜集需要输出的数据节点。
    CHECK_RET_FILL(m_tSqlTypeCheck.FillValidType(&m_stSqlStruct),ERR_SQL_INVALID,"FillValidType failed..");
    CHECK_RET_FILL(CollectPrimaryKeyFromSQL(m_stSqlStruct.pstIdList),ERR_SQL_INVALID,"CollectPrimaryKey failed.");
    if(m_stSqlStruct.pMdbTable->bIsCheckPriKey && m_listOutputPriKey.vMemValue.size()>0)
    {//判断是否需要主键校验
        m_stSqlStruct.bCheckInsertPriKey = true;
        CHECK_RET_FILL(CollectIndexForVerifyPK(m_stSqlStruct.pstIdList),ERR_SQL_INVALID,"CollectIndexForVerifyPK failed.");
    }
    else
    {
        m_stSqlStruct.bCheckInsertPriKey = false;
    }
    CollectInputValue();//搜集需要输入的数据节点
    VectorToList();
    ChangeValueToRefence(m_listInputWhere);
    TADD_FUNC("Finish.CheckPK=[%d]",m_stSqlStruct.bCheckInsertPriKey);
    return iRet;
}

/******************************************************************************
* 函数名称	:  BuildDelete
* 函数描述	:  构建delete 解析结构
* 输入		:  pToken - 表名的token，pstExpr - where 条件组
* 输出		:  无
* 返回值	:  无
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::BuildDelete(ST_EXPR_LIST * pHint, ST_EXPR * pFirst,ST_EXPR * pstWhereExpr,ST_EXPR * pLimit,ST_EXPR * pOffset)
{
    TADD_FUNC("Start,pstExpr=[%p]",pstWhereExpr);
    int iRet = 0;
    m_stSqlStruct.iSqlType = TK_DELETE;
    m_stSqlStruct.pWhere = pstWhereExpr;
	m_stSqlStruct.pHintList = pHint;
	
    CHECK_RET_FILL(FillFirstAndLimit( pFirst, pLimit,pOffset),ERR_SQL_INVALID,"FillFirstAndLimit failed.");//
    CHECK_RET_FILL(RemoveTableAlias(),ERR_SQL_INVALID,"RemoveTableAlias failed");//去除表别名
	CHECK_RET_FILL(FillHintInfo(),ERR_SQL_INVALID,"FillHintInfo Failed.");
	//CHECK_RET_FILL(FillSequenceValue(),ERR_SQL_INVALID,"FillSequenceValue failed");       
    CHECK_RET_FILL(CollectRouterID(m_stSqlStruct.pstRouterID),ERR_SQL_FILL_MDB_INFO,"CollectRouterID faild.");//搜集路由信息
    CHECK_RET_FILL(FillMdbInfo(),ERR_SQL_FILL_MDB_INFO,"FillMdbInfo error.");//填充MDB相关信息
    CHECK_RET_FILL(CollectWhereIndex(),ERR_TAB_INDEX_NOT_EXIST,"CollectWhereIndex failed");//搜集where中的index
    CHECK_RET_FILL(m_tSqlTypeCheck.FillValidType(&m_stSqlStruct),ERR_SQL_INVALID,"FillValidType failed..");
    CHECK_RET_FILL(CollectPrimaryKeyFromTable(),ERR_SQL_INVALID,"CollectPrimaryKeyFromTable failed.");
    CollectOutputValue();//输出值
    CollectInputValue();//搜集需要输入的数据节点
    VectorToList();
    ChangeValueToRefence(m_listInputWhere);
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  BuildUpdate
* 函数描述	:  构建update解析结构
* 输入		:  pToken - 表名的token，pCollist - set 值设置组，pstExpr - where 条件组
* 输出		:  无
* 返回值	:  无
* 作者		:  jin.shaohua
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
    CHECK_RET_FILL(RemoveTableAlias(),ERR_SQL_INVALID,"RemoveTableAlias failed");//去除表别名
	CHECK_RET_FILL(FillHintInfo(),ERR_SQL_INVALID,"FillHintInfo Failed.");
	//CHECK_RET_FILL(FillSequenceValue(),ERR_SQL_INVALID,"FillSequenceValue failed");   
    CHECK_RET_FILL(CheckSetList(stSetList),ERR_SQL_INVALID,"CheckSetList failed.");
    CHECK_RET_FILL(CollectRouterID(m_stSqlStruct.pstRouterID),ERR_SQL_FILL_MDB_INFO,"CollectRouterID faild.");//搜集路由信息
    CHECK_RET_FILL(FillMdbInfo(),ERR_SQL_FILL_MDB_INFO,"FillMdbInfo error.");//填充MDB相关信息
    CHECK_RET_FILL(CollectWhereIndex(),ERR_TAB_INDEX_NOT_EXIST,"CollectWhereIndex failed");//搜集where中的index
    //CHECK_RET_FILL(SpanUpdateCollumnList(m_stSqlStruct.pstIdList),ERR_SQL_FILL_MDB_INFO,"SpanUpdateCollumnList failed.");
    CHECK_RET_FILL(CollectChangeIndex(m_stSqlStruct.pstIdList),ERR_SQL_FILL_MDB_INFO,"CollectChangeIndex error.");
    CHECK_RET_FILL(m_tSqlTypeCheck.FillValidType(&m_stSqlStruct),ERR_SQL_INVALID,"FillValidType failed..");
    CHECK_RET_FILL(CollectPrimaryKeyFromSQL(m_stSqlStruct.pstIdList),ERR_SQL_INVALID,"CollectPrimaryKey failed.");
    if(m_listOutputPriKey.vMemValue.size()!=0)
    {
        CHECK_RET_FILL(-1,ERR_SQL_INVALID,"can't update primary key");
    }
    CHECK_RET_FILL(CollectPrimaryKeyFromTable(),ERR_SQL_INVALID,"CollectPrimaryKeyFromTable failed.");
    CollectOutputValue();//待输出的值
    CollectInputValue();//搜集需要输入的数据节点
    VectorToList();
    ChangeValueToRefence(m_listInputWhere);
    TADD_FUNC("Finish");
    return iRet;
}


/******************************************************************************
* 函数名称	:  CollectOutputValue
* 函数描述	:  搜集需要输出的值
* 输入		:  无
* 输出		:  无
* 返回值	:  无
* 作者		:  jin.shaohua
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
            {//错误
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
* 函数名称	:  CollectInputValue
* 函数描述	:  搜集需要输入的值
* 输入		:  无
* 输出		:  无
* 返回值	:  无
* 作者		:  jin.shaohua
*******************************************************************************/
void TMdbSqlParser::CollectInputValue()
{
    TADD_FUNC("Start.");
    ST_WALKER stWalker;
    stWalker.xExprCallback = FillArrayInput;
    stWalker.pMdbSqlParser = this;
    stWalker.iInputType  = WRC_InputCollist;
    TWalker::WalkExprList(&stWalker,m_stSqlStruct.pColList);
    TWalker::WalkExpr(&stWalker,m_stSqlStruct.pstRouterID);//搜集路由信息
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
* 函数名称	:  ExecuteWhere
* 函数描述	:  计算where条件值
* 输入		:  无
* 输出		:  bResult - 计算结果true/false
* 返回值	:  0 - 计算成功，!0 - 计算失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ExecuteWhere(bool & bResult)
{
    TADD_FUNC("Start");
    int iRet = 0;
    if(NULL == m_stSqlStruct.pWhere)
    {//没有where 条件
        TADD_DETAIL("where is NULL");
        bResult = true;
    }
    else
    {//计算where条件
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
* 函数名称	:  ExecuteSQL
* 函数描述	:  按语法树进行运算
* 输入		:  无
* 输出		:  无
* 返回值	:  0
* 作者		:  jin.shaohua
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
	            {//列相同
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
* 函数名称	:  FillMdbInfo
* 函数描述	:  向表达式树上填充MDB相关信息(列信息)
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功，!0- 失败
* 作者		:  jin.shaohua
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
* 函数名称	:  FillIdList
* 函数描述	:  填充IDlist，获取ID所对应的列信息,
* 输入		:  ST_ID_LIST * pIdList - 待填充的idlist
* 输出		:  无
* 返回值	:  0 - 成功，!0- 失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillIdList(ST_ID_LIST * pIdList)
{
    TADD_FUNC("Start,pIdList=[%p]",pIdList);
    int iRet = 0;
    if(NULL == pIdList || pIdList->iIdNum == 0) return iRet;

    int i = 0;
    if(pIdList->iIdNum != m_stSqlStruct.pColList->iItemNum)
    {//insert  ,update 列和值不对应
        CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"column num[%d] != values num[%d]",pIdList->iIdNum,
            m_stSqlStruct.pColList->iItemNum);
    }
    for(i = 0;i<pIdList->iIdNum;i++)
    {//填充列信息和列的值
        ID_LIST_ITEM & item = pIdList->pstItem[i];
        TADD_DETAIL("column name=[%s]",item.zName);
        item.pColumn = m_tSqlParserHelper.GetMdbColumn(m_stSqlStruct.pMdbTable,item.zName);//获取列信息
        if(NULL == item.pColumn)
        {
            CHECK_RET_FILL(ERR_TAB_COLUMN_NOT_EXIST,ERR_TAB_COLUMN_NOT_EXIST,"not find column[%s]...",item.zName);
        }
        item.pExpr = m_stSqlStruct.pColList->pExprItems[i].pExpr;//一个列名对应一个列值

        CHECK_OBJ(item.pExpr);    
        CHECK_RET(TranslateBlobColumnValue(item.pExpr,item.pColumn),"Translate value BlobColumn[%s] failed.",item.pColumn->sName);
        item.pExpr->pExprValue->pColumnToSet = item.pColumn;//设置这个ID的值所对应的列
    }
    TADD_FUNC("Finish.");
    return iRet;
}


/******************************************************************************
* 函数名称	:  FillExprColumn
* 函数描述	:  向表达式中填充列信息
* 输入		:  pstWalker - 遍历类，pstExpr 待填充Expr
* 输出		:  无
* 返回值	:  0 - 成功，!0- 失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillExprColumn(ST_WALKER * pstWalker,ST_EXPR * pstExpr)
{
    TADD_FUNC("Start.pstWalker=[%p],pstExpr=[%p]",pstWalker,pstExpr);
    if(NULL == pstExpr || NULL == pstWalker) return WRC_Continue;
    TMdbSqlParser * pMdbSqlParser = pstWalker->pMdbSqlParser;
    if(TK_ID == pstExpr->iOpcode)
    {//列名
        TMdbColumn * pColumn  = NULL;
        TADD_DETAIL("Column = [%s].",pstExpr->sTorken);
        pColumn = pMdbSqlParser->m_tSqlParserHelper.GetMdbColumn(pMdbSqlParser->m_stSqlStruct.pMdbTable,pstExpr->sTorken);
        if(NULL == pColumn)
        {//没有找到列信息
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
    {//检测表名或者表的别名
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
* 函数名称	:  FillArrayInput
* 函数描述	:  填充需要输入的信息节点
* 输入		:  pstWalker - 遍历类，pstExpr - 待填充的节点
* 输出		:  无
* 返回值	:  0 - 成功，!0- 失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillArrayInput(ST_WALKER * pstWalker,ST_EXPR * pstExpr)
{
    TADD_FUNC("Start.pstWalker=[%p],pstExpr=[%p]",pstWalker,pstExpr);
    if(NULL == pstExpr || NULL == pstWalker) return WRC_Continue;
    TMdbSqlParser * pMdbSqlParser = pstWalker->pMdbSqlParser;
    std::vector<_ST_MEM_VALUE *> * pInputArray = NULL;
    switch(pstWalker->iInputType)
    {//判断输入队列类型
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
        pInputArray = &(pMdbSqlParser->m_listInputLimit.vMemValue);//理论上limit中不可以含有列信息
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
    case TK_VARIABLE://绑定变量
        {        
            if(NULL == pstExpr->pExprValue->sAlias)
            {
                pstExpr->pExprValue->sAlias = pstExpr->sTorken +1;//去掉:
            }
            std::vector<ST_MEM_VALUE *>::iterator itor  = pMdbSqlParser->m_listInputVariable.vMemValue.begin();
            for(;itor != pMdbSqlParser->m_listInputVariable.vMemValue.end();++itor)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp((*itor)->sAlias,pstExpr->pExprValue->sAlias) == 0 )
                {//有重复的绑定变量名
                    pMdbSqlParser->m_tError.FillErrMsg(ERR_SQL_INVALID,"Bind Param[%s] is duplicate",pstExpr->pExprValue->sAlias);
                    return WRC_Abort;
                }
            }
            pstExpr->pExprValue->iFlags |= MEM_Variable;
            pMdbSqlParser->m_listInputVariable.vMemValue.push_back(pstExpr->pExprValue);
            if(NULL == pstExpr->pExprValue->pColumnToSet )
            {//先根据绑定遍历名获取可能的需要设置的变量信息给回滚段使用
                pstExpr->pExprValue->pColumnToSet = pMdbSqlParser->m_tSqlParserHelper.GetMdbColumn(pMdbSqlParser->m_stSqlStruct.pMdbTable,pstExpr->pExprValue->sAlias);
            }
        } 
        break;
    case TK_ID://列名
        if(NULL == pstExpr->pExprValue->sAlias)
        {
            pstExpr->pExprValue->sAlias = pstExpr->sTorken;
        }
        pInputArray->push_back(pstExpr->pExprValue);
        break;
    case TK_FUNCTION://聚合函数的临时值
        {
            if(FuncHasProperty(pstExpr->pFunc->pFuncDef,FUNC_AGGREAGATE))
            {//是聚合函数
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
* 函数名称	:  CollectWhereIndex
* 函数描述	:  搜集where条件中的可以使用index 以及全量遍历索引
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功，!0- 失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::CollectWhereIndex()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_RET_FILL(m_MdbIndexCtrl.AttachTable(m_stSqlStruct.pShmDSN,m_stSqlStruct.pMdbTable),
        ERR_DB_NOT_CONNECTED,
        "m_MdbIndexCtrl.AttachTable failed....");//搜集table所对应的index信息

	m_stSqlStruct.pScanAllIndex = m_pHintIndex;
	if(NULL == m_stSqlStruct.pScanAllIndex)
	{
		m_stSqlStruct.pScanAllIndex = m_MdbIndexCtrl.GetScanAllIndex();//获取遍历所有的index
	}

    if(NULL != m_stSqlStruct.pWhere)
    {
        CHECK_RET_FILL(OptimizeWhereClause(m_stSqlStruct.pWhere,m_stSqlStruct.tWhereOrClause),ERR_SQL_INVALID, "Optimize Where failed.");//优化where 条件
        CHECK_RET_FILL(GetWhereIndex(m_stSqlStruct.vIndexUsed,m_stSqlStruct.tWhereOrClause),ERR_SQL_INVALID,"Get Where Index failed.");//搜集索引
        CHECK_RET_FILL(CheckLPM(m_stSqlStruct.pWhere, m_stSqlStruct.vIndexUsed),ERR_SQL_INVALID, "CheckLPM failed.");//检测where语句中最长匹配索引
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
			"m_MdbIndexCtrl.AttachTable failed....");//搜集table所对应的index信息
	
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
* 函数名称	:  GetWhereIndex
* 函数描述	:  获取所使用的index值
* 输入		:  无
* 输出		:  vIndex - 获取到的索引信息
* 返回值	:  0 - 成功，!0- 失败
* 作者		:  jin.shaohua
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
    pVIndex = &(m_stSqlStruct.vIndexUsed);//返回
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  OptimizeWhereClause
* 函数描述	: 优化where 条件,将where条件全部转化成(A and B and c) or (A and C and D)的形式
* 输入		:  
* 输出		:  
* 返回值	:   0 - 成功，!0- 失败
* 作者		:  jin.shaohua
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
    case TK_AND://组合
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
    case TK_OR: //两个or 合成一个or
        {
            tWhereOrClause.m_vAndClause.insert(tWhereOrClause.m_vAndClause.end(),tLeftWOrC.m_vAndClause.begin(),tLeftWOrC.m_vAndClause.end());
            tWhereOrClause.m_vAndClause.insert(tWhereOrClause.m_vAndClause.end(),tRightWOrC.m_vAndClause.begin(),tRightWOrC.m_vAndClause.end());
        }
        break;
    default://其他当成最小粒子处理
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
* 函数名称	:  GetWhereIndex
* 函数描述	:  获取where条件中的索引信息
* 输入		:  pstExpr - where 条件组
* 输出		:  vIndex - 获取到的索引信息
* 返回值	:   0 - 成功，!0- 失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::GetWhereIndex(std::vector< ST_INDEX_VALUE > & vIndexValue,ST_WHERE_OR_CLAUSE & tWhereOrClause)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //查询各个where子中的索引
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
        {//有一段没有索引，就都不走索引
            vIndexValue.clear();
            break;
        }
    }
    TADD_FUNC("Finish.vIndex.size=[%d],vIndexColumn.size=[%d]",vIndexValue.size(),vIndexColumn.size());
    return iRet;
}

//最长匹配不进行相等判断，需要做特殊处理
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
* 函数名称	:  GetSubWhereIndex
* 函数描述	:  获取子where条件中的index
* 输入		:  pstExpr - where 条件组
* 输出		:  vIndex - 获取到的索引信息
* 返回值	:   0 - 成功，!0- 失败
* 作者		:  jin.shaohua
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
    case TK_AND://and 去除重复的索引可能列
    case TK_OR://在or 条件下，整合索引
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"expr.opcode can not be [and]/[or].");
        }
    case TK_EQ:// = 只有=号才能有索引 a = 1或者a=:a 这样的形式才有效
        {
            ST_TABLE_INDEX_INFO * pTableIndex = NULL;
            ST_EXPR * pExpr = NULL;
            int iColNoPos = 0;
            if(TK_ID == pstExpr->pLeft->iOpcode && false == m_tMdbExpr.IsContainColumn(pstExpr->pRight)/*TK_ID!= pstExpr->pRight->iOpcode*/)
            {//左枝
                pTableIndex = 
                    m_MdbIndexCtrl.GetIndexByColumnPos(pstExpr->pLeft->pExprValue->pColumn->iPos,iColNoPos);
                pExpr = pstExpr->pRight;//保存右值
                pExpr->pExprValue->pColumnToSet = pstExpr->pLeft->pExprValue->pColumn;
            }
            else if(TK_ID == pstExpr->pRight->iOpcode && false == m_tMdbExpr.IsContainColumn(pstExpr->pLeft)/*TK_ID!= pstExpr->pLeft->iOpcode*/)
            {//右枝
                pTableIndex = 
                    m_MdbIndexCtrl.GetIndexByColumnPos(pstExpr->pRight->pExprValue->pColumn->iPos,iColNoPos);
                pExpr = pstExpr->pLeft;//保存右值
                pExpr->pExprValue->pColumnToSet = pstExpr->pRight->pExprValue->pColumn;
            }
            if(NULL != pTableIndex && NULL != pExpr)
            {//是索引列
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
            //TODO:检测对于另一枝中的不能包含TK_ID类型的节点
        }
    case TK_IN://in关键字
        {//只有当左枝为column列时
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
                            {//in 列表中不可包含列
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
    case TK_ID:// 列名
        break;
    default:
        break;	
    }
    TADD_FUNC("Finish.,vIndexColumn.size=[%d]",vIndexColumn.size());
    return iRet;
}

#if 0
/******************************************************************************
* 函数名称	:  RemoveDupIndex
* 函数描述	:  移除重复的索引
* 输入		:  vLeftIndex 
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::RemoveDupIndex(std::vector< ST_INDEX_VALUE > & vIndex)
{
    TADD_FUNC("Start.vIndex.size[%d]",vIndex.size());
    int iRet = 0;
    if(vIndex.size() < 2){return 0;}//直接返回
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
* 函数名称	:  SpanInsertCollumnList
* 函数描述	:  扩充insert column
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
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
            {//找到
                m_listOutputCollist.vMemValue.push_back(pExistList->pstItem[j].pExpr->pExprValue);
                bIsExist = true;
                break;
            }
        }
        if(bIsExist){continue;}//若找到continue

        if(pColumn->bIsDefault)
        {//有默认值
            TADD_DETAIL("column[%s] has default value[%s]",pColumn->sName,pColumn->iDefaultValue);
            ST_MEM_VALUE * pMemValue = QMDB_MALLOC->AllocMemValue(NULL);
            CHECK_RET_FILL(m_tSqlTypeCheck.ConverType(pColumn,pMemValue),0,"ConverType error.");
            pMemValue->pColumnToSet = pColumn;
            //设置默认值
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
                    {//blob型需要转化
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
            m_arrOtherMemValue.push_back(pMemValue);//保存以防止内存泄露
        }
        else if(pColumn->m_bNullable)
        {
            TADD_DETAIL("column[%s] has default value[%s]",pColumn->sName,pColumn->iDefaultValue);
            ST_MEM_VALUE * pMemValue = QMDB_MALLOC->AllocMemValue(NULL);
            CHECK_RET_FILL(m_tSqlTypeCheck.ConverType(pColumn,pMemValue),0,"ConverType error.");
            pMemValue->pColumnToSet = pColumn;
            //设置默认值null
            pMemValue->SetNull();
            m_listOutputCollist.vMemValue.push_back(pMemValue);
            m_arrOtherMemValue.push_back(pMemValue);//保存以防止内存泄露
        }
        else
        {//没有默认值，错误
            CHECK_RET_FILL(ERR_TAB_COLUMN_VALUE_INVALID,ERR_TAB_COLUMN_VALUE_INVALID,
                "column[%s] has no default value...",pColumn->sName);
        }

    }
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* 函数名称	:  SpanUpdateCollumnList
* 函数描述	:  扩充update column,采集需要输出的值列表
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
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
* 函数名称	:  CleanOtherMem
* 函数描述	:  清理其它内存数据，防止内存泄露
* 输入		:  无
* 输出		:  无
* 返回值	:  无
* 作者		:  jin.shaohua
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
* 函数名称	:  SpanTKALLCollumnList
* 函数描述	:  扩展* 字段支持 *,还不支持a.*
* 输入		:  无
* 输出		:  无
* 返回值	:  无
* 作者		:  jin.shaohua
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
* 函数名称	:  ClearMemValue
* 函数描述	:  清理数据,只是清理数据，并不做内存清理
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
        MemValueClearProperty(pstMemValue,MEM_Null);//去除null标记
        */
    }
    return;
}

/******************************************************************************
* 函数名称	:  CollectChangeIndex
* 函数描述	:  获取update 或者insert中所变更的索引，并保存其 的值，
对于组合索引，若有一部分没有变更，则手动添加MemValue让内核自动填充
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
        //if(NULL == pTableIndex){continue;}//不是索引
        //查看下vIndexUpdate中是否已经有记录
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
    //填充更新的组合索引中一部分值在内核中(不在set中)
    for(i = 0;i <(int) m_stSqlStruct.vIndexChanged.size();i++)
    {
        ST_INDEX_VALUE & stIndexValue = m_stSqlStruct.vIndexChanged[i];
        if(HT_CMP ==  stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType)
        {
            for(j = 0;j < MAX_INDEX_COLUMN_COUNTS;j++)
            {
                int iColumnPos = stIndexValue.pstTableIndex->pIndexInfo->iColumnNo[j];
                if(iColumnPos >= 0 && NULL == stIndexValue.pExprArr[j])
                {//该值在内存中
                    ST_EXPR * pTempExpr = QMDB_MALLOC->AllocExpr();
                    pTempExpr->pExprValue = QMDB_MALLOC->AllocMemValue(NULL);
                    pTempExpr->pExprValue->pColumn = &(m_stSqlStruct.pMdbTable->tColumn[iColumnPos]);
                    m_tSqlTypeCheck.ConverType(pTempExpr->pExprValue->pColumn,pTempExpr->pExprValue);
                    m_listInputCollist.vMemValue.push_back(pTempExpr->pExprValue);
                    m_arrOtherExpr.push_back(pTempExpr);//防止内存泄露
                    stIndexValue.pExprArr[j] = pTempExpr;
                }
            }
        }
    }
    TADD_FUNC("Finish.vIndexChanged.size=[%d]",m_stSqlStruct.vIndexChanged.size());
    return iRet;
}



/******************************************************************************
* 函数名称	:  ExecuteOrderby
* 函数描述	:  计算orderby 的值
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ExecuteOrderby()
{
    // m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pOrderby,CALC_PER_Row);
    return m_tMdbExpr.CalcExprList(m_stSqlStruct.pOrderby);
}


/******************************************************************************
* 函数名称	:  ExecuteGroupby
* 函数描述	:  计算Group by 的值
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ExecuteGroupby()
{
    //m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pGroupby,CALC_PER_Row);
    return m_tMdbExpr.CalcExprList(m_stSqlStruct.pGroupby);
}
/******************************************************************************
* 函数名称	:  ExecuteHaving
* 函数描述	:  计算having的值
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ExecuteHaving()
{
    //m_tMdbExpr.ResetCalcFlagByCalcLevel(m_stSqlStruct.pHaving,CALC_PER_Row);
    return m_tMdbExpr.CalcExpr(m_stSqlStruct.pHaving);
}


/******************************************************************************
* 函数名称	:  FillOrderbyInfo
* 函数描述	: 填充orderby 信息  
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillOrderbyInfo(ST_EXPR_LIST * pExprList)
{
    TADD_FUNC("Start.pExprList=[%p]",pExprList);
    int iRet = 0;
    int i = 0;
    if(NULL == pExprList){return 0;}
    if(pExprList->iItemNum > MAX_ORDERBY_COUNT)
    {//orderby 列太多
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
* 函数名称	:  ExecuteLimit
* 函数描述	:  计算limit值
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
* 函数名称	:  ChangeValueToRefence
* 函数描述	:  将memvalue中的值改成引用值,优化使其不需要从内存中拷贝出来
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::ChangeValueToRefence(ST_MEM_VALUE_LIST & tListMemValue)
{
    int iRet = 0;
    std::vector<ST_MEM_VALUE *>::iterator itor = tListMemValue.vMemValue.begin();
    for(;itor !=  tListMemValue.vMemValue.end();++itor)
    {
        ST_MEM_VALUE * pMemValue = *itor;
        if(MemValueHasProperty(pMemValue,MEM_Dyn))
        {//对于动态分配的字符值改成，去除动态分配
            MemValueClearProperty(pMemValue,MEM_Dyn);
            SAFE_DELETE_ARRAY(pMemValue->sValue);
            pMemValue->iSize = -1;
        }
    }
    return iRet;
}




/******************************************************************************
* 函数名称	:  VectorToList
* 函数描述	:  为了性能
* 输入		:  
* 输出		:  
* 返回值	: 
* 作者		:  jin.shaohua
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

    m_listInputGroupBy.VectorToList();//需要输入的group by
    m_listOutputGroupBy.VectorToList();//需要输出的group by
    m_listInputHaving.VectorToList();
    m_listOutputHaving.VectorToList();//

}
/******************************************************************************
* 函数名称	:  GenRollbackSql
* 函数描述	:  生成回滚的sql语句
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::GenRollbackSql(char *  sNewSql)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int i = 0;
    //按主键拼接where 语句
    char sWhere[1024] = {0};
    TMdbPrimaryKey & tPriKey = m_stSqlStruct.pMdbTable->m_tPriKey;
    TMdbTable  * pTable = m_stSqlStruct.pMdbTable;
    for(i = 0;i < tPriKey.iColumnCounts;i++)
    {
        sprintf(sWhere+strlen(sWhere)," %s=:%s and",pTable->tColumn[tPriKey.iColumnNo[i]].sName,
            pTable->tColumn[tPriKey.iColumnNo[i]].sName);
    }
    sWhere[strlen(sWhere)-3] = '\0';//去除最后一个'and'
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
            sColumn[strlen(sColumn) -1] = '\0';//去除最后一个，
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
                {//使用增量回滚
                    sprintf(sSet+strlen(sSet)," %s =%s +:%s,",pColumnToSet->sName,pColumnToSet->sName,pColumnToSet->sName);
                }
                else
                {//非数值型使用全量回滚
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
* 函数名称	:  CollectPrimaryKey
* 函数描述	:  从SQL中搜集主键信息
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
            //同时搜集对于主机列值给内核去填充，用于主键重复性校验
            ST_MEM_VALUE * pNewMemValue = QMDB_MALLOC->AllocMemValue(NULL);
            pNewMemValue->pColumn = pstMemValue->pColumnToSet;
            m_tSqlTypeCheck.ConverType(pNewMemValue->pColumn,pNewMemValue);
            m_listInputPriKey.vMemValue.push_back(pNewMemValue);
            m_arrOtherMemValue.push_back(pNewMemValue);//保存以防止内存泄露
        }
    }
    TADD_FUNC("Finish m_listInputPriKey.size[%d]",m_listInputPriKey.vMemValue.size());
    return iRet;
}

/******************************************************************************
* 函数名称	:  CollectPrimaryKeyFromTable
* 函数描述	:  从表中获取主键信息
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
        m_arrOtherMemValue.push_back(pNewMemValue);//保存以防止内存泄露
    }
    TADD_FUNC("Finish,m_listInputPriKey.size(%d)",m_listInputPriKey.vMemValue.size());
    return iRet;
}

/******************************************************************************
* 函数名称	:  AddColumn
* 函数描述	:  添加列
* 输入		:  pToken - 列名
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
        //检查增加的列是否已经存在
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
        //开始增加列
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
* 函数名称	:  AddColumnAttribute
* 函数描述	:  添加列属性
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
* 函数名称	:  BuildCreateTable
* 函数描述	:  构建create table语法
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::BuildCreateTable(int iIFNE,Token * pTableName)//构建createtable
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
		//检查密码设置的有效性
		iRtnValue = CheckPasswordValid(sPassword);
		CHECK_RET_FILL_BREAK(iRtnValue,iRtnValue,"the password is invalid");
        //对用户密码进行加密处理
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
    //检查表空间是否存在
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
    //检查表空间是否存在
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
    //检查表空间是否存在
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
        //使用m_tAttr数组的前两个对象记录修改的属性名
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
    //检查表空间是否存在
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
        //密码需要加密处理
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
* 函数名称	:  AddTableAttribute
* 函数描述	:  添加表属性
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
* 函数名称	:  AddTablePrimayKey
* 函数描述	:  添加表的主键信息
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
* 函数名称	:  BuildCreateIndex
* 函数描述	:  构建创建索引
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
        {//索引已满
            CHECK_RET_FILL_BREAK(ERR_SQL_INVALID,ERR_SQL_INVALID,"index counts[%d] is full",MAX_INDEX_COUNTS);
        }
        int i = 0;
        for(i = 0;i< pExistTable->iIndexCounts;i++)
        {
            //检测索引是否已存在,注:没有做(字段相同，名不同)的检测
            if(TMdbNtcStrFunc::StrNoCaseCmp(pExistTable->tIndex[i].sName,sIndexName)== 0)
            {
                //已存在
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
        //计算索引hash类型
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
        //索引优先级默认为1
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

        //校验主键上的列数不能超过10列
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
        //使用sModifyColumnAttr暂时报错删除的列名
        m_pDDLSqlStruct->vModifyColumnAttr.push_back(sColumnName);
        if(NULL == m_pMdbConfig)
        {
            SAFESTRCPY(m_pDDLSqlStruct->pTable->sTableName,\
                sizeof(m_pDDLSqlStruct->pTable->sTableName),sTableName);
            break;
        }
        //动态下不支持删除列操作
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
        //静态下执行，默认修改的是第一列
        if(NULL == m_pMdbConfig)
        {
            m_pDDLSqlStruct->iModifyColumnPos = 0;
            SAFESTRCPY(m_pDDLSqlStruct->pTable->tColumn[0].sName, \
                sizeof(m_pDDLSqlStruct->pTable->tColumn[0].sName),sColumnName);
            break;
        }

        //下面是动态执行
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
            {//离线
                tColumn.iDataType = iDataType;
            }
            else if(iDataType != tColumn.iDataType)
            {//不同的类型
                TMdbTable* pMdbTable = m_tSqlParserHelper.GetMdbTable(m_pDDLSqlStruct->pTable->sTableName);
                if(!pMdbTable)
                {
                    CHECK_RET_FILL_BREAK(-1,ERR_TAB_NO_TABLE,"not find table [%s]",m_pDDLSqlStruct->pTable->sTableName);
                }
                if(pMdbTable->iCounts <= 0)
                {//没有数据的时候
                    tColumn.iDataType = iDataType;
                }
                else if(tColumn.iDataType == DT_Int || iDataType == DT_Int)
                {//有数据的时候，整形不能与其他类型互相转换
                    CHECK_RET_FILL_BREAK(-1,ERR_SQL_DATA_TYPE_INVALID,\
                        "Do not modify column[%s] data type[%s] dynamicly with records.",
                        tColumn.sName,sAttr);
                }
                else
                {//不为整形可以修改
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
* 函数名称	:  BuildLoadData
* 函数描述	:  上载某种表的数据
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  CollectIndexForVerifyPK
* 函数描述	:  获取用于检测主键的索引列
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::CollectIndexForVerifyPK(ST_ID_LIST * pIdList)
{
    TADD_FUNC("Start.pIdList=[%p]",pIdList);
    int iRet = 0;
    CHECK_OBJ_RET_0(pIdList);
    m_stSqlStruct.stIndexForVerifyPK.pstTableIndex  = m_MdbIndexCtrl.GetVerfiyPKIndex();
    if(NULL == m_stSqlStruct.stIndexForVerifyPK.pstTableIndex)
    {//主键上没有索引
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
            {//获取值
                m_stSqlStruct.stIndexForVerifyPK.pExprArr[j] = pIdList->pstItem[i].pExpr;
            }
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* 函数名称	:  IsFullIndex
* 函数描述	:  是否都是完整的索引组
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
* 函数名称	:  IsFullIndex
* 函数描述	:  是否都是完整的索引
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
bool TMdbSqlParser::IsFullIndex( ST_INDEX_VALUE & stIndexValue)
{
    if(HT_CMP == stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType)
    {// 组合索引
        int i = 0;
        for(i = 0;i<MAX_INDEX_COLUMN_COUNTS;i++)
        {
            if(stIndexValue.pstTableIndex->pIndexInfo->iColumnNo[i]>=0 &&
                NULL == stIndexValue.pExprArr[i])
            {//不完整
                TADD_FLOW("CMP_Index[%s] is not complete.",stIndexValue.pstTableIndex->pIndexInfo->sName);
                return false;
            }
        }
    }
    return true;
}

/******************************************************************************
* 函数名称	:  CheckSetList
* 函数描述	:  检测update的set列表
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::CheckSetList(ST_SET_LIST & stSetList)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int i = 0;
    for(i = 0;i < stSetList.pExprList->iItemNum;++i)
    {
        ST_EXPR * pstExpr = stSetList.pExprList->pExprItems[i].pExpr;
        //set 列表中不能包含and 和or
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
* 函数名称	:  RemoveTableAlias
* 函数描述	:  去除表别名
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
* 函数名称	:  IsTableAlias
* 函数描述	:  判断是否是表的别名
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
				pDsn->iNtcPort[i] = TMdbNtcStrFunc::StrToInt(sTempPort);//代理端口
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
				pDsn->iNoNtcPort[i] = TMdbNtcStrFunc::StrToInt(sTempPort);//代理端口
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
					pDsn->iAgentPort[i] = TMdbNtcStrFunc::StrToInt(sTempPort);//代理端口
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
* 函数名称	:  BuildAddSequence
* 函数描述	:  构造新增序列结构
* 输入		:  pTableName 表名
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  BuildDelSequence
* 函数描述	:  构造删除序列结构
* 输入		:  pTableName 表名
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  BuildAlterSequence
* 函数描述	:  构造更新序列结构
* 输入		:  pTableName 表名
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  AddSequenceValue
* 函数描述	:  获取增加序列或删除序列的列值
* 输入		:  pColumnName 列名，pValue 列值
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  CollectRouterID
* 函数描述	:  采集routerID信息
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
        {//有路由信息列
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
* 函数名称	:  IsCurRowNeedCapture
* 函数描述	:  当前行是否需要捕获
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
bool TMdbSqlParser::IsCurRowNeedCapture()
{
    if(NULL == m_stSqlStruct.pstRouterID){return false;}//没有路由ID
    if(TK_SELECT == m_stSqlStruct.iSqlType){return false;}//select 不捕获
   // if(m_iSourceId != SOURCE_APP){return false;}//不需要落地，可能是备机同步过来的
    if(false == m_stSqlStruct.pShmDSN->GetInfo()->m_bIsCaptureRouter){return false;}//不需要捕获
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
* 函数名称	:  BuildCreateJob
* 函数描述	:  构造新增job
* 输入		:  pJobName job名
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  BuildAlterJob
* 函数描述	:  构造修改job
* 输入		:  pJobName job名
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  BuildRemoveJob
* 函数描述	:  构造删除job
* 输入		:  pJobName job名
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  AddJobValue
* 函数描述	:  获取job属性和值
* 输入		:  pAttrName job属性，pAttrValue 列值
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  BuildAddFlushSQLParam
* 函数描述	:  设置flush-sql or load-sql中绑定参数的信息
* 输入		:  
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
        //最多增加10个参数
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
* 函数名称	:  AddFlushSQLorLoadSQLParamAttr
* 函数描述	:  填充新增parameter参数的属性
* 输入		:  
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  BuildModifyFlushSQLParam
* 函数描述	:  设置修改表指定的参数
* 输入		:  
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  ModifyFlushSQLorLoadSQLParamAttr
* 函数描述	:  填充修改参数的属性
* 输入		:  
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
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
* 函数名称	:  TranslateBlobColumnValue
* 函数描述	:  转换sql中的blob值
* 输入		:  
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
                //转换
                std::string encoded = Base::base64_encode(reinterpret_cast<const unsigned char*>(pExpr->pExprValue->sValue),strlen(pExpr->pExprValue->sValue));
                QMDB_MALLOC->ReleaseStr(pExpr->pExprValue->sValue);
                pExpr->pExprValue->sValue = QMDB_MALLOC->CopyFromStr(encoded.c_str());
                pExpr->pExprValue->iSize  = strlen(pExpr->pExprValue->sValue) +1;
                MemValueSetProperty(pExpr->pExprValue,MEM_Str|MEM_Dyn);
            }
            break;
        case TK_ID: //列和绑定变量无需转换         
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
* 函数名称	:  FillFirstAndLimit
* 函数描述	:  填充first limit
* 输入		:  
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::FillFirstAndLimit(ST_EXPR * pFirst,ST_EXPR * pLimit,ST_EXPR * pOffset)
{
    int iRet = 0;
    if(NULL != pFirst && NULL != pLimit)
    {//first 和limit 只能有一个
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
	//order by中的列, 必须出现在 group by 后面
	if(m_stSqlStruct.pOrderby != NULL)
	{
	    for(int i = 0; i < m_stSqlStruct.pOrderby->iItemNum ;++i)
	    {
	        ST_EXPR * pstSelExpr =  m_stSqlStruct.pOrderby->pExprItems[i].pExpr;
	        if(NULL == pstSelExpr->pExprValue || NULL == pstSelExpr->pExprValue->pColumn){continue;}
	        bool bMatch = false;//没找到
	        int j = 0;
	        for(j = 0;j < m_stSqlStruct.pGroupby->iItemNum;++j)
	        {  
	            ST_EXPR * pstExpr =  m_stSqlStruct.pGroupby->pExprItems[j].pExpr;
	            if(pstSelExpr->pExprValue->pColumn == pstExpr->pExprValue->pColumn)
	            {//列相同
	                bMatch = true;
	                break;
	            }
	        }
	        if(false == bMatch)
	        {//group by 和order by 列表不匹配
	            CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"Column[%s] in order by ,but not in group by.",
	                pstSelExpr->pExprValue->pColumn->sName);
	        }
	    }
	}
	return iRet;
}
/******************************************************************************
* 函数名称	:  BuildGroupBy
* 函数描述	:  构建group by子句,
group by 有一个原则,就是 select 后面的所有列中,没有使用聚合函数的列,
必须出现在 group by 后面
* 输入		:  
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSqlParser::BuildGroupBy()
{
    int iRet = 0;
    if(NULL == m_stSqlStruct.pGroupby && NULL != m_stSqlStruct.pHaving)
    {//having子句必须用在group by中
        CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"[having]SQL should be used with[group by]");
    }
    if(NULL == m_stSqlStruct.pGroupby){return 0;}//没有group by
    //group by 得是列
    int i = 0;
    for(i = 0;i < m_stSqlStruct.pGroupby->iItemNum;++i)
    {  
        ST_EXPR * pstExpr =  m_stSqlStruct.pGroupby->pExprItems[i].pExpr;
        if(NULL == pstExpr->pExprValue || NULL == pstExpr->pExprValue->pColumn)
        {
            CHECK_RET_FILL(ERR_SQL_INVALID,ERR_SQL_INVALID,"Group by column[%d] is error.",i);
        }
    }
    //select 后面的所有列中,没有使用聚合函数的列, 必须出现在 group by 后面
    if(m_stSqlStruct.pColList != NULL)
	{
	    for(i = 0; i < m_stSqlStruct.pColList->iItemNum ;++i)
	    {
	        ST_EXPR * pstSelExpr =  m_stSqlStruct.pColList->pExprItems[i].pExpr;
	        if(NULL == pstSelExpr->pExprValue || NULL == pstSelExpr->pExprValue->pColumn){continue;}//
	        bool bMatch = false;//没找到
	        int j = 0;
	        for(j = 0;j < m_stSqlStruct.pGroupby->iItemNum;++j)
	        {  
	            ST_EXPR * pstExpr =  m_stSqlStruct.pGroupby->pExprItems[j].pExpr;
	            if(pstSelExpr->pExprValue->pColumn == pstExpr->pExprValue->pColumn)
	            {//列相同
	                bMatch = true;
	                break;
	            }
	        }
	        if(false == bMatch)
	        {//group by 和select 列表不匹配
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
        {//离线如果存在，也不能rename
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

