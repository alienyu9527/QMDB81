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
#include <sys/uio.h>
#include <math.h>
//#include <pthread.h>
#include "fmacros.h"


/* Return the size consumed from the allocator, for the specified SDS string,*/

client *createClient(int fd) {
    //client *c = (client *)zmalloc(sizeof(client));
    client *c = new(std::nothrow) client();
    if(!c) return NULL;
    /* passing -1 as fd it is possible to create a non connected client.
     * This is useful since all the commands needs to be executed
     * in the context of a client. When commands are executed in other
     * contexts (for instance a Lua script) we need a non connected client. */
    if (fd != -1) {
        anetNonBlock(NULL,fd);
        anetEnableTcpNoDelay(NULL,fd);
        if (server.tcpkeepalive)
            anetKeepAlive(NULL,fd,server.tcpkeepalive);
        if (aeCreateFileEvent(server.el,fd,AE_READABLE,
            readQueryFromClient, c) == AE_ERR)
        {
            close(fd);
            SAFE_DELETE(c);
            return NULL;
        }
    }

    c->m_pDB = new(std::nothrow) TMdbDatabase();
    if(NULL == c->m_pDB) 
    {
        close(fd);
        SAFE_DELETE(c);
        return NULL;
    }
    c->m_tRecvDataParse.InitDataSrc((char*)c->recvbuf);
    c->m_tSendDataParse.InitDataSrc((char*)c->sendbuf);
    //c->id = server.next_client_id++;//as session_id
    c->fd = fd;
    c->bufpos = 0;
    c->sentlen = 0;
    c->recvlen = 0;
    c->flags = 0;
    c->m_iSessionID = 0;
    c->m_bRegister = false;
    c->m_iThreadId = pthread_self();
    //if (fd != -1) listAddNodeTail(server.clients,c);
    return c;
}

/* This function is called every time we are going to transmit new data
 * to the client. The behavior is the following:
 *
 * If the client should receive new data (normal clients will) the function
 * returns C_OK, and make sure to install the write handler in our event
 * loop so that when the socket is writable new data gets written.
 *
 * If the client should not receive new data, because it is a fake client
 * (used to load AOF in memory), a master or because the setup of the write
 * handler failed, the function returns C_ERR.
 *
 * The function may return C_OK without actually installing the write
 * event handler in the following cases:
 *
 * 1) The event handler should already be installed since the output buffer
 *    already contained something.
 * 2) The client is a slave but not yet online, so we want to just accumulate
 *    writes in the buffer but not actually sending them yet.
 *
 * Typically gets called every time a reply is built, before adding more
 * data to the clients output buffers. If the function returns C_ERR no
 * data should be appended to the output buffers. */
int prepareClientToWrite(client *c) {
    if (c->fd <= 0) return C_ERR; /* Fake client for AOF loading. */

    /* Schedule the client to write the output buffers to the socket only
     * if not already done (there were no pending writes already and the client
     * was yet not flagged), and, for slaves, if the slave can actually
     * receive writes at this stage. */
    if (!clientHasPendingReplies(c) &&
        !(c->flags & CLIENT_PENDING_WRITE)
        )
    {
        c->flags |= CLIENT_PENDING_WRITE;
        listAddNodeHead(server.clients_pending_write,c);
    }

    return C_OK;
}

/* -----------------------------------------------------------------------------
 * Low level functions to add more data to output buffers.
 * -------------------------------------------------------------------------- */
/* -----------------------------------------------------------------------------
 * Higher level functions to queue data on the client output buffer.
 * The following functions are the ones that commands implementations will call.
 * -------------------------------------------------------------------------- */


/* Return true if the specified client has pending reply buffers to write to
 * the socket. */
//int clientHasPendingReplies(client *c) {
//    return c->bufpos ;
//}

#define MAX_ACCEPTS_PER_CALL 1000
static void acceptCommonHandler(int fd, int flags, char *ip) {
    client *c;
    if ((c = createClient(fd)) == NULL) {
        TADD_ERROR(ERROR_UNKNOWN,
            "Error registering fd event for the new client: %s (fd=%d)",
            strerror(errno),fd);
        close(fd); /* May be already closed, just ignore errors */
        return;
    }
    /* If maxclient directive is set and this is one client more... close the
     * connection. Note that we create the client instead to check before
     * for this condition, since now the socket is already set in non-blocking
     * mode and we can send an error for free using the Kernel I/O */
    if (server.stat_numconnections >= server.maxclients) {
        //const char *err = "-ERR max number of clients reached\r\n";
        TADD_ERROR(ERROR_UNKNOWN,"max number of clients reached [%d]",server.maxclients);
        /* That's a best effort error message, don't check write errors */
        //if (write(c->fd,err,strlen(err)) == -1) {
            /* Nothing to do, Just to avoid the warning... */
        //}
        server.stat_rejected_conn++;
        freeClient(c);
        return;
    }
    c->m_tTMdbCSLink.iFD = fd;
    SAFESTRCPY(c->m_tTMdbCSLink.m_sClientIP,sizeof(c->m_tTMdbCSLink.m_sClientIP),ip);
    server.stat_numconnections++;
    c->flags |= flags;
}

void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    int cport, cfd, max = MAX_ACCEPTS_PER_CALL;
    char cip[NET_IP_STR_LEN];
    UNUSED(el);
    UNUSED(mask);
    UNUSED(privdata);

    while(max--) {
        cfd = anetTcpAccept(server.neterr, fd, cip, sizeof(cip), &cport);
        if (cfd == ANET_ERR) {
            if (errno != EWOULDBLOCK)
                TADD_ERROR(ERR_NET_ACCEPT,
                    "Accepting client connection: %s", server.neterr);
            return;
        }
        TADD_NORMAL("Accepted %s:%d, client number =[%d] now", cip, cport,server.stat_numconnections);
        acceptCommonHandler(cfd,0,cip);
    }
}
 
/* Close all the slaves connections. This is useful in chained replication
 * when we resync with our own master and want to force all our slaves to
 * resync with us as well. */

/* Remove the specified client from global lists where the client could
 * be referenced, not including the Pub/Sub channels.
 * This is used by freeClient() and replicationCacheMaster(). */
void unlinkClient(client *c) {
    listNode *ln;

    /* If this is marked as current client unset it. */
    if (server.current_client == c) server.current_client = NULL;

    /* Certain operations must be done only if the client has an active socket.
     * If the client was already unlinked or if it's a "fake client" the
     * fd is already set to -1. */
    if (c->fd != -1) {
        /* Remove from the list of active clients. */
        //ln = listSearchKey(server.clients,c);
        //if(!ln) return;
        //listDelNode(server.clients,ln);
        server.stat_numconnections--;

        /* Unregister async I/O handlers and close the socket. */
        aeDeleteFileEvent(server.el,c->fd,AE_READABLE);
        aeDeleteFileEvent(server.el,c->fd,AE_WRITABLE);
        close(c->fd);
        c->fd = -1;
    }

    /* Remove from the list of pending writes if needed. */
    if (c->flags & CLIENT_PENDING_WRITE) {
        ln = listSearchKey(server.clients_pending_write,c);
        if(!ln) return;
        listDelNode(server.clients_pending_write,ln);
        c->flags &= ~CLIENT_PENDING_WRITE;
    }

}

void freeClient(client *c) {
    gMdbAgent.Disconnect(c);
    unlinkClient(c);
    SAFE_DELETE(c);
}

/* Schedule a client to free it at a safe time in the serverCron() function.
 * This function is useful when we need to terminate a client but we are in
 * a context where calling freeClient() is not possible, because the client
 * should be valid for the continuation of the flow of the program. */
/* Write data in output buffers to client. Return C_OK if the client
 * is still valid after the call, C_ERR if it was freed. */
int writeToClient(int fd, client *c, int handler_installed) {
    ssize_t nwritten = 0, totwritten = 0;
    while(clientHasPendingReplies(c)) {
        /*if (c->bufpos > 0)*/ {
            nwritten = write(fd,c->sendbuf+c->sentlen,c->bufpos-c->sentlen);
            if (nwritten <= 0) break;
            c->sentlen += nwritten;
            totwritten += nwritten;

            /* If the buffer was sent, set bufpos to zero to continue with
             * the remainder of the reply. */
            if ((int)c->sentlen == c->bufpos) {
                c->bufpos = 0;
                c->sentlen = 0;
            }
        } 
        /* Note that we avoid to send more than NET_MAX_WRITES_PER_EVENT
         * bytes, in a single threaded server it's a good idea to serve
         * other clients as well, even if a very large request comes from
         * super fast link that is always able to accept data (in real world
         * scenario think about 'KEYS *' against the loopback interface).
         *
         * However if we are over the maxmemory limit we ignore that and
         * just deliver as much data as it is possible to deliver. */
        //server.stat_net_output_bytes += totwritten;
        if (totwritten > NET_MAX_WRITES_PER_EVENT) break;
    }
    if (nwritten == -1) {
        if (errno == EAGAIN) {
            nwritten = 0;
        } else {
            TADD_ERROR(ERR_NET_SEND_FAILED,
                "Error writing to client: %s", strerror(errno));
            freeClient(c);
            return C_ERR;
        }
    }
    if (!clientHasPendingReplies(c)) {
        c->sentlen = 0;
        if (handler_installed) aeDeleteFileEvent(server.el,c->fd,AE_WRITABLE);

        /* Close connection after entire reply has been sent. 
        if (c->flags & CLIENT_CLOSE_AFTER_REPLY) {
            freeClient(c);
            return C_ERR;
        }*/
    }
    return C_OK;
}

/* Write event handler. Just send data to the client. */
void sendReplyToClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    UNUSED(el);
    UNUSED(mask);
    writeToClient(fd,(client*)privdata,1);
}

/* This function is called just before entering the event loop, in the hope
 * we can just write the replies to the client output buffer without any
 * need to use a syscall in order to install the writable event handler,
 * get it called, and so forth. */
int handleClientsWithPendingWrites(void) {
    listIter li;
    listNode *ln;
    int processed = listLength(server.clients_pending_write);

    listRewind(server.clients_pending_write,&li);
    while((ln = listNext(&li))) {
        client *c = (client *)listNodeValue(ln);
        c->flags &= ~CLIENT_PENDING_WRITE;
        listDelNode(server.clients_pending_write,ln);

        /* Try to write buffers to the client socket. */
        if (writeToClient(c->fd,c,0) == C_ERR) continue;

        /* If there is nothing left, do nothing. Otherwise install
         * the write handler. */
        if (clientHasPendingReplies(c) &&
            aeCreateFileEvent(server.el, c->fd, AE_WRITABLE,
                sendReplyToClient, c) == AE_ERR)
        {
            //freeClientAsync(c);
        }
    }
    return processed;
}
//add qmdb process 0729
/*
int AppFillNextResult(client *ptClient,TMdbQuery * pQuery,bool bFirstNext)//throw(TMdbException,TMdbCSPException)
{
    int iRet = 0;
    int iCount = 0;
    int iSetTmp = 0;
    int iValueLen = 0;
    char cDataType = 0;
    bool bHaveNext = true;
    bool bFirstNextTmp = bFirstNext;
    ptClient->m_tSendDataParse.SetData(&iSetTmp,sizeof(char));//have next
    ptClient->m_tSendDataParse.SetData(&iSetTmp,sizeof(short int));//记录个数
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
            }
            for(j = 0; j<ptClient->m_iFieldCount; j++)
            {
                TMdbField& tField = pQuery->Field(j);
                cDataType = tField.GetDataType();
                if(bFirstNextTmp)
                {
                    iValueLen = strlen(tField.GetName())+1;
                    ptClient->m_tSendDataParse.SetData(&cDataType,sizeof(cDataType));//name len
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));//name len
                    ptClient->m_tSendDataParse.SetData(tField.GetName(),iValueLen);//name value
                }
                if(tField.isNULL())
                {//null值
                    iValueLen = 0;//null
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));
                }
                else if(1 == cDataType)
                {
                    long long lData = tField.AsRealnt();
                    iValueLen = sizeof(lData);
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));
                    ptClient->m_tSendDataParse.SetData(&lData,iValueLen);
                }
                else
                {
                    char *pData = tField.AsRealStr();
                    iValueLen = strlen(pData) + 1;
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));
                    ptClient->m_tSendDataParse.SetData(pData,iValueLen);
                }
            }
            bFirstNextTmp = false;
            ptClient->m_bFirstNext = false;
            if(ptClient->m_tSendDataParse.GetSize() > 64*1024 - 4096)
            {//达到最大值
                printf("Up to MAX next[%d]",iCount);
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
    ptClient->m_tSendDataParse.SetData(&iCount,16+5,sizeof(short int));//记录个数
    if(bHaveNext)
    {
        iSetTmp = 1;
        ptClient->m_tSendDataParse.SetData(&iSetTmp,16+4,sizeof(char));//have next
    }
    else
    {
        iSetTmp = 0 ;
        ptClient->m_tSendDataParse.SetData(&iSetTmp,16+4,sizeof(char));//have next
    }
    return iRet;
}

int cmdAppSendParam(client *ptClient)
{
    
    //#define MEM_Str       0x0002
    //#define MEM_Date      0x0800
    //#define MemValueHasAnyProperty(V,P)  (((V)->iFlags&(P))!=0)
    int iRet      = 0;
    int iSqlLabel = 0;
    ptClient->m_tRecvDataParse.GetData(&iSqlLabel,16,sizeof(int));
    //ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
    TMdbQuery * pQuery = ptClient->pQuery;
    //CHECK_OBJ(pQuery);
    int iParamCount = 0;
    int iParamPackageLen = 0;
    int iParamIndex = 0;
    int iParamPos = 0;
    ptClient->m_tRecvDataParse.GetData(&iParamCount,20,sizeof(char));
    iParamPos += 20 + sizeof(char); 
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
                //iRet = AppErrorAnswer(ptClient,-1,"not find param by iParamIndex[%d]",iParamIndex); 
                return iRet;
            }
        }
        iParamPos += iParamPackageLen;
    }
    //#endif
    ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->buf,CSP_APP_SEND_SQL,ptClient->id,ptClient->m_tRecvDataParse.isequence);
    ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
    //ptClient->m_bFirstNext = true;
    if(ptClient->m_iSqlType == TK_SELECT)
    {
        pQuery->Open();
        CHECK_RET(AppFillNextResult(ptClient,pQuery,ptClient->m_bFirstNext),"FillNextResult error");
    }
    else
    {//
        pQuery->Execute();
        int iTmp = 0;
        ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(int));//AVP_SELECT_HAVE_NEXT
        iTmp = pQuery->RowsAffected();
        ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(int));//AVP_AFFECTED_ROW
    
    }
    //ptClient->m_bFirstNext = false;
    ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(short int),true);
    ptClient->m_tSendDataParse.SetSize();
    
    ptClient->bufpos = ptClient->m_tSendDataParse.GetSize();
    
    ptClient->flags |= CLIENT_PENDING_WRITE;
    listAddNodeHead(server.clients_pending_write,ptClient);
    return iRet;
}

int cmdAppSendSQL(client *ptClient)
{
    int iSqlLabel = 0;
    int iRet = 0;
    ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->recvbuf);
    ptClient->m_tRecvDataParse.GetData(&iSqlLabel,16,sizeof(int));
    char *pSQL = ptClient->m_tRecvDataParse.GetDataPtr() + 20;
    //ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
    TMdbQuery * pQuery = ptClient->pQuery;
    //CHECK_OBJ(pQuery);
    pQuery->SetSQL(pSQL,0);//设置SQL
    ptClient->m_iParamCount = pQuery->ParamCount();
    TADD_NORMAL("QuerySqlLabel=[%d],SetSQL(%s),IsDynamic=[%s]",iSqlLabel,pSQL,(ptClient->m_iParamCount != 0)?"TRUE":"FALSE");
    ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->buf,CSP_APP_SEND_SQL,ptClient->id,ptClient->m_tRecvDataParse.isequence);
    ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
    ptClient->m_bFirstNext = true;
    ptClient->m_iFieldCount = pQuery->FieldCount();
    ptClient->m_iSqlType = pQuery->GetSQLType();
    if(ptClient->m_iParamCount != 0)
    {
        //动态SQL
        iSqlLabel = 0;
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(char));//AVP_SELECT_HAVE_NEXT
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(short int));//AVP_AFFECTED_ROW
    }
    else
    {
        //静态SQL
        if(TK_SELECT == ptClient->m_iSqlType)
        {
            pQuery->Open();
            CHECK_RET(AppFillNextResult(ptClient,pQuery,ptClient->m_bFirstNext),"FillNextResult error");
        }
        else
        {
            pQuery->Execute();
            iSqlLabel = 0;
            ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(char));//AVP_SELECT_HAVE_NEXT
            iSqlLabel = pQuery->RowsAffected();
            ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(short int));//AVP_AFFECTED_ROW
        }
    }
    iSqlLabel = 0;
    ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(short int),true);
    ptClient->m_tSendDataParse.SetSize();
    ptClient->bufpos = ptClient->m_tSendDataParse.GetSize();
    ptClient->flags |= CLIENT_PENDING_WRITE;
    listAddNodeHead(server.clients_pending_write,ptClient);
}
*/
//qmdb cs包处理
void processMdbPackage(client *c,int iReadLen){
    server.current_client = c;
    //登录包使用ocp，以后根据协议
    if(IS_OCP(c))
    {//parse ocp
        if(c->recvlen == 0)
        {
            gMdbAgent.RecvPackageOnce(c);
        }
        c->recvlen += iReadLen;
        if(c->recvlen < (int)c->m_tHead.iLen) return;
        c->recvlen = 0;
        gMdbAgent.DoOperation(c);
    }
    else
    {//parse bin
        if(c->recvlen == 0)
        {
            c->m_tRecvDataParse.InitDataSrc((char*)c->recvbuf);
            c->m_tRecvDataParse.Parse();
        }
        
        c->recvlen += iReadLen;
        if(c->recvlen < c->m_tRecvDataParse.GetSize()) return;
        c->recvlen = 0;
        gMdbAgent.DoOperationBin(c);
    }
    /*
    //if(sdslen(c->querybuf))
    {
        int iRet = 0;
        switch(c->m_tRecvDataParse.iCmdCode)
        {
            case CSP_APP_SEND_SQL:
            {
                //接受SQL注册
                iRet=cmdAppSendSQL(c);
                //CHECK_RET(iRet,"AppSendSQL return Value=[%d]",iRet);
                break;
            }
            case CSP_APP_SEND_PARAM: //0k
            {
            
                iRet=cmdAppSendParam(c);
                //CHECK_RET(iRet,"AppSendParam return Value=[%d]",iRet);
                break;
            }
            default:
            {
                iRet = -1;
                printf("cmdcode[%d] not sp\n",c->m_tRecvDataParse.iCmdCode);
                break;
            }
        }
        
        // Only reset the client when the command was executed. 
        if (iRet == C_OK)
            resetClient(c);
    }*/
    server.current_client = NULL;
    
}

//add qmdb process end 0729
void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    client *c = (client*) privdata;
    int nread, readlen;
    UNUSED(el);
    UNUSED(mask);

    readlen = PROTO_IOBUF_LEN /*PROTO_INLINE_MAX_SIZE*/;
    nread = read(fd, c->recvbuf+c->recvlen, readlen);
    if (nread == -1) {
        if (errno == EAGAIN) {
            return;
        } else {
            TADD_NORMAL("Reading from client: %s",strerror(errno));
            freeClient(c);
            return;
        }
    } else if (nread == 0) {
        TADD_NORMAL( "Client closed connection");
        freeClient(c);
        return;
    }
    //printf("read=%d\n",nread);
    processMdbPackage(c,nread);
}


