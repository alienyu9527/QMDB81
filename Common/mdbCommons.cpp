#include "Common/mdbCommons.h"
#ifdef OS_WINDOWS
    #pragma comment(lib, "WS2_32.LIB")
#else
    #include <sys/types.h>
    #include <netinet/in.h>
#endif
#include <stdarg.h>
#include <ctype.h>
//namespace QuickMDB
//{


        MDB_UINT64 mdb_ntc_htonl64(MDB_UINT64 host)
        {
            static unsigned int uiData = 0x12345678;
            if(*(unsigned char*)(&uiData) == 0x12)
            {
                return host;
            }
            MDB_UINT64   ret = 0;   
            MDB_UINT32   high,low;
            low   =   ((MDB_UINT32)host) & 0xFFFFFFFF;
            high   =  ((MDB_UINT32)(host >> 32)) & 0xFFFFFFFF;
            low   =   htonl(low);
            high   =   htonl(high);
            ret   =   low;
            ret   <<= 32; 
            ret   |=   high;
            return   ret;
            
        }
        //network to host MDB_UINT64
        MDB_UINT64 mdb_ntc_ntohl64(MDB_UINT64 net)
        {   
            static unsigned int uiData = 0x12345678;
            if(*(unsigned char*)(&uiData) == 0x12)
            {
                return net;
            }
            MDB_UINT64   ret = 0;
            MDB_UINT32   high,low;
            low   =   ((MDB_UINT32)net) & 0xFFFFFFFF;
            high   =  ((MDB_UINT32)(net >> 32)) & 0xFFFFFFFF;
            low   =   ntohl(low);
            high   =   ntohl(high);
            ret   =   low;
            ret   <<= 32;
            ret   |=   high;
            return   ret;
        }
        
        MDB_INT32 MdbNtcSnprintf(char *str, size_t size, const char *fmt, ...)
        {
            if(size == 0) return 0;
            va_list ap; 
            va_start (ap,fmt); 
            MDB_INT32 iRet = vsnprintf( str, size, fmt, ap ); 
            va_end (ap);
#ifdef OS_WINDOWS
            str[ size - 1 ] = '\0';
#endif
            return iRet == (int)strlen( str ) ? iRet : -1;
        }

        char *MdbNtcStrcpy(char *dest, const char *src, size_t n)
        {
            if( NULL == src || NULL == dest || 0 == n )
                return NULL;
            if(n > 0)
            {
                memccpy( dest, src, 0x00, n );
                dest[n-1]='\0';
            }
            return dest;
        }

		int MdbMaxIntArray(int* iArray, size_t n)
		{
		    int iMax = iArray[0];
			for(int i=1;i<n;i++)
			{
				if(iArray[i]>iMax) iMax=iArray[i];
			}
			return iMax;
		}
		
//        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcException, TMdbNtcBaseObject);
        
        TMdbNtcException::TMdbNtcException()
        {
            *m_szErrMsg = '\0';
            m_iLine = 0;
            m_pszFileName = "";
            m_pszFuncName = "";
        }

        TMdbNtcException::TMdbNtcException(const char *pszFileName, int iLine, const char *pszFuncName)
            :m_pszFileName(pszFileName),m_iLine(iLine),m_pszFuncName(pszFuncName)
        {
        }

        TMdbNtcException::TMdbNtcException(const char *pszFileName, int iLine, const char *pszFuncName, const char * fmt, ...)
            :m_pszFileName(pszFileName),m_iLine(iLine),m_pszFuncName(pszFuncName)
        {
            *m_szErrMsg = '\0';
            if(fmt)
            {
                va_list ap;
                va_start(ap,fmt);
                vFormat(fmt, ap);
                va_end (ap);
            }
        }
        
        TMdbNtcException::~TMdbNtcException()
        {
        }

        TMdbNtcException& TMdbNtcException::Format(const char * fmt, ...)
        {
            *m_szErrMsg = '\0';
            if(fmt)
            {
                va_list ap;
                va_start(ap,fmt);
                vFormat(fmt, ap);
                va_end (ap);
            }
            return *this;
        }

        TMdbNtcException& TMdbNtcException::vFormat(const char * fmt, va_list ap)
        {
            snprintf(m_szErrMsg, sizeof(m_szErrMsg), "[%20s:%5d] ", m_pszFileName, m_iLine);            
            if(fmt)
            {
                size_t uiLength = strlen(m_szErrMsg);
                vsnprintf(m_szErrMsg+uiLength, sizeof(m_szErrMsg)-uiLength-1, fmt, ap);
            }
            m_szErrMsg[sizeof(m_szErrMsg)-1] = '\0';
            return *this;
        }

        const char* TMdbNtcException::GetErrMsg() const
        {
            return m_szErrMsg;
        }

//}
       
