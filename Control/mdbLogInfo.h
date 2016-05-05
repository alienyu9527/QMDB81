/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbLogInfo.h		
*@Description： 负责设置内存数据库的各种日志信息
*@Author:		li.shugang
*@Date：	    2009年04月23日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_LOG_INFORMATION_H__
#define __MINI_DATABASE_LOG_INFORMATION_H__

#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
#include "Control/mdbMgrShm.h"

//namespace QuickMDB{

    class TMdbLogInfo
    {
    public:
        TMdbLogInfo();
        ~TMdbLogInfo();
        
        /******************************************************************************
        * 函数名称	:  Connect()
        * 函数描述	:  链接某个DSN，但是不在管理区注册任何信息    
        * 输入		:  pszDSN, 锁管理区所属的DSN 
        * 输出		:  无
        * 返回值	:  成功返回0, 失败返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Connect(const char* pszDSN);   
        

        /******************************************************************************
        * 函数名称	:  SetLocalLink()
        * 函数描述	:  设置本地链接日志级别    
        * 输入		:  iPid，进程ID
        * 输入		:  iTid，线程ID
        * 输入		:  iLogLevel, 日志级别
        * 输出		:  无
        * 返回值	:  成功返回0, 失败返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int SetLocalLink(int iPid, int iTid, int iLogLevel);
        int SetLocalLink(int iPid, int iLogLevel);
        int SetLocalLink(TMdbDSN  *pDsn, int iPid, int iLogLevel);
        int SetLocalLink(TMdbDSN  *pDsn, int iPid, int iTid, int iLogLevel);
        /******************************************************************************
        * 函数名称	:  SetRemoteLink()
        * 函数描述	:  设置远程链接日志级别    
        * 输入		:  pszIP, 远程IP
        * 输入		:  iLogLevel, 日志级别
        * 输出		:  无
        * 返回值	:  成功返回0, 失败返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int SetRemoteLink(const char* pszIP, int iLogLevel);
        int SetRemoteLink(TMdbDSN  *pDsn,int pid, int iLogLevel);
        int SetRemoteLink(TMdbDSN  *pDsn,int pid,int tid, int iLogLevel);
        /******************************************************************************
        * 函数名称	:  SetMonitor()
        * 函数描述	:  设置监控进程的日志级别    
        * 输入		:  iLogLevel, 日志级别
        * 输出		:  无
        * 返回值	:  成功返回0, 失败返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int SetMonitor(int iLogLevel);
        

        /******************************************************************************
        * 函数名称	:  SetProc()
        * 函数描述	:  设置进程日志信息,根据进程号
        * 输入		:  iPid, 进程ID
        * 输入		:  iLogLevel, 日志级别
        * 输出		:  无
        * 返回值	:  成功返回0, 失败返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int SetProc(int iPid, int iLogLevel);
    	int SetProc(TMdbDSN *pDsn, char *processName, int iLogLevel);
    	int SetProc(TMdbDSN  *pDsn, int ipid, int iLogLevel);
    	/******************************************************************************
        * 函数名称	:  SetProc()
        * 函数描述	:  设置进程日志信息，根据进程名。支持模糊匹配 
        * 输入		:  pProcessName, 进程名
        * 输入		:  iLogLevel, 日志级别
        * 输出		:  无
        * 返回值	:  成功返回0, 失败返回错误码
        * 作者		:  zhang.lin
        *******************************************************************************/
    	int SetProc(const char *pProcessName, int iLogLevel);
    	
    private:
        
        TMdbShmDSN *m_pShmDSN;
        TMdbDSN    *m_pDsn;     
    };
//}

#endif //__MINI_DATABASE_LOG_INFORMATION_H__



