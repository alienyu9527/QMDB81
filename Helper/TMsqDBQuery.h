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

//oracle�����쳣������
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
	* ��������	:  AsInteger()
	* ��������	:  ��ȡ�������Ͳ���ֵ  
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ���ز���ֵ
	* ����		:  li.shugang
	*******************************************************************************/
    int     AsInteger()  throw (TMsqDBException);
    /******************************************************************************
	* ��������	:  AsFloat()
	* ��������	:  ��ȡ���ظ����Ͳ���ֵ   
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ���ز���ֵ
	* ����		:  li.shugang
	*******************************************************************************/
    double  AsFloat()    throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  AsLong()
	* ��������	:  ��ȡ���س����Ͳ���ֵ    
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ���ز���ֵ
	* ����		:  li.shugang
	*******************************************************************************/
    long    AsLong()     throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  AsString()
	* ��������	:  ��ȡ�����ַ�����ֵ    
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ���ز���ֵ
	* ����		:  li.shugang
	*******************************************************************************/
    char*   AsString()   throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  isNULL()
	* ��������	:  ����ֵ�Ƿ�Ϊ��   
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  true �ǣ�false ��Ϊ��
	* ����		:  li.shugang
	*******************************************************************************/
    bool    isNULL()     throw (TMsqDBException);
private:
    char    *name;
    enum_field_types     dataType;              //��������
    int     intValue;
    double  dblValue;         					//�洢����������ֵ(������)
    long    longValue;        				    //llong���ݻ�����
    char   * stringValue;     					//�ַ������ػ�����    

    unsigned long string_length;         		//string��MAX_LENGTH
    unsigned long length;                       //string��ʵ�ʴ�С
    my_bool  is_null[MAX_DATA_COUNTS];          					//�ڷ���ֵʱ���Ƿ�Ϊ��  
    
    int    * intArray;        					//INT����
    double * dblArray;        					//DOUBLE����
    long  * longArray;       					//LONG����
    MDB_INT64 * llongArray;   						//LONG LONG����
    char * stringArray[MAX_DATA_COUNTS];		//STRING����
    
    bool  fIsOutput;          					//�Ƿ����������.Ĭ�����������
      
};

class TMsqDBField : public TMDBDBFieldInterface
{
    friend class TMsqDBQuery;
public: 
    virtual ~TMsqDBField(); 
	/******************************************************************************
	* ��������	:  isNULL()
	* ��������	:  ��fetch�����и��е������Ƿ�Ϊ��  
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  true �ǣ�false ��Ϊ��
	* ����		:  li.shugang
	*******************************************************************************/
    bool    isNULL();           																
	/******************************************************************************
	* ��������	:  AsString()
	* ��������	:  ���ֶ��������͵�����ת�����ַ���  
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ת������ַ���
	* ����		:  li.shugang
	*******************************************************************************/
    char   *AsString() throw(TMsqDBException);
	/******************************************************************************
	* ��������	:  AsFloat()
	* ��������	:  ��������������� 
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ��������
	* ����		:  li.shugang
	*******************************************************************************/
    double  AsFloat() throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  isNULL()
	* ��������	:  ��������������� 
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ��������
	* ����		:  li.shugang
	*******************************************************************************/
    MDB_INT64   AsInteger() throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  AsBlobFile()
	* ��������	:  Blob���ʹ�����ȡ��file��
	* ����		:  fileName ����Blob����
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
    void        AsBlobFile(const char *fileName) throw (TMsqDBException); 
	/******************************************************************************
	* ��������	:  AsBlobBuffer()
	* ��������	:  ��Blob���ݱ��浽������,�������Ĵ�С�Զ������������ػ�������С*bufLength.
	* ����		:  ��
	* ���		:  buf BLOB�ֶ�ֵ��bufLength BLOB�ֶ�ֵ����
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
    void        AsBlobBuffer(unsigned char* &buf, unsigned int *bufLength) throw (TMsqDBException); 
    /******************************************************************************
	* ��������	:  AsBlobBuffer()
	* ��������	:  ����ֵ�Ƿ�Ϊ��   
	* ����		:  ��
	* ���		:  iBufferLen BLOB�ֶ�ֵ����
	* ����ֵ	:  BLOB�ֶ�ֵ
	* ����		:  li.shugang
	*******************************************************************************/
    char*       AsBlobBuffer(int &iBufferLen) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  AsDateTimeString()
	* ��������	:  �������͵�����HH:MM:DD HH24:MI��ʽ��ȡ,ʹ��asString()��ȡ����������������ΪHH:MM:DD
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ת�����ʱ���ַ���,��ʽΪYYYYMMDDHH24MMSS
	* ����		:  li.shugang
	*******************************************************************************/
	char       *AsDateTimeString() throw (TMsqDBException); 
	/******************************************************************************
	* ��������	:  AsDateTime()
	* ��������	:  ���������ֶεĸ�������  
	* ����		:  ��
	* ���		:  year �꣬month �£�day �գ�hour Сʱ��minute ���ӣ�second ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void        AsDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TMsqDBException); 
	/******************************************************************************
	* ��������	:  AsTimeT()
	* ��������	:  ����time_tʱ������
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ת�����ʱ��
	* ����		:  li.shugang
	*******************************************************************************/
	time_t      AsTimeT() throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  LoadFromFile()
	* ��������	:  д�뵽blob�� 
	* ����		:  fileName ����blob�����ļ�
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void        LoadFromFile(const char *fileName) throw (TMsqDBException); 
	/******************************************************************************
	* ��������	:  LoadFromBuffer()
	* ��������	:  ��BLOB�������û������������      
	* ����		:  buf ����BLOB���ݵĻ�������bufLength ����������
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void        LoadFromBuffer(unsigned char *buf, unsigned int bufLength) throw (TMsqDBException);    
	/******************************************************************************
	* ��������	:  ClearDataBuf()
	* ��������	:  ��ջ�����
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void        ClearDataBuf();																																																		//�α�ѭ����ȡָ���������ݺ�,δ����ϴεĻ���

public:
	/******************************************************************************
	* ��������	:  GetFieldName()
	* ��������	:  ��ȡ����
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ����
	* ����		:  li.shugang
	*******************************************************************************/
    char *GetFieldName();    																			
	/******************************************************************************
	* ��������	:  GetFieldType()
	* ��������	:  ��ȡ������   
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ��������
	* ����		:  li.shugang
	*******************************************************************************/
	long GetFieldType();     																		
	/******************************************************************************
	* ��������	:  GetFieldSize()
	* ��������	:  ��ȡ�д�С
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  �д�С
	* ����		:  li.shugang
	*******************************************************************************/
	long GetFieldSize();     																			
	/******************************************************************************
	* ��������	:  GetFieldPrecision()
	* ��������	:  ��ȡ�о���   
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  �о���
	* ����		:  li.shugang
	*******************************************************************************/
	int  GetFieldPrecision();     													
//private:
    /******************************************************************************
	* ��������	:  AsDateTimeInternal()
	* ��������	:  �������ڵĸ�������,û��������У�飬ֻ���ڲ�����    
	* ����		:  ��
	* ���		:  year �꣬month �£�day �գ�hour Сʱ��minute ���ӣ�second ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
    void    AsDateTimeInternal(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TMsqDBException);                  

    TMsqDBField();
    MYSQL_FIELD * field;

    char    *name;              						    //�ֶ�����
    char    *buffer;                                        //�ֶ�����ָ��
    unsigned long length;                 					//����ʵ�ʳ���
    unsigned long max_length;                 				//���ݶ��峤��
    enum_field_types   buffer_type;               			//�������� in(INT_TYPE,FLOAT_TYPE,DATE_TYPE,STRING_TYPE,ROWID_TYPE)
    my_bool is_null;           					            //�ֶζ���ʱ�Ƿ�����Ϊ��ֵ--Ϊ�˺���������һ��

    char fStrBuffer[MDB_MAX_STRING_VALUE_LENGTH];           //���ڱ���ת��Ϊ�ַ������ֵ
    TMsqDBQuery *fParentQuery;                 				//ָ���Field�����ڵ�Query   
};


class TMsqDBQuery: public TMDBDBQueryInterface
{
    friend class TMsqDBField;
public:
    /******************************************************************************
	* ��������	:  Close()
	* ��������	:  �ر�SQL��䣬��׼��������һ��sql���   
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
    void Close(); 
	/******************************************************************************
	* ��������	:  CloseSQL()
	* ��������	:  ����odbc
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  �о���
	* ����		:  li.shugang
	*******************************************************************************/
	void CloseSQL() {}
	/******************************************************************************
	* ��������	:  SetSQL()
	* ��������	:  ����Sqlstatement   
	* ����		:  inSqlstmt sql���
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetSQL(const char *inSqlstmt,int iPreFetchRows=0) throw (TMsqDBException); 
	/******************************************************************************
	* ��������	:  Open()
	* ��������	:  ��SQL SELECT��䷵�ؽ���� 
	* ����		:  prefetchRows Ԥ����ǰ��¼����Ĭ��200��
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void Open(int prefetchRows=MDB_PREFETCH_ROWS) throw (TMsqDBException); 
	/******************************************************************************
	* ��������	:  Next()
	* ��������	:  �ƶ�����һ����¼ 
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  true �ɹ�;false ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
	bool Next() throw (TMsqDBException);   
	/******************************************************************************
	* ��������	:  Execute()
	* ��������	:  ִ�з�SELECT���,û�з��ؽ����
	* ����		:  iters ָ��sql����ִ�д���
	* ���		:  ��
	* ����ֵ	:  true �ɹ�;false ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
	bool Execute(int iters=1) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  Commit()
	* ��������	:  �����ύ
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  true �ύ�ɹ���false �ύʧ��
	* ����		:  li.shugang
	*******************************************************************************/
	bool Commit();                                            
	/******************************************************************************
	* ��������	:  Rollback()
	* ��������	:  ����ع�
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  true �ع��ɹ���false �ع�ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
	bool Rollback();                                                
	/******************************************************************************
	* ��������	:  RowsAffected()
	* ��������	:  DELETE/UPDATE/INSERT����޸ĵļ�¼��Ŀ
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  DELETE/UPDATE/INSERT����޸ĵļ�¼��Ŀ
	* ����		:  li.shugang
	*******************************************************************************/
	int  RowsAffected() { return iAffectedRows;};              
	/******************************************************************************
	* ��������	:  GetSQLCode()
	* ��������	:  ����Oracleִ�н������
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  Oracleִ�н������
	* ����		:  li.shugang
	*******************************************************************************/
	int  GetSQLCode() { return fErrorNo;};                        

    //������Ϣ���              
	/******************************************************************************
	* ��������	:  FieldCount()
	* ��������	:  �ܹ��м�����
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ����
	* ����		:  li.shugang
	*******************************************************************************/
	int FieldCount() ;       
	/******************************************************************************
	* ��������	:  GetFieldPrecision()
	* ��������	:  ����ָ������Ϣ  
	* ����		:  index ������
	* ���		:  ��
	* ����ֵ	:  �ж���
	* ����		:  li.shugang
	*******************************************************************************/
	TMsqDBField& Field(int index)  throw (TMsqDBException)    ;           
	/******************************************************************************
	* ��������	:  Field()
	* ��������	:  ��������(���ִ�Сд)��������Ϣ; ����ʹ��Field(int i)��ø��ߵ�Ч��   
	* ����		:  fieldName ����
	* ���		:  ��
	* ����ֵ	:  ����Ϣ
	* ����		:  li.shugang
	*******************************************************************************/
	TMsqDBField& Field(const char *fieldName) throw (TMsqDBException);    
	/******************************************************************************
	* ��������	:  IsFieldExist()
	* ��������	:  ָ�����Ƿ����   
	* ����		:  fieldName ����
	* ���		:  ��
	* ����ֵ	:  �о���
	* ����		:  li.shugang
	*******************************************************************************/
	bool IsFieldExist(const char *fieldName);

    //�������Ϣ���
	/******************************************************************************
	* ��������	:  ParamCount()
	* ��������	:  ��ȡ��������  
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ��������
	* ����		:  li.shugang
	*******************************************************************************/
	int ParamCount() ;
	/******************************************************************************
	* ��������	:  Param()
	* ��������	:  ����ָ��������Ϣ
	* ����		:  index ��������
	* ���		:  ��
	* ����ֵ	:  ��������
	* ����		:  li.shugang
	*******************************************************************************/
	TMsqDBParam& Param(int index) throw (TMsqDBException);              
	/******************************************************************************
	* ��������	:  Param()
	* ��������	:  ���ݲ�����(���ִ�Сд)��������Ϣ; ����ʹ��Field(int i)��ø��ߵ�Ч��
	* ����		:  paramName ������
	* ���		:  ��
	* ����ֵ	:  ��������
	* ����		:  li.shugang
	*******************************************************************************/
	TMsqDBParam& Param(const char *paramName) throw (TMsqDBException);  

    //���������ò���ֵ
	/******************************************************************************
	* ��������	:  IsParamExist()
	* ��������	:  �ж�ָ�������Ƿ����   
	* ����		:  paramName ������
	* ���		:  ��
	* ����ֵ	:  true ���ڣ�false ������
	* ����		:  li.shugang
	*******************************************************************************/
	virtual bool IsParamExist(const char *paramName);
	/******************************************************************************
	* ��������	:  SetParameter()
	* ��������	:  ��ȡ�о���   
	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
	*           :  �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, const char* paramValue, bool isOutput = false) throw (TMsqDBException);   
	/******************************************************************************
	* ��������	:  SetParameter()
	* ��������	:  ��ȡ�о���   
	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
	*           :  �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, const char paramValue, bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  SetParameter()
	* ��������	:  ��ȡ�о���   
	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
	*           :  �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, int paramValue, bool isOutput = false) throw (TMsqDBException); 
	/******************************************************************************
	* ��������	:  SetParameter()
	* ��������	:  ��ȡ�о���   
	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
	*           :  �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, double paramValue, bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  SetParameter()
	* ��������	:  ��ȡ�о���   
	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
	*           :  �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, long paramValue, bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  SetParameter()
	* ��������	:  ��ȡ�о���   
	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
	*           :  �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName, long long paramValue, bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  SetParameter()
	* ��������	:  ��ȡ�о���   
	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
	*           :  �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParameter(const char *paramName,const char* paramValue,int iBufferLen,bool isOutput = false) throw (TMsqDBException);//���ڴ���BLOB/BINARY�����ֶ�
	/******************************************************************************
	* ��������	:  SetParameterNULL()
	* ��������	:  ����NULL����   
	* ����		:  paramName ������
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParameterNULL(const char *paramName) throw (TMsqDBException);

    /*��ע:����int iArraySize=0Ϊ�˼���Informix���ֵ�ͬ������,OCI���ֲ������κδ���*/
    //�������
	/******************************************************************************
	* ��������	:  SetParamArray()
	* ��������	:  �����������  
	* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParamArray(const char *paramName, char     ** paramValue, int iStructSize, int iStrSize ,int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMsqDBException);  
	/******************************************************************************
	* ��������	:  SetParamArray()
	* ��������	:  �����������  
	* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParamArray(const char *paramName, int       * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMsqDBException); 
	/******************************************************************************
	* ��������	:  SetParamArray()
	* ��������	:  �����������  
	* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParamArray(const char *paramName, double    * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  SetParamArray()
	* ��������	:  �����������  
	* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParamArray(const char *paramName, long      * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  SetParamArray()
	* ��������	:  �����������  
	* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetParamArray(const char *paramName, long long * paramValue, int iStructSize,int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  SetBlobParamArray()
	* ��������	:  �����������  
	* ����		:  paramName ��������paramValue ����ֵ��iBufferLen iStrSize 
	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void SetBlobParamArray(const char *paramName,char *paramValue,int iBufferLen,int iArraySize=0,short* iIsNull=NULL,bool isOutput=false) throw (TMsqDBException);//���ڴ���BLOB/BINARY�����ֶ��������

    //constructor & destructor
    TMsqDBQuery(TMsqDBDatabase *msqdb) throw (TMsqDBException);
    virtual ~TMsqDBQuery();

private:
    TMsqDBParam *ParamByName(const char *paramName) throw (TMsqDBException);//���ڲ�ʹ�ã�ֱ�ӷ��ز�����ָ��
    void CheckError(const char* sSql=NULL) throw (TMsqDBException);         //�����жϵ�ǰ������Ƿ���ȷִ�У�����д�����Ѵ�����Ϣ����errMsg;
    
    void GetParamsDef() throw (TMsqDBException);                        //��setSQLʱ���ò�����Ϣ
    void FormatSqlStr() throw (TMsqDBException);                        //�淶sqlstr��oracle��ʹ�õĺ����淶Ϊmysql��ʹ�õĺ���
    void GetFieldsDef() throw (TMsqDBException);                        //����ֶ���Ϣ,��Ϊ�ֶη���ȡֵ�Ļ�����

    char *sqlstr;                            							//����open��Select��䣬���Է������
    int  iArrSize;                                                      //SetParamterArray Ԫ�ظ���
    
    MYSQL_STMT    *stmt;
    MYSQL_BIND    *bindparam;
    MYSQL_BIND    *bindresult;
    MYSQL_RES     *prepare_meta_result;

    bool isSelectSql;                          							//sqlstr�Ƿ���SELECT��䣬ֻ��select���ŷ��ؽ����
    int  iAffectedRows;
    int iFieldCount;                        							//�ֶθ���
    int iParamCount;                        							//��������

    TMsqDBField *fieldList;                    							//���ڲ�����������ֶ���Ϣ
    TMsqDBParam *paramList;                								//���в������õ���Ϣ   

    TMsqDBDatabase *db;                        							//��query�����ĸ�Dabase,��Constructor�д���
 
    bool fActivated;                                                    //�Ƿ��Ѿ��򿪹���setsql()����
    int  fErrorNo;                                                      //������
    unsigned    fPrefetchRows;              							//1.. 
    bool fOpened;                           							//���ݼ��Ƿ��    
    int nTransTimes;                        							//�Ƿ�����ִ�й�Execute()����������Ա���ع�.
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
	* ��������	:  SetLogin()
	* ��������	:  �������ݿ��¼��
	* ����		:  user �û�����password ���룬tnsString ������
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
    void SetLogin(const char *user, const char *password, const char *tnsString) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  Connect()
	* ��������	:  ���ݿ�����
	* ����		:  bUnused �Ƿ�ʹ��,Ĭ��false
	* ���		:  ��
	* ����ֵ	:  true ���ӳɹ���false ����ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
	bool Connect(bool bIsAutoCommit = false) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  Connect()
	* ��������	:  ���ݿ�����
	* ����		:  user �û�����password ���룬tnsString ��������bUnused �Ƿ�ʹ��,Ĭ��false
	* ���		:  ��
	* ����ֵ	:  true ���ӳɹ���false ����ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
	bool Connect(const char *usr, const char *pwd, const char *tns, bool bIsAutoCommit=false) throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  Disconnect()
	* ��������	:  �Ͽ����ݿ�����
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  1 �ɹ�������ʧ��
	* ����		:  li.shugang
	*******************************************************************************/
	int Disconnect() throw (TMsqDBException);
	/******************************************************************************
	* ��������	:  Commit()
	* ��������	:  �����ύ
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void Commit();
	/******************************************************************************
	* ��������	:  Rollback()
	* ��������	:  ����ع�
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  li.shugang
	*******************************************************************************/
	void Rollback();
	/******************************************************************************
	* ��������	:  IsConnect()
	* ��������	:  �������ݿ��Ƿ���������
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  true ������false ���ݿ������쳣
	* ����		:  li.shugang
	*******************************************************************************/
	bool IsConnect() throw (TMsqDBException);						
	/******************************************************************************
	* ��������	:  GetProvider()
	* ��������	:  ��ȡ������
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ORADB
	* ����		:  li.shugang
	*******************************************************************************/
	const char* GetProvider() {return _MYSQL_CONN_STRING_;}
	/******************************************************************************
	* ��������	:  CreateDBQuery()
	* ��������	:  �������ݿ��ѯ����
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  TMsqDBQuery ����
	* ����		:  li.shugang
	*******************************************************************************/
	TMsqDBQuery *CreateDBQuery() throw (TMsqDBException);
private:     															//���ڵ�¼���ӷ�����      
    char *usr, *pwd, *dbs, *host, *tns;
    bool AutoCommit;
    int port;
    MYSQL * conn;    
    int  fErrorNo;         												//�����
    void CheckError(const char* sSql=NULL) throw (TMsqDBException);     //�����жϵ�ǰ������Ƿ���ȷִ�У�����д�����Ѵ�����Ϣ����errMsg;
    static bool bInitMutex;
	TMsqDBDatabase * ppDBList[MAX_QUERY_PER_DB];
};

//}

#endif   //DB_MYSQL

#endif   //__ORACLE_QUERY_H__

