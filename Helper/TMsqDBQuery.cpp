#ifdef   DB_MYSQL

#include <time.h> 
#include <errmsg.h>

#include "Helper/TMsqDBQuery.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbStruct.h"

//namespace QuickMDB{


#define REDUNDANT_BIT_COUNT         50
#define MAX_SQL_LEN                 8192 

#ifdef OS_SUN
 
#define MYSQL_ERROR_TO_THROW(_errSql,...) \
TADD_ERROR(ERROR_UNKNOWN,__VA_ARGS__);\
throw TMsqDBException(_errSql,__VA_ARGS__);\

#define MYSQL_CHECK_RET_THROW_NOSQL(_ret,_errCode,...)  if((iRet = _ret)!=0){\
TADD_ERROR(ERROR_UNKNOWN,__VA_ARGS__);\
throw TMsqDBException(_errCode,"",__VA_ARGS__);\
}

#define MYSQL_ERROR_TO_THROW_NOSQL(_errCode,...) \
TADD_ERROR(ERROR_UNKNOWN,__VA_ARGS__);\
throw TMsqDBException(_errCode,"",__VA_ARGS__);\

#else

#define MYSQL_ERROR_TO_THROW(_errSql,FMT,...) \
TADD_ERROR(ERROR_UNKNOWN,FMT,##__VA_ARGS__);\
throw TMsqDBException(_errSql,"File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);\

#define MYSQL_CHECK_RET_THROW_NOSQL(_ret,_errCode,FMT,...)  if((iRet = _ret)!=0){\
TADD_ERROR(ERROR_UNKNOWN,FMT,##__VA_ARGS__);\
throw TMsqDBException(_errCode,"","File=[%s], Line=[%d],"FMT,__FILE__,__LINE__,##__VA_ARGS__);\
}

#define MYSQL_ERROR_TO_THROW_NOSQL(_errCode,FMT,...) \
TADD_ERROR(ERROR_UNKNOWN,FMT,##__VA_ARGS__);\
throw TMsqDBException(_errCode,"","File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);\
 
#endif


//�Ƚ�2���ַ����Ƿ���ͬ(�����Ǵ�Сд)
bool inline CompareStrNoCase(const char *ori, const char *des)
{
    return TMdbNtcStrFunc::StrNoCaseCmp(ori, des) == 0;
}

/****************** TMsqDBException implementation **************************/
TMsqDBException::TMsqDBException(int errNumb, const char *errMsg, const char *sql)
{
    size_t nLen = 0;

    m_lErrCode = errNumb;

    nLen = strlen(errMsg);
    nLen = nLen >= MDB_MAX_ERRMSG_LENGTH ? MDB_MAX_ERRMSG_LENGTH : nLen;
    strncpy(m_sErrMsg,errMsg,nLen);
    m_sErrMsg[nLen] = '\0';

    if(NULL == sql)
    {
        m_sErrSql[0] = '\0';
    }
    else
    {
        nLen = strlen(sql);
        nLen = nLen >= MDB_MAX_SQLSTMT_LENGTH ? MDB_MAX_SQLSTMT_LENGTH : nLen;
        strncpy(m_sErrSql,sql,nLen);
        m_sErrSql[nLen] = '\0';
    }    
}
                 
TMsqDBException::TMsqDBException(const char *sql, const char* errFormat, ...)
{
	if(NULL == sql)
    {
        m_sErrSql[0] = '\0';
    }
	else
	{
	    size_t nLen;
	    nLen = strlen(sql);
	    nLen = (nLen >= MDB_MAX_SQLSTMT_LENGTH) ? MDB_MAX_SQLSTMT_LENGTH :nLen;
	    strncpy(m_sErrSql,sql,nLen);
	    m_sErrSql[nLen] = '\0';
	}
	
    va_list ap;
    va_start(ap, errFormat);
    vsprintf((char *)m_sErrMsg, errFormat, ap);    
    va_end(ap);
}


TMsqDBException::~TMsqDBException()
{

}



/****************** parameter implementation **************************/
TMsqDBParam::TMsqDBParam()
{
	name = NULL;
    fIsOutput = false;
    stringValue = NULL;
    dblArray=NULL;
    llongArray=NULL;
	intArray = NULL;
	longArray = NULL;    
    string_length = 0;
    length = 0;
    //is_null = 0;            //�ڷ���ֵʱ���Ƿ�Ϊ��

    for(int k=0;k<MAX_DATA_COUNTS;k++)
    {
        stringArray[k]=NULL;
		is_null[k] = false;
    }
}

TMsqDBParam::~TMsqDBParam()
{
    if(name!=NULL)
    {
        delete[] name;
        name=NULL;
    }

    if(stringValue!=NULL)
    {
        delete[] stringValue;
        stringValue=NULL;
    }

    for(int k=0;k<MAX_DATA_COUNTS;k++)
    {
        if(stringArray[k]!=NULL)
        {
            delete [] stringArray[k];
            stringArray[k] = NULL;
        }
    }
    
    if((llongArray!=NULL)&&(dblArray!=NULL))
    {
        delete dblArray;
        dblArray=NULL;
        llongArray=NULL;
    }
}

/******************************************************************************
* ��������	:  AsInteger()
* ��������	:  ��ȡ�������Ͳ���ֵ  
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ���ز���ֵ
* ����		:  li.shugang
*******************************************************************************/
int TMsqDBParam::AsInteger() throw (TMsqDBException)
{
    if ( isNULL() )
        intValue = 0;

    if (dataType == MYSQL_TYPE_LONG)
    {
        return intValue;
    }        
    else    
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBParam::AsInteger()    MDB_ERR_READ_PARAM_DATA",__FILE__,__LINE__);
        throw TMsqDBException("TMsqDBParam", MDB_ERR_READ_PARAM_DATA, name, "AsInteger()");   
    }
}

/******************************************************************************
* ��������	:  AsLong()
* ��������	:  ��ȡ���س����Ͳ���ֵ    
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ���ز���ֵ
* ����		:  li.shugang
*******************************************************************************/
long TMsqDBParam::AsLong() throw (TMsqDBException)
{
    if ( isNULL() )
        longValue = 0;

    if (dataType == MYSQL_TYPE_LONGLONG)
    {
        return longValue;
    }        
    else    
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBParam::AsLong()    MDB_ERR_READ_PARAM_DATA",__FILE__,__LINE__);
        throw TMsqDBException("TMsqDBParam", MDB_ERR_READ_PARAM_DATA, name, "AsLong()");  
    }
}

/******************************************************************************
* ��������	:  AsFloat()
* ��������	:  ��ȡ���ظ����Ͳ���ֵ   
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ���ز���ֵ
* ����		:  li.shugang
*******************************************************************************/
double TMsqDBParam::AsFloat() throw (TMsqDBException)
{
    if ( isNULL() )
        dblValue = 0;

    if (dataType == MYSQL_TYPE_DOUBLE)
    {
        return dblValue;
    }
    else    
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBParam::AsFloat()    MDB_ERR_READ_PARAM_DATA",__FILE__,__LINE__);
        throw TMsqDBException("TMsqDBParam::AsFloat()", MDB_ERR_READ_PARAM_DATA, name, "AsFloat()");
    }
}

/******************************************************************************
* ��������	:  AsString()
* ��������	:  ��ȡ�����ַ�����ֵ	 
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ���ز���ֵ
* ����		:  li.shugang
*******************************************************************************/
char* TMsqDBParam::AsString() throw (TMsqDBException)
{
    if ( isNULL() )
        stringValue[0] = '\0';

    if (dataType == MYSQL_TYPE_STRING)
    {
        return stringValue;
    }          
    else    
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::AsString()    MDB_ERR_READ_PARAM_DATA",__FILE__,__LINE__);
        throw TMsqDBException("TMsqDBParam", MDB_ERR_READ_PARAM_DATA, name, "AsString()");
    }
}

/******************************************************************************
* ��������	:  isNULL()
* ��������	:  ����ֵ�Ƿ�Ϊ��	
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true �ǣ�false ��Ϊ��
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBParam::isNULL() throw (TMsqDBException)
{
    if (! fIsOutput)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::isNULL()    MDB_ERR_READ_PARAM_DATA",__FILE__,__LINE__);
        throw TMsqDBException("TMsqDBParam, not an output parameter", MDB_ERR_READ_PARAM_DATA, name, "isNULL()");
    }
    
    return (is_null[0] == true);
}



/****************** TMsqDBField implementation **************************/
TMsqDBField::TMsqDBField()
{
    name = NULL;    
    buffer = NULL;
    max_length = 0;
    length = 0;
    is_null = 0;    
};

TMsqDBField::~TMsqDBField()
{
    if (name != NULL)
    {
        delete [] name;
        name = NULL;
    }
    if (buffer != NULL)
    {
        delete [] buffer;
        buffer = NULL;
    }           
}

/******************************************************************************
* ��������	:  AsString()
* ��������	:  ���ֶ��������͵�����ת�����ַ���  
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ת������ַ���
* ����		:  li.shugang
*******************************************************************************/
char* TMsqDBField::AsString() throw (TMsqDBException)
{
    int year, month, day, hour, minute, second;
    static char NumStr[100];

    if ( isNULL() )
    {
        sprintf((char *)fStrBuffer,"%s", MDB_NULL_STRING);
        return (char *)fStrBuffer;
    }

    switch ( this->buffer_type )
    {
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_YEAR:
        {
            this->AsDateTimeInternal(year, month, day, hour, minute, second);
            sprintf((char *)fStrBuffer,"%04d%02d%02d%02d%02d%02d", year, month, day,hour, minute, second);
            return (char *)fStrBuffer;
        }
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        {
            int*  intValue = (int*)buffer;
            sprintf(NumStr, "%d", *intValue);
            return NumStr;
        }
        case MYSQL_TYPE_LONGLONG:
        {
            long*  longValue = (long*)buffer;
            sprintf(NumStr, "%ld", *longValue);
            return NumStr;    
        } 
        case MYSQL_TYPE_DOUBLE:
        {
            long longValue;
            double* floatValue = (double*)buffer;
            if ( *floatValue == (longValue=(long)(*floatValue)))
            {
                sprintf(NumStr, "%ld", longValue);
            }
            else
            {
                sprintf(NumStr, "%f", *floatValue);
            }
            return NumStr;
        } 
        case MYSQL_TYPE_FLOAT:
        {
            float* floatValue = (float*)buffer;
            sprintf(NumStr, "%f", *floatValue);
            return NumStr;
        }     
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
        {
            return (char*)buffer; 
        }    
        case MYSQL_TYPE_BLOB:
        {
            sprintf((char *)fStrBuffer, "BLOB...");
            return (char *)fStrBuffer;
        }  
        default:
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBField::AsString()  MDB_ERR_DATA_TYPE_CONVERT  buffer_type[%d]",__FILE__,__LINE__,buffer_type);
            throw TMsqDBException("TMsqDBField::AsString()", MDB_ERR_DATA_TYPE_CONVERT, name, this->buffer_type, "AsString()");
        }
    }
}

/******************************************************************************
* ��������	:  isNULL()
* ��������	:  ��fetch�����и��е������Ƿ�Ϊ��	
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true �ǣ�false ��Ϊ��
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBField::isNULL()
{
    return (this->is_null == 1);
}

/******************************************************************************
* ��������	:  AsBlobFile()
* ��������	:  Blob���ʹ�����ȡ��file��
* ����		:  fileName ����Blob����
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void  TMsqDBField::AsBlobFile(const char *fileName) throw (TMsqDBException)
{
    FILE *fileHandle = NULL;

    if (buffer_type != MYSQL_TYPE_BLOB)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBField::AsBlobFile()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TMsqDBException(fParentQuery->sqlstr, MDB_ERR_DATA_TYPE_CONVERT, name, buffer_type, "asLobFile()");
    }

    fileHandle = fopen( fileName, (const char *) "wb");
    fseek(fileHandle, 0, 0);
    fwrite((void *)buffer, (size_t)length, (size_t)1, fileHandle);
    fclose(fileHandle);
}

/******************************************************************************
* ��������	:  AsBlobBuffer()
* ��������	:  ��Blob���ݱ��浽������,�������Ĵ�С�Զ������������ػ�������С*bufLength.
* ����		:  ��
* ���		:  buf BLOB�ֶ�ֵ��bufLength BLOB�ֶ�ֵ����
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void  TMsqDBField::AsBlobBuffer(unsigned char* &buf, unsigned int *lobLength) throw (TMsqDBException)
{
    unsigned int uintValue = length;
    lobLength = &uintValue;
    buf = (unsigned char*)buffer;
}

/******************************************************************************
* ��������	:  AsBlobBuffer()
* ��������	:  ����ֵ�Ƿ�Ϊ��	
* ����		:  ��
* ���		:  iBufferLen BLOB�ֶ�ֵ����
* ����ֵ	:  BLOB�ֶ�ֵ
* ����		:  li.shugang
*******************************************************************************/
char* TMsqDBField::AsBlobBuffer(int &iBufferLen) throw (TMsqDBException)
{
    iBufferLen = length;
    return (char*)buffer;
}

/******************************************************************************
* ��������	:  LoadFromFile()
* ��������	:  д�뵽blob�� 
* ����		:  fileName ����blob�����ļ�
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void  TMsqDBField::LoadFromFile(const char *fileName) throw (TMsqDBException)
{
    //δ���ã��ǽӿ���TDBFieldInterface��Ա���ݲ�ʵ��
}

/******************************************************************************
* ��������	:  LoadFromBuffer()
* ��������	:  ��BLOB�������û������������ 	 
* ����		:  buf ����BLOB���ݵĻ�������bufLength ����������
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void  TMsqDBField::LoadFromBuffer(unsigned char *buf, unsigned int bufLength) throw (TMsqDBException)
{
    //δ���ã��ǽӿ���TDBFieldInterface��Ա���ݲ�ʵ��
}


/******************************************************************************
* ��������	:  AsFloat()
* ��������	:  ��������������� 
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��������
* ����		:  li.shugang
*******************************************************************************/
double TMsqDBField::AsFloat() throw (TMsqDBException)
{
    float* fRet = NULL;
    double* dRet = NULL;
	double dValue;
    if ( isNULL() )
    {
        dValue = 0;
		return dValue;
    }       

    if (buffer_type == MYSQL_TYPE_FLOAT)
    {
        fRet = (float*)buffer;
        return (double)(*fRet);
    }
    else if(buffer_type == MYSQL_TYPE_DOUBLE)
    {
        dRet = (double*)buffer;
        return *dRet;
    }
    else if(buffer_type == MYSQL_TYPE_DECIMAL)
    {
        dValue = atof(buffer);
        return dValue;
    }
    else if(buffer_type == MYSQL_TYPE_NEWDECIMAL)
    {
        dValue = atof(buffer);
        return dValue;
    }
    else    
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBField::AsFloat()    MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TMsqDBException("TMsqDBField::AsFloat", MDB_ERR_DATA_TYPE_CONVERT, name,buffer_type,"AsFloat()");
    }
}

/******************************************************************************
* ��������	:  AsDateTimeString()
* ��������	:  �������͵�����HH:MM:DD HH24:MI��ʽ��ȡ,ʹ��asString()��ȡ����������������ΪHH:MM:DD
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ת�����ʱ���ַ���,��ʽΪYYYYMMDDHH24MMSS
* ����		:  li.shugang
*******************************************************************************/
char* TMsqDBField::AsDateTimeString() throw (TMsqDBException)
{
    int year, month, day, hour, minute, second;

    if ( buffer_type != MYSQL_TYPE_TIME
         &&buffer_type != MYSQL_TYPE_DATE
         &&buffer_type != MYSQL_TYPE_DATETIME
         &&buffer_type != MYSQL_TYPE_TIMESTAMP)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::AsDateTimeString()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TMsqDBException(fParentQuery->sqlstr, MDB_ERR_DATA_TYPE_CONVERT, name, buffer_type, "AsDateTimeString()");
    }
    else
    {
        this->AsDateTimeInternal(year, month, day, hour, minute, second);
        if ( year == 0 )
            sprintf( (char *)fStrBuffer,"%s", MDB_NULL_STRING);
        else    
            sprintf( (char *)fStrBuffer,"%04d%02d%02d%02d%02d%02d", year, month, day, hour, minute,second);
        return (char *)fStrBuffer;
    }
}

/******************************************************************************
* ��������	:  AsDateTime()
* ��������	:  ���������ֶεĸ�������  
* ����		:  ��
* ���		:  year �꣬month �£�day �գ�hour Сʱ��minute ���ӣ�second ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void    TMsqDBField::AsDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TMsqDBException)
{
    if ( buffer_type != MYSQL_TYPE_TIME
         &&buffer_type != MYSQL_TYPE_DATE
         &&buffer_type != MYSQL_TYPE_DATETIME
         &&buffer_type != MYSQL_TYPE_TIMESTAMP )
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::AsDateTime()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TMsqDBException(fParentQuery->sqlstr, MDB_ERR_DATA_TYPE_CONVERT, name, buffer_type, "AsDateTime()");
    }
    else
	{
		this->AsDateTimeInternal(year, month, day, hour, minute, second);
	}
}

/******************************************************************************
* ��������	:  AsTimeT()
* ��������	:  ����time_tʱ������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ת�����ʱ��
* ����		:  li.shugang
*******************************************************************************/
time_t TMsqDBField::AsTimeT() throw (TMsqDBException)
{
    time_t tSeconds;
    static struct tm ts;    


    if ( buffer_type != MYSQL_TYPE_TIME
         &&buffer_type != MYSQL_TYPE_DATE
         &&buffer_type != MYSQL_TYPE_DATETIME
         &&buffer_type != MYSQL_TYPE_TIMESTAMP )
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::AsTimeT()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TMsqDBException(fParentQuery->sqlstr, MDB_ERR_DATA_TYPE_CONVERT, name, buffer_type, "AsTimeT()");
    }
    else if (isNULL())
    {
        return (time_t)0;
    }        
    else
    {
        this->AsDateTimeInternal(ts.tm_year, ts.tm_mon, ts.tm_mday, ts.tm_hour, ts.tm_min, ts.tm_sec);
        
        ts.tm_year -= 1900;
        ts.tm_mon  -= 1;
        ts.tm_isdst = (int)0;

        tSeconds = mktime(&ts);
        return tSeconds;
    }    
}

/******************************************************************************
* ��������	:  AsDateTimeInternal()
* ��������	:  �������ڵĸ�������,û��������У�飬ֻ���ڲ�����	  
* ����		:  ��
* ���		:  year �꣬month �£�day �գ�hour Сʱ��minute ���ӣ�second ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBField::AsDateTimeInternal(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TMsqDBException)
{
    if ( isNULL() )
    {
        year = 0;
        month = 0;
        day = 0;
        hour = 0;
        minute = 0;
        second = 0;
        return;
    }
    
    MYSQL_TIME*  ts = (MYSQL_TIME*)buffer;
    year = ts->year;
    month = ts->month;
    day = ts->day;
    hour = ts->hour;
    minute = ts->minute;
    second = ts->second;  
}

/******************************************************************************
* ��������	:  isNULL()
* ��������	:  ��������������� 
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��������
* ����		:  zhang.yu
*******************************************************************************/
long long TMsqDBField::AsInteger() throw (TMsqDBException)
{   
    long long iRet=0;
    if (buffer_type != MYSQL_TYPE_SHORT
        &&buffer_type != MYSQL_TYPE_LONG
        && buffer_type != MYSQL_TYPE_LONGLONG)
    {
		if(buffer_type == MYSQL_TYPE_FLOAT || buffer_type == MYSQL_TYPE_DOUBLE || buffer_type == MYSQL_TYPE_DECIMAL ||buffer_type == MYSQL_TYPE_NEWDECIMAL)
		{
			TADD_WARNING("Column data type is float, please check mysql.");
			//MYSQL_TYPE_FLOAT
            if(buffer_type == MYSQL_TYPE_FLOAT)
            {
                long lvalue = (long)AsFloat();
                return lvalue;
            }
            //MYSQL_TYPE_DOUBLE
            if(buffer_type == MYSQL_TYPE_DOUBLE)
            {
                long lvalue = (long)AsFloat();
                return lvalue;
            }
            //MYSQL_TYPE_DECIMAL
            if(buffer_type == MYSQL_TYPE_DECIMAL)
            {
                long lvalue = (long)AsFloat();
                return lvalue;
            }
            //MYSQL_TYPE_NEWDECIMAL
            if(buffer_type == MYSQL_TYPE_NEWDECIMAL)
            {
                long lvalue = (long)AsFloat();
                return lvalue;
            }
		}
		else
		{
			TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::AsInteger()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        	throw TMsqDBException(fParentQuery->sqlstr, MDB_ERR_DATA_TYPE_CONVERT, name, buffer_type, "AsInteger()");
		}
    }
    else 
    {
        if (isNULL())
        {
            return 0;
        }
        else
        {   
            //MYSQL_TYPE_SHORT
            if(buffer_type == MYSQL_TYPE_SHORT)
            {
                short int* sintvalue = (short int*)buffer;
                return *sintvalue;
            }
            //MYSQL_TYPE_LONG
            if(buffer_type == MYSQL_TYPE_LONG)
            {
                int* intvalue = (int*)buffer;
                return *intvalue;
            }
            //MYSQL_TYPE_LONGLONG
            else
            {
                long* lvalue = (long*)buffer;
                return *lvalue;
            }
        }
        return iRet;
    }
}

/******************************************************************************
* ��������	:  ClearDataBuf()
* ��������	:  ��ջ�����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBField::ClearDataBuf()
{
    if((buffer!=NULL) && (length>0))
    {
        memset(buffer,0,max_length+REDUNDANT_BIT_COUNT);
    }
    return;
}

/******************************************************************************
* ��������	:  GetFieldName()
* ��������	:  ��ȡ����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ����
* ����		:  li.shugang
*******************************************************************************/
char *TMsqDBField::GetFieldName()
{
    return name;
}

/******************************************************************************
* ��������	:  GetFieldType()
* ��������	:  ��ȡ������	
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��������
* ����		:  li.shugang
*******************************************************************************/
long TMsqDBField::GetFieldType()
{
    return buffer_type;
}

/******************************************************************************
* ��������	:  GetFieldSize()
* ��������	:  ��ȡ�д�С
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �д�С
* ����		:  li.shugang
*******************************************************************************/
long  TMsqDBField::GetFieldSize()
{
    return length;
}

/******************************************************************************
* ��������	:  GetFieldPrecision()
* ��������	:  ��ȡ�о���/����	
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �о���
* ����		:  li.shugang
*******************************************************************************/
int  TMsqDBField::GetFieldPrecision() 
{
    return length;
}


/****************** TMsqDBQuery implementation **************************/
TMsqDBQuery::TMsqDBQuery(TMsqDBDatabase *mysqldb) throw (TMsqDBException)
{
#ifdef __DEBUG__
    bExecuteFlag = false;
#endif

    if (! mysqldb->IsConnect())
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::TMsqDBQuery() construct function",__FILE__,__LINE__);
        throw TMsqDBException("", MDB_ERR_GENERAL, "TMsqDBQuery(TMsqDBDatabase &db): Can not declare a TMsqDBQuery when the database is not connected");
    }

    fOpened = false;
    fPrefetchRows = 1;

    nTransTimes = 0;
    iFieldCount = 0;
    iParamCount = 0;
    fActivated = false;
    db = mysqldb;
    sqlstr = NULL;
    iArrSize = 0;
    paramList = NULL;
    fieldList = NULL;

    bindparam = NULL;
    bindresult = NULL;
    stmt= NULL;
    prepare_meta_result = NULL;
}

TMsqDBQuery::~TMsqDBQuery()
{
    if (sqlstr != NULL)
    {
        delete[] sqlstr;
        sqlstr = NULL;
    }
    
    if (paramList != NULL)
    {
        for(int i=0;i<iParamCount;i++)
        {            
            if(paramList[i].name!=NULL)
            {
                delete [] paramList[i].name;
                paramList[i].name = NULL;
            }
            if(paramList[i].stringValue!=NULL)
            {
                delete [] paramList[i].stringValue;
                paramList[i].stringValue = NULL;
            }
            for(int k=0;k<MAX_DATA_COUNTS;k++)
            {
                if(paramList[i].stringArray[k]!=NULL)
                {
                    delete [] paramList[i].stringArray[k];
                    paramList[i].stringArray[k] = NULL;
                }
            }
        }
        
        delete[] paramList;
        paramList = NULL;
    }

    if (fieldList != NULL)
    {
        for(int i=0;i<iFieldCount;i++)
        {
            if(fieldList[i].name!=NULL)
            {
                delete [] fieldList[i].name;
                fieldList[i].name = NULL;
            }

            if(fieldList[i].buffer!=NULL)
            {
                delete [] (fieldList[i].buffer);
                fieldList[i].buffer = NULL;
            }
        }
    
        delete[] fieldList;
        fieldList = NULL;        
    }
    
    if (bindparam != NULL)
    {
        delete[] bindparam;
        bindparam = NULL;
    }
    
    if (bindresult != NULL)
    {
        delete[] bindresult;
        bindresult = NULL;
    }      

    if(prepare_meta_result != NULL)
    {
        mysql_free_result(prepare_meta_result);
        prepare_meta_result = NULL;
    }

    if(stmt != NULL)
    {
    	//�ͷ���ִ��Ԥ����������ɵĽ�����йص��ڴ档���ڸ���䣬������ڴ򿪵Ĺ�꣬mysql_stmt_free_result()���ر���������ɹ��ͷ��˽����������0��������ִ��󣬷��ط�0ֵ��
        if (mysql_stmt_free_result(stmt))
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Close()  mysql_stmt_free_result failed",__FILE__,__LINE__);
            fErrorNo = mysql_stmt_errno(stmt);
            throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Close()  mysql_stmt_free_result failed");
        }
        if(0 == mysql_ping(db->conn))
        {
	        if (mysql_stmt_close(stmt))
	        {
	            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Close()  mysql_stmt_close failed. Errorno[%d], errmsg[%s]",__FILE__,__LINE__, mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
	            fErrorNo = mysql_stmt_errno(stmt);
	            //throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Close()  mysql_stmt_close failed");
	            //��ֹmysql�������Ϊ��ʱ����ʱ�׳��쳣���»ָ������ж�
        	}
        }
        stmt = NULL;
    }

    iArrSize = 0;
    
    if(db)
    {
        db->Disconnect();
        delete db;
    }
}

/******************************************************************************
* ��������	:  Close()
* ��������	:  �ر�SQL��䣬��׼��������һ��sql���   
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::Close()
{    
    if (sqlstr != NULL)
    {
        delete[] sqlstr;
        sqlstr = NULL;
    }
    
    if (paramList != NULL)
    {
        for(int i=0;i<iParamCount;i++)
        {            
            if(paramList[i].name!=NULL)
            {
                delete [] paramList[i].name;
                paramList[i].name = NULL;
            }
            if(paramList[i].stringValue!=NULL)
            {
                delete [] paramList[i].stringValue;
                paramList[i].stringValue = NULL;
            }
            for(int k=0;k<MAX_DATA_COUNTS;k++)
            {
                if(paramList[i].stringArray[k]!=NULL)
                {
                    delete [] paramList[i].stringArray[k];
                    paramList[i].stringArray[k] = NULL;
                }
            }
        }
        
        delete[] paramList;
        paramList = NULL;
    }
    
    if (fieldList != NULL)
    {
        for(int i=0;i<iFieldCount;i++)
        {
            if(fieldList[i].name!=NULL)
            {
                delete [] fieldList[i].name;
                fieldList[i].name = NULL;
            }

            if(fieldList[i].buffer!=NULL)
            {
                delete [] (fieldList[i].buffer);
                fieldList[i].buffer = NULL;
            }
        }
    
        delete[] fieldList;
        fieldList = NULL;        
    }
    
    if (bindparam != NULL)
    {
        delete[] bindparam;
        bindparam = NULL;
    }
    
    if (bindresult != NULL)
    {
        delete[] bindresult;
        bindresult = NULL;
    }

    if(prepare_meta_result != NULL)
    {
        mysql_free_result(prepare_meta_result);
        prepare_meta_result = NULL;
    }      
     
    if(stmt != NULL)
    {
      
        if (mysql_stmt_free_result(stmt))
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Close()  mysql_stmt_free_result failed",__FILE__,__LINE__);
            fErrorNo = mysql_stmt_errno(stmt);
            throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Close()  mysql_stmt_free_result failed");
        }

        if(0 == mysql_ping(db->conn))
        {
	        if (mysql_stmt_close(stmt))
	        {
	            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Close()  mysql_stmt_close failed. Errorno[%d], errmsg[%s]",__FILE__,__LINE__, mysql_stmt_errno(stmt), mysql_stmt_error(stmt));
	            fErrorNo = mysql_stmt_errno(stmt);
	            //throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Close()  mysql_stmt_close failed");
	            //��ֹmysql�������Ϊ��ʱ����ʱ�׳��쳣���»ָ������ж�
        	}
        }

        stmt = NULL;
    }
   
    iFieldCount = 0;
    iParamCount = 0;
    iAffectedRows = 0;
    iArrSize = 0;
    fActivated = false; 

    fOpened = false;
    fPrefetchRows = MDB_PREFETCH_ROWS;
}

/******************************************************************************
* ��������	:  Commit()
* ��������	:  �����ύ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true �ύ�ɹ���false �ύʧ��
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBQuery::Commit()
{
#ifdef __DEBUG__
    bExecuteFlag = false;
#endif

    if(0 != mysql_commit(db->conn))
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Commit()  mysql commit failed. errmsg:%s",__FILE__,__LINE__,mysql_error(db->conn));
        throw TMsqDBException(mysql_errno(db->conn), mysql_error(db->conn),"TMsqDBQuery Commit failed.");
        return false;
    }
    else
    {
        return true;  
    }    
}

/******************************************************************************
* ��������	:  Rollback()
* ��������	:  ����ع�
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true �ع��ɹ���false �ع�ʧ��
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBQuery::Rollback()
{
#ifdef __DEBUG__
    bExecuteFlag = false;
#endif

    if(0 != mysql_rollback(db->conn))
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Rollback()  mysql rollback failed.",__FILE__,__LINE__);
        throw TMsqDBException(mysql_errno(db->conn), mysql_error(db->conn),"TMsqDBQuery rollback failed.");
        return false;
    }
    else
    {
        return true;  
    } 
}


void TMsqDBQuery::GetFieldsDef() throw (TMsqDBException)
{    
    if (NULL != fieldList)
    {
        delete[] fieldList;
        fieldList = NULL;
    }
    
    fieldList = new(std::nothrow) TMsqDBField[iFieldCount];
	if(fieldList == NULL)
	{
		MYSQL_ERROR_TO_THROW("TMsqDBQuery::GetFieldsDef()", "Memory allocate failed");
	}
    MYSQL_FIELD * field = NULL;
    int len = 0;
    //��ȡ�ֶ�����
    for (int i=0;i<iFieldCount;i++ ) 
    {
        if(NULL != fieldList[i].name)
        {
            delete [] (fieldList[i].name);
            fieldList[i].name = NULL;
        }

        if(NULL != fieldList[i].buffer)
        {
            delete [] (fieldList[i].buffer);
            fieldList[i].buffer = NULL;
        }

		//mysql bug
		fieldList[i].fParentQuery =  this;
		
        field = mysql_fetch_field(prepare_meta_result);

        len = strlen(field->name);
        fieldList[i].name =  new(std::nothrow) char[len+1]; 
		if(fieldList[i].name == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBQuery::GetFieldsDef()", "Memory allocate failed");
		}
        strncpy(fieldList[i].name,field->name,len);
        fieldList[i].name[len]='\0';
       
        fieldList[i].buffer_type = field->type;
        
        fieldList[i].max_length = field->length;        
        
        if(fieldList[i].max_length > 0)
        {        
            //��������λ�û�������һ�㣬����������ʱ������delete [] buffer ��core����
            fieldList[i].buffer = new(std::nothrow) char[fieldList[i].max_length + REDUNDANT_BIT_COUNT];
			if(fieldList[i].buffer == NULL)
			{
				MYSQL_ERROR_TO_THROW("TMsqDBQuery::GetFieldsDef()", "Memory allocate failed");
			}
            int j = 0;
    		for(; j < (fieldList[i].max_length + REDUNDANT_BIT_COUNT)/4294967295; j++)
    		{
    			memset(fieldList[i].buffer+j*4294967295,0,4294967295);
    		}
    		memset(fieldList[i].buffer,0,fieldList[i].max_length + REDUNDANT_BIT_COUNT - 4294967295*j);
        }        
    }
}

/******************************************************************************
* ��������	:  GetParamsDef()
* ��������	:  ��ʼ���������飬���޸�sql���������mysql
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true �ع��ɹ���false �ع�ʧ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::GetParamsDef() throw (TMsqDBException)
{
    char *params[MDB_MAX_PARAMS_COUNT];
    int i, in_literal, n, nParamLen,nFlag = 0;
    char *cp,*ph;
    char *sql = NULL;

    int nLen = strlen(this->sqlstr);
    sql = new(std::nothrow) char[nLen+1];
	if(sql == NULL)
	{
		MYSQL_ERROR_TO_THROW("TMsqDBQuery::GetParamsDef()", "Memory allocate failed");
	}
    strcpy(sql, this->sqlstr);
    sql[nLen] = '\0';

    if (iParamCount>0)
    {
        delete[] paramList;
        paramList=NULL;
    }

    // Find and bind input variables for placeholders. 
    for (i = 0, in_literal = false, cp = sql; *cp != 0; cp++)
    {
        if (*cp == '\'')
            in_literal = ~in_literal;
        if (*cp == ':' && *(cp+1) != '=' && !in_literal)
        {

            for ( ph = ++cp, n = 0;  *cp && (isalnum(*cp) || *cp == '_'); cp++, n++);
            if(*cp == 0) 
                nFlag = 1;
            else 
                *cp = 0;

            if ( i > (int)MDB_MAX_PARAMS_COUNT)
            {
                TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::GetParamsDef()   param count execedes max numbers, please refer to OCIQuery.h",__FILE__,__LINE__);
                throw TMsqDBException(sqlstr, MDB_ERR_CAPABILITY_NOT_YET_SUPPORT, " param count execedes max numbers, please refer to OCIQuery.h");
            }
            nParamLen = strlen((char *)ph);

            params[i] = new(std::nothrow) char[nParamLen+1];
			if(params[i] == NULL)
			{
				MYSQL_ERROR_TO_THROW("TMsqDBQuery::GetParamsDef()", "Memory allocate failed");
			}
            strcpy(params[i],(char *)ph);
            params[i][nParamLen] = '\0';
            i++;
            if(nFlag == 1) break;
        }   
    }
    delete[] sql;

    iParamCount = i;
    
    if (iParamCount>0)
    {
        paramList = new(std::nothrow) TMsqDBParam[iParamCount];
		if(paramList == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBQuery::GetParamsDef()", "Memory allocate failed");
		}

        for (i=0; i<iParamCount; i++)
        {
            nParamLen = strlen(params[i]);
            if(NULL != paramList[i].name)
            {
                delete [] (paramList[i].name);
                paramList[i].name = NULL;
            }            
            paramList[i].name = new(std::nothrow) char[nParamLen+1];
			if(paramList[i].name == NULL)
			{
				MYSQL_ERROR_TO_THROW("TMsqDBQuery::GetParamsDef()", "Memory allocate failed");
			}
            strncpy(paramList[i].name, params[i], nParamLen);
            paramList[i].name[nParamLen] = '\0';
            delete [] params[i];
        }
    }    
}

/******************************************************************************
* ��������	:  GetParamsDef()
* ��������	:  ��ʼ���������飬���޸�sql���������mysql
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true �ع��ɹ���false �ع�ʧ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::FormatSqlStr() throw (TMsqDBException)
{
    //printf("--HH--SRCSQL:%s\n",sqlstr);
    //�޸�sql��䣬��:V �滻Ϊ ?  ת��Ϊ����mysql���﷨�����
    size_t iNumCh = 0; //���������ַ��ĸ���
	char *pCh=NULL,*pPlaceholder=NULL;
    int in_literal, nFlag = 0;
	//����m_pSqlStmt����Ҫ�󶨵ı���ʹ��ռλ��'?'��� ;��������Ӧ���ֶ�������m_pTParmList������
	for (in_literal = false, pCh = sqlstr; *pCh != 0; pCh++)
	{
		if (*pCh == '\'')
		{
			in_literal = ~in_literal;
		}
		if (*pCh == ':' && *(pCh+1) != '=' && !in_literal)
		{
			*pCh = '?';
			for ( iNumCh = 0, pPlaceholder = ++pCh ;  *pCh && (isalnum(*pCh) || *pCh == '_'); pCh++, iNumCh++);
			if(*pCh == 0)
			{
				nFlag = 1;
			}                   
			memset(pPlaceholder,' ',iNumCh);

			if(nFlag == 1)
			{
				break;
			}
		}
	}
    //printf("--HH--MIDSQL:%s\n",sqlstr);
    
    char TmpStrA[MAX_SQL_LEN] = "";
    char TmpStrB[MAX_SQL_LEN] = "";
    TMdbNtcStrFunc::Replace(sqlstr, "NVL", "IFNULL", TmpStrA, false);
    //TMdbNtcStrFunc::Replace(TmpStrA, "TO_DATE", "DATE_FORMAT", TmpStrB, false);
    TMdbNtcStrFunc::Replace(TmpStrA, "TO_DATE", "STR_TO_DATE", TmpStrB, false);
    memset(TmpStrA,0,MAX_SQL_LEN);
	TMdbNtcStrFunc::Replace(TmpStrB, "TO_CHAR","CONCAT", TmpStrA, false);
    memset(TmpStrB,0,MAX_SQL_LEN);
    TMdbNtcStrFunc::Replace(TmpStrA, "YYYYMMDDHH24MISS", "%Y%m%d%H%i%s", TmpStrB, false);
    memset(TmpStrA,0,MAX_SQL_LEN);
    TMdbNtcStrFunc::Replace(TmpStrB, "YYYY-MM-DD HH24:MI:SS","%Y%m%d%H%i%s", TmpStrA, false);
    memset(TmpStrB,0,MAX_SQL_LEN);
    TMdbNtcStrFunc::Replace(TmpStrA, "SYSDATE", "SYSDATE()", TmpStrB, false);
    memset(TmpStrA,0,MAX_SQL_LEN);

    //printf("--HH--DESTSQL:%s\n",sqlstr);
    

    //����format֮���sql���
    delete[] sqlstr;
    sqlstr = NULL;

    int nLen = strlen(TmpStrB);
    sqlstr = new(std::nothrow) char[nLen + 1];
	if(sqlstr == NULL)
	{
		MYSQL_ERROR_TO_THROW("TMsqDBQuery::FormatSqlStr()", "Memory allocate failed");
	}

    strncpy(sqlstr,TmpStrB,nLen);
    sqlstr[nLen] = '\0';    
}

/******************************************************************************
* ��������	:  SetSQL()
* ��������	:  ����Sqlstatement   
* ����		:  inSqlstmt sql���
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetSQL(const char *inSqlstmt,int iPreFetchRows) throw (TMsqDBException)
{     
    fErrorNo = 0;
    if (!db->IsConnect())
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::SetSQL()  can't set sqlstmt on disconnected Database",__FILE__,__LINE__);
        throw TMsqDBException(inSqlstmt, MDB_ERR_GENERAL, "SetSQL(): can't set sqlstmt on disconnected Database");
    }
    if (fActivated)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::SetSQL()  can't set sqlstmt on opened state",__FILE__,__LINE__);
        throw TMsqDBException(inSqlstmt, MDB_ERR_GENERAL, "SetSQL(): can't set sqlstmt on opened state");
    }
    fOpened = false;

    //����sql���
    if (sqlstr != NULL)
        delete[] sqlstr;
    int nLen = strlen(inSqlstmt);
    sqlstr = new(std::nothrow) char[nLen + 1];
    if (sqlstr == NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::SetSQL()  MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED",__FILE__,__LINE__);
        fErrorNo = -1;
        throw TMsqDBException(inSqlstmt, MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED, "SetSQL()", __LINE__);
    }
    strncpy(sqlstr,inSqlstmt,nLen);
    sqlstr[nLen] = '\0';    
    
    //��ȡ����������׼���������顣
    GetParamsDef();
    //��ʽ��sql��䣬sql���Ϊ������mysql
    FormatSqlStr();
    //��ʼ�� MYSQL_STMT    
    stmt = mysql_stmt_init(db->conn);
    if (!stmt)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::SetSQL()   mysql_stmt_init failed",__FILE__,__LINE__);
        fErrorNo = mysql_stmt_errno(stmt);
        throw TMsqDBException( mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "mysql_stmt_init failed"); 
    }
    //TADD_NORMAL("[%s:%d] TMsqDBQuery::SetSQL()  sqlstr:%s",__FILE__,__LINE__,sqlstr);    
        
    //�����޸ĺ��sql�����ܷ���mysql_stmt_prepare���鿴��������
    if (mysql_stmt_prepare(stmt, sqlstr, strlen(sqlstr)))
    {    
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::SetSQL()   mysql_stmt_prepare field, errmsg[%s], sqlstr[%s]",__FILE__,__LINE__,mysql_stmt_error(stmt),sqlstr);
        fErrorNo = mysql_stmt_errno(stmt);
        throw TMsqDBException( mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "mysql_stmt_prepare field"); 
    }
    //�鿴��������ֶθ���
    prepare_meta_result = mysql_stmt_result_metadata(stmt);
    if (NULL == prepare_meta_result)
    {
        isSelectSql = false;
        return;
    }
    isSelectSql = true;

    iFieldCount = mysql_num_fields(prepare_meta_result);
    //GetFieldsDef();  

    fActivated = true;
    
    CheckError();
}

TMsqDBParam *TMsqDBQuery::ParamByName(const char *paramName) throw (TMsqDBException)
{
    TMsqDBParam *para = NULL;
    bool found = false;
    int i;

    if (sqlstr == NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::ParamByName()  sql statement is empty",__FILE__,__LINE__);
        throw TMsqDBException(paramName, MDB_ERR_GENERAL, "ParamByName(): sql statement is empty.");
    }

    //printf("--HH--iParamCount[%d]---\n",iParamCount);
    for(i=0; i<iParamCount; i++)
    {
        //printf("--HH--Param(i).name[%s]---paramName[%s]---\n",Param(i).name,paramName);
        found = CompareStrNoCase(Param(i).name,paramName);
        
        if ( found )
            break;
    }
    if ( found ) 
    {
        para = &paramList[i];
    }        
    else 
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::ParamByName()  MDB_ERR_PARAM_NOT_EXISTS, paramName[%s]  \nsql[%s]",__FILE__,__LINE__,paramName,sqlstr);
        throw TMsqDBException(sqlstr, MDB_ERR_PARAM_NOT_EXISTS, paramName);
    }
    return para;
}

/******************************************************************************
* ��������	:  IsParamExist()
* ��������	:  �ж�ָ�������Ƿ����   
* ����		:  paramName ������
* ���		:  ��
* ����ֵ	:  true ���ڣ�false ������
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBQuery::IsParamExist(const char *paramName)
{
    for(int i=0; i<iParamCount; i++)
    {
        if (CompareStrNoCase(Param(i).name,paramName))  return true;
    }
    return false;
}

/******************************************************************************
* ��������	:  SetParameterNULL()
* ��������	:  ����NULL����   
* ����		:  paramName ������
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParameterNULL(const char *paramName) throw (TMsqDBException)
{
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣
    if(para == NULL)
    {
        return;
    }
    
    para->dataType = MYSQL_TYPE_NULL;
    para->is_null[0] = true;
}

/******************************************************************************
* ��������	:  SetParameter()
* ��������	:  ����double����	
* ����		:  paramName ��������paramValue ����ֵ��isOutput 
*			:  �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParameter(const char *paramName, double paramValue, bool isOutput ) throw (TMsqDBException)
{
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣
    if(para == NULL)
    {
        return;
    }
    para->fIsOutput = isOutput;
    para->dataType = MYSQL_TYPE_DOUBLE;
    para->dblValue = paramValue;
    para->is_null[0] = false;
}

/******************************************************************************
* ��������	:  SetParameter()
* ��������	:  ����long����
* ����		:  paramName ��������paramValue ����ֵ��isOutput 
*			:  �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParameter(const char *paramName, long paramValue, bool isOutput) throw (TMsqDBException)
{
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    if(para == NULL)
    {
        return;
    }
    
    para->fIsOutput = isOutput;
    para->dataType = MYSQL_TYPE_LONGLONG;
    para->longValue = paramValue;
    para->is_null[0] = false;
}

/******************************************************************************
* ��������	:  SetParameter()
* ��������	:  ����long long����	
* ����		:  paramName ��������paramValue ����ֵ��isOutput 
*			:  �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParameter(const char *paramName, long long paramValue, bool isOutput) throw (TMsqDBException)
{
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    if(para == NULL)
    {
        return;
    }
    
    para->fIsOutput = isOutput;
    para->dataType = MYSQL_TYPE_LONGLONG;
    para->longValue = paramValue;
    para->is_null[0] = false;
}

/******************************************************************************
* ��������	:  SetParameter()
* ��������	:  ����int���Ͳ���	
* ����		:  paramName ��������paramValue ����ֵ��isOutput 
*			:  �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParameter(const char *paramName, int paramValue, bool isOutput ) throw (TMsqDBException)
{
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣
    
    if(para == NULL)
    {   
        return;
    }
    
    para->fIsOutput = isOutput;
    para->dataType = MYSQL_TYPE_LONG;
    para->intValue = paramValue;
    para->is_null[0] = false;
}

/******************************************************************************
* ��������	:  SetParameter()
* ��������	:  �����ַ����Ͳ���	
* ����		:  paramName ��������paramValue ����ֵ��isOutput 
*			:  �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  fu.wenjun
*******************************************************************************/
void TMsqDBQuery::SetParameter(const char *paramName, const char paramValue, bool isOutput ) throw (TMsqDBException)
{    
    char strparamvalue[2];
	memset(strparamvalue,0,sizeof(strparamvalue));
    strparamvalue[0] = paramValue ;
    strparamvalue[1] = '\0';
    SetParameter(paramName,strparamvalue,isOutput);       
}

/******************************************************************************
* ��������	:  SetParameter()
* ��������	:  �����ַ������Ͳ���	
* ����		:  paramName ��������paramValue ����ֵ��isOutput 
*			:  �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParameter(const char *paramName, const char* paramValue, bool isOutput ) throw (TMsqDBException)
{
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣
    
    if(para == NULL)
    {        
        return;
    }

    para->fIsOutput = isOutput;
    para->dataType = MYSQL_TYPE_STRING;
    
    if (para->stringValue)
        delete[] para->stringValue;

    int nLen;

    if (isOutput)
    { 
        para->stringValue = new(std::nothrow) char[MDB_MAX_STRING_VALUE_LENGTH];
		if(para->stringValue == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBQuery::SetParameter()", "Memory allocate failed");
		}
        memset(para->stringValue,0,MDB_MAX_STRING_VALUE_LENGTH);
        para->string_length = MDB_MAX_STRING_VALUE_LENGTH;
        para->length = 0;
        para->is_null[0] = false;             
    }
    else 
    {
        if(paramValue == NULL)
        {
            para->stringValue = NULL;
            para->string_length = 0;
            para->length  = 0; 
            para->is_null[0] = true;         
        }
        else
        {
            nLen = strlen(paramValue);
            para->stringValue = new(std::nothrow) char[nLen+1];
			if(para->stringValue == NULL)
			{
				MYSQL_ERROR_TO_THROW("TMsqDBQuery::SetParameter()", "Memory allocate failed");
			}
            strncpy((char *)para->stringValue,paramValue,nLen);
            para->stringValue[nLen] = '\0';            
            para->string_length = nLen+1;
            para->length  = nLen; 
            para->is_null[0] = false;
        }
    }
}

/******************************************************************************
* ��������	:  SetParameter()
* ��������	:  �����ַ������Ͳ���	
* ����		:  paramName ��������paramValue ����ֵ��isOutput 
*			:  �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParameter(const char *paramName,const char* paramValue,int iBufferLen,bool isOutput) throw (TMsqDBException)
{
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = MYSQL_TYPE_BLOB;
    
    if (para->stringValue)
        delete[] para->stringValue;
    
    if (isOutput)
    {
        para->stringValue = new(std::nothrow) char[iBufferLen];
		if(para->stringValue == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBQuery::SetParameter()", "Memory allocate failed");
		}
        memset(para->stringValue,0,iBufferLen);
        para->string_length = iBufferLen;
        para->length = iBufferLen;
        para->is_null[0] = false;              
    }
    else 
    {
        if(paramValue == NULL || iBufferLen == 0)
        {
            para->stringValue = NULL;
            para->string_length = 0;
            para->length  = 0; 
            para->is_null[0] = true;
        }
        else
        {             
            para->stringValue = new(std::nothrow) char[iBufferLen];
			if(para->stringValue == NULL)
			{
				MYSQL_ERROR_TO_THROW("TMsqDBQuery::SetParameter()", "Memory allocate failed");
			}
            memcpy(para->stringValue,paramValue,iBufferLen);
            para->string_length = iBufferLen;
            para->length  = iBufferLen; 
            para->is_null[0] = false; 
        }
    }
}

/******************************************************************************
* ��������	:  SetParamArray()
* ��������	:  �����������  
* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
*			:  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParamArray(const char *paramName, char ** paramValue,int iStructSize,int iStrSize,int iArraySize,short* iIsNull,bool isOutput) throw (TMsqDBException)
{
    if(iArrSize == 0)
    {
        iArrSize = iArraySize/iStructSize;
    }
    else
    {
        if(iArrSize != iArraySize/iStructSize)
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::SetParamArray()  iArrSize[%d] doesn't match iArrSize before[%d]",__FILE__,__LINE__,iArraySize/iStructSize,iArraySize);
            throw TMsqDBException("TMsqDBQuery::SetParamArray","iArrSize[%d] doesn't match iArrSize before[%d]",iArraySize/iStructSize,iArraySize);
            return;
        }
    }

    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣
    para->fIsOutput = isOutput;
    para->dataType = MYSQL_TYPE_STRING;
	for(int i = 0; i < iArrSize; i++)
	{
		if(iIsNull[i] == 0) 
		{
			para->is_null[i] = false; 
		}
		else 
		{
			para->is_null[i] = true;
		}
	}
	
    for(int i = 0;i<MAX_DATA_COUNTS;i++)
    {
        para->stringArray[i] = new(std::nothrow) char[iStructSize+1];
		if(para->stringArray[i] == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBQuery::SetParamArray()", "Memory allocate failed");
		}
        strncpy((char *)para->stringArray[i], (char*)paramValue+(i*iStructSize), iStructSize); 
        para->stringArray[i][iStructSize]='\0';
    }
    para->string_length =iStructSize+1;
    para->length=iStructSize;
}

/******************************************************************************
* ��������	:  SetParamArray()
* ��������	:  �����������  
* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
*			:  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParamArray(const char *paramName, int * paramValue, int iStructSize,int iArraySize, short* iIsNull,bool isOutput ) throw (TMsqDBException)
{
    if(iArrSize == 0)
    {
        iArrSize = iArraySize/iStructSize;
    }
    else
    {
        if(iArrSize != iArraySize/iStructSize)
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::SetParamArray()  iArrSize[%d] doesn't match iArrSize before[%d]",__FILE__,__LINE__,iArraySize/iStructSize,iArraySize);
            throw TMsqDBException("TMsqDBQuery::SetParamArray","iArrSize[%d] doesn't match iArrSize before[%d]",iArraySize/iStructSize,iArraySize);
            return;
        }
    }
    
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = MYSQL_TYPE_LONG;
	for(int i = 0; i < iArrSize; i++)
	{
		if(iIsNull[i] == 0) 
		{
			para->is_null[i] = false; 
		}
		else 
		{
			para->is_null[i] = true;
		}
	}
    para->intArray = paramValue;
}

/******************************************************************************
* ��������	:  SetParamArray()
* ��������	:  �����������  
* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
*			:  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParamArray(const char *paramName, double * paramValue,int iStructSize, int iArraySize,short* iIsNull,bool isOutput)  throw (TMsqDBException)
{
    if(iArrSize == 0)
    {
        iArrSize = iArraySize/iStructSize;
    }
    else
    {
        if(iArrSize != iArraySize/iStructSize)
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::SetParamArray()  iArrSize[%d] doesn't match iArrSize before[%d]",__FILE__,__LINE__,iArraySize/iStructSize,iArraySize);
            throw TMsqDBException("TMsqDBQuery::SetParamArray","iArrSize[%d] doesn't match iArrSize before[%d]",iArraySize/iStructSize,iArraySize);
            return;
        }
    }
    
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = MYSQL_TYPE_DOUBLE;
	for(int i = 0; i < iArrSize; i++)
	{
		if(iIsNull[i] == 0) 
		{
			para->is_null[i] = false; 
		}
		else 
		{
			para->is_null[i] = true;
		}
	}
    para->dblArray = paramValue;
}
/******************************************************************************
* ��������	:  SetParamArray()
* ��������	:  �����������  
* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
*			:  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParamArray(const char *paramName, long * paramValue, int iStructSize,int iArraySize, short* iIsNull,bool isOutput ) throw (TMsqDBException)
{
    if(iArrSize == 0)
    {
        iArrSize = iArraySize/iStructSize;
    }
    else
    {
        if(iArrSize != iArraySize/iStructSize)
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::SetParamArray()  iArrSize[%d] doesn't match iArrSize before[%d]",__FILE__,__LINE__,iArraySize/iStructSize,iArraySize);
            throw TMsqDBException("TMsqDBQuery::SetParamArray","iArrSize[%d] doesn't match iArrSize before[%d]",iArraySize/iStructSize,iArraySize);
            return;
        }
    }
    
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = MYSQL_TYPE_LONGLONG;
	for(int i = 0; i < iArrSize; i++)
	{
		if(iIsNull[i] == 0) 
		{
			para->is_null[i] = false; 
		}
		else 
		{
			para->is_null[i] = true;
		}
	}
    para->longArray = paramValue;
}

/******************************************************************************
* ��������	:  SetParamArray()
* ��������	:  �����������  
* ����		:  paramName ��������paramValue ����ֵ��iStructSize iStrSize 
*			:  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetParamArray(const char *paramName, long long * paramValue, int iStructSize,int iArraySize, short* iIsNull,bool isOutput ) throw (TMsqDBException)
{
    if(iArrSize == 0)
    {
        iArrSize = iArraySize/iStructSize;
    }
    else
    {
        if(iArrSize != iArraySize/iStructSize)
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::SetParamArray()  iArrSize[%d] doesn't match iArrSize before[%d]",__FILE__,__LINE__,iArraySize/iStructSize,iArraySize);
            throw TMsqDBException("TMsqDBQuery::SetParamArray","iArrSize[%d] doesn't match iArrSize before[%d]",iArraySize/iStructSize,iArraySize);
            return;
        }
    }
    
    TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
	para->dataType = MYSQL_TYPE_LONGLONG;
	for(int i = 0; i < iArrSize; i++)
	{
		if(iIsNull[i] == 0) 
		{
			para->is_null[i] = false; 
		}
		else 
		{
			para->is_null[i] = true;
		}
	}
	para->llongArray = paramValue;
	
}

/******************************************************************************
* ��������	:  SetBlobParamArray()
* ��������	:  �����������  
* ����		:  paramName ��������paramValue ����ֵ��iBufferLen iStrSize 
*			:  iArraySize �����С��isOutput �Ƿ��������(Ĭ��false)
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::SetBlobParamArray(const char *paramName,char *paramValue,int iBufferLen,int iArraySize,short* iIsNull,bool isOutput) throw (TMsqDBException)
{
    if(iArrSize == 0)
    {
        iArrSize = iArraySize/iBufferLen;
    }
    else
    {
        if(iArrSize != iArraySize/iBufferLen)
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::SetParamArray()  iArrSize[%d] doesn't match iArrSize before[%d]",__FILE__,__LINE__,iArraySize/iBufferLen,iArraySize);
            throw TMsqDBException("TMsqDBQuery::SetParamArray","iArrSize[%d] doesn't match iArrSize before[%d]",iArraySize/iBufferLen,iArraySize);
            return;
        }
    }
    //TMsqDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    //para->fIsOutput = isOutput;
    //para->dataType = MYSQL_TYPE_BLOB;
    //para->stringArray =(char**) paramValue;
    //para->stringSize = iBufferLen;
    TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::SetParamArray()  mysql doesn't not support batch operation",__FILE__,__LINE__);
    throw TMsqDBException("TMsqDBQuery::SetParamArray","mysql doesn't not support batch operation");
}

void TMsqDBQuery::CheckError(const char* sSql) throw (TMsqDBException)
{
    if (fErrorNo != 0) 
    {
        throw TMsqDBException(fErrorNo, "TMsqDBQuery check error failed", sqlstr);
    }
}

/******************************************************************************
* ��������	:  Execute()
* ��������	:  ִ�з�SELECT���,û�з��ؽ����
* ����		:  iters ָ��sql����ִ�д���
* ���		:  ��
* ����ֵ	:  true �ɹ�;false ʧ��
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBQuery::Execute(int iters) throw (TMsqDBException)
{
    if (bindparam != NULL)
    {
        delete[] bindparam;
        bindparam = NULL;
    }
    
    if(iArrSize == 0)
    {
        //������
        if(iParamCount>0)
        {   
            bindparam = new(std::nothrow) MYSQL_BIND[iParamCount];
			if(bindparam == NULL)
			{
				MYSQL_ERROR_TO_THROW("TMsqDBQuery::Execute()", "Memory allocate failed");
			}
            memset(bindparam, 0, sizeof(MYSQL_BIND)*iParamCount);

            for(int i=0;i<iParamCount;i++)
            {                
                bindparam[i].buffer_length= paramList[i].string_length;
                bindparam[i].length= &paramList[i].length;
                bindparam[i].is_null= &paramList[i].is_null[0];
                bindparam[i].buffer_type= paramList[i].dataType;
                switch ( bindparam[i].buffer_type )
                {
                    case MYSQL_TYPE_DOUBLE:
                    {
                        bindparam[i].buffer = (char *)&paramList[i].dblValue;
                        break;
                    }                
                    case MYSQL_TYPE_LONGLONG:
                    {
                        bindparam[i].buffer = (char *)&paramList[i].longValue;
                        break;
                    }
                    case MYSQL_TYPE_LONG:
                    {
                        bindparam[i].buffer = (char *)&paramList[i].intValue;
                        break;
                    }
                    case MYSQL_TYPE_STRING:
                    {
                        bindparam[i].buffer = (char *)paramList[i].stringValue;
                        break;
                    }
                    case MYSQL_TYPE_BLOB:
                    {
                        bindparam[i].buffer = (char *)paramList[i].stringValue;
                        break;
                    }            
					case MYSQL_TYPE_NULL:
					{
						//bindparam[i].buffer = (char *)paramList[i].stringValue;
						bindparam[i].buffer = NULL;
						break;
					}
                    default:
                    {
                        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Execute()  paramList[%d].dataType[%d] is wrong",__FILE__,__LINE__,i,paramList[i].dataType);
                        fErrorNo = -1;
                        return false;
                    }
                }
            }
            
            if (mysql_stmt_bind_param(stmt, bindparam))
            {
                if (bindparam != NULL)
                {
                    delete[] bindparam;
                    bindparam = NULL;
                }
                TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Execute()  mysql_stmt_bind_param failed",__FILE__,__LINE__);
                fErrorNo = mysql_stmt_errno(stmt);
                throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Open()  mysql_stmt_bind_param failed");
                return false;
            }
        }
        
        if (mysql_stmt_execute(stmt))
        {
            if (bindparam != NULL)
            {
                delete[] bindparam;
                bindparam = NULL;
            }
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Execute()  mysql_stmt_execute failed, errmsg[%s]\nSQL:[%s]",__FILE__,__LINE__,mysql_stmt_error(stmt),sqlstr);
            fErrorNo = mysql_stmt_errno(stmt);
            throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Execute()  mysql_stmt_execute failed");
            return false;
        }

        if (bindparam != NULL)
        {
            delete[] bindparam;
            bindparam = NULL;
        }        
    }
    else
    {
        for(int k=0;k<iArrSize;k++)
        {            
            //������
            if(iParamCount>0)
            {                
                bindparam = new(std::nothrow) MYSQL_BIND[iParamCount];
				if(bindparam == NULL)
				{
					MYSQL_ERROR_TO_THROW("TMsqDBQuery::Execute()", "Memory allocate failed");
				}
                memset(bindparam, 0, sizeof(MYSQL_BIND)*iParamCount);

                //printf("--HH--param count[%d]--\n",iParamCount);
                for(int i=0;i<iParamCount;i++)
                {
                    //printf("\n\n--HH--buffer_type[%d]--\n",paramList[i].dataType);
                    bindparam[i].buffer_length= paramList[i].string_length;
                    bindparam[i].length= &paramList[i].length;
                    bindparam[i].is_null= &paramList[i].is_null[k];
                    bindparam[i].buffer_type= paramList[i].dataType;
                    switch ( bindparam[i].buffer_type )
                    {
                        case MYSQL_TYPE_DOUBLE:
                        {
                            bindparam[i].buffer = (char *)&paramList[i].dblArray[k];
                            //printf("--HH--setPara DOUBLE--i[%d] k[%d]---[%lf] \n",i,k,paramList[i].dblArray[k]);
                            break;
                        }                
                        case MYSQL_TYPE_LONGLONG:
                        {
                            bindparam[i].buffer = (char *)&paramList[i].llongArray[k];
                            //printf("--HH--setPara LONGLONG--i[%d] k[%d]---[%lld] \n",i,k,paramList[i].llongArray[k]);
                            break;
                        }
                        case MYSQL_TYPE_LONG:
                        {
                            bindparam[i].buffer = (char *)&paramList[i].intArray[k];
                            //printf("--HH--setPara LONG--i[%d] k[%d]---[%d] \n",i,k,paramList[i].intArray[k]);
                            break;
                        }
                        case MYSQL_TYPE_STRING:
                        {                              
                            bindparam[i].buffer = (char *)paramList[i].stringArray[k];
                            paramList[i].length = strlen(paramList[i].stringArray[k]);
                            //printf("--HH--setPara STRING--i[%d] k[%d] [%s] length[%d] \n",i,k,paramList[i].stringArray[k],strlen(paramList[i].stringArray[k]));
                            break;
                        }
                        case MYSQL_TYPE_BLOB:
                        {
                            bindparam[i].buffer = (char *)paramList[i].stringArray[k];
                            //printf("--HH--setPara BLOB--i[%d] k[%d]---[%s] \n",i,k,paramList[i].stringArray[k]);
                            break;
                        }
                        default:
                        {
                            //printf("--HH--buffer_type[default]--\n");
                            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Execute()  paramList[%d].dataType[%d] is wrong",__FILE__,__LINE__,i,paramList[i].dataType);
                            fErrorNo = -1;
                            return false;
                        }
                    }
                }
                
                if (mysql_stmt_bind_param(stmt, bindparam))
                {
                    if (bindparam != NULL)
                    {
                        delete[] bindparam;
                        bindparam = NULL;
                    }
                    TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Execute()  mysql_stmt_bind_param failed",__FILE__,__LINE__);
                    fErrorNo = mysql_stmt_errno(stmt);
                    throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Open()  mysql_stmt_bind_param failed");
                    return false;
                }
            }
            
            if (mysql_stmt_execute(stmt))
            {
                if (bindparam != NULL)
                {
                    delete[] bindparam;
                    bindparam = NULL;
                }
                TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Execute()  mysql_stmt_execute failed, errmsg[%s]",__FILE__,__LINE__,mysql_stmt_error(stmt));
                fErrorNo = mysql_stmt_errno(stmt);
                throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Execute()  mysql_stmt_execute failed");
                return false;
            }

            if (bindparam != NULL)
            {
                delete[] bindparam;
                bindparam = NULL;
            }
        }        
    }

    return true;    
}


/******************************************************************************
* ��������	:  Open()
* ��������	:  ��SQL SELECT��䷵�ؽ���� 
* ����		:  prefetchRows Ԥ����ǰ��¼����Ĭ��200��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBQuery::Open(int prefetch_Row) throw (TMsqDBException)
{
    if (bindparam != NULL)
    {
        delete[] bindparam;
        bindparam = NULL;
    }
    
    //������
    if(iParamCount>0)
    {   
        bindparam = new(std::nothrow) MYSQL_BIND[iParamCount];
		if(bindparam == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBQuery::Open()", "Memory allocate failed");
		}
        memset(bindparam, 0, sizeof(MYSQL_BIND)*iParamCount);        

        for(int i=0;i<iParamCount;i++)
        {
            bindparam[i].buffer_length= paramList[i].string_length;
            bindparam[i].length= &paramList[i].length;
            bindparam[i].is_null= &paramList[i].is_null[0];
            bindparam[i].buffer_type= paramList[i].dataType;

            switch ( bindparam[i].buffer_type )
            {
                case MYSQL_TYPE_DOUBLE:
                {
                    bindparam[i].buffer = (char *)&paramList[i].dblValue;
                    break;
                }                
                case MYSQL_TYPE_LONGLONG:
                {
                    bindparam[i].buffer = (char *)&paramList[i].longValue;
                    break;
                }
                case MYSQL_TYPE_LONG:
                {
                    bindparam[i].buffer = (char *)&paramList[i].intValue;
                    break;
                }
                case MYSQL_TYPE_STRING:
                {
                    bindparam[i].buffer = (char *)paramList[i].stringValue;
                    break;
                }
                case MYSQL_TYPE_BLOB:
                {
                    bindparam[i].buffer = (char *)paramList[i].stringValue;
                    break;
                }            
                default:
                {
                    TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Open()  paramList[%d].dataType[%d] is wrong",__FILE__,__LINE__,i,paramList[i].dataType);
                    fErrorNo = -1;
                    return;
                }
            }            
        }
        
        if (mysql_stmt_bind_param(stmt, bindparam))
        {
            if (bindparam != NULL)
            {
                delete[] bindparam;
                bindparam = NULL;
            }
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Open()  mysql_stmt_bind_param failed",__FILE__,__LINE__);
            fErrorNo = mysql_stmt_errno(stmt);
            throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Open()  mysql_stmt_bind_param failed");
            return;
        }
    }
    
    //����ÿ�λ���ļ�¼����   
    unsigned long prefetch_rows = (unsigned long)prefetch_Row;
    if(mysql_stmt_attr_set(stmt, STMT_ATTR_PREFETCH_ROWS,(void*) &prefetch_rows))
    {
        if (bindparam != NULL)
        {
            delete[] bindparam;
            bindparam = NULL;
        }
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Open()  mysql_stmt_attr_set failed",__FILE__,__LINE__);
        fErrorNo = mysql_stmt_errno(stmt);
        throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Open()  mysql_stmt_attr_set failed");
        return;
    }
       
    if (mysql_stmt_execute(stmt))
    {
        if (bindparam != NULL)
        {
            delete[] bindparam;
            bindparam = NULL;
        }
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Open()  mysql_stmt_execute failed",__FILE__,__LINE__);
        fErrorNo = mysql_stmt_errno(stmt);
        throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Open()  mysql_stmt_execute failed");
        return;
    }

    //�������select��䣬û�н������ֱ�ӷ��ء�
    if(!isSelectSql)
    {
        if (bindparam != NULL)
        {
            delete[] bindparam;
            bindparam = NULL;
        }
        return;
    }

    GetFieldsDef(); 

    bindresult = new(std::nothrow) MYSQL_BIND[iFieldCount];
	if(bindresult == NULL)
	{
		MYSQL_ERROR_TO_THROW("TMsqDBQuery::Open()", "Memory allocate failed");
	}
    memset(bindresult, 0, sizeof(MYSQL_BIND)*iFieldCount);

    for(int i=0;i<iFieldCount;i++)
    {
        bindresult[i].buffer_type = fieldList[i].buffer_type;
        bindresult[i].buffer = fieldList[i].buffer;
        bindresult[i].buffer_length = fieldList[i].max_length;        
        bindresult[i].length = &fieldList[i].length;
        bindresult[i].is_null = &fieldList[i].is_null;
    }    
     
    // Bind the result buffers 
    if (mysql_stmt_bind_result(stmt, bindresult))
    {
        if (bindparam != NULL)
        {
            delete[] bindparam;
            bindparam = NULL;
        }

        if (bindresult != NULL)
        {
            delete[] bindresult;
            bindresult = NULL;
        }
        
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Open()  mysql_stmt_bind_result failed",__FILE__,__LINE__);
        fErrorNo = mysql_stmt_errno(stmt);
        throw TMsqDBException(mysql_stmt_errno(stmt), mysql_stmt_error(stmt), "TMsqDBQuery::Open()  mysql_stmt_bind_result failed");
        return;
    }

    fOpened = true;

    if (bindparam != NULL)
    {
        delete[] bindparam;
        bindparam = NULL;
    } 
}

/******************************************************************************
* ��������	:  FieldCount()
* ��������	:  �ܹ��м�����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ����
* ����		:  li.shugang
*******************************************************************************/
int TMsqDBQuery::FieldCount()
{
    return iFieldCount;
}

/******************************************************************************
* ��������	:  ParamCount()
* ��������	:  ��ȡ��������  
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��������
* ����		:  li.shugang
*******************************************************************************/
int TMsqDBQuery::ParamCount()
{
    return iParamCount;
}

/******************************************************************************
* ��������	:  GetFieldPrecision()
* ��������	:  ����ָ������Ϣ  
* ����		:  index ������
* ���		:  ��
* ����ֵ	:  �ж���
* ����		:  li.shugang
*******************************************************************************/
TMsqDBField& TMsqDBQuery::Field(int i) throw (TMsqDBException)
{
    if (sqlstr == NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Field()   MDB_ERR_FIELD_NOT_EXISTS",__FILE__,__LINE__);
        throw TMsqDBException("", MDB_ERR_GENERAL, "Field(index): sql statement is not presented");
    }
    
    if ( (i>=0) && (i<iFieldCount) ) 
        return fieldList[i];
    else 
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Field()   field index out of bound when call Field(i)",__FILE__,__LINE__);
        throw TMsqDBException(sqlstr , MDB_ERR_INDEX_OUT_OF_BOUND, "field index out of bound when call Field(i)");
    }
}

/******************************************************************************
* ��������	:  Field()
* ��������	:  ��������(���ִ�Сд)��������Ϣ; ����ʹ��Field(int i)��ø��ߵ�Ч��	
* ����		:  fieldName ����
* ���		:  ��
* ����ֵ	:  ����Ϣ
* ����		:  li.shugang
*******************************************************************************/
TMsqDBField& TMsqDBQuery::Field(const char *fieldName) throw (TMsqDBException)
{
    int i;
    bool found = false;

    if (sqlstr == NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Field()   MDB_ERR_FIELD_NOT_EXISTS",__FILE__,__LINE__);
        throw TMsqDBException("", MDB_ERR_GENERAL, "Field(*fieldName): sql statement is not presented");
    }
    
    if (! fOpened)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Field()   can not access field before open",__FILE__,__LINE__);
        throw TMsqDBException(sqlstr, MDB_ERR_GENERAL, "can not access field before open");
    }

    for(i=0; i<iFieldCount; i++)
    {
        found = CompareStrNoCase(Field(i).name,fieldName);
        if ( found )
            break;
    }
    
    if ( found ) 
        return fieldList[i];
    else 
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Field()   MDB_ERR_FIELD_NOT_EXISTS",__FILE__,__LINE__);
        throw TMsqDBException(sqlstr, MDB_ERR_FIELD_NOT_EXISTS, fieldName);
    }
}

/******************************************************************************
* ��������	:  IsFieldExist()
* ��������	:  ָ�����Ƿ����	
* ����		:  fieldName ����
* ���		:  ��
* ����ֵ	:  �о���
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBQuery::IsFieldExist(const char *fieldName)
{
    bool found = false;
    for(int i=0; i<iFieldCount; i++)
    {
        found = CompareStrNoCase(Field(i).name,fieldName);
        if ( found )
            return true;
    }
    return false ;
}

/******************************************************************************
* ��������	:  Param()
* ��������	:  ����ָ��������Ϣ
* ����		:  index ��������
* ���		:  ��
* ����ֵ	:  ��������
* ����		:  li.shugang
*******************************************************************************/
TMsqDBParam& TMsqDBQuery::Param(int index) throw (TMsqDBException)
{
    if (sqlstr == NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Param()   MDB_ERR_FIELD_NOT_EXISTS",__FILE__,__LINE__);
        throw TMsqDBException("", MDB_ERR_GENERAL, "Param(i): sql statement is not presented");
    }

    if ( (index>=0) && (index<iParamCount) ) 
        return paramList[index];
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Param()   param index out of bound when call Param(i)",__FILE__,__LINE__);
        throw TMsqDBException(sqlstr , MDB_ERR_INDEX_OUT_OF_BOUND, "param index out of bound when call Param(i)");
    }
}

/******************************************************************************
* ��������	:  Param()
* ��������	:  ���ݲ�����(���ִ�Сд)��������Ϣ; ����ʹ��Field(int i)��ø��ߵ�Ч��
* ����		:  paramName ������
* ���		:  ��
* ����ֵ	:  ��������
* ����		:  li.shugang
*******************************************************************************/
TMsqDBParam& TMsqDBQuery::Param(const char *inName) throw (TMsqDBException)
{
    int i;
    bool found = false;

    if (sqlstr == NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Param()   sql statement is not presented",__FILE__,__LINE__);
        throw TMsqDBException("", MDB_ERR_GENERAL, "Param(paramName): sql statement is not presented");
    }

    for(i=0; i<iParamCount; i++)
    {
        found = CompareStrNoCase(paramList[i].name,inName);
        if (found)
            break;
    }
    if ( found ) 
        return paramList[i];
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Param()   MDB_ERR_PARAM_NOT_EXISTS",__FILE__,__LINE__);
        throw TMsqDBException(sqlstr, MDB_ERR_PARAM_NOT_EXISTS, (const char*)inName);
    }
}

/******************************************************************************
* ��������	:  Next()
* ��������	:  �ƶ�����һ����¼ 
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true �ɹ�;false ʧ��
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBQuery::Next() throw (TMsqDBException)
{    
    if(!isSelectSql)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Next()   sql is not select_sql, can't use next",__FILE__,__LINE__);
        throw TMsqDBException(sqlstr, "TMsqDBQuery::Next()  sql is not select_sql, can't use next");
        return false;
    }
    
    if(!fOpened)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBQuery::Next()   MsqDBQuery not open",__FILE__,__LINE__);
        throw TMsqDBException(sqlstr, "TMsqDBQuery::Next()   MsqDBQuery not open");
        return false;
    }
    
    if(mysql_stmt_fetch(stmt))
    {
        //mysql select������commit������ʹ��仯�ˣ�sql��ѯ������ǲ���仯��
        if(isSelectSql)
        {
            Commit();
        }    
        return false;
    }
    else
    {
        return true;
    }   
}

bool TMsqDBDatabase::bInitMutex = true;

/****************** TMsqDBDatabase implementation **************************/
TMsqDBDatabase::TMsqDBDatabase() throw (TMsqDBException)
{    
    usr = NULL;
    pwd = NULL;
    dbs = NULL;
    host = NULL;
    tns = NULL;
    port = 0;
    fErrorNo = 0;
    fConnected = false;

	
	for(int i = 0; i<MAX_QUERY_PER_DB; i++)
	{
		ppDBList[i] = NULL;
	}

	while(true)
	{
		if(bInitMutex)
		{
			bInitMutex = false;
			for( int i = 0; i < 3; i++)
			{
				conn = mysql_init(NULL);
				if(conn != NULL) break;
			}
			if(conn != NULL) break;
		}
		TMdbDateTime::MSleep(10);
	}
	bInitMutex = true;
    if (conn == NULL) 
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::TMsqDBDatabase()  MDB_ERR_DB_INIT.",__FILE__,__LINE__);
        throw TMsqDBException(mysql_errno(conn), mysql_error(conn),"mysql_init() ");
	}
}

TMsqDBDatabase::~TMsqDBDatabase()
{
    delete[] usr;
    delete[] pwd;
    delete[] dbs;
    delete[] host;
    delete[] tns;
    port = 0;

    if (fConnected) 
        mysql_close(conn);

    fErrorNo = 0;
}

/******************************************************************************
* ��������	:  SetLogin()
* ��������	:  �������ݿ��¼��
* ����		:  user �û�����password ���룬tnsString ������
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBDatabase::SetLogin(const char *user, const char *password, const char *tnsString) throw (TMsqDBException)
{
    if (fConnected)
    {
    	if(IsConnect())
		{
	        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::SetLogin()  MDB_ERR_SET_LOGIN.",__FILE__,__LINE__);
	        throw TMsqDBException("SetLogin()", MDB_ERR_SET_LOGIN , __LINE__);
		}
		else
		{
			fConnected = false;
		}
    }

    int nLen;
    //�����ⲿ���ݵĲ���
    if (usr != NULL)
        delete[] usr;
    if (pwd != NULL)
        delete[] pwd;
    if (dbs != NULL)
        delete[] dbs;
    if (host != NULL)
        delete[] host;
    port =0;


    //�����ⲿ���ݵĲ���
    if (user)
    {
        nLen = strlen(user);
        usr = new(std::nothrow) char[nLen+1];
		if(usr == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBDatabase::SetLogin()", "Memory allocate failed");
		}
        strncpy(usr,user,nLen);
        usr[nLen] = '\0';
    }
    else
    {
        nLen = 0;
        usr = new(std::nothrow) char[1];
		if(usr == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBDatabase::SetLogin()", "Memory allocate failed");
		}
        usr[0] = '\0';
    }

    if (password)
    {
        nLen = strlen(password);
        pwd = new(std::nothrow) char[nLen+1];
		if(pwd == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBDatabase::SetLogin()", "Memory allocate failed");
		}
        strncpy(pwd,password,nLen);
        pwd[nLen] = '\0';
    }
    else
    {
        nLen = 0;
        pwd = new(std::nothrow) char[1];
		if(pwd == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBDatabase::SetLogin()", "Memory allocate failed");
		}
        pwd[0] = '\0';
    }

    //"mydb|localhost:3306"
    if (tnsString)
    {
        nLen = strlen(tnsString);
        tns = new(std::nothrow) char[nLen+1];
		if(tns == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBDatabase::SetLogin()", "Memory allocate failed");
		}
        strncpy(tns,tnsString,nLen);
        tns[nLen] = '\0';
        
        TMdbNtcSplit dbsplit;
        dbsplit.SplitString(tnsString, "|");
        if(2 != dbsplit.GetFieldCount())
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::SetLogin()  MDB_ERR_SET_LOGIN.",__FILE__,__LINE__);
            throw TMsqDBException("SetLogin()", "Config db-id[%s] error. file[%s],line[%d], " ,tnsString, __FILE__,__LINE__);
        } 
        
        nLen = strlen(dbsplit.Field(0));
        dbs  = new(std::nothrow) char[nLen+1];
		if(dbs == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBDatabase::SetLogin()", "Memory allocate failed");
		}
        strncpy(dbs,dbsplit.Field(0),nLen);
        dbs[nLen] = '\0';
        
        TMdbNtcSplit hostsplit;
        hostsplit.SplitString(dbsplit.Field(1), ":");
        if(2 != hostsplit.GetFieldCount())
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::SetLogin()  MDB_ERR_SET_LOGIN.",__FILE__,__LINE__);
            throw TMsqDBException("SetLogin()", "Config db-id[%s] error. file[%s],line[%d], " ,tnsString, __FILE__,__LINE__);
        } 
        
        nLen = strlen(hostsplit.Field(0));
        host  = new(std::nothrow) char[nLen+1];
		if(host == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBDatabase::SetLogin()", "Memory allocate failed");
		}
        strncpy(host,hostsplit.Field(0),nLen);
        host[nLen] = '\0';
        
        if(TMdbNtcStrFunc::IsDigital(hostsplit.Field(1)))
        {
            port = TMdbNtcStrFunc::StrToInt(hostsplit.Field(1)); 
        }
        else
        {            
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::SetLogin()  MDB_ERR_SET_LOGIN.",__FILE__,__LINE__);
            throw TMsqDBException("SetLogin()", "Config db-id[%s] error. file[%s],line[%d], " ,tnsString, __FILE__,__LINE__);
        }
    }
    else    
    {
        dbs = new(std::nothrow) char[1];
		if(dbs == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBDatabase::SetLogin()", "Memory allocate failed");
		}
        dbs[0] = '\0';

        host = new(std::nothrow) char[1];
		if(host == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBDatabase::SetLogin()", "Memory allocate failed");
		}
        host[0] = '\0';

        tns = new(std::nothrow) char[1];
		if(tns == NULL)
		{
			MYSQL_ERROR_TO_THROW("TMsqDBDatabase::SetLogin()", "Memory allocate failed");
		}
        tns[0] = '\0';

        port = 0;
    }
}

/******************************************************************************
* ��������	:  CheckError()
* ��������	:  �����жϵ�ǰ������Ƿ���ȷִ�У�����д�����Ѵ�����Ϣ����errMsg
* ����		:  bUnused �Ƿ�ʹ��,Ĭ��false
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBDatabase::CheckError(const char* sSql) throw (TMsqDBException)
{
    if (mysql_errno(conn) != 0) 
    {
        throw TMsqDBException(mysql_errno(conn), mysql_error(conn), sSql);
    }

}

/******************************************************************************
* ��������	:  Connect()
* ��������	:  ���ݿ�����
* ����		:  bUnused �Ƿ�ʹ��,Ĭ��false
* ���		:  ��
* ����ֵ	:  true ���ӳɹ���false ����ʧ��
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBDatabase::Connect(bool bIsAutoCommit) throw (TMsqDBException)
{
    TADD_NORMAL("connect mysql [%s/******@%s|%s:%d].",usr,dbs,host,port);
    
    AutoCommit = bIsAutoCommit;
    
    if (fConnected)
    {
    	if(IsConnect())
		{
	        TADD_NORMAL("Has already connected to mysql server.");
	        return true;
		}
		else
		{
			fConnected = false;
		}
    }

    if ( (usr == NULL) || (dbs == NULL) )
    {
        fConnected = false;
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Connect()  MDB_ERR_CONNECT_NO_LOGIN_INFO.",__FILE__,__LINE__);
        throw TMsqDBException("TMsqDBDatabase::Connect()", MDB_ERR_CONNECT_NO_LOGIN_INFO, __LINE__);
        return false;
    }

	bool bReconn = true;
	if(mysql_options(conn, MYSQL_OPT_RECONNECT, &bReconn))
	{
		fConnected = false;
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Connect()  MDB_ERR_CONNECT_SET_RECONNECT_FAIL.",__FILE__,__LINE__);
        throw TMsqDBException(mysql_errno(conn), mysql_error(conn),"mysql_options failed.");
        return false;
	}

    if (mysql_real_connect(conn, host, usr, pwd, dbs, port, NULL, 0)==NULL)
    {
        fConnected = false;
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Connect()  MDB_ERR_CONNECT_FAIL.",__FILE__,__LINE__);
        throw TMsqDBException(mysql_errno(conn), mysql_error(conn),"mysql_real_connect failed.");
        return false;
    }    

    if(bIsAutoCommit)
    {
        if(mysql_autocommit(conn, 1))
        {
            fConnected = false;
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Connect()  mysql_autocommit failed.",__FILE__,__LINE__);
            throw TMsqDBException(mysql_errno(conn), mysql_error(conn),"mysql_real_connect failed.");
        } 
    }
    else
    {
        if(mysql_autocommit(conn, 0))
        {
            fConnected = false;
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Connect()  mysql_autocommit failed.",__FILE__,__LINE__);
            throw TMsqDBException(mysql_errno(conn), mysql_error(conn),"mysql_real_connect failed.");
        } 
    }   
	fConnected = true;
	return true;
}

/******************************************************************************
* ��������	:  Connect()
* ��������	:  ���ݿ�����
* ����		:  inUsr �û�����inPwd ���룬inTns ������|IP:PORT��bIsAutoCommit �Ƿ��Զ�commit,Ĭ��false
* ���		:  ��
* ����ֵ	:  true ���ӳɹ���false ����ʧ��
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBDatabase::Connect(const char *inUsr, const char *inPwd, const char *inTns, bool bIsAutoCommit) throw (TMsqDBException)
{
    SetLogin(inUsr, inPwd, inTns);
    return Connect(bIsAutoCommit);
}

/******************************************************************************
* ��������	:  Disconnect()
* ��������	:  �Ͽ����ݿ�����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  1 �ɹ�������ʧ��
* ����		:  li.shugang
*******************************************************************************/
int TMsqDBDatabase::Disconnect() throw (TMsqDBException)
{
    if (fConnected)
    {
        mysql_close(conn);    
        fConnected = false;
    } 
    return 1;
}

/******************************************************************************
* ��������	:  Commit()
* ��������	:  �����ύ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBDatabase::Commit()
{
    if(0 != mysql_commit(conn))
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Commit()  mysql commit failed.",__FILE__,__LINE__);
        throw TMsqDBException(mysql_errno(conn), mysql_error(conn),"Commit failed.");
    }        
	for(int i = 0; i<MAX_QUERY_PER_DB; i++)
	{
		if(ppDBList[i] != NULL)
		{
			if(0 != mysql_commit(ppDBList[i]->conn))
		    {
		        TADD_ERROR("[%s:%d] TMsqDBDatabase::Commit()  mysql commit failed.",__FILE__,__LINE__);
		        throw TMsqDBException(mysql_errno(ppDBList[i]->conn), mysql_error(ppDBList[i]->conn),"Commit failed.");
		    }
		}
	}  
}

/******************************************************************************
* ��������	:  Rollback()
* ��������	:  ����ع�
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TMsqDBDatabase::Rollback()
{
    if(0 != mysql_rollback(conn))
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::Connect()  mysql rollback failed.",__FILE__,__LINE__);
        throw TMsqDBException(mysql_errno(conn), mysql_error(conn),"Rollback failed.");
    }
	for(int i = 0; i<MAX_QUERY_PER_DB; i++)
	{
		if(ppDBList[i] != NULL)
		{
			if(0 != mysql_rollback(ppDBList[i]->conn))
		    {
		        TADD_ERROR("[%s:%d] TMsqDBDatabase::Connect()  mysql rollback failed.",__FILE__,__LINE__);
		        throw TMsqDBException(mysql_errno(ppDBList[i]->conn), mysql_error(ppDBList[i]->conn),"Rollback failed.");
		    }
		}
	}  
}

/******************************************************************************
* ��������	:  IsConnect()
* ��������	:  �������ݿ��Ƿ���������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true ������false ���ݿ������쳣
* ����		:  li.shugang
*******************************************************************************/
bool TMsqDBDatabase::IsConnect() throw (TMsqDBException)
{
    if(fConnected==true)
    {
        int iRet = mysql_ping(conn);
        if(0 == iRet)
        {
            return true;
        }
		else
		{//ǰ�������Ҫsleepһ��ʱ�������������������ȷ��
			iRet = mysql_ping(conn);
	        if(0 == iRet)
	        {
	            return true;
	        }
	        else if(CR_COMMANDS_OUT_OF_SYNC == iRet)
	        {
	            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::IsConnect()  exec cmd with wrong order. errno[%d], errmsg[%s].",__FILE__,__LINE__,mysql_errno(conn), mysql_error(conn));
	            fConnected = false;
	            return false;
	        }
	        else if(CR_SERVER_GONE_ERROR == iRet)
	        {
	            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::IsConnect()  server can't be used. errno[%d], errmsg[%s].",__FILE__,__LINE__,mysql_errno(conn), mysql_error(conn));
	            fConnected = false;
	            return false;
	        }
	        else
	        {
	            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMsqDBDatabase::IsConnect()  unkonw error.  errno[%d], errmsg[%s].",__FILE__,__LINE__,mysql_errno(conn), mysql_error(conn));
	            fConnected = false;
	            return false;
	        }
		}
    }
    else
    {
        TADD_NORMAL("[%s:%d] TMsqDBDatabase::IsConnect()  mysql is not connect yet.",__FILE__,__LINE__);
        fConnected = false;
        return false;
    }
}

/******************************************************************************
* ��������	:  CreateDBQuery()
* ��������	:  �������ݿ��ѯ����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  TMsqDBQuery ����
* ����		:  li.shugang
*******************************************************************************/
TMsqDBQuery * TMsqDBDatabase::CreateDBQuery() throw (TMsqDBException)
{
	int i;
    TMsqDBDatabase* pbase = new(std::nothrow) TMsqDBDatabase();
	if(pbase == NULL)
	{
		MYSQL_ERROR_TO_THROW("TMsqDBDatabase::CreateDBQuery()", "Memory allocate failed");
	}
	
    pbase->Connect(usr,pwd,tns,AutoCommit);
	for(i = 0; i<MAX_QUERY_PER_DB; i++)
	{
		if(ppDBList[i] == NULL)
		{
			ppDBList[i] = pbase;
			break;
		}
	}
	
	if(i == MAX_QUERY_PER_DB)
	{
		TADD_ERROR(ERROR_UNKNOWN, "Too many querys on this database.");
		return NULL;
	}
    return new(std::nothrow) TMsqDBQuery(pbase);    
}

//}

#endif   //DB_MYSQL
