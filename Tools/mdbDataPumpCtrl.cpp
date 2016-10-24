#include "Tools/mdbDataPumpCtrl.h"
#include "Helper/TThreadLog.h"
#include "Helper/mdbDateTime.h"


//namespace QuickMDB{


    TMdbConnCtrl::TMdbConnCtrl()
    {
        m_pDBLink = NULL;
        m_pQuery = NULL;
    }

    TMdbConnCtrl::~TMdbConnCtrl()
    {
        if(NULL != m_pDBLink )
        {
            m_pDBLink->Disconnect();
        }
        SAFE_DELETE(m_pDBLink);
        SAFE_DELETE(m_pQuery);
    }

    /******************************************************************************
    * ��������  :  Create()
    * ��������  :  ����QUERY
    * ����		:  psDsnName DSN����
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbConnCtrl::Create(const char * psDsnName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(psDsnName);
        m_sDsn = psDsnName;
        if(NULL == m_pDBLink)
        {
            m_pDBLink = new(std::nothrow) TMdbDatabase();
            CHECK_OBJ(m_pDBLink);
        }
        try
        {
            if(!m_pDBLink->IsConnect())
            {
                if(!m_pDBLink->ConnectAsMgr(psDsnName))
                {
                    CHECK_RET(ERR_DB_NOT_CONNECTED,"connect(%s) failed.",psDsnName);
                }
            }

            if(NULL == m_pQuery)
            {
                m_pQuery = m_pDBLink->CreateDBQuery();
                CHECK_OBJ(m_pQuery);
            }
        }
        catch(TMdbException& e)
        {
            CHECK_RET(ERR_DB_NOT_CONNECTED,"connect(%s)ERROR_SQL=%s.\nERROR_MSG=%s\n",
                m_sDsn.c_str(), e.GetErrSql(), e.GetErrMsg());
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  Destroy()
    * ��������  :  ����QUERY
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbConnCtrl::Destroy()
    {
        if(m_pQuery != NULL)
        {
            m_pQuery->Close();
        }
        if(m_pDBLink != NULL)
        {
            m_pDBLink->Disconnect();
        }
        SAFE_DELETE(m_pQuery);
        SAFE_DELETE(m_pDBLink);    
        return 0;
    }

    TWriter::TWriter()
    {
        m_pFile = NULL;
        Clear();
    }

    TWriter::~TWriter()
    {
        SAFE_CLOSE(m_pFile);
    }

    /******************************************************************************
    * ��������  :  Init()
    * ��������  :  ��ʼ��
    * ����		:  sFileName �ļ���(֧�ֺ�·�����ļ���)
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TWriter::Init(const char* sFileName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(sFileName);
        char sTempFile[MAX_PATH_NAME_LEN] = {0};
        char sPath[MAX_PATH_NAME_LEN] = {0};
        Clear();
        //����û��ṩ���ļ����Ƿ���·��
        if(TMdbNtcStrFunc::FindString(sFileName,"/") != -1)
        {
            SAFESTRCPY(sTempFile,sizeof(sTempFile),TMdbNtcFileOper::GetFileName(sFileName));
            strncpy(sPath,sFileName,strlen(sFileName)-strlen(sTempFile));
            bool bExist = TMdbNtcFileOper::IsExist(sPath);
            if(!bExist)
            {
                if(!TMdbNtcDirOper::MakeFullDir(sPath))
                {
                    TADD_ERROR(-1,"Can't create dir=[%s].",sPath);
                    return ERR_OS_CREATE_DIR;
                }
            }
        }
        m_pFile = fopen(sFileName, "wb");
        if(NULL == m_pFile)
        {
           TADD_ERROR(-1,"Open file [%s] fail.",sFileName);
           return ERR_OS_NO_MEMROY;
        }
        setbuf(m_pFile,NULL);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  Write()
    * ��������  :  ʹ�û���д�ļ�
    * ����		:  sData ��Ҫд������
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TWriter::Write(char* sData)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        int iLen = (int)(strlen(sData));
        if(m_iBufPos + iLen < MAX_SEND_BUF)
        {
            strncpy(&m_sBuf[m_iBufPos], sData, (size_t)iLen);
            m_iBufPos+=iLen;
        }
        else
        {
            if(fwrite(m_sBuf, m_iBufPos, 1, m_pFile) == 0)
            {
                TADD_ERROR(-1,"fwrite() failed, errno=%d, errmsg=[%s].", errno, strerror(errno));
                return ERR_OS_WRITE_FILE;
            }
            m_iFileSize+=m_iBufPos;
            strncpy(m_sBuf, sData, (size_t)iLen);
            m_iBufPos = iLen;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  Write()
    * ��������  :  ֱ��д�ļ�
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TWriter::Write()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        if(m_iBufPos <= 0){return iRet;}
        if(fwrite(m_sBuf, m_iBufPos,1,m_pFile) == 0)
        {
            TADD_ERROR(-1,"fwrite() failed, errno=%d, errmsg=[%s].",errno, strerror(errno));
            return ERR_OS_WRITE_FILE;
        }
        m_iFileSize+=m_iBufPos;
        m_iBufPos = 0;
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  Clear()
    * ��������  :  ���
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  ��
    * ����		:  cao.peng
    *******************************************************************************/
    void TWriter::Clear()
    {
        SAFE_CLOSE(m_pFile);
        memset(m_sFileName, 0, MAX_PATH_NAME_LEN);
        memset(m_sBuf, 0, sizeof(m_sBuf));
        m_iBufPos = 0;
        m_iFileSize = 0;
    }

    TMdbExportData::TMdbExportData()
    {
        m_iTableCounts = 0;
        m_pShmDSN = NULL;
        m_iFileFormat = FORMAT_UNKOWN;
        memset(m_sInsertSQL,0,sizeof(m_sInsertSQL));
        memset(m_sSelectSQL,0,sizeof(m_sSelectSQL));
        memset(m_sDumpFile,0,sizeof(m_sDumpFile));
        m_mTableName2Record.clear();
    }

    TMdbExportData::~TMdbExportData()
    {
        m_tConnCtrl.Destroy();
    }

    /******************************************************************************
    * ��������  :  Init()
    * ��������  :  ��ʼ��
    * ����		:  pShmDSNָ�����ڴ���� iFileFormat�ļ���ʽ(1or2)��pDumpFile �����ļ���
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbExportData::Init(TMdbShmDSN * pShmDSN, int iFileFormat, const char * pDumpFile)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pShmDSN);
        CHECK_OBJ(pDumpFile);
        m_pShmDSN = pShmDSN;
        m_iFileFormat = iFileFormat;
        //���ļ�
        char sTempFile[MAX_PATH_NAME_LEN] = {0};
        char sPath[MAX_PATH_NAME_LEN] = {0};
        SAFESTRCPY(sTempFile,sizeof(sTempFile),TMdbNtcFileOper::GetFileName(pDumpFile));
        //����û��ṩ���ļ����Ƿ���·��
        if(TMdbNtcStrFunc::FindString(pDumpFile,"/") != -1)
        {
            strncpy(sPath,pDumpFile,strlen(pDumpFile)-strlen(sTempFile));
        }
        TMdbNtcSplit  tPkSplit;
        tPkSplit.SplitString(sTempFile,'.');
        if(FORMAT_SQL == m_iFileFormat)
        {
            snprintf(sTempFile,sizeof(sTempFile),"%s.sql",tPkSplit[0]);
        }
        else
        {
            snprintf(sTempFile,sizeof(sTempFile),"%s.txt",tPkSplit[0]);
        }
        snprintf(m_sDumpFile,sizeof(m_sDumpFile),"%s%s",sPath,sTempFile);
        CHECK_RET(m_tWriter.Init(m_sDumpFile),"Failed to initialize.");
        //����QUERY
        CHECK_RET(m_tConnCtrl.Create(m_pShmDSN->GetInfo()->sName),"Create Query failed.");
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  ExportByTableName()
    * ��������  :  ����ָ�����ȫ������
    * ����		:  pTableName ����
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbExportData::ExportByTableName(const char * pTableName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pTableName);
        TMdbTable *pMdbTable = NULL;
        TMdbNtcSplit  tTableSplit;
        tTableSplit.SplitString(pTableName,',');
        for(int i = 0; i< tTableSplit.GetFieldCount();i++)
        {
            pMdbTable = m_pShmDSN->GetTableByName(tTableSplit[i]);
            if(NULL == pMdbTable)
            {
                TADD_WARNING("Table[%s] does not exist in the shared memory.",tTableSplit[i]);
                continue;
            }
            if(IsBlobTable(pMdbTable))
            {
                TADD_ERROR(-1,"Table[%s] containing BLOB columns cannot export data.",pMdbTable->sTableName);
                continue;
            }
            if(GetQuerySQL(pMdbTable) < 0)
            {
                TADD_ERROR(-1,"Failed to get query SQL,Table-name = %s.",pMdbTable->sTableName);
                continue;
            }
            if(m_iFileFormat == FORMAT_SQL)
            {
                CHECK_RET(WriteSQLFile(pMdbTable),"Failed to export SQL file,TableName = [%s].",pMdbTable->sTableName);
            }
            else 
            {
                CHECK_RET(WriteTextFile(pMdbTable),"Failed to export SQL file,TableName = [%s].",pMdbTable->sTableName);
            }
            m_mTableName2Record[pMdbTable->sTableName] = m_tConnCtrl.m_pQuery->RowsAffected();
        }
        TADD_FUNC("Finish.");
        return iRet;

    }

    /******************************************************************************
    * ��������  :  ExportBySQL()
    * ��������  :  ͨ��ָ��SQL��������
    * ����		:  pSQL SQL���
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbExportData::ExportBySQL(const char * pSQL)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pSQL);
        char sTableName[MAX_NAME_LEN] = {0};
        TMdbTable *pMdbTable = NULL;
        TMdbNtcSplit  tTableSplit;
        tTableSplit.SplitString(pSQL,':');
        for(int i = 0; i< tTableSplit.GetFieldCount();i++)
        {
            memset(m_sSelectSQL,0,sizeof(m_sSelectSQL));
            SAFESTRCPY(m_sSelectSQL,sizeof(m_sSelectSQL),tTableSplit[i]);
            GetTableNameBySQL(m_sSelectSQL,sTableName,sizeof(sTableName));
            pMdbTable = m_pShmDSN->GetTableByName(sTableName);
            if(NULL == pMdbTable)
            {
                TADD_WARNING("Table[%s] does not exist in the shared memory.",sTableName);
                continue;
            }
            TADD_NORMAL("Table-name = %s,Select-SQL = %s",sTableName,m_sSelectSQL);
            if(m_iFileFormat == FORMAT_SQL)
            {
                CHECK_RET(WriteSQLFile(pMdbTable),"Failed to export SQL file,TableName = [%s].",pMdbTable->sTableName);
            }
            else 
            {
                CHECK_RET(WriteTextFile(pMdbTable),"Failed to export SQL file,TableName = [%s].",pMdbTable->sTableName);
            }
            m_mTableName2Record[pMdbTable->sTableName] = m_tConnCtrl.m_pQuery->RowsAffected();
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  PrintDumpFileInfo()
    * ��������  :  ��ӡ����������Ϣ
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  ��
    * ����		:  cao.peng
    *******************************************************************************/
    void TMdbExportData::PrintDumpFileInfo()
    {
        std::map<string ,int>::iterator itor = m_mTableName2Record.begin();
        for(;itor != m_mTableName2Record.end();++itor)
        {   
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Table-Name = [%s]  Export records = [%d]",\
                    (itor->first).c_str(),itor->second);
            printf("Table-Name = [%s]  Export records = [%d].\n",(itor->first).c_str(),itor->second);
        }
    }

    /******************************************************************************
    * ��������  :  GetTableNameBySQL()
    * ��������  :  ͨ��SQL����ȡ����
    * ����		:  pSQL SQL���
    * ���		:  pTableName ����
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbExportData::GetTableNameBySQL(const char * pSQL,char *pTableName,const int iLen)
    {
        int iRet = 0;
        int iPos = -1;
        char sSQL[MAX_SQL_LEN] = {0};
        TMdbNtcSplit  tTableSplit;
        SAFESTRCPY(sSQL, sizeof(sSQL), pSQL);
        TMdbNtcStrFunc::ToUpper(sSQL);
        iPos = TMdbNtcStrFunc::FindString(sSQL,"FROM");
        if(iPos < 0)
        {   
            TADD_ERROR(-1,"SQL[%s] is invalid.",pSQL);
            return ERR_SQL_INVALID;
        }
        SAFESTRCPY(sSQL,sizeof(sSQL),sSQL + iPos + 4);
        TMdbNtcStrFunc::Trim(sSQL);
        tTableSplit.SplitString(sSQL,' ');
        SAFESTRCPY(pTableName,iLen,tTableSplit[0]);
        return iRet;
    }

    /******************************************************************************
    * ��������  :  IsBlobTable()
    * ��������  :  �жϱ��Ƿ���BLOB��
    * ����		:  pMdbTable �����
    * ���		:  ��
    * ����ֵ    :  true:����BLOB��;false:����BLOB��
    * ����		:  cao.peng
    *******************************************************************************/
    bool TMdbExportData::IsBlobTable(TMdbTable *pMdbTable)
    {
        if(NULL == pMdbTable)
        {
            return false;
        }
        
        for(int iCount = 0;iCount<pMdbTable->iColumnCounts;iCount++)
        {
            if(pMdbTable->tColumn[iCount].iDataType == DT_Blob)
            {
                return true;
            }
        }
        return false;
    }

    /******************************************************************************
    * ��������  :  WriteSQLFile()
    * ��������  :  ��sql��ʽд�ļ����ļ��е����ݸ�ʽ insert into ����(column...)values(value,..);
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbExportData::WriteSQLFile(TMdbTable *pMdbTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sValues[MAX_SEND_BUF] = {0};
        char sSQL[MAX_SEND_BUF] = {0};
        CHECK_OBJ(pMdbTable);
        try
        {
            //ȫ����ѯ
            GetInsertColumnSQL(pMdbTable,false);
            m_tConnCtrl.m_pQuery->Close();
            m_tConnCtrl.m_pQuery->SetSQL(m_sSelectSQL);
            m_tConnCtrl.m_pQuery->Open();
            while(m_tConnCtrl.m_pQuery->Next())
            {
                memset(sValues,0,sizeof(sValues));
                memset(sSQL,0,sizeof(sSQL));
                for(int i = 0;i<pMdbTable->iColumnCounts;i++)
                {
                    if(m_tConnCtrl.m_pQuery->Field(i).isNULL())
                    {
                        snprintf(sValues+strlen(sValues),\
                            sizeof(sValues)-strlen(sValues),"%s,","null");
                        continue;
                    }
                    if(pMdbTable->tColumn[i].iDataType == DT_Int)
                    {
                        snprintf(sValues+strlen(sValues),\
                            sizeof(sValues)-strlen(sValues),\
                            "%lld,",m_tConnCtrl.m_pQuery->Field(i).AsInteger());
                    }
                    else
                    {
                        snprintf(sValues+strlen(sValues),\
                            sizeof(sValues)-strlen(sValues),\
                            "'%s',",m_tConnCtrl.m_pQuery->Field(i).AsString());
                    }
                }
                TMdbNtcStrFunc::TrimRight(sValues,',');
                snprintf(sSQL,sizeof(sSQL),"%s%s);\n",m_sInsertSQL,sValues);
                CHECK_RET(m_tWriter.Write(sSQL),"Failed to write ");
            }
            CHECK_RET(m_tWriter.Write(),"Failed to write ");
        }
        catch(TMdbException& oe)
        {
            TADD_ERROR(-1,"Err-msg=[%s], Err-sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
            return ERR_SQL_INVALID;
        }
        catch(...)
        {
            TADD_ERROR(-1,"Unkown Exception.");
            return ERR_SQL_INVALID;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  WriteTextFile()
    * ��������  :  дtxt�ļ����ļ���ʽΪ:
    * ��������  :  ���=��̬SQL
    * ��������  :  ���,@��ֵ,@��ֵ....;
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbExportData::WriteTextFile(TMdbTable *pMdbTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pMdbTable);
        char sValues[MAX_SEND_BUF] = {0};
        char sNullValue[MAX_FILE_NAME] = {0};
        char sHead[MAX_FILE_NAME] = {0};
        char sTmpValues[MAX_SEND_BUF] = {0};
        bool bIsNull = false;
        int iSelectCounts = 0;
        try
        {
            //ȫ����ѯ
            m_tConnCtrl.m_pQuery->Close();
            m_tConnCtrl.m_pQuery->SetSQL(m_sSelectSQL);
            //��д��ͷ��Ϣ
            m_iTableCounts ++;
            GetInsertColumnSQL(pMdbTable,true);
            snprintf(sHead,sizeof(sHead),"%d=%s\n",m_iTableCounts,m_sInsertSQL);
            m_tConnCtrl.m_pQuery->Open();
            while(m_tConnCtrl.m_pQuery->Next())
            {
                iSelectCounts ++;
                bIsNull = false;
                memset(sValues,0,sizeof(sValues));
                memset(sNullValue,0,sizeof(sNullValue));
                snprintf(sNullValue,sizeof(sNullValue),"%s",",@99=");
                if(iSelectCounts == 1)
                {
                    CHECK_RET(m_tWriter.Write(sHead),"Failed to write.");
                }
                for(int i = 0;i<pMdbTable->iColumnCounts;i++)
                {
                    if(m_tConnCtrl.m_pQuery->Field(i).isNULL())
                    {
                        bIsNull = true;
                        if(i<10)
                        {
                            snprintf(sNullValue+strlen(sNullValue),\
                                sizeof(sNullValue)-strlen(sNullValue),"0%d",i);
                        }
                        else
                        {
                            snprintf(sNullValue+strlen(sNullValue),\
                                sizeof(sNullValue)-strlen(sNullValue),"%d",i);
                        }
                        snprintf(sValues+strlen(sValues),\
                            sizeof(sValues)-strlen(sValues),"%s,@"," ");
                        continue;
                    }
                    
                    if(pMdbTable->tColumn[i].iDataType == DT_Int)
                    {
                        snprintf(sValues+strlen(sValues),\
                            sizeof(sValues)-strlen(sValues),\
                            "%lld,@",m_tConnCtrl.m_pQuery->Field(i).AsInteger());
                    }
                    else
                    {
                        snprintf(sValues+strlen(sValues),\
                            sizeof(sValues)-strlen(sValues),\
                            "'%s',@",m_tConnCtrl.m_pQuery->Field(i).AsString());
                    }
                }
                sValues[strlen(sValues)-2]='\0';
                if(bIsNull)
                {
                    snprintf(sTmpValues,sizeof(sTmpValues),"%d,@%s%s;\n",m_iTableCounts,sValues,sNullValue);
                }
                else
                {
                    snprintf(sTmpValues,sizeof(sTmpValues),"%d,@%s;\n",m_iTableCounts,sValues);
                }
                CHECK_RET(m_tWriter.Write(sTmpValues),"Failed to write ");
            }
            CHECK_RET(m_tWriter.Write(),"Failed to write ");
        }
        catch(TMdbException& oe)
        {
            TADD_ERROR(-1,"Err-msg=[%s], Err-sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
            return ERR_SQL_INVALID;
        }
        catch(...)
        {
            TADD_ERROR(-1,"Unkown Exception.");
            return ERR_SQL_INVALID;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  GetQuerySQL()
    * ��������  :  ��ȡָ����Ĳ�ѯSQL
    * ����		:  pMdbTable �����
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbExportData::GetQuerySQL(TMdbTable *pMdbTable)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        memset(m_sSelectSQL,0,sizeof(m_sSelectSQL));
        //����ƴдselect����
        SAFESTRCPY(m_sSelectSQL,sizeof(m_sSelectSQL),"select ");
        for(int i=0; i<pMdbTable->iColumnCounts; ++i)
        {
            if(0 == i)
            {
                snprintf(m_sSelectSQL+strlen(m_sSelectSQL),\
                    sizeof(m_sSelectSQL)- strlen(m_sSelectSQL),\
                    "%s", pMdbTable->tColumn[i].sName);
            }
            else
            {
                snprintf(m_sSelectSQL+strlen(m_sSelectSQL),\
                    sizeof(m_sSelectSQL)- strlen(m_sSelectSQL),\
                    ",%s",pMdbTable->tColumn[i].sName);
            }
        }

        snprintf(m_sSelectSQL+strlen(m_sSelectSQL),sizeof(m_sSelectSQL)- strlen(m_sSelectSQL),\
                    " from %s", pMdbTable->sTableName);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  GetInsertColumnSQL()
    * ��������  :  ��ȡָ����Ĳ���SQL��Ϣ
    * ����		:  pMdbTable ����� bIsValue �Ƿ�ƴ��values�����ֵ
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbExportData::GetInsertColumnSQL(TMdbTable *pMdbTable,bool bIsValue)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pMdbTable);
        memset(m_sInsertSQL, 0, sizeof(m_sInsertSQL));
        //����ƴдinsert����
        snprintf(m_sInsertSQL,sizeof(m_sInsertSQL), "insert into %s (", pMdbTable->sTableName);
        for(int i=0; i<pMdbTable->iColumnCounts; ++i)
        {
            if(0 == i)
            {
                snprintf(m_sInsertSQL+strlen(m_sInsertSQL),\
                    sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),\
                    "%s", pMdbTable->tColumn[i].sName);
            }
            else
            {
                snprintf(m_sInsertSQL+strlen(m_sInsertSQL),\
                    sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),\
                    ",%s", pMdbTable->tColumn[i].sName);
            }
        }
        snprintf(m_sInsertSQL+strlen(m_sInsertSQL),\
            sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),"%s", ") values (");
        if(bIsValue)
        {
            //ƴдvalue����
            for(int i=0; i<pMdbTable->iColumnCounts; ++i)
            {
                if(0 == i)
                {
                    snprintf(m_sInsertSQL+strlen(m_sInsertSQL),\
                        sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),\
                        ":%s", pMdbTable->tColumn[i].sName);
                }
                else
                {
                    snprintf(m_sInsertSQL+strlen(m_sInsertSQL),\
                        sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),\
                        ",:%s", pMdbTable->tColumn[i].sName);
                }
            }
            snprintf(m_sInsertSQL+strlen(m_sInsertSQL),\
                        sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),\
                        "%s", ");");
        }
        TADD_FUNC("Finish.");
        return iRet;

    }

    TMdbImportData::TMdbImportData()
    {
        m_pShmDSN = NULL;
        m_pFile = NULL;
        m_pMdbTable = NULL;
        m_iFileFormat = FORMAT_UNKOWN;
        m_iTotalRecord = 0;
        memset(m_sInsertSQL,0,sizeof(m_sInsertSQL));
        m_mSeq2SQL.clear();
        m_mSeq2ConCtrl.clear();
    }

    TMdbImportData::~TMdbImportData()
    {
        SAFE_CLOSE(m_pFile);
        m_mSeq2SQL.clear();
        std::map<int ,TMdbConnCtrl* >::iterator itor = m_mSeq2ConCtrl.begin();
        for(;itor != m_mSeq2ConCtrl.end();++itor)
        {
            SAFE_DELETE(itor->second);
        }
        m_mSeq2ConCtrl.clear();
    }

    /******************************************************************************
    * ��������  :  Init()
    * ��������  :  ��ʼ��
    * ����		:  pShmDSN ָ�����ڴ����pDumpFile �����ļ���
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbImportData::Init(TMdbShmDSN * pShmDSN,const char * pDumpFile)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pShmDSN);
        CHECK_OBJ(pDumpFile);
        char sTempFile[MAX_PATH_NAME_LEN] = {0};
        //�ж��ļ��Ƿ����
        if(!TMdbNtcFileOper::IsExist(pDumpFile))
        {
            TADD_ERROR(-1,"File[%s] does not exist.",pDumpFile);
            return ERR_APP_FILE_IS_EXIST;
        }
        //�ж��ļ�����(*.sql����*.txt)
        SAFESTRCPY(sTempFile,sizeof(sTempFile),TMdbNtcFileOper::GetFileName(pDumpFile));
        TMdbNtcSplit  tPkSplit;
        tPkSplit.SplitString(sTempFile,'.');
        int iIndexCounts = tPkSplit.GetFieldCount();
        if(iIndexCounts == 0)
        {
            TADD_ERROR(-1,"File name format is wrong,it must be *.sql or *.txt.");
            return ERROR_UNKNOWN;
        }
        if(TMdbNtcStrFunc::StrNoCaseCmp(tPkSplit[iIndexCounts-1],"sql") == 0)
        {
            m_iFileFormat = FORMAT_SQL;
        }
        else if(TMdbNtcStrFunc::StrNoCaseCmp(tPkSplit[iIndexCounts-1],"txt") == 0)
        {
            m_iFileFormat = FORMAT_TEXT;
        }
        else
        {
            m_iFileFormat = FORMAT_UNKOWN;
            TADD_ERROR(-1,"File name format is wrong,it must be *.sql or *.txt.");
            return ERROR_UNKNOWN;
        }
        //���ļ�
        m_pFile = fopen(pDumpFile, "rt");
        if(m_pFile == NULL)
        {
            TADD_ERROR(-1,"Failed to open file[%s].",pDumpFile);
            return ERR_OS_READ_FILE;   
        }
        m_pShmDSN = pShmDSN;
        //����QUERY
        CHECK_RET(m_tConnCtrl.Create(m_pShmDSN->GetInfo()->sName),"Create Query failed.");
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  ImportData()
    * ��������  :  �����ļ�
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbImportData::ImportData()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        switch(m_iFileFormat)
        {
            case FORMAT_SQL:
                CHECK_RET(ImportSqlFile(),"Failed to import sql file.");
                break;
            case FORMAT_TEXT:
                CHECK_RET(ImportTextFile(),"Failed to import text file.");
                break;
            default:
                TADD_ERROR(-1,"File format is wrong,it must be *.sql or *.txt.");
                break;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  PrintDumpFileInfo()
    * ��������  :  ��ӡ������ļ���¼��Ϣ
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  ��
    * ����		:  cao.peng
    *******************************************************************************/
    void TMdbImportData::PrintDumpFileInfo()
    {
        /*TMdbConnCtrl* pConnCtrl = NULL;
        if(m_iFileFormat == FORMAT_SQL)
        {
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Total number of imported records = [%d]",m_iTotalRecord);
            printf("Total number of imported records = [%d].\n",m_iTotalRecord);
            return;
        }
        std::map<int ,TMdbConnCtrl* >::iterator itor = m_mSeq2ConCtrl.begin();
        for(;itor != m_mSeq2ConCtrl.end();++itor)
        {   
            pConnCtrl = itor->second;
            TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Table-Name = [%s]  Import records = [%d]",
                pConnCtrl->m_pQuery->m_pMdbSqlParser->m_stSqlStruct.pMdbTable->sTableName,
                pConnCtrl->m_pQuery->RowsAffected());
            printf("Table-Name = [%s]  Import records = [%d].\n",
                pConnCtrl->m_pQuery->m_pMdbSqlParser->m_stSqlStruct.pMdbTable->sTableName,
                pConnCtrl->m_pQuery->RowsAffected());
        }*/

		
		TADD_NORMAL_TO_CLI(FMT_CLI_OK,"Total number of imported records = [%d]",m_iTotalRecord);
		printf("Total number of imported records = [%d].\n",m_iTotalRecord);
		return;
		
    }

    /******************************************************************************
    * ��������  :  ImportSqlFile()
    * ��������  :  ����sql�ļ�����
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbImportData::ImportSqlFile()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sBufMsg[MAX_VALUE_LEN] = {0};
        m_iTotalRecord = 0;
        while(fgets(sBufMsg, sizeof(sBufMsg), m_pFile) != NULL)
        {
            //ȥ�����з�\n \t ��
            TMdbNtcStrFunc::FormatChar(sBufMsg);
            //���������Լ�ע�ͺ���
            if (sBufMsg[0] == '\n' || strlen(sBufMsg) == 0)
            {
                continue;
            } 
            iRet = ExecuteStaticSQL(sBufMsg);
            if(iRet < 0)
            {
                TADD_ERROR(-1,"Failed to execute SQL[%s].",sBufMsg);
                continue;
            }
            m_iTotalRecord ++;
            if(m_iTotalRecord%5000 == 0)
            {
                m_tConnCtrl.m_pQuery->Commit();
            }
        }
        m_tConnCtrl.m_pQuery->Commit();
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  ImportTextFile()
    * ��������  :  ����txt�ļ���Ϣ
    * ����		:  ��
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbImportData::ImportTextFile()
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        char sBufMsg[MAX_VALUE_LEN] = {0};
		m_iTotalRecord = 0;
        while(fgets(sBufMsg, sizeof(sBufMsg), m_pFile) != NULL)
        {
            int iSqlSeq = -1;
            //ȥ�����з�\n \t ��
            TMdbNtcStrFunc::FormatChar(sBufMsg);
            //���������Լ�ע�ͺ���
            if (sBufMsg[0] == '\n' || strlen(sBufMsg) == 0)
            {
                continue;
            }
            //����
            TADD_NORMAL("sBufMsg = %s",sBufMsg);
            iRet = ParseRecord(sBufMsg,iSqlSeq);
            if(iRet != 0)
            {
                TADD_ERROR(-1,"Invalid data: %s.",sBufMsg);
                continue;
            }
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  ExecuteStaticSQL()
    * ��������  :  ִ��SQL
    * ����		:  pSQL SQL���
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbImportData::ExecuteStaticSQL(const char *pSQL)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        try
        {
            m_tConnCtrl.m_pQuery->Close();
            m_tConnCtrl.m_pQuery->SetSQL(pSQL);        
            m_tConnCtrl.m_pQuery->Execute();
        }
        catch(TMdbException& oe)
        {
            TADD_ERROR(-1,"Err-msg=[%s], Err-sql=[%s]", oe.GetErrMsg(), oe.GetErrSql());
            return ERR_SQL_INVALID;
        }
        catch(...)
        {
            TADD_ERROR(-1,"Unkown Exception.");
            return ERR_SQL_INVALID;
        }
        TADD_FUNC("Finish.");
        return iRet;  
    }

    /******************************************************************************
    * ��������  :  ParseRecord()
    * ��������  :  ������ִ�е����ļ���¼
    * ����		:  pData ����
    * ���		:  iSeqSQL sql������к�
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbImportData::ParseRecord(char* pData,int &iSeqSQL)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //������ĳ�ű������
        TMdbNtcSplit mdbSplit;
        TMdbNtcStrFunc::TrimRight(pData,';');
        int iEqual = TMdbNtcStrFunc::FindString(pData,"=");

		mdbSplit.SplitString(pData, ",@");
		int iSplitCount =  mdbSplit.GetFieldCount();
        // ���� = �����Ҳ����� ",@" ������ִ��sql��
        if(iEqual > 0 && iSplitCount < 2)
        {
            ST_SQL_INFO stSQL;
            TMdbConnCtrl *pConnCtrl = new(std::nothrow) TMdbConnCtrl();
            CHECK_OBJ(pConnCtrl);
            char sSqlSeq[32] = {0};
            strncpy(sSqlSeq,pData,iEqual);
            iSeqSQL = TMdbNtcStrFunc::StrToInt(sSqlSeq);
            stSQL.vSql2Param.clear();
            stSQL.sSQL = pData + iEqual + 1;
            iRet = pConnCtrl->Create(m_pShmDSN->GetInfo()->sName);
            if(iRet < 0){return iRet;}
            try
            {
                pConnCtrl->m_pQuery->Close();
                pConnCtrl->m_pQuery->SetSQL(stSQL.sSQL.c_str());
            }
            catch(TMdbException& e)
            {
                TADD_ERROR(-1,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
                return ERROR_UNKNOWN;
            }
            catch(...)
            {
                TADD_ERROR(-1,"Unknown error!\n");   
                return ERROR_UNKNOWN;    
            }  
            stSQL.iParamLineCounts = 0;
            stSQL.iSqlSeq = iSeqSQL;
            m_mSeq2SQL[iSeqSQL] = stSQL;
            m_mSeq2ConCtrl[iSeqSQL] = pConnCtrl;
        }
        else if(iSplitCount >= 2)//���� = �������ǲ�����
        {
            //��ȡNULL��λ��
            bool bIsExistNULL = false;
            GetNullColumnPos(pData,bIsExistNULL);
            int iCounts = bIsExistNULL?iSplitCount-1:iSplitCount;
            iSeqSQL = TMdbNtcStrFunc::StrToInt(mdbSplit[0]);
            for(int i=1;i<iCounts;i++)
            {
                ST_PARAM stParam;
                stParam.sParamLine = pData;
                stParam.iSqlSeq = iSeqSQL;
                stParam.iIndex = i-1;
                stParam.sValue = mdbSplit[i];
                //�ж�ֵ���� 
                if(TMdbNtcStrFunc::IsDigital(mdbSplit[i]))
                {
                    stParam.iDataType = DT_Int;
                }
                else
                {
                    stParam.iDataType = DT_VarChar;
                }
                m_mSeq2SQL[iSeqSQL].vSql2Param.push_back(stParam);//ST_PARAM
            }  
            //ִ��SQL
            ExecuteDynamicStatic(iSeqSQL);
        }
        else
        {
            return ERR_SQL_INVALID;
        }
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  ExecuteDynamicStatic()
    * ��������  :  ��ִ̬��SQL����Ҫ�󶨲���
    * ����		:  iSeqSQL sql���
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbImportData::ExecuteDynamicStatic(int iSeqSQL)
    {
        int iRet = 0;
        try
        {
            ST_SQL_INFO & stSQL = m_mSeq2SQL[iSeqSQL];   
            TMdbConnCtrl *pConnCtrl = m_mSeq2ConCtrl[iSeqSQL];
            CHECK_OBJ(pConnCtrl);
            CHECK_OBJ(pConnCtrl->m_pQuery);
            TADD_FLOW("iSqlSeq = %d,sSQL = %s.",stSQL.iSqlSeq,stSQL.sSQL.c_str());
            std::vector<ST_PARAM> &vParam = stSQL.vSql2Param;
            for(vector<ST_PARAM>::size_type j=0;j != vParam.size();++j)
            {
                int iDataType = vParam[j].iDataType;
                //�ж��Ƿ�ΪNULLֵ
                if(m_tData[j].isNull == -1)
                {
                    pConnCtrl->m_pQuery->SetParameterNULL(j);
                    continue;
                }
                if(iDataType == DT_Int)
                {
                    long long llValue = TMdbNtcStrFunc::StrToInt(vParam[j].sValue.c_str());
                    pConnCtrl->m_pQuery->SetParameter(vParam[j].iIndex,llValue);
                }
                else 
                {
                    char sValueTemp[MAX_CMD_LEN] = {0};
                    SAFESTRCPY(sValueTemp,sizeof(sValueTemp),vParam[j].sValue.c_str());
                    TMdbNtcStrFunc::Trim(sValueTemp,'\'');
                    pConnCtrl->m_pQuery->SetParameter(vParam[j].iIndex,sValueTemp);
                }
            }
            vParam.clear();
            pConnCtrl->m_pQuery->Execute();
            stSQL.iParamLineCounts ++;
			m_iTotalRecord ++;
            if(stSQL.iParamLineCounts %1000 == 0)
            {
                pConnCtrl->m_pQuery->Commit();
            }
        }
        catch(TMdbException& e)
        {
            TADD_ERROR(-1,"ERROR_SQL=%s.\nERROR_MSG=%s\n", e.GetErrSql(), e.GetErrMsg());   
            return ERROR_UNKNOWN;
        }
        catch(...)
        {
            TADD_ERROR(-1,"Unknown error!\n");   
            return ERROR_UNKNOWN;    
        }  
        return iRet;
    }

    /******************************************************************************
    * ��������  :  GetNullColumnPos()
    * ��������  :  ��ȡ�����¼�е�nullֵ����Ϣ
    * ����		:  pData ��¼
    * ���		:  bIsExistNull �Ƿ���NULLֵ
    * ����ֵ    :  void
    * ����		:  cao.peng
    *******************************************************************************/
    void TMdbImportData::GetNullColumnPos(const char * pData,bool &bIsExistNull)
    {
        TADD_FUNC("Start.");
        int iPos = 0;
        int iStartPos = 0;
        int iLen = 0;
        char sNull[MAX_SQL_LEN];
        for(int j=0;j<MAX_COLUMN_COUNTS;j++)
        {
            m_tData[j].Clear();
        }
        while(pData[iPos] != '\0' && pData[iPos] != '|' && pData[iPos] !='#')//\0 ��|Ϊ��ֹ��
        {

            if(pData[iPos] == ',' && pData[iPos+1] == '@' && pData[iPos+2] == '9' && pData[iPos+3] == '9' && pData[iPos+4] == '=')
            {
                iPos+=5;//����һ��=�ţ�����Ҫ���һλ
                iStartPos=iPos;
                while(pData[iStartPos]  != ',' && pData[iStartPos] != '\0' && pData[iStartPos] != '|' && pData[iStartPos] !='#')
                {
                    iStartPos++;
                }
                if(iStartPos == iPos)
                {
                    break;
                }
                iLen = iStartPos - iPos;

                memcpy(sNull,&pData[iPos],iLen);
                sNull[iLen] = 0;
                break;
            }
            iPos++;
        }
        if(iLen > 0)
        {
            int iCurPos = -1;
            for(int i = 0; i<iLen; i+=2)
            {
                iCurPos = (sNull[i]-'0')*10+(sNull[i+1]-'0');
                m_tData[iCurPos].isNull = -1;
            }
            bIsExistNull = true;
        }
        TADD_FUNC("Finish.");
        return;
    }

    /******************************************************************************
    * ��������  :  GetInsertSQL()
    * ��������  :  ��ȡ����SQL
    * ����		:  pTableName ����
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbImportData::GetInsertSQL(const char* pTableName)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pTableName);
        memset(m_sInsertSQL, 0, sizeof(m_sInsertSQL));
        m_pMdbTable = m_pShmDSN->GetTableByName(pTableName);
        if(NULL == m_pMdbTable)
        {
            TADD_WARNING("Table[%s] does not exist in the shared memory.",pTableName);
            return ERR_TAB_NO_TABLE;
        }
        //����ƴдinsert����
        snprintf(m_sInsertSQL,sizeof(m_sInsertSQL),"insert into %s (", m_pMdbTable->sTableName);
        for(int i=0; i<m_pMdbTable->iColumnCounts; ++i)
        {
            if(0 == i)
            {
                snprintf(m_sInsertSQL+strlen(m_sInsertSQL),
                    sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),\
                    "%s", m_pMdbTable->tColumn[i].sName);
            }
            else
            {
                snprintf(m_sInsertSQL+strlen(m_sInsertSQL),
                    sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),\
                    ",%s", m_pMdbTable->tColumn[i].sName);
            }
        }
        snprintf(m_sInsertSQL+strlen(m_sInsertSQL),sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),"%s", ") values (");
        //ƴдvalue����
        for(int i=0; i<m_pMdbTable->iColumnCounts; ++i)
        {
            if(0 == i)
            {
                snprintf(m_sInsertSQL+strlen(m_sInsertSQL),
                    sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),\
                    ":%s", m_pMdbTable->tColumn[i].sName);
            }
            else
            {
                snprintf(m_sInsertSQL+strlen(m_sInsertSQL),
                    sizeof(m_sInsertSQL)-strlen(m_sInsertSQL),\
                    ",:%s", m_pMdbTable->tColumn[i].sName);
            }
        }
        snprintf(m_sInsertSQL+strlen(m_sInsertSQL),sizeof(m_sInsertSQL)-strlen(m_sInsertSQL), "%s", ")");
        TADD_FUNC("Finish.");
        return iRet;
    }

    TMdbDataPumpCtrl::TMdbDataPumpCtrl()
    {
        m_pShmDSN = NULL;
        m_pDsn = NULL;
    }

    TMdbDataPumpCtrl::~TMdbDataPumpCtrl()
    {
        
    }

    /******************************************************************************
    * ��������  :  Init()
    * ��������  :  ��ʼ��
    * ����		:  pDsn DSN����
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbDataPumpCtrl::Init(const char * pDsn)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pDsn);
        m_pShmDSN =  TMdbShmMgr::GetShmDSN(pDsn);
        CHECK_OBJ(m_pShmDSN);
        m_pDsn = m_pShmDSN->GetInfo();
        CHECK_OBJ(m_pDsn);
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  ExportData()
    * ��������  :  ��������
    * ����		:  pTableName ������cFileFormat �����ļ���ʽ
    * ����		:  pDumpFile �����ļ�����pSQL ��������������SQL
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbDataPumpCtrl::ExportData(const char * pTableName, char cFileFormat, const char * pDumpFile, const char * pSQL)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        //ͨ��sql����������
        CHECK_OBJ(pDumpFile);
        int iFileFormat = 0;
        switch(toupper(cFileFormat))
        {
            case 'S':
                iFileFormat = FORMAT_SQL;
                break;
            case 'T':
                iFileFormat = FORMAT_TEXT;
                break;
            default:
                iFileFormat = FORMAT_SQL;
                break;
        }
        CHECK_RET(m_tExportData.Init(m_pShmDSN,iFileFormat,pDumpFile),"Failed to initialize.");
        if(NULL == pTableName || strlen(pTableName) ==0)
        {
            TADD_NORMAL("iFileFormat = %d,pDumpFile = %s,pSQL = [%s].",iFileFormat,pDumpFile,pSQL);
            CHECK_RET(m_tExportData.ExportBySQL(pSQL),\
                    "Failed to export data through SQL[%s].",pSQL);
        }
        else
        {//ͨ������ȫ����������
            TADD_NORMAL("pTableName = %s,iFileFormat = %d,pDumpFile = %s",pTableName,iFileFormat,pDumpFile);
            CHECK_RET(m_tExportData.ExportByTableName(pTableName),\
                    "Failed to export data through tablename [%s]",pTableName);
        }
        m_tExportData.PrintDumpFileInfo();
        TADD_FUNC("Finish.");
        return iRet;
    }

    /******************************************************************************
    * ��������  :  ImportData()
    * ��������  :  ��������
    * ����		:  pDumpFile �����ļ���֧��*sql��*.txt��ʽ��
    * ���		:  ��
    * ����ֵ    :  0 - �ɹ�!0 -ʧ��
    * ����		:  cao.peng
    *******************************************************************************/
    int TMdbDataPumpCtrl::ImportData(const char * pDumpFile)
    {
        TADD_FUNC("Start.");
        int iRet = 0;
        CHECK_OBJ(pDumpFile);
        CHECK_RET(m_tImportData.Init(m_pShmDSN,pDumpFile),"Failed to initialize.");
        CHECK_RET(m_tImportData.ImportData(),"Failed to import file[%s]",pDumpFile);
        m_tImportData.PrintDumpFileInfo();
        TADD_FUNC("Finish.");
        return iRet;
    }
//}    

