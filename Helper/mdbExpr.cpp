#include "mdbExpr.h"
#include "parser.h"
#include "mdbMalloc.h"
#include <iostream>
#include "mdbFunc.h"
#include "mdbMemValue.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

using namespace std;

//namespace QuickMDB{

#define CALC_BINARY(MEM_FALG) do{\
if(m_bNull)\
{\
    iRet = CalcBinaryExprNull(pstExpr,(MEM_FALG));\
}\
else\
{\
    iRet = CalcBinaryExpr(pstExpr,(MEM_FALG));\
}\
}while(0)

TMdbExpr::TMdbExpr()
{
	m_lRowPos = 0;
	m_pExprValue = NULL;
	m_bNull = false;
}
TMdbExpr::~TMdbExpr()
{
	SAFE_DELETE(m_pExprValue);
}

//生成一个表达式
ST_EXPR * TMdbExpr::BuildExpr(int op)
{
	Token token;
	token.z = 0;
	token.n = 0;
	return ExprAlloc(op,&token,0);
}
//给expr分配空间
ST_EXPR * TMdbExpr::ExprAlloc(int op,				/* Expression opcode */
							  const Token * pToken, /* Token argument.  Might be NULL */
							  int dequote            /* True to dequote */)
{
	ST_EXPR * pNew = QMDB_MALLOC->AllocExpr();
	pNew->pExprValue = QMDB_MALLOC->AllocMemValue(pNew->pExprValue);
	pNew->iOpcode = op;
	if (pToken)
	{
		if (op == TK_INTEGER)
		{//常量整值
			//pNew->flags |=EP_IntValue;
			char * temp = QMDB_MALLOC->NameFromToken(pToken);
			//pNew->pExprValue->lValue = TMdbStrFunc::StrTollong(temp);
			//int iRet = TMdbNtcStrFunc::StrToInt(temp,pNew->pExprValue->lValue);
            pNew->pExprValue->lValue = TMdbNtcStrFunc::StrToInt(temp);
            /*if(iRet < 0)
            {
                TADD_ERROR(ERROR_UNKNOWN,"data[%s] overflow.",temp);
                SAFE_DELETE_ARRAY(temp);
                return NULL;
            } */
			pNew->pExprValue->iSize  = sizeof(long long);
			MemValueSetProperty(pNew->pExprValue,MEM_Int);
			delete []temp;
		}
		else if(op == TK_STRING)
		{//字符串常量
			pNew->pExprValue->sValue = QMDB_MALLOC->NameFromToken(pToken);
			pNew->pExprValue->iSize  = strlen(pNew->pExprValue->sValue) +1;
			MemValueSetProperty(pNew->pExprValue,MEM_Str|MEM_Dyn);
			/*
			if(TMdbStrFunc::IsNumStr(pNew->pExprValue->sValue ))
			{//若为num也保存其转化后的int值
				pNew->pExprValue->lValue = TMdbStrFunc::StrTollong(pNew->pExprValue->sValue);
				MemValueSetProperty(pNew->pExprValue,MEM_Int);
			}
			*/
			
		}
              else if(TK_NULL == op)
              {//NULL值
                     pNew->pExprValue->SetNull();
              }
		else
		{
			pNew->sTorken = QMDB_MALLOC->NameFromToken(pToken);
		}
	}
	if (dequote)
	{//去除双引号等

	}
	return pNew;
}

//想exprlist添加expr
ST_EXPR_LIST * TMdbExpr::ExprListAppend(
										  ST_EXPR_LIST *pList,        /* List to which to append. Might be NULL */
										  ST_EXPR *pExpr             /* Expression to be appended. Might be NULL */
										  )
{
	if (NULL == pList)
	{
		pList = QMDB_MALLOC->AllocExprList();
	}
	if (pList->iAllocNum <= pList->iItemNum)
	{
		ST_EXPR_ITEM * pHeadItem;
		int iNewNum = pList->iAllocNum*2 + 4;
		pHeadItem = (ST_EXPR_ITEM *)QMDB_MALLOC->ReAlloc(pList->pExprItems,pList->iAllocNum*sizeof(ST_EXPR_ITEM),iNewNum*sizeof(ST_EXPR_ITEM));
		pList->pExprItems = pHeadItem;
		pList->iAllocNum = iNewNum;
	}
	ST_EXPR_ITEM * pItem = &pList->pExprItems[pList->iItemNum++];
	memset(pItem,0,sizeof(*pItem));
	pItem->pExpr = pExpr;
	return pList;
}

/************************************************************************/
/* 创建表达式                                                           */
/************************************************************************/

ST_EXPR * TMdbExpr::BuildPExpr(int op,ST_EXPR * pLeft,ST_EXPR * pRight,const Token * pToken)
{
	ST_EXPR * p = ExprAlloc(op,pToken,1);
	ExprAttachSubtrees(p,pLeft,pRight);
	return  p;
}
//创建函数表达式
ST_EXPR * TMdbExpr::BuildExprFunction(ST_EXPR_LIST * pList,Token *pToken)
{
	ST_EXPR * pNew = ExprAlloc(TK_FUNCTION,pToken,1);
	pNew->pFunc = QMDB_MALLOC->AllocExprFunc();
	pNew->pFunc->pFuncArgs = pList;
	ExprCodeTarget(pNew);
	return pNew;
}

void  TMdbExpr::ExprAttachSubtrees(ST_EXPR * pRoot,ST_EXPR * pLeft,ST_EXPR * pRight)
{
	if (NULL == pRoot)
	{
		
	}else
	{
		if (pLeft)
		{
			pRoot->pLeft = pLeft;
		}
		if (pRight)
		{
			pRoot->pRight =  pRight;
		}
	}
}


void TMdbExpr::ExprListSetName(  ST_EXPR_LIST *pList,        /* List to which to add the span. */
					 Token *pToken,           /* Name to be added */
					 int dequote             /* True to cause the name to be dequoted */)
{
	if( pList ){
		ST_EXPR_ITEM *pItem;
		pItem = &pList->pExprItems[pList->iItemNum-1];
		pItem->sName = QMDB_MALLOC->NameFromToken(pToken);
		if( dequote && pItem->sName ) 
		{//清除双引号
			
		}
		/*
		if(pItem->pExpr && pItem->pExpr->pExprValue)
		{//设置值的名字
			pItem->pExpr->pExprValue->sAlias = pItem->sName;
		}
		*/
  }
}
void  TMdbExpr::ExprListSetSpan(ST_EXPR_LIST *pList,        /* List to which to add the span. */
					  ST_EXPR_SPAN *pSpan         /* The span to be added */
		)
{
	if( pList )
    {
        ST_EXPR_ITEM *pItem = &pList->pExprItems[pList->iItemNum-1];
        if(TK_DOT == pSpan->pExpr->iOpcode 
            && TK_ID_TABLENAME== pSpan->pExpr->pLeft->iOpcode 
            && TK_ID  == pSpan->pExpr->pRight->iOpcode)
        {//对于a.name这种形式，span 为name 而不是a.name
            pItem->sSpan = QMDB_MALLOC->CopyFromStr(pSpan->pExpr->pRight->sTorken);
        }
        else
        {
            Token token;
            token.z = (char*)pSpan->zStart;
            token.n =	(int)(pSpan->zEnd - pSpan->zStart);
            pItem->sSpan = QMDB_MALLOC->NameFromToken(&token);
        }
        /*
        if(pItem->pExpr && pItem->pExpr->pExprValue)
        {//设置值的名字
        pItem->pExpr->pExprValue->sAlias = pItem->sSpan;
        }
        */
    }
}

int TMdbExpr::ExprCodeTarget(ST_EXPR * pstExpr)
{
	if (NULL == pstExpr)
	{
		return 0;
	}
	if (pstExpr->iOpcode == TK_FUNCTION)
	{
		CheckFunction(pstExpr);
	}
	return 0;
}
//检测函数
int TMdbExpr::CheckFunction(ST_EXPR * pstExpr)
{
	//推迟到类型校验的时候做
	return 0;
}

//计算表达式的值
int TMdbExpr::CalcExpr(ST_EXPR * & pstExpr)
{
	int iRet = 0;
	if(NULL == pstExpr)
	{
		return 0;
	}
	if(NULL == pstExpr->pExprValue)
	{//没有value值
		CHECK_RET(ERR_SQL_INVALID,"pstExpr[%s] pExprValue is NULL",TokenName[pstExpr->iOpcode]);
	}
       if(ExprCalcHasProperty(pstExpr,CALC_NO_NEED|CALC_PER_Exec) ||
        ExprCalcHasProperty(pstExpr,CALC_NO_NEED|CALC_PER_Query))
       {//无需计算
            return 0;
       }
	switch (pstExpr->iOpcode)
	{
	case TK_STRING:
	case TK_INTEGER://常量值
	case TK_NULL:
		break;
	case TK_CONST_FUNC:
	case TK_FUNCTION:
	case TK_IN:// in 关键字
		iRet = CalcFunction(pstExpr);//函数计算
		break;
	case TK_PLUS://+
	case TK_MINUS://-
	case TK_STAR://*
	case TK_SLASH:// /
	case TK_REM:// %
	case TK_AND:// and
	case TK_OR:// or
	    CALC_BINARY(MEM_Int|MEM_Date);
		break;
        /*
       case TK_AND:// and
	case TK_OR:// or
	        iRet = CalcAndOrExpr(pstExpr,MEM_Int);
	       break;
	*/
	case TK_LT://<
	case TK_LE://<=
	case TK_GE://>=
	case TK_GT://>
	case TK_EQ://= '='可支持字符串比较
	case TK_NE:
		iRet = CalcBinaryExpr(pstExpr,(MEM_Int|MEM_Str|MEM_Date));
		break;
	case TK_CONCAT:
	    CALC_BINARY(MEM_Str);
		break;
	case TK_ID:
	case TK_VARIABLE:
    case TK_ID_SEQUENCE:
		//无需计算
		break;
	//单元操作符
	case TK_UMINUS: 
	case TK_UPLUS:
	case TK_NOT:
	case TK_BITNOT:
		iRet = CalcUnaryExpr(pstExpr,(MEM_Int));
		break;
       case TK_NOTNULL://
              pstExpr->pExprValue->lValue = pstExpr->pLeft->pExprValue->IsNull()?0:1;
              break;
       case TK_ISNULL:
              pstExpr->pExprValue->lValue = pstExpr->pLeft->pExprValue->IsNull()?1:0;
              break;
	default:
		CHECK_RET(ERR_SQL_CALCULATE_EXP_ERROR,"can`t calc type = [%d]",pstExpr->iOpcode);
		break;
	}
	TADD_FUNC("value=[%s].",ShowExprValue(pstExpr));
       ExprCalcSetProperty(pstExpr,CALC_NO_NEED);//已计算过
	return iRet;
}
//计算函数
int TMdbExpr::CalcFunction(ST_EXPR * & pstExpr)
{
	int iRet = 0;
	CHECK_OBJ_RET_0(pstExpr);
	CHECK_RET(CalcExpr(pstExpr->pLeft),"calc left expr error");//先计算参数值
	CHECK_RET(CalcExpr(pstExpr->pRight),"calc right expr error");
	CHECK_RET(CalcExprList(pstExpr->pFunc->pFuncArgs),"calc func_expr error");
    #if 0
	if(1 != m_lRowPos && 0 != m_lRowPos)
	{
		CHECK_RET(pstExpr->pFunc->pFuncDef->ExecuteStep(),"ExecuteStep error");// 计算
	}
	else
	{
		CHECK_RET(pstExpr->pFunc->pFuncDef->ExecuteFunc(),"ExecuteFunc error");// 计算第一条记录
	}
  #endif
       CHECK_RET(pstExpr->pFunc->pFuncDef->ExecuteStep(),"ExecuteStep error");// 计算
	return iRet;
}
//计算表达式列表
int TMdbExpr::CalcExprList(ST_EXPR_LIST * & pstExprList)
{
	CHECK_OBJ_RET_0(pstExprList);
	int i = 0;
	int iRet = 0;
	for (i = 0;i<pstExprList->iItemNum;i++)
	{
		CHECK_RET(CalcExpr(pstExprList->pExprItems[i].pExpr),"CalcExpr[%d][%s] error",i,
                                                REPLACE_NULL_STR(pstExprList->pExprItems[i].pExpr->sTorken));//计算各个表达式
	}
	TADD_FUNC("Finish.");
	return iRet;
}

/******************************************************************************
* 函数名称	:  CalcUnaryExpr
* 函数描述	:  计算一元运算
* 输入		:  iSubValueType 可支持的子类型
* 输出		:  pstExpr - 计算结果
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbExpr::CalcUnaryExpr(ST_EXPR * & pstExpr,int iSubValueType)
{
	TADD_FUNC("Start.");
	int iRet = 0;
	if(NULL == pstExpr){return 0;}
	if(NULL == pstExpr->pLeft){return -1;}//错误
	CHECK_RET(CalcExpr(pstExpr->pLeft),"calc left expr error");//计算左值
	ST_MEM_VALUE * pLeftValue = pstExpr->pLeft->pExprValue;
	ST_MEM_VALUE * pResultValue = pstExpr->pExprValue;
	
	if( !(MemValueHasAnyProperty(pstExpr->pLeft->pExprValue,iSubValueType)))
	{//错误
		CHECK_RET(ERR_SQL_CALCULATE_EXP_ERROR,"left value[%s] .iSubValueType should be[%d]...",
													ShowExprValue(pstExpr->pLeft),
													iSubValueType);
	}
	else
	{
		switch(pstExpr->iOpcode)
		{
			case TK_UMINUS:// -
				pResultValue->lValue = 0 - pLeftValue->lValue;
				break;
			case TK_UPLUS:// +
				pResultValue->lValue = 0 + pLeftValue->lValue;
				break;
			case TK_NOT:// not
			       pResultValue->lValue = (0==pLeftValue->lValue)?1:0;
				break;
			case TK_BITNOT:
				pResultValue->lValue = 0 ^ pLeftValue->lValue;
				break;
			default:
				iRet = -1;
				break;	
		}
	}
	TADD_FUNC("Finish[].");
	return iRet;
}

//计算and or 表达式
int TMdbExpr::CalcAndOrExpr(ST_EXPR * & pstExpr,int iSubValueType)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(NULL == pstExpr){return 0;}
    CHECK_OBJ(pstExpr->pLeft);
    CHECK_OBJ(pstExpr->pRight);
    CHECK_RET(CalcExpr(pstExpr->pLeft),"calc left expr error");//计算左值
    ST_MEM_VALUE * pLeftValue = pstExpr->pLeft->pExprValue;
    ST_MEM_VALUE * pRightValue = pstExpr->pRight->pExprValue;
    ST_MEM_VALUE * pResultValue = pstExpr->pExprValue;
    switch(pstExpr->iOpcode)
    {
        case TK_AND:// and
            {
                if(0 == pLeftValue->lValue)
                {
                   pResultValue->lValue = 0;
                }
                else
                {
                    CHECK_RET(CalcExpr(pstExpr->pRight),"calc right expr error");//计算右值
                    pResultValue->lValue = (pLeftValue->lValue && pRightValue->lValue)?1:0;  
                }
                break;
            }
        case TK_OR:// or
            {
                if(0 != pLeftValue->lValue)
                {
                   pResultValue->lValue = 1;
                }
                else
                {
                    CHECK_RET(CalcExpr(pstExpr->pRight),"calc right expr error");//计算右值
                    pResultValue->lValue = (pLeftValue->lValue || pRightValue->lValue)?1:0;
                }
                break; 
            }
        default:
            iRet = -1;
            break;
    }
    TADD_FUNC("Finish.iRet[%d].result value[%s]",iRet,ShowExprValue(pstExpr));
    return iRet;
}


//计算双元运算符iSubValueType = 可支持的子类型
//+ - * / and or ||根据系统配置文件中的is-null设置来使用新规则

int TMdbExpr::CalcBinaryExprNull(ST_EXPR * & pstExpr,int iSubValueType)
{
	TADD_FUNC("Start.");
	int iRet = 0;
	if(NULL == pstExpr){return 0;}
      CHECK_OBJ(pstExpr->pLeft);
      CHECK_OBJ(pstExpr->pRight);
      CHECK_RET(CalcExpr(pstExpr->pLeft),"calc left expr error");//计算左值
      CHECK_RET(CalcExpr(pstExpr->pRight),"calc right expr error");//计算右值
	
	ST_MEM_VALUE * pLeftValue = pstExpr->pLeft->pExprValue;
	ST_MEM_VALUE * pRightValue = pstExpr->pRight->pExprValue;
	ST_MEM_VALUE * pResultValue = pstExpr->pExprValue;
	
	if( !(MemValueHasAnyProperty(pstExpr->pLeft->pExprValue,iSubValueType)&&
		MemValueHasAnyProperty(pstExpr->pRight->pExprValue,iSubValueType)))
	{//错误
		CHECK_RET(ERR_SQL_CALCULATE_EXP_ERROR,"left value[%s] right value[%s].iSubValueType should be[%d]...",
													ShowExprValue(pstExpr->pLeft),
													ShowExprValue(pstExpr->pRight),
													iSubValueType);
	}
	else
	{
	    if(MemValueHasAnyProperty(pResultValue,MEM_Null))
	    {//含有上次的残留数据，需要清理
    	    MemValueClearProperty(pResultValue,MEM_Null);
	    }
		switch(pstExpr->iOpcode)
		{
			case TK_PLUS://+
			    iRet = 
			    TMdbMemValue::CalcMemValue(pstExpr->iOpcode,pResultValue,pLeftValue,pRightValue,m_bNull);
				break;
			case TK_MINUS://-
			 	iRet = 
			 	TMdbMemValue::CalcMemValue(pstExpr->iOpcode,pResultValue,pLeftValue,pRightValue,m_bNull);
				break;
			case TK_STAR://*
			    TMdbMemValue::CalcMemValueForStar(pResultValue,pLeftValue,pRightValue,m_bNull);
				break;
			case TK_SLASH:// /
                if(pRightValue->IsNull())	
                {
                    return -1;
                }
                else if(pLeftValue->IsNull())
                {
                    pResultValue->SetNull();
                }
				else if(pRightValue->lValue != 0)
				{
					pResultValue->lValue = pLeftValue->lValue / pRightValue->lValue;
				}
				else
				{//除数为0
					return -1;
				}
				break;
			case TK_REM:// %
                if(pRightValue->IsNull())	
                {
                    return -1;
                }
                else if(pLeftValue->IsNull())
                {
                    pResultValue->SetNull();
                }
				else if(pRightValue->lValue != 0)
				{
					pResultValue->lValue = pLeftValue->lValue % pRightValue->lValue;
				}
				else
				{//除数为0
					return -1;
				}
				break;
			case TK_AND:// and
                if(pLeftValue->IsNull() || pRightValue->IsNull())
                {
                    pResultValue->lValue = 0;
                }
                else
                {
      				pResultValue->lValue = (pLeftValue->lValue && pRightValue->lValue)?1:0;
				}
				break;
			case TK_OR:// or
                if(pLeftValue->IsNull() && pRightValue->IsNull())
                {
                    pResultValue->lValue = 0;
                }
                else if(pLeftValue->IsNull())
                {
                    pResultValue->lValue = (pRightValue->lValue)?1:0;
                }
                else if(pRightValue->IsNull())
                {
                    pResultValue->lValue = (pLeftValue->lValue)?1:0;
                }
                else
                {
      				pResultValue->lValue = (pLeftValue->lValue || pRightValue->lValue)?1:0;
				}
				break;
			case TK_CONCAT:// || 字符串连接
				memset(pResultValue->sValue,0x00,pResultValue->iSize);
				if((int)strlen(pLeftValue->sValue) + (int)strlen(pRightValue->sValue) >= pResultValue->iSize)
				{
					CHECK_RET(ERR_SQL_CALCULATE_EXP_ERROR,"left[%s]+right[%s]>size[%d].",
						pLeftValue->sValue,pRightValue->sValue,pResultValue->iSize);
				}
				if(false == pLeftValue->IsNull())
				{
      				strcat(pResultValue->sValue,pLeftValue->sValue);
				}
				if(false == pRightValue->IsNull())
				{
      				strcat(pResultValue->sValue,pRightValue->sValue);
				}
				break;
			default:
				iRet = -1;
				break;
		}
	}
	TADD_FUNC("Finish.iRet[%d].result value[%s]",iRet,ShowExprValue(pstExpr));
	return iRet;
}

//计算双元运算符iSubValueType = 可支持的子类型
int TMdbExpr::CalcBinaryExpr(ST_EXPR * & pstExpr,int iSubValueType)
{
	TADD_FUNC("Start.");
	int iRet = 0;
	if(NULL == pstExpr){return 0;}
      CHECK_OBJ(pstExpr->pLeft);
      CHECK_OBJ(pstExpr->pRight);
      CHECK_RET(CalcExpr(pstExpr->pLeft),"calc left expr error");//计算左值
      CHECK_RET(CalcExpr(pstExpr->pRight),"calc right expr error");//计算右值
	
	ST_MEM_VALUE * pLeftValue = pstExpr->pLeft->pExprValue;
	ST_MEM_VALUE * pRightValue = pstExpr->pRight->pExprValue;
	ST_MEM_VALUE * pResultValue = pstExpr->pExprValue;
	
	if( !(MemValueHasAnyProperty(pstExpr->pLeft->pExprValue,iSubValueType)&&
		MemValueHasAnyProperty(pstExpr->pRight->pExprValue,iSubValueType)))
	{//错误
		CHECK_RET(ERR_SQL_CALCULATE_EXP_ERROR,"left value[%s] right value[%s].iSubValueType should be[%d]...",
													ShowExprValue(pstExpr->pLeft),
													ShowExprValue(pstExpr->pRight),
													iSubValueType);
	}
	else
	{
	    if(MemValueHasAnyProperty(pResultValue,MEM_Null))
	    {//含有上次的残留数据，需要清理
    	    MemValueClearProperty(pResultValue,MEM_Null);
	    }
		switch(pstExpr->iOpcode)
		{
			case TK_PLUS://+
			    iRet = 
			    TMdbMemValue::CalcMemValue(pstExpr->iOpcode,pResultValue,pLeftValue,pRightValue);
				break;
			case TK_MINUS://-
			 	iRet = 
			 	TMdbMemValue::CalcMemValue(pstExpr->iOpcode,pResultValue,pLeftValue,pRightValue);
				break;
			case TK_STAR://*
                {//date类型相减的结果乘积，增加精度
                    TMdbMemValue::CalcMemValueForStar(pResultValue,pLeftValue,pRightValue);
    				break;
				}
			case TK_SLASH:// /
				if(pRightValue->lValue != 0)
				{
					pResultValue->lValue = pLeftValue->lValue / pRightValue->lValue;
				}
				else
				{//除数为0
					return -1;
				}
				break;
			case TK_REM:// %
				if(pRightValue->lValue != 0)
				{
					pResultValue->lValue = pLeftValue->lValue % pRightValue->lValue;
				}
				else
				{//除数为0
					return -1;
				}
				break;
			case TK_LT://<
			    if(pRightValue->IsNull() || pLeftValue->IsNull()) 
			    {
    			    pResultValue->lValue = 0;
			    }
			    else 
			    {
    				pResultValue->lValue = (TMdbMemValue::CompareExprValue(pLeftValue,pRightValue,false) < 0)?1:0;
				}
				break;
			case TK_LE://<=
			    if(pRightValue->IsNull() || pLeftValue->IsNull()) 
			    {
    			    pResultValue->lValue = 0;
			    }
			    else 
			    {
    				pResultValue->lValue = (TMdbMemValue::CompareExprValue(pLeftValue,pRightValue,false) <= 0)?1:0;
				}
				break;
			case TK_EQ://= '='可支持字符串比较
			    if(pRightValue->IsNull() || pLeftValue->IsNull()) 
			    {
    			    pResultValue->lValue = 0;
			    }
				else if(pRightValue->IsLPM() || pLeftValue->IsLPM() )
				{	
					pResultValue->lValue = 1;
				}
			    else 
			    {
                    pResultValue->lValue = (TMdbMemValue::CompareExprValue(pLeftValue,pRightValue,false) == 0)?1:0;
				}
				break;
			case TK_NE://<>
			    if(pRightValue->IsNull() || pLeftValue->IsNull()) 
			    {
    			    pResultValue->lValue = 0;
			    }
			    else 
			    {
                    pResultValue->lValue = (TMdbMemValue::CompareExprValue(pLeftValue,pRightValue,false) == 0)?0:1;
				}
				break;
			case TK_GE://>=
			    if(pRightValue->IsNull() || pLeftValue->IsNull()) 
			    {
    			    pResultValue->lValue = 0;
			    }
			    else 
			    {
                    pResultValue->lValue = (TMdbMemValue::CompareExprValue(pLeftValue,pRightValue,false) >= 0)?1:0;
				}
			
				break;
			case TK_GT://>
			    if(pRightValue->IsNull() || pLeftValue->IsNull()) 
			    {
    			    pResultValue->lValue = 0;
			    }
			    else 
			    {
                    pResultValue->lValue = (TMdbMemValue::CompareExprValue(pLeftValue,pRightValue,false) > 0)?1:0;
				}
			
				break;
			case TK_AND:// and
                {
      				pResultValue->lValue = (pLeftValue->lValue && pRightValue->lValue)?1:0;
				}
				break;
			case TK_OR:// or
                {
      				pResultValue->lValue = (pLeftValue->lValue || pRightValue->lValue)?1:0;
				}
				break;
			case TK_CONCAT:// || 字符串连接
				memset(pResultValue->sValue,0x00,pResultValue->iSize);
				if((int)strlen(pLeftValue->sValue) + (int)strlen(pRightValue->sValue) >= pResultValue->iSize)
				{
					CHECK_RET(ERR_SQL_CALCULATE_EXP_ERROR,"left[%s]+right[%s]>size[%d].",
						pLeftValue->sValue,pRightValue->sValue,pResultValue->iSize);
				}
  				strcat(pResultValue->sValue,pLeftValue->sValue);
  				strcat(pResultValue->sValue,pRightValue->sValue);
				break;
			default:
				iRet = -1;
				break;
		}
	}
	TADD_FUNC("Finish.iRet[%d].result value[%s]",iRet,ShowExprValue(pstExpr));
	return iRet;
}

/******************************************************************************
* 函数名称	:  DeleteExpr
* 函数描述	:  从list中删除expr
* 输入		:  pstExprList - 从该list中删除
* 输入		:  pstExprToDel - 准备删除的expr
* 输出		:  无
* 返回值	: ST_EXPR_LIST -  删除后的exprlist
* 作者		:  jin.shaohua
*******************************************************************************/

ST_EXPR_LIST * TMdbExpr::DeleteExpr(ST_EXPR_LIST * pstExprList,ST_EXPR * pstExprToDel)
{
	int i = 0;
	if(NULL == pstExprList || NULL == pstExprToDel){return pstExprList;}
	for(i = 0;i<pstExprList->iItemNum;i++)
	{
		ST_EXPR_ITEM & item = pstExprList->pExprItems[i];
		if(item.pExpr == pstExprToDel)
		{//find  delete
		       QMDB_MALLOC->ReleaseExpr(item.pExpr);
                	SAFE_DELETE_ARRAY(item.sName);
                	SAFE_DELETE_ARRAY(item.sSpan);
			if(i != pstExprList->iItemNum - 1)
			{//不是最后一个需要将后面的移到前面去
			    memmove(pstExprList->pExprItems + i,pstExprList->pExprItems + i + 1,
					(pstExprList->iItemNum-i-1)*sizeof(ST_EXPR_ITEM));
			}
			//清空最后一个元素
			memset(pstExprList->pExprItems+pstExprList->iItemNum - 1,0x00,sizeof(ST_EXPR_ITEM));
			pstExprList->iItemNum --;
			break;
		}
	}
	return pstExprList;
}
/******************************************************************************
* 函数名称	:  ShowExprValue
* 函数描述	:  只是显示expr 的value
* 输入		:  pstExpr - expr
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
const char * TMdbExpr::ShowExprValue(const ST_EXPR * pstExpr)
{
	//申请一块空间放置expr 值
	 if(NULL == m_pExprValue)
	 {
		m_pExprValue = new (std::nothrow)char[4096];
		if(NULL == m_pExprValue)
		{
			return "nil";
		}
	 }
	 if(NULL == pstExpr || NULL == pstExpr->pExprValue)
	 {//没有值
		sprintf(m_pExprValue,"nil");
	 }
	 else
	 {	
	 	sprintf(m_pExprValue,"[%s][%s][type:%d|l_v:%lld|s_v:%s].",
										TokenName[pstExpr->iOpcode],
										ReplaceNull(pstExpr->sTorken),
										pstExpr->pExprValue->iFlags,
										pstExpr->pExprValue->lValue,
										ReplaceNull(pstExpr->pExprValue->sValue));
	 }
	 return m_pExprValue;
}
/******************************************************************************
* 函数名称	:  ReplaceNull
* 函数描述	:  替换NULL
* 输入		:  pStr
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
const char *  TMdbExpr::ReplaceNull(const char * pStr)
{
	if(NULL == pStr)
	{
		return "nil";
	}
	else
	{
		return pStr;
	}	
}
/******************************************************************************
* 函数名称	:  ContainColumn
* 函数描述	:  查看是否包含column列
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
bool TMdbExpr::IsContainColumn(ST_EXPR * pstExpr)
{
    return IsContainOpcode(pstExpr,TK_ID);
}

/******************************************************************************
* 函数名称	:  IsContainOpcode
* 函数描述	:  是否包含某操作码
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
bool TMdbExpr::IsContainOpcode(ST_EXPR * pstExpr,int iOpcode)
{
    if(NULL == pstExpr){return false;}
    if(iOpcode == pstExpr->iOpcode){return true;}
    if(true == IsContainOpcode(pstExpr->pLeft,iOpcode) || true== IsContainOpcode(pstExpr->pRight,iOpcode))
    {
        return true;
    }
    return false;
}

/******************************************************************************
* 函数名称	:  RemoveTableAlias
* 函数描述	:  去除表别名
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbExpr::RemoveTableAlias(ST_EXPR * & pstExpr,const char * sTableAlias)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(NULL == pstExpr){return iRet;}
    switch(pstExpr->iOpcode)
    {
        case TK_IN:
            {//函数中支持别名
                CHECK_RET(RemoveTableAlias(pstExpr->pLeft,sTableAlias),"RemoveTableAlias failed.");
                ST_EXPR_LIST *pColList = pstExpr->pFunc->pFuncArgs;
                if(NULL == pColList) break;
                for(int i = 0; i<pColList->iItemNum; ++i)
                {
                    CHECK_RET(RemoveTableAlias(pColList->pExprItems[i].pExpr,sTableAlias),"RemoveTableAlias failed.");
                }
            }
            break;
        case TK_FUNCTION:
            {//函数中支持别名
                ST_EXPR_LIST *pColList = pstExpr->pFunc->pFuncArgs;
                if(NULL == pColList) break;
                for(int i = 0; i<pColList->iItemNum; ++i)
                {
                    CHECK_RET(RemoveTableAlias(pColList->pExprItems[i].pExpr,sTableAlias),"RemoveTableAlias failed.");
                }
            }
            break;
        case TK_DOT:
            {
                CHECK_OBJ(pstExpr->pLeft);
                CHECK_OBJ(pstExpr->pRight);
                if(TK_ID_TABLENAME == pstExpr->pLeft->iOpcode)
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pstExpr->pLeft->sTorken,sTableAlias) != 0 )
                    {
                         CHECK_RET(ERR_SQL_INVALID,"name[%s] is diff from table alias[%s]",
                            pstExpr->pLeft->sTorken,sTableAlias);
                    }
                    QMDB_MALLOC->ReleaseExpr(pstExpr->pLeft);//先删除左枝
                    ST_EXPR * pstOld = pstExpr;
                    pstExpr = pstExpr->pRight;
                    pstOld->pRight = NULL;
                    QMDB_MALLOC->ReleaseExpr(pstOld);//再删除TK_DOT
                }
            }
            break;
        default:
            {//递归检查
                CHECK_RET(RemoveTableAlias(pstExpr->pLeft,sTableAlias),"RemoveTableAlias failed.");
                CHECK_RET(RemoveTableAlias(pstExpr->pRight,sTableAlias),"RemoveTableAlias failed.");
            }
            break;
        
    
    }
    #if 0
    if(TK_DOT == pstExpr->iOpcode)
    {
        CHECK_OBJ(pstExpr->pLeft);
        CHECK_OBJ(pstExpr->pRight);
        if(TK_ID_TABLENAME == pstExpr->pLeft->iOpcode)
        {
            if(TMdbStrFunc::StrNoCaseCmp(pstExpr->pLeft->sTorken,sTableAlias) != 0 )
            {
                 CHECK_RET(ERROR_SQL_INVALID,"name[%s] is diff from table alias[%s]",
                    pstExpr->pLeft->sTorken,sTableAlias);
            }
            QMDB_MALLOC->ReleaseExpr(pstExpr->pLeft);//先删除左枝
            ST_EXPR * pstOld = pstExpr;
            pstExpr = pstExpr->pRight;
            pstOld->pRight = NULL;
            QMDB_MALLOC->ReleaseExpr(pstOld);//再删除TK_DOT
        }
    }
    else
    {//递归检查
        CHECK_RET(RemoveTableAlias(pstExpr->pLeft,sTableAlias),"RemoveTableAlias failed.");
        CHECK_RET(RemoveTableAlias(pstExpr->pRight,sTableAlias),"RemoveTableAlias failed.");
    }
    #endif
    TADD_FUNC("Finish.");
    return iRet;
}

//}

