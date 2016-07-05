/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbJobCtrl .h		
*@Description�� mdb��job����
*@Author:			jin.shaohua
*@Date��	    2013.5
*@History:
******************************************************************************************/
#ifndef _QUICKMDB_JOB_CTRL_H_
#define _QUICKMDB_JOB_CTRL_H_
#include "Control/mdbMgrShm.h"
#include "Control/mdbProcCtrl.h"
#include "Interface/mdbQuery.h"
#include "Helper/mdbThreadBase.h"


int CalcNextExecDate(TMdbJob * pJob);//������һ��ִ�е�ʱ��
int AddOneTimeInterval(TMdbJob * pJob);


    class TMdbDatabase;

	class TMdbJobThread:public TMdbThreadBase
	{
		public:
		    TMdbJobThread();
		    virtual ~TMdbJobThread();
		    int Start(const char* m_sDsn, TMdbJob* pJob);
		    virtual int svc();
		private:
		    int DoJob(TMdbJob * pJob);//����job
		private:
		    TMdbQuery  *m_pQuery;  
		    TMdbDatabase m_tDB;
			
			TMdbJob *m_pJob;
			const char* m_sDsn;
	};

    //mdb job���� 
    class TMdbJobCtrl
    {
    public:
        TMdbJobCtrl();
        ~TMdbJobCtrl();
        int Init(const char * sDsn);
        int LoadJobs(const char * sDsn);//����job
        int StartJob();//����jobs
        int AddNewJob(TMdbJob * pNewJob);//�����job
        int DeleteJobByName(const char * sJobName);//����job name ɾ��job
		int SetCancel(const char* pJobName);
    private:
        int ConnectMDB(const char * sDsn);//�������ݿ�
		TMdbJobThread* GetJobThread();
    private:
        TMdbShmDSN * m_pShmDsn;
        TMdbProcCtrl m_tProcCtrl;
		TMdbJobThread* m_pJobThreadPool[MAX_THREAD_COUNTS];
    };
//}

#endif

