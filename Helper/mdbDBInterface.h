/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbDBInterface.cpp		
*@Description： 提供DB访问接口基类
*@Author:		li.shugang
*@Date：	    2009年08月13日
*@History:
******************************************************************************************/

#ifndef __QUICK_MEMORY_DATABASE_INTERFACE_H__
#define __QUICK_MEMORY_DATABASE_INTERFACE_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "Helper/TBaseException.h"
 
#ifdef WIN32
	#pragma warning(disable: 4290)
	#pragma warning(disable: 4267)
#endif

//#ifndef __DB_DEF_
//	#define __DB_DEF_
	
	const   size_t MDB_PREFETCH_ROWS = 200;                      				//预先提取n行数据到缓冲区,减少网络流量
	const   size_t MDB_MAX_STRING_VALUE_LENGTH = 255;            				//返回的字符串最大的列、返回过程参数长度
	const   size_t MDB_MAX_LOB_BUFFER_LENGTH = 1024;             				//LOB数据缓冲区的最大空间
	const   size_t MDB_MAX_PARAMS_COUNT = 1000;                   				//参数最大数目
	const   size_t MDB_LOB_FLUSH_BUFFER_SIZE = 400*1024;         				//LOB数据积累到此量时，写入数据库
	
	//异常使用
	const   size_t MDB_MAX_ERRMSG_LENGTH = 4096;                 				//错误信息的最大长度
	const   size_t MDB_MAX_SQLSTMT_LENGTH = 4096;                				//出现错误的SQL语句长度
	
	//error message definination:
	const   char* const MDB_ERR_GENERAL = "General Error: %s"; 			//throw TMDBDBExcpInterface("TDBQueryInterface(TDBInterface &db)", ERR_GENERAL, "Can not declare a TDBQueryInterface when the database is not connected");
	const   char* const MDB_ERR_INDEX_OUT_OF_BOUND = "%s";    				//throw TMDBDBExcpInterface(fSqlStmt , ERR_INDEX_OUT_OF_BOUND, "field index out of bound when call Field(i)");   
	const   char* const MDB_ERR_DB_INIT = "OCI: OCI handles init fail in TDatabase constructor: @line:%d";
	const   char* const MDB_ERR_SET_LOGIN = "OCI: You can only set login infomation on disconnect status: line %d";
	const   char* const MDB_ERR_CONNECT_NO_LOGIN_INFO = "No login information provided before Connect(), call SetLogin first, line:%d";
	const   char* const MDB_ERR_NO_DATASET = "OCI: function:%s , Result Dataset is on Bof/Eof. field:%s"; //throw TMDBDBExcpInterface(fParentQuery->fSqlStmt, ERR_NO_DATASET, "asBlobBuffer()", name);
	const   char* const MDB_ERR_DATA_TYPE_CONVERT = "Data type convertion error: field:%s data type:%d can not be access by %s"; //throw TMDBDBExcpInterface(fParentQuery->fSqlStmt, ERR_DATA_TYPE_CONVERT, name, type, "asLobBuffer()");
	const   char* const MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED = "no more memory can be allocate when :%s, source code:%d"; //throw TMDBDBExcpInterface(fParentQuery->fSqlStmt, ERR_NOMORE_MEMORY_BE_ALLOCATED, "asBlobBuffer()", __LINE__);
	const   char* const MDB_ERR_FILE_IO = "%s: can not open file:%s"; //throw TMDBDBExcpInterface(fParentQuery->fSqlStmt, ERR_FILE_IO, "LoadFromFile()", fileName);
	const   char* const MDB_ERR_MEM_BUFFER_IO = "asBlobWriter() error: read from file to buffer, field:%s, file:%s, @line:%d"; //throw TMDBDBExcpInterface(fParentQuery->fSqlStmt, ERR_MEM_BUFFER_IO, name, fileName, __LINE__);
	const   char* const MDB_ERR_DATA_TYPE_NOT_SUPPORT = "field:%s, datatype:%d not yet supported"; //, pCurrField->name,innerDataType);
	const   char* const MDB_ERR_PARAM_NOT_EXISTS = "param:%s does not exists."; //throw TMDBDBExcpInterface(fSqlStmt, ERR_PARAM_NO_EXISTS, paramName, "check spelling error");
	const   char* const MDB_ERR_FIELD_NOT_EXISTS = "field:%s does not exists.";
	const   char* const MDB_ERR_INVALID_METHOD_CALL = "%s: invalid call method:%s";
	const   char* const MDB_ERR_CAPABILITY_NOT_YET_SUPPORT = "capability not support yet:%s"; //例如参数个数超越范围
	const   char* const MDB_ERR_READ_PARAM_DATA = "read parameter value data type error, parameter name:%s, method:%s";
	
	//const define:
	const char* const MDB_NULL_STRING = "";
	const int MDB_NULL_NUMBER = 0;
	
	#define MDB_SIZE_LONG_LONG 64																																																						//LONG LONG最大长度
//#endif



/* classes defined in this file: */
class TMDBDBQueryInterface;
class TMDBDBExcpInterface;
class TMDBDBInterface;
class TMDBDBFieldInterface;
//class TDBSessionInterface;

class TMDBDBExcpInterface: public TBaseException
{
public:
    TMDBDBExcpInterface() {}
    virtual ~TMDBDBExcpInterface() {}
    virtual char *GetErrMsg() const {return((char*)m_sErrMsg);}
    virtual char *GetErrSql() const {return((char*)m_sErrSql);}
    virtual int   GetErrCode() const {return m_lErrCode;}
protected:    
    char m_sErrMsg[MDB_MAX_ERRMSG_LENGTH+1];        			//错误信息
    char m_sErrSql[MDB_MAX_SQLSTMT_LENGTH+1];           	//发生错误的sql语句
    int m_lErrCode;                                   //错误号
};

class TMDBDBInterface
{
public:
    TMDBDBInterface() throw (TMDBDBExcpInterface) {};
    virtual ~TMDBDBInterface(){};
    virtual void SetLogin(const char *user, const char *password, const char *tnsString) throw (TMDBDBExcpInterface)=0;
    virtual bool Connect(bool bIsAutoCommit=false) throw (TMDBDBExcpInterface)=0;
    virtual void CheckError(const char* sSql=NULL) throw (TMDBDBExcpInterface)=0;
    virtual bool Connect(const char *usr, const char *pwd, const char *tns,bool bIsAutoCommit=false) throw (TMDBDBExcpInterface)=0;
    virtual int  Disconnect() throw (TMDBDBExcpInterface)=0;
    virtual void Commit()=0;    
    virtual void Rollback()=0;
    virtual bool IsConnect()=0;																		//测试数据库是否连接正常
    virtual const char* GetProvider()=0;
    virtual TMDBDBQueryInterface *CreateDBQuery() throw (TMDBDBExcpInterface)=0;
    bool fConnected;       //在Connect中是否连接成功
};

class TMDBDBQueryInterface
{
public:
    TMDBDBQueryInterface() {};
    virtual ~TMDBDBQueryInterface() {}
    //主要功能
    virtual void Close()=0;                                                   				//关闭SQL语句，以准备接收下一个sql语句
    virtual void CloseSQL()=0;    
    virtual void SetSQL(const char *inSqlstmt,int iPreFetchRows=0) throw (TMDBDBExcpInterface)=0;        //设置Sqlstatement
    virtual void Open(int prefetchRows=MDB_PREFETCH_ROWS) throw (TMDBDBExcpInterface)=0; //打开SQL SELECT语句返回结果集
    virtual bool Next() throw (TMDBDBExcpInterface)=0;                               //移动到下一个记录
    virtual bool Execute(int iters=1) throw (TMDBDBExcpInterface)=0;                 //执行非SELECT语句,没有返回结果集
    virtual bool Commit()=0;                                                  				//事务提交
    virtual bool Rollback()=0;                                                				//事务回滚
    virtual int  RowsAffected()=0;               																				//DELETE/UPDATE/INSERT语句修改的记录数目
    virtual bool IsFieldExist(const char *fieldName)=0;                                     //查看参数是否存在   
    //暂不开放//virtual int  GetSQLCode()=0;                          											//返回Oracle执行结果代码
    virtual void CheckError(const char* sSql=NULL) throw (TMDBDBExcpInterface)=0;  
    

    //与列信息相关              
    virtual int FieldCount()=0;                                                         //总共有几个列
    virtual TMDBDBFieldInterface& Field(int index)  throw (TMDBDBExcpInterface)=0;            //返回第i个列信息
    virtual TMDBDBFieldInterface& Field(const char *fieldName) throw (TMDBDBExcpInterface)=0; //根据列名(不分大小写)返回列信息; 建议使用Field(int i)获得更高的效率
    
    
    //以下是设置参数值
    virtual bool IsParamExist(const char *paramName)=0;
    virtual void SetParameter(const char *paramName, const char* paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0;   
    virtual void SetParameter(const char *paramName, const char paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0;    //add by fu.wenjun@20041125
    
    virtual void SetParameter(const char *paramName, int paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0; 
    virtual void SetParameter(const char *paramName, double paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    virtual void SetParameter(const char *paramName, long paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    virtual void SetParameter(const char *paramName, long long paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    
    
    virtual void SetParameter(const char *paramName,const char* paramValue,int iBufferLen,bool isOutput = false) throw (TMDBDBExcpInterface)=0;//用于传入BLOB/BINARY类型字段
    virtual void SetParameterNULL(const char *paramName) throw (TMDBDBExcpInterface)=0;

    /*备注:增加int iArraySize=0为了兼容Informix部分的同名函数,OCI部分不进行任何处理*/
    //数组操作
    virtual void SetParamArray(const char *paramName, char     ** paramValue, int iStructSize, int iStrSize ,int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMDBDBExcpInterface)=0;  
    virtual void SetParamArray(const char *paramName, int       * paramValue, int iStructSize,               int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMDBDBExcpInterface)=0; 
    virtual void SetParamArray(const char *paramName, double    * paramValue, int iStructSize,               int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    virtual void SetParamArray(const char *paramName, long      * paramValue, int iStructSize,               int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    virtual void SetParamArray(const char *paramName, long long * paramValue, int iStructSize,               int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    virtual void SetBlobParamArray(const char *paramName,char *paramValue,int iBufferLen,int iArraySize=0,short* iIsNull=NULL,bool isOutput=false) throw (TMDBDBExcpInterface)=0;//用于传入BLOB/BINARY类型字段数组参数
};


class TMDBDBFieldInterface
{
public: 
    virtual ~TMDBDBFieldInterface() {} 
    virtual bool    isNULL()=0;           										//在fetch过程中该列的数据是否为空
    virtual char   *AsString() throw(TMDBDBExcpInterface)=0;
    virtual double  AsFloat() throw (TMDBDBExcpInterface)=0;
    virtual long long   AsInteger() throw (TMDBDBExcpInterface)=0;
	virtual char* AsBlobBuffer(int &iBufferLen)throw (TMDBDBExcpInterface)=0;          //输出BLOB字段   
    virtual long GetFieldType()=0;
    virtual char *GetFieldName()=0;
    //日期处理
    virtual char*		AsDateTimeString() throw (TMDBDBExcpInterface)=0;   			//把日期型的列以HH:MM:DD HH24:MI格式读取,使用asString()读取的日期型数据类型为HH:MM:DD
    virtual void   AsDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TMDBDBExcpInterface)=0; //返回日期的各个部分
    //暂不开放//virtual time_t AsTimeT() throw (TOraDBExcpInterface)=0;
    virtual void   ClearDataBuf()=0;																	  	//游标循环获取指定数量数据后,未清空上次的缓存
};


#endif  //__QUICK_MEMORY_DATABASE_INTERFACE_H__

