/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbQueue.h
*@Description�� ��oralceͬ����������ͬ�������з�װ���ṩͳһ�����ӿ�
*@Author:		cao.peng
*@Date��	    2012.11
*@History:
******************************************************************************************/
#ifndef _MDB__QUEUE_H_
#define _MDB__QUEUE_H_

#include "Helper/TThreadLog.h" 
#include "Control/mdbMgrShm.h"
#include "Control/mdbProcCtrl.h"

//namespace QuickMDB{

#define ERROR_TIMES 10
#define  T_SUCCESS                0    
#define  T_BIG_PushPos         1    
#define  T_BIG_TailPos           2   
#define  T_BIG_ENDERROR      3
#define  T_BIG_BEGINERROR   4
#define  T_EMPTY                     5
#define  T_LENGTH_ERROR       6
#define  T_SQLTYPE_ERROR      7
#define  T_SOURCEID_ERROR    8

#define T_UPDATE                          2
#define T_DELETE                          3
#define T_INSERT                          4


class TMdbQueue
{
public:
	TMdbQueue();
	~TMdbQueue();
	int Init(TMdbMemQueue * pMemQueue,TMdbDSN * pDsn,const bool bWriteErrorData=false);
	
	/******************************************************************************
	* ��������	:  Push
	* ��������	:  ����DSN����
	* ����		:  sData push�����ݣ�iLen push���ݵĳ���
	* ���		:  ��
	* ����ֵ	:  !0 - ʧ��,0 - �ɹ�
	* ����		:  cao.peng
	*******************************************************************************/
	bool Push(char * const sData,const int iLen);

	/******************************************************************************
	* ��������	:  Pop
	* ��������	:  ��ȡ������һ������
	* ����		:  ��
	* ���		:  sData �洢pop�������ݣ�iBufLen �洢pop���ݵĳ��ȣ�iLen pop�����ݵĳ���
	* ����ֵ	:  !0 - ʧ��,0 - �ɹ�
	* ����		:  cao.peng
	*******************************************************************************/	
	int Pop();

    int PopRepData();

	/******************************************************************************
	* ��������	:  GetSourceId
	* ��������	:  ��ȡpop�����ݵ�������Դ
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ����Դ
	* ����		:  cao.peng
	*******************************************************************************/
	//int GetSourceId();

	/******************************************************************************
	* ��������	:  GetSqlType
	* ��������	:  ��ȡsql����
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  sql����
	* ����		:  cao.peng
	*******************************************************************************/
	int GetSqlType();

	int GetRecordLen();
	char* GetData();

	void SetParameter();
       void SetCheckDataFlag(bool bCheck){m_bCheckData = bCheck;}//�Ƿ�У������
private:
	/******************************************************************************
	* ��������	:  CheckDataIsValid
	* ��������	:  �ж϶����е�ĳ�������Ƿ���Ч
	* ����		:  pCurAddr ��⹲���ڴ��ַ
	* ���		:  iLen ���ݳ���
	* ����ֵ	:  true ������Ч��false ������Ч
	* ����		:  cao.peng
	*******************************************************************************/
	int CheckDataIsValid();

    int CheckRepDataIsValid();
		
	/******************************************************************************
	* ��������	:  CheckRecord
	* ��������	:  �ж�pop���������Ƿ�Ϸ�����Ҫ������ݵĳ����Լ��Ƿ���##����
	* ����		:  pszRecord ����¼,iLen ��¼����
	* ���		:  ��
	* ����ֵ	:  true ��¼�Ϸ���false ��¼��Ч
	* ����		:  cao.peng
	*******************************************************************************/
	bool CheckRecord(char *pszRecord,const int iLen);

	/******************************************************************************
	* ��������	:  PrintInvalidData
	* ��������	:  ��ӡ�����¼��Ϣ
	* ����		:  pCurAddr ���ݵ�ַ��iLen��Ч���ݳ���
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  cao.peng
	*******************************************************************************/
	void PrintInvalidData(char *pCurAddr,const int iLen);
	int GetPosOfNext();//��ȡ��һ����¼λ��
	bool WriteInvalidData(const int iInvalidLen);//��¼��Ч��¼
private:
	TMdbDSN      * m_pDsn;
	TMdbMemQueue * m_pQueueShm;    //���ݴ洢��ַ 
	char* m_pszRecord;
	char * m_pCurAddr;
	int m_iSQLType;                //sql����
	int m_iSyncType;               //������Դ
	int m_iRecordLen;            //���ݳ���
	int m_iErrTry;                 //pop����ʱ�����������

	int m_iPushPos;
	int m_iPopPos;
	int m_iTailPos;
	bool m_bCheckData;//�Ƿ�У������
	char m_sFileName[MAX_PATH_NAME_LEN];
	char *m_pszErrorRecord;
	FILE* m_pFile;
	bool m_bWriteErrorData;
};

class TMdbOnlineRepQueue
{
public:
	TMdbOnlineRepQueue();
	~TMdbOnlineRepQueue();
	int Init(TMdbOnlineRepMemQueue * pOnlineRepMemQueue,TMdbDSN * pDsn,const bool bWriteErrorData=false);
	
	/******************************************************************************
	* ��������	:  Push
	* ��������	:  ����DSN����
	* ����		:  sData push�����ݣ�iLen push���ݵĳ���
	* ���		:  ��
	* ����ֵ	:  !0 - ʧ��,0 - �ɹ�
	* ����		:  cao.peng
	*******************************************************************************/
	bool Push(char * const sData,const int iLen);

	/******************************************************************************
	* ��������	:  Pop
	* ��������	:  ��ȡ������һ������
	* ����		:  ��
	* ���		:  sData �洢pop�������ݣ�iBufLen �洢pop���ݵĳ��ȣ�iLen pop�����ݵĳ���
	* ����ֵ	:  !0 - ʧ��,0 - �ɹ�
	* ����		:  cao.peng
	*******************************************************************************/	
	int Pop();

	int RollbackPopPos();

	int GetPosOfNext();

	int GetRecordLen();
	char* GetData();
	int GetUsedPercentage();
	int CheckRepDataIsValid();
	bool WriteInvalidData(const int iInvalidLen);
private:
	

	/******************************************************************************
	* ��������	:  PrintInvalidData
	* ��������	:  ��ӡ�����¼��Ϣ
	* ����		:  pCurAddr ���ݵ�ַ��iLen��Ч���ݳ���
	* ���		:  ��
	* ����ֵ	:  ��
	* ����		:  cao.peng
	*******************************************************************************/
	void PrintInvalidData(char *pCurAddr,const int iLen);
private:
	TMdbDSN      * m_pDsn;
	TMdbOnlineRepMemQueue * m_pOnlineRepQueueShm;    //��·ͬ�����ݴ洢��ַ 
	char* m_pszRecord;
	char * m_pCurAddr;
	int m_iSQLType;                //sql����
	int m_iSyncType;               //������Դ
	int m_iRecordLen;            //���ݳ���
	int m_iErrTry;                 //pop����ʱ�����������

	int m_iPushPos;
	int m_iPopPos;
	int m_iCleanPos;
	int m_iStartPos;
	int m_iTailPos;
	bool m_bCheckData;//�Ƿ�У������
	char m_sFileName[MAX_PATH_NAME_LEN];
	char *m_pszErrorRecord;
	FILE* m_pFile;
	bool m_bWriteErrorData;
};

//}
#endif
