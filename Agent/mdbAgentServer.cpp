/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbAgentServer.cpp
*@Description�� �������miniDB��Զ�̷��ʴ���ӿ�
*@Author:       li.shugang jiang.mingjun
*@Date��        2009��05��23��
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

//namespace QuickMDB{

#define PARTIAL_NEXT_SEND_NUM 1000
#define SESSION_ID_STR "{[%s] SessionID=[%d],Sequence=[%d] "
#define BUFFER_PLUS_COUNT 200 

// //���ݰ������е������ַ�--һЩ˵�����ֱ���SESSION_ID_STR
#define DUMP_FILE_SIZE 128//����ץ������ļ�������ֽ�


//���ret ���ش����
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
ClientQuery::ClientQuery()
{
    m_pQuery = NULL;
    m_iSqlLabel = -1;
}

ClientQuery::~ClientQuery()
{
    SAFE_DELETE(m_pQuery);
}

/******************************************************************************
* ��������	:  CalcMaxNextNum
* ��������	:  �������next����
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
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
* ��������	:  Init
* ��������	:  ��ʼ��
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
* ��������	:  TAgentClient
* ��������	:  ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
}

void TAgentClient::Print()
{

}
/******************************************************************************
* ��������	:  ~TAgentClient
* ��������	:  ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
TAgentClient::~TAgentClient()
{
    Clear();
}
/******************************************************************************
* ��������	:  Clear
* ��������	:  ��������
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
    
}


/******************************************************************************
* ��������	:  GetClientQuery
* ��������	:  ��ȡClientQuery
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
ClientQuery *  TAgentClient::GetClientQuery(int iSqlLabel)
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
    if(m_vClientQuery.size() > MAX_PREPARE_NUM)
    {
        TADD_WARNING("To many Prepare Query,num=[%d].",m_vClientQuery.size());
    }
    std::vector<ClientQuery *>::iterator itor = m_vClientQuery.begin();
    for(;itor != m_vClientQuery.end();++itor)
    {
        if(iSqlLabel == (*itor)->m_iSqlLabel)
        {//�ҵ�
            pRetClientQuery = (*itor);
			break;
        }
    }
    if(NULL == pRetClientQuery)
    {//û����ѹ���µ�
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
        {//�ҵ�
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
* ��������	:  GetSeqMgrByName
* ��������	:  ��ȡTMdbSequenceMgr
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
    for( i=0; i<MAX_SEQUENCE_NUM; i++) //�ȴӻ������
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

    for( i=0; i<MAX_SEQUENCE_NUM; i++) //�����в����ڣ�new
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
* ��������	:  TAgentFile
* ��������	:  ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
TAgentFile::TAgentFile()
{
    m_fp = NULL;
    memset(m_sFileName, 0, sizeof(m_sFileName));
}

/******************************************************************************
* ��������	:  TAgentFile
* ��������	:  ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
TAgentFile::~TAgentFile()
{
    SAFE_CLOSE(m_fp);
}

/******************************************************************************
* ��������	:  Init
* ��������	:  ��ʼ��
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
int TAgentFile::Init()
{

#ifdef WIN32
        snprintf(m_sFileName, sizeof(m_sFileName), "%s\\log\\mdbAgent_package", getenv("QuickMDB_HOME"));
        //�ж�Ŀ¼�Ƿ����.
#else
        snprintf(m_sFileName, sizeof(m_sFileName), "%s/log/mdbAgent_package", getenv("QuickMDB_HOME"));
#endif
    return Open(m_sFileName);
}

/******************************************************************************
* ��������	:  Open
* ��������	: ��
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
* ��������	:  Write
* ��������	: д�ļ�
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
* ��������	:  checkAndBackup
* ��������	: ��鱸��
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
int TAgentFile::checkAndBackup()
{
    char     sFileNameOld[MAX_PATH_NAME_LEN]="";
    struct stat buf;
    //�ж��ļ��Ƿ����
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

        //����־�ļ�������
        TMdbNtcFileOper::Rename(m_sFileName,sFileNameOld);

        TMdbNtcFileOper::Remove(m_sFileName);
        return Open(m_sFileName);
    }
    return 0;
}

/******************************************************************************
* ��������	:  TMdbAgentServer
* ��������	:  ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
* ��������  :  OnConnect
* ��������  :  ����socket���ӻ���socket���ӳɹ�ʱ�������˺���
* ����      :  �¼���Ϣ\�¼���
* ���      :
* ����ֵ    :
* ����      :
*******************************************************************************/
bool TMdbAgentServer::OnConnect(TMdbConnectEvent * pEventInfo,TMdbEventPump * pEventPump)
{
    TADD_NORMAL("Find a new connection from[%s:%u],Pno[%d],Client count=[%d]", 
            pEventInfo->pPeerInfo->GetRemoteAddrInfo().c_str(),pEventInfo->pPeerInfo->GetRemotePort(),pEventInfo->pPeerInfo->pno,GetPeerCount()); 
    //AddWorkThread();//���ӹ����߳�
    //����TAgentClient
    if(NULL == GetFreeAgentClient(pEventInfo))
    {
        TADD_ERROR(ERROR_UNKNOWN, "Get AgentClient error");
        return false;
    }
    return true;
}

/******************************************************************************
* ��������  :  OnDisconnect
* ��������  :  �������ӶϿ�ʱ�������˺���
* ����      :  �¼���Ϣ\�¼���
* ���      :
* ����ֵ    :
* ����      :
*******************************************************************************/
bool TMdbAgentServer::OnDisconnect(TMdbDisconnectEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    int iPno = pEventInfo->pPeerInfo->pno;
    TADD_NORMAL("Disconnect quest from[%s:%d],Pno[%d],Client count=[%d] now", 
            m_ptClient[iPno]->m_tTMdbCSLink.m_sClientIP,m_ptClient[iPno]->m_iPort,iPno,GetPeerCount()); 
    //�˳�ʱ������ݰ�
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
* ��������  :  OnError
* ��������  :  ����������ʱ�������˺������������Ϣ�������
* ����      :  �¼���Ϣ\�¼���
* ���      :
* ����ֵ    :
* ����      :
*******************************************************************************/
bool TMdbAgentServer::OnError(TMdbErrorEvent* pEventInfo, TMdbEventPump* pEventPump)
{
    TADD_ERROR(ERROR_UNKNOWN, "Peer count=[%d],Error:%s", 
            GetPeerCount(), 
            pEventInfo->sErrorInfo.c_str());

    return true;
}

/******************************************************************************
* ��������  :  OnTimeOut
* ��������  :  �����г�ʱʱ�������˺���
* ����      :  �¼���Ϣ\�¼���
* ���      :
* ����ֵ    :
* ����      :
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
* ��������  :  OnRecvMsg
* ��������  :  ���ͻ�������Ϣ����ʱ�򣬴����˺���
* ����      :  �¼���Ϣ\�¼���
* ���      :
* ����ֵ    :
* ����      :
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
    int iRet = DoOperation(m_ptClient[iPno]);
    if(iRet != 0)
    {
        TADD_ERROR(ERROR_UNKNOWN, "DoOperation return Value=[%d]", iRet);
        AppErrorAnswer(m_ptClient[iPno],iRet,"ERR_CODE=[%d],DoOperation error",iRet);
        return false;
    }
    
    m_bRecvFlag = false;
    return true;
}

//�������Ƶ�ip�б�
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
            if(tSplitNext.GetFieldCount() == 4 && strlen(tSplit[i]) < MAX_IP_LEN)//ip ��4λ
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
            if(tSplitNext.GetFieldCount() == 4 && strlen(tSplit[i]) < MAX_IP_LEN)//ip ��4λ
            {
                m_vInValidIP.push_back(string(tSplit[i]));
            }
        }
        
    }
    return;
    

}
/******************************************************************************
* ��������  :  CheckNewClient
* ��������  :  ��������ip �Ƿ�����
* ����      :  
* ���      :
* ����ֵ    :
* ����      :
*******************************************************************************/
bool TMdbAgentServer::CheckNewClient(QuickMDB_SOCKET new_fd,  TMdbServerInfo* pServerInfo)
{
    if(m_vValidIP.begin() != m_vValidIP.end() || m_vInValidIP.begin() != m_vInValidIP.end())
    {
        TMdbNtcStringBuffer sIP = TMdbNtcSocket::GetRemoteAddrInfo(new_fd);//����������ip
        TMdbNtcSplit tSplitPeer,tSplit;
        tSplitPeer.SplitString(sIP.c_str(),'.');
        if(m_vValidIP.begin() != m_vValidIP.end())
        {
            if(false == IsIPExist(m_vValidIP, tSplit, tSplitPeer))
            {//������Чip �б���
                TADD_WARNING("IP[%s] not allowed to connect server", sIP.c_str());
                return false;
            }
            
        }
        if(m_vInValidIP.begin() != m_vInValidIP.end())
        {
            if(true == IsIPExist(m_vInValidIP, tSplit, tSplitPeer))
            {//��Чip
                TADD_WARNING("IP[%s] not allowed to connect server", sIP.c_str());
                return false;
            }
            
        }
        
    }
    
    AddWorkThread();//���ӹ����߳�
    return true;
}

/******************************************************************************
* ��������  :  IsIPExist
* ��������  :  ���ip�Ƿ��������б���
* ����      :  
* ���      :
* ����ֵ    :
* ����      :
*******************************************************************************/
bool TMdbAgentServer::IsIPExist(std::vector<string> &vIPListCfg, TMdbNtcSplit &tSplitCfg, TMdbNtcSplit &tSplitPeer)
{
    //ƥ��ip ��ʽ10.45.7.61 ��10.45.7.*
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
            {//ģ��ƥ�����
                continue;
            }
            else if(strcmp(tSplitCfg[i],tSplitPeer[i]) != 0) 
            {//����ͬ���˳�ƥ����һ��ip�б�
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
* ��������  :  RecvMsg
* ��������  :  �������ݰ�
* ����      :  TAgentClient * 
* ���      :
* ����ֵ    :
* ����      :
*******************************************************************************/
int TMdbAgentServer::RecvMsg(TAgentClient * ptClient)
{
    int iRet = 0;
    const char * pData = ptClient->m_pEventInfo->pMsgInfo->GetBuffer();
    CHECK_OBJ(pData);
    unsigned int iGetLen = ptClient->m_pEventInfo->pMsgInfo->GetLength();
    
    if(ptClient->m_iRecvPos == 0)
    {//��һ��������Ҫ����ͷ��
        if(iGetLen <= SIZE_MSG_AVP_HEAD) 
        {
            CHECK_RET(ERR_CSP_INVALID_RCV_PACKAGE,"RecvMsg len[%u]", iGetLen);
        }
        memcpy(ptClient->m_sRecvPackage, pData, SIZE_MSG_AVP_HEAD);
        ptClient->m_sRecvPackage[SIZE_MSG_AVP_HEAD] = '\0';
        CHECK_RET(ParseMsgHead(ptClient),"ParseMsgHead failed");
        //У�鳤��
        if(iGetLen == ptClient->m_tHead.iLen)
        {//һ�ξ�����
            memcpy(&ptClient->m_sRecvPackage[SIZE_MSG_AVP_HEAD], pData+SIZE_MSG_AVP_HEAD, ptClient->m_tHead.iLen - SIZE_MSG_AVP_HEAD);
        }
        else if(iGetLen < ptClient->m_tHead.iLen)
        {//����������Ҫ��������
            memcpy(&ptClient->m_sRecvPackage[SIZE_MSG_AVP_HEAD], pData+SIZE_MSG_AVP_HEAD, iGetLen - SIZE_MSG_AVP_HEAD);
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
    {//���ݲ���������Ҫ��������
        if(iGetLen + ptClient->m_iRecvPos < ptClient->m_tHead.iLen)
        {//����
            memcpy(&ptClient->m_sRecvPackage[ptClient->m_iRecvPos], pData, iGetLen);
            ptClient->m_iRecvPos += iGetLen;
            return ERR_CSP_KEEP_ON_RECV;
        }
        else if(iGetLen + ptClient->m_iRecvPos == ptClient->m_tHead.iLen)
        {//������
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
    
    //�������ݰ����
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
    //����ͷ��
    ptClient->m_tHead.Clear();
    ptClient->m_tHead.BinToCnvt((unsigned char*)(ptClient->m_sRecvPackage));
    if(ptClient->m_tHead.sHeadName[5] == '>' && ptClient->m_tHead.sHeadName[0]=='<' )
    {
        if(ptClient->m_iSessionID > 0 && ptClient->m_iSessionID != ptClient->m_tHead.iSessionId)
        {//session id��ƥ��
            CHECK_RET(ERR_CSP_PARSER_SESSIONID_NOEQUAL,"Peer session id[%d] != cur session id[%d]",
                    ptClient->m_tHead.iSessionId,ptClient->m_iSessionID);
        }
    }
    else
    {//ͷ��������
        CHECK_RET(ERR_CSP_PARSER_ERROR_CSP,"Parse head error.");
    }
    return iRet;
}

/******************************************************************************
* ��������  :  Init()
* ��������  :  ��ʼ����Attach�����ڴ棬ȡ��������Ϣ����λ��
* ����      :  pszDSN, ��������������DSN
* ���      :  ��
* ����ֵ    :  0 - �ɹ�!0 -ʧ��
* ����      :  li.shugang
*******************************************************************************/
int TMdbAgentServer::Init(const char* pszDSN)
{
    CHECK_OBJ(pszDSN);
    TADD_FUNC("Init(%s) : Start.", pszDSN);
    int iRet = 0;
    //�������ö���
    m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
    //�����Ϲ����ڴ�
    m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN);
    CHECK_OBJ(m_pShmDSN);
    m_pDsn = m_pShmDSN->GetInfo();
    CHECK_RET(m_tProcCtrl.Init(pszDSN),"m_tProcCtrl.Init(%s) failed.",pszDSN);
    CHECK_RET(m_tLinkCtrl.Attach(pszDSN),"m_tLinkCtrl.Init(%s) failed.",pszDSN);
    CHECK_RET(m_tLinkCtrl.ClearRemoteLink(),"m_tLinkCtrl.ClearRemoteLink(%s) failed.",pszDSN);
    m_tAgentFile.Init();
    if(m_pConfig) ParseCSIPCfg();//����ip
    tMutex.Create();
    tMutexConnect.Create();
    TADD_FUNC("TMdbAgentServer::Init(%s) : Finish.", pszDSN);
    return iRet;
}
/******************************************************************************
* ��������  :  PreSocket()
* ��������  :  ��ʼ��Socket�������м���
* ����      :
* ���      :  ��
* ����ֵ    :  �ɹ�����0�����򷵻ظ���������
* ����      :  li.shugang
*******************************************************************************/
int TMdbAgentServer::PreSocket(int iAgentPort)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TADD_DETAIL("iAgentPort=%d.", iAgentPort);
    if(false == AddListen(NULL, iAgentPort))
    {
        CHECK_RET(ERR_NET_LISTEN_PORT, "NTC AddListen (%s:%d) failed,ntc_error[%s].", m_pDsn->sLocalIP, iAgentPort,mdb_ntc_errstr.c_str());
    }
    TADD_NORMAL("PreSocket(%s:%d) : Waiting for connections and data......\n", m_pDsn->sLocalIP, iAgentPort);
    return iRet;
}
/******************************************************************************
* ��������  :  StartServer()
* ��������  :  ������������������ӽ��д���
* ����      :  pszName, ������
* ���      :  ��
* ����ֵ    :  ����������ӷ���0, ���򷵻ظ���������
* ����      :  li.shugang
*******************************************************************************/
int TMdbAgentServer::StartServer(const char* pszName)
{
    TADD_FUNC("Begin[%s].",pszName);
    
    int iRet = 0;
    int iCount = 0;
    m_iPumpMaxCount = m_pConfig->GetDSN()->m_iCSPumpMaxCount;
    m_iPeerCountPerPump = m_pConfig->GetDSN()->m_iCSPeerCountPerPump;
    
    SetMaxPeerNo(MAX_CS_THREAD_COUNTS);//���öԶ˸���
    InitEventPump(m_pConfig->GetDSN()->m_iCSPumpInitCount);//��ʼ���¼��ø���
    if(false == Start())//��������
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
        
        m_tProcCtrl.UpdateProcHeart(0);//Ӧ�ý�������ʱ��
        if(!m_bRecvFlag && iCount%1000 == 0)
        {//û�н����¼�������¼���״̬���200s
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
* ��������	:  AddWorkThread
* ��������	:  ���ӹ����߳�
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
int TMdbAgentServer::AddWorkThread()
{
    int iRet = 0;
    CHECK_RET(tMutexConnect.Lock(true),"Lock Faild");
    int iPumpCount = GetPumpCount();
    int iPeerCount = GetPeerCount();
    if(iPumpCount < m_iPumpMaxCount && iPeerCount >= m_iPeerCountPerPump*iPumpCount)
    {//�����¼���
        TMdbEventPump* pEventPump = CreateEventPump();
        if(!pEventPump)
        {
            TADD_WARNING("Add EventPump faild, pump:peer count =[%d:%d].",iPumpCount,iPeerCount);
        }
        else
        {
            AddEventPump(pEventPump);
            if(!pEventPump->Run())
            {//���������ͷ�
                TADD_WARNING("Run EventPump faild, pump:peer count =[%d:%d].",iPumpCount,iPeerCount);
            }
        }
    }
    
    CHECK_RET(tMutexConnect.UnLock(true),"UnLock Faild");
    return iRet;
    
}
/******************************************************************************
* ��������	:  ClientExit
* ��������	:  �Զ��˳���β����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
void TMdbAgentServer::ClientExit(TAgentClient *&ptClient)
{
    SAFE_DELETE(ptClient);
    return;
}

/******************************************************************************
* ��������	:  GetFreeAgentClient
* ��������	:  ��ȡ����agent client
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
        TADD_ERROR(ERROR_UNKNOWN,"Client Init() Failed");//�ͷŶ���......
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
* ��������	: Authentication 
* ��������	:  ��֤
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbAgentServer::Authentication(TAgentClient *ptClient)
{
    TADD_FUNC("Begin.");
    int iRet = 0;
    if(CSP_APP_LOGON != ptClient->m_tHead.iCmdCode)
    {//�ǵ�½��
        CHECK_RET_SEND_ANS(ERR_CSP_PARSER_LOGON,ptClient,"want to recv logon package,but recv[%d]",ptClient->m_tHead.iCmdCode);
    }
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_LOGON,true);
        CHECK_OBJ(pParser);
        CHECK_RET(pParser->DeSerialize((unsigned char*)(ptClient->m_sRecvPackage),ptClient->m_tHead.iLen),"parser logon package error");
       // pParser->m_tHead = ptClient->m_tHead;//head ���������
        IS_LOG(2){pParser->Print();}
        //��ȡ������Ϣ
        ptClient->m_tTMdbCSLink.m_sUser = pParser->GetStringValue(pParser->m_pRootAvpItem,AVP_USER_NAME);
        ptClient->m_tTMdbCSLink.m_sPass = pParser->GetStringValue(pParser->m_pRootAvpItem,AVP_USER_PWD);
        ptClient->m_tTMdbCSLink.m_sDSN = m_pDsn->sName;//����DSN����
        ptClient->m_tTMdbCSLink.m_iClientPID = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_PROCESS_ID);//��ȡ�ͻ��˵Ľ���ID
        ptClient->m_tTMdbCSLink.m_iClientTID = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_THREAD_ID);
		if(NULL != pParser->FindExistAvpItem(pParser->m_pRootAvpItem,AVP_LOW_PRIORITY))
        {//��sql-priority
            ptClient->m_tTMdbCSLink.m_iLowPriority = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_LOW_PRIORITY);
			if(ptClient->m_tTMdbCSLink.m_iLowPriority == 1)
			{
				TADD_NORMAL("Low priority link request.");
			}
        }
		//TADD_NORMAL("ptClient->m_tTMdbCSLink.m_iLowPriority = [%d]", ptClient->m_tTMdbCSLink.m_iLowPriority);
        //�������ݿ�
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
        {//��½ʧ��
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
    //����SessionID
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
    //�ظ���½�ɹ�
    TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_LOGON,false);
    CHECK_RET(SendAnswer(pParser,ptClient,0,"LOGIN_OK"),"SendAnswer[LOGIN_OK],failed");
    return iRet;
}

/******************************************************************************
* ��������	:  PrintSendMsg
* ��������	:  ��ӡ������Ϣ
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
* ��������	:  PrintMsg
* ��������	:  ��ӡ������Ϣ
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
* ��������	:  DumpSendPackage
* ��������	:  �������ݰ�
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
* ��������	:  DumpRecvPackage
* ��������	:  �������ݰ�
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
* ��������	:  DumpPackage
* ��������	: ���ݰ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
void TMdbAgentServer::DumpPackage(TAgentClient *ptClient, bool bSendFlag)
{
    if(NULL == ptClient) return;
    unsigned int iSize = sizeof(ptClient->m_sPackageBuffer);
    unsigned int iCount = strlen(ptClient->m_sPackageBuffer);
    char *pPackageBuffer = ptClient->m_sPackageBuffer;
    int iMsgLen = ptClient->m_tHead.iLen;
    
    if(iCount + iMsgLen*2 + iMsgLen/2 + BUFFER_PLUS_COUNT >= iSize-1)
    {//����buffer�ܴ�С,BUFFER_PLUS_COUNTĿǰ�㹻��
        m_tAgentFile.Write(pPackageBuffer, iCount);
        memset(pPackageBuffer, 0, iSize);
        iCount = 0;
    }
    
    //����ʼ{ [%s]SessionID=[%d]Sequence=[%d]
    char strCurrentDateTime[15] = {0};
    TMdbDateTime::GetCurrentTimeStr(strCurrentDateTime);
    snprintf(&pPackageBuffer[iCount], iSize-iCount,SESSION_ID_STR, 
        strCurrentDateTime, ptClient->m_tHead.iSessionId, ptClient->m_tHead.isequence);
        
    //������Ϣ
    if(bSendFlag)
    {
        DumpSendPackage(ptClient);
    }
    else
    {
        DumpRecvPackage(ptClient);
    }
    //������
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
* ��������	:  AppSendSQL
* ��������	:  SQLע��
* ����		:
* ���		:
* ����ֵ	:
* ����		:
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
        pParser->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//����
        //pParser->m_tHead = ptClient->m_tHead;//head ���������
        IS_LOG(2){pParser->Print();}
        iSqlLabel = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        sSQL	  = pParser->GetStringValue(pParser->m_pRootAvpItem,AVP_SQL_STATEMENT);
        if(NULL != pParser->FindExistAvpItem(pParser->m_pRootAvpItem,AVP_SQL_FLAG))
        {//��sql-flag
            iSqlFlag = pParser->GetINT32Value(pParser->m_pRootAvpItem,AVP_SQL_FLAG);
        }
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//����SQLLABEL����ȡclient
        if(NULL == pClientQuery)
        {
           CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        pQuery->SetSQL(sSQL,iSqlFlag,0);//����SQL
        TADD_NORMAL("QuerySqlLabel=[%d],SetSQL(%s),IsDynamic=[%s],SqlFlag=[%d]",iSqlLabel,sSQL,(pQuery->ParamCount() != 0)?"TRUE":"FALSE",iSqlFlag);
        TMdbCspParser * pSendSQLAns = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,false);
        CHECK_OBJ(pSendSQLAns);
        pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)iSqlLabel);//������sqllable
        if(pQuery->ParamCount() != 0)
        {
            //��̬SQL
            pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SELECT_HAVE_NEXT,(int)0);
            pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_AFFECTED_ROW,0);
        }
        else
        {
            //��̬SQL
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
* ��������	:  AppSendParam
* ��������	:  ���������Ĳ���
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
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
       // pParser->m_tHead = ptClient->m_tHead;//head ���������
        IS_LOG(2){pParser->Print();}
        iSqlLabel         = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//����SQLLABEL����ȡclient
        if(NULL == pClientQuery)
        {
            CHECK_RET_SEND_ANS(ERR_CSP_PARSER_ERROR_CSP,ptClient,"not get query label=[%d]",iSqlLabel);
        }
        TMdbQuery * pQuery = pClientQuery->m_pQuery;
        CHECK_OBJ(pQuery);
        std::vector<TMdbAvpItem *> vParamGroup;
        pParser->GetExistGroupItem(pParser->m_pRootAvpItem,AVP_PARAM_GROUP,vParamGroup);
        std::vector<TMdbAvpItem *>::iterator itor = vParamGroup.begin();//���ò���
        for(; itor != vParamGroup.end(); ++itor)
        {
            if( pParser->IsNullValue((*itor)->pChildItem,AVP_PARAM_VALUE))
            {//nullֵ
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
        pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)iSqlLabel);//������sqllable
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
* ��������	:  AppSendParamArray
* ��������	:  ���������Ĳ�������
* ����		:
* ����		:
* ���		:
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
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
        //pParser->m_tHead = ptClient->m_tHead;//head ���������
         IS_LOG(2){pParser->Print();}
        iSqlLabel         = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//����SQLLABEL����ȡclient
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
        //��ȡ�׸�BATCH_GROUP
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
                {//nullֵ
                    pQuery->SetParameterNULL(iParamIndex++);
                }
                else
                {//ParamGroup �Դ�value
                    pQuery->SetParameter(iParamIndex++, pParamGroupItem->pszValue);
                }
            }
            pQuery->Execute();
            iRowEffect += pQuery->RowsAffected();//ִ��
        }
        TMdbCspParser * pSendSQLAns = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,false);
        CHECK_OBJ(pSendSQLAns);
        pSendSQLAns->SetItemValue(pSendSQLAns->m_pRootAvpItem,AVP_SQL_LABEL,(unsigned int)iSqlLabel);//������sqllable
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

//����ѯ��next ����ֵ
int TMdbAgentServer::FillNextResult(TAgentClient *ptClient,TMdbQuery * pQuery,TMdbCspParser * pSendSQLAns)throw(TMdbException,TMdbCSPException)
{
    int iRet = 0;
    //TMdbCspParser * pSendSQLAns = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_SEND_SQL,false);
    CHECK_OBJ(pSendSQLAns);
    int iCount = 0;
    TMdbAvpItem * pRowGroup 	= NULL;
    TMdbAvpItem * pColumnGroup	= NULL;
    bool bHaveNext = true;
    //for(i = 0; i<PARTIAL_NEXT_SEND_NUM; i++) //ÿ��next������� 
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
                {//nullֵ
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
            {//�ﵽ���ֵ
                TADD_FLOW("Up to MAX next[%d]",iCount);
                break;
            }
        }
        else
        {
            //next ����
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

//��Ӧnext����
int TMdbAgentServer::AppNextSQLResult(TAgentClient *ptClient)
{
    int iRet = 0;
    int iSqlLabel = 0;
    try
    {
        TMdbCspParser * pParser = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_NEXT,true);
        CHECK_OBJ(pParser);
        pParser->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);
        //pParser->m_tHead = ptClient->m_tHead;//head ���������
        IS_LOG(2){pParser->Print();}
        iSqlLabel  = pParser->GetUINT32Value(pParser->m_pRootAvpItem,AVP_SQL_LABEL);
        
        ClientQuery * pClientQuery = ptClient->GetClientQuery(iSqlLabel);//����SQLLABEL����ȡclient
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
            //���ǲ�ѯ
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
* ��������	:  AppTransAction
* ��������	:  ������
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
int TMdbAgentServer::AppTransAction(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        TMdbCspParser * pParserSend = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_ACTION,true);
        CHECK_OBJ(pParserSend);
        pParserSend->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//����
        //pParserSend->m_tHead = ptClient->m_tHead;//head ���������
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
            ERROR_TO_THROW_NOSQL(-1,"sCmd[%s] is not rollback/commit",sCmd);
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
* ��������	:  AppGetSequence
* ��������	:  ��ȡ����
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
int TMdbAgentServer::AppGetSequence(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        TMdbCspParser * pParserSend = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_GET_SEQUENCE,true);
        CHECK_OBJ(pParserSend);
        pParserSend->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//����
        //pParserSend->m_tHead = ptClient->m_tHead;//head ���������
        IS_LOG(2){pParserSend->Print();}
        char * sSeqName = NULL;
        sSeqName = pParserSend->GetStringValue(pParserSend->m_pRootAvpItem,AVP_SEQUENCE_NAME);
        TMdbSequenceMgr * pSeqMgr = ptClient->GetSeqMgrByName(sSeqName,m_pConfig);
        if(NULL == pSeqMgr)
        {
            ERROR_TO_THROW_NOSQL(-1,"not find sequence mgr by [%s].",sSeqName);
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

//���ͽ����Ϣ
int TMdbAgentServer::SendPackageBody(TAgentClient *ptClient,int iSendLen)
{
    int lRet = 0;
    bool bRet = true;
    IS_LOG(1)
    {
        lRet = PrintSendMsg(ptClient);
        if(lRet < 0)
        {
            //û������CmdID
            return lRet;
        }
    }
    //��¼ÿ�εķ���
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
            //û������CmdID
            return lRet;
        }
        TMdbDateTime::MSleep(3000);
        
        bRet = pPeerInfo->PostMessage(ptClient->m_sSendPackage,iSendLen);
        if(false == bRet)
        {
            return ERR_NET_SEND_FAILED;        
        }
        
    }
    //�������ݰ����
    if(m_tProcCtrl.GetProcState() == PDUMP)
    {
        ptClient->m_cProcState = PDUMP;
        DumpPackage(ptClient, true);
    }
    return 0;
}

/******************************************************************************
* ��������	:  AppErrorAnswer
* ��������	:  ������ϢӦ��
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
int TMdbAgentServer::AppErrorAnswer(TAgentClient *ptClient,int iAnsCode,const char *fmt,...)
{
    char sTemp[10240] = {0};
    va_list ap;
    va_start(ap,fmt);
    memset(sTemp,0,sizeof(sTemp));
    vsnprintf(sTemp, sizeof(sTemp), fmt, ap);
    va_end (ap);
    TMdbCspParser * pParserError = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_ERROR,false);
    return SendAnswer(pParserError, ptClient, iAnsCode,sTemp);
}
/******************************************************************************
* ��������	:  DoOperation
* ��������	:  ֻ������ȷ�ı���
* ����		:
* ���		:
* ����ֵ	:
* ����		:
*******************************************************************************/
int TMdbAgentServer::DoOperation(TAgentClient *ptClient)
{
    int iRet = 0;
    //״̬���ʱ���֮ǰ�����ݰ�
    char cProcState = m_tProcCtrl.GetProcState();
    if(PDUMP !=  cProcState
        && PDUMP == ptClient->m_cProcState) 
    {
        ptClient->m_cProcState = cProcState;
        m_tAgentFile.Write(ptClient->m_sPackageBuffer, strlen(ptClient->m_sPackageBuffer));
        memset(ptClient->m_sPackageBuffer, 0, sizeof(ptClient->m_sPackageBuffer));
    }
    
    iRet = RecvMsg(ptClient);
    if(iRet == ERR_CSP_KEEP_ON_RECV ) {return 0;}//��������
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
    
    clock_t tStart = clock();
    switch(ptClient->m_tHead.iCmdCode)
    {
        case CSP_APP_LOGON:
        {
            CHECK_RET_BREAK(Authentication(ptClient),"Authentication failed.");//��֤
            TADD_NORMAL("ThreadID=[%lu],ClientIP=[%s],SequenceID=[%d],SocketID=[%d],Pno=[%d] Login Success!! ",
                    ptClient->m_iThreadId,ptClient->m_tTMdbCSLink.m_sClientIP,ptClient->m_tHead.GetSequence(),
                    ptClient->m_tTMdbCSLink.iFD,ptClient->m_iPno);
            
            break;
        }
        case CSP_APP_KICK_LINK:  //ok
        {

            break;
        }
        case CSP_APP_SEND_SQL:
        {
            //����SQLע��
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
        case CSP_APP_QMDB_INFO://��ȡqmdb��Ϣ
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
    clock_t tFinish = clock();
    double duration = (double)(tFinish - tStart)/CLOCKS_PER_SEC;
    if(duration >= 3.0)
    {
        TADD_WARNING("CmdCode=[%u],ThreadID=[%lu], ClientIP=[%s],SequenceID=[%d] DoOperation Time[%f] > 3s ",
                ptClient->m_tHead.iCmdCode,ptClient->m_iThreadId,ptClient->m_tTMdbCSLink.m_sClientIP,ptClient->m_tHead.GetSequence(),duration);
    }
    
    TADD_FUNC("end iRet=%d",iRet);
    return iRet;
}

/******************************************************************************
* ��������	:  SendAnswer
* ��������	:  ������Ӧ��
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
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
* ��������	:  CheckClientSequence
* ��������	:  ���ͻ������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
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
    {//��Ҫ�ط�
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
* ��������	:  AppGetQmdbInfo
* ��������	:  ��ȡqmdb ��Ϣ��
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbAgentServer::AppGetQmdbInfo(TAgentClient *ptClient)
{
    int iRet = 0;
    try
    {
        TMdbCspParser * pParserQmdbInfo = ptClient->m_tCspParserMgr.GetParserByType(CSP_APP_QMDB_INFO,true);
        CHECK_OBJ(pParserQmdbInfo);
        pParserQmdbInfo->DeSerialize(ptClient->m_sRecvPackage,ptClient->m_tHead.iLen);//����
       // pParserQmdbInfo->m_tHead = ptClient->m_tHead;//head ���������
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

//}
