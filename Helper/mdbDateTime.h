#ifndef _MDB_DATE_TIME_H_
#define _MDB_DATE_TIME_H_

/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbDateTime.h		
*@Description�� ʱ�䴦����
*@Author:		jin.shaohua
*@Date��	    2014/03/05
*@History:
******************************************************************************************/

//#include "BillingSDK.h"
#include <sys/time.h>
#define MDB_TIME_DIFF_XLS (-99) //ʹ������ʱ�µ�ʱ���ж�

//namespace QuickMDB{
    class TMdbDateTime
    {
    public:
        static time_t StringToTime(const char sTime[],int iTimeDifferent = MDB_TIME_DIFF_XLS);//���ַ���YYYYMMDDHHMMSSʱ�����ʽת��Ϊϵͳ��time_t��
        static void TimeToString(const time_t tTime,char sCurtime[]); //time to string
        static void GetCurrentTimeStr(char sTime[]); // �õ���ǰʱ���YYYYMMDDHHMMSS�ַ�����ʽ
        static void Sleep(const int iSeconds);// ˯�ߺ�������װ�˲���ϵͳ�Ĳ��죩
        static void MSleep(const int iMicroSeconds); // ˯�ߺ�������װ�˲���ϵͳ�Ĳ��죩
        static int GetTimeDifferent();//��ȡʱ��
        static void AddDay(char sDate[],int iDays);//����ʱ��
        static void AddSeconds(char sDate[],int iSeconds);//����ʱ��
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
        static bool IsUseXLS();//�Ƿ�ʹ��������ʱ
    };
//}

#endif
