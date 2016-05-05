/****************************************************************************************
*@Copyrights  2014�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	   mdbRepLog.cpp		
*@Description: �������ڴ�����ͬ�����ݰ�·�ɺ�������ص���ͬ���ļ���ͬһ��·�ɶ�Ӧ�������ʱ������ض��
*@Author:		jiang.lili
*@Date��	    2014/05/4
*@History:
******************************************************************************************/
#include "Replication/mdbRepLog.h"
#include "Helper/mdbDateTime.h"
//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB
//{
    TRoutingRepFile::TRoutingRepFile()
    {
        m_iBufLen = 0;
        m_iHostID = DEFALUT_ROUT_ID;
        m_tCreateTime = 0;
        m_fp = NULL;
        m_iBufLen = 0;
        m_iFilePos = 0;
        memset(m_sFileBuf, 0x00, sizeof(m_sFileBuf));
    }
    TRoutingRepFile::~TRoutingRepFile()
    {
        SAFE_CLOSE(m_fp);
    }
    TRoutingRepLog::TRoutingRepLog():m_pRepConfig(NULL)
        , m_pShmMgr(NULL)
        ,m_pShmRep(NULL)
        ,m_pQueueCtrl(NULL)
        ,m_pShmDSN(NULL)
        ,m_pDsn(NULL)
        ,m_pRecdParser(NULL)
    {
        memset(m_sLogPath, 0, sizeof(m_sLogPath));
        m_iRecord = 0;
        m_iCheckCounts = 0;
        m_iLen = 0;
        m_spzRecord = NULL;
        m_iRoutID = 0;

    }

    TRoutingRepLog::~TRoutingRepLog()
    {
        SAFE_DELETE(m_pShmMgr);
        SAFE_DELETE(m_pRepConfig);
        m_pShmRep = NULL;
        SAFE_DELETE(m_pRecdParser);
        ClearFileMap();
    }
    /******************************************************************************
    * ��������	:  Init
    * ��������	: ��ʼ�������ӹ����ڴ棬��ȡ�����ļ�
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TRoutingRepLog::Init(const char* sDsn, TMdbQueue & mdbQueueCtrl)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        m_pShmMgr = new(std::nothrow) TMdbShmRepMgr(sDsn);
        CHECK_OBJ(m_pShmMgr);
        CHECK_RET(m_pShmMgr->Attach(),"Attach to shared memory failed.");
        m_pShmRep = m_pShmMgr->GetRoutingRep();

        m_pRepConfig = new(std::nothrow) TMdbRepConfig();
        CHECK_OBJ(m_pRepConfig);
        CHECK_RET(m_pRepConfig->Init(sDsn), "TMdbRepConfig failed.");
        m_pShmDSN = TMdbShmMgr::GetShmDSN(sDsn);
        if(m_pShmDSN == NULL)
        {
            CHECK_RET(ERR_APP_INVALID_PARAM,"Init(%s) : GetShmDSN(%s) failed.", sDsn, sDsn);
        }

        m_pDsn = m_pShmDSN->GetInfo();

        SAFESTRCPY(m_sLogPath, sizeof(m_sLogPath), m_pDsn->m_arrSyncArea.m_sDir[SA_REP]);
        if(m_sLogPath[strlen(m_sLogPath)-1] != '/')
        {
            m_sLogPath[strlen(m_sLogPath)] = '/';
        }
        if(false == TMdbNtcDirOper::IsExist(m_sLogPath) && false == TMdbNtcDirOper::MakeFullDir(m_sLogPath))
        {//����Ŀ¼
            CHECK_RET(ERR_APP_INVALID_PARAM,"Mkdir[%s] faild.",m_sLogPath);
        }
        TADD_NORMAL("m_sLogPath=[%s]",m_sLogPath);
        
        m_pRecdParser = new TMdbRepRecdDecode();
        CHECK_OBJ(m_pRecdParser);
        m_tProcCtrl.Init(sDsn);
        ClearFileMap();

        m_pQueueCtrl = &mdbQueueCtrl;

        TADD_FUNC("Finish.");
        return iRet;
    }
        /******************************************************************************
        * ��������	:  Log
        * ��������	: ���𻺳�����Ƭ�����������
        * ����		:  bEmpty �������Ƿ��з�Ƭ��������
        * ���		:  
        * ����ֵ	:  0 - �ɹ�!0 -ʧ��
        * ����		:  jiang.lili
        *******************************************************************************/
    int TRoutingRepLog::Log(bool bEmpty /* = false */)
    {
        int iRet = 0;
        TADD_FUNC("TMdbRepLog::Log() : Start.");
        m_iCheckCounts++;
        if(m_iCheckCounts >= 200)
        {
            CheckAndBackup();
            m_iCheckCounts = 0;
        }

        if (bEmpty)//������û������
        {
            return iRet;
        }

        m_spzRecord = m_pQueueCtrl->GetData();//��ȡ�Ѿ�pop����������
        if(m_spzRecord[0] == 0)//û������
        { 
            return iRet;
        }
        m_iLen = m_pQueueCtrl->GetRecordLen();
        //iRoutID = m_pRecdParser->GetRoutingID(m_spzRecord);
        m_iRoutID = m_tParser.GetRoutingID(m_spzRecord);
        TADD_NORMAL("Routid=[%d],Recd=[%s]", m_iRoutID, m_spzRecord);
        if(m_pShmRep->m_bNoRtMode == true)
        {
            TADD_NORMAL("TEST:no-routing-id-rep mode! set send routing-id = %d",DEFALUT_ROUT_ID);
            m_iRoutID = DEFALUT_ROUT_ID;
        }
        Write(m_iRoutID, m_spzRecord, m_iLen);
        ++m_iRecord;
        if(m_iRecord == 10000 )
        {
            TADD_DETAIL("TMdbRepLog::Log() : Deal-Records=%ld.", m_iRecord);
            m_iRecord = 0;
        }
        TADD_FUNC("TMdbRepLog::Log() : Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  Write
    * ��������	:  ��ָ��·������д���Ӧ���ļ�
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TRoutingRepLog::Write(int iRoutingID, const char* sDataBuf, int iBufLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TMdbShmRepRouting* pRepHost = m_pShmMgr->GetRepHosts(iRoutingID);
        if (NULL == pRepHost)//��ȡ���������������Ǹ�·�ɲ��ֲ��ڱ������ϣ����߸�·�����뱸��
        {
#if 0
            for (unsigned i = 0; i < m_pShmMgr->GetRoutingRep()->m_iRoutingIDCount; i++)
            {
                if (m_pShmMgr->GetRoutingRep()->m_arRouting[i] == iRoutingID)
                {
                    TADD_NORMAL("Routing ID[%d] needn't rep.", iRoutingID);
                    break;
                }
            }
            if (i == m_pShmMgr->GetRoutingRep()->m_iRoutingIDCount)
            {
                TADD_WARNING("Cannot get the standby host for routing_ID[%d]. Check if the routing id is distributed on this host.", iRoutingID);
            }
            
#endif
            return 0;
        }
        TRoutingRepFile* pRepFile=NULL;
        for (int i = 0; i<MAX_REP_HOST_COUNT; i++)
        {
            if (pRepHost->m_aiHostID[i] != MDB_REP_EMPTY_HOST_ID)
            {
                TADD_DETAIL("write to rep host.");
                pRepFile = GetRepFile(pRepHost->m_aiHostID[i]);
                CHECK_OBJ(pRepFile);
                CHECK_RET(CheckWriteToFile(pRepFile, sDataBuf, iBufLen), "CheckWriteToFile failed.");//����д�ļ�
            }
        }
        // д����
        if (pRepHost->m_iRecoveryHostID != MDB_REP_EMPTY_HOST_ID)
        {
            TADD_DETAIL("write to recovery host.");
            pRepFile = GetRepFile(pRepHost->m_iRecoveryHostID);
            CHECK_OBJ(pRepFile);
            CHECK_RET(CheckWriteToFile(pRepFile, sDataBuf, iBufLen), "CheckWriteToFile failed.");//����д�ļ�
        }

        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  CheckAndBackup
    * ��������	:  ����ļ�����ʱ�䣬����ʱ�ļ�����
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TRoutingRepLog::CheckAndBackup()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TRoutingRepFile *pRepFile = NULL;
        std::map<int, TRoutingRepFile*>::iterator itor = m_tMapFile.begin();

        for (; itor!=m_tMapFile.end(); ++itor)
        {    
            pRepFile = itor->second;
            WriteToFile(pRepFile);

            if (pRepFile->m_iFilePos <=0)
            {
                continue;
            }
           
            if (pRepFile->m_iFilePos > m_pRepConfig->m_iMaxFileSize 
                || /*QuickMDB::*/TMdbNtcDateTime::GetDiffSeconds(/*QuickMDB::*/TMdbNtcDateTime::GetCurTime(), pRepFile->m_tCreateTime)>= m_pRepConfig->m_iBackupInterval)
            {
                CHECK_RET(Backup(pRepFile), "Backup file failed.");
            }
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    void TRoutingRepLog::WriteToFile(TRoutingRepFile* pRepFile)
    {
        if(pRepFile->m_iBufLen <= 0){return;}
        if(fwrite(pRepFile->m_sFileBuf, pRepFile->m_iBufLen, 1, pRepFile->m_fp) !=1 )
        {
            TADD_ERROR(ERR_OS_WRITE_FILE,"[%s : %d] : TMdbRepLog::Log() : fwrite() failed, errno=%d, errmsg=[%s].", __FILE__, __LINE__, errno, strerror(errno));
            return;
        }
        
        pRepFile->m_iFilePos += pRepFile->m_iBufLen;

        //memset(pRepFile->m_sFileBuf, 0, sizeof(pRepFile->m_sFileBuf));
        pRepFile->m_sFileBuf[0] = '\0';
        pRepFile->m_iBufLen = 0;
        return;
    }
    /******************************************************************************
    * ��������	:  CreateRepFile
    * ��������	: ����ͬ���ļ�
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TRoutingRepLog::CreateRepFile(TRoutingRepFile *ptRepFile, int iHostID)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sFileName[MAX_NAME_LEN] = {0};
        snprintf(sFileName, MAX_NAME_LEN, "%sRep.%d",m_sLogPath, iHostID);
        FILE *fp = fopen(sFileName, "wb");
        if (fp == NULL)
        {
            CHECK_RET(ERR_OS_OPEN_FILE, "Open file [%s] failed. errno = [%d], errmsg = [%s]", sFileName, errno, strerror(errno));
        }

        TADD_DETAIL("sFileName=%s",sFileName);

        ptRepFile->m_iHostID = iHostID;
        ptRepFile->m_fp = fp;
        ptRepFile->m_strFileName = sFileName;
        ptRepFile->m_tCreateTime = /*QuickMDB::*/TMdbNtcDateTime::GetCurTime();
        ptRepFile->m_iFilePos = 0;
        setbuf(ptRepFile->m_fp,NULL);
 
        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  CheckWriteToFile
    * ��������	:  ���ݻ���д�ļ�
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TRoutingRepLog::CheckWriteToFile(TRoutingRepFile*ptRepFile, const char* sDataBuf, int iBufLen)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if (ptRepFile->m_iBufLen+iBufLen<MDB_MAX_REP_FILE_BUF_LEN)//������δ����д�뻺����
        {
            snprintf(&ptRepFile->m_sFileBuf[ptRepFile->m_iBufLen], (MDB_MAX_REP_FILE_BUF_LEN-ptRepFile->m_iBufLen), "%s",
                sDataBuf);//����һ����¼��������
            ptRepFile->m_iBufLen += iBufLen;
        }
        else//������������д���ļ�
        {
            if (fwrite(ptRepFile->m_sFileBuf, ptRepFile->m_iBufLen, 1, ptRepFile->m_fp) !=1 )
            {
                CHECK_RET(ERR_OS_OPEN_FILE, "fwrite() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));
            }

            ptRepFile->m_iFilePos+=ptRepFile->m_iBufLen;

            snprintf(ptRepFile->m_sFileBuf, MDB_MAX_REP_FILE_BUF_LEN, "%s",  sDataBuf);
            ptRepFile->m_iBufLen = iBufLen;      
        }

        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������	:  Backup
    * ��������	: ����ͬ���ļ�
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    int TRoutingRepLog::Backup(TRoutingRepFile*pRepFile)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        TADD_NORMAL("Backup.");

        
        char sBakName[MAX_PATH_NAME_LEN] = {0};
        snprintf(sBakName, MAX_PATH_NAME_LEN, "%s.%lld.OK", pRepFile->m_strFileName.c_str(), (long long)/*QuickMDB::*/TMdbNtcDateTime::GetCurTime());
        /*QuickMDB::*/TMdbNtcFileOper::Rename(pRepFile->m_strFileName.c_str(), sBakName);
        TADD_DETAIL("sBakName=%s",sBakName);

        SAFE_CLOSE(pRepFile->m_fp);
        
        //pRepFile->m_iPos = 0;
        pRepFile->m_fp = fopen(pRepFile->m_strFileName.c_str(),"wb");
        if(pRepFile->m_fp == NULL)
        {
            CHECK_RET(ERROR_UNKNOWN, "Check And Backup Log File: Open file [%s] failed. errno = [%d], errmsg = [%s]", pRepFile->m_strFileName.c_str(), errno, strerror(errno));
        }
        setbuf(pRepFile->m_fp ,NULL);
        pRepFile->m_iFilePos = 0;
        //pRepFile->m_iBufLen= 0;
        pRepFile->m_tCreateTime = /*QuickMDB::*/TMdbNtcDateTime::GetCurTime();

        TADD_FUNC("Finish.");
        return iRet;
    }
    /******************************************************************************
    * ��������	:  GetRepFile
    * ��������	: ����HostID���ҵ���Ӧ���ļ���Ϣ�࣬û���򴴽�
    * ����		:  
    * ���		:  
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  jiang.lili
    *******************************************************************************/
    TRoutingRepFile* TRoutingRepLog::GetRepFile(int iHostID)
    {
        
        TADD_DETAIL("iHostID=%d",iHostID);
        TRoutingRepFile *pRepFile = NULL;
        std::map<int, TRoutingRepFile*>::iterator itor = m_tMapFile.find(iHostID);
        if (itor == m_tMapFile.end())
        {
            pRepFile = new(std::nothrow) TRoutingRepFile();
            if (pRepFile !=NULL && CreateRepFile(pRepFile, iHostID) == 0)
            {
                TADD_DETAIL("map add iHostID=%d",iHostID);
                m_tMapFile.insert(std::pair<int, TRoutingRepFile*>(iHostID, pRepFile));
            }
            else
            {
                SAFE_DELETE(pRepFile);
            }
        }
        else
        {
            pRepFile = itor->second;
        }
        return pRepFile;
    }

    void TRoutingRepLog::ClearFileMap()
    {
        for (unsigned int i = 0; i<m_tMapFile.size(); i++)
        {
            SAFE_DELETE(m_tMapFile[i]);
        }
    }
//}
