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

#ifndef __REDIS_H
#define __REDIS_H

#include "fmacros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <syslog.h>
#include <netinet/in.h>
#include <signal.h>
#include "Interface/mdbQuery.h"
#include "Helper/SqlParserStruct.h"
#include "Helper/mdbCSPParser.h"
#include "Helper/mdbCspAvpMgr.h"
#include "Control/mdbProcCtrl.h"
#include "Control/mdbLinkCtrl.h"

#include "ae.h"      /* Event driven programming library */
//#include "sds.h"     /* Dynamic safe strings */
#include "adlist.h"  /* Linked lists */
#include "anet.h"    /* Networking the easy way */

/* Error codes */
#define C_OK                    0
#define C_ERR                   -1

/* Static server configuration */
#define CONFIG_DEFAULT_SERVER_PORT        6379    /* TCP port */
#define CONFIG_DEFAULT_TCP_BACKLOG       511     /* TCP listen backlog */
#define NET_MAX_WRITES_PER_EVENT (1024*64)
#define CONFIG_DEFAULT_MAX_CLIENTS 10000

/* Protocol and I/O related defines */
#define PROTO_MAX_QUERYBUF_LEN  (1024*1024*1024) /* 1GB max query buffer. */
#define PROTO_IOBUF_LEN         (1024*16)  /* Generic I/O buffer size */
#define PROTO_INLINE_MAX_SIZE   (1024*64) /* Max size of inline reads */

/* Defines related to the dump file format. To store 32 bits lengths for short
 * keys requires a lot of space, so we check the most significant 2 bits of
 * the first byte to interpreter the length:
 *
 * 00|000000 => if the two MSB are 00 the len is the 6 bits of this byte
 * 01|000000 00000000 =>  01, the len is 14 byes, 6 bits + 8 bits of next byte
 * 10|000000 [32 bit integer] => if it's 10, a full 32 bit len will follow
 * 11|000000 this means: specially encoded object will follow. The six bits
 *           number specify the kind of object that follows.
 *           See the RDB_ENC_* defines.
 *
 * Lengths up to 63 are stored using a single byte, most DB keys, and may
 * values, will fit inside. */

/* Client flags */
#define CLIENT_PENDING_WRITE (1<<21) /* Client has output to send but a write
                                        handler is yet not installed. */
#define CLIENT_REPLY_OFF (1<<22)   /* Don't send replies to client. */

#define CONFIG_BINDADDR_MAX 16

/* Anti-warning macro... */
#define UNUSED(V) ((void) V)

/*-----------------------------------------------------------------------------
 * Data types
 *----------------------------------------------------------------------------*/

/* The following structure represents a node in the server.ready_keys list,
 * where we accumulate all the keys that had clients blocked with a blocking
 * operation such as B[LR]POP, but received new data in the context of the
 * last executed command.
 *
 * After the execution of every command or script, we run this list to check
 * if as a result we should serve data to clients blocked, unblocking them.
 * Note that server.ready_keys will not have duplicates as there dictionary
 * also called ready_keys in every structure representing a Redis database,
 * where we make sure to remember if a given key was already added in the
 * server.ready_keys list. */

class stClientQuery
{
public:
    stClientQuery(){m_pQuery=NULL;m_iSqlLabel=0;m_bFirstNext=true;};
    ~stClientQuery(){SAFE_DELETE(m_pQuery);};
public:
    TMdbQuery *m_pQuery;
    int m_iSqlLabel;//SQL标识
    bool m_bFirstNext;
};

/* With multiplexing we need to take per-client state.
 * Clients are taken in a linked list. */
class client {
public:
    client();
    ~client();
public:
    uint64_t m_iSessionID;            /* Client incremental unique ID. as session id*/
    int fd;                 /* Client socket. */
    TMdbDatabase  *m_pDB;     //qmdb
    std::vector<stClientQuery *> m_vClientQuery;//sql对象指针容器
    TMdbCspParserMgr      m_tCspParserMgr;//csp解析器管理
    TMdbCSLink m_tTMdbCSLink;//CS 注册信息
    TMdbAvpHead m_tHead;  //avp head 解析
    size_t sentlen;         /* Amount of bytes already sent in the current
                               buffer or object being sent. */
    int flags;              /* Client flags: CLIENT_* macros. */
    int recvlen;                           
    unsigned long m_iThreadId;    //所在的线程ID
    NoOcpParse m_tRecvDataParse;
    NoOcpParse m_tSendDataParse;
    unsigned char m_iFieldCount;
    unsigned char m_iParamCount;
    bool m_bFirstNext;//第一次解析
    char m_iSqlType;
    bool m_bRegister;
    /* Response buffer */
    int bufpos;
    unsigned char recvbuf[PROTO_INLINE_MAX_SIZE];
    unsigned char sendbuf[PROTO_INLINE_MAX_SIZE];
};


/* AIX defines hz to __hz, we don't use this define and in order to allow
 * Redis build on AIX we need to undef it. */
#define NET_IP_STR_LEN 46 /* INET6_ADDRSTRLEN is 46, but we need to be sure */

struct redisServer {
    /* General */
    pid_t pid;                  /* Main process pid. */
    aeEventLoop *el;
    int arch_bits;              /* 32 or 64 depending on sizeof(long) */
    /* Networking */
    int port;                   /* TCP listening port */
    int tcp_backlog;            /* TCP listen() backlog */
    char *bindaddr[CONFIG_BINDADDR_MAX]; /* Addresses we should bind to */
    int bindaddr_count;         /* Number of addresses in server.bindaddr[] */
    int ipfd[CONFIG_BINDADDR_MAX]; /* TCP socket file descriptors */
    int ipfd_count;             /* Used slots in ipfd[] */
    //list *clients;              /* List of active clients */
    list *clients_pending_write; /* There is to write or install handler. */
    client *current_client; /* Current client, only used on crash report */
    char neterr[ANET_ERR_LEN];   /* Error buffer for anet.c */
    uint64_t next_client_id;    /* Next client unique ID. Incremental. */
    int tcpkeepalive;               /* Set SO_KEEPALIVE if non-zero. */
    size_t client_max_querybuf_len; /* Limit for client query buffer length */
    /* Limits */
    unsigned int maxclients;            /* Max number of simultaneous clients */
    int stat_rejected_conn;
    int stat_numconnections;
};

/* networking.c -- Networking and Client related operations */
client *createClient(int fd);
void freeClient(client *c);
void resetClient(client *c);
void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask);
void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask);
void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask);
int listenToPort(int port, int *fds, int *count);
int handleClientsWithPendingWrites(void);
//int clientHasPendingReplies(client *c);
void unlinkClient(client *c);
int writeToClient(int fd, client *c, int handler_installed);
#define clientHasPendingReplies(c) ((c)->bufpos)

int ll2string(char* dst, size_t dstlen, long long svalue);
int string2ll(const char *s, size_t slen, long long *value);
//cs protocol 
class TMdbRedisAgentServer
{
public:
    TMdbRedisAgentServer();
    ~TMdbRedisAgentServer(){};
    int Init(const char* pszDSN,int iAgentPort);//初始化：Attach共享内存，取出进程信息所在位
    void Disconnect(client *ptClient){m_tLinkCtrl.UnRegRemoteLink(ptClient->m_tTMdbCSLink,-1);};
    int DoHeartBeat();
    int DoMain();
public:
    static void* Agent(void* p);
    int Login(client *ptClient) throw (TMdbException);//登陆, 并进行权限验证
    int DoOperation(client *ptClient);//处理各类应用
    int AppErrorAnswer(client *ptClient,int iAnsCode,const char *fmt,...);//错误消息应答
    int AppSendSQL(client *ptClient);//SQL注册，区分动态静态SQL
    int AppSendParam(client *ptClient);//处理发送来的参数
    int AppSendParamArray(client *ptClient);//处理发送来的参数数组
    int AppNextSQLResult(client *ptClient);//响应next操作
    int AppTransAction(client *ptClient);//事务处理
    int AppGetSequence(client *ptClient);//获取序列
    int FillNextResult(client *ptClient,TMdbQuery * pQuery,TMdbCspParser * pSendSQLAns)throw(TMdbException,TMdbCSPException);//填充查询的next 返回值
    int AppGetQmdbInfo(client *ptClient){return 0;};//获取qmdb 信息包
    
    int SendPackageBody(client *ptClient,int iSendLen);// 发送包体
    stClientQuery* GetClientQuery(client * ptClient,int iSqlLabel);
    client * GetFreeAgentClient(int iSockFD,sockaddr_in  tAddr);//获取空闲agent client
    int  GetConnectAgentPort(TMdbCspParser * pParser);
    int Authentication(client *ptClient);//认证
    int SendAnswer(TMdbCspParser * pCspParser,client *ptClient,int AnsCode,const char * sMsg);//发回复信息
    int RecvPackageOnce(client * ptClient);//接收消息包
    
    int CheckClientSequence(client * ptClient);//检测客户端序号

//bin 不使用ocp协议
    int DoOperationBin(client *ptClient);//处理各类应用
    int AppSendSQLBin(client *ptClient);//SQL注册，区分动态静态SQL
    int AppSendParamBin(client *ptClient);//处理发送来的参数
    int AppSendParamArrayBin(client *ptClient);//处理发送来的参数数组
    int AppNextSQLResultBin(client *ptClient);//响应next操作
    int AuthenticationBin(client *ptClient);//认证
    int SendAnswer(client *ptClient,int AnsCode,const char * sMsg);//发回复信息
    int FillNextResult(client *ptClient,TMdbQuery * pQuery,bool &bFirstNext)throw(TMdbException,TMdbCSPException);//填充查询的next 返回值
    int AppTransActionBin(client *ptClient);//事务处理
    int AppGetQmdbInfoBin(client *ptClient);//获取qmdb 信息包
    int AppGetSequenceBin(client *ptClient);//获取序列
    
private:
    TMdbShmDSN* m_pShmDSN;
    TMdbConfig *m_pConfig;
    TMdbDSN    *m_pDsn;
    TMdbProcCtrl m_tProcCtrl;//进程控制
    TMdbLinkCtrl m_tLinkCtrl;//链接管理
	int iPort;
    static int iHeartBeat;
    #define NO_OCP_SQLLABEL_POS SIZE_MSG_BIN_HEAD
    #define NO_OCP_SQL_POS (SIZE_MSG_BIN_HEAD+sizeof(int))
    #define NO_OCP_PARAMCNT_POS (SIZE_MSG_BIN_HEAD+sizeof(int))
    #define NO_OCP_BATCHCNT_POS (SIZE_MSG_BIN_HEAD+sizeof(int))
	
};
int mdbRedisServermain(const char* pszDSN, int iAgentPort); 
extern TMdbRedisAgentServer gMdbAgent;
extern struct redisServer server;
#endif
