/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbJobCtrl .cpp		
*@Description�� job����
*@Author:			jin.shaohua
*@Date��	    2013.5
*@History:
******************************************************************************************/
#include "Control/mdbJobCtrl.h"
#include "Helper/mdbConfig.h"
#include "Helper/mdbSQLParser.h"
#include "Helper/mdbDateTime.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

#define _TRY_CATCH_BEGIN_ try{

#define _TRY_CATCH_END_ }\
    catch(TMdbException& e)\
    {\
        TADD_ERROR(ERROR_UNKNOWN, "ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());\
        iRet = ERROR_UNKNOWN;\
    }\
    catch(...)\
    {\
        TADD_ERROR(ERROR_UNKNOWN,"Unknown error!\n");\
        iRet = ERROR_UNKNOWN;\
    }

/******************************************************************************
* ��������	:  CalcNextExecDate
* ��������	:  ������һ��ִ�е�ʱ��
* ����		:  pJob -  �������job
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int CalcNextExecDate(TMdbJob * pJob)
{
	TADD_FUNC("Start.");
	int iRet = 0;
	char sCurTime[MAX_TIME_LEN] = {0};
	TMdbDateTime::GetCurrentTimeStr(sCurTime);
	if(TMdbDateTime::GetDiffSeconds(sCurTime,pJob->m_sExecuteDate) < 0)
	{//δ����ĳ��ʱ����ִ��
		memcpy(pJob->m_sNextExecuteDate,pJob->m_sExecuteDate,MAX_TIME_LEN);
		return 0;
	}
	else if(0 == pJob->m_sNextExecuteDate[0])
	{		
		memcpy(pJob->m_sNextExecuteDate,pJob->m_sExecuteDate,MAX_TIME_LEN);
		//����ʼʱ�俪ʼ���㣬ֱ���ҵ���ǰʱ��֮���ʱ���
		while(TMdbDateTime::GetDiffSeconds(sCurTime,pJob->m_sNextExecuteDate) >= 0)
		{
			AddOneTimeInterval(pJob);
		}
	}
	else
	{
		AddOneTimeInterval(pJob);
	}
	
	TADD_FUNC("Finish");
	return iRet;
}


int AddOneTimeInterval(TMdbJob * pJob)
{
	int iRet = 0;
	CHECK_OBJ(pJob);
	switch(pJob->m_iRateType)
	{
		case JOB_PER_YEAR:
			{
				TMdbDateTime::AddYears(pJob->m_sNextExecuteDate,pJob->m_iInterval);
			}
			break;
		case JOB_PER_MONTH:
			{
				TMdbDateTime::AddMonths(pJob->m_sNextExecuteDate,pJob->m_iInterval);
			}
			break;
		case JOB_PER_DAY:
			{
				TMdbDateTime::AddDay(pJob->m_sNextExecuteDate,pJob->m_iInterval);
			}
			break;
		case JOB_PER_HOUR:
			{
				TMdbDateTime::AddHours(pJob->m_sNextExecuteDate,pJob->m_iInterval);
			}
			break;
		case JOB_PER_MIN:
			{
				TMdbDateTime::AddMins(pJob->m_sNextExecuteDate,pJob->m_iInterval);
			}
			break;
		case JOB_PER_SEC:
			{
				TMdbDateTime::AddSeconds(pJob->m_sNextExecuteDate,pJob->m_iInterval);
			}
			break;
		default:
			CHECK_RET(ERR_APP_INVALID_PARAM,"error ratetype[%d]",pJob->m_iRateType);
			break;
	}

	return iRet;
}




/******************************************************************************
* ��������	:  
* ��������	:  
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  dong.chun
*******************************************************************************/
TMdbJobThread::TMdbJobThread()
{
    m_pQuery = NULL;
	m_pJob = NULL;
}

/******************************************************************************
* ��������	:  
* ��������	:  
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  dong.chun
*******************************************************************************/
TMdbJobThread::~TMdbJobThread()
{
    SAFE_DELETE(m_pQuery);
}

/******************************************************************************
* ��������	:  Start
* ��������	:  ��ʼ���������߳�
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  dong.chun
*******************************************************************************/
int TMdbJobThread::Start(const char* sDsn, TMdbJob* pJob)
{
    int iRet = 0;
    CHECK_OBJ(pJob);
    CHECK_OBJ(sDsn);
	m_pJob = pJob;
	m_sDsn = sDsn;
	
    SetThreadInfo(this,1024*1024*20);   
    CHECK_RET(Run(NULL),"Run Thread faild.");
    return iRet;
}

/******************************************************************************
* ��������	:  svc
* ��������	:  ��ʼ���������߳�
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  dong.chun
*******************************************************************************/
int TMdbJobThread::svc()
{
    int iRet = 0;

	//����
	m_tDB.Disconnect();
	TMdbConfig * pConfig = TMdbConfigMgr::GetMdbConfig(m_sDsn);
	CHECK_OBJ(pConfig);
	TMDbUser * pUser = pConfig->GetUser(0);
	CHECK_OBJ(pUser);		
	if(false == m_tDB.Connect(pUser->sUser,pUser->sPwd,m_sDsn))
	{
		CHECK_RET(ERR_DB_LOCAL_CONNECT,"connect mdb by[%s/******@%s] failed.",pUser->sUser,m_sDsn);
	}
	
	SAFE_DELETE(m_pQuery);
    m_pQuery = m_tDB.CreateDBQuery();
    CHECK_OBJ(m_pQuery);

	
    do
    {
        TADD_NORMAL("Start Do Job [%s]",m_pJob->m_sSQL);
        m_pJob->StateToRunning();//�л�״̬��ִ��̬
        CHECK_RET_BREAK(DoJob(m_pJob),"Do Job Faild[%s]",m_pJob->m_sSQL);
        m_pJob->StateToWait();//�л�״̬���ȴ�̬
        TADD_NORMAL("Finish Do Job [%s]",m_pJob->m_sSQL);
    }while(0);

	
    return iRet;
}

/******************************************************************************
* ��������	:  DoJob
* ��������	:  ����job
* ����		:  
* ����		:  
* ���		:  
* ����ֵ	:  0 - �ɹ�!0 -ʧ��
* ����		:  jin.shaohua
*******************************************************************************/
int TMdbJobThread::DoJob(TMdbJob * pJob)
{
    TADD_FUNC("Start.");
    int iRet = 0;
    CHECK_OBJ(pJob);
    _TRY_CATCH_BEGIN_
    CHECK_OBJ(m_pQuery);
    m_pQuery->Close();
    m_pQuery->SetSQL(pJob->m_sSQL);
	pJob->SetStopFlag(0);

	//����queryȡ����
	m_pQuery->SetCancelPoint(&(pJob->m_iStop));
	
    //��ִ��select
    if(TK_SELECT == m_pQuery->GetSQLType())
    {
        TADD_ERROR(-1,"do not execute select sql[%s].",pJob->m_sSQL);
    }
    else
    {
        m_pQuery->Execute();
        m_pQuery->Commit();
    }
    _TRY_CATCH_END_
    if(0 == pJob->m_iExcCount)
    {//�״�ִ��,��¼�״�ִ��ʱ��
        SAFESTRCPY(pJob->m_sStartTime,sizeof(pJob->m_sStartTime),pJob->m_sNextExecuteDate);
    }
    pJob->m_iExcCount ++;
	CalcNextExecDate(pJob);
	
    TADD_FUNC("Finish.");
    return iRet;
}


    /******************************************************************************
    * ��������	:  
    * ��������	:  
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbJobCtrl::TMdbJobCtrl():
    m_pShmDsn(NULL)
    {
		for(int i = 0; i<MAX_THREAD_COUNTS;i++)
	    {
	        m_pJobThreadPool[i] = NULL;
	    }
    }
    /******************************************************************************
    * ��������	:  
    * ��������	:  
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbJobCtrl::~TMdbJobCtrl()
    {
		for(int i = 0; i<MAX_THREAD_COUNTS;i++)
	    {
	        SAFE_DELETE(m_pJobThreadPool[i]);
	    }
    }
    /******************************************************************************
    * ��������	:  Init
    * ��������	:  
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::Init(const char * sDsn)
    {
        int iRet = 0;
        m_pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDsn);
        m_tProcCtrl.Init(sDsn);
        return iRet;
    }
    /******************************************************************************
    * ��������	:  LoadJobs
    * ��������	:  ���´������ļ�����
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::LoadJobs(const char * sDsn)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbShmDSN * pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(pShmDsn);
        TMdbConfig * pConfig = TMdbConfigMgr::GetMdbConfig(pShmDsn->GetInfo()->sName);
        CHECK_OBJ(pConfig);
        //�������ڴ��е�
        TShmList<TMdbJob>::iterator itorMem = pShmDsn->m_JobList.begin();
        for(;itorMem != pShmDsn->m_JobList.end();++itorMem)
        {
            itorMem->Clear();
        }
        //��ȡ�����ļ�����
        std::vector<TMdbJob>::iterator itorCfg = pConfig->m_vMdbJob.begin();
        for(;itorCfg != pConfig->m_vMdbJob.end();++itorCfg)
        {
            TMdbJob * pMemJob = NULL;
            CHECK_RET(pShmDsn->AddNewJob(pMemJob),"AddNewJob faild.");
            CHECK_OBJ(pMemJob);
            memcpy(pMemJob,&(*itorCfg),sizeof(TMdbJob));//��䵽�ڴ���
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  StartJob
    * ��������	:  ����jobs
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::StartJob()
    {
        TADD_FUNC("Start.");
	    int iRet = 0;
	    int iCount = 0;
	    CHECK_OBJ(m_pShmDsn);
		//�״μ���
	    TShmList<TMdbJob>::iterator itorMem = m_pShmDsn->m_JobList.begin();
	    for(;itorMem != m_pShmDsn->m_JobList.end();++itorMem)
	    {
	        TMdbJob * pJob = &(*itorMem);
	        if(pJob->IsValid())
	        {
	            CalcNextExecDate(pJob);
				pJob->StateToWait();
	        }
	    }

		int iSleepCount = 0;
	    //ѭ������
	    while(1)
	    {
	        m_tProcCtrl.UpdateProcHeart(0);//��������
	        if(m_tProcCtrl.IsCurProcStop())
	        {
	            TADD_NORMAL("Get exit msg....");
	            break;
	        }
	        char sCurTime[MAX_TIME_LEN] = {0};
	        TMdbDateTime::GetCurrentTimeStr(sCurTime);
	        //��������job
	        itorMem = m_pShmDsn->m_JobList.begin();
	        for(;itorMem != m_pShmDsn->m_JobList.end();++itorMem)
	        {
	            TMdbJob * pJob = &(*itorMem);
				
				//ֻ��ִ�еȴ�״̬��job
	            if(pJob->IsValid() && pJob->IsWaiting() 
					&& TMdbDateTime::GetDiffSeconds(pJob->m_sNextExecuteDate,sCurTime) < 0)
	            {
	                TMdbJobThread* pMdbJob = GetJobThread();
	                CHECK_OBJ(pMdbJob);
	                if(pMdbJob->Start(m_pShmDsn->GetInfo()->sName, pJob) != 0)
	                {
	                    TADD_ERROR(-1,"Start Thread Faild");
	                }
	            }
	        }
	        m_tProcCtrl.UpdateProcHeart(0);
	        TMdbDateTime::MSleep(1000);//��Ϣ
	        iSleepCount++;
	        //����̳߳������ڹ������̸߳���
	        iCount = 0;
	        for(int i = 0; i<MAX_THREAD_COUNTS;i++)
	        {
	            if(m_pJobThreadPool[i] != NULL)
	            {
	                if(m_pJobThreadPool[i]->GetTID() != -1)
	                {
	                    iCount++;
	                }
	            }
	        }
			if(iSleepCount >= 30)
			{
				iSleepCount = 0;
	    		TADD_NORMAL("Current ThreadCount = [%d]",iCount);
			}
	    }
	    TADD_FUNC("Finish");
	    return iRet;
    }

    /******************************************************************************
    * ��������	:  AddNewJob
    * ��������	:  �����job
    * ����		:  pNewJob -  ������job
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::AddNewJob(TMdbJob * pNewJob)
    {
      TADD_FUNC("Start.");
      int iRet = 0;
      CHECK_OBJ(pNewJob);
      CHECK_OBJ(m_pShmDsn);
      if(pNewJob->IsValid())
      {
            //�ж��Ƿ����
            if(NULL != m_pShmDsn->GetJobByName(pNewJob->m_sName))
            {
                CHECK_RET(ERR_APP_INVALID_PARAM,"job[%s] is exist",pNewJob->m_sName);
            }
            TMdbJob * pNewMemJob = NULL;
            m_pShmDsn->AddNewJob(pNewMemJob);
            CHECK_OBJ(pNewMemJob);
            CalcNextExecDate(pNewJob);
            memcpy(pNewMemJob,pNewJob,sizeof(TMdbJob));
      }
      else
      {//���Ϸ���job
        CHECK_RET(ERR_APP_INVALID_PARAM,"job[%s] is invalid",pNewJob->ToString().c_str());
      }
      TADD_FUNC("Finish.");
      return iRet;
    }

    /******************************************************************************
    * ��������	:  DeleteJobByName
    * ��������	:  ����job name ɾ��job
    * ����		:  sJobName -  ��ɾ��job
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::DeleteJobByName(const char * sJobName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(NULL == sJobName || 0 == sJobName[0]){return 0;}//��Чname
        TMdbJob * pJobToDel = m_pShmDsn->GetJobByName(sJobName);
        if(NULL == pJobToDel)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"Not find job[%s]",sJobName);
        }
        do
        {
            if(pJobToDel->IsCanModify())
            {//���Ա��
                pJobToDel->Clear();
                break;
            }
            TADD_NORMAL("Job[%s] is running...please wait.",sJobName);
            TMdbDateTime::Sleep(1);
        }while(1);
        TADD_FUNC("Finish.");
        return iRet;
    }


	//�ж��������е�job
    int TMdbJobCtrl::SetCancel(const char* pJobName)
    {
		TShmList<TMdbJob>::iterator itorMem = m_pShmDsn->m_JobList.begin();
        for(;itorMem != m_pShmDsn->m_JobList.end();++itorMem)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pJobName,itorMem->m_sName) == 0)
            {
				if(itorMem->GetStat() != JOB_STATE_RUNNING)
				{
					printf("Job %s is not running now.\n",pJobName);
					return -1;
				}
				else
				{
					itorMem->SetStopFlag(1);
					itorMem->SetStat(JOB_STATE_PAUSE);	
					TADD_NORMAL("Set job [%s] to pause.", pJobName);
					return 0;
				}
			}
        }

		printf("Cannot find Job %s\n",pJobName);
		return -1;

	}

	int TMdbJobCtrl::Resume(const char* pJobName)
    {
		TShmList<TMdbJob>::iterator itorMem = m_pShmDsn->m_JobList.begin();
        for(;itorMem != m_pShmDsn->m_JobList.end();++itorMem)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pJobName,itorMem->m_sName) == 0)
            {
            	itorMem->StateToWait();
				TADD_NORMAL("Set job [%s] to wait.", pJobName);
				return 0;
			}
        }

		printf("Cannot find Job %s\n",pJobName);
		return -1;

	}

	TMdbJobThread* TMdbJobCtrl::GetJobThread()
	{
	    int i = 0;
	    for(; i<MAX_THREAD_COUNTS;i++)
	    {
	        if(m_pJobThreadPool[i] != NULL)
	        {
	            if(m_pJobThreadPool[i]->GetTID() == -1)
	            {
	                return m_pJobThreadPool[i];
	            }
	        }
	        else
	        {
	            m_pJobThreadPool[i] = new(std::nothrow) TMdbJobThread();
				if(m_pJobThreadPool[i] ==  NULL)
				{
					TADD_ERROR(ERR_OS_NO_MEMROY,"can't create new m_pJobThreadPool[i]");
					return NULL;
				}
	            return m_pJobThreadPool[i];
	        }
	    }

	    if(i >= MAX_THREAD_COUNTS)
	    {
	        TADD_ERROR(-1,"thread pool = MAX_THREAD_COUNTS");
	    }
	    return NULL;
	}
//}

