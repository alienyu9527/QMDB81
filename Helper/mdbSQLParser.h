#ifndef _MDB_SQL_PARSER__H_
#define _MDB_SQL_PARSER__H_
#include "Helper/mdbExpr.h"
#include "Helper/SqlParserStruct.h"
#include "Control/mdbIndexCtrl.h"
#include "Helper/SqlParserHelper.h"
#include <vector>
#include "Walker.h"
#include "Helper/SqlTypeCheck.h"
#include "Helper/mdbErrorHelper.h"

//namespace QuickMDB{

/************************************************************************/
/* sql解析器                                                                                                     */
/************************************************************************/
class TMdbSqlParser
{
public:
	TMdbSqlParser();
	~TMdbSqlParser();
	int Init();//初始化
	int ParseSQL(const char * sSql);//解析
    int CheckDDLSQL(const char * sSql);//检查DDL,并申请内存
	int FillTableName(const Token * pToken,const Token * pTokenAlias);//填充表信息
	ST_ID_LIST * IdListAppend(ST_ID_LIST *pList, Token *pToken);//
	int AddColumn(Token *pToken);//添加列
	int AddColumnAttribute(const char * sAttrName, Token *pToken);//添加列属性
	int AddTableAttribute(Token *pAttrName, Token *pAttrValue);//添加表属性
	int AddTablePrimayKey(ST_ID_LIST * pIdList);//添加表的主键信息
	void CleanResult();
       int ClearLastExecute();//清理上一次的执行
	int BuildSelect(ST_EXPR_LIST * pHint,ST_EXPR * pFirst,int iDistinct,
									ST_EXPR_LIST * pcolList,ST_EXPR * pstWhere,
									ST_EXPR_LIST * pstGroupby,ST_EXPR * pstHaving,
									ST_EXPR_LIST * pstOrder,ST_EXPR * pLimit,ST_EXPR * pOffset);//生成select
	int BuildInsert(ST_ID_LIST * pCollist,ST_EXPR_LIST * pstExprList);//构建insert
	int BuildDelete(ST_EXPR_LIST * pHint,ST_EXPR * pFirst,ST_EXPR * pstExpr,ST_EXPR * pLimit,ST_EXPR * pOffset);//构建delete
	int BuildUpdate(ST_EXPR_LIST * pHint,ST_EXPR * pFirst,ST_SET_LIST stSetList,ST_EXPR * pstExpr,ST_EXPR * pLimit,ST_EXPR * pOffset);//构建update
	int BuildCreateIndex(int iIFNE,Token * pIndexName,Token *pTableName,ST_ID_LIST * pIdList, Token* psAlgoName, Token* pLayerLimit = NULL);//构建createindex
	int BuildCreateTable(int iIFNE,Token * pTableName);//构建createtable
	int CheckOrderByInGroupBy();
    int BuildGroupBy();//构建group by子句
	int BuildDropTable(int iIFE,Token * pTableName);
	int BuildTruncateTable(int iIFE,Token * pTableName);
	int BuildDropIndex(int iIFE,Token * pIndexName,Token * pTableName);

	int BuildCreateDsn(int iIFNE,Token * pDsnName);
	int BuildDropDsn(int iIFE,Token * pDsnName);

	int AddColumnNULLAttribute();
	int BuildCreatePrimKey(int iIFNE,Token *pTableName,ST_ID_LIST * pIdList);
	
	int BuildUser(int iIFNE,Token * pUserName,const int iSqlType);
	int CheckNumberValid(char* sPassword);
	int CheckStrValid(char* sPassword);
	int CheckPasswordValid(char* sPassword);
	int AddUserAttribute(Token * pPassword,Token * pAccess = NULL);
	int ModifyUserAttribute(Token * pPassword,Token * pAccess = NULL);
	
	int BuildAlterTableSpace(int iIFE,Token * pTablespaceName);
	int BuildCreateTableSpace(int iIFNE,Token * pTablespaceName);
	int BuildDropTableSpace(int iIFE,Token * pTablespaceName);
	int AddTablespaceAttribute(Token * pPageSize =NULL,Token * pAskPage = NULL, Token* pStorage = NULL);
	int ModifyTablespaceAttribute(Token * pPageSize,Token * pAskPage = NULL, Token* pStorage = NULL);
	
	int BuildDataSource(Token * pDataSource);
	int BuildDataSourceForMySQL(Token * pDataSource);
	int BuildDataSourceForOracle(Token * pDataSource);
	int BuildUseDsn(int iIFE,Token *pDsnName);
	int BuildUseTablespace(int iIFE,Token *pTablespaceName);
	int AddDsnAttribute(Token *pPropertyName,Token *pValue);
	int BuildAlterDsn(int iIFE,Token * pDsnName);
	
	int BuildModifyTableAttribute(int iIFNE,Token * pTableName,const char*pOptName);
	int DropTableAttribute(Token * pAttrName);
	int ModifyTableAttributeFromSql(char * sAttrName,char * sAttrValue);
	int ModifyTableAttribute(Token *pAttrName, Token *pAttrValue);
	
	int BuildAddTableColumn(int iIFNE,Token * pTableName);
	int BuildDropTableColumn(int iIFNE,Token * pTableName,Token * pColumnName);
	int BuildModifyTableColumn(int iIFNE,Token * pTableName);
	int ModifyColumn(Token * pColumnName);
	int ModifyColumnAttribute(const char * sAttrName, Token *pToken);
	int ModifyColumnNULLAttribute(const int iNullAble);
	int BuildLoadData(Token * pTableName);//构造上载某种表的数据
	int BuildAddSequence(); //构造新增序列结构
	int BuildDelSequence();//构造删除序列结构
	int BuildAlterSequence(); //构造更新序列结构
	int AddSequenceValue(Token *pColumnName,Token *pValue);//获取增加序列或删除序列的列值
		
	//开放给内核模块的接口
	int ExecuteWhere(bool & bResult);//计算where条件
	int ExecuteSQL();//按语法树进行运算
	int ExecuteOrderby();//计算orderby 的值
	int ExecuteGroupby();//计算Groupby 的值
	int ExecuteHaving();//计算having值
	int ExecuteLimit();//计算limit 的值
	int GetWhereIndex(std::vector< ST_INDEX_VALUE >* & vIndex);//计算所使用的index值
	int SetDB(TMdbShmDSN * pMdbDsn,TMdbConfig * pConfig);//设置MDB的连接信息
	int SetMDBConfig(TMdbConfig * pConfig);//设置MDBconfig
	void ClearMemValue(ST_MEM_VALUE_LIST & stMemValueList);// 只是清理数据，并不做内存清理
	int GenRollbackSql(char * sNewSql);// 生成rollback的sql;
	bool IsTableAlias(Token * pToken);//判断是否是表的别名
	bool IsCurRowNeedCapture();//当前行是否需要捕获
	int BuildCreateJob(Token * pJobName); //构造新增job
	int BuildAlterJob(Token * pJobName); //构造修改job
	int BuildRemoveJob(Token * pJobName); //构造删除job	
	int AddJobValue(Token *pAttrName,Token *pAttrValue); //添加job属性和值
	int BuildAddFlushSQLParam(int iIFNE,Token *pTableName,Token *pParamName);
	int AddFlushSQLorLoadSQLParamAttr(const char * pAttrName,Token *pAttrValue);
	int BuildModifyFlushSQLParam(int iIFNE,Token *pTableName,Token *pParamName,const char*pOptName);
	int ModifyFlushSQLorLoadSQLParamAttr(const char * pAttrName,Token *pAttrValue);
        void SetTimeStamp(long long iTimeStamp);
        long long GetTimeStamp();
    
    int BuildRenameTable(Token * pOldTableName,Token * pNewTableName);//rename table
    
	int BuildHint(Token * pHintName);
	
    int CheckTableExist();//表是否存在
    bool IsDDLSQL(){return m_bIsDDLSQL;}
    int NewDDLStruct();//对于不解析SQL，而直接调用DDL 解析器的接口，需要手工申请空间
    
protected:
	void CollectOutputValue();// 搜集需要输出的值
	void CollectInputValue();// 搜集需要输入的值
	int SimpleColumnCheck(ST_EXPR * pstExpr);
	int FillMdbInfo();//填充MDB相关信息
	int static FillExprColumn(ST_WALKER * pstWalker,ST_EXPR * pstExpr);//填充列信息
	int static FillArrayInput(ST_WALKER * pstWalker,ST_EXPR * pstExpr);//填充列信息

	int CollectWhereIndex();//获取where条件中的index
	int GetWhereIndex(std::vector< ST_INDEX_VALUE > & vIndexValue,ST_WHERE_OR_CLAUSE & tWhereOrClause);//获取where索引
	//int GetSubWhereIndex(ST_EXPR * pstExpr,std::vector< ST_INDEX_VALUE > & vIndexValue,std::vector<ST_INDEX_COLUMN> & vIndexColumn);//获取子where条件中的index
	int GetSubWhereIndex(ST_EXPR * pstExpr,std::vector<ST_INDEX_COLUMN> & vIndexColumn);//获取子where条件中的index
	//int CombineCMPIndex(std::vector< ST_INDEX_VALUE > & vLeftIndex,std::vector< ST_INDEX_VALUE > & vRightIndex,std::vector< ST_INDEX_VALUE > & vOutIndex);//组合where中的index
	//int RemoveDupIndex(std::vector< ST_INDEX_VALUE > & vIndex);//移除重复的复合索引
	int FillIdList(ST_ID_LIST * pIdList);//填充ID_LIST

	int SpanInsertCollumnList(ST_ID_LIST * pExistList);//扩充insert ColumnList 到所有列
	int SpanUpdateCollumnList(ST_ID_LIST *pstIdList);//扩充insert ColumnList 到所有列
	void CleanOtherMem();// 清理其它内存数据
	int SpanTKALLCollumnList(ST_EXPR_LIST * pCollist);//扩展* 字段
	int CollectChangeIndex(ST_ID_LIST *pstIdList);//获取update ，insert中所变更的索引
	int FillOrderbyInfo(ST_EXPR_LIST * pExprList);//填充orderbyinfo
	void VectorToList();//将vector转成list
	int CollectPrimaryKeyFromSQL(ST_ID_LIST * pIdList);//搜集主键信息
	int CollectPrimaryKeyFromTable();//从表中获取主键信息
	int CollectIndexForVerifyPK(ST_ID_LIST * pIdList);//获取用于检测主键的索引列
	bool IsFullIndex(std::vector< ST_INDEX_VALUE > & vIndex);//是否都是完整的索引组
	bool IsFullIndex(  ST_INDEX_VALUE & stIndexValue);//是否都是完整的索引
	int CheckSetList(ST_SET_LIST & stSetList);//检测update的set列表
	int RemoveTableAlias();//去除表别名
	int SetDsnLinkAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue);
	int SetDsnRepAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue);
	int SetDsnOtherAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue);
	int SetCfgProAttribute(TMdbCfgProAttr *pProAttr,char *sAttrName,char *sAttrValue);
	void InitDsnAttrName();
	int CollectRouterID(_ST_EXPR * & pstExpr);//搜集routerid列信息
	int ChangeValueToRefence(ST_MEM_VALUE_LIST & tListMemValue);//将memvalue中的值改成引用值,优化使其不需要从内存中拷贝出来
	int OptimizeWhereClause(ST_EXPR * pstExpr,ST_WHERE_OR_CLAUSE & tWhereOrClause);//优化where 条件
	int TranslateBlobColumnValue(ST_EXPR * pExpr,TMdbColumn * pColumn);//转换sql中的blob值
	int FillFirstAndLimit(ST_EXPR * pFirst,ST_EXPR * pLimit,ST_EXPR * pOffset);//填充first limit
	//int FillSequenceValue();//填充序列值
	//int CalcSequenceValue(ST_EXPR * & pstExpr,int iSeqType);
	
	int FillHintInfo();
        
public:
	ST_MEM_VALUE_LIST m_listInputVariable;   //获取绑定变量值
	ST_MEM_VALUE_LIST m_listInputWhere;  //where条件需要输入的值
	ST_MEM_VALUE_LIST m_listInputCollist;//需要输入的值
	ST_MEM_VALUE_LIST m_listOutputCollist;//输出的值
	ST_MEM_VALUE_LIST m_listInputOrderby;//orderby 的input
	ST_MEM_VALUE_LIST m_listOutputOrderby;//orderby 的output
	ST_MEM_VALUE_LIST m_listInputLimit;//limit 的input
	ST_MEM_VALUE_LIST m_listOutputLimit;//limit的output
	ST_MEM_VALUE_LIST m_listOutputPriKey;//输出的主键值
	ST_MEM_VALUE_LIST m_listInputPriKey;//需要输入的主键值
	
    ST_MEM_VALUE_LIST m_listInputGroupBy;//需要输入的group by
    ST_MEM_VALUE_LIST m_listOutputGroupBy;//需要输出的group by
    ST_MEM_VALUE_LIST m_listInputHaving;
    ST_MEM_VALUE_LIST m_listOutputHaving;//

    ST_MEM_VALUE_LIST m_listInputAggValue;//带输入的聚合值
       
	TMdbErrorHelper  m_tError;//错误信息
	
	_ST_SQL_STRUCT m_stSqlStruct;		//DML sql
	//_ST_DDL_SQL_STRUCT m_stDDLSqlStruct;//DDL sql
	_ST_DDL_SQL_STRUCT * m_pDDLSqlStruct;//DDL sql
	TMdbExpr       m_tMdbExpr;
	TSqlParserHelper m_tSqlParserHelper;//SQL 解析helper类
	TMdbConfig   * m_pMdbConfig;//mdb配置信息,由外部传进来，不负责销毁。
	//int m_iSourceId;//数据来源，用于主备容灾同步
	bool m_bGetValueChecked;//已检测过GetValue接口
    std::vector<ST_EXPR * > m_vAggExpr; //聚合表达式

	//Hint
	ST_TABLE_INDEX_INFO*	m_pHintIndex;   
       
private:
	std::vector<_ST_MEM_VALUE *> m_arrOtherMemValue;//防止内存泄露
	std::vector<_ST_EXPR *>  m_arrOtherExpr;//其他expr;防止内存泄露
	TMdbIndexCtrl  m_MdbIndexCtrl;//mdbindex管理
	TSqlTypeCheck  m_tSqlTypeCheck;//sql 数据类型校验类
	std::vector<TMdbColumn * >  m_vNewColumn;//新列信息
	std::map<string,string> m_mapDsnAttrName;
    bool m_bIsDDLSQL;//是否是DDL
    long long m_iSetTimeStamp; // 设置记录的时间戳
};

//}
#endif

