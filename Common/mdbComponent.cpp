#include "Common/mdbComponent.h"
//#include "Common/mdbLogInterface.h"
#include "Common/mdbFileUtils.h"
//namespace QuickMDB
//{
        #if 0
        TMdbNtcStringBuffer TMdbNtcParameter::GetLogPath(void)
        {
            TMdbNtcStringBuffer StrTemp;
            char *pEnv = getenv(MDB_NTC_ZS_DEFAULT_LOG_PATH_NAME);
            StrTemp.Snprintf(1024,"%s",pEnv?pEnv:".");
            if( !TMdbNtcDirOper::IsDirExist( StrTemp.c_str() ) )
            {
                TMdbNtcDirOper::MakeFullDir( StrTemp.c_str() );
            }
            if( StrTemp[StrTemp.GetLength()-1] == MDB_NTC_ZS_PATH_DELIMITATED_CHAR )
                StrTemp[StrTemp.GetLength()-1] = '\0';
            return StrTemp;
        }

        TMdbNtcStringBuffer TMdbNtcParameter::GetPerfStatPath(void)
        {
            TMdbNtcStringBuffer StrTemp;
            char *pEnv = getenv(MDB_NTC_ZS_DEFAULT_INST_ENV_NAME);
            StrTemp.Snprintf(1024,"%s"MDB_NTC_ZS_PATH_DELIMITATED"perf",pEnv?pEnv:".");
            if( !TMdbNtcDirOper::IsDirExist( StrTemp.c_str() ) )
            {
                TMdbNtcDirOper::MakeFullDir( StrTemp.c_str() );
            }
            return StrTemp;
        }
        #endif
        TMdbNtcStringBuffer TMdbNtcParameter::GetIPCKeyPath(const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {                        
            if(pszInstEnvName == NULL || *pszInstEnvName == '\0') pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME;
            char *pEnv = getenv(pszInstEnvName);
            return TMdbNtcStringBuffer(1024,"%s"MDB_NTC_ZS_PATH_DELIMITATED"sys"MDB_NTC_ZS_PATH_DELIMITATED"ipc",pEnv?pEnv:".");
        }

        TMdbNtcStringBuffer TMdbNtcParameter::GetNameObjectPath(const char* pszInstEnvName /* = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME */)
        {            
            if(pszInstEnvName == NULL || *pszInstEnvName == '\0') pszInstEnvName = MDB_NTC_ZS_DEFAULT_INST_ENV_NAME;
            char *pEnv = getenv(pszInstEnvName);
            return TMdbNtcStringBuffer(1024,"%s"MDB_NTC_ZS_PATH_DELIMITATED"sys"MDB_NTC_ZS_PATH_DELIMITATED"sno",pEnv?pEnv:".");
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcComponentObject, TMdbNtcBaseObject);
        TMdbNtcComponentObject::TMdbNtcComponentObject()
        {
            //m_pLogger = g_pMdbNtcGlobalLog;
            m_iLogCode = 0;
        }
        #if 0
        void TMdbNtcComponentObject::SetLogger(TMdbNtcBaseLog *pLogger)
        {
            m_pLogger = pLogger;
        }
        TMdbNtcBaseLog* TMdbNtcComponentObject::GetLogger() const
        {
            return m_pLogger;
        }
        #endif

//        MDB_ZF_IMPLEMENT_OBJECT(TSystemObject, TMdbNtcBaseObject);
        
//}
