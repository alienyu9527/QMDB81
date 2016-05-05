#ifndef _WALKER_H_
#define _WALKER_H_
#include "Helper/SqlParserStruct.h"
#include "Helper/mdbSQLParser.h"
//namespace QuickMDB{
	/*
	** Return code from the parse-tree walking primitives and their
	** callbacks.
	*/
#define WRC_Continue    0   /* Continue down into children */
#define WRC_Prune       1   /* Omit children but continue walking siblings */
#define WRC_Abort       2   /* Abandon the tree walk */


	/*
	**定义填充哪种输入容器
	*/
#define WRC_InputWhere   0
#define WRC_InputCollist 1
#define WRC_InputOrderby 2
#define WRC_InputLimit   3
#define WRC_InputGroupBy 4

	/*
	** Context pointer passed down through the tree-walk.
	*/
	class TMdbSqlParser;

	struct _ST_WALKER {
		int (*xExprCallback)(_ST_WALKER*, ST_EXPR *);     /* Callback for expressions */
		TMdbSqlParser* pMdbSqlParser;
		int iInputType;//遍历并填充哪种输入队列
		//int (*xSelectCallback)(Walker*,Select*);  /* Callback for SELECTs */
		// Parse *pParse;                            /* Parser context.  */ 
		//  union {                                   /* Extra data for callback */
		//    NameContext *pNC;                          /* Naming context */ 
		//   int i;                                     /* Integer value */
		// } u;
	};

	/**
	* 遍历类
	*/
	class TWalker
	{
	public:
		static int WalkExpr(ST_WALKER * pstWalker,ST_EXPR *pstExpr);//遍历表达式
		static int WalkExprList(ST_WALKER * pstWalker,ST_EXPR_LIST *pstExprList);//遍历表达式链表
	};
//}


#endif

