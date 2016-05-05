/****************************************************************************************
*@Copyrights  2009�����������Ͼ�����������޹�˾ ������ CCB��Ŀ--C++�����
*@                   All rights reserved.
*@Name��	    TMdbImage.cpp
*@Description�� �ڴ����ݿ�ľ������
*@Author:		li.shugang
*@Modify:       jiang.mingjun
*@Date��	    2009��04��24��
*@History:      ����Э��ӿ��ļ�,���ڴ����ݿ�ˢ��ָ����ʽ�ļ�.
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

    //������ռ���ShmID��ӳ���ϵ
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
            //������ռ�
            for(;itor != m_pShmDSN->m_TSList.end();++itor)
            {
                TMdbTableSpace* pTableSpace = &(*itor);
                if(pTableSpace->sName[0] != 0)
                {
                    TADD_NORMAL("TableSpaceName=[%s].", pTableSpace->sName);
                    //�ҵ��ڵ�
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
    * ��������  :  FlushToImage
    * ��������  :  ˢ�������ļ�
    * ����      :
    * ����      :
    * ���      :
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����      :  jin.shaohua
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
        int iOneSize = iSize;//1024*1024;// 1Mдһ��
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
    * ��������	: Export
    * ��������	:
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        //���洢Ŀ¼
        if(TMdbNtcDirOper::IsExist(sExportDir) == false)
        {
            //û��Ŀ¼
            if(TMdbNtcDirOper::MakeFullDir(sExportDir) == false)
            {
                CHECK_RET(ERR_OS_CREATE_DIR,"create dir[%s] failed",sExportDir);
            }
        }
        TADD_NORMAL_TO_CLI(FMT_CLI_START,"Export [%s] to [%s].",sDsn,sExportDir);
        char sFileName[MAX_FILE_NAME] = {0};
        char sPrefix[MAX_FILE_NAME] = {0};
        sprintf(sPrefix,"%s%s",sExportDir,sDsn);
        //�����������ı���Ϣ
        sprintf(sFileName,"%s_Mgr.ini",sPrefix);
        CHECK_RET(FlushDsnMgrInfo(sFileName,m_pShmDSN->GetInfo()),"FlushDsnMgrInfo failed");
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        //����������
        sprintf(sFileName,"%s_Mgr.img",sPrefix);
        CHECK_RET(FlushToImage(sFileName,(char *)m_pShmDSN->GetAddrByOffset(0),m_pConfig->GetDSN()->iManagerSize),"FlushToImage failed.");
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        //������������
        int i = 0;
        for(i = 0; i<pDsn->iBaseIndexShmCounts; ++i)
        {
            sprintf(sFileName,"%s_BaseIndex_%d.img",sPrefix,i);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetBaseIndex(i),m_pConfig->GetDSN()->iDataSize),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        //������ͻ����
        for(i = 0; i<pDsn->iConflictIndexShmCounts; ++i)
        {
            sprintf(sFileName,"%s_ConflictIndex_%d.img",sPrefix,i);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetConflictIndex(i),m_pConfig->GetDSN()->iDataSize),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        //����������
        for(i = 0; i<pDsn->iShmCounts; ++i)
        {
            sprintf(sFileName,"%s_data_%d.img",sPrefix,i);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetDataShm(i),m_pConfig->GetDSN()->iDataSize),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        //����varchar��
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
            //����OraShm
            sprintf(sFileName,"%s_%s.img",sPrefix,tSA.m_sName);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetSyncAreaShm(),tSA.m_iShmSize*1024*1024),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
    //����ͬ����
    /*
        for(i = 0;i < SA_MAX;++i)
        {
            TMdbSyncArea & tSA = pDsn->m_arrSyncArea[i];
            if(m_pShmDSN->GetSyncAreaShm(i) != NULL)
            {
                //����OraShm
                sprintf(sFileName,"%s_%s.img",sPrefix,tSA.m_sName);
                CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetSyncAreaShm(i),tSA.m_iShmSize*1024*1024),"FlushToImage failed.");
                TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
            }
        }
        */
        #if 0
        if(m_pShmDSN->GetOraShm() != NULL)
        {
            //����OraShm
            sprintf(sFileName,"%s_Ora.img",sPrefix);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetOraShm(),m_pShmDSN->GetInfo()->iOraShmSize*1024*1024),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        if(m_pShmDSN->GetRepShm() != NULL)
        {
            //����RepShm
            sprintf(sFileName,"%s_Rep.img",sPrefix);
            CHECK_RET(FlushToImage(sFileName,m_pShmDSN->GetRepShm(),m_pShmDSN->GetInfo()->iRepShmSize*1024*1024),"FlushToImage failed.");
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Export [%s].",sFileName);
        }
        #endif
        
        TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Export [%s].",sDsn);
        return iRet;
    }
    /******************************************************************************
    * ��������	: CreateFromImage
    * ��������	:  �Ӿ��񴴽�
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
    * ��������	:  Import
    * ��������	:  �Ӿ���Ŀ¼����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        //���³�ʼ����
        TMdbMutexCtrl tMutexCtrl;
        CHECK_RET(tMutexCtrl.Init(sDsn),"tMutexCtrl.Init failed");
        CHECK_RET(tMutexCtrl.ReNewAllMutex(),"ReNewAllMutex failed.");
        TADD_NORMAL_TO_CLI(FMT_CLI_OK,"ReNewAllMutex");
        TADD_NORMAL_TO_CLI(FMT_CLI_SUCCESS,"Import [%s].",sDsn);
        TADD_FUNC("Finish");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  FlushDsnMgrInfo
    * ��������	:  �Ӿ���Ŀ¼����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        //������д���ļ�.������ļ�������,�����ռ�,��
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
        
        //ˢд�����Ϣ
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
        //ˢд��ռ����Ϣ
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
        //ˢд������Ϣ
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
    * ��������	:  ImportMgr
    * ��������	:  ���ع�����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::ImportMgr(const char * sPrefix,TMdbDSN * & pDsn)
    {
        int iRet = 0;
        char sFileName[MAX_FILE_NAME] = {0};
        int iShmID = -1;
        char * pAddr = NULL;
        MDB_UINT64 lSize = 0;
        //���ع�����
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
    * ��������	:  ImportIndex
    * ��������	:  ����������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::ImportIndex(const char * sPrefix,TMdbDSN *  pDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pDsn);
        char sFileName[MAX_FILE_NAME] = {0};
        int iShmID = -1;
        char * pAddr = NULL;
        MDB_UINT64 lSize = 0;
        //���ػ���������
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
        //���س�ͻ������
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
    * ��������	:  ImportData
    * ��������	:  ����������
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
    *******************************************************************************/
    int TMdbImage::ImportData(const char * sPrefix,TMdbDSN *  pDsn)
    {
        int iRet = 0;
        CHECK_OBJ(pDsn);
        char sFileName[MAX_FILE_NAME] = {0};
        int iShmID = -1;
        char * pAddr = NULL;
        MDB_UINT64 lSize = 0;
        //����������
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
        //����varchar
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
    * ��������	:  ImportRepShm
    * ��������	:  ����ͬ����
    * ����		:
    * ����		:
    * ���		:
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jin.shaohua
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
        //����ora shm
        sprintf(sFileName,"%s_Ora.img",sPrefix);
        if(TMdbFileOper::IsExist(sFileName))
        {
            lSize =  TMdbFileOper::GetFileSize(sFileName);
            CHECK_RET(CreateFromImage(sFileName,pDsn->m_arrSyncArea[SA_ORACLE].m_iShmKey,lSize,iShmID,pAddr),"CreateFromImage failed");
            pDsn->m_arrSyncArea[SA_ORACLE].m_iShmID= iShmID;
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Import [%s]",sFileName);
        }
        //����rep shm
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
