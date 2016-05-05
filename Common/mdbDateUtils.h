/**
 * @file DateUtils.hxx
 * @brief ����ʱ����
 *
 * ����ʱ����
 *
 * @author Ge.zhengyi, Jiang.jinzhou, Du.jiagen, Zhang.he
 * @version 1.0
 * @date 20121214
 * @warning
 */
#ifndef _MDB_H_DateUtils_
#define _MDB_H_DateUtils_
#include "Common/mdbCommons.h"
#include "Common/mdbStrings.h"
#include <assert.h>
//namespace QuickMDB
//{
//    namespace BillingSDK
//    {
        /**
         * @brief ʱ��ӿڻ���
         * 
         */
        template<class _Ty>
        class TMdbNtcDateTimeBase
        {
            /** \example  example_TDateTime.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
        private:
            enum
            {
                YEAR_LENGTH     =   4,
                MONTH_LENGTH    =   2,
                DAY_LENGTH      =   2,
                HOUR_LENGTH     =   2,
                MINUTE_LENGTH   =   2,
                SECOND_LENGTH   =   2
            };
            TMdbNtcDateTimeBase(){}
        public:
            /**
             * @brief ��õ�ǰ��ʱ���
             * 
             * @return time_t
             * @retval ʱ���
             */
            inline static time_t GetCurTime()
            {
                return time(NULL);
            }
            /**
             * @brief ����ָ����ʽ��ʽ��ʱ���ַ���
             * 
             * @param pszPattern [in] ʱ���ʽ, YYYY(��),MM(��),DD(��),HH(ʱ),MI(��),SS(��)
             * @param tTime      [in] ʱ��,-1��ʾȡ��ǰʱ��(�ɾ���TΪ����ʱ�仹��GMTʱ�����)
             * @return TMdbNtcStringBuffer
             * @retval ��ʽ����Ĵ�
             */
            static TMdbNtcStringBuffer Format(const char* pszPattern, time_t tTime = (time_t)-1);
            /**
             * @brief ����ָ����ʽ��ʽ��ʱ���ַ���
             * 
             * @param pszPattern [in] ʱ���ʽ, YYYY(��),MM(��),DD(��),HH(ʱ),MI(��),SS(��)
             * @param ptm        [in] ʱ��,NULL��ʾȡ��ǰʱ��(�ɾ���TΪ����ʱ�仹��GMTʱ�����)
             * @return TMdbNtcStringBuffer
             * @retval ��ʽ����Ĵ�
             */
            static TMdbNtcStringBuffer Format(const char* pszPattern, struct tm *ptm);
            /**
             * @brief ˯�ߺ�������װ�˲���ϵͳ�Ĳ��죩
             * 
             * @param iMilliSeconds [in] �������ߵ�ʱ�������룩
             */
            static void Sleep(int iMilliSeconds);
            /**
             * @brief ���ַ���ת��Ϊʱ���
             * 
             * @param pszPattern [in] ʱ���ʽ
             * @param pszTime    [in] ʱ���ַ���
             * @return time_t
             * @retval -1 ʧ��
             */
            static time_t MakeTime(const char* pszPattern, const char* pszTime);
            /**
             * @brief ����ָ��������ʱ����ת��Ϊʱ���
             * 
             * @param iYear   [in] ��
             * @param iMonth  [in] ��
             * @param iDay    [in] ��
             * @param iHour   [in] ʱ
             * @param iMinute [in] ��
             * @param iSecond [in] ��
             * @return time_t
             * @retval -1 ʧ��
             */
            static time_t MakeTime(int iYear, int iMonth, int iDay, int iHour, int iMinute, int iSecond);
            /**
             * @brief ����tm�ṹת����ʱ���
             * 
             * @param ptmValue [in] struct tm 
             * @return time_t
             * @retval -1 ʧ��
             */
            static time_t MakeTime(tm *ptmValue);
            /**
             * @brief ��������ʱ������Ŀ������������ͬһ���ڣ������0
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int     
             */
            static int GetDiffYears(time_t tTime1, time_t tTime2);
            /**
             * @brief ��������ʱ������Ŀ������������ͬһ���ڣ������0
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int
             */
            static int GetDiffMonths(time_t tTime1, time_t tTime2);
            /**
             * @brief ��������ʱ������Ŀ������������ͬһ���ڣ������0
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int     
             */
            static int GetDiffDays(time_t tTime1, time_t tTime2);
            /**
             * @brief ��������ʱ������Ŀ�Сʱ���������ͬһСʱ�ڣ������0
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int
             */
            static int GetDiffHours(time_t tTime1, time_t tTime2);
            /**
             * @brief ��������ʱ������Ŀ�������������ͬһ�����ڣ������0
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int
             */
            static int GetDiffMinutes(time_t tTime1, time_t tTime2);
            /**
             * @brief ��������ʱ�����������
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int
             */
            inline static int GetDiffSeconds(time_t tTime1, time_t tTime2)
            {
                return (int)difftime(tTime1, tTime2);
            }
            /**
             * @brief ��������ʱ������������ʱ�����������365��
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int
             */
            static int GetDistanceYears(time_t tTime1, time_t tTime2);
            /**
             * @brief ��������ʱ������������ʱ�����������30��
             * 
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int
             */
            static int GetDistanceMonths(time_t tTime1, time_t tTime2);
            /**
             * @brief ��������ʱ������������ʱ�����������86400
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int     
             */
            static int GetDistanceDays(time_t tTime1, time_t tTime2);
            /**
             * @brief ��������ʱ������Сʱ����ʱ�����������3600
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int
             */
            static int GetDistanceHours(time_t tTime1, time_t tTime2);
            /**
             * @brief ��������ʱ�����ķ�������ʱ�����������60
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int
             */
            static int GetDistanceMinutes(time_t tTime1, time_t tTime2);
            /**
             * @brief ��������ʱ�����������
             * 
             * @param tTime1 [in] ʱ��1
             * @param tTime2 [in] ʱ��2
             * @return int
             */
            inline static int GetDistanceSeconds(time_t tTime1, time_t tTime2)
            {
                return difftime(tTime1, tTime2);
            }
            /**
             * @brief ��ȡʱ���ǽ���ĵڶ�����
             * 
             * @param tTime [in] ʱ��
             * @return int
             * @retval days since January 1 - [0,365]
             */
            static int GetYearDay(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡʱ��������µĵڶ����죬ͬGetDay
             * 
             * @param tTime [in] ʱ��
             * @return int
             * @retval day of the month - [1,31]
             */
            static int GetMonthDay(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡʱ�������ڼ�
             * 
             * @param tTime [in] ʱ��
             * @return int
             * @retval days since Sunday - [0,6]
             */
            static int GetWeekDay(time_t tTime = (time_t)-1);
            /**
             * @brief ����ʱ�������
             * 
             * @param tTime [in] ʱ��
             * @return int
             * @retval [1970, 2036]
             */
            static int GetYear(time_t tTime = (time_t)-1);
            /**
             * @brief ����ʱ�����·�
             * 
             * @param tTime [in] ʱ��
             * @return int
             * @retval [1,12]
             */
            static int GetMonth(time_t tTime = (time_t)-1);
            /**
             * @brief ����ʱ������
             * 
             * @param tTime [in] ʱ��
             * @return int
             * @retval [1,31]
             */
            static int GetDay(time_t tTime = (time_t)-1);
            /**
             * @brief ����ʱ����Сʱ
             * 
             * @param tTime [in] ʱ��
             * @return int
             * @retval [0,23]
             */
            static int GetHour(time_t tTime = (time_t)-1);
            /**
             * @brief ����ʱ���÷���
             * 
             * @param tTime [in] ʱ��
             * @return int
             * @retval [0,59]
             */
            static int GetMinute(time_t tTime = (time_t)-1);
            /**
             * @brief ����ʱ������
             * 
             * @param tTime [in] ʱ��
             * @return int
             * @retval [0,59]
             */
            static int GetSecond(time_t tTime = (time_t)-1);
            
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�꿪ʼʱ��
             * 
             * @param tTime [in] ָ����ʱ���
             * @return time_t
             * @retval ��ǰһ�꿪ʼʱ��
             */
            static time_t GetYearBegin(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�����ʱ��
             * 
             * @param tTime [in] ָ����ʱ���
             * @return time_t
             * @retval ��ǰһ�����ʱ��
             */
            static time_t GetYearEnd(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�¿�ʼʱ��
             * 
             * @param tTime [in] ָ����ʱ���
             * @return time_t
             * @retval ��ǰһ�¿�ʼʱ��
             */
            static time_t GetMonthBegin(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�����ʱ��
             * 
             * @param tTime [in] ָ����ʱ���
             * @return time_t
             * @retval ��ǰһ�����ʱ��
             */
            static time_t GetMonthEnd(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�쿪ʼʱ��
             * 
             * @param tTime [in] ָ����ʱ���
             * @return time_t
             * @retval ��ǰһ�쿪ʼʱ��
             */
            static time_t GetDayBegin(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�����ʱ��
             * 
             * @param tTime [in] ָ����ʱ���
             * @return time_t
             * @retval ��ǰһ�����ʱ��
             */
            static time_t GetDayEnd(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰСʱ��ʼʱ��
             * 
             * @param tTime [in] ָ����ʱ���
             * @return time_t
             * @retval ��ǰСʱ��ʼʱ��
             */
            static time_t GetHourBegin(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰСʱ���ʱ��
             * 
             * @param tTime [in] ָ����ʱ���
             * @return time_t
             * @retval ��ǰСʱ���ʱ��
             */
            static time_t GetHourEnd(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰ���ӿ�ʼʱ��
             * 
             * @param tTime [in] ָ����ʱ���
             * @return time_t
             * @retval ��ǰ���ӿ�ʼʱ��
             */
            static time_t GetMinuteBegin(time_t tTime = (time_t)-1);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰ�������ʱ��
             * 
             * @param tTime [in] ָ����ʱ���
             * @return time_t
             * @retval ��ǰ�������ʱ��
             */
            static time_t GetMinuteEnd(time_t tTime = (time_t)-1);
            /**
             * @brief �ж�����������Ƿ��ڸ����ķ�Χ֮��(���ڵ���begin��С��end),��Χ��ʾΪ[begin,end)
             * 
             * @param pszInDate    [in] ����������
             * @param pszBeginDate [in] ��Χ�Ŀ�ʼ����,���ΪNULL�����ʾֻ�ж�end
             * @param pszEndDate   [in] ��Χ�Ľ�������,���ΪNULL�����ʾֻ�ж�begin
             * @return bool
             * @retval true λ��֮��
             */
            static bool BetweenDate(const char *pszInDate,const char *pszBeginDate,const char *pszEndDate);
            /**
             * @brief �ж�����������Ƿ��ڸ����ķ�Χ֮��(���ڵ���begin��С��end),��Χ��ʾΪ[begin,end)
             * 
             * @param ttInDate    [in] ����������
             * @param ttBeginDate [in] ��Χ�Ŀ�ʼ����,���Ϊ-1�����ʾֻ�ж�end
             * @param ttEndDate   [in] ��Χ�Ľ�������,���Ϊ-1�����ʾֻ�ж�begin
             * @return bool
             * @retval true λ��֮��
             */
            static bool BetweenDate(time_t ttInDate, time_t ttBeginDate, time_t ttEndDate);
            /**
             * @brief ����ʱ�����ȡTM�ṹʱ��
             * @param ptm   [out] ��ȡ��TM�ṹ��ʱ�� 
             * @param tTime [in]  ʱ���
             * @return struct tm*
             * @retval  ��NULL, �ɹ�
             */
            static struct tm* GetTM(struct tm* ptm, time_t tTime = (time_t)-1);
            /**
             * @brief ��ԭʱ������ϵ������ڣ������գ�
             * 
             * @param tTime   [in] ԭʱ��
             * @param iYears  [in] ����
             * @param iMonths [in] ����
             * @param iDays   [in] ����
             * @return time_t
             * @retval ���Ӻ��ʱ��
             */
            static time_t AddDate(time_t tTime, int iYears = 0, int iMonths = 0, int iDays = 0);
            /**
             * @brief ��ԭʱ������ϵ���ʱ�䣨ʱ���룩
             * 
             * @param tTime    [in] ԭʱ��
             * @param iHours   [in] Сʱ��
             * @param iMinutes [in] ������
             * @param iSeconds [in] ����
             * @return time_t
             * @retval ���Ӻ��ʱ��
             */
            static time_t AddTime(time_t tTime, int iHours = 0, int iMinutes = 0, int iSeconds = 0);
            /**
             * @brief ��ԭʱ������ϵ�������ʱ�䣨������ʱ���룩
             * 
             * @param tTime    [in] ԭʱ��
             * @param iYears   [in] ����
             * @param iMonths  [in] ����
             * @param iDays    [in] ����
             * @param iHours   [in] Сʱ��
             * @param iMinutes [in] ������
             * @param iSeconds [in] ����
             * @return time_t
             * @retval ���Ӻ��ʱ��
             */
            static time_t AddDateTime(time_t tTime, int iYears = 0, int iMonths = 0, int iDays = 0,
                int iHours = 0, int iMinutes = 0, int iSeconds = 0);
            /**
             * @brief ��ԭʱ������ϵ������
             * 
             * @param tTime  [in] ԭʱ��
             * @param iYears [in] ����
             * @return time_t
             * @retval ���Ӻ��ʱ��
             */
            inline static time_t AddYears(time_t tTime, int iYears = 1)
            {
                return _Ty::AddDate(tTime, iYears, 0, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ�������
             * 
             * @param tTime   [in] ԭʱ��
             * @param iMonths [in] ����
             * @return time_t
             * @retval ���Ӻ��ʱ��
             */
            inline static time_t AddMonths(time_t tTime, int iMonths = 1)
            {
                return _Ty::AddDate(tTime, 0, iMonths, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ�������
             * 
             * @param tTime [in] ԭʱ��
             * @param iDays [in] ����
             * @return time_t
             * @retval ���Ӻ��ʱ��
             */
            inline static time_t AddDays(time_t tTime, int iDays = 1)
            {
                return _Ty::AddDate(tTime, 0, 0, iDays);
            }
            /**
             * @brief ��ԭʱ������ϵ���Сʱ��
             * 
             * @param tTime  [in] ԭʱ��
             * @param iHours [in] Сʱ��
             * @return time_t
             * @retval ���Ӻ��ʱ��
             */
            inline static time_t AddHours(time_t tTime, int iHours = 1)
            {
                return _Ty::AddTime(tTime, iHours, 0, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ��ӷ�����
             * 
             * @param tTime    [in] ԭʱ��
             * @param iMinutes [in] ������
             * @return time_t
             * @retval ���Ӻ��ʱ��
             */
            inline static time_t AddMinutes(time_t tTime, int iMinutes = 1)
            {
                return _Ty::AddTime(tTime, 0, iMinutes, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ�������
             * 
             * @param tTime    [in] ԭʱ��
             * @param iSeconds [in] ����
             * @return time_t
             * @retval ���Ӻ��ʱ��
             */
            inline static time_t AddSeconds(time_t tTime, int iSeconds = 1)
            {
                return _Ty::AddTime(tTime, 0, 0, iSeconds);
            }
            /**
             * @brief ��������ж��Ƿ�Ϊ����
             * 
             * @param iYear [in] ���
             * @return bool
             */
            inline static bool IsLeapYear(int iYear)
            {
                return (iYear %4 == 0) && ( iYear%100 != 0 || iYear%400 ==0);
            }
        };

        /**
         * @brief Localʱ��(����ʱ)�ӿ���, ���ʱ������
         * 
         */
        class TMdbNtcLocalDateTime:public TMdbNtcDateTimeBase<TMdbNtcLocalDateTime>
        {
            /** \example  example_TDateTime.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
        public:
            using TMdbNtcDateTimeBase<TMdbNtcLocalDateTime>::MakeTime;
            /**
             * @brief ����tm�ṹת����ʱ���
             * 
             * @param ptmValue [in] struct tm 
             * @return time_t
             * @retval -1 ʧ��
             */
            static time_t MakeTime(tm *ptmValue);

            /**
             * @brief ����ʱ�����ȡTM�ṹʱ��
             * @param ptm   [out] ��ȡ��TM�ṹ��ʱ�� 
             * @param tTime [in] ʱ���
             * @return struct tm*
             * @retval  ��NULL���ɹ�
             */
            static struct tm* GetTM(struct tm* ptm, time_t tTime = (time_t)-1);
        };

        /**
         * @brief GMTʱ��(�������α�׼ʱ)�ӿ���
         * 
         */
        class TMdbNtcGmtDateTime:public TMdbNtcDateTimeBase<TMdbNtcGmtDateTime>
        {
            /** \example  example_TDateTime.cpp
             * This is an example of how to use the TMdbNtcBaseList class.
             * More details about this example.
             */
        public:
            using TMdbNtcDateTimeBase<TMdbNtcGmtDateTime>::MakeTime;
            /**
             * @brief ����tm�ṹת����ʱ���
             * 
             * @param ptmValue [in] struct tm 
             * @return time_t
             * @retval -1 ʧ��
             */
            static time_t MakeTime(tm *ptmValue);
            /**
             * @brief ����ʱ�����ȡTM�ṹʱ��
             * @param ptm   [out] ��ȡ��TM�ṹ��ʱ�� 
             * @param tTime [in]  ʱ���
             * @return struct tm*
             * @retval  ��NULL���ɹ�
             */
            static struct tm* GetTM(struct tm* ptm, time_t tTime = (time_t)-1);
        private:
            static int  m_iTimeZone;///< Ĭ��ֵ����ʱ�����ֵ����ʾδ��ȡʱ��
        private:
              /**
             * @brief ������aArr�еõ���iYear(iYear>=1970)��1970�ж��ٸ��������iYear(iYear<1970)��0���ж��ٸ�����
             * @param iSize [in]  �����С
             * @param iYear [in]  Ҫ���ҵ����
             * @param bFound [out] �Ƿ��ҵ�iYear,ֱ�ӱ��������ǲ�������
             * @retval  >=0��iYear>=1970,retval��ֵ�Ϸ�ӳiYear��1970�ж�������;(iYear<1970),��ʾ��0���ж�������
             */
            static MDB_INT32 GetLeapYearNum(const int* aArr,  int iSize, int iYear, bool& bFound);
            
        };

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcDateTimeBase<_Ty>::Format(const char* pszPattern, time_t tTime /* = -1*/ )
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }
            
            struct tm tmValue = {0};
            tmValue.tm_isdst = -1;
            if (_Ty::GetTM(&tmValue, tTime))
            {
                tmValue.tm_isdst = -1;
                
                return Format(pszPattern, &tmValue);
            }
            
            return "";
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcDateTimeBase<_Ty>::Format(const char* pszPattern, struct tm *ptm)
        {
            if (NULL == ptm)
            {
                return Format(pszPattern, time(NULL));
            }
            
            TMdbNtcStringBuffer strData(pszPattern);
                
            char szTmp[5] = {0};
            
            int iPos = strData.Find("YYYY");
            if (iPos >= 0)
            {
                MdbNtcSnprintf(szTmp, sizeof(szTmp), "%04d", ptm->tm_year + 1900);
                strData.Replace(iPos, YEAR_LENGTH, szTmp);
            }
            
            iPos = strData.Find("MM");
            if (iPos >= 0)
            {
                MdbNtcSnprintf(szTmp, sizeof(szTmp), "%02d", ptm->tm_mon + 1);
                strData.Replace(iPos, MONTH_LENGTH, szTmp);
            }
            
            
            iPos = strData.Find("DD");
            if (iPos >= 0)
            {
                MdbNtcSnprintf(szTmp, sizeof(szTmp), "%02d", ptm->tm_mday);
                strData.Replace(iPos, DAY_LENGTH, szTmp);
            }

            iPos = strData.Find("HH");
            if (iPos >= 0)
            {
                MdbNtcSnprintf(szTmp, sizeof(szTmp), "%02d", ptm->tm_hour);
                strData.Replace(iPos, HOUR_LENGTH, szTmp);
             }

            iPos = strData.Find("MI");
            if (iPos >= 0)
            {
                MdbNtcSnprintf(szTmp, sizeof(szTmp), "%02d", ptm->tm_min);
                strData.Replace(iPos, MINUTE_LENGTH, szTmp);
            }

            iPos = strData.Find("SS");
            if (iPos >= 0)
            {
                MdbNtcSnprintf(szTmp, sizeof(szTmp), "%02d", ptm->tm_sec);
                strData.Replace(iPos, SECOND_LENGTH, szTmp);
            }

            return strData;
        }

        template<class _Ty>
        void TMdbNtcDateTimeBase<_Ty>::Sleep(int iMilliSeconds)
        {
        #ifdef  OS_WINDOWS
            ::Sleep(iMilliSeconds);
        #else
            struct timeval timeout;
            if (iMilliSeconds >= 1000)
            {
                timeout.tv_sec  = iMilliSeconds/1000;
                timeout.tv_usec = (iMilliSeconds%1000)*1000;
            }
            else
            {
                timeout.tv_sec  = 0;
                timeout.tv_usec = iMilliSeconds * 1000;                
            }
            select(0,NULL,NULL,NULL,&timeout);
        #endif
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::MakeTime(const char* pszPattern, const char* pszTime)
        {
            if (NULL == pszPattern || NULL == pszTime)
            {
                return (time_t)-1;
            }

            if (strlen(pszPattern) != strlen(pszTime))
            {
                return (time_t)-1;
            }
            
            char cYear[5] = {0};
            char cMon[3] = "01";
            char cDay[3] = "01";
            char cHour[3] = "00";
            char cMin[3] = "00";
            char cSec[3] = "00";
            
            const char *pValue = strstr(pszPattern, "YYYY");
            if (NULL == pValue)
            {
                return (time_t)-1;
            }
            int iPos = (int)(pValue - pszPattern);
            memcpy(cYear, pszTime+iPos, YEAR_LENGTH);
            
            pValue = strstr(pszPattern, "MM");
            if (pValue)
            {
                iPos = (int)(pValue - pszPattern);
                memcpy(cMon, pszTime+iPos, MONTH_LENGTH);
            }
            
            pValue = strstr(pszPattern, "DD");
            if (pValue)
            {
                iPos = (int)(pValue - pszPattern);
                memcpy(cDay, pszTime+iPos, DAY_LENGTH);
            }

            pValue = strstr(pszPattern, "HH");
            if (pValue)
            {
                iPos = (int)(pValue - pszPattern);
                memcpy(cHour, pszTime+iPos, HOUR_LENGTH);
            }

            pValue = strstr(pszPattern, "MI");
            if (pValue)
            {
                iPos = (int)(pValue - pszPattern);
                memcpy(cMin, pszTime+iPos, MINUTE_LENGTH);
            }

            pValue = strstr(pszPattern, "SS");
            if (pValue)
            {
                iPos = (int)(pValue - pszPattern);
                memcpy(cSec, pszTime+iPos, SECOND_LENGTH);
            }            
            
            
            struct tm tmValue={0};
            tmValue.tm_year = atoi(cYear) - 1900;
            tmValue.tm_mon = atoi(cMon) - 1;
            tmValue.tm_mday = atoi(cDay);
            tmValue.tm_hour = atoi(cHour);
            tmValue.tm_min = atoi(cMin);
            tmValue.tm_sec = atoi(cSec);
            tmValue.tm_isdst = -1;
            
            return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::MakeTime(int iYear, int iMonth, int iDay, int iHour, int iMinute, int iSecond)
        {
            struct tm tmValue={0};
            tmValue.tm_year = iYear - 1900;
            tmValue.tm_mon = iMonth - 1;
            tmValue.tm_mday = iDay;
            tmValue.tm_sec = iSecond;         /* seconds */
            tmValue.tm_min = iMinute;         /* minutes */
            tmValue.tm_hour = iHour;        /* hours */

            tmValue.tm_isdst = -1;
            
            return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDiffYears(time_t tTime1, time_t tTime2)
        {
            if (tTime1 == (time_t)-1)
            {
                tTime1 = time(NULL);
            }

            if (tTime2 == (time_t)-1)
            {
                tTime2 = time(NULL);
            }
            if(tTime1 == tTime2) return 0;
            
            struct tm tmValue1 = {0};
            struct tm tmValue2 = {0};
            
            if ((NULL == _Ty::GetTM(&tmValue1, tTime1))
                 || (NULL == _Ty::GetTM(&tmValue2, tTime2)))
            {
                return -1;
            }
            
            return tmValue1.tm_year - tmValue2.tm_year;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDiffMonths(time_t tTime1, time_t tTime2)
        {
            if (tTime1 == (time_t)-1)
            {
                tTime1 = time(NULL);
            }
            
            if (tTime2 == (time_t)-1)
            {
                tTime2 = time(NULL);
            }
            if(tTime1 == tTime2) return 0;
            
            struct tm tmValue1, tmValue2;
            if (NULL == _Ty::GetTM(&tmValue1, tTime1)
                || NULL == _Ty::GetTM(&tmValue2, tTime2))
            {
                return 0;
            }
            return (tmValue1.tm_year - tmValue2.tm_year) * 12 + tmValue1.tm_mon - tmValue2.tm_mon;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDiffDays(time_t tTime1, time_t tTime2)
        { 
            if (tTime1 == (time_t)-1)
            {
                tTime1 = time(NULL);
            }

            if (tTime2 == (time_t)-1)
            {
                tTime2 = time(NULL);
            }
            if(tTime1 == tTime2) return 0;
            //ȡ���賿ʱ��
            tTime1 = GetDayBegin(tTime1);
            tTime2 = GetDayBegin(tTime2);
            int iDiffDay = (int)difftime(tTime1, tTime2)/ 86400;
            return iDiffDay;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDiffHours(time_t tTime1, time_t tTime2)
        {
            if (tTime1 == (time_t)-1)
            {
                tTime1 = time(NULL);
            }
            
            if (tTime2 == (time_t)-1)
            {
                tTime2 = time(NULL);
            }
            if(tTime1 == tTime2) return 0;
            tTime1 = GetHourBegin(tTime1);
            tTime2 = GetHourEnd(tTime2);
            return (int)difftime(tTime1, tTime2) / 3600 ;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDiffMinutes(time_t tTime1, time_t tTime2)
        {
            if (tTime1 == (time_t)-1)
            {
                tTime1 = time(NULL);
            }
            
            if (tTime2 == (time_t)-1)
            {
                tTime2 = time(NULL);
            }
            if(tTime1 == tTime2) return 0;
            tTime1 = GetMinuteBegin(tTime1);
            tTime2 = GetMinuteBegin(tTime2);
            return (int)difftime(tTime1, tTime2) / 60;    
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDistanceYears(time_t tTime1, time_t tTime2)
        {
            if (tTime1 == (time_t)-1)
            {
                tTime1 = time(NULL);
            }
            
            if (tTime2 == (time_t)-1)
            {
                tTime2 = time(NULL);
            }
            return (int)difftime(tTime1, tTime2)/31536000;//����365���������
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDistanceMonths(time_t tTime1, time_t tTime2)
        {
            if (tTime1 == (time_t)-1)
            {
                tTime1 = time(NULL);
            }
            
            if (tTime2 == (time_t)-1)
            {
                tTime2 = time(NULL);
            }
            if(tTime1 == tTime2) return 0;    
            struct tm tmValue1, tmValue2;
            if (NULL == _Ty::GetTM(&tmValue1, tTime1)
                || NULL == _Ty::GetTM(&tmValue2, tTime2))
            {
                return 0;
            }
            
            return (int)difftime(tTime1, tTime2)/2592000;//����30���������
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDistanceDays(time_t tTime1, time_t tTime2)
        { 
            if (tTime1 == (time_t)-1)
            {
                tTime1 = time(NULL);
            }
            
            if (tTime2 == (time_t)-1)
            {
                tTime2 = time(NULL);
            }
            return (int)difftime(tTime1, tTime2)/86400;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDistanceHours(time_t tTime1, time_t tTime2)
        {
            if (tTime1 == (time_t)-1)
            {
                tTime1 = time(NULL);
            }
            if (tTime2 == (time_t)-1)
            {
                tTime2 = time(NULL);
            }
            return (int)difftime(tTime1, tTime2) / 3600 ;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDistanceMinutes(time_t tTime1, time_t tTime2)
        {
            if (tTime1 == (time_t)-1)
            {
                tTime1 = time(NULL);
            }
            
            if (tTime2 == (time_t)-1)
            {
                tTime2 = time(NULL);
            }
            
            return (int)difftime(tTime1, tTime2) / 60;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetYearDay(time_t tTime /* = (time_t)-1 */ )
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return -1;
            }
            
            return tmValue.tm_yday;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetMonthDay(time_t tTime /* = (time_t)-1 */)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return -1;
            }
            return tmValue.tm_mday;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetWeekDay(time_t tTime /* = (time_t)-1 */)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return -1;
            }
            
            return tmValue.tm_wday;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetYear(time_t tTime /* = (time_t)-1 */)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }
            
            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return -1;
            }
            
            return tmValue.tm_year + 1900;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetMonth(time_t tTime /* = (time_t)-1 */)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }
            
            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return -1;
            }
            
            return tmValue.tm_mon + 1;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetDay(time_t tTime /* = (time_t)-1 */)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return -1;
            }
            
            return tmValue.tm_mday;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetHour(time_t tTime /* = (time_t)-1 */)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return -1;
            }
            
            return tmValue.tm_hour;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetMinute(time_t tTime /* = (time_t)-1 */)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return -1;
            }
            
            return tmValue.tm_min;
        }

        template<class _Ty>
        int TMdbNtcDateTimeBase<_Ty>::GetSecond(time_t tTime /* = (time_t)-1 */)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return -1;
            }
            
            return tmValue.tm_sec;
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::GetYearBegin(time_t tTime/* = (time_t)-1*/)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return (time_t)-1;
            }

            tmValue.tm_mon = 0;
            tmValue.tm_mday = 1;
            tmValue.tm_sec = 0;         /* seconds */
            tmValue.tm_min = 0;         /* minutes */
            tmValue.tm_hour = 0;        /* hours */
            tmValue.tm_isdst = -1;

            return _Ty::MakeTime(&tmValue);    
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::GetYearEnd(time_t tTime/* = (time_t)-1*/)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return (time_t)-1;
            }

            tmValue.tm_mon = 11;
            tmValue.tm_mday = 31;
            tmValue.tm_hour = 23;        /* hours */
            tmValue.tm_sec = 59;         /* seconds */
            tmValue.tm_min = 59;         /* minutes */
            tmValue.tm_isdst = -1;

           return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::GetMonthBegin(time_t tTime /*= (time_t)-1*/)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue = {0};
            tmValue.tm_isdst = -1;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return (time_t)-1;
            }
            
            tmValue.tm_mday = 1;
            tmValue.tm_hour = 0;        /* hours */
            tmValue.tm_sec = 0;         /* seconds */
            tmValue.tm_min = 0;         /* minutes */
            tmValue.tm_isdst = -1;

            return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::GetMonthEnd(time_t tTime/* = (time_t)-1*/)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return (time_t)-1;
            }
            
            switch (tmValue.tm_mon+1)
            {
            case 2:
                {
                    if (TMdbNtcDateTimeBase::IsLeapYear(tmValue.tm_year + 1900))
                    {
                        tmValue.tm_mday = 29;
                    }
                    else
                    {
                        tmValue.tm_mday = 28;
                    }
                    break;
                }
            case 1:
            case 3:
            case 5:
            case 7:
            case 8:
            case 10:
            case 12:
                {
                   tmValue.tm_mday = 31;
                   break;
                }
            default:
                {
                   tmValue.tm_mday = 30;
                }
            }
            
            tmValue.tm_hour = 23;        /* hours */
            tmValue.tm_min = 59;         /* minutes */
            tmValue.tm_sec = 59;         /* seconds */
            tmValue.tm_isdst = -1;

            return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::GetDayBegin(time_t tTime /*= (time_t)-1*/)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return (time_t)-1;
            }
            
            tmValue.tm_hour = 0;        /* hours */
            tmValue.tm_sec = 0;         /* seconds */
            tmValue.tm_min = 0;         /* minutes */
            tmValue.tm_isdst = -1;

            return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::GetDayEnd(time_t tTime/* = (time_t)-1*/)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return (time_t)-1;
            }
            
            tmValue.tm_hour = 23;        /* hours */
            tmValue.tm_sec = 59;         /* seconds */
            tmValue.tm_min = 59;         /* minutes */
            tmValue.tm_isdst = -1;

            return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::GetHourBegin(time_t tTime/* = (time_t)-1*/)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return (time_t)-1;
            }
            
            tmValue.tm_sec = 0;         /* seconds */
            tmValue.tm_min = 0;         /* minutes */
            tmValue.tm_isdst = -1;

            return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::GetHourEnd(time_t tTime/* = (time_t)-1*/)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return (time_t)-1;
            }
            
            tmValue.tm_sec = 59;         /* seconds */
            tmValue.tm_min = 59;         /* minutes */
            tmValue.tm_isdst = -1;

            return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::GetMinuteBegin(time_t tTime/* = (time_t)-1*/)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return (time_t)-1;
            }
            
            tmValue.tm_sec = 0;         /* seconds */
            tmValue.tm_isdst = -1;

            return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::GetMinuteEnd(time_t tTime/* = (time_t)-1*/)
        {
            if (tTime == (time_t)-1)
            {
                tTime = time(NULL);
            }

            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return (time_t)-1;
            }
            
            tmValue.tm_sec = 59;         /* seconds */
            tmValue.tm_isdst = -1;

            return _Ty::MakeTime(&tmValue);
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::AddDate(time_t tTime, int iYears /* = 0 */, int iMonths /* = 0 */, int iDays /* = 0 */)
        {
            if (tTime == (time_t)-1)
            {
                return (time_t)-1;
            }
            if (0 == iYears && 0 == iMonths && 0 == iDays)
            {
                return tTime;
            }    
            struct tm tmValue;
            if (NULL == _Ty::GetTM(&tmValue, tTime))
            {
                return tTime;
            }
            tmValue.tm_year += iYears;
            tmValue.tm_mon += iMonths;
            if(tmValue.tm_mon >= 12)
            {
                do 
                {
                    tmValue.tm_mon -= 12;
                    ++tmValue.tm_year; 
                } while (tmValue.tm_mon >= 12);        
            }
            else if(tmValue.tm_mon < 0)
            {
                do 
                {
                    tmValue.tm_mon += 12;
                    --tmValue.tm_year; 
                } while (tmValue.tm_mon < 0);
            }
            tmValue.tm_isdst = -1;
           
            tTime = _Ty::MakeTime(&tmValue);
            if(tTime != (time_t)-1)
            {
                tTime += iDays*86400;
            }
            return tTime;
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::AddTime(time_t tTime, int iHours /* = 0 */, int iMinutes /* = 0 */, int iSeconds /* = 0 */)
        { 
            if (tTime == (time_t)-1)
            {
                return (time_t)-1;
            }
            else
            {
                return tTime + iHours * 3600 + iMinutes * 60 + iSeconds;
            }   
        }

        template<class _Ty>
        time_t TMdbNtcDateTimeBase<_Ty>::AddDateTime(time_t tTime, int iYears /* = 0 */, int iMonths /* = 0 */, int iDays /* = 0 */,
                                         int iHours /* = 0 */, int iMinutes /* = 0 */, int iSeconds /* = 0 */)
        {
            if (0 == iHours && 0 == iMinutes && 0 == iSeconds
                && 0 == iYears && 0 == iMonths && 0 == iDays)
            {
                return tTime;
            }
            tTime = AddDate(tTime, iYears, iMonths, iDays);
            if(tTime != (time_t)-1)
            {
                tTime += iHours*3600 + iMinutes*60 + iSeconds;
            }
            return tTime;
        }

        template<class _Ty>
        bool TMdbNtcDateTimeBase<_Ty>::BetweenDate(const char *pszInDate,const char *pszBeginDate,const char *pszEndDate)
        {
            assert(pszInDate != NULL && (pszInDate != NULL || pszEndDate != NULL));
            if(pszBeginDate == NULL)
            {
                return strcmp(pszInDate, pszEndDate)<0;
            }
            else if(pszEndDate == NULL)
            {
                return strcmp(pszInDate, pszBeginDate)>=0;
            }
            else
            {
                return strcmp(pszInDate, pszBeginDate)>=0 && strcmp(pszInDate, pszEndDate)<0;
            }
        }

        template<class _Ty>
        bool TMdbNtcDateTimeBase<_Ty>::BetweenDate(time_t ttInDate, time_t ttBeginDate, time_t ttEndDate)
        {
            if(ttBeginDate == (time_t)-1)
            {
                return ttInDate<ttEndDate;
            }
            else if(ttEndDate == (time_t)-1)
            {
                return ttInDate>=ttBeginDate;
            }
            else
            {
                return ttInDate>=ttBeginDate && ttInDate<ttEndDate;
            }
        }

        typedef TMdbNtcLocalDateTime TMdbNtcDateTime;

        #if !(defined(OS_WINDOWS) && _MSC_VER < 1300) //���ڷ�VC6 (VC6��֧��using _Ty::Format;�������÷�)
        /**
         * @brief ����ʽ������ʱ��ר����(YYYY-MM-DD HH:MI:SS)
         * 
         */
        template<class _Ty>
        class TMdbNtcLongDateTimeBase:public _Ty
        {
        public:
            /**
             * @brief ����ָ����ʽ��ʽ��ʱ���ַ���
             *      
             * @param tTime [in] ʱ��,-1��ʾȡ��ǰʱ��(�ɾ���TΪ����ʱ�仹��GMTʱ�����)
             * @return TMdbNtcStringBuffer
             * @retval ��ʽ����Ĵ�
             */
            using _Ty::Format;
            inline static TMdbNtcStringBuffer Format(time_t tTime = (time_t)-1)
            {
                return Format("YYYY-MM-DD HH:MI:SS", tTime);
            }
            /**
             * @brief ����ָ����ʽ��ʽ��ʱ���ַ���
             * 
             * @param ptm [in] ʱ��,NULL��ʾȡ��ǰʱ��(�ɾ���TΪ����ʱ�仹��GMTʱ�����)
             * @return TMdbNtcStringBuffer
             * @retval ��ʽ����Ĵ�
             */    
            inline static TMdbNtcStringBuffer Format(struct tm *ptm)
            {
                return Format("YYYY-MM-DD HH:MI:SS", ptm);
            }
            /**
             * @brief ���ַ���ת��Ϊʱ���
             * 
             * @param pszPattern [in] ʱ���ʽ
             * @param pszTime [in] ʱ���ַ���
             * @return time_t
             * @retval -1 ʧ��
             */
            using _Ty::MakeTime;
            inline static time_t MakeTime(const char* pszTime)
            {
                return MakeTime("YYYY-MM-DD HH:MI:SS", pszTime);
            }
                /**
             * @brief ��������ʱ������Ŀ������������ͬһ���ڣ������0
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int     
             */
            using _Ty::GetDiffYears;
            static int GetDiffYears(const char* pszTime1, const char* pszTime2);
            /**
             * @brief ��������ʱ������Ŀ������������ͬһ���ڣ������0
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDiffMonths;
            static int GetDiffMonths(const char* pszTime1, const char* pszTime2);
            /**
             * @brief ��������ʱ������Ŀ������������ͬһ���ڣ������0
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int     
             */
            using _Ty::GetDiffDays;
            inline static int GetDiffDays(const char* pszTime1, const char* pszTime2)
            {
                return GetDiffDays(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������Ŀ�Сʱ���������ͬһСʱ�ڣ������0
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDiffHours;
            inline static int GetDiffHours(const char* pszTime1, const char* pszTime2)
            {
                return GetDiffHours(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������Ŀ�������������ͬһ�����ڣ������0
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDiffMinutes;
            inline static int GetDiffMinutes(const char* pszTime1, const char* pszTime2)
            {
                return GetDiffMinutes(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ�����������
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDiffSeconds;
            inline static int GetDiffSeconds(const char* pszTime1, const char* pszTime2)
            {
                return difftime(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������������ʱ�����������365��
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDistanceYears;
            inline static int GetDistanceYears(const char* pszTime1, const char* pszTime2)
            {
                return GetDistanceYears(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������������ʱ�����������30��
             * 
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDistanceMonths;
            inline static int GetDistanceMonths(const char* pszTime1, const char* pszTime2)
            {
                return GetDistanceMonths(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������������ʱ�����������86400
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int     
             */
            using _Ty::GetDistanceDays;
            inline static int GetDistanceDays(const char* pszTime1, const char* pszTime2)
            {
                return GetDistanceDays(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������Сʱ����ʱ�����������3600
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDistanceHours;
            inline static int GetDistanceHours(const char* pszTime1, const char* pszTime2)
            {
                return GetDistanceHours(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ�����ķ�������ʱ�����������60
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDistanceMinutes;
            inline static int GetDistanceMinutes(const char* pszTime1, const char* pszTime2)
            {
                return GetDistanceMinutes(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ�����������
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDistanceSeconds;
            inline static int GetDistanceSeconds(const char* pszTime1, const char* pszTime2)
            {
                return difftime(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��ȡʱ���ǽ���ĵڶ�����
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval days since January 1 - [0,365]
             */
            using _Ty::GetYearDay;
            inline static int GetYearDay(const char* pszTime)
            {
                return GetYearDay(MakeTime(pszTime));
            }
            /**
             * @brief ��ȡʱ��������µĵڶ�����
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval day of the month - [1,31]
             */
            using _Ty::GetMonthDay;
            inline static int GetMonthDay(const char* pszTime);
            /**
             * @brief ��ȡʱ�������ڼ�
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval days since Sunday - [0,6]
             */
            using _Ty::GetWeekDay;
            inline static int GetWeekDay(const char* pszTime)
            {
                return GetWeekDay(MakeTime(pszTime));
            }
            /**
             * @brief ����ʱ�������
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [1970, 2036]
             */
            using _Ty::GetYear;
            static int GetYear(const char* pszTime);
            /**
             * @brief ����ʱ�����·�
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [1,12]
             */
            using _Ty::GetMonth;
            static int GetMonth(const char* pszTime);
            /**
             * @brief ����ʱ������
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [1,31]
             */
            using _Ty::GetDay;
            static int GetDay(const char* pszTime);
            /**
             * @brief ����ʱ����Сʱ
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [0,23]
             */
            using _Ty::GetHour;
            static int GetHour(const char* pszTime);
            /**
             * @brief ����ʱ���÷���
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [0,59]
             */
            using _Ty::GetMinute;
            static int GetMinute(const char* pszTime);
            /**
             * @brief ����ʱ������
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [0,59]
             */
            using _Ty::GetSecond;
            static int GetSecond(const char* pszTime);
            
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�꿪ʼʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�꿪ʼʱ��
             */
            using _Ty::GetYearBegin;
            static TMdbNtcStringBuffer GetYearBegin(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�����ʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�����ʱ��
             */
            using _Ty::GetYearEnd;
            static TMdbNtcStringBuffer GetYearEnd(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�¿�ʼʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�¿�ʼʱ��
             */
            using _Ty::GetMonthBegin;
            static TMdbNtcStringBuffer GetMonthBegin(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�����ʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�����ʱ��
             */
            using _Ty::GetMonthEnd;
            static TMdbNtcStringBuffer GetMonthEnd(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�쿪ʼʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�쿪ʼʱ��
             */
            using _Ty::GetDayBegin;
            static TMdbNtcStringBuffer GetDayBegin(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�����ʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�����ʱ��
             */
            using _Ty::GetDayEnd;
            static TMdbNtcStringBuffer GetDayEnd(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰСʱ��ʼʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰСʱ��ʼʱ��
             */
            using _Ty::GetHourBegin;
            static TMdbNtcStringBuffer GetHourBegin(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰСʱ���ʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰСʱ���ʱ��
             */
            using _Ty::GetHourEnd;
            static TMdbNtcStringBuffer GetHourEnd(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰ���ӿ�ʼʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰ���ӿ�ʼʱ��
             */
            using _Ty::GetMinuteBegin;
            static TMdbNtcStringBuffer GetMinuteBegin(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰ�������ʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰ�������ʱ��
             */
            using _Ty::GetMinuteEnd;
            static TMdbNtcStringBuffer GetMinuteEnd(const char* pszTime);
            /**
             * @brief ��ԭʱ������ϵ������ڣ������գ�
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iYears     [in] ����
             * @param iMonths    [in] ����
             * @param iDays      [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddDate;
            static TMdbNtcStringBuffer AddDate(const char* pszCurTime, int iYears = 0, int iMonths = 0, int iDays = 0);
            /**
             * @brief ��ԭʱ������ϵ���ʱ�䣨ʱ���룩
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iHours     [in] Сʱ��
             * @param iMinutes   [in] ������
             * @param iSeconds   [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddTime;
            static TMdbNtcStringBuffer AddTime(const char* pszCurTime, int iHours = 0, int iMinutes = 0, int iSeconds = 0);
            /**
             * @brief ��ԭʱ������ϵ�������ʱ�䣨������ʱ���룩
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iYears     [in] ����
             * @param iMonths    [in] ����
             * @param iDays      [in] ����
             * @param iHours     [in] Сʱ��
             * @param iMinutes   [in] ������
             * @param iSeconds   [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddDateTime;
            static TMdbNtcStringBuffer AddDateTime(const char* pszCurTime, int iYears = 0, int iMonths = 0, int iDays = 0,
                int iHours = 0, int iMinutes = 0, int iSeconds = 0)
            {
                if (0 == iHours && 0 == iMinutes && 0 == iSeconds
                    && 0 == iYears && 0 == iMonths && 0 == iDays)
                {
                    return pszCurTime;
                }
                time_t tCurTime = MakeTime(pszCurTime);
                if(tCurTime == (time_t)-1)
                {
                    return "";
                }
                tCurTime = AddDate(tCurTime, iYears, iMonths, iDays);
                if(tCurTime == (time_t)-1)
                {
                    return "";
                }
                else
                {
                    tCurTime += iHours*3600 + iMinutes*60 + iSeconds;
                    return Format(tCurTime);
                }
            }
            /**
             * @brief ��ԭʱ������ϵ������
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iYears     [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddYears;
            inline static TMdbNtcStringBuffer AddYears(const char* pszCurTime, int iYears = 1)
            {
                return AddDate(pszCurTime, iYears, 0, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ�������
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iMonths    [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddMonths;
            inline static TMdbNtcStringBuffer AddMonths(const char* pszCurTime, int iMonths = 1)
            {
                return AddDate(pszCurTime, 0, iMonths, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ�������
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iDays      [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddDays;
            inline static TMdbNtcStringBuffer AddDays(const char* pszCurTime, int iDays = 1)
            {
                return AddDate(pszCurTime, 0, 0, iDays);
            }
            /**
             * @brief ��ԭʱ������ϵ���Сʱ��
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iHours     [in] Сʱ��
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddHours;
            inline static TMdbNtcStringBuffer AddHours(const char* pszCurTime, int iHours = 1)
            {
                return AddTime(pszCurTime, iHours, 0, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ��ӷ�����
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iMinutes   [in] ������
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddMinutes;
            inline static TMdbNtcStringBuffer AddMinutes(const char* pszCurTime, int iMinutes = 1)
            {
                return AddTime(pszCurTime, 0, iMinutes, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ�������
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iSeconds   [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddSeconds;
            inline static TMdbNtcStringBuffer AddSeconds(const char* pszCurTime, int iSeconds = 1)
            {
                return AddTime(pszCurTime, 0, 0, iSeconds);
            }
        };

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::AddDate(const char* pszCurTime, int iYears /* = 0 */, int iMonths /* = 0 */, int iDays /* = 0 */)
        {
            time_t tCurTime = MakeTime(pszCurTime);
            if(tCurTime == (time_t)-1)
            {
                return "";
            }
            tCurTime = AddDate(tCurTime, iYears, iMonths, iDays);
            if(tCurTime == (time_t)-1)
            {
                return "";
            }
            else
            {
                return Format(tCurTime);
            }
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::AddTime(const char* pszCurTime, int iHours /* = 0 */, int iMinutes /* = 0 */, int iSeconds /* = 0 */)
        {
            time_t tCurTime = MakeTime(pszCurTime);
            if(tCurTime == (time_t)-1)
            {
                return "";
            }
            tCurTime += iHours * 3600 + iMinutes * 60 + iSeconds;
            return Format(tCurTime);
        }

        template<class _Ty>
        int TMdbNtcLongDateTimeBase<_Ty>::GetDiffYears(const char* pszTime1, const char* pszTime2)
        {
            char szYear1[5]={'\0'}, szYear2[5]={'\0'};
            memcpy(szYear1, pszTime1, 4);
            memcpy(szYear2, pszTime2, 4);
            return atoi(szYear1)-atoi(szYear2);
        }

        template<class _Ty>
        int TMdbNtcLongDateTimeBase<_Ty>::GetDiffMonths(const char* pszTime1, const char* pszTime2)
        {
            int iDiffYears = GetDiffYears(pszTime1, pszTime2);
            char szMonth1[3]={'\0'}, szMonth2[3]={'\0'};
            memcpy(szMonth1, pszTime1+5, 2);
            memcpy(szMonth2, pszTime2+5, 2);
            return iDiffYears*12+(atoi(szMonth1)-atoi(szMonth2));
        }

        template<class _Ty>
        int TMdbNtcLongDateTimeBase<_Ty>::GetMonthDay(const char* pszTime)
        {
            char szDay[3]={'\0'};
            memcpy(szDay, pszTime+8, 2);
            return atoi(szDay);
        }

        template<class _Ty>
        int TMdbNtcLongDateTimeBase<_Ty>::GetYear(const char* pszTime)
        {
            char szYear[5]={'\0'};
            memcpy(szYear, pszTime, 4);
            return atoi(szYear);
        }

        template<class _Ty>
        int TMdbNtcLongDateTimeBase<_Ty>::GetMonth(const char* pszTime)
        {
            char szMonth[3]={'\0'};
            memcpy(szMonth, pszTime+5, 2);
            return atoi(szMonth);
        }

        template<class _Ty>
        int TMdbNtcLongDateTimeBase<_Ty>::GetDay(const char* pszTime)
        {
            char szDay[3]={'\0'};
            memcpy(szDay, pszTime+8, 2);
            return atoi(szDay);
        }

        template<class _Ty>
        int TMdbNtcLongDateTimeBase<_Ty>::GetHour(const char* pszTime)
        {
            char szHour[3]={'\0'};
            memcpy(szHour, pszTime+11, 2);
            return atoi(szHour);
        }

        template<class _Ty>
        int TMdbNtcLongDateTimeBase<_Ty>::GetMinute(const char* pszTime)
        {
            char szMinute[3]={'\0'};
            memcpy(szMinute, pszTime+14, 2);
            return atoi(szMinute);
        }

        template<class _Ty>
        int TMdbNtcLongDateTimeBase<_Ty>::GetSecond(const char* pszTime)
        {
            char szSecond[3]={'\0'};
            memcpy(szSecond, pszTime+17, 2);
            return atoi(szSecond);
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::GetYearBegin(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == 19)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+5, "01-01 00:00:00", 14); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::GetYearEnd(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == 19)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+5, "12-31 23:59:59", 14); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::GetMonthBegin(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == 19)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+8, "01 00:00:00", 11); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::GetMonthEnd(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_LONG_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                int iMonth = GetMonth(pszTime), iDay = 30;
                switch (iMonth)
                {
                case 2:
                    {
                        if (IsLeapYear(GetYear(pszTime)))
                        {
                            iDay = 29;
                        }
                        else
                        {
                            iDay = 28;
                        }
                        break;
                    }
                case 1:
                case 3:
                case 5:
                case 7:
                case 8:
                case 10:
                case 12:
                    {
                        iDay = 31;
                        break;
                    }
                default:
                    {
                        iDay = 30;
                    }
                }
                MdbNtcSnprintf(sBuffer+8, MDB_NTC_ZS_MAX_DATE_LONG_SIZE-8, "%02d 23:59:59", iDay);
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::GetDayBegin(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_LONG_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+11, "00:00:00", 8);
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::GetDayEnd(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_LONG_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+11, "23:59:59", 8);
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::GetHourBegin(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_LONG_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+14, "00:00", 5); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::GetHourEnd(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_LONG_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+14, "59:59", 5); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::GetMinuteBegin(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_LONG_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+17, "00", 2); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcLongDateTimeBase<_Ty>::GetMinuteEnd(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_LONG_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+17, "59", 2); 
            }
            return sRet;
        }
        typedef TMdbNtcLongDateTimeBase<TMdbNtcLocalDateTime> TMdbNtcLocalLongDateTime;
        typedef TMdbNtcLongDateTimeBase<TMdbNtcGmtDateTime> TMdbNtcGmtLongDateTime;
        typedef TMdbNtcLocalLongDateTime TMdbNtcLongDateTime;
        #else
        class TMdbNtcLongDateTime:public TMdbNtcLocalDateTime
        {
        public:
            /**
             * @brief ����ָ����ʽ��ʽ��ʱ���ַ���
             *      
             * @param tTime [in] ʱ��,-1��ʾȡ��ǰʱ��(�ɾ���TΪ����ʱ�仹��GMTʱ�����)
             * @return TMdbNtcStringBuffer
             * @retval ��ʽ����Ĵ�
             */
            using TMdbNtcLocalDateTime::Format;
            inline static TMdbNtcStringBuffer Format(time_t tTime = (time_t)-1)
            {
                return Format("YYYY-MM-DD HH:MI:SS", tTime);
            }
            /**
             * @brief ����ָ����ʽ��ʽ��ʱ���ַ���
             * 
             * @param ptm [in] ʱ��,NULL��ʾȡ��ǰʱ��(�ɾ���TΪ����ʱ�仹��GMTʱ�����)
             * @return TMdbNtcStringBuffer
             * @retval ��ʽ����Ĵ�
             */
            inline static TMdbNtcStringBuffer Format(struct tm *ptm)
            {
                return Format("YYYY-MM-DD HH:MI:SS", ptm);
            }
            /**
             * @brief ���ַ���ת��Ϊʱ���
             * 
             * @param pszPattern [in] ʱ���ʽ
             * @param pszTime [in] ʱ���ַ���
             * @return time_t
             * @retval -1 ʧ��
             */
            using TMdbNtcLocalDateTime::MakeTime;
            inline static time_t MakeTime(const char* pszTime)
            {
                return MakeTime("YYYY-MM-DD HH:MI:SS", pszTime);
            }
        };
        #endif//end of !(defined(OS_WINDOWS) && _MSC_VER < 1300)

        #if !(defined(OS_WINDOWS) && _MSC_VER < 1300) //���ڷ�VC6(VC6��֧��using _Ty::Format;�������÷�)
        /**
         * @brief �̸�ʽ������ʱ��ר����(YYYYMMDDHHMISS)
         * 
         */
        template<class _Ty>
        class TMdbNtcShortDateTimeBase:public _Ty
        {
        public:
            /**
             * @brief ����ָ����ʽ��ʽ��ʱ���ַ���
             *      
             * @param tTime [in] ʱ��,-1��ʾȡ��ǰʱ��(�ɾ���TΪ����ʱ�仹��GMTʱ�����)
             * @return TMdbNtcStringBuffer
             * @retval ��ʽ����Ĵ�
             */
            using _Ty::Format;
            inline static TMdbNtcStringBuffer Format(time_t tTime = (time_t)-1)
            {
                return Format("YYYYMMDDHHMISS", tTime);
            }
            /**
             * @brief ����ָ����ʽ��ʽ��ʱ���ַ���
             * 
             * @param ptm [in] ʱ��,NULL��ʾȡ��ǰʱ��(�ɾ���TΪ����ʱ�仹��GMTʱ�����)
             * @return TMdbNtcStringBuffer
             * @retval ��ʽ����Ĵ�
             */    
            inline static TMdbNtcStringBuffer Format(struct tm *ptm)
            {
                return Format("YYYYMMDDHHMISS", ptm);
            }
            /**
             * @brief ���ַ���ת��Ϊʱ���
             * 
             * @param pszPattern [in] ʱ���ʽ
             * @param pszTime    [in] ʱ���ַ���
             * @return time_t
             * @retval -1 ʧ��
             */
            using _Ty::MakeTime;
            inline static time_t MakeTime(const char* pszTime)
            {
                return MakeTime("YYYYMMDDHHMISS", pszTime);
            }
                /**
             * @brief ��������ʱ������Ŀ������������ͬһ���ڣ������0
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int     
             */
            using _Ty::GetDiffYears;
            inline static int GetDiffYears(const char* pszTime1, const char* pszTime2);
            /**
             * @brief ��������ʱ������Ŀ������������ͬһ���ڣ������0
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDiffMonths;
            inline static int GetDiffMonths(const char* pszTime1, const char* pszTime2);
            /**
             * @brief ��������ʱ������Ŀ������������ͬһ���ڣ������0
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int     
             */
            using _Ty::GetDiffDays;
            inline static int GetDiffDays(const char* pszTime1, const char* pszTime2)
            {
                return GetDiffDays(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������Ŀ�Сʱ���������ͬһСʱ�ڣ������0
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDiffHours;
            inline static int GetDiffHours(const char* pszTime1, const char* pszTime2)
            {
                return GetDiffHours(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������Ŀ�������������ͬһ�����ڣ������0
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDiffMinutes;
            inline static int GetDiffMinutes(const char* pszTime1, const char* pszTime2)
            {
                return GetDiffMinutes(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ�����������
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDiffSeconds;
            inline static int GetDiffSeconds(const char* pszTime1, const char* pszTime2)
            {
                return difftime(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������������ʱ�����������365��
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDistanceYears;
            inline static int GetDistanceYears(const char* pszTime1, const char* pszTime2)
            {
                return GetDistanceYears(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������������ʱ�����������30��
             * 
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDistanceMonths;
            inline static int GetDistanceMonths(const char* pszTime1, const char* pszTime2)
            {
                return GetDistanceMonths(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������������ʱ�����������86400
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int     
             */
            using _Ty::GetDistanceDays;
            inline static int GetDistanceDays(const char* pszTime1, const char* pszTime2)
            {
                return GetDistanceDays(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ������Сʱ����ʱ�����������3600
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDistanceHours;
            inline static int GetDistanceHours(const char* pszTime1, const char* pszTime2)
            {
                return GetDistanceHours(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ�����ķ�������ʱ�����������60
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDistanceMinutes;
            inline static int GetDistanceMinutes(const char* pszTime1, const char* pszTime2)
            {
                return GetDistanceMinutes(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��������ʱ�����������
             * 
             * @param pszTime1 [in] ʱ��1
             * @param pszTime2 [in] ʱ��2
             * @return int
             */
            using _Ty::GetDistanceSeconds;
            inline static int GetDistanceSeconds(const char* pszTime1, const char* pszTime2)
            {
                return difftime(MakeTime(pszTime1), MakeTime(pszTime2));
            }
            /**
             * @brief ��ȡʱ���ǽ���ĵڶ�����
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval days since January 1 - [0,365]
             */
            using _Ty::GetYearDay;
            inline static int GetYearDay(const char* pszTime)
            {
                return GetYearDay(MakeTime(pszTime));
            }
            /**
             * @brief ��ȡʱ��������µĵڶ�����
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval day of the month - [1,31]
             */
            using _Ty::GetMonthDay;
            static int GetMonthDay(const char* pszTime);
            /**
             * @brief ��ȡʱ�������ڼ�
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval days since Sunday - [0,6]
             */
            using _Ty::GetWeekDay;
            inline static int GetWeekDay(const char* pszTime)
            {
                return GetWeekDay(MakeTime(pszTime));
            }
            /**
             * @brief ����ʱ�������
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [1970, 2036]
             */
            using _Ty::GetYear;
            static int GetYear(const char* pszTime);
            /**
             * @brief ����ʱ�����·�
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [1,12]
             */
            using _Ty::GetMonth;
            static int GetMonth(const char* pszTime);
            /**
             * @brief ����ʱ������
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [1,31]
             */
            using _Ty::GetDay;
            static int GetDay(const char* pszTime);
            /**
             * @brief ����ʱ����Сʱ
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [0,23]
             */
            using _Ty::GetHour;
            static int GetHour(const char* pszTime);
            /**
             * @brief ����ʱ���÷���
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [0,59]
             */
            using _Ty::GetMinute;
            static int GetMinute(const char* pszTime);
            /**
             * @brief ����ʱ������
             * 
             * @param pszTime [in] ʱ��
             * @return int
             * @retval [0,59]
             */
            using _Ty::GetSecond;
            static int GetSecond(const char* pszTime);
            
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�꿪ʼʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�꿪ʼʱ��
             */
            using _Ty::GetYearBegin;
            static TMdbNtcStringBuffer GetYearBegin(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�����ʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�����ʱ��
             */
            using _Ty::GetYearEnd;
            static TMdbNtcStringBuffer GetYearEnd(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�¿�ʼʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�¿�ʼʱ��
             */
            using _Ty::GetMonthBegin;
            static TMdbNtcStringBuffer GetMonthBegin(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�����ʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�����ʱ��
             */
            using _Ty::GetMonthEnd;
            static TMdbNtcStringBuffer GetMonthEnd(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�쿪ʼʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�쿪ʼʱ��
             */
            using _Ty::GetDayBegin;
            static TMdbNtcStringBuffer GetDayBegin(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰһ�����ʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰһ�����ʱ��
             */
            using _Ty::GetDayEnd;
            static TMdbNtcStringBuffer GetDayEnd(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰСʱ��ʼʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰСʱ��ʼʱ��
             */
            using _Ty::GetHourBegin;
            static TMdbNtcStringBuffer GetHourBegin(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰСʱ���ʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰСʱ���ʱ��
             */
            using _Ty::GetHourEnd;
            static TMdbNtcStringBuffer GetHourEnd(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰ���ӿ�ʼʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰ���ӿ�ʼʱ��
             */
            using _Ty::GetMinuteBegin;
            static TMdbNtcStringBuffer GetMinuteBegin(const char* pszTime);
            /**
             * @brief ��ȡָ��ʱ��ĵ�ǰ�������ʱ��
             * 
             * @param pszTime [in] ָ����ʱ���
             * @return TMdbNtcStringBuffer
             * @retval ��ǰ�������ʱ��
             */
            using _Ty::GetMinuteEnd;
            static TMdbNtcStringBuffer GetMinuteEnd(const char* pszTime);
            /**
             * @brief ��ԭʱ������ϵ������ڣ������գ�
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iYears     [in] ����
             * @param iMonths    [in] ����
             * @param iDays      [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddDate;
            static TMdbNtcStringBuffer AddDate(const char* pszCurTime, int iYears = 0, int iMonths = 0, int iDays = 0);
            /**
             * @brief ��ԭʱ������ϵ���ʱ�䣨ʱ���룩
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iHours     [in] Сʱ��
             * @param iMinutes   [in] ������
             * @param iSeconds   [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddTime;
            static TMdbNtcStringBuffer AddTime(const char* pszCurTime, int iHours = 0, int iMinutes = 0, int iSeconds = 0);
            /**
             * @brief ��ԭʱ������ϵ�������ʱ�䣨������ʱ���룩
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iYears     [in] ����
             * @param iMonths    [in] ����
             * @param iDays      [in] ����
             * @param iHours     [in] Сʱ��
             * @param iMinutes   [in] ������
             * @param iSeconds   [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddDateTime;
            static TMdbNtcStringBuffer AddDateTime(const char* pszCurTime, int iYears = 0, int iMonths = 0, int iDays = 0,
                int iHours = 0, int iMinutes = 0, int iSeconds = 0)
            {
                if (0 == iHours && 0 == iMinutes && 0 == iSeconds
                    && 0 == iYears && 0 == iMonths && 0 == iDays)
                {
                    return pszCurTime;
                }
                time_t tCurTime = MakeTime(pszCurTime);
                if(tCurTime == (time_t)-1)
                {
                    return "";
                }
                tCurTime = AddDate(tCurTime, iYears, iMonths, iDays);
                if(tCurTime == (time_t)-1)
                {
                    return "";
                }
                else
                {
                    tCurTime += iHours*3600 + iMinutes*60 + iSeconds;
                    return Format(tCurTime);
                }
            }
            /**
             * @brief ��ԭʱ������ϵ������
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iYears     [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddYears;
            inline static TMdbNtcStringBuffer AddYears(const char* pszCurTime, int iYears = 1)
            {
                return AddDate(pszCurTime, iYears, 0, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ�������
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iMonths    [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddMonths;
            inline static TMdbNtcStringBuffer AddMonths(const char* pszCurTime, int iMonths = 1)
            {
                return AddDate(pszCurTime, 0, iMonths, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ�������
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iDays      [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddDays;
            inline static TMdbNtcStringBuffer AddDays(const char* pszCurTime, int iDays = 1)
            {
                return AddDate(pszCurTime, 0, 0, iDays);
            }
            /**
             * @brief ��ԭʱ������ϵ���Сʱ��
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iHours     [in] Сʱ��
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddHours;
            inline static TMdbNtcStringBuffer AddHours(const char* pszCurTime, int iHours = 1)
            {
                return AddTime(pszCurTime, iHours, 0, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ��ӷ�����
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iMinutes   [in] ������
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddMinutes;
            inline static TMdbNtcStringBuffer AddMinutes(const char* pszCurTime, int iMinutes = 1)
            {
                return AddTime(pszCurTime, 0, iMinutes, 0);
            }
            /**
             * @brief ��ԭʱ������ϵ�������
             * 
             * @param pszCurTime [in] ��ǰʱ��
             * @param iSeconds   [in] ����
             * @return TMdbNtcStringBuffer
             * @retval ���Ӻ��ʱ��
             */
            using _Ty::AddSeconds;
            inline static TMdbNtcStringBuffer AddSeconds(const char* pszCurTime, int iSeconds = 1)
            {
                return AddTime(pszCurTime, 0, 0, iSeconds);
            }
        };

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::AddDate(const char* pszCurTime, int iYears /* = 0 */, int iMonths /* = 0 */, int iDays /* = 0 */)
        {
            time_t tCurTime = MakeTime(pszCurTime);
            if(tCurTime == (time_t)-1)
            {
                return "";
            }
            tCurTime = AddDate(tCurTime, iYears, iMonths, iDays);
            if(tCurTime == (time_t)-1)
            {
                return "";
            }
            else
            {
                return Format(tCurTime);
            }
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::AddTime(const char* pszCurTime, int iHours /* = 0 */, int iMinutes /* = 0 */, int iSeconds /* = 0 */)
        {
            time_t tCurTime = MakeTime(pszCurTime);
            if(tCurTime == (time_t)-1)
            {
                return "";
            }
            tCurTime += iHours * 3600 + iMinutes * 60 + iSeconds;
            return Format(tCurTime);
        }

        template<class _Ty>
        int TMdbNtcShortDateTimeBase<_Ty>::GetDiffYears(const char* pszTime1, const char* pszTime2)
        {
            char szYear1[5]={'\0'}, szYear2[5]={'\0'};
            memcpy(szYear1, pszTime1, 4);
            memcpy(szYear2, pszTime2, 4);
            return atoi(szYear1)-atoi(szYear2);
        }

        template<class _Ty>
        int TMdbNtcShortDateTimeBase<_Ty>::GetDiffMonths(const char* pszTime1, const char* pszTime2)
        {
            int iDiffYears = GetDiffYears(pszTime1, pszTime2);
            char szMonth1[3]={'\0'}, szMonth2[3]={'\0'};
            memcpy(szMonth1, pszTime1+4, 2);
            memcpy(szMonth2, pszTime2+4, 2);
            return iDiffYears*12+(atoi(szMonth1)-atoi(szMonth2));
        }

        template<class _Ty>
        int TMdbNtcShortDateTimeBase<_Ty>::GetMonthDay(const char* pszTime)
        {
            char szDay[3]={'\0'};
            memcpy(szDay, pszTime+6, 2);
            return atoi(szDay);
        }

        template<class _Ty>
        int TMdbNtcShortDateTimeBase<_Ty>::GetYear(const char* pszTime)
        {
            char szYear[5]={'\0'};
            memcpy(szYear, pszTime, 4);
            return atoi(szYear);
        }

        template<class _Ty>
        int TMdbNtcShortDateTimeBase<_Ty>::GetMonth(const char* pszTime)
        {
            char szMonth[3]={'\0'};
            memcpy(szMonth, pszTime+4, 2);
            return atoi(szMonth);
        }

        template<class _Ty>
        int TMdbNtcShortDateTimeBase<_Ty>::GetDay(const char* pszTime)
        {
            char szDay[3]={'\0'};
            memcpy(szDay, pszTime+6, 2);
            return atoi(szDay);
        }

        template<class _Ty>
        int TMdbNtcShortDateTimeBase<_Ty>::GetHour(const char* pszTime)
        {
            char szHour[3]={'\0'};
            memcpy(szHour, pszTime+8, 2);
            return atoi(szHour);
        }

        template<class _Ty>
        int TMdbNtcShortDateTimeBase<_Ty>::GetMinute(const char* pszTime)
        {
            char szMinute[3]={'\0'};
            memcpy(szMinute, pszTime+10, 2);
            return atoi(szMinute);
        }

        template<class _Ty>
        int TMdbNtcShortDateTimeBase<_Ty>::GetSecond(const char* pszTime)
        {
            char szSecond[3]={'\0'};
            memcpy(szSecond, pszTime+12, 2);
            return atoi(szSecond);
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::GetYearBegin(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == 14)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+4, "0101000000", 10); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::GetYearEnd(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == 14)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+4, "1231235959", 10); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::GetMonthBegin(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == 14)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+6, "01000000", 8); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::GetMonthEnd(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_SHORT_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                int iMonth = GetMonth(pszTime), iDay = 30;
                switch (iMonth)
                {
                case 2:
                    {
                        if (IsLeapYear(GetYear(pszTime)))
                        {
                            iDay = 29;
                        }
                        else
                        {
                            iDay = 28;
                        }
                        break;
                    }
                case 1:
                case 3:
                case 5:
                case 7:
                case 8:
                case 10:
                case 12:
                    {
                        iDay = 31;
                        break;
                    }
                default:
                    {
                        iDay = 30;
                    }
                }
                MdbNtcSnprintf(sBuffer+6, MDB_NTC_ZS_MAX_DATE_SHORT_SIZE-6, "%02d235959", iDay);
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::GetDayBegin(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_SHORT_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+8, "000000", 6);
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::GetDayEnd(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_SHORT_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+8, "235959", 6);
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::GetHourBegin(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_SHORT_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+10, "0000", 4); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::GetHourEnd(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_SHORT_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+10, "5959", 4); 
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::GetMinuteBegin(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_SHORT_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+12, "00", 2);
            }
            return sRet;
        }

        template<class _Ty>
        TMdbNtcStringBuffer TMdbNtcShortDateTimeBase<_Ty>::GetMinuteEnd(const char* pszTime)
        {
            TMdbNtcStringBuffer sRet(pszTime);
            if(sRet.GetLength() == MDB_NTC_ZS_MAX_DATE_SHORT_SIZE-1)
            {
                char* sBuffer = sRet.GetBuffer();
                memcpy(sBuffer+12, "59", 2); 
            }
            return sRet;
        }
        typedef TMdbNtcShortDateTimeBase<TMdbNtcLocalDateTime> TMdbNtcLocalShortDateTime;
        typedef TMdbNtcShortDateTimeBase<TMdbNtcGmtDateTime> TMdbNtcGmtShortDateTime;
        typedef TMdbNtcLocalShortDateTime TMdbNtcShortDateTime;
        #else
        class TMdbNtcShortDateTime:public TMdbNtcLocalDateTime
        {
        public:
            /**
             * @brief ����ָ����ʽ��ʽ��ʱ���ַ���
             * 
             * @param tTime [in] ʱ��,-1��ʾȡ��ǰʱ��(�ɾ���TΪ����ʱ�仹��GMTʱ�����)
             * @return TMdbNtcStringBuffer
             * @retval ��ʽ����Ĵ�
             */
            using TMdbNtcLocalDateTime::Format;
            inline static TMdbNtcStringBuffer Format(time_t tTime = (time_t)-1)
            {
                return Format("YYYYMMDDHHMISS", tTime);
            }
            /**
             * @brief ����ָ����ʽ��ʽ��ʱ���ַ���
             * 
             * @param ptm [in] ʱ��,NULL��ʾȡ��ǰʱ��(�ɾ���TΪ����ʱ�仹��GMTʱ�����)
             * @return TMdbNtcStringBuffer
             * @retval ��ʽ����Ĵ�
             */
            inline static TMdbNtcStringBuffer Format(struct tm *ptm)
            {
                return Format("YYYYMMDDHHMISS", ptm);
            }
            /**
             * @brief ���ַ���ת��Ϊʱ���
             * 
             * @param pszPattern [in] ʱ���ʽ
             * @param pszTime [in] ʱ���ַ���
             * @return time_t
             * @retval -1 ʧ��
             */
            using TMdbNtcLocalDateTime::MakeTime;
            inline static time_t MakeTime(const char* pszTime)
            {
                return MakeTime("YYYYMMDDHHMISS", pszTime);
            }
        };
        #endif//end of !(defined(OS_WINDOWS) && _MSC_VER < 1300)
//    }
//}
#endif
