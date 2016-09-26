#include "Tools/mdbSQL.h"
#include "Helper/mdbSQLParser.h"
#include "Helper/SyntaxTreeAnalyse.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbDateTime.h"
//#include "Helper/mdbSQLHelper.h"

#include <stdlib.h>
#include <stdio.h>

//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;

#ifdef HAVE_READLINE
#include "readline/readline.h"
#include "readline/history.h"
#endif

//namespace QuickMDB{



ST_RICH_SQL_PROPERTY g_Property[] =
{
    {"explain","explain sql parse",PP_EXPLAIN,0},
    {"autocommit","auto commit",PP_AUTO_COMMIT,0},
	{"showtimestamp","show timestamp of records",PP_SHOW_TIMESTAMP,0},
    {"","",-1,-1}
};
ST_KEYWORD g_stKeywords[] =
{
    { "insert", CMD_EXEC_SQL, "Execute sql", false },
    { "delete", CMD_EXEC_SQL, "Execute sql", true },
    { "update", CMD_EXEC_SQL, "Execute sql", false },
    { "select", CMD_EXEC_SQL, "Execute sql", false },
    { "execute", CMD_EXEC_SQL_FILE, "Execute sql file", true },
    { "where",	CMD_NONE, "part of sql", false },
    { "from",	CMD_NONE, "part of sql", false },
    { "into",	CMD_NONE, "part of sql", false },
    { "order",	CMD_NONE, "part of sql", false },
    { "limit",	CMD_NONE, "part of sql", false },
    { "first",	CMD_NONE, "part of sql", false },
    { "sysdate",CMD_NONE, "part of sql", false },
    { "desc",	CMD_NONE, "part of sql", false },
    { "asc",	CMD_NONE, "part of sql", false },
    { "sum",	CMD_NONE, "part of sql", false },
    { "min",	CMD_NONE, "part of sql", false },
    { "max",	CMD_NONE, "part of sql", false },
    { "count",	CMD_NONE, "part of sql", false },
    { "to_date",CMD_NONE, "part of sql", false },
    { "by",		CMD_NONE, "part of sql", false },
    { "in",		CMD_NONE, "part of sql", false },
    { "values",	CMD_NONE, "part of sql", false },
    { "as",		CMD_NONE, "part of sql", false },


    { "help",	CMD_HELP, "Display this text", false },
    { "help;",	CMD_HELP, "Display this text", false },
    { "?",	  	CMD_HELP, "Synonym for `help'", false },

    { "set",	CMD_SET_PP, "Set mdbRichSQL Env.", false },
    { "explain",CMD_SET_PP, "Set property list.", false },
    { "autocommit",CMD_SET_PP, "Set property list.", false },
    { "showtimestamp",CMD_SET_PP, "Set property list.", false },
    { "quit", 	CMD_QUIT, "Quit using mdbRichSQL", false },
    { "quit;", 	CMD_QUIT, "Quit using mdbRichSQL", false },
    { "desc", 	CMD_DESC, "DESC <table>.", false },
    //mjx sql tool add start
    { "show",	CMD_SHOW_INFO, "show sys para info", false},
    //mjx sql tool add end
    { "tables", CMD_TABLES,"Show all tables.", false },
    { "tables;", CMD_TABLES,"Show all tables.", false },

    { "commit", CMD_COMMIT,"Commit SQL.", false },
    { "commit;", CMD_COMMIT,"Commit SQL.", false },
    { "rollback",CMD_ROLLBACK,"rollback SQL.", false},
    { "rollback;",CMD_ROLLBACK,"rollback SQL.", false},
    { "env", 	CMD_ENV,"Environment.", false},
    { "env;", 	CMD_ENV,"Environment.", false},

    { DDL_KEYWORD_CREATE,  CMD_EXEC_DDL,"Execute DDL sql.", false},
    { DDL_KEYWORD_ALTER, 	 CMD_EXEC_DDL,"Execute DDL sql.", true},
    { DDL_KEYWORD_DROP, 	 CMD_EXEC_DDL,"Execute DDL sql.", true},
    { DDL_KEYWORD_TRUNCATE, 	 CMD_EXEC_DDL,"Execute DDL sql.", true},
    { DDL_KEYWORD_USE,     CMD_EXEC_DDL,"Execute DDL sql.", false},
    { DDL_KEYWORD_CONNECT, CMD_EXEC_DDL,"Execute DDL sql.", false},
    { DDL_KEYWORD_LOAD,    CMD_EXEC_DDL,"Execute DDL sql.", false},
    { DDL_KEYWORD_ADD,     CMD_EXEC_DDL,"Execute DDL sql.", false},
    { DDL_KEYWORD_REMOVE,  CMD_EXEC_DDL,"Execute DDL sql.", true},
    { DDL_KEYWORD_RENAME,  CMD_EXEC_DDL,"Execute DDL sql.", true},
    
    { "startup",    CMD_EXEC_START,"Execute DDL sql.", false},
    { "shutdown",   CMD_EXEC_DOWN, "Execute DDL sql.", false},

    { "mdberr",   CM_MDB_ERR, "Execution error code query.", false},
    {"",-1,"", false}
};


std::vector<ST_KEYWORD> g_vKeyword;//关键字


#ifdef HAVE_READLINE
//char *command_generator __P((const char *, int));
char *command_generator (const char *, int);

/******************************************************************************
* 函数名称	:  拷贝字符串
* 函数描述	:  dupstr
* 输入		:
* 输入		:
* 输出		:
* 返回值	:
* 作者		:  jin.shaohua
*******************************************************************************/
char * dupstr (const char * s)
{
    char *r;
    r = (char *)malloc(strlen (s) + 1);
    strcpy (r, s);
    return (r);
}
/******************************************************************************
 * 函数名称  :	command_generator
 * 函数描述  :Generator function for command completion.  STATE lets us
   know whether to start from scratch; without any state
   (i.e. STATE == 0), then we start at the top of the list.
   readline 库的自动补齐回调函数
 * 输入 	 :
 * 输入 	 :
 * 输出 	 :
 * 返回值	 :	0 - 成功!0 -失败
 * 作者 	 :	jin.shaohua
 *******************************************************************************/
char * command_generator (const char *text,int state)
{
    static int list_index, len;
    const char *name;
    /* If this is a new word to complete, initialize now.  This
     includes saving the length of TEXT for efficiency, and
     initializing the index variable to 0. */
    if(!state)
    {
        list_index = 0;
        len = strlen (text);
    }
    /* Return the next name which partially matches from the
     command list. */

    while (list_index < g_vKeyword.size())
    {
        name = g_vKeyword[list_index].sName.c_str();
        list_index++;
        if (TMdbNtcStrFunc::StrNoCaseCmp(name, text, len) == 0)
            return (dupstr(name));
    }
    /* If no names matched, then return NULL. */
    return ((char *)NULL);
}
#else
# define readline(p) local_getline(p,stdin)
# define add_history(X)


/*
** This routine reads a line of text from FILE in, stores
** the text in memory obtained from malloc() and returns a pointer
** to the text.  NULL is returned at end of file, or if malloc()
** fails.
**
** The interface is like "readline" but no command-line editing
** is done.
*/
static char *local_getline(const char *zPrompt, FILE *in)
{
    char *zLine;
    int nLine;
    int n;

    if( zPrompt && *zPrompt )
    {
        printf("%s",zPrompt);
        fflush(stdout);
    }
    nLine = 100;
    zLine = (char *)malloc( static_cast<size_t>(nLine) );
    if( zLine==0 ) return 0;
    n = 0;
    while( 1 )
    {
        if( n+100>nLine )
        {
            nLine = nLine*2 + 100;
            char *sTemp;
            sTemp = zLine;
            zLine = (char *)realloc(sTemp, static_cast<size_t>(nLine));
            //zLine = (char *)realloc(zLine, nLine);
            if( zLine==0 )
            {
                if (sTemp)
                {
                    free(sTemp);
                    sTemp = NULL;
                }
                return 0;
            }

        }
        if( fgets(&zLine[n], nLine - n, in)==0 )
        {
            if( n==0 )
            {
                free(zLine);
				zLine = NULL;
                return 0;
            }
            zLine[n] = 0;
            break;
        }
        while( zLine[n] )
        {
            n++;
        }
        if( n>0 && zLine[n-1]=='\n' )
        {
            n--;
            if( n>0 && zLine[n-1]=='\r' ) n--;
            zLine[n] = 0;
            break;
        }
    }
    zLine = (char *)realloc( zLine, static_cast<size_t>(n+1) );
    return zLine;
}

#endif




/******************************************************************************
* 函数名称	:  TMdbRichSQL
* 函数描述	:  构造
* 输入		:
* 输入		:
* 输出		:
* 返回值	:
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbRichSQL::TMdbRichSQL():
    m_pQuery(NULL),
    m_pShmDSN(NULL),
    m_pCSQuery(NULL),
    m_pScript(NULL)
{
    m_pConfig = NULL;
    m_pMdbSqlParser = NULL;
    m_pDDLExecuteEngine = NULL;
    memset(sUid,0,sizeof(sUid));
    memset(sPwd,0,sizeof(sPwd));
    memset(sDsn,0,sizeof(sDsn));
    memset(m_sExecStartTime,0,sizeof(m_sExecStartTime));
    memset(m_sExecEndTime,0,sizeof(m_sExecEndTime));
}
/******************************************************************************
* 函数名称	:  ~TMdbRichSQL
* 函数描述	:  析构
* 输入		:
* 输入		:
* 输出		:
* 返回值	:
* 作者		:  jin.shaohua
*******************************************************************************/
TMdbRichSQL::~TMdbRichSQL()
{
    SAFE_DELETE(m_pQuery);
    SAFE_DELETE(m_pCSQuery);
    SAFE_DELETE(m_pScript);
    SAFE_DELETE(m_pDDLExecuteEngine);
    SAFE_DELETE(m_pMdbSqlParser);
}

/******************************************************************************
* 函数名称	:  Init
* 函数描述	:  初始化
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::Init(const bool bIsOffLine)
{
    int iRet = 0;
    g_vKeyword.clear();
    int i = 0;
    while(g_stKeywords[i].sName != "")
    {
        g_vKeyword.push_back(g_stKeywords[i]);
        i++;
    }
    
    m_pDDLExecuteEngine = new(std::nothrow )TMdbDDLExecuteEngine();
    CHECK_OBJ(m_pDDLExecuteEngine);
	m_pMdbSqlParser = new(std::nothrow) TMdbSqlParser();
    CHECK_OBJ(m_pMdbSqlParser);
    //离线、在线都可以使用错误码查询工具
    CHECK_RET(m_tErrHelper.InitErrorDescription(),"InitErrorDescription failed.");
    
    //离线模式下直接返回成功
    if(bIsOffLine)
    {
        m_pScript = new (std::nothrow)TMdbScript();
        CHECK_OBJ(m_pScript);
        m_pScript->Init(m_pMdbSqlParser);
        return 0;
    }
    //加入表名
    //连接上共享内存
    m_pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
    m_pShmDSN =  TMdbShmMgr::GetShmDSN(sDsn);
    CHECK_OBJ(m_pShmDSN);
    TMdbDSN *pDsn = m_pShmDSN->GetInfo();
    if(pDsn != NULL)
    {
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {//
            TMdbTable *pTable = &(*itor);
             if(pTable->sTableName[0] != 0)
            {
                ST_KEYWORD stkeyword;
                stkeyword.iCmdType = CMD_NONE;
                stkeyword.sName    = pTable->sTableName;
                stkeyword.sDoc     = "table name";
                g_vKeyword.push_back(stkeyword);
            }
        }
    }
    return 0;
}

/******************************************************************************
* 函数名称	:  Login
* 函数描述	:
* 输入		:  sUser - 用户名,sPwd - 密码 ,sDsn - dsn名
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::Login(const st_cmdopt tCmdOpt)
{
    int iRet = 0;
    SAFESTRCPY(sUid,sizeof(sUid),tCmdOpt.sUid);
    SAFESTRCPY(sPwd,sizeof(sPwd),tCmdOpt.sPwd);
    SAFESTRCPY(sDsn,sizeof(sDsn),tCmdOpt.sDsn);
    printf("Login:%s/%s@%s\n",sUid, sPwd, sDsn);
    if(strlen(tCmdOpt.sIP) == 0)
    {//直连模式
        m_iMode = MODE_DIRECT;
    }
    else
    {
        m_iMode = MODE_CLIENT;
    }
    try
    {
        switch(m_iMode)
        {
            case MODE_DIRECT:
                {
                    m_tDB.SetLogin(sUid,sPwd,sDsn);
                    if(m_tDB.Connect() == false)
                    {
                        CHECK_RET(ERR_DB_LOCAL_CONNECT,"Can't connect to minidb.please check QMDB is running!");
                    }
                    m_pQuery = m_tDB.CreateDBQuery();
                    if(NULL == m_pQuery)
                    {
                        CHECK_RET(ERR_DB_LOCAL_CONNECT,"m_pQuery is NULL");
                    }
                }
                break;
            case MODE_CLIENT:
                {
                    m_tCSDB.SetLogin(sUid,sPwd,sDsn);
                    m_tCSDB.SetServer(tCmdOpt.sIP,atoi(tCmdOpt.sPort));
                    if(m_tCSDB.Connect() == false)
                    {
                        CHECK_RET(ERR_DB_REMOTE_CONNECT,"Can't connect to minidb.please check QMDB is running!");
                    }
                    m_pCSQuery = m_tCSDB.CreateDBQuery();
                    if(NULL == m_pCSQuery)
                    {
                        CHECK_RET(ERR_DB_REMOTE_CONNECT,"m_pCSQuery is NULL");
                    }
                }
                break;
            default:
                break;
        }

    }
    catch(TMdbException &e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Can't connect to minidb.please check QMDB is running!\nERROR_MSG=%s\n", e.GetErrMsg());
        return ERR_DB_LOCAL_CONNECT;
    }

    Init();
    TADD_FUNC("Login(%s/******@%s) : Finish.",sUid,sDsn);
    return iRet;
}
/******************************************************************************
* 函数名称	:  Start
* 函数描述	:  开始工作
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::Start(const bool bIsOffline)
{
    int iRet = 0;
#ifdef HAVE_READLINE
    //初始化readline
    rl_readline_name = "mdbRichSQL";
    rl_completion_entry_function   = command_generator;
#endif
    char * sLine = NULL;
    char sCmdStr[4096] = {0};
    while(1)
    {
        //循环读取
        SAFE_FREE(sLine);
        sLine = readline("SQL>>");
		if(NULL == sLine) 
		{
			printf("readline end. Quit");
			return iRet;
		}
        TrimLeftOrRightSpecialChar(sLine);
        if (sLine && *sLine)
        {
            if(strlen(sLine) >= MAX_CMD_LEN)
            {
                printf("cmd[%s] > MAX_LEN(%d)\n",sLine,MAX_CMD_LEN);
                continue;
            }
            //如果输入的是注释行--，跳过
            if(sLine[0] == '-' && sLine[1] == '-')
            {
                continue;
            }
            SAFESTRCPY(sCmdStr,sizeof(sCmdStr),sLine);
            char sCmd[MAX_CMD_LEN] = {0};//截取第一个命令。
            strncpyEx(sCmd,sCmdStr,static_cast<int>(strlen(sCmdStr)),' ');
            ST_KEYWORD * pCmd = FindCmd(sCmd);
            int iLineCount = 1;
            char sPrompt[8] = {0};
            while(IsComplete(pCmd,sLine) == false)
            {
                iLineCount++;
                SAFE_FREE(sLine);
                sprintf(sPrompt,"  %d ",iLineCount);
                sLine = readline(sPrompt);
				if(NULL == sLine) 
				{
					printf("readline end. Quit");
					return iRet;
				}
                TrimLeftOrRightSpecialChar(sLine);
                sCmdStr[strlen(sCmdStr) + 1] = '\0';//末尾添加一个空格
                sCmdStr[strlen(sCmdStr)] = ' ';
                SAFESTRCPY(sCmdStr+strlen(sCmdStr),static_cast<int>(sizeof(sCmdStr)-strlen(sCmdStr)),sLine);
            }
            add_history(sCmdStr); //添加到历史记录
            if(!bIsOffline)
            {
                iRet = ExecOnLineCommand(pCmd,sCmdStr);
            }
            else
            {
                iRet = ExecOffLineCommand(pCmd,sCmdStr);
            }
            if(CMD_QUIT == iRet){break;}
        }
    }
    SAFE_FREE(sLine);
    printf("Good bye .(^_^)\n");
    return iRet;
}

/******************************************************************************
* 函数名称	:  TrimLeftOrRightSpecialChar
* 函数描述	:  去除字符串两边的空格\t、\r字符
* 输入		:  pSQL 字符串
* 输出		:  无
* 返回值	:  无
* 作者		:  cao.peng
*******************************************************************************/
void TMdbRichSQL::TrimLeftOrRightSpecialChar(char *pSQL)
{
    if(NULL == pSQL){ return;}
    while(pSQL[0] == ' ' 
        || pSQL[0] == '\t'
        || pSQL[0] == '\r'
        || pSQL[strlen(pSQL)-1] == ' '
        || pSQL[strlen(pSQL)-1] == '\t'
        || pSQL[strlen(pSQL)-1] == '\r')
    {
        TMdbNtcStrFunc::Trim(pSQL);
        TMdbNtcStrFunc::Trim(pSQL,'\r');
        TMdbNtcStrFunc::Trim(pSQL,'\t');
    }
}

/******************************************************************************
* 函数名称	:  IsSQLComplete
* 函数描述	:  SQL是否完整(以;结尾)
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
bool TMdbRichSQL::IsComplete(ST_KEYWORD * pCmd,const char * sStr)
{
    if(NULL == pCmd || NULL == sStr)
    {
        return true;
    }
    if(CMD_EXEC_SQL == pCmd->iCmdType || CMD_EXEC_DDL == pCmd->iCmdType)
    {
        char sTemp[4096] = {0};
        SAFESTRCPY(sTemp,sizeof(sTemp),sStr);
        TMdbNtcStrFunc::Trim(sTemp);
        if(';' == sTemp[strlen(sTemp) - 1])
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return true;
    }
    //char sCmdStr[4096] = {0};
}

/******************************************************************************
* 函数名称	:  FindCmd
* 函数描述	:  搜索合适的cmd
* 输入		:  sCmd - cmd的名字
* 输入		:
* 输出		:
* 返回值	:
* 作者		:  jin.shaohua
*******************************************************************************/
ST_KEYWORD * TMdbRichSQL::FindCmd(const char * sCmd)
{
    //搜索适合的cmd来执行
    size_t i = 0;
    ST_KEYWORD * pCmd = NULL;
    for(i = 0; i<g_vKeyword.size(); ++i)
    {
        if(NULL == g_vKeyword[i].sName.c_str())continue;
        if(TMdbNtcStrFunc::StrNoCaseCmp(g_vKeyword[i].sName.c_str(),sCmd) == 0)
        {
            pCmd = &g_vKeyword[i];
        }
    }
    if(NULL == pCmd || CMD_NONE == pCmd->iCmdType )
    {
        //没有找到命令
        printf("Sorry,not find cmd[%s],you can input 'help'.\n",sCmd);
        return NULL;
    }
    else
    {
        return pCmd;
    }
}

/******************************************************************************
* 函数名称	:  ExecOnLineCommand
* 函数描述	:  执行在线命令
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::ExecOnLineCommand(ST_KEYWORD * pCmd,char * sCmdStr)
{
    if(NULL == pCmd)
    {
        return 0;
    }
    TADD_NORMAL("CMD[%s]",sCmdStr);
    float fDiffTime = 0.0;
    if(CMD_EXEC_DDL != pCmd->iCmdType)
    {
        TMdbNtcStrFunc::TrimRight(sCmdStr,';');//去除最右边的;
    }
    struct timeval tTV;
    gettimeofday(&tTV, NULL);
    TMdbDateTime::GetCurrentTimeStr(m_sExecStartTime);
    sprintf(&m_sExecStartTime[14],"%03d",tTV.tv_usec/1000);
    switch(pCmd->iCmdType)
    {
    case CMD_EXEC_SQL:
        SWITCH_MODE(m_iMode,ExecuteSQL(pCmd,sCmdStr),ExecuteClientSQL(pCmd,sCmdStr));
        break;
    case CMD_HELP:
        ExecuteHelp(pCmd,sCmdStr);
        break;
    case CMD_QUIT:
        return CMD_QUIT;
        break;
    case CMD_SET_PP:
        ExecuteSetPP(pCmd,sCmdStr);
        break;
    case CMD_TABLES:
        Tables(pCmd,sCmdStr);
        break;
    case CMD_DESC:
        Desc(pCmd,sCmdStr);
        break;
    case CMD_COMMIT:
        Commit();
        break;
    case CMD_ROLLBACK:
        RollBack();
        break;
    case CMD_ENV:
        Env();
        break;
	//mjx sql tool add start
	case CMD_SHOW_INFO:
		ShowSysParaInfo(pCmd,sCmdStr);
		break;
	//mjx sql tool add end
    case CMD_EXEC_SQL_FILE:
        if(false == ConfirmBeforeExecute(pCmd,sCmdStr))
        {//动态时，才确认
            TADD_WARNING("User Quit Operation!");
            break;
        }
		ExecuteSQLFile(pCmd,sCmdStr);
		break;
    case CMD_EXEC_DDL:
        OnLineExecuteDDLSQL(pCmd,sCmdStr);
		break;
    case CM_MDB_ERR:
        ShowErr(pCmd,sCmdStr);
        break;
    case CMD_NONE:
    default:
        break;
    }
    gettimeofday(&tTV, NULL);
    TMdbDateTime::GetCurrentTimeStr(m_sExecEndTime);
    sprintf(&m_sExecEndTime[14],"%03d",tTV.tv_usec/1000);
    fDiffTime = TMdbDateTime::GetDiffMSecond(m_sExecEndTime,m_sExecStartTime);
    printf("Done in %0.3f seconds.\n",fDiffTime/1000);
    return 0;
}

/******************************************************************************
* 函数名称	:  ExecOffLineCommand
* 函数描述	:  执行离线命令
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbRichSQL::ExecOffLineCommand(ST_KEYWORD * pCmd,char * sCmdStr)
{
    if(NULL == pCmd || NULL == sCmdStr)
    {
        return 0;
    }
    float fDiffTime = 0.0;
    struct timeval tTV;
    gettimeofday(&tTV, NULL);
    TMdbDateTime::GetCurrentTimeStr(m_sExecStartTime);
    sprintf(&m_sExecStartTime[14],"%03d",tTV.tv_usec/1000);
    switch(pCmd->iCmdType)
    {
    case CMD_HELP:
        ExecuteHelp(pCmd,sCmdStr);
        break;
    case CMD_QUIT:
        return CMD_QUIT;
        break;
    case CMD_EXEC_START:
		ExecuteStart(pCmd,sCmdStr);
		break;
    case CMD_EXEC_DOWN:
		ExecuteDown(pCmd,sCmdStr);
		break;
    case CMD_EXEC_DDL:
        OffLineExecuteDDLSQL(sCmdStr);
		break;
    case CMD_EXEC_SQL_FILE:
        ExecuteSQLFile(pCmd,sCmdStr,false);
        break;
    case CMD_EXEC_SQL:
        TADD_ERROR(ERROR_UNKNOWN,"Offline mode does not support the DML statement operation.");
        break;
    case CM_MDB_ERR:
        ShowErr(pCmd,sCmdStr);
        break;
        
    case CMD_NONE:
    default:
        TADD_ERROR(ERROR_UNKNOWN,"Does not support this SQL syntax[%s].", sCmdStr);
        break;
    }
    gettimeofday(&tTV, NULL);
    TMdbDateTime::GetCurrentTimeStr(m_sExecEndTime);
    sprintf(&m_sExecEndTime[14],"%03d",tTV.tv_usec/1000);
    fDiffTime = TMdbDateTime::GetDiffMSecond(m_sExecEndTime,m_sExecStartTime);
    printf("Done in %0.3f seconds.\n",fDiffTime/1000);
    return 0;
}

/******************************************************************************
* 函数名称	:  ExecuteSQL
* 函数描述	:
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::ExecuteSQL(ST_KEYWORD * pCmd,const char * sSQL)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //float fDiffTime = 0;
    char sTemp[4096] = {0};
    SAFESTRCPY(sTemp,sizeof(sTemp),sSQL);
    TMdbNtcStrFunc::ToUpper(sTemp);
    if(TMdbNtcStrFunc::FindString(sTemp,"WHERE",0) == -1 
        && TMdbNtcStrFunc::FindString(sTemp,"INSERT",0) == -1 
        && TMdbNtcStrFunc::FindString(sTemp,"SELECT",0) == -1)
    {
        printf("you should use 'where' condition.\n");
        return 0;
    }
    CHECK_OBJ(pCmd);
    CHECK_OBJ(sSQL);
    CHECK_OBJ((m_pQuery));
    try
    {
        m_pQuery->SetSQL(sSQL);
        if(GetProperty("",PP_EXPLAIN)->iValue != 0)
        {
            //打印解析树
            TSyntaxTreeAnalyse::Print(m_pQuery->m_pMdbSqlParser,100);
        }
        switch(m_pQuery->GetSQLType())
        {
        case TK_SELECT:
        {
            m_pQuery->Open();
            ShowSelectResult();
        }
        break;
        case TK_INSERT:
        case TK_UPDATE:
        {
            m_pQuery->Execute();
            if(GetProperty("",PP_AUTO_COMMIT)->iValue != 0)
            {
                m_pQuery->Commit();
            }
            cout<<"\nRow effect: "<<m_pQuery->RowsAffected()<<" records."<<endl;
        }
        break;
        case TK_DELETE:
        {
            if(false == ConfirmBeforeExecute(pCmd,sSQL))
            {//重大操作需要交互确认
                TADD_WARNING("User Quit Operation!");
                return iRet;
            }
        
            m_pQuery->Execute();
            if(GetProperty("",PP_AUTO_COMMIT)->iValue != 0)
            {
                m_pQuery->Commit();
            }
            cout<<"\nRow effect: "<<m_pQuery->RowsAffected()<<" records."<<endl;
        }
        break;
        default:
            CHECK_RET(-1,"sql_type[%d] error.",m_pQuery->GetSQLType());
            break;
        }
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());
        return ERROR_UNKNOWN;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
        return ERROR_UNKNOWN;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  OnLineExecuteDDLSQL
* 函数描述	:  在线执行DDL命令
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbRichSQL::OnLineExecuteDDLSQL(ST_KEYWORD * pCmd,const char * sSQL)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(sSQL);
    //不支持远程执行DDL语句
    if(MODE_CLIENT == m_iMode)
    {
        TADD_ERROR(ERROR_UNKNOWN,"CS mode supports only the DML statement.");
        return ERR_SQL_INVALID;
    }
    try
    {
        int iRet = 0;
        if(m_tDB.IsConnect() == false)
        {
            CHECK_RET(ERR_DB_NOT_CONNECTED,"DB not connect. Please connect!");
        }
        //在线执行要求用户必须是admin用户才可以执行
        CHECK_RET(m_pConfig->CheckParam(sUid,sPwd,sDsn),"input uid or  password error.");
        if(m_pConfig->GetAccess() != MDB_ADMIN)
        {
            CHECK_RET(ERR_DB_PRIVILEGE_INVALID,\
                "User[%s] does not have permission to execute DDL Sql[%s],he must be administrator.",sUid,sSQL);
        }
        //做一些简单处理和校验
        TADD_DETAIL("sSql=%s", sSQL);
        SaveOnLineDDL(sDsn, sSQL);
        CHECK_RET(m_pMdbSqlParser->SetDB(m_pShmDSN,m_pConfig),\
                "SQL = [%s],Error Info[%s].",sSQL,m_pMdbSqlParser->m_tError.GetErrMsg());
        CHECK_RET(m_pMdbSqlParser->ParseSQL(sSQL),\
                "SQL = [%s],Error Info[%s].",sSQL,m_pMdbSqlParser->m_tError.GetErrMsg());
        CHECK_RET(m_pDDLExecuteEngine->Init(m_pMdbSqlParser),"Init failed.");
        CHECK_RET(m_pDDLExecuteEngine->SetCurOperateUser(sUid),"Set the current operation of the user fails.");
        if(false == ConfirmBeforeExecute(pCmd,sSQL))
        {//重大操作需要交互确认
            TADD_WARNING("User Quit Operation!");
            return iRet;
        }
        
        CHECK_RET(m_pDDLExecuteEngine->Execute(),"Execute failed.");
        IS_LOG(1)
        {
            //打印解析树
            TSyntaxTreeAnalyse::Print(m_pMdbSqlParser,TLOG_FLOW);
        }
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());
        return ERROR_UNKNOWN;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
        return ERROR_UNKNOWN;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  OffLineExecuteDDLSQL
* 函数描述	:  离线执行DDL命令
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbRichSQL::OffLineExecuteDDLSQL(const char * sSQL)
{
    int iRet = 0;
    CHECK_OBJ(sSQL);
    try
    {
        SaveOffLineDDL(sSQL);
        iRet = m_pScript->Execute(sSQL);
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());
        return ERROR_UNKNOWN;
    }
    catch(...)
    {
         TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
        return ERROR_UNKNOWN;
    }
    return iRet;
}

/******************************************************************************
* 函数名称	:  ExecuteStart
* 函数描述	:  离线执行启动QMDB命令
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbRichSQL::ExecuteStart(ST_KEYWORD * pCmd,const char * sCmdStr)
{
    if(NULL == pCmd || NULL == sCmdStr)
    {
        printf("[%s:%d] pCmd or sCmdStr is null.\n",__FILE__,__LINE__);
        return ERR_APP_INVALID_PARAM;
    }
    char sTemp[256] = {0};
    char sStartAgrs[256] = {0};
    char sCreateAgrs[256] = {0};
	char sUserInfo[256] = {0};
    SAFESTRCPY(sTemp,sizeof(sTemp),sCmdStr + strlen(pCmd->sName.c_str()));//去掉头的命令符
    TMdbNtcStrFunc::Trim(sTemp);
    TMdbNtcSplit tSplit;
    tSplit.SplitString(sTemp,'@');
    if(tSplit.GetFieldCount() != 2 )
    {
        //校验是否合法
        printf("format [%s] is error,input format: startup uid/pwd@DSN.\n",sTemp);
        return -1;
    }
    SAFESTRCPY(sUserInfo,sizeof(sUserInfo),tSplit[0]);
    SAFESTRCPY(sDsn,sizeof(sDsn),tSplit[1]);
	
    tSplit.SplitString(sUserInfo,'/');
    if(tSplit.GetFieldCount() != 2 )
    {
        //校验是否合法
        printf("format [%s] is error,input format: startup uid/pwd@DSN.\n",sTemp);
        return -1;
    }
    SAFESTRCPY(sUid,sizeof(sUid),tSplit[0]);
    SAFESTRCPY(sPwd,sizeof(sPwd),tSplit[1]);
    
    sprintf(sStartAgrs,"QuickMDB 'dsn=%s;uid=%s;pwd=%s;oper=create'",sDsn,sUid,sPwd);
    sprintf(sCreateAgrs,"QuickMDB 'dsn=%s;uid=%s;pwd=%s;oper=start'",sDsn,sUid,sPwd);
    
    //首先判断指定的DSN是否已经启动，如果启动则返回
    if(TMdbOS::IsProcExist(sCreateAgrs))
    {
        printf("QuickMDB is already running,please check.\n");
        return -1;
    }
    
    system(sStartAgrs);
    TMdbDateTime::Sleep(10);

    system(sCreateAgrs);
    TMdbDateTime::Sleep(15);
    return 0;
}

/******************************************************************************
* 函数名称	:  ExecuteDwon
* 函数描述	:  离线执行销毁QMDB命令
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbRichSQL::ExecuteDown(ST_KEYWORD * pCmd,const char * sCmdStr)
{
    int iRet = 0;
    if(NULL == pCmd || NULL == sCmdStr)
    {
        printf("[%s:%d] pCmd or sCmdStr is null.\n",__FILE__,__LINE__);
        return ERR_APP_INVALID_PARAM;
    }
    char sTemp[256] = {0};
    char sDestroyAgrs[256] = {0};
	char sUserInfo[256] = {0};
    SAFESTRCPY(sTemp,sizeof(sTemp),sCmdStr + strlen(pCmd->sName.c_str()));
    TMdbNtcStrFunc::Trim(sTemp);
    TMdbNtcSplit tSplit;
    tSplit.SplitString(sTemp,'@');
    if(tSplit.GetFieldCount() != 2 )
    {
        //校验是否合法
        printf("format [%s] is error,input format: shutdown uid/pwd@DSN.\n",sTemp);
        return -1;
    }
    SAFESTRCPY(sUserInfo,sizeof(sUserInfo),tSplit[0]);
    SAFESTRCPY(sDsn,sizeof(sDsn),tSplit[1]);
    tSplit.SplitString(sUserInfo,'/');
    if(tSplit.GetFieldCount() != 2 )
    {
        //校验是否合法
        printf("format [%s] is error,input format: shutdown uid/pwd@DSN.\n",sTemp);
        return -1;
    }
    SAFESTRCPY(sUid,sizeof(sUid),tSplit[0]);
    SAFESTRCPY(sPwd,sizeof(sPwd),tSplit[1]);
    
    sprintf(sDestroyAgrs,"QuickMDB 'dsn=%s;uid=%s;pwd=%s;oper=destroy'",sDsn,sUid,sPwd);
    system(sDestroyAgrs);
    TMdbDateTime::Sleep(8);
    return iRet;
}

void TMdbRichSQL::GenSplitLine(char* pInOutStr,int iBuffLen,int iAddLen)
{
    if(!pInOutStr) return;
	char sTempzhy[MAX_VALUE_LEN] = {0};
    snprintf(sTempzhy,sizeof(sTempzhy),"%s*",pInOutStr);
    snprintf(pInOutStr,static_cast<size_t>(iBuffLen), "%s",sTempzhy) ;
    for(int j=1; j<iAddLen; ++j)
    {
        char sTemTemp[MAX_VALUE_LEN] = {0};
        snprintf(sTemTemp,sizeof(sTemTemp),"%s-",pInOutStr);
        snprintf(pInOutStr,iBuffLen, "%s",sTemTemp) ;
    }
}



/******************************************************************************
* 函数名称	:  ShowSelectResult
* 函数描述	:  查询结果集
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::ShowSelectResult()
{
    int iCounts = 0;
    //float fDiffTime= 0.0;
    int iColumn = m_pQuery->FieldCount();
	bool bShowTimeStamp = GetProperty("",PP_SHOW_TIMESTAMP)->iValue;
    while(m_pQuery->Next())
    {
        if(iCounts == 0)
        {
            printf("\n");
            char sSplitLine[MAX_VALUE_LEN];
            memset(sSplitLine, 0, sizeof(sSplitLine));
            int i = 0;
            for(i=0; i<iColumn; ++i)
            {
                ST_MEM_VALUE * pstMemValue = m_pQuery->Field(i).m_pMemValue;
                int iLen = pstMemValue->iSize > (int)strlen(pstMemValue->sAlias)?pstMemValue->iSize + 2:strlen(pstMemValue->sAlias)+2;
				NUM_REVISE_MAX(iLen,32);  //不能太长,最大32

                if(MemValueHasProperty(pstMemValue,MEM_Int))
                {
                    NUM_REVISE_MIN(iLen,20);  //数值型设定不小于20位长度
                }
                printf("%-*s",iLen,pstMemValue->sAlias);
				GenSplitLine(sSplitLine,sizeof(sSplitLine),iLen);
            }
			
			if (bShowTimeStamp)
			{
				int iLen = MAX_TIME_LEN;
				printf("%-*s",iLen,"<UpdateTime>");				
				GenSplitLine(sSplitLine,sizeof(sSplitLine),iLen);
			}
            printf("\n%s\n", sSplitLine);
        }

        for(int i=0; i<iColumn; ++i)
        {
            ST_MEM_VALUE * pstMemValue = m_pQuery->Field(i).m_pMemValue;
            int iLen = pstMemValue->iSize >(int)strlen(pstMemValue->sAlias)?pstMemValue->iSize + 2:strlen(pstMemValue->sAlias)+2;            
			NUM_REVISE_MAX(iLen,32);  //不能太长,最大32
            
            if(MemValueHasProperty(pstMemValue,MEM_Int))
            {
				NUM_REVISE_MIN(iLen,20);//数值型设定不小于20位长度
            }
            if(m_pQuery->Field(i).isNULL())
            {
                printf("%-*s", iLen,"(nil)");
            }
            else if(MemValueHasProperty(m_pQuery->Field(i).m_pMemValue,MEM_Int))
            {
                printf("%-*lld", iLen, m_pQuery->Field(i).AsInteger());
            }
            else if(MemValueHasAnyProperty(m_pQuery->Field(i).m_pMemValue,MEM_Str|MEM_Date))
            {
                printf("%-*s", iLen, m_pQuery->Field(i).AsString());
            }

        }

		if (bShowTimeStamp)
		{
			long long llTimeStamp = 0;
			llTimeStamp = m_pQuery->GetTimeStamp();
			char sTimeStamp[MAX_TIME_LEN] = {0};
			if(llTimeStamp!=0)
			{
				TMdbDateTime::TimeToString(llTimeStamp,sTimeStamp);
			}
			else
			{
				memcpy(sTimeStamp,"(nil)",6);
			}
			printf("%-*s", MAX_TIME_LEN, sTimeStamp);
		}
		
        printf("\n");
        ++iCounts;
    }
    cout<<"\nTotal "<<iCounts<<" records." <<endl;
    return 0;
}

/******************************************************************************
* 函数名称	:  ExecuteHelp
* 函数描述	:  打印帮助
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::ExecuteHelp(ST_KEYWORD * pCmd,const char * sCmdStr)
{
    size_t i = 0;
    for(i = 0; i<g_vKeyword.size(); ++i)
    {
        if(CMD_NONE != g_vKeyword[i].iCmdType )
        {
            printf("[%-8s]:%s\n",g_vKeyword[i].sName.c_str(),g_vKeyword[i].sDoc.c_str());
        }

    }
    return 0;
}
/******************************************************************************
* 函数名称	:  ExecuteSetPP
* 函数描述	:  属性设置
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::ExecuteSetPP(ST_KEYWORD * pCmd,const char * sCmdStr)
{
    int iRet = 0;
    TADD_FUNC("Start");
    CHECK_OBJ(pCmd);
    CHECK_OBJ(sCmdStr);
    if(MODE_CLIENT == m_iMode)
    {
        TADD_ERROR(ERROR_UNKNOWN,"CS mode supports only the DML statement.");
        return ERR_SQL_INVALID;
    }
    if(CMD_SET_PP != pCmd->iCmdType)
    {
        CHECK_RET(-1,"pCmd->iCmdType[%d] is not CMD_SET_PP",pCmd->iCmdType);
    }
    char sTemp[128] = {0};
    SAFESTRCPY(sTemp,sizeof(sTemp),sCmdStr + strlen(pCmd->sName.c_str()));//去掉头的命令符
    TMdbNtcStrFunc::Trim(sTemp);
    TMdbNtcSplit tSplit;
    tSplit.SplitString(sTemp,'=');
    if(tSplit.GetFieldCount() != 2 )
    {
        //校验是否合法
        CHECK_RET(-1,"[%s] is error.",sTemp);
    }
    char sPPName[128] = {0};
    SAFESTRCPY(sPPName,sizeof(sPPName),tSplit[0]);
    TMdbNtcStrFunc::Trim(sPPName);
    char sPPValue[128] = {0};
    SAFESTRCPY(sPPValue,sizeof(sPPValue),tSplit[1]);
    TMdbNtcStrFunc::Trim(sPPValue);
    if(!TMdbNtcStrFunc::IsDigital(sPPValue))
    {
        CHECK_RET(-1,"[%s] is not num.",sPPValue);
    }
    ST_RICH_SQL_PROPERTY * pProPerty = GetProperty(sPPName,-1);//取属性
    if(NULL == pProPerty)
    {
        CHECK_RET(-1,"Property[%s] not exist.",sPPName);
    }
    pProPerty->iValue = TMdbNtcStrFunc::StrToInt(sPPValue);
    Env();
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* 函数名称	:  GetProperty
* 函数描述	:  获取property
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
ST_RICH_SQL_PROPERTY * TMdbRichSQL::GetProperty(const char * sName,int iID)
{
    if(NULL == sName)
    {
        return NULL;
    }
    int i = 0;
    for(i = 0; i<(int)ArraySize(g_Property); ++i)
    {
        if(iID < 0)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(g_Property[i].sName.c_str(),sName)== 0)
            {
                return &g_Property[i];
            }
        }
        else
        {
            if(iID == g_Property[i].iId)
            {
                return &g_Property[i];
            }
        }
    }
    return NULL;
}

//mjx sql tool add start
int TMdbRichSQL::ShowOneParaInfo(char* sParaName)
{
	int iRet = 0;
	if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"log-level") == 0)
		printf("log-level = %d(default 0 ,the value bigger, the log more detailed)\n",m_pConfig->GetDSN()->iLogLevel);
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"Clean-Time") == 0)
		printf("Clean-Time=%d(minutes,clean interval of deleting db records that have been transfered to qmdb) \n",m_pConfig->GetDSN()->iCleanTime);
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"ora-rep-counts") == 0)
		printf("ora-rep-counts=%d(defalut 1, the number of process to rep update oper to db)\n",m_pConfig->GetDSN()->iOraRepCounts);
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"is-ora-rep") == 0)
		printf("is-ora-rep=%s(oracle rep switch)\n",m_pConfig->GetDSN()->bIsOraRep?"Y":"N");
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"is-shard-backup") == 0)
		printf("is-shard-backup=%s(shard backup switch)\n",m_pConfig->GetDSN()->m_bIsShardBackup?"Y":"N");
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"is-shard-backup") == 0)
		printf("is-single-disaster=%s(single disaster switch)\n",m_pConfig->GetDSN()->m_bSingleDisaster?"Y":"N");
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"is-capture-router") == 0)
		printf("is-capture-router=%s(capture router switch)\n",m_pConfig->GetDSN()->bIsCaptureRouter?"Y":"N");
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"Is-Seq-Cache") == 0)
		printf("Is-Seq-Cache=%d(Sequence cache size)\n",m_pConfig->GetDSN()->m_iSeqCacheSize);
	return iRet;
}

int TMdbRichSQL::ShowOneParaInfo2(char* sParaName)
{
	int iRet = 0;
	
	if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"long-sql-time") == 0)
		printf("long-sql-time=%d(default 3seconds)\n",m_pConfig->GetDSN()->m_iLongSqlTime);
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"buf-size") == 0)
		printf("buf-size=%d(qmdb buffer size default 256MB)\n",m_pConfig->GetDSN()->iLogBuffSize);
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"file-size") == 0)
		printf("file-size=%d(Synchronize files size,default 128MB)\n",m_pConfig->GetDSN()->iLogBuffSize);
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"client-timeout") == 0)
		printf("client-timeout=%d(default 3seconds)\n",m_pConfig->GetDSN()->iClientTimeout);
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"manager-size") == 0)
		printf("manager-size=%d(default 256MB)\n",static_cast<int>(m_pConfig->GetDSN()->iManagerSize));
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"data-size") == 0)
		printf("data-size=%d(default 1024MB)\n",static_cast<int>(m_pConfig->GetDSN()->iDataSize));
	else if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"Is-Use-NTC") == 0)
		printf("is-use-ntc=%s(default Y)\n",m_pConfig->GetDSN()->m_bUseNTC?"Y":"N");
	else 
		TADD_ERROR(ERR_SQL_INVALID,"the show parameter is invalid");
	return iRet;
	
}


int TMdbRichSQL::ShowSysParaInfo(ST_KEYWORD * pCmd,const char * sCmdStr)
{
	TADD_FUNC("Start.");
	int iRet = 0;
	char sParaName[128];
    
    memset(sParaName, 0, sizeof(sParaName));
    SAFESTRCPY(sParaName,sizeof(sParaName),sCmdStr + strlen(pCmd->sName.c_str()));
    TMdbNtcStrFunc::Trim(sParaName);
	//show all
	if(TMdbNtcStrFunc::StrNoCaseCmp(sParaName,"all") == 0)
	{
		printf("log-level = %d(default 0 ,the value bigger, the log more detailed)\n",m_pConfig->GetDSN()->iLogLevel);
		printf("Clean-Time=%d(minutes,clean interval of deleting db records that have been transfered to qmdb) \n",m_pConfig->GetDSN()->iCleanTime);
		printf("ora-rep-counts=%d(defalut 1, the number of process to rep update oper to db)\n",m_pConfig->GetDSN()->iOraRepCounts);
		printf("is-ora-rep=%s(oracle rep switch)\n",m_pConfig->GetDSN()->bIsOraRep?"Y":"N");
		printf("is-shard-backup=%s(shard backup switch)\n",m_pConfig->GetDSN()->m_bIsShardBackup?"Y":"N");
		printf("is-capture-router=%s(capture router switch)\n",m_pConfig->GetDSN()->bIsCaptureRouter?"Y":"N");
		
		printf("is-single-disaster=%s(single disaster switch)\n",m_pConfig->GetDSN()->m_bSingleDisaster?"Y":"N");
		printf("Is-Seq-Cache=%d(Sequence cache size)\n",m_pConfig->GetDSN()->m_iSeqCacheSize);
		printf("long-sql-time=%d(default 3seconds)\n",m_pConfig->GetDSN()->m_iLongSqlTime);
		printf("buf-size=%d(qmdb buffer size default 256MB)\n",m_pConfig->GetDSN()->iLogBuffSize);
		printf("file-size=%d(Synchronize files size,default 128MB)\n",m_pConfig->GetDSN()->iLogBuffSize);
		printf("client-timeout=%d(default 3seconds)\n",m_pConfig->GetDSN()->iClientTimeout);
		printf("manager-size=%d(default 256MB)\n",static_cast<int>(m_pConfig->GetDSN()->iManagerSize));
		printf("data-size=%d(default 1024MB)\n",static_cast<int>(m_pConfig->GetDSN()->iDataSize));
		printf("is-use-ntc=%s(default Y)\n",m_pConfig->GetDSN()->m_bUseNTC?"Y":"N");
			
	}

	else
	{
		iRet = ShowOneParaInfo(sParaName);
		iRet = ShowOneParaInfo2(sParaName);

	}
	return iRet;
}
//mjx sql tool add end
/******************************************************************************
* 函数名称	:  Desc
* 函数描述	:  输出表的描述
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::Desc(ST_KEYWORD * pCmd,const char * sCmdStr)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char sTableName[128];
    char sRepType[MAX_NAME_LEN] = {0};
    memset(sTableName, 0, sizeof(sTableName));
    SAFESTRCPY(sTableName,sizeof(sTableName),sCmdStr + strlen(pCmd->sName.c_str()));
    TMdbNtcStrFunc::Trim(sTableName);
    if(MODE_CLIENT == m_iMode)
    {
        TADD_ERROR(ERROR_UNKNOWN,"CS mode supports only the DML statement.");
        return ERR_SQL_INVALID;
    }
    TMdbDSN *pDsn = m_pShmDSN->GetInfo();
    CHECK_OBJ(pDsn);
    TMdbTable *pTable = m_pShmDSN->GetTableByName(sTableName);
    if(NULL == pTable)
    {
        printf("ERROR-MSG : object [%s] does not exist.\n\n",sTableName);
        return iRet;
    }
    int i = 0;
    cout<<"Name                            "<<"Type        "<<"Len         ";
    cout<<"rep-type         "<<"Primary Key  "<<"Index        ";
    cout<<"Nullable        "<< endl;
    cout<<"------------------------------- "<<"----------- "<<"----------- "<<"---------------- ";
    cout<<"------------ "<<"------------ "<<"------------ "<< endl;
    for(i=0; i<pTable->iColumnCounts; ++i)
    {
        bool bPriKey = false;
        bool bIndex = false;
        bool bNullable = false;
        CHECK_COLUMN_ISPK(pTable,i,bPriKey);
        CHECK_COLUMN_ISIDX(pTable,i, bIndex);
        m_pConfig->GetRepType(pTable->iRepAttr,sRepType,sizeof(sRepType));
        if(pTable->tColumn[i].m_bNullable)
        {
            bNullable = true;
        }
        
        if(pTable->tColumn[i].iDataType == DT_Int)
        {
            printf("%-32s%-12s%-12d%-18s%-13s%-13s%-15s\n",
                   pTable->tColumn[i].sName, "Number", pTable->tColumn[i].iColumnLen,sRepType,
                   BOOL_TO_Y(bPriKey), BOOL_TO_Y(bIndex),BOOL_TO_Y(bNullable));
        }
        else if(pTable->tColumn[i].iDataType == DT_Char)
        {
            printf("%-32s%-12s%-12d%-18s%-13s%-13s%-15s\n",
                   pTable->tColumn[i].sName, "Char", pTable->tColumn[i].iColumnLen-1,sRepType,
                   BOOL_TO_Y(bPriKey), BOOL_TO_Y(bIndex),BOOL_TO_Y(bNullable));
        }
        else if(pTable->tColumn[i].iDataType == DT_VarChar)
        {
            printf("%-32s%-12s%-12d%-18s%-13s%-13s%-15s\n",
                   pTable->tColumn[i].sName, "VarChar", pTable->tColumn[i].iColumnLen-1,sRepType,
                   BOOL_TO_Y(bPriKey), BOOL_TO_Y(bIndex),BOOL_TO_Y(bNullable));
        }
        else if(pTable->tColumn[i].iDataType == DT_DateStamp)
        {
            printf("%-32s%-12s%-12d%-18s%-13s%-13s%-15s\n",
                   pTable->tColumn[i].sName, "DateStamp", pTable->tColumn[i].iColumnLen,sRepType,
                   BOOL_TO_Y(bPriKey), BOOL_TO_Y(bIndex),BOOL_TO_Y(bNullable));
        }
        else if(pTable->tColumn[i].iDataType == DT_Blob)
        {
            printf("%-32s%-12s%-12d%-18s%-13s%-13s%-15s\n",
                   pTable->tColumn[i].sName, "BLOB", pTable->tColumn[i].iColumnLen-1,sRepType,
                   BOOL_TO_Y(bPriKey), BOOL_TO_Y(bIndex),BOOL_TO_Y(bNullable));
        }
    }
    printf("\n");

	//mjx sql tool add start
	for(i=0; i< pTable->iIndexCounts; i++)
		pTable->tIndex[i].Print(pTable);
	//mjx sql tool add end
	
    TADD_FUNC("TMdbSqlPlus::Desc() : Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  Tables
* 函数描述	:  输出所有表信息
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::Tables(ST_KEYWORD * pCmd,const char * sCmdStr)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(MODE_CLIENT == m_iMode)
    {
        TADD_ERROR(ERROR_UNKNOWN,"CS mode supports only the DML statement.");
        return ERR_SQL_INVALID;
    }
    printf("Table-Name\n");
    printf("-----------------------------\n");
    TMdbDSN *pDsn = m_pShmDSN->GetInfo();
    if(pDsn != NULL)
    {
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {//
            TMdbTable *pTable = &(*itor);
             if(pTable->sTableName[0] != 0)
            {
                printf("%s\n", pTable->sTableName);
            }
        }
    }
    printf("\n");
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* 函数名称	:  Commit
* 函数描述	:  提交
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::Commit()
{
    try
    {
        switch(m_iMode)
        {
            case MODE_DIRECT:
                m_tDB.Commit();
                break;
             case MODE_CLIENT:
                m_tCSDB.Commit();
                break;
        }
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());
        return ERROR_UNKNOWN;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
        return ERROR_UNKNOWN;
    }
    printf("\nCommit OK.\n");
    return 0;
}
/******************************************************************************
* 函数名称	:  RollBack
* 函数描述	:  回滚
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::RollBack()
{
    try
    {
        switch(m_iMode)
        {
            case MODE_DIRECT:
                m_tDB.Rollback();
                break;
             case MODE_CLIENT:
                m_tCSDB.Rollback();
                printf("\nRollBack OK\n");
                break;
        }
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());
        return ERROR_UNKNOWN;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
        return ERROR_UNKNOWN;
    }
    
    return 0;

}
/******************************************************************************
* 函数名称	:  Env
* 函数描述	:  环境变量
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::Env()
{
    printf("-----------------------------------------\n");
    int i = 0;
    while(g_Property[i].sName != "")
    {
        printf("[%-10s]=[%d]   #%s\n",g_Property[i].sName.c_str(),g_Property[i].iValue,g_Property[i].sDoc.c_str());
        i++;
    }
    printf("-----------------------------------------\n");
    return 0;

}
/******************************************************************************
* 函数名称	:  ExecuteClientSQL
* 函数描述	:  执行CS SQL
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::ExecuteClientSQL(ST_KEYWORD * pCmd,const char * sSQL)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char sTemp[4096] = {0};
    SAFESTRCPY(sTemp,sizeof(sTemp),sSQL);
    TMdbNtcStrFunc::ToUpper(sTemp);
    if(TMdbNtcStrFunc::FindString(sTemp,"WHERE",0) == -1 &&
            TMdbNtcStrFunc::FindString(sTemp,"INSERT",0) == -1 && TMdbNtcStrFunc::FindString(sTemp,"SELECT",0) == -1)
    {
        printf("you should use 'where' condition.\n");
        return 0;
    }
    CHECK_OBJ(pCmd);
    CHECK_OBJ(sSQL);
    CHECK_OBJ((m_pCSQuery));
    try
    {
        m_pCSQuery->SetSQL(sSQL);
        switch(sSQL[0])
        {
            case 'i':
            case 'I':
            case 'u':
            case 'U'://增删改
                m_pCSQuery->Execute();
                cout<<"\nRow effect: "<<m_pCSQuery->RowsAffected()<<" records."<<endl<<endl;
                break;
            case 'd':
            case 'D':
                if(false == ConfirmBeforeExecute(pCmd,sSQL))
                {//重大操作需要交互确认
                    TADD_WARNING("User Quit Operation!");
                    return iRet;
                }

                m_pCSQuery->Execute();
                cout<<"\nRow effect: "<<m_pCSQuery->RowsAffected()<<" records."<<endl<<endl;
                break;
                
            case 's':
            case 'S'://查询
                m_pCSQuery->Open();
                ShowClientSelectResult();
                break;
            default:
                CHECK_RET(-1,"sql_type[%s] error.",sSQL);
            break;
        }
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());
        return ERROR_UNKNOWN;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");
        return ERROR_UNKNOWN;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  IsDDLSql
* 函数描述	:  判断sql是DDL还是DML语句
* 输入		:  pszSQL sql语句
* 输出		:  
* 返回值	:  true :DDL语句；false 非ddl语句
* 作者		:  cao.peng
*******************************************************************************/
bool TMdbRichSQL::IsDDLSql(const char *pszSQL)
{
    TADD_FUNC("Start.");
    bool bDDLSql = false;
    //截取第一个命令。
    if(NULL == pszSQL)
    {
        return bDDLSql;
    }
    TMdbNtcSplit mdbSplit;
    mdbSplit.SplitString(pszSQL,' ');
    //做简单的判断，通过判断sql语句第一个字符是否为数字
    //因为对于DML脚本，需要每条记录前面都要有序号的，支持静态和动态绑定参数
    if(!isdigit(pszSQL[0]) 
        && TMdbNtcStrFunc::StrNoCaseCmp(mdbSplit[0],"select") != 0
        && TMdbNtcStrFunc::StrNoCaseCmp(mdbSplit[0],"insert") != 0
        && TMdbNtcStrFunc::StrNoCaseCmp(mdbSplit[0],"update") != 0
        && TMdbNtcStrFunc::StrNoCaseCmp(mdbSplit[0],"delete") != 0)
    {
        bDDLSql = true;
    }
    TADD_FUNC("Finish.");
    return bDDLSql;
}

/******************************************************************************
* 函数名称	:  ExecuteSQLFile
* 函数描述	:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  cao.peng
*******************************************************************************/
int TMdbRichSQL::ExecuteSQLFile(ST_KEYWORD * pCmd,const char * sCmdStr,const bool bIsOnLine)
{
    TADD_FUNC("Start.");
	int iRet = 0;
	CHECK_OBJ(pCmd);
    CHECK_OBJ(sCmdStr);
    char sFile[MAX_PATH_NAME_LEN] = {0};
    SAFESTRCPY(sFile,sizeof(sFile),sCmdStr + strlen(pCmd->sName.c_str()));
	TMdbNtcStrFunc::Trim(sFile);
    //不支持CS模式执行文件
    if(MODE_CLIENT == m_iMode)
    {
        TADD_ERROR(ERROR_UNKNOWN,"CS mode supports only the DML statement.");
        return ERR_SQL_INVALID;
    }
    //检查SQL文件是否存在
	bool bExist = TMdbNtcFileOper::IsExist(sFile);
    if(!bExist)
    {
        TADD_ERROR(ERROR_UNKNOWN,"sql file=[%s] not exist.",sFile);
        return ERR_OS_NO_FILE;
    } 
    FILE* fp = fopen(sFile, "r");
	if(fp == NULL)
	{
		TADD_ERROR(ERROR_UNKNOWN,"Can't open file=%s, errno=%d, errmsg=%s.",sFile, errno, strerror(errno));
		return ERR_OS_NO_MEMROY;	
	}
    TADD_NORMAL("SQL file name:%s",sFile);
    char sMsg[MAX_BLOB_LEN] = {0};
    char sSQL[MAX_BLOB_LEN] = {0};
    std::vector<ST_SQL> vSQL;//SQL
    std::vector<ST_PARAM> vPARAM;//parameter
    vPARAM.clear();
    vSQL.clear();
    while(fgets(sMsg, sizeof(sMsg), fp) != NULL)
	{
	    //去除换行符\n \t 等
    	TMdbNtcStrFunc::FormatChar(sMsg);
        TrimLeftOrRightSpecialChar(sMsg);
        //跳过空行以及注释航行
        if (sMsg[0] == '\n' || strlen(sMsg) == 0)
        {
            continue;
        } 
        //掉过注释行
        if(sMsg[0] == '-' && sMsg[1] == '-')
        {
            continue;
        }
        //合并一条完成的sql语句
        int iPos = TMdbNtcStrFunc::FindString(sMsg,";");
        if(iPos < 0)
        {
            snprintf(sSQL+strlen(sSQL),sizeof(sSQL)," %s",sMsg);
            continue;
        }
        else
        {
            snprintf(sSQL+strlen(sSQL),sizeof(sSQL)," %s",sMsg);
        }
        TMdbNtcStrFunc::Trim(sSQL);
        //需要判断是DDL还是DML
        if(IsDDLSql(sSQL))
        {
            if(bIsOnLine)
            {
                OnLineExecuteDDLSQL(NULL,sSQL);//文件只在入口处确认一次，文件内部不确认
            }
            else
            {
                OffLineExecuteDDLSQL(sSQL);
            }
        }
        else
        {
            TMdbNtcStrFunc::Trim(sSQL,';');
            if(bIsOnLine)
            {
                PushSqlParam(sSQL,&vSQL,&vPARAM);
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"Offline mode can not perform DML SQL[%s].",sSQL);
            }
        }
        memset(sSQL,0,sizeof(sSQL));
	}
    SAFE_CLOSE(fp);
    if(sSQL[0]!=0)
    {
        CHECK_RET(ERR_SQL_INVALID,"Invalid SQL statement[%s].",sSQL);
    }
    if(bIsOnLine)
    {
        //适配SQL对应的参数
        AdjustVector(&vSQL,&vPARAM);
        //SQL 处理
        ExeStSql(&vSQL);
    }
    TADD_FUNC("Finish.");
	return iRet;
}

int TMdbRichSQL::PushSqlParam(const char * sMsg,std::vector<ST_SQL> *pvSQL,std::vector<ST_PARAM> *pvPARAM)
{
    int iRet = 0;
    TMdbNtcSplit mdbSplit;
    //sql前不带序号的，认为是静态SQL
    if(!isdigit(sMsg[0]))
    {
        ST_SQL stSQL;
        stSQL.sLineInfo = sMsg;
        stSQL.iSqlSeq = -9999;
        stSQL.sSQL = sMsg;
        stSQL.vParam.clear();
        pvSQL->push_back(stSQL);
        return iRet;
    }
    //动态SQL解析
    int iEqual = TMdbNtcStrFunc::FindString(sMsg,"=");
    mdbSplit.SplitString(sMsg, ",@");
    int iSplitCount = mdbSplit.GetFieldCount();
    if(iEqual > 0 && iSplitCount < 2)// 包含 = ，并且不包含 ",@" 代表是执行sql行
    {
        ST_SQL stSQL;
        stSQL.sLineInfo = sMsg;
        char sSqlSeq[32] = {0};
        strncpy(sSqlSeq,sMsg,iEqual);
        stSQL.iSqlSeq = TMdbNtcStrFunc::StrToInt(sSqlSeq);
        stSQL.sSQL = sMsg + iEqual + 1;
        stSQL.vParam.clear();

        pvSQL->push_back(stSQL);//ST_SQL push_back 
    }
    else if(iSplitCount >= 2)//不含 = ，代表是参数行
    {
        int iSqlSep = -1;//SQL sequence
        iSqlSep = TMdbNtcStrFunc::StrToInt(mdbSplit[0]);
        for(int i=1;i<iSplitCount;i++)
        {
            ST_PARAM stParam;
            stParam.sParamLine = sMsg;
            stParam.iSqlSeq = iSqlSep;
            stParam.iIndex = i-1;
            stParam.sValue = mdbSplit[i];
            //判断值类型 
            if(TMdbNtcStrFunc::IsDigital(mdbSplit[i]))
            {
                stParam.iDataType = DT_Int;
            }
            else
            {
                stParam.iDataType = DT_Char;
            }
            pvPARAM->push_back(stParam);//ST_PARAM push_back 
        }
    }
    else//无效数据
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR Line Info=[%s]\n",sMsg); 
    }
    return iRet;
}
int TMdbRichSQL::AdjustVector(std::vector<ST_SQL> *pvSQL,std::vector<ST_PARAM> *pvPARAM)
{ 
    int iRet = 0;
    for(vector<ST_SQL>::size_type i=0;i != pvSQL->size();++i)
    {
        int iSqlSeq = (*pvSQL)[i].iSqlSeq;
        for(vector<ST_PARAM>::size_type j=0;j != pvPARAM->size();++j)
        {   
            int iParamSqlSeq = (*pvPARAM)[j].iSqlSeq;
            if(iSqlSeq == iParamSqlSeq)//同一个sql
            {
                (*pvSQL)[i].vParam.push_back((*pvPARAM)[j]);
            }
            
        }
    }
    return iRet;
}
int TMdbRichSQL::ExeStSql(std::vector<ST_SQL> *pvSQL)
{
    int iRet = 0;
    int iRowsAff = 0;
    CHECK_OBJ(m_pQuery);
    try
	{	
    	for(vector<ST_SQL>::size_type i=0;i != pvSQL->size();++i)
        {
            int iSqlSeq = (*pvSQL)[i].iSqlSeq;
            TADD_NORMAL("SQL Sequence=[%d].SQL_MSG=[%s]\n",iSqlSeq,(*pvSQL)[i].sSQL.c_str());         
            m_pQuery->Close();
            m_pQuery->SetSQL((*pvSQL)[i].sSQL.c_str());
            std::vector<ST_PARAM> vParam = (*pvSQL)[i].vParam;
            bool bParamExist = false;
            char sParamLineInfo[4096] = {0};
            int iParamSqlSeq = -1;
            for(vector<ST_PARAM>::size_type j=0;j != vParam.size();++j)
            {
                bParamExist = true;
                memset(sParamLineInfo,0,sizeof(sParamLineInfo));
                SAFESTRCPY(sParamLineInfo,sizeof(sParamLineInfo),vParam[j].sParamLine.c_str());
                iParamSqlSeq = vParam[j].iSqlSeq;
                int iDataType = vParam[j].iDataType;
                TADD_DETAIL("Param Index=[%d].DataType=[%s] \n",j,(iDataType==1)?"INTTEGER":"CHAR/VARCHAR/DATE");
                if(iDataType == DT_Int)
                {
                    long long llValue = TMdbNtcStrFunc::StrToInt(vParam[j].sValue.c_str());
                    m_pQuery->SetParameter(vParam[j].iIndex,llValue);
                    TADD_DETAIL("Param[%zd] Value=[%lld] \n",j,llValue);
                }
                else 
                {
                    char sValueTemp[4096] = {0};
                    SAFESTRCPY(sValueTemp,sizeof(sValueTemp),vParam[j].sValue.c_str());
                    TMdbNtcStrFunc::Trim(sValueTemp,'\'');
                    TADD_DETAIL("Param[%zd] Value=[%s] \n",j,sValueTemp);
                    m_pQuery->SetParameter(vParam[j].iIndex,sValueTemp);
                }
            }
            if(bParamExist)
            {
                TADD_DETAIL("SQL Seq = [%d],PARAM Line Info=[%s]\n",iParamSqlSeq,sParamLineInfo); 
            }
            m_pQuery->Execute();
            iRowsAff += m_pQuery->RowsAffected();
            m_pQuery->Commit();
        }
        TADD_NORMAL("Execute OK,RowsAffected=[%d].\n",iRowsAff); 
        printf("Execute OK,RowsAffected=[%d].\n",iRowsAff); 
	}
	catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
    	return ERROR_UNKNOWN;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");   
        return ERROR_UNKNOWN;    
    }  
    return iRet;
}
/******************************************************************************
* 函数名称	:  ShowClientSelectResult
* 函数描述	:  CS select结果
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRichSQL::ShowClientSelectResult()
{
    int iCounts = 0;
    
    int iColumn = m_pCSQuery->FieldCount();
    while(m_pCSQuery->Next())
    {
        iColumn = m_pCSQuery->FieldCount();
        if(iCounts == 0)
        {
            printf("\n");
            char sSplitLine[MAX_VALUE_LEN];
            memset(sSplitLine, 0, sizeof(sSplitLine));
            int i = 0;
            for(i=0; i<iColumn; ++i)
            { 
                int iLen = 32;
                printf("%-*s",iLen,m_pCSQuery->Field(i).GetName());                
				GenSplitLine(sSplitLine,sizeof(sSplitLine),iLen);                
            }
            printf("\n%s\n", sSplitLine);
        }
        for(int i=0; i<iColumn; ++i)
        {
            int iLen = 32;
            if(m_pCSQuery->Field(i).isNULL())
            {
                printf("%-*s", iLen, "(nil)");
            }
            else
            {
                if(MDB_CS_USE_OCP == m_tCSDB.GetUseOcpFlag())
                {
                    printf("%-*s", iLen, m_pCSQuery->Field(i).AsString());
                }
                else
                {
                    if(m_pCSQuery->Field(i).DataType() == DT_Int)
                    {
                        iLen = 20;
                        printf("%-*lld", iLen, m_pCSQuery->Field(i).AsInteger());
                    }
                    else
                    {
                        printf("%-*s", iLen, m_pCSQuery->Field(i).AsString());
                    
                    }
                }
            }
        }
        printf("\n");
        ++iCounts;
    }
    cout<<"\nTotal "<<iCounts<<" records."<<endl<<endl;
    return 0;
}

void TMdbRichSQL::strncpyEx(char * desc,char * source,int size,char endtag)
{
	if(NULL == desc || NULL == source)
	{
		return;
	}
    for (int i=0;i<size && *source ;i++,desc++,source++)
    {
        *desc=*source;
        if(*desc == endtag)
        {
            *desc=0;
            break;
        }
    }
}

int TMdbRichSQL::ShowErr(ST_KEYWORD * pCmd,const char * sCmdStr)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    char sErrCode[MAX_NAME_LEN] = {0};
    SAFESTRCPY(sErrCode,sizeof(sErrCode),sCmdStr + strlen(pCmd->sName.c_str()));
    TMdbNtcStrFunc::Trim(sErrCode);
    //判断错误码是否数字串
    if(!TMdbNtcStrFunc::IsDigital(sErrCode))
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid error code[%s].",sErrCode);
        return ERR_APP_INVALID_PARAM;
    }
    int iErrCode = atoi(sErrCode);
    //查找错误码
    TMdbError *pError = m_tErrHelper.FindErrorByCode(iErrCode);
    if(NULL == pError)
    {
        TADD_ERROR(ERR_APP_INVALID_PARAM,"Error code[%d] not defined.",iErrCode);
        return ERR_APP_INVALID_PARAM;
    }
    //打印错误信息
    printf("%d : %-20s\n",pError->m_iCode,pError->m_sBrief.c_str());
    printf("   *Cause  : %-20s\n",pError->m_sCause.c_str());
    printf("   *Action : %-20s\n",pError->m_sAction.c_str());
    TADD_FUNC("Finish.");
    return iRet;
}

    int TMdbRichSQL::SaveOnLineDDL(const char* psDsn, const char* psSql)
    {
        int iRet = 0;
        CHECK_OBJ(psDsn);
        CHECK_OBJ(psSql);

        char sPath[MAX_PATH_NAME_LEN] = {0};
        char sFile[MAX_FILE_NAME] = {0};

#ifndef WIN32
        if (NULL == getenv("QuickMDB_HOME"))
        {
            return -1;
        }
        char *pszHome = getenv("QuickMDB_HOME");
#else
        char *pszHome = getenv("QuickMDB_HOME");
#endif

        char sConfigHome[MAX_PATH_NAME_LEN] ={0};
        SAFESTRCPY(sConfigHome,sizeof(sConfigHome),pszHome);
        if(sConfigHome[strlen(sConfigHome)-1] != '/')
        {
            sConfigHome[strlen(sConfigHome)] = '/';
        }

        char sDsnUpper[MAX_NAME_LEN] = {0};
        SAFESTRCPY(sDsnUpper, sizeof(sDsnUpper), psDsn);
        TMdbNtcStrFunc::ToUpper(sDsnUpper);
        snprintf(sPath, sizeof(sPath), "%sscript/online/%s", sConfigHome,sDsnUpper);
        if(!TMdbNtcDirOper::IsExist(sPath))
        {
            if(!TMdbNtcDirOper::MakeFullDir(sPath))
            {
                TADD_ERROR(-1, "make path[%s] failed.", sPath);
                return ERR_OS_CREATE_DIR;
            }
        }

        char sCurTime[MAX_TIME_LEN] = {0};
        TMdbDateTime::GetCurrentTimeStr(sCurTime);

        snprintf(sFile, sizeof(sFile), "%s/script_histroy_%s", sPath, sDsnUpper);
        if(TMdbNtcFileOper::IsExist(sFile))
        {
            unsigned long long iFileSize = 0;
            TMdbNtcFileOper::GetFileSize(sFile, iFileSize);
            if(iFileSize > 1024*1024*1024)
            {
                
                char sBakFile[MAX_FILE_NAME] = {0};
                snprintf(sBakFile, sizeof(sBakFile), "%s_bak_%s", sFile, sCurTime);
                TMdbNtcFileOper::Rename(sFile, sBakFile);
            }
        }

        char sContent[MAX_SQL_LEN*2] = {0};
        snprintf(sContent, MAX_SQL_LEN*2, "%s|%s\n", sCurTime,psSql);

        
        bool bRet = TMdbNtcFileOper::AppendContent(sFile, sContent);
        if(!bRet)
        {
            TADD_ERROR(-1, "append content[%s] to file[%s] failed.", sContent,sFile);
            return ERR_OS_WRITE_FILE;
        }

        return iRet;
    }

    int TMdbRichSQL::SaveOffLineDDL(const char* psSql)
    {
        int iRet = 0;
        CHECK_OBJ(psSql);

        char sPath[MAX_PATH_NAME_LEN] = {0};
        char sFile[MAX_FILE_NAME] = {0};

#ifndef WIN32
        if (NULL == getenv("QuickMDB_HOME"))
        {
            return -1;
        }
        char *pszHome = getenv("QuickMDB_HOME");
#else
        char *pszHome = getenv("QuickMDB_HOME");
#endif

        char sConfigHome[MAX_PATH_NAME_LEN] ={0};
        SAFESTRCPY(sConfigHome,sizeof(sConfigHome),pszHome);
        if(sConfigHome[strlen(sConfigHome)-1] != '/')
        {
            sConfigHome[strlen(sConfigHome)] = '/';
        }

        snprintf(sPath, sizeof(sPath), "%sscript/offline", sConfigHome);
        if(!TMdbNtcDirOper::IsExist(sPath))
        {
            if(!TMdbNtcDirOper::MakeFullDir(sPath))
            {
                TADD_ERROR(-1, "make path[%s] failed.", sPath);
                return ERR_OS_CREATE_DIR;
            }
        }

        char sCurTime[MAX_TIME_LEN] = {0};
        TMdbDateTime::GetCurrentTimeStr(sCurTime);

        snprintf(sFile, sizeof(sFile), "%s/script_histroy", sPath);
        if(TMdbNtcFileOper::IsExist(sFile))
        {
            unsigned long long iFileSize = 0;
            TMdbNtcFileOper::GetFileSize(sFile, iFileSize);
            if(iFileSize > 1024*1024*1024)
            {
                
                char sBakFile[MAX_FILE_NAME] = {0};
                snprintf(sBakFile, sizeof(sBakFile), "%s_bak_%s", sFile, sCurTime);
                TMdbNtcFileOper::Rename(sFile, sBakFile);
            }
        }

        char sContent[MAX_SQL_LEN*2] = {0};
        snprintf(sContent, MAX_SQL_LEN*2, "%s|%s\n", sCurTime,psSql);

        
        bool bRet = TMdbNtcFileOper::AppendContent(sFile, sContent);
        if(!bRet)
        {
            TADD_ERROR(-1, "append content[%s] to file[%s] failed.", sContent,sFile);
            return ERR_OS_WRITE_FILE;
        }

        return iRet;
    }

//动态执行删除或者重大的DDL操作时，需要确认操作
/* DDL:drop  remove truncate alter 可能涉及到的重大操作，需要确认*/
/* DML:delete 也需要确认*/
/* 执行sql文件时，只在第一次读取文件时交互*/
bool TMdbRichSQL::ConfirmBeforeExecute(ST_KEYWORD * pCmd,const char *pSQL)
{
    if(!m_pConfig) return true;
    if(false == m_pConfig->GetDSN()->m_bSQLIsAnswer) return true;
    if(NULL == pCmd) return true;
    if(false == pCmd->bCmdConfirm) return true;
    int cInput = 'N';//y,Y:yes;  n,N:no
    int c = ' ';

    printf("\n");
    printf("Are you sure to Execute SQL[%s]?\n",pSQL);
    printf("Please input y for yes,n for no.\n");
    cInput = getchar();
    while (cInput != '\n' && (c=getchar()) != '\n' && c != EOF) {}
    if (cInput == 'y' || cInput == 'Y')
      return true;
    else
      return false;
}

//}




//using namespace QuickMDB;




void Help(int argc, char* argv[])
{
    printf("-------\n"
        " Usage:\n"
        "   %s \n"
        "   %s <UID>/<PWD>@<sDsn> \n"
        "   %s <UID>/<PWD>@<sDsn> <IP>:<Port>\n"
        "   %s [ -H | -h ] \n"
        " Example:\n"
        "   %s \n"
        "   %s ocs/ocs@ocs \n"
        "   %s ocs [if UID=PWD=sDsn] \n"
        " Note:\n"
        "     %s : Start QMDB offline.\n"
        "     <UID> : user id.\n"
        "     <PWD> : user password.\n"
        "     <sDsn>: data source.\n"
        "     <IP>: peer IP.\n"
        "     <Port>: peer port.\n"
        "     -H|-h Print Help.\n"
        "-------\n",argv[0],argv[0],argv[0],argv[0],argv[0],argv[0],argv[0],argv[0]);
}

void HeadInfo(struct st_cmdopt strValue )
{
    system("clear");
    printf("========================================================================================\n");
    printf("* [MODE=%s]  Version : %-60s*\n",0==strValue.sIP[0]?"Direct":"Client",MDB_VERSION);
    printf("* Copyright (c) 2011--2020, ZTEsoft                                                    *\n");
    printf("* Input 'help or ?' for more information.                                              *\n");
#ifdef HAVE_READLINE
    printf("* Support TAB to auto-complete  and up/down to view history.                           *\n");
#else
    printf("* Do not support TAB to auto-complete and up/down to view history  .                   *\n");
#endif
    printf("* If you find any bugs, please e-mail to li.shugang@zte.com.cn                         *\n");
    printf("========================================================================================\n\n");  
}

int CmdParse(int argc, char* argv[],struct st_cmdopt &strValue)
{
    int iPos = 0;
    if((argc != 2 && argc != 3) 
        || TMdbNtcStrFunc::StrNoCaseCmp(argv[1],"-h") == 0)
    {
        Help(argc,argv);
        return ERR_APP_INVALID_PARAM;
    }
    
    for(int i=0; i<(int)strlen(argv[1]); ++i)
    {
        if(argv[1][i] == '/')
        {
            iPos = i+1;
            break;
        }
        strValue.sUid[i] = argv[1][i];
    }
    for(int i=iPos; i<(int)strlen(argv[1]); ++i)
    {
        if(argv[1][i] == '@')
        {
            iPos = i+1;
            break;
        }
        strValue.sPwd[i-iPos] = argv[1][i];
    }
    for(int i=iPos; i<(int)strlen(argv[1]); ++i)
    {
        strValue.sDsn[i-iPos] = argv[1][i];
    }
    
    if(strValue.sUid[0] == 0)
    {
        printf("(%s/******@%s) : Invalid USER_ID.",strValue.sUid,strValue.sDsn);
        return ERR_APP_USER_EMPTY;
    }
    if(strValue.sPwd[0] == 0)
    {
        printf("(%s/%s@%s) : Invalid PASSWORD.",strValue.sUid,strValue.sPwd,strValue.sDsn);
        return ERR_APP_PWD_EMPTY;
    }
    if(strValue.sDsn[0] == 0)
    {
        printf("(%s/******@%s) : Invalid DSN.",strValue.sUid,strValue.sDsn);
        return ERR_APP_DSN_EMPTY;
    }
    
    if(3 == argc)
    {
        TMdbNtcSplit tSplit;
        tSplit.SplitString(argv[2],':');
        if(tSplit.GetFieldCount() != 2 || !TMdbNtcStrFunc::IsDigital(tSplit[1]))
        {
            printf("[%s] :Invalid ip port",tSplit[1]);
            Help(argc,argv);
            return ERR_NET_IP_INVALID;
        }
        SAFESTRCPY(strValue.sIP,sizeof(strValue.sIP),tSplit[0]);
        SAFESTRCPY(strValue.sPort,sizeof(strValue.sPort),tSplit[1]);
    }
    return 0;
}

/******************************************************************************
* 函数名称	:  ChangeProcName
* 函数描述	:  修改进程参数名
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  dongchun
*******************************************************************************/
int ChangeProcName(st_cmdopt &strValue,char* argv[])
{
    int iRet = 0;
    int iLen = strlen(strValue.sPwd);
    char strPwd[256];
    memset(strPwd,0,sizeof(strPwd));
    for(int i = 0;i<iLen;i++)
    {
        strPwd[i] = '*';
    }

    char sTemp[245];
    memset(sTemp,0,sizeof(sTemp));
    if(strValue.sIP[0] ==0)
    {
        sprintf(sTemp,"%s/%s@%s",strValue.sUid,strPwd,strValue.sDsn);
    }
    else
    {
        sprintf(sTemp,"%s/%s@%s %s:%s",strValue.sUid,strPwd,strValue.sDsn,strValue.sIP,strValue.sPort);
    }

    int isTempLen= strlen(sTemp)+1;
    SAFESTRCPY(argv[1],isTempLen,sTemp);
    argv[1][isTempLen] = 0;
    return iRet;
}

int main(int argc, char* argv[])
{
    int iRet = 0;
    TMdbRichSQL tMdbRichSQL;
    if(argc == 1)
    {
        //离线方式启动
        printf("Start QMDB offline.\n");
        TADD_OFFSTART("OFFLINE","mdbSQL",3,false);
        tMdbRichSQL.Init(true);
        tMdbRichSQL.Start(true);
        return 0;
    }
    
    //在线启动
    struct st_cmdopt strValue;
    memset(&strValue,0,sizeof(strValue));
    if(CmdParse(argc,argv,strValue) != 0)
    {
        return 0;
    }

    iRet = ChangeProcName(strValue,argv);
    iRet = TADD_START(strValue.sDsn,"mdbSQL", 0, false,false);
    if (iRet != 0)
    {
        return iRet;
    }
    int iRestlt = tMdbRichSQL.Login(strValue);
    if( iRestlt < 0)
    {
        printf("Login failed.");
        return 0;
    }
    HeadInfo(strValue);
    tMdbRichSQL.Start();
//    TADD_END();
    return iRet;
}


