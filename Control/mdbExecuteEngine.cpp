/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 开发部 CCB项目--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbExecuteEngine.cpp
*@Description： SQL执行引擎
*@Author:	     jin.shaohua
*@Date：	    2012.05
*@History:
******************************************************************************************/
#include "Control/mdbExecuteEngine.h"
#include "Control/mdbLinkCtrl.h"
#include "Control/mdbVarcharMgr.h"
#include "Helper/mdbDateTime.h"
#include "Helper/SyntaxTreeAnalyse.h"
//#include "Helper/mdbPerfInfo.h"
#include "Interface/mdbRollback.h"
#include "Helper/mdbMemValue.h"
#include "Helper/mdbBase.h"
#include "Helper/mdbMalloc.h"
#include "Interface/mdbQuery.h"
//#include "BillingSDK.h"


//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{

#define CHECK_RET_FILL(_ret,...) if((iRet = _ret)!= 0){ TADD_ERROR(iRet,__VA_ARGS__);m_tError.FillErrMsg(iRet,__VA_ARGS__);return iRet;}
#define CHECK_RET_FILL_CODE(_ret,_code,...) if((iRet = _ret)!= 0){ TADD_ERROR(iRet,__VA_ARGS__);m_tError.FillErrMsg(_code,__VA_ARGS__);return _code;}
#define CHECK_OBJ_FILL(_obj) if(NULL == _obj){TADD_ERROR(ERR_APP_INVALID_PARAM,#_obj" is null");m_tError.FillErrMsg(ERR_OS_NO_MEMROY,#_obj" is null");return ERR_APP_INVALID_PARAM;}
#define CHECK_RET_FILL_BREAK(_ret,...)if((iRet = _ret)!= 0){ TADD_ERROR(iRet,__VA_ARGS__);m_tError.FillErrMsg(iRet,__VA_ARGS__);break;}
#define CHECK_RET_FILL_CODE_BREAK(_ret,_code,...) if((iRet = _ret)!= 0){ TADD_ERROR(iRet,__VA_ARGS__);iRet=_code;m_tError.FillErrMsg(_code,__VA_ARGS__);break;}

    //next 的类型
    enum E_NEXT_TYPE
    {
        NEXT_NORMAL  = 1, //正常按索引遍历的next
        NEXT_CACHE    = 2//缓存查询
    };

    /******************************************************************************
    * 函数名称	:  TMdbExecuteEngine
    * 函数描述	:  mdb执行引擎初始化工作
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbExecuteEngine::TMdbExecuteEngine():
        m_pMdbSqlParser(NULL),
        m_iCurIndex(-1),
        m_bScanAll(false),
        m_llScanAllPos(-1),
        m_pTable(NULL),
        m_pDsn(NULL),
        m_pMdbShmDsn(NULL),
        m_iRowsAffected(0),
        m_pInsertBlock(NULL),
        m_pVarCharBlock(NULL),
        m_bCanRollBack(false),
        m_pUpdateBlock(NULL),
        m_aRowIndexPos(NULL),
        m_pIsStop(NULL)
    {
        memset(m_sTempValue,0x00,sizeof(m_sTempValue));
    }

    /******************************************************************************
    * 函数名称	:  ~TMdbExecuteEngine
    * 函数描述	:  mdb执行引擎析构
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbExecuteEngine::~TMdbExecuteEngine()
    {
        SAFE_DELETE_ARRAY(m_pInsertBlock);
        SAFE_DELETE_ARRAY(m_pVarCharBlock);
        SAFE_DELETE_ARRAY(m_pUpdateBlock);
        SAFE_DELETE_ARRAY(m_aRowIndexPos);
    }
    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	:  做一些初始化并清理工作
    * 输入		:  pMdbSqlParser -解析器iFlag - SQL属性
    * 输出		:  无
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::Init(TMdbSqlParser * pMdbSqlParser,MDB_INT32 iFlag, TMdbLocalLink* pLocalLink)
    {
        TADD_FUNC("Start.iFlag=[%d].",iFlag);
        int iRet = 0;
        SAFE_DELETE_ARRAY(m_pInsertBlock);//对于insertblock需要每次清理，因为记录长度不一样
        SAFE_DELETE_ARRAY(m_pUpdateBlock);
        SAFE_DELETE_ARRAY(m_aRowIndexPos);//清理
        CHECK_OBJ_FILL(pMdbSqlParser);
        m_pMdbSqlParser = pMdbSqlParser;
		CHECK_OBJ_FILL(pLocalLink);
		m_pLocalLink = pLocalLink;
        ClearLastExecute();
        m_pTable = m_pMdbSqlParser->m_stSqlStruct.pMdbTable;
        TADD_DETAIL("Execute Table=[%s].",m_pTable->sTableName);
        m_pDsn   = m_pMdbSqlParser->m_stSqlStruct.pShmDSN->GetInfo();
        m_mdbPageCtrl.SetDSN(m_pDsn->sName);
        //m_tVarcharMgr.SetConfig(m_pMdbSqlParser->m_stSqlStruct.pShmDSN);//设置varchar
        m_tVarcharCtrl.Init(m_pDsn->sName);
        CHECK_RET_FILL_CODE(m_mdbTSCtrl.Init(m_pDsn->sName,m_pTable->m_sTableSpace),
                       ERR_OS_ATTACH_SHM,"m_mdbTSCtrl.Init error");
		
		CHECK_RET_FILL_CODE(m_mdbIndexCtrl.AttachDsn(m_pMdbSqlParser->m_stSqlStruct.pShmDSN),
                       ERR_OS_ATTACH_SHM,"m_mdbIndexCtrl.AttachDsn failed.");
        CHECK_RET_FILL_CODE(m_mdbIndexCtrl.AttachTable(m_pMdbSqlParser->m_stSqlStruct.pShmDSN,m_pMdbSqlParser->m_stSqlStruct.pMdbTable),
                       ERR_OS_ATTACH_SHM,"m_mdbIndexCtrl.AttachTable failed.");
        CHECK_RET_FILL_CODE(m_MdbTableWalker.AttachTable(m_pMdbSqlParser->m_stSqlStruct.pShmDSN,m_pMdbSqlParser->m_stSqlStruct.pMdbTable,m_pMdbSqlParser->m_stSqlStruct.iSqlType),
						ERR_OS_ATTACH_SHM,"m_MdbTableWalker.AttachTable failed.");
		CHECK_RET_FILL_CODE(m_mdbIndexCtrl.FillLinkInfo(pLocalLink),ERROR_UNKNOWN,"m_mdbIndexCtrl.FillLinkInfo failed");
        CHECK_RET_FILL_CODE(m_tMdbFlush.Init(pMdbSqlParser,iFlag),ERR_SQL_FLUSH_DATA,"m_tMdbFlush.Init failed");//初始化flush模块
        CHECK_RET_FILL_CODE(m_tObserveTableExec.Init(m_pDsn->sName,OB_TABLE_EXEC),ERR_DB_OBSERVE,"m_tObserveTableExec.Init failed");
        CHECK_RET_FILL_CODE(m_tObserveTableExec.SetExecEngine(this),ERR_DB_OBSERVE,"m_tObserveTableExec.SetExecEngine failed.");
        CHECK_RET_FILL_CODE(m_tObserveTableExec.SetSQLParser(pMdbSqlParser),ERR_DB_OBSERVE,"m_tObserveTableExec.SetSQLParser failed.");
        CHECK_RET_FILL_CODE(m_tRowCtrl.Init(m_pDsn->sName,m_pTable->sTableName),ERR_OS_ATTACH_SHM,"m_tRowCtrl.AttachTable failed.");//记录管理
        CHECK_RET_FILL_CODE(m_tCacheTable.Create(pMdbSqlParser),iRet,"Cache Table Create failed.");//创建临时表
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbExecuteEngine::Init(TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable,TMdbLocalLink* pLocalLink)
    {
        int iRet = 0;
		CHECK_OBJ(pTable);
		CHECK_OBJ(pMdbShmDsn);
		CHECK_OBJ(pLocalLink);
		
		if(m_pMdbShmDsn != pMdbShmDsn)
		{
			m_pMdbShmDsn = pMdbShmDsn;
			m_pDsn = pMdbShmDsn->GetInfo();
			CHECK_RET_FILL_CODE(m_mdbIndexCtrl.AttachDsn(pMdbShmDsn),ERR_OS_ATTACH_SHM,"m_mdbIndexCtrl.AttachDsn error.");
			CHECK_RET_FILL_CODE(m_tVarcharCtrl.Init(pMdbShmDsn->GetInfo()->sName),ERR_OS_ATTACH_SHM,"m_tVarcharCtrl.Init error.");
			m_mdbPageCtrl.SetDSN(pMdbShmDsn->GetInfo()->sName);
		}
		
		CHECK_RET_FILL_CODE(m_MdbTableWalker.AttachTable(pMdbShmDsn,pTable,TK_INSERT),ERR_OS_ATTACH_SHM,"m_MdbTableWalker.AttachTable error.");
		
		if(m_pTable != pTable)
		{
			m_pTable = pTable;
			CHECK_RET_FILL_CODE(m_mdbIndexCtrl.AttachTable(pMdbShmDsn,pTable),ERR_OS_ATTACH_SHM,"m_mdbIndexCtrl.AttachTable error.");
			CHECK_RET_FILL_CODE(m_tRowCtrl.Init(pMdbShmDsn->GetInfo()->sName,pTable),ERR_OS_ATTACH_SHM,"m_tRowCtrl.Init error.");
		}
		if(m_pLocalLink!=pLocalLink)
		{
			m_pLocalLink = pLocalLink;
			CHECK_RET_FILL_CODE(m_mdbIndexCtrl.FillLinkInfo(m_pLocalLink),ERR_OS_ATTACH_SHM,"m_mdbIndexCtrl.FillLinkInfo error.");
		}

        return iRet;
    }

	
	void TMdbExecuteEngine::SetLink(TMdbLocalLink* pLocalLink)
	{
		m_pLocalLink = pLocalLink;
	}
	
	bool TMdbExecuteEngine::IsUseTrans()
	{
		return m_bCanRollBack && m_pTable->bRollBack;
	}
	
	int TMdbExecuteEngine::CheckDiskFree()
	{
		int iRet = 0;

		if (!m_tMdbFlush.bNeedToFlush())
		{
			return iRet;
		}
		
		if(m_pDsn->bDiskSpaceStat == false)
		{
			return ERR_OS_NO_DISK_SPACE;
		}

		return iRet;
	}

    /******************************************************************************
    * 函数名称	:  Execute
    * 函数描述	:  按解析出的SQL结构进行解析
    * 输入		:
    * 输出		:  无
    * 返回值	:  0 - 成功!0 - 不成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::Execute()
    {
        TADD_FUNC("Start.");
        int iRet = 0;	
        ClearLastExecute();//清理上一次的执行结果
        CHECK_RET_FILL(CheckDiskFree(),"DiskSpace < [%lld]KB, Cannot Flush log, Execute failed.",MIN_DISK_SPACE_KB);
        //表可能发生改动，检查表是否存在
        CHECK_RET_FILL(m_pMdbSqlParser->CheckTableExist(),"Table[%s] not exist",m_pMdbSqlParser->m_stSqlStruct.sTableName);
        CHECK_RET_FILL(m_pMdbSqlParser->GetWhereIndex(m_pVIndex),"GetWhereIndex error.");//获取可以使用的索引
        CHECK_OBJ(m_pVIndex);
        if(m_pVIndex->size() == 0)
        {
            TADD_DETAIL("no index, use all scan....");//如果没有使用索引则全量遍历
            m_bScanAll = true;
            m_llScanAllPos = -1;
        }
        m_pMdbSqlParser->ExecuteLimit();//计算limit范围
		m_pLocalLink->iExecuteID++;
		
        switch(m_pMdbSqlParser->m_stSqlStruct.iSqlType)
        {
            //按SQL类型执行
        case TK_INSERT:
            CHECK_RET_FILL(ExecuteInsert(),"ExecuteInsert failed");
            break;
        case TK_DELETE:
            CHECK_RET_FILL(m_tLimitCtrl.Init(m_pMdbSqlParser->m_listOutputLimit),"m_tLimitCtrl.Init error..");
            CHECK_RET_FILL(ExecuteDelete(),"ExecuteDelete failed");
            break;
        case TK_UPDATE:
			{
	            CHECK_RET_FILL(m_tLimitCtrl.Init(m_pMdbSqlParser->m_listOutputLimit),"m_tLimitCtrl.Init error..");
				if(IsUseTrans())
				{
					CHECK_RET_FILL(ExecuteUpdateTrans(),"ExecuteUpdateTrans failed");
				}
				else
				{
					CHECK_RET_FILL(ExecuteUpdate(),"ExecuteUpdate failed");
				}
        	}
            break;
        case TK_SELECT:
            m_pMdbSqlParser->ClearMemValue(m_pMdbSqlParser->m_listOutputCollist);//清理输出列
            if(m_tCacheTable.bNeedCached())
            {//需要缓存查询
                ExecuteCacheSelect();
                m_tCacheTable.Open();
            }
            CHECK_RET_FILL(m_tLimitCtrl.Init(m_pMdbSqlParser->m_listOutputLimit),"m_tLimitCtrl.Init error..");
            break;
        default:
            CHECK_RET_FILL(-1,"error sql type[%d]",m_pMdbSqlParser->m_stSqlStruct.iSqlType);
            break;
        }
        m_tObserveTableExec.Record();//记录
        TADD_FUNC("Finish.");
        return iRet;
    }


	
	void TMdbExecuteEngine::SetCancelPoint(int* pPoint)
	{
		m_pIsStop = pPoint;
	}

    /******************************************************************************
    * 函数名称	:  ExecuteUpdate
    * 函数描述	:  执行update操作
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ExecuteUpdate()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_iRowsAffected = 0;
        //创建临时更新块
        if(NULL == m_pUpdateBlock)
        {
            m_pUpdateBlock = new(std::nothrow) char[m_pTable->iOneRecordSize];
            CHECK_OBJ_FILL(m_pUpdateBlock);
            TADD_FLOW("Create new Update Block.size[%d].",m_pTable->iOneRecordSize);
        }
		
        bool bNext = false; 
        long long iTimeStamp = m_pDsn->iTimeStamp;  
        
        while(1)
        {
            
            CHECK_RET_FILL_BREAK(Next(bNext),"Next failed.");//下一个
            if(false == bNext){break;}//结束
            TADD_DETAIL("Find Row[%d].",m_iRowsAffected);
            if(CheckIsStop() == true)
            {
                //收到信号控制
                CHECK_RET_FILL(ERR_SQL_STOP_EXEC,"Catch the SIGINT signal.");
            }
            CHECK_RET_FILL_CODE(m_mdbPageCtrl.Attach(m_pPageAddr, m_pTable->bReadLock, m_pTable->bWriteLock),
                ERR_OS_ATTACH_SHM,"tPageCtrl.Attach() failed.");
            CHECK_RET_FILL(m_mdbPageCtrl.WLock(),"tPageCtrl.WLock() failed.");
            do{
                CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputPriKey),"FillSqlParserValue failed.");//填充主键信息
                CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue error.");
                CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteSQL(),"ExecuteSQL error.");
                CHECK_RET_FILL_BREAK(GetUpdateDiff(),"GetUpdateDiff error.");
                CHECK_RET_FILL_BREAK(UpdateData(),"UpdateData failed..");
                CHECK_RET_FILL_BREAK(UpdateRowDataTimeStamp(m_pDataAddr,m_pTable->m_iTimeStampOffset,iTimeStamp), "update row data timestamp failed.");
            }while(0);
            m_mdbPageCtrl.UnWLock();//解页锁

            CHECK_RET_FILL(iRet,"ERROR.");
            m_mdbTSCtrl.SetPageDirtyFlag(((TMdbPage*)m_pPageAddr)->m_iPageID);//设置脏页
            m_iRowsAffected ++;
            CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoQueue(((TMdbPage *)m_pPageAddr)->m_iPageLSN,iTimeStamp),ERR_SQL_FLUSH_DATA,"InsertIntoQueue failed[%d].",iRet);
            CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoCapture(((TMdbPage *)m_pPageAddr)->m_iPageLSN,iTimeStamp),ERR_SQL_FLUSH_DATA,"InsertIntoCapture failed[%d].",iRet);
        }
        TADD_FUNC("Finish.");
        return iRet;

    }


	//事务  delete + insert,不同步
	int TMdbExecuteEngine::ExecuteUpdateTrans()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_iRowsAffected = 0;
        if(NULL == m_pInsertBlock)
        {
            m_pInsertBlock = new(std::nothrow) char[m_pTable->iOneRecordSize];
            CHECK_OBJ_FILL(m_pInsertBlock);
            TADD_FLOW("Create new Insert Block.size[%d].",m_pTable->iOneRecordSize);
        }


		bool bNext = false;
        while(1)
        {
            
            CHECK_RET_FILL_BREAK(Next(bNext),"Next failed.");//下一个
            if(false == bNext){break;}//结束
            TADD_DETAIL("Find Row[%d].",m_iRowsAffected);
            if(CheckIsStop()== 1)
            {
                //收到信号控制
                CHECK_RET_FILL(ERR_SQL_STOP_EXEC,"Catch the SIGINT signal.");
            }

			//删除
			if(0 != SetDataFlagDelete(m_pDataAddr) ) break;
			//后插入
			memcpy(m_pInsertBlock, m_pDataAddr, m_pTable->iOneRecordSize);
			//复制不需要修改的VarChar和blob
			CHECK_RET_FILL_BREAK(CloneVarChar(m_pInsertBlock, m_pDataAddr, m_pMdbSqlParser->m_listOutputCollist),"CloneVarChar Failed.");
			
			TMdbRowID RowID;
			do
	        {
				CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue error.");				
                CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteSQL(),"ExecuteSQL error.");
				
	            for(int i = 0; i<m_pMdbSqlParser->m_listOutputCollist.iItemNum; i++)
	            {
	                ST_MEM_VALUE * pstMemValue = m_pMdbSqlParser->m_listOutputCollist.pValueArray[i];
	                CHECK_RET_FILL_BREAK(m_tRowCtrl.FillOneColumn(m_pInsertBlock,pstMemValue->pColumnToSet,pstMemValue,TK_UPDATE),"FillOneColumn error");
	            } 
				
				
				CHECK_RET_FILL_BREAK(InsertData(m_pInsertBlock,m_pTable->iOneRecordSize,RowID),"InsertData failed");//插入到内存中				
				m_mdbPageCtrl.UpdatePageLSN();//更新LSN
			}
			while(0);

			TRBRowUnit tRBRowUnit;
			SAFESTRCPY(tRBRowUnit.sTableName,MAX_NAME_LEN,m_pTable->sTableName);
			tRBRowUnit.SQLType = TK_UPDATE;
			tRBRowUnit.iRealRowID = m_tCurRowIDData.m_iRowID;
			tRBRowUnit.iVirtualRowID = RowID.m_iRowID;

			
			CHECK_RET_FILL(m_pLocalLink->AddNewRBRowUnit(&tRBRowUnit),"AddNewRBRowUnit failed.");

            m_iRowsAffected ++;
			
        }
        TADD_FUNC("Finish.");
        return iRet;

    }

	
	int TMdbExecuteEngine::CloneVarChar(char* pDestBlock,char* pSourceBlock, ST_MEM_VALUE_LIST & stMemSkipList )
	{
		int iRet = 0 ;
		for(int i=0;i<m_pTable->iColumnCounts;i++)
		{
			if(m_pTable->tColumn[i].iDataType == DT_Blob ||m_pTable->tColumn[i].iDataType == DT_VarChar)
			{
				bool bSkip = false;
		        for(int j = 0; j< stMemSkipList.iItemNum; j++)
		        {
		        	TMdbColumn * pColumn = stMemSkipList.pValueArray[j]->pColumnToSet;
		            if( 0 == strcmp(pColumn->sName, m_pTable->tColumn[i].sName))
		            {
						bSkip = true;
						//重置varchar记录位
						m_tVarcharCtrl.SetStorgePos(-1, 0, pDestBlock+m_pTable->tColumn[i].iOffSet);
						continue;
					}
		        }

				//跳过需要update的列
				if(bSkip) continue;
		
				do
	            {
	                int iWhichPos = -1;
	                unsigned int iRowId = 0;
					int iOffSet = m_pTable->tColumn[i].iOffSet;
	                m_tVarcharCtrl.GetStoragePos(pSourceBlock+iOffSet, iWhichPos, iRowId);
					TMdbShmDSN* pMdbShmDSN = TMdbShmMgr::GetShmDSN(m_pDsn->sName);
	                TMdbTableSpace * pTablespace = pMdbShmDSN->GetTableSpaceAddrByName(m_pTable->m_sTableSpace);
	                char cStorage = pTablespace->m_bFileStorage?'Y':'N';
	                if(iWhichPos >= VC_16 && iWhichPos <= VC_8192)
	                {
	                	int iLen = m_tVarcharCtrl.GetValueSize(iWhichPos);
	                	char*  pResultValue = new char[iLen];
						CHECK_OBJ(pResultValue);
	                	m_tVarcharCtrl.GetVarcharValue(pResultValue,pSourceBlock+iOffSet);
						
	                    CHECK_RET_NONE(m_tVarcharCtrl.Insert(pResultValue, iWhichPos, iRowId,cStorage),"insert Varchar Faild,ColoumName=[%s],iVarCharlen[%d]",m_pTable->tColumn->sName,strlen(pResultValue));
	                    m_tVarcharCtrl.SetStorgePos(iWhichPos, iRowId, pDestBlock+iOffSet);
						SAFE_DELETE(pResultValue);
	                }
	                else
	                {
						TADD_ERROR(-1,"VarChar Data missing.iWhichPos = %d",iWhichPos);
					}
	                
	            }while(0);
			}
		}
		return iRet;
	}

    /******************************************************************************
    * 函数名称	:  ExecuteInsert
    * 函数描述	:  执行insert操作
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  0 - 成功!0 - 不成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ExecuteInsert()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_RET_FILL(m_pMdbSqlParser->ExecuteSQL(),"ExecuteSQL error.");
        //对于创建的表如果没有索引不允许插入操作
        if(m_pTable->iIndexCounts == 0)
        {
            TADD_ERROR(ERR_TAB_INDEX_NOT_EXIST,"Table[%s] doesn't have any index,Insert operations are not allowed.",m_pTable->sTableName);
            return ERR_TAB_INDEX_NOT_EXIST;
        }
		if(false == m_mdbIndexCtrl.CheckHashConflictIndexFull())
	    {
	        TADD_ERROR(ERR_TAB_NO_CONFLICT_INDEX_NODE,"table[%s] CheckHashConflictIndexFull falied!",m_pTable->sTableName);
	        return ERR_TAB_NO_CONFLICT_INDEX_NODE;
	    }

		//事务模式下，execute的时候不检测主键，而是在commit的时候检测
        if(m_pMdbSqlParser->m_stSqlStruct.bCheckInsertPriKey && !IsUseTrans())
        {
            //检测主键
            TADD_DETAIL("Need to Check Primary Key.");
            CHECK_RET_FILL(IsPKExist(),"IsPKExist failed..");
        }
        if(NULL == m_pInsertBlock)
        {
            m_pInsertBlock = new(std::nothrow) char[m_pTable->iOneRecordSize];
            CHECK_OBJ_FILL(m_pInsertBlock);
            TADD_FLOW("Create new Insert Block.size[%d].",m_pTable->iOneRecordSize);
        }
        memset(m_pInsertBlock,0x00,m_pTable->iOneRecordSize);
        long long iTimeStamp = m_pDsn->iTimeStamp;  

        TMdbRowID  rowID;
        do
        {
            CHECK_RET_FILL_BREAK(InsertDataFill(m_pInsertBlock),"InsertDataFill failed....");//填充数据
            // 设置记录时间戳
            CHECK_RET_FILL_BREAK(SetRowDataTimeStamp(m_pInsertBlock, m_pTable->m_iTimeStampOffset,iTimeStamp),"insert row data timestamp failed.");
            CHECK_RET_FILL_BREAK(InsertData(m_pInsertBlock,m_pTable->iOneRecordSize,rowID),"InsertData failed");//插入到内存中
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue failed.");//填充其他需要填充的信心
        }
        while(0);
        CHECK_RET_FILL(iRet,"ERROR.");

		//回滚模式下不生成同步数据，不计数
		if(IsUseTrans())
		{
			TRBRowUnit tRBRowUnit;
			
			SAFESTRCPY(tRBRowUnit.sTableName,MAX_NAME_LEN,m_pTable->sTableName);			
			tRBRowUnit.SQLType = TK_INSERT;
			tRBRowUnit.iRealRowID = 0;
			tRBRowUnit.iVirtualRowID = rowID.m_iRowID;			
			CHECK_RET_FILL(m_pLocalLink->AddNewRBRowUnit(&tRBRowUnit),"AddNewRBRowUnit failed.");
		}
		else
		{
	        m_pTable->tTableMutex.Lock(m_pTable->bWriteLock,&m_pDsn->tCurTime);
	        ++m_pTable->iCounts;//表记录数增加1
	        m_pTable->tTableMutex.UnLock(m_pTable->bWriteLock);
			
			CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoQueue(((TMdbPage *)m_pPageAddr)->m_iPageLSN,iTimeStamp),ERR_SQL_FLUSH_DATA,"InsertIntoQueue failed[%d].",iRet);
        	CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoCapture(((TMdbPage *)m_pPageAddr)->m_iPageLSN,iTimeStamp),ERR_SQL_FLUSH_DATA,"InsertIntoCapture failed[%d].",iRet);       
		}
		
        ++m_iRowsAffected;
        TADD_FUNC("Finish.");
        return iRet;
    }

	//删除某一条记录,外部调用,非sql调用
	//steps: 删除索引->删除varchar->删除内存
    int TMdbExecuteEngine::ExecuteDelete(char* pPage,char* pDataAddr,TMdbRowID rowID,TMdbShmDSN * pMdbShmDsn,TMdbTable * pTable)
    {		
        TADD_FUNC("Start.");
		int iRet = 0;
		m_pTable = pTable;
		do
		{
			for(int i=0; i<m_pTable->iIndexCounts; ++i)
	        {
	            CHECK_RET_FILL_BREAK(m_mdbIndexCtrl.DeleteIndexNode(i,pDataAddr,m_tRowCtrl,rowID),"DeleteIndexNode[%d] failed.",i);
	        }
			 CHECK_RET_FILL_BREAK(iRet,"CalcIndexValue failed.");
	        //删除varchar数据
	        do
	        {
	            CHECK_RET_FILL_CODE_BREAK(DeleteVarCharValue(pDataAddr),ERR_SQL_INDEX_COLUMN_ERROR,"DeleteVarCharValue error.");
	        }
	        while(0);
	        CHECK_RET_FILL_BREAK(iRet,"iRet = [%d].",iRet);
			CHECK_RET_FILL_BREAK(m_mdbPageCtrl.Attach(pPage, m_pTable->bReadLock, m_pTable->bWriteLock),
										"m_mdbPageCtrl.Attach faild");
			CHECK_RET_FILL(m_mdbPageCtrl.WLock(),"tPageCtrl.WLock() failed.");
			CHECK_RET_FILL_BREAK(m_mdbPageCtrl.DeleteData_NoMutex(rowID.GetDataOffset()),"m_mdbPageCtrl.DeleteData");
			m_mdbPageCtrl.UnWLock();//解页锁
			
			m_pTable->tTableMutex.Lock(m_pTable->bWriteLock, &(pMdbShmDsn->GetInfo()->tCurTime));
            --m_pTable->iCounts; //减少一条记录
            m_pTable->tTableMutex.UnLock(m_pTable->bWriteLock);
		}
		while(0);
		return iRet;
	}

	
    /******************************************************************************
    * 函数名称	:  ExecuteDelete
    * 函数描述	:  执行删除
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ExecuteDelete()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_iRowsAffected = 0;
        int i = 0;
        bool bNext = false;

		bool bUpdateLSN = true;
		if(IsUseTrans())
		{
			bUpdateLSN = false;
		}
		
        while(1)
        {
            CHECK_RET_FILL_BREAK(Next(bNext),"next failed.");
            if(false == bNext){break;}
            TADD_DETAIL("Find Row[%d].",m_iRowsAffected);
            if(CheckIsStop() == true)
            {
                //收到信号控制
                CHECK_RET_FILL_BREAK(ERR_SQL_STOP_EXEC,"Catch the SIGINT signal.");
            }
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputPriKey),"FillSqlParserValue failed.");//填充主键信息
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue failed.");//填充其他需要填充的信心
			//在使用事务的情况下，只设置标志位，不删除数据
			if(IsUseTrans())
            {				
				if(0 != SetDataFlagDelete(m_pDataAddr) ) break;
				
				TRBRowUnit tRBRowUnit;
				
				SAFESTRCPY(tRBRowUnit.sTableName,MAX_NAME_LEN,m_pTable->sTableName);
				tRBRowUnit.SQLType = m_pMdbSqlParser->m_stSqlStruct.iSqlType;
				tRBRowUnit.iRealRowID = m_tCurRowIDData.m_iRowID;
				tRBRowUnit.iVirtualRowID = 0;
				CHECK_RET_FILL(m_pLocalLink->AddNewRBRowUnit(&tRBRowUnit),"AddNewRBRowUnit failed.");
			}
			else
			{
	            //删除索引
	            for(i=0; i<m_pTable->iIndexCounts; ++i)
	            {
	                CHECK_RET_FILL_BREAK(m_mdbIndexCtrl.DeleteIndexNode(i,m_pDataAddr,m_tRowCtrl,m_tCurRowIDData),
	                    "DeleteIndexNode[%d] failed.",i);
	            }
	            CHECK_RET_FILL_BREAK(iRet,"CalcIndexValue failed.");
	            //删除varchar数据
	            do
	            {
	                CHECK_RET_FILL_CODE_BREAK(DeleteVarCharValue(m_pDataAddr),ERR_SQL_INDEX_COLUMN_ERROR,"DeleteVarCharValue error.");
	            }
	            while(0);
	            CHECK_RET_FILL_BREAK(iRet,"iRet = [%d].",iRet);
	            //删除内存中的数据
	            TADD_DETAIL("Delete From Page[%d]",m_iPagePos);
	            CHECK_RET_FILL_BREAK(m_mdbPageCtrl.Attach(m_pPageAddr, m_pTable->bReadLock, m_pTable->bWriteLock),
	                            "m_mdbPageCtrl.Attach faild");
	            CHECK_RET_FILL(m_mdbPageCtrl.WLock(),"tPageCtrl.WLock() failed.");
	            CHECK_RET_FILL_BREAK(m_mdbPageCtrl.DeleteData_NoMutex(m_iPagePos, bUpdateLSN),"m_mdbPageCtrl.DeleteData");
	            m_mdbTSCtrl.SetPageDirtyFlag(((TMdbPage*)m_pPageAddr)->m_iPageID);//设置脏页
	            m_mdbPageCtrl.UnWLock();//解页锁
	            //调节表中的页面链
	            TMdbPage *pPage = (TMdbPage*)m_pPageAddr;
	            if(pPage->bNeedToMoveToFreeList() && TMdbNtcStrFunc::StrNoCaseCmp(pPage->m_sState, "full") == 0)
	            {
	                //如果数据删除之后,空间足够大(可用空间>=页面大小的25%)，则进入Free-Page链表
	                CHECK_RET_FILL_BREAK(m_mdbTSCtrl.TablePageFullToFree(m_pTable,pPage),"TablePageFullToFree error");
	            }
	            m_pTable->tTableMutex.Lock(m_pTable->bWriteLock, &m_pDsn->tCurTime);
	            --m_pTable->iCounts; //减少一条记录
	            m_pTable->tTableMutex.UnLock(m_pTable->bWriteLock);
				
				CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoQueue(((TMdbPage *)m_pPageAddr)->m_iPageLSN,TMdbDateTime::StringToTime(m_pDsn->sCurTime)),ERR_SQL_FLUSH_DATA,"InsertIntoQueue failed[%d].",iRet);
            	CHECK_RET_FILL_CODE(m_tMdbFlush.InsertIntoCapture(((TMdbPage *)m_pPageAddr)->m_iPageLSN,TMdbDateTime::StringToTime(m_pDsn->sCurTime)),ERR_SQL_FLUSH_DATA,"InsertIntoCapture failed[%d].",iRet);
        
			}
            m_iRowsAffected ++;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  InsertDataFill
    * 函数描述	:  对于插入的值按内存分布进行填充
    * 输入		:  无
    * 输出		:  sTmp - 待填充的内存空间
    * 返回值	:  0 - 成功!0 - 不成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::InsertDataFill(char* const &sTmp)
    {
        TADD_FUNC("Start.sTmp=[%p].",sTmp);
        int iRet = 0;
        int i    = 0;
        int iSize = m_pMdbSqlParser->m_listOutputCollist.iItemNum;
        TADD_DETAIL("Column counts[%d].",iSize);
        ST_MEM_VALUE ** pMemValueArray = m_pMdbSqlParser->m_listOutputCollist.pValueArray;
        for(i = 0; i < iSize; i++)
        {
            CHECK_RET_FILL(m_tRowCtrl.FillOneColumn(sTmp,pMemValueArray[i]->pColumnToSet,pMemValueArray[i],TK_INSERT),
                        "FillOneColumn[%s] error,value=[%s]",pMemValueArray[i]->pColumnToSet->sName,pMemValueArray[i]->ToString().c_str());
        }

        
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  InsertData
    * 函数描述	:  将数据插入到内存中
    插入数据可以直接把表锁住, 因为肯定是要更新索引/获取自由页/更新表信息，这些都需要加锁

    * 输入		:  pAddr - 数据地址，iSize - 数据大小
    * 输出		:  无
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::InsertData(char* pAddr, int iSize,TMdbRowID&  rowID)
    {
        TADD_FUNC("(iSize=%d) : Start.", iSize);

		bool bUpdateLSN = true;
		if(IsUseTrans())
		{
			bUpdateLSN = false;
		}

        int iRet = 0;
        while(true)
        {
            if(CheckIsStop() == true)
            {
                CHECK_RET_FILL(ERR_SQL_FILL_MDB_INFO,"Catch the SIGINT signal.");
            }
            TMdbPage * pFreePage= NULL;
            CHECK_RET_FILL(m_mdbTSCtrl.GetFreePage(m_pTable,pFreePage),"GetFreePage failed.");//找到一个自由页
            //把数据放入页面中
            CHECK_RET_FILL_CODE(m_mdbPageCtrl.Attach((char *)pFreePage, m_pTable->bReadLock, m_pTable->bWriteLock),
                           ERR_OS_ATTACH_SHM,"Can't Attach to page.");
            CHECK_RET_FILL(m_mdbPageCtrl.WLock(),"tPageCtrl.WLock() failed.");
            iRet = m_mdbPageCtrl.InsertData_NoMutex((unsigned char*)pAddr, iSize, rowID,m_pDataAddr, bUpdateLSN);
            if(iRet == 0)
            {
                m_mdbTSCtrl.SetPageDirtyFlag(pFreePage->m_iPageID);
				InitDataMutex(m_pDataAddr);
				SetDataFlagInsert(m_pDataAddr);
            }
            m_mdbPageCtrl.UnWLock();//解页锁
            TADD_DETAIL("Get one record:DataAddr[%p],CurRowData[%d|%d],PageAddr[%p],PagePos[%d].",
            m_pDataAddr,rowID.GetPageID(),rowID.GetDataOffset(),
            (char*)pFreePage,rowID.GetDataOffset());

            if(ERR_PAGE_NO_MEMORY == iRet)
            {
                //对于找到的自由页满了而无法插入数据，则找下一个自由页
                TADD_DETAIL("Current page is Full.");
                CHECK_RET_FILL_CODE(m_mdbTSCtrl.TablePageFreeToFull(m_pTable,pFreePage),
                               ERR_OS_NO_MEMROY,"FreeToFull() error.iRet=[%d]",iRet);
                continue;
            }
            m_pPageAddr = (char *)pFreePage;
            break;
        }
        TADD_DETAIL("InsertData(%s) : Data insert OK,rowID=%p .",m_pTable->sTableName,&rowID);
        CHECK_RET_FILL(ChangeInsertIndex(m_pDataAddr, rowID),"ChangeInsertIndex failed...");
        TADD_FUNC("Finish.");
        return 0;
    }


    /******************************************************************************
    * 函数名称	:  UpdateData
    * 函数描述	:  将数据更新到
    * 输入		:
    * 输入		:
    * 输出		:  无
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::UpdateData()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //先更新索引
        CHECK_RET_FILL(ChangeUpdateIndex(m_pMdbSqlParser->m_stSqlStruct.vIndexChanged),"ChangeUpdateIndex failed.");
        //CHECK_RET_FILL_CODE(m_mdbPageCtrl.Attach(m_pPageAddr, m_pTable->bReadLock, m_pTable->bWriteLock),ERR_OS_ATTACH_SHM,"tPageCtrl.Attach() failed.");
        //CHECK_RET_FILL(m_mdbPageCtrl.WLock(),"tPageCtrl.WLock() failed.");

        do
        {
            int i = 0;
            for(i = 0; i<m_pMdbSqlParser->m_listOutputCollist.iItemNum; i++)
            {
                ST_MEM_VALUE * pstMemValue = m_pMdbSqlParser->m_listOutputCollist.pValueArray[i];
                CHECK_RET_FILL_BREAK(m_tRowCtrl.FillOneColumn(m_pDataAddr,pstMemValue->pColumnToSet,pstMemValue,TK_UPDATE),"FillOneColumn error");
            }

            
        }
        while(0);
        m_mdbPageCtrl.UpdatePageLSN();//更新LSN
        //m_mdbPageCtrl.UnWLock();
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  ChangeInsertIndex
    * 函数描述	:  调整索引(插入时)
    * 输入		:  pAddr - 数据地址，插入数据的rowid
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ChangeInsertIndex( char* pAddr, TMdbRowID& rowID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //调整索引信息
        for(int i=0; i<m_pTable->iIndexCounts; ++i)
        {
            TADD_DETAIL("rowID.iPageID=%d, rowID.iDataOffset=%d", rowID.GetPageID(), rowID.GetDataOffset());
            CHECK_RET_FILL(m_mdbIndexCtrl.InsertIndexNode(i,pAddr,m_tRowCtrl,rowID),
                "InsertIndexNode failed.index=[%s]",m_pTable->tIndex[i].sName);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  ChangeUpdateIndex
    * 函数描述	:  调整索引(插入时)
    * 输入		:  vMemValue - update中set的值,只需要更新update涉及到的索引列就行了
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ChangeUpdateIndex(std::vector<ST_INDEX_VALUE > & vUpdateIndex)
    {
        TADD_FUNC("Start.vUpdateIndex.size=%d.",vUpdateIndex.size());
        int iRet = 0;
        std::vector<ST_INDEX_VALUE >::iterator itor = vUpdateIndex.begin();
        for(; itor != vUpdateIndex.end(); ++itor)
        {
            ST_INDEX_VALUE & stIndexValue = (*itor);
            do
            {
                CHECK_RET_FILL_CODE_BREAK(m_mdbIndexCtrl.UpdateIndexNode(stIndexValue.pstTableIndex->iIndexPos,
                                     m_pDataAddr,stIndexValue, m_tRowCtrl,m_tCurRowIDData),ERR_SQL_INDEX_COLUMN_ERROR,"UpdateIndexNode error[%d].",iRet);
            }
            while(0);
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  CalcMPIndexValue
    * 函数描述	:  计算组合索引的值
    * 输入		:  pAddr - 记录的地址，iIndexPos - index的位置
    * 输出		:  iError - 错误码
    * 返回值	:  计算结果
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*long long TMdbExecuteEngine::CalcMPIndexValue( char* pAddr, int iIndexPos, int& iError)
    {
        TADD_FUNC("Start(pAddr=%p,iIndexPos=%d).",pAddr,iIndexPos);
        iError = 0;
        //首先找到本索引对应的数据
        m_sTempValue[0] = 0;
        for(int i=0; i<MAX_INDEX_COLUMN_COUNTS; ++i)
        {
            if(m_pTable->tIndex[iIndexPos].iColumnNo[i] < 0)
            {
                break;
            }
            //获取索引列所在数据的偏移量
            int iCol = m_pTable->tIndex[iIndexPos].iColumnNo[i];
            long long llValue = 0;
            char * sValue = NULL;
            int iValueType = 0;
            iError = m_tRowCtrl.GetOneColumnValue(pAddr,&(m_pTable->tColumn[iCol]),llValue,sValue,-1,iValueType);
            if(iError != 0)
            {
                TADD_ERROR(iError,"GetOneColumnValue failed.");
                return 0;
            }
            switch(iValueType)
            {
                case MEM_Int:
                    sprintf(m_sTempValue,"%s;%lld", m_sTempValue, llValue);
                    break;
                case MEM_Str:
                    sprintf(m_sTempValue,"%s;%s", m_sTempValue, sValue);
                    break;
                case MEM_Null:
                    sprintf(m_sTempValue,"%s;0", m_sTempValue);
                    break;
                default:
                    TADD_ERROR(ERR_SQL_TYPE_INVALID,"iValueType=%d invalid.", iValueType);
                    iError = ERR_SQL_TYPE_INVALID;
                    break;
            }
            TADD_DETAIL("sTempValue=%s.", m_sTempValue);
        }
        long long llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
        TADD_FUNC("Finish(iIndexValue=%lld).", llValue);
        return llValue;
    }
    */


    /******************************************************************************
    * 函数名称	:  CalcOneIndexValue
    * 函数描述	:  计算单个索引的值
    * 输入		:  pAddr - 记录的地址，iIndexPos - index的位置
    * 输出		:  iError - 错误码
    * 返回值	:  计算结果
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*long long TMdbExecuteEngine::CalcOneIndexValue( char* pAddr, int iIndexPos, int& iError)
    {
        TADD_FUNC("Start(iIndexPos=%d).", iIndexPos);
        iError = 0;
        int iCol = m_pTable->tIndex[iIndexPos].iColumnNo[0];
        TMdbColumn * pColumn = &(m_pTable->tColumn[iCol]);
        TADD_DETAIL("ColoumName=[%s], offset=%d,iCol=%d",pColumn->sName,pColumn->iOffSet,iCol);
        long long llValue = 0;
        char * sValue = 0;
        int iValueType = 0;
        iError = m_tRowCtrl.GetOneColumnValue(pAddr,pColumn,llValue,sValue,-1,iValueType);
        if(iError != 0)
        {
            TADD_ERROR(iError,"GetOneColumnValue[%s] failed.",pColumn->sName);
            return 0;
        }
        switch(iValueType)
        {
            case MEM_Int://直接用获取的数值
                break;
            case MEM_Str:
                llValue = TMdbNtcStrFunc::StrToHash(sValue);
                break;
            case MEM_Null:
                llValue = 0;
                break;
            default:
                TADD_ERROR(ERR_SQL_TYPE_INVALID,"iValueType=%d invalid.", iValueType);
                iError = ERR_SQL_TYPE_INVALID;
                break;
        }
        TADD_FUNC("Finish(iIndexValue=%lld).", llValue);
        return llValue;
    }
    */

    /******************************************************************************
    * 函数名称	:  CalcIndexValue
    * 函数描述	:  计算索引计算的值
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*long long TMdbExecuteEngine::CalcIndexValue( char* pAddr, int iIndexPos, int& iError)
    {
        TADD_FUNC("Start(pAddr=%p,iIndexPos=%d).",pAddr,iIndexPos);
        iError = 0;
        long long llValue = 0;
        if(m_pTable->tIndex[iIndexPos].m_iIndexType == HT_CMP)
        {
            llValue = CalcMPIndexValue(pAddr, iIndexPos, iError);
        }
        else
        {
            llValue = CalcOneIndexValue(pAddr, iIndexPos, iError);
        }
        llValue = llValue<0? -llValue:llValue;
        llValue = llValue % m_pTable->iRecordCounts;
        TADD_FUNC("Finish(iIndexValue=%lld).", llValue);
        return llValue;
    }*/

    /******************************************************************************
    * 函数名称	:  Next
    * 函数描述	:  获取下一条数据
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  true - 下一条有值，false - 没有下一条了
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::Next(bool & bResult)
    {
        TADD_FUNC("Start.next_type=[%d].",m_iNextType);
        int iRet = 0;
        bool bLoop = false;
        do
        {
            bLoop = false;
            switch(m_iNextType)
            {
            case NEXT_NORMAL:
                CHECK_RET(NextWhere(bResult),"NextWhere failed.");
                if(bResult){m_pMdbSqlParser->m_stSqlStruct.iRowPos ++;}
                break;
             #if 0
            case NEXT_MONI:
                bResult =  MoniNext();
                break;
            case NEXT_ORDERBY:
                bResult = OrderbyNext();
                break;
            #endif
            case NEXT_CACHE:
                {
                    bResult = CacheNext();
                }
                break;
            default:
                TADD_ERROR(ERROR_UNKNOWN,"error next type[%d].",m_iNextType);
                break;
            }
            switch(m_tLimitCtrl.DoNext())
            {
            case DO_NEXT_TRUE:
                bLoop = false;
                break;
            case DO_NEXT_CONTINUE:
                bLoop = true;
                break;
            case DO_NEXT_END:
                bLoop = false;
                bResult =  false;
                break;
            default:
                break;
            }
        }
        while(bLoop);
        TADD_FUNC("Finish.iRet[%d],result[%d].",iRet,bResult);
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  GetNextIndex
    * 函数描述	:  获取下一个索引
    * 输入		:  无
    * 输出		:  pTableIndex - 索引信息，llValue - 索引值信息
    * 返回值	:   0 - 成功!0 - 不成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::GetNextIndex(ST_TABLE_INDEX_INFO * & pTableIndex,long long & llValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(m_bScanAll)
        {
            if(NULL == m_pMdbSqlParser->m_stSqlStruct.pScanAllIndex)
            {
                pTableIndex = NULL;
                return iRet;
            }
        
            // 全量遍历
            m_llScanAllPos ++;
            llValue = m_llScanAllPos;
            long long iMaxPos = 0;
            if(m_pMdbSqlParser->m_stSqlStruct.pScanAllIndex->pIndexInfo->m_iAlgoType == INDEX_HASH)
            {
                iMaxPos = m_pTable->iRecordCounts;
            }
            else if(m_pMdbSqlParser->m_stSqlStruct.pScanAllIndex->pIndexInfo->m_iAlgoType == INDEX_M_HASH)
            {
                iMaxPos = m_pTable->iTabLevelCnts;
            }
            else
            {
                iMaxPos = m_pTable->iRecordCounts;
            }
            
            if(m_llScanAllPos == 0 || m_llScanAllPos < iMaxPos)
            {
                pTableIndex = m_pMdbSqlParser->m_stSqlStruct.pScanAllIndex;
            }
            else
            {
                pTableIndex = NULL;
            }
        }
        else
        {
            //按索引遍历
            if((int)m_pVIndex->size() - 1 == m_iCurIndex || m_iCurIndex < -1)
            {
                pTableIndex = NULL;
            }
            else
            {
                m_iCurIndex ++;
                pTableIndex = (*m_pVIndex)[m_iCurIndex].pstTableIndex;
                CalcMemValueHash((*m_pVIndex)[m_iCurIndex],llValue);
				CHECK_RET(SetTrieWord((*m_pVIndex)[m_iCurIndex]),"SetTrieWord Failed.");
            }
        }
        if(NULL != pTableIndex)
        {
            TADD_DETAIL("pTableIndex=[%p][%s],llValue=[%lld]",pTableIndex,pTableIndex->pIndexInfo->sName,llValue);
        }
        else
        {
            TADD_DETAIL("pTableIndex=NULL,llValue=[%lld]",llValue);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

	
    int TMdbExecuteEngine::SetTrieWord(ST_INDEX_VALUE&  stIndexValue)
   	{
   		int  iRet = 0;
		m_MdbTableWalker.m_sTrieWord[0] = 0;
		
		if(stIndexValue.pstTableIndex->pIndexInfo->m_iAlgoType == INDEX_TRIE && \
			stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType == HT_Char ) 
		{
			SAFESTRCPY(m_MdbTableWalker.m_sTrieWord, MAX_TRIE_WORD_LEN ,stIndexValue.pExprArr[0]->pExprValue->sValue);			
		}

		return iRet;		
	}
	

    /******************************************************************************
    * 函数名称	:  CalcMemValueHash
    * 函数描述	:  计算value的hash值
    * 输入		:  pMemValue - 计算的节点
    * 输出		:   llValue:返回的hash值
    * 返回值	:  0 - 成功!0 - 不成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbExecuteEngine::CalcMemValueHash(ST_INDEX_VALUE & stIndexValue,long long & llValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_sTempValue[0] = 0;
        if(HT_CMP ==  stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType)
        {
            //组合索引
            int i = 0;
            for(i = 0; i < MAX_INDEX_COLUMN_COUNTS; ++i)
            {
                if(stIndexValue.pstTableIndex->pIndexInfo->iColumnNo[i] < 0)
                {
                    break;
                }
                CHECK_OBJ_FILL(stIndexValue.pExprArr[i]);
                ST_MEM_VALUE * pstMemValue = stIndexValue.pExprArr[i]->pExprValue;
                CHECK_OBJ_FILL(pstMemValue);
                if(pstMemValue->IsNull())
                {
                    sprintf(m_sTempValue, "%s;0",m_sTempValue);
                }
                else if(MemValueHasProperty(pstMemValue,MEM_Int))
                {
                    sprintf(m_sTempValue, "%s;%lld", m_sTempValue, pstMemValue->lValue);
                }
                else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
                {
                    sprintf(m_sTempValue, "%s;%s", m_sTempValue, pstMemValue->sValue);
                }
            }
            llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
        }
        else
        {
            //单索引
            if(stIndexValue.pExprArr[0]->pExprValue->IsNull())
            {
                llValue = 0;
            }
            else  if(MemValueHasAnyProperty(stIndexValue.pExprArr[0]->pExprValue,MEM_Str|MEM_Date))
            {
                llValue = TMdbNtcStrFunc::StrToHash(stIndexValue.pExprArr[0]->pExprValue->sValue);
            }
            else if(MemValueHasProperty(stIndexValue.pExprArr[0]->pExprValue,MEM_Int))
            {
                llValue = stIndexValue.pExprArr[0]->pExprValue->lValue;
            }
        }
        llValue = llValue<0? -llValue:llValue;
        //进行散列
        llValue = llValue % m_pTable->iRecordCounts;//m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iRecordCounts;
        TADD_FUNC("Finish.llValue = %lld.m_sTempValue=[%s].",llValue,m_sTempValue);
        return iRet;
    }
    */

    /******************************************************************************
    * 函数名称	:  CalcMemValueHash
    * 函数描述	:  计算value的hash值
    * 输入		:  pMemValue - 计算的节点
    * 输出		:   llValue:返回的hash值
    * 返回值	:  0 - 成功!0 - 不成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    /*int TMdbExecuteEngine::CalcMemValueListHash(ST_MEM_VALUE_LIST & stMemValueList,long long & llValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(stMemValueList.iItemNum > 1)
        {
            //组合索引
            m_sTempValue[0] = 0;
            int i = 0;
            for(i = 0; i < stMemValueList.iItemNum; i++)
            {
                ST_MEM_VALUE * pstMemValue = stMemValueList.pValueArray[i];
                if(pstMemValue != NULL)
                {
                    if(pstMemValue->IsNull())
                    {
                        sprintf(m_sTempValue, "%s;0", m_sTempValue);
                    }
                    else if(MemValueHasProperty(pstMemValue,MEM_Int))
                    {
                        sprintf(m_sTempValue, "%s;%lld", m_sTempValue, pstMemValue->lValue);
                    }
                    else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
                    {
                        sprintf(m_sTempValue, "%s;%s", m_sTempValue, pstMemValue->sValue);
                    }

                }
            }
            llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
            TADD_DETAIL("CMP_INDEX:m_sTempValue=[%s],llValue=[%lld]",m_sTempValue,llValue );
        }
        else
        {
            //单索引
            ST_MEM_VALUE * pstMemValue = stMemValueList.pValueArray[0];
            if(pstMemValue->IsNull())
            {
                llValue = 0;
            }
            else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
            {
                llValue = TMdbNtcStrFunc::StrToHash(pstMemValue->sValue);
            }
            else if(MemValueHasProperty(pstMemValue,MEM_Int))
            {
                llValue = pstMemValue->lValue;
            }
        }
        llValue = llValue<0? -llValue:llValue;
        //进行散列
        llValue = llValue % m_pTable->iRecordCounts;//m_pMdbSqlParser->m_stSqlStruct.pMdbTable->iRecordCounts;
        TADD_FUNC("Finish.llValue=%lld.",llValue);
        return iRet;
    }
    */

    /******************************************************************************
    * 函数名称	:  CheckWhere
    * 函数描述	:  检测where条件是否满足
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  true - 满足，false - 不满足
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::CheckWhere(bool &bResult)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

		bool bVisible = true;
		CHECK_RET(CheckVisible(bVisible),"CheckVisible failed");
		if(bVisible==false) 
		{
			bResult = false;
			return iRet;
		}
		
        CHECK_RET(FillSqlParserValue(m_pMdbSqlParser->m_listInputWhere),"FillSqlParserValue where failed.");
        CHECK_RET(m_pMdbSqlParser->ExecuteWhere(bResult),"ExecuteWhere failed.");
        TADD_FUNC("Finish(%s).", bResult?"TRUE":"FALSE");
        return iRet;
    }

    int TMdbExecuteEngine::CheckVisible(bool &bVisible)
    {
    	int iRet = 0;

    	if(!m_pDataAddr)
    	{
			return false;
		}
		TMdbPageNode* pNode = (TMdbPageNode*)m_pDataAddr - 1;

		//共享的数据
		if(0 == pNode->iSessionID)
		{
			bVisible = true;	
		}
		else if(pNode->iSessionID == m_pLocalLink->iSessionID)
		{
			TADD_NORMAL(" Node:%d | SQL:%d \n",pNode->iExecuteID, m_pLocalLink->iExecuteID);
			//自己看不到自己删除的数据
			if( pNode->cFlag & DATA_DELETE)
			{
				bVisible = false;
			}  
			else if(pNode->iExecuteID == m_pLocalLink->iExecuteID) //单次执行内，数据互相隔离
			{
				bVisible = false;
			}
			else
			{
				bVisible = true;
			}	
		}
		else
		{
			//其他链接的数据看不到
			if( pNode->cFlag & DATA_VIRTUAL)
			{
				bVisible = false;
			}	
			else
			{
				bVisible = true;
			}
		}
		return iRet;
	}
    /******************************************************************************
    * 函数名称	:  FillSqlParserValue
    * 函数描述	:  填充值sql解析器中需要填充的值
    * 输入		:  vMemValue - 待填充的值
    * 输出		:  vMemValue - 填充好后的值
    * 返回值	:  0 - 成功!0 - 不成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::FillSqlParserValue(ST_MEM_VALUE_LIST & stMemValueList)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int i = 0;
        m_pMdbSqlParser->ClearMemValue(stMemValueList);//清理
        int iSize = stMemValueList.iItemNum;
        TMdbColumn * pColumn = NULL;
        ST_MEM_VALUE * pMemValue = NULL;
        int iValueType = 0;
        for(i = 0; i<iSize; i++)
        {
            if(NULL == m_pDataAddr){continue;}
            pMemValue = stMemValueList.pValueArray[i];
            pColumn = pMemValue->pColumn;
            TADD_DETAIL("Column[%s],type[%d],offset[%d].",pColumn->sName,pColumn->iDataType,pColumn->iOffSet);
            CHECK_RET(m_tRowCtrl.GetOneColumnValue(m_pDataAddr,pColumn,pMemValue->lValue,pMemValue->sValue,
                pMemValue->iSize,iValueType),"GetOneColumnValue failed.");
            switch(iValueType)
            {
                case MEM_Int:
                case MEM_Str:
                    break;
                case MEM_Null:
                    pMemValue->SetNull();
                    break;
                default:
                    CHECK_RET_FILL(ERR_SQL_TYPE_INVALID,"column[%s] iValueType=%d invalid.",pColumn->sName, iValueType);
                    break;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    
    int TMdbExecuteEngine::FillFieldForCSBin(NoOcpParse &tParseData,bool bFirst)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        /*
        ST_MEM_VALUE_LIST & stMemValueList = m_pMdbSqlParser->m_listInputCollist;
        //m_pMdbSqlParser->ClearMemValue(stMemValueList);//清理
        int iSize = stMemValueList.iItemNum;
        int iValueLen = 0;
        char cDataType = 0;
        TMdbColumn * pColumn = NULL;
        ST_MEM_VALUE * pMemValue = NULL;
        for(int i = 0; i<iSize; i++)
        {
            if(NULL == m_pDataAddr){continue;}
            pMemValue = stMemValueList.pValueArray[i];
            pColumn = pMemValue->pColumn;
            if(bFirst)
            {
                int iLen = strlen(pMemValue->sAlias)+1;
                cDataType = pColumn->iDataType;
                tParseData.SetData(&cDataType,sizeof(cDataType));//name len
                tParseData.SetData(&iLen,sizeof(int));//name len
                tParseData.SetData(pMemValue->sAlias,iLen);//name value
                
            }
            if(m_tRowCtrl.IsColumnNull(pColumn,m_pDataAddr))
            {//空值
                iValueLen = 0;
                tParseData.SetData(&iValueLen,sizeof(int));
                continue;
            }
            switch(pColumn->iDataType)
            {
            case DT_Int:  //Integer
            case DT_Char://Char
            case DT_DateStamp: //时间用string保存
            {
                
                tParseData.SetData(&pColumn->iColumnLen,sizeof(int));
                tParseData.SetData(&m_pDataAddr[pColumn->iOffSet],pColumn->iColumnLen);
                break;
            }
            case DT_VarChar:  //VarChar
            {
                char *pAddr = NULL;
                CHECK_RET_FILL_CODE(m_tVarcharCtrl.GetVarcharValue(m_pDataAddr+pColumn->iOffSet,iValueLen,pAddr),
                               ERR_SQL_GET_MEMORY_VALUE_ERROR,"GetVarcharValue ERROR");
                tParseData.SetData(&iValueLen,sizeof(int));
                tParseData.SetData(pAddr,iValueLen);
                               
                //TADD_FLOW("Column[%d] value=[%s],col_pos=[%d]",i,(char *)(pStructAddr+Column[i]),Column[i]);
                break;
            }
            
            case DT_Blob:
            {
                //#if 0
                char *pAddr = NULL;
                CHECK_RET_FILL_CODE(m_tVarcharCtrl.GetVarcharValue(m_pDataAddr+pColumn->iOffSet,iValueLen,pAddr),
                               ERR_SQL_GET_MEMORY_VALUE_ERROR,"GetVarcharValue ERROR");
                //TADD_FLOW("Column[%d] value=[%s],col_pos=[%d]",i,(char *)(pStructAddr+Column[i]),Column[i]);
                //Base64解码
                std::string encoded = pAddr;
                std::string decoded = Base::base64_decode(encoded);
                iValueLen = decoded.length();
                tParseData.SetData(&iValueLen,sizeof(int));
                tParseData.SetData(decoded.c_str(),decoded.length());
                //#endif
                break;
            }
            
            
            default:
                CHECK_RET_FILL(ERR_SQL_TYPE_INVALID,"column[%s]DataType=%d invalid.", pColumn->sName,pColumn->iDataType);
                break;
                
            }
            TADD_DETAIL("Column[%s],type[%d],offset[%d].",pColumn->sName,pColumn->iDataType,pColumn->iOffSet);
        }
        TADD_FUNC("Finish.");
        */
        return iRet;
    }
    //删除变长数据
    int TMdbExecuteEngine::DeleteVarCharValue( char*  const &pAddr)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //判断是否有变长存储
        for(int i=0; i<m_pTable->iColumnCounts; ++i)
        {
            if(m_pTable->tColumn[i].iDataType == DT_VarChar || m_pTable->tColumn[i].iDataType == DT_Blob)
            {
                int iOffSet = m_pTable->tColumn[i].iOffSet;
                int iWhichPos = -1;
                unsigned int iRowId = 0;
                m_tVarcharCtrl.GetStoragePos(&(pAddr[iOffSet]), iWhichPos, iRowId);
                if(iWhichPos < VC_16 || iWhichPos > VC_8192)
                {
                    continue;
                }
                CHECK_RET(m_tVarcharCtrl.Delete(iWhichPos,iRowId),"Delete Varchar Faild");
            }
        }
        TADD_FUNC("End");
        return iRet;
    }



    /******************************************************************************
    * 函数名称	:  IsNeedReadLock
    * 函数描述	:  是否需要加读锁:走索引并且加读锁时，使用cache方式 next
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool  TMdbExecuteEngine::IsNeedReadLock()
    {
        return (m_pVIndex->size() != 0 && m_pTable->bReadLock);
    }



    /******************************************************************************
    * 函数名称	:  ExecuteMySelect
    * 函数描述	:  执行查询缓存
    				加锁查询，并	将查询结果缓存起来，
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ExecuteCacheSelect()
    {
       TADD_FUNC("Start.");
        int iRet = 0;
        if(IsNeedReadLock())
        {
            m_pTable->tTableMutex.Lock(m_pTable->bReadLock,&m_pDsn->tCurTime);//加表锁
        }
        bool bNext = false;
        TMdbCacheRow * pCacheRow = NULL;

		
		if(m_pMdbSqlParser->m_listOutputGroupBy.iItemNum != 0)
		{
			iRet =m_tCacheTable.CreateHashTable();
			CHECK_RET(iRet,"Create hash table failed");
			
		}
		
        while(1)
        {
            CHECK_RET_FILL_BREAK(Next(bNext),"next failed.");
            if(false == bNext){break;}
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputOrderby),"Fill Order by error...");
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputGroupBy),"Fill Group by error...");
            CHECK_RET_FILL_BREAK(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue error");
            CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteOrderby(),"ExecuteOrderby error...");
            CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteGroupby(),"ExecuteGroupby error...");
            CHECK_RET_FILL_BREAK(m_tCacheTable.GetAggValue(m_pDataAddr,pCacheRow),"GetAggValue error");
            CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteSQL(),"ExecuteSQL failed.");
            CHECK_RET_FILL_BREAK(m_pMdbSqlParser->ExecuteHaving(),"ExecuteHaving failed.");//计算having值
            CHECK_RET_FILL_BREAK(m_tCacheTable.SetAggValue(m_pDataAddr,pCacheRow),"SetAggValue failed.");
        }
        if(IsNeedReadLock())
        {
            m_pTable->tTableMutex.UnLock(m_pTable->bReadLock);//加表锁
        }
		
		
		if(m_pMdbSqlParser->m_listOutputGroupBy.iItemNum != 0)
			m_tCacheTable.DestroyHashTable();
    	//m_tCacheTable.FinishInsert();
        m_tCacheTable.FinishInsert(m_pMdbSqlParser->m_listOutputLimit);
		
        m_iNextType = NEXT_CACHE;
        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  FillCollist
    * 函数描述	:  填充列值
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::FillCollist()
    {
        TADD_FUNC("Start.m_iNextType=[%d].",m_iNextType);
        int iRet = 0;
        CHECK_RET_FILL(FillSqlParserValue(m_pMdbSqlParser->m_listInputCollist),"FillSqlParserValue error");
        CHECK_RET_FILL(m_pMdbSqlParser->ExecuteSQL(), "%s", m_pMdbSqlParser->m_tError.GetErrMsg());
        TADD_FUNC("Finish.");
        return iRet;
    }


    /******************************************************************************
    * 函数名称	:  CacheNext
    * 函数描述	:  缓存next
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool TMdbExecuteEngine::CacheNext()
    {
         if(m_tCacheTable.Next(m_pDataAddr))
         {
            return true;
         }
         else
         {
            return false;
         }
    }


    /******************************************************************************
    * 函数名称	:  NextWhere
    * 函数描述	:  符合where条件的记录
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::NextWhere(bool & bResult)
    {
        int iRet = 0;

		if(m_bScanAll)
		{
			CHECK_RET(NextWhereByPage(bResult),"NextWhereByPage Failed");
		}
		else
		{
			CHECK_RET(NextWhereByIndex(bResult),"NextWhereByIndex Failed");
		}
       
        return iRet;
    }

	
    int TMdbExecuteEngine::NextWhereByPage(bool & bResult)
    {
		TADD_FUNC("Start.");
		int iRet = 0;
		if(m_MdbTableWalker.m_iScanPageType == TYPE_SCAN_PAGE_END)
		{
			bResult = false;
			return iRet;
		}

		//由于删除过程中页面会从full迁移到free，所以先遍历free		
		//遍历free链
		if(m_MdbTableWalker.m_iScanPageType == TYPE_SCAN_PAGE_FREE)
		{			
			if(NULL == m_MdbTableWalker.m_pCurPage)
			{
				//需要预先保存iFullPageID和iFreePageID，因为删除过程中会变化
				m_MdbTableWalker.m_iFirstFullPageID = m_pTable->iFullPageID;
				m_MdbTableWalker.m_iFirstFreePageID = m_pTable->iFreePageID;
				
	        	m_MdbTableWalker.WalkByPage(m_MdbTableWalker.m_iFirstFreePageID);
			}
	        while(m_MdbTableWalker.NextByPage())
	        {
	            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
	            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
	            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
	            m_iPagePos      = m_MdbTableWalker.GetPagePos();

				CHECK_RET(CheckWhere(bResult),"CheckWhere failed.");
				if(bResult)
				{
					m_MdbTableWalker.m_iAffect ++;
				   TADD_FLOW("where condition is satisfy.");				   
				   return iRet;
				   
				}
				else
				{
				  TADD_FLOW("where condition is not satisfy.");
				  continue;
				}
	        }
			//TADD_NORMAL("Scan Free End,Swicth to Full,Affect = %d\n",m_MdbTableWalker.m_iAffect);
			m_MdbTableWalker.m_iScanPageType = TYPE_SCAN_PAGE_FULL;
			m_MdbTableWalker.m_pCurPage = NULL;
			m_MdbTableWalker.m_pStartPage = NULL;
			m_MdbTableWalker.m_pNextPage = NULL;
			bResult = false;
		}

		//遍历full 链
		if(m_MdbTableWalker.m_iScanPageType == TYPE_SCAN_PAGE_FULL)
		{
			if(NULL == m_MdbTableWalker.m_pCurPage)
			{
	        	m_MdbTableWalker.WalkByPage(m_MdbTableWalker.m_iFirstFullPageID);
			}
			while(m_MdbTableWalker.NextByPage())
	        {
	            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
	            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
	            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
	            m_iPagePos      = m_MdbTableWalker.GetPagePos();

				CHECK_RET(CheckWhere(bResult),"CheckWhere failed.");
				if(bResult)
				{
					m_MdbTableWalker.m_iAffect ++;
				   TADD_FLOW("where condition is satisfy.");				   
				   return iRet;
				   
				}
				else
				{
				  TADD_FLOW("where condition is not satisfy.");
				  continue;
				}
	        }
			//TADD_NORMAL("Scan Full End,Swicth to End,Affect = %d\n",m_MdbTableWalker.m_iAffect);
			m_MdbTableWalker.m_iScanPageType = TYPE_SCAN_PAGE_END;
			m_MdbTableWalker.m_pCurPage = NULL;
			m_MdbTableWalker.m_pStartPage = NULL;
			bResult = false;
		}	
        
		return iRet;
	}

	
    int TMdbExecuteEngine::NextWhereByIndex(bool & bResult)
    {
        TADD_FUNC("Start.");
		int iRet = 0;
		ST_TABLE_INDEX_INFO * pTableIndex = NULL;
		long long   llValue	= -1;
		while(1)
		{
		   if(m_MdbTableWalker.Next() == false)
		   {
			   //按某一索引遍历完了选择下一条索引
			   TADD_FUNC("Finish walk one index.");
			   GetNextIndex(pTableIndex,llValue);
			   if(NULL == pTableIndex)
			   {
				   //索引全部遍历完了。返回false;
				   TADD_FLOW("All indexs have been walked.");
				   bResult = false;
				   break;
			   }
			   m_MdbTableWalker.WalkByIndex(pTableIndex, llValue);//按索引遍历	
			   continue;
		   }
		   else
		   {
			   //获取到一条记录
			   m_pDataAddr	   = m_MdbTableWalker.GetDataAddr();
			   m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
			   m_pPageAddr	   = m_MdbTableWalker.GetPageAddr();
			   m_iPagePos	   = m_MdbTableWalker.GetPagePos();
			   TADD_FLOW("Get one record:DataAddr[%p],CurRowData[%d|%d],PageAddr[%p],PagePos[%d].",
						   m_pDataAddr,m_tCurRowIDData.GetPageID(),m_tCurRowIDData.GetDataOffset(),
						   m_pPageAddr,m_iPagePos);
			   if(IsDataPosBefore())
			   {
				   //数据曾经定位过
				   TADD_FLOW("Data PosBefore");
				   continue;
			   }
			   CHECK_RET(CheckWhere(bResult),"CheckWhere failed.");
			   if(bResult)
			   {
				   //where 条件满足
				   TADD_FLOW("where condition is satisfy.");
				   break;
			   }
			   else
			   {
				   //where 条件不满足
				  TADD_FLOW("where condition is not satisfy.");
				  continue;
			   }
		   }
		}
		return iRet;
	}
	

    /******************************************************************************
    * 函数名称	:  ClearLastExecute
    * 函数描述	:  清理上一次的执行结果
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    void TMdbExecuteEngine::ClearLastExecute()
    {
        m_iCurIndex    = -1;
        m_bScanAll     = false;
        m_llScanAllPos = -1;
        m_iRowsAffected= 0;
        m_iNextType    = NEXT_NORMAL;
        m_iMoniNext    = 0;
        m_tCurRowIDData.Clear();
        m_MdbTableWalker.StopWalk();//结束上一次遍历
        m_tError.FillErrMsg(0,"");//清理错误信息
        if(m_pMdbSqlParser != NULL){m_pMdbSqlParser->ClearLastExecute();}
        m_tCacheTable.Clear();
        m_tLimitCtrl.Clear();
    }
    /******************************************************************************
    * 函数名称	:  SetRollbackBlock
    * 函数描述	:  设置回滚段
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    void TMdbExecuteEngine::SetRollback(bool bCanRollback)
    {
		m_bCanRollBack = bCanRollback;
    }

    /******************************************************************************
    * 函数名称	:  IsPKExist
    * 函数描述	:  检测主键是否已存在
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::IsPKExist()
    {
        TADD_FUNC("Start.");
        int iRet= 0;
        long long llValue = 0;
        TADD_FLOW("Check PK index[%s].",m_pMdbSqlParser->m_stSqlStruct.stIndexForVerifyPK.pstTableIndex->pIndexInfo->sName);
        CalcMemValueHash(m_pMdbSqlParser->m_stSqlStruct.stIndexForVerifyPK,llValue);//计算主键值
		SetTrieWord(m_pMdbSqlParser->m_stSqlStruct.stIndexForVerifyPK);
		ST_INDEX_VALUE & stIndexForVerifyPK = m_pMdbSqlParser->m_stSqlStruct.stIndexForVerifyPK;
        m_MdbTableWalker.WalkByIndex(stIndexForVerifyPK.pstTableIndex, llValue);
        while(m_MdbTableWalker.Next())
        {
            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
            m_iPagePos      = m_MdbTableWalker.GetPagePos();
			
            CHECK_RET_FILL(FillSqlParserValue(m_pMdbSqlParser->m_listInputPriKey),"FillSqlParserValue error.");
            if(TMdbMemValue::CompareMemValueList(m_pMdbSqlParser->m_listInputPriKey,
                                                 m_pMdbSqlParser->m_listOutputPriKey) == 0)
            {
                //主键冲突
                char sTemp[1024] = {0};
                int i = 0 ;
                for(i = 0; i<m_pMdbSqlParser->m_listOutputPriKey.iItemNum; ++i)
                {
                    ST_MEM_VALUE * pstMemValue = m_pMdbSqlParser->m_listOutputPriKey.pValueArray[i];
                    if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
                    {
                        sprintf(sTemp+strlen(sTemp),"|%s",pstMemValue->sValue);
                    }
                    else if(MemValueHasProperty(pstMemValue,MEM_Int))
                    {
                        sprintf(sTemp+strlen(sTemp),"|%lld",pstMemValue->lValue);
                    }
                }
                CHECK_RET_FILL(ERR_TAB_PK_CONFLICT,"primary key conflict...value[%s].",sTemp);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }


	//外部调用 @ Commit的时候
	int TMdbExecuteEngine::IsPKExist(char* pDataAddr)
    {
        TADD_FUNC("Start.");
        int iRet= 0;
        long long llValue = 0;
		
		ST_TABLE_INDEX_INFO * pstTablePKIndex = m_mdbIndexCtrl.GetVerfiyPKIndex();
		CHECK_OBJ(pstTablePKIndex);		
		
        CalcMemValueHash(pstTablePKIndex, pDataAddr, llValue, m_tIColumnAddr);//计算主键索引hash值
		//SetTrieWord(stIndexForVerifyPK);
        m_MdbTableWalker.WalkByIndex(pstTablePKIndex, llValue);
        while(m_MdbTableWalker.Next())
        {
            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
            m_iPagePos      = m_MdbTableWalker.GetPagePos();

			//查到了自己
			if(m_pDataAddr == pDataAddr)
			{
				continue; 
			}

			bool bVisible = true;
			CHECK_RET(CheckVisible(bVisible),"CheckVisible failed");
			if(false == bVisible) continue;

			//获取已有的记录行，并比较
			GetOneRowData(&m_tNColumnAddr,false);
			if( true == IsPKValueSame(m_tIColumnAddr, m_tNColumnAddr))
			{
                CHECK_RET(ERR_TAB_PK_CONFLICT,"primary key conflict....");
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

	//return false 不等   return true  相等
	bool  TMdbExecuteEngine::IsPKValueSame(TMdbColumnAddr& tIColumnAddr, TMdbColumnAddr& tNColumnAddr)
	{

		int iPKColumnNo = m_pTable->m_tPriKey.iColumnCounts;

		for(int i = 0; i < iPKColumnNo; ++i)
        {
        	int iColumnNo = m_pTable->m_tPriKey.iColumnNo[i];
            if(iColumnNo < 0)
            {
                break;
            }

			//该列都是空
			if( NULL == tNColumnAddr[iColumnNo] && NULL == tIColumnAddr[iColumnNo])
			{
				continue;
			}
			
			//只有一边为空
			if( (NULL == tNColumnAddr[iColumnNo] && NULL!= tIColumnAddr[iColumnNo]) ||
				(NULL != tNColumnAddr[iColumnNo] && NULL== tIColumnAddr[iColumnNo]))
			{
                return false;
            }
			else if(m_pTable->tColumn[iColumnNo].iDataType == DT_Int)
			{
				if(*(long long*)tIColumnAddr[iColumnNo] != *(long long*)tNColumnAddr[iColumnNo])
				{
					return false;
				}
			}
			else
			{
                if( 0!=strcmp( (char*)tIColumnAddr[iColumnNo], (char*)tNColumnAddr[iColumnNo]))
				{
					return false;
				}
			}
        }

	
		
		return true;
	}

    /******************************************************************************
    * 函数名称	:  IsDataPosBefore
    * 函数描述	:  该记录是否被定位过
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool TMdbExecuteEngine::IsDataPosBefore()
    {
        TADD_FUNC("Start.");
        bool bRet = false;
        int i = 0;
        long long lPosValue = 0;
        long long lDataValue = 0;
        
        int * arrRowIndex = NULL;
        for(i = 0; i<m_iCurIndex; ++i)
        {
            if(0 == i)
            {
               arrRowIndex  =GetRowIndexArray(m_pDataAddr);
            }
            ST_INDEX_VALUE & stIndexValue = (*m_pVIndex)[i];
            CalcMemValueHash(stIndexValue,lPosValue);
            lDataValue = (arrRowIndex[stIndexValue.pstTableIndex->iIndexPos]);
            TADD_DETAIL("PosValue=[%lld],DataValue=[%lld]",lPosValue,lDataValue);
            if(lPosValue == lDataValue)
            {
                bRet = true;
                break;
            }//之前的索引曾经定位过
        }
        TADD_FUNC("Finish.bRet[%d]",bRet);
        return bRet;
    }

    /******************************************************************************
    * 函数名称	:  CheckRowDataStruct
    * 函数描述	:  校验结构长度信息
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::CheckRowDataStruct(int* Column)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(m_pTable);
        if(1 == m_pTable->iColumnCounts){return iRet;} //只有一列无需检测
        int i = 0;
        TMdbColumn * pMdbColumn = NULL;
        for(i = 0;i < m_pTable->iColumnCounts - 1 ;++i)
        {//无法校验最后一个列的长度
            int iOutLen = Column[i+1] - Column[i];
            pMdbColumn = &(m_pTable->tColumn[i]);
            int iColumnLen = pMdbColumn->iColumnLen;
            bool bResult = false;//校验结果
            switch(pMdbColumn->iDataType)
            {
                case DT_Int://long long
                    bResult = (iOutLen == iColumnLen);
                    break;
                case DT_Blob:
                case DT_Char:
                case DT_VarChar:
                    {
                        if(1 == iOutLen)
                        {//单字符
                            bResult = (iColumnLen == 2);
                        }
                        else
                        {
                            bResult = (iOutLen >= iColumnLen);
                        }
                    }
                    break;
                case DT_DateStamp:
                    {
                        if(DATE_TIME_SIZE == iColumnLen)
                        {//非压缩
                           bResult = (iOutLen >= iColumnLen);
                        }
                        else
                        {//压缩时间
                            bResult = (iOutLen == iColumnLen);
                        }
                    }
                    break;
                default:
                    CHECK_RET(ERR_APP_INVALID_PARAM,"Unknow dataType[%d]",pMdbColumn->iDataType);
                    break;
            }
            if(false == bResult)
            {//长度不正确
                TADD_WARNING("RowDataStruct Table[%s],Column[%s] ColumnLen=[%d] but OutLen=[%d].", 
                    m_pTable->sTableName,pMdbColumn->sName,iColumnLen,iOutLen);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  GetOneRowData
    * 函数描述	:  获取一列数据信息
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::GetOneRowData(void *pStruct,int* Column)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ_FILL(m_pDataAddr);
        if(false == m_pMdbSqlParser->m_bGetValueChecked)
        {//检测GetValue接口，只检测一次
            CheckRowDataStruct(Column);
            m_pMdbSqlParser->m_bGetValueChecked = true;
        }
        char* pStructAddr = (char*)pStruct;
        TMdbColumn * pMdbColumn = NULL;
        int i = 0;
        for(i = 0; i<m_pTable->iColumnCounts; ++i)
        {
            pMdbColumn = &(m_pTable->tColumn[i]);
            switch(pMdbColumn->iDataType)
            {
            case DT_Int:  //Integer
            {
                memcpy(pStructAddr+Column[i],&m_pDataAddr[pMdbColumn->iOffSet],pMdbColumn->iColumnLen);
                TADD_FLOW("Column[%d] value=[%lld],col_pos=[%d]",i,(long long)*(long long *)(pStructAddr+Column[i]),Column[i]);
                break;
            }
            case DT_Char://Char
            {
                memcpy(pStructAddr+Column[i],&m_pDataAddr[pMdbColumn->iOffSet],pMdbColumn->iColumnLen);
                TADD_FLOW("Column[%d] value=[%s],col_pos=[%d]",i,(char *)(pStructAddr+Column[i]),Column[i]);
                break;
            }
            case DT_Blob:
            {
				if(m_tRowCtrl.IsColumnNull(pMdbColumn,m_pDataAddr) == true)
		        {// NULL值
		            *(pStructAddr+Column[i]) = 0;
		            TADD_FLOW("Column[%d] is NULL",i);
		            break;
		        }
                CHECK_RET_FILL_CODE(m_tVarcharCtrl.GetVarcharValue(pStructAddr+Column[i], m_pDataAddr+pMdbColumn->iOffSet),
                               ERR_SQL_GET_MEMORY_VALUE_ERROR,"GetVarcharValue ERROR");
                TADD_FLOW("Column[%d] value=[%s],col_pos=[%d]",i,(char *)(pStructAddr+Column[i]),Column[i]);
                //Base64解码
                std::string encoded = (pStructAddr+Column[i]);
                std::string decoded = Base::base64_decode(encoded);
                memset(pStructAddr+Column[i],0,strlen(pStructAddr+Column[i])); //清除原来数据
                memcpy(pStructAddr+Column[i],decoded.c_str(),decoded.length());
            }
            break;
            case DT_VarChar:  //VarChar
            {
				if(m_tRowCtrl.IsColumnNull(pMdbColumn,m_pDataAddr) == true)
		        {// NULL值
		        	*(pStructAddr+Column[i]) = 0;
		            TADD_FLOW("Column[%d] is NULL",i);
		            break;
		        }
                CHECK_RET_FILL_CODE(m_tVarcharCtrl.GetVarcharValue(pStructAddr+Column[i], m_pDataAddr+pMdbColumn->iOffSet),
                               ERR_SQL_GET_MEMORY_VALUE_ERROR,"GetVarcharValue ERROR");
                TADD_FLOW("Column[%d] value=[%s],col_pos=[%d]",i,(char *)(pStructAddr+Column[i]),Column[i]);
                break;
            }
            case DT_DateStamp: //时间用string保存
            {
                memcpy(pStructAddr+Column[i],&m_pDataAddr[pMdbColumn->iOffSet],pMdbColumn->iColumnLen);
                if(sizeof(int) == pMdbColumn->iColumnLen)
                {
                    TADD_FLOW("Column[%d] value=[%d],col_pos=[%d]",i,(int)*(int *)(pStructAddr+Column[i]),Column[i]);
                }
                else if(sizeof(long long) == pMdbColumn->iColumnLen)
                {
                    TADD_FLOW("Column[%d] value=[%lld],col_pos=[%d]",i,(long long)*(long long *)(pStructAddr+Column[i]),Column[i]);
                }
                else 
                {
                    TADD_FLOW("Column[%d] value=[%s],col_pos=[%d]",i,(char *)(pStructAddr+Column[i]),Column[i]);
                }
                break;
            }
            default:
                CHECK_RET_FILL(ERR_SQL_TYPE_INVALID,"column[%s]DataType=%d invalid.", pMdbColumn->sName,pMdbColumn->iDataType);
                break;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbExecuteEngine::GetOneRowData(TMdbColumnAddr* pTColumnAddr, bool bBlob)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
		CHECK_OBJ_FILL(pTColumnAddr);
        CHECK_OBJ_FILL(m_pDataAddr);
        TMdbColumn * pMdbColumn = NULL;
        int i = 0;
        for(i = 0; i<m_pTable->iColumnCounts; ++i)
        {
            pMdbColumn = &(m_pTable->tColumn[i]);
            switch(pMdbColumn->iDataType)
            {
            case DT_Int:  //Integer            
            case DT_Char: //Char
            case DT_DateStamp://时间用string保存
            {
				pTColumnAddr->m_ppColumnAddr[i] = &m_pDataAddr[pMdbColumn->iOffSet];
				pTColumnAddr->m_iDataLen[i] = pMdbColumn->iColumnLen;
                break;
            }
            case DT_VarChar:  //VarChar
            {
				if(m_tRowCtrl.IsColumnNull(pMdbColumn,m_pDataAddr) == true)
		        {// NULL值
		        	pTColumnAddr->m_ppColumnAddr[i] = NULL;
					pTColumnAddr->m_iDataLen[i] = 0;
		            TADD_FLOW("Column[%d] is NULL",i);
		            break;
		        }
				
				int iWhichPos = 0;
				unsigned int iRowId = 0;
				m_tVarcharCtrl.GetStoragePos(m_pDataAddr+pMdbColumn->iOffSet,iWhichPos,iRowId);
				TMdbShmDSN* pShmDSN = TMdbShmMgr::GetShmDSN(m_pDsn->sName);
				TMdbVarchar* pVarChar = pShmDSN->GetVarchar(iWhichPos);
	    		CHECK_OBJ(pVarChar);
				m_tVarcharCtrl.SetVarchar(pVarChar);

				TMdbRowID tRowId;
			    tRowId.SetRowId(iRowId);
			    pTColumnAddr->m_ppColumnAddr[i] = m_tVarcharCtrl.GetAddressRowId(&tRowId);	
				pTColumnAddr->m_iDataLen[i] = m_tVarcharCtrl.GetValueSize(iWhichPos);
                break;
            }
			case DT_Blob:
            {
				if(m_tRowCtrl.IsColumnNull(pMdbColumn,m_pDataAddr) == true)
		        {// NULL值
		        	pTColumnAddr->m_ppColumnAddr[i] = NULL;
					pTColumnAddr->m_iDataLen[i] = 0;
		            TADD_FLOW("Column[%d] is NULL",i);
		            break;
		        }
				if(!bBlob) return iRet;
				
                int iWhichPos = 0;
				unsigned int iRowId = 0;
				size_t iEnCodeLen = 0;
				m_tVarcharCtrl.GetStoragePos(m_pDataAddr+pMdbColumn->iOffSet,iWhichPos,iRowId);
				TMdbShmDSN* pShmDSN = TMdbShmMgr::GetShmDSN(m_pDsn->sName);
				TMdbVarchar* pVarChar = pShmDSN->GetVarchar(iWhichPos);
	    		CHECK_OBJ(pVarChar);
				m_tVarcharCtrl.SetVarchar(pVarChar);

				TMdbRowID tRowId;
			    tRowId.SetRowId(iRowId);
			    std::string encoded = m_tVarcharCtrl.GetAddressRowId(&tRowId);
				std::string decoded = Base::base64_decode(encoded);
				iEnCodeLen =  decoded.length();
				
				pTColumnAddr->m_iDataLen[i] = (int)iEnCodeLen;
				
				//长度不够再重新申请
				if(pTColumnAddr->m_iBlobAskLen[i] < (int)iEnCodeLen)
				{
					SAFE_DELETE(pTColumnAddr->m_ppBlob[i]);
					pTColumnAddr->m_ppBlob[i] = new(std::nothrow) char[iEnCodeLen];
					if(pTColumnAddr->m_ppBlob[i] ==  NULL)
					{
						iRet = ERR_OS_NO_MEMROY;
						TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new pTColumnAddr->m_ppBlob ");
						break;
					}
					pTColumnAddr->m_iBlobAskLen[i] = (int)iEnCodeLen;
				}
				
				memcpy(pTColumnAddr->m_ppBlob[i],decoded.c_str(),iEnCodeLen);				
				pTColumnAddr->m_ppColumnAddr[i] = pTColumnAddr->m_ppBlob[i];

				break;
            }
           
            default:
                CHECK_RET_FILL(ERR_SQL_TYPE_INVALID,"column[%s]DataType=%d invalid.", pMdbColumn->sName,pMdbColumn->iDataType);
                break;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

	

    long long TMdbExecuteEngine::GetRowTimeStamp()
    {
        TADD_FUNC("Start.");
        int  iRet = 0;
        long long iTimeStamp = 0;
        iRet = m_tRowCtrl.GetTimeStamp(m_pDataAddr, m_pTable->m_iTimeStampOffset, iTimeStamp);
        if(iRet != 0)
        {
            iTimeStamp = 0;
        }
        return iTimeStamp;
    }


    /******************************************************************************
    * 函数名称	:  GetRowIndexArray
    * 函数描述	:  获取rowindex数组
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int * TMdbExecuteEngine::GetRowIndexArray(char * pDataAddr)
    {
        TADD_FUNC("Start");
        if(NULL == m_pTable)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"table is null");
            return NULL;
        }
        if(NULL == m_aRowIndexPos)
        {
            m_aRowIndexPos = new(std::nothrow) int[m_pTable->iIndexCounts];
        }
        if(NULL == m_aRowIndexPos){return NULL;}
        if(NULL != pDataAddr)
        {//计算索引值,基础索引值
            int i = 0;
            int iError = 0;
            for(i = 0;i < m_pTable->iIndexCounts;++i)
            { 
                m_aRowIndexPos[i] = -1;
                m_aRowIndexPos[i] = m_mdbIndexCtrl.CalcIndexValue(m_tRowCtrl, pDataAddr, &(m_pTable->tIndex[i]), iError);
                if(0 != iError)
                {
                    TADD_ERROR(iError,"CalcIndexValue(iError=%d) failed.",iError);
                    return NULL;
                }
            }
        }
        return m_aRowIndexPos;
    }
    

    /******************************************************************************
    * 函数名称	:  GetUpdateDiff
    * 函数描述	:  获取增量更新值,只获取数值型的增量
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::GetUpdateDiff()
    {
        TADD_FUNC("Start");
        int iRet = 0;
        CHECK_OBJ_FILL(m_pUpdateBlock);
        int i = 0;
        for(i = 0; i<m_pMdbSqlParser->m_listOutputCollist.iItemNum; i++)
        {
            ST_MEM_VALUE * pstMemValue = m_pMdbSqlParser->m_listOutputCollist.pValueArray[i];
            int iOffSet = pstMemValue->pColumnToSet->iOffSet;
            //if(DT_Int == pstMemValue->pColumnToSet->iDataType)
            if(pstMemValue->pColumnToSet->bIncrementalUpdate())
            {//增量型
                    long long * pDiffInt = (long long*)&m_pUpdateBlock[iOffSet];
                    * pDiffInt = (*(long long*)&m_pDataAddr[iOffSet]) - pstMemValue->lValue;//获取差值
                    TADD_DETAIL("pDiffInt=[%lld]",*pDiffInt);
            }
        }
        //获取主键值
        for(i = 0; i<m_pMdbSqlParser->m_listInputPriKey.iItemNum; i++)
        {
            ST_MEM_VALUE * pstMemValue = m_pMdbSqlParser->m_listInputPriKey.pValueArray[i];
            int iOffSet = pstMemValue->pColumnToSet->iOffSet;
            if(DT_Int == pstMemValue->pColumnToSet->iDataType)
            {//数值型
                    long long * pPKInt = (long long*)&m_pUpdateBlock[iOffSet];
                    *pPKInt = pstMemValue->lValue;
                    TADD_DETAIL("pPKInt=[%lld]",*pPKInt);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbExecuteEngine::SetRowDataTimeStamp(char* pAddr, int iOffset,long long iTimeStamp)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        long long* piTimeStamp = (long long*)&pAddr[iOffset];

        if(iTimeStamp == 0)
        {
            *piTimeStamp = TMdbDateTime::StringToTime( m_pDsn->sCurTime);
        }
        else
        {
            *piTimeStamp = iTimeStamp;
        }

        return iRet;
    }

    int TMdbExecuteEngine::UpdateRowDataTimeStamp(char* const & pAddr,int iOffset, long long iTimeStamp )
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        if(iTimeStamp == 0)
        {
            iRet = m_tRowCtrl.SetTimeStamp(pAddr,iOffset,(long long) TMdbDateTime::StringToTime( m_pDsn->sCurTime));
        }
        else
        {
            iRet = m_tRowCtrl.SetTimeStamp(pAddr, iOffset,iTimeStamp);
        }

        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  ReBuildTableFromPage
    * 函数描述	:  从内存页重新构建表,索引
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbExecuteEngine::ReBuildTableFromPage(const char * sDSN,TMdbTable * pMdbTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        ClearLastExecute();
        CHECK_OBJ(pMdbTable);
        CHECK_OBJ(sDSN);
        m_pTable = pMdbTable;
        TMdbShmDSN * pShmDSN = TMdbShmMgr::GetShmDSN(sDSN);
        CHECK_OBJ(pShmDSN);
        m_pDsn = pShmDSN->GetInfo();
		m_mdbPageCtrl.SetDSN(m_pDsn->sName);
        m_mdbIndexCtrl.AttachTable(pShmDSN,pMdbTable);
        CHECK_RET(m_MdbTableWalker.AttachTable(pShmDSN,m_pTable),"AttachTable failed.");
        CHECK_RET_FILL_CODE(m_tRowCtrl.Init(m_pDsn->sName,m_pTable->sTableName),ERR_OS_ATTACH_SHM,"m_tRowCtrl.AttachTable failed.");//记录管理
        //遍历full 链
        m_MdbTableWalker.WalkByPage(m_pTable->iFullPageID);
        while(m_MdbTableWalker.NextByPage())
        {
            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
            m_iPagePos      = m_MdbTableWalker.GetPagePos();
			
			if( -1 == FSCheckInvalidData())
			{
				FSRemoveInvalidData();
				continue;
			}
			
            TADD_DETAIL("page_id=[%d],offset=[%d],rowid=[%ud].",m_pTable->iFullPageID,m_iPagePos,m_tCurRowIDData.m_iRowID);
            CHECK_RET(ChangeInsertIndex(m_pDataAddr,m_tCurRowIDData),"ChangeInsertIndex failed.");//插入索引
            m_pTable->iCounts++;
        }
        //遍历free 链
        m_MdbTableWalker.WalkByPage(m_pTable->iFreePageID);
        while(m_MdbTableWalker.NextByPage())
        {
            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
            m_iPagePos      = m_MdbTableWalker.GetPagePos();

			if( -1 == FSCheckInvalidData())
			{
				FSRemoveInvalidData();
				continue;
			}
			
            TADD_DETAIL("page_id=[%d],offset=[%d],rowid=[%d].",m_tCurRowIDData.GetPageID(),m_tCurRowIDData.GetDataOffset(),m_tCurRowIDData.m_iRowID);
            CHECK_RET(ChangeInsertIndex(m_pDataAddr,m_tCurRowIDData),"ChangeInsertIndex failed.");//插入索引
            m_pTable->iCounts ++;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }


	// return -1 代表数据非法   0  合法
	int TMdbExecuteEngine::FSCheckInvalidData()
	{	

    	if(!m_pDataAddr)
    	{
			return -1;
		}
		TMdbPageNode* pNode = (TMdbPageNode*)m_pDataAddr - 1;

		
		if(0 == pNode->iSessionID)
		{
			return 0;	
		}
		else 
		{
			//影子数据  还没有commit
			if(pNode->cFlag & DATA_VIRTUAL) return -1;

			//
		}
		
		return 0;
	}

	
	int TMdbExecuteEngine::FSRemoveInvalidData()
	{
		int iRet = 0;
		TADD_NORMAL("Data From FS is UnCommited. Will Remove. page_id=[%d],offset=[%d],rowid=[%u]. ",((TMdbPage*)m_pPageAddr)->m_iPageID, m_iPagePos,m_tCurRowIDData.m_iRowID);
        CHECK_RET(DeleteVarCharValue(m_pDataAddr),"DeleteVarCharValue error.");

		CHECK_RET(m_mdbPageCtrl.Attach(m_pPageAddr, m_pTable->bReadLock, m_pTable->bWriteLock),"m_mdbPageCtrl.Attach faild");
		CHECK_RET(m_mdbPageCtrl.DeleteData_NoMutex(m_tCurRowIDData.GetDataOffset()),"m_mdbPageCtrl.DeleteData");
		return iRet;
	}


	int TMdbExecuteEngine::BuildSingleIndexFromPage(TMdbShmDSN * pShmDSN,TMdbTable * pMdbTable,int iIndexPos)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        ClearLastExecute();
        CHECK_OBJ(pMdbTable);
        m_pTable = pMdbTable;
        CHECK_OBJ(pShmDSN);
        m_pDsn = pShmDSN->GetInfo();
        m_mdbIndexCtrl.AttachTable(pShmDSN,pMdbTable);
        CHECK_RET(m_MdbTableWalker.AttachTable(pShmDSN,m_pTable),"AttachTable failed.");
        CHECK_RET_FILL_CODE(m_tRowCtrl.Init(m_pDsn->sName,m_pTable->sTableName),ERR_OS_ATTACH_SHM,"m_tRowCtrl.AttachTable failed.");//记录管理
        //遍历free 链
        m_MdbTableWalker.WalkByPage(m_pTable->iFreePageID);
        while(m_MdbTableWalker.NextByPage())
        {
            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
            m_iPagePos      = m_MdbTableWalker.GetPagePos();
			CHECK_RET_FILL(m_mdbIndexCtrl.InsertIndexNode(iIndexPos,m_pDataAddr,m_tRowCtrl,m_tCurRowIDData),
						   "InsertIndexNode failed.index=[%s]",m_pTable->tIndex[iIndexPos].sName);
        }
		//遍历full 链
        m_MdbTableWalker.WalkByPage(m_pTable->iFullPageID);
        while(m_MdbTableWalker.NextByPage())
        {
            m_pDataAddr     = m_MdbTableWalker.GetDataAddr();
            m_tCurRowIDData = m_MdbTableWalker.GetDataRowID();
            m_pPageAddr     = m_MdbTableWalker.GetPageAddr();
            m_iPagePos      = m_MdbTableWalker.GetPagePos();
            CHECK_RET_FILL(m_mdbIndexCtrl.InsertIndexNode(iIndexPos,m_pDataAddr,m_tRowCtrl,m_tCurRowIDData),
                "InsertIndexNode failed.index=[%s]",m_pTable->tIndex[iIndexPos].sName);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbExecuteEngine::CalcMemValueHash(ST_INDEX_VALUE & stIndexValue,long long & llValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_sTempValue[0] = 0;
        if(HT_CMP ==  stIndexValue.pstTableIndex->pIndexInfo->m_iIndexType)
        {
            //组合索引
            int i = 0;
            for(i = 0; i < MAX_INDEX_COLUMN_COUNTS; ++i)
            {
                if(stIndexValue.pstTableIndex->pIndexInfo->iColumnNo[i] < 0)
                {
                    break;
                }
                CHECK_OBJ(stIndexValue.pExprArr[i]);
                ST_MEM_VALUE * pstMemValue = stIndexValue.pExprArr[i]->pExprValue;
                CHECK_OBJ(pstMemValue);
                if(pstMemValue->IsNull())
                {
                    sprintf(m_sTempValue, "%s;0",m_sTempValue);
                }
                else if(MemValueHasProperty(pstMemValue,MEM_Int))
                {
                    sprintf(m_sTempValue, "%s;%lld", m_sTempValue, pstMemValue->lValue);
                }
                else if(MemValueHasAnyProperty(pstMemValue,MEM_Str|MEM_Date))
                {
                    sprintf(m_sTempValue, "%s;%s", m_sTempValue, pstMemValue->sValue);
                }
            }
            llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
        }
        else
        {
            //单索引
            if(stIndexValue.pExprArr[0]->pExprValue->IsNull())
            {
                llValue = 0;
            }
            else  if(MemValueHasAnyProperty(stIndexValue.pExprArr[0]->pExprValue,MEM_Str|MEM_Date))
            {
                llValue = TMdbNtcStrFunc::StrToHash(stIndexValue.pExprArr[0]->pExprValue->sValue);
            }
            else if(MemValueHasProperty(stIndexValue.pExprArr[0]->pExprValue,MEM_Int))
            {
                llValue = stIndexValue.pExprArr[0]->pExprValue->lValue;
            }
        }
        llValue = llValue<0? -llValue:llValue;
        if(stIndexValue.pstTableIndex->pIndexInfo->m_iAlgoType == INDEX_HASH)
        {
            //进行散列
            llValue = llValue % m_pTable->iRecordCounts; 
        }
        TADD_FUNC("Finish.llValue = %lld.m_sTempValue=[%s].",llValue,m_sTempValue);
        return iRet;
    }

	//根据主键索引 和记录地址  找出主键的hash值
	int TMdbExecuteEngine::CalcMemValueHash(ST_TABLE_INDEX_INFO * pstTablePKIndex,char* pDataAddr,long long & llValue,TMdbColumnAddr &tColumnAddr)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
		if(pstTablePKIndex->pIndexInfo->m_iAlgoType != INDEX_HASH &&
			pstTablePKIndex->pIndexInfo->m_iAlgoType != INDEX_M_HASH)
		{
			return iRet;	
		}
		
        m_sTempValue[0] = 0;
		//先根据pDataAddr 获取整条记录
		m_pDataAddr = pDataAddr;
		CHECK_RET(GetOneRowData(&tColumnAddr, false),"GetOneRowData Failed");

        if(HT_CMP ==  pstTablePKIndex->pIndexInfo->m_iIndexType)
        {
            //组合索引
            int i = 0;
            for(i = 0; i < MAX_INDEX_COLUMN_COUNTS; ++i)
            {
            	int iColumnNo = pstTablePKIndex->pIndexInfo->iColumnNo[i];
                if(iColumnNo < 0)
                {
                    break;
                }
					
				if(NULL == tColumnAddr[iColumnNo])
				{
                    sprintf(m_sTempValue, "%s;0",m_sTempValue);
                }
				else if(m_pTable->tColumn[iColumnNo].iDataType == DT_Int)
				{
                    sprintf(m_sTempValue, "%s;%lld", m_sTempValue, *(long long*)tColumnAddr[iColumnNo]);
				}
				else
				{
                    sprintf(m_sTempValue, "%s;%s", m_sTempValue, (char*)tColumnAddr[iColumnNo]);
				}
            }
            llValue = TMdbNtcStrFunc::StrToHash(m_sTempValue);
        }
        else
        {
            //单索引
            int iColumnNo = pstTablePKIndex->pIndexInfo->iColumnNo[0];
			if(NULL == tColumnAddr[iColumnNo])
			{
                llValue = 0;
            }
			else if(m_pTable->tColumn[iColumnNo].iDataType == DT_Int)
			{
                llValue = *(long long*)tColumnAddr[iColumnNo];
			}
			else
			{
                llValue = TMdbNtcStrFunc::StrToHash((char*)tColumnAddr[iColumnNo]);
			}
        }
        llValue = llValue<0? -llValue:llValue;
        //进行散列
        llValue = llValue % m_pTable->iRecordCounts; 

        TADD_FUNC("Finish.llValue = %lld.m_sTempValue=[%s].",llValue,m_sTempValue);
        return iRet;
    }

	
	void TMdbExecuteEngine::SetDataFlagInsert(char* pAddr)
	{
		TMdbPageNode*  pNode = (TMdbPageNode*)pAddr -1;
		if(IsUseTrans())
		{
			pNode->iSessionID = m_pLocalLink->iSessionID;
			pNode->iExecuteID = m_pLocalLink->iExecuteID;
			pNode->cFlag = DATA_VIRTUAL;
		}
		else
		{
			pNode->iSessionID = 0;
			pNode->iExecuteID = 0;
			pNode->cFlag = DATA_REAL;
		}
	}

	void  TMdbExecuteEngine::InitDataMutex(char* pAddr)
	{
		if(IsUseTrans())
		{
			TMdbPageNode*  pNode = (TMdbPageNode*)pAddr -1;
			pNode->tMutex.Create();
		}
	}

	int  TMdbExecuteEngine::SetDataFlagDelete(char* pAddr)
	{
		int iRet = 0;
		if(IsUseTrans())
		{
			TMdbPageNode*  pNode = (TMdbPageNode*)pAddr -1;

			//如果是当前链接没有提交的数据,不加锁
			if(pNode->iSessionID == m_pLocalLink->iSessionID)
			{
				//do nothing
			}
			else
			{
				int iTry = 5;
				while(pNode->tMutex.TryLock()!=0)
				{
					printf("Table %s,Lock Row %d Failed,Try %d times.\n",m_pTable->sTableName,m_tCurRowIDData.m_iRowID,iTry);
					iTry--;
					TMdbDateTime::Sleep(4);
					if(iTry<=0)
					{
						printf("Mutex[Table:%s][Row:%d] Maybe dead,force unLock it.\n",m_pTable->sTableName,m_tCurRowIDData.m_iRowID);
						CHECK_RET(pNode->tMutex.UnLock(true),"Force Unlock Row failed,[Table:%s][Row:%d].",m_pTable->sTableName,m_tCurRowIDData.m_iRowID);

						if(pNode->cFlag  & DATA_RECYCLE)
						{
							return -1;
						}
						
						CHECK_RET(pNode->tMutex.Lock(true),"Lock Row failed,[Table:%s][Row:%d].",m_pTable->sTableName,m_tCurRowIDData.m_iRowID);
						break;
					}	
				}
				
				if(pNode->cFlag  & DATA_RECYCLE)
				{
					return -1;
				}

				//printf("Table %s,Lock Row %d OK\n",m_pTable->sTableName,m_tCurRowIDData.m_iRowID);
	
				pNode->iSessionID = m_pLocalLink->iSessionID;
			}
			
			pNode->cFlag |= DATA_DELETE;
		}
		return iRet;
	}
//}

