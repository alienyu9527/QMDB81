/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	   mdbJobCtrl .cpp		
*@Description： job管理
*@Author:			jin.shaohua
*@Date：	    2013.5
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
    * 函数名称	:  
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbJobCtrl::TMdbJobCtrl():
    m_pShmDsn(NULL)
    {
        m_pQuery = NULL;
        m_pDelQuery = NULL;
    }
    /******************************************************************************
    * 函数名称	:  
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbJobCtrl::~TMdbJobCtrl()
    {
        SAFE_DELETE(m_pQuery);
        SAFE_DELETE(m_pDelQuery);
    }
    /******************************************************************************
    * 函数名称	:  Init
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
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
    * 函数名称	:  LoadJobs
    * 函数描述	:  重新从配置文件加载
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::LoadJobs(const char * sDsn)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbShmDSN * pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(pShmDsn);
        TMdbConfig * pConfig = TMdbConfigMgr::GetMdbConfig(pShmDsn->GetInfo()->sName);
        CHECK_OBJ(pConfig);
        //清理共享内存中的
        TShmList<TMdbJob>::iterator itorMem = pShmDsn->m_JobList.begin();
        for(;itorMem != pShmDsn->m_JobList.end();++itorMem)
        {
            itorMem->Clear();
        }
        //读取配置文件加载
        std::vector<TMdbJob>::iterator itorCfg = pConfig->m_vMdbJob.begin();
        for(;itorCfg != pConfig->m_vMdbJob.end();++itorCfg)
        {
            TMdbJob * pMemJob = NULL;
            CHECK_RET(pShmDsn->AddNewJob(pMemJob),"AddNewJob faild.");
            CHECK_OBJ(pMemJob);
            memcpy(pMemJob,&(*itorCfg),sizeof(TMdbJob));//填充到内存中
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  StartJob
    * 函数描述	:  启动jobs
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::StartJob()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(m_pShmDsn);
        //进行首次计算处理
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
        //链接数据库
        CHECK_RET(ConnectMDB(m_pShmDsn->GetInfo()->sName),"connect mdb failed.");
        //循环处理
        while(1)
        {
            m_tProcCtrl.UpdateProcHeart(0);//心跳处理
            if(m_tProcCtrl.IsCurProcStop())
            {
                TADD_NORMAL("Get exit msg....");
                break;
            }
            //处理所有job
            char sCurTime[MAX_TIME_LEN] = {0};
            TMdbDateTime::GetCurrentTimeStr(sCurTime);
            itorMem = m_pShmDsn->m_JobList.begin();
            for(;itorMem != m_pShmDsn->m_JobList.end();++itorMem)
            {
                TMdbJob * pJob = &(*itorMem);
                if(pJob->IsValid() && TMdbDateTime::GetDiffSeconds(pJob->m_sNextExecuteDate,sCurTime) < 0)
                {//需要执行
                    pJob->StateToRunning();//切换状态到执行态
                    DoJob(pJob);
                    CalcNextExecDate(pJob);//计算下一次执行时间
                    pJob->StateToWait();//切换状态到等待态
                    m_tProcCtrl.UpdateProcHeart(0);
                }
            }
            TMdbDateTime::Sleep(1);//休息
        }
        TADD_FUNC("Finish");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  CalcNextExecDate
    * 函数描述	:  计算下一次执行的时间
    * 输入		:  pJob -  待计算的job
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::CalcNextExecDate(TMdbJob * pJob)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sCurTime[MAX_TIME_LEN] = {0};
        TMdbDateTime::GetCurrentTimeStr(sCurTime);
        if(TMdbDateTime::GetDiffSeconds(sCurTime,pJob->m_sExecuteDate) < 0)
        {//未来的某个时间点才执行
            memcpy(pJob->m_sNextExecuteDate,pJob->m_sExecuteDate,MAX_TIME_LEN);
            return 0;
        }
		else if(0 == pJob->m_sNextExecuteDate[0])
	    {    	
	        memcpy(pJob->m_sNextExecuteDate,pJob->m_sExecuteDate,MAX_TIME_LEN);
			//从起始时间开始计算，直到找到当前时间之后的时间点
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
    * 函数名称	:  DoJob
    * 函数描述	:  处理job
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
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
        //不执行select
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
        {//首次执行,记录首次执行时间
            SAFESTRCPY(pJob->m_sStartTime,sizeof(pJob->m_sStartTime),pJob->m_sNextExecuteDate);
        }
        pJob->m_iExcCount ++;
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  ConnectMDB
    * 函数描述	:  链接数据库
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
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
        {//链接失败
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
    * 函数名称	:  AddNewJob
    * 函数描述	:  添加新job
    * 输入		:  pNewJob -  新增的job
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::AddNewJob(TMdbJob * pNewJob)
    {
      TADD_FUNC("Start.");
      int iRet = 0;
      CHECK_OBJ(pNewJob);
      CHECK_OBJ(m_pShmDsn);
      if(pNewJob->IsValid())
      {
            //判断是否存在
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
      {//不合法的job
        CHECK_RET(ERR_APP_INVALID_PARAM,"job[%s] is invalid",pNewJob->ToString().c_str());
      }
      TADD_FUNC("Finish.");
      return iRet;
    }

    /******************************************************************************
    * 函数名称	:  DeleteJobByName
    * 函数描述	:  根据job name 删除job
    * 输入		:  sJobName -  待删除job
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbJobCtrl::DeleteJobByName(const char * sJobName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(NULL == sJobName || 0 == sJobName[0]){return 0;}//无效name
        TMdbJob * pJobToDel = m_pShmDsn->GetJobByName(sJobName);
        if(NULL == pJobToDel)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"Not find job[%s]",sJobName);
        }
        do
        {
            if(pJobToDel->IsCanModify())
            {//可以变更
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
    * 函数名称	:  DealDeleteTask
    * 函数描述	:  删除任务操作
    * 输入		:  pDelSQL -  删除sql
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  cao.peng
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
    * 函数名称	:  GenSelctSQLByDelSQL
    * 函数描述	:  通过执行job任务sql获取反查sql
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  cao.peng
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
        //获取删除SQL中的where条件
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
    * 函数名称	:  GenDelSQL
    * 函数描述	:  获取删除SQL
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  cao.peng
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

