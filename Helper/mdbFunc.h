/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbFunc.h		
*@Description�� mdb�������õ��ĺ�����
*@Author:		jin.shaohua
*@Date��	    2012.06
*@History:
******************************************************************************************/
#ifndef _MDB_FUNC_H_
#define _MDB_FUNC_H_
#include "Helper/SqlParserStruct.h"
#include "Helper/SqlTypeCheck.h"
#include <vector>
//namespace QuickMDB{
	//intֵ��str��ӳ��
	struct ST_INT_STR_MAP
	{
		int iValue;
		char sValue[32];
	};


	class TMdbFuncBase;
	//mdb �����Ĺ�����
	class TMdbFuncFactory
	{
	public:
		static int GetFunction(ST_EXPR * pstExpr,ST_SQL_STRUCT * pstSqlStruct);//��ȡ����
	private:
		static TMdbFuncBase* GetFunctionByName(const char * sName);
	};
	//mdb��������
	class TMdbFuncBase
	{
	public:
		TMdbFuncBase();
		virtual ~TMdbFuncBase();
		virtual int InitFunction(ST_EXPR * pstExpr,ST_SQL_STRUCT * pstSqlStruct);
		virtual int ExecuteFunc() = 0;  //����ִ��
		virtual int ExecuteStep(){return ExecuteFunc();}  //����ִ��,���پۺϺ���
		int ExecuteFinalize();//���ִ��,���پۺϺ���
		int GetValueMapIndex(const ST_INT_STR_MAP *pstIntStrMap,int iMapSize,const char * sName);//��ȡӳ��ֵ
		int m_iFlags;           //��ʶ
	protected:
		virtual int CheckArgv() = 0;//У�麯������
	protected:
		char m_sName[MAX_NAME_LEN];//������
		int  m_iArgc;//�������� -1
		std::vector<ST_MEM_VALUE *> m_vArgv;//����
		ST_FUNC_CONTEXT m_stFuncContext;//��¼������������Ϣ
		ST_EXPR   * m_pstExpr;   //��¼�º����ڵ�
		TSqlTypeCheck m_tTypeCheck;
		char m_sDsn[MAX_NAME_LEN];//dsn��
	};

	//in ����
	class TMdbInFunc:public TMdbFuncBase
	{
	public:
		TMdbInFunc();
		~TMdbInFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//У�麯���Ĳ���
	};
	//count ����
	class TMdbCountFunc:public TMdbFuncBase
	{
	public:
		TMdbCountFunc();
		~TMdbCountFunc();
		int ExecuteFunc();
		int ExecuteStep();
	protected:
		int CheckArgv();//У�麯���Ĳ���

	};
	//sysdate����
	class TMdbSysdateFunc:public TMdbFuncBase
	{
	public:
		TMdbSysdateFunc();
		~TMdbSysdateFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//У�麯���Ĳ���

	};

	//Max����
	class TMdbMaxFunc:public TMdbFuncBase
	{
	public:
		TMdbMaxFunc();
		~TMdbMaxFunc();
		int ExecuteFunc();
		int ExecuteStep();
	protected:
		int CheckArgv();//У�麯���Ĳ���

	};

	//Min����
	class TMdbMinFunc:public TMdbFuncBase
	{
	public:
		TMdbMinFunc();
		~TMdbMinFunc();
		int ExecuteFunc();
		int ExecuteStep();
	protected:
		int CheckArgv();//У�麯���Ĳ���
	};

	//Min����
	class TMdbSumFunc:public TMdbFuncBase
	{
	public:
		TMdbSumFunc();
		~TMdbSumFunc();
		int ExecuteFunc();
		int ExecuteStep();
	protected:
		int CheckArgv();//У�麯���Ĳ���
	};




	//ʱ������
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

	//to_date����
	class TMdbToDateFunc:public TMdbFuncBase
	{
	public:
		TMdbToDateFunc();
		~TMdbToDateFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//У�麯������
		int m_iDateType;
	};


	//to_char����
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
		int CheckArgv();//У�麯������
		int m_iDateType;
	};



	//nvl����,
	/*
	�﷨
	NVL(eExpression1, eExpression2) ����
	eExpression1, eExpression2 ��� eExpression1 �ļ�����Ϊ null ֵ��
	�� NVL( ) ���� eExpression2��
	��� eExpression1 �ļ��������� null ֵ���򷵻� eExpression1��
	��� eExpression1 �� eExpression2 �Ľ����Ϊ null ֵ���� NVL( ) ���� .NULL.��

	*/
	class TMdbNVLFunc:public TMdbFuncBase
	{
	public:
		TMdbNVLFunc();
		~TMdbNVLFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//У�麯������
	};


	/*
	�﷨
	BlobToChar(Exp) ����
	��exp�е�blobתΪchar 

	*/
	class TMdbBlobToCharFunc:public TMdbFuncBase
	{
	public:
		TMdbBlobToCharFunc();
		~TMdbBlobToCharFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//У�麯������
	};

	/*
	�﷨
	add_seconds(date,Sec) ����
	��ʱ��(date)��������(sec),sec����Ϊ����

	*/
	class TMdbAddSecondsFunc:public TMdbFuncBase
	{
	public:
		TMdbAddSecondsFunc();
		~TMdbAddSecondsFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//У�麯������
	};

	class TMdbSequence;

	/*
	�﷨
	nextval(seq_name) ����
	��ȡ��һ��sequence
	*/
	class TMdbNextvalFunc:public TMdbFuncBase
	{
	public:
		TMdbNextvalFunc();
		~TMdbNextvalFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//У�麯������
		TMdbSequence * m_pSeq;//mdb����
	};

	/*
	�﷨
	currval(seq_name) ����
	��ȡ��һ��sequence
	*/
	class TMdbCurrvalFunc:public TMdbFuncBase
	{
	public:
		TMdbCurrvalFunc();
		~TMdbCurrvalFunc();
		int ExecuteFunc();
	protected:
		int CheckArgv();//У�麯������
		TMdbSequence * m_pSeq;//mdb����
	};
//}



#endif

