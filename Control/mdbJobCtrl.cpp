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
        m_pQuery = NULL;
        m_pDelQuery = NULL;
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
        SAFE_DELETE(m_pQuery);
        SAFE_DELETE(m_pDelQuery);
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
        CHECK_OBJ(m_pShmDsn);
        //�����״μ��㴦��
        TShmList<TMdbJob>::iterator itorMem = m_pShmDsn->m_JobList.begin();
        for(;itorMem != m_pShmDsn->m_JobList.end();++itorMem)
        {
            TMdbJob * pJob = &(*itorMem);
            if(pJob->IsValid())
            {
                CalcNextExecDate(pJob);
            }
        }
        TADD_NORMAL("after scan job");
        //�������ݿ�
        CHECK_RET(ConnectMDB(m_pShmDsn->GetInfo()->sName),"connect mdb failed.");
        //ѭ������
        while(1)
        {
            m_tProcCtrl.UpdateProcHeart(0);//��������
            if(m_tProcCtrl.IsCurProcStop())
            {
                TADD_NORMAL("Get exit msg....");
                break;
            }
            //��������job
            char sCurTime[MAX_TIME_LEN] = {0};
            TMdbDateTime::GetCurrentTimeStr(sCurTime);
            itorMem = m_pShmDsn->m_JobList.begin();
            for(;itorMem != m_pShmDsn->m_JobList.end();++itorMem)
            {
                TMdbJob * pJob = &(*itorMem);
                if(pJob->IsValid() && TMdbDateTime::GetDiffSeconds(pJob->m_sNextExecuteDate,sCurTime) < 0)
                {//��Ҫִ��
                    pJob->StateToRunning();//�л�״̬��ִ��̬
                    DoJob(pJob);
                    CalcNextExecDate(pJob);//������һ��ִ��ʱ��
                    pJob->StateToWait();//�л�״̬���ȴ�̬
                    m_tProcCtrl.UpdateProcHeart(0);
                }
            }
            TMdbDateTime::Sleep(1);//��Ϣ
        }
        TADD_FUNC("Finish");
        return iRet;
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
    int TMdbJobCtrl::CalcNextExecDate(TMdbJob * pJob)
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


	int TMdbJobCtrl::AddOneTimeInterval(TMdbJob * pJob)
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
    * ��������	:  DoJob
    * ��������	:  ����job
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::DoJob(TMdbJob * pJob)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pJob);
        TADD_NORMAL("Try to do job[%s]",pJob->m_sSQL);
        _TRY_CATCH_BEGIN_
        CHECK_OBJ(m_pQuery);
        m_pQuery->Close();
        m_pQuery->SetSQL(pJob->m_sSQL);
        //��ִ��select
        if(TK_SELECT == m_pQuery->GetSQLType())
        {
            TADD_ERROR(ERROR_UNKNOWN,"do not execute select sql[%s]",pJob->m_sSQL);
        }
        else if(TK_DELETE == m_pQuery->GetSQLType())
        {
            DealDeleteTask(pJob->m_sSQL);
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
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  ConnectMDB
    * ��������	:  �������ݿ�
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::ConnectMDB(const char * sDsn)
    {
        int iRet = 0;
        TMdbConfig * pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
        CHECK_OBJ(pConfig);
        TMDbUser * pUser = pConfig->GetUser(0);
        CHECK_OBJ(pUser);
        _TRY_CATCH_BEGIN_
        if(false == m_tDB.Connect(pUser->sUser,pUser->sPwd,sDsn))
        {//����ʧ��
            CHECK_RET(ERR_APP_INVALID_PARAM,"connect mdb by[%s/******@%s] failed.",pUser->sUser,sDsn);
        }
        m_pQuery = m_tDB.CreateDBQuery();
        CHECK_OBJ(m_pQuery);
        m_pDelQuery = m_tDB.CreateDBQuery();
        CHECK_OBJ(m_pDelQuery);
        _TRY_CATCH_END_
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

    /******************************************************************************
    * ��������	:  DealDeleteTask
    * ��������	:  ɾ���������
    * ����		:  pDelSQL -  ɾ��sql
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbJobCtrl::DealDeleteTask(const char*pDelSQL)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iDelCount = 0;
        char sDelSQL[MAX_SQL_LEN] = {0};
        char sSelSQL[MAX_SQL_LEN] = {0};
		int iColumnNo = 0;
        CHECK_OBJ(pDelSQL);
        _TRY_CATCH_BEGIN_
        CHECK_OBJ(m_pQuery);
        TMdbTable * pTable = m_pQuery->m_pMdbSqlParser->m_stSqlStruct.pMdbTable;
        CHECK_RET(GenSelctSQLByDelSQL(pTable,pDelSQL,sSelSQL,MAX_SQL_LEN),"Failed to get query SQL.");
        CHECK_RET(GenDelSQL(pTable,sDelSQL,MAX_SQL_LEN),"Failed to get the delete SQL.");
        m_pQuery->Close();
        m_pDelQuery->Close();
        m_pQuery->SetSQL(sSelSQL);
        m_pDelQuery->SetSQL(sDelSQL);
        m_pQuery->Open();
        while(m_pQuery->Next())
        {
            iDelCount++;
			iColumnNo = 0;
            for(int i= 0;i<m_pQuery->FieldCount();i++)
            {
                if(m_pQuery->Field(i).isNULL())
                {
                    m_pDelQuery->SetParameterNULL(i);
                }
                iColumnNo = pTable->m_tPriKey.iColumnNo[i];
                if(pTable->tColumn[iColumnNo].iDataType == DT_Int)
                {
                    m_pDelQuery->SetParameter(i,m_pQuery->Field(i).AsInteger());
                }
                else
                {
                    m_pDelQuery->SetParameter(i,m_pQuery->Field(i).AsString());
                }
        	}
            m_pDelQuery->Execute();
            if(iDelCount%10000 == 0)
            {
               m_pDelQuery->Commit();
               m_tProcCtrl.UpdateProcHeart(0);
            }
        }
        if(iDelCount%10000 != 0)
        {
            m_pDelQuery->Commit();
        }
        _TRY_CATCH_END_
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GenSelctSQLByDelSQL
    * ��������	:  ͨ��ִ��job����sql��ȡ����sql
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbJobCtrl::GenSelctSQLByDelSQL(TMdbTable * pTable,const char*pDelSQL,char *pSSQL,const int iLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pTable);
        CHECK_OBJ(pDelSQL);
        snprintf(pSSQL,iLen,"%s","select ");
        for(int i=0; i<pTable->m_tPriKey.iColumnCounts; ++i)
        {
            int iColumnNo = pTable->m_tPriKey.iColumnNo[i];
            if(i == 0)
            {
                snprintf(pSSQL+strlen(pSSQL),iLen-strlen(pSSQL)," %s", pTable->tColumn[iColumnNo].sName);
            }
            else
            {
                snprintf(pSSQL+strlen(pSSQL),iLen-strlen(pSSQL),",%s", pTable->tColumn[iColumnNo].sName);
            }
        }
        snprintf(pSSQL+strlen(pSSQL),iLen-strlen(pSSQL)," from %s ",pTable->sTableName);
        //��ȡɾ��SQL�е�where����
        char pTmpSQL[MAX_SQL_LEN] = {0};
        SAFESTRCPY(pTmpSQL, MAX_SQL_LEN, pDelSQL);
        TMdbNtcStrFunc::ToUpper(pTmpSQL);
        int iPos = TMdbNtcStrFunc::FindString(pTmpSQL,"WHERE");
        if(iPos >= 0)
        {
            snprintf(pSSQL+strlen(pSSQL),iLen-strlen(pSSQL)," %s ",pTmpSQL+iPos);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  GenDelSQL
    * ��������	:  ��ȡɾ��SQL
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbJobCtrl::GenDelSQL(TMdbTable * pTable,char *pDelSQL,const int iLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pTable);
        snprintf(pDelSQL,iLen,"delete from %s where ",pTable->sTableName);
        for(int i=0; i<pTable->m_tPriKey.iColumnCounts; ++i)
        {
            int iColumnNo = pTable->m_tPriKey.iColumnNo[i];
            if(i == 0)
            {
                snprintf(pDelSQL+strlen(pDelSQL),iLen-strlen(pDelSQL),"%s=:%s",\
                    pTable->tColumn[iColumnNo].sName, pTable->tColumn[iColumnNo].sName);
            }
            else
            {
                snprintf(pDelSQL+strlen(pDelSQL),iLen-strlen(pDelSQL)," and %s=:%s",\
                    pTable->tColumn[iColumnNo].sName, pTable->tColumn[iColumnNo].sName);
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
//}

