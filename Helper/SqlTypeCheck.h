/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    SqlTypeCheck.h		
*@Description�� SQL��������У����
*@Author:	    jin.shaohua
*@Date��	    2012.05
*@History:
******************************************************************************************/

#ifndef _SQL_TYPE_CHECK_H_
#define _SQL_TYPE_CHECK_H_
#include "Helper/mdbExpr.h"
#include "Helper/SqlParserStruct.h"
#include <vector>
//namespace QuickMDB{
	//����У�����
	struct ST_ADAPT_RULE
	{
		int iHasType;   // has or hasany
		int iLeftType;   //��ֵ����
		int iRightType; //��ֵ����
		int iResultType;//�������
		int iResultSize;//�����С
	};

	//��������У��
	class TSqlTypeCheck
	{
	public:
		TSqlTypeCheck();
		~TSqlTypeCheck();
		int FillValidType(ST_SQL_STRUCT * pstSqlStruct);//��ڵ��������֧�ֵĺϷ�����
		int ConverType(TMdbColumn * pColumn,ST_MEM_VALUE * pstMemValue);//����ת��
		int AdaptAllExprType(ST_EXPR *pstExpr,int iType,int iSize);//�������е�expr
		int SetSqlStruct(ST_SQL_STRUCT * pstSqlStruct){if(NULL != pstSqlStruct)m_pstSqlStruct = pstSqlStruct;return 0;};//����SQL �ṹ��
	private:
		int FillInsertValidType(ST_SQL_STRUCT * pstSqlStruct);//��ڵ��������֧�ֵĺϷ�����
		int FillDeleteValidType(ST_SQL_STRUCT * pstSqlStruct);//��ڵ��������֧�ֵĺϷ�����
		int FillUpdateValidType(ST_SQL_STRUCT * pstSqlStruct);//��ڵ��������֧�ֵĺϷ�����
		int FillSelectValidType(ST_SQL_STRUCT * pstSqlStruct);//��ڵ��������֧�ֵĺϷ�����
		int GetTypeAndSize(TMdbColumn * pColumn,int & iValueType,int & iValueSize);//��ȡֵ���ͺͳ���
		int CheckTypeSizeValid(int iValidType,int iValidSize,ST_MEM_VALUE * pstMemValue);//type ��size �Ƿ���ȷ

		int FillExprListValidType(ST_EXPR_LIST * pstExprList,int iType,int iSize);//����ʽlist�ڵ������Ϸ�����
		int FillFunctionValidType(ST_EXPR * pstExpr);//�������Ϸ�����
		int AdaptExprType(ST_EXPR *pstExpr);//����expr type
		int AdaptVariableExprType(ST_EXPR *pstExpr,int iType,int iSize);//����󶨱�����expr
		int AdaptExprTypeRule(ST_EXPR *pstExpr,ST_ADAPT_RULE * arrAdaptRule);//���������У��
		int SetExprCalcFalgs(ST_EXPR *pstExpr);//���ñ��ʽ�����ʶ
	private:
		ST_SQL_STRUCT * m_pstSqlStruct;
	};
//}

#endif
