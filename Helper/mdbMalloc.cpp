/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbMalloc.cpp		
*@Description�� mdb�ڴ��������ͷŵ�ͳһ�ӿڡ�����ģʽ
*@Author:			jin.shaohua
*@Date��	    2012.05
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
	* ��������	:  Instance
	* ��������	:  ��ȡTMdbMallocΨһʵ��
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  TMdbMallocʵ��
	* ����		:  jin.shaohua
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
	* ��������	:  ReAlloc
	* ��������	:  ���·����С
	* ����		:  sTablename ����
	* ���		:  ��
	* ����ֵ	:  dbtableָ��
	* ����		:  jin.shaohua
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
	* ��������	:  MallocRaw
	* ��������	:  ����ռ�
	* ����		:  n ����ռ��С
	* ���		:  ��
	* ����ֵ	:  �·����ڴ�ָ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	void * TMdbMalloc::MallocRaw(int n)
	{
		return malloc(static_cast<size_t>(n));
	}

	/******************************************************************************
	* ��������	:  NameFromToken
	* ��������	:  ��token�л�ȡname
	* ����		:  pToken
	* ���		:  ��
	* ����ֵ	:  name
	* ����		:  jin.shaohua
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
	* ��������	:  NameFromToken
	* ��������	: ��token��ȡ����
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
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
	* ��������	:  CopyFromStr
	* ��������	:  ����һ��string
	* ����		:  sStr ԭ�ַ���
	* ���		:  ��
	* ����ֵ	:  copy����ַ���
	* ����		:  jin.shaohua
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
	* ��������	:  ReleaseStr
	* ��������	:  �ͷ���Դ
	* ����		:  sStr �ڴ�ָ��
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	void TMdbMalloc::ReleaseStr(char * & sStr)
	{
        SAFE_DELETE_ARRAY(sStr);
	}

	/******************************************************************************
	* ��������	:  AllocExpr
	* ��������	:  ����ST_EXPR
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ST_EXPRָ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	ST_EXPR * TMdbMalloc::AllocExpr()
	{
		return new ST_EXPR();
	}

	/******************************************************************************
	* ��������	:  ReleaseExpr
	* ��������	:  �ͷ�st_expr
	* ����		:  pstExpr ���ʽ
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�������ʧ��
	* ����		:  jin.shaohua
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
		ReleaseMemValue(pstExpr->pExprValue);//ɾ��ֵ
		SAFE_DELETE(pstExpr);
		return 0;
	}

	/******************************************************************************
	* ��������	:  AllocExprList
	* ��������	:  ����ST_EXPR_LIST
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ST_EXPR_LIST ���ʽ����ָ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	ST_EXPR_LIST * TMdbMalloc::AllocExprList()
	{
		return new ST_EXPR_LIST();
	}

	/******************************************************************************
	* ��������	:  ReleaseExprList
	* ��������	:  �ͷ�st_expr_list
	* ����		:  pstExprList ���ʽ����ָ��
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�������ʧ��
	* ����		:  jin.shaohua
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
	* ��������	:  AllocExprFunc
	* ��������	:  ����ST_EXPR_FUNC
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ST_EXPR_FUNC ������
	* ����		:  jin.shaohua
	*******************************************************************************/
	ST_EXPR_FUNC * TMdbMalloc::AllocExprFunc()
	{
		return new ST_EXPR_FUNC();
	}

	/******************************************************************************
	* ��������	:  ReleaseExprFunc
	* ��������	:  ��ȡmdbtableָ��
	* ����		:  pstExprFunc ������ָ��
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�������ʧ��
	* ����		:  jin.shaohua
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
	* ��������	:  AllocIdList
	* ��������	:  ����ST_ID_LIST
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ST_ID_LIST ָ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	ST_ID_LIST * TMdbMalloc::AllocIdList()
	{
		return new ST_ID_LIST();
	}

	/******************************************************************************
	* ��������	:  ReleaseIdList
	* ��������	:  ��ȡmdbtableָ��
	* ����		:  pstIdList ����
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�������ʧ��
	* ����		:  jin.shaohua
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
	* ��������	:  ArrayAllocate
	* ��������	:  ��ȡmdbtableָ��
	* ����		:  sTablename ����
	* ���		:  ��
	* ����ֵ	:  dbtableָ��
	* ����		:  jin.shaohua
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
	* ��������	:  AllocMemValue
	* ��������	:  ����memvalue
	* ����		:  ST_MEM_VALUE ֵ�ṹָ��
	* ���		:  ��
	* ����ֵ	:  ST_MEM_VALUE ָ��
	* ����		:  jin.shaohua
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
	* ��������	:  ResetMemValue
	* ��������	:  ����  memvalue
	* ����		:  pstMemValue ֵ�ṹָ��
	* ���		:  ��
	* ����ֵ	:  ST_MEM_VALUEָ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	ST_MEM_VALUE * TMdbMalloc::ResetMemValue(ST_MEM_VALUE * & pstMemValue)
	{
		if(NULL == pstMemValue)
		{
			return AllocMemValue(pstMemValue);
		}
		else
		{
			//sAlias ,pColumn������
			pstMemValue->iFlags = 0;
			pstMemValue->iSize  = 0;
			pstMemValue->lValue = 0;
			if((pstMemValue->iFlags & (MEM_Str|MEM_Dyn)) != 0)
			{//��̬�����str���Żᱻɾ��
				ReleaseStr(pstMemValue->sValue);
			}
			pstMemValue->sValue = NULL;
			return pstMemValue;
		}
	}

	/******************************************************************************
	* ��������	:  ReleaseMemValue
	* ��������	:  �ͷ�  memvalue
	* ����		:  pstMemValue ֵ�ṹָ��
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�������ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbMalloc::ReleaseMemValue(ST_MEM_VALUE * & pstMemValue)
	{
		if (NULL != pstMemValue)
		{
			if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
			{
				if(MemValueHasAnyProperty(pstMemValue,MEM_Dyn))
				{//��̬���ɵ���Ҫ����
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
	* ��������	:  AllocMemValueArray
	* ��������	:  ����memvalue
	* ����		:  iSize ���������С
	* ���		:  ��
	* ����ֵ	:  ST_MEM_VALUEָ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	ST_MEM_VALUE ** TMdbMalloc::AllocMemValueArray(int iSize)
	{
		if(iSize <= 0){return NULL;}
		ST_MEM_VALUE ** pRet = new ST_MEM_VALUE *[iSize];	
		return pRet;
	}

	/******************************************************************************
	* ��������	:  ReleaseMemValueArray
	* ��������	:  ��ȡmdbtableָ��
	* ����		:  pstMemValueArray ֵ�ṹ����ָ��
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	void TMdbMalloc::ReleaseMemValueArray(ST_MEM_VALUE ** & pstMemValueArray)
	{
		if(NULL != pstMemValueArray)
		{
			delete[] pstMemValueArray;
		}
	}

	/******************************************************************************
	* ��������	:  TDynamicMemBlock
	* ��������	:  ����������
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  
	* ����		:  jin.shaohua
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
	* ��������	:  Init
	* ��������	:  ��ʼ���������пռ������
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TDynamicMemBlock::Init(size_t iMinSize,size_t iMaxSize,size_t iIncreaseSize)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		if(0 == m_iCurSize)
		{//��û�пռ�
			m_pAddr = (char *)QMDB_MALLOC->MallocRaw(static_cast<int>(iMinSize));
			CHECK_OBJ(m_pAddr);
			m_iCurSize = iMinSize;
		}
		else if(iMinSize <= m_iCurSize)
		{//�ռ��㹻
		}
		else if(iMinSize > m_iCurSize)
		{//��С�ռ䲻��
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
	* ��������	:  �������ݣ�������Ҫ��չ�ռ�
	* ��������	:  
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TDynamicMemBlock::InsertData(char * pDataAddr,size_t iValueSize)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
		if(m_iCurSize >= iValueSize + m_iDataPos)
		{//�ռ��㹻
        
		}
		else
		{//�ռ䲻��
			size_t iExpandSize = (iValueSize % m_iIncreaseSize +1)*m_iIncreaseSize;//��Ҫ��չ�Ŀռ�
			if(iExpandSize + m_iCurSize > m_iMaxSize)
			{
				CHECK_RET(ERR_OS_NO_MEMROY,"valusSize[%lld] cannot insert,maxsize[%lld]",iValueSize,m_iMaxSize);
			}
			m_pAddr = (char *)QMDB_MALLOC->ReAlloc(m_pAddr,static_cast<int>(m_iCurSize),static_cast<int>(iExpandSize + m_iCurSize));
			CHECK_OBJ(m_pAddr);
			m_iCurSize += iExpandSize;
		}
		memcpy(m_pAddr+m_iDataPos,pDataAddr,iValueSize);//���ݿ���
		m_iDataPos += iValueSize;//����λ������
		TADD_FUNC("Finish.");
		return iRet;
	}
	/******************************************************************************
	* ��������	:  Clear
	* ��������	:  ��������
	* ����		:  
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
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

