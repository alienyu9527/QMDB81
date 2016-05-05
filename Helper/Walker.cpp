//#include "BillingSDK.h"
#include "Helper/Walker.h"

//namespace QuickMDB{
	//遍历表达式节点
	int TWalker::WalkExpr(ST_WALKER * pstWalker,ST_EXPR *pstExpr)
	{
		int rc;
		if( NULL == pstExpr ) return WRC_Continue;
		rc = pstWalker->xExprCallback(pstWalker, pstExpr);

		if( rc==WRC_Continue){
			if( WalkExpr(pstWalker, pstExpr->pLeft) ) return WRC_Abort;
			if( WalkExpr(pstWalker, pstExpr->pRight) ) return WRC_Abort;
			if( NULL != pstExpr->pFunc){
				if(WalkExprList(pstWalker,pstExpr->pFunc->pFuncArgs))return WRC_Abort;
			}
		}
		return rc & WRC_Abort;
	}
	//遍历表达式链表
	int TWalker::WalkExprList(ST_WALKER * pstWalker,ST_EXPR_LIST *pstExprList)
	{
		if(NULL == pstExprList)
			return 0;
		int i = 0;
		for(i  = 0;i<pstExprList->iItemNum;i++)
		{
			if(WalkExpr(pstWalker,pstExprList->pExprItems[i].pExpr))return WRC_Abort;
		}
		return WRC_Continue;
	}
//}


