/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    mdbProcCtrl.h		
*@Description： 内存数据库的进程管理控制
*@Author:		li.shugang
*@Date：	    2008年12月05日
*@History:
******************************************************************************************/
#ifndef __MINI_DATABASE_PROCESS_CONTRL_H__
#define __MINI_DATABASE_PROCESS_CONTRL_H__

#include "Helper/mdbStruct.h"
#include "Control/mdbMgrShm.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbJson.h"

//namespace QuickMDB{

    

    class TMdbProcCtrl
    {
    public:
        TMdbProcCtrl();
        ~TMdbProcCtrl();

        /******************************************************************************
        * 函数名称	:  Init()
        * 函数描述	:  初始化：Attach共享内存，清空进程信息  
        * 输入		:  pszDSN, 锁管理区所属的DSN  
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Init(const char* pszDSN);


        /******************************************************************************
        * 函数名称	:  Restart()
        * 函数描述	:  重新启动某个进程  
        * 输入		:  pszProcName, 进程名
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int Restart(const char* pszProcName,bool bSkipWarnLog = false);

        /******************************************************************************
        * 函数名称	:  StopAll()
        * 函数描述	:  停止所有进程  
        * 输入		:  无
        * 输出		:  无
        * 返回值	:  成功返回0，否则返回-1
        * 作者		:  li.shugang
        *******************************************************************************/
        int StopAll();
        bool IsDBStop();
        void ClearProcInfo();
        TMdbProc*   RegProc(const char * sProcName,const bool bIsMonitor = true);
        int UpdateProcHeart(int iWaitSec);//更新某进程心跳
        int ScanAndClear();//检测并清除进程信息
        bool IsCurProcStop();//检测当前进程是否需要停止
        bool IsMonitorStart();//监控进程是否已经启动
        bool IsMdbSysProcess(const char * sProcName);//判断是不是MDB系统进程
        int ResetLogLevel();//重置进程日志级别
        int Serialize(rapidjson::PrettyWriter<TMdbStringStream> & writer);//序列化
        bool IsMdbSysProcExist(const char * sDsn);//是否有系统进程存在
        bool IsProcExist(TMdbProc * pProc,const char * sCurTime);//该进程是否存在
        int StopProcess(const int iPID,const int iHeartBeatFatal);//停掉指定进程
        bool IsLongHeartBeatProcess(TMdbProc *pProc);//判断是否为长心跳监控进程
        char GetProcState();//获取进程状态
    private:
        TMdbShmDSN* m_pShmDSN;
        TMdbConfig *m_pConfig;
        TMdbDSN    *m_pDsn;    
        TMdbProc * m_pCurProc;//记录当前使用的
    };
//}

#endif //__MINI_DATABASE_PROCESS_CONTRL_H__

