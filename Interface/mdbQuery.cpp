/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@                     All rights reserved.
*@Name��	    mdbQuery.cpp
*@Description�� ����ֱ�����ʷ�ʽ�Ľӿ�
*@Author:	   jin.shaohua
*@Date��	    2012.05
*@History:
******************************************************************************************/
#include "Interface/mdbQuery.h"
#include "Helper/mdbConfig.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"

#include "Helper/mdbBase.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbSQLParser.h"
#include "Control/mdbExecuteEngine.h"

#include "Helper/mdbMalloc.h"
#include "Helper/SyntaxTreeAnalyse.h"
#include "Helper/SqlParserHelper.h"

#include "Control/mdbMgrShm.h"
#include "Interface/mdbRollback.h"
#include "Control/mdbProcCtrl.h"
#include "Control/mdbLinkCtrl.h"
#include "Helper/mdbMultiProtector.h"
#include "Control/mdbDDLExecuteEngine.h"
//#include "Interface/mdbSQLHelper.h"

//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{


#define LOG_SQLPARSER_DETAIL \
{\
TADD_ERROR(ERROR_UNKNOWN,"SQL=[%s].Row_Affected=[%d]",m_pMdbSqlParser->m_stSqlStruct.sSQL,RowsAffected());\
std::vector<_ST_MEM_VALUE *> & arrMemValue = m_pMdbSqlParser->m_listInputVariable.vMemValue;\
std::vector<_ST_MEM_VALUE *>::iterator itor = arrMemValue.begin();\
for(; itor != arrMemValue.end(); ++itor)\
{\
    ST_MEM_VALUE * pstMemValue = *itor;\
    TADD_ERROR(ERROR_UNKNOWN,"[%-10s],Flag=[%d],lvalue=[%lld],sValue=[%s],iSize=[%d]",\
            TSyntaxTreeAnalyse::ReplaceNull(pstMemValue->sAlias),\
            pstMemValue->iFlags,\
            pstMemValue->lValue,\
            TSyntaxTreeAnalyse::ReplaceNull(pstMemValue->sValue),\
            pstMemValue->iSize);  \
}\
}


#ifdef OS_SUN
#define CHECK_RET_THROW(_ret,_errCode,_sql,...)  if((iRet = _ret)!=0){\
TADD_ERROR(ERROR_UNKNOWN,__VA_ARGS__);\
LOG_SQLPARSER_DETAIL \
throw TMdbException(_errCode,_sql,__VA_ARGS__);\
}

#define CHECK_RET_THROW_NOSQL(_ret,_errCode,...)  if((iRet = _ret)!=0){\
TADD_ERROR(ERROR_UNKNOWN,__VA_ARGS__);\
throw TMdbException(_errCode,"",__VA_ARGS__);\
}

#define ERROR_TO_THROW_NOSQL(_errCode,...) \
TADD_ERROR(ERROR_UNKNOWN,__VA_ARGS__);\
throw TMdbException(_errCode,"",__VA_ARGS__);\
 
#define ERROR_TO_THROW(_errCode,_sql,...) \
TADD_ERROR(ERROR_UNKNOWN,__VA_ARGS__);\
 LOG_SQLPARSER_DETAIL \
throw TMdbException(_errCode,_sql,__VA_ARGS__);

#else

#define CHECK_RET_THROW(_ret,_errCode,_sql,FMT,...)  if((iRet = _ret)!=0){\
TADD_ERROR(ERROR_UNKNOWN,FMT,##__VA_ARGS__);\
LOG_SQLPARSER_DETAIL \
throw TMdbException(_errCode,_sql,"File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);\
}

#define CHECK_RET_THROW_NOSQL(_ret,_errCode,FMT,...)  if((iRet = _ret)!=0){\
TADD_ERROR(ERROR_UNKNOWN,FMT,##__VA_ARGS__);\
throw TMdbException(_errCode,"","File=[%s], Line=[%d],"FMT,__FILE__,__LINE__,##__VA_ARGS__);\
}


#define ERROR_TO_THROW_NOSQL(_errCode,FMT,...) \
TADD_ERROR(ERROR_UNKNOWN,FMT,##__VA_ARGS__);\
throw TMdbException(_errCode,"","File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);\
 

#define ERROR_TO_THROW(_errCode,_sql,FMT,...) \
TADD_ERROR(ERROR_UNKNOWN,FMT,##__VA_ARGS__);\
LOG_SQLPARSER_DETAIL \
throw TMdbException(_errCode,_sql,"File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);

#endif

//�������÷�ʽ
enum E_SET_PARAM_TYPE
{
    SET_PARAM_NONE  = 0,//��û��
    SET_PARAM_ONE   = 1, //������
    SET_PARAM_ARRAY = 2  //������
};


TMdbException::~TMdbException() {}
TMdbException::TMdbException() {}

TMdbException::TMdbException(int errId,const char *pszSql, const char* pszFormat, ...)
{
    va_list args;

    memset(m_sErrMsg,0,sizeof(m_sErrMsg));
    memset(m_sErrSql,0,sizeof(m_sErrSql));
    m_iErrCode=0;
    sprintf(m_sErrMsg,"Error=[%d].Detail:",errId);//��Ӵ�������ϸǰ׺
    va_start(args, pszFormat);
    int iLen = strlen(m_sErrMsg);
    vsnprintf(m_sErrMsg+iLen,sizeof(m_sErrMsg)-iLen, pszFormat, args);
    va_end(args);
    m_sErrMsg[sizeof(m_sErrMsg)-1] = '\0';
    

    strncpy(m_sErrSql, pszSql, sizeof(m_sErrSql)-1);
    m_sErrSql[sizeof(m_sErrSql)-1] = '\0';
    m_iErrCode = errId;
}

const char *TMdbException::GetErrMsg()
{
   return((char*)m_sErrMsg);
}
int TMdbException::GetErrCode()
{
    return m_iErrCode<0?0-m_iErrCode:m_iErrCode;
}
const char *TMdbException::GetErrSql()
{
    return m_sErrSql;
}

/******************************************************************************
* ������	:  TMdbNullArrWrap
* ������	:  null�����װ��
* ����		:
*******************************************************************************/
TMdbNullArrWrap::TMdbNullArrWrap():m_bNullArr(NULL),m_iAlloc(0)
{
    
};
TMdbNullArrWrap::~TMdbNullArrWrap()
{
    SAFE_DELETE_ARRAY(m_bNullArr);
}
/******************************************************************************
* ��������	:  Init
* ��������	:  ��ʼ��
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbNullArrWrap::Init(bool * bNullArr,int iCount)
{
    if(NULL == bNullArr )
    {
        if(NULL != m_bNullArr){memset(m_bNullArr,0,m_iAlloc * sizeof(bool));}
        return 0;
    }
    Assign(iCount);
    memset(m_bNullArr,0,m_iAlloc * sizeof(bool));
    memcpy(m_bNullArr,bNullArr,iCount* sizeof(bool));
    return 0;
};
/******************************************************************************
* ��������	:  IsNull
* ��������	:  �ж��Ƿ�ΪNULL
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbNullArrWrap::IsNull(int iIndex)
{
    if(NULL == m_bNullArr)
    {
        return false;
   }
   else
   {
    return m_bNullArr[iIndex];
   }
}
/******************************************************************************
* ��������	:  Assign
* ��������	:  �����ڴ�
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbNullArrWrap::Assign(int iCount)
{
    if(iCount > m_iAlloc)
    {
        SAFE_DELETE_ARRAY(m_bNullArr);
        m_bNullArr = new bool[iCount];
        m_iAlloc = iCount;
    }
    if(iCount > 0)
    {
        memset(m_bNullArr,0,m_iAlloc *sizeof(bool));
    }
    return 0;
}
/******************************************************************************
* ��������	:  SetNull
* ��������	:  ����NULL;
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbNullArrWrap::SetNull(int iIndex)
{
    int iRet = 0;
    if(iIndex >= m_iAlloc)
    {
        CHECK_RET(ERROR_UNKNOWN,"iIndex[%d] >= m_iAlloc[%d]",iIndex,m_iAlloc);
    }
    m_bNullArr[iIndex] = true;
    return iRet;
}


/******************************************************************************
* ��������	:  TMdbParamArray
* ��������	:  �������鶨��
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
TMdbParamArray::TMdbParamArray()
{
    m_iParamIndex = -1;
    m_iParamType  = -1;
    m_iArraySize  = -1;
    m_pllValue    = NULL;
    m_psValue     = NULL;
    m_iAllocSize = -1;
}

TMdbParamArray::~TMdbParamArray()
{
    SAFE_DELETE_ARRAY(m_pllValue);
    SAFE_DELETE_ARRAY(m_psValue);
}
//���·����С
int TMdbParamArray::ReAlloc(int iArraySize,int iType)
{
    if(iArraySize <= 0)
    {
        return -1;
    }
    m_tNullArrWrap.Assign(iArraySize);//nullֵ����
    if(m_iAllocSize >= iArraySize && iType == m_iParamType)
    {
        m_iArraySize = iArraySize;
    }
    else
    {
        SAFE_DELETE_ARRAY(m_pllValue);
        SAFE_DELETE_ARRAY(m_psValue);
        switch(iType)
        {
        case MEM_Int:
            m_pllValue = new (std::nothrow)long long[iArraySize];
            if(NULL == m_pllValue)
            {
                TADD_ERROR(ERROR_UNKNOWN,"m_pllValue is NULL.");
                return ERR_OS_NO_MEMROY;
            }
            break;
        case MEM_Str:
            m_psValue = new (std::nothrow)char *[iArraySize];
            if(NULL == m_psValue)
            {
                TADD_ERROR(ERROR_UNKNOWN,"m_psValue is NULL.");
                return ERR_OS_NO_MEMROY;
            }
            break;
        default:
            TADD_ERROR(ERROR_UNKNOWN,"type[%d] error.",iType);
            return -1;
            break;
        }
        m_iArraySize = iArraySize;
        m_iParamType = iType;
        m_iAllocSize = iArraySize;
    }
    return 0;
}



/******************************************************************************
* ��������	:  TMdbField
* ��������	:  Fieldֵ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
TMdbField::TMdbField()
{
    memset(m_sValue,0, sizeof(m_sValue));
    m_pMemValue = NULL;
}


TMdbField::~TMdbField()
{
}

/******************************************************************************
* ��������	:  isNULL
* ��������	:  ֵ�Ƿ�Ϊ��
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
bool TMdbField::isNULL()
{
    return (NULL != m_pMemValue) && m_pMemValue->IsNull();
}

/******************************************************************************
* ��������	:  AsString
* ��������	:  ��ȡstringֵ
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
char* TMdbField::AsString() throw (TMdbException)
{
    if(NULL == m_pMemValue )
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_VALUE_INVALID,"Column=[%s] is NULL",m_pMemValue->sAlias);
    }
    if(m_pMemValue->IsNull())
    {
        m_sValue[0] = 0;
        return m_sValue;
    }
    //���������
    if(MemValueHasProperty(m_pMemValue,MEM_Int) && false == MemValueHasAnyProperty(m_pMemValue,MEM_Str|MEM_Date))
    {
        sprintf(m_sValue, "%lld", m_pMemValue->lValue);
        return m_sValue;
    }
    return m_pMemValue->sValue;
}
/******************************************************************************
* ��������	:  AsFloat
* ��������	:  ��ȡ��������
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
double TMdbField::AsFloat() throw (TMdbException)
{
    if(NULL == m_pMemValue)
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_VALUE_INVALID,"Column=[%s] is NULL",m_pMemValue->sAlias);
    }
    //���������
    if(MemValueHasProperty(m_pMemValue,MEM_Int))
    {
        return m_pMemValue->lValue;
    }
    else
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"Column=[%s] isn't INTERGER/FLOAT",m_pMemValue->sAlias);
    }
}

/******************************************************************************
* ��������	:  AsInteger
* ��������	:  ��ȡ��������
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
long long  TMdbField::AsInteger() throw (TMdbException)
{
    if(NULL == m_pMemValue || m_pMemValue->IsNull())
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_VALUE_INVALID,"Column=[%s] is NULL",m_pMemValue->sAlias);
    }
    //���������
    if(MemValueHasProperty(m_pMemValue,MEM_Int))
    {
        return m_pMemValue->lValue;
    }
    else
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"Column=[%s] isn't INTERGER",m_pMemValue->sAlias);
    }
}
/******************************************************************************
* ��������	:  AsDateTime
* ��������	:  �������ڵĸ�������
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
void  TMdbField::AsDateTime(int &iYear,int &iMonth,int &iDay,int &iHour,int &iMinute,int &iSecond) throw (TMdbException)
{
    if(NULL == m_pMemValue|| m_pMemValue->IsNull())
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_VALUE_INVALID,"Column=[%s] is NULL",m_pMemValue->sAlias);
    }
    if(MemValueHasAnyProperty(m_pMemValue,MEM_Str|MEM_Date))
    {
       // strcpy(m_sValue,m_pMemValue->sValue);
       SAFESTRCPY(m_sValue,sizeof(m_sValue),m_pMemValue->sValue);
    }
    else if(MemValueHasProperty(m_pMemValue,MEM_Int))
    {
        sprintf(m_sValue,"%lld",m_pMemValue->lValue);
    }
    else
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"Column=[%s] isn't TIMESTAMP",m_pMemValue->sAlias);
    }
    iYear  = (m_sValue[0]-'0')*1000 + (m_sValue[1]-'0')*100 + (m_sValue[2]-'0')*10 + (m_sValue[3]-'0');
    iMonth = (m_sValue[4]-'0')*10 + (m_sValue[5]-'0');
    iDay   = (m_sValue[6]-'0')*10 + (m_sValue[7]-'0');
    iHour  = (m_sValue[8]-'0')*10 + (m_sValue[9]-'0');
    iMinute = (m_sValue[10]-'0')*10 + (m_sValue[11]-'0');
    iSecond = (m_sValue[12]-'0')*10 + (m_sValue[13]-'0');
}

/******************************************************************************
* ��������	:  AsDateTimeString
* ��������	:  ��ȡ�����ַ���
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
char* TMdbField::AsDateTimeString() throw (TMdbException)
{
    if(NULL == m_pMemValue|| m_pMemValue->IsNull())
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_VALUE_INVALID,"Column=[%s] is NULL",m_pMemValue->sAlias);
    }
    /*
    if(m_pMemValue->IsNull())
    {
        m_sValue[0] = 0;
        return m_sValue;
    }
    */
    if(MemValueHasAnyProperty(m_pMemValue,MEM_Str|MEM_Date))
    {
        return m_pMemValue->sValue;
    }
    else
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"Column=[%s] isn't TIMESTAMP",m_pMemValue->sAlias);
    }
}
/******************************************************************************
* ��������	:  AsBlobBuffer
* ��������	:  ��ȡ blobֵ
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
void TMdbField::AsBlobBuffer(unsigned char *buffer, int &iBufferLen) throw (TMdbException)
{
    if(NULL == m_pMemValue|| m_pMemValue->IsNull())
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_VALUE_INVALID,"Column=[%s] is NULL",m_pMemValue->sAlias);
    }
    /*
    if(m_pMemValue->IsNull())
    {
        buffer[0] = 0;
        return;
    }*/
    //blob
    if(MemValueHasProperty(m_pMemValue,MEM_Str))
    {
        //Base64����
        std::string encoded = m_pMemValue->sValue;
        std::string decoded = Base::base64_decode(encoded);
        iBufferLen = decoded.length();
        memcpy(buffer,decoded.c_str(),decoded.length());
    }
    else
    {
        ERROR_TO_THROW_NOSQL(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"Column=[%s] isn't BLOB",m_pMemValue->sAlias);

    }
}

/******************************************************************************
* ��������	:  ClearDataBuf
* ��������	:  �������
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
void TMdbField::ClearDataBuf()
{
    m_sValue[0] = 0;
    m_pMemValue = NULL;
}
/******************************************************************************
* ��������	:  Name
* ��������	:  ��ȡ��field��name
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
char * TMdbField::GetName()
{
    if(NULL == m_pMemValue)
    {
        return NULL;
    }
    else
    {
        return m_pMemValue->sAlias;
    }
}

int TMdbField::GetDataType()
{
    if(NULL == m_pMemValue)
    {
        return -1;
    }
    else if(MemValueHasProperty(m_pMemValue,MEM_Int))
    {
        return DT_Int;
    }
    else
    {
        return DT_Char;
    }
}
void TMdbQuery::CheckError(const char* sSql) throw (TMdbException)
{

}

//����Query�ж��źŴ���
void TMdbQuery::SetSignal()
{
    //((TMdbTableCtrl*)m_pMdbTC)->SetSignal();
}
/******************************************************************************
* ��������	:  TMdbQuery
* ��������	:  ��ѯ�ӿڵĹ���
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
TMdbQuery::TMdbQuery(TMdbDatabase *pTMdbDatabase,int iSQLFlag)
{
    TADD_FUNC("Start.db=[%p],SQLFlag=[%d].",pTMdbDatabase,iSQLFlag);
    m_pMdb = pTMdbDatabase;
    if(NULL == m_pMdb)
    {
        ERROR_TO_THROW(ERR_DB_NOT_CREATE,"","pTMdbDatabase is NULL");
    }
    m_bSetSQL     = false;
    //memset(m_pszSQL,0x00,sizeof(m_pszSQL));
	m_pszSQL = new char[MAX_SQL_LEN];
	if(m_pszSQL == NULL)
	{
		ERROR_TO_THROW(ERR_OS_NO_MEMROY,m_pszSQL,"Mem Not Enough.");
	}
	memset(m_pszSQL, 0, MAX_SQL_LEN);
	m_iSQLBuffLen = MAX_SQL_LEN;
    m_pMdbSqlParser = NULL;
    m_pMdbSqlParser = new(std::nothrow) TMdbSqlParser();
    if(NULL == m_pMdbSqlParser)
    {
        ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pMdbSqlParser is NULL");
    }
    m_pExecuteEngine = NULL;
    m_pExecuteEngine = new(std::nothrow )TMdbExecuteEngine();
    if(NULL == m_pExecuteEngine)
    {
        ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pExecuteEngine is NULL");
    }
    m_pDDLExecuteEngine = NULL;
    
    
    m_iSetParamType = SET_PARAM_NONE;
    m_bSetList   = NULL;
    m_pParamArray= NULL;
    m_bFinishSet = false;
    m_bOpen      = false;
    m_bFillField = false;
    m_iOpenArrayPos = 0;
    m_fLastRecordFlag = false;
    m_iQueryFlag = 0;
    m_iRowsAff = 0;
    m_iParamComparePos = -1;
    m_iParamPoolCurPos = -1;
    m_pParamPool = NULL;
    //m_bIsDDLSQL = false;
    TADD_FUNC("Finish.");
}

/******************************************************************************
* ��������	:  ~TMdbQuery
* ��������	:  ��ѯ�ӿڵ�����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
TMdbQuery::~TMdbQuery()
{
    TADD_FUNC("Start.");
    CloseSQL();
    SAFE_DELETE(m_pMdbSqlParser);
    SAFE_DELETE(m_pExecuteEngine);
    SAFE_DELETE(m_pDDLExecuteEngine);
	SAFE_DELETE(m_pszSQL);
    TADD_FUNC("Finish.");
}

/******************************************************************************
* ��������	:  Close
* ��������	:  �ر�SQL��䣬��׼��������һ��sql���
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
void TMdbQuery::Close()
{
    CloseSQL();
}


/******************************************************************************
* ��������	:  Close
* ��������	:  �ر�SQL��䣬��׼��������һ��sql���
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
void TMdbQuery::CloseSQL()
{
    TADD_FUNC("Start.");
    m_bSetSQL = false;
    m_iQueryFlag = 0;
    size_t i = 0;
    for(i = 0; i<m_vField.size(); i++)
    {
        SAFE_DELETE(m_vField[i]);
    }
    m_vField.clear();
    if(m_pszSQL != NULL)    memset(m_pszSQL,0x00,m_iSQLBuffLen);
    SAFE_DELETE_ARRAY(m_bSetList);
    SAFE_DELETE_ARRAY(m_pParamArray);
    SAFE_DELETE_ARRAY(m_pParamPool);
    m_iSetParamType = SET_PARAM_NONE;
    m_bFinishSet = false;
    m_bOpen      = false;
    m_fLastRecordFlag = false;
    m_iRowsAff   = 0;
    m_iParamPoolCurPos = -1;
    m_iParamComparePos = -1;
    //m_bIsDDLSQL = false;
    
    TADD_FUNC("Finish.");
}

const char* TMdbQuery::GetSQL()
{
    return m_pszSQL;
}

/******************************************************************************
* ��������	:  CheckAndSetSql
* ��������	:  ��sql��һЩ��У�鲢����sql
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbQuery::CheckAndSetSql(const char * sSqlStatement) throw (TMdbException)
{
    int iRet = 0;
    if(m_pszSQL !=NULL && 0 != m_pszSQL[0])
    {
        TADD_WARNING("SetSQL again. OldSQL=[%s], NewSQL=[%s].", m_pszSQL, sSqlStatement);
    }
    int sqlLen = strlen(sSqlStatement);
    if(sqlLen >= m_iSQLBuffLen)
	{
		SAFE_DELETE(m_pszSQL);
		m_pszSQL = new char[sqlLen + 100];
		if(m_pszSQL == NULL)
		{
			ERROR_TO_THROW(ERR_OS_NO_MEMROY,m_pszSQL,"Mem Not Enough.");
		}
		memset(m_pszSQL, 0, sqlLen + 100);
		m_iSQLBuffLen = sqlLen + 100;
	}
    SAFESTRCPY(m_pszSQL,m_iSQLBuffLen,sSqlStatement);//����SQL���
    
    //��β���';' ��Ȼ����ʧ��
    TMdbNtcStrFunc::Trim(m_pszSQL,' ');
    int iLen = strlen(m_pszSQL);
    if(';' != m_pszSQL[iLen-1])
    {
        m_pszSQL[iLen] = ';';
        m_pszSQL[iLen + 1] = '\0';
    }
    return iRet;
}

/******************************************************************************
* ��������	:  SetDataSource
* ��������	:  ����������Դ
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
//void TMdbQuery::SetDataSource(int iSourceId)throw (TMdbException)
//{
//    if(m_pMdbSqlParser != NULL)
//    {
//        m_pMdbSqlParser->m_iSourceId = iSourceId;
//    }
//    else
//    {
//        ERROR_TO_THROW(ERR_SQL_INVALID,m_pszSQL,"m_pMdbSqlParser is NULL");
//    }
//}


int TMdbQuery::InitSqlBuff(bool & bFirstSet)
{
    if(m_pszSQL == NULL)
	{
		m_pszSQL = new char[MAX_SQL_LEN];
		if(m_pszSQL == NULL)
		{
			ERROR_TO_THROW(ERR_OS_NO_MEMROY,m_pszSQL,"Mem Not Enough.");
		}
		memset(m_pszSQL, 0, MAX_SQL_LEN);
		m_iSQLBuffLen = MAX_SQL_LEN;
		bFirstSet = true;
	}
	else if(0 == m_pszSQL[0])
	{
		bFirstSet = true;
	}
	return 0;
}


/******************************************************************************
* ��������	:  SetSQL
* ��������	:  ����ӿ�
* ����		:  sSqlStatement - sql��� iPreFetchRows - ��ʱ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/

void TMdbQuery::SetSQL(const char *sSqlStatement,int iPreFetchRows) throw (TMdbException)
{
    SetSQL(sSqlStatement,0,iPreFetchRows);
}


/******************************************************************************
* ��������	:  DealTableVersion
* ��������	:  �����İ汾����Ϣ�����ҷ����Ƿ�һ��
* ����		:  
* ����		:	true -- �汾һ��   false --�汾��һ��
* ���		:
* ����ֵ	:
* ����		:  yu.lianxiang
*******************************************************************************/
bool TMdbQuery::DealTableVersion()
{
	std::string sTableName =  m_pMdbSqlParser->m_stSqlStruct.pMdbTable->sTableName;
	int iTableVersion = m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iVersion;

	//�Ҳ����ͼ�һ��
	if(m_mapTableNameToVersion.find(sTableName) == m_mapTableNameToVersion.end())
	{
		m_mapTableNameToVersion[sTableName] = iTableVersion;
		TADD_NORMAL("table:[%s],Add version[%d] .",m_pMdbSqlParser->m_stSqlStruct.pMdbTable->sTableName,iTableVersion);
		return false;
	}

	//�ж��Ƿ�һ��
	int iTableVersionInQuery = m_mapTableNameToVersion[sTableName];
	if( iTableVersionInQuery == iTableVersion) 
	{
		return true;
	}
	else
	{
		TADD_NORMAL("table:[%s],version[%d] to [%d].",m_pMdbSqlParser->m_stSqlStruct.pMdbTable->sTableName,iTableVersionInQuery,iTableVersion);

		//ͬ��
		m_mapTableNameToVersion[sTableName] = iTableVersion;
		
		return false;
	}
}


/******************************************************************************
* ��������	: SetSQL
* ��������	:  ����Ҫִ�е�SQL
* ����		:  sSqlStatement - sql ��� - iFlag �Ƿ�����ͬ���ı�ʶ
iFlag -  QUERY_NO_ROLLBACK |QUERY_NO_ORAFLUSH|QUERY_NO_REPFLUSH|QUERY_NO_PEERFLUSH
Ĭ�������������
* ����		:  iPreFetchRows - ��ʱ����
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::SetSQL(const char *sSqlStatement,MDB_INT32 iFlag,int iPreFetchRows) throw (TMdbException)
{
    int iRet = 0;
    TADD_FUNC("SQL=[%s],flag=[%d],fetchRows=[%d].",sSqlStatement,iFlag,iPreFetchRows);
    if(m_pMdb == NULL || m_pMdb->IsConnect() == false || false == IsLinkOK())
    {
        ERROR_TO_THROW(ERR_DB_NOT_CONNECTED,sSqlStatement,"DB not connect. Please connect!");
    } 
    if(NULL == sSqlStatement)
    {
        ERROR_TO_THROW_NOSQL(ERR_DB_NOT_CONNECTED,"SQL cannot be NULL.");
    }
	
	bool bFirstSet = false;
	InitSqlBuff(bFirstSet);

	//�ж��Ƿ��б�Ҫ����sql���Ƿ��ظ���У��
    if(true == IsNeedToCheckSQLRepeat(bFirstSet,iFlag))
    {
    	char * sTempSQL = NULL;
	    sTempSQL = new char[m_iSQLBuffLen];
		memset(sTempSQL, 0, m_iSQLBuffLen);
		SAFESTRCPY(sTempSQL,m_iSQLBuffLen,m_pszSQL);
		
        m_pszSQL[0] = 0;		
        CheckAndSetSql(sSqlStatement);//��һЩ�򵥴����У��

		
    	//�ж��Ƿ��ظ�SetSQL,�����ظ��Ͳ��ظ�������ֱ������ϴ�ִ�н��
        if(TMdbNtcStrFunc::StrNoCaseCmp(m_pszSQL,sTempSQL) == 0)
        {
            m_pExecuteEngine->ClearLastExecute();
            m_bSetSQL = true;
            m_iSetParamType = SET_PARAM_NONE;
            m_bFinishSet = false;
            m_bOpen      = false;
            m_fLastRecordFlag = false;
            m_iRowsAff   = 0;
			SAFE_DELETE(sTempSQL);
            return;
        }
		SAFE_DELETE(sTempSQL);
    }
    CloseSQL();//�ȹر�sql �����ϴεĽ��
    m_iQueryFlag = iFlag;   
    CheckAndSetSql(sSqlStatement);//��һЩ�򵥴����У��
    TADD_DETAIL("sSql=%s", m_pszSQL);
    CHECK_RET_THROW(m_pMdbSqlParser->SetDB(m_pMdb->GetShmDsn(),m_pMdb->m_pConfig),
                    ERR_DB_NOT_CONNECTED,m_pszSQL,"[%s].",m_pMdbSqlParser->m_tError.GetErrMsg());
    CHECK_RET_THROW(m_pMdbSqlParser->ParseSQL(m_pszSQL),ERR_SQL_INVALID,
                    m_pszSQL,"[%s].",m_pMdbSqlParser->m_tError.GetErrMsg());
    
    TADD_DETAIL("SQL[%s],flag[%d].",m_pszSQL,m_iQueryFlag);
    //CheckDDLSQL();
    if(m_pMdbSqlParser->IsDDLSQL())
    {//DDL
        if(m_pMdb->m_pConfig->GetAccess() != MDB_ADMIN)
        {
            ERROR_TO_THROW(ERR_DB_PRIVILEGE_INVALID,m_pszSQL,
                           "Link-Access is [%d],trying to execute DDL SQL.",m_pMdb->m_pConfig->GetAccess());
        }
        if(NULL == m_pDDLExecuteEngine)
        {
            m_pDDLExecuteEngine = new(std::nothrow )TMdbDDLExecuteEngine();
            if(NULL == m_pDDLExecuteEngine)
            {
                ERROR_TO_THROW(ERR_OS_NO_MEMROY,m_pszSQL,"m_pDDLExecuteEngine is NULL");
            }
        }
        CHECK_RET_THROW(m_pDDLExecuteEngine->Init(m_pMdbSqlParser),m_pDDLExecuteEngine->m_tError.GetErrCode(),
                        m_pszSQL,"[%s].",m_pDDLExecuteEngine->m_tError.GetErrMsg());
        CHECK_RET_THROW(m_pDDLExecuteEngine->SetCurOperateUser(m_pMdb->GetUser()),m_pDDLExecuteEngine->m_tError.GetErrCode(),
                        m_pszSQL,"[%s].",m_pDDLExecuteEngine->m_tError.GetErrMsg());
        m_bSetSQL = true;
                        
        return;
    }
    
    if(TK_INSERT != m_pMdbSqlParser->m_stSqlStruct.iSqlType 
        && m_pMdbSqlParser->m_stSqlStruct.vIndexUsed.size() == 0
        && m_pMdbSqlParser->m_stSqlStruct.pMdbTable->bIsSysTab == false )
    {
        TADD_WARNING_NO_SCREEN("SQL[%s] do not use index.,scan all table!!",m_pszSQL);
    }
    IS_LOG(3)
    {
        //��ӡ������
        TSyntaxTreeAnalyse::Print(m_pMdbSqlParser,TLOG_DETAIL);
    }
    if(!IsCanAccess())//�ж��Ƿ���Ȩ��
    {
        ERROR_TO_THROW(ERR_DB_PRIVILEGE_INVALID,m_pszSQL,
                       "Link-Access is [%d],trying to operate system table[%s].",
                       m_pMdb->m_pConfig->GetAccess(),m_pMdbSqlParser->m_stSqlStruct.pMdbTable->sTableName);
    }
    CHECK_RET_THROW(m_pExecuteEngine->Init(m_pMdbSqlParser,m_iQueryFlag,m_pMdb->m_pLocalLink),m_pExecuteEngine->m_tError.GetErrCode(),
                    m_pszSQL,"[%s].",m_pExecuteEngine->m_tError.GetErrMsg());
    if(IsCanRollback())
    {
		m_pExecuteEngine->SetRollback(true);
    }
    PrepareParam();
    m_bSetSQL = true;
    TADD_FUNC("Finish.");
}

bool TMdbQuery::IsNeedToCheckSQLRepeat(bool bFirstSet, int iFlag)
{	
	bool bFlag=((false == m_pMdbSqlParser->IsDDLSQL()) && (true == m_bSetSQL) && (!bFirstSet )&& (iFlag == m_iQueryFlag));	
	return bFlag && DealTableVersion();
}

/******************************************************************************
* ��������	:  ExecuteDDLSQL
* ��������	:  ִ�����ݿ���SQL���
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::ExecuteDDLSQL()throw (TMdbException)
{
    struct timeval time1 = m_pMdb->GetShmDsn()->GetInfo()->tCurTime;
	int iRet = m_pDDLExecuteEngine->Execute();
    struct timeval time2 = m_pMdb->GetShmDsn()->GetInfo()->tCurTime;
    if(time2.tv_sec - time1.tv_sec >= m_pMdbSqlParser->m_pMdbConfig->GetDSN()->m_iLongSqlTime)
    {
        TADD_WARNING("SQL[%s],cost too much=[%ds],long-sql-time=[%ds].",m_pszSQL,time2.tv_sec - time1.tv_sec,
             m_pMdbSqlParser->m_pMdbConfig->GetDSN()->m_iLongSqlTime);
        TSyntaxTreeAnalyse::PrintArrMemValue(m_pMdbSqlParser->m_listInputVariable.vMemValue,TLOG_WARNING);
    }

    if(iRet != 0)
    {
        ERROR_TO_THROW(m_pDDLExecuteEngine->m_tError.GetErrCode(),m_pszSQL,"[%s]",m_pDDLExecuteEngine->m_tError.GetErrMsg());
    }
	
	#if 0
	CloseSQL();
	if(m_pMdb == NULL || m_pMdb->IsConnect() == false)
    {
		ERROR_TO_THROW(ERR_DB_NOT_CONNECTED,sSqlStatement,"DB not connect. Please connect!");
	}
	CheckAndSetSql(sSqlStatement);//��һЩ�򵥴����У��
	TADD_DETAIL("sSql=%s", m_pszSQL); 
	CHECK_RET_THROW(m_pMdbSqlParser->SetDB(m_pMdb->GetShmDsn(),m_pMdb->m_pConfig),
	    ERR_DB_NOT_CONNECTED,m_pszSQL,"[%s].",m_pMdbSqlParser->m_tError.GetErrMsg());
	CHECK_RET_THROW(m_pMdbSqlParser->ParseSQL(m_pszSQL),ERR_SQL_INVALID,
		m_pszSQL,"[%s].",m_pMdbSqlParser->m_tError.GetErrMsg());
	IS_LOG(1)
	{//��ӡ������
		TSyntaxTreeAnalyse::Print(m_pMdbSqlParser,TLOG_FLOW);
	}
	#endif

}

/******************************************************************************
* ��������	:  GetSQLType
* ��������	:  ��ȡsql ����
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbQuery::GetSQLType()
{
    if(NULL == m_pMdbSqlParser)
    {
        return 0;
    }
    return m_pMdbSqlParser->m_stSqlStruct.iSqlType;
}


/******************************************************************************
* ��������	:  AddError
* ��������	:  ����ͳ��
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::AddError()
{
    switch(m_pMdbSqlParser->m_stSqlStruct.iSqlType)
    {
    case TK_SELECT:
        ++m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iQueryFailCounts;
        break;
    case TK_INSERT:
        ++m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iInsertFailCounts;
        break;
    case TK_DELETE:
        ++m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iDeleteFailCounts;
        break;
    case TK_UPDATE:
        ++m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iUpdateFailCounts;
        break;
    }
}
/******************************************************************************
* ��������	:  AddSuccess
* ��������	:  ��ȷͳ��
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::AddSuccess()
{
    switch(m_pMdbSqlParser->m_stSqlStruct.iSqlType)
    {
    case TK_SELECT:
        ++m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iQueryCounts;
        break;
    case TK_INSERT:
        m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iInsertCounts += m_pExecuteEngine->GetRowsAffected();
        break;
    case TK_DELETE:
        m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iDeleteCounts += m_pExecuteEngine->GetRowsAffected();
        break;
    case TK_UPDATE:
        m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iUpdateCounts += m_pExecuteEngine->GetRowsAffected();
        break;
    }
}
/******************************************************************************
* ��������	:  IsFieldExist
* ��������	:  Field�Ƿ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
bool TMdbQuery::IsFieldExist(const char *fieldName)
{
    if(GetParamByName(fieldName)!= NULL)
    {
        return true;
    }
    return false;
}

/******************************************************************************
* ��������	:  GetParamIndexByName
* ��������	:  ����ParamName��ȡParamIndex
* ����		:
* ����		:
* ���		:
* ����ֵ	:����ȡ��������-1
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbQuery::GetParamIndexByName(const char *sParamName)
{
    TADD_DETAIL("sParamName = %s.",sParamName);
    ST_MEM_VALUE * pstMemValue = NULL;
    int i = 0;
    if(m_iParamComparePos > -1 && m_iParamComparePos < m_pMdbSqlParser->m_listInputVariable.iItemNum)
    {
        i = m_pParamPool[m_iParamComparePos];
        pstMemValue = m_pMdbSqlParser->m_listInputVariable.pValueArray[i];
        if(TMdbNtcStrFunc::StrNoCaseCmp(pstMemValue->sAlias,sParamName) == 0)
        {   
            TADD_FLOW("i=%d\n",i);
            m_iParamComparePos++;
            return i;
        }
    }
    bool bFind = false;
    for(i = 0; i < m_pMdbSqlParser->m_listInputVariable.iItemNum; ++i)
    {
        pstMemValue = m_pMdbSqlParser->m_listInputVariable.pValueArray[i];
        if(TMdbNtcStrFunc::StrNoCaseCmp(pstMemValue->sAlias,sParamName) == 0)
        {
            bFind = true;
            break;
        }
    }
    if(bFind)
    {
        TADD_FLOW("%d,i=%d\n",m_iParamPoolCurPos,i);
        
        m_iParamPoolCurPos++;
        if(m_iParamPoolCurPos < m_pMdbSqlParser->m_listInputVariable.iItemNum)
        {
            m_pParamPool[m_iParamPoolCurPos] = i;
        }
        return i;
    }
    return -1;
}
/******************************************************************************
* ��������	:  GetParamNameByIndex
* ��������	:  ����ParamName��ȡParamIndex
* ����		:
* ����		:
* ���		:
* ����ֵ	:����Ҳ����ͷ���null
* ����		:  jin.shaohua
*******************************************************************************/
const char * TMdbQuery::GetParamNameByIndex(int iIndex)
{
   ST_MEM_VALUE * pMemValue = GetParamByIndex(iIndex);
   if(NULL == pMemValue)
   {//û���ҵ�
        return NULL;
   }
   return pMemValue->sAlias;
}

void TMdbQuery::SetTimeStamp(long long iTimeStamp)
{   
    m_pMdbSqlParser->SetTimeStamp(iTimeStamp);
}

long long TMdbQuery::GetTimeStamp()
{
    return m_pExecuteEngine->GetRowTimeStamp();
}

void TMdbQuery::SetCancelPoint(int* pPoint)
{
	m_pExecuteEngine->SetCancelPoint(pPoint);
}


/******************************************************************************
* ��������	:  GetParamByName
* ��������	:  ����ParamName��ȡParam
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
ST_MEM_VALUE * TMdbQuery::GetParamByName(const char *sParamName)
{
    return GetParamByIndex(GetParamIndexByName(sParamName));;
}
/******************************************************************************
* ��������	:  GetParamByIndex
* ��������	:  ͨ��index��ȡ����ֵ
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
ST_MEM_VALUE * TMdbQuery::GetParamByIndex(int iIndex)
{
    if(!m_bSetSQL || iIndex >= m_pMdbSqlParser->m_listInputVariable.iItemNum||iIndex < 0)
    {
        return NULL;
    }
    else
    {
        return m_pMdbSqlParser->m_listInputVariable.pValueArray[iIndex];
    }
}
/******************************************************************************
* ��������	:  IsLinkOK
* ��������	:  ����Ƿ�������״̬
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::IsLinkOK()
{
    return m_pMdb->IsLinkOK();
}
/******************************************************************************
* ��������	:  Open
* ��������	:  �򿪽����
* ����		:  iPreFetchRows��ʵ������
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::Open(int prefetchRows) throw (TMdbException)
{
    TADD_FLOW("Start.SQL=[%s].m_iSetParamType=[%d]", m_pszSQL,m_iSetParamType);
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set.");
    }
    if(!IsLinkOK())
    {
        ERROR_TO_THROW(ERR_DB_NOT_CONNECTED,m_pszSQL,"Link is down.");
    }
    if(!IsAllSet())
    {
        ERROR_TO_THROW(ERR_SQL_PARAME_NOT_BOUND ,m_pszSQL,"Some param not be setted.");
    }
    m_bFinishSet = true;//����
    m_iRowsAff   = 0;//Ӱ���¼����λ
    switch(m_iSetParamType)
    {
    case SET_PARAM_ONE:
    case SET_PARAM_NONE:
        ExecuteOne();
        break;
    case SET_PARAM_ARRAY:
        OpenArray();
        break;
    default:
        ERROR_TO_THROW(ERR_SQL_PARAME_NOT_BOUND ,m_pszSQL,"m_iSetParamType[%d] error.",m_iSetParamType);
        break;
    }
    m_bOpen = true;
    TADD_FUNC("Finish.SQL=[%s].", m_pszSQL);
}
/******************************************************************************
* ��������	:  OpenArray
* ��������	:  ������
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool  TMdbQuery::OpenArray()throw(TMdbException)
{
    TADD_FUNC("start.");
    int i = 0;
    if(NULL == m_pParamArray || m_iOpenArrayPos >= m_pParamArray[0].m_iArraySize)
    {
        TADD_DETAIL("all params have been opened");
        return false;//�������õĲ�����ȫ��open����
    }
    int iParamCount = m_pMdbSqlParser->m_listInputVariable.iItemNum;
    TADD_DETAIL("iParamCount = [%d].",iParamCount);
    for(i = 0; i<iParamCount; i++)
    {
        if(m_pParamArray[i].m_tNullArrWrap.IsNull(m_iOpenArrayPos))
        {//null����
            SetParameterNULL(i);
        }
        else
        {
            switch(m_pParamArray[i].m_iParamType)
            {
            case MEM_Int:
                SetParameter(i,m_pParamArray[i].m_pllValue[m_iOpenArrayPos]);
                break;
            case MEM_Str:
                SetParameter(i,m_pParamArray[i].m_psValue[m_iOpenArrayPos]);
                break;
            default:
                ERROR_TO_THROW(ERR_SQL_PARAME_NOT_BOUND,m_pszSQL,"Param[%d]Type[%d] error.",i,
                               m_pParamArray[i].m_iParamType);
                break;
            }
        }
    }
    ExecuteOne();//һ�������������ִ��һ��
    ++ m_iOpenArrayPos;
    TADD_FUNC("Finish. m_iOpenArrayPos=[%d]",m_iOpenArrayPos);
    return true;
}
/******************************************************************************
* ��������	:  Next
* ��������	:  �ƶ�����һ����¼
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::Next()throw (TMdbException)
{
    TADD_FUNC("Start, SQL=[%s].", m_pszSQL);
    if(!m_bOpen)
    {
        //û��open��next ���쳣
        ERROR_TO_THROW(-1,m_pszSQL,"no open before Next...");
    }
    //ͳ��ǰ��ʱ���Ƿ񳬳�
    struct timeval time1 = m_pMdb->GetShmDsn()->GetInfo()->tCurTime;
    bool bFlag = false;
    int iRet = 0;
    iRet = m_pExecuteEngine->Next(bFlag);
    struct timeval time2 = m_pMdb->GetShmDsn()->GetInfo()->tCurTime;
    if(time2.tv_sec - time1.tv_sec >= m_pMdbSqlParser->m_pMdbConfig->GetDSN()->m_iLongSqlTime)
    {
        TADD_WARNING("SQL[%s],cost too much=[%ds],long-sql-time=[%ds].",m_pszSQL,time2.tv_sec - time1.tv_sec,
             m_pMdbSqlParser->m_pMdbConfig->GetDSN()->m_iLongSqlTime);
        TSyntaxTreeAnalyse::PrintArrMemValue(m_pMdbSqlParser->m_listInputVariable.vMemValue,TLOG_WARNING);
    }
    if(iRet != 0)
    {//�д���
        ERROR_TO_THROW(m_pExecuteEngine->m_tError.GetErrCode(),m_pszSQL,"[%s]",m_pExecuteEngine->m_tError.GetErrMsg());
    }
    TADD_DETAIL("Current-Next=[%d]",bFlag);
    if(!bFlag)
    {
        //û��next�ˣ��ʹ���һ����
        if(OpenArray())
        {
            return Next();
        }
    }
    else
    {
        m_iRowsAff ++;
    }
    m_bFillField = false;
    m_fLastRecordFlag = bFlag?false:true;
    TADD_FUNC("Finish, ret=[%d].",bFlag);
    return bFlag;
}

/******************************************************************************
* ��������	:  GetValue
* ��������	:  ֱ�ӻ�ȡ�ڴ�ֵ
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::GetValue(void *pStruct,int* Column)throw (TMdbException)
{
    if(m_pExecuteEngine->GetOneRowData(pStruct,Column) != 0)
    {
        ERROR_TO_THROW(m_pExecuteEngine->m_tError.GetErrCode(),m_pszSQL,"[%s]",m_pExecuteEngine->m_tError.GetErrMsg());
    }
}


void TMdbQuery::GetValue(TMdbColumnAddr* pTColumnAddr)throw (TMdbException)//ֱ�ӻ�ȡֵ
{
	if(m_pExecuteEngine->GetOneRowData(pTColumnAddr) != 0)
	{
	    ERROR_TO_THROW(m_pExecuteEngine->m_tError.GetErrCode(),m_pszSQL,"[%s]",m_pExecuteEngine->m_tError.GetErrMsg());
	}

}
/******************************************************************************
* ��������	:FillOutputToField
* ��������	:   ����ȡ�����ڴ�ֵ��䵽Field
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbQuery::FillOutputToField()
{
    TADD_FUNC("Start.Field Size=[%d]",m_pMdbSqlParser->m_listOutputCollist.iItemNum);
    int i = 0;
    int iSize = m_pMdbSqlParser->m_listOutputCollist.iItemNum;
    for(i = 0; i<iSize; i++)
    {
        ST_MEM_VALUE * pMemValue = m_pMdbSqlParser->m_listOutputCollist.pValueArray[i];
        TMdbField * pField = new(std::nothrow) TMdbField();
        if(pField == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Out of Memory.");
            throw  TMdbException(ERR_OS_NO_MEMROY,m_pszSQL, "File=[%s], Line=[%d], Out of Memory.", __FILE__, __LINE__);
        }
        pField->ClearDataBuf();
        pField->m_pMemValue = pMemValue;//
        m_vField.push_back(pField);
    }
    TADD_FUNC("Finish.");
    return 0;
}

//TODO :�Ƿ�����ı�ʶ��
bool TMdbQuery::Eof(void)
{
    TADD_DETAIL("%s.", m_fLastRecordFlag?"TRUE":"FALSE");
    return m_fLastRecordFlag;
}
/******************************************************************************
* ��������	:  GetInputMemValue
* ��������	:  ��ȡ��Ҫ����MemValueֵ
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
ST_MEM_VALUE_LIST * TMdbQuery::GetInputMemValue()
{
    return &(m_pMdbSqlParser->m_listInputVariable);
}

/******************************************************************************
* ��������	:  Execute
* ��������	:  ִ��SQL
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::Execute(int iExecuteRows) throw (TMdbException)
{
    TADD_FLOW("Start, SQL=[%s].m_iSetParamType = [%d],iExecuteRows=[%d]", m_pszSQL,m_iSetParamType,iExecuteRows );
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set.");
    }
    if(!IsLinkOK())
    {
        ERROR_TO_THROW(ERR_DB_NOT_CONNECTED,m_pszSQL,"Link is down.");
    }
    if(!IsAllSet())
    {
        ERROR_TO_THROW(ERR_SQL_PARAME_NOT_BOUND ,m_pszSQL,"Some param not be setted.");
    }
    m_bFinishSet = true;//����
    m_iRowsAff   = 0;//Ӱ���¼����λ
    switch(m_iSetParamType)
    {
    case SET_PARAM_ONE:
    case SET_PARAM_NONE:
        if(m_pMdbSqlParser->IsDDLSQL())
        {
            ExecuteDDLSQL();
        }
        else
        {
            ExecuteOne();
        }
        break;
    case SET_PARAM_ARRAY:
        ExecuteArray(iExecuteRows);
        break;
    default:
        ERROR_TO_THROW(ERR_SQL_PARAME_NOT_BOUND ,m_pszSQL,"m_iSetParamType[%d] error.",m_iSetParamType);
        break;
    }
    TADD_FLOW("Finish, row effect =[%d].", RowsAffected());
    return true;
}

/******************************************************************************
* ��������	:  ExecuteOne
* ��������	:  ����ִ��
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::ExecuteOne()throw (TMdbException)
{
    struct timeval time1 = m_pMdb->GetShmDsn()->GetInfo()->tCurTime;
    int iRet = m_pExecuteEngine->Execute();
    struct timeval time2 = m_pMdb->GetShmDsn()->GetInfo()->tCurTime;
    if(time2.tv_sec - time1.tv_sec >= m_pMdbSqlParser->m_pMdbConfig->GetDSN()->m_iLongSqlTime)
    {
        TADD_WARNING("SQL[%s],cost too much=[%ds],long-sql-time=[%ds].",m_pszSQL,time2.tv_sec - time1.tv_sec,
             m_pMdbSqlParser->m_pMdbConfig->GetDSN()->m_iLongSqlTime);
        TSyntaxTreeAnalyse::PrintArrMemValue(m_pMdbSqlParser->m_listInputVariable.vMemValue,TLOG_WARNING);
    }
    if(iRet != 0)
    {
        AddError();
        ERROR_TO_THROW(m_pExecuteEngine->m_tError.GetErrCode(),m_pszSQL,"[%s]",m_pExecuteEngine->m_tError.GetErrMsg());
    }
    AddSuccess();
    m_iRowsAff += m_pExecuteEngine->GetRowsAffected();
    m_iParamComparePos = 0;
    m_iParamPoolCurPos = -1;
    return true;
}

/******************************************************************************
* ��������	:  ExecuteArray
* ��������	:  ����ִ��
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::ExecuteArray(int iExecuteRows)throw (TMdbException)
{
    int i = 0;
    int j = 0;
    int iDataCount  = m_pParamArray[0].m_iArraySize;
    if(iExecuteRows > 0)
    {
        iDataCount = iExecuteRows < iDataCount ?iExecuteRows:iDataCount;//ִ�н�С��
    }
    int iParamCount = m_pMdbSqlParser->m_listInputVariable.iItemNum;
    TADD_FUNC("Start.iDataCount[%d],iParamCount[%d].",iDataCount,iParamCount);

    try
    {
        for(i = 0; i<iDataCount; ++i)
        {
            for(j = 0; j<iParamCount; j++)
            {
                TADD_DETAIL("SetParameter(%d).",j);
                if(m_pParamArray[j].m_tNullArrWrap.IsNull(i))
                {//null����
                    SetParameterNULL(j);
                }
                else
                {
                    switch(m_pParamArray[j].m_iParamType)
                    {
                    case MEM_Int:
                        SetParameter(j,m_pParamArray[j].m_pllValue[i]);
                        break;
                    case MEM_Str:
                        SetParameter(j,m_pParamArray[j].m_psValue[i]);
                        break;
                    default:
                        ERROR_TO_THROW(ERR_SQL_PARAME_NOT_BOUND,m_pszSQL,"Param[%d]Type[%d] error.",j,
                                       m_pParamArray[j].m_iParamType);
                        break;
                    }
                }
            }
            TADD_DETAIL("Execute array(%d).",i);
            ExecuteOne();//һ�������������ִ��һ��
        }
    }
    catch(TMdbException& e)
    {
        throw;
    }
    catch(...)
    {
        ERROR_TO_THROW(ERR_APP_INVALID_PARAM,m_pszSQL,"Unknown error!");
    }
    TADD_FUNC("Finish.");
    return true;
}

/******************************************************************************
* ��������	:  TransBegin
* ��������	:  ������
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::TransBegin()
{
    m_pMdb->TransBegin();
    return true;
}

/******************************************************************************
* ��������	:  Commit
* ��������	:  �����ύ
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::Commit()
{
    m_pMdb->Commit();
    return true;
}

/******************************************************************************
* ��������	:  Commit
* ��������	:  ����ع�
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::Rollback()
{
    m_pMdb->Rollback();
    return true;
}
int TMdbQuery::RowsAffected()
{
    return m_iRowsAff;
}

/******************************************************************************
* ��������	:  FieldCount
* ��������	:  ��ȡ�и���
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbQuery::FieldCount()
{
    return m_pMdbSqlParser->m_listOutputCollist.iItemNum;
}
/******************************************************************************
* ��������	:  ParamCount
* ��������	:  ��ȡ�󶨲�������
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbQuery::ParamCount()
{
    return m_pMdbSqlParser->m_listInputVariable.iItemNum;
}


/******************************************************************************
* ��������	:  IsCanAccess
* ��������	:  �Ƿ���з���Ȩ��
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true /false
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::IsCanAccess()
{
    if(TK_SELECT == m_pMdbSqlParser->m_stSqlStruct.iSqlType)
    {
        //��ѯ�Ļ������б�����
        return true;
    }
    else
    {
        if(m_pMdb->m_pConfig->GetAccess() == MDB_READ)
        {
            //ֻ��selectȨ��
            return false;
        }
        if(m_pMdbSqlParser->m_stSqlStruct.pMdbTable->bIsSysTab &&
                m_pMdb->m_pConfig->GetAccess() != MDB_ADMIN)
        {
            return false;
        }
    }
    return true;
}

/******************************************************************************
* ��������	:  PrepareParam
* ��������	:  ׼����param��صĹ���
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int  TMdbQuery::PrepareParam()
{
    int iRet = 0;
    TADD_FUNC("Start.");
    if(m_pMdbSqlParser->m_listInputVariable.iItemNum > 0)
    {
        SAFE_DELETE_ARRAY(m_bSetList);
        SAFE_DELETE_ARRAY(m_pParamArray);
        SAFE_DELETE_ARRAY(m_pParamPool);
        m_iParamPoolCurPos = -1;
        m_pParamPool = new(std::nothrow) char[m_pMdbSqlParser->m_listInputVariable.iItemNum];
        memset(m_pParamPool,0x0,m_pMdbSqlParser->m_listInputVariable.iItemNum);
        m_bSetList = new bool[m_pMdbSqlParser->m_listInputVariable.iItemNum];//����"�Ƿ������õ�list"
        memset(m_bSetList,0x00,m_pMdbSqlParser->m_listInputVariable.iItemNum * sizeof(bool));
        m_pParamArray = new TMdbParamArray[m_pMdbSqlParser->m_listInputVariable.iItemNum];
    }
    TADD_FUNC("Finish.Param size=[%d]",m_pMdbSqlParser->m_listInputVariable.iItemNum);
    return iRet;
}

/******************************************************************************
* ��������	:  IsAllSet
* ��������	:  �Ƿ�ȫ���󶨱������Ѿ�����
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::IsAllSet()
{
    if(NULL == m_bSetList)
    {
        //û�а󶨱���
        return true;
    }
    else
    {
        int i = 0;
        for(i = 0 ; i< m_pMdbSqlParser->m_listInputVariable.iItemNum; i++)
        {
            if(false == m_bSetList[i])
            {
                return false;
            }
        }
    }
    return true;
}

/******************************************************************************
* ��������	:  FieldPos
* ��������	:  sFieldName - field��
* ����		:  pos - λ�� tField ָ��
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
TMdbField& TMdbQuery::FieldPos(const char *sFieldName,int &pos,TMdbField** tField) throw (TMdbException)
{
    if(m_vField.size() == 0)
    {
        FillOutputToField();
    }
    if(!m_bFillField)
    {
        //���fieldֵ��������ӳټ��㲢���
        //m_pExecuteEngine->FillCollist();
        int iRet = 0;
        CHECK_RET_THROW(m_pExecuteEngine->FillCollist(),m_pExecuteEngine->m_tError.GetErrCode(),m_pszSQL,"[%s].",m_pExecuteEngine->m_tError.GetErrMsg());
        m_bFillField = true;
    }
    int i = 0;
    std::vector<TMdbField * >::iterator itor = m_vField.begin();
    for(; itor != m_vField.end(); ++itor)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(sFieldName,(*itor)->m_pMemValue->sAlias)==0)
        {
            pos = i;
            *tField = &Field(i);
            return Field(i);
        }
        ++i;
    }
    ERROR_TO_THROW(ERR_OS_NO_MEMROY,"","not find Field[%s]",sFieldName);
}


bool TMdbQuery::IsParamExist(const char *paramName)
{
    return GetParamIndexByName(paramName) < 0?false:true;
}

/******************************************************************************
* ��������	:  Field
* ��������	:  ����������ȡ��iIndex����ʵ��,��0��ʼ
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
TMdbField& TMdbQuery::Field(int iIndex) throw (TMdbException)
{
    TADD_FUNC("Index[%d].",iIndex);
    if(m_vField.size() == 0)
    {
        FillOutputToField();
    }
    if(!m_bFillField)
    {
        //���fieldֵ��������ӳټ��㲢���
        //TODO:���Լ�ⷵ��ֵ�������쳣
        //m_pExecuteEngine->FillCollist();
        int iRet = 0;
        CHECK_RET_THROW(m_pExecuteEngine->FillCollist(),m_pExecuteEngine->m_tError.GetErrCode(),m_pszSQL,"[%s].",m_pExecuteEngine->m_tError.GetErrMsg());
        m_bFillField = true;
    }
    if(iIndex >= 0 && iIndex < (int)m_vField.size())
    {
        return *m_vField[iIndex];
    }
    else
    {
		ERROR_TO_THROW(ERR_SQL_COLUMN_INDEX_INVALID,"","not find field[%d], please check if enough colunms been selected.",iIndex);
    }
    return *m_vField[iIndex];
}

/******************************************************************************
* ��������	:  Field
* ��������	:  ����������ȡ��ʵ��
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
TMdbField& TMdbQuery::Field(const char *sFieldName) throw (TMdbException)
{
    TADD_FUNC("Name[%s].",sFieldName);
    if(m_vField.size() == 0)
    {
        FillOutputToField();
    }
    if(!m_bFillField)
    {
        //���fieldֵ
        //m_pExecuteEngine->FillCollist();
        int iRet = 0;
        CHECK_RET_THROW(m_pExecuteEngine->FillCollist(),m_pExecuteEngine->m_tError.GetErrCode(),m_pszSQL,"[%s].",m_pExecuteEngine->m_tError.GetErrMsg());
        m_bFillField = true;
    }
    std::vector<TMdbField * >::iterator itor = m_vField.begin();
    for(; itor != m_vField.end(); ++itor)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(sFieldName,(*itor)->m_pMemValue->sAlias)==0)
        {
            return *(*itor);
        }
    }
    ERROR_TO_THROW(ERR_TAB_COLUMN_NOT_EXIST,m_pszSQL,"not find field[%s]",sFieldName);
}

/******************************************************************************
* ��������	:  CheckAndSetParamType
* ��������	:  ��鲢����paramtype
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::CheckAndSetParamType(int iType)throw (TMdbException)
{
    TADD_DETAIL("m_iSetParamType = %d,Type = %d,m_bFinishSet = %d",m_iSetParamType,iType,m_bFinishSet);
    if(SET_PARAM_NONE == m_iSetParamType || m_bFinishSet)
    {
        m_iSetParamType = iType;
    }
    else if(iType != m_iSetParamType)
    {
        ERROR_TO_THROW(ERR_SQL_SETPARAMETER_TYPE,m_pszSQL,"SetParameter last type[%d] != new type[%d]",
                       m_iSetParamType,iType);
    }
    m_iOpenArrayPos = 0;//set��ʱ���λΪ0
    m_bOpen   = false;
    return true;
}
/******************************************************************************
* ��������	:  IsCanRollback
* ��������	:  �Ƿ���Իع�
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbQuery::IsCanRollback()
{
    if(m_pMdb->m_bIsNeedCommit && !QueryHasProperty(m_iQueryFlag,QUERY_NO_ROLLBACK) &&
            m_pMdbSqlParser->m_stSqlStruct.pMdbTable->bRollBack &&
            m_pMdbSqlParser->m_stSqlStruct.iSqlType != TK_SELECT)
    {
        return true;
    }
    return false;
}

/******************************************************************************
* ��������	:  SetParameter
* ��������	:  ���ò���ֵ
* ����		:
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::SetParameter(const char *sParamName,const char* sParamValue, bool isOutput_Unused) throw (TMdbException)
{
    TADD_FLOW("(sParamName=[%s], sParamValue=[%s]) : Start.", sParamName, sParamValue);
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s],sParamValue=[%s].", 
                sParamName?sParamName:"NULL", sParamValue?sParamValue:"NULL");
    }
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);
    }
    SetParameter(iIndex,sParamValue);
    TADD_FUNC("Finish.");
}
void TMdbQuery::SetParameter(const char *sParamName, const char cParamValue, bool isOutput) throw (TMdbException)
{
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s],cParamValue=[%c].", 
                sParamName?sParamName:"NULL", cParamValue);
    }
    TADD_FLOW("(sParamName=[%s], cParamValue=[%c]) : Start.", sParamName, cParamValue);
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);
    }
    SetParameter(iIndex,cParamValue);
    TADD_FUNC("Finish.");
}
void TMdbQuery::SetParameter(const char *sParamName,int iParamValue, bool isOutput_Unused) throw (TMdbException)
{
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s],iParamValue=[%d].", 
                sParamName?sParamName:"NULL", iParamValue);
    }
    TADD_FLOW("(sParamName=[%s], iParamValue=[%d]) : Start.", sParamName, iParamValue);
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);
    }
    SetParameter(iIndex,iParamValue);
    TADD_FUNC("Finish.");
}
void TMdbQuery::SetParameter(const char *sParamName,long lParamValue, bool isOutput_Unused) throw (TMdbException)
{
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s],lParamValue=[%ld].", 
                sParamName?sParamName:"NULL", lParamValue);
    }
    TADD_FLOW("(sParamName=[%s], iParamValue=[%ld]) : Start.", sParamName, lParamValue);
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);
    }
    SetParameter(iIndex,lParamValue);
    TADD_FUNC("Finish.");
}
void TMdbQuery::SetParameter(const char *sParamName,double dParamValue, bool isOutput_Unused) throw (TMdbException)
{
    TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMdbQuery::SetParameter()    SetParameter() failed, Don't support FLOAT",__FILE__,__LINE__);
    throw TMdbException(ERR_TAB_COLUMN_DATA_TYPE_INVALID,m_pszSQL, "File=[%s], Line=[%d], SetParameter() failed, Don't support FLOAT.", __FILE__, __LINE__);
}
void TMdbQuery::SetParameter(const char *sParamName,long long llParamValue, bool isOutput_Unused) throw (TMdbException)
{
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s],llParamValue=[%lld].", 
                sParamName?sParamName:"NULL", llParamValue);
    }
    TADD_FLOW("(sParamName=[%s], iParamValue=[%lld]) : Start.", sParamName, llParamValue);
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);
    }
    SetParameter(iIndex,llParamValue);
    TADD_FUNC("Finish.");
}
void TMdbQuery::SetParameter(const char *sParamName,const char* sParamValue,int iBufferLen, bool isOutput_Unused) throw (TMdbException)//���ڴ���BLOB/BINARY�����ֶ�
{
    TADD_FLOW("(sParamName=[%s], ValueSize=[%d]) : Start.", sParamName, iBufferLen);
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s],sParamValue=[%s],iBufferLen=[%d].", 
                sParamName?sParamName:"NULL",sParamValue?sParamValue:"NULL",iBufferLen);
    }
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);

    }
    SetParameter(iIndex,sParamValue,iBufferLen);
    TADD_FUNC("Finish.");
}
/******************************************************************************
* ��������	:  SetParameterNULL
* ��������	:  ���ò���Ϊ��
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::SetParameterNULL(const char *sParamName) throw (TMdbException)  
{
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s].", 
                sParamName?sParamName:"NULL");
    }
    TADD_FLOW("sParamName=[%s].", sParamName);
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);
    }
    SetParameterNULL(iIndex);
    TADD_FUNC("Finish.");
}
/******************************************************************************
* ��������	:  SetParameter
* ��������	:  ���ð󶨱���
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::SetParameter(int iParamIndex,const char* sParamValue) throw (TMdbException)
{
    TADD_FLOW("index(%d),paramValue=[%s].",iParamIndex,sParamValue);
    int iRet =0;
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,iParamIndex=[%d],sParamValue=[%s].", 
                iParamIndex,sParamValue?sParamValue:"NULL");
    }
    ST_MEM_VALUE * pMemValue = GetParamByIndex(iParamIndex);
    if(NULL != pMemValue)
    {
        //find
        if(MemValueHasAnyProperty(pMemValue,MEM_Str|MEM_Date))
        {
            if((int)strlen(sParamValue) > pMemValue->iSize - 1)
            {
                CHECK_RET_THROW(-1,ERR_SQL_DATA_LEN_ERROR,m_pszSQL,"ParamIndex(%d) type size(%d) > memsize(%d),value=[%s]",
                                iParamIndex,strlen(sParamValue),pMemValue->iSize - 1,sParamValue);
            }
            else
            {
                pMemValue->ClearValue();
                SAFESTRCPY(pMemValue->sValue,pMemValue->iSize,sParamValue);
            }
        }
        else if(TMdbNtcStrFunc::IsDigital(sParamValue))
        {
            pMemValue->ClearValue();
            pMemValue->lValue = TMdbNtcStrFunc::StrToInt(sParamValue);
        }
        else
        {
            CHECK_RET_THROW(-1,ERR_SQL_DATA_TYPE_INVALID,m_pszSQL,"ParamIndex(%d) type is not string,value=[%s]",iParamIndex,sParamValue);
        }

    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"SetParameter() failed,  Invalid-ParamIndex(%d)",iParamIndex);
        throw TMdbException(ERR_SQL_PARAM_INDEX_INVALID,m_pszSQL,
                            "File=[%s], Line=[%d], SetParameter() failed, Invalid-ParamIndex(%d).", __FILE__, __LINE__, iParamIndex);
    }
    m_bSetList[iParamIndex] = true;
    CheckAndSetParamType(SET_PARAM_ONE);
}

void TMdbQuery::SetParameter(int iParamIndex, const char cParamValue) throw (TMdbException)
{
    char sTemp[2] = {0};
    sTemp[0] = cParamValue;
    SetParameter(iParamIndex,sTemp);
}
void TMdbQuery::SetParameter(int iParamIndex,int iParamValue) throw (TMdbException)
{
    SetParameter(iParamIndex,(long long)iParamValue);
}
void TMdbQuery::SetParameter(int iParamIndex,long lParamValue) throw (TMdbException)
{
    SetParameter(iParamIndex,(long long)lParamValue);
}
void TMdbQuery::SetParameter(int iParamIndex,double dParamValue) throw (TMdbException)
{
    TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TMdbQuery::SetParameter()  SetParameter() failed, Param in Set-List(Sorry, can't support double).",__FILE__,__LINE__);
    throw TMdbException(ERR_TAB_COLUMN_DATA_TYPE_INVALID,m_pszSQL, "File=[%s], Line=[%d], SetParameter() failed, Param in Set-List(Sorry, can't support double).", __FILE__, __LINE__);
}

/******************************************************************************
* ��������	:  SetParameter
* ��������	:  ���ð󶨱���
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::SetParameter(int iParamIndex,long long llParamValue) throw (TMdbException)
{
    TADD_FLOW("index(%d),paramValue=[%lld].",iParamIndex,llParamValue);
    int iRet =0;
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,iParamIndex=[%d],llParamValue=[%lld].", 
                iParamIndex,llParamValue);
    }
    ST_MEM_VALUE * pMemValue = GetParamByIndex(iParamIndex);
    if(NULL != pMemValue)
    {
        //find
        if(MemValueHasProperty(pMemValue,MEM_Int))
        {
            pMemValue->ClearValue();
            pMemValue->lValue = llParamValue;
        }
        else
        {
            CHECK_RET_THROW(-1,ERR_SQL_DATA_TYPE_INVALID,m_pszSQL,"ParamIndex(%d) type is not number",iParamIndex);
        }
    }
    else
    {
        TADD_ERROR(ERROR_UNKNOWN,"SetParameter() failed,  Invalid-ParamIndex(%d)", iParamIndex);
        throw TMdbException(ERR_SQL_PARAM_INDEX_INVALID,m_pszSQL,
                            "File=[%s], Line=[%d], SetParameter() failed, Invalid-ParamIndex(%d).", __FILE__, __LINE__, iParamIndex);
    }
    m_bSetList[iParamIndex] = true;
    CheckAndSetParamType(SET_PARAM_ONE);
}

/******************************************************************************
* ��������	:  SetParameter
* ��������	:  ���ð󶨱���
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::SetParameter(int iParamIndex,const char* sParamValue,int iBufferLen) throw (TMdbException)//���ڴ���BLOB/BINARY�����ֶ�
{
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,iParamIndex=[%d],sParamValue=[%s],iBufferLen=[%d].", 
                iParamIndex,sParamValue?sParamValue:"NULL",iBufferLen);
    }
    ST_MEM_VALUE * pMemValue = GetParamByIndex(iParamIndex);
    if(NULL != pMemValue)
    {
        //find
        std::string encoded = Base::base64_encode(reinterpret_cast<const unsigned char*>(sParamValue),iBufferLen);

        if((int)encoded.length() >= pMemValue->iSize)
        {
            ERROR_TO_THROW(ERR_SQL_PARAM_INDEX_INVALID,m_pszSQL,
                "File=[%s], Line=[%d], SetParameter() failed, ParamIndex(%d) type size(%d) > memsize(%d).",
                __FILE__, __LINE__, iParamIndex,encoded.length(),pMemValue->iSize);
        }
        else
        {
            pMemValue->ClearValue();
            SAFESTRCPY(pMemValue->sValue,pMemValue->iSize,encoded.c_str());
        }
        //pMemValue->sValue = QMDB_MALLOC->CopyFromStr(encoded.c_str());
        //pMemValue->iFlags|= (MEM_Str);
        //pMemValue->iSize  = encoded.length();
    }
    else
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamIndex(%d)", iParamIndex);
    }
    m_bSetList[iParamIndex] = true;
    CheckAndSetParamType(SET_PARAM_ONE);
}

/******************************************************************************
* ��������	:  SetParameterNULL
* ��������	:  ���ò���Ϊ��
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbQuery::SetParameterNULL(int iParamIndex) throw (TMdbException)  
{
    TADD_DETAIL("index(%d)",iParamIndex);
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,iParamIndex=[%d].",iParamIndex);
    }
    ST_MEM_VALUE * pMemValue = GetParamByIndex(iParamIndex);
    if(NULL != pMemValue)
    {
        pMemValue->ClearValue();
        pMemValue->SetNull();
    }
    else
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_INDEX_INVALID,m_pszSQL,"SetParameterNULL() failed,  Invalid-ParamIndex(%d)", iParamIndex);
    }
    m_bSetList[iParamIndex] = true;
    CheckAndSetParamType(SET_PARAM_ONE);
}
//�����������ֵ
void TMdbQuery::SetParamArray(const char *sParamName,char **asParamValue,int iInterval,
                              int iElementSize,int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s].",sParamName?sParamName:"NULL");
    }
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);
    }
    SetParamArray(iIndex,asParamValue,iInterval,iElementSize,iArraySize,bOutput,bNullArr);
}
void TMdbQuery::SetParamArray(const char *sParamName,int *aiParamValue,int iInterval,
                              int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s].",sParamName?sParamName:"NULL");
    }
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);
    }
    SetParamArray(iIndex,aiParamValue,iInterval,iArraySize,bOutput,bNullArr);
}
void TMdbQuery::SetParamArray(const char *sParamName,long *alParamValue,int iInterval,
                            int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s].",sParamName?sParamName:"NULL");
    }
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);
    }
    SetParamArray(iIndex,alParamValue,iInterval,iArraySize,bOutput,bNullArr);
}
void TMdbQuery::SetParamArray(const char *sParamName,double *adParamValue,int iInterval,
                            int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    ERROR_TO_THROW(ERR_SQL_FLOAT_PARAM_NOT_SUPPORT_BIND,m_pszSQL,"Don't support FLOAT.");
}
void TMdbQuery::SetParamArray(const char *sParamName,long long *allParamValue,int iInterval,
                            int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,sParamName=[%s].",sParamName?sParamName:"NULL");
    }
    int iIndex = GetParamIndexByName(sParamName);
    if(iIndex < 0)
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_NAME_NOT_EXIST,m_pszSQL,"Invalid-ParamName(%s)", sParamName);
    }
    SetParamArray(iIndex,allParamValue,iInterval,iArraySize,bOutput,bNullArr);
}
void TMdbQuery::SetBlobParamArray(const char *sParamName,char *sParamValue,int iBufferLen,int iArraySize,bool bOutput,bool * bNull) throw (TMdbException)//���ڴ���BLOB/BINARY�����ֶ��������
{
    ERROR_TO_THROW(ERR_SQL_BLOB_PARAM_NOT_SUPPORT_BIND,m_pszSQL,"Can't support the function(Blob).");
}

//����index���ð󶨱���
void TMdbQuery::SetParamArray(int iParamIndex,char **asParamValue,int iInterval,
                              int iElementSize,int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    int iRet = 0;
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,iParamIndex=[%d].",iParamIndex);
    }
    ST_MEM_VALUE * pMemValue = GetParamByIndex(iParamIndex);
    if(NULL != pMemValue)
    {
        //find
        TMdbParamArray & tParamArray = m_pParamArray[iParamIndex];
        CHECK_RET_THROW(tParamArray.ReAlloc(iArraySize/iInterval,MEM_Str),
                        ERR_SQL_SETPARAMETER_TYPE,m_pszSQL,"ReAlloc(%lld,%d)",iArraySize/iInterval,MEM_Str);
        tParamArray.m_iParamIndex = iParamIndex;
        int i = 0;
        for(i = 0; i<tParamArray.m_iArraySize ; ++i)
        {
            tParamArray.m_psValue[i] =  (char *)asParamValue + iElementSize * i;
        }
        tParamArray.m_tNullArrWrap.Init(bNullArr,tParamArray.m_iArraySize);
    }
    else
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_INDEX_INVALID,m_pszSQL,"Invalid-ParamIndex(%d)",iParamIndex);
    }
    m_bSetList[iParamIndex] = true;
    CheckAndSetParamType(SET_PARAM_ARRAY);
}
void TMdbQuery::SetParamArray(int iParamIndex,int *aiParamValue,int iInterval,
                              int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    int iRet =0;
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,iParamIndex=[%d].",iParamIndex);
    }
    ST_MEM_VALUE * pMemValue = GetParamByIndex(iParamIndex);
    if(NULL != pMemValue)
    {
        //find
        TMdbParamArray & tParamArray = m_pParamArray[iParamIndex];
        CHECK_RET_THROW(tParamArray.ReAlloc(iArraySize/iInterval,MEM_Int),
                        ERR_SQL_SETPARAMETER_TYPE,m_pszSQL,"ReAlloc(%lld,%d)",iArraySize/iInterval,MEM_Int);
        tParamArray.m_iParamIndex = iParamIndex;
        int i = 0;
        for(i = 0; i<tParamArray.m_iArraySize ; ++i)
        {
            tParamArray.m_pllValue[i] = (MDB_INT64)*(aiParamValue + i);
        }
        tParamArray.m_tNullArrWrap.Init(bNullArr,tParamArray.m_iArraySize);
    }
    else
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_INDEX_INVALID,m_pszSQL,"Invalid-ParamIndex(%d)",iParamIndex);
    }
    m_bSetList[iParamIndex] = true;
    CheckAndSetParamType(SET_PARAM_ARRAY);

}
void TMdbQuery::SetParamArray(int iParamIndex,long *alParamValue,int iInterval,
                              int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    int iRet = 0;
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,iParamIndex=[%d].",iParamIndex);
    }
    ST_MEM_VALUE * pMemValue = GetParamByIndex(iParamIndex);
    if(NULL != pMemValue)
    {
        //find
        TMdbParamArray & tParamArray = m_pParamArray[iParamIndex];
        CHECK_RET_THROW(tParamArray.ReAlloc(iArraySize/iInterval,MEM_Int),
                        ERR_SQL_SETPARAMETER_TYPE,m_pszSQL,"ReAlloc(%lld,%d)",iArraySize/iInterval,MEM_Int);
        tParamArray.m_iParamIndex = iParamIndex;
        int i = 0;
        for(i = 0; i<tParamArray.m_iArraySize ; ++i)
        {
            tParamArray.m_pllValue[i] = (MDB_INT64)*(alParamValue + i);
        }
        tParamArray.m_tNullArrWrap.Init(bNullArr,tParamArray.m_iArraySize);
    }
    else
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_INDEX_INVALID,m_pszSQL,"Invalid-ParamIndex(%d)",iParamIndex);
    }
    m_bSetList[iParamIndex] = true;
    CheckAndSetParamType(SET_PARAM_ARRAY);
}
void TMdbQuery::SetParamArray(int iParamIndex,double *adParamValue,int iInterval,
                              int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    ERROR_TO_THROW(ERR_SQL_FLOAT_PARAM_NOT_SUPPORT_BIND,m_pszSQL,"Don't support FLOAT.");

}
void TMdbQuery::SetParamArray(int iParamIndex,long long *allParamValue,int iInterval,
                              int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    int iRet = 0;
    if(!m_bSetSQL)
    {
        ERROR_TO_THROW_NOSQL(ERR_SQL_INVALID,"SQL not set,iParamIndex=[%d].",iParamIndex);
    }
    ST_MEM_VALUE * pMemValue = GetParamByIndex(iParamIndex);
    if(NULL != pMemValue)
    {
        //find
        TMdbParamArray & tParamArray = m_pParamArray[iParamIndex];
        CHECK_RET_THROW(tParamArray.ReAlloc(iArraySize/iInterval,MEM_Int),
                        ERR_SQL_SETPARAMETER_TYPE,m_pszSQL,"ReAlloc(%lld,%d)",iArraySize/iInterval,MEM_Int);
        tParamArray.m_iParamIndex = iParamIndex;
        int i = 0;
        for(i = 0; i<tParamArray.m_iArraySize ; ++i)
        {
            tParamArray.m_pllValue[i] = *(allParamValue + i);
        }
        tParamArray.m_tNullArrWrap.Init(bNullArr,tParamArray.m_iArraySize);
    }
    else
    {
        ERROR_TO_THROW(ERR_SQL_PARAM_INDEX_INVALID,m_pszSQL,"Invalid-ParamIndex(%d)",iParamIndex);
    }
    m_bSetList[iParamIndex] = true;
    CheckAndSetParamType(SET_PARAM_ARRAY);
}
void TMdbQuery::SetBlobParamArray(int iParamIndex,char *sParamValue,int iBufferLen,
                                  int iArraySize,bool bOutput,bool * bNullArr) throw (TMdbException)
{
    ERROR_TO_THROW(ERR_SQL_BLOB_PARAM_NOT_SUPPORT_BIND,m_pszSQL,"Can't support the function(Blob).");
}

int TMdbQuery::FillFieldForCSBin(NoOcpParse & tParseData,bool bFirst)
{
    return m_pExecuteEngine->FillFieldForCSBin(tParseData,bFirst);
}

int TMdbDatabase::m_iSQLFlag = 1;

TMdbDatabase::TMdbDatabase()
{
    memset(m_sUser, 0, sizeof(m_sUser));
    memset(m_sPWD, 0, sizeof(m_sPWD));
    memset(m_sDSN, 0, sizeof(m_sDSN));
    m_bConnectFlag = false;
    m_pConfig  = NULL;
    m_pShmDSN  = NULL;
    m_bIsNeedCommit = true;
    m_bAsManager = false;
    m_pLinkCtrl = NULL;
    m_pLocalLink = NULL;
    m_pMultiProtector = NULL;
}


TMdbDatabase::~TMdbDatabase()
{
    Disconnect();
    SAFE_DELETE(m_pLinkCtrl);
    SAFE_DELETE(m_pMultiProtector);
}

/******************************************************************************
* ��������	:  SetLogin
* ��������	:  ���õ�½��Ϣ
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbDatabase::SetLogin(const char *sUser,const char *sPassword,const char *sServerName) throw (TMdbException)
{
    TADD_DETAIL("User=[%s],PWD=[%s],DSN=[%s].",sUser,sPassword,sServerName);
    SAFESTRCPY(m_sUser,sizeof(m_sUser),sUser);
    SAFESTRCPY(m_sPWD,sizeof(m_sPWD),sPassword);
    SAFESTRCPY(m_sDSN,sizeof(m_sDSN),sServerName);
}

/******************************************************************************
* ��������	:  ConnectAsMgr
* ��������	: ����Ա��ʽ�������ݿ�
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbDatabase::ConnectAsMgr(const char* sDSN) throw (TMdbException)
{
    TADD_FUNC("Start.");
    memset(m_sUser, 0, sizeof(m_sUser));
    memset(m_sPWD,  0, sizeof(m_sPWD));
    SAFESTRCPY(m_sDSN,sizeof(m_sDSN),sDSN);
    m_bAsManager = true;
    return Connect(false);
}
/******************************************************************************
* ��������	:  Connect
* ��������	:  �������ݿ�
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbDatabase::Connect(bool bIsAutoCommit) throw (TMdbException)
{
    int iRet = 0;
	
	if(TADD_START_FLAG)
	{
	 char sProcName[128];
	 memset(sProcName,0,sizeof(sProcName));
	 TMdbOS::GetProcFullNameByPID(TMdbOS::GetPID(),sProcName,sizeof(sProcName));
	 if(strlen(sProcName)==0)
	 {
		 SAFESTRCPY(sProcName, sizeof(sProcName), "mdbLink");
	 }
	 TADD_START(m_sDSN,sProcName, 0,true,false);
	}


    TADD_DETAIL("m_bAsManager=[%d].",m_bAsManager);
    TADD_DETAIL("User=[%s],PWD=[%s],DSN=[%s].",m_sUser,m_sPWD,m_sDSN);
    if(m_bAsManager)
    {
        if(m_sDSN[0]==0)
        {
            ERROR_TO_THROW_NOSQL(ERR_APP_INVALID_PARAM,"DSN is null...");
        }
    }
    else if(m_sUser[0]==0 || m_sPWD[0]==0 || m_sDSN[0]==0)
    {
        ERROR_TO_THROW_NOSQL(ERR_APP_INVALID_PARAM,
                             "Connect to %s/%s@%s failed. The parameter is invalid.", m_sUser, m_sPWD, m_sDSN);
    }
    m_pConfig = TMdbConfigMgr::GetMdbConfig(m_sDSN);
    if(NULL == m_pConfig)
    {
        ERROR_TO_THROW_NOSQL(ERR_APP_INVALID_PARAM,"Get config[%s] failed.",m_sDSN);
    }
    CHECK_OBJ(m_pConfig);
    if(!m_bAsManager)
    {
        //����û����������Ƿ���ȷ
        iRet = m_pConfig->CheckParam(m_sUser, m_sPWD, m_sDSN);
        CHECK_RET_THROW_NOSQL(iRet,iRet,"Connect to %s/%s@%s failed. Invalid username or password.",\
                            m_sUser, m_sPWD, m_sDSN);
        //���Ӽ�ؽ���
        CHECK_RET_THROW_NOSQL(LinkMonitor(),ERR_NET_PEER_REFUSE,"Link-Monitor failed.");
    }
    //�����Ϲ����ڴ�
    m_pShmDSN = TMdbShmMgr::GetShmDSN(m_sDSN);
    if(NULL == m_pShmDSN)
    {
        ERROR_TO_THROW_NOSQL(ERR_OS_ATTACH_SHM," Attach [%s] failed.",m_sDSN);
    }
    else
    {
        CHECK_RET_THROW_NOSQL(m_pShmDSN->Attach(m_sDSN,*m_pConfig),ERR_OS_ATTACH_SHM,
                              " Connect to %s/%s@%s failed. Can't find Config-file.", m_sUser, m_sPWD, m_sDSN);
    }
    //���ӹ���
    if(NULL == m_pLinkCtrl)
    {
        m_pLinkCtrl = new(std::nothrow) TMdbLinkCtrl();
        if(NULL == m_pLinkCtrl)
        {
             ERROR_TO_THROW_NOSQL(ERR_OS_NO_MEMROY,"m_pLinkCtrl is NULL");
        }
    }
    CHECK_RET_THROW_NOSQL(m_pLinkCtrl->Attach(m_sDSN),ERR_OS_ATTACH_SHM," Attach [%s] failed.",m_sDSN);
    CHECK_RET_THROW_NOSQL(m_pLinkCtrl->RegLocalLink(m_pLocalLink),ERR_DB_EXCEED_MAX_LOCAL_CONNECTION,"Register link failed.");//�����ڴ�ע��������Ϣ

	m_iSessionID = m_pLocalLink->iSessionID;
	
    m_bConnectFlag = true;

    if(NULL == m_pMultiProtector)
    {
        m_pMultiProtector = new(std::nothrow) TMdbMultiProtector();//��������
    }
    m_pMultiProtector->Reset();
    
    return true;
}

/******************************************************************************
* ��������	:  Connect
* ��������	:  �������ݿ�
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbDatabase::Connect(const char *sUser, const char *sPassword, const char *sServerName, bool bIsAutoCommit) throw (TMdbException)
{
    SAFESTRCPY(m_sUser,sizeof(m_sUser),sUser);
    SAFESTRCPY(m_sPWD,sizeof(m_sPWD),sPassword);
    SAFESTRCPY(m_sDSN,sizeof(m_sDSN),sServerName);
    return Connect(bIsAutoCommit);
}
/******************************************************************************
* ��������	:  ReleaseSysSQL
* ��������	:  �ͷ�ע���sql
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbDatabase::ReleaseSysSQL() throw (TMdbException)
{
    TADD_FUNC("Start.");
    /*
    TMdbDSN* pDsn = ((TMdbShmDSN*)m_pShmDSN)->GetInfo();
    //�ҵ�ϵͳSQL���׵�ַ
    char* pAddr = (char*)pDsn;
    TMdbSysSQL* pSysSQL = (TMdbSysSQL*)(pAddr + pDsn->iSQLAddr);
    int i = 0;
    char sTempAddr[20];
    memset(sTempAddr,0,20);
    sprintf(sTempAddr,"%p",this);
    for(i=0; i< MAX_SYS_SQL_COUNTS; i++)
    {
        if(pSysSQL->iSqlPos != -1 && pSysSQL->iRowID != -1)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(sTempAddr,pSysSQL->sDatabaseAddr) == 0)
            {
                pSysSQL->Clear();
                --pDsn->iSQLCounts;
            }
        }
        ++pSysSQL;
    }
    TADD_FUNC("Finish.");
    */
    return 0;
}

/******************************************************************************
* ��������	:  Disconnect
* ��������	:  �Ͽ����ݿ�����
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbDatabase::Disconnect() throw(TMdbException)
{
    TADD_FUNC("Start.");

    //�ͷ���ص�ע��SQL
    if(m_bConnectFlag == true)
    {
        //ע������
        if(NULL != m_pLinkCtrl && NULL != m_pLocalLink)
        {
            m_pLinkCtrl->UnRegLocalLink(m_pLocalLink);
        }
        
        m_bConnectFlag = false;
        m_pShmDSN = NULL;
        m_pMultiProtector->Reset();
    }
    TADD_FUNC("Finish.");
    //TADD_END();
    return 0;
}


/******************************************************************************
* ��������	:  LinkMonitor
* ��������	:  ���Ӽ�ؽ���
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbDatabase::LinkMonitor()
{
    TMdbProcCtrl tProCtrl;
    int iRet = 0;
    CHECK_RET(tProCtrl.Init(m_sDSN),"tProCtrl.Init(%s)",m_sDSN);
    if(tProCtrl.IsMonitorStart() == false)
    {
        CHECK_RET(ERR_DB_NOT_CONNECTED,"QuickMDB is not running.");
    }
    return iRet;
}

/******************************************************************************
* ��������	:  TransBegin
* ��������	:  ��������������Ĭ�Ͽ����ģ���������������岻��
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbDatabase::TransBegin()
{
    m_iTransactionState = TRANS_IN;//������
    return;
}

/******************************************************************************
* ��������	:  Commit
* ��������	:  �ύ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/                          																								//��������
void TMdbDatabase::Commit()
{
	m_pLocalLink->Commit(m_pShmDSN);
    m_iTransactionState = TRANS_IN;
    return;
}


//�ύ, ��¼�ύ�ļ�¼��                            																								//��������
void TMdbDatabase::CommitEx()
{
    return;
}

/******************************************************************************
* ��������	:  Rollback
* ��������	:  �ع�����
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbDatabase::Rollback()
{
	m_pLocalLink->RollBack(m_pShmDSN);	
    return;
}
/******************************************************************************
* ��������	:  IsNullConnect
* ��������	:  �Ƿ�Ϊ������
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbDatabase::IsNullConnect()
{
    return m_bConnectFlag;
}

/******************************************************************************
* ��������	:  IsConnect
* ��������	:  �Ƿ��Ѿ�����
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
bool TMdbDatabase::IsConnect()
{
    return m_bConnectFlag;
}


/******************************************************************************
* ��������	:  DisableCommit
* ��������	:  �����ύ(�Զ��ύ)
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbDatabase::DisableCommit()
{
    m_bIsNeedCommit = false;
}

/******************************************************************************
* ��������	:  EnableCommit
* ��������	:  �����ύ
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbDatabase::EnableCommit()
{
    m_bIsNeedCommit = true;
}

/******************************************************************************
* ��������	:  CreateDBQuery
* ��������	:  ����һ���µĲ�ѯ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/ 																													//�������ݿ��Ƿ���������
TMdbQuery *TMdbDatabase::CreateDBQuery() throw (TMdbException)
{
    if(NULL == m_pLocalLink)
    {//��û������
        ERROR_TO_THROW_NOSQL(ERR_DB_NOT_CONNECTED,"not connect qmdb.please connect first.");
    }
    if(!IsLinkOK())
    {
        ERROR_TO_THROW_NOSQL(ERR_DB_NOT_CONNECTED,"Link is down.");
    }
    if(false == m_pMultiProtector->IsValid())
    {//�����ϲ���
        TADD_WARNING("current thread not connect,qmdb not support concurrency on one link.DBLink=[%d|%d],current=[%d|%lu]",
            m_pMultiProtector->GetPID(),m_pMultiProtector->GetTID(),TMdbOS::GetPID(),TMdbOS::GetTID());
        /*
        ERROR_TO_THROW_NOSQL(ERROR_DB_CURTHREAD_NOT_CONNECT,
            "current thread not connect,qmdb not support concurrency on one link.DBLink=[%d|%d],current=[%d|%lu]",
            m_pMultiProtector->GetPID(),m_pMultiProtector->GetTID(),TMdbOS::GetPID(),TMdbOS::GetTID());
        */
    }

    return new(std::nothrow) TMdbQuery(this,TMdbDatabase::m_iSQLFlag++);
}

TMdbNosqlQuery* TMdbDatabase::CreateNosqlQuery() throw (TMdbException)
{
    if(NULL == m_pLocalLink)
    {//��û������
        ERROR_TO_THROW_NOSQL(ERR_DB_NOT_CONNECTED,"not connect qmdb.please connect first.");
    }
    if(!IsLinkOK())
    {
        ERROR_TO_THROW_NOSQL(ERR_DB_NOT_CONNECTED,"Link is down.");
    }
    if(false == m_pMultiProtector->IsValid())
    {//�����ϲ���
        TADD_WARNING("current thread not connect,qmdb not support concurrency on one link.DBLink=[%d|%d],current=[%d|%lu]",
            m_pMultiProtector->GetPID(),m_pMultiProtector->GetTID(),TMdbOS::GetPID(),TMdbOS::GetTID());
    }

    return new(std::nothrow) TMdbNosqlQuery(this,TMdbDatabase::m_iSQLFlag++);
}


void TMdbDatabase::CheckError(const char* sSql) throw (TMdbException) 
{}
const char* TMdbDatabase::GetProvider()
{
    return NULL;
}

/******************************************************************************
* ��������	:  SetSQLPos
* ��������	:  ����ִ�е�SQL��λ��
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/
void TMdbDatabase::SetSQLPos(int iPos)
{
    if(m_pLocalLink != NULL)
    {
        m_pLocalLink->iSQLPos = iPos;
    }
}

/******************************************************************************
* ��������	:  RowsAffected
* ��������	:  Ӱ���¼��
* ����		:
* ���		:
* ����ֵ	:
* ����		:  jin.shaohua
*******************************************************************************/

int TMdbDatabase::RowsAffected()
{
	return m_pLocalLink->iAffect;
}

TMdbShmDSN * TMdbDatabase::GetShmDsn()
{
    return m_pShmDSN;
}

//����Ƿ�������״̬
bool TMdbDatabase::IsLinkOK()
{
    if(NULL == m_pLocalLink){return false;}
    if(m_pLocalLink->cState != Link_use)
    {
        Disconnect();//�Ͽ�����
        return false;
    }
    return true;
}
int TMdbDatabase::GetTransactionState()
{
    return m_iTransactionState;
}
char* TMdbDatabase::GetUser()
{
    return m_sUser;   //��ȡuser,pwd,dsn
}
char* TMdbDatabase::GetPWD()
{
    return m_sPWD;
};
char* TMdbDatabase::GetDSN()
{
    return m_sDSN;
};

/******************************************************************************
* ��������	:  SetCaptureRouter
* ��������	:  ���ò���·��,���ŷָ�
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbDatabase::SetCaptureRouter(const char * sRouters)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(sRouters);
    CHECK_OBJ(m_pShmDSN);
    TMdbNtcSplit tSplit;
    tSplit.SplitString(sRouters,',');
    if(MAX_ROUTER_LIST_LEN < tSplit.GetFieldCount()+1)
    {
        CHECK_RET(ERR_APP_INVALID_PARAM,"router[%s] is too long",sRouters);
    }
    int arrRouter[MAX_ROUTER_LIST_LEN] = {0};
    unsigned int i = 0;
    arrRouter[0] = tSplit.GetFieldCount();//��һ���Ÿ���
    for(i = 0;i < tSplit.GetFieldCount();++i)
    {
        if(TMdbNtcStrFunc::IsDigital(tSplit[i]) == false)
        {//���ַ���,���
            memset(arrRouter,0,sizeof(arrRouter));
            break;
        }
        arrRouter[i+1] = TMdbNtcStrFunc::StrToInt(tSplit[i]);
    }
    CHECK_RET(m_pShmDSN->LockDSN(),"lock failed.");
    memcpy(m_pShmDSN->GetInfo()->m_arrRouterToCapture,arrRouter,sizeof(m_pShmDSN->GetInfo()->m_arrRouterToCapture));
    CHECK_RET(m_pShmDSN->UnLockDSN(),"unlock failed.");
    TADD_FUNC("Stop");
    return iRet;
}
/******************************************************************************
* ��������	:  GetCaptureRouter
* ��������	:  ��ȡ����·��
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int  TMdbDatabase::GetCaptureRouter(char * sRouterRet)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(sRouterRet);
    CHECK_OBJ(m_pShmDSN);
    sRouterRet[0] = 0;
    int * arrRouter = m_pShmDSN->GetInfo()->m_arrRouterToCapture;
    int i = 0;
    for(i = 1;i<= arrRouter[0];++i)
    {
        sprintf(sRouterRet + strlen(sRouterRet),"%d,",arrRouter[i]);
    }
    TADD_FUNC("Stop");
    return iRet;
}


/******************************************************************************
* ������	:  TMdbParam
* ������	:  mdb SQL �󶨱�������
* ����		:
*******************************************************************************/
TMdbParam::TMdbParam()
{
    m_iCount = 0;
    m_pParamPool = NULL;
    m_iParamComparePos = -1;
    m_iParamPoolCurPos = -1;

}
TMdbParam::~TMdbParam()
{
    SAFE_DELETE_ARRAY(m_pParamPool);
}
/******************************************************************************
* ��������	:  Clear
* ��������	:  ����
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbParam::Clear()
{
    m_vParamName.clear();
    SAFE_DELETE_ARRAY(m_pParamPool);
    m_iParamComparePos = -1;
    m_iParamPoolCurPos = -1;
    return 0;
}

/******************************************************************************
* ��������	:  ��Ӳ���
* ��������	:  
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbParam::AddParam(const char * sParamName)
{
    int iRet = 0;
    std::vector<std::string>::iterator itor = m_vParamName.begin();
    for(;itor != m_vParamName.end();++itor)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(itor->c_str(),sParamName) == 0)
        {
            CHECK_RET(ERR_SQL_INVALID,"sParam is exist");
        }
    }
    m_vParamName.push_back(sParamName);
    return iRet;
}

/******************************************************************************
* ��������	:  ���ݲ���index��ȡ������
* ��������	:  
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
const char * TMdbParam::GetParamNameByIndex(int iIndex)
{
    if(iIndex >= (int)m_vParamName.size())
    {
        return NULL;
    }
    else
   {
        return m_vParamName[iIndex].c_str();
    }
}
/******************************************************************************
* ��������	:  ���ݲ�������ȡ����index
* ��������	:  
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbParam::GetParamIndexByName(const char * sParamName)
{
    std::vector<std::string>::iterator itor = m_vParamName.begin();
    int iCount = 0;
    if(m_iParamComparePos > -1 && m_iParamComparePos < m_iCount)
    {//���һ�����
        iCount = m_pParamPool[m_iParamComparePos];
        if(TMdbNtcStrFunc::StrNoCaseCmp(m_vParamName[iCount].c_str(),sParamName) == 0)
        {   
            //printf("iCount=%d\n",iCount);
            m_iParamComparePos++;
            return iCount;
        }
    }
    //û�л��棬��ԭ���Ĳ���
    bool bFind = false;
    
    for(;itor != m_vParamName.end();++itor)
    {
        if(TMdbNtcStrFunc::StrNoCaseCmp(itor->c_str(),sParamName) == 0)
        {//find
            bFind = true;
            break;
        }
        iCount++;
    }
    if(bFind)
    {//д�뻺��
        //printf("%d,iCount=%d\n",m_iParamPoolCurPos,iCount);
    
        m_iParamPoolCurPos++;
        if(m_iParamPoolCurPos < m_iCount)
        {
            m_pParamPool[m_iParamPoolCurPos] = iCount;
        }
        return iCount;
    }
    return -1;
}

/******************************************************************************
* ��������	: ��ȡ��������
* ��������	:  
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbParam::GetCount()
{
    return m_vParamName.size();
}

void TMdbParam::NewParamPool()
{
    m_iCount = GetCount();
    SAFE_DELETE_ARRAY(m_pParamPool);
    m_iParamComparePos = -1;
    m_iParamPoolCurPos = -1;
    m_pParamPool = new(std::nothrow) char[m_iCount];
}
void TMdbParam::InitParamPool()
{
    m_iParamComparePos = 0;
    m_iParamPoolCurPos = -1;
}

TMdbSequenceMgr::TMdbSequenceMgr()
{
    memset(m_SequenceName,0,64);
}

TMdbColumnAddr::TMdbColumnAddr()
{
	memset(this,0,sizeof(this));
}

TMdbColumnAddr::~TMdbColumnAddr()
{
	for(int i = 0;i<MAX_COLUMN_COUNTS;i++)
	{
		SAFE_DELETE(m_ppBlob[i]);
		m_ppBlob[i] = NULL;
	}
}


//}

