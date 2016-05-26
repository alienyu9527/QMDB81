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
	int RegRemoteLink(TMdbCSLink &tCSLink);//ע��Զ������
	int UnRegRemoteLink(TMdbCSLink &tCSLink);//ע��Զ������
	int RegRepLink(TMdbRepLink &tRepLink);//ע��rep����
	int UnRegRepLink(int iSocket,char cState);//ע��rep����
	int ClearInvalidLink();//�����Ч����
	int ClearRemoteLink();//����Զ������
	int ClearAllLink();//�����������
private:
	TMdbShmDSN * m_pShmDsn;//������
	TMdbDSN       *  m_pDsn;//dsn��Ϣ	
	TShmAlloc m_tMgrShmAlloc;//�����ڴ������
	char* m_pMgrAddr;
};


//}
#endif
