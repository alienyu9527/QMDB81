/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbSysTableThread.h
*@Description： 负责管理时间信息
*@Author:		li.shugang
*@Date：	    2009年08月12日
*@History:
******************************************************************************************/
#include "Helper/mdbDateTime.h"
#include "Control/mdbSysTableThread.h"
#include <errno.h>

//namespace QuickMDB{

#define _MDB_TRY_CATCH_BEGIN_ try{

#define _MDB_TRY_CATCH_END_ }\
    catch(TMdbException& e)\
    {\
        TADD_ERROR(ERR_SQL_INVALID,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());\
        iRet = e.GetErrCode();\
    }\
    catch(...)\
    {\
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");\
        iRet = ERROR_UNKNOWN;\
    }


    TMdbSysTableSync::TMdbSysTableSync():
    m_pMDB(NULL),
    m_pShmDSN(NULL),
    m_pMdbConfig(NULL),
    m_pDsn(NULL)
    {
        int i = 0;
        for(i = 0; i < E_DBA_MAX; ++i)
        {
            int j = 0;
            for(j = 0; j<E_QUERY_MAX; ++j)
            {
                m_pIDUSQuery[i][j] = NULL;
            }
        }
        m_tTableVersion.clear();
        m_iIndexCounts.clear();

    }

    void TMdbSysTableSync::Clear()
    {
        m_tTableVersion.clear();
        m_iIndexCounts.clear();
        int i = 0;
        for(i = 0; i < E_DBA_MAX; ++i)
        {
            int j = 0;
            for(j = 0; j<E_QUERY_MAX; ++j)
            {
                SAFE_DELETE(m_pIDUSQuery[i][j]);
            }
        }
        SAFE_DELETE(m_pMDB);
    }

    TMdbSysTableSync::~TMdbSysTableSync()
    {
        
    }
    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	: 初始化
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::Init(const char * sDsn)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDSN);
        m_pDsn = m_pShmDSN->GetInfo();
        CHECK_OBJ(m_pDsn);
        m_pMdbConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
        CHECK_OBJ(m_pMdbConfig);
        Clear();
        m_pMDB = new (std::nothrow)TMdbDatabase();
        CHECK_OBJ(m_pMDB);
        _MDB_TRY_CATCH_BEGIN_
        if(false == m_pMDB->ConnectAsMgr(sDsn)){CHECK_RET(ERR_DB_NOT_CONNECTED,"can not connect db[%s]",sDsn);}
        int i = 0;
        for(i = 0; i < E_DBA_MAX; ++i)
        {
            int j = 0;
            for(j = 0; j<E_QUERY_MAX; ++j)
            {
                m_pIDUSQuery[i][j] = m_pMDB->CreateDBQuery();
            }
        }
        //dba_tables
        m_pIDUSQuery[E_DBA_TABLE][E_INSERT]->SetSQL("insert into dba_tables "
                "( table_name, table_space, record_set_counts, real_counts, "
                "stat, expand_record, read_lock, write_lock, column_counts, index_counts, "
                "primary_key, cin_counts, left_cin_nodes, full_pages, free_pages,shard_backup, storage_type) values ("
                ":table_name, :table_space, :record_set_counts, :real_counts, "
                ":stat, :expand_record, :read_lock, :write_lock, :column_counts, :index_counts, "
                ":primary_key, :cin_counts, :left_cin_nodes, :full_pages, :free_pages, :shard_backup, :storage_type)");
        m_pIDUSQuery[E_DBA_TABLE][E_DELETE]->SetSQL("delete from dba_tables where table_name = :table_name");
        m_pIDUSQuery[E_DBA_TABLE][E_UPDATE]->SetSQL("update dba_tables "
                "set real_counts=:real_counts, stat=:stat, cin_counts=:cin_counts, "
                "left_cin_nodes=:left_cin_nodes, full_pages=:full_pages, free_pages=:free_pages, "
                "read_lock=:read_lock,write_lock=:write_lock,column_counts=:column_counts,index_counts=:index_counts, shard_backup=:shard_backup, storage_type=:storage_type "
                "where table_name=:table_name");
        m_pIDUSQuery[E_DBA_TABLE][E_SELECT]->SetSQL("select table_name from dba_tables where table_name = :table_name");
        m_pIDUSQuery[E_DBA_TABLE][E_SELECT_ALL]->SetSQL("select table_name from dba_tables");
        //dba_tablespace
        m_pIDUSQuery[E_DBA_TABLESPACE][E_INSERT]->SetSQL( "insert into dba_table_space "
                "(table_space_name, page_size, ask_pages, total_pages, free_pages, file_storage) values ("
                ":table_space_name, :page_size, :ask_pages, :total_pages, :free_pages, :file_storage)");
        m_pIDUSQuery[E_DBA_TABLESPACE][E_UPDATE]->SetSQL("update dba_table_space "
                "set total_pages=:total_pages, free_pages=:free_pages "
                "where table_space_name=:table_space_name ");
        m_pIDUSQuery[E_DBA_TABLESPACE][E_SELECT]->SetSQL("select table_space_name from dba_table_space where table_space_name=:table_space_name");
        m_pIDUSQuery[E_DBA_TABLESPACE][E_DELETE]->SetSQL("delete from dba_table_space where table_space_name=:table_space_name");
        m_pIDUSQuery[E_DBA_TABLESPACE][E_SELECT_ALL]->SetSQL("select table_space_name from dba_table_space ");
        //dba_sequence
        m_pIDUSQuery[E_DBA_SEQUENCE][E_INSERT]->SetSQL("insert into dba_sequence "
                "(sequence_name, start_value, end_value, cur_value, step_value) values ("
                ":sequence_name, :start_value, :end_value, :cur_value, :step_value)");
        m_pIDUSQuery[E_DBA_SEQUENCE][E_UPDATE]->SetSQL( "update dba_sequence "
                "set start_value=:start_value,end_value=:end_value,cur_value=:cur_value,step_value=:step_value "
                "where sequence_name=:sequence_name ");
        m_pIDUSQuery[E_DBA_SEQUENCE][E_SELECT]->SetSQL("select cur_value from dba_sequence where sequence_name=:sequence_name");
        m_pIDUSQuery[E_DBA_SEQUENCE][E_SELECT_ALL]->SetSQL("select sequence_name from dba_sequence");
        m_pIDUSQuery[E_DBA_SEQUENCE][E_DELETE]->SetSQL("delete from dba_sequence where sequence_name=:sequence_name");

        //dba_session
        m_pIDUSQuery[E_DBA_SESSION][E_INSERT]->SetSQL("insert into dba_session "
                "(session_id, pid, tid, ip, Handle, start_time, state, log_level,sqlpos) "
                "values (:session_id, :pid, :tid, :ip, :Handle, :start_time, :state, :log_level,:sqlpos) ");
        m_pIDUSQuery[E_DBA_SESSION][E_DELETE]->SetSQL("delete from dba_session where session_id=:session_id ");
        m_pIDUSQuery[E_DBA_SESSION][E_UPDATE]->SetSQL("update dba_session "
                "set pid=:pid, tid=:tid, ip=:ip, Handle=:Handle, start_time=:start_time, "
                "state=:state, log_level=:log_level, sqlpos=:sqlpos "
                "where session_id=:session_id ");
        m_pIDUSQuery[E_DBA_SESSION][E_SELECT]->SetSQL("select pid from dba_session where session_id=:session_id ");
        //dba_resource
        m_pIDUSQuery[E_DBA_RESOURCE][E_INSERT]->SetSQL("insert into dba_resource "
                "(mem_key, mem_id, mem_type, mem_size, mem_left) "
                "values (:mem_key, :mem_id, :mem_type, :mem_size, :mem_left) ");
        m_pIDUSQuery[E_DBA_RESOURCE][E_UPDATE]->SetSQL( "update dba_resource "
                "set mem_id=:mem_id, mem_type=:mem_type, mem_size=:mem_size, mem_left=:mem_left "
                "where mem_key=:mem_key ");
        m_pIDUSQuery[E_DBA_RESOURCE][E_SELECT]->SetSQL("select mem_id from dba_resource where mem_key=:mem_key ");
        //dba_column
        m_pIDUSQuery[E_DBA_COLUMN][E_INSERT]->SetSQL("insert into dba_column "
                "(table_name, column_name, data_type, data_len, data_pos, rep_attr ,nullable) values "
                "(:table_name, :column_name, :data_type, :data_len, :data_pos, :rep_attr, :nullable)");
        m_pIDUSQuery[E_DBA_COLUMN][E_DELETE]->SetSQL("delete from dba_column where table_name = :table_name");
        m_pIDUSQuery[E_DBA_COLUMN][E_SELECT]->SetSQL("select table_name from dba_column where  table_name = :table_name");
        m_pIDUSQuery[E_DBA_COLUMN][E_SELECT_ALL]->SetSQL("select table_name,column_name from dba_column");
        m_pIDUSQuery[E_DBA_COLUMN][E_UPDATE]->SetSQL("update dba_column "
                "set data_type =:data_type,data_len=:data_len,rep_attr=:rep_attr, "
                "nullable = :nullable where table_name=:table_name and column_name=:column_name");
        //dba_index
        m_pIDUSQuery[E_DBA_INDEX][E_INSERT]->SetSQL("insert into dba_index "
                "(table_name, index_name, index_type, priority, data_pos, is_fix, Algorithm_Type) values ("
                ":table_name, :index_name, :index_type, :priority, :data_pos, :is_fix, :Algorithm_Type)");
        m_pIDUSQuery[E_DBA_INDEX][E_DELETE]->SetSQL("delete from dba_index where table_name = :table_name");
        m_pIDUSQuery[E_DBA_INDEX][E_SELECT]->SetSQL("select table_name from dba_index where  table_name = :table_name");
        m_pIDUSQuery[E_DBA_INDEX][E_SELECT_ALL]->SetSQL("select table_name,index_name from dba_index");
        //dba_process
        m_pIDUSQuery[E_DBA_PROCESS][E_INSERT]->SetSQL("insert into dba_process(process_name,pid,start_time,log_level) "
                                            "values(:process_name,:pid,:start_time,:log_level)");
        m_pIDUSQuery[E_DBA_PROCESS][E_DELETE]->SetSQL("delete from dba_process where process_name = :process_name ");
        m_pIDUSQuery[E_DBA_PROCESS][E_SELECT]->SetSQL("select process_name from dba_process where  process_name = :process_name ");
        m_pIDUSQuery[E_DBA_PROCESS][E_UPDATE]->SetSQL("update dba_process set pid = :pid,start_time = :start_time,log_level = :log_level "
                                            "where process_name = :process_name ");
        m_pIDUSQuery[E_DBA_PROCESS][E_SELECT_ALL]->SetSQL("select process_name,pid from dba_process ");
        _MDB_TRY_CATCH_END_
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  SyncAll
    * 函数描述	:  同步所有系统表
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncAll()
    {
        int iRet = 0;
        CHECK_RET(SyncTables(m_pIDUSQuery[E_DBA_TABLE]),"SyncTables failed.");
        CHECK_RET(SyncTableSpace(m_pIDUSQuery[E_DBA_TABLESPACE]),"SyncTableSpace failed.");
        CHECK_RET(SyncSequence(m_pIDUSQuery[E_DBA_SEQUENCE]),"SyncSequence failed.");
        CHECK_RET(SyncSession(m_pIDUSQuery[E_DBA_SESSION]),"SyncSession failed.");
        CHECK_RET(SyncResource(m_pIDUSQuery[E_DBA_RESOURCE]),"SyncResource failed.");
        CHECK_RET(SyncColumn(m_pIDUSQuery[E_DBA_COLUMN]),"SyncColumn failed.");
        CHECK_RET(SyncIndex(m_pIDUSQuery[E_DBA_INDEX]),"SyncIndex failed.");
        CHECK_RET(SyncProcess(m_pIDUSQuery[E_DBA_PROCESS]),"SyncProcess failed.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SyncTable
    * 函数描述	:  同步dba_tables;
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncTables(TMdbQuery * pQuery[])
    {

        TADD_FUNC("Start.");
        int iRet = 0;
        char sTemp[32] = {0};
        _MDB_TRY_CATCH_BEGIN_
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {
            TMdbTable * pTable = &(*itor);
            if(pTable->sTableName[0] ==  0 ){continue;}
            pQuery[E_SELECT]->SetParameter("table_name",pTable->sTableName);
            pQuery[E_SELECT]->Open();
            bool bInMdb = pQuery[E_SELECT]->Next();
            if(true == bInMdb && 0 != pTable->sTableName[0])
            {
                //已有记录，更新
                pQuery[E_UPDATE]->SetParameter("real_counts",    pTable->iCounts);
                pQuery[E_UPDATE]->SetParameter("stat",           pTable->cState);
                pQuery[E_UPDATE]->SetParameter("cin_counts",     pTable->iCINCounts);
                pQuery[E_UPDATE]->SetParameter("left_cin_nodes", pTable->lLeftCollIndexNodeCounts);
                pQuery[E_UPDATE]->SetParameter("full_pages",     pTable->iFullPages);
                pQuery[E_UPDATE]->SetParameter("free_pages",     pTable->iFreePages);
                pQuery[E_UPDATE]->SetParameter("read_lock",         pTable->bReadLock?"Y":"N");
                pQuery[E_UPDATE]->SetParameter("write_lock",        pTable->bWriteLock?"Y":"N");
                pQuery[E_UPDATE]->SetParameter("column_counts",     pTable->iColumnCounts);
                pQuery[E_UPDATE]->SetParameter("index_counts",      pTable->iIndexCounts);
                pQuery[E_UPDATE]->SetParameter("table_name",       pTable->sTableName);
                pQuery[E_UPDATE]->SetParameter("shard_backup",        pTable->m_bShardBack?"Y":"N");
                pQuery[E_UPDATE]->SetParameter("storage_type",        pTable->m_cStorageType);
                pQuery[E_UPDATE]->Execute();
                pQuery[E_UPDATE]->Commit();
            }
            else if(false == bInMdb && 0 != pTable->sTableName[0])
            {
                //没记录，插入
                pQuery[E_INSERT]->SetParameter("table_name",        pTable->sTableName);
                pQuery[E_INSERT]->SetParameter("table_space",    pTable->m_sTableSpace);
                pQuery[E_INSERT]->SetParameter("record_set_counts", pTable->iRecordCounts);
                pQuery[E_INSERT]->SetParameter("real_counts",       pTable->iCounts);
                pQuery[E_INSERT]->SetParameter("stat",              pTable->cState);
                pQuery[E_INSERT]->SetParameter("expand_record",     pTable->iExpandRecords);
                pQuery[E_INSERT]->SetParameter("read_lock",         pTable->bReadLock?"Y":"N");
                pQuery[E_INSERT]->SetParameter("write_lock",        pTable->bWriteLock?"Y":"N");
                pQuery[E_INSERT]->SetParameter("column_counts",     pTable->iColumnCounts);
                pQuery[E_INSERT]->SetParameter("index_counts",      pTable->iIndexCounts);
                pQuery[E_INSERT]->SetParameter("cin_counts",        pTable->iCINCounts);
                pQuery[E_INSERT]->SetParameter("left_cin_nodes",    pTable->lLeftCollIndexNodeCounts);
                pQuery[E_INSERT]->SetParameter("full_pages",        pTable->iFullPages);
                pQuery[E_INSERT]->SetParameter("free_pages",        pTable->iFreePages);
                pQuery[E_INSERT]->SetParameter("shard_backup",        pTable->m_bShardBack?"Y":"N");
                pQuery[E_INSERT]->SetParameter("storage_type",        pTable->m_cStorageType);
                memset(sTemp, 0, sizeof(sTemp));
                for(int n=0; n<pTable->m_tPriKey.iColumnCounts; ++n)
                {
                    if(n == 0)
                        sprintf(&sTemp[strlen(sTemp)], "%d", pTable->m_tPriKey.iColumnNo[n]);
                    else
                        sprintf(&sTemp[strlen(sTemp)], ",%d", pTable->m_tPriKey.iColumnNo[n]);
                }
                pQuery[E_INSERT]->SetParameter("primary_key",        sTemp);
                pQuery[E_INSERT]->Execute();
                pQuery[E_INSERT]->Commit();
            }
        }
        //删除
        char sTmpTabName[MAX_NAME_LEN] = {0};
        pQuery[E_SELECT_ALL]->Open();
        while(pQuery[E_SELECT_ALL]->Next())
        {
            memset(sTmpTabName, 0,sizeof(sTmpTabName));
            SAFESTRCPY(sTmpTabName, sizeof(sTmpTabName), pQuery[E_SELECT_ALL]->Field("table_name").AsString());
            if( NULL == m_pShmDSN->GetTableByName(sTmpTabName))
            {
                pQuery[E_DELETE]->SetParameter("table_name",sTmpTabName);
                pQuery[E_DELETE]->Execute();
                pQuery[E_DELETE]->Commit();
            }
        }
        _MDB_TRY_CATCH_END_
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  SyncTableSpace
    * 函数描述	:  同步表空间信息
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncTableSpace(TMdbQuery * pQuery[])
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        _MDB_TRY_CATCH_BEGIN_
        TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
        for(;itor != m_pShmDSN->m_TSList.end();++itor)
        {
            TMdbTableSpace * pTableSpace  = &(*itor);
            if(0 == pTableSpace->sName[0])
            {
                continue;
            }
            pQuery[E_SELECT]->SetParameter("table_space_name",pTableSpace->sName);
            pQuery[E_SELECT]->Open();
            if(pQuery[E_SELECT]->Next())
            {
                //找到记录更新
                pQuery[E_UPDATE]->SetParameter("total_pages",    pTableSpace->iTotalPages);
                pQuery[E_UPDATE]->SetParameter("free_pages",     pTableSpace->iEmptyPages);
                pQuery[E_UPDATE]->SetParameter("table_space_name", pTableSpace->sName);
                pQuery[E_UPDATE]->Execute();
                pQuery[E_UPDATE]->Commit();
            }
            else
            {
                //没有找到记录，插入
                pQuery[E_INSERT]->SetParameter("table_space_name", pTableSpace->sName);
                pQuery[E_INSERT]->SetParameter("page_size",        (long long)pTableSpace->iPageSize);
                pQuery[E_INSERT]->SetParameter("ask_pages",        pTableSpace->iRequestCounts);
                pQuery[E_INSERT]->SetParameter("total_pages",      pTableSpace->iTotalPages);
                pQuery[E_INSERT]->SetParameter("free_pages",       pTableSpace->iEmptyPages);
                pQuery[E_INSERT]->SetParameter("file_storage",       pTableSpace->m_bFileStorage?"TRUE":"FALSE");
                pQuery[E_INSERT]->Execute();
                pQuery[E_INSERT]->Commit();
            }
        }
        //删除
        char sTmpTsName[MAX_NAME_LEN] = {0};
        pQuery[E_SELECT_ALL]->Open();
        while(pQuery[E_SELECT_ALL]->Next())
        {
            memset(sTmpTsName, 0, sizeof(sTmpTsName));
            SAFESTRCPY(sTmpTsName, sizeof(sTmpTsName), pQuery[E_SELECT_ALL]->Field("table_space_name").AsString());
            if(NULL == m_pShmDSN->GetTableSpaceAddrByName(sTmpTsName))
            {
                pQuery[E_DELETE]->SetParameter("table_space_name",sTmpTsName);
                pQuery[E_DELETE]->Execute();
                pQuery[E_DELETE]->Commit();
            }
        }
        _MDB_TRY_CATCH_END_
        TADD_FUNC("Finish");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  SyncSequence
    * 函数描述	:  同步序列信息
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncSequence(TMdbQuery * pQuery[])
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        _MDB_TRY_CATCH_BEGIN_
        TShmList<TMemSeq>::iterator itor = m_pShmDSN->m_MemSeqList.begin();
        for(;itor != m_pShmDSN->m_MemSeqList.end();++itor)
        {
            TMemSeq * pSeq = &(*itor);  
            if(pSeq->sSeqName[0] == 0)
            {
                continue;
            }
            pQuery[E_SELECT]->SetParameter("sequence_name",pSeq->sSeqName);
            pQuery[E_SELECT]->Open();
            if(pQuery[E_SELECT]->Next())
            {
                pQuery[E_UPDATE]->SetParameter("start_value", pSeq->iStart);
                pQuery[E_UPDATE]->SetParameter("end_value",   pSeq->iEnd);
                pQuery[E_UPDATE]->SetParameter("cur_value",   pSeq->iCur);
                pQuery[E_UPDATE]->SetParameter("step_value", pSeq->iStep);
                pQuery[E_UPDATE]->SetParameter("sequence_name",pSeq->sSeqName);
                pQuery[E_UPDATE]->Execute();
                pQuery[E_UPDATE]->Commit();
            }
            else
            {
                //插入
                pQuery[E_INSERT]->SetParameter("sequence_name",   pSeq->sSeqName);
                pQuery[E_INSERT]->SetParameter("start_value",     pSeq->iStart);
                pQuery[E_INSERT]->SetParameter("end_value",       pSeq->iEnd);
                pQuery[E_INSERT]->SetParameter("cur_value",       pSeq->iCur);
                pQuery[E_INSERT]->SetParameter("step_value",      pSeq->iStep);
                pQuery[E_INSERT]->Execute();
                pQuery[E_INSERT]->Commit();
            }
        }
        //删除在内存中不存在的序列
        pQuery[E_SELECT_ALL]->Open();
        while(pQuery[E_SELECT_ALL]->Next())
        {
            if(NULL == m_pShmDSN->GetMemSeqByName(pQuery[E_SELECT_ALL]->Field("sequence_name").AsString()))
            {
                pQuery[E_DELETE]->SetParameter("sequence_name",pQuery[E_SELECT_ALL]->Field("sequence_name").AsString());
                pQuery[E_DELETE]->Execute();
                pQuery[E_DELETE]->Commit();
            }
        }
        _MDB_TRY_CATCH_END_
        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SyncSession
    * 函数描述	:  同步session
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncSession(TMdbQuery * pQuery[])
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        SyncLocalLink(pQuery);
        SyncRemoteLink(pQuery);
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  GeneralSyncResource
    * 函数描述	:  通用同步资源信息
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::GeneralSyncResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stReourceInfo)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        if(stReourceInfo.llMemKey <=0 )
        {
            return 0;
        }
        _MDB_TRY_CATCH_BEGIN_
        pQuery[E_SELECT]->SetParameter("mem_key", stReourceInfo.llMemKey);
        pQuery[E_SELECT]->Open();
        if(pQuery[E_SELECT]->Next())
        {
            //更新
            pQuery[E_UPDATE]->SetParameter("mem_id",   stReourceInfo.iMemId);
            pQuery[E_UPDATE]->SetParameter("mem_type", stReourceInfo.sMemType);
            pQuery[E_UPDATE]->SetParameter("mem_size", (long long)stReourceInfo.llMemSize);
            pQuery[E_UPDATE]->SetParameter("mem_left", (long long)stReourceInfo.llMemLeft);
            pQuery[E_UPDATE]->SetParameter("mem_key",stReourceInfo.llMemKey);
            pQuery[E_UPDATE]->Execute();
            pQuery[E_UPDATE]->Commit();
        }
        else
        {
            //插入
            pQuery[E_INSERT]->SetParameter("mem_id",   stReourceInfo.iMemId);
            pQuery[E_INSERT]->SetParameter("mem_type", stReourceInfo.sMemType);
            pQuery[E_INSERT]->SetParameter("mem_size", (long long)stReourceInfo.llMemSize);
            pQuery[E_INSERT]->SetParameter("mem_left", (long long)stReourceInfo.llMemLeft);
            pQuery[E_INSERT]->SetParameter("mem_key",stReourceInfo.llMemKey);
            pQuery[E_INSERT]->Execute();
            pQuery[E_INSERT]->Commit();
        }
        TADD_FUNC("Finish.");
        _MDB_TRY_CATCH_END_
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SyncResource
    * 函数描述	:  资源信息同步
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncResource(TMdbQuery * pQuery[])
    {
        TADD_FUNC("Start");
        int iRet = 0;

        ST_RESOURCE_INFO stResourceInfo;
        stResourceInfo.Clear();
        //管理区
        stResourceInfo.llMemKey = GET_MGR_KEY(m_pDsn->llDsnValue);
        stResourceInfo.llMemSize = m_pMdbConfig->GetDSN()->iManagerSize;
        if(TMdbShm::IsShmExist(stResourceInfo.llMemKey,stResourceInfo.llMemSize,stResourceInfo.iMemId) == false)
        {
            CHECK_RET(ERR_OS_SHM_NOT_EXIST,"Shm is not exist.");
        }
        stResourceInfo.llMemLeft = stResourceInfo.llMemSize - m_pShmDSN->GetUsedSize();
        SAFESTRCPY(stResourceInfo.sMemType,sizeof(stResourceInfo.sMemType),"MgrBlock");
        CHECK_RET(GeneralSyncResource(pQuery,stResourceInfo),"GeneralSyncResource falsed.");
        //数据区
        CHECK_RET(SyncDataAreaResource(pQuery,stResourceInfo),"SyncDataAreaResource falsed.");
        //变长存储区
        CHECK_RET(SyncVarcharAreaResource(pQuery,stResourceInfo),"SyncVarcharAreaResource falsed.");
        //基础索引区
        CHECK_RET(SyncBaseIdxAreaResource(pQuery,stResourceInfo),"SyncBaseIdxAreaResource falsed.");
        //冲突索引区
        CHECK_RET(SyncConflictIdxAreaResource(pQuery,stResourceInfo),"SyncConflictIdxAreaResource falsed.");
        //同步区
        CHECK_RET(SyncSAResouce(pQuery,stResourceInfo),"SyncSAResouce failed.");
        #if 0
        //主备同步缓存
        CHECK_RET(SyncRepAreaResource(pQuery,stResourceInfo),"SyncRepAreaResource falsed.");
        //Oracle同步缓存
        CHECK_RET(SyncOraRepAreaResource(pQuery,stResourceInfo),"SyncOraRepAreaResource falsed.");
        //capture同步缓存
        CHECK_RET(SyncCaptureRepAreaResource(pQuery,stResourceInfo),"SyncCaptureRepAreaResource falsed.");
        #endif
        
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  SyncColumn
    * 函数描述	:  同步列信息
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncColumn(TMdbQuery * pQuery[])
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sDataType[MAX_NAME_LEN] = {0};
        char sRepAttr[MAX_NAME_LEN] = {0};
        _MDB_TRY_CATCH_BEGIN_
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {
            TMdbTable * pTable = &(*itor);
            if(0 == pTable->sTableName[0]){continue;}
            pQuery[E_SELECT]->SetParameter("table_name",pTable->sTableName);
            pQuery[E_SELECT]->Open();
            bool bInMDB = pQuery[E_SELECT]->Next();
            if(!bInMDB)
            {
                int n = 0;
                for(n=0; n<pTable->iColumnCounts; ++n)
                {
                    memset(sDataType,0,sizeof(sDataType));
                    memset(sRepAttr,0,sizeof(sRepAttr));
                    m_pMdbConfig->GetDataType(pTable->tColumn[n].iDataType,sDataType,sizeof(sDataType));
                    m_pMdbConfig->GetRepType(pTable->iRepAttr,sRepAttr, sizeof(sRepAttr) );
                    pQuery[E_INSERT]->SetParameter("table_name",      pTable->sTableName);
                    pQuery[E_INSERT]->SetParameter("column_name",   pTable->tColumn[n].sName);
                    pQuery[E_INSERT]->SetParameter("data_type",     sDataType);
                    pQuery[E_INSERT]->SetParameter("data_len",      pTable->tColumn[n].iColumnLen);
                    pQuery[E_INSERT]->SetParameter("data_pos",      pTable->tColumn[n].iPos);
                    pQuery[E_INSERT]->SetParameter("rep_attr",      sRepAttr);
                    pQuery[E_INSERT]->SetParameter("Nullable",      pTable->tColumn[n].m_bNullable?"Y":"N");
                    pQuery[E_INSERT]->Execute();
                    pQuery[E_INSERT]->Commit();
                }
            }
            else
            {//表结构发生变更，需要重新插入列信息
                if(pTable->iMagic_n > m_tTableVersion[pTable->sTableName])
                {
                    m_tTableVersion[pTable->sTableName] = pTable->iMagic_n;
                    pQuery[E_DELETE]->SetParameter("table_name",pTable->sTableName);
                    pQuery[E_DELETE]->Execute();
                    pQuery[E_DELETE]->Commit();
                }
            }        
        }
        //刷新动态删除表的情况
        pQuery[E_SELECT_ALL]->Open();
        while(pQuery[E_SELECT_ALL]->Next())
        {
            if(NULL == m_pShmDSN->GetTableByName(pQuery[E_SELECT_ALL]->Field("table_name").AsString()))
            {
                pQuery[E_DELETE]->SetParameter("table_name",pQuery[E_SELECT_ALL]->Field("table_name").AsString());
                pQuery[E_DELETE]->Execute();
                pQuery[E_DELETE]->Commit();
            }
        }
        _MDB_TRY_CATCH_END_
        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SyncIndex
    * 函数描述	:  同步索引区
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncIndex(TMdbQuery * pQuery[])
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        _MDB_TRY_CATCH_BEGIN_
        TShmList<TMdbTable>::iterator itor = m_pShmDSN->m_TableList.begin();
        for(;itor != m_pShmDSN->m_TableList.end();++itor)
        {
            TMdbTable * pTable = &(*itor);
            if(0 == pTable->sTableName[0])
            {
                continue;
            }
            pQuery[E_SELECT]->SetParameter("table_name",pTable->sTableName);
            pQuery[E_SELECT]->Open();
            bool bInMDB = pQuery[E_SELECT]->Next();//不在系统表中认为是新建的表
            if(!bInMDB)
            {
                for(int n=0; n<pTable->iIndexCounts; ++n)
                {
                    pQuery[E_INSERT]->SetParameter("table_name",      pTable->sTableName);
                    pQuery[E_INSERT]->SetParameter("index_name",    pTable->tIndex[n].sName);
                    pQuery[E_INSERT]->SetParameter("index_type",    pTable->tIndex[n].m_iIndexType);
                    pQuery[E_INSERT]->SetParameter("priority",      pTable->tIndex[n].iPriority);
                    char sTemp[32] = { 0 };
                    for(int j=0; j<MAX_INDEX_COLUMN_COUNTS; ++j)
                    {
                        if(pTable->tIndex[n].iColumnNo[j] >= 0)
                        {
                            sprintf(sTemp+strlen(sTemp),"%d,",pTable->tIndex[n].iColumnNo[j]);
                        }
                    }
                    sTemp[strlen(sTemp) - 1] = 0;
                    pQuery[E_INSERT]->SetParameter("data_pos",sTemp);
                    pQuery[E_INSERT]->SetParameter("is_fix",      "Y");
                    pQuery[E_INSERT]->SetParameter("Algorithm_Type",     pTable->tIndex[n].GetAlgoType());
                    pQuery[E_INSERT]->Execute();
                    pQuery[E_INSERT]->Commit();
                }
                m_iIndexCounts[pTable->sTableName] = pTable->iIndexCounts;
            }
            else
            {//表结构发生变更需要删除该表的记录，然后重新插入
                if(pTable->iIndexCounts > m_iIndexCounts[pTable->sTableName])
                {
                    m_iIndexCounts[pTable->sTableName] = pTable->iIndexCounts;
                    pQuery[E_DELETE]->SetParameter("table_name",pTable->sTableName);
                    pQuery[E_DELETE]->Execute();
                    pQuery[E_DELETE]->Commit();
                }
            }
        }
        //处理表删除后索引的清理
        pQuery[E_SELECT_ALL]->Open();
        while(pQuery[E_SELECT_ALL]->Next())
        {
            if(NULL == m_pShmDSN->GetTableByName(pQuery[E_SELECT_ALL]->Field("table_name").AsString()))
            {
                pQuery[E_DELETE]->SetParameter("table_name",pQuery[E_SELECT_ALL]->Field("table_name").AsString());
                pQuery[E_DELETE]->Execute();
                pQuery[E_DELETE]->Commit();
            }
        }
        _MDB_TRY_CATCH_END_
        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SyncProcess
    * 函数描述	:  同步进程区
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbSysTableSync::SyncProcess(TMdbQuery * pQuery[])
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbProc tTempProc;
        _MDB_TRY_CATCH_BEGIN_
        TShmList<TMdbProc>::iterator itor = m_pShmDSN->m_ProcList.begin();
        for(;itor != m_pShmDSN->m_ProcList.end();++itor)
        {
            TMdbProc *pProc = &(*itor);
            if(pProc->sName[0] == 0)
            {
                continue;
            }
            memcpy(&tTempProc,pProc,sizeof(TMdbProc));
            if(false == tTempProc.IsValid()){continue;}//无效
            pQuery[E_SELECT]->SetParameter("process_name",tTempProc.sName);
            pQuery[E_SELECT]->Open();
            if(pQuery[E_SELECT]->Next())
            {
                //找到记录更新
                pQuery[E_UPDATE]->SetParameter("pid",           tTempProc.iPid);
                pQuery[E_UPDATE]->SetParameter("start_time",    tTempProc.sStartTime);
                pQuery[E_UPDATE]->SetParameter("log_level",     tTempProc.iLogLevel);
                pQuery[E_UPDATE]->SetParameter("process_name",  tTempProc.sName);
                pQuery[E_UPDATE]->Execute();
                pQuery[E_UPDATE]->Commit();
            }
            else
            {
                TADD_NORMAL("Proccess register [%s][%d].",tTempProc.sName,tTempProc.iPid);
                //没有找到记录，插入
                pQuery[E_INSERT]->SetParameter("process_name", tTempProc.sName);
                pQuery[E_INSERT]->SetParameter("pid",          tTempProc.iPid);
                pQuery[E_INSERT]->SetParameter("start_time",   tTempProc.sStartTime);
                pQuery[E_INSERT]->SetParameter("log_level",    tTempProc.iLogLevel);
                pQuery[E_INSERT]->Execute();
                pQuery[E_INSERT]->Commit();
            }
        }

        //扫描dba_process表中的记录是否在内存中存在
        int iProcID = 0;
        char sProcName[MAX_NAME_LEN];
        pQuery[E_SELECT_ALL]->Open();
        while(pQuery[E_SELECT_ALL]->Next())
        {
            memset(sProcName,0,sizeof(sProcName));
            SAFESTRCPY(sProcName,sizeof(sProcName),pQuery[E_SELECT_ALL]->Field("process_name").AsString());
            iProcID = pQuery[E_SELECT_ALL]->Field("pid").AsInteger();
            if(NULL == m_pShmDSN->GetProcByPid(iProcID))
            {
                pQuery[E_DELETE]->SetParameter("process_name",sProcName);
                pQuery[E_DELETE]->Execute();
                pQuery[E_DELETE]->Commit();
            }
        }
        _MDB_TRY_CATCH_END_
        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SyncDataAreaResource
    * 函数描述	:  同步数据区资源使用情况
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncDataAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo)
    {
        int iRet = 0;
        for(int i = 0; i < m_pDsn->iShmCounts; ++i)
        {
            if(m_pDsn->iShmID[i]<= 0)
            {
                continue;
            }
            TMdbShmHead *pHead = (TMdbShmHead*)m_pShmDSN->GetDataShm(i);
            if(NULL == pHead)
            {
                continue;
            }
            stResourceInfo.Clear();
            stResourceInfo.iMemId = m_pDsn->iShmID[i];
            stResourceInfo.llMemKey = m_pDsn->iShmKey[i];
            stResourceInfo.llMemSize = pHead->iTotalSize;
            stResourceInfo.llMemLeft  = pHead->iTotalSize - pHead->iLeftOffSet;
            SAFESTRCPY(stResourceInfo.sMemType,sizeof(stResourceInfo.sMemType),"DataBlock");
            CHECK_RET(GeneralSyncResource(pQuery,stResourceInfo),"GeneralSyncResource falsed.");
        }
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  SyncVarcharAreaResource
    * 函数描述	:  资源信息同步
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncVarcharAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo)
    {
        int iRet = 0;
        for(int i = 0; i < m_pDsn->iVarCharShmCounts; ++i)
        {
            if(m_pDsn->iVarCharShmID[i]<= 0)
            {
                continue;
            }
            char* pAddr  = NULL;
            m_pShmDSN->AttachvarCharBlockShm(i,&pAddr);
            TMdbShmHead *pHead = (TMdbShmHead*)pAddr;
            if(NULL == pHead)
            {
                continue;
            }
            stResourceInfo.Clear();
            stResourceInfo.iMemId = m_pDsn->iVarCharShmID[i];
            stResourceInfo.llMemKey = m_pDsn->iVarCharShmKey[i];
            stResourceInfo.llMemSize = pHead->iTotalSize;
            stResourceInfo.llMemLeft  = pHead->iTotalSize - pHead->iLeftOffSet;
            SAFESTRCPY(stResourceInfo.sMemType,sizeof(stResourceInfo.sMemType),"VarCharBlock");
            CHECK_RET(GeneralSyncResource(pQuery,stResourceInfo),"GeneralSyncResource falsed.");
        }
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  SyncResource
    * 函数描述	:  资源信息同步
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncBaseIdxAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo)
    {
        int iRet = 0;
        for(int i = 0; i < m_pDsn->iBaseIndexShmCounts; ++i)
        {
            if(m_pDsn->iBaseIndexShmID[i]<= 0)
            {
                continue;
            }
            TMdbBaseIndexMgrInfo* pInfo  = (TMdbBaseIndexMgrInfo*)m_pShmDSN->GetBaseIndex(i);
            if(NULL == pInfo)
            {
                continue;
            }
            stResourceInfo.Clear();
            stResourceInfo.iMemId = m_pDsn->iBaseIndexShmID[i];
            stResourceInfo.llMemKey = m_pDsn->iBaseIndexShmKey[i];
            stResourceInfo.llMemSize = pInfo->iTotalSize;
            size_t iMemLeft = 0;
            for(int j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
            {
                if(pInfo->tFreeSpace[j].iPosAdd == 0)
                {
                    break;
                }
                iMemLeft += pInfo->tFreeSpace[j].iSize;
            }
            stResourceInfo.llMemLeft  = iMemLeft;
            SAFESTRCPY(stResourceInfo.sMemType,sizeof(stResourceInfo.sMemType),"BaseIndexBlock");
            CHECK_RET(GeneralSyncResource(pQuery,stResourceInfo),"GeneralSyncResource falsed.");
        }
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  SyncResource
    * 函数描述	:  资源信息同步
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncConflictIdxAreaResource(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo)
    {
        int iRet = 0;
        for(int i = 0; i < m_pDsn->iConflictIndexShmCounts; ++i)
        {
            if(m_pDsn->iConflictIndexShmID[i]<= 0)
            {
                continue;
            }
            TMdbConflictIndexMgrInfo* pInfo  = (TMdbConflictIndexMgrInfo*)m_pShmDSN->GetConflictIndex(i);
            if(NULL == pInfo)
            {
                continue;
            }
            stResourceInfo.Clear();
            stResourceInfo.iMemId = m_pDsn->iConflictIndexShmID[i];
            stResourceInfo.llMemKey = m_pDsn->iConflictIndexShmKey[i];
            stResourceInfo.llMemSize = pInfo->iTotalSize;
            size_t iMemLeft = 0;
            for(int j = 0; j<MAX_BASE_INDEX_COUNTS; j++)
            {
                if(pInfo->tFreeSpace[j].iPosAdd == 0)
                {
                    break;
                }
                iMemLeft += pInfo->tFreeSpace[j].iSize;
            }
            stResourceInfo.llMemLeft  = iMemLeft;
            SAFESTRCPY(stResourceInfo.sMemType,sizeof(stResourceInfo.sMemType),"ConflictIndexBlock");
            CHECK_RET(GeneralSyncResource(pQuery,stResourceInfo),"GeneralSyncResource falsed.");
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SyncSAResouce
    * 函数描述	:  同步[同步区]信息
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncSAResouce(TMdbQuery * pQuery[],ST_RESOURCE_INFO & stResourceInfo)
    {
        int iRet = 0;
        TMdbSyncArea &tSA = m_pDsn->m_arrSyncArea;
        TMdbMemQueue* pQueue = (TMdbMemQueue*)m_pShmDSN->GetSyncAreaShm();
        CHECK_OBJ(pQueue);
        long iLeftSize = 0;
        if(pQueue->iPushPos >= pQueue->iPopPos)
        {
            iLeftSize = (pQueue->iTailPos - pQueue->iStartPos) - (pQueue->iPushPos - pQueue->iPopPos);
        }
        else
        {
            iLeftSize = (pQueue->iPopPos - pQueue->iPushPos);
        }
        stResourceInfo.Clear();
        
        stResourceInfo.iMemId =   tSA.m_iShmID;
        stResourceInfo.llMemKey = tSA.m_iShmKey;
        stResourceInfo.llMemSize =tSA.m_iShmSize*1024*1024; 
        stResourceInfo.llMemLeft  = iLeftSize;
        SAFESTRCPY(stResourceInfo.sMemType,sizeof(stResourceInfo.sMemType),tSA.m_sName);
        CHECK_RET(GeneralSyncResource(pQuery,stResourceInfo),"GeneralSyncResource falsed.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SyncLocalLink
    * 函数描述	:  同步本地链接
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncLocalLink(TMdbQuery * pQuery[])
    {
        TADD_FUNC("Start.");
         int iRet = 0;
         _MDB_TRY_CATCH_BEGIN_
         int i = 0;
         TMdbLocalLink tTempLocalLink;
         TShmList<TMdbLocalLink>::iterator itorLocal = m_pShmDSN->m_LocalLinkList.begin();
         for(i=0; i<MAX_LINK_COUNTS && itorLocal != m_pShmDSN->m_LocalLinkList.end(); ++i,++itorLocal)
         {
             TMdbLocalLink * pLocalLink = &(*itorLocal);
             pQuery[E_SELECT]->SetParameter("session_id",i);
             pQuery[E_SELECT]->Open();
             if(pQuery[E_SELECT]->Next())
             {
                 //有记录
                 if(pLocalLink->iPID < 0)
                 {
                     //删除
                     pQuery[E_DELETE]->SetParameter("session_id",i);
                     pQuery[E_DELETE]->Execute();
                     pQuery[E_DELETE]->Commit();
                 }
                 else
                 { //更新
                     memcpy(&tTempLocalLink,pLocalLink,sizeof(TMdbLocalLink));
                     if(tTempLocalLink.IsValid())
                     {
                         pQuery[E_UPDATE]->SetParameter("pid",         tTempLocalLink.iPID);
                         pQuery[E_UPDATE]->SetParameter("tid",         (long long)tTempLocalLink.iTID);
                         pQuery[E_UPDATE]->SetParameter("ip",          "127.0.0.1");
                         pQuery[E_UPDATE]->SetParameter("Handle",      -1);
                         pQuery[E_UPDATE]->SetParameter("start_time",  tTempLocalLink.sStartTime);
                         pQuery[E_UPDATE]->SetParameter("state",       tTempLocalLink.cState==Link_use?"L":"D");
                         pQuery[E_UPDATE]->SetParameter("log_level",   tTempLocalLink.iLogLevel);
                         pQuery[E_UPDATE]->SetParameter("sqlpos",      tTempLocalLink.iSQLPos);
                         pQuery[E_UPDATE]->SetParameter("session_id",  i);
                         pQuery[E_UPDATE]->Execute();
                         pQuery[E_UPDATE]->Commit();
                     }
                 }
             }
             else if(pLocalLink->iPID > 0)
             {
                 //新增
                 memcpy(&tTempLocalLink,pLocalLink,sizeof(TMdbLocalLink));
                 TADD_NORMAL("New local link to mdb [pid=%d,tid=%lld]",tTempLocalLink.iPID,(long long)tTempLocalLink.iTID);
                 if(tTempLocalLink.IsValid())
                 {
                     pQuery[E_INSERT]->SetParameter("session_id",  i);
                     pQuery[E_INSERT]->SetParameter("pid",         tTempLocalLink.iPID);
                     pQuery[E_INSERT]->SetParameter("tid",         (long long)tTempLocalLink.iTID);
                     pQuery[E_INSERT]->SetParameter("ip",          "127.0.0.1");
                     pQuery[E_INSERT]->SetParameter("Handle",      -1);
                     pQuery[E_INSERT]->SetParameter("start_time",  tTempLocalLink.sStartTime);
                     pQuery[E_INSERT]->SetParameter("state",       tTempLocalLink.cState==Link_use?"L":"D");
                     pQuery[E_INSERT]->SetParameter("log_level",   tTempLocalLink.iLogLevel);
                     pQuery[E_INSERT]->SetParameter("sqlpos",         tTempLocalLink.iSQLPos);
                     pQuery[E_INSERT]->Execute();
                     pQuery[E_INSERT]->Commit();
                 }
             }
         }
         _MDB_TRY_CATCH_END_
         TADD_FUNC("Finish.");
         return iRet;
    }

    /******************************************************************************
    * 函数名称	:  SyncRemoteLink
    * 函数描述	: 同步远程链接
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbSysTableSync::SyncRemoteLink(TMdbQuery * pQuery[])
    {
        TADD_FUNC("Start.");
         int iRet = 0;
         _MDB_TRY_CATCH_BEGIN_
         int i = 0;
         //远程链接
         TMdbRemoteLink tTempRemoteLink;
         TShmList<TMdbRemoteLink>::iterator itorRemote = m_pShmDSN->m_RemoteLinkList.begin();
         for(i=0; i<MAX_LINK_COUNTS && itorRemote != m_pShmDSN->m_RemoteLinkList.end(); ++i,++itorRemote)
         {
             TMdbRemoteLink * pRemoteLink = &(*itorRemote);
             pQuery[E_SELECT]->SetParameter("session_id",i+MAX_LINK_COUNTS);
             pQuery[E_SELECT]->Open();
             if(pQuery[E_SELECT]->Next())
             {
                 if(0 == pRemoteLink->sIP[0])
                 {
                     //删除
                     pQuery[E_DELETE]->SetParameter("session_id", i+MAX_LINK_COUNTS);
                     pQuery[E_DELETE]->Execute();
                     pQuery[E_DELETE]->Commit();
                 }
                 else
                 {
                     //更新
                     memcpy(&tTempRemoteLink,pRemoteLink,sizeof(TMdbRemoteLink));
                     if(tTempRemoteLink.IsValid())
                     {               
                         pQuery[E_UPDATE]->SetParameter("pid",         tTempRemoteLink.iPID);
                         pQuery[E_UPDATE]->SetParameter("tid",         (long long)tTempRemoteLink.iTID);
                         pQuery[E_UPDATE]->SetParameter("ip",          tTempRemoteLink.sIP);
                         pQuery[E_UPDATE]->SetParameter("Handle",      tTempRemoteLink.iHandle);
                         pQuery[E_UPDATE]->SetParameter("start_time",  tTempRemoteLink.sStartTime);
                         pQuery[E_UPDATE]->SetParameter("state",       tTempRemoteLink.cState==Link_use?"L":"D");
                         pQuery[E_UPDATE]->SetParameter("log_level",   tTempRemoteLink.iLogLevel);
                         pQuery[E_UPDATE]->SetParameter("sqlpos",      tTempRemoteLink.iSQLPos);
                         pQuery[E_UPDATE]->SetParameter("session_id",  i+MAX_LINK_COUNTS);
                         pQuery[E_UPDATE]->Execute();
                         pQuery[E_UPDATE]->Commit();
                     }
        
                 }
             }
             else if(0 != pRemoteLink->sIP[0])
             {
                 //新增
                 memcpy(&tTempRemoteLink,pRemoteLink,sizeof(TMdbRemoteLink));
                 if(tTempRemoteLink.IsValid())
                 {
                     pQuery[E_INSERT]->SetParameter("session_id",  i+MAX_LINK_COUNTS);
                     pQuery[E_INSERT]->SetParameter("pid",         tTempRemoteLink.iPID);
                     pQuery[E_INSERT]->SetParameter("tid",         (long long)tTempRemoteLink.iTID);
                     pQuery[E_INSERT]->SetParameter("ip",          tTempRemoteLink.sIP);
                     pQuery[E_INSERT]->SetParameter("Handle",      tTempRemoteLink.iHandle);
                     pQuery[E_INSERT]->SetParameter("start_time",  tTempRemoteLink.sStartTime);
                     pQuery[E_INSERT]->SetParameter("state",       tTempRemoteLink.cState==Link_use?"L":"D");
                     pQuery[E_INSERT]->SetParameter("sqlpos",         tTempRemoteLink.iSQLPos);
                     pQuery[E_INSERT]->SetParameter("log_level",   tTempRemoteLink.iLogLevel);
                     pQuery[E_INSERT]->Execute();
                     pQuery[E_INSERT]->Commit();
                 }
             }
         }
         _MDB_TRY_CATCH_END_
         TADD_FUNC("Finish.");
         return iRet;

    }

    TMdbSysTableThread::TMdbSysTableThread()
    {
        m_bIsRun  = false;
        m_pShmDSN = NULL;
        m_pDsn    = NULL;
    }


    TMdbSysTableThread::~TMdbSysTableThread()
    {
        
    }

    //初始化
    void TMdbSysTableThread::Init(TMdbShmDSN* pShmDSN)
    {
        m_pShmDSN = pShmDSN;
        m_pDsn = m_pShmDSN->GetInfo();
        m_tLinkCtrl.Attach(m_pDsn->sName);
        m_tProcCtrl.Init(m_pDsn->sName);
    }

    void* TMdbSysTableThread::agent(void* p)
    {
        TMdbSysTableThread* pThread = (TMdbSysTableThread*)p;
        pThread->svc();
        return 0;
    }


    //开始启动
    int TMdbSysTableThread::Start()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        pthread_t       tID;
        pthread_attr_t  thread_attr;
        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);

        CHECK_RET(pthread_attr_setstacksize(&thread_attr, 5*1024*1024),"Can't pthread_attr_setstacksize(), errno=[%d], errmsg=[%s].",
                  errno, strerror(errno));
        CHECK_RET(pthread_create(&tID, &thread_attr, agent, this),"Can't pthread_attr_setstacksize(), errno=[%d], errmsg=[%s].",
                  errno, strerror(errno));
        return iRet;
    }

    //开始停止
    void TMdbSysTableThread::Stop()
    {
        m_bRunFlag = false;
    }

    //处理
    int TMdbSysTableThread::svc()
    {
        TADD_FLOW("***************Start SysTableThread****************");
        int iRet = 0;
        m_bRunFlag = true;
        m_bIsRun = true;
        TMdbSysTableSync tSysTableSync;
        CHECK_RET(tSysTableSync.Init(m_pDsn->sName),"tSysTableSync.Init failed.");
        time(&m_tLastTime); //取得当前时间的time_t值
        int iCount = 15;
        while(true)
        {
            //如果有命令需要停止
            if(m_pDsn->cState == DB_stop || m_bRunFlag == false)
            {
                TADD_NORMAL("SysTableThread stop.");
                m_bIsRun = false;
                break;
            }
            if(iCount -- < 0)
            {//10s清理一次连接
                ClearLink();
                iCount = 15;
            }
            tSysTableSync.SyncAll();//同步系统表
            ResetSysLogLevel();
            TMdbDateTime::MSleep(1000);
        };
        TADD_NORMAL("TMdbSysTableThread Exit");
        return iRet;
    }
    //重置日志级别
    void TMdbSysTableThread::ResetSysLogLevel()
    {
        TADD_FUNC("Start.");
        time_t tCurTime;
        time(&tCurTime);
        if((tCurTime - m_tLastTime) >= 1800)
        {
           m_tProcCtrl.ResetLogLevel();
           m_tLastTime = tCurTime;
        }
        TADD_FUNC("Finish.");
    }

    //清除不需要的链接
    void TMdbSysTableThread::ClearLink()
    {
        TADD_FUNC("Start.");
        m_tLinkCtrl.ClearInvalidLink();
        TADD_FUNC("Finish.");
    }
//}

