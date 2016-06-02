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
//#include "Agent/mdbAgentServer.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbDateTime.h"

//namespace QuickMDB{

TMdbLinkCtrl::TMdbLinkCtrl():
m_pShmDsn(NULL),
m_pDsn(NULL)
{

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
        
        //链接回滚链表到共享内存
        int iRet = pLocalLink->m_RBList.Attach(m_tMgrShmAlloc,pLocalLink->iRBAddrOffset);
		CHECK_RET(iRet,"Attach RBList of LoaclLink Failed.");
		//申请事务ID
        pLocalLink->iSessionID = m_pDsn->GetSessionID();
		
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
    pLocalLink->RollBack();
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
int TMdbLinkCtrl::RegRemoteLink(TMdbCSLink &tCSLink,int iAgentPort)
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
        tCSLink.m_pRemoteLink =pRemoteLink;

		if(iAgentPort != -1)
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
			itor->RollBack();
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
int TMdbLinkCtrl::ClearRemoteLink()
{
    TADD_FUNC("Start");
    int iRet = 0;
    CHECK_RET(m_pShmDsn->LockDSN(),"lock failed.");
    //m_pShmDsn->m_RemoteLinkList.clear();
    TShmList<TMdbRemoteLink>::iterator itor = m_pShmDsn->m_RemoteLinkList.begin();
    //先搜寻空闲的
    for(;itor != m_pShmDsn->m_RemoteLinkList.end();++itor)
    {
        if(strlen(itor->sIP) != 0)
        {
            TADD_NORMAL("Clean up the remote link,HostIP = [%s],Handle = [%d]",\
                itor->sIP,itor->iHandle);
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
	TADD_NORMAL("ClearCntNumForPort() : Start_mjx.");
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


//}
