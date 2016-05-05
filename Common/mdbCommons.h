/**
 * @file mdbCommons.h
 * @brief 公用的定义，以及基础类
 *
 * 公用的定义，以及基础类
 *
 * @author Ge.zhengyi, Jiang.jinzhou, Du.jiagen, Zhang.he
 * @version 1.0
 * @date 20121214
 * @warning
 */

#ifndef _MDB_H_Commons_
#define _MDB_H_Commons_

/** variant platform macro define */
#if defined(AIX) ||defined(_AIX_) || defined(_AIX) || defined(_AIX_SOURCE) || defined(_IBM) || defined(OS_IBM)
    #undef OS_IBM
    /**< define AIX platform */
    #define OS_IBM
#elif defined(HPUX) || defined(HP_UX) ||defined(HP_UNIX) || defined(_HP_UX) \
    || defined (_HP_SOURCE) || defined(HP) || defined(OS_HP)
    #undef OS_HP
    /**< define HP-UNIX platform */
    #define OS_HP
#elif defined(LINUX) || defined(_LINUX_) || defined(_LINUX) || defined(_LINUX_SOURCE) || defined(OS_LINUX)
    #undef OS_LINUX
    /**< define LINUX platform */
    #define OS_LINUX
#elif defined(SUN) || defined(_SUN_OS) || defined(_SUN_SOURCE) || defined(OS_SUN)
    #undef OS_SUN
    /**< define SUN platform */
    #define OS_SUN
#elif defined(WIN32) || defined(_WIN32) || defined(_MSC_VER)
    #undef OS_WINDOWS
    /**< define WINDOWS platform */
    #define OS_WINDOWS
#elif defined(FREEBSD) || defined(_FREEBSD) || defined(OS_FREEBSD)
    #undef OS_FREEBSD
    /**< define FREEBSD platform */
    #define OS_FREEBSD
#else
    #error Cannot find a define type for your platform
#endif
#if defined(OS_IBM) || defined(OS_HP) || defined(OS_SUN) || defined(OS_LINUX) || defined(OS_CYWIN) || defined(FREEBSD)
    #undef OS_UNIX
    /**< define _UNIX platform */
    #define OS_UNIX
#endif

#ifndef QuickMDB_SOCKET
typedef unsigned int QuickMDB_SOCKET;
#endif

#ifdef OS_WINDOWS

    #ifndef _CRT_SECURE_NO_WARNINGS
        #define _CRT_SECURE_NO_WARNINGS //消除.net中对于一些函数的安全警告
    #endif

    #ifndef _USE_32BIT_TIME_T
        #define _USE_32BIT_TIME_T //使用32位的time_t达到平台兼容
    #endif

    #undef _WIN32_WINNT
    #define _WIN32_WINNT 0x0500

    //windows屏蔽掉stl的map中4786的警告
    #pragma warning(disable:4786)
    #pragma warning(disable:4146)
    #pragma warning(disable:4996)
    #pragma warning(disable:4290)
    #ifndef _WINSOCKAPI_   /* if winsock.h is included */ 
        #include <winsock2.h>
    #endif
    #include <windows.h>
    #include <process.h>
    #include <time.h>
    #include <signal.h>
    #include <io.h>
    #include <direct.h>
#else

    //信号函数申明
    #include <signal.h>

    //共享内存函数申明
    #include <sys/ipc.h>
    #include <sys/shm.h>

    //消息队列函数申明
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/msg.h>

    //信号灯函数申明
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/sem.h>
    #include <semaphore.h>

    #include <sys/socket.h>
    #include <sys/utsname.h>
    #include <sys/errno.h>
    #include <sys/time.h>

    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <pthread.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <signal.h>
    #include <unistd.h>
    #include <sys/time.h>
#endif

#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "Helper/TThreadLog.h"

//common error code define
#define MDB_NTC_ERROR_PARAMETER         501
#define MDB_NTC_ERROR_STATE             502
#define MDB_NTC_ERROR_GENERAL           503
#define MDB_NTC_ERROR_SPACE_OVERFLOW    504
#define MDB_NTC_ERROR_ENVIRONMENT       505
#define MDB_NTC_ERROR_INPUT_VALUE       506

/**
 * @brief 统一定义变量类型
 *
 */
typedef signed   char       MDB_INT8;
typedef unsigned char       MDB_UINT8;
typedef signed short        MDB_INT16;
typedef unsigned short      MDB_UINT16;
typedef signed int          MDB_INT32;
typedef unsigned int        MDB_UINT32;
#ifdef OS_WINDOWS
    typedef __int64             MDB_INT64;
    typedef unsigned __int64    MDB_UINT64;
    /*
#elif defined(OS_HP)//因为与dlfcn.h(动态库加载函数)内的定义要吻合
    #ifdef __LP64__
        typedef long                MDB_INT64;
        typedef unsigned long       MDB_UINT64;
    #else
        typedef long long           MDB_INT64;
        typedef unsigned long long  MDB_UINT64;
    #endif
    */
#else
    typedef signed   long long  MDB_INT64;
    typedef unsigned long long  MDB_UINT64;
#endif
typedef float           MDB_FLOAT32;
typedef double          MDB_FLOAT64;

typedef void (*OnMdbEventFunc)(void *AObject);

/**
 * @brief 统一定义64位整数格式化串
 *
 */
#ifdef OS_WINDOWS
#define MDB_NTC_ZS_FORMAT_INT64  "I64d"
#define MDB_NTC_ZS_FORMAT_UINT64 "I64u"
#else
#define MDB_NTC_ZS_FORMAT_INT64  "lld"
#define MDB_NTC_ZS_FORMAT_UINT64 "llu"
#endif

#if defined(OS_WINDOWS)
    #if _MSC_VER < 1300//针对VC6
        #define __MDB_FUNC__ ""
    #else
        #define __MDB_FUNC__ __FUNCTION__
    #endif
#elif defined (__GNUC__)
    #define __MDB_FUNC__ __PRETTY_FUNCTION__
#else
    #define __MDB_FUNC__ __func__
#endif

/**
 * @brief 跨平台字符串函数的统一
 *
 */

#ifdef OS_WINDOWS
    #define MDB_NTC_ZS_PATH_DELIMITATED_CHAR '\\'
    #define MDB_NTC_ZS_PATH_DELIMITATED      "\\"
    #define snprintf  _snprintf
    #define vsnprintf _vsnprintf
    #define mdb_ntc_stricmp   _stricmp
    #define mdb_ntc_strnicmp  _strnicmp
    #define strlwr    _strlwr
    #define strupr    _strupr
    #define memccpy   _memccpy
    #define getpid    _getpid
    typedef HANDLE    MDB_IPC_HANDLE;
#else
    #define MDB_NTC_ZS_PATH_DELIMITATED_CHAR '/'
    #define MDB_NTC_ZS_PATH_DELIMITATED      "/"
    #define mdb_ntc_stricmp strcasecmp
    #define mdb_ntc_strnicmp strncasecmp
    typedef int      MDB_IPC_HANDLE;
#endif

#ifndef MDB_ABS
#define MDB_ABS(n)      ((n)>=0?(n):-(n))
#endif

#ifndef MDB_MIN
#define MDB_MIN(a,b)    (((a)<(b))?(a):(b))
#endif

#ifndef MDB_MAX
#define MDB_MAX(a,b)    (((a)>(b))?(a):(b))
#endif

#ifndef MDB_SIGN
#define MDB_SIGN(a,b)       ((a)==(b)?(0):((a)<(b))?(-1):(1))
#endif

#ifdef OS_WINDOWS

    #ifndef SIGHUP
    #define SIGHUP     1    /* hangup, generated when terminal disconnects */
    #endif

    #ifndef SIGQUIT
    #define SIGQUIT    3    /* (*) quit, generated from terminal special char */
    #endif

    #ifndef SIGKILL
    #define SIGKILL    9    /* kill (cannot be caught or ignored) */
    #endif

    #ifndef SIGPIPE
    #define SIGPIPE   13    /* write on a pipe with no one to read it */
    #endif

    #ifndef SIGCHLD
    #define SIGCHLD   20    /* (+) sent to parent on child stop or exit */
    #endif

#endif


#define MDB_NTC_MAKE_SEM_TIMEOUT(timeout, iMilliSeconds) struct timespec timeout;{\
    if(iMilliSeconds == 0)\
    {\
    timeout.tv_sec = 0;\
    timeout.tv_nsec = 0;\
    }\
    else\
    {\
    timeout.tv_sec = iMilliSeconds/1000;\
    timeout.tv_nsec = (iMilliSeconds%1000)*1000000;\
    if(timeout.tv_nsec >= 1e9)\
        {\
        ++timeout.tv_sec;\
        timeout.tv_nsec -= (long)1e9;\
        }\
    }\
}

#define MDB_NTC_MAKE_TIMEOUT(timeout, iMilliSeconds) struct timespec timeout;{\
    struct timeval now;\
    gettimeofday(&now, NULL);\
    if(iMilliSeconds == 0)\
    {\
        timeout.tv_sec = now.tv_sec;\
        timeout.tv_nsec = 0;\
    }\
    else\
    {\
        timeout.tv_sec = now.tv_sec + iMilliSeconds/1000;\
        timeout.tv_nsec = now.tv_usec * 1000+(iMilliSeconds%1000)*1000000;\
        if(timeout.tv_nsec >= 1e9)\
        {\
            ++timeout.tv_sec;\
            timeout.tv_nsec -= (long)1e9;\
        }\
    }\
}

/**
 * @brief 下面是一些基础的定义，如常量，TExcept,断言异常类
 *
 */
 
//namespace QuickMDB{
        const MDB_UINT16 MDB_NTC_ZS_MAX_FILE_SIZE           =   80;
        const MDB_UINT16 MDB_NTC_ZS_MAX_PATH_SIZE           =   256;
        const MDB_UINT16 MDB_NTC_ZS_MAX_IPC_KEY_NAME_SIZE   =   512;
        const MDB_UINT16 MDB_NTC_ZS_MAX_PATH_FILE_SIZE      =   256;
        const MDB_UINT16 MDB_NTC_ZS_MAX_DATE_LONG_SIZE      =   20;  //2012-07-20 13:45:30
        const MDB_UINT16 MDB_NTC_ZS_MAX_DATE_SHORT_SIZE     =   15;  //20120720134530
        const MDB_UINT16 MDB_NTC_ZS_WM_USER                 =   0x0400;
        const MDB_INT32  MDB_NTC_ZS_ERROR_APP               =   -1;
        const MDB_INT32  MDB_NTC_ZS_SUCCESS_APP             =   0;

        const MDB_UINT16 MDB_NTC_ZS_SPLIT_INIT_BUFFER_LENGTH = 2048;
        const MDB_UINT16 MDB_NTC_ZS_SPLIT_INCR_FIELD_COUNT   = 50;
        const MDB_UINT16 MDB_NTC_ZS_SPLIT_INCR_BUFFER_LENGTH = 2048;

        const MDB_INT32 MDB_NTC_ZS_MAX_INT32   =  2147483647;
        const MDB_INT32 MDB_NTC_ZS_MIN_INT32   =  (-MDB_NTC_ZS_MAX_INT32-1);
        const MDB_UINT32 MDB_NTC_ZS_MAX_UINT32 =  4294967295U;
    #if defined(OS_WINDOWS) && _MSC_VER < 1300 //对于VC6.0不支持后缀LL
        const MDB_INT64 MDB_NTC_ZS_MAX_INT64   =  9223372036854775807L;
        const MDB_INT64 MDB_NTC_ZS_MIN_INT64   =  (-MDB_NTC_ZS_MAX_INT64 - 1L);
        const MDB_UINT64 MDB_NTC_ZS_MAX_UINT64 =  18446744073709551615UL;
    #else
        const MDB_INT64 MDB_NTC_ZS_MAX_INT64   =  9223372036854775807LL;
        const MDB_INT64 MDB_NTC_ZS_MIN_INT64   =  (-MDB_NTC_ZS_MAX_INT64 - 1L);
        const MDB_UINT64 MDB_NTC_ZS_MAX_UINT64 =  18446744073709551615ULL;
    #endif
        const char* const MDB_NTC_ZS_DEFAULT_INST_ENV_NAME = "CCB_INSTANCE_HOME";///< 默认的实例环境变量名称
        const char* const MDB_NTC_ZS_DEFAULT_LOG_PATH_NAME = "CCB_LOG_PATH";     ///< 默认的日志输出路径


        /**
         * @brief 判断是否是大端对齐
         *
         * @return bool
         * @retval true 是
         */
        inline bool MdbNtcIsBigEndian()
        {
            static unsigned int uiData = 0x12345678;
            return *(unsigned char*)(&uiData) == 0x12?true:false;
        }

        MDB_UINT64 mdb_ntc_htonl64(MDB_UINT64 host);
        //network to host long 64
        MDB_UINT64 mdb_ntc_ntohl64(MDB_UINT64 net);
        MDB_INT32 MdbNtcSnprintf(char *str, size_t size, const char *fmt, ...);
        char *MdbNtcStrcpy(char *dest, const char *src, size_t n);
		int MdbMaxIntArray(int* iArray, size_t n);

        /**
         * @brief 异常的基类
         *
         */
        class TMdbNtcException//:public TMdbNtcBaseObject
        {
//            MDB_ZF_DECLARE_OBJECT(TMdbNtcException);
        public:
            TMdbNtcException();
            TMdbNtcException(const char *pszFileName, int iLine, const char *pszFuncName);
            TMdbNtcException(const char *pszFileName, int iLine, const char *pszFuncName, const char * fmt, ...);            
            TMdbNtcException& Format(const char * fmt, ...);
            virtual ~TMdbNtcException();
            virtual const char * GetErrMsg() const;
        protected:
            TMdbNtcException& vFormat(const char * fmt, va_list ap);
        protected:
            const char *m_pszFileName;
            int m_iLine;
            const char *m_pszFuncName;
            char m_szErrMsg[4096];///< 错误信息
        };

#define MDB_NTC_ZF_ASSERT(condition) if(!(condition))\
    throw TMdbNtcException( __FILE__, __LINE__, __MDB_FUNC__, "Assert(%s)", #condition);

#if !(defined(OS_WINDOWS) && _MSC_VER < 1300)//针对大于VC6
#define MDB_NTC_ZF_THROW( Class, ... ) throw Class( __FILE__, __LINE__, __MDB_FUNC__, __VA_ARGS__ )
#endif

//数据库异常宏
#define MDB_NTC_ZF_DATABASE_THORW(ERR_CODE,STR_SQL,INFO) throw TDBException( __FILE__,__LINE__,__MDB_FUNC__,ERR_CODE,STR_SQL,INFO)
//#define ZF_DATABASE_QMDB_THROW(ERR_CODE,STR_SQL,INFO) throw TDBException( __FILE__,__LINE__,__FUNC__,ERR_CODE,STR_SQL,INFO)
//#define ZF_DATABASE_TT_THROW(ERR_CODE,STR_SQL,INFO) throw TDBException( __FILE__,__LINE__,__FUNC__,ERR_CODE,STR_SQL,INFO)
//#define ZF_DATABASE_MYSQL_THROW(ERR_CODE,STR_SQL,INFO) throw TDBException( __FILE__,__LINE__,__FUNC__,ERR_CODE,STR_SQL,INFO)
//#define ZF_DATABASE_ORACLE_THROW(ERR_CODE,STR_SQL,INFO) throw TDBException( __FILE__,__LINE__,__FUNC__,ERR_CODE,STR_SQL,INFO)

#define MDB_NTC_ZF_SNPRINTF /*QuickMDB::*/MdbNtcSnprintf
#define MDB_NTC_ZF_STRCPY /*QuickMDB::*/MdbNtcStrcpy
        inline const char* mdb_ntc_strnchr(const char* pszSrc, char cChar, int iLength = -1)
        {
            if(pszSrc == NULL || iLength == 0) return NULL;
            else if(iLength < 0) return strchr(pszSrc, cChar);
            const char* p = pszSrc;
            do
            {
                if(*p == cChar) return p;
                else ++p;
            } while (p-pszSrc < iLength);
            return NULL;
        }

        /**
         * @brief 跨平台精确时间（毫秒级）
         *
         */
        class TMdbNtcAccurateTime
        {
        public:
            /**
             * @brief 获得当前时间点，待到后面再次调用得到的结果相减，得到所经过时长(毫秒)
             *
             * @return TMdbNtcAccurateTime
             * @retval 获得当前时间点
             */
            inline static TMdbNtcAccurateTime GetCurTime()
            {
                TMdbNtcAccurateTime oTime;
            #ifdef OS_WINDOWS
                oTime.m_tTime = ::GetTickCount();
            #else
                gettimeofday(&oTime.m_tTime, NULL);
            #endif
                return oTime;
            }
            /**
             * @brief 判断两个时间是否相等
             *
             * @param oTime [in] 与之比较的时间
             * @return bool
             * @retval true 相等
             */
            inline bool operator == (const TMdbNtcAccurateTime& oTime) const
            {
            #ifdef OS_WINDOWS
                return m_tTime==oTime.m_tTime;
            #else
                return m_tTime.tv_sec==oTime.m_tTime.tv_sec && m_tTime.tv_usec==oTime.m_tTime.tv_usec;
            #endif
            }
            /**
             * @brief 判断两个时间是否不相等
             *
             * @param oTime [in] 与之比较的时间
             * @return bool
             * @retval true 不相等
             */
            inline bool operator != (const TMdbNtcAccurateTime& oTime) const
            {
            #ifdef OS_WINDOWS
                return m_tTime!=oTime.m_tTime;
            #else
                return m_tTime.tv_sec!=oTime.m_tTime.tv_sec || m_tTime.tv_usec!=oTime.m_tTime.tv_usec;
            #endif
            }
            /**
             * @brief 判断两个时间谁先谁后
             *
             * @param oTime [in] 与之比较的时间
             * @return bool
             * @retval true 相等
             */
            inline bool operator < (const TMdbNtcAccurateTime& oTime) const
            {
            #ifdef OS_WINDOWS
                return m_tTime<oTime.m_tTime;
            #else
                return m_tTime.tv_sec<oTime.m_tTime.tv_sec || (m_tTime.tv_sec==oTime.m_tTime.tv_sec && m_tTime.tv_usec<oTime.m_tTime.tv_usec);
            #endif
            }
            /**
             * @brief 判断两个时间谁先谁后
             *
             * @param oTime [in] 与之比较的时间
             * @return bool
             * @retval true 相等
             */
            inline bool operator <= (const TMdbNtcAccurateTime& oTime) const
            {
            #ifdef OS_WINDOWS
                return m_tTime<=oTime.m_tTime;
            #else
                return m_tTime.tv_sec<oTime.m_tTime.tv_sec || (m_tTime.tv_sec==oTime.m_tTime.tv_sec && m_tTime.tv_usec<=oTime.m_tTime.tv_usec);
            #endif
            }
            /**
             * @brief 判断两个时间谁先谁后
             *
             * @param oTime [in] 与之比较的时间
             * @return bool
             * @retval true 相等
             */
            inline bool operator > (const TMdbNtcAccurateTime& oTime) const
            {
            #ifdef OS_WINDOWS
                return m_tTime>oTime.m_tTime;
            #else
                return m_tTime.tv_sec>oTime.m_tTime.tv_sec || (m_tTime.tv_sec==oTime.m_tTime.tv_sec && m_tTime.tv_usec>oTime.m_tTime.tv_usec);
            #endif
            }
            /**
             * @brief 判断两个时间谁先谁后
             *
             * @param oTime [in] 与之比较的时间
             * @return bool
             * @retval true 相等
             */
            inline bool operator >= (const TMdbNtcAccurateTime& oTime) const
            {
            #ifdef OS_WINDOWS
                return m_tTime>=oTime.m_tTime;
            #else
                return m_tTime.tv_sec>oTime.m_tTime.tv_sec || (m_tTime.tv_sec==oTime.m_tTime.tv_sec && m_tTime.tv_usec>=oTime.m_tTime.tv_usec);
            #endif
            }
            /**
             * @brief 判断两个时间之间相隔的毫秒数
             *
             * @param oTime [in] 与之比较的时间
             * @return MDB_INT64
             * @retval >0 oTime早
             * @retval =0 二者时间相等
             * @retval <0 oTime迟
             */
            inline MDB_INT64 operator - (const TMdbNtcAccurateTime& oTime) const
            {
            #ifdef OS_WINDOWS
                return m_tTime-oTime.m_tTime;
            #else
                return ((MDB_INT64)(m_tTime.tv_sec-oTime.m_tTime.tv_sec) * 1000) + (m_tTime.tv_usec-oTime.m_tTime.tv_usec)/1000;
            #endif
            }
        protected:
        #ifdef OS_WINDOWS
            DWORD           m_tTime;///< 记录时间
        #else
            struct timeval  m_tTime;///< 记录时间
        #endif
        };

        /**
         * @brief 简易计时器（计时精度：WINDOWS平台毫秒级/非WINDOWS平台微秒级）
         *
         */
        class TMdbNtcSimpleTimer
        {
        public:
            TMdbNtcSimpleTimer() {
                Restart();
            }
            void Restart() {
              #ifdef OS_WINDOWS
                m_beginPoint = ::GetTickCount();
              #else
                gettimeofday(&m_beginPoint, NULL);
              #endif            
            }
            MDB_INT64 Elapsed() {
              #ifdef OS_WINDOWS
                return ( ::GetTickCount() - m_beginPoint );
              #else
                gettimeofday(&m_endPoint, NULL);
                return (m_endPoint.tv_sec - m_beginPoint.tv_sec) * 1000000 + (m_endPoint.tv_usec - m_beginPoint.tv_usec);
              #endif
            }
        private:
          #ifdef OS_WINDOWS
            DWORD m_beginPoint;
          #else
            struct timeval m_beginPoint,m_endPoint;
          #endif
        };

        /**
         * @brief 禁止复制的基类，只需要从其派生即可，省却了重复实现私有化拷贝构造函数和赋值操作符
         *
         */
        class TMdbNtcNoncopyable
        {
        protected:
            TMdbNtcNoncopyable(){}
            ~TMdbNtcNoncopyable(){}
        private:
            TMdbNtcNoncopyable(const TMdbNtcNoncopyable &);
            TMdbNtcNoncopyable & operator=(const TMdbNtcNoncopyable &);
        };

//}
#endif
