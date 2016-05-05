/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        TMdbLoadFromDb.h
*@Description： 负责从Oracle上载数据；
*@Author:       li.shugang
*@Date：        2008年12月15日
*@History:
******************************************************************************************/

#include "Dbflush/mdbLoadFromDb.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbDateTime.h"
#include "Control/mdbTableSpaceCtrl.h"
#include "Helper/mdbBase.h"
#include "Control/mdbIndexCtrl.h"
#include "Control/mdbProcCtrl.h"

#include <errno.h>
#include <sys/types.h>

//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{

    TMdbLoadFromDb::TMdbLoadFromDb()
    {
        m_pConfig = NULL;
        m_pShmDSN = NULL;
    }
    TMdbLoadFromDb::~TMdbLoadFromDb()
    {

    }
    /******************************************************************************
    * 函数名称  :  Init()
    * 函数描述  :  根据配置文件分配表空间
    * 输入      :  无
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  li.shugang
    *******************************************************************************/
    int TMdbLoadFromDb::Init(TMdbConfig *pConfig)
    {
        TADD_FUNC("Start.");
        //检测参数
        m_pConfig = pConfig;
        if(m_pConfig == NULL || m_pConfig->GetDSN() == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"TMdbLoadFromDb::Init() : pConfig=NULL.");
            return ERR_APP_INVALID_PARAM;
        }
        m_pShmDSN = TMdbShmMgr::GetShmDSN(m_pConfig->GetDSN()->sName);
        if(m_pShmDSN == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"TMdbTableCtrl::SetDSN() : pszDSN=%s.", m_pConfig->GetDSN()->sName);
            return ERR_APP_INVALID_PARAM;
        }
        TADD_FUNC("TMdbLoadFromDb::Init() : Finish.");

        return 0;
    }
    
    #ifdef WIN32
    DWORD WINAPI TMdbLoadFromDb::agent( void* p )
    #else
    void* TMdbLoadFromDb::agent(void* p)
    #endif
    {
        struct PthreadParam *pThread = (struct PthreadParam*)p;
        (pThread->tmdbLoadfromOra)->svc(pThread->m_sTSName);

        if(p != NULL)
        {
            //free(p);
            delete (struct PthreadParam*)p;
            p = NULL;
        }
        return 0;
    }

    int TMdbLoadFromDb::svc(const char* psTSName)
    {
        int iRet = 0;
        CHECK_OBJ(psTSName);
        //取得DSN管理信息
        m_pShmDSN = TMdbShmMgr::GetShmDSN(m_pConfig->GetDSN()->sName);
        CHECK_OBJ(m_pShmDSN);
        TMdbDSN* pDsn = m_pShmDSN->GetInfo();
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
            TADD_ERROR(ERR_DB_NOT_CONNECTED,"ERROR_SQL=%s.\nERROR_MSG=%s\n",e.GetErrSql(), e.GetErrMsg());
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

        bool bRepSuccess = false;//从备机加载的数据是否成功
        TMdbShmRepMgr *pRepShm = NULL;
        char sFailedRoutingList[MAX_REP_ROUTING_LIST_LEN] = {0};
        if(m_pConfig->GetIsStartShardBackupRep())
        {
            pRepShm = new(std::nothrow) TMdbShmRepMgr(m_pConfig->GetDSN()->sName);
            CHECK_RET(pRepShm->Attach(), "Attach TMdbShmRep failed.");
            const char* pFailedRoutingList = pRepShm->GetFailedRoutingList();
            if (pFailedRoutingList[0] == '\0')
            {
                bRepSuccess = true;
            }
            else
            {
                SAFESTRCPY(sFailedRoutingList,sizeof(sFailedRoutingList),pFailedRoutingList);
            }
        }
        
        TADD_FLOW("TableSpace[%s] Load Begin.....", psTSName);
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {
            TMdbTable * pTable = &(*itor);
            if(pTable->sTableName[0] == 0){continue;}
            // 过滤系统表和表空间
            if(FilterTable(pTable, psTSName, pTable->sTableName) == false)
            {
                continue;
            }
            if (pTable->m_bShardBack && bRepSuccess)//已经从备机成功加载
            {
                continue;
            }
            //初始化改表的上载SQL和插入SQL
            CHECK_RET(dao->Init(m_pShmDSN, pTable, sFailedRoutingList),"dao init error.");
            if (0 == TMdbNtcStrFunc::StrNoCaseCmp(pTable->sTableName, SYS_DBA_SEQUENCE))
            {
                iRet = dao->LoadSequence(mdb,pDsn); //dba_sequence表上载
            }
            else
            {
                iRet = dao->Load(mdb);//用户表上载
            }
            if(iRet != 0)
            {
                TADD_ERROR(iRet,"Can't Load table=%s.",pTable->sTableName);
            }
        }
        SAFE_DELETE(dao);
        mdb->Disconnect();
        SAFE_DELETE(mdb);
        SAFE_DELETE(pRepShm);
        TADD_FLOW("Load TableSpace[%s] Finish.", psTSName);
        return 0;
    }



    /******************************************************************************
    * 函数名称  :  LoadAll()
    * 函数描述  :  一个个的把表数据上载上来
    * 输入      :  无
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  li.shugang
    *******************************************************************************/
    int TMdbLoadFromDb::LoadAll()
    {
        TADD_FUNC("Start.");
        //取得DSN管理信息
        TMdbDSN* pDsn = m_pShmDSN->GetInfo();
        if(pDsn == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Can't GetInfo().");
            return ERR_APP_INVALID_PARAM;
        }
        //取到内存中的地址


#ifdef WIN32
        svc(0,NULL);
#else
        m_vPthreadT = new vector<pthread_t>;
        pthread_t       tID;
        pthread_attr_t  thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);
        int iRet = pthread_attr_setstacksize(&thread_attr,1024*1024*m_pConfig->GetDSN()->iLoadThreadStackSize);
        if(iRet != 0)
        {
            TADD_ERROR(ERR_OS_SET_THREAD_ATTR, "Can't pthread_attr_setstacksize(), errno=[%d], errmsg=[%s].", errno, strerror(errno));
            return ERR_OS_SET_THREAD_ATTR;
        }
        TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            TMdbTableSpace *pTableSpace  = &(*itor);
            //如果这个表空间是文件存储的，并且采用的文件加载命令，则不处理
            if(pTableSpace->m_bFileStorage && pDsn->m_bLoadFromDisk && pDsn->m_bLoadFromDiskOK)
            {
                continue;
            }
            if(pTableSpace->sName[0] != 0)
            {
                TADD_FLOW(" TableSpace name = [%s].",pTableSpace->sName);
                //创建一个线程,把这个线程下的所有表都上载到内存
                struct PthreadParam *stParam = new(std::nothrow) PthreadParam();
                if(stParam == NULL)
                {
                    TADD_ERROR(ERR_OS_NO_MEMROY, "TMdbLoadFromDb::LoadAll() Mem Not Enough");
                    return ERR_OS_NO_MEMROY;
                }
                stParam->tmdbLoadfromOra = this;
                SAFESTRCPY(stParam->m_sTSName, sizeof(stParam->m_sTSName), pTableSpace->sName);
                if(0 != pthread_create(&tID, &thread_attr, agent, stParam) )
                {
                    TADD_ERROR(ERR_OS_CREATE_THREAD,"TMdbLoadFromDb::LoadAll : Can't pthread_create(), errno=[%d], errmsg=[%s].",
                               errno, strerror(errno));
                    return ERR_OS_CREATE_THREAD;
                }
                m_vPthreadT->push_back(tID);
            }
        }
        //主线程等待子线程的退出
        for(size_t i = 0; i< m_vPthreadT->size(); i++)
        {
            if ( pthread_join(m_vPthreadT->at(i), NULL) )
            {
                // 等待term线程结束
                TADD_ERROR(ERR_OS_WAIT_THREAD," Wait child thread error, errno=[%d], errmsg=[%s].",  errno, strerror(errno));
                return ERR_OS_WAIT_THREAD;
            }
        }
        delete m_vPthreadT;
        pthread_attr_destroy(&thread_attr);
#endif
        TADD_FUNC("Finish.");
        return 0;
    }


    /******************************************************************************
    * 函数名称	:  LoadTable()
    * 函数描述	:  把一个表的数据上载上来
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbLoadFromDb::LoadTable(TMdbDatabase* pMdb, TMdbTable* pTableIN,bool bIsNeedLoad)
    {
        TADD_FUNC("TMdbLoadFromDb::LoadTable() : Start.");
        int iRet = 0;
        //取得DSN管理信息
        TMdbDSN* pDsn = m_pShmDSN->GetInfo();
        if(pDsn == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Can't GetInfo().");
            return ERR_APP_INVALID_PARAM;
        }
        TMdbDAOLoad dao(m_pConfig);
        if(dao.Connect() == false)
        {
            TADD_ERROR(ERR_APP_CONNCET_ORACLE_FAILED,"[%s : %d] : TMdbLoadFromDb::LoadTable() : Can't dao.Connect()().", __FILE__, __LINE__);
            return ERR_APP_CONNCET_ORACLE_FAILED;
        }
        TMdbTable * pTable = m_pShmDSN->GetTableByName(pTableIN->sTableName);
        if(NULL != pTable)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"table name[%s] is exist.",pTableIN->sTableName);
            return ERR_APP_INVALID_PARAM;
        }
        CHECK_RET(m_pShmDSN->AddNewTable(pTable,pTableIN),"AddNewTable failed.");
        TADD_NORMAL("Table=[%s].", pTable->sTableName);
        //分配表的基础索引和冲突索引
        TMdbIndexCtrl tIndexCtrl;
        tIndexCtrl.AttachDsn(m_pShmDSN);
        CHECK_RET(tIndexCtrl.AddTableIndex(pTable,m_pConfig->GetDSN()->iDataSize),
                                " Can't InitIndexMem table=%s.",pTable->sTableName);

        char sHead[8];
        memset(sHead, 0, sizeof(sHead));
        strncpy(sHead, pTable->sTableName, 4);
        if(TMdbNtcStrFunc::StrNoCaseCmp(sHead, "dba_") != 0 && bIsNeedLoad == true)
        {
            CHECK_RET(dao.Init(m_pShmDSN, pTable),"dao init error.");
            iRet = dao.Load(pMdb);
            if(iRet != 0)
            {
                TADD_ERROR(iRet,"TMdbLoadFromDb::LoadTable() : Can't Load table=%s.",
                           pTable->sTableName);
                return iRet;
            }
        }
        TADD_NORMAL("Load Table=[%s] OK.", pTable->sTableName);
        pTable->Print();
        TADD_FUNC("TMdbLoadFromDb::LoadTable() : Finish.");
        return iRet;
    }



    bool TMdbLoadFromDb::FilterTable(TMdbTable* pTable, const char* psTSName, const char* psTabName)
    {
        if(NULL == psTSName ||NULL == psTabName )
        {
            return false;
        }
        TMdbTable* pTmp = m_pConfig->GetTableByName(psTabName);
        if(pTmp == NULL )
        {
            return false;
        }
#ifndef WIN32
        if(0 != TMdbNtcStrFunc::StrNoCaseCmp(pTmp->m_sTableSpace, psTSName)  )
        {
            return false;
        }
#endif
		//除sequence之外的系统表不需要从DATABASE加载
        if(pTmp->bIsSysTab)
        {
            return false;
        }
        /*
        if(pTmp->bIsSysTab && 0 != TMdbNtcStrFunc::StrNoCaseCmp(pTmp->sTableName, SYS_DBA_SEQUENCE))
        {
            return false;
        }
        */

        //if(pTable->bIsNeedLoadFromOra == false)
        //{
        //    //主备同步已经加载过，无需再加载
        //    return false;
        //}

        //判断是否需要从Oracle上载
        if(pTable->iRepAttr == REP_TO_DB)
        {
            return true;
        }
        if(pTable->iRepAttr == REP_FROM_DB && m_pConfig->IsNotLoadFromDB(pTable->sTableName) == false)
        {//没有配置 不从数据库加载
            return true;
        }
        
		
		//sequence表需要加载
		if(0 == TMdbNtcStrFunc::StrNoCaseCmp(pTmp->sTableName, SYS_DBA_SEQUENCE))
        {
            return true;
        }
		
        return false;
    }
    TMdbReLoadFromOra::TMdbReLoadFromOra()
    {
        m_pShmDSN = NULL;
    }


    TMdbReLoadFromOra::~TMdbReLoadFromOra()
    {
    }

    int TMdbReLoadFromOra::Init(char* pszDsn)
    {
        TADD_FUNC("TMdbReLoadFromOra::Init() : Start.");
        int iRet = m_tConfig.Init();
        if(iRet < 0 )
        {
            TADD_ERROR(ERROR_UNKNOWN,"TMdbReLoadFromOra::Init(%s) : Init config failed.", pszDsn);
            return -1;
        }

        //上载配置, 配置具有随着内存数据库的变更, 动态扩张的特性
        iRet = m_tConfig.LoadCfg(pszDsn);
        if(iRet < 0 )
        {
            TADD_ERROR(ERROR_UNKNOWN,"TMdbReLoadFromOra::Init(%s) : Can't load config dsn=[%s].", pszDsn, pszDsn);
            return -1;
        }

        //检测参数
        if(m_tConfig.GetDSN() == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"TMdbReLoadFromOra::Init() : pConfig=NULL.");
            return ERR_APP_INVALID_PARAM;
        }

        m_pShmDSN = TMdbShmMgr::GetShmDSN(m_tConfig.GetDSN()->sName);
        if(m_pShmDSN == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"TMdbReLoadFromOra::SetDSN() : pszDSN=%s.", m_tConfig.GetDSN()->sName);
            return ERR_APP_INVALID_PARAM;
        }
        TADD_FUNC("TMdbReLoadFromOra::Init() : Finish.");

        return 0;
    }



    bool TMdbReLoadFromOra::FilterTable(TMdbTable* pTable, const char* psTabName)
    {
        TMdbTable* pTmp = m_tConfig.GetTableByName(psTabName);
        if(pTmp == NULL )
        {
            return false;
        }

        if(pTmp->bIsSysTab)
        {
            return false;
        }

        //判断是否需要从Oracle上载

        if(pTable->iRepAttr == REP_NO_REP)
        {
            return false;
        }
        return true;
    }

    int TMdbReLoadFromOra::LoadAll(const char* pszTable, const char * pszFilterSql)
    {
        TADD_FUNC("begin.");

        int iRet = 0;
        TMdbDSN* pDsn = m_pShmDSN->GetInfo();
        if(pDsn == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"TMdbReLoadFromOra::LoadAll : Can't GetInfo().");
            return ERR_APP_INVALID_PARAM;
        }

        TMdbDatabase *mdb = new(std::nothrow) TMdbDatabase();
        if(mdb == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"TMdbReLoadFromOra::LoadAll Mem Not Enough");
            return ERR_OS_NO_MEMROY;
        }

        TMdbProcCtrl tProCtrl;
        CHECK_RET(tProCtrl.Init(pDsn->sName),"tProCtrl.Init(%s)",pDsn->sName);
        if(tProCtrl.IsMonitorStart() == false)
        {
            CHECK_RET(ERR_DB_NOT_CONNECTED,"QuickMDB is not running.");
        }

        try
        {
            if(mdb->ConnectAsMgr(m_tConfig.GetDSN()->sName) == false)
            {
                TADD_ERROR(ERR_DB_NOT_CONNECTED, "TMdbReLoadFromOra::LoadAll(%s) fail", m_tConfig.GetDSN()->sName);
                mdb->Disconnect();
                SAFE_DELETE(mdb);
                return ERR_DB_NOT_CONNECTED;
            }
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(ERR_DB_NOT_CONNECTED,"TMdbReLoadFromOra::LoadAll(%s) : ERROR_SQL=%s.\nERROR_MSG=%s\n",
                       m_tConfig.GetDSN()->sName, e.GetErrSql(), e.GetErrMsg());
            mdb->Disconnect();
            SAFE_DELETE(mdb);
            return ERR_DB_NOT_CONNECTED;
        }

        TMdbDAOLoad *dao = new(std::nothrow) TMdbDAOLoad(&m_tConfig);
        if(dao == NULL)
        {
            TADD_ERROR(ERR_OS_NO_MEMROY,"TMdbReLoadFromOra::LoadAll() Mem Not Enough");
            mdb->Disconnect();
            SAFE_DELETE(mdb);
            return ERR_OS_NO_MEMROY;
        }

        if(dao->Connect() == false)
        {
            TADD_ERROR(ERR_APP_CONNCET_ORACLE_FAILED,"TMdbReLoadFromOra::LoadAll() : Can't dao.Connect()().");
            SAFE_DELETE(dao);
            mdb->Disconnect();
            SAFE_DELETE(mdb);
            return ERR_APP_CONNCET_ORACLE_FAILED;
        }

        if(TMdbNtcStrFunc::StrNoCaseCmp(pszTable,"all") == 0)
        {
            TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
            for(;itor != m_pShmDSN->m_TableList.end();++itor)
            {
                TMdbTable * pTable = &(*itor);
                if(pTable->sTableName[0]== 0){continue;}
                // 过滤系统表和表空间
                if(FilterTable(pTable,pTable->sTableName) == false)
                {
                    continue;
                }

                //初始化改表的上载SQL和插入SQL
                CHECK_RET(dao->Init(m_pShmDSN, pTable),"dao init error.");
                //用户表上载
                TADD_DETAIL("ReLoad table[%s] Start.",pTable->sTableName);
                iRet = dao->ReLoad(mdb);
                if(iRet != 0)
                {
                    TADD_ERROR(iRet,"Can't Load table=%s.",pTable->sTableName);
                    SAFE_DELETE(dao);
                    mdb->Disconnect();
                    SAFE_DELETE(mdb);
                    return iRet;
                }
                TADD_NORMAL("ReLoad Table[%s] OK.", pTable->sTableName);
            }
        }
        else
        {
            TMdbTable * pTable = m_pShmDSN->GetTableByName(pszTable);
			
			if(pTable == NULL)
			{
				iRet = ERR_TAB_NO_TABLE;
				TADD_ERROR(iRet,"Table[%s] does not exist in the shared memory.",pszTable);
				SAFE_DELETE(dao);
                mdb->Disconnect();
                SAFE_DELETE(mdb);
				return iRet;
			}
			
            if(FilterTable(pTable,pTable->sTableName) == true)
            {
                //初始化改表的上载SQL和插入SQL
                dao->Init(m_pShmDSN, pTable,NULL,pszFilterSql);

                //用户表上载
                TADD_DETAIL("ReLoad table[%s] Start.",pTable->sTableName);
                iRet = dao->ReLoad(mdb);
                if(iRet != 0)
                {
                    TADD_ERROR(iRet,"Can't Load table=%s.",pTable->sTableName);
                    SAFE_DELETE(dao);
                    mdb->Disconnect();
                    SAFE_DELETE(mdb);
                    return iRet;
                }
                TADD_NORMAL("ReLoad Table=[%s] OK.",pTable->sTableName);
            }    
        }
        SAFE_DELETE(dao);
        mdb->Disconnect();
        SAFE_DELETE(mdb);
        TADD_FUNC("END.");
        return 0;
    }


//}
