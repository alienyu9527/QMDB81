#ifndef __ORACLE_QUERY_H__
#define __ORACLE_QUERY_H__

#ifdef DB_ORACLE

#include "Helper/mdbDBInterface.h"
//#include "BillingSDK.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <oci.h>
#include <oratypes.h>
#include <ocidfn.h>
#include <ocidem.h>
#include <ociapr.h>
#include <oci.h>
#include "Common/mdbCommons.h"
#include "Common/mdbNtcSplit.h"

//using namespace ZSmart::BillingSDK;


#ifdef WIN32
	#pragma warning(disable: 4290)
	#pragma warning(disable: 4267)
#endif

//namespace QuickMDB{

    //异常使用
#ifndef MAX_ERR_CAT_LENGTH
    	#define MAX_ERR_CAT_LENGTH 50	
#endif

#ifndef _ORADB_CONN_STRING_
    	#define _ORADB_CONN_STRING_              "ORADB"
#endif

    /* classes defined in this file: */
    class TOraDBException;
    class TOraDBDatabase;
    class TOraDBQuery;
    class TOraDBField;
    class TOraDBParam;
    class TOraDBSession;

     struct Table_Reocrd
    {
    	OCIString *sRecord_value;
    };
    struct null_Table_Reocrd
    {
        OCIInd null_sRecord_value;
    };

    class Tcolumn
    {
    public:
    	Tcolumn();
    	~Tcolumn();
    	char name[20];
    	char value[4096];
    };

    class TData
    {
    public:
       TData();
       ~TData();
       char tablename[20];
       char operate[10];
       Tcolumn m_column[40];
    };

    //oracle操作异常处理类
    class TOraDBException: public TMDBDBExcpInterface
    {
    public:
        virtual ~TOraDBException() {}
        TOraDBException(sword errNumb, OCIError *err,const char *cat,const char *sql);//执行OCI函数发生的错误
        TOraDBException(const char *sql, const char* errFormat, ...);
    private:
        char errCategory[MAX_ERR_CAT_LENGTH+1];  																	 //错误分类
    };

//    typedef long long MDB_INT64; 
    class TOraDBParam  
    {
        friend class TOraDBQuery;
    public:
        TOraDBParam();
        virtual ~TOraDBParam();
    	/******************************************************************************
    	* 函数名称	:  AsInteger()
    	* 函数描述	:  读取返回整型参数值  
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  返回参数值
    	* 作者		:  li.shugang
    	*******************************************************************************/
        int     AsInteger()  throw (TOraDBException);
        /******************************************************************************
    	* 函数名称	:  AsFloat()
    	* 函数描述	:  读取返回浮点型参数值   
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  返回参数值
    	* 作者		:  li.shugang
    	*******************************************************************************/
        double  AsFloat()    throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  AsLong()
    	* 函数描述	:  读取返回长整型参数值    
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  返回参数值
    	* 作者		:  li.shugang
    	*******************************************************************************/
        long    AsLong()     throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  AsString()
    	* 函数描述	:  读取返回字符参数值    
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  返回参数值
    	* 作者		:  li.shugang
    	*******************************************************************************/
        char*   AsString()   throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  isNULL()
    	* 函数描述	:  返回值是否为空   
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  true 是，false 不为空
    	* 作者		:  li.shugang
    	*******************************************************************************/
        bool    isNULL()     throw (TOraDBException);
    private:
        char    *name;
        ub2     dataType;
        int     intValue;
        double  dblValue;         																		//存储输入和输出的值(缓冲区)
        long    longValue;        																		//long数据缓冲区
        char   * stringValue;     																		//字符串返回缓冲区
        int    * intArray;        																		//INT数组
        double * dblArray;        																		//DOUBLE数组
        long  * longArray;       																		//LONG数组
        MDB_INT64 * llongArray;   																		//LONG LONG数组
        char ** stringArray;      																		//STRING数组
        int   stringSize;         																		//STRING数组中的string大小
        bool  fIsOutput;          																		//是否是输出参数.默认是输入参数
        sb2   indicator;          																		//在返回值时候是否为空
        OCIBind  *hBind;
    };

    class TOraDBField : public TMDBDBFieldInterface
    {
        friend class TOraDBQuery;
    public: 
        virtual ~TOraDBField(); 
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
        char   *AsString() throw(TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  AsFloat()
    	* 函数描述	:  浮点类型数据输出 
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  浮点数据
    	* 作者		:  li.shugang
    	*******************************************************************************/
        double  AsFloat() throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  isNULL()
    	* 函数描述	:  整型类型数据输出 
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  整形数据
    	* 作者		:  li.shugang
    	*******************************************************************************/
        long long   AsInteger() throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  AsBlobFile()
    	* 函数描述	:  Blob类型处理，读取到file中
    	* 输入		:  fileName 保存Blob数据
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
        void        AsBlobFile(const char *fileName) throw (TOraDBException); 
    	/******************************************************************************
    	* 函数名称	:  AsBlobBuffer()
    	* 函数描述	:  将Blob数据保存到缓冲区,缓冲区的大小自动创建，并返回缓冲区大小*bufLength.
    	* 输入		:  无
    	* 输出		:  buf BLOB字段值，bufLength BLOB字段值长度
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
        void        AsBlobBuffer(unsigned char* &buf, unsigned int *bufLength) throw (TOraDBException); 
        /******************************************************************************
    	* 函数名称	:  AsBlobBuffer()
    	* 函数描述	:  返回值是否为空   
    	* 输入		:  无
    	* 输出		:  iBufferLen BLOB字段值长度
    	* 返回值	:  BLOB字段值
    	* 作者		:  li.shugang
    	*******************************************************************************/
        char*       AsBlobBuffer(int &iBufferLen) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  AsDateTimeString()
    	* 函数描述	:  把日期型的列以HH:MM:DD HH24:MI格式读取,使用asString()读取的日期型数据类型为HH:MM:DD
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  转换后的时间字符串,格式为YYYYMMDDHH24MMSS
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	char       *AsDateTimeString() throw (TOraDBException); 
    	/******************************************************************************
    	* 函数名称	:  AsDateTime()
    	* 函数描述	:  返回日期字段的各个部分  
    	* 输入		:  无
    	* 输出		:  year 年，month 月，day 日，hour 小时，minute 分钟，second 秒
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void        AsDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TOraDBException); 
    	/******************************************************************************
    	* 函数名称	:  AsTimeT()
    	* 函数描述	:  返回time_t时间类型
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  转换后的时间
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	time_t      AsTimeT() throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  LoadFromFile()
    	* 函数描述	:  写入到blob中 
    	* 输入		:  fileName 保存blob数据文件
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void        LoadFromFile(const char *fileName) throw (TOraDBException); 
    	/******************************************************************************
    	* 函数名称	:  LoadFromBuffer()
    	* 函数描述	:  把BLOB的内容用缓冲区内容替代      
    	* 输入		:  buf 保存BLOB数据的缓冲区，bufLength 缓冲区长度
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void        LoadFromBuffer(unsigned char *buf, unsigned int bufLength) throw (TOraDBException);    
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
    private:
        /******************************************************************************
    	* 函数名称	:  AsDateTimeInternal()
    	* 函数描述	:  返回日期的各个部分,没有作其他校验，只是内部调用    
    	* 输入		:  无
    	* 输出		:  year 年，month 月，day 日，hour 小时，minute 分钟，second 秒
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
        void    AsDateTimeInternal(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TOraDBException);                  
        char    *name;              																//字段名称
        long    size;               																//数据长度
        long    type;               																//数据类型 in(INT_TYPE,FLOAT_TYPE,DATE_TYPE,STRING_TYPE,ROWID_TYPE)
        int     precision;          																//数值总长度
        int     scale;              																//数值中小数点个数
        bool    nullable;           																//字段定义时是否允许为空值--为了和其他的相一致
        TOraDBQuery *fParentQuery;                 				//指向该Field所属于的Query
        TOraDBField();
        //数据缓冲区,分别为字符串、整数、浮点数分配空间
        ub1 fStrBuffer[MDB_MAX_STRING_VALUE_LENGTH];    //用于保存转换为字符串后的值
        ub1 *fDataBuf;                      								//在分析字段时候获得空间max(该列的最大长度,MAX_STRING_VALUE_LENGTH), 在Destructor中释放
        OCILobLocator *hBlob;               								//支持LOB
        sb2 *fDataIndicator;                								//在defineByPos中使用，用于在fetch时察看是否有字段值返回、字段值是否被截断;valueIsNULL, isTruncated根据此值获得结果
                                            								
        ub2   fReturnDataLen;               								//读取数据时返回的真实长度
        ub2 fInternalType;                  								//Oracle内部数据类型
        ub2 fRequestType;                   								//在读取数据时候的请求数据类型
        OCIDefine *hDefine;                 								//用于读取列信息
                                            								
        long lDataBufLen;                   								//记录fDataBuf实际长度,列长*数组元素个数
        char *sBlob;																																//存放BLOB字段值
                                            								
        																			                 								
        OCILobLocator **ahBlob;																					//获取选择列类型为BLOB的字段值
        int iBlobCount;
    };

    class TOraDBQuery: public TMDBDBQueryInterface
    {
        friend class TOraDBField;
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
    	void SetSQL(const char *inSqlstmt,int iPreFetchRows=0) throw (TOraDBException); 
    	/******************************************************************************
    	* 函数名称	:  Open()
    	* 函数描述	:  打开SQL SELECT语句返回结果集 
    	* 输入		:  prefetchRows 预先提前记录数，默认200条
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void Open(int prefetchRows=MDB_PREFETCH_ROWS) throw (TOraDBException); 
    	/******************************************************************************
    	* 函数名称	:  Next()
    	* 函数描述	:  移动到下一个记录 
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  true 成功;false 失败
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	bool Next() throw (TOraDBException);   
    	/******************************************************************************
    	* 函数名称	:  Execute()
    	* 函数描述	:  执行非SELECT语句,没有返回结果集
    	* 输入		:  iters 指出sql语句的执行次数
    	* 输出		:  无
    	* 返回值	:  true 成功;false 失败
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	bool Execute(int iters=1) throw (TOraDBException);
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
    	int  RowsAffected() { return static_cast<int>(fTotalRowsFetched);};              
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
    	TOraDBField& Field(int index)  throw (TOraDBException)    ;           
    	/******************************************************************************
    	* 函数名称	:  Field()
    	* 函数描述	:  根据列名(不分大小写)返回列信息; 建议使用Field(int i)获得更高的效率   
    	* 输入		:  fieldName 列名
    	* 输出		:  无
    	* 返回值	:  列信息
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	TOraDBField& Field(const char *fieldName) throw (TOraDBException);    
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
    	TOraDBParam& Param(int index) throw (TOraDBException);              
    	/******************************************************************************
    	* 函数名称	:  Param()
    	* 函数描述	:  根据参数名(不分大小写)返回列信息; 建议使用Field(int i)获得更高的效率
    	* 输入		:  paramName 参数名
    	* 输出		:  无
    	* 返回值	:  参数对象
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	TOraDBParam& Param(const char *paramName) throw (TOraDBException);  

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
    	void SetParameter(const char *paramName, const char* paramValue, bool isOutput = false) throw (TOraDBException);   
    	/******************************************************************************
    	* 函数名称	:  SetParameter()
    	* 函数描述	:  获取列精度   
    	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
    	*           :  是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName, const char paramValue, bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  SetParameter()
    	* 函数描述	:  获取列精度   
    	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
    	*           :  是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName, int paramValue, bool isOutput = false) throw (TOraDBException); 
    	/******************************************************************************
    	* 函数名称	:  SetParameter()
    	* 函数描述	:  获取列精度   
    	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
    	*           :  是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName, double paramValue, bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  SetParameter()
    	* 函数描述	:  获取列精度   
    	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
    	*           :  是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName, long paramValue, bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  SetParameter()
    	* 函数描述	:  获取列精度   
    	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
    	*           :  是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName, long long paramValue, bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  SetParameter()
    	* 函数描述	:  获取列精度   
    	* 输入		:  paramName 参数名，paramValue 参数值，isOutput 
    	*           :  是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName,const char* paramValue,int iBufferLen,bool isOutput = false) throw (TOraDBException);//用于传入BLOB/BINARY类型字段
    	/******************************************************************************
    	* 函数名称	:  SetParameterNULL()
    	* 函数描述	:  设置NULL参数   
    	* 输入		:  paramName 参数名
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParameterNULL(const char *paramName) throw (TOraDBException);

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
    	void SetParamArray(const char *paramName, char     ** paramValue, int iStructSize, int iStrSize ,int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TOraDBException);  
    	/******************************************************************************
    	* 函数名称	:  SetParamArray()
    	* 函数描述	:  设置数组参数  
    	* 输入		:  paramName 参数名，paramValue 参数值，iStructSize iStrSize 
    	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParamArray(const char *paramName, int       * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TOraDBException); 
    	/******************************************************************************
    	* 函数名称	:  SetParamArray()
    	* 函数描述	:  设置数组参数  
    	* 输入		:  paramName 参数名，paramValue 参数值，iStructSize iStrSize 
    	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParamArray(const char *paramName, double    * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  SetParamArray()
    	* 函数描述	:  设置数组参数  
    	* 输入		:  paramName 参数名，paramValue 参数值，iStructSize iStrSize 
    	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParamArray(const char *paramName, long      * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  SetParamArray()
    	* 函数描述	:  设置数组参数  
    	* 输入		:  paramName 参数名，paramValue 参数值，iStructSize iStrSize 
    	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetParamArray(const char *paramName, long long * paramValue, int iStructSize,int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  SetBlobParamArray()
    	* 函数描述	:  设置数组参数  
    	* 输入		:  paramName 参数名，paramValue 参数值，iBufferLen iStrSize 
    	*           :  iArraySize 数组大小，isOutput 是否输出参数(默认false)
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	void SetBlobParamArray(const char *paramName,char *paramValue,int iBufferLen,int iArraySize=0,short* iIsNull=NULL,bool isOutput=false) throw (TOraDBException);//用于传入BLOB/BINARY类型字段数组参数

        //constructor & destructor
        TOraDBQuery(TOraDBDatabase *oradb) throw (TOraDBException);
        TOraDBQuery(TOraDBDatabase *oradb,TOraDBSession *session) throw (TOraDBException);
        virtual ~TOraDBQuery();
    private:
        char *fSqlStmt;                         							//保存open的Select语句，可以方便调试
        ub2 fStmtType;                          							//***ub2!!! 保存sqlstmt的类型:SELECT/UPDATE/DELETE/INSERT/CREATE/BEGIN/ALTER...
        bool fActivated;                        							//是否已经处于打开状态，在调用OCIStmtPrepare成功后为True
                                                							
        unsigned    fFetched;                   							//0..prefetchRows
        unsigned    fPrefetchRows;              							//1.. 
        unsigned    fCurrRow;                   							//0..fetched */
        unsigned    fTotalRowsFetched;          							//rows fetched from the start
                                                							
        int fFieldCount;                        							//字段个数
        TOraDBDatabase *db;                        							//此query属于哪个Dabase,在Constructor中创建
        TOraDBField *fieldList;                    							//在内部保存的所有字段信息
        void GetFieldsDef() throw (TOraDBException);      //获得字段信息,并为字段分配取值的缓冲区

        TOraDBParam *ParamByName(const char *paramName) throw (TOraDBException);//在内部使用，直接返回参数的指针

        void CheckError(const char* sSql=NULL) throw (TOraDBException);        //用于判断当前的语句是否正确执行，如果有错误则把错误信息放入errMsg;

        int fParamCount;                        							//参数个数
        TOraDBParam *paramList;                											//所有参数设置的信息
        void GetParamsDef() throw (TOraDBException);      //在setSQL时候获得参数信息
        int nTransTimes;                        							//是否曾经执行过Execute()事务操作，以便与回滚.
      
        OCIStmt *hStmt;                     											//用于分析sql语句的handle
        OCIError *hErr;                     											//错误处理
        sword fErrorNo;                     											//错误号
        bool fEof;                              							//在Fetch时候，已经达到最后一个记录,防止已经读到最后一个记录后，又fetch发生的错误
        bool fBof;                              							//在Open()时候为True,在Next()如果有数据为false;用于判断用户是否可以从缓冲区中读取列值,该部分尚未完成
        bool fOpened;                           							//数据集是否打开    
#ifdef __DEBUG__
        bool bExecuteFlag;
#endif
    };

    class TOraQueueBase
    {
    public:
    	TOraQueueBase() throw(TOraDBException);
    	 ~TOraQueueBase();
    	 void SetLogin(const char *user, const char *password, const char *tnsString) throw (TOraDBException);
    	 bool Connect(bool bUnused=false) throw (TOraDBException);
    	 bool Connect(const char *usr, const char *pwd, const char *tns, bool bUnused=false) throw (TOraDBException);
    	 int Disconnect() throw (TOraDBException);
    	 void Commit();
    	 void InitDequeue();
    	 TData* AQDequeue();
    	 void SetTableRecord();
    public:
           void CheckError(const char* sSql=NULL) throw (TOraDBException);     //用于判断当前的语句是否正确执行，如果有错误则把错误信息放入errMsg;
    	OCISession *hUser;
    	OCISvcCtx *hDBSvc;      																				//用于登录链接服务器      
    	char *usr, *pwd, *tns;  																				//登录名
    	bool fConnected;        																				//在Connect中是否连接成功
    	sword fErrorNo;         																				//错误号
    	OCIError *hDBErr;
    	OCIEnv *hEnv;
    	OCIServer *hSvr;
    	ub4 wait;
    	ub4 deq_mode;
    	ub4 navigation;
    	Table_Reocrd *deqMesg;
    	null_Table_Reocrd *ndeqMesg;
    	OCIType *mesg_tdo;
    	OCIAQDeqOptions *deqopt;
    	TMdbNtcSplit tSplit;
    	TData* m_tableRecord;
    	char sSourceStr[4096];
    	

    };



    class TOraDBDatabase: public TMDBDBInterface
    {
        friend class TOraDBQuery;
        friend class TOraDBSession;
        friend class TOraDBDirPath;
        friend class TOraDBField;
    public:
        TOraDBDatabase() throw (TOraDBException);
        virtual ~TOraDBDatabase();
    	/******************************************************************************
    	* 函数名称	:  SetLogin()
    	* 函数描述	:  设置数据库登录名
    	* 输入		:  user 用户名，password 密码，tnsString 服务名
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
        void SetLogin(const char *user, const char *password, const char *tnsString) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  Connect()
    	* 函数描述	:  数据库连接
    	* 输入		:  bUnused 是否使用,默认false
    	* 输出		:  无
    	* 返回值	:  true 连接成功，false 连接失败
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	bool Connect(bool bUnused=false) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  Connect()
    	* 函数描述	:  数据库连接
    	* 输入		:  user 用户名，password 密码，tnsString 服务名，bUnused 是否使用,默认false
    	* 输出		:  无
    	* 返回值	:  true 连接成功，false 连接失败
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	bool Connect(const char *usr, const char *pwd, const char *tns, bool bUnused=false) throw (TOraDBException);
    	/******************************************************************************
    	* 函数名称	:  Disconnect()
    	* 函数描述	:  断开数据库连接
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  1 成功，否则失败
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	int Disconnect() throw (TOraDBException);
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
    	bool IsConnect() throw (TOraDBException);						
    	/******************************************************************************
    	* 函数名称	:  GetProvider()
    	* 函数描述	:  获取开发者
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  ORADB
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	const char* GetProvider() {return _ORADB_CONN_STRING_;}
    	/******************************************************************************
    	* 函数名称	:  CreateDBQuery()
    	* 函数描述	:  创建数据库查询对象
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  TOraDBQuery 对象
    	* 作者		:  li.shugang
    	*******************************************************************************/
    	TOraDBQuery *CreateDBQuery() throw (TMDBDBExcpInterface);
    private:
        OCISession *hUser;
        OCISvcCtx *hDBSvc;      																				//用于登录链接服务器      
        char *usr, *pwd, *tns;  																				//登录名
        bool fConnected;        																				//在Connect中是否连接成功
        sword fErrorNo;         																				//错误号
        void CheckError(const char* sSql=NULL) throw (TOraDBException);     //用于判断当前的语句是否正确执行，如果有错误则把错误信息放入errMsg;
        OCIError *hDBErr;
        OCIEnv *hEnv;
        OCIServer *hSvr;
        TOraDBQuery *m_pTOraDBQuery;																						//用于测试数据库是否连接正常
    };



    //外部不访问,取消派生
    class TOraDBSession //: public TOraDBSessionInterface
    {   
    public:
        TOraDBSession(TOraDBDatabase *pDB);
        virtual ~TOraDBSession();
    	/******************************************************************************
    	* 函数名称	:  sessionBegin()
    	* 函数描述	:  开始回话
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
        void sessionBegin();
    	/******************************************************************************
    	* 函数名称	:  sessionEnd()
    	* 函数描述	:  结束一个回话 
    	* 输入		:  无
    	* 输出		:  无
    	* 返回值	:  无
    	* 作者		:  li.shugang
    	*******************************************************************************/
        void sessionEnd();

    private:
        OCISvcCtx   *m_hSrvCtx;
        OCISession  *m_hSession;
        OCIError    *m_hError;

        bool    m_bActive;

        int m_iErrorNo;
        void CheckError();

        friend  class TOraDBDirPath;
        friend  class TOraDBQuery; 
    };

//}

#endif   //DB_ORACLE

#endif   //__ORACLE_QUERY_H__
