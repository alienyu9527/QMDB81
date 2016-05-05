/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        TMdbLoadFromDb.h
*@Description�� �����Oracle�������ݣ�
*@Author:       li.shugang
*@Date��        2008��12��15��
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
    * ��������  :  Init()
    * ��������  :  ���������ļ������ռ�
    * ����      :  ��
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻�-1
    * ����      :  li.shugang
    *******************************************************************************/
    int TMdbLoadFromDb::Init(TMdbConfig *pConfig)
    {
        TADD_FUNC("Start.");
        //������
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
        //ȡ��DSN������Ϣ
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
        //ȡ���ڴ��еĵ�ַ
       
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

        bool bRepSuccess = false;//�ӱ������ص������Ƿ�ɹ�
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
            // ����ϵͳ��ͱ�ռ�
            if(FilterTable(pTable, psTSName, pTable->sTableName) == false)
            {
                continue;
            }
            if (pTable->m_bShardBack && bRepSuccess)//�Ѿ��ӱ����ɹ�����
            {
                continue;
            }
            //��ʼ���ı������SQL�Ͳ���SQL
            CHECK_RET(dao->Init(m_pShmDSN, pTable, sFailedRoutingList),"dao init error.");
            if (0 == TMdbNtcStrFunc::StrNoCaseCmp(pTable->sTableName, SYS_DBA_SEQUENCE))
            {
                iRet = dao->LoadSequence(mdb,pDsn); //dba_sequence������
            }
            else
            {
                iRet = dao->Load(mdb);//�û�������
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
    * ��������  :  LoadAll()
    * ��������  :  һ�����İѱ�������������
    * ����      :  ��
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻�-1
    * ����      :  li.shugang
    *******************************************************************************/
    int TMdbLoadFromDb::LoadAll()
    {
        TADD_FUNC("Start.");
        //ȡ��DSN������Ϣ
        TMdbDSN* pDsn = m_pShmDSN->GetInfo();
        if(pDsn == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Can't GetInfo().");
            return ERR_APP_INVALID_PARAM;
        }
        //ȡ���ڴ��еĵ�ַ


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
            //��������ռ����ļ��洢�ģ����Ҳ��õ��ļ���������򲻴���
            if(pTableSpace->m_bFileStorage && pDsn->m_bLoadFromDisk && pDsn->m_bLoadFromDiskOK)
            {
                continue;
            }
            if(pTableSpace->sName[0] != 0)
            {
                TADD_FLOW(" TableSpace name = [%s].",pTableSpace->sName);
                //����һ���߳�,������߳��µ����б����ص��ڴ�
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
        //���̵߳ȴ����̵߳��˳�
        for(size_t i = 0; i< m_vPthreadT->size(); i++)
        {
            if ( pthread_join(m_vPthreadT->at(i), NULL) )
            {
                // �ȴ�term�߳̽���
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
    * ��������	:  LoadTable()
    * ��������	:  ��һ�����������������
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbLoadFromDb::LoadTable(TMdbDatabase* pMdb, TMdbTable* pTableIN,bool bIsNeedLoad)
    {
        TADD_FUNC("TMdbLoadFromDb::LoadTable() : Start.");
        int iRet = 0;
        //ȡ��DSN������Ϣ
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
        //�����Ļ��������ͳ�ͻ����
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
		//��sequence֮���ϵͳ����Ҫ��DATABASE����
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
        //    //����ͬ���Ѿ����ع��������ټ���
        //    return false;
        //}

        //�ж��Ƿ���Ҫ��Oracle����
        if(pTable->iRepAttr == REP_TO_DB)
        {
            return true;
        }
        if(pTable->iRepAttr == REP_FROM_DB && m_pConfig->IsNotLoadFromDB(pTable->sTableName) == false)
        {//û������ �������ݿ����
            return true;
        }
        
		
		//sequence����Ҫ����
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

        //��������, ���þ��������ڴ����ݿ�ı��, ��̬���ŵ�����
        iRet = m_tConfig.LoadCfg(pszDsn);
        if(iRet < 0 )
        {
            TADD_ERROR(ERROR_UNKNOWN,"TMdbReLoadFromOra::Init(%s) : Can't load config dsn=[%s].", pszDsn, pszDsn);
            return -1;
        }

        //������
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

        //�ж��Ƿ���Ҫ��Oracle����

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
                // ����ϵͳ��ͱ�ռ�
                if(FilterTable(pTable,pTable->sTableName) == false)
                {
                    continue;
                }

                //��ʼ���ı������SQL�Ͳ���SQL
                CHECK_RET(dao->Init(m_pShmDSN, pTable),"dao init error.");
                //�û�������
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
                //��ʼ���ı������SQL�Ͳ���SQL
                dao->Init(m_pShmDSN, pTable,NULL,pszFilterSql);

                //�û�������
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
