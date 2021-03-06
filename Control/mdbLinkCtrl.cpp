/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbLinkCtrl.cpp		
*@Description： 链接注册模块优化改造
*@Author:			jin.shaohua
*@Date：	    2012.11
*@History:
******************************************************************************************/
#include "Control/mdbLinkCtrl.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbDateTime.h"
#include "Control/mdbIndexCtrl.h"


TMdbSingleTableIndexInfo::TMdbSingleTableIndexInfo()
{
	sTableName[0] = 0;
	for(int i=0; i<MAX_INDEX_COUNTS; i++)
	{
		arrLinkIndex[i].Clear();
	}
}


void TMdbLocalLink::Clear()
{
	iPID	  = -1;
	iTID	  = 0;
	iSocketID = -1;
	cState	  = Link_down;
	memset(sStartTime, 0, sizeof(sStartTime));
	memset(sFreshTime, 0, sizeof(sFreshTime));
	iLogLevel = 0;
	cAccess   = MDB_WRITE;
	iSQLPos   = -1;
	
	iQueryCounts = 0;  //查询次数
	iQueryFailCounts = 0;  //查询次数
	
	iInsertCounts = 0; //插入次数
	iInsertFailCounts = 0; //插入次数
	
	iUpdateCounts = 0; //更新次数
	iUpdateFailCounts = 0; //更新次数
	
	iDeleteCounts = 0; //删除次数
	iDeleteFailCounts = 0; //删除次数
	
	
	memset(sProcessName,0,sizeof(sProcessName));

	iRBAddrOffset = 0;

	for(int i=0;i<MAX_TABLE_COUNTS;i++)
	{
		m_AllTableIndexInfo[i].Clear();
	}
}	 
//是否是当前线程的链接
bool TMdbLocalLink::IsCurrentThreadLink()
{
	if(TMdbOS::GetPID() == iPID  && TMdbOS::GetTID() == iTID)
	{
		return true;
	}
	return false;
}

void TMdbLocalLink::Show(bool bIfMore)
{
	char sHead[500];
	static bool bfirst = false;
	memset(sHead,0,500);
	if(bfirst == false)
	{
		sprintf(sHead,"%-11s %-22s %-8s %-14s %-14s %-6s %-10s %-7s SQLPos\n",\
		"Pid","Tid","SocketID","StartTime","FreshTime","State","LogLevel","Access");
		printf("\n");
		printf("[ Local-Link-Info ]:\n");
		printf("%s",sHead);
		bfirst = true;
	}
	if(iPID > 0)
	{
		printf("%-11d %-22lu %-8d %-14s %-14s %-6c %-10d %-7c %d\n",\
		iPID,iTID,iSocketID,sStartTime,sFreshTime,cState,iLogLevel,cAccess, iSQLPos);
	}	
}


void TMdbLocalLink::ShowRBUnits()
{
	if(m_RBList.empty()) return;
	
	int iMaxShow = 100;
	int i = 0;
	TShmList<TRBRowUnit>::iterator itor = m_RBList.begin();
	for(;itor != m_RBList.end();++itor)
	{
		if(i>100)
		{
			printf("Only Show %d RBUnits.\n",iMaxShow);
			break;
		}
		printf("[%d]--",i);
		itor->Show();
		i++;
	}
	int iSize = m_RBList.size();
	printf("RBUnits num %d,Cost Memory %d Bytes.\n",iSize,iSize*sizeof(TRBRowUnit));
}

void TMdbLocalLink::ShowIndexInfo()
{
	for(int i=0;i<MAX_TABLE_COUNTS;i++)
	{
		if(!m_AllTableIndexInfo[i].sTableName[0]) continue;
		printf("Table:[%s].\n",m_AllTableIndexInfo[i].sTableName);
		
		for(int j=0; j<MAX_INDEX_COUNTS; j++)
		{
			ST_LINK_INDEX_INFO& tIndexInfo = m_AllTableIndexInfo[i].arrLinkIndex[j];
			if(!tIndexInfo.bInit) continue;
			tIndexInfo.Print();
		}
	}

}

void TMdbLocalLink::Print()
{
	TADD_NORMAL("==============Local-Link=================");
	TADD_NORMAL("	 iPID		= %d.", iPID);
	TADD_NORMAL("	 iTID		= %lu.", iTID);
	TADD_NORMAL("	 SocketID	= %d.", iSocketID);
	TADD_NORMAL("	 cState 	= %c(%c-USE, %c-DOWN).", cState, Link_use, Link_down);
	TADD_NORMAL("	 sStartTime = %s.", sStartTime);
	TADD_NORMAL("	 sFreshTime = %s.", sFreshTime);
	TADD_NORMAL("	 iLogLevel	= %d.", iLogLevel);
	TADD_NORMAL("	 cAccess	= %c.", cAccess);
	TADD_NORMAL("	 iSQLPos	= %d.", iSQLPos);
	TADD_NORMAL("	 iQueryCounts	 = %d.", iQueryCounts);
	TADD_NORMAL("	 iQueryFailCounts	 = %d.", iQueryFailCounts);
	TADD_NORMAL("	 iInsertCounts	  = %d.", iInsertCounts);
	TADD_NORMAL("	 iInsertFailCounts	  = %d.", iInsertFailCounts);
	TADD_NORMAL("	 iUpdateCounts	  = %d.", iUpdateCounts);
	TADD_NORMAL("	 iUpdateFailCounts	  = %d.", iUpdateFailCounts);
	TADD_NORMAL("	 iDeleteCounts	  = %d.", iDeleteCounts);
	TADD_NORMAL("	 iDeleteFailCounts	  = %d.", iDeleteFailCounts);
	TADD_NORMAL("==============Local-Link=================");
}

int TMdbLocalLink::AddNewRBRowUnit(TRBRowUnit* pRBRowUnit)
{
	int iRet = 0;
	//TADD_NORMAL("TMdbLocalLink::AddNewRBRowUnit\n");
	TShmList<TRBRowUnit>::iterator itorNew =  m_RBList.insert(m_RBList.end());
	if(itorNew != m_RBList.end())
	{//分配成功
		TRBRowUnit* pNewRBRowUnit = &(*itorNew);
		SAFESTRCPY(pNewRBRowUnit->sTableName,MAX_NAME_LEN,pRBRowUnit->sTableName);
		pNewRBRowUnit->SQLType = pRBRowUnit->SQLType;
		pNewRBRowUnit->iRealRowID = pRBRowUnit->iRealRowID;
		pNewRBRowUnit->iVirtualRowID = pRBRowUnit->iVirtualRowID;
	}
	else
	{//分配失败
		CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for RBRowUnit");
	}
	return iRet;
}

int  TMdbLocalLink::ReturnAllIndexNodeToTable(TMdbShmDSN * pShmDSN)
{
	int iRet = 0;
	TMdbIndexCtrl tIndex;
	CHECK_RET(tIndex.ReturnAllIndexNodeToTable(this, pShmDSN),"ReturnAllIndexNodeToTable failed.");
	return iRet;

}


TMdbSingleTableIndexInfo*  TMdbLocalLink::FindCurTableIndex(const char* sTableName)
{
	for(int i=0; i<MAX_TABLE_COUNTS; i++)
	{
		if(0 == strcasecmp(sTableName,m_AllTableIndexInfo[i].sTableName))
		{
			return &m_AllTableIndexInfo[i];
		}
	
	}

	return NULL;
}

TRBRowUnit::TRBRowUnit()
{
	sTableName[0]=0;
	SQLType = TK_SELECT;	
	iRealRowID = 0; 	
	iVirtualRowID = 0;			
}




void TRBRowUnit::Show()
{
	printf("[TableName:%s][SQLType:%s][iRealRowID:%u][iVirtualRowID:%u]\n",
		sTableName,GetSQLName(),iRealRowID,iVirtualRowID);
}

const char* TRBRowUnit::GetSQLName()
{
	switch(SQLType)
	{
		case TK_INSERT:
			return "insert";
		case TK_DELETE:
			return "delete";
		case TK_UPDATE:
			return "update";
		default:
			return "unkown";
	}			
	return "unkown";
}



TMdbRBCtrl::TMdbRBCtrl()
{
	m_pShmDSN = NULL;
	m_pLocalLink = NULL;
	m_iDataSize = 0;
}

TMdbRBCtrl::~TMdbRBCtrl()
{

}

int TMdbRBCtrl::Init(TMdbShmDSN * pShmDSN, TMdbLocalLink* pLocalLink)
{
	int iRet = 0;

	m_pShmDSN = pShmDSN;
	m_pLocalLink = pLocalLink;
	
	return iRet;

}

void TMdbRBCtrl::ShowRBUnits()
{
	if(m_pLocalLink->m_RBList.empty()) 
	{
		printf("Current Link has no RB units.\n");
		return ;
	}
	
	//正向遍历
	TShmList<TRBRowUnit>::iterator itorB = m_pLocalLink->m_RBList.begin();
	while(!m_pLocalLink->m_RBList.empty())
	{		
		itorB->Show();
		itorB++;
	}
}


int TMdbRBCtrl::Commit()
{
	int iRet =0;
	if(m_pLocalLink->m_RBList.empty()) 
	{
		return iRet;
	}

	//正向遍历
	TShmList<TRBRowUnit>::iterator itorB = m_pLocalLink->m_RBList.begin();
	while(!m_pLocalLink->m_RBList.empty())
	{	
		iRet == Commit(&(*itorB));
		if(iRet == ERR_TAB_PK_CONFLICT)
		{
			RollBack(&(*itorB));
		}

		if(iRet!=0)
		{
			m_pLocalLink->m_RBList.clear();
			break; 
		}
		
		itorB= m_pLocalLink->m_RBList.erase(itorB);
	}

	return iRet;
}



int TMdbRBCtrl::Commit(TRBRowUnit* pRBRowUnit)
{
	int iRet = 0;
	switch(pRBRowUnit->SQLType)
	{
		case TK_INSERT:
			CHECK_RET(CommitInsert(pRBRowUnit),"CommitInsert failed");
			break;
		case TK_UPDATE:
			CHECK_RET(CommitUpdate(pRBRowUnit),"CommitUpdate failed");
			break;
		case TK_DELETE:
			CHECK_RET(CommitDelete(pRBRowUnit),"CommitDelete failed");
			break;
		default:
			break;
	}	
	return iRet;
}

int TMdbRBCtrl::CommitInsert(TRBRowUnit* pRBRowUnit)
{
	int iRet = 0;
	TMdbTable* pTable = m_pShmDSN->GetTableByName(pRBRowUnit->sTableName);
	CHECK_OBJ(pTable);

	CHECK_RET(m_tTableWalker.AttachTable(m_pShmDSN, pTable, pRBRowUnit->SQLType),"m_tTableWalker.AttachTable failed.");

	TMdbRowID VirtualRowID;
	VirtualRowID.SetRowId(pRBRowUnit->iVirtualRowID);
	char* pVirtualDataAddr = m_tTableWalker.GetAddressRowID(&VirtualRowID, m_iDataSize, true);
	char* pVirtualPageAddr = m_tTableWalker.GetPageAddr();

	CHECK_RET(CheckPK(pTable, pVirtualDataAddr),"CommitInsert CheckPK Failed.");
	
	TMdbPageNode* pPageNode = (TMdbPageNode*)pVirtualDataAddr - 1;
	pPageNode->iSessionID = 0;	
	pPageNode->iExecuteID = 0;
	pPageNode->cFlag = DATA_REAL;	
	pTable->tTableMutex.Lock(pTable->bWriteLock, &(m_pShmDSN->GetInfo()->tCurTime));
	++pTable->iCounts;
	pTable->tTableMutex.UnLock(pTable->bWriteLock);

	
	//生成同步数据
	m_tFlushTrans.Init(m_pShmDSN,pTable,TK_INSERT,pVirtualDataAddr);

	if(!m_tFlushTrans.IsNeedMakeBuf()){return iRet;}

	TMdbPage* pPage = (TMdbPage*)(pVirtualPageAddr);
	long long llLSN = m_pShmDSN->GetInfo()->GetLSN();
	pPage->m_iPageLSN = llLSN;

	//TADD_NORMAL("llLSN:%ld",llLSN);
	
	CHECK_RET(m_tFlushTrans.MakeBuf(llLSN),"Make ReoBuf failed.");
	CHECK_RET(m_tFlushTrans.InsertBufIntoQueue(),"InsertBufIntoQueue failed.");
	CHECK_RET(m_tFlushTrans.InsertBufIntoCapture(),"InsertBufIntoCapture failed.");
	return iRet;
}

int TMdbRBCtrl::CommitUpdate(TRBRowUnit* pRBRowUnit)
{
	int iRet=0;
	TMdbTable* pTable = m_pShmDSN->GetTableByName(pRBRowUnit->sTableName);
	CHECK_OBJ(pTable);

	CHECK_RET(m_tTableWalker.AttachTable(m_pShmDSN, pTable, pRBRowUnit->SQLType),"m_tTableWalker.AttachTable failed.");

	TMdbRowID VirtualRowID,RealRowID;	
	VirtualRowID.SetRowId(pRBRowUnit->iVirtualRowID);
	RealRowID.SetRowId(pRBRowUnit->iRealRowID);
	
	char* pRealDataAddr = m_tTableWalker.GetAddressRowID(&RealRowID, m_iDataSize, true);
	char* pRealPageAddr = m_tTableWalker.GetPageAddr();
	
	char* pVirtualDataAddr = m_tTableWalker.GetAddressRowID(&VirtualRowID, m_iDataSize, true);
	char* pVirtualPageAddr = m_tTableWalker.GetPageAddr();
	
	TMdbPageNode* pPageNode = (TMdbPageNode* )pVirtualDataAddr-1;
	pPageNode->iSessionID = 0;
	pPageNode->iExecuteID = 0;
	pPageNode->cFlag = DATA_REAL;

	//生成同步数据
	m_tFlushTrans.Init(m_pShmDSN,pTable,TK_UPDATE,pVirtualDataAddr);
	if(m_tFlushTrans.IsNeedMakeBuf())
	{	
		TMdbPage* pPage = (TMdbPage*)(pVirtualPageAddr);
		long long llLSN = m_pShmDSN->GetInfo()->GetLSN();
		pPage->m_iPageLSN = llLSN;
		CHECK_RET(m_tFlushTrans.MakeBuf(llLSN),"Make flush failed.");
	}
	

	//删除之前的数据	
	pPageNode = (TMdbPageNode* )pRealDataAddr -1;
	if(pPageNode->cFlag & DATA_RECYCLE)
	{
		//CHECK_RET(-1,"CommitUpdate, But data is deleted by other link.");
		return 0;
	}
			
	TMdbRowID RowID;
	RowID.SetRowId(pRBRowUnit->iRealRowID);

	CHECK_RET(m_tEngine.Init(m_pShmDSN, pTable, m_pLocalLink),"m_tEngine Init Failed.");
	CHECK_RET(m_tEngine.ExecuteDelete(pRealPageAddr, pRealDataAddr,RowID,m_pShmDSN,pTable),"ExecuteDelete failed.");

	//回收掉的数据节点打上标志
	pPageNode->cFlag |= DATA_RECYCLE;
	CHECK_RET(UnLockRow(pRealDataAddr),"UnLockRow Failed.");
	
	//将同步数据插入队列
	CHECK_RET(m_tFlushTrans.InsertBufIntoQueue(),"InsertBufIntoQueue failed.");
	CHECK_RET(m_tFlushTrans.InsertBufIntoCapture(),"InsertBufIntoCapture failed.");
	
	return iRet;

}

int TMdbRBCtrl::CommitDelete(TRBRowUnit* pRBRowUnit)
{
	int iRet=0;
	TMdbTable* pTable = m_pShmDSN->GetTableByName(pRBRowUnit->sTableName);
	CHECK_OBJ(pTable);

	CHECK_RET(m_tTableWalker.AttachTable(m_pShmDSN, pTable, pRBRowUnit->SQLType),"m_tTableWalker.AttachTable failed.");

	TMdbRowID rowID;
	rowID.SetRowId(pRBRowUnit->iRealRowID);	
	char* pRealDataAddr = m_tTableWalker.GetAddressRowID(&rowID, m_iDataSize, true);
	
	CHECK_OBJ(pRealDataAddr);
	TMdbPageNode* pPageNode = (TMdbPageNode* )pRealDataAddr -1;
	if(pPageNode->cFlag & DATA_RECYCLE)
	{
		//CHECK_RET(-1,"CommitDelete, But data is deleted by other link.");
		return 0;
	}

	char* pRealPageAddr = m_tTableWalker.GetPageAddr();

	//在删除之前需要采集同步数据
	m_tFlushTrans.Init(m_pShmDSN,pTable,TK_DELETE,pRealDataAddr);
	if(m_tFlushTrans.IsNeedMakeBuf())
	{
		TMdbPage* pPage = (TMdbPage*)(pRealDataAddr);
		long long llLSN = m_pShmDSN->GetInfo()->GetLSN();
		pPage->m_iPageLSN = llLSN;
		CHECK_RET(m_tFlushTrans.MakeBuf(llLSN),"Make ReoBuf failed.");
	}
	
	TMdbRowID RowID;
	RowID.SetRowId(pRBRowUnit->iRealRowID);

	CHECK_RET(m_tEngine.Init(m_pShmDSN, pTable, m_pLocalLink),"m_tEngine Init Failed.");
	CHECK_RET(m_tEngine.ExecuteDelete(pRealPageAddr, pRealDataAddr, RowID, m_pShmDSN, pTable),"ExecuteDelete failed.");

	//回收掉的数据节点打上标志
	pPageNode->cFlag |= DATA_RECYCLE;	
	CHECK_RET(UnLockRow(pRealDataAddr),"UnLockRow Failed.");
	
	//写入队列
	CHECK_RET(m_tFlushTrans.InsertBufIntoQueue(),"InsertBufIntoQueue failed.");
	CHECK_RET(m_tFlushTrans.InsertBufIntoCapture(),"InsertBufIntoCapture failed.");

	return iRet;


}

int TMdbRBCtrl::RollBack()
{
	int iRet = 0;
	if(m_pLocalLink->m_RBList.empty()) return 0 ;
	
	int iAffect = 0;	
	//反向遍历
	TShmList<TRBRowUnit>::iterator itor = m_pLocalLink->m_RBList.end();
	while(!m_pLocalLink->m_RBList.empty())
	{
		--itor; 
		if(RollBack( &(*itor) ) !=0)
		{
			m_pLocalLink->m_RBList.clear();
			break; 
		}
		
		itor = m_pLocalLink->m_RBList.erase(itor);
		iAffect++;
	}
	printf("Rollback Affected: %d.\n",iAffect);
	
	return iRet;
}



int TMdbRBCtrl::RollBack(TRBRowUnit* pRBRowUnit)
{
	int iRet = 0;
	switch(pRBRowUnit->SQLType)
	{
		case TK_INSERT:
			CHECK_RET(RollBackInsert(pRBRowUnit),"RollBackInsert failed");
			break;
		case TK_UPDATE:
			CHECK_RET(RollBackUpdate(pRBRowUnit),"RollBackUpdate failed");
			break;
		case TK_DELETE:
			CHECK_RET(RollBackDelete(pRBRowUnit),"RollBackDelete failed");
			break;
		default:
			break;
	}	
	return iRet;
}

int TMdbRBCtrl::RollBackInsert(TRBRowUnit* pRBRowUnit)
{
	int iRet=0;

	TMdbTable* pTable = m_pShmDSN->GetTableByName(pRBRowUnit->sTableName);
	CHECK_OBJ(pTable);
	CHECK_RET(m_tTableWalker.AttachTable(m_pShmDSN, pTable, pRBRowUnit->SQLType),"m_tTableWalker.AttachTable failed.");

	TMdbRowID rowID;
	rowID.SetRowId(pRBRowUnit->iVirtualRowID);	
	
	char* pVirtualDataAddr = m_tTableWalker.GetAddressRowID(&rowID, m_iDataSize, true);
	char* pVirtualPageAddr = m_tTableWalker.GetPageAddr();

	//删除索引->删除varchar->删除内存
	CHECK_RET(m_tEngine.Init(m_pShmDSN, pTable, m_pLocalLink),"m_tEngine Init Failed.");
	CHECK_RET(m_tEngine.ExecuteDelete(pVirtualPageAddr, pVirtualDataAddr, rowID, m_pShmDSN,pTable),"ExecuteDelete failed.");
	return iRet;

}
int TMdbRBCtrl::RollBackUpdate(TRBRowUnit* pRBRowUnit)
{
	int iRet=0;
	
	TMdbTable* pTable = m_pShmDSN->GetTableByName(pRBRowUnit->sTableName);
	CHECK_OBJ(pTable);
	CHECK_RET(m_tTableWalker.AttachTable(m_pShmDSN, pTable, pRBRowUnit->SQLType),"m_tTableWalker.AttachTable failed.");

	TMdbRowID VirtualRowID,RealRowID;	
	VirtualRowID.SetRowId(pRBRowUnit->iVirtualRowID);
	RealRowID.SetRowId(pRBRowUnit->iRealRowID);


	char* pRealDataAddr = m_tTableWalker.GetAddressRowID(&RealRowID, m_iDataSize, true);
	char* pRealPageAddr = m_tTableWalker.GetPageAddr();
	
	char* pVirtualDataAddr = m_tTableWalker.GetAddressRowID(&VirtualRowID, m_iDataSize, true);
	char* pVirtualPageAddr = m_tTableWalker.GetPageAddr();	


	//RollBackDelete
	CHECK_OBJ(pRealDataAddr);
	TMdbPageNode* pPageNode = (TMdbPageNode* )pRealDataAddr -1;
	pPageNode->cFlag&=~DATA_DELETE;	
	CHECK_RET(UnLockRow(pRealDataAddr),"UnLockRow Failed.");

	//RollBackInsert	
	//删除索引->删除varchar->删除内存
	CHECK_RET(m_tEngine.Init(m_pShmDSN, pTable, m_pLocalLink),"m_tEngine Init Failed.");
	CHECK_RET(m_tEngine.ExecuteDelete(pVirtualPageAddr, pVirtualDataAddr, VirtualRowID,m_pShmDSN,pTable),"ExecuteDelete failed.");
	
	return iRet;
}
int TMdbRBCtrl::RollBackDelete(TRBRowUnit* pRBRowUnit)
{
	int iRet = 0;
	
	TMdbTable* pTable = m_pShmDSN->GetTableByName(pRBRowUnit->sTableName);
	CHECK_OBJ(pTable);
	CHECK_RET(m_tTableWalker.AttachTable(m_pShmDSN, pTable, pRBRowUnit->SQLType),"m_tTableWalker.AttachTable failed.");

	TMdbRowID RealRowID;
	RealRowID.SetRowId(pRBRowUnit->iRealRowID);
	char* pRealDataAddr = m_tTableWalker.GetAddressRowID(&RealRowID, m_iDataSize, true);
	char* pRealPageAddr = m_tTableWalker.GetPageAddr();	
	
	if(NULL == pRealDataAddr)
	{
		pRBRowUnit->Show();
		CHECK_OBJ(pRealDataAddr);
	}
	TMdbPageNode* pPageNode = (TMdbPageNode* )pRealDataAddr -1;
	pPageNode->cFlag&=~DATA_DELETE;
	
	CHECK_RET(UnLockRow(pRealDataAddr),"UnLockRow Failed.");
	return iRet;

}
int TMdbRBCtrl::UnLockRow(char* pDataAddr)
{
	int iRet = 0;
	CHECK_OBJ(pDataAddr);
	TMdbPageNode* pPageNode = (TMdbPageNode* )pDataAddr -1;

	CHECK_RET(pPageNode->tMutex.UnLock(true),"PageNode UnLock Failed.");
	
	return iRet;

}


int TMdbRBCtrl::CheckPK(TMdbTable* pTable, char* pDataAddr)
{
	TADD_FUNC("Start.");
	int iRet= 0;

	if(!pTable->bIsCheckPriKey) return iRet;

	CHECK_RET(m_tEngine.Init(m_pShmDSN, pTable, m_pLocalLink),"m_tEngine Init Failed.");
	return m_tEngine.IsPKExist(pDataAddr);	
}





TMdbLinkCtrl::TMdbLinkCtrl()
{
	m_pShmDsn = NULL;
	m_pDsn = NULL;
}
TMdbLinkCtrl::~TMdbLinkCtrl()
{

}
/******************************************************************************
* 函数名称	:  Attach
* 函数描述	:  链接共享内存
* 输入		:  sDsn - dsn名
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::Attach(const char * sDsn)
{
    TADD_FUNC("Start");
    int iRet = 0;
    CHECK_OBJ(sDsn);
    m_pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
    CHECK_OBJ(m_pShmDsn);
    m_pDsn = m_pShmDsn->GetInfo();
    CHECK_OBJ(m_pDsn);	
	m_tMgrShmAlloc.AttachByKey(m_pShmDsn->GetMgrKey(),m_pMgrAddr);	
    return iRet;
}
/******************************************************************************
* 函数名称	:  RegLocalLink
* 函数描述	:  注册本地链接
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::RegLocalLink(TMdbLocalLink *& pLocalLink)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    pLocalLink = NULL;
    CHECK_OBJ(m_pShmDsn);
    CHECK_RET(m_pShmDsn->LockDSN(),"lock failed.");
    do{
        CHECK_RET_BREAK(m_pShmDsn->AddNewLocalLink(pLocalLink),"AddNewLocalLink failed.");
        pLocalLink->iPID = TMdbOS::GetPID();
        pLocalLink->iTID = TMdbOS::GetTID();
        pLocalLink->cState = Link_use;
        pLocalLink->iLogLevel = m_pDsn->iLogLevel;//日志级别暂时没用
        
        //链接 回滚链表到共享内存
        int iRet = pLocalLink->m_RBList.Attach(m_tMgrShmAlloc,pLocalLink->iRBAddrOffset);
		CHECK_RET(iRet,"Attach RBList of LoaclLink Failed.");
		//申请事务ID
        pLocalLink->iSessionID = m_pDsn->GetSessionID();
		pLocalLink->iExecuteID = 0;
		
        SAFESTRCPY(pLocalLink->sStartTime,MAX_TIME_LEN,m_pShmDsn->GetInfo()->sCurTime);
        SAFESTRCPY(pLocalLink->sFreshTime,MAX_TIME_LEN,m_pShmDsn->GetInfo()->sCurTime);
    }while(0);
    CHECK_RET(m_pShmDsn->UnLockDSN(),"unlock failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  UnRegLocalLink
* 函数描述	:  注销本地链接管理
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::UnRegLocalLink(TMdbLocalLink *& pLocalLink)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pLocalLink);
    CHECK_RET(m_pDsn->tMutex.Lock(true,&(m_pDsn->tCurTime)), "lock failed.");//加锁
    
    TMdbRBCtrl tRBCtrl;
	tRBCtrl.Init(m_pShmDsn, pLocalLink);
	tRBCtrl.RollBack();	
	
	pLocalLink->ReturnAllIndexNodeToTable(m_pShmDsn);
    pLocalLink->Clear();
    pLocalLink = NULL;
    CHECK_RET(m_pDsn->tMutex.UnLock(true),"unlock failed.");//加锁
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  RegRemoteLink
* 函数描述	:  注册远程链接
* 输入		:  ptClient - 客户端信息
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::RegRemoteLink(TMdbCSLink &tCSLink,int iAgentPort,bool bMasterPort)
{
    TADD_FUNC("Start");
    int iRet = 0;
    TMdbRemoteLink * pRemoteLink = NULL;

    CHECK_OBJ(m_pShmDsn);
    CHECK_RET(m_pShmDsn->LockDSN(),"lock failed.");
    do{
        CHECK_RET_BREAK(m_pShmDsn->AddNewRemoteLink(pRemoteLink),"AddNewRemoteLink failed.");
        if(0 == tCSLink.m_sClientIP[0])
        {
            SAFESTRCPY(pRemoteLink->sIP, MAX_IP_LEN,inet_ntoa(tCSLink.tAddr.sin_addr));
            SAFESTRCPY(tCSLink.m_sClientIP, MAX_IP_LEN,pRemoteLink->sIP);
        }
        else
        {
            SAFESTRCPY(pRemoteLink->sIP, MAX_IP_LEN,tCSLink.m_sClientIP);
        }
        pRemoteLink->iHandle = tCSLink.iFD;            //链接句柄
        pRemoteLink->cState = Link_use;        //链接状态
        SAFESTRCPY(pRemoteLink->sStartTime,MAX_TIME_LEN,m_pShmDsn->GetInfo()->sCurTime);
        SAFESTRCPY(pRemoteLink->sFreshTime,MAX_TIME_LEN,m_pShmDsn->GetInfo()->sCurTime);
        //TMdbDateTime::GetCurrentTimeStr(pRemoteLink->sStartTime);
        //TMdbDateTime::GetCurrentTimeStr(pRemoteLink->sFreshTime);
        pRemoteLink->iLogLevel = m_pDsn->iLogLevel;//日志级别暂时没用
        SAFESTRCPY(pRemoteLink->sUser,MAX_NAME_LEN, tCSLink.m_sUser);     //用户名
        SAFESTRCPY(pRemoteLink->sPass,MAX_NAME_LEN, tCSLink.m_sPass);     //密码
        SAFESTRCPY(pRemoteLink->sDSN, MAX_NAME_LEN, tCSLink.m_sDSN);      //DSN名称
        pRemoteLink->iLowPriority = tCSLink.m_iLowPriority;
        pRemoteLink->iPID = (tCSLink.m_iClientPID);  //客户端PID
        pRemoteLink->iTID = (tCSLink.m_iClientTID);  //客户端线程ID
        pRemoteLink->iProtocol = tCSLink.m_iUseOcp;
        pRemoteLink->iPort = iAgentPort;
        tCSLink.m_pRemoteLink =pRemoteLink;

		if(iAgentPort != -1 && !bMasterPort)
			AddConNumForPort(iAgentPort);

    }while(0);
    CHECK_RET(m_pShmDsn->UnLockDSN(),"unlock failed.");
    TADD_FUNC("Finish.");
    return iRet;
}
/******************************************************************************
* 函数名称	:  UnRegRemoteLink
* 函数描述	:  注销远程链接
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::UnRegRemoteLink(TMdbCSLink &tCSLink,int iAgentPort)
{
    TADD_FUNC("Start.");
     //找出远程链接信息
    int iRet = 0;
    TMdbRemoteLink * pRemoteLink = tCSLink.m_pRemoteLink;
    if(NULL == pRemoteLink){return 0;}
    CHECK_RET(m_pShmDsn->LockDSN(),"lock failed.");
    if(strcmp(pRemoteLink->sIP, tCSLink.m_sClientIP) ==0 && pRemoteLink->iHandle == tCSLink.iFD)
    {
        pRemoteLink->Clear();
    }

	if(iAgentPort != -1)
		MinusConNumForPort(iAgentPort);
    CHECK_RET(m_pShmDsn->UnLockDSN(),"unlock failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

//注册rep链接
int TMdbLinkCtrl::RegRepLink(TMdbRepLink &tRepLink)
{
    int iRet = 0;
    TMdbRepLink * pRepLink = NULL;
    CHECK_RET(m_pShmDsn->LockDSN(),"lock failed.");
    do{
        CHECK_RET_BREAK(m_pShmDsn->AddNewRepLink(pRepLink),"AddNewRepLink failed.");
        pRepLink->iSocketID = tRepLink.iSocketID;
        pRepLink->iPort = tRepLink.iPort;
        SAFESTRCPY(pRepLink->sIP, MAX_IP_LEN,tRepLink.sIP);
        TMdbDateTime::GetCurrentTimeStr(pRepLink->sStartTime);
        pRepLink->iPID      = TMdbOS::GetPID();
        pRepLink->sState = tRepLink.sState;
        pRepLink->sLink_Type = tRepLink.sLink_Type;
    }while(0);
    CHECK_RET(m_pShmDsn->UnLockDSN(),"unlock failed.");
    return iRet;
}
//注销rep链接
int TMdbLinkCtrl::UnRegRepLink(int iSocket,char cState)
{
    int iRet = 0;
    CHECK_RET(m_pDsn->tMutex.Lock(true,&(m_pDsn->tCurTime)),"lock failed.");//加锁
    TShmList<TMdbRepLink>::iterator itor = m_pShmDsn->m_RepLinkList.begin();
    for(;itor != m_pShmDsn->m_RepLinkList.end();++itor)
    {
        if(itor->iSocketID == iSocket && itor->sLink_Type == cState)
        {
            itor->Clear();
        }
    }
    CHECK_RET(m_pDsn->tMutex.UnLock(true),"unlock failed.");//加锁
    return iRet;
}
/******************************************************************************
* 函数名称	:  ClearInvalidLink
* 函数描述	:  清除无效链接
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::ClearInvalidLink()
{
    int iRet = 0;
    TADD_FUNC(" Start.");
    CHECK_OBJ(m_pShmDsn);
    CHECK_RET(m_pDsn->tMutex.Lock(true,&(m_pDsn->tCurTime)),"lock failed.");//加锁
    TShmList<TMdbLocalLink >::iterator itor  = m_pShmDsn->m_LocalLinkList.begin();
    for(;itor != m_pShmDsn->m_LocalLinkList.end();++itor)
    {
        if(itor->iPID > 0 && NULL == m_pShmDsn->GetProcByPid(itor->iPID) &&
            false == TMdbOS::IsProcExistByPopen(itor->iPID))
        {        
            TADD_NORMAL("Clear Link=[PID=%d,TID=%d].",itor->iPID,itor->iTID); 
			itor->m_RBList.Attach(m_tMgrShmAlloc,itor->iRBAddrOffset);

			TMdbRBCtrl tRBCtrl;
			tRBCtrl.Init(m_pShmDsn, &(*itor));
			tRBCtrl.RollBack();	
			
			itor->ReturnAllIndexNodeToTable(m_pShmDsn);
            itor->Clear();
        }
    }
    TShmList<TMdbRepLink>::iterator itorRep  = m_pShmDsn->m_RepLinkList.begin();
    for(;itorRep != m_pShmDsn->m_RepLinkList.end();++itorRep)
    {
        if(itorRep->iPID > 0 && false == TMdbOS::IsProcExistByKill(itorRep->iPID))
        {
            TADD_NORMAL("Clear Link=[PID=%d,SocketID=%d].", itorRep->iPID,itorRep->iSocketID);
            itorRep->Clear();
        }
    }
    CHECK_RET(m_pDsn->tMutex.UnLock(true),"unlock failed.");//加锁
    //解锁
    TADD_FUNC("Finish.");	
    return iRet;
}
/******************************************************************************
* 函数名称	:  ClearRemoteLink
* 函数描述	:  清理远程链接
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::ClearRemoteLink(int iPort)
{
    TADD_FUNC("Start");
    int iRet = 0;
    CHECK_RET(m_pShmDsn->LockDSN(),"lock failed.");
    //m_pShmDsn->m_RemoteLinkList.clear();
    TShmList<TMdbRemoteLink>::iterator itor = m_pShmDsn->m_RemoteLinkList.begin();
    //先搜寻空闲的
    for(;itor != m_pShmDsn->m_RemoteLinkList.end();++itor)
    {
        if(strlen(itor->sIP) != 0 &&(iPort == 0 || iPort == itor->iPort))
        {
            TADD_NORMAL("Clean up the remote link,HostIP = [%s],Handle = [%d],Port = [%d]",\
                itor->sIP,itor->iHandle,itor->iPort);
            itor->Clear();
        }
    }
    CHECK_RET(m_pShmDsn->UnLockDSN(),"unlock failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* 函数名称	:  ClearAllLink
* 函数描述	:  清除所有链接
* 输入		:  
* 输入		:  
* 输出		:  
* 返回值	:  0 - 成功!0 -失败
* 作者		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::ClearAllLink()
{
    int iRet = 0;
    TADD_FUNC(" Start.");
    CHECK_OBJ(m_pShmDsn);
    CHECK_RET(m_pDsn->tMutex.Lock(true,&(m_pDsn->tCurTime)),"lock failed.");//加锁
    //本地链接
    TShmList<TMdbLocalLink >::iterator itorLocal  = m_pShmDsn->m_LocalLinkList.begin();
    for(;itorLocal != m_pShmDsn->m_LocalLinkList.end();++itorLocal)
    {
        if(itorLocal->iPID > 0)
        {        
            TADD_NORMAL("Clear Link=[PID=%d,TID=%d].",itorLocal->iPID,itorLocal->iTID);
			
			itorLocal->m_RBList.Attach(m_tMgrShmAlloc,itorLocal->iRBAddrOffset);
			TMdbRBCtrl tRBCtrl;
			tRBCtrl.Init(m_pShmDsn, &(*itorLocal));
			tRBCtrl.RollBack();	
			
			itorLocal->ReturnAllIndexNodeToTable(m_pShmDsn);
            itorLocal->Clear();
        }
    }
    //同步进程链接
    TShmList<TMdbRepLink>::iterator itorRep  = m_pShmDsn->m_RepLinkList.begin();
    for(;itorRep != m_pShmDsn->m_RepLinkList.end();++itorRep)
    {
        if(itorRep->iPID > 0)
        {
            TADD_NORMAL("Clear Link=[PID=%d,SocketID=%d].", itorRep->iPID,itorRep->iSocketID);
            itorRep->Clear();
        }
    }
    //远程链接
    TShmList<TMdbRemoteLink>::iterator itorRemote = m_pShmDsn->m_RemoteLinkList.begin();
    for(;itorRemote != m_pShmDsn->m_RemoteLinkList.end();++itorRemote)
    {
        if(strlen(itorRemote->sIP) != 0)
        {
            TADD_NORMAL("Clean up the remote link,HostIP = [%s],Handle = [%d]",\
                itorRemote->sIP,itorRemote->iHandle);
            itorRemote->Clear();
        }
    }

	//清除cs连接的统计信息
	
	/*
	for(int i=0; i<MAX_AGENT_PORT_COUNTS; i++)
	{
		m_pShmDsn->iConnectNum[i] = 0;
		m_pShmDsn->iNoNtcAgentPorts[i] = -1;
	}
	*/

	TShmList<TMdbPortLink>::iterator itorPort = m_pShmDsn->m_PortLinkList.begin();
    for(;itorPort != m_pShmDsn->m_PortLinkList.end();++itorPort)
    {
        if(itorPort->iAgentPort != -1)
        {
            TADD_NORMAL("Clean up the remote link,port = [%d],connum = [%d]",\
                itorPort->iAgentPort,itorPort->iConNum);
            itorPort->Clear();
        }
    }
	
	
    CHECK_RET(m_pDsn->tMutex.UnLock(true),"unlock failed.");//加锁
    //解锁
    TADD_FUNC("Finish.");	
    return iRet;
}

int   TMdbLinkCtrl::ClearCntNumForPort(int iAgentPort)
{
	int iRet = 0;
    TADD_FUNC(" Start.");
    CHECK_OBJ(m_pShmDsn);
   	CHECK_RET(m_pShmDsn->LockDSN(),"lock failed.");
	//int i;
	//TADD_NORMAL("ClearCntNumForPort() : Start_mjx.");
	TADD_NORMAL("iAgentPort %d",iAgentPort);
	/*
	for(i=0; i<MAX_AGENT_PORT_COUNTS; i++)
	 if(-1 ==  m_pShmDsn->iNoNtcAgentPorts[i])
		{
			m_pShmDsn->iNoNtcAgentPorts[i] = iAgentPort;
			m_pShmDsn->iConnectNum[i] = 0;
			//TADD_NORMAL("iNoNtcAgentPorts %d, i:%d",m_pShmDsn->iNoNtcAgentPorts[i],i);
			break;
	 	}
	
	*/
	 TMdbPortLink * pPortLink = NULL;
	 CHECK_RET(m_pShmDsn->AddNewPortLink(pPortLink),"AddNewPortLink failed.");
	 pPortLink->iAgentPort = iAgentPort;
	 pPortLink->iConNum = 0;
	
	
	CHECK_RET(m_pShmDsn->UnLockDSN(),"unlock failed.");
    //解锁
    TADD_FUNC("Finish.");	
    return iRet;

}

int   TMdbLinkCtrl::GetCsAgentPort(int iClientPort)
{
	
    int iRet = 0;
	int iRetPort = 0;
    TADD_FUNC(" Start.");
    CHECK_OBJ(m_pShmDsn);
   	CHECK_RET(m_pShmDsn->LockDSN(),"lock failed.");
	//最大连接数的端口号，最小连接数的端口号 ，二者相差不超过5即可
	int iMax = -1,iMin = 65536;
	int iMaxPort = -1,iMinPort = -1;
	TShmList<TMdbPortLink>::iterator itorPort = m_pShmDsn->m_PortLinkList.begin();
    for(;itorPort != m_pShmDsn->m_PortLinkList.end();++itorPort)
	{
		if(itorPort->iAgentPort != -1 && itorPort->iConNum > iMax)
		{
			iMax = itorPort->iConNum;
			iMaxPort = itorPort->iAgentPort;
			
		}

		if(itorPort->iAgentPort != -1 && itorPort->iConNum < iMin)
		{
			iMin = itorPort->iConNum;
			iMinPort = itorPort->iAgentPort;
			
		}

		//TADD_NORMAL(" no ntc port numm %d, con num %d",itorPort->iAgentPort,itorPort->iConNum);

	}

	//TADD_NORMAL("GetCsAgentPort:iMax%d,iMaxPort %d,iMin%d,iMinPort%d",iMax,iMaxPort,iMin,iMinPort);
	
	if(iMaxPort>-1 && iMinPort> -1 && iMax-iMin>= CONNECT_DIFF)
		iRetPort = iMinPort;
	else
		iRetPort = iClientPort;

	

	//TADD_NORMAL("GetCsAgentPort:iRet%d,iClientPort%d",iRetPort,iClientPort);
	
	CHECK_RET(m_pShmDsn->UnLockDSN(),"unlock failed.");
    //解锁
    TADD_FUNC("Finish.");	

	iRet = iRetPort;
	//TADD_NORMAL(" return :GetCsAgentPort:iRet%d,iClientPort%d",iRetPort,iClientPort);
    return iRet;
	
	

}

int   TMdbLinkCtrl::GetCsAgentPortBaseOnMasterPort(int iClientPort)
{
	
    int iRet = 0;
	int iRetPort = 0;
    TADD_FUNC(" Start.");
    CHECK_OBJ(m_pShmDsn);
   	CHECK_RET(m_pShmDsn->LockDSN(),"lock failed.");
	//最大连接数的端口号，最小连接数的端口号 ，二者相差不超过5即可
	int iMin = 65536;
	int iMinPort = -1;
	TShmList<TMdbPortLink>::iterator itorPort = m_pShmDsn->m_PortLinkList.begin();
    for(;itorPort != m_pShmDsn->m_PortLinkList.end();++itorPort)
	{
		if(itorPort->iAgentPort > 0 && itorPort->iConNum < iMin)
		{
			iMin = itorPort->iConNum;
			iMinPort = itorPort->iAgentPort;
		}
		//TADD_NORMAL(" no ntc port numm %d, con num %d",itorPort->iAgentPort,itorPort->iConNum);
	}
    iRetPort = (iMinPort > 0) ? iMinPort : iClientPort;
	//TADD_NORMAL("iRet=%d,iClientPort=%d",iRetPort,iClientPort);
    for(itorPort = m_pShmDsn->m_PortLinkList.begin();itorPort != m_pShmDsn->m_PortLinkList.end();++itorPort)
	{
		if(itorPort->iAgentPort == iRetPort)
		{
			itorPort->iConNum++;
			break;
		}
	}
	CHECK_RET(m_pShmDsn->UnLockDSN(),"unlock failed.");
    //解锁
    TADD_FUNC("Finish.");	
	iRet = iRetPort;
	//TADD_NORMAL(" return :GetCsAgentPort:iRet%d,iClientPort%d",iRetPort,iClientPort);
    return iRet;
}

int	  TMdbLinkCtrl::AddConNumForPort(int iAgentPort)
{
	
	 //int i;
	 //for(i=0; i<MAX_AGENT_PORT_COUNTS; i++)
	 //	if(iAgentPort ==  m_pShmDsn->iNoNtcAgentPorts[i])
	 TShmList<TMdbPortLink>::iterator itorPort = m_pShmDsn->m_PortLinkList.begin();
     for(;itorPort != m_pShmDsn->m_PortLinkList.end();++itorPort)
	{
		if(itorPort->iAgentPort ==  iAgentPort)
		{
			itorPort->iConNum++;
			break;
		}
	 }
	 return 0;

}
int	  TMdbLinkCtrl::MinusConNumForPort(int iAgentPort)
{
	//int i;
	// for(i=0; i<MAX_AGENT_PORT_COUNTS; i++)
	// 	if(iAgentPort ==  m_pShmDsn->iNoNtcAgentPorts[i])
	TShmList<TMdbPortLink>::iterator itorPort = m_pShmDsn->m_PortLinkList.begin();
    for(;itorPort != m_pShmDsn->m_PortLinkList.end();++itorPort)
	{
		if(itorPort->iAgentPort ==  iAgentPort)
		{
			if(itorPort->iConNum > 0 )
				itorPort->iConNum--;
			break;
		}
			
	}
	return 0;
}


