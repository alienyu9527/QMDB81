/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbFunc.cpp		
*@Description： mdb解析所用到的函数类
*@Author:		jin.shaohua
*@Date：	    2012.06
*@History:
******************************************************************************************/
//#include "BillingSDK.h"
#include <iostream>
#include "Helper/mdbFunc.h"
#include "Helper/mdbMemValue.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbBase.h"
#include "Interface/mdbQuery.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{
	#define ArraySize(X) (sizeof(X)/sizeof(X[0]))
#define MAX_VALUE(_x,_y) (_x)<(_y)?(_y):(_x)
/******************************************************************************
* 函数名称	:  GetFunction
* 函数描述	:  获取函数信息
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbFuncFactory::GetFunction(ST_EXPR * pstExpr,ST_SQL_STRUCT * pstSqlStruct)
{
	TADD_FUNC("Start.");
	int iRet = 0;
	CHECK_OBJ(pstExpr);
	TMdbFuncBase * pFuncDef = NULL;
	switch(pstExpr->iOpcode)
	{
		case TK_IN:
		case TK_CONST_FUNC:
		case TK_FUNCTION:
			{
				pFuncDef = GetFunctionByName(pstExpr->sTorken);
				if(NULL == pFuncDef){CHECK_RET(ERR_SQL_FUNCTION_NOT_SUPPORT,"not support function[%s].",pstExpr->sTorken);}
				CHECK_RET(pFuncDef->InitFunction(pstExpr,pstSqlStruct),"InitFunction[%s] failed.",pstExpr->sTorken);
				pstExpr->pFunc->pFuncDef = pFuncDef;
			}
			break;
		default:
			CHECK_RET(-1,"[%s] is not a function.",TokenName[pstExpr->iOpcode]);
			break;
	}
	TADD_FUNC("Finish.");
	return iRet;
}


/******************************************************************************
* 函数名称	:  GetFunctionByName
* 函数描述	:  根据函数名获取函数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  NULL - 失败,!NULL - 成功
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbFuncBase* TMdbFuncFactory::GetFunctionByName(const char * sName)
{
	TADD_FUNC("Start.[%s].",sName);
	TMdbFuncBase* pRetFunc = NULL;
	if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"in") == 0)
	{
		pRetFunc = new TMdbInFunc();
	}else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"count") == 0)
	{
		pRetFunc = new TMdbCountFunc();
	}
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"sysdate") == 0)
	{
		pRetFunc = new TMdbSysdateFunc();
	}
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"max") == 0)
	{
		pRetFunc = new TMdbMaxFunc();
	}
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"min") == 0)
	{
		pRetFunc = new TMdbMinFunc();
	}
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"to_date") == 0)
	{
		pRetFunc = new TMdbToDateFunc();
	}
    	else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"to_char") == 0)
	{
		pRetFunc = new TMdbToCharFunc();
	}
       else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"sum") == 0)
	{
		pRetFunc = new TMdbSumFunc();
	}
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"nvl") == 0)
	{
		pRetFunc = new TMdbNVLFunc();
	}
       else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"blobtochar") == 0)
	{
		pRetFunc = new TMdbBlobToCharFunc();
	}
       else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"add_seconds") == 0)
	{
		pRetFunc = new TMdbAddSecondsFunc();
	}
       else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"nextval") == 0)
	{
		pRetFunc = new TMdbNextvalFunc();
	}
       else if(TMdbNtcStrFunc::StrNoCaseCmp(sName,"currval") == 0)
	{
		pRetFunc = new TMdbCurrvalFunc();
	}
	TADD_FUNC("Finish.pRetFunc=[%p].",pRetFunc);
	return pRetFunc;
}



/******************************************************************************
* 函数名称	:  TMdbFuncBase
* 函数描述	:  函数基类
* 输入		:  
* 输出		:  
* 返回值	: 
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbFuncBase::TMdbFuncBase()
{
	memset(m_sName,0x00,sizeof(m_sName));
	m_iArgc = -1;
	m_iFlags = 0;
	memset(&m_stFuncContext,0x00,sizeof(m_stFuncContext));
	m_vArgv.clear();
       memset(m_sDsn,0,sizeof(m_sDsn));
}
/******************************************************************************
* 函数名称	:  ~TMdbFuncBase
* 函数描述	:  函数基类析构
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbFuncBase::~TMdbFuncBase()
{

}

/******************************************************************************
* 函数名称	:  InitFunction
* 函数描述	:  初始化函数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbFuncBase::InitFunction(ST_EXPR * pstExpr,ST_SQL_STRUCT * pstSqlStruct)
{
	TADD_FUNC("Start.");
	int iRet = 0;
	CHECK_OBJ(pstExpr);
	TADD_DETAIL("m_iArgc = [%d].",m_iArgc);
	//校验参数
	if(NULL == pstExpr->pFunc->pFuncArgs && -1 != m_iArgc)
	{
		CHECK_RET(-1,"real argc[%d] not match function argc[%d]",0,m_iArgc);
	}
	if(NULL != pstExpr->pFunc->pFuncArgs && -1 != m_iArgc && pstExpr->pFunc->pFuncArgs->iItemNum != m_iArgc)
	{
		CHECK_RET(-1,"real argc[%d] not match function argc[%d]",pstExpr->pFunc->pFuncArgs->iItemNum,m_iArgc);
	}
	m_stFuncContext.ReturnValue = pstExpr->pExprValue;//记录返回值
	m_pstExpr = pstExpr;
       CHECK_OBJ(pstSqlStruct);
       m_tTypeCheck.SetSqlStruct(pstSqlStruct);
       SAFESTRCPY(m_sDsn,sizeof(m_sDsn),pstSqlStruct->pShmDSN->GetInfo()->sName);
	CHECK_RET(CheckArgv(),"Check args failed.");
	TADD_FUNC("Finish.");
	return iRet;
}
/******************************************************************************
* 函数名称	:  GetValueMapIndex
* 函数描述	:  
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  >=0  - 成功<0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbFuncBase::GetValueMapIndex(const ST_INT_STR_MAP *pstIntStrMap,int iMapSize,const char * sName)
{
	int i = 0;
	for(i = 0;i<iMapSize;i++)
	{
		if(TMdbNtcStrFunc::StrNoCaseCmp(pstIntStrMap[i].sValue,sName) == 0)
		{
			return pstIntStrMap[i].iValue;
		}
	}
	return -1;
}

/******************************************************************************
* 函数名称	:  TMdbInFunc
* 函数描述	:  
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbInFunc::TMdbInFunc()
{
	m_iArgc = -1;
}
TMdbInFunc::~TMdbInFunc()
{
	
}

/******************************************************************************
* 函数名称	:  ExecuteFunc
* 函数描述	:   函数执行
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbInFunc::ExecuteFunc()
{
	int iResult = 0;
	//int i = 0;
	std::vector<ST_MEM_VALUE *>::iterator itor = m_vArgv.begin();
	ST_MEM_VALUE * pOutMemValue = *itor;
	for(++itor;itor != m_vArgv.end();++itor)
	{
		if(TMdbMemValue::CompareExprValue(pOutMemValue,*itor) == 0)
		{
			iResult = 1;
			break;
		}
	}
	m_stFuncContext.ReturnValue->lValue = iResult;
	return 0;
}
/******************************************************************************
* 函数名称	:  CheckArgv
* 函数描述	:  校验函数的参数
* 输入		: in函数的格式是 left_expr in (expr_list)
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbInFunc::CheckArgv()
{
	int iRet = 0;
	m_vArgv.clear();
	SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Int,sizeof(long long));//结果值确定为int
	CHECK_RET(m_tTypeCheck.AdaptAllExprType(m_pstExpr->pLeft,MEM_Int|MEM_Str|MEM_Date,MAX_BLOB_LEN),
		"AdaptAllExprType error.");
	m_vArgv.push_back(m_pstExpr->pLeft->pExprValue);//设定左值
	int i = 0;
	ST_EXPR_LIST * pExprList = m_pstExpr->pFunc->pFuncArgs;
	CHECK_OBJ(pExprList);
	for(i = 0;i < pExprList->iItemNum;i++)
	{
		m_tTypeCheck.AdaptAllExprType(pExprList->pExprItems[i].pExpr,m_pstExpr->pLeft->pExprValue->iFlags,
			m_pstExpr->pLeft->pExprValue->iSize);
		m_vArgv.push_back(pExprList->pExprItems[i].pExpr->pExprValue);
	}
	return iRet;
}



/******************************************************************************
* 函数名称	:  TMdbCountFunc
* 函数描述	:  
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbCountFunc::TMdbCountFunc()
{
	FuncSetProperty(this,FUNC_AGGREAGATE);
	m_iArgc = -1;
}
TMdbCountFunc::~TMdbCountFunc()
{
	
}

/******************************************************************************
* 函数名称	:  ExecuteFunc
* 函数描述	:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbCountFunc::ExecuteFunc()
{
	m_stFuncContext.ReturnValue->lValue = 1;
	return 0;
}
/******************************************************************************
* 函数名称	:  ExecuteStep
* 函数描述	:  聚合函数的单步执行
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbCountFunc::ExecuteStep()
{
	m_stFuncContext.ReturnValue->lValue ++;
	return 0;
}

/******************************************************************************
* 函数名称	:  CheckArgv
* 函数描述	:  
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbCountFunc::CheckArgv()
{
	int iRet = 0;
	SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Int,sizeof(long long));//结果值确定为int
	return iRet;
}



/******************************************************************************
* 函数名称	:  TMdbSysdateFunc
* 函数描述	:  
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbSysdateFunc::TMdbSysdateFunc()
{
	m_iArgc = -1;
}
TMdbSysdateFunc::~TMdbSysdateFunc()
{
	
}

/******************************************************************************
* 函数名称	:  ExecuteFunc
* 函数描述	:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSysdateFunc::ExecuteFunc()
{
	TMdbDateTime::GetCurrentTimeStr(m_stFuncContext.ReturnValue->sValue);
	return 0;
}
/******************************************************************************
* 函数名称	:  CheckArgv
* 函数描述	:  
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSysdateFunc::CheckArgv()
{
	int iRet = 0;
	SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Str|MEM_Date,MAX_TIME_LEN);//结果值确定为int
	return iRet;
}




/******************************************************************************
* 函数名称	:  TMdbMaxFunc
* 函数描述	:  取最大值
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbMaxFunc::TMdbMaxFunc()
{
	FuncSetProperty(this,FUNC_AGGREAGATE);
	m_iArgc = 1;
}
TMdbMaxFunc::~TMdbMaxFunc()
{

}

/******************************************************************************
* 函数名称	:  ExecuteFunc
* 函数描述	:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbMaxFunc::ExecuteFunc()
{
	m_stFuncContext.ReturnValue->lValue =  m_vArgv[0]->lValue;
	return 0;
}

/******************************************************************************
* 函数名称	:  ExecuteStep
* 函数描述	:  聚合函数的单步执行
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbMaxFunc::ExecuteStep()
{
       if(0 == m_stFuncContext.ReturnValue->iAggCalcCounts ||  
        m_stFuncContext.ReturnValue->lValue  < m_vArgv[0]->lValue)
       {
            m_stFuncContext.ReturnValue->lValue =  m_vArgv[0]->lValue;
       } 
       m_stFuncContext.ReturnValue->iAggCalcCounts ++;
	return 0;
}

/******************************************************************************
* 函数名称	:  CheckArgv
* 函数描述	:  校验参数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbMaxFunc::CheckArgv()
{
	int iRet = 0;
	m_vArgv.clear();
	SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Int,sizeof(long long));//结果值确定为int
	CHECK_RET(m_tTypeCheck.AdaptAllExprType(m_pstExpr->pFunc->pFuncArgs[0].pExprItems[0].pExpr,
			MEM_Int,sizeof(long long)),
		"AdaptAllExprType error.");
	m_vArgv.push_back(m_pstExpr->pFunc->pFuncArgs[0].pExprItems[0].pExpr->pExprValue);
	return iRet;
}

/******************************************************************************
* 函数名称	:  TMdbMaxFunc
* 函数描述	:  取最大值
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbMinFunc::TMdbMinFunc()
{
	FuncSetProperty(this,FUNC_AGGREAGATE);
	m_iArgc = 1;
}
TMdbMinFunc::~TMdbMinFunc()
{

}
int TMdbMinFunc::ExecuteFunc()
{
	m_stFuncContext.ReturnValue->lValue  =  m_vArgv[0]->lValue;
	return 0;
}

/******************************************************************************
* 函数名称	:  ExecuteStep
* 函数描述	:  聚合函数的单步执行
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbMinFunc::ExecuteStep()
{
	if(0 == m_stFuncContext.ReturnValue->iAggCalcCounts ||  
        m_stFuncContext.ReturnValue->lValue  > m_vArgv[0]->lValue)
	{
		m_stFuncContext.ReturnValue->lValue  =  m_vArgv[0]->lValue;
	}
       m_stFuncContext.ReturnValue->iAggCalcCounts ++;
	return 0;

}
/******************************************************************************
* 函数名称	:  CheckArgv
* 函数描述	:  检查参数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbMinFunc::CheckArgv()
{
	int iRet = 0;
	m_vArgv.clear();
	SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Int,sizeof(long long));//结果值确定为int
	CHECK_RET(m_tTypeCheck.AdaptAllExprType(m_pstExpr->pFunc->pFuncArgs[0].pExprItems[0].pExpr,
			MEM_Int,sizeof(long long)),
		"AdaptAllExprType error.");
	m_vArgv.push_back(m_pstExpr->pFunc->pFuncArgs[0].pExprItems[0].pExpr->pExprValue);
	return iRet;
}


/******************************************************************************
* 函数名称	:  TMdbSumFunc
* 函数描述	:  取最大值
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbSumFunc::TMdbSumFunc()
{
	FuncSetProperty(this,FUNC_AGGREAGATE);
	m_iArgc = 1;
}
TMdbSumFunc::~TMdbSumFunc()
{

}
int TMdbSumFunc::ExecuteFunc()
{
	m_stFuncContext.ReturnValue->lValue  =  m_vArgv[0]->lValue;
	return 0;
}

/******************************************************************************
* 函数名称	:  ExecuteStep
* 函数描述	:  聚合函数的单步执行
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSumFunc::ExecuteStep()
{
       m_stFuncContext.ReturnValue->lValue  +=  m_vArgv[0]->lValue;
	return 0;
}
/******************************************************************************
* 函数名称	:  CheckArgv
* 函数描述	:  检查参数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbSumFunc::CheckArgv()
{
	int iRet = 0;
	m_vArgv.clear();
	SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Int,sizeof(long long));//结果值确定为int
	CHECK_RET(m_tTypeCheck.AdaptAllExprType(m_pstExpr->pFunc->pFuncArgs[0].pExprItems[0].pExpr,
			MEM_Int,sizeof(long long)),"AdaptAllExprType error.");
	m_vArgv.push_back(m_pstExpr->pFunc->pFuncArgs[0].pExprItems[0].pExpr->pExprValue);
	return iRet;
}


/******************************************************************************
* 函数名称	:  TToDateFunc
* 函数描述	:  时间处理函数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbToDateFunc::TMdbToDateFunc()
{
	m_iArgc = 2;//两个参数
}
TMdbToDateFunc::~TMdbToDateFunc()
{

}
/******************************************************************************
* 函数名称	:  ExecuteFunc
* 函数描述	:  时间函数执行
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbToDateFunc::ExecuteFunc()
{
	int iRet = 0;
       if(m_vArgv[0]->IsNull())
       {//设置NULL
            m_stFuncContext.ReturnValue->SetNull();
            return iRet;
       }
       m_stFuncContext.ReturnValue->ClearValue();
	char * sSrcDate = m_vArgv[0]->sValue;//原始字符串
	char * sDesDate = m_stFuncContext.ReturnValue->sValue;
	int iSrcDateLen = strlen(sSrcDate);
	switch(m_iDateType)
	{
		case E_LONG_YEAR://YYYY-MM-DD
			{
				if(10 == iSrcDateLen)
				{				
					memcpy(sDesDate,sSrcDate,4);
					memcpy(sDesDate+4,sSrcDate+5,2);
					memcpy(sDesDate+6,sSrcDate+8,2);
					SAFESTRCPY(sDesDate+8,7,"000000");
				}
				else
				{
					CHECK_RET(ERR_SQL_TYPE_INVALID,"[%s] is not a date[YYYY-MM-DD].",sSrcDate);
				}

			}
			break;
		case E_LONG_YEAR_TIME://YYYY-MM-DD hh24:mi:ss
			{
				if(19 == iSrcDateLen)
				{				
					memcpy(sDesDate,sSrcDate,4);
					memcpy(sDesDate+4,sSrcDate+5,2);
					memcpy(sDesDate+6,sSrcDate+8,2);
					memcpy(sDesDate+8,sSrcDate+11,2);
					memcpy(sDesDate+10,sSrcDate+14,2);
					memcpy(sDesDate+12,sSrcDate+17,2);
				}
				else
				{
					CHECK_RET(ERR_SQL_TYPE_INVALID,"[%s] is not a date[YYYY-MM-DD hh24:mi:ss].",sSrcDate);
				}
			}
			break;
		case E_SHORT_YEAR://YYYYMMDD
			{
				if(8 == iSrcDateLen)
				{				
					memcpy(sDesDate,sSrcDate,8);
					SAFESTRCPY(sDesDate+8,7,"000000");
				}
				else
				{
					CHECK_RET(ERR_SQL_TYPE_INVALID,"[%s] is not a date[YYYYMMDD].",sSrcDate);
				}
			}
			break;
		case E_SHORT_YEAR_TIME://YYYYMMDDhh24miss
			if(14 == iSrcDateLen)
			{				
				memcpy(sDesDate,sSrcDate,14);
			}
			else
			{
				CHECK_RET(ERR_SQL_TYPE_INVALID,"[%s] is not a date[YYYYMMDDhh24miss].",sSrcDate);
			}
			break;
		default:
			break;
	}
	sDesDate[14] = '\0';
	return iRet;
}
/******************************************************************************
* 函数名称	:  CheckArgv
* 函数描述	:  检查参数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
/*
E_LONG_YEAR 	 = 1, //YYYY-MM-DD
E_LONG_TIME 	 = 2, //hh24:mi:ss
E_LONG_YEAR_TIME = 3, //YYYY-MM-DD hh24:mi:ss
E_SHORT_YEAR	 = 4, //YYYYMMDD
E_SHORT_TIME	 = 5, //hh24miss
E_SHORT_YEAR_TIME= 6  //YYYYMMDDhh24miss

*/
int TMdbToDateFunc::CheckArgv()
{
	int iRet = 0;
	m_vArgv.clear();
	SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Date,MAX_TIME_LEN);//结果值确定为date类型
	ST_EXPR_LIST * pExprList = m_pstExpr->pFunc->pFuncArgs;
	ST_EXPR * pDateTypeExpr  = pExprList->pExprItems[1].pExpr;
	if(TK_STRING != pDateTypeExpr->iOpcode)
	{
		CHECK_RET(-1,"to_date dateformat is error.");
	}
	//检查日期格式是否正确
	char * sDateType = pDateTypeExpr->pExprValue->sValue;
	CHECK_OBJ(sDateType);
	m_iDateType = GetValueMapIndex(gDateTypeMap,ArraySize(gDateTypeMap),sDateType);
	if(m_iDateType < 0)
	{
		CHECK_RET(-1,"sDateType[%s] error.",sDateType);
	}
	int iDateLen = 0;
	switch(m_iDateType)
	{
		case E_LONG_YEAR:
			iDateLen = 11;
			break;
		case E_LONG_YEAR_TIME:
			iDateLen = 22;
			break;
		case E_SHORT_YEAR:
			iDateLen = 9;
			break;
		case E_SHORT_YEAR_TIME:
			iDateLen = 15;
			break;
		default:
			CHECK_RET(-1,"m_iDateType[%d] error.",m_iDateType);
			break;
	}
       if(false == pExprList->pExprItems[0].pExpr->pExprValue->IsNull())
        {//不为NULL时再检测
           CHECK_RET(m_tTypeCheck.AdaptAllExprType(pExprList->pExprItems[0].pExpr,
                           MEM_Str,iDateLen),
                       "AdaptAllExprType error.");
       }
	m_vArgv.push_back(pExprList->pExprItems[0].pExpr->pExprValue);
	m_vArgv.push_back(pExprList->pExprItems[1].pExpr->pExprValue);
	return iRet;
}


//to_char函数
/******************************************************************************
* 函数名称	:  TMdbToCharFunc
* 函数描述	:  字符串处理函数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbToCharFunc::TMdbToCharFunc()
{
    m_iArgc = -1;
}
TMdbToCharFunc::~TMdbToCharFunc()
{

}

/******************************************************************************
* 函数名称	:  ExecuteFunc
* 函数描述	:  时间函数执行
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbToCharFunc::ExecuteFunc()
{
	int iRet = 0;
    if(m_vArgv[0]->IsNull())
    {//设置NULL
        m_stFuncContext.ReturnValue->SetNull();
        return iRet;
    }
    m_stFuncContext.ReturnValue->ClearValue();
    if(m_vArgv.size() == 1)
    {
        return ExecuteInToChar();
    }
    else
    {
        return ExecuteDateToChar();
    }
        
}

int TMdbToCharFunc::ExecuteInToChar()
{
    int iRet = 0;

    snprintf(m_stFuncContext.ReturnValue->sValue, m_stFuncContext.ReturnValue->iSize, "%lld", m_vArgv[0]->lValue);
    return iRet;
}

int TMdbToCharFunc::ExecuteDateToChar()
{
    int iRet = 0;
	char * sSrcDate = m_vArgv[0]->sValue;//原始字符串YYYYMMDDhh24miss
	char * sDesDate = m_stFuncContext.ReturnValue->sValue;
	switch(m_iDateType)
	{
		case E_LONG_YEAR://YYYY-MM-DD
			{
                        memcpy(sDesDate,"0000-00-00",10);
                        memcpy(sDesDate,sSrcDate,4);
                        memcpy(sDesDate+5,sSrcDate+4,2);
                        memcpy(sDesDate+8,sSrcDate+6,2);
                        sDesDate[10] = '\0';
			}
			break;
		case E_LONG_YEAR_TIME://YYYY-MM-DD hh24:mi:ss
			{
                        memcpy(sDesDate,"0000-00-00 00:00:00",19);
                        memcpy(sDesDate,sSrcDate,4);
                        memcpy(sDesDate+5,sSrcDate+4,2);
                        memcpy(sDesDate+8,sSrcDate+6,2);
                        memcpy(sDesDate+11,sSrcDate+8,2);
                        memcpy(sDesDate+14,sSrcDate+10,2);
                        memcpy(sDesDate+17,sSrcDate+12,2);
                        sDesDate[19] = '\0';
			}
			break;
		case E_SHORT_YEAR://YYYYMMDD
			{
                         memcpy(sDesDate,sSrcDate,8);
                         sDesDate[8] = '\0';
			}
			break;
		case E_SHORT_YEAR_TIME://YYYYMMDDhh24miss
                     {
                        memcpy(sDesDate,sSrcDate,14);
                        sDesDate[14] = '\0';
                     }
			break;
		default:
			break;
	}

	return iRet;
}

/******************************************************************************
* 函数名称	:  CheckArgv
* 函数描述	:  检测参数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbToCharFunc::CheckArgv()
{
    int iRet = 0;
    m_vArgv.clear();

    ST_EXPR_LIST * pExprList = m_pstExpr->pFunc->pFuncArgs;
    CHECK_OBJ(pExprList);
    int iArgCnt = pExprList->iItemNum;
    if(iArgCnt< 1 || iArgCnt > 2)
    {
        CHECK_RET(-1,"to_char argument format is error");
    }

    if(1 == iArgCnt)
    {      
         iRet = CheckInToCharArgv(pExprList);
    }
    else
    {
        iRet = CheckDateToCharArgv(pExprList);
    }   

    return iRet;
}

/******************************************************************************
* 函数名称	:  CheckInToCharArgv
* 函数描述	:  检测int to char 时参数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  li.ming
*******************************************************************************/
int TMdbToCharFunc::CheckInToCharArgv(const ST_EXPR_LIST * pFuncArgs)
{
    int iRet = 0;

    SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Str,20);
    if(false == pFuncArgs->pExprItems[0].pExpr->pExprValue->IsNull())
    {//不为NULL时再检测
        CHECK_RET(m_tTypeCheck.AdaptAllExprType(pFuncArgs->pExprItems[0].pExpr,
        MEM_Int,sizeof(long long)), "AdaptAllExprType error.");
    }
    m_vArgv.push_back(pFuncArgs->pExprItems[0].pExpr->pExprValue);
    
    return iRet;
}

int TMdbToCharFunc::CheckDateToCharArgv(const ST_EXPR_LIST * pFuncArgs)
{
    int iRet = 0;

    SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Str,20);//最长为YYYY-MM-DD hh24:mi:ss
    ST_EXPR * pDateTypeExpr  = pFuncArgs->pExprItems[1].pExpr;
	if(TK_STRING != pDateTypeExpr->iOpcode)
	{
		CHECK_RET(-1,"to_char dateformat is error.");
	}
	//检查日期格式是否正确
	char * sDateType = pDateTypeExpr->pExprValue->sValue;
	CHECK_OBJ(sDateType);
	m_iDateType = GetValueMapIndex(gDateTypeMap,ArraySize(gDateTypeMap),sDateType);
	if(m_iDateType < 0)
	{
		CHECK_RET(-1,"sDateType[%s] error.",sDateType);
	}
    if(false == pFuncArgs->pExprItems[0].pExpr->pExprValue->IsNull())
    {//不为NULL时再检测
        CHECK_RET(m_tTypeCheck.AdaptAllExprType(pFuncArgs->pExprItems[0].pExpr,
            MEM_Date|MEM_Str,MAX_TIME_LEN),
            "AdaptAllExprType error.");
    }
    m_vArgv.push_back(pFuncArgs->pExprItems[0].pExpr->pExprValue);
    m_vArgv.push_back(pFuncArgs->pExprItems[1].pExpr->pExprValue);
	return iRet;
}


/******************************************************************************
* 函数名称  :  TMdbNVLFunc
* 函数描述  :  NVL函数
* 输入      :  
* 输入      :  
* 输出      :  
* 返回值    :  0 - 成功!0 -失败
* 作者      :  jin.shaohua
*******************************************************************************/
TMdbNVLFunc::TMdbNVLFunc()
{
    m_iArgc = 2;//两个参数
}
TMdbNVLFunc::~TMdbNVLFunc()
{

}
/******************************************************************************
* 函数名称	:  ExecuteFunc
* 函数描述	:  
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbNVLFunc::ExecuteFunc()
{
    ST_MEM_VALUE * pLeftValue = m_vArgv[0];
    ST_MEM_VALUE * pRightValue = m_vArgv[1];
    m_stFuncContext.ReturnValue->ClearValue();
    if(pLeftValue->IsNull())
    {
        pRightValue->CopyToMemValue(m_stFuncContext.ReturnValue);
    }
    else
    {
        pLeftValue->CopyToMemValue(m_stFuncContext.ReturnValue);
    }
    return 0;
}

/******************************************************************************
* 函数名称	:  CheckArgv
* 函数描述	:  检查参数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbNVLFunc::CheckArgv()
{
    int iRet = 0;
    m_vArgv.clear();
    ST_EXPR_LIST * pExprList = m_pstExpr->pFunc->pFuncArgs;
    ST_EXPR * pExpr_1 = pExprList->pExprItems[0].pExpr;
    ST_EXPR * pExpr_2 = pExprList->pExprItems[1].pExpr;
    CHECK_RET(m_tTypeCheck.AdaptAllExprType(pExpr_1,
                            MEM_Int|MEM_Str|MEM_Date|MEM_Blob,MAX_BLOB_LEN),"AdaptAllExprType[0] failed.");
    CHECK_RET(m_tTypeCheck.AdaptAllExprType(pExpr_2,
                            MEM_Int|MEM_Str|MEM_Date|MEM_Blob,MAX_BLOB_LEN),"AdaptAllExprType[1] failed.");
    SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,pExpr_1->pExprValue->iFlags|pExpr_2->pExprValue->iFlags,
            MAX_VALUE(pExpr_1->pExprValue->iSize,pExpr_2->pExprValue->iSize));//设定结果取值

    m_vArgv.push_back(pExpr_1->pExprValue);
    m_vArgv.push_back(pExpr_2->pExprValue);
    return iRet;
}


/*
语法
BlobToChar(Exp) 参数
将exp中的blob转为char 

*/
TMdbBlobToCharFunc::TMdbBlobToCharFunc()
{
    m_iArgc = 1;
}
TMdbBlobToCharFunc::~TMdbBlobToCharFunc()
{

}
int TMdbBlobToCharFunc::ExecuteFunc()
{
    ST_MEM_VALUE * pLeftValue = m_vArgv[0];
    m_stFuncContext.ReturnValue->ClearValue();
    if(pLeftValue->IsNull())
    {
        m_stFuncContext.ReturnValue->SetNull();
    }
    else
    {
        //Base64解码
        std::string encoded = pLeftValue->sValue;
        std::string decoded = Base::base64_decode(encoded);
        SAFESTRCPY(m_stFuncContext.ReturnValue->sValue,m_stFuncContext.ReturnValue->iSize,decoded.c_str());
    }
    return 0;
}
//校验函数参数
int TMdbBlobToCharFunc::CheckArgv()
{
    int iRet = 0;
    m_vArgv.clear();
    ST_EXPR_LIST * pExprList = m_pstExpr->pFunc->pFuncArgs;
    ST_EXPR * pExpr_1 = pExprList->pExprItems[0].pExpr;
    CHECK_RET(m_tTypeCheck.AdaptAllExprType(pExpr_1,
                    MEM_Str|MEM_Date|MEM_Blob,MAX_BLOB_LEN),"AdaptAllExprType[0] failed.");
    SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,pExpr_1->pExprValue->iFlags,pExpr_1->pExprValue->iSize);//设定结果取值
    m_vArgv.push_back(pExpr_1->pExprValue);
    return iRet;
}





/******************************************************************************
* 函数名称  :  TMdbAddSecondsFunc
* 函数描述  :  add_seconds(date,Sec) 参数
                            将时间(date)加上秒数(sec),sec可以为负的
* 输入      :  
* 输入      :  
* 输出      :  
* 返回值    :  0 - 成功!0 -失败
* 作者      :  jin.shaohua
*******************************************************************************/
TMdbAddSecondsFunc::TMdbAddSecondsFunc()
{
    m_iArgc = 2;//两个参数
}
TMdbAddSecondsFunc::~TMdbAddSecondsFunc()
{

}
/******************************************************************************
* 函数名称	:  ExecuteFunc
* 函数描述	:  
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbAddSecondsFunc::ExecuteFunc()
{
    int iRet = 0;
    ST_MEM_VALUE * pLeftValue = m_vArgv[0];
    ST_MEM_VALUE * pRightValue = m_vArgv[1];
    m_stFuncContext.ReturnValue->ClearValue();
    pLeftValue->CopyToMemValue(m_stFuncContext.ReturnValue);
    if(pLeftValue->IsNull()){return 0;}//为null的不计算
    if(false == TMdbNtcStrFunc::IsDateTime(m_stFuncContext.ReturnValue->sValue))
    {
        CHECK_RET(ERR_APP_DATE_VALUE_INVALID,"[%s] is not a date.",m_stFuncContext.ReturnValue->sValue);
    }
    TMdbDateTime::AddSeconds(m_stFuncContext.ReturnValue->sValue,pRightValue->lValue);
    return iRet;
}

/******************************************************************************
* 函数名称	:  CheckArgv
* 函数描述	:  检查参数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbAddSecondsFunc::CheckArgv()
{
    int iRet = 0;
    m_vArgv.clear();
    ST_EXPR_LIST * pExprList = m_pstExpr->pFunc->pFuncArgs;
    ST_EXPR * pExpr_1 = pExprList->pExprItems[0].pExpr;
    ST_EXPR * pExpr_2 = pExprList->pExprItems[1].pExpr;
    CHECK_RET(m_tTypeCheck.AdaptAllExprType(pExpr_1,
                            MEM_Date,MAX_TIME_LEN),"AdaptAllExprType[0] failed.");
    CHECK_RET(m_tTypeCheck.AdaptAllExprType(pExpr_2,
                            MEM_Int,sizeof(long long)),"AdaptAllExprType[1] failed.");
    SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Date,MAX_TIME_LEN);//设定结果取值
    
    m_vArgv.push_back(pExpr_1->pExprValue);
    m_vArgv.push_back(pExpr_2->pExprValue);
    return iRet;
}


/******************************************************************************
* 函数名称  :  TMdbNextvalFunc
* 函数描述  :  构造函数
                           
* 输入      :  
* 输入      :  
* 输出      :  
* 返回值    :  0 - 成功!0 -失败
* 作者      :  jin.shaohua
*******************************************************************************/
TMdbNextvalFunc::TMdbNextvalFunc():
m_pSeq(NULL)
{
    m_iArgc = 1;
    m_pSeq = new TMdbSequence;

}
TMdbNextvalFunc::~TMdbNextvalFunc()
{

}
/******************************************************************************
* 函数名称  :  ExecuteFunc
* 函数描述  :  
                           
* 输入      :  
* 输入      :  
* 输出      :  
* 返回值    :  0 - 成功!0 -失败
* 作者      :  jin.shaohua
*******************************************************************************/
int TMdbNextvalFunc::ExecuteFunc()
{
    m_stFuncContext.ReturnValue->lValue  =  m_pSeq->GetNextIntVal();
    return 0;
}   
/******************************************************************************
* 函数名称  :  CheckArgv
* 函数描述  :  校验函数参数
                           
* 输入      :  
* 输入      :  
* 输出      :  
* 返回值    :  0 - 成功!0 -失败
* 作者      :  jin.shaohua
*******************************************************************************/

int TMdbNextvalFunc::CheckArgv()
{
    int iRet = 0;
    
    m_vArgv.clear();
    ST_EXPR_LIST * pExprList = m_pstExpr->pFunc->pFuncArgs;
    ST_EXPR * pExpr_1 = pExprList->pExprItems[0].pExpr;
    CHECK_OBJ(pExpr_1);
    ExprCalcSetProperty(m_pstExpr,CALC_PER_Row);
    CHECK_RET(m_pSeq->SetConfig(m_sDsn,pExpr_1->pExprValue->sValue),"not find sequence[%s] in dsn[%s]",pExpr_1->pExprValue->sValue,m_sDsn);
    SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Int,sizeof(long long));//结果值确定为int
    m_vArgv.push_back(pExpr_1->pExprValue);

    return iRet;
	
}





/******************************************************************************
* 函数名称  :  TMdbCurrvalFunc
* 函数描述  :  构造函数
                           
* 输入      :  
* 输入      :  
* 输出      :  
* 返回值    :  0 - 成功!0 -失败
* 作者      :  jin.shaohua
*******************************************************************************/
TMdbCurrvalFunc::TMdbCurrvalFunc():
m_pSeq(NULL)
{
    m_iArgc = 1;
    m_pSeq = new TMdbSequence;

}
TMdbCurrvalFunc::~TMdbCurrvalFunc()
{

}
/******************************************************************************
* 函数名称  :  ExecuteFunc
* 函数描述  :  
                           
* 输入      :  
* 输入      :  
* 输出      :  
* 返回值    :  0 - 成功!0 -失败
* 作者      :  jin.shaohua
*******************************************************************************/
int TMdbCurrvalFunc::ExecuteFunc()
{
    m_stFuncContext.ReturnValue->lValue  =  m_pSeq->GetCurrVal();
    return 0;
}   
/******************************************************************************
* 函数名称  :  CheckArgv
* 函数描述  :  校验函数参数
                           
* 输入      :  
* 输入      :  
* 输出      :  
* 返回值    :  0 - 成功!0 -失败
* 作者      :  jin.shaohua
*******************************************************************************/

int TMdbCurrvalFunc::CheckArgv()
{
    int iRet = 0;

    m_vArgv.clear();
    ST_EXPR_LIST * pExprList = m_pstExpr->pFunc->pFuncArgs;
    ST_EXPR * pExpr_1 = pExprList->pExprItems[0].pExpr;
    CHECK_OBJ(pExpr_1);
    ExprCalcSetProperty(m_pstExpr,CALC_PER_Row);
    CHECK_RET(m_pSeq->SetConfig(m_sDsn,pExpr_1->pExprValue->sValue),"not find sequence[%s] in dsn[%s]",pExpr_1->pExprValue->sValue,m_sDsn);
    SET_MEM_TYPE_SIZE(m_pstExpr->pExprValue,MEM_Int,sizeof(long long));//结果值确定为int
    m_vArgv.push_back(pExpr_1->pExprValue);

    return iRet;
}
//}




