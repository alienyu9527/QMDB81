//////////////////////////////////////////////////////////////////////////
// QMDB����ͳ����:����ִ��ʱ��ͳ�Ƶ�
//////////////////////////////////////////////////////////////////////////

#ifndef _PERF_INFO_H_
#define _PERF_INFO_H_

#include <vector>
#include <string>

using namespace std; 


#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

//namespace QuickMDB{

    class TMdbPerfStat 
    {
    public:
        TMdbPerfStat();
        TMdbPerfStat(const char *sFunName,int perCount = 0);
        virtual ~TMdbPerfStat();
        /******************************************************************************
        * ��������	:  Stat()
        * ��������	:  ����ͳ��
        * ����		:  ��  
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void Stat();

        /******************************************************************************
        * ��������	:  ClearStat()
        * ��������	:  ���ָ��ͳ����	 
        * ����		:  ��  
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void ClearStat();

        /******************************************************************************
        * ��������	:  begin()
        * ��������	:  ����ָ�꿪ʼͳ��	 
        * ����		:  ��  
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void begin();

        /******************************************************************************
        * ��������	:  end()
        * ��������	:  ����ͳ��
        * ����		:  ��  
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void end();

        /******************************************************************************
        * ��������	:  GetSqlInfo()
        * ��������	:  ��ȡsql	 
        * ����		:  ��  
        * ���		:  ��
        * ����ֵ	:  sql
        * ����		:  li.shugang
        *******************************************************************************/
        std::vector<std::string> GetSqlInfo();

        /******************************************************************************
        * ��������	:  SetSqlInfo()
        * ��������	:  ����ͳ��sql	 
        * ����		:  ͳ��sql  
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void SetSqlInfo(const char *sMySql);

        /******************************************************************************
        * ��������	:  SetLastLog()
        * ��������	:  �Ƿ�������¼	 
        * ����		:  ��  
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void SetLastLog();
    private:
        int m_microseconds;
        int m_TimeMap[4];
        int m_PerTimeMap[4];
        int m_counter;//ִ�д���
        int m_perCount;
        int m_Totalmicseconds;
        time_t m_tLogTime;//��־��¼ʱ��
        std::vector<std::string> m_vecSqls;

        #ifdef WIN32
            clock_t m_beginPoint; 
        #else
            struct timeval m_beginPoint,m_endPoint;
        #endif  
        
        float m_fOsValue;
        bool m_bLastLog;//ֻ������¼
        public:
        int theMicSec;
        char m_functionName[40];

    };

    #define MaxPerfStatePointNbr 512 //���֧�ֶ��ٸ����ܼ���

    class TMdbPerfStatMgr
    {
    public:
        TMdbPerfStatMgr();
        /******************************************************************************
        * ��������	:  ShowStatic()
        * ��������	:  ��ȡ��������ͳ�Ƶ���Ϣ	 
        * ����		:  ��  
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void ShowStatic();

        /******************************************************************************
        * ��������	:  AddPS()
        * ��������	:  ��������ͳ�Ƶ�	 
        * ����		:  ����ͳ�ƶ���
        * ���		:  ��
        * ����ֵ	:  ��
        * ����		:  li.shugang
        *******************************************************************************/
        void AddPS(TMdbPerfStat * thePS);
    public:	
        char m_PerfFileName[256];	//��־�ļ���
        private:
        TMdbPerfStat * m_PSContainer[MaxPerfStatePointNbr]; //���֧��512������ͳ�Ƶ�
        int m_iCount;
    };
    

    //���ܼ���
    #ifdef _PERF
        #define PS_MDB_START(name) static TMdbPerfStat PS##name(#name); PS##name.begin();PS##name.SetLastLog();
        #define PS_MDB_END(name) PS##name.end();
    #else
        #define PS_MDB_START(name) 
        #define PS_MDB_END(name) 
    #endif


    #ifdef __OS_WINDOWS__
        void kill(int pid,int code);
        int waitpid(int pid, int *status, int options);
        bool IsProcessExists(int pid);
    #endif


//}

#endif

