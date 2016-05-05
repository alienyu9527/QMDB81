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

//namespace QuickMDB{
    class TMdbDatabase;
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
    private:
        int CalcNextExecDate(TMdbJob * pJob);//计算下一次执行的时间
		int AddOneTimeInterval(TMdbJob * pJob);
        int DoJob(TMdbJob * pJob);//处理job
        int ConnectMDB(const char * sDsn);//链接数据库
        int DealDeleteTask(const char*pDelSQL);//处理删除任务
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

