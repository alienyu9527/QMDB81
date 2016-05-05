////////////////////////////////////////////////
// Name: mdbFileList.h
// Author: Li.ShuGang
// Date: 2009/03/27
// Description: 读取minidb生成的日志文件名
////////////////////////////////////////////////
/*
* $History: mdbFileList.h $
 * 
 * *****************  Version 1.0  ***************** 
*/
#ifndef __MINI_DATABASE_FILE_LIST_H__
#define __MINI_DATABASE_FILE_LIST_H__

#include "Helper/mdbStruct.h"

#ifdef WIN32
#include <windows.h>
#endif

//namespace QuickMDB{

#define MAX_FILE_COUNTS 1000

class TMdbFileList
{
public:
    TMdbFileList();
    ~TMdbFileList();
    
public:
	/******************************************************************************
	* 函数名称	:  Init
	* 函数描述	:  设置读取的目录
	* 输入		:  pszPath 文件路径
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	*******************************************************************************/
    int Init(const char* pszPath);

	/******************************************************************************
	* 函数名称	:  GetFileList
	* 函数描述	:  获取文件列表(文件名升级)
	* 输入		:  iCounts ，iPos
	* 输入		:  pszHeadFlag 文件头标示(默认Ora_开头)，pszTail
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	*******************************************************************************/
    int GetFileList(int iCounts, int iPos, const char* pszHeadFlag="Ora_", const char* pszTail="");	

    /******************************************************************************
    * 函数名称	:  GetFileList
    * 函数描述	:  获取文件列表(文件名升级)
    * 输入		:  pszHeadFlag 文件头标示(默认Ora_开头)，pszTail
    * 输出		:  无
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  li.ming
    *******************************************************************************/
    int GetFileList(const char* pszHeadFlag, const char* pszTail="");
    
    /******************************************************************************
	* 函数名称	:  Next
	* 函数描述	:  获取下一个文件名(带路径)
	* 输入		:  无
	* 输出		:  pszFullFileName 文件名
	* 返回值	:  0 - 成功!0 -失败
	*******************************************************************************/
    int Next(char* pszFullFileName);
	int GetFileNameFromFullFileName(char* &pszFileName);

    /******************************************************************************
	* 函数名称	:  Clear
	* 函数描述	:  清除记录，重新开始
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 - 成功!0 -失败
	*******************************************************************************/
    void Clear();

    /******************************************************************************
	* 函数名称	:  GetFileCounts
	* 函数描述	:  获取当前的文件数
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  当前文件数量
	*******************************************************************************/
    int GetFileCounts()
    {
        return  m_iFileCounts;   
    }

private:
    char m_sPath[MAX_PATH_NAME_LEN];   //路径名
    int  m_iFileCounts;                //本次读取文件数
    char *m_FileName[MAX_FILE_COUNTS+1]; //文件名   
    int  m_iCurPos;                    //这次文件位置
};

//}

#endif //__MINI_DATABASE_FILE_LIST_H__


