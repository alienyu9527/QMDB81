#ifndef _MDB_SQL_H_
#define _MDB_SQL_H_
#include <vector>
#include <string>
#include "Interface/mdbQuery.h"
#include "Control/mdbScript.h"
#include "Control/mdbDDLExecuteEngine.h"

//namespace QuickMDB{

#define ArraySize(X) (sizeof(X)/sizeof(X[0]))
#define BOOL_TO_Y(VALUE)  ((VALUE)?"Y":"")

#define SWITCH_MODE(_mode,_doDirect,_doClient)\
switch(_mode)\
{\
    case MODE_DIRECT:{_doDirect;}\
        break;\
    case MODE_CLIENT:{_doClient;}\
        break;\
    default:\
        printf("error");\
        break;\
}

#define CHECK_COLUMN_ISPK(pTable,iPos,bPriKey)\
for(int n=0; n<pTable->m_tPriKey.iColumnCounts; ++n)\
{\
    if(iPos == pTable->m_tPriKey.iColumnNo[n])\
    {\
        bPriKey = true;\
        break;\
    }\
}

#define CHECK_COLUMN_ISIDX(pTable,iPos,bIndex)\
for(int n=0; n<pTable->iIndexCounts; ++n)\
{\
    for(int temp=0; temp<MAX_INDEX_COLUMN_COUNTS; ++temp)\
    {\
        if(iPos == pTable->tIndex[n].iColumnNo[temp])\
        {\
            bIndex = true;\
            break;\
        }\
    }\
}

#define NUM_REVISE_MAX(_num,_max)  (_num) = ((_num) > (_max))?(_max):(_num);
#define NUM_REVISE_MIN(_num,_min)  (_num) = ((_num) < (_min))?(_min):(_num);


//属性ID
enum E_RICH_SQL_PROPERTY
{
	PP_EXPLAIN 			= 1,//是否解释SQL
	PP_AUTO_COMMIT 		= 2,//是否自动提交
	PP_SHOW_TIMESTAMP	= 3,//是否显示时间戳
};

//属性结构
struct ST_RICH_SQL_PROPERTY
{
	std::string sName; // name
	std::string sDoc;  //说明
	int iId;
	int iValue;
};

//命令类型
enum E_CMD_TYPE
{
	CMD_EXEC_SQL = 1, // 执行sql
	CMD_HELP     = 2, //help 说明
	CMD_QUIT     = 3, //退出
	CMD_SET_PP   = 4, //设置property
	CMD_NONE     = 5, //不是cmd
	CMD_DESC     = 6, // 描述某个表
	CMD_TABLES   = 7, //所有sql
	CMD_COMMIT   = 8,//事务提交
	CMD_ROLLBACK = 9,//事务提交
	CMD_ENV      = 10,//环境变量
	CMD_EXEC_SQL_FILE = 11, // execute sql file
	CMD_EXEC_DDL =12,
	CMD_EXEC_START = 13,
	CMD_EXEC_DOWN = 14,
	CM_MDB_ERR = 15
};

//mdbSQL模式
enum E_MDBSQL_MODE
{
    MODE_DIRECT = 1,  //直连模式
    MODE_CLIENT = 2 //CS访问
    
};
//关键字
struct ST_KEYWORD
{
	std::string sName;	// name		 
	int 	   iCmdType;	
	std::string sDoc;	//说明	 
    bool       bCmdConfirm;//操作前是否需要确认
};

//sql file 参数行结构
struct ST_PARAM
{
	int 	    iSqlSeq;//sql 序号
	int 	    iIndex;//参数行索引
	int  	    iDataType;//参数列数据类型 DT_Int、DT_Char
	std::string sValue;	// 参数值 ,string存储，暂不区分类型
	std::string sParamLine;	// 参数行信息
};

//sql file SQL结构
struct ST_SQL
{
	int 	    iSqlSeq;//sql 序号
	std::string sSQL;	// SQL 语句
	std::string sLineInfo;	// SQL 行信息
	std::vector<ST_PARAM> vParam;//参数容器
};

struct st_cmdopt
{
    char sUid[32];
    char sPwd[32];
    char sDsn[32];
	char sIP[32];
    char sPort[32];
};


//富客户端
class TMdbRichSQL
{
public:
	TMdbRichSQL();
	~TMdbRichSQL();
	int Init(const bool bIsOffLine = false);//初始化
	int Login(const st_cmdopt tCmdOpt);//登陆
	int Start(const bool bIsOffline=false);//开始
private:
	ST_KEYWORD * FindCmd(const char * sCmd);//搜索合适的cmd
	int ExecOnLineCommand(ST_KEYWORD * pCmd,char * sCmdStr);//执行命令
	int ExecuteSQL(ST_KEYWORD * pCmd,const char * sSQL);//执行SQL
	int ExecuteHelp(ST_KEYWORD * pCmd,const char * sCmdStr);//执行help	
	void GenSplitLine(char* pInOutStr,int iBuffLen,int iAddLen);//生成分割线
	int ShowSelectResult();//select结果
	int ExecuteSetPP(ST_KEYWORD * pCmd,const char * sCmdStr);//执行set property 
	int Desc(ST_KEYWORD * pCmd,const char * sCmdStr);//输出表的描述
	int Tables(ST_KEYWORD * pCmd,const char * sCmdStr);//输出所有表信息
	ST_RICH_SQL_PROPERTY * GetProperty(const char * sName,int iID);//获取property
	int Commit();//commit
	int RollBack();//rollback
	int Env();//环境变量
	bool IsDDLSql(const char *pszSQL); //判断从文件解析的sql是不是ddl语句
	int ExecuteSQLFile(ST_KEYWORD * pCmd,const char * sCmdStr,const bool bIsOnLine=true);//执行SQL FILE
	bool IsComplete(ST_KEYWORD * pCmd,const char * sStr);
	int ExecuteClientSQL(ST_KEYWORD * pCmd,const char * sSQL);//执行CS SQL
	int ShowClientSelectResult();//CS select结果
private:
	int PushSqlParam(const char * sMsg,std::vector<ST_SQL> *pvSQL,std::vector<ST_PARAM> *pvPARAM);//ST_SQL push_back  and ST_PARAM push_back 
	int AdjustVector(std::vector<ST_SQL> *pvSQL,std::vector<ST_PARAM> *pvPARAM);//ajust vector ST_SQL push_back

	int ExeStSql(std::vector<ST_SQL> *pvSQL);//TMdbQuery 执行sql，含参数

	int ExecOffLineCommand(ST_KEYWORD * pCmd,char * sCmdStr);
	int ExecuteStart(ST_KEYWORD * pCmd,const char * sCmdStr);
	int ExecuteDown(ST_KEYWORD * pCmd,const char * sCmdStr);
	int OnLineExecuteDDLSQL(ST_KEYWORD * pCmd,const char * sSQL);
	int OffLineExecuteDDLSQL(const char * sSQL);
	void TrimLeftOrRightSpecialChar(char *pSQL);
	void strncpyEx(char * desc,char * source,int size,char endtag);
	int ShowErr(ST_KEYWORD * pCmd,const char * sCmdStr);
    int SaveOnLineDDL(const char* psDsn, const char* psSql);
    int SaveOffLineDDL(const char* psSql);
    bool ConfirmBeforeExecute(ST_KEYWORD * pCmd,const char *pSQL);//执行删除或者重大的DDL操作时，需要确认操作
private:
	TMdbSqlParser * m_pMdbSqlParser;//new mdb sql parser
	TMdbDDLExecuteEngine * m_pDDLExecuteEngine;//SQL执行引擎
	TMdbDatabase m_tDB; //数据库连接
	TMdbQuery *  m_pQuery;//
	TMdbShmDSN * m_pShmDSN;//shmdsn;
	TMdbConfig *m_pConfig;
	TMdbClientDatabase m_tCSDB;//CS模式链接
	TMdbClientQuery *   m_pCSQuery;//CS模式的查询
	int m_iMode;//模式
	TMdbScript *m_pScript;
	TMdbErrorHelper m_tErrHelper;

	char sUid[32];
    char sPwd[32];
    char sDsn[32];

	char m_sExecStartTime[MAX_NAME_LEN];
	char m_sExecEndTime[MAX_NAME_LEN];
};

//}
#endif
