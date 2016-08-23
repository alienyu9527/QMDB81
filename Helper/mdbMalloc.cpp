/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbMalloc.cpp		
*@Description： mdb内存申请与释放的统一接口。单例模式
*@Author:			jin.shaohua
*@Date：	    2012.05
*@History:
******************************************************************************************/
//#include "BillingSDK.h"
#include "Helper/mdbMalloc.h"
#include "Helper/parser.h"
#include <iostream>
#include "Helper/mdbFunc.h"
using namespace std;
//using namespace ZSmart::BillingSDK;
//namespace QuickMDB{


	TMdbMalloc * TMdbMalloc::m_pMdbmalloc = NULL;
	TMdbMalloc::CGarbo TMdbMalloc::Garbo; 


	TMdbMalloc::TMdbMalloc()
	{
	
	}
	TMdbMalloc::~TMdbMalloc()
	{

	}

	/******************************************************************************
	* 函数名称	:  Instance
	* 函数描述	:  获取TMdbMalloc唯一实例
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  TMdbMalloc实例
	* 作者		:  jin.shaohua
	*******************************************************************************/
	TMdbMalloc * TMdbMalloc::Instance()
	{
		if (NULL == m_pMdbmalloc)
		{
			m_pMdbmalloc = new TMdbMalloc();
		}
		return m_pMdbmalloc;
	}

	/******************************************************************************
	* 函数名称	:  ReAlloc
	* 函数描述	:  重新分配大小
	* 输入		:  sTablename 表名
	* 输出		:  无
	* 返回值	:  dbtable指针
	* 作者		:  jin.shaohua
	*******************************************************************************/
	void * TMdbMalloc::ReAlloc(void * p,int iOldSize,int iNewSize)
	{
		void *pNew = 0;
		if (iOldSize>iNewSize)
		{
			return p;
		}
		if(p == NULL)
		{
			return MallocRaw(iNewSize);
		}
		pNew = MallocRaw(iNewSize);
		if(pNew != NULL)
		{
			memcpy(pNew,p,static_cast<size_t>(iOldSize));
			free(p);
		}
		return pNew;
	}

	/******************************************************************************
	* 函数名称	:  MallocRaw
	* 函数描述	:  分配空间
	* 输入		:  n 分配空间大小
	* 输出		:  无
	* 返回值	:  新分配内存指针
	* 作者		:  jin.shaohua
	*******************************************************************************/
	void * TMdbMalloc::MallocRaw(int n)
	{
		return malloc(static_cast<size_t>(n));
	}

	/******************************************************************************
	* 函数名称	:  NameFromToken
	* 函数描述	:  从token中获取name
	* 输入		:  pToken
	* 输出		:  无
	* 返回值	:  name
	* 作者		:  jin.shaohua
	*******************************************************************************/
	char * TMdbMalloc::NameFromToken(const Token * pToken)
	{
		char *zName;
		if(pToken != NULL)
		{
			zName = new char[pToken->n + 1];
			memset(zName,0,pToken->n + 1);
			strncpy(zName,pToken->z,pToken->n);
			if('\'' == zName[0])
			{
				TMdbNtcStrFunc::Trim(zName,zName[0]);
			}
			else if('"' == zName[0])
			{
				TMdbNtcStrFunc::Trim(zName,zName[0]);
			}
		}
		else
		{
			zName = NULL;
		}
		return zName;
	}
	/******************************************************************************
	* 函数名称	:  NameFromToken
	* 函数描述	: 从token获取名字
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbMalloc::NameFromToken(const Token * pToken,char * sName,int iMaxLen)
	{
		int iRet = 0;
		char * sTemp = NameFromToken(pToken);
		CHECK_OBJ(sTemp);
		do 
		{
			if(strlen(sTemp) >= static_cast<size_t>(iMaxLen))
			{
				CHECK_RET(ERR_SQL_TOO_LONG,"name=[%s],max=len=[%d]",sTemp,iMaxLen);
			}
			TMdbNtcStrFunc::StrCopy(sName,sTemp,static_cast<MDB_UINT32>(iMaxLen));
		} while (0);
		SAFE_DELETE(sTemp);
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  CopyFromStr
	* 函数描述	:  拷贝一份string
	* 输入		:  sStr 原字符串
	* 输出		:  无
	* 返回值	:  copy后的字符串
	* 作者		:  jin.shaohua
	*******************************************************************************/
	char * TMdbMalloc::CopyFromStr(const char * sStr)
	{
		char *zName;
		if(sStr != NULL)
		{
			size_t iSize = strlen(sStr) + 1;
			zName = new char[iSize];
			memset(zName,0,iSize);
			strncpy(zName,sStr,iSize);
		}
		else
		{
			zName = NULL;
		}
		return zName;

	}

	/******************************************************************************
	* 函数名称	:  ReleaseStr
	* 函数描述	:  释放资源
	* 输入		:  sStr 内存指针
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  jin.shaohua
	*******************************************************************************/
	void TMdbMalloc::ReleaseStr(char * & sStr)
	{
        SAFE_DELETE_ARRAY(sStr);
	}

	/******************************************************************************
	* 函数名称	:  AllocExpr
	* 函数描述	:  分配ST_EXPR
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  ST_EXPR指针
	* 作者		:  jin.shaohua
	*******************************************************************************/
	ST_EXPR * TMdbMalloc::AllocExpr()
	{
		return new ST_EXPR();
	}

	/******************************************************************************
	* 函数名称	:  ReleaseExpr
	* 函数描述	:  释放st_expr
	* 输入		:  pstExpr 表达式
	* 输出		:  无
	* 返回值	:  0 成功，否则失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbMalloc::ReleaseExpr(ST_EXPR * & pstExpr)
	{
		if (NULL == pstExpr)
		{
			return 0;
		}	
		SAFE_DELETE_ARRAY(pstExpr->sTorken);
		ReleaseExprFunc(pstExpr->pFunc);
		ReleaseExpr(pstExpr->pLeft);
		ReleaseExpr(pstExpr->pRight);
		ReleaseMemValue(pstExpr->pExprValue);//删除值
		SAFE_DELETE(pstExpr);
		return 0;
	}

	/******************************************************************************
	* 函数名称	:  AllocExprList
	* 函数描述	:  分配ST_EXPR_LIST
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  ST_EXPR_LIST 表达式链表指针
	* 作者		:  jin.shaohua
	*******************************************************************************/
	ST_EXPR_LIST * TMdbMalloc::AllocExprList()
	{
		return new ST_EXPR_LIST();
	}

	/******************************************************************************
	* 函数名称	:  ReleaseExprList
	* 函数描述	:  释放st_expr_list
	* 输入		:  pstExprList 表达式链表指针
	* 输出		:  无
	* 返回值	:  0 成功，否则失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbMalloc::ReleaseExprList(ST_EXPR_LIST * & pstExprList)
	{
		if (NULL == pstExprList)
		{
			return 0;
		}
		int i = 0;
		for (i = 0;i<pstExprList->iItemNum;i++)
		{
			ST_EXPR_ITEM & item = pstExprList->pExprItems[i];
			ReleaseExpr(item.pExpr);
			SAFE_DELETE_ARRAY(item.sName);
			SAFE_DELETE_ARRAY(item.sSpan);
		}
		SAFE_FREE(pstExprList->pExprItems);
		SAFE_DELETE(pstExprList);

		return 0;
	}

	/******************************************************************************
	* 函数名称	:  AllocExprFunc
	* 函数描述	:  分配ST_EXPR_FUNC
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  ST_EXPR_FUNC 函数项
	* 作者		:  jin.shaohua
	*******************************************************************************/
	ST_EXPR_FUNC * TMdbMalloc::AllocExprFunc()
	{
		return new ST_EXPR_FUNC();
	}

	/******************************************************************************
	* 函数名称	:  ReleaseExprFunc
	* 函数描述	:  获取mdbtable指针
	* 输入		:  pstExprFunc 函数项指针
	* 输出		:  无
	* 返回值	:  0 成功，否则失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbMalloc::ReleaseExprFunc(ST_EXPR_FUNC * & pstExprFunc)
	{
		if (NULL == pstExprFunc)
		{
			return 0;
		}
	//	SAFE_DELETE_ARR(pstExprFunc->sName);
		SAFE_DELETE(pstExprFunc->pFuncDef);
		ReleaseExprList(pstExprFunc->pFuncArgs);
		SAFE_DELETE(pstExprFunc);
		return 0;
	}

	/******************************************************************************
	* 函数名称	:  AllocIdList
	* 函数描述	:  分配ST_ID_LIST
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  ST_ID_LIST 指针
	* 作者		:  jin.shaohua
	*******************************************************************************/
	ST_ID_LIST * TMdbMalloc::AllocIdList()
	{
		return new ST_ID_LIST();
	}

	/******************************************************************************
	* 函数名称	:  ReleaseIdList
	* 函数描述	:  获取mdbtable指针
	* 输入		:  pstIdList 表名
	* 输出		:  无
	* 返回值	:  0 成功，否则失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbMalloc::ReleaseIdList(ST_ID_LIST * & pstIdList)
	{
		if(NULL == pstIdList){return 0;}
		if(pstIdList->iAllocNum > 0)
		{
			int i = 0;
			for(i = 0;i<pstIdList->iIdNum;++i)
			{
				ReleaseStr(pstIdList->pstItem[i].zName);
			}
					SAFE_FREE(pstIdList->pstItem);
			//SAFE_DELETE_ARRAY(pstIdList->pstItem);
		}
		SAFE_DELETE(pstIdList);
		return 0;
	}

	/******************************************************************************
	* 函数名称	:  ArrayAllocate
	* 函数描述	:  获取mdbtable指针
	* 输入		:  sTablename 表名
	* 输出		:  无
	* 返回值	:  dbtable指针
	* 作者		:  jin.shaohua
	*******************************************************************************/
	void * TMdbMalloc::ArrayAllocate(
							void *pArray,     /* Array of objects.  Might be reallocated */
							int szEntry,      /* Size of each object in the array */
							int initSize,     /* Suggested initial allocation, in elements */
							int *pnEntry,     /* Number of objects currently in use */
							int *pnAlloc,     /* Current size of the allocation, in elements */
							int *pIdx         /* Write the index of a new slot here */
		)
	{
		char *z;
		if( *pnEntry >= *pnAlloc ){
			void *pNew;
			int newSize;
			newSize = (*pnAlloc)*2 + initSize;
			pNew = ReAlloc(pArray, (*pnAlloc)*szEntry,newSize*szEntry);
			if( pNew==0 ){
				*pIdx = -1;
				return pArray;
			}
			*pnAlloc = newSize;
			pArray = pNew;
		}
		z = (char*)pArray;
		memset(&z[*pnEntry * szEntry], 0, static_cast<size_t>(szEntry));
		*pIdx = *pnEntry;
		++*pnEntry;
		return pArray;
	}


	/******************************************************************************
	* 函数名称	:  AllocMemValue
	* 函数描述	:  分配memvalue
	* 输入		:  ST_MEM_VALUE 值结构指针
	* 输出		:  无
	* 返回值	:  ST_MEM_VALUE 指针
	* 作者		:  jin.shaohua
	*******************************************************************************/
	ST_MEM_VALUE * TMdbMalloc::AllocMemValue(ST_MEM_VALUE * pstOld)
	{
		ST_MEM_VALUE * pNew = pstOld;
		if (NULL == pNew)
		{
			pNew = new ST_MEM_VALUE();
		}
		return pNew;
	}

	/******************************************************************************
	* 函数名称	:  ResetMemValue
	* 函数描述	:  重置  memvalue
	* 输入		:  pstMemValue 值结构指针
	* 输出		:  无
	* 返回值	:  ST_MEM_VALUE指针
	* 作者		:  jin.shaohua
	*******************************************************************************/
	ST_MEM_VALUE * TMdbMalloc::ResetMemValue(ST_MEM_VALUE * & pstMemValue)
	{
		if(NULL == pstMemValue)
		{
			return AllocMemValue(pstMemValue);
		}
		else
		{
			//sAlias ,pColumn不重置
			pstMemValue->iFlags = 0;
			pstMemValue->iSize  = 0;
			pstMemValue->lValue = 0;
			if((pstMemValue->iFlags & (MEM_Str|MEM_Dyn)) != 0)
			{//动态分配的str串才会被删除
				ReleaseStr(pstMemValue->sValue);
			}
			pstMemValue->sValue = NULL;
			return pstMemValue;
		}
	}

	/******************************************************************************
	* 函数名称	:  ReleaseMemValue
	* 函数描述	:  释放  memvalue
	* 输入		:  pstMemValue 值结构指针
	* 输出		:  无
	* 返回值	:  0 成功，否则失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbMalloc::ReleaseMemValue(ST_MEM_VALUE * & pstMemValue)
	{
		if (NULL != pstMemValue)
		{
			if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
			{
				if(MemValueHasAnyProperty(pstMemValue,MEM_Dyn))
				{//动态生成的需要销毁
					ReleaseStr(pstMemValue->sValue);
				}
			}
			pstMemValue->sValue = NULL;
			//ReleaseStr(pstMemValue->sAlias);
		}
		SAFE_DELETE(pstMemValue);
		return 0;
	}

	/******************************************************************************
	* 函数名称	:  AllocMemValueArray
	* 函数描述	:  分配memvalue
	* 输入		:  iSize 分配数组大小
	* 输出		:  无
	* 返回值	:  ST_MEM_VALUE指针
	* 作者		:  jin.shaohua
	*******************************************************************************/
	ST_MEM_VALUE ** TMdbMalloc::AllocMemValueArray(int iSize)
	{
		if(iSize <= 0){return NULL;}
		ST_MEM_VALUE ** pRet = new ST_MEM_VALUE *[iSize];	
		return pRet;
	}

	/******************************************************************************
	* 函数名称	:  ReleaseMemValueArray
	* 函数描述	:  获取mdbtable指针
	* 输入		:  pstMemValueArray 值结构数组指针
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  jin.shaohua
	*******************************************************************************/
	void TMdbMalloc::ReleaseMemValueArray(ST_MEM_VALUE ** & pstMemValueArray)
	{
		if(NULL != pstMemValueArray)
		{
			delete[] pstMemValueArray;
		}
	}

	/******************************************************************************
	* 函数名称	:  TDynamicMemBlock
	* 函数描述	:  构造与析构
	* 输入		:  
	* 输入		:  
	* 输出		:  
	* 返回值	:  
	* 作者		:  jin.shaohua
	*******************************************************************************/
	TDynamicMemBlock::TDynamicMemBlock():
	m_iCurSize(0),
	m_iMaxSize(0),
	m_iDataPos(0),
	m_iIncreaseSize(0),
	m_pAddr(NULL)
	{

	}
	TDynamicMemBlock::~TDynamicMemBlock()
	{
		SAFE_FREE(m_pAddr);
	}
	/******************************************************************************
	* 函数名称	:  Init
	* 函数描述	:  初始化，若已有空间则清空
	* 输入		:  
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TDynamicMemBlock::Init(size_t iMinSize,size_t iMaxSize,size_t iIncreaseSize)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		if(0 == m_iCurSize)
		{//暂没有空间
			m_pAddr = (char *)QMDB_MALLOC->MallocRaw(static_cast<int>(iMinSize));
			CHECK_OBJ(m_pAddr);
			m_iCurSize = iMinSize;
		}
		else if(iMinSize <= m_iCurSize)
		{//空间足够
		}
		else if(iMinSize > m_iCurSize)
		{//最小空间不够
			SAFE_FREE(m_pAddr);
			m_pAddr = (char *)QMDB_MALLOC->MallocRaw(static_cast<int>(iMinSize));
			CHECK_OBJ(m_pAddr);
			m_iCurSize = iMinSize;

		}
		m_iMaxSize = iMaxSize;
		m_iIncreaseSize = iIncreaseSize;
		Clear();
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  插入数据，可能需要扩展空间
	* 函数描述	:  
	* 输入		:  
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TDynamicMemBlock::InsertData(char * pDataAddr,size_t iValueSize)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		if(m_iCurSize >= iValueSize + m_iDataPos)
		{//空间足够
        
		}
		else
		{//空间不够
			size_t iExpandSize = (iValueSize % m_iIncreaseSize +1)*m_iIncreaseSize;//需要扩展的空间
			if(iExpandSize + m_iCurSize > m_iMaxSize)
			{
				CHECK_RET(ERR_OS_NO_MEMROY,"valusSize[%lld] cannot insert,maxsize[%lld]",iValueSize,m_iMaxSize);
			}
			m_pAddr = (char *)QMDB_MALLOC->ReAlloc(m_pAddr,static_cast<int>(m_iCurSize),static_cast<int>(iExpandSize + m_iCurSize));
			CHECK_OBJ(m_pAddr);
			m_iCurSize += iExpandSize;
		}
		memcpy(m_pAddr+m_iDataPos,pDataAddr,iValueSize);//数据拷贝
		m_iDataPos += iValueSize;//数据位置增长
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* 函数名称	:  Clear
	* 函数描述	:  清理数据
	* 输入		:  
	* 输入		:  
	* 输出		:  
	* 返回值	:  0 - 成功!0 -失败
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TDynamicMemBlock::Clear()
	{
		if(NULL == m_pAddr){return 0;}
		if(m_iDataPos != 0)
		{
			memset(m_pAddr,0,m_iDataPos);
		}
		m_iDataPos = 0;
		return 0;
	}
//}

