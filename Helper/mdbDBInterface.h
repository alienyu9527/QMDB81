/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbDBInterface.cpp		
*@Description�� �ṩDB���ʽӿڻ���
*@Author:		li.shugang
*@Date��	    2009��08��13��
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
	
	const   size_t MDB_PREFETCH_ROWS = 200;                      				//Ԥ����ȡn�����ݵ�������,������������
	const   size_t MDB_MAX_STRING_VALUE_LENGTH = 255;            				//���ص��ַ��������С����ع��̲�������
	const   size_t MDB_MAX_LOB_BUFFER_LENGTH = 1024;             				//LOB���ݻ����������ռ�
	const   size_t MDB_MAX_PARAMS_COUNT = 1000;                   				//���������Ŀ
	const   size_t MDB_LOB_FLUSH_BUFFER_SIZE = 400*1024;         				//LOB���ݻ��۵�����ʱ��д�����ݿ�
	
	//�쳣ʹ��
	const   size_t MDB_MAX_ERRMSG_LENGTH = 4096;                 				//������Ϣ����󳤶�
	const   size_t MDB_MAX_SQLSTMT_LENGTH = 4096;                				//���ִ����SQL��䳤��
	
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
	const   char* const MDB_ERR_CAPABILITY_NOT_YET_SUPPORT = "capability not support yet:%s"; //�������������Խ��Χ
	const   char* const MDB_ERR_READ_PARAM_DATA = "read parameter value data type error, parameter name:%s, method:%s";
	
	//const define:
	const char* const MDB_NULL_STRING = "";
	const int MDB_NULL_NUMBER = 0;
	
	#define MDB_SIZE_LONG_LONG 64																																																						//LONG LONG��󳤶�
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
    char m_sErrMsg[MDB_MAX_ERRMSG_LENGTH+1];        			//������Ϣ
    char m_sErrSql[MDB_MAX_SQLSTMT_LENGTH+1];           	//���������sql���
    int m_lErrCode;                                   //�����
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
    virtual bool IsConnect()=0;																		//�������ݿ��Ƿ���������
    virtual const char* GetProvider()=0;
    virtual TMDBDBQueryInterface *CreateDBQuery() throw (TMDBDBExcpInterface)=0;
    bool fConnected;       //��Connect���Ƿ����ӳɹ�
};

class TMDBDBQueryInterface
{
public:
    TMDBDBQueryInterface() {};
    virtual ~TMDBDBQueryInterface() {}
    //��Ҫ����
    virtual void Close()=0;                                                   				//�ر�SQL��䣬��׼��������һ��sql���
    virtual void CloseSQL()=0;    
    virtual void SetSQL(const char *inSqlstmt,int iPreFetchRows=0) throw (TMDBDBExcpInterface)=0;        //����Sqlstatement
    virtual void Open(int prefetchRows=MDB_PREFETCH_ROWS) throw (TMDBDBExcpInterface)=0; //��SQL SELECT��䷵�ؽ����
    virtual bool Next() throw (TMDBDBExcpInterface)=0;                               //�ƶ�����һ����¼
    virtual bool Execute(int iters=1) throw (TMDBDBExcpInterface)=0;                 //ִ�з�SELECT���,û�з��ؽ����
    virtual bool Commit()=0;                                                  				//�����ύ
    virtual bool Rollback()=0;                                                				//����ع�
    virtual int  RowsAffected()=0;               																				//DELETE/UPDATE/INSERT����޸ĵļ�¼��Ŀ
    virtual bool IsFieldExist(const char *fieldName)=0;                                     //�鿴�����Ƿ����   
    //�ݲ�����//virtual int  GetSQLCode()=0;                          											//����Oracleִ�н������
    virtual void CheckError(const char* sSql=NULL) throw (TMDBDBExcpInterface)=0;  
    

    //������Ϣ���              
    virtual int FieldCount()=0;                                                         //�ܹ��м�����
    virtual TMDBDBFieldInterface& Field(int index)  throw (TMDBDBExcpInterface)=0;            //���ص�i������Ϣ
    virtual TMDBDBFieldInterface& Field(const char *fieldName) throw (TMDBDBExcpInterface)=0; //��������(���ִ�Сд)��������Ϣ; ����ʹ��Field(int i)��ø��ߵ�Ч��
    
    
    //���������ò���ֵ
    virtual bool IsParamExist(const char *paramName)=0;
    virtual void SetParameter(const char *paramName, const char* paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0;   
    virtual void SetParameter(const char *paramName, const char paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0;    //add by fu.wenjun@20041125
    
    virtual void SetParameter(const char *paramName, int paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0; 
    virtual void SetParameter(const char *paramName, double paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    virtual void SetParameter(const char *paramName, long paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    virtual void SetParameter(const char *paramName, long long paramValue, bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    
    
    virtual void SetParameter(const char *paramName,const char* paramValue,int iBufferLen,bool isOutput = false) throw (TMDBDBExcpInterface)=0;//���ڴ���BLOB/BINARY�����ֶ�
    virtual void SetParameterNULL(const char *paramName) throw (TMDBDBExcpInterface)=0;

    /*��ע:����int iArraySize=0Ϊ�˼���Informix���ֵ�ͬ������,OCI���ֲ������κδ���*/
    //�������
    virtual void SetParamArray(const char *paramName, char     ** paramValue, int iStructSize, int iStrSize ,int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMDBDBExcpInterface)=0;  
    virtual void SetParamArray(const char *paramName, int       * paramValue, int iStructSize,               int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMDBDBExcpInterface)=0; 
    virtual void SetParamArray(const char *paramName, double    * paramValue, int iStructSize,               int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    virtual void SetParamArray(const char *paramName, long      * paramValue, int iStructSize,               int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    virtual void SetParamArray(const char *paramName, long long * paramValue, int iStructSize,               int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TMDBDBExcpInterface)=0;
    virtual void SetBlobParamArray(const char *paramName,char *paramValue,int iBufferLen,int iArraySize=0,short* iIsNull=NULL,bool isOutput=false) throw (TMDBDBExcpInterface)=0;//���ڴ���BLOB/BINARY�����ֶ��������
};


class TMDBDBFieldInterface
{
public: 
    virtual ~TMDBDBFieldInterface() {} 
    virtual bool    isNULL()=0;           										//��fetch�����и��е������Ƿ�Ϊ��
    virtual char   *AsString() throw(TMDBDBExcpInterface)=0;
    virtual double  AsFloat() throw (TMDBDBExcpInterface)=0;
    virtual long long   AsInteger() throw (TMDBDBExcpInterface)=0;
	virtual char* AsBlobBuffer(int &iBufferLen)throw (TMDBDBExcpInterface)=0;          //���BLOB�ֶ�   
    virtual long GetFieldType()=0;
    virtual char *GetFieldName()=0;
    //���ڴ���
    virtual char*		AsDateTimeString() throw (TMDBDBExcpInterface)=0;   			//�������͵�����HH:MM:DD HH24:MI��ʽ��ȡ,ʹ��asString()��ȡ����������������ΪHH:MM:DD
    virtual void   AsDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TMDBDBExcpInterface)=0; //�������ڵĸ�������
    //�ݲ�����//virtual time_t AsTimeT() throw (TOraDBExcpInterface)=0;
    virtual void   ClearDataBuf()=0;																	  	//�α�ѭ����ȡָ���������ݺ�,δ����ϴεĻ���
};


#endif  //__QUICK_MEMORY_DATABASE_INTERFACE_H__

