/****************************************************************************************
*@Copyrights  2007，中兴软创（南京）计算机有限公司 开发部 CCB项目组
*@                   All rights reserved.
*@Name：	    mdbConfig.cpp
*@Description： minidb的配置文件控制类
*@Author:		li.shugang
*@Date：	    2008年11月25日
*@History:
******************************************************************************************/
#include "Helper/mdbConfig.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbFileList.h"
#include "Helper/mdbEncrypt.h"
#include "Common/mdbIniFiles.h"

//#include "BillingSDK.h"


#include <stdio.h>
#ifdef WIN32
#pragma warning(disable:4305)
#pragma warning(disable:4101)
#endif

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{




    TMdbCfgDSN::TMdbCfgDSN()
    {
        Clear();
    }


    TMdbCfgDSN::~TMdbCfgDSN()
    {
        Clear();
    }

    void TMdbCfgDSN::Init()
    {
        iLogLevel = 0;
        iLogTime = 3;
		iLogCount = 1;
        iKillTime = 3;
        iDelayTime = 10;
        iCleanTime = 1;
        iRepType  = 1;
        iOraRepCounts = 1;
        bIsRep    = false; 
        bIsOraRep = false; 
        bIsPeerRep = false;
        bIsCaptureRouter = false;
        iLogBuffSize = 256;
        iLogFileSize = 128;
        memset(sLocalIP,  0, sizeof(sLocalIP));
		memset(sLocalIP_active,  0, sizeof(sLocalIP_active));
        memset(sPeerIP,  0, sizeof(sPeerIP));
        memset(sActiveIP,  0, sizeof(sActiveIP));
        memset(sStandByIP,  0, sizeof(sStandByIP));
        iLocalPort = 19801;
	    iAgentPort[0] = 19804;
		for(int i = 1; i<MAX_AGENT_PORT_COUNTS; i++)
		{
			iAgentPort[i] = 0;
			iNtcPort[i] = 0;
			iNoNtcPort[i] = 0;
		}
		memset(sAgentPortStr, 0, sizeof(sAgentPortStr));
		SAFESTRCPY(sAgentPortStr, sizeof(sAgentPortStr), "19804");
        iLocalPort = -1;
        iPeerPort = -1;
        iActivePort = -1;
        iStandbyPort = -1;

        iManagerSize = 256*1024*1024;
        iDataSize = 1024*1024*1024;
        iClientTimeout = 3;
        m_iOraRepInterval = 5;
        m_iOraRepDelaySec = 600;
        m_iSeqCacheSize = 0;
        m_bIsDiskStorage = false;
        m_iRepFileTimeout = 24;
        m_bShadow = false;
        m_bSingleDisaster = false;
		m_bIsShardBackup = false;
        m_bNull = false;
        m_iLoadPriority = LOAD_FILE_FIRST;

		m_bUseNTC =  true;
        m_iCSPumpInitCount = 1;
        m_iCSPumpMaxCount = 100;
        m_iCSPeerCountPerPump = 50;
        m_sCSValidIP[0] = '\0';;
        m_sCSInValidIP[0] = '\0';;
        
        m_bSQLIsAnswer = true;

        memset(sRepSvrIp, 0, sizeof(sRepSvrIp));
        memset(sRepStandbySvrIp, 0, sizeof(sRepStandbySvrIp));
        //memset(sRepLocalIp, 0, sizeof(sRepLocalIp));

        iRepSvrPort = 0;
        iRepStandbySvrPort = 0;
        iRepLocalPort = 0;
        iInvalidRepFileTime = 0;

        memset(sRepFilePath, 0, sizeof(sRepFilePath));

		m_bReloadOra = false;
    	m_bReloadEncrypt = false;
    	m_sReloadCfgName[0] = '\0';
    	m_sReloadDbType[0] = '\0';
        
    }

    void TMdbCfgDSN::Clear()
    {
        memset(sName, 0, sizeof(sName));
        //iValue = -1;
        llValue = -1;
		
		for(int i = 0; i<MAX_AGENT_PORT_COUNTS; i++)
		{
			iAgentPort[i] = 0;
			iNtcPort[i] = 0;
			iNoNtcPort[i] = 0;
		}
		memset(sAgentPortStr, 0, sizeof(sAgentPortStr));
		memset(sNtcPortStr, 0, sizeof(sNtcPortStr));
		memset(sNoNtcPortStr, 0, sizeof(sNoNtcPortStr));

        iLogBuffSize = 128;
        if(NULL == getenv("QuickMDB_HOME"))
        {
            printf("ERROR : not find env [QuickMDB_HOME].\n");
            return;
        }
        sprintf(sLogDir, "%s/mdb/log/", getenv("QuickMDB_HOME"));
        iLogFileSize = 64;

        iRepBuffSize = 128;
        sprintf(sRepDir, "%s/mdb/rep/", getenv("QuickMDB_HOME"));
        iRepFileSize = 64;

        iCaptureBuffSize = 128;
        sprintf(sCaptureDir, "%s/mdb/capture/", getenv("QuickMDB_HOME"));
        iCaptureFileSize = 64;

        iRedoBuffSize = 128;
        sprintf(sRedoDir, "%s/.data/redo/", getenv("QuickMDB_HOME"));
        iRedoFileSize = 64;

        sprintf(sDataStore, "%s/", getenv("QuickMDB_HOME"));
        iPermSize    = 256;
        
        sprintf(sStorageDir,"%s/.data/storage/",getenv("QuickMDB_HOME"));

        memset(sLocalIP,  0, sizeof(sLocalIP));
		memset(sLocalIP_active,  0, sizeof(sLocalIP_active));
        memset(sPeerIP,  0, sizeof(sPeerIP));
        memset(sActiveIP,  0, sizeof(sActiveIP));
        memset(sStandByIP,  0, sizeof(sStandByIP));

        memset(sOracleID,  0, sizeof(sOracleID));
        memset(sOracleUID, 0, sizeof(sOracleUID));
        memset(sOraclePWD, 0, sizeof(sOraclePWD));
        memset(checkDataByOra,0,sizeof(checkDataByOra));
        memset(checkDataByMdb,0,sizeof(checkDataByMdb));
        memset(errorLog,0,sizeof(errorLog));
        memset(sRoutingList, 0, sizeof(sRoutingList));
        memset(sRoutingName, 0, sizeof(sRoutingName));

        iLogTime = 0;
		iLogCount = 1;
        iManagerSize = 256*1024*1024;
        iDataSize = 1024*1024*1024;
        iAgentThreadStackSize = 50;
        iLoadThreadStackSize = 10;


        iLogLevel = 0;             //日志级别
        bIsRep    = false;         //是否开启主备同步
        bIsOraRep = false;          //是否开启Oracle的同步
        bIsPeerRep = false;
        bIsCaptureRouter = false;
        // bIsLoadView = true;    //是否支持多表关联上载
        bIsReadOnlyAttr = false;
        iKillTime = 3;
        iRepType  = 1;
        iCleanTime = 1;

        iOraRepCounts = 1;
        iNetDropTime  = 30;

        // iDisTributeAccessAttribute = -1;
        cType = MDB_DS_TYPE_FILE;

        m_iOraRepInterval = 5;
        m_iOraRepDelaySec = 60;
        m_iSeqCacheSize = 0;
        m_bIsDiskStorage = false;
        m_iLongSqlTime = 3;
        m_iRepFileTimeout = 24;
        m_bShadow = false;
        m_bSingleDisaster = false;
		m_bIsShardBackup = false;
        m_bNull = false;
        m_iLoadPriority = LOAD_FILE_FIRST;

		m_bUseNTC =  true;
        m_iCSPumpInitCount = 1;
        m_iCSPumpMaxCount = 100;
        m_iCSPeerCountPerPump = 50;
        m_sCSValidIP[0] = '\0';;
        m_sCSInValidIP[0] = '\0';;
        m_bSQLIsAnswer = true;
        sprintf(sRepFilePath, "%s/mdb/rep_shardbak/", getenv("QuickMDB_HOME"));
        memset(sRepSvrIp, 0, sizeof(sRepSvrIp));
        memset(sRepStandbySvrIp, 0, sizeof(sRepStandbySvrIp));
        //memset(sRepLocalIp, 0, sizeof(sRepLocalIp));
        m_strNotLoadFromDBList = "";
        m_vNotLoadFromDBTab.clear();

        iRepSvrPort = 0;
        iRepStandbySvrPort = 0;
        iRepLocalPort = 0;
        iInvalidRepFileTime = 0;

		m_bReloadOra = false;
    	m_bReloadEncrypt = false;
    	m_sReloadCfgName[0] = '\0';
    	m_sReloadDbType[0] = '\0';
    }


    void TMdbCfgDSN::Print()
    {
        TADD_DETAIL("[DSN]");

        TADD_DETAIL("    Name = %s", sName);
        //TADD_DETAIL("    Value= %d", iValue);
        TADD_DETAIL("    llValue= %lld", llValue);
        TADD_DETAIL("    LogBuffSize = %d(M)", iLogBuffSize);
        TADD_DETAIL("    LogDir      = %s", sLogDir);
        TADD_DETAIL("    LogFileSize = %d(M)", iLogFileSize);

        TADD_DETAIL("    RepBuffSize = %d(M)", iRepBuffSize);
        TADD_DETAIL("    RepDir      = %s", sRepDir);
        TADD_DETAIL("    RepFileSize = %d(M)", iRepFileSize);

        TADD_DETAIL("    DataStore   = %s", sDataStore);
        TADD_DETAIL("    PermSize    = %d(M)", iPermSize);

        TADD_DETAIL("    OracleID    = %s", sOracleID);
        TADD_DETAIL("    OracleUID   = %s", sOracleUID);
        TADD_DETAIL("    sOraclePWD  = %s", sOraclePWD);

        TADD_DETAIL("    cType  = %c", cType);

        TADD_DETAIL("    iLogLevel   = %d", iLogLevel);
        TADD_DETAIL("    bIsRep      = %s", bIsRep?"TRUE":"FALSE");
        TADD_DETAIL("    bIsReadOnly = %s", bIsReadOnlyAttr?"TRUE":"FALSE");
        TADD_DETAIL("    bIsOraRep   = %s", bIsOraRep?"TRUE":"FALSE");
        TADD_DETAIL("    bIsCaptureRouter  = %s", bIsCaptureRouter?"TRUE":"FALSE");
        // TADD_DETAIL("    bIsLoadView   = %s", bIsLoadView?"TRUE":"FALSE");
        TADD_DETAIL("    iKillTime   = %d", iKillTime);
        TADD_DETAIL(" iOraRepCounts  = %d", iOraRepCounts);
        TADD_DETAIL("  iNetDropTime  = %d", iNetDropTime);
        // TADD_DETAIL("iDisTributeAttr = %d", iDisTributeAccessAttribute);
        TADD_DETAIL("iManagerSize = %lu", iManagerSize);
        TADD_DETAIL("iDataSize = %lu", iDataSize);
        TADD_DETAIL("iAgentThreadStackSize = %ld", iAgentThreadStackSize);
        TADD_DETAIL("iLoadThreadStackSize = %ld", iLoadThreadStackSize);
        TADD_DETAIL("iClientTimeout = %d", iClientTimeout);
        TADD_DETAIL("orarep-interval = %d", m_iOraRepInterval);
        TADD_DETAIL("orarep-delaytime = %d", m_iOraRepDelaySec);
        TADD_DETAIL("Is-Seq-Cache = %d", m_iSeqCacheSize);
        TADD_DETAIL("iRepFileTimeout = %d", m_iRepFileTimeout);
        TADD_DETAIL("is-shadow =%s", m_bShadow?"TRUE":"FALSE");
        TADD_DETAIL("is-single-disaster =%s", m_bSingleDisaster?"TRUE":"FALSE");
        TADD_DETAIL("is-shard-backup =%s", m_bIsShardBackup?"TRUE":"FALSE");
        TADD_DETAIL("is-null =%s", m_bNull?"TRUE":"FALSE");

		TADD_DETAIL("Is-Reload-Ora =%s", m_bReloadOra?"TRUE":"FALSE");
    	TADD_DETAIL("Is-Reload-Encrypt =%s", m_bReloadEncrypt?"TRUE":"FALSE");
    	TADD_DETAIL("Reload-Cfg-Name =%s", m_sReloadCfgName);
    	TADD_DETAIL("Reload-Db-Type =%s", m_sReloadDbType);
		
        TADD_DETAIL("[/DSN]");
    }


    TMdbConfig::TMdbConfig()
    {
        memset(m_sCfgFile,           0, sizeof(m_sCfgFile));
        memset(m_sTabCfgFile,           0, sizeof(m_sTabCfgFile));
        

        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            m_pTable[i] = NULL;
            m_pTableSpace[i] = NULL;
            //m_pOldTabStruct[i] = NULL;
        }

        for(int i=0; i<MAX_SEQUENCE_COUNTS; ++i)
        {
            m_pSeq[i] = NULL;
        }

        for(int i=0; i<MAX_USER_COUNT; ++i)
        {
            m_pUser[i] = new(std::nothrow) TMDbUser();
            if(m_pUser[i] == NULL)
            {
                TADD_ERROR(ERROR_UNKNOWN, "Mem Not Enough");
                return;
            }
            m_pUser[i]->Clear();
        }

        m_iUserCounts = 0;
        m_iTableCounts = 0;
        m_iTableSpaceCounts = 0;
        m_bCreateFlag = false;
        m_bStartFlushFromDbProc = false;
        m_bStartDbRepProc = false;
        m_bStartShardBackupProc = false;
        m_bStartFileStorageProc = false;
        m_pDBLink  = NULL;
        m_cAccess = MDB_ADMIN;
        memset(m_sOnsiteFile,0,sizeof(m_sOnsiteFile));
        m_tTsAlterInfo.Clear();
        m_vTableAlterInfo.clear();

    }


    TMdbConfig::~TMdbConfig()
    {
        TADD_FUNC("Start.");
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            SAFE_DELETE(m_pTable[i]);
            SAFE_DELETE(m_pTableSpace[i]);
        }

        TMdbTable* pOldTable = NULL;
        std::vector<TMdbTable*>::iterator itor = m_vpOldTabStruct.begin();
        for(; itor != m_vpOldTabStruct.end(); ++itor)
        {
            pOldTable = *itor;
            SAFE_DELETE(pOldTable);
        }
        
        for(int i=0; i<MAX_SEQUENCE_COUNTS; ++i)
        {
            SAFE_DELETE(m_pSeq[i]);
        }

        for(int i=0; i<MAX_USER_COUNT; ++i)
        {
            SAFE_DELETE(m_pUser[i]);
        }
        TADD_FUNC("Finish.");
    }


    /******************************************************************************
    * 函数名称	:  Init()
    * 函数描述	:  初始化配置文件
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::Init()
    {
        TADD_FUNC("Start.");
        m_bCreateFlag = false;
        TADD_FUNC("Finish.");
        return 0;
    }
	
	int TMdbConfig::ParseAgentPort()
	{
		int iRet = 0;
		if(m_tDsn.sAgentPortStr[0] == 0) 
		{
			m_tDsn.iAgentPort[0] = 19804;
			for(int i = 1; i<MAX_AGENT_PORT_COUNTS; i++)
			{
				m_tDsn.iAgentPort[i] = 0;
			}
			memset(m_tDsn.sAgentPortStr, 0, sizeof(m_tDsn.sAgentPortStr));
			SAFESTRCPY(m_tDsn.sAgentPortStr, sizeof(m_tDsn.sAgentPortStr), "19804");
		}
		else
		{
			char sAgentPort[64] = {0};
			SAFESTRCPY(sAgentPort, 64, m_tDsn.sAgentPortStr);
			TMdbNtcSplit tSplit;
			tSplit.SplitString(sAgentPort, ',');
			if(tSplit.GetFieldCount() <= 0)
			{
				TADD_ERROR(ERR_NET_IP_INVALID, "Too few agent port value!");
				return ERR_NET_IP_INVALID;
			}
			else if(tSplit.GetFieldCount() > MAX_AGENT_PORT_COUNTS)
			{
				TADD_ERROR(ERR_NET_IP_INVALID, "Too many agent port value!");
				return ERR_NET_IP_INVALID;
			}
			else
			{
				char sTempPort[16] = {0};
				for(unsigned int i = 0; i<tSplit.GetFieldCount(); i++)
				{
					memset(sTempPort, 0, sizeof(sTempPort));
					SAFESTRCPY(sTempPort, sizeof(sTempPort), tSplit[i]);
					TMdbNtcStrFunc::Trim(sTempPort, ' ');
					m_tDsn.iAgentPort[i] = TMdbNtcStrFunc::StrToInt(sTempPort);//代理端口
					if(m_tDsn.iAgentPort[i] <= 0)
					{
						TADD_ERROR(ERR_NET_IP_INVALID, "Invalid agent port value!");
						return ERR_NET_IP_INVALID;
					}
					TADD_DETAIL("m_tDsn.iAgentPort[%d] = [%d]", i, m_tDsn.iAgentPort[i]);
				}
			}
		}
		return iRet;
	}

	int TMdbConfig::ParseNtcAgentPort(char * sPortStr,int * iPortArray)
	{
		int iRet = 0;
		if(sPortStr[0] == 0) 
		{
			
		}
		else
		{
		    char sAgentPort[64] = {0};
			SAFESTRCPY(sAgentPort, 64, sPortStr);
			TMdbNtcSplit tSplit;
			tSplit.SplitString(sAgentPort, ',');
			if(tSplit.GetFieldCount() <= 0)
			{
				TADD_ERROR(ERR_NET_IP_INVALID,"Too few agent port value!");
				return ERR_NET_IP_INVALID;
			}
			else if(tSplit.GetFieldCount() > MAX_AGENT_PORT_COUNTS)
			{
				TADD_ERROR(ERR_NET_IP_INVALID,"Too many agent port value!");
				return ERR_NET_IP_INVALID;
			}
			else
			{
				char sTempPort[16] = {0};
				for(int i = 0; i<tSplit.GetFieldCount(); i++)
				{
					memset(sTempPort, 0, sizeof(sTempPort));
					SAFESTRCPY(sTempPort, sizeof(sTempPort), tSplit[i]);
					TMdbNtcStrFunc::Trim(sTempPort, ' ');
					iPortArray[i] = TMdbNtcStrFunc::StrToInt(sTempPort);//代理端口
					if(iPortArray[i] <= 0)
					{
						TADD_ERROR(ERR_NET_IP_INVALID,"Invalid agent port value!");
						return ERR_NET_IP_INVALID;
					}
					TADD_DETAIL("iPortArray[%d] = [%d]", i, iPortArray[i]);
				}
			}
		}
		return iRet;
	}
    /******************************************************************************
    * 函数名称	:  SetFlag()
    * 函数描述	:  设置动作标识，是否需要检测记录数。仅供mdbCtrl调用
    * 输入		:  bFlag， 如果需要检测记录数,则为true,否则为false
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbConfig::SetFlag(bool bFlag)
    {
        m_bCreateFlag = bFlag;
    }

    /******************************************************************************
    * 函数名称	:  GetConfigFileName()
    * 函数描述	:  获取系统配置文件名
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  系统配置文件名
    * 作者		:  li.shugang
    *******************************************************************************/
    char * TMdbConfig::GetConfigFileName()
    {
        return m_sCfgFile;
    }

    /******************************************************************************
    * 函数名称	:  GetIsStartFlushFromOra()
    * 函数描述	:  是否启动从oracle同步数据进程
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  是返回true，否则返回false
    * 作者		:  li.shugang
    *******************************************************************************/
    bool TMdbConfig::GetIsStartFlushFromOra()
    {
        return m_bStartFlushFromDbProc;
    }

    /******************************************************************************
    * 函数名称	:  GetIsStartOracleRep()
    * 函数描述	:  获取是否向oracle同步数据进程
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  是返回true，否则返回false
    * 作者		:  li.shugang
    *******************************************************************************/
    bool TMdbConfig::GetIsStartOracleRep()
    {
        return m_bStartDbRepProc;
    }

    bool TMdbConfig::GetIsStartShardBackupRep()
    {
        return m_bStartShardBackupProc;
    }

    bool TMdbConfig::GetIsStartFileStorageProc()
    {
        return m_bStartFileStorageProc;
    }


    /******************************************************************************
    * 函数名称	:  LoadCfg()
    * 函数描述	:  装载某个DSN的配置
    * 输入		:  pszDsn, DSN名称
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadCfg(const char* pszDsn)
    {
        strncpy(m_sDSN, pszDsn, sizeof(m_sDSN)-1);
        TMdbNtcStrFunc::ToUpper(m_sDSN);
        m_iTableCounts = 0;
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            SAFE_DELETE(m_pTable[i]);
        }

        TMdbTable* pOldTable = NULL;
        std::vector<TMdbTable*>::iterator itor = m_vpOldTabStruct.begin();
        for(; itor != m_vpOldTabStruct.end(); ++itor)
        {
            pOldTable = *itor;
            SAFE_DELETE(pOldTable);
        }
        m_vpOldTabStruct.clear();
        
        //检查对应的DSN  的XML文件是否存在，表的struct和property配置文件是否匹配
        if (false == IsExist(pszDsn) )
        {
            return ERR_APP_TABLE_XML_NOT_MATCH;
        }

        int iRet = 0;
        CHECK_RET(LoadSysCfg(m_sCfgFile),"LoadSysCfg failed.");//读取系统配置

        // 读取表配置
        CHECK_RET(LoadTable()," Load Table failed.");

        // 读取备份的上一次create成功后的表配置文件。for file storage
        CHECK_RET(LoadTableOldConfig()," Load Table failed.");
        
        //上载系统表
        CHECK_RET(LoadSysTable()," LoadSysTable failed.");
        //加载%DSN%_onsite.xml文件
        CHECK_RET(LoadOnSiteFile(pszDsn),"LoadOnSiteFile failed.");
        //检查配置信息
        CHECK_RET(CheckCfg(),"CheckCfg() failed.");
        //上载job信息
        CHECK_RET(LoadMdbJob(),"LoadMdbJob failed.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  LoadOnSiteFile()
    * 函数描述	:  加载onsite配置文件
    * 输入		:  pszDsn DSN实例名
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbConfig::LoadOnSiteFile(const char* pszDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pszDsn);
        char   dsnName[MAX_NAME_LEN] = {0};
        memset(dsnName,0,sizeof(dsnName));
        if(m_szHome[0] == 0)
        {
            if(SetConfigHomePath(pszDsn) != 0){return -1;}
        }
        //TMdbNtcStrFunc::ToUpper(pszDsn,dsnName);
        SAFESTRCPY(dsnName, sizeof(dsnName), pszDsn);
        TMdbNtcStrFunc::ToUpper(dsnName);
        snprintf(m_sOnsiteFile,sizeof(m_sOnsiteFile),"%s%s_onsite.xml", m_szHome,dsnName);
        //如果onsite文件不存在，则不加载
        iRet = access(m_sOnsiteFile, F_OK);
        if(iRet == 0)
        {
            MDBXMLDocument tDoc(m_sOnsiteFile);
            if (!tDoc.LoadFile())
            {
                TADD_ERROR(ERROR_UNKNOWN,"Load onsite configuration failed.");
                return -1;
            }
            MDBXMLElement* pRoot = tDoc.FirstChildElement("MDBConfig");
            if(NULL == pRoot)
            {
                TADD_ERROR(ERR_APP_CONFIG_ITEM_NOT_EXIST,"MDBConfig node does not exist in onsite configuration.");
                return ERR_APP_CONFIG_ITEM_NOT_EXIST;
            }
            CHECK_RET(LoadSysInfo(pRoot,true),"Failed to load the onsite configuration.",m_sOnsiteFile);
            //加载table节点
            MDBXMLElement* pEle = pRoot->FirstChildElement("table");
            if(NULL == pEle)
            {
                TADD_ERROR(ERR_APP_CONFIG_ITEM_NOT_EXIST,"sys node does not exist,please check the DSN configuration.");
                return ERR_APP_CONFIG_ITEM_NOT_EXIST;
            }
            TMdbTable* pTable = NULL;
            MDBXMLAttribute* pAttr      = NULL;
            MDBXMLAttribute* pAttrValue = NULL;
            for (MDBXMLElement* pSec=pEle->FirstChildElement("record-counts"); pSec; 
                                        pSec=pSec->NextSiblingElement("record-counts"))
            {
                pAttr  = pSec->FirstAttribute();
                pAttrValue = pAttr->Next();
                if(pAttrValue != NULL)
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0 
                        && TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Name(), "value") == 0)
                    {
                        pTable = GetTableByName(pAttr->Value());
						if(pTable == NULL)
						{
							TADD_ERROR(ERR_APP_INVALID_PARAM,"Cannot find table [%s]. Set record counts failed!", pAttr->Value());
							continue;
						}
                        pTable->iRecordCounts = pTable->iExpandRecords + atoi(pAttrValue->Value());
                        if(pTable->iRecordCounts  < 10000)
                        {
                            pTable->iRecordCounts = 10000;
                        }
                        //pTable->iCounts = pTable->iRecordCounts;
                    }
                    else
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM," Invalid element=[%s].",  pAttr->Value());
                        return ERR_APP_INVALID_PARAM;
                    }
                }
            }
        }
        return 0;
    }

    /******************************************************************************
    * 函数名称	:  LoadSysCfg()
    * 函数描述	:  装载系统配置
    * 输入		:  cSysCfgFile, 系统配置文件
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadSysCfg(const char* cSysCfgFile,const bool bCheck)
    {
        MDBXMLDocument tDoc(cSysCfgFile);
        if (false == tDoc.LoadFile())
        {
            TADD_ERROR(ERROR_UNKNOWN,"Load sys configuration failed.");
            return -1;
        }
        MDBXMLElement* pRoot = tDoc.FirstChildElement("MDBConfig");
        if(NULL == pRoot)
        {
            TADD_ERROR(ERR_APP_CONFIG_ITEM_NOT_EXIST,"MDBConfig node does not exist in sys configuration.");
            return ERR_APP_CONFIG_ITEM_NOT_EXIST;
        }
        //数据源信息
        int iRet = 0;
        CHECK_RET(LoadDsnInfo(pRoot,bCheck),"LoadDsnInfo() failed.");
        //加载系统配置信息
        CHECK_RET(LoadSysInfo(pRoot,bCheck),"LoadSysInfo() failed.");
        //加载用户信息
        CHECK_RET(LoadUser(pRoot),"LoadUser() failed.");
        //加载表空间信息
        CHECK_RET(LoadTableSpaceCfg(pRoot,bCheck)," LoadTableSpaceCfg() failed.");

        if(0 != LoadTableSpaceAlterInfo(m_szHome))
        {
            TADD_ERROR(-1," Get TableSpace Alter info failed.");
            return -1;
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  LoadUser()
    * 函数描述	:  读取 用户 配置
    * 输入		:  pMDB user节点
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadUser(MDBXMLElement* pMDB)
    {
        int iPos = 0;
        m_iUserCounts = 0;
        for(MDBXMLElement* pEle=pMDB->FirstChildElement("user"); pEle; pEle=pEle->NextSiblingElement("user"))
        {
            MDBXMLAttribute* pAttr = NULL;
            if(iPos >= MAX_USER_COUNT)
            {
                TADD_ERROR(ERR_DB_MAX_USER_NUMBER,"The number of users reached a maximum value[64],please check.");
                return ERR_DB_MAX_USER_NUMBER;
            }
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                {
                    strncpy(m_pUser[iPos]->sUser, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "password") == 0)
                {
                    if(!TMdbEncrypt::IsEncryptStr(pAttr->Value()))
                    {
                        strncpy(m_pUser[iPos]->sPwd, pAttr->Value(), MAX_NAME_LEN-1);
                    }
                    else
                    {
                        TMdbEncrypt::DecryptEx(const_cast<char *>(pAttr->Value()),m_pUser[iPos]->sPwd);
                    }
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "access") == 0)
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "A") == 0)
                    {
                        SAFESTRCPY(m_pUser[iPos]->sAccess,sizeof(m_pUser[iPos]->sAccess),"Administrator");
                    }
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "W") == 0)
                    {
                        SAFESTRCPY(m_pUser[iPos]->sAccess,sizeof(m_pUser[iPos]->sAccess),"Read-Write");
                    }
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "R") == 0)
                    {
                        SAFESTRCPY(m_pUser[iPos]->sAccess,sizeof(m_pUser[iPos]->sAccess),"ReadOnly");
                    }
                    else
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid Access=[%s].",  pAttr->Value());
                        return ERR_APP_INVALID_PARAM;
                    }
                }
                else
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s].", pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }
            ++iPos;
            ++m_iUserCounts;
        }
        return 0;
    }
    //end dongchun modify

    /******************************************************************************
    * 函数名称  :	LoadDsnCfg()
    * 函数描述  :	读取XML中的数据源信息
    * 输入 	 :	pRoot XML文件根节点
    * 输出 	 :	无
    * 返回值	 :	成功加载返回true，否则返回false
    * 作者 	 :	li.shugang
    *******************************************************************************/
    bool TMdbConfig::LoadDsnCfg(MDBXMLElement* pRoot)
    {
        m_tDsn.Clear();
        m_tDsn.bIsLicense = true;
        MDBXMLAttribute* pAttr = NULL;
        MDBXMLElement* pEle=pRoot->FirstChildElement("DataSource");

        for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
        {
        	/*
            if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
            {
                strncpy(m_tDsn.sName, pAttr->Value(), MAX_NAME_LEN-1);
            }
            else
            */
            if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "type") == 0)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "oracle") == 0)
                {
                    m_tDsn.cType = MDB_DS_TYPE_ORACLE;
                }
                 else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "mysql") == 0)
                {
                    m_tDsn.cType = MDB_DS_TYPE_MYSQL;
                }
                else
                {
                    TADD_ERROR(ERROR_UNKNOWN, "Invalid DS-Type=[%s].",pAttr->Value());
                    return false;
                }
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "user") == 0)
            {
                strncpy(m_tDsn.sOracleUID, pAttr->Value(), MAX_NAME_LEN-1);
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "password") == 0)
            {
                if(!TMdbEncrypt::IsEncryptStr(pAttr->Value()))
                {
                    strncpy(m_tDsn.sOraclePWD, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else
                {
                    TMdbEncrypt::DecryptEx(const_cast<char *>(pAttr->Value()),m_tDsn.sOraclePWD);
                }
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "db-id") == 0)
            {
                strncpy(m_tDsn.sOracleID, pAttr->Value(), MAX_NAME_LEN-1);
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN, "Invalid element=[%s].", pAttr->Name());
                return false;
            }
        }

    	strncpy(m_tDsn.sName, m_sDSN, MAX_NAME_LEN-1);
        //对参数进行校验
        if (m_tDsn.sOracleID[0] == '\0')
        {
            TADD_WARNING_NO_SCREEN("Config file=[%s] Not find ORA_ID.",m_sCfgFile);
            //return false;
        }

        if(m_tDsn.sOraclePWD[0] == '\0')
        {
            TADD_WARNING_NO_SCREEN(" Config file=[%s] Not find ORA_PWD.", m_sCfgFile);
            //return false;
        }

        if(m_tDsn.sOracleUID[0] == '\0')
        {
            TADD_WARNING_NO_SCREEN("Config file=[%s] Not find ORA_UID.",m_sCfgFile);
            //return false;
        }
        return true;
    }


    /******************************************************************************
    * 函数名称	:  LoadDsnInfo()
    * 函数描述	:  读取数据源信息
    * 输入		:  pEle DataSource节点
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadDsnInfo(MDBXMLElement* pEle,const bool bCheck)
    {
        MDBXMLElement* pEDS = pEle->FirstChildElement("DataSource");
        if(NULL == pEDS)
        {
            TADD_ERROR(ERR_APP_CONFIG_ITEM_NOT_EXIST,"DataSource node does not exist,please check the DSN configuration.");
            return ERR_APP_CONFIG_ITEM_NOT_EXIST;
        }
        MDBXMLAttribute* pAttr      = NULL;
        for(pAttr=pEDS->FirstAttribute(); pAttr; pAttr=pAttr->Next())
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
            {
                strncpy(m_tDsn.sName, pAttr->Value(), MAX_NAME_LEN-1);
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "type") == 0)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "oracle") == 0)
                {
                    m_tDsn.cType = MDB_DS_TYPE_ORACLE;
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "mysql") == 0)
                {
                    m_tDsn.cType = MDB_DS_TYPE_MYSQL;
                }
                else
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Invalid DS-Type=[%s].",pAttr->Value());
                    return -1;
                }
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "user") == 0)
            {
                strncpy(m_tDsn.sOracleUID, pAttr->Value(), MAX_NAME_LEN-1);
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "password") == 0)
            {
                if(!TMdbEncrypt::IsEncryptStr(pAttr->Value()))
                {
                    strncpy(m_tDsn.sOraclePWD, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else
                {
                    TMdbEncrypt::DecryptEx(const_cast<char *>(pAttr->Value()),m_tDsn.sOraclePWD);
                }
            }
            else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "db-id") == 0)
            {
                strncpy(m_tDsn.sOracleID, pAttr->Value(), MAX_NAME_LEN-1);
            }
            else
            {
                TADD_ERROR(ERROR_UNKNOWN,"Invalid element=[%s].", pAttr->Name());
                return -1;
            }
        }
        if(!bCheck) return 0;
        //对参数进行校验
        if (m_tDsn.sOracleID[0] == '\0')
        {
            TADD_WARNING_NO_SCREEN("Not find ORA_ID in sys configuration.");
            //return -1;
        }

        if(m_tDsn.sOraclePWD[0] == '\0')
        {
            TADD_WARNING_NO_SCREEN("Not find ORA_PWD in sys configuration.");
            //return -1;
        }

        if(m_tDsn.sOracleUID[0] == '\0')
        {
            TADD_WARNING_NO_SCREEN("Not find ORA_UID in sys configuration.");
            //return -1;
        }
        //DSN配置文件中配置的DSN名称和提供的配置文件是否一致
        if(TMdbNtcStrFunc::StrNoCaseCmp(m_tDsn.sName,m_sDSN) != 0)
        {
            TADD_ERROR(ERR_DB_DSN_INVALID,"DataSource node name property is configured incorrectly,it must be configured to %s instead of %s.",\
                m_sDSN,m_tDsn.sName);
            return ERR_DB_DSN_INVALID;
        }
        return 0;
    }

    void TMdbConfig::LoadFromDBCfg(const char *sAttrValue,std::vector<std::string> &vLoadFromDB)
    {
        if(sAttrValue[0] != '\0')
        {
            TMdbNtcSplit tSplit;
            tSplit.SplitString(sAttrValue,',');
            for(unsigned int i=0; i<tSplit.GetFieldCount(); ++i)
            {
                if(tSplit[i][0] != 0)
                {
                    vLoadFromDB.push_back(tSplit[i]);
                }
            }
            
        }
        
        
    }

	/******************************************************************************
	* 函数名称  :  ReLoadOracle
	* 函数描述  : oracle 二次登录校验
	* 输入      :  
	* 输入      :  
	* 输出      :  
	* 返回值    : 
	* 作者      :  
	*******************************************************************************/
	int TMdbConfig::ReLoadOracle()
	{
	    if(!m_tDsn.m_bReloadOra) return 0;
	    if(m_tDsn.m_sReloadCfgName[0] == '\0' ||m_tDsn.m_sReloadDbType[0] == '\0')
	    {
	        TADD_ERROR(ERR_OS_OPEN_FILE,"Config file Not find Reload-Cfg-Name or Reload-Db-Type when setting Is-Reload-Ora.");
	        return ERR_OS_OPEN_FILE;
	    }
	    //读取二次登录密码
	    TMdbNtcReadIni  cfg;
	    char sOraUID[MAX_NAME_LEN*2] = {0};
	    char sOraPWD[MAX_NAME_LEN*2] = {0};
		if(false == cfg.OpenFile(m_tDsn.m_sReloadCfgName))
        {
            TADD_ERROR(ERR_OS_OPEN_FILE,"Open file[%s] failed.",m_tDsn.m_sReloadCfgName);
			return ERR_OS_OPEN_FILE;
        }

		TADD_NORMAL("ReLoadOracle:open file ok:%s",m_tDsn.m_sReloadCfgName);
		
		//TMdbNtcStringBuffer sItem = ReadIni.ReadString(sDsn,"IP","");
	    TMdbNtcStringBuffer sItemName = cfg.ReadString(m_tDsn.m_sReloadDbType, "USER_NAME", "");
	    TMdbNtcStringBuffer sItemPwd = cfg.ReadString(m_tDsn.m_sReloadDbType, "PASS_WORD",  "");
		
		strncpy(sOraUID,sItemName.c_str(),MAX_NAME_LEN*2-1);
		strncpy(sOraPWD,sItemPwd.c_str(),MAX_NAME_LEN*2-1);
		
	    if(m_tDsn.m_bReloadEncrypt)
	    {//有加密需求，需要解密
	       cfg.Decrypt(sOraUID);
	       cfg.Decrypt(sOraPWD);
	    }
	    if(sOraUID[0] != 0 && sOraPWD[0] != 0)
	    {
	        strncpy(m_tDsn.sOracleUID, sOraUID, MAX_NAME_LEN-1);
	        strncpy(m_tDsn.sOraclePWD, sOraPWD, MAX_NAME_LEN-1);
	    }

		TADD_NORMAL("ReLoadOracle:the user name :%s",m_tDsn.sOracleUID);
		TADD_NORMAL("ReLoadOracle:the pass word :%s",m_tDsn.sOraclePWD);
		
		cfg.CloseFile();
	    return 0;
	}

	
	int TMdbConfig::LoadNtcPortsInfo(MDBXMLAttribute* pAttr,MDBXMLAttribute* pAttrValue)
	{
		int iRet = 0;
		if(TMdbNtcStrFunc::StrNoCaseCmp("use-ntc-agent-port",pAttr->Value()) == 0)
		{
			if (0 == TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Value(),""))
			{
				memset(m_tDsn.sNtcPortStr,0,sizeof(m_tDsn.sNtcPortStr));
			}
			else
			{
				strncpy(m_tDsn.sNtcPortStr,pAttrValue->Value(),sizeof(m_tDsn.sNtcPortStr)-1);
			}
			CHECK_RET(ParseNtcAgentPort(m_tDsn.sNtcPortStr,m_tDsn.iNtcPort),"Invalid use ntc agent port value");
			
		}
	
		if(TMdbNtcStrFunc::StrNoCaseCmp("notuse-ntc-agent-port",pAttr->Value()) == 0)
		{
			if (0 == TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Value(),""))
			{
				memset(m_tDsn.sNoNtcPortStr,0,sizeof(m_tDsn.sNoNtcPortStr));
			}
			else
			{
				strncpy(m_tDsn.sNoNtcPortStr,pAttrValue->Value(),sizeof(m_tDsn.sNoNtcPortStr)-1);
			}
			CHECK_RET(ParseNtcAgentPort(m_tDsn.sNoNtcPortStr,m_tDsn.iNoNtcPort),"Invalid not use ntc agent port value");
			
		}

		if(TMdbNtcStrFunc::StrNoCaseCmp("is-use-ntc",pAttr->Value()) == 0)
		{
			if (0 == TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Value(),""))
			{
				m_tDsn.m_bUseNTC = false;
			}
			else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Value(),"Y"))
			{
				m_tDsn.m_bUseNTC = true;
			}
			else if(0 == TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Value(),"N"))
			{
				m_tDsn.m_bUseNTC = false;
			}
			else
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s=%s].",pAttr->Value(),pAttrValue->Value());
                return ERR_APP_INVALID_PARAM;
            }
				
			
		}
	
		return iRet;
					
	}
	int TMdbConfig::LoadSysInfoFromXML(MDBXMLElement* pESys)
	{
		MDBXMLAttribute* pAttr		= NULL;
		MDBXMLAttribute* pAttrValue = NULL;
		int iRet = 0;
		for (MDBXMLElement* pSec=pESys->FirstChildElement("section"); pSec; pSec=pSec->NextSiblingElement("section"))
		{
			pAttr  = pSec->FirstAttribute();
			pAttrValue = pAttr->Next();
			if(pAttrValue != NULL)
			{
				if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0 
					&& TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Name(), "value") == 0)
				{
					// dsn 
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iLogLevel,pAttr->Value(),"log-level",pAttrValue->Value(),0);
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iLogTime,pAttr->Value(),"log-time",pAttrValue->Value(),3);
					if(m_tDsn.iLogTime < 1)
					{
						m_tDsn.iLogTime = 1;
					}
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iLogCount,pAttr->Value(),"log-count",pAttrValue->Value(),1);
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iRepType,pAttr->Value(),"Rep-Type",pAttrValue->Value(),1);
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iKillTime,pAttr->Value(),"kill-time",pAttrValue->Value(),3);
					SET_SYS_PARAM_INT_VALUE(m_tProAttr.iHeartBeatWarning,pAttr->Value(),"HeartBeatWarning",pAttrValue->Value(),10);
					SET_SYS_PARAM_INT_VALUE(m_tProAttr.iHeartBeatFatal,pAttr->Value(),"HeartBeatFatal",pAttrValue->Value(),30);
					//SET_SYS_PARAM_INT_VALUE(m_tDsn.iAgentPort,pAttr->Value(),"agent-port",pAttrValue->Value(),19804);
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iClientTimeout,pAttr->Value(),"client-timeout",pAttrValue->Value(),3);
		
					SET_SYS_PARAM_INT_VALUE(m_tDsn.m_iCSPumpInitCount,pAttr->Value(),"CsPumpInitCount",pAttrValue->Value(),1); 
					SET_SYS_PARAM_INT_VALUE(m_tDsn.m_iCSPumpMaxCount,pAttr->Value(),"CsPumpMaxCount",pAttrValue->Value(),100); 
					SET_SYS_PARAM_INT_VALUE(m_tDsn.m_iCSPeerCountPerPump,pAttr->Value(),"CsPeerCountPerPump",pAttrValue->Value(),50); 
					SET_SYS_PARAM_CHAR_VALUE(m_tDsn.m_sCSValidIP,sizeof(m_tDsn.m_sCSValidIP),pAttr->Value(),"valid-ip",pAttrValue->Value());
					SET_SYS_PARAM_CHAR_VALUE(m_tDsn.m_sCSInValidIP,sizeof(m_tDsn.m_sCSInValidIP),pAttr->Value(),"invalid-ip",pAttrValue->Value());
		
					SET_SYS_BOOL_VALUE(m_tDsn.m_bSQLIsAnswer,pAttr->Value(),"Is-Answer",pAttrValue->Value(),true);
					if(TMdbNtcStrFunc::StrNoCaseCmp("agent-port",pAttr->Value()) == 0)
					{
						if (0 == TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Value(),""))
						{
							memset(m_tDsn.sAgentPortStr,0,sizeof(m_tDsn.sAgentPortStr));
						 }
						else
						{
							strncpy(m_tDsn.sAgentPortStr,pAttrValue->Value(),sizeof(m_tDsn.sAgentPortStr)-1);
						}
						CHECK_RET(ParseAgentPort(),"Invalid agent port value");
						continue;
					}
					
					if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "buf-size") == 0)
					{
						m_tDsn.iLogBuffSize = atoi(pAttrValue->Value());
						m_tDsn.iRepBuffSize = atoi(pAttrValue->Value());
						m_tDsn.iCaptureBuffSize = atoi(pAttrValue->Value());
						m_tDsn.iRedoBuffSize = atoi(pAttrValue->Value());
						continue;
					 }
					else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "file-size") == 0)
					{
						m_tDsn.iLogFileSize = atoi(pAttrValue->Value());
						m_tDsn.iRepFileSize = atoi(pAttrValue->Value());
						m_tDsn.iCaptureFileSize = atoi(pAttrValue->Value());
						m_tDsn.iRedoFileSize	= atoi(pAttrValue->Value());
						continue;
					}
					else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "file-path") == 0)
					{
						snprintf(m_tDsn.sLogDir,sizeof(m_tDsn.sLogDir), "%s/oracle_sync", pAttrValue->Value());
						snprintf(m_tDsn.sRepDir,sizeof(m_tDsn.sRepDir),  "%s/rep_sync", pAttrValue->Value());
						snprintf(m_tDsn.sDataStore,sizeof(m_tDsn.sDataStore),  "%s/data_store", pAttrValue->Value());
						snprintf(m_tDsn.checkDataByOra,sizeof(m_tDsn.checkDataByOra),  "%s/check_data_ora", pAttrValue->Value());
						snprintf(m_tDsn.checkDataByMdb,sizeof(m_tDsn.checkDataByMdb),  "%s/check_data_mdb", pAttrValue->Value());
						snprintf(m_tDsn.errorLog,sizeof(m_tDsn.errorLog),  "%s/Error_log", pAttrValue->Value());
						snprintf(m_tDsn.sCaptureDir,sizeof(m_tDsn.sCaptureDir),  "%s/capture_sync/", pAttrValue->Value());
						//snprintf(m_tDsn.sStorageDir,sizeof(m_tDsn.sStorageDir),  "%s/storage/", pAttrValue->Value());
						//snprintf(m_tDsn.sRedoDir,sizeof(m_tDsn.sRedoDir),  "%s/redo/", pAttrValue->Value());
						snprintf(m_tDsn.sRepFilePath,sizeof(m_tDsn.sRepFilePath),  "%s/rep_shardbak/", pAttrValue->Value());
						continue;
					}
					else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "routing-list") == 0)
					{
						if (0 == TMdbNtcStrFunc::StrNoCaseCmp(pAttrValue->Value(), ""))
						{
							memset(m_tDsn.sRoutingList, 0, MAX_ROUTER_LIST_LEN);
						}
						else
						{
							snprintf(m_tDsn.sRoutingList, MAX_ROUTER_LIST_LEN, "-1,%s", pAttrValue->Value());
						}
						continue;
					}
					if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "not-load-table-list") == 0)
					{
						 LoadFromDBCfg(pAttrValue->Value(),m_tDsn.m_vNotLoadFromDBTab);
						 continue;
					}
					
					SET_SYS_PARAM_SIZE_VALUE(m_tDsn.iManagerSize,pAttr->Value(),"manager-size",pAttrValue->Value(),256);
					SET_SYS_PARAM_SIZE_VALUE(m_tDsn.iDataSize,pAttr->Value(),"data-size",pAttrValue->Value(),1024);
					SET_PARAM_BOOL_VALUE(m_tDsn.bIsReadOnlyAttr,pAttr->Value(),"is-ReadOnlyAttr",pAttrValue->Value());
					SET_SYS_PARAM_INT_VALUE(m_tDsn.m_iSeqCacheSize,pAttr->Value(),"Is-Seq-Cache",pAttrValue->Value(),10000);
					SET_SYS_PARAM_INT_VALUE(m_tDsn.m_iLongSqlTime,pAttr->Value(),"long-sql-time",pAttrValue->Value(),3); //超时sql时间
					SET_SYS_PARAM_INT_VALUE(m_tDsn.m_iRepFileTimeout,pAttr->Value(),"rep-file-timeout",pAttrValue->Value(),24); //同步文件没有处理超时时间
					SET_PARAM_BOOL_VALUE(m_tDsn.m_bNull,pAttr->Value(),"is-null",pAttrValue->Value());
					SET_SYS_PARAM_CHAR_VALUE(m_tDsn.sLocalIP,sizeof(m_tDsn.sLocalIP),pAttr->Value(),"Local-Ip",pAttrValue->Value());
					SET_SYS_PARAM_CHAR_VALUE(m_tDsn.sLocalIP_active,sizeof(m_tDsn.sLocalIP_active),pAttr->Value(),"Local-Active-Ip",pAttrValue->Value());
		
					// load priority
					SET_SYS_PARAM_SIZE_VALUE(m_tDsn.m_iLoadPriority,pAttr->Value(),"Load-priority",pAttrValue->Value(),1);
		
					// db rep
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iDelayTime,pAttr->Value(),"Delay-Time",pAttrValue->Value(),10);
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iCleanTime,pAttr->Value(),"Clean-Time",pAttrValue->Value(),1);
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iOraRepCounts,pAttr->Value(),"ora-rep-counts",pAttrValue->Value(),1);
					SET_PARAM_BOOL_VALUE(m_tDsn.bIsOraRep,pAttr->Value(),"is-ora-rep",pAttrValue->Value());
					SET_SYS_PARAM_INT_VALUE(m_tDsn.m_iOraRepInterval,pAttr->Value(),"orarep-interval",pAttrValue->Value(),5);
					SET_SYS_PARAM_INT_VALUE(m_tDsn.m_iOraRepDelaySec,pAttr->Value(),"orarep-delaytime",pAttrValue->Value(),60);
					SET_PARAM_BOOL_VALUE(m_tDsn.m_bShadow,pAttr->Value(),"is-shadow",pAttrValue->Value());
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iLoadThreadStackSize,pAttr->Value(),"Load_ThreadStackSize",pAttrValue->Value(),10);
		
		
					// route capture
					SET_SYS_PARAM_CHAR_VALUE(m_tDsn.sRoutingName,sizeof(m_tDsn.sRoutingName),pAttr->Value(),"Routing-Name",pAttrValue->Value());
					SET_PARAM_BOOL_VALUE(m_tDsn.bIsCaptureRouter,pAttr->Value(),"is-capture-router",pAttrValue->Value());
		
					// file storage
					SET_PARAM_BOOL_VALUE(m_tDsn.m_bIsDiskStorage,pAttr->Value(),"is-disk-storage",pAttrValue->Value());
		
					// shard-backup
					SET_PARAM_BOOL_VALUE(m_tDsn.bIsPeerRep,pAttr->Value(),"is-Peer-rep",pAttrValue->Value());
					SET_PARAM_BOOL_VALUE(m_tDsn.bIsRep,pAttr->Value(),"is-Rep",pAttrValue->Value());
					SET_PARAM_BOOL_VALUE(m_tDsn.m_bIsShardBackup,pAttr->Value(),"is-shard-backup",pAttrValue->Value());
					SET_SYS_PARAM_CHAR_VALUE(m_tDsn.sRepSvrIp,sizeof(m_tDsn.sRepSvrIp),pAttr->Value(),"Rep-Server-ip",pAttrValue->Value());
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iRepSvrPort,pAttr->Value(),"Rep-Server-port",pAttrValue->Value(),-1);
					SET_SYS_PARAM_CHAR_VALUE(m_tDsn.sRepStandbySvrIp,sizeof(m_tDsn.sRepStandbySvrIp),pAttr->Value(),"Rep-standby-Server-ip",pAttrValue->Value());
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iRepStandbySvrPort,pAttr->Value(),"Rep-standby-Server-port",pAttrValue->Value(),-1);
					//SET_SYS_PARAM_CHAR_VALUE(m_tDsn.sRepLocalIp,sizeof(m_tDsn.sRepLocalIp),pAttr->Value(),"Rep-Local-Ip",pAttrValue->Value());
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iRepLocalPort,pAttr->Value(),"Rep-Local-port",pAttrValue->Value(),-1);
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iInvalidRepFileTime,pAttr->Value(),"Rep-file-invalid-time",pAttrValue->Value(),3);
					///////////////////////////////////////////////////////////////////////////////////////////
					// define ,but not be used
						  
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iNetDropTime,pAttr->Value(),"Net-Drop-Time",pAttrValue->Value(),30);
					SET_SYS_PARAM_INT_VALUE(m_tDsn.iAgentThreadStackSize,pAttr->Value(),"Agent_ThreadStackSize",pAttrValue->Value(),50);
		
					//二次登陆校验参数
					SET_PARAM_BOOL_VALUE(m_tDsn.m_bReloadOra,pAttr->Value(),"Is-Reload-Ora",pAttrValue->Value());
					SET_SYS_PARAM_CHAR_VALUE(m_tDsn.m_sReloadCfgName,sizeof(m_tDsn.m_sReloadCfgName),pAttr->Value(),"Reload-Cfg-Name",pAttrValue->Value());
					SET_SYS_PARAM_CHAR_VALUE(m_tDsn.m_sReloadDbType,sizeof(m_tDsn.m_sReloadDbType),pAttr->Value(),"Reload-Db-Type",pAttrValue->Value());
					SET_PARAM_BOOL_VALUE(m_tDsn.m_bReloadEncrypt,pAttr->Value(),"Is-Reload-Encrypt",pAttrValue->Value());
		
					CHECK_RET(LoadNtcPortsInfo(pAttr,pAttrValue),"Load use-ntc-agent-port or notuse-ntc-agent-port failed");
						  
				}
				else
				{
					TADD_ERROR(ERR_APP_INVALID_PARAM," Invalid element=[%s].",  pAttr->Value());
					return ERR_APP_INVALID_PARAM;
				}
				
			}
			
		}
		snprintf(m_tDsn.sStorageDir, sizeof(m_tDsn.sStorageDir), "%s/.data/%s/storage/", getenv("QuickMDB_HOME"),m_sDSN);
		snprintf(m_tDsn.sRedoDir,sizeof(m_tDsn.sRedoDir),  "%s/.data/%s/redo/", getenv("QuickMDB_HOME"),m_sDSN);
		return iRet;
	}
    /******************************************************************************
    * 函数名称	:  LoadSysInfo()
    * 函数描述	:  读取sys节点数据源信息
    * 输入		:  pEle sys节点
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadSysInfo(MDBXMLElement* pEle,const bool bCheck)
    {
        int iRet = 0;
        MDBXMLElement* pESys = pEle->FirstChildElement("sys");
        if(NULL == pESys)
        {
            TADD_ERROR(ERR_APP_CONFIG_ITEM_NOT_EXIST,"sys node does not exist,please check the DSN configuration files.");
            return ERR_APP_CONFIG_ITEM_NOT_EXIST;
        }

		iRet = LoadSysInfoFromXML(pESys);
		if(iRet != 0)
			return iRet;
		


		if(m_tDsn.sLocalIP[0] == 0 && m_tDsn.sLocalIP_active[0] != 0)
			strncpy(m_tDsn.sLocalIP,m_tDsn.sLocalIP_active,sizeof(m_tDsn.sLocalIP));
		
        m_tDsn.llValue = GetDsnHashValue(m_sDSN);
		
		//读取oracle 二次登录参数
    	iRet = ReLoadOracle();
		if(iRet != 0)
			return iRet;
		
        //对数据进行校验
        if(bCheck)
        {
            CHECK_RET(VerifySysCfg(),"VerifySysCfg failed.");
        }
        return iRet;
    }

    long long TMdbConfig::GetDsnHashValue(const char *sDsn)
    {
        char sQuickMDBHome[MAX_PATH_NAME_LEN]={0};
        char sLowerDsn[MAX_NAME_LEN]={0};
        char *pszHome = getenv("QuickMDB_HOME");
        if(NULL == pszHome || NULL == sDsn)
        {
            return -1;
        }
        SAFESTRCPY(sQuickMDBHome,sizeof(sQuickMDBHome),pszHome);
        SAFESTRCPY(sLowerDsn,sizeof(sLowerDsn),sDsn);
        TMdbNtcStrFunc::ToLower(sQuickMDBHome);
        TMdbNtcStrFunc::ToLower(sLowerDsn);
        long long llQuickMDBHome = TMdbNtcStrFunc::StrToHash(sQuickMDBHome);
        long long llDsn = TMdbNtcStrFunc::StrToHash(sLowerDsn);
        long long llRet = llQuickMDBHome*3 + llDsn*1317;
        if(llRet < 0 ) llRet = -llRet;
        return llRet%MANAGER_KEY;
        
    }

    int TMdbConfig::BackUpConfig()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        
        // setup bak path
        char sBakPath[MAX_PATH_NAME_LEN] ={0}; 
        snprintf(sBakPath, sizeof(sBakPath), "%s.BAK", m_szHome);
        
        char sBakBakPath[MAX_PATH_NAME_LEN] ={0}; // 备份路径的备份
        
        if(TMdbNtcDirOper::IsExist(sBakPath)) // 存在，先把原来的备份文件备份。
        {
            snprintf(sBakBakPath, MAX_PATH_NAME_LEN, "%s_BAK",sBakPath );
            if(TMdbNtcDirOper::IsExist(sBakBakPath))
            {
                TMdbNtcDirOper::Remove(sBakBakPath, true);
            }
            
            if(!TMdbNtcDirOper::Rename(sBakPath, sBakBakPath, TMdbNtcPathOper::IsSameFileSystem(sBakPath, sBakBakPath)))
            {
                TADD_ERROR(-1, "Rename path[%s] to [%s] failed.", sBakPath, sBakBakPath);
                return ERR_OS_BACKUP_FILE;
            }
        }

        if(!TMdbNtcDirOper::MakeFullDir(sBakPath))
        {
            TADD_ERROR(-1, "create path[%s] failed.", sBakPath);
            return ERR_OS_CREATE_DIR;
        }

        // 备份QuickMDB_SYS_DSN.xml
        char sBakFile[MAX_FILE_NAME] ={0};
        snprintf(sBakFile, sizeof(sBakFile), "%s/.QuickMDB_SYS_%s.xml", sBakPath, m_sDSN);
        if(!TMdbNtcFileOper::Copy(m_sCfgFile, sBakFile))
        {
            TADD_ERROR(-1, "copy sys config file[%s] failed.", m_sCfgFile);
            return ERR_OS_OPEN_FILE;
        }

        // 备份分片备份本地配置
        char sShBFile[MAX_FILE_NAME] = {0};
        snprintf(sShBFile, MAX_FILE_NAME, "%s.ShardBackupRep.xml",m_szHome);
        if(TMdbNtcFileOper::IsExist(sShBFile))
        {
            char sBakShBFile[MAX_FILE_NAME] = {0};
            snprintf(sBakShBFile, MAX_FILE_NAME, "%s.ShardBackupRep.xml",sBakPath);
            if(!TMdbNtcFileOper::Copy(sShBFile, sBakShBFile))
            {
                TADD_ERROR(-1, "copy table config file[%s] failed.", sShBFile);
                return ERR_OS_OPEN_FILE;
            }
        }

        // 备份各个表配置文件
        char sTabDir[MAX_PATH_NAME_LEN] = {0};
        snprintf(sTabDir, sizeof(sTabDir), "%s.TABLE", m_szHome);
        
        TMdbNtcFileScanner tDirScan;
        if(!tDirScan.ScanDir(sTabDir))
        {   
            TADD_ERROR(-1,"Get Table info failed.");
            return ERR_OS_OPEN_DIR;
        }

        const char* psTabDir = NULL;
        char sTabName[MAX_NAME_LEN] = {0};
        char sTabConfigFile[MAX_FILE_NAME] ={0};
        char sDestBakPath[MAX_PATH_NAME_LEN] ={0};
        char sDestBakFile[MAX_FILE_NAME] = {0};
        char sTabBakPath[MAX_PATH_NAME_LEN] = {0};
        snprintf(sTabBakPath, MAX_PATH_NAME_LEN, "%s/.TABLE", sBakPath);
        while((psTabDir = tDirScan.GetNext()) != NULL)
        {
            memset(sTabName, 0, sizeof(sTabName));    
            if( GetTableNameFromDir(psTabDir,sTabName,sizeof(sTabName)) < 0)
            {
                iRet = ERROR_UNKNOWN;
                break;
            }
            TMdbNtcStrFunc::ToUpper(sTabName);

            // 建目录
            memset(sDestBakPath, 0, MAX_PATH_NAME_LEN);
            snprintf(sDestBakPath, sizeof(sDestBakPath), "%s/.%s",sTabBakPath, sTabName);
            if(!TMdbNtcDirOper::MakeFullDir(sDestBakPath))		        
            {		           
                TADD_ERROR(-1, "create path[%s] failed.", sDestBakPath);		            
                return ERR_OS_CREATE_DIR;		        
            }
            
            // 备份文件
            memset(sTabConfigFile, 0, MAX_FILE_NAME);
            snprintf(sTabConfigFile, sizeof(sTabConfigFile), "%s/.Tab_%s_%s.xml",psTabDir, m_sDSN, sTabName);
            memset(sDestBakFile, 0, MAX_FILE_NAME);
            snprintf(sDestBakFile, sizeof(sDestBakFile), "%s/.Tab_%s_%s.xml",sDestBakPath, m_sDSN, sTabName);

            if(!TMdbNtcFileOper::Copy(sTabConfigFile, sDestBakFile))
            {
                TADD_ERROR(-1, "copy table config file[%s] failed.", sTabConfigFile);
                return ERR_OS_OPEN_FILE;
            }

        }

        // 删除上一次的备份sBakBakPath
        if(sBakBakPath[0] != 0)
        {
            TMdbNtcDirOper::Remove(sBakBakPath, true);
        }
        
        return iRet;
        
    }

    int TMdbConfig::ClearAlterFile()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sTsAlteFile[MAX_FILE_NAME] ={0};
        snprintf(sTsAlteFile, sizeof(sTsAlteFile), "%s/.TableSpace_update.xml", m_szHome);
        char sTsAlteInfoFile[MAX_FILE_NAME] ={0};
        snprintf(sTsAlteInfoFile, sizeof(sTsAlteInfoFile), "%s/.TableSpace_update_info.xml", m_szHome);
		if(TMdbNtcFileOper::IsExist(sTsAlteInfoFile))
		{
			if(!TMdbNtcFileOper::Remove(sTsAlteInfoFile))
            {
                TADD_ERROR(-1, "remove tablespace alter info file[%s] failed.", sTsAlteInfoFile);
                return ERR_OS_REMOVE_FILE;
            }
		}

        if(TMdbNtcFileOper::IsExist(sTsAlteFile))
        {
            if(!TMdbNtcFileOper::Rename(sTsAlteFile, sTsAlteInfoFile))
            {
                TADD_ERROR(-1, "Rename path[%s] to [%s] failed.", sTsAlteFile, sTsAlteInfoFile);
                return ERR_OS_BACKUP_FILE;
            }
        }
        char sTabDir[MAX_PATH_NAME_LEN] = {0};
        snprintf(sTabDir, sizeof(sTabDir), "%s.TABLE", m_szHome);
        
        TMdbNtcFileScanner tDirScan;
        if(!tDirScan.ScanDir(sTabDir))
        {   
            TADD_ERROR(-1,"Get Table info failed.");
            return ERR_OS_OPEN_DIR;
        }

        const char* psTabDir = NULL;
        char sTabName[MAX_NAME_LEN] = {0};
        char sTabConfigAlterFile[MAX_FILE_NAME] ={0};
        char sTabConfigAlterInfoFile[MAX_FILE_NAME] ={0};
        while((psTabDir = tDirScan.GetNext()) != NULL)
        {
            memset(sTabName, 0, sizeof(sTabName)); 
            if(GetTableNameFromDir(psTabDir,sTabName,sizeof(sTabName)) < 0)
            {
                iRet = ERROR_UNKNOWN;
                break;
            }
            
            memset(sTabConfigAlterFile, 0, MAX_FILE_NAME);
            snprintf(sTabConfigAlterFile,MAX_FILE_NAME, "%s/.Tab_%s_update.xml", psTabDir, sTabName);
            memset(sTabConfigAlterInfoFile, 0, MAX_FILE_NAME);
            snprintf(sTabConfigAlterInfoFile,MAX_FILE_NAME, "%s/.Tab_%s_update_info.xml", psTabDir, sTabName);

            if(TMdbNtcFileOper::IsExist(sTabConfigAlterInfoFile))
            {
                if(!TMdbNtcFileOper::Remove(sTabConfigAlterInfoFile))
	            {
	                TADD_ERROR(-1, "remove table alter info file[%s] failed.", sTabConfigAlterInfoFile);
	                return ERR_OS_REMOVE_FILE;
	            }
            }
			if(TMdbNtcFileOper::IsExist(sTabConfigAlterFile))
			{
				if(!TMdbNtcFileOper::Rename(sTabConfigAlterFile, sTabConfigAlterInfoFile, TMdbNtcPathOper::IsSameFileSystem(sTabConfigAlterFile, sTabConfigAlterInfoFile)))
				{
					TADD_ERROR(-1, "Rename path[%s] to [%s] failed.", sTabConfigAlterFile, sTabConfigAlterInfoFile);
					return ERR_OS_BACKUP_FILE;
				}
			}
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  VerifySysCfg()
    * 函数描述	:  校验DSN配置文件参数
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0,否则返回-1
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbConfig::VerifySysCfg()
    {
        int iRet = 0;
        //if(m_tDsn.iValue < 0 || m_tDsn.iValue >= MAX_DSN_COUNTS)
        if(m_tDsn.llValue < 0)
        {
            CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"invalid  DSN-VALUE=%lld.",m_tDsn.llValue);
        }
        if(m_tDsn.iKillTime <= 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"KillTime=[%d] is invalid.", m_tDsn.iKillTime);
            return -1;
        }

        if(m_tDsn.iDelayTime <= 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"DelayTime=[%d] is invalid.",m_tDsn.iDelayTime);
            return -1;
        }

        if(m_tDsn.iCleanTime <= 0)
        {
            TADD_ERROR(ERROR_UNKNOWN," CleanTime=[%d] is invalid.",m_tDsn.iCleanTime);
            return -1;
        }

        if(m_tDsn.iRepType <= 0 || m_tDsn.iRepType >=4)
        {
            TADD_ERROR(ERROR_UNKNOWN,"RepType=[%d] is invalid.", m_tDsn.iRepType);
            return -1;
        }

        if(m_tDsn.iOraRepCounts <= 0)
        {
            TADD_ERROR(ERROR_UNKNOWN," OraRepCounts=[%d] is invalid.", m_tDsn.iOraRepCounts);
            return -1;
        }

        if(m_tProAttr.iHeartBeatFatal < m_tProAttr.iHeartBeatWarning)
        {
            TADD_ERROR(ERROR_UNKNOWN,"HeartBeatFatal<HeartBeatWarning.");
            return -1;
        }

        if (m_tDsn.bIsPeerRep && !m_tDsn.bIsRep)//配置了容灾备份而未配置主备同步
        {
            TADD_ERROR(ERROR_UNKNOWN,"Cannot set [is-peer-rep] = [Y] while [is-rep] = [N].");
            return -1;
        }

        if(m_tDsn.iManagerSize > MAX_MGR_SHM_SIZE)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"iManagerSize = [%ldMB] > MAX_MGR_SHM_SIZE[%dMB].",
                m_tDsn.iManagerSize/(1024*1024),MAX_MGR_SHM_SIZE/(1024*1024))
        }

        if(m_tDsn.iLogBuffSize < 256)
        {
            m_tDsn.iLogBuffSize = 256;
        }

        if(m_tDsn.iRepBuffSize < 256)
        {
            m_tDsn.iRepBuffSize = 256;
        }

        if(m_tDsn.iRepFileSize < 128)
        {
            m_tDsn.iRepFileSize = 128;
        }

        if(m_tDsn.m_iSeqCacheSize > 10000)
        {
            m_tDsn.m_iSeqCacheSize = 10000;
        }

        if (strlen(m_tDsn.sRoutingName) == 0)
        {
            SAFESTRCPY(m_tDsn.sRoutingName, sizeof(m_tDsn.sRoutingName), ROUTER_ID_COL_NAME);
        }
        
        if(m_tDsn.m_iRepFileTimeout <= 0)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"rep-file-timeout = [%d] is invalid.",m_tDsn.m_iRepFileTimeout);
            return ERR_APP_INVALID_PARAM;
        }
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  LoadSQL()
    * 函数描述	:  上载SQL 信息
    * 输入		:  pMDB table节点
    * 输出		:  pTable 表信息
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadSQL(TMdbTable* pTable,MDBXMLElement* pMDB)
    {
        //TADD_FUNC("TMdbConfig::LoadSQL() : Start.");
        if(pMDB != NULL)
        {
            MDBXMLElement* pEleFilterSql = pMDB->FirstChildElement("filter-sql");
            if(pEleFilterSql != NULL)
            {
                MDBXMLAttribute* pAttr = NULL;
                for(pAttr=pEleFilterSql->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0)
                    {
                        strncpy(pTable->m_sFilterSQL, pAttr->Value(), MAX_SQL_LEN-1);
                    }
                }
            }

            MDBXMLElement* pEleLoadSQL = pMDB->FirstChildElement("load-sql");
            if(pEleLoadSQL != NULL)
            {
                MDBXMLAttribute* pAttr = NULL;
                for(pAttr=pEleLoadSQL->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0)
                    {
                        strncpy(pTable->m_sLoadSQL, pAttr->Value(), MAX_SQL_LEN-1);
                    }
                }
            }

            MDBXMLElement* pEleFlushSQL = pMDB->FirstChildElement("flush-sql");
            if(pEleFlushSQL != NULL)
            {
                MDBXMLAttribute* pAttr = NULL;
                for(pAttr=pEleFlushSQL->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0)
                    {
                        strncpy(pTable->m_sFlushSQL, pAttr->Value(), MAX_SQL_LEN-1);
                    }
                }
            }

            //UR: 415074 若用户设置了需要加载的路由列表，且表的同步属性是mdb到oracle，
            //需要设置加载sql的routing_id条件
            AddRouteIDForLoadSQL(pTable);
        }
        return 0;
    }

    /******************************************************************************
    * 函数名称	:  AddRouteIDForLoadSQL()
    * 函数描述	:  为表刷新SQL添加路由
    * 输入		:  pTable 表
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  cao.peng
    *******************************************************************************/
    void TMdbConfig::AddRouteIDForLoadSQL(TMdbTable* pTable)
    {
        if (m_tDsn.sRoutingList[0] != 0) 
        {
            for (int i = 0; i < pTable->iColumnCounts; i++)
            {
                if(pTable->iRepAttr == REP_TO_DB)
                {
                    if (pTable->m_sLoadSQL[0] == 0)//LoadSql为空
                    {
                        //FilterSQL附加routing_id条件                        
                        if (pTable->m_sFilterSQL[0] == 0)//FilterSql为空
                        {
                            snprintf(pTable->m_sFilterSQL, MAX_SQL_LEN, "%s IN (%s)", m_tDsn.sRoutingName, m_tDsn.sRoutingList);
                        }
                        else
                        {
                            snprintf(pTable->m_sFilterSQL+strlen(pTable->m_sFilterSQL), MAX_SQL_LEN - strlen(pTable->m_sLoadSQL), " AND %s IN (%s)", m_tDsn.sRoutingName, m_tDsn.sRoutingList);
                        }
                    }
                    else //LoadSQL附加routing_id条件
                    {    
                        if (HasWhereCond(pTable->sTableName, pTable->m_sLoadSQL)) //LoadSQL中已经包含where条件
                        {
                            snprintf(pTable->m_sLoadSQL+strlen(pTable->m_sLoadSQL), MAX_SQL_LEN - strlen(pTable->m_sLoadSQL), " AND %s IN (%s)", m_tDsn.sRoutingName, m_tDsn.sRoutingList);
                        }
                        else
                        {
                            snprintf(pTable->m_sLoadSQL+strlen(pTable->m_sLoadSQL), MAX_SQL_LEN - strlen(pTable->m_sLoadSQL), " WHERE %s IN (%s)", m_tDsn.sRoutingName, m_tDsn.sRoutingList);
                        }
                    }
                    break;
                }
            }  
        }  
        TADD_DETAIL("LoadSQL : %s\n", pTable->m_sLoadSQL);
    }

    /******************************************************************************
    * 函数名称	:  GetTablePropertyValue()
    * 函数描述	:  读取 表 配置，获取指定属性的值
    * 输入		:  pMDB table节点,pPropertyName 属性名
    * 输出		:  无
    * 返回值	:  属性值
    * 作者		:  cao.peng
    *******************************************************************************/
    char *TMdbConfig::GetTablePropertyValue(MDBXMLElement* pMDB,const char* pPropertyName)
    {
        for(MDBXMLElement* pEleId=pMDB->FirstChildElement(pPropertyName); pEleId;\
            pEleId=pEleId->NextSiblingElement(pPropertyName))
        {
            MDBXMLAttribute* pAttr = NULL;
            for(pAttr=pEleId->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0)
                {
                    return const_cast<char*>(pAttr->Value());
                }
            }
        }
        return NULL;
    }
    /******************************************************************************
    * 函数名称	:  LoadTableCfg()
    * 函数描述	:  读取 表 配置
    * 输入		:  pMDB table节点
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadTableStruct(const char* psTableDir, const char* psTableName)
    {
        char sCfgFile[MAX_FILE_NAME] = {0};
		int  comRepType = -1;
        snprintf(sCfgFile, sizeof(sCfgFile), "%s/.Tab_%s_%s.xml",psTableDir, m_sDSN, psTableName);
        
        MDBXMLDocument tDoc(sCfgFile);
        if (false == tDoc.LoadFile())
        {
            TADD_ERROR(-1,"Load table [%s] configuration failed. Please check if the configuration conforms to the XML syntax rules.", psTableName);
            return -1;
        }
        MDBXMLElement* pRoot = tDoc.FirstChildElement("MDBConfig");
        if(pRoot == NULL)
        {
            TADD_ERROR(-1,"Not find element=[MDBConfig] when loading table configuration.");
            return -1;
        }
        
        //清除以前信息
        int iRet = 0;
        char *pValue = NULL;
        for(MDBXMLElement* pEle=pRoot->FirstChildElement("table"); pEle; pEle=pEle->NextSiblingElement("table"))
        {
			comRepType = -1;
			
            int iTableIndex = GetIdleTableId();
            if(iTableIndex< 0 ||iTableIndex > 450)
            {
                TADD_ERROR(-1,"reach max table count!");
                return ERR_APP_INVALID_PARAM;
            }

            m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
            if(m_pTable[iTableIndex] == NULL)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY," Mem Not Enough");
                return ERR_OS_NO_MEMROY;
            }
            m_pTable[iTableIndex]->Clear();
            
            //获取name节点值
            pValue = GetTablePropertyValue(pEle,"name");
            CHECK_OBJ(pValue);
            TMdbNtcStrFunc::Trim(pValue);
            //判断表名是否有效
            if(!IsValidName(pValue))
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM,"The table name[%s] is invalid,please check the configuration table.",pValue);
                return ERR_APP_INVALID_PARAM;  
            }
            //判断表名是否重复
            if(CheckRepeatTableName(pValue))
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid name element node,Table name[%s] already exist.", pValue);
                return ERR_APP_INVALID_PARAM;
            }
            //TMdbNtcStrFunc::ToUpper(pValue,m_pTable[iTableID]->sTableName);
            if(0 != TMdbNtcStrFunc::StrNoCaseCmp(pValue, psTableName))
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM,"inner table info error, table name not match,expect=[%s], actually=[%s]",psTableName,pValue);
                return ERR_APP_INVALID_PARAM;
            }
            
            SAFESTRCPY(m_pTable[iTableIndex]->sTableName, sizeof(m_pTable[iTableIndex]->sTableName), TMdbNtcStrFunc::ToUpper(pValue));

            // tableid主要为了向下兼容1.2版本向R81升级。
            pValue = GetTablePropertyValue(pEle,"table-id");
            if(pValue != NULL)
            {
                m_pTable[iTableIndex]->m_iTableId = atoi(pValue);
            }

            pValue = GetTablePropertyValue(pEle,"table-level");
            int iTableLevel = -1;
            if(NULL != pValue)
            {
                iTableLevel = atoi(pValue);
                if (iTableLevel < TAB_TINY || iTableLevel > TAB_ENORMOUS)
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[tablevel=%d].", iTableLevel);
                    return ERR_APP_INVALID_PARAM;
                }
                m_pTable[iTableIndex]->iTableLevel = iTableLevel;
                m_pTable[iTableIndex]->iTabLevelCnts = GetTableLevelCnt(iTableLevel);
                
            }            
            
            // 所属表空间
            pValue = GetTablePropertyValue(pEle,"table-space");
            if(pValue != NULL)
            {
                TMdbNtcStrFunc::Trim(pValue);
                SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), TMdbNtcStrFunc::ToUpper(pValue));
            }

            //获取record-counts节点值
            pValue = GetTablePropertyValue(pEle,"record-counts");
            if(pValue != NULL)
            {
                m_pTable[iTableIndex]->iRecordCounts = atoi(pValue);
            }

            //获取expand-record节点值
            pValue = GetTablePropertyValue(pEle,"expand-record");
            if(pValue != NULL)
            {
                m_pTable[iTableIndex]->iExpandRecords = atoi(pValue);
            }

            if(m_pTable[iTableIndex]->iRecordCounts > 0) //设置了recordcounts，已recordcounts为准
            {
                m_pTable[iTableIndex]->iTableLevel = GetTabLevelByRecordCounts(m_pTable[iTableIndex]->iRecordCounts);
            }
            else 
            {// 未设置recordcount，则根据指定的table-level来计算
                m_pTable[iTableIndex]->iRecordCounts  = GetRecordCountsByTabLevel(m_pTable[iTableIndex]->iTableLevel);
            }

            
            m_pTable[iTableIndex]->iRecordCounts += m_pTable[iTableIndex]->iExpandRecords;
            if(m_pTable[iTableIndex]->iRecordCounts < 10000)
            {
                m_pTable[iTableIndex]->iRecordCounts = 10000;
            }

            

            //获取Is-Zip-Time节点值
            pValue = GetTablePropertyValue(pEle,"Is-Zip-Time");
            if(pValue != NULL)
            {
                m_pTable[iTableIndex]->m_cZipTimeType = pValue[0];
                if('Y' != pValue[0] && 'N' != pValue[0] && 'L' != pValue[0] )
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[Is-Zip-Time=%s].",pValue);
                    return ERR_APP_INVALID_PARAM;
                }
            }

            //获取rep-type节点值
            pValue = GetTablePropertyValue(pEle,"rep-type");
            if(pValue != NULL)
            {
                m_pTable[iTableIndex]->iRepAttr = GetRepType(pValue);
				if(m_pTable[iTableIndex]->iRepAttr ==  REP_TO_DB_MDB)
				{
					m_pTable[iTableIndex]->iRepAttr = REP_TO_DB;
					comRepType = REP_TO_DB_MDB;
				}
            }

            //获取is-read-lock节点值
            pValue = GetTablePropertyValue(pEle,"is-read-lock");
            SET_TABLE_PROPERTY_BOOL_VALUE(m_pTable[iTableIndex]->bReadLock,"is-read-lock",pValue);

            //获取is-write-lock节点值
            pValue = GetTablePropertyValue(pEle,"is-write-lock");
            SET_TABLE_PROPERTY_BOOL_VALUE(m_pTable[iTableIndex]->bWriteLock,"is-write-lock",pValue);

            //获取is-rollback节点值
            pValue = GetTablePropertyValue(pEle,"is-rollback");
            SET_TABLE_PROPERTY_BOOL_VALUE(m_pTable[iTableIndex]->bRollBack,"is-rollback",pValue);

            //获取is-PerfStat节点值
            pValue = GetTablePropertyValue(pEle,"is-PerfStat");
            SET_TABLE_PROPERTY_BOOL_VALUE(m_pTable[iTableIndex]->bIsPerfStat,"is-PerfStat",pValue);

            //获取checkPriKey节点值
            pValue = GetTablePropertyValue(pEle,"checkPriKey");
            SET_TABLE_PROPERTY_BOOL_VALUE(m_pTable[iTableIndex]->bIsCheckPriKey,"checkPriKey",pValue);

            // 获取是否分片备份属性
             pValue = GetTablePropertyValue(pEle,"shard-backup");
            SET_TABLE_PROPERTY_BOOL_VALUE(m_pTable[iTableIndex]->m_bShardBack,"shard-backup",pValue);
            if(m_pTable[iTableIndex]->m_bShardBack ==  false && comRepType == REP_TO_DB_MDB )
				m_pTable[iTableIndex]->m_bShardBack = true;
			
            //获取LoadType节点值
            pValue = GetTablePropertyValue(pEle,"LoadType");
            if(NULL != pValue)
            {
                m_pTable[iTableIndex]->iLoadType = atoi(pValue);
            }

            //上载SQL  信息
            CHECK_RET(LoadSQL(m_pTable[iTableIndex],pEle),"LoadSQL(%s) faield.",m_pTable[iTableIndex]->sTableName);
            CHECK_RET(LoadParameter(m_pTable[iTableIndex],pEle),"LoadParameter(%s) faield.",m_pTable[iTableIndex]->sTableName);

            //上载对应的列信息,填充列的偏移量信息
            CHECK_RET(LoadTableColumn(m_pTable[iTableIndex], pEle)," LoadColumn(%s) failed.",m_pTable[iTableIndex]->sTableName);
            CHECK_RET(SetColOffset(m_pTable[iTableIndex]),"SetColOffset(%s) failed.",m_pTable[iTableIndex]->sTableName);
            //上载对应的索引
            CHECK_RET(LoadTableIndex(m_pTable[iTableIndex], pEle),"LoadTableIndex failed.");
            //上载对应主键信息
            CHECK_RET(LoadTablePrimaryKey(m_pTable[iTableIndex], pEle),"LoadTablePrimaryKey failed.");
            CHECK_RET(CalcOneRecordSize(m_pTable[iTableIndex]),"CalcOneRecordSize(%s) failed.",m_pTable[iTableIndex]->sTableName);
            //m_pTable[iTableIndex]->iCounts = m_pTable[iTableIndex]->iRecordCounts;
            ++m_iTableCounts;
            if(m_iTableCounts >= MAX_TABLE_COUNTS)
            {
                TADD_ERROR(ERR_TAB_TABLE_NUM_EXCEED_MAX," too many Table,Max=[%d].",MAX_TABLE_COUNTS);
                return ERR_TAB_TABLE_NUM_EXCEED_MAX;
            }

			
        }
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  LoadSysTable()
    * 函数描述	:  填充系统表信息
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadSysTable()
    {
        int iRet = 0;
        //添加dba_tables
        CHECK_RET(LoadDBATables(),"LoadDBATables error.");
        ++m_iTableCounts;
        //添加dba_column
        CHECK_RET(LoadDBAColumn(),"LoadDBAColumn error.");
        ++m_iTableCounts;
        //添加dba_index
        CHECK_RET(LoadDBAIndex(),"LoadDBAIndex error.");
        ++m_iTableCounts;
        //添加dba_tablespace
        CHECK_RET(LoadDBATableSpace(),"LoadDBATableSpace error.");
        ++m_iTableCounts;
        //添加dba_sequence
        CHECK_RET(LoadDBASequence(),"LoadDBASequence error.");
        ++m_iTableCounts;
        //添加dba_user
        CHECK_RET(LoadDBAUser(),"LoadDBAUser error.");
        ++m_iTableCounts;
        //添加dba_session
        CHECK_RET(LoadDBASession(),"LoadDBASession error.");
        ++m_iTableCounts;
        //添加dba_resource
        CHECK_RET(LoadDBAResource(),"LoadDBAResource error.");
        ++m_iTableCounts;
        CHECK_RET(LoadDBASQL(),"LoadDBASQL error.");
        ++m_iTableCounts;
        //添加dba_process
        CHECK_RET(LoadDBAProcess(),"LoadDBAProcess error.");
        ++m_iTableCounts;
        //添加dba dual
        CHECK_RET(LoadDBADual(),"LoadDBADual error.");
        ++m_iTableCounts;
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SetColumn()
    * 函数描述	:  设置系统表的某一个列信息
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbConfig::SetColumn(TMdbTable* pTable, int iCPos, const char* pszColumnName, int iDataType, int iDataLen)
    {
        pTable->tColumn[iCPos].Clear();

        SAFESTRCPY(pTable->tColumn[iCPos].sName,sizeof(pTable->tColumn[iCPos].sName),pszColumnName);
        pTable->tColumn[iCPos].iHashPos = pTable->m_ColoumHash.SetHashTable(const_cast<char *>(pszColumnName));

        pTable->tColumn[iCPos].iDataType      = iDataType;
        pTable->tColumn[iCPos].iColumnLen     = iDataLen;
        pTable->tColumn[iCPos].iPos           = iCPos;
        
        pTable->tColumn[iCPos].isInOra        = false;
    }

    /******************************************************************************
    * 函数名称	:  LoadDBATables()
    * 函数描述	:  添加dba_tables
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadDBATables()
    {

        int iRet = 0;

        int iTableIndex = 451;


        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"[%s:%d] TMdbConfig::LoadDBATables()  Mem Not Enough",__FILE__,__LINE__);
            return ERR_OS_NO_MEMROY;
        }
        m_pTable[iTableIndex]->Clear();

        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),"DBA_TABLES");
        m_pTable[iTableIndex]->iRecordCounts  = 1000;
        m_pTable[iTableIndex]->iExpandRecords = 100;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType = 1;
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat   = false;
        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 17;

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;
        m_pTable[iTableIndex]->bIsSysTab = true;

        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;

        SetColumn(m_pTable[iTableIndex], 0,  "table_name",        DT_Char, 32);
        SetColumn(m_pTable[iTableIndex], 1,  "table_space",    DT_Char,  32);
        SetColumn(m_pTable[iTableIndex], 2,  "record_set_counts", DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 3,  "real_counts",       DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 4,  "stat",              DT_Char, 4);
        SetColumn(m_pTable[iTableIndex], 5,  "expand_record",     DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 6,  "read_lock",         DT_Char, 4);
        SetColumn(m_pTable[iTableIndex], 7,  "write_lock",        DT_Char, 4);
        SetColumn(m_pTable[iTableIndex], 8,  "column_counts",     DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 9, "index_counts",      DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 10, "primary_key",       DT_Char, 32);
        SetColumn(m_pTable[iTableIndex], 11, "cin_counts",        DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 12, "left_cin_nodes",    DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 13, "full_pages",        DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 14, "free_pages",        DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 15, "shard_backup",        DT_Char,  4);
        SetColumn(m_pTable[iTableIndex], 16, "storage_type",        DT_Char,  4);



        //设置表中的索引信息
        m_pTable[iTableIndex]->iIndexCounts = 1;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"dba_table_name");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 1;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;
        m_pTable[iTableIndex]->cState  = Table_running;

        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }

        if(iRet < 0)
        {
            TADD_ERROR(iRet,"TMdbConfig::LoadDBATables() : Table=[%s] error.",
                m_pTable[iTableIndex]->sTableName);
            return iRet;
        }


        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  LoadDBAColumn()
    * 函数描述	:  添加dba_column
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadDBAColumn()
    {
        int iRet = 0;

        int iTableIndex = 452;

        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"[%s:%d] TMdbConfig::LoadDBATables()  Mem Not Enough",__FILE__,__LINE__);
            return ERR_OS_NO_MEMROY;
        }

        m_pTable[iTableIndex]->Clear();

        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),"DBA_COLUMN");
        m_pTable[iTableIndex]->iRecordCounts  = 20000;
        m_pTable[iTableIndex]->iExpandRecords = 100;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType = '1';
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat   = false;

        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 7;

        m_pTable[iTableIndex]->bIsSysTab = true;

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;

        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;

        SetColumn(m_pTable[iTableIndex], 0,  "table_name",          DT_Char,  33);
        SetColumn(m_pTable[iTableIndex], 1,  "column_name",       DT_Char, 33);
        SetColumn(m_pTable[iTableIndex], 2,  "data_type",         DT_Char, 21);
        SetColumn(m_pTable[iTableIndex], 3,  "data_len",          DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 4,  "data_pos",          DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 5,  "rep_attr",          DT_Char, 20);
        SetColumn(m_pTable[iTableIndex], 6,  "nullable",          DT_Char, 2);
        

        //设置表中的索引信息
        m_pTable[iTableIndex]->iIndexCounts = 2;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        //strcpy(m_pTable[iTableID]->tIndex[0].sName, "dba_column_name");
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"dba_column_name");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Char;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 1;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        m_pTable[iTableIndex]->tIndex[1].Clear();
        //strcpy(m_pTable[iTableID]->tIndex[1].sName, "dba_column_tid");
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[1].sName,sizeof(m_pTable[iTableIndex]->tIndex[1].sName),"dba_column_tid");
        m_pTable[iTableIndex]->tIndex[1].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[1].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[1].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[1].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 2;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[1]  = 1;
        m_pTable[iTableIndex]->cState  = Table_running;
        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }

        if(iRet < 0)
        {
            TADD_ERROR(iRet,"TMdbConfig::LoadDBAColumn() : Table=[%s] config error.",
                m_pTable[iTableIndex]->sTableName);
            return iRet;
        }

        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  LoadDBAIndex()
    * 函数描述	:  添加dba_index
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadDBAIndex()
    {
        int iRet = 0;

        int iTableIndex = 453;

        TADD_FLOW("TMdbConfig::LoadDBAIndex() : TableName=[%s].",  "dba_index");

        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY," TMdbConfig::LoadDBATables()  Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
        m_pTable[iTableIndex]->Clear();

        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),"DBA_INDEX");
        m_pTable[iTableIndex]->iRecordCounts  = 2000;
        m_pTable[iTableIndex]->iExpandRecords = 100;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType = '1';
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat   = false;
        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 7;

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;
        m_pTable[iTableIndex]->bIsSysTab = true;

        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;

        SetColumn(m_pTable[iTableIndex], 0,  "table_name",   DT_Char,  33);
        SetColumn(m_pTable[iTableIndex], 1,  "index_name", DT_Char, 65);
        SetColumn(m_pTable[iTableIndex], 2,  "index_type", DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 3,  "priority",   DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 4,  "data_pos",   DT_Char,  32);
        SetColumn(m_pTable[iTableIndex], 5,  "is_fix",     DT_Char, 4);
        SetColumn(m_pTable[iTableIndex], 6,  "Algorithm_Type",     DT_Char, 32);



        //设置表中的索引信息
        m_pTable[iTableIndex]->iIndexCounts = 2;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"dba_index_name");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Char;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 1;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        m_pTable[iTableIndex]->tIndex[1].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[1].sName,sizeof(m_pTable[iTableIndex]->tIndex[1].sName),"dba_index_tid");
        m_pTable[iTableIndex]->tIndex[1].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[1].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[1].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[1].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 2;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[1]  = 1;
        m_pTable[iTableIndex]->cState  = Table_running;

        if(iRet < 0)
        {
            TADD_ERROR(iRet,"TMdbConfig::LoadDBAIndex() : Table=[%s] config error.",
                m_pTable[iTableIndex]->sTableName);
            return iRet;
        }
        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }


        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  LoadDBATableSpace()
    * 函数描述	:  添加dba_tablespace
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadDBATableSpace()
    {
        int iRet = 0;

        int iTableIndex = 454;

        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY, " TMdbConfig::LoadDBATables()  Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
        m_pTable[iTableIndex]->Clear();

        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),"DBA_TABLE_SPACE");
        m_pTable[iTableIndex]->iRecordCounts  = 1000;
        m_pTable[iTableIndex]->iExpandRecords = 100;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType = '1';
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat   = false;
        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 6;

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;
        m_pTable[iTableIndex]->bIsSysTab = true;
        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;

        SetColumn(m_pTable[iTableIndex], 0,  "table_space_name", DT_Char, 33);
        SetColumn(m_pTable[iTableIndex], 1,  "page_size",        DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 2,  "ask_pages",        DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 3,  "total_pages",      DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 4,  "free_pages",       DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 5,  "file_storage",       DT_Char,  33);
        m_pTable[iTableIndex]->iIndexCounts = 2;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"dba_table_space_name");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Char;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 1;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        m_pTable[iTableIndex]->tIndex[1].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[1].sName,sizeof(m_pTable[iTableIndex]->tIndex[1].sName),"dba_table_space_id");
        m_pTable[iTableIndex]->tIndex[1].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[1].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[1].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[1].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 2;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[1]  = 1;
        m_pTable[iTableIndex]->cState  = Table_running;

        if(iRet < 0)
        {
            TADD_ERROR(iRet," TMdbConfig::LoadDBATableSpace() : Table=[%s] config error.",
                m_pTable[iTableIndex]->sTableName);
            return iRet;
        }
        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }

        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  LoadDBASequence()
    * 函数描述	:  添加dba_sequence
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadDBASequence()
    {

        int iRet = 0;
        int iTableIndex = 455;

        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY," TMdbConfig::LoadDBATables()  Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
        m_pTable[iTableIndex]->Clear();

        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),SYS_DBA_SEQUENCE);
        m_pTable[iTableIndex]->iRecordCounts  = 1000;
        m_pTable[iTableIndex]->iExpandRecords = 100;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
       // m_pTable[iTableID]->m_bIsZipTime = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType = '1';
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat   = false;
        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 5;

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;
        m_pTable[iTableIndex]->bIsSysTab = true;

        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;

        SetColumn(m_pTable[iTableIndex], 0,  "sequence_name",  DT_Char, 33);
        SetColumn(m_pTable[iTableIndex], 1,  "start_value",    DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 2,  "end_value",      DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 3,  "cur_value",      DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 4,  "step_value",     DT_Int,  8);



        //设置表中的索引信息
        m_pTable[iTableIndex]->iIndexCounts = 1;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"dba_sequence_name");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Char;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 1;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;
        m_pTable[iTableIndex]->cState  = Table_running;
        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }

        if(iRet < 0)
        {
            TADD_ERROR(iRet,"TMdbConfig::LoadDBASequence() : Table=[%s] config error.",
                m_pTable[iTableIndex]->sTableName);
            return iRet;
        }


        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  LoadDBAUser()
    * 函数描述	:  添加dba_user
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadDBAUser()
    {

        int iRet = 0;
        int iTableIndex = 456;

        TADD_FLOW("TMdbConfig::LoadDBAUser() : TableName=[%s].",  "dba_user");

        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY," TMdbConfig::LoadDBATables()  Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
        m_pTable[iTableIndex]->Clear();

        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),"DBA_USER");
        m_pTable[iTableIndex]->iRecordCounts  = 500;
        m_pTable[iTableIndex]->iExpandRecords = 100;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType = '1';
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat   = false;
        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 3;

        SetColumn(m_pTable[iTableIndex], 0,  "user_name",   DT_Char, 65);
        SetColumn(m_pTable[iTableIndex], 1,  "user_pwd",    DT_Char, 65);
        SetColumn(m_pTable[iTableIndex], 2,  "access_attr", DT_Char, 65);

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;

        m_pTable[iTableIndex]->bIsSysTab = true;



        //设置表中的索引信息
        m_pTable[iTableIndex]->iIndexCounts = 1;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"dba_user_name");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Char;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 1;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;
        m_pTable[iTableIndex]->cState  = Table_running;
        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }

        if(iRet < 0)
        {
            TADD_ERROR(iRet,"TMdbConfig::LoadDBAUser() : Table=[%s] config error.",
                m_pTable[iTableIndex]->sTableName);
            return iRet;
        }


        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  LoadDBASession()
    * 函数描述	:  添加dba_session
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadDBASession()
    {
        int iRet = 0;

        int iTableIndex = 457;

        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"TMdbConfig::LoadDBATables()  Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
        m_pTable[iTableIndex]->Clear();

        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),"DBA_SESSION");
        m_pTable[iTableIndex]->iRecordCounts  = 20000;
        m_pTable[iTableIndex]->iExpandRecords = 1000;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType = '1';
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat   = false;
        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 9;

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;

        m_pTable[iTableIndex]->bIsSysTab = true;

        SetColumn(m_pTable[iTableIndex], 0,  "session_id",  DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 1,  "pid",         DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 2,  "tid",         DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 3,  "ip",          DT_Char, 17);
        SetColumn(m_pTable[iTableIndex], 4,  "Handle",      DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 5,  "start_time",  DT_DateStamp, 15);
        SetColumn(m_pTable[iTableIndex], 6,  "state",       DT_Char, 6);
        SetColumn(m_pTable[iTableIndex], 7,  "log_level",   DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 8,  "sqlpos",         DT_Int,  8);



        //设置表中的索引信息
        m_pTable[iTableIndex]->iIndexCounts = 3;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"dba_session_id");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        m_pTable[iTableIndex]->tIndex[1].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[1].sName,sizeof(m_pTable[iTableIndex]->tIndex[1].sName),"dba_session_pid");
        m_pTable[iTableIndex]->tIndex[1].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[1].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[1].iColumnNo[0] = 1;
        m_pTable[iTableIndex]->tIndex[1].iPriority = 1;

        m_pTable[iTableIndex]->tIndex[2].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[2].sName,sizeof(m_pTable[iTableIndex]->tIndex[2].sName),"dba_session_ip");
        m_pTable[iTableIndex]->tIndex[2].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[2].m_iIndexType = HT_Char;
        m_pTable[iTableIndex]->tIndex[2].iColumnNo[0] = 3;
        m_pTable[iTableIndex]->tIndex[2].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 1;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;
        m_pTable[iTableIndex]->cState  = Table_running;
        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }

        if(iRet < 0)
        {
            TADD_ERROR(iRet," TMdbConfig::LoadDBASession() : Table=[%s] config error.",
                m_pTable[iTableIndex]->sTableName);
            return iRet;
        }

        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  LoadDBAResource()
    * 函数描述	:  添加dba_resource
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadDBAResource()
    {
        int iRet = 0;

        int iTableIndex = 458;

        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"TMdbConfig::LoadDBATables()  Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
        m_pTable[iTableIndex]->Clear();

        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),"DBA_RESOURCE");
        m_pTable[iTableIndex]->iRecordCounts  = 1000;
        m_pTable[iTableIndex]->iExpandRecords = 100;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType = '1';
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat   = false;
        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 5;

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;

        m_pTable[iTableIndex]->bIsSysTab = true;

        SetColumn(m_pTable[iTableIndex], 0,  "mem_key",     DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 1,  "mem_id",      DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 2,  "mem_type",    DT_Char, 33);
        SetColumn(m_pTable[iTableIndex], 3,  "mem_size",    DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 4,  "mem_left",    DT_Int,  8);



        //设置表中的索引信息
        m_pTable[iTableIndex]->iIndexCounts = 2;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"dba_mem_key");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        m_pTable[iTableIndex]->tIndex[1].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[1].sName,sizeof(m_pTable[iTableIndex]->tIndex[1].sName),"dba_mem_id");
        m_pTable[iTableIndex]->tIndex[1].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[1].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[1].iColumnNo[0] = 1;
        m_pTable[iTableIndex]->tIndex[1].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 1;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;
        m_pTable[iTableIndex]->cState  = Table_running;

        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }

        if(iRet < 0)
        {
            TADD_ERROR(iRet," TMdbConfig::LoadDBAResource() : Table=[%s] config error.",
                m_pTable[iTableIndex]->sTableName);
            return iRet;
        }

        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  LoadDBASQL()
    * 函数描述	:  添加dba_sql
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadDBASQL()
    {
        int iRet = 0;

        int iTableIndex = 459;

        TADD_FLOW("TMdbConfig::LoadDBASQL() : TableName=[%s].", "dba_sql");

        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"TMdbConfig::LoadDBATables()  Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
        m_pTable[iTableIndex]->Clear();

        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),"DBA_SQL");
        m_pTable[iTableIndex]->iRecordCounts  = 1000;
        m_pTable[iTableIndex]->iExpandRecords = 100;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType = '1';
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat   = false;
        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 7;

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;

        m_pTable[iTableIndex]->bIsSysTab = true;

        SetColumn(m_pTable[iTableIndex], 0,  "sql_series",  DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 1,  "pid",         DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 2,  "tid",         DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 3,  "ip",          DT_Char, 17);
        SetColumn(m_pTable[iTableIndex], 4,  "sqlpos",      DT_Int,  8);
        SetColumn(m_pTable[iTableIndex], 5,  "sql",         DT_Char,  1024);
        SetColumn(m_pTable[iTableIndex], 6,  "sdbaddr",     DT_Char,MAX_NAME_LEN);



        //设置表中的索引信息
        m_pTable[iTableIndex]->iIndexCounts = 3;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"dba_pid");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 1;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        m_pTable[iTableIndex]->tIndex[1].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[1].sName,sizeof(m_pTable[iTableIndex]->tIndex[1].sName),"dba_sqlpos");
        m_pTable[iTableIndex]->tIndex[1].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[1].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[1].iColumnNo[0] = 4;
        m_pTable[iTableIndex]->tIndex[1].iPriority = 1;

        m_pTable[iTableIndex]->tIndex[2].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[2].sName,sizeof(m_pTable[iTableIndex]->tIndex[2].sName),"dba_sql_series");
        m_pTable[iTableIndex]->tIndex[2].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[2].m_iIndexType = HT_Int;
        m_pTable[iTableIndex]->tIndex[2].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[2].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 1;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;

        m_pTable[iTableIndex]->cState  = Table_running;
        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }
        if(iRet < 0)
        {
            TADD_ERROR(iRet," TMdbConfig::LoadDBASQL() : Table=[%s] config error.",
                m_pTable[iTableIndex]->sTableName);
            return iRet;
        }


        return iRet;

    }

    /******************************************************************************
    * 函数名称	:  LoadDBAProcess()
    * 函数描述	:  添加dba_process
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbConfig::LoadDBAProcess()
    {
        int iRet = 0;
        int iTableIndex = 460;

        TADD_FLOW("TMdbConfig::LoadDBAProcess() : TableName=[%s].", "dba_process");
        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"TMdbConfig::LoadDBAProcess()  Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
        m_pTable[iTableIndex]->Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),"DBA_PROCESS");
        m_pTable[iTableIndex]->iRecordCounts  = 1000;
        m_pTable[iTableIndex]->iExpandRecords = 100;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView        = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType      = '1';
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat    = false;

        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 4;

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;

        m_pTable[iTableIndex]->bIsSysTab = true;
        
        SetColumn(m_pTable[iTableIndex], 0,  "process_name",  DT_Char,MAX_NAME_LEN + 1);
        SetColumn(m_pTable[iTableIndex], 1,  "pid",           DT_Int, 8);
        SetColumn(m_pTable[iTableIndex], 2,  "start_time",    DT_DateStamp,15);
        SetColumn(m_pTable[iTableIndex], 3,  "log_level",     DT_Int, 8);

        //设置表中的索引信息
        m_pTable[iTableIndex]->iIndexCounts = 1;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"index_dba_process");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Char;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 1;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;
        m_pTable[iTableIndex]->cState  = Table_running;

        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }

        if(iRet < 0)
        {
            TADD_ERROR(iRet,"Table=[%s] config error.", m_pTable[iTableIndex]->sTableName);
            return iRet;
        }
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  LoadDBADual()
    * 函数描述	:  加载dual表
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbConfig::LoadDBADual()
    {
        int iRet = 0;
         int iTableIndex = 461;
        TADD_FLOW("TableName=[%s].",  "dual");
        m_pTable[iTableIndex] = new(std::nothrow) TMdbTable();
        if(m_pTable[iTableIndex] == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }
        m_pTable[iTableIndex]->Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->sTableName,sizeof(m_pTable[iTableIndex]->sTableName),"DUAL");
        m_pTable[iTableIndex]->iRecordCounts  = 1000;
        m_pTable[iTableIndex]->iExpandRecords = 100;
        SAFESTRCPY(m_pTable[iTableIndex]->m_sTableSpace, sizeof(m_pTable[iTableIndex]->m_sTableSpace), SYS_TABLE_SPACE);
        m_pTable[iTableIndex]->bReadLock      = false;
        m_pTable[iTableIndex]->bIsView        = false;
        m_pTable[iTableIndex]->bWriteLock     = false;
        m_pTable[iTableIndex]->bRollBack      = false;
        m_pTable[iTableIndex]->bIsCheckPriKey = true;
        m_pTable[iTableIndex]->iLoadType      = '1';
        m_pTable[iTableIndex]->bFixedLength   = true;
        m_pTable[iTableIndex]->bIsPerfStat    = false;

        m_pTable[iTableIndex]->bIsSysTab = true;
        //设置表中的列信息
        m_pTable[iTableIndex]->iColumnCounts = 1;

        m_pTable[iTableIndex]->m_bShardBack   = false;
        m_pTable[iTableIndex]->m_cStorageType   = MDB_DS_TYPE_NO;
        m_pTable[iTableIndex]->iRepAttr = REP_NO_REP;
        
        SetColumn(m_pTable[iTableIndex], 0,  "dummy",  DT_Char,MAX_NAME_LEN + 1);

        //设置表中的索引信息
        m_pTable[iTableIndex]->iIndexCounts = 1;

        m_pTable[iTableIndex]->tIndex[0].Clear();
        SAFESTRCPY(m_pTable[iTableIndex]->tIndex[0].sName,sizeof(m_pTable[iTableIndex]->tIndex[0].sName),"index_dba_process");
        m_pTable[iTableIndex]->tIndex[0].m_iAlgoType = INDEX_HASH;
        m_pTable[iTableIndex]->tIndex[0].m_iIndexType = HT_Char;
        m_pTable[iTableIndex]->tIndex[0].iColumnNo[0] = 0;
        m_pTable[iTableIndex]->tIndex[0].iPriority = 1;

        //设置表中的主键信息
        m_pTable[iTableIndex]->m_tPriKey.iColumnCounts = 1;
        m_pTable[iTableIndex]->m_tPriKey.iColumnNo[0]  = 0;
        m_pTable[iTableIndex]->cState  = Table_running;

        //填充列的偏移量信息
        if(iRet == 0)
        {
            iRet = SetColOffset(m_pTable[iTableIndex]);
            iRet = CalcOneRecordSize(m_pTable[iTableIndex]);
        }
        if(iRet < 0)
        {
            TADD_ERROR(iRet,"Table=[%s] config error.", m_pTable[iTableIndex]->sTableName);
            return iRet;
        }
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  SetColOffset()
    * 函数描述	:  填充列的偏移量信息
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  设置成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::SetColOffset(TMdbTable* pTable)
    {
        //TADD_FUNC("TMdbConfig::SetColOffset(table=%s) : Start.", pTable->sTableName);

        int iRet = 0;
        pTable->bFixOffset = true;

        for(int i=0; i<pTable->iColumnCounts; ++i)
        {
            if(i == 0)
            {
                if(pTable->tColumn[i].iDataType == DT_VarChar||pTable->tColumn[i].iDataType == DT_Blob)
                {
                    //pTable->bFixOffset = false;
                }
                pTable->tColumn[i].iOffSet = 0;
            }
            else
            {
                if(pTable->tColumn[i].iDataType == DT_VarChar||pTable->tColumn[i].iDataType == DT_Blob)
                {
                    //pTable->bFixOffset = false;
                    if(pTable->tColumn[i-1].iDataType != DT_VarChar && pTable->tColumn[i-1].iDataType != DT_Blob)
                    {
                        pTable->tColumn[i].iOffSet = pTable->tColumn[i-1].iColumnLen+pTable->tColumn[i-1].iOffSet;
                    }
                    else
                    {
                        pTable->tColumn[i].iOffSet = 1+sizeof(long)*2+pTable->tColumn[i-1].iOffSet;
                    }

                    //pTable->bFixOffset = false;
                }
                else
                {
                    if(pTable->tColumn[i-1].iDataType == DT_VarChar||pTable->tColumn[i-1].iDataType == DT_Blob)
                    {
                        pTable->tColumn[i].iOffSet = pTable->tColumn[i-1].iOffSet + 1+sizeof(long)*2;
                    }
                    else
                    {
                        pTable->tColumn[i].iOffSet = pTable->tColumn[i-1].iOffSet + pTable->tColumn[i-1].iColumnLen;
                    }
                }
            }
            TADD_DETAIL("TMdbConfig::SetColOffset(table=%s) : Column=[%s], Offset=[%d].",
                pTable->sTableName, pTable->tColumn[i].sName, pTable->tColumn[i].iOffSet);
        }

        //TADD_FUNC("TMdbConfig::SetColOffset(table=%s) : Finish.", pTable->sTableName);

        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  CalcOneRecordSize
    * 函数描述	:  计算表中一条记录的大小
    * 输入		:  pTable - 需要计算的表
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbConfig::CalcOneRecordSize(TMdbTable * pTable)
    {
        int iRet = 0;
        if(NULL == pTable)return -1;
        int i = 0;
        pTable->iOneRecordSize = 0;
        //数据区的长度
        for(i = 0; i<pTable->iColumnCounts; i++)
        {
            if(DT_VarChar == pTable->tColumn[i].iDataType || DT_Blob == pTable->tColumn[i].iDataType)
            {
                //变长存储字段大小统一为9 = 1 + long + long
                pTable->iOneRecordSize = pTable->iOneRecordSize + 1 + sizeof(long)*2;
            }
            else
            {
                pTable->iOneRecordSize += pTable->tColumn[i].iColumnLen;
            }
        }
        if(pTable->iOneRecordSize<=0 || pTable->iOneRecordSize > MAX_VALUE_LEN)
        {
            TADD_ERROR(ERROR_UNKNOWN,"pTable[%s] iOneRecordSize = [%d] is error...",pTable->sTableName,pTable->iOneRecordSize);
            return -1;
        }

        // 记录的时间戳
        pTable->m_iTimeStampOffset = pTable->iOneRecordSize;
        pTable->iOneRecordSize += sizeof(long long);
        
        //加上，改条记录的NULL值
        //:TODO 判断该表是需要NULL
        int iNullSize = TMdbNULLManager::CalcNullSize( pTable);
        if(0 != iNullSize)
        {
            pTable->iOneRecordNullOffset = pTable->iOneRecordSize;
            pTable->iOneRecordSize += iNullSize;
        }
        else
        {
            pTable->iOneRecordNullOffset = -1;
        }
        TMdbTableSpace * pTS = GetTableSpaceByName(pTable->m_sTableSpace);
        if(NULL == pTS)
        {
            TADD_ERROR(ERR_DB_TPS_ID_NOT_EXIST,"Tablespace[%s] corresponds to the tablespace does not exist.",pTable->m_sTableSpace);
            return ERR_DB_TPS_ID_NOT_EXIST;
        }
        //校验每个页放的记录数不能超过MAX_MDB_PAGE_RECORD_COUNT
        if(pTS->iPageSize/(pTable->iOneRecordSize+sizeof(TMdbPageNode)) > MAX_MDB_PAGE_RECORD_COUNT)
        {
            int iPageSize = (pTable->iOneRecordSize+sizeof(TMdbPageNode))*MAX_MDB_PAGE_RECORD_COUNT/1024;
            CHECK_RET(ERR_APP_INVALID_PARAM,"Table[%s]'s record size[%d] is too small. Alter the table's record size or set the table-space[%s]'s page-size from [%d]KB to no more than [%d]KB.",
                pTable->sTableName, pTable->iOneRecordSize, pTS->sName, pTS->iPageSize/1024, iPageSize);
        }
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  LoadTableColumn()
    * 函数描述	:  上载表相关的列信息
    * 输入		:  pMDB table节点
    * 输出		:  pTable 表信息
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadTableColumn(TMdbTable* pTable, MDBXMLElement* pMDB)
    {
        //TADD_FUNC("TMdbConfig::LoadTableColumn(table=%s) : Start.", pTable->sTableName);
        int iRet = 0;
        pTable->iColumnCounts = 0;
        for(MDBXMLElement* pEle=pMDB->FirstChildElement("column"); pEle; pEle=pEle->NextSiblingElement("column"))
        {

            MDBXMLAttribute* pAttr = NULL;
            if(pTable->iColumnCounts >= MAX_COLUMN_COUNTS)
            {
                TADD_ERROR(ERR_TAB_COLUMN_NUM_EXCEED_MAX,"Table=[%s] too many columns,Max=[%d].",pTable->sTableName, MAX_COLUMN_COUNTS);
                return ERR_TAB_COLUMN_NUM_EXCEED_MAX;
            }
            
            int iColumnCounts = pTable->iColumnCounts;
            pTable->tColumn[iColumnCounts].Clear();
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                SET_PARAM_BOOL_VALUE(pTable->tColumn[iColumnCounts].bIsDefault,\
                    pAttr->Name(),"Is-Default",pAttr->Value());
                SET_PARAM_BOOL_VALUE(pTable->tColumn[iColumnCounts].m_bNullable,\
                    pAttr->Name(),"Null-able",pAttr->Value());
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                {
                    strncpy(pTable->tColumn[iColumnCounts].sName, pAttr->Value(), MAX_NAME_LEN-1);
                    TMdbNtcStrFunc::Trim(pTable->tColumn[iColumnCounts].sName);
                    if(!IsValidName(pTable->tColumn[iColumnCounts].sName))
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"The column name[%s] is invalid, please check the configuration table[%s].",\
                            pTable->tColumn[iColumnCounts].sName,pTable->sTableName);
                        return ERR_APP_INVALID_PARAM;
                    }
                    pTable->tColumn[iColumnCounts].iHashPos  = pTable->m_ColoumHash.SetHashTable(const_cast<char *>(pTable->tColumn[iColumnCounts].sName));
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "column-pos") == 0)
                {
                    pTable->tColumn[iColumnCounts].iPos = atoi(pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "data-type") == 0)
                {
                    pTable->tColumn[iColumnCounts].iDataType = GetDataType(pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "data-len") == 0)
                {
                    pTable->tColumn[iColumnCounts].iColumnLen = atoi(pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"Default-Value") == 0)
                {
                    strncpy(pTable->tColumn[iColumnCounts].iDefaultValue, pAttr->Value(), sizeof(pTable->tColumn[iColumnCounts].iDefaultValue)-1);
                }
				else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(),"rep-type") == 0)
				{
				}
                else
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s].",pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }
            CHECK_RET(CheckColumnProperty(pTable,iColumnCounts),"Check column attribute failure.");
            pTable->iColumnCounts++;
        }
        //所有列信息都加载完后需要校验配置的列pos是否连续
        CHECK_RET(CheckColumPosIsContinuous(pTable),"CheckColumPosIsContinuous failure.");
        //TADD_FUNC("TMdbConfig::LoadTableColumn(table=%s) : Finish.", pTable->sTableName);
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  CheckColumPosIsContinuous()
    * 函数描述	:  校验用户表列配置的column-pos属性是否连续
    * 输入		:  pTable 检验表名
    * 输出		:  无
    * 返回值	:  0:成功；!0 : 失败
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbConfig::CheckColumPosIsContinuous(TMdbTable* pTable)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        bool bIsExist = false;
        for(int iColumnPos=0;iColumnPos<pTable->iColumnCounts;iColumnPos++)
        {
            bIsExist = false;
            for(int i = 0;i < pTable->iColumnCounts; i++)
            {
                if(pTable->tColumn[i].iPos == iColumnPos)
                {
                    bIsExist = true;
                    break;
                }
            }
            
            if(!bIsExist)
            {
                TADD_ERROR(ERR_TAB_MISSING_COLUMN,"Table[%s] No find Column[%d].",pTable->sTableName,iColumnPos);
                return ERR_TAB_MISSING_COLUMN;
            }
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  CheckColumnProperty()
    * 函数描述	:  校验指定列的长度、同步属性等
    * 输入		:  pTable 检验表名，iColumnIndex 检查的列index，
    * 输出		:  无
    * 返回值	:  0:成功；!0 : 失败
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbConfig::CheckColumnProperty(TMdbTable* pTable, const int iColumnIndex)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        if(iColumnIndex >= MAX_COLUMN_COUNTS)
        {
            TADD_ERROR(ERR_TAB_COLUMN_NUM_EXCEED_MAX,"Table=[%s] too many columns,Max=[%d].",pTable->sTableName, MAX_COLUMN_COUNTS);
            return ERR_TAB_COLUMN_NUM_EXCEED_MAX;
        }
        //检查列数据类型是否有效
        if(pTable->tColumn[iColumnIndex].iDataType < 0)
        {
            TADD_ERROR(ERR_TAB_COLUMN_DATA_TYPE_INVALID,"Table=[%s] ColumnName=[%s], TYPE is invalid.\n\tExp: NUMBER, CHAR, VARCHAR, DATE/TIME/DATESTAMP, BLOB.",
                pTable->sTableName, pTable->tColumn[iColumnIndex].sName);
            return ERR_TAB_COLUMN_DATA_TYPE_INVALID;
        }
        //检验数据长度
        if(pTable->tColumn[iColumnIndex].iColumnLen <= 0)
        {
            TADD_ERROR(ERR_TAB_COLUMN_LENGTH_INVALID,"Table=[%s] ColumnName=[%s], length=[%d] is invalid.\n",
                pTable->sTableName, pTable->tColumn[iColumnIndex].sName, pTable->tColumn[iColumnIndex].iColumnLen);
            return ERR_TAB_COLUMN_LENGTH_INVALID;
        }
        //变长存储,长度和类型转换
        if(pTable->tColumn[iColumnIndex].iColumnLen < 16 
            && (pTable->tColumn[iColumnIndex].iDataType==DT_Blob
            ||pTable->tColumn[iColumnIndex].iDataType==DT_VarChar))
        {
            TADD_WARNING("TableName=[%s], ColoumName=[%s],ColoumType=VARCHAR|BLOB, ColoumLen=[%d] is too small",\
                pTable->sTableName,pTable->tColumn[iColumnIndex].sName,pTable->tColumn[iColumnIndex].iColumnLen);
            pTable->tColumn[iColumnIndex].iDataType = DT_Char;
        }
        //整型列长度固定8位，非压缩时间列为15位
        if(pTable->tColumn[iColumnIndex].iDataType == DT_Int)
        {
            pTable->tColumn[iColumnIndex].iColumnLen = 8;
        }
        else if(pTable->tColumn[iColumnIndex].iDataType == DT_DateStamp)
        {
            if(pTable->tColumn[iColumnIndex].bIsDefault)
            {
                if(strlen(pTable->tColumn[iColumnIndex].iDefaultValue) != 14 && 0 != TMdbNtcStrFunc::StrNoCaseCmp("sysdate",pTable->tColumn[iColumnIndex].iDefaultValue))
                {
                    TADD_ERROR(ERR_TAB_COLUMN_LENGTH_INVALID,"Table=[%s] ColumnName=[%s], default_value length is wrong,it must=14",
                        pTable->sTableName, pTable->tColumn[iColumnIndex].sName);
                    return ERR_TAB_COLUMN_LENGTH_INVALID;
                }
            }
            pTable->tColumn[iColumnIndex].iColumnLen = pTable->GetTimeValueLength();
            /*
            if(pTable->m_bIsZipTime)
            {
                pTable->tColumn[iColumnIndex].iColumnLen = 4;
            }
            else
            {
                pTable->tColumn[iColumnIndex].iColumnLen = 15;
            }
            */
        }
        else
        {
            //如果有默认值，则需要确保默认值的长度不成超过前面所配置的长度
            if(pTable->tColumn[iColumnIndex].bIsDefault)
            {
                if(pTable->tColumn[iColumnIndex].iColumnLen <(int) strlen(pTable->tColumn[iColumnIndex].iDefaultValue))
                {
                    TADD_ERROR(ERR_TAB_COLUMN_LENGTH_INVALID,"Table=[%s] ColumnName=[%s], default_value length must < ColumnLength",
                        pTable->sTableName, pTable->tColumn[iColumnIndex].sName);
                    return ERR_TAB_COLUMN_LENGTH_INVALID;
                }
            }
            pTable->tColumn[iColumnIndex].iColumnLen += 1;
        }
        //校验列同步属性
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  GetDataType()
    * 函数描述	:  根据字段类型获取内部类型
    * 输入		:  pszDataTpye 字段类型
    * 输出		:  无
    * 返回值	:  内部数据类型
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::GetDataType(const char* pszDataTpye)
    {
        //TADD_FUNC("TMdbConfig::GetDataType(pszDataTpye=%s) : Start.", pszDataTpye);

        int iDataType = -1;

        if(TMdbNtcStrFunc::StrNoCaseCmp(pszDataTpye, "NUMBER") == 0)
        {
            iDataType = DT_Int;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszDataTpye, "VARCHAR") == 0 || TMdbNtcStrFunc::StrNoCaseCmp(pszDataTpye, "VARCHAR2") == 0)
        {
            iDataType = DT_VarChar;
            //暂时不支持varchar
            //iDataType = DT_Char;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszDataTpye, "CHAR") == 0)
        {
            iDataType = DT_Char;
        }
        else if((TMdbNtcStrFunc::StrNoCaseCmp(pszDataTpye, "DATE") == 0) ||
            (TMdbNtcStrFunc::StrNoCaseCmp(pszDataTpye, "TIME") == 0) ||
            (TMdbNtcStrFunc::StrNoCaseCmp(pszDataTpye, "DATESTAMP") == 0))
        {
            iDataType = DT_DateStamp;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszDataTpye, "BLOB") == 0)
        {
            iDataType = DT_Blob;
        }
        else
        {
            iDataType = -1;
        }

        //TADD_FUNC("TMdbConfig::GetDataType(pszDataTpye=%s, iDataType=%d) : Finish.", pszDataTpye, iDataType);

        return iDataType;
    }


    /******************************************************************************
    * 函数名称	:  GetRepType()
    * 函数描述	:  获取同步类型
    * 输入		:  pszRepTpye 配置的同步类型
    * 输出		:  无
    * 返回值	:  转换后内部同步类型
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::GetRepType(const char* pszRepTpye)
    {

        int iRepType = -1;

        if(TMdbNtcStrFunc::StrNoCaseCmp(pszRepTpye, REP_DB2MDB) == 0)
        {
            iRepType = REP_FROM_DB;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszRepTpye, REP_MDB2DB) == 0)
        {
            iRepType = REP_TO_DB;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pszRepTpye, REP_NoRep) == 0)
        {
            iRepType = REP_NO_REP;
        }
		else if(TMdbNtcStrFunc::StrNoCaseCmp(pszRepTpye, "oracle2mdb") == 0)
		{
			iRepType = REP_FROM_DB;

		}
		else if(TMdbNtcStrFunc::StrNoCaseCmp(pszRepTpye, "mdb2oracle") == 0)
		{
			iRepType = REP_TO_DB;

		}
		else if(TMdbNtcStrFunc::StrNoCaseCmp(pszRepTpye, "mdb2oracle2mdb") == 0)
		{
			iRepType = REP_TO_DB_MDB;

		}
        else
        {
            iRepType = -1;
        }

        return iRepType;
    }

    /******************************************************************************
    * 函数名称	:  GetDataType
    * 函数描述	:  获取数据类型的字符串表示
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbConfig::GetDataType(int iDataType,char *pDataType,const int iLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pDataType);
        switch(iDataType)
        {
        case DT_Int:
            SAFESTRCPY(pDataType,iLen,"NUMBER");
            break;
        case DT_Char:
            SAFESTRCPY(pDataType,iLen,"CHAR");
            break;
        case DT_VarChar:
            SAFESTRCPY(pDataType,iLen,"VARCHAR");
            break;
        case DT_DateStamp:
            SAFESTRCPY(pDataType,iLen,"DATESTAMP");
            break;
        case DT_Blob:
            SAFESTRCPY(pDataType,iLen,"BLOB");
            break;  
        default:
            iRet = -1;
            break;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  GetDataRepType
    * 函数描述	:  获取数据同步类型的字符串表示
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbConfig::GetRepType(int iRepType,char *pRepType,const int iLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pRepType);
        switch(iRepType)
        {
        case REP_FROM_DB:
            SAFESTRCPY(pRepType,iLen,"DB2MDB");
            break;
        case REP_TO_DB:
            SAFESTRCPY(pRepType,iLen,"MDB2DB");
            break;
        case REP_NO_REP:
            SAFESTRCPY(pRepType,iLen,"NoRep");
            break;  
        default:
            iRet = -1;
            break;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  LoadTableIndex()
    * 函数描述	:  上载表相关的索引信息
    * 输入		:  pMDB 表节点
    * 输出		:  pTable 表信息
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadTableIndex(TMdbTable* pTable,MDBXMLElement* pMDB)
    {
        //TADD_FUNC("TMdbConfig::LoadTableIndex(table=%s) : Start.", pTable->sTableName);
        pTable->iIndexCounts = 0;

        for(MDBXMLElement* pEle=pMDB->FirstChildElement("index"); pEle; pEle=pEle->NextSiblingElement("index"))
        {
            MDBXMLAttribute* pAttr = NULL;
            int iIndexCounts = pTable->iIndexCounts;
            pTable->tIndex[iIndexCounts].Clear();

            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                {
                    strncpy(pTable->tIndex[iIndexCounts].sName, pAttr->Value(), MAX_NAME_LEN-1);
                    TMdbNtcStrFunc::Trim(pTable->tIndex[iIndexCounts].sName);
                    if(!IsValidName(pTable->tIndex[iIndexCounts].sName))
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"The index name[%s] is invalid, please check the index configuration.",\
                            pTable->tIndex[iIndexCounts].sName);
                        return ERR_APP_INVALID_PARAM;
                    }
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "algo-type") == 0)
                {
                    pTable->tIndex[iIndexCounts].m_iAlgoType = atoi(pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "column-pos") == 0)
                {
                    TMdbNtcSplit tSplit;
                    tSplit.SplitString(pAttr->Value(),',');
                    //tSplit.SetString(pAttr->Value());
                    for(unsigned int i=0; i<tSplit.GetFieldCount(); ++i)
                    {
                        pTable->tIndex[iIndexCounts].iColumnNo[i] = atoi(tSplit[i]);
                    }
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "priority") == 0)
                {
                    pTable->tIndex[iIndexCounts].iPriority = atoi(pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "max-layer") == 0)
                {
                    pTable->tIndex[iIndexCounts].iMaxLayer= atoi(pAttr->Value());
                }
                else
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"[%s : %d] : TMdbConfig::LoadIndex() : Invalid element=[%s].", __FILE__, __LINE__, pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }

            if(pTable->tIndex[iIndexCounts].m_iAlgoType != INDEX_HASH
                && pTable->tIndex[iIndexCounts].m_iAlgoType != INDEX_M_HASH
                && pTable->tIndex[iIndexCounts].m_iAlgoType != INDEX_TRIE)
            {
                TADD_ERROR(ERR_TAB_INDEX_INVALID_TYPE,"TMdbConfig::LoadTableIndex() : Table=[%s] too many Index,Max=[%d].",
                    pTable->sTableName, MAX_INDEX_COUNTS);
                return ERR_TAB_INDEX_INVALID_TYPE;
            }

            int iCPos = pTable->tIndex[iIndexCounts].iColumnNo[0];
            if(pTable->tIndex[iIndexCounts].iColumnNo[1] > -1)
            {
                pTable->tIndex[iIndexCounts].m_iIndexType = HT_CMP;
            }
            else if(pTable->tColumn[iCPos].iDataType == DT_Int)
            {
                pTable->tIndex[iIndexCounts].m_iIndexType = HT_Int;
            }
            else
            {
                pTable->tIndex[iIndexCounts].m_iIndexType = HT_Char;
            }

			
			if (pTable->tIndex[iIndexCounts].m_iAlgoType  == INDEX_TRIE \
				&&  pTable->tIndex[iIndexCounts].m_iIndexType != HT_Char)
			{
				TADD_ERROR(ERR_TAB_INDEX_INVALID_TYPE, "index[%s] is invalid: trie only supports string column", pTable->tIndex[iIndexCounts].sName);
				return  ERR_TAB_INDEX_INVALID_TYPE;
			}

            ++pTable->iIndexCounts;
            if(pTable->iIndexCounts >= MAX_INDEX_COUNTS)
            {
                TADD_ERROR(ERR_TAB_INDEX_NUM_EXCEED_MAX,"TMdbConfig::LoadTableIndex() : Table=[%s] too many Index,Max=[%d].",
                    pTable->sTableName, MAX_INDEX_COUNTS);
                return ERR_TAB_INDEX_NUM_EXCEED_MAX;
            }

        }

        //pTable->iExpandRecords = pTable->iExpandRecords * pTable->iIndexCounts;

        return 0;
    }


    /******************************************************************************
    * 函数名称	:  LoadTablePrimaryKey()
    * 函数描述	:  上载表相关的主键信息
    * 输入		:  pMDB pkey节点
    * 输出		:  pTable 表信息
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadTablePrimaryKey(TMdbTable* pTable,MDBXMLElement* pMDB)
    {
        pTable->m_tPriKey.Clear();
        SAFESTRCPY(pTable->m_tPriKey.m_sTableName, sizeof(pTable->m_tPriKey.m_sTableName), pTable->sTableName);

        for(MDBXMLElement* pEle=pMDB->FirstChildElement("pkey"); pEle; pEle=pEle->NextSiblingElement("pkey"))
        {
            MDBXMLAttribute* pAttr = NULL;
            int iColumnCounts = pTable->m_tPriKey.iColumnCounts;
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "column-pos") == 0)
                {
                     if(false == TMdbNtcStrFunc::IsDigital(pAttr->Value()))
                    {
                        TADD_WARNING("Table[%s] PK config:column-pos =  [%s] is error,should be single number.",
                            pTable->sTableName,pAttr->Value());
                    }
                    if(pTable->tColumn[atoi(pAttr->Value())].m_bNullable)
                    {
                        TADD_ERROR(ERR_TAB_PR_COLUMN_IS_NOT_NULLABLE,"In table %s,the primary key column[%s] cannot be configured for NULL.",\
                            pTable->sTableName,pTable->tColumn[atoi(pAttr->Value())].sName);
                        return ERR_TAB_PR_COLUMN_IS_NOT_NULLABLE;
                    }
                    pTable->m_tPriKey.iColumnNo[iColumnCounts] = atoi(pAttr->Value());
                }
                else
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s].",pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }
            //校验主键列是否有缺省值
            TMdbColumn &tColumn = pTable->tColumn[pTable->m_tPriKey.iColumnNo[iColumnCounts]];
            if(tColumn.bIsDefault)
            {
                TADD_ERROR(ERR_TAB_PK_COLUMN_HAS_DEFAULT_VALUE,"In table %s,the primary key column[%s] cannot have default values.",\
                    pTable->sTableName,tColumn.sName);
                return ERR_TAB_PK_COLUMN_HAS_DEFAULT_VALUE;
            }
            ++(pTable->m_tPriKey.iColumnCounts);
            //TADD_FLOW("TMdbConfig::LoadTablePrimaryKey(table=%-10s) : COLUMN_NAME=%s.", pTable->sTableName, pTable->tColumn[pTable->m_tPriKey.iColumnNo[iColumnCounts]].sName);
        }
        //TADD_FUNC("TMdbConfig::LoadTablePrimaryKey(table=%s) : Finish.", pTable->sTableName);

        return 0;
    }

    /******************************************************************************
    * 函数名称	:  LoadChildInfo()
    * 函数描述	:  上载子表信息
    * 输入		:  pMDB table节点
    * 输出		:  pTable 表信息
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    /*int TMdbConfig::LoadChildInfo(TMdbTable * pTable, MDBXMLElement * pMDB)
    {
        MDBXMLAttribute* pAttr = NULL;
        pTable->iChildTableCounts = 0;
        int iRet = 0;

        for(MDBXMLElement* pEle=pMDB->FirstChildElement("child-table"); pEle; pEle=pEle->NextSiblingElement("child-table"))
        {
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                {
                    strncpy(pTable->tChildTable[pTable->iChildTableCounts].sTableName, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else
                {
                    TADD_ERROR("[%s : %d] : TMdbConfig::LoadTable() : Invalid element=[%s].", __FILE__, __LINE__, pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }

            iRet = LoadRelation(&pTable->tChildTable[pTable->iChildTableCounts],pEle);
            if (iRet < 0)
            {
                TADD_ERROR("[%s : %d] : TMdbConfig::LoadTable() : LoadChildInfo(%s) faield.", __FILE__, __LINE__, pTable->tChildTable[pTable->iChildTableCounts].sTableName);
                return iRet;
            }
            pTable->tChildTable[pTable->iChildTableCounts].Print();
            pTable->iChildTableCounts++;
        }

        return iRet;
    }
    */

    /******************************************************************************
    * 函数名称	:  LoadParameter()
    * 函数描述	:  上载参数配置
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadParameter(TMdbTable * pTable, MDBXMLElement * pMDB)
    {
        //TADD_FUNC("TMdbConfig::LoadParameter(table=%s) : Start.", pTable->sTableName);
        MDBXMLAttribute* pAttr = NULL;
        pTable->iParameterCount = 0;
        //int iRet = 0;

        for(MDBXMLElement* pEle=pMDB->FirstChildElement("Parameter"); pEle; pEle=pEle->NextSiblingElement("Parameter"))
        {
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                {
                    strncpy(pTable->tParameter[pTable->iParameterCount].sName, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "data-type") == 0)
                {
                    pTable->tParameter[pTable->iParameterCount].iDataType = GetDataType(pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0)
                {
                    SAFESTRCPY(pTable->tParameter[pTable->iParameterCount].sValue,sizeof(pTable->tParameter[pTable->iParameterCount].sValue),pAttr->Value());
                }
                else if (TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "parameter-type") == 0)
                {
                    pTable->tParameter[pTable->iParameterCount].iParameterType = atoi(pAttr->Value());
                }
                else
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"TMdbConfig::LoadTable() : Invalid element=[%s].",  pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }
            pTable->iParameterCount++;
        }
        //TADD_FUNC("TMdbConfig::LoadParameter: Finished.");

        return 0;
    }

    /******************************************************************************
    * 函数名称	:  LoadRelation()
    * 函数描述	:  上载子表和主表的对映关系
    * 输入		:  pMDB
    * 输出		:  pChildTable 子表信息
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
   /* int TMdbConfig::LoadRelation(TMdbChildTable * pChildTable, MDBXMLElement * pMDB)
    {
        MDBXMLAttribute* pAttr = NULL;
        for(MDBXMLElement* pEle=pMDB->FirstChildElement("column"); pEle; pEle=pEle->NextSiblingElement("column"))
        {
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "base-name") == 0)
                {
                    strncpy(pChildTable->m_ChildColumn[pChildTable->iRelationCount].sBaseColumnName, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "child-name") == 0)
                {
                    strncpy(pChildTable->m_ChildColumn[pChildTable->iRelationCount].sChildColumnName,pAttr->Value(), MAX_NAME_LEN-1);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "data-type") == 0)
                {
                    pChildTable->m_ChildColumn[pChildTable->iRelationCount].iDataType = GetDataType(pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "Is-Default") == 0)
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "Y") == 0)
                        pChildTable->m_ChildColumn[pChildTable->iRelationCount].bIsDefault = true;
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "N") == 0)
                        pChildTable->m_ChildColumn[pChildTable->iRelationCount].bIsDefault = false;
                    else
                    {
                        TADD_ERROR("[%s : %d] : TMdbConfig::LoadSysInfo() : Invalid element=[Is-Default=%s].", __FILE__, __LINE__,
                            pAttr->Value());
                        return ERR_APP_INVALID_PARAM;
                    }
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "Default-Value") == 0)
                {
                    SAFESTRCPY(pChildTable->m_ChildColumn[pChildTable->iRelationCount].iDefaultValue,sizeof(TName),pAttr->Value());
                }
                else
                {
                    TADD_ERROR("[%s : %d] : TMdbConfig::LoadTable() : Invalid element=[%s].", __FILE__, __LINE__, pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }

            pChildTable->iRelationCount++;
        }

        //上载子表的主键
        for(MDBXMLElement* pEle=pMDB->FirstChildElement("pkey"); pEle; pEle=pEle->NextSiblingElement("pkey"))
        {
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "column-name") == 0)
                {
                    strncpy(pChildTable->sPriKeyName[pChildTable->iPriKeyCount], pAttr->Value(), MAX_NAME_LEN-1);
                }
                else
                {
                    TADD_ERROR("[%s : %d] : TMdbConfig::LoadTable() : Invalid element=[%s].", __FILE__, __LINE__, pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }

            pChildTable->iPriKeyCount++;
        }
        return 0;
    }
    */

    /******************************************************************************
    * 函数名称	:  LoadTableSpaceCfg()
    * 函数描述	:  读取 表空间 配置
    * 输入		:  pMDB table-space节点
    * 输出		:  无
    * 返回值	:  加载成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::LoadTableSpaceCfg(MDBXMLElement* pMDB,const bool bCheck)
    {
        m_iTableSpaceCounts = 0;
        bool isHave = false;
        for(MDBXMLElement* pEle=pMDB->FirstChildElement("table-space"); pEle; pEle=pEle->NextSiblingElement("table-space"))
        {
            MDBXMLAttribute* pAttr = NULL;
            int iCounts = m_iTableSpaceCounts;
            m_pTableSpace[iCounts] = new(std::nothrow) TMdbTableSpace();
            if(m_pTableSpace[iCounts] == NULL)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY,"Mem Not Enough");
                return ERR_OS_NO_MEMROY;
            }
            for(pAttr=pEle->FirstAttribute(); pAttr; pAttr=pAttr->Next())
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                {
                    strncpy(m_pTableSpace[iCounts]->sName, pAttr->Value(), MAX_NAME_LEN-1);
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "page-size") == 0)
                {
                    m_pTableSpace[iCounts]->iPageSize = atoi(pAttr->Value())*1024;
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "ask-pages") == 0)
                {
                    m_pTableSpace[iCounts]->iRequestCounts = atoi(pAttr->Value());
                }
                else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "is-file-storage") == 0)
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Value(), "Y") == 0)
                    {
                        m_pTableSpace[iCounts]->m_bFileStorage = true;
                        m_bStartFileStorageProc = true;
                    }
                    else
                    {
                        m_pTableSpace[iCounts]->m_bFileStorage = false;
                    }
                }
                else
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[%s].", pAttr->Name());
                    return ERR_APP_INVALID_PARAM;
                }
            }
            
            
            ++m_iTableSpaceCounts;
            if(m_iTableSpaceCounts >= MAX_TABLE_COUNTS)
            {
                TADD_ERROR(ERR_DB_TSAPCE_NUM_EXCEED_MAX,"too many TableSpace,Max=[%d].",MAX_TABLE_COUNTS);
                return ERR_DB_TSAPCE_NUM_EXCEED_MAX;
            }
        }

        if (false == isHave)
        {
            int iCounts = m_iTableSpaceCounts;
            m_pTableSpace[iCounts] = new(std::nothrow) TMdbTableSpace();
            if(m_pTableSpace[iCounts] == NULL)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY,"Mem Not Enough");
                return ERR_OS_NO_MEMROY;
            }
            strncpy(m_pTableSpace[iCounts]->sName, SYS_TABLE_SPACE, MAX_NAME_LEN-1);
			m_pTableSpace[iCounts]->m_bFileStorage = false;
#ifdef WIN32
            m_pTableSpace[iCounts]->iPageSize = 16*1024;
#else
            m_pTableSpace[iCounts]->iPageSize = 32*1024;
#endif
            m_pTableSpace[iCounts]->iRequestCounts = 10;

            ++m_iTableSpaceCounts;
            if(m_iTableSpaceCounts >= MAX_TABLE_COUNTS)
            {
                TADD_ERROR(ERR_DB_TSAPCE_NUM_EXCEED_MAX,"too many TableSpace,Max=[%d].",MAX_TABLE_COUNTS);
                return ERR_DB_TSAPCE_NUM_EXCEED_MAX;
            }
        }


        
        return 0;
    }

	/******************************************************************************
    * 函数名称	:  CheckColumnCfg()
    * 函数描述	:  检测表的列属性配置
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  0表示配置ok,非0表示配置非法
    * 作者		:  yu.lianxiang
    *******************************************************************************/
	int TMdbConfig:: CheckColumnCfg(TMdbTable * pTable)
	{
		CHECK_OBJ(pTable);		
		int iTotleSize = 0;
		if(pTable->iColumnCounts <= 0)
        {
            TADD_ERROR(ERR_TAB_COLUMN_NOT_EXIST,"Table=[%s] doesn't have any column.",pTable->sTableName);
            return ERR_TAB_COLUMN_NOT_EXIST;
        }
        if(pTable->iColumnCounts > MAX_COLUMN_COUNTS)
        {
            TADD_ERROR(ERR_TAB_TABLE_NUM_EXCEED_MAX,"Table=[%s] too many  column,it must < [%d].",pTable->sTableName,MAX_COLUMN_COUNTS);
            return ERR_TAB_TABLE_NUM_EXCEED_MAX;
        }
        if(pTable->m_tPriKey.iColumnCounts <= 0)
        {
            TADD_ERROR(ERR_TAB_NO_PRIMARY_KEY,"Table=[%s] doesn't have any primary-key.",pTable->sTableName);
            return ERR_TAB_NO_PRIMARY_KEY;
        }
        int k = 0;
        for (k=0; k<pTable->iColumnCounts; k++)
        {
            if(pTable->tColumn[k].iColumnLen > MAX_BLOB_LEN)
            {
                TADD_ERROR(ERR_TAB_COLUMN_LENGTH_INVALID,"Table=[%s],tableColumn=[%s] too long ,it must < [%d].",pTable->sTableName,pTable->tColumn[k].sName,MAX_BLOB_LEN);
                return ERR_TAB_COLUMN_LENGTH_INVALID;
            }
            
            iTotleSize+=pTable->tColumn[k].iColumnLen;
        }
        if (iTotleSize > MAX_VALUE_LEN)
        {
            TADD_ERROR(ERR_TAB_COLUMN_LENGTH_INVALID,"Table=[%s],total colunmLength  too long ,it must < [%d].",pTable->sTableName,MAX_VALUE_LEN);
            return ERR_TAB_COLUMN_LENGTH_INVALID;
        }
        
        for(k =0; k<pTable->m_tPriKey.iColumnCounts; ++k)
        {
            if(pTable->m_tPriKey.iColumnNo[k] >= pTable->iColumnCounts)
            {
                TADD_ERROR(ERR_TAB_PK_COLUMN_POS_INVALID,"Table=[%s] has invalid primary-key=[column-no=%d].",pTable->sTableName, pTable->m_tPriKey.iColumnNo[k]);
                return ERR_TAB_PK_COLUMN_POS_INVALID;
            }
        }

		return 0;
	}

    /******************************************************************************
    * 函数名称	:  CheckCfg()
    * 函数描述	:  检测配置(主要检查表的配置和表空间的配置)
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  0表示配置ok,非0表示配置非法
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::CheckCfg()
    {
        int iRet = 0;
		//表级别的分片备份开关
		bool bIsShard_tab = false;
		
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            TMdbTable * pTable = m_pTable[i] ;
            if(pTable == NULL)
                continue;

            if(pTable->m_bShardBack)
            {
                bIsShard_tab = true;
            }
            if(pTable->iRepAttr == REP_FROM_DB)
            {
                m_bStartFlushFromDbProc = true;
            }

            if(pTable->iRepAttr == REP_TO_DB)
            {
                m_bStartDbRepProc = true;
            }
			
            if(NULL == GetTableSpaceByName(pTable->m_sTableSpace))
            {
                TADD_ERROR(ERR_DB_TPS_ID_NOT_EXIST,"not find the table_space=[%s] of table=[%s].",pTable->m_sTableSpace, pTable->sTableName);
                return ERR_DB_TPS_ID_NOT_EXIST;
            }
            CHECK_RET(CheckColumnCfg(pTable),"Failed to CheckColumnCfg.");
            CHECK_RET(CheckPKIsIndexed(pTable),"Failed to verify that primary keys are indexed.");
        }

		//!!兼容性
		//该情况适用于从1.2版本迁移过来,只要其中一个开关打开,则打开系统级开关
		if(true == m_tDsn.bIsPeerRep || true == m_tDsn.bIsRep) 
		{
			m_tDsn.m_bIsShardBackup = true;
		}

		if(true == bIsShard_tab && true == m_tDsn.m_bIsShardBackup)
		{
			m_bStartShardBackupProc = true;
		}
		else
		{
			m_bStartShardBackupProc = false;
		}
		
	
        for(int i = 0; i<m_iTableSpaceCounts; i++)
        {
            if((m_pTableSpace[i]->iPageSize*m_pTableSpace[i]->iRequestCounts) >= m_tDsn.iDataSize)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Share memory is too small,data-size is=[%ld],tableSpace=[%s].",m_tDsn.iDataSize,m_pTableSpace[i]->sName);
                return ERROR_UNKNOWN;
            }
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称  :  CheckPKIsIndexed()
    * 函数描述  :  检查主键列是否建索引
    * 输入		:  pTable 表对象
    * 输出		:  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbConfig::CheckPKIsIndexed(TMdbTable * pTable)
    {
        int k = 0;
        int iRet = 0;
        CHECK_OBJ(pTable);
        //校验主键上必须有索引
        for(k = 0;k < pTable->iIndexCounts;++k)
        {
           int n = 0;
           bool bExist= true;//假设符合
           for(n = 0;n < MAX_INDEX_COLUMN_COUNTS;++n)
           {
                if(pTable->tIndex[k].iColumnNo[n] < 0){break;}
                int m = 0;
                for(m = 0;m < pTable->m_tPriKey.iColumnCounts;++m)
                {
                    if(pTable->tIndex[k].iColumnNo[n] == pTable->m_tPriKey.iColumnNo[m])
                    {//有主键
                        break;
                    }
                }
                if(m == pTable->m_tPriKey.iColumnCounts)
                {
                    bExist = false;
                    break;
                }
            }
            if(bExist){break;}
        }
        if(k == pTable->iIndexCounts)
        {//都找完了，都没找到,需要自动添加主键
            pTable->iIndexCounts ++;
            pTable->tIndex[k].Clear();//新增索引K
            snprintf(pTable->tIndex[k].sName,sizeof(pTable->tIndex[k].sName),"_auto_pk_index");
            pTable->tIndex[k].iPriority = 1;
            memcpy(pTable->tIndex[k].iColumnNo,pTable->m_tPriKey.iColumnNo,
                sizeof(pTable->tIndex[k].iColumnNo[0])*pTable->m_tPriKey.iColumnCounts);
            if(pTable->m_tPriKey.iColumnCounts > 1)
            {
                pTable->tIndex[k].m_iIndexType = HT_CMP;
            }
            else
            {
               pTable->tIndex[k].m_iIndexType = 
                    (DT_Int == pTable->tColumn[pTable->tIndex[k].iColumnNo[0]].iDataType)?HT_Int : HT_Char;
            }
            TADD_DETAIL("Add PK Index to table[%s]",pTable->sTableName);
        }
        if(k != 0)
        {//将主键索引放在第一个
            TMdbIndex tTempIndex = pTable->tIndex[k];
            pTable->tIndex[k] = pTable->tIndex[0];
            pTable->tIndex[0] = tTempIndex;
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  GetTable()
    * 函数描述	:  获取某个表信息
    * 输入		:  pszTable, 表名称
    * 输出		:  无
    * 返回值	:  成功返回表结构指针，否则返回NULL
    * 作者		:  li.shugang
    *******************************************************************************/
    TMdbTable* TMdbConfig::GetTable(const char* pszTable)
    {
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            if(m_pTable[i] == NULL)
                continue;

            if(TMdbNtcStrFunc::StrNoCaseCmp(m_pTable[i]->sTableName, pszTable) == 0)
            {
                return m_pTable[i];
            }
        }
        return NULL;
    }

    TMdbTable* TMdbConfig::GetOldTableStruct(const char* pszTabName)
    {

        std::vector<TMdbTable*>::iterator itor = m_vpOldTabStruct.begin();
        for(; itor != m_vpOldTabStruct.end(); ++itor)
        {

            if(TMdbNtcStrFunc::StrNoCaseCmp((*itor)->sTableName, pszTabName) == 0)
            {
                return (*itor);
            }
        }
        
        return NULL;
    }

    /******************************************************************************
    * 函数名称	:  IsAdmin()
    * 函数描述	:  判断当前用户是否是管理员权限
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  是返回true，否则返回false
    * 作者		:  li.shugang
    *******************************************************************************/
    bool TMdbConfig::IsAdmin()
    {
        if(m_cAccess == 'A')
            return true;
        else
            return false;

    }

    TMdbTableSpace* TMdbConfig::GetIdleTableSpace()
    {
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        { 
            if(i >= 490)
            {
                continue;
            }
            if(m_pTableSpace[i] == NULL)
            {
                m_pTableSpace[i] = new(std::nothrow) TMdbTableSpace();
                return m_pTableSpace[i];
            }
        }
        return NULL;
    }


    TMdbTable* TMdbConfig::GetIdleTable()
    {
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        { 
            if(m_pTable[i] == NULL)
            {
                m_pTable[i] = new(std::nothrow) TMdbTable();
                return m_pTable[i];
            }
			if(m_pTable[i] != NULL && m_pTable[i]->sTableName[0] == 0 )
			 	return m_pTable[i];
        }
        return NULL;
    }


    /******************************************************************************
    * 函数名称	:  GetConfigHomePath()
    * 函数描述	:  获取配置文件 homepath
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  配置文件路径
    * 作者		:  li.shugang
    *******************************************************************************/
    char * TMdbConfig::GetConfigHomePath()
    {
        if(m_szHome[0] == 0)
        {
            return NULL;
        }
        else
        {
            return m_szHome;
        }
    }

    /******************************************************************************
    * 函数名称	:  SetConfigHomePath()
    * 函数描述	:  设置配置文件home路径
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  设置成功返回0,否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::SetConfigHomePath(const char* pszDsn)
    {
        int iRet = 0;

#ifndef WIN32
        if (NULL == getenv("QuickMDB_HOME"))
        {
            return -1;
        }
        char *pszHome = getenv("QuickMDB_HOME");
#else
        char *pszHome = getenv("QuickMDB_HOME");
#endif

        char sConfigHome[MAX_PATH_NAME_LEN] = {0};

        SAFESTRCPY(sConfigHome,sizeof(sConfigHome),pszHome);
        if(sConfigHome[strlen(sConfigHome)-1] != '/')
        {
            sConfigHome[strlen(sConfigHome)] = '/';
        }
#ifndef WIN32
        char sTempzhy[MAX_PATH_NAME_LEN] = {0};
        snprintf(sTempzhy,sizeof(sTempzhy),"%s.config/",sConfigHome);
        snprintf(sConfigHome,sizeof(sConfigHome), "%s",sTempzhy) ;
#else
        char sTempzhy[MAX_PATH_NAME_LEN] = {0};
        snprintf(sTempzhy,sizeof(sTempzhy),"%sconfig/",sConfigHome);
        snprintf(sConfigHome,sizeof(sConfigHome), "%s",sTempzhy) ;
#endif

        SAFESTRCPY(m_szHome,sizeof(m_szHome),sConfigHome);
        char   dsnName[MAX_NAME_LEN] = {0};
        memset(dsnName,0,sizeof(dsnName));
        SAFESTRCPY(dsnName, sizeof(dsnName), pszDsn);
        TMdbNtcStrFunc::ToUpper(dsnName);
        memset(sTempzhy,0,sizeof(sTempzhy));
        snprintf(sTempzhy,sizeof(sTempzhy),"%s.%s/",sConfigHome,dsnName);
        snprintf(sConfigHome,sizeof(sConfigHome), "%s",sTempzhy) ;
        if(TMdbNtcDirOper::IsExist(sConfigHome))
        {
            SAFESTRCPY(m_szHome,sizeof(m_szHome),sConfigHome);
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  IsExist()
    * 函数描述	:  检查配置文件是否存在
    * 输入		:  pszDsn DSN名称
    * 输出		:  无
    * 返回值	:  存在返回true,否则返回false
    * 作者		:  li.shugang
    *******************************************************************************/
    bool TMdbConfig::IsExist(const char* pszDsn)
    {
        int iRet = 0;

        char   dsnName[MAX_NAME_LEN] = {0};
        memset(dsnName,0,sizeof(dsnName));
        SAFESTRCPY(dsnName, sizeof(dsnName),pszDsn );
        TMdbNtcStrFunc::ToUpper(dsnName);
        
        if(SetConfigHomePath(dsnName) != 0)
        {
            //设置配置文件home路径
            return false;
        }
        
#ifndef WIN32
        sprintf(m_sCfgFile, "%s.QuickMDB_SYS_%s.xml", m_szHome,dsnName);
#else
        sprintf(m_sCfgFile, "%sSYS_%s.xml", m_szHome,dsnName);
#endif

        //检测文件是否存在
#ifndef WIN32
        iRet = access(m_sCfgFile, F_OK);
        if(iRet != 0)
        {
            TADD_ERROR(ERROR_UNKNOWN, "Sys configuration not found.");
            return false;
        }
        return true;
#else
        iRet = access(m_sCfgFile, 0x00);
        if(iRet != 0)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Sys configuration not found.");
            return false;
        }
        return true;
#endif
    }


    /******************************************************************************
    * 函数名称	:  CheckLicense()
    * 函数描述	:  检测序列号是否正确
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  正确则返回true，否则返回false
    * 作者		:  li.shugang
    *******************************************************************************/
    bool TMdbConfig::CheckLicense()
    {
        //如果不需要检测序列号，直接返回
        if(m_tDsn.bIsLicense == false)
        {
            return true;
        }

        char sLinceseFile[128];
        memset(sLinceseFile, 0, sizeof(sLinceseFile));

        //拼写出文件名
#ifndef WIN32
        if(getenv("QuickMDB_HOME") == NULL)
            return false;
        sprintf(sLinceseFile, "%s/etc/minidb.license", getenv("QuickMDB_HOME"));
#else
        sprintf(sLinceseFile, "%s\\config\\minidb.license",getenv("QuickMDB_HOME"));
#endif

        //检测文件是否存在
#ifndef WIN32
        int iRet = access(sLinceseFile, F_OK);
#else
        int iRet = access(sLinceseFile, 0x00);
#endif

        //如果文件不存在返回false
        if(iRet != 0)
        {
            return false;
        }

        //获取实际需要的license
        char sLincese[128];
        GetLincese(sLincese);

        //查看文件中的license
        char sFileL[128];
        memset(sFileL, 0, sizeof(sFileL));

        FILE* fp = fopen (sLinceseFile, "r");
        if(fp == NULL)
        {
            return false;
        }

        fread(sFileL, 16, 1, fp);
        //fgets(sFileL, 17, fp);
        SAFE_CLOSE(fp);
        /*
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("TMdbConfig::CheckLicense() : Need=[%s], Read=[%s].", sLincese, sFileL);
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        */
        //进行比较
        if(strcmp(sFileL, sLincese) == 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }


    /******************************************************************************
    * 函数名称	:  GetLincese()
    * 函数描述	:  获取配置文件 homepath
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbConfig::GetLincese(char* pLicense)
    {
        TMdbNtcSplit tSplit;
        tSplit.SplitString(m_tDsn.sLocalIP,'.');
        //tSplit.SetString(m_tDsn.sLocalIP);

        int iUserID = 0;
#ifndef WIN32
        iUserID = getuid();
#endif

        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("LocalIP=[%s], UID=[%d].", m_tDsn.sLocalIP, iUserID);
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");
        TADD_NORMAL("");

        pLicense[0] = 0;
        if(tSplit.GetFieldCount() != 4)
        {
            return;
        }

        for(int i=0; i<4; ++i)
        {
            sprintf(&pLicense[strlen(pLicense)], "%c%c%c", atoi(tSplit[i])%13+iUserID%13+'A',
                atoi(tSplit[i])%17+'B', iUserID%19+'C');
        }

        sprintf(&pLicense[strlen(pLicense)], "%c", (atoi(tSplit[0])+iUserID)%11+'A');
        sprintf(&pLicense[strlen(pLicense)], "%c", (atoi(tSplit[1])+iUserID)%17+'A');
        sprintf(&pLicense[strlen(pLicense)], "%c", (atoi(tSplit[2])+iUserID)%19+'A');
        sprintf(&pLicense[strlen(pLicense)], "%c", (atoi(tSplit[3])+iUserID)%23+'A');
    }
    /******************************************************************************
    * 函数名称	:  GetTableCounts()
    * 函数描述	:  获取表数目
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  返回表数目
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::GetTableCounts()
    {
        return m_iTableCounts;
    }


    /******************************************************************************
    * 函数名称	:  GetTableSpaceCounts()
    * 函数描述	:  获取表空间数目
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  返回表空间数目
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::GetTableSpaceCounts()
    {
        return m_iTableSpaceCounts;
    }


    /******************************************************************************
    * 函数名称	:  GetTableByPos()
    * 函数描述	:  根据位置获取表信息
    * 输入		:  iPos，表的位置
    * 输出		:  无
    * 返回值	:  返回表信息指针
    * 作者		:  li.shugang
    *******************************************************************************/
    TMdbTable* TMdbConfig::GetTableByPos(int iPos)
    {
        if(iPos>=0 && iPos<MAX_TABLE_COUNTS)
            return m_pTable[iPos];
        else
            return NULL;
    }

    TMdbTable* TMdbConfig::GetTableByTableId(int iTableId)
    {
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            if(m_pTable[i] != NULL)
            {
                if(m_pTable[i]->m_iTableId == iTableId && m_pTable[i]->m_iTableId != -1)
                {
                    return m_pTable[i];
                }
            }
        }
        return NULL;
    }


    /******************************************************************************
    * 函数名称  :  GetTableByName()
    * 函数描述  :  根据表名获取表信息
    * 输入		:  pszTableName，表名
    * 输出		:  无
    * 返回值	:  返回表信息指针
    * 作者		:  cao.peng
    *******************************************************************************/
    TMdbTable* TMdbConfig::GetTableByName(const char* pszTableName)
    {
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            if(m_pTable[i] != NULL)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(m_pTable[i]->sTableName,pszTableName) == 0)
                {
                    return m_pTable[i];
                }
            }
        }
        return NULL;
    }

    TMdbTable* TMdbConfig::GetTableByIdxName(const char* pszIdxName)
    {
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            if(m_pTable[i] == NULL)
                continue;

            for(int j = 0; j< m_pTable[i]->iIndexCounts;j++)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(m_pTable[i]->tIndex[j].sName, pszIdxName) == 0)
                {
                    return m_pTable[i];
                }
            }
        }
        return NULL;
    }

    /******************************************************************************
    * 函数名称	:  GetTableSpace()
    * 函数描述	:  获取表空间信息
    * 输入		:  iPos，表空间的位置
    * 输出		:  无
    * 返回值	:  返回表空间信息指针
    * 作者		:  li.shugang
    *******************************************************************************/
    TMdbTableSpace* TMdbConfig::GetTableSpace(int iPos)
    {
        if(iPos>=0 && iPos<m_iTableSpaceCounts)
            return m_pTableSpace[iPos];
        else
            return NULL;
    }

    //通过表空间名获取表空间信息
    TMdbTableSpace* TMdbConfig::GetTableSpaceByName(const char* pszTablespaceName)
    {
        int iPos = 0;
        for(iPos = 0; iPos < MAX_TABLE_COUNTS; iPos++)
        {
            if(m_pTableSpace[iPos] == NULL || m_pTableSpace[iPos]->sName[0] == 0)
            {
                continue;
            }
            if(TMdbNtcStrFunc::StrNoCaseCmp(m_pTableSpace[iPos]->sName,pszTablespaceName) == 0)
            {
                return m_pTableSpace[iPos];
            }
        }
        return NULL;
    }

    /******************************************************************************
    * 函数名称	:  GetSeqCounts()
    * 函数描述	:  获取序列的数目
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  返回序列的数目
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::GetSeqCounts()
    {
        return m_iSeqCounts;
    }

    /******************************************************************************
    * 函数名称	:  GetUserCounts()
    * 函数描述	:  获取用户的数目
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  返回用户的数目
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::GetUserCounts()
    {
        return m_iUserCounts;
    }

    /******************************************************************************
    * 函数名称	:  SetUserCounts()
    * 函数描述	:  设置用户个数
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  无
    * 作者		:  li.shugang
    *******************************************************************************/
    void TMdbConfig::SetUserCounts(int iCount)
    {
        m_iUserCounts = iCount;
    }

    void TMdbConfig::SetTablespaceCounts(int iCount)
    {
        m_iTableSpaceCounts = iCount;
    }

    void TMdbConfig::SetTableCounts(int iCount)
    {
        m_iTableCounts = iCount;
    }

    /******************************************************************************
    * 函数名称	:  GetSequence()
    * 函数描述	:  获取序列的信息
    * 输入		:  iPos，序列的位置
    * 输出		:  无
    * 返回值	:  返回序列信息指针
    * 作者		:  li.shugang
    *******************************************************************************/
    TMemSeq* TMdbConfig::GetSequence(int iPos)
    {
        if(iPos >= m_iSeqCounts)
            return NULL;
        else
            return m_pSeq[iPos];
    }


    /******************************************************************************
    * 函数名称	:  GetDSN()
    * 函数描述	:  获取DSN信息
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  返回DSN信息指针
    * 作者		:  li.shugang
    *******************************************************************************/
    TMdbCfgDSN* TMdbConfig::GetDSN()
    {
        return &m_tDsn;
    }


    /******************************************************************************
    * 函数名称	:  GetProAttr()
    * 函数描述	:  获取TMdbCfgProAttr信息
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  返回TMdbCfgProAttr信息指针
    * 作者		:  li.shugang
    *******************************************************************************/
    TMdbCfgProAttr* TMdbConfig::GetProAttr()
    {
        return &m_tProAttr;
    }

    TMDbUser* TMdbConfig::GetUser(const char* pszUserName)
    {
        for (int i = 0; i < MAX_USER_COUNT; i++)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pszUserName,m_pUser[i]->sUser) == 0)
            {
                return m_pUser[i];
            }
        }
        return NULL;
    }

    /******************************************************************************
    * 函数名称	:  CheckParam()
    * 函数描述	:  检查用户名和密码的正确性
    * 输入		:  pszUser, 用户名
    * 输入		:  pszPWD, 密码
    * 输入		:  pszDsn, DSN名称
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbConfig::CheckParam(const char* pszUser, const char* pszPWD, const char* pszDsn)
    {
        TADD_FUNC("TMdbConfig::CheckParam(%s/%s@%s) : Start.", pszUser, pszPWD, pszDsn);

        bool userflag = false;
        bool passwordflag = false;
        char sPwd[MAX_NAME_LEN] = {0};
        for (int i = 0; i < m_iUserCounts; i++)
        {
            //printf("m_pUser=%s,sPwd=%s\n",m_pUser[i]->sUser,m_pUser[i]->sPwd);
            if(TMdbNtcStrFunc::StrNoCaseCmp(pszUser,m_pUser[i]->sUser) == 0)
            {
                userflag = true;
                SAFESTRCPY(sPwd,sizeof(sPwd), m_pUser[i]->sPwd);
                if(TMdbEncrypt::IsEncryptStr(pszPWD))
                {
                    memset(sPwd,0,sizeof(sPwd));
                    TMdbEncrypt::EncryptEx(m_pUser[i]->sPwd,sPwd);
                }

                if(TMdbNtcStrFunc::StrNoCaseCmp(pszPWD,sPwd) == 0)
                {
                    passwordflag = true;
                    if (TMdbNtcStrFunc::StrNoCaseCmp(m_pUser[i]->sAccess,"Administrator") == 0)
                    {
                        m_cAccess = 'A';
                    }
                    else if (TMdbNtcStrFunc::StrNoCaseCmp(m_pUser[i]->sAccess,"Read-Write") == 0)
                    {
                        m_cAccess = 'W';
                    }
                    else
                    {
                        m_cAccess = 'R';
                    }
                    break;
                }
            }
        }

        if (false == userflag)
        {
            TADD_ERROR(ERR_DB_USER_INVALID,"TMdbConfig::CheckParam(%s/%s@%s) : username.",
                pszUser, pszPWD, m_sDSN);
            return ERR_DB_USER_INVALID;
        }

        if(false == passwordflag)
        {
            TADD_ERROR(ERR_DB_PASSWORD_INVALID,"TMdbConfig::CheckParam(%s/%s@%s) : password.",
                pszUser, pszPWD, m_sDSN);
            return ERR_DB_PASSWORD_INVALID;
        }


        TADD_FUNC("TMdbConfig::CheckParam(%s) : Finish(true).", m_sDSN);
        return 0;
    }


    /******************************************************************************
    * 函数名称	:  IsWriteUnLock()
    * 函数描述	:  是否有类似OCS_SESSION一类的频繁插入/删除的表
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  有则返回true，否则返回false
    * 作者		:  li.shugang
    *******************************************************************************/
    bool TMdbConfig::IsWriteUnLock()
    {
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            if(m_pTable[i] == NULL)
                continue;

            if(m_pTable[i]->bWriteLock == false)
            {
                return true;
            }
        }

        return false;
    }

    /******************************************************************************
    * 函数名称	:  GetDataShmCounts
    * 函数描述	:  获取需要的数据共享块
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbConfig::GetDbDataShmCounts(int &iDataShmCounts)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        long long llSize = 0;
        //依次计算各个表的页面数(数据+冲突索引)
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            //取到表信息指针
            TMdbTable* pTable = GetTableByPos(i);
            if(pTable == NULL)
            {
                continue;
            }
            long long llTempSize = 0;
            CHECK_RET(GetOneTableDataSize(pTable,llTempSize),"GetOneTableDataSize failed.");
            llSize += llTempSize;
        }
        //另外给表空间2*AskPage的空间作扩展用
        for(int j=0; j<GetTableSpaceCounts(); ++j)
        {
            TADD_DETAIL("iRequestCounts=%d.", GetTableSpace(j)->iRequestCounts);
            llSize += 2 * GetTableSpace(j)->iRequestCounts * GetTableSpace(j)->iPageSize;
        }
        TADD_DETAIL("iSize=%ld.", llSize);
        //计算出所需要的共享内存数
        iDataShmCounts  = llSize /GetDSN()->iDataSize + 1;
        TADD_FLOW("DataShmCount =%d.", iDataShmCounts);
        TADD_FUNC("Finish.");
        return iRet;    
    }

    /******************************************************************************
    * 函数名称	:  GetOneTableDataSize
    * 函数描述	:  获取某张表所需要的数据空间大小
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbConfig::GetOneTableDataSize(TMdbTable * pTable,long long &llDataSize)
    {
        int iRet = 0;
        //取到表信息指针
        CHECK_OBJ(pTable);
        //取得表空间信息指针
        TMdbTableSpace* pTableSpace = GetTableSpaceByName(pTable->m_sTableSpace);
        CHECK_OBJ(pTableSpace);
        //一条记录的长度
        TADD_FLOW("table_name= %s,iOneRecordSize=%d, iPageSize=%lu.", 
            pTable->sTableName, pTable->iOneRecordSize, pTableSpace->iPageSize);
        //页记录大小= 表记录大小+一个TMdbPageNode + 几个记录索引位置
        int iOneTotalSize = pTable->iOneRecordSize +  sizeof(TMdbPageNode);
        int iRecCounts     = (pTableSpace->iPageSize - sizeof(TMdbPage)) /iOneTotalSize;
        TADD_FLOW("PageSize=%d, iRecCounts=%d, iOneTotalSize=%d.", pTableSpace->iPageSize, iRecCounts, iOneTotalSize);
        if(iRecCounts < 10 || iRecCounts > 10000)
        {
            TADD_ERROR(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"the PageSize is[%d],it's too small,plealse change it!", pTableSpace->iPageSize);
            return ERR_APP_CONFIG_ITEM_VALUE_INVALID;
        }
        //计算所需页面数
        int iPageCounts = pTable->iRecordCounts/iRecCounts + 1;
        TADD_FLOW("iPageCounts=%d.", iPageCounts);
        //计算所需空间
        llDataSize = ((long long)iPageCounts) * pTableSpace->iPageSize;
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  GetOneTableSpaceDataSize
    * 函数描述	:  获取某个表空间需要的数据空间大小
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbConfig::GetOneTableSpaceDataSize(char* psTableSpaceName,long long &llDataSize)
    {
        int iRet = 0;
        int i = 0;
        llDataSize = 0;
        for(i = 0;i < MAX_TABLE_COUNTS;i++)
        {
            if(m_pTable[i] != NULL && 0 == TMdbNtcStrFunc::StrNoCaseCmp(m_pTable[i]->m_sTableSpace,psTableSpaceName ) )
            {
                long long llTempSize = 0;
                CHECK_RET(GetOneTableDataSize(m_pTable[i],llTempSize),"GetOneTableDataSize(table=%d) failed",m_pTable[i]->sTableName);
                llDataSize += llTempSize;
            }
        }
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  IsDiffToMemTable
    * 函数描述	:  是否和内存表一致
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool TMdbConfig::IsDiffToMemTable(TMdbTable * pCfgTable,TMdbTable * pMemTable)
    {
        bool bDiffTable = false;
        //表属性校验
        if(!TablePropertyCmp(pCfgTable,pMemTable))
        {
            bDiffTable = true;
        }
        //校验列属性
        if(!bDiffTable && !ColumnAttrCmp(pCfgTable,pMemTable))
        {
            bDiffTable = true;
        }
        //校验索引信息
        if(!bDiffTable && !IndexCmp(pCfgTable,pMemTable))
        {
            bDiffTable = true;
        }
        //校验主键信息
        if(!bDiffTable && !PrimaryKeyCmp(pCfgTable,pMemTable))
        {
            bDiffTable = true;
        }
        return bDiffTable;
    }
    /******************************************************************************
    * 函数名称	:  LoadMdbJob
    * 函数描述	:  上载mdb的job
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbConfig::LoadMdbJob()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sJobFile[MAX_FILE_NAME] = {0};
        sprintf(sJobFile,"%s.JOB/.QuickMDB_JOB.xml",m_szHome);
        if(false == TMdbNtcFileOper::IsExist(sJobFile))
        {//没有job 文件
            TADD_FLOW("No job info find.");
            return 0;
        }
        //读取配置文件
        MDBXMLDocument tDoc(sJobFile);
        if (false == tDoc.LoadFile())
        {
            CHECK_RET(ERR_APP_LAOD_CONFIG_FILE_FALIED,"Load job configuration failed.",sJobFile);
        }
        MDBXMLElement* pRoot = tDoc.FirstChildElement("MDB_JOB");
        CHECK_OBJ(pRoot);
        m_vMdbJob.clear();
        MDBXMLElement* pEle = NULL;
        for(pEle=pRoot->FirstChildElement("job"); pEle; pEle=pEle->NextSiblingElement("job"))
        {
            TMdbJob tJob;
            const char * pValue = NULL;
            //name
            pValue =  pEle->Attribute("name");
            CHECK_OBJ(pValue);
            SAFESTRCPY(tJob.m_sName,sizeof(tJob.m_sName),pValue);
            //nextdate
            pValue =  pEle->Attribute("exec_date");
            CHECK_OBJ(pValue);
            if(TMdbNtcStrFunc::IsDateTime(pValue) == false)
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"[%s] is not a date",pValue);
            }
            SAFESTRCPY(tJob.m_sExecuteDate,sizeof(tJob.m_sExecuteDate),pValue);
            //interval
            pValue =  pEle->Attribute("interval");
            CHECK_OBJ(pValue);
            tJob.m_iInterval = TMdbNtcStrFunc::StrToInt(pValue);
            //ratetype
            pValue =  pEle->Attribute("ratetype");
            CHECK_OBJ(pValue);
            CHECK_RET(tJob.SetRateType(pValue),"SetRateType faild.");
            //sql
            pValue =  pEle->Attribute("sql");
            CHECK_OBJ(pValue);
            if(0 == pValue[0])
            {
                CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"please input sql for job");
            }
            SAFESTRCPY(tJob.m_sSQL,sizeof(tJob.m_sSQL),pValue);
            //检测job是否已存在
            std::vector<TMdbJob>::iterator itor = m_vMdbJob.begin();
            for(;itor != m_vMdbJob.end();++itor)
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sName,tJob.m_sName) == 0)
                {
                    CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,"job[%s] is duplicate",tJob.m_sName);
                }
            }
            m_vMdbJob.push_back(tJob);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  HasWhereCond()
    * 函数描述	:  判断一张表的Load SQL语句中是否包含where条件
    * 输入		:  sTableName 表名
    * 输入		:  sLoadSQL sql语句
    * 输出		:  无
    * 返回值	:  存在返回true,否则返回false
    * 作者		:  jiang.lili
    *******************************************************************************/
    bool TMdbConfig::HasWhereCond(const char* sTableName, const char * sLoadSQL)
    {
        bool bRet = false;
        char sTmpSQL[MAX_SQL_LEN];
        memset(sTmpSQL, 0, sizeof(sTmpSQL));

        //去除LoadSQL中连续空格情况
        int iLen = 0;
        for (int i = 0;  i<MAX_SQL_LEN && sLoadSQL[i] != '\0' ; i++)
        {
            if (iLen-1 >=0 && ' ' == sTmpSQL[iLen-1] &&  ' ' == sLoadSQL[i])
            {
                continue;
            }
            sTmpSQL[iLen] = sLoadSQL[i];
            iLen ++;
        }

        TMdbNtcSplit tSplit;
        tSplit.SplitString(sTmpSQL, ' ');
        for (unsigned int i = 0; i < tSplit.GetFieldCount()-2; i++)
        {
            if (TMdbNtcStrFunc::StrNoCaseCmp(tSplit[i], "from") == 0
                && TMdbNtcStrFunc::StrNoCaseCmp(tSplit[i+1], sTableName) == 0
                && TMdbNtcStrFunc::StrNoCaseCmp(tSplit[i+2], "where") == 0)
            {
                bRet = true;
                break;
            }
        }

        return bRet;
    }

    /******************************************************************************
    * 函数名称	:  IndexCmp
    * 函数描述	:  比较两个表的属性是否一致
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  true 一致;false 不一致
    * 作者		:  cao.peng
    *******************************************************************************/
    bool TMdbConfig::IndexCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable)
    {
        bool bSame = true;
        for(int j = 0; j < pSrcTable->iIndexCounts;++j)
        {
            TMdbIndex & tCfgIndex = pSrcTable->tIndex[j];
            TMdbIndex & tMemIndex = pDesTable->tIndex[j];
            int k = 0;
            for(k = 0; k < MAX_INDEX_COLUMN_COUNTS;++k)
            {
                if(tCfgIndex.iColumnNo[k] != tMemIndex.iColumnNo[k])
                {
                    bSame = false;
                    break;
                }
            }
            //如果索引对应的列号不一致，直接返回false
            if(!bSame)
            {
                return bSame;
            }
            if(tCfgIndex.m_iIndexType != tMemIndex.m_iIndexType 
                || tCfgIndex.iPriority != tMemIndex.iPriority
                || TMdbNtcStrFunc::StrNoCaseCmp(tCfgIndex.sName,tMemIndex.sName) != 0)
            {
                bSame = false;
                break;
            }
        }
        return bSame;
    }

    /******************************************************************************
    * 函数名称	:  ColumnAttrCmp
    * 函数描述	:  比较两个表的列是否一样
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  true 一致;false 不一致
    * 作者		:  cao.peng
    *******************************************************************************/
    bool TMdbConfig::ColumnAttrCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable)
    {
        bool bSame = true;
        for(int j = 0; (j< pSrcTable->iColumnCounts) ;++j)
        {
            TMdbColumn & tCfgColumn = pSrcTable->tColumn[j];
            TMdbColumn & tMemColumn = pDesTable->tColumn[j];
            if(TMdbNtcStrFunc::StrNoCaseCmp(tCfgColumn.sName,tMemColumn.sName) != 0 
                ||tCfgColumn.iPos != tMemColumn.iPos 
                ||tCfgColumn.iDataType != tMemColumn.iDataType 
                ||tCfgColumn.iColumnLen != tMemColumn.iColumnLen 
                ||tCfgColumn.bIsDefault != tMemColumn.bIsDefault 
                ||TMdbNtcStrFunc::StrNoCaseCmp(tCfgColumn.iDefaultValue,tMemColumn.iDefaultValue) != 0)
            {
                bSame = false;
                break;
            }
        }
        return bSame;
    }

    /******************************************************************************
    * 函数名称	:  PrimaryKeyCmp
    * 函数描述	:  比较两个表的主键是否一致
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  true 一致;false 不一致
    * 作者		:  cao.peng
    *******************************************************************************/
    bool TMdbConfig::PrimaryKeyCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable)
    {
        bool bSame = true;
        for(int j = 0; j < pSrcTable->m_tPriKey.iColumnCounts;++j)
        {
            if(pSrcTable->m_tPriKey.iColumnNo[j] != pDesTable->m_tPriKey.iColumnNo[j])
            {
                bSame = true;
                break;
            }
        }
        return bSame;
    }

    /******************************************************************************
    * 函数名称	:  PrimaryKeyCmp
    * 函数描述	:  比较两个表的主键是否一致
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  true 一致;false 不一致
    * 作者		:  cao.peng
    *******************************************************************************/
    bool TMdbConfig::TablePropertyCmp(TMdbTable * pSrcTable,TMdbTable * pDesTable)
    {
        bool bSame = true;
        if(
            0 != TMdbNtcStrFunc::StrNoCaseCmp(pSrcTable->sTableName, pDesTable->sTableName)
            || 0 != TMdbNtcStrFunc::StrNoCaseCmp(pSrcTable->m_sTableSpace, pDesTable->m_sTableSpace)
            || pSrcTable->iExpandRecords  != pDesTable->iExpandRecords 
            || pSrcTable->bReadLock  != pDesTable->bReadLock 
            || pSrcTable->bWriteLock  != pDesTable->bWriteLock 
            || pSrcTable->bRollBack  != pDesTable->bRollBack 
            || pSrcTable->m_cZipTimeType!= pDesTable->m_cZipTimeType 
            || pSrcTable->bIsCheckPriKey != pDesTable->bIsCheckPriKey 
            || pSrcTable->bIsPerfStat  != pDesTable->bIsPerfStat 
            || pSrcTable->iLoadType  != pDesTable->iLoadType 
            || pSrcTable->iColumnCounts != pDesTable->iColumnCounts 
            || pSrcTable->iIndexCounts  != pDesTable->iIndexCounts 
            || pSrcTable->m_tPriKey.iColumnCounts != pDesTable->m_tPriKey.iColumnCounts)
        {
            bSame  = false;
        }
        return bSame;
    }

    /******************************************************************************
    * 函数名称	:  CheckRepeatTableName
    * 函数描述	:  判断上载的表中是否存在重复的表名
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  true 存在;false 不存在
    * 作者		:  cao.peng
    *******************************************************************************/
    bool TMdbConfig::CheckRepeatTableName(const char* pTableName)
    {
        bool bRepeat = false;
        for(int i = 0; i<MAX_TABLE_COUNTS; i++)
        {
            if (m_pTable[i] != NULL)
            {
                if (TMdbNtcStrFunc::StrNoCaseCmp(pTableName,m_pTable[i]->sTableName) == 0)
                {
                    bRepeat = true;
                    break;
                }
            }
        }
        return bRepeat;
    }

    /******************************************************************************
    * 函数名称	:  IsValidName
    * 函数描述	:  判断表名、列名、索引名是否合法
    * 输入		:  pName表名或列名
    * 输出		:  无
    * 返回值	:  true 和法;false 非法
    * 作者		:  cao.peng
    *******************************************************************************/
    bool TMdbConfig::IsValidName(const char *pName)
    {
        if(NULL == pName)
        {
            return false;
        }
        //名称只能由字母数字以及_下化线组长的字符串
        int iLen = strlen(pName);
    	for(int i=0; i<iLen; ++i)
    	{
    	    if(pName[i]>='A' && pName[i]<='Z')
            {
                continue;
            }   
            else if(pName[i]>='a' && pName[i]<='z')
            {
                continue;
            }
    		else if(pName[i] == '_' || isdigit(pName[i]))
            {
                continue;
            }   
            else
            {   
                return false;
            }
    	}
        return true;
    }

    //单独上载DSN配置文件
    int TMdbConfig::LoadDsnConfigFile(const char* pszDsn,const bool bCheck)
    {
        int iRet = 0;
        CHECK_OBJ(pszDsn);
        SAFESTRCPY(m_sDSN,sizeof(m_sDSN),pszDsn);
        if (!IsExist(pszDsn))
        {
            return ERR_APP_LAOD_CONFIG_FILE_FALIED;
        }
        CHECK_RET(LoadSysCfg(m_sCfgFile,bCheck),"LoadSysCfg(name=%s) failed.",m_sCfgFile);
        return iRet;
    }
    int TMdbConfig::LoadTableSpaceAlterInfo(const char* psSysFilePath)
    {
        int iRet = 0;
        //TADD_DETAIL("sys config dir[%s]",psSysFilePath);

        m_tTsAlterInfo.Clear();
        
        char sTSUpdateFile[MAX_FILE_NAME] = {0};
        snprintf(sTSUpdateFile, MAX_FILE_NAME, "%s.TableSpace_update.xml", psSysFilePath);
        if(false == TMdbNtcFileOper::IsExist(sTSUpdateFile))
        {
            TADD_DETAIL("tablespace have no struct modify info.",sTSUpdateFile);
            return iRet;
        }

        MDBXMLDocument tDoc(sTSUpdateFile);
        if (false == tDoc.LoadFile())
        {
            TADD_ERROR(-1,"Load tablespace configuration update failed!");
            return -1;
        }
        MDBXMLElement* pRoot = tDoc.FirstChildElement("MDBConfig");
        if(pRoot == NULL)
        {
            TADD_ERROR(-1,"Not find element=[MDBConfig] when loading tablespace configuration update.");
            return -1;
        }
        
        char *pValue = NULL;
        for(MDBXMLElement* pEle=pRoot->FirstChildElement("table-space"); pEle; pEle=pEle->NextSiblingElement("table-space"))
        {
            // 增加的表空间
            pValue = GetTablePropertyValue(pEle,"add-ts");
            if(pValue != NULL)
            {
                m_tTsAlterInfo.m_vAddTs.push_back(pValue);
            }

            // 删除的表空间
            pValue = GetTablePropertyValue(pEle,"del-ts");
            if(pValue != NULL)
            {
                m_tTsAlterInfo.m_vDelTs.push_back(pValue);
            }

            // 变更page-size 的表空间
            for(MDBXMLElement* pCol=pEle->FirstChildElement("mod-pagesize"); pCol; pCol=pCol->NextSiblingElement("mod-pagesize"))
            {
                MDBXMLAttribute* pAttr = NULL; 
                
                TIntAlterAttr tAlter;
                tAlter.Clear();

                std::string sTsName;

                bool bName =false;
                bool bOld = false;
                bool bNew = false;
        
                for(pAttr=pCol->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                    {
                        sTsName = pAttr->Value();
                        bName = true;
                    }
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "old-value") == 0)
                    {
                        tAlter.m_iOldValue = GetDataType(pAttr->Value());
                        bOld = true;
                    }
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "new-value") == 0)
                    {
                        tAlter.m_iNewValue = GetDataType(pAttr->Value());
                        bNew = true;
                    }
                    else
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"The ATTR name[%s] is invalid, please check the  configuration.",pAttr->Value());
                        return ERR_APP_INVALID_PARAM;
                    }
                }

                if(!bName || !bOld || !bNew)
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"config is not integrated.");
                    return ERR_APP_INVALID_PARAM;
                }
                m_tTsAlterInfo.m_vPageSizeAlter[sTsName] = tAlter;
            }
        }
        return iRet;
    }

    int TMdbConfig::LoadTableOldConfig()
    {
        int iRet = 0;

        char sTabDir[MAX_PATH_NAME_LEN] = {0};
        snprintf(sTabDir, sizeof(sTabDir), "%s.BAK/.TABLE", m_szHome);
        
        if(!TMdbNtcDirOper::IsExist(sTabDir))
        {
            TADD_DETAIL("no old config file exist.");
            return iRet;
        }


        TMdbNtcFileScanner tDirScan;
        if(!tDirScan.ScanDir(sTabDir))
        {   
            TADD_ERROR(-1,"Get Table info failed.");
            return ERR_OS_OPEN_DIR;
        }

        const char* psTabDir = NULL;
        char sTabName[MAX_NAME_LEN] = {0};
        while((psTabDir = tDirScan.GetNext()) != NULL)
        {
            memset(sTabName, 0, sizeof(sTabName));   
            if(GetTableNameFromDir(psTabDir,sTabName,sizeof(sTabName)) < 0)
            {
                continue;
            }
        
            iRet = LoadTableOldStruct(psTabDir,sTabName);
            if(0 != iRet)
            {
                return iRet;
            }
        }
        
        return iRet;
    }

    int TMdbConfig::LoadTableOldStruct(const char* psCfgDir,const char* psTabName)
    {
        char sCfgFile[MAX_FILE_NAME] = {0};
        snprintf(sCfgFile, sizeof(sCfgFile), "%s/.Tab_%s_%s.xml",psCfgDir, m_sDSN, psTabName);
        
        MDBXMLDocument tDoc(sCfgFile);
        if (false == tDoc.LoadFile())
        {
            TADD_ERROR(-1,"Load table configuration backup failed.");
            return -1;
        }
        MDBXMLElement* pRoot = tDoc.FirstChildElement("MDBConfig");
        if(pRoot == NULL)
        {
            TADD_ERROR(-1,"Not find element=[MDBConfig] when loading table configuration backup.",sCfgFile);
            return -1;
        }
        
        //清除以前信息
        int iRet = 0;
        char *pValue = NULL;
        TMdbTable* pOldTable = NULL;
        for(MDBXMLElement* pEle=pRoot->FirstChildElement("table"); pEle; pEle=pEle->NextSiblingElement("table"))
        {
            
            pOldTable = new(std::nothrow) TMdbTable();
            if(pOldTable == NULL)
            {
                TADD_ERROR(ERR_OS_NO_MEMROY," Mem Not Enough");
                return ERR_OS_NO_MEMROY;
            }
            pOldTable->Clear();
            
            //获取name节点值
            pValue = GetTablePropertyValue(pEle,"name");
            CHECK_OBJ(pValue);
            TMdbNtcStrFunc::Trim(pValue);
            //判断表名是否有效
            if(!IsValidName(pValue))
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM,"The table name[%s] is invalid,please check the configuration table.",pValue);
                return ERR_APP_INVALID_PARAM;  
            }
            
            //TMdbNtcStrFunc::ToUpper(pValue,m_pTable[iTableID]->sTableName);
            if(0 != TMdbNtcStrFunc::StrNoCaseCmp(pValue, psTabName))
            {
                TADD_ERROR(ERR_APP_INVALID_PARAM,"inner table info error, table name not match,expect=[%s], actually=[%s]",psTabName,pValue);
                return ERR_APP_INVALID_PARAM;
            }
            
            SAFESTRCPY(pOldTable->sTableName, sizeof(pOldTable->sTableName), TMdbNtcStrFunc::ToUpper(pValue));
            
            // 所属表空间
            pValue = GetTablePropertyValue(pEle,"table-space");
            if(pValue != NULL)
            {
                TMdbNtcStrFunc::Trim(pValue);
                SAFESTRCPY(pOldTable->m_sTableSpace, sizeof(pOldTable->m_sTableSpace), TMdbNtcStrFunc::ToUpper(pValue));
            }

            //获取record-counts节点值
            pValue = GetTablePropertyValue(pEle,"record-counts");
            if(pValue != NULL)
            {
                pOldTable->iRecordCounts = atoi(pValue);
            }

            //获取expand-record节点值
            pValue = GetTablePropertyValue(pEle,"expand-record");
            if(pValue != NULL)
            {
                pOldTable->iExpandRecords = atoi(pValue);
            }
            pOldTable->iRecordCounts += pOldTable->iExpandRecords;
            if(pOldTable->iRecordCounts < 10000)
            {
                pOldTable->iRecordCounts = 10000;
            }

            //获取Is-Zip-Time节点值
            pValue = GetTablePropertyValue(pEle,"Is-Zip-Time");
            if(pValue != NULL)
            {
                pOldTable->m_cZipTimeType = pValue[0];
                if('Y' != pValue[0] && 'N' != pValue[0] && 'L' != pValue[0] )
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"Invalid element=[Is-Zip-Time=%s].",pValue);
                    return ERR_APP_INVALID_PARAM;
                }
            }

            //获取rep-type节点值
            pValue = GetTablePropertyValue(pEle,"rep-type");
            if(pValue != NULL)
            {
                pOldTable->iRepAttr = GetRepType(pValue);
            }

            //获取is-read-lock节点值
            pValue = GetTablePropertyValue(pEle,"is-read-lock");
            SET_TABLE_PROPERTY_BOOL_VALUE(pOldTable->bReadLock,"is-read-lock",pValue);

            //获取is-write-lock节点值
            pValue = GetTablePropertyValue(pEle,"is-write-lock");
            SET_TABLE_PROPERTY_BOOL_VALUE(pOldTable->bWriteLock,"is-write-lock",pValue);

            //获取is-rollback节点值
            pValue = GetTablePropertyValue(pEle,"is-rollback");
            SET_TABLE_PROPERTY_BOOL_VALUE(pOldTable->bRollBack,"is-rollback",pValue);

            //获取is-PerfStat节点值
            pValue = GetTablePropertyValue(pEle,"is-PerfStat");
            SET_TABLE_PROPERTY_BOOL_VALUE(pOldTable->bIsPerfStat,"is-PerfStat",pValue);

            //获取checkPriKey节点值
            pValue = GetTablePropertyValue(pEle,"checkPriKey");
            SET_TABLE_PROPERTY_BOOL_VALUE(pOldTable->bIsCheckPriKey,"checkPriKey",pValue);
            
            //获取LoadType节点值
            pValue = GetTablePropertyValue(pEle,"LoadType");
            if(NULL != pValue)
            {
                pOldTable->iLoadType = atoi(pValue);
            }

            //上载SQL  信息
            CHECK_RET(LoadSQL(pOldTable,pEle),"LoadSQL(%s) faield.",pOldTable->sTableName);
            CHECK_RET(LoadParameter(pOldTable,pEle),"LoadParameter(%s) faield.",pOldTable->sTableName);

            //上载对应的列信息,填充列的偏移量信息
            CHECK_RET(LoadTableColumn(pOldTable, pEle)," LoadColumn(%s) failed.",pOldTable->sTableName);
            CHECK_RET(SetColOffset(pOldTable),"SetColOffset(%s) failed.",pOldTable->sTableName);
            //上载对应的索引
            CHECK_RET(LoadTableIndex(pOldTable, pEle),"LoadTableIndex failed.");
            //上载对应主键信息
            CHECK_RET(LoadTablePrimaryKey(pOldTable, pEle),"LoadTablePrimaryKey failed.");
            CHECK_RET(CalcOneRecordSize(pOldTable),"CalcOneRecordSize(%s) failed.",pOldTable->sTableName);
            //pOldTable->iCounts = pOldTable->iRecordCounts;

           m_vpOldTabStruct.push_back(pOldTable);
            
        }
        
        return iRet;
    }

    int TMdbConfig::LoadTableAlterInfo(const char* psCfgDir,const char* psTabName)
    {
        int iRet = 0;
        //TADD_DETAIL("table config dir[%s], table name=[%s]",psCfgDir ,psTabName);
        char sTabUpdateFile[MAX_FILE_NAME] = {0};
        snprintf(sTabUpdateFile,MAX_FILE_NAME, "%s/.Tab_%s_update.xml", psCfgDir, psTabName);
        char sTabUpdateInfoFile[MAX_FILE_NAME] = {0};
        snprintf(sTabUpdateInfoFile,MAX_FILE_NAME, "%s/.Tab_%s_update_info.xml", psCfgDir, psTabName);
		char * fileName = NULL;
        if(false == TMdbNtcFileOper::IsExist(sTabUpdateFile))
        {
        	if(false == TMdbNtcFileOper::IsExist(sTabUpdateInfoFile))
        	{
	            TADD_DETAIL("table[%s] have no struct modify info.",psTabName);
	            return iRet;
        	}
			fileName = sTabUpdateInfoFile;
        }
		else
		{
			fileName = sTabUpdateFile;
		}
        TMdbTabAlterInfo tAlterInfo;
        tAlterInfo.Clear();

        tAlterInfo.m_sTableName = psTabName;

        MDBXMLDocument tDoc(fileName);
        if (false == tDoc.LoadFile())
        {
            TADD_ERROR(-1,"Load table configuration update failed.");
            return -1;
        }
        MDBXMLElement* pRoot = tDoc.FirstChildElement("MDBConfig");
        if(pRoot == NULL)
        {
            TADD_ERROR(-1,"Not find element=[MDBConfig] when loading table configuration update.");
            return -1;
        }
        

        for(MDBXMLElement* pEle=pRoot->FirstChildElement("table"); pEle; pEle=pEle->NextSiblingElement("table"))
        {
        	TColumnInfo tColumnInfo;
            // 增加列
            for(MDBXMLElement* pEleId=pEle->FirstChildElement("add-column"); pEleId; pEleId=pEleId->NextSiblingElement("add-column"))
	        {
	        	tColumnInfo.Clear();
	            MDBXMLAttribute* pAttr = NULL;
	            for(pAttr=pEleId->FirstAttribute(); pAttr; pAttr=pAttr->Next())
	            {
	                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0)
	                {
						tColumnInfo.m_sName = const_cast<char*>(pAttr->Value());
	                }
					if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "position") == 0)
	                {
						tColumnInfo.m_iPos = atoi(const_cast<char*>(pAttr->Value()));
	                }
	            }
				tAlterInfo.m_vAddColumn.push_back(tColumnInfo);
	        }
			int iCount = 0;
			// 删除列
			for(MDBXMLElement* pEleId=pEle->FirstChildElement("drop-column"); pEleId; pEleId=pEleId->NextSiblingElement("drop-column"))
	        {
	        	tColumnInfo.Clear();
	            MDBXMLAttribute* pAttr = NULL;
	            for(pAttr=pEleId->FirstAttribute(); pAttr; pAttr=pAttr->Next())
	            {
	                if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "value") == 0)
	                {
						tColumnInfo.m_sName = const_cast<char*>(pAttr->Value());
	                }
					if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "position") == 0)
	                {
						tColumnInfo.m_iPos = atoi(const_cast<char*>(pAttr->Value())) + iCount;//消除列号调整的影响
	                }
	            }
				tAlterInfo.m_vDropColumn.push_back(tColumnInfo);
				iCount++;
	        }
			
            // 变更data-type 的列
            for(MDBXMLElement* pCol=pEle->FirstChildElement("mod-datatype"); pCol; pCol=pCol->NextSiblingElement("mod-datatype"))
            {
                MDBXMLAttribute* pAttr = NULL; 
                
                TColumnAlter tDataTypeAlter;
                tDataTypeAlter.Clear();
                tDataTypeAlter.m_bTypeAlter =true;

                bool bName =false;
                bool bOld = false;
                bool bNew = false;
        
                for(pAttr=pCol->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                    {
                        tDataTypeAlter.m_sColmName = pAttr->Value();
                        bName = true;
                    }
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "old-value") == 0)
                    {
                        tDataTypeAlter.m_tAlterInfo.m_iOldValue = GetDataType(pAttr->Value());
                        bOld = true;
                    }
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "new-value") == 0)
                    {
                        tDataTypeAlter.m_tAlterInfo.m_iNewValue = GetDataType(pAttr->Value());
                        bNew = true;
                    }
                    else
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"The ATTR name[%s] is invalid, please check the  configuration.",pAttr->Value());
                        return ERR_APP_INVALID_PARAM;
                    }
                }

                if(!bName || !bOld || !bNew)
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"config is not integrated.");
                    return ERR_APP_INVALID_PARAM;
                }
                tAlterInfo.m_vColumnAlter.push_back(tDataTypeAlter);
            }
            

            // 变更data-len 的列
            for(MDBXMLElement* pCol=pEle->FirstChildElement("mod-datalen"); pCol; pCol=pCol->NextSiblingElement("mod-datalen"))
            {
                MDBXMLAttribute* pAttr = NULL; 
                
                TColumnAlter tDataTypeAlter;
                tDataTypeAlter.Clear();
                tDataTypeAlter.m_bLenAlter=true;

                bool bName =false;
                bool bOld = false;
                bool bNew = false;
        
                for(pAttr=pCol->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "name") == 0)
                    {
                        tDataTypeAlter.m_sColmName = pAttr->Value();
                        bName = true;
                    }
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "old-value") == 0)
                    {
                        tDataTypeAlter.m_tAlterInfo.m_iOldValue = atoi(pAttr->Value());
                        bOld = true;
                    }
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "new-value") == 0)
                    {
                        tDataTypeAlter.m_tAlterInfo.m_iNewValue = atoi(pAttr->Value());
                        bNew = true;
                    }
                    else
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"The ATTR name[%s] is invalid, please check the  configuration.",pAttr->Value());
                        return ERR_APP_INVALID_PARAM;
                    }
                }

                if(!bName || !bOld || !bNew)
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"config is not integrated.");
                    return ERR_APP_INVALID_PARAM;
                }
                tAlterInfo.m_vColumnAlter.push_back(tDataTypeAlter);
            }

            // 变更talespace 的列
            for(MDBXMLElement* pCol=pEle->FirstChildElement("mod-talespace"); pCol; pCol=pCol->NextSiblingElement("mod-talespace"))
            {
                MDBXMLAttribute* pAttr = NULL; 
                tAlterInfo.m_bTsAlter = true;
                
                bool bOld = false;
                bool bNew = false;
        
                for(pAttr=pCol->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "old-value") == 0)
                    {
                        tAlterInfo.m_tTsAlter.m_sOldValue = pAttr->Value();
                        bOld = true;
                    }
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "new-value") == 0)
                    {
                        tAlterInfo.m_tTsAlter.m_sNewValue = pAttr->Value();
                        bNew = true;
                    }
                    else
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"The ATTR name[%s] is invalid, please check the  configuration.",pAttr->Value());
                        return ERR_APP_INVALID_PARAM;
                    }
                }

                if( !bOld || !bNew)
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"config is not integrated.");
                    return ERR_APP_INVALID_PARAM;
                }
            }

            // 变更is-zip-time 的列
            for(MDBXMLElement* pCol=pEle->FirstChildElement("mod-timezip"); pCol; pCol=pCol->NextSiblingElement("mod-timezip"))
            {
                MDBXMLAttribute* pAttr = NULL; 
                tAlterInfo.m_bZipTime = true;
                
                bool bOld = false;
                bool bNew = false;
        
                for(pAttr=pCol->FirstAttribute(); pAttr; pAttr=pAttr->Next())
                {
                    if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "old-value") == 0)
                    {
                        tAlterInfo.m_ZipTimeAlter.m_sOldValue = pAttr->Value();
                        bOld = true;
                    }
                    else if(TMdbNtcStrFunc::StrNoCaseCmp(pAttr->Name(), "new-value") == 0)
                    {
                        tAlterInfo.m_ZipTimeAlter.m_sNewValue = pAttr->Value();
                        bNew = true;
                    }
                    else
                    {
                        TADD_ERROR(ERR_APP_INVALID_PARAM,"The ATTR name[%s] is invalid, please check the  configuration.",pAttr->Value());
                        return ERR_APP_INVALID_PARAM;
                    }
                }

                 if(!bOld || !bNew)
                {
                    TADD_ERROR(ERR_APP_INVALID_PARAM,"config is not integrated.");
                    return ERR_APP_INVALID_PARAM;
                }
            }
        }

        m_vTableAlterInfo.push_back(tAlterInfo);
        
        return iRet;
    }

    int TMdbConfig::GetTableNameFromDir(const char* psDir, char* psTableName, int iNameLen)
    {
        //TADD_DETAIL("table dir=[%s]", psDir);
        TMdbNtcSplit tSplit;
        tSplit.SplitString(psDir,'/');
        int iCnt = tSplit.GetFieldCount();
        if(iCnt<=4) // 起码第4层
        {
            TADD_ERROR(-1, "error table configuration path.");
            return ERROR_UNKNOWN;
        }

        SAFESTRCPY(psTableName, iNameLen, tSplit[iCnt-1]);
        TMdbNtcStrFunc::Trim(psTableName, '.');
        TADD_DETAIL("Get Table Name=[%s]", psTableName);
        
        return 0;
    }

    int TMdbConfig::GetIdleTableId()
    {
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        { 
            if(m_pTable[i] == NULL)
            {
                return i;
            }
        }
        return -1;
    }

    int TMdbConfig::LoadTable()
    {
        int iRet = 0;

        char sTabDir[MAX_PATH_NAME_LEN] = {0};
        snprintf(sTabDir, sizeof(sTabDir), "%s.TABLE", m_szHome);
        
        if(!TMdbNtcDirOper::IsExist(sTabDir))
        {
            TMdbNtcDirOper::MakeFullDir(sTabDir);
            return iRet;
        }

        TMdbNtcFileScanner tDirScan;
        if(!tDirScan.ScanDir(sTabDir))
        {   
            TADD_ERROR(-1,"Get Table info failed.");
            return ERR_OS_OPEN_DIR;
        }

        const char* psTabDir = NULL;
        char sTabName[MAX_NAME_LEN] = {0};
        while((psTabDir = tDirScan.GetNext()) != NULL)
        {
            memset(sTabName, 0, sizeof(sTabName));  
            if(GetTableNameFromDir(psTabDir,sTabName,sizeof(sTabName)) < 0)
            {
                iRet = ERROR_UNKNOWN;
                break;
            }
        
            iRet = LoadTableStruct(psTabDir,sTabName);
            if(0 != iRet)
            {
                return iRet;
            }

            CHECK_RET(LoadTableAlterInfo(psTabDir, sTabName),"Get inner table alter info failed.");

        }
        
        return iRet;
    }

    int TMdbConfig::GetTableLevelCnt(int iTabLevel)
    {
        TADD_FUNC("Start. tableLevel=[%d]",iTabLevel);
        int iRet = 0;
        switch(iTabLevel)
        {
            case TAB_TINY:
                iRet = 16384;
                break;
            case TAB_MINI: // pow(2, 19)
                iRet = 524288;
                break;
            case TAB_SMALL:
                iRet = 2097152; // pow(2, 21)
                break;
            case TAB_LARGE: // pow(2, 24)
                iRet = 16777216;
                break;
             case TAB_HUGE: // pow(2, 26)
                iRet = 67108864;
                break;
            case TAB_ENORMOUS:// pow(2, 27)
                iRet = 134217728;
                break;
              default:
                iRet = 16384;
                break;
        }
        
        return iRet;
    }

    int TMdbConfig::GetTabLevelByRecordCounts(int iRecdCnts)
    {
        int iLevel = 0;
        if(iRecdCnts <= 50000)
        {
            iLevel = TAB_TINY;
        }
        else if(iRecdCnts>50000 &&  iRecdCnts <= 1000000)
        {
            iLevel = TAB_MINI;
        }
        else if(iRecdCnts>1000000 &&  iRecdCnts <= 10000000)
        {
            iLevel = TAB_SMALL;
        }
        else if(iRecdCnts>10000000 &&  iRecdCnts <= 50000000)
        {
            iLevel = TAB_LARGE;
        }
        else if(iRecdCnts>50000000 &&  iRecdCnts <= 100000000)
        {
            iLevel = TAB_HUGE;
        }
        else if(iRecdCnts>100000000)
        {
            iLevel = TAB_ENORMOUS;
        }
        return iLevel;
    }

    int TMdbConfig::GetRecordCountsByTabLevel(int iTabLevel)
    {
        return GetTableLevelCnt(iTabLevel) ;
    }

    
    bool TMdbConfig::IsDsnSame(const char* sName)
    {
        if(NULL == sName) return false;
        else
            return TMdbNtcStrFunc::StrNoCaseCmp(sName,m_sDSN) == 0;
    }


    bool TMdbConfig::HaveNewTableSpace()
    {
        return m_tTsAlterInfo.m_vAddTs.size() > 0;
    }

    bool TMdbConfig::IsNewTableSpace(const char* psTsName)
    {
        if(NULL == psTsName) return false;
        
        std::vector<std::string>::iterator itor = m_tTsAlterInfo.m_vAddTs.begin();
        for(; itor != m_tTsAlterInfo.m_vAddTs.end(); ++itor)
        {
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psTsName, itor->c_str()) )
            {
                return true;
            }
        }

        return false;
    }

    bool TMdbConfig::HaveDelTableSpace()
    {
        return m_tTsAlterInfo.m_vDelTs.size() >0;
    }

    bool TMdbConfig::IsDelTableSpace(const char* psTsName)
    {
        if(NULL == psTsName) return false;
        
        std::vector<std::string>::iterator itor = m_tTsAlterInfo.m_vDelTs.begin();
        for(; itor != m_tTsAlterInfo.m_vDelTs.end(); ++itor)
        {
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(psTsName, itor->c_str()) )
            {
                return true;
            }
        }

        return false;
    }

    bool TMdbConfig::HavePageSizeAlterTS()
    {
        return m_tTsAlterInfo.m_vPageSizeAlter.size() > 0;
    }

    bool TMdbConfig::IsPageSizeAlter(const char* psTableSpaceName, TIntAlterAttr* pAlterAttr)
    {
        if(NULL == psTableSpaceName) return false;

        std::map<std::string, TIntAlterAttr>::iterator itor = m_tTsAlterInfo.m_vPageSizeAlter.begin();
        for(; itor != m_tTsAlterInfo.m_vPageSizeAlter.end(); ++itor)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(psTableSpaceName, itor->first.c_str()) == 0)
            {
                pAlterAttr = &(itor->second);
                return true;
            }
        }
        return false;
    }

    bool TMdbConfig::IsTableSpaceChange(const char* psTableName, TStringAlterAttr* pTsAlterInfo)
    {
        if(NULL == psTableName) return false;
        
        std::vector<TMdbTabAlterInfo>::iterator itor = m_vTableAlterInfo.begin();
        for(; itor != m_vTableAlterInfo.end(); ++itor)
        {
            if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sTableName.c_str(), psTableName))
            {
                if(itor->m_bTsAlter)
                {
                    pTsAlterInfo = &(itor->m_tTsAlter);
                    return true;
                }
                return false;
            }
        }
        
        return false;
    }

    bool TMdbConfig::HaveNewColumn(const char* psTableName)
    {
        if(NULL == psTableName) return false;
        std::vector<TMdbTabAlterInfo>::iterator itor = m_vTableAlterInfo.begin();
        for(; itor != m_vTableAlterInfo.end(); ++itor)
        {
            if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sTableName.c_str(), psTableName))
            {
                if(itor->m_vAddColumn.size() > 0)
                {
                    return true;
                }
                return false;
            }
        }

        return false;
    }

	bool TMdbConfig::HaveDropColumn(const char* psTableName)
    {
        if(NULL == psTableName) return false;
        std::vector<TMdbTabAlterInfo>::iterator itor = m_vTableAlterInfo.begin();
        for(; itor != m_vTableAlterInfo.end(); ++itor)
        {
            if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sTableName.c_str(), psTableName))
            {
                if(itor->m_vDropColumn.size() > 0)
                {
                    return true;
                }
                return false;
            }
        }

        return false;
    }

	int TMdbConfig::GetDropColumnIndex(const char* sTableName, int * iDropIndex)
	{
		int iRet = 0;
		std::vector<TMdbTabAlterInfo>::iterator itor = m_vTableAlterInfo.begin();
		for(; itor != m_vTableAlterInfo.end(); ++itor)
		{
			if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sTableName.c_str(), sTableName))
			{
				std::vector<TColumnInfo>::iterator itorC = itor->m_vDropColumn.begin();
				for(; itorC != itor->m_vDropColumn.end(); itorC++)
				{
					iDropIndex[itorC->m_iPos] = 1;
				}
			}
		}
		return iRet;
	}

	bool TMdbConfig::IsDropColumn(const char* psTableName, const char* psColmName)
	{
		if(NULL == psTableName) return false;
		
		std::vector<TMdbTabAlterInfo>::iterator itor = m_vTableAlterInfo.begin();
		for(; itor != m_vTableAlterInfo.end(); ++itor)
		{
			if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sTableName.c_str(), psTableName))
			{
				std::vector<TColumnInfo>::iterator itorC = itor->m_vDropColumn.begin();
				for(; itorC != itor->m_vDropColumn.end(); itorC++)
				{
					if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itorC->m_sName.c_str(), psColmName))
					{
						return true;
					}
				}
				return false;
			}
		}

		return false;
	}

    //true 表示不从db加载
    bool TMdbConfig::IsNotLoadFromDB(const char * pTableName)
    {
        if(!pTableName) return false;
        if(m_tDsn.m_vNotLoadFromDBTab.empty()) return false;
        std::vector<std::string>::iterator itor = m_tDsn.m_vNotLoadFromDBTab.begin();
        for(; itor != m_tDsn.m_vNotLoadFromDBTab.end(); ++itor)
        {
            if(0 == TMdbNtcStrFunc::StrNoCaseCmp(itor->c_str(), pTableName))
            {
                return true;
            }
        }
        return false;
    }

    bool TMdbConfig::IsNewColumn(const char* psTableName, const char* psColmName)
    {
        if(NULL == psTableName) return false;
        
        std::vector<TMdbTabAlterInfo>::iterator itor = m_vTableAlterInfo.begin();
        for(; itor != m_vTableAlterInfo.end(); ++itor)
        {
            if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sTableName.c_str(), psTableName))
            {
                std::vector<TColumnInfo>::iterator itorC = itor->m_vAddColumn.begin();
                for(; itorC != itor->m_vAddColumn.end(); itorC++)
                {
                    if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itorC->m_sName.c_str(), psColmName))
                    {
                        return true;
                    }
                }
                return false;
            }
        }

        return false;
    }

    bool TMdbConfig::IsColumnDataTypeAlter(const char* psTableName, const char* psColumnName, TIntAlterAttr* pAlterInfo)
    {
        if(NULL == psTableName) return false;
        if(NULL == psColumnName) return false;

        std::vector<TMdbTabAlterInfo>::iterator itor = m_vTableAlterInfo.begin();
        for(; itor != m_vTableAlterInfo.end(); ++itor)
        {
            if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sTableName.c_str(), psTableName))
            {
                std::vector<TColumnAlter>::iterator itorC = itor->m_vColumnAlter.begin();
                for(; itorC != itor->m_vColumnAlter.end(); itorC++)
                {
                    if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itorC->m_sColmName.c_str(), psColumnName))
                    {
                        if(itorC->m_bTypeAlter)
                        {
                            pAlterInfo = &(itorC->m_tAlterInfo);
                            return true;
                        }
                        return false;                        
                    }
                }
                return false;
            }
        }

        return false;
    }

    bool TMdbConfig::IsColumnDataLenAlter(const char* psTableName, const char* psColumnName, TIntAlterAttr* pAlterInfo)
    {
        if(NULL == psTableName) return false;
        if(NULL == psColumnName) return false;

        std::vector<TMdbTabAlterInfo>::iterator itor = m_vTableAlterInfo.begin();
        for(; itor != m_vTableAlterInfo.end(); ++itor)
        {
            if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sTableName.c_str(), psTableName))
            {
                std::vector<TColumnAlter>::iterator itorC = itor->m_vColumnAlter.begin();
                for(; itorC != itor->m_vColumnAlter.end(); itorC++)
                {
                    if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itorC->m_sColmName.c_str(), psColumnName))
                    {
                        if(itorC->m_bLenAlter)
                        {
                            pAlterInfo = &(itorC->m_tAlterInfo);
                            return true;
                        }
                        return false;                        
                    }
                }
                return false;
            }
        }

        return false;
    }

    bool TMdbConfig::IsTableAlter(const char* psTableName)
    {
        if(NULL == psTableName) return false;
        std::vector<TMdbTabAlterInfo>::iterator itor = m_vTableAlterInfo.begin();
        for(; itor != m_vTableAlterInfo.end(); ++itor)
        {
            if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sTableName.c_str(), psTableName))
            {
                if(itor->HaveAnyChange())
                {
                    return true;
                }
                return false;
            }
        }

        return false;
    }

    bool TMdbConfig::IsColumnAlter(const char* psTableName, const char* psColmName)
    {
        bool bRet = false;
        if(NULL == psTableName) return false;
        if(NULL == psColmName) return false;
        std::vector<TMdbTabAlterInfo>::iterator itor = m_vTableAlterInfo.begin();
        for(; itor != m_vTableAlterInfo.end(); ++itor)
        {
            if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itor->m_sTableName.c_str(), psTableName))
            {
                // 新增列变化中是否有该列
                std::vector<TColumnInfo>::iterator itorAdd = itor->m_vAddColumn.begin();
                for(; itorAdd!= itor->m_vAddColumn.end(); itorAdd++)
                {
                    if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itorAdd->m_sName.c_str(), psColmName))
                    {
                        bRet = true;
                        break;
                    }
                }
                if(bRet)
                {
                    break;
                }
                
                // 变更列变化中是否有该列
                std::vector<TColumnAlter>::iterator itorMod = itor->m_vColumnAlter.begin();
                for(; itorMod!= itor->m_vColumnAlter.end(); itorMod++)
                {
                    if(0 ==TMdbNtcStrFunc::StrNoCaseCmp(itorMod->m_sColmName.c_str(), psColmName))
                    {
                        bRet = true;
                        break;
                    }
                }
                if(bRet)
                {
                    break;
                }
                
            }
        }
        
        return bRet;
    }


    
    //初始化
    TMdbConfig * TMdbConfigMgr::m_pArrMdbConfig[MAX_DSN_COUNTS] = {0};
    TMdbConfig * TMdbConfigMgr::m_pArrDsnConfig[MAX_DSN_COUNTS] = {0};
    TMdbConfigMgr::CGarbo TMdbConfigMgr::Garbo;

    TMutex    TMdbConfigMgr::m_tMutex(true);

    /******************************************************************************
    * 函数名称	:  TMdbConfigMgr
    * 函数描述	:  构造
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbConfigMgr::TMdbConfigMgr()
    {

    }

    /******************************************************************************
    * 函数名称	:  ~TMdbConfigMgr
    * 函数描述	:  析构
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbConfigMgr::~TMdbConfigMgr()
    {

    }

    /******************************************************************************
    * 函数名称	:  GetMdbConfig
    * 函数描述	:  获取配置文件
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbConfig * TMdbConfigMgr::GetMdbConfig(const char * sDsn)
    {
        if(NULL == sDsn || 0 == sDsn[0])
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"dsn is null.");
            return NULL;
        }
        int i = 0;
        int iRet = 0;
        TMdbConfig * pRetConfig = NULL;
        m_tMutex.Lock(true);//加锁
        for(i = 0;i < MAX_DSN_COUNTS;++i)
        {
            if(NULL != m_pArrMdbConfig[i])
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(m_pArrMdbConfig[i]->GetDSN()->sName,sDsn) == 0)
                {//找到一个
                    pRetConfig =  m_pArrMdbConfig[i];
                    break;
                }
                else
                {//不匹配继续
                    continue;
                }
            }
            else
            {
                TMdbConfig * pNewMdbConfig = NULL;
                do
                {
                    pNewMdbConfig = new TMdbConfig();
                    if( NULL == pNewMdbConfig)
                    {
                        CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"TMdbConfig is NULL.");
                    }
                    //加载配置
                    CHECK_RET_BREAK(pNewMdbConfig->Init(),"config init failed[%s]",sDsn);
                    CHECK_RET_BREAK(pNewMdbConfig->LoadCfg(sDsn),"LoadCfg(%s)",sDsn) ;
                }while(0);
                if(0 != iRet)
                {//有错
                    SAFE_DELETE(pNewMdbConfig);
                }
                else
                {
                    m_pArrMdbConfig[i] = pNewMdbConfig;
                    pRetConfig =  m_pArrMdbConfig[i]; 
                }
                break;
            }
        }
        m_tMutex.UnLock(true);//解锁
        return pRetConfig;
    }
    //仅仅获取DSN配置文件，不获取表配置信息
    TMdbConfig * TMdbConfigMgr::GetDsnConfig(const char * sDsn)
    {
        if(NULL == sDsn || 0 == sDsn[0])
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"dsn is null.");
            return NULL;
        }
        int i = 0;
        int iRet = 0;
        TMdbConfig * pRetConfig = NULL;
        m_tMutex.Lock(true);//加锁
        for(i = 0;i < MAX_DSN_COUNTS;++i)
        {
            if(NULL != m_pArrDsnConfig[i])
            {
                if(TMdbNtcStrFunc::StrNoCaseCmp(m_pArrDsnConfig[i]->GetDSN()->sName,sDsn) == 0)
                {//找到一个
                    pRetConfig =  m_pArrDsnConfig[i];
                    break;
                }
            }
            else
            {
                TMdbConfig * pNewMdbConfig = NULL;
                do
                {
                    pNewMdbConfig = new TMdbConfig();
                    if( NULL == pNewMdbConfig)
                    {
                        CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"TMdbConfig is NULL.");
                    }
                    //加载配置
                    CHECK_RET_BREAK(pNewMdbConfig->Init(),"config init failed[%s]",sDsn);
                    CHECK_RET_BREAK(pNewMdbConfig->LoadDsnConfigFile(sDsn,false),"LoadDsnConfigFile(%s)",sDsn) ;
                }while(0);
                if(0 != iRet)
                {//有错
                    SAFE_DELETE(pNewMdbConfig);
                }
                else
                {
                    m_pArrDsnConfig[i] = pNewMdbConfig;
                    pRetConfig =  m_pArrDsnConfig[i]; 
                }
                break;
            }
        }
        m_tMutex.UnLock(true);//解锁
        return pRetConfig;
    }


    //NULL值管理
    /******************************************************************************
    * 函数名称	:  CalcNullSize
    * 函数描述	:  计算NULL值所占的空间
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbNULLManager::CalcNullSize(TMdbTable * pTable)
    {
        if(NULL == pTable){return 0;}
        int i = 0;
        for(i = 0;i < pTable->iColumnCounts;++i)
        {
            if(true == pTable->tColumn[i].m_bNullable)
            {
                break;
            }
        }
        if(i == pTable->iColumnCounts){return 0;}//所有列都不支持NULL;
        
        int iCount = pTable->iColumnCounts/MDB_CHAR_SIZE;
        if(0 != pTable->iColumnCounts % MDB_CHAR_SIZE )
        {
            iCount ++;
        }
        return iCount * MDB_CHAR_SIZE;
    }
    /******************************************************************************
    * 函数名称	:  IsColumnNULL
    * 函数描述	:  判断该列是否为NULL
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool TMdbNULLManager::IsColumnNULL(char * pAddr,int iColumnPos)
    {
        return false;  
    }

//}

