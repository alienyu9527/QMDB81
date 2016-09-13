/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbAgentServer.cpp
*@Description： 负责管理miniDB的远程访问代理接口
*@Author:       li.shugang jiang.mingjun
*@Date：        2009年05月23日
*@History:
******************************************************************************************/
#include <limits.h>
#include <assert.h>
#include <sys/stat.h>
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"

#include "Helper/mdbErr.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbSocket.h"
#include "Agent/mdbAgentServer.h"
#include "Control/mdbLogInfo.h"
#include "Dbflush/mdbLoadFromDb.h"
#include <sys/ioctl.h>
#include "Agent/mdbInfoCollect.h"
#include "Common/mdbCommons.h"

//namespace QuickMDB{

#define PARTIAL_NEXT_SEND_NUM 1000
#define SESSION_ID_STR "{[%s] SessionID=[%d],Sequence=[%d] "
#define BUFFER_PLUS_COUNT 200 

// //数据包缓存中的冗余字符--一些说明文字比如SESSION_ID_STR
#define DUMP_FILE_SIZE 128//数据抓包落地文件的最大字节
#define RECV_PACKAGE_LEN_DEFAULT 1024*16

//检测ret 并回错误包
#define CHECK_RET_SEND_ANS(_ret,_ptClient,...) if((iRet = _ret)!=0)\
{TADD_ERROR(_ret,__VA_ARGS__);\
AppErrorAnswer(_ptClient,iRet,__VA_ARGS__);\
return iRet;}

#ifdef OS_SUN
#define CHECK_RET_THROW(_ret,_errCode,_sql,...)  if((iRet = _ret)!=0){\
TADD_ERROR(_ret,__VA_ARGS__);\
throw TMdbException(_errCode,_sql,__VA_ARGS__);\
}

#define CHECK_RET_THROW_NOSQL(_ret,_errCode,...)  if((iRet = _ret)!=0){\
TADD_ERROR(_ret,__VA_ARGS__);\
throw TMdbException(_errCode,"",__VA_ARGS__);\
}

#define ERROR_TO_THROW_NOSQL(_errCode,...) \
TADD_ERROR(_errCode,__VA_ARGS__);\
throw TMdbException(_errCode,"",__VA_ARGS__);\
 
#define ERROR_TO_THROW(_errCode,_sql,...) \
TADD_ERROR(_errCode,__VA_ARGS__);\
throw TMdbException(_errCode,_sql,__VA_ARGS__);

#else

#define CHECK_RET_THROW(_ret,_errCode,_sql,FMT,...)  if((iRet = _ret)!=0){\
TADD_ERROR(_ret,FMT,##__VA_ARGS__);\
throw TMdbException(_errCode,_sql,"File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);\
}

#define CHECK_RET_THROW_NOSQL(_ret,_errCode,FMT,...)  if((iRet = _ret)!=0){\
TADD_ERROR(_ret,FMT,##__VA_ARGS__);\
throw TMdbException(_errCode,"","File=[%s], Line=[%d],"FMT,__FILE__,__LINE__,##__VA_ARGS__);\
}


#define ERROR_TO_THROW_NOSQL(_errCode,FMT,...) \
TADD_ERROR(_errCode,FMT,##__VA_ARGS__);\
throw TMdbException(_errCode,"","File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);\
 

#define ERROR_TO_THROW(_errCode,_sql,FMT,...) \
TADD_ERROR(_errCode,FMT,##__VA_ARGS__);\
throw TMdbException(_errCode,_sql,"File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);

#define TO_THROW(_errCode,_sql,FMT,...) \
throw TMdbException(_errCode,_sql,"File=[%s], Line=[%d],"FMT, __FILE__, __LINE__,##__VA_ARGS__);

#endif

int TMdbAgentServer::m_iSessionStep = 0;
int TMdbNoNtcAgentServer::m_iSessionStep = 0;

ClientQuery::ClientQuery()
{
    m_pQuery = NULL;
    m_iSqlLabel = -1;
    m_bFirstNext = true;
}

ClientQuery::~ClientQuery()
{
    SAFE_DELETE(m_pQuery);
}

/******************************************************************************
* 函数名称	:  CalcMaxNextNum
* 函数描述	:  计算最大next次数
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int ClientQuery::CalcMaxNextNum()
{
    int iRet = 0;
    CHECK_OBJ(m_pQuery);
    return iRet;
}


TClientSocket::TClientSocket()
{
    m_clientFd = -1;
    m_pAgentServer = NULL;
}
TClientSocket::~TClientSocket()
{

}
TAgentClientSequence::TAgentClientSequence()
{
    Clear();
}
 TAgentClientSequence::~TAgentClientSequence()
{
     Clear();
}
int TAgentClientSequence::Clear()
{
    m_iLastSendLen = 0;
    memset(m_sLastSendPackage,0,sizeof(m_sLastSendPackage));
    m_iLastSequence = -1;
    return 0;
}


/******************************************************************************
* 函数名称	:  Init
* 函数描述	:  初始化
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TAgentClient::Init()
{
    m_pDB = new(std::nothrow) TMdbDatabase();
    if(m_pDB == NULL)
    {
        TADD_ERROR(ERR_OS_NO_MEMROY," errno=%d,strerror=[%s], Mem Not Enough",errno,strerror(errno));
        return ERR_OS_NO_MEMROY;
    }
    return 0;
}
/******************************************************************************
* 函数名称	:  TAgentClient
* 函数描述	:  构造
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
TAgentClient::TAgentClient()
{
    m_iSessionID = 0;
    m_pDB = NULL;
    m_iThreadId = 0;
    for(int i=0; i<MAX_SEQUENCE_NUM; i++)
    {
        m_pSequenceMgr[i] = NULL;
    }
    //memset(m_sPackageBuffer, 0, sizeof(m_sPackageBuffer));
    m_sPackageBuffer[0] = '\0';
    m_cProcState = PFREE;
    m_pEventInfo = NULL;
    m_iPno = -1;
    m_iPort = -1;
    m_iRecvPos = 0;
    m_bFirstNext = true;
    m_bRegister = false;
    m_iFieldCount = 0;
    m_iParamCount = 0;
    m_iSqlType = 0;
}

void TAgentClient::Print()
{

}
/******************************************************************************
* 函数名称	:  ~TAgentClient
* 函数描述	:  析构
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
TAgentClient::~TAgentClient()
{
    Clear();
}
/******************************************************************************
* 函数名称	:  Clear
* 函数描述	:  清理数据
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TAgentClient::Clear()
{
    for(int i = 0; i<MAX_SEQUENCE_NUM; i++)
    {
        SAFE_DELETE(m_pSequenceMgr[i]);
    }
    for(vector<ClientQuery *>::size_type i=0; i != m_vClientQuery.size(); ++i)
    {
        SAFE_DELETE(m_vClientQuery[i]);
    }
    m_vClientQuery.clear();
    //memset(m_sPackageBuffer, 0, sizeof(m_sPackageBuffer));
    m_sPackageBuffer[0] = '\0';
    m_cProcState = PFREE;
    SAFE_DELETE(m_pDB);
    m_pEventInfo = NULL;
    m_iPno = -1;
    m_iPort = -1;
    m_iRecvPos = 0;
    m_tTMdbCSLink.clear();
    m_bFirstNext = true;
    m_bRegister = false;
}


/******************************************************************************
* 函数名称	:  GetClientQuery
* 函数描述	:  获取ClientQuery
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
ClientQuery *  TAgentClient::GetClientQuery(int iSqlLabel,bool bOutWarn)
{
    ClientQuery * pRetClientQuery = NULL;
    if(iSqlLabel <0 )
    {
        TADD_ERROR(ERROR_UNKNOWN,"invalid SqlLabel[%d] < 0",iSqlLabel);
        return NULL;
    }
    else if(iSqlLabel>=MAX_PREPARE_NUM)
    {
        //TADD_WARNING(" SqlLabel[%d]>=MAX_PREPARE_NUM(128)",iSqlLabel);
    }
    int iSize = (int)m_vClientQuery.size();
    if(bOutWarn && iSize > MAX_PREPARE_NUM)
    {
        TADD_WARNING("To many Prepare Query,num=[%d].",m_vClientQuery.size());
    }
    if(iSqlLabel < iSize)
    {
        pRetClientQuery = m_vClientQuery[iSqlLabel];
        if(iSqlLabel == pRetClientQuery->m_iSqlLabel)
        {
            return pRetClientQuery;
        }
    }
    std::vector<ClientQuery *>::iterator itor = m_vClientQuery.begin();
    for(;itor != m_vClientQuery.end();++itor)
    {
        if(iSqlLabel == (*itor)->m_iSqlLabel)
        {//找到
            pRetClientQuery = (*itor);
			break;
        }
    }
    if(NULL == pRetClientQuery)
    {//没有则，压入新的
        pRetClientQuery = new (std::nothrow)ClientQuery();
        if(NULL == pRetClientQuery)
        {//
            TADD_ERROR(ERROR_UNKNOWN,"no memory....");
            return NULL;
        }
        pRetClientQuery->m_pQuery = m_pDB->CreateDBQuery();
        if(NULL == pRetClientQuery->m_pQuery)
        {
            SAFE_DELETE(pRetClientQuery);
            TADD_ERROR(ERROR_UNKNOWN,"no memory....");
             return NULL;
        }
        pRetClientQuery->m_iSqlLabel = iSqlLabel;
        m_vClientQuery.push_back(pRetClientQuery);
        m_tTMdbCSLink.m_pRemoteLink->iSQLPos = m_vClientQuery.size();
    }
    return pRetClientQuery;
}

int TAgentClient::RemoveClientQuery(int iSqlLabel)
{
    int iRet = 0;
    ClientQuery * pRetClientQuery = NULL;
    if(iSqlLabel <0 )
    {
        TADD_ERROR(ERROR_UNKNOWN, "invalid SqlLabel[%d] < 0",iSqlLabel);
        return iRet;
    }
    std::vector<ClientQuery *>::iterator itor = m_vClientQuery.begin();
    for(;itor != m_vClientQuery.end();++itor)
    {
        if(iSqlLabel == (*itor)->m_iSqlLabel)
        {//找到
            pRetClientQuery = (*itor);
            SAFE_DELETE(pRetClientQuery);
		    m_vClientQuery.erase(itor);
            m_tTMdbCSLink.m_pRemoteLink->iSQLPos = m_vClientQuery.size();
			break;
        }
    }
	return iRet;
}


/******************************************************************************
* 函数名称	:  GetSeqMgrByName
* 函数描述	:  获取TMdbSequenceMgr
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
TMdbSequenceMgr * TAgentClient::GetSeqMgrByName(const char * sSeqName,TMdbConfig * pConfig)
{
    int iRet = 0;
    if(NULL == sSeqName || 0 == sSeqName[0])
    {
        return NULL;
    }

    TMdbSequenceMgr * pRetMdbSequenceMgr = NULL;
    int i = 0;
    for( i=0; i<MAX_SEQUENCE_NUM; i++) //先从缓存查找
    {
        if(m_pSequenceMgr[i] && (m_pSequenceMgr[i]->m_SequenceName[0] != 0) )
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_pSequenceMgr[i]->m_SequenceName,sSeqName) == 0)
            {
                pRetMdbSequenceMgr = m_pSequenceMgr[i];
                return pRetMdbSequenceMgr;
            }
        }
    }

    for( i=0; i<MAX_SEQUENCE_NUM; i++) //缓存中不存在，new
    {
        if(m_pSequenceMgr[i] == NULL)
        {
            m_pSequenceMgr[i] = new(std::nothrow) TMdbSequenceMgr();
            if(m_pSequenceMgr[i] == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN,"errno=%d,strerror=[%s],Mem Not Enough\n",errno,strerror(errno));
                return NULL;
            }
            SAFESTRCPY(m_pSequenceMgr[i]->m_SequenceName,MAX_NAME_LEN ,sSeqName);
            iRet = m_pSequenceMgr[i]->m_Sequence.SetConfig(m_tTMdbCSLink.m_sDSN,sSeqName,pConfig);
            if(iRet < 0)
            {
                m_pSequenceMgr[i]->m_SequenceName[0] = '\0';
                SAFE_DELETE(m_pSequenceMgr[i]);
                return NULL;
            }
            pRetMdbSequenceMgr = m_pSequenceMgr[i];
            break;
        }
    }
    if(i == MAX_SEQUENCE_NUM)
    {
        TADD_ERROR(ERROR_UNKNOWN,"sequence Num=[%d] gather than MAX_SEQUENCE_NUM=[%d]",i+1,MAX_SEQUENCE_NUM);
        pRetMdbSequenceMgr = NULL;
    }
    return pRetMdbSequenceMgr;
}


/******************************************************************************
* 函数名称	:  TAgentFile
* 函数描述	:  构造
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
TAgentFile::TAgentFile()
{
    m_fp = NULL;
    memset(m_sFileName, 0, sizeof(m_sFileName));
}

/******************************************************************************
* 函数名称	:  TAgentFile
* 函数描述	:  析构
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
TAgentFile::~TAgentFile()
{
    SAFE_CLOSE(m_fp);
}

/******************************************************************************
* 函数名称	:  Init
* 函数描述	:  初始化
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TAgentFile::Init()
{

#ifdef WIN32
        snprintf(m_sFileName, sizeof(m_sFileName), "%s\\log\\mdbAgent_package", getenv("QuickMDB_HOME"));
        //判断目录是否存在.
#else
        snprintf(m_sFileName, sizeof(m_sFileName), "%s/log/mdbAgent_package", getenv("QuickMDB_HOME"));
#endif
    return Open(m_sFileName);
}

/******************************************************************************
* 函数名称	:  Open
* 函数描述	: 打开
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TAgentFile::Open(const char* sFileName)
{
    m_fp = fopen(sFileName, "at+");
    if(NULL == m_fp)
    {
        TADD_ERROR(ERR_OS_NO_MEMROY,"open file[%s] error ", sFileName);
        return ERR_OS_NO_MEMROY;
    }
    return 0;
}

/******************************************************************************
* 函数名称	:  Write
* 函数描述	: 写文件
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TAgentFile::Write(const char *buffer, unsigned int iCount)
{
    if(NULL == m_fp || NULL == buffer || iCount <= 0) return 0;
    if(buffer[0] == 0) return 0; 
    if(checkAndBackup() != 0) return 0;
    if(0 == fwrite(buffer, iCount, 1, m_fp))
    {
        TADD_ERROR(ERR_OS_WRITE_FILE,"fwrite() failed, errno=%d, errmsg=[%s].",errno, strerror(errno));
        return ERR_OS_WRITE_FILE;
    }
    fflush(m_fp);
    return 0;
}

/******************************************************************************
* 函数名称	:  checkAndBackup
* 函数描述	: 检查备份
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TAgentFile::checkAndBackup()
{
    char     sFileNameOld[MAX_PATH_NAME_LEN]="";
    struct stat buf;
    //判断文件是否存在
    if (::access(m_sFileName, F_OK) < 0)
    {
        SAFE_CLOSE(m_fp);
        int iRet = Open(m_sFileName);
        if (iRet != 0) return iRet;
    }
    if (stat(m_sFileName,&buf) != 0) 
    {
        TADD_ERROR(ERR_OS_READ_FILE,"stat file[%s] error",m_sFileName);        
        return ERR_OS_READ_FILE;
    }
    if (buf.st_size == -1)
    {
        return ERR_OS_READ_FILE;
    }

    if (buf.st_size >= DUMP_FILE_SIZE*1024*1024)
    {
        SAFE_CLOSE(m_fp);
        TMdbNtcFileOper::Remove(sFileNameOld);
        snprintf(sFileNameOld,sizeof(sFileNameOld),"%s.%s", m_sFileName, "old");

        //将日志文件重命名
        TMdbNtcFileOper::Rename(m_sFileName,sFileNameOld);

        TMdbNtcFileOper::Remove(m_sFileName);
        return Open(m_sFileName);
    }
    return 0;
}



/******************************************************************************
* 函数名称	:  TMdbNoNtcAgentServer
* 函数描述	:  构造
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
TMdbNoNtcAgentServer::TMdbNoNtcAgentServer()
{
    m_pShmDSN = NULL;
    m_pConfig = NULL;
    m_pDsn    = NULL;
    m_clientFd = -1;
    for(int i=0; i<MAX_CS_THREAD_COUNTS; i++)
    {
        m_ptClient[i] = NULL;
    }
    m_iClientThreadNum = 0;
}


TMdbNoNtcAgentServer::~TMdbNoNtcAgentServer()
{
    for(int i=0; i<MAX_CS_THREAD_COUNTS; i++)
    {
        SAFE_DELETE(m_ptClient[i]);
    }
    tMutex.Destroy();
}

/******************************************************************************
* 函数名称  :  Init()
* 函数描述  :  初始化：Attach共享内存，取出进程信息所在位置
* 输入      :  pszDSN, 锁管理区所属的DSN
* 输出      :  无
* 返回值    :  0 - 成功!0 -失败
* 作者      :  li.shugang
*******************************************************************************/
int TMdbNoNtcAgentServer::Init(const char* pszDSN, int iAgentPort)
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
    CHECK_RET(m_tLinkCtrl.ClearRemoteLink(iAgentPort),"m_tLinkCtrl.ClearRemoteLink(%d) failed.",iAgentPort);
	CHECK_RET(m_tLinkCtrl.ClearCntNumForPort(iAgentPort),"m_tLinkCtrl.ClearCntNumForPort(%d) failed.",iAgentPort);
    CHECK_RET(m_tAgentFile.Init()," m_tAgentFile.Init failed.");
    tMutex.Create();
    TADD_FUNC("TMdbNoNtcAgentServer::Init(%s) : Finish.", pszDSN);
    return iRet;
}

/******************************************************************************
* 函数名称  :  PreSocket()
* 函数描述  :  初始化Socket，并进行监听
* 输入      :  端口号
* 输出      :  无
* 返回值    :  成功返回0，否则返回负数返回码
* 作者      :  li.shugang
*******************************************************************************/
int TMdbNoNtcAgentServer::PreSocket(int iAgentPort)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_RET(m_oSocket.Create(),"Create Socket failed.");
    TADD_DETAIL("iAgentPort=%d.", iAgentPort);
    CHECK_RET(m_oSocket.Bind(iAgentPort),"Bind socket failed,iRet = [%d].",iRet);
    CHECK_RET( m_oSocket.Listen(),"Listen Socket failed=[%d].",iRet);
    TADD_NORMAL("PreSocket(%s:%d) : Waiting for connections and data......\n", m_pDsn->sLocalIP, iAgentPort);
    return iRet;
}


/******************************************************************************
* 函数名称  :  Start()
* 函数描述  :  建立监听，并针对链接进行处理
* 输入      :  pszName, 进程名
* 输出      :  无
* 返回值    :  如果可以链接返回0, 否则返回负数返回码
* 作者      :  li.shugang
*******************************************************************************/
int TMdbNoNtcAgentServer::Start(const char* pszName)
{
    TADD_FUNC("Begin[%s].",pszName);
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
        int iReady = m_oSocket.Select();//0.2S监视一次
        if(iReady == 0)
        {
            continue;
        }
        else if(iReady < 0)
        {
            TADD_ERROR(ERR_NET_SELECT,"select() failed. errno=%d, errmsg=%s.", errno, strerror(errno));
            break;
        }
        TADD_NORMAL("agentserver socket select() OK!,iReady=[%d].",iReady);
        if(false == m_oSocket.FdIsSet())
        {
            TADD_ERROR(ERR_NET_SET_ATTR,"FdIsSet() failed. errno=%d, errmsg=%s.", errno, strerror(errno));
            break;
        }
        //接受客户端连接
        m_clientFd = m_oSocket.Accept();
        if(m_clientFd < 0)
        {
            TADD_ERROR(ERR_NET_ACCEPT,"accept() failed. errno=%d, errmsg=%s,Connectfd=%d.", errno, strerror(errno),m_clientFd);
            break;
        }
        if(m_iClientThreadNum >= MAX_CS_THREAD_COUNTS)
        {
            TADD_WARNING("Client Thread Num=[%d] >= MAX_THREAD_COUNTS=[%d].", m_iClientThreadNum, MAX_CS_THREAD_COUNTS);
            close(m_clientFd);
            continue;
        }
        //启动一个新的线程处理连接请求
        TADD_NORMAL("Find a new connection from=[%s], ClientThreadNumber=[%d]", 
                                        inet_ntoa(m_oSocket.m_tAddTmp.sin_addr),m_iClientThreadNum);
        if(StartNewPthread()<0)
        {
            close(m_clientFd);
        }
        else
        {
            m_iClientThreadNum++;
        }
    }
    m_oSocket.CloseListenSocket();
    TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Process[mdbAgent] Exist.");
    return 0;
}

/******************************************************************************
* 函数名称	:  StartNewPthread
* 函数描述	:  开启新线程
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbNoNtcAgentServer::StartNewPthread()
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
    CodeBlock::Within w(durThreadMain);
    TClientSocket * pTempClientSocket = new(std::nothrow) TClientSocket();//交给各个线程内部销毁
    if(pTempClientSocket == NULL)
    {
    	TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new TClientSocket");
		return ERR_OS_NO_MEMROY;
    }
    pTempClientSocket->m_clientFd = m_clientFd;
    pTempClientSocket->m_oSocket = m_oSocket;
    pTempClientSocket->m_pAgentServer = this;
    if(0 != pthread_create(&tID, &thread_attr, agent, pTempClientSocket))
    {
        TADD_ERROR(ERR_OS_SET_THREAD_ATTR,"Can't pthread_create(), errno=[%d], errmsg=[%s].",errno, strerror(errno));
        return ERR_OS_CREATE_THREAD;
    }
    TADD_NORMAL("m_clientFd=[%d],ThreadId=[%lu] Create New Thread OK", m_clientFd,tID);
    TADD_FUNC("end.");
    return 0;
}

/******************************************************************************
* 函数名称	:  agent
* 函数描述	:  代理例程
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void* TMdbNoNtcAgentServer::agent(void* p)
{
    TClientSocket * pClientSocket = (TClientSocket * )p;
    int iSockFD = pClientSocket->m_clientFd;
    TMdbNoNtcAgentServer* pThread  = pClientSocket->m_pAgentServer;
    struct sockaddr_in  tAddr = pClientSocket->m_oSocket.m_tAddTmp;
    SAFE_DELETE(pClientSocket);//销毁
    pthread_detach(pthread_self());//线程自动释放资源
    pThread->svc(iSockFD,tAddr);
    return 0;
}
/******************************************************************************
* 函数名称	:  threadExit
* 函数描述	:  线程结束的收尾工作
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TMdbNoNtcAgentServer::threadExit(TAgentClient *&ptClient,bool bCloseSocket)
{
    m_iClientThreadNum--;
    if(ptClient != NULL)
    {
        if(bCloseSocket)
        {
            ptClient->m_ClientSocket.Close();
        }
        ptClient->Clear();
        int i = 0;
        for(i=0; i<MAX_CS_THREAD_COUNTS; i++)
        {
            if(m_ptClient[i] == ptClient)
            {
                SAFE_DELETE(ptClient);
                ptClient = NULL;
                m_ptClient[i] = NULL;
                break;
            }
        }
    }
#ifndef OS_LINUX
    int pthread_kill_err;
    pthread_kill_err = pthread_kill(pthread_self(),0);
    if(pthread_kill_err == ESRCH)
    {
        TADD_NORMAL("ThreadId=[%ld] is Not Exist",pthread_self());
        return;
    }
    else
    {
        TADD_NORMAL("ThreadId=[%ld] call pthread_exit!!",pthread_self());
        pthread_exit(NULL);
    }
#endif
}

/******************************************************************************
* 函数名称	:  GetFreeAgentClient
* 函数描述	:  获取空闲agent client
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
	TAgentClient * TMdbNoNtcAgentServer::GetFreeAgentClient(int iSockFD,sockaddr_in	tAddr)
	{
		TAgentClient * pRetClient = NULL;
		if(tMutex.Lock(true) != 0)
		{
			TADD_ERROR(ERR_OS_MUTEX_LOCK,"Lock Faild");
			return NULL;
		}
		//tMutex.Lock(true);
		int iRet = 0;
		int i = 0;
		for( i=0; i<MAX_CS_THREAD_COUNTS; i++)
		{
			if(m_ptClient[i] == NULL)
			{
				m_ptClient[i] = new(std::nothrow) TAgentClient();
				if(m_ptClient[i] == NULL)
				{
					TADD_ERROR_CODE(ERR_OS_NO_MEMROY,"errno=%d,strerror=[%s]",errno,strerror(errno));
					threadExit(m_ptClient[i],true);
				}
				iRet = m_ptClient[i]->Init();
				if(iRet < 0)
				{
					TADD_ERROR(ERROR_UNKNOWN,"Client Init() Failed");//释放对象......
					threadExit(m_ptClient[i],true);
				}
				pRetClient = m_ptClient[i];
				pRetClient->m_iThreadId = pthread_self();
				break;
			}
		}
		if(tMutex.UnLock(true) != 0)
		{
			TADD_ERROR(ERR_OS_MUTEX_UNLOCK,"UnLock Faild");
			return NULL;
		}
		//tMutex.UnLock(true);
		if(i == MAX_CS_THREAD_COUNTS)
		{
			TADD_ERROR(ERR_OS_CREATE_THREAD,"Client Thread Num=[%d] gather than MAX_THREAD_COUNTS=[%d]",i,MAX_CS_THREAD_COUNTS);
		}
		if(NULL != pRetClient)
		{
			TADD_NORMAL("m_ptClient[i=%d]",i);
			pRetClient->m_tTMdbCSLink.iFD = iSockFD;
			pRetClient->m_ClientSocket.SetSocketID(iSockFD);
	
#ifndef _SUN
			int ul=0;
			int iUnBlock = ioctl(pRetClient->m_tTMdbCSLink.iFD, FIONBIO, &ul);//非阻塞设置
			if(iUnBlock == -1)
			{
				TADD_ERROR(ERR_NET_SELECT,"SET ioctl error,error=%d,msg=%s",errno,strerror(errno));
			}
#endif
			pRetClient->m_tTMdbCSLink.tAddr = tAddr;
		}
		return pRetClient;
	}

int TMdbNoNtcAgentServer::GetConnectAgentPort(TMdbCspParser * pParser)
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

/******************************************************************************
* 函数名称	: Authentication 
* 函数描述	:  认证
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbNoNtcAgentServer::Authentication(TAgentClient *ptClient)
{
    TADD_FUNC("Begin.");
    int iRet = 0;
	int iRtnPort = 0;
    CHECK_RET(RecvPackageOnce(ptClient),"Authentication RecvPackage error,ptClient->iFD=[%d]",ptClient->m_tTMdbCSLink.iFD);
    CHECK_RET(CheckClientSequence(ptClient),"CheckClientSequence error.iRet=[%d]",iRet);//登陆认证的时候不能接受重发
    if(CSP_APP_LOGON != ptClient->m_tHead.iCmdCode)
    {//非登陆包
        CHECK_RET_SEND_ANS(ERR_CSP_PARSER_LOGON,ptClient,"want to recv logon package,but recv[%d]",ptClient->m_tHead.iCmdCode);
    }
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_LOGON,true);
        CHECK_OBJ(pParser);
        CHECK_RET(pParser->DeSerialize((unsigned char*)(ptClient->m_sRecvPackage),ptClient->m_tHead.iLen),"parser logon package error");
       // pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
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
		if(NULL != pParser->FindExistAvpItem(pParser->m_pRootAvpItem,AVP_LOW_PRIORITY))
        {//有sql-priority
            ptClient->m_tTMdbCSLink.m_iLowPriority = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_LOW_PRIORITY);
			if(ptClient->m_tTMdbCSLink.m_iLowPriority == 1)
			{
				TADD_NORMAL("Low priority link request.");
			}
        }

		
		//第一次使用非ntc连接的时候，选择合适的端口
		
		iRtnPort = GetConnectAgentPort(pParser);

		

		//如果不需要重新换端口，则可以继续操作
		//
		if(pParser->GetINT32Value(pParser->m_pRootAvpItem,AVP_CON_NUM) == 1 || iRtnPort == 0)
		{
			//TADD_NORMAL("ptClient->m_tTMdbCSLink.m_iLowPriority = [%d]", ptClient->m_tTMdbCSLink.m_iLowPriority);
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
    //生成SessionID
    CHECK_RET(tMutex.Lock(true),"Lock Faild");
    //tMutex.Lock(true);
    TMdbNoNtcAgentServer::m_iSessionStep++;
    if(TMdbNoNtcAgentServer::m_iSessionStep >= INT_MAX)
    {
        m_iSessionStep = 1;
    }
    ptClient->m_iSessionID = m_iSessionStep;
    CHECK_RET(tMutex.UnLock(true),"UnLock Faild");
    //tMutex.UnLock(true);
    //回复登陆成功
    ptClient->m_bRegister = true;
    TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_LOGON,false);
	//端口号写进去
	//如果端口号不需要换，填0，否则填端口号
	//TADD_NORMAL("the send to client port:%d",iRtnPort);
	pParser->SetItemValue(pParser->m_pRootAvpItem,AVP_ANSWER_PORT,iRtnPort);
    pParser->SetVersion(ptClient->m_tTMdbCSLink.m_iUseOcp);
    CHECK_RET(SendAnswer(pParser,ptClient,0,"LOGIN_OK"),"SendAnswer[LOGIN_OK],failed");
    return iRet;
}


/******************************************************************************
* 函数名称  :  svc()
* 函数描述  :  数据交互处理
* 输入      :  iSockFD, 客户端套接字id;tAddr,套接字地址
* 输出      :  无
* 返回值    :  成功返回0, 否则返回负数返回码
* 作者      :  li.shugang
*******************************************************************************/
int TMdbNoNtcAgentServer::svc(int iSockFD,sockaddr_in  tAddr)
{
    TADD_NORMAL("svc(iSockFD=%d) Begin",iSockFD);
    TAgentClient* ptClient = GetFreeAgentClient(iSockFD,tAddr);//获取空闲agent client
    CHECK_OBJ(ptClient);
    TADD_NORMAL("ThreadId=[%lu], SocketID=[%d], ClientIP=[%s]\n",ptClient->m_iThreadId ,ptClient->m_tTMdbCSLink.iFD,inet_ntoa(ptClient->m_tTMdbCSLink.tAddr.sin_addr));
    int iRet = 0;
    do
    {
        //clock_t tStart = clock();
        
        struct timeval tCurTime,tEndTime;
        time_t costTime = 0;
        tCurTime = m_pDsn->tCurTime;
        CHECK_RET_BREAK(Authentication(ptClient),"Authentication failed.");//认证
        //clock_t tFinish = clock();
        //double duration = (double)(tFinish - tStart)/CLOCKS_PER_SEC;
        tEndTime = m_pDsn->tCurTime;
        costTime = tEndTime.tv_sec - tCurTime.tv_sec;
        if(costTime >= 3)
        {//登录请求大于3秒
            TADD_WARNING("ThreadID=[%lu], ClientIP=[%s],SequenceID=[%d] Login Time[%d] > 3s ",ptClient->m_iThreadId,inet_ntoa(ptClient->m_tTMdbCSLink.tAddr.sin_addr), ptClient->m_tHead.GetSequence(),costTime);
        }
        TADD_NORMAL("ThreadID=[%lu],ClientIP=[%s],SequenceID=[%d],SocketID=[%d],Protocol=[%d] Login Success!! ",
                ptClient->m_iThreadId,inet_ntoa(ptClient->m_tTMdbCSLink.tAddr.sin_addr),ptClient->m_tHead.GetSequence(),iSockFD,ptClient->m_tTMdbCSLink.m_iUseOcp);
        // 死循环接收数据
        while(true)
        {
            //状态变更时落地之前的数据包
            char cProcState = m_tProcCtrl.GetProcState();
            if(PSTOP == cProcState) 
            {
                break;
            }
            
            if(PDUMP !=  cProcState
                && PDUMP == ptClient->m_cProcState) 
            {
                ptClient->m_cProcState = cProcState;
                m_tAgentFile.Write(ptClient->m_sPackageBuffer, strlen(ptClient->m_sPackageBuffer));
                memset(ptClient->m_sPackageBuffer, 0, sizeof(ptClient->m_sPackageBuffer));
            }
        
            iRet = Run(ptClient);
            if(iRet < 0)
            {
                TADD_NORMAL("Run(m_iThreadId = %lu) return value[%d]\n",ptClient->m_iThreadId,iRet);
                if(iRet == ERR_NET_RECV_FAILED || iRet == ERR_NET_SEND_FAILED ||iRet == ERR_NET_SOCKET_CLOSE )
                {
                    break;
                }
            }
            
        }
    }while(0);
    //退出时落地数据包
    m_tAgentFile.Write(ptClient->m_sPackageBuffer, strlen(ptClient->m_sPackageBuffer));
    m_tLinkCtrl.UnRegRemoteLink(ptClient->m_tTMdbCSLink,iPort);
    threadExit(ptClient,true);
    return iRet;
}

/******************************************************************************
* 函数名称	:  PrintSendMsg
* 函数描述	:  打印发送信息
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbNoNtcAgentServer::PrintSendMsg(TAgentClient *ptClient)
{
    unsigned char *caTemp = ptClient->m_sSendPackage;
    int iMsgLen = ptClient->m_tHead.iLen;
    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_KICK_LINK:
        {
            TADD_NORMAL(": /*--------SendDeleteSessionCSPPackage--------*/");
            break;
        }
        case CSP_APP_SEND_SQL:
        {
            TADD_NORMAL(": /*----------SendSQLResultCSPPackage----------*/");
            break;
        }
        case CSP_APP_SEND_PARAM:
        {
            TADD_NORMAL(": /*----------SendSQLParameterCSPPackage----------*/");
            break;
        }
         case CSP_APP_SEND_PARAM_ARRAY:
        {
            TADD_NORMAL(": /*----------SendSQLParameArrayCSPPackage----------*/");
            break;
        }
        case CSP_APP_ACTION:
        {
            TADD_NORMAL(": /*----------SendACTIONCSPPackage--------------*/");
            break;

        }
        case CSP_APP_ADJUST_LOG:
        {
            TADD_NORMAL(": /*----------SendModifyLogLevelPackage--------*/");
            break;
        }
        case CSP_APP_NEXT:
        {
            TADD_NORMAL(": /*----------SendSQLResultCSPPackage-----------");
            break;
        }
        case CSP_APP_C_TABLE:
        {
            TADD_NORMAL(": /*----------SendCreateTableCSPPackage--------*/");
            break;
        }
        case CSP_APP_C_USER:
        {
            TADD_NORMAL(": /*----------SendCreateUserCSPPackage---------*/");
            break;
        }
        case CSP_APP_D_TABLE:
        {
            TADD_NORMAL(": /*----------SendDropTableCSPPackage----------*/");
            break;
        }
        case CSP_APP_D_USER:
        {
            TADD_NORMAL(": /*----------SendDropUserCSPPackage-----------*/");
            break;
        }
        case CSP_APP_GET_SEQUENCE:
        {
            TADD_NORMAL(": /*----------SendGetSequenceCSPPackage--------*/");
            break;
        }
        case CSP_APP_LOGON:
        {
            TADD_NORMAL(": /*----------SendLogOnCSPPackage--------------*/");
            break;
        }
        case CSP_APP_ERROR:
        {
            TADD_NORMAL(": /*----------SendERRORCSPPackage--------------*/");
            break;
        }
        case CSP_APP_QMDB_INFO:
        {
            TADD_NORMAL(": /*----------SendQMDBINFOPackage--------------*/");
            break;
        }
        default:
        {
            TADD_ERROR(ERR_CSP_INVALID_SEND_PACKAGE,"Send Invalid CSPPackage,CommandCode=[%d]",ptClient->m_tHead.iCmdCode);
            break;
        }
    }
    PrintMsg(ptClient,caTemp,iMsgLen);
    return 0;
}

int TMdbNoNtcAgentServer::PrintRecvMsg(TAgentClient *ptClient)
{
    unsigned char *caTemp = ptClient->m_sRecvPackage;
    int iMsgLen = ptClient->m_tHead.iLen;
    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_KICK_LINK:
        {
            TADD_NORMAL(": /*--------RecvDeleteSessionCSPPackage--------*/");
            break;
        }
        case CSP_APP_SEND_SQL:
        {
            TADD_NORMAL(": /*----------RecvSQLStatementPackage----------*/");
            break;
        }
        case CSP_APP_SEND_PARAM:
        {
            TADD_NORMAL(": /*----------RecvSQLParameterCSPPackage----------*/");
            break;
        }
        case CSP_APP_ACTION:
        {
            TADD_NORMAL(": /*-----------RecvTransactionPackage----------*/");
            break;
        }
        case CSP_APP_ADJUST_LOG:
        {
            TADD_NORMAL(": /*----------RecvModifyLogLevelPackage--------*/");
            break;
        }
        case CSP_APP_NEXT:
        {
            TADD_NORMAL(": /*----------RecvNextSQLOperPackage-----------*/");
            break;
        }
        case CSP_APP_C_TABLE:
        {
            TADD_NORMAL(": /*----------RecvCreateTableCSPPackage--------*/");
            break;
        }
        case CSP_APP_C_USER:
        {
            TADD_NORMAL(": /*----------RecvCreateUserCSPPackage---------*/");
            break;
        }
        case CSP_APP_D_TABLE:
        {
            TADD_NORMAL(": /*----------RecvDropTableCSPPackage----------*/");
            break;
        }
        case CSP_APP_D_USER:
        {
            TADD_NORMAL(": /*----------RecvDropUserCSPPackage-----------*/");
            break;
        }
        case CSP_APP_GET_SEQUENCE:
        {
            TADD_NORMAL(": /*----------RecvGetSequenceCSPPackage--------*/");
            break;
        }
        case CSP_APP_SEND_PARAM_ARRAY:
        {
            TADD_NORMAL(": /*----------RecvSQLParameArrayCSPPackage--------*/");
            break;
        }
        case CSP_APP_QMDB_INFO:
        {
            TADD_NORMAL(": /*----------RecvGetQMDBInfoCSPPackage--------*/");
            break;
        }
        case CSP_APP_LOGON:
        {
            TADD_NORMAL(": /*----------RecvLogOnCSPPackage--------------*/");
            break;
        }
        default:
        {
            TADD_ERROR(ERR_CSP_INVALID_RCV_PACKAGE,"Recve Invalid CSPPackage,CommandCode=[%d]",ptClient->m_tHead.iCmdCode);
            break;
        }
    }
    PrintMsg(ptClient,caTemp,iMsgLen);
    return 0;
}

/******************************************************************************
* 函数名称	:  PrintMsg
* 函数描述	:  打印发送信息
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TMdbNoNtcAgentServer::PrintMsg(TAgentClient *ptClient,unsigned char *caTemp,int iMsgLen)
{
    char sTemp[64]= {0};
    TADD_NORMAL(":iMsgLen=[%d],Sequence=[%d]",iMsgLen,ptClient->m_tHead.GetSequence());
    for(int i=0; i<iMsgLen; ++i)
    {
        if(i%16 == 0&& i!=0 )
        {
            TADD_NORMAL(":%s",sTemp);
            memset(sTemp,'\0',64);
        }
        snprintf(&sTemp[strlen(sTemp)],sizeof(sTemp),"%02x ", caTemp[i]);
    }
    TADD_NORMAL(":%s",sTemp);
}

/******************************************************************************
* 函数名称	:  DumpSendPackage
* 函数描述	:  发送数据包
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TMdbNoNtcAgentServer::DumpSendPackage(TAgentClient *ptClient)
{
    unsigned int iSize = sizeof(ptClient->m_sPackageBuffer);
    unsigned int iCount = strlen(ptClient->m_sPackageBuffer);
    char *pPackageBuffer = ptClient->m_sPackageBuffer;
    int iMsgLen = ptClient->m_tHead.iLen;

    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_KICK_LINK:
        {
            strncat(pPackageBuffer, " SendDeleteSessionCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_SEND_SQL:
        {
            strncat(pPackageBuffer, " SendSQLResultCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_SEND_PARAM:
        {
            strncat(pPackageBuffer, " SendSQLParameterCSPPackage ", iSize-iCount);
            break;
        }
         case CSP_APP_SEND_PARAM_ARRAY:
        {
            strncat(pPackageBuffer, " SendSQLParameArrayCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_ACTION:
        {
            strncat(pPackageBuffer, " SendACTIONCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_ADJUST_LOG:
        {
            strncat(pPackageBuffer, " SendModifyLogLevelPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_NEXT:
        {
            strncat(pPackageBuffer, " SendSQLResultCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_C_TABLE:
        {
            strncat(pPackageBuffer, " SendCreateTableCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_C_USER:
        {
            strncat(pPackageBuffer, " SendCreateUserCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_D_TABLE:
        {
            strncat(pPackageBuffer, " SendDropTableCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_D_USER:
        {
            strncat(pPackageBuffer, " SendDropUserCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_GET_SEQUENCE:
        {
            strncat(pPackageBuffer, " SendGetSequenceCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_LOGON:
        {
            strncat(pPackageBuffer, " SendLogOnCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_ERROR:
        {
            strncat(pPackageBuffer, " SendERRORCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_QMDB_INFO:
        {
            strncat(pPackageBuffer, " SendQMDBINFOPackage ", iSize-iCount);
            break;
        }
        default:
        {
            break;
        }
    }
    
    for(int i=0; i<iMsgLen; ++i)
    {
        iCount = strlen(pPackageBuffer);
        snprintf(&pPackageBuffer[iCount], iSize-iCount, "%02x ", ptClient->m_sSendPackage[i]);
    }
    return;

}

/******************************************************************************
* 函数名称	:  DumpRecvPackage
* 函数描述	:  接收数据包
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TMdbNoNtcAgentServer::DumpRecvPackage(TAgentClient *ptClient)
{
    unsigned int iSize = sizeof(ptClient->m_sPackageBuffer);
    unsigned int iCount = strlen(ptClient->m_sPackageBuffer);
    char *pPackageBuffer = ptClient->m_sPackageBuffer;
    int iMsgLen = ptClient->m_tHead.iLen;
    
    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_KICK_LINK:
        {
            strncat(pPackageBuffer, " RecvDeleteSessionCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_SEND_SQL:
        {
            strncat(pPackageBuffer, " RecvSQLStatementPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_SEND_PARAM:
        {
            strncat(pPackageBuffer, " RecvSQLParameterCSPPackage ", iSize-iCount);
            break;
        }
         case CSP_APP_SEND_PARAM_ARRAY:
        {
            strncat(pPackageBuffer, " RecvSQLParameArrayCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_ACTION:
        {
            strncat(pPackageBuffer, " RecvTransactionPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_ADJUST_LOG:
        {
            strncat(pPackageBuffer, " RecvModifyLogLevelPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_NEXT:
        {
            strncat(pPackageBuffer, " RecvNextSQLOperPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_C_TABLE:
        {
            strncat(pPackageBuffer, " RecvCreateTableCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_C_USER:
        {
            strncat(pPackageBuffer, " RecvCreateUserCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_D_TABLE:
        {
            strncat(pPackageBuffer, " RecvDropTableCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_D_USER:
        {
            strncat(pPackageBuffer, " RecvDropUserCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_GET_SEQUENCE:
        {
            strncat(pPackageBuffer, " RecvGetSequenceCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_LOGON:
        {
            strncat(pPackageBuffer, " RecvLogOnCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_QMDB_INFO:
        {
            strncat(pPackageBuffer, " RecvGetQMDBInfoCSPPackage ", iSize-iCount);
            break;
        }
        default:
        {
            break;
        }
    }
    
    for(int i=0; i<iMsgLen; ++i)
    {
        iCount = strlen(pPackageBuffer);
        snprintf(&pPackageBuffer[iCount], iSize-iCount, "%02x ", ptClient->m_sRecvPackage[i]); 
        
    }
    return;
    
}



/******************************************************************************
* 函数名称	:  DumpPackage
* 函数描述	: 数据包落地
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TMdbNoNtcAgentServer::DumpPackage(TAgentClient *ptClient, bool bSendFlag)
{
    if(NULL == ptClient) return;
    unsigned int iSize = sizeof(ptClient->m_sPackageBuffer);
    unsigned int iCount = strlen(ptClient->m_sPackageBuffer);
    char *pPackageBuffer = ptClient->m_sPackageBuffer;
    int iMsgLen = ptClient->m_tHead.iLen;
    
    if(iCount + iMsgLen*2 + iMsgLen/2 + BUFFER_PLUS_COUNT >= iSize-1)
    {//计算buffer总大小,BUFFER_PLUS_COUNT目前足够大
        m_tAgentFile.Write(pPackageBuffer, iCount);
        memset(pPackageBuffer, 0, iSize);
        iCount = 0;
    }
    
    //包开始{ [%s]SessionID=[%d]Sequence=[%d]
    char strCurrentDateTime[15] = {0};
    TMdbDateTime::GetCurrentTimeStr(strCurrentDateTime);
    snprintf(&pPackageBuffer[iCount], iSize-iCount,SESSION_ID_STR, 
        strCurrentDateTime, ptClient->m_tHead.iSessionId, ptClient->m_tHead.isequence);
        
    //数据信息
    if(bSendFlag)
    {
        DumpSendPackage(ptClient);
    }
    else
    {
        DumpRecvPackage(ptClient);
    }
    //包结束
    iCount = strlen(pPackageBuffer);
    strncat(pPackageBuffer, "}\n", iSize-iCount);
    
    if(iCount+2 == iSize-1)
    {
        m_tAgentFile.Write(pPackageBuffer, iCount+2);
        memset(pPackageBuffer, 0, iSize);
    }
    
    return;
    
}

/******************************************************************************
* 函数名称	:  AppSendSQL
* 函数描述	:  SQL注册
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbNoNtcAgentServer::AppSendSQL(TAgentClient *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    int iSqlFlag = 0;
    char * sSQL   = NULL;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//解析
        //pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
        iSqlLabel = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        sSQL	  = pParser->GetStringValue(pParser->m_pRootAvpItem,AVP_SQL_STATEMENT);
        if(NULL != pParser->FindExistAvpItem(pParser->m_pRootAvpItem,AVP_SQL_FLAG))
        {//有sql-flag
            iSqlFlag = pParser->GetINT32Value(pParser->m_pRootAvpItem,AVP_SQL_FLAG);
        }
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel,true);//根据SQLLABEL，获取client
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
int TMdbNoNtcAgentServer::AppSendParam(TAgentClient *ptClient)
{
    int iRet      = 0;
    int iSqlLabel = 0;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_PARAM,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);
       // pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
        iSqlLabel         = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
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


/******************************************************************************
* 函数名称	:  AppSendParamArray
* 函数描述	:  处理发送来的参数数组
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbNoNtcAgentServer::AppSendParamArray(TAgentClient *ptClient)
{
    TADD_FUNC("Start,AppSendParamArray");
    int iRet = 0;
    int iSqlLabel = 0;
     try
     {
         TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_PARAM_ARRAY,true);
         CHECK_OBJ(pParser);
         pParser->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);
         //pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
         IS_LOG(2){pParser->Print();}
         iSqlLabel         = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
         ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
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
int TMdbNoNtcAgentServer::FillNextResult(TAgentClient *ptClient,TMdbQuery * pQuery,TMdbCspParser * pSendSQLAns)throw(TMdbException,TMdbCSPException)
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

int TMdbNoNtcAgentServer::AppNextSQLResult(TAgentClient *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_NEXT,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);
        //pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
        iSqlLabel  = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
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

/******************************************************************************
* 函数名称	:  AppTransAction
* 函数描述	:  事务处理
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbNoNtcAgentServer::AppTransAction(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        TMdbCspParser * pParserSend = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_ACTION,true);
        CHECK_OBJ(pParserSend);
        pParserSend->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//解析
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
int TMdbNoNtcAgentServer::AppGetSequence(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        TMdbCspParser * pParserSend = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_GET_SEQUENCE,true);
        CHECK_OBJ(pParserSend);
        pParserSend->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//解析
        //pParserSend->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParserSend->Print();}
        char * sSeqName = NULL;
        sSeqName = pParserSend->GetStringValue(pParserSend->m_pRootAvpItem,AVP_SEQUENCE_NAME);
        TMdbSequenceMgr * pSeqMgr = ptClient->GetSeqMgrByName(sSeqName,m_pConfig);
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
    }
    return iRet;
}

//处理消息
int TMdbNoNtcAgentServer::Run(TAgentClient *ptClient)
{
    TADD_FUNC("Begin.");
    int iRet = 0;
    if((iRet = RecvPackageOnce(ptClient)) != 0)
    {
        TADD_NORMAL("Run RecvPackage ret=[%d],ptClient->iFD=[%d]",iRet,ptClient->m_tTMdbCSLink.iFD);
        return iRet;
    }
    if((iRet = CheckClientSequence(ptClient)) != 0)
    {
        TADD_NORMAL("Run CheckClientSequence ret=[%d],ptClient->iFD=[%d]",iRet,ptClient->m_tTMdbCSLink.iFD);
        return iRet;
    }
    TADD_DETAIL("start...ptClient->m_tHead.iCmdCode=%d",ptClient->m_tHead.iCmdCode);
    //clock_t tStart = clock();
    
    struct timeval tCurTime,tEndTime;
    tCurTime = m_pDsn->tCurTime;
    time_t costTime = 0;
    if(MDB_CS_USE_OCP == ptClient->m_tTMdbCSLink.m_iUseOcp)
    {
        CHECK_RET(DoOperation(ptClient),"DoOperation return Value=[%d]",iRet);
        
    }
    else
    {
        CHECK_RET(DoOperationBin(ptClient),"DoOperation return Value=[%d]",iRet);
        
    }
    
    //CHECK_RET(DoOperation(ptClient),"DoOperation return Value=[%d]",iRet);
    //clock_t tFinish = clock();
    
    //double duration = (double)(tFinish - tStart)/CLOCKS_PER_SEC;
    
    tEndTime = m_pDsn->tCurTime;
    costTime = tEndTime.tv_sec - tCurTime.tv_sec;
    TADD_DETAIL("ThreadID=[%lu], ClientIP=[%s],SequenceID=[%d] DoOperation Time[%d]",ptClient->m_iThreadId,inet_ntoa(ptClient->m_tTMdbCSLink.tAddr.sin_addr),ptClient->m_tHead.GetSequence(),costTime);
    if(costTime >= 3)
    {
        TADD_WARNING("ThreadID=[%lu], ClientIP=[%s],SequenceID=[%d] DoOperation Time[%d] > 3s ",ptClient->m_iThreadId,inet_ntoa(ptClient->m_tTMdbCSLink.tAddr.sin_addr),ptClient->m_tHead.GetSequence(),costTime);
    }
    TADD_FUNC("Finish(%d).",iRet);
    return iRet;
}

//发送结果信息
int TMdbNoNtcAgentServer::SendPackageBody(TAgentClient *ptClient,int iSendLen)
{
    int lRet = 0;
    IS_LOG(1)
    {
        lRet = PrintSendMsg(ptClient);
        if(lRet < 0)
        {
            //没有配置CmdID
            return lRet;
        }
    }
    //记录每次的发送
    memcpy(ptClient->m_tLastSequence.m_sLastSendPackage,ptClient->m_sSendPackage,iSendLen);
    ptClient->m_tLastSequence.m_iLastSendLen = iSendLen;
    lRet=ptClient->m_ClientSocket.write(ptClient->m_sSendPackage,iSendLen);
    if(lRet < 0)
    {
        TADD_ERROR(ERR_NET_SEND_FAILED,"send Package failed.m_ClientSocket.write return Value=[%d]",lRet);
        lRet = PrintSendMsg(ptClient);
        if(lRet < 0)
        {
            //没有配置CmdID
            return lRet;
        }
        ThreadSleep(3000);
        lRet=ptClient->m_ClientSocket.write(ptClient->m_sSendPackage,iSendLen);
        if(lRet == ERR_NET_SEND_FAILED)
        {
            lRet = ERR_NET_SOCKET_CLOSE;
        }
        return lRet;
    }
    //发送数据包落地
    if(m_tProcCtrl.GetProcState() == PDUMP)
    {
        ptClient->m_cProcState = PDUMP;
        DumpPackage(ptClient, true);
    }
    return 0;
}

/******************************************************************************
* 函数名称	:  AppErrorAnswer
* 函数描述	:  错误消息应答
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbNoNtcAgentServer::AppErrorAnswer(TAgentClient *ptClient,int iAnsCode,const char *fmt,...)
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
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_ERROR,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        return SendAnswer(ptClient, iAnsCode, sTemp);
    }
}
/******************************************************************************
* 函数名称	:  DoOperation
* 函数描述	:  只处理正确的报文
* 输入		:
* 输出		:
* 返回值	:
* 作者

*******************************************************************************/
int TMdbNoNtcAgentServer::DoOperation(TAgentClient *ptClient)
{
    TADD_DETAIL("start...ptClient->m_tHead.iCmdCode=%d",ptClient->m_tHead.iCmdCode);
    int iRet = 0;
    switch(ptClient->m_tHead.iCmdCode)
    {
    case CSP_APP_KICK_LINK:  //ok
    {

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
    case CSP_APP_ADJUST_LOG:    //ok
    {
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
    case  CSP_APP_C_TABLE:    //ok
    {

        break;
    }
    case CSP_APP_C_USER:    //ok
    {


        break;
    }
    case CSP_APP_D_TABLE:   //ok
    {


        break;
    }
    case CSP_APP_D_USER:
    {


        break;
    }
    case CSP_APP_GET_SEQUENCE:
    {
        iRet = AppGetSequence(ptClient);
        if(iRet != 0)
        {
            TADD_ERROR(iRet,"return Value=[%d]",iRet);
        }
        break;
    }
    default:
    {
        TADD_ERROR(iRet," AVP CommonCode = [%d] not support!",ptClient->m_tHead.iCmdCode);
        return iRet;
    }
    }
    TADD_FUNC("end iRet=%d",iRet);
    return iRet;
}



/******************************************************************************
* 函数名称	:  RecvPackage
* 函数描述	:  接收消息包，
* 输入		:  ptClient - 表示每个客户端链接
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbNoNtcAgentServer::RecvPackage(TAgentClient * ptClient)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //接收头部
    if((iRet = ptClient->m_ClientSocket.read(ptClient->m_sRecvPackage,SIZE_MSG_AVP_HEAD)) <0)
    {
        TADD_NORMAL("Socket Read Faild=[%d] ",iRet);
        return iRet;
    }
    iRet = iRet >0?0:iRet;//大于0的不算错
    ptClient->m_sRecvPackage[SIZE_MSG_AVP_HEAD] = '\0';
    //解析头部
    ptClient->m_tHead.Clear();
    if(MDB_CS_USE_OCP == ptClient->m_tTMdbCSLink.m_iUseOcp)
    {
        ptClient->m_tHead.BinToCnvt((unsigned char*)(ptClient->m_sRecvPackage));
    }
    else
    {
        ptClient->m_tHead.BinToCnvtPlus((unsigned char*)(ptClient->m_sRecvPackage));
    }
    
    //ptClient->m_tHead.BinToCnvt((unsigned char*)(ptClient->m_sRecvPackage));
    if(ptClient->m_tHead.sHeadName[5] == '>' && ptClient->m_tHead.sHeadName[0]=='<' )
    {
        if(ptClient->m_iSessionID > 0 && ptClient->m_iSessionID != ptClient->m_tHead.iSessionId)
        {//session id不匹配
            CHECK_RET(ERR_CSP_PARSER_SESSIONID_NOEQUAL,"Peer session id[%d] != cur session id[%d]",
                    ptClient->m_tHead.iSessionId,ptClient->m_iSessionID);
        }
    }
    else
    {//头解析错误
        CHECK_RET(ERR_CSP_PARSER_ERROR_CSP,"Parse head error.");
    }
    if(false == ptClient->m_bRegister)
    {
        ptClient->m_tTMdbCSLink.m_iUseOcp = ptClient->m_tHead.iVersion;
    }
    
    //获取包体
    if(ptClient->m_ClientSocket.read(&ptClient->m_sRecvPackage[SIZE_MSG_AVP_HEAD],ptClient->m_tHead.iLen - SIZE_MSG_AVP_HEAD) <0)
    {
        CHECK_RET(ERR_NET_RECV_FAILED,"Read package body error.len=[%d]",ptClient->m_tHead.iLen);
    }
    IS_LOG(1)(PrintRecvMsg(ptClient));
    
    //接收数据包落地
    //if(-1 == *(QMDB_LOG_INST->m_pCurLogLevel))
    if(m_tProcCtrl.GetProcState() == PDUMP)
    {
        ptClient->m_cProcState = PDUMP;
        DumpPackage(ptClient, false);
    }
    TADD_FUNC("Finish");
    return iRet;
}

int TMdbNoNtcAgentServer::RecvPackageOnce(TAgentClient * ptClient)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iHeadLen = SIZE_MSG_BIN_HEAD;
    if(MDB_CS_USE_OCP == ptClient->m_tTMdbCSLink.m_iUseOcp)
    {
        iHeadLen = SIZE_MSG_AVP_HEAD;
    }
    //接收头部
    if((iRet = ptClient->m_ClientSocket.Recv(ptClient->m_sRecvPackage,RECV_PACKAGE_LEN_DEFAULT)) <0)
    {
        TADD_NORMAL("Socket Read Faild=[%d] ",iRet);
        return iRet;
    }
    int iRecvLen = iRet;
    iRet = iRet >0?0:iRet;//大于0的不算错
    unsigned char cHead = ptClient->m_sRecvPackage[iHeadLen];
    ptClient->m_sRecvPackage[iHeadLen] = '\0';
    //解析头部
    //ptClient->m_tHead.Clear();
    if(MDB_CS_USE_OCP == ptClient->m_tTMdbCSLink.m_iUseOcp)
    {
        ptClient->m_tHead.BinToCnvt((unsigned char*)(ptClient->m_sRecvPackage));
    }
    else
    {
        ptClient->m_tHead.Parse((unsigned char*)(ptClient->m_sRecvPackage));
    }
    
    if(ptClient->m_iSessionID > 0 && ptClient->m_iSessionID != ptClient->m_tHead.iSessionId)
    {//session id不匹配
        CHECK_RET(ERR_CSP_PARSER_SESSIONID_NOEQUAL,"Peer session id[%d] != cur session id[%d]",
                ptClient->m_tHead.iSessionId,ptClient->m_iSessionID);
    }
    if(false == ptClient->m_bRegister)
    {
        ptClient->m_tTMdbCSLink.m_iUseOcp = ptClient->m_tHead.iVersion;
    }
    ptClient->m_sRecvPackage[iHeadLen] = cHead;
    ptClient->m_sRecvPackage[iRecvLen] = '\0';
    //printf("recv %d\n",ptClient->m_tHead.iLen);
    if(ptClient->m_tHead.iLen > iRecvLen)
    {
        //printf("read twice\n");
        //获取包体
        if(ptClient->m_ClientSocket.read(&ptClient->m_sRecvPackage[iRecvLen],ptClient->m_tHead.iLen - iRecvLen) <0)
        {
            CHECK_RET(ERR_NET_RECV_FAILED,"Read package body error.len=[%d]",ptClient->m_tHead.iLen);
        }
    }
    IS_LOG(1)(PrintRecvMsg(ptClient));
    
    //接收数据包落地
    //if(-1 == *(QMDB_LOG_INST->m_pCurLogLevel))
    if(m_tProcCtrl.GetProcState() == PDUMP)
    {
        ptClient->m_cProcState = PDUMP;
        DumpPackage(ptClient, false);
    }
    TADD_FUNC("Finish");
    return iRet;
}
/******************************************************************************
* 函数名称	:  SendAnswer
* 函数描述	:  发送响应包
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbNoNtcAgentServer::SendAnswer(TMdbCspParser * pCspParser,TAgentClient *ptClient,int iAnsCode,const char * sMsg)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pCspParser);
    try
    {
        //TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(iAnsType,false);
        //CHECK_OBJ(pParser);
        pCspParser->SetItemValue(pCspParser->m_pRootAvpItem,AVP_ANSWER_CODE,iAnsCode);
        pCspParser->SetItemValue(pCspParser->m_pRootAvpItem,AVP_ANSWER_MSG,sMsg);
        pCspParser->Serialize(ptClient->m_sSendPackage,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        IS_LOG(2){pCspParser->Print();}
        ptClient->m_tHead = pCspParser->m_tHead;
        CHECK_RET(SendPackageBody(ptClient,pCspParser->m_tHead.iLen),"SendPackageBody return value=[%d]",iRet);
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




/******************************************************************************
* 函数名称	:  CheckClientSequence
* 函数描述	:  检测客户端序号
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbNoNtcAgentServer::CheckClientSequence(TAgentClient * ptClient)
{
    int iRet = 0;
    CHECK_OBJ(ptClient);
    TAgentClientSequence & tLastSeq = ptClient->m_tLastSequence;
    if(ptClient->m_tHead.GetSequence() < 0)
    {
        CHECK_RET_SEND_ANS(ERR_CSP_SEQ_VALUE,ptClient,"Client sequence can not be < 0");
    }
    if((int) ptClient->m_tHead.GetSequence() == tLastSeq.m_iLastSequence)
    {//需要重发
        TADD_WARNING("Client Sequence[%d] = Last sequence,need to resend.",ptClient->m_tHead.GetSequence());
        memcpy(ptClient->m_sSendPackage,tLastSeq.m_sLastSendPackage,tLastSeq.m_iLastSendLen);
        CHECK_RET(SendPackageBody(ptClient,tLastSeq.m_iLastSendLen),"SendPackageBody error.");
        return ptClient->m_tHead.GetSequence();
    }
    else
    {
        tLastSeq.m_iLastSequence = ptClient->m_tHead.GetSequence();
    }
    return iRet;
}
/******************************************************************************
* 函数名称	:  AppGetQmdbInfo
* 函数描述	:  获取qmdb 信息包
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbNoNtcAgentServer::AppGetQmdbInfo(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        TMdbCspParser * pParserQmdbInfo = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_QMDB_INFO,true);
        CHECK_OBJ(pParserQmdbInfo);
        pParserQmdbInfo->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//解析
       // pParserQmdbInfo->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParserQmdbInfo->Print();}
        char * sCmd = NULL;
        sCmd = pParserQmdbInfo->GetStringValue(pParserQmdbInfo->m_pRootAvpItem,AVP_COMMAND_NAME);
        TADD_DETAIL("cmd=[%s]",sCmd);
        TMdbInfoCollect mdbInfoCollect;
        mdbInfoCollect.Attach(m_pDsn->sName);
        std::string str;
        mdbInfoCollect.GetInfo(str,sCmd);
        TMdbCspParser * pParserQmdbInfoAns = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_QMDB_INFO,false);
        CHECK_RET(SendAnswer(pParserQmdbInfoAns,ptClient,0,str.c_str()),"SendAnswer error.");
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


//二进制
int TMdbNoNtcAgentServer::DoOperationBin(TAgentClient *ptClient)
{
    TADD_DETAIL("start...ptClient->m_tHead.iCmdCode=%d",ptClient->m_tHead.iCmdCode);

    int iRet = 0;
    /*
    //状态变更时落地之前的数据包
    char cProcState = m_tProcCtrl.GetProcState();
    if(PDUMP !=  cProcState
        && PDUMP == ptClient->m_cProcState) 
    {
        ptClient->m_cProcState = cProcState;
        m_tAgentFile.Write(ptClient->m_sPackageBuffer, strlen(ptClient->m_sPackageBuffer));
        memset(ptClient->m_sPackageBuffer, 0, sizeof(ptClient->m_sPackageBuffer));
    }
    
    iRet = RecvMsg(ptClient);
    if(iRet == ERR_CSP_KEEP_ON_RECV ) {return 0;}//继续接收
    if(iRet != 0 )
    {
        TADD_NORMAL("RecvMsg ret=[%d],iFD=[%d],Pno=[%d]",iRet,ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno);
        return iRet;
    }

    TADD_DETAIL("DoOperation...iCmdCode=%d",ptClient->m_tHead.iCmdCode);
    if((iRet = CheckClientSequence(ptClient)) != 0)
    {
        TADD_NORMAL("CheckClientSequence ret=[%d],iFD=[%d],Pno=[%d]",iRet,ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno);
        return iRet;
    }
    */
    //clock_t tStart = clock();
    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_LOGON:
        {
            CHECK_RET_BREAK(AuthenticationBin(ptClient),"Authentication failed.");//认证
            TADD_NORMAL("ThreadID=[%lu],ClientIP=[%s],SequenceID=[%d],SocketID=[%d],Pno=[%d] Login Success!! ",
                    ptClient->m_iThreadId,ptClient->m_tTMdbCSLink.m_sClientIP,ptClient->m_tHead.GetSequence(),
                    ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno);
            
            break;
        }
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
        }
        case CSP_APP_GET_SEQUENCE:
        {
            iRet = AppGetSequenceBin(ptClient);
            CHECK_RET(iRet,"AppGetSequence return Value=[%d]",iRet);
            break;
        }
        default:
        {
            TADD_ERROR(ERROR_UNKNOWN," AVP CommonCode = [%d] not support!",ptClient->m_tHead.iCmdCode);
            return iRet;
        }
    }
    /*
    clock_t tFinish = clock();
    double duration = (double)(tFinish - tStart)/CLOCKS_PER_SEC;
    if(duration >= 3.0)
    {
        TADD_WARNING("CmdCode=[%u],ThreadID=[%lu], ClientIP=[%s],SequenceID=[%d] DoOperation Time[%f] > 3s ",
                ptClient->m_tHead.iCmdCode,ptClient->m_iThreadId,ptClient->m_tTMdbCSLink.m_sClientIP,ptClient->m_tHead.GetSequence(),duration);
    }
    */
    TADD_FUNC("end iRet=%d",iRet);
    return iRet;
}

int TMdbNoNtcAgentServer::AuthenticationBin(TAgentClient *ptClient)
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
        
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
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
    TMdbNoNtcAgentServer::m_iSessionStep++;
    if(TMdbNoNtcAgentServer::m_iSessionStep >= INT_MAX)
    {
        m_iSessionStep = 1;
    }
    ptClient->m_iSessionID = m_iSessionStep;
    CHECK_RET(tMutex.UnLock(true),"UnLock Faild");
    //回复登陆成功
    //head+anscode+msg
    ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_LOGON,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
    CHECK_RET(SendAnswer(ptClient,0,"LOGIN_OK"),"SendAnswer[LOGIN_OK],failed");
    return iRet;
}

//bin
int TMdbNoNtcAgentServer::AppSendSQLBin(TAgentClient *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    int iSqlFlag = 0;
    int iTmp = 0;
    short int iTmpSh = 0;
    //char sSQL[MAX_SQL_LEN]   = "";
    try
    {
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        char *pSQL = ptClient->m_tRecvDataParse.GetDataPtr() + NO_OCP_SQL_POS;
        //ptClient->m_tRecvDataParse.GetData(sSQL,NO_OCP_SQL_POS,ptClient->m_tHead.iLen - NO_OCP_SQL_POS);
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
           CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        pQuery->SetSQL(pSQL,iSqlFlag,0);//设置SQL
        ptClient->m_iParamCount = pQuery->ParamCount();
        TADD_NORMAL("QuerySqlLabel=[%d],SetSQL(%s),IsDynamic=[%s],SqlFlag=[%d]",iSqlLabel,pSQL,(ptClient->m_iParamCount != 0)?"TRUE":"FALSE",iSqlFlag);
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_SEND_SQL,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        pClientQuery->m_bFirstNext = true;
        //ptClient->m_iFieldCount = pQuery->FieldCount();
        //ptClient->m_iSqlType = pQuery->GetSQLType();
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
            if(TK_SELECT == pQuery->GetSQLType())
            {
                pQuery->Open();
                TADD_FLOW("QuerySqlLabel=[%d],Open(),StaticSQL.",iSqlLabel);
                CHECK_RET(FillNextResult(ptClient,pQuery,pClientQuery->m_bFirstNext),"FillNextResult error");
            }
            else
            {
                pQuery->Execute();
                iTmp = 0;
                ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(char));//AVP_SELECT_HAVE_NEXT
                iTmpSh = (short int)pQuery->RowsAffected();
                ptClient->m_tSendDataParse.SetData(&iTmpSh,sizeof(short int));//AVP_AFFECTED_ROW
                
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

int TMdbNoNtcAgentServer::AppSendParamBin(TAgentClient *ptClient)
{
    int iRet      = 0;
    int iSqlLabel = 0;
    try
    {
        //ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
            CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        char iParamCount = 0;
        short int iParamPackageLen = 0;
        char iParamIndex = 0;
        int iParamPos = 0;
        //head+sqllabel+cnt+packageLen+paramIndex+Value
        //packageLen = sizeof(int) * 2 -> null
        ptClient->m_tRecvDataParse.GetData(&iParamCount,NO_OCP_PARAMCNT_POS,sizeof(char));
        iParamPos += NO_OCP_PARAMCNT_POS + sizeof(char); 
        for(int i=0; i<iParamCount; ++i)
        {
            ptClient->m_tRecvDataParse.GetData(&iParamPackageLen,(size_t)iParamPos,sizeof(short int));
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
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_SEND_SQL,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        //ptClient->m_bFirstNext = true;
        //ptClient->m_iFieldCount = pQuery->FieldCount();
        if(pQuery->GetSQLType() == TK_SELECT)
        {
            pQuery->Open();
            CHECK_RET(FillNextResult(ptClient,pQuery,pClientQuery->m_bFirstNext),"FillNextResult error");
        }
        else
        {//
            pQuery->Execute();
            int iTmp = 0;
            ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(char));//AVP_SELECT_HAVE_NEXT
            short int iTmpSh = (short int)pQuery->RowsAffected();
            ptClient->m_tSendDataParse.SetData(&iTmpSh,sizeof(short int));//AVP_AFFECTED_ROW
        
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

/***
AppSendParamArray
->
	head 43
	sqlabel 4
	batchcount 4
	paramcount 4
	PackageLen (=4,null)
	Value 

***/
int TMdbNoNtcAgentServer::AppSendParamArrayBin(TAgentClient *ptClient)
{
    TADD_FUNC("Start,AppSendParamArray");
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
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

        short int iBatchCount = 0;
        char iParamCount = 0;
        short int iParamPackageLen = 0;
        char iParamIndex = 0;
        int iParamPos = 0;
        short int iRowEffect = 0;
        
        //head+sqllabel+batchcnt+pramcount+packageLen+Value
        //packageLen = sizeof(int) * 2 -> null
        ptClient->m_tRecvDataParse.GetData(&iBatchCount,NO_OCP_BATCHCNT_POS,sizeof(short int));
        ptClient->m_tRecvDataParse.GetData(&iParamCount,NO_OCP_BATCHCNT_POS+sizeof(short int),sizeof(char));
        iParamPos += NO_OCP_BATCHCNT_POS + sizeof(short int) + sizeof(char); 
        
        //获取首个BATCH_GROUP
        for(short int iBatchIndex = 0; iBatchIndex < iBatchCount; ++iBatchIndex)
        {
            //一个批次
            for(iParamIndex=0; iParamIndex<iParamCount; ++iParamIndex)
            {
                ptClient->m_tRecvDataParse.GetData(&iParamPackageLen,(size_t)iParamPos,sizeof(short int));
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
            iRowEffect += (short int)pQuery->RowsAffected();//执行
        }

        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_SEND_SQL,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        int iTmp = 0;
        ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(char));//AVP_SELECT_HAVE_NEXT
        ptClient->m_tSendDataParse.SetData(&iRowEffect,sizeof(short int));//AVP_AFFECTED_ROW
        SendAnswer(ptClient,0,"AppSendParamArray OK");
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

int TMdbNoNtcAgentServer::FillNextResult(TAgentClient *ptClient,TMdbQuery * pQuery,bool &bFirstNext)throw(TMdbException,TMdbCSPException)
{
    int iRet = 0;
    int iFieldCount = pQuery->FieldCount();
    short int iCount = 0;
    char iTmpCh = 0;
    short iTmpSh = 0;
    short int iValueLen = 0;
    char cDataType = 0;
    bool bHaveNext = true;
    bool bFirstNextTmp = bFirstNext;
    //bool bSetFieldCount = false;
    //bool bFirst = false;
    //head+sqllabel+haveNext+recordCount+fieldCount+nameLen+fieldName+valueLen+Value
    //fieldLen=4 -> null, value no
    ptClient->m_tSendDataParse.SetData(&iTmpCh,sizeof(char));//have next
    ptClient->m_tSendDataParse.SetData(&iTmpSh,sizeof(short int));//记录个数
    
    while(1)
    {
        if(pQuery->Next())
        {
            iCount ++;
            int j = 0;
            if(bFirstNextTmp)
            {
                iTmpCh = (char)iFieldCount;
                ptClient->m_tSendDataParse.SetData(&iTmpCh,sizeof(char));//字段个数
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
            for(j = 0; j<iFieldCount; j++)
            {
                cDataType = pQuery->Field(j).GetDataType();
                if(bFirstNextTmp)
                {
                    iValueLen = (short int)(strlen(pQuery->Field(j).GetName())+1);
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
                    iValueLen = (short int)sizeof(lData);
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));
                    ptClient->m_tSendDataParse.SetData(&lData,iValueLen);
                }
                else
                {
                    char *pData = pQuery->Field(j).AsRealStr();
                    iValueLen = (short int)(strlen(pData) + 1);
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));
                    ptClient->m_tSendDataParse.SetData(pData,iValueLen);
                }
            }
            //#endif
            bFirstNextTmp = false;
            bFirstNext = false;
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
    
    //ptClient->m_tSendDataParse.SetSize();
	//bNext = bHaveNext;
    ptClient->m_tSendDataParse.SetData(&iCount,SIZE_MSG_BIN_HEAD+5,sizeof(short int));//记录个数
    if(bHaveNext)
    {
        iTmpCh = 1;
        ptClient->m_tSendDataParse.SetData(&iTmpCh,SIZE_MSG_BIN_HEAD+4,sizeof(char));//have next
    }
    else
    {
        iTmpCh = 0 ;
        ptClient->m_tSendDataParse.SetData(&iTmpCh,SIZE_MSG_BIN_HEAD+4,sizeof(char));//have next
    }
    return iRet;
}

//响应next操作
int TMdbNoNtcAgentServer::AppNextSQLResultBin(TAgentClient *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
            CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_SEND_SQL,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        if(pQuery->GetSQLType() == TK_SELECT)
        {
            CHECK_RET(FillNextResult(ptClient,pQuery,pClientQuery->m_bFirstNext),"FillNextResult error");
            SendAnswer(ptClient,0,"AppNextSQLResult OK");
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

int TMdbNoNtcAgentServer::AppTransActionBin(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        char sCmd[32]="";
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(sCmd,SIZE_MSG_BIN_HEAD,ptClient->m_tHead.iLen - SIZE_MSG_BIN_HEAD);
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
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_ACTION,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        CHECK_RET(SendAnswer(ptClient,0,"OK"),"SendAnswer error.");
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

int TMdbNoNtcAgentServer::AppGetQmdbInfoBin(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        char sCmd[32]="";
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(sCmd,SIZE_MSG_BIN_HEAD,ptClient->m_tHead.iLen - SIZE_MSG_BIN_HEAD);
        TMdbInfoCollect mdbInfoCollect;
        mdbInfoCollect.Attach(m_pDsn->sName);
        std::string str;
        mdbInfoCollect.GetInfo(str,sCmd);
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_QMDB_INFO,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
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

int TMdbNoNtcAgentServer::AppGetSequenceBin(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        char sSeqName[64]="";
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(sSeqName,SIZE_MSG_BIN_HEAD,ptClient->m_tHead.iLen - SIZE_MSG_BIN_HEAD);
        TMdbSequenceMgr * pSeqMgr = ptClient->GetSeqMgrByName(sSeqName,m_pConfig);
        if(NULL == pSeqMgr)
        {
            TADD_ERROR(-1,"not find sequence mgr by [%s].",sSeqName);
            iRet = AppErrorAnswer(ptClient,-1,"not find sequence mgr by [%s].",sSeqName); 
            return iRet;
        }
        
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_GET_SEQUENCE,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
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
    }
    return iRet;
}

int TMdbNoNtcAgentServer::SendAnswer(TAgentClient *ptClient,int iAnsCode,const char * sMsg)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    try
    {
        short int iAnsCodeSh = (short int)iAnsCode;
        ptClient->m_tSendDataParse.SetDataAnsCode(&iAnsCodeSh,sizeof(short int));
        if(sMsg)
        {
            ptClient->m_tSendDataParse.SetData((char*)sMsg,strlen(sMsg)+1);
        }
        ptClient->m_tSendDataParse.SetSize();
        ptClient->m_tHead.iLen = ptClient->m_tSendDataParse.GetSize();
        CHECK_RET(SendPackageBody(ptClient,ptClient->m_tSendDataParse.GetSize()),"SendPackageBody return value=[%d]",iRet);
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

/******************************************************************************
* 函数名称	:  TMdbAgentServer
* 函数描述	:  构造
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
TMdbAgentServer::TMdbAgentServer()
{
    m_pShmDSN = NULL;
    m_pConfig = NULL;
    m_pDsn    = NULL;
    m_iPumpMaxCount = 0;
    m_iPeerCountPerPump = 0;
    m_iClientThreadNum = 0;
    for(int i=0; i<MAX_CS_THREAD_COUNTS; i++)
    {
        m_ptClient[i] = NULL;
    }
    m_bRecvFlag = true;
    m_vValidIP.clear();
    m_vInValidIP.clear();
}


TMdbAgentServer::~TMdbAgentServer()
{
    for(int i=0; i<MAX_CS_THREAD_COUNTS; i++)
    {
        SAFE_DELETE(m_ptClient[i]);
    }
    tMutex.Destroy();
    tMutexConnect.Destroy();    
}

/******************************************************************************
* 函数名称  :  OnConnect
* 函数描述  :  当有socket连接或者socket连接成功时，触发此函数
* 输入      :  事件信息\事件泵
* 输出      :
* 返回值    :
* 作者      :
*******************************************************************************/
bool TMdbAgentServer::OnConnect(TMdbConnectEvent * pEventInfo,TMdbEventPump * pEventPump)
{
    TADD_NORMAL("Find a new connection from[%s:%u],Pno[%d],Client count=[%d]", 
            pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(),pEventInfo->pPeerInfo->GetRemotePort(),pEventInfo->pPeerInfo->pno,GetPeerCount()); 
    //AddWorkThread();//增加工作线程
    //申请TAgentClient
    if(NULL == GetFreeAgentClient(pEventInfo))
    {
        TADD_ERROR(ERROR_UNKNOWN, "Get AgentClient error");
        return false;
    }
    return true;
}

/******************************************************************************
* 函数名称  :  OnDisconnect
* 函数描述  :  当有连接断开时，触发此函数
* 输入      :  事件信息\事件泵
* 输出      :
* 返回值    :
* 作者      :
*******************************************************************************/
bool TMdbAgentServer::OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    int iPno = pEventInfo->pPeerInfo->pno;
    TADD_NORMAL("Disconnect quest from[%s:%d],Pno[%d],Client count=[%d] now", 
            m_ptClient[iPno]->m_tTMdbCSLink.m_sClientIP,m_ptClient[iPno]->m_iPort,iPno,GetPeerCount()); 
    //退出时落地数据包
    if(m_ptClient[iPno]->m_sPackageBuffer[0] != '\0')
    {
        m_tAgentFile.Write(m_ptClient[iPno]->m_sPackageBuffer, strlen(m_ptClient[iPno]->m_sPackageBuffer));
    }
    m_tLinkCtrl.UnRegRemoteLink(m_ptClient[iPno]->m_tTMdbCSLink);
    ClientExit(m_ptClient[iPno]);    
    TADD_NORMAL("Pno=[%d] exit!!",iPno);
    return true;
}

/******************************************************************************
* 函数名称  :  OnError
* 函数描述  :  当发生错误时，触发此函数，如解析消息包出错等
* 输入      :  事件信息\事件泵
* 输出      :
* 返回值    :
* 作者      :
*******************************************************************************/
bool TMdbAgentServer::OnError(TMdbErrorEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    TADD_ERROR(ERROR_UNKNOWN, "Peer count=[%d],Error:%s", 
            GetPeerCount(), 
            pEventInfo->sErrorInfo.c_str());

    return true;
}

/******************************************************************************
* 函数名称  :  OnTimeOut
* 函数描述  :  当空闲超时时，触发此函数
* 输入      :  事件信息\事件泵
* 输出      :
* 返回值    :
* 作者      :
*******************************************************************************/
bool TMdbAgentServer::OnTimeOut(TMdbTimeoutEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    if(pEventInfo->pPeerInfo)
    {
        int iPno = pEventInfo->pPeerInfo->pno;
        TADD_WARNING("Time out from[%s:%d],Pno[%d]",
                m_ptClient[iPno]->m_tTMdbCSLink.m_sClientIP,m_ptClient[iPno]->m_iPort,iPno);
    }
    else
    {
        TADD_WARNING("Time out ");
    
    }
    return true;
}

/******************************************************************************
* 函数名称  :  OnRecvMsg
* 函数描述  :  当客户端有消息来的时候，触发此函数
* 输入      :  事件信息\事件泵
* 输出      :
* 返回值    :
* 作者      :
*******************************************************************************/
bool TMdbAgentServer::OnRecvMsg(TMdbRecvMsgEvent * pEventInfo,TMdbEventPump * pEventPump)
{
    int iPno = pEventInfo->pPeerInfo->pno;
    m_bRecvFlag = true;
    
    if(NULL == m_ptClient[iPno])
    {
        TADD_ERROR(ERROR_UNKNOWN, "ptClient is null");
        return false;
    }
    m_ptClient[iPno]->m_pEventInfo = pEventInfo;
    int iRet = 0;
    if(MDB_CS_USE_OCP == m_ptClient[iPno]->m_tTMdbCSLink.m_iUseOcp)
    {
        iRet = DoOperation(m_ptClient[iPno]);
    }
    else
    {
        iRet = DoOperationBin(m_ptClient[iPno]);
    }
    if(iRet != 0)
    {
        TADD_ERROR(ERROR_UNKNOWN, "DoOperation return Value=[%d]", iRet);
        AppErrorAnswer(m_ptClient[iPno],iRet,"ERR_CODE=[%d],DoOperation error",iRet);
        return false;
    }
    
    m_bRecvFlag = false;
    return true;
}

//解析限制的ip列表
void TMdbAgentServer::ParseCSIPCfg()
{
    const char *pValidIP = m_pConfig->GetDSN()->m_sCSValidIP;
    const char *pInValidIP = m_pConfig->GetDSN()->m_sCSInValidIP;
    //char sIP[MAX_IP_LEN] = {0};
    TMdbNtcSplit tSplit,tSplitNext;
    if(pValidIP[0] != '\0')
    {
        
        tSplit.SplitString(pValidIP,';');
        for(unsigned int i=0; i<tSplit.GetFieldCount(); ++i)
        {
            tSplitNext.SplitString(tSplit[i],'.');
            if(tSplitNext.GetFieldCount() == 4 && strlen(tSplit[i]) < MAX_IP_LEN)//ip 点4位
            {
                m_vValidIP.push_back(string(tSplit[i]));
            }
        }
        
    }
    if(pInValidIP[0] != '\0')
    {
        tSplit.SplitString(pInValidIP,';');
        for(unsigned int i=0; i<tSplit.GetFieldCount(); ++i)
        {
            tSplitNext.SplitString(tSplit[i],'.');
            if(tSplitNext.GetFieldCount() == 4 && strlen(tSplit[i]) < MAX_IP_LEN)//ip 点4位
            {
                m_vInValidIP.push_back(string(tSplit[i]));
            }
        }
        
    }
    return;
    

}
/******************************************************************************
* 函数名称  :  CheckNewClient
* 函数描述  :  检验连接ip 是否受限
* 输入      :  
* 输出      :
* 返回值    :
* 作者      :
*******************************************************************************/
bool TMdbAgentServer::CheckNewClient(QuickMDB_SOCKET new_fd,  TMdbServerInfo* pServerInfo)
{
    if(m_vValidIP.begin() != m_vValidIP.end() || m_vInValidIP.begin() != m_vInValidIP.end())
    {
        TMdbNtcStringBuffer sIP = TMdbNtcSocket::GetRemoteAddrInfo(new_fd);//连接上来的ip
        TMdbNtcSplit tSplitPeer,tSplit;
        tSplitPeer.SplitString(sIP.c_str(),'.');
        if(m_vValidIP.begin() != m_vValidIP.end())
        {
            if(false == IsIPExist(m_vValidIP, tSplit, tSplitPeer))
            {//不在有效ip 列表中
                TADD_WARNING("IP[%s] not allowed to connect server", sIP.c_str());
                return false;
            }
            
        }
        if(m_vInValidIP.begin() != m_vInValidIP.end())
        {
            if(true == IsIPExist(m_vInValidIP, tSplit, tSplitPeer))
            {//无效ip
                TADD_WARNING("IP[%s] not allowed to connect server", sIP.c_str());
                return false;
            }
            
        }
        
    }
    
    AddWorkThread();//增加工作线程
    return true;
}

/******************************************************************************
* 函数名称  :  IsIPExist
* 函数描述  :  检查ip是否在配置列表中
* 输入      :  
* 输出      :
* 返回值    :
* 作者      :
*******************************************************************************/
bool TMdbAgentServer::IsIPExist(std::vector<string> &vIPListCfg, TMdbNtcSplit &tSplitCfg, TMdbNtcSplit &tSplitPeer)
{
    //匹配ip 格式10.45.7.61 或10.45.7.*
    std::vector<string>::iterator itor = vIPListCfg.begin();
    std::vector<string>::iterator itor_end = vIPListCfg.end();
    bool bFind = false;
    for(; itor != itor_end; ++itor)
    {
        tSplitCfg.SplitString((*itor).c_str(),'.');
        if(tSplitCfg.GetFieldCount() != 4) continue;//4//
        unsigned int i = 0;
        for(; i<tSplitCfg.GetFieldCount(); i++)
        {
            if(tSplitCfg[i][0] == '*') 
            {//模糊匹配继续
                continue;
            }
            else if(strcmp(tSplitCfg[i],tSplitPeer[i]) != 0) 
            {//不相同，退出匹配下一个ip列表
                break;
            }
        }
        if(i == tSplitCfg.GetFieldCount()) 
        {
            bFind = true;
            break;
        }
    }

    return bFind;
}
/******************************************************************************
* 函数名称  :  RecvMsg
* 函数描述  :  接收数据包
* 输入      :  TAgentClient * 
* 输出      :
* 返回值    :
* 作者      :
*******************************************************************************/
int TMdbAgentServer::RecvMsg(TAgentClient * ptClient)
{
    int iRet = 0;
    const char * pData = ptClient->m_pEventInfo->pMsgInfo->GetBuffer();
    CHECK_OBJ(pData);
    unsigned int iGetLen = ptClient->m_pEventInfo->pMsgInfo->GetLength();
    unsigned int iHeadLen = (MDB_CS_USE_OCP == ptClient->m_tTMdbCSLink.m_iUseOcp)?SIZE_MSG_AVP_HEAD:SIZE_MSG_BIN_HEAD;
    if(ptClient->m_iRecvPos == 0)
    {//第一个包，需要解析头部
        if(iGetLen <= iHeadLen) 
        {
            CHECK_RET(ERR_CSP_INVALID_RCV_PACKAGE,"RecvMsg len[%u]", iGetLen);
        }
        memcpy(ptClient->m_sRecvPackage, pData, iHeadLen);
        ptClient->m_sRecvPackage[iHeadLen] = '\0';
        CHECK_RET(ParseMsgHead(ptClient),"ParseMsgHead failed");
        //校验长度
        if(iGetLen == ptClient->m_tHead.iLen)
        {//一次就完整
            memcpy(&ptClient->m_sRecvPackage[iHeadLen], pData+iHeadLen, ptClient->m_tHead.iLen - iHeadLen);
        }
        else if(iGetLen < ptClient->m_tHead.iLen)
        {//不完整，需要继续接收
            memcpy(&ptClient->m_sRecvPackage[iHeadLen], pData+iHeadLen, iGetLen - iHeadLen);
            ptClient->m_iRecvPos = iGetLen;
            return ERR_CSP_KEEP_ON_RECV;
        }
        else
        {
            ptClient->m_iRecvPos = 0;
            CHECK_RET(ERR_CSP_PARSER_ERROR_CSP,"GetLength[%u] > [%u].", iGetLen, ptClient->m_tHead.iLen);
        }
        
    }
    else
    {//数据不完整，需要继续接收
        if(iGetLen + ptClient->m_iRecvPos < ptClient->m_tHead.iLen)
        {//继续
            memcpy(&ptClient->m_sRecvPackage[ptClient->m_iRecvPos], pData, iGetLen);
            ptClient->m_iRecvPos += iGetLen;
            return ERR_CSP_KEEP_ON_RECV;
        }
        else if(iGetLen + ptClient->m_iRecvPos == ptClient->m_tHead.iLen)
        {//已完整
            memcpy(&ptClient->m_sRecvPackage[ptClient->m_iRecvPos], pData, iGetLen);
            ptClient->m_iRecvPos = 0;
        }
        else
        {
            unsigned int iRecvPos = ptClient->m_iRecvPos;
            ptClient->m_iRecvPos = 0;
            CHECK_RET(ERR_CSP_PARSER_ERROR_CSP,"GetLength[%u] + RecvLen[%u] > [%u].", iGetLen,iRecvPos,ptClient->m_tHead.iLen);
        }
    }
    
    IS_LOG(1)
    (PrintRecvMsg(ptClient));
    
    //接收数据包落地
    if(m_tProcCtrl.GetProcState() == PDUMP)
    {
        ptClient->m_cProcState = PDUMP;
        DumpPackage(ptClient, false);
    }
    
    return iRet;
}

int TMdbAgentServer::ParseMsgHead(TAgentClient * ptClient)
{
    int iRet = 0;
    //解析头部
    ptClient->m_tHead.Clear();
    if(MDB_CS_USE_OCP == ptClient->m_tTMdbCSLink.m_iUseOcp)
    {
        ptClient->m_tHead.BinToCnvt((unsigned char*)(ptClient->m_sRecvPackage));
    }
    else
    {
        ptClient->m_tHead.Parse((unsigned char*)(ptClient->m_sRecvPackage));
    }
    //if(ptClient->m_tHead.sHeadName[5] == '>' && ptClient->m_tHead.sHeadName[0]=='<' )
    {
        if(ptClient->m_iSessionID > 0 && ptClient->m_iSessionID != ptClient->m_tHead.iSessionId)
        {//session id不匹配
            CHECK_RET(ERR_CSP_PARSER_SESSIONID_NOEQUAL,"Peer session id[%d] != cur session id[%d]",
                    ptClient->m_tHead.iSessionId,ptClient->m_iSessionID);
        }
    }
    if(false == ptClient->m_bRegister)
    {
        ptClient->m_tTMdbCSLink.m_iUseOcp = ptClient->m_tHead.iVersion;
    }
    return iRet;
}

/******************************************************************************
* 函数名称  :  Init()
* 函数描述  :  初始化：Attach共享内存，取出进程信息所在位置
* 输入      :  pszDSN, 锁管理区所属的DSN
* 输出      :  无
* 返回值    :  0 - 成功!0 -失败
* 作者      :  li.shugang
*******************************************************************************/
int TMdbAgentServer::Init(const char* pszDSN)
{
    CHECK_OBJ(pszDSN);
    TADD_FUNC("Init(%s) : Start.", pszDSN);
    int iRet = 0;
    //构造配置对象
    m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
    //连接上共享内存
    m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN);
    CHECK_OBJ(m_pShmDSN);
    m_pDsn = m_pShmDSN->GetInfo();
    CHECK_RET(m_tProcCtrl.Init(pszDSN),"m_tProcCtrl.Init(%s) failed.",pszDSN);
    CHECK_RET(m_tLinkCtrl.Attach(pszDSN),"m_tLinkCtrl.Init(%s) failed.",pszDSN);
    m_tAgentFile.Init();
    if(m_pConfig) ParseCSIPCfg();//解析ip
    tMutex.Create();
    tMutexConnect.Create();
    TADD_FUNC("TMdbAgentServer::Init(%s) : Finish.", pszDSN);
    return iRet;
}
/******************************************************************************
* 函数名称  :  PreSocket()
* 函数描述  :  初始化Socket，并进行监听
* 输入      :
* 输出      :  无
* 返回值    :  成功返回0，否则返回负数返回码
* 作者      :  li.shugang
*******************************************************************************/
int TMdbAgentServer::PreSocket(int iAgentPort)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_RET(m_tLinkCtrl.ClearRemoteLink(iAgentPort),"m_tLinkCtrl.ClearRemoteLink(%d) failed.",iAgentPort);
    TADD_DETAIL("iAgentPort=%d.", iAgentPort);
    if(false == AddListen(NULL, iAgentPort))
    {
        CHECK_RET(ERR_NET_LISTEN_PORT, "NTC AddListen (%s:%d) failed,ntc_error[%s].", m_pDsn->sLocalIP, iAgentPort,mdb_ntc_errstr.c_str());
    }
    TADD_NORMAL("PreSocket(%s:%d) : Waiting for connections and data......\n", m_pDsn->sLocalIP, iAgentPort);
    return iRet;
}
/******************************************************************************
* 函数名称  :  StartServer()
* 函数描述  :  建立监听，并针对链接进行处理
* 输入      :  pszName, 进程名
* 输出      :  无
* 返回值    :  如果可以链接返回0, 否则返回负数返回码
* 作者      :  li.shugang
*******************************************************************************/
int TMdbAgentServer::StartServer(const char* pszName)
{
    TADD_FUNC("Begin[%s].",pszName);
    
    int iRet = 0;
    int iCount = 0;
    m_iPumpMaxCount = m_pConfig->GetDSN()->m_iCSPumpMaxCount;
    m_iPeerCountPerPump = m_pConfig->GetDSN()->m_iCSPeerCountPerPump;
    
    SetMaxPeerNo(MAX_CS_THREAD_COUNTS);//设置对端个数
    InitEventPump(m_pConfig->GetDSN()->m_iCSPumpInitCount);//初始化事件泵个数
    if(false == Start())//启动服务
    {
        CHECK_RET(ERR_NET_NTC_START, "Start NtcEngine failed,ntc_error[%s].",mdb_ntc_errstr.c_str());
    }
    while(true)
    {
        if(m_tProcCtrl.IsCurProcStop())
        {
            TADD_NORMAL("Stop......");
            break;
        }
        
        m_tProcCtrl.UpdateProcHeart(0);//应用进程心跳时间
        if(!m_bRecvFlag && iCount%1000 == 0)
        {//没有接收事件，输出事件泵状态间隔200s
            //TADD_NORMAL("PrintAllPump:%s",PrintAllPump().c_str());
            for (unsigned int i = 0; i < m_arrayPump.GetSize(); ++i)
            {
                TMdbEventPump* pEventPump = static_cast<TMdbEventPump*>(m_arrayPump[i]);
                if(pEventPump->GetQueueSize() > 0)
                {
                    TADD_NORMAL("event pump[%u]:%s", i, pEventPump->ToString().c_str());                    
                }
            }
        }
        //AddWorkThread();
        
        TMdbDateTime::MSleep(200);//0.2s
        if(iCount >= INT_MAX-1) iCount = 1;
        iCount++;
        
    }
    TADD_NORMAL("Process[mdbAgent] Exist.");
    return iRet;
}
/******************************************************************************
* 函数名称	:  AddWorkThread
* 函数描述	:  增加工作线程
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbAgentServer::AddWorkThread()
{
    int iRet = 0;
    CHECK_RET(tMutexConnect.Lock(true),"Lock Faild");
    int iPumpCount = GetPumpCount();
    int iPeerCount = GetPeerCount();
    if(iPumpCount < m_iPumpMaxCount && iPeerCount >= m_iPeerCountPerPump*iPumpCount)
    {//增加事件泵
        TMdbEventPump* pEventPump = CreateEventPump();
        if(!pEventPump)
        {
            TADD_WARNING("Add EventPump faild, pump:peer count =[%d:%d].",iPumpCount,iPeerCount);
        }
        else
        {
            AddEventPump(pEventPump);
            if(!pEventPump->Run())
            {//由引擎来释放
                TADD_WARNING("Run EventPump faild, pump:peer count =[%d:%d].",iPumpCount,iPeerCount);
            }
        }
    }
    
    CHECK_RET(tMutexConnect.UnLock(true),"UnLock Faild");
    return iRet;
    
}
/******************************************************************************
* 函数名称	:  ClientExit
* 函数描述	:  对端退出收尾工作
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TMdbAgentServer::ClientExit(TAgentClient *&ptClient)
{
    SAFE_DELETE(ptClient);
    return;
}

/******************************************************************************
* 函数名称	:  GetFreeAgentClient
* 函数描述	:  获取空闲agent client
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
TAgentClient * TMdbAgentServer::GetFreeAgentClient(TMdbConnectEvent * pEventInfo)
{
    int iPno = pEventInfo->pPeerInfo->pno;
    //TADD_NORMAL("Get AgentClient[Pno=%d] begin",iPno);
    if(iPno >= MAX_CS_THREAD_COUNTS)
    {
        TADD_ERROR(ERR_APP_EXCEED_MAX_CS_PEER_COUNT,"Pno=[%d] greater than MAX_THREAD_COUNTS=[%d]",iPno,MAX_CS_THREAD_COUNTS);
        return NULL;
    }
    
    ClientExit(m_ptClient[iPno]); 
    m_ptClient[iPno] = new(std::nothrow) TAgentClient();
    if(m_ptClient[iPno] == NULL)
    {
        TADD_ERROR(ERR_OS_NO_MEMROY,"errno=%d,strerror=[%s]",errno,strerror(errno));
        return NULL;
    }
    if(m_ptClient[iPno]->Init() < 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Client Init() Failed");//释放对象......
        ClientExit(m_ptClient[iPno]); 
        return NULL;
    }
    m_ptClient[iPno]->m_iThreadId = pthread_self();
    m_ptClient[iPno]->m_iPno = iPno;
    m_ptClient[iPno]->m_tTMdbCSLink.iFD = pEventInfo->pPeerInfo->GetSocketID();
    //pEventInfo->pPeerInfo->GetRemoteAddrInfo(m_ptClient[iPno]->m_tTMdbCSLink.tAddr);
    m_ptClient[iPno]->m_iPort = pEventInfo->pPeerInfo->GetRemotePort();
    SAFESTRCPY(m_ptClient[iPno]->m_tTMdbCSLink.m_sClientIP, MAX_IP_LEN, pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str());
    //TADD_NORMAL("Get AgentClient[Pno=%d] ",iPno);
    
    return m_ptClient[iPno];
}

/******************************************************************************
* 函数名称	: Authentication 
* 函数描述	:  认证
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbAgentServer::Authentication(TAgentClient *ptClient)
{
    TADD_FUNC("Begin.");
    int iRet = 0;
    if(CSP_APP_LOGON != ptClient->m_tHead.iCmdCode)
    {//非登陆包
        CHECK_RET_SEND_ANS(ERR_CSP_PARSER_LOGON,ptClient,"want to recv logon package,but recv[%d]",ptClient->m_tHead.iCmdCode);
    }
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_LOGON,true);
        CHECK_OBJ(pParser);
        CHECK_RET(pParser->DeSerialize((unsigned char*)(ptClient->m_sRecvPackage),ptClient->m_tHead.iLen),"parser logon package error");
       // pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
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
		if(NULL != pParser->FindExistAvpItem(pParser->m_pRootAvpItem,AVP_LOW_PRIORITY))
        {//有sql-priority
            ptClient->m_tTMdbCSLink.m_iLowPriority = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_LOW_PRIORITY);
			if(ptClient->m_tTMdbCSLink.m_iLowPriority == 1)
			{
				TADD_NORMAL("Low priority link request.");
			}
        }
		//TADD_NORMAL("ptClient->m_tTMdbCSLink.m_iLowPriority = [%d]", ptClient->m_tTMdbCSLink.m_iLowPriority);
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
    //tMutex.Lock(true);
    TMdbAgentServer::m_iSessionStep++;
    if(TMdbAgentServer::m_iSessionStep >= INT_MAX)
    {
        m_iSessionStep = 1;
    }
    ptClient->m_iSessionID = m_iSessionStep;
    CHECK_RET(tMutex.UnLock(true),"UnLock Faild");
    //tMutex.UnLock(true);
    //回复登陆成功
    ptClient->m_bRegister = true;
    TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_LOGON,false);
    pParser->SetVersion(ptClient->m_tTMdbCSLink.m_iUseOcp);
    CHECK_RET(SendAnswer(pParser,ptClient,0,"LOGIN_OK"),"SendAnswer[LOGIN_OK],failed");
    return iRet;
}

/******************************************************************************
* 函数名称	:  PrintSendMsg
* 函数描述	:  打印发送信息
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbAgentServer::PrintSendMsg(TAgentClient *ptClient)
{

    unsigned char *caTemp = ptClient->m_sSendPackage;
    int iMsgLen = ptClient->m_tHead.iLen;
    //TADD_NORMAL("Send:iMsgLen=[%d],Sequence=[%d],Pno=[%u]",iMsgLen,ptClient->m_tHead.GetSequence(),ptClient->m_iPno);
    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_KICK_LINK:
        {
            TADD_NORMAL(": /*--------SendDeleteSessionCSPPackage--------*/");
            break;
        }
        case CSP_APP_SEND_SQL:
        {
            TADD_NORMAL(": /*----------SendSQLResultCSPPackage----------*/");
            break;
        }
        case CSP_APP_SEND_PARAM:
        {
            TADD_NORMAL(": /*----------SendSQLParameterCSPPackage----------*/");
            break;
        }
         case CSP_APP_SEND_PARAM_ARRAY:
        {
            TADD_NORMAL(": /*----------SendSQLParameArrayCSPPackage----------*/");
            break;
        }
        case CSP_APP_ACTION:
        {
            TADD_NORMAL(": /*----------SendACTIONCSPPackage--------------*/");
            break;

        }
        case CSP_APP_ADJUST_LOG:
        {
            TADD_NORMAL(": /*----------SendModifyLogLevelPackage--------*/");
            break;
        }
        case CSP_APP_NEXT:
        {
            TADD_NORMAL(": /*----------SendSQLResultCSPPackage-----------");
            break;
        }
        case CSP_APP_C_TABLE:
        {
            TADD_NORMAL(": /*----------SendCreateTableCSPPackage--------*/");
            break;
        }
        case CSP_APP_C_USER:
        {
            TADD_NORMAL(": /*----------SendCreateUserCSPPackage---------*/");
            break;
        }
        case CSP_APP_D_TABLE:
        {
            TADD_NORMAL(": /*----------SendDropTableCSPPackage----------*/");
            break;
        }
        case CSP_APP_D_USER:
        {
            TADD_NORMAL(": /*----------SendDropUserCSPPackage-----------*/");
            break;
        }
        case CSP_APP_GET_SEQUENCE:
        {
            TADD_NORMAL(": /*----------SendGetSequenceCSPPackage--------*/");
            break;
        }
        case CSP_APP_LOGON:
        {
            TADD_NORMAL(": /*----------SendLogOnCSPPackage--------------*/");
            break;
        }
        case CSP_APP_ERROR:
        {
            TADD_NORMAL(": /*----------SendERRORCSPPackage--------------*/");
            break;
        }
        case CSP_APP_QMDB_INFO:
        {
            TADD_NORMAL(": /*----------SendQMDBINFOPackage--------------*/");
            break;
        }
        default:
        {
            TADD_ERROR(ERROR_UNKNOWN,"Send Invalid CSPPackage,CommandCode=[%d]",ptClient->m_tHead.iCmdCode);
            break;
        }
    }
    PrintMsg(ptClient,caTemp,iMsgLen);
    return 0;
}

int TMdbAgentServer::PrintRecvMsg(TAgentClient *ptClient)
{
    unsigned char *caTemp = ptClient->m_sRecvPackage;
    int iMsgLen = ptClient->m_tHead.iLen;
    //TADD_NORMAL("Recv:iMsgLen=[%d],Sequence=[%d],Pno=[%u]",iMsgLen,ptClient->m_tHead.GetSequence(),ptClient->m_iPno);
    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_KICK_LINK:
        {
            TADD_NORMAL(": /*--------RecvDeleteSessionCSPPackage--------*/");
            break;
        }
        case CSP_APP_SEND_SQL:
        {
            TADD_NORMAL(": /*----------RecvSQLStatementPackage----------*/");
            break;
        }
        case CSP_APP_SEND_PARAM:
        {
            TADD_NORMAL(": /*----------RecvSQLParameterCSPPackage----------*/");
            break;
        }
        case CSP_APP_ACTION:
        {
            TADD_NORMAL(": /*-----------RecvTransactionPackage----------*/");
            break;
        }
        case CSP_APP_ADJUST_LOG:
        {
            TADD_NORMAL(": /*----------RecvModifyLogLevelPackage--------*/");
            break;
        }
        case CSP_APP_NEXT:
        {
            TADD_NORMAL(": /*----------RecvNextSQLOperPackage-----------*/");
            break;
        }
        case CSP_APP_C_TABLE:
        {
            TADD_NORMAL(": /*----------RecvCreateTableCSPPackage--------*/");
            break;
        }
        case CSP_APP_C_USER:
        {
            TADD_NORMAL(": /*----------RecvCreateUserCSPPackage---------*/");
            break;
        }
        case CSP_APP_D_TABLE:
        {
            TADD_NORMAL(": /*----------RecvDropTableCSPPackage----------*/");
            break;
        }
        case CSP_APP_D_USER:
        {
            TADD_NORMAL(": /*----------RecvDropUserCSPPackage-----------*/");
            break;
        }
        case CSP_APP_GET_SEQUENCE:
        {
            TADD_NORMAL(": /*----------RecvGetSequenceCSPPackage--------*/");
            break;
        }
        case CSP_APP_SEND_PARAM_ARRAY:
        {
            TADD_NORMAL(": /*----------RecvSQLParameArrayCSPPackage--------*/");
            break;
        }
        case CSP_APP_QMDB_INFO:
        {
            TADD_NORMAL(": /*----------RecvGetQMDBInfoCSPPackage--------*/");
            break;
        }
        case CSP_APP_LOGON:
        {
            TADD_NORMAL(": /*----------RecvLogOnCSPPackage--------------*/");
            break;
        }
        default:
        {
            TADD_ERROR(ERROR_UNKNOWN,"Recve Invalid CSPPackage,CommandCode=[%d]",ptClient->m_tHead.iCmdCode);
            break;
        }
    }
    PrintMsg(ptClient,caTemp,iMsgLen);
    return 0;
}
/******************************************************************************
* 函数名称	:  PrintMsg
* 函数描述	:  打印发送信息
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TMdbAgentServer::PrintMsg(TAgentClient *ptClient,unsigned char *caTemp,int iMsgLen)
{
    char sTemp[64]= {0};
    TADD_NORMAL(":iMsgLen=[%d],Sequence=[%d],Pno=[%u]",iMsgLen,ptClient->m_tHead.GetSequence(),ptClient->m_iPno);
    for(int i=0; i<iMsgLen; ++i)
    {
        if(i%16 == 0&& i!=0 )
        {
            TADD_NORMAL(":%s",sTemp);
            memset(sTemp,'\0',64);
        }
        snprintf(&sTemp[strlen(sTemp)],sizeof(sTemp),"%02x ", caTemp[i]);
    }
    TADD_NORMAL(":%s",sTemp);
}

/******************************************************************************
* 函数名称	:  DumpSendPackage
* 函数描述	:  发送数据包
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TMdbAgentServer::DumpSendPackage(TAgentClient *ptClient)
{
    unsigned int iSize = sizeof(ptClient->m_sPackageBuffer);
    unsigned int iCount = strlen(ptClient->m_sPackageBuffer);
    char *pPackageBuffer = ptClient->m_sPackageBuffer;
    int iMsgLen = ptClient->m_tHead.iLen;

    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_KICK_LINK:
        {
            strncat(pPackageBuffer, " SendDeleteSessionCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_SEND_SQL:
        {
            strncat(pPackageBuffer, " SendSQLResultCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_SEND_PARAM:
        {
            strncat(pPackageBuffer, " SendSQLParameterCSPPackage ", iSize-iCount);
            break;
        }
         case CSP_APP_SEND_PARAM_ARRAY:
        {
            strncat(pPackageBuffer, " SendSQLParameArrayCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_ACTION:
        {
            strncat(pPackageBuffer, " SendACTIONCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_ADJUST_LOG:
        {
            strncat(pPackageBuffer, " SendModifyLogLevelPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_NEXT:
        {
            strncat(pPackageBuffer, " SendSQLResultCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_C_TABLE:
        {
            strncat(pPackageBuffer, " SendCreateTableCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_C_USER:
        {
            strncat(pPackageBuffer, " SendCreateUserCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_D_TABLE:
        {
            strncat(pPackageBuffer, " SendDropTableCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_D_USER:
        {
            strncat(pPackageBuffer, " SendDropUserCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_GET_SEQUENCE:
        {
            strncat(pPackageBuffer, " SendGetSequenceCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_LOGON:
        {
            strncat(pPackageBuffer, " SendLogOnCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_ERROR:
        {
            strncat(pPackageBuffer, " SendERRORCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_QMDB_INFO:
        {
            strncat(pPackageBuffer, " SendQMDBINFOPackage ", iSize-iCount);
            break;
        }
        default:
        {
            break;
        }
    }
    
    for(int i=0; i<iMsgLen; ++i)
    {
        iCount = strlen(pPackageBuffer);
        snprintf(&pPackageBuffer[iCount], iSize-iCount, "%02x ", ptClient->m_sSendPackage[i]);
    }
    return;

}

/******************************************************************************
* 函数名称	:  DumpRecvPackage
* 函数描述	:  接收数据包
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TMdbAgentServer::DumpRecvPackage(TAgentClient *ptClient)
{
    unsigned int iSize = sizeof(ptClient->m_sPackageBuffer);
    unsigned int iCount = strlen(ptClient->m_sPackageBuffer);
    char *pPackageBuffer = ptClient->m_sPackageBuffer;
    int iMsgLen = ptClient->m_tHead.iLen;
    
    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_KICK_LINK:
        {
            strncat(pPackageBuffer, " RecvDeleteSessionCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_SEND_SQL:
        {
            strncat(pPackageBuffer, " RecvSQLStatementPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_SEND_PARAM:
        {
            strncat(pPackageBuffer, " RecvSQLParameterCSPPackage ", iSize-iCount);
            break;
        }
         case CSP_APP_SEND_PARAM_ARRAY:
        {
            strncat(pPackageBuffer, " RecvSQLParameArrayCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_ACTION:
        {
            strncat(pPackageBuffer, " RecvTransactionPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_ADJUST_LOG:
        {
            strncat(pPackageBuffer, " RecvModifyLogLevelPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_NEXT:
        {
            strncat(pPackageBuffer, " RecvNextSQLOperPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_C_TABLE:
        {
            strncat(pPackageBuffer, " RecvCreateTableCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_C_USER:
        {
            strncat(pPackageBuffer, " RecvCreateUserCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_D_TABLE:
        {
            strncat(pPackageBuffer, " RecvDropTableCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_D_USER:
        {
            strncat(pPackageBuffer, " RecvDropUserCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_GET_SEQUENCE:
        {
            strncat(pPackageBuffer, " RecvGetSequenceCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_LOGON:
        {
            strncat(pPackageBuffer, " RecvLogOnCSPPackage ", iSize-iCount);
            break;
        }
        case CSP_APP_QMDB_INFO:
        {
            strncat(pPackageBuffer, " RecvGetQMDBInfoCSPPackage ", iSize-iCount);
            break;
        }
        default:
        {
            break;
        }
    }
    
    for(int i=0; i<iMsgLen; ++i)
    {
        iCount = strlen(pPackageBuffer);
        snprintf(&pPackageBuffer[iCount], iSize-iCount, "%02x ", ptClient->m_sRecvPackage[i]); 
        
    }
    return;
    
}

/******************************************************************************
* 函数名称	:  DumpPackage
* 函数描述	: 数据包落地
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
void TMdbAgentServer::DumpPackage(TAgentClient *ptClient, bool bSendFlag)
{
    if(NULL == ptClient) return;
    unsigned int iSize = sizeof(ptClient->m_sPackageBuffer);
    unsigned int iCount = strlen(ptClient->m_sPackageBuffer);
    char *pPackageBuffer = ptClient->m_sPackageBuffer;
    int iMsgLen = ptClient->m_tHead.iLen;
    
    if(iCount + iMsgLen*2 + iMsgLen/2 + BUFFER_PLUS_COUNT >= iSize-1)
    {//计算buffer总大小,BUFFER_PLUS_COUNT目前足够大
        m_tAgentFile.Write(pPackageBuffer, iCount);
        memset(pPackageBuffer, 0, iSize);
        iCount = 0;
    }
    
    //包开始{ [%s]SessionID=[%d]Sequence=[%d]
    char strCurrentDateTime[15] = {0};
    TMdbDateTime::GetCurrentTimeStr(strCurrentDateTime);
    snprintf(&pPackageBuffer[iCount], iSize-iCount,SESSION_ID_STR, 
        strCurrentDateTime, ptClient->m_tHead.iSessionId, ptClient->m_tHead.isequence);
        
    //数据信息
    if(bSendFlag)
    {
        DumpSendPackage(ptClient);
    }
    else
    {
        DumpRecvPackage(ptClient);
    }
    //包结束
    iCount = strlen(pPackageBuffer);
    strncat(pPackageBuffer, "}\n", iSize-iCount);
    
    if(iCount+2 == iSize-1)
    {
        m_tAgentFile.Write(pPackageBuffer, iCount+2);
        memset(pPackageBuffer, 0, iSize);
    }
    
    return;
    
}

/******************************************************************************
* 函数名称	:  AppSendSQL
* 函数描述	:  SQL注册
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbAgentServer::AppSendSQL(TAgentClient *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    int iSqlFlag = 0;
    char * sSQL   = NULL;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//解析
        //pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
        iSqlLabel = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        sSQL	  = pParser->GetStringValue(pParser->m_pRootAvpItem,AVP_SQL_STATEMENT);
        if(NULL != pParser->FindExistAvpItem(pParser->m_pRootAvpItem,AVP_SQL_FLAG))
        {//有sql-flag
            iSqlFlag = pParser->GetINT32Value(pParser->m_pRootAvpItem,AVP_SQL_FLAG);
        }
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel,true);//根据SQLLABEL，获取client
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
        snprintf(sMsg, sizeof(sMsg),"QuerySqlLabel[%d] SetSQL OK.",iSqlLabel);
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
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
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
int TMdbAgentServer::AppSendParam(TAgentClient *ptClient)
{
    int iRet      = 0;
    int iSqlLabel = 0;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_PARAM,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);
       // pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
        iSqlLabel         = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
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
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }

    return iRet;
}
/******************************************************************************
* 函数名称	:  AppSendParamArray
* 函数描述	:  处理发送来的参数数组
* 输入		:
* 输入		:
* 输出		:
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbAgentServer::AppSendParamArray(TAgentClient *ptClient)
{
    TADD_FUNC("Start,AppSendParamArray");
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_PARAM_ARRAY,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);
        //pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
         IS_LOG(2){pParser->Print();}
        iSqlLabel         = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
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
        //获取首个BATCH_GROUP
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
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }
    return iRet;
}

//填充查询的next 返回值
int TMdbAgentServer::FillNextResult(TAgentClient *ptClient,TMdbQuery * pQuery,TMdbCspParser * pSendSQLAns)throw(TMdbException,TMdbCSPException)
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
int TMdbAgentServer::AppNextSQLResult(TAgentClient *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_NEXT,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);
        //pParser->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParser->Print();}
        iSqlLabel  = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
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
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }
    return iRet;
}

/******************************************************************************
* 函数名称	:  AppTransAction
* 函数描述	:  事务处理
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbAgentServer::AppTransAction(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        TMdbCspParser * pParserSend = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_ACTION,true);
        CHECK_OBJ(pParserSend);
        pParserSend->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//解析
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
            TADD_ERROR(-1,"sCmd[%s] is not rollback/commit",sCmd);
            iRet = AppErrorAnswer(ptClient,-1,"sCmd[%s] is not rollback/commit",sCmd); 
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
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
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
int TMdbAgentServer::AppGetSequence(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        TMdbCspParser * pParserSend = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_GET_SEQUENCE,true);
        CHECK_OBJ(pParserSend);
        pParserSend->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//解析
        //pParserSend->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParserSend->Print();}
        char * sSeqName = NULL;
        sSeqName = pParserSend->GetStringValue(pParserSend->m_pRootAvpItem,AVP_SEQUENCE_NAME);
        TMdbSequenceMgr * pSeqMgr = ptClient->GetSeqMgrByName(sSeqName,m_pConfig);
        if(NULL == pSeqMgr)
        {
            TADD_ERROR(-1,"not find sequence mgr by [%s].",sSeqName);
            iRet = AppErrorAnswer(ptClient,-1,"not find sequence mgr by [%s].",sSeqName); 
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
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!");
        iRet = AppErrorAnswer(ptClient,ERROR_UNKNOWN,"ERROR_UNKNOWN");
    }
    return iRet;
}

//发送结果信息
int TMdbAgentServer::SendPackageBody(TAgentClient *ptClient,int iSendLen)
{
    int lRet = 0;
    bool bRet = true;
    IS_LOG(1)
    {
        lRet = PrintSendMsg(ptClient);
        if(lRet < 0)
        {
            //没有配置CmdID
            return lRet;
        }
    }
    //记录每次的发送
    memcpy(ptClient->m_tLastSequence.m_sLastSendPackage,ptClient->m_sSendPackage,iSendLen);
    ptClient->m_tLastSequence.m_iLastSendLen = iSendLen;
    TMdbPeerInfo* pPeerInfo = ptClient->m_pEventInfo->pPeerInfo;
    bRet = pPeerInfo->PostMessage(ptClient->m_sSendPackage,iSendLen);
    if(false == bRet)
    {
        TADD_ERROR(ERR_NET_SEND_FAILED,"PostMessage failed,ntc_error[%s]",mdb_ntc_errstr.c_str());
        lRet = PrintSendMsg(ptClient);
        if(lRet < 0)
        {
            //没有配置CmdID
            return lRet;
        }
        TMdbDateTime::MSleep(3000);
        
        bRet = pPeerInfo->PostMessage(ptClient->m_sSendPackage,iSendLen);
        if(false == bRet)
        {
            return ERR_NET_SEND_FAILED;        
        }
        
    }
    //发送数据包落地
    if(m_tProcCtrl.GetProcState() == PDUMP)
    {
        ptClient->m_cProcState = PDUMP;
        DumpPackage(ptClient, true);
    }
    return 0;
}

/******************************************************************************
* 函数名称	:  AppErrorAnswer
* 函数描述	:  错误消息应答
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbAgentServer::AppErrorAnswer(TAgentClient *ptClient,int iAnsCode,const char *fmt,...)
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
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_ERROR,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        return SendAnswer(ptClient, iAnsCode, sTemp);
    }
}
/******************************************************************************
* 函数名称	:  DoOperation
* 函数描述	:  只处理正确的报文
* 输入		:
* 输出		:
* 返回值	:
* 作者		:
*******************************************************************************/
int TMdbAgentServer::DoOperation(TAgentClient *ptClient)
{
    int iRet = 0;
    //状态变更时落地之前的数据包
    char cProcState = m_tProcCtrl.GetProcState();
    if(PDUMP !=  cProcState
        && PDUMP == ptClient->m_cProcState) 
    {
        ptClient->m_cProcState = cProcState;
        m_tAgentFile.Write(ptClient->m_sPackageBuffer, strlen(ptClient->m_sPackageBuffer));
        memset(ptClient->m_sPackageBuffer, 0, sizeof(ptClient->m_sPackageBuffer));
    }
    
    iRet = RecvMsg(ptClient);
    if(iRet == ERR_CSP_KEEP_ON_RECV ) {return 0;}//继续接收
    if(iRet != 0 )
    {
        TADD_NORMAL("RecvMsg ret=[%d],iFD=[%d],Pno=[%d]",iRet,ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno);
        return iRet;
    }

    TADD_DETAIL("DoOperation...iCmdCode=%d",ptClient->m_tHead.iCmdCode);
    if((iRet = CheckClientSequence(ptClient)) != 0)
    {
        TADD_NORMAL("CheckClientSequence ret=[%d],iFD=[%d],Pno=[%d]",iRet,ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno);
        return iRet;
    }
    
    //clock_t tStart = clock();
    
    struct timeval tCurTime,tEndTime;
    tCurTime = m_pDsn->tCurTime;
    time_t costTime = 0;
    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_LOGON:
        {
            CHECK_RET_BREAK(Authentication(ptClient),"Authentication failed.");//认证
            TADD_NORMAL("ThreadID=[%lu],ClientIP=[%s],SequenceID=[%d],SocketID=[%d],Pno=[%d],Protocol=[%d] Login Success!! ",
                    ptClient->m_iThreadId,ptClient->m_tTMdbCSLink.m_sClientIP,ptClient->m_tHead.GetSequence(),
                    ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno,ptClient->m_tTMdbCSLink.m_iUseOcp);
            
            break;
        }
        case CSP_APP_KICK_LINK:  //ok
        {

            break;
        }
        case CSP_APP_SEND_SQL:
        {
            //接受SQL注册
            iRet = AppSendSQL(ptClient);
            CHECK_RET(iRet,"AppSendSQL return Value=[%d]",iRet);
            break;
        }
        case CSP_APP_SEND_PARAM: //0k
        {

            iRet = AppSendParam(ptClient);
            CHECK_RET(iRet,"AppSendParam return Value=[%d]",iRet);
            break;
        }
        case CSP_APP_SEND_PARAM_ARRAY:
        {
            iRet = AppSendParamArray(ptClient);
            CHECK_RET(iRet,"AppSendParamArray return Value=[%d]",iRet);
            break;
        }
        case CSP_APP_ACTION:     //ok
        {
            iRet = AppTransAction(ptClient);
            CHECK_RET(iRet,"AppNextSQLResult return Value=[%d]",iRet);
            break;
        }
        case CSP_APP_ADJUST_LOG:    //ok
        {
            break;
        }
        case CSP_APP_NEXT:          //ok
        {

            iRet = AppNextSQLResult(ptClient);
            CHECK_RET(iRet,"AppNextSQLResult return Value=[%d]",iRet);
            break;

        }
        case CSP_APP_QMDB_INFO://获取qmdb信息
        {
            iRet = AppGetQmdbInfo(ptClient);
            CHECK_RET(iRet,"AppGetQmdbInfo return Value=[%d]",iRet);
            break;
        }
        case  CSP_APP_C_TABLE:    //ok
        {

            break;
        }
        case CSP_APP_C_USER:    //ok
        {


            break;
        }
        case CSP_APP_D_TABLE:   //ok
        {


            break;
        }
        case CSP_APP_D_USER:
        {


            break;
        }
        case CSP_APP_GET_SEQUENCE:
        {
            iRet = AppGetSequence(ptClient);
            CHECK_RET(iRet,"AppGetSequence return Value=[%d]",iRet);
            break;
        }
        default:
        {
            TADD_ERROR(ERROR_UNKNOWN," AVP CommonCode = [%d] not support!",ptClient->m_tHead.iCmdCode);
            return iRet;
        }
    }
    tEndTime = m_pDsn->tCurTime;
    costTime = tEndTime.tv_sec - tCurTime.tv_sec;
    if(costTime >= 3)
    {
        TADD_WARNING("CmdCode=[%u],ThreadID=[%lu], ClientIP=[%s],SequenceID=[%d] DoOperation Time[%d] > 3s ",
                ptClient->m_tHead.iCmdCode,ptClient->m_iThreadId,ptClient->m_tTMdbCSLink.m_sClientIP,ptClient->m_tHead.GetSequence(),costTime);
    }
    
    TADD_FUNC("end iRet=%d",iRet);
    return iRet;
}
//二进制
int TMdbAgentServer::DoOperationBin(TAgentClient *ptClient)
{
    int iRet = 0;
    //状态变更时落地之前的数据包
    char cProcState = m_tProcCtrl.GetProcState();
    if(PDUMP !=  cProcState
        && PDUMP == ptClient->m_cProcState) 
    {
        ptClient->m_cProcState = cProcState;
        m_tAgentFile.Write(ptClient->m_sPackageBuffer, strlen(ptClient->m_sPackageBuffer));
        memset(ptClient->m_sPackageBuffer, 0, sizeof(ptClient->m_sPackageBuffer));
    }
    
    iRet = RecvMsg(ptClient);
    if(iRet == ERR_CSP_KEEP_ON_RECV ) {return 0;}//继续接收
    if(iRet != 0 )
    {
        TADD_NORMAL("RecvMsg ret=[%d],iFD=[%d],Pno=[%d]",iRet,ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno);
        return iRet;
    }

    TADD_DETAIL("DoOperation...iCmdCode=%d",ptClient->m_tHead.iCmdCode);
    if((iRet = CheckClientSequence(ptClient)) != 0)
    {
        TADD_NORMAL("CheckClientSequence ret=[%d],iFD=[%d],Pno=[%d]",iRet,ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno);
        return iRet;
    }
    
    //clock_t tStart = clock();
    
    struct timeval tCurTime,tEndTime;
    tCurTime = m_pDsn->tCurTime;
    time_t costTime = 0;
    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_LOGON:
        {
            CHECK_RET_BREAK(AuthenticationBin(ptClient),"Authentication failed.");//认证
            TADD_NORMAL("ThreadID=[%lu],ClientIP=[%s],SequenceID=[%d],SocketID=[%d],Pno=[%d] Login Success!! ",
                    ptClient->m_iThreadId,ptClient->m_tTMdbCSLink.m_sClientIP,ptClient->m_tHead.GetSequence(),
                    ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno);
            
            break;
        }
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
        }
        case CSP_APP_GET_SEQUENCE:
        {
            iRet = AppGetSequenceBin(ptClient);
            CHECK_RET(iRet,"AppGetSequence return Value=[%d]",iRet);
            break;
        }
        default:
        {
            TADD_ERROR(ERROR_UNKNOWN," AVP CommonCode = [%d] not support!",ptClient->m_tHead.iCmdCode);
            return iRet;
        }
    }
    //clock_t tFinish = clock();
    //double duration = (double)(tFinish - tStart)/CLOCKS_PER_SEC;
    
    tEndTime = m_pDsn->tCurTime;
    costTime = tEndTime.tv_sec - tCurTime.tv_sec;
    if(costTime >= 3)
    {
        TADD_WARNING("CmdCode=[%u],ThreadID=[%lu], ClientIP=[%s],SequenceID=[%d] DoOperation Time[%d] > 3s ",
                ptClient->m_tHead.iCmdCode,ptClient->m_iThreadId,ptClient->m_tTMdbCSLink.m_sClientIP,ptClient->m_tHead.GetSequence(),costTime);
    }
    
    TADD_FUNC("end iRet=%d",iRet);
    return iRet;
}

/******************************************************************************
* 函数名称	:  SendAnswer
* 函数描述	:  发送响应包
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbAgentServer::SendAnswer(TMdbCspParser * pCspParser,TAgentClient *ptClient,int iAnsCode,const char * sMsg)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pCspParser);
    try
    {
        //TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(iAnsType,false);
        //CHECK_OBJ(pParser);
        pCspParser->SetItemValue(pCspParser->m_pRootAvpItem,AVP_ANSWER_CODE,iAnsCode);
        pCspParser->SetItemValue(pCspParser->m_pRootAvpItem,AVP_ANSWER_MSG,sMsg);
        pCspParser->Serialize(ptClient->m_sSendPackage,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        IS_LOG(2){pCspParser->Print();}
        ptClient->m_tHead = pCspParser->m_tHead;
        CHECK_RET(SendPackageBody(ptClient,pCspParser->m_tHead.iLen),"SendPackageBody return value=[%d]",iRet);
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

/******************************************************************************
* 函数名称	:  CheckClientSequence
* 函数描述	:  检测客户端序号
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbAgentServer::CheckClientSequence(TAgentClient * ptClient)
{
    int iRet = 0;
    CHECK_OBJ(ptClient);
    TAgentClientSequence & tLastSeq = ptClient->m_tLastSequence;
    if(ptClient->m_tHead.GetSequence() < 0)
    {
        CHECK_RET_SEND_ANS(ERR_CSP_SEQ_VALUE,ptClient,"Client sequence can not be < 0");
    }
    if((int) ptClient->m_tHead.GetSequence() == tLastSeq.m_iLastSequence)
    {//需要重发
        TADD_WARNING("Client Sequence[%d] = Last sequence,need to resend.",ptClient->m_tHead.GetSequence());
        memcpy(ptClient->m_sSendPackage,tLastSeq.m_sLastSendPackage,tLastSeq.m_iLastSendLen);
        CHECK_RET(SendPackageBody(ptClient,tLastSeq.m_iLastSendLen),"SendPackageBody error.");
        return ptClient->m_tHead.GetSequence();
    }
    else
    {
        tLastSeq.m_iLastSequence = ptClient->m_tHead.GetSequence();
    }
    return iRet;
}
/******************************************************************************
* 函数名称	:  AppGetQmdbInfo
* 函数描述	:  获取qmdb 信息包
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbAgentServer::AppGetQmdbInfo(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        TMdbCspParser * pParserQmdbInfo = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_QMDB_INFO,true);
        CHECK_OBJ(pParserQmdbInfo);
        pParserQmdbInfo->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//解析
       // pParserQmdbInfo->m_tHead = ptClient->m_tHead;//head 解析结果。
        IS_LOG(2){pParserQmdbInfo->Print();}
        char * sCmd = NULL;
        sCmd = pParserQmdbInfo->GetStringValue(pParserQmdbInfo->m_pRootAvpItem,AVP_COMMAND_NAME);
        TADD_DETAIL("cmd=[%s]",sCmd);
        TMdbInfoCollect mdbInfoCollect;
        mdbInfoCollect.Attach(m_pDsn->sName);
        std::string str;
        mdbInfoCollect.GetInfo(str,sCmd);
        TMdbCspParser * pParserQmdbInfoAns = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_QMDB_INFO,false);
        CHECK_RET(SendAnswer(pParserQmdbInfoAns,ptClient,0,str.c_str()),"SendAnswer error.");
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

int TMdbAgentServer::AuthenticationBin(TAgentClient *ptClient)
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
        
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
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
    TMdbAgentServer::m_iSessionStep++;
    if(TMdbAgentServer::m_iSessionStep >= INT_MAX)
    {
        m_iSessionStep = 1;
    }
    ptClient->m_iSessionID = m_iSessionStep;
    CHECK_RET(tMutex.UnLock(true),"UnLock Faild");
    //回复登陆成功
    //head+anscode+msg
    ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_LOGON,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
    CHECK_RET(SendAnswer(ptClient,0,"LOGIN_OK"),"SendAnswer[LOGIN_OK],failed");
    return iRet;
}

//bin
int TMdbAgentServer::AppSendSQLBin(TAgentClient *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    int iSqlFlag = 0;
    char iTmpCh = 0;
    short int iTmpSh = 0;
    //char sSQL[MAX_SQL_LEN]   = "";
    try
    {
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        char *pSQL = ptClient->m_tRecvDataParse.GetDataPtr() + NO_OCP_SQL_POS;
        //ptClient->m_tRecvDataParse.GetData(sSQL,NO_OCP_SQL_POS,ptClient->m_tHead.iLen - NO_OCP_SQL_POS);
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
           CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        pQuery->SetSQL(pSQL,iSqlFlag,0);//设置SQL
        TADD_NORMAL("QuerySqlLabel=[%d],SetSQL(%s),IsDynamic=[%s],SqlFlag=[%d]",iSqlLabel,pSQL,(pQuery->ParamCount() != 0)?"TRUE":"FALSE",iSqlFlag);
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_SEND_SQL,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        pClientQuery->m_bFirstNext = true;
        if(pQuery->ParamCount() != 0)
        {
            //动态SQL
            iTmpCh = 0;
            iTmpSh = 0;
            ptClient->m_tSendDataParse.SetData(&iTmpCh,sizeof(char));//AVP_SELECT_HAVE_NEXT
            ptClient->m_tSendDataParse.SetData(&iTmpSh,sizeof(short int));//AVP_AFFECTED_ROW
        }
        else
        {
            //静态SQL
            if(TK_SELECT == pQuery->GetSQLType())
            {
                pQuery->Open();
                TADD_FLOW("QuerySqlLabel=[%d],Open(),StaticSQL.",iSqlLabel);
                CHECK_RET(FillNextResult(ptClient,pQuery,pClientQuery->m_bFirstNext),"FillNextResult error");
            }
            else
            {
                pQuery->Execute();
                iTmpCh = 0;
                ptClient->m_tSendDataParse.SetData(&iTmpCh,sizeof(char));//AVP_SELECT_HAVE_NEXT
                iTmpSh = (short int)pQuery->RowsAffected();
                ptClient->m_tSendDataParse.SetData(&iTmpSh,sizeof(short int));//AVP_AFFECTED_ROW
                
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

int TMdbAgentServer::AppSendParamBin(TAgentClient *ptClient)
{
    int iRet      = 0;
    int iSqlLabel = 0;
    try
    {
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
            CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        char iParamCount = 0;
        short int iParamPackageLen = 0;
        char iParamIndex = 0;
        int iParamPos = 0;
        //head+sqllabel+cnt+packageLen+paramIndex+Value
        //packageLen = sizeof(int) * 2 -> null
        ptClient->m_tRecvDataParse.GetData(&iParamCount,NO_OCP_PARAMCNT_POS,sizeof(char));
        iParamPos += NO_OCP_PARAMCNT_POS + sizeof(char); 
        for(int i=0; i<iParamCount; ++i)
        {
            ptClient->m_tRecvDataParse.GetData(&iParamPackageLen,(size_t)iParamPos,sizeof(short int));
            ptClient->m_tRecvDataParse.GetData(&iParamIndex,iParamPos+sizeof(short int),(int)sizeof(char));
            if(iParamPackageLen == (sizeof(short int)+sizeof(char)) )
            {//NULL
                pQuery->SetParameterNULL(iParamIndex);
            }
            else
            {
                char *pData = ptClient->m_tRecvDataParse.GetDataPtr() + iParamPos + sizeof(short int)+sizeof(char);
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
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_SEND_SQL,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        //ptClient->m_bFirstNext = true;
        if(pQuery->GetSQLType() == TK_SELECT)
        {
            pQuery->Open();
            CHECK_RET(FillNextResult(ptClient,pQuery,pClientQuery->m_bFirstNext),"FillNextResult error");
        }
        else
        {//
            pQuery->Execute();
            int iTmp = 0;
            ptClient->m_tSendDataParse.SetData(&iTmp,sizeof(char));//AVP_SELECT_HAVE_NEXT
            short int iTmpSh = (short int)pQuery->RowsAffected();
            ptClient->m_tSendDataParse.SetData(&iTmpSh,sizeof(short int));//AVP_AFFECTED_ROW
        
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

/***
AppSendParamArray
->
	head 43
	sqlabel 4
	batchcount 4
	paramcount 4
	PackageLen (=4,null)
	Value 

***/
int TMdbAgentServer::AppSendParamArrayBin(TAgentClient *ptClient)
{
    TADD_FUNC("Start,AppSendParamArray");
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
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

        short int iBatchCount = 0;
        char iParamCount = 0;
        short int iParamPackageLen = 0;
        char iParamIndex = 0;
        int iParamPos = 0;
        short int iRowEffect = 0;
        
        //head+sqllabel+batchcnt+pramcount+packageLen+Value
        //packageLen = sizeof(int) * 2 -> null
        ptClient->m_tRecvDataParse.GetData(&iBatchCount,NO_OCP_BATCHCNT_POS,sizeof(short int));
        ptClient->m_tRecvDataParse.GetData(&iParamCount,NO_OCP_BATCHCNT_POS+sizeof(short int),sizeof(char));
        iParamPos += NO_OCP_BATCHCNT_POS + sizeof(short int) + sizeof(char); 
        
        //获取首个BATCH_GROUP
        for(short int iBatchIndex = 0; iBatchIndex < iBatchCount; ++iBatchIndex)
        {
            //一个批次
            for(iParamIndex=0; iParamIndex<iParamCount; ++iParamIndex)
            {
                ptClient->m_tRecvDataParse.GetData(&iParamPackageLen,(size_t)iParamPos,sizeof(short int));
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
            iRowEffect += (short int)pQuery->RowsAffected();//执行
        }

        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_SEND_SQL,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
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

int TMdbAgentServer::FillNextResult(TAgentClient *ptClient,TMdbQuery * pQuery,bool &bFirstNext)throw(TMdbException,TMdbCSPException)
{
    int iRet = 0;
    short int iCount = 0;
    char iTmpCh = 0;
    short int iTmpSh = 0;
    short int iValueLen = 0;
    char cDataType = 0;
    bool bHaveNext = true;
    bool bFirstNextTmp = bFirstNext;
    //bool bSetFieldCount = false;
    //bool bFirst = false;
    //head+sqllabel+haveNext+recordCount+fieldCount+nameLen+fieldName+valueLen+Value
    //fieldLen=4 -> null, value no
    ptClient->m_tSendDataParse.SetData(&iTmpCh,sizeof(char));//have next
    ptClient->m_tSendDataParse.SetData(&iTmpSh,sizeof(short int));//记录个数
    
    while(1)
    {
        if(pQuery->Next())
        {
            iCount ++;
            int j = 0;
            if(bFirstNextTmp)
            {
                iTmpCh = (char)pQuery->FieldCount();
                ptClient->m_tSendDataParse.SetData(&iTmpCh,sizeof(char));//字段个数
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
            for(j = 0; j<pQuery->FieldCount(); j++)
            {
                cDataType = pQuery->Field(j).GetDataType();
                if(bFirstNextTmp)
                {
                    iValueLen = (short int)(strlen(pQuery->Field(j).GetName())+1);
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
                    iValueLen = (short int)sizeof(lData);
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));
                    ptClient->m_tSendDataParse.SetData(&lData,iValueLen);
                }
                else
                {
                    char *pData = pQuery->Field(j).AsRealStr();
                    iValueLen = (short int)strlen(pData) + 1;
                    ptClient->m_tSendDataParse.SetData(&iValueLen,sizeof(short int));
                    ptClient->m_tSendDataParse.SetData(pData,iValueLen);
                }
            }
            //#endif
            bFirstNextTmp = false;
            bFirstNext = false;
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
    
    //ptClient->m_tSendDataParse.SetSize();
	//bNext = bHaveNext;
    ptClient->m_tSendDataParse.SetData(&iCount,SIZE_MSG_BIN_HEAD+5,sizeof(short int));//记录个数
    if(bHaveNext)
    {
        iTmpCh = 1;
        ptClient->m_tSendDataParse.SetData(&iTmpCh,SIZE_MSG_BIN_HEAD+4,sizeof(char));//have next
    }
    else
    {
        iTmpCh = 0 ;
        ptClient->m_tSendDataParse.SetData(&iTmpCh,SIZE_MSG_BIN_HEAD+4,sizeof(char));//have next
    }
    return iRet;
}

//响应next操作
int TMdbAgentServer::AppNextSQLResultBin(TAgentClient *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(&iSqlLabel,NO_OCP_SQLLABEL_POS,sizeof(int));
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//根据SQLLABEL，获取client
        if(NULL == pClientQuery)
        {
            CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_SEND_SQL,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        ptClient->m_tSendDataParse.SetData(&iSqlLabel,sizeof(int));
        if(pQuery->GetSQLType() == TK_SELECT)
        {
            CHECK_RET(FillNextResult(ptClient,pQuery,pClientQuery->m_bFirstNext),"FillNextResult error");
            SendAnswer(ptClient,0,"AppNextSQLResult OK");
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

int TMdbAgentServer::AppTransActionBin(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        char sCmd[32]="";
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(sCmd,SIZE_MSG_BIN_HEAD,ptClient->m_tHead.iLen - SIZE_MSG_BIN_HEAD);
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
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_ACTION,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
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

int TMdbAgentServer::AppGetQmdbInfoBin(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        char sCmd[32]="";
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(sCmd,SIZE_MSG_BIN_HEAD,(int)(ptClient->m_tHead.iLen - SIZE_MSG_BIN_HEAD));
        TMdbInfoCollect mdbInfoCollect;
        mdbInfoCollect.Attach(m_pDsn->sName);
        std::string str;
        mdbInfoCollect.GetInfo(str,sCmd);
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_QMDB_INFO,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
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

int TMdbAgentServer::AppGetSequenceBin(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        char sSeqName[64]="";
        ptClient->m_tRecvDataParse.InitDataSrc((char*)ptClient->m_sRecvPackage);
        ptClient->m_tRecvDataParse.GetData(sSeqName,SIZE_MSG_BIN_HEAD,(int)(ptClient->m_tHead.iLen - SIZE_MSG_BIN_HEAD));
        TMdbSequenceMgr * pSeqMgr = ptClient->GetSeqMgrByName(sSeqName,m_pConfig);
        if(NULL == pSeqMgr)
        {
            TADD_ERROR(-1,"not find sequence mgr by [%s].",sSeqName);
            iRet = AppErrorAnswer(ptClient,-1,"not find sequence mgr by [%s].",sSeqName); 
            return iRet;
        }
        
        ptClient->m_tSendDataParse.SerializeHead((char*)ptClient->m_sSendPackage,CSP_APP_GET_SEQUENCE,ptClient->m_iSessionID,ptClient->m_tHead.GetSequence());
        long long llValue = pSeqMgr->m_Sequence.GetNextIntVal();
        ptClient->m_tSendDataParse.SetData(&llValue,sizeof(llValue));
        CHECK_RET(SendAnswer(ptClient,0,"OK"),"SendAnswer error.");
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

int TMdbAgentServer::SendAnswer(TAgentClient *ptClient,int iAnsCode,const char * sMsg)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    try
    {
        short int iAnsCodeSh = (short int)iAnsCode;
        ptClient->m_tSendDataParse.SetDataAnsCode(&iAnsCodeSh,sizeof(short int));
        if(sMsg)
        {
            ptClient->m_tSendDataParse.SetData((char*)sMsg,(int)(strlen(sMsg)+1));
        }
        ptClient->m_tSendDataParse.SetSize();
        ptClient->m_tHead.iLen = (unsigned int)ptClient->m_tSendDataParse.GetSize();
        CHECK_RET(SendPackageBody(ptClient,ptClient->m_tSendDataParse.GetSize()),"SendPackageBody return value=[%d]",iRet);
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
//}
