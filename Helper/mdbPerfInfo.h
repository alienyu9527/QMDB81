//////////////////////////////////////////////////////////////////////////
// QMDB性能统计类:程序执行时间统计等
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
        * 函数名称	:  Stat()
        * 函数描述	:  性能统计
        * 输入		:  无  
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void Stat();

        /******************************************************************************
        * 函数名称	:  ClearStat()
        * 函数描述	:  清空指标统计项	 
        * 输入		:  无  
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void ClearStat();

        /******************************************************************************
        * 函数名称	:  begin()
        * 函数描述	:  性能指标开始统计	 
        * 输入		:  无  
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void begin();

        /******************************************************************************
        * 函数名称	:  end()
        * 函数描述	:  结束统计
        * 输入		:  无  
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void end();

        /******************************************************************************
        * 函数名称	:  GetSqlInfo()
        * 函数描述	:  获取sql	 
        * 输入		:  无  
        * 输出		:  无
        * 返回值	:  sql
        * 作者		:  li.shugang
        *******************************************************************************/
        std::vector<std::string> GetSqlInfo();

        /******************************************************************************
        * 函数名称	:  SetSqlInfo()
        * 函数描述	:  设置统计sql	 
        * 输入		:  统计sql  
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void SetSqlInfo(const char *sMySql);

        /******************************************************************************
        * 函数名称	:  SetLastLog()
        * 函数描述	:  是否在最后记录	 
        * 输入		:  无  
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void SetLastLog();
    private:
        int m_microseconds;
        int m_TimeMap[4];
        int m_PerTimeMap[4];
        int m_counter;//执行次数
        int m_perCount;
        int m_Totalmicseconds;
        time_t m_tLogTime;//日志记录时间
        std::vector<std::string> m_vecSqls;

        #ifdef WIN32
            clock_t m_beginPoint; 
        #else
            struct timeval m_beginPoint,m_endPoint;
        #endif  
        
        float m_fOsValue;
        bool m_bLastLog;//只在最后记录
        public:
        int theMicSec;
        char m_functionName[40];

    };

    #define MaxPerfStatePointNbr 512 //最大支持多少个性能检查点

    class TMdbPerfStatMgr
    {
    public:
        TMdbPerfStatMgr();
        /******************************************************************************
        * 函数名称	:  ShowStatic()
        * 函数描述	:  获取各个性能统计点信息	 
        * 输入		:  无  
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void ShowStatic();

        /******************************************************************************
        * 函数名称	:  AddPS()
        * 函数描述	:  增加性能统计点	 
        * 输入		:  性能统计对象
        * 输出		:  无
        * 返回值	:  无
        * 作者		:  li.shugang
        *******************************************************************************/
        void AddPS(TMdbPerfStat * thePS);
    public:	
        char m_PerfFileName[256];	//日志文件名
        private:
        TMdbPerfStat * m_PSContainer[MaxPerfStatePointNbr]; //最多支持512个性能统计点
        int m_iCount;
    };
    

    //性能检测宏
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

