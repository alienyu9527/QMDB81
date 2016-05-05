/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	    mdbQueueLog.cpp		
*@Description： mdb内存队列数据落地
*@Author:       dong.chun
*@Date：	    2013.4
*@History:
******************************************************************************************/
#include "Replication/mdbQueueLog.h"
#include "Control/mdbMgrShm.h"
#include "Helper/mdbDateTime.h"
#include "Helper/mdbQueue.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;


//namespace QuickMDB{

    TMdbQueueLog::TMdbQueueLog()
    {
        m_pShmDsn = NULL;
        m_pDsn = NULL;
        m_pConfig = NULL;
    }
    TMdbQueueLog::~TMdbQueueLog()
    {

    }

    int TMdbQueueLog::Init(const char * sDsn)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(sDsn);
        m_tProcCtrl.Init(sDsn);
        m_pShmDsn = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDsn);
        m_pDsn = m_pShmDsn->GetInfo();
        CHECK_OBJ(m_pDsn);    
        m_pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
        CHECK_OBJ(m_pConfig);
        TMdbMemQueue  * pMemQueue  = (TMdbMemQueue  *)m_pShmDsn->GetSyncAreaShm();
        CHECK_OBJ(pMemQueue);
        CHECK_RET(m_tMdbQueueCtrl.Init(pMemQueue,m_pShmDsn->GetInfo(),true),"tMdbQueueCtrl init failed.");

        CHECK_RET(m_mdbDBlog.Init(sDsn, m_tMdbQueueCtrl),"mdbDBlog init failed.");

        if(m_pConfig->GetIsStartShardBackupRep() == true)
        {
            CHECK_RET(m_mdbReplog.Init(sDsn, m_tMdbQueueCtrl),"mdbReplog init failed.");
        }

        if(true == m_pDsn->m_bIsCaptureRouter)
        {
            CHECK_RET(m_tCaptureLog.Init(sDsn, m_tMdbQueueCtrl),"m_tCaptureLog init failed.");
        }

        CHECK_RET(m_mdbRedolog.Init(sDsn, m_tMdbQueueCtrl),"mdbRedolog init failed.");

        TADD_FUNC("Finish");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  Start
    * 函数描述	:  
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbQueueLog::Start()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        while(true)
        {
            if(m_tProcCtrl.IsCurProcStop())
            {
                TADD_NORMAL("Get exit msg....");
                iRet = 0;
                break;
            }
            m_tProcCtrl.UpdateProcHeart(0);//进程心跳时间 

            iRet = m_tMdbQueueCtrl.Pop();
            switch(iRet)
            {
            case T_EMPTY://消息队列空
                {
                    CheckBackupFile();
                    TMdbDateTime::MSleep(10);
                    break;
                }           
            case T_SUCCESS://有数据
                {
                    WriteToFile();
                    break;
                }
            default:
                {
                    TADD_ERROR(iRet, "Queue iRet = [%d]",iRet);
                    break;
                }
            }
        }
        TADD_FUNC("Finish");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  WriteToFile
    * 函数描述	:  写入文件
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbQueueLog::WriteToFile()
    {
        TADD_FUNC("Start.");

        bool bOraEmpty = true;
        bool bShardEmpty = true;
        bool bRedoEmpty = true;
        bool bCaptureEmpty = true;
        char* pData = NULL;

        TADD_DETAIL("data=[%s]",m_tMdbQueueCtrl.GetData());
        pData = m_tMdbQueueCtrl.GetData(); 
        if(pData[0] != 0)//消息队列有数据，判断数据类型
        {
            static int iSyncType = 0;
            iSyncType = GetSyncType(pData);
            TADD_DETAIL("iSyncType=[%0xd]",iSyncType);

            if(FlushTypeHasProperty(iSyncType,FLUSH_ORA))//向数据库同步
            {
                bOraEmpty = false;
            }
            if(FlushTypeHasProperty(iSyncType,FLUSH_SHARD_BACKUP))//分片备份
            {
                bShardEmpty = false;
            }
            if(FlushTypeHasProperty(iSyncType,FLUSH_REDO))//回滚日志
            {
                bRedoEmpty = false;
            }
            if(FlushTypeHasProperty(iSyncType,FLUSH_CAPTURE))//捕获日志
            {
                bCaptureEmpty = false;
            }
        }

        m_mdbDBlog.Log(bOraEmpty);
        m_mdbRedolog.Log(bRedoEmpty);
        if(m_pConfig->GetIsStartShardBackupRep() == true)
        {
            m_mdbReplog.Log(bShardEmpty);
        }
        if(true == m_pDsn->m_bIsCaptureRouter)
        {
            m_tCaptureLog.Log(bCaptureEmpty);
        }

        TADD_FUNC("Finish");
        return 0;
    }

    int TMdbQueueLog::CheckBackupFile()
    {
        TADD_FUNC("Start.");

        m_mdbDBlog.Log(true);        
        m_mdbRedolog.Log(true);
        if(m_pConfig->GetIsStartShardBackupRep() == true)
        {
            m_mdbReplog.Log(true);
        }        
        if(true == m_pDsn->m_bIsCaptureRouter)
        {
            m_tCaptureLog.Log(true);
        }
        
        TADD_FUNC("Finish");
        return 0;

    }

    /******************************************************************************
    * 函数名称	:  GetSyncType
    * 函数描述	:  获取同步方向
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbQueueLog::GetSyncType(char * sData)
    {
        int iSyncType = 0;
        char cVersion = m_LogParser.GetVersion(sData);

        if(cVersion == VERSION_DATA_SYNC)
        {
            iSyncType = m_LogParser.GetSyncFlag(sData);
            
        }
        else if(cVersion == VERSION_DATA_CAPTURE)
        {
            if(true == m_pDsn->m_bIsCaptureRouter)
            {
                FlushTypeSetProperty(iSyncType,FLUSH_CAPTURE);
            }
        }

        return iSyncType;
    }


//}

