#ifndef __MYSQL_QUERY_H__
#define __MYSQL_QUERY_H__

#ifdef DB_MYSQL

#include <string.h>
#include <my_global.h>
#include <mysql.h>
#include "Helper/mdbDBInterface.h"
#include "Common/mdbCommons.h"
#include "Helper/mdbDateTime.h"
//#include "BillingSDK.h"


//using namespace ZSmart::BillingSDK;


#ifdef WIN32
	#pragma warning(disable: 4290)
	#pragma warning(disable: 4267)
#endif

//namespace QuickMDB{

#ifndef _MYSQL_CONN_STRING_
	#define _MYSQL_CONN_STRING_              "MYSQL"
#endif

#define  MAX_DATA_COUNTS      512

#define  MAX_QUERY_PER_DB   30000  


/* classes defined in this file: */
class TMsqDBException;
class TMsqDBParam;
class TMsqDBField;
class TMsqDBQuery;
class TMsqDBDatabase;

//oracle操作异常处理类
class TMsqDBException: public TMDBDBExcpInterface
{
public:    
    TMsqDBException(int errNumb, const char *errMsg, const char *sql); 
    TMsqDBException(const char *sql, const char* errFormat, ...);
    virtual ~TMsqDBException();
};

class TMsqDBParam  
{
    friend class TMsqDBQuery;
public:
    TMsqDBParam();
    virtual ~TMsqDBParam();
	/******************************************************************************
	* 函数名称	:  AsInteger()
	* 函数描述	:  读取返回整型参数值  
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  返回参数值
	* 作者		:  li.shugang
	*******************************************************************************/
    int     AsInteger()  throw (TMsqDBException);
    /******************************************************************************
	* 函数名称	:  AsFloat()
	* 函数描述	:  读取返回浮点型参数值   
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  返回参数值
	* 作者		:  li.shugang
	*******************************************************************************/
    double  AsFloat()    throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  AsLong()
	* 函数描述	:  读取返回长整型参数值    
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  返回参数值
	* 作者		:  li.shugang
	*******************************************************************************/
    long    AsLong()     throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  AsString()
	* 函数描述	:  读取返回字符参数值    
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  返回参数值
	* 作者		:  li.shugang
	*******************************************************************************/
    char*   AsString()   throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  isNULL()
	* 函数描述	:  返回值是否为空   
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  true 是，false 不为空
	* 作者		:  li.shugang
	*******************************************************************************/
    bool    isNULL()     throw (TMsqDBException);
private:
    char    *name;
    enum_field_types     dataType;              //数据类型
    int     intValue;
    double  dblValue;         					//存储输入和输出的值(缓冲区)
    long    longValue;        				    //llong数据缓冲区
    char   * stringValue;     					//字符串返回缓冲区    

    unsigned long string_length;         		//string的MAX_LENGTH
    unsigned long length;                       //string的实际大小
    my_bool  is_null[MAX_DATA_COUNTS];          					//在返回值时候是否为空  
    
    int    * intArray;        					//INT数组
    double * dblArray;        					//DOUBLE数组
    long  * longArray;       					//LONG数组
    MDB_INT64 * llongArray;   						//LONG LONG数组
    char * stringArray[MAX_DATA_COUNTS];		//STRING数组
    
    bool  fIsOutput;          					//是否是输出参数.默认是输入参数
      
};

class TMsqDBField : public TMDBDBFieldInterface
{
    friend class TMsqDBQuery;
public: 
    virtual ~TMsqDBField(); 
	/******************************************************************************
	* 函数名称	:  isNULL()
	* 函数描述	:  在fetch过程中该列的数据是否为空  
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  true 是，false 不为空
	* 作者		:  li.shugang
	*******************************************************************************/
    bool    isNULL();           																
	/******************************************************************************
	* 函数名称	:  AsString()
	* 函数描述	:  将字段其他类型的数据转换成字符串  
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  转换后的字符串
	* 作者		:  li.shugang
	*******************************************************************************/
    char   *AsString() throw(TMsqDBException);
	/******************************************************************************
	* 函数名称	:  AsFloat()
	* 函数描述	:  浮点类型数据输出 
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  浮点数据
	* 作者		:  li.shugang
	*******************************************************************************/
    double  AsFloat() throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  isNULL()
	* 函数描述	:  整型类型数据输出 
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  整形数据
	* 作者		:  li.shugang
	*******************************************************************************/
    MDB_INT64   AsInteger() throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  AsBlobFile()
	* 函数描述	:  Blob类型处理，读取到file中
	* 输入		:  fileName 保存Blob数据
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
    void        AsBlobFile(const char *fileName) throw (TMsqDBException); 
	/******************************************************************************
	* 函数名称	:  AsBlobBuffer()
	* 函数描述	:  将Blob数据保存到缓冲区,缓冲区的大小自动创建，并返回缓冲区大小*bufLength.
	* 输入		:  无
	* 输出		:  buf BLOB字段值，bufLength BLOB字段值长度
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
    void        AsBlobBuffer(unsigned char* &buf, unsigned int *bufLength) throw (TMsqDBException); 
    /******************************************************************************
	* 函数名称	:  AsBlobBuffer()
	* 函数描述	:  返回值是否为空   
	* 输入		:  无
	* 输出		:  iBufferLen BLOB字段值长度
	* 返回值	:  BLOB字段值
	* 作者		:  li.shugang
	*******************************************************************************/
    char*       AsBlobBuffer(int &iBufferLen) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  AsDateTimeString()
	* 函数描述	:  把日期型的列以HH:MM:DD HH24:MI格式读取,使用asString()读取的日期型数据类型为HH:MM:DD
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  转换后的时间字符串,格式为YYYYMMDDHH24MMSS
	* 作者		:  li.shugang
	*******************************************************************************/
	char       *AsDateTimeString() throw (TMsqDBException); 
	/******************************************************************************
	* 函数名称	:  AsDateTime()
	* 函数描述	:  返回日期字段的各个部分  
	* 输入		:  无
	* 输出		:  year 年，month 月，day 日，hour 小时，minute 分钟，second 秒
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void        AsDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TMsqDBException); 
	/******************************************************************************
	* 函数名称	:  AsTimeT()
	* 函数描述	:  返回time_t时间类型
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  转换后的时间
	* 作者		:  li.shugang
	*******************************************************************************/
	time_t      AsTimeT() throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  LoadFromFile()
	* 函数描述	:  写入到blob中 
	* 输入		:  fileName 保存blob数据文件
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void        LoadFromFile(const char *fileName) throw (TMsqDBException); 
	/******************************************************************************
	* 函数名称	:  LoadFromBuffer()
	* 函数描述	:  把BLOB的内容用缓冲区内容替代      
	* 输入		:  buf 保存BLOB数据的缓冲区，bufLength 缓冲区长度
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void        LoadFromBuffer(unsigned char *buf, unsigned int bufLength) throw (TMsqDBException);    
	/******************************************************************************
	* 函数名称	:  ClearDataBuf()
	* 函数描述	:  清空缓冲区
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void        ClearDataBuf();																																																		//游标循环获取指定数量数据后,未清空上次的缓存

public:
	/******************************************************************************
	* 函数名称	:  GetFieldName()
	* 函数描述	:  获取列名
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  列名
	* 作者		:  li.shugang
	*******************************************************************************/
    char *GetFieldName();    																			
	/******************************************************************************
	* 函数名称	:  GetFieldType()
	* 函数描述	:  获取列类型   
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  列名类型
	* 作者		:  li.shugang
	*******************************************************************************/
	long GetFieldType();     																		
	/******************************************************************************
	* 函数名称	:  GetFieldSize()
	* 函数描述	:  获取列大小
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  列大小
	* 作者		:  li.shugang
	*******************************************************************************/
	long GetFieldSize();     																			
	/******************************************************************************
	* 函数名称	:  GetFieldPrecision()
	* 函数描述	:  获取列精度   
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  列精度
	* 作者		:  li.shugang
	*******************************************************************************/
	int  GetFieldPrecision();     													
//private:
    /******************************************************************************
	* 函数名称	:  AsDateTimeInternal()
	* 函数描述	:  返回日期的各个部分,没有作其他校验，只是内部调用    
	* 输入		:  无
	* 输出		:  year 年，month 月，day 日，hour 小时，minute 分钟，second 秒
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
    void    AsDateTimeInternal(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TMsqDBException);                  

    TMsqDBField();
    MYSQL_FIELD * field;

    char    *name;              						    //字段名称
    char    *buffer;                                        //字段内容指针
    unsigned long length;                 					//数据实际长度
    unsigned long max_length;                 				//数据定义长度
    enum_field_types   buffer_type;               			//数据类型 in(INT_TYPE,FLOAT_TYPE,DATE_TYPE,STRING_TYPE,ROWID_TYPE)
    my_bool is_null;           					            //字段定义时是否允许为空值--为了和其他的相一致

    char fStrBuffer[MDB_MAX_STRING_VALUE_LENGTH];           //用于保存转换为字符串后的值
    TMsqDBQuery *fParentQuery;                 				//指向该Field所属于的Query   
};


class TMsqDBQuery: public TMDBDBQueryInterface
{
    friend class TMsqDBField;
public:
    /******************************************************************************
	* 函数名称	:  Close()
	* 函数描述	:  关闭SQL语句，以准备接收下一个sql语句   
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
    void Close(); 
	/******************************************************************************
	* 函数名称	:  CloseSQL()
	* 函数描述	:  兼容odbc
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  列精度
	* 作者		:  li.shugang
	*******************************************************************************/
	void CloseSQL() {}
	/******************************************************************************
	* 函数名称	:  SetSQL()
	* 函数描述	:  设置Sqlstatement   
	* 输入		:  inSqlstmt sql语句
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetSQL(const char *inSqlstmt,int iPreFetchRows=0) throw (TMsqDBException); 
	/******************************************************************************
	* 函数名称	:  Open()
	* 函数描述	:  打开SQL SELECT语句返回结果集 
	* 输入		:  prefetchRows 预先提前记录数，默认200条
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void Open(int prefetchRows=MDB_PREFETCH_ROWS) throw (TMsqDBException); 
	/******************************************************************************
	* 函数名称	:  Next()
	* 函数描述	:  移动到下一个记录 
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  true 成功;false 失败
	* 作者		:  li.shugang
	*******************************************************************************/
	bool Next() throw (TMsqDBException);   
	/******************************************************************************
	* 函数名称	:  Execute()
	* 函数描述	:  执行非SELECT语句,没有返回结果集
	* 输入		:  iters 指出sql语句的执行次数
	* 输出		:  无
	* 返回值	:  true 成功;false 失败
	* 作者		:  li.shugang
	*******************************************************************************/
	bool Execute(int iters=1) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  Commit()
	* 函数描述	:  事务提交
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  true 提交成功，false 提交失败
	* 作者		:  li.shugang
	*******************************************************************************/
	bool Commit();                                            
	/******************************************************************************
	* 函数名称	:  Rollback()
	* 函数描述	:  事务回滚
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  true 回滚成功，false 回滚失败
	* 作者		:  li.shugang
	*******************************************************************************/
	bool Rollback();                                                
	/******************************************************************************
	* 函数名称	:  RowsAffected()
	* 函数描述	:  DELETE/UPDATE/INSERT语句修改的记录数目
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  DELETE/UPDATE/INSERT语句修改的记录数目
	* 作者		:  li.shugang
	*******************************************************************************/
	int  RowsAffected() { return iAffectedRows;};              
	/******************************************************************************
	* 函数名称	:  GetSQLCode()
	* 函数描述	:  返回Oracle执行结果代码
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  Oracle执行结果代码
	* 作者		:  li.shugang
	*******************************************************************************/
	int  GetSQLCode() { return fErrorNo;};                        

    //与列信息相关              
	/******************************************************************************
	* 函数名称	:  FieldCount()
	* 函数描述	:  总共有几个列
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  列数
	* 作者		:  li.shugang
	*******************************************************************************/
	int FieldCount() ;       
	/******************************************************************************
	* 函数名称	:  GetFieldPrecision()
	* 函数描述	:  返回指定列信息  
	* 输入		:  index 列序列
	* 输出		:  无
	* 返回值	:  列对象
	* 作者		:  li.shugang
	*******************************************************************************/
	TMsqDBField& Field(int index)  throw (TMsqDBException)    ;           
	/******************************************************************************
	* 函数名称	:  Field()
	* 函数描述	:  根据列名(不分大小写)返回列信息; 建议使用Field(int i)获得更高的效率   
	* 输入		:  fieldName 列名
	* 输出		:  无
	* 返回值	:  列信息
	* 作者		:  li.shugang
	*******************************************************************************/
	TMsqDBField& Field(const char *fieldName) throw (TMsqDBException);    
	/******************************************************************************
	* 函数名称	:  IsFieldExist()
	* 函数描述	:  指定列是否存在   
	* 输入		:  fieldName 列名
	* 输出		:  无
	* 返回值	:  列精度
	* 作者		:  li.shugang
	*******************************************************************************/
	bool IsFieldExist(const char *fieldName);

    //与参数信息相关
	/******************************************************************************
	* 函数名称	:  ParamCount()
	* 函数描述	:  获取参数总数  
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  参数总数
	* 作者		:  li.shugang
	*******************************************************************************/
	int ParamCount() ;
	/******************************************************************************
	* 函数名称	:  Param()
	* 函数描述	:  返回指定参数信息
	* 输入		:  index 参数序列
	* 输出		:  无
	* 返回值	:  参数对象
	* 作者		:  li.shugang
	*******************************************************************************/
	TMsqDBParam& Param(int index) throw (TMsqDBException);              
	/******************************************************************************
	* 函数名称	:  Param()
	* 函数描述	:  根据参数名(不分大小写)返回列信息; 建议使用Field(int i)获得更高的效率
	* 输入		:  paramName 参数名
	* 输出		:  无
	* 返回值	:  参数对象
	* 作者		:  li.shugang
	*******************************************************************************/
	TMsqDBParam& Param(const char *paramName) throw (TMsqDBException);  

    //以下是设置参数值
	/******************************************************************************
	* 函数名称	:  IsParamExist()
	* 函数描述	:  判断指定参数是否存在   
	* 输入		:  paramName 参数名
	* 输出		:  无
	* 返回值	:  true 存在，false 不存在
	* 作者		:  li.shugang
	*******************************************************************************/
	virtual bool IsParamExist(const char *paramName);
	/******************************************************************************
	* 函数名称	:  SetParameter()
	* 函数描述	:  获取列精度   
	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
	*           :  是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, const char* paramValue, bool isOutput = false) throw (TMsqDBException);   
	/******************************************************************************
	* 函数名称	:  SetParameter()
	* 函数描述	:  获取列精度   
	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
	*           :  是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, const char paramValue, bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  SetParameter()
	* 函数描述	:  获取列精度   
	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
	*           :  是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, int paramValue, bool isOutput = false) throw (TMsqDBException); 
	/******************************************************************************
	* 函数名称	:  SetParameter()
	* 函数描述	:  获取列精度   
	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
	*           :  是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, double paramValue, bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  SetParameter()
	* 函数描述	:  获取列精度   
	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
	*           :  是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, long paramValue, bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  SetParameter()
	* 函数描述	:  获取列精度   
	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
	*           :  是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, long long paramValue, bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  SetParameter()
	* 函数描述	:  获取列精度   
	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
	*           :  是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName,const char* paramValue,int iBufferLen,bool isOutput = false) throw (TMsqDBException);//用于传入BLOB/BINARY类型字段
	/******************************************************************************
	* 函数名称	:  SetParameterNULL()
	* 函数描述	:  设置NULL参数   
	* 输入		:  paramName 参数名
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParameterNULL(const char *paramName) throw (TMsqDBException);

    /*备注:增加int iArraySize=0为了兼容Informix部分的同名函数,OCI部分不进行任何处理*/
    //数组操作
	/******************************************************************************
	* 函数名称	:  SetParamArray()
	* 函数描述	:  设置数组参数  
	* 输入		:  paramName 参数名，paramValue 参数值，iStructSize iStrSize 
	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParamArray(const char *paramName, char     ** paramValue, int iStructSize, int iStrSize ,int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMsqDBException);  
	/******************************************************************************
	* 函数名称	:  SetParamArray()
	* 函数描述	:  设置数组参数  
	* 输入		:  paramName 参数名，paramValue 参数值，iStructSize iStrSize 
	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParamArray(const char *paramName, int       * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMsqDBException); 
	/******************************************************************************
	* 函数名称	:  SetParamArray()
	* 函数描述	:  设置数组参数  
	* 输入		:  paramName 参数名，paramValue 参数值，iStructSize iStrSize 
	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParamArray(const char *paramName, double    * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  SetParamArray()
	* 函数描述	:  设置数组参数  
	* 输入		:  paramName 参数名，paramValue 参数值，iStructSize iStrSize 
	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParamArray(const char *paramName, long      * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  SetParamArray()
	* 函数描述	:  设置数组参数  
	* 输入		:  paramName 参数名，paramValue 参数值，iStructSize iStrSize 
	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetParamArray(const char *paramName, long long * paramValue, int iStructSize,int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  SetBlobParamArray()
	* 函数描述	:  设置数组参数  
	* 输入		:  paramName 参数名，paramValue 参数值，iBufferLen iStrSize 
	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void SetBlobParamArray(const char *paramName,char *paramValue,int iBufferLen,int iArraySize=0,short* iIsNull=NULL,bool isOutput=false) throw (TMsqDBException);//用于传入BLOB/BINARY类型字段数组参数

    //constructor & destructor
    TMsqDBQuery(TMsqDBDatabase *msqdb) throw (TMsqDBException);
    virtual ~TMsqDBQuery();

private:
    TMsqDBParam *ParamByName(const char *paramName) throw (TMsqDBException);//在内部使用，直接返回参数的指针
    void CheckError(const char* sSql=NULL) throw (TMsqDBException);         //用于判断当前的语句是否正确执行，如果有错误则把错误信息放入errMsg;
    
    void GetParamsDef() throw (TMsqDBException);                        //在setSQL时候获得参数信息
    void FormatSqlStr() throw (TMsqDBException);                        //规范sqlstr将oracle中使用的函数规范为mysql中使用的函数
    void GetFieldsDef() throw (TMsqDBException);                        //获得字段信息,并为字段分配取值的缓冲区

    char *sqlstr;                            							//保存open的Select语句，可以方便调试
    int  iArrSize;                                                      //SetParamterArray 元素个数
    
    MYSQL_STMT    *stmt;
    MYSQL_BIND    *bindparam;
    MYSQL_BIND    *bindresult;
    MYSQL_RES     *prepare_meta_result;

    bool isSelectSql;                          							//sqlstr是否是SELECT语句，只有select语句才返回结果集
    int  iAffectedRows;
    int iFieldCount;                        							//字段个数
    int iParamCount;                        							//参数个数

    TMsqDBField *fieldList;                    							//在内部保存的所有字段信息
    TMsqDBParam *paramList;                								//所有参数设置的信息   

    TMsqDBDatabase *db;                        							//此query属于哪个Dabase,在Constructor中创建
 
    bool fActivated;                                                    //是否已经打开过，setsql()调用
    int  fErrorNo;                                                      //错误码
    unsigned    fPrefetchRows;              							//1.. 
    bool fOpened;                           							//数据集是否打开    
    int nTransTimes;                        							//是否曾经执行过Execute()事务操作，以便与回滚.
#ifdef __DEBUG__
    bool bExecuteFlag;
#endif

};


#define MAX_USR_LENGTH 64
#define MAX_PWD_LENGTH 64
#define MAX_TNS_LENGTH 64

class TMsqDBDatabase: public TMDBDBInterface
{
    friend class TMsqDBQuery;
public:
    TMsqDBDatabase() throw (TMsqDBException);
    virtual ~TMsqDBDatabase();
	/******************************************************************************
	* 函数名称	:  SetLogin()
	* 函数描述	:  设置数据库登录名
	* 输入		:  user 用户名，password 密码，tnsString 服务名
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
    void SetLogin(const char *user, const char *password, const char *tnsString) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  Connect()
	* 函数描述	:  数据库连接
	* 输入		:  bUnused 是否使用,默认false
	* 输出		:  无
	* 返回值	:  true 连接成功，false 连接失败
	* 作者		:  li.shugang
	*******************************************************************************/
	bool Connect(bool bIsAutoCommit = false) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  Connect()
	* 函数描述	:  数据库连接
	* 输入		:  user 用户名，password 密码，tnsString 服务名，bUnused 是否使用,默认false
	* 输出		:  无
	* 返回值	:  true 连接成功，false 连接失败
	* 作者		:  li.shugang
	*******************************************************************************/
	bool Connect(const char *usr, const char *pwd, const char *tns, bool bIsAutoCommit=false) throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  Disconnect()
	* 函数描述	:  断开数据库连接
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  1 成功，否则失败
	* 作者		:  li.shugang
	*******************************************************************************/
	int Disconnect() throw (TMsqDBException);
	/******************************************************************************
	* 函数名称	:  Commit()
	* 函数描述	:  事务提交
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void Commit();
	/******************************************************************************
	* 函数名称	:  Rollback()
	* 函数描述	:  事务回滚
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  无
	* 作者		:  li.shugang
	*******************************************************************************/
	void Rollback();
	/******************************************************************************
	* 函数名称	:  IsConnect()
	* 函数描述	:  测试数据库是否连接正常
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  true 正常，false 数据库连接异常
	* 作者		:  li.shugang
	*******************************************************************************/
	bool IsConnect() throw (TMsqDBException);						
	/******************************************************************************
	* 函数名称	:  GetProvider()
	* 函数描述	:  获取开发者
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  ORADB
	* 作者		:  li.shugang
	*******************************************************************************/
	const char* GetProvider() {return _MYSQL_CONN_STRING_;}
	/******************************************************************************
	* 函数名称	:  CreateDBQuery()
	* 函数描述	:  创建数据库查询对象
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  TMsqDBQuery 对象
	* 作者		:  li.shugang
	*******************************************************************************/
	TMsqDBQuery *CreateDBQuery() throw (TMsqDBException);
private:     															//用于登录链接服务器      
    char *usr, *pwd, *dbs, *host, *tns;
    bool AutoCommit;
    int port;
    MYSQL * conn;    
    int  fErrorNo;         												//错误号
    void CheckError(const char* sSql=NULL) throw (TMsqDBException);     //用于判断当前的语句是否正确执行，如果有错误则把错误信息放入errMsg;
    static bool bInitMutex;
	TMsqDBDatabase * ppDBList[MAX_QUERY_PER_DB];
};

//}

#endif   //DB_MYSQL

#endif   //__ORACLE_QUERY_H__

