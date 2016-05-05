/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    SqlTypeCheck.h		
*@Description： SQL数据类型校验类
*@Author:	    jin.shaohua
*@Date：	    2012.05
*@History:
******************************************************************************************/

#ifndef _SQL_TYPE_CHECK_H_
#define _SQL_TYPE_CHECK_H_
#include "Helper/mdbExpr.h"
#include "Helper/SqlParserStruct.h"
#include <vector>
//namespace QuickMDB{
	//类型校验规则
	struct ST_ADAPT_RULE
	{
		int iHasType;   // has or hasany
		int iLeftType;   //左值类型
		int iRightType; //右值类型
		int iResultType;//结果类型
		int iResultSize;//结果大小
	};

	//数据类型校验
	class TSqlTypeCheck
	{
	public:
		TSqlTypeCheck();
		~TSqlTypeCheck();
		int FillValidType(ST_SQL_STRUCT * pstSqlStruct);//向节点中填充所支持的合法类型
		int ConverType(TMdbColumn * pColumn,ST_MEM_VALUE * pstMemValue);//类型转化
		int AdaptAllExprType(ST_EXPR *pstExpr,int iType,int iSize);//适配所有的expr
		int SetSqlStruct(ST_SQL_STRUCT * pstSqlStruct){if(NULL != pstSqlStruct)m_pstSqlStruct = pstSqlStruct;return 0;};//设置SQL 结构体
	private:
		int FillInsertValidType(ST_SQL_STRUCT * pstSqlStruct);//向节点中填充所支持的合法类型
		int FillDeleteValidType(ST_SQL_STRUCT * pstSqlStruct);//向节点中填充所支持的合法类型
		int FillUpdateValidType(ST_SQL_STRUCT * pstSqlStruct);//向节点中填充所支持的合法类型
		int FillSelectValidType(ST_SQL_STRUCT * pstSqlStruct);//向节点中填充所支持的合法类型
		int GetTypeAndSize(TMdbColumn * pColumn,int & iValueType,int & iValueSize);//获取值类型和长度
		int CheckTypeSizeValid(int iValidType,int iValidSize,ST_MEM_VALUE * pstMemValue);//type 和size 是否正确

		int FillExprListValidType(ST_EXPR_LIST * pstExprList,int iType,int iSize);//向表达式list节点中填充合法类型
		int FillFunctionValidType(ST_EXPR * pstExpr);//向函数填充合法类型
		int AdaptExprType(ST_EXPR *pstExpr);//适配expr type
		int AdaptVariableExprType(ST_EXPR *pstExpr,int iType,int iSize);//适配绑定变量的expr
		int AdaptExprTypeRule(ST_EXPR *pstExpr,ST_ADAPT_RULE * arrAdaptRule);//按规则进行校验
		int SetExprCalcFalgs(ST_EXPR *pstExpr);//设置表达式计算标识
	private:
		ST_SQL_STRUCT * m_pstSqlStruct;
	};
//}

#endif
