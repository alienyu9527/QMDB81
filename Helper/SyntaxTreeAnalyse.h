#ifndef _SYNTAX_TREE_ANALYSE_H_
#define _SYNTAX_TREE_ANALYSE_H_
#include "Helper/mdbSQLParser.h"
//namespace QuickMDB{
	class TSyntaxTreeAnalyse
	{
	public:
		int static Print(TMdbSqlParser * pSqlParser,int iLogType);//��ӡSQL�﷨��
		int static PrintVariablePKValue(TMdbSqlParser * pSqlParser,int iLogType);//��ӡ������Ϣ
		static const char *  ReplaceNull(char * pStr);
		static std::string DescribeMemValueFlag(int iFlags);//����memvalue�е�flag
		static std::string DescribeMemValueFlag(ST_MEM_VALUE * pstMemValue);//����memvalue�е�flag
		static std::string DescribeExprCalcFlag(ST_EXPR * pstExpr);//����expr�е�calcflag
		int static PrintArrMemValue(std::vector<_ST_MEM_VALUE *> arrMemValue,int iLogType);
		int static CheckWhereIndex(TMdbSqlParser * pSqlParser);//У��where �е�index
	protected:
		int static PrintValues(TMdbSqlParser * pSqlParser,int iLogType);//��ӡֵ
		static char * GetOffSet(int n,char zOff='\t');//��ȡƫ��
		//print
		int static PrintExprList(ST_EXPR_LIST * pstExprList,int offset,int iLogType);//��ӡ���ʽ�б�
		int static PrintExpr(const char* sPrefix,ST_EXPR * pstExpr,int offset,int iLogType);//��ӡ���ʽ
		int static PrintIdList(ST_ID_LIST * pstIdList,int offset,int iLogType);//��ӡIDlist

		int static PrintTable(TMdbTable * pTable,int iLogType);//��ӡ����Ϣ

		static int OutPutInfo(int iLogType,const char * fmt, ...);//���

		static int PrintIndex(std::vector< _ST_INDEX_VALUE> & vIndexValue,int offset,int iLogType);
		static int PrintWhereClause(int iLogType,ST_WHERE_OR_CLAUSE & stWhereOrClause);//��ӡwhere ��
	private:
		static char m_sOffset[1024];
	};
//}

#endif
