/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbJobCtrl .h		
*@Description： mdb的job管理
*@Author:			jin.shaohua
*@Date：	    2013.5
*@History:
******************************************************************************************/
#ifndef _QUICKMDB_JOB_CTRL_H_
#define _QUICKMDB_JOB_CTRL_H_
#include "Control/mdbMgrShm.h"
#include "Control/mdbProcCtrl.h"
#include "Interface/mdbQuery.h"
#include "Helper/mdbThreadBase.h"


int CalcNextExecDate(TMdbJob * pJob);//计算下一次执行的时间
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
		    int DoJob(TMdbJob * pJob);//处理job
		private:
		    TMdbQuery  *m_pQuery;  
		    TMdbDatabase m_tDB;
			
			TMdbJob *m_pJob;
			const char* m_sDsn;
	};

    //mdb job管理 
    class TMdbJobCtrl
    {
    public:
        TMdbJobCtrl();
        ~TMdbJobCtrl();
        int Init(const char * sDsn);
        int LoadJobs(const char * sDsn);//加载job
        int StartJob();//启动jobs
        int AddNewJob(TMdbJob * pNewJob);//添加新job
        int DeleteJobByName(const char * sJobName);//根据job name 删除job
		int SetCancel(const char* pJobName);
    private:
        int ConnectMDB(const char * sDsn);//链接数据库
		TMdbJobThread* GetJobThread();
    private:
        TMdbShmDSN * m_pShmDsn;
        TMdbProcCtrl m_tProcCtrl;
		TMdbJobThread* m_pJobThreadPool[MAX_THREAD_COUNTS];
    };
//}

#endif

