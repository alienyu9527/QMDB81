/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    SqlTypeCheck.cpp
*@Description： SQL数据类型校验类
*@Author:	    jin.shaohua
*@Date：	    2012.05
*@History:
******************************************************************************************/
//#include "BillingSDK.h"
#include "Helper/SqlTypeCheck.h"
#include "Helper/parser.h"
#include "Helper/mdbFunc.h"
#include "Helper/SqlParserStruct.h"
#include "Helper/SyntaxTreeAnalyse.h"

//#define CHECK_RET(_ret,...)if((iRet = _ret)!=0){TADD_ERROR_CODE(iRet,__VA_ARGS__);return iRet;}
//#define CHECK_OBJ(_obj) if(NULL == _obj){TADD_ERROR_CODE(ERR_APP_INVALID_PARAM, #_obj" is null"); return ERR_APP_INVALID_PARAM;}
//namespace QuickMDB{


	enum E_HAS_TYPE
	{
		HAS_ALL  = 1,
		HAS_ANY = 2
	};
	/******************************************************************************
	* 函数名称	: TSqlTypeCheck
	* 函数描述	:   SQL数据类型校验类 构造
	* 输入		:
	* 输入		:
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	TSqlTypeCheck::TSqlTypeCheck():
		m_pstSqlStruct(NULL)
	{
		
	}
	/******************************************************************************
	* 函数名称	:  ~TSqlTypeCheck
	* 函数描述	:  SQL数据类型校验类 析构
	* 输入		:
	* 输入		:
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	TSqlTypeCheck::~TSqlTypeCheck()
	{

	}

	/******************************************************************************
	* 函数名称	:  FillValidType
	* 函数描述	:  向节点中填充所支持的合法类型
	* 输入		:  pstSqlStruct - sql解析后的结构体
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::FillValidType(ST_SQL_STRUCT * pstSqlStruct)
	{
		TADD_FUNC("Start.pstSqlStruct=[%p]",pstSqlStruct);
		int iRet = 0;
		CHECK_OBJ(pstSqlStruct);
		m_pstSqlStruct = pstSqlStruct;
		switch(pstSqlStruct->iSqlType)
		{
		case TK_INSERT:
			CHECK_RET(FillInsertValidType(pstSqlStruct),"FillInsertValidType error");
			break;
		case TK_DELETE:
			CHECK_RET(FillDeleteValidType(pstSqlStruct),"FillDeleteValidType error");
			break;
		case TK_UPDATE:
			CHECK_RET(FillUpdateValidType(pstSqlStruct),"FillUpdateValidType error");
			break;
		case TK_SELECT:
			CHECK_RET(FillSelectValidType(pstSqlStruct),"FillSelectValidType error");
			break;
		default:
			CHECK_RET(-1,"type[%d] invalid",pstSqlStruct->iSqlType);
			break;
		}
		TADD_FUNC("Finish");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  FillInsertValidType
	* 函数描述	:  向节点中填充所支持的合法类型
	* 输入		:  pstSqlStruct - sql解析后的结构体
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::FillInsertValidType(ST_SQL_STRUCT * pstSqlStruct)
	{
		TADD_FUNC("Start.pstSqlStruct=[%p]",pstSqlStruct);
		int iRet = 0;
		ST_ID_LIST * pIdList     = pstSqlStruct->pstIdList;
		ST_EXPR_LIST * pColList  = pstSqlStruct->pColList;
		if(pIdList->iIdNum != pColList->iItemNum)
		{
			CHECK_RET(ERR_SQL_INVALID,"insert column num not match...");
		}
		//先设置顶层expr_list的合法数据类型
		int i = 0;
		int iValueType = 0;
		int iValueSize = 0;
		for(i = 0; i < pIdList->iIdNum; i++)
		{
			CHECK_RET(GetTypeAndSize(pIdList->pstItem[i].pColumn,iValueType,iValueSize),
					  "GetTypeAndSize column[%s] error.",pIdList->pstItem[i].pColumn->sName);
			//将顶层expr_list合法数据类型传递到下层
			CHECK_RET(AdaptAllExprType(pColList->pExprItems[i].pExpr,iValueType,iValueSize),
					  "AdaptAllExprType error.");
		}
		TADD_FUNC("Finish");
		return iRet;
	}


	/******************************************************************************
	* 函数名称	:  FillDeleteValidType
	* 函数描述	:  向节点中填充所支持的合法类型
	* 输入		:  pstSqlStruct - sql解析后的结构体
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/

	int TSqlTypeCheck::FillDeleteValidType(ST_SQL_STRUCT * pstSqlStruct)
	{
		TADD_FUNC("Start.pstSqlStruct=[%p]",pstSqlStruct);
		int iRet = 0;
		CHECK_RET(AdaptAllExprType(pstSqlStruct->pWhere,(MEM_Int),MAX_BLOB_LEN),
				  "AdaptAllExprType error...");
		TADD_FUNC("Finish");
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  FillUpdateValidType
	* 函数描述	:  向节点中填充所支持的合法类型
	* 输入		:  pstSqlStruct - sql解析后的结构体
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/

	int TSqlTypeCheck::FillUpdateValidType(ST_SQL_STRUCT * pstSqlStruct)
	{
		TADD_FUNC("Start.pstSqlStruct=[%p]",pstSqlStruct);
		int iRet = 0;
		ST_ID_LIST * pIdList     = pstSqlStruct->pstIdList;
		ST_EXPR_LIST * pColList  = pstSqlStruct->pColList;
		if(pIdList->iIdNum != pColList->iItemNum)
		{
			CHECK_RET(-1,"insert column num not match...");
		}
		//先设置顶层expr_list的合法数据类型
		int i = 0;
		int iValueType = 0;
		int iValueSize = 0;
		for(i = 0; i < pIdList->iIdNum; i++)
		{
			CHECK_RET(GetTypeAndSize(pIdList->pstItem[i].pColumn,iValueType,iValueSize),
					  "GetTypeAndSize column[%s] error.",pIdList->pstItem[i].pColumn->sName);
			//将顶层expr_list合法数据类型传递到下层
			CHECK_RET(AdaptAllExprType(pColList->pExprItems[i].pExpr,iValueType,iValueSize),
					  "AdaptAllExprType error.");
		}
		CHECK_RET(AdaptAllExprType(pstSqlStruct->pWhere,(MEM_Int),sizeof(long long)),
				  "CheckAndSetExprType error...");
		TADD_FUNC("Finish");
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  FillSelectValidType
	* 函数描述	:  向节点中填充所支持的合法类型
	* 输入		:  pstSqlStruct - sql解析后的结构体
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/

	int TSqlTypeCheck::FillSelectValidType(ST_SQL_STRUCT * pstSqlStruct)
	{
		TADD_FUNC("Start.pstSqlStruct=[%p]",pstSqlStruct);
		int iRet = 0;
		CHECK_RET(FillExprListValidType(pstSqlStruct->pColList,(MEM_Str|MEM_Int|MEM_Date),MAX_BLOB_LEN),
				  "FillExprListValidType error");
		CHECK_RET(AdaptAllExprType(pstSqlStruct->pWhere,(MEM_Int),sizeof(long long)),
				  "AdaptAllExprType error...");
		CHECK_RET(FillExprListValidType(pstSqlStruct->pGroupby,(MEM_Str|MEM_Int|MEM_Date),MAX_BLOB_LEN),
				  "AdaptAllExprType error...");
		CHECK_RET(AdaptAllExprType(pstSqlStruct->pHaving,(MEM_Str|MEM_Int|MEM_Date),MAX_BLOB_LEN),
				  "AdaptAllExprType error...");
		CHECK_RET(FillExprListValidType(pstSqlStruct->pOrderby,(MEM_Str|MEM_Int|MEM_Date),MAX_BLOB_LEN),
				  "FillExprListValidType error");
		CHECK_RET(AdaptAllExprType(pstSqlStruct->pstLimit,(MEM_Int),sizeof(long long)),
				  "AdaptAllExprType error...");
		CHECK_RET(AdaptAllExprType(pstSqlStruct->pstOffset,(MEM_Int),sizeof(long long)),
				  "AdaptAllExprType error...");
		TADD_FUNC("Finish");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  ConverType
	* 函数描述	:  将TMdbColumn所对应的数据类型，转化成ST_MEM_VALUE所支持的数据类型
	* 输入		:
	* 输入		:
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::ConverType(TMdbColumn * pColumn,ST_MEM_VALUE * pstMemValue)
	{

		int iRet = 0;
		CHECK_OBJ(pColumn);
		CHECK_OBJ(pstMemValue);
		TADD_FUNC("Start.pColumn[%s]",pColumn->sName);
		int iValueType = 0;
		int iValueSize = 0;
		CHECK_RET(GetTypeAndSize(pColumn,iValueType,iValueSize),"GetTypeAndSize failed...");
		SET_MEM_TYPE_SIZE(pstMemValue,iValueType,iValueSize);
		TADD_FUNC("Finish");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  FillExprListValidType
	* 函数描述	:  向表达式list节点中填充合法类型
	* 输入		:
	* 输入		:
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::FillExprListValidType(ST_EXPR_LIST * pstExprList,int iType,int iSize)
	{
		TADD_FUNC("Start.pstExprList=[%p],iType=[%d],iSize=[%d]",pstExprList,iType,iSize);
		int iRet = 0;
		if(NULL == pstExprList)
		{
			return iRet;
		}
		int i = 0;
		for(i = 0; i<pstExprList->iItemNum; i++)
		{
			CHECK_RET(AdaptAllExprType(pstExprList->pExprItems[i].pExpr,iType,iSize),
					  "CheckAndSetExprType[%d] error...",i);
		}
		TADD_FUNC("Finish");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  FillFunctionValidType
	* 函数描述	:  填充函数的合法数据类型
	* 输入		:  pstExpr - 函数表达式结构
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::FillFunctionValidType(ST_EXPR * pstExpr)
	{
		int iRet = 0;
		TADD_FUNC("Start.pstExpr=[%p]",pstExpr);
		CHECK_RET(TMdbFuncFactory::GetFunction(pstExpr,m_pstSqlStruct),"GetFunction error");
		if(TMdbNtcStrFunc::StrNoCaseCmp(pstExpr->sTorken, "count") == 0 || 
			TMdbNtcStrFunc::StrNoCaseCmp(pstExpr->sTorken, "max") == 0 ||
			TMdbNtcStrFunc::StrNoCaseCmp(pstExpr->sTorken, "min") == 0 ||
			TMdbNtcStrFunc::StrNoCaseCmp(pstExpr->sTorken, "sum") == 0 )
		{
			if(m_pstSqlStruct->bIsSimpleCol)
			{
				CHECK_RET(ERR_SQL_INVALID, "Not a single-group group function [%s].", pstExpr->sTorken);
			}
			m_pstSqlStruct->bIsSingleGroupFunc = true;
		}
		if(FuncHasProperty(pstExpr->pFunc->pFuncDef,FUNC_AGGREAGATE))
		{
			SelectSqlSetProperty(m_pstSqlStruct,SF_Aggregate);
		}
		TADD_FUNC("Finish");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  GetTypeAndSize
	* 函数描述	:  获取值类型和长度
	* 输入		:
	* 输入		:
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::GetTypeAndSize(TMdbColumn * pColumn,int & iValueType,int &iValueSize)
	{
		int iRet = 0;
		CHECK_OBJ(pColumn);
		TADD_FUNC("Start.pColumn[%s]",pColumn->sName);
		switch(pColumn->iDataType)
		{
		case DT_Int:
			iValueType = MEM_Int;
			iValueSize = pColumn->iColumnLen;
			break;
		case DT_Char:
			iValueType = MEM_Str;
			iValueSize = pColumn->iColumnLen;
			break;
		case DT_DateStamp://对于系统优化方面做处理? 压缩时间类型?
			iValueType = MEM_Date;
			iValueSize = MAX_TIME_LEN;//时间类型给15的长度以字符串保存//pColumn->m_iDataLen;
			break;
		case DT_VarChar:
		case DT_Blob://varchar ,blob放在可变长区
			iValueType = MEM_Str;
			iValueSize = pColumn->iColumnLen;
			break;
		default:
			CHECK_RET(-1,"column[%s] type[%d] error...",pColumn->sName,pColumn->iDataType);
			break;
		}
		TADD_FUNC("Finish.pColumn[%s],iValueType=[%d],iValueSize=[%d]",pColumn->sName,iValueType,iValueSize);
		return iRet;
	}


	/******************************************************************************
	* 函数名称	:  IsTypeSizeValid
	* 函数描述	:  type 和size 是否正确
	* 输入		:  iValidType -合法的类型 iValidSize- 合法的大小
	* 输入		:  pstMemValue - 需要校验的内存值
	* 输出		:  无
	* 返回值	:  true - 成功false -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int  TSqlTypeCheck::CheckTypeSizeValid(int iValidType,int iValidSize,ST_MEM_VALUE * pstMemValue)
	{
		TADD_FUNC("Start,iValidType=[%d],iValidSize=[%d],pstMemValue=[%p]",iValidType,iValidSize,pstMemValue);
		int iRet  = 0;
		bool bRet = false;
		if(pstMemValue->IsNull())
		{
			bRet = true;
		}
		else if((iValidType & (MEM_Str|MEM_Date)) && MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
		{
			//都为string
			bRet =  iValidSize < pstMemValue->iSize ?false:true;
		}
		else if((iValidType & MEM_Int) && (pstMemValue->iFlags  & MEM_Int))
		{
			//都为int
			bRet = true;
		}
		if(false == bRet)
		{
			CHECK_RET(ERR_SQL_TYPE_INVALID,"valid type[%s]size[%d],but expr type[%s]size[%d]",
					 TSyntaxTreeAnalyse::DescribeMemValueFlag(iValidType).c_str(), iValidSize,
					 TSyntaxTreeAnalyse::DescribeMemValueFlag(pstMemValue).c_str(),pstMemValue->iSize);
		}
		TADD_FUNC("Finish");
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  AdaptExprType
	* 函数描述	:  适配单个二叉树的expr类型自底向上进行适配
	* 输入		:
	* 输入		:
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::AdaptExprType(ST_EXPR *pstExpr)
	{
		TADD_FUNC("Start.pstExpr=[%p]",pstExpr);
		int iRet = 0;
		if(NULL == pstExpr)
		{
			return 0;
		}
		//先适配左右子树的类型
		CHECK_RET(AdaptExprType(pstExpr->pLeft),"AdaptExprType[left] failed.");
		CHECK_RET(AdaptExprType(pstExpr->pRight),"AdaptExprType[right] failed.");
		//根据左右子树确定当前类型
		TADD_DETAIL("pstExpr->iOpcode[%d].",pstExpr->iOpcode);
		switch(pstExpr->iOpcode)
		{
		case TK_ID://列信息直接设置类型
			CHECK_RET(ConverType(pstExpr->pExprValue->pColumn,pstExpr->pExprValue),
					  "ConverType[%s] failed...",TSyntaxTreeAnalyse::ReplaceNull(pstExpr->sTorken));
        
			break;
		case TK_ID_SEQUENCE:
			SET_MEM_TYPE_SIZE(pstExpr->pExprValue,MEM_Int,sizeof(long long));
			break;
		case TK_INTEGER:// 数值常量无需再设置
            break;
        case TK_NULL://null 关键字伪装成int 或str用来跳过类型校验
            SET_MEM_TYPE_SIZE(pstExpr->pExprValue,MEM_Int|MEM_Str,sizeof(long long));
			break;
			//单元操作符
		case TK_UMINUS:
		case TK_UPLUS:
		case TK_NOT:
		case TK_BITNOT:
		{
			if(MemValueHasProperty(pstExpr->pLeft->pExprValue,MEM_Int))
			{
            
				//左值必须为int
				SET_MEM_TYPE_SIZE(pstExpr->pExprValue,MEM_Int,sizeof(long long));
            
			}
			else
			{
				CHECK_RET(-1,"opcode[%d] left type[%s] is error.",pstExpr->iOpcode,
						 TSyntaxTreeAnalyse::DescribeMemValueFlag(pstExpr->pLeft->pExprValue).c_str());
			}
		}

		break;
		case TK_STAR://*
		case TK_SLASH:// /
		case TK_REM:// %
		case TK_AND:// and
		case TK_OR:// or
		{
			//两边必须都是是数值
			ST_ADAPT_RULE arrAdaptRule[] =
			{
				{HAS_ALL,MEM_Int,MEM_Int,MEM_Int,sizeof(long long)},
				{-1,0,0,0,0},
			};
			CHECK_RET(AdaptExprTypeRule(pstExpr,arrAdaptRule),"AdaptExprTypeRule error.");
		}
		break;
		case TK_LT://<
		case TK_LE://<=
		case TK_GE://>=
		case TK_GT://>
		case TK_NE://<>
		case TK_EQ:// =
		{
			//两边可以同时为数值或者同时为string |date，结果为 int
			ST_ADAPT_RULE arrAdaptRule[] =
			{
				{HAS_ALL,MEM_Int,MEM_Int,MEM_Int,sizeof(long long)},
				{HAS_ANY,MEM_Str|MEM_Date,MEM_Str|MEM_Date,MEM_Int,sizeof(long long)},
				{-1,0,0,0,0},
			};
			CHECK_RET(AdaptExprTypeRule(pstExpr,arrAdaptRule),"AdaptExprTypeRule error.");
		}
		break;
		case TK_PLUS://+
		{
			//两边可以都为int 或者有左边为date 右边为int
			ST_ADAPT_RULE arrAdaptRule[] =
			{
				{HAS_ALL,MEM_Int,MEM_Int,MEM_Int,sizeof(long long)},
				{HAS_ALL,MEM_Date,MEM_Int,MEM_Date,MAX_TIME_LEN},
				{HAS_ALL,MEM_Int,MEM_Date,MEM_Date,MAX_TIME_LEN},
				{-1,0,0,0,0},
			};
			CHECK_RET(AdaptExprTypeRule(pstExpr,arrAdaptRule),"AdaptExprTypeRule error.");
		}
		break;
		case TK_MINUS://-
		{
			//两边可以都为int 或者有左边为date 右边为int或者两边都为date
			ST_ADAPT_RULE arrAdaptRule[] =
			{
				{HAS_ALL,MEM_Int,MEM_Int,MEM_Int,sizeof(long long)},
				{HAS_ALL,MEM_Date,MEM_Int,MEM_Date,MAX_TIME_LEN},
				{HAS_ALL,MEM_Date,MEM_Date,MEM_Int,sizeof(long long)},
				{-1,0,0,0,0},
			};
			CHECK_RET(AdaptExprTypeRule(pstExpr,arrAdaptRule),"AdaptExprTypeRule error.");
		}
		break;
		case TK_CONCAT:// || 字符串连接
		{
			//两边必须都为str
			ST_ADAPT_RULE arrAdaptRule[] =
			{
				{HAS_ALL,MEM_Str,MEM_Str,MEM_Str,MAX_BLOB_LEN},
				{-1,0,0,0,0},
			};
			CHECK_RET(AdaptExprTypeRule(pstExpr,arrAdaptRule),"AdaptExprTypeRule error.");
		}
		break;
		case TK_STRING:// 字符串常量
			break;
		case TK_IN:// int 关键字
		case TK_CONST_FUNC://静态函数
		case TK_FUNCTION://TODO: 关于函数的数据类型检测
		case TK_NEXTVAL:
		case TK_CURRVAL:
			CHECK_RET(FillFunctionValidType(pstExpr),"FillFunctionValidType error");
			break;
		case TK_VARIABLE://可变参数先设置为所有类型
			pstExpr->pExprValue->iFlags = MEM_Str|MEM_Int|MEM_Date;
			pstExpr->pExprValue->iSize  = MAX_BLOB_LEN;
			break;
		case TK_ISNULL:
		case TK_NOTNULL:
			//无需校验
			SET_MEM_TYPE_SIZE(pstExpr->pExprValue,MEM_Int,sizeof(long long));
			break;
		default:
		{
			if(NULL != pstExpr->pLeft && NULL != pstExpr->pRight)
			{
				CHECK_RET(ERR_SQL_TYPE_INVALID,"opcode[%d] left[%s] type[%s] or right[%s] type[%s] is error.",
						  pstExpr->iOpcode,TSyntaxTreeAnalyse::ReplaceNull(pstExpr->pLeft->sTorken),
						  TSyntaxTreeAnalyse::DescribeMemValueFlag(pstExpr->pLeft->pExprValue).c_str(),
						  TSyntaxTreeAnalyse::ReplaceNull(pstExpr->pRight->sTorken),
						  TSyntaxTreeAnalyse::DescribeMemValueFlag(pstExpr->pRight->pExprValue).c_str());
			}
			else
			{
				CHECK_RET(ERR_SQL_TYPE_INVALID,"opcode[%d] not support",pstExpr->iOpcode);
			}
		}
		break;
		}
		TADD_FUNC("Finish.Expr:iFlag[%d],size[%d]",pstExpr->pExprValue->iFlags,pstExpr->pExprValue->iSize);
		return iRet;
	}

	/******************************************************************************
	* 函数名称	:  AdaptVariableExprType
	* 函数描述	:  适配绑定变量的expr
	* 输入		:
	* 输入		:
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::AdaptVariableExprType(ST_EXPR *pstExpr,int iType,int iSize)
	{
		TADD_FUNC("Start.pstExpr=[%p],iType=[%d],iSize=[%d]",pstExpr,iType,iSize);
		int iRet = 0;
		if(NULL == pstExpr)
		{
			return 0;
		}
		switch(pstExpr->iOpcode)
		{
		case TK_ID:
		case TK_ID_SEQUENCE:
			break;
		case TK_INTEGER:
		case TK_NULL:
			break;
			//单元操作符
		case TK_UMINUS:
		case TK_UPLUS:
		case TK_NOT:
		case TK_BITNOT:
			CHECK_RET(AdaptVariableExprType(pstExpr->pLeft,MEM_Int,sizeof(long long)),"AdaptVariableExprType error[%s].",
				TSyntaxTreeAnalyse::ReplaceNull(pstExpr->pLeft->sTorken));
			break;
		case TK_STAR://*
		case TK_SLASH:// /
		case TK_REM:// %
		case TK_AND:// and
		case TK_OR:// or
		case TK_PLUS://+
		case TK_MINUS://-
			//两边必须都是是数值
			CHECK_RET(AdaptVariableExprType(pstExpr->pRight,MEM_Int,sizeof(long long)),"AdaptVariableExprType error[%s].",
			TSyntaxTreeAnalyse::ReplaceNull(pstExpr->pRight->sTorken));
			CHECK_RET(AdaptVariableExprType(pstExpr->pLeft,MEM_Int,sizeof(long long)),"AdaptVariableExprType error[%s].",
				TSyntaxTreeAnalyse::ReplaceNull(pstExpr->pLeft->sTorken));
			break;
		case TK_LT://<
		case TK_LE://<=
		case TK_GE://>=
		case TK_GT://>

		case TK_NE://<>
		case TK_EQ:// =
			CHECK_RET(AdaptVariableExprType(pstExpr->pRight,pstExpr->pLeft->pExprValue->iFlags,
											pstExpr->pLeft->pExprValue->iSize),"AdaptVariableExprType error.");
			CHECK_RET(AdaptVariableExprType(pstExpr->pLeft,pstExpr->pRight->pExprValue->iFlags,
											pstExpr->pRight->pExprValue->iSize),"AdaptVariableExprType error.");
			break;
		case TK_CONCAT:// || 字符串连接
			SET_MEM_TYPE_SIZE(pstExpr->pExprValue,MEM_Str,iSize);
			CHECK_RET(AdaptVariableExprType(pstExpr->pRight,MEM_Str,iSize),"AdaptVariableExprType error.");
			CHECK_RET(AdaptVariableExprType(pstExpr->pLeft,MEM_Str,iSize),"AdaptVariableExprType error.");
			break;
		case TK_STRING:// 字符串常量
			break;
		case TK_IN:// int 关键字
		case TK_CONST_FUNC://
		case TK_FUNCTION:
			break;
		case TK_VARIABLE://可变参数TODO:待改进
			pstExpr->pExprValue->iFlags = 0;//清理
			SET_MEM_TYPE_SIZE(pstExpr->pExprValue,iType,iSize);
			break;
		case TK_ISNULL:
		case TK_NOTNULL:
			break;
		default:
			CHECK_RET(ERR_SQL_INVALID,"opcode[%d] is error",pstExpr->iOpcode);
			break;
		}
		TADD_FUNC("Finish");
		return iRet;
	}


	/******************************************************************************
	* 函数名称	:  AdaptAllExprType
	* 函数描述	:  适配所有的expr
	* 输入		:
	* 输入		:
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::AdaptAllExprType(ST_EXPR *pstExpr,int iType,int iSize)
	{
		TADD_FUNC("Start.pstExpr=[%p],iType=[%d],iSize=[%d]",pstExpr,iType,iSize);
		int iRet = 0;
		if(NULL == pstExpr)
		{
			return 0;
		}
		CHECK_RET(AdaptExprType(pstExpr),"Check ExprType error,Token=[%s]",TSyntaxTreeAnalyse::ReplaceNull(pstExpr->sTorken));
		CHECK_RET(AdaptVariableExprType(pstExpr,iType,iSize),"Check Variable ExprType error,Token=[%s]",TSyntaxTreeAnalyse::ReplaceNull(pstExpr->sTorken));
		CHECK_RET(CheckTypeSizeValid(iType,iSize,pstExpr->pExprValue),"CheckTypeSizeValid error,Token=[%s].",TSyntaxTreeAnalyse::ReplaceNull(pstExpr->sTorken));
		CHECK_RET(SetExprCalcFalgs(pstExpr),"SetExprCalcFalgs failed.");
		TADD_FUNC("Finish");
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  AdaptExprTypeRule
	* 函数描述	:  按规则进行校验
	* 输入		:
	* 输入		:
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::AdaptExprTypeRule(ST_EXPR *pstExpr,ST_ADAPT_RULE * arrAdaptRule)
	{
		TADD_FUNC("Start");
		int iRet = 0;
		CHECK_OBJ(arrAdaptRule);
		int i = 0;
		for(i = 0; arrAdaptRule[i].iHasType > 0; ++i)
		{
			ST_ADAPT_RULE & stAdaptRule = arrAdaptRule[i];
			if(HAS_ALL == stAdaptRule.iHasType)
			{
				if(MemValueHasProperty(pstExpr->pLeft->pExprValue,stAdaptRule.iLeftType) &&
						MemValueHasProperty(pstExpr->pRight->pExprValue,stAdaptRule.iRightType))
				{
					//条件匹配
					SET_MEM_TYPE_SIZE(pstExpr->pExprValue,stAdaptRule.iResultType,stAdaptRule.iResultSize);
					break;
				}
			}
			else if(HAS_ANY == stAdaptRule.iHasType)
			{
				if(MemValueHasAnyProperty(pstExpr->pLeft->pExprValue,stAdaptRule.iLeftType) &&
						MemValueHasAnyProperty(pstExpr->pRight->pExprValue,stAdaptRule.iRightType))
				{
					//条件匹配
					SET_MEM_TYPE_SIZE(pstExpr->pExprValue,stAdaptRule.iResultType,stAdaptRule.iResultSize);
					break;
				}
			}
			else
			{
				CHECK_RET(ERR_SQL_TYPE_INVALID,"error has type[%d] in adapt rule[%d].",stAdaptRule.iHasType,i);
			}
		}
		if(arrAdaptRule[i].iHasType < 0)
		{
			//规则匹配完毕,没有可用规则
			CHECK_RET(ERR_SQL_TYPE_INVALID,"opcode[%d] left[%s] type[%s] or right[%s] type[%s] is error.",
					  pstExpr->iOpcode,TSyntaxTreeAnalyse::ReplaceNull(pstExpr->pLeft->sTorken),
					  TSyntaxTreeAnalyse::DescribeMemValueFlag(pstExpr->pLeft->pExprValue).c_str(),
					  TSyntaxTreeAnalyse::ReplaceNull(pstExpr->pRight->sTorken),
					  TSyntaxTreeAnalyse::DescribeMemValueFlag(pstExpr->pRight->pExprValue).c_str());
		}
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  SetExprCalcFalgs
	* 函数描述	: 设置表达式计算标识
	* 输入		:
	* 输入		:
	* 输出		:
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TSqlTypeCheck::SetExprCalcFalgs(ST_EXPR *pstExpr)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		if(NULL == pstExpr){return 0;}
		SetExprCalcFalgs(pstExpr->pLeft);
		SetExprCalcFalgs(pstExpr->pRight);
			//根据左右子树确定当前类型
		TADD_DETAIL("pstExpr->iOpcode[%d].",pstExpr->iOpcode);
		switch(pstExpr->iOpcode)
		{
			case TK_ID://列信息直接设置类型
				{
					ExprCalcSetProperty(pstExpr,CALC_PER_Row);
				}
				break;
			 //常量
			case TK_STRING:// 字符串常量
			case TK_INTEGER:// 数值常量无需再设置
			case TK_NULL://null 常量无需设置
				{
					ExprCalcSetProperty(pstExpr,CALC_PER_Exec);
                    pstExpr->pExprValue->SetConst();
				}
				break;
			//单元操作符
			case TK_UMINUS:
			case TK_UPLUS:
			case TK_NOT:
			case TK_BITNOT:
			case TK_ISNULL:
			case TK_NOTNULL:
				{
						ExprCalcSetProperty(pstExpr,pstExpr->pLeft->iCalcFlags);//与左值相同
				}
			break;
			//双元操作符
			case TK_STAR://*
			case TK_SLASH:// /
			case TK_REM:// %
			case TK_AND:// and
			case TK_OR:// or
			case TK_LT://<
			case TK_LE://<=
			case TK_GE://>=
			case TK_GT://>
			case TK_NE://<>
			case TK_EQ:// =
			case TK_PLUS://+
			case TK_MINUS://-
			case TK_CONCAT:// || 字符串连接
				{//取消的
						pstExpr->iCalcFlags = pstExpr->pLeft->iCalcFlags < pstExpr->pRight->iCalcFlags ?
							pstExpr->pLeft->iCalcFlags:pstExpr->pRight->iCalcFlags;
				}
			break;
			case TK_CONST_FUNC://静态函数
				{
					ExprCalcSetProperty(pstExpr,CALC_PER_Exec);
				}
			break;
			case TK_IN:// int 关键字
			case TK_FUNCTION:
				{
						int iTemp = CALC_PER_Exec;
						if(FuncHasProperty(pstExpr->pFunc->pFuncDef,FUNC_AGGREAGATE))
						{//聚合函数
							iTemp = CALC_PER_Row;
						}
						if(ExprCalcHasAnyProperty(pstExpr,CALC_PER_Exec|CALC_PER_Row|CALC_PER_Query))
						{
							iTemp = pstExpr->iCalcFlags;
						}
						//考虑左右数据类型
						if(NULL != pstExpr->pLeft && pstExpr->pLeft->iCalcFlags < iTemp)
						{
							iTemp = pstExpr->pLeft->iCalcFlags;
						}
						if(NULL != pstExpr->pRight && pstExpr->pRight->iCalcFlags < iTemp)
						{
							iTemp = pstExpr->pRight->iCalcFlags;
						}
						//考虑函数列表
						if(NULL != pstExpr->pFunc->pFuncArgs)
						{
							int i = 0;
							for(i = 0;i < pstExpr->pFunc->pFuncArgs->iItemNum;++i)
							{
								ST_EXPR * pTempExpr = pstExpr->pFunc->pFuncArgs->pExprItems[i].pExpr;
								SetExprCalcFalgs(pTempExpr);
								if(pTempExpr->iCalcFlags < iTemp){iTemp = pTempExpr->iCalcFlags;}
							}
						}
						pstExpr->iCalcFlags = iTemp;
				}
				break;
			case TK_VARIABLE://可变参数先设置为所有类型
				{
						ExprCalcSetProperty(pstExpr,CALC_PER_Exec);
				}
				break;
			default: //每次都要计算
				{
						ExprCalcSetProperty(pstExpr,CALC_PER_Row);
				}
			break;  
		}

            if(NULL != m_pstSqlStruct)
            {
                if(ExprCalcHasAnyProperty(pstExpr,CALC_PER_Query))
                {
                    m_pstSqlStruct->vExprCalcPerQuery.push_back(pstExpr);
                }
                if(ExprCalcHasAnyProperty(pstExpr,CALC_PER_Exec))
                {
                    m_pstSqlStruct->vExprCalcPerExec.push_back(pstExpr);
                }
            }
            
		TADD_FUNC("End.");
		return iRet;
	}
//}

