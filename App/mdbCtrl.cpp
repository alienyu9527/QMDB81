/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbCtrl.h
*@Description： 内存数据库的控制程序
*@Author:		li.shugang
*@Date：	    2008年11月25日
*@History:
******************************************************************************************/
#include "App/mdbCtrl.h"
#include "Control/mdbSocketTry.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Control/mdbProcCtrl.h"
#include "Control/mdbLinkCtrl.h"
#include "Dbflush/mdbLoadFromDb.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbStruct.h"

#include "Helper/mdbOS.h"
#include "Helper/mdbEncrypt.h"
//#include "OracleFlush/mdbLoadFromPeer.h"

#include "Control/mdbPageCtrl.h"
#include "Control/mdbSysTableThread.h"
#include "Control/mdbStorageEngine.h"
#include "Replication/mdbRepCtrl.h"

#include "Replication/mdbRepLoadData.h"

//namespace QuickMDB{

#ifdef WIN32
#pragma warning(disable: 4305)
#endif

TMdbCtrl::TMdbCtrl()
{
    memset(m_sDsn, 0, sizeof(m_sDsn));
    memset(m_sLockFile, 0, sizeof(m_sLockFile));
    memset(sPeerIP,0,sizeof(sPeerIP));
    iPeerPort = 0;
    m_pConfig = NULL;
    m_bLoadFromDisk = false;
}


TMdbCtrl::~TMdbCtrl()
{
}


//初始化
int TMdbCtrl::Init(const char* pszDsn)
{
    SAFESTRCPY(m_sDsn,sizeof(m_sDsn),pszDsn);
    TMdbNtcStrFunc::ToUpper(m_sDsn);
    int iRet = 0;
    m_pConfig  = TMdbConfigMgr::GetMdbConfig(m_sDsn);
    CHECK_OBJ(m_pConfig);
    return iRet;
}
/******************************************************************************
* 函数名称	:  IsMdbCreate
* 函数描述	:  mdb是否已经被创建了
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
bool TMdbCtrl::IsMdbCreate()
{
    //首先判断共享内存是否存在
    TMdbCfgDSN* pDSN = m_pConfig->GetDSN();//m_tConfig.GetDSN();
    if(pDSN == NULL)
    {
       TADD_ERROR(ERROR_UNKNOWN,"Config invalid.");
       return false;
    }
    long long MgrKey = GET_MGR_KEY(pDSN->llValue);
    SHAMEM_T iShmID = 0;
    return TMdbShm::IsShmExist(MgrKey,pDSN->iManagerSize,iShmID);
}


/******************************************************************************
* 函数名称	:  Create
* 函数描述	:  创建某个内存数据库
* 输入		:  
* 输出		:  
* 返回值	:  
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbCtrl::Create()
{
    TADD_FUNC("(%s) : Start.", m_sDsn);
    int iRet = 0;
    TADD_NORMAL_TO_CLI(FMT_CLI_START,"Create QuickMDB [%s]",m_sDsn);
    //判断是否有残留进程存在
    if(true == m_tProcCtrl.IsMdbSysProcExist(m_sDsn))
    {
        //TADD_WARNING("Some mdb[%s] sys process is exist.please check it ,if it is in other user.",m_sDsn);
        CHECK_RET(ERR_APP_PROC_ALREADY_EXIST,"Some mdb[%s] sys process is exist.!please kill it first.",m_sDsn);
    }
    
    m_pConfig->SetFlag(true);//m_tConfig.SetFlag(true);
    //首先判断共享内存是否存在
    TMdbCfgDSN* pDSN = m_pConfig->GetDSN();
    CHECK_OBJ(pDSN);
    //判断启动主机地址配置是否正确
    if(TMdbNtcStrFunc::StrNoCaseCmp("127.0.0.1",pDSN->sLocalIP) == 0 ||
        !TMdbOS::IsLocalIP(pDSN->sLocalIP))
    {
        TADD_WARNING("local-active-ip[%s] may be not correct.",pDSN->sLocalIP);
    }
    //创建共享内存
    if(IsMdbCreate()  == true)
    {//说明原来的共享内存存在,不能再创建
        CHECK_RET(ERR_OS_SHM_EXIST,"Create(%s) share memory is exist ,if you want to create qmdb ,please run destroy cmd first!",m_sDsn);
    }

	#ifndef DB_NODB
    //启动startcheck
    char cmd[80] = {0};
    memset(cmd,0,80);
    sprintf(cmd,"mdbStartCheck -c %s",m_sDsn);
    system(cmd);
    TMdbDateTime::Sleep(1);
    
    //判断OraRep进程是否存在
    char sProcName[MAX_NAME_LEN] = {0};
    sprintf(sProcName,"%s %s 1 -1","mdbDbRep",m_sDsn);
    if(TMdbOS::IsProcExist(sProcName))
    {
        //TADD_WARNING("Create(%s):[%s] is stil exist! please check it ,if it is in other user.",m_sDsn,sProcName);
        CHECK_RET(ERR_APP_PROC_ALREADY_EXIST,"Create(%s):[%s] is stll exist!please kill it first.",m_sDsn,sProcName);
    }
	#endif
	
    CHECK_RET(CreateSysMem(),"(%s)CreateSysMem() failed.",m_sDsn);//创建共享内存
    CHECK_RET(CreateTableSpace(),"(%s) :CreateTableSpace() failed.",m_sDsn);//创建表空间
    CHECK_RET(CreateVarChar(),"(%s) :CreateVarChar() failed.",m_sDsn);//创建varchar区
    CHECK_RET(CreateTable(),"(%s) :CreateTable() failed.",m_sDsn);//初始化表
    CHECK_RET(LoadData(),"(%s) : LoadData() failed.", m_sDsn);//数据上载
    CHECK_RET(LoadSysData(),"(%s) : LoadSysData() failed.",m_sDsn); //初始化系统表
    CHECK_RET_NONE(LoadSequence(),"(%s) : LoadSequence() failed.",m_sDsn); // //加载sequence
    TMdbShmDSN * pShmDSN = TMdbShmMgr::GetShmDSN(m_sDsn);
    CHECK_OBJ(pShmDSN);
    TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Manager Shm Used=[%d]MB",pShmDSN->GetUsedSize()/(1024*1024));

    CHECK_RET(ReportState(E_MDB_STATE_CREATED), "CheckIfReportState failed.");//向mdbServer上报状态

    TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Create QuickMDB [%s]",m_sDsn);

    // 备份配置文件
    CHECK_RET(m_pConfig->BackUpConfig(),"backup config file failed.");
    
    // 清理配置变更文件
    CHECK_RET(m_pConfig->ClearAlterFile(),"clear table alter config file failed.");
    
    TADD_FUNC("(%s)Finish.", m_sDsn);
    return iRet;
}
/******************************************************************************
* 函数名称	:  Destroy
* 函数描述	:  销毁某个内存数据库
* 输入		:  bForceFlag 是否强制删除标示
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TMdbCtrl::Destroy(const char* pszDSN, bool bForceFlag)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pszDSN);
    SAFESTRCPY(m_sDsn,sizeof(m_sDsn),pszDSN);
    TMdbNtcStrFunc::ToUpper(m_sDsn);
    TMdbShmDSN* pMdbShmDSN = TMdbShmMgr::GetShmDSN(m_sDsn,true);
    if(pMdbShmDSN == NULL)
    {
        TADD_NORMAL("(%s): Shared memory has been destroyed OK",m_sDsn);
        return 0;
    }
    Stop(m_sDsn);
	
	m_pConfig = TMdbConfigMgr::GetDsnConfig(pszDSN);
    CHECK_OBJ(m_pConfig);
    
    //打印出仍然存在的进程信息(业务进程),需要手动kill掉
    int iCheckProcRet = 0;
    TShmList<TMdbProc> & tProcList = pMdbShmDSN->m_ProcList;
    TShmList<TMdbProc>::iterator itor = tProcList.begin();
    for(;itor != tProcList.end();++itor)
    {
        TMdbProc & tMdbProc = *itor; 
        if(tMdbProc.sName[0] == 0   || TMdbNtcStrFunc::FindString(tMdbProc.sName,"QuickMDB") != -1 ||
                false == TMdbOS::IsProcExistByPopen(tMdbProc.iPid) )
        {//已退出的跳过,QuickMDB进程不强制退出
            continue;
        }
        iCheckProcRet = ERR_APP_PROC_ALREADY_EXIST;
        TADD_WARNING("[Notice] Process[%s][%d] is still connecting to QMDB, please kill it manually. ^_^",tMdbProc.sName,tMdbProc.iPid);
    }
    if (false == bForceFlag)
    {//不强制删除时，有业务进程连接就不允许删除
        CHECK_RET(iCheckProcRet,"Proc Connecting QMDB ,Failed to destroy the shared memory[%s].",m_sDsn);
    }

    CHECK_RET(ReportState(E_MDB_STATE_DESTROYED), "CheckIfReportState failed.");//向mdbServer上报状态

    TADD_NORMAL_TO_CLI(FMT_CLI_START,"Destroy QuickMDB[%s]",m_sDsn);
    TADD_FLOW("[%s] Begin Destroy share memory!",m_sDsn);
    CHECK_RET(pMdbShmDSN->Destroy(),"Failed to destroy the shared memory[%s].",m_sDsn);
    TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Destroy QuickMDB[%s]",m_sDsn);

    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* 函数名称	:  Start
* 函数描述	:  启动某个内存数据库
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  
*******************************************************************************/
int TMdbCtrl::Start()
{
    TADD_FUNC("TMdbCtrl::Start(%s) : Start.", m_sDsn);
    TADD_NORMAL_TO_CLI(FMT_CLI_START,"Start QuickMDB[%s]",m_sDsn);
    
    if(IsMdbCreate() == false)
    {//mdb还没创建
        TADD_NORMAL_TO_CLI(FMT_CLI_FAILED,"QuickMDB is not create, Start QuickMDB[%s]",m_sDsn);
        return ERR_DB_NOT_CREATE;
    }
    m_pConfig->SetFlag(true);
    //加锁文件
    int iRet = 0;
    CHECK_RET(LockFile(),"Lock file failed. Please check if QMDB has been started.");
    TMdbShmDSN* pMdbShmDSN = TMdbShmMgr::GetShmDSN(m_sDsn);
    CHECK_OBJ(pMdbShmDSN);
    CHECK_RET(CheckSystem(),"CheckSystem failed.");
    TMdbDSN *pTMdbDSN = pMdbShmDSN->GetInfo();
    CHECK_OBJ(pTMdbDSN);
    pTMdbDSN->cState = DB_running;  //WIN32上这句有异常抛出,原因待查.
    if(!CheckVersion(pTMdbDSN))
    {//版本不一致，不能启动，可能版本太低
        CHECK_RET(ERR_DB_VERSION_NOT_MATCH,"Version is not match, cannot start.");
    }
    //更新时差
    pTMdbDSN->m_iTimeDifferent = TMdbDateTime::GetTimeDifferent();
    
    CHECK_RET(m_tProcCtrl.Init(m_sDsn),"Can't m_tProcCtrl.Init=[%s].",pTMdbDSN->sName);
    
    //清理下管理区的进程信息，将已经不存在的进程信息删掉
    m_tProcCtrl.ClearProcInfo();
    std::vector<std::string > vProcToStart;
    CHECK_RET(GetProcToStart( vProcToStart),"GetProcToStart failed.");
    int i = 0;
    for(i = 0;i < (int)vProcToStart.size();++i)
    {
        CHECK_RET(m_tProcCtrl.Restart(vProcToStart[i].c_str()),"restart[%s] error.",vProcToStart[i].c_str());
    }
    TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Start QuickMDB[%s]",m_sDsn);

    //running状态由mdbRep来上报，建立与mdbServer的长连接，异常断链时，mdbServer可以监测到
    //CHECK_RET(ReportState(E_MDB_STATE_RUNNING), "CheckIfReportState failed.");//向mdbServer上报状态
    return iRet;
}

//锁文件
int TMdbCtrl::UnLockFile()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    //检测文件是否存在
    bool bExist = TMdbNtcFileOper::IsExist(m_sLockFile);
    if(bExist == true)
    {
        //对文件释放锁
        if(UnLockFile(m_sLockFile) < 0)
        {
            CHECK_RET(ERR_OS_LOCK_FILE,"Can't UnLockFile=[%s].", m_sLockFile);
        }
        //删除锁文件
        TMdbNtcFileOper::Remove(m_sLockFile);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

//停止某个内存数据库
int TMdbCtrl::Stop(const char*pszDSN)
{
    TADD_FUNC("Start.");
    SAFESTRCPY(m_sDsn,sizeof(m_sDsn),pszDSN);
    TMdbNtcStrFunc::ToUpper(m_sDsn);
    TADD_NORMAL_TO_CLI(FMT_CLI_START,"Stop QuickMDB[%s]",m_sDsn);
    int iRet  = 0;
    TMdbShmDSN* pMdbShmDSN = TMdbShmMgr::GetShmDSN(m_sDsn,true);
    if(pMdbShmDSN == NULL)
    {
        TADD_NORMAL("(%s): Share Memory has not been created",m_sDsn);
        return 0;
    }
    TMdbDSN *pTMdbDSN = pMdbShmDSN->GetInfo();
    CHECK_OBJ(pTMdbDSN);

    pTMdbDSN->cState = DB_stop;  //WIN32上这句有异常抛出,原因待查.
    CHECK_RET(m_tProcCtrl.Init(m_sDsn),"Can't m_tProcCtrl.Init=[%s].",pTMdbDSN->sName);
    m_tProcCtrl.StopAll();
    m_tProcCtrl.ClearProcInfo();
    UnLockFile();

    //清除所有链接信息
    TMdbLinkCtrl tLinkCtrl;
    tLinkCtrl.Attach(m_sDsn);
    tLinkCtrl.ClearAllLink();
    
   
    TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Stop QuickMDB[%s]",m_sDsn);

    //CHECK_RET(ReportState(E_MDB_STATE_CREATED), "CheckIfReportState failed.");//向mdbServer上报状态

    TADD_FUNC("Stop(%s) : Finish.", m_sDsn);
    return iRet;
}


int TMdbCtrl::LockFile()
{
    TADD_FUNC("TMdbCtrl::LockFile() : Start.");
    char sPath[128];
    memset(sPath, 0, sizeof(sPath));
//拼写出文件名
#ifndef _WIN32
    sprintf(m_sLockFile, "%s/etc/minidb_%s.lock", getenv("QuickMDB_HOME"), m_sDsn);
    snprintf(sPath, sizeof(sPath),"%s/etc", getenv("QuickMDB_HOME"));
#else
    sprintf(m_sLockFile, "C:\\minidb\\minidb_%s.lock", m_sDsn);
    SAFESTRCPY(sPath,sizeof(sPath),"C:\\minidb");
#endif
    int iRet = 0;
    //检测文件是否存在
    bool bExist = TMdbNtcFileOper::IsExist(m_sLockFile);
    //创建文件
    if(bExist == false)
    {
        //创建目录
        bExist = TMdbNtcDirOper::IsExist(sPath);
        if(bExist == false)
        {
            if(TMdbNtcDirOper::MakeFullDir(sPath) == false)
            {
                CHECK_RET(ERR_OS_CREATE_DIR,"Can't create dir=[%s], errno=%d, errmsg=%s.",
                    sPath, errno, strerror(errno));
            }
        }
        //生成文件
        FILE* fp = fopen(m_sLockFile, "w+");
        if(fp == NULL)
        {
            CHECK_RET(ERR_OS_NO_MEMROY,"Can't open file[%s],errno=%d, errmsg=%s.",
                m_sLockFile, errno, strerror(errno));
        }
        SAFE_CLOSE(fp);
    }
    //对文件加锁
    if(LockFile(m_sLockFile) < 0)
    {
        CHECK_RET(ERR_OS_LOCK_FILE,"Can't LockFile=[%s].",m_sLockFile);
    }
    TADD_FUNC("Finish.");
    return iRet;
}


//获得正确的启动方式，如果可以连接到对端则是DB_NORMAL_LOAD, 如果链接不上，则DB_SELF_CREATE
int TMdbCtrl::GetStartMethod()
{
    TADD_FUNC("TMdbCtrl::GetStartMethod() : Start.");

    if(TMdbSocketTry::TrySocket(m_pConfig->GetDSN()->sPeerIP, m_pConfig->GetDSN()->iPeerPort) == true)
    {
        //优先判断本地的standby机器是否正常
        SAFESTRCPY(sPeerIP,sizeof(sPeerIP),m_pConfig->GetDSN()->sPeerIP);
        iPeerPort = m_pConfig->GetDSN()->iPeerPort;
        TADD_NORMAL_TO_CLI(FMT_CLI_START,"Load from local standby,ip=[%s],port=[%d].",sPeerIP,iPeerPort);
        return DB_NORMAL_LOAD;
    }
    else if(TMdbSocketTry::TrySocket(m_pConfig->GetDSN()->sStandByIP, m_pConfig->GetDSN()->iStandbyPort) == true)
    {
        //其次判断容灾站点的standby机器是否正常
        SAFESTRCPY(sPeerIP,sizeof(sPeerIP),m_pConfig->GetDSN()->sStandByIP);
        iPeerPort = m_pConfig->GetDSN()->iStandbyPort;
        TADD_NORMAL_TO_CLI(FMT_CLI_START,"Load from peer standby,ip=[%s],port=[%d].",sPeerIP,iPeerPort);
        return DB_NORMAL_LOAD;
    }
    else if(TMdbSocketTry::TrySocket(m_pConfig->GetDSN()->sActiveIP, m_pConfig->GetDSN()->iActivePort) == true)
    {
        //最后判断容灾站点的active机器是否正常
        SAFESTRCPY(sPeerIP,sizeof(sPeerIP),m_pConfig->GetDSN()->sActiveIP);
        iPeerPort = m_pConfig->GetDSN()->iActivePort;
        TADD_NORMAL_TO_CLI(FMT_CLI_START,"Load from peer active,ip=[%s],port=[%d].",sPeerIP,iPeerPort);
        return DB_NORMAL_LOAD;
    }
    else
    {
        TADD_FUNC("TMdbCtrl::GetStartMethod() : Finish=[DB_SELF_CREATE].");
        return DB_SELF_CREATE;
    }
    TADD_FUNC("TMdbCtrl::GetStartMethod() : Finished.");
}


//创建共享内存块
int TMdbCtrl::CreateSysMem()
{
    TADD_FUNC("Start.");
    int iRet = TMdbShmMgr::CreateMgrShm(*m_pConfig);
    TADD_FUNC("Finish.");
    return iRet;
}
//创建表空间
int TMdbCtrl::CreateTableSpace()
{
    TADD_FUNC("Start.");
    TMdbTableSpaceCtrl tsCtrl;
    int iRet = 0;
	if(!m_bLoadFromDisk)
    {
        TMdbStorageEngine tStorageEngine;
        tStorageEngine.Attach(m_sDsn);
        CHECK_RET(tStorageEngine.RemoveNormalFile(),"RemoveNormalFile failed");
        CHECK_RET(tStorageEngine.RemoveVarcharFile(),"RemoveVarcharFile failed");
        CHECK_RET(tStorageEngine.RemoveChangeFile(),"RemoveChangeFile failed");
    }
    //逐个控制
    for(int i=0; i<m_pConfig->GetTableSpaceCounts(); ++i)
    {
        TADD_DETAIL("No.%d TableSpace.", i);
        //取得表空间信息
        TMdbTableSpace* pTableSpace = m_pConfig->GetTableSpace(i);
        if(pTableSpace == NULL)
            continue;
        CHECK_RET(tsCtrl.CreateTableSpace(pTableSpace, m_pConfig, (!m_bLoadFromDisk)&&pTableSpace->m_bFileStorage),"Can't CreateTableSpace=[%s].",pTableSpace->sName);//创建表空间
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Create TableSpace[%s].",pTableSpace->sName);
    }
    TADD_FUNC("Finish.");
    return iRet;
}

//初始化系统表
int TMdbCtrl::LoadSysData()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TMdbDatabase *mdb = new TMdbDatabase();
    CHECK_OBJ(mdb);
    try
    {
        if(mdb->ConnectAsMgr(m_sDsn) == false)
        {
            mdb->Disconnect();
            SAFE_DELETE(mdb);
            CHECK_RET(ERR_DB_NOT_CONNECTED,"connect(%s) fail",m_sDsn);
        }
    }
    catch(TMdbException& e)
    {
        mdb->Disconnect();
        SAFE_DELETE(mdb);
        CHECK_RET(ERR_DB_NOT_CONNECTED,"connect(%s)ERROR_SQL=%s.\nERROR_MSG=%s\n",
            m_sDsn, e.GetErrSql(), e.GetErrMsg());
    }
    TMdbSysTableSync tSysTableSync;
    CHECK_RET(tSysTableSync.Init(m_sDsn),"tSysTableSync.Init failed");
    CHECK_RET(tSysTableSync.SyncAll(),"tSysTableSync.SyncAll failed");
    CHECK_RET(LoadDBAUser(mdb),"LoadDBAUser failed.");
    CHECK_RET(LoadDBADual(mdb),"LoadDBADual failed.");
    //dba_***系统表信息填充放到start进程中处理。
    mdb->Disconnect();
    SAFE_DELETE(mdb);
    TADD_FUNC(" Finish.");
    return iRet;
}
//创建varchar管理区
int TMdbCtrl::CreateVarChar()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TMdbVarCharCtrl varCharCtrl;
    varCharCtrl.Init(m_pConfig->GetDSN()->sName);
    CHECK_RET(varCharCtrl.CreateVarChar(),"Can't CreateVarchar.");
    TADD_NORMAL("Create VarChar SuccessFul.");
    TADD_FUNC("Finish.");
    return iRet;
}

//创建表
int TMdbCtrl::CreateTable()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TMdbShmDSN* pMdbShmDSN = TMdbShmMgr::GetShmDSN(m_sDsn);
    TMdbTable *  pNewTable = NULL;
    CHECK_OBJ(pMdbShmDSN);
    for(int i=0; i<MAX_TABLE_COUNTS; ++i)
    {
        TMdbTable* pTmp = m_pConfig->GetTableByPos(i);
        if(NULL != pTmp)
        {
            CHECK_RET(pMdbShmDSN->AddNewTable(pNewTable,pTmp),"AddNewTable[%s] failed.",pTmp->sTableName);
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Create Table[%s].",pNewTable->sTableName);
        }
    }
    TADD_FUNC("Finish.");
    return iRet;
}

//上载数据
int TMdbCtrl::LoadData()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TMdbShmDSN* pMdbShmDSN = TMdbShmMgr::GetShmDSN(m_sDsn);
    CHECK_OBJ(pMdbShmDSN);
	pMdbShmDSN->GetInfo()->m_bLoadFromDisk = m_bLoadFromDisk;
    if(m_bLoadFromDisk)
    {//处理临时文件
    	TMdbCheckPoint tMdbCheckPoint;
		CHECK_RET(tMdbCheckPoint.Init(m_sDsn),"Init failed");
		CHECK_RET(tMdbCheckPoint.LinkFile(),"LinkFile failed");
		//加载
		if(tMdbCheckPoint.NeedFlushFile())
		{
			CHECK_RET(tMdbCheckPoint.FlushStorageFile(),"FlushStorageFile failed");
			CHECK_RET(tMdbCheckPoint.ClearRedoLog(),"ClearRedoLog failed");
		}
		//清除
		char changeStartFile[MAX_NAME_LEN] = {0};
		snprintf(changeStartFile,sizeof(changeStartFile),"%s/%s",pMdbShmDSN->GetInfo()->sStorageDir,"CHANGE.START");
		if(TMdbNtcFileOper::IsExist(changeStartFile))
		{
			TMdbStorageEngine tStorageEngine;
            tStorageEngine.Attach(m_sDsn);
			CHECK_RET(tStorageEngine.RemoveChangeFile(),"RemoveChangeFile failed");
		}
		
		//从磁盘加载
        TADD_NORMAL_TO_CLI(FMT_CLI_START,"Load From Disk.");
        iRet = LoadFromDisk();
        if (iRet == ERROR_SUCCESS)
        {
            pMdbShmDSN->GetInfo()->m_bLoadFromDiskOK = true;
            //如果是从磁盘加载，并且页发生改变，则需要清理掉原来的文件，重新刷
            if(NeedRemoveFile())
            {
                TMdbStorageEngine tStorageEngine;
                tStorageEngine.Attach(m_sDsn);
                CHECK_RET(tStorageEngine.FlushFull(),"FlushFull failed");
            }
            TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Load From Disk.");
        }
        else
        {
			TADD_ERROR(ERR_DB_LOAD_DATA_FAILED, "Load from disk failed! Please destroy dsn and re-create!");
			return ERR_DB_LOAD_DATA_FAILED;
        }
    }
    pMdbShmDSN->GetInfo()->cState = DB_loading;
    if(m_pConfig->GetIsStartShardBackupRep() == true)
    {
        CHECK_RET(LoadFromShardBackupHost(),"LoadFromStandbyHost failed.");
    }
    
    //上载需要从Oracle上载的表
    if(m_pConfig->GetDSN()->bIsOraRep == true)
    {
    	#ifndef DB_NODB 
        CHECK_RET(LoadFromOra(),"LoadFromOra failed");
		#else
		TADD_NORMAL("Will not LoadDataFromDB in NO DB Mode");
		#endif
    }
    
    pMdbShmDSN->GetInfo()->cState = DB_running;
    
    if(!m_bLoadFromDisk)
    {//做首次落地
	    TMdbCheckPoint tTMdbCheckPoint;
        CHECK_RET(tTMdbCheckPoint.Init(m_sDsn),"Init failed.");
        CHECK_RET(tTMdbCheckPoint.LinkFile(),"Attach failed.");
        CHECK_RET(tTMdbCheckPoint.DoCheckPoint(),"FlushOneTable falied");
    }
	/*if(m_bLoadFromDisk && !pMdbShmDSN->GetInfo()->m_bLoadFromDiskOK)
	{
		TMdbStorageEngine tStorageEngine;
        tStorageEngine.Attach(m_sDsn);
        CHECK_RET(tStorageEngine.FlushFull(),"FlushFull failed");
	}*/
    TADD_FUNC("Finish.");
    return iRet;
}


//跟备机通信,从备机上载数据
int TMdbCtrl::LoadFromStandbyHost()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    // TODO:
    /*
    //判断如果没有表的表属性是主备同步的，则退出
    int iRepCount = 0;
    for(int iRepCount=0; iRepCount<MAX_TABLE_COUNTS; ++iRepCount)
    {
        TMdbTable* pTmp = m_pConfig->GetTableByPos(iRepCount);
        if(pTmp == NULL )
        {
            continue;
        }
        if(pTmp->tColumn[0].iRepAttr == Column_Ora_Rep || pTmp->tColumn[0].iRepAttr == Column_To_Rep)
        {
            break;
        }
    }
    if(iRepCount == MAX_TABLE_COUNTS)
    {
        TADD_NORMAL("Do not need  load from Peer.");
        return iRet;
    }
    TMdbLoadDataThread load;
    iRet = load.Init(m_pConfig);
    CHECK_RET(iRet,"Can't  init LoadFromPeer.");
    //一个个的把表数据从备机上载上来
    iRet = load.LoadAll();
    if(iRet == ERR_NET_PEER_INVALID)
    {
        return 0;
    }
    else if(iRet < 0)
    {
        //删除已经创建的共享内存，保证加载失败，应用无法启动
        CHECK_RET(Destroy(m_sDsn),"Destroy Faild!.");
    }
    */
    CHECK_RET(iRet,"Can't load some tables.");
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbCtrl::LoadFromShardBackupHost()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    int iRepCount = 0;
    for(int iRepCount=0; iRepCount<MAX_TABLE_COUNTS; ++iRepCount)
    {
        TMdbTable* pTmp = m_pConfig->GetTableByPos(iRepCount);
        if(pTmp == NULL )
        {
            continue;
        }
        if(pTmp->m_bShardBack)
        {
            break;
        }
    }
    if(iRepCount == MAX_TABLE_COUNTS)
    {
        TADD_NORMAL("Do not need  load from Peer.");
        return iRet;
    }
    TADD_NORMAL_TO_CLI(FMT_CLI_START,"Load From shard backup hosts.");
    TMdbRepLoadDataCtrl tLoadCtrl;
    CHECK_RET(tLoadCtrl.Init(m_sDsn, m_pConfig),"Can't  init LoadFromPeer.");
    iRet = tLoadCtrl.LoadData();
    if(iRet == ERR_NET_PEER_INVALID)
    {
        return 0;
    }
    else if(iRet < 0)
    {
        //删除已经创建的共享内存，保证加载失败，应用无法启动
		if (ERROR_SUCCESS!=Destroy(m_sDsn))
		{
			CHECK_RET(ERR_OS_DESTROY_SHME,"Destroy Dsn Failed!");
		}
    }
    
    CHECK_RET(iRet,"Can't load some tables.");
    TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Load from shard backup hosts");
    TADD_FUNC("Finish.");
    return iRet;
}

//数据库是否需要停止
bool TMdbCtrl::IsStop()
{
    TMdbShmDSN* pMdbShmDSN = TMdbShmMgr::GetShmDSN(m_sDsn);
    if(pMdbShmDSN == NULL)
    {
        TADD_ERROR(ERROR_UNKNOWN,"GetShmDSN() failed.");
        return true;
    }

    TADD_DETAIL("TMdbCtrl::IsStop() : cState=%c.", pMdbShmDSN->GetInfo()->cState);
    return pMdbShmDSN->GetInfo()->cState == DB_stop;
}


//从Oracle上载全量数据, 根据配置文件来上载
int TMdbCtrl::LoadFromOra()
{
    TADD_FUNC("Start.");
    int iRet = 0;

    //首先要根据配置文件分配表空间
    TMdbLoadFromDb load;
    
    CHECK_RET(load.Init(m_pConfig),"Can't dispath table-space.");
    //一个个的把表数据上载上来，如果一个表中的所有字段都没有在Oracle中，就意味着这个表不需要上载
    CHECK_RET(load.LoadAll(),"Can't load some tables.");
    
    TADD_FUNC(" Finish.");
    return iRet;
}

//上载dba_user
int TMdbCtrl::LoadDBAUser(TMdbDatabase* pMdb)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    try
    {
        TMdbQuery *pQuery = pMdb->CreateDBQuery();
        if(pQuery == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Can't CreateDBQuery.");
            return -1;
        }
        char sPwd[MAX_NAME_LEN] = {0};
        char sSQL[1024] = "insert into dba_user "
                          "(user_name, user_pwd, access_attr) values ("
                          ":user_name, :user_pwd, :access_attr)";

        pQuery->SetSQL(sSQL);
        //逐个比较表名称
        for(int i=0; i<MAX_USER_COUNT; ++i)
        {
            TMDbUser *pUser = m_pConfig->GetUser(i);
            if(NULL == pUser)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Get User Error,count=[%d]",i);
            }
            if(pUser->sUser[0] == 0)
            {
                continue;
            }
            memset(sPwd,0,sizeof(sPwd));
            TMdbEncrypt::EncryptEx(pUser->sPwd,sPwd);
            TADD_FLOW("user_name=[%s], user_pwd=[%s], access_attr=[%s].",pUser->sUser,sPwd, pUser->sAccess);
            pQuery->SetParameter("user_name",   pUser->sUser);
            pQuery->SetParameter("user_pwd",    sPwd);
            pQuery->SetParameter("access_attr", pUser->sAccess);
            pQuery->Execute();
            pQuery->Commit();
        }
        delete pQuery;
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN," ERROR_SQL=%s.\nERROR_MSG=%s.\n", e.GetErrSql(), e.GetErrMsg());
        iRet = -1;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown Error.");
        iRet = -1;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

//上载dual表
int TMdbCtrl::LoadDBADual(TMdbDatabase* pMdb)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    try
    {
        TMdbQuery *pQuery = pMdb->CreateDBQuery();
        if(pQuery == NULL)
        {
            TADD_ERROR(ERROR_UNKNOWN,"Can't CreateDBQuery.");
            return -1;
        }
        char sSQL[1024] = "insert into dual(dummy) values('X'); ";
        pQuery->SetSQL(sSQL);
        pQuery->Execute();
        pQuery->Commit();
        SAFE_DELETE(pQuery);
    }
    catch(TMdbException& e)
    {
        TADD_ERROR(ERROR_UNKNOWN," ERROR_SQL=%s.\nERROR_MSG=%s.\n", e.GetErrSql(), e.GetErrMsg());
        iRet = -1;
    }
    catch(...)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Unknown Error.");
        iRet = -1;
    }
    TADD_FUNC("Finish.");
    return iRet;
}

int TMdbCtrl::LoadSequence()
{
    int iRet = 0;
    FILE * m_pSeqFile = NULL;
    TMdbShmDSN* pMdbShmDSN = TMdbShmMgr::GetShmDSN(m_sDsn);
    CHECK_OBJ(pMdbShmDSN);
    CHECK_OBJ(m_pConfig);
    char sFileName[MAX_NAME_LEN];
    memset(sFileName,0,sizeof(sFileName));
    sprintf(sFileName,"%s%s",pMdbShmDSN->GetInfo()->sStorageDir,"Sequence.mdb");
    m_pSeqFile = fopen (sFileName,"rb+");

    

	//如果文件不存在，从数据库加载sequence
	if (NULL == m_pSeqFile)
	{
        TMdbDSN* pDsn = pMdbShmDSN->GetInfo();
        CHECK_OBJ(pDsn);
        TMdbDatabase *mdb = new(std::nothrow) TMdbDatabase();
        CHECK_OBJ(mdb);

        try
        {
            if(mdb->ConnectAsMgr(m_pConfig->GetDSN()->sName) == false)
            {
                TADD_ERROR(ERR_DB_NOT_CONNECTED,"ConnectAsMgr(%s) failed",m_pConfig->GetDSN()->sName);
                mdb->Disconnect();
                SAFE_DELETE(mdb);
                return ERR_DB_NOT_CONNECTED;
            }
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(ERR_DB_NOT_CONNECTED,"DSN=%s. ERROR_SQL=%s.\nERROR_MSG=%s\n",m_pConfig->GetDSN()->sName, e.GetErrSql(), e.GetErrMsg());
            mdb->Disconnect();
            SAFE_DELETE(mdb);
            return ERR_DB_NOT_CONNECTED;
        }
        //取到内存中的地址
       
        TMdbDAOLoad *dao = new(std::nothrow) TMdbDAOLoad(m_pConfig);
        CHECK_OBJ(dao);		
		
        if(dao->Connect() == false)
        {
            TADD_ERROR(ERR_APP_CONNCET_ORACLE_FAILED,"Can't dao.Connect().");
            SAFE_DELETE(dao);
            mdb->Disconnect();
            SAFE_DELETE(mdb);
            return ERR_APP_CONNCET_ORACLE_FAILED;
        }
		
		//dba_sequence表上载
		CHECK_RET(dao->LoadSequence(mdb,pDsn),"LoadSequence from DB Failed"); 
		return  iRet;	
	}

	//从文件加载sequence
    char* pBuff = NULL;
    int iBuffSize = MAX_SEQUENCE_COUNTS* sizeof(TMemSeq);
    pBuff = new char[iBuffSize];
    if(pBuff == NULL)
    {
        SAFE_CLOSE(m_pSeqFile);
        return -1;
    }
    int iReadPageCount = fread(pBuff,sizeof(TMemSeq),MAX_SEQUENCE_COUNTS,m_pSeqFile);
    if(iReadPageCount <= 0)
    {
        TADD_NORMAL("No Sequence Need to Load,Cout=[%d],file=[%s]",iReadPageCount,sFileName);
        SAFE_CLOSE(m_pSeqFile);
        SAFE_DELETE(pBuff);
        return iRet;
    }
    TMemSeq*  pSeq = NULL;
    for(int i = 0; i<iReadPageCount;i++)
    {
        pSeq = NULL;
        if(pMdbShmDSN->AddNewMemSeq(pSeq) != 0)
        {
            TADD_ERROR(-1,"AddNewMemSeq failed.");
            SAFE_CLOSE(m_pSeqFile);
            SAFE_DELETE(pBuff);
            return -1;
        }
        memcpy(pSeq,pBuff+(sizeof(TMemSeq)*i),sizeof(TMemSeq));
        pSeq->iCur += pSeq->iStep * m_pConfig->GetDSN()->m_iSeqCacheSize;;;
    }
    SAFE_CLOSE(m_pSeqFile);
    SAFE_DELETE(pBuff);
    return iRet;
}

//内核版本号格式为QuickMDB V*.*.*.日期
bool TMdbCtrl::CheckVersion(TMdbDSN *pTMdbDSN)
{
    TADD_FUNC("Start.");
    bool bRet = true;
    if(NULL == pTMdbDSN)
    {
        TADD_ERROR(ERROR_UNKNOWN,"pTMdbDSN is null.");
        return false;
    }
    int iKcount=0;
    int iPCount=0;
    TMdbNtcSplit mdbKSplit;
    TMdbNtcSplit mdbPSplit;

    mdbPSplit.SplitString(MDB_VERSION, '.');
    mdbKSplit.SplitString(pTMdbDSN->sVersion, '.');
    iKcount = mdbKSplit.GetFieldCount();
    iPCount = mdbPSplit.GetFieldCount();
    //防止老的版本信息格式问题，如minidb_1.0.00
    if(iKcount != iPCount && iKcount < 3)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Last Mem-Version=[%s], Update Mem-Version=[%s],Need to re-create the shared memory.",pTMdbDSN->sVersion, MDB_VERSION);
        return false;
    }
    TADD_NORMAL("Current Version[%s]",MDB_VERSION);
    if(strcmp(mdbPSplit[0],mdbKSplit[0]) != 0 ||
            strcmp(mdbPSplit[1],mdbKSplit[1]) != 0 ||
            strcmp(mdbPSplit[2],mdbKSplit[2]) != 0)
    {
        TADD_ERROR(ERROR_UNKNOWN,"Last Mem-Version=[%s], Update Mem-Version=[%s],Please run destroy cmd first and then run create cmd.", pTMdbDSN->sVersion, MDB_VERSION);
        return false;
    }
    TADD_FUNC("Finish.");
    return bRet;
}
/******************************************************************************
* 函数名称	:  CheckSystem
* 函数描述	:  校验配置项是否与内存一致
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbCtrl::CheckSystem()
{
    TADD_FUNC("Start.");
    int iRet = 0;
    TMdbShmDSN* pMdbShmDSN = TMdbShmMgr::GetShmDSN(m_sDsn);
    CHECK_OBJ(pMdbShmDSN);
    TMdbDSN* pDsn = pMdbShmDSN->GetInfo();
    CHECK_OBJ(pDsn);
    //校验CREATE操作和START操作之间时间间隔是否超时，如果超时需要重新create操作
    if(pDsn->m_bIsRep)
    {
        char sCurTime[MAX_TIME_LEN] = {0};
        if(m_pConfig->GetDSN()->m_iRepFileTimeout == 0)
        {
            m_pConfig->GetDSN()->m_iRepFileTimeout = 12;
        }
        TMdbDateTime::GetCurrentTimeStr(sCurTime);
        if(TMdbDateTime::GetDiffSeconds(sCurTime,pDsn->sCurTime) 
                    > m_pConfig->GetDSN()->m_iRepFileTimeout*60*60)
        {
            TADD_ERROR(ERROR_UNKNOWN,"QMDB stop too long,need to re-create.");
            return ERR_DB_STOP_TIMEOUT;
        }
    }
    //校验表空间属性
    CHECK_RET(CheckTablespace(pMdbShmDSN), "Tablespace in system configuration file are not consistent with what in shared memory.");
     //校验表结构
    CHECK_RET(CheckTables(pMdbShmDSN), "Tables in configuration file are not consistent with what in shared memory."); 
    //根据配置信息，覆盖DSN可变信息
    TMdbCfgDSN* pCfgDSN = m_pConfig->GetDSN();
    if(pCfgDSN != NULL)
    {
        SAFESTRCPY(pDsn->sPeerIP, MAX_IP_LEN, pCfgDSN->sPeerIP);
        pDsn->iLocalPort = pCfgDSN->iLocalPort;
        pDsn->iPeerPort  = pCfgDSN->iPeerPort;
    }
    //校验系统属性(如要校验主备同步区、oracle同步区、数据捕获同步区)
    CHECK_RET(CheckSysConfig(pCfgDSN,pMdbShmDSN),"System configuration parameters in the configuration file and inconsistencies in memory.");
    TADD_FUNC("Finish(true).");
    return iRet;
}
/******************************************************************************
* 函数名称	:  GetProcToStart
* 函数描述	:  获取需要启动的进程信息
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbCtrl::GetCSMethod(int iAgentPort)
{
	int j = 0,k = 0;
	int iuseNtc = 0;
	for(j=0; j<MAX_AGENT_PORT_COUNTS; j++)
	{
		TADD_NORMAL("iNtcPort:%d,iAgentPort:%d",m_pConfig->GetDSN()->iNtcPort[j],iAgentPort);
		if(iAgentPort ==  m_pConfig->GetDSN()->iNtcPort[j])
				break;
	}
	for(k=0; k<MAX_AGENT_PORT_COUNTS; k++)
	{
		TADD_NORMAL("iNoNtcPort:%d,iAgentPort:%d",m_pConfig->GetDSN()->iNoNtcPort[k],iAgentPort);
		if(iAgentPort ==  m_pConfig->GetDSN()->iNoNtcPort[k])
				break;
	}
	if(j<MAX_AGENT_PORT_COUNTS && k<MAX_AGENT_PORT_COUNTS)
	{
		TADD_ERROR(ERR_NET_IP_INVALID,"the ntc method set in xml is conflict,the agent with this port %d can't start",iAgentPort);
		iuseNtc = -1;
	}
	else if(j<MAX_AGENT_PORT_COUNTS)
		iuseNtc = 1;
	else if(k<MAX_AGENT_PORT_COUNTS)
		iuseNtc = 0;
	else
		iuseNtc = m_pConfig->GetDSN()->m_bUseNTC?1:0;
	return iuseNtc;
		
}

int TMdbCtrl::GetProcToStart(std::vector<std::string > & vProcToStart)
{
    int iRet = 0;
    char sProcFullName[MAX_NAME_LEN] = {0};

	#ifndef DB_NODB
    //如果需要与Oracle同步，则启动相关进程
    if(m_pConfig->GetDSN()->bIsOraRep == true)
    {
        if(m_pConfig->GetIsStartOracleRep() == true)
        {//MDB=======>ORACLE
            for(int i=-1; i<m_pConfig->GetDSN()->iOraRepCounts; ++i)
            {
                sprintf(sProcFullName, "mdbDbRep %s %d %d", m_sDsn, m_pConfig->GetDSN()->iOraRepCounts, i);
                vProcToStart.push_back(sProcFullName);
            }
        }
        else
        {
            TADD_FLOW("need't start proc [mdbFlushDb,mdbDbRep].");
        }

        // DB2MDB
        //如果所有的表都不需要重Oralce同步到QuickMDB，则不需要启动mdbFlushFromOra进程
        if(m_pConfig->GetIsStartFlushFromOra())
        {
            sprintf(sProcFullName, "mdbFlushFromDb %s", m_sDsn);
            vProcToStart.push_back(sProcFullName);
            //启动mdbClean进程,清理%DSN%_MDB_CHANGE_NOTIF 数据
            sprintf(sProcFullName, "mdbClean %s", m_sDsn);
            vProcToStart.push_back(sProcFullName);
        }
        else
        {
             TADD_FLOW("need't start proc mdbFlushFromDb.");
        }
    }
	#endif

    //  如果需要，启动分片备份进程
    if(m_pConfig->GetIsStartShardBackupRep())
    {
        sprintf(sProcFullName, "mdbRep %s", m_sDsn);
        vProcToStart.push_back(sProcFullName);
        
        sprintf(sProcFullName, "mdbRepClient %s", m_sDsn);
        vProcToStart.push_back(sProcFullName);
        
         sprintf(sProcFullName, "mdbRepServer %s", m_sDsn);
        vProcToStart.push_back(sProcFullName);
    }
    sprintf(sProcFullName, "mdbFlushRep %s", m_sDsn);
    vProcToStart.push_back(sProcFullName);

    
    //启动checkpoint进程
    if(m_pConfig->GetIsStartFileStorageProc())
    {
        sprintf(sProcFullName, "mdbCheckPoint %s", m_sDsn);
        vProcToStart.push_back(sProcFullName);
    }

    //启动flush sequence进程
    sprintf(sProcFullName, "mdbFlushSequence %s", m_sDsn);
    vProcToStart.push_back(sProcFullName);
    
    //启动mdbAgent => cs代理进程
	for(int i = 0; i<MAX_AGENT_PORT_COUNTS; i++)
	{
		
		if(m_pConfig->GetDSN()->iAgentPort[i] > 0)
		{
			int iuseNtc=  GetCSMethod(m_pConfig->GetDSN()->iAgentPort[i]);
			if(iuseNtc == -1)
				continue;
			sprintf(sProcFullName, "mdbAgent %s %d %d", m_sDsn, m_pConfig->GetDSN()->iAgentPort[i],iuseNtc);
    		vProcToStart.push_back(sProcFullName);
		}
		else
		{
			break;
		}
	}

     //启动mdbJob进程
    sprintf(sProcFullName, "mdbJob %s", m_sDsn);
    vProcToStart.push_back(sProcFullName);

    TADD_FUNC("(%s) : Finish.", m_sDsn);
    return iRet;
}

bool TMdbCtrl::NeedRemoveFile()
{
    TMdbShmDSN* pMdbShmDSN = TMdbShmMgr::GetShmDSN(m_sDsn);
    TMdbConfig* m_pConfig = TMdbConfigMgr::GetMdbConfig(pMdbShmDSN->GetInfo()->sName);
    TShmList<TMdbTable>::iterator itor = pMdbShmDSN->m_TableList.begin();
    for(;itor != pMdbShmDSN->m_TableList.end();++itor)
    {
        TMdbTable * pTable = &(*itor);
        if(pTable->bIsSysTab){continue;}//跳过系统表
        if(m_pConfig->IsTableAlter(pTable->sTableName))
        {
            return  true;
        }
        TIntAlterAttr* pIntAlterAttr = NULL;
        if(m_pConfig->IsPageSizeAlter(pTable->m_sTableSpace, pIntAlterAttr))
        {
            return true;
        }
        
    }

    return false;    
}

/******************************************************************************
* 函数名称	:  LoadFromDisk
* 函数描述	:  从磁盘加载
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  dong.chun
*******************************************************************************/
int TMdbCtrl::LoadFromDisk()
{
    int iRet = 0;
    TADD_FUNC("Start");
    TMdbLoadFromDisk tMdbLoadFromDisk;
    CHECK_RET(tMdbLoadFromDisk.Init(m_sDsn),"Init failed.");
    CHECK_RET(tMdbLoadFromDisk.LinkFile(),"LinkFile failed.");
    CHECK_RET(tMdbLoadFromDisk.LoadVarcharData(),"LoadVarcharData failed.");
    CHECK_RET(tMdbLoadFromDisk.LoadNormalData(),"LoadNormalData failed.");
    CHECK_RET(tMdbLoadFromDisk.LoadRedoLog(),"LoadRedoLog failed.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  CheckTablespace
* 函数描述	:  检查配置文件和内存中的表空间是否一致
* 输入		:  
* 输出		:  
* 返回值	:  0- 成功; 非0- 失败
* 作者		:  jiang.lili
*******************************************************************************/
int TMdbCtrl::CheckTablespace(TMdbShmDSN* pMdbShmDSN)
{
    int iRet = 0;
    TMdbTableSpace* pCTableSpace = NULL;
    TMdbTableSpace* pMTableSpace = NULL;
    //配置文件与表空间比较
    for(int i=0; i<m_pConfig->GetTableSpaceCounts(); ++i)
    {
        pCTableSpace = m_pConfig->GetTableSpace(i);
        CHECK_OBJ(pCTableSpace);
        if(strlen(pCTableSpace->sName) == 0)
        {
            continue;
        }
        pMTableSpace = pMdbShmDSN->GetTableSpaceAddrByName(pCTableSpace->sName);
        if(NULL == pMTableSpace)
        {
            CHECK_RET(ERR_APP_CONFIG_TPS_DIFF_MEMORY,"TableSpace(Name=[%s]) in configuration file is not found in shared memory.", pCTableSpace->sName);
        }
    }
    //表空间与配置文件比较
    TShmList<TMdbTableSpace>::iterator itor = pMdbShmDSN->m_TSList.begin();
    for(;itor != pMdbShmDSN->m_TSList.end();++itor)
    {
        pMTableSpace = &(*itor);
        CHECK_OBJ(pMTableSpace);
        if(strlen(pMTableSpace->sName) == 0)
        {
            continue;
        }
        pCTableSpace = m_pConfig->GetTableSpaceByName(pMTableSpace->sName);
        if(NULL == pCTableSpace)
        {
            CHECK_RET(ERR_APP_CONFIG_TPS_DIFF_MEMORY," TableSpace(Name=[%s]) in shared memory is not found in configuration file.", pMTableSpace->sName);
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(pCTableSpace->sName, pMTableSpace->sName) != 0 ||
            pCTableSpace->iPageSize != pMTableSpace->iPageSize ||
            pCTableSpace->iRequestCounts != pMTableSpace->iRequestCounts)
        {
            CHECK_RET(ERR_APP_CONFIG_TPS_DIFF_MEMORY,"Tablespace([%s]) has been changed after QuickMDB created. In memory, [name=%s][pagesize=%d][requestCounts=%d]; while in file, [name=%s][pagesize=%d][requestCounts=%d],",
                pMTableSpace->sName, pMTableSpace->sName, pMTableSpace->iPageSize, pMTableSpace->iRequestCounts,
                pCTableSpace->sName, pCTableSpace->iPageSize, pCTableSpace->iRequestCounts);
        }
    }
    return iRet;
}

/******************************************************************************
* 函数名称	:  CheckTables
* 函数描述	:  检查配置文件和内存中的表是否一致
* 输入		:  
* 输出		:  
* 返回值	:  0- 成功; 非0- 失败
* 作者		:  jiang.lili
*******************************************************************************/
int TMdbCtrl::CheckTables(TMdbShmDSN* pMdbShmDSN)
{
    int iRet = 0;
    TMdbTable* pCfgTable = NULL;
    TMdbTable* pMemTable = NULL;
    //配置文件与共享内存比较
    for(int i=0; i<MAX_TABLE_COUNTS; ++i)
    {
        pCfgTable = m_pConfig->GetTableByPos(i);
        if(pCfgTable == NULL ||strlen(pCfgTable->sTableName) == 0)
        {
            continue;
        }
        pMemTable = pMdbShmDSN->GetTableByName(pCfgTable->sTableName);
        if(NULL == pMemTable || strlen(pMemTable->sTableName) == 0)
        {
            CHECK_RET(ERR_APP_CONFIG_TABLE_NOT_IN_MEMORY," Table(Name=[%s], ID=[%d]) in configuration files is not found in shared memory.", pCfgTable->sTableName,i);
        }
    }
    //共享内存与配置文件比较
    TShmList<TMdbTable>::iterator itor = pMdbShmDSN->m_TableList.begin();
    for(;itor != pMdbShmDSN->m_TableList.end();++itor)
    {
        pMemTable = &(*itor);
        CHECK_OBJ(pMemTable);
        if(pMemTable == NULL ||strlen(pMemTable->sTableName) == 0)
        {
            continue;
        }
        pCfgTable = m_pConfig->GetTableByName(pMemTable->sTableName);
        if(NULL == pCfgTable || strlen(pCfgTable->sTableName) == 0)
        {
            CHECK_RET(ERR_APP_CONFIG_TABLE_NOT_IN_MEMORY," Table(Name=[%s]) in shared memory is not found in configuration files.", pMemTable->sTableName);
        }
        //表不一致
        if(m_pConfig->IsDiffToMemTable(pMemTable, pCfgTable))
        {
            TADD_NORMAL("Config table[%s].",pCfgTable->sTableName);
            pCfgTable->Print();
            TADD_NORMAL("Mem Table[%s].",pMemTable->sTableName);
            pMemTable->Print();
            CHECK_RET(ERR_APP_CONFIG_TABLE_NOT_IN_MEMORY,"Configuration files of table(Name=[%s]) have been changed after QuickMDB created", pMemTable->sTableName);
        }
    }

    return iRet;
}

int TMdbCtrl::CheckSysConfig(TMdbCfgDSN* pCfgDSN,TMdbShmDSN* pMdbShmDSN)
{
    int iRet = 0;
    CHECK_OBJ(pCfgDSN);
    CHECK_OBJ(pMdbShmDSN);
    //主要校验与共享内存有关的配置参数
    if(pCfgDSN->bIsOraRep != pMdbShmDSN->GetInfo()->m_bIsOraRep)
    {
        CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,
            "The configuration parameter[is-ora-rep] values[%s] with different memory[%s].",
            pCfgDSN->bIsOraRep?"Y":"N",pMdbShmDSN->GetInfo()->m_bIsOraRep?"Y":"N");
    }
    //主备同步区
    if(pCfgDSN->bIsRep != pMdbShmDSN->GetInfo()->m_bIsRep)
    {
        CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,
            "The configuration parameter[is-rep] values[%s] with different memory[%s].",
            pCfgDSN->bIsRep?"Y":"N",pMdbShmDSN->GetInfo()->m_bIsRep?"Y":"N");
    }
    //数据捕获区
    if(pCfgDSN->bIsCaptureRouter != pMdbShmDSN->GetInfo()->m_bIsCaptureRouter)
    {
        CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,
            "The configuration parameter[is-capture-router] values[%s] with different memory[%s].",
            pCfgDSN->bIsCaptureRouter?"Y":"N",pMdbShmDSN->GetInfo()->m_bIsCaptureRouter?"Y":"N");
    }
    //是否文件存储
    if(pCfgDSN->m_bIsDiskStorage != pMdbShmDSN->GetInfo()->m_bIsDiskStorage)
    {
        CHECK_RET(ERR_APP_CONFIG_ITEM_VALUE_INVALID,
            "The configuration parameter[is-disk-storage] values[%s] with different memory[%s].",
            pCfgDSN->m_bIsDiskStorage?"Y":"N",pMdbShmDSN->GetInfo()->m_bIsDiskStorage?"Y":"N");
    }
    return iRet;
}

int TMdbCtrl::LockFile(const char* pszFile)
{
	char sFileName[256];
	memset(sFileName,0,sizeof(sFileName));
	
	
#ifdef _WIN32
	//拼写出全路径的文件名
	//sprintf(sFileName, "C:\\etc\\%s", pszFile);
	sprintf(sFileName, "%s", pszFile);
	
	//打开文件
	HANDLE hFile = CreateFile(sFileName,     // open Two.txt 
		GENERIC_READ,                 // open for writing 
		0,                            // do not share 
		NULL,                         // no security 
		OPEN_EXISTING,                // open or create 
		FILE_ATTRIBUTE_NORMAL,        // normal file 
		NULL);                        // no attr. template 
	
	if (hFile == INVALID_HANDLE_VALUE) 
	{
		printf("open file:[%s] error, errno:%d\n", sFileName,errno);
		return -2;
	}
	
	//加锁
    int iRet = ::LockFile(hFile, 0, 0, 0, 0); 
	if (!iRet)
	{
		printf("lock file error! errno[%d]\n", errno);
		CloseHandle(hFile);
		return -1;
	}
#else
    int	fileQ = -1;
	//拼写出全路径的文件名
	//sprintf(sFileName, "%s/etc/%s", getenv("HOME"), pszFile);
	sprintf(sFileName, "%s", pszFile);
	
	//打开文件
	fileQ = open(sFileName, O_RDWR);
	if(fileQ < 0)	
	{
		return -2;
	}
	
	//锁住文件
	if(lockf(fileQ, F_TLOCK, 0) < 0)	
	{
		return -1;
	}
#endif
	return 0;	
}

int TMdbCtrl::UnLockFile(const char* pszFile)
{
	//char sFileName[256];
#ifdef _WIN32
#else
	int	fileQ = -1;
	
	//拼写出全路径的文件名
	//sprintf(sFileName, "%s/etc/%s", getenv("HOME"), pszFile);
	//sprintf(sFileName, "%s", pszFile);
	
	//打开文件
	fileQ = open(pszFile, O_RDWR);
	if(fileQ < 0)	
	{
		return -2;
	}
	
	//锁住文件
	if(lockf(fileQ, F_ULOCK, 0) < 0)	
	{
		return -1;
	}
	close(fileQ);
#endif
	return 0;	
}

    /******************************************************************************
    * 函数名称	:  ReportState()
    * 函数描述	:  检查是否需要上报状态，如果需要，向mdbServer上报状态
    * 输入		:  
    * 输出		:  无
    * 返回值	:  成功返回0，失败返回其他值
    * 作者		:  jiang.lili
    *******************************************************************************/
int TMdbCtrl::ReportState(EMdbRepState eState)
{
    int iRet = 0;
	
	if (m_pConfig->GetIsStartShardBackupRep())
	{
		TMdbShmRepMgr *pShmMgr = new (std::nothrow)TMdbShmRepMgr(m_sDsn);
	    CHECK_OBJ(pShmMgr);
	    iRet = pShmMgr->Attach();
		CHECK_RET(iRet, "MdbShmRepMgr Attach Failed.");
		
		 //上报状态
        pShmMgr->SetMdbState(eState);
        iRet = TMdbRepCtrl::ReportState(m_sDsn, pShmMgr->GetRoutingRep()->m_iHostID, eState);
        if (iRet != ERROR_SUCCESS)
        {
            TADD_ERROR(ERR_NET_SEND_FAILED, "Report state to mdbServer failed.");
        }
        else
        {
            TADD_NORMAL("Report state[%d] to mdbServer OK.", eState);
        }
		
		SAFE_DELETE(pShmMgr);
	}
	else
    {
        //未启用分片备份，不需要上报状态
    }

    return ERROR_SUCCESS;
}


//}


