#ifndef _MDB_NOSQL_QUERY_H_
#define _MDB_NOSQL_QUERY_H_

#include "Interface/mdbQuery.h"

class TMdbColumn;
class TMdbTable;
class TMdbKeyValue
{
public:
    TMdbKeyValue();
    ~TMdbKeyValue();
    
    void Clear();
    void SetNULL();
    bool IsNULL();

public:
    long long  lValue; //long long
    double     dValue;//double
    char * sValue;    //string
    int iSize;		  //string size;
    int iDataType;
    bool  bNull;  /* Some combination of MEM_Null, MEM_Str, MEM_Dyn, etc. */
    TMdbColumn * pColumn;//TK_ID所对应pColumn信息
    
};

class TMdbNosqlInfo
{
public:
    TMdbNosqlInfo();
    ~TMdbNosqlInfo();
    void Clear();

public:
    TMdbTable* m_pCurTable;
    std::vector<TMdbKeyValue*> m_vKey;
};

#endif