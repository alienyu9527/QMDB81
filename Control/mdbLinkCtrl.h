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
#include "Control/mdbTableWalker.h"
#include "Control/mdbExecuteEngine.h"


//�ع���Ԫ
class TRBRowUnit 
{	
	public:
		TRBRowUnit(){}
		~TRBRowUnit(){}
		void Show();
		int Commit(TMdbShmDSN * pShmDSN);
		int RollBack(TMdbShmDSN * pShmDSN);
		
	public:
	 	TMdbTable*  pTable;
		char	SQLType;		//Insert or  delete or update
	 	unsigned int 	iRealRowID;		//�������ڴ���ԭʼ���ݼ�¼λ��
	 	unsigned int  	iVirtualRowID;  	//���������������м�¼λ�ã�commit֮ǰΪ��������

	private:		
		const char* GetSQLName();		
		int CommitInsert(TMdbShmDSN * pShmDSN);
		int CommitUpdate(TMdbShmDSN * pShmDSN);
		int CommitDelete(TMdbShmDSN * pShmDSN);
		int RollBackInsert(TMdbShmDSN * pShmDSN);
		int RollBackUpdate(TMdbShmDSN * pShmDSN);
		int RollBackDelete(TMdbShmDSN * pShmDSN);
		int UnLockRow(char* pDataAddr, TMdbShmDSN * pShmDSN);
};


//���ݿⱾ��������Ϣ
class TMdbLocalLink
{
public:
    void Clear();
    void Print();
    void Show(bool bIfMore=false);
    bool IsValid(){return (iPID > 0 && 0 != sStartTime[0]);};//�Ƿ��ǺϷ���
    bool IsCurrentThreadLink();//�Ƿ��ǵ�ǰ�̵߳�����
    void Commit(TMdbShmDSN * pShmDSN);
	void RollBack(TMdbShmDSN * pShmDSN);
	int AddNewRBRowUnit(TRBRowUnit* pRBRowUnit);
	void ShowRBUnits();
public:
    int iPID;         //���̵�PID
    unsigned long int iTID;         //���̵�Thread-ID
    int iSocketID;    //����ID
    char cState;      //����״̬
    char sStartTime[MAX_TIME_LEN]; //��������ʱ��
    char sFreshTime[MAX_TIME_LEN]; //�����������ʱ�䣬�൱������
    int  iLogLevel;                //���ӵ���־����
    char cAccess;                  //Ȩ��, A-����Ա;W-�ɶ�д;R-ֻ��
    int  iSQLPos;     //��ǰִ�е�SQLλ��

    int  iQueryCounts;  //��ѯ����
    int  iQueryFailCounts;  //��ѯ����

    int  iInsertCounts; //�������
    int  iInsertFailCounts; //�������

    int  iUpdateCounts; //���´���
    int  iUpdateFailCounts; //���´���

    int  iDeleteCounts; //ɾ������
    int  iDeleteFailCounts; //ɾ������

    char sProcessName[MAX_NAME_LEN];   //�ĸ����̴�������
	
    unsigned int iSessionID; //����id
    TShmList<TRBRowUnit>  m_RBList; //�ع�����
	size_t  iRBAddrOffset;
};

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
