/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        TMdbMgrShm.cpp
*@Description： 管理区的相关数据的控制，做成静态
*@Author:       li.shugang
*@Date：        2008年11月30日
*@History:
******************************************************************************************/
#include "Control/mdbMgrShm.h"
#include "Control/mdbVarcharMgr.h"
#include "Helper/mdbStruct.h"
#include "Control/mdbObserveCtrl.h"
#include "Control/mdbJobCtrl.h"
#include "Control/mdbLinkCtrl.h"
#include "Helper/mdbDateTime.h"
//#include "BillingSDK.h"

//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

    TMdbShmDSN TMdbShmMgr::m_tShmDSN[MAX_DSN_COUNTS];

    TMdbShmDSN::TMdbShmDSN()
    {
        #if 0
        m_pOraShm       = NULL;
        m_pRepShm       = NULL;
        m_pTMdbDSN      = NULL;
        #endif

        m_pShmMgr = NULL;
        m_pMHashConfMgr = NULL;
        m_pMHashLayerMgr = NULL;
        
        for(int i=0; i<MAX_SHM_ID; ++i)
        {
            m_pOtherShmID[i]        = new(std::nothrow) int;
            *m_pOtherShmID[i]       = -1;
            m_pOtherShmAddr[i]      = NULL;
            m_pBaseIndexShmAddr[i]      = NULL;
            m_pConflictIndexShmAddr[i] = NULL;
            m_pMHashBaseIndexShmAddr[i] = NULL;
            
            m_pMHashMutexShmAddr[i]=NULL;
        }

        for(int i = 0; i < MAX_MHASH_SHMID_COUNT; ++i)
        {
            m_pMHashConfIndexShmAddr[i] = NULL;
            m_pMHashLayerIndexShmAddr[i] = NULL;
        }

        for(int i=0; i<MAX_VARCHAR_SHM_ID; ++i)
        {
            m_pVarcharShmID[i]        = new(std::nothrow) int;
            *m_pVarcharShmID[i]       = -1;
            m_pVarcharShmAddr[i] = NULL;
        }
        
        m_bIsAttach = false;
        m_pMdbIndexCtrl = new(std::nothrow)TMdbIndexCtrl();
        m_bTryAttach = false;
        m_arrSyncAreaShm = NULL;

		/*TADD_NORMAL("TMdbShmDSN::TMdbShmDSN clear connect port");
		for(int i=0; i<MAX_AGENT_PORT_COUNTS; i++)
		{
			iNoNtcAgentPorts[i] = -1;
			iConnectNum[i] = 0;
		}
		*/
         
    }

    TMdbShmDSN::~TMdbShmDSN()
    {
        for(int i=0; i<MAX_SHM_ID; ++i)
        {
            SAFE_DELETE(m_pOtherShmID[i]);
        }
        SAFE_DELETE(m_pMdbIndexCtrl);
    }
    //仅仅是尝试attach
    int TMdbShmDSN::TryAttach()
    {
        m_bTryAttach = true;
        return 0;
    }

    int TMdbShmDSN::TryAttachEx(TMdbConfig &config)
    {
        TADD_FUNC("Start.");
        //m_iMgrKey = MANAGER_KEY + 1000000 * config.GetInfo()->llValue;
        m_iMgrKey = GET_MGR_KEY(config.GetDSN()->llValue);
        char *pMgrAddr = NULL;
        return TMdbShm::AttachByKey(m_iMgrKey, pMgrAddr);
    }
    /******************************************************************************
    * 函数名称  :  Attach()
    * 函数描述  :  关联共享内存
    * 输入      :  无
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  li.shugang jiang.mingjun
    *******************************************************************************/
    int TMdbShmDSN::Attach()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(m_bIsAttach == true)
        {
            TADD_FUNC("Finish(m_bIsAttach == true).");
            return iRet;
        }
        //首先要连接管理区的共享内存
        char *pMgrAddr = NULL;
        iRet = TMdbShm::AttachByKey(m_iMgrKey, pMgrAddr);
        if(iRet != 0)
        {
            if(!m_bTryAttach)
            {
                TADD_ERROR(ERROR_UNKNOWN,"Can't attach manager_key=%lld(0x%0x),iRet = %d",m_iMgrKey, m_iMgrKey,iRet);
            }
            m_bTryAttach = false;
            return iRet;
        }
        m_tMgrShmAlloc.AttachByKey(m_iMgrKey,pMgrAddr);//分配区attach
        m_pTMdbDSN = (TMdbDSN*)(pMgrAddr + m_tMgrShmAlloc.m_pShmHead->m_iFirstInfoOffset);
        
        GetMHashConfMgr();
        GetMHashLayerMgr();

		GetTrieConfMgr();
		GetTrieBranchMgr();
        
        //list attach
        CHECK_RET(m_TableList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iUserTableAddr),"tTableList Attach failed.");
        CHECK_RET(m_TSList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iTableSpaceAddr),"m_TSList Attach failed.");
        CHECK_RET(m_ProcList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iProcAddr),"m_ProcList Attach failed.");
        CHECK_RET(m_ObserverList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iObserveAddr),"m_ObserverList Attach failed.");
        CHECK_RET(m_LocalLinkList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iLocalLinkAddr),"m_LocalLinkList Attach failed.");
        CHECK_RET(m_RemoteLinkList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iRemoteLinkAddr),"m_RemoteLinkList Attach failed.");
        CHECK_RET(m_MemSeqList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iSeqAddr),"m_MemSeqList Attach failed.");
        CHECK_RET(m_RepLinkList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iRepLinkAddr),"m_RepLinkList Attach failed.");
        CHECK_RET(m_JobList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iJobAddr),"m_JobList Attach failed.");
        CHECK_RET(m_VarCharList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iVarcharAddr),"m_VarCharList Attach failed.");

		
		CHECK_RET(m_MhashConfList.Attach(m_tMgrShmAlloc, m_pTMdbDSN->iMHashConfAddr),"m_MhashConfList attach failed.");
        CHECK_RET(m_MhashLayerList.Attach(m_tMgrShmAlloc, m_pTMdbDSN->iMHashLayerAddr),"m_MhashLayerList attach failed.");

		CHECK_RET(m_TrieBranchList.Attach(m_tMgrShmAlloc, m_pTMdbDSN->iTrieBranchAddr),"m_TrieBranchList attach failed.");
        CHECK_RET(m_TrieConfList.Attach(m_tMgrShmAlloc, m_pTMdbDSN->iTrieConfAddr),"m_TrieConfList attach failed.");
        
		//cs link
		CHECK_RET(m_PortLinkList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iPortLinkAddr),"m_PortLinkList Attach failed.");
		
		
        TADD_DETAIL("m_iMgrKey=%lld, pMgrAddr=%p.", m_iMgrKey, pMgrAddr);
        TADD_DETAIL("iBaseIndexShmCounts=%d.", m_pTMdbDSN->iBaseIndexShmCounts);
        CHECK_RET(ReAttachIndex(),"ReAttachIndex() failed");
        //链接同步区
        GetSyncAreaShm();
        
        TADD_DETAIL("TMdbShmDSN::Attach() : iShmCounts=[%d].", m_pTMdbDSN->iShmCounts);
        //连接其他共享内存
        for(int i=0; i<m_pTMdbDSN->iShmCounts; ++i)
        {
            *m_pOtherShmID[i] = m_pTMdbDSN->iShmID[i];
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iShmID[i], m_pOtherShmAddr[i]),
                      "Can't attach OtherShm=%d(0x%0x).", m_pTMdbDSN->iShmID[i], m_pTMdbDSN->iShmID[i]);
            TADD_DETAIL("iShmID[%d], ShmAddr=%p.",m_pTMdbDSN->iShmID[i], m_pOtherShmAddr[i]);
        }
        //连接varchar
        for(int i = 0;i<MAX_VARCHAR_SHM_ID;++i)
        {
            if(m_pTMdbDSN->iVarCharShmID[i] != -1)
            {
                *m_pVarcharShmID[i] = m_pTMdbDSN->iVarCharShmID[i];
                CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iVarCharShmID[i], m_pVarcharShmAddr[i]),
                          "Can't attach OtherShm=%d(0x%0x).", m_pTMdbDSN->iVarCharShmID[i], m_pTMdbDSN->iVarCharShmID[i]);
                TADD_DETAIL("iShmID[%d], ShmAddr=%p.",m_pTMdbDSN->iVarCharShmID[i], m_pVarcharShmID[i]);
            }
        }
        m_bIsAttach = true;
        TADD_FUNC("Finish.");
        return 0;
    }


    /******************************************************************************
    * 函数名称  :  Attach()
    * 函数描述  :  关联共享内存
    * 输入      :  pszDSN, DSN名称
    * 输入      :  config, 配置文件类
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  li.shugang
    *******************************************************************************/
    int TMdbShmDSN::Attach(const char * pszDSN, TMdbConfig &config)
    {
        TADD_FUNC("TMdbShmDSN::Attach() : Start.");
        if(m_bIsAttach == true)
        {
            TADD_FUNC("TMdbShmDSN::Attach() : Finish.");
            return 0;
        }
        //m_iMgrKey = MANAGER_KEY + 1000000 * config.GetInfo()->llValue;
        m_iMgrKey = GET_MGR_KEY(config.GetDSN()->llValue);
        return Attach();
    }

    /******************************************************************************
      * 函数名称  :  ReAttachIndex()
      * 函数描述  :  重新关联索引
      * 输入      :  无
      * 输出      :  无
      * 返回值    :  成功返回0，否则返回-1
      * 作者      :  jin.shaohua
      *******************************************************************************/
    int TMdbShmDSN::ReAttachIndex()
    {
        int iRet = 0;
        //连接hash基础索引区
        for(int i=0; i<m_pTMdbDSN->iBaseIndexShmCounts; ++i)
        {
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iBaseIndexShmID[i], m_pBaseIndexShmAddr[i]),
                      "Can't attach BaseIndexShm=%d(0x%0x).",m_pTMdbDSN->iBaseIndexShmID[i], m_pTMdbDSN->iBaseIndexShmID[i]);
            TADD_DETAIL("iBaseIndexShmID[%d]=[%d], ShmAddr=%p.", i, m_pTMdbDSN->iBaseIndexShmID[i], m_pBaseIndexShmAddr[i]);
        }
        //连接hash冲突索引
        for(int i=0; i<m_pTMdbDSN->iConflictIndexShmCounts; ++i)
        {
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iConflictIndexShmID[i], m_pConflictIndexShmAddr[i]),
                      "Can't attach ConflictIndexShm=%d(0x%0x).",m_pTMdbDSN->iConflictIndexShmID[i], m_pTMdbDSN->iConflictIndexShmID[i]);
            TADD_DETAIL("iConflictIndexShmID[%d]=[%d], ShmAddr=%p.", i, m_pTMdbDSN->iConflictIndexShmID[i], m_pConflictIndexShmAddr[i]);
        }
		
		// 连接hash索引锁区
        for(int i=0; i<m_pTMdbDSN->iHashMutexShmCnt; ++i)
        {
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iHashMutexShmID[i], m_pHashMutexShmAddr[i]),
                      "Can't attach M-Hash MutexShm=%d(0x%0x).",m_pTMdbDSN->iMHashMutexShmID[i], m_pTMdbDSN->iMHashMutexShmID[i]);
            TADD_DETAIL("iMHashMutexShmID[%d]=[%d], ShmAddr=%p.", i, m_pTMdbDSN->iMHashMutexShmID[i], m_pMHashMutexShmAddr[i]);
        }

        // 连接阶梯式索引基础索引区
        for(int i=0; i<m_pTMdbDSN->iMHashBaseIdxShmCnt; ++i)
        {
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iMHashBaseIdxShmID[i], m_pMHashBaseIndexShmAddr[i]),
                      "Can't attach M-Hash baseIndexShm=%d(0x%0x).",m_pTMdbDSN->iMHashBaseIdxShmID[i], m_pTMdbDSN->iMHashBaseIdxShmID[i]);
            TADD_DETAIL("iMHashBaseIdxShmID[%d]=[%d], ShmAddr=%p.", i, m_pTMdbDSN->iMHashBaseIdxShmID[i], m_pMHashBaseIndexShmAddr[i]);
        }
        // 连接阶梯式索引冲突索引区
        for(int i=0; i<m_pTMdbDSN->iMHashConfIdxShmCnt; ++i)
        {
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iMHashConfIdxShmID[i], m_pMHashConfIndexShmAddr[i]),
                      "Can't attach M-Hash ConflictIndexShm=%d(0x%0x).",m_pTMdbDSN->iMHashConfIdxShmID[i], m_pTMdbDSN->iMHashConfIdxShmID[i]);
            TADD_DETAIL("iMHashConfIdxShmID[%d]=[%d], ShmAddr=%p.", i, m_pTMdbDSN->iMHashConfIdxShmID[i], m_pMHashConfIndexShmAddr[i]);
        }
        
        // 连接阶梯式索引阶梯索引区
        for(int i=0; i<m_pTMdbDSN->iMHashLayerIdxShmCnt; ++i)
        {
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iMHashLayerIdxShmID[i], m_pMHashLayerIndexShmAddr[i]),
                      "Can't attach M-Hash LayerIndexShm=%d(0x%0x).",m_pTMdbDSN->iMHashLayerIdxShmID[i], m_pTMdbDSN->iMHashLayerIdxShmID[i]);
            TADD_DETAIL("iMHashLayerIdxShmID[%d]=[%d], ShmAddr=%p.", i, m_pTMdbDSN->iMHashLayerIdxShmID[i], m_pMHashLayerIndexShmAddr[i]);
        }

         // 连接阶梯式索引锁区
        for(int i=0; i<m_pTMdbDSN->iMHashMutexShmCnt; ++i)
        {
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iMHashMutexShmID[i], m_pMHashMutexShmAddr[i]),
                      "Can't attach M-Hash MutexShm=%d(0x%0x).",m_pTMdbDSN->iMHashMutexShmID[i], m_pTMdbDSN->iMHashMutexShmID[i]);
            TADD_DETAIL("iMHashMutexShmID[%d]=[%d], ShmAddr=%p.", i, m_pTMdbDSN->iMHashMutexShmID[i], m_pMHashMutexShmAddr[i]);
        }

		// 连接树形索引root节点索引区
        for(int i=0; i<m_pTMdbDSN->iTrieRootIdxShmCnt; ++i)
        {
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iTrieRootIdxShmID[i], m_pTrieRootIndexShmAddr[i]),
                      "Can't attach TrieRootShm=%d(0x%0x).",m_pTMdbDSN->iTrieRootIdxShmID[i], m_pTMdbDSN->iTrieRootIdxShmID[i]);
            TADD_DETAIL("iTrieRootIdxShmID[%d]=[%d], ShmAddr=%p.", i, m_pTMdbDSN->iTrieRootIdxShmID[i], m_pTrieRootIndexShmAddr[i]);
        }
		 
		
		 // 连接树形索引branch节点索引区
        for(int i=0; i<m_pTMdbDSN->iTrieBranchIdxShmCnt; ++i)
        {
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iTrieBranchIdxShmID[i], m_pTrieBranchIndexShmAddr[i]),
                      "Can't attach TrieBranchShm=%d(0x%0x).",m_pTMdbDSN->iTrieBranchIdxShmID[i], m_pTMdbDSN->iTrieBranchIdxShmID[i]);
            TADD_DETAIL("iTrieBranchIdxShmID[%d]=[%d], ShmAddr=%p.", i, m_pTMdbDSN->iTrieBranchIdxShmID[i], m_pTrieBranchIndexShmAddr[i]);
        }

		 // 连接树形索引冲突节点索引区
        for(int i=0; i<m_pTMdbDSN->iTrieConfIdxShmCnt; ++i)
        {
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iTrieConfIdxShmID[i], m_pTrieConfIndexShmAddr[i]),
                      "Can't attach TrieConfShm=%d(0x%0x).",m_pTMdbDSN->iTrieConfIdxShmID[i], m_pTMdbDSN->iTrieConfIdxShmID[i]);
            TADD_DETAIL("iTrieConfIdxShmID[%d]=[%d], ShmAddr=%p.", i, m_pTMdbDSN->iTrieConfIdxShmID[i], m_pTrieConfIndexShmAddr[i]);
        }

        return iRet;
    }

    /******************************************************************************
    * 函数名称  :  Detach()
    * 函数描述  :  取消和共享内存的关联
    * 输入      :  无
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  li.shugang
    *******************************************************************************/
    int TMdbShmDSN::Detach()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //TADD_REST_LOG_LEVEL_AFTER_DETACH;
        if(m_bIsAttach)
        {  
            CHECK_RET(DetachSyncArea()," Can't detach sync area");
            CHECK_RET(DetachMhashMgr(),"Cant't detach mhash mgr shm");
            
            //断开基础索引的管理区
            for(int i=0; i<MAX_SHM_ID; ++i)
            {
                if(m_pBaseIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Detach(m_pBaseIndexShmAddr[i])," Can't detach BaseIndex=%d(0x%0x).",
                              m_pTMdbDSN->iBaseIndexShmID[i], m_pTMdbDSN->iBaseIndexShmID[i]);
                }
                if(m_pConflictIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Detach(m_pConflictIndexShmAddr[i])," Can't detach ConflictIndex=%d(0x%0x).",
                              m_pTMdbDSN->iConflictIndexShmID[i], m_pTMdbDSN->iConflictIndexShmID[i]);
                }
                // m-hash index
                if(m_pMHashBaseIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Detach(m_pMHashBaseIndexShmAddr[i])," Can't detach m-hash BaseIndex=%d(0x%0x).",
                              m_pTMdbDSN->iMHashBaseIdxShmID[i], m_pTMdbDSN->iMHashBaseIdxShmID[i]);
                }

                if(m_pMHashMutexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Detach(m_pMHashMutexShmAddr[i])," Can't detach m-hash BaseIndex=%d(0x%0x).",
                              m_pTMdbDSN->iMHashMutexShmID[i], m_pTMdbDSN->iMHashMutexShmID[i]);
                }

                
                
                if(m_pOtherShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Detach(m_pOtherShmAddr[i])," Can't detach BaseIndex=%d(0x%0x).",
                              m_pTMdbDSN->iShmID[i], m_pTMdbDSN->iShmID[i]);
                }
            }

            for(int i = 0; i < MAX_MHASH_SHMID_COUNT; ++i)
            {
                if(m_pMHashConfIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Detach(m_pMHashConfIndexShmAddr[i])," Can't detach m-hash confIndex=%d(0x%0x).",
                              m_pTMdbDSN->iMHashConfIdxShmID[i], m_pTMdbDSN->iMHashConfIdxShmID[i]);
                }

                if(m_pMHashLayerIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Detach(m_pMHashLayerIndexShmAddr[i])," Can't detach m-hash layer Index=%d(0x%0x).",
                              m_pTMdbDSN->iMHashLayerIdxShmID[i], m_pTMdbDSN->iMHashLayerIdxShmID[i]);
                }

            }

           
            //断开管理区
            CHECK_RET(TMdbShm::Detach((char*)m_pTMdbDSN)," Can't detach RepShm=%d(0x%0x).", m_iMgrKey, m_iMgrKey);
        }
        m_bIsAttach = false;
        return iRet;
    }

    /******************************************************************************
    * 函数名称  :  Detach()
    * 函数描述  :  取消和共享内存的关联
    * 输入      :  无
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  li.shugang
    *******************************************************************************/
    int TMdbShmDSN::Destroy()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //TADD_REST_LOG_LEVEL_AFTER_DETACH;
        if(m_bIsAttach)
        {    
            CHECK_RET(DestroySyncArea()," Can't destroy sync area]");
            CHECK_RET(DestroyMhashMgr()," Can't destroy mhash mgr area]");
			CHECK_RET(DestroyTrieMgr()," Can't destroy trie mgr area]");
			
            CHECK_RET(DestroyShardBackupRepInfoShm()," Can't destroy shardbackup rep info shm");
            
            //断开基础索引的管理区
            for(int i=0; i<MAX_SHM_ID; ++i)
            {
                // hash index
                if(m_pBaseIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iBaseIndexShmID[i])," Can't Destroy BaseIndex=%d(0x%0x).",
                              m_pTMdbDSN->iBaseIndexShmID[i], m_pTMdbDSN->iBaseIndexShmID[i]);
                }
                if(m_pConflictIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iConflictIndexShmID[i])," Can't Destroy ConflictIndex=%d(0x%0x).",
                              m_pTMdbDSN->iConflictIndexShmID[i], m_pTMdbDSN->iConflictIndexShmID[i]);
                }

                // m-hash index
                if(m_pMHashBaseIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iMHashBaseIdxShmID[i])," Can't Destroy m-hash BaseIndex=%d(0x%0x).",
                              m_pTMdbDSN->iMHashBaseIdxShmID[i], m_pTMdbDSN->iMHashBaseIdxShmID[i]);
                }

                if(m_pMHashMutexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iMHashMutexShmID[i])," Can't Destroy m-hash mutex=%d(0x%0x).",
                              m_pTMdbDSN->iMHashMutexShmID[i], m_pTMdbDSN->iMHashMutexShmID[i]);
                }
                
                // other
                if(m_pOtherShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iShmID[i])," Can't Destroy BaseIndex=%d(0x%0x).",
                              m_pTMdbDSN->iShmID[i], m_pTMdbDSN->iShmID[i]);
                }
            }

            for(int i = 0; i < MAX_MHASH_SHMID_COUNT; ++i)
            {
                if(m_pMHashConfIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iMHashConfIdxShmID[i])," Can't Destroy m-hash confIndex=%d(0x%0x).",
                              m_pTMdbDSN->iMHashConfIdxShmID[i], m_pTMdbDSN->iMHashConfIdxShmID[i]);
                }

                if(m_pMHashLayerIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iMHashLayerIdxShmID[i])," Can't Destroy m-hash layer Index=%d(0x%0x).",
                              m_pTMdbDSN->iMHashLayerIdxShmID[i], m_pTMdbDSN->iMHashLayerIdxShmID[i]);
                }

            }

			for(int i = 0; i < MAX_TRIE_SHMID_COUNT; ++i)
            {

				if(m_pTrieRootIndexShmAddr[i] != NULL)
				{
					CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iTrieRootIdxShmID[i])," Can't Destroy TrieRoot ShmID=%d(0x%0x).",
							  m_pTMdbDSN->iTrieRootIdxShmID[i], m_pTMdbDSN->iTrieRootIdxShmID[i]);
				}
			
                if(m_pTrieBranchIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iTrieBranchIdxShmID[i])," Can't Destroy TrieBaseIdx ShmID=%d(0x%0x).",
                              m_pTMdbDSN->iTrieBranchIdxShmID[i], m_pTMdbDSN->iTrieBranchIdxShmID[i]);
                }

                if(m_pTrieConfIndexShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iTrieConfIdxShmID[i])," Can't Destroy TrieConfIdx ShmID=%d(0x%0x).",
                              m_pTMdbDSN->iTrieConfIdxShmID[i], m_pTMdbDSN->iTrieConfIdxShmID[i]);
                }
            }

            for(int i=0; i<MAX_VARCHAR_SHM_ID; ++i)
            {
                if(m_pVarcharShmAddr[i] != NULL)
                {
                    CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iVarCharShmID[i])," Can't Destroy Varchar=%d(0x%0x).",
                              m_pTMdbDSN->iVarCharShmID[i], m_pTMdbDSN->iVarCharShmID[i]);
                }
            }

            //断开管理区
            int iShmID = shmget(m_iMgrKey, 0, 0666);
            if(iShmID < 0)
            {
                TADD_ERROR(ERR_OS_CREATE_SHM,"Can't shmget m_iMgrKey=%d(0x%0x).",m_iMgrKey, m_iMgrKey);
                return ERR_OS_CREATE_SHM;
            }
            CHECK_RET(TMdbShm::Destroy(iShmID),"Can't Destroy MgrShm=%d(0x%0x).",iShmID, iShmID);
            //断开变长存储区
        }
        m_bIsAttach = false;
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称  :  GetInfo()
    * 函数描述  :  获取信息
    * 输入      :  无
    * 输出      :  无
    * 返回值    :  成功返回有效指针，否则返回NULL
    * 作者      :  li.shugang
    *******************************************************************************/
    TMdbDSN* TMdbShmDSN::GetInfo()
    {
        if(m_bIsAttach == true)
            return m_pTMdbDSN;
        else
            return NULL;
    }

    /******************************************************************************
    * 函数名称	:  LockDSN
    * 函数描述	:  锁住管理区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::LockDSN()
    {
        if(NULL == m_pTMdbDSN){return 0;}
        if(m_pTMdbDSN->tMutex.Lock(true,&(m_pTMdbDSN->tCurTime)) != 0)
        {
            return -1;
        }
        return 0;
    }
    //解锁
    int TMdbShmDSN::UnLockDSN()
    {
        if(NULL == m_pTMdbDSN){return 0;}
        if(m_pTMdbDSN->tMutex.UnLock(true) != 0)
        {
            return -1;
        }
        return 0;
    }

    /******************************************************************************
    * 函数名称  :  CreateMgrShm()
    * 函数描述  :  创建管理区的共享内存块
    * 输入      :  config, 配置信息
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::CreateMgrShm(TMdbConfig& config)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbCfgDSN* pDSN = config.GetDSN();
        if(pDSN == NULL)
        {
            TADD_ERROR(ERR_APP_INVALID_PARAM,"Config invalid.");
            return ERR_APP_INVALID_PARAM;
        }
        //为了防止create共享内存多次,再创建前,判断下内存是否已经创建
        m_iMgrKey = GET_MGR_KEY(pDSN->llValue);
        SHAMEM_T iShmID = INITVAl;
        if(TMdbShm::IsShmExist(m_iMgrKey,config.GetDSN()->iManagerSize,iShmID) == true)
        {//共享内存已存在
            CHECK_RET(ERR_OS_SHM_EXIST,"share memory is exist,dsn value=[%d]",pDSN->llValue);
        }
        iShmID = INITVAl;
        //管理区大小从配置项读取
        TADD_FLOW("Create ManagerShm");   
        CHECK_RET(m_tMgrShmAlloc.CreateShm(m_iMgrKey, config.GetDSN()->iManagerSize, iShmID),
                  "Can't create manager share memory, errno=%d, errmsg=[%s].",errno, strerror(errno));
        //链接到共享内存上面
        char *pAddr = NULL;
        CHECK_RET(m_tMgrShmAlloc.AttachByID(iShmID, pAddr)," Can't attach manager share memory, errno=%d[%s].",errno, strerror(errno));
        TADD_FLOW("SHM:ID =[%d].SIZE=[%lu],ADDRESS=[%p],",iShmID,config.GetDSN()->iManagerSize,pAddr);
        TADD_NORMAL("Create Manager Shm ,size=[%luMB].",config.GetDSN()->iManagerSize/(1024*1024));
        //初始化DSN域
        CHECK_RET(InitDSN(config),"InitDSN() failed.");

        CHECK_RET(CreateMhashMgrShm(),"Can't create mhash mgr share memory.");
		CHECK_RET(CreateTrieMgrShm(),"Can't create Trie mgr share memory.");
        
        //m_tMdbVarcharMgr.SetConfig(this);
        //创建基础索引区
        CHECK_RET(m_pMdbIndexCtrl->CreateAllIndex(config),"CreateAllIndex() failed.");
        //创建数据区
        CHECK_RET(CreateDataShm(config)," CreateDataShm() failed.");
        CHECK_RET(CreateSyncAreaShm(),"Can't create Rep share memory.");
        
        // 创建分片备份同步区
        if(config.GetIsStartShardBackupRep())
        {
            CHECK_RET(CreateShardBackupRepInfoShm(pDSN->sName),"Can't create shard-backup share memory.");
            //CHECK_RET(CreateSyncAreaShm(SA_SHARD_BACKUP),"Can't create shard-backup share memory.");
        }       
                
        CHECK_RET(Attach(),"Attach failed.");
        //观测点
        TObserveMgr tObMgr;
        tObMgr.InitObservePoint(m_ObserverList);//观测点初始化
        //job初始化
        TMdbJobCtrl tJobCtrl;
        tJobCtrl.LoadJobs(m_pTMdbDSN->sName);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  AttachvarCharBlockShm
    * 函数描述	:  attach 变长存储区共享内存
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::AttachvarCharBlockShm(int iPos, char **pAddr)
    {
        return 0;
    }

    /******************************************************************************
    * 函数名称	:  CreateDataShm
    * 函数描述	:  创建数据区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::CreateDataShm(TMdbConfig &config)
    {
        TADD_FUNC("Start.");
        config.GetDbDataShmCounts(m_pTMdbDSN->iShmCounts);
        int iRet = 0;
        for(int i=0; i<m_pTMdbDSN->iShmCounts; ++i)
        {
            TADD_FLOW("Create DataShm:[%d]",i);
            //根据要求创建共享内存
            //数据区大小由配置项配置
            CHECK_RET(TMdbShm::Create(m_pTMdbDSN->iShmKey[i], config.GetDSN()->iDataSize, m_pTMdbDSN->iShmID[i]),
                      "Can't create manager share memory, errno=%d[%s].",errno, strerror(errno));
            //链接到共享内存上面
            *m_pOtherShmID[i] = m_pTMdbDSN->iShmID[i];
            CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iShmID[i], m_pOtherShmAddr[i]),
                      "Can't attach manager share memory, errno=%d[%s].", errno, strerror(errno));
            TADD_FLOW("SHM:ID =[%d].SIZE=[%lu].ADDRESS=[%p]",m_pTMdbDSN->iShmID[i],config.GetDSN()->iDataSize,m_pOtherShmAddr[i]);
            TADD_NORMAL("Create DataShm:[%d],size=[%luMB]",i,config.GetDSN()->iDataSize/(1024*1024));
            //初始化头信息
            TMdbShmHead *pHead = (TMdbShmHead*)m_pOtherShmAddr[i];
            pHead->iShmID = m_pTMdbDSN->iShmID[i];
            pHead->iTotalSize = config.GetDSN()->iDataSize;
            pHead->iLeftOffSet = sizeof(TMdbShmHead);
            pHead->tMutex.Create();
            TADD_DETAIL("[%p].[%p].", this, m_pOtherShmAddr[i]);
        }
        TADD_FUNC(" Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称  :  CreateNewDataShm()
    * 函数描述  :  从系统中创建一个新的共享内存块
    * 输入      :  无
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  li.shugang
    *******************************************************************************/
    int TMdbShmDSN::CreateNewDataShm(TMdbConfig &config)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_RET(m_pTMdbDSN->tMutex.Lock(true,&m_pTMdbDSN->tCurTime), "Lock failed.");
        do
        {
            //根据要求创建共享内存
            int iCounts = m_pTMdbDSN->iShmCounts;
            CHECK_RET_BREAK(TMdbShm::Create(m_pTMdbDSN->iShmKey[iCounts], config.GetDSN()->iDataSize, m_pTMdbDSN->iShmID[iCounts]),
                      "Can't create manager share memory, errno=%d[%s].",errno, strerror(errno));
            CHECK_RET_BREAK(TMdbShm::AttachByID(m_pTMdbDSN->iShmID[iCounts], m_pOtherShmAddr[iCounts]),
                      "Can't attach manager share memory, errno=%d[%s].",errno, strerror(errno));
             //链接到共享内存上面
            *m_pOtherShmID[iCounts] = m_pTMdbDSN->iShmID[iCounts];
            //初始化头信息
            TMdbShmHead *pHead = (TMdbShmHead*)m_pOtherShmAddr[iCounts];
            pHead->iShmID = m_pTMdbDSN->iShmID[iCounts];
            pHead->iTotalSize = config.GetDSN()->iDataSize;
            pHead->iLeftOffSet = sizeof(TMdbShmHead);
            pHead->tMutex.Create();
            //内存块数增长1
            ++m_pTMdbDSN->iShmCounts;
        }while(0);
        CHECK_RET(m_pTMdbDSN->tMutex.UnLock(true),"UnLock failed.");
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  InitDsnAddrOffset
    * 函数描述	:  初始化地址偏移
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int  TMdbShmDSN::InitDsnAddr(TMdbConfig &config)
    {
        int iRet = 0;
        //共享页锁
        m_tMgrShmAlloc.Allocate(MAX_MUTEX_COUNTS * sizeof(TMutex) + INTERVAL_SIZE,m_pTMdbDSN->iPageMutexAddr);
        TMutex* pMutex = (TMutex*) GetPageMutexAddr();
        CHECK_OBJ(pMutex);
        int i = 0;
        for( i=0; i<MAX_MUTEX_COUNTS; ++i)
        {
            pMutex->Create();
            ++pMutex;
        }

		m_tMgrShmAlloc.Allocate(MAX_VARCHAR_MUTEX_COUNTS * sizeof(TMutex) + INTERVAL_SIZE, m_pTMdbDSN->iVarcharPageMutexAddr);
		pMutex = (TMutex*) GetVarcharPageMutexAddr();
		CHECK_OBJ(pMutex);
		for( i=0; i<MAX_VARCHAR_MUTEX_COUNTS; i++)
		{
			pMutex->Create();
			pMutex++;
		}
		
		        
        //设置各个共享块的key
        for(i=0; i<MAX_SHM_ID; ++i)
        {
            //数据块

            m_pTMdbDSN->iShmKey[i] = m_iMgrKey + 37*i + 1;
            m_pTMdbDSN->iShmID[i]  = INITVAl;


            //基础索引
            m_pTMdbDSN->iBaseIndexShmKey[i] = m_iMgrKey + 37*i + 2;
            m_pTMdbDSN->iBaseIndexShmID[i]  = INITVAl;

            //冲突索引
            m_pTMdbDSN->iConflictIndexShmKey[i] = m_iMgrKey + 37*i + 3;
            m_pTMdbDSN->iConflictIndexShmID[i]  = INITVAl;

            // 阶梯式索引
            m_pTMdbDSN->iMHashBaseIdxShmKey[i] = m_iMgrKey + 37*i + 4;
            m_pTMdbDSN->iMHashBaseIdxShmID[i] = INITVAl;


            m_pTMdbDSN->iMHashMutexShmKey[i] = m_iMgrKey + 37*i + 5;
            m_pTMdbDSN->iMHashMutexShmID[i] = INITVAl;
            
        }  

        m_pTMdbDSN->iMhashConfMgrShmId = INITVAl;

        m_pTMdbDSN->iMhashConfMgrShmKey = m_iMgrKey+6;

        m_pTMdbDSN->iMhashLayerMgrShmId = INITVAl;

        m_pTMdbDSN->iMhashLayerMgrShmKey = m_iMgrKey+7;

        for(i = 0; i < MAX_MHASH_SHMID_COUNT; ++i)
        {

            m_pTMdbDSN->iMHashConfIdxShmKey[i] = m_iMgrKey + 37*i + 8;
            m_pTMdbDSN->iMHashConfIdxShmID[i] = INITVAl;


            m_pTMdbDSN->iMHashLayerIdxShmKey[i] = m_iMgrKey + 37*i + 9;
            m_pTMdbDSN->iMHashLayerIdxShmID[i] = INITVAl;
        }
		
		/***********************for trie begin*************************/
		m_pTMdbDSN->iTrieBranchMgrShmId = INITVAl;
        m_pTMdbDSN->iTrieBranchMgrShmKey = m_iMgrKey+10;

        m_pTMdbDSN->iTrieConfMgrShmId = INITVAl;
        m_pTMdbDSN->iTrieConfMgrShmKey = m_iMgrKey+11;


        for(i = 0; i < MAX_TRIE_SHMID_COUNT; ++i)
        {
            m_pTMdbDSN->iTrieRootIdxShmKey[i] = m_iMgrKey + 37*i + 12;
            m_pTMdbDSN->iTrieRootIdxShmID[i] = INITVAl;
			
            m_pTMdbDSN->iTrieBranchIdxShmKey[i] = m_iMgrKey + 37*i + 13;
            m_pTMdbDSN->iTrieBranchIdxShmID[i] = INITVAl;

            m_pTMdbDSN->iTrieConfIdxShmKey[i] = m_iMgrKey + 37*i + 14;
            m_pTMdbDSN->iTrieConfIdxShmID[i] = INITVAl;		
        }

		/***********************for trie end*************************/
		
        
        //初始化各同步区信息
        const char * sSyncAreaName[] = {"rep"};
        TMdbSyncArea & tSA = m_pTMdbDSN->m_arrSyncArea;
        SAFESTRCPY(tSA.m_sName,sizeof(tSA.m_sName),sSyncAreaName[0]);
        tSA.m_iSAType = 0;
        tSA.m_iShmID = INITVAl;
        tSA.m_iShmKey = m_iMgrKey + 9;
        
        m_pTMdbDSN->tMutex.Create();            //管理区共享锁        
        m_pTMdbDSN->m_SessionMutex.Create();            //事务ID共享锁
        
        
        //list 首次attach
        CHECK_RET(m_TableList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iUserTableAddr),"tTableList Attach failed.");
        CHECK_RET(m_TSList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iTableSpaceAddr),"m_TSList Attach failed.");
        CHECK_RET(m_ProcList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iProcAddr),"m_ProcList Attach failed.");
        CHECK_RET(m_ObserverList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iObserveAddr),"m_ObserverList Attach failed.");
        CHECK_RET(m_LocalLinkList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iLocalLinkAddr),"m_LocalLinkList Attach failed.");
        CHECK_RET(m_RemoteLinkList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iRemoteLinkAddr),"m_RemoteLinkList Attach failed.");
        CHECK_RET(m_MemSeqList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iSeqAddr),"m_MemSeqList Attach failed.");
        CHECK_RET(m_RepLinkList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iRepLinkAddr),"m_RepLinkList Attach failed.");
        CHECK_RET(m_JobList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iJobAddr),"m_JobList Attach failed.");
        CHECK_RET(m_VarCharList.Attach(m_tMgrShmAlloc,m_pTMdbDSN->iVarcharAddr),"m_VarCharList Attach failed.");
        CHECK_RET(m_MhashConfList.Attach(m_tMgrShmAlloc, m_pTMdbDSN->iMHashConfAddr),"m_MhashConfList attach failed.");
        CHECK_RET(m_MhashLayerList.Attach(m_tMgrShmAlloc, m_pTMdbDSN->iMHashLayerAddr),"m_MhashConfList attach failed.");
        return iRet;
    }



    /******************************************************************************
    * 函数名称	:  InitDSN
    * 函数描述	:  初始化DSN
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::InitDSN(TMdbConfig &config)
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        m_pTMdbDSN =  (TMdbDSN * ) m_tMgrShmAlloc.Allocate(sizeof(TMdbDSN),m_tMgrShmAlloc.m_pShmHead->m_iFirstInfoOffset);//dsn域信息体
        m_pTMdbDSN->Clear();
        CHECK_RET(InitDsnAddr(config),"InitDsnAddr failed.");//初始化地址偏移

        //设置DSN信息
        SAFESTRCPY(m_pTMdbDSN->sVersion,sizeof(m_pTMdbDSN->sVersion),MDB_VERSION);
        SAFESTRCPY(m_pTMdbDSN->sName,sizeof(m_pTMdbDSN->sName),config.GetDSN()->sName);
        m_pTMdbDSN->llDsnValue = config.GetDSN()->llValue;
        m_pTMdbDSN->cState = DB_unused;
        m_pTMdbDSN->iRepAttr = REP_FROM_DB;
        m_pTMdbDSN->iRoutingID = -1;
        m_pTMdbDSN->iTableCounts = config.GetTableCounts();
        SetDsnRepAttr(config);//设置同步属性
        m_pTMdbDSN->iMutexCounts = MAX_MUTEX_COUNTS;
        TMdbDateTime::GetCurrentTimeStr(m_pTMdbDSN->sCreateTime);
        TMdbDateTime::GetCurrentTimeStr(m_pTMdbDSN->sCurTime);
        m_pTMdbDSN->m_arrSyncArea.m_iShmSize = config.GetDSN()->iRepBuffSize;
        m_pTMdbDSN->m_arrSyncArea.SetFileAndSize(config.GetDSN()->sRepFilePath,config.GetDSN()->iRepFileSize,SA_REP);
        m_pTMdbDSN->m_arrSyncArea.SetFileAndSize(config.GetDSN()->sLogDir,config.GetDSN()->iLogFileSize,SA_ORACLE);
        m_pTMdbDSN->m_arrSyncArea.SetFileAndSize(config.GetDSN()->sRedoDir,config.GetDSN()->iRedoFileSize,SA_REDO); 
        m_pTMdbDSN->m_arrSyncArea.SetFileAndSize(config.GetDSN()->sCaptureDir,config.GetDSN()->iCaptureFileSize,SA_CAPTURE);
        //MDB文件存储位置
        SAFESTRCPY(m_pTMdbDSN->sStorageDir,sizeof(m_pTMdbDSN->sStorageDir),config.GetDSN()->sStorageDir);
        
        SAFESTRCPY(m_pTMdbDSN->sDataStore,sizeof(m_pTMdbDSN->sDataStore),config.GetDSN()->sDataStore);
        m_pTMdbDSN->iPermSize   = config.GetDSN()->iPermSize;     //只有在表信息不确定的情况下，才需要设定
        SAFESTRCPY(m_pTMdbDSN->sOracleID,sizeof(m_pTMdbDSN->sOracleID),config.GetDSN()->sOracleID);
        SAFESTRCPY(m_pTMdbDSN->sOracleUID,sizeof(m_pTMdbDSN->sOracleUID),config.GetDSN()->sOracleUID);
        SAFESTRCPY(m_pTMdbDSN->sOraclePWD,sizeof(m_pTMdbDSN->sOraclePWD),config.GetDSN()->sOraclePWD);
        SAFESTRCPY(m_pTMdbDSN->sLocalIP,sizeof(m_pTMdbDSN->sLocalIP),config.GetDSN()->sLocalIP);
        SAFESTRCPY(m_pTMdbDSN->sPeerIP,sizeof(m_pTMdbDSN->sPeerIP),config.GetDSN()->sPeerIP);

        m_pTMdbDSN->iLocalPort = config.GetDSN()->iLocalPort; //对应的LocalPort
        m_pTMdbDSN->iPeerPort  = config.GetDSN()->iPeerPort;  //对应的iPeerPort

        for(int i = 0; i<MAX_AGENT_PORT_COUNTS; i++)
		{
			m_pTMdbDSN->iAgentPort[i] = config.GetDSN()->iAgentPort[i];
			m_pTMdbDSN->iNtcPort[i] = config.GetDSN()->iNtcPort[i];
			m_pTMdbDSN->iNoNtcPort[i] = config.GetDSN()->iNoNtcPort[i];
			
		}
        m_pTMdbDSN->m_iDataSize = config.GetDSN()->iDataSize;

        m_pTMdbDSN->iLogLevel = config.GetDSN()->iLogLevel; //日志级别
        //设置主备同步、Oracle同步开关
        m_pTMdbDSN->m_iOraRepCounts = config.GetDSN()->iOraRepCounts;
        m_pTMdbDSN->m_bIsOraRep = config.GetDSN()->bIsOraRep;
        m_pTMdbDSN->m_bIsRep = config.GetDSN()->bIsRep;
        m_pTMdbDSN->m_bIsCaptureRouter = config.GetDSN()->bIsCaptureRouter;
        m_pTMdbDSN->m_bIsDiskStorage = config.GetDSN()->m_bIsDiskStorage;
        //当前机器时差
        m_pTMdbDSN->m_iTimeDifferent = TMdbDateTime::GetTimeDifferent();
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  GetSyncAreaShm
    * 函数描述	:  获取同步区共享内存
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    char * TMdbShmDSN::GetSyncAreaShm()
    {
        TMdbSyncArea & tSA = m_pTMdbDSN->m_arrSyncArea;
        if(NULL == m_arrSyncAreaShm && tSA.m_iShmID >0)
        {
            if(TMdbShm::AttachByID(tSA.m_iShmID, m_arrSyncAreaShm) != 0)
            {
                  TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach [%s]ShmID=%d(0x%x).",tSA.m_sName,tSA.m_iShmID,tSA.m_iShmID);
                  return NULL;
            }
        }
        return m_arrSyncAreaShm;
    }


    /******************************************************************************
    * 函数名称	:  GetTableByName
    * 函数描述	:  根据表名获取表
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbTable * TMdbShmDSN::GetTableByName(const char * pTableName)
    {
        TShmList<TMdbTable>::iterator itor = m_TableList.begin();
        for(;itor != m_TableList.end();++itor)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pTableName,itor->sTableName) == 0)
            {
                return &(*itor);
            }
        }
        return NULL;
    }
    

    /******************************************************************************
    * 函数名称	:  GetObPiontById
    * 函数描述	:  获取观测点信息
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TObservePoint * TMdbShmDSN::GetObPiontById(int iObId)
    {
        TShmList<TObservePoint>::iterator itor = m_ObserverList.begin();
        for(;itor != m_ObserverList.end();++itor)
        {
            if(itor->m_iType == iObId)
            {
                return &(*itor);
            }
        }
        return NULL;
    }
    /******************************************************************************
    * 函数名称	:  GetObPiontByName
    * 函数描述	:  获取观测点信息
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TObservePoint * TMdbShmDSN::GetObPiontByName(const char * sName)
    {
        TShmList<TObservePoint>::iterator itor = m_ObserverList.begin();
        for(;itor != m_ObserverList.end();++itor)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(sName,itor->m_sName) == 0)
            {
                return &(*itor);
            }
        }
        return NULL;
    }



    /******************************************************************************
    * 函数名称	:  GetProcByPid
    * 函数描述	:  根据进程ID，获取进程信息
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbProc * TMdbShmDSN::GetProcByPid(int iPid)
    {
        TShmList<TMdbProc>::iterator itor = m_ProcList.begin();
        for(;itor != m_ProcList.end();++itor)
        {
            if(iPid == itor->iPid)
            {
                return &(*itor);
            }
        }
        return NULL;
    }

    /******************************************************************************
    * 函数名称	:  GetTableSpaceAddrByName
    * 函数描述	:  根据名称获取表空间
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  cao.peng
    *******************************************************************************/
    TMdbTableSpace * TMdbShmDSN::GetTableSpaceAddrByName(const char * pTablespaceName)
    {
        TShmList<TMdbTableSpace>::iterator itor = m_TSList.begin();
        for(;itor != m_TSList.end();++itor)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(pTablespaceName,itor->sName) == 0)
            {
                return &(*itor);
            }
        }
        return NULL;
    }
    /******************************************************************************
    * 函数名称	:  GetVarchar
    * 函数描述	:  获取varchar段
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  dong.chun
    *******************************************************************************/
    TMdbVarchar* TMdbShmDSN::GetVarchar(int iWhichPos)
    {
        TShmList<TMdbVarchar>::iterator itor = m_VarCharList.begin();
        for(;itor != m_VarCharList.end();++itor)
        {
            if(itor->iVarcharID == iWhichPos)
            {
                return &(*itor);
            }
        }
        return NULL;
    }

    /******************************************************************************
    * 函数名称	:  GetDataShmAddr
    * 函数描述	:根据shmid获取数据地址
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    char* TMdbShmDSN::GetDataShmAddr(SHAMEM_T iShmID)
    {
        for(int i=0; i<MAX_SHM_ID; ++i)
        {
            if(iShmID == *m_pOtherShmID[i])
            {
                return m_pOtherShmAddr[i];
            }
        }

        for(int i=0; i<MAX_SHM_ID; ++i)
        {
            if(-1 == *m_pOtherShmID[i])
            {
                int iRet = TMdbShm::AttachByID(iShmID, m_pOtherShmAddr[i]);
                if(iRet != 0)
                {
                    TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach iShmID=%d(0x%x).", iShmID, iShmID);
                    return NULL;
                }
                *m_pOtherShmID[i] = iShmID;
                return m_pOtherShmAddr[i];
            }
        }
        return NULL;
    }

    char* TMdbShmDSN::GetVarcharShmAddr(SHAMEM_T iShmID)
    {
        for(int i=0; i<MAX_VARCHAR_SHM_ID; ++i)
        {
            if(iShmID == *m_pVarcharShmID[i])
            {
                return m_pVarcharShmAddr[i];
            }
        }

        for(int i=0; i<MAX_VARCHAR_SHM_ID; ++i)
        {
            if(-1 == *m_pVarcharShmID[i])
            {
                int iRet = TMdbShm::AttachByID(iShmID, m_pVarcharShmAddr[i]);
                if(iRet != 0)
                {
                    TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach iShmID=%d(0x%x).", iShmID, iShmID);
                    return NULL;
                }
                *m_pVarcharShmID[i] = iShmID;
                return m_pVarcharShmAddr[i];
            }
        }
        return NULL;
    }

    char* TMdbShmDSN::GetMhashConfShmAddr(SHAMEM_T iShmID)
    {
        for(int i = 0; i < MAX_MHASH_SHMID_COUNT; ++i)
        {
            if(iShmID == m_pTMdbDSN->iMHashConfIdxShmID[i] && m_pMHashConfIndexShmAddr[i])
            {
                return m_pMHashConfIndexShmAddr[i];
            }
        }

        for(int i=0; i<MAX_MHASH_SHMID_COUNT; ++i)
        {
            if(-1 == m_pTMdbDSN->iMHashConfIdxShmID[i] || !m_pMHashConfIndexShmAddr[i])
            {
                int iRet = TMdbShm::AttachByID(iShmID, m_pMHashConfIndexShmAddr[i]);
                if(iRet != 0)
                {
                    TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach iShmID=%d(0x%x).", iShmID, iShmID);
                    return NULL;
                }
                m_pTMdbDSN->iMHashConfIdxShmID[i] = iShmID;
                return m_pMHashConfIndexShmAddr[i];
            }
        }
        return NULL;
    }

    char* TMdbShmDSN::GetMhashLayerShmAddr(SHAMEM_T iShmID)
    {
        for(int i = 0; i < MAX_MHASH_SHMID_COUNT; ++i)
        {
            if(iShmID == m_pTMdbDSN->iMHashLayerIdxShmID[i] && m_pMHashLayerIndexShmAddr[i])
            {
                return m_pMHashLayerIndexShmAddr[i];
            }
        }

        for(int i=0; i<MAX_MHASH_SHMID_COUNT; ++i)
        {
            if(-1 == m_pTMdbDSN->iMHashLayerIdxShmID[i] || !m_pMHashLayerIndexShmAddr[i])
            {
                int iRet = TMdbShm::AttachByID(iShmID, m_pMHashLayerIndexShmAddr[i]);
                if(iRet != 0)
                {
                    TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach iShmID=%d(0x%x).", iShmID, iShmID);
                    return NULL;
                }
                m_pTMdbDSN->iMHashLayerIdxShmID[i] = iShmID;
                return m_pMHashLayerIndexShmAddr[i];
            }
        }
        return NULL;
    }


	char* TMdbShmDSN::GetTrieBranchShmAddr(SHAMEM_T iShmID)
	   {
		   for(int i = 0; i < MAX_TRIE_SHMID_COUNT; ++i)
		   {
			   if(iShmID == m_pTMdbDSN->iTrieBranchIdxShmID[i] && m_pTrieBranchIndexShmAddr[i])
			   {
				   return m_pTrieBranchIndexShmAddr[i];
			   }
		   }
	
		   for(int i=0; i<MAX_TRIE_SHMID_COUNT; ++i)
		   {
			   if(-1 == m_pTMdbDSN->iTrieBranchIdxShmID[i] || !m_pTrieBranchIndexShmAddr[i])
			   {
				   int iRet = TMdbShm::AttachByID(iShmID, m_pTrieBranchIndexShmAddr[i]);
				   if(iRet != 0)
				   {
					   TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach iShmID=%d(0x%x).", iShmID,iShmID);
					   return NULL;
				   }
				   m_pTMdbDSN->iTrieBranchIdxShmID[i] = iShmID;
				   return m_pTrieBranchIndexShmAddr[i];
			   }
		   }
		   return NULL;
	   }
	
	   char* TMdbShmDSN::GetTrieConfShmAddr(SHAMEM_T iShmID)
	   {
		   for(int i = 0; i < MAX_TRIE_SHMID_COUNT; ++i)
		   {
			   if(iShmID == m_pTMdbDSN->iTrieConfIdxShmID[i] && m_pTrieConfIndexShmAddr[i])
			   {
				   return m_pTrieConfIndexShmAddr[i];
			   }
		   }
	
		   for(int i=0; i<MAX_TRIE_SHMID_COUNT; ++i)
		   {
			   if(-1 == m_pTMdbDSN->iTrieConfIdxShmID[i] || !m_pTrieConfIndexShmAddr[i])
			   {
				   int iRet = TMdbShm::AttachByID(iShmID, m_pTrieConfIndexShmAddr[i]);
				   if(iRet != 0)
				   {
					   TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach iShmID=%d(0x%x).", iShmID, iShmID);
					   return NULL;
				   }
				   m_pTMdbDSN->iTrieConfIdxShmID[i] = iShmID;
				   return m_pTrieConfIndexShmAddr[i];
			   }
		   }
		   return NULL;
	   }

	

    /******************************************************************************
    * 函数名称	:  GetDataShm
    * 函数描述	:获取数据内存区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    char* TMdbShmDSN::GetDataShm(int iPos)
    {
        if(iPos>=0 && iPos< m_pTMdbDSN->iShmCounts)
        {
            if(NULL == m_pOtherShmAddr[iPos])
            {//需要重新attach
                TMdbShm::AttachByID(m_pTMdbDSN->iShmID[iPos], m_pOtherShmAddr[iPos]);
            }
            return  m_pOtherShmAddr[iPos];
        }
        else
            return NULL;
    }

    /******************************************************************************
    * 函数名称	:  GetBaseIndex
    * 函数描述	:获取基础索引区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    char* TMdbShmDSN::GetBaseIndex(int iPos)
    {
        if(iPos>=0 && iPos<MAX_SHM_ID)
            return  m_pBaseIndexShmAddr[iPos];
        else
            return NULL;
    }


    /******************************************************************************
    * 函数名称	:  GetConflictIndex
    * 函数描述	:获取冲突索引区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    char* TMdbShmDSN::GetConflictIndex(int iPos)
    {
        if(iPos>=0 && iPos<MAX_SHM_ID)
            return  m_pConflictIndexShmAddr[iPos];
        else
            return NULL;
    }

	char* TMdbShmDSN::GetHashMutex(int iPos)
    {
        if(iPos>=0 && iPos<MAX_SHM_ID)
            return  m_pHashMutexShmAddr[iPos];
        else
            return NULL;
    }

    char* TMdbShmDSN::GetMHashBaseIndex(int iPos)
    {
        if(iPos>=0 && iPos<MAX_SHM_ID)
            return  m_pMHashBaseIndexShmAddr[iPos];
        else
            return NULL;
    }

    char* TMdbShmDSN::GetMHashMutex(int iPos)
    {
        if(iPos>=0 && iPos<MAX_SHM_ID)
            return  m_pMHashMutexShmAddr[iPos];
        else
            return NULL;
    }

    TMdbMHashConflictIndexMgrInfo* TMdbShmDSN::GetMHashConfMgr()
    {
        if(NULL == m_pMHashConfMgr&& m_pTMdbDSN->iMhashConfMgrShmId>0)
        {
            if(TMdbShm::AttachByID(m_pTMdbDSN->iMhashConfMgrShmId, m_pMHashConfMgr) != 0)
            {
                  TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach mhash conf ShmID=%d(0x%x).",m_pTMdbDSN->iMhashConfMgrShmId,m_pTMdbDSN->iMhashConfMgrShmId);
                  return NULL;
            }
        }
        return (TMdbMHashConflictIndexMgrInfo*)m_pMHashConfMgr;
    }

    TMdbMHashLayerIndexMgrInfo* TMdbShmDSN::GetMHashLayerMgr()
    {
        if(NULL == m_pMHashLayerMgr&& m_pTMdbDSN->iMhashLayerMgrShmId>0)
        {
            if(TMdbShm::AttachByID(m_pTMdbDSN->iMhashLayerMgrShmId, m_pMHashLayerMgr) != 0)
            {
                  TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach mhash layer ShmID=%d(0x%x).",m_pTMdbDSN->iMhashLayerMgrShmId,m_pTMdbDSN->iMhashLayerMgrShmId);
                  return NULL;
            }
        }
        return (TMdbMHashLayerIndexMgrInfo*)m_pMHashLayerMgr;
    }

	char* TMdbShmDSN::GetTrieRootIndex(int iPos)
    {
        if(iPos>=0 && iPos<MAX_TRIE_SHMID_COUNT)
            return  m_pTrieRootIndexShmAddr[iPos];
        else
            return NULL;
    }

	
	TMdbTrieBranchIndexMgrInfo* TMdbShmDSN::GetTrieBranchMgr()
	{
		if(NULL == m_pTrieBranchMgr&& m_pTMdbDSN->iTrieBranchMgrShmId>0)
		{
			if(TMdbShm::AttachByID(m_pTMdbDSN->iTrieBranchMgrShmId, m_pTrieBranchMgr) != 0)
			{
				  TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach GetTrieBranchMgr ShmID=%d(0x%x).",m_pTMdbDSN->iMhashLayerMgrShmId,m_pTMdbDSN->iMhashLayerMgrShmId);
				  return NULL;
			}
		}
		return (TMdbTrieBranchIndexMgrInfo*)m_pTrieBranchMgr;
	}

	
	TMdbTrieConflictIndexMgrInfo* TMdbShmDSN::GetTrieConfMgr()
    {
        if(NULL == m_pTrieConfMgr&& m_pTMdbDSN->iTrieConfMgrShmId >0)
        {
            if(TMdbShm::AttachByID(m_pTMdbDSN->iTrieConfMgrShmId, m_pTrieConfMgr) != 0)
            {
                  TADD_ERROR(ERR_OS_ATTACH_SHM,"Can't attach GetTrieConfMgr ShmID=%d(0x%x).",m_pTMdbDSN->iMhashConfMgrShmId,m_pTMdbDSN->iMhashConfMgrShmId);
                  return NULL;
            }
        }
        return (TMdbTrieConflictIndexMgrInfo*)m_pTrieConfMgr;
    }
  
    /******************************************************************************
    * 函数名称	:  GetVarcharMgrAddr
    * 函数描述	: 获取varchar 管理区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    char * TMdbShmDSN::GetVarcharMgrAddr()
    {
         if(NULL == m_pTMdbDSN)
        {
            return NULL;
        }
        else
        {
            return GetAddrByOffset(m_pTMdbDSN->iVarcharAddr);
        }    
    }

    /******************************************************************************
    * 函数名称	:  SetDsnRepAttr
    * 函数描述	:  设置DSN同步属性
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::SetDsnRepAttr(TMdbConfig &config)
    {
        int iCount = 0;
        m_pTMdbDSN->iRepAttr = DSN_No_Rep;
          //设置数据库的同步属性：0-从Ora同步;1-向Ora同步;2-向备机同步;3-向Ora和备机同步
        for(int i=0; i<MAX_TABLE_COUNTS; ++i)
        {
            TMdbTable* pTable = config.GetTableByPos(i);
            if(pTable == NULL)
            {
                continue;
            }
            iCount++;

            if(pTable->iRepAttr == REP_TO_DB)
            {
                m_pTMdbDSN->iRepAttr |= DSN_Rep;
            }
            else if(pTable->m_bShardBack == true)
            {
                m_pTMdbDSN->iRepAttr |= DSN_Ora_Rep;
            }
            else if(pTable->m_bShardBack == true && (pTable->iRepAttr == REP_TO_DB || pTable->iRepAttr == REP_FROM_DB))
            {
                m_pTMdbDSN->iRepAttr |= DSN_Two_Rep;
            }
            
            if(iCount == config.GetTableCounts())
            {
                break;
            }
        }
        if(false == config.GetDSN()->bIsOraRep){m_pTMdbDSN->iRepAttr &=(0^(DSN_Ora_Rep));}
        if(false == config.GetDSN()->bIsRep){m_pTMdbDSN->iRepAttr &=(0^(DSN_Rep));}
        return 0;
    }
    /******************************************************************************
    * 函数名称	:  GetPageMutexAddr
    * 函数描述	: 获取页锁地址
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
    char * TMdbShmDSN::GetPageMutexAddr()
    {
        if(NULL == m_pTMdbDSN)
        {
            return NULL;
        }
        else
        {
            return GetAddrByOffset(m_pTMdbDSN->iPageMutexAddr);
        }  
    }

	/******************************************************************************
	* 函数名称	:  GetVarcharPageMutexAddr
	* 函数描述	: 获取varchar页锁地址
	* 输入		:  
	* 输入		:  
	* 输出		:  
	* 返回值	:  NULL - 失败 !NULL -成功
	* 作者		:  jiang.xiaolong
	*******************************************************************************/

	char * TMdbShmDSN::GetVarcharPageMutexAddr()
	{
		if(NULL == m_pTMdbDSN)
		{
			return NULL;
		}
		else
		{
			return GetAddrByOffset(m_pTMdbDSN->iVarcharPageMutexAddr);
		}
	}

    /******************************************************************************
    * 函数名称	: CreateSyncAreaShm 
    * 函数描述	:  创建同步区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::CreateSyncAreaShm()
    {
       TADD_FUNC("Start.");
        int iRet = 0;
        TMdbSyncArea & tSA = m_pTMdbDSN->m_arrSyncArea;
        TADD_FLOW("Create [%s]:",tSA.m_sName);
        //主备同步区已经存在，不需要重新创建
        if(TMdbShm::IsShmExist(tSA.m_iShmKey,tSA.m_iShmSize,tSA.m_iShmID))
        {//共享内存已存在
            TADD_NORMAL("Shared memory is exist,[%s]ShmID =[%d]",tSA.m_sName,tSA.m_iShmID);
            return iRet;
        }
        CHECK_RET(TMdbShm::Create(tSA.m_iShmKey,tSA.m_iShmSize*1024*1024,tSA.m_iShmID),
                  "Can't create the  area shared memory, errno=%d[%s].",errno, strerror(errno));
        //链接到共享内存上面
        TMdbMemQueue* pQueue = (TMdbMemQueue*)GetSyncAreaShm();
        CHECK_OBJ(pQueue);
        SAFESTRCPY(pQueue->sFlag,sizeof(pQueue->sFlag),"###@@@");
        pQueue->tPushMutex.Create();
        pQueue->tPopMutex.Create();
        pQueue->iPushPos = 1024;
        pQueue->iPopPos  = 1024;
        pQueue->iStartPos = 1024;
        pQueue->iEndPos   = tSA.m_iShmSize * 1024 * 1024;
        pQueue->iTailPos  = tSA.m_iShmSize * 1024 * 1024;
        TADD_DETAIL("CreateMgrShm: iPushPos=%d, iPopPos=%d, iStartPos=%d, iEndPos=%d.",
                    pQueue->iPushPos, pQueue->iPopPos, pQueue->iStartPos, pQueue->iEndPos);
        TADD_NORMAL("Create [%s] Shm ,size=[%dMB].",tSA.m_sName,tSA.m_iShmSize);
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbShmDSN::CreateMhashMgrShm()
    {
       TADD_FUNC("Start.");
        int iRet = 0;
        if(TMdbShm::IsShmExist(m_pTMdbDSN->iMhashConfMgrShmKey,sizeof(TMdbMHashConflictIndexMgrInfo),m_pTMdbDSN->iMhashConfMgrShmId))
        {//共享内存已存在
            TADD_NORMAL("Shared memory is exist,mhash conf ShmID =[%d]",m_pTMdbDSN->iMhashConfMgrShmId);
            return iRet;
        }
        CHECK_RET(TMdbShm::Create(m_pTMdbDSN->iMhashConfMgrShmKey,sizeof(TMdbMHashConflictIndexMgrInfo),m_pTMdbDSN->iMhashConfMgrShmId),
                  "Can't create the  area shared memory, errno=%d[%s].",errno, strerror(errno));
        //链接到共享内存上面
        TMdbMHashConflictIndexMgrInfo* pConfMgr = GetMHashConfMgr();
        CHECK_OBJ(pConfMgr);
        pConfMgr->Clear();

        if(TMdbShm::IsShmExist(m_pTMdbDSN->iMhashLayerMgrShmKey,sizeof(TMdbMHashLayerIndexMgrInfo),m_pTMdbDSN->iMhashLayerMgrShmId))
        {//共享内存已存在
            TADD_NORMAL("Shared memory is exist,mhash layer ShmID =[%d]",m_pTMdbDSN->iMhashLayerMgrShmId);
            return iRet;
        }
        CHECK_RET(TMdbShm::Create(m_pTMdbDSN->iMhashLayerMgrShmKey,sizeof(TMdbMHashLayerIndexMgrInfo),m_pTMdbDSN->iMhashLayerMgrShmId),
                  "Can't create the  area shared memory, errno=%d[%s].",errno, strerror(errno));
        //链接到共享内存上面
        TMdbMHashLayerIndexMgrInfo* pLayerMgr = GetMHashLayerMgr();
        CHECK_OBJ(pLayerMgr);
        pLayerMgr->Clear();

        
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbShmDSN::DetachMhashMgr()
    {   
        TADD_FUNC("Start.");
        int iRet = 0;
        if(m_pTMdbDSN->iMhashConfMgrShmId> 0)
        {
            CHECK_RET(TMdbShm::Detach(m_pMHashConfMgr)," Can't detach mahsh conf Shm=%d(0x%0x).",m_pTMdbDSN->iMhashConfMgrShmId,m_pTMdbDSN->iMhashConfMgrShmId);
        }

         if(m_pTMdbDSN->iMhashLayerMgrShmId> 0)
        {
            CHECK_RET(TMdbShm::Detach(m_pMHashLayerMgr)," Can't detach mhash layer Shm=%d(0x%0x).",m_pTMdbDSN->iMhashLayerMgrShmId,m_pTMdbDSN->iMhashLayerMgrShmId);
        }
         
        TADD_FUNC("Finish.");
        return iRet;
    } 

    int TMdbShmDSN::DestroyMhashMgr()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(m_pTMdbDSN->iMhashConfMgrShmId> 0)
        {
            CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iMhashConfMgrShmId)," Can't Destroy mahsh conf Shm=%d(0x%0x).",m_pTMdbDSN->iMhashConfMgrShmId,m_pTMdbDSN->iMhashConfMgrShmId);
        }

         if(m_pTMdbDSN->iMhashLayerMgrShmId> 0)
        {
            CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iMhashLayerMgrShmId)," Can't Destroy mhash layer Shm=%d(0x%0x).",m_pTMdbDSN->iMhashLayerMgrShmId,m_pTMdbDSN->iMhashLayerMgrShmId);
        }
         
        TADD_FUNC("Finish.");
        return iRet;
    }



	int TMdbShmDSN::CreateTrieMgrShm()
    {
       TADD_FUNC("Start.");
        int iRet = 0;
        if(TMdbShm::IsShmExist(m_pTMdbDSN->iTrieConfMgrShmKey,sizeof(TMdbTrieConflictIndexMgrInfo),m_pTMdbDSN->iTrieConfMgrShmId))
        {//共享内存已存在
            TADD_NORMAL("Shared memory is exist,TrieConf ShmID =[%d]",m_pTMdbDSN->iMhashConfMgrShmId);
            return iRet;
        }
        CHECK_RET(TMdbShm::Create(m_pTMdbDSN->iTrieConfMgrShmKey,sizeof(TMdbTrieConflictIndexMgrInfo),m_pTMdbDSN->iTrieConfMgrShmId),
                  "Can't create the  area shared memory, errno=%d[%s].",errno, strerror(errno));
        //链接到共享内存上面
        TMdbTrieConflictIndexMgrInfo* pConfMgr = GetTrieConfMgr();
        CHECK_OBJ(pConfMgr);
        pConfMgr->Clear();

        if(TMdbShm::IsShmExist(m_pTMdbDSN->iTrieBranchMgrShmKey,sizeof(TMdbTrieRootIndexMgrInfo),m_pTMdbDSN->iTrieBranchMgrShmId))
        {//共享内存已存在
            TADD_NORMAL("Shared memory is exist,TrieBranch ShmID =[%d]",m_pTMdbDSN->iMhashLayerMgrShmId);
            return iRet;
        }
        CHECK_RET(TMdbShm::Create(m_pTMdbDSN->iTrieBranchMgrShmKey,sizeof(TMdbTrieRootIndexMgrInfo),m_pTMdbDSN->iTrieBranchMgrShmId),
                  "Can't create the  area shared memory, errno=%d[%s].",errno, strerror(errno));
        //链接到共享内存上面
        TMdbTrieBranchIndexMgrInfo* pBranchMgr = GetTrieBranchMgr();
        CHECK_OBJ(pBranchMgr);
        pBranchMgr->Clear();

        
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbShmDSN::DetachTrieMgr()
    {   
        TADD_FUNC("Start.");
        int iRet = 0;
        if(m_pTMdbDSN->iTrieConfMgrShmId> 0)
        {
            CHECK_RET(TMdbShm::Detach(m_pTrieConfMgr)," Can't detach trie conf Shm=%d(0x%0x).",m_pTMdbDSN->iTrieConfMgrShmId,m_pTMdbDSN->iTrieConfMgrShmId);
        }

         if(m_pTMdbDSN->iTrieBranchMgrShmId> 0)
        {
            CHECK_RET(TMdbShm::Detach(m_pTrieBranchMgr)," Can't detach trie branch Shm=%d(0x%0x).",m_pTMdbDSN->iTrieBranchMgrShmId,m_pTMdbDSN->iTrieBranchMgrShmId);
        }
         
        TADD_FUNC("Finish.");
        return iRet;
    } 

	int TMdbShmDSN::DestroyTrieMgr()
    {   
        TADD_FUNC("Start.");
        int iRet = 0;
        if(m_pTMdbDSN->iTrieConfMgrShmId> 0)
        {
            CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iTrieConfMgrShmId)," Can't Destroy trie conf Shm=%d(0x%0x).",m_pTMdbDSN->iTrieConfMgrShmId,m_pTMdbDSN->iTrieConfMgrShmId);
        }

         if(m_pTMdbDSN->iTrieBranchMgrShmId> 0)
        {
            CHECK_RET(TMdbShm::Destroy(m_pTMdbDSN->iTrieBranchMgrShmId)," Can't Destroy trie branch Shm=%d(0x%0x).",m_pTMdbDSN->iTrieBranchMgrShmId,m_pTMdbDSN->iTrieBranchMgrShmId);
        }
         
        TADD_FUNC("Finish.");
        return iRet;
    } 

	
    /******************************************************************************
    * 函数名称	:  DetachSyncArea
    * 函数描述	:  断链同步区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::DetachSyncArea()
    {   
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbSyncArea & tSA = m_pTMdbDSN->m_arrSyncArea;
        if(tSA.m_iShmID > 0)
        {
            CHECK_RET(TMdbShm::Detach(m_arrSyncAreaShm)," Can't detach RepShm=%d(0x%0x).",tSA.m_iShmID,tSA.m_iShmID);
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	: DestroySyncArea 
    * 函数描述	:  销毁同步区
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::DestroySyncArea()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbSyncArea & tSA = m_pTMdbDSN->m_arrSyncArea;
        if(tSA.m_iShmID > 0)
        {
            CHECK_RET(TMdbShm::Destroy(tSA.m_iShmID)," Can't Destroy RepShm=%d(0x%0x).",tSA.m_iShmID ,tSA.m_iShmID );
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbShmDSN::CreateShardBackupRepInfoShm(const char * pszDSN)
    {
         TADD_FUNC("Start.");
         int iRet = 0;
         m_pShmMgr = new (std::nothrow)TMdbShmRepMgr(pszDSN);
         CHECK_OBJ(m_pShmMgr);
         CHECK_RET(m_pShmMgr->Create(),"Create shm failed.");
         return iRet;
    }

    int TMdbShmDSN::DestroyShardBackupRepInfoShm()
    {
        TADD_FUNC("Start.");
        int iRet = 0;

        m_pShmMgr = new (std::nothrow)TMdbShmRepMgr(m_pTMdbDSN->sName);
        CHECK_OBJ(m_pShmMgr);
        CHECK_RET(m_pShmMgr->Destroy(),"Destroy shm failed.");
        
        return iRet;
    }

    /******************************************************************************
    * 函数名称	: AddNewTableSpace
    * 函数描述	: 添加一个新的表空间
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  NULL - 失败 !NULL -成功
    * 作者		:  jin.shaohua
    *******************************************************************************/
     int TMdbShmDSN::AddNewTableSpace(TMdbTableSpace * & pNewTableSpace)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        pNewTableSpace = NULL;
        int iCount = 0;
        TShmList<TMdbTableSpace >::iterator itor=  m_TSList.begin();
        //先搜寻空闲的
        for(;itor != m_TSList.end();++itor)
        {
           if(itor->iEmptyPageID < 0)
           {//free
               pNewTableSpace = &(*itor);
               pNewTableSpace->Clear();
               break;
           }
           iCount++;
        }
        if(iCount >= MAX_TABLE_COUNTS){CHECK_RET(ERR_TAB_TABLE_NUM_EXCEED_MAX,"Max tablespace counts=[%d]",MAX_TABLE_COUNTS);}
        if(NULL == pNewTableSpace)
        {
            TShmList<TMdbTableSpace >::iterator itorNew =  m_TSList.insert(m_TSList.end());
            if(itorNew != m_TSList.end())
            {//分配成功
                pNewTableSpace = &(*itorNew);
                pNewTableSpace->Clear();
            }
            else
            {//分配失败
                CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for tablespace");
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  AddNewTable
    * 函数描述	:  新增一个表
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::AddNewTable(TMdbTable* & pNewTable,TMdbTable * pSrcTable)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TShmList<TMdbTable >::iterator itor = m_TableList.begin();
        pNewTable = NULL;
        int iCount = 0;
        //先搜寻空闲的
        for(;itor != m_TableList.end();++itor)
        {
           if(0 == itor->sTableName[0])
           {//free
               pNewTable = &(*itor);
               CHECK_RET(pNewTable->Init(pSrcTable),"pNewTable->Init failed.");
               pNewTable->ResetRecordCounts();//重置记录数
               break;
           }
           iCount++;
        }
        if(iCount >= MAX_TABLE_COUNTS){CHECK_RET(ERR_TAB_TABLE_NUM_EXCEED_MAX,"Max table counts=[%d]",MAX_TABLE_COUNTS);}
        if(NULL == pNewTable)
        {
            TShmList<TMdbTable >::iterator itorNew =  m_TableList.insert(m_TableList.end());
            if(itorNew != m_TableList.end())
            {//分配成功
                pNewTable = &(*itorNew);
                CHECK_RET(pNewTable->Init(pSrcTable),"pNewTable->Init failed.");
                pNewTable->ResetRecordCounts();//重置记录数
            }
            else
            {//分配失败
                CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for tablespace");
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  AddNewProc
    * 函数描述	:  新增一个进程信息
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::AddNewProc(TMdbProc * & pNewProc)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TShmList<TMdbProc >::iterator itor = m_ProcList.begin();
        pNewProc = NULL;
        int iCount = 0;
        //先搜寻空闲的
        for(;itor != m_ProcList.end();++itor)
        {
            if(0 == itor->sName[0] && itor->iPid < 0)
            {//free
                pNewProc = &(*itor);
                pNewProc->Clear();
                break;
            }
            iCount ++;
        }
        if(iCount >= MAX_PROCESS_COUNTS){CHECK_RET(ERR_OS_NO_MEMROY,"Max Process is =[%d]",MAX_PROCESS_COUNTS);}
        if(NULL == pNewProc)
        {//申请新节点
            TShmList<TMdbProc >::iterator itorNew =  m_ProcList.insert(m_ProcList.end());
            if(itorNew != m_ProcList.end())
            {//分配成功
                pNewProc = &(*itorNew);
                pNewProc->Clear();
            }
            else
            {//分配失败
                CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for Proc");
            } 
        }
        TADD_FUNC("Finish.");
        return iRet; 
    }

    /******************************************************************************
    * 函数名称	:  AddNewLocalLink
    * 函数描述	:  新增一个链接
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::AddNewLocalLink(TMdbLocalLink *& pNewLink)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TShmList<TMdbLocalLink>::iterator itor = m_LocalLinkList.begin();
        pNewLink = NULL;
        int iCount = 0;
        //先搜寻空闲的
        for(;itor != m_LocalLinkList.end();++itor)
        {
            if(itor->iPID < 0 || itor->cState == Link_down)
            {//free
                pNewLink = &(*itor);
                break;
            }
            iCount ++;
        }
        if(iCount >= MAX_LINK_COUNTS){CHECK_RET(ERR_OS_NO_MEMROY,"Max LocalLink is =[%d]",MAX_LINK_COUNTS);}
        if(NULL == pNewLink)
        {//申请新节点
            TShmList<TMdbLocalLink >::iterator itorNew =  m_LocalLinkList.insert(m_LocalLinkList.end());
            if(itorNew != m_LocalLinkList.end())
            {//分配成功
                pNewLink = &(*itorNew);
                pNewLink->Clear();
            }
            else
            {//分配失败
                CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for LocalLink");
            } 
        }
        TADD_FUNC("Finish.");
        return iRet; 
    }
    /******************************************************************************
    * 函数名称	:  AddNewRemoteLink
    * 函数描述	:  新增一个链接
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::AddNewRemoteLink(TMdbRemoteLink *& pNewLink)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TShmList<TMdbRemoteLink>::iterator itor = m_RemoteLinkList.begin();
        pNewLink = NULL;
        int iCount = 0;
        //先搜寻空闲的
        for(;itor != m_RemoteLinkList.end();++itor)
        {
            if(0 == itor->sIP[0])
            {//free
                pNewLink = &(*itor);
                pNewLink->Clear();
                break;
            }
            iCount ++;
        }
         if(iCount >= MAX_LINK_COUNTS){CHECK_RET(ERR_OS_NO_MEMROY,"Max RemoteLink is =[%d]",MAX_LINK_COUNTS);}
        if(NULL == pNewLink)
        {//申请新节点
            TShmList<TMdbRemoteLink >::iterator itorNew =  m_RemoteLinkList.insert(m_RemoteLinkList.end());
            if(itorNew != m_RemoteLinkList.end())
            {//分配成功
                pNewLink = &(*itorNew);
                pNewLink->Clear();
            }
            else
            {//分配失败
                CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for RemoteLink");
            } 
        }
        TADD_FUNC("Finish.");
        return iRet; 
    }

	int TMdbShmDSN::AddNewPortLink(TMdbPortLink *& pNewLink)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TShmList<TMdbPortLink>::iterator itor = m_PortLinkList.begin();
        pNewLink = NULL;
        
        //先搜寻空闲的
        for(;itor != m_PortLinkList.end();++itor)
        {
            if(-1 == itor->iAgentPort)
            {//free
                pNewLink = &(*itor);
                pNewLink->Clear();
                break;
            }
         
        }
        
        if(NULL == pNewLink)
        {//申请新节点
            TShmList<TMdbPortLink >::iterator itorNew =  m_PortLinkList.insert(m_PortLinkList.end());
            if(itorNew != m_PortLinkList.end())
            {//分配成功
                pNewLink = &(*itorNew);
                pNewLink->Clear();
				//TADD_NORMAL("new a portlink ok to agent");
            }
            else
            {//分配失败
                CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for RemoteLink");
            } 
        }
        TADD_FUNC("Finish.");
        return iRet; 
    }
	
    /******************************************************************************
    * 函数名称	:  AddNewRepLink
    * 函数描述	:  新增一个链接
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::AddNewRepLink(TMdbRepLink *& pNewLink)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TShmList<TMdbRepLink>::iterator itor = m_RepLinkList.begin();
        pNewLink = NULL;
        int iCount = 0;
        //先搜寻空闲的
        for(;itor != m_RepLinkList.end();++itor)
        {
            if(itor->iSocketID < 0)
            {//free
                pNewLink = &(*itor);
                pNewLink->Clear();
                break;
            }
            iCount ++;
        }
        if(iCount >= MAX_LINK_COUNTS){CHECK_RET(ERR_OS_NO_MEMROY,"Max RepLink  is =[%d]",MAX_LINK_COUNTS);}
        if(NULL == pNewLink)
        {//申请新节点
            TShmList<TMdbRepLink >::iterator itorNew =  m_RepLinkList.insert(m_RepLinkList.end());
            if(itorNew != m_RepLinkList.end())
            {//分配成功
                pNewLink = &(*itorNew);
                pNewLink->Clear();
            }
            else
            {//分配失败
                CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for RepLink");
            } 
        }
        TADD_FUNC("Finish.");
        return iRet; 
    }

    /******************************************************************************
    * 函数名称	:  AddNewMemSeq
    * 函数描述	:  新增一个memseq
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::AddNewMemSeq(TMemSeq*& pNewMemSeq)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TShmList<TMemSeq>::iterator itor = m_MemSeqList.begin();
        pNewMemSeq = NULL;
        int iCount = 0;
        //先搜寻空闲的
        for(;itor != m_MemSeqList.end();++itor)
        {
            if(0 == itor->sSeqName[0])
            {//free
                pNewMemSeq = &(*itor);
                pNewMemSeq->Init();
                break;
            }
            iCount ++;
        }
        if(iCount >= MAX_SEQUENCE_COUNTS){CHECK_RET(ERR_OS_NO_MEMROY,"Max MemSeq  is =[%d]",MAX_SEQUENCE_COUNTS);}
        if(NULL == pNewMemSeq)
        {//申请新节点
            TShmList<TMemSeq >::iterator itorNew =  m_MemSeqList.insert(m_MemSeqList.end());
            if(itorNew != m_MemSeqList.end())
            {//分配成功
                pNewMemSeq = &(*itorNew);
                pNewMemSeq->Init();
            }
            else
            {//分配失败
                CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for MemSeq");
            } 
        }
        TADD_FUNC("Finish.");
        return iRet; 
    }

    /******************************************************************************
    * 函数名称	:  AddNewJob
    * 函数描述	:  新增一个新job
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbShmDSN::AddNewJob(TMdbJob *& pNewJob)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        TShmList<TMdbJob>::iterator itor = m_JobList.begin();
        pNewJob = NULL;
        int iCount = 0;
        //先搜寻空闲的
        for(;itor != m_JobList.end();++itor)
        {
            if(false == itor->IsValid())
            {//free
                pNewJob = &(*itor);
                pNewJob->Clear();
                break;
            }
            iCount ++;
        }
        if(iCount >= MAX_JOB_COUNT){CHECK_RET(ERR_OS_NO_MEMROY,"Max Job  is =[%d]",MAX_JOB_COUNT);}
        if(NULL == pNewJob)
        {//申请新节点
            TShmList<TMdbJob >::iterator itorNew =  m_JobList.insert(m_JobList.end());
            if(itorNew != m_JobList.end())
            {//分配成功
                pNewJob = &(*itorNew);
                pNewJob->Clear();
            }
            else
            {//分配失败
                CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for Job");
            } 
        }
        TADD_FUNC("Finish.");
        return iRet; 

    }
    /******************************************************************************
    * 函数名称	:  AddNewVarChar
    * 函数描述	:  增加一个varchar段
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  dong.chun
    *******************************************************************************/
    int TMdbShmDSN::AddNewVarChar(TMdbVarchar * & pNewVarChar)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        pNewVarChar = NULL;
        TShmList<TMdbVarchar>::iterator itorNew =  m_VarCharList.insert(m_VarCharList.end());
        if(itorNew != m_VarCharList.end())
        {//分配成功
            pNewVarChar = &(*itorNew);
            pNewVarChar->Clear();
        }
        else
        {//分配失败
            CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for Varchar");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbShmDSN::AddNewMhashConflict(TMdbMhashBlock *&pNewBlock)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        pNewBlock = NULL;
        TShmList<TMdbMhashBlock>::iterator itorNew =  m_MhashConfList.insert(m_MhashConfList.end());
        if(itorNew != m_MhashConfList.end())
        {//分配成功
            pNewBlock = &(*itorNew);
            pNewBlock->Clear();
        }
        else
        {//分配失败
            CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for Mhash conflict");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    int TMdbShmDSN::AddNewMhashLayer(TMdbMhashBlock *&pNewBlock)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        pNewBlock = NULL;
        TShmList<TMdbMhashBlock>::iterator itorNew =  m_MhashLayerList.insert(m_MhashLayerList.end());
        if(itorNew != m_MhashLayerList.end())
        {//分配成功
            pNewBlock = &(*itorNew);
            pNewBlock->Clear();
        }
        else
        {//分配失败
            CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for Mhash layer");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbShmDSN::AddNewTrieBranch(TMdbTrieBlock *&pNewBlock)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        pNewBlock = NULL;
        TShmList<TMdbTrieBlock>::iterator itorNew =  m_TrieBranchList.insert(m_TrieBranchList.end());
        if(itorNew != m_TrieBranchList.end())
        {//分配成功
            pNewBlock = &(*itorNew);
            pNewBlock->Clear();
        }
        else
        {//分配失败
            CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for trie branch");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

	int TMdbShmDSN::AddNewTrieConflict(TMdbTrieBlock *&pNewBlock)
    {
        int iRet = 0;
        TADD_FUNC("Start.");
        pNewBlock = NULL;
        TShmList<TMdbTrieBlock>::iterator itorNew =  m_TrieConfList.insert(m_TrieConfList.end());
        if(itorNew != m_TrieConfList.end())
        {//分配成功
            pNewBlock = &(*itorNew);
            pNewBlock->Clear();
        }
        else
        {//分配失败
            CHECK_RET(ERR_OS_NO_MEMROY,"no mem space for trie conflict");
        }
        TADD_FUNC("Finish.");
        return iRet;
    }
	
    /******************************************************************************
    * 函数名称	:  GetJobByName
    * 函数描述	:  根据名字获取job
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbJob * TMdbShmDSN::GetJobByName(const char * sName)
    {
        if(NULL == sName || 0 == sName[0]){return NULL;}
        TShmList<TMdbJob>::iterator itor = m_JobList.begin();
        for(;itor != m_JobList.end();++itor)
        {
            if(itor->IsValid() && TMdbNtcStrFunc::StrNoCaseCmp(sName,itor->m_sName) == 0)
            {//find
                return &(*itor);
            }
        }
        return NULL;
    }

    /******************************************************************************
    * 函数名称	:  GetMemSeqByName
    * 函数描述	:  根据seqname获取
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMemSeq * TMdbShmDSN::GetMemSeqByName(const char * sSeqName)
    {
        TShmList<TMemSeq>::iterator itor = m_MemSeqList.begin();
        for(;itor != m_MemSeqList.end();++itor)
        {
            if(TMdbNtcStrFunc::StrNoCaseCmp(itor->sSeqName,sSeqName) == 0)
            {
                return &(*itor);
            }
        }
        return NULL;
    }
    /******************************************************************************
    * 函数名称	:  GetMdbProcByName
    * 函数描述	:  根据名字获取进程
    * 输入		:  
    * 输入		:  
    * 输出		:  
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    TMdbProc * TMdbShmDSN::GetProcByName(const char * sName)
    {
        TShmList<TMdbProc>::iterator itor = m_ProcList.begin();
        for(;itor != m_ProcList.end();++itor)
        {
            
            if(TMdbNtcStrFunc::StrNoCaseCmp(itor->sName,sName) == 0)
            {
                return &(*itor);
            }
        }
        return NULL;
    }

    int TMdbShmDSN::InitVarchar(int iPos, int iShmId, char *& pVarcharAddr)
    {
        int iRet = 0;
        *m_pVarcharShmID[iPos] = iShmId;
         CHECK_RET(TMdbShm::AttachByID(*m_pVarcharShmID[iPos], m_pVarcharShmAddr[iPos]),
                  "Can't attach varchar[%d], errno=%d[%s].", iPos,errno, strerror(errno));
         pVarcharAddr = m_pVarcharShmAddr[iPos];
         return iRet;
    }
    
    int TMdbShmDSN::InitMHashConfBlock(int iPos, char*& pAddr)
    {
        int iRet = 0;
        CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iMHashConfIdxShmID[iPos], m_pMHashConfIndexShmAddr[iPos]),
                  "Can't attach mhash conflict blocks[%d], errno=%d[%s].", iPos,errno, strerror(errno));
        pAddr = m_pMHashConfIndexShmAddr[iPos];
        return iRet;
    }

    int TMdbShmDSN::InitMHashLayerBlock(int iPos, char*& pAddr)
    {
        int iRet = 0;
        CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iMHashLayerIdxShmID[iPos], m_pMHashLayerIndexShmAddr[iPos]),
                  "Can't attach mhash layer blocks[%d], errno=%d[%s].", iPos,errno, strerror(errno));
        pAddr = m_pMHashLayerIndexShmAddr[iPos];
        return iRet;
    }

	int TMdbShmDSN::InitTrieConfBlock(int iPos, char*& pAddr)
    {
        int iRet = 0;
        CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iTrieConfIdxShmID[iPos], m_pTrieConfIndexShmAddr[iPos]),
                  "Can't attach trie conflict blocks[%d], errno=%d[%s].", iPos,errno, strerror(errno));
        pAddr = m_pTrieConfIndexShmAddr[iPos];
        return iRet;
    }

	int TMdbShmDSN::InitTrieBranchBlock(int iPos, char*& pAddr)
    {
        int iRet = 0;
        CHECK_RET(TMdbShm::AttachByID(m_pTMdbDSN->iTrieBranchIdxShmID[iPos], m_pTrieBranchIndexShmAddr[iPos]),
                  "Can't attach trie Branch blocks[%d], errno=%d[%s].", iPos,errno, strerror(errno));
        pAddr = m_pTrieBranchIndexShmAddr[iPos];
        return iRet;
    }

	TMdbTrieBlock* TMdbShmDSN::GetTrieConfBlockById(int iBlockID)
    {
        TShmList<TMdbTrieBlock>::iterator itor = m_TrieConfList.begin();
        for(;itor != m_TrieConfList.end();++itor)
        {
            if(itor->iBlockId == iBlockID)
            {
                return &(*itor);
            }
        }
        return NULL;
    }


	//?
	TMdbTrieBlock* TMdbShmDSN::GetTrieBranchBlockById(int iBlockID)
    {
        TShmList<TMdbTrieBlock>::iterator itor = m_TrieBranchList.begin();
        for(;itor != m_TrieBranchList.end();++itor)
        {
            if(itor->iBlockId == iBlockID)
            {
                return &(*itor);
            }
        }
        return NULL;
    }
		

    TMdbMhashBlock* TMdbShmDSN::GetMhashConfBlockById(int iBlockID)
    {
        TShmList<TMdbMhashBlock>::iterator itor = m_MhashConfList.begin();
        for(;itor != m_MhashConfList.end();++itor)
        {
            if(itor->iBlockId == iBlockID)
            {
                return &(*itor);
            }
        }
        return NULL;
    }

    TMdbMhashBlock* TMdbShmDSN::GetMhashLayerBlockById(int iBlockID)
    {
        TShmList<TMdbMhashBlock>::iterator itor = m_MhashLayerList.begin();
        for(;itor != m_MhashLayerList.end();++itor)
        {
            if(itor->iBlockId == iBlockID)
            {
                return &(*itor);
            }
        }
        return NULL;
    }

    /******************************************************************************
    * 函数名称  :  GetShmDSN()
    * 函数描述  :  创建管理区的共享内存块
    * 输入      :  config, 配置信息
    * 输出      :  无
    * 返回值    :  成功返回指针，否则返回NULL
    * 作者      :  li.shugang
    *******************************************************************************/
    TMdbShmDSN* TMdbShmMgr::GetShmDSN(const char* pszDSN,bool bSkipErrorLog)
    {
        TADD_FUNC("(%s) : Start.", pszDSN);

        int iFreePos = -1;
        for(int i=0; i<MAX_DSN_COUNTS; ++i)
        {
            TMdbDSN* pDSN = m_tShmDSN[i].GetInfo();
            if(pDSN == NULL)
            {
                if(iFreePos == -1)
                    iFreePos = i;
                continue;
            }
            TADD_DETAIL("(%s) : sName=[%s], pszDSN=[%s].", pszDSN, pDSN->sName, pszDSN);
            if(TMdbNtcStrFunc::StrNoCaseCmp(pDSN->sName, pszDSN) == 0)
            {
                return &m_tShmDSN[i];
            }
        }
        if(iFreePos == -1)
        {
            TADD_ERROR(ERR_DB_MAX_DSN_NUMBER,"(%s) : Can't find Free-DSN.",pszDSN);
            return NULL;
        }
        TADD_FLOW("(%s) : Not find DSN, Attach it.", pszDSN);
        TMdbConfig *pConfig = new(std::nothrow) TMdbConfig();
        TMdbShmDSN* pRetShmDsn = NULL;
        do
        {
            int iRet = 0;
            if(NULL == pConfig)
            {
                CHECK_RET_BREAK(ERR_OS_NO_MEMROY,"(%s) : Mem Not Enough.",  pszDSN);
            }
            iRet = 0;
            //检测配置文件
            CHECK_RET_BREAK(pConfig->Init()," Can't find Config-file(%s).", pszDSN);
            //上载系统参数：DSN、表(列+索引)、表空间
            CHECK_RET_BREAK(pConfig->LoadDsnConfigFile(pszDSN,false),"(%s) : Can't load Config-file, maybe sys-config is error.",  pszDSN);;
            //连接上共享内存
            if(bSkipErrorLog){m_tShmDSN[iFreePos].TryAttach();}//
            if(m_tShmDSN[iFreePos].Attach(pszDSN, *pConfig) != 0)
            {
                if(!bSkipErrorLog){TADD_ERROR(ERR_OS_ATTACH_SHM,"Attach [%s] failed.Maybe mdb is not create",  pszDSN);}
                break;
            }
            pRetShmDsn =  &m_tShmDSN[iFreePos];
            //0 == iRet?0:0;////只是为了规避CPPCHECK的检测
        }while(0);
        SAFE_DELETE(pConfig);
        TADD_FUNC("(%s) : Finish(%p).", pszDSN,pRetShmDsn);
        return pRetShmDsn;
    }


    /******************************************************************************
    * 函数名称  :  CreateMgrShm()
    * 函数描述  :  创建管理区的共享内存块
    * 输入      :  config, 配置信息
    * 输出      :  无
    * 返回值    :  成功返回0，否则返回-1
    * 作者      :  li.shugang
    *******************************************************************************/
    int TMdbShmMgr::CreateMgrShm(TMdbConfig &config)
    {
        for(int i=0; i<MAX_DSN_COUNTS; ++i)
        {
            TMdbDSN* pDSN = m_tShmDSN[i].GetInfo();

            if(pDSN == NULL)
            {
                return m_tShmDSN[i].CreateMgrShm(config);
            }
        }
        return ERROR_UNKNOWN;
    }

//}

