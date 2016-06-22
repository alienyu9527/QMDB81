#include "Tools/mdbCheckState.h"
#include "Helper/mdbOS.h"
#include "dirent.h"



//namespace QuickMDB{


const int TMDBCheckState::m_iMaxFileCount = 10;
TMDBCheckState::TMDBCheckState()
{
    m_pShmDSN = NULL;
    m_fp = NULL;
}


TMDBCheckState::~TMDBCheckState()
{
}

int TMDBCheckState::Init(char *pszDSN)
{
    int iRet = 0;
    m_pConfig = new(std::nothrow)TMdbConfig();
    CHECK_OBJ(m_pConfig);

    iRet = m_pConfig->Init();
    if(iRet < 0)
    {
        TADD_ERROR(-1,"[%s : %d] : TMDBCheckState::Init() : Connect to %s failed. Can't find Config-file.", __FILE__, __LINE__, pszDSN); 
        return iRet;
    }
    iRet = m_pConfig->LoadCfg(pszDSN);
    if(iRet < 0)
    {
        TADD_ERROR(-1,"[%s : %d] : TMDBCheckState::Init() : Can't load Config-file, maybe sys-config is error.", __FILE__, __LINE__);  
        return iRet; 
    }
    
    m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN);
    if(m_pShmDSN == NULL)
    {
        TADD_ERROR(-1,"[%s : %d] : TMDBCheckState::Init() : m_pShmDSN == NULL, pszDSN=[%s].", __FILE__, __LINE__, pszDSN);   
        return -1;
    }
    
    iRet = m_pShmDSN->Attach(pszDSN, *m_pConfig);
    if(iRet < 0)
    {
        TADD_ERROR(-1,"[%s : %d] : TMDBCheckState::Init() : Attach [%s] failed. Parameters are invalid.", __FILE__, __LINE__, pszDSN);   
        return iRet;
    }
    return iRet;
    
}

void TMDBCheckState::CheckAllProcess()
{
    TMdbProc *pTMdbProc = NULL;
    TShmList<TMdbProc >::iterator itor = m_pShmDSN->m_ProcList.begin();
    for(;itor != m_pShmDSN->m_ProcList.end();++itor)
    {
        if(m_pShmDSN->GetInfo()->cState == DB_stop)
        {
            cout<<"QMDB is not running. No Process!!"<<endl;
            return;
        }	     

        pTMdbProc = &(*itor);

        if(pTMdbProc->sName[0] == 0)
        {
            continue;
        }    

        bool IsExist = TMdbOS::IsProcExistByPopen(pTMdbProc->iPid);
        if(IsExist == false)
        {
            cout<<pTMdbProc->sName<<"..........................closed"<<endl;
        }	
        else
        {
            cout<<pTMdbProc->sName<<"..........................OK"<<endl;
        }
    }

    return;
}

void TMDBCheckState::CheckOraRep()
{
    TMdbDSN *pDsn = m_pShmDSN->GetInfo();
    if (!pDsn->m_bIsOraRep)
    {
        cout<<"DSN["<<pDsn->sName<<"] is not Oracle Rep ."<<endl;
        return;
    }
    
    //检查ora同步进程是否存在
    std::vector<std::string > vProcName;
    char sProcFullName[MAX_NAME_LEN] = {0};
    snprintf(sProcFullName,MAX_NAME_LEN, "mdbFlushRep %s", pDsn->sName);
    vProcName.push_back(sProcFullName);
    for(int i=-1; i<m_pConfig->GetDSN()->iOraRepCounts; ++i)
    {
        snprintf(sProcFullName, MAX_NAME_LEN, "mdbDbRep %s %d %d", pDsn->sName, m_pConfig->GetDSN()->iOraRepCounts, i);
        vProcName.push_back(sProcFullName);
    }

    TMdbProc *pMdbProc = NULL; 
    for (std::vector<std::string>::size_type i = 0; i < vProcName.size(); i++)
    {
        pMdbProc = m_pShmDSN->GetProcByName(vProcName[i].c_str());
        if (NULL != pMdbProc && TMdbOS::IsProcExistByPopen(pMdbProc->iPid))
        {
            cout<<"Process ["<<vProcName[i].c_str()<<"] OK"<<endl;
        }
        else
        {
            cout<<"Process ["<<vProcName[i].c_str()<<"] not exist"<<endl;
        }
    }

    //检查是否有文件积压
    m_tFileList.Clear();
    m_tFileList.Init(m_pConfig->GetDSN()->sLogDir);
    m_tFileList.GetFileList(CheckOraFile);
    int iCounts = m_tFileList.GetFileCounts();
    if(iCounts >= m_iMaxFileCount)//去掉文件diskSpace
    {
        cout<<"OraRep File Count=["<<iCounts<<"], OraRep too slow"<<endl;
    }
    else
    {
        //cout<<"OraRep Speed Normal"<<endl;
        cout<<"OraRep File Count=["<<iCounts<<"], OraRep Speed Normal"<<endl;
    }
    
    //检查是否有错误日志
    m_tFileList.Clear();
    m_tFileList.Init(m_pConfig->GetDSN()->errorLog);
    m_tFileList.GetFileList(CheckAllFile);
    iCounts = m_tFileList.GetFileCounts();
    if(iCounts > 0)
    {
         cout<<"OraRep Error File Count=["<<iCounts<<"], OraRep Error"<<endl;
    }
    else
    {
        cout<<"OraRep No Error File"<<endl;
    }

    return;
}

void TMDBCheckState::CheckPeerRep()
{
    TMdbDSN    *m_pDsn = m_pShmDSN->GetInfo();
    if (!m_pDsn->m_bIsRep)
    {
        cout<<"DSN["<<m_pDsn->sName<<"] is not Peer Rep."<<endl;
        return;
    }

    //检查主备同步进程是否存在
    std::vector<std::string > vProcName;
    char sProcFullName[MAX_NAME_LEN] = {0};
    snprintf(sProcFullName, MAX_NAME_LEN, "mdbFlushRep %s", m_pDsn->sName);
    vProcName.push_back(sProcFullName);
    snprintf(sProcFullName, MAX_NAME_LEN, "mdbRepServer %s", m_pDsn->sName);
    vProcName.push_back(sProcFullName);
    snprintf(sProcFullName, MAX_NAME_LEN, "mdbRepClient %s", m_pDsn->sName);
    vProcName.push_back(sProcFullName);

    TMdbProc *pMdbProc = NULL; 
    for (std::vector<std::string>::size_type i = 0; i < vProcName.size(); i++)
    {
        pMdbProc = m_pShmDSN->GetProcByName(vProcName[i].c_str());
        if (NULL != pMdbProc && TMdbOS::IsProcExistByPopen(pMdbProc->iPid))
        {
            cout<<"Process ["<<vProcName[i].c_str()<<"] OK"<<endl;
        }
        else
        {
            cout<<"Process ["<<vProcName[i].c_str()<<"] not exist"<<endl;
        }
    }

    //检查同步文件是否积压
    char m_sLogPath[MAX_PATH_NAME_LEN];
    char m_sLocalLogPath[MAX_PATH_NAME_LEN];
    memset(m_sLogPath,0,sizeof(m_sLogPath));
    memset(m_sLocalLogPath,0,sizeof(m_sLocalLogPath));
    SAFESTRCPY(m_sLogPath,sizeof(m_sLogPath),m_pConfig->GetDSN()->sRepDir);
    if(m_sLogPath[strlen(m_sLogPath)-1] != '/')
    {
        m_sLogPath[strlen(m_sLogPath)] = '/';   
    }
    snprintf(m_sLocalLogPath,MAX_PATH_NAME_LEN, "%sLocalFileLog/",m_sLogPath);
    m_tFileList.Clear();
    m_tFileList.Init(m_sLocalLogPath);
    m_tFileList.GetFileList(CheckRepFile);
    int iCounts = m_tFileList.GetFileCounts();
    if(iCounts >= 10)
    {
        cout<<"PeerRep File Count=["<<iCounts<<"],PeerRep too slow"<<endl;
    }
    else
    {
        cout<<"PeerRep Speed Normal"<<endl;
    }
    return;

}

void TMDBCheckState::CheckLog()
{
    bool bErrorFlag = false;
    char sPath[MAX_PATH_NAME_LEN];
    memset(sPath,0,sizeof(sPath));
    snprintf(sPath, MAX_PATH_NAME_LEN, "%s/log/", getenv("QuickMDB_HOME"));
    m_tFileList.Clear();
    m_tFileList.Init(sPath);
    m_tFileList.GetFileList(CheckAllFile);
    int iCounts = m_tFileList.GetFileCounts();
    if(iCounts <= 0)
    {
        cout<<"No Log!"<<endl;
        return;
    }

    char sFileName[256];
    memset(sFileName,0,sizeof(sFileName));

    char sBuf[1024];
    memset(sBuf,0,sizeof(sBuf));
    bool bFirstError = true;
    while(m_tFileList.Next(sFileName) == 0)
    {
        SAFE_CLOSE(m_fp);

        m_fp=fopen(sFileName, "rt");
            
        if(m_fp==NULL)
        {
            cout<<"Open File Error!"<<endl;
            continue;
        }

        char *p =NULL;
        memset(sBuf,0,sizeof(sBuf));
        p=fgets(sBuf,1023,m_fp);
        bFirstError = true;
        while(p != NULL)
        {
            if(strncmp(&p[22], "ERROR", strlen("ERROR")) == 0)
            {
                if (bFirstError)
                {
                    bFirstError = false;
                    cout<<"Error in "<<sFileName<<":\n"<<endl;
                }
                cout<<p<<endl<<endl;
                bErrorFlag = true;
            }

            memset(sBuf,0,sizeof(sBuf));
            p=fgets(sBuf,1023,m_fp);     
        }  
        SAFE_CLOSE(m_fp);
    }

    if(bErrorFlag == false)
    {
        cout<<"No Error Information."<<endl;
    }
}

int TMDBCheckState::CheckIndex()
{
    int iRet = 0;
    TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
    TMdbIndexCtrl tIndexCtrl;
    CHECK_RET(tIndexCtrl.AttachDsn(m_pShmDSN), "Attach DSN[%s] error.\n", m_pShmDSN->GetInfo()->sName);
    
    char sInputNum[64];
    memset(sInputNum, 0, sizeof(sInputNum));
    cout<<"Input max length of conflict index link:";
    scanf("%9s", sInputNum);
    int iMaxLen = atoi(sInputNum);
    if (iMaxLen<=0)
    {
        cout<<endl<<"Wrong input."<<endl;
        return -1;
    }
    
    CHECK_RET(tIndexCtrl.PrintWarnIndexInfo(iMaxLen), "Print warning index information error.\n");
    return iRet;
}

TMdbFileListEx::TMdbFileListEx()
{
    memset(m_sPath, 0, sizeof(m_sPath));
    m_iFileCounts = 0;
    m_iCurPos     = 0;
    
    for(int i=0; i<MAX_FILE_COUNTS; ++i)
    {
        m_FileName[i] = NULL; 
    }
}


TMdbFileListEx::~TMdbFileListEx()
{
    for(int i=0; i<MAX_FILE_COUNTS; ++i)
    {
        delete m_FileName[i]; 
    }
}


//设置读取的目录
int TMdbFileListEx::Init(const char* pszPath)
{
    TADD_FUNC("TMdbFileList::Init(%s) : Start.", pszPath);
    
    if(strlen(pszPath) >= MAX_PATH_NAME_LEN-MAX_NAME_LEN)
    {
        TADD_ERROR(-1,"[%s : %d] : TMdbFileList::Init(%s) failed, Path-Name is too long.", __FILE__, __LINE__, pszPath);
        return -1;   
    }
    
    //初始化文件列表
    for(int i=0; i<MAX_FILE_COUNTS; ++i)
    {
        if(m_FileName[i] == NULL)
        {
            m_FileName[i] = new(std::nothrow) char[MAX_PATH_NAME_LEN];
            if(m_FileName[i] == NULL)
            {
                TADD_ERROR(-1,"[%s : %d] : TMdbFileList::Init(%s) failed, out of memory.", __FILE__, __LINE__, pszPath);
                return ERR_OS_NO_MEMROY;    
            }   
        } 
    }
    
    //记录路径名称
    //strcpy(m_sPath, pszPath);
    memset(m_sPath,0,sizeof(m_sPath));
    SAFESTRCPY(m_sPath,sizeof(m_sPath),pszPath);
    if(m_sPath[strlen(m_sPath)-1] != '/')
    {
        m_sPath[strlen(m_sPath)] = '/';  
    }
    
    m_iFileCounts = 0;
    m_iCurPos     = 0;
    
    TADD_FUNC("TMdbFileList::Init(%s) : Finish.", pszPath);
    
    return 0;
}


//开始读取
int TMdbFileListEx::GetFileList(MdbCheckFileType iFileType)
{
    TADD_FUNC("TMdbFileList::GetFileList() : Start.");
    TADD_DETAIL("TMdbFileList::GetFileList() : m_sPath=[%s].", m_sPath);
    
    //打开目录
    m_iFileCounts = 0;
    m_iCurPos     = 0;
    DIR *dp = opendir(m_sPath);
    if(dp == NULL)
    {
    	bool bCreate = TMdbNtcDirOper::MakeFullDir(m_sPath);
    	if(bCreate == true)
    	{
    		return 0;
    	}
    	else
    	{
        	TADD_ERROR(-1,"[%s : %d] : TMdbFileList::GetFileList() : Mkdir(%s) failed.", __FILE__, __LINE__, m_sPath);
        	return ERR_OS_CREATE_DIR;
    	}     
    }
    
    //开始读取
    const char* sRepFileBegin = "Rep";
    const char* sOraFileBegin = "Ora";
    const char* sInsertOraBegin = "Insert_Ora";
    struct dirent *dirp;
    bool bRet = false;
    while((dirp=readdir(dp)) != NULL)
    {
        bRet = false;
        switch (iFileType)
        {
        case CheckRepFile:
            {
                if(strncmp(dirp->d_name, sRepFileBegin, strlen(sRepFileBegin)) == 0)
                {
                    bRet = true;
                }
            }
        	break;
        case CheckOraFile:
            {
                if(strncmp(dirp->d_name, sOraFileBegin, strlen(sOraFileBegin))== 0 || strncmp(dirp->d_name, sInsertOraBegin, strlen(sInsertOraBegin)) == 0)
                {
                    bRet = true;
                }
            }
            break;
        case CheckAllFile:
            {
                if(strncmp(dirp->d_name, ".", strlen(".")) != 0 && strncmp(dirp->d_name, "..", strlen("..")) != 0)
                {
                    bRet = true;
                }
            }
            break;
        default:
            {
                break;
            }
        }
        if (bRet)
        {
            sprintf(m_FileName[m_iFileCounts], "%s%s",m_sPath, dirp->d_name);//每个文件名中带路径
            unsigned long long iFileSize = 0;
            TMdbNtcFileOper::GetFileSize(m_FileName[m_iFileCounts],iFileSize);
            if (iFileSize> 0)
            {
                ++m_iFileCounts;
                if(m_iFileCounts >= MAX_FILE_COUNTS)
                {
                    break;    
                }
            }            
        }
    }
    
    closedir(dp);
        
    TADD_FUNC("TMdbFileList::GetFileList(m_iFileCounts=%d) : Finish.", m_iFileCounts);
    
    return 0;
}

//下一个文件名(带路径)
int TMdbFileListEx::Next(char* pszFullFileName)
{
    TADD_FUNC("TMdbFileList::Next() : Start.");
    
    if(m_iFileCounts == m_iCurPos)
    {
        TADD_FUNC("TMdbFileList::Next() : Finish(End).");
        return -1;    
    }
	sprintf(pszFullFileName, "%s",m_FileName[m_iCurPos]);
    ++m_iCurPos;
    
    TADD_FUNC("TMdbFileList::Next() : Finish.");
    
    return 0;
}



//清除记录，重新开始
void TMdbFileListEx::Clear()
{
    TADD_FUNC("TMdbFileList::Clear() : Start.");
    
    m_iFileCounts = 0;
    m_iCurPos     = 0;
    
    TADD_FUNC("TMdbFileList::Clear() : Finish."); 
}


TTableRecordCount::TTableRecordCount()
{
    m_pConfig = NULL;
    m_pShmDSN = NULL;
    m_pDBLink = NULL;
    m_pQueryOra = NULL;
    m_pTable = NULL;
    memset(m_sSSQL,0,sizeof(m_sSSQL));
}

TTableRecordCount::~TTableRecordCount()
{
    SAFE_DELETE(m_pQueryOra);
    SAFE_DELETE(m_pDBLink);
}

int TTableRecordCount::Init(const char * pszDSN)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
    if(NULL == m_pConfig)
    {
        TADD_ERROR(-1,"DSN[%s] configuration file not found.",pszDSN);
        return ERR_APP_LAOD_CONFIG_FILE_FALIED;
    }
    CHECK_RET(ConnectOracle(),"Failed to connect the Oracle[%s/******@%s].",
        m_pConfig->GetDSN()->sOracleUID,m_pConfig->GetDSN()->sOracleID);
    TADD_FUNC("Finish.");
    return iRet;
}

int TTableRecordCount::RecordCount(const char* pTableName)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(NULL == pTableName)
    {
        TADD_ERROR(-1,"No table name specified,please check..");
        return ERR_APP_INVALID_PARAM;
    }
    //判断是单表还是全表
    if(TMdbNtcStrFunc::StrNoCaseCmp(pTableName,"all") == 0)
    {
        for(int i = 0;i < MAX_TABLE_COUNTS; ++i)
        {
            TMdbTable* pTable = m_pConfig->GetTableByPos(i);
            if(NULL == pTable){continue;}
            if(IsNeedQueryFromOra(pTable->sTableName))
            {
                if(GenOraSSQL() != 0)
                {
                    TADD_WARNING("Failed to get the table[%s] select-sql.",pTableName);
                    continue;
                }
                if(Query() != 0)
                {
                    TADD_WARNING("Failed to get the table[%s] record count.",pTableName);
                    continue;
                }
            }
        }
    }
    else
    {
        if(IsNeedQueryFromOra(pTableName))
        {
            CHECK_RET(GenOraSSQL(),"Failed to get the table[%s] select-sql.",pTableName);
            CHECK_RET(Query(),"Failed to get the table[%s] record count.",pTableName);
        }
        else
        {
            TADD_WARNING("Table[%s] does not exist in the oracle[%s/******@%s] or Synchronized table property has nothing to do with the Oracle.",
                pTableName,m_pConfig->GetDSN()->sOracleUID,m_pConfig->GetDSN()->sOracleID);
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}

bool TTableRecordCount::IsNeedQueryFromOra(const char* pTableName)
{
    TADD_FUNC("Start.");
    if(NULL == pTableName)
    {
        TADD_ERROR(-1,"No table name specified,please check..");
        return false;
    }
    
    //判断查询的表在QMDB中是否存在
    m_pTable = m_pConfig->GetTableByName(pTableName);
    if(NULL == m_pTable)
    {
        return false;
    }
    
    //过滤系统表
    if(m_pTable->bIsSysTab)
    {
        return false;
    }
    
    //判断表的同步属性是否与oracle有关
    if(m_pTable->iRepAttr != REP_FROM_DB
        && m_pTable->iRepAttr != REP_TO_DB)
    {
        return false;
    }
    TADD_FUNC("Finish.");
    return true;
}

int TTableRecordCount::Query()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(m_pTable);
    CHECK_OBJ(m_pQueryOra);
    int iTotalRecord = 0;
    try
    {
        m_pQueryOra->Close();
        m_pQueryOra->SetSQL(m_sSSQL);
        if(m_pTable->iParameterCount != 0 && m_pTable->m_sLoadSQL[0] != 0)
	    {
	        for(int i = 0; i<m_pTable->iParameterCount; i++)
	        {
	            if(m_pTable->tParameter[i].iDataType == DT_Int && m_pTable->tParameter[i].iParameterType == 0)
	            {
	                m_pQueryOra->SetParameter(m_pTable->tParameter[i].sName,TMdbNtcStrFunc::StrToInt(m_pTable->tParameter[i].sValue));
	            }
	            else if(m_pTable->tParameter[i].iParameterType == 0)
	            {
	                m_pQueryOra->SetParameter(m_pTable->tParameter[i].sName,m_pTable->tParameter[i].sValue);
	            }
	        }
	    }
        m_pQueryOra->Open();
        while(m_pQueryOra->Next())
        {
            iTotalRecord = m_pQueryOra->Field("total").AsInteger();
            TADD_NORMAL("Table = [%s], Total record counts = [%d].",m_pTable->sTableName,iTotalRecord);
            break;
        }
    }
    catch(TMDBDBExcpInterface &e)
    {   
        TADD_ERROR(-1,"Query-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        return ERR_SQL_INVALID;
    }
    catch(TMdbException &e)
    {
        TADD_ERROR(-1,"Insert-SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        return ERR_SQL_INVALID;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TTableRecordCount::GenOraSSQL()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    memset(m_sSSQL,0,sizeof(m_sSSQL));
    if(m_pTable->iRepAttr == REP_TO_DB )
    {
        snprintf(m_sSSQL,sizeof(m_sSSQL),"select count(1) as total from %s ",m_pTable->sTableName);
    }

    if(m_pTable->iRepAttr == REP_FROM_DB)
    {
        if(m_pTable->m_sLoadSQL[0] != 0)
        {
            snprintf(m_sSSQL,sizeof(m_sSSQL),"select count(1) as total from (%s)",m_pTable->m_sLoadSQL);
        }
        else
        {
            snprintf(m_sSSQL,sizeof(m_sSSQL),"select count(1) as total from %s ",m_pTable->sTableName);
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TTableRecordCount::ConnectOracle()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    if(m_pDBLink != NULL)
    {
        if(m_pDBLink->IsConnect()){return iRet;}
        m_pDBLink->Disconnect();
        SAFE_DELETE(m_pDBLink);
    }
    try
    {
        m_pDBLink = TMDBDBFactory::CeatDB();
        CHECK_OBJ(m_pDBLink);
        m_pDBLink->SetLogin(m_pConfig->GetDSN()->sOracleUID, m_pConfig->GetDSN()->sOraclePWD, m_pConfig->GetDSN()->sOracleID);
        if(!m_pDBLink->Connect())
        {
            TADD_ERROR(-1,"Connect Oracle Faild,user=[%s],pwd=[%s],dsn=[%s].",\
            m_pConfig->GetDSN()->sOracleUID,m_pConfig->GetDSN()->sOraclePWD,m_pConfig->GetDSN()->sOracleID);
            return ERR_DB_NOT_CONNECTED;
        }
        m_pQueryOra = m_pDBLink->CreateDBQuery();
        CHECK_OBJ(m_pQueryOra);
    }
    catch(TMDBDBExcpInterface &e)
    {
        TADD_ERROR(-1,"SQL=[%s], error_msg=[%s].",e.GetErrSql(), e.GetErrMsg());
        return ERR_DB_NOT_CONNECTED;
    }
    TADD_FUNC("Finish.");
    return iRet;
}


//}
