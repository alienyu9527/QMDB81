#include "Control/mdbTableInfoCtrl.h"
#include "Dbflush/mdbLoadFromDb.h"
#include "Control/mdbDDLExecuteEngine.h"




//namespace QuickMDB{


#define _TRY_CATCH_BEGIN_ try{

#define _TRY_CATCH_END_ }\
catch(TMdbException& e)\
{\
    TADD_ERROR(ERROR_UNKNOWN,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());\
    iRet = ERROR_UNKNOWN;\
}\
catch(...)\
{\
    TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");\
    iRet = ERROR_UNKNOWN;\
}

    TMdbTableInfoCtrl::TMdbTableInfoCtrl():
    m_pConfig(NULL),
    m_pShmDSN(NULL),
    m_pDsn(NULL),
    m_pTable(NULL)
    {

    }
    TMdbTableInfoCtrl::~TMdbTableInfoCtrl()
    {

    }

    //校验是否需要从oracle 上载
    bool TMdbTableInfoCtrl::IsNeedLoadFromOra(TMdbTable * pTable)
    {
    	//判断是否需要从Oracle上载
        bool bFlag = false;
        for(int i=0; i<pTable->iColumnCounts; ++i)
    	{
    		if(pTable->iRepAttr == REP_FROM_DB || pTable->iRepAttr == REP_TO_DB)
    		{
    		    bFlag = true;
    			break;
    		}	
    	}
        return bFlag;
    }

    /******************************************************************************
    * 函数名称	:  CreateNewTable()
    * 函数描述	:  创建新table;
    * 输入		:  pTableName, 表名
    * 输出		:  无
    * 返回值	:  成功返回0, 失败返回错误码
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableInfoCtrl::CreateNewTable(const char * pTableName)
    {	
    	int iRet = 0;
    	if(!IsValidNewTable(pTableName))
    	{
    		CHECK_RET(-1,"table[%s] is not valid....",pTableName);
    	}
    	TMdbTableSpace * pTableSapce = m_pShmDSN->GetTableSpaceAddrByName(m_pTable->m_sTableSpace);
    	if(NULL == pTableSapce)
    	{
    	    //该表对应的表空间不存在需要创建一个
    		pTableSapce = m_pConfig->GetTableSpaceByName(m_pTable->m_sTableSpace);
    		CHECK_OBJ(pTableSapce);
    		CHECK_RET(m_mdbTSCtrl.CreateTableSpace(pTableSapce,m_pConfig,true),"CreateTableSpace failed...");
    		//CHECK_RET(InsertSysTableSpace(pTableSapce),"InsertSysTableSpace[%s] failed",pTableSapce->sName);
    	}
        //上载数据
        iRet = CreateToRollBack();
    	if(iRet != 0)
    	{
    		TADD_NORMAL("Create Table[%s] failed. Drop Table",pTableName);
    		if(DropTable(pTableName)!=0)
    		{
    			TADD_ERROR(ERROR_UNKNOWN,"Drop Table[%s] failed.",pTableName);
    			return -1;
    		}
    	}
    	return iRet;
    }

    //创建表失败的情况下可以回滚
    int TMdbTableInfoCtrl::CreateToRollBack()
    {
    	int iRet = 0;
    	//创建功能内存表
        TMdbTable * pTable = m_pShmDSN->GetTableByName(m_pTable->sTableName);
        if(NULL != pTable)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Table[%s] already exists.",pTable->sTableName);
            return ERR_APP_INVALID_PARAM;
        }
        CHECK_RET(m_pShmDSN->AddNewTable(pTable,m_pTable),"AddNewTable failed.");
        //分配表的基础索引和冲突索引
        TMdbIndexCtrl tIndexCtrl;
        tIndexCtrl.AttachDsn(m_pShmDSN);
        CHECK_RET(tIndexCtrl.AddTableIndex(pTable,m_pConfig->GetDSN()->iDataSize),
                                " Can't InitIndexMem table=%s.",pTable->sTableName);

        //上载数据
        TMdbDDLExecuteEngine tDDLexecuteEngine;
        tDDLexecuteEngine.SetDB(m_pShmDSN,m_pConfig);
        CHECK_RET(tDDLexecuteEngine.LoadDataFromRepOrOracle(pTable->sTableName),\
                "Upload table[%s] data fails.",pTable->sTableName);
    	return iRet;
    }

    /******************************************************************************
    * 函数名称	:  DropTable()
    * 函数描述	:  删除表
    * 输入		:  pTableName, 表名
    * 输出		:  无
    * 返回值	:  成功返回0, 失败返回错误码
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableInfoCtrl::DropTable(const char * pTableName, bool bCheckpoint )
    {
    	int iRet = 0;
        if(strlen(pTableName) > 4)
        {
            char sHead[8] = {0};
        	strncpy(sHead,pTableName,4);
        	if(TMdbNtcStrFunc::StrNoCaseCmp(sHead, "dba_") == 0)
        	{//系统表不可以 删除
        		CHECK_RET(-1,"system table can`t be droped....");
        	}
        }
    	
    	m_pTable = m_pShmDSN->GetTableByName(pTableName);
    	if(NULL == m_pTable)
    	{
    		CHECK_RET(-1,"table[%s] is not exist....",pTableName);
    	}
		char * sTableSpace = new(std::nothrow) char[MAX_NAME_LEN];
		if(sTableSpace ==  NULL)
		{
			TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new sTableSpace");
			return ERR_OS_NO_MEMROY;

		}
		memset(sTableSpace, 0, MAX_NAME_LEN);
		if(m_pTable->m_sTableSpace == NULL)
		{
			SAFE_DELETE(sTableSpace);
			sTableSpace = NULL;
		}
		else
		{
			strncpy(sTableSpace, m_pTable->m_sTableSpace, MAX_NAME_LEN);
		}
        CHECK_RET(m_mdbTSCtrl.Init(m_pDsn->sName,sTableSpace),"m_mdbTSCtrl.Init failed");
        //删除数据
        CHECK_RET(DeleteData(m_pTable),"Can't DeleteData");
    	//删除表管理的页面,归还表空间
    	CHECK_RET(DeletePage(m_pTable)," Can't DeletePage().");
    	//删除数据对应的所有索引
    	CHECK_RET(DeleteIndex(m_pTable),"Can't DeleteIndex()");	
    	//删除表头信息
    	m_pTable->Clear();

        if(bCheckpoint && m_mdbTSCtrl.IsFileStorage())
        {
            TMdbCheckPoint tTMdbCheckPoint;
            CHECK_RET(tTMdbCheckPoint.Init(m_pShmDSN->GetInfo()->sName),"Init failed.");
            //CHECK_RET(tTMdbCheckPoint.LinkFile(sTableSpace),"Attach failed.");
           // CHECK_RET(tTMdbCheckPoint.DoCheckPoint(sTableSpace),"FlushOneTable falied");
            CHECK_RET(tTMdbCheckPoint.LinkFile(),"Attach failed.");
            CHECK_RET(tTMdbCheckPoint.DoCheckPoint(),"FlushOneTable falied");
        }
        SAFE_DELETE(sTableSpace);
    	TADD_FUNC("Finish.");
    	return iRet;
    }
	
	/******************************************************************************
	* 函数名称	:  TruncateTable()
	* 函数描述	:  Truncate表
	* 输入		:  pTableName, 表名
	* 输出		:  无
	* 返回值	:  成功返回0, 失败返回错误码
	* 作者		:  jin.shaohua
	*******************************************************************************/
	int TMdbTableInfoCtrl::TruncateTable(const char * pTableName)
	{
		TADD_FUNC("Start.");
		int iRet = 0;
        if(strlen(pTableName) > 4)
        {
            char sHead[8] = {0};
        	strncpy(sHead,pTableName,strlen("dba_"));
        	if(TMdbNtcStrFunc::StrNoCaseCmp(sHead, "dba_") == 0)
        	{//系统表不可以 删除
        		CHECK_RET(-1,"system table can`t be droped....");
        	}
        }
    	
    	m_pTable = m_pShmDSN->GetTableByName(pTableName);
    	if(NULL == m_pTable)
    	{
    		CHECK_RET(-1,"table[%s] is not exist....",pTableName);
    	}
        CHECK_RET(m_mdbTSCtrl.Init(m_pDsn->sName,m_pTable->m_sTableSpace),"m_mdbTSCtrl.Init failed");
		CHECK_RET(TruncateIndex(m_pTable),"Can't truncate index");
		CHECK_RET(TruncateData(m_pTable),"Can't truncate data");
		
		TMdbCheckPoint tMdbCheckPoint;
	    CHECK_RET(tMdbCheckPoint.Init(m_pDsn->sName),"Init failed.");
		CHECK_RET(tMdbCheckPoint.LinkFile(),"Attach failed.");
		CHECK_RET(tMdbCheckPoint.DoCheckPoint(),"DoCheckPoint Faild.");
		
		TADD_FUNC("Finish.");
		return iRet;
	}
	
    /******************************************************************************
    * 函数名称	:  SetDSN()
    * 函数描述	:  设置DSN信息    
    * 输入		:  pszDSN, 管理区所属的DSN 
    * 输出		:  无
    * 返回值	:  成功返回0, 失败返回-1
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbTableInfoCtrl::SetDSN(const char* pszDSN)
    {
        TADD_FUNC("Begin."); 
    	int iRet = 0;
        m_pConfig = TMdbConfigMgr::GetMdbConfig(pszDSN);
        CHECK_OBJ(m_pConfig);
        
        m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN); //连接上共享内存
        if(NULL == m_pShmDSN)
        {
    		CHECK_RET(-1,"QMDB may be not started...");
        }
        CHECK_RET(m_pShmDSN->Attach(pszDSN, *m_pConfig),"Attach [%s] failed. Parameters are invalid.",pszDSN);
        m_pDsn = m_pShmDSN->GetInfo();
        _TRY_CATCH_BEGIN_
            if(m_tMDB.ConnectAsMgr(pszDSN) == false)
            {
                TADD_ERROR(-1,"Can't Connect [%s]", pszDSN);
                return -1;
            } 
        _TRY_CATCH_END_
    	TADD_FUNC("Finish."); 		
    	return iRet;
    }


    //是否合法:表还未存在,而且不属于dba_开头的系统表
    bool TMdbTableInfoCtrl::IsValidNewTable(const char * pTableName)
    {
        TADD_FUNC("Begin."); 
        if(strlen(pTableName) > 4)
        {
            //看看是否为系统表
        	char sHead[32] = {0};
        	memset(sHead, 0, sizeof(sHead));
        	strncpy(sHead, pTableName, strlen("dba_"));
        	if(TMdbNtcStrFunc::StrNoCaseCmp(sHead, "dba_") == 0)
        	{//判断是否是系统表
        		TADD_ERROR(-1,"system-table=[%s] is invalid.",pTableName);
                return false; 	
        	}
        }
    	if(m_pShmDSN->GetTableByName(pTableName) != NULL)
    	{//判断该表是否已存在
    		TADD_ERROR(-1,"Table=[%s] exist.",pTableName);
            TADD_FUNC("Finish."); 	
    		return false;
    	}
        //获取表配置,在xml中不存在
    	m_pTable = m_pConfig->GetTable(pTableName);
        if(NULL == m_pTable)
        {
        	TADD_ERROR(-1,"Not find table-config=[%s].",pTableName);	
            TADD_FUNC("Finish."); 	
    		return false;	
        }   	
        TADD_FUNC("Finish."); 	
    	return true;	
    }


    //删除表管理的页面,归还表空间
    int TMdbTableInfoCtrl::DeletePage(TMdbTable * pTable)
    {
    	TADD_FUNC("Begin.");	
    	int iRet = 0;
    	//归还fullPage
    	int iPageID = pTable->iFullPageID;
    	TMdbPage* pPage = NULL;
    	while(iPageID > 0)
    	{
    		pPage = (TMdbPage*)m_mdbTSCtrl.GetAddrByPageID(iPageID);
    		iPageID = pPage->m_iNextPageID;
    		m_mdbTSCtrl.PushBackPage(pPage);//回收
    		pTable->iFullPages--;
    	}
    	//归还freepage
    	iPageID = pTable->iFreePageID;
    	while(iPageID > 0)
    	{
            pPage = (TMdbPage*)m_mdbTSCtrl.GetAddrByPageID(iPageID);
            
            CHECK_RET(pTable->tFreeMutex.Lock(true, &m_pDsn->tCurTime),"[%s].tFreeMutex.Lock() failed.",pTable->sTableName);
            m_mdbTSCtrl.RemovePageFromCircle(pPage,pTable->iFreePageID);
            pTable->iFreePages--;
            pTable->tFreeMutex.UnLock(true, m_pDsn->sCurTime);
            
            iPageID = pTable->iFreePageID;
            m_mdbTSCtrl.PushBackPage(pPage);//回收
    		
    	}
           TADD_NORMAL_TO_CLI(FMT_CLI_OK,"DeletePage FreePages[%d],FullPages[%d]",pTable->iFreePages,pTable->iFullPages);
    	TADD_FUNC("Finish.");
    	return iRet;		
    }

        
    //删除数据对应的所有索引
    int TMdbTableInfoCtrl::DeleteIndex(TMdbTable *pTable)
    {
    	int iRet = 0;
    	TADD_NORMAL("Total index counts=%d", pTable->iIndexCounts);
    	TMdbIndexCtrl tIndexCtrl;
        CHECK_RET(pTable->tTableMutex.Lock(true,&(m_pDsn->tCurTime)),"Lock Faild");
    	//pTable->tTableMutex.Lock(true,&(m_pDsn->tCurTime));
    	CHECK_RET(tIndexCtrl.DeleteTableIndex(m_pShmDSN,pTable),"DeleteTableIndex failed...");
    	//pTable->tTableMutex.UnLock(true);
        CHECK_RET(pTable->tTableMutex.UnLock(true),"UnLock Faild");
           TADD_NORMAL_TO_CLI(FMT_CLI_OK,"DeleteIndex");
    	return iRet;	
    }
	
    //删除数据
    int TMdbTableInfoCtrl::DeleteData(TMdbTable * pTable)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
        int i = 0;
        TADD_NORMAL("Try to Clear Data.");
        //拼接遍历SQL和删除SQL
        char sSelColumns[MAX_SQL_LEN] = {0};
        char sDelWhere[MAX_SQL_LEN] = {0};
        for(i = 0;i< pTable->m_tPriKey.iColumnCounts;++i)
        {
            int iPkPos = pTable->m_tPriKey.iColumnNo[i];
            if(iPkPos >= 0)
            {
                sprintf(sSelColumns+strlen(sSelColumns),"%s,",pTable->tColumn[iPkPos].sName);
                sprintf(sDelWhere+strlen(sDelWhere),"%s=:%s and ",pTable->tColumn[iPkPos].sName,pTable->tColumn[iPkPos].sName);
            }
        }
        if(0 == strlen(sSelColumns))
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"table[%s] has no pk!!!!",pTable->sTableName);
        }
        sSelColumns[strlen(sSelColumns) - 1] = '\0';
        sDelWhere[strlen(sDelWhere) - 4] = '\0';

        char sQueryAll[MAX_SQL_LEN] = {0};
        char sDelete[MAX_SQL_LEN] = {0};
        sprintf(sQueryAll,"select %s from %s;",sSelColumns,pTable->sTableName);
        sprintf(sDelete,"delete from %s where %s",pTable->sTableName,sDelWhere);
        TADD_NORMAL("Query SQL=[%s],Delete SQL=[%s]",sQueryAll,sDelete);
        TMdbDatabase * pDB = NULL;
        TMdbQuery * pQueryAll = NULL;
        TMdbQuery * pDelete    = NULL;
        _TRY_CATCH_BEGIN_
        pDB = new TMdbDatabase();
        if(false == pDB->ConnectAsMgr(m_pDsn->sName))
        {
            CHECK_RET(ERR_DB_NOT_CONNECTED,"connect mdb[%s] failed.",m_pDsn->sName);
        }
        pQueryAll = pDB->CreateDBQuery();
        pDelete    = pDB->CreateDBQuery();
        pQueryAll->SetSQL(sQueryAll);
        pDelete->SetSQL(sDelete,QUERY_NO_ORAFLUSH|QUERY_NO_SHARDFLUSH|QUERY_NO_ROLLBACK|QUERY_NO_REDOFLUSH,0);
        do{
            pQueryAll->Open();
            if(false == pQueryAll->Next())
            {
                break;//没有记录
            }
            else
            {//有记录需要删除
                TADD_NORMAL("Delete data now,do not insert.....");
                int iCount = 0;
                do
                {
                    int i = 0;
                    for(i = 0; i < pQueryAll->FieldCount();++i)
                    {
                        pDelete->SetParameter(i,pQueryAll->Field(i).AsString());
                    }
                    pDelete->Execute();
                    pDelete->Commit();
                    iCount += pDelete->RowsAffected();
                }while(pQueryAll->Next());
                TADD_NORMAL("Delete data counts=[%d]",iCount);
            }
        }while(1);
        _TRY_CATCH_END_
        SAFE_DELETE(pQueryAll);
        SAFE_DELETE(pDelete);
        SAFE_DELETE(pDB);
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Delete data.");
        return iRet;
    }

	//清空数据
    int TMdbTableInfoCtrl::TruncateIndex(TMdbTable * pTable)
    {
        int iRet = 0;
        CHECK_OBJ(pTable);
		TADD_NORMAL("Try to Clear Index. Total index counts=%d", pTable->iIndexCounts);
    	TMdbIndexCtrl tIndexCtrl;
        CHECK_RET(pTable->tTableMutex.Lock(true,&(m_pDsn->tCurTime)),"Lock Faild");
    	CHECK_RET(tIndexCtrl.TruncateTableIndex(m_pShmDSN,pTable),"TruncateTableIndex failed...");
        CHECK_RET(pTable->tTableMutex.UnLock(true),"UnLock Faild");
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"TruncateIndex");

        return iRet;
    }

	//清空数据
    int TMdbTableInfoCtrl::TruncateData(TMdbTable * pTable)
    {
        int iRet = 0;
		
        CHECK_OBJ(pTable);
        TADD_NORMAL("Try to Clear Data.");
		int iAffect = 0;
		
    	pTable->tTableMutex.Lock(true,&(m_pDsn->tCurTime));
		int varcharPos[MAX_COLUMN_COUNTS] = {-1};
		int varcharCount = 0;
		for (int i = 0; i < pTable->iColumnCounts; i++)
		{
            if(DT_VarChar == pTable->tColumn[i].iDataType || DT_Blob == pTable->tColumn[i].iDataType)
            {
            	varcharPos[varcharCount++] = i;
            }
		}
		
		TMdbPageCtrl pageCtrl;
		pageCtrl.SetDSN(m_pDsn->sName);
		TMdbVarCharCtrl varcharCtrl;
		varcharCtrl.Init(m_pDsn->sName);
		TMdbRowCtrl rowCtrl;
		rowCtrl.Init(m_pDsn->sName, pTable);
		int iPageId = 0;
        TMdbPage * pPage = NULL;
		int iWhichPos = -1;
		unsigned int iRowId = 0;
		char * pDataAddr = NULL;	
		char* pNextDataAddr = NULL;
		int iDataOffset = 0;
		//回收fullpage链及相关varchar数据
		iPageId = pTable->iFullPageID;
        while(iPageId > 0)
        {
            pPage = (TMdbPage * )m_mdbTSCtrl.GetAddrByPageID(iPageId);
            if(NULL == pPage)
            {
                break;
            }
			if(pPage->m_iRecordCounts != 0)
			{
				pDataAddr = NULL;
				iDataOffset = 0;
				while(pPage->GetNextDataAddr(pDataAddr, iDataOffset, pNextDataAddr) != NULL)
				{
					for(int i = 0; i < varcharCount; i++)
					{
						iWhichPos = -1;
						iRowId = 0;
						varcharCtrl.GetStoragePos(pDataAddr + pTable->tColumn[varcharPos[i]].iOffSet, iWhichPos, iRowId);
		                if(iWhichPos < VC_16 || iWhichPos > VC_8192)
		                {
		                    continue;
		                }
						CHECK_RET(varcharCtrl.Delete(iWhichPos, iRowId),"Delete Varchar Faild");
					}
					iAffect++;
				}
			}
			//
			m_mdbTSCtrl.SetPageDirtyFlag(iPageId);
            iPageId = pPage->m_iNextPageID;
			pTable->iFullPageID = iPageId;
    		m_mdbTSCtrl.PushBackPage(pPage);//回收
    		pTable->iFullPages--;
        }

		//回收freepage链及相关varchar数据
		iPageId = pTable->iFreePageID;
    	while(iPageId > 0)
    	{
            CHECK_RET(pTable->tFreeMutex.Lock(true, &m_pDsn->tCurTime),"[%s].tFreeMutex.Lock() failed.",pTable->sTableName);
            pPage = (TMdbPage * )m_mdbTSCtrl.GetAddrByPageID(iPageId);
            if(NULL == pPage)
            {
                //return -1;
            }
			if(pPage->m_iRecordCounts == 0)
			{
				//continue;
			}
            else
			{
				pDataAddr = NULL;
				iDataOffset = 0;
				while(pPage->GetNextDataAddr(pDataAddr, iDataOffset, pNextDataAddr) != NULL)
				{
					for(int i = 0; i < varcharCount; i++)
					{
						iWhichPos = -1;
						iRowId = 0;
						varcharCtrl.GetStoragePos(pDataAddr + pTable->tColumn[varcharPos[i]].iOffSet, iWhichPos, iRowId);
		                if(iWhichPos < VC_16 || iWhichPos > VC_8192)
		                {
		                    continue;
		                }
						CHECK_RET(varcharCtrl.Delete(iWhichPos, iRowId),"Delete Varchar Faild");
					}
					iAffect++;
				}
			}
			m_mdbTSCtrl.SetPageDirtyFlag(iPageId);
            m_mdbTSCtrl.RemovePageFromCircle(pPage,pTable->iFreePageID);
            pTable->iFreePages--;
            pTable->tFreeMutex.UnLock(true, m_pDsn->sCurTime);
            iPageId = pTable->iFreePageID;
            m_mdbTSCtrl.PushBackPage(pPage);//回收
    	}
        pTable->iCounts = 0; //记录数清零
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Truncate data.iAffect=%d.",iAffect);
    	pTable->tTableMutex.UnLock(true);
        return iRet;
    }
//}

