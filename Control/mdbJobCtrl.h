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

//namespace QuickMDB{
    class TMdbDatabase;
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
    private:
        int CalcNextExecDate(TMdbJob * pJob);//������һ��ִ�е�ʱ��
		int AddOneTimeInterval(TMdbJob * pJob);
        int DoJob(TMdbJob * pJob);//����job
        int ConnectMDB(const char * sDsn);//�������ݿ�
        int DealDeleteTask(const char*pDelSQL);//����ɾ������
        int GenSelctSQLByDelSQL(TMdbTable * pTable,const char*pDelSQL,char *pSSQL,const int iLen);
        int GenDelSQL(TMdbTable * pTable,char *pDelSQL,const int iLen);
    private:
        TMdbShmDSN * m_pShmDsn;
        TMdbProcCtrl m_tProcCtrl;
        TMdbQuery * m_pDelQuery;
        TMdbQuery  *m_pQuery;
    	TMdbDatabase m_tDB;
    };
//}

#endif

