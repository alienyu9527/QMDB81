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
/* sql������                                                                                                     */
/************************************************************************/
class TMdbSqlParser
{
public:
	TMdbSqlParser();
	~TMdbSqlParser();
	int Init();//��ʼ��
	int ParseSQL(const char * sSql);//����
    int CheckDDLSQL(const char * sSql);//���DDL,�������ڴ�
	int FillTableName(const Token * pToken,const Token * pTokenAlias);//������Ϣ
	ST_ID_LIST * IdListAppend(ST_ID_LIST *pList, Token *pToken);//
	int AddColumn(Token *pToken);//�����
	int AddColumnAttribute(const char * sAttrName, Token *pToken);//���������
	int AddTableAttribute(Token *pAttrName, Token *pAttrValue);//��ӱ�����
	int AddTablePrimayKey(ST_ID_LIST * pIdList);//��ӱ��������Ϣ
	void CleanResult();
       int ClearLastExecute();//������һ�ε�ִ��
	int BuildSelect(ST_EXPR_LIST * pHint,ST_EXPR * pFirst,int iDistinct,
									ST_EXPR_LIST * pcolList,ST_EXPR * pstWhere,
									ST_EXPR_LIST * pstGroupby,ST_EXPR * pstHaving,
									ST_EXPR_LIST * pstOrder,ST_EXPR * pLimit,ST_EXPR * pOffset);//����select
	int BuildInsert(ST_ID_LIST * pCollist,ST_EXPR_LIST * pstExprList);//����insert
	int BuildDelete(ST_EXPR_LIST * pHint,ST_EXPR * pFirst,ST_EXPR * pstExpr,ST_EXPR * pLimit,ST_EXPR * pOffset);//����delete
	int BuildUpdate(ST_EXPR_LIST * pHint,ST_EXPR * pFirst,ST_SET_LIST stSetList,ST_EXPR * pstExpr,ST_EXPR * pLimit,ST_EXPR * pOffset);//����update
	int BuildCreateIndex(int iIFNE,Token * pIndexName,Token *pTableName,ST_ID_LIST * pIdList, Token* psAlgoName, Token* pLayerLimit = NULL);//����createindex
	int BuildCreateTable(int iIFNE,Token * pTableName);//����createtable
	int CheckOrderByInGroupBy();
    int BuildGroupBy();//����group by�Ӿ�
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
	int BuildLoadData(Token * pTableName);//��������ĳ�ֱ������
	int BuildAddSequence(); //�����������нṹ
	int BuildDelSequence();//����ɾ�����нṹ
	int BuildAlterSequence(); //����������нṹ
	int AddSequenceValue(Token *pColumnName,Token *pValue);//��ȡ�������л�ɾ�����е���ֵ
		
	//���Ÿ��ں�ģ��Ľӿ�
	int ExecuteWhere(bool & bResult);//����where����
	int ExecuteSQL();//���﷨����������
	int ExecuteOrderby();//����orderby ��ֵ
	int ExecuteGroupby();//����Groupby ��ֵ
	int ExecuteHaving();//����havingֵ
	int ExecuteLimit();//����limit ��ֵ
	int GetWhereIndex(std::vector< ST_INDEX_VALUE >* & vIndex);//������ʹ�õ�indexֵ
	int SetDB(TMdbShmDSN * pMdbDsn,TMdbConfig * pConfig);//����MDB��������Ϣ
	int SetMDBConfig(TMdbConfig * pConfig);//����MDBconfig
	void ClearMemValue(ST_MEM_VALUE_LIST & stMemValueList);// ֻ���������ݣ��������ڴ�����
	int GenRollbackSql(char * sNewSql);// ����rollback��sql;
	bool IsTableAlias(Token * pToken);//�ж��Ƿ��Ǳ�ı���
	bool IsCurRowNeedCapture();//��ǰ���Ƿ���Ҫ����
	int BuildCreateJob(Token * pJobName); //��������job
	int BuildAlterJob(Token * pJobName); //�����޸�job
	int BuildRemoveJob(Token * pJobName); //����ɾ��job	
	int AddJobValue(Token *pAttrName,Token *pAttrValue); //���job���Ժ�ֵ
	int BuildAddFlushSQLParam(int iIFNE,Token *pTableName,Token *pParamName);
	int AddFlushSQLorLoadSQLParamAttr(const char * pAttrName,Token *pAttrValue);
	int BuildModifyFlushSQLParam(int iIFNE,Token *pTableName,Token *pParamName,const char*pOptName);
	int ModifyFlushSQLorLoadSQLParamAttr(const char * pAttrName,Token *pAttrValue);
        void SetTimeStamp(long long iTimeStamp);
        long long GetTimeStamp();
    
    int BuildRenameTable(Token * pOldTableName,Token * pNewTableName);//rename table
    
	int BuildHint(Token * pHintName);
	
    int CheckTableExist();//���Ƿ����
    bool IsDDLSQL(){return m_bIsDDLSQL;}
    int NewDDLStruct();//���ڲ�����SQL����ֱ�ӵ���DDL �������Ľӿڣ���Ҫ�ֹ�����ռ�
    
protected:
	void CollectOutputValue();// �Ѽ���Ҫ�����ֵ
	void CollectInputValue();// �Ѽ���Ҫ�����ֵ
	int SimpleColumnCheck(ST_EXPR * pstExpr);
	int FillMdbInfo();//���MDB�����Ϣ
	int static FillExprColumn(ST_WALKER * pstWalker,ST_EXPR * pstExpr);//�������Ϣ
	int static FillArrayInput(ST_WALKER * pstWalker,ST_EXPR * pstExpr);//�������Ϣ

	int CollectWhereIndex();//��ȡwhere�����е�index
	int GetWhereIndex(std::vector< ST_INDEX_VALUE > & vIndexValue,ST_WHERE_OR_CLAUSE & tWhereOrClause);//��ȡwhere����
	//int GetSubWhereIndex(ST_EXPR * pstExpr,std::vector< ST_INDEX_VALUE > & vIndexValue,std::vector<ST_INDEX_COLUMN> & vIndexColumn);//��ȡ��where�����е�index
	int GetSubWhereIndex(ST_EXPR * pstExpr,std::vector<ST_INDEX_COLUMN> & vIndexColumn);//��ȡ��where�����е�index
	//int CombineCMPIndex(std::vector< ST_INDEX_VALUE > & vLeftIndex,std::vector< ST_INDEX_VALUE > & vRightIndex,std::vector< ST_INDEX_VALUE > & vOutIndex);//���where�е�index
	//int RemoveDupIndex(std::vector< ST_INDEX_VALUE > & vIndex);//�Ƴ��ظ��ĸ�������
	int FillIdList(ST_ID_LIST * pIdList);//���ID_LIST

	int SpanInsertCollumnList(ST_ID_LIST * pExistList);//����insert ColumnList ��������
	int SpanUpdateCollumnList(ST_ID_LIST *pstIdList);//����insert ColumnList ��������
	void CleanOtherMem();// ���������ڴ�����
	int SpanTKALLCollumnList(ST_EXPR_LIST * pCollist);//��չ* �ֶ�
	int CollectChangeIndex(ST_ID_LIST *pstIdList);//��ȡupdate ��insert�������������
	int FillOrderbyInfo(ST_EXPR_LIST * pExprList);//���orderbyinfo
	void VectorToList();//��vectorת��list
	int CollectPrimaryKeyFromSQL(ST_ID_LIST * pIdList);//�Ѽ�������Ϣ
	int CollectPrimaryKeyFromTable();//�ӱ��л�ȡ������Ϣ
	int CollectIndexForVerifyPK(ST_ID_LIST * pIdList);//��ȡ���ڼ��������������
	bool IsFullIndex(std::vector< ST_INDEX_VALUE > & vIndex);//�Ƿ���������������
	bool IsFullIndex(  ST_INDEX_VALUE & stIndexValue);//�Ƿ�������������
	int CheckSetList(ST_SET_LIST & stSetList);//���update��set�б�
	int RemoveTableAlias();//ȥ�������
	int SetDsnLinkAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue);
	int SetDsnRepAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue);
	int SetDsnOtherAttribute(TMdbCfgDSN * pDsn,char *sAttrName,char *sAttrValue);
	int SetCfgProAttribute(TMdbCfgProAttr *pProAttr,char *sAttrName,char *sAttrValue);
	void InitDsnAttrName();
	int CollectRouterID(_ST_EXPR * & pstExpr);//�Ѽ�routerid����Ϣ
	int ChangeValueToRefence(ST_MEM_VALUE_LIST & tListMemValue);//��memvalue�е�ֵ�ĳ�����ֵ,�Ż�ʹ�䲻��Ҫ���ڴ��п�������
	int OptimizeWhereClause(ST_EXPR * pstExpr,ST_WHERE_OR_CLAUSE & tWhereOrClause);//�Ż�where ����
	int TranslateBlobColumnValue(ST_EXPR * pExpr,TMdbColumn * pColumn);//ת��sql�е�blobֵ
	int FillFirstAndLimit(ST_EXPR * pFirst,ST_EXPR * pLimit,ST_EXPR * pOffset);//���first limit
	//int FillSequenceValue();//�������ֵ
	//int CalcSequenceValue(ST_EXPR * & pstExpr,int iSeqType);
	
	int FillHintInfo();
        
public:
	ST_MEM_VALUE_LIST m_listInputVariable;   //��ȡ�󶨱���ֵ
	ST_MEM_VALUE_LIST m_listInputWhere;  //where������Ҫ�����ֵ
	ST_MEM_VALUE_LIST m_listInputCollist;//��Ҫ�����ֵ
	ST_MEM_VALUE_LIST m_listOutputCollist;//�����ֵ
	ST_MEM_VALUE_LIST m_listInputOrderby;//orderby ��input
	ST_MEM_VALUE_LIST m_listOutputOrderby;//orderby ��output
	ST_MEM_VALUE_LIST m_listInputLimit;//limit ��input
	ST_MEM_VALUE_LIST m_listOutputLimit;//limit��output
	ST_MEM_VALUE_LIST m_listOutputPriKey;//���������ֵ
	ST_MEM_VALUE_LIST m_listInputPriKey;//��Ҫ���������ֵ
	
    ST_MEM_VALUE_LIST m_listInputGroupBy;//��Ҫ�����group by
    ST_MEM_VALUE_LIST m_listOutputGroupBy;//��Ҫ�����group by
    ST_MEM_VALUE_LIST m_listInputHaving;
    ST_MEM_VALUE_LIST m_listOutputHaving;//

    ST_MEM_VALUE_LIST m_listInputAggValue;//������ľۺ�ֵ
       
	TMdbErrorHelper  m_tError;//������Ϣ
	
	_ST_SQL_STRUCT m_stSqlStruct;		//DML sql
	//_ST_DDL_SQL_STRUCT m_stDDLSqlStruct;//DDL sql
	_ST_DDL_SQL_STRUCT * m_pDDLSqlStruct;//DDL sql
	TMdbExpr       m_tMdbExpr;
	TSqlParserHelper m_tSqlParserHelper;//SQL ����helper��
	TMdbConfig   * m_pMdbConfig;//mdb������Ϣ,���ⲿ�����������������١�
	//int m_iSourceId;//������Դ��������������ͬ��
	bool m_bGetValueChecked;//�Ѽ���GetValue�ӿ�
    std::vector<ST_EXPR * > m_vAggExpr; //�ۺϱ��ʽ

	//Hint
	ST_TABLE_INDEX_INFO*	m_pHintIndex;   
       
private:
	std::vector<_ST_MEM_VALUE *> m_arrOtherMemValue;//��ֹ�ڴ�й¶
	std::vector<_ST_EXPR *>  m_arrOtherExpr;//����expr;��ֹ�ڴ�й¶
	TMdbIndexCtrl  m_MdbIndexCtrl;//mdbindex����
	TSqlTypeCheck  m_tSqlTypeCheck;//sql ��������У����
	std::vector<TMdbColumn * >  m_vNewColumn;//������Ϣ
	std::map<string,string> m_mapDsnAttrName;
    bool m_bIsDDLSQL;//�Ƿ���DDL
    long long m_iSetTimeStamp; // ���ü�¼��ʱ���
};

//}
#endif

