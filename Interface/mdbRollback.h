/****************************************************************************************
*@Copyrights  2012�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    mdbRollback.h
*@Description�� �������QuickMDB�Ļع����ƽӿ�
*@Author:		li.shugang
*@Date��	    2012��02��6��
*@History:
******************************************************************************************/
#ifndef __QUICK_MEMORY_DATABASE_ROLLBACK_H__
#define __QUICK_MEMORY_DATABASE_ROLLBACK_H__

#include <map>
#include <vector>
//#include "BillingSDK.h"
#include "Control/mdbVarcharMgr.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{
    
	#define MAX_ROLLBACK_COUNTS 10000

	class TMdbQuery;
	class TMdbDatabase;
	class TMdbSqlParser;
	class TMdbColumn;
	class TMdbVarcharMgr;
	class TMdbRowCtrl;
	
	//�ع���Ԫ
	class TRBUnit
	{
	public:
		TRBUnit();
		~TRBUnit();
	public:
		TMdbQuery* pQuery;
		char * sSQL;
	};

	//�ع���
	class TMdbRollback
	{
	public:
		TMdbRollback();
		~TMdbRollback();
		void SetDB(TMdbDatabase* pDB,char* sDsn);
		/******************************************************************************
		* ��������	:  CreateQuery()
		* ��������	:  ����SQL��ȡ��Ӧ��Query
		* ����		:  pMdbTable, ��ṹָ��
		* ����		:  pszSQL, ԭʼSQL
		* ���		:  ��
		* ����ֵ	:  �ɹ�����Query��λ��, ʧ�ܷ���-1
		* ����		:  li.shugang
		*******************************************************************************/
		int CreateQuery(const char * sSql,TMdbSqlParser * pMdbSqlParser);
		/******************************************************************************
		* ��������	:  PushData()
		* ��������	:  ����Pos��ȡ��Ӧ��SQL����
		* ����		:  iPos, Query��λ�ã�0---999
		* ����		:  pData, ʵ�ʼ�¼�ĵ�ַ
		* ����		:  iCounts, �Ѿ�ִ�еļ�¼��
		* ���		:  ��
		* ����ֵ	:  �ɹ�����Query�ľ��, ʧ�ܷ���NULL
		* ����		:  li.shugang
		*******************************************************************************/
		int PushData(int iPos, const char* pData, int iCounts,const char * pExtraDataAddr,TMdbRowCtrl * pRowCtrl);
		/******************************************************************************
		* ��������	:  Commit()
		* ��������	:  �ύ���е�SQL����
		* ����		:  ��
		* ���		:  ��
		* ����ֵ	:  �ɹ�����0, ʧ�ܷ��ظ���
		* ����		:  li.shugang
		*******************************************************************************/
		int Commit();
		int CommitEx();
	#if 0
		/******************************************************************************
		* ��������	:  Rollback()
		* ��������	:  �ع����е�SQL����
		* ����		:  ��
		* ���		:  ��
		* ����ֵ	:  �ɹ�����0, ʧ�ܷ��ظ���
		* ����		:  li.shugang
		*******************************************************************************/
		int Rollback(bool bOneSql);
	#endif
		int RowsAffected();
		int SetCloseRBUnit(int iPos);
		int RollbackAll();//�ع�����
		int RollbackOneArrayExecute();//�ع�һ������ִ��
		int RollbackOneExecute();//�ع�һ��ִ��
		bool bIsRollbackEmpty(){return m_iOffSet < 0 || m_pszRollback == NULL || m_pBuffer == NULL;}//�ع����Ƿ�Ϊ��
		int SetArraryExecuteStart();//��������ִ�п�ʼ
	private:
		int GetFreePos();//��ȡһ�����еĻع���Ԫ
		//���ڴ��л�ȡ���ݣ���д��ع�buffer
    	int GetDataFromAddr(TMdbColumn * pColumn,const char* pData,const char * pExtraDataAddr,TMdbRowCtrl * pRowCtrl, TMdbSqlParser * pMdbSqlParser);
		int ReAlloc();//���ݵ�ǰ�ع��Σ����·���ع��ε��ڴ棬���ʧ�ܣ�����-1
		int PushDataIntoRB();//��buffer����д��ع�����
		int RollbackOneRecord();//ֻ�ع����һ��
	private:
		TMdbDatabase* m_pDB;
		TRBUnit* m_ptRBUnit[MAX_ROLLBACK_COUNTS];   //һ�����Ļع�SQL��Ϣ
		char  m_sSQL[4096];                         //��ʱSQL
		char* m_pBuffer;                            //�����ʱ��һ���ع�����
		char* m_pszRollback;                        //�ع��ε�ָ��
		int   m_iSize;                              //�ع��εĴ�С����λΪM
		int   m_iOffSet;                            //�ع����ݵ�ƫ��������һ��ջ�����Ƕ���
		int   m_iRowsAffected;                      //�ύ���ع�Ӱ��ļ�¼��
		std::vector<int>  m_vCloseRBUnit;			//�ѹرյ�QUERY����Ӧ��RBUnit
		TMdbVarCharCtrl m_pVarcharCtrl;
		//�䳤�洢������
		//TMdbVarcharMgr * m_pVarcharMgr;//varchar����
		TMdbNtcSplit * m_pRBValueSplit;//�ع����ݷָ���
		//TMdbSplit * m_pRBValueSplit;//�ع����ݷָ���
	};
//}


#endif //__QUICK_MEMORY_DATABASE_ROLLBACK_H__

