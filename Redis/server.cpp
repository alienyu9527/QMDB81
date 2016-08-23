/*
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "server.h"

#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <locale.h>
#include <sys/socket.h>
#include "Helper/mdbDateTime.h"
#include "Agent/mdbInfoCollect.h"

//#define zmalloc malloc
//#define zfree free
//#define zrealloc realloc
//检测ret 并回错误包
#define CHECK_RET_SEND_ANS(_ret,_ptClient,...) if((iRet = _ret)!=0)\
{TADD_ERROR(_ret,__VA_ARGS__);\
AppErrorAnswer(_ptClient,iRet,__VA_ARGS__);\
return iRet;}

/*================================= Globals ================================= */

/* Global vars */
struct redisServer server; /* server global state */
TMdbRedisAgentServer gMdbAgent;
/* Our command table.
 *
 * Every entry is composed of the following fields:
 *
 * name: a string representing the command name.
 * function: pointer to the C function implementing the command.
 * arity: number of arguments, it is possible to use -N to say >= N
 * sflags: command flags as string. See below for a table of flags.
 * flags: flags as bitmask. Computed by Redis using the 'sflags' field.
 * get_keys_proc: an optional function to get key arguments from a command.
 *                This is only used when the following three fields are not
 *                enough to specify what arguments are keys.
 * first_key_index: first argument that is a key
 * last_key_index: last argument that is a key
 * key_step: step to get all the keys from first to last argument. For instance
 *           in MSET the step is two since arguments are key,val,key,val,...
 * microseconds: microseconds of total execution time for this command.
 * calls: total number of calls of this command.
 *
 * The flags, microseconds and calls fields are computed by Redis and should
 * always be set to zero.
 *
 * Command flags are expressed using strings where every character represents
 * a flag. Later the populateCommandTable() function will take care of
 * populating the real 'flags' field using this characters.
 *
 * This is the meaning of the flags:
 *
 * w: write command (may modify the key space).
 * r: read command  (will never modify the key space).
 * m: may increase memory usage once called. Don't allow if out of memory.
 * a: admin command, like SAVE or SHUTDOWN.
 * p: Pub/Sub related command.
 * f: force replication of this command, regardless of server.dirty.
 * s: command not allowed in scripts.
 * R: random command. Command is not deterministic, that is, the same command
 *    with the same arguments, with the same key space, may have different
 *    results. For instance SPOP and RANDOMKEY are two random commands.
 * S: Sort command output array if called from script, so that the output
 *    is deterministic.
 * l: Allow command while loading the database.
 * t: Allow command while a slave has stale data but is not allowed to
 *    server this data. Normally no command is accepted in this condition
 *    but just a few.
 * M: Do not automatically propagate the command on MONITOR.
 * k: Perform an implicit ASKING for this command, so the command will be
 *    accepted in cluster mode if the slot is marked as 'importing'.
 * F: Fast command: O(1) or O(log(N)) command that should never delay
 *    its execution as long as the kernel scheduler is giving us time.
 *    Note that commands that may trigger a DEL as a side effect (like SET)
 *    are not fast commands.
 */


/*============================ Utility functions ============================ */



/* This function gets called every time Redis is entering the
 * main loop of the event driven library, that is, before to sleep
 * for ready file descriptors. */
void beforeSleep(struct aeEventLoop *eventLoop) {
    UNUSED(eventLoop);
    /* Handle writes with pending output buffers. */
    handleClientsWithPendingWrites();
}

//add for test 2016.7.13

//end

void initServerConfig(void) {
    server.arch_bits = (sizeof(long) == 8) ? 64 : 32;
    server.port = CONFIG_DEFAULT_SERVER_PORT;
    server.tcp_backlog = CONFIG_DEFAULT_TCP_BACKLOG;
    server.bindaddr_count = 0;
    server.ipfd_count = 0;
    server.tcpkeepalive = 0;
    //server.active_expire_enabled = 1;
    server.client_max_querybuf_len = PROTO_MAX_QUERYBUF_LEN;
    server.maxclients = CONFIG_DEFAULT_MAX_CLIENTS;
    server.next_client_id = 1; /* Client IDs, start from 1 .*/
}


/* Initialize a set of file descriptors to listen to the specified 'port'
 * binding the addresses specified in the Redis server configuration.
 *
 * The listening file descriptors are stored in the integer array 'fds'
 * and their number is set in '*count'.
 *
 * The addresses to bind are specified in the global server.bindaddr array
 * and their number is server.bindaddr_count. If the server configuration
 * contains no specific addresses to bind, this function will try to
 * bind * (all addresses) for both the IPv4 and IPv6 protocols.
 *
 * On success the function returns C_OK.
 *
 * On error the function returns C_ERR. For the function to be on
 * error, at least one of the server.bindaddr addresses was
 * impossible to bind, or no bind addresses were specified in the server
 * configuration but the function is not able to bind * for at least
 * one of the IPv4 or IPv6 protocols. */
int listenToPort(int port, int *fds, int *count) {
    /* Force binding of 0.0.0.0 if no bind address is specified, always
     * entering the loop if j == 0. */
    if (server.bindaddr_count == 0) server.bindaddr[0] = NULL;
    fds[*count] = anetTcpServer(server.neterr,port,NULL,server.tcp_backlog);
    if (fds[*count] != ANET_ERR) 
    {
        anetNonBlock(NULL,fds[*count]);
        (*count)++;
    }
    if (fds[*count] == ANET_ERR) 
    {
        TADD_ERROR(ERR_NET_LISTEN_PORT,
            "Creating Server TCP listening socket %s:%d: %s",
            "*",port, server.neterr);
        return C_ERR;
    }
    return C_OK;
}

void initServer(int iListenPort) {
    int j;

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    //setupSignalHandlers();
    server.pid = getpid();
    server.current_client = NULL;
    //server.clients = listCreate();
    //server.clients_to_close = listCreate();
    server.clients_pending_write = listCreate();
    //server.unblocked_clients = listCreate();
    server.el = aeCreateEventLoop(server.maxclients);
    server.stat_numconnections = 0;
    server.stat_rejected_conn = 0;
    server.port = iListenPort;
    /* Open the TCP listening socket for the user commands. */
    if (server.port != 0 &&
        listenToPort(server.port,server.ipfd,&server.ipfd_count) == C_ERR)
        {
            TADD_ERROR(ERR_NET_LISTEN_PORT,"listenToPort [%d] failed",server.port);  
            exit(1);
        }


    /* Abort if there are no listening sockets at all. */
    if (server.ipfd_count == 0) {
        TADD_ERROR(ERR_NET_LISTEN_PORT, "Configured to not listen anywhere, exiting.");
        exit(1);
    }

    //server.cronloops = 0;
    /* Create an event handler for accepting new connections in TCP and Unix
     * domain sockets. */
    for (j = 0; j < server.ipfd_count; j++) {
        if (aeCreateFileEvent(server.el, server.ipfd[j], AE_READABLE,
            acceptTcpHandler,NULL) == AE_ERR)
            {
                TADD_ERROR(ERR_NET_LISTEN_PORT,
                    "Unrecoverable error creating server.ipfd file event.");
                exit(1);
            }
    }


}

client::client()
{
    m_bRegister = false;
    m_bFirstNext = true;
    recvbuf[0] = 0;
    sendbuf[0] = 0;
}
client::~client()
{
    for(std::vector<stClientQuery *>::size_type i=0; i != m_vClientQuery.size(); ++i)
    {
        SAFE_DELETE(m_vClientQuery[i]);
    }
    m_vClientQuery.clear();
    SAFE_DELETE(m_pDB);
}

//二进制
int TMdbRedisAgentServer::iHeartBeat = 0;
TMdbRedisAgentServer::TMdbRedisAgentServer()
{
   m_pConfig = NULL;
   m_pShmDSN = NULL;
   m_pDsn   = NULL;
}

int TMdbRedisAgentServer::Init(const char* pszDSN, int iAgentPort)
{
    CHECK_OBJ(pszDSN);
    TADD_FUNC("Init(%s) : Start.", pszDSN);
	//TADD_NORMAL("Init(%s) : Start_mjx.", pszDSN);
    int iRet = 0;
	iPort = iAgentPort;
    //构造配置对象
    m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
    //连接上共享内存
    m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN);
    CHECK_OBJ(m_pShmDSN);
    m_pDsn = m_pShmDSN->GetInfo();
    CHECK_RET(m_tProcCtrl.Init(pszDSN),"m_tProcCtrl.Init(%s) failed.",pszDSN);
    CHECK_RET(m_tLinkCtrl.Attach(pszDSN),"m_tLinkCtrl.Init(%s) failed.",pszDSN);
    CHECK_RET(m_tLinkCtrl.ClearRemoteLink(),"m_tLinkCtrl.ClearRemoteLink(%s) failed.",pszDSN);
	CHECK_RET(m_tLinkCtrl.ClearCntNumForPort(iAgentPort),"m_tLinkCtrl.ClearCntNumForPort(%d) failed.",iAgentPort);
    TADD_FUNC("TMdbRedisAgentServer::Init(%s) : Finish.", pszDSN);
    
    iHeartBeat= m_pConfig->GetProAttr()->iHeartBeatWarning / 2 - 1;
    if(iHeartBeat < 1) 
    {
        TADD_NORMAL("iHeartBeatWarning < 3 ",m_pConfig->GetProAttr()->iHeartBeatWarning);
        iHeartBeat = 4;
    }
    return iRet;
}

int TMdbRedisAgentServer::GetConnectAgentPort(TMdbCspParser * pParser)
{
	int iRtnPort = 0;
	if(pParser->GetINT32Value(pParser->m_pRootAvpItem,AVP_CON_NUM) == 0)
	{
		int iValue = m_tLinkCtrl.GetCsAgentPort(iPort);
		if(iValue > 0 && iValue != iPort)
			iRtnPort = iValue;
		else
			iRtnPort = 0;
		TADD_NORMAL("the first connect oper,iRtnPort %d,iVlaue:%d,iPort:%d ",iRtnPort,iValue,iPort);
	}

	return iRtnPort;
}

stClientQuery *  TMdbRedisAgentServer::GetClientQuery(client * ptClient,int iSqlLabel)
{
    stClientQuery * pRetClientQuery = NULL;
    if(iSqlLabel <0 )
    {
        TADD_ERROR(ERROR_UNKNOWN,"invalid SqlLabel[%d] < 0",iSqlLabel);
        return NULL;
    }
    int iSize = (int)ptClient->m_vClientQuery.size();
    if(iSqlLabel < iSize)
    {
        pRetClientQuery = ptClient->m_vClientQuery[iSqlLabel];
        if(iSqlLabel == pRetClientQuery->m_iSqlLabel)
        {
            return pRetClientQuery;
        }
    }
    std::vector<stClientQuery *>::iterator itor = ptClient->m_vClientQuery.begin();
    for(;itor != ptClient->m_vClientQuery.end();++itor)
    {
        if(iSqlLabel == (*itor)->m_iSqlLabel)
        {//找到
            pRetClientQuery = (*itor);
			break;
        }
    }
    if(NULL == pRetClientQuery)
    {//没有则，压入新的
        pRetClientQuery = new (std::nothrow)stClientQuery();
        if(NULL == pRetClientQuery)
        {//
            TADD_ERROR(ERROR_UNKNOWN,"no memory....");
            return NULL;
        }
        pRetClientQuery->m_pQuery = ptClient->m_pDB->CreateDBQuery();
        if(NULL == pRetClientQuery->m_pQuery)
        {
            SAFE_DELETE(pRetClientQuery);
            TADD_ERROR(ERROR_UNKNOWN,"no memory....");
            return NULL;
        }
        pRetClientQuery->m_iSqlLabel = iSqlLabel;
        ptClient->m_vClientQuery.push_back(pRetClientQuery);
        ptClient->m_tTMdbCSLink.m_pRemoteLink->iSQLPos = ptClient->m_vClientQuery.size();
    }
    return pRetClientQuery;
}

int TMdbRedisAgentServer::RecvPackageOnce(client * ptClient)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iHeadLen = SIZE_MSG_AVP_HEAD;
    unsigned char cHead = ptClient->recvbuf[iHeadLen];
    ptClient->recvbuf[iHeadLen] = '\0';
    //解析头部
    ptClient->m_tHead.BinToCnvt((unsigned char*)(ptClient->recvbuf));
    
    if(ptClient->m_iSessionID > 0 && ptClient->m_iSessionID != ptClient->m_tHead.iSessionId)
    {//session id不匹配
        CHECK_RET(ERR_CSP_PARSER_SESSIONID_NOEQUAL,"Peer session id[%d] != cur session id[%d]",
                ptClient->m_tHead.iSessionId,ptClient->m_iSessionID);
    }
    if(false == ptClient->m_bRegister)
    {
        ptClient->m_tTMdbCSLink.m_iUseOcp = ptClient->m_tHead.iVersion;
    }
    ptClient->recvbuf[iHeadLen] = cHead;
    TADD_FUNC("Finish");
    return iRet;
}

int TMdbRedisAgentServer::DoOperation(client *ptClient)
{
    TADD_DETAIL("start...ptClient->m_tHead.iCmdCode=%d",ptClient->m_tHead.iCmdCode);
    int iRet = 0;
    switch(ptClient->m_tHead.iCmdCode)
    {
    case CSP_APP_LOGON:
    {
        CHECK_RET_BREAK(Authentication(ptClient),"Authentication failed.");//认证
        TADD_NORMAL("ThreadID=[%lu],ClientIP=[%s],SequenceID=[%d],SocketID=[%d],Protocol=[%d] Login Success!! ",
                ptClient->m_iThreadId,ptClient->m_tTMdbCSLink.m_sClientIP,ptClient->m_tHead.GetSequence(),
                ptClient->m_tTMdbCSLink.iFD,ptClient->m_tTMdbCSLink.m_iUseOcp);
        break;
    }
    case CSP_APP_SEND_SQL:
    {
        //接受SQL注册
        iRet = AppSendSQL(ptClient);
        if(iRet != 0)
        {
            TADD_ERROR(iRet,"AppSendSQL return Value=[%d]",iRet);
        }
        break;
    }
    case CSP_APP_SEND_PARAM: //0k
    {

        iRet = AppSendParam(ptClient);
        if(iRet != 0)
        {
            TADD_ERROR(iRet,"return Value=[%d]",iRet);
        }
        break;
    }
    case CSP_APP_SEND_PARAM_ARRAY:
    {
        iRet = AppSendParamArray(ptClient);
        if(iRet != 0)
        {
            TADD_ERROR(iRet,"return Value=[%d]",iRet);
        }
        break;
    }
    case CSP_APP_ACTION:     //ok
    {
        iRet = AppTransAction(ptClient);
        if(iRet != 0)
        {
            TADD_ERROR(iRet,"return Value=[%d]",iRet);
        }
        break;
    }
    case CSP_APP_NEXT:          //ok
    {

        iRet = AppNextSQLResult(ptClient);
        if(iRet != 0)
        {
            TADD_ERROR(iRet,"return Value=[%d]",iRet);
        }
        break;

    }
    case CSP_APP_QMDB_INFO://获取qmdb信息
    {
        iRet = AppGetQmdbInfo(ptClient);
        if(iRet != 0)
        {
            TADD_ERROR(iRet,"return Value=[%d]",iRet);
        }
        break;
    }
    /*
    case CSP_APP_GET_SEQUENCE:
    {
        iRet = AppGetSequence(ptClient);
        if(iRet != 0)
        {
            TADD_ERROR(iRet,"return Value=[%d]",iRet);
        }
        break;
    }*/
    default:
    {
        CHECK_RET_SEND_ANS(ERROR_UNKNOWN,ptClient," AVP CommonCode = [%d] not support!",ptClient->m_tHead.iCmdCode);
        return iRet;
    }
    }
    TADD_FUNC("end iRet=%d",iRet);
    return iRet;
}

int TMdbRedisAgentServer::Authentication(client *ptClient)
{
    TADD_FUNC("Begin.");
    int iRet = 0;
	int iRtnPort = 0;
    if(CSP_APP_LOGON != ptClient->m_tHead.iCmdCode)
    {//非登陆包
        CHECK_RET_SEND_ANS(ERR_CSP_PARSER_LOGON,ptClient,"want to recv logon package,but recv[%d]",ptClient->m_tHead.iCmdCode);
    }
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_LOGON,true);
        CHECK_OBJ(pParser);
        CHECK_RET(pParser->DeSerialize((unsigned char*)(ptClient->recvbuf),ptClient->m_tHead.iLen),"parser logon package error");
        //IS_LOG(2){pParser->Print();}
        //设置协议模式
        int iBig = MdbNtcIsBigEndian()?(MDB_CS_BIG_ED):(MDB_CS_LIT_ED);
        if(pParser->m_tHead.iVersion == MDB_CS_USE_OCP || iBig != pParser->m_tHead.iVersion)
        {//没配置，或者设置强制标记
            ptClient->m_tTMdbCSLink.m_iUseOcp = MDB_CS_USE_OCP;
        }
        else if(iBig == pParser->m_tHead.iVersion)
        {//大小头一致，则使用bin
            ptClient->m_tTMdbCSLink.m_iUseOcp = MDB_CS_USE_BIN;
        }
        
        //读取报文信息
        ptClient->m_tTMdbCSLink.m_sUser = pParser->GetStringValue(pParser->m_pRootAvpItem,AVP_USER_NAME);
        ptClient->m_tTMdbCSLink.m_sPass = pParser->GetStringValue(pParser->m_pRootAvpItem,AVP_USER_PWD);
        ptClient->m_tTMdbCSLink.m_sDSN = m_pDsn->sName;//保存DSN名称
        ptClient->m_tTMdbCSLink.m_iClientPID = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_PROCESS_ID);//获取客户端的进程ID
        ptClient->m_tTMdbCSLink.m_iClientTID = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_THREAD_ID);
		//第一次使用非ntc连接的时候，选择合适的端口
		iRtnPort = GetConnectAgentPort(pParser);
		//如果不需要重新换端口，则可以继续操作
		if(pParser->GetINT32Value(pParser->m_pRootAvpItem,AVP_CON_NUM) == 1 || iRtnPort == 0)
		{
	        //链接数据库
	        if(NULL == ptClient->m_pDB)
	        {
	            ptClient->m_pDB = new(std::nothrow) TMdbDatabase();
	            if(NULL == ptClient->m_pDB)
	            {
	                CHECK_RET_SEND_ANS(ERR_OS_NO_MEMROY,ptClient,"no mem for new db");
	            }
	        }
	        ptClient->m_pDB->SetLogin(ptClient->m_tTMdbCSLink.m_sUser,ptClient->m_tTMdbCSLink.m_sPass,ptClient->m_tTMdbCSLink.m_sDSN);
	        if(false ==  ptClient->m_pDB->Connect())
	        {//登陆失败
	            CHECK_RET_SEND_ANS(ERR_CSP_LOGON_FAILED,ptClient,"logon failed [%s/%s@%s] is error.",
	                                        ptClient->m_tTMdbCSLink.m_sUser,ptClient->m_tTMdbCSLink.m_sPass,ptClient->m_tTMdbCSLink.m_sDSN);
	        }
	        CHECK_RET_SEND_ANS(m_tLinkCtrl.RegRemoteLink(ptClient->m_tTMdbCSLink,iPort),ptClient,"RegRemoteLink failed.");
		}
    }
    catch(TMdbException& e)
    {
        CHECK_RET_SEND_ANS(e.GetErrCode(),ptClient,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        CHECK_RET_SEND_ANS(e.GetErrCode(),ptClient,"ERR_CODE=[%d].\nERROR_MSG=%s",e.GetErrCode(),e.GetErrMsg());
    }
    catch(...)
    {
        CHECK_RET_SEND_ANS(ERROR_UNKNOWN,ptClient,"UnKown error!");
    }
    ptClient->m_iSessionID = server.next_client_id++;
    //回复登陆成功
    ptClient->m_bRegister = true;
    TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_LOGON,false);
	//端口号写进去
	//如果端口号不需要换，填0，否则填端口号
	pParser->SetItemValue(pParser->m_pRootAvpItem,AVP_ANSWER_PORT,iRtnPort);
    pParser->SetVersion(ptClient->m_tTMdbCSLink.m_iUseOcp);
    CHECK_RET(SendAnswer(pParser,ptClient,0,"OK"),"SendAnswer[LOGIN_OK],failed");
    return iRet;
}
int TMdbRedisAgentServer::AppSendSQL(client *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    int iSqlFlag = 0;
    char * sSQL   = NULL;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->recvbuf,ptClient->m_tHead.iLen);//解析
        //pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
        iSqlLabel = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        sSQL	  = pParser->GetStringValue(pParser->m_pRootAvpItem,AVP_SQL_STATEMENT);
        if(NULL != pParser->FindExistAvpItem(pParser->m_pRootAvpItem,AVP_SQL_FLAG))
        {//有sql-flag
            iSqlFlag = pParser->GetINT32Value(pParser->m_pRootAvpItem,AVP_SQL_FLAG);
        }
        stClientQuery * pClientQuery = GetClientQuery(ptClient,iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
           CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        pQuery->SetSQL(sSQL,iSqlFlag,0);//设置SQL
        TADD_NORMAL("QuerySqlLabel=[%d],SetSQL(%s),IsDynamic=[%s],SqlFlag=[%d]",iSqlLabel,sSQL,(pQuery->ParamCount() != 0)?"TRUE":"FALSE",iSqlFlag);
        TMdbCspParser * pSendSQLAns = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,false);
        CHECK_OBJ(pSendSQLAns);
        pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)iSqlLabel);//先设置sqllable
        if(pQuery->ParamCount() != 0)
        {
            //动态SQL
            pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SELECT_HAVE_NEXT,(int)0);
            pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_AFFECTED_ROW,0);
        }
        else
        {
            //静态SQL
            if(TK_SELECT == pQuery->GetSQLType())
            {
                pQuery->Open();
                TADD_FLOW("QuerySqlLabel=[%d],Open(),StaticSQL.",iSqlLabel);
                CHECK_RET(FillNextResult(ptClient,pQuery,pSendSQLAns),"FillNextResult error");
            }
            else
            {
                pQuery->Execute();
                TADD_FLOW("QuerySqlLabel=[%d],Execute(),StaticSQL.",iSqlLabel);
                pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SELECT_HAVE_NEXT,(int)0);
                pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_AFFECTED_ROW,(int)pQuery->RowsAffected());
            }
        }
        char sMsg[256] = {0};
        snprintf(sMsg,sizeof(sMsg)-1,"QuerySqlLabel[%d] SetSQL OK.",iSqlLabel);
        SendAnswer(pSendSQLAns,ptClient,0,sMsg);
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"UnKown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKOWN");
    }
    return iRet;
}

/******************************************************************************
* 函数名称	:  AppSendParam
* 函数描述	:  处理发送来的参数
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbRedisAgentServer::AppSendParam(client *ptClient)
{
    int iRet      = 0;
    int iSqlLabel = 0;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_PARAM,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->recvbuf,ptClient->m_tHead.iLen);
       // pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
        iSqlLabel         = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        stClientQuery * pClientQuery = GetClientQuery(ptClient,iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
            CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        std::vector<TMdbAvpItem *> vParamGroup;
        pParser->GetExistGroupItem(pParser->m_pRootAvpItem,AVP_PARAM_GROUP,vParamGroup);
        std::vector<TMdbAvpItem *>::iterator itor = vParamGroup.begin();//设置参数
        for(; itor != vParamGroup.end(); ++itor)
        {
            if( pParser->IsNullValue((*itor)->pChildItem,AVP_PARAM_VALUE))
            {//null值
                pQuery->SetParameterNULL(pParser->GetStringValue((*itor)->pChildItem,AVP_PARAM_NAME));
            }
            else
            {
                pQuery->SetParameter(pParser->GetStringValue((*itor)->pChildItem,AVP_PARAM_NAME),
                                                 pParser->GetStringValue((*itor)->pChildItem,AVP_PARAM_VALUE));
            }
        }
        TMdbCspParser * pSendSQLAns = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,false);
        CHECK_OBJ(pSendSQLAns);
        pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)iSqlLabel);//先设置sqllable
        if(pQuery->GetSQLType() == TK_SELECT)
        {
            pQuery->Open();
            CHECK_RET(FillNextResult(ptClient,pQuery,pSendSQLAns),"FillNextResult error");
        }
        else
        {
            pQuery->Execute();
            
            pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SELECT_HAVE_NEXT,(int)0);
            pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_AFFECTED_ROW,(int)pQuery->RowsAffected());
        }
        SendAnswer(pSendSQLAns,ptClient,0,"AppSendParam OK");
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"UnKown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKOWN");
    }

    return iRet;
}

int TMdbRedisAgentServer::AppSendParamArray(client *ptClient)
{
    TADD_FUNC("Start,AppSendParamArray");
    int iRet = 0;
    int iSqlLabel = 0;
     try
     {
         TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_PARAM_ARRAY,true);
         CHECK_OBJ(pParser);
         pParser->DeSerialize(ptClient->recvbuf,ptClient->m_tHead.iLen);
         //pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
         IS_LOG(2){pParser->Print();}
         iSqlLabel         = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
         stClientQuery * pClientQuery = GetClientQuery(ptClient,iSqlLabel);//根据SQLLABEL，获取client
         if(NULL == pClientQuery)
         {
             CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
         }
         TMdbQuery * pQuery = pClientQuery->m_pQuery;
         CHECK_OBJ(pQuery);
         if(TK_SELECT == pQuery->GetSQLType())
         {
            iRet = AppErrorAnswer(ptClient,ERR_SQL_INVALID,"cs-select[%s] not support setparamarray",pQuery->GetSQL());
            return iRet;
         }
         
         TMdbAvpItem *pBatchGroupItem = pParser->GetExistGroupItemForParam(pParser->m_pRootAvpItem,AVP_BATCH_GROUP);
         if(NULL == pBatchGroupItem)
         {
              CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get AVP_BATCH_GROUP");
         }
         int iRowEffect = 0;
         int iParamIndex = 0;
         for(; pBatchGroupItem != NULL && false == pParser->IsAvpFree(pBatchGroupItem); pBatchGroupItem = pBatchGroupItem->pNextItem)
         {
             TMdbAvpItem *pParamGroupItem = pBatchGroupItem->pChildItem;
             iParamIndex = 0;
             for(; pParamGroupItem != NULL && false == pParser->IsAvpFree(pParamGroupItem); pParamGroupItem = pParamGroupItem->pNextItem)
             {
                 if(pParamGroupItem->m_bNULLValue)
                 {//null值
                     pQuery->SetParameterNULL(iParamIndex++);
                 }
                 else
                 {//ParamGroup 自带value
                     pQuery->SetParameter(iParamIndex++, pParamGroupItem->pszValue);
                 }
             }
             pQuery->Execute();
             iRowEffect += pQuery->RowsAffected();//执行
         }
         /*
         std::vector<TMdbAvpItem *> vBatchGroup;//批量值
         pParser->GetExistGroupItem(pParser->m_pRootAvpItem,AVP_BATCH_GROUP,vBatchGroup);
         std::vector<TMdbAvpItem *>::iterator itorBatch = vBatchGroup.begin();
         int iRowEffect = 0;
         for(;itorBatch != vBatchGroup.end();++itorBatch)
         {
            std::vector<TMdbAvpItem *> vParamGroup;
            pParser->GetExistGroupItem((*itorBatch)->pChildItem,AVP_PARAM_GROUP,vParamGroup);
            std::vector<TMdbAvpItem *>::iterator itor = vParamGroup.begin();//设置参数
            for(; itor != vParamGroup.end(); ++itor)
            {
                if( pParser->IsNullValue((*itor)->pChildItem,AVP_PARAM_VALUE))
                {//null值
                    pQuery->SetParameterNULL(pParser->GetStringValue((*itor)->pChildItem,AVP_PARAM_NAME));
                }
                else
                {
                    pQuery->SetParameter(pParser->GetStringValue((*itor)->pChildItem,AVP_PARAM_NAME),
                                                     pParser->GetStringValue((*itor)->pChildItem,AVP_PARAM_VALUE));
                }
            }
            pQuery->Execute();
            iRowEffect += pQuery->RowsAffected();//执行
         }
         */
         TMdbCspParser * pSendSQLAns = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,false);
         CHECK_OBJ(pSendSQLAns);
         pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)iSqlLabel);//先设置sqllable
         pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SELECT_HAVE_NEXT,(int)0);
         pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_AFFECTED_ROW,(int)iRowEffect);
         SendAnswer(pSendSQLAns,ptClient,0,"AppSendParam OK");
     }
     catch(TMdbException& e)
     {
         TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
         iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
     }
     catch(TMdbCSPException& e)
     {
         TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
         iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
     }
     catch(...)
     {
         TADD_ERROR(ERROR_UNKNOWN,"UnKown error!");
         iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKOWN");
     }
    return iRet;
}

//填充查询的next 返回值
int TMdbRedisAgentServer::FillNextResult(client *ptClient,TMdbQuery * pQuery,TMdbCspParser * pSendSQLAns)throw(TMdbException,TMdbCSPException)
{
    int iRet = 0;
    //TMdbCspParser * pSendSQLAns = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,false);
    CHECK_OBJ(pSendSQLAns);
    int iCount = 0;
    TMdbAvpItem * pRowGroup 	= NULL;
    TMdbAvpItem * pColumnGroup	= NULL;
    bool bHaveNext = true;
    //for(i = 0; i<PARTIAL_NEXT_SEND_NUM; i++) //每次next最大条数 
    pRowGroup = pSendSQLAns->m_pRootAvpItem;
    while(1)
    {
        int iRowGroupLen = pSendSQLAns->GetTotalLen();
        
        if(pQuery->Next())
        {
            iCount ++;
            pRowGroup = pSendSQLAns->GetFreeAvpItem(pRowGroup,AVP_ROW_GROUP);
            CHECK_OBJ(pRowGroup);
            int j = 0;
            pColumnGroup = pRowGroup->pChildItem;
            for(j = 0; j<pQuery->FieldCount(); j++)
            {
                int iColumnGroupLen = pSendSQLAns->GetTotalLen();
                pColumnGroup = pSendSQLAns->GetFreeAvpItem(pColumnGroup,AVP_COLUMN_GROUP);
                pSendSQLAns->SetItemValue(pColumnGroup->pChildItem,AVP_COLUMN_NAME,pQuery->Field(j).GetName());
                if(pQuery->Field(j).isNULL())
                {//null值
                    pSendSQLAns->SetItemValueNULL(pColumnGroup->pChildItem,AVP_COLUMN_VALUE);
                }
                else
                {
                    pSendSQLAns->SetItemValue(pColumnGroup->pChildItem,AVP_COLUMN_VALUE,pQuery->Field(j).AsString());
                }
                pSendSQLAns->FinishFillGroup(pColumnGroup,pSendSQLAns->GetTotalLen()-iColumnGroupLen);
                //pColumnGroup->FinishFill();
            }
            pSendSQLAns->FinishFillGroup(pRowGroup,pSendSQLAns->GetTotalLen()-iRowGroupLen);
            //pRowGroup->FinishFill();
            if(pSendSQLAns->GetTotalLen() > MAX_CSP_LEN - 4096)
            {//达到最大值
                TADD_FLOW("Up to MAX next[%d]",iCount);
                break;
            }
        }
        else
        {
            //next 完了
            bHaveNext  =  false;
            break;
        }
    }
    pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_AFFECTED_ROW,(int)iCount);
    if(bHaveNext)
    {
        pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SELECT_HAVE_NEXT,(int)1);
    }
    else
    {
        pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SELECT_HAVE_NEXT,(int)0);
    }
    return iRet;
}

//响应next操作

int TMdbRedisAgentServer::AppNextSQLResult(client *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_NEXT,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->recvbuf,ptClient->m_tHead.iLen);
        //pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
        iSqlLabel  = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        
        stClientQuery * pClientQuery = GetClientQuery(ptClient,iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
            CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        TMdbCspParser * pAnsParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,false);
        CHECK_OBJ(pAnsParser);
        pAnsParser->SetItemValue(pAnsParser->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)iSqlLabel);
        if(pQuery->GetSQLType() == TK_SELECT)
        {
            CHECK_RET(FillNextResult(ptClient,pQuery,pAnsParser),"FillNextResult error");
            SendAnswer(pAnsParser,ptClient,0,"AppNextSQLResult OK");
        }
        else
        {
            //不是查询
            char sErrorInfo[128] = {0};
            snprintf(sErrorInfo,sizeof(sErrorInfo),"SQL TYPE[%d] is not select",pQuery->GetSQLType());
            SendAnswer(pAnsParser,ptClient,ERR_SQL_INVALID,sErrorInfo);
        }
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"UnKown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKOWN");
    }
    return iRet;
}

int TMdbRedisAgentServer::AppTransAction(client *ptClient)
{
    int iRet = 0;
    try
    {
        TMdbCspParser * pParserSend = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_ACTION,true);
        CHECK_OBJ(pParserSend);
        pParserSend->DeSerialize(ptClient->recvbuf,ptClient->m_tHead.iLen);//解析
        //pParserSend->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParserSend->Print();}
        char * sCmd = NULL;
        sCmd = pParserSend->GetStringValue(pParserSend->m_pRootAvpItem,AVP_COMMAND_NAME);
        if(TMdbNtcStrFunc::StrNoCaseCmp(sCmd,"rollback") == 0)
        {
            ptClient->m_pDB->Rollback();
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sCmd,"commit") == 0)
        {
            ptClient->m_pDB->Commit();
        }
        else
        {
            //ERROR_TO_THROW_NOSQL(-1,"sCmd[%s] is not rollback/commit",sCmd);
            TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=sCmd[%s] is not rollback/commit",ERR_CSP_PARSER_ERROR_CSP, sCmd);
        	iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=sCmd[%s] is not rollback/commit",ERR_CSP_PARSER_ERROR_CSP, sCmd);
			return iRet;
		}
        TMdbCspParser * pParserAns = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_ACTION,false);
        CHECK_RET(SendAnswer(pParserAns,ptClient,0,"OK"),"SendAnswer error.");
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"UnKown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKOWN");
    }
    return iRet;
}

/******************************************************************************
* 函数名称	:  AppGetSequence
* 函数描述	:  获取序列
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbRedisAgentServer::AppGetSequence(client *ptClient)
{
    int iRet = 0;
    /*
    try
    {
        TMdbCspParser * pParserSend = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_GET_SEQUENCE,true);
        CHECK_OBJ(pParserSend);
        pParserSend->DeSerialize(ptClient->recvbuf,ptClient->m_tHead.iLen);//解析
        //pParserSend->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParserSend->Print();}
        char * sSeqName = NULL;
        sSeqName = pParserSend->GetStringValue(pParserSend->m_pRootAvpItem,AVP_SEQUENCE_NAME);
        TMdbSequenceMgr * pSeqMgr = NULL; //ptClient->GetSeqMgrByName(sSeqName,m_pConfig);
        if(NULL == pSeqMgr)
        {
           // ERROR_TO_THROW_NOSQL(-1,"not find sequence mgr by [%s].",sSeqName);
           TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=not find sequence mgr by [%s]",ERROR_UNKNOWN,"no sql", sSeqName);
           iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=not find sequence mgr by [%s]",ERROR_UNKNOWN,"no sql", sSeqName);
		   return iRet;
        }

        TMdbCspParser * pParserRecv = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_GET_SEQUENCE,false);
        CHECK_OBJ(pParserRecv);
        pParserRecv->SetItemValue(pParserRecv->m_pRootAvpItem,AVP_SEQUENCE_VALUE,pSeqMgr->m_Sequence.GetNextIntVal());
        CHECK_RET(SendAnswer(pParserRecv,ptClient,0,"OK"),"SendAnswer error.return value=[%d]",iRet);
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"UnKown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKOWN");
    }*/
    return iRet;
}

int TMdbRedisAgentServer::SendAnswer(TMdbCspParser * pCspParser,client *ptClient,int iAnsCode,const char * sMsg)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pCspParser);
    try
    {
        pCspParser->SetItemValue(pCspParser->m_pRootAvpItem,AVP_ANSWER_CODE,iAnsCode);
        pCspParser->SetItemValue(pCspParser->m_pRootAvpItem,AVP_ANSWER_MSG,sMsg);
        pCspParser->Serialize(ptClient->sendbuf,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        ptClient->bufpos = pCspParser->m_tHead.iLen;
        ptClient->flags |= CLIENT_PENDING_WRITE;
        listAddNodeHead(server.clients_pending_write,ptClient);
    }
    catch(TMdbCSPException& e)
    {
        CHECK_RET(ERR_CSP_PARSER_ERROR_CSP,"code[%d],msg[%s]",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        CHECK_RET(ERROR_UNKNOWN,"ERROR_UNKOWN");
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbRedisAgentServer::DoOperationBin(client *ptClient)
{
    TADD_DETAIL("start...ptClient->m_tHead.iCmdCode=%d",ptClient->m_tHead.iCmdCode);
    int iRet = 0;
    switch(ptClient->m_tRecvDataParse.iCmdCode)
    {
    /*
        case CSP_APP_LOGON:
        {
            CHECK_RET_BREAK(AuthenticationBin(ptClient),"Authentication failed.");//认证
            TADD_NORMAL("ThreadID=[%lu],ClientIP=[%s],SequenceID=[%d],SocketID=[%d],Pno=[%d] Login Success!! ",
                    ptClient->m_iThreadId,ptClient->m_tTMdbCSLink.m_sClientIP,ptClient->m_tHead.GetSequence(),
                    ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno);
            break;
        }*/
        case CSP_APP_SEND_SQL:
        {
            //接受SQL注册
            iRet = AppSendSQLBin(ptClient);
            CHECK_RET(iRet,"AppSendSQL return Value=[%d]",iRet);
            break;
        }
        case CSP_APP_SEND_PARAM: //0k
        {
            iRet = AppSendParamBin(ptClient);
            CHECK_RET(iRet,"AppSendParam return Value=[%d]",iRet);
            break;
        }
        case CSP_APP_SEND_PARAM_ARRAY:
        {
            iRet = AppSendParamArrayBin(ptClient);
            CHECK_RET(iRet,"AppSendParamArray return Value=[%d]",iRet);
            break;
        }
        case CSP_APP_ACTION:     //ok
        {
            iRet = AppTransActionBin(ptClient);
            CHECK_RET(iRet,"AppNextSQLResult return Value=[%d]",iRet);
            break;
        }
        case CSP_APP_NEXT:          //ok
        {
            iRet = AppNextSQLResultBin(ptClient);
            CHECK_RET(iRet,"AppNextSQLResult return Value=[%d]",iRet);
            break;

        }
        case CSP_APP_QMDB_INFO://获取qmdb信息
        {
            iRet = AppGetQmdbInfoBin(ptClient);
            CHECK_RET(iRet,"AppGetQmdbInfo return Value=[%d]",iRet);
            break;
        }/*
        case CSP_APP_GET_SEQUENCE:
        {
            iRet = AppGetSequenceBin(ptClient);
            CHECK_RET(iRet,"AppGetSequence return Value=[%d]",iRet);
            break;
        }*/
        default:
        {
            CHECK_RET_SEND_ANS(ERROR_UNKNOWN,ptClient," AVP CommonCode = [%d] not support!",ptClient->m_tHead.iCmdCode);
            return iRet;
        }
    }
    TADD_FUNC("end iRet=%d",iRet);
    return iRet;
}
/*
int TMdbRedisAgentServer::AuthenticationBin(client *ptClient)
{
    TADD_FUNC("Begin.");
    int iRet = 0;
    if(CSP_APP_LOGON != ptClient->m_tHead.iCmdCode)
    {//非登陆包
        CHECK_RET_SEND_ANS(ERR_CSP_PARSER_LOGON,ptClient,"want to recv logon package,but recv[%d]",ptClient->m_tHead.iCmdCode);
    }
    try
    {
        //读取报文信息
        char user[NO_OCP_USERNAME_LEN]="";
        char pwd[NO_OCP_USERNAME_LEN]="";
        
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->recvbuf);
        ptClient->m_tRecvDataParse.GetData((char*)user,NO_OCP_USERNAME_POS,NO_OCP_USERNAME_LEN);
        ptClient->m_tRecvDataParse.GetData((char*)pwd,NO_OCP_PASSWORD_POS,NO_OCP_PASSWORD_LEN);
        ptClient->m_tTMdbCSLink.m_sDSN = m_pDsn->sName;//保存DSN名称
        ptClient->m_tTMdbCSLink.m_sUser = (char*)user;
        ptClient->m_tTMdbCSLink.m_sPass = (char*)pwd;
        //链接数据库
        if(NULL == ptClient->m_pDB)
        {
            ptClient->m_pDB = new(std::nothrow) TMdbDatabase();
            if(NULL == ptClient->m_pDB)
            {
                CHECK_RET_SEND_ANS(ERR_OS_NO_MEMROY,ptClient,"no mem for new db");
            }
        }
        ptClient->m_pDB->SetLogin(ptClient->m_tTMdbCSLink.m_sUser,ptClient->m_tTMdbCSLink.m_sPass,ptClient->m_tTMdbCSLink.m_sDSN);
        if(false ==  ptClient->m_pDB->Connect())
        {//登陆失败
            CHECK_RET_SEND_ANS(ERR_CSP_LOGON_FAILED,ptClient,"logon failed [%s/%s@%s] is error.",
                                        ptClient->m_tTMdbCSLink.m_sUser,ptClient->m_tTMdbCSLink.m_sPass,ptClient->m_tTMdbCSLink.m_sDSN);
        }
        CHECK_RET_SEND_ANS(m_tLinkCtrl.RegRemoteLink(ptClient->m_tTMdbCSLink),ptClient,"RegRemoteLink failed.");
    }
    catch(TMdbException& e)
    {
        CHECK_RET_SEND_ANS(e.GetErrCode(),ptClient,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        CHECK_RET_SEND_ANS(e.GetErrCode(),ptClient,"ERR_CODE=[%d].\nERROR_MSG=%s",e.GetErrCode(),e.GetErrMsg());
    }
    catch(...)
    {
        CHECK_RET_SEND_ANS(ERROR_UNKNOWN,ptClient,"Unknown error!");
    }
    //生成SessionID
    CHECK_RET(tMutex.Lock(true),"Lock Faild");
    TMdbRedisAgentServer::m_iSessionStep++;
    if(TMdbRedisAgentServer::m_iSessionStep >= INT_MAX)
    {
        m_iSessionStep = 1;
    }
    ptClient->m_iSessionID = m_iSessionStep;
    CHECK_RET(tMutex.UnLock(true),"UnLock Faild");
    //回复登陆成功
    //head+anscode+msg
    ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->sendbuf,CSP_APP_LOGON,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
    CHECK_RET(SendAnswer(ptClient,0,"LOGIN_OK"),"SendAnswer[LOGIN_OK],failed");
    return iRet;
}
*/
int TMdbRedisAgentServer::AppSendSQLBin(client *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    int iSqlFlag = 0;
    int iTmp = 0;
    //char sSQL[MAX_SQL_LEN]   = "";
    try
    {
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->recvbuf);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        char *pSQL = ptClient->m_tRecvDataParse.GetDataPtr() + NO_OCP_SQL_POS;
        //ptClient->m_tRecvDataParse.GetData(sSQL,NO_OCP_SQL_POS,ptClient->m_tHead.iLen - NO_OCP_SQL_POS);
        stClientQuery * pClientQuery = GetClientQuery(ptClient,iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
           CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        pQuery->SetSQL(pSQL,iSqlFlag,0);//设置SQL
        ptClient->m_iParamCount = pQuery->ParamCount();
        TADD_NORMAL("QuerySqlLabel=[%d],SetSQL(%s),IsDynamic=[%s],SqlFlag=[%d]",iSqlLabel,pSQL,(ptClient->m_iParamCount != 0)?"TRUE":"FALSE",iSqlFlag);
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->sendbuf,CSP_APP_SEND_SQL,ptClient->m_tRecvDataParse.iSessionId,ptClient->m_tRecvDataParse.isequence);
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        ptClient->m_bFirstNext = true;
        ptClient->m_iFieldCount = pQuery->FieldCount();
        ptClient->m_iSqlType = pQuery->GetSQLType();
        if(ptClient->m_iParamCount != 0)
        {
            //动态SQL
            iTmp = 0;
            ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(char));//AVP_SELECT_HAVE_NEXT
            ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(short int));//AVP_AFFECTED_ROW
        }
        else
        {
            //静态SQL
            if(TK_SELECT == ptClient->m_iSqlType)
            {
                pQuery->Open();
                TADD_FLOW("QuerySqlLabel=[%d],Open(),StaticSQL.",iSqlLabel);
                CHECK_RET(FillNextResult(ptClient,pQuery,ptClient->m_bFirstNext),"FillNextResult error");
            }
            else
            {
                pQuery->Execute();
                iTmp = 0;
                ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(char));//AVP_SELECT_HAVE_NEXT
                iTmp = pQuery->RowsAffected();
                ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(short int));//AVP_AFFECTED_ROW
                
                TADD_FLOW("QuerySqlLabel=[%d],Execute(),StaticSQL.",iSqlLabel);
            
            }
        }
        SendAnswer(ptClient,0,NULL);
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }
    return iRet;
}
int TMdbRedisAgentServer::AppSendParamBin(client *ptClient)
{
    int iRet      = 0;
    int iSqlLabel = 0;
    try
    {
        //ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->recvbuf);
        //ptClient->m_tRecvDataParse.GetData(&iSqlLabel,4,sizeof(int));
        
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        stClientQuery * pClientQuery = GetClientQuery(ptClient,iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
            CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        int iParamCount = 0;
        int iParamPackageLen = 0;
        int iParamIndex = 0;
        int iParamPos = 0;
        //head+sqllabel+cnt+packageLen+paramIndex+Value
        //packageLen = sizeof(int) * 2 -> null
        //ptClient->m_tRecvDataParse.GetData(&iParamCount,8,sizeof(int));
        //iParamPos += 8 + sizeof(int); 
        //#if 0
        ptClient->m_tRecvDataParse.GetData(&iParamCount,NO_OCP_PARAMCNT_POS,sizeof(char));
        iParamPos += NO_OCP_PARAMCNT_POS + sizeof(char); 
        for(int i=0; i<iParamCount; ++i)
        {
            ptClient->m_tRecvDataParse.GetData(&iParamPackageLen,iParamPos,sizeof(short int));
            ptClient->m_tRecvDataParse.GetData(&iParamIndex,iParamPos+sizeof(short int),sizeof(char));
            if(iParamPackageLen == (sizeof(char)+ sizeof(short int)))
            {//NULL
                pQuery->SetParameterNULL(iParamIndex);
            }
            else
            {
                char *pData = ptClient->m_tRecvDataParse.GetDataPtr() + iParamPos + sizeof(short int) + sizeof(char);
                ST_MEM_VALUE * pMemValue = pQuery->GetParamByIndex(iParamIndex);
                if(pMemValue && MemValueHasAnyProperty(pMemValue,MEM_Str|MEM_Date))
                {
                    pQuery->SetParameter(iParamIndex,pData);
                }
                else if(pMemValue)
                {
                    pQuery->SetParameter(iParamIndex,*(long long*)pData);
                }
                else
                {
                    TADD_ERROR(-1,"not find param by iParamIndex[%d]",iParamIndex);
                    iRet = AppErrorAnswer(ptClient,-1,"not find param by iParamIndex[%d]",iParamIndex); 
                    return iRet;
                }
            }
            iParamPos += iParamPackageLen;
        }
        //#endif
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->sendbuf,CSP_APP_SEND_SQL,ptClient->m_tRecvDataParse.iSessionId,ptClient->m_tRecvDataParse.isequence);
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        //ptClient->m_bFirstNext = true;
        if(ptClient->m_iSqlType == TK_SELECT)
        {
            pQuery->Open();
            CHECK_RET(FillNextResult(ptClient,pQuery,ptClient->m_bFirstNext),"FillNextResult error");
        }
        else
        {//
            pQuery->Execute();
            int iTmp = 0;
            ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(char));//AVP_SELECT_HAVE_NEXT
            iTmp = pQuery->RowsAffected();
            ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(short int));//AVP_AFFECTED_ROW
        
        }
        //ptClient->m_bFirstNext = false;
        SendAnswer(ptClient,0,NULL);
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }

    return iRet;
}

int TMdbRedisAgentServer::AppSendParamArrayBin(client *ptClient)
{
    TADD_FUNC("Start,AppSendParamArray");
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        //ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        stClientQuery * pClientQuery = GetClientQuery(ptClient,iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
             CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        if(TK_SELECT == pQuery->GetSQLType())
        {
            iRet = AppErrorAnswer(ptClient,ERR_SQL_INVALID,"cs-select[%s] not support setparamarray",pQuery->GetSQL());
            return iRet;
        }

        int iBatchCount = 0;
        int iParamCount = 0;
        int iParamPackageLen = 0;
        int iParamIndex = 0;
        int iParamPos = 0;
        int iRowEffect = 0;
        
        //head+sqllabel+batchcnt+pramcount+packageLen+Value
        //packageLen = sizeof(int) * 2 -> null
        ptClient->m_tRecvDataParse.GetData(&iBatchCount,NO_OCP_BATCHCNT_POS,sizeof(short int));
        ptClient->m_tRecvDataParse.GetData(&iParamCount,NO_OCP_BATCHCNT_POS+sizeof(short int),sizeof(char));
        iParamPos += NO_OCP_BATCHCNT_POS + sizeof(short int) + sizeof(char); 
        
        //获取首个BATCH_GROUP
        for(int iBatchIndex = 0; iBatchIndex < iBatchCount; ++iBatchIndex)
        {
            //一个批次
            for(iParamIndex=0; iParamIndex<iParamCount; ++iParamIndex)
            {
                ptClient->m_tRecvDataParse.GetData(&iParamPackageLen,iParamPos,sizeof(short int));
                //ptClient->m_tRecvDataParse.GetData(&iParamIndex,iParamPos+sizeof(int),sizeof(int));
                if(iParamPackageLen == sizeof(short int) )
                {
                    pQuery->SetParameterNULL(iParamIndex);
                }
                else
                {
                    char *pData = ptClient->m_tRecvDataParse.GetDataPtr() + iParamPos + sizeof(short int);
                    ST_MEM_VALUE * pMemValue = pQuery->GetParamByIndex(iParamIndex);
                    if(pMemValue && MemValueHasAnyProperty(pMemValue,MEM_Str|MEM_Date))
                    {
                        pQuery->SetParameter(iParamIndex,pData);
                    }
                    else if(pMemValue)
                    {
                        pQuery->SetParameter(iParamIndex,*(long long*)pData);
                    }
                    else
                    {
                        TADD_ERROR(-1,"not find param by iParamIndex[%d]",iParamIndex);
                        iRet = AppErrorAnswer(ptClient,-1,"not find param by iParamIndex[%d]",iParamIndex); 
                        return iRet;
                    }
                    
                }
                iParamPos += iParamPackageLen;
            }
            pQuery->Execute();
            iRowEffect += pQuery->RowsAffected();//执行
        }

        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->sendbuf,CSP_APP_SEND_SQL,ptClient->m_tRecvDataParse.iSessionId,ptClient->m_tRecvDataParse.isequence);
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        int iTmp = 0;
        ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(char));//AVP_SELECT_HAVE_NEXT
        ptClient->m_tSendDataParse.SetData(&iRowEffect,sizeof(short int));//AVP_AFFECTED_ROW
        SendAnswer(ptClient,0,NULL);
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }
    return iRet;
}

int TMdbRedisAgentServer::FillNextResult(client *ptClient,TMdbQuery * pQuery,bool bFirstNext)throw(TMdbException,TMdbCSPException)
{
    int iRet = 0;
    int iCount = 0;
    int iSetTmp = 0;
    int iValueLen = 0;
    char cDataType = 0;
    bool bHaveNext = true;
    bool bFirstNextTmp = bFirstNext;
    //char value[]="value";
    //bool bSetFieldCount = false;
    //bool bFirst = false;
    //head+sqllabel+haveNext+recordCount+fieldCount+nameLen+fieldName+valueLen+Value
    //fieldLen=4 -> null, value no
    ptClient->m_tSendDataParse.SetData(&iSetTmp,sizeof(char));//have next
    ptClient->m_tSendDataParse.SetData(&iSetTmp,sizeof(short int));//记录个数
    //#if 0
    while(1)
    {
        if(pQuery->Next())
        {
            iCount ++;
            int j = 0;
            if(bFirstNextTmp)
            {
                iSetTmp = ptClient->m_iFieldCount;
                ptClient->m_tSendDataParse.SetData(&iSetTmp,sizeof(char));//字段个数
                //bSetFieldCount = true;
            }
            #if 0
            if(ptClient->m_bFirstNext && 1 == iCount)
            {
                bFirst = true;
            }
            pQuery->FillFieldForCSBin(ptClient->m_tSendDataParse,bFirst);
            #endif
            //#if 0
            for(j = 0; j<ptClient->m_iFieldCount; j++)
            {
                cDataType = pQuery->Field(j).GetDataType();
                if(bFirstNextTmp)
                {
                    iValueLen = strlen(pQuery->Field(j).GetName())+1;
                    ptClient->m_tSendDataParse.SetData(&cDataType,sizeof(cDataType));//name len
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));//name len
                    ptClient->m_tSendDataParse.SetData(pQuery->Field(j).GetName(),iValueLen);//name value
                }
                if(pQuery->Field(j).isNULL())
                {//null值
                    iValueLen = 0;//null
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));
                }
                else if(DT_Int == cDataType)
                {
                    long long lData = pQuery->Field(j).AsRealnt();
                    iValueLen = sizeof(lData);
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));
                    ptClient->m_tSendDataParse.SetData(&lData,iValueLen);
                }
                else
                {
                    char *pData = pQuery->Field(j).AsRealStr();
                    iValueLen = strlen(pData) + 1;
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));
                    ptClient->m_tSendDataParse.SetData(pData,iValueLen);
                }
            }
            //#endif
            bFirstNextTmp = false;
            ptClient->m_bFirstNext = false;
            //break;
            if(ptClient->m_tSendDataParse.GetSize() > MAX_CSP_LEN - 4096)
            {//达到最大值
                TADD_FLOW("Up to MAX next[%d]",iCount);
                break;
            }
        }
        else
        {
            //next 完了
            bHaveNext  =  false;
            break;
        }
    }
    //#endif
    ptClient->m_tSendDataParse.SetData(&iCount,SIZE_MSG_BIN_HEAD+5,sizeof(short int));//记录个数
    if(bHaveNext)
    {
        iSetTmp = 1;
        ptClient->m_tSendDataParse.SetData(&iSetTmp,SIZE_MSG_BIN_HEAD+4,sizeof(char));//have next
    }
    else
    {
        iSetTmp = 0 ;
        ptClient->m_tSendDataParse.SetData(&iSetTmp,SIZE_MSG_BIN_HEAD+4,sizeof(char));//have next
    }
    return iRet;
}

//响应next操作
int TMdbRedisAgentServer::AppNextSQLResultBin(client *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->recvbuf);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        stClientQuery * pClientQuery = GetClientQuery(ptClient,iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
            CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->sendbuf,CSP_APP_SEND_SQL,ptClient->m_tRecvDataParse.iSessionId,ptClient->m_tRecvDataParse.isequence);
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        if(pQuery->GetSQLType() == TK_SELECT)
        {
            CHECK_RET(FillNextResult(ptClient,pQuery),"FillNextResult error");
            SendAnswer(ptClient,0,NULL);
        }
        else
        {
            //不是查询
            char sErrorInfo[128] = {0};
            snprintf(sErrorInfo,sizeof(sErrorInfo),"SQL TYPE[%d] is not select",pQuery->GetSQLType());
            SendAnswer(ptClient,ERR_SQL_INVALID,sErrorInfo);
        }
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }
    return iRet;
}
int TMdbRedisAgentServer::AppTransActionBin(client *ptClient)
{
    int iRet = 0;
    try
    {
        char sCmd[32]="";
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->recvbuf);
        ptClient->m_tRecvDataParse.GetData(sCmd,SIZE_MSG_BIN_HEAD,ptClient->m_tRecvDataParse.GetSize()- SIZE_MSG_BIN_HEAD);
        if(TMdbNtcStrFunc::StrNoCaseCmp(sCmd,"Commit") == 0)
        {
            ptClient->m_pDB->Commit();
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(sCmd,"Rollback") == 0)
        {
            ptClient->m_pDB->Rollback();
        }
        else
        {
            TADD_ERROR(-1,"sCmd[%s] is not rollback/commit",sCmd);
            iRet = AppErrorAnswer(ptClient,-1,"sCmd[%s] is not rollback/commit",sCmd); 
            return iRet;
        }
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->sendbuf,CSP_APP_ACTION,ptClient->m_tRecvDataParse.iSessionId,ptClient->m_tRecvDataParse.isequence);
        CHECK_RET(SendAnswer(ptClient,0,NULL),"SendAnswer error.");
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }
    return iRet;
}

int TMdbRedisAgentServer::AppGetQmdbInfoBin(client *ptClient)
{
    int iRet = 0;
    try
    {
        char sCmd[32]="";
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->recvbuf);
        ptClient->m_tRecvDataParse.GetData(sCmd,SIZE_MSG_BIN_HEAD,ptClient->m_tRecvDataParse.GetSize()- SIZE_MSG_BIN_HEAD);
        TMdbInfoCollect mdbInfoCollect;
        mdbInfoCollect.Attach(m_pDsn->sName);
        std::string str;
        mdbInfoCollect.GetInfo(str,sCmd);
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->sendbuf,CSP_APP_QMDB_INFO,ptClient->m_tRecvDataParse.iSessionId,ptClient->m_tRecvDataParse.isequence);
        CHECK_RET(SendAnswer(ptClient,0,str.c_str()),"SendAnswer error.");
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }
    return iRet;
}

int TMdbRedisAgentServer::AppGetSequenceBin(client *ptClient)
{
    int iRet = 0;
    //不缓存了，通过sql查询吧
    /*
    try
    {
        char sSeqName[64]="";
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->recvbuf);
        ptClient->m_tRecvDataParse.GetData(sSeqName,SIZE_MSG_BIN_HEAD,ptClient->m_tRecvDataParse.GetSize()- SIZE_MSG_BIN_HEAD);
        TMdbSequenceMgr * pSeqMgr = NULL;//ptClient->GetSeqMgrByName(sSeqName,m_pConfig);
        if(NULL == pSeqMgr)
        {
            TADD_ERROR(-1,"not find sequence mgr by [%s].",sSeqName);
            iRet = AppErrorAnswer(ptClient,-1,"not find sequence mgr by [%s].",sSeqName); 
            return iRet;
        }
        
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->sendbuf,CSP_APP_GET_SEQUENCE,ptClient->m_tRecvDataParse.iSessionId,ptClient->m_tRecvDataParse.isequence);
        long long llValue = pSeqMgr->m_Sequence.GetNextIntVal();
        ptClient->m_tSendDataParse.SetData(&llValue,sizeof(llValue));
        CHECK_RET(SendAnswer(ptClient,0,NULL),"SendAnswer error.");
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN,"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,e.GetErrCode(),"ERR_CODE=[%d].ERROR_SQL=%s.\nERROR_MSG=%s",e.GetErrCode(),e.GetErrSql(), e.GetErrMsg());
    }
    catch(TMdbCSPException& e)
    {
        TADD_ERROR(ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
        iRet = AppErrorAnswer(ptClient,ERR_CSP_PARSER_ERROR_CSP,"ERR_CODE=[%d]\nERROR_MSG=%s",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }*/
    return iRet;
}
int TMdbRedisAgentServer::SendAnswer(client *ptClient,int iAnsCode,const char * sMsg)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    try
    {
        ptClient->m_tSendDataParse.SetData(&iAnsCode,sizeof(short int),true);
        if(sMsg)
        {
            ptClient->m_tSendDataParse.SetData((char*)sMsg,strlen(sMsg)+1);
        }
        ptClient->m_tSendDataParse.SetSize();
        ptClient->bufpos = ptClient->m_tSendDataParse.GetSize();
        ptClient->flags |= CLIENT_PENDING_WRITE;
        listAddNodeHead(server.clients_pending_write,ptClient);
    }
    catch(TMdbCSPException& e)
    {
        CHECK_RET(ERR_CSP_PARSER_ERROR_CSP,"code[%d],msg[%s]",e.GetErrCode(), e.GetErrMsg());
    }
    catch(...)
    {
        CHECK_RET(ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbRedisAgentServer::AppErrorAnswer(client *ptClient,int iAnsCode,const char *fmt,...)
{
    char sTemp[10240] = {0};
    va_list ap;
    va_start(ap,fmt);
    memset(sTemp,0,sizeof(sTemp));
    vsnprintf(sTemp, sizeof(sTemp), fmt, ap);
    va_end (ap);
    if(MDB_CS_USE_OCP == ptClient->m_tTMdbCSLink.m_iUseOcp)
    {
        TMdbCspParser * pParserError = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_ERROR,false);
        return SendAnswer(pParserError, ptClient, iAnsCode,sTemp);
    }
    else
    {
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->sendbuf,CSP_APP_ERROR,ptClient->m_iSessionID,ptClient->m_tRecvDataParse.isequence);
        return SendAnswer(ptClient, iAnsCode, sTemp);
    }
}
void* TMdbRedisAgentServer::Agent(void *p)
{
    pthread_detach(pthread_self());//线程自动释放资源
    TMdbProcCtrl *pTMdbProcCtrl = (TMdbProcCtrl*)p;
    if(!pTMdbProcCtrl)
    {
        TADD_NORMAL("Agent p=NULL");
        return 0;
    }
    while(true)
    {
        if(pTMdbProcCtrl->IsCurProcStop())
        {
            TADD_NORMAL("Stop......");
         	TMdbDateTime::MSleep(1000);
            break;
        }
        //应用进程心跳时间
        pTMdbProcCtrl->UpdateProcHeart(0);
        TMdbDateTime::Sleep(iHeartBeat);
    }
    TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Process[mdbAgent] Exist.");
    return 0;
}

int TMdbRedisAgentServer::DoHeartBeat()
{
    while(true)
    {
        if(m_tProcCtrl.IsCurProcStop())
        {
            TADD_NORMAL("Stop......");
         	TMdbDateTime::MSleep(1000);
            break;
        }
        //应用进程心跳时间
        m_tProcCtrl.UpdateProcHeart(0);
        TMdbDateTime::Sleep(iHeartBeat);
    }
    TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Process[mdbAgent] Exit.");
    return 0;
}
int TMdbRedisAgentServer::DoMain()
{
    TADD_FUNC(" Begin.");
    pthread_t       tID = 0;;
    int iRet = 0;
    pthread_attr_t  thread_attr;
    pthread_attr_init(&thread_attr);
    errno = 0;
    iRet = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    if(0 != iRet)
    {
        TADD_ERROR(ERR_OS_SET_THREAD_ATTR,"Can't pthread_attr_setdetachstate(), errno=[%d], errmsg=[%s].", errno, strerror(errno));
        return ERR_OS_SET_THREAD_ATTR;
    }
    iRet = pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);
    if(0 != iRet)
    {
        TADD_ERROR(ERR_OS_SET_THREAD_ATTR,"Can't pthread_attr_setscope(), errno=[%d], errmsg=[%s].",errno, strerror(errno));
        return ERR_OS_SET_THREAD_ATTR;
    }
    iRet = pthread_attr_setstacksize(&thread_attr, 1024*1024*m_pConfig->GetDSN()->iAgentThreadStackSize);
    if(0 != iRet)
    {
        TADD_ERROR(ERR_OS_SET_THREAD_ATTR,"Can't pthread_attr_setstacksize(), errno=[%d], errmsg=[%s].",errno, strerror(errno));
        return ERR_OS_SET_THREAD_ATTR;
    }
    if(0 != pthread_create(&tID, &thread_attr, aeMain, server.el))
    {
        TADD_ERROR(ERR_OS_SET_THREAD_ATTR,"Can't pthread_create(), errno=[%d], errmsg=[%s].",errno, strerror(errno));
        return ERR_OS_CREATE_THREAD;
    }
    TADD_FUNC("end.");
    return 0;
}

int mdbRedisServermain(const char* pszDSN, int iAgentPort) 
{
    gMdbAgent.Init(pszDSN, iAgentPort);
    initServerConfig();
    initServer(iAgentPort);
    aeSetBeforeSleepProc(server.el,handleClientsWithPendingWrites);
    gMdbAgent.DoMain();
    gMdbAgent.DoHeartBeat();
    //aeMain(server.el);
    return 0;    
}


/* The End */
