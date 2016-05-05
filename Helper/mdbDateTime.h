#ifndef _MDB_DATE_TIME_H_
#define _MDB_DATE_TIME_H_

/****************************************************************************************
*@Copyrights  2014，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbDateTime.h		
*@Description： 时间处理类
*@Author:		jin.shaohua
*@Date：	    2014/03/05
*@History:
******************************************************************************************/

//#include "BillingSDK.h"
#include <sys/time.h>
#define MDB_TIME_DIFF_XLS (-99) //使用夏令时下的时差判断

//namespace QuickMDB{
    class TMdbDateTime
    {
    public:
        static time_t StringToTime(const char sTime[],int iTimeDifferent = MDB_TIME_DIFF_XLS);//将字符串YYYYMMDDHHMMSS时间的形式转换为系统的time_t型
        static void TimeToString(const time_t tTime,char sCurtime[]); //time to string
        static void GetCurrentTimeStr(char sTime[]); // 得到当前时间的YYYYMMDDHHMMSS字符串形式
        static void Sleep(const int iSeconds);// 睡眠函数（封装了操作系统的差异）
        static void MSleep(const int iMicroSeconds); // 睡眠函数（封装了操作系统的差异）
        static int GetTimeDifferent();//获取时差
        static void AddDay(char sDate[],int iDays);//增减时间
        static void AddSeconds(char sDate[],int iSeconds);//增减时间
        static int GetDiffSeconds(const char sTime1[], const char sTime2[]);
        static bool BetweenDate(const char *dInDate,const char *dBeginDate,const char *dEndDate);

        static void AddYears(char sDate[],int iYears);
        static void AddMonths(char sDate[],int iMonths);
        static void AddHours(char sDate[],int iHours);
        static void AddMins(char sDate[],int iMins);

         static int GetDiffMSecond(const char* pszTime, const char* pszOldTime);
         static int GetDiffHour(time_t time1, time_t time2);
         static int GetDiffHour(const char* psTime1, const char* psTime2);
         static void GetNowDateTime(char *sDateTime,int iDateTimeFlag);
         static long GetDiffDay(const char sTime1[], const char sTime2[]);
         static int  TimevalToTimeStr( struct timeval &stTimeVal,char sTime[],bool bAddMS);
    private:
        static bool IsUseXLS();//是否使用了夏令时
    };
//}

#endif
