/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    TCacheLog.cpp		
*@Description： 缓存式输出日志文件，日志信息先写入缓存，然后定时或定量输出
*@Author:			jin.shaohua
*@Date：	    2012.11
*@History:
******************************************************************************************/
#include "Helper/mdbCacheLog.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbOS.h"


//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

/******************************************************************************
* 函数名称  :  TCacheLog
* 函数描述  :  构造
* 输入      :  
* 输入      :  
* 输出      :  
* 返回值    :  
* 作者      :  jin.shaohua
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
* 函数名称	:  ~TCacheLog
* 函数描述	:  析构
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
TCacheLog::~TCacheLog()
{
    FlushImmediately();
    SAFE_DELETE_ARRAY(m_pCache);
}
/******************************************************************************
* 函数名称	:  SetLogFile
* 函数描述	:  设置日志文件
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
* 函数名称	:  SetFlushCycle
* 函数描述	:  设置输出文件周期
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TCacheLog::SetFlushCycle(int iCycleType,int iCycleValue)
{
    TADD_FUNC("Start.iCycleType[%d] error.iCycleValue[%d]",iCycleType,iCycleValue);
    int iRet = 0;
    FlushImmediately();
    switch(iCycleType)
    {
        case FLUSH_CYCLE_TIME:// 单位: s
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
* 函数名称	:  Log
* 函数描述	:  日志记录
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TCacheLog::Log(const char * sInfo)
{
    int iRet = 0;
    CHECK_OBJ(m_pCache); 
    int iStrLen = strlen(sInfo);
    m_iLogCount ++;//计数
    if(iStrLen > m_iCacheSize - m_iCurCachePos)
    {//需要将缓存中的立刻刷出
        CHECK_RET(CacheToFile(),"FlushToFile failed.");
    }
    if(iStrLen > m_iCacheSize - m_iCurCachePos)
    {//缓存已经不足放一个日志信息。直接刷出到文件
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
    {//添加到缓存中
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
* 函数名称	:  AllocCache
* 函数描述	:  
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
* 函数名称	:  CacheToFile
* 函数描述	:  缓存刷出到文件
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
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
    //清空计数器
    switch(m_iCycleType)
    {
        case FLUSH_CYCLE_TIME:// 单位: s
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
* 函数名称	:  bNeedToFlush
* 函数描述	:  是否需要刷出
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
bool TCacheLog::bNeedToFlush()
{
    bool bRet = true;
    switch(m_iCycleType)
    {
        case FLUSH_CYCLE_TIME:// 单位: s
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
* 函数名称	:  FlushImmediately
* 函数描述	:  立刻刷出
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
int TCacheLog::FlushImmediately()
{
    int iRet = 0;
    if(m_iCurCachePos > 0)
    {//先清空之前的缓存
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
        //将日志文件重命名
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
