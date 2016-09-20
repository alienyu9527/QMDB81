/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@                     All rights reserved.
*@Name：	    mdbQuery.cpp
*@Description：mdb对外的访问接口
*@Author:	   jin.shaohua
*@Date：	    2012.05
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

//SetSQL时可以设置SQL属性
#define QUERY_NO_ROLLBACK 0x01   //该Query无需回滚
#define QUERY_NO_ORAFLUSH 0x02  //该Query无需向Oracle刷新
#define QUERY_NO_REDOFLUSH 0x04 //该Query无需生成redo日志
#define QUERY_NO_SHARDFLUSH 0x08 // 该Query无需生成分片备份同步

#ifndef MAX_COLUMN_COUNTS
#define MAX_COLUMN_COUNTS  64
#endif



#define TMdbClientException TMdbException




class NoOcpParse;

/******************************************************************************
* 类名称	:  TMdbException
* 类描述	:  mdb异常类
* 作者		:
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
    char m_sErrMsg[1024];		 //错误信息
    int m_iErrCode; 			 //错误号
    char m_sErrSql[4096];        //SQL
};
/******************************************************************************
* 类名称	:  TMdbNullArrWrap
* 类描述	:  null数组包装类
* 作者		:
*******************************************************************************/
class TMdbNullArrWrap
{
public:
    TMdbNullArrWrap();
    ~TMdbNullArrWrap();
    int Init(bool * bNullArr,int iCount);//初始化
    bool IsNull(int iIndex);//判断是否为NULL
    int Assign(int iCount);//分配内存
    int SetNull(int iIndex);//设置NULL;
private:
   bool * m_bNullArr;
   int m_iAlloc;
};

/******************************************************************************
* 类名称	:  TMdbParamArray
* 类描述	:  参数数组，用于批量绑定
* 作者		:
*******************************************************************************/
class TMdbParamArray
{
public:
    TMdbParamArray();
    ~TMdbParamArray();
    int ReAlloc(int iArraySize,int iType);
public:
    int m_iParamIndex;//参数位置
    int m_iParamType;//参数类型
    int m_iArraySize;//数组大小
    int m_iAllocSize;//分配的空间大小
    long long * m_pllValue;
    char **      m_psValue;
    TMdbNullArrWrap m_tNullArrWrap;//null标识封装
};
/******************************************************************************
* 类名称	:  TMdbParam
* 类描述	:  mdb SQL 绑定变量定义
* 作者		:
*******************************************************************************/
class TMdbParam
{
public:
    TMdbParam();
    ~TMdbParam();
    int Clear();//清理
    int AddParam(const char * sParamName);//添加参数
    const char * GetParamNameByIndex(int iIndex);//根据参数index获取参数名
    int GetParamIndexByName(const char * sParamName);//根据参数名获取参数index
    int GetCount();
    void NewParamPool();
    void InitParamPool();
private:
    std::vector<std::string> m_vParamName;//参数名
    char *m_pParamPool;
    int m_iParamPoolCurPos;
    int m_iParamComparePos;
    int m_iCount;
};

/******************************************************************************
* 类名称	:  TMdbField
* 类描述	:  select的Field的定义
* 作者		:
*******************************************************************************/
class TMdbField
{
public:
    TMdbField();
    ~TMdbField();
    bool   isNULL(); //判断该值是否是null值
    char * GetName();//field的名字
    char*  AsString() throw (TMdbException);
    double AsFloat() throw (TMdbException);
    long long  AsInteger() throw (TMdbException);
    void AsBlobBuffer(unsigned char *buffer, int &iBufferLen) throw (TMdbException);
    void  AsDateTime(int &iYear,int &iMonth,int &iDay,int &iHour,int &iMinute,int &iSecond) throw (TMdbException);//返回日期的各个部分
    char* AsDateTimeString() throw (TMdbException); //YYYYMMDDHHMISS
    char*  AsRealStr();
    long long  AsRealnt();
    void ClearDataBuf();
    int GetDataType();//
    //char  m_sValue[8192];  //存储字符串
    char  m_sValue[32];
    ST_MEM_VALUE * m_pMemValue;      //内存值
};

/******************************************************************************
* 类名称	:  TMdbQuery
* 类描述	:  查询接口
* 作者		:
*******************************************************************************/

class TMdbColumnAddr;
class TMdbQuery
{
public:
    TMdbQuery(TMdbDatabase *pTMdbDatabase,int iSQLFlag);
    ~TMdbQuery();
    void Close() ;//关闭SQL语句，以准备接收下一个sql语句
    void CloseSQL(); //Close the cursor associated with the SELECT statement
    const char* GetSQL();
    void ExecuteDDLSQL()throw (TMdbException);//执行DDL 语句
    int InitSqlBuff(bool & bFirstSet);
	int ParseSQLForRollback(char* sSqlStatement);
    /******************************************************************************
    * 函数名称	:  SetSQL
    * 函数描述	:  设置SQL
    * 输入		:  sSqlStatement - SQL语句，iPreFetchRows - 暂无用处
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  
    *******************************************************************************/
    void SetSQL(const char *sSqlStatement,int iPreFetchRows=-1) throw (TMdbException);
    /******************************************************************************
    * 函数名称	:  SetSQL
    * 函数描述	:  设置SQL
    * 输入		:  sSqlStatement - SQL语句，iPreFetchRows - 暂无用处
    * 输入		:  iFlag - SQL属性QUERY_NO_ROLLBACK,QUERY_NO_ORAFLUSH,QUERY_NO_REPFLUSH,QUERY_NO_PEERFLUSH
    * 输出		:  
    * 返回值	:  
    * 作者		:  
    *******************************************************************************/
    void SetSQL(const char *sSqlStatement,int iFlag,int iPreFetchRows) throw (TMdbException); //设置要执行的SQL

    //void CheckDDLSQL();//校验sql是否为DDL语句
    
    //void SetDDLSQL(const char *sSqlStatement,int iPreFetchRows=-1)throw (TMdbException);
    void SetRollBackSQL(const char *sSqlStatement) throw (TMdbException); //设置要执行的回滚SQL
    void Open(int prefetchRows=0) throw (TMdbException);  //打开SQL SELECT语句返回结果集,iPreFetchRows无实际作用
    bool Next()throw (TMdbException);//移动到下一个记录
    bool Eof();
    bool IsFieldExist(const char *fieldName);
    /******************************************************************************
    * 函数名称	:  Execute
    * 函数描述	:  执行
    * 输入		:  iExecuteRows -用于批量绑定时，填写需要执行的记录数，单条绑定可以不填写
    * 输入		:  
    * 输出		:  
    * 返回值	: 
    * 作者		:  
    *******************************************************************************/
    bool Execute(int iExecuteRows=-1) throw (TMdbException); //执行SQL
    bool TransBegin(); //事务开启
    bool Commit();//事务提交
    bool Rollback();//事务回滚
    int RowsAffected();//DELETE/UPDATE/INSERT语句修改的记录数目,SELECT语句目前Next之后的记录数
    //与列相关的操作
    int FieldCount();	 //获取列个数
    int ParamCount();//绑定参数个数
    TMdbField& Field(int iIndex) throw (TMdbException);		//根据索引获取第i个列实例,从0开始
    TMdbField& Field(const char *sFieldName) throw (TMdbException);//根据列名获取列实例
    void GetValue(void *pStruct,int* Column)throw (TMdbException);//直接获取值
    void GetValue(TMdbColumnAddr* pTColumnAddr)throw (TMdbException);
    
    TMdbField& FieldPos(const char *sFieldName,int &pos,TMdbField** tField) throw (TMdbException);
    //根据参数名设置参数值
    bool IsParamExist(const char *paramName);
    /******************************************************************************
    * 函数名称	:  SetParameter
    * 函数描述	:  单条绑定设定参数值
    * 输入		:  iParamIndex/sParamName -表示待绑定变量的pos或者name
    * 输入		:  *ParamValue - 表示待绑定的值 isOutput_Unused - 暂不用到，无需给值
    * 输出		:  
    * 返回值	:  
    * 作者		:  
    *******************************************************************************/
    void SetParameter(const char *sParamName,const char* sParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName, const char cParamValue, bool isOutput = false) throw (TMdbException);
    void SetParameter(const char *sParamName,int iParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,long lParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,double dParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,long long llParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,const char* sParamValue,int iBufferLen, bool isOutput_Unused = false) throw (TMdbException);//用于传入BLOB/BINARY类型字段
    void SetParameterNULL(const char *sParamName) throw (TMdbException);     //设置参数为空
    void SetParameter(int iParamIndex,const char* sParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex, const char cParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,int iParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,long lParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,double dParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,long long llParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,const char* sParamValue,int iBufferLen) throw (TMdbException);//用于传入BLOB/BINARY类型字段
    void SetParameterNULL(int iParamIndex) throw (TMdbException);     //设置参数为空
    /******************************************************************************
    * 函数名称	:  SetParamArray
    * 函数描述	:  批量绑定变量
    * 输入		:  iParamIndex/sParamName -表示待绑定变量的pos或者name
    * 输入		:  *ParamValue - 表示待绑定的值(数组) iInterval - 每个元素之间的间隔
    * 输入		:  iElementSize - 元素大小(和iInterval设置成一样就可以),iArraySize -整个数组的大小
    * 输入		:  bOutput - 暂不使用，bNullArr-null设定的数组，若不需要可以不填。
    * 返回值	:  
    * 作者		:  
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
                           int iArraySize,bool bOutput=false,bool * bNullArr= NULL) throw (TMdbException);//用于传入BLOB/BINARY类型字段数组参数

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
                           int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);//用于传入BLOB/BINARY类型字段数组参数

    //检查数据库错误,有错报异常
    void CheckError(const char* sSql) throw (TMdbException) ;
    void SetSignal();
    //void SetDataSource(int iSourceId)throw (TMdbException);//设置数据源，用于主备容灾
    ST_MEM_VALUE_LIST *  GetInputMemValue();//获取需要输入的memValue;
    int GetSQLType();//获取sql 类型
    int GetParamIndexByName(const char *sParamName);//获取参数的idx
    const char * GetParamNameByIndex(int iIndex);//根据pos获取绑定参数名
    void SetTimeStamp(long long iTimeStamp);
    long long GetTimeStamp();    
    ST_MEM_VALUE * GetParamByIndex(int iIndex);//通过名字获取参数值
    int FillFieldForCSBin(NoOcpParse &tParseData,bool bFirst);
	void SetCancelPoint(int* pPoint);
private:
    bool ExecuteOne()throw (TMdbException);//单条执行
    bool ExecuteArray(int iExecuteRows)throw (TMdbException);//批量执行
    bool OpenArray()throw(TMdbException);//批量打开
    ST_MEM_VALUE * GetParamByName(const char *sParamName);//通过名字获取参数值

    int FillOutputToField();//填充field值
    void AddError();    //错误统计
    void AddSuccess();//正确统计
    bool IsLinkOK();//检测是否还是链接状态
    int CheckAndSetSql(const char * sSql) throw (TMdbException);//初步检测SQL语句合法性
    void CleanRBUnit();//清理回滚单元
    bool IsCanAccess();//检测访问属性
    int  PrepareParam();// 准备与param相关的工作
    bool IsAllSet();//是否都已经全部设置
    bool CheckAndSetParamType(int iType)throw (TMdbException);//检查并设置paramtype
    bool IsCanRollback();//是否可以回滚
	bool DealTableVersion();
	bool IsNeedToCheckSQLRepeat(bool bFirstSet, int iFlag);
	
public:
    TMdbSqlParser * m_pMdbSqlParser;//new mdb sql parser
private:
    TMdbDatabase  *m_pMdb;    //数据库指针
    TMdbExecuteEngine * m_pExecuteEngine;//SQL执行引擎
    std::vector<TMdbField *> m_vField;//fieldex
    bool m_bSetSQL;      //是否已经初始化
    char* m_pszSQL; //存放临时的SQL语句
    int m_iSQLBuffLen; //当前存放sql内存长度
    int m_iQueryFlag;//query flag

    int  m_iSetParamType;//setparam方式
    bool * m_bSetList;//是否已都设置
    TMdbParamArray * m_pParamArray;//参数数组
    bool m_bFinishSet;//结束set
    bool m_bOpen;//是否已经open
    bool m_bFillField;//是否已经填充field值
    int  m_iOpenArrayPos;//对于open的绑定遍历的pos;
    bool m_fLastRecordFlag;
    int m_iRowsAff;//影响行数
    TMdbDDLExecuteEngine * m_pDDLExecuteEngine;//SQL执行引擎
    std::map<std::string,int> m_mapTableNameToVersion;
    char *m_pParamPool;
    int m_iParamPoolCurPos;
    int m_iParamComparePos;
     //bool m_bIsDDLSQL;//是否是DDL

};

/******************************************************************************
* 类名称	:  TMdbDatabase
* 类描述	:  QMDB的数据库连接类
* 作者		:
*******************************************************************************/
class TMdbDatabase
{
    friend class TMdbQuery;
public:
    TMdbDatabase();
    ~TMdbDatabase();
    void SetLogin(const char *sUser,const char *sPassword,const char *sServerName) throw (TMdbException);       //设置登陆信息
    bool Connect(bool bIsAutoCommit=false) throw (TMdbException);                            //连接数据库
    bool Connect(const char *sUser,const char *sPassword,const char *sServerName,bool bIsAutoCommit=false) throw (TMdbException);       //连接数据库
    bool ConnectAsMgr(const char* sDSN) throw (TMdbException);
    int Disconnect() throw(TMdbException);                               	//断开数据库连接
    void TransBegin();                               																								//开启事务
    void Commit() ;
    void CommitEx() ;
    void Rollback() ;
    void DisableCommit();  //禁用提交
    void EnableCommit();   //启用提交
    bool IsNullConnect();
    bool IsConnect() ;																													//测试数据库是否连接正常
    TMdbQuery *CreateDBQuery() throw (TMdbException);
    TMdbNosqlQuery* CreateNosqlQuery() throw (TMdbException);
    void CheckError(const char* sSql=NULL) throw (TMdbException);
    const char* GetProvider();
    int SetSQL(const char* pszSQL,int iSqlPos) throw (TMdbException);  //设置执行的SQL语句,返回对应的位置
    void SetSQLPos(int iPos) ;        //设置执行的SQL的位置
    int ReleaseSysSQL() throw (TMdbException);
    int RowsAffected();
    TMdbShmDSN * GetShmDsn();
    bool IsLinkOK();
    int GetTransactionState();
    char* GetUser();
    char* GetPWD();
    char* GetDSN();
    int SetCaptureRouter(const char * sRouters);//设置捕获路由,
    int GetCaptureRouter(char * sRouterRet);//获取捕获路由
private:
    int LinkMonitor(); //链接监控进程
private:
    bool m_bConnectFlag;
    char m_sUser[128];
    char m_sPWD[128];
    char m_sDSN[128];

    TMdbConfig *m_pConfig;
    TMdbShmDSN * m_pShmDSN;
    TMdbLocalLink* m_pLocalLink;

    bool  m_bIsNeedCommit;
    static int m_iSQLFlag; //sql标签
    bool m_bAsManager;//作为管理员登陆(无需验证用户名密码)
    int  m_iTransactionState;//事务状态
    int m_iSessionID;

    TMdbLinkCtrl * m_pLinkCtrl;//链接管理
    TMdbMultiProtector * m_pMultiProtector;//并发保护
    
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
* 类名称	:  TMdbClientField
* 类描述	:  CS模式下的Field
* 作者		:
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
    //返回日期的各个部分
    void       AsDateTime(int &iYear,int &iMonth,int &iDay,int &iHour,int &iMinute,int &iSecond) throw (TMdbException);
    char*      AsDateTimeString() throw (TMdbException); //YYYYMMDDHHMISS
    void       ClearDataBuf();
    void       ClearDataBufPlus();
    int        DataType(void);
    const char* GetName();
    void SetUseOcp(bool bUseOcp){m_bUseOcp = bUseOcp;}
private:
    long long 	m_iValue;                //存储长整形
    char*  	m_sValue;
    char*	m_sName;                 //存储列名称
    char m_iDataType;             //数据类型：1-DT_Int, 2-DT_Char,3-DT_VarChar 4-DT_DateStamp 9-DT_Blob
    bool  	m_bIsNULL;                //是否为空
    char m_sNullValue[1];//NULL值
    bool m_bUseOcp;
};
//bin parse
class NoOcpParse
{
public:
    NoOcpParse();
    ~NoOcpParse();
    /******************************************************************************
    * 函数名称	:  SetData
    * 函数描述	:  设置数据
    * 输入		:  bAnsCode表示是否为应答码
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
    char*   m_pData;//源数据地址
    int     m_iSize;
    unsigned short int iCmdCode;
    unsigned short int iAnsCodePos;
    unsigned int iSessionId;
    unsigned int isequence; //序列
    bool    m_bSetParam;
    TMdbAvpHead *m_pHead;  //avp head 解析
};
/******************************************************************************
* 类名称	:  TMdbClientQuery
* 类描述	:  cs的query
* 作者		:
*******************************************************************************/
class TMdbClientQuery
{
    friend class TMdbClientField;

public:
    TMdbClientQuery(TMdbClientDatabase *pTMdbClientDatabase,int iSQLLabel);
    virtual ~TMdbClientQuery();
    void 	Close() ; //关闭SQL语句，以准备接收下一个sql语句
    void 	CloseSQL(); //Close the cursor associated with the SELECT statement
    void 	SetSQL(const char *sSqlStatement,int iPreFetchRows=-1) throw (TMdbException); //设置要执行的SQL
    void 	SetSQL(const char *sSqlStatement,int iSqlFlag,int iPreFetchRows) throw (TMdbException); //设置要执行的SQL
    void 	Open(int prefetchRows=0) throw (TMdbException);  //参数为了兼容接口而添加
    bool 	Next()throw (TMdbException); //移动到下一个记录
    bool 	Execute(int iExecuteRows=-1) throw (TMdbException); //执行SQL
    bool 	TransBegin();//事务开启
    bool 	Commit(); //事务提交
    bool 	Rollback(); //事务回滚
    bool 	Eof();
    int RowsAffected(); //DELETE/UPDATE/INSERT语句修改的记录数目,SELECT语句目前Next之后的记录数
    int FieldCount();	 //获取列个数
    TMdbClientField& Field(int iIndex) throw (TMdbException);		//根据索引获取第i个列实例,从0开始
    TMdbClientField& Field(const char *sFieldName) throw (TMdbException);//根据列名获取列实例
    //add time 20121122
    void GetValue(void *pStruct,int* Column)throw (TMdbException);//直接获取值
    //设置参数值
    bool IsParamExist(const char *paramName);
    //与直连用法一致
    void SetParameter(const char *sParamName,const char* sParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *paramName, const char paramValue, bool isOutput = false) throw (TMdbException); 
    void SetParameter(const char *sParamName,int iParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,long lParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,double dParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,long long llParamValue, bool isOutput_Unused = false) throw (TMdbException);
    void SetParameter(const char *sParamName,const char* sParamValue,int iBufferLen, bool isOutput_Unused = false) throw (TMdbException);//用于传入BLOB/BINARY类型字段
    void SetParameterNULL(const char *sParamName) throw (TMdbException);     //设置参数为空

    void SetParameter(int iParamIndex,const char* sParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex, const char cParamValue) throw (TMdbException); 
    void SetParameter(int iParamIndex,int iParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,long lParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,double dParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,long long llParamValue) throw (TMdbException);
    void SetParameter(int iParamIndex,const char* sParamValue,int iBufferLen) throw (TMdbException);//用于传入BLOB/BINARY类型字段
    void SetParameterNULL(int iParamIndex) throw (TMdbException);     //设置参数为空

    //设置数组参数值
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
                           int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);//用于传入BLOB/BINARY类型字段数组参数

        //设置数组参数值
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
                           int iArraySize,bool bOutput=false,bool * bNullArr = NULL) throw (TMdbException);//用于传入BLOB/BINARY类型字段数组参数
                           
    //检查数据库错误,有错报异常
    void CheckError(const char* sSql) throw (TMdbException) ;
    long long GetSequenceByName(const char * sName)throw (TMdbException);//获取序列值
    int SendParam() throw (TMdbException);//发送参数数据
    int ParamCount();//获取绑定参数个数
     const char * GetParamNameByIndex(int iIndex);//根据idx获取绑定参数名
    int GetParamIndexByName(const char * sName);//根据name来获取绑定参数index

    void    SetSQLBin(const char *sSqlStatement,int iSqlFlag,int iPreFetchRows) throw (TMdbException); //设置要执行的SQL
    void    OpenBin(int prefetchRows=0) throw (TMdbException);  //参数为了兼容接口而添加
    bool    NextBin()throw (TMdbException); //移动到下一个记录
    int     SendParamBin() throw (TMdbException);//发送参数数据
    void    SetParameterBin(int iParamIndex,const char* sParamValue) throw (TMdbException);
    void    SetParameterBin(int iParamIndex, const char cParamValue) throw (TMdbException); 
    void    SetParameterBin(int iParamIndex,int iParamValue) throw (TMdbException);
    void    SetParameterBin(int iParamIndex,long lParamValue) throw (TMdbException);
    void    SetParameterBin(int iParamIndex,double dParamValue) throw (TMdbException);
    void    SetParameterBin(int iParamIndex,long long llParamValue) throw (TMdbException);
    void    SetParameterBin(int iParamIndex,const char* sParamValue,int iBufferLen) throw (TMdbException);//用于传入BLOB/BINARY类型字段
    void    SetParameterNULLBin(int iParamIndex) throw (TMdbException);     //设置参数为空
    long long GetSequenceByNameBin(const char * sName)throw (TMdbException);//获取序列值
    int     NewMemory(char * &pData,int iSize);
private:
    int FillFieldValue()throw (TMdbException);//填充filed的值
    int FillFieldValueBin()throw (TMdbException);//填充filed的值
    bool IsDynamicSQL(const char * sSQL);//判断是否是动态SQL
    TMdbAvpItem * m_pCurRowGroup;//当前RowGroup 用于记录当前的row group
private:
    int 					m_iFiledCounts;  //filed个数
    int 					m_iSQLType;    //sql类型
    int   					m_iRowsAff;        //select ,update delete 等影响的行数
    int                     m_iRowsCurNext;//当前批次的记录数
    int 					m_iIsNeedNextOper;   //返回的数据量大,是否需要next指令继续查询
    int 					m_iSQLLabel;     //sql编号标签
    int 					m_iNextStep;
    char* 					m_sSQL; //存放临时的SQL语句
    int 					m_iSQLBuffLen;
    //bin
    char 			        m_iFiledCountsBin;  //filed个数
    short int   	        m_iRowsAffBin;
    short int               m_iRowsCurNextBin;        //select ,update delete 等影响的行数
    char 					m_iIsNeedNextOperBin;   //返回的数据量大,是否需要next指令继续查询
    //char 					m_sMsgHead[SIZE_MSG_AVP_HEAD+1];
    unsigned char 			m_sRecvPackage[65536];
    unsigned char 			m_sSendPackage[65536];
    //AVP处理类
    TMdbCspParser * 		m_pCspSetSQL; //Set SQL使用
    TMdbCspParser * 		m_pCspSetParam;//Set Param解析器
    TMdbCspParser *             m_pCspSetParamArray;//Set param array 解析器
    TMdbCspParser * 		m_pCspSetSQLAns;//SetSQL响应解析器
    TMdbCspParser * 		m_pCspNext;    //Next请求解析器
    TMdbCspParser * 		m_pCspError;   //错误消息解析器
    TMdbCspParser *  		m_pCspSeqSend; //SEQ请求解析器
    TMdbCspParser *  		m_pCspSeqRecv; //SEQ响应解析器
    TMdbCspParserMgr *      m_pCspParserMgr;//解析器管理类
    TMdbAvpHead   *			m_pHead; //报头
    TMdbClientDatabase*		m_pMdb;    //数据库指针
    std::vector<TMdbClientField *> m_vField;
    bool 					m_bIsDynamic; //是否动态sql
    TMdbParam                      m_tParam;
    bool m_fLastRecordFlag;
    bool m_bIsParamArray;//是否是参数数组
    TMdbParamArray * m_pParamArray;//参数数组    
    bool                                m_bCanOpenAgain;//是否可以再次被open
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
    int 	Disconnect() throw(TMdbException);   //断开数据库连接
    void 	SetServer(const char* pszIP,int pszPort);  //设置远程链接的IP和端口
    void 	SetLogin(const char *sUser,const char *sPassword,const char *sServerName) throw (TMdbException); //设置登陆信息
    char*   GetUser();
    char*   GetPWD();
    char*   GetDSN();
    void 	TransBegin();                               																								//开启事务
    void 	Commit() ;
    void 	Rollback() ;
    bool 	IsNullConnect();
    bool 	Connect(bool bIsAutoCommit=false) throw (TMdbException);                            //连接数据库
    bool 	Connect(const char *sUser,const char *sPassword,const char *sServerName,bool bIsAutoCommit=false) throw (TMdbException);       //连接数据库
    bool 	IsConnect() ;																																	//测试数据库是否连接正常
    TMdbClientQuery *CreateDBQuery() throw (TMdbException);

    void CheckError(const char* sSql=NULL) throw (TMdbException) ;
    const char* GetProvider();
    int GetSQLFlag();
    void SetTimeout(int iTimeout);//设置超时时间
    void RecvPackage(int iCspAppType,unsigned char * sRecvMsg,TMdbAvpHead & tAvpHead,TMdbCspParser * pCspErrorParser)throw (TMdbException);
    void RecvPackage(int iCspAppType,unsigned char * sRecvMsg,TMdbAvpHead & tAvpHead)throw (TMdbException);
    void RecvPackageOnce(int iCspAppType,unsigned char * sRecvMsg,TMdbAvpHead & tAvpHead,TMdbCspParser * pCspErrorParser)throw (TMdbException);
    void RecvPackageOnce(int iCspAppType,unsigned char * sRecvMsg,TMdbAvpHead & tAvpHead)throw (TMdbException);
    void CheckAnsCode(TMdbCspParser * pParser)throw (TMdbException);
    void CheckAnsCode(NoOcpParse &tRecvData,unsigned short int iAnsCodePos)throw (TMdbException);
    void MultCmdBin(const char * sCmd);
    int GetQmdbInfoBin(const char * sCmd,std::string & sAnswer);//获取qmdb信息
    int GetSendSequence();//获取发送序号
    int GetQmdbInfo(const char * sCmd,std::string & sAnswer);//获取qmdb信息
    int GetUseOcpFlag(){return m_iUseOcp;}
    void SetUseOcpFlag(int iFlag){m_iIsBigEndian=iFlag;}
    void UseOcp();
private:
    //链接到服务端,创建Socket
    int 	LinkServer();
	int 	LinkServerWithOtherPort();
    int 	GetIP(char* sIP);
    void    GetPortAndIP();//$QuickMDB_HOME/qmdb_inst_list.ini  获取CS  ip 、port


private:
    bool 	m_bConnectFlag;
    char 	m_sUser[128]; //用户名
    char 	m_sPWD[128];  //密码
    char 	m_sDSN[128];
    char 	m_sIP[32];    //登录IP
    int 	m_iPort;      //端口
    char 	m_sLocalIP[32];
    long 	m_iSocketID;
    int 	m_iTimeout;//超时时间
    int 			m_iSQLFlag; //sql标签
    int    m_iIsBigEndian;// 1 big
    int    m_iUseOcp;// 1 ocp
    unsigned long 		m_lSessionId;  //会话ID
    unsigned char 		m_sRecvPackage[65536];
    unsigned char 		m_sSendPackage[65536];


    TMdbCspParser * m_pCspLogOnSend;//登录
    TMdbCspParser * m_pCspLogOnRecv;
    TMdbCspParser * m_pCspTransSend;//事务
    TMdbCspParser * m_pCspTransRecv;

    TMdbCspParser * m_pCspErrorRecv;//错误消息
    TMdbCspParserMgr * m_pCspParserMgr;
    TMdbAvpHead   *	m_pHead; //报头

    Socket 		  *	m_pSocket;
    int m_iSendSequence;//发送序号
    TMdbMultiProtector * m_pMultiProtector;//并发保护
    TMdbClientEngine * m_pMdbClientEngine;//billingNTC 封装
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
    * 函数名称	:  SetConfig()
    * 函数描述	:  设置DSN和序列名称,定位到具体的序列点上面
    * 输入		:  pszDSN, 数据库实例的名称
    * 输入		:  pszSeqName, 序列的名称
    * 输出		:  无
    * 返回值	:  成功返回0，失败则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int SetConfig(const char* pszDSN, const char *pszSeqName);
    int SetConfig(const char* pszDSN, const char *pszSeqName,TMdbConfig *pConfig);



    /******************************************************************************
    * 函数名称	:  GetNextIntVal()
    * 函数描述	:  获取下一个序列值
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  返回下一个序列值
    * 作者		:  li.shugang
    *******************************************************************************/
    long long GetNextIntVal();

    /******************************************************************************
    * 函数名称	:  GetStep()
    * 函数描述	:  获取步长
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  返回步长
    * 作者		:  li.shugang
    *******************************************************************************/
    long long GetStep();

	/******************************************************************************
	* 函数名称	:  SetStep()
	* 函数描述	:  设置步长  
	* 输入		:  iStep 步长 
	* 输出		:  无
	* 返回值	:  0 成功；非0失败 
	* 作者		:  cao.peng
	*******************************************************************************/
	long long SetStep(const int iStep);

	/******************************************************************************
	* 函数名称	:  SetStartSeq()
	* 函数描述	:  设置开始序列
	* 输入		:  iStartSeq 开始序列 
	* 输出		:  无
	* 返回值	:  0 成功；非0失败  
	* 作者		:  cao.peng
	*******************************************************************************/
	long long SetStartSeq(const int iStartSeq);

	/******************************************************************************
	* 函数名称	:  SetEndSeq()
	* 函数描述	:  设置结束序列
	* 输入		:  iEndSeq 结束序列
	* 输出		:  无
	* 返回值	:  0 成功；非0失败 
	* 作者		:  cao.peng
	*******************************************************************************/
	long long SetEndSeq(const int iEndSeq);

	/******************************************************************************
	* 函数名称	:  SetCurSeq()
	* 函数描述	:  设置当前序列值  
	* 输入		:  iCurSeq 序列值
	* 输出		:  无
	* 返回值	:  0 成功；非0失败 
	* 作者		:  cao.peng
	*******************************************************************************/
	long long SetCurSeq(const int iCurSeq);

	/******************************************************************************
	* 函数名称	:  Clear
	* 函数描述	:  初始化当前序列(不能clear当前正在使用的序列)  
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 成功；非0失败 
	* 作者		:  cao.peng
	*******************************************************************************/
	long long Clear();
       long long GetCurrVal();//获取当前sequence
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
    void GetValue(void *pStruct,int* Column) throw (TMdbException);  // 用法同 TMdbQuery::GetValue
    
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
		char*   m_ppBlob[MAX_COLUMN_COUNTS];//保存解码后的blob数据
		int     m_iBlobAskLen[MAX_COLUMN_COUNTS];//blob数据的动态申请长度
};





#endif //__QUICK_MEMORY_DATABASE_QUERY_H__

