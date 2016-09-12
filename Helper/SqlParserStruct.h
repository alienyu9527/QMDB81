/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    SqlParserStruct.h		
*@Description�� SQL��������ʹ�õ�һЩ�ṹ�嶨��
*@Author:			jin.shaohua
*@Date��	    2012.07
*@History:
******************************************************************************************/

#ifndef _SQL_PARSER_STRUCT_H_
#define _SQL_PARSER_STRUCT_H_
#include <string>
#include <vector>
#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"

//namespace QuickMDB{

#define REPLACE_NULL_STR(_obj)  NULL == (_obj)?"nil":_obj

#define CHECK_OBJ_RET_0(_obj) if (NULL == _obj)\
{\
	return 0;\
}

typedef struct _ST_EXPR_FUNC ST_EXPR_FUNC;
typedef struct _ST_EXPR ST_EXPR;
typedef struct _ST_EXPR_ITEM ST_EXPR_ITEM;
typedef struct _ST_EXPR_LIST ST_EXPR_LIST;
typedef struct _ST_SQL_STRUCT ST_SQL_STRUCT;
typedef struct _ST_EXPR_SPAN ST_EXPR_SPAN;
typedef struct _Token Token;
typedef struct _LikeOp LikeOp;

typedef struct _ST_ID_LIST ST_ID_LIST;
typedef struct _ID_LIST_ITEM ID_LIST_ITEM;


typedef struct _ST_MEM_VALUE     ST_MEM_VALUE;
typedef struct _ST_FUNC_CONTEXT  ST_FUNC_CONTEXT;

typedef struct _ST_FUNC_DEF_HASH ST_FUNC_DEF_HASH;
typedef struct _ST_WALKER  ST_WALKER;

typedef struct _ST_INDEX_COLUMN ST_INDEX_COLUMN;
typedef struct _ST_INDEX_VALUE ST_INDEX_VALUE;
typedef struct _ST_SET_LIST  ST_SET_LIST;
typedef struct _ST_ORDERBY_INFO ST_ORDERBY_INFO;
typedef struct _ST_MEM_VALUE_LIST ST_MEM_VALUE_LIST;
typedef struct _ST_DDL_SQL_STRUCT ST_DDL_SQL_STRUCT;

typedef struct _ST_WHERE_OR_CLAUSE ST_WHERE_OR_CLAUSE ;
typedef struct _ST_WHERE_AND_CLAUSE ST_WHERE_AND_CLAUSE ;

class TMdbFuncBase;//����ָ��
#define MAX_ORDERBY_COUNT 10  //orderby���Ԫ�ظ���
// orderby 
struct _ST_ORDERBY_INFO
{
	int iNum;//orderby ��Ԫ�ظ���
	int iTypeArray[MAX_ORDERBY_COUNT];//���Ͷ���
	int iSizeArray[MAX_ORDERBY_COUNT];//��С����
	int iOffsetArray[MAX_ORDERBY_COUNT];//����ƫ��
	int iSortType[MAX_ORDERBY_COUNT];//��������
	int iTotalSize;//�ܴ�С
	void clear()
	{
		memset(this,0x00,sizeof(_ST_ORDERBY_INFO));
	}
};
/**
* where �����е�and��
*/
struct _ST_WHERE_AND_CLAUSE
{
    void Clear()
    {
        m_vExpr.clear();
    }
    std::vector<ST_EXPR *> m_vExpr;
};

/**
* where ������ʾ��or��
*/
struct _ST_WHERE_OR_CLAUSE
{
    void Clear()
    {
        size_t i = 0;
        for(i = 0;i < m_vAndClause.size();++i)
        {
            m_vAndClause[i].Clear();
        }
        m_vAndClause.clear();
    }
    std::vector<ST_WHERE_AND_CLAUSE> m_vAndClause;//��op=TK_ORʱ��
};


/* 
* calc flag
*/
#define CALC_NO_NEED      0x0001   /*no need to calc*/
#define CALC_PER_Row       0x0002 /*calc per one row*/ 
#define CALC_PER_Exec     0x0004 /*calc per one Execute*/
#define CALC_PER_Query   0x0008 /*calc per one Query*/

/*
** These macros can be used to test, set, or clear bits in the 
** _ST_EXPR.iCalcFlags
*/
#define ExprCalcHasProperty(V,P)     (((V)->iCalcFlags&(P))==(P))
#define ExprCalcHasAnyProperty(V,P)  (((V)->iCalcFlags&(P))!=0)
#define ExprCalcSetProperty(V,P)     (V)->iCalcFlags|=(P)
#define ExprCalcClearProperty(V,P)   (V)->iCalcFlags&=~(P)


/**
* ���ʽ�ṹ��Ϣ
*/
struct _ST_EXPR
{
	int iOpcode;//������
	int flags;	 //��ʶexpr��һЩ����
	char * sTorken;//torken��
	_ST_EXPR * pLeft;//������
	_ST_EXPR * pRight; //������
	_ST_EXPR_FUNC * pFunc;//������Ϣָ��
	_ST_MEM_VALUE * pExprValue;//�ñ��ʽ��ֵ
	int iCalcFlags;//�����ʶ:�Ƿ���Ҫ���㣬�������� 
	bool bNeedToCalc()/*�Ƿ���Ҫ����*/
       {
            return false == ExprCalcHasProperty(this,CALC_NO_NEED);
       }
	_ST_EXPR()
	{
		memset(this,0x00,sizeof(_ST_EXPR));
	}
};



/**
* Expr��
* pExpr/name��Ͽ�����Ϊ��ѯ�е� pExpr as name ������Ϊupdate�� name=:pExpr
*/
struct _ST_EXPR_ITEM
{
	_ST_EXPR * pExpr; //ָ��expr 
	char * sName;     //expr������ 
	char * sSpan;	 //expr��ԭʼ�ַ���
	int iSortOrder;   //���з�ʽ
	_ST_EXPR_ITEM()
	{
		memset(this,0x00,sizeof(_ST_EXPR_ITEM));
	}
};

/**
*Expr����
*/
struct _ST_EXPR_LIST
{
	int iItemNum;//item�ĸ���
	int iAllocNum;//�ѷ���ĸ���
	_ST_EXPR_ITEM *pExprItems;//ָ��expr_item����
	_ST_EXPR_LIST()
	{
		memset(this,0x00,sizeof(_ST_EXPR_LIST));
	}
};

/* One or more of the following flags are set to indicate the validOK
** representations of the value stored in the Mem struct.
**
** If the MEM_Null flag is set, then the value is an SQL NULL value.
** No other flags may be set in this case.
**
** If the MEM_Str flag is set then Mem.z points at a string representation.
** Usually this is encoded in the same unicode encoding as the main
** database (see below for exceptions). If the MEM_Term flag is also
** set, then the string is nul terminated. The MEM_Int and MEM_Real 
** flags may coexist with the MEM_Str flag.
*/
#define MEM_Null      0x0001   /* Value is NULL */
#define MEM_Str       0x0002   /* Value is a string */
#define MEM_Int       0x0004   /* Value is an integer */
#define MEM_Float     0x0008   /* Value is a real number */
#define MEM_Blob      0x0010   /* Value is a BLOB */
#define MEM_Const     0x0020   /* Value is a Const */ //����ֵ�����ɱ��޸�

#define MEM_Invalid   0x0080   /* Value is undefined */
#define MEM_TypeMask  0x00ff   /* Mask of type bits */

/* Whenever Mem contains a valid string or blob representation, one of
** the following flags must be set to determine the memory management
** policy for Mem.z.  The MEM_Term flag tells us whether or not the
** string is \000 or \u0000 terminated
*/
#define MEM_Variable  0x0100   /* Value is a bind value */
#define MEM_Term      0x0200   /* String rep is nul terminated */
#define MEM_Dyn       0x0400   /* Need to call Free() on Mem.z */
#define MEM_Date      0x0800   /* Mem.z points to a date value */
#define MEM_Ephem     0x1000   /* Mem.z points to an ephemeral string */
#define MEM_Agg       0x2000   /* Mem.z points to an agg function context */
#define MEM_Zero      0x4000   /* Mem.i contains count of 0s appended to blob */
#define MEM_LPM		  0x8000   /* Longest Prefix Match */

/*
** These macros can be used to test, set, or clear bits in the 
** MEM.iFlags field.
*/
#define MemValueHasProperty(V,P)     (((V)->iFlags&(P))==(P))
#define MemValueHasAnyProperty(V,P)  (((V)->iFlags&(P))!=0)
#define MemValueSetProperty(V,P)     (V)->iFlags|=(P)
#define MemValueClearProperty(V,P)   (V)->iFlags&=~(P)


//����memvalue��type��size//����string ���ڴ�ռ�
#define SET_MEM_TYPE_SIZE(M,T,S) MemValueSetProperty(M,(T));M->iSize = S;\
if(MemValueHasAnyProperty(M,MEM_Str|MEM_Date|MEM_Blob))\
{\
       if(NULL != M->sValue){delete [] (M->sValue);M->sValue=NULL;}\
	M->sValue = new char[S];\
	memset(M->sValue,0x00,(size_t)S);\
	MemValueSetProperty(M,MEM_Dyn);\
}

/*
* ֵ�ṹ��������ʾ����ֵ
*/
struct _ST_MEM_VALUE
{
	long long  lValue; //long long
	double     dValue;//double
	//bool bIsSeqValue; //�жϸ�ֵ�Ƿ�Ϊ����ֵ
	//long long lSeqValue;//���ڱ������е�nextval or currvalֵ
	int iSize;		  //string size;
	int iFlags;  /* Some combination of MEM_Null, MEM_Str, MEM_Dyn, etc. */
	TMdbColumn * pColumn;//TK_ID����ӦpColumn��Ϣ
	TMdbColumn * pColumnToSet;//update ��col= value,col ����Ӧ��pColumn��Ϣ
	int iColIndexToSet;//update ��col= value,col ����Ӧ��pColumn ����Ӧ��index
	char * sValue;    //string
	char * sAlias;
       int iAggCalcCounts;//�ۺϺ����������
	_ST_MEM_VALUE()
	{
		memset(this,0x00,sizeof(_ST_MEM_VALUE));
	}
	//����Ϊ����ֵ����
	void SetConst()
	{
		MemValueSetProperty(this,MEM_Const);
	}
       //�ж�NULLֵ
       bool IsNull()
       {
            return MemValueHasAnyProperty(this,MEM_Null);
       }

		// == ʹ���ǰ׺ƥ�䷽��
	   bool IsLPM()
       {
            return MemValueHasAnyProperty(this,MEM_LPM) && MemValueHasAnyProperty(this,MEM_Str);
       }
		
        //����ֵ
        void ClearValue()
        {
            if(MemValueHasProperty(this,MEM_Const)){return;} //����ֵ�����

            lValue = 0;
			dValue = 0.0;
            if(MemValueHasProperty(this,MEM_Dyn) && NULL != sValue)
            {
                sValue[0]= 0;
            }
            else
            {
                sValue = NULL;
            }
            
            MemValueClearProperty(this,MEM_Null);
            iAggCalcCounts = 0;
        }
       //����null
       void SetNull()
       {
            MemValueSetProperty(this,MEM_Null);
       }
       //��������
       void CopyToMemValue(_ST_MEM_VALUE * & pOut)
       {
            pOut->ClearValue();
            if(this->IsNull())
            {
                pOut->SetNull();
                return;
            }
            if(MemValueHasAnyProperty(pOut,MEM_Int) && MemValueHasAnyProperty(this,MEM_Int))
            {
                pOut->lValue = this->lValue;
            }
            if(MemValueHasAnyProperty(pOut,MEM_Date|MEM_Str|MEM_Blob) && MemValueHasAnyProperty(this,MEM_Date|MEM_Str|MEM_Blob))
            {
                SAFESTRCPY(pOut->sValue,pOut->iSize,this->sValue);
            }
            pOut->iAggCalcCounts =  this->iAggCalcCounts;
       }
       //����һ��
       _ST_MEM_VALUE * Dup()
       {
            _ST_MEM_VALUE * pNew = new _ST_MEM_VALUE();
            if(NULL == pNew){return NULL;}
            pNew->iFlags = this->iFlags;
            pNew->iSize  = this->iSize;
            pNew->pColumn = this->pColumn;
            pNew->pColumnToSet = this->pColumnToSet;
            pNew->iColIndexToSet = this->iColIndexToSet;
            pNew->pColumnToSet = this->pColumnToSet;
            pNew->sAlias = this->sAlias;
            if(NULL != this->sValue && 0 != pNew->iSize)
            {
                pNew->sValue = new char[pNew->iSize];
            }
            CopyToMemValue(pNew);
            return pNew;
       }
       //���
       std::string ToString()
       {
            char sTemp[MAX_BLOB_LEN] = {0};
            sprintf(sTemp,"sAlias[%s],flag[%d],lValue[%lld],sValue[%s],size[%d],iColIndexToSet[%d]",
                        REPLACE_NULL_STR(sAlias),iFlags,lValue,REPLACE_NULL_STR(sValue),iSize,iColIndexToSet);
            std::string sRet = sTemp;
            return sRet;
       }
};







/**
* ������
*/
struct _ST_EXPR_FUNC
{
	TMdbFuncBase * pFuncDef;//��������
	int iType;		//�������ͣ��Ƿ��ǾۺϺ���
	_ST_EXPR_LIST * pFuncArgs;//�����Ĳ����б�������ΪIN ,BETWEEN�Ĳ���
	_ST_EXPR_FUNC()
	{
		memset(this,0x00,sizeof(_ST_EXPR_FUNC));
	}
};

#if 0

//����������Ϣ
struct _ST_FUNC_DEFINE
{
	char * sName;//������
	int nArgs;   //�����Ĳ�������
	ST_MEM_VALUE ** pArgv;
	//void * pUserData;//�û�����
	int iFlags; 
	_ST_FUNC_DEFINE *pNext; /* ָ����һ��ͬ������ */
	void (*xFunc)(ST_FUNC_CONTEXT*,int,ST_MEM_VALUE**); /* ���溯������ */
	void (*xStep)(ST_FUNC_CONTEXT*,int,ST_MEM_VALUE**); /* �ۺϺ���step���� */
    void (*xFinalize)(ST_FUNC_CONTEXT*);                /* �ۺϺ�����final���� */
	_ST_FUNC_DEFINE * pHash;  /* ָ����һ��ͬhash�Ĳ�ͬ���ĺ��� */
	int iTempArgs;//��¼���������в�������
	ST_FUNC_CONTEXT tTempContext;//��¼���μ����������
	void (*xCheckArgs)();//У������ĺ���
	
};
#endif

//������������Ϣ
struct _ST_FUNC_CONTEXT
{
	_ST_MEM_VALUE  *ReturnValue;//���淵��ֵ
	_ST_MEM_VALUE * pMem;     //������������
	int iRetCode;			  //������
};

#define FUNC_AGGREAGATE  0x0001 /* function is aggreate*/

#define FuncHasProperty(V,P)     (((V)->m_iFlags&(P))==(P))
#define FuncHasAnyProperty(V,P)  (((V)->m_iFlags&(P))!=0)
#define FuncSetProperty(V,P)     (V)->m_iFlags|=(P)
#define FuncClearProperty(V,P)   (V)V->m_iFlags&=~(P)

#if 0
/*
** ���溯�������hash��
*/
struct _ST_FUNC_DEF_HASH {
	_ST_FUNC_DEFINE *a[23];       /* ������ϣ�� */
};
#endif



struct ST_TABLE_INDEX_INFO;

/*
** ��������
*/
struct _ST_INDEX_COLUMN
{
    ST_TABLE_INDEX_INFO * pstTableIndex;
    TMdbColumn                 * pColumn;  //�У�������������һ����
    std::vector<ST_EXPR *>  vExpr;//ֵ���ʽ,�����ж��
};

/*
** ������ֵ
*/
struct _ST_INDEX_VALUE
{
	//TMdbIndexNode * pIndexNode;
	ST_TABLE_INDEX_INFO * pstTableIndex;
	ST_EXPR       * pExprArr[MAX_INDEX_COLUMN_COUNTS];//����ֵ,�����Ǹ�������
	_ST_INDEX_VALUE()
	{
		int i = 0;
		for(i= 0;i<MAX_INDEX_COLUMN_COUNTS;i++)
		{
			pExprArr[i] = NULL;
		}
		pstTableIndex = NULL;
	}
    _ST_INDEX_VALUE& operator=(const _ST_INDEX_VALUE& s)//���������
     {
        this->pstTableIndex = s.pstTableIndex;
        int i = 0;
	for(i= 0;i<MAX_INDEX_COLUMN_COUNTS;i++)
	{
		this->pExprArr[i] = s.pExprArr[i];
	}
        return *this;
     }
    _ST_INDEX_VALUE(const _ST_INDEX_VALUE& s)//���ƹ��캯��
    {
            *this = s;
    }
   
};

/**
* select a,b,c from table_x where a=:a and b>:b
      \___/                   \__________/
      PColist                 pWhere

*��������sql�ṹ
*/
class TMdbShmDSN;
struct _ST_SQL_STRUCT
{
	int iSqlType;        //sql����,TK_SELECT,TK_INSERT,TK_UPDATE,TK_DELETE
	char * sTableName;    //����
	char * sAlias;		//��ı���
	TMdbTable * pMdbTable;
	bool bIsSimpleCol;
	bool bIsSingleGroupFunc;
	ST_TABLE_INDEX_INFO * pScanAllIndex;//ȫ������������
	TMdbShmDSN * pShmDSN;//�ڴ�DSN��Ϣ
	_ST_EXPR_LIST * pHintList;
	_ST_EXPR_LIST * pColList; //��ѯ����У�����insert,update��ֵ��
	_ST_EXPR      * pWhere;//where�Ӿ�
	_ST_EXPR_LIST * pGroupby;//group by �Ӿ�
	_ST_EXPR      * pHaving;//having�Ӿ�
	_ST_EXPR_LIST * pOrderby;//orderby�Ӿ�
	_ST_EXPR * pstLimit; //limit�Ӿ�
	_ST_EXPR * pstOffset;//offset �Ӿ�
	_ST_ID_LIST * pstIdList;//����insert ,update �е�����
	std::vector< _ST_INDEX_VALUE> vIndexUsed;//where ������ʹ������
	std::vector< _ST_INDEX_VALUE> vIndexChanged;// update set ,insert����ɵ��������
	bool    bCheckInsertPriKey;//����ʱ���Ƿ��������У��
	int iSelFlags;//select��flags
	ST_ORDERBY_INFO stOrderbyInfo;//orderbyԪ�ص�һЩ��Ϣ
	long long iRowPos;//�ڼ�����¼
	char sSQL[MAX_SQL_LEN];//sql���
	_ST_INDEX_VALUE stIndexForVerifyPK;//��������У�������
	_ST_EXPR * pstRouterID;//ָ��·��ID����
	ST_WHERE_OR_CLAUSE tWhereOrClause;//������where����
	std::vector<_ST_EXPR *> vExprCalcPerExec;  //ÿ��ִ��ִ��һ�ε�expr
       std::vector<_ST_EXPR *> vExprCalcPerQuery; //ÿ��queryִ��һ�ε�expr
	void clear()
	{
		iSqlType = 0;
		sTableName = NULL;
		sAlias	   = NULL;
		pMdbTable  = NULL;
	    bIsSimpleCol = false;
	    bIsSingleGroupFunc = false;
		pScanAllIndex = NULL;
		pScanAllIndex = NULL;
		pShmDSN    = NULL;
		pColList   = NULL;
		pWhere	   = NULL;
        pGroupby = NULL;
        pHaving    = NULL;
		pOrderby   = NULL;
		pstLimit   = NULL;
		pstOffset  = NULL;
		pstIdList  = NULL;
		pHintList = NULL;
		bCheckInsertPriKey = false;
		iSelFlags  = 0;
		stOrderbyInfo.clear();
		iRowPos    = 0;
		vIndexUsed.clear();
		vIndexChanged.clear();
		sSQL[0] = 0;
              pstRouterID = NULL;
              tWhereOrClause.Clear();
              vExprCalcPerExec.clear();
              vExprCalcPerQuery.clear();
	}
	_ST_SQL_STRUCT()
	{
		clear();
	}
};

/*
*DDL SQL�ṹ��
*/
struct _ST_DDL_SQL_STRUCT
{
	int iSqlType[2];
	int iModifyColumnPos;
	bool  bIfNE;
	char sDsnName[128];
	char sTableSpaceName[128];
	char sTableAttr[64];
	char sTableAttrValue[MAX_SQL_LEN*4];
	char sIndexName[MAX_NAME_LEN];
    char sNewTableName[MAX_NAME_LEN];//rename table name
	TMDbUser  *pUser;
	TMdbCfgDSN  *pDsn;
	TMdbCfgProAttr *pProAttr;
	TMdbTable *pTable;
	TMdbTableSpace *pTablespace;
	TMdbOtherAttr m_tAttr[MAX_SQL_COUNTS];
	TMemSeq tMemSeq; //���Ӻ�ɾ������ʹ��
	TMdbJob *pMdbJob; //�����޸�ɾ��jobʱʹ��
	TMdbParameter tParam;//�û�flush-sql or load-sqlʹ��
	std::vector<string> vModifyColumnAttr;
	void clear()
	{
		iSqlType[0] = -1;
		iSqlType[1] = -1;
		bIfNE = false;
		pUser = NULL;
		pDsn  = NULL;
		pProAttr = NULL;
		pTable = NULL;
		pTablespace = NULL;
		pMdbJob = NULL;
		iModifyColumnPos = -1;
		memset(sIndexName,0,sizeof(sIndexName));
		memset(sDsnName,0,sizeof(sDsnName));
		memset(sTableSpaceName,0,sizeof(sTableSpaceName));
		memset(sTableAttr,0,sizeof(sTableAttr));
		memset(sTableAttrValue,0,sizeof(sTableAttrValue));
		for(int i=0;i<MAX_SQL_COUNTS;i++)
		{
			m_tAttr[i].Clear();
		}
		tMemSeq.Clear();
		tParam.Clear();
		vModifyColumnAttr.clear();
        sNewTableName[0] = 0;
	}
	_ST_DDL_SQL_STRUCT()
	{
		clear();
	}
};
/*
** Allowed values for Select.selFlags.  The "SF" prefix stands for
** "Select Flag".
*/
#define SF_Distinct        0x01  /* Output should be DISTINCT */
#define SF_Resolved        0x02  /* Identifiers have been resolved */
#define SF_Aggregate       0x04  /* Contains aggregate functions */
#define SF_UsesEphemeral   0x08  /* Uses the OpenEphemeral opcode */
#define SF_Expanded        0x10  /* sqlite3SelectExpand() called on this */
#define SF_HasTypeInfo     0x20  /* FROM subqueries have Table metadata */
#define SF_UseSorter       0x40  /* Sort using a sorter */


#define SelectSqlHasProperty(V,P)     (((V)->iSelFlags&(P))==(P))
#define SelectSqlHasAnyProperty(V,P)  (((V)->iSelFlags&(P))!=0)
#define SelectSqlSetProperty(V,P)     (V)->iSelFlags|=(P)
#define SelectSqlClearProperty(V,P)   (V)V->iSelFlags&=~(P)


/*
*������¼һЩ����Ϣ�ڵ㡣insert�е��б�ʶ
*/
struct _ID_LIST_ITEM {
	char *zName;      /* Name of the identifier */
//	int idx;          /* Index in some Table.aCol[] of a column named zName */
	TMdbColumn * pColumn;//��ָ��
	ST_EXPR * pExpr;//ID����Ӧ��ֵһ������insert ,update set�С�
};
/*
* id list
*/
struct _ST_ID_LIST {
	_ID_LIST_ITEM *pstItem;
	int iIdNum;         /* Number of identifiers on the list */
	int iAllocNum;      /* Number of entries allocated for a[] below */
};

struct _Token {
	const char *z;     /* Text of the token.  Not NULL-terminated! */
	unsigned int n;    /* Number of characters in this token */

};

struct _ST_EXPR_SPAN
{
  _ST_EXPR *pExpr;          /* The expression parse tree */
  const char *zStart;   /* First character of input text */
  const char *zEnd;     /* One character past the end of input text */
};

/*
** Constant tokens for values 0 and 1.
*/
const Token mdbIntTokens[] = {
	{ "0", 1 },
	{ "1", 1 }
};
//IN�ؼ��ֵ�token
const Token mdbFuncInToken = {"in",2};
/*
** An instance of this structure is used to store the LIKE,
** GLOB, NOT LIKE, and NOT GLOB operators.
*/
struct _LikeOp {
	Token eOperator;  /* "like" or "glob" or "regexp" */
	int iNot;         /* True if the NOT keyword is present */
};



/*
** An instance of this structure can hold a simple list of identifiers,
** such as the list "a,b,c" in the following statements:
**
**      INSERT INTO t(a,b,c) VALUES ...;
**      CREATE INDEX idx ON t(a,b,c);
**      CREATE TRIGGER trig BEFORE UPDATE ON t(a,b,c) ...;
**
** The IdList.a.idx field is used when the IdList represents the list of
** column names after a table name in an INSERT statement.  In the statement
**
**     INSERT INTO t(a,b,c) ...
**
** If "a" is the k-th column of table "t", then IdList.a[0].idx==k.
*/


/*
** The following are the meanings of bits in the Expr.flags field.
*/

//#define EP_FromJoin   0x0001  /* Originated in ON or USING clause of a join */
//#define EP_Agg        0x0002  /* Contains one or more aggregate functions */
#define EP_Resolved   0x0004  /* IDs have been resolved to COLUMNs */
#define EP_Error      0x0008  /* Expression contains one or more errors */
#define EP_Distinct   0x0010  /* Aggregate function with DISTINCT keyword */
//#define EP_VarSelect  0x0020  /* pSelect is correlated, not constant */
#define EP_DblQuoted  0x0040  /* token.z was originally in "..." */
#define EP_InfixFunc  0x0080  /* True for an infix function: LIKE, GLOB, etc */
//#define EP_ExpCollate 0x0100  /* Collating sequence specified explicitly */
//#define EP_FixedDest  0x0200  /* Result needed in a specific register */
#define EP_IntValue   0x0400  /* Integer value contained in u.iValue */
//#define EP_xIsSelect  0x0800  /* x.pSelect is valid (otherwise x.pList is) */
//#define EP_Hint       0x1000  /* Optimizer hint. Not required for correctness */
//#define EP_Reduced    0x2000  /* Expr struct is EXPR_REDUCEDSIZE bytes only */
//#define EP_TokenOnly  0x4000  /* Expr struct is EXPR_TOKENONLYSIZE bytes only */
//#define EP_Static     0x8000  /* Held in memory not obtained from malloc() */






/*
** These macros can be used to test, set, or clear bits in the 
** Expr.flags field.
*/
#define ExprHasProperty(E,P)     (((E)->flags&(P))==(P))
#define ExprHasAnyProperty(E,P)  (((E)->flags&(P))!=0)
#define ExprSetProperty(E,P)     (E)->flags|=(P)
#define ExprClearProperty(E,P)   (E)->flags&=~(P)

/*
** An instance of this structure holds information about the
** LIMIT clause of a SELECT statement.
*/
struct LimitVal {
	_ST_EXPR  *pLimit;    /* The LIMIT expression.  NULL if there is no limit */
	_ST_EXPR  *pOffset;   /* The OFFSET expression.  NULL if there is none */
};

/*
** A sort order can be either ASC or DESC.
*/
#define MDB_SO_ASC       0  /* Sort in ascending order */
#define MDB_SO_DESC      1  /* Sort in ascending order */

/************************************************************************/
/* �����������Լ�������Ϣ                                                                     */
/************************************************************************/
/*
typedef struct _ST_PARSER_RET
{
	int iRet;//������
	std::string sErrMsg;//������Ϣ
}ST_PARSER_RET;
*/



/*
** set ֵ�� set column = value,.....
*pIdList    => column
*pExprList => value
*/
struct _ST_SET_LIST
{
	ST_ID_LIST   * pIdList;//column
	ST_EXPR_LIST * pExprList;//value
};

//����ִ������
enum E_EXEC_FUNC_TYPE
{
	E_XFUNC  = 1,
	E_XSTEP  = 2,
	E_XFINAL = 3
};

//ST_MEM_VALUE���б�,
struct _ST_MEM_VALUE_LIST
{
	std::vector<ST_MEM_VALUE *> vMemValue; //��SQL����ʱʹ�ã�����push
	ST_MEM_VALUE ** pValueArray;		   //��ִ�е�ʱ��ʹ�ã��������
	int             iItemNum;
	_ST_MEM_VALUE_LIST()
	{
		pValueArray = NULL;
		iItemNum    = 0;
	}
	//����
	void clear()
	{
		vMemValue.clear();
		if(NULL != pValueArray)
		{
			delete [] pValueArray;
		}
		pValueArray = NULL;
		iItemNum    = 0;
	}
	void VectorToList()
	{
		int i = 0;

		if(NULL != pValueArray)
		{
			delete [] pValueArray;
		}
		pValueArray = NULL;
		
		iItemNum = static_cast<int>(vMemValue.size());
		if(iItemNum <= 0){return;}
		pValueArray = new ST_MEM_VALUE *[iItemNum];
		for(i = 0;i<(int) vMemValue.size();i++)
		{
			pValueArray[i] = vMemValue[static_cast<size_t>(i)];
		}
	}
};

//#define DDL_CMD_COUNT 9 //DDL �ؼ��ָ���,�¼ӹؼ��֣��������Ҫ�޸�
#define MAX_DDL_CMD_LEN 16 //DDL �����
#define DDL_KEYWORD_CREATE  "CREATE" //DDL�ؼ���
#define DDL_KEYWORD_DROP    "DROP"
#define DDL_KEYWORD_TRUNCATE "TRUNCATE"
#define DDL_KEYWORD_ALTER   "ALTER"
#define DDL_KEYWORD_USE     "USE"
#define DDL_KEYWORD_CONNECT "CONNECT"
#define DDL_KEYWORD_LOAD    "LOAD"
#define DDL_KEYWORD_ADD     "ADD"
#define DDL_KEYWORD_REMOVE  "REMOVE"
#define DDL_KEYWORD_RENAME  "RENAME"

//}

#endif
