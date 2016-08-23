/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbQueue.cpp
*@Description�� ��oralceͬ����������ͬ�������з�װ���ṩͳһ�����ӿ�
*@Author:		cao.peng
*@Date��	    2012.11
*@History:
******************************************************************************************/
#include "Helper/mdbQueue.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbStruct.h"

#include "Helper/mdbOS.h"


//namespace QuickMDB{


#define SAFE_DELETE_T(_obj) if(NULL != _obj){delete [] _obj;_obj=NULL;}

TMdbQueue::TMdbQueue()
{
    m_pDsn = NULL;
    m_pQueueShm = NULL;
    m_iSQLType = -1;
    m_iSyncType =0;
    m_iErrTry = 0;
    m_pszRecord = NULL;
    m_pCurAddr = NULL;
    m_bCheckData = true;
    m_pFile = NULL;
    m_pszErrorRecord = NULL;
    memset(m_sFileName,0,sizeof(m_sFileName));
}

TMdbQueue::~TMdbQueue()
{
    SAFE_CLOSE(m_pFile);
    SAFE_DELETE_ARRAY(m_pszRecord);
    SAFE_DELETE_ARRAY(m_pszErrorRecord);
}

int TMdbQueue::Init(TMdbMemQueue * pMemQueue,TMdbDSN * pDsn,const bool bWriteErrorData)
{
    int iRet = 0;
    if(m_pszRecord == NULL)
    {
        m_pszRecord = new(std::nothrow) char[MAX_VALUE_LEN];
        CHECK_OBJ(m_pszRecord);
        memset(m_pszRecord,0,MAX_VALUE_LEN);
    }
    if(m_pszErrorRecord == NULL)
    {
        m_pszErrorRecord = new(std::nothrow) char[MAX_VALUE_LEN];
        CHECK_OBJ(m_pszErrorRecord);
        memset(m_pszErrorRecord,0,MAX_VALUE_LEN);
    }
    m_pQueueShm = pMemQueue;
    m_pDsn = pDsn;
    CHECK_OBJ(m_pQueueShm);
    CHECK_OBJ(m_pDsn);
    //����REP��ORACLEͬ������Ҫ��¼�쳣�ļ�
    m_bWriteErrorData = bWriteErrorData;
    char sErrorLogPath[MAX_PATH_NAME_LEN] = {0};
    memset(sErrorLogPath, 0, sizeof(sErrorLogPath));
    char sDsnName[MAX_NAME_LEN] = {0};
    SAFESTRCPY(sDsnName, sizeof(sDsnName), m_pDsn->sName);
    TMdbNtcStrFunc::ToLower(sDsnName);
#ifdef WIN32
    sprintf(sErrorLogPath, "%s\\log\\%s\\", getenv("QuickMDB_HOME"), sDsnName);
#else
    sprintf(sErrorLogPath, "%s/log/%s/", getenv("QuickMDB_HOME"), sDsnName);
#endif
    if(!TMdbNtcDirOper::IsExist(sErrorLogPath))
    {
        TMdbNtcDirOper::MakeFullDir(sErrorLogPath);
    }
    snprintf(m_sFileName,sizeof(m_sFileName),"%sRep_Error_Info_%d.log",\
        sErrorLogPath,TMdbOS::GetPID());
    TADD_DETAIL("ERROR_LOG=[%s].",m_sFileName);
    return iRet;
}
/******************************************************************************
* ��������  :  Push
* ��������  :  ����DSN����
* ����      :  sData push�����ݣ�iLen push���ݵĳ���
* ���      :  ��
* ����ֵ    :  !0 - ʧ��,0 - �ɹ�
* ����      :  cao.peng
*******************************************************************************/
bool TMdbQueue::Push(char * const sData, const int iLen)
{
    TADD_FUNC("Start.");
    bool ShmIsEnable = false;
    int iPushErrorCount = 0;
    while(true)
    {
        m_pQueueShm->tPushMutex.Lock(true, &m_pDsn->tCurTime);
        m_iPushPos = m_pQueueShm->iPushPos;
        m_iPopPos  = m_pQueueShm->iPopPos;
        m_iTailPos = m_pQueueShm->iTailPos;
        m_pCurAddr = (char*)m_pQueueShm + m_iPushPos;
        
        if(m_iPushPos >= m_iPopPos)
        {
            if(m_iPushPos + iLen > m_pQueueShm->iEndPos)
            {
                if(m_iPopPos !=  m_pQueueShm->iStartPos)
                {
                    m_pQueueShm->iTailPos = m_pQueueShm->iPushPos;
                    m_pQueueShm->iPushPos  = m_pQueueShm->iStartPos;
                    m_pQueueShm->tPushMutex.UnLock(true); 
                    continue;//������Ե�ͷ����ֱ�ӵ�ͷ�����ܵ�ͷ˵����������
                }
                ShmIsEnable = false;
            }
            else
            {
                ShmIsEnable = true;
            }
        }
        else
        {
            if(m_iPushPos + iLen < m_iPopPos)
            {
                ShmIsEnable = true;
            }
            else
            {
                ShmIsEnable = false;
            }
        }

        if(ShmIsEnable == true)
        {
            memcpy(m_pCurAddr, sData, iLen);
            m_pQueueShm->iPushPos += iLen;
             m_pQueueShm->tPushMutex.UnLock(true); 
            break;
        }
        else
        {
            TADD_WARNING("wait for Pop.");
            if(iPushErrorCount >= 100)
            {
                m_pQueueShm->tPushMutex.UnLock(true);    
                break;
            }
            iPushErrorCount++;
            if(iPushErrorCount == 99)
            {
                TMdbDateTime::MSleep(10);
            }
            m_pQueueShm->tPushMutex.UnLock(true); 
        }
    }
    TADD_FUNC("Finish.");
    return ShmIsEnable;
}

/******************************************************************************
* ��������  :  Pop
* ��������  :  ��ȡ������һ������
* ����      :  ��
* ���      :  sData �洢pop�������ݣ�iLen pop�����ݵĳ���
* ����ֵ    :  !0 - ʧ��,0 - �ɹ�
* ����      :  cao.peng
*******************************************************************************/    
int TMdbQueue::Pop()
{   
    TADD_FUNC("Start.");
    int iRet = T_SUCCESS;
    int iErrorCount = 0;
    m_iRecordLen = 0;
    m_iPushPos = m_pQueueShm->iPushPos;
    m_iPopPos  = m_pQueueShm->iPopPos;
    m_iTailPos = m_pQueueShm->iTailPos;
    m_pCurAddr = (char*)m_pQueueShm + m_iPopPos;

    if(m_iPushPos == m_iPopPos)
    {
        //����Ϊ��
        m_pszRecord[0] = 0;
        return T_EMPTY;
    }

    if(m_iPopPos == m_iTailPos && m_iPopPos != m_iPushPos)
    {
        m_pQueueShm->iPopPos = m_pQueueShm->iStartPos;
        return Pop();
    }
    
    for(iErrorCount = 0; iErrorCount<ERROR_TIMES;iErrorCount++)
    {
        iRet = CheckRepDataIsValid();
        if(iRet == T_SUCCESS)
        {
            memcpy(m_pszRecord,m_pCurAddr,m_iRecordLen);
            m_pszRecord[m_iRecordLen] = 0;
            memset(m_pCurAddr,0,m_iRecordLen);
            break;
        }
        if(ERROR_TIMES-1 == iErrorCount)
        {
            TMdbDateTime::MSleep(5);  
        }  
    }

    if( iErrorCount == ERROR_TIMES)
    {
        //�ظ�У��10����Ȼ������ȷ��������¼�϶��Ǵ��
        switch(iRet)
        {
            case T_BIG_PushPos : TADD_ERROR(ERROR_UNKNOWN,"pop[%d] + len[%d] > push[%d].",m_iPopPos,m_iRecordLen,m_iPushPos);break;
            case T_BIG_TailPos : TADD_ERROR(ERROR_UNKNOWN,"pop[%d] + len[%d] > tail[%d].",m_iPopPos,m_iRecordLen,m_iTailPos);break;
            case T_BIG_ENDERROR : TADD_ERROR(ERROR_UNKNOWN,"Check End[##] ERROR.");break;
            case T_BIG_BEGINERROR: TADD_ERROR(ERROR_UNKNOWN,"Check Begin[^^] ERROR.");break;
            case T_LENGTH_ERROR: TADD_ERROR(ERROR_UNKNOWN,"Length[%d] ERROR.",m_iRecordLen);break;
            case T_SQLTYPE_ERROR: TADD_ERROR(ERROR_UNKNOWN,"SqlType[%d] ERROR.",m_iSQLType);break;
            case T_SOURCEID_ERROR: TADD_ERROR(ERROR_UNKNOWN,"SyncType[%d] ERROR.",m_iSyncType);break;
            default : break;
        }
        m_iRecordLen = 0;
        m_iRecordLen = GetPosOfNext();
        WriteInvalidData(m_iRecordLen);
        memset(m_pCurAddr,0,m_iRecordLen);
    }
    m_pQueueShm->iPopPos+=m_iRecordLen;
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbQueue::PopRepData()
{   
    TADD_FUNC("Start.");
    int iRet = T_SUCCESS;
    int iErrorCount = 0;
    m_iRecordLen = 0;
    m_iPushPos = m_pQueueShm->iPushPos;
    m_iPopPos  = m_pQueueShm->iPopPos;
    m_iTailPos = m_pQueueShm->iTailPos;
    m_pCurAddr = (char*)m_pQueueShm + m_iPopPos;

    if(m_iPushPos == m_iPopPos)
    {
        //����Ϊ��
        return T_EMPTY;
    }

    if(m_iPopPos == m_iTailPos && m_iPopPos != m_iPushPos)
    {
        m_pQueueShm->iPopPos = m_pQueueShm->iStartPos;
        return PopRepData();
    }
    
    for(iErrorCount = 0; iErrorCount<ERROR_TIMES;iErrorCount++)
    {
        iRet = CheckRepDataIsValid();
        if(iRet == T_SUCCESS)
        {
            memcpy(m_pszRecord,m_pCurAddr,m_iRecordLen);

            m_pszRecord[m_iRecordLen] = 0;
            memset(m_pCurAddr,0,m_iRecordLen);
            break;
        }
        if(ERROR_TIMES-1 == iErrorCount)
        {
            TMdbDateTime::MSleep(5);  
        }  
    }

    if( iErrorCount == ERROR_TIMES)
    {
        //�ظ�У��10����Ȼ������ȷ��������¼�϶��Ǵ��
        switch(iRet)
        {
            case T_BIG_PushPos : TADD_ERROR(ERROR_UNKNOWN,"pop[%d] + len[%d] > push[%d].",m_iPopPos,m_iRecordLen,m_iPushPos);break;
            case T_BIG_TailPos : TADD_ERROR(ERROR_UNKNOWN,"pop[%d] + len[%d] > tail[%d].",m_iPopPos,m_iRecordLen,m_iTailPos);break;
            case T_BIG_ENDERROR : TADD_ERROR(ERROR_UNKNOWN,"Check End[##] ERROR.");break;
            case T_BIG_BEGINERROR: TADD_ERROR(ERROR_UNKNOWN,"Check Begin[^^] ERROR.");break;
            case T_LENGTH_ERROR: TADD_ERROR(ERROR_UNKNOWN,"Length[%d] ERROR.",m_iRecordLen);break;
            case T_SQLTYPE_ERROR: TADD_ERROR(ERROR_UNKNOWN,"SqlType[%d] ERROR.",m_iSQLType);break;
            case T_SOURCEID_ERROR: TADD_ERROR(ERROR_UNKNOWN,"SyncType[%d] ERROR.",m_iSyncType);break;
            default : break;
        }
        m_iRecordLen = 0;
        m_iRecordLen = GetPosOfNext();
        WriteInvalidData(m_iRecordLen);
        memset(m_pCurAddr,0,m_iRecordLen);
    }
    m_pQueueShm->iPopPos+=m_iRecordLen;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������  :  GetSourceId
* ��������  :  ��ȡpop�����ݵ�������Դ
* ����      :  ��
* ���      :  ��
* ����ֵ    :  ����Դ
* ����      :  cao.peng
*******************************************************************************/
//int TMdbQueue::GetSourceId()
//{
//    return  m_iSourceID;
//}

/******************************************************************************
* ��������  :  GetSqlType
* ��������  :  ��ȡsql����
* ����      :  ��
* ���      :  ��
* ����ֵ    :  sql����
* ����      :  cao.peng
*******************************************************************************/
int TMdbQueue::GetSqlType()
{
    return m_iSQLType;
}

int TMdbQueue::GetRecordLen()
{
    return m_iRecordLen;
}

char* TMdbQueue::GetData()
{
    return m_pszRecord;
}
void TMdbQueue::SetParameter()
{
        m_iSQLType = (m_pCurAddr[44]-'0')*10 + (m_pCurAddr[45]-'0');
        m_iSyncType = (m_pCurAddr[7]-'0')*1000 + (m_pCurAddr[8]-'0')*100 + (m_pCurAddr[9]-'0')*10 + (m_pCurAddr[10]-'0');
}
/******************************************************************************
* ��������  :  CheckDataIsValid
* ��������  :  �ж϶����е�ĳ�������Ƿ���Ч
* ����      :  pCurAddr ��⹲���ڴ��ַ
* ���      :  iLen ���ݳ���
* ����ֵ    :  true ������Ч��false ������Ч
* ����      :  cao.peng
*******************************************************************************/
int TMdbQueue::CheckDataIsValid()
{
    TADD_FUNC("Start.");
    if(m_pCurAddr[0] == '^' && m_pCurAddr[1] == '^')
    {
        m_iRecordLen= (m_pCurAddr[2]-'0')*1000 + (m_pCurAddr[3]-'0')*100 + (m_pCurAddr[4]-'0')*10 + (m_pCurAddr[5]-'0');
        if(m_iRecordLen < 10 || m_iRecordLen > MAX_VALUE_LEN)
        {
            return T_LENGTH_ERROR;
        }
        if(m_bCheckData)
        {//У������
            SetParameter();
            //if(m_iSourceID != SOURCE_REP && m_iSourceID != SOURCE_PEER && m_iSourceID != SOURCE_PEER_REP)
            //{
            //    return T_SOURCEID_ERROR;
            //}
            if(m_iSQLType != T_UPDATE && m_iSQLType != T_DELETE && m_iSQLType != T_INSERT)
            {
                return T_SQLTYPE_ERROR;
            }
        }
        //���ݲ��ᳬ�������е�����
        if(m_iPushPos > m_iPopPos)
        {
            if(m_iPopPos + m_iRecordLen > m_iPushPos)
            {
                return T_BIG_PushPos;
            }
        }
        else
        {
            if(m_iPopPos + m_iRecordLen > m_iTailPos)
            {
                return T_BIG_TailPos;
            }   
        } 
        
        //���ݳ��ȱ��븴��Լ����������Ҫ����У��
        if(m_pCurAddr[m_iRecordLen-2] != '#' || m_pCurAddr[m_iRecordLen-1] != '#')
        {
            return T_BIG_ENDERROR;
        }

    }
    else
    {
        m_iRecordLen = 0;
        return T_BIG_BEGINERROR;
    }
    TADD_FUNC("Finish.");
    return T_SUCCESS;
}

int TMdbQueue::CheckRepDataIsValid()
{
    TADD_FUNC("Start.");
    if(m_pCurAddr[0] == '^' && m_pCurAddr[1] == '^')
    {
        m_iRecordLen= (m_pCurAddr[2]-'0')*1000 + (m_pCurAddr[3]-'0')*100 + (m_pCurAddr[4]-'0')*10 + (m_pCurAddr[5]-'0');
        if(m_iRecordLen < 10 || m_iRecordLen > MAX_VALUE_LEN)
        {
            return T_LENGTH_ERROR;
        }
        if(m_bCheckData)
        {//У������
            m_iSQLType = (m_pCurAddr[49]-'0')*10 + (m_pCurAddr[50]-'0');
            if(m_iSQLType != T_UPDATE && m_iSQLType != T_DELETE && m_iSQLType != T_INSERT)
            {
                return T_SQLTYPE_ERROR;
            }
        }
        //���ݲ��ᳬ�������е�����
        if(m_iPushPos > m_iPopPos)
        {
            if(m_iPopPos + m_iRecordLen > m_iPushPos)
            {
                return T_BIG_PushPos;
            }
        }
        else
        {
            if(m_iPopPos + m_iRecordLen > m_iTailPos)
            {
                return T_BIG_TailPos;
            }   
        } 
        
        //���ݳ��ȱ��븴��Լ����������Ҫ����У��
        if(m_pCurAddr[m_iRecordLen-2] != '#' || m_pCurAddr[m_iRecordLen-1] != '#')
        {
            return T_BIG_ENDERROR;
        }

    }
    else
    {
        m_iRecordLen = 0;
        return T_BIG_BEGINERROR;
    }
    TADD_FUNC("Finish.");
    return T_SUCCESS;
}

/******************************************************************************
* ��������  :  CheckRecord
* ��������  :  �ж�pop���������Ƿ�Ϸ�����Ҫ������ݵĳ����Լ��Ƿ���##����
* ����      :  pszRecord ����¼,iLen ��¼����
* ���      :  ��
* ����ֵ    :  true ��¼�Ϸ���false ��¼��Ч
* ����      :  cao.peng
*******************************************************************************/
bool TMdbQueue::CheckRecord(char *pszRecord,const int iLen)
{
    TADD_FUNC("Start.");
    int iRealLen = strlen(pszRecord);
    if(iRealLen != iLen)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Record length error,RealLen=%d,RecordLen=%d.",iRealLen,iLen);
        return false;    
    }
    
    if(pszRecord[iRealLen-1] != '#' || pszRecord[iRealLen-2] != '#')
    {
        TADD_ERROR(ERROR_UNKNOWN,"The recording head is not ## at the beginning.");
        return false;    
    }
    
    TADD_FUNC("Finish.");
    return true;
}

/******************************************************************************
* ��������  :  WriteInvalidData
* ��������  :  ������Ч�ļ�¼������Ҫд�ļ�
* ����      :  iLen��Ч���ݳ���
* ���      :  ��
* ����ֵ    :  
* ����      :  cao.peng
*******************************************************************************/
bool TMdbQueue::WriteInvalidData(const int iInvalidLen)
{
    TADD_ERROR(ERROR_UNKNOWN,"Rep Data Error:PushPos=%d,PopPos=%d,TailPos=%d.m_bWriteErrorData=[%s]",m_iPushPos,m_iPopPos,m_iTailPos,true?"true":"false");
    int iLen = 0;
    char sCurtime[MAX_TIME_LEN] = {0};
    char sFileNameOld[MAX_FILE_NAME] = {0};
    if(!m_bWriteErrorData){return true;}
    TADD_ERROR(ERROR_UNKNOWN,"1111");
    if(NULL == m_pFile)
    {
        m_pFile =  fopen(m_sFileName, "a+");
        CHECK_OBJ(m_pFile);
        setbuf(m_pFile,NULL);
    }
    TADD_ERROR(ERROR_UNKNOWN,"2222");
    //�ж���Ч�ļ�¼�����Ƿ�ᳬ��32K
    TMdbDateTime::GetCurrentTimeStr(sCurtime);
    memset(m_pszErrorRecord,0,sizeof(m_pszErrorRecord));
    snprintf(m_pszErrorRecord,MAX_VALUE_LEN,"\n%s [Invalid Data] : ",sCurtime);
    iLen = strlen(m_pszErrorRecord);
    if(iInvalidLen >= MAX_VALUE_LEN-iLen)
    {
        memcpy(m_pszErrorRecord+iLen,m_pCurAddr,MAX_VALUE_LEN-iLen);
        iLen = MAX_VALUE_LEN;
        m_pszErrorRecord[iLen-1] = '\0';
    }
    else
    {
        memcpy(m_pszErrorRecord+iLen,m_pCurAddr,iInvalidLen);
        iLen += iInvalidLen;
        m_pszErrorRecord[iLen] = '\0';
    }
    TADD_ERROR(ERROR_UNKNOWN,"3333");
    //���������־�ļ�����
    unsigned long long  iFilesize = 0;
    if (TMdbNtcFileOper::GetFileSize(m_sFileName, iFilesize))
    {
        TADD_ERROR(ERR_OS_GET_FILE_SIZE, "Get file[%s] size failed.", m_sFileName);
    }
    if (iFilesize >= 128*1024*1024)
    {
        //���������,�Ե�ǰʱ��YYYYMMDDHHM24SS��ʾ
        SAFE_CLOSE(m_pFile);
        TMdbNtcFileOper::Remove(sFileNameOld);
        snprintf(sFileNameOld,sizeof(sFileNameOld),"%s.old",m_sFileName);
        //����־�ļ�������
        TMdbNtcFileOper::Rename(m_sFileName,sFileNameOld);
        TMdbNtcFileOper::Remove(m_sFileName);
        m_pFile = fopen (m_sFileName,"a+");
        if(m_pFile == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Open file [%s] fail.\n",m_sFileName);
            return false;
        }
    }
    if(fwrite(m_pszErrorRecord,iLen,1,m_pFile) == 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"fwrite() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));
        return false;
    }
    TADD_ERROR(ERROR_UNKNOWN,"4444[%s],name[%s]",m_pszErrorRecord,m_sFileName);
    return true;
}

/******************************************************************************
* ��������  :  GetPosOfNext
* ��������  :  ��ȡ��һ����¼��λ��
* ����      :  ��
* ���      :  ��
* ����ֵ    :  ��һ����¼���λ��
* ����      :  cao.peng
*******************************************************************************/
int TMdbQueue::GetPosOfNext()
{
    int iLength = 0;
    if(m_iPopPos < m_iPushPos)
    {
        iLength = m_iPushPos - m_iPopPos;
    }
    else
    {
        iLength = m_iTailPos - m_iPopPos;
    }

    int i = 1;
    for(i = 1; i<iLength;i++)
    {
        if(m_pCurAddr[i] == '^' && m_pCurAddr[i+1] == '^')
        {
            break;
        }
    }
    return i;
    
}



TMdbOnlineRepQueue::TMdbOnlineRepQueue()
{
    m_pDsn = NULL;
    m_pOnlineRepQueueShm = NULL;
    m_iSQLType = -1;
    m_iSyncType =0;
    m_iErrTry = 0;
    m_pszRecord = NULL;
    m_pCurAddr = NULL;
    m_bCheckData = true;
    m_pFile = NULL;
    m_pszErrorRecord = NULL;
    memset(m_sFileName,0,sizeof(m_sFileName));
}

TMdbOnlineRepQueue::~TMdbOnlineRepQueue()
{
    SAFE_CLOSE(m_pFile);
    SAFE_DELETE_ARRAY(m_pszRecord);
    SAFE_DELETE_ARRAY(m_pszErrorRecord);
}

int TMdbOnlineRepQueue::Init(TMdbOnlineRepMemQueue * pOnlineRepMemQueue,TMdbDSN * pDsn, const bool bWriteErrorData)
{
    int iRet = 0;
    if(m_pszRecord == NULL)
    {
        m_pszRecord = new char[MAX_VALUE_LEN];
        CHECK_OBJ(m_pszRecord);
        memset(m_pszRecord,0,MAX_VALUE_LEN);
    }
    if(m_pszErrorRecord == NULL)
    {
        m_pszErrorRecord = new char[MAX_VALUE_LEN];
        CHECK_OBJ(m_pszErrorRecord);
        memset(m_pszErrorRecord,0,MAX_VALUE_LEN);
    }
	m_bWriteErrorData = bWriteErrorData;
    m_pOnlineRepQueueShm = pOnlineRepMemQueue;
    m_pDsn = pDsn;
    CHECK_OBJ(m_pOnlineRepQueueShm);
    CHECK_OBJ(m_pDsn);
    //����REP��ORACLEͬ������Ҫ��¼�쳣�ļ�
    char sErrorLogPath[MAX_PATH_NAME_LEN] = {0};
    memset(sErrorLogPath, 0, sizeof(sErrorLogPath));
    char sDsnName[MAX_NAME_LEN] = {0};
    SAFESTRCPY(sDsnName, sizeof(sDsnName), m_pDsn->sName);
    TMdbNtcStrFunc::ToLower(sDsnName);
#ifdef WIN32
    snprintf(sErrorLogPath, sizeof(sErrorLogPath), "%s\\log\\%s\\", getenv("QuickMDB_HOME"), sDsnName);
#else
    snprintf(sErrorLogPath, sizeof(sErrorLogPath), "%s/log/%s/", getenv("QuickMDB_HOME"), sDsnName);
#endif
    if(!TMdbNtcDirOper::IsExist(sErrorLogPath))
    {
        TMdbNtcDirOper::MakeFullDir(sErrorLogPath);
    }
    snprintf(m_sFileName,sizeof(m_sFileName),"%sRep_Error_Info_%d.log",\
        sErrorLogPath,TMdbOS::GetPID());
    TADD_DETAIL("ERROR_LOG=[%s].",m_sFileName);
    return iRet;
}
/******************************************************************************
* ��������  :  Push
* ��������  :  ����DSN����
* ����      :  sData push�����ݣ�iLen push���ݵĳ���
* ���      :  ��
* ����ֵ    :  !0 - ʧ��,0 - �ɹ�
* ����      :  cao.peng
*******************************************************************************/
bool TMdbOnlineRepQueue::Push(char * const sData, const int iLen)
{
    TADD_FUNC("Start.");
    bool ShmIsEnable = false;
    int iPushErrorCount = 0;
	CHECK_OBJ(sData);
	TADD_DETAIL("iPushPos = [%d], iPopPos = [%d], iCleanPos = [%d]", m_pOnlineRepQueueShm->iPushPos, m_pOnlineRepQueueShm->iPopPos, m_pOnlineRepQueueShm->iCleanPos);
    TADD_DETAIL("sData = [%s], iLen = [%d]", sData, iLen);
    while(true)
    {
        m_pOnlineRepQueueShm->tPushMutex.Lock(true, &m_pDsn->tCurTime);
        m_iPushPos = m_pOnlineRepQueueShm->iPushPos;
        m_iPopPos  = m_pOnlineRepQueueShm->iPopPos;
		m_iCleanPos = m_pOnlineRepQueueShm->iCleanPos;
        m_iTailPos = m_pOnlineRepQueueShm->iTailPos;
        m_pCurAddr = (char*)m_pOnlineRepQueueShm + m_iPushPos;

		if(m_iPushPos >= m_iCleanPos)
        {
            if(m_iPushPos + iLen > m_pOnlineRepQueueShm->iEndPos)
            {
                if(m_iCleanPos !=  m_pOnlineRepQueueShm->iStartPos)
                {
                    m_pOnlineRepQueueShm->iTailPos = m_pOnlineRepQueueShm->iPushPos;
                    m_pOnlineRepQueueShm->iPushPos  = m_pOnlineRepQueueShm->iStartPos;
                    m_pOnlineRepQueueShm->tPushMutex.UnLock(true); 
                    continue;//������Ե�ͷ����ֱ�ӵ�ͷ�����ܵ�ͷ˵����������
                }
                ShmIsEnable = false;
            }
            else
            {
                ShmIsEnable = true;
            }
        }
        else
        {
            if(m_iPushPos + iLen < m_iCleanPos)
            {
                ShmIsEnable = true;
            }
            else
            {
                ShmIsEnable = false;
            }
        }

        if(ShmIsEnable == true)
        {
            memcpy(m_pCurAddr, sData, iLen);
            m_pOnlineRepQueueShm->iPushPos += iLen;
            m_pOnlineRepQueueShm->tPushMutex.UnLock(true); 
			TADD_DETAIL("iPushPos = [%d], iPopPos = [%d], iCleanPos = [%d]", m_pOnlineRepQueueShm->iPushPos, m_pOnlineRepQueueShm->iPopPos, m_pOnlineRepQueueShm->iCleanPos);
            break;
        }
        else
        {
            TADD_WARNING("wait for Pop.");
            if(iPushErrorCount >= 100)
            {
                m_pOnlineRepQueueShm->tPushMutex.UnLock(true);    
                break;
            }
            iPushErrorCount++;
            if(iPushErrorCount == 99)
            {
                TMdbDateTime::MSleep(10);
            }
            m_pOnlineRepQueueShm->tPushMutex.UnLock(true); 
        }
    }
    TADD_FUNC("Finish.");
    return ShmIsEnable;
}

/******************************************************************************
* ��������  :  Pop
* ��������  :  ��ȡ������һ������
* ����      :  ��
* ���      :  sData �洢pop�������ݣ�iLen pop�����ݵĳ���
* ����ֵ    :  !0 - ʧ��,0 - �ɹ�
* ����      :  cao.peng
*******************************************************************************/    
int TMdbOnlineRepQueue::Pop()
{   
    TADD_FUNC("Start.");
    int iRet = T_SUCCESS;
    int iErrorCount = 0;
    m_iRecordLen = 0;
	
    TADD_DETAIL("iPushPos = [%d], iPopPos = [%d], iCleanPos = [%d]", m_pOnlineRepQueueShm->iPushPos, m_pOnlineRepQueueShm->iPopPos, m_pOnlineRepQueueShm->iCleanPos);

	m_iPushPos = m_pOnlineRepQueueShm->iPushPos;
    m_iPopPos  = m_pOnlineRepQueueShm->iPopPos;
	m_iCleanPos = m_pOnlineRepQueueShm->iCleanPos;
	m_iStartPos = m_pOnlineRepQueueShm->iStartPos;
    m_iTailPos = m_pOnlineRepQueueShm->iTailPos;
    m_pCurAddr = (char*)m_pOnlineRepQueueShm + m_iPopPos;

	if(m_iPushPos == m_iPopPos)
    {
        //����Ϊ��
        m_pszRecord[0] = 0;
        return T_EMPTY;
    }

    if(m_iPopPos == m_iTailPos)
    {
        m_pOnlineRepQueueShm->iPopPos = m_pOnlineRepQueueShm->iStartPos;
        return Pop();
    }
    
    for(iErrorCount = 0; iErrorCount<ERROR_TIMES;iErrorCount++)
    {
        iRet = CheckRepDataIsValid();
        if(iRet == T_SUCCESS)
        {
            memcpy(m_pszRecord,m_pCurAddr,m_iRecordLen);
            m_pszRecord[m_iRecordLen] = 0;
            //memset(m_pCurAddr,0,m_iRecordLen);
            break;
        }
        if(ERROR_TIMES-1 == iErrorCount)
        {
            TMdbDateTime::MSleep(5);  
        }  
    }

    if( iErrorCount == ERROR_TIMES)
    {
        //�ظ�У��10����Ȼ������ȷ��������¼�϶��Ǵ��
        switch(iRet)
        {
            case T_BIG_PushPos : TADD_ERROR(ERROR_UNKNOWN,"pop[%d] + len[%d] > push[%d].",m_iPopPos,m_iRecordLen,m_iPushPos);break;
            case T_BIG_TailPos : TADD_ERROR(ERROR_UNKNOWN,"pop[%d] + len[%d] > tail[%d].",m_iPopPos,m_iRecordLen,m_iTailPos);break;
            case T_BIG_ENDERROR : TADD_ERROR(ERROR_UNKNOWN,"Check End[##] ERROR.");break;
            case T_BIG_BEGINERROR: TADD_ERROR(ERROR_UNKNOWN,"Check Begin[^^] ERROR.");break;
            case T_LENGTH_ERROR: TADD_ERROR(ERROR_UNKNOWN,"Length[%d] ERROR.",m_iRecordLen);break;
            case T_SQLTYPE_ERROR: TADD_ERROR(ERROR_UNKNOWN,"SqlType[%d] ERROR.",m_iSQLType);break;
            case T_SOURCEID_ERROR: TADD_ERROR(ERROR_UNKNOWN,"SyncType[%d] ERROR.",m_iSyncType);break;
            default : break;
        }
        m_iRecordLen = 0;
        m_iRecordLen = GetPosOfNext();
        WriteInvalidData(m_iRecordLen);
        //memset(m_pCurAddr,0,m_iRecordLen);
    }
    m_pOnlineRepQueueShm->iPopPos+=m_iRecordLen;
	m_iPopPos = m_pOnlineRepQueueShm->iPopPos;

	if(m_iCleanPos <= m_iPopPos)
	{
		if(m_iPopPos - m_iCleanPos >= MAX_REP_LINK_BUF_RESERVE_LEN)
		{
			memset((char*)m_pOnlineRepQueueShm + m_iCleanPos, 0, m_iPopPos - m_iCleanPos - MAX_REP_LINK_BUF_RESERVE_LEN);
			m_pOnlineRepQueueShm->iCleanPos=m_pOnlineRepQueueShm->iPopPos - MAX_REP_LINK_BUF_RESERVE_LEN;
		}
	}
	else
	{
		if(m_iTailPos-m_iCleanPos + m_iPopPos-m_iStartPos >= MAX_REP_LINK_BUF_RESERVE_LEN)
		{
			if(m_iPopPos-m_iStartPos >= MAX_REP_LINK_BUF_RESERVE_LEN)
			{
				memset((char*)m_pOnlineRepQueueShm + m_iCleanPos, 0, m_iTailPos-m_iCleanPos);
				memset((char*)m_pOnlineRepQueueShm + m_iStartPos, 0, m_iPopPos-m_iStartPos - MAX_REP_LINK_BUF_RESERVE_LEN);
				m_pOnlineRepQueueShm->iCleanPos=m_pOnlineRepQueueShm->iPopPos-MAX_REP_LINK_BUF_RESERVE_LEN;
			}
			else
			{
				memset((char*)m_pOnlineRepQueueShm + m_iCleanPos, 0, m_iTailPos + m_iPopPos -m_iStartPos - MAX_REP_LINK_BUF_RESERVE_LEN -m_iCleanPos);
				m_pOnlineRepQueueShm->iCleanPos=m_pOnlineRepQueueShm->iTailPos+m_pOnlineRepQueueShm->iPopPos-m_pOnlineRepQueueShm->iStartPos-MAX_REP_LINK_BUF_RESERVE_LEN;
			}
		}
	}
	TADD_DETAIL("iPushPos = [%d], iPopPos = [%d], iCleanPos = [%d]", m_pOnlineRepQueueShm->iPushPos, m_pOnlineRepQueueShm->iPopPos, m_pOnlineRepQueueShm->iCleanPos);
            
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbOnlineRepQueue::RollbackPopPos()
{
	TADD_FUNC("Start.");
	int iRet = 0;
	m_iPopPos = m_pOnlineRepQueueShm->iPopPos;
	m_iCleanPos = m_pOnlineRepQueueShm->iCleanPos;
	m_iTailPos = m_pOnlineRepQueueShm->iTailPos;
	char * pBuf = (char*)m_pOnlineRepQueueShm;
	if(m_iCleanPos == m_iPopPos)
	{
		return iRet;
	}
	else if(m_iCleanPos < m_iPopPos)
	{
		while(m_iCleanPos <= m_iPopPos)
		{
			if(pBuf[m_iCleanPos] == '^' && pBuf[m_iCleanPos+1] == '^')
			{
				if(m_iCleanPos == m_iPopPos)
				{
					TADD_DETAIL("PopPos not change.");
				}
				break;
			}
			m_iCleanPos++;
		}
		if(m_iCleanPos > m_iPopPos)
		{
			TADD_ERROR(ERROR_UNKNOWN, "Data error. Buf=[%s], PopPos = [%d].", pBuf[m_iPopPos], m_iPopPos);
			return ERROR_UNKNOWN;
		}
	}
	else
	{
		while(m_iCleanPos < m_iTailPos)
		{
			if(pBuf[m_iCleanPos] == '^' && pBuf[m_iCleanPos+1] == '^')
			{
				break;
			}
			m_iCleanPos++;
		}
		if(m_iCleanPos == m_iTailPos)
		{
			m_pOnlineRepQueueShm->tPushMutex.Lock(true, &m_pDsn->tCurTime);
			m_pOnlineRepQueueShm->iCleanPos = m_pOnlineRepQueueShm->iStartPos;
			m_pOnlineRepQueueShm->tPushMutex.UnLock(true);
			RollbackPopPos();
			return iRet;
		}
	}
	
	if(m_iCleanPos != m_iPopPos)
	{
		m_pOnlineRepQueueShm->tPushMutex.Lock(true, &m_pDsn->tCurTime);
		m_pOnlineRepQueueShm->iCleanPos = m_iCleanPos;
		m_pOnlineRepQueueShm->iPopPos = m_pOnlineRepQueueShm->iCleanPos;
		m_pOnlineRepQueueShm->tPushMutex.UnLock(true);
		TADD_DETAIL("iCleanPos = [%d], iPopPos = [%d]", m_pOnlineRepQueueShm->iCleanPos, m_pOnlineRepQueueShm->iPopPos);
	}
	
	TADD_FUNC("Finish");
	return iRet;
}


int TMdbOnlineRepQueue::CheckRepDataIsValid()
{
    TADD_FUNC("Start.");
    if(m_pCurAddr[0] == '^' && m_pCurAddr[1] == '^')
    {
        m_iRecordLen= (m_pCurAddr[2]-'0')*1000 + (m_pCurAddr[3]-'0')*100 + (m_pCurAddr[4]-'0')*10 + (m_pCurAddr[5]-'0');
        if(m_iRecordLen < 10 || m_iRecordLen > MAX_VALUE_LEN)
        {
            return T_LENGTH_ERROR;
        }
        if(m_bCheckData)
        {//У������
            m_iSQLType = (m_pCurAddr[49]-'0')*10 + (m_pCurAddr[50]-'0');
            if(m_iSQLType != T_UPDATE && m_iSQLType != T_DELETE && m_iSQLType != T_INSERT)
            {
                return T_SQLTYPE_ERROR;
            }
        }
        //���ݲ��ᳬ�������е�����
        if(m_iPushPos > m_iPopPos)
        {
            if(m_iPopPos + m_iRecordLen > m_iPushPos)
            {
                return T_BIG_PushPos;
            }
        }
        else
        {
            if(m_iPopPos + m_iRecordLen > m_iTailPos)
            {
                return T_BIG_TailPos;
            }   
        } 
        
        //���ݳ��ȱ��븴��Լ����������Ҫ����У��
        if(m_pCurAddr[m_iRecordLen-2] != '#' || m_pCurAddr[m_iRecordLen-1] != '#')
        {
            return T_BIG_ENDERROR;
        }

    }
    else
    {
        m_iRecordLen = 0;
        return T_BIG_BEGINERROR;
    }
    TADD_FUNC("Finish.");
    return T_SUCCESS;
}

/******************************************************************************
* ��������  :  WriteInvalidData
* ��������  :  ������Ч�ļ�¼������Ҫд�ļ�
* ����      :  iLen��Ч���ݳ���
* ���      :  ��
* ����ֵ    :  
* ����      :  cao.peng
*******************************************************************************/
bool TMdbOnlineRepQueue::WriteInvalidData(const int iInvalidLen)
{
    TADD_ERROR(ERROR_UNKNOWN,"Rep Data Error:PushPos=%d,PopPos=%d,TailPos=%d.m_bWriteErrorData=[%s]",m_iPushPos,m_iPopPos,m_iTailPos,true?"true":"false");
    int iLen = 0;
    char sCurtime[MAX_TIME_LEN] = {0};
    char sFileNameOld[MAX_FILE_NAME] = {0};
    if(!m_bWriteErrorData){return true;}
    TADD_ERROR(ERROR_UNKNOWN,"1111");
    if(NULL == m_pFile)
    {
        if((m_pFile =  fopen(m_sFileName, "a+"))==NULL)
    	{
			TADD_ERROR(ERROR_UNKNOWN, "m_pFile is NULL.");
			return ERROR_UNKNOWN;
    	}
        setbuf(m_pFile,NULL);
    }
    TADD_ERROR(ERROR_UNKNOWN,"2222");
    //�ж���Ч�ļ�¼�����Ƿ�ᳬ��32K
    TMdbDateTime::GetCurrentTimeStr(sCurtime);
    memset(m_pszErrorRecord,0,sizeof(m_pszErrorRecord));
    snprintf(m_pszErrorRecord,MAX_VALUE_LEN,"\n%s [Invalid Data] : ",sCurtime);
    iLen = strlen(m_pszErrorRecord);
    if(iInvalidLen >= MAX_VALUE_LEN-iLen)
    {
        memcpy(m_pszErrorRecord+iLen,m_pCurAddr,MAX_VALUE_LEN-iLen);
        iLen = MAX_VALUE_LEN;
        m_pszErrorRecord[iLen-1] = '\0';
    }
    else
    {
        memcpy(m_pszErrorRecord+iLen,m_pCurAddr,iInvalidLen);
        iLen += iInvalidLen;
        m_pszErrorRecord[iLen] = '\0';
    }
    TADD_ERROR(ERROR_UNKNOWN,"3333");
    //���������־�ļ�����
    unsigned long long  iFilesize = 0;
    if (TMdbNtcFileOper::GetFileSize(m_sFileName, iFilesize))
    {
        TADD_ERROR(ERR_OS_GET_FILE_SIZE, "Get file[%s] size failed.", m_sFileName);
    }
    if (iFilesize >= 128*1024*1024)
    {
        //���������,�Ե�ǰʱ��YYYYMMDDHHM24SS��ʾ
        SAFE_CLOSE(m_pFile);
        TMdbNtcFileOper::Remove(sFileNameOld);
        snprintf(sFileNameOld,sizeof(sFileNameOld),"%s.old",m_sFileName);
        //����־�ļ�������
        TMdbNtcFileOper::Rename(m_sFileName,sFileNameOld);
        TMdbNtcFileOper::Remove(m_sFileName);
        m_pFile = fopen (m_sFileName,"a+");
        if(m_pFile == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Open file [%s] fail.\n",m_sFileName);
            return false;
        }
    }
    if(fwrite(m_pszErrorRecord,iLen,1,m_pFile) == 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"fwrite() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));
        return false;
    }
    TADD_ERROR(ERROR_UNKNOWN,"4444[%s],name[%s]",m_pszErrorRecord,m_sFileName);
    return true;
}


/******************************************************************************
* ��������  :  GetPosOfNext
* ��������  :  ��ȡ��һ����¼��λ��
* ����      :  ��
* ���      :  ��
* ����ֵ    :  ��һ����¼���λ��
* ����      :  cao.peng
*******************************************************************************/
int TMdbOnlineRepQueue::GetPosOfNext()
{
    int iLength = 0;
    if(m_iPopPos < m_iPushPos)
    {
        iLength = m_iPushPos - m_iPopPos;
    }
    else
    {
        iLength = m_iTailPos - m_iPopPos;
    }

    int i = 1;
    for(i = 1; i<iLength;i++)
    {
        if(m_pCurAddr[i] == '^' && m_pCurAddr[i+1] == '^')
        {
            break;
        }
    }
    return i;
    
}

int TMdbOnlineRepQueue::GetRecordLen()
{
    return m_iRecordLen;
}

char* TMdbOnlineRepQueue::GetData()
{
    return m_pszRecord;
}

int TMdbOnlineRepQueue::GetUsedPercentage()
{
	if(m_pOnlineRepQueueShm->iPushPos < m_pOnlineRepQueueShm->iCleanPos)
	{
		return (((m_pOnlineRepQueueShm->iTailPos - m_pOnlineRepQueueShm->iCleanPos)+(m_pOnlineRepQueueShm->iPushPos - m_pOnlineRepQueueShm->iStartPos))*100)/(m_pOnlineRepQueueShm->iEndPos - m_pOnlineRepQueueShm->iStartPos);
	}
	else
	{
		return ((m_pOnlineRepQueueShm->iPushPos - m_pOnlineRepQueueShm->iCleanPos)*100)/(m_pOnlineRepQueueShm->iEndPos - m_pOnlineRepQueueShm->iStartPos);
	}
}


//}
