/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbProcCtrl.cpp
*@Description： 内存数据库的进程管理控制
*@Author:       li.shugang
*@Date：        2008年12月05日
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
    * 函数名称  :  Init()
    * 函数描述  :  初始化：Attach共享内存，取出进程信息所在位置
    * 输入      :  pszDSN, 锁管理区所属的DSN
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  li.shugang
    *******************************************************************************/
    int TMdbProcCtrl::Init(const char* pszDSN)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pszDSN);
        m_pConfig = TMdbConfigMgr::GetDsnConfig(pszDSN);
        CHECK_OBJ(m_pConfig);
        //连接上共享内存
        m_pShmDSN = TMdbShmMgr::GetShmDSN(pszDSN,false);
        CHECK_OBJ(m_pShmDSN);
        m_pDsn = m_pShmDSN->GetInfo();
        m_pCurProc = m_pShmDSN->GetProcByPid(TMdbOS::GetPID());//可能还没注册
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称  :  Restart()
    * 函数描述  :  重新启动某个进程
    * 输入      :  pszProcName, 进程名
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  li.shugang
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
            //进程存在
            if(!bSkipWarnLog)
            {
                TADD_WARNING("Process=[%s] is running, can't restart.", pszProcName);
            }
            return 0;
        }
        else
        {
            //进程不存在
            system(pszProcName);//启动进程
            TADD_DETAIL("system(%s) OK.",pszProcName);
            TMdbDateTime::Sleep(1);
            //等待进程启动
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
    * 函数名称	:  StopAll()
    * 函数描述	:  停止所有进程
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  li.shugang
    *******************************************************************************/
    int TMdbProcCtrl::StopAll()
    {
        int iRet = 0;
        CHECK_OBJ(m_pShmDSN);
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        //先设置状态
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc & tMdbProc = *itor; 
            if(tMdbProc.sName[0] == 0 || false == TMdbOS::IsProcExistByPopen(tMdbProc.iPid))
            {
                continue;
            }
            tMdbProc.cState = PSTOP;
        }
        TMdbDateTime::Sleep(2);//停止2秒
        //检测并强制退出
        itor = tProcList.begin();
        for(;itor != tProcList.end();++itor)
        {
            TMdbProc & tMdbProc = *itor; 
            if(tMdbProc.sName[0] == 0     || TMdbNtcStrFunc::FindString(tMdbProc.sName,"QuickMDB") != -1 ||
                    false == TMdbOS::IsProcExistByPopen(tMdbProc.iPid) )
            {//已退出的跳过,QuickMDB进程不强制退出
                continue;
            }
            //等待一个心跳时间
            int iSleepCount = 0;
            do
            {
                if(IsMdbSysProcess(tMdbProc.sName) == false)
                {//非mdb进程强制结束
                    break;
                }
                iSleepCount ++;
                TMdbDateTime::Sleep(1);
                if(false == TMdbOS::IsProcExistByPopen(tMdbProc.iPid)){break;}
                TADD_NORMAL("wait process[%s] exit normal.",tMdbProc.sName);
            }while(iSleepCount < m_pConfig->GetProAttr()->iHeartBeatFatal);
            //强制退出
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
    * 函数名称	:  AddProc
    * 函数描述	:  向进程管理区添加进程信息
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
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
            //不管进程需要不要监控，已经注册过的进程，不在注册
            if(NULL != m_pCurProc)
            {
                if(TMdbOS::IsProcExistByPopen(m_pCurProc->iPid))
                {//已经存在，不要重复注册
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
                    SAFESTRCPY(m_pCurProc->sName, sizeof(m_pCurProc->sName),sProcName);//进程名最后复制
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
    * 函数名称	:  UpdateProcHeart
    * 函数描述	:  更新某进程心跳
    * 输入		:  iWaitSec -  额外等待时间
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbProcCtrl::UpdateProcHeart(int iWaitSec)
    {
        int iRet = 0;
        if(NULL == m_pCurProc || IsCurProcStop())
        {//程序已终止无需心跳
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
            //如果进程已经退出，无需延迟
            if(IsCurProcStop()){break;}
        }
        while(iWaitSec > 0);
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  ScanAndClear
    * 函数描述	:  检测并清除进程信息
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
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
            {//跳过空进程状态
                continue;
            }
            TADD_DETAIL("Process[%d]=[%s].",pTmpProc->iPid, pTmpProc->sName);
            //根据pid判断进程是否存在和状态
            if(false == IsProcExist(pTmpProc,&sCurTime[0]))
            {
                if(pTmpProc->bIsMonitor)
                {
                    //受监控进程需要输出
                    TADD_ERROR(ERROR_UNKNOWN,"Process[%d]=[%s]  isn't exist.", pTmpProc->iPid, pTmpProc->sName);
                }
                pTmpProc->cState= PKILL;//修改状态
            }

            if(pTmpProc->bIsMonitor)
            {
                //对于需要监控的进程判断心跳
                lDiffSec = TMdbDateTime::GetDiffSeconds(sCurTime, pTmpProc->sUpdateTime);
                if(IsLongHeartBeatProcess(pTmpProc))
                {
                    if(lDiffSec > 3600)
                    {
                        //超过错误时间
                        TADD_ERROR(ERROR_UNKNOWN,"Process[%s] need to restart.sCurTime=[%s],sUpdateTime=[%s],Heart-Beat-Fatal=[3600]",
                            pTmpProc->sName,sCurTime,pTmpProc->sUpdateTime);
                        pTmpProc->cState= PKILL;
                    }
                }
                else
                {
                    if(lDiffSec > 60*10)
                    {//误差太大时间获取错误，可能由于时区变动等情况
                        TADD_WARNING("lDiffSec[%lds]>600s,sCurTime=[%s],sUpdateTime=[%s].",lDiffSec,sCurTime,pTmpProc->sUpdateTime);
                    }
                    else if(lDiffSec > iHeartBeatWarning)
                    {
                        //超过警告时间
                        TADD_WARNING("Process[%d]=[%s]  may be not running in [%d]s,sCurTime=[%s],sUpdateTime=[%s].", pTmpProc->iPid, 
                            pTmpProc->sName,iHeartBeatWarning,sCurTime,pTmpProc->sUpdateTime);
                        if(lDiffSec > iHeartBeatFatal)
                        {
                            //超过错误时间
                            TADD_ERROR(ERROR_UNKNOWN,"Process[%s] need to restart.sCurTime=[%s],sUpdateTime=[%s],iHeartBeatFatal=[%d]",
                                       pTmpProc->sName,sCurTime,pTmpProc->sUpdateTime,iHeartBeatFatal);
                            pTmpProc->cState= PKILL;
                        }
                    }
                }
            }
            
            //清理或重启进程
            if(PKILL == pTmpProc->cState)
            {
                if(IsProcExist(pTmpProc,&sCurTime[0]))
                {
                    TADD_NORMAL("Kill Process[%s][%d].",pTmpProc->sName,pTmpProc->iPid);
                    TMdbOS::KillProc(pTmpProc->iPid);
                }
                if(pTmpProc->bIsMonitor)
                {
                    //监控进程
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
                    //非监控进程,清理信息
                    TADD_NORMAL("Clear Process[%d]=[%s].", pTmpProc->iPid, pTmpProc->sName);
                    pTmpProc->Clear();
                }
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  IsCurProcStop
    * 函数描述	:  检测当前进程是否需要停止
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
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
    * 函数名称	:  IsMonitorStart
    * 函数描述	:  监控进程是否已经启动
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool TMdbProcCtrl::IsMonitorStart()
    {
        TMdbProc * pProc = m_pShmDSN->GetProcByName("QuickMDB");
        if(NULL == pProc){return false;}
        return TMdbOS::IsProcExistByPopen(pProc->iPid);
    }
    /******************************************************************************
    * 函数名称	:  IsMdbSysProcess
    * 函数描述	:  判断是不是MDB系统进程
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
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
            {//找到
                return true;
            }
        }
        return false;
    }

    /******************************************************************************
    * 函数名称	:  ResetLogLevel
    * 函数描述	:  重置进程日志级别
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:
    * 作者		:  jin.shaohua
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
    * 函数名称	:  Serialize
    * 函数描述	:  序列化
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
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
    * 函数名称	:  IsMdbSysProcExist
    * 函数描述	: 是否有系统进程存在
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
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
    * 函数名称	:  IsProcExist
    * 函数描述	: 该进程是否存在
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    bool TMdbProcCtrl::IsProcExist(TMdbProc * pProc,const char * sCurTime)
    {
        if(NULL == pProc){return false;}
        if(pProc->iPid < 0){return false;}
        if(IsMdbSysProcess(pProc->sName))
        {//系统进程
            return TMdbOS::IsProcExistByKill(pProc->iPid);
        }
        else
        {//业务进程
            if(TMdbDateTime::GetDiffSeconds(sCurTime,pProc->sUpdateTime) >= 15)
            {//15秒检测一次
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
    * 函数名称	:  StopProces()
    * 函数描述	:  停掉指定进程
    * 输入		:  无
    * 输出		:  无
    * 返回值	:  成功返回0，否则返回-1
    * 作者		:  cao.peng
    *******************************************************************************/
    int TMdbProcCtrl::StopProcess(const int iPID,const int iHeartBeatFatal)
    {
        int iRet = 0;
        CHECK_OBJ(m_pShmDSN);
        TShmList<TMdbProc> & tProcList = m_pShmDSN->m_ProcList;
        TShmList<TMdbProc>::iterator itor = tProcList.begin();
        //先设置状态
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
            TMdbDateTime::Sleep(2);//停止2秒
            //检测并强制退出
            if(TMdbNtcStrFunc::FindString(tMdbProc.sName,"QuickMDB") != -1
                || !TMdbOS::IsProcExistByPopen(tMdbProc.iPid))
            {//已退出的跳过,QuickMDB进程不强制退出
                break;
            }
            //等待一个心跳时间
            int iSleepCount = 0;
            do
            {
                if(IsMdbSysProcess(tMdbProc.sName) == false)
                {//非mdb进程强制结束
                    break;
                }
                iSleepCount ++;
                TMdbDateTime::Sleep(1);
                if(false == TMdbOS::IsProcExistByPopen(tMdbProc.iPid)){break;}
                TADD_NORMAL("wait process[%s] exit normal.",tMdbProc.sName);
            }while(iSleepCount < iHeartBeatFatal);
            //强制退出
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
    * 函数名称  :  IsMonitorProcess()
    * 函数描述  :  判断是否为长心跳监控进程
    * 输入		:  无
    * 输出		:  无
    * 返回值    :  true:长心跳监控进程;false:短心跳监控进程
    * 作者		:  cao.peng
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

