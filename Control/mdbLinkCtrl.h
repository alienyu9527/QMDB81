/****************************************************************************************
*@Copyrights  2012，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbLinkCtrl.h		
*@Description： mdb链接管理
*@Author:	jin.shaohua
*@Date：	    2012.11
*@History:
******************************************************************************************/
#ifndef _MDB_LINK_CTRL_H_
#define _MDB_LINK_CTRL_H_
#include "Helper/mdbStruct.h"
#include "Helper/mdbDictionary.h"

#include "Control/mdbMgrShm.h"
#include "Control/mdbTableWalker.h"
#include "Control/mdbExecuteEngine.h"


//链接上只需要记录"需要动态申请的索引"的信息
struct ST_LINK_INDEX_INFO
{
    bool bInit;//是否被初始化
	char sName[MAX_NAME_LEN]; //索引名称
	int  iAlgoType;  // 索引的算法类型: hash , multistep hash, btree

  //Hash
	//conflict
	int iHashCFreeHeadPos;   
	int iHashCFreeNodeCounts;

  //Mhash
	//conflict
	int iMHashCFreeHeadPos; 
	int iMHashCFreeNodeCounts;
	//Layer
	int iMHashLFreeHeadPos;
	int iMHashLFreeNodeCounts;

  //Trie
	//conflict
	int iTrieCFreeHeadPos; 
	int iTrieCFreeNodeCounts;
	//Branch
	int iTrieBFreeHeadPos;
	int iTrieBFreeNodeCounts;

    void Clear()
    {
        bInit = false;
		sName[0] = 0;
		iAlgoType = INDEX_UNKOWN;
		iHashCFreeHeadPos = -1;
		iHashCFreeNodeCounts = 0;
		iMHashCFreeHeadPos = -1;
		iMHashCFreeNodeCounts = 0;
		iMHashLFreeHeadPos = -1;
		iMHashLFreeNodeCounts = 0;
		iTrieCFreeHeadPos = -1;
		iTrieCFreeNodeCounts  =0;
		iTrieBFreeHeadPos = -1;
		iTrieBFreeNodeCounts = 0;
    }

	void Print()
	{
		printf("[IndexName:%s].",sName);
		switch(iAlgoType)
		{
			case INDEX_HASH:
			{
				printf("[Hash] ");
				printf("[Conflict FreeHeadPos:%d]",iHashCFreeHeadPos);
				printf("[Conflict FreeNodeCounts:%d]",iHashCFreeNodeCounts);
				break;
			}
			/*
			case INDEX_M_HASH:
			{
				printf("[MHash]");
				printf("[Conflict FreeHeadPos:%d]",iMHashCFreeHeadPos);
				printf("[Conflict FreeNodeCounts:%d]",iMHashCFreeNodeCounts);

				printf("[Layer FreeHeadPos:%d]",iMHashLFreeHeadPos);
				printf("[Layer FreeNodeCounts:%d]",iMHashLFreeNodeCounts);
				break;
			}
			case INDEX_TRIE:
			{
				printf("[Trie]");
				printf("[Conflict FreeHeadPos:%d]",iTrieCFreeHeadPos);
				printf("[Conflict FreeNodeCounts:%d]",iTrieCFreeNodeCounts);

				printf("[Branch FreeHeadPos:%d]",iTrieBFreeHeadPos);
				printf("[Branch FreeNodeCounts:%d]",iTrieBFreeNodeCounts);
				break;
			}*/
			default:
				break;
		}
		printf("\n");
	}

};


//连接上 记录表的全部索引信息
class TMdbSingleTableIndexInfo
{	
	public:
		TMdbSingleTableIndexInfo();
		
		void Clear()
		{
			sTableName[0]=0;
			for(int i=0;i<MAX_INDEX_COUNTS;i++)
			{
				arrLinkIndex[i].Clear();
			}

		}
		
	public:
		char  sTableName[MAX_NAME_LEN];
		ST_LINK_INDEX_INFO	 arrLinkIndex[MAX_INDEX_COUNTS];
};


//回滚单元
class TRBRowUnit 
{	
	public:
		TRBRowUnit();
		~TRBRowUnit(){}
		void Show();
		
	public:
	 	char  sTableName[MAX_NAME_LEN];
		char	SQLType;		//Insert or  delete or update
	 	unsigned int 	iRealRowID;		//代表共享内存中原始数据记录位置
	 	unsigned int  	iVirtualRowID;  	//代表新增的数据行记录位置，影子内存， commit之前为独享数据

	private:		
		const char* GetSQLName();		
		
};

//回滚管理器
class TMdbRBCtrl
{
	public:
		TMdbRBCtrl();
		~TMdbRBCtrl();
	
		int Init(TMdbShmDSN* pShmDSN, TMdbLocalLink* pLocalLink);		
		int Commit();
		int RollBack();
		void ShowRBUnits();


	private:	
		int Commit(TRBRowUnit* pRBRowUnit);
		int RollBack(TRBRowUnit* pRBRowUnit);
		
		int CommitInsert(TRBRowUnit* pRBRowUnit);
		int CommitUpdate(TRBRowUnit* pRBRowUnit);
		int CommitDelete(TRBRowUnit* pRBRowUnit);
		
		int RollBackInsert(TRBRowUnit* pRBRowUnit);
		int RollBackUpdate(TRBRowUnit* pRBRowUnit);
		int RollBackDelete(TRBRowUnit* pRBRowUnit);

		
		int CheckPK(TMdbTable* pTable, char* pDataAddr);
		int UnLockRow(char* pDataAddr);	

		TMdbShmDSN* 	m_pShmDSN;
		TMdbLocalLink*  m_pLocalLink;
		
		TMdbFlushTrans  	m_tFlushTrans;
		TMdbTableWalker 	m_tTableWalker;
		TMdbExecuteEngine 	m_tEngine;

		int m_iDataSize;
		
};


//数据库本地链接信息
class TMdbLocalLink
{
public:
    void Clear();
    void Print();
    void Show(bool bIfMore=false);
    bool IsValid(){return (iPID > 0 && 0 != sStartTime[0]);};//是否是合法的
    bool IsCurrentThreadLink();//是否是当前线程的链接

	int AddNewRBRowUnit(TRBRowUnit* pRBRowUnit);
	void ShowRBUnits();
	void ShowIndexInfo();	
	int ReturnAllIndexNodeToTable(TMdbShmDSN * pShmDSN);
	
	TMdbSingleTableIndexInfo*  FindCurTableIndex(const char* sTableName);
	
public:
    int iPID;         //进程的PID
    unsigned long int iTID;         //进程的Thread-ID
    int iSocketID;    //链接ID
    char cState;      //链接状态
    char sStartTime[MAX_TIME_LEN]; //链接启动时间
    char sFreshTime[MAX_TIME_LEN]; //链接最近更新时间，相当于心跳
    int  iLogLevel;                //链接的日志级别
    char cAccess;                  //权限, A-管理员;W-可读写;R-只读
    int  iSQLPos;     //当前执行的SQL位置

    int  iQueryCounts;  //查询次数
    int  iQueryFailCounts;  //查询次数

    int  iInsertCounts; //插入次数
    int  iInsertFailCounts; //插入次数

    int  iUpdateCounts; //更新次数
    int  iUpdateFailCounts; //更新次数

    int  iDeleteCounts; //删除次数
    int  iDeleteFailCounts; //删除次数

    char sProcessName[MAX_NAME_LEN];   //哪个进程触发链接
	
    unsigned int iSessionID; //事务id
    unsigned int iExecuteID; //sql执行id
	
    TShmList<TRBRowUnit>  m_RBList; //回滚链表
    int iAffect;
	size_t  iRBAddrOffset;

	//需要保存多张表的索引信息
	TMdbSingleTableIndexInfo m_AllTableIndexInfo[MAX_TABLE_COUNTS];
};

//链接管理
class TMdbLinkCtrl
{
public:
	TMdbLinkCtrl();
	~TMdbLinkCtrl();
	int Attach(const char * sDsn);//链接共享内存
	int RegLocalLink(TMdbLocalLink *& pLocalLink);//注册本地链接
	int UnRegLocalLink(TMdbLocalLink *& pLocalLink);//注销本地链接管理
	int RegRemoteLink(TMdbCSLink &tCSLink,int iAgentPort =-1,bool bMasterPort=false);//注册远程链接
	int UnRegRemoteLink(TMdbCSLink &tCSLink,int iAgentPort =-1);//注销远程链接
	int RegRepLink(TMdbRepLink &tRepLink);//注册rep链接
	int UnRegRepLink(int iSocket,char cState);//注册rep链接
	int ClearInvalidLink();//清除无效链接
	int ClearRemoteLink(int iPort = 0);//清理远程链接
	int ClearAllLink();//清除所有链接
	int   GetCsAgentPort(int iClientPort);//根据客户端发过来的端口号，结合其他端口号判断，取一个合适的
	int   GetCsAgentPortBaseOnMasterPort(int iClientPort);//根据客户端发过来的端口号，结合其他端口号判断，取一个合适的
	int	  AddConNumForPort(int iAgentPort);//连接成功后，连接数增加
	int	  MinusConNumForPort(int iAgentPort);//连接断开后，连接数减1
	int   ClearCntNumForPort(int iAgentPort);//agent初始化时，连接数清0
private:
	TMdbShmDSN * m_pShmDsn;//管理区
	TMdbDSN       *  m_pDsn;//dsn信息	
	TShmAlloc m_tMgrShmAlloc;//共享内存分配器
	char* m_pMgrAddr;
};



#endif
