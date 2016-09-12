////////////////////////////////////////////////
// Name: mdbFileList.cpp
// Author: Li.ShuGang
// Date: 2009/03/27
// Description: 读取minidb生成的日志文件名
////////////////////////////////////////////////
/*
* $History: mdbFileList.cpp $
 * 
 * *****************  Version 1.0  ***************** 
*/
#include "Helper/mdbFileList.h"
#include "Helper/TThreadLog.h"

#ifdef WIN32
	#include <windows.h>
	#include <direct.h>
#else
	#include "dirent.h"
#endif

//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

TMdbFileList::TMdbFileList()
{
    memset(m_sPath, 0, sizeof(m_sPath));
    m_iFileCounts = 0;
    m_iCurPos     = 0;
    
    for(int i=0; i<MAX_FILE_COUNTS; ++i)
    {
        m_FileName[i] = NULL; 
    }
}


TMdbFileList::~TMdbFileList()
{
    for(int i=0; i<MAX_FILE_COUNTS; ++i)
    {
        SAFE_DELETE_ARRAY(m_FileName[i]);
    }
}


/******************************************************************************
* 函数名称	:  Init
* 函数描述	:  设置读取的目录
* 输入		:  pszPath 文件路径
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
*******************************************************************************/
int TMdbFileList::Init(const char* pszPath)
{
	if(NULL == pszPath)
    {
    	TADD_ERROR(ERROR_UNKNOWN,"[%s : %d] : TMdbFileList::Init(pszPath) failed, Param is null.", __FILE__, __LINE__);
        return -1;   
    }
    TADD_FUNC("TMdbFileList::Init(%s) : Start.", pszPath);
    if(strlen(pszPath) >= MAX_PATH_NAME_LEN-MAX_NAME_LEN)
    {
        TADD_ERROR(ERROR_UNKNOWN,"[%s : %d] : TMdbFileList::Init(%s) failed, Path-Name is too long.", __FILE__, __LINE__, pszPath);
        return -1;   
    }
    
    //初始化文件列表
    for(int i=0; i<MAX_FILE_COUNTS; ++i)
    {
        if(m_FileName[i] == NULL)
        {
            m_FileName[i] = new(std::nothrow) char[MAX_PATH_NAME_LEN];
            if(m_FileName[i] == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN,"[%s : %d] : TMdbFileList::Init(%s) failed, out of memory.", __FILE__, __LINE__, pszPath);
                return ERR_OS_NO_MEMROY;    
            }   
        } 
    }
    
    //记录路径名称
    //strcpy(m_sPath, pszPath);
    SAFESTRCPY(m_sPath,sizeof(m_sPath),pszPath);
    if(m_sPath[strlen(m_sPath)-1] != '/')
    {
        m_sPath[strlen(m_sPath)] = '/';  
    }
    
    m_iFileCounts = 0;
    m_iCurPos     = 0;
    
    TADD_FUNC("TMdbFileList::Init(%s) : Finish.", pszPath);
    
    return 0;
}


/******************************************************************************
* 函数名称	:  GetFileList
* 函数描述	:  获取文件列表(文件名升级)
* 输入		:  iCounts ，iPos
* 输入		:  pszHeadFlag 文件头标示(默认Ora_开头)，pszTail
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
*******************************************************************************/
int TMdbFileList::GetFileList(int iCounts, int iPos, const char* pszHeadFlag, const char* pszTail)
{
    TADD_FUNC("m_sPath=[%s].", m_sPath);
    int iRet = 0;
    CHECK_OBJ(pszHeadFlag);
    CHECK_OBJ(pszTail);
    //打开目录
    m_iFileCounts = 0;
    m_iCurPos     = 0;
    DIR *dp = opendir(m_sPath);
    if(dp == NULL)
    {
    	bool bCreate = TMdbNtcDirOper::MakeFullDir(m_sPath);
    	if(bCreate == true)
    	{
    		return 0;
    	}
    	else
    	{
    	    CHECK_RET(ERR_OS_CREATE_DIR,"Mkdir(%s) failed",m_sPath);
    	}     
    }
    //开始读取
    struct dirent *dirp;
    while((dirp=readdir(dp)) != NULL)
    {
        TADD_DETAIL("iPos=%d, d_name=[%s], pszHeadFlag=[%s], pszTail=[%s].", iPos, dirp->d_name, pszHeadFlag, pszTail);
        size_t iLen = strlen(dirp->d_name);
        if(strncmp(dirp->d_name, pszHeadFlag, strlen(pszHeadFlag)) == 0)
        { 
        	bool bFlag = true;
        	if(strlen(pszTail) > 0)
        	{
        		bFlag = strncmp(&dirp->d_name[iLen-strlen(pszTail)], pszTail, strlen(pszTail)) == 0;
        	}
        	if(bFlag == false)
        	{
        		TADD_DETAIL("[%s] != tail[%s].", dirp->d_name, pszTail);
        		continue;
        	}
        	if(iPos == -1)
        	{
        		if(iLen > 14 && iLen < 28)
        		{
	        		sprintf(m_FileName[m_iFileCounts], "%s",dirp->d_name);
	        		TADD_DETAIL("TMdbFileList::GetFileList() : d_name=[%s] is OK.", dirp->d_name);
					++m_iFileCounts;
		      	}	

        	}
        	else
        	{
	            long iNumber = 0;
	            iNumber = dirp->d_name[3] - '0';
	            if(iNumber%iCounts == iPos)
	            {            
	                sprintf(m_FileName[m_iFileCounts], "%s",dirp->d_name);
	                TADD_DETAIL("d_name=[%s] is valid.", dirp->d_name);
	                ++m_iFileCounts;
	            }
	        }
            if(m_iFileCounts >= MAX_FILE_COUNTS)
            {
                break;    
            }
        }
    }
    
    closedir(dp);
    //对文件进行排序
    char sTemp[MAX_PATH_NAME_LEN];
    for(int i=0; i<m_iFileCounts; ++i)
    {
    	for(int j=i+1; j<m_iFileCounts; ++j)
    	{
    		//如果前面的比后面的大,交换数据
    		if(strcmp(m_FileName[i], m_FileName[j]) >= 0)
    		{
    			memset(sTemp,0,MAX_PATH_NAME_LEN);
				strcpy(sTemp, m_FileName[i]);
				strcpy(m_FileName[i], m_FileName[j]);
				strcpy(m_FileName[j], sTemp);
    		}	
    	}    	
    }
    TADD_FUNC("(m_iFileCounts=%d) : Finish.", m_iFileCounts);
    return iRet;
}

/******************************************************************************
* 函数名称	:  Next
* 函数描述	:  获取下一个文件名(带路径)
* 输入		:  无
* 输出		:  pszFullFileName 文件名
* 返回值	:  0 - 成功!0 -失败
*******************************************************************************/
int TMdbFileList::Next(char* pszFullFileName)
{
    TADD_FUNC("TMdbFileList::Next() : Start.");
    
    if(m_iFileCounts == m_iCurPos)
    {
        TADD_FUNC("TMdbFileList::Next() : Finish(End).");
        return -1;    
    }
	sprintf(pszFullFileName, "%s%s",m_sPath,m_FileName[m_iCurPos]);
    //strcpy(pszFullFileName, m_FileName[m_iCurPos]);    
    ++m_iCurPos;
    
    TADD_FUNC("TMdbFileList::Next() : Finish.");
    
    return 0;
}
int TMdbFileList::GetFileNameFromFullFileName(char* &pszFileName)
{

	if(m_iCurPos == 0 ||m_iCurPos > m_iFileCounts)
		return -1;
	pszFileName = m_FileName[m_iCurPos-1];
	return 0;
	
}
/******************************************************************************
* 函数名称	:  Clear
* 函数描述	:  清除记录，重新开始
* 输入		:  无
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
*******************************************************************************/
void TMdbFileList::Clear()
{
    TADD_FUNC("TMdbFileList::Clear() : Start.");
    
    m_iFileCounts = 0;
    m_iCurPos     = 0;
    
    TADD_FUNC("TMdbFileList::Clear() : Finish."); 
}

/******************************************************************************
* 函数名称	:  GetFileList
* 函数描述	:  获取文件列表(文件名升级)
* 输入		:  pszHeadFlag 文件头标示(默认Ora_开头)，pszTail
* 输出		:  无
* 返回值	:  0 - 成功!0 -失败
* 作者		:  li.ming
*******************************************************************************/
int TMdbFileList::GetFileList(const char* pszHeadFlag, const char* pszTail)
{
    TADD_FUNC("m_sPath=[%s].", m_sPath);
    int iRet = 0;
    CHECK_OBJ(pszHeadFlag);
    CHECK_OBJ(pszTail);
    //打开目录
    m_iFileCounts = 0;
    m_iCurPos     = 0;
    DIR *dp = opendir(m_sPath);
    if(dp == NULL)
    {
    	bool bCreate = TMdbNtcDirOper::MakeFullDir(m_sPath);
    	if(bCreate == true)
    	{
    		return 0;
    	}
    	else
    	{
    	    CHECK_RET(ERR_OS_CREATE_DIR,"Mkdir(%s) failed",m_sPath);
    	}     
    }
    //开始读取
    struct dirent *dirp;
    while((dirp=readdir(dp)) != NULL)
    {
        TADD_DETAIL("d_name=[%s], pszHeadFlag=[%s], pszTail=[%s].", dirp->d_name, pszHeadFlag, pszTail);
        size_t iLen = strlen(dirp->d_name);
        if(strncmp(dirp->d_name, pszHeadFlag, strlen(pszHeadFlag)) == 0)
        { 
        	bool bFlag = true;
        	if(strlen(pszTail) > 0)
        	{
        		bFlag = strncmp(&dirp->d_name[iLen-strlen(pszTail)], pszTail, strlen(pszTail)) == 0;
        	}
        	if(bFlag == false)
        	{
        		TADD_DETAIL("[%s] != tail[%s].", dirp->d_name, pszTail);
        		continue;
        	}

            snprintf(m_FileName[m_iFileCounts],MAX_PATH_NAME_LEN, "%s",dirp->d_name);
            TADD_DETAIL("TDgFileList::GetFileList() : d_name=[%s] is OK.", dirp->d_name);
            ++m_iFileCounts;
            if(m_iFileCounts >= MAX_FILE_COUNTS)
            {
                break;    
            }
        }
    }
    
    closedir(dp);
    //对文件进行排序
    char sTemp[MAX_PATH_NAME_LEN];
    for(int i=0; i<m_iFileCounts; ++i)
    {
    	for(int j=i+1; j<m_iFileCounts; ++j)
    	{
    		//如果前面的比后面的大,交换数据
    		if(strcmp(m_FileName[i], m_FileName[j]) >= 0)
    		{
                memset(sTemp,0,MAX_PATH_NAME_LEN);
                strncpy(sTemp, m_FileName[i],sizeof(sTemp)-1);
                strncpy(m_FileName[i], m_FileName[j],MAX_PATH_NAME_LEN-1);
                strncpy(m_FileName[j], sTemp,MAX_PATH_NAME_LEN-1);
    		}	
    	}    	
    }
    TADD_FUNC("(m_iFileCounts=%d) : Finish.", m_iFileCounts);
    return iRet;
}

//}

