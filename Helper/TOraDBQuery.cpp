#ifdef DB_ORACLE

#include "Helper/TOraDBQuery.h"
#include <time.h> 
#include "Helper/mdbErr.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbStruct.h"

//namespace QuickMDB{


Tcolumn::Tcolumn()
{
    memset(name,0,sizeof(name));
    memset(value,0,sizeof(value));
}

Tcolumn::~Tcolumn()
{
}

TData::TData()
{
    memset(tablename,0,sizeof(tablename));
    memset(operate,0,sizeof(operate));
}

TData::~TData()
{
}
TOraDBException::TOraDBException(sword errNumb, OCIError *hErr,const char *cat,const char *sql)
{
    size_t nLen;
    sb4 errcode;

    nLen = strlen(cat);
    nLen = (nLen >= MAX_ERR_CAT_LENGTH)? MAX_ERR_CAT_LENGTH : nLen;
    strncpy(errCategory,cat,nLen);
    errCategory[nLen] = '\0';

	if (NULL == sql)
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

    m_lErrCode = errNumb;
    
    if(NULL!=hErr)
    {
        (void)OCIErrorGet ((dvoid *) hErr, (ub4) 1, (text *) NULL, &errcode,
            (text*)m_sErrMsg, (ub4)sizeof(m_sErrMsg)-1, (ub4) OCI_HTYPE_ERROR);
    }
    else
    {
         snprintf(m_sErrMsg,MDB_MAX_ERRMSG_LENGTH, "Invalid Handle Of OCIError!");
    }
}

TOraDBException::TOraDBException(const char *sql, const char* errFormat, ...)
{
	if (NULL == sql)
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


//�Ƚ�2���ַ����Ƿ���ͬ(�����Ǵ�Сд)
bool inline CompareStrNoCase(const char *ori, const char *des)
{
    return TMdbNtcStrFunc::StrNoCaseCmp(ori, des) == 0;
}

/********* TConnection implementation *********/
TOraQueueBase::TOraQueueBase()throw (TOraDBException)
{
    sword errorNo;

    hUser = NULL;
    hDBSvc = NULL;  
    hDBErr = NULL;
    hEnv = NULL;
    hSvr = NULL;
    usr = NULL;
    pwd = NULL;
    tns = NULL;
    m_tableRecord = NULL;
    memset(sSourceStr,0,sizeof(sSourceStr));
    wait = OCI_DEQ_NO_WAIT;
    deq_mode =OCI_DEQ_REMOVE;
    navigation = OCI_DEQ_NEXT_MSG;
    deqMesg = (Table_Reocrd*)0;
    ndeqMesg=(null_Table_Reocrd *)0;
    mesg_tdo = (OCIType *) 0;
    deqopt = (OCIAQDeqOptions *)0;
    errorNo = OCIInitialize((ub4) OCI_DEFAULT|OCI_OBJECT,0, 0,0,0 );
    errorNo = errorNo + OCIEnvInit( (OCIEnv **) &hEnv, (ub4) OCI_THREADED,(size_t) 0, (dvoid **) 0 );
    errorNo = errorNo + OCIHandleAlloc( (dvoid *) hEnv, (dvoid **) &hDBSvc,(ub4) OCI_HTYPE_SVCCTX,(size_t) 0, (dvoid **) 0);
    errorNo = errorNo + OCIHandleAlloc( (dvoid *) hEnv, (dvoid **) &hDBErr,(ub4) OCI_HTYPE_ERROR,(size_t) 0, (dvoid **) 0); 
    errorNo = errorNo + OCIHandleAlloc( (dvoid *) hEnv, (dvoid **) &hSvr,(ub4) OCI_HTYPE_SERVER,(size_t) 0, (dvoid **) 0);

    if ( errorNo != 0 )
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::TOraDBDatabase()  MDB_ERR_DB_INIT.",__FILE__,__LINE__);
        throw TOraDBException( "TOraDBDatabase::TOraDBDatabase()", MDB_ERR_DB_INIT, __LINE__);
    }
}
TOraQueueBase::~TOraQueueBase()
{
    delete[] usr;
    delete[] pwd;
    delete[] tns;
    if(m_tableRecord != NULL)
    {
        delete m_tableRecord;
        m_tableRecord = NULL;
    }
    OCIServerDetach(hSvr, hDBErr, OCI_DEFAULT );
    OCIHandleFree(hSvr, OCI_HTYPE_SERVER);      
    OCIHandleFree(hDBSvc, OCI_HTYPE_SVCCTX);
    OCIHandleFree(hDBErr,OCI_HTYPE_ERROR);
    OCIHandleFree(hEnv,OCI_HTYPE_ENV);
}

void TOraQueueBase::SetLogin(const char *user, const char *password, const char *tnsString) throw (TOraDBException)
{
    int nLen;
    //�����ⲿ���ݵĲ���
    if ( usr != NULL) 
        delete[] usr;
    if (pwd != NULL)
        delete[] pwd;
    if (tns != NULL)
        delete[] tns;

    //�����ⲿ���ݵĲ���
    if (user)
    {
        nLen = strlen(user);
        usr = new char[nLen+1];
        strncpy(usr,user,nLen);
        usr[nLen] = '\0';
    }
    else
    {
        nLen = 0;
        usr = new char[1];
        usr[0] = '\0';
    }

    if (password)
    {
        nLen = strlen(password);
        pwd = new char[nLen+1];
        strncpy(pwd,password,nLen);
        pwd[nLen] = '\0';
    }
    else
    {
        nLen = 0;
        pwd = new char[1];
        pwd[0] = '\0';
    }

    if (tnsString)
    {
        nLen = strlen(tnsString);
        tns = new char[nLen+1];
        strncpy(tns,tnsString,nLen);
        tns[nLen] = '\0';
    }
    else    
    {
        nLen = 0;
        tns = new char[1];
        tns[0] = '\0';
    }
}

bool TOraQueueBase::Connect(bool bUnused) throw (TOraDBException)
{
    sword errorNo;
    if ( (usr == NULL) || (tns==NULL) )
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Connect()  MDB_ERR_CONNECT_NO_LOGIN_INFO.",__FILE__,__LINE__);
        throw TOraDBException("Connect()", MDB_ERR_CONNECT_NO_LOGIN_INFO, __LINE__);
        }

    errorNo = OCIServerAttach(hSvr, hDBErr, (text *)tns, strlen(tns), 0);
    if (errorNo != OCI_SUCCESS)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Connect()  try to connect Server.",__FILE__,__LINE__);
        throw TOraDBException(errorNo, hDBErr, "Connect()", "try to connect Server");
        }

    //modified: 2003.1
    fErrorNo = OCIHandleAlloc(hEnv, (dvoid **) &hUser,(ub4) OCI_HTYPE_SESSION,(size_t) 0, (dvoid **) 0);
    CheckError();

    fErrorNo = OCIHandleAlloc(hEnv, (dvoid **)&hDBSvc, OCI_HTYPE_SVCCTX,0, 0);
    CheckError();

    fErrorNo = OCIAttrSet (hDBSvc, OCI_HTYPE_SVCCTX, hSvr, 0, OCI_ATTR_SERVER, hDBErr);
    CheckError();

    /* set the username/password in user handle */
    OCIAttrSet(hUser, OCI_HTYPE_SESSION, usr, strlen(usr),OCI_ATTR_USERNAME, hDBErr);
    OCIAttrSet(hUser, OCI_HTYPE_SESSION, pwd, strlen(pwd),OCI_ATTR_PASSWORD, hDBErr);

    // Set the Authentication handle in the service handle
    fErrorNo = OCIAttrSet(hDBSvc, OCI_HTYPE_SVCCTX, hUser, 0, OCI_ATTR_SESSION, hDBErr);
    CheckError();

    fErrorNo=OCISessionBegin (hDBSvc, hDBErr, hUser, OCI_CRED_RDBMS, OCI_DEFAULT);
    CheckError();
    return (fConnected = (errorNo == OCI_SUCCESS));
}

bool TOraQueueBase::Connect(const char *inUsr, const char *inPwd, const char *inTns, bool bUnused) throw (TOraDBException)
{
    SetLogin(inUsr, inPwd, inTns);
    return Connect();
}
int TOraQueueBase::Disconnect() throw (TOraDBException)
{
    OCISessionEnd (hDBSvc, hDBErr, hUser, OCI_DEFAULT);
    sword errorNo = OCIServerDetach(hSvr, hDBErr, OCI_DEFAULT );
    if (errorNo != OCI_SUCCESS)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Connect()  OCIServerDetatch error.",__FILE__,__LINE__);
        throw TOraDBException(errorNo, hDBErr,"Disconnect()", "OCIServerDetatch error");
    }
    return 1;
}
void TOraQueueBase::Commit()
{
    OCITransCommit(hDBSvc, hDBErr, OCI_DEFAULT);
}
void TOraQueueBase::CheckError(const char* sSql) throw (TOraDBException)
{
    if (fErrorNo != OCI_SUCCESS) 
    {
        //TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::CheckError()  Oracle OCI Call.",__FILE__,__LINE__);
        throw TOraDBException(fErrorNo, hDBErr, "Oracle OCI Call", "OCIDatabase");
    }
}

void TOraQueueBase::InitDequeue()
{
    char sLowToUpp[128];
    memset(sLowToUpp,0,sizeof(sLowToUpp));
    //TMdbNtcStrFunc::ToUpper(usr,sLowToUpp);
    SAFESTRCPY(sLowToUpp, sizeof(sLowToUpp), TMdbNtcStrFunc::ToUpper(usr));
    fErrorNo = OCITypeByName(hEnv, hDBErr, hDBSvc, (CONST text*)sLowToUpp,strlen(sLowToUpp),(CONST text *)"QUICKMDB_TYPE", strlen("QUICKMDB_TYPE"), (text *)0, 0, OCI_DURATION_SESSION, OCI_TYPEGET_ALL, &mesg_tdo);
    CheckError();
    fErrorNo = OCIDescriptorAlloc(hEnv, (dvoid **)&deqopt, OCI_DTYPE_AQDEQ_OPTIONS, 0,(dvoid **)0);
    CheckError();
    fErrorNo = OCIAttrSet(deqopt, OCI_DTYPE_AQDEQ_OPTIONS,(dvoid *)&wait, 0, OCI_ATTR_WAIT, hDBErr);
    CheckError();    
    fErrorNo = OCIAttrSet(deqopt, OCI_DTYPE_AQDEQ_OPTIONS,(dvoid *)&deq_mode, 0,  OCI_ATTR_DEQ_MODE, hDBErr);
    CheckError();
    fErrorNo =OCIAttrSet(deqopt, OCI_DTYPE_AQDEQ_OPTIONS,(dvoid *)&navigation, 0,OCI_ATTR_NAVIGATION, hDBErr);
    CheckError();
    //tSplit.SetSplitter('~');
    if(m_tableRecord == NULL)
    {
        m_tableRecord = new TData();
    }

    if(m_tableRecord == NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraQueueBase::InitDequeue().",__FILE__,__LINE__);
        return;
    }
    return;
    
}

TData*  TOraQueueBase::AQDequeue()
{
    if(OCIAQDeq(hDBSvc,hDBErr,(OraText *)"quickmdb_queue",deqopt,0,mesg_tdo,(dvoid **)&deqMesg,(dvoid **)&ndeqMesg,0,0) == OCI_SUCCESS)
    {
        tSplit.SplitString((char*)OCIStringPtr(hEnv,deqMesg->sRecord_value), '~');
        SetTableRecord();
        return m_tableRecord;
    }
    else
    {
        return NULL;
    }
    return NULL;
}

void TOraQueueBase::SetTableRecord()
{
    int iPos = 0;
    for(unsigned int i = 0; i<tSplit.GetFieldCount();i++) 
    {
        if(i == 0)
        {
            SAFESTRCPY(m_tableRecord->tablename,sizeof(m_tableRecord->tablename),tSplit[i]);
        }
        else if(i == 1)
        {
            SAFESTRCPY(m_tableRecord->operate,sizeof(m_tableRecord->operate),tSplit[i]);
        }
        else
        {
            SAFESTRCPY(m_tableRecord->m_column[iPos].name,sizeof(m_tableRecord->m_column[iPos].name),tSplit[i]);
            i++;
            SAFESTRCPY(m_tableRecord->m_column[iPos].value,sizeof(m_tableRecord->m_column[iPos].value),tSplit[i]);
            iPos++;
        }
    }
    return;
}
TOraDBDatabase::TOraDBDatabase() throw (TOraDBException)
{
    sword errorNo;

    hUser = NULL;
    hDBSvc = NULL;  
    hDBErr = NULL;
    hEnv = NULL;
    hSvr = NULL;

    //errorNo = OCIInitialize((ub4) OCI_DEFAULT|OCI_OBJECT,0, 0,0,0 );
    errorNo = OCIInitialize((ub4) OCI_THREADED,0, 0,0,0 );
    errorNo = errorNo + OCIEnvInit( (OCIEnv **) &hEnv, (ub4) OCI_THREADED,(size_t) 0, (dvoid **) 0 );
    errorNo = errorNo + OCIHandleAlloc( (dvoid *) hEnv, (dvoid **) &hDBSvc,(ub4) OCI_HTYPE_SVCCTX,(size_t) 0, (dvoid **) 0);
    errorNo = errorNo + OCIHandleAlloc( (dvoid *) hEnv, (dvoid **) &hDBErr,(ub4) OCI_HTYPE_ERROR,(size_t) 0, (dvoid **) 0); 
    errorNo = errorNo + OCIHandleAlloc( (dvoid *) hEnv, (dvoid **) &hSvr,(ub4) OCI_HTYPE_SERVER,(size_t) 0, (dvoid **) 0);

    if ( errorNo != 0 )
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::TOraDBDatabase()  MDB_ERR_DB_INIT.",__FILE__,__LINE__);
        throw TOraDBException( "TOraDBDatabase::TOraDBDatabase()", MDB_ERR_DB_INIT, __LINE__);
        }
    fConnected = false;
    usr = NULL;
    pwd = NULL;
    tns = NULL;
    m_pTOraDBQuery=NULL;
}

TOraDBDatabase::~TOraDBDatabase()
{
    if(m_pTOraDBQuery!=NULL)
    {
        m_pTOraDBQuery->Close();
        delete m_pTOraDBQuery;
        m_pTOraDBQuery=NULL;
    }

    delete[] usr;
    delete[] pwd;
    delete[] tns;

    if (fConnected) 
        OCIServerDetach(hSvr, hDBErr, OCI_DEFAULT );

    OCIHandleFree(hSvr, OCI_HTYPE_SERVER);      
    OCIHandleFree(hDBSvc, OCI_HTYPE_SVCCTX);
    OCIHandleFree(hDBErr,OCI_HTYPE_ERROR);
    OCIHandleFree(hEnv,OCI_HTYPE_ENV);
}

/******************************************************************************
* ��������	:  SetLogin()
* ��������	:  �������ݿ��¼��
* ����		:  user �û�����password ���룬tnsString ������
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TOraDBDatabase::SetLogin(const char *user, const char *password, const char *tnsString) throw (TOraDBException)
{
    if (fConnected)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::SetLogin()  MDB_ERR_SET_LOGIN.",__FILE__,__LINE__);
        throw TOraDBException("SetLogin()", MDB_ERR_SET_LOGIN , __LINE__);
        }

    int nLen;
    //�����ⲿ���ݵĲ���
    if ( usr != NULL) 
        delete[] usr;
    if (pwd != NULL)
        delete[] pwd;
    if (tns != NULL)
        delete[] tns;

    //�����ⲿ���ݵĲ���
    if (user)
    {
        nLen = strlen(user);
        usr = new char[nLen+1];
        strncpy(usr,user,nLen);
        usr[nLen] = '\0';
    }
    else
    {
        nLen = 0;
        usr = new char[1];
        usr[0] = '\0';
    }

    if (password)
    {
        nLen = strlen(password);
        pwd = new char[nLen+1];
        strncpy(pwd,password,nLen);
        pwd[nLen] = '\0';
    }
    else
    {
        nLen = 0;
        pwd = new char[1];
        pwd[0] = '\0';
    }

    if (tnsString)
    {
        nLen = strlen(tnsString);
        tns = new char[nLen+1];
        strncpy(tns,tnsString,nLen);
        tns[nLen] = '\0';
    }
    else    
    {
        nLen = 0;
        tns = new char[1];
        tns[0] = '\0';
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
void TOraDBDatabase::CheckError(const char* sSql) throw (TOraDBException)
{
    if (fErrorNo != OCI_SUCCESS) 
    {
        //TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::CheckError()  Oracle OCI Call.",__FILE__,__LINE__);
        throw TOraDBException(fErrorNo, hDBErr, "Oracle OCI Call", "OCIDatabase");
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
bool TOraDBDatabase::Connect(bool bUnused) throw (TOraDBException)
{
    TADD_NORMAL("connect oracle [%s/******@%s].",usr,tns);
    sword errorNo;
    if (fConnected)
    {
        TADD_NORMAL("The oracle has connected success.");
        return true;
    }

    if ( (usr == NULL) || (tns==NULL) )
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Connect()  MDB_ERR_CONNECT_NO_LOGIN_INFO.",__FILE__,__LINE__);
        throw TOraDBException("Connect()", MDB_ERR_CONNECT_NO_LOGIN_INFO, __LINE__);
    }

    errorNo = OCIServerAttach(hSvr, hDBErr, (text *)tns, strlen(tns), 0);
    if (errorNo != OCI_SUCCESS)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Connect()  try to connect Server.",__FILE__,__LINE__);
        throw TOraDBException(errorNo, hDBErr, "Connect()", "try to connect Server");
    }

    //modified: 2003.1
    fErrorNo = OCIHandleAlloc(hEnv, (dvoid **) &hUser,(ub4) OCI_HTYPE_SESSION,(size_t) 0, (dvoid **) 0);
    CheckError();

    fErrorNo = OCIHandleAlloc(hEnv, (dvoid **)&hDBSvc, OCI_HTYPE_SVCCTX,0, 0);
    CheckError();

    fErrorNo = OCIAttrSet (hDBSvc, OCI_HTYPE_SVCCTX, hSvr, 0, OCI_ATTR_SERVER, hDBErr);
    CheckError();

    /* set the username/password in user handle */
    OCIAttrSet(hUser, OCI_HTYPE_SESSION, usr, strlen(usr),OCI_ATTR_USERNAME, hDBErr);
    OCIAttrSet(hUser, OCI_HTYPE_SESSION, pwd, strlen(pwd),OCI_ATTR_PASSWORD, hDBErr);

    // Set the Authentication handle in the service handle
    fErrorNo = OCIAttrSet(hDBSvc, OCI_HTYPE_SVCCTX, hUser, 0, OCI_ATTR_SESSION, hDBErr);
    CheckError();

    fErrorNo=OCISessionBegin (hDBSvc, hDBErr, hUser, OCI_CRED_RDBMS, OCI_DEFAULT);
    CheckError();
    //Set Trans:
    //OCIAttrSet(hDBSvc, OCI_HTYPE_SVCCTX, hTrans, 0, OCI_ATTR_TRANS, hErr);
    TADD_NORMAL("connect oracle [%s/******@%s].",usr,tns);
    return (fConnected = (errorNo == OCI_SUCCESS));
}

/******************************************************************************
* ��������	:  Connect()
* ��������	:  ���ݿ�����
* ����		:  user �û�����password ���룬tnsString ��������bUnused �Ƿ�ʹ��,Ĭ��false
* ���		:  ��
* ����ֵ	:  true ���ӳɹ���false ����ʧ��
* ����		:  li.shugang
*******************************************************************************/
bool TOraDBDatabase::Connect(const char *inUsr, const char *inPwd, const char *inTns, bool bUnused) throw (TOraDBException)
{
    SetLogin(inUsr, inPwd, inTns);
    return Connect();
}

/******************************************************************************
* ��������	:  Disconnect()
* ��������	:  �Ͽ����ݿ�����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  1 �ɹ�������ʧ��
* ����		:  li.shugang
*******************************************************************************/
int TOraDBDatabase::Disconnect() throw (TOraDBException)
{
    if (!fConnected) return 1;
    OCISessionEnd (hDBSvc, hDBErr, hUser, OCI_DEFAULT);
    sword errorNo = OCIServerDetach(hSvr, hDBErr, OCI_DEFAULT );
    if (errorNo != OCI_SUCCESS)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Connect()  OCIServerDetatch error.",__FILE__,__LINE__);
        throw TOraDBException(errorNo, hDBErr,"Disconnect()", "OCIServerDetatch error");
    }
    fConnected = false;
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
void TOraDBDatabase::Commit()
{
    OCITransCommit(hDBSvc, hDBErr, OCI_DEFAULT);
}

/******************************************************************************
* ��������	:  Rollback()
* ��������	:  ����ع�
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TOraDBDatabase::Rollback()
{
    OCITransRollback(hDBSvc, hDBErr, OCI_DEFAULT);
}

/******************************************************************************
* ��������	:  IsConnect()
* ��������	:  �������ݿ��Ƿ���������
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true ������false ���ݿ������쳣
* ����		:  li.shugang
*******************************************************************************/
bool TOraDBDatabase::IsConnect() throw (TOraDBException)
{
    if(fConnected==true)
    {
        if(m_pTOraDBQuery==NULL)
        {
            m_pTOraDBQuery = new(std::nothrow) TOraDBQuery(this);
        }

        try
        {
            m_pTOraDBQuery->Close();
            m_pTOraDBQuery->SetSQL("SELECT 1 FROM DUAL");
            m_pTOraDBQuery->Open();
            /*if(m_pTOraDBQuery->Next())
            {
            }*/
            m_pTOraDBQuery->Close();
        }
        catch(TOraDBException)
        {
            Disconnect();
            fConnected=false;
        }
        catch(...)
        {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::IsConnect()  Exception",__FILE__,__LINE__);
            throw TOraDBException("","TOraDBDatabase::IsConnect() catch ... Exception");
        }
    }

    return fConnected;
}

/******************************************************************************
* ��������	:  CreateDBQuery()
* ��������	:  �������ݿ��ѯ����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  TOraDBQuery ����
* ����		:  li.shugang
*******************************************************************************/
TOraDBQuery * TOraDBDatabase::CreateDBQuery() throw (TMDBDBExcpInterface)
{
    return new(std::nothrow) TOraDBQuery(this);
}

TOraDBField::TOraDBField()
{
    //��ʼ������Ϣ,�в��ֵĳ�ʼ����Ϣ��Describe�н���
    name = NULL;
    hBlob = NULL;
    hDefine = (OCIDefine *) 0; ;
    fDataBuf = NULL;
    fDataIndicator = NULL;
    fParentQuery = NULL;

    fReturnDataLen = 0;
    size = 0;
    precision = 0;
    scale = 0;
    size = 0;
    lDataBufLen=0;

    sBlob=NULL;
    ahBlob=NULL;
    iBlobCount=0;
};

TOraDBField::~TOraDBField()
{
    if (fDataIndicator != NULL)
        delete[] fDataIndicator;
    if (name != NULL)
        delete[] name; 
    if (fDataBuf != NULL)
        delete[] fDataBuf;
    //if (type == SQLT_BLOB)
    //    OCIDescriptorFree((dvoid *)hBlob, (ub4) OCI_DTYPE_LOB);
    if(ahBlob!=NULL)
    {
        for(int iIndex=0;iIndex<iBlobCount;iIndex++)
        {
            OCIDescriptorFree((dvoid *)ahBlob[iIndex], (ub4) OCI_DTYPE_LOB);
        }
        delete[] ahBlob;
        ahBlob=NULL;
    }

    if(sBlob!=NULL)
    {
        delete[] sBlob;
        sBlob=NULL;
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
char* TOraDBField::AsString() throw (TOraDBException)
{
    int year, month, day, hour, minute, second;
    //static char intStr[100];

    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::IsConnect()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "AsString()", name);
        }

    if ( isNULL() )
    {
        sprintf((char *)fStrBuffer,"%s", MDB_NULL_STRING);
        return (char *)fStrBuffer;
    }

    switch ( this->type )
    {
    case DATE_TYPE:
        this->AsDateTimeInternal(year, month, day, hour, minute, second);
        sprintf((char *)fStrBuffer,"%04d%02d%02d%02d%02d%02d", year, month, day,hour, minute, second);
        return (char *)fStrBuffer;
    case INT_TYPE:
        long long  intValue;
        if ( (OCINumberToInt(fParentQuery->hErr, (OCINumber *)(fDataBuf + (size+1) * fParentQuery->fCurrRow ),sizeof(intValue), OCI_NUMBER_SIGNED,&intValue))!= OCI_SUCCESS)
        {
            fParentQuery->CheckError();
        }

        sprintf((char *)fStrBuffer, "%lld", intValue);
        return (char *)fStrBuffer;
    case FLOAT_TYPE:
        //int status;
        double floatValue;

        if ( (OCINumberToReal(fParentQuery->hErr, (OCINumber *)(fDataBuf + (size+1) * fParentQuery->fCurrRow ),sizeof(floatValue), &floatValue))!= OCI_SUCCESS)
        {
            fParentQuery->CheckError();
        }
        if ( floatValue == (intValue=(long long)floatValue))
        {
            sprintf((char *)fStrBuffer, "%lld", intValue);
        }
        else
        {
            sprintf((char *)fStrBuffer, "%f", floatValue);
        }
        return (char *)fStrBuffer;      
        //return((char *)(fDataBuf + (size+1) * fParentQuery->fCurrRow));
    case STRING_TYPE:
    case ROWID_TYPE:
        return((char *)(fDataBuf + (size+1) * fParentQuery->fCurrRow));
    case SQLT_BLOB:
        sprintf((char *)fStrBuffer, "BLOB...");
        return (char *)fStrBuffer;
    default:
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::IsConnect()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "AsString()");
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
bool TOraDBField::isNULL()
{
    return (fDataIndicator[fParentQuery->fCurrRow]==-1);
}

/******************************************************************************
* ��������	:  AsBlobFile()
* ��������	:  Blob���ʹ�����ȡ��file��
* ����		:  fileName ����Blob����
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void  TOraDBField::AsBlobFile(const char *fileName) throw (TOraDBException)
{
    ub4 offset = 1;
    ub1 buf[MDB_MAX_LOB_BUFFER_LENGTH];
    ub4 nActual = 0;    //ʵ�ʶ�ȡ��
    ub4 nTry = 0;       //��ͼ��ȡ��
    ub4 totalSize = 0;
    FILE *fileHandle = NULL;
    ub4 lobLength;

    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::IsConnect()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "AsBlobFile()", name);
        }

    if (type != SQLT_BLOB)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::IsConnect()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "asLobFile()");
        }

    fileHandle = fopen( fileName, (const char *) "wb");
    fseek(fileHandle, 0, 0);
    /* set amount to be read per iteration */
    nTry = nActual = MDB_MAX_LOB_BUFFER_LENGTH;
    hBlob=ahBlob[fParentQuery->fCurrRow];
    OCILobGetLength(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob, &lobLength);

    while (nActual)
    {
        fParentQuery->fErrorNo = OCILobRead(fParentQuery->db->hDBSvc, fParentQuery->hErr, 
            hBlob, &nActual, (ub4)offset, (dvoid *) buf, (ub4) nTry, (dvoid *)0, 
            (sb4 (*)(dvoid *, CONST dvoid *, ub4, ub1)) 0, (ub2) 0, (ub1) SQLCS_IMPLICIT);
        fParentQuery->CheckError();   

        if (nActual<=0) break;

        totalSize += nActual;
        fwrite((void *)buf, (size_t)nActual, (size_t)1, fileHandle);
        offset += nActual;
    }
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
void  TOraDBField::AsBlobBuffer(unsigned char* &buf, unsigned int *lobLength) throw (TOraDBException)
{
    ub1 innerBuf[MDB_MAX_LOB_BUFFER_LENGTH];
    ub4 remainder, nActual, nTry;
    //ub4  flushedAmount = 0;
    ub4 offset = 1;
    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsBlobBuffer()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "AsBlobBuffer()", name);
        }

    if (type != SQLT_BLOB)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsBlobBuffer()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "asLobBuffer()");
        }

    hBlob=ahBlob[fParentQuery->fCurrRow];
    OCILobGetLength(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob, &remainder);
    *lobLength = nActual = nTry = remainder;

    try
    {
        buf = new unsigned char[sizeof(ub1) * remainder];
    }
    catch (...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsBlobBuffer()  MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED, "AsBlobBuffer()", __LINE__);
    }

    nTry = nActual = MDB_MAX_LOB_BUFFER_LENGTH;
    while (remainder)
    {       
        fParentQuery->fErrorNo = OCILobRead(fParentQuery->db->hDBSvc, fParentQuery->hErr, 
            hBlob, &nActual, (ub4)offset, (dvoid *)innerBuf, (ub4) nTry, (dvoid *)0, 
            (sb4 (*)(dvoid *, CONST dvoid *, ub4, ub1)) 0, (ub2) 0, (ub1) SQLCS_IMPLICIT);
        fParentQuery->CheckError();   
        memcpy( (buf) + offset -1, innerBuf, nActual);
        if (nActual<=0) break;

        offset += nActual;
        remainder -= nActual;
    }
}

/******************************************************************************
* ��������	:  AsBlobBuffer()
* ��������	:  ����ֵ�Ƿ�Ϊ��	
* ����		:  ��
* ���		:  iBufferLen BLOB�ֶ�ֵ����
* ����ֵ	:  BLOB�ֶ�ֵ
* ����		:  li.shugang
*******************************************************************************/
char* TOraDBField::AsBlobBuffer(int &iBufferLen) throw (TOraDBException)
{
    ub1 innerBuf[MDB_MAX_LOB_BUFFER_LENGTH];
    ub4 remainder, nActual, nTry;
   // ub4  flushedAmount = 0;
    ub4  offset = 1;

    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsBlobBuffer()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "AsBlobBuffer()", name);
        }

    if (type != SQLT_BLOB)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsBlobBuffer()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "asLobBuffer()");
        }
    hBlob=ahBlob[fParentQuery->fCurrRow];
    OCILobGetLength(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob, &remainder);
    iBufferLen = nActual = nTry = remainder;

    try
    {
        if(sBlob!=NULL)
        {
            delete[] sBlob;
            sBlob=NULL;
        }
        sBlob = new char[sizeof(ub1) * remainder];
    }
    catch (...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsBlobBuffer()  MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED, "AsBlobBuffer()", __LINE__);
    }

    nTry = nActual = MDB_MAX_LOB_BUFFER_LENGTH;
    while (remainder)
    {       
        fParentQuery->fErrorNo = OCILobRead(fParentQuery->db->hDBSvc, fParentQuery->hErr, 
            hBlob, &nActual, (ub4)offset, (dvoid *)innerBuf, (ub4) nTry, (dvoid *)0, 
            (sb4 (*)(dvoid *, CONST dvoid *, ub4, ub1)) 0, (ub2) 0, (ub1) SQLCS_IMPLICIT);
        fParentQuery->CheckError();   
        memcpy( (sBlob) + offset -1, innerBuf, nActual);
        if (nActual<=0) break;

        offset += nActual;
        remainder -= nActual;
    }

    return sBlob;
}

/******************************************************************************
* ��������	:  LoadFromFile()
* ��������	:  д�뵽blob�� 
* ����		:  fileName ����blob�����ļ�
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void  TOraDBField::LoadFromFile(const char *fileName) throw (TOraDBException)
{
    ub4 remainder, nActual, nTry, offset = 1;//���ļ��ж�ȡ��ʣ��������
    ub1 buf[MDB_MAX_LOB_BUFFER_LENGTH];
    ub4 LobLength;
    ub4  flushedAmount = 0;
    FILE *fileHandle ;

    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::LoadFromFile()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "LoadFromFile()", name);
        }

    if (type != SQLT_BLOB)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::LoadFromFile()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "LoadFromFile()");
        }

    if( (fileHandle = fopen(fileName, (const char *) "rb")) == NULL )
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::LoadFromFile()  MDB_ERR_FILE_IO",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_FILE_IO, "LoadFromFile()", fileName);
        }

    fseek(fileHandle,0,SEEK_END);
    remainder = ftell(fileHandle);
    fseek(fileHandle, 0, 0);

    hBlob=ahBlob[fParentQuery->fCurrRow];
    fParentQuery->fErrorNo = OCILobGetLength(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob, &LobLength);
    fParentQuery->CheckError();

    fParentQuery->fErrorNo = OCILobTrim(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob, 0);
    fParentQuery->CheckError();

    /* enable the BLOB locator for buffering operations */
    fParentQuery->fErrorNo = OCILobEnableBuffering(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob);
    fParentQuery->CheckError();

    while ( (remainder > 0) && !feof(fileHandle))
    {
        nActual = nTry = (remainder > MDB_MAX_LOB_BUFFER_LENGTH) ? MDB_MAX_LOB_BUFFER_LENGTH : remainder;

        if (fread((void *)buf, (size_t)nTry, (size_t)1, fileHandle) != (size_t)1)
            {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::LoadFromFile()  MDB_ERR_MEM_BUFFER_IO",__FILE__,__LINE__);
            throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_MEM_BUFFER_IO, name, fileName, __LINE__);
            }

        fParentQuery->fErrorNo = OCILobWrite(fParentQuery->db->hDBSvc, fParentQuery->hErr, 
            hBlob, &nActual, offset, (dvoid *) buf, (ub4) nTry, OCI_ONE_PIECE, (dvoid *)0,
            (sb4 (*)(dvoid *, dvoid *, ub4 *, ub1 *)) 0,    (ub2) 0, (ub1) SQLCS_IMPLICIT);
        if ( fParentQuery->fErrorNo != OCI_SUCCESS) 
        {
            fclose(fileHandle);
            fParentQuery->CheckError();
        }

        flushedAmount += nTry;
        remainder -= nTry;
        offset += nTry;
        //incase the internal buffer is not big enough for the lob , flush the buffer content to db after some interval:
        if (flushedAmount >= MDB_LOB_FLUSH_BUFFER_SIZE)
        {
            flushedAmount = 0;
            fParentQuery->fErrorNo = OCILobFlushBuffer(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob, OCI_LOB_BUFFER_NOFREE);
            fParentQuery->CheckError(); 
        }
    }

    if ( flushedAmount )
    {
        fParentQuery->fErrorNo = OCILobFlushBuffer(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob, OCI_LOB_BUFFER_NOFREE);
        fParentQuery->CheckError(); 
    }
    fclose(fileHandle);
}

/******************************************************************************
* ��������	:  LoadFromBuffer()
* ��������	:  ��BLOB�������û������������ 	 
* ����		:  buf ����BLOB���ݵĻ�������bufLength ����������
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void  TOraDBField::LoadFromBuffer(unsigned char *buf, unsigned int bufLength) throw (TOraDBException)
{
    ub1 innerBuf[MDB_MAX_LOB_BUFFER_LENGTH];

    ub4 remainder, nActual, nTry;
    ub4  flushedAmount = 0, offset = 1;

    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::LoadFromBuffer()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "LoadFromBuffer()", name);
        }

    if (type != SQLT_BLOB)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::LoadFromBuffer()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "LoadFromBuffer()");
        }

    hBlob=ahBlob[fParentQuery->fCurrRow];
    fParentQuery->fErrorNo = OCILobTrim(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob, 0);
    fParentQuery->CheckError();
    remainder = bufLength;

    /* enable the BLOB locator for buffering operations */
    fParentQuery->fErrorNo = OCILobEnableBuffering(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob);
    fParentQuery->CheckError();

    while (remainder > 0)
    {
        nActual = nTry = (remainder > MDB_MAX_LOB_BUFFER_LENGTH) ? MDB_MAX_LOB_BUFFER_LENGTH : remainder;

        memcpy(innerBuf, buf + offset-1, nActual);

        fParentQuery->fErrorNo = OCILobWrite(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob,
            &nActual, offset, (dvoid *)innerBuf, (ub4)nTry, OCI_ONE_PIECE, (dvoid *)0,
            (sb4 (*)(dvoid *, dvoid *, ub4 *, ub1 *)) 0,    (ub2) 0, (ub1) SQLCS_IMPLICIT);
        fParentQuery->CheckError();

        flushedAmount += nTry;
        remainder -= nTry;
        offset += nTry;

        if (flushedAmount >= MDB_LOB_FLUSH_BUFFER_SIZE)
        {
            flushedAmount = 0;
            fParentQuery->fErrorNo = OCILobFlushBuffer(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob, OCI_LOB_BUFFER_NOFREE);
            fParentQuery->CheckError(); 
        }
    }

    if ( flushedAmount )
    {
        fParentQuery->fErrorNo = OCILobFlushBuffer(fParentQuery->db->hDBSvc, fParentQuery->hErr, hBlob, OCI_LOB_BUFFER_NOFREE);
        fParentQuery->CheckError(); 
    }
}

/******************************************************************************
* ��������	:  AsFloat()
* ��������	:  ��������������� 
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��������
* ����		:  li.shugang
*******************************************************************************/
double TOraDBField::AsFloat() throw (TOraDBException)
{
    double iRet; 
    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsFloat()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "AsFloat()", name);
        }

    if ( isNULL() )
        return 0;
    if ( (type == FLOAT_TYPE) || ( type == INT_TYPE) ){
        if ( (OCINumberToReal(fParentQuery->hErr, (OCINumber *)(fDataBuf + (size+1) * fParentQuery->fCurrRow ),sizeof(iRet), &iRet))!= OCI_SUCCESS)
        {
            fParentQuery->CheckError();
        }
        return iRet;
    }

    else    
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsFloat()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "AsFloat()");
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
char* TOraDBField::AsDateTimeString() throw (TOraDBException)
{
    int year, month, day, hour, minute, second;

    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsDateTimeString()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "AsDateTimeString()", name);
        }

    if (type != DATE_TYPE)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsDateTimeString()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "AsDateTimeString()");
        }
    else
    {
        this->AsDateTimeInternal(year, month, day, hour, minute, second);
        if ( year == 0 )
            sprintf( (char *)fStrBuffer,"%s", MDB_NULL_STRING);
        else    sprintf( (char *)fStrBuffer,"%04d%02d%02d%02d%02d%02d", year, month, day, hour, minute,second);
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
void    TOraDBField::AsDateTime(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TOraDBException)
{
    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsDateTime()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "AsDateTime()", name);
        }

    if (type != DATE_TYPE)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsDateTime()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "AsDateTime()");
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
time_t TOraDBField::AsTimeT() throw (TOraDBException)
{
    time_t tSeconds;
    static struct tm ts;
    
    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsTimeT()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "AsTimeT()", name);
        }

    if (type != DATE_TYPE)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsTimeT()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "AsTimeT()");
        }
    else if (isNULL())
        return (time_t)0;
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
void TOraDBField::AsDateTimeInternal(int &year, int &month, int &day, int &hour, int &minute, int &second) throw (TOraDBException)
{
    unsigned char cc,yy,mm,dd,hh,mi,ss;
    ub1 *data;

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
    data = fDataBuf + 7 * (fParentQuery->fCurrRow);
    cc=data[0]; 
    yy=data[1]; 
    mm=data[2]; 
    dd=data[3]; 
    hh=data[4]-1; 
    mi=data[5]-1; 
    ss=data[6]-1; 
    cc=(unsigned char)((cc-100)<0?100-cc:cc-100);
    yy=(unsigned char)((yy-100)<0?100-yy:yy-100);
    year = (unsigned int)cc*100 + (unsigned int) yy;
    month = mm;
    day = dd;
    hour = hh;
    minute = mi;
    second = ss;
}

/******************************************************************************
* ��������	:  isNULL()
* ��������	:  ��������������� 
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��������
* ����		:  zhang.yu
*******************************************************************************/
long long TOraDBField::AsInteger() throw (TOraDBException)
{   
    long long iRet=0;
    if (fParentQuery->fBof || fParentQuery->fEof)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsInteger()  MDB_ERR_NO_DATASET",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_NO_DATASET, "AsInteger()", name);
        }

    if (type != INT_TYPE && type != FLOAT_TYPE)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsInteger()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException(fParentQuery->fSqlStmt, MDB_ERR_DATA_TYPE_CONVERT, name, type, "AsInteger()");
        }
    else {
        if ( (OCINumberToInt(fParentQuery->hErr, (OCINumber *)(fDataBuf + (size+1) * fParentQuery->fCurrRow ),sizeof(iRet), OCI_NUMBER_SIGNED,&iRet))!= OCI_SUCCESS)
        {
            fParentQuery->CheckError();
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
void TOraDBField::ClearDataBuf()
{
    if((fDataBuf!=NULL) && (lDataBufLen>0))
    {
        memset(fDataBuf,0,lDataBufLen);
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
char *TOraDBField::GetFieldName()
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
long TOraDBField::GetFieldType()
{
    return type;
}

/******************************************************************************
* ��������	:  GetFieldSize()
* ��������	:  ��ȡ�д�С
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �д�С
* ����		:  li.shugang
*******************************************************************************/
long  TOraDBField::GetFieldSize()
{
    return size;
}

/******************************************************************************
* ��������	:  GetFieldPrecision()
* ��������	:  ��ȡ�о���	
* ����		:  ��
* ���		:  ��
* ����ֵ	:  �о���
* ����		:  li.shugang
*******************************************************************************/
int  TOraDBField::GetFieldPrecision() 
{
    return precision;
}

/*********** TOraDBQuery Implementation************/
TOraDBQuery::TOraDBQuery(TOraDBDatabase *oradb) throw (TOraDBException)
{
    if (! oradb->fConnected)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::AsInteger()  MDB_ERR_DATA_TYPE_CONVERT",__FILE__,__LINE__);
        throw TOraDBException("", MDB_ERR_GENERAL, "TOraDBQuery(TOraDBDatabase &db): Can not declare a TOraDBQuery when the database is not connected");
    }

    fFetched = 0;
    fPrefetchRows = 1;
    fCurrRow = 0;
    fTotalRowsFetched = 0;
    fBof = false;
    fEof = false;
#ifdef __DEBUG__
    bExecuteFlag = false;
#endif
    nTransTimes = 0;
    db = oradb;
    fActivated = false;
    fFieldCount = 0;
    fParamCount = 0;

    fSqlStmt = NULL;
    paramList = NULL;
    fieldList = NULL;
    hErr = NULL;

    fErrorNo = OCIHandleAlloc(db->hEnv, (dvoid **) &hErr,(ub4) OCI_HTYPE_ERROR,(size_t) 0, (dvoid **) 0);
    CheckError();
    fErrorNo = OCIHandleAlloc(db->hEnv, (dvoid **)&hStmt, OCI_HTYPE_STMT, 0, 0);
    CheckError();

    //fErrorNo = OCIHandleAlloc(db->hEnv, (dvoid **)&hTrans, OCI_HTYPE_TRANS, 0, 0);
    //CheckError();

}

TOraDBQuery::TOraDBQuery(TOraDBDatabase *oradb,TOraDBSession *session) throw (TOraDBException)
{
    if (! session->m_bActive)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::TOraDBQuery()  Can not declare a TOraDBQuery when the database is not connected",__FILE__,__LINE__);
        throw TOraDBException("", MDB_ERR_GENERAL, "TOraDBQuery(TOraDBDatabase &db): Can not declare a TOraDBQuery when the database is not connected");
        }

    fFetched = 0;
    fPrefetchRows = 1;
    fCurrRow = 0;
    fTotalRowsFetched = 0;
    fBof = false;
    fEof = false;
#ifdef __DEBUG__
    bExecuteFlag = false;
#endif
    nTransTimes = 0;
    db = oradb;
    fActivated = false;
    fFieldCount = 0;
    fParamCount = 0;

    fSqlStmt = NULL;
    paramList = NULL;
    fieldList = NULL;

    /*  hUser = session->m_hSession;
    */
    hErr = session->m_hError;
    /*hSvc = session->m_hSrvCtx;
    */
    fErrorNo = OCIHandleAlloc(db->hEnv, (dvoid **)&hStmt, OCI_HTYPE_STMT, 0, 0);
    CheckError();

    //fErrorNo = OCIHandleAlloc(db->hEnv, (dvoid **)&hTrans, OCI_HTYPE_TRANS, 0, 0);
    //CheckError(); 
}


TOraDBQuery::~TOraDBQuery()
{
    if (fSqlStmt != NULL)
        delete[] fSqlStmt;
    if (fParamCount >0)
        delete[] paramList;
    if (fFieldCount >0)
        delete[] fieldList;
    if (nTransTimes)
    {
        //fErrorNo = OCITransRollback(db->hDBSvc, hErr, OCI_DEFAULT);
        //CheckError();
    }
#ifdef __DEBUG__
    if(bExecuteFlag)
        userlog("TOraDBQueryִ����Execute()����û���ύ��ع�,�����������"); 
#endif
    /*
    OCISessionEnd (hSvc, hErr, hUser, OCI_DEFAULT);
    */
    OCIHandleFree(hStmt, OCI_HTYPE_STMT);
    //OCIHandleFree(hTrans,OCI_HTYPE_TRANS);
    /*OCIHandleFree(hSvc, OCI_HTYPE_SVCCTX);
    OCIHandleFree(hUser,OCI_HTYPE_SESSION);
    */
    OCIHandleFree(hErr,OCI_HTYPE_ERROR);
}

/******************************************************************************
* ��������	:  Close()
* ��������	:  �ر�SQL��䣬��׼��������һ��sql���   
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TOraDBQuery::Close()
{
    if (! fActivated)
        return;

    if (fSqlStmt != NULL)
    {
        delete[] fSqlStmt;
        fSqlStmt = NULL;
    }
    if (fParamCount > 0)
    {
        delete[] paramList;
        paramList = NULL;
    }
    if (fFieldCount > 0)
    {
        delete[] fieldList;
        fieldList = NULL;
    }

    fFieldCount = 0;
    fParamCount = 0;
    fActivated = false;

    fFetched = 0;
    fPrefetchRows = MDB_PREFETCH_ROWS;
    fCurrRow = 0;
    fTotalRowsFetched = 0;
}

/******************************************************************************
* ��������	:  Commit()
* ��������	:  �����ύ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true �ύ�ɹ���false �ύʧ��
* ����		:  li.shugang
*******************************************************************************/
bool TOraDBQuery::Commit()
{
#ifdef __DEBUG__
    bExecuteFlag = false;
#endif
    fErrorNo = OCITransCommit(db->hDBSvc, hErr, OCI_DEFAULT);
    CheckError();
    if (fErrorNo == OCI_SUCCESS)
        nTransTimes = 0;
    return (fErrorNo == OCI_SUCCESS);   
}

/******************************************************************************
* ��������	:  Rollback()
* ��������	:  ����ع�
* ����		:  ��
* ���		:  ��
* ����ֵ	:  true �ع��ɹ���false �ع�ʧ��
* ����		:  li.shugang
*******************************************************************************/
bool TOraDBQuery::Rollback()
{
    bool exeSuccess = false;
#ifdef __DEBUG__
    bExecuteFlag = false;
#endif
    fErrorNo = OCITransRollback(db->hDBSvc, hErr, OCI_DEFAULT);
    if (fErrorNo == OCI_SUCCESS)
    {
        nTransTimes = 0;
        exeSuccess = true;
    }
    else
        CheckError();
    return exeSuccess;
}

void TOraDBQuery::GetFieldsDef() throw (TOraDBException)
{
    TOraDBField *pCurrField;
    OCIParam    *param = (OCIParam *)0;
    ub4  counter;                 //����������ѭ�������ʵ��
    ub4  nColumnCount;            //�ڷ���sqlstmt�󣬻�õ�����Ŀ
    //���²����ĳ��Ȳμ�"OCI Programmer's" Guide P208 , 6-10
    text   *columnName;           //�ֶ�����
    ub4  columnNameLength,j;      //�ֶ����Ƴ��� unsigned int
    ub2  innerDataSize;           //���ݳ��� unsigned short
    ub2  innerDataType;           //Oracle �ڲ��������� signed short
    ub2  innerPrecision;          //����С�������λ��, ub1 is a bug in documentation?
    sb1  innerScale;              //С�������
    ub1  innerIsNULL;             //�Ƿ�����Ϊ��ֵ

    if(fStmtType==OCI_STMT_SELECT)
        fErrorNo = OCIStmtExecute(db->hDBSvc, hStmt, hErr, (ub4)0, (ub4)0, 0, 0, OCI_DEFAULT);
    else    
        fErrorNo = OCIStmtExecute(db->hDBSvc, hStmt, hErr, (ub4)1, (ub4)0, 0, 0, OCI_DEFAULT);
    CheckError(); 
    //��Execute�󣬿��Ի���еĸ������еĸ���?
    //���û��Ϊselect���ķ���ֵ�����������������hErr��Ĳ���itersӦ������Ϊ0������>0;����Ƿ�SELECT��䣬��ֵ>0;
    fErrorNo = OCIAttrGet((dvoid *)hStmt, (ub4)OCI_HTYPE_STMT, (dvoid *)&nColumnCount, (ub4 *) 0, (ub4)OCI_ATTR_PARAM_COUNT, hErr);
    CheckError();

    if (fFieldCount >0 )
    {
        delete[] fieldList;
        fieldList = NULL;
    }
    fieldList = new(std::nothrow) TOraDBField[nColumnCount];
    fFieldCount = nColumnCount;

    for(counter=1; counter<=nColumnCount ; counter ++)
    {
        fErrorNo = OCIParamGet(hStmt, OCI_HTYPE_STMT, hErr, (dvoid **)&param, counter);
        CheckError();

        // column name and column name length
        fErrorNo = OCIAttrGet((dvoid*)param, OCI_DTYPE_PARAM, (dvoid**)&columnName,(ub4 *)&columnNameLength, OCI_ATTR_NAME, hErr);
        CheckError();

        // data length 
        fErrorNo = OCIAttrGet((dvoid*)param, OCI_DTYPE_PARAM, (dvoid *)&innerDataSize, (ub4 *)0, OCI_ATTR_DATA_SIZE, hErr);
        CheckError();

        // precision 
        fErrorNo = OCIAttrGet((dvoid*)param, OCI_DTYPE_PARAM, (dvoid *)&innerPrecision, (ub4 *)0, OCI_ATTR_PRECISION, hErr);
        CheckError();

        // scale 
        fErrorNo = OCIAttrGet((dvoid*)param, OCI_DTYPE_PARAM, (dvoid *)&innerScale, (ub4 *)0, OCI_ATTR_SCALE, hErr);
        CheckError();

        // isNULL
        fErrorNo = OCIAttrGet((dvoid*)param, OCI_DTYPE_PARAM, (dvoid *)&innerIsNULL, (ub4 *)0, OCI_ATTR_IS_NULL, hErr);
        CheckError();

        // data type:
        fErrorNo = OCIAttrGet((dvoid*)param, OCI_DTYPE_PARAM, (dvoid *)&innerDataType, (ub4 *)0, OCI_ATTR_DATA_TYPE, hErr);
        CheckError();

        pCurrField = &fieldList[counter-1];

        pCurrField->name = new(std::nothrow) char[columnNameLength+1];
        if (pCurrField->name == NULL)
            {
            TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::GetFieldsDef()  MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED",__FILE__,__LINE__);
            throw TOraDBException(fSqlStmt, MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED, "GetFieldsDef()", __LINE__);
            }
        for (j=0; j<columnNameLength; j++)
            pCurrField->name[j] = columnName[j];
        pCurrField->name[columnNameLength] = '\0';

        pCurrField->nullable = innerIsNULL>0;

        pCurrField->type = innerDataType; //��ʼ��Ϊ�ڲ����ͣ����ڴ��󷵻�
        pCurrField->fParentQuery = this;
        pCurrField->fDataIndicator = new(std::nothrow) sb2[fPrefetchRows];

        switch  (innerDataType)
        {
        case SQLT_NUM://NUMBER_TYPE: 
            if (! innerDataSize)
                pCurrField->size = 255;
            else    
                pCurrField->size = innerDataSize;
            pCurrField->precision = innerPrecision;
            pCurrField->scale = innerScale;
            //Ԥ�ȶ����ַ������������ڽ�����ֵ���ַ�ת���Ľ��
            pCurrField->fDataBuf = new ub1[fPrefetchRows * (pCurrField->size+1)];
            pCurrField->lDataBufLen=sizeof(ub1)*fPrefetchRows * (pCurrField->size+1);
            if (innerScale == 0) //û��С���㣬Ϊ����
                pCurrField->type = INT_TYPE;
            else    
                pCurrField->type = FLOAT_TYPE;

            //��������ݵ�������(�������Ͱ󶨵�����������)
            fErrorNo = OCIDefineByPos(hStmt, &(pCurrField->hDefine), hErr, counter,
                (dvoid *)pCurrField->fDataBuf, pCurrField->size + 1, SQLT_VNU,
                (dvoid *)pCurrField->fDataIndicator, (ub2 *)0 , (ub2 *) 0, OCI_DEFAULT);
            CheckError();
            break;
        case SQLT_DAT://DATE_TYPE:
            pCurrField->type = DATE_TYPE;
            pCurrField->size = 7;
            //��������ݵ�������(date����Ҳ�ǰ󶨵��ַ���������)
            pCurrField->fDataBuf = new ub1[fPrefetchRows *(pCurrField->size)];
            pCurrField->lDataBufLen=sizeof(ub1)*fPrefetchRows * (pCurrField->size);
            fErrorNo = OCIDefineByPos(hStmt, &(pCurrField->hDefine), hErr, counter,
                pCurrField->fDataBuf, 7, SQLT_DAT,
                (dvoid *)pCurrField->fDataIndicator, (ub2 *)0, (ub2 *) 0, OCI_DEFAULT);
            CheckError();
            break;
        case SQLT_CHR: case SQLT_AFC: //DATA_TYPE_CHAR: case VARCHAR2_TYPE: 
            pCurrField->type = STRING_TYPE;
            pCurrField->size = innerDataSize;  //��ϵͳȡ�õ��ֶγ�����Ϊ���ݵĳ��ȴ�С
            //��������ݵ�������
            pCurrField->fDataBuf = new ub1[fPrefetchRows * (pCurrField->size+1)];
            pCurrField->lDataBufLen=sizeof(ub1)*fPrefetchRows * (pCurrField->size+1);
            fErrorNo = OCIDefineByPos(hStmt, &(pCurrField->hDefine), hErr, counter,
                pCurrField->fDataBuf, pCurrField->size+1, SQLT_STR,
                (dvoid *)pCurrField->fDataIndicator, (ub2 *)0, (ub2 *)0, OCI_DEFAULT);
            CheckError(); 
            break;
        case SQLT_RDD:
            pCurrField->type = ROWID_TYPE;
            pCurrField->size = 18;
            //��������ݵ�������
            pCurrField->fDataBuf = new ub1[fPrefetchRows * (pCurrField->size+1)];
            pCurrField->lDataBufLen=sizeof(ub1)*fPrefetchRows * (pCurrField->size+1);
            fErrorNo = OCIDefineByPos(hStmt, &(pCurrField->hDefine), hErr, counter,
                (dvoid *)pCurrField->fDataBuf, 
                pCurrField->size+1, SQLT_STR,
                (dvoid *)pCurrField->fDataIndicator, (ub2 *) 0, (ub2 *) 0, OCI_DEFAULT);
            CheckError();
            break;
        case SQLT_BLOB:
            pCurrField->size = 4;
            pCurrField->type = SQLT_BLOB;
            pCurrField->iBlobCount=fPrefetchRows;
            pCurrField->ahBlob=(OCILobLocator**)new char[sizeof(OCILobLocator*)*pCurrField->iBlobCount];
            for(int iIndex=0;iIndex<pCurrField->iBlobCount;iIndex++)
            {
                fErrorNo = OCIDescriptorAlloc((dvoid *)db->hEnv, (dvoid **)&pCurrField->ahBlob[iIndex],
                    (ub4)OCI_DTYPE_LOB, (size_t)0, (dvoid **) 0);
                CheckError();
            }

            fErrorNo = OCIDefineByPos(hStmt, &(pCurrField->hDefine), hErr, counter,
                (dvoid *)pCurrField->ahBlob, (sb4)-1, SQLT_BLOB,
                (dvoid *)pCurrField->fDataIndicator,  (ub2 *)0, (ub2 *) 0, OCI_DEFAULT);
            CheckError();
            break;
        default:
            throw TOraDBException(fSqlStmt, MDB_ERR_DATA_TYPE_NOT_SUPPORT, pCurrField->name,innerDataType);
            break;
        } //end of data type convertion
        fErrorNo = OCIDescriptorFree((dvoid *)param, OCI_DTYPE_PARAM);
        CheckError();
    }//end of for loop every column
}

/******************************************************************************
* ��������	:  SetSQL()
* ��������	:  ����Sqlstatement   
* ����		:  inSqlstmt sql���
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TOraDBQuery::SetSQL(const char *inSqlstmt,int iPreFetchRows) throw (TOraDBException)
{
    if (! db->fConnected)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::SetSQL()  can't set sqlstmt on disconnected Database",__FILE__,__LINE__);
        throw TOraDBException(inSqlstmt, MDB_ERR_GENERAL, "SetSQL(): can't set sqlstmt on disconnected Database");
        }

    if (fActivated)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::SetSQL()  can't set sqlstmt on opened state",__FILE__,__LINE__);
        throw TOraDBException(inSqlstmt, MDB_ERR_GENERAL, "SetSQL(): can't set sqlstmt on opened state");
        }

    //������Ѿ�����ռ��sqlstatement,����Close()���Ѿ��ͷţ���Ϊֻ����Close()��ſ��Ը���SQLstatementֵ
    fActivated  = false;
    fTotalRowsFetched = 0;
    fEof = false;
    fOpened = false;

    //����sql���
    if (fSqlStmt != NULL)
        delete[] fSqlStmt;
    int nLen = strlen(inSqlstmt);
    fSqlStmt = new char[nLen + 1];
    if (fSqlStmt == NULL)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::SetSQL()  MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED",__FILE__,__LINE__);
        throw TOraDBException(inSqlstmt, MDB_ERR_NOMORE_MEMORY_BE_ALLOCATED, "SetSQL()", __LINE__);
        }
    strcpy(fSqlStmt,inSqlstmt);
    fSqlStmt[nLen] = '\0';

    fErrorNo = OCIStmtPrepare(hStmt, hErr, (unsigned char *)fSqlStmt, strlen(fSqlStmt), OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT);
    fActivated = (fErrorNo == OCI_SUCCESS);
    CheckError();

    fErrorNo = OCIAttrGet(hStmt, OCI_HTYPE_STMT, &(this->fStmtType),  0, OCI_ATTR_STMT_TYPE, hErr);
    CheckError();
    GetParamsDef();
}


TOraDBParam *TOraDBQuery::ParamByName(const char *paramName) throw (TOraDBException)
{
    TOraDBParam *para = NULL;
    bool found = false;
    int i;

    if (fSqlStmt == NULL)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::ParamByName()  sql statement is empty",__FILE__,__LINE__);
        throw TOraDBException(paramName, MDB_ERR_GENERAL, "ParamByName(): sql statement is empty.");
        }

    for(i=0; i<fParamCount; i++)
    {
        found = CompareStrNoCase(Param(i).name,paramName);
        if ( found )
            break;
    }
    if ( found ) 
        para = &paramList[i];
    else 
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::ParamByName()  MDB_ERR_PARAM_NOT_EXISTS",__FILE__,__LINE__);
        throw TOraDBException(fSqlStmt, MDB_ERR_PARAM_NOT_EXISTS, paramName);
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
bool TOraDBQuery::IsParamExist(const char *paramName)
{
    for(int i=0; i<fParamCount; i++)
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
void TOraDBQuery::SetParameterNULL(const char *paramName) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->dataType = SQLT_LNG;
    fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name), //you don't have to pass any value/value length if the parameter value is null, or may raise oci success with info
        -1, (ub1 *)0,(sword)0, para->dataType, (dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);

    CheckError();
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
void TOraDBQuery::SetParameter(const char *paramName, double paramValue, bool isOutput ) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_FLT;
    para->dblValue = paramValue;

    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)para->name,
        -1,  (ub1 *)&(para->dblValue),(sb4)sizeof(para->dblValue), 
        para->dataType, (dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    else
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)para->name,
        strlen(para->name), (dvoid *)&(para->dblValue),(sb4)sizeof(para->dblValue), 
        para->dataType, (dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);

    CheckError();
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
void TOraDBQuery::SetParameter(const char *paramName, long paramValue, bool isOutput) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_INT;
    para->longValue = paramValue;
    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        -1, (ub1 *)&(para->longValue),(sword)sizeof(para->longValue), 
        para->dataType, (dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    else 
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        strlen(para->name), (ub1 *)&(para->longValue),(sword)sizeof(para->longValue), 
        para->dataType, (dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);

    CheckError();
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
void TOraDBQuery::SetParameter(const char *paramName, long long paramValue, bool isOutput) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_FLT;
    para->dblValue = (double)paramValue;

    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)para->name,
        -1,  (ub1 *)&(para->dblValue),(sb4)sizeof(para->dblValue), 
        para->dataType, (dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    else
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)para->name,
        strlen(para->name), (dvoid *)&(para->dblValue),(sb4)sizeof(para->dblValue), 
        para->dataType, (dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);

    CheckError();
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
void TOraDBQuery::SetParameter(const char *paramName, int paramValue, bool isOutput ) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_INT;
    para->intValue = paramValue;
    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        -1, (ub1 *)&(para->intValue),(sword)sizeof(para->intValue), 
        para->dataType, (dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    else 
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        strlen(para->name), (ub1 *)&(para->intValue),(sword)sizeof(para->intValue), 
        para->dataType, (dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);

    CheckError();
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
void TOraDBQuery::SetParameter(const char *paramName, const char paramValue, bool isOutput ) throw (TOraDBException)
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
void TOraDBQuery::SetParameter(const char *paramName, const char* paramValue, bool isOutput ) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_STR;
    if (para->stringValue)
        delete[] para->stringValue;

    int nLen;

    if (isOutput)
    {
        nLen = MDB_MAX_STRING_VALUE_LENGTH-1; 
        para->stringValue = new char[nLen+1];
        para->stringValue[nLen] = '\0';
    }
    else 
    {
        if(paramValue != NULL)
        {
            nLen = strlen(paramValue);
            para->stringValue = new char[nLen+1];
            strncpy((char *)para->stringValue,paramValue,nLen);
            para->stringValue[nLen] = '\0';
        }
        else
        {
            SetParameterNULL(paramName);
            return;
        }
    }

    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        -1,  (dvoid *)(para->stringValue),(sb4)(nLen+1), 
        para->dataType, (dvoid *) &para->indicator, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *)0, OCI_DEFAULT);
    else
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        (sb4)strlen(para->name), (dvoid *)(para->stringValue),(sb4)(nLen+1), 
        para->dataType, (dvoid *)&para->indicator, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    CheckError();
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
void TOraDBQuery::SetParameter(const char *paramName,const char* paramValue,int iBufferLen,bool isOutput) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_LBI;
    if (para->stringValue)
        delete[] para->stringValue;
    int iLen=0;

    if((paramValue==NULL)
        ||(iBufferLen<=0)
        )
    {
        SetParameterNULL(paramName);
        return;
    }
    else
    {
        iLen=iBufferLen;
        para->stringValue=new char[iLen+1];
        strncpy(para->stringValue,paramValue,iLen);
		para->stringValue[iLen] = '\0';
    }

    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        -1,  (dvoid *)para->stringValue,(sb4)(iLen), 
        para->dataType, (dvoid *) &para->indicator, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *)0, OCI_DEFAULT);
    else
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        (sb4)strlen(para->name), (dvoid *)para->stringValue,(sb4)(iLen), 
        para->dataType, (dvoid *)&para->indicator, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    CheckError();
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
void TOraDBQuery::SetParamArray(const char *paramName, char ** paramValue,int iStructSize,int iStrSize,int iArraySize,short* iIsNull,bool isOutput) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_STR;
    para->stringArray = paramValue;
    para->stringSize = iStrSize;


    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        -1,  (dvoid *)(para->stringArray),(sb4)(para->stringSize), 
        para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *)0, OCI_DEFAULT);
    else
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        (sb4)strlen(para->name), (dvoid *)(para->stringArray),(sb4)(para->stringSize), 
        para->dataType, (sb2 *)iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    CheckError();

    fErrorNo = OCIBindArrayOfStruct ( para->hBind, hErr,(ub4) iStructSize,(ub4)sizeof(sb2),0,0);
    CheckError();
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
void TOraDBQuery::SetParamArray(const char *paramName, int * paramValue, int iStructSize,int iArraySize, short* iIsNull,bool isOutput ) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_INT;
    para->intArray = paramValue;
    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        -1, (ub1 *)(para->intArray),(sword)sizeof(para->intArray[0]), 
        para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    else 
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        strlen(para->name),(ub1 *)(para->intArray),(sword)sizeof(para->intArray[0]), 
        para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);

    CheckError();
    fErrorNo = OCIBindArrayOfStruct ( para->hBind, hErr,(ub4) iStructSize,(ub4)sizeof(sb2),0,0);
    CheckError();
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
void TOraDBQuery::SetParamArray(const char *paramName, double * paramValue,int iStructSize, int iArraySize,short* iIsNull,bool isOutput)  throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_FLT;
    para->dblArray = paramValue;

    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)para->name,
        -1,  (ub1 *)(para->dblArray),(sb4)sizeof(para->dblArray[0]), 
        para->dataType,(sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    else
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)para->name,
        strlen(para->name), (dvoid *)(para->dblArray),(sb4)sizeof(para->dblArray[0]), 
        para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);

    CheckError();
    fErrorNo = OCIBindArrayOfStruct ( para->hBind, hErr,(ub4) iStructSize,(ub4)sizeof(sb2),0,0);
    CheckError();
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
void TOraDBQuery::SetParamArray(const char *paramName, long * paramValue, int iStructSize,int iArraySize, short* iIsNull,bool isOutput ) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_INT;
    para->longArray = paramValue;
    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        -1, (ub1 *)(para->longArray),(sword)sizeof(para->longArray[0]), 
        para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    else 
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        strlen(para->name),(ub1 *)(para->longArray),(sword)sizeof(para->longArray[0]), 
        para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);

    CheckError();
    fErrorNo = OCIBindArrayOfStruct ( para->hBind, hErr,(ub4) iStructSize,(ub4)sizeof(sb2),0,0);
    CheckError();
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
void TOraDBQuery::SetParamArray(const char *paramName, long long * paramValue, int iStructSize,int iArraySize, short* iIsNull,bool isOutput ) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    if(sizeof(long)==sizeof(long long))
    {
        para->fIsOutput = isOutput;
        para->dataType = SQLT_INT;
        para->longArray = (long*)paramValue;

        if (isOutput)
            fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
            -1, (ub1 *)(para->longArray),(sword)sizeof(para->longArray[0]), 
            para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
        else 
            fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
            strlen(para->name),(ub1 *)(para->longArray),(sword)sizeof(para->longArray[0]), 
            para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);

        CheckError();
        fErrorNo = OCIBindArrayOfStruct ( para->hBind, hErr,(ub4) iStructSize,(ub4)sizeof(sb2),0,0);
        CheckError();
    }
    else
    {
        para->llongArray=paramValue;

        int iElementCount=iArraySize/iStructSize;
        long long llElement=0;
        char *sParam=(char*)paramValue;

        para->dblArray=new double[iElementCount];
        memset(para->dblArray,0,sizeof(double)*iElementCount);
        for(int iIndex=0;iIndex<iElementCount;iIndex++)
        {
            memcpy(&llElement,sParam+iStructSize*iIndex,sizeof(long long));
            para->dblArray[iIndex]=(double)llElement;
        }
        iStructSize=sizeof(double);
        iArraySize=sizeof(double)*iElementCount;

        para->fIsOutput = isOutput;
        para->dataType = SQLT_FLT;
        

        if (isOutput)
            fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
            -1, (ub1 *)(para->dblArray),(sword)sizeof(para->dblArray[0]), 
            para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
        else 
            fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
            strlen(para->name),(ub1 *)(para->dblArray),(sword)sizeof(para->dblArray[0]), 
            para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    
        CheckError();
        fErrorNo = OCIBindArrayOfStruct ( para->hBind, hErr,(ub4) iStructSize,(ub4)sizeof(sb2),0,0);
        CheckError();
    }
   
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
void TOraDBQuery::SetBlobParamArray(const char *paramName,char *paramValue,int iBufferLen,int iArraySize,short* iIsNull,bool isOutput) throw (TOraDBException)
{
    TOraDBParam *para = ParamByName(paramName); //��ParamByName���Ѿ����жϲ����������׳��쳣

    para->fIsOutput = isOutput;
    para->dataType = SQLT_LBI;
    para->stringArray =(char**) paramValue;
    para->stringSize = iBufferLen;


    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        -1,  (dvoid *)(para->stringArray),(sb4)(para->stringSize), 
        para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *)0, OCI_DEFAULT);
    else
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        (sb4)strlen(para->name), (dvoid *)(para->stringArray),(sb4)(para->stringSize), 
        para->dataType, (sb2 *) iIsNull, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
	/*
    if (isOutput)
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        -1,  (dvoid *)(para->stringArray),(sb4)(para->stringSize), 
        para->dataType, (dvoid *) &para->indicator, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *)0, OCI_DEFAULT);
    else
        fErrorNo = OCIBindByName(hStmt, &para->hBind, hErr, (text *)(para->name),
        (sb4)strlen(para->name), (dvoid *)(para->stringArray),(sb4)(para->stringSize), 
        para->dataType, (dvoid *)&para->indicator, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT);
    */

    CheckError();
    fErrorNo = OCIBindArrayOfStruct ( para->hBind, hErr,(ub4) iBufferLen,(ub4)sizeof(sb2),0,0);
    CheckError();
}

void TOraDBQuery::CheckError(const char* sSql) throw (TOraDBException)
{
    if (fErrorNo != OCI_SUCCESS) 
        {
        //TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::CheckError()  Oracle OCI Call",__FILE__,__LINE__);
        throw TOraDBException(fErrorNo, hErr, "Oracle OCI Call", fSqlStmt);
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
bool TOraDBQuery::Execute(int iters) throw (TOraDBException)
{
    sb4 errcode;
    static text errbuf[MDB_MAX_ERRMSG_LENGTH-1];
    bool exeResult = false;
#ifdef __DEBUG__
    bExecuteFlag = true;
#endif

    if (fSqlStmt == NULL)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Execute()  sql statement is not presented",__FILE__,__LINE__);
        throw TOraDBException("", MDB_ERR_GENERAL, "Execute(): sql statement is not presented");
        }

    if  (this->fStmtType == OCI_STMT_SELECT)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Execute()  Can't Execute a select statement",__FILE__,__LINE__);
        throw TOraDBException( fSqlStmt, MDB_ERR_GENERAL, "Execute(): Can't Execute a select statement.");
        }
    fErrorNo = OCIStmtExecute(db->hDBSvc, hStmt, hErr, (ub4)iters, (ub4)0, 0, 0, OCI_DEFAULT);
    OCIAttrGet((dvoid*)hStmt, OCI_HTYPE_STMT, (dvoid *)&fTotalRowsFetched, (ub4 *)0, OCI_ATTR_ROW_COUNT, hErr);
    nTransTimes ++;

    if (fErrorNo == OCI_SUCCESS)
        exeResult = true;
    else if ( fErrorNo == OCI_ERROR ) //���������ؿղ���(1405)
    {
        OCIErrorGet (hErr, (ub4) 1, (text *) NULL, &errcode,
            errbuf, (ub4) sizeof(errbuf), (ub4) OCI_HTYPE_ERROR);
        if (errcode == 1405) 
            exeResult = true;
        else 
            CheckError();
    }
    else 
    {
        CheckError();
    }

    return exeResult;
}


void TOraDBQuery::GetParamsDef() throw (TOraDBException)
{
    char *params[MDB_MAX_PARAMS_COUNT];
    int i, in_literal, n, nParamLen,nFlag = 0;
    char *cp,*ph;
    char *sql;

    int nLen = strlen(this->fSqlStmt);
    sql = new char[nLen+1];
    strcpy(sql, this->fSqlStmt);
    sql[nLen] = '\0';

    if (fParamCount>0)
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
                TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::GetParamsDef()   param count execedes max numbers, please refer to OCIQuery.h",__FILE__,__LINE__);
                throw TOraDBException(fSqlStmt, MDB_ERR_CAPABILITY_NOT_YET_SUPPORT, " param count execedes max numbers, please refer to OCIQuery.h");
                }
            nParamLen = strlen((char *)ph);

            params[i] = new char[nParamLen+1];
            strcpy(params[i],(char *)ph);
            params[i][nParamLen] = '\0';
            i++;
            if(nFlag == 1) break;
        }   
    }
    delete[] sql;

    fParamCount = i;
    if (fParamCount>0)
    {
        paramList = new TOraDBParam[fParamCount];

        for (i=0; i<fParamCount; i++)
        {
            nParamLen = strlen(params[i]);
            paramList[i].name = new char[nParamLen+1];
            strncpy(paramList[i].name, params[i], nParamLen);
            paramList[i].name[nParamLen] = '\0';
            delete[] params[i];
        }
    }
}

/******************************************************************************
* ��������	:  Open()
* ��������	:  ��SQL SELECT��䷵�ؽ���� 
* ����		:  prefetchRows Ԥ����ǰ��¼����Ĭ��200��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TOraDBQuery::Open(int prefetch_Row) throw (TOraDBException)
{
    fPrefetchRows = prefetch_Row;
    if (fOpened)
    {
        fCurrRow = 0;
        fFetched = 0;
        //fCurrRow = 0;
        fTotalRowsFetched = 0;
    }
    if (fSqlStmt == NULL)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Open()   sql statement is empty",__FILE__,__LINE__);
        throw TOraDBException("", MDB_ERR_GENERAL, "Open(): sql statement is empty.");
        }

    if ( this->fStmtType !=OCI_STMT_SELECT)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Open()   Can't open none-select statement",__FILE__,__LINE__);
        throw TOraDBException( fSqlStmt, MDB_ERR_GENERAL, "Can't open none-select statement");
        }

    GetFieldsDef();
    fBof = true;
    fOpened = true;
}

/******************************************************************************
* ��������	:  FieldCount()
* ��������	:  �ܹ��м�����
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ����
* ����		:  li.shugang
*******************************************************************************/
int TOraDBQuery::FieldCount()
{
    return fFieldCount;
}

/******************************************************************************
* ��������	:  ParamCount()
* ��������	:  ��ȡ��������  
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��������
* ����		:  li.shugang
*******************************************************************************/
int TOraDBQuery::ParamCount()
{
    return fParamCount;
}

/******************************************************************************
* ��������	:  GetFieldPrecision()
* ��������	:  ����ָ������Ϣ  
* ����		:  index ������
* ���		:  ��
* ����ֵ	:  �ж���
* ����		:  li.shugang
*******************************************************************************/
TOraDBField& TOraDBQuery::Field(int i) throw (TOraDBException)
{
    if (fSqlStmt == NULL)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Field()   sql statement is not presented",__FILE__,__LINE__);
        throw TOraDBException("", MDB_ERR_GENERAL, "Field(i): sql statement is not presented");
        }

    if ( (i>=0) && (i<fFieldCount) ) 
        return fieldList[i];
    else 
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Field()   field index out of bound when call Field(i)",__FILE__,__LINE__);
        throw TOraDBException(fSqlStmt , MDB_ERR_INDEX_OUT_OF_BOUND, "field index out of bound when call Field(i)");
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
TOraDBField& TOraDBQuery::Field(const char *fieldName) throw (TOraDBException)
{
    int i;
    bool found = false;

    if (fSqlStmt == NULL)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Field()   sql statement is not presented",__FILE__,__LINE__);
        throw TOraDBException("", MDB_ERR_GENERAL, "Field(*fieldName): sql statement is not presented");
        }

    if (! fOpened)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Field()   can not access field before open",__FILE__,__LINE__);
        throw TOraDBException(fSqlStmt, MDB_ERR_GENERAL, "can not access field before open");
        }

    for(i=0; i<fFieldCount; i++)
    {
        found = CompareStrNoCase(Field(i).name,fieldName);
        if ( found )
            break;
    }
    if ( found ) 
        return fieldList[i];
    else 
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Field()   MDB_ERR_FIELD_NOT_EXISTS",__FILE__,__LINE__);
        throw TOraDBException(fSqlStmt, MDB_ERR_FIELD_NOT_EXISTS, fieldName);
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
bool TOraDBQuery::IsFieldExist(const char *fieldName)
{
    bool found = false;
    for(int i=0; i<fFieldCount; i++)
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
TOraDBParam& TOraDBQuery::Param(int index) throw (TOraDBException)
{
    if (fSqlStmt == NULL)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Param()   MDB_ERR_FIELD_NOT_EXISTS",__FILE__,__LINE__);
        throw TOraDBException("", MDB_ERR_GENERAL, "Param(index): sql statement is not presented");
        }

#ifdef debug
    printf("param i constructor\n");
#endif

    if ( (index>=0) && (index<fParamCount) ) 
        return paramList[index];
    else
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Param()   param index out of bound when call Param(i)",__FILE__,__LINE__);
        throw TOraDBException(fSqlStmt , MDB_ERR_INDEX_OUT_OF_BOUND, "param index out of bound when call Param(i)");
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
TOraDBParam& TOraDBQuery::Param(const char *inName) throw (TOraDBException)
{
    int i;
    bool found = false;

    if (fSqlStmt == NULL)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Param()   sql statement is not presented",__FILE__,__LINE__);
        throw TOraDBException("", MDB_ERR_GENERAL, "Param(paramName): sql statement is not presented");
        }

    for(i=0; i<fParamCount; i++)
    {
        found = CompareStrNoCase(paramList[i].name,inName);
        if (found)
            break;
    }
    if ( found ) 
        return paramList[i];
    else
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBDatabase::Param()   MDB_ERR_PARAM_NOT_EXISTS",__FILE__,__LINE__);
        throw TOraDBException(fSqlStmt, MDB_ERR_PARAM_NOT_EXISTS, (const char*)inName);
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
bool TOraDBQuery::Next() throw (TOraDBException)
{
    int fCanFetch = 1;                  //��ǰ��¼ָ���λ���Ƿ���Դ�ȡ����
    int tmpFetchedAllRows;

    sb4 errcode;
    static text errbuf[MDB_MAX_ERRMSG_LENGTH];
    bool exeResult = true;

    if (fSqlStmt == NULL)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBQuery::Next()    sql statement is not presented",__FILE__,__LINE__);
        throw TOraDBException("", MDB_ERR_GENERAL, "Next(): sql statement is not presented");
        }

    if (!fOpened)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBQuery::Next()    can not access data before open it",__FILE__,__LINE__);
        throw TOraDBException(fSqlStmt, MDB_ERR_GENERAL, "Next(): can not access data before open it");
        }

    fCurrRow ++ ;
    if( (fCurrRow == fFetched) && (fFetched < fPrefetchRows)) 
    {
        fCanFetch=0;
        fCurrRow--;
    }
    else if(fCurrRow==fFetched || ! fFetched)
    {
        //���FiledList�Ļ�����
        if(fieldList!=NULL)
        {
            for(int iIndex=0;iIndex<fFieldCount;iIndex++)
            {
                fieldList[iIndex].ClearDataBuf();
            }
        }

        fErrorNo = OCIStmtFetch(hStmt, hErr, (ub4)fPrefetchRows, (ub4) OCI_FETCH_NEXT, (ub4) OCI_DEFAULT);  
        if ( fErrorNo == OCI_ERROR ) //���������ؿ���(1405),�����������Է��ر��ضϵ���(1406)
        {
            OCIErrorGet (hErr, (ub4) 1, (text *) NULL, &errcode,
                errbuf, (ub4) sizeof(errbuf), (ub4) OCI_HTYPE_ERROR);
            if (errcode == 1405)
                exeResult = true;
            else CheckError();
        }

        tmpFetchedAllRows = fTotalRowsFetched;
        fErrorNo = OCIAttrGet((dvoid*)hStmt, OCI_HTYPE_STMT, (dvoid *)&fTotalRowsFetched, (ub4 *)0, OCI_ATTR_ROW_COUNT, hErr);
        fFetched = fTotalRowsFetched - tmpFetchedAllRows;
        if(fFetched) 
        {
            fCanFetch=1;
            fCurrRow=0;
        }
        else 
        {
            fCanFetch=0;
            fCurrRow= 0;
        }
        switch(fErrorNo)
        {
            case OCI_SUCCESS:
                exeResult = true;
                break;
            case OCI_NO_DATA:
                exeResult = false;
                break;
            case OCI_ERROR:
                 {
                    OCIErrorGet (hErr, (ub4) 1, (text *) NULL, &errcode,errbuf, (ub4) sizeof(errbuf), (ub4) OCI_HTYPE_ERROR);
                    if (errcode == 1405)
                        exeResult = true;
                    else CheckError();
                 }
                break;
            default:
                CheckError();
                break;
        };
    }

    fBof = false;
    fEof = (fFetched && !fCanFetch);
    return (exeResult && fCanFetch);
}


/****************** parameter implementation **************************/
TOraDBParam::TOraDBParam()
{
	name = NULL;
    fIsOutput = false;
    stringValue = NULL;
    indicator = 0;
    hBind = (OCIBind *) 0; 
    dblArray=NULL;
    llongArray=NULL;
	intArray = NULL;
	longArray = NULL;
	stringArray = NULL;
}

TOraDBParam::~TOraDBParam()
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
int TOraDBParam::AsInteger() throw (TOraDBException)
{
    if ( isNULL() )
        intValue = 0;

    if (dataType == SQLT_INT)
        return intValue;
    else    
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBQuery::Next()    MDB_ERR_READ_PARAM_DATA",__FILE__,__LINE__);
        throw TOraDBException("TOraDBParam", MDB_ERR_READ_PARAM_DATA, name, "AsInteger()");   
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
long TOraDBParam::AsLong() throw (TOraDBException)
{
    if ( isNULL() )
        longValue = 0;

    if (dataType == SQLT_LNG)
        return longValue;
    else    
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBQuery::AsLong()    MDB_ERR_READ_PARAM_DATA",__FILE__,__LINE__);
        throw TOraDBException("TOraDBParam", MDB_ERR_READ_PARAM_DATA, name, "AsLong()");  
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
double TOraDBParam::AsFloat() throw (TOraDBException)
{
    if ( isNULL() )
        dblValue = 0;

    if (dataType == SQLT_FLT)
        return dblValue;
    else    
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBQuery::AsFloat()    MDB_ERR_READ_PARAM_DATA",__FILE__,__LINE__);
        throw TOraDBException("TOraDBParam", MDB_ERR_READ_PARAM_DATA, name, "AsFloat()");
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
char* TOraDBParam::AsString() throw (TOraDBException)
{
    if ( isNULL() )
        stringValue[0] = '\0';

    if (dataType == SQLT_STR)
        return stringValue;
    else    
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBQuery::AsString()    MDB_ERR_READ_PARAM_DATA",__FILE__,__LINE__);
        throw TOraDBException("TOraDBParam", MDB_ERR_READ_PARAM_DATA, name, "AsString()");
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
bool TOraDBParam::isNULL() throw (TOraDBException)
{
    if (! fIsOutput)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBQuery::isNULL()    MDB_ERR_READ_PARAM_DATA",__FILE__,__LINE__);
        throw TOraDBException("TOraDBParam, not an output parameter", MDB_ERR_READ_PARAM_DATA, name, "isNULL()");
        }
    return (indicator == -1);
}

/*********************************************************************************
*TOraDBSession implementation
*********************************************************************************/
TOraDBSession::TOraDBSession(TOraDBDatabase *pDB)
{
    if(!pDB->fConnected)
        {
        TADD_ERROR(ERROR_UNKNOWN,"[%s:%d] TOraDBQuery::TOraDBSession()     Can not create a TOraDBSession when the database is not connected",__FILE__,__LINE__);
        throw TOraDBException("", MDB_ERR_GENERAL, "TOraDBSession(pDB): Can not create a TOraDBSession when the database is not connected");
        }

    m_hSession = NULL;
    m_hSrvCtx = NULL;
    m_bActive = FALSE;

    OCIHandleAlloc((dvoid *)pDB->hEnv,(dvoid **)&m_hSession,(ub4)OCI_HTYPE_SESSION,(size_t)0,(dvoid **) 0);

    OCIHandleAlloc((dvoid *)pDB->hEnv,(dvoid **)&m_hSrvCtx,(ub4)OCI_HTYPE_SVCCTX,(size_t)0,(dvoid **) 0);

    OCIHandleAlloc((dvoid *)pDB->hEnv,(dvoid **)&m_hError,(ub4)OCI_HTYPE_ERROR,(size_t)0,(dvoid **)0);

    m_iErrorNo = OCIAttrSet(m_hSrvCtx,OCI_HTYPE_SVCCTX,pDB->hSvr,0,OCI_ATTR_SERVER,m_hError);
    CheckError();

    //set the username/password in session handle
    m_iErrorNo = OCIAttrSet(m_hSession,OCI_HTYPE_SESSION,pDB->usr,strlen(pDB->usr),OCI_ATTR_USERNAME,m_hError);
    m_iErrorNo = OCIAttrSet(m_hSession,OCI_HTYPE_SESSION,pDB->pwd,strlen(pDB->pwd),OCI_ATTR_PASSWORD,m_hError);

    //set the Authentication handle in the server context handle
    m_iErrorNo = OCIAttrSet(m_hSrvCtx,OCI_HTYPE_SVCCTX,m_hSession,0,OCI_ATTR_SESSION,m_hError);
    CheckError();

};

TOraDBSession::~TOraDBSession()
{
    if(m_bActive)
        sessionEnd();

    OCIHandleFree((dvoid *)m_hSession,(ub4)OCI_HTYPE_SESSION);
    OCIHandleFree((dvoid *)m_hSrvCtx,(ub4)OCI_HTYPE_SVCCTX);

}

/******************************************************************************
* ��������	:  sessionBegin()
* ��������	:  ��ʼ�ػ�
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TOraDBSession::sessionBegin()
{
    if(m_bActive)
        return;

    //begin a session
    m_iErrorNo = OCISessionBegin(m_hSrvCtx,m_hError,m_hSession,OCI_CRED_RDBMS,(ub4)OCI_DEFAULT);
    CheckError();

    m_bActive = TRUE;                            
}

/******************************************************************************
* ��������	:  sessionEnd()
* ��������	:  ����һ���ػ� 
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TOraDBSession::sessionEnd()
{
    if(!m_bActive)
        return;
    //end a session
    m_iErrorNo = OCISessionEnd(m_hSrvCtx,m_hError,m_hSession,OCI_DEFAULT);
    CheckError();   
}

/******************************************************************************
* ��������	:  errprint()
* ��������	:  ��ӡ������Ϣ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
static void errprint(dvoid *errhp, ub4 htype, sb4 *errcodep)
{
    text errbuf[512];

    if (errhp)
    {
        sb4  errcode;

        if (errcodep == (sb4 *)0)
            errcodep = &errcode;

        (void) OCIErrorGet((dvoid *)errhp, (ub4) 1, (text *) NULL, errcodep,
            errbuf, (ub4) sizeof(errbuf), htype);
        (void) printf("Error - %.*s\n", 512, errbuf);
    }
}

/******************************************************************************
* ��������	:  CheckError()
* ��������	:  ��鵱ǰ������Ϣ
* ����		:  ��
* ���		:  ��
* ����ֵ	:  ��
* ����		:  li.shugang
*******************************************************************************/
void TOraDBSession::CheckError()
{

    switch (m_iErrorNo)
    {
    case OCI_SUCCESS:
        break;
    case OCI_SUCCESS_WITH_INFO:
        (void) printf( "Error - OCI_SUCCESS_WITH_INFO\n");
        errprint(m_hError, OCI_HTYPE_ERROR, &m_iErrorNo);
        break;
    case OCI_NEED_DATA:
        (void) printf( "Error - OCI_NEED_DATA\n");
        break;
    case OCI_NO_DATA:
        (void) printf( "Error - OCI_NODATA\n");
        break;
    case OCI_ERROR:
        errprint(m_hError, OCI_HTYPE_ERROR, &m_iErrorNo);
        break;
    case OCI_INVALID_HANDLE:
        (void) printf( "Error - OCI_INVALID_HANDLE\n");
        break;
    case OCI_STILL_EXECUTING:
        (void) printf( "Error - OCI_STILL_EXECUTE\n");
        break;
    case OCI_CONTINUE:
        (void) printf( "Error - OCI_CONTINUE\n");
        break;
    default:
        break;
    }  
}

//}

#endif //DB_ORACLE

