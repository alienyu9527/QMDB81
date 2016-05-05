/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbCacheLog.h		
*@Description： 缓存式输出日志文件，日志信息先写入缓存，然后定时或定量输出
*@Author:		jin.shaohua
*@Date：	    2012.11
*@History:
******************************************************************************************/
#ifndef _T_CACHE_LOG_H_
#define _T_CACHE_LOG_H_
#include <time.h>
#include "Helper/TThreadLog.h"
#include "Helper/mdbStruct.h"


//namespace QuickMDB{

#define MAX_CACHE_LOG_SIZE 1024*1024*10   //最大缓存2M
//刷出周期类型
enum E_FLUSH_CYCLE_TYPE
{
	FLUSH_CYCLE_TIME     = 1,//根据时间,单位:秒
	FLUSH_CYCLE_SIZE      = 2,//大小，单位:KB
	FLUSH_CYCLE_COUNT  = 3 //次数
};

//缓存式日志组件
class TCacheLog
{
public:
	TCacheLog();
	~TCacheLog();
	int SetLogFile(const char * sFullPathName,int iLogFileSize);
	int SetFlushCycle(int iCycleType,int iCycleValue);//设置输出文件周期
	int Log(const char * sInfo);
	int FlushImmediately();//立刻刷出
private:
	int AllocCache(int iSize);//申请缓存
	int CacheToFile();//缓存输出到文件
	bool bNeedToFlush();//是否需要刷出
	int CheckAndBackup(const char * sFileName , FILE*& fp,int iMaxSizeM);
private:
	int m_iCycleType;//刷出周期类型
	int m_iCycleValue;//刷出周期值
	char * m_pCache;//指向缓存空间
	char    m_sLogFile[MAX_PATH_NAME_LEN];//日志文件
	int      m_iCacheSize;//缓存大小
	int      m_iCurCachePos;//当前缓存位置
	int      m_iLogFileSizeM;//日志文件大小
private:
	int 	   m_iLogCount;//日志次数
	time_t m_tNextLogTime;//下次日志输出的时间
};

//}
#endif 

