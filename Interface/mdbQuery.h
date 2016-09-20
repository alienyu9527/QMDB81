/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--QuickMDBС��
*@                     All rights reserved.
*@Name��	    mdbQuery.cpp
*@Description��mdb����ķ��ʽӿ�
*@Author:	   jin.shaohua
*@Date��	    2012.05
*@History:
******************************************************************************************/
#ifndef __QUICK_MEMORY_DATABASE_QUERY_H__
#define __QUICK_MEMORY_DATABASE_QUERY_H__
#include <vector>
#include <map>
#include <string>
#include <string.h>


//namespace QuickMDB{


class TMdbDatabase;
class TMdbSqlParser;
class TMdbExecuteEngine;
class TMdbShmDSN;
class TMdbRollback;
class TMdbLocalLink;
class TMdbConfig;
class TMdbLinkCtrl;
class TMdbRBCtrl;
class TMdbMultiProtector;
class TMdbClientEngine;
class TMdbDDLExecuteEngine;
class TMdbNosqlQuery;

struct _ST_MEM_VALUE;
typedef struct _ST_MEM_VALUE ST_MEM_VALUE;

struct _ST_MEM_VALUE_LIST;
typedef struct _ST_MEM_VALUE_LIST ST_MEM_VALUE_LIST;

//SetSQLʱ��������SQL����
#define QUERY_NO_ROLLBACK 0x01   //��Query����ع�
#define QUERY_NO_ORAFLUSH 0x02  //��Query������Oracleˢ��
#define QUERY_NO_REDOFLUSH 0x04 //��Query��������redo��־
#define QUERY_NO_SHARDFLUSH 0x08 // ��Query�������ɷ�Ƭ����ͬ��

#ifndef MAX_COLUMN_COUNTS
#define MAX_COLUMN_COUNTS  64
#endif



#define TMdbClientException TMdbException




class NoOcpParse;

/******************************************************************************
* ������	:  TMdbException
* ������	:  mdb�쳣��
* ����		:
*******************************************************************************/
class TMdbException
{
public:
    ~TMdbException() ;
    TMdbException();
    TMdbException(int errId,const char *pszSql, const char* pszFormat, ...);
    const char *GetErrMsg() ;
    int GetErrCode() ;
    const char *GetErrSql();
protected:
    char m_sErrMsg[1024];		 //������Ϣ
    int m_iErrCode; 			 //�����
    char m_sErrSql[4096];        //SQL
};
/******************************************************************************
* ������	:  TMdbNullArrWrap
* ������	:  null�����װ��
* ����		:
*******************************************************************************/
class TMdbNullArrWrap
{
public:
    TMdbNullArrWrap();
    ~TMdbNullArrWrap();
    int Init(bool * bNullArr,int iCount);//��ʼ��
    bool IsNull(int iIndex);//�ж��Ƿ�ΪNULL
    int Assign(int iCount);//�����ڴ�
    int SetNull(int iIndex);//����NULL;
private:
   bool * m_bNullArr;
   int m_iAlloc;
};

/******************************************************************************
* ������	:  TMdbParamArray
* ������	:  �������飬����������
* ����		:
*******************************************************************************/
class TMdbParamArray
{
public:
    TMdbParamArray();
    ~TMdbParamArray();
    int ReAlloc(int iArraySize,int iType);
public:
    int m_iParamIndex;//����λ��
    int m_iParamType;//��������
    int m_iArraySize;//�����С
    int m_iAllocSize;//����Ŀռ��С
    long long * m_pllValue;
    char **      m_psValue;
    TMdbNullArrWrap m_tNullArrWrap;//null��ʶ��װ
};
/******************************************************************************
* ������	:  TMdbParam
* ������	:  mdb SQL �󶨱�������
* ����		:
*******************************************************************************/
class TMdbParam
{
public:
    TMdbParam();
    ~TMdbParam();
    int Clear();//����
    int AddParam(const char * sParamName);//��Ӳ���
    const char * GetParamNameByIndex(int iIndex);//���ݲ���index��ȡ������
    int GetParamIndexByName(const char * sParamName);//���ݲ�������ȡ����index
    int GetCount();
    void NewParamPool();
    void InitParamPool();
private:
    std::vector<std::string> m_vParamName;//������
    char *m_pParamPool;
    int m_iParamPoolCurPos;
    int m_iParamComparePos;
    int m_iCount;
};

/******************************************************************************
* ������	:  TMdbField
* ������	:  select��Field�Ķ���
* ����		:
*******************************************************************************/
class TMdbField
{
public:
    TMdbField();
    ~TMdbField();
    bool   isNULL(); //�жϸ�ֵ�Ƿ���nullֵ
    char * GetName();//field������
    char*  AsString() throw (TMdbException);
    double AsFloat() throw (TMdbException);
    long long  AsInteger() throw (TMdbException);
    void AsBlobBuffer(unsigned char *buffer, int &iBufferLen) throw (TMdbException);
    void  AsDateTime(int &iYear,int &iMonth,int &iDay,int &iHour,int &iMinute,int &iSecond) throw (TMdbException);//�������ڵĸ�������
    char* AsDateTimeString() throw (TMdbException); //YYYYMMDDHHMISS
    char*  AsRealStr();
    long long  AsRealnt();
    void ClearDataBuf();
    int GetDataType();//
    //char  m_sValue[8192];  //�洢�ַ���
    char  m_sValue[32];
    ST_MEM_VALUE * m_pMemValue;      //�ڴ�ֵ
};

/******************************************************************************
* ������	:  TMdbQuery
* ������	:  ��ѯ�ӿ�
* ����		:
*******************************************************************************/

class TMdbColumnAddr;
class TMdbQuery
{
public:
    TMdbQuery(TMdbDatabase *pTMdbDatabase,int iSQLFlag);
    ~TMdbQuery();
    void Close() ;//�ر�SQL��䣬��׼��������һ��sql���
    void CloseSQL(); //Close the cursor associated with the SELECT statement
    const char* GetSQL();
    void ExecuteDDLSQL()throw (TMdbException);//ִ��DDL ���
    int InitSqlBuff(bool & bFirstSet);
	int ParseSQLForRollback(char* sSqlStatement);
    /******************************************************************************
    * ��������	:  SetSQL
    * ��������	:  ����SQL
    * ����		:  sSqlStatement - SQL��䣬iPreFetchRows - �����ô�
    * ����		:  
    * ���		:  
    * ����ֵ	:  
    * ����		:  
    *******************************************************************************/
    void SetSQL(const char *sSqlStatement,int iPreFetchRows=-1) throw (TMdbException);
    /******************************************************************************
    * ��������	:  SetSQL
    * ��������	:  ����SQL
    * ����		:  sSqlStatement - SQL��䣬iPreFetchRows - �����ô�
    * ����		:  iFlag - SQL����QUERY_NO_ROLLBACK,QUERY_NO_ORAFLUSH,QUERY_NO_REPFLUSH,QUERY_NO_PEERFLUSH
    * ���		:  
    * ����ֵ	:  
    * ����		:  
    *******************************************************************************/
    void SetSQL(const char *sSqlStatement,int iFlag,int iPreFetchRows) throw (TMdbException); //����Ҫִ�е�SQL

    //void CheckDDLSQL();//У��sql�Ƿ�ΪDDL���
    
    //void SetDDLSQL(const char *sSqlStatement,int iPreFetchRows=-1)throw (TMdbException);
    void SetRollBackSQL(const char *sSqlStatement) throw (TMdbException); //����Ҫִ�еĻع�SQL
    void Open(int prefetchRows=0) throw (TMdbException);  //��SQL SELECT��䷵�ؽ����,iPreFetchRows��ʵ������
    bool Next()throw (TMdbException);//�ƶ�����һ����¼
    bool Eof();
    bool IsFieldExist(const char *fieldName);
    /******************************************************************************
    * ��������	:  Execute
    * ��������	:  ִ��
    * ����		:  iExecuteRows -����������ʱ����д��Ҫִ�еļ�¼���������󶨿��Բ���д
    * ����		:  
    * ���		:  
    * ����ֵ	: 
    * ����		:  
    *******************************************************************************/
    bool Execute(int iExecuteRows=-1) throw (TMdbException); //ִ��SQL
    bool TransBegin(); //������
    bool Commit();//�����ύ
    bool Rollback();//����ع�
    int RowsAffected();//DELETE/UPDATE/INSERT����޸ĵļ�¼��Ŀ,SELECT���ĿǰNext֮��ļ�¼��
    //������صĲ���
    int FieldCount();	 //��ȡ�и���
    int ParamCount();//�󶨲�������
    TMdbField& Field(int iIndex) throw (TMdbException);		//����������ȡ��i����ʵ��,��0��ʼ
    TMdbField& Field(const char *sFieldName) throw (TMdbException);//����������ȡ��ʵ��
    void GetValue(void *pStruct,int* Column)throw (TMdbException);//ֱ�ӻ�ȡֵ
    void GetValue(TMdbColumnAddr* pTColumnAddr)throw (TMdbException);
    
    TMdbField& FieldPos(const char *sFieldName,int &pos,TMdbField** tField) throw (TMdbException);
    //���ݲ��������ò���ֵ
    bool IsParamExist(const char *paramName);
    /******************************************************************************
    * ��������	:  SetParameter
    * ��������	:  �������趨����ֵ
    * ����		:  iParamIndex/sParamName -��ʾ���󶨱�����pos����name
    * ����		:  *ParamValue - ��ʾ���󶨵�ֵ isOutput_Unused - �ݲ��õ��������ֵ
    * ���		:  
    * ����ֵ	:  
    * ����		:  
    *******************************************************************************/
    void SetParameter(const char *sParamName,const char* sParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName, const char cParamValue, bool isOutput = false) throw (TMdbException);
    void SetParameter(const char *sParamName,int iParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,long lParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,double dParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,long long llParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,const char* sParamValue,int iBufferLen, bool isOutput_Unused = false) throw (TMdbException);//���ڴ���BLOB/BINARY�����ֶ�
    void SetParameterNULL(const char *sParamName) throw (TMdbException);     //���ò���Ϊ��
    void SetParameter(int iParamIndex,const char* sParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex, const char cParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,int iParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,long lParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,double dParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,long long llParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,const char* sParamValue,int iBufferLen) throw (TMdbException);//���ڴ���BLOB/BINARY�����ֶ�
    void SetParameterNULL(int iParamIndex) throw (TMdbException);     //���ò���Ϊ��
    /******************************************************************************
    * ��������	:  SetParamArray
    * ��������	:  �����󶨱���
    * ����		:  iParamIndex/sParamName -��ʾ���󶨱�����pos����name
    * ����		:  *ParamValue - ��ʾ���󶨵�ֵ(����) iInterval - ÿ��Ԫ��֮��ļ��
    * ����		:  iElementSize - Ԫ�ش�С(��iInterval���ó�һ���Ϳ���),iArraySize -��������Ĵ�С
    * ����		:  bOutput - �ݲ�ʹ�ã�bNullArr-null�趨�����飬������Ҫ���Բ��
    * ����ֵ	:  
    * ����		:  
    *******************************************************************************/
    void SetParamArray(const char *sParamName,char **asParamValue,int iInterval,
                       int iElementSize,int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(const char *sParamName,int *aiParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr= NULL) throw (TMdbException);
    void SetParamArray(const char *sParamName,long *alParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr= NULL) throw (TMdbException);
    void SetParamArray(const char *sParamName,double *adParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr= NULL) throw (TMdbException);
    void SetParamArray(const char *sParamName,long long *allParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr= NULL) throw (TMdbException);
    void SetBlobParamArray(const char *sParamName,char *sParamValue,int iBufferLen,
                           int iArraySize,bool bOutput=false,bool * bNullArr= NULL) throw (TMdbException);//���ڴ���BLOB/BINARY�����ֶ��������

    void SetParamArray(int iParamIndex,char **asParamValue,int iInterval,
                       int iElementSize,int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(int iParamIndex,int *aiParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(int iParamIndex,long *alParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(int iParamIndex,double *adParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(int iParamIndex,long long *allParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetBlobParamArray(int iParamIndex,char *sParamValue,int iBufferLen,
                           int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);//���ڴ���BLOB/BINARY�����ֶ��������

    //������ݿ����,�д��쳣
    void CheckError(const char* sSql) throw (TMdbException) ;
    void SetSignal();
    //void SetDataSource(int iSourceId)throw (TMdbException);//��������Դ��������������
    ST_MEM_VALUE_LIST *  GetInputMemValue();//��ȡ��Ҫ�����memValue;
    int GetSQLType();//��ȡsql ����
    int GetParamIndexByName(const char *sParamName);//��ȡ������idx
    const char * GetParamNameByIndex(int iIndex);//����pos��ȡ�󶨲�����
    void SetTimeStamp(long long iTimeStamp);
    long long GetTimeStamp();    
    ST_MEM_VALUE * GetParamByIndex(int iIndex);//ͨ�����ֻ�ȡ����ֵ
    int FillFieldForCSBin(NoOcpParse &tParseData,bool bFirst);
	void SetCancelPoint(int* pPoint);
private:
    bool ExecuteOne()throw (TMdbException);//����ִ��
    bool ExecuteArray(int iExecuteRows)throw (TMdbException);//����ִ��
    bool OpenArray()throw(TMdbException);//������
    ST_MEM_VALUE * GetParamByName(const char *sParamName);//ͨ�����ֻ�ȡ����ֵ

    int FillOutputToField();//���fieldֵ
    void AddError();    //����ͳ��
    void AddSuccess();//��ȷͳ��
    bool IsLinkOK();//����Ƿ�������״̬
    int CheckAndSetSql(const char * sSql) throw (TMdbException);//�������SQL���Ϸ���
    void CleanRBUnit();//����ع���Ԫ
    bool IsCanAccess();//����������
    int  PrepareParam();// ׼����param��صĹ���
    bool IsAllSet();//�Ƿ��Ѿ�ȫ������
    bool CheckAndSetParamType(int iType)throw (TMdbException);//��鲢����paramtype
    bool IsCanRollback();//�Ƿ���Իع�
	bool DealTableVersion();
	bool IsNeedToCheckSQLRepeat(bool bFirstSet, int iFlag);
	
public:
    TMdbSqlParser * m_pMdbSqlParser;//new mdb sql parser
private:
    TMdbDatabase  *m_pMdb;    //���ݿ�ָ��
    TMdbExecuteEngine * m_pExecuteEngine;//SQLִ������
    std::vector<TMdbField *> m_vField;//fieldex
    bool m_bSetSQL;      //�Ƿ��Ѿ���ʼ��
    char* m_pszSQL; //�����ʱ��SQL���
    int m_iSQLBuffLen; //��ǰ���sql�ڴ泤��
    int m_iQueryFlag;//query flag

    int  m_iSetParamType;//setparam��ʽ
    bool * m_bSetList;//�Ƿ��Ѷ�����
    TMdbParamArray * m_pParamArray;//��������
    bool m_bFinishSet;//����set
    bool m_bOpen;//�Ƿ��Ѿ�open
    bool m_bFillField;//�Ƿ��Ѿ����fieldֵ
    int  m_iOpenArrayPos;//����open�İ󶨱�����pos;
    bool m_fLastRecordFlag;
    int m_iRowsAff;//Ӱ������
    TMdbDDLExecuteEngine * m_pDDLExecuteEngine;//SQLִ������
    std::map<std::string,int> m_mapTableNameToVersion;
    char *m_pParamPool;
    int m_iParamPoolCurPos;
    int m_iParamComparePos;
     //bool m_bIsDDLSQL;//�Ƿ���DDL

};

/******************************************************************************
* ������	:  TMdbDatabase
* ������	:  QMDB�����ݿ�������
* ����		:
*******************************************************************************/
class TMdbDatabase
{
    friend class TMdbQuery;
public:
    TMdbDatabase();
    ~TMdbDatabase();
    void SetLogin(const char *sUser,const char *sPassword,const char *sServerName) throw (TMdbException);       //���õ�½��Ϣ
    bool Connect(bool bIsAutoCommit=false) throw (TMdbException);                            //�������ݿ�
    bool Connect(const char *sUser,const char *sPassword,const char *sServerName,bool bIsAutoCommit=false) throw (TMdbException);       //�������ݿ�
    bool ConnectAsMgr(const char* sDSN) throw (TMdbException);
    int Disconnect() throw(TMdbException);                               	//�Ͽ����ݿ�����
    void TransBegin();                               																								//��������
    void Commit() ;
    void CommitEx() ;
    void Rollback() ;
    void DisableCommit();  //�����ύ
    void EnableCommit();   //�����ύ
    bool IsNullConnect();
    bool IsConnect() ;																													//�������ݿ��Ƿ���������
    TMdbQuery *CreateDBQuery() throw (TMdbException);
    TMdbNosqlQuery* CreateNosqlQuery() throw (TMdbException);
    void CheckError(const char* sSql=NULL) throw (TMdbException);
    const char* GetProvider();
    int SetSQL(const char* pszSQL,int iSqlPos) throw (TMdbException);  //����ִ�е�SQL���,���ض�Ӧ��λ��
    void SetSQLPos(int iPos) ;        //����ִ�е�SQL��λ��
    int ReleaseSysSQL() throw (TMdbException);
    int RowsAffected();
    TMdbShmDSN * GetShmDsn();
    bool IsLinkOK();
    int GetTransactionState();
    char* GetUser();
    char* GetPWD();
    char* GetDSN();
    int SetCaptureRouter(const char * sRouters);//���ò���·��,
    int GetCaptureRouter(char * sRouterRet);//��ȡ����·��
private:
    int LinkMonitor(); //���Ӽ�ؽ���
private:
    bool m_bConnectFlag;
    char m_sUser[128];
    char m_sPWD[128];
    char m_sDSN[128];

    TMdbConfig *m_pConfig;
    TMdbShmDSN * m_pShmDSN;
    TMdbLocalLink* m_pLocalLink;

    bool  m_bIsNeedCommit;
    static int m_iSQLFlag; //sql��ǩ
    bool m_bAsManager;//��Ϊ����Ա��½(������֤�û�������)
    int  m_iTransactionState;//����״̬
    int m_iSessionID;

    TMdbLinkCtrl * m_pLinkCtrl;//���ӹ���
    TMdbMultiProtector * m_pMultiProtector;//��������
    
	TMdbRBCtrl* m_pRBCtrl;

};


////////////////////////////////////////////////client query//////////////////////////////////////////////

class TMdbClientDatabase;
class TMdbClientField;
class TMdbClientQuery;
class TMdbCspParser;
class TMdbAvpHead;
class TMdbCspParserMgr;
class Socket;
class TMdbAvpItem;

/******************************************************************************
* ������	:  TMdbClientField
* ������	:  CSģʽ�µ�Field
* ����		:
*******************************************************************************/
class TMdbClientField
{
    friend class TMdbClientQuery;
public:
    TMdbClientField();
    ~TMdbClientField();
    bool       isNULL();
    char*      AsString() throw (TMdbException);
    double     AsFloat() throw (TMdbException);
    long long  AsInteger() throw (TMdbException);
    void       AsBlobBuffer(unsigned char *buffer, int &iBufferLen) throw (TMdbException);
    //�������ڵĸ�������
    void       AsDateTime(int &iYear,int &iMonth,int &iDay,int &iHour,int &iMinute,int &iSecond) throw (TMdbException);
    char*      AsDateTimeString() throw (TMdbException); //YYYYMMDDHHMISS
    void       ClearDataBuf();
    void       ClearDataBufPlus();
    int        DataType(void);
    const char* GetName();
    void SetUseOcp(bool bUseOcp){m_bUseOcp = bUseOcp;}
private:
    long long 	m_iValue;                //�洢������
    char*  	m_sValue;
    char*	m_sName;                 //�洢������
    char m_iDataType;             //�������ͣ�1-DT_Int, 2-DT_Char,3-DT_VarChar 4-DT_DateStamp 9-DT_Blob
    bool  	m_bIsNULL;                //�Ƿ�Ϊ��
    char m_sNullValue[1];//NULLֵ
    bool m_bUseOcp;
};
//bin parse
class NoOcpParse
{
public:
    NoOcpParse();
    ~NoOcpParse();
    /******************************************************************************
    * ��������	:  SetData
    * ��������	:  ��������
    * ����		:  bAnsCode��ʾ�Ƿ�ΪӦ����
    *******************************************************************************/
    void    SetData(char* pData,int iLen)
    {
        memcpy(m_pData+m_iSize,pData,(size_t)iLen);
        m_iSize += iLen;
    }
    void    SetData(int * pData,int iLen)
    {
        memcpy(m_pData+m_iSize,pData,(size_t)iLen);
        m_iSize += iLen;
    }
    void    SetData(long long * pData,int iLen)
    {
        memcpy(m_pData+m_iSize,pData,(size_t)iLen);
        m_iSize += iLen;
    }
    void    SetData(short int * pData,int iLen)
    {
        memcpy(m_pData+m_iSize,pData,(size_t)iLen);
        m_iSize += iLen;
    }
    void    SetDataAnsCode(short int * pData,size_t iLen)
    {
        memcpy(m_pData+m_iSize,pData,iLen);
        iAnsCodePos = (unsigned short int)m_iSize;
        m_iSize += (int)iLen;
    }
    void    SetData(short int *pData,int iPos,size_t iLen){memcpy(m_pData+iPos,pData,(size_t)iLen);};
    void    SetData(char *pData,int iPos,size_t iLen){memcpy(m_pData+iPos,pData,(size_t)iLen);};
    void    SetData(int *pData,int iPos,int iLen){memcpy(m_pData+iPos,pData,(size_t)iLen);};
    void    SetData(int *pData,int iPos,size_t iLen){memcpy(m_pData+iPos,pData,(size_t)iLen);};
    void    SetSize();

    void    GetData(char* pData,int iPos,int iLen){memcpy(pData,m_pData+iPos,(size_t)iLen);};
    void    GetData(int * pData,int iPos,int iLen){memcpy(pData,m_pData+iPos,(size_t)iLen);};
    void    GetData(long long * pData,int iPos,int iLen){memcpy(pData,m_pData+iPos,(size_t)iLen);};
    void    GetData(short int * pData,size_t iPos,int iLen){memcpy(pData,m_pData+iPos,(size_t)iLen);};
    void    GetData(unsigned short int * pData,unsigned short int iPos,int iLen){memcpy(pData,m_pData+iPos,(size_t)iLen);};
    
    void    InitDataSrc(char *pSrc){m_pData = pSrc;};
    void    SerializeHead(char* pSrc,int iCmdCode,unsigned int SessionId,unsigned int iSequence)
    {
        m_pData = pSrc;
        this->iCmdCode = (unsigned short int)iCmdCode;
        this->isequence = iSequence;
        this->iSessionId = SessionId;
        m_iSize = 16;
    }
    void    ResetParamFlag(bool bFlag){m_bSetParam = bFlag;}
    char*   GetDataPtr(){return m_pData;}; 
    bool    GetSetParamFlag(){return m_bSetParam;}
    int     GetSize(){return m_iSize;}
    void    Parse(){memcpy(&m_iSize,m_pData,16);};
public:
    char*   m_pData;//Դ���ݵ�ַ
    int     m_iSize;
    unsigned short int iCmdCode;
    unsigned short int iAnsCodePos;
    unsigned int iSessionId;
    unsigned int isequence; //����
    bool    m_bSetParam;
    TMdbAvpHead *m_pHead;  //avp head ����
};
/******************************************************************************
* ������	:  TMdbClientQuery
* ������	:  cs��query
* ����		:
*******************************************************************************/
class TMdbClientQuery
{
    friend class TMdbClientField;

public:
    TMdbClientQuery(TMdbClientDatabase *pTMdbClientDatabase,int iSQLLabel);
    virtual ~TMdbClientQuery();
    void 	Close() ; //�ر�SQL��䣬��׼��������һ��sql���
    void 	CloseSQL(); //Close the cursor associated with the SELECT statement
    void 	SetSQL(const char *sSqlStatement,int iPreFetchRows=-1) throw (TMdbException); //����Ҫִ�е�SQL
    void 	SetSQL(const char *sSqlStatement,int iSqlFlag,int iPreFetchRows) throw (TMdbException); //����Ҫִ�е�SQL
    void 	Open(int prefetchRows=0) throw (TMdbException);  //����Ϊ�˼��ݽӿڶ����
    bool 	Next()throw (TMdbException); //�ƶ�����һ����¼
    bool 	Execute(int iExecuteRows=-1) throw (TMdbException); //ִ��SQL
    bool 	TransBegin();//������
    bool 	Commit(); //�����ύ
    bool 	Rollback(); //����ع�
    bool 	Eof();
    int RowsAffected(); //DELETE/UPDATE/INSERT����޸ĵļ�¼��Ŀ,SELECT���ĿǰNext֮��ļ�¼��
    int FieldCount();	 //��ȡ�и���
    TMdbClientField& Field(int iIndex) throw (TMdbException);		//����������ȡ��i����ʵ��,��0��ʼ
    TMdbClientField& Field(const char *sFieldName) throw (TMdbException);//����������ȡ��ʵ��
    //add time 20121122
    void GetValue(void *pStruct,int* Column)throw (TMdbException);//ֱ�ӻ�ȡֵ
    //���ò���ֵ
    bool IsParamExist(const char *paramName);
    //��ֱ���÷�һ��
    void SetParameter(const char *sParamName,const char* sParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *paramName, const char paramValue, bool isOutput = false) throw (TMdbException); 
    void SetParameter(const char *sParamName,int iParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,long lParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,double dParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,long long llParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,const char* sParamValue,int iBufferLen, bool isOutput_Unused = false) throw (TMdbException);//���ڴ���BLOB/BINARY�����ֶ�
    void SetParameterNULL(const char *sParamName) throw (TMdbException);     //���ò���Ϊ��

    void SetParameter(int iParamIndex,const char* sParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex, const char cParamValue) throw (TMdbException); 
    void SetParameter(int iParamIndex,int iParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,long lParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,double dParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,long long llParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,const char* sParamValue,int iBufferLen) throw (TMdbException);//���ڴ���BLOB/BINARY�����ֶ�
    void SetParameterNULL(int iParamIndex) throw (TMdbException);     //���ò���Ϊ��

    //�����������ֵ
    void SetParamArray(const char *sParamName,char **asParamValue,int iInterval,
                       int iElementSize,int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(const char *sParamName,int *aiParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(const char *sParamName,long *alParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(const char *sParamName,double *adParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(const char *sParamName,long long *allParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetBlobParamArray(const char *sParamName,char *sParamValue,int iBufferLen,
                           int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);//���ڴ���BLOB/BINARY�����ֶ��������

        //�����������ֵ
    void SetParamArray(int iParamIndex,char **asParamValue,int iInterval,
                       int iElementSize,int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(int iParamIndex,int *aiParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(int iParamIndex,long *alParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(int iParamIndex,double *adParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetParamArray(int iParamIndex,long long *allParamValue,int iInterval,
                       int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);
    void SetBlobParamArray(int iParamIndex,char *sParamValue,int iBufferLen,
                           int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);//���ڴ���BLOB/BINARY�����ֶ��������
                           
    //������ݿ����,�д��쳣
    void CheckError(const char* sSql) throw (TMdbException) ;
    long long GetSequenceByName(const char * sName)throw (TMdbException);//��ȡ����ֵ
    int SendParam() throw (TMdbException);//���Ͳ�������
    int ParamCount();//��ȡ�󶨲�������
     const char * GetParamNameByIndex(int iIndex);//����idx��ȡ�󶨲�����
    int GetParamIndexByName(const char * sName);//����name����ȡ�󶨲���index

    void    SetSQLBin(const char *sSqlStatement,int iSqlFlag,int iPreFetchRows) throw (TMdbException); //����Ҫִ�е�SQL
    void    OpenBin(int prefetchRows=0) throw (TMdbException);  //����Ϊ�˼��ݽӿڶ����
    bool    NextBin()throw (TMdbException); //�ƶ�����һ����¼
    int     SendParamBin() throw (TMdbException);//���Ͳ�������
    void    SetParameterBin(int iParamIndex,const char* sParamValue) throw (TMdbException);
    void    SetParameterBin(int iParamIndex, const char cParamValue) throw (TMdbException); 
    void    SetParameterBin(int iParamIndex,int iParamValue) throw (TMdbException);
    void    SetParameterBin(int iParamIndex,long lParamValue) throw (TMdbException);
    void    SetParameterBin(int iParamIndex,double dParamValue) throw (TMdbException);
    void    SetParameterBin(int iParamIndex,long long llParamValue) throw (TMdbException);
    void    SetParameterBin(int iParamIndex,const char* sParamValue,int iBufferLen) throw (TMdbException);//���ڴ���BLOB/BINARY�����ֶ�
    void    SetParameterNULLBin(int iParamIndex) throw (TMdbException);     //���ò���Ϊ��
    long long GetSequenceByNameBin(const char * sName)throw (TMdbException);//��ȡ����ֵ
    int     NewMemory(char * &pData,int iSize);
private:
    int FillFieldValue()throw (TMdbException);//���filed��ֵ
    int FillFieldValueBin()throw (TMdbException);//���filed��ֵ
    bool IsDynamicSQL(const char * sSQL);//�ж��Ƿ��Ƕ�̬SQL
    TMdbAvpItem * m_pCurRowGroup;//��ǰRowGroup ���ڼ�¼��ǰ��row group
private:
    int 					m_iFiledCounts;  //filed����
    int 					m_iSQLType;    //sql����
    int   					m_iRowsAff;        //select ,update delete ��Ӱ�������
    int                     m_iRowsCurNext;//��ǰ���εļ�¼��
    int 					m_iIsNeedNextOper;   //���ص���������,�Ƿ���Ҫnextָ�������ѯ
    int 					m_iSQLLabel;     //sql��ű�ǩ
    int 					m_iNextStep;
    char* 					m_sSQL; //�����ʱ��SQL���
    int 					m_iSQLBuffLen;
    //bin
    char 			        m_iFiledCountsBin;  //filed����
    short int   	        m_iRowsAffBin;
    short int               m_iRowsCurNextBin;        //select ,update delete ��Ӱ�������
    char 					m_iIsNeedNextOperBin;   //���ص���������,�Ƿ���Ҫnextָ�������ѯ
    //char 					m_sMsgHead[SIZE_MSG_AVP_HEAD+1];
    unsigned char 			m_sRecvPackage[65536];
    unsigned char 			m_sSendPackage[65536];
    //AVP������
    TMdbCspParser * 		m_pCspSetSQL; //Set SQLʹ��
    TMdbCspParser * 		m_pCspSetParam;//Set Param������
    TMdbCspParser *             m_pCspSetParamArray;//Set param array ������
    TMdbCspParser * 		m_pCspSetSQLAns;//SetSQL��Ӧ������
    TMdbCspParser * 		m_pCspNext;    //Next���������
    TMdbCspParser * 		m_pCspError;   //������Ϣ������
    TMdbCspParser *  		m_pCspSeqSend; //SEQ���������
    TMdbCspParser *  		m_pCspSeqRecv; //SEQ��Ӧ������
    TMdbCspParserMgr *      m_pCspParserMgr;//������������
    TMdbAvpHead   *			m_pHead; //��ͷ
    TMdbClientDatabase*		m_pMdb;    //���ݿ�ָ��
    std::vector<TMdbClientField *> m_vField;
    bool 					m_bIsDynamic; //�Ƿ�̬sql
    TMdbParam                      m_tParam;
    bool m_fLastRecordFlag;
    bool m_bIsParamArray;//�Ƿ��ǲ�������
    TMdbParamArray * m_pParamArray;//��������    
    bool                                m_bCanOpenAgain;//�Ƿ�����ٴα�open
    bool m_bFirstNext;//

    int m_iSetParamCount;
    int m_iCurPos;
    int m_iUseOcp;// 1 ocp
    NoOcpParse m_tRecvDataParse;
    NoOcpParse m_tSendDataParse;
};

class TMdbClientDatabase
{
    friend class TMdbClientQuery;
public:
    TMdbClientDatabase();
    ~TMdbClientDatabase();
    int 	Disconnect() throw(TMdbException);   //�Ͽ����ݿ�����
    void 	SetServer(const char* pszIP,int pszPort);  //����Զ�����ӵ�IP�Ͷ˿�
    void 	SetLogin(const char *sUser,const char *sPassword,const char *sServerName) throw (TMdbException); //���õ�½��Ϣ
    char*   GetUser();
    char*   GetPWD();
    char*   GetDSN();
    void 	TransBegin();                               																								//��������
    void 	Commit() ;
    void 	Rollback() ;
    bool 	IsNullConnect();
    bool 	Connect(bool bIsAutoCommit=false) throw (TMdbException);                            //�������ݿ�
    bool 	Connect(const char *sUser,const char *sPassword,const char *sServerName,bool bIsAutoCommit=false) throw (TMdbException);       //�������ݿ�
    bool 	IsConnect() ;																																	//�������ݿ��Ƿ���������
    TMdbClientQuery *CreateDBQuery() throw (TMdbException);

    void CheckError(const char* sSql=NULL) throw (TMdbException) ;
    const char* GetProvider();
    int GetSQLFlag();
    void SetTimeout(int iTimeout);//���ó�ʱʱ��
    void RecvPackage(int iCspAppType,unsigned char * sRecvMsg,TMdbAvpHead & tAvpHead,TMdbCspParser * pCspErrorParser)throw (TMdbException);
    void RecvPackage(int iCspAppType,unsigned char * sRecvMsg,TMdbAvpHead & tAvpHead)throw (TMdbException);
    void RecvPackageOnce(int iCspAppType,unsigned char * sRecvMsg,TMdbAvpHead & tAvpHead,TMdbCspParser * pCspErrorParser)throw (TMdbException);
    void RecvPackageOnce(int iCspAppType,unsigned char * sRecvMsg,TMdbAvpHead & tAvpHead)throw (TMdbException);
    void CheckAnsCode(TMdbCspParser * pParser)throw (TMdbException);
    void CheckAnsCode(NoOcpParse &tRecvData,unsigned short int iAnsCodePos)throw (TMdbException);
    void MultCmdBin(const char * sCmd);
    int GetQmdbInfoBin(const char * sCmd,std::string & sAnswer);//��ȡqmdb��Ϣ
    int GetSendSequence();//��ȡ�������
    int GetQmdbInfo(const char * sCmd,std::string & sAnswer);//��ȡqmdb��Ϣ
    int GetUseOcpFlag(){return m_iUseOcp;}
    void SetUseOcpFlag(int iFlag){m_iIsBigEndian=iFlag;}
    void UseOcp();
private:
    //���ӵ������,����Socket
    int 	LinkServer();
	int 	LinkServerWithOtherPort();
    int 	GetIP(char* sIP);
    void    GetPortAndIP();//$QuickMDB_HOME/qmdb_inst_list.ini  ��ȡCS  ip ��port


private:
    bool 	m_bConnectFlag;
    char 	m_sUser[128]; //�û���
    char 	m_sPWD[128];  //����
    char 	m_sDSN[128];
    char 	m_sIP[32];    //��¼IP
    int 	m_iPort;      //�˿�
    char 	m_sLocalIP[32];
    long 	m_iSocketID;
    int 	m_iTimeout;//��ʱʱ��
    int 			m_iSQLFlag; //sql��ǩ
    int    m_iIsBigEndian;// 1 big
    int    m_iUseOcp;// 1 ocp
    unsigned long 		m_lSessionId;  //�ỰID
    unsigned char 		m_sRecvPackage[65536];
    unsigned char 		m_sSendPackage[65536];


    TMdbCspParser * m_pCspLogOnSend;//��¼
    TMdbCspParser * m_pCspLogOnRecv;
    TMdbCspParser * m_pCspTransSend;//����
    TMdbCspParser * m_pCspTransRecv;

    TMdbCspParser * m_pCspErrorRecv;//������Ϣ
    TMdbCspParserMgr * m_pCspParserMgr;
    TMdbAvpHead   *	m_pHead; //��ͷ

    Socket 		  *	m_pSocket;
    int m_iSendSequence;//�������
    TMdbMultiProtector * m_pMultiProtector;//��������
    TMdbClientEngine * m_pMdbClientEngine;//billingNTC ��װ
    NoOcpParse m_tRecvDataParse;
    NoOcpParse m_tSendDataParse;
    
};



////////////////////////////////////////////////////mdbSequance.h///////////////////////////////////////////
class TMemSeq;
class TMdbSequence
{
public:
    TMdbSequence();
    ~TMdbSequence();

    /******************************************************************************
    * ��������	:  SetConfig()
    * ��������	:  ����DSN����������,��λ����������е�����
    * ����		:  pszDSN, ���ݿ�ʵ��������
    * ����		:  pszSeqName, ���е�����
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0��ʧ���򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int SetConfig(const char* pszDSN, const char *pszSeqName);
    int SetConfig(const char* pszDSN, const char *pszSeqName,TMdbConfig *pConfig);



    /******************************************************************************
    * ��������	:  GetNextIntVal()
    * ��������	:  ��ȡ��һ������ֵ
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ������һ������ֵ
    * ����		:  li.shugang
    *******************************************************************************/
    long long GetNextIntVal();

    /******************************************************************************
    * ��������	:  GetStep()
    * ��������	:  ��ȡ����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  ���ز���
    * ����		:  li.shugang
    *******************************************************************************/
    long long GetStep();

	/******************************************************************************
	* ��������	:  SetStep()
	* ��������	:  ���ò���  
	* ����		:  iStep ���� 
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ�� 
	* ����		:  cao.peng
	*******************************************************************************/
	long long SetStep(const int iStep);

	/******************************************************************************
	* ��������	:  SetStartSeq()
	* ��������	:  ���ÿ�ʼ����
	* ����		:  iStartSeq ��ʼ���� 
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ��  
	* ����		:  cao.peng
	*******************************************************************************/
	long long SetStartSeq(const int iStartSeq);

	/******************************************************************************
	* ��������	:  SetEndSeq()
	* ��������	:  ���ý�������
	* ����		:  iEndSeq ��������
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ�� 
	* ����		:  cao.peng
	*******************************************************************************/
	long long SetEndSeq(const int iEndSeq);

	/******************************************************************************
	* ��������	:  SetCurSeq()
	* ��������	:  ���õ�ǰ����ֵ  
	* ����		:  iCurSeq ����ֵ
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ�� 
	* ����		:  cao.peng
	*******************************************************************************/
	long long SetCurSeq(const int iCurSeq);

	/******************************************************************************
	* ��������	:  Clear
	* ��������	:  ��ʼ����ǰ����(����clear��ǰ����ʹ�õ�����)  
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 �ɹ�����0ʧ�� 
	* ����		:  cao.peng
	*******************************************************************************/
	long long Clear();
       long long GetCurrVal();//��ȡ��ǰsequence
private:
    TMemSeq *m_pSeq;
    TMdbConfig *m_pConfig;
};

//sequance
class TMdbSequenceMgr
{
public:

    TMdbSequenceMgr();

    TMdbSequence m_Sequence;
    char m_SequenceName[64];
};




class TMdbKeyValue;
class TMdbNosqlInfo;
class TMdbNosqlQuery
{
public:
    TMdbNosqlQuery(TMdbDatabase *pTMdbDatabase,int iSQLFlag);
    ~TMdbNosqlQuery();

    void SetTable(const char* psTableName) throw (TMdbException);
    
    void SetKey(const char* psKeyName, const char* psKeyValue) throw (TMdbException);
    void SetKey(const char* psKeyName, int iKeyValue) throw (TMdbException);
    void SetKeyNULL(const char* psKeyName) throw (TMdbException);
    
    void SetKey(const char* psKeyName, char cKeyValue) throw (TMdbException);
    void SetKey(const char* psKeyName, double fKeyValue) throw (TMdbException);    
    void SetKey(const char* psKeyName, long lKeyValue) throw (TMdbException);
    void SetKey(const char* psKeyName, long long llKeyValue) throw (TMdbException);
    
    
    void Find() throw (TMdbException);
    bool Next() throw (TMdbException);
    void GetValue(void *pStruct,int* Column) throw (TMdbException);  // �÷�ͬ TMdbQuery::GetValue
    
    void Close()throw (TMdbException);

    
    
private:
    bool IsLinkOK();
    
private:
    TMdbDatabase* m_pMdb;
    TMdbShmDSN * m_pShmDsn;
    TMdbNosqlInfo* m_pNosqlInfo;
    TMdbQuery* m_pQuery;

};

class  TMdbColumnAddr
{
	public:
		TMdbColumnAddr();
		~TMdbColumnAddr();
		inline const void*  operator[](unsigned int i){return   i<MAX_COLUMN_COUNTS?m_ppColumnAddr[i]:NULL;}
		inline int GetDataLen(unsigned int i){return   i<MAX_COLUMN_COUNTS?m_iDataLen[i]:0;}
			
	protected:		
		friend  class TMdbExecuteEngine;
		void*   m_ppColumnAddr[MAX_COLUMN_COUNTS];
		int  	m_iDataLen[MAX_COLUMN_COUNTS];
		char*   m_ppBlob[MAX_COLUMN_COUNTS];//���������blob����
		int     m_iBlobAskLen[MAX_COLUMN_COUNTS];//blob���ݵĶ�̬���볤��
};





#endif //__QUICK_MEMORY_DATABASE_QUERY_H__

