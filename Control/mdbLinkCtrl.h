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

//namespace QuickMDB{

//class TAgentClient;
//链接管理
class TMdbLinkCtrl
{
public:
	TMdbLinkCtrl();
	~TMdbLinkCtrl();
	int Attach(const char * sDsn);//链接共享内存
	int RegLocalLink(TMdbLocalLink *& pLocalLink);//注册本地链接
	int UnRegLocalLink(TMdbLocalLink *& pLocalLink);//注销本地链接管理
	int RegRemoteLink(TMdbCSLink &tCSLink);//注册远程链接
	int UnRegRemoteLink(TMdbCSLink &tCSLink);//注销远程链接
	int RegRepLink(TMdbRepLink &tRepLink);//注册rep链接
	int UnRegRepLink(int iSocket,char cState);//注册rep链接
	int ClearInvalidLink();//清除无效链接
	int ClearRemoteLink();//清理远程链接
	int ClearAllLink();//清除所有链接
private:
	TMdbShmDSN * m_pShmDsn;//管理区
	TMdbDSN       *  m_pDsn;//dsn信息	
	TShmAlloc m_tMgrShmAlloc;//共享内存分配器
	char* m_pMgrAddr;
};


//}
#endif
