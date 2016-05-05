/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbFunc.h		
*@Description： mdb解析所用到的函数类
*@Author:		jin.shaohua
*@Date：	    2012.06
*@History:
******************************************************************************************/
#ifndef _MDB_FUNC_H_
#define _MDB_FUNC_H_
#include "Helper/SqlParserStruct.h"
#include "Helper/SqlTypeCheck.h"
#include <vector>
//namespace QuickMDB{
	//int值和str的映射
	struct ST_INT_STR_MAP
	{
		int iValue;
		char sValue[32];
	};


	class TMdbFuncBase;
	//mdb 函数的工厂类
	class TMdbFuncFactory
	{
	public:
		static int GetFunction(ST_EXPR * pstExpr,ST_SQL_STRUCT * pstSqlStruct);//获取函数
	private:
		static TMdbFuncBase* GetFunctionByName(const char * sName);
	};
	//mdb函数基类
	class TMdbFuncBase
	{
	public:
		TMdbFuncBase();
		virtual ~TMdbFuncBase();
		virtual int InitFunction(ST_EXPR * pstExpr,ST_SQL_STRUCT * pstSqlStruct);
		virtual int ExecuteFunc() = 0;  //函数执行
		virtual int ExecuteStep(){return ExecuteFunc();}  //单步执行,用再聚合函数
		int ExecuteFinalize();//最后执行,用再聚合函数
		int GetValueMapIndex(const ST_INT_STR_MAP *pstIntStrMap,int iMapSize,const char * sName);//获取映射值
		int m_iFlags;           //标识
	protected:
		virtual int CheckArgv() = 0;//校验函数参数
	protected:
		char m_sName[MAX_NAME_LEN];//函数名
		int  m_iArgc;//参数个数 -1
		std::vector<ST_MEM_VALUE *> m_vArgv;//参数
		ST_FUNC_CONTEXT m_stFuncContext;//记录函数上下文信息
		ST_EXPR   * m_pstExpr;   //记录下函数节点
		TSqlTypeCheck m_tTypeCheck;
		char m_sDsn[MAX_NAME_LEN];//dsn名
	};

	//in 函数
	class TMdbInFunc:public TMdbFuncBase
	{
	public:
		TMdbInFunc();
		~TMdbInFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//校验函数的参数
	};
	//count 函数
	class TMdbCountFunc:public TMdbFuncBase
	{
	public:
		TMdbCountFunc();
		~TMdbCountFunc();
		int ExecuteFunc();
		int ExecuteStep();
	protected:
		int CheckArgv();//校验函数的参数

	};
	//sysdate函数
	class TMdbSysdateFunc:public TMdbFuncBase
	{
	public:
		TMdbSysdateFunc();
		~TMdbSysdateFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//校验函数的参数

	};

	//Max函数
	class TMdbMaxFunc:public TMdbFuncBase
	{
	public:
		TMdbMaxFunc();
		~TMdbMaxFunc();
		int ExecuteFunc();
		int ExecuteStep();
	protected:
		int CheckArgv();//校验函数的参数

	};

	//Min函数
	class TMdbMinFunc:public TMdbFuncBase
	{
	public:
		TMdbMinFunc();
		~TMdbMinFunc();
		int ExecuteFunc();
		int ExecuteStep();
	protected:
		int CheckArgv();//校验函数的参数
	};

	//Min函数
	class TMdbSumFunc:public TMdbFuncBase
	{
	public:
		TMdbSumFunc();
		~TMdbSumFunc();
		int ExecuteFunc();
		int ExecuteStep();
	protected:
		int CheckArgv();//校验函数的参数
	};




	//时间类型
	enum E_DATE_TYPE
	{
		E_LONG_YEAR      = 1, //YYYY-MM-DD
		E_LONG_YEAR_TIME = 2, //YYYY-MM-DD hh24:mi:ss
		E_SHORT_YEAR     = 3, //YYYYMMDD
		E_SHORT_YEAR_TIME= 4  //YYYYMMDDhh24miss
	};


	const static ST_INT_STR_MAP gDateTypeMap[] = {
		{E_LONG_YEAR,"YYYY-MM-DD"},{E_LONG_YEAR_TIME,"YYYY-MM-DD hh24:mi:ss"},
		{E_SHORT_YEAR,"YYYYMMDD"},{E_SHORT_YEAR_TIME,"YYYYMMDDhh24miss"}
	};

	//to_date函数
	class TMdbToDateFunc:public TMdbFuncBase
	{
	public:
		TMdbToDateFunc();
		~TMdbToDateFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//校验函数参数
		int m_iDateType;
	};


	//to_char函数
	class TMdbToCharFunc:public TMdbFuncBase
	{
	public:
		TMdbToCharFunc();
		~TMdbToCharFunc();
		int ExecuteFunc();
	private:
	    
	    int ExecuteInToChar();
	    int ExecuteDateToChar();
	    
	    int CheckInToCharArgv(const ST_EXPR_LIST * pFuncArgs);
	    int CheckDateToCharArgv(const ST_EXPR_LIST * pFuncArgs);
	protected:
		int CheckArgv();//校验函数参数
		int m_iDateType;
	};



	//nvl函数,
	/*
	语法
	NVL(eExpression1, eExpression2) 参数
	eExpression1, eExpression2 如果 eExpression1 的计算结果为 null 值，
	则 NVL( ) 返回 eExpression2。
	如果 eExpression1 的计算结果不是 null 值，则返回 eExpression1。
	如果 eExpression1 与 eExpression2 的结果皆为 null 值，则 NVL( ) 返回 .NULL.。

	*/
	class TMdbNVLFunc:public TMdbFuncBase
	{
	public:
		TMdbNVLFunc();
		~TMdbNVLFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//校验函数参数
	};


	/*
	语法
	BlobToChar(Exp) 参数
	将exp中的blob转为char 

	*/
	class TMdbBlobToCharFunc:public TMdbFuncBase
	{
	public:
		TMdbBlobToCharFunc();
		~TMdbBlobToCharFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//校验函数参数
	};

	/*
	语法
	add_seconds(date,Sec) 参数
	将时间(date)加上秒数(sec),sec可以为负的

	*/
	class TMdbAddSecondsFunc:public TMdbFuncBase
	{
	public:
		TMdbAddSecondsFunc();
		~TMdbAddSecondsFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//校验函数参数
	};

	class TMdbSequence;

	/*
	语法
	nextval(seq_name) 参数
	获取下一个sequence
	*/
	class TMdbNextvalFunc:public TMdbFuncBase
	{
	public:
		TMdbNextvalFunc();
		~TMdbNextvalFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//校验函数参数
		TMdbSequence * m_pSeq;//mdb序列
	};

	/*
	语法
	currval(seq_name) 参数
	获取下一个sequence
	*/
	class TMdbCurrvalFunc:public TMdbFuncBase
	{
	public:
		TMdbCurrvalFunc();
		~TMdbCurrvalFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//校验函数参数
		TMdbSequence * m_pSeq;//mdb序列
	};
//}



#endif

