#include "Interface/mdbNosqlQuery.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbStruct.h"





#define NOSQL_THROW(_errCode,FMT,...) \
TADD_ERROR(ERROR_UNKNOWN,FMT,##__VA_ARGS__);\
throw TMdbException(_errCode,"NOSQL","File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);


TMdbKeyValue::TMdbKeyValue()
{
    lValue= 0;
    dValue=0.0;
    sValue = NULL;
    iSize = 0;
    bNull=false;
    iDataType = DT_Unknown;
    pColumn=NULL;
}

TMdbKeyValue::~TMdbKeyValue()
{
    Clear();
}

void TMdbKeyValue::Clear()
{
    lValue= 0;
    dValue=0.0;
    SAFE_DELETE_ARRAY(sValue);
    iSize = 0;
    bNull=false;
    iDataType = DT_Unknown;
    pColumn=NULL;
}

bool TMdbKeyValue::IsNULL()
{
    return bNull;
}


void TMdbKeyValue::SetNULL()
{
    bNull = true;
}


TMdbNosqlInfo::TMdbNosqlInfo()
{
    m_pCurTable = NULL;
}

TMdbNosqlInfo::~TMdbNosqlInfo()
{
    Clear();
}


void TMdbNosqlInfo::Clear()
{
    m_pCurTable = NULL;
    std::vector<TMdbKeyValue*>::iterator itor = m_vKey.begin();
    for(; itor != m_vKey.end(); itor++)
    {
        TMdbKeyValue* pKv = *(itor);
        SAFE_DELETE(pKv);
    }
    m_vKey.clear();
    
}





TMdbNosqlQuery::TMdbNosqlQuery(TMdbDatabase *pTMdbDatabase,int iSQLFlag)
{
    m_pMdb = pTMdbDatabase;
    if(NULL == m_pMdb)
    {
        NOSQL_THROW(ERR_DB_NOT_CREATE,"","mdb not connected.");
    }

    m_pShmDsn = m_pMdb->GetShmDsn();

    m_pNosqlInfo = new TMdbNosqlInfo();
    if(NULL == m_pNosqlInfo)
    {
        NOSQL_THROW(ERR_OS_NO_MEMROY,"", "new TMdbNosqlInfo object failed.");
    }
	m_pQuery = NULL;
}

TMdbNosqlQuery::~TMdbNosqlQuery()
{
    m_pQuery->Close();
    SAFE_DELETE(m_pQuery);
    SAFE_DELETE(m_pNosqlInfo);
}


bool TMdbNosqlQuery::IsLinkOK()
{
    return m_pMdb->IsLinkOK();
}


void TMdbNosqlQuery::SetTable(const char* psTableName) throw (TMdbException)
{
    if(!IsLinkOK())
    {
        NOSQL_THROW(ERR_DB_NOT_CONNECTED,"", "Link is down.");
    }

    if(NULL == psTableName)
    {
        NOSQL_THROW(ERR_TAB_TABLE_NAME_INVALID,"",  "table name is null.input parameter invalid");
    }
    
    m_pNosqlInfo->m_pCurTable = m_pShmDsn->GetTableByName(psTableName);
    if(NULL == m_pNosqlInfo->m_pCurTable)
    {
        NOSQL_THROW(ERR_TAB_NO_TABLE,"",  "table[%s] is not exist", psTableName);
    }

    return ;
}


void TMdbNosqlQuery::SetKey(const char* psKeyName, const char* psValue) throw (TMdbException)
{
    if(NULL == psKeyName || NULL == psValue)
    {
        NOSQL_THROW(ERR_APP_INVALID_PARAM,"",  "key name or key value is null.input parameter invalid");
    }

    if(NULL == m_pNosqlInfo->m_pCurTable)
    {
        NOSQL_THROW(ERR_APP_INVALID_PARAM,"",  "table is not set ,pls SetTable  first.");
    }

    TMdbColumn* pColumn = m_pNosqlInfo->m_pCurTable->GetColumnByName(psKeyName);
    if(NULL == pColumn)
    {
        NOSQL_THROW(ERR_TAB_NO_COLUMN,"",  "key column [%s] not exist.", psKeyName);
    }

    TMdbKeyValue* pKey = new TMdbKeyValue();
    if(NULL == pKey)
    {
        NOSQL_THROW(ERR_OS_NO_MEMROY,"",  "new TMdbKeyValue object failed.");
    }
    pKey->Clear();

    pKey->iSize = strlen(psValue);
    pKey->pColumn = pColumn;
    pKey->sValue =  new char[pKey->iSize+1];
    if(NULL == pKey->sValue)
    {
        NOSQL_THROW(ERR_OS_NO_MEMROY,"",  "new char[] object failed.", psKeyName);
    }
    memset(pKey->sValue, 0, pKey->iSize+1);
    SAFESTRCPY(pKey->sValue,pKey->iSize,psValue);
    pKey->iDataType = DT_Char;
    
    m_pNosqlInfo->m_vKey.push_back(pKey);
    
}

void TMdbNosqlQuery::SetKey(const char* psKeyName, int iKeyValue) throw (TMdbException)
{
    if(NULL == psKeyName)
    {
        NOSQL_THROW(ERR_APP_INVALID_PARAM,"",  "key name is null.input parameter invalid");
    }

    if(NULL == m_pNosqlInfo->m_pCurTable)
    {
        NOSQL_THROW(ERR_APP_INVALID_PARAM,"",  "table is not set ,pls SetTable  first.");
    }

    TMdbColumn* pColumn = m_pNosqlInfo->m_pCurTable->GetColumnByName(psKeyName);
    if(NULL == pColumn)
    {
        NOSQL_THROW(ERR_TAB_NO_COLUMN,"",  "key column [%s] not exist.", psKeyName);
    }

    TMdbKeyValue* pKey = new TMdbKeyValue();
    if(NULL == pKey)
    {
        NOSQL_THROW(ERR_OS_NO_MEMROY,"",  "new TMdbKeyValue object failed.");
    }
    pKey->Clear();

    pKey->pColumn = pColumn;
    pKey->lValue =  iKeyValue;
    pKey->iDataType = DT_Int;
    
    m_pNosqlInfo->m_vKey.push_back(pKey);
}

void TMdbNosqlQuery::SetKeyNULL(const char* psKeyName) throw (TMdbException)
{
    if(NULL == psKeyName)
    {
        NOSQL_THROW(ERR_APP_INVALID_PARAM,"",  "key name is null.input parameter invalid");
    }

    if(NULL == m_pNosqlInfo->m_pCurTable)
    {
        NOSQL_THROW(ERR_APP_INVALID_PARAM,"",  "table is not set ,pls SetTable  first.");
    }

    TMdbColumn* pColumn = m_pNosqlInfo->m_pCurTable->GetColumnByName(psKeyName);
    if(NULL == pColumn)
    {
        NOSQL_THROW(ERR_TAB_NO_COLUMN,"",  "key column [%s] not exist.", psKeyName);
    }

    TMdbKeyValue* pKey = new TMdbKeyValue();
    if(NULL == pKey)
    {
        NOSQL_THROW(ERR_OS_NO_MEMROY,"",  "new TMdbKeyValue object failed.");
    }
    pKey->Clear();
    pKey->pColumn = pColumn;
    pKey->SetNULL();
    
    m_pNosqlInfo->m_vKey.push_back(pKey);
    
}

void TMdbNosqlQuery::Find() throw (TMdbException)
{
    if(!IsLinkOK())
    {
        NOSQL_THROW(ERR_DB_NOT_CONNECTED,"", "Link is down.");
    }

    if(NULL == m_pNosqlInfo->m_pCurTable)
    {
        NOSQL_THROW(ERR_APP_INVALID_PARAM, "", "table is not set ,pls SetTable  first.");
    }

    if(NULL == m_pQuery)
    {
        m_pQuery = m_pMdb->CreateDBQuery();
        if(NULL == m_pQuery)
        {
            NOSQL_THROW(ERR_OS_NO_MEMROY, "", "CreateDBQuery failed.");
        }
        
    }
    
    
    char sSQL[MAX_SQL_LEN] = {0};
    snprintf(sSQL, sizeof(sSQL), "select * from %s", m_pNosqlInfo->m_pCurTable->sTableName);
    if(m_pNosqlInfo->m_vKey.size() >0 )
    {
        bool bFirst = true;
        std::vector<TMdbKeyValue*>::iterator itor = m_pNosqlInfo->m_vKey.begin();
        for(; itor != m_pNosqlInfo->m_vKey.end(); itor++)
        {
            TMdbKeyValue* pKv = *(itor);
            if(bFirst)
            {
                snprintf(sSQL+strlen(sSQL), sizeof(sSQL) - strlen(sSQL), " where %s=:%s ", pKv->pColumn->sName,pKv->pColumn->sName);
                bFirst =false;
            }
            else
            {
                snprintf(sSQL+strlen(sSQL), sizeof(sSQL) - strlen(sSQL), " and %s=:%s ", pKv->pColumn->sName,pKv->pColumn->sName);
            }
        }
    }

    m_pQuery->Close();
    m_pQuery->SetSQL(sSQL);
    std::vector<TMdbKeyValue*>::iterator itor = m_pNosqlInfo->m_vKey.begin();
    for(; itor != m_pNosqlInfo->m_vKey.end(); itor++)
    {
        TMdbKeyValue* pKv = *(itor);
        if(pKv->IsNULL())
        {
            m_pQuery->SetParameterNULL(pKv->pColumn->sName);
        }
        else
        {
            if(pKv->iDataType ==  DT_Int)
            {
                m_pQuery->SetParameter(pKv->pColumn->sName, pKv->lValue);
            }
            else
            {
                m_pQuery->SetParameter(pKv->pColumn->sName, pKv->sValue);
            }
            
        }
    }
    m_pQuery->Open();
    

    return ;

    
}

bool TMdbNosqlQuery::Next() throw (TMdbException)
{
    return m_pQuery->Next();
}

void TMdbNosqlQuery::GetValue(void *pStruct,int* Column) throw (TMdbException)
{
    m_pQuery->GetValue(pStruct,Column);
}


void TMdbNosqlQuery::Close()throw (TMdbException)
{
    if(NULL != m_pQuery)
    {
        m_pQuery->Close();
    }

    if(NULL != m_pNosqlInfo)
    {
        m_pNosqlInfo->Clear();
    }
}


void TMdbNosqlQuery::SetKey(const char* psKeyName, char cKeyValue) throw (TMdbException)
{
    return;
}

void TMdbNosqlQuery::SetKey(const char* psKeyName, double fKeyValue) throw (TMdbException)
{
    return;
}

void TMdbNosqlQuery::SetKey(const char* psKeyName, long lKeyValue) throw (TMdbException)
{
    return;
}


void TMdbNosqlQuery::SetKey(const char* psKeyName, long long llKeyValue) throw (TMdbException)
{
    return;
}











