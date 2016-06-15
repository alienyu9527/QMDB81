#include "Control/mdbCaptureLog.h"
#include "Helper/mdbDateTime.h"
//#include "BillingSDK.h"
//using namespace ZSmart::BillingSDK;

//namespace QuickMDB{

    TMdbCaptureLog::TMdbCaptureLog():m_pQueueCtrl(NULL), m_pParser(NULL), m_pCurFile(NULL), m_pData(NULL), m_iCheckCnt(0), m_iLogTime(0), m_iMaxFileSize(0)
    {
        memset(m_sDsn, 0, sizeof(m_sDsn));
        memset(m_sLogPath, 0, sizeof(m_sLogPath));
        memset(m_sTime, 0, sizeof(m_sTime));
        memset(m_sRecd, 0, sizeof(m_sRecd));
    }

    TMdbCaptureLog::~TMdbCaptureLog()
    {
        SAFE_DELETE(m_pParser);
        SAFE_CLOSE(m_pCurFile);
    }

    int TMdbCaptureLog::Init(const char* psDsn, TMdbQueue & mdbQueueCtrl)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(psDsn);

        TMdbShmDSN* pShmDsn = TMdbShmMgr::GetShmDSN(psDsn);
        CHECK_OBJ(pShmDsn);

        TMdbConfig* pConfig = TMdbConfigMgr::GetMdbConfig(psDsn);
        CHECK_OBJ(pConfig);

        TMdbDSN* pDsn = pShmDsn->GetInfo();

        m_iMaxFileSize = pDsn->m_arrSyncArea.m_iFileSize;
        m_iLogTime = pConfig->GetDSN()->iLogTime;
        SAFESTRCPY(m_sLogPath, sizeof(m_sLogPath), pDsn->m_arrSyncArea.m_sDir[SA_CAPTURE]);
        if(m_sLogPath[strlen(m_sLogPath)-1] != '/')
        {
            m_sLogPath[strlen(m_sLogPath)] = '/';
        }
        if(false == TMdbNtcDirOper::IsExist(m_sLogPath) && false == TMdbNtcDirOper::MakeFullDir(m_sLogPath))
        {//创建目录
            CHECK_RET(ERR_APP_INVALID_PARAM,"Mkdir[%s] faild.",m_sLogPath);
        }
        TADD_FLOW("m_sLogPath=[%s]",m_sLogPath);

        char sTempFile[MAX_FILE_NAME] = {0};
        sprintf(sTempFile,"%s/temp.%s",m_sLogPath,"capture");//临时文件
        m_sTempFile = sTempFile;
        sprintf(sTempFile,"%s/%s.OK",m_sLogPath,"capture");//OK文件待读取
        m_sOKFile = sTempFile;

        m_pCurFile = fopen(m_sTempFile.c_str(),"a+");
        CHECK_OBJ(m_pCurFile);
        
        m_pParser = new TMdbRepRecdDecode();
        CHECK_OBJ(m_pParser );

        TMdbDateTime::GetCurrentTimeStr(m_sTime);

        m_pQueueCtrl = &mdbQueueCtrl;
        
        return iRet;
    }

        /******************************************************************************
        * 函数名称	:  Log
        * 函数描述	: 负责缓冲区需要捕获的数据落地
        * 输入		:  bEmpty 缓冲区是否有数据
        * 输出		:  
        * 返回值	:  0 - 成功!0 -失败
        * 作者		: 
        *******************************************************************************/
    int TMdbCaptureLog::Log(bool bEmpty /* = false */)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
         if(m_iCheckCnt >= 100)
        {
            //检查是否需要备份
            BackUp();
            m_iCheckCnt = 0;
        }
        m_iCheckCnt++;
        
        if (bEmpty)//没有数据
        {
            return iRet;
        }

        m_pData = m_pQueueCtrl->GetData();
        if(m_pData[0] == 0)
        {//没有数据
            return iRet;
        }
        Adapt();
        WriteToFile();
        return iRet;
    }

    int TMdbCaptureLog::BackUp()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        fflush(m_pCurFile);
        MDB_UINT64  filesize = 0 ;
        if (!TMdbNtcFileOper::GetFileSize(m_sTempFile.c_str(), filesize))
        {
            TADD_ERROR(ERR_OS_GET_FILE_SIZE, "Get file[%s] size failed.", m_sTempFile.c_str());
        }
        
        char sTemp[MAX_TIME_LEN] = {0};
        TMdbDateTime::GetCurrentTimeStr(sTemp);
        
        if(0 == filesize)
        {//空文件不备份
            SAFESTRCPY(m_sTime, MAX_TIME_LEN, sTemp);
            return 0;
        }
        if (filesize >= m_iMaxFileSize*1024*1024 || TMdbDateTime::GetDiffMSecond(sTemp, m_sTime) >= m_iLogTime)	
        {
            SAFE_CLOSE(m_pCurFile);
            
            //将日志文件重命名
            TMdbNtcFileOper::Rename(m_sTempFile.c_str(),m_sOKFile.c_str());
            m_pCurFile= fopen (m_sTempFile.c_str(),"a+");
            if(m_pCurFile == NULL)
            {
                TADD_ERROR(ERR_OS_OPEN_FILE,"Check And Backup  File: Open file [%s] fail.\n",m_sTempFile.c_str());
                return ERR_OS_OPEN_FILE;
            }
            SAFESTRCPY(m_sTime, MAX_TIME_LEN, sTemp);
        }

        return iRet;
    }

    int TMdbCaptureLog::WriteToFile()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(m_pCurFile);
        TADD_DETAIL("data=[%s],len=[%d]",m_sRecd,m_iRecdLen);
		fseek(m_pCurFile,0,SEEK_END);
        if(fwrite(m_sRecd, m_iRecdLen, 1, m_pCurFile) == 0)
        {//写文件
            if(false == TMdbNtcFileOper::IsExist(m_sTempFile.c_str()))
            {
                TADD_ERROR(-1,"File[%s] is not exist",m_sTempFile.c_str());
            }
            else
            {
                TADD_DETAIL("File[%s] is  exist,sData=[%s],ilen=[%d]",m_sTempFile.c_str(),m_sRecd,m_iRecdLen);
            }
            TADD_ERROR(-1,"fwrite() failed, errno=%d, errmsg=[%s]",errno, strerror(errno));
        }
        return iRet;
    }

    int TMdbCaptureLog::Adapt()
    {
         TADD_FUNC("Start.");
        int iRet = 0;

        m_sRecd[0]='\0';
        m_iRecdLen = 0;
        m_tParseRecd.Clear();

        CHECK_RET(m_pParser->DeSerialize(m_pData, m_tParseRecd),"parse record[%s] failed.",m_pData);
        int iSqlType = m_tParseRecd.m_tHeadInfo.m_iSqlType;
        sprintf(m_sRecd,"^^0000,@%d,@%s",iSqlType,m_tParseRecd.m_tHeadInfo.m_sTableName);
        if(TK_INSERT == iSqlType || TK_UPDATE == iSqlType)
        {
            //insert 和update需要
            std::vector<TRepColm>::iterator itor = m_tParseRecd.m_vColms.begin();
            for(; itor != m_tParseRecd.m_vColms.end(); ++itor)
            {
                sprintf(m_sRecd+strlen(m_sRecd),",@%s=",itor->m_sColmName.c_str());
                if(itor->m_bNull)
                {
                    sprintf(m_sRecd+strlen(m_sRecd),"(nil)");
                }
                else
                {
                    sprintf(m_sRecd+strlen(m_sRecd),"%s",itor->m_sColmValue.c_str());
                }
            }
        }
        
        //补充where条件
        if(TK_UPDATE == iSqlType  || TK_DELETE == iSqlType)
        {
            sprintf(m_sRecd+strlen(m_sRecd),"|");
            std::vector<TRepColm>::iterator itor = m_tParseRecd.m_vWColms.begin();
             for(; itor != m_tParseRecd.m_vWColms.end(); ++itor)
            {
                
                sprintf(m_sRecd+strlen(m_sRecd),",@%s=",itor->m_sColmName.c_str());
                if(itor->m_bNull)
                {
                    sprintf(m_sRecd+strlen(m_sRecd),"(nil)");
                }
                else
                {
                    sprintf(m_sRecd+strlen(m_sRecd),"%s",itor->m_sColmValue.c_str());
                }
             }
        }
        sprintf(m_sRecd+strlen(m_sRecd),"##");
        char sTemp[16] = {0};
        int iLen = strlen(m_sRecd);
        sprintf(sTemp,"%04d",iLen);
        memcpy(m_sRecd+2,sTemp,4);//添加长度值
        m_iRecdLen = iLen;
        return iRet;
    }
    
//}
