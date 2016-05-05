#ifndef _MDB_EXPR_H_
#define _MDB_EXPR_H_
#include "SqlParserStruct.h"
/************************************************************************/
/* MDB sql����еı��ʽ                                                */
/************************************************************************/

//namespace QuickMDB{

class TMdbExpr
{
public:
	TMdbExpr();
	~TMdbExpr();

	ST_EXPR * BuildExpr(int op);//����һ�����ʽ
	ST_EXPR_LIST * ExprListAppend(
		ST_EXPR_LIST *pList,        /* List to which to append. Might be NULL */
		ST_EXPR *pExpr             /* Expression to be appended. Might be NULL */
		);
	ST_EXPR * BuildPExpr(int op,ST_EXPR * pChild,ST_EXPR * pNext,const Token * pToken);
	ST_EXPR * BuildExprFunction(ST_EXPR_LIST * pList,Token *pToken);//����
	void ExprListSetName(  ST_EXPR_LIST *pList,        /* List to which to add the span. */
		Token *pName,           /* Name to be added */
		int dequote             /* True to cause the name to be dequoted */);
	void  ExprListSetSpan(ST_EXPR_LIST *pList,        /* List to which to add the span. */
		ST_EXPR_SPAN *pSpan         /* The span to be added */
		);
	int CalcExpr(ST_EXPR * & pstExpr);//������ʽ��ֵ
	int CalcFunction(ST_EXPR * & pstExpr);//���㺯��
	int CalcExprList(ST_EXPR_LIST * & pstExprList);//������ʽ�б�

	ST_EXPR_LIST * DeleteExpr(ST_EXPR_LIST * pstExprList,ST_EXPR * pstExprToDel);//��list��ɾ��expr
	void SetExecRowPos(long long iPos){m_lRowPos = iPos;}
	const char * ShowExprValue(const ST_EXPR * pstExpr);//ֻ����ʾexpr ��value
	const char *  ReplaceNull(const char * pStr);//�滻NULL
	bool IsContainColumn(ST_EXPR * pstExpr);//�鿴�Ƿ����column��
	bool IsContainOpcode(ST_EXPR * pstExpr,int iOpcode);//�Ƿ����ĳ������
	int RemoveTableAlias(ST_EXPR * & pstExpr,const char * sTableAlias);//ȥ�������
	void SetNullFlag(bool bNullFlag){m_bNull = bNullFlag;};
	//int ResetCalcFlagByCalcLevel(ST_EXPR * pstExpr,int iCalcLevel);//���ݼ���ȼ����ü����ʶ
	//int ResetCalcFlagByCalcLevel(ST_EXPR_LIST * & pstExprList,int iCalcLevel);//���ݼ���ȼ����ü����ʶ
protected:
	void  ExprAttachSubtrees(ST_EXPR * pRoot,ST_EXPR * pChild,ST_EXPR * pNext);
	ST_EXPR * ExprAlloc(int op,const Token * pToken,int dequote  );//��expr����ռ�
	int ExprCodeTarget(ST_EXPR * pstExpr);//
	int CheckFunction(ST_EXPR * pstExpr);//��⺯��
	int CalcBinaryExpr(ST_EXPR * & pstExpr,int iSubValueType);//����˫Ԫ�����
	int CalcUnaryExpr(ST_EXPR * & pstExpr,int iSubValueType);//����һԪ����
	int CalcAndOrExpr(ST_EXPR * & pstExpr,int iSubValueType);//����and or ���ʽ
	int CalcBinaryExprNull(ST_EXPR * & pstExpr,int iSubValueType);//�µ�null�߼��������Ԫ����       
private:
	long long 		m_lRowPos;
	char	*		m_pExprValue;
	bool 			m_bNull;//true ��ʾnil ����Ϊnil
};


//}
#endif


