/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbLinkCtrl.cpp		
*@Description�� ����ע��ģ���Ż�����
*@Author:			jin.shaohua
*@Date��	    2012.11
*@History:
******************************************************************************************/
#include "Control/mdbLinkCtrl.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbDateTime.h"


TMdbLinkCtrl::TMdbLinkCtrl()
{
	m_pShmDsn = NULL;
	m_pDsn = NULL;
}
TMdbLinkCtrl::~TMdbLinkCtrl()
{

}
/******************************************************************************
* ��������	:  Attach
* ��������	:  ���ӹ����ڴ�
* ����		:  sDsn - dsn��
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
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
* ��������	:  RegLocalLink
* ��������	:  ע�᱾������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
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
        pLocalLink->iLogLevel = m_pDsn->iLogLevel;//��־������ʱû��
        
        //���� �ع����������ڴ�
        int iRet = pLocalLink->m_RBList.Attach(m_tMgrShmAlloc,pLocalLink->iRBAddrOffset);
		CHECK_RET(iRet,"Attach RBList of LoaclLink Failed.");
		//��������ID
        pLocalLink->iSessionID = m_pDsn->GetSessionID();
		
        SAFESTRCPY(pLocalLink->sStartTime,MAX_TIME_LEN,m_pShmDsn->GetInfo()->sCurTime);
        SAFESTRCPY(pLocalLink->sFreshTime,MAX_TIME_LEN,m_pShmDsn->GetInfo()->sCurTime);
    }while(0);
    CHECK_RET(m_pShmDsn->UnLockDSN(),"unlock failed.");
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  UnRegLocalLink
* ��������	:  ע���������ӹ���
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::UnRegLocalLink(TMdbLocalLink *& pLocalLink)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pLocalLink);
    CHECK_RET(m_pDsn->tMutex.Lock(true,&(m_pDsn->tCurTime)), "lock failed.");//����
    pLocalLink->RollBack(m_pShmDsn);
    pLocalLink->Clear();
    pLocalLink = NULL;
    CHECK_RET(m_pDsn->tMutex.UnLock(true),"unlock failed.");//����
    TADD_FUNC("Finish.");
    return iRet;
}

/******************************************************************************
* ��������	:  RegRemoteLink
* ��������	:  ע��Զ������
* ����		:  ptClient - �ͻ�����Ϣ
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
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
        pRemoteLink->iHandle = tCSLink.iFD;            //���Ӿ��
        pRemoteLink->cState = Link_use;        //����״̬
        SAFESTRCPY(pRemoteLink->sStartTime,MAX_TIME_LEN,m_pShmDsn->GetInfo()->sCurTime);
        SAFESTRCPY(pRemoteLink->sFreshTime,MAX_TIME_LEN,m_pShmDsn->GetInfo()->sCurTime);
        //TMdbDateTime::GetCurrentTimeStr(pRemoteLink->sStartTime);
        //TMdbDateTime::GetCurrentTimeStr(pRemoteLink->sFreshTime);
        pRemoteLink->iLogLevel = m_pDsn->iLogLevel;//��־������ʱû��
        SAFESTRCPY(pRemoteLink->sUser,MAX_NAME_LEN, tCSLink.m_sUser);     //�û���
        SAFESTRCPY(pRemoteLink->sPass,MAX_NAME_LEN, tCSLink.m_sPass);     //����
        SAFESTRCPY(pRemoteLink->sDSN, MAX_NAME_LEN, tCSLink.m_sDSN);      //DSN����
        pRemoteLink->iLowPriority = tCSLink.m_iLowPriority;
        pRemoteLink->iPID = (tCSLink.m_iClientPID);  //�ͻ���PID
        pRemoteLink->iTID = (tCSLink.m_iClientTID);  //�ͻ����߳�ID
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
* ��������	:  UnRegRemoteLink
* ��������	:  ע��Զ������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::UnRegRemoteLink(TMdbCSLink &tCSLink,int iAgentPort)
{
    TADD_FUNC("Start.");
     //�ҳ�Զ��������Ϣ
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

//ע��rep����
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
//ע��rep����
int TMdbLinkCtrl::UnRegRepLink(int iSocket,char cState)
{
    int iRet = 0;
    CHECK_RET(m_pDsn->tMutex.Lock(true,&(m_pDsn->tCurTime)),"lock failed.");//����
    TShmList<TMdbRepLink>::iterator itor = m_pShmDsn->m_RepLinkList.begin();
    for(;itor != m_pShmDsn->m_RepLinkList.end();++itor)
    {
        if(itor->iSocketID == iSocket && itor->sLink_Type == cState)
        {
            itor->Clear();
        }
    }
    CHECK_RET(m_pDsn->tMutex.UnLock(true),"unlock failed.");//����
    return iRet;
}
/******************************************************************************
* ��������	:  ClearInvalidLink
* ��������	:  �����Ч����
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::ClearInvalidLink()
{
    int iRet = 0;
    TADD_FUNC(" Start.");
    CHECK_OBJ(m_pShmDsn);
    CHECK_RET(m_pDsn->tMutex.Lock(true,&(m_pDsn->tCurTime)),"lock failed.");//����
    TShmList<TMdbLocalLink >::iterator itor  = m_pShmDsn->m_LocalLinkList.begin();
    for(;itor != m_pShmDsn->m_LocalLinkList.end();++itor)
    {
        if(itor->iPID > 0 && NULL == m_pShmDsn->GetProcByPid(itor->iPID) &&
            false == TMdbOS::IsProcExistByPopen(itor->iPID))
        {        
            TADD_NORMAL("Clear Link=[PID=%d,TID=%d].",itor->iPID,itor->iTID); 
			itor->RollBack(m_pShmDsn);
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
    CHECK_RET(m_pDsn->tMutex.UnLock(true),"unlock failed.");//����
    //����
    TADD_FUNC("Finish.");	
    return iRet;
}
/******************************************************************************
* ��������	:  ClearRemoteLink
* ��������	:  ����Զ������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::ClearRemoteLink()
{
    TADD_FUNC("Start");
    int iRet = 0;
    CHECK_RET(m_pShmDsn->LockDSN(),"lock failed.");
    //m_pShmDsn->m_RemoteLinkList.clear();
    TShmList<TMdbRemoteLink>::iterator itor = m_pShmDsn->m_RemoteLinkList.begin();
    //����Ѱ���е�
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
* ��������	:  ClearAllLink
* ��������	:  �����������
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbLinkCtrl::ClearAllLink()
{
    int iRet = 0;
    TADD_FUNC(" Start.");
    CHECK_OBJ(m_pShmDsn);
    CHECK_RET(m_pDsn->tMutex.Lock(true,&(m_pDsn->tCurTime)),"lock failed.");//����
    //��������
    TShmList<TMdbLocalLink >::iterator itorLocal  = m_pShmDsn->m_LocalLinkList.begin();
    for(;itorLocal != m_pShmDsn->m_LocalLinkList.end();++itorLocal)
    {
        if(itorLocal->iPID > 0)
        {        
            TADD_NORMAL("Clear Link=[PID=%d,TID=%d].",itorLocal->iPID,itorLocal->iTID); 
            itorLocal->Clear();
        }
    }
    //ͬ����������
    TShmList<TMdbRepLink>::iterator itorRep  = m_pShmDsn->m_RepLinkList.begin();
    for(;itorRep != m_pShmDsn->m_RepLinkList.end();++itorRep)
    {
        if(itorRep->iPID > 0)
        {
            TADD_NORMAL("Clear Link=[PID=%d,SocketID=%d].", itorRep->iPID,itorRep->iSocketID);
            itorRep->Clear();
        }
    }
    //Զ������
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

	//���cs���ӵ�ͳ����Ϣ
	
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
	
	
    CHECK_RET(m_pDsn->tMutex.UnLock(true),"unlock failed.");//����
    //����
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
    //����
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
	//����������Ķ˿ںţ���С�������Ķ˿ں� ������������5����
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
    //����
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
	
	iQueryCounts = 0;  //��ѯ����
	iQueryFailCounts = 0;  //��ѯ����
	
	iInsertCounts = 0; //�������
	iInsertFailCounts = 0; //�������
	
	iUpdateCounts = 0; //���´���
	iUpdateFailCounts = 0; //���´���
	
	iDeleteCounts = 0; //ɾ������
	iDeleteFailCounts = 0; //ɾ������
	
	
	memset(sProcessName,0,sizeof(sProcessName));

	iRBAddrOffset = 0;
}	 
//�Ƿ��ǵ�ǰ�̵߳�����
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
		}
		printf("[%d]--",i);
		itor->Show();
		i++;
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
	TADD_NORMAL("TMdbLocalLink::AddNewRBRowUnit\n");
	TShmList<TRBRowUnit>::iterator itorNew =  m_RBList.insert(m_RBList.end());
	if(itorNew != m_RBList.end())
	{//����ɹ�
		TRBRowUnit* pNewRBRowUnit = &(*itorNew);
		pNewRBRowUnit->pTable = pRBRowUnit->pTable;
		pNewRBRowUnit->SQLType = pRBRowUnit->SQLType;
		pNewRBRowUnit->iRealRowID = pRBRowUnit->iRealRowID;
		pNewRBRowUnit->iVirtualRowID = pRBRowUnit->iVirtualRowID;
	}
	else
	{//����ʧ��
		CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for RBRowUnit");
	}
	return iRet;
}


void TMdbLocalLink::Commit(TMdbShmDSN * pShmDSN)
{	
	iAffect = 0;
	if(m_RBList.empty()) return;
	
	TADD_NORMAL("TMdbLocalLink::Commit\n");
		
	//�������
	TShmList<TRBRowUnit>::iterator itorB = m_RBList.begin();
	while(!m_RBList.empty())
	{		
		if(itorB->Commit(pShmDSN)!=0)
		{
			m_RBList.clear();
			break; 
		}
		iAffect++;
		itorB= m_RBList.erase(itorB);
	}
	
}

void  TMdbLocalLink::RollBack(TMdbShmDSN * pShmDSN)
{
	iAffect = 0;
	if(m_RBList.empty()) return;
	
	TADD_NORMAL("TMdbLocalLink::RollBack\n");
	
	
	//�������
	TShmList<TRBRowUnit>::iterator itor = m_RBList.end();
	while(!m_RBList.empty())
	{
		--itor; 
		if(itor->RollBack(pShmDSN)!=0)
		{
			m_RBList.clear();
			break; 
		}
		
		iAffect++;
		itor = m_RBList.erase(itor);
	}
}

int TRBRowUnit::UnLockRow(char* pDataAddr,TMdbShmDSN * pShmDSN)
{
	int iRet = 0;
	CHECK_OBJ(pDataAddr);
	TMdbPageNode* pPageNode = (TMdbPageNode* )pDataAddr -1;

	CHECK_RET(pPageNode->tMutex.UnLock(true),"PageNode UnLock Failed.");
	printf("Table %s,UnLock Row %d\n",pTable->sTableName,iRealRowID);
		
	return iRet;
}


void TRBRowUnit::Show()
{
	printf("[TableName:%s][SQLType:%s][iRealRowID:%u][iVirtualRowID:%u]\n",
		pTable->sTableName,GetSQLName(),iRealRowID,iVirtualRowID);
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

int TRBRowUnit::Commit(TMdbShmDSN * pShmDSN)
{
	int iRet = 0;
	switch(SQLType)
	{
		case TK_INSERT:
			CHECK_RET(CommitInsert(pShmDSN),"CommitInsert failed");
			break;
		case TK_UPDATE:
			CHECK_RET(CommitUpdate(pShmDSN),"CommitUpdate failed");
			break;
		case TK_DELETE:
			CHECK_RET(CommitDelete(pShmDSN),"CommitDelete failed");
			break;
		default:
			break;
	}	
	return iRet;
}
int TRBRowUnit::CommitInsert(TMdbShmDSN * pShmDSN)
{
	int iRet = 0;
	TMdbTableWalker tWalker;
	tWalker.AttachTable(pShmDSN,pTable);
	TMdbRowID rowID;
	rowID.SetRowId(iVirtualRowID);
	int iDataSize = 0;
	char* pDataAddr = tWalker.GetAddressRowID(&rowID,iDataSize,true);
	CHECK_OBJ(pDataAddr);
	TMdbPageNode* pPageNode = (TMdbPageNode* )pDataAddr -1;
	pPageNode->iSessionID = 0;
	pPageNode->cFlag = DATA_REAL;	
	pTable->tTableMutex.Lock(pTable->bWriteLock, &(pShmDSN->GetInfo()->tCurTime));
	++pTable->iCounts; //����һ����¼
	pTable->tTableMutex.UnLock(pTable->bWriteLock);


	//����ͬ������
	TMdbFlushTrans tMdbFlushTrans;
	tMdbFlushTrans.Init(pShmDSN,pTable,TK_INSERT,pDataAddr);
	long long iTimeStamp = TMdbDateTime::StringToTime(pShmDSN->GetInfo()->sCurTime);
	CHECK_RET(tMdbFlushTrans.MakeBuf(((TMdbPage *)tWalker.m_pCurPage)->m_iPageLSN,iTimeStamp),"Make ReoBuf failed.");
	CHECK_RET(tMdbFlushTrans.InsertBufIntoQueue(),"InsertBufIntoQueue failed.");
	CHECK_RET(tMdbFlushTrans.InsertBufIntoCapture(),"InsertBufIntoCapture failed.");
	return iRet;
}

int TRBRowUnit::CommitDelete(TMdbShmDSN * pShmDSN)
{
	int iRet=0;
	TMdbTableWalker tWalker;
	tWalker.AttachTable(pShmDSN,pTable);
	TMdbRowID rowID;
	rowID.SetRowId(iRealRowID);
	int iDataSize = 0;
	char* pDataAddr = tWalker.GetAddressRowID(&rowID,iDataSize,true);
	CHECK_OBJ(pDataAddr);
	TMdbPageNode* pPageNode = (TMdbPageNode* )pDataAddr -1;
	if(pPageNode->cFlag & DATA_RECYCLE)
	{
		CHECK_RET(-1,"CommitDelete, But data is deleted by other link.");
	}
	
	char* pPage = tWalker.GetPageAddr();	
	CHECK_OBJ(pPage);

	//��ɾ��֮ǰ��Ҫ�ɼ�ͬ������
	TMdbFlushTrans tMdbFlushTrans;
	tMdbFlushTrans.Init(pShmDSN,pTable,TK_DELETE,pDataAddr);
	long long iTimeStamp = TMdbDateTime::StringToTime(pShmDSN->GetInfo()->sCurTime);
	CHECK_RET(tMdbFlushTrans.MakeBuf(((TMdbPage *)tWalker.m_pCurPage)->m_iPageLSN,iTimeStamp),"Make ReoBuf failed.");
	
	TMdbExecuteEngine tEngine;
	//ɾ������->ɾ��varchar->ɾ���ڴ�
	CHECK_RET(tEngine.ExecuteDelete(pPage,pDataAddr,rowID,pShmDSN,pTable),"ExecuteDelete failed.");

	//���յ������ݽڵ���ϱ�־
	pPageNode->cFlag |= DATA_RECYCLE;
	
	//д�����
	CHECK_RET(tMdbFlushTrans.InsertBufIntoQueue(),"InsertBufIntoQueue failed.");
	CHECK_RET(tMdbFlushTrans.InsertBufIntoCapture(),"InsertBufIntoCapture failed.");

	CHECK_RET(UnLockRow(pDataAddr,pShmDSN),"UnLockRow Failed.");
	return iRet;
}	

int TRBRowUnit::CommitUpdate(TMdbShmDSN * pShmDSN)
{	
	int iRet=0;
	
	TMdbTableWalker tWalker;
	tWalker.AttachTable(pShmDSN,pTable);
	
	TMdbRowID VirtualRowID,RealRowID;	
	int iDataSize = 0;
	char* pDataAddr = NULL;
	TMdbPageNode* pPageNode = NULL;

	VirtualRowID.SetRowId(iVirtualRowID);
	RealRowID.SetRowId(iRealRowID);

	//��Ч���������
	pDataAddr = tWalker.GetAddressRowID(&VirtualRowID,iDataSize,true);
	CHECK_OBJ(pDataAddr);
	pPageNode = (TMdbPageNode* )pDataAddr -1;
	pPageNode->iSessionID = 0;
	pPageNode->cFlag = DATA_REAL;
	pTable->tTableMutex.Lock(pTable->bWriteLock, &(pShmDSN->GetInfo()->tCurTime));
	++pTable->iCounts; //����һ����¼
	pTable->tTableMutex.UnLock(pTable->bWriteLock);

	//����ͬ������
	TMdbFlushTrans tMdbFlushTrans;
	tMdbFlushTrans.Init(pShmDSN,pTable,TK_UPDATE,pDataAddr);
	long long iTimeStamp = TMdbDateTime::StringToTime(pShmDSN->GetInfo()->sCurTime);
	CHECK_RET(tMdbFlushTrans.MakeBuf(((TMdbPage *)tWalker.m_pCurPage)->m_iPageLSN,iTimeStamp),"Make ReoBuf failed.");
	

	//ɾ��֮ǰ������
	pDataAddr = NULL;
	pDataAddr = tWalker.GetAddressRowID(&RealRowID,iDataSize,true);
	CHECK_OBJ(pDataAddr);
	pPageNode = (TMdbPageNode* )pDataAddr -1;
	if(pPageNode->cFlag & DATA_RECYCLE)
	{
		CHECK_RET(-1,"CommitUpdate, But data is deleted by other link.");
	}
		
	char* pPage = tWalker.GetPageAddr();	
	CHECK_OBJ(pPage);		
	TMdbExecuteEngine tEngine;
	//ɾ������->ɾ��varchar->ɾ���ڴ�
	CHECK_RET(tEngine.ExecuteDelete(pPage,pDataAddr,RealRowID,pShmDSN,pTable),"ExecuteDelete failed.");

	//���յ������ݽڵ���ϱ�־
	pPageNode->cFlag |= DATA_RECYCLE;
	
	//��ͬ�����ݲ������
	CHECK_RET(tMdbFlushTrans.InsertBufIntoQueue(),"InsertBufIntoQueue failed.");
	CHECK_RET(tMdbFlushTrans.InsertBufIntoCapture(),"InsertBufIntoCapture failed.");
	
	CHECK_RET(UnLockRow(pDataAddr,pShmDSN),"UnLockRow Failed.");
	return iRet;
	
}


int TRBRowUnit::RollBack(TMdbShmDSN * pShmDSN)
{
	int iRet = 0;
	switch(SQLType)
	{
		case TK_INSERT:
			CHECK_RET(RollBackInsert(pShmDSN),"RollBackInsert failed");
			break;
		case TK_UPDATE:
			CHECK_RET(RollBackUpdate(pShmDSN),"RollBackUpdate failed");
			break;
		case TK_DELETE:
			CHECK_RET(RollBackDelete( pShmDSN),"RollBackDelete failed");
			break;
		default:
			break;
	}	
	return iRet;
}
int TRBRowUnit::RollBackInsert(TMdbShmDSN * pShmDSN)
{
	int iRet=0;
	
	TMdbTableWalker tWalker;
	tWalker.AttachTable(pShmDSN,pTable);
	TMdbRowID rowID;
	rowID.SetRowId(iVirtualRowID);
	int iDataSize = 0;
	char* pDataAddr = tWalker.GetAddressRowID(&rowID,iDataSize,true);
	CHECK_OBJ(pDataAddr);
	char* pPage = tWalker.GetPageAddr();	
	CHECK_OBJ(pPage);
	
	TMdbExecuteEngine tEngine;
	//ɾ������->ɾ��varchar->ɾ���ڴ�
	CHECK_RET(tEngine.ExecuteDelete(pPage,pDataAddr,rowID,pShmDSN,pTable),"ExecuteDelete failed.");

	return iRet;
}

int TRBRowUnit::RollBackDelete(TMdbShmDSN * pShmDSN)
{
	int iRet = 0;
	
	TMdbTableWalker tWalker;
	tWalker.AttachTable(pShmDSN,pTable);
	TMdbRowID rowID;
	rowID.SetRowId(iRealRowID);
	int iDataSize = 0;
	char* pDataAddr = tWalker.GetAddressRowID(&rowID,iDataSize,true);
	CHECK_OBJ(pDataAddr);
	TMdbPageNode* pPageNode = (TMdbPageNode* )pDataAddr -1;
	pPageNode->cFlag&=~DATA_DELETE;
	
	CHECK_RET(UnLockRow(pDataAddr,pShmDSN),"UnLockRow Failed.");
	return iRet;
}


int TRBRowUnit::RollBackUpdate(TMdbShmDSN * pShmDSN)
{
	int iRet=0;
	
	TMdbTableWalker tWalker;
	tWalker.AttachTable(pShmDSN,pTable);
	
	TMdbRowID VirtualRowID,RealRowID;	
	int iDataSize = 0;
	char* pDataAddr = NULL;
	TMdbPageNode* pPageNode = NULL;

	VirtualRowID.SetRowId(iVirtualRowID);
	RealRowID.SetRowId(iRealRowID);

	//RollBackDelete
	pDataAddr = tWalker.GetAddressRowID(&RealRowID,iDataSize,true);
	CHECK_OBJ(pDataAddr);
	pPageNode = (TMdbPageNode* )pDataAddr -1;
	pPageNode->cFlag&=~DATA_DELETE;	
	CHECK_RET(UnLockRow(pDataAddr,pShmDSN),"UnLockRow Failed.");

	//RollBackInsert
	pDataAddr = NULL;
	pDataAddr = tWalker.GetAddressRowID(&VirtualRowID,iDataSize,true);
	CHECK_OBJ(pDataAddr);		
	char* pPage = tWalker.GetPageAddr();
	CHECK_OBJ(pPage);
	
	TMdbExecuteEngine tEngine;
	//ɾ������->ɾ��varchar->ɾ���ڴ�
	CHECK_RET(tEngine.ExecuteDelete(pPage,pDataAddr,VirtualRowID,pShmDSN,pTable),"ExecuteDelete failed.");
	

	return iRet;
}


//}
