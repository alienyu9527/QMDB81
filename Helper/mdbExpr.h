#ifndef _MDB_EXPR_H_
#define _MDB_EXPR_H_
#include "SqlParserStruct.h"
/************************************************************************/
/* MDB sql语句中的表达式                                                */
/************************************************************************/

//namespace QuickMDB{

class TMdbExpr
{
public:
	TMdbExpr();
	~TMdbExpr();

	ST_EXPR * BuildExpr(int op);//生成一个表达式
	ST_EXPR_LIST * ExprListAppend(
		ST_EXPR_LIST *pList,        /* List to which to append. Might be NULL */
		ST_EXPR *pExpr             /* Expression to be appended. Might be NULL */
		);
	ST_EXPR * BuildPExpr(int op,ST_EXPR * pChild,ST_EXPR * pNext,const Token * pToken);
	ST_EXPR * BuildExprFunction(ST_EXPR_LIST * pList,Token *pToken);//创建
	void ExprListSetName(  ST_EXPR_LIST *pList,        /* List to which to add the span. */
		Token *pName,           /* Name to be added */
		int dequote             /* True to cause the name to be dequoted */);
	void  ExprListSetSpan(ST_EXPR_LIST *pList,        /* List to which to add the span. */
		ST_EXPR_SPAN *pSpan         /* The span to be added */
		);
	int CalcExpr(ST_EXPR * & pstExpr);//计算表达式的值
	int CalcFunction(ST_EXPR * & pstExpr);//计算函数
	int CalcExprList(ST_EXPR_LIST * & pstExprList);//计算表达式列表

	ST_EXPR_LIST * DeleteExpr(ST_EXPR_LIST * pstExprList,ST_EXPR * pstExprToDel);//从list中删除expr
	void SetExecRowPos(long long iPos){m_lRowPos = iPos;}
	const char * ShowExprValue(const ST_EXPR * pstExpr);//只是显示expr 的value
	const char *  ReplaceNull(const char * pStr);//替换NULL
	bool IsContainColumn(ST_EXPR * pstExpr);//查看是否包含column列
	bool IsContainOpcode(ST_EXPR * pstExpr,int iOpcode);//是否包含某操作码
	int RemoveTableAlias(ST_EXPR * & pstExpr,const char * sTableAlias);//去除表别名
	void SetNullFlag(bool bNullFlag){m_bNull = bNullFlag;};
	//int ResetCalcFlagByCalcLevel(ST_EXPR * pstExpr,int iCalcLevel);//根据计算等级重置计算标识
	//int ResetCalcFlagByCalcLevel(ST_EXPR_LIST * & pstExprList,int iCalcLevel);//根据计算等级重置计算标识
protected:
	void  ExprAttachSubtrees(ST_EXPR * pRoot,ST_EXPR * pChild,ST_EXPR * pNext);
	ST_EXPR * ExprAlloc(int op,const Token * pToken,int dequote  );//给expr分配空间
	int ExprCodeTarget(ST_EXPR * pstExpr);//
	int CheckFunction(ST_EXPR * pstExpr);//检测函数
	int CalcBinaryExpr(ST_EXPR * & pstExpr,int iSubValueType);//计算双元运算符
	int CalcUnaryExpr(ST_EXPR * & pstExpr,int iSubValueType);//计算一元运算
	int CalcAndOrExpr(ST_EXPR * & pstExpr,int iSubValueType);//计算and or 表达式
	int CalcBinaryExprNull(ST_EXPR * & pstExpr,int iSubValueType);//新的null逻辑来处理二元运算       
private:
	long long 		m_lRowPos;
	char	*		m_pExprValue;
	bool 			m_bNull;//true 表示nil 运算为nil
};


//}
#endif


