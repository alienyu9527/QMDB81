/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbDateTime.cpp		
*@Description�� ʱ�䴦����
*@Author:		jin.shaohua
*@Date��	    2014/03/05
*@History:
******************************************************************************************/
#include "Helper/mdbDateTime.h"
#include "Helper/mdbStruct.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{


	/******************************************************************************
	* ��������	:  ���ַ���YYYYMMDDHHMMSSʱ�����ʽת��Ϊϵͳ��time_t��
	* ��������	: 
	* ����		:  
	* ���		:  
	* ����ֵ	:  ����ʱ���-�ɹ�  (time) -1 --ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	time_t TMdbDateTime::StringToTime(const char sTime[],int iTimeDifferent)
	{
#if 1
		if(iTimeDifferent == MDB_TIME_DIFF_XLS)
		{
			return   TMdbNtcShortDateTime::MakeTime(sTime);
		}
		else
		{
			time_t tGmt = TMdbNtcGmtShortDateTime::MakeTime(sTime);
			return (tGmt==(time_t)-1)?StringToTime(sTime): tGmt + iTimeDifferent;
		}
#endif
		return 0;
	}
	/******************************************************************************
	* ��������	:  TimeToString
	* ��������	: time to string
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	void TMdbDateTime::TimeToString(const time_t tTime,char sCurtime[])
	{
		memcpy(sCurtime,TMdbNtcShortDateTime::Format(tTime).c_str(),MAX_TIME_LEN);
	}
	/******************************************************************************
	* ��������	:  GetCurrentTimeStr
	* ��������	: �õ���ǰʱ���YYYYMMDDHHMMSS�ַ�����ʽ
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	void TMdbDateTime::GetCurrentTimeStr(char sTime[])
	{
		memcpy(sTime,TMdbNtcShortDateTime::Format(TMdbNtcShortDateTime::GetCurTime()).c_str(),MAX_TIME_LEN);
	}
	/******************************************************************************
	* ��������	:   Sleep
	* ��������	: ˯�ߺ���
	* ����		:  
	* ���		:  
	* ����ֵ	:  
	* ����		:  jin.shaohua
	*******************************************************************************/
	void TMdbDateTime::MSleep(const int iMicroSeconds)
	{
		TMdbNtcDateTime::Sleep(iMicroSeconds);
	}

        void TMdbDateTime::Sleep(const int iSeconds)
        {
            #ifdef _WIN32
            ::Sleep(iSeconds*1000);
            #else
            ::sleep(iSeconds);
            #endif
        }
        
	/******************************************************************************
	* ��������	:   GetTimeDifferent
	* ��������	: ��ȡʱ��
	* ����		:  
	* ���		:  
	* ����ֵ	:  �����û��ʹ������ʱ�Ļ���������time_t-GMTʱ�䣬���ʹ��������ʱ����ʹ��time_tʱ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	int TMdbDateTime::GetTimeDifferent()
	{
#if 1
		if(IsUseXLS())
		{
			return MDB_TIME_DIFF_XLS;
		}
		else
		{
			time_t tGMT = TMdbNtcGmtShortDateTime::MakeTime("20131126010000");
			time_t tLocal = TMdbNtcShortDateTime::MakeTime("20131126010000");
			return (int)(tLocal - tGMT);
		}
#endif
		return 0;
	}
	/******************************************************************************
	* ��������	:  IsUseXLS
	* ��������	: �Ƿ�ʹ��������ʱ
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	bool TMdbDateTime::IsUseXLS()
	{
#if 1
		bool bRet = false;
		//����12���µ����ڼ���
		int iTimeDiff = (int)(TMdbNtcShortDateTime::MakeTime("20131126010000") - TMdbNtcGmtShortDateTime::MakeTime("20131126010000"));
		char sTestTime[15] = {0};
		char sOutTime[15] = {0};
		int i = 0;
		for(i = 0;i < 12;++i)
		{
			snprintf(sTestTime,sizeof(sTestTime),"2014%02d01000000",i+1);
			time_t iTime = StringToTime(sTestTime,iTimeDiff);
			TimeToString(iTime,sOutTime);
			if(strcmp(sTestTime,sOutTime) != 0)
			{//��������ʱ
				bRet = true;
				break;
			}
		}
		//����ԭ��ʱ�亯��
		for(i = 0;i < 12;++i)
		{
			snprintf(sTestTime,sizeof(sTestTime),"2014%02d01000000",i+1);
			time_t iTime = StringToTime(sTestTime,MDB_TIME_DIFF_XLS);
			TimeToString(iTime,sOutTime);
			if(strcmp(sTestTime,sOutTime) != 0)
			{
				printf("date time convert function is error.....\n");
				break;
			}
		}
		return bRet;
#endif
		return false;
	}

	/******************************************************************************
	* ��������	:  AddDay
	* ��������	: ����ʱ��
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	void TMdbDateTime::AddDay(char sDate[],int iDays)
	{
		memcpy(sDate,TMdbNtcShortDateTime::Format(TMdbNtcDateTime::AddDays(TMdbNtcShortDateTime::MakeTime(sDate),iDays)).c_str(),MAX_TIME_LEN) ;
	}

	/******************************************************************************
	* ��������	:  AddSeconds
	* ��������	: ����ʱ��
	* ����		:  
	* ���		:  
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	* ����		:  jin.shaohua
	*******************************************************************************/
	void TMdbDateTime::AddSeconds(char sDate[],int iSeconds)
	{
		memcpy(sDate,TMdbNtcShortDateTime::Format(TMdbNtcDateTime::AddSeconds(TMdbNtcShortDateTime::MakeTime(sDate),iSeconds)).c_str(),MAX_TIME_LEN) ;
	}

        int TMdbDateTime::GetDiffSeconds(const char sTime1[], const char sTime2[])
        {
            return TMdbNtcShortDateTime::GetDiffSeconds(sTime1, sTime2);
        }

        bool TMdbDateTime::BetweenDate(const char *dInDate,const char *dBeginDate,const char *dEndDate)
        {
            return TMdbNtcLocalDateTime::BetweenDate(dInDate, dBeginDate, dEndDate);
        }

        void TMdbDateTime::AddYears(char sDate[],int iYears)
        {
            memcpy(sDate,TMdbNtcShortDateTime::Format(TMdbNtcDateTime::AddYears(TMdbNtcShortDateTime::MakeTime(sDate),iYears)).c_str(),MAX_TIME_LEN) ;
        }

        void TMdbDateTime::AddMonths(char sDate[],int iMonths)
        {
            memcpy(sDate,TMdbNtcShortDateTime::Format(TMdbNtcDateTime::AddMonths(TMdbNtcShortDateTime::MakeTime(sDate),iMonths)).c_str(),MAX_TIME_LEN) ;
        }

        void TMdbDateTime::AddHours(char sDate[],int iHours)
        {
            memcpy(sDate,TMdbNtcShortDateTime::Format(TMdbNtcDateTime::AddHours(TMdbNtcShortDateTime::MakeTime(sDate),iHours)).c_str(),MAX_TIME_LEN) ;
        }

        void TMdbDateTime::AddMins(char sDate[],int iMins)
        {
            memcpy(sDate,TMdbNtcShortDateTime::Format(TMdbNtcDateTime::AddMinutes(TMdbNtcShortDateTime::MakeTime(sDate),iMins)).c_str(),MAX_TIME_LEN) ;
        }

        int TMdbDateTime::GetDiffMSecond(const char* pszTime, const char* pszOldTime)
        {
            char sTime[64], sOldTime[64];
            memset(sTime, 0, sizeof(sTime));
            memset(sOldTime, 0, sizeof(sOldTime));

            strncpy(sTime, pszTime, 14);
            strncpy(sOldTime, pszOldTime, 14);
            int iMSecond = GetDiffSeconds(sTime, sOldTime)*1000 + (atoi(&pszTime[14])-atoi(&pszOldTime[14]));
            return iMSecond;
        }

            void TMdbDateTime::GetNowDateTime(char *sDateTime,int iDateTimeFlag)
            {
                time_t timeNow;
                struct tm *ptmNow;

                time(&timeNow);
                ptmNow=localtime(&timeNow);

                if(iDateTimeFlag==0)
                {
                    /*����ʱ��*/
                    sprintf(sDateTime,"%04d%02d%02d%02d%02d%02d",ptmNow->tm_year+1900,ptmNow->tm_mon+1,ptmNow->tm_mday,
                    ptmNow->tm_hour,ptmNow->tm_min,ptmNow->tm_sec);
                }
                else if(iDateTimeFlag==1)
                {
                    /*����*/
                    sprintf(sDateTime,"%04d%02d%02d",ptmNow->tm_year+1900,ptmNow->tm_mon+1,ptmNow->tm_mday);
                }
                else if(iDateTimeFlag==2)
                {
                    /*ʱ��*/
                    sprintf(sDateTime,"%02d%02d%02d",ptmNow->tm_hour,ptmNow->tm_min,ptmNow->tm_sec);
                }
            }

            long TMdbDateTime::GetDiffDay(const char sTime1[], const char sTime2[])
            {
                return TMdbNtcShortDateTime::GetDiffDays(sTime1, sTime2);
            }

            int TMdbDateTime::GetDiffHour(const char* psTime1, const char* psTime2)
            {
                return TMdbNtcShortDateTime::GetDiffHours(psTime1, psTime2);
            }

            int TMdbDateTime::GetDiffHour(time_t time1, time_t time2)
            {
                return TMdbNtcShortDateTime::GetDiffHours(time1, time2);
            }

            int  TMdbDateTime::TimevalToTimeStr( struct timeval &stTimeVal,char sTime[],bool bAddMS)
            {
                TimeToString(stTimeVal.tv_sec,sTime);
                if(bAddMS);
                {
                    sprintf(sTime+14,"%03d", (int)(stTimeVal.tv_usec/1000));
                }
                return 0;
            }
//}