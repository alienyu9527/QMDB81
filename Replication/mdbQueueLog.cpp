/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbQueueLog.cpp		
*@Description�� mdb�ڴ�����������
*@Author:       dong.chun
*@Date��	    2013.4
*@History:
******************************************************************************************/
#include "Replication/mdbQueueLog.h"
#include "Replication/mdbRepCtrl.h"
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

        if(m_pConfig->GetIsStartShardBackupRep() == true)
        {
			TMdbNtcString m_strName;
			m_strName.Assign(sDsn);
	        m_strName.ToUpper();
	        m_strName.Append("_");
	        m_strName.Append(MDB_SHM_ROUTING_REP_NAME);
			if (!TMdbNtcShareMem::CheckExist(m_strName.c_str(), ENV_QMDB_HOME_NAME))
	        {
            	CHECK_RET(m_pShmDsn->CreateShardBackupRepInfoShm(sDsn),"Can't create shard-backup info share memory.");
	        }
			TMdbShardBuckupCfgCtrl tSBCfgCtrl;
            CHECK_RET(tSBCfgCtrl.Init(sDsn), "m_tSBCfgCtrl init failed.");
			CHECK_RET(tSBCfgCtrl.GetShardBuckupInfo(), "GetShardBuckupInfo failed.");
			CHECK_RET(m_pShmDsn->CreateShardBakBufAreaShm(sDsn), "CreateShardBakBufAreaShm failed.");
			
	        TMdbShmRepMgr * pShmMgr = new (std::nothrow)TMdbShmRepMgr(sDsn);
			CHECK_OBJ(pShmMgr);
			CHECK_RET(pShmMgr->Attach(),"Attach shm failed."); 

			TMdbShmRep * pShmRep = (TMdbShmRep*)pShmMgr->GetRoutingRep();
			CHECK_OBJ(pShmRep);
			
			TMdbOnlineRepMemQueue * pOnlineRepMemQueue;
			for(int i = 0; i<pShmRep->m_iRepHostCount; i++)
			{
				pOnlineRepMemQueue = NULL;
				int iHostID = pShmRep->m_arHosts[i].m_iHostID;
				if(iHostID != MDB_REP_EMPTY_HOST_ID)
				{
					pOnlineRepMemQueue = (TMdbOnlineRepMemQueue *)m_pShmDsn->GetShardBakBufAreaShm(iHostID);
			        CHECK_OBJ(pOnlineRepMemQueue);
			        CHECK_RET(m_tMdbOnlineRepQueueCtrl[iHostID].Init(pOnlineRepMemQueue,m_pShmDsn->GetInfo(), true),"tMdbOnlineRepQueueCtrl init failed.");
				}
				else
				{
					TADD_ERROR(ERR_APP_INVALID_PARAM, "pShmRep->m_arHosts[%d] is empty.", i);
				}
			}
			CHECK_RET(m_mdbRepDispatcher.Init(sDsn, m_tMdbQueueCtrl, m_tMdbOnlineRepQueueCtrl),"Init m_mdbRepDispatcher failed.");
        }

        CHECK_RET(m_mdbDBlog.Init(sDsn, m_tMdbQueueCtrl),"mdbDBlog init failed.");

        if(true == m_pDsn->m_bIsCaptureRouter)
        {
            CHECK_RET(m_tCaptureLog.Init(sDsn, m_tMdbQueueCtrl),"m_tCaptureLog init failed.");
        }

        CHECK_RET(m_mdbRedolog.Init(sDsn, m_tMdbQueueCtrl),"mdbRedolog init failed.");

        TADD_FUNC("Finish");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  Start
    * ��������	:  
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  dong.chun
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
            m_tProcCtrl.UpdateProcHeart(0);//��������ʱ�� 

            iRet = m_tMdbQueueCtrl.Pop();
            switch(iRet)
            {
            case T_EMPTY://��Ϣ���п�
                {
                    CheckBackupFile();
                    TMdbDateTime::MSleep(10);
                    break;
                }           
            case T_SUCCESS://������
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
    * ��������	:  WriteToFile
    * ��������	:  д���ļ�
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  dong.chun
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
        if(pData[0] != 0)//��Ϣ���������ݣ��ж���������
        {
            static int iSyncType = 0;
            iSyncType = GetSyncType(pData);
            TADD_DETAIL("iSyncType=[%0xd]",iSyncType);

            if(FlushTypeHasProperty(iSyncType,FLUSH_ORA))//�����ݿ�ͬ��
            {
                bOraEmpty = false;
            }
            if(FlushTypeHasProperty(iSyncType,FLUSH_SHARD_BACKUP))//��Ƭ����
            {
                bShardEmpty = false;
            }
            if(FlushTypeHasProperty(iSyncType,FLUSH_REDO))//�ع���־
            {
                bRedoEmpty = false;
            }
            if(FlushTypeHasProperty(iSyncType,FLUSH_CAPTURE))//������־
            {
                bCaptureEmpty = false;
            }
        }

        m_mdbDBlog.Log(bOraEmpty);
        m_mdbRedolog.Log(bRedoEmpty);
        if(m_pConfig->GetIsStartShardBackupRep() == true)
        {
            //m_mdbReplog.Log(bShardEmpty);
			m_mdbRepDispatcher.Dispatch(bShardEmpty);
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
            //m_mdbReplog.Log(true);
			m_mdbRepDispatcher.Dispatch(true);
        }        
        if(true == m_pDsn->m_bIsCaptureRouter)
        {
            m_tCaptureLog.Log(true);
        }
        
        TADD_FUNC("Finish");
        return 0;

    }

    /******************************************************************************
    * ��������	:  GetSyncType
    * ��������	:  ��ȡͬ������
    * ����		:  
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  dong.chun
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

