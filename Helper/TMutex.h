////////////////////////////////////////////////
// Name: TMutex.h
// Author: Li.ShuGang
// Date: 2008/10/28
// Description: 锁类
////////////////////////////////////////////////

#ifndef __MEMORY_DATABASE_MUTEX_H_
#define __MEMORY_DATABASE_MUTEX_H_

#include <stdio.h>
#include <string.h>
#include <string>
#ifdef WIN32

#pragma warning(disable:4312)           //unsigned int 或者int 转换到HANDLE
#pragma warning(disable:4311)           //void*转换到long
#pragma warning(disable:4996)

#include <windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
//#include "Helper/TThreadLog.h"



#endif


//namespace QuickMDB{

#define STR_FORMAT_SEM_ID               "SEM%09d"           //WIN32下用于生成标识名称
#define SEM_ID_MOD                      10000000000         //用于生成标识取模
#define MAX_SEM_ID_SIZE                 64                  //SEM_ID字符串格式的长度
#define MAX_SEM_ERROR_MSG_SIZE          1024                //信号量类错误信息的最大长度
#define SEM_BASE                        200                 //信号量操作信息起始常量

#define ERROR_SEM_PARAMETER             SEM_BASE+1          //参数错误
#define ERROR_SEM_CREATE                SEM_BASE+2          //信号量创建失败
#define ERROR_SEM_OPEN                  SEM_BASE+3          //信号量获取信号标识或句柄失败
#define ERROR_SEM_SET_VALUE             SEM_BASE+4          //信号量设置信号值失败
#define ERROR_SEM_P_OPERATION           SEM_BASE+5          //信号量P操作失败
#define ERROR_SEM_V_OPERATION           SEM_BASE+6          //信号量V操作失败
#define ERROR_SEM_DESTROY               SEM_BASE+7          //信号量删除失败
#define ERROR_SEM_EXIST                 SEM_BASE+8          //信号量已经存在
#define ERROR_SEM_FTOK                  SEM_BASE+9          //FTOK失败
#define ERROR_SEM_GETVAL                SEM_BASE+10         //信号量获取值失败

using namespace std;

//互斥锁是阻塞的，考虑到计费业务的特点，暂不设置其它类型的锁
class TMutex //: public TBaseMutex
{
public:
	TMutex(bool bFlag = false);  //如果在构造的时候就初始化bFlag=true
	~TMutex();	

public:
	/******************************************************************************
	* 函数名称	:  Create()
	* 函数描述	:  创建锁 
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 成功，非0失败
	* 作者		:  li.shugang
	*******************************************************************************/
    int Create();
	/******************************************************************************
	* 函数名称	:  Destroy()
	* 函数描述	:  销毁锁 
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 成功，非0失败
	* 作者		:  li.shugang
	*******************************************************************************/
    int Destroy();
	/******************************************************************************
	* 函数名称	:  Lock()
	* 函数描述	:  加锁
	* 输入		:  bFlag 锁是否已经创建，tCurtime 加锁的时间
	* 输出		:  无
	* 返回值	:  0 成功，非0失败
	* 作者		:  li.shugang
	*******************************************************************************/
    int Lock(bool bFlag,  struct timeval *tCurtime = NULL);
    /******************************************************************************
	* 函数名称	:  UnLock()
	* 函数描述	:  解锁
	* 输入		:  bFlag 锁是否已经创建
	* 输出		:  无
	* 返回值	:  0 成功，-1 失败
	* 作者		:  li.shugang
	*******************************************************************************/
    int UnLock(bool bFlag,  const char* pszTime=NULL);
    /******************************************************************************
	* 函数名称	:  TryLock()
	* 函数描述	:  非阻塞调用模式
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  成功则返回0, 出错则返回错误编号
	* 作者		:  li.shugang
	*******************************************************************************/
    int TryLock();
    
    int GetLockPID(){return m_iLockPID;}
	bool IsLock(){return bIsLock;}
	bool IsCreate(){return bIsCreate;}
	
    struct timeval m_tCurTime;
	
private:
	pthread_mutex_t     mutex;
	pthread_mutexattr_t mattr;
	bool bIsCreate;
	bool bIsLock;
    int    m_iLockPID;//锁住的进程ID
};


//不需要做超时检测的锁，尽量减少空间消耗
class TMiniMutex
{
public:
	TMiniMutex(bool bFlag = false);  //如果在构造的时候就初始化bFlag=true
	~TMiniMutex();	

public:
    int Create();
    int Destroy();
    int Lock(bool bFlag);
    int UnLock(bool bFlag);
    int TryLock();
	bool IsLock(){return bIsLock;}
	bool IsCreate(){return bIsCreate;}

private:
	pthread_mutex_t     mutex;
	pthread_mutexattr_t mattr;   
	bool bIsCreate;
	bool bIsLock;
};



//互斥锁是阻塞的，考虑到计费业务的特点，暂不设置其它类型的锁
class TOMutex //: public TBaseMutex
{
public:
	TOMutex(bool bFlag = false);  //如果在构造的时候就初始化bFlag=true
	~TOMutex();	

public:
	/******************************************************************************
	* 函数名称	:  Create()
	* 函数描述	:  创建锁 
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 成功，非0失败
	* 作者		:  li.shugang
	*******************************************************************************/
    int Create();
	/******************************************************************************
	* 函数名称	:  Destroy()
	* 函数描述	:  销毁锁 
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  0 成功，非0失败
	* 作者		:  li.shugang
	*******************************************************************************/
    int Destroy();        
	/******************************************************************************
	* 函数名称	:  Lock()
	* 函数描述	:  加锁
	* 输入		:  bFlag 锁是否已经创建，tCurtime 加锁的时间
	* 输出		:  无
	* 返回值	:  0 成功，非0失败
	* 作者		:  li.shugang
	*******************************************************************************/
    int Lock(bool bFlag, int iPID, int iTID); 
    /******************************************************************************
	* 函数名称	:  UnLock()
	* 函数描述	:  解锁
	* 输入		:  bFlag 锁是否已经创建
	* 输出		:  无
	* 返回值	:  0 成功，-1 失败
	* 作者		:  li.shugang
	*******************************************************************************/
    int UnLock(bool bFlag, int iPID, int iTID);
    /******************************************************************************
	* 函数名称	:  GetErrMsg()
	* 函数描述	:  获取错误信息   
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  错误信息
	* 作者		:  li.shugang
	*******************************************************************************/
    const char* GetErrMsg(int iErrno);
    /******************************************************************************
	* 函数名称	:  GetErrMsg()
	* 函数描述	:  获取错误信息   
	* 输入		:  无
	* 输出		:  无
	* 返回值	:  错误信息
	* 作者		:  li.shugang
	*******************************************************************************/
	static int GetErrorCode();
	/******************************************************************************
	* 函数名称	:  GetErrMsgByCode()
	* 函数描述	:  通过错误码获取错误信息   
	* 输入		:  iErrCode 错误码
	* 输出		:  无
	* 返回值	:  错误信息
	* 作者		:  li.shugang
	*******************************************************************************/
    static const char* GetErrMsgByCode(int iErrCode,int iDetailCode=0);
       	
private:
#ifdef WIN32
	static int iErrCode;                                //系统错误编码
	HANDLE mutex;
#else
	pthread_mutex_t     mutex;
	pthread_mutexattr_t mattr;
	pthread_cond_t	    cond;

#endif
	bool bIsCreate;
	bool bIsLock;
	int  m_iPID;
	int  m_iTID;
	int  m_iCounts;
};

//}

#endif //__MEMORY_DATABASE_MUTEX_H_

