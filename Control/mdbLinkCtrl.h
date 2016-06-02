/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbLinkCtrl.h		
*@Description�� mdb���ӹ���
*@Author:	jin.shaohua
*@Date��	    2012.11
*@History:
******************************************************************************************/
#ifndef _MDB_LINK_CTRL_H_
#define _MDB_LINK_CTRL_H_
#include "Helper/mdbStruct.h"
#include "Helper/mdbDictionary.h"

#include "Control/mdbMgrShm.h"

//namespace QuickMDB{

//class TAgentClient;
//���ӹ���
class TMdbLinkCtrl
{
public:
	TMdbLinkCtrl();
	~TMdbLinkCtrl();
	int Attach(const char * sDsn);//���ӹ����ڴ�
	int RegLocalLink(TMdbLocalLink *& pLocalLink);//ע�᱾������
	int UnRegLocalLink(TMdbLocalLink *& pLocalLink);//ע���������ӹ���
	int RegRemoteLink(TMdbCSLink &tCSLink,int iAgentPort =-1);//ע��Զ������
	int UnRegRemoteLink(TMdbCSLink &tCSLink,int iAgentPort =-1);//ע��Զ������
	int RegRepLink(TMdbRepLink &tRepLink);//ע��rep����
	int UnRegRepLink(int iSocket,char cState);//ע��rep����
	int ClearInvalidLink();//�����Ч����
	int ClearRemoteLink();//����Զ������
	int ClearAllLink();//�����������
	int   GetCsAgentPort(int iClientPort);//���ݿͻ��˷������Ķ˿ںţ���������˿ں��жϣ�ȡһ�����ʵ�
	int	  AddConNumForPort(int iAgentPort);//���ӳɹ�������������
	int	  MinusConNumForPort(int iAgentPort);//���ӶϿ�����������1
	int   ClearCntNumForPort(int iAgentPort);//agent��ʼ��ʱ����������0
private:
	TMdbShmDSN * m_pShmDsn;//������
	TMdbDSN       *  m_pDsn;//dsn��Ϣ	
	TShmAlloc m_tMgrShmAlloc;//�����ڴ������
	char* m_pMgrAddr;
};


//}
#endif
