/****************************************************************************************
*@Copyrights  2009，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：	    TMdbImage.cpp
*@Description： 内存数据库的镜像控制
*@Author:		li.shugang
*@Modify:       jiang.mingjun
*@Date：	    2009年04月24日
*@History:      定义协议接口文件,把内存数据库刷成指定格式文件.
******************************************************************************************/
#include "Tools/mdbImage.h"
#include "Tools/mdbMutexCtrl.h"

//namespace QuickMDB{

    TMdbImage::TMdbImage()
    {
        m_pConfig  = NULL;
        m_pShmDSN  = NULL;
        for(int i = 0;i < MAX_SHM_ID; i++)
        {
            m_iOldDataShmID[i] = 0;
        }
        
    }


    TMdbImage::~TMdbImage()
    {
    }

    //调整表空间与ShmID的映射关系
    int TMdbImage::AdjustTableSpace(TMdbDSN * pDsn)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iOldShmID = 0;
        int iShmID = 0;
        CHECK_OBJ(pDsn);
        m_pShmDSN = TMdbShmMgr::GetShmDSN(pDsn->sName);
        CHECK_OBJ(m_pShmDSN);
        for(int i = 0; i< pDsn->iShmCounts; i++)
        {
            iOldShmID = m_iOldDataShmID[i];
            iShmID = pDsn->iShmID[i];
            TShmList<TMdbTableSpace>::iterator itor = m_pShmDSN->m_TSList.begin();
            //遍历表空间
            for(;itor != m_pShmDSN->m_TSList.end();++itor)
            {
                TMdbTableSpace* pTableSpace = &(*itor);
                if(pTableSpace->sName[0] != 0)
                {
                    TADD_NORMAL("TableSpaceName=[%s].", pTableSpace->sName);
                    //找到节点
                    TMdbTSNode* pNode = (TMdbTSNode*)&pTableSpace->tNode;
                    TADD_NORMAL("pNode->iPageEnd[%d] , pNode->iNext[%d].", pNode->iPageEnd , pNode->iNext);
                    do
                    {
                        TADD_NORMAL(" Cur-ShmID=[%d], OldShmID=[%d], NewShmID=[%d].",pNode->iShmID, iOldShmID, iShmID);
                        if(pNode->iShmID == iOldShmID)
                        {
                            pNode->iShmID = iShmID;
                        }
                        if(pNode->iNext > 0)
                        {
                            pNode = (TMdbTSNode*)(m_pShmDSN->GetAddrByOffset(pNode->iNext));
                        }
                        else
                        {
                            break;
                        }
                    }
                    while(1);
                }
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * 函数名称  :  FlushToImage
    * 函数描述  :  刷出镜像文件
    * 输入      :
    * 输入      :
    * 输出      :
    * 返回值    :  0 - 成功!0 -失败
    * 作者      :  jin.shaohua
    *******************************************************************************/
    int TMdbImage::FlushToImage(const char sFileName[],char * pMemAddr,int iSize)
    {
        TADD_FUNC("Start[%s],size=[%d]",sFileName,iSize);
        int iRet = 0;
        CHECK_OBJ(sFileName);
        CHECK_OBJ(pMemAddr);
        if(TMdbNtcFileOper::IsExist(sFileName))
        {
            CHECK_RET(ERR_APP_FILE_IS_EXIST,"File[%s] is exist",sFileName);
        }
        FILE * fp = fopen(sFileName,"wb");
        int iOneSize = iSize;//1024*1024;// 1M写一次
        int i = 0;
        for(i = 0; i*iOneSize < iSize; ++i)
        {
            int iWrite = 0;
            if((iWrite = fwrite(pMemAddr+i*iOneSize, iOneSize, 1, fp)) != 1 )
            {
                SAFE_CLOSE(fp);
                CHECK_RET(ERR_OS_WRITE_FILE,"fwrite(%s) error[iWrite=%d, iOneSize=%d,i=%d], errno=%d, errormsg=[%s].",
                          sFileName,iWrite,iOneSize,i, errno, strerror(errno));
            }
        }
        TADD_FUNC("Finish");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	: Export
    * 函数描述	:
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::Export(const char * sDsn,const char *sImageDir)
    {
        TADD_FUNC("Start[%s]",sDsn);
        int iRet = 0;
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        CHECK_OBJ(m_pShmDSN);
        m_pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
        CHECK_OBJ(m_pConfig);
        TMdbDSN * pDsn = m_pShmDSN->GetInfo();
        CHECK_OBJ(pDsn);
        char sExportDir[MAX_FILE_NAME] = {0};
        if(0 == sImageDir[0])
        {
            sprintf(sExportDir,"%s/",pDsn->sDataStore);
        }
        else
        {
            sprintf(sExportDir,"%s/",sImageDir);
        }
        //检查存储目录
        if(TMdbNtcDirOper::IsExist(sExportDir) == false)
        {
            //没有目录
            if(TMdbNtcDirOper::MakeFullDir(sExportDir) == false)
            {
                CHECK_RET(ERR_OS_CREATE_DIR,"create dir[%s] failed",sExportDir);
            }
        }
        TADD_NORMAL_TO_CLI(FMT_CLI_START,"Export [%s] to [%s].",sDsn,sExportDir);
        char sFileName[MAX_FILE_NAME] = {0};
        char sPrefix[MAX_FILE_NAME] = {0};
        sprintf(sPrefix,"%s%s",sExportDir,sDsn);
        //导出管理区文本信息
        sprintf(sFileName,"%s_Mgr.ini",sPrefix);
        CHECK_RET(FlushDsnMgrInfo(sFileName,m_pShmDSN->GetInfo()),"FlushDsnMgrInfo failed");
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        //导出管理区
        sprintf(sFileName,"%s_Mgr.img",sPrefix);
        CHECK_RET(FlushToImage(sFileName,(char *)m_pShmDSN->GetAddrByOffset(0),m_pConfig->GetDSN()->iManagerSize),"FlushToImage failed.");
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        //导出基础索引
        int i = 0;
        for(i = 0; i<pDsn->iBaseIndexShmCounts; ++i)
        {
            sprintf(sFileName,"%s_BaseIndex_%d.img",sPrefix,i);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetBaseIndex(i),m_pConfig->GetDSN()->iDataSize),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        //导出冲突索引
        for(i = 0; i<pDsn->iConflictIndexShmCounts; ++i)
        {
            sprintf(sFileName,"%s_ConflictIndex_%d.img",sPrefix,i);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetConflictIndex(i),m_pConfig->GetDSN()->iDataSize),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        //导出数据区
        for(i = 0; i<pDsn->iShmCounts; ++i)
        {
            sprintf(sFileName,"%s_data_%d.img",sPrefix,i);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetDataShm(i),m_pConfig->GetDSN()->iDataSize),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        //导出varchar区
        for(i=0; i<pDsn->iVarCharShmCounts; i++)
        {
            char * pAddr = NULL;
            CHECK_RET(m_pShmDSN->AttachvarCharBlockShm(i,&pAddr),"AttachvarCharBlockShm failed");
            sprintf(sFileName,"%s_varchar_%d.img",sPrefix,i);
            TMdbShmHead *pHead = (TMdbShmHead*)(pAddr);
            CHECK_RET(FlushToImage(sFileName,pAddr,pHead->iTotalSize),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        TMdbSyncArea & tSA = pDsn->m_arrSyncArea;
        if(m_pShmDSN->GetSyncAreaShm() != NULL)
        {
            //导出OraShm
            sprintf(sFileName,"%s_%s.img",sPrefix,tSA.m_sName);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetSyncAreaShm(),tSA.m_iShmSize*1024*1024),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
    //导出同步区
    /*
        for(i = 0;i < SA_MAX;++i)
        {
            TMdbSyncArea & tSA = pDsn->m_arrSyncArea[i];
            if(m_pShmDSN->GetSyncAreaShm(i) != NULL)
            {
                //导出OraShm
                sprintf(sFileName,"%s_%s.img",sPrefix,tSA.m_sName);
                CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetSyncAreaShm(i),tSA.m_iShmSize*1024*1024),"FlushToImage failed.");
                TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
            }
        }
        */
        #if 0
        if(m_pShmDSN->GetOraShm() != NULL)
        {
            //导出OraShm
            sprintf(sFileName,"%s_Ora.img",sPrefix);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetOraShm(),m_pShmDSN->GetInfo()->iOraShmSize*1024*1024),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        if(m_pShmDSN->GetRepShm() != NULL)
        {
            //导出RepShm
            sprintf(sFileName,"%s_Rep.img",sPrefix);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetRepShm(),m_pShmDSN->GetInfo()->iRepShmSize*1024*1024),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        #endif
        
        TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Export [%s].",sDsn);
        return iRet;
    }
    /******************************************************************************
    * 函数名称	: CreateFromImage
    * 函数描述	:  从镜像创建
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::CreateFromImage(const char  sFileName[],long lKey,long iSize,int &iShmID,char * & pAddr)
    {
        TADD_FUNC("Start");
        int iRet = 0;
        int id = shmget(lKey, iSize, 0666 | IPC_EXCL);
        if(id != -1)
        {
            CHECK_RET(ERROR_SEM_EXIST,"share memery is exist");
        }
        CHECK_RET(TMdbShm::Create(lKey, iSize, iShmID),"Can't create share memory, errno=%d, errormsg=[%s]", errno, strerror(errno));
        CHECK_RET(TMdbShm::AttachByID(iShmID, pAddr),"Can't attache share memory, errno=%d, errormsg=[%s]",errno, strerror(errno));
        FILE * fp = NULL;
        fp = fopen(sFileName,"r");
        CHECK_OBJ(fp);
        if(fread(pAddr, iSize, 1, fp) != 1)
        {
            SAFE_CLOSE(fp);
            CHECK_RET(ERR_OS_READ_FILE,"fread[%s]failed.",sFileName);
        }
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  Import
    * 函数描述	:  从镜像目录导入
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::Import(const char * sDsn,const char * sImageDir)
    {
        TADD_FUNC("Start[%s],from[%s]",sDsn,sImageDir);
        int iRet = 0;
        m_pConfig = TMdbConfigMgr::GetMdbConfig(sDsn);
        CHECK_OBJ(m_pConfig);
        char sPrefix[MAX_FILE_NAME] = {0};
        if(0 == sImageDir[0])
        {
            sprintf(sPrefix,"%s/%s",m_pConfig->GetDSN()->sDataStore,sDsn);
        }
        else
        {
            sprintf(sPrefix,"%s/%s",sImageDir,sDsn);
        }
        TMdbDSN * pDsn = NULL;
        TADD_NORMAL_TO_CLI(FMT_CLI_START,"Import[%s] from[%s]",sDsn,sPrefix);
        CHECK_RET(ImportMgr(sPrefix,pDsn),"ImportMgr error.");
        CHECK_RET(ImportIndex(sPrefix,pDsn),"ImportMgr error.");
        CHECK_RET(ImportData(sPrefix,pDsn),"ImportMgr error.");
        CHECK_RET(ImportRepShm(sPrefix,pDsn),"ImportMgr error.");
        CHECK_RET(AdjustTableSpace(pDsn),"AdjustTableSpace error.");
        //重新初始化锁
        TMdbMutexCtrl tMutexCtrl;
        CHECK_RET(tMutexCtrl.Init(sDsn),"tMutexCtrl.Init failed");
        CHECK_RET(tMutexCtrl.ReNewAllMutex(),"ReNewAllMutex failed.");
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"ReNewAllMutex");
        TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Import [%s].",sDsn);
        TADD_FUNC("Finish");
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  FlushDsnMgrInfo
    * 函数描述	:  从镜像目录导入
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::FlushDsnMgrInfo(const char  sFileName[],TMdbDSN * pDsn )
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(sFileName);
        CHECK_OBJ(pDsn);
        if(TMdbNtcFileOper::IsExist(sFileName))
        {
            CHECK_RET(ERR_APP_FILE_IS_EXIST,"File[%s] is exist",sFileName);
        }
        FILE* fp = fopen(sFileName, "w");
        CHECK_OBJ(fp);
        char sData[2048]= {0};
        memset(sData,0,2048);
        //把数据写入文件.按多个文件来划分,比如表空间,等
        sprintf(sData,"[%s]\n",pDsn->sName);
        sprintf(sData+strlen(sData),"sName=%s\n",pDsn->sName);
        sprintf(sData+strlen(sData),"sVersion=%s\n",pDsn->sVersion);
        sprintf(sData+strlen(sData),"cState=%c\n",pDsn->cState);
        sprintf(sData+strlen(sData),"iRepAttr=%d\n",pDsn->iRepAttr);
        sprintf(sData+strlen(sData),"iRoutingID=%d\n",pDsn->iRoutingID);
        sprintf(sData+strlen(sData),"iTableCounts=%d\n",pDsn->iTableCounts);
        sprintf(sData+strlen(sData),"iLogLevel=%d\n",pDsn->iLogLevel);
        sprintf(sData+strlen(sData),"sCreateTime=%s\n",pDsn->sCreateTime);
        sprintf(sData+strlen(sData),"iShmCounts=%d\n",pDsn->iShmCounts);
        char caTemp[1024];
        memset(caTemp,0,1024);
        int i = 0;
        for(i =0 ; i<pDsn->iShmCounts; i++)
        {
            sprintf(caTemp+strlen(caTemp)," %lld",pDsn->iShmKey[i]);
        }
        sprintf(sData+strlen(sData),"iShmKey=%s\n",caTemp);
        sprintf(sData+strlen(sData),"iBaseIndexShmCounts=%d\n",pDsn->iBaseIndexShmCounts);
        memset(caTemp,0,1024);
        for(int i =0 ; i<pDsn->iBaseIndexShmCounts; i++)
        {
            sprintf(caTemp+strlen(caTemp)," %d",pDsn->iBaseIndexShmKey[i]);
        }
        sprintf(sData+strlen(sData),"iBaseIndexShmKey=%s\n",caTemp);
        fputs(sData,fp);
        
        //刷写表的信息
        TShmList<TMdbTable>::iterator itorTb = m_pShmDSN->m_TableList.begin();
        for(;itorTb != m_pShmDSN->m_TableList.end();++itorTb)
        {
            TMdbTable * pTable = &(*itorTb);
            if(pTable->sTableName[0] != 0)
            {
                memset(sData,0,2048);
                sprintf(sData,"[%s]\n",pTable->sTableName);
                sprintf(sData+strlen(sData),"iRecordCounts=%d\n",pTable->iRecordCounts);
                sprintf(sData+strlen(sData),"sCreateTime=%s\n",pTable->sCreateTime);
                sprintf(sData+strlen(sData),"cState=%c\n",pTable->cState);
                sprintf(sData+strlen(sData),"lTotalCollIndexNodeCounts=%lld\n",pTable->lTotalCollIndexNodeCounts);
                sprintf(sData+strlen(sData),"lLeftCollIndexNodeCounts=%lld\n",pTable->lLeftCollIndexNodeCounts);
                sprintf(sData+strlen(sData),"iCounts=%d\n",pTable->iCounts);
                sprintf(sData+strlen(sData),"iCINCounts=%d\n",pTable->iCINCounts);
                sprintf(sData+strlen(sData),"iFullPages=%d\n",pTable->iFullPages);
                sprintf(sData+strlen(sData),"iFullPageID=%d\n",pTable->iFullPageID);
                sprintf(sData+strlen(sData),"iFreePages=%d\n",pTable->iFreePages);
                sprintf(sData+strlen(sData),"iFreePageID=%d\n",pTable->iFreePageID);
                fputs(sData,fp);
            }
        }
        //刷写表空间的信息
        TShmList<TMdbTableSpace>::iterator itorTS = m_pShmDSN->m_TSList.begin();
        for(;itorTS != m_pShmDSN->m_TSList.end();++itorTS)
        {
            TMdbTableSpace * pTableSpace = &(*itorTS);
            if(pTableSpace->sName[0] != 0)
            {
                memset(sData,0,2048);
                sprintf(sData,"[%s]\n",pTableSpace->sName);
                sprintf(sData+strlen(sData),"iEmptyPages=%d\n",pTableSpace->iEmptyPages);
                sprintf(sData+strlen(sData),"sName=%s\n",pTableSpace->sName);
                sprintf(sData+strlen(sData),"iTotalPages=%d\n",pTableSpace->iTotalPages);
                sprintf(sData+strlen(sData),"iEmptyPageID=%d\n",pTableSpace->iEmptyPageID);
                sprintf(sData+strlen(sData),"cState=%c\n",pTableSpace->cState);
                sprintf(sData+strlen(sData),"sCreateTime=%s\n",pTableSpace->sCreateTime);
                fputs(sData,fp);
            }
        }
        //刷写序列信息
        TShmList<TMemSeq>::iterator itorSeq = m_pShmDSN->m_MemSeqList.begin();
        for(;itorSeq != m_pShmDSN->m_MemSeqList.end();++itorSeq)
        {
            TMemSeq * pSeq = &(*itorSeq);
            if(pSeq->sSeqName[0] !=0 )
            {
                memset(sData,0,2048);
                sprintf(sData,"[%s]\n",pSeq->sSeqName);
                sprintf(sData+strlen(sData),"sSeqName=%s\n",pSeq->sSeqName);
                sprintf(sData+strlen(sData),"iStart=%lld\n",pSeq->iStart);
                sprintf(sData+strlen(sData),"iEnd=%lld\n",pSeq->iEnd);
                sprintf(sData+strlen(sData),"iCur=%lld\n",pSeq->iCur);
                sprintf(sData+strlen(sData),"iStep=%lld\n",pSeq->iStep);
                fputs(sData,fp);
            }
        }
        SAFE_CLOSE(fp);
        return iRet;
    }

    /******************************************************************************
    * 函数名称	:  ImportMgr
    * 函数描述	:  加载管理区
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::ImportMgr(const char * sPrefix,TMdbDSN * & pDsn)
    {
        int iRet = 0;
        char sFileName[MAX_FILE_NAME] = {0};
        int iShmID = -1;
        char * pAddr = NULL;
        MDB_UINT64 lSize = 0;
        //加载管理区
        //long long iMgrKey = MANAGER_KEY + m_pConfig->GetDSN()->llValue;
        long long iMgrKey = GET_MGR_KEY(m_pConfig->GetDSN()->llValue);
        
        sprintf(sFileName,"%s_Mgr.img",sPrefix);
        if(TMdbNtcFileOper::IsExist(sFileName))
        {
            TMdbNtcFileOper::GetFileSize(sFileName,lSize);
            CHECK_RET(CreateFromImage(sFileName,iMgrKey,lSize,iShmID,pAddr),"CreateFromImage failed");
        }
        else
        {
            CHECK_RET(ERR_OS_NO_FILE,"Mgr file[%s] is not exist",sFileName);
        }
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Import [%s]",sFileName);
        pDsn = (TMdbDSN * )(pAddr+sizeof(TShmAllocHead));
        CHECK_OBJ(pDsn);
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  ImportIndex
    * 函数描述	:  加载索引区
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::ImportIndex(const char * sPrefix,TMdbDSN *  pDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pDsn);
        char sFileName[MAX_FILE_NAME] = {0};
        int iShmID = -1;
        char * pAddr = NULL;
        MDB_UINT64 lSize = 0;
        //加载基础索引区
        int i = 0;
        do
        {
            sprintf(sFileName,"%s_BaseIndex_%d.img",sPrefix,i);
            if(TMdbNtcFileOper::IsExist(sFileName))
            {
                TMdbNtcFileOper::GetFileSize(sFileName,lSize);
                CHECK_RET(CreateFromImage(sFileName,pDsn->iBaseIndexShmKey[i],lSize,iShmID,pAddr),"CreateFromImage failed");
                pDsn->iBaseIndexShmID[i] = iShmID;
            }
            else
            {
                break;
            }
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Import [%s]",sFileName);
        }
        while(++i);
        //加载冲突索引区
        i = 0;
        do
        {
            sprintf(sFileName,"%s_ConflictIndex_%d.img",sPrefix,i);
            if(TMdbNtcFileOper::IsExist(sFileName))
            {
                TMdbNtcFileOper::GetFileSize(sFileName,lSize);
                CHECK_RET(CreateFromImage(sFileName,pDsn->iConflictIndexShmKey[i],lSize,iShmID,pAddr),"CreateFromImage failed");
                pDsn->iConflictIndexShmID[i] = iShmID;
            }
            else
            {
                break;
            }
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Import [%s]",sFileName);
        }
        while(++i);
        return iRet;
    }
    /******************************************************************************
    * 函数名称	:  ImportData
    * 函数描述	:  加载数据区
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::ImportData(const char * sPrefix,TMdbDSN *  pDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pDsn);
        char sFileName[MAX_FILE_NAME] = {0};
        int iShmID = -1;
        char * pAddr = NULL;
        MDB_UINT64 lSize = 0;
        //加载数据区
        int i = 0;
        do
        {
            sprintf(sFileName,"%s_data_%d.img",sPrefix,i);
            if(TMdbNtcFileOper::IsExist(sFileName))
            {
                TMdbNtcFileOper::GetFileSize(sFileName,lSize);
                CHECK_RET(CreateFromImage(sFileName,pDsn->iShmKey[i],lSize,iShmID,pAddr),"CreateFromImage failed");
                m_iOldDataShmID[i] = pDsn->iShmID[i];
                pDsn->iShmID[i] = iShmID;
                TMdbShmHead *pHead = (TMdbShmHead*)pAddr;
                pHead->iShmID = iShmID;
            }
            else
            {
                break;
            }
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Import [%s]",sFileName);
        }
        while(++i);
        //加载varchar
        i = 0;
        do
        {
            sprintf(sFileName,"%s_varchar_%d.img",sPrefix,i);
            if(TMdbNtcFileOper::IsExist(sFileName))
            {
                TMdbNtcFileOper::GetFileSize(sFileName,lSize);
                CHECK_RET(CreateFromImage(sFileName,pDsn->iVarCharShmKey[i],lSize,iShmID,pAddr),"CreateFromImage failed");
                pDsn->iVarCharShmID[i] = iShmID;
                TMdbShmHead *pHead = (TMdbShmHead*)pAddr;
                pHead->iShmID = iShmID;
            }
            else
            {
                break;
            }
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Import [%s]",sFileName);
        }
        while(++i);
        return iRet;

    }
    /******************************************************************************
    * 函数名称	:  ImportRepShm
    * 函数描述	:  加载同步区
    * 输入		:
    * 输入		:
    * 输出		:
    * 返回值	:  0 - 成功!0 -失败
    * 作者		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::ImportRepShm(const char * sPrefix,TMdbDSN *  pDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pDsn);
        char sFileName[MAX_FILE_NAME] = {0};
        int iShmID = -1;
        char * pAddr = NULL;
        MDB_UINT64 lSize = 0;
        TMdbSyncArea & tSA = pDsn->m_arrSyncArea;
        sprintf(sFileName,"%s_%s.img",sPrefix,tSA.m_sName);
        if(TMdbNtcFileOper::IsExist(sFileName))
        {
            TMdbNtcFileOper::GetFileSize(sFileName,lSize);
            CHECK_RET(CreateFromImage(sFileName,tSA.m_iShmKey,lSize,iShmID,pAddr),"CreateFromImage failed");
            tSA.m_iShmID= iShmID;
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Import [%s]",sFileName);
        }
        /*
        int i = 0;
        for(i = 0;i < SA_MAX;++i)
        {
            TMdbSyncArea & tSA = pDsn->m_arrSyncArea[i];
            sprintf(sFileName,"%s_%s.img",sPrefix,tSA.m_sName);
            if(TMdbNtcFileOper::IsExist(sFileName))
            {
                TMdbNtcFileOper::GetFileSize(sFileName,lSize);
                CHECK_RET(CreateFromImage(sFileName,tSA.m_iShmKey,lSize,iShmID,pAddr),"CreateFromImage failed");
                tSA.m_iShmID= iShmID;
                TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Import [%s]",sFileName);
            }
        }
        */
        #if 0
        //加载ora shm
        sprintf(sFileName,"%s_Ora.img",sPrefix);
        if(TMdbFileOper::IsExist(sFileName))
        {
            lSize =  TMdbFileOper::GetFileSize(sFileName);
            CHECK_RET(CreateFromImage(sFileName,pDsn->m_arrSyncArea[SA_ORACLE].m_iShmKey,lSize,iShmID,pAddr),"CreateFromImage failed");
            pDsn->m_arrSyncArea[SA_ORACLE].m_iShmID= iShmID;
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Import [%s]",sFileName);
        }
        //加载rep shm
        sprintf(sFileName,"%s_Rep.img",sPrefix);
        if(TMdbFileOper::IsExist(sFileName))
        {
            lSize =  TMdbFileOper::GetFileSize(sFileName);
            CHECK_RET(CreateFromImage(sFileName,pDsn->m_arrSyncArea[SA_REP].m_iShmKey,lSize,iShmID,pAddr),"CreateFromImage failed");
            pDsn->m_arrSyncArea[SA_REP].m_iShmID= iShmID;
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Import [%s]",sFileName);
        }
        #endif
        
        return iRet;
    }
//}
