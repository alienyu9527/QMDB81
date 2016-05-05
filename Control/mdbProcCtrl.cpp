/****************************************************************************************
*@Copyrights  2008�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��        mdbProcCtrl.cpp
*@Description�� �ڴ����ݿ�Ľ��̹������
*@Author:       li.shugang
*@Date��        2008��12��05��
*@History:
******************************************************************************************/

#include "Control/mdbProcCtrl.h"
#include "Helper/mdbOS.h"
#include "Helper/mdbDateTime.h"

#ifndef _WIN32
#include <sys/types.h>
#include <signal.h>
#else
#include <windows.h>
#include <tlhelp32.h>
#endif

//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

    TMdbProcCtrl::TMdbProcCtrl()
    {
        m_pCurProc = NULL;
        m_pDsn    = NULL;
        m_pShmDSN = NULL;
        m_pConfig = NULL;
    }


    TMdbProcCtrl::~TMdbProcCtrl()
    {

    }


    bool TMdbProcCtrl::IsDBStop()
    {
        return m_pShmDSN->GetInfo()->cState == DB_stop;
    }

    /******************************************************************************
    * ��������  :  Init()
    * ��������  :  ��ʼ����Attach�����ڴ棬ȡ��������Ϣ����λ��
    * ����      :  pszDSN, ��������������DSN
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻�-1
    * ����      :  li.shugang
    *******************************************************************************/
    int TMdbProcCtrl::Init(const char* pszDSN)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pszDSN);
        m_pConfig = TMdbConfigMgr::GetDsnConfig(pszDSN);
        CHECK_OBJ(m_pConfig);
        //�����Ϲ����ڴ�
        m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN,false);
        CHECK_OBJ(m_pShmDSN);
        m_pDsn = m_pShmDSN->GetInfo();
        m_pCurProc = m_pShmDSN->GetProcByPid(TMdbOS::GetPID());//���ܻ�ûע��
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  Restart()
    * ��������  :  ��������ĳ������
    * ����      :  pszProcName, ������
    * ���      :  ��
    * ����ֵ    :  �ɹ�����0�����򷵻�-1
    * ����      :  li.shugang
    *******************************************************************************/
    int TMdbProcCtrl::Restart(const char* pszProcName,bool bSkipWarnLog)
    {
        if(IsDBStop())
        {
            return 0;
        }
        TADD_FUNC("Start.[%s]", pszProcName);
        if(NULL == m_pShmDSN)
        {
            TADD_ERROR(ERR_OS_ATTACH_SHM,"Not attach manager.");
            return ERR_OS_ATTACH_SHM;
        }
        if(TMdbOS::IsProcExist(pszProcName) ==  true)
        {
            //���̴���
            if(!bSkipWarnLog)
            {
                TADD_WARNING("Process=[%s] is running, can't restart.", pszProcName);
            }
            return 0;
        }
        else
        {
            //���̲�����
            system(pszProcName);//��������
            TADD_DETAIL("system(%s) OK.",pszProcName);
            TMdbDateTime::Sleep(1);
            //�ȴ���������
            int iCounts = 0;
            TMdbProc * pProc = NULL;
            while(true)
            {
                if(IsDBStop())
                {
                    TADD_NORMAL("QuickMDB is stopping....");
                    return ERR_DB_NOT_CREATE;
                }
                pProc = m_pShmDSN->GetProcByName(pszProcName);
                if(TMdbOS::IsProcExist(pszProcName) ==  false || NULL == pProc)
                {
                    TMdbDateTime::Sleep(1);
                    if(iCounts%5 == 0)
                    {
                        TADD_NORMAL("Waiting for process=[%s] to start.", pszProcName);
                    }
                    if(iCounts%31 == 30)
                    {
                        TADD_ERROR(ERR_SQL_EXECUTE_TIMEOUT,"Can't start process=[%s].", pszProcName);
                        return ERR_SQL_EXECUTE_TIMEOUT;
                    }
                    ++iCounts;
                }
                else
                {
                    TADD_NORMAL("Start Process=[%s][%d].",pProc->sName,pProc->iPid);
                    break;
                }
            }
        }
        return 0;
    }


    /******************************************************************************
    * ��������	:  StopAll()
    * ��������	:  ֹͣ���н���
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  li.shugang
    *******************************************************************************/
    int TMdbProcCtrl::StopAll()
    {
        int iRet = 0;
        CHECK_OBJ(m_pShmDSN);
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        //������״̬
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc & tMdbProc = *itor; 
            if(tMdbProc.sName[0] == 0 || false == TMdbOS::IsProcExistByPopen(tMdbProc.iPid))
            {
                continue;
            }
            tMdbProc.cState = PSTOP;
        }
        TMdbDateTime::Sleep(2);//ֹͣ2��
        //��Ⲣǿ���˳�
        itor = tProcList.begin();
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc & tMdbProc = *itor; 
            if(tMdbProc.sName[0] == 0     || TMdbNtcStrFunc::FindString(tMdbProc.sName,"QuickMDB") != -1 ||
                    false == TMdbOS::IsProcExistByPopen(tMdbProc.iPid) )
            {//���˳�������,QuickMDB���̲�ǿ���˳�
                continue;
            }
            //�ȴ�һ������ʱ��
            int iSleepCount = 0;
            do
            {
                if(IsMdbSysProcess(tMdbProc.sName) == false)
                {//��mdb����ǿ�ƽ���
                    break;
                }
                iSleepCount ++;
                TMdbDateTime::Sleep(1);
                if(false == TMdbOS::IsProcExistByPopen(tMdbProc.iPid)){break;}
                TADD_NORMAL("wait process[%s] exit normal.",tMdbProc.sName);
            }while(iSleepCount < m_pConfig->GetProAttr()->iHeartBeatFatal);
            //ǿ���˳�
             if(TMdbOS::IsProcExistByPopen(tMdbProc.iPid) && 
                (IsMdbSysProcess(tMdbProc.sName) ||
                TMdbNtcStrFunc::FindString(tMdbProc.sName,"mdbSQL") != -1))
             {
                TADD_WARNING("Process=[%s] can't stop normal,kill it....", tMdbProc.sName);
                if(TMdbOS::KillProc(tMdbProc.iPid)== false)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Process=[%s] Can Not be Killed,Please Kill it by hand.", tMdbProc.sName);
                }
             }
        }
        return iRet;
    }
    void TMdbProcCtrl::ClearProcInfo()
    {
        TADD_FUNC("Start.");
        if(NULL == m_pShmDSN){return ;}
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        for(;itor != tProcList.end();itor ++)
        {
            TMdbProc * pMdbProc = &(*itor); 
            if(pMdbProc->iPid < 0){continue;}
            if(TMdbOS::IsProcExistByPopen(pMdbProc->iPid) == false)
            {
                TADD_NORMAL("Clear ProcPid=[%d][%s].", pMdbProc->iPid,pMdbProc->sName);
                itor->Clear();
            }
        }
        TADD_FUNC("Finish.");
        return;
    }

    /******************************************************************************
    * ��������	:  AddProc
    * ��������	:  ����̹�������ӽ�����Ϣ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    TMdbProc*  TMdbProcCtrl::RegProc(const char * sProcName,const bool bIsMonitor)
    {
        TADD_FUNC("Start[%s].",sProcName);
        int iRet = 0;
        if(m_pShmDSN->LockDSN() != 0)
        {
            TADD_ERROR(ERR_OS_MUTEX_LOCK,"LockDSN Faild.");
            return NULL;
        }
        do
        {
            m_pCurProc = m_pShmDSN->GetProcByPid(TMdbOS::GetPID());
            //���ܽ�����Ҫ��Ҫ��أ��Ѿ�ע����Ľ��̣�����ע��
            if(NULL != m_pCurProc)
            {
                if(TMdbOS::IsProcExistByPopen(m_pCurProc->iPid))
                {//�Ѿ����ڣ���Ҫ�ظ�ע��
                    break;
                }
                else
                {
                    m_pCurProc->cState = PFREE;
                    m_pCurProc->iPid = TMdbOS::GetPID();
                    m_pCurProc->bIsMonitor = bIsMonitor;
                    TMdbDateTime::GetCurrentTimeStr(m_pCurProc->sStartTime);
                    TMdbDateTime::GetCurrentTimeStr(m_pCurProc->sUpdateTime);
                }             
            }
            else
            {
                m_pCurProc = m_pShmDSN->GetProcByName(sProcName);
                if(m_pCurProc != NULL && bIsMonitor)
                {
                    m_pCurProc->cState = PFREE;
                    m_pCurProc->iPid = TMdbOS::GetPID();
                    m_pCurProc->bIsMonitor = bIsMonitor;
                    TMdbDateTime::GetCurrentTimeStr(m_pCurProc->sStartTime);
                    TMdbDateTime::GetCurrentTimeStr(m_pCurProc->sUpdateTime);
                }
                else
                {
                    CHECK_RET_BREAK(m_pShmDSN->AddNewProc(m_pCurProc),"AddNewProc[%s] failed.",sProcName);
                    m_pCurProc->Clear();
                    m_pCurProc->cState = PFREE;
                    m_pCurProc->bIsMonitor = bIsMonitor;
                    m_pCurProc->iPid = TMdbOS::GetPID();
                    m_pCurProc->iLogLevel = m_pDsn->iLogLevel;
                    TMdbDateTime::GetCurrentTimeStr(m_pCurProc->sStartTime);
                    TMdbDateTime::GetCurrentTimeStr(m_pCurProc->sUpdateTime);
                    SAFESTRCPY(m_pCurProc->sName, sizeof(m_pCurProc->sName),sProcName);//�����������
                    TADD_NORMAL("The registration process for %s, the registration time is %s.",\
                                         sProcName,m_pCurProc->sStartTime);
                }
            }
        }while(0);
        if(m_pShmDSN->UnLockDSN() != 0)
        {
            TADD_ERROR(ERR_OS_MUTEX_UNLOCK,"UnLockDSN Faild.");
            return NULL;
        }
        if(0 != iRet)return NULL;
        return m_pCurProc;
    }

    /******************************************************************************
    * ��������	:  UpdateProcHeart
    * ��������	:  ����ĳ��������
    * ����		:  iWaitSec -  ����ȴ�ʱ��
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbProcCtrl::UpdateProcHeart(int iWaitSec)
    {
        int iRet = 0;
        if(NULL == m_pCurProc || IsCurProcStop())
        {//��������ֹ��������
            return 0;
        }
        do
        {
            TMdbDateTime::GetCurrentTimeStr(m_pCurProc->sUpdateTime);
            if(iWaitSec > 0)
            {
                TMdbDateTime::Sleep(1);
                iWaitSec--;
            }
            //��������Ѿ��˳��������ӳ�
            if(IsCurProcStop()){break;}
        }
        while(iWaitSec > 0);
        return iRet;
    }
    /******************************************************************************
    * ��������	:  ScanAndClear
    * ��������	:  ��Ⲣ���������Ϣ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbProcCtrl::ScanAndClear()
    {
        TADD_FUNC("Start.");
        int iRet =0;
        char sCurTime[32] = {0};
        TMdbDateTime::GetCurrentTimeStr(sCurTime);
        int iHeartBeatWarning = m_pConfig->GetProAttr()->iHeartBeatWarning;
        int iHeartBeatFatal   = m_pConfig->GetProAttr()->iHeartBeatFatal;
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        long lDiffSec = 0;
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc *pTmpProc = &(*itor);
            if(pTmpProc->sName[0] == 0 || PSTOP == pTmpProc->cState)
            {//�����ս���״̬
                continue;
            }
            TADD_DETAIL("Process[%d]=[%s].",pTmpProc->iPid, pTmpProc->sName);
            //����pid�жϽ����Ƿ���ں�״̬
            if(false == IsProcExist(pTmpProc,&sCurTime[0]))
            {
                if(pTmpProc->bIsMonitor)
                {
                    //�ܼ�ؽ�����Ҫ���
                    TADD_ERROR(ERROR_UNKNOWN,"Process[%d]=[%s]  isn't exist.", pTmpProc->iPid, pTmpProc->sName);
                }
                pTmpProc->cState= PKILL;//�޸�״̬
            }

            if(pTmpProc->bIsMonitor)
            {
                //������Ҫ��صĽ����ж�����
                lDiffSec = TMdbDateTime::GetDiffSeconds(sCurTime, pTmpProc->sUpdateTime);
                if(IsLongHeartBeatProcess(pTmpProc))
                {
                    if(lDiffSec > 3600)
                    {
                        //��������ʱ��
                        TADD_ERROR(ERROR_UNKNOWN,"Process[%s] need to restart.sCurTime=[%s],sUpdateTime=[%s],Heart-Beat-Fatal=[3600]",
                            pTmpProc->sName,sCurTime,pTmpProc->sUpdateTime);
                        pTmpProc->cState= PKILL;
                    }
                }
                else
                {
                    if(lDiffSec > 60*10)
                    {//���̫��ʱ���ȡ���󣬿�������ʱ���䶯�����
                        TADD_WARNING("lDiffSec[%lds]>600s,sCurTime=[%s],sUpdateTime=[%s].",lDiffSec,sCurTime,pTmpProc->sUpdateTime);
                    }
                    else if(lDiffSec > iHeartBeatWarning)
                    {
                        //��������ʱ��
                        TADD_WARNING("Process[%d]=[%s]  may be not running in [%d]s,sCurTime=[%s],sUpdateTime=[%s].", pTmpProc->iPid, 
                            pTmpProc->sName,iHeartBeatWarning,sCurTime,pTmpProc->sUpdateTime);
                        if(lDiffSec > iHeartBeatFatal)
                        {
                            //��������ʱ��
                            TADD_ERROR(ERROR_UNKNOWN,"Process[%s] need to restart.sCurTime=[%s],sUpdateTime=[%s],iHeartBeatFatal=[%d]",
                                       pTmpProc->sName,sCurTime,pTmpProc->sUpdateTime,iHeartBeatFatal);
                            pTmpProc->cState= PKILL;
                        }
                    }
                }
            }
            
            //�������������
            if(PKILL == pTmpProc->cState)
            {
                if(IsProcExist(pTmpProc,&sCurTime[0]))
                {
                    TADD_NORMAL("Kill Process[%s][%d].",pTmpProc->sName,pTmpProc->iPid);
                    TMdbOS::KillProc(pTmpProc->iPid);
                }
                if(pTmpProc->bIsMonitor)
                {
                    //��ؽ���
                    if(m_pConfig->GetProAttr()->bAutoRestart)
                    {
                        TADD_NORMAL("Try To restart Process[%s].",pTmpProc->sName);
                        if(Restart(pTmpProc->sName) < 0)
                        {
                            TADD_ERROR(ERROR_UNKNOWN,"Process[%s] can't restart.", pTmpProc->sName);
                        }
                    }
                }
                else
                {
                    //�Ǽ�ؽ���,������Ϣ
                    TADD_NORMAL("Clear Process[%d]=[%s].", pTmpProc->iPid, pTmpProc->sName);
                    pTmpProc->Clear();
                }
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  IsCurProcStop
    * ��������	:  ��⵱ǰ�����Ƿ���Ҫֹͣ
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbProcCtrl::IsCurProcStop()
    {
        if(NULL == m_pCurProc|| PSTOP == m_pCurProc->cState)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    /******************************************************************************
    * ��������	:  IsMonitorStart
    * ��������	:  ��ؽ����Ƿ��Ѿ�����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbProcCtrl::IsMonitorStart()
    {
        TMdbProc * pProc = m_pShmDSN->GetProcByName("QuickMDB");
        if(NULL == pProc){return false;}
        return TMdbOS::IsProcExistByPopen(pProc->iPid);
    }
    /******************************************************************************
    * ��������	:  IsMdbSysProcess
    * ��������	:  �ж��ǲ���MDBϵͳ����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbProcCtrl::IsMdbSysProcess(const char * sProcName)
    {
        if(NULL == sProcName){return false;}
        const char * sSysProcess[] = {"QuickMDB","mdbAgent","mdbFlushRep",
                                                       "mdbRepClient","mdbRepServer","mdbRep",
                                                       "mdbDbRep","mdbFlushFromDb",
                                                       "mdbJob","mdbFlushSequence","mdbClean","mdbCheckPoint"};
        int iSysProcCount = 12;
        int i = 0;
        for(i = 0;i < iSysProcCount;++i)
        {
            if(TMdbNtcStrFunc::FindString(sProcName,sSysProcess[i]) != -1)
            {//�ҵ�
                return true;
            }
        }
        return false;
    }

    /******************************************************************************
    * ��������	:  ResetLogLevel
    * ��������	:  ���ý�����־����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbProcCtrl::ResetLogLevel()
    {
        TADD_FUNC("Start.");
        if(NULL == m_pShmDSN){return 0;}
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc * pTmp = &(*itor);
            if(0 != pTmp->sName[0] && pTmp->iLogLevel > 0)
            {
                TADD_NORMAL("ResetLogLevel : PID=[%d] ,set logLevel [%d] to [0].", pTmp->iPid,pTmp->iLogLevel);
                pTmp->iLogLevel = 0;
            }
        }
        TADD_FUNC("Finish.");
        return 0;
    }
    /******************************************************************************
    * ��������	:  Serialize
    * ��������	:  ���л�
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbProcCtrl::Serialize(rapidjson::PrettyWriter<TMdbStringStream> & writer)
    {
        int iRet = 0;
        CHECK_OBJ(m_pShmDSN);
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        writer.StartArray();
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc & tMdbProc = *itor;
            if(tMdbProc.iPid > 0)
            {
                writer.StartObject();
                tMdbProc.Serialize(writer);
                writer.EndObject();
            }
        }
        writer.EndArray();
        return iRet;
    }
    /******************************************************************************
    * ��������	:  IsMdbSysProcExist
    * ��������	: �Ƿ���ϵͳ���̴���
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbProcCtrl::IsMdbSysProcExist(const char * sDsn)
    {
        const char * sSysProcess[] = {"mdbClean","mdbAgent","mdbFlushRep","mdbRepClient","mdbRepServer","mdbFlushFromDb","mdbJob","mdbFlushSequence"};
        int iSysProcCount = 8;
        char sTemp[MAX_NAME_LEN] = {0};
        int i = 0;
        for(i = 0;i < iSysProcCount;++i)
        {
            sprintf(sTemp,"%s %s",sSysProcess[i],sDsn);
            if(TMdbOS::IsProcExist(sTemp))
            {
                TADD_NORMAL("[%s] is exist.",sTemp);
                return true;
            }
        }
        sprintf(sTemp,"mdbDbRep %s 1 -1",sDsn);
        if(TMdbOS::IsProcExist(sTemp))
        {
            TADD_NORMAL("[%s] is exist.",sTemp);
            return true;
        }
        return false;
    }

    /******************************************************************************
    * ��������	:  IsProcExist
    * ��������	: �ý����Ƿ����
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    bool TMdbProcCtrl::IsProcExist(TMdbProc * pProc,const char * sCurTime)
    {
        if(NULL == pProc){return false;}
        if(pProc->iPid < 0){return false;}
        if(IsMdbSysProcess(pProc->sName))
        {//ϵͳ����
            return TMdbOS::IsProcExistByKill(pProc->iPid);
        }
        else
        {//ҵ�����
            if(TMdbDateTime::GetDiffSeconds(sCurTime,pProc->sUpdateTime) >= 15)
            {//15����һ��
                 bool bRet = TMdbOS::IsProcExistByPopen(pProc->iPid);
                 if(bRet)
                 {
                     SAFESTRCPY(pProc->sUpdateTime,sizeof(pProc->sUpdateTime),sCurTime);
                 }
                 return bRet;
            }
            else
            {
                return true;
            }
        }
    }

    /******************************************************************************
    * ��������	:  StopProces()
    * ��������	:  ͣ��ָ������
    * ����		:  ��
    * ���		:  ��
    * ����ֵ	:  �ɹ�����0�����򷵻�-1
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbProcCtrl::StopProcess(const int iPID,const int iHeartBeatFatal)
    {
        int iRet = 0;
        CHECK_OBJ(m_pShmDSN);
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        //������״̬
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc & tMdbProc = *itor; 
            if(tMdbProc.sName[0] == 0 
                || false == TMdbOS::IsProcExistByPopen(tMdbProc.iPid))
            {
                continue;
            }
            if(iPID != tMdbProc.iPid)
            {
                continue;
            }
            tMdbProc.cState = PSTOP;
            TMdbDateTime::Sleep(2);//ֹͣ2��
            //��Ⲣǿ���˳�
            if(TMdbNtcStrFunc::FindString(tMdbProc.sName,"QuickMDB") != -1
                || !TMdbOS::IsProcExistByPopen(tMdbProc.iPid))
            {//���˳�������,QuickMDB���̲�ǿ���˳�
                break;
            }
            //�ȴ�һ������ʱ��
            int iSleepCount = 0;
            do
            {
                if(IsMdbSysProcess(tMdbProc.sName) == false)
                {//��mdb����ǿ�ƽ���
                    break;
                }
                iSleepCount ++;
                TMdbDateTime::Sleep(1);
                if(false == TMdbOS::IsProcExistByPopen(tMdbProc.iPid)){break;}
                TADD_NORMAL("wait process[%s] exit normal.",tMdbProc.sName);
            }while(iSleepCount < iHeartBeatFatal);
            //ǿ���˳�
            if(TMdbOS::IsProcExistByPopen(tMdbProc.iPid))
            {
                TADD_WARNING("Process=[%s] can't stop normal,kill it....", tMdbProc.sName);
                if(TMdbOS::KillProc(tMdbProc.iPid)== false)
                {
                    TADD_ERROR(ERROR_UNKNOWN,"Process=[%s] Can Not be Killed,Please Kill it by hand.", tMdbProc.sName);
                    return -1;
                }
            }
            break;
        }
        return iRet;
    }

    /******************************************************************************
    * ��������  :  IsMonitorProcess()
    * ��������  :  �ж��Ƿ�Ϊ��������ؽ���
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  true:��������ؽ���;false:��������ؽ���
    * ����		:  cao.peng
    *******************************************************************************/
    bool TMdbProcCtrl::IsLongHeartBeatProcess(TMdbProc *pProc)
    {
        if(NULL == pProc){return false;}
        if(pProc->bIsMonitor)
        {
            if(TMdbNtcStrFunc::FindString(pProc->sName,"mdbFlushFromDb") == -1 
                && TMdbNtcStrFunc::FindString(pProc->sName,"mdbCheckPoint") == -1
                && TMdbNtcStrFunc::FindString(pProc->sName,"mdbClean") == -1
                && TMdbNtcStrFunc::FindString(pProc->sName,"mdbFlushSequence") == -1
                && TMdbNtcStrFunc::FindString(pProc->sName,"mdbDbRep") == -1)
            {
                return false;
            }
            return true;
        }
        return false;
    }

    char TMdbProcCtrl::GetProcState()
    {
        if(NULL == m_pCurProc)
        {
            return PSTOP;
        }
        else
        {
            return m_pCurProc->cState;
        }
        
    }

//}

