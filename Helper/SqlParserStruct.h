/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    SqlParserStruct.h		
*@Description： SQL解析器所使用的一些结构体定义
*@Author:			jin.shaohua
*@Date：	    2012.07
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

class TMdbFuncBase;//函数指针
#define MAX_ORDERBY_COUNT 10  //orderby最多元素个数
// orderby 
struct _ST_ORDERBY_INFO
{
	int iNum;//orderby 的元素个数
	int iTypeArray[MAX_ORDERBY_COUNT];//类型队列
	int iSizeArray[MAX_ORDERBY_COUNT];//大小队列
	int iOffsetArray[MAX_ORDERBY_COUNT];//数据偏移
	int iSortType[MAX_ORDERBY_COUNT];//排序类型
	int iTotalSize;//总大小
	void clear()
	{
		memset(this,0x00,sizeof(_ST_ORDERBY_INFO));
	}
};
/**
* where 条件中的and段
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
* where 条件表示的or段
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
    std::vector<ST_WHERE_AND_CLAUSE> m_vAndClause;//当op=TK_OR时，
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
* 表达式结构信息
*/
struct _ST_EXPR
{
	int iOpcode;//操作码
	int flags;	 //标识expr的一些属性
	char * sTorken;//torken串
	_ST_EXPR * pLeft;//左子树
	_ST_EXPR * pRight; //右子树
	_ST_EXPR_FUNC * pFunc;//函数信息指针
	_ST_MEM_VALUE * pExprValue;//该表达式的值
	int iCalcFlags;//计算标识:是否需要计算，计算周期 
	bool bNeedToCalc()/*是否需要计算*/
       {
            return false == ExprCalcHasProperty(this,CALC_NO_NEED);
       }
	_ST_EXPR()
	{
		memset(this,0x00,sizeof(_ST_EXPR));
	}
};



/**
* Expr项
* pExpr/name组合可以作为查询中的 pExpr as name 或者作为update的 name=:pExpr
*/
struct _ST_EXPR_ITEM
{
	_ST_EXPR * pExpr; //指向expr 
	char * sName;     //expr的名字 
	char * sSpan;	 //expr的原始字符串
	int iSortOrder;   //排列方式
	_ST_EXPR_ITEM()
	{
		memset(this,0x00,sizeof(_ST_EXPR_ITEM));
	}
};

/**
*Expr链表
*/
struct _ST_EXPR_LIST
{
	int iItemNum;//item的个数
	int iAllocNum;//已分配的个数
	_ST_EXPR_ITEM *pExprItems;//指向expr_item数组
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
#define MEM_Const     0x0020   /* Value is a Const */ //常量值，不可被修改

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


//设置memvalue的type和size//分配string 的内存空间
#define SET_MEM_TYPE_SIZE(M,T,S) MemValueSetProperty(M,(T));M->iSize = S;\
if(MemValueHasAnyProperty(M,MEM_Str|MEM_Date|MEM_Blob))\
{\
       if(NULL != M->sValue){delete [] (M->sValue);M->sValue=NULL;}\
	M->sValue = new char[S];\
	memset(M->sValue,0x00,(size_t)S);\
	MemValueSetProperty(M,MEM_Dyn);\
}

/*
* 值结构。用来表示参数值
*/
struct _ST_MEM_VALUE
{
	long long  lValue; //long long
	double     dValue;//double
	//bool bIsSeqValue; //判断该值是否为序列值
	//long long lSeqValue;//用于保存序列的nextval or currval值
	int iSize;		  //string size;
	int iFlags;  /* Some combination of MEM_Null, MEM_Str, MEM_Dyn, etc. */
	TMdbColumn * pColumn;//TK_ID所对应pColumn信息
	TMdbColumn * pColumnToSet;//update 中col= value,col 所对应的pColumn信息
	int iColIndexToSet;//update 中col= value,col 所对应的pColumn 所对应的index
	char * sValue;    //string
	char * sAlias;
       int iAggCalcCounts;//聚合函数计算次数
	_ST_MEM_VALUE()
	{
		memset(this,0x00,sizeof(_ST_MEM_VALUE));
	}
	//设置为常量值属性
	void SetConst()
	{
		MemValueSetProperty(this,MEM_Const);
	}
       //判断NULL值
       bool IsNull()
       {
            return MemValueHasAnyProperty(this,MEM_Null);
       }

		// == 使用最长前缀匹配方法
	   bool IsLPM()
       {
            return MemValueHasAnyProperty(this,MEM_LPM) && MemValueHasAnyProperty(this,MEM_Str);
       }
		
        //清理值
        void ClearValue()
        {
            if(MemValueHasProperty(this,MEM_Const)){return;} //常量值不清空

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
       //设置null
       void SetNull()
       {
            MemValueSetProperty(this,MEM_Null);
       }
       //拷贝数据
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
       //复制一份
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
       //输出
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
* 函数项
*/
struct _ST_EXPR_FUNC
{
	TMdbFuncBase * pFuncDef;//函数定义
	int iType;		//函数类型，是否是聚合函数
	_ST_EXPR_LIST * pFuncArgs;//函数的参数列表，或者作为IN ,BETWEEN的参数
	_ST_EXPR_FUNC()
	{
		memset(this,0x00,sizeof(_ST_EXPR_FUNC));
	}
};

#if 0

//函数定义信息
struct _ST_FUNC_DEFINE
{
	char * sName;//函数名
	int nArgs;   //函数的参数个数
	ST_MEM_VALUE ** pArgv;
	//void * pUserData;//用户数据
	int iFlags; 
	_ST_FUNC_DEFINE *pNext; /* 指向下一个同名函数 */
	void (*xFunc)(ST_FUNC_CONTEXT*,int,ST_MEM_VALUE**); /* 常规函数定义 */
	void (*xStep)(ST_FUNC_CONTEXT*,int,ST_MEM_VALUE**); /* 聚合函数step函数 */
    void (*xFinalize)(ST_FUNC_CONTEXT*);                /* 聚合函数的final函数 */
	_ST_FUNC_DEFINE * pHash;  /* 指向下一个同hash的不同名的函数 */
	int iTempArgs;//记录本次运算中参数个数
	ST_FUNC_CONTEXT tTempContext;//记录本次计算的上下文
	void (*xCheckArgs)();//校验参数的函数
	
};
#endif

//函数上下文信息
struct _ST_FUNC_CONTEXT
{
	_ST_MEM_VALUE  *ReturnValue;//保存返回值
	_ST_MEM_VALUE * pMem;     //保存其他数据
	int iRetCode;			  //返回码
};

#define FUNC_AGGREAGATE  0x0001 /* function is aggreate*/

#define FuncHasProperty(V,P)     (((V)->m_iFlags&(P))==(P))
#define FuncHasAnyProperty(V,P)  (((V)->m_iFlags&(P))!=0)
#define FuncSetProperty(V,P)     (V)->m_iFlags|=(P)
#define FuncClearProperty(V,P)   (V)V->m_iFlags&=~(P)

#if 0
/*
** 保存函数定义的hash表
*/
struct _ST_FUNC_DEF_HASH {
	_ST_FUNC_DEFINE *a[23];       /* 函数哈希表 */
};
#endif



struct ST_TABLE_INDEX_INFO;

/*
** 索引和列
*/
struct _ST_INDEX_COLUMN
{
    ST_TABLE_INDEX_INFO * pstTableIndex;
    TMdbColumn                 * pColumn;  //列，可能是索引的一部分
    std::vector<ST_EXPR *>  vExpr;//值表达式,可能有多个
};

/*
** 索引和值
*/
struct _ST_INDEX_VALUE
{
	//TMdbIndexNode * pIndexNode;
	ST_TABLE_INDEX_INFO * pstTableIndex;
	ST_EXPR       * pExprArr[MAX_INDEX_COLUMN_COUNTS];//索引值,可能是复合索引
	_ST_INDEX_VALUE()
	{
		int i = 0;
		for(i= 0;i<MAX_INDEX_COLUMN_COUNTS;i++)
		{
			pExprArr[i] = NULL;
		}
		pstTableIndex = NULL;
	}
    _ST_INDEX_VALUE& operator=(const _ST_INDEX_VALUE& s)//重载运算符
     {
        this->pstTableIndex = s.pstTableIndex;
        int i = 0;
	for(i= 0;i<MAX_INDEX_COLUMN_COUNTS;i++)
	{
		this->pExprArr[i] = s.pExprArr[i];
	}
        return *this;
     }
    _ST_INDEX_VALUE(const _ST_INDEX_VALUE& s)//复制构造函数
    {
            *this = s;
    }
   
};

/**
* select a,b,c from table_x where a=:a and b>:b
      \___/                   \__________/
      PColist                 pWhere

*解析出的sql结构
*/
class TMdbShmDSN;
struct _ST_SQL_STRUCT
{
	int iSqlType;        //sql类型,TK_SELECT,TK_INSERT,TK_UPDATE,TK_DELETE
	char * sTableName;    //表名
	char * sAlias;		//表的别名
	TMdbTable * pMdbTable;
	bool bIsSimpleCol;
	bool bIsSingleGroupFunc;
	ST_TABLE_INDEX_INFO * pScanAllIndex;//全量遍历的索引
	TMdbShmDSN * pShmDSN;//内存DSN信息
	_ST_EXPR_LIST * pHintList;
	_ST_EXPR_LIST * pColList; //查询结果列，或者insert,update的值列
	_ST_EXPR      * pWhere;//where子句
	_ST_EXPR_LIST * pGroupby;//group by 子句
	_ST_EXPR      * pHaving;//having子句
	_ST_EXPR_LIST * pOrderby;//orderby子句
	_ST_EXPR * pstLimit; //limit子句
	_ST_EXPR * pstOffset;//offset 子句
	_ST_ID_LIST * pstIdList;//用于insert ,update 中的列名
	std::vector< _ST_INDEX_VALUE> vIndexUsed;//where 条件所使用索引
	std::vector< _ST_INDEX_VALUE> vIndexChanged;// update set ,insert所造成的索引变更
	bool    bCheckInsertPriKey;//插入时的是否进行主键校验
	int iSelFlags;//select的flags
	ST_ORDERBY_INFO stOrderbyInfo;//orderby元素的一些信息
	long long iRowPos;//第几条记录
	char sSQL[MAX_SQL_LEN];//sql语句
	_ST_INDEX_VALUE stIndexForVerifyPK;//用于主键校验的索引
	_ST_EXPR * pstRouterID;//指向路由ID的列
	ST_WHERE_OR_CLAUSE tWhereOrClause;//整理后的where条件
	std::vector<_ST_EXPR *> vExprCalcPerExec;  //每次执行执行一次的expr
       std::vector<_ST_EXPR *> vExprCalcPerQuery; //每个query执行一次的expr
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
*DDL SQL结构体
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
	TMemSeq tMemSeq; //增加和删除序列使用
	TMdbJob *pMdbJob; //增加修改删除job时使用
	TMdbParameter tParam;//用户flush-sql or load-sql使用
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
*用来记录一些简单信息节点。insert中的列标识
*/
struct _ID_LIST_ITEM {
	char *zName;      /* Name of the identifier */
//	int idx;          /* Index in some Table.aCol[] of a column named zName */
	TMdbColumn * pColumn;//列指针
	ST_EXPR * pExpr;//ID所对应的值一般用在insert ,update set中。
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
//IN关键字的token
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
/* 解析返回码以及返回信息                                                                     */
/************************************************************************/
/*
typedef struct _ST_PARSER_RET
{
	int iRet;//返回码
	std::string sErrMsg;//错误信息
}ST_PARSER_RET;
*/



/*
** set 值对 set column = value,.....
*pIdList    => column
*pExprList => value
*/
struct _ST_SET_LIST
{
	ST_ID_LIST   * pIdList;//column
	ST_EXPR_LIST * pExprList;//value
};

//函数执行类型
enum E_EXEC_FUNC_TYPE
{
	E_XFUNC  = 1,
	E_XSTEP  = 2,
	E_XFINAL = 3
};

//ST_MEM_VALUE的列表,
struct _ST_MEM_VALUE_LIST
{
	std::vector<ST_MEM_VALUE *> vMemValue; //在SQL解析时使用，方便push
	ST_MEM_VALUE ** pValueArray;		   //在执行的时候使用，提高性能
	int             iItemNum;
	_ST_MEM_VALUE_LIST()
	{
		pValueArray = NULL;
		iItemNum    = 0;
	}
	//清理
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

//#define DDL_CMD_COUNT 9 //DDL 关键字个数,新加关键字，这个数字要修改
#define MAX_DDL_CMD_LEN 16 //DDL 命令最长
#define DDL_KEYWORD_CREATE  "CREATE" //DDL关键字
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
