#ifndef _SYNTAX_TREE_ANALYSE_H_
#define _SYNTAX_TREE_ANALYSE_H_
#include "Helper/mdbSQLParser.h"
//namespace QuickMDB{
	class TSyntaxTreeAnalyse
	{
	public:
		int static Print(TMdbSqlParser * pSqlParser,int iLogType);//打印SQL语法树
		int static PrintVariablePKValue(TMdbSqlParser * pSqlParser,int iLogType);//打印主键信息
		static const char *  ReplaceNull(char * pStr);
		static std::string DescribeMemValueFlag(int iFlags);//解释memvalue中的flag
		static std::string DescribeMemValueFlag(ST_MEM_VALUE * pstMemValue);//解释memvalue中的flag
		static std::string DescribeExprCalcFlag(ST_EXPR * pstExpr);//解释expr中的calcflag
		int static PrintArrMemValue(std::vector<_ST_MEM_VALUE *> arrMemValue,int iLogType);
		int static CheckWhereIndex(TMdbSqlParser * pSqlParser);//校验where 中的index
	protected:
		int static PrintValues(TMdbSqlParser * pSqlParser,int iLogType);//打印值
		static char * GetOffSet(int n,char zOff='\t');//获取偏移
		//print
		int static PrintExprList(ST_EXPR_LIST * pstExprList,int offset,int iLogType);//打印表达式列表
		int static PrintExpr(const char* sPrefix,ST_EXPR * pstExpr,int offset,int iLogType);//打印表达式
		int static PrintIdList(ST_ID_LIST * pstIdList,int offset,int iLogType);//打印IDlist

		int static PrintTable(TMdbTable * pTable,int iLogType);//打印表信息

		static int OutPutInfo(int iLogType,const char * fmt, ...);//输出

		static int PrintIndex(std::vector< _ST_INDEX_VALUE> & vIndexValue,int offset,int iLogType);
		static int PrintWhereClause(int iLogType,ST_WHERE_OR_CLAUSE & stWhereOrClause);//打印where 段
	private:
		static char m_sOffset[1024];
	};
//}

#endif
