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


//������ֻ��Ҫ��¼"��Ҫ��̬���������"����Ϣ
struct ST_LINK_INDEX_INFO
{
    bool bInit;//�Ƿ񱻳�ʼ��
	char sName[MAX_NAME_LEN]; //��������
	int  iAlgoType;  // �������㷨����: hash , multistep hash, btree

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


//������ ��¼���ȫ��������Ϣ
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


//�ع���Ԫ
class TRBRowUnit 
{	
	public:
		TRBRowUnit();
		~TRBRowUnit(){}
		void Show();
		
	public:
	 	char  sTableName[MAX_NAME_LEN];
		char	SQLType;		//Insert or  delete or update
	 	unsigned int 	iRealRowID;		//�������ڴ���ԭʼ���ݼ�¼λ��
	 	unsigned int  	iVirtualRowID;  	//���������������м�¼λ�ã�Ӱ���ڴ棬 commit֮ǰΪ��������

	private:		
		const char* GetSQLName();		
		
};

//�ع�������
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


//���ݿⱾ��������Ϣ
class TMdbLocalLink
{
public:
    void Clear();
    void Print();
    void Show(bool bIfMore=false);
    bool IsValid(){return (iPID > 0 && 0 != sStartTime[0]);};//�Ƿ��ǺϷ���
    bool IsCurrentThreadLink();//�Ƿ��ǵ�ǰ�̵߳�����

	int AddNewRBRowUnit(TRBRowUnit* pRBRowUnit);
	void ShowRBUnits();
	void ShowIndexInfo();	
	int ReturnAllIndexNodeToTable(TMdbShmDSN * pShmDSN);
	
	TMdbSingleTableIndexInfo*  FindCurTableIndex(const char* sTableName);
	
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
    unsigned int iExecuteID; //sqlִ��id
	
    TShmList<TRBRowUnit>  m_RBList; //�ع�����
    int iAffect;
	size_t  iRBAddrOffset;

	//��Ҫ������ű��������Ϣ
	TMdbSingleTableIndexInfo m_AllTableIndexInfo[MAX_TABLE_COUNTS];
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
	int RegRemoteLink(TMdbCSLink &tCSLink,int iAgentPort =-1,bool bMasterPort=false);//ע��Զ������
	int UnRegRemoteLink(TMdbCSLink &tCSLink,int iAgentPort =-1);//ע��Զ������
	int RegRepLink(TMdbRepLink &tRepLink);//ע��rep����
	int UnRegRepLink(int iSocket,char cState);//ע��rep����
	int ClearInvalidLink();//�����Ч����
	int ClearRemoteLink(int iPort = 0);//����Զ������
	int ClearAllLink();//�����������
	int   GetCsAgentPort(int iClientPort);//���ݿͻ��˷������Ķ˿ںţ���������˿ں��жϣ�ȡһ�����ʵ�
	int   GetCsAgentPortBaseOnMasterPort(int iClientPort);//���ݿͻ��˷������Ķ˿ںţ���������˿ں��жϣ�ȡһ�����ʵ�
	int	  AddConNumForPort(int iAgentPort);//���ӳɹ�������������
	int	  MinusConNumForPort(int iAgentPort);//���ӶϿ�����������1
	int   ClearCntNumForPort(int iAgentPort);//agent��ʼ��ʱ����������0
private:
	TMdbShmDSN * m_pShmDsn;//������
	TMdbDSN       *  m_pDsn;//dsn��Ϣ	
	TShmAlloc m_tMgrShmAlloc;//�����ڴ������
	char* m_pMgrAddr;
};



#endif
