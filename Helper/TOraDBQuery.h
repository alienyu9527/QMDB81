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

    //�쳣ʹ��
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

    //oracle�����쳣������
    class TOraDBException: public TMDBDBExcpInterface
    {
    public:
        virtual ~TOraDBException() {}
        TOraDBException(sword errNumb, OCIError *err,const char *cat,const char *sql);//ִ��OCI���������Ĵ���
        TOraDBException(const char *sql, const char* errFormat, ...);
    private:
        char errCategory[MAX_ERR_CAT_LENGTH+1];  																	 //�������
    };

//    typedef long long MDB_INT64; 
    class TOraDBParam  
    {
        friend class TOraDBQuery;
    public:
        TOraDBParam();
        virtual ~TOraDBParam();
    	/******************************************************************************
    	* ��������	:  AsInteger()
    	* ��������	:  ��ȡ�������Ͳ���ֵ  
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ���ز���ֵ
    	* ����		:  li.shugang
    	*******************************************************************************/
        int     AsInteger()  throw (TOraDBException);
        /******************************************************************************
    	* ��������	:  AsFloat()
    	* ��������	:  ��ȡ���ظ����Ͳ���ֵ   
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ���ز���ֵ
    	* ����		:  li.shugang
    	*******************************************************************************/
        double  AsFloat()    throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  AsLong()
    	* ��������	:  ��ȡ���س����Ͳ���ֵ    
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ���ز���ֵ
    	* ����		:  li.shugang
    	*******************************************************************************/
        long    AsLong()     throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  AsString()
    	* ��������	:  ��ȡ�����ַ�����ֵ    
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ���ز���ֵ
    	* ����		:  li.shugang
    	*******************************************************************************/
        char*   AsString()   throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  isNULL()
    	* ��������	:  ����ֵ�Ƿ�Ϊ��   
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  true �ǣ�false ��Ϊ��
    	* ����		:  li.shugang
    	*******************************************************************************/
        bool    isNULL()     throw (TOraDBException);
    private:
        char    *name;
        ub2     dataType;
        int     intValue;
        double  dblValue;         																		//�洢����������ֵ(������)
        long    longValue;        																		//long���ݻ�����
        char   * stringValue;     																		//�ַ������ػ�����
        int    * intArray;        																		//INT����
        double * dblArray;        																		//DOUBLE����
        long  * longArray;       																		//LONG����
        MDB_INT64 * llongArray;   																		//LONG LONG����
        char ** stringArray;      																		//STRING����
        int   stringSize;         																		//STRING�����е�string��С
        bool  fIsOutput;          																		//�Ƿ����������.Ĭ�����������
        sb2   indicator;          																		//�ڷ���ֵʱ���Ƿ�Ϊ��
        OCIBind  *hBind;
    };

    class TOraDBField : public TMDBDBFieldInterface
    {
        friend class TOraDBQuery;
    public: 
        virtual ~TOraDBField(); 
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
        char   *AsString() throw(TOraDBException);
    	/******************************************************************************
    	* ��������	:  AsFloat()
    	* ��������	:  ��������������� 
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ��������
    	* ����		:  li.shugang
    	*******************************************************************************/
        double  AsFloat() throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  isNULL()
    	* ��������	:  ��������������� 
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ��������
    	* ����		:  li.shugang
    	*******************************************************************************/
        long long   AsInteger() throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  AsBlobFile()
    	* ��������	:  Blob���ʹ�����ȡ��file��
    	* ����		:  fileName ����Blob����
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
        void        AsBlobFile(const char *fileName) throw (TOraDBException); 
    	/******************************************************************************
    	* ��������	:  AsBlobBuffer()
    	* ��������	:  ��Blob���ݱ��浽������,�������Ĵ�С�Զ������������ػ�������С*bufLength.
    	* ����		:  ��
    	* ���		:  buf BLOB�ֶ�ֵ��bufLength BLOB�ֶ�ֵ����
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
        void        AsBlobBuffer(unsigned char* &buf, unsigned int *bufLength) throw (TOraDBException); 
        /******************************************************************************
    	* ��������	:  AsBlobBuffer()
    	* ��������	:  ����ֵ�Ƿ�Ϊ��   
    	* ����		:  ��
    	* ���		:  iBufferLen BLOB�ֶ�ֵ����
    	* ����ֵ	:  BLOB�ֶ�ֵ
    	* ����		:  li.shugang
    	*******************************************************************************/
        char*       AsBlobBuffer(int &iBufferLen) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  AsDateTimeString()
    	* ��������	:  �������͵�����HH:MM:DD HH24:MI��ʽ��ȡ,ʹ��asString()��ȡ����������������ΪHH:MM:DD
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ת�����ʱ���ַ���,��ʽΪYYYYMMDDHH24MMSS
    	* ����		:  li.shugang
    	*******************************************************************************/
    	char       *AsDateTimeString() throw (TOraDBException); 
    	/******************************************************************************
    	* ��������	:  AsDateTime()
    	* ��������	:  ���������ֶεĸ�������  
    	* ����		:  ��
    	* ���		:  year �꣬month �£�day �գ�hour Сʱ��minute ���ӣ�second ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void        AsDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TOraDBException); 
    	/******************************************************************************
    	* ��������	:  AsTimeT()
    	* ��������	:  ����time_tʱ������
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ת�����ʱ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	time_t      AsTimeT() throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  LoadFromFile()
    	* ��������	:  д�뵽blob�� 
    	* ����		:  fileName ����blob�����ļ�
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void        LoadFromFile(const char *fileName) throw (TOraDBException); 
    	/******************************************************************************
    	* ��������	:  LoadFromBuffer()
    	* ��������	:  ��BLOB�������û������������      
    	* ����		:  buf ����BLOB���ݵĻ�������bufLength ����������
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void        LoadFromBuffer(unsigned char *buf, unsigned int bufLength) throw (TOraDBException);    
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
    private:
        /******************************************************************************
    	* ��������	:  AsDateTimeInternal()
    	* ��������	:  �������ڵĸ�������,û��������У�飬ֻ���ڲ�����    
    	* ����		:  ��
    	* ���		:  year �꣬month �£�day �գ�hour Сʱ��minute ���ӣ�second ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
        void    AsDateTimeInternal(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TOraDBException);                  
        char    *name;              																//�ֶ�����
        long    size;               																//���ݳ���
        long    type;               																//�������� in(INT_TYPE,FLOAT_TYPE,DATE_TYPE,STRING_TYPE,ROWID_TYPE)
        int     precision;          																//��ֵ�ܳ���
        int     scale;              																//��ֵ��С�������
        bool    nullable;           																//�ֶζ���ʱ�Ƿ�����Ϊ��ֵ--Ϊ�˺���������һ��
        TOraDBQuery *fParentQuery;                 				//ָ���Field�����ڵ�Query
        TOraDBField();
        //���ݻ�����,�ֱ�Ϊ�ַ���������������������ռ�
        ub1 fStrBuffer[MDB_MAX_STRING_VALUE_LENGTH];    //���ڱ���ת��Ϊ�ַ������ֵ
        ub1 *fDataBuf;                      								//�ڷ����ֶ�ʱ���ÿռ�max(���е���󳤶�,MAX_STRING_VALUE_LENGTH), ��Destructor���ͷ�
        OCILobLocator *hBlob;               								//֧��LOB
        sb2 *fDataIndicator;                								//��defineByPos��ʹ�ã�������fetchʱ�쿴�Ƿ����ֶ�ֵ���ء��ֶ�ֵ�Ƿ񱻽ض�;valueIsNULL, isTruncated���ݴ�ֵ��ý��
                                            								
        ub2   fReturnDataLen;               								//��ȡ����ʱ���ص���ʵ����
        ub2 fInternalType;                  								//Oracle�ڲ���������
        ub2 fRequestType;                   								//�ڶ�ȡ����ʱ���������������
        OCIDefine *hDefine;                 								//���ڶ�ȡ����Ϣ
                                            								
        long lDataBufLen;                   								//��¼fDataBufʵ�ʳ���,�г�*����Ԫ�ظ���
        char *sBlob;																																//���BLOB�ֶ�ֵ
                                            								
        																			                 								
        OCILobLocator **ahBlob;																					//��ȡѡ��������ΪBLOB���ֶ�ֵ
        int iBlobCount;
    };

    class TOraDBQuery: public TMDBDBQueryInterface
    {
        friend class TOraDBField;
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
    	void SetSQL(const char *inSqlstmt,int iPreFetchRows=0) throw (TOraDBException); 
    	/******************************************************************************
    	* ��������	:  Open()
    	* ��������	:  ��SQL SELECT��䷵�ؽ���� 
    	* ����		:  prefetchRows Ԥ����ǰ��¼����Ĭ��200��
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void Open(int prefetchRows=MDB_PREFETCH_ROWS) throw (TOraDBException); 
    	/******************************************************************************
    	* ��������	:  Next()
    	* ��������	:  �ƶ�����һ����¼ 
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  true �ɹ�;false ʧ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	bool Next() throw (TOraDBException);   
    	/******************************************************************************
    	* ��������	:  Execute()
    	* ��������	:  ִ�з�SELECT���,û�з��ؽ����
    	* ����		:  iters ָ��sql����ִ�д���
    	* ���		:  ��
    	* ����ֵ	:  true �ɹ�;false ʧ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	bool Execute(int iters=1) throw (TOraDBException);
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
    	int  RowsAffected() { return static_cast<int>(fTotalRowsFetched);};              
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
    	TOraDBField& Field(int index)  throw (TOraDBException)    ;           
    	/******************************************************************************
    	* ��������	:  Field()
    	* ��������	:  ��������(���ִ�Сд)��������Ϣ; ����ʹ��Field(int i)��ø��ߵ�Ч��   
    	* ����		:  fieldName ����
    	* ���		:  ��
    	* ����ֵ	:  ����Ϣ
    	* ����		:  li.shugang
    	*******************************************************************************/
    	TOraDBField& Field(const char *fieldName) throw (TOraDBException);    
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
    	TOraDBParam& Param(int index) throw (TOraDBException);              
    	/******************************************************************************
    	* ��������	:  Param()
    	* ��������	:  ���ݲ�����(���ִ�Сд)��������Ϣ; ����ʹ��Field(int i)��ø��ߵ�Ч��
    	* ����		:  paramName ������
    	* ���		:  ��
    	* ����ֵ	:  ��������
    	* ����		:  li.shugang
    	*******************************************************************************/
    	TOraDBParam& Param(const char *paramName) throw (TOraDBException);  

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
    	void SetParameter(const char *paramName, const char* paramValue, bool isOutput = false) throw (TOraDBException);   
    	/******************************************************************************
    	* ��������	:  SetParameter()
    	* ��������	:  ��ȡ�о���   
    	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
    	*           :  �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName, const char paramValue, bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  SetParameter()
    	* ��������	:  ��ȡ�о���   
    	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
    	*           :  �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName, int paramValue, bool isOutput = false) throw (TOraDBException); 
    	/******************************************************************************
    	* ��������	:  SetParameter()
    	* ��������	:  ��ȡ�о���   
    	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
    	*           :  �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName, double paramValue, bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  SetParameter()
    	* ��������	:  ��ȡ�о���   
    	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
    	*           :  �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName, long paramValue, bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  SetParameter()
    	* ��������	:  ��ȡ�о���   
    	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
    	*           :  �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName, long long paramValue, bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  SetParameter()
    	* ��������	:  ��ȡ�о���   
    	* ����		:  paramName ��������paramValue ����ֵ��isOutput 
    	*           :  �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParameter(const char *paramName,const char* paramValue,int iBufferLen,bool isOutput = false) throw (TOraDBException);//���ڴ���BLOB/BINARY�����ֶ�
    	/******************************************************************************
    	* ��������	:  SetParameterNULL()
    	* ��������	:  ����NULL����   
    	* ����		:  paramName ������
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParameterNULL(const char *paramName) throw (TOraDBException);

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
    	void SetParamArray(const char *paramName, char     ** paramValue, int iStructSize, int iStrSize ,int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TOraDBException);  
    	/******************************************************************************
    	* ��������	:  SetParamArray()
    	* ��������	:  �����������  
    	* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
    	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParamArray(const char *paramName, int       * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TOraDBException); 
    	/******************************************************************************
    	* ��������	:  SetParamArray()
    	* ��������	:  �����������  
    	* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
    	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParamArray(const char *paramName, double    * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  SetParamArray()
    	* ��������	:  �����������  
    	* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
    	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParamArray(const char *paramName, long      * paramValue, int iStructSize, int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  SetParamArray()
    	* ��������	:  �����������  
    	* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
    	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetParamArray(const char *paramName, long long * paramValue, int iStructSize,int iArraySize=0,short* iIsNull=NULL,bool isOutput = false) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  SetBlobParamArray()
    	* ��������	:  �����������  
    	* ����		:  paramName ��������paramValue ����ֵ��iBufferLen iStrSize 
    	*           :  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	void SetBlobParamArray(const char *paramName,char *paramValue,int iBufferLen,int iArraySize=0,short* iIsNull=NULL,bool isOutput=false) throw (TOraDBException);//���ڴ���BLOB/BINARY�����ֶ��������

        //constructor & destructor
        TOraDBQuery(TOraDBDatabase *oradb) throw (TOraDBException);
        TOraDBQuery(TOraDBDatabase *oradb,TOraDBSession *session) throw (TOraDBException);
        virtual ~TOraDBQuery();
    private:
        char *fSqlStmt;                         							//����open��Select��䣬���Է������
        ub2 fStmtType;                          							//***ub2!!! ����sqlstmt������:SELECT/UPDATE/DELETE/INSERT/CREATE/BEGIN/ALTER...
        bool fActivated;                        							//�Ƿ��Ѿ����ڴ�״̬���ڵ���OCIStmtPrepare�ɹ���ΪTrue
                                                							
        unsigned    fFetched;                   							//0..prefetchRows
        unsigned    fPrefetchRows;              							//1.. 
        unsigned    fCurrRow;                   							//0..fetched */
        unsigned    fTotalRowsFetched;          							//rows fetched from the start
                                                							
        int fFieldCount;                        							//�ֶθ���
        TOraDBDatabase *db;                        							//��query�����ĸ�Dabase,��Constructor�д���
        TOraDBField *fieldList;                    							//���ڲ�����������ֶ���Ϣ
        void GetFieldsDef() throw (TOraDBException);      //����ֶ���Ϣ,��Ϊ�ֶη���ȡֵ�Ļ�����

        TOraDBParam *ParamByName(const char *paramName) throw (TOraDBException);//���ڲ�ʹ�ã�ֱ�ӷ��ز�����ָ��

        void CheckError(const char* sSql=NULL) throw (TOraDBException);        //�����жϵ�ǰ������Ƿ���ȷִ�У�����д�����Ѵ�����Ϣ����errMsg;

        int fParamCount;                        							//��������
        TOraDBParam *paramList;                											//���в������õ���Ϣ
        void GetParamsDef() throw (TOraDBException);      //��setSQLʱ���ò�����Ϣ
        int nTransTimes;                        							//�Ƿ�����ִ�й�Execute()����������Ա���ع�.
      
        OCIStmt *hStmt;                     											//���ڷ���sql����handle
        OCIError *hErr;                     											//������
        sword fErrorNo;                     											//�����
        bool fEof;                              							//��Fetchʱ���Ѿ��ﵽ���һ����¼,��ֹ�Ѿ��������һ����¼����fetch�����Ĵ���
        bool fBof;                              							//��Open()ʱ��ΪTrue,��Next()���������Ϊfalse;�����ж��û��Ƿ���Դӻ������ж�ȡ��ֵ,�ò�����δ���
        bool fOpened;                           							//���ݼ��Ƿ��    
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
           void CheckError(const char* sSql=NULL) throw (TOraDBException);     //�����жϵ�ǰ������Ƿ���ȷִ�У�����д�����Ѵ�����Ϣ����errMsg;
    	OCISession *hUser;
    	OCISvcCtx *hDBSvc;      																				//���ڵ�¼���ӷ�����      
    	char *usr, *pwd, *tns;  																				//��¼��
    	bool fConnected;        																				//��Connect���Ƿ����ӳɹ�
    	sword fErrorNo;         																				//�����
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
    	* ��������	:  SetLogin()
    	* ��������	:  �������ݿ��¼��
    	* ����		:  user �û�����password ���룬tnsString ������
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
        void SetLogin(const char *user, const char *password, const char *tnsString) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  Connect()
    	* ��������	:  ���ݿ�����
    	* ����		:  bUnused �Ƿ�ʹ��,Ĭ��false
    	* ���		:  ��
    	* ����ֵ	:  true ���ӳɹ���false ����ʧ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	bool Connect(bool bUnused=false) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  Connect()
    	* ��������	:  ���ݿ�����
    	* ����		:  user �û�����password ���룬tnsString ��������bUnused �Ƿ�ʹ��,Ĭ��false
    	* ���		:  ��
    	* ����ֵ	:  true ���ӳɹ���false ����ʧ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	bool Connect(const char *usr, const char *pwd, const char *tns, bool bUnused=false) throw (TOraDBException);
    	/******************************************************************************
    	* ��������	:  Disconnect()
    	* ��������	:  �Ͽ����ݿ�����
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  1 �ɹ�������ʧ��
    	* ����		:  li.shugang
    	*******************************************************************************/
    	int Disconnect() throw (TOraDBException);
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
    	bool IsConnect() throw (TOraDBException);						
    	/******************************************************************************
    	* ��������	:  GetProvider()
    	* ��������	:  ��ȡ������
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ORADB
    	* ����		:  li.shugang
    	*******************************************************************************/
    	const char* GetProvider() {return _ORADB_CONN_STRING_;}
    	/******************************************************************************
    	* ��������	:  CreateDBQuery()
    	* ��������	:  �������ݿ��ѯ����
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  TOraDBQuery ����
    	* ����		:  li.shugang
    	*******************************************************************************/
    	TOraDBQuery *CreateDBQuery() throw (TMDBDBExcpInterface);
    private:
        OCISession *hUser;
        OCISvcCtx *hDBSvc;      																				//���ڵ�¼���ӷ�����      
        char *usr, *pwd, *tns;  																				//��¼��
        bool fConnected;        																				//��Connect���Ƿ����ӳɹ�
        sword fErrorNo;         																				//�����
        void CheckError(const char* sSql=NULL) throw (TOraDBException);     //�����жϵ�ǰ������Ƿ���ȷִ�У�����д�����Ѵ�����Ϣ����errMsg;
        OCIError *hDBErr;
        OCIEnv *hEnv;
        OCIServer *hSvr;
        TOraDBQuery *m_pTOraDBQuery;																						//���ڲ������ݿ��Ƿ���������
    };



    //�ⲿ������,ȡ������
    class TOraDBSession //: public TOraDBSessionInterface
    {   
    public:
        TOraDBSession(TOraDBDatabase *pDB);
        virtual ~TOraDBSession();
    	/******************************************************************************
    	* ��������	:  sessionBegin()
    	* ��������	:  ��ʼ�ػ�
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
    	*******************************************************************************/
        void sessionBegin();
    	/******************************************************************************
    	* ��������	:  sessionEnd()
    	* ��������	:  ����һ���ػ� 
    	* ����		:  ��
    	* ���		:  ��
    	* ����ֵ	:  ��
    	* ����		:  li.shugang
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
