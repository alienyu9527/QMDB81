/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    TCacheLog.cpp		
*@Description�� ����ʽ�����־�ļ�����־��Ϣ��д�뻺�棬Ȼ��ʱ�������
*@Author:			jin.shaohua
*@Date��	    2012.11
*@History:
******************************************************************************************/
#include "Helper/mdbCacheLog.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"


//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

/******************************************************************************
* ��������  :  TCacheLog
* ��������  :  ����
* ����      :  
* ����      :  
* ���      :  
* ����ֵ    :  
* ����      :  jin.shaohua
*******************************************************************************/
TCacheLog::TCacheLog():
m_iCycleType(0),
m_iCycleValue(0),
m_pCache(NULL),
m_iCacheSize(0),
m_iCurCachePos(0),
m_iLogCount(0),
m_tNextLogTime(0)
{

}
/******************************************************************************
* ��������	:  ~TCacheLog
* ��������	:  ����
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
TCacheLog::~TCacheLog()
{
    FlushImmediately();
    SAFE_DELETE_ARRAY(m_pCache);
}
/******************************************************************************
* ��������	:  SetLogFile
* ��������	:  ������־�ļ�
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TCacheLog::SetLogFile(const char * sFullPathName,int iLogFileSizeM)
{
    int iRet = 0;
    FlushImmediately();
    CHECK_OBJ(sFullPathName);
    if(strlen(sFullPathName) > sizeof(m_sLogFile))
    {
        CHECK_RET(ERR_APP_FLUSH_LOG,"sFullPathName[%s] is too log",sFullPathName);
    }
    SAFESTRCPY(m_sLogFile,sizeof(m_sLogFile),sFullPathName);
    if(iLogFileSizeM < 16 || iLogFileSizeM > 1024*10)
    {
        CHECK_RET(ERR_APP_INVALID_PARAM,"10G >= LogFileSize >= 16M ");
    }
    m_iLogFileSizeM = iLogFileSizeM;
    return iRet;
}
/******************************************************************************
* ��������	:  SetFlushCycle
* ��������	:  ��������ļ�����
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TCacheLog::SetFlushCycle(int iCycleType,int iCycleValue)
{
    TADD_FUNC("Start.iCycleType[%d] error.iCycleValue[%d]",iCycleType,iCycleValue);
    int iRet = 0;
    FlushImmediately();
    switch(iCycleType)
    {
        case FLUSH_CYCLE_TIME:// ��λ: s
            CHECK_RET(AllocCache(1024*1024),"AllocCache(%d) failed",MAX_CACHE_LOG_SIZE);
            break;
        case FLUSH_CYCLE_SIZE:
            CHECK_RET(AllocCache(iCycleValue*1024),"AllocCache(%d) failed",MAX_CACHE_LOG_SIZE);
            break;
        case FLUSH_CYCLE_COUNT:
            CHECK_RET(AllocCache(1024*1024),"AllocCache(%d) failed",MAX_CACHE_LOG_SIZE);
            break;
        default:
            CHECK_RET(ERR_APP_FLUSH_LOG,"iCycleType[%d] error.iCycleValue[%d]",iCycleType,iCycleValue);
            break;
    }
    m_iCycleType  = iCycleType;
    m_iCycleValue = iCycleValue;
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  Log
* ��������	:  ��־��¼
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TCacheLog::Log(const char * sInfo)
{
    int iRet = 0;
    CHECK_OBJ(m_pCache); 
    int iStrLen = strlen(sInfo);
    m_iLogCount ++;//����
    if(iStrLen > m_iCacheSize - m_iCurCachePos)
    {//��Ҫ�������е�����ˢ��
        CHECK_RET(CacheToFile(),"FlushToFile failed.");
    }
    if(iStrLen > m_iCacheSize - m_iCurCachePos)
    {//�����Ѿ������һ����־��Ϣ��ֱ��ˢ�����ļ�
        FILE * fp = fopen(m_sLogFile, "a+");
        if(fp == NULL)
        {
            printf("TObserveBase::FlushToFile: Open file [%s] failed. \n", m_sLogFile);
            return 0;
        }
        CheckAndBackup(m_sLogFile,fp,m_iLogFileSizeM);
        fprintf(fp, "%s",sInfo);
        fflush(fp);
        fclose(fp);
    }
    else
    {//��ӵ�������
        SAFESTRCPY(m_pCache+m_iCurCachePos,m_iCacheSize - m_iCurCachePos,sInfo);
        m_iCurCachePos += iStrLen;
    }
    if(bNeedToFlush())
    {
        CHECK_RET(CacheToFile(),"FlushToFile failed.");
    }
    return iRet;
}

/******************************************************************************
* ��������	:  AllocCache
* ��������	:  
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TCacheLog::AllocCache(int iSize)
{
    TADD_FUNC("Start.size=[%d].",iSize);
    int iRet = 0;
    if(iSize <= 0 || iSize > MAX_CACHE_LOG_SIZE)
    {
        CHECK_RET(ERR_APP_FLUSH_LOG,"iSize[%d] is not in[0~%d]",iSize,MAX_CACHE_LOG_SIZE);
    }
    m_iCacheSize    = 0;
    m_iCurCachePos = 0;
    SAFE_DELETE_ARRAY(m_pCache);
    m_pCache = new (std::nothrow) char[iSize];
    CHECK_OBJ(m_pCache);
    m_iCacheSize = iSize;
    TADD_FUNC("Finish");
    return iRet;
}
/******************************************************************************
* ��������	:  CacheToFile
* ��������	:  ����ˢ�����ļ�
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TCacheLog::CacheToFile()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(m_pCache);
    FILE * fp = fopen(m_sLogFile, "a+");
    if(fp == NULL)
    {
        printf("TObserveBase::FlushToFile: Open file [%s] failed. \n", m_sLogFile);
        return 0;
    }
    CheckAndBackup(m_sLogFile,fp,m_iLogFileSizeM);
    fprintf(fp, "%s",m_pCache);
    fflush(fp);
    fclose(fp);
    m_pCache[0] = '\0';
    m_iCurCachePos = 0;
    //��ռ�����
    switch(m_iCycleType)
    {
        case FLUSH_CYCLE_TIME:// ��λ: s
            time(&m_tNextLogTime);
            m_tNextLogTime += m_iCycleValue;
            break;
        case FLUSH_CYCLE_COUNT:
            m_iLogCount = 0;
            break;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  bNeedToFlush
* ��������	:  �Ƿ���Ҫˢ��
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
bool TCacheLog::bNeedToFlush()
{
    bool bRet = true;
    switch(m_iCycleType)
    {
        case FLUSH_CYCLE_TIME:// ��λ: s
            {
                time_t tCurTime;
                time(&tCurTime);
                bRet = tCurTime > m_tNextLogTime ? true:false;
            }
            break;
        case FLUSH_CYCLE_SIZE:
            {
                bRet = m_iCurCachePos > m_iCycleValue*1024 ? true:false;
            }
            break;
        case FLUSH_CYCLE_COUNT:
            {
                bRet = m_iLogCount > m_iCycleValue ? true:false;
            }
            break;
    }
    return bRet;
}
/******************************************************************************
* ��������	:  FlushImmediately
* ��������	:  ����ˢ��
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  
* ����		:  jin.shaohua
*******************************************************************************/
int TCacheLog::FlushImmediately()
{
    int iRet = 0;
    if(m_iCurCachePos > 0)
    {//�����֮ǰ�Ļ���
        CHECK_RET(CacheToFile(),"CacheToFile failed.");
    }
    return iRet;
}

int TCacheLog::CheckAndBackup(const char * sFileName , FILE*& fp,int iMaxSizeM)
{
    if(NULL == sFileName)
    {
        return ERR_APP_INVALID_PARAM;
    }
    unsigned long long filesize = 0 ;
    bool bRet =  TMdbNtcFileOper::GetFileSize((char*)sFileName,filesize);
    if (bRet == false) 
    {
        TADD_ERROR(ERR_OS_GET_FILE_SIZE, "Get file[%s] size failed.", sFileName);
    	return ERR_OS_GET_FILE_SIZE;
    }
    if (filesize >= (unsigned long long)iMaxSizeM*1024*1024)	
    {
        SAFE_CLOSE(fp);
        char  sFileNameOld[MAX_PATH_NAME_LEN] = {0};
        sprintf(sFileNameOld,"%s.%s", sFileName, "old");
        TMdbNtcDirOper::Remove(sFileNameOld);
        //����־�ļ�������
        TMdbNtcDirOper::Rename(sFileName,sFileNameOld);
        TMdbNtcDirOper::Remove(sFileName);
        fp = fopen (sFileName,"a+");
        if(fp == NULL)
        {
            printf("Check And Backup Log File: Open file [%s] fail.\n",sFileName);
            return ERR_APP_INVALID_PARAM;
        }
    }
    return 0 ;
}

//}
