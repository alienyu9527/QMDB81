#include "Dbflush/mdbShaowServerCtrl.h"
#include "Helper/mdbDateTime.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbXML.h"
#include "Helper/mdbShm.h"
#include "Helper/mdbStruct.h"
#include "Helper/mdbConfig.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{

    TShmMonitorInfo::TShmMonitorInfo()
    {
        Clear();
    }

    TShmMonitorInfo::~TShmMonitorInfo()
    {
    }

    void TShmMonitorInfo::Clear()
    {
        m_bRunFlag = false;
        m_iProcId= 0;
    }

    TMdbShadowSvrCtrl::TMdbShadowSvrCtrl()
    {
        m_iShmId = 0;
        m_pShmInfo = NULL;
        m_bAttach = false;
        memset(m_sDsn, 0 , sizeof(m_sDsn));
    }

    TMdbShadowSvrCtrl::~TMdbShadowSvrCtrl()
    {
    }

    int TMdbShadowSvrCtrl::Start(const char* psDsn)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        CHECK_OBJ(psDsn);

        SAFESTRCPY(m_sDsn, sizeof(m_sDsn), psDsn);
        TShadowCfgNode tCfgInfo;
        iRet = CheckDsnCfg(psDsn,tCfgInfo);
        if(iRet < 0 )
        {
            TADD_ERROR(ERROR_UNKNOWN,"dsn[%s] not found in config file.",psDsn);
            return -1;
        }    

        //int iMdbDsnId = tCfgInfo.m_iMdbDsnId;
        //dsn value hash获取修改
        MDB_INT64 iShmKey = SHADOW_SERVER_KEY + tCfgInfo.m_llMdbDsnId;
        int iShmSize = 1024;

        SHAMEM_T iShmID = INITVAl;
        if(TMdbShm::IsShmExist(iShmKey,iShmSize,iShmID) == true)
        {//共享内存已存在
            CHECK_RET(ERR_OS_SHM_EXIST,"share memory is exist,please check process already start");
        }
        
        CHECK_RET(m_tShmAlloc.CreateShm(iShmKey,iShmSize,m_iShmId),
                  "Can't create the shadow server shared memory, errno=%d[%s].",errno, strerror(errno));

        char *pAddr = NULL;
        CHECK_RET(m_tShmAlloc.AttachByID(m_iShmId, pAddr)," Can't attach shadow server share memory, errno=%d[%s].",errno, strerror(errno));
        m_bAttach = true;

        m_pShmInfo =  (TShmMonitorInfo * ) m_tShmAlloc.Allocate(sizeof(TShmMonitorInfo),m_tShmAlloc.m_pShmHead->m_iFirstInfoOffset);
        m_pShmInfo->Clear();

        if(StartProc(psDsn) < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Start shadow flush process failed.");
            TMdbShm::Destroy(m_iShmId);
            iRet = -1;
        }

        return iRet;
        
    }


    int TMdbShadowSvrCtrl::Stop(const char* psDsn)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        CHECK_OBJ(psDsn);

        SAFESTRCPY(m_sDsn, sizeof(m_sDsn), psDsn);
        TShadowCfgNode tCfgInfo;
        iRet = CheckDsnCfg(psDsn, tCfgInfo);
        if(iRet < 0 )
        {
            TADD_ERROR(ERROR_UNKNOWN,"dsn[%s] not found in config file.",psDsn);
            return -1;
        }    

        //int iMdbDsnId = tCfgInfo.m_iMdbDsnId;

        MDB_INT64 iShmKey = SHADOW_SERVER_KEY + tCfgInfo.m_llMdbDsnId;
        int iShmSize = 1024;

        SHAMEM_T iShmID = INITVAl;
        if(TMdbShm::IsShmExist(iShmKey,iShmSize,iShmID) == false)
        {//共享内存不存在
            CHECK_RET(ERR_OS_SHM_NOT_EXIST,"share memory not exist");
        }
        m_iShmId = iShmID;
        char *pAddr = NULL;
        CHECK_RET(m_tShmAlloc.AttachByID(m_iShmId, pAddr)," Can't attach shadow server share memory, errno=%d[%s].",errno, strerror(errno));
        m_pShmInfo = (TShmMonitorInfo * )(pAddr + m_tShmAlloc.m_pShmHead->m_iFirstInfoOffset);
        m_pShmInfo->m_bRunFlag = false;

        TMdbDateTime::Sleep(2);

        int iSleepCount = 0;
        do
        {
            iSleepCount ++;
            TMdbDateTime::Sleep(1);
            if(!TMdbOS::IsProcExistByPopen(m_pShmInfo->m_iProcId)){break;}
            TADD_NORMAL("wait process exit normal.");
        }while(iSleepCount < 30);

        
        //强制退出
        if(TMdbOS::IsProcExistByPopen(m_pShmInfo->m_iProcId))
        {
            TADD_WARNING("can't stop normal,kill it....");
            if(!TMdbOS::KillProc(m_pShmInfo->m_iProcId))
            {
                TADD_ERROR(ERROR_UNKNOWN,"Process  Can Not be Killed,Please Kill it manually.");
            }
        }

        // 销毁共享内存
        TMdbShm::Destroy(m_iShmId);
        return 0;

    }

    int TMdbShadowSvrCtrl::CheckDsnCfg(const char* psDsn, TShadowCfgNode& tCfgNode)
    {
        int iRet = 0;
        char *pszHome = getenv("QuickMDB_HOME");
        char sCfgName[MAX_PATH_NAME_LEN] = {0};
        char sCfgPath[MAX_PATH_NAME_LEN] = {0};

        SAFESTRCPY(sCfgPath,sizeof(sCfgPath),pszHome);
        if(sCfgPath[strlen(sCfgPath)-1] != '/')
        {
            sCfgPath[strlen(sCfgPath)] = '/';
        }
        snprintf(sCfgName,sizeof(sCfgName),"%setc/QuickMDB_ShadowServer.xml",sCfgPath);

        if(false == TMdbNtcFileOper::IsExist(sCfgName))
        {
            TADD_ERROR(ERROR_UNKNOWN,"config file[%s] not found",sCfgName);
            return -1;
        }

        bool bMdbFound = false;

        MDBXMLDocument tDoc(sCfgName);
        if (false == tDoc.LoadFile())
        {
            TADD_ERROR(ERROR_UNKNOWN,"Load file (name=%s) failed. Please check if the file conforms to the XML syntax rules.", sCfgName);
            return -1;
        }
        MDBXMLElement* pRoot = tDoc.FirstChildElement("ShadowServer");
        if(NULL == pRoot)
        {
            TADD_ERROR(ERR_APP_CONFIG_ITEM_NOT_EXIST,"ShadowServer node does not exist in the file[%s].",sCfgName);
            return ERR_APP_CONFIG_ITEM_NOT_EXIST;
        }

        for(MDBXMLElement* pEDS=pRoot->FirstChildElement("DataSource"); pEDS; pEDS=pEDS->NextSiblingElement("DataSource"))
        {
            MDBXMLAttribute* pAttr      = NULL;
            tCfgNode.Clear();
            for(pAttr=pEDS->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "mdb") == 0)
                {
                    strncpy(tCfgNode.m_sMdbDsn, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "dsn-id") == 0)
                {
                    //tCfgNode.m_iMdbDsnId = atoi(pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "type") == 0)
                {
                    strncpy(tCfgNode.m_sDBType, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "user") == 0)
                {
                    strncpy(tCfgNode.m_sDBUid, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "password") == 0)
                {
                    strncpy(tCfgNode.m_sDBPwd, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "db-tns") == 0)
                {
                    strncpy(tCfgNode.m_sDBTns, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Invalid element=[%s].", pAttr->Name());
                    return -1;
                }
            }

            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(tCfgNode.m_sMdbDsn, psDsn))
            {
                bMdbFound = true;
                break;
            }
        }

        if(false == bMdbFound)
        {
            TADD_ERROR(ERROR_UNKNOWN,"no mdb dsn[%s] found in config file",psDsn);
            return -1;
        }
        tCfgNode.m_llMdbDsnId = TMdbConfig::GetDsnHashValue(psDsn);
        return iRet;
    }

    int TMdbShadowSvrCtrl::Monitor()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(false == m_bAttach)
        {
            TADD_ERROR(ERROR_UNKNOWN,"not attach shm");
            return -1;
        }
        
        while(1)
        {
            TADD_FLOW("ShadowServer Process Auto-Checking......");
            TMdbDateTime::MSleep(1500);
            //判断是否结束
            if(m_pShmInfo->m_bRunFlag== false)
            {
                TADD_NORMAL("Monitor stop");
                break;
            }

            if(false == TMdbOS::IsProcExistByPopen(m_pShmInfo->m_iProcId))
            {
                TADD_NORMAL("process [pid:%d] not exist, restart it", m_pShmInfo->m_iProcId);
                StartProc(m_sDsn);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbShadowSvrCtrl::StartProc(const char* psDsn)
    {
        char sProcName[MAX_NAME_LEN] ={0};
        snprintf(sProcName, sizeof(sProcName), "mdbShadowFlush %s", m_sDsn);
        if(TMdbOS::IsProcExist(sProcName) ==  true)
        {
            //进程存在
            TADD_WARNING("Process=[%s] is running, can't restart.", sProcName);
            return 0;
        }
        else
        {
            //进程不存在
            system(sProcName);//启动进程
            TADD_DETAIL("system(%s) OK.",sProcName);
            TMdbDateTime::Sleep(1);
            //等待进程启动
            int iCounts = 0;
            while(true)
            {
                if(m_pShmInfo->m_bRunFlag== false)
                {
                    TADD_NORMAL("shadow server is stopping....");
                    return -1;
                }
                
                if(TMdbOS::IsProcExist(sProcName) ==  false )
                {
                    TMdbDateTime::Sleep(1);
                    if(iCounts%5 == 0)
                    {
                        TADD_NORMAL("Waiting for process=[%s] to start.", sProcName);
                    }
                    if(iCounts%31 == 30)
                    {
                        TADD_ERROR(ERR_SQL_EXECUTE_TIMEOUT,"Can't start process=[%s].", sProcName);
                        return ERR_SQL_EXECUTE_TIMEOUT;
                    }
                    ++iCounts;
                }
                else
                {
                    TADD_NORMAL("Start Process=[%s] success.",sProcName);
                    break;
                }
            }
        }


        return 0;
    }

    TShmMonitorInfo* TMdbShadowSvrCtrl::AttachSvrMgr(long long llDsnId)
    {
        TADD_FUNC("Start.");
        MDB_INT64 iShmKey = SHADOW_SERVER_KEY + llDsnId;
        int iShmSize = 1024;

        SHAMEM_T iShmID = INITVAl;
        if(TMdbShm::IsShmExist(iShmKey,iShmSize,iShmID) == false)
        {//共享内存不存在
            TADD_ERROR(ERROR_UNKNOWN,"share memory not exist");
            return NULL;
        }

        m_iShmId = iShmID;

        char *pAddr = NULL;
        if(m_tShmAlloc.AttachByID(m_iShmId, pAddr) < 0)
        {
            TADD_ERROR(ERROR_UNKNOWN," Can't attach shadow server share memory, errno=%d[%s].",errno, strerror(errno));
            return NULL;
        }
        m_pShmInfo = (TShmMonitorInfo * )(pAddr + m_tShmAlloc.m_pShmHead->m_iFirstInfoOffset);

        return m_pShmInfo;
    }

    TShadowCfgNode::TShadowCfgNode()
    {
        Clear();
    }

    TShadowCfgNode::~TShadowCfgNode()
    {
    }

    void TShadowCfgNode::Clear()
    {
        //m_iMdbDsnId = 0;
        m_llMdbDsnId = 0;
        memset(m_sMdbDsn, 0, sizeof(m_sMdbDsn));
        memset(m_sDBType, 0, sizeof(m_sDBType));
        memset(m_sDBUid, 0, sizeof(m_sDBUid));
        memset(m_sDBPwd, 0, sizeof(m_sDBPwd));
        memset(m_sDBTns, 0, sizeof(m_sDBTns));
    }

//}
